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
 * File: testHashMapCUnit.c
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
#include "testHashMapCUnit.h"

//defined in ismutil.c
void ism_common_initUtil2(int type);

/**
 * Perform initialization for hash map test suite.
 */
int initHashMapCUnitSuite(void){
	return 0;
}

/**
 * Clean up after hash map test suite is executed.
 */
int cleanHashMapCUnitSuite(void){
	return 0;
}

static int freeCount = 0;
static void myFree(char* ptr) {
	CU_ASSERT(ptr != NULL);
	ism_common_free(ism_memory_utils_misc,ptr);
	freeCount++;
}
#define NULL_PTR ((void**)0)
static void CUnit_ISM_hm_test(void){
	ism_common_initUtil2(2);
	int i,rc,j=0;
	char key[32];
	char value[128];
	char* oldValue;
	void** oldValuePtr = (void**)&oldValue;
	int size;
    ismHashMap * hashMap1 = ism_common_createHashMap(32, HASH_INT32);
    ismHashMap * hashMap2 = ism_common_createHashMap(32, HASH_STRING);
    ismHashMap * hashMap3 = ism_common_createHashMap(32, HASH_INT64);
    CU_ASSERT(hashMap1 != NULL);
    CU_ASSERT(hashMap2 != NULL);
    CU_ASSERT(hashMap3 != NULL);
	for(i = 0; i < 256; i++){
		CU_ASSERT(ism_common_getHashMapNumElements(hashMap1) == i);
		sprintf(value,"value.%d",i);
		if(i%2){
			rc = ism_common_putHashMapElement(hashMap1,&i,4,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value),NULL_PTR);
		}else{
			rc = ism_common_putHashMapElementLock(hashMap1,&i,4,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value),oldValuePtr);
			CU_ASSERT(oldValue == NULL);
		}
		CU_ASSERT(rc == 0);
	}
    CU_ASSERT(ism_common_getHashMapNumElements(hashMap1) == i);

	for(i = 0; i < 256; i++){
		CU_ASSERT(ism_common_getHashMapNumElements(hashMap2) == i);
		sprintf(key,"key.%d",i);
		sprintf(value,"value.%d",i);
		if(i%2){
			rc = ism_common_putHashMapElement(hashMap2,key,0,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value),NULL_PTR);
		}else{
			rc = ism_common_putHashMapElementLock(hashMap2,key,0,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value),oldValuePtr);
			CU_ASSERT(oldValue == NULL);
		}
		CU_ASSERT(rc == 0);
	}
    CU_ASSERT(ism_common_getHashMapNumElements(hashMap2) == i);
	for(i = 0; i < 256; i++){
		CU_ASSERT(ism_common_getHashMapNumElements(hashMap3) == i);
		sprintf(key,"skey.%03d",i);
		sprintf(value,"value.%03d",i);
		if(i%2){
			rc = ism_common_putHashMapElement(hashMap3,key,8,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value),NULL_PTR);
		}else{
			rc = ism_common_putHashMapElementLock(hashMap3,key,8,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value),oldValuePtr);
			CU_ASSERT(oldValue == NULL);
		}
		CU_ASSERT(rc == 0);
	}
	CU_ASSERT(ism_common_getHashMapNumElements(hashMap3) == i);

	rc = ism_common_putHashMapElement(hashMap1,&j,4,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),"0:newValue"),oldValuePtr);
	CU_ASSERT(rc == 0);
	CU_ASSERT(strcmp(oldValue,"value.0") == 0);
	rc = ism_common_putHashMapElementLock(hashMap1,&j,4,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),"0:newValue1"),oldValuePtr);
	CU_ASSERT(rc == 0);
	CU_ASSERT(strcmp(oldValue,"0:newValue") == 0);

	rc = ism_common_putHashMapElement(hashMap2,"key.200",0,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),"200:newValue"),oldValuePtr);
	CU_ASSERT(rc == 0);
	CU_ASSERT(strcmp(oldValue,"value.200") == 0);
	rc = ism_common_putHashMapElementLock(hashMap2,"key.200",0,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),"200:newValue1"),oldValuePtr);
	CU_ASSERT(rc == 0);
	CU_ASSERT(strcmp(oldValue,"200:newValue") == 0);

	rc = ism_common_putHashMapElement(hashMap3,"skey.151",8,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),"151:newValue"),oldValuePtr);
	CU_ASSERT(rc == 0);
	CU_ASSERT(strcmp(oldValue,"value.151") == 0);
	rc = ism_common_putHashMapElementLock(hashMap3,"skey.151",8,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),"151:newValue1"),oldValuePtr);
	CU_ASSERT(rc == 0);
	CU_ASSERT(strcmp(oldValue,"151:newValue") == 0);
	size = ism_common_getHashMapNumElements(hashMap3);
	CU_ASSERT(size == 256);
	for(i = 0; i < 256; i++){
		if(!(i%3)){
			sprintf(key,"skey.%03d",i);
			sprintf(value,"value.%03d",i);
			size--;
			if(i%2){
				oldValue = ism_common_removeHashMapElement(hashMap3,key,8);
				CU_ASSERT(strcmp(oldValue,value) == 0);
				oldValue = ism_common_removeHashMapElement(hashMap3,key,8);
				CU_ASSERT(oldValue == NULL);
			}else{
				oldValue = ism_common_removeHashMapElementLock(hashMap3,key,8);
				CU_ASSERT(strcmp(oldValue,value) == 0);
				oldValue = ism_common_removeHashMapElementLock(hashMap3,key,8);
				CU_ASSERT(oldValue == NULL);
			}
			CU_ASSERT(size == ism_common_getHashMapNumElements(hashMap3));
		}
	}

    size = ism_common_getHashMapNumElements(hashMap2);
    CU_ASSERT(size == 256);
    for(i = 0; i < 256; i++){
		if(!(i%3)){
			sprintf(key,"key.%d",i);
			sprintf(value,"value.%d",i);
			size--;
			if(i%2){
				oldValue = ism_common_removeHashMapElement(hashMap2,key,0);
				CU_ASSERT(strcmp(oldValue,value) == 0);
				if(strcmp(oldValue,value)){
				    fprintf(stderr,"key=%s old=%s exp=%s\n",key,oldValue,value);
				}
				oldValue = ism_common_removeHashMapElement(hashMap2,key,0);
				CU_ASSERT(oldValue == NULL);
			}else{
				oldValue = ism_common_removeHashMapElementLock(hashMap2,key,0);
				CU_ASSERT(strcmp(oldValue,value) == 0);
				oldValue = ism_common_removeHashMapElementLock(hashMap2,key,0);
				CU_ASSERT(oldValue == NULL);
			}
			CU_ASSERT(size == ism_common_getHashMapNumElements(hashMap2));
		}
	}

    size = ism_common_getHashMapNumElements(hashMap1);
    CU_ASSERT(size == 256);
    freeCount = 0;
    ism_common_destroyHashMapAndFreeValues(hashMap1, (ism_freeValueObject)myFree);
    CU_ASSERT(freeCount == size);
    ism_common_destroyHashMap(hashMap2);
    ism_common_destroyHashMap(hashMap3);
}

