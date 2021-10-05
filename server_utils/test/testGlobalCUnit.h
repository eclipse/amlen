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
 * File: testGlobalCUnit.h
 * Component: server
 * SubComponent: server_utils
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#ifndef TESTGLOBALCUNIT_H_
#define TESTGLOBALCUNIT_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

/*Initialize the Globalization Test Suite*/
int initGlobalUtilSuite(void);

/*Clean the Globalization Test Suite*/
int cleanGlobalUtilSuite(void);

/*Test Request Locale functions*/
void CUnit_test_setgetRequestLocale(void);

void testCRC(void);

/*Globalization Test Suite Array*/
extern CU_TestInfo ISM_Util_CUnit_global[3];



#endif /* TESTGLOBALCUNIT_H_ */
