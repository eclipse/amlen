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
/// @file  memHandler.c
/// @brief Functions for allocing/deallocing/monitoring memory usage
//****************************************************************************
#define TRACE_COMP Util

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <malloc.h> //For malloc_usable_size
#include <mcheck.h> //for mtrace

#include <commonMemHandler.h>
#include "ismutil.h"
#include "threadlocal.h"


#ifdef COMMON_MALLOC_WRAPPER

//array to store whether allocations are allowed for each memory type
static bool preventMallocs[ism_common_mem_numtypes] = { 0 };

//array that stores how much memory (in bytes) is in use by each memory type
static size_t memSizes[ism_common_mem_numtypes] = { 0 };


//How much memory of each type a thread reserves for itself in one go: 512k
static uint32_t ismmThreadMemChunkBytes = (512L * 1024L);



// Set up the structure that defines the comment for each mem type and how readily we should disable allocs
#define COMMON_MEMHANDLER_RICH_TYPE_STRUCTURE
#include <commonMemHandlerTypes.h>
#undef COMMON_MEMHANDLER_RICH_TYPE_STRUCTURE

//*************************************************************************
/// @brief Checks whether a memory allocation of given type+size should be allowed
///
/// @param[in] type  - memory type of hypothetical memory
/// @param[in] size  - size of hypothetical memory in bytes
///
/// @return true - allocation should be allowed (false = disallowed)
//*************************************************************************
static inline bool ismm_isAllowedMemUsage(ism_common_memoryType type,
                                          size_t size)
{
    bool increaseAllowed = true;

    if (preventMallocs[type])
    {
        VTRACE(type, ISM_NORMAL_TRACE,
                   "Mem request: type %d, size %lu DENIED\n", type, size);
        increaseAllowed = false;
    }

    return increaseAllowed;
}


//*************************************************************************
/// @brief Increases record of how much memory is used by a given type
///
/// @remark If the memUsage array becomes a bottle-neck, each thread could
/// keep temporary counts in thread-local-storage and only update the central
/// array periodically
//*************************************************************************
static inline void ismm_increaseGlobalMemUsage(ism_common_memoryType type, size_t size)
{
    __sync_add_and_fetch(&(memSizes[type]), size);
}

static inline void ismm_increaseMemUsage(ism_threadmemusage_t *pThreadUsage, ism_common_memoryType type, size_t size)
{
    if (size <= pThreadUsage->threadReservation[type])
    {
        pThreadUsage->threadReservation[type] -= size;
    }
    else
    {
        if (size >= ismmThreadMemChunkBytes)
        {
            ismm_increaseGlobalMemUsage(type, size);
        }
        else
        {
            ismm_increaseGlobalMemUsage(type, ismmThreadMemChunkBytes);
            pThreadUsage->threadReservation[type] += ismmThreadMemChunkBytes - size;
        }
    }
}

//*************************************************************************
/// @brief Decreases record of how much memory is used by a given type
//*************************************************************************
static inline void ismm_reduceGlobalMemUsage(ism_common_memoryType type, size_t size)
{
    DEBUG_ONLY size_t oldSize;
    oldSize = __sync_fetch_and_sub(&(memSizes[type]), size);
    assert(oldSize >= size);
}

static inline void ismm_reduceMemUsage(ism_threadmemusage_t *pThreadUsage, ism_common_memoryType type, size_t size)
{
    if (size >= ismmThreadMemChunkBytes)
    {
        ismm_reduceGlobalMemUsage(type, size);
    }
    else
    {
        pThreadUsage->threadReservation[type] += size;
        if (pThreadUsage->threadReservation[type] > ismmThreadMemChunkBytes)
        {
            ismm_reduceGlobalMemUsage(type, pThreadUsage->threadReservation[type] - ismmThreadMemChunkBytes);
            pThreadUsage->threadReservation[type] = ismmThreadMemChunkBytes;
        }
    }
}

