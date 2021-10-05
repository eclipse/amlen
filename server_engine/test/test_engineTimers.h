/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

/*********************************************************************/
/*                                                                   */
/* Module Name: test_engineTimers.h                                  */
/*                                                                   */
/* Description: CUnit tests of engine timer functions                */
/*                                                                   */
/*********************************************************************/
#ifndef __TEST_ENGINETIMERS_DEFINED
#define __TEST_ENGINETIMERS_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include <CUnit/CUnit.h>
#include "engineCommon.h"     /* Engine common internal header file  */
#include "engineInternal.h"   /* Engine internal header file         */

/*********************************************************************/
/*                                                                   */
/* FUNCTION PROTOTYPES                                               */
/*                                                                   */
/*********************************************************************/
void test_timerStartup(void);
void test_directCall(void);

extern CU_TestInfo ISM_Engine_CUnit_Timers[];

#endif /* __TEST_ENGINETIMERS_DEFINED */

/*********************************************************************/
/* End of test_engineTimers.h                                        */
/*********************************************************************/
