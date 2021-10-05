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
#define TRACE_COMP Engine

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

#include "engineInternal.h"
#include "memHandler.h"
#include "ismutil.h"
#include "engineUtils.h"
#include "engineAsync.h"

//When we return memory back to the system, leave us with this percentage of the
//total memory for our use.
#define IEMEM_TRIM_PADDING_PERCENT 2

#ifndef NO_MALLOC_WRAPPER

//array to store whether allocations are allowed for each memory type
static bool preventMallocs[iemem_numtypes] = { 0 };

//array that stores how much memory (in bytes) is in use by each memory type
static size_t memSizes[iemem_numtypes] = { 0 };

//array that stores which function is registered (if any) to reduce memory usage
//for each memory type
static iemem_reduceCallback_t memReduceCBs[iemem_numtypes] = {0};

//When we have a repeating timer running watching memory levels,
//this key is used to cancel the timer
static ism_timer_t memCheckTimerKey = 0;

//We need to ensure we are not in the midst of a callback when we cancel the timer
//this counter is used to do that.
static uint32_t memCheckActiveTimerUseCount = 0;

//How much memory of each type a thread reserves for itself in one go: 512k
static uint32_t iememThreadMemChunkBytes = (512L * 1024L);

//The current overall memory state
static iememMemoryLevel_t currState = iememPlentifulMemory;

// Set up the structure that defines the comment for each mem type and how readily we should disable allocs
#define IEMEM_MEMHANDLER_RICH_TYPE_STRUCTURE
#include <memHandlerTypes.h>
#undef IEMEM_MEMHANDLER_RICH_TYPE_STRUCTURE

