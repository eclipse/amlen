/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.plugin;

/**
 * Define the type of the message body.
 */
public enum ImaMessageType {
    
    /** A message where the body is uninterpreted bytes */
    bytes,
    
    /** A message where the body is text in UTF-8 encoding */
    text,
    
    /** A message where the body is a JSON object */
    jsonObject,
    
    /** A message where the body is a JSON array */
    jsonArray,
    
    /** A JMS message with no body */
    jmsMessage,
    
    /** A JMS BytesMessage */
    jmsBytesMessage,
    
    /** A JMS MapMessage */
    jmsMapMessage,   
    
    /** A JMS ObjectMessage */
    jmsObjectMessage, 
    
    /** A JMS StreamMessage */
    jmsStreamMessage,  
    
    /** A JMS TextMessage */
    jmsTextMessage,
    
    /** A null retained message which will cause the retained message to be deleted */
    nullRetained;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
