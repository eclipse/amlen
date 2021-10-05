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
 * File: testHashMapCUnit.h
 * Component: server
 * SubComponent: server_utils
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#ifndef TESTTIMEUTILSCUNIT_H_
#define TESTTIMEUTILSCUNIT_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

/**
 * Perform initialization for time utilities test suite.
 */
extern int initTimeUtilsCUnitSuite(void);

/**
 * Clean up after time utilities suite is executed.
 */
extern int cleanTimeUtilsCUnitSuite(void);

/**   
 * Test array for time utilities test suite.
 */
extern CU_TestInfo ISM_Util_CUnit_TimeUtils[];

#endif /* TESTTIMERCUNIT_H_ */
