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

#ifndef TESTLOGGINGCUNIT_H_
#define TESTLOGGINGCUNIT_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>



/**
 * Perform initialization for timer test cases.
 */
int initLoggingTests(void);

int cleanLoggingTests(void);

void CUnit_test_ISM_Logging_init(void);
void CUnit_test_ISM_Logging_getLogLevel(void) ;
void CUnit_test_ISM_Logging_getTraceLevelforLog(void) ;
void CUnit_test_ISM_Logging_getErrorString(void);
void CUnit_test_ISM_Logging_setgetError(void);
void CUnit_test_ISM_Logging_getErrorStringByLocale(void);
void CUnit_test_ISM_Logging_formatLastError(void);
void CUnit_test_ISM_Logging_getErrorStringByLocale_truncate(void);
void CUnit_test_ISM_Logging_ism_log_conditionallyLogged(void);
void CUnit_test_ISM_Logging_ism_log_getLoggedCount(void);

/**
 * Test array for timer test suite.
 */
extern CU_TestInfo ISM_Util_CUnit_logging[];


#endif /* TESTTIMERCUNIT_H_ */
