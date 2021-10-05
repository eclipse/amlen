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
 * File: testTimerCUnit.h
 * Component: server
 * SubComponent: server_utils
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#ifndef TESTTIMERCUNIT_H_
#define TESTTIMERCUNIT_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

/**
 * Perform initialization for timer test cases.
 */
int initTimerTests(void);

/**
 * Perform initialization for timer test suite.
 */
int initTimerUtilSuite(void);

/**
 * Clean up after timer test cases were executed.
 */
int cleanTimerTests(void);

/**
 * Clean up after timer test suite is executed.
 */
int cleanTimerUtilSuite(void);

/**
 * Test ism_common_setTimerOnce function.
 */
void CUnit_test_ISM_timer_once(void);

/**
 * Test ism_common_cancelTimer function with one-off events.
 */
void CUnit_test_ISM_timer_once_cancel(void);

/**
 * Test single event scheduled with ism_common_setTimerRate function.
 */
void CUnit_test_ISM_timer_repeat_single(void);

/**
 * Test multiple simultaneous repeating events scheduled
 * with ism_common_setTimerRate function.
 */
void CUnit_test_ISM_timer_repeat_multi(void);

/**   
 * Test array for timer test suite.
 */
extern CU_TestInfo ISM_Util_CUnit_timer[];

#endif /* TESTTIMERCUNIT_H_ */