//*************************************************************************
/// @brief returns how much memory the thread has reserved
///
/// @return amount of memory thread has reserved (but isn't using)
//*************************************************************************
static inline size_t ismm_getMemReservation(ism_threadmemusage_t *pThreadUsage, ism_common_memoryType type)
{
    return pThreadUsage->threadReservation[type];
}

//*************************************************************************
/// @brief Initialises per-thread memory accounting
///
/// @return Ptr to per-thread memory totals
//*************************************************************************
int32_t ism_common_initializeThreadMemUsage(void)
{
    int32_t rc = ISMRC_OK;

    if (ism_common_threaddata->memUsage == NULL)
    {
        ism_common_threaddata->memUsage = calloc(1, sizeof(ism_threadmemusage_t));
        if (ism_common_threaddata->memUsage == NULL)
        {
            rc = ISMRC_AllocateError;
        }
    }

    return rc;
}

//*************************************************************************
/// @brief Destroy per-thread memory accounting
//*************************************************************************
void ism_common_destroyThreadMemUsage(void)
{
    if (ism_common_threaddata->memUsage != NULL)
    {
        // Return any remaining reservations to the global pool
        for (uint32_t type=0; type<ism_common_mem_numtypes; type++)
        {
            size_t typeReservation = ism_common_threaddata->memUsage->threadReservation[type];

            if (typeReservation != 0)
            {
                ismm_reduceGlobalMemUsage(type, typeReservation);
            }
        }

        free(ism_common_threaddata->memUsage);
        ism_common_threaddata->memUsage = NULL;
    }
    if (ism_common_threaddata != NULL )
    {
        //The thread data will be freed by it's creator but we need to clear the pointer
        ism_common_threaddata = NULL;
    }
}

char *ism_common_strdup(uint32_t probe, const char *str)
{
    if (!str)
            return (char *)str;

    ism_common_memoryType type = ISM_GET_MEMORY_TYPE(probe);
    void *mem = NULL;
    size_t length = strlen(str) + 1;
    size_t size = (sizeof(char) * length) + sizeof(ism_common_Eyecatcher_t);

    if (ismm_isAllowedMemUsage(type, size)) {
        mem = malloc(size);
        if (mem != NULL) {
            if ( LIKELY(ism_common_threaddata != NULL) )
            {
                ismm_increaseMemUsage(ism_common_threaddata->memUsage, type,
                        malloc_usable_size(mem));
            }
            ism_common_Eyecatcher_t *eyeC = (ism_common_Eyecatcher_t *) mem;
            mem += sizeof(ism_common_Eyecatcher_t); //Tell the caller the memory starts after the eyeCatcher
            ism_common_setStructId(eyeC->StructId, ISM_MEM_STRUCTID);
            eyeC->memType = type;
            eyeC->point = ISM_GET_MEMORY_PROBEID(probe);
            strcpy(mem, str);
        } else {
            VTRACE(size, ISM_ERROR_TRACE,
                    "malloc failed: type %d (probe %d), size %lu\n", type,
                    ISM_GET_MEMORY_PROBEID(probe), size);
        }
    }
    return mem;
}

//*************************************************************************
/// @brief Wrapper (for allowing more granular monitoring) of malloc
/// @remark If wrappers are disabled this is defined to literally be malloc
//*************************************************************************
void *ism_common_malloc(uint32_t probe, size_t size)
{
    ism_common_memoryType type = ISM_GET_MEMORY_TYPE(probe);
    void *mem = NULL;

    //Work out how much extra we need for our eye-catcher...round up
    size += sizeof(ism_common_Eyecatcher_t);

    if (ismm_isAllowedMemUsage(type, size))
    {
        mem = malloc(size);

        if (mem != NULL)
        {
            if ( LIKELY(ism_common_threaddata != NULL) )
            {
                ismm_increaseMemUsage(ism_common_threaddata->memUsage, type, malloc_usable_size(mem));
            }

            ism_common_Eyecatcher_t *eyeC = (ism_common_Eyecatcher_t *)mem;
            mem += sizeof(ism_common_Eyecatcher_t); //Tell the caller the memory starts after the eyeCatcher
            ism_common_setStructId(eyeC->StructId, ISM_MEM_STRUCTID);
            eyeC->memType = type;
            eyeC->point = ISM_GET_MEMORY_PROBEID(probe);
        }
        else
        {
            VTRACE( size, ISM_ERROR_TRACE,
                   "malloc failed: type %d (probe %d), size %lu\n",
                   type, ISM_GET_MEMORY_PROBEID(probe), size);
        }
    }

    return mem;
}

