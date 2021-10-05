/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

package com.ibm.ima.jms.impl;

import java.nio.ByteBuffer;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.LinkedList;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import javax.jms.JMSException;

/*
 *  
 */
public abstract class ImaAction implements Cloneable, ImaConstants{
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    static final int    HEADER_START   = 4;    
    static final int    ACTION_POS     = HEADER_START;
    static final int    FLAGS_POS      = ACTION_POS+1;
    static final int    HDR_COUNT_POS  = FLAGS_POS+1;
    static final int    BODY_TYPE_POS  = HDR_COUNT_POS + 1;
    static final int    PRIORITY_POS   = BODY_TYPE_POS+1;
    static final int    DUP_POS        = PRIORITY_POS+1;
    static final int    ITEM_TYPE_POS  = DUP_POS+2;
    static final int    MSG_ID_POS     = ITEM_TYPE_POS + 1;
    static final int    ITEM_POS       = MSG_ID_POS+8;
    static final int    HEADER_END     = ITEM_POS+4;
    static final int    HEADER_LENGTH  = HEADER_END - HEADER_START;
    static final int    DATA_POS       = HEADER_END;

    /**
     * Defines action item types. Should be synchronized with equivalent
     * enum in jms.c
     */
    public enum ItemTypes {
        None     ((byte)0),
        Thread   ((byte)1),
        Session  ((byte)2),
        Consumer ((byte)3),
        Producer ((byte)4);
        
        private byte value;
        
        ItemTypes(byte value) {
            this.value = value;
        }
        
        public byte value() {
            return value;
        }
    }
        
    public static final int message             = 1;
    public static final int messageWait         = 2;
    public static final int messageNoProd       = 3;
    public static final int messageNoProdWait   = 4;
    public static final int receiveMsg          = 5;
    public static final int receiveMsgNoWait    = 6;
    public static final int reply               = 9;
    public static final int createConnection    = 10;
    public static final int startConnection     = 11;
    public static final int stopConnection      = 12;
    public static final int closeConnection     = 13;
    public static final int createSession       = 14;
    public static final int closeSession        = 15;
    public static final int createConsumer      = 16;
    public static final int createBrowser       = 17;
    public static final int createDurable       = 18;
    public static final int unsubscribeDurable  = 19;
    public static final int closeConsumer       = 20;
    public static final int createProducer      = 21;
    public static final int closeProducer       = 22;
    public static final int setMsgListener      = 23;
    public static final int removeMsgListener   = 24;
    public static final int rollbackSession     = 25;
    public static final int commitSession       = 26;
    public static final int ack                 = 27;
    public static final int ackSync             = 28;
    public static final int recover             = 29;
    public static final int getTime             = 30;
    public static final int createTransaction   = 31;
    public static final int setWill             = 32;
    public static final int checkBrowser        = 33;
    public static final int resumeSession       = 34;
    public static final int stopSession         = 35;
    public static final int raiseException      = 36;
    public static final int startConsumer       = 37;
    public static final int suspendConsumer     = 38;
    public static final int createDestination   = 39;
    /* Transport layer calls                       */
    public static final int initConnection      = 40;
    public static final int termConnection      = 43;
    /* ISMC-specific                               */
    public static final int updateSubscription  = 44;
    /* 45-50 in use by ISMC for XA support         */
    /* 51 in use by ISMC for listing subscriptions */
    public static final int startGlobalTrans    = 52;
    public static final int endGlobalTrans      = 53;
    public static final int prepareGlobalTrans  = 54;
    public static final int recoverGlobalTrans  = 55;
    public static final int commitGlobalTrans   = 56;
    public static final int rollbackGlobalTrans = 57;
    public static final int forgetGlobalTrans   = 58;
    
    private    final Lock lock = new ReentrantLock();
    private final Condition cv = lock.newCondition();
    int             action;
    int             corrid=-1;
    int             hdrpos = 4;
    int                rc = 1;
    ByteBuffer      inBB;
    ByteBuffer      outBB;
    private 		JMSException cancelReason = null;
    private static  final int MAX_ACTION_ID; 
        
