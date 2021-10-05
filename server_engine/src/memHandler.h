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
/// @file  memHandler.h
/// @brief Defines functions/constants for allocating/freeing/monitoring memory
/// use inside the engine
//****************************************************************************

#ifndef __ISM_MEMHANDLER_DEFINED
#define __ISM_MEMHANDLER_DEFINED

#include <memHandlerTypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "engineCommon.h"

#ifdef NO_MALLOC_WRAPPER
//If NO_MALLOC_WRAPPER is defined the iemem_* wrapper become their stdlib equivalents
#define iemem_malloc(threaddata, probe, size) malloc(size)
#define iemem_calloc(threaddata, probe, nmemb, size) calloc(nmemb, size)
#define iemem_realloc(threaddata, probe, ptr, size) realloc(ptr, size)
#define iemem_free(threaddata, type, mem) free(mem)
#define iemem_usable_size(type, mem) malloc_usable_size(mem)
#else

#define IEMEM_GET_MEMORY_TYPE(probe) ((probe) & 0x0000FFFF)
#define IEMEM_GET_MEMORY_PROBEID(probe)  (((probe) & 0xFFFF0000)>>16)

// Create a memoryType specific probe. The probeid can be 1-59999 and should be unique
// within the particular memory type. The range 60000-65535 is reserved for use by generic
// routines that allocate storage on behalf of another memory type, add to the following
// list when a subset is taken from this range.
//
// 60000-60099 = topicTreeHashTable.c
#define IEMEM_PROBE(type, probeid) (((uint32_t)(probeid)<<16)|type)

//defined in memHandler.c... update that file with a new name
//whenever a name is added above...
extern char *iemem_memoryTypeNames[iemem_numtypes];


//Structure used for per-thread memory accounting. These are bytes reserved from
//process-wide totals, but not currently allocated
typedef struct tag_iememThreadMemUsage_t
{
    size_t threadReservation[iemem_numtypes];
} iememThreadMemUsage_t;

//If free memory goes below this percentage of total mem, we fail new mallocs
//for most memory types until memory has been freed
#define IEMEM_FREEMEM_EMERGENCYTHRESHOLD 10

//If free memory goes below this percentage of total mem, we fail new mallocs for
//a small number memory types designed to cause puts of new messages to fail
//(but getting of messages should be allowed to continue
#define IEMEM_FREEMEM_DISABLEEARLYTHRESHOLD 15

//If free memory goes below this percentage of total memory,
//we periodically call the callbacks to try and reduce it
#define IEMEM_FREEMEM_REDUCETHRESHOLD 50

typedef enum tag_iememMemoryLevel_t {
    iememPlentifulMemory,      ///< memory free is more than IEMEM_FREEFRACTION_REDUCETHRESHOLD - we have lots of memory
    iememReduceMemory,         ///< memory free is below IEMEM_FREEFRACTION_REDUCETHRESHOLD, memory is tight and we should start
                               ///   to reduce it...  (one day by offloading queued messages to disk?)
    iememDisableEarly,         ///< memory free is below IEMEM_FREEFRACTION_DISABLEEARLYTHRESHOLD... disable puts of new messages
    iememEmergencyMemory,      ///< memory free is below IEMEM_FREEFRACTION_EMERGENCYTHRESHOLD, disable most mallocs
    iememDisableAll            ///< Only used in testing... ALL mallocs are refused
} iememMemoryLevel_t;


// The value 'probe' can either be just an iemem_memoryType value or can be a specific
// memory probe id which combines an iemem_memoryType with a unique identifier.
// The IEMEM_PROBE macro can be used to build the probe, make sure to check that the
// probeId you use has not already been used for that type for example:
//
//  grep "IEMEM_PROBE(<type>" *.c
int32_t iemem_initializeThreadMemUsage(ieutThreadData_t *pThreadData);
void iemem_destroyThreadMemUsage(ieutThreadData_t *pThreadData);
void *iemem_malloc(ieutThreadData_t *pThreadData, uint32_t probe, size_t size);
void *iemem_calloc(ieutThreadData_t *pThreadData, uint32_t probe, size_t nmemb, size_t size);
void *iemem_realloc(ieutThreadData_t *pThreadData, uint32_t probe, void *ptr, size_t size);
void iemem_free(ieutThreadData_t *pThreadData, iemem_memoryType type, void *mem);
void iemem_freeStruct(ieutThreadData_t *pThreadData, iemem_memoryType type, void *pStruct, char *pStructId);
size_t iemem_usable_size(iemem_memoryType type, void *mem);
size_t iemem_full_size(iemem_memoryType type, void *mem);

