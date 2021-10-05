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
/* Module Name: test_qos1.h                                          */
/*                                                                   */
/* Description: CUnit tests of Engine MQTT QoS1-style functions      */
/*                                                                   */
/*********************************************************************/
#ifndef __TEST_QOS1_DEFINED
#define __TEST_QOS1_DEFINED

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
void qos1TestNoSubscriberDelivery(void);
void qos1TestSingleDelivery(void);
void qos1TestMultipleDelivery(void);
void qos1TestDeliveryStopDelivery(void);
void qos1TestDeliveryDestroyConsumer(void);
void qos1TestDeliveryDestroySession(void);
void qos1TestDeliveryDestroyClient(void);
void qos1TestDeliveryStopDeliveryRetcode(void);
void qos1TestDeliveryStopDeliveryRetcodeThenStart(void);
void qos1TestDeliveryStopDeliveryCallback(void);
void qos1TestDeliveryDestroyConsumerCallback(void);
void qos1TestDeliveryDestroySessionCallback(void);
void qos1TestDeliveryDestroyClientCallback(void);
void qos1TestDeliveryLateStart(void);

extern CU_TestInfo ISM_Engine_CUnit_QoS1Basic[];

#endif /* __TEST_QOS1_DEFINED */

/*********************************************************************/
/* End of test_qos1.h                                                */
/*********************************************************************/
