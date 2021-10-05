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
import javax.jms.TemporaryTopic;

/*
 * This class implements TemporaryTopic and TemporaryQueue.  
 * These are only a small delta from ImaDestination, but includes object references
 * which we do not want in the serializeable ImaDestination class.
 */
public class ImaTemporaryTopic extends ImaTopic implements TemporaryTopic {
    private static final long serialVersionUID = -6469636347668272943L;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    ImaConnection connect;
    HashSet<ImaMessageConsumer> consumers = new HashSet<ImaMessageConsumer>();
    
    public ImaTemporaryTopic(String name, ImaConnection connect) throws JMSException {
        super(name);
        this.connect = connect;
    }
    
    /*
     * Delete the temporary topic/queue/destination.
     * As these objects have no actual presence in IMA, there is no action needed to delete them. 
     * @see javax.jms.TemporaryTopic#delete()
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
