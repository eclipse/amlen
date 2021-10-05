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
 * File: testLinkedListCUnit.c
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
#include "testLinkedListCUnit.h"

static ism_common_list	linkedLists[2];
/**
 * Perform initialization for linked list test suite.
 */
int initLinkedListCUnitSuite(void){
	return 0;
}

/**
 * Clean up after linked list test suite is executed.
 */
int cleanLinkedListCUnitSuite(void){
	return 0;
}

/**
 * Test ism_common_list_init function.
 */
static void CUnit_test_ISM_ll_init(void){
	CU_ASSERT(ism_common_list_init(&linkedLists[0],0,NULL) == 0);
	CU_ASSERT(linkedLists[0].head == NULL);
	CU_ASSERT(linkedLists[0].tail == NULL);
	CU_ASSERT(linkedLists[0].lock == NULL);
	CU_ASSERT(linkedLists[0].destroy == NULL);
	CU_ASSERT(linkedLists[0].size == 0);
	CU_ASSERT(ism_common_list_init(&linkedLists[1],1,NULL) == 0);
	CU_ASSERT(linkedLists[1].head == NULL);
	CU_ASSERT(linkedLists[1].tail == NULL);
	CU_ASSERT(linkedLists[1].lock != NULL);
	CU_ASSERT(linkedLists[1].destroy == NULL);
	CU_ASSERT(linkedLists[1].size == 0);
}

/**
 * Test ism_common_list_destroy function.
 */
static void CUnit_test_ISM_ll_destroy(void){
	ism_common_list_destroy(&linkedLists[0]);
	CU_ASSERT(linkedLists[0].head == NULL);
	CU_ASSERT(linkedLists[0].tail == NULL);
	CU_ASSERT(linkedLists[0].lock == NULL);
	CU_ASSERT(linkedLists[0].destroy == NULL);
	CU_ASSERT(linkedLists[0].size == 0);
	ism_common_list_destroy(&linkedLists[1]);
	CU_ASSERT(linkedLists[1].head == NULL);
	CU_ASSERT(linkedLists[1].tail == NULL);
	CU_ASSERT(linkedLists[1].lock == ((void*)-1));
	CU_ASSERT(linkedLists[1].destroy == NULL);
	CU_ASSERT(linkedLists[1].size == 0);
}

static void checkOrder(ism_common_list * list, uint64_t minNum,uint64_t maxNumber){
	ism_common_listIterator iter;
//	int i = 0;
	uint64_t expectedValue = minNum;
	ism_common_list_iter_init(&iter,list);
	CU_ASSERT(iter.list == list);
	CU_ASSERT(iter.currNode == list->head);
//	fprintf(stderr,"\n");
	while(ism_common_list_iter_hasNext(&iter)){
		ism_common_list_node * node = ism_common_list_iter_next(&iter);
//		fprintf(stderr,"checkOrder: i = %d: node = %p next=%p data=%llu expected=%llu\n",
//				i++,node,node->next,(uint64_t)node->data,expectedValue);
		CU_ASSERT(node != NULL);
		CU_ASSERT(expectedValue == ((uint64_t)(node->data)));

		expectedValue++;
	}
	ism_common_list_iter_destroy(&iter);
	CU_ASSERT(expectedValue == maxNumber);
}

