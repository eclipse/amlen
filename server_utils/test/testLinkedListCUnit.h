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
 * File: testLinkedListCUnit.h
 * Component: server
 * SubComponent: server_utils
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#ifndef TESTLINKEDLISTCUNIT_H_
#define TESTLINKEDLISTCUNIT_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

/**
 * Perform initialization for timer test suite.
 */
extern int initLinkedListCUnitSuite(void);

/**
 * Clean up after timer test suite is executed.
 */
extern int cleanLinkedListCUnitSuite(void);

/**   
 * Test array for timer test suite.
 */
extern CU_TestInfo ISM_Util_CUnit_LinkedList[];

#endif /* TESTTIMERCUNIT_H_ */
