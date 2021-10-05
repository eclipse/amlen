/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
/// @file  resourceSetMem.c
/// @brief Engine component memory functions for resource sets
//****************************************************************************
#define TRACE_COMP Engine

#include "engineInternal.h"
#include "memHandler.h"
#include "resourceSetStats.h"

#ifdef MEMDEBUG
    typedef struct tag_iereMemEyecatcher_t {
        char StructId[4];
        iereResourceSetHandle_t resourceSet;
    } __attribute__ (( aligned(16))) iereMemEyecatcher_t;

#define IERE_MEM_STRUCTID "IERM"
#define iereEYECATCHER_SIZE sizeof(iereMemEyecatcher_t)
#else
#define iereEYECATCHER_SIZE 0
#endif

//*************************************************************************
/// @brief Increase the total memory statistic (after a successful allocation)
//*************************************************************************
static inline void *iere_increaseTotalMem(ieutThreadData_t *pThreadData,
                                          iereResourceSetHandle_t resourceSet,
                                          uint32_t probe,
                                          void *mem )
{
    if (mem != NULL)
    {
        int64_t actualSize = (int64_t)iemem_full_size(iemem_numtypes, mem);

        iere_updateTotalMemStat(pThreadData, resourceSet, probe, mem, actualSize);

#ifdef MEMDEBUG
        DEBUG_ONLY iereThreadCacheEntry_t *resThreadCache = pThreadData->curThreadCacheEntry;
        iereMemEyecatcher_t *eyeC = (iereMemEyecatcher_t *)mem;
        mem += iereEYECATCHER_SIZE;
        ismEngine_SetStructId(eyeC->StructId, IERE_MEM_STRUCTID);
        assert(resourceSet == iereNO_RESOURCE_SET ||
               (resThreadCache != NULL && resThreadCache->resourceSet == resourceSet));
        eyeC->resourceSet = resourceSet;
#endif
    }

    return mem;
}

//*************************************************************************
/// @brief iere version of usable_size
//*************************************************************************
size_t iere_usable_size(iemem_memoryType type, void *mem)
{
#ifdef MEMDEBUG
    //Check the thing we are freeing used the same resource set when allocated
    if (mem != NULL)
    {
        mem -= iereEYECATCHER_SIZE;
        iereMemEyecatcher_t *eyeC = (iereMemEyecatcher_t *)mem;
        ismEngine_CheckStructId(eyeC->StructId, IERE_MEM_STRUCTID, ieutPROBE_001);
    }
#endif

    return iemem_usable_size(type,mem)-iereEYECATCHER_SIZE;
}

//*************************************************************************
/// @brief Return the full size of the memory area passed
//*************************************************************************
size_t iere_full_size(iemem_memoryType type, void *mem)
{
#ifdef MEMDEBUG
    if (mem != NULL)
    {
        mem -= iereEYECATCHER_SIZE;
        iereMemEyecatcher_t *eyeC = (iereMemEyecatcher_t *)mem;
        ismEngine_CheckStructId(eyeC->StructId, IERE_MEM_STRUCTID, ieutPROBE_001);
    }
#endif

    return iemem_full_size(type, mem);
}

//*************************************************************************
/// @brief Wrapper around iemem_malloc to allow resourceSet association
//*************************************************************************
void *iere_malloc(ieutThreadData_t *pThreadData,
                  iereResourceSetHandle_t resourceSet,
                  uint32_t probe, size_t size)
{
#ifdef MEMDEBUG
    // Allow space for our eye catcher
    size += iereEYECATCHER_SIZE;
#endif

    void *mem = iemem_malloc(pThreadData, probe, size);

    return iere_increaseTotalMem(pThreadData, resourceSet, probe, mem);
}

//*************************************************************************
/// @brief Wrapper around iemem_calloc to allow resourceSet association
//*************************************************************************
void *iere_calloc(ieutThreadData_t *pThreadData,
                  iereResourceSetHandle_t resourceSet,
                  uint32_t probe, size_t nmemb, size_t size)
{
#ifdef MEMDEBUG
    //Work out how much extra we need for our eye-catcher...round up
    nmemb += (iereEYECATCHER_SIZE + (size -1)) /size;
#endif

    void *mem = iemem_calloc(pThreadData, probe, nmemb, size);

    return iere_increaseTotalMem(pThreadData, resourceSet, probe, mem);
}

