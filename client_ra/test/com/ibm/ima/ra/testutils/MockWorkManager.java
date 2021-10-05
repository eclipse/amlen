/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.ra.testutils;

import javax.resource.spi.work.ExecutionContext;
import javax.resource.spi.work.Work;
import javax.resource.spi.work.WorkException;
import javax.resource.spi.work.WorkListener;
import javax.resource.spi.work.WorkManager;

public class MockWorkManager implements WorkManager {

    @Override
    public void doWork(Work arg0) throws WorkException {
        // TODO Auto-generated method stub

    }

    @Override
    public void doWork(Work arg0, long arg1, ExecutionContext arg2,
            WorkListener arg3) throws WorkException {
        // TODO Auto-generated method stub

    }

    @Override
    public void scheduleWork(Work arg0) throws WorkException {
        // TODO Auto-generated method stub

    }

    @Override
    public void scheduleWork(Work arg0, long arg1, ExecutionContext arg2,
            WorkListener arg3) throws WorkException {
        // TODO Auto-generated method stub

    }

    @Override
    public long startWork(Work arg0) throws WorkException {
        // TODO Auto-generated method stub
        return 0;
    }

    @Override
    public long startWork(Work arg0, long arg1, ExecutionContext arg2,
            WorkListener arg3) throws WorkException {
        // TODO Auto-generated method stub
        return 0;
    }

}