    private static final ImaAction     corrObj[];
    private static final Thread        activeThreads[];
    static {
        int m = 4096;
        try {
            String s = null;
            s = (String) AccessController.doPrivileged(new PrivilegedAction<Object>() {
                public Object run() {
                    return System.getenv("IMAMaxJMSThreads");
                }
            });
            if ((s != null) && (s.length() > 0))
                m = Integer.parseInt(s);
        } catch (Throwable e) {
            m = 4096;
        }
        MAX_ACTION_ID = m;
        corrObj = new ImaAction[MAX_ACTION_ID+1];
        activeThreads = new Thread[MAX_ACTION_ID + 1];
    }

    /*
     * Create the correlation index for this thread
     */
    private static final synchronized int getThreadCorrelationID() {
        for (int i = 1; i < activeThreads.length; i++) {
            if ((activeThreads[i] == null) || (activeThreads[i].getState() == Thread.State.TERMINATED)) {
                activeThreads[i] = Thread.currentThread();
                return i;
            }
        }
        IndexOutOfBoundsException iex = new IndexOutOfBoundsException(
                "The number of IBM MessageSight client threads within a single JMS process cannot exceed "
                        + MAX_ACTION_ID);
        ImaTrace.traceException(2, iex);
        throw iex;
    }

    static final ThreadLocal < Integer > threadNo = 
        new ThreadLocal < Integer > () {
            protected Integer initialValue() {
                return Integer.valueOf(getThreadCorrelationID());
            }
    };
    
    /*
     * Create an action.
     */
    ImaAction (int action) {
        this(action,256);
    }
    ImaAction (int action, int size) {
        outBB = ByteBuffer.allocateDirect(size);
        corrid = threadNo.get();
        reset(action);
    }
    
    void reset(int action) {
        this.action = action;
        outBB.clear();
        outBB.put(ACTION_POS,(byte)action);
        outBB.put(FLAGS_POS,(byte) 0);
        outBB.put(HDR_COUNT_POS,(byte) 0);
        outBB.put(BODY_TYPE_POS,(byte) 0);
        outBB.put(PRIORITY_POS,(byte) 0);
        outBB.put(DUP_POS,(byte) 0);
        outBB.put(ITEM_TYPE_POS, (byte)ItemTypes.Thread.value());    // Item is THREAD, no item info
        corrid = threadNo.get();
        outBB.putLong(MSG_ID_POS, corrid);                            // Store response id
        outBB.position(DATA_POS);
        rc = 1;
        cancelReason = null;
        if (inBB != null)
            inBB.clear();        
    }
    /*
     * Set the count of header items
     */
    void setHeaderCount(int count) {
        outBB.put(HDR_COUNT_POS,(byte) count);
    }
    
    /* Increase the count of header items */
    void incHeaderCount(int value) {
        int count = outBB.get(HDR_COUNT_POS) + value;
        outBB.put(HDR_COUNT_POS,(byte) count);
    }
    
    /*
     * Get the action we are waiting for
     */
    static  ImaAction getAction(long respid) {
        return corrObj[(int)respid];
    }
    static synchronized void setAction(long respid, ImaAction action) {
        corrObj[(int)respid] = action;
    }
    
    /*
     * Make the request
     */
    abstract void request(boolean syncCall) throws JMSException ; 
    void request() throws JMSException {
        request(true); 
    }
    /*
     * Make the request with extra body bytes.
     * For now we just copy them into the action and make the request
     */
    protected void request(ByteBuffer bb, boolean syncCall) throws JMSException {
        if (bb != null) {
            int length = bb.limit();
            outBB = ImaUtils.ensureBuffer(outBB, length+32);
            ImaUtils.putSmallValue(outBB,length, S_ByteArray);
            bb.position(0);
            outBB.put(bb);
        } else {
            outBB = ImaUtils.putNullValue(outBB);
        }
            
        request(syncCall);
    }
    
    /*
     * Wait for a result.
     * There is a small window in the wait mechanism which we handle by
     * doing a timeout.  The length of the timeout increases from 1 to 128ms.
     */
    final void await() throws JMSException {
    	try {
        	awaitNanos(Long.MAX_VALUE);
    	} catch (InterruptedException ex) {
    		ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0044", ex, "A request failed due to an InterruptedException when waiting for a response from MessageSight.");
    		ImaTrace.traceException(2, jex);
    		throw jex;
    	}
    }
    
