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
 * The ImaSubscription object is created using the newSubscription() method of ImaConnection.
 * The configuration of the subscription is read only, and the only actions which can be taken 
 * on the subscription are subscribe() and close().  A subscription object is tied to a 
 * connection.   
 */
public interface ImaTransaction {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

    
    /**
     * Get the connection associated with a transaction.
     * 
     * @return The connection object
     */
    public ImaConnection getConnection();
    
    /**
     * Get the transaction listerner associated with a transaction.
     * @return The transaction listener object
     */
    public ImaTransactionListener getTransactionListener();

    /**
     * Commit the transaction.
     * 
     * All messages sent from this transaction are made available to receivers.
     */
    public void commit();

    /**
     * Rollback the transaction.
     * 
     * All changed made from this transaction are undone.
     */
    public void rollback();

    /*
     * Check if the transaction is currently being processed.
     * @return true if a commit or rollback is active in the transaction.
     */
    public boolean inUse();

    /**
     * Check if the transaction is fully created.
     * @return ture if the transaction is completely created.
     */
    public boolean isValid();
}