static void CUnit_ISM_hm2array_test(void){
	ism_common_initUtil2(2);
	int i,rc;
	char value[128];
	int size;
	ismHashMapEntry ** array;
	ismHashMap * hashMap = ism_common_createHashMap(32000,HASH_INT32);
	CU_ASSERT(hashMap != NULL);
	for(i = 0; i < 4096; i++){
		CU_ASSERT(ism_common_getHashMapNumElements(hashMap) == i);
		sprintf(value,"value.%d",i);
		rc = ism_common_putHashMapElement(hashMap,&i,4,ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value),NULL);
		CU_ASSERT(rc == 0);
	}
	size = ism_common_getHashMapNumElements(hashMap);
	CU_ASSERT(size == i);
	array = ism_common_getHashMapEntriesArray(hashMap);
	CU_ASSERT(array != NULL);
	i = 0;
	while(array[i] != ((void*)-1)){
		int *pi = (int*)(array[i]->key);
		sprintf(value,"value.%d",*pi);
		CU_ASSERT(strcmp(value,array[i]->value) == 0);
		i++;
	}
	CU_ASSERT(size == i);
	ism_common_freeHashMapEntriesArray(array);
	ism_common_destroyHashMap(hashMap);

}

/*
 * Array of hash map tests for server_utils APIs to CUnit framework.
 */
CU_TestInfo ISM_Util_CUnit_HashMap[] =
  {
        { "hashMapTest", CUnit_ISM_hm_test },
        { "hashMap2ArrayTest", CUnit_ISM_hm2array_test },
       CU_TEST_INFO_NULL
  };

