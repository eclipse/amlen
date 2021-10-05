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

//*********************************************************************
/// @file  engineCommon.h
/// @brief Engine component common internal header file
///        Should be included by all Engine component headers.
//*********************************************************************
#ifndef __ISM_ENGINECOMMON_DEFINED
#define __ISM_ENGINECOMMON_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

//If we define CACHELINE_SEPARATION then...
/* Some structures are aligned on cache-line boundaries to try to    */
/* minimise false sharing.                                           */
//#define CACHELINE_SEPARATION 1

#ifdef CACHELINE_SEPARATION
  #define CACHELINE_SIZE 64
  #define CACHELINE_ALIGNED __attribute__ ((aligned (CACHELINE_SIZE)))
#else
  #define CACHELINE_ALIGNED
#endif


/* *********************************************************************
 * Branch Prediction macros (only used for GCC)
 * These should not be overused, a study in the linux kernel found them
 * overused to the extent that turning many of them off made the code
 * faster.
 * http://lwn.net/Articles/420019/
 *
 */
#ifdef __GNUC__
#define UNLIKELY(a) __builtin_expect((a), 0)
#define LIKELY(a)   __builtin_expect((a), 1)
#else
#define UNLIKELY(a) (a)
#define LIKELY(a)   (a)
#endif

// Variables that are only used for debugging (e.g. assert calls) should be
// qualified with 'DEBUG_ONLY' to stop the compiler complaining when asserts
// are disabled (NDEBUG is set)
#ifdef NDEBUG
#define DEBUG_ONLY __attribute__((unused))
#else
#define DEBUG_ONLY
#endif

// Constants for trace levels

// Component entry/exit
#define ENGINE_CEI_TRACE          7

// Function entry/exit for selected functions
#define ENGINE_FNC_TRACE          8

// Functions called so often they should require a higher trace level
#define ENGINE_HIFREQ_FNC_TRACE   9

// Server ought to be stopping, everything is going wrong
#define ENGINE_SEVERE_ERROR_TRACE 2

// It's gone wrong (e.g. for one queue)...but the server won't come down
#define ENGINE_ERROR_TRACE        4

// Something looks suspicious
#define ENGINE_WORRYING_TRACE     5

// Something a little unusual happened
#define ENGINE_UNUSUAL_TRACE      7

// Something generally interesting happened
#define ENGINE_NORMAL_TRACE       8

// Something interesting which happens with a high frequency
#define ENGINE_HIGH_TRACE         9

// First failure data capture
#define ENGINE_FFDC               2

// Something interesting happened - such as last server timestamp
#define ENGINE_INTERESTING_TRACE  5

// When diagnosing perf problems these trace points may help....
#define ENGINE_PERFDIAG_TRACE     6

// To diagnose shutdown problems, we might need a higher level of trace.
#define ENGINE_SHUTDOWN_DIAG_TRACE ENGINE_INTERESTING_TRACE

// Constants for use on trace entry and exit or to identify a small function
// (use __func__ for function name)
// e.g.
//   TRACE(ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);
//   TRACE(ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
//   TRACE(ENGINE_FNC_TRACE, FUNCTION_IDENT "\n", __func__);
#define FUNCTION_ENTRY ">>> %s "
#define FUNCTION_IDENT "=== %s "
#define FUNCTION_EXIT  "<<< %s "

#define SEL_ACTIVATE   "~A"
#define SEL_DEACTIVATE "~D"
#define SEL_SINGLE     "~S"
#define SEL_ROUNDUP    "~R"

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINECOMMON_DEFINED */

/*********************************************************************/
/* End of engineCommon.h                                             */
/*********************************************************************/