//*************************************************************************
/// @brief Checks whether a memory allocation of given type+size should be allowed
///
/// @param[in] type  - memory type of hypothetical memory
/// @param[in] size  - size of hypothetical memory in bytes
///
/// @return true - allocation should be allowed (false = disallowed)
//*************************************************************************
static inline bool iemem_isAllowedMemUsage(ieutThreadData_t *pThreadData,
                                           iemem_memoryType type,
                                           size_t size)
{
    bool increaseAllowed = true;

    if (preventMallocs[type])
    {
        ieutTRACEL(pThreadData, type, ENGINE_NORMAL_TRACE,
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
static inline void iemem_increaseGlobalMemUsage(iemem_memoryType type, size_t size)
{
    __sync_add_and_fetch(&(memSizes[type]), size);
}

static inline void iemem_increaseMemUsage(iememThreadMemUsage_t *pThreadUsage, iemem_memoryType type, size_t size)
{
    if (size <= pThreadUsage->threadReservation[type])
    {
        pThreadUsage->threadReservation[type] -= size;
    }
    else
    {
        if (size >= iememThreadMemChunkBytes)
        {
            iemem_increaseGlobalMemUsage(type, size);
        }
        else
        {
            iemem_increaseGlobalMemUsage(type, iememThreadMemChunkBytes);
            pThreadUsage->threadReservation[type] += iememThreadMemChunkBytes - size;
        }
    }
}

//*************************************************************************
/// @brief Decreases record of how much memory is used by a given type
//*************************************************************************
static inline void iemem_reduceGlobalMemUsage(iemem_memoryType type, size_t size)
{
    DEBUG_ONLY size_t oldSize;
    oldSize = __sync_fetch_and_sub(&(memSizes[type]), size);
    assert(oldSize >= size);
}

static inline void iemem_reduceMemUsage(iememThreadMemUsage_t *pThreadUsage, iemem_memoryType type, size_t size)
{
    if (size >= iememThreadMemChunkBytes)
    {
        iemem_reduceGlobalMemUsage(type, size);
    }
    else
    {
        pThreadUsage->threadReservation[type] += size;
        if (pThreadUsage->threadReservation[type] > iememThreadMemChunkBytes)
        {
            iemem_reduceGlobalMemUsage(type, pThreadUsage->threadReservation[type] - iememThreadMemChunkBytes);
            pThreadUsage->threadReservation[type] = iememThreadMemChunkBytes;
        }
    }
}

//*************************************************************************
/// @brief Initialises per-thread memory accounting
///
/// @return Ptr to per-thread memory totals
//*************************************************************************
int32_t iemem_initializeThreadMemUsage(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    if (pThreadData->memUsage == NULL)
    {
        pThreadData->memUsage = calloc(1, sizeof(iememThreadMemUsage_t));
        if (pThreadData->memUsage == NULL)
        {
            rc = ISMRC_AllocateError;
        }
    }

    return rc;
}

//*************************************************************************
/// @brief Destroy per-thread memory accounting
//*************************************************************************
void iemem_destroyThreadMemUsage(ieutThreadData_t *pThreadData)
{
    if (pThreadData->memUsage != NULL)
    {
        // Return any remaining reservations to the global pool
        for (uint32_t type=0; type<iemem_numtypes; type++)
        {
            size_t typeReservation = pThreadData->memUsage->threadReservation[type];

            if (typeReservation != 0)
            {
                iemem_reduceGlobalMemUsage(type, typeReservation);
            }
        }

        free(pThreadData->memUsage);
        pThreadData->memUsage = NULL;
    }
}

//*************************************************************************
/// @brief Wrapper (for allowing more granular monitoring) of malloc
/// @remark If wrappers are disabled this is defined to literally be malloc
//*************************************************************************
void *iemem_malloc(ieutThreadData_t *pThreadData, uint32_t probe, size_t size)
{
    iemem_memoryType type = IEMEM_GET_MEMORY_TYPE(probe);
    void *mem = NULL;

#ifdef MEMDEBUG
    //Work out how much extra we need for our eye-catcher...round up
    size += sizeof(iememEyecatcher_t);
#endif

    if (iemem_isAllowedMemUsage(pThreadData, type, size))
    {
        mem = malloc(size);

        if (mem != NULL)
        {
            iemem_increaseMemUsage(pThreadData->memUsage, type, malloc_usable_size(mem));
#ifdef MEMDEBUG
            iememEyecatcher_t *eyeC = (iememEyecatcher_t *)mem;
            mem += sizeof(iememEyecatcher_t); //Tell the caller the memory starts after the eyeCatcher
            ismEngine_SetStructId(eyeC->StructId, IEMEM_STRUCTID);
            eyeC->memType = type;
#endif
        }
        else
        {
            ieutTRACEL(pThreadData, size, ENGINE_ERROR_TRACE,
                       "malloc failed: type %d (probe %d), size %lu\n",
                       type, IEMEM_GET_MEMORY_PROBEID(probe), size);
        }
    }

    return mem;
}

//*************************************************************************
/// @brief Wrapper (for allowing more granular monitoring) of calloc
/// @remark If wrappers are disabled this is defined to literally be calloc
//*************************************************************************
void *iemem_calloc(ieutThreadData_t *pThreadData, uint32_t probe, size_t nmemb, size_t size)
{
    iemem_memoryType type = IEMEM_GET_MEMORY_TYPE(probe);
    void *mem = NULL;

#ifdef MEMDEBUG
    //Work out how much extra we need for our eye-catcher...round up
    nmemb += (sizeof(iememEyecatcher_t) + (size -1)) /size;
#endif

    if (iemem_isAllowedMemUsage(pThreadData, type, (nmemb*size)))
    {
        mem = calloc(nmemb, size);

        if (mem != NULL)
        {
            iemem_increaseMemUsage(pThreadData->memUsage, type, malloc_usable_size(mem));
#ifdef MEMDEBUG
            iememEyecatcher_t *eyeC = (iememEyecatcher_t *)mem;
            mem += sizeof(iememEyecatcher_t); //Tell the caller the memory starts after the eyeCatcher
            ismEngine_SetStructId(eyeC->StructId, IEMEM_STRUCTID);
            eyeC->memType = type;
#endif
        }
        else
        {
            ieutTRACEL(pThreadData, nmemb*size, ENGINE_ERROR_TRACE,
                       "calloc failed: type %d (probeId %d), nmemb %lu, size %lu\n",
                       type, IEMEM_GET_MEMORY_PROBEID(probe), nmemb, size);
        }
    }

    return mem;
}

//*************************************************************************
/// @brief Wrapper (for allowing more granular monitoring) of realloc
/// @remark If wrappers are disabled this is defined to literally be realloc
//*************************************************************************
void *iemem_realloc(ieutThreadData_t *pThreadData, uint32_t probe, void *ptr, size_t size)
{
    iemem_memoryType type = IEMEM_GET_MEMORY_TYPE(probe);
    void *mem = NULL;

#ifdef MEMDEBUG
    //Check the thing we are changing is valid
    if (ptr != NULL)
    {
        ptr -= sizeof(iememEyecatcher_t); //The block we want to free resize at the eyeCatcher
        DEBUG_ONLY iememEyecatcher_t *eyeC = (iememEyecatcher_t *)ptr;
        assert(type == eyeC->memType);
    }

    //And allow space in realloc'd version (for eyecatcher)
    size += sizeof(iememEyecatcher_t);
#endif
    size_t oldSize = (ptr != NULL) ? malloc_usable_size(ptr) : 0;


    if ((size <= oldSize) || iemem_isAllowedMemUsage(pThreadData, type, (size - oldSize)))
    {
        mem = realloc(ptr, size);

        if (mem != NULL)
        {
            size_t newSize = malloc_usable_size(mem);

            if(newSize > oldSize)
            {
                iemem_increaseMemUsage(pThreadData->memUsage, type, (newSize - oldSize));
            }
            else if(oldSize > newSize)
            {
                iemem_reduceMemUsage(pThreadData->memUsage, type, (oldSize - newSize));
            }
#ifdef MEMDEBUG
            if (oldSize == 0)
            {
                //We need to add the eyeCatcher as it wasn't there before
                iememEyecatcher_t *eyeC = (iememEyecatcher_t *)mem;
                ismEngine_SetStructId(eyeC->StructId, IEMEM_STRUCTID);
                eyeC->memType = type;
            }
            mem += sizeof(iememEyecatcher_t); //Tell the caller the memory starts after the eyeCatcher
#endif
        }
        else
        {
            ieutTRACEL(pThreadData, size, ENGINE_ERROR_TRACE,
                       "realloc failed: type %d (probeId %d), ptr %p, size %lu\n",
                       type, IEMEM_GET_MEMORY_PROBEID(probe), ptr, size);
        }
    }

    return mem;
}

//*************************************************************************
/// @brief Wrapper (for allowing more granular monitoring) of malloc_usable_size
/// @remark If wrappers are disabled this is defined to literally be malloc_usable_size
//*************************************************************************
size_t iemem_usable_size(iemem_memoryType type, void *mem)
{
#ifdef MEMDEBUG
    if (mem != NULL)
    {
        mem -= sizeof(iememEyecatcher_t);
        iememEyecatcher_t *eyeC = (iememEyecatcher_t *)mem;
        ismEngine_CheckStructId(eyeC->StructId, IEMEM_STRUCTID, ieutPROBE_001);
        assert(type == iemem_numtypes || type == eyeC->memType);
        return malloc_usable_size(mem)-sizeof(iememEyecatcher_t);
    }
#endif

    return malloc_usable_size(mem);
}

//*************************************************************************
/// @brief Return the full size of the memory area passed (including any
/// headers etc)
//*************************************************************************
size_t iemem_full_size(iemem_memoryType type, void *mem)
{
#ifdef MEMDEBUG
    if (mem != NULL)
    {
        mem -= sizeof(iememEyecatcher_t);
        iememEyecatcher_t *eyeC = (iememEyecatcher_t *)mem;
        ismEngine_CheckStructId(eyeC->StructId, IEMEM_STRUCTID, ieutPROBE_001);
        assert(type == iemem_numtypes || type == eyeC->memType);
    }
#endif

    return malloc_usable_size(mem);
}

//*************************************************************************
/// @brief Wrapper (for allowing more granular monitoring) of free
/// @remark If wrappers are disabled this is defined to literally be free
//*************************************************************************
void iemem_free(ieutThreadData_t *pThreadData, iemem_memoryType type, void *mem)
{
#ifdef MEMDEBUG
    //Check the thing we are changing is valid
    if (mem != NULL)
    {
        mem -= sizeof(iememEyecatcher_t); //The block we want to free starts at the eyeCatcher
        iememEyecatcher_t *eyeC = (iememEyecatcher_t *)mem;
        ismEngine_CheckStructId(eyeC->StructId, IEMEM_STRUCTID, ieutPROBE_001);
        assert(type == eyeC->memType);
    }

    //We don't bother to overwrite the eyecatcher as if MEMDEBUG is defined
    //we use  mallopt(M_PERTURB,...) which will overwrite on both malloc and free
#endif
    iemem_reduceMemUsage(pThreadData->memUsage, type, malloc_usable_size(mem));
    free(mem);
}

//*************************************************************************
/// @brief Wrapper for freeing a structure which also invalidates the strucId
/// @remark If wrappers are disabled this is defined to literally be free
//*************************************************************************
void iemem_freeStruct(ieutThreadData_t *pThreadData,
                      iemem_memoryType type,
                      void *pStruct,
                      char *pStructId)
{
    if (pStruct != NULL)
    {
        assert(pStructId != NULL);
        ismEngine_InvalidateStructId(pStructId);
    }

    iemem_free(pThreadData, type, pStruct);
}

//****************************************************************************
/// @brief Query how much memory is used by the engine for each memory type
///
/// @param[out]  typeLevels[iemem_numtypes]    buffer to store the memory levels in
///                                            for individual memoryTypes
//*******************************************************************************
void iemem_queryControlledMemory( size_t typeLevels[iemem_numtypes])
{
    //Copy the live type info
    memcpy(typeLevels,
           memSizes,
          (iemem_numtypes * sizeof(size_t)));
}

//****************************************************************************
/// @brief Query how much memory is used by the engine for each memory group
///
/// @param[in]   typeLevels[iemem_numtypes]    data about the individual types
/// @param[out]  groupLevels[iemem_numgroups]  buffer for the rolled up levels for
///                                            memory groups
//*******************************************************************************
void iemem_queryGroupsMemory( size_t typeLevels[iemem_numtypes], size_t groupLevels[iemem_numgroups])
{
    //Clear the group info
    memset(groupLevels, 0, (iemem_numgroups * sizeof(size_t)));

    //Add up the stats for the groups
    for (uint32_t i = 0; i < iemem_numtypes; i++)
    {
        groupLevels[iememMemTypeInfo[i].group] += typeLevels[i];
    }
}

//****************************************************************************
/// @brief Query total amount of memory used by the engine
///
/// @returns  Amount of memory allocated for use in the engine
//*******************************************************************************

size_t iemem_queryTotalControlledMemory(void)
{
    size_t total = 0;

    for (uint32_t i = 0; i < iemem_numtypes; i++)
    {
        total += memSizes[i];
    }

    return total;
}

//****************************************************************************
/// @brief Query the current overall malloc state memory level
///
/// @returns  Amount current memory level in place
//*******************************************************************************

iememMemoryLevel_t iemem_queryCurrentMallocState(void)
{
    return currState;
}

//****************************************************************************
/// @brief Registers a function to be called when memory needs to be reduced
///
/// @param[in]  type     type of memory that can be reduced by this callback
/// @param[in]  callback function to call (of type iemem_reduceCallback_t )
///
/// @see iemem_reduceCallback_t
///
/// @remarks the algorithm for when to call the callback is currently very
/// crude and likely to change...
//****************************************************************************

void iemem_setMemoryReduceCallback(iemem_memoryType type,
                                   iemem_reduceCallback_t callback)
{
    memReduceCBs[type] = callback;
}

//****************************************************************************
/// @brief Enables/Disabled mallocs for a given type of memory
///
/// @param[in]  type     type of memory to allow/disallow
/// @param[in]  allowed  true - mallocs allowed false: mallocs disabled
//****************************************************************************

void iemem_setMallocStateForType(iemem_memoryType type,
                                 bool allowed)
{
    preventMallocs[type] = !allowed;
}

//****************************************************************************
/// @brief Enables/Disabled mallocs for all monitored memory types
///
/// @param[in]  memoryLevel - a constant that is used to determine how low we are on memory...
//****************************************************************************
void iemem_setMallocState(iememMemoryLevel_t memoryLevel)
{
    int i;

    TRACE(ENGINE_WORRYING_TRACE, "Changing malloc state to %u\n", memoryLevel);
    for (i=0; i<iemem_numtypes; i++)
    {
        bool prevented = false;

        if (memoryLevel == iememEmergencyMemory)
        {
            if (  (iememMemTypeInfo[i].behaviour == iememDuringLowMemDisableEarly)
                ||(iememMemTypeInfo[i].behaviour == iememDuringLowMemDisable))
            {
                prevented = true;
            }
        }
        else if (memoryLevel == iememDisableEarly)
        {
            if (iememMemTypeInfo[i].behaviour == iememDuringLowMemDisableEarly)
            {
                prevented = true;
            }
        }
        else if (memoryLevel == iememDisableAll)
        {
            if (iememMemTypeInfo[i].behaviour != iememDuringLowMemALWAYSEnable)
            {
                prevented = true;
            }
        }
        preventMallocs[i] = prevented;
    }
}


//****************************************************************************
/// @brief Sets minimum size of chunks in which memory is reserved in the memHandler
///
/// @param[in]  newChunkSizeBytes
///
/// @remark Should only be used in unit tests
//****************************************************************************
void iemem_setMemChunkSize(uint32_t newChunkSizeBytes)
{
    TRACE(ENGINE_WORRYING_TRACE, "Setting iememThreadMemChunkBytes to %u bytes\n", newChunkSizeBytes);
    iememThreadMemChunkBytes = newChunkSizeBytes;
}
//****************************************************************************
/// @brief Gets minimum size of chunks in which memory is reserved in the memHandler
/// @remark Should only be used in unit tests
//****************************************************************************
uint32_t iemem_getMemChunkSize(void)
{
    TRACE(ENGINE_WORRYING_TRACE, "Getting iememThreadMemChunkBytes\n");
    return iememThreadMemChunkBytes;
}

//****************************************************************************
/// @brief Calls applicable callbacks to try to reduce memory usage
///
/// @remark Currently very crude
//****************************************************************************
void iemem_reduceMemoryUsage(iememMemoryLevel_t prevState,
                             iemem_systemMemInfo_t *sysMemInfo)
{
    int i;
    size_t levels[iemem_numtypes];

    iemem_queryControlledMemory(levels);

    for (i=0; i<iemem_numtypes; i++)
    {
        //At the moment, we just try and reduce memory if it's non-zero
        //We need to do something smarter, which memory levels are higher
        //than "expected", which can easily be reduced etc...
        if (levels[i] > 0)
        {
            if(memReduceCBs[i] != NULL)
            {
                memReduceCBs[i](currState, prevState, i, levels[i], sysMemInfo);
            }
        }
    }
}

//**********************************************************************************************
/// @brief Decides based on free memory in the system, how severely constained mallocs ought to be
///
/// @param[in]    sysmeminfo               - how much free memory there is in the system
/// @param[out]   memoryUsageRecordPeriod  - how often memory usage should be trace out (in units of .5seconds)
/// @param[out]   memoryDetailRecordPeriod - how often detailed memory usage should be trace out (in units of .5seconds)
/// @param[out]   memoryTrimPeriod         - how often we should try to return memory to OS
/// @param[out]   traceLevel               - the level at which to write trace
///
/// @returns one of the iememMemoryLevel_t enum values
//*************************************************************************************************
static inline iememMemoryLevel_t iemem_selectMemoryLevel( iemem_systemMemInfo_t *sysmeminfo
                                                        , uint64_t *memoryUsageRecordPeriod
                                                        , uint64_t *memoryDetailRecordPeriod
                                                        , uint64_t *memoryTrimPeriod
                                                        , int32_t *traceLevel)
{
    uint8_t freePercentage = sysmeminfo->freeIncPercentage;
    iememMemoryLevel_t selectedLevel;

    if (ismEngine_serverGlobal.runPhase < EnginePhaseRunning)
    {
        // During restart we do not want to start limiting memory
        selectedLevel             = iememPlentifulMemory;

        if (freePercentage < IEMEM_FREEMEM_DISABLEEARLYTHRESHOLD)
        {
            *memoryUsageRecordPeriod  = 2; //trace often
            *memoryDetailRecordPeriod = 120; //trace detail every 60 seconds
            *memoryTrimPeriod         = 60;  //malloc trim every 30 seconds
            *traceLevel = ENGINE_WORRYING_TRACE;
        }
        else
        {
            // Otherwise during restart we trace detailed memory stats
            // every 60 seconds
            *memoryUsageRecordPeriod  = 120; //trace every 60 seconds
            *memoryDetailRecordPeriod = 120; //trace detail every 60 seconds
            *memoryTrimPeriod         = 1200;  //malloc trim every 10 mins
            *traceLevel = ENGINE_NORMAL_TRACE;
        }
    }
    else if (freePercentage <  IEMEM_FREEMEM_EMERGENCYTHRESHOLD)
    {
        selectedLevel             = iememEmergencyMemory;
        *memoryUsageRecordPeriod  = 0; //trace every time the memoryhandler runs
        *memoryDetailRecordPeriod = 120; //trace detail every 60 seconds
        *memoryTrimPeriod         = 10; //trim every 5 seconds
        *traceLevel = ENGINE_WORRYING_TRACE;
    }
    else if (freePercentage < IEMEM_FREEMEM_DISABLEEARLYTHRESHOLD)
    {
        selectedLevel             = iememDisableEarly;
        *memoryUsageRecordPeriod  = 2; //trace often
        *memoryDetailRecordPeriod = 120; //trace detail every 60 seconds
        *memoryTrimPeriod         = 10; //trim every 5 seconds
        *traceLevel = ENGINE_WORRYING_TRACE;
    }
    else if (freePercentage < IEMEM_FREEMEM_REDUCETHRESHOLD)
    {
        selectedLevel             = iememReduceMemory;
        *memoryUsageRecordPeriod  = 120; //trace every 60 seconds
        *memoryDetailRecordPeriod = 1200; //trace every 10 minutes
        *memoryTrimPeriod         = 120; //trim every 60 seconds
        *traceLevel = ENGINE_NORMAL_TRACE;
    }
    else
    {
        //We have loads of free memory...
        selectedLevel             = iememPlentifulMemory;
        *memoryUsageRecordPeriod  = 120; //trace every 60 seconds
        *memoryDetailRecordPeriod = 0; //don't trace out detailed memory usage
        *memoryTrimPeriod         = 0; //don't call malloc_trim
        *traceLevel = ENGINE_NORMAL_TRACE;
    }

    return selectedLevel;
}

//****************************************************************************
/// @internal
///
/// @brief Trace memory statistics
//****************************************************************************
static inline void ism_engine_traceMemoryStatistics( iemem_systemMemInfo_t *system_meminfo, bool Detail, int32_t TraceLevel )
{
    size_t levels[iemem_numtypes];
    int32_t index;

    // Trace the current information in effect from imposed limits
    TRACE(TraceLevel, "Effective Memory Total(%lu) Free(%lu) FromCgroup(%s) Virtual(%lu) Resident(%lu)\n",
          system_meminfo->effectiveMemoryBytes,
          system_meminfo->freeIncBuffersCacheBytes,
          system_meminfo->effectiveFromCgroup ? "True" : "False",
          system_meminfo->processVirtualMemorySize * 4096,
          system_meminfo->processResidentSetSize * 4096);

    if (Detail)
    {
        TRACE(TraceLevel, "System Memory Total(%lu) CurrentFree(%lu)\n",
              system_meminfo->totalMemoryBytes,
              system_meminfo->currentFreeBytes);
        TRACE(TraceLevel, "CGroup Memory Limit(%lu) Usage(%lu) Free(%lu)\n",
              system_meminfo->cgroupMemInfo.limitInBytes,
              system_meminfo->cgroupMemInfo.usageInBytes,
              system_meminfo->cgroupMemInfo.freeBytes);

        iemem_queryControlledMemory(levels);

        for (index =0; index < iemem_numtypes; index++)
        {
            TRACE(TraceLevel, "Memory Type(%s) Used(%lu)\n", iememMemTypeInfo[index].description, levels[index]);
        }
    }

    return;
}

#ifdef HAINVESTIGATIONSTATS

void tracePerThreadHAInvestigationStats( ieutThreadData_t *pThreadData
                                       , void *context)
{
    TRACE(4,"TEMPSTATS: id %u pThreadData %p (type %u)\n",
            pThreadData->engineThreadId, pThreadData, pThreadData->threadType);
    TRACE(4,"TEMPSTATS1: syncCommits %lu syncRollbacks %lu asyncCommits %lu\n",
             pThreadData->stats.syncCommits,
             pThreadData->stats.syncRollbacks,
             pThreadData->stats.asyncCommits);
    if (pThreadData->stats.numDeliveryBatches > 0)
    {
        TRACE(4,"TEMPSTATS2: numBatches %lu, min: %lu, max: %lu, avg: %2.2f\n",
                 pThreadData->stats.numDeliveryBatches,
                 pThreadData->stats.minDeliveryBatchSize,
                 pThreadData->stats.maxDeliveryBatchSize,
                 pThreadData->stats.avgDeliveryBatchSize);
    }
    else
    {
        TRACE(4,"TEMPSTATS2: numBatches 0\n");
    }
    if(    (pThreadData->stats.messagesDeliveryCancelled   > 0)
        || (pThreadData->stats.perConsumerFlowControlCount > 0)
        || (pThreadData->stats.perSessionFlowControlCount  > 0))
    {
        TRACE(4, "TEMPSTATS3: msgDlvryCancelled %lu, perConsumerFlowControl: %lu, perSessionFlowControl: %lu\n",
                 pThreadData->stats.messagesDeliveryCancelled,
                 pThreadData->stats.perConsumerFlowControlCount,
                 pThreadData->stats.perSessionFlowControlCount);
    }
    TRACE(4, "TEMPSTATS5: startDel %lu, startLoc: %lu, delwithJob: %lu delNoJob: %lu delSchedCW:%lu delCalledCW %lu\n",
                     pThreadData->stats.asyncMsgBatchStartDeliver,
                     pThreadData->stats.asyncMsgBatchStartLocate,
                     pThreadData->stats.asyncMsgBatchDeliverWithJobThread,
                     pThreadData->stats.asyncMsgBatchDeliverNoJobThread,
                     pThreadData->stats.asyncMsgBatchDeliverScheduledCW,
                     pThreadData->stats.asyncMsgBatchDeliverDirectCallCW);
    TRACE(4, "TEMPSTATS6: CWSched %lu, CWCalled: %lu, CWWaiter: %lu\n",
                     pThreadData->stats.CheckWaitersScheduled,
                     pThreadData->stats.CheckWaitersCalled,
                     pThreadData->stats.CheckWaitersGotWaiter);

    uint32_t numConsSlotsUsed = 0;
    int64_t totalTimeBetweenGets = 0;
    int64_t totalTimeBeforeStoreCommit = 0;
    int64_t totalTimeAfterStoreCommit = 0;
    uint64_t totalBatched = 0;
    int64_t totalStoreCommitTime = 0;
    int64_t totalConsumerLockedTime = 0;

    for (uint32_t slot = 0; slot < CONS_SLOTS; slot++)
    {
        if(pThreadData->stats.consTotalConsumerLockedTime[slot] > 0)
        {
            numConsSlotsUsed++;
            totalTimeBetweenGets += pThreadData->stats.consGapBetweenGets[slot];
            totalTimeBeforeStoreCommit += pThreadData->stats.consBeforeStoreCommitTime[slot];
            totalTimeAfterStoreCommit  += pThreadData->stats.consAfterStoreCommitTime[slot];
            totalBatched += pThreadData->stats.consBatchSize[slot];
            totalStoreCommitTime += pThreadData->stats.consStoreCommitTime[slot];
            totalConsumerLockedTime += pThreadData->stats.consTotalConsumerLockedTime[slot];
            
            //Don't keep using the same data every time....
            pThreadData->stats.consTotalConsumerLockedTime[slot] = 0;
        }
        else
        {
            break;
        }
    }

    if (numConsSlotsUsed > 0)
    {
        TRACE(4, "TEMPSTATS4: Avg %u gets: TimeBetweenGets: %g TimeBefore1stStorecommit: %g TimeAfter1stStorecommit: %g AvgBatch %2.2g Time1stStoreCommit: %g, TimeConsLocked: %g\n",
                numConsSlotsUsed,
                (((double)totalTimeBetweenGets) / numConsSlotsUsed),
                (((double)totalTimeBeforeStoreCommit) / numConsSlotsUsed),
                (((double)totalTimeAfterStoreCommit) / numConsSlotsUsed),
                (((double)totalBatched) / numConsSlotsUsed),
                (((double)totalStoreCommitTime) / numConsSlotsUsed),
                (((double)totalConsumerLockedTime) / numConsSlotsUsed));
    }
}

void traceHAInvestigationStats(void)
{
    TRACE(4, "BEGINTEMPSTATS\n");
    ieut_enumerateThreadData(tracePerThreadHAInvestigationStats, NULL);
    TRACE(4, "ENDTEMPSTATS\n");
}
#endif

//*******************************************************************************************
/// @brief Called periodically to check memory levels and take appropriate corrective action
///
/// If free memory (inc. buffers+cache) is less than IEMEM_FREEMEM_EMERGENCYTHRESHOLD percent of total memory
/// then most mallocs will be denied (some mallocs essentially to reducing memory usage may be allowed)
///
/// Else If free memory (inc. buffers+cache) is less than IEMEM_FREEMEM_DISABLEEARLYTHRESHOLD percent of total memory
/// some mallocs may be denied (that try to prevent new messages coming into the system
///
/// If free memory (inc. buffers+cache) is less than IEMEM_FREEMEM_REDUCETHRESHOLD percent of total memory
/// then any registered callbacks will be called to try and reduce the amount of memory currently in use.
//*******************************************************************************************
int iemem_checkMemoryLevels(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    iemem_systemMemInfo_t sysmeminfo;

    //Count of how many times this function has been called without
    //tracing current memory levels....
    static uint64_t checksSinceTrace = 0;
    static uint64_t checksSinceDetailedTrace = 0;
    static uint64_t checksSinceMallocTrim = 0;
    static size_t engineMemBytesAtTrim = 0;
    uint64_t memoryUsageRecordPeriod   = 0;
    uint64_t memoryDetailRecordPeriod  = 0;
    uint64_t mallocTrimPeriod          = 0;
    int32_t traceLevel;

    bool loopAgain;
    int32_t rc = OK;

    // Ensure that this invocation is noticed if we shut down
    __sync_add_and_fetch(&memCheckActiveTimerUseCount, 1);

    do
    {
        loopAgain = false;

        rc = iemem_querySystemMemory(&sysmeminfo);

        if (rc == OK)
        {
            iememMemoryLevel_t prevState = currState;

            //Work out how concerned we are by the amount of free memory...
            iememMemoryLevel_t newState = iemem_selectMemoryLevel( &sysmeminfo
                                                                 , &memoryUsageRecordPeriod
                                                                 , &memoryDetailRecordPeriod
                                                                 , &mallocTrimPeriod
                                                                 , &traceLevel);

            if (newState > currState)
            {
                //Things have just got worse...
                //Sound the alarms! (Don't Panic, Mr. Mainwaring)
                iemem_setMallocState(newState);
                currState = newState;

                // Force trace to be written this time round the loop
                checksSinceTrace = memoryUsageRecordPeriod;
                checksSinceDetailedTrace = memoryDetailRecordPeriod;

                //Force malloc trim to be called...
                checksSinceMallocTrim = mallocTrimPeriod;

                traceLevel = ENGINE_WORRYING_TRACE;

                //see if they immediately get better when we try and reduce things...
                loopAgain = true;

                //trace out the current memory usage in detail
            }
            else if (newState < currState)
            {
                //Things are getting better!
                iemem_setMallocState(newState);
                currState = newState;
            }

            checksSinceTrace++;
            checksSinceDetailedTrace++;
            checksSinceMallocTrim++;

            if (currState > iememPlentifulMemory)
            {
                //We could do with reclaiming memory...
                iemem_reduceMemoryUsage(prevState, &sysmeminfo);

                size_t engineMemBytes = iemem_queryTotalControlledMemory();
                size_t trimPadding = (size_t)((sysmeminfo.effectiveMemoryBytes * IEMEM_TRIM_PADDING_PERCENT) / 100);

                //Do a trim if we've reduce memory usage by more than a large amount since the last trim or periodically
                // (don't call it during shutdown...)
                //According to https://bugzilla.redhat.com/show_bug.cgi?id=921676
                //Calling malloc_trim will reduce the memory cached for use in fastbins...
                if (  (ismEngine_serverGlobal.runPhase < EnginePhaseEnding)
                    &&(     (engineMemBytes + trimPadding  < engineMemBytesAtTrim)
                       ||((mallocTrimPeriod) &&(checksSinceMallocTrim > mallocTrimPeriod ))))
                {
                    malloc_trim(trimPadding);
                    engineMemBytesAtTrim = engineMemBytes;
                    checksSinceMallocTrim = 0;
                }
            }
            else if (prevState > iememPlentifulMemory)
            {
                // We are back into plentiful memory - make a final call to any callbacks
                // to let them know.
                iemem_reduceMemoryUsage(prevState, &sysmeminfo);
            }

            if (checksSinceTrace > memoryUsageRecordPeriod)
            {
                if ((memoryDetailRecordPeriod) && (checksSinceDetailedTrace > memoryDetailRecordPeriod))
                {
                    ism_engine_traceMemoryStatistics(&sysmeminfo, true, traceLevel);
                    ism_common_traceMemoryStatistics(traceLevel);
                    ism_utils_traceBufferPoolsDiagnostics(traceLevel);
                    checksSinceDetailedTrace = 0;
                }
                else
                {
                    ism_engine_traceMemoryStatistics(&sysmeminfo, false, traceLevel);
                }
                checksSinceTrace = 0;
            }
        }
        else
        {
            TRACE(ENGINE_ERROR_TRACE,"Unable to query memory! rc=%d\n", rc);
        }
    }
    while ((rc == OK) && loopAgain);

#ifdef HAINVESTIGATIONSTATS
    static uint64_t passesSinceTraceStats = 0;

    passesSinceTraceStats++;

    if (passesSinceTraceStats > 10)
    {
        traceHAInvestigationStats();
        passesSinceTraceStats = 0;
    }
#endif

    uint32_t oldUseCount = __sync_sub_and_fetch(&memCheckActiveTimerUseCount, 1);
    int runagain = (oldUseCount == 0 ? 0 : 1);
    if (!runagain && __sync_bool_compare_and_swap(&memCheckTimerKey, key, NULL))
    {
        ism_common_cancelTimer(key);
    }

    return runagain;
}

void iemem_startMemDebugging(void)
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
        TRACE(ENGINE_INTERESTING_TRACE, "IEMEM_DEBUG: Causing memory to be initialised to non-empty on malloc and free\n");
        mallopt(M_PERTURB, 0xDEADBEEF); //initialise memory to non-zero when malloc'd/free'd

        if (getenv("MALLOC_CHECK_") == NULL)
        {
            TRACE(ENGINE_INTERESTING_TRACE, "IEMEM_DEBUG: WARNING MALLOC_CHECK_ unset. Setting this env var to 3 is handy for memory debugging\n");
        }
    }

    if (getenv("MALLOC_TRACE") != NULL)
    {
        TRACE(ENGINE_INTERESTING_TRACE, "IEMEM_DEBUG: Starting malloc trace...\n");
        mtrace();
    }
    else if (memdebugging)
    {
        TRACE(ENGINE_WORRYING_TRACE, "IEMEM_DEBUG: WARNING Not starting malloc trace (to use set MALLOC_TRACE env) to a filename\n");
    }
}

//***********************************************************************
/// @brief Initialise memory handler
///
/// @remark NOTE: To start calling iemem_ functions to allocate and free
/// memory the thread must be initialised via a call to ism_engine_threadInit.
//***********************************************************************
void iemem_initMemHandler(void)
{
    TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    //Do some debug related things based on compile flags/env vars
    iemem_startMemDebugging();

    TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT, "\n");
}