void iemem_queryControlledMemory( size_t levels[iemem_numtypes]);
iememMemoryLevel_t iemem_queryCurrentMallocState(void);

void iemem_setMallocStateForType(iemem_memoryType type, bool allowed);
void iemem_setMallocState(iememMemoryLevel_t memoryLevel);
void iemem_setMemChunkSize(uint32_t newChunkSizeBytes);
uint32_t iemem_getMemChunkSize(void);

const char *iemem_getTypeName(iemem_memoryType type);

#ifdef MEMDEBUG
    typedef struct tag_iememEyecatcher_t {
        char StructId[4];
        iemem_memoryType memType;
    } __attribute__ (( aligned(16))) iememEyecatcher_t;

#define IEMEM_STRUCTID "IEMM"
#endif

///Called when memory usage needs to by reduced if possible
struct tag_iemem_systemMemInfo_t;

typedef void (*iemem_reduceCallback_t)(
                 iememMemoryLevel_t currState,
                 iememMemoryLevel_t prevState,
                 iemem_memoryType type,
                 uint64_t currentLevel,
                 struct tag_iemem_systemMemInfo_t *memInfo);

void iemem_setMemoryReduceCallback(iemem_memoryType type,
                                   iemem_reduceCallback_t callback);

#endif //end else ifdef NO_MALLOC_WRAPPER

///Stores data about memory cgroup statistics
typedef struct tag_iemem_cgroupMemInfo_t {
    uint64_t limitInBytes;       ///< Limit set for the cgroup
    uint64_t usageInBytes;       ///< Usage reported for the cgroup
    uint64_t freeBytes;          ///< Free bytes reported for the cgroup
    uint64_t cacheBytes;
    uint64_t activeFileBytes;
    uint64_t inactiveFileBytes;
    uint64_t tmpfsBytes;
} iemem_cgroupMemInfo_t;

///Stores data about memory usage in the system, some of which may be limited by
///membership of a memory cgroup
typedef struct tag_iemem_systemMemInfo_t {
    uint64_t effectiveMemoryBytes;     ///< Effective total memory (from system or cgroup)
    uint64_t freeIncBuffersCacheBytes; ///< Include Caches+Buffers as these are available for mallocs if needed (but not shm)
    uint8_t  freeIncPercentage;        ///< rounded down: (freeIncBuffersCacheBytes / total)*100
    bool     effectiveFromCgroup;      ///< Effective total comes from the cgroup
    uint64_t totalMemoryBytes;
    uint64_t currentFreeBytes;
    uint64_t processVirtualMemorySize;
    uint64_t processResidentSetSize;
    iemem_cgroupMemInfo_t cgroupMemInfo;
} iemem_systemMemInfo_t;

#define IEMEM_KIBIBYTE (1024L)
#define IEMEM_MEBIBYTE (1024L * IEMEM_KIBIBYTE)
#define IEMEM_GIBIBYTE (1024L * IEMEM_MEBIBYTE)

// Format strings for cgroup files containing useful information (the %s is replaced
// with a cgroup directory path)
#define IEMEM_MEMORY_CGROUP_LIMIT_IN_BYTES_FORMAT "%smemory.limit_in_bytes"
#define IEMEM_MEMORY_CGROUP_USAGE_IN_BYTES_FORMAT "%smemory.usage_in_bytes"
#define IEMEM_UNIFIED_CGROUP_LIMIT_IN_BYTES_FORMAT "%smemory.max"
#define IEMEM_UNIFIED_CGROUP_USAGE_IN_BYTES_FORMAT "%smemory.current"
#define IEMEM_CGROUP_MEMORY_STAT_FORMAT "%smemory.stat" //seems to be the same in memory and unified cgroup
#define IEMEM_CGROUP_STAT_CACHE_IDENTIFIER  "cache "

//When we read /proc/meminfo we keep the buffer in the
//stack... don't need it very large, most of the numbers we need
//are early on, only later one is shared mem which isn't very far on
#define IEMEM_MEMINFO_BUFFERSIZE 2048

int32_t iemem_querySystemMemory(iemem_systemMemInfo_t *sysmeminfo);

void iemem_initMemHandler(void);
void iemem_termMemHandler(ieutThreadData_t *pThreadData);

//Whether or not we have a malloc wrapper we have a start function.
//If the wrapper is disabled it just traces a warning...
int32_t iemem_startMemoryMonitorTask(ieutThreadData_t *pThreadData);
void iemem_stopMemoryMonitorTask(ieutThreadData_t *pThreadData);

void ism_engine_showEngineMemory(void);

void iemem_touch(void *area, uint64_t areaBytes);

#endif /* __ISM_MEMHANDLER_DEFINED */
