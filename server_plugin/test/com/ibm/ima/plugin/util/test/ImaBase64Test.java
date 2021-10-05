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

package com.ibm.ima.plugin.util.test;

import com.ibm.ima.plugin.util.ImaBase64;

import junit.framework.TestCase;


public class ImaBase64Test extends TestCase {
    public void testToBase64() throws Exception {
    	String fromStr = "Base64Test";
    	String resultStr = ImaBase64.toBase64(fromStr.getBytes());
    	System.out.println("The result is "+resultStr);
        assertEquals(resultStr, "QmFzZTY0VGVzdA==");
    }    
}