//*************************************************************************
/// @brief Wrapper (for allowing more granular monitoring) of malloc
/// @remark If wrappers are disabled this is defined to literally be malloc
///
/// Should be used where an eyecatcher cannot be used eg mallocing a field
/// where that field may be allocated by the system elsewhere
/// Should be freed via ism_common_free_raw
//*************************************************************************
void *ism_common_malloc_noeyecatcher(size_t size)
{
    return malloc(size);
}

//*************************************************************************
/// @brief Wrapper (for allowing more granular monitoring) of calloc
/// @remark If wrappers are disabled this is defined to literally be calloc
//*************************************************************************
void *ism_common_calloc( uint32_t probe, size_t nmemb, size_t size)
{
    ism_common_memoryType type = ISM_GET_MEMORY_TYPE(probe);
    void *mem = NULL;

    //Work out how much extra we need for our eye-catcher...round up
    nmemb += (sizeof(ism_common_Eyecatcher_t) + (size -1)) /size;

    if (ismm_isAllowedMemUsage(type, (nmemb*size)))
    {
        mem = calloc(nmemb, size);

        if (mem != NULL)
        {
            if ( LIKELY(ism_common_threaddata != NULL) )
            {
                ismm_increaseMemUsage(ism_common_threaddata->memUsage, type, malloc_usable_size(mem));
            }

            ism_common_Eyecatcher_t *eyeC = (ism_common_Eyecatcher_t *)mem;
            mem += sizeof(ism_common_Eyecatcher_t); //Tell the caller the memory starts after the eyeCatcher
            ism_common_setStructId(eyeC->StructId, ISM_MEM_STRUCTID);
            eyeC->memType = type;
            eyeC->point = ISM_GET_MEMORY_PROBEID(probe);
        }
        else
        {
            VTRACE( nmemb*size, ISM_ERROR_TRACE,
                       "calloc failed: type %d (probeId %d), nmemb %lu, size %lu\n",
                       type, ISM_GET_MEMORY_PROBEID(probe), nmemb, size);
        }
    }

    return mem;
}

//*************************************************************************
/// @brief Confirm that an eyecatcher is correct take core dump if not for debug builds
//*************************************************************************
void ism_check(ism_common_memoryType type, void * mem)
{
    if (mem != NULL) {
        mem -= sizeof(ism_common_Eyecatcher_t); //The block we want to free starts at the eyeCatcher
        ism_common_Eyecatcher_t *eyeC = (ism_common_Eyecatcher_t *) mem;
        ism_common_checkStructId(eyeC->StructId, ISM_MEM_STRUCTID, CORE_DUMP_SOMETIMES,
                ismCommonFFDCPROBE_001);
        ism_common_checkId(eyeC->memType, type, 0, CORE_DUMP_SOMETIMES,
                        ismCommonFFDCPROBE_001);
    }
}

//*************************************************************************
/// @brief Confirm an eyecatcher is valid
/// Never triggers a core dump, primarily designed for unit tests
//*************************************************************************
bool ism_confirm_eyecatcher(void * mem)
{
    mem -= sizeof(ism_common_Eyecatcher_t); //The block we want to free starts at the eyeCatcher
    ism_common_Eyecatcher_t *eyeC = (ism_common_Eyecatcher_t *) mem;
    return ism_common_checkStructId(eyeC->StructId, ISM_MEM_STRUCTID, CORE_DUMP_NEVER, ismCommonFFDCPROBE_001);
}

