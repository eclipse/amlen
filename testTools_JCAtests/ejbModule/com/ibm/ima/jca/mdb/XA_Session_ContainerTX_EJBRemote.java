package com.ibm.ima.jca.mdb;

import com.ibm.ima.jms.test.jca.TestProps;

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

public interface XA_Session_ContainerTX_EJBRemote {

    public TestProps sendMessage_REQUIRED(TestProps test);

    public TestProps sendMessage_REQUIRES_NEW(TestProps test);

    public TestProps sendMessage_SUPPORTS(TestProps test);

    public TestProps sendMessage_NOT_SUPPORTED(TestProps test);

    public TestProps sendMessage_MANDATORY(TestProps test);

    public TestProps sendMessage_NEVER(TestProps test);

    public void setTest(TestProps test);

    public void queryDB();
    
    public void doEvil();
}
