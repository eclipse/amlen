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
/// @file  engineDeferredFree.h
/// @brief Defines structures and functions used for deferring freeing of
///        memory
//****************************************************************************
#ifndef __ISM_ENGINE_DEFERRED_FREE_DEFINED
#define __ISM_ENGINE_DEFERRED_FREE_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "memHandler.h"

/// @brief An individual memory area that has be scheduled for deferred free processing
typedef struct tag_ieutDeferredFreeArea_t
{
    void *memory;                        ///< Memory address to be freed
    void *memoryStructId;                ///< Memory address of the structId (if any)
    iemem_memoryType memType;            ///< Memory type of this memory area
    iereResourceSetHandle_t resourceSet; ///< Resource set handle of this memory area (if any)
    uint64_t freeAtMemUpdateCount;       ///< Memory update count at which it can be freed
} ieutDeferredFreeArea_t;

/// @brief List of memory areas scheduled for deferred free processing
typedef struct tag_ieutDeferredFreeList_t
{
    ieutDeferredFreeArea_t *areas; ///< Pointer to an array of areas to be freed
    uint32_t areaCount;            ///< Counter of the areas in the array
    uint32_t areaMax;              ///< Maximum number of entries in the area
    pthread_mutex_t lock;          ///< lock protecting array
} ieutDeferredFreeList_t;

#define ieutDEFERREDFREE_AREA_MAX_INCREMENT 1000

int32_t ieut_initDeferredFreeList(ieutThreadData_t *pThreadData,
                                  ieutDeferredFreeList_t *pDeferredFreeList);
void ieut_destroyDeferredFreeList(ieutThreadData_t *pThreadData,
                                  ieutDeferredFreeList_t *pDeferredFreeList);
void ieut_addDeferredFree(ieutThreadData_t *pThreadData,
                          ieutDeferredFreeList_t *pDeferredFreeList,
                          void *memory,
                          void *memoryStructId,
                          iemem_memoryType memType,
                          iereResourceSetHandle_t resourceSet);
#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_DEFERRED_FREE_DEFINED */
