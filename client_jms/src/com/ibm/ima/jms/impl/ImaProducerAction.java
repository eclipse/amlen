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


/*
 *  TODO:
 */
public class ImaProducerAction extends ImaSessionAction {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    int producerID;
    // senddestname is needed for unit tests using the ImaLoopbackClient
    String senddestname;
    /*
     * Create an action.
     * This action is used for sending messages for a given producer
     * and for closing the producer (type = Producer)
     */
    ImaProducerAction (int action, ImaSession session, int producerID) {
        super(action,session,1024);
        outBB.put(ITEM_TYPE_POS, ItemTypes.Producer.value());
        outBB.putInt(ITEM_POS, producerID);
        this.producerID = producerID;
    }

    /*
     * Create an action.
     * This action is used for sending messages without a producer (type = Session).
     */
    ImaProducerAction (int action, ImaSession session, int domain, String destname) throws JMSException {
        super(action,session,1024);
        outBB = ImaUtils.putByteValue(outBB, (byte)domain);
        outBB = ImaUtils.putStringValue(outBB, destname);
        setHeaderCount(2);
    }
    
    /*
     * Set the header fields of a message 
     */
    void setMessageHdr(int msgtype, int deliverymode, int priority, boolean disableack, boolean expire, boolean retain) {
        outBB.put(BODY_TYPE_POS,(byte)msgtype);
        
        byte flags = 0;
        if (deliverymode == javax.jms.DeliveryMode.PERSISTENT) {
            /* if ACKs disabled, AT_LEAST_ONCE otherwise EXACTLY_ONCE */
            flags = (byte) (disableack ? 0x81 : 0x82);
        }
        else {
            /* If ACKs disabled, AT_MOST_ONCE, otherwise AT_LEAST_ONCE */
            flags = (byte) (disableack ? 0x00 : 0x01);
        }
        if (expire) {
        	flags |= 0x20;
        }
        if (retain)
            flags |= 0x48;        /* retain, retain for publish */
        outBB.put(FLAGS_POS,(byte)flags);
        outBB.put(PRIORITY_POS,(byte)priority);
    }
    
    /*
     * Reset the action with a producer ID
     */
    void reset(int action, int producerID) {
        super.reset(action);
        outBB.put(ITEM_TYPE_POS, ItemTypes.Producer.value());
        outBB.putInt(ITEM_POS, producerID);
    }
    
    /*
     * Reset the action with a destination
     */
    void reset(int action, int domain, String destname) throws JMSException {
        super.reset(action);
        outBB = ImaUtils.putByteValue(outBB, (byte)domain);
        outBB = ImaUtils.putStringValue(outBB, destname);
        setHeaderCount(2);
    }
}
