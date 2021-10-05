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
import java.util.Enumeration;
import java.util.Iterator;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import javax.jms.JMSException;
import javax.jms.Queue;
import javax.jms.QueueBrowser;


/*
 * A queue browser allows message on a queue to be seen without being removed.
 * 
 * To browse a queue you create the browser from the session.  This only does minimal
 * checking and does not contact the server.  You then do a getEnumeration which does the
 * final checking and calls the server to create a message consumer object.  You can
 * have multiple enumerators in the same browser.
 */
public class ImaQueueBrowser extends ImaReadonlyProperties implements QueueBrowser { 

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    String selector;
    ImaMessageSelector select;         /* The compiled selector */
    ImaSession     session;
    ImaConnection  connect;
    boolean        isClosed;           /* A flag to indicate that the consumer is closed */
    ImaQueue       queue;              /* The destination to listen to */
    int            id;
    int            fullSize;              
    int            emptySize;
    final AtomicInteger  enumCount = new AtomicInteger(0);
    
	/*
	 * Constructor for a queue browser
	 * @param session    The session in which the browser exists
	 * @param queue      The queue to browse
	 * @param selector   The selection string or null for no selector
	 * @throws JMSException
	 */
    public ImaQueueBrowser(ImaSession session, ImaQueue queue, String selector) throws JMSException {
        super(null);
        this.session = session;
        this.selector = selector;
        this.queue = queue;
        connect = session.connect;
        
        /* 
         * Compile selector
         */
        if (selector != null) {
            select = new ImaMessageSelector(selector);
        }

        this.props = queue.getCurrentProperties();
        
        fullSize = 1000;
        emptySize = fullSize/4;
        
        putInternal("isClosed", false);
        putInternal("Connection", connect);
        putInternal("Session", session);
    }    
    
    

    /*
     * @see javax.jms.QueueBrowser.getEnumeration()
     */
    public Enumeration<?> getEnumeration() throws JMSException {
        ImaAction act = new ImaSessionAction(ImaAction.createBrowser, session);
        act.outBB = ImaUtils.putStringValue(act.outBB, queue.getQueueName()); /* val0 = queue name */ 
        act.outBB = ImaUtils.putStringValue(act.outBB, selector);             /* val1 = selector */
        act.setHeaderCount(2);
        act.request(true);

        
        if (act.rc == 0) {
            id = ImaUtils.getInteger(act.inBB);
            enumCount.incrementAndGet();
        } else {
            switch (act.rc) {
       		case ImaReturnCode.IMARC_NotAuthorized:
       		case ImaReturnCode.IMARC_NotAuthenticated: 
                ImaTrace.trace(2, "Failed to create browser enumeration: Authorization failed. " +
                        " queue=" + queue.getQueueName());
                ImaJmsSecurityException jex = new ImaJmsSecurityException("CWLNC0207", "A client request to MessageSight failed due to an authorization failure.");
                ImaTrace.traceException(2, jex);
                throw jex;
                
            default:    
                ImaTrace.trace(2, "Failed to create browser enumeration: rc=" + act.rc +
                            " queue=" + queue.getQueueName());
                ImaJmsExceptionImpl jex2 = new ImaJmsExceptionImpl("CWLNC0220", "A request to create a queue browser failed with MessageSight return code = {0}.", act.rc);
                ImaTrace.traceException(2, jex2);
                throw jex2;
            }
        }

        /*
         * Create the actual consumer
         */
        Enumeration<ImaMessage> browser = new QEnumerator(id);
        
        /* 
         * Start the consumer
         */
        act = new ImaConsumerAction(ImaAction.startConsumer, session, id);
        synchronized (session) {
            session.checkClosed();
            act.request(false);
            session.consumerList.add((ImaConsumer) browser);
        }
        
        if (ImaTrace.isTraceable(6)) {
            ImaTrace.trace("Browser is created: browser=" + ((ImaConsumer)browser).toString(ImaConstants.TS_All));
        }
        
        return browser;
    }
	
