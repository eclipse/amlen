/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
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
/*-----------------------------------------------------------------
 * System Name : MQ Low Latency Messaging
 * File        : TestAction.java
 * Author      :  
 *-----------------------------------------------------------------
 */

package com.ibm.ima.jms.test;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

/**
 * 
 * Temporary class
 * 
 */
public final class TestAction extends Action {

    TestAction(Element element, DataRepository testData, TrWriter trWriter) {
        super(element, testData, trWriter);
    }

    protected boolean run() {
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000",
                "Action {0} executed", this.toString());
        return true;
    }

}