//*************************************************************************
/// @brief Confirm an eyecatcher's memtype
/// Never triggers a core dump, primarily designed for unit tests
//*************************************************************************
bool ism_confirm_memType(ism_common_memoryType type, void * mem)
{
    mem -= sizeof(ism_common_Eyecatcher_t); //The block we want to free starts at the eyeCatcher
    ism_common_Eyecatcher_t *eyeC = (ism_common_Eyecatcher_t *) mem;
    return ism_common_checkId(eyeC->memType, type, eyeC->point, CORE_DUMP_NEVER, ismCommonFFDCPROBE_001);
}

//*************************************************************************
/// @brief Query memory reservation for a type
/// @remark desgined for unit tests - this is not the way to report memory usage
//*************************************************************************
size_t ism_common_queryReservation(ism_common_memoryType type)
{
    return ismm_getMemReservation(ism_common_threaddata->memUsage, type);
}

//*************************************************************************
/// @brief Wrapper (for allowing more granular monitoring) of realloc
/// @remark If wrappers are disabled this is defined to literally be realloc
//*************************************************************************
void *ism_common_realloc(uint32_t probe, void *ptr, size_t size)
{
    ism_common_memoryType type = ISM_GET_MEMORY_TYPE(probe);
    void *mem = NULL;

    //Check the thing we are changing is valid
    if (ptr != NULL)
    {
        ptr -= sizeof(ism_common_Eyecatcher_t); //The block we want to free resize at the eyeCatcher
        DEBUG_ONLY ism_common_Eyecatcher_t *eyeC = (ism_common_Eyecatcher_t *)ptr;
        assert(type == eyeC->memType);
    }

    //And allow space in realloc'd version (for eyecatcher)
    size += sizeof(ism_common_Eyecatcher_t);

    size_t oldSize = (ptr != NULL) ? malloc_usable_size(ptr) : 0;


    if ((size <= oldSize) || ismm_isAllowedMemUsage(type, (size - oldSize)))
    {
        mem = realloc(ptr, size);

        if (mem != NULL)
        {
            size_t newSize = malloc_usable_size(mem);

            if ( LIKELY(ism_common_threaddata != NULL) )
            {
                if(newSize > oldSize)
                {
                    ismm_increaseMemUsage(ism_common_threaddata->memUsage, type, (newSize - oldSize));
                }
                else if(oldSize > newSize)
                {
                    ismm_reduceMemUsage(ism_common_threaddata->memUsage, type, (oldSize - newSize));
                }
            }

            if (oldSize == 0)
            {
                //We need to add the eyeCatcher as it wasn't there before
                ism_common_Eyecatcher_t *eyeC = (ism_common_Eyecatcher_t *)mem;
                ism_common_setStructId(eyeC->StructId, ISM_MEM_STRUCTID);
                eyeC->memType = type;
                eyeC->point = ISM_GET_MEMORY_PROBEID(probe);
            }
            mem += sizeof(ism_common_Eyecatcher_t); //Tell the caller the memory starts after the eyeCatcher
        }
        else
        {
            VTRACE( size, ISM_ERROR_TRACE,
                       "realloc failed: type %d (probeId %d), ptr %p, size %lu\n",
                       type, ISM_GET_MEMORY_PROBEID(probe), ptr, size);
        }
    }

    return mem;
}

//*************************************************************************
/// @brief Wrapper (for allowing more granular monitoring) of malloc_usable_size
/// @remark If wrappers are disabled this is defined to literally be malloc_usable_size
//*************************************************************************
size_t ism_common_usable_size(void *mem)
{
    if (mem != NULL)
    {
        mem -= sizeof(ism_common_Eyecatcher_t);
        ism_common_Eyecatcher_t *eyeC = (ism_common_Eyecatcher_t *)mem;
        ism_common_checkStructId(eyeC->StructId, ISM_MEM_STRUCTID, CORE_DUMP_SOMETIMES, ismCommonFFDCPROBE_001);
        return ism_common_usable_size(mem)-sizeof(ism_common_Eyecatcher_t);
    }
    return malloc_usable_size(mem);
}

