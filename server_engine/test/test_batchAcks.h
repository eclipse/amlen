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
/* Module Name: test_batchAcks.h                                     */
/*                                                                   */
/* Description: CUnit tests of Engine batched acknowledgements       */
/*                                                                   */
/*********************************************************************/
#ifndef __TEST_BATCHACKS_DEFINED
#define __TEST_BATCHACKS_DEFINED

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
void batchAcksTestSessionRecoverPubSub(void);

extern CU_TestInfo ISM_Engine_CUnit_BatchAcks[];

#endif /* __TEST_BATCHACKS_DEFINED */

/*********************************************************************/
/* End of test_batchAcks.h                                           */
/*********************************************************************/