static void CUnit_test_ISM_ll_insert_tail(void){
	uint64_t i, j;
	CUnit_test_ISM_ll_init();
	for(i = 0; i < 2; i++){
		ism_common_list * list = &linkedLists[i];
		for(j=0; j < 100; j++){
			CU_ASSERT(ism_common_list_insert_tail(list,(void*)j) == 0);
			CU_ASSERT(ism_common_list_size(list) == (j+1));
		}
		checkOrder(list,0,100);
	}
	CUnit_test_ISM_ll_destroy();
}
static void CUnit_test_ISM_ll_insert_head(void){
	uint64_t i, j;
	CUnit_test_ISM_ll_init();
	for(i = 0; i < 2; i++){
		ism_common_list * list = &linkedLists[i];
		for(j=0; j < 100; j++){
			CU_ASSERT(ism_common_list_insert_head(list,(void*)(99-j)) == 0);
			CU_ASSERT(ism_common_list_size(list) == (j+1));
		}
		checkOrder(list,0,100);
	}
	CUnit_test_ISM_ll_destroy();
}
int comparator(const void *data1, const void *data2){
	uint64_t i = (uint64_t) data1;
	uint64_t j = (uint64_t) data2;
	if(i < j) return -1;
	if(i > j) return 1;
	return 0;
}
static void CUnit_test_ISM_ll_insert_ordered(void){
	uint64_t i, j;
	CUnit_test_ISM_ll_init();
	for(i = 0; i < 2; i++){
		ism_common_list * list = &linkedLists[i];
		for(j=0; j < 100; j++){
			if(j%2)
				CU_ASSERT(ism_common_list_insert_ordered(list,(void*)(j),comparator) == 0);
		}
		for(j=100; j > 0; j--){
			uint64_t val = j-1;
			if(val%2 == 0)
				CU_ASSERT(ism_common_list_insert_ordered(list,(void*)val,comparator) == 0);
		}
		checkOrder(list,0,100);
	}
	CUnit_test_ISM_ll_destroy();
}
static void CUnit_test_ISM_ll_2_array(void){
	uint64_t i, j;
	void ** array;
	CUnit_test_ISM_ll_init();
	for(i = 0; i < 2; i++){
		ism_common_list * list = &linkedLists[i];
		CU_ASSERT(ism_common_list_to_array(list,&array)==0);
		CU_ASSERT(array == NULL);
		for(j=0; j < 100; j++){
			CU_ASSERT(ism_common_list_insert_head(list,(void*)(99-j)) == 0);
			CU_ASSERT(ism_common_list_size(list) == (j+1));
		}
		checkOrder(list,0,100);
		CU_ASSERT(ism_common_list_to_array(list,&array)==100);
		CU_ASSERT(array != NULL);
		for(j=0; j < 100; j++){
			CU_ASSERT(j == ((uint64_t)array[j]));
		}
		ism_common_list_array_free(array);
	}
	CUnit_test_ISM_ll_destroy();
}
static void CUnit_test_ISM_array_2_ll(void){
	uint64_t i, j;
	void * array[100];
	CUnit_test_ISM_ll_init();
	for(i = 0; i < 2; i++){
		ism_common_list * list = &linkedLists[i];
		for(j=0; j < 100; j++){
			array[j] = (void*)j;
		}
		CU_ASSERT(ism_common_list_from_array(list,array,0)==0);
		CU_ASSERT(ism_common_list_from_array(list,array,100)==0);
		checkOrder(list,0,100);
		CU_ASSERT(ism_common_list_from_array(list,array,100)==-10);
	}
	CUnit_test_ISM_ll_destroy();
}
static void CUnit_test_ISM_ll_remove(void){
	uint64_t i, j;
	void *data;
	CUnit_test_ISM_ll_init();
	for(i = 0; i < 2; i++){
		ism_common_listIterator iter;
		ism_common_list * list = &linkedLists[i];
		CU_ASSERT(ism_common_list_remove_head(list,&data)==-2);
		CU_ASSERT(ism_common_list_remove_tail(list,&data)==-2);
		CU_ASSERT(ism_common_list_remove(list,NULL,&data)==-2);
		for(j=0; j < 100; j++){
			CU_ASSERT(ism_common_list_insert_head(list,(void*)(99-j)) == 0);
			CU_ASSERT(ism_common_list_size(list) == (j+1));
		}
		CU_ASSERT(ism_common_list_remove_head(list,&data)==0);
		CU_ASSERT(((uint64_t)data)==0);
		CU_ASSERT(ism_common_list_remove(list,NULL,&data)==0);
		CU_ASSERT(((uint64_t)data)==1);
		CU_ASSERT(ism_common_list_remove(list,NULL,&data)==0);
		CU_ASSERT(((uint64_t)data)==2);
		CU_ASSERT(ism_common_list_remove(list,NULL,&data)==0);
		CU_ASSERT(((uint64_t)data)==3);
		CU_ASSERT(ism_common_list_remove_tail(list,&data)==0);
		CU_ASSERT(((uint64_t)data)==99);
		checkOrder(list,4,99);
		ism_common_list_iter_init(&iter,list);
		while(ism_common_list_iter_hasNext(&iter)){
			ism_common_list_node * node = ism_common_list_iter_next(&iter);
			uint64_t val = (uint64_t)node->data;
			if((val%2) == 0){
				CU_ASSERT(ism_common_list_remove(list,&iter,&data)==0);
				CU_ASSERT(((uint64_t)data) == val);
//				fprintf(stderr,"CUnit_test_ISM_ll_remove: val=%llu data=%llu\n",val,(uint64_t)data);
			}
		}
		ism_common_list_iter_reset(&iter);
		CU_ASSERT(iter.currNode == list->head);
		CU_ASSERT(iter.lastNode == ((void*)-1));
		while(ism_common_list_iter_hasNext(&iter)){
			ism_common_list_node * node = ism_common_list_iter_next(&iter);
			uint64_t val = (uint64_t)node->data;
			CU_ASSERT((val%2) == 1);
		}
		ism_common_list_iter_destroy(&iter);
		ism_common_list_clear(list);
		CU_ASSERT(list->head == NULL);
		CU_ASSERT(list->tail == NULL);
		CU_ASSERT(list->size == 0);
	}
	CUnit_test_ISM_ll_destroy();
}


/*
 * Array of linked list tests for server_utils APIs to CUnit framework.
 */
CU_TestInfo ISM_Util_CUnit_LinkedList[] =
  {
        { "initLinkedList", CUnit_test_ISM_ll_init },
        { "destroyLinkedList", CUnit_test_ISM_ll_destroy },
        { "LinkedListInsertTail", CUnit_test_ISM_ll_insert_tail },
        { "LinkedListInsertHead", CUnit_test_ISM_ll_insert_head },
        { "LinkedListInsertOrdered", CUnit_test_ISM_ll_insert_ordered },
        { "LinkedList2Array", CUnit_test_ISM_ll_2_array },
        { "LinkedArray2List", CUnit_test_ISM_array_2_ll },
        { "LinkedListRemove", CUnit_test_ISM_ll_remove },
       CU_TEST_INFO_NULL
  };

