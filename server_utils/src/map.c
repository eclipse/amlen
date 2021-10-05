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

#ifndef  H_ISMMAP_C
#define  H_ISMMAP_C

#include <ismutil.h>
#include <stdio.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif


#define FNV_OFFSET_BASIS_32 0x811C9DC5
#define FNV_PRIME_32 0x1000193

#define FREE(x)       if (x != NULL) { ism_common_free(ism_memory_utils_misc,x); x = NULL; }
//#define MAP_DEBUG
typedef uint32_t (* hash_func_t)(const void * in, size_t *lens);
struct ismHashMap_t
{
  hash_func_t              hashFunc;
  ismHashMapEntry      **  elements;
  pthread_mutex_t          mutex;
  pthread_spinlock_t       lock;
  uint32_t                 mask;
  uint32_t                 capacity;
  uint32_t                 size;
  uint32_t                 nelements;
#ifdef MAP_DEBUG
  double                   lastLockTime;
  uint64_t                 numAdds;
  uint64_t                 numRems;
  uint64_t                 numGets;
  uint64_t                 numLocks;
  double                   timeToAdd;
  double                   timeToRemove;
  double                   timeToGet;
  double                   timeToLock;
  double                   lockTime;
  double                   maxLockTime;
  uint64_t                 maxChain;
#endif
} ;

static int poolLockType = 0;

void ism_hashMapInit(void) {
    if(ism_common_getBooleanConfig("UseSpinLocks", 0))
        poolLockType = 1;
    else
        poolLockType = 0;
}

static inline void mapLock(ismHashMap *map) {
    if(poolLockType)
        pthread_spin_lock(&map->lock);
    else
        pthread_mutex_lock(&map->mutex);
}

static inline void mapUnlock(ismHashMap *map) {
    if(poolLockType)
        pthread_spin_unlock(&map->lock);
    else
        pthread_mutex_unlock(&map->mutex);
}

static inline uint32_t memhash_int32(const void * in, size_t *len) {
    uint32_t hash = *((uint32_t*)in);
    hash = hash<<7 | hash>>25;    /* rotate left 7 bit */
    return hash/127;
}
static inline uint32_t memhash_int64(const void * in, size_t *len) {
    uint32_t hash;
    uint64_t val = *((uint64_t*)in);
    val = val<<7 | val>>57;       /* rotate left 7 bits */
    hash = (uint32_t) ((val/127) & 0xFFFFFFFFUL);
    return hash;
}

static uint32_t memhash_string(const void * in, size_t *len) {
    uint32_t hash = FNV_OFFSET_BASIS_32;
    uint8_t * in8 = (uint8_t *) in;
    size_t length = 0;
    uint8_t b;
    do{
        b = *(in8++);
        hash ^= b;
        hash *= FNV_PRIME_32;
        length++;
    }while(b);
    *len = length;
    return hash;

}
static uint32_t memhash_byteArray(const void * in, size_t *len) {
    uint32_t hash = FNV_OFFSET_BASIS_32;
    uint8_t * in8 = (uint8_t *) in;
    size_t length = *len;
    while (length--) {
        hash ^= *(in8++);
        hash *= FNV_PRIME_32;
    }
    return hash;
}


/*********************************************************/
/*-------         Hash map data structure        --------*/
/*********************************************************/
static void HM_resize_map(ismHashMap *hash_map) {
    int index, i;

    int prev_capacity;
    ismHashMapEntry **elements, *elm, *pelm;
    if (hash_map->capacity > 0xFFFFFF)
        return;

    if ((elements = (ismHashMapEntry **)ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,135),2 * hash_map->capacity, sizeof(ismHashMapEntry *))) == NULL) {
        return;
    }
    prev_capacity = hash_map->capacity;
    hash_map->capacity *=2;
    hash_map->mask = hash_map->capacity-1;

    for (i = 0, hash_map->size = 0; i < prev_capacity; i++) {
        if ((elm = hash_map->elements[i])) {
            while (elm) {
                pelm = elm;
                elm = elm->next;
                /* Need to rehash due the capacity change map */
                index = pelm->hash_code & hash_map->mask;

                pelm->next = elements[index];
                elements[index] = pelm;

                if (!pelm->next) {
                    hash_map->size++;
                }
            }
        }
    }

    ism_common_free(ism_memory_utils_misc,hash_map->elements);
    hash_map->elements = elements;
}

