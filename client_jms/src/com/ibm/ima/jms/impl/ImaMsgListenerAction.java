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


/*
 * Create a session related action. 
 */
public class ImaMsgListenerAction extends ImaConsumerAction {   

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


	private final ImaMessageConsumer consumer;
	private final ImaReceiveAction rcvAction;
    /*
     * Create an action with a default buffer size
     */
    ImaMsgListenerAction (ImaMessageConsumer consumer, ImaReceiveAction rcvAction) {
        super(ImaAction.setMsgListener, consumer.session, consumer.consumerid);
        this.consumer = consumer;
        this.rcvAction = rcvAction;
    }
    ImaMsgListenerAction (ImaMessageConsumer consumer) {
        super(ImaAction.removeMsgListener, consumer.session, consumer.consumerid);
        this.consumer = consumer;
        this.rcvAction = null;
    }

    protected void parseReply(ByteBuffer bb) {
    	super.parseReply(bb);
    	if (rc == 0) {
    		while (!consumer.receivedMessages.isEmpty()) {
    			ImaMessage savedMsg = consumer.receivedMessages.remove();
    			consumer.deliveryThread.addTask(rcvAction, savedMsg);
                if (ImaTrace.isTraceable(9)) {
                    ImaTrace.trace("Move message consumer receive to delivery queue: consumer=" + consumer.consumerid + 
                            " ackSqn=" + savedMsg.ack_sqn);
                }                       
    		}
    		consumer.rcvAction = rcvAction;
    	}
    }
}