//*************************************************************************
/// @brief Return the full size of the memory area passed (including any
/// headers etc)
//*************************************************************************
size_t ism_common_full_size(void *mem)
{
    if (mem != NULL)
    {
        mem -= sizeof(ism_common_Eyecatcher_t);
        ism_common_Eyecatcher_t *eyeC = (ism_common_Eyecatcher_t *)mem;
        ism_common_checkStructId(eyeC->StructId, ISM_MEM_STRUCTID, CORE_DUMP_SOMETIMES, ismCommonFFDCPROBE_001);
    }

    return malloc_usable_size(mem);
}

//*************************************************************************
/// @brief Wrapper (for allowing more granular monitoring) of free
/// @remark If wrappers are disabled this is defined to literally be free
//*************************************************************************
void ism_common_free_location(ism_common_memoryType type, void *mem, const char * file, int lineno)
{
    //Check the thing we are changing is valid
    if (mem != NULL)
    {
        mem -= sizeof(ism_common_Eyecatcher_t); //The block we want to free starts at the eyeCatcher
        ism_common_Eyecatcher_t *eyeC = (ism_common_Eyecatcher_t *)mem;

        // If the eyecatcher check fails then trigger FFDC and leak the memory
        if (ism_common_checkStructIdLocation(eyeC->StructId, ISM_MEM_STRUCTID, CORE_DUMP_SOMETIMES, __FUNCTION__, ismCommonFFDCPROBE_001, file, lineno) )
        {
            // If the eyecatcher has the wrong type trigger FFDC and use the type in the
            // eyecatcher for the accounting

            if (!ism_common_checkIdLocation(eyeC->memType, type, eyeC->point, CORE_DUMP_SOMETIMES, __FUNCTION__, ismCommonFFDCPROBE_001, file, lineno) )
            {
                type = eyeC->memType;
            }

            ism_common_setStructId(eyeC->StructId, NOT_ISM_MEM_STRUCTID);

            if ( LIKELY(ism_common_threaddata != NULL) )
            {
                //The majority of the time ism_common_threaddata is set, the exception is ssl calling free during thread destruction
                // after we have freed the threaddata
                ismm_reduceMemUsage(ism_common_threaddata->memUsage, type, malloc_usable_size(mem));
            }
            free(mem);
        }
    }
}

//************************************************************************
/// @brief transfer memory usage from one type to another
/// @remark If wrappers are disabled this does nothing
//*************************************************************************
void ism_common_transfer_memory(ism_common_memoryType sourceType , ism_common_memoryType destType , void * mem)
{
    if (mem != NULL)
    {
        mem -= sizeof(ism_common_Eyecatcher_t); //The block we want to free starts at the eyeCatcher
        ism_common_Eyecatcher_t *eyeC = (ism_common_Eyecatcher_t *)mem;

        // If the eyecatcher check fails then trigger FFDC and do nothing
        if (ism_common_checkStructIdLocation(eyeC->StructId, ISM_MEM_STRUCTID, CORE_DUMP_SOMETIMES, __FUNCTION__, ismCommonFFDCPROBE_001, __FILE__, __LINE__) )
        {
            // If the location fails then trigger FFDC and don't reduce memory usage for source type but still set the type and increase usages for dest type
            if (ism_common_checkIdLocation(eyeC->memType, sourceType, eyeC->point, CORE_DUMP_SOMETIMES, __FUNCTION__, ismCommonFFDCPROBE_001, __FILE__, __LINE__))
            {
                if ( LIKELY(ism_common_threaddata != NULL) )
                {
                    //The majority of the time ism_common_threaddata is set, the exception is ssl calling free during thread destruction
                    // after we have freed the threaddata
                    ismm_reduceMemUsage(ism_common_threaddata->memUsage, sourceType, malloc_usable_size(mem));
                }
            }
            ismm_increaseMemUsage(ism_common_threaddata->memUsage, destType, malloc_usable_size(mem));

            eyeC->memType = destType;
        }
    }
}

