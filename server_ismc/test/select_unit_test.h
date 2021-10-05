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
#ifndef SELECT_UNIT_TEST_H_
#define SELECT_UNIT_TEST_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

/*
 Array that carries all selector tests for APIs to CUnit framework.
 */
extern CU_TestInfo ISM_select_tests[];

void test_const(void);
void test_expression(void);
void test_bad_compile(void);
void test_selection(void);

#endif /* SELECT_UNIT_TEST_H_ */
