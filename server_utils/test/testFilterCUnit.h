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
 * File: testFilterCUnit.h
 * Component: server
 * SubComponent: server_utils
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#ifndef TESTFILTERCUNIT_H_
#define TESTFILTERCUNIT_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <selector.h>

XAPI int ism_common_tryReadLockACLList( void );
XAPI int ism_common_tryWriteLockACLList( void );
XAPI void ism_common_unlockACLList( void );
XAPI void ism_protocol_lockACLList(bool write, ismMessageSelectionLockStrategy_t * lockStrategy);
XAPI void ism_protocol_unlockACLList(ismMessageSelectionLockStrategy_t * lockStrategy);

/**
 * Perform initialization for timer test suite.
 */
extern int initFilterCUnitSuite(void);

/**
 * Clean up after timer test suite is executed.
 */
extern int cleanFilterCUnitSuite(void);

/**   
 * Test array for timer test suite.
 */
extern CU_TestInfo ISM_Util_CUnit_Filter[];

#endif /* TESTFILTERCUNIT_H_ */
