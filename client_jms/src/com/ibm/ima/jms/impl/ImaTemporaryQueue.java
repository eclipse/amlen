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

//import java.io.IOException;
import java.util.HashSet;

import javax.jms.JMSException;
import javax.jms.TemporaryQueue;


/*
 * This class implements TemporaryQueue and TemporaryQueue.  
 * These are only a small delta from ImaDestination, but includes object references
 * which we do not want in the serilaizable ImaDestination class.
 */
public class ImaTemporaryQueue extends ImaQueue implements TemporaryQueue {
    private static final long serialVersionUID = -2510681951440265342L;
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    ImaConnection connect;
    HashSet<ImaMessageConsumer> consumers = new HashSet<ImaMessageConsumer>();
    
    public ImaTemporaryQueue(String name, ImaConnection connect) throws JMSException {
        super(name);
        this.connect = connect;
        if (ImaTrace.isTraceable(8)) {
            ImaTrace.trace("Create destination: "+name);
        }
        ImaAction act = new ImaConnectionAction(ImaAction.createDestination, connect.client);
        act.outBB = ImaUtils.putByteValue(act.outBB, (byte)ImaJms.Queue);        /* val0 = domain */
        act.outBB = ImaUtils.putStringValue(act.outBB, name);                    /* val1 = name */
        act.outBB = ImaUtils.putIntValue(act.outBB, 0);                          /* val2 = maxcount */
        act.outBB = ImaUtils.putBooleanValue(act.outBB, true);                   /* val3 = concurrent */
        act.setHeaderCount(4);
        act.request();
        
        if (act.rc != 0) {
        	/* TODO: more error handling */
            if (ImaTrace.isTraceable(2)) {
                ImaTrace.trace("Failed to create temporary queue: rc=" + act.rc +
                    " name=" + name +		
                    " connect=" + connect.toString(ImaConstants.TS_All));
            }
            ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0026", "A client request to create a temporary queue failed with MessageSight return code = {0}.", act.rc);
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }
    
    /*
     * Delete the temporary Queue/queue/destination.
     * As these objects have no actual presence in IMA, there is no action needed to delete them. 
     * @see javax.jms.TemporaryQueue#delete()
     */
    public void delete() throws JMSException {
        if (!consumers.isEmpty()) {
        	ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0005", "A method called to delete a temporary topic or a temporary queue failed because the temporary destination has active message consumers.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }
    
    /*
     * Add a consumer to our list 
     */
    void addConsumer(ImaMessageConsumer consumer) {
        synchronized(this) {
            consumers.add(consumer);    
        }
    }
    
    /*
     * Remove a consumer from our list
     */
    void removeConsumer(ImaMessageConsumer consumer) {
        synchronized(this) {
            consumers.remove(consumer);
        }
    }
    
//    /*
//     * Do not allow serialization of a temporary destination.
//     */
//    private void writeObject(java.io.ObjectOutputStream out) throws IOException {
//        // TODO: AddMsgID
//    	IOException iex = new IOException("A temporary destination cannot be serilaized");
//        ImaTrace.traceException(2, iex);
//        throw iex;
//    }
    
    /*
     * (non-Javadoc)
     * @see com.ibm.ima.jms.impl.ImaTopic#getClassName()
     */
    public String getClassName() {
    	return "ImaTemporaryTopic";
    }
}