XAPI uint32_t ism_common_computeHashCode(const char * ptr, size_t length) {
    size_t myLen = length;
    if(myLen) {
        return memhash_byteArray(ptr, &myLen);
    }
    return memhash_string(ptr, &myLen);
}

/*
 * Create HashMap object
 * Call ism_common_destroyHashMap to delete the HashMap object which return by
 * this function.
 * @param capacity initial capacity for the hash map
 * @return   ismHashMap object
 */
XAPI ismHashMap *ism_common_createHashMap(uint32_t capacity, ismHashFunctionType_t hashType) {
    ismHashMap *hash_map;

    if ((hash_map = (ismHashMap *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,137),1, sizeof(ismHashMap))) == NULL) {
        return (NULL);
    }
    if (capacity < 0x1000000) {
        hash_map->capacity = 1;
        do {
            hash_map->capacity *= 2;
        } while (hash_map->capacity < capacity);
    } else {
        capacity = 0x1000000;
        hash_map->capacity = 0x1000000;
    }

    if ((hash_map->elements = (ismHashMapEntry **) ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,138),hash_map->capacity, sizeof(ismHashMapEntry *))) == NULL) {
        FREE(hash_map);
        return (NULL);
    }
    hash_map->size = hash_map->nelements = 0;
    hash_map->mask = hash_map->capacity - 1;
    pthread_spin_init(&hash_map->lock, 0);
    pthread_mutex_init(&hash_map->mutex,NULL);
    switch(hashType){
    case HASH_INT32:
        hash_map->hashFunc = memhash_int32;
        break;
    case HASH_INT64:
        hash_map->hashFunc = memhash_int64;
        break;
    case HASH_STRING:
        hash_map->hashFunc = memhash_string;
        break;
    default:
        hash_map->hashFunc = memhash_byteArray;
        break;
    }
    return (hash_map);
}
/**
 * Destroy HashMap object
 * @param hash_map the HashMap object
 */
XAPI void ism_common_destroyHashMap(ismHashMap *hash_map) {
	ism_common_destroyHashMapAndFreeValues(hash_map, NULL);
}
XAPI void ism_common_destroyHashMapAndFreeValues(ismHashMap * hash_map, ism_freeValueObject freeCB) {
    int i;
    ismHashMapEntry *lle, *lle_next;

    if (!hash_map || !hash_map->elements) {
        return;
    }

    for (i = 0; i < hash_map->capacity; i++) {
        lle = hash_map->elements[i];
        while (lle) {
            lle_next = lle->next;
            if(lle->value && freeCB)
            	freeCB(lle->value);
            ism_common_free(ism_memory_utils_misc,lle);
            lle = lle_next;
        }
    }
    FREE(hash_map->elements);
    pthread_spin_destroy(&(hash_map->lock));
    pthread_mutex_destroy(&hash_map->mutex);
#ifdef MAP_DEBUG
    TRACE(6, "Destroying hashMap: map=%p capacity=%d maxChainSize=%llu putStats=(%llu %g) getStats=(%llu %g), delStats=(%llu %g) lockStats=(%llu %g %g %g) \n",
            hash_map, hash_map->capacity, hash_map->maxChain, hash_map->numAdds, hash_map->timeToAdd,
            hash_map->numGets, hash_map->timeToGet, hash_map->numRems, hash_map->timeToRemove,
            hash_map->numLocks, hash_map->timeToLock, hash_map->lockTime, hash_map->maxLockTime);
#endif
    FREE(hash_map);
}
/**
 * Put an element into the map
 * @param hash_map the HashMap object
 * @param key the key in the map
 * @param key_len lenghth for the key
 * @param retvalue the previous value for the key if any.
 * @return  A return code, 0=good.
 */
