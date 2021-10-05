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
/* Module Name: test_willMessage.h                                   */
/*                                                                   */
/* Description: CUnit tests of Engine will message functions         */
/*                                                                   */
/*********************************************************************/
#ifndef __TEST_WILLMESSAGE_DEFINED
#define __TEST_WILLMESSAGE_DEFINED

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
void willMessageTestNoSubscribers(void);
void willMessageTestOneSubscriber(void);
void willMessageTestUnset(void);
void willMessageTestNondurableNonpersistent(void);
void willMessageTestNondurablePersistent(void);
void willMessageTestDurableNonpersistent(void);
void willMessageTestDurablePersistent(void);
void willMessageTestDurableDiscard(void);
void willMessageTestDurableSimpQDestFull(void);
void willMessageTestDurableIntermediateDestFull(void);
void willMessageTestDurableMultiConsumerQDestFull(void);
void willMessageTestInvalidTopic(void);
void willMessageTestClearRetained(void);
void willMessageTestSecurity(void);
void willMessageMaxMessageTimeToLive(void);
void willMessageTestRequestInProgress(void);
void willMessageTestDelay(void);
void willMessageTestDelayTakeOver(void);

extern CU_TestInfo ISM_Engine_CUnit_WillMessageBasic[];
extern CU_TestInfo ISM_Engine_CUnit_WillMessageSecurity[];
extern CU_TestInfo ISM_Engine_CUnit_WillDelay[];

#endif /* __TEST_WILLMESSAGE_DEFINED */

/*********************************************************************/
/* End of test_willMessage.h                                         */
/*********************************************************************/
