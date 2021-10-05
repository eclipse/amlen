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
 * Create a session related action. 
 */
public class ImaSessionAction extends ImaConnectionAction {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    ImaSession           session;
    
    /*
     * Create an action with a default buffer size
     */
    ImaSessionAction (int action, ImaSession session) {
        this(action, session, 256);
    }
    
    /*
     * Create an action with a specified buffer size.
     * Used for all session-related actions except for async ACK
     */
    ImaSessionAction (int action, ImaSession session, int size) {
        super(action, session.connect.client, size);
        this.session = session;
        outBB.put(ITEM_TYPE_POS, ItemTypes.Session.value());
        outBB.putInt(ITEM_POS, session.sessid);
        outBB.putLong(MSG_ID_POS, corrid);
    }

    /*
     * Reuse the action.
     * @see com.ibm.ima.jms.impl.ImaAction#reset(int)
     */
    void reset(int action) {
        super.reset(action);
        outBB.put(ITEM_TYPE_POS, ItemTypes.Session.value());
    }

}
