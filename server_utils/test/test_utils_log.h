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
#ifndef __ISM_TEST_UTILS_LOG_H_DEFINED
#define __ISM_TEST_UTILS_LOG_H_DEFINED

#include <stdarg.h>

typedef enum tag_testLogLevel_t
{
    testLOGLEVEL_SILENT,
    testLOGLEVEL_TESTNAME,     ///< test name & errors only
    testLOGLEVEL_ERROR = testLOGLEVEL_TESTNAME, 
    testLOGLEVEL_TESTDESC,     ///< above + test description
    testLOGLEVEL_CHECK,        ///< above + validation check
    testLOGLEVEL_TESTPROGRESS, ///< above + test progress
    testLOGLEVEL_VERBOSE       ///< above + verbose test progress
} testLogLevel_t;

/// @brief Test log function
/// @param[in]    level  Debug level of the text provided
/// @param[in]     format              - Format string
/// @param[in]     ...                 - Arguments to format string
void test_log(testLogLevel_t level, char *format, ...);

/// @brief Set maximum log level at which future calls to test log will be output
void test_setLogLevel(testLogLevel_t maxlevel);

/// @brief Query the maximum log level at which calls to test log will be output
testLogLevel_t test_getLogLevel(void);

///Used internally by TEST_ASSERT:
void test_log_fatal(char *format, ...);

// @brief Display a back trace for the current location
void test_log_backtrace(void);

#endif //end #ifndef __ISM_TEST_UTILS_LOG_H_DEFINED
