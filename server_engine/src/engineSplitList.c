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
/// @file  engineSplitList.c
/// @brief Split list used for quick addition/removal of objects from a list
///        which is then only ever enumerated.
///
///        The split list is a list of objects which is split into chains by
///        a hash, each chain having an individual lock protecting members in
///        that subchain.
///
///        Adding an object to the list will result in the object appearing in
///        the list, but there is no guarantee of the order in the list, it
///        will not be added at the end, and will move as other objects are
///        removed.
///
///        All objects in the list are expected to be of the same type, we rely
///        on this object containing an ieutSplitListLink_t within it, and that
///        the offset of this in the object is specified at creation time.
///
///        Note: the number of chains that the list is split into is fixed.
//****************************************************************************
#define TRACE_COMP Engine

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>

#include "engineInternal.h"
#include "engineSplitList.h"
#include "memHandler.h"

// Use a fixed number of chains picked from: http://planetmath.org/goodhashtableprimes
#define ieutSPLIT_LIST_CHAIN_COUNT 49157

//****************************************************************************
/// @brief Generate a hash value from an object's pointer
///
/// @param[in]     object  The pointer to the object for which to generate a hash
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
static inline __attribute__((always_inline)) uint64_t ieut_generateSplitListObjectHash(void *object)
{
    return (uint64_t)object;
}

