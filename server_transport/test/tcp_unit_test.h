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
#ifndef TCP_UNIT_TEST_H_
#define TCP_UNIT_TEST_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

void testMqttFraming(void);
void testHandshake(void);
void testSendReceiveBytes(void);
void testSendReceiveBytesMulti(void);
void testStartStop(void);
void testSaveArea(void);
void testMultiplePorts(void);

/**
 * Test array for simple tcp tests.
 */
extern CU_TestInfo ISM_tcp_tests[];

/**
 * Test array for client-server tcp tests
 */
extern CU_TestInfo ISM_client_server_tcp_tests[];

#endif /* TCP_UNIT_TEST_H_ */
