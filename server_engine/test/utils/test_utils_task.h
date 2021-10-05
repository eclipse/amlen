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
#ifndef __ISM_TEST_UTILS_TASK_H_DEFINED
#define __ISM_TEST_UTILS_TASK_H_DEFINED

#include <test_utils_log.h>

typedef void (*test_task_fnptr)(void *context);

void test_task_create( char *name
                     , test_task_fnptr pfn
                     , void *context
                     , uint32_t uFrequency
                     , void **phandle);

void test_task_stop( void *handle
                   , testLogLevel_t level
                   , uint64_t *pTriggerCount
                   , uint64_t *pMissCount );

void test_task_stopall( testLogLevel_t level );

typedef void * (* ism_simple_thread_func_t)(void * parm);
int test_task_startThread(ism_threadh_t * handle, ism_simple_thread_func_t addr , void * data, const char * name);

#endif //end #ifndef __ISM_TEST_UTILS_TASK_H_DEFINED
