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

package com.ibm.ima.plugin.impl;

import com.ibm.ima.plugin.ImaConnection;
import com.ibm.ima.plugin.ImaTransaction;
import com.ibm.ima.plugin.ImaTransactionListener;

final class ImaTransactionImpl implements ImaTransaction {
    private final ImaConnectionImpl      connection;
    private final ImaChannel             channel;
    private final int                    connID;
    private final ImaTransactionListener listener;
    private boolean                      inUse = false;
    long                                 handle = 0;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    public ImaTransactionImpl(ImaConnectionImpl connection, ImaTransactionListener listener) {
        if (listener == null) {
            throw new RuntimeException("Transaction listener is null");
        }
        this.connection = connection;
        this.listener = listener;
        connID = connection.connectID;
        channel = connection.channel;
        ImaPluginAction action = new ImaPluginAction(ImaPluginAction.CreateTransaction, 128, 2);
        action.setObject(this);
        int seqnum = connection.setWork(action);
        ImaPluginUtils.putIntValue(action.bb, connID);
        ImaPluginUtils.putIntValue(action.bb, seqnum); /* Seqnum */
        action.send(channel);
    }

    /* (non-Javadoc)
     * @see com.ibm.ima.plugin.ImaTransaction#getConnection()
     */
    public ImaConnection getConnection() {
        return connection;
    }

    public boolean inUse() {
        return inUse;
    }

    public boolean isValid() {
        return (handle != 0);
    }

    /* (non-Javadoc)
     * @see com.ibm.ima.plugin.ImaTransaction#getTransactionListener()
     */
    public ImaTransactionListener getTransactionListener() {
        return listener;
    }

    void onComplete(int action, int rc, Object data) {
        String reason = null;
        inUse = false;
        if (rc == 0)
            this.handle = ((Long) data).longValue();
        else
            reason = (String) data;
        switch (action) {
        case ImaPluginAction.CommitTransaction:
            listener.onCommit(this, rc, reason);
            break;
        case ImaPluginAction.RollbackTransaction:
            listener.onRollback(this, rc, reason);
            break;
        case ImaPluginAction.CreateTransaction:
            listener.onCreate(this, rc, reason);
            break;
        }
    }

    private void invokeAction(int act) {
        if (inUse)
            throw new RuntimeException("Transaction is in use");
        if (handle == 0)
            throw new RuntimeException("Transaction handle is null");
        inUse = true;
        ImaPluginAction action = new ImaPluginAction(act, 128, 3);
        action.setObject(this);
        int seqnum = connection.setWork(action);
        ImaPluginUtils.putIntValue(action.bb, connID);
        ImaPluginUtils.putIntValue(action.bb, seqnum); /* Seqnum */
        ImaPluginUtils.putLongValue(action.bb, handle); /* TX handle */
        handle = 0;
        action.send(channel);
    }

    /*
     * (non-Javadoc)
     * 
     * @see com.ibm.ima.plugin.ImaTransaction#commit()
     */
    public void commit() {
        invokeAction(ImaPluginAction.CommitTransaction);
    }

    public void rollback() {
        invokeAction(ImaPluginAction.RollbackTransaction);
    }

}
