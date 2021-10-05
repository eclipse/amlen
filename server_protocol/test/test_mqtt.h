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
#ifndef TEST_MQTT_H_
#define TEST_MQTT_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <protocol_test_globals.h>

/*
void testGetJmsActionName(void);
void testGetProperty(void);
void testJmsReceive(void);
*/
void getmqtterror(void);
void getmqttactionname(void);
void testmqttconnect(void);
void testmqttpublish(void);
void testmqttsubscribe(void);
void testmqttunsubscribe(void);
void testmqtttype(void);

void testmqtt4(void);
void testcheckstring(void);
void testparsetopic(void);

/**
 * Test array for API tests.
 */
/*
extern CU_TestInfo ISM_Protocol_CUnit_JmsBasic[];
*/
extern CU_TestInfo ISM_Protocol_CUnit_MqttBasic[];
extern CU_TestInfo ISM_Protocol_CUnit_MqttFull[];


#endif /* TEST_MQTT_H_ */
