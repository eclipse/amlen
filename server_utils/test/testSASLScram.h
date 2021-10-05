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
 * File: testTimerCUnit.h
 * Component: server
 * SubComponent: server_utils
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */


#ifndef TESTSASLCRAM_H_
#define TESTSASLCRAM_H_

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

int cleanSASLScramSuite(void) ;
int initSASLScramSuite(void) ;
int initSASLScramTests(void) ;
extern CU_TestInfo ISM_Util_CUnit_SASLScram[];

void sasl_scram_generate_nonce_test(void);
void sasl_scram_build_client_first_message_test(void);
void sasl_scram_get_property_test (void);
void sasl_scram_Hi_test(void);
void sasl_scram_HMAC_test(void);
void sasl_scram_build_client_final_message_buffer_test(void);


#endif /* TESTMEMHANDLER_H_ */

