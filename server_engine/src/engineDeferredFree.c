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
/// @file  engineDeferredFree.c
/// @brief Maintain a list of memory areas whose free must be deferred until
///        no threads are referring to them.
//****************************************************************************
#define TRACE_COMP Engine

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>

#include "engineInternal.h"
#include "engineDeferredFree.h"
#include "engineUtils.h"
#include "resourceSetStats.h"

//****************************************************************************
/// @brief Initialize a deferred free list
///
/// @param[in]     pThreadData       Thread data for the current thread
/// @param[in]     pDeferredFreeList The list to be initialized
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieut_initDeferredFreeList(ieutThreadData_t *pThreadData,
                                  ieutDeferredFreeList_t *pDeferredFreeList)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pDeferredFreeList, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "pDeferredFreeList=%p\n",
               __func__, pDeferredFreeList);

    pDeferredFreeList->areaCount = 0;
    pDeferredFreeList->areaMax = 0;
    pDeferredFreeList->areas = NULL;

    int osrc = pthread_mutex_init(&pDeferredFreeList->lock, NULL);

    if (osrc != 0)
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Terminate a deferred free list
///
/// @param[in]     pThreadData       Thread data for the current thread
/// @param[in]     pDeferredFreeList The list to be initialized
///
/// @remark Note that this will free all the memory currently in the list.
//****************************************************************************
void ieut_destroyDeferredFreeList(ieutThreadData_t *pThreadData,
                                  ieutDeferredFreeList_t *pDeferredFreeList)
{
    ieutTRACEL(pThreadData, pDeferredFreeList, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pDeferredFreeList=%p\n",
               __func__, pDeferredFreeList);

    // We are potentially going to change the thread cache primed value as we free
    // memory, but we want to reset it so our caller is ready to go.
    iereResourceSetHandle_t prevPrimedResourceSet = iere_getPrimedResourceSet(pThreadData);

    ismEngine_lockMutex(&pDeferredFreeList->lock);

    // Free everything, ignoring the update count
    for(uint32_t i=0; i<pDeferredFreeList->areaCount; i++)
    {
        ieutDeferredFreeArea_t *pArea = &pDeferredFreeList->areas[i];

        iere_primeThreadCache(pThreadData, pArea->resourceSet);
        if (pArea->memoryStructId != NULL)
        {
            iere_freeStruct(pThreadData,
                            pArea->resourceSet,
                            pArea->memType,
                            pArea->memory,
                            pArea->memoryStructId);
        }
        else
        {
            iere_free(pThreadData,
                      pArea->resourceSet,
                      pArea->memType,
                      pArea->memory);
        }
    }

    iere_primeThreadCache(pThreadData, prevPrimedResourceSet);

    iemem_free(pThreadData, iemem_deferredFreeLists, pDeferredFreeList->areas);

    pDeferredFreeList->areaCount = pDeferredFreeList->areaMax = 0;
    pDeferredFreeList->areas = NULL;

    (void)pthread_mutex_destroy(&pDeferredFreeList->lock);

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

void findLowestMemUpdateCount(ieutThreadData_t *pThreadData,
                              void *context)
{
    uint64_t lowestUpdateCount = *(uint64_t *)context;

    if (pThreadData->memUpdateCount != 0 && pThreadData->memUpdateCount < lowestUpdateCount)
    {
        *(uint64_t *)context = pThreadData->memUpdateCount;
    }
}

//****************************************************************************
/// @brief Add memory to the deferred free list
///
/// @param[in]     pThreadData       Thread data for the current thread
/// @param[in]     pDeferredFreeList The list to be initialized
/// @param[in]     memory            The memory address to add
/// @param[in]     memoryStructId    The address of a strucId if any (NULL
///                                  for non-structure memory)
/// @param[in]     memType           The memory type of this memory area
/// @param[in]     resourceSet       The resourceSet if any of this memory
///                                  area.
///
/// @remark We do NOT police against the same memory address being added
///         multiple times to the list - just as free does not.
//****************************************************************************
void ieut_addDeferredFree(ieutThreadData_t *pThreadData,
                          ieutDeferredFreeList_t *pDeferredFreeList,
                          void *memory,
                          void *memoryStructId,
                          iemem_memoryType memType,
                          iereResourceSetHandle_t resourceSet)
{
    ieutTRACEL(pThreadData, memory, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY
               "pDeferredFreeList=%p memory=%p memoryStructId=%p\n",
               __func__, pDeferredFreeList, memory, memoryStructId);

    uint64_t memUpdateCountNow = __sync_add_and_fetch(&ismEngine_serverGlobal.memUpdateCount, 1);

    uint64_t lowestThreadMemUpdateCount = memUpdateCountNow - 1;

    ieut_enumerateThreadData(findLowestMemUpdateCount, &lowestThreadMemUpdateCount);

    // We are potentially going to change the thread cache primed value as we free
    // memory, but we want to reset it so our caller is ready to go.
    iereResourceSetHandle_t prevPrimedResourceSet = iere_getPrimedResourceSet(pThreadData);

    ismEngine_lockMutex(&pDeferredFreeList->lock);

    // Free anything we now can
    uint32_t index = 0;
    for(; index<pDeferredFreeList->areaCount; index++)
    {
        ieutDeferredFreeArea_t *pArea = &pDeferredFreeList->areas[index];
        if (pArea->freeAtMemUpdateCount <= lowestThreadMemUpdateCount)
        {
            if (pArea->resourceSet != iereNO_RESOURCE_SET)
            {
                iere_primeThreadCache(pThreadData, pArea->resourceSet);
            }

            if (pArea->memoryStructId != NULL)
            {
                iere_freeStruct(pThreadData,
                                pArea->resourceSet,
                                pArea->memType,
                                pArea->memory,
                                pArea->memoryStructId);
            }
            else
            {
                iere_free(pThreadData,
                          pArea->resourceSet,
                          pArea->memType,
                          pArea->memory);
            }
        }
        else
        {
            break;
        }
    }

    // Reset primed resource set
    iere_primeThreadCache(pThreadData, prevPrimedResourceSet);

    // Move down anything we can
    if (index != 0)
    {
        uint32_t newCount = pDeferredFreeList->areaCount-index;
        memmove(&pDeferredFreeList->areas[0],
                &pDeferredFreeList->areas[index],
                sizeof(ieutDeferredFreeArea_t) * newCount);
        pDeferredFreeList->areaCount = newCount;
    }

    // We're at the next boundary (but this could be the first time in)
    if (pDeferredFreeList->areaCount == pDeferredFreeList->areaMax)
    {
        uint32_t newAreaMax = pDeferredFreeList->areaMax + ieutDEFERREDFREE_AREA_MAX_INCREMENT;

        ieutDeferredFreeArea_t *newAreas = iemem_realloc(pThreadData,
                                                         IEMEM_PROBE(iemem_deferredFreeLists, 2),
                                                         pDeferredFreeList->areas,
                                                         sizeof(ieutDeferredFreeArea_t) * newAreaMax);

        if (newAreas == NULL)
        {
            // We can't reallocate... we'll have to leak it
            ieutTRACEL(pThreadData, pDeferredFreeList, ENGINE_WORRYING_TRACE,
                       "Couldn't realloc deferred free areas. Leaking %p [%lu bytes]\n",
                       memory, iemem_full_size(iemem_deferredFreeLists, memory));
            ism_common_setError(ISMRC_AllocateError);
            goto mod_exit;
        }

        pDeferredFreeList->areaMax = newAreaMax;
        pDeferredFreeList->areas = newAreas;
    }
    // If everything is not being forced deferred, and
    // we're half way to the *next* boundary grumble.
    #ifndef DEFER_ALL_DEFERRABLE_FREES
    else if (pDeferredFreeList->areaCount ==
             pDeferredFreeList->areaMax - (ieutDEFERREDFREE_AREA_MAX_INCREMENT/2))
    {
        // Take a core in a debug build, not in a non-debug build
        #ifndef NDEBUG
        bool takeCore = true;
        #else
        bool takeCore = false;
        #endif

        ieutTRACE_FFDC(ieutPROBE_001, takeCore, "Deferred free list getting long", ISMRC_Error,
                       "DeferredFreeList", pDeferredFreeList, sizeof(ieutDeferredFreeList_t),
                       NULL);
    }
    #endif

    // Add ourselves to the list
    pDeferredFreeList->areas[pDeferredFreeList->areaCount].memory = memory;
    pDeferredFreeList->areas[pDeferredFreeList->areaCount].memoryStructId = memoryStructId;
    pDeferredFreeList->areas[pDeferredFreeList->areaCount].memType = memType;
    pDeferredFreeList->areas[pDeferredFreeList->areaCount].resourceSet = resourceSet;
    pDeferredFreeList->areas[pDeferredFreeList->areaCount].freeAtMemUpdateCount = memUpdateCountNow;
    pDeferredFreeList->areaCount += 1;

mod_exit:

    ismEngine_unlockMutex(&pDeferredFreeList->lock);

    ieutTRACEL(pThreadData, pDeferredFreeList, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}
