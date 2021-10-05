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

/*********************************************************************/
/*                                                                   */
/* Module Name: test_storeMQRecords.h                                */
/*                                                                   */
/* Description: CUnit tests of Engine MQ Record store functions      */
/*                                                                   */
/*********************************************************************/
#ifndef __TEST_STOREMQRECORDS_DEFINED
#define __TEST_STOREMQRECORDS_DEFINED

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
void storeMQRecordsTestQMgrRecord(void);
void storeMQRecordsTestQMXidRecord(void);
void storeMQRecordsTestTransactions(void);

extern CU_TestInfo ISM_Engine_CUnit_StoreMQRecords[];

#endif /* __TEST_STOREMQRECORDS_DEFINED */

/*********************************************************************/
/* End of test_storeMQRecords.h                                      */
/*********************************************************************/
