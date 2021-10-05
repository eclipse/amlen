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

import javax.transaction.xa.Xid;
import junit.framework.TestCase;
//import org.junit.Assert;

public class UtilsTest extends TestCase {
    
    public void testXid() throws Exception {
        byte [] b = new byte[1024];
        ByteBuffer bb = ByteBuffer.wrap(b);
        ImaTrace.init();
    
        Xid xid =(Xid) new ImaXidImpl(0x1234, "abc".getBytes(), "defj".getBytes());
        bb = ImaUtils.putXidValue(bb, xid);
        bb.flip();
        Object obj = ImaUtils.getObjectValue(bb);
        assertTrue(obj instanceof Xid);
        assertTrue(obj instanceof ImaXidImpl);
        Xid xid2 = (Xid)obj;
        assertEquals(xid.getFormatId(), xid2.getFormatId());
        System.out.println("xid2=" + xid2);
        //Assert.assertArrayEquals(xid.getBranchQualifier(), xid2.getBranchQualifier());
        //Assert.assertArrayEquals(xid.getGlobalTransactionId(), xid2.getGlobalTransactionId());
    }    
}
