/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

#ifndef  H_Array_C
#define  H_Array_C

#include <ismutil.h>
#include <stdio.h>
#include <assert.h>

#define FREE(x)       if (x != NULL) { ism_common_free(ism_memory_utils_array,x); x = NULL; }
static int arrayLockType = 1;
struct ismUtilsArray_t
{
  uintptr_t *              elements;
  pthread_mutex_t          mutex;
  pthread_spinlock_t       lock;
  uint32_t                 capacity;
  uint32_t                 nelements;
};

XAPI void ism_ArrayInit(void) {
    if(ism_common_getBooleanConfig("UseSpinLocks", 0))
        arrayLockType = 1;
    else
        arrayLockType = 0;
}

static inline void arrayLock(ismArray_t array) {
    if(arrayLockType)
        pthread_spin_lock(&array->lock);
    else
        pthread_mutex_lock(&array->mutex);
}

static inline void arrayUnlock(ismArray_t array) {
    if(arrayLockType)
        pthread_spin_unlock(&array->lock);
    else
        pthread_mutex_unlock(&array->mutex);
}

/*
 * Create Array object
 * Call ism_common_destroyArray to delete the HashMap object which return by
 * this function.
 * @param capacity - capacity for the list
 * @return   pointer to ismArray_t object
 */
XAPI ismArray_t ism_common_createArray(uint32_t capacity) {
    ismArray_t array;
    uint32_t i;
    if ((array = (ismArray_t) ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_array,250),sizeof(struct ismUtilsArray_t)+(capacity*sizeof(uintptr_t)))) == NULL) {
        return (NULL);
    }
    array->capacity = capacity;
    array->nelements = 0;
    array->elements = (uintptr_t*)(array+1);
    pthread_spin_init(&array->lock, 0);
    pthread_mutex_init(&array->mutex,NULL);
    for(i = 0; i < (capacity-1); i++) {
        uintptr_t freeHead = i+1;
        array->elements[i] = (freeHead << 1) | 1;
    }
    array->elements[i] = 1;
    return (array);
}
/**
 * Destroy Array object
 * @param array the Array object
 */
XAPI void ism_common_destroyArray(ismArray_t array) {
    ism_common_destroyArrayAndFreeValues(array, NULL);
}
XAPI void ism_common_destroyArrayAndFreeValues(ismArray_t array, ism_freeValueObject freeCB) {
    if (!array || !array->elements) {
        return;
    }
    if(freeCB){
        int i;
        for (i = 1; i < array->capacity; i++) {
            if(array->elements[i] & 1)
                continue;
            freeCB((void*)(array->elements[i]));
        }
    }
    pthread_spin_destroy(&(array->lock));
    pthread_mutex_destroy(&array->mutex);
    ism_common_free(ism_memory_utils_array,array);
}
/*
 * Put an object into the list
 * @param array - Array to put object to
 * @param object
 * @return  A index of array entry where object was put to. 0=failure (array is full).
 */
XAPI uint32_t ism_common_addToArray(ismArray_t array, void * object) {
    uint32_t freeHead = (uint32_t)(array->elements[0] >> 1);
    uintptr_t value = (uintptr_t) object;
    assert((value & 1) == 0);
    if(freeHead) {
        array->elements[0] = array->elements[freeHead];
        array->elements[freeHead] = value;
        array->nelements++;
        return freeHead;
    }
    return 0;
}
/*
 * Put an object into the list. Take lock first
 * @param array - Array to put object to
 * @param object
 * @return  A index of array entry where object was put to. 0=failure (array is full).
 */
XAPI uint32_t ism_common_addToArrayLock(ismArray_t array, void * object) {
    uint32_t result = 0;
    arrayLock(array);
    result = ism_common_addToArray(array, object);
    arrayUnlock(array);
    return result;
}

static void * getArrayElement(ismArray_t array, uint32_t index, int remove) {
    if(array && index && (array->capacity > index) && ((array->elements[index] & 1) == 0))  {
        void * object = (void*)(array->elements[index]);
        if(remove) {
            uintptr_t freeHead = index;
            array->elements[index] = array->elements[0];
            array->elements[0] = (freeHead << 1) | 1;
            array->nelements--;
        }
        return object;
    }
    return NULL;
}


/*
 * Get an element from the array
 * @param array
 * @param index
 * @return the element
 */
XAPI void * ism_common_getArrayElement(ismArray_t array, uint32_t index) {
    return getArrayElement(array, index,0);
}


 /*
 * Get an element from the array. Take lock first.
 * @param array
 * @param index
 * @return the element
 */
XAPI void * ism_common_getArrayElementLock(ismArray_t array, uint32_t index) {
    void * result = NULL;
    arrayLock(array);
    result = getArrayElement(array, index,0);
    arrayUnlock(array);
    return result;
}

/*
 * Remove an element from the array
 * @param array
 * @param index
 * @return the element
 */
XAPI void * ism_common_removeArrayElement(ismArray_t array, uint32_t index) {
    return getArrayElement(array, index,1);
}

/*
* Get an element from the array. Take lock first.
 * @param array
* @param index
* @return the element
*/
XAPI void * ism_common_removeArrayElementLock(ismArray_t array, uint32_t index) {
   void * result = NULL;
   arrayLock(array);
   result = getArrayElement(array, index,1);
   arrayUnlock(array);
   return result;
}

/*
 * Lock Array object
 * @param array - Array object
 */
XAPI void ism_common_ArrayLock(ismArray_t array) {
    arrayLock(array);
}

/*
 * Unlock Array object
 * @param array - Array object
 */
XAPI void ism_common_ArrayUnlock(ismArray_t array) {
    arrayUnlock(array);
}

/*
 * Get number of elements in array
 * @param array - Array object
 * @return number of elements in array
 */
XAPI int ism_common_getArrayNumElements(const ismArray_t array) {
    return array->nelements;
}

#endif