XAPI int ism_common_putHashMapElement(ismHashMap *hash_map, const void *key, int key_len, void *value, void **retvalue) {
    uint32_t index;
    double ratio;
    void * previous_value = NULL;
    int found = 0;
    ismHashMapEntry *lle, *new_lle, *lastlle;
    size_t keyLen = key_len;
    uint32_t hash_code;
#ifdef MAP_DEBUG
    double t1 = ism_common_readTSC();
    int chainSize = 1;
#endif

    if (__builtin_expect((!hash_map || !key || key_len < 0 || !value),0)) {
        return (-1);
    }

    hash_code = hash_map->hashFunc(key, &keyLen);
    index = hash_code & hash_map->mask;

    if ((new_lle = (ismHashMapEntry *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,140),sizeof(ismHashMapEntry) + keyLen)) == NULL) {
        return (-1);
    }

    new_lle->key = (char*) (new_lle + 1);

    memcpy(new_lle->key, key, keyLen);
    new_lle->key_len = keyLen;
    new_lle->value = value;
    new_lle->next = NULL;
    new_lle->hash_code = hash_code;

    if ((lle = hash_map->elements[index])) {

        while (lle) {
            /*Check if the key is the same. Then just replace the value.*/
            if (__builtin_expect((lle->key_len == keyLen), 1)) {
                if (keyLen == 8) {
                    int64_t k1 = *((int64_t*) key);
                    int64_t k2 = *((int64_t*) lle->key);
                    found = (k1 == k2);
                } else {
                    if (keyLen == 4) {
                        int k1 = *((int*) key);
                        int k2 = *((int*) lle->key);
                        found = (k1 == k2);
                    } else {
                        found = (memcmp(key, lle->key, keyLen) == 0);
                    }
                }
            }
            if (__builtin_expect(found, 0)) {
                previous_value = lle->value;
                lle->value = value;
                FREE(new_lle);
                break;
            }
            lastlle = lle;
            lle = lle->next;
#ifdef MAP_DEBUG
            chainSize++;
#endif
        }

        /*If the existing element is not found, put it at the end of the list*/
        if (!found) {
            lastlle->next = new_lle;
        }
    } else {
        hash_map->elements[index] = new_lle;
        hash_map->size++;
    }
    if (found == 0) {
        hash_map->nelements++;
#ifdef MAP_DEBUG
        if (hash_map->maxChain < chainSize)
            hash_map->maxChain = chainSize;
#endif
    }

    if (1.6 * hash_map->size > hash_map->capacity) {
        ratio = (double) hash_map->nelements / hash_map->size;
        if (ratio > 1.6) {
            HM_resize_map(hash_map);
        }
    }
    if (retvalue != NULL) {
        *retvalue = previous_value;
    }
#ifdef MAP_DEBUG
    hash_map->timeToAdd += (ism_common_readTSC() - t1);
    hash_map->numAdds++;
#endif
    return 0;
}

static void *getHashMapElement(ismHashMap *hash_map, const void *key, int key_len, int remove) {
    uint32_t index;
    ismHashMapEntry *lle, *lle_prev = NULL;
    void *value = NULL;
    size_t keyLen = key_len;
    uint32_t hash_code;
    if (__builtin_expect((!hash_map || !key || key_len < 0 || hash_map->nelements == 0),0)) {
        return (NULL);
    }

    hash_code = hash_map->hashFunc(key, &keyLen);
    index = hash_code & hash_map->mask;

    for (lle = hash_map->elements[index]; lle; lle = lle->next) {
        int found = 0;
        if (__builtin_expect(((lle->key_len == keyLen)&&(lle->hash_code == hash_code)), 1)) {
            if (keyLen == 8) {
                int64_t k1 = *((int64_t*) key);
                int64_t k2 = *((int64_t*) lle->key);
                found = (k1 == k2);
            } else {
                if (keyLen == 4) {
                    int k1 = *((int*) key);
                    int k2 = *((int*) lle->key);
                    found = (k1 == k2);
                } else {
                    found = (memcmp(key, lle->key, keyLen) == 0);
                }
            }
        }

        if (found) {
            value = lle->value;
            if (remove) {
                if (lle_prev) {
                    lle_prev->next = lle->next;
                } else {
                    if ((hash_map->elements[index] = lle->next) == NULL) {
                        hash_map->size--;
                    }
                }
                FREE(lle);
                hash_map->nelements--;
            }
            break;
        }
        lle_prev = lle;
    }

    return value;
}


