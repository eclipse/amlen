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
 * The transaction listener defines the callbacks associated with a transaction. <br>
 * The object implementing this interface is owned by the plug-in.
 */
public interface ImaTransactionListener {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    /**
     * Called when the transaction is created.
     * 
     * The transaction can be used now.
     * 
     * @param transaction
     * @param rc A return code 0=normal
     * @param reason A human readable reason code
     */
    public void onCreate(ImaTransaction transaction, int rc, String reason);
    
    /**
     * Called when the transaction is committed.
     * 
     * The transaction can be reused after that.
     * 
     * @param rc A return code 0=normal
     * @param reason A human readable reason code
     */
    public void onCommit(ImaTransaction transaction, int rc, String reason);

    /**
     * Called when the transaction is rolled back.
     * 
     * The transaction can be reused after that.
     * 
     * @param rc A return code 0=normal
     * @param reason A human readable reason code
     */
    public void onRollback(ImaTransaction transaction, int rc, String reason);
    
}
