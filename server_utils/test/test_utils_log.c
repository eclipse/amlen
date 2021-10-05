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
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>

#include "test_utils_log.h"
#include "test_utils_file.h"

static testLogLevel_t maxLevelOutput = testLOGLEVEL_TESTDESC;

/// @brief Test log function
/// @param[in]    level  Debug level of the text provided
/// @param[in]     format              - Format string
/// @param[in]     ...                 - Arguments to format string
void test_log(testLogLevel_t level, char *format, ...)
{
#define BUFFER_SIZE 2048
    static char indent[]="--------------------";
    char buffer[BUFFER_SIZE+2]; 
    va_list ap;
    int len;

    if (maxLevelOutput >= level)
    {
        va_start(ap, format);
        if (format[0] != '\0')
        {
            len=sprintf(buffer, "%.*s ", level+1, indent);
            len+=vsnprintf(&buffer[len], BUFFER_SIZE-len, format, ap);
            buffer[len]='\n';
            buffer[len+1]=0;

            printf("%s",buffer);
        }
        else
        {
            printf("\n");
        }
        va_end(ap);
    }
}
/// @brief Set maximum log level at which future calls to test log will be output
void test_setLogLevel(testLogLevel_t maxlevel)
{
    maxLevelOutput = maxlevel;
}

/// @brief Query the maximum log level at which calls to test log will be output
testLogLevel_t test_getLogLevel(void)
{
    return maxLevelOutput;
}

///Used internally by TEST_ASSERT:
void test_log_fatal(char *format, ...)
{
    va_list ap;

    if (format[0] != '\0')
    {
        va_start(ap, format);
        vprintf(format, ap);
        printf("\n");
        va_end(ap);
    }
}

// @brief Display a back trace for the current location
void test_log_backtrace(void)
{
    #define BACKTRACE_DEPTH 100

    void    *array[BACKTRACE_DEPTH];
    size_t  size, i;
    char    **strings;

    size = backtrace(array, BACKTRACE_DEPTH);
    strings = backtrace_symbols(array, size);

    for (i = 0; i < size; ++i)
    {
        printf("  %p : %s\n", array[i], strings[i]);
    }

    free(strings);
}

