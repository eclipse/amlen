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
/* Module Name: test_clientState.h                                   */
/*                                                                   */
/* Description: CUnit tests of Engine client-state functions         */
/*                                                                   */
/*********************************************************************/
#ifndef __TEST_CLIENTSTATE_DEFINED
#define __TEST_CLIENTSTATE_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include <CUnit/CUnit.h>
#include "engineCommon.h"     /* Engine common internal header file  */
#include "engineInternal.h"   /* Engine internal header file         */

#include "clientState.h" /* for iecs_clientStateConfigCallback */

/*********************************************************************/
/*                                                                   */
/* FUNCTION PROTOTYPES                                               */
/*                                                                   */
/*********************************************************************/
void clientStateTestAnonymous(void);
void clientStateDestroyContext(void);
void clientStateTestSimpleBounce(void);
void clientStateTestSimpleSteal(void);
void clientStateTestStealRemoveZombie(void);
void clientStateTestNondurableZombieToDurable(void);
void clientStateTestStealDuringDelivery(void);
void clientStateTestStealCleanStart(void);
void clientStateTestDoubleStealLazy(void);
void clientStateTestDoubleStealEager(void);
void clientStateTestZombieCleanStart(void);
void clientStateTestInheritDurability(void);
void clientStateTestUnreleased(void);
void clientStateTestSimpleDurable(void);
void clientStateTestCleanSession0(void);
void clientStateTestCleanSession1(void);
void clientStateTestDurableToDurable(void);
void clientStateTestNondurableToDurable(void);
void clientStateTestDurableToNondurable(void);
void clientStateTestMassive(void);
void clientStateTestDurableTran(void);
void clientStateTestStealDurableSubs(void);
void clientStateTestProtocolMismatch(void);
void clientStateTestUnusualSteals(void);
void clientStateTestForceDiscard(void);
void clientStateTestClientStateTableTraversal(void);
void clientStateTestBasicExpiry(void);
void clientStateTestTableResizeDuringRecovery(void);

extern CU_TestInfo ISM_Engine_CUnit_ClientStateBasic[];
extern CU_TestInfo ISM_Engine_CUnit_ClientStateLowIDRange[];
extern CU_TestInfo ISM_Engine_CUnit_ClientStateHardToReach[];
extern CU_TestInfo ISM_Engine_CUnit_ClientStateSecurity[];

#endif /* __TEST_CLIENTSTATE_DEFINED */

/*********************************************************************/
/* End of test_clientState.h                                         */
/*********************************************************************/