//***********************************************************************
/// @brief Kicks off periodic checks of memory usage in the system
//***********************************************************************
int32_t iemem_startMemoryMonitorTask(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pThreadData, ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    memCheckTimerKey = ism_common_setTimerRate(ISM_TIMER_HIGH,
                                               iemem_checkMemoryLevels,
                                               NULL,
                                               500,
                                               500,
                                               TS_MILLISECONDS);

    if (memCheckTimerKey == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memCheckActiveTimerUseCount = 1;

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//***********************************************************************
/// @brief Terminates memory monitoring (if it was active)
//***********************************************************************
void iemem_stopMemoryMonitorTask(ieutThreadData_t *pThreadData)
{
    int rc = 0;

    ieutTRACEL(pThreadData, memCheckTimerKey, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    ism_timer_t timerKey = __sync_lock_test_and_set(&memCheckTimerKey, NULL);
    if (timerKey != NULL)
    {
        rc = ism_common_cancelTimer(timerKey);

        if (rc != 0)
        {
            ieutTRACEL(pThreadData, rc, ENGINE_ERROR_TRACE, "Unable to stop memory monitor: rc = %d\n", rc);
        }
        else
        {
            int pauseMs = 20000;   // Initial pause is 0.02s
            uint32_t loop = 0;

            (void)__sync_fetch_and_sub(&memCheckActiveTimerUseCount, 1);

            // Wait for the timer to finish
            while ((volatile uint32_t)memCheckActiveTimerUseCount > 0)
            {
                if ((loop%100) == 0)
                {
                    ieutTRACEL(pThreadData, loop, ENGINE_NORMAL_TRACE, "%s: memCheckActiveTimerUseCount is %d\n",
                               __func__, memCheckActiveTimerUseCount);
                }

                // And pause for a short time to allow the timer to end
                ism_common_sleep(pauseMs);

                if (++loop > 290) // After 2 minutes
                {
                    pauseMs = 5000000; // Upgrade pause to 5 seconds
                }
                else if (loop > 50) // After 1 Seconds
                {
                    pauseMs = 500000; // Upgrade pause to .5 second
                }
            }
        }

        // Clear any memReduceCBs which we won't want to call any more
        for (int32_t i=0; i<iemem_numtypes; i++)
        {
            memReduceCBs[i] = NULL;
        }
    }

    ieutTRACEL(pThreadData, rc, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);
}

//***********************************************************************
/// @brief Clean up any resources left behind by memory monitoring
//***********************************************************************
void iemem_termMemHandler(ieutThreadData_t *pThreadData)
{
    // No resources allocated any more - but keep the function for possible future
    // requirements.
    ieutTRACEL(pThreadData, 0, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_IDENT "\n", __func__);
}
#else //else NO_MALLOC_WRAPPER
void iemem_initMemHandler(void)
{
}

int32_t iemem_startMemoryMonitorTask(ieutThreadData_t *pThreadData)
{
    ieutTRACEL(pThreadData, pThreadData, ENGINE_WORRYING_TRACE, "Memory monitor disabled\n");
    return OK;
}
void iemem_stopMemoryMonitorTask(ieutThreadData_t *pThreadData)
{
}

void iemem_termMemHandler(ieutThreadData_t *pThreadData)
{
}

#endif // end ifndef NO_MALLOC_WRAPPER


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
void iemem_touch(void *area, uint64_t areaBytes)
{
    char *areaptr = (char *)area;

    for (uint64_t i = 0; i < areaBytes; i += 4*IEMEM_KIBIBYTE)
    {
        areaptr[i] = 0;
    }
}

//****************************************************************************
/// @internal
///
/// @brief Get memory statistics
///
/// @param[out] pStatistics       Memory statistics
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getMemoryStatistics(ismEngine_MemoryStatistics_t *pStatistics)
{
    iemem_systemMemInfo_t system_meminfo;
    int32_t rc = OK;

    memset(pStatistics, 0, sizeof(*pStatistics));

    rc = iemem_querySystemMemory(&system_meminfo);
    if (rc == OK)
    {
        pStatistics->MemoryCGroupInUse = system_meminfo.effectiveFromCgroup;
        pStatistics->MemoryTotalBytes = system_meminfo.effectiveMemoryBytes;
        pStatistics->MemoryFreeBytes = system_meminfo.freeIncBuffersCacheBytes;
        pStatistics->MemoryFreePercent = 100.0 * system_meminfo.freeIncBuffersCacheBytes / system_meminfo.effectiveMemoryBytes;
        pStatistics->ServerVirtualMemoryBytes = system_meminfo.processVirtualMemorySize * 4096;
        pStatistics->ServerResidentSetBytes = system_meminfo.processResidentSetSize * 4096;
    }

#ifdef NO_MALLOC_WRAPPER
    TRACE(ENGINE_WORRYING_TRACE, "Memory monitor disabled\n");
#else
    size_t levels[iemem_numtypes];
    size_t groups[iemem_numgroups];

    iemem_queryControlledMemory(levels);
    iemem_queryGroupsMemory(levels, groups);

    pStatistics->GroupMessagePayloads    = groups[iememGroupMessages];
    pStatistics->GroupPublishSubscribe   = groups[iememGroupPublishSubscribe];
    pStatistics->GroupDestinations       = groups[iememGroupDestinations];
    pStatistics->GroupCurrentActivity    = groups[iememGroupCurrentActivity];
    pStatistics->GroupRecovery           = groups[iememGroupRecovery];
    pStatistics->GroupClientStates       = groups[iememGroupClientStates];

    pStatistics->MessagePayloads         = levels[iemem_messageBody];
    pStatistics->TopicAnalysis           = levels[iemem_topicAnalysis];
    pStatistics->SubscriberTree          = levels[iemem_subsTree];
    pStatistics->NamedSubscriptions      = levels[iemem_namedSubs];
    pStatistics->SubscriberCache         = levels[iemem_subscriberCache];
    pStatistics->SubscriberQuery         = levels[iemem_subsQuery];
    pStatistics->TopicTree               = levels[iemem_topicsTree];
    pStatistics->TopicQuery              = levels[iemem_topicsQuery];
    pStatistics->ClientState             = levels[iemem_clientState];
    pStatistics->CallbackContext         = levels[iemem_callbackContext];
    pStatistics->PolicyInfo              = levels[iemem_policyInfo];
    pStatistics->QueueNamespace          = levels[iemem_queueNamespace];
    pStatistics->SimpleQueueDefns        = levels[iemem_simpleQ];
    pStatistics->SimpleQueuePages        = levels[iemem_simpleQPage];
    pStatistics->IntermediateQueueDefns  = levels[iemem_intermediateQ];
    pStatistics->IntermediateQueuePages  = levels[iemem_intermediateQPage];
    pStatistics->MulticonsumerQueueDefns = levels[iemem_multiConsumerQ];
    pStatistics->MulticonsumerQueuePages = levels[iemem_multiConsumerQPage];
    pStatistics->LockManager             = levels[iemem_lockManager];
    pStatistics->MQConnectivityRecords   = levels[iemem_mqBridgeRecords];
    pStatistics->RecoveryTable           = levels[iemem_restoreTable];
    pStatistics->ExternalObjects         = levels[iemem_externalObjs];
    pStatistics->LocalTransactions       = levels[iemem_localTransactions];
    pStatistics->GlobalTransactions      = levels[iemem_globalTransactions];
    pStatistics->MonitoringData          = levels[iemem_monitoringData];
    pStatistics->NotificationData        = levels[iemem_notificationData];
    pStatistics->MessageExpiryData       = levels[iemem_messageExpiryData];
    pStatistics->RemoteServerData        = levels[iemem_remoteServers];
    pStatistics->CommitData              = levels[iemem_commitData];
    pStatistics->UnneededRetainedMsgs    = levels[iemem_unneededRetainedMsgs];
    pStatistics->ExpiringGetData         = levels[iemem_expiringGetData];
    pStatistics->ExportResources         = levels[iemem_exportResources];
    pStatistics->Diagnostics             = levels[iemem_diagnostics];
    pStatistics->UnneededBufferedMsgs    = levels[iemem_unneededBufferedMsgs];
    pStatistics->JobQueues               = levels[iemem_jobQueues];
    pStatistics->ResourceSetStats        = levels[iemem_resourceSetStats];
    pStatistics->DeferredFreeLists       = levels[iemem_deferredFreeLists];
#endif

    return rc;
}

//****************************************************************************
/// @brief Read (the first part) of a file into a buffer for parsing
///
/// @param[out]    buffer[IEMEM_MEMINFO_BUFFERSIZE]  buffer to store the contents in
/// @param[out]    bytesRead                         How many bytes were read from the file
///
/// @remark Reading files from /proc should occur in a single read (as they update between reads)
///
/// @remark Only the first IEMEM_MEMINFO_BUFFERSIZE (at most) of the file are read
//*******************************************************************************
static inline int32_t iemem_readMemInfoFile(char *filename,
                                            char buffer[IEMEM_MEMINFO_BUFFERSIZE],
                                            int *bytesRead)
{
    int f = open(filename, O_RDONLY);
    int32_t rc = OK;

    if (f < 0)
    {
        TRACE(ENGINE_ERROR_TRACE, "Couldn't open %s, errno: %d\n", filename, errno);
        rc = ISMRC_Error;
    }
    else
    {
        int bytes_read = read(f, buffer, IEMEM_MEMINFO_BUFFERSIZE - 1);

        if (bytes_read < 1)
        {
            TRACE(ENGINE_ERROR_TRACE, "Couldn't read from %s, errno: %d\n", filename, errno);
            rc = ISMRC_Error;
        }
        else
        {
            buffer[bytes_read] = 0;
            *bytesRead = bytes_read;
        }
        close(f);
    }
    return rc;
}

//****************************************************************************
/// @brief Read a simple (single value) from a file
///
/// @param[in]     filename            The file name to use
/// @param[out]    pValue              Pointer to receive a uint64_t value
///
/// @return OK or an ISMRC_ error
//*******************************************************************************
static inline int32_t iemem_readSimpleValueFromFile(char *filename, uint64_t *pValue)
{
    int32_t rc = OK;
    char buffer[50];

    int f = open(filename, O_RDONLY);
    if (f < 0)
    {
        TRACE(ENGINE_ERROR_TRACE, "Couldn't open %s, errno: %d\n", filename, errno);
        rc = ISMRC_NotFound;
    }
    else
    {
        size_t bytes_read = read(f, buffer, sizeof(buffer)-1);

        if (bytes_read < 1)
        {
            TRACE(ENGINE_ERROR_TRACE, "Couldn't read from %s, errno: %d\n", filename, errno);
            rc = ISMRC_NotFound;
        }
        else
        {
            *pValue = strtoul(buffer, NULL, 10);
        }

        close(f);
    }

    return rc;
}

//****************************************************************************
/// @brief Read the memory cgroup information for the running process
///
/// @param[out]    buffer[IEMEM_MEMINFO_BUFFERSIZE]  buffer to use when reading files
/// @param[out]    cgroupMemInfo                     structure to fill in
///
/// @return OK if the cgroup could be determined, and the files read, else ISMRC_NotFound
//*******************************************************************************
static inline int32_t iemem_readCgroupMemInfo(char buffer[IEMEM_MEMINFO_BUFFERSIZE],
                                              iemem_cgroupMemInfo_t *cgroupMemInfo)
{
    int bytes_read;
    int32_t rc = OK;

    static char *cgroupDirectory = NULL;
    static bool cgroupIsUnified = false; //In v1 of cgroups there were separate dirs for memory/cpu etc. In v2 they have been combined (and format changed)

    // Work out which cgroup we're a member of (if any)
    if (cgroupDirectory == NULL)
    {
        int isTopLevelGroup = 0;
        bool isUnified = false;
        char *pathptr = NULL;

        rc = ism_common_getCGroupPath(ISM_CGROUP_MEMORY,
                                      buffer,
                                      IEMEM_MEMINFO_BUFFERSIZE,
                                      &pathptr,
                                      &isTopLevelGroup);
        if (rc == ISMRC_OK)
        {
            isUnified = false; //found a memory specific cgroup
        }
        else
        {
            rc = ism_common_getCGroupPath(ISM_CGROUP_UNIFIED,
                                          buffer,
                                          IEMEM_MEMINFO_BUFFERSIZE,
                                          &pathptr,
                                          &isTopLevelGroup);
            if (rc == ISMRC_OK)
            {
                isUnified = true; //found a v2 unified cgroup
            }
        }

        if (rc == ISMRC_OK)
        {
            size_t pathLen = strlen(pathptr);
            char *myCgroupDirectory = malloc(pathLen+2);
            assert(myCgroupDirectory != NULL);

            // Copy the cgroup name into our buffer, and add a slash if required.
            memcpy(myCgroupDirectory, pathptr, pathLen);
            if (pathLen > 1) myCgroupDirectory[pathLen++] = '/';
            myCgroupDirectory[pathLen] = 0;

            // Update the name, if someone else beat us to it, check we agree and free our area
            if (__sync_bool_compare_and_swap(&cgroupDirectory,
                                             NULL,
                                             myCgroupDirectory) == true)
            {
                cgroupIsUnified = isUnified;
                TRACE(ENGINE_INTERESTING_TRACE, "%s CGroup directory string: '%s'\n",
                        (isUnified ? "Unified": "Memory"), myCgroupDirectory);

            }
            else
            {
                assert(strcmp(cgroupDirectory, myCgroupDirectory) == 0);
                free(myCgroupDirectory);
            }
        }
        else
        {
            TRACE(ENGINE_ERROR_TRACE, "Couldn't find cgroup %d\n",rc);
            rc = ISMRC_NotFound;
        }
    }

    // We know our cgroup directory and it is not blank, so now we can try to access
    // information from it.
    if (cgroupDirectory != NULL && cgroupDirectory[0] != 0)
    {
        assert(rc == OK);

        // Read limit
        char fname[100+strlen(cgroupDirectory)];
        if (cgroupIsUnified)
        {
            sprintf(fname, IEMEM_UNIFIED_CGROUP_LIMIT_IN_BYTES_FORMAT, cgroupDirectory);
        }
        else
        {
            sprintf(fname, IEMEM_MEMORY_CGROUP_LIMIT_IN_BYTES_FORMAT, cgroupDirectory);
        }
        rc = iemem_readSimpleValueFromFile(fname, &cgroupMemInfo->limitInBytes);

        // Read usage
        // From https://www.kernel.org/doc/Documentation/cgroups/memory.txt
        // 5.5 usage_in_bytes
        // For efficiency, as other kernel components, memory cgroup uses some optimization
        // to avoid unnecessary cacheline false sharing. usage_in_bytes is affected by the
        // method and doesn't show 'exact' value of memory (and swap) usage, it's a fuzz
        // value for efficient access. (Of course, when necessary, it's synchronized.)
        // If you want to know more exact memory usage, you should use RSS+CACHE(+SWAP)
        // value in memory.stat(see 5.2).
        //
        // In practice, the fuzz value seems to be accurate enough, and means we only
        // need to read the cache from memory.stat.
        if (rc == OK)
        {
            if (cgroupIsUnified)
            {
                sprintf(fname, IEMEM_UNIFIED_CGROUP_USAGE_IN_BYTES_FORMAT, cgroupDirectory);
            }
            else
            {
                sprintf(fname, IEMEM_MEMORY_CGROUP_USAGE_IN_BYTES_FORMAT, cgroupDirectory);
            }

            rc = iemem_readSimpleValueFromFile(fname, &cgroupMemInfo->usageInBytes);
        }

        // Read from stats
        if (rc == OK)
        {
            bytes_read = 0;

            sprintf(fname, IEMEM_CGROUP_MEMORY_STAT_FORMAT, cgroupDirectory);

            rc = iemem_readMemInfoFile(fname, buffer, &bytes_read);
        }

        // Pull values from the stat file
        if (rc == OK)
        {
            #define IEMEM_STAT_LINES 3
            char       *valueIdentifier[IEMEM_STAT_LINES] = {"cache ", "inactive_file ", "active_file "};
            size_t   valueIdentifierLen[IEMEM_STAT_LINES] = {6,        14,               12};
            bool   valueIdentifierFound[IEMEM_STAT_LINES] = {false,    false,            false};
            uint64_t              value[IEMEM_STAT_LINES] = {0,        0,                0};

            int32_t bufPos = 0;
            int32_t foundVals = 0;

            //While we haven't found all the values and we have more data left to parse...
            while ((foundVals < IEMEM_STAT_LINES) && (bufPos < bytes_read))
            {
                int32_t i;

                //for each string we're looking for...
                for (i=0; i < IEMEM_STAT_LINES; i++)
                {
                    if ( !(valueIdentifierFound[i]) )
                    {
                        if (strncmp(&buffer[bufPos], valueIdentifier[i], valueIdentifierLen[i]) == 0)
                        {
                            //We've matched one of the strings we're looking for
                            bufPos += valueIdentifierLen[i];

                            errno = 0;
                            value[i] = strtoul(&buffer[bufPos], NULL, 10);

                            if (errno == 0)
                            {
                                valueIdentifierFound[i] = true;
                                foundVals++;
                                break;
                            }
                            else
                            {
                                TRACE(ENGINE_ERROR_TRACE, "Failed to parse numeric value %d from memory.stat.", i);
                                rc = ISMRC_Error;
                                ism_common_setError(rc);
                            }
                        }
                    }
                }

                //Skip to the next line
                while((bufPos < bytes_read) && (buffer[bufPos] != '\n'))
                {
                    bufPos++;
                }

                while((bufPos < bytes_read) && isspace(buffer[bufPos]))
                {
                    bufPos++;
                }
            }

            assert(rc == OK);

            cgroupMemInfo->cacheBytes = value[0];
            cgroupMemInfo->activeFileBytes = value[1];
            cgroupMemInfo->inactiveFileBytes = value[2];

            uint64_t allFileBytes = cgroupMemInfo->activeFileBytes + cgroupMemInfo->inactiveFileBytes;

            // Memory in tempfs file systems is included in cache, we calculate it based on
            // the formula "tmpfs = cache - active_file - inactive_file" (from Redhat documentation
            // stating that "active_file + inactive_file = cache - sizeof tmpfs" and also the
            // following docker article on runmetrics).

            // We have observed cases where cache - active_file - inactive_file is negative, and
            // this throws the calculation of tmpfsBytes out - so we set it to 0 if that is the
            // case.
            if (cgroupMemInfo->cacheBytes > allFileBytes)
            {
                cgroupMemInfo->tmpfsBytes = cgroupMemInfo->cacheBytes - allFileBytes;
            }
            else
            {
                cgroupMemInfo->tmpfsBytes = 0;
            }
        }

        // From the usage, calculate free
        if (rc == OK)
        {
            cgroupMemInfo->freeBytes = cgroupMemInfo->limitInBytes - cgroupMemInfo->usageInBytes;
        }
    }
    else
    {
        rc = ISMRC_NotFound;
    }

    return rc;
}

//****************************************************************************
/// @brief Read /proc/<pid>/statm into a buffer for parsing
///
/// @param[out]    buffer[IEMEM_MEMINFO_BUFFERSIZE]  buffer to store the contents in
/// @param[out]    bytesRead                         How many bytes were read from the file
///
/// @remark Reading files from /proc should occur in a single read (as they update between reads)
///
/// @remark Only the first IEMEM_MEMINFO_BUFFERSIZE (at most) of the file are read
//*******************************************************************************
static inline int32_t iemem_readProcessMemInfo(char buffer[IEMEM_MEMINFO_BUFFERSIZE], int *bytesRead)
{
    char fname[64];
    sprintf(fname, "/proc/%d/statm", getpid());

    int f = open(fname, O_RDONLY);
    int32_t rc = OK;

    if (f < 0)
    {
        TRACE(ENGINE_ERROR_TRACE, "Couldn't open %s, errno: %d\n", fname, errno);
        rc = ISMRC_Error;
        ism_common_setError(rc);
    }
    else
    {
        int bytes_read = read(f, buffer, IEMEM_MEMINFO_BUFFERSIZE - 1);

        if (bytes_read < 1)
        {
            TRACE(ENGINE_ERROR_TRACE, "Couldn't read from %s, errno: %d\n", fname, errno);
            rc = ISMRC_Error;
            ism_common_setError(rc);
        }
        else
        {
            buffer[bytes_read] = 0;
            *bytesRead = bytes_read;
        }
        close(f);
    }
    return rc;
}

//****************************************************************************
/// @brief Parses /proc/meminfo to determine how much free memory there is in the system
///
/// @param[out] sysmeminfo Current Memory Levels
/// @see  iemem_systemMemInfo_t
//****************************************************************************
int32_t iemem_querySystemMemory(iemem_systemMemInfo_t *sysmeminfo)
{
    int32_t rc = OK;
    char memInfoBuffer[IEMEM_MEMINFO_BUFFERSIZE];
    char processInfoBuffer[IEMEM_MEMINFO_BUFFERSIZE];
    uint64_t virtualMemorySize;
    uint64_t residentSetSize;
    int found;
    int bytesRead;
    int dummy;
    int bufPos=0;

    //Read enough of /proc/meminfo to determine how much much free memory there is
    rc = iemem_readMemInfoFile("/proc/meminfo", memInfoBuffer, &bytesRead);
    if (rc != OK)
    {
        ism_common_setError(rc);
    }
    else
    {
        rc = iemem_readProcessMemInfo(processInfoBuffer, &dummy);
        if (rc == OK)
        {
            //The process memory info is very simple - 7 numbers separated by spaces - we want the first and second
            found = sscanf(processInfoBuffer, "%lu %lu", &virtualMemorySize, &residentSetSize);
            if (found != 2)
            {
                TRACE(ENGINE_ERROR_TRACE,"Process memory not parsed\n");
                rc = ISMRC_Error;
                ism_common_setError(rc);
            }
        }
    }

    if (rc == OK)
    {
        //Create some data structures that represent the strings we're looking for in the file
        // Change this to 6 per defect 180968; adding reclaimable slab memory
#define IEMEM_MEMINFO_LINES 6
        char *memStrings[IEMEM_MEMINFO_LINES] = {"MemTotal:",  "MemFree:",   "Buffers:",   "Cached:",  "Shmem:",   "SReclaimable:"};
        int   memStrLens[IEMEM_MEMINFO_LINES] = {9,            8,            8,            7,          6,          13 };
        bool memStrFound[IEMEM_MEMINFO_LINES] = {false,        false,        false,        false,      false,      false   };
        uint64_t memVals[IEMEM_MEMINFO_LINES] = {0};

        int foundVals = 0;

        //While we haven't found all the values and we have more data left to parse...
        while ((foundVals < IEMEM_MEMINFO_LINES) && (bufPos < bytesRead))
        {
            int i;

            //for each string we're looking for...
            for (i=0; i < IEMEM_MEMINFO_LINES; i++)
            {
                if ( !(memStrFound[i]) )
                {
                    if (strncmp(&memInfoBuffer[bufPos], memStrings[i], memStrLens[i]) == 0)
                    {
                        //We've matched one of the strings we're looking for
                        bufPos +=memStrLens[i];

                        errno = 0;
                        memVals[i] = strtoul(&memInfoBuffer[bufPos], NULL, 10);

                        if (errno == 0)
                        {
                            memStrFound[i] = true;
                            foundVals++;
#ifndef NDEBUG
                            //Check that the value is in KibiBytes by finding 'kB'
                            bool foundkB = false;

                            while(   (bufPos < bytesRead)
                                  && (!foundkB)
                                  && (memInfoBuffer[bufPos] != '\n')
                                  && (memInfoBuffer[bufPos] != '\0'))
                            {
                                if (memInfoBuffer[bufPos] == 'B')
                                {
                                    assert(memInfoBuffer[bufPos-1] == 'k');
                                }
                                foundkB = true;
                            }
                            assert(foundkB);
#endif
                            break;
                        }
                        else
                        {
                            TRACE(ENGINE_ERROR_TRACE, "Failed to parse numeric value %d from /proc/meminfo.\n", i);
                            rc = ISMRC_Error;
                            ism_common_setError(rc);
                        }
                    }
                }
            }
            //Skip to the next line
            while((bufPos < bytesRead) && (memInfoBuffer[bufPos] != '\n'))
            {
                bufPos++;
            }
            while((bufPos < bytesRead) && isspace(memInfoBuffer[bufPos]))
            {
                bufPos++;
            }
        }

        //If we didn't find all the values we were looking for (things have gone BADLY wrong!)
        if(foundVals < IEMEM_MEMINFO_LINES)
        {
            int  i;

            //dump out what we did manage to parse...
            for (i=0; i < IEMEM_MEMINFO_LINES; i++)
            {
                if (memStrFound[i])
                {
                    TRACE(ENGINE_ERROR_TRACE,"memVals %d (%s) is %lu\n",
                          i, memStrings[i], memVals[i]);
                }
                else
                {
                    TRACE(ENGINE_ERROR_TRACE,"memVals %d (%s) was NOT FOUND!\n",
                          i, memStrings[i]);
                }
            }
            rc = ISMRC_Error;
            ism_common_setError(rc);
        }
        else
        {
            static bool readCgroupInfo = true;

            memset(&sysmeminfo->cgroupMemInfo, 0, sizeof(sysmeminfo->cgroupMemInfo));

            //Record for our caller what the memory level we found were....
            sysmeminfo->totalMemoryBytes = memVals[0] * IEMEM_KIBIBYTE;
            sysmeminfo->currentFreeBytes = memVals[1] * IEMEM_KIBIBYTE;

            // See if we have any cgroup memory information available
            rc = readCgroupInfo ? iemem_readCgroupMemInfo(memInfoBuffer, &sysmeminfo->cgroupMemInfo) : ISMRC_NotFound;

            // Use cgroup values if they are available and limit us to less than total memory

            // Note: If we're inside a docker container, the value will be 0x7FFFFFFFFFFFFFFF
            //       but outside of docker, the default cgroup has a very large number, but
            // which is smaller than this (I've seen 0x7FFFFFFFFFFFFF00 for instance on a
            // fedora machine).
            //
            // Because this value is considered a very large number when seen as a uint64_t,
            // we can simply test it against the available total system memory and, if it is
            // lower, treat it as our view of 'total memory' - it is, in effect, the total
            // memory available to this cgroup - this way all of the subsequent reporting
            // and limiting will be based on the cgroup.
            if (   rc == OK
                && sysmeminfo->cgroupMemInfo.limitInBytes < sysmeminfo->totalMemoryBytes
                && sysmeminfo->cgroupMemInfo.limitInBytes > 0)

            {
                // We will use the limit we've been set as our effective memory
                sysmeminfo->effectiveMemoryBytes = sysmeminfo->cgroupMemInfo.limitInBytes;
                sysmeminfo->effectiveFromCgroup = true;

                // The value for tmpfsBytes includes both files in tmpfs filesystems, and
                // shared memory - neither of these will shrink under memory pressure, but
                // both appear under the cacheBytes figure.
                //
                // We need to discount tmpfsBytes from our 'free including buffers and cache'
                // value.
                //
                // MessageSight won't be using shared memory, but other things running in
                // this cgroup/docker container might.
                assert(sysmeminfo->cgroupMemInfo.cacheBytes >= sysmeminfo->cgroupMemInfo.tmpfsBytes);
                
                sysmeminfo->freeIncBuffersCacheBytes = sysmeminfo->cgroupMemInfo.freeBytes +
                                                       sysmeminfo->cgroupMemInfo.cacheBytes -
                                                       sysmeminfo->cgroupMemInfo.tmpfsBytes;
            }
            else
            {
                if (rc == OK)
                {
                    // TODO: Could avoid cgroup lookup next time by setting readCGroupInfo to false
                }
                else
                {
                    // Caller doesn't need to know the rc from iemem_readCgroupMemInfo...
                    rc = OK;
                }

                uint64_t buffers = memVals[2];
                uint64_t cached  = memVals[3];
                uint64_t shm     = memVals[4];
                uint64_t slabr   = memVals[5];
                uint64_t freeInc = 0;

                // We will use the total system memory bytes reported as our effectiveMemory
                sysmeminfo->effectiveMemoryBytes = sysmeminfo->totalMemoryBytes;
                sysmeminfo->effectiveFromCgroup = false;

                //Shm is included in cached figure but won't shrink under memory pressure...discount that
                // Add reclaimable slab memory into total value in both cases
                // If we do have a value for MemAvailable, use that instead
                if (cached > shm)
                {
                    freeInc = memVals[1] + buffers + cached + slabr - shm;
                }
                else
                {
                    freeInc = memVals[1] + buffers + slabr;
                }

                //printf("free (%lu), buffers (%lu), cached (%lu), shm (%lu), freeInc(%lu)\n", memVals[1] , buffers, cached, shm, freeInc);
                sysmeminfo->freeIncBuffersCacheBytes = freeInc * IEMEM_KIBIBYTE;

                //Buffers & cache shrink under memory pressure but they don't shrink to zero...
                //...if the memory is less than say 100GiB, then their minimum size is important...we'll allow them
                //say 300MiB (exact values set by cfg params)...
                if (sysmeminfo->effectiveMemoryBytes < ismEngine_serverGlobal.freeMemRsvdThresholdMB * IEMEM_MEBIBYTE)
                {
                    if(   sysmeminfo->freeIncBuffersCacheBytes
                        > sysmeminfo->currentFreeBytes + (ismEngine_serverGlobal.freeMemReservedMB * IEMEM_MEBIBYTE))
                    {
                        sysmeminfo->freeIncBuffersCacheBytes -= (ismEngine_serverGlobal.freeMemReservedMB * IEMEM_MEBIBYTE);
                    }
                    else
                    {
                        sysmeminfo->freeIncBuffersCacheBytes = sysmeminfo->currentFreeBytes;
                    }
                }
            }

            sysmeminfo->freeIncPercentage = (100L * sysmeminfo->freeIncBuffersCacheBytes)/(sysmeminfo->effectiveMemoryBytes);
            sysmeminfo->processVirtualMemorySize = virtualMemorySize;
            sysmeminfo->processResidentSetSize = residentSetSize;
            //TRACE(ENGINE_NORMAL_TRACE, "Mem values, Total %lu, CurrFree: %lu, FreeInclusive: %lu\n",
            //        sysmeminfo->totalMemoryBytes, sysmeminfo->currentFreeBytes, sysmeminfo->freeIncBuffersCacheBytes);
        }
    }

    return rc;
}

//****************************************************************************
/// @brief Return the name of a specified memoryType
///
/// @param[in] type   Memory type to get
///
/// @return Name of the specified iemem_ memory type.
//****************************************************************************
const char *iemem_getTypeName(iemem_memoryType type)
{
    assert(type < iemem_numtypes);
    return iememMemTypeInfo[type].typeName;
}
