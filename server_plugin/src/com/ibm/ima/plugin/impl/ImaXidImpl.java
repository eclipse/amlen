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

package com.ibm.ima.plugin.impl;

import javax.transaction.xa.Xid;

final public class ImaXidImpl implements Xid {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final int    formatId;
    private final byte[] branchQualifier;
    private final byte[] globalTransactionId;

    public ImaXidImpl(int formatId,  byte[] globalTransactionId, byte[] branchQualifier) {
        this.formatId = formatId;
        this.branchQualifier = branchQualifier;
        this.globalTransactionId = globalTransactionId;
    }

    /* 
     * @see javax.transaction.xa.Xid#getBranchQualifier()
     */
    public byte[] getBranchQualifier() {
        return branchQualifier;
    }

    /* 
     * @see javax.transaction.xa.Xid#getFormatId()
     */
    public int getFormatId() {
        return formatId;
    }

    /* 
     * @see javax.transaction.xa.Xid#getGlobalTransactionId()
     */
    public byte[] getGlobalTransactionId() {
        return globalTransactionId;
    }

    static final String myhex = "0123456789ABCDEF";
    
    /*
     * @see java.lang.Object#toString()
     */
    static String toString(Xid xid) {
        char [] out = new char[278];
        int   outp = 0;
        int    i;
        byte[] branchQualifier = xid.getBranchQualifier();
        byte[] globalTransactionId = xid.getGlobalTransactionId();
        if (branchQualifier == null || branchQualifier.length > 64 || 
            globalTransactionId == null || globalTransactionId.length > 64)
            return "Xid@" + xid.hashCode();
        out[outp++] = ':';
        for (i=0; i<globalTransactionId.length; i++) {
            out[outp++] = myhex.charAt((globalTransactionId[i]>>4)&0xf);
            out[outp++] = myhex.charAt(globalTransactionId[i]&0xf);
        }
        out[outp++] = ':';
        for (i=0; i<branchQualifier.length; i++) {
            out[outp++] = myhex.charAt((branchQualifier[i]>>4)&0xf);
            out[outp++] = myhex.charAt(branchQualifier[i]&0xf);
        }

        return Integer.toHexString(xid.getFormatId()).toUpperCase() + new String(out, 0, outp);
    }

    /*
     * @see java.lang.Object#toString()
     */
    public String toString() {
        return toString(this);
    }
}
