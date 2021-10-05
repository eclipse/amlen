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
/// @file  engineHashSet.c
/// @brief Engine component hash set functions
//****************************************************************************
#define TRACE_COMP Engine

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>

#include "engineInternal.h"
#include "engineHashSet.h"
#include "memHandler.h"

//These sizes are good primes for hash table sizes based on:
// http://planetmath.org/goodhashtableprimes
static uint64_t ieut_hashSetCapacities[] = {193, 3079, 49157, 393241, 3145739};
#define ieutHASHSET_RESIZE_LOAD_FACTOR 1.2

static int32_t ieut_resizeHashSet(ieutThreadData_t *pThreadData, ieutHashSet_t *set);

//****************************************************************************
/// @brief Generate a hash value from a 64-bit key (e.g. a pointer)
///
/// @param[in]     value   The value for which to generate a hash
///
/// @remark Previously the hash function used the "64-bit Mix Function" from the following URL
///
/// http://web.archive.org/web/20111228030130/http://www.concentric.net/~Ttwang/tech/inthash.htm
///
/// value = (~value) + (value << 21);
/// value = value ^ (value >> 24);
/// value = (value + (value << 3)) + (value << 8);
/// value = value ^ (value >> 14);
/// value = (value + (value << 2)) + (value << 4);
/// value = value ^ (value >> 28);
/// value = value + (value << 31);
/// return value;
///
/// @return generated hash value.
//****************************************************************************
static inline __attribute__((always_inline)) uint64_t ieut_generateHashSetValueHash(uint64_t value)
{
    return value;
}

