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

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.TextMessage;

/**
 * Implement the TextMessage interface for the IBM MessageSight JMS client.
 *
 */
public class ImaTextMessage extends ImaMessage implements TextMessage {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    String text;
    
    /*
     * Constructor for the TextMessage 
     */
    public ImaTextMessage(ImaSession session) {
        super(session, MTYPE_TextMessage);
    }
    
    /*
     * Constructor for the TextMessage 
     */
    public ImaTextMessage(ImaSession session, String text) throws JMSException {
        super(session, MTYPE_TextMessage);
        setText(text);
    }
    
    /*
     * Constructor for the TextMessage from a TextMessage. 
     */
    public ImaTextMessage(TextMessage msg, ImaSession session) throws JMSException {
        super(session, MTYPE_TextMessage, (Message)msg);
        setText(msg.getText());
    }
    
    /*
     * Constructor for a cloned received message
     */
    public ImaTextMessage(ImaMessage msg) {
        super(msg);
    }
    
    /* 
     * Clear the body.
     * This will allow the body to be written, but not read.
     * 
     * @see javax.jms.Message#clearBody()
     */
    public void clearBody() throws JMSException {
        super.clearBody();
        setText(null);
    }
    
    /* 
     * @see javax.jms.TextMessage#getText()
     */
    public String getText() throws JMSException {
        if (body == null)
            return null;
        if (text != null)
            return text;
        /* Ignore a UTF-8 BOM if present TODO: check this */
        if (!(body.limit() >= 3 && body.get()==(byte)0xef && body.get()==(byte)0xbb && body.get()==(byte)0xbf))
            body.position(0);
        text = ImaUtils.fromUTF8(body, body.remaining(), true);
        return text;
    }

    
    /* 
     * @see javax.jms.TextMessage#setText(java.lang.String)
     */
    public void setText(String text) throws JMSException {
        if (text == null) {
            body = null;
            this.text = null;
        } else {
            int utflen = ImaUtils.sizeUTF8(text);
            if (utflen < 0) {
            	ImaMessageFormatException jex = new ImaMessageFormatException("CWLNC0039", "A request failed because the string ({0}) contains characters that do not form a valid UTF-16 string. This is usually caused by mismatched surrogate characters.", text);
                ImaTrace.traceException(2, jex);
                throw jex;
            }

            body = ImaUtils.makeUTF8(text, utflen, null);
            this.text = text;
        }   
    }
    
    /*
     * (non-Javadoc)
     * @see com.ibm.ima.jms.impl.ImaMessage#reset()
     */
    public void reset() throws JMSException {
        if (!isReadonly) {
        	if (body != null)
        		body.flip();
        }
        isReadonly = true;
    }

    /*
     * (non-Javadoc)
     * @see com.ibm.ima.jms.impl.ImaMessage#formatBody()
     */
    public String formatBody() {
    	String ret = null;
    	try {
    	    ret = "\"" + getText() + "\"";
        } catch (Exception ex) {}     
    	return ret;
    }
}
