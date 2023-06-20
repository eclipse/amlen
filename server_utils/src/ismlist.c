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

#include <stdlib.h>
#include <ismutil.h>

/*
 * Initializes this list with the user-specified destroy function
 */
int ism_common_list_init(ism_common_list *list, int synchronized, void (*destroy)(void *data)) {
     list->size = 0;
     list->head = NULL;
     list->tail = NULL;
     list->destroy = destroy;
     if (synchronized) {
      	list->lock = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_list,203),sizeof(pthread_spinlock_t));
    	if (list->lock == NULL)
      		return -1;
       	pthread_spin_init(list->lock,0);
    } else {
     	list->lock = NULL;
    }
    return 0;
}


/*
 * Removes all elements from the list
 */
static void removeAll(ism_common_list *list){
    ism_common_list_node * pNode;
    ism_common_list_node * freeNode;
    pNode = list->head;
    while (pNode){
    	if ((pNode->data) && (list->destroy)){
    		list->destroy(pNode->data);
    	}
    	freeNode = pNode;
    	pNode = freeNode->next;
    	ism_common_free(ism_memory_utils_list,freeNode);
    }
    list->head = list->tail = NULL;
    list->size = 0;
}

/*
 * Removes all elements from the list, applies the user-specified destroy function to each data element (does not free the list)
 */
void ism_common_list_destroy(ism_common_list *list) {
    if (!list)
      return;
    if (list->lock)
    	pthread_spin_lock(list->lock);
    removeAll(list);
    if (list->lock){
    	pthread_spin_unlock(list->lock);
    	pthread_spin_destroy(list->lock);
    	ism_common_free(ism_memory_utils_list,(void*)list->lock);
    	list->lock = (void*)-1;
    }
}

/*
 * Converts a list to an array. Takes a pointer to an array of void pointers.
 * Memory is allocated for the array.
 */
int ism_common_list_to_array(ism_common_list *list, void ***array) {
    int i = 0;
    void ** result;
    ism_common_list_node * pNode;
    if (list->lock)
    	pthread_spin_lock(list->lock);
    if (list->size == 0){
    	*array = NULL;
        if (list->lock)
        	pthread_spin_unlock(list->lock);
    	return 0;
    }
    *array = result = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_list,206),list->size*(sizeof(void *)));
    if (result == NULL) {
        if (list->lock)
        	pthread_spin_unlock(list->lock);
    	return -1;
    }
    for (pNode = list->head; pNode != NULL; pNode = pNode->next){
    	result[i++] = pNode->data;
    }
    if (list->lock)
    	pthread_spin_unlock(list->lock);
    return i;
}

#define INIT_NODE(NODE, DATA)  \
	NODE = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_list,207),1,sizeof(ism_common_list_node)); \
	if (NODE == NULL) \
		return -1; \
	NODE->data = (void *)DATA;
#define INSERT_HEAD(LIST, NODE)  \
		if (LIST->size) { \
			LIST->head->prev = NODE;\
			NODE->next = LIST->head; \
			LIST->head = NODE; \
		} else { \
			LIST->tail = LIST->head = NODE; \
			NODE->next = NULL; \
		} \
		NODE->prev = NULL;\
		LIST->size++;
#define INSERT_TAIL(LIST, NODE)  \
		if (LIST->size) {\
			NODE->prev = LIST->tail; \
			LIST->tail->next = NODE; \
			LIST->tail = NODE; \
		} else { \
			LIST->head = LIST->tail = NODE; \
			NODE->prev = NULL; \
		} \
		NODE->next = NULL; \
		LIST->size++;

/*
 *
 */
static int insertAfter(ism_common_list *list, ism_common_list_node * prevNode, ism_common_list_node * newNode) {
	if (prevNode == NULL){
		INSERT_HEAD(list,newNode);
		return 0;
	}
	if (prevNode == list->tail){
		INSERT_TAIL(list,newNode);
		return 0;
	}
	newNode->next = prevNode->next;
	newNode->prev = prevNode;
	prevNode->next->prev = newNode;
	prevNode->next = newNode;
    list->size++;
    return 0;
}
#define REMOVE_HEAD(LIST, DATA)  \
		if (LIST->size > 0) {	\
			ism_common_list_node * pNode = LIST->head; \
			if (DATA) *DATA = pNode->data; \
			LIST->head = pNode->next; \
			ism_common_free(ism_memory_utils_list,pNode); \
			if (LIST->head) \
				LIST->head->prev = NULL; \
			else \
				LIST->tail = NULL; \
			LIST->size--;\
			rc = 0; \
		} else { \
			if (DATA) *DATA = NULL; \
			rc = -2; \
		}
