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


/*
 *  
 */
public class ImaConsumerAction extends ImaSessionAction {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    int consumerID;
    /*
     * Create an action.
     */
    ImaConsumerAction (int action, ImaSession session, int consumerID) {
        this(action, session, consumerID, null ,256);
    }
    ImaConsumerAction (int action, ImaSession session, int consumerID, byte [] buffer, int size) {
        super(action,session,size);
        outBB.put(ITEM_TYPE_POS, ItemTypes.Consumer.value());
        outBB.putInt(ITEM_POS, consumerID);
        this.consumerID = consumerID;
    }
    void reset(int action) {
        super.reset(action);
        outBB.put(ITEM_TYPE_POS, ItemTypes.Consumer.value());
        outBB.putInt(ITEM_POS, consumerID);
    }
}
