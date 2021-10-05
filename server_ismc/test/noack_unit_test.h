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

#ifndef NOACK_UNIT_TEST_H_
#define NOACK_UNIT_TEST_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

void testTopicNoAck(void);
void testQueueNoAck(void);
void testTopicSession(void);

/**
 * Test array for destination tests without acknowledgment (noack).
 */
extern CU_TestInfo ISM_noack_tests[];

#endif /* NOACK_UNIT_TEST_H_ */
