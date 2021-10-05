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
 
#ifndef TESTTRACINGCUNIT_H_
#define TESTTRACINGCUNIT_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
/**
 * Perform initialization for timer test cases.
 */
int initTracingTests(void);

int cleanTracingTests(void);

void CUnit_test_ISM_Tracing_setgetTraceComponentLevel(void);
void CUnit_test_ISM_Logging_TraceMacro(void) ;
void CUnit_test_ISM_Logging_TraceMacroWithReplacements(void) ;
void CUnit_test_ISM_Logging_TraceMacroWithComponentLevels(void);
/**   
 * Test array for timer test suite.
 */
extern CU_TestInfo ISM_Util_CUnit_tracing[];


#endif /* TESTTIMERCUNIT_H_ */