//*************************************************************************
/// @brief Wrapper (for allowing more granular monitoring) of free
/// @remark If wrappers are disabled this is defined to literally be free
///         Allows freeing of structures whose origin can't be guarenteed
//*************************************************************************
void ism_common_free_anyType(void *mem) {
    ism_common_memoryType type;
    if (mem != NULL) {
        mem -= sizeof(ism_common_Eyecatcher_t); //The block we want to free starts at the eyeCatcher
        ism_common_Eyecatcher_t *eyeC = (ism_common_Eyecatcher_t *) mem;
        // If the eyecatcher check fails then trigger FFDC and leak the memory
        if (ism_common_checkStructId(eyeC->StructId, ISM_MEM_STRUCTID,
                CORE_DUMP_SOMETIMES, ismCommonFFDCPROBE_001)) {
            type = eyeC->memType;
            ism_common_setStructId(eyeC->StructId, NOT_ISM_MEM_STRUCTID);

            if ( LIKELY(ism_common_threaddata != NULL) )
            {
                //We don't bother to overwrite the eyecatcher as if MEMDEBUG is defined
                //we use  mallopt(M_PERTURB,...) which will overwrite on both malloc and free
                ismm_reduceMemUsage(ism_common_threaddata->memUsage, type,
                        malloc_usable_size(mem));
            }
            free(mem);
        }
    }
}

//*************************************************************************
/// @brief Wrapper of free for memory allocated using posix_memalign
/// @remark once posix_memalign is wrapped properly then this will adjust accounting
///        until then it just calls free
//*************************************************************************
void ism_common_free_memaligned(ism_common_memoryType type, void *mem)
{
    free(mem);
}

//*************************************************************************
/// @brief Wrapper of realloc for memory allocated using posix_memalign
/// @remark once posix_memalign is wrapped properly then this will adjust accounting
///        until then it just calls realloc
//*************************************************************************
void * ism_common_realloc_memaligned(ism_common_memoryType type, void *mem, size_t size)
{
    return realloc(mem, size);
}

//*************************************************************************
/// @brief Wrapper of free
/// @remark this is just to make it clear that the object has intentionally been freed
///        objects freed by this are often strings created using strdup
//*************************************************************************
void ism_common_free_raw(ism_common_memoryType type, void *mem)
{
    free(mem);
}

//*************************************************************************
/// @brief Wrapper for freeing a structure which also invalidates the strucId
/// @remark If wrappers are disabled this is defined to literally be free
//*************************************************************************
void ism_common_freeStruct(ism_common_memoryType type,
                      void *pStruct,
                      char *pStructId)
{
    if (pStruct != NULL)
    {
        assert(pStructId != NULL);
        ism_common_invalidateStructId(pStructId);
    }

    ism_common_free(type, pStruct);
}

//****************************************************************************
/// @brief Query how much memory is used for each memory type
///
/// @param[out]  typeLevels[ism_common_mem_numtypes]    buffer to store the memory levels in
///                                                     for individual memoryTypes
//*******************************************************************************
void ism_common_queryControlledMemory( size_t typeLevels[ism_common_mem_numtypes])
{
    //Copy the live type info
    memcpy(typeLevels,
           memSizes,
          (ism_common_mem_numtypes * sizeof(size_t)));
}

