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
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

#include "assert.h"
#include "test_utils_log.h"

#include <signal.h>
#include <stdio.h>

#ifdef IMA_ASAN
//If we're running under ASAN, pause the process as we won't process a useful core
static inline void asan_pause(void)
{
    //Want to get to the pause ASAP... so use puts rather than printf)
    puts("ASAN: Pausing... (use gdb or fg or kill -s SIGCONT to continue)\n");

    //pause us...
    raise(SIGTSTP);

    //Ensure the signal arrived
    sleep(1);

    //If we got here we're running again
    printf("Running again\n");
}
#else
//If we're not running under ASAN we can go and produe a core as usual... no need to pause
#define asan_pause()
#endif

#define TEST_ASSERT_EQUAL_FORMAT(a, b, format) \
        { if( (a) != (b)) { \
             test_log_fatal( "TEST_ASSERT ("#a ")=" format " == (" #b ")=" format " on line %u file: %s\n", \
                             (a),(b), __LINE__,__FILE__); \
             asan_pause(); \
             assert((a) == (b)); \
             __builtin_trap(); \
          } else { \
              CU_ASSERT_EQUAL_FATAL(a, b); \
          } \
        }

#define TEST_ASSERT_EQUAL(a, b) TEST_ASSERT_EQUAL_FORMAT(a, b, "%d")
#define TEST_ASSERT_PTR_EQUAL(p1, p2) TEST_ASSERT_EQUAL_FORMAT(p1, p2, "%p")
#define TEST_ASSERT_PTR_NULL(p) TEST_ASSERT_EQUAL_FORMAT(p, NULL, "%p")

#define TEST_ASSERT_NOT_EQUAL_FORMAT(a, b, format) \
        { if( (a) == (b)) { \
             test_log_fatal( "TEST_ASSERT ("#a ")=" format " != (" #b ")=" format " on line %u file: %s\n", \
                             (a),(b), __LINE__,__FILE__); \
             asan_pause(); \
             assert((a) != (b)); \
             __builtin_trap(); \
          } else { \
              CU_ASSERT_NOT_EQUAL_FATAL(a, b); \
          } \
        }

#define TEST_ASSERT_NOT_EQUAL(a, b) TEST_ASSERT_NOT_EQUAL_FORMAT(a, b, "%d")
#define TEST_ASSERT_PTR_NOT_NULL(p) TEST_ASSERT_NOT_EQUAL_FORMAT(p, NULL, "%p")

#define TEST_ASSERT_GREATER_THAN_FORMAT(a, b, format) \
        { if( (a) <= (b)) { \
             test_log_fatal( "TEST_ASSERT ("#a ")=" format " > (" #b ")=" format " on line %u file: %s\n", \
                             (a),(b), __LINE__,__FILE__); \
             asan_pause(); \
             assert((a) != (b)); \
             __builtin_trap(); \
          } else { \
              CU_ASSERT_FATAL((a)>(b)); \
          } \
        }

#define TEST_ASSERT_GREATER_THAN(a, b) TEST_ASSERT_GREATER_THAN_FORMAT(a, b, "%d")

#define TEST_ASSERT_GREATER_THAN_OR_EQUAL_FORMAT(a, b, format) \
        { if( (a) < (b)) { \
             test_log_fatal( "TEST_ASSERT ("#a ")=" format " >= (" #b ")=" format " on line %u file: %s\n", \
                             (a),(b), __LINE__,__FILE__); \
             asan_pause(); \
             assert((a) != (b)); \
             __builtin_trap(); \
          } else { \
              CU_ASSERT_FATAL((a)>=(b)); \
          } \
        }

#define TEST_ASSERT_GREATER_THAN_OR_EQUAL(a, b) TEST_ASSERT_GREATER_THAN_OR_EQUAL_FORMAT(a, b, "%d")

#define TEST_ASSERT_CUNIT(_exp, _text) \
       {  if (!(_exp)) { \
                test_log(1, "ASSERT(%s)", #_exp); \
                test_log_fatal _text; \
                asan_pause(); \
                assert(_exp); \
                __builtin_trap(); \
          } else { \
                CU_ASSERT_FATAL(_exp); \
          } \
       }

#define TEST_ASSERT(_exp, _text) \
       {  if (!(_exp)) { \
                test_log(1, "ASSERT(%s)", #_exp); \
                test_log_fatal _text; \
                asan_pause(); \
                assert(_exp); \
                __builtin_trap(); \
          } \
       }

#define TEST_ASSERT_STRINGS_EQUAL(a, b) \
        { if( strcmp((a), (b)) != 0) { \
             test_log_fatal( "TEST_ASSERT ("#a ")='%s' == (" #b ")='%s' on line %u file: %s\n", \
                             (a),(b), __LINE__,__FILE__); \
             asan_pause(); \
             assert(false); \
             __builtin_trap(); \
          } else { \
              CU_ASSERT_EQUAL_FATAL(strcmp((a),(b)),0); \
          } \
        }