#define REMOVE_TAIL(LIST, DATA)  \
		if (LIST->size > 0) { \
			ism_common_list_node * pNode = LIST->tail; \
			if (DATA) \
                *DATA = pNode->data; \
			LIST->tail = pNode->prev; \
			ism_common_free(ism_memory_utils_list,pNode); \
			if (LIST->tail) \
			    LIST->tail->next = NULL; \
			else \
			    LIST->head = NULL; \
			LIST->size--; \
			rc = 0; \
		} else { \
			if (DATA) \
                *DATA = NULL; \
			rc = -2; \
		}
/*
* Move elements from one list to another.
* @param list1   A pointer to the list to be merged in
* @param list2   A pointer to the list to be inserted
* @return 0 on success, -1 if memory could not be allocated for new node
*/
int ism_common_list_merge_lists(ism_common_list * list1, ism_common_list * list2) {
    ism_common_list_node * pNode;
    int rc = 0;
    if (!list1 || !list2 || (list1 == list2))
        return rc;
    if (list1->lock)
        pthread_spin_lock(list1->lock);
    if (list2->lock)
        pthread_spin_lock(list2->lock);
    pNode = list2->head;
    while (pNode) {
        ism_common_list_node *mvNode = pNode;
        pNode = pNode->next;
        INSERT_TAIL(list1,mvNode);
    }
    list2->head = list2->tail = NULL;
    list2->size = 0;
    if (list2->lock)
        pthread_spin_unlock(list2->lock);
    if (list1->lock)
        pthread_spin_unlock(list1->lock);
    return rc;
}

/*
 * Converts an array to a list. The list must be empty but initialized.
 * The third argument is the length of the array.
 */
int ism_common_list_from_array(ism_common_list *list, void **array, int size) {
    int retVal = -10;
    if (size < 1)
    	return 0;
    if (list->lock)
    	pthread_spin_lock(list->lock);
    if (!(list->head)){
    	int i;
    	retVal = 0;
    	for (i = 0; i < size; i++){
			ism_common_list_node *newNode = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_list,210),1,sizeof(ism_common_list_node));
			if (!newNode){
				retVal = -1;
				break;
			}
			newNode->data = array[i];
			INSERT_TAIL(list,newNode);
    	}
    }
    if (list->lock)
    	pthread_spin_unlock(list->lock);
    return retVal;
}

/*
 *
 */
XAPI void ism_common_list_array_free(void **array) {
    ism_common_free(ism_memory_utils_list,array);
}

/*
 * Initializes an iterator for the specified list.
 */
XAPI void ism_common_list_iter_init(ism_common_listIterator *iter, ism_common_list *list) {
    iter->list = list;
    if (list->lock)
    	pthread_spin_lock(list->lock);

    ism_common_list_iter_reset(iter);
}

/*
 *
 */
XAPI void ism_common_list_iter_destroy(ism_common_listIterator *iter) {
    if (iter->list->lock)
        pthread_spin_unlock(iter->list->lock);
}

 // Returns a pointer to the next node in the list
ism_common_list_node *ism_common_list_iter_next(ism_common_listIterator *iter) {
    ism_common_list_node *next = iter->currNode;
    if (next) iter->currNode = next->next;
    iter->lastNode = next;
    return next;
}

/*
 * Returns -1 if the list contains no more nodes, otherwise returns 1
 */
int ism_common_list_iter_hasNext(ism_common_listIterator *iter) {
    if (iter->currNode != NULL)
        return 1;
    return 0;
}

/*
 * Inserts data into a linked list as the first element.
 * Inserts data in a new node after the specified node in the list.
 * @param list      A pointer to the list
 * @param data		A pointer to the data to be inserted
 * @return 0 on success, -1 if memory could not be allocated for new node
 */
int ism_common_list_insert_head(ism_common_list *list, const void *data){
    ism_common_list_node *newNode;
    INIT_NODE(newNode,data);
    if (list->lock)
	 	pthread_spin_lock(list->lock);
    INSERT_HEAD(list,newNode);
	if (list->lock)
	   	pthread_spin_unlock(list->lock);
    return 0;
}

/*
 * Inserts data into an ordered linked list.
 * @param list      A pointer to the list
 * @param data		A pointer to the data to be inserted
 * @param comparator
 * @return 0 on success, -1 if memory could not be allocated for new node
 */
