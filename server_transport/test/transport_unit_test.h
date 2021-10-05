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

#ifndef TRANSPORT_UNIT_TEST_H_
#define TRANSPORT_UNIT_TEST_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

/**
 * Test transport instance creation.
 */
void testNewTransport(void);

/**
 * Test protocol registration (ism_transport_registerProtocol
 * and ism_transport_findProtocol).
 */
void testProtocolRegistration(void);

/**
  * Test framer registration (ism_transport_registerFramer
  * and ism_transport_findFramer).
  */
void testFramerRegistration(void);

/**
 * Test ism_transport_allocBytes function.
 */
void testAllocBytes(void);

/**
 * Test ism_transport_allocBytes function.
 */
void testCloseConnection(void);

/**
 * Test monitor functions (add, remove, get, free).
 */
void testMonitors(void);

/**
 * Test array for transport test suite.
 */
extern CU_TestInfo ISM_transport_tests[];

#endif /* TRANSPORT_UNIT_TEST_H_ */
