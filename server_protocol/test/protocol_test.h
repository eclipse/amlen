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

/*
 * File: protocol_test.h
 * Component: server
 * SubComponent: server_protocol
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <security.h>
#include <protocol.h>
#include <test_mqtt.h>
#include <test_jms.h>
#include <test_iotmonitor.h>
extern CU_TestInfo ISM_Protocol_CUnit_Xid[];
extern CU_TestInfo ISM_Protocol_CUnit_Props[];
extern CU_TestInfo ISM_Protocol_CUnit_Msgid[];
extern CU_TestInfo ISM_Protocol_CUnit_Fwd[];
extern CU_TestInfo ISM_Protocol_CUnit_FwdConnect[];

#define BASIC_TEST_MODE       0
#define FULL_TEST_MODE        1
#define BYNAME_TEST_MODE      2

extern int g_verbose;

int test_mode = BASIC_TEST_MODE;
int default_test_mode = BASIC_TEST_MODE;

int init_fakeengine(void);

/*
 * Array that carries the test suite and other functions to the CUnit framework
 */
CU_SuiteInfo ISM_Protocol_CUnit_basicsuites[] = {
    IMA_TEST_SUITE("Xid",     NULL,            NULL, ISM_Protocol_CUnit_Xid),
    IMA_TEST_SUITE("Props",   NULL,            NULL, ISM_Protocol_CUnit_Props),
    IMA_TEST_SUITE("MQTT",    init_fakeengine, NULL, ISM_Protocol_CUnit_MqttBasic),
    IMA_TEST_SUITE("Msgid",   init_fakeengine, NULL, ISM_Protocol_CUnit_Msgid),
    IMA_TEST_SUITE("Jms",     init_fakeengine, NULL, ISM_Protocol_CUnit_JmsBasic),
    IMA_TEST_SUITE("Fwd",     init_fakeengine, NULL, ISM_Protocol_CUnit_Fwd),
    IMA_TEST_SUITE("Monitor", init_fakeengine, NULL, ISM_Protocol_CUnit_IoTMonitorBasic),
    CU_SUITE_INFO_NULL,
};


/*
 * Array that carries the test suite and other functions to the CUnit framework
 */
CU_SuiteInfo ISM_Protocol_CUnit_allsuites[] = {
    IMA_TEST_SUITE("MQTT",     init_fakeengine, NULL, ISM_Protocol_CUnit_MqttFull),
    IMA_TEST_SUITE("Props",    NULL,            NULL, ISM_Protocol_CUnit_Props),
    IMA_TEST_SUITE("Msgid",    init_fakeengine, NULL, ISM_Protocol_CUnit_Msgid),
    IMA_TEST_SUITE("Jms",      init_fakeengine, NULL, ISM_Protocol_CUnit_JmsBasic),
    IMA_TEST_SUITE("Xid",      NULL,            NULL, ISM_Protocol_CUnit_Xid),
    IMA_TEST_SUITE("Fwd",      init_fakeengine, NULL, ISM_Protocol_CUnit_Fwd),
    IMA_TEST_SUITE("Monitor",  init_fakeengine, NULL, ISM_Protocol_CUnit_IoTMonitorFull),
    CU_SUITE_INFO_NULL,
};


