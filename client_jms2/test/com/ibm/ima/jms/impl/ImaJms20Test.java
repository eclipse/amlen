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

import javax.jms.Connection;

import junit.framework.TestCase;


public class ImaJms20Test extends TestCase {
    public void testJMS11() throws Exception {
        Connection connect = new ImaConnection(true);
        assertEquals("2.0", connect.getMetaData().getJMSVersion());
    }    
}
