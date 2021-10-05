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
/// @file  topicTreeThreads.c
/// @brief Engine component topic tree per-thread data manipulation functions
//****************************************************************************
#define TRACE_COMP Engine

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>

#include "engineInternal.h"
#include "topicTree.h"
#include "topicTreeInternal.h"

//****************************************************************************
/// @brief Initialize the topic tree values in the per thread data structure
///
/// @param[in]     pThreadData  Pointer to the per-thread data structure
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_createThreadData(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    TRACE(ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pThreadData->sublist = ism_common_calloc(ISM_MEM_PROBE(ism_memory_engine_misc,1),1, sizeof(iettSubscriberList_t));

    pThreadData->topicStringBufferSize = 0;

    // Initialize a cache on this thread - if it fails, run with no cache
    if (ismEngine_serverGlobal.subListCacheCapacity != 0)
    {
        (void)ieut_createHashTable(pThreadData,
                                   ismEngine_serverGlobal.subListCacheCapacity,
                                   iemem_subscriberCache,
                                   &pThreadData->sublistCache);
    }
    else
    {
        pThreadData->sublistCache = NULL;
    }

    TRACE(ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Release the topic tree values from the per-thread data structure
///
/// @param[in]     pThreadData  Pointer to the per-thread data structure
//****************************************************************************
void iett_destroyThreadData(ieutThreadData_t *pThreadData)
{
    TRACE(ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(NULL != pThreadData);

    // Free values in the cached sublist
    if (NULL != pThreadData->sublist)
    {
        if (NULL != pThreadData->sublist->subscriberNodes)
        {
            iemem_free(pThreadData, iemem_subsQuery, pThreadData->sublist->subscriberNodes);
        }

        if (NULL != pThreadData->sublist->subscribers)
        {
            iemem_free(pThreadData, iemem_subsQuery, pThreadData->sublist->subscribers);
        }

        if (NULL != pThreadData->sublist->topicString)
        {
            iemem_free(pThreadData, iemem_subsQuery, (void *)pThreadData->sublist->topicString);
        }

        ism_common_free(ism_memory_engine_misc,pThreadData->sublist);
    }

    // Free the sublist cache hash table
    if (NULL != pThreadData->sublistCache)
    {
        ieut_destroyHashTable(pThreadData, pThreadData->sublistCache);
    }

    TRACE(ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

//****************************************************************************
/// @brief Initialize a Subscriber list from the topic thread data arrays
///
/// @param[in]     pSublist     The subscriber list to initialize
///
/// @remark A return code of OK or ISMRC_NotFound indicates that there is no need
///         to call iett_getSubscriberList, the list was initialized from the threads
///         cache, and nothing has changed.
///
///         A return code of ISMRC_NotInThreadCache indicates that a call to
///         iett_getSubscriberList is required, as the topic string did not match
///         the locally cached value (or more subscriptions have been added).
///
/// @return OK or ISMRC_NotFound indicating that the cache satisfied the request,
///         or ISMRC_NotInThreadCache indicating that it was not.
//****************************************************************************
int32_t iett_initSublistArrays(ieutThreadData_t *pThreadData,
                               iettSubscriberList_t *pSublist)
{
    int32_t rc = ISMRC_NotInThreadCache;

    // We can use the cached arrays if this is the 1st nested publish on this thread
    // (which is usually the case).
    if (LIKELY(pThreadData->publishDepth == 1))
    {
        assert(pThreadData->sublist != NULL);

        iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

        pSublist->subscribers = pThreadData->sublist->subscribers;
        pSublist->subscriberCapacity = pThreadData->sublist->subscriberCapacity;
        pSublist->subscriberNodes = pThreadData->sublist->subscriberNodes;
        pSublist->subscriberNodeCapacity = pThreadData->sublist->subscriberNodeCapacity;
        pSublist->publishSUV = pThreadData->sublist->publishSUV;
        pSublist->usingCachedArrays = true;

        // This might be the exact same request as last time, if it is we can avoid
        // calling iett_getSubscriberList altogether.
        if (pThreadData->topicStringBufferSize != 0 &&
            tree->subsUpdates == pThreadData->sublist->publishSUV &&
            (strcmp(pThreadData->sublist->topicString, pSublist->topicString) == 0))
        {
            // Grab the lock and check that nothing has changed
            ismEngine_getRWLockForRead(&tree->subsLock);

            // Yes - this request matches the last.
            if (tree->subsUpdates == pSublist->publishSUV)
            {
                // No subscribers / subsNodes - no further work to do.
                if (pThreadData->sublist->subscriberCount == 0 &&
                    pThreadData->sublist->subscriberNodeCount == 0)
                {
                    rc = ISMRC_NotFound;
                }
                // Increment the useCount on each node.
                else
                {
                    rc = OK;

                    for(int32_t i=pThreadData->sublist->subscriberNodeCount-1; i>=0; i--)
                    {
                        pSublist->subscriberNodes[i]->listCount++;
                        __sync_fetch_and_add(&(pSublist->subscriberNodes[i]->useCount), 1);
                    }
                }
            }

            // Release the subscription tree lock
            ismEngine_unlockRWLock(&tree->subsLock);
        }
    }
    // Don't attempt to use the thread's cache for recursive publish
    else
    {
        assert(rc == ISMRC_NotInThreadCache);

        ieutTRACEL(pThreadData, pThreadData->publishDepth, ENGINE_INTERESTING_TRACE,
                   FUNCTION_IDENT "Not using cached arrays. pThreadData->publishDepth=%u\n",
                   __func__, pThreadData->publishDepth);

        pSublist->subscribers = NULL;
        pSublist->subscriberCapacity = 0;
        pSublist->subscriberNodes = NULL;
        pSublist->subscriberNodeCapacity = 0;

        pSublist->usingCachedArrays = false;
    }

    if (rc == ISMRC_NotInThreadCache)
    {
        pSublist->publishSUV = 0;
        pSublist->subscriberCount = 0;
        pSublist->subscriberNodeCount= 0;
        pSublist->requestSelection = false;
    }
    else
    {
        pSublist->subscriberCount = pThreadData->sublist->subscriberCount;
        pSublist->subscriberNodeCount = pThreadData->sublist->subscriberNodeCount;
        pSublist->requestSelection = pThreadData->sublist->requestSelection;
    }

    // Remote server information is not part of this cache
    pSublist->remoteServers = NULL;
    pSublist->remoteServerCount = 0;
    pSublist->remoteServerCapacity = 0;

    return rc;
}

//****************************************************************************
/// @brief Update the topic thread data arrays from a subscriber list
///
/// @param[in]     pSublist     The subscriber list to use
/// @param[in]     rc           Return code to decide whether to free
//****************************************************************************
void iett_updateCachedArrays(ieutThreadData_t *pThreadData,
                             iettSubscriberList_t *pSublist,
                             int32_t rc)
{
    // If we were using the cached arrays, update them
    if (pSublist->usingCachedArrays)
    {
        char *newTopicString;

        size_t newTopicStringLength = strlen(pSublist->topicString)+1;

        // Make sure we have space to store the topic string
        if (newTopicStringLength > pThreadData->topicStringBufferSize)
        {
            newTopicString = iemem_realloc(pThreadData,
                                           IEMEM_PROBE(iemem_subsQuery, 6),
                                           (char *)(pThreadData->sublist->topicString),
                                           newTopicStringLength);

            if (newTopicString != NULL)
            {
                pThreadData->topicStringBufferSize = newTopicStringLength;
                pThreadData->sublist->topicString = newTopicString;
            }


        }
        else{
            newTopicString = (char *)(pThreadData->sublist->topicString);
        }

        if (newTopicString != NULL)
        {
            memcpy(newTopicString, pSublist->topicString, newTopicStringLength);
            pThreadData->sublist->publishSUV = pSublist->publishSUV;
            pThreadData->sublist->subscribers = pSublist->subscribers;
            pThreadData->sublist->subscriberCount = pSublist->subscriberCount;
            pThreadData->sublist->subscriberCapacity = pSublist->subscriberCapacity;
            pThreadData->sublist->subscriberNodes = pSublist->subscriberNodes;
            pThreadData->sublist->subscriberNodeCount = pSublist->subscriberNodeCount;
            pThreadData->sublist->subscriberNodeCapacity = pSublist->subscriberNodeCapacity;
            pThreadData->sublist->requestSelection = pSublist->requestSelection;
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }
    }

    // If this is the publish which could have been using the cache, we can tidy them up.
    if (LIKELY(pThreadData->publishDepth == 1))
    {
        // If the system is short on memory, release our caches.
        if (rc == ISMRC_AllocateError)
        {
            assert(pThreadData->sublist != NULL);

            // Empty the cached sublist
            if (NULL != pThreadData->sublist->subscriberNodes)
            {
                iemem_free(pThreadData, iemem_subsQuery, pThreadData->sublist->subscriberNodes);
            }

            if (NULL != pThreadData->sublist->subscribers)
            {
                iemem_free(pThreadData, iemem_subsQuery, pThreadData->sublist->subscribers);
            }

            if (NULL != pThreadData->sublist->topicString)
            {
                iemem_free(pThreadData, iemem_subsQuery, (void *)(pThreadData->sublist->topicString));
            }

            memset(pThreadData->sublist, 0, sizeof(iettSubscriberList_t));

            pThreadData->topicStringBufferSize = 0;

            // Destroy the sublist cache hash table
            if (NULL != pThreadData->sublistCache)
            {
                ieut_destroyHashTable(pThreadData, pThreadData->sublistCache);
                pThreadData->sublistCache = NULL;
            }
        }
        // If the system is NOT short on memory check whether we need to tidy the cache
        else if (NULL != pThreadData->sublistCache)
        {
            // If the cache has more than double it's capacity in entries, try removing empty
            // entries from it.
            if (pThreadData->sublistCache->totalCount > (2 * pThreadData->sublistCache->capacity))
            {
                ieut_removeEmptyHashEntries(pThreadData, pThreadData->sublistCache);
            }

            // If we still have too many entries, clear the table completely - in the future
            // this might remove the oldest from the cache, at the moment we have no way to
            // identify the oldest.
            if (pThreadData->sublistCache->totalCount > (2 * pThreadData->sublistCache->capacity))
            {
                ieut_clearHashTable(pThreadData, pThreadData->sublistCache);
            }
        }
    }
}
