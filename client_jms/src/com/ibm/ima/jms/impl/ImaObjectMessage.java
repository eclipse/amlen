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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.nio.ByteBuffer;
import java.security.AccessController;
import java.security.PrivilegedAction;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.ObjectMessage;


/**
 * Implement the ObjectMessage interface for the IBM MessageSight JMS client
 *
 */
public class ImaObjectMessage extends ImaMessage implements ObjectMessage {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    static Boolean disallowGetObject = true;
    static {
        try {
            disallowGetObject = (Boolean) AccessController.doPrivileged(new PrivilegedAction<Object>() {
                public Object run() {
                   return new Boolean(System.getProperty("IMAEnforceObjectMessageSecurity", "true"));
                }
            });
        } catch (Exception e) {}
    }

    Object obj;
    /*
     * Constructor for the ObjectMessage 
     */
    public ImaObjectMessage(ImaSession session) {
        super(session, MTYPE_ObjectMessage);
    }
    
    /*
     * Constructor for the ObjectMessage from another ObjectMessage.
     */
    public ImaObjectMessage(ObjectMessage msg, ImaSession session) throws JMSException {
        super(session, MTYPE_ObjectMessage, (Message)msg);
        setObject(msg.getObject());
    }
    

    /*
     * Internal method called when content is initialized.
     * This is similar to reset, but only the body and bodylen are assumed to be set.
     * 
     * @see com.ibm.ima.jms.impl.ImaMessage#initContent()
     */
    public void initContent() throws JMSException {
        super.initContent();
        obj = null;
        if (body != null)
        	body.position(0);
        isReadonly = true;
    }
    

    /* 
     * @see javax.jms.ObjectMessage#getObject()
     */
    public Serializable getObject() throws JMSException {
        if (body == null || body.limit() == 0)
            return null;
    	if(disallowGetObject) {
    		throw new ImaJmsSecurityException("CWLNC0077", "A call to getObject() on an ObjectMessage failed because this method is disabled by default for security purposes.");
    	}
        if (obj != null)
            return (Serializable)obj;
        try {
            final ByteArrayInputStream bais;
            if (body.hasArray()) {
            	bais = new ByteArrayInputStream(body.array());
            } else {
            	body.position(0);
            	byte[] ba = new byte[body.limit()];
            	body.get(ba);
            	bais = new ByteArrayInputStream(ba);
            }
            ObjectInputStream ois = new ObjectInputStream(bais);
            Serializable obj = (Serializable) ois.readObject();
            return obj;
        } catch (Exception e) {
        	ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0075", e, "A call to getObject() on an ObjectMessage failed due to an exception during deserialization.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
    }
    
    /*
     * Constructor for a cloned received message
     */
    public ImaObjectMessage(ImaMessage msg) {
        super(msg);
    }

    /* 
     * @see javax.jms.ObjectMessage#setObject(java.io.Serializable)
     */
    public void setObject(Serializable obj) throws JMSException {
        if (isReadonly) {
        	ImaMessageNotWriteableException jex = new ImaMessageNotWriteableException("CWLNC0038", "A write operation failed on a message object body or on a message object property because the message was in read only state.");
            ImaTrace.traceException(2, jex);
            throw jex;
        }
        if (obj == null) {
            body = null;
            this.obj = obj;
        } else {
            try {
                ByteArrayOutputStream baos = new ByteArrayOutputStream();
                ObjectOutputStream oos = new ObjectOutputStream(baos);
                oos.writeObject(obj);
                byte[] ba = baos.toByteArray();
                body = ByteBuffer.wrap(ba);
                this.obj = obj;
            } catch (Exception e) {
            	ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0041", e, "A call to the setObject() method on an ObjectMessage failed due to an exception that occurred during serialization.");
                ImaTrace.traceException(2, jex);
                throw jex;
            }
        }    
    }
    
	public void clearBody() throws JMSException {
		body = null;
		isReadonly = false;
	}
}
