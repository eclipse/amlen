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
 * File: testArrayCUnit.c
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
#include "testArrayCUnit.h"

/**
 * Perform initialization for hash map test suite.
 */
int initArrayCUnitSuite(void){
    ism_ArrayInit();
	return 0;
}

/**
 * Clean up after hash map test suite is executed.
 */
int cleanArrayCUnitSuite(void){
	return 0;
}

static int freeCount = 0;
static void myFree(char* ptr) {
	CU_ASSERT(ptr != NULL);
	ism_common_free(ism_memory_utils_misc,ptr);
	freeCount++;
}
#define NULL_PTR ((void**)0)
static const int capacity = 0xff;
static void CUnit_ISM_array_test(void){
	int i,rc;
	char value[128];
	char *pValue;
	int size;
	ismArray_t array = ism_common_createArray(capacity);
    CU_ASSERT(array != NULL);
	for(i = 1; i < capacity; i++){
		CU_ASSERT(ism_common_getArrayNumElements(array) == (i-1));
		sprintf(value,"value.%d",i);
		if(i%2){
			rc = ism_common_addToArray(array,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value));
		}else{
			rc = ism_common_addToArrayLock(array,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value));
		}
		CU_ASSERT(rc == i);
        CU_ASSERT(ism_common_getArrayNumElements(array) == i);
	}
    CU_ASSERT(ism_common_getArrayNumElements(array) == (capacity-1));
    rc = ism_common_addToArray(array,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value));
    CU_ASSERT(rc == 0);

    sprintf(value,"value.%d",6);
    pValue = (char*) ism_common_getArrayElementLock(array, 6);
    CU_ASSERT_FATAL(pValue != NULL);
    CU_ASSERT(strcmp(value,pValue) == 0);
    pValue = (char*) ism_common_removeArrayElementLock(array, 6);
    CU_ASSERT_FATAL(pValue != NULL);
    CU_ASSERT(strcmp(value,pValue) == 0);
    pValue = (char*) ism_common_getArrayElementLock(array, 6);
    CU_ASSERT_FATAL(pValue == NULL);
    pValue = (char*) ism_common_removeArrayElementLock(array, 6);
    CU_ASSERT_FATAL(pValue == NULL);
    size = ism_common_getArrayNumElements(array);
    CU_ASSERT(size == capacity-2);
    sprintf(value,"newValue.%d",6);
    rc = ism_common_addToArrayLock(array,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value));
    CU_ASSERT(rc == 6);
    size = ism_common_getArrayNumElements(array);
    CU_ASSERT(size == (capacity-1));
    pValue = (char*) ism_common_getArrayElementLock(array, 6);
    CU_ASSERT_FATAL(pValue != NULL);
    CU_ASSERT(strcmp(value,pValue) == 0);

    sprintf(value,"value.%d",7);
    pValue = (char*) ism_common_getArrayElement(array, 7);
    CU_ASSERT_FATAL(pValue != NULL);
    CU_ASSERT(strcmp(value,pValue) == 0);
    pValue = (char*) ism_common_removeArrayElement(array, 7);
    CU_ASSERT_FATAL(pValue != NULL);
    CU_ASSERT(strcmp(value,pValue) == 0);
    pValue = (char*) ism_common_getArrayElement(array, 7);
    CU_ASSERT_FATAL(pValue == NULL);
    pValue = (char*) ism_common_removeArrayElement(array, 7);
    CU_ASSERT_FATAL(pValue == NULL);
    size = ism_common_getArrayNumElements(array);
    CU_ASSERT(size == capacity-2);
    sprintf(value,"newValue.%d",7);
    rc = ism_common_addToArray(array,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value));
    CU_ASSERT(rc == 7);
    size = ism_common_getArrayNumElements(array);
    CU_ASSERT(size == (capacity-1));
    pValue = (char*) ism_common_getArrayElement(array, 7);
    CU_ASSERT_FATAL(pValue != NULL);
    CU_ASSERT(strcmp(value,pValue) == 0);


    size = ism_common_getArrayNumElements(array);
    CU_ASSERT(size == (capacity-1));
    freeCount = 0;
    ism_common_destroyArrayAndFreeValues(array, (ism_freeValueObject)myFree);
    CU_ASSERT(freeCount == size);
}

/*
 * Array tests for server_utils APIs to CUnit framework.
 */
CU_TestInfo ISM_Util_CUnit_Array[] =
  {
        { "arrayTest", CUnit_ISM_array_test },
       CU_TEST_INFO_NULL
  };

