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
import java.util.Iterator;

import javax.jms.JMSException;


/*
 * Start a connection action. 
 */
public class ImaStartConnectionAction extends ImaConnectionAction {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    final ImaConnection           connection;
    
    /*
     * Create an action with a default buffer size
     */
    ImaStartConnectionAction (ImaConnection con) {
        super(ImaAction.startConnection, con.client);
        connection = con;
    }

    protected void parseReply(ByteBuffer bb) {
    	super.parseReply(bb);
    	if (rc == 0) {
    	    int headerCount = inBB.get(HDR_COUNT_POS);
    	    if (headerCount > 1 ){
    	        try {
                    int counter = ImaUtils.getInteger(inBB);
                    for(int i = 0; i < counter; i++){
                        int sessID = ImaUtils.getInteger(inBB);
                        long sqn = ImaUtils.getLong(inBB);
                        ImaSession sess = connection.sessionsMap.get(new Integer(sessID));
                        if (sess != null)
                            sess.setLastDeliveredSqn(sqn);
                    }
                } catch (JMSException e) {
                    rc = ImaReturnCode.IMARC_BadClientData;
                    return;
                }
    	    }
            Iterator<ImaConsumer> it = connection.consumerMap.values().iterator();
            while(it.hasNext()){
                ImaConsumer ic = it.next();
                if (ic instanceof ImaMessageConsumer) {
                    ImaMessageConsumer consumer = (ImaMessageConsumer) ic;
                    consumer.startDelivery();
                }
            }
    	}
    }
}
