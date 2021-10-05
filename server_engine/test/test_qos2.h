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
/* Module Name: test_qos2.h                                          */
/*                                                                   */
/* Description: CUnit tests of Engine MQTT QoS2-style functions      */
/*                                                                   */
/*********************************************************************/
#ifndef __TEST_QOS2_DEFINED
#define __TEST_QOS2_DEFINED

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
void qos2TestReconnect(void);
void qos2TestUnreleased(void);
void qos2TestNoSubs(void);

extern CU_TestInfo ISM_Engine_CUnit_QoS2Basic[];
extern CU_TestInfo ISM_Engine_CUnit_QoS2LowIDRange[];

#endif /* __TEST_QOS2_DEFINED */

/*********************************************************************/
/* End of test_qos2.h                                                */
/*********************************************************************/