//****************************************************************************
/// @brief Query how much memory is used by the engine for each memory group
///
/// @param[in]   typeLevels[ism_common_mem_numtypes]   data about the individual types
/// @param[out]  groupLevels[ism_common_mem_numtypes]  buffer for the rolled up levels for
///                                                    memory groups
//*******************************************************************************
void ism_common_queryGroupsMemory( size_t typeLevels[ism_common_mem_numtypes], size_t groupLevels[ism_common_mem_numgroups])
{
    //Clear the group info
    memset(groupLevels, 0, (ism_common_mem_numgroups * sizeof(size_t)));

    //Add up the stats for the groups
    for (uint32_t i = 0; i < ism_common_mem_numtypes; i++)
    {
        groupLevels[ism_common_memTypeInfo[i].group] += typeLevels[i];
    }
}

//****************************************************************************
/// @brief Query total amount of memory used by the engine
///
/// @returns  Amount of memory allocated for use in the engine
//*******************************************************************************

size_t ism_common_queryTotalControlledMemory(void)
{
    size_t total = 0;

    for (uint32_t i = 0; i < ism_common_mem_numtypes; i++)
    {
        total += memSizes[i];
    }

    return total;
}


//****************************************************************************
/// @brief Enables/Disabled mallocs for a given type of memory
///
/// @param[in]  type     type of memory to allow/disallow
/// @param[in]  allowed  true - mallocs allowed false: mallocs disabled
//****************************************************************************

void ism_common_setMallocStateForType(ism_common_memoryType type,
                                      bool allowed)
{
    preventMallocs[type] = !allowed;
}

//****************************************************************************
/// @brief Sets minimum size of chunks in which memory is reserved in the memHandler
///
/// @param[in]  newChunkSizeBytes
///
/// @remark Should only be used in unit tests
//****************************************************************************
void ism_common_setMemChunkSize(uint32_t newChunkSizeBytes)
{
    VTRACE(newChunkSizeBytes, 5, "Setting ismmThreadMemChunkBytes to %u bytes\n", newChunkSizeBytes);
    ismmThreadMemChunkBytes = newChunkSizeBytes;
}
//****************************************************************************
/// @brief Gets minimum size of chunks in which memory is reserved in the memHandler
/// @remark Should only be used in unit tests
//****************************************************************************
uint32_t ism_common_getMemChunkSize(void)
{
    VTRACE(ismmThreadMemChunkBytes, 6, "Getting ismmThreadMemChunkBytes\n");
    return ismmThreadMemChunkBytes;
}

void ism_common_startMemDebugging(void)
{
    bool memdebugging = false; //Is there any indication that someone is debugging
                               //a memory issue
#ifdef MEMDEBUG
    memdebugging = true;  //They turned on a compile flag that says they are!
#endif

    if(getenv("ISM_DEBUG_MEMORY") != NULL)
    {
        memdebugging = true;
    }

    if (memdebugging)
    {
        TRACE(5, "IEMEM_DEBUG: Causing memory to be initialised to non-empty on malloc and free\n");
        mallopt(M_PERTURB, 0xDEADBEEF); //initialise memory to non-zero when malloc'd/free'd

        if (getenv("MALLOC_CHECK_") == NULL)
        {
            TRACE(5, "IEMEM_DEBUG: WARNING MALLOC_CHECK_ unset. Setting this env var to 3 is handy for memory debugging\n");
        }
    }

    if (getenv("MALLOC_TRACE") != NULL)
    {
        TRACE(5, "IEMEM_DEBUG: Starting malloc trace...\n");
        mtrace();
    }
    else if (memdebugging)
    {
        TRACE(5, "IEMEM_DEBUG: WARNING Not starting malloc trace (to use set MALLOC_TRACE env) to a filename\n");
    }
}

//***********************************************************************
/// @brief Initialise memory handler
///
/// @remark NOTE: To start calling iemem_ functions to allocate and free
/// memory the thread must be initialised via a call to ism_engine_threadInit.
//***********************************************************************
void ism_common_initMemHandler(void)
{
    TRACE(7, "In %s\n", __func__);

    //Do some debug related things based on compile flags/env vars
    ism_common_startMemDebugging();
}

//***********************************************************************
/// @brief Clean up any resources left behind by memory monitoring
//***********************************************************************
void ism_common_termMemHandler(void)
{
    // No resources allocated any more - but keep the function for possible future
    // requirements.
    VTRACE(0, 7, "%s\n", __func__);
}