//****************************************************************************
/// @brief Create a hash set
///
/// @param[in]     pThreadData Thread data for the current thread
/// @param[in]     memType     The iemem_memoryType that this hash set contains
/// @param[out]    set         Allocated set
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieut_createHashSet(ieutThreadData_t *pThreadData,
                           iemem_memoryType memType,
                           ieutHashSet_t **set)
{
    int32_t rc = OK;
    ieutHashSet_t *localSet = NULL;
    ieutHashSetChain_t *localChains = NULL;

    ieutTRACEL(pThreadData, memType, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    localSet = iemem_calloc(pThreadData,
                            IEMEM_PROBE(memType, 60100),
                            1, sizeof(ieutHashSet_t));

    if (NULL == localSet)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Assume the smallest capacity to start
    localSet->capacity = ieut_hashSetCapacities[0];
    localSet->resizeAtCount = localSet->capacity * ieutHASHSET_RESIZE_LOAD_FACTOR;

    localChains = iemem_calloc(pThreadData,
                               IEMEM_PROBE(memType, 60101),
                               1, localSet->capacity * sizeof(ieutHashSetChain_t));

    if (NULL == localChains)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    localSet->chains = localChains;
    localSet->memType = memType;

    *set = localSet;

mod_exit:

    if (rc != OK)
    {
        if (localSet) iemem_free(pThreadData, memType, localSet);
        if (localChains) iemem_free(pThreadData, memType, localChains);
        localSet = NULL;
    }

    ieutTRACEL(pThreadData, localSet,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Remove all current entries from a hash set but leave the set
///        ready for new additions.
///
/// @param[in]     pThreadData Thread data for the current thread
/// @param[in]     set         Set to be cleared
//****************************************************************************
void ieut_clearHashSet(ieutThreadData_t *pThreadData, ieutHashSet_t *set)
{
    int32_t chain;

    ieutTRACEL(pThreadData, set, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    for(chain=0;chain<set->capacity;chain++)
    {
        if (NULL != set->chains[chain].entries)
        {
            iemem_free(pThreadData, set->memType, set->chains[chain].entries);
            set->chains[chain].entries = NULL;
            set->chains[chain].size = 0;
        }

        set->chains[chain].count = 0;
    }

    set->totalCount = 0;

    ieutTRACEL(pThreadData, set, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief Visit a call-back function with all current values in a hash set
///
/// @param[in]     pThreadData Thread data for the current thread
/// @param[in]     set         Set to be traversed
/// @param[in]     callback    Callback routine to call
/// @param[in]     context     Context information for the callback routine
//****************************************************************************
void ieut_traverseHashSet(ieutThreadData_t *pThreadData,
                          ieutHashSet_t *set,
                          ieutHashSet_TraverseCallback_t callback,
                          void *context)
{
    ieutTRACEL(pThreadData, set, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (set->totalCount != 0)
    {
        int32_t chain;

        for(chain=0;chain<set->capacity;chain++)
        {
            if (set->chains[chain].count > 0)
            {
                int32_t index = set->chains[chain].count;
                ieutHashSetEntry_t *entry = set->chains[chain].entries;

                while(index > 0)
                {
                    callback(pThreadData, entry->value, context);
                    entry++;
                    index--;
                }
            }
        }
    }

    ieutTRACEL(pThreadData, context, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief Resize a hash set to the next capacity
///
/// @param[in]     set         Set to be resized
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static int32_t ieut_resizeHashSet(ieutThreadData_t *pThreadData, ieutHashSet_t *set)
{
    int32_t rc = OK;

    uint64_t newCapacity = UINT64_MAX;

    for(size_t i=0; i<(sizeof(ieut_hashSetCapacities)/sizeof(ieut_hashSetCapacities[0])); i++)
    {
        if (set->capacity == ieut_hashSetCapacities[i])
        {
            if (i < (sizeof(ieut_hashSetCapacities)/sizeof(ieut_hashSetCapacities[0])-1))
            {
                newCapacity = ieut_hashSetCapacities[i+1];
            }
            break;
        }
    }

    // We are at the maximum resize limit allowed, so return OK, but stop any further resizing
    if (newCapacity == UINT64_MAX)
    {
        set->resizeAtCount = UINT64_MAX;
        goto mod_exit;
    }

    ieutHashSetChain_t *newChains = iemem_calloc(pThreadData,
                                                 IEMEM_PROBE(set->memType, 60102),
                                                 1, newCapacity * sizeof(ieutHashSetChain_t));

    if (NULL == newChains)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    ieutHashSet_t newSet;

    newSet.capacity = newCapacity;
    newSet.resizeAtCount = newCapacity * ieutHASHSET_RESIZE_LOAD_FACTOR;
    newSet.totalCount = 0;
    newSet.chains = newChains;
    newSet.memType = set->memType;

    if (set->totalCount != 0)
    {
        int chainIndex;

        for(chainIndex=0; chainIndex<set->capacity; chainIndex++)
        {
            if (set->chains[chainIndex].count > 0)
            {
                int32_t entryIndex = set->chains[chainIndex].count;
                ieutHashSetEntry_t *oldEntry = set->chains[chainIndex].entries;

                while(entryIndex > 0)
                {
                    rc = ieut_addValueToHashSet(pThreadData, &newSet, oldEntry->value);

                    if (rc != OK) goto mod_exit;

                    oldEntry++;
                    entryIndex--;
                }
            }

            if (NULL != set->chains[chainIndex].entries)
            {
                iemem_free(pThreadData, set->memType, set->chains[chainIndex].entries);
            }
        }

        iemem_free(pThreadData, set->memType, set->chains);
    }

    set->capacity = newSet.capacity;
    set->resizeAtCount = newSet.resizeAtCount;
    set->totalCount = newSet.totalCount;
    set->chains = newChains;

mod_exit:

    if (rc != OK && NULL != newChains)
    {
        iemem_free(pThreadData, set->memType, newChains);
    }

    return rc;
}

//****************************************************************************
/// @brief Add a value to the hash set.
///
/// @param[in]     set         Set to add the value to
/// @param[in]     value       Value to add
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieut_addValueToHashSet(ieutThreadData_t *pThreadData,
                               ieutHashSet_t *set,
                               uint64_t value)
{
    int32_t rc = OK;
    uint64_t valueHash = ieut_generateHashSetValueHash(value);
    ieutHashSetChain_t *chain;
    ieutHashSetEntry_t *entry;
    bool existing;

    // Check for an existing entry in this chain.
    int32_t index;

    while(1)
    {
        existing = false;
        chain = &set->chains[valueHash%set->capacity];

        int32_t end=chain->count;

        index=0;
        while(index != end)
        {
            int32_t mid = index+((end-index)/2);
            uint64_t entryValue = chain->entries[mid].value;

            if (entryValue == value)
            {
                existing = true;
                end = index = mid;
            }
            else if (entryValue > value)
            {
                end = mid;
            }
            else
            {
                index = mid + 1;
            }
        }

        // Resize the hash set if it's getting full
        if (!existing && (set->totalCount > set->resizeAtCount))
        {
            rc = ieut_resizeHashSet(pThreadData, set);

            if (rc != OK) goto mod_exit;
        }
        else
        {
            break;
        }
    }

    // Did not find the value, so add it in.
    if (!existing)
    {
        // Resize the chain if required
        if (chain->count == chain->size)
        {
            int32_t newSize;

            if (chain->size == 0)
            {
                newSize = 2; // Assume that we will have few collisions
            }
            else
            {
                newSize = chain->size * 2;
            }

            ieutHashSetEntry_t *newEntries = iemem_realloc(pThreadData,
                                                           IEMEM_PROBE(set->memType, 60103),
                                                           chain->entries, newSize * sizeof(ieutHashSetEntry_t));

            if (NULL == newEntries)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            chain->entries = newEntries;
            chain->size = newSize;
        }

        if (index < chain->count)
        {
            memmove(&(chain->entries[index+1]),
                    &(chain->entries[index]),
                    (chain->count-index)*sizeof(ieutHashSetEntry_t));
        }

        chain->count++;
        set->totalCount++;

        entry = &chain->entries[index];

        entry->value = value;
    }

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief Find a value in a hash set
///
/// @param[in]     set      Set to find value in
/// @param[in]     value    Value to find
///
/// @return OK if the value is found, ISMRC_NotFound if the value is not
///         found or an ISMRC_ value.
//****************************************************************************
int32_t ieut_findValueInHashSet(ieutHashSet_t *set, uint64_t value)
{
    int32_t rc = ISMRC_NotFound;
    uint64_t valueHash = ieut_generateHashSetValueHash(value);
    ieutHashSetChain_t *chain = &set->chains[valueHash%set->capacity];

    int32_t index=0;
    int32_t end = chain->count;
    while(index != end)
    {
        int32_t mid = index+((end-index)/2);
        uint64_t entryValue = chain->entries[mid].value;

        if (entryValue == value)
        {
            rc = OK;
            break;
        }
        else if (entryValue > value)
        {
            end = mid;
        }
        else
        {
            index = mid + 1;
        }
    }

    return rc;
}

//****************************************************************************
/// @brief Remove a specific value from the hash set
///
/// @param[in]     set     Set to remove the value from
/// @param[in]     value   Value to be removed from the table
///
/// @remark Note, if the value specified is not found in the set there is no
///         failure returned.
//****************************************************************************
void ieut_removeValueFromHashSet(ieutHashSet_t *set, uint64_t value)
{
    uint64_t valueHash = ieut_generateHashSetValueHash(value);
    ieutHashSetChain_t *chain = &set->chains[valueHash%set->capacity];

    int32_t index = 0;
    int32_t end = chain->count;
    while(index != end)
    {
        int32_t mid = index+((end-index)/2);
        uint64_t entryValue = chain->entries[mid].value;

        if (entryValue == value)
        {
            chain->count--;
            set->totalCount--;
            memmove(&chain->entries[mid], &chain->entries[mid+1], (chain->count-mid)*sizeof(ieutHashSetEntry_t));
            break;
        }
        else if (entryValue > value)
        {
            end = mid;
        }
        else
        {
            index = mid + 1;
        }
    }

    return;
}

//****************************************************************************
/// @brief Destroy a hash table in use by the topic tree, freeing all
///        associated storage.
///
/// @param[in]     pThreadData Thread data for the current thread
/// @param[in]     table  Table to be destroyed
//****************************************************************************
void ieut_destroyHashSet(ieutThreadData_t *pThreadData, ieutHashSet_t *set)
{
    ieutTRACEL(pThreadData, set, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Start out by clearing the entire table
    ieut_clearHashSet(pThreadData, set);

    // Free the chains
    if (set->chains)
    {
        iemem_free(pThreadData, set->memType, set->chains);
    }

    iemem_free(pThreadData, set->memType, set);

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

#if 0
#define ieutNUM_CHAINLENGTHBOUNDARIES 15
void ieut_checkHashSet(ieutHashSet_t *set)
{
    uint32_t chainLengthBoundaries[ieutNUM_CHAINLENGTHBOUNDARIES]={1,2,3,4,5,6,7,8,9,10,20,30,50,1000};
    uint32_t chainCounts[ieutNUM_CHAINLENGTHBOUNDARIES+1]={0};

    for(uint32_t chainNum = 0; chainNum < set->capacity; chainNum++)
    {
        uint64_t prev = 0;
        uint32_t chainLength = set->chains[chainNum].count;

        // Check the chain is ordered correctly
        for(int32_t x=0; x<chainLength; x++)
        {
            if (set->chains[chainNum].entries[x].value < prev)
            {
                printf("ERROR: Chain out of order.\n");
            }
        }

        int i=0;
        for(i=0; i< ieutNUM_CHAINLENGTHBOUNDARIES; i++)
        {
            if(chainLength < chainLengthBoundaries[i])
            {
                chainCounts[i]++;
                break;
            }
        }
        if(i == ieutNUM_CHAINLENGTHBOUNDARIES)
        {
            /* We weren't less than any boundary */
            chainCounts[ieutNUM_CHAINLENGTHBOUNDARIES]++;
        }
    }

    for(int32_t i=0; i< ieutNUM_CHAINLENGTHBOUNDARIES; i++)
    {
        printf("Num chains of length < %u = %u\n", chainLengthBoundaries[i], chainCounts[i]);
    }
    printf("Number of longer chains is %u\n", chainCounts[ieutNUM_CHAINLENGTHBOUNDARIES]);
}
#endif
