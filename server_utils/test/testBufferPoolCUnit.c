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
 * File: testBufferPoolCUnit.c
 * Component: server
 * SubComponent: server_utils
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ismutil.h>
#include <ismuints.h>
#include "test_utils_assert.h"
#include "testBufferPoolCUnit.h"

#define TOBJ_INIT_SIZE  1536
#define MINPOOLSIZE 1024
#define MAXPOOLSIZE 10*1024
#define NUMTHREADS 2

static ism_byteBuffer tmpbb = NULL;
static const int bufsPerThread = MINPOOLSIZE;
static ismArray_t *threadBufs;

/**
 * Perform initialization for hash map test suite.
 */
int initBufferPoolCUnitSuite(void){
    /* create an array of ismArray objects to be used later to store byte buffer pointers.
     * threads will concurrently grab buffers from the bufferpool and temporarily store them in its own ismArray */
    threadBufs = (ismArray_t*) calloc(NUMTHREADS,sizeof(ismArray_t));
    for (int i=0;i<NUMTHREADS;i++){
    	threadBufs[i] = ism_common_createArray(bufsPerThread);
    }
    return 0;
}

/**
 * Clean up after hash map test suite is executed.
 */
int cleanBufferPoolCUnitSuite(void){
    // clean up memory allocated in the init cunit suite routine
    for (int i=0;i<NUMTHREADS;i++){
        ism_common_destroyArray(threadBufs[i]);
    }
    free(threadBufs);
	return 0;
}

static ism_byteBufferPool pool = NULL;
static void CUnit_ISM_BufferPool_test_create(void) {
    // Allocate buffer pool and verify head and sizes are as expected
    pool = ism_common_createBufferPool(TOBJ_INIT_SIZE, MINPOOLSIZE, MAXPOOLSIZE,"TestPool");
    TEST_ASSERT_PTR_NOT_NULL(pool);
    TEST_ASSERT_PTR_NOT_NULL(pool->head);
	TEST_ASSERT_PTR_NOT_NULL(pool->head->next);
	TEST_ASSERT_EQUAL(pool->allocated, pool->free); // at this point allocated should equal free
	TEST_ASSERT_EQUAL(pool->free, MINPOOLSIZE); // number of free buffers should be equal to the min pool size
	TEST_ASSERT_GREATER_THAN_OR_EQUAL(pool->maxPoolSize, pool->minPoolSize);
	TEST_ASSERT_EQUAL(pool->maxPoolSize, MAXPOOLSIZE);
	TEST_ASSERT_EQUAL(pool->minPoolSize, MINPOOLSIZE);
}

static void CUnit_ISM_BufferPool_test_destroy(void) {
	ism_common_destroyBufferPool(pool);
}

static void CUnit_ISM_BufferPool_test_getBuffer(void) {
    ism_byteBuffer headBeforeGetBuffer = pool->head;
    ism_byteBuffer headNext = pool->head->next;
    tmpbb = ism_common_getBuffer(pool, 0);
    ism_byteBuffer headAfterGetBuffer = pool->head;
    TEST_ASSERT_PTR_EQUAL(headBeforeGetBuffer, tmpbb); // assert that the buffer was retrieved from the head of the buffer pool
    TEST_ASSERT_PTR_EQUAL(headAfterGetBuffer, headNext); // assert current head points to previous head next
    TEST_ASSERT_EQUAL(tmpbb->used, 0); // buf used count should be zero on a new buffer
    TEST_ASSERT_EQUAL(tmpbb->allocated, TOBJ_INIT_SIZE); // byte buffer should be of the expected size
    TEST_ASSERT_PTR_NOT_NULL(tmpbb->buf); // buf should be non-NULL
    TEST_ASSERT_PTR_NOT_NULL(pool->head); // ensure pool head is not null
    TEST_ASSERT_EQUAL(pool->free, MINPOOLSIZE - 1); // free should be 1 buffer smaller than initial/min size
    TEST_ASSERT_EQUAL(pool->allocated, MINPOOLSIZE); // allocated should not have changed
}