/*
 * Get an element from the hash map base on the input key
 * @param hash_map the HashMap object
 * @param key the key in the map
 * @param key_len length for the key
 * @return the element
 */
void *ism_common_getHashMapElement(ismHashMap *hash_map, const void *key, int key_len) {
#ifndef MAP_DEBUG
    return getHashMapElement(hash_map, key, key_len, 0);
#else
    void * result;
    double t1 = ism_common_readTSC();
    result = getHashMapElement(hash_map, key, key_len, 0);
    hash_map->timeToGet += (ism_common_readTSC() - t1);
    hash_map->numGets++;
    return result;
#endif
}

/*
 * Remove the HashMap element based on the key.
 * @param hash_map the HashMap object
 * @param key the key in the map
 * @param key_len lenghth for the key
 * @return the element
 */
void *ism_common_removeHashMapElement(ismHashMap *hash_map, const void *key, int key_len) {
#ifndef MAP_DEBUG
    return getHashMapElement(hash_map, key, key_len, 1);
#else
    void * result;
    double t1 = ism_common_readTSC();
    result = getHashMapElement(hash_map, key, key_len, 1);
    hash_map->timeToRemove += (ism_common_readTSC() - t1);
    hash_map->numRems++;
    return result;
#endif
}

/*
 * Put an element into the map with lock
 * @param hash_map the HashMap object
 * @param key the key in the map
 * @param key_len lenghth for the key
 * @param retvalue the previous value for the key if any.
 * @return  A return code, 0=good.
 */
int ism_common_putHashMapElementLock(ismHashMap *hash_map, const void *key, int key_len, void *value, void **retvalue) {
    int retValue;
#ifndef MAP_DEBUG
    mapLock(hash_map);
    retValue = ism_common_putHashMapElement(hash_map, key, key_len, value, retvalue);
    mapUnlock(hash_map);
#else
    double t1 = ism_common_readTSC();
    double t2;
    mapLock(hash_map);
    t2 = ism_common_readTSC();
    hash_map->timeToLock += (t2 - t1);
    hash_map->numLocks++;
    retValue = ism_common_putHashMapElement(hash_map, key, key_len, value, retvalue);
    t1 = ism_common_readTSC() - t2;
    hash_map->lockTime += t1;
    if (t1 > hash_map->maxLockTime)
        hash_map->maxLockTime = t1;
    mapUnlock(hash_map);
#endif
    return retValue;
}


/*
 * Lock HashMap object
 * @param hash_map the HashMap object
 */
void ism_common_HashMapLock(ismHashMap * hash_map) {
#ifdef MAP_DEBUG
    double t = ism_common_readTSC();
#endif
    mapLock(hash_map);
#ifdef MAP_DEBUG
    hash_map->lastLockTime = ism_common_readTSC();
    hash_map->timeToLock += (hash_map->lastLockTime - t);
    hash_map->numLocks++;
#endif
}


/*
 * Unlock HashMap object
 * @param hash_map the HashMap object
 */
void ism_common_HashMapUnlock(ismHashMap * hash_map) {
#ifdef MAP_DEBUG
    double t = ism_common_readTSC() - hash_map->lastLockTime;
    hash_map->lockTime += t;
    hash_map->lastLockTime = 0;
    if (t > hash_map->maxLockTime)
        hash_map->maxLockTime = t;

#endif
    mapUnlock(hash_map);
}


