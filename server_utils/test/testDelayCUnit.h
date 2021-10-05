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

#ifndef TESTDELAYCUNIT_H_
#define TESTDELAYCUNIT_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>



/**
 * Perform initialization for timer test cases.
 */
int initDelayTests(void);

int cleanDelayTests(void);

void CUnit_test_ism_throttle_parseThrottleConfiguration(void);

void CUnit_test_ism_throttle_getDelayTime(void);

void CUnit_test_ism_throttle_getDelayTimeNanos(void);

void CUnit_test_ism_throttle_addAuthFailedClientEntryCount(void);

void CUnit_test_ism_throttle_setConnectReqInQ (void);

void CUnit_test_ism_throttle_incrementClienIDStealCount(void);

void CUnit_test_ism_throttle_incrementConnCloseError(void);


/**
 * Test array for timer test suite.
 */
extern CU_TestInfo ISM_Throttle_CUnit_Delay[];


#endif /* TESTTIMERCUNIT_H_ */