static void CUnit_ISM_BufferPool_test_returnBuffer(void) {
    ism_byteBuffer headBeforeReturnBuffer = pool->head;
    ism_common_returnBuffer(tmpbb, __FILE__, __LINE__);
    ism_byteBuffer headAfterReturnBuffer = pool->head;
    ism_byteBuffer headAfterReturnBufferNext = headAfterReturnBuffer->next;
    TEST_ASSERT_PTR_EQUAL(headBeforeReturnBuffer, headAfterReturnBufferNext) // ensure that previous head is next of current head, after return buffer
    TEST_ASSERT_PTR_EQUAL(tmpbb, headAfterReturnBuffer); // ensure that the returned buffer is now the head of the bufferpool list,after return buffer
    TEST_ASSERT_PTR_NOT_NULL(pool->head); // ensure pool head is not null
    TEST_ASSERT_EQUAL(pool->free, MINPOOLSIZE); // bufferpool free should now match the initial/min pool size
}

static void CUnit_ISM_BufferPool_test_drainPoolWithForce(void) {
	/* *
	 * 1. call getBuffer with the force flag
	 */
	ismArray_t buffArray = ism_common_createArray(4 * MAXPOOLSIZE); // 4 x max pool size
	ism_byteBuffer buff = NULL;
	uint32_t rc = 0;
	for (int i = 0; i < (2 * MAXPOOLSIZE); i++) {
		buff = ism_common_getBuffer(pool, 1);
		TEST_ASSERT_PTR_NOT_NULL(buff); // buff should be non-NULL
		TEST_ASSERT_PTR_NOT_NULL(buff->buf); // buff->buf should be non-NULL
		TEST_ASSERT_EQUAL(buff->used, 0); // buff used count should be zero on a new buffer
		TEST_ASSERT_EQUAL(buff->allocated, TOBJ_INIT_SIZE); // byte buffer should be of the expected size
		if (pool->free > 0) {
			TEST_ASSERT_PTR_NOT_NULL(pool->head); // ensure pool head is not null if there are free buffers in the pool
		} else {
			TEST_ASSERT_EQUAL(pool->free, 0); // buffer pool free count should be 0
			TEST_ASSERT_PTR_NULL(pool->head); // ensure pool head is null
		}
		rc = ism_common_addToArray(buffArray, buff);
		TEST_ASSERT_NOT_EQUAL(rc, 0); // ensure that array insert did not return index with value 0 (0 = insert failed)
	}

	TEST_ASSERT_EQUAL(pool->allocated, 2 * MAXPOOLSIZE); // buffer pool allocated should equal 2 x MAXPOOLSIZE

	// return MAXPOOLSIZE number of buffers back to the pool, remember MAXPOOLSIZE * 2 were allocated
	for (int i = 1 /*cannot use index 0 for utils array*/, count = 0; count < 2 * MAXPOOLSIZE; i++, count++) {
		buff = ism_common_removeArrayElement(buffArray, i);
		if (pool->free < MAXPOOLSIZE) {
			TEST_ASSERT_EQUAL(pool->free, count); // buffer pool free count should be equal to count up to MAXPOOLSIZE
		} else {
			TEST_ASSERT_EQUAL(pool->free, MAXPOOLSIZE);
		}
		ism_common_returnBuffer(buff, __FILE__, __LINE__);
		TEST_ASSERT_PTR_NOT_NULL(pool->head); // ensure pool head is not null when free count > 0
	}

	TEST_ASSERT_EQUAL(pool->allocated, MAXPOOLSIZE); // buffer pool allocated should equal MAXPOOLSIZE after returning all the buffers to the pool
	TEST_ASSERT_EQUAL(pool->free, MAXPOOLSIZE); // buffer pool free should equal 2 * MAXPOOLSIZE now that all buffers are returned to the pool
	TEST_ASSERT_PTR_NOT_NULL(pool->head); // ensure pool head is not null

	for (int i = 0; i < (2 * MAXPOOLSIZE); i++) {
		buff = ism_common_getBuffer(pool, 1);
		TEST_ASSERT_PTR_NOT_NULL(buff); // buff should be non-NULL
		TEST_ASSERT_PTR_NOT_NULL(buff->buf); // buff->buf should be non-NULL
		TEST_ASSERT_EQUAL(buff->used, 0); // buff used count should be zero on a new buffer
		TEST_ASSERT_EQUAL(buff->allocated, TOBJ_INIT_SIZE); // byte buffer should be of the expected size
		if (pool->free > 0) {
			TEST_ASSERT_PTR_NOT_NULL(pool->head); // ensure pool head is not null if there are free buffers in the pool
		}
		rc = ism_common_addToArray(buffArray, buff);
		TEST_ASSERT_NOT_EQUAL(rc, 0); // ensure that array insert did not return index with value 0 (0 = insert failed)
	}

	// return buffers back to the pool for the next test
	for (int i = 1 /*cannot use index 0 for utils array*/; i < (2 * MAXPOOLSIZE) + 1; i++) {
		buff = ism_common_removeArrayElement(buffArray, i);
		ism_common_returnBuffer(buff, __FILE__, __LINE__);
	}

	TEST_ASSERT_PTR_NOT_NULL(pool->head); // ensure pool head is not null

	// cleanup temp array
	ism_common_destroyArray(buffArray);
}