//****************************************************************************
/// @brief Create a split list
///
/// @param[in]     pThreadData       Thread data for the current thread
/// @param[in]     objectLinkOffset  The offset of the ieutSplitListLink_t in
///                                  the objects that will be stored in this list
/// @param[in]     memoryType        The memory type to use for allocations in
///                                  this list.
/// @param[out]    list              Allocated list
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieut_createSplitList(ieutThreadData_t *pThreadData,
                             size_t objectLinkOffset,
                             iemem_memoryType memoryType,
                             ieutSplitList_t **list)
{
    int32_t rc = OK;
    ieutSplitList_t *localList = NULL;
    ieutSplitListChain_t *localChains = NULL;

    ieutTRACEL(pThreadData, objectLinkOffset, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    localList = iemem_calloc(pThreadData, IEMEM_PROBE(memoryType, 60200), 1, sizeof(ieutSplitList_t));

    if (NULL == localList)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    localList->objectLinkOffset = objectLinkOffset;
    localList->memoryType = memoryType;

    localChains = iemem_calloc(pThreadData,
                               IEMEM_PROBE(memoryType, 60201),
                               1, ieutSPLIT_LIST_CHAIN_COUNT * sizeof(ieutSplitListChain_t));

    if (NULL == localChains)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Initialize the chain locks
    for(int32_t chainIndex=0; chainIndex<ieutSPLIT_LIST_CHAIN_COUNT; chainIndex++)
    {
        int osrc = pthread_mutex_init(&localChains[chainIndex].lock, NULL);

        if (osrc != 0)
        {
            rc = ISMRC_Error;
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    localList->chains = localChains;

    *list = localList;

mod_exit:

    if (rc != OK)
    {
        if (localList) iemem_free(pThreadData, memoryType, localList);
        if (localChains) iemem_free(pThreadData, memoryType, localChains);
    }

    ieutTRACEL(pThreadData, *list,  ENGINE_FNC_TRACE, FUNCTION_EXIT "list=%p rc=%d\n", __func__, *list, rc);

    return rc;
}

//****************************************************************************
/// @brief Visit a call-back function with all current values in the list
///
/// @param[in]     pThreadData Thread data for the current thread
/// @param[in]     list        List to be traversed
/// @param[in]     callback    Callback routine to call
/// @param[in]     context     Context information for the callback routine
//****************************************************************************
void ieut_traverseSplitList(ieutThreadData_t *pThreadData,
                            ieutSplitList_t *list,
                            ieutSplitList_TraverseCallback_t callback,
                            void *context)
{
    ieutTRACEL(pThreadData, list,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "list=%p\n", __func__, list);

    size_t objectLinkOffset = list->objectLinkOffset;

    for(int32_t chainIndex=0; chainIndex<ieutSPLIT_LIST_CHAIN_COUNT; chainIndex++)
    {
        ieutSplitListCallbackAction_t action = ieutSPLIT_LIST_CALLBACK_CONTINUE;
        ieutSplitListChain_t *chain = &list->chains[chainIndex];

        ismEngine_lockMutex(&chain->lock);

        ieutSplitListLink_t *link = chain->head;

        // Loop through the objects on this chain
        while(link != NULL)
        {
            void *object = (void *)link - objectLinkOffset;

            action = callback(pThreadData, object, context);

            // Move on to the next item in the list
            if (LIKELY(action == ieutSPLIT_LIST_CALLBACK_CONTINUE))
            {
                link = link->next;
            }
            // Stop enumerating this list
            else if (action == ieutSPLIT_LIST_CALLBACK_STOP)
            {
                ismEngine_unlockMutex(&chain->lock);
                goto mod_exit;
            }
            // Remove the object from the list and continue.
            else
            {
                ieutSplitListLink_t *nextLink;

                assert(action == ieutSPLIT_LIST_CALLBACK_REMOVE_OBJECT);

                if (link->prev == (ieutSplitListLink_t *)chain)
                {
                    chain->head = link->next;
                }
                else
                {
                    link->prev->next = link->next;
                }

                if (link->next != NULL)
                {
                    link->next->prev = link->prev;
                    nextLink = link->next;
                }
                else
                {
                    nextLink = NULL;
                }

                link->prev = link->next = NULL;

                link = nextLink;
            }
        }

        ismEngine_unlockMutex(&chain->lock);
    }

mod_exit:

    ieutTRACEL(pThreadData, list, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief Add an object to the split list.
///
/// @param[in]     list        List to add the object to
/// @param[in]     object      Object to be added
//****************************************************************************
void ieut_addObjectToSplitList(ieutSplitList_t *list, void *object)
{
    uint64_t objectHash = ieut_generateSplitListObjectHash(object);

    ieutSplitListChain_t *chain = &list->chains[objectHash%ieutSPLIT_LIST_CHAIN_COUNT];
    ieutSplitListLink_t *link = (ieutSplitListLink_t *)(object + list->objectLinkOffset);

    ismEngine_lockMutex(&chain->lock);

    // Already in the list if the link->prev is not NULL.
    if (LIKELY(link->prev == NULL))
    {
        // Add to the chain
        link->prev = (ieutSplitListLink_t *)chain;
        link->next = chain->head;

        if (link->next != NULL)
        {
            link->next->prev = link;
        }

        chain->head = link;
    }

    ismEngine_unlockMutex(&chain->lock);

    return;
}

//****************************************************************************
/// @brief Remove an object from the split list
///
/// @param[in]     list    List to remove the object from
/// @param[in]     object  Object to be removed
//****************************************************************************
void ieut_removeObjectFromSplitList(ieutSplitList_t *list, void *object)
{
    uint64_t objectHash = ieut_generateSplitListObjectHash(object);

    ieutSplitListChain_t *chain = &list->chains[objectHash%ieutSPLIT_LIST_CHAIN_COUNT];
    ieutSplitListLink_t *link = (ieutSplitListLink_t *)(object + list->objectLinkOffset);

    ismEngine_lockMutex(&chain->lock);

    // Already in the list if the link->prev is NULL.
    if (LIKELY(link->prev != NULL))
    {
        // Remove from the list
        if (link->prev == (ieutSplitListLink_t *)chain)
        {
            chain->head = link->next;
        }
        else
        {
            link->prev->next = link->next;
        }

        if (link->next != NULL)
        {
            link->next->prev = link->prev;
        }

        link->prev = link->next = NULL;
    }

    ismEngine_unlockMutex(&chain->lock);

    return;
}

//****************************************************************************
/// @brief Destroy a split list freeing list storage (but not objects)
///
/// @param[in]     pThreadData  Thread data for the current thread
/// @param[in]     list         Split List to be destroyed
//****************************************************************************
void ieut_destroySplitList(ieutThreadData_t *pThreadData, ieutSplitList_t *list)
{
    ieutTRACEL(pThreadData, list,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "list=%p\n", __func__, list);

    // Start out by clearing the entire list
    for(int32_t chainIndex=0; chainIndex<ieutSPLIT_LIST_CHAIN_COUNT; chainIndex++)
    {
        ieutSplitListChain_t *chain = &list->chains[chainIndex];

        ismEngine_lockMutex(&chain->lock);

        ieutSplitListLink_t *link = chain->head;

        while(link != NULL)
        {
            ieutSplitListLink_t *nextLink = link->next;

            link->prev = link->next = NULL;

            link = nextLink;
        }

        chain->head = NULL;

        (void)pthread_mutex_destroy(&chain->lock);
    }

    // Free the chains
    if (list->chains)
    {
        iemem_free(pThreadData, list->memoryType, list->chains);
    }

    iemem_free(pThreadData, list->memoryType, list);

    ieutTRACEL(pThreadData, list, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

#if 0
#define ieutNUM_CHAINLENGTHBOUNDARIES 15
void ieut_checkSplitList(ieutSplitList_t *list)
{
    uint32_t chainLengthBoundaries[ieutNUM_CHAINLENGTHBOUNDARIES]={1,2,3,4,5,6,7,8,9,10,20,30,50,1000};
    uint32_t chainCounts[ieutNUM_CHAINLENGTHBOUNDARIES+1]={0};
    uint64_t totalCount = 0;

    size_t objectLinkOffset = list->objectLinkOffset;

    for(uint32_t chainIndex = 0; chainIndex < ieutSPLIT_LIST_CHAIN_COUNT; chainIndex++)
    {
        ieutSplitListChain_t *chain = &list->chains[chainIndex];

        uint32_t chainLength = 0;

        ismEngine_lockMutex(&chain->lock);

        ieutSplitListLink_t *link = chain->head;
        ieutSplitListLink_t *prevCheck = (ieutSplitListLink_t *)chain;

        while(link != NULL)
        {
            if (link->prev != prevCheck)
            {
                printf("ERROR: Link for object %p pointing to %p not prev object (%p).\n",
                        (void*)link - objectLinkOffset, link->prev, prevCheck);
            }

            prevCheck = link;
            link = link->next;

            chainLength++;
        }

        ismEngine_unlockMutex(&chain->lock);

        totalCount += chainLength;

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

    printf("Total objects: %lu\n", totalCount);
    for(int32_t i=0; i< ieutNUM_CHAINLENGTHBOUNDARIES; i++)
    {
        printf("Num chains of length < %u = %u\n", chainLengthBoundaries[i], chainCounts[i]);
    }
    printf("Number of longer chains is %u\n", chainCounts[ieutNUM_CHAINLENGTHBOUNDARIES]);
}
#endif
