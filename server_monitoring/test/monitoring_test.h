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
 * File: config_test.h
 * Component: server
 * SubComponent: server_admin
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>


/*
#include "monitApi_test.c"
#include "serialization_test.c"
#include "connectionmon_test.c"
#include "monitoringutil_test.c"
#include "monitoringevent_test.c"
*/

#include "inactiveClusterMember_remove_test.c"

#define BASIC_TEST_MODE       0
#define FULL_TEST_MODE        1
#define BYNAME_TEST_MODE      2

extern int g_verbose;

int test_mode = BASIC_TEST_MODE;
int default_test_mode = BASIC_TEST_MODE;

int init_monitoring(void);

int init_adminconfig(void);

/*
 *
    {"MonitAPI", testMonitoringAPI },
    {"Serialization", testSerialziation },
    {"Connection Monitoring", testConnectionMon },
    {"MonitoringUtil Monitoring", testMonitoringUtil },
    {"MonitoringEvent Monitoring", testMonitoringEvent },
 */

CU_TestInfo ISM_Monitoring_CUnit_MonitBasic[] = {
    {"test_inactiveCMRemove", test_inactiveCMRemove },
    CU_TEST_INFO_NULL
};

/*
 * Array that carries the test suite and other functions to the CUnit framework
 */

CU_SuiteInfo ISM_Monitoring_CUnit_basicsuites[] = {
    IMA_TEST_SUITE("MonitBasic", init_monitoring, NULL, ISM_Monitoring_CUnit_MonitBasic),
    CU_SUITE_INFO_NULL,
};


/*
 * Array that carries the test suite and other functions to the CUnit framework
 */

CU_SuiteInfo ISM_Monitoring_CUnit_allsuites[] = {
    IMA_TEST_SUITE("MonitBasic", init_monitoring, NULL, ISM_Monitoring_CUnit_MonitBasic),
    CU_SUITE_INFO_NULL,
};