    /*
     * @see javax.jms.QueueBrowser.close()
     */
	public void close() throws JMSException {
		closeInternal();
	}
	
    /*
     * Internal close without notification to the session.
     * This is used to implement the close method, and to implement the close of the parent session.
     */
    public void closeInternal() throws JMSException {
        synchronized (this) {
            if (!isClosed && enumCount.get() > 0) {
            	// Do NOT set isClosed yet.  
            	// Doing so will prevent close request from
            	// being sent to the server.

                /*
                 * Close all browsers for this session
                 */  
            	Iterator<ImaConsumer> iter = session.consumerList.iterator();
            	while(iter.hasNext()){
            		ImaConsumer cons = iter.next();
                	if (cons instanceof QEnumerator && ((QEnumerator)cons).browser == this){
                		iter.remove();
                	    cons.closeInternal();
                	}
            	}
            }   
            isClosed = true;
            ((ImaPropertiesImpl)this.props).put("isClosed", true);
        	if (ImaTrace.isTraceable(6)) {
        		ImaTrace.trace("Browser is closed: browser=" + toString(ImaConstants.TS_Common) + " session=" + session.toString(ImaConstants.TS_Common));
        	}
        }    
    }
    
    public void markClosed() {
        synchronized (this) {
            if (!isClosed && enumCount.get() > 0) {
  
                Iterator<ImaConsumer> iter = session.consumerList.iterator();
                while(iter.hasNext()){
                    ImaConsumer cons = iter.next();
                    if (cons instanceof QEnumerator && ((QEnumerator)cons).browser == this){
                        iter.remove();
                        cons.markClosed();
                    }
                }
            }   
            isClosed = true;
            ((ImaPropertiesImpl)this.props).putInternal("isClosed", true);
            if (ImaTrace.isTraceable(6)) {
                ImaTrace.trace("Browser is marked closed: browser=" + toString(ImaConstants.TS_Common) + " session=" + session.toString(ImaConstants.TS_Common));
            }
        }    
    }

    
	/*
	 * @see javax.jms.QueueBrowser.getMessageSelector()
	 */
	public String getMessageSelector() throws JMSException {
        checkClosed();
        return selector;
	}
	
	/*
	 * @see javax.jms.QueueBrowser.getQueue()
	 */
	public Queue getQueue() throws JMSException {
        checkClosed();
        return queue;
	}

