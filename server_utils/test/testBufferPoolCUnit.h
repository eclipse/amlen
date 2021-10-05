/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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


/*
 * File: testBufferPoolCUnit.h
 * Component: server
 * SubComponent: server_utils
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#ifndef TESTBUFFERPOOLCUNIT_H_
#define TESTBUFFERPOOLCUNIT_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

/**
 * Perform initialization for buffer pool test suite.
 */
extern int initBufferPoolCUnitSuite(void);

/**
 * Clean up after buffer pool test suite is executed.
 */
extern int cleanBufferPoolCUnitSuite(void);

/**   
 * List of BufferPool tests in the suite.
 */
extern CU_TestInfo ISM_Util_CUnit_BufferPool[];

#endif /* TESTBUFFERPOOLCUNIT_H_ */
