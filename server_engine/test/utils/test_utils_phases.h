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

//****************************************************************************
/// @file  test_utils_phases.h
/// @brief Simple phased unit test framework for use in unit tests
//****************************************************************************
#ifndef __ISM_TEST_UTILS_PHASES_H_DEFINED
#define __ISM_TEST_UTILS_PHASES_H_DEFINED

#include <CUnit/CUnit.h>

int test_utils_phaseMain(int argc,
                         char *argv[],
                         CU_SuiteInfo **phaseSuites,
                         uint32_t availablePhases);

#define test_utils_simplePhases(_argc, _argv, _phaseSuites) \
    test_utils_phaseMain((_argc),(_argv),(_phaseSuites),(uint32_t)(sizeof((_phaseSuites))/sizeof((_phaseSuites)[0])))

#endif //end ifndef __ISM_TEST_UTILS_PHASES_H_DEFINED
