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
 * File: testUtilsMemHandler.c
 * Component: server
 * SubComponent: server_utils
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */
#include <ismutil.h>
#include <imacontent.h>
#include <log.h> // for ism_log_init();

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "testUtilsMemHandlerCUnit.h"
extern int g_verbose;
//memHandler function for unit tests:
size_t ism_common_queryReservation(ism_common_memoryType type);

/*   (programming note 1)
 * (programming note 11)
 * Array of timer tests for server_utils APIs to CUnit framework.
 */
CU_TestInfo ISM_Util_CUnit_memHandler[] = {
		{ "basic", CUnit_test_MemHandler_basic },
		{ "checking", CUnit_test_MemHandler_checking },
		{ "realloc", CUnit_test_MemHandler_realloc },
		{ "transfer", CUnit_test_MemHandler_transfer },
		CU_TEST_INFO_NULL };

/* The timer tests initialization function.
 * Creates a timer thread.
 * Returns zero on success, non-zero otherwise.
 */
int initMemHandlerTests(void) {

    return 0;
}

/* The timer test suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
int initMemHandlerSuite(void) {
	ism_log_init();
    return 0;
}

/* The timer test suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
int cleanMemHandlerSuite(void) {

	return 0;
}

typedef struct test_struct_t
{
	char payload[256];
} test_struct_t;

typedef struct test_struct_large_t
{
	char payload[512];
} test_struct_large_t;

void CUnit_test_MemHandler_basic(void) {

	test_struct_t * buffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_unit_test,1), sizeof(test_struct_t));
	ism_common_free(ism_memory_unit_test, buffer);

	test_struct_t * buffer2 = ism_common_calloc(ISM_MEM_PROBE(ism_memory_unit_test,2), 10, sizeof(test_struct_t));
	ism_common_free(ism_memory_unit_test, buffer2);

#ifdef COMMON_MALLOC_WRAPPER
	CU_ASSERT(ism_common_queryReservation(ism_memory_unit_test) == ism_common_getMemChunkSize());
#endif
}

void CUnit_test_MemHandler_checking(void) {

	printf("\n");
	test_struct_t * buffer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_unit_test,3), 1, sizeof(test_struct_t));
#ifdef COMMON_MALLOC_WRAPPER
	CU_ASSERT(ism_confirm_eyecatcher(buffer));
	CU_ASSERT(ism_confirm_memType(ism_memory_unit_test,buffer));
	TRACE(1, "Expecting memType failure now:\n");
	CU_ASSERT(!ism_confirm_memType(ism_memory_utils_misc,buffer));
#endif
	printf("going to free memory:\n");
	ism_common_free(ism_memory_unit_test, buffer);

	printf("callocing:\n");
	test_struct_t * buffer2 = calloc(1,sizeof(test_struct_t));
	free(buffer2);
#ifdef COMMON_MALLOC_WRAPPER
	CU_ASSERT(ism_common_queryReservation(ism_memory_unit_test) == ism_common_getMemChunkSize());
#endif
}

void CUnit_test_MemHandler_realloc(void) {
	test_struct_t * buffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_unit_test,3), sizeof(test_struct_t));
	test_struct_large_t * largeBuffer = ism_common_realloc(ISM_MEM_PROBE(ism_memory_unit_test,4), buffer, sizeof(test_struct_large_t));
	ism_common_free(ism_memory_unit_test, largeBuffer);
#ifdef COMMON_MALLOC_WRAPPER
	CU_ASSERT(ism_common_queryReservation(ism_memory_unit_test) == ism_common_getMemChunkSize());
#endif
}

void CUnit_test_MemHandler_transfer(void) {
    test_struct_t * buffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_unit_test,3), sizeof(test_struct_t));
    test_struct_large_t * largeBuffer = ism_common_realloc(ISM_MEM_PROBE(ism_memory_unit_test,6), buffer, sizeof(test_struct_large_t));
#ifdef COMMON_MALLOC_WRAPPER
    CU_ASSERT(ism_common_queryReservation(0) == 0);
#endif
    ism_common_transfer_memory(ism_memory_unit_test, 0, largeBuffer);
#ifdef COMMON_MALLOC_WRAPPER
    size_t size = sizeof(test_struct_large_t) + sizeof(ism_common_Eyecatcher_t);
    CU_ASSERT(ism_common_queryReservation(0) <= (ism_common_getMemChunkSize() - size));
#endif
    ism_common_free(0, largeBuffer);
#ifdef COMMON_MALLOC_WRAPPER
    CU_ASSERT(ism_common_queryReservation(0) == ism_common_getMemChunkSize());
#endif
}
