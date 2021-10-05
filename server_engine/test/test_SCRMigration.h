/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
/* Module Name: test_SCRMigration.h                                  */
/*                                                                   */
/* Description: CUnit tests of Engine SCR migration                  */
/*                                                                   */
/*********************************************************************/
#ifndef __TEST_SCRMIGRATION_DEFINED
#define __TEST_SCRMIGRATION_DEFINED

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
void SCRMigration(void);

extern CU_TestInfo ISM_Engine_CUnit_SCRMigration[];

#endif /* __TEST_SCRMIGRATION_DEFINED */

/*********************************************************************/
/* End of test_SCRMigration.h                                        */
/*********************************************************************/
