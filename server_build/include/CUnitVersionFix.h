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

/* CUNIT v2.1.3 changed the CU_SuiteInfo structure to include two additional functions to */
/* invoke - we need a way to include suites dependent on the version of CUNIT being used  */

#if IMA_CUNIT_VERSION >= 213
#define IMA_TEST_SUITE(_name,_initSuite,_termSuite,_suite) { (_name), (_initSuite), (_termSuite), NULL, NULL, (_suite) }
#else
#define IMA_TEST_SUITE(_name,_initSuite,_termSuite,_suite) { (_name), (_initSuite), (_termSuite), (_suite) }
#endif


