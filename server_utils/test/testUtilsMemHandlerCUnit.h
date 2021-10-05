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


#ifndef TESTMEMHANDLER_H_
#define TESTMEMHANDLER_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

int cleanMemHandlerSuite(void) ;
int initMemHandlerSuite(void) ;
int initMemHandlerTests(void) ;
extern CU_TestInfo ISM_Util_CUnit_memHandler[];

void CUnit_test_MemHandler_basic(void);
void CUnit_test_MemHandler_checking(void);
void CUnit_test_MemHandler_realloc(void);
void CUnit_test_MemHandler_transfer(void);


#endif /* TESTMEMHANDLER_H_ */

