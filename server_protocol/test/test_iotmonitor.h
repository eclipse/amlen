/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
#ifndef TEST_IOTMONITOR_H_
#define TEST_IOTMONITOR_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <protocol_test_globals.h>

void testiotmonitormsg(void);
void testiotmonitorAPIs(void);
void testiotmonitorReconcile(void);

extern CU_TestInfo ISM_Protocol_CUnit_IoTMonitorBasic[];
extern CU_TestInfo ISM_Protocol_CUnit_IoTMonitorFull[];

#endif /* TEST_IOTMONITOR_H_ */