static void CUnit_ISM_BufferPool_test_drainPoolWithoutForce(void) {
	ismArray_t buffArray = ism_common_createArray(MAXPOOLSIZE + 1); // pool max size + 1
	ism_byteBuffer buff = NULL;
	uint32_t rc=0;
	for (int i=0 ; i < MAXPOOLSIZE ; i++) {
		buff = ism_common_getBuffer(pool, 0);
		TEST_ASSERT_PTR_NOT_NULL(buff); // buff should be non-NULL
		TEST_ASSERT_PTR_NOT_NULL(buff->buf); // buff->buf should be non-NULL
		TEST_ASSERT_EQUAL(buff->used, 0); // buff used count should be zero on a new buffer
		TEST_ASSERT_EQUAL(buff->allocated, TOBJ_INIT_SIZE); // byte buffer should be of the expected size
		if (pool->free > 0) {
			TEST_ASSERT_PTR_NOT_NULL(pool->head); // ensure pool head is not null if there are free buffers in the pool
		}
		rc = ism_common_addToArray(buffArray, buff);
		TEST_ASSERT_NOT_EQUAL(rc, 0); // ensure that array insert did not return index with value 0 (0 = insert failed)
	}
	buff = ism_common_getBuffer(pool, 0); // get one buffer beyond max size of the pool
	TEST_ASSERT_PTR_NULL(buff); // we've exceeded the buffer pool max size and force flag was not enabled, so buffer should be NULL
	TEST_ASSERT_PTR_NULL(pool->head); // pool head should be NULL when there are no more free buffers
	TEST_ASSERT_EQUAL(pool->allocated, MAXPOOLSIZE); // buffer pool allocated should equal MAXPOOLSIZE

	// return buffers back to the pool for the next test
	for (int i = 1 /*cannot use index 0 for utils array*/; i < MAXPOOLSIZE + 1; i++) {
		buff  = ism_common_removeArrayElement(buffArray, i);
		ism_common_returnBuffer(buff, __FILE__, __LINE__);
	}

	TEST_ASSERT_EQUAL(pool->allocated, MAXPOOLSIZE); // buffer pool allocated should equal MAXPOOLSIZE
	TEST_ASSERT_EQUAL(pool->free, MAXPOOLSIZE); // buffer pool free should equal MAXPOOLSIZE now that all buffers are returned to the pool
	TEST_ASSERT_PTR_NOT_NULL(pool->head); // ensure pool head is not null

	// cleanup temp array
	ism_common_destroyArray(buffArray);
}


/*
 * BufferPool tests for server_utils APIs to CUnit framework.
 */
CU_TestInfo ISM_Util_CUnit_BufferPool[] =
  {
       { "createBufferPool", CUnit_ISM_BufferPool_test_create },
       { "simpleGetBuffer", CUnit_ISM_BufferPool_test_getBuffer},
       { "simpleReturnBuffer", CUnit_ISM_BufferPool_test_returnBuffer},
       { "drainPoolWithoutForce", CUnit_ISM_BufferPool_test_drainPoolWithoutForce},
	   { "drainPoolWithForce", CUnit_ISM_BufferPool_test_drainPoolWithForce},
       { "destroyBufferPool", CUnit_ISM_BufferPool_test_destroy },
       CU_TEST_INFO_NULL
  };