//*************************************************************************
/// @brief Wrapper around iemem_realloc to allow resourceSet association
//*************************************************************************
void *iere_realloc(ieutThreadData_t *pThreadData,
                   iereResourceSetHandle_t resourceSet,
                   uint32_t probe, void *ptr, size_t size)
{
#ifdef MEMDEBUG
    void *oldPtr = ptr;

    if (ptr != NULL)
    {
        //Check the thing we are freeing used the same resource set when allocated
        DEBUG_ONLY iereThreadCacheEntry_t *resThreadCache = pThreadData->curThreadCacheEntry;
        ptr -= iereEYECATCHER_SIZE;
        iereMemEyecatcher_t *eyeC = (iereMemEyecatcher_t *)ptr;
        ismEngine_CheckStructId(eyeC->StructId, IERE_MEM_STRUCTID, ieutPROBE_001);
        assert((eyeC->resourceSet == iereNO_RESOURCE_SET && resourceSet == iereNO_RESOURCE_SET) ||
               (eyeC->resourceSet != iereNO_RESOURCE_SET && resThreadCache != NULL && eyeC->resourceSet == resThreadCache->resourceSet));
    }

    // Allow space for our eye catcher when the realloc occurs
    size += iereEYECATCHER_SIZE;
#endif

    int64_t oldSize = (int64_t)iemem_full_size(iemem_numtypes, ptr);

    void *mem = iemem_realloc(pThreadData, probe, ptr, size);

    if (mem != NULL)
    {
        int64_t newSize = (int64_t)iemem_full_size(iemem_numtypes, mem);

#ifdef MEMDEBUG
        if (oldSize != 0)
        {
            iere_updateTotalMemStat(pThreadData, resourceSet, probe, oldPtr, -oldSize);
        }

        iere_updateTotalMemStat(pThreadData, resourceSet, probe, mem, newSize);

        if (oldSize == 0)
        {
            DEBUG_ONLY iereThreadCacheEntry_t *resThreadCache = pThreadData->curThreadCacheEntry;
            iereMemEyecatcher_t *eyeC = (iereMemEyecatcher_t *)mem;
            ismEngine_SetStructId(eyeC->StructId, IERE_MEM_STRUCTID);
            assert(resourceSet == iereNO_RESOURCE_SET ||
                   (resThreadCache != NULL && resThreadCache->resourceSet == resourceSet));
            eyeC->resourceSet = resourceSet;
        }

        mem += iereEYECATCHER_SIZE;
#else
        if (newSize > oldSize)
        {
            iere_updateTotalMemStat(pThreadData, resourceSet, probe, mem, (newSize-oldSize));
        }
        else if (oldSize > newSize)
        {
            iere_updateTotalMemStat(pThreadData, resourceSet, probe, mem, -(oldSize-newSize));
        }
#endif
    }

    return mem;
}

//*************************************************************************
/// @brief Wrapper around iemem_calloc with resourceSet knowledge
//*************************************************************************
void iere_free(ieutThreadData_t *pThreadData,
               iereResourceSetHandle_t resourceSet,
               iemem_memoryType type,
               void *mem)
{
#ifdef MEMDEBUG
    //Check the thing we are freeing used the same resource set when allocated
    if (mem != NULL)
    {
        DEBUG_ONLY iereThreadCacheEntry_t *resThreadCache = pThreadData->curThreadCacheEntry;
        mem -= iereEYECATCHER_SIZE;
        iereMemEyecatcher_t *eyeC = (iereMemEyecatcher_t *)mem;
        ismEngine_CheckStructId(eyeC->StructId, IERE_MEM_STRUCTID, ieutPROBE_001);
        assert((eyeC->resourceSet == iereNO_RESOURCE_SET && resourceSet == iereNO_RESOURCE_SET) ||
               (eyeC->resourceSet != iereNO_RESOURCE_SET && resThreadCache != NULL && eyeC->resourceSet == resThreadCache->resourceSet));
    }
#endif

    if (mem != NULL)
    {
        int64_t actualSize = (int64_t)iemem_full_size(type, mem);

        iere_updateTotalMemStat(pThreadData, resourceSet, (uint32_t)type, mem, -actualSize);

        iemem_free(pThreadData, type, mem);
    }
}

//*************************************************************************
/// @brief iere version of freeStruct
//*************************************************************************
void iere_freeStruct(ieutThreadData_t *pThreadData,
                     iereResourceSetHandle_t resourceSet,
                     iemem_memoryType type,
                     void *pStruct,
                     char *pStructId)
{
    if (pStruct != NULL)
    {
        assert(pStructId != NULL);
        ismEngine_InvalidateStructId(pStructId);
    }

    iere_free(pThreadData, resourceSet, type, pStruct);
}

#ifdef MEMDEBUG
//*************************************************************************
/// @brief Update the memory stats for a NO_RESOURCE_SET memory area
//*************************************************************************
void iere_updateMem(ieutThreadData_t *pThreadData,
                    iereResourceSetHandle_t resourceSet,
                    uint32_t probe, void *mem,
                    size_t size)
{
    assert(mem != NULL);

    DEBUG_ONLY iereThreadCacheEntry_t *resThreadCache = pThreadData->curThreadCacheEntry;
    assert(resThreadCache->resourceSet == resourceSet);
    mem -= iereEYECATCHER_SIZE;
    iereMemEyecatcher_t *eyeC = (iereMemEyecatcher_t *)mem;
    ismEngine_CheckStructId(eyeC->StructId, IERE_MEM_STRUCTID, ieutPROBE_001);
    eyeC->resourceSet = resourceSet;

    iere_updateTotalMemStat(pThreadData, resourceSet, probe, mem, (int64_t)size);
}
#endif
