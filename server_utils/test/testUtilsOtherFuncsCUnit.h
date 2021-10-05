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


#ifndef TESTOTHERFUNCTIONS_H_
#define TESTOTHERFUNCTIONS_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

int cleanOtherFunctionsUtilSuite(void) ;
int initOtherFunctionsUtilSuite(void) ;
int initOtherFunctionsTests(void) ;
extern CU_TestInfo ISM_Util_CUnit_otherFunctions[];


void CUnit_test_ISM_Tracing_Base64(void);
void CUnit_test_content(void);
void CUnit_test_ISM_ifmapip(void);
void CUnit_test_ISM_ifmapname(void);
void CUnit_test_ISM_ifmapnameIPv6(void);
void CUnit_test_ISM_ifmapipIPv6(void);


#endif /* TESTOTHERFUNCTIONS_H_ */