#else //else !COMMON_MALLOC_WRAPPER
void ism_common_initMemHandler(void)
{
}


void ism_common_termMemHandler(void)
{
}

#endif // end ifdef COMMON_MALLOC_WRAPPER


//****************************************************************************
/// @internal
///
/// @brief Write zeros into every memory page of the buffer (assumed to be 4k)
/// so the OS definitely gives us the memory for them.
///
/// @param[in] area       Area to write zeros to
/// @param[in] areaBytes  Size of area to write zeros to
///
/// @return void
//****************************************************************************
void ism_common_mem_touch(void *area, uint64_t areaBytes)
{
    char *areaptr = (char *)area;

    for (uint64_t i = 0; i < areaBytes; i += 4*1024)
    {
        areaptr[i] = 0;
    }
}


void ism_common_traceMemoryStatistics( int32_t TraceLevel )
{
#ifdef COMMON_MALLOC_WRAPPER
    ism_MemoryStatistics_t memoryStats;

    int32_t rc = ism_common_getMemoryStatistics(&memoryStats);

    if (rc != ISMRC_OK)
    {
        ism_common_setError(rc);
    }
    else
    {
        uint32_t groupId = 0;
        for (groupId = 0; groupId < ism_common_mem_numgroups; groupId++) {

            if ( ism_common_getPrintableFromGroup(groupId) ) {
                uint64_t total = memoryStats.groups[groupId];
                TRACE(TraceLevel, "Memory Group(%s) Used(%lu)\n", ism_common_getMemoryGroupName(groupId), total);
                if ( total > 0 ) {
                    uint32_t typeId = 0;
                    for (typeId = 0; typeId < ism_common_mem_numtypes; typeId++) {
                        if (ism_common_getMemoryGroupFromType(typeId) == groupId)
                            TRACE(TraceLevel, "    Memory Type(%s) Used(%lu)\n", ism_common_getMemoryTypeName(typeId), memoryStats.types[typeId]);{
                        }
                    }
                }
            }
        }
    }
#endif
    return;
}

#ifdef COMMON_MALLOC_WRAPPER
//****************************************************************************
/// @internal
///
/// @brief Get memory statistics
///
/// @param[out] pStatistics       Memory statistics
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t ism_common_getMemoryStatistics(ism_MemoryStatistics_t *pStatistics)
{
    int32_t rc = ISMRC_OK;

    memset(pStatistics, 0, sizeof(*pStatistics));

    ism_common_queryControlledMemory(pStatistics->types);
    ism_common_queryGroupsMemory(pStatistics->types, pStatistics->groups);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief Get memory group name from the memInfo structure
/// @param id       group ID to get the name
///
/// @return group's name
//****************************************************************************
XAPI const char * ism_common_getMemoryGroupName(uint32_t id)
{
	assert(id < ism_common_mem_numgroups);
    return ism_common_memInfo[id].description;
}

//****************************************************************************
/// @internal
///
/// @brief Get if a memory group should be printed
/// @param id       group ID to look up
///
/// @return if group is printable
//****************************************************************************
XAPI bool ism_common_getPrintableFromGroup(uint32_t id)
{
    assert(id < ism_common_mem_numgroups);
    return ism_common_memInfo[id].printable;
}


//****************************************************************************
/// @internal
///
/// @brief Get memory type name from the memInfo structure
/// @param id       type ID to get the name
///
/// @return type's name
//****************************************************************************
XAPI const char * ism_common_getMemoryTypeName(uint32_t id)
{
	assert(id < ism_common_mem_numtypes);
    return ism_common_memTypeInfo[id].description;
}


ism_common_memoryGroup ism_common_getMemoryGroupFromType(uint32_t id)
{
    assert(id < ism_common_mem_numtypes);
    return ism_common_memTypeInfo[id].group;
}


#endif