/*
 * Get an element from the hash map base on the input key with lock
 * @param hash_map the HashMap object
 * @param key the key in the map
 * @param key_len lenghth for the key
 * @return the element
 */
void * ism_common_getHashMapElementLock(ismHashMap *hash_map, const void *key, int key_len) {
    void * retValue = NULL;
#ifndef MAP_DEBUG
    mapLock(hash_map);
    retValue = ism_common_getHashMapElement(hash_map, key, key_len);
    mapUnlock(hash_map);
#else
    double t1 = ism_common_readTSC();
    double t2;
    mapLock(hash_map);
    t2 = ism_common_readTSC();
    hash_map->timeToLock += (t2 - t1);
    hash_map->numLocks++;
    retValue = ism_common_getHashMapElement(hash_map, key, key_len);
    t1 = ism_common_readTSC() - t2;
    hash_map->lockTime += t1;
    if (t1 > hash_map->maxLockTime)
        hash_map->maxLockTime = t1;
    mapUnlock(hash_map);
#endif
    return retValue;
}


/*
 * Remove the HashMap element based on the key with lock
 * @param hash_map the HashMap object
 * @param key the key in the map
 * @param key_len lenghth for the key
 * @return the element
 */
void * ism_common_removeHashMapElementLock(ismHashMap *hash_map, const void *key, int key_len) {
    void * retValue = NULL;
#ifndef MAP_DEBUG
    mapLock(hash_map);
    retValue = ism_common_removeHashMapElement(hash_map, key, key_len);
    mapUnlock(hash_map);
#else
    double t1 = ism_common_readTSC();
    double t2;
    mapLock(hash_map);
    t2 = ism_common_readTSC();
    hash_map->timeToLock += (t2 - t1);
    hash_map->numLocks++;
    retValue = ism_common_removeHashMapElement(hash_map, key, key_len);
    t1 = ism_common_readTSC();
    hash_map->lockTime += (t1-t2);
    mapUnlock(hash_map);
#endif

    return retValue;
}

/*
 * Get Hash map Size
 * @param hash_map the HashMap object
 * @return the size of the hashmap
 */
int ism_common_getHashMapSize(const ismHashMap *hash_map) {
    return hash_map->size;
}

/*
 * Get Hash map elements number
 * @param hash_map the HashMap object
 * @return the number of the elements in the map
 */
int ism_common_getHashMapNumElements(const ismHashMap *hash_map) {
    return hash_map->nelements;
}


/*
 * Get hash map entries.
 * @param hash_map the HashMap object
 * @return the array with all entries in the map. Last element of the array is (void*)-1;
 */
ismHashMapEntry ** ism_common_getHashMapEntriesArray(ismHashMap * hash_map) {
    ismHashMapEntry ** result = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,141),(hash_map->nelements + 1) * sizeof(ismHashMapEntry*));
    ismHashMapEntry *hme;
    int index;
    int counter = 0;
    for (index = 0; index < hash_map->capacity; index++) {
        hme = hash_map->elements[index];
        while (hme) {
            result[counter++] = hme;
            hme = hme->next;
        }
    }

#ifndef REMOVE_EXTRA_CONSISTENCY_CHECKING
    if ((uint32_t)counter != hash_map->nelements) abort();
#endif

    result[counter] = (void*) -1;
    return result;
}

/*
 * Enumerate the has map entries
 * The invoker must have locked the hash map
 */
int ism_common_enumerateHashMap(ismHashMap * hash_map, hash_enum_f callback, void * context) {
    ismHashMapEntry * hme;
    int i;
    int rc;
    for (i=0; i<hash_map->capacity; i++) {
        hme = hash_map->elements[i];
        while (hme) {
            rc = callback(hme, context);
            if (rc)
                return rc;
            hme = hme->next;
        }
    }
    return 0;
}

/*
 * Free hash map entries array returned by ism_common_getHashMapEntriesArray.
 * @param array
 */
XAPI void ism_common_freeHashMapEntriesArray(ismHashMapEntry **array) {
    FREE(array);
}

#endif