    /* 
     * Internal check if the session is closed.
     */
    void checkClosed() throws JMSException {
        if (isClosed) {
        	ImaIllegalStateException jex = new ImaIllegalStateException("CWLNC0015", "A call to a queue browser object method failed because the queue browser is closed.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }
    

    
    /*
     * Implement an internal class which returns messages from a queue browser
     */
	class QEnumerator implements Enumeration<ImaMessage>, ImaConsumer {
    	int   id;
    	boolean isClosed = false;
    	ImaQueueBrowser browser; 
    	LinkedBlockingDeque<ImaMessage> list; 
	    private int  msgCount = 0;
	    private int  suspendCount = 0;
	    ImaAction cbAction = null;
	    private final ImaConsumerAction    resumeAction;
	    private final Lock mutex = new ReentrantLock();    
		
    	/*
    	 * Constructor for queue enumerator
    	 */
        public QEnumerator(int id) throws JMSException {
        	this.id = id;
        	this.browser = ImaQueueBrowser.this;
        	list = new LinkedBlockingDeque<ImaMessage>();
    		session.connect.consumerMap.put(id, this);
    		resumeAction = new ImaConsumerAction(ImaAction.startConsumer, session, id);
    	}
         
    	/*
    	 * @see java.util.Enumeration#hasMoreElements()
    	 */
		public boolean hasMoreElements() {
			if (isClosed)
				return false;
		    boolean hasmore = false;
			/*
             * if message on queue return true, otherwise send checkBrowser to see whether 
             * the server still indicates there are unconsumed messages
             */
		    hasmore = !list.isEmpty();
			if (hasmore)
				return true;
			
			/* Check the number of unconsumed messages on the server */
			boolean serverHasMore = false;
			if (cbAction != null)
				cbAction.reset(ImaAction.checkBrowser);
			else
				cbAction = new ImaConsumerAction(ImaAction.checkBrowser, session, id);
			try {
				cbAction.request(true);
			    serverHasMore = (ImaUtils.getInteger(cbAction.inBB) == 1);
			} catch (JMSException jex) {
				ImaTrace.traceException(2, jex);
			}
			
			if (serverHasMore) {
			    requestResume();
				if (list.isEmpty()) {
					ImaMessage element = null;
					/* Need to synchronize to assure that a new message is not added to the list before the
					 * polled message is put back.
					 */
                    try { element = list.pollFirst(1000, TimeUnit.MILLISECONDS); } catch (InterruptedException ex) {}
                    if (element != null) {
                        if (ImaTrace.isTraceable(9)) {
				            ImaTrace.trace("Putting message "+element.toString(ImaConstants.TS_All)+" back into browser enumeration cache (browser=" + id + ").");
                        }
                        list.addFirst(element);
                    }
                    else if (ImaTrace.isTraceable(9)) {
			             ImaTrace.trace("No elements received for browsing (browser=" + id + ").");
                    }
				}
			}
			
			hasmore = !list.isEmpty();
			
			if (!hasmore && ImaTrace.isTraceable(9))
			    ImaTrace.trace("No elements to browse (browser=" + id + "): " + (serverHasMore ? "messages " : "no messages ") + "found on server, client cache size(" + list.size() + ")");
			
			return hasmore;
		}

		private final void cachedMessageRemoved(ImaMessage msg){
		    boolean sendResume = false;
		    mutex.lock();
		    try{
		        msgCount--;
		        if (suspendCount > 0){
		            if (msgCount <= emptySize ){
		                suspendCount--;
		                sendResume = true;
		            }
		        } else {
		            if (msg.suspend)
		                sendResume = true;
		        }
	            if (ImaTrace.isTraceable(9)) {
	                ImaTrace.trace("Remove message from browser enumeration cache: browser=" + id + 
	                        " msgCount=" + msgCount + " msg=" + msg.toString(ImaConstants.TS_All));
	            }
		    } finally {
		        mutex.unlock();
		    }
		    if (sendResume)
		        requestResume();
		}
		/*
		 * @see java.util.Enumeration#nextElement()
		 */
		public ImaMessage nextElement() {
			/*
			 * if message on queue return it.
			 */
			if (isClosed) {
				ImaNoSuchElementException nsex = new ImaNoSuchElementException("CWLNC0050", "A call to nextElement() on a QueueBrowser's Enumeration object failed because the browser is closed.");
				ImaTrace.traceException(6, nsex);
				throw nsex;
			}
			if (list.isEmpty() && !hasMoreElements()) {
				/* 
				 * Do not trace at level 2 as consumers may use this exception to determine there
				 * is no available browse data.
				 */
				ImaNoSuchElementException nex = new ImaNoSuchElementException("CWLNC0051", "A call to nextElement() on a QueueBrowser's Enumeration object failed because there are no more elements to be browsed.");
				ImaTrace.traceException(6, nex);
				throw nex;
			}
			
			ImaMessage msg = list.poll();			
			cachedMessageRemoved(msg);
			return msg;
		}
		
	    /*
	     * @see com.ibm.ima.jms.impl.ImaConsumer#getDest()
	     */
	    public ImaDestination getDest() {
	    	return (ImaDestination)queue;
	    }
	    
	    /*
	     * @see com.ibm.ima.jms.impl.ImaConsumer.getConsumerID()
	     */
	    public int getID() {
	    	return id;
	    }
	    
	    /*
	     * @see com.ibm.ima.jms.impl.ImaConsumer.getSession()
	     */
	    public ImaSession getSession() {
	    	return session;
	    }

	    /*
	     * @see com.ibm.ima.jms.impl.ImaConsumer.closeInternal()
	     */
		public void closeInternal() throws JMSException {
	        synchronized (this) {
	            if (!this.isClosed) {
	                this.isClosed = true;
	                ImaAction act = new ImaConsumerAction(ImaAction.closeConsumer, session, id);
	                act.request(); 
	           		browser.session.connect.consumerMap.remove(id);
	           		
	            	if (ImaTrace.isTraceable(6)) { 
	            		ImaTrace.trace("Browser enumerator is closed: browser enumerator=" + this.toString(ImaConstants.TS_All));
	            	}
	            	enumCount.decrementAndGet();
	            	if (enumCount.get() == 0) browser.closeInternal();
	            }    
	        } 
		}
	      public void markClosed() {
	            synchronized (this) {
	                if (!this.isClosed) {
	                    this.isClosed = true; 
	                    browser.session.connect.consumerMap.remove(id);
	                    
	                    if (ImaTrace.isTraceable(6)) { 
	                        ImaTrace.trace("Browser enumerator is marked closed: browser enumerator=" + this.toString(ImaConstants.TS_All));
	                    }
	                    enumCount.decrementAndGet();
	                    if (enumCount.get() == 0) browser.markClosed();
	                }    
	            } 
	        }
		private final void requestResume(){
            resumeAction.reset(ImaAction.startConsumer);                    
            try {
                resumeAction.request(false);
            } catch (JMSException e) {
                if (!isClosed) {
                    connect.onError(e);
                    return;
                }
            }
            if (ImaTrace.isTraceable(8)) {
                ImaTrace.trace("Resume browser: id= " + id + " browser=" + toString(ImaConstants.TS_Common));
            }
		}
	    		
	    /*
	     * Default object to current browser
	     */
	    public String toString(int opt) {
	    	return toString(opt, browser);
	    }
	    
	    /*
	     * Put out the detailed tostring for the subclassed object
	     */
	    public String toString(int opt, ImaReadonlyProperties props) {
	    	StringBuffer sb = new StringBuffer();
	    	boolean started = false;

	    	if (props == null) {
	    		return "";
	    	}
	    	
	    	if ((opt & ImaConstants.TS_Class) != 0) {
	    		sb.append(props.getClassName());
	    		started = true;
	    	}
	    	if ((opt & ImaConstants.TS_HashCode) != 0) {
	    		sb.append('@').append(Integer.toHexString(hashCode()));
	    		started = true;
	    	}
	    	if ((opt & (ImaConstants.TS_Info|ImaConstants.TS_Properties) ) != 0) {
	    		if (started)
	    			sb.append(' ');
	    		started = true;
	    		String info = this.getInfo();
	    		if (info != null) {
	    			sb.append(info).append(' ');
	    		}
	    		sb.append("properties=").append(props.props);
	    	}

	    	if ((opt & ImaConstants.TS_Details) != 0) {
	    		String details = props.getDetails();
	    		if (details != null) {
		    		if (started)
		    			sb.append("\n  ");
		    		started = true;
		    		sb.append(details);
	    		}
	    	}

	    	return sb.toString();
	    }
	    
	    public String getInfo() {
	    	return id >= 0 ? ("id="+id) : null;
	    }
	    

		@Override
		public void handleMessage(ByteBuffer bb) throws JMSException {
			ImaMessage msg = ImaMessage.makeMessage(bb, session, this);
			mutex.lock();
			try {
			    msgCount++;
			    if (msg.suspend)
			        suspendCount++;
	            if (ImaTrace.isTraceable(9)) {
	                ImaTrace.trace("Add message to browser enumeration cache: browser=" + id + 
	                        " msgCount=" + msgCount + " msg=" + msg.toString(ImaConstants.TS_All));
	            }                
            } finally {
                mutex.unlock();
            }
			list.add(msg);
		}
    }
}