int ism_common_list_insert_ordered(ism_common_list *list, const void *data, int (*comparator)(const void *data1, const void *data2)) {
    ism_common_list_node *newNode;
    ism_common_list_node *prevNode = NULL;
    ism_common_list_node *currNode = NULL;
    INIT_NODE(newNode,data);
    if (list->lock)
        pthread_spin_lock(list->lock);
    for (currNode = list->head; currNode != NULL; currNode = currNode->next) {
        if (comparator(data,currNode->data) > 0) {
        	prevNode = currNode;
            continue;
        }
        insertAfter(list,prevNode,newNode);
        if (list->lock)
     	    pthread_spin_unlock(list->lock);
        return 0;
    }
    INSERT_TAIL(list,newNode);
    if (list->lock)
 	 	pthread_spin_unlock(list->lock);
    return 0;
}


/*
 * Inserts data into a linked list as the last element.
 * @param list      A pointer to the list
 * @param data		A pointer to the data to be inserted
 * @return 0 on success, -1 if memory could not be allocated for new node
 */
int ism_common_list_insert_tail(ism_common_list *list, const void *data){
    ism_common_list_node *newNode;
    INIT_NODE(newNode,data);
    if (list->lock)
	 	 pthread_spin_lock(list->lock);
    INSERT_TAIL(list,newNode);
	if (list->lock)
	   	pthread_spin_unlock(list->lock);
	return 0;
}

/*
 * Removes first element from a linked.
 * @param list      A pointer to the list
 * @param data		A pointer to the data to be returned
 * @return 0 on success, -2 if list is empty
 */
int ism_common_list_remove_head(ism_common_list *list, void **data){
    int rc;
    if (list->lock)
	    pthread_spin_lock(list->lock);
    REMOVE_HEAD(list,data);
	if (list->lock)
	    pthread_spin_unlock(list->lock);
	return rc;
}

/*
 * Removes last element from a linked.
 * @param list      A pointer to the list
 * @param data		A pointer to the data to be returned
 * @return 0 on success, -2 if list is empty
 */
int ism_common_list_remove_tail(ism_common_list *list, void **data) {
	int rc;
    if (list->lock)
	 	pthread_spin_lock(list->lock);
    REMOVE_TAIL(list,data);
	if (list->lock)
	   	pthread_spin_unlock(list->lock);
	return rc;
}

/*
 * Resets the iterator to the beginning of its list.
 * @param iter      A pointer to the iterator to reset
 */
void ism_common_list_iter_reset(ism_common_listIterator *iter) {
    iter->currNode = iter->list->head;
	iter->lastNode = (void*) -1;
}

/*
 * Removes from the underlying linked list the last element returned by the
 * ism_common_list_iter_next.  This method can be called only once per
 * call to ism_common_list_iter_next.
 * @param list      A pointer to the list
 * @param node      A pointer to the iterator. If NULL, removes from the head of the list.
 * @param data	   A pointer to direct at the removed data
 * @return 0 on success,-2 if list is empty, -3 if the ism_common_list_iter_next method has not
 *		  yet been called, or this method has already been called after the
 *		  last call to the ism_common_list_iter_next method, or if NULL was returned by the
 *		  last call  to the ism_common_list_iter_next.
 */
int ism_common_list_remove(ism_common_list *list, ism_common_listIterator *iter, void **data){
	int rc;
	ism_common_list_node *node;
	if (iter == NULL){
	    if (list->lock)
		 	pthread_spin_lock(list->lock);
    	REMOVE_HEAD(list,data);
        if (list->lock)
    	 	pthread_spin_unlock(list->lock);
   	    return rc;
    }
	node = iter->lastNode;
	iter->lastNode = (void*)-1;
	if ((node == ((void*)-1)) || (node == NULL))
		return -3;
    if (node == list->head){
    	REMOVE_HEAD(list,data);
   	    return rc;
    }
    if (node == list->tail){
    	REMOVE_TAIL(list,data);
   	    return rc;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    if (data)
        *data = node->data;
    ism_common_free(ism_memory_utils_list,node);
    list->size--;
	return 0;
}

/*
 * Removes all nodes from a linked list.
 * @param list      A pointer to the list
 */
void ism_common_list_clear(ism_common_list *list){
    if (list->lock)
    	pthread_spin_lock(list->lock);
    removeAll(list);
	if (list->lock)
	 	pthread_spin_unlock(list->lock);
}

int ism_common_list_getSize(ism_common_list *list) {
    return list->size;
}
