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
#ifndef TESTTLS_H_
#define TESTTLS_H_
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

void csvtest(void);
void psktest(void);
void timetest(void);
void crltest(void);
extern CU_TestInfo ISM_Util_CUnit_TLS[];

#endif
