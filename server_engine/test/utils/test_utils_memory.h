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
/// @file  test_utils_memory.h
/// @brief Routines to affect the memory allocation routines for tests.
//****************************************************************************
#ifndef __ISM_TEST_UTILS_MEMORY_H_DEFINED
#define __ISM_TEST_UTILS_MEMORY_H_DEFINED

#include "memHandler.h"

#define TEST_ANALYSE_IEMEM_NONE             0
#define TEST_ANALYSE_IEMEM_PROBE            IEMEM_PROBE(iemem_numtypes, 0)
#define TEST_ANALYSE_IEMEM_PROBE_STACK      IEMEM_PROBE(iemem_numtypes, 1)
#define TEST_ASSERT_IEMEM_PROBE             IEMEM_PROBE(iemem_numtypes, 2)
#define TEST_IEMEM_PROBE_MUTEX_INIT         IEMEM_PROBE(iemem_numtypes, 3)
#define TEST_IEMEM_PROBE_SPIN_INIT          IEMEM_PROBE(iemem_numtypes, 4)
#define TEST_IEMEM_PROBE_CONDATTR_INIT      IEMEM_PROBE(iemem_numtypes, 5)
#define TEST_IEMEM_PROBE_CONDATTR_SETCLOCK  IEMEM_PROBE(iemem_numtypes, 6)
#define TEST_IEMEM_PROBE_CONDATTR_DESTROY   IEMEM_PROBE(iemem_numtypes, 7)
//#define TEST_IEMEM_PROBE_COND_INIT          IEMEM_PROBE(iemem_numtypes, 8)
//#define TEST_IEMEM_PROBE_COND_BROADCAST     IEMEM_PROBE(iemem_numtypes, 9)

// The threadId of a failing test is set to this value
#define IEMEM_FAILURE_TEST_THREADID  UINT32_MAX

//****************************************************************************
/// @brief  Provide a list of memory allocation probes to fail and set / reset
///         calling thread's engineThreadId to IEMEM_FAILURE_TEST_THREADID
///
/// @param[in]     probesToFail  List of probes to fail.
/// @param[in]     probeCounts   Counts of each probe to allow before failing
///                              (or pass the original thread id if unsetting probes)
///
/// @remark The list is terminated with a 0, only the probes specified will fail
///         automatically, others will call the real underlying iemem_*alloc.
///         Setting NULL will turn calling iemem_*alloc back on.
///
///         A NULL probeCounts pointer will mean all of the specified probes
///         will fail - non null, we decrement the probe count each time the
///         probe is seen, once it hits zero, it starts failing.
///
/// @return The original threadId for the calling thread unless it has been
///         set to IEMEM_FAILURE_TEST_THREADID
//****************************************************************************
uint32_t test_memoryFailMemoryProbes(uint32_t *probesToFail, uint32_t *probeCounts);

#endif //end ifndef __ISM_TEST_UTILS_MEMORY_H_DEFINED
