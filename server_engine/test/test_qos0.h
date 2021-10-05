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
/* Module Name: test_qos0.h                                          */
/*                                                                   */
/* Description: CUnit tests of Engine MQTT QoS0-style functions      */
/*                                                                   */
/*********************************************************************/
#ifndef __TEST_QOS0_DEFINED
#define __TEST_QOS0_DEFINED

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
void qos0TestNoSubscriberDelivery(void);
void qos0TestSingleDelivery(void);
void qos0TestMultipleDelivery(void);
void qos0TestDeliveryStopDelivery(void);
void qos0TestDeliveryDestroyConsumer(void);
void qos0TestDeliveryResumeConsumer(void);
void qos0TestDeliveryDestroySession(void);
void qos0TestDeliveryDestroyClient(void);
void qos0TestDeliveryStopDeliveryRetcode(void);
void qos0TestDeliveryStopDeliveryRetcodeThenStart(void);
void qos0TestDeliveryStopDeliveryCallback(void);
void qos0TestDeliveryDestroyConsumerCallback(void);
void qos0TestDeliveryDestroySessionCallback(void);
void qos0TestDeliveryDestroyClientCallback(void);
void qos0TestDeliveryLateStart(void);
void qos0TestDuplicateConsumer(void);
extern CU_TestInfo ISM_Engine_CUnit_QoS0Basic[];

/*And the duplicate version of the above that use the explicit suspend functionality...*/
void qos0TestExplicitSuspendDestroyConsumer(void);
void qos0TestExplicitSuspendResumeConsumer(void);
void qos0TestExplicitSuspendDestroySession(void);
void qos0TestExplicitSuspendDestroyClient(void);
void qos0TestExplicitSuspendStopDeliveryRetcode(void);
void qos0TestExplicitSuspendStopDeliveryRetcodeThenStart(void);
void qos0TestExplicitSuspendStopDeliveryCallback(void);
void qos0TestExplicitSuspendDestroyConsumerCallback(void);
void qos0TestExplicitSuspendDestroySessionCallback(void);
void qos0TestExplicitSuspendDestroyClientCallback(void);
void qos0TestExplicitSuspendLateStart(void);
extern CU_TestInfo ISM_Engine_CUnit_QoS0ExplicitSuspend[];

#endif /* __TEST_QOS0_DEFINED */

/*********************************************************************/
/* End of test_qos0.h                                                */
/*********************************************************************/
