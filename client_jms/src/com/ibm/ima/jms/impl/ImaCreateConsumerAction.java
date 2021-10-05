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

import javax.jms.JMSException;


/*
 * Create a session related action. 
 */
public class ImaCreateConsumerAction extends ImaSessionAction {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    final ImaMessageConsumer           consumer;
    
    /*
     * Create an action with a default buffer size
     */
    ImaCreateConsumerAction (int action, ImaSession session, ImaMessageConsumer cons) {
        super(action, session);
        consumer = cons;
    }

    protected void parseReply(ByteBuffer bb) {
    	super.parseReply(bb);
    	if (rc == 0) {
            try {
                if (!session.connect.isStopped)
                    consumer.startDelivery();
        		consumer.consumerid = ImaUtils.getInteger(inBB);
                session.consumerList.add((ImaConsumer)consumer);
                session.connect.consumerMap.put(Integer.valueOf(consumer.consumerid), consumer);        		
            } catch (JMSException e) {
                rc = ImaReturnCode.IMARC_BadClientData;    
            }
    	}
    }
}