    /*
     * Wait for a result or timeout.
     * @return <= 0, if timed out; >0 otherwise
     */
    final long awaitNanos(long nanosTimeout) throws InterruptedException, JMSException {
        long ret = 0;
        lock.lock();
        try {
            if ((rc == 1) && (cancelReason == null))
                ret = cv.awaitNanos(nanosTimeout);
        } finally {
            lock.unlock();
        }
        if (cancelReason != null)
            throw cancelReason;

        return ret;
    }    
    
    /*
     * Parse the reply from an action
     */
    protected void parseReply(ByteBuffer bb) {
        int length = bb.limit();
        if ((inBB == null) || (inBB.capacity() < length))
            inBB = ByteBuffer.allocate(length+1024);
        inBB.put(bb);

        if (inBB.get(ACTION_POS) != ImaAction.reply)
            inBB.put(ACTION_POS, (byte)ImaAction.message);
        hdrpos = HEADER_START;
        inBB.flip();
        inBB.position(DATA_POS);
        
        /* Return code is in the 1st header, if available */
        rc = 0;
        int headerCount = inBB.get(HDR_COUNT_POS);
        if (headerCount > 0) {
            try {
                rc = ImaUtils.getInteger(inBB);
            } catch (JMSException e) {
                rc = ImaReturnCode.IMARC_BadClientData;    /* Bad data */
            }
            if (rc == 1)
                rc = 0;
        }
    }
    protected final void complete(ByteBuffer bb) {
        lock.lock();
        try{
        	parseReply(bb);
        }finally{
            corrObj[corrid] = null;
            cv.signal();
            lock.unlock();
        }
    }

    //For JUNIT
    final void complete() {
        lock.lock();
        try {
            cv.signal();
            corrObj[corrid] = null;
        } finally {
            lock.unlock();
        }
    }
    
    /*
     * Put a full big-endian long. 
     * This code assumes that there is space in the body.
     */
    void putLongBig(int offset, long val) {
        outBB.putLong(offset,val);
    }

    public Object clone()throws CloneNotSupportedException{
        return super.clone();
    }
    /*
     * Return the name of an action
     */
    public static String actionName(int action) {
        switch (action) {
        case message             : return("message");
        case messageWait         : return("messageWait");
        case ack                 : return("ack");
        case reply               : return("reply");
        case receiveMsg          : return("receiveMsg");
        case receiveMsgNoWait    : return("receiveMsgNoWait");
        case createConnection    : return("createConnection");
        case startConnection     : return("startConnection");
        case stopConnection      : return("stopConnection");
        case closeConnection     : return("closeConnection");
        case createSession       : return("createSession");
        case closeSession        : return("closeSession");
        case createConsumer      : return("createConsumer");
        case createBrowser       : return("createBrowser");
        case createDurable       : return("createDurable");
        case unsubscribeDurable  : return("unsubscribeDurable");
        case closeConsumer       : return("closeConsumer");
        case createProducer      : return("createProducer");
        case closeProducer       : return("closeProducer");
        case setMsgListener      : return("setMessageListener");
        case removeMsgListener   : return("removeMessageListener");
        case rollbackSession     : return("rollbackSession");
        case commitSession       : return("commitSession");
        case recover             : return("recover");
        case getTime             : return("getTime");
        case createTransaction   : return("createTransaction");
        case ackSync             : return("ackSync");
        case raiseException      : return("raiseException");
        case startConsumer       : return("startConsumer");
        case suspendConsumer     : return("suspendConsumer");
        case createDestination   : return("createDestination");
        case initConnection      : return("initConnection");
        case termConnection      : return("termConnection");
        case startGlobalTrans    : return("startGlobalTransaction");
        case endGlobalTrans      : return("endGlobalTransaction");
        case prepareGlobalTrans      : return("prepareSession");
        case recoverGlobalTrans  : return("recoverTransaction");
        }
        return "Unknown";
    }

    /*
     * Cancel an action
     */
    void cancel(JMSException reason) {
        lock.lock();
        try {
            cv.signal();
            cancelReason = reason;
            setAction(corrid, null);
        } finally {
            lock.unlock();
        }
    }
    
    /*
     * Get the list of pending actions
     */
    static synchronized LinkedList<ImaAction> getPendingActions() {
    	LinkedList<ImaAction> result = new LinkedList<ImaAction>();
    	for (int i = 0; i < corrObj.length; i++) {
			if (corrObj[i] != null)
				result.addLast(corrObj[i]);
		}
    	return result;
    }
}
