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
/// @file  commonMemHandler.h
/// @brief Defines functions/constants for allocating/freeing/monitoring memory
/// use inside the product
//****************************************************************************

#ifndef __ISM_COMMON_MEMHANDLER_DEFINED
#define __ISM_COMMON_MEMHANDLER_DEFINED

#include <commonMemHandlerTypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct
{
    uint64_t groups[ism_common_mem_numgroups];
    uint64_t types[ism_common_mem_numtypes];

} ism_MemoryStatistics_t;


#ifndef COMMON_MALLOC_WRAPPER
//If COMMON_MALLOC_WRAPPER is not defined the ism_common_* wrapper become their stdlib equivalents
#define ism_common_malloc(probe, size) malloc(size)
#define ism_common_malloc_noeyecatcher(size) malloc(size)
#define ism_common_calloc(probe, nmemb, size) calloc(nmemb, size)
#define ism_common_realloc(probe, ptr, size) realloc(ptr, size)
#define ism_common_free(type, mem) free(mem)
#define ism_common_usable_size(type, mem) malloc_usable_size(mem)
#define ism_common_check(type,mem)
#define ism_common_strdup(probe,str) ((str != NULL) ? strdup(str) : str)
#define ism_confirm_eyecatcher(mem) false
#define ism_confirm_memType(type,mem) false
#define ism_common_queryMemReservation(type) 0
#define ism_common_free_memaligned(type,mem) free(mem)
#define ism_common_realloc_memaligned(probe,ptr,size) realloc(ptr,size)
#define ism_common_free_raw(type,mem) free(mem)
#define ism_common_free_anyType(mem) free(mem)
#define ism_common_initializeThreadMemUsage() ;
#define ism_common_destroyThreadMemUsage() ;
#define ism_common_transfer_memory(srcType, destType, mem) ;
#else

#define ISM_GET_MEMORY_TYPE(probe) ((probe) & 0x0000FFFF)
#define ISM_GET_MEMORY_PROBEID(probe)  (((probe) & 0xFFFF0000)>>16)
#define ism_common_free(type,mem) ism_common_free_location(type,mem,__FILE__,__LINE__)
// Create a memoryType specific probe. The probeid can be 1-59999 and should be unique
// within the particular memory type. The range 60000-65535 is reserved for use by generic
// routines that allocate storage on behalf of another memory type, add to the following
// list when a subset is taken from this range.
//
#define ISM_MEM_PROBE(type, probeid) (((uint32_t)(probeid)<<16)|type)

// C++ doesn't allow implicit casting from an enum (ie the type) to an int so use
// a different function to skip the type, commonMemHandlerTypes.tag_memoryType has a dummy
// value inserted at the top of the enum to accomodate this
#define ISM_MEM_PROBE_CPP(type, probeid) (ism_memory_cpp)

//defined in memHandler.c... update that file with a new name
//whenever a name is added above...
extern char *ism_common_memoryTypeNames[ism_common_mem_numtypes];

//Structure used for per-thread memory accounting. These are bytes reserved from
//process-wide totals, but not currently allocated
typedef struct tag_ismThreadMemUsage_t
{
    size_t threadReservation[ism_common_mem_numtypes];
} ism_threadmemusage_t;

//****************************************************************************
/** @brief Memory Statistics
 * Statistics about memory
 */
//****************************************************************************


// The value 'probe' can either be just an ism_common_memoryType value or can be a specific
// memory probe id which combines an ism_common_memoryType with a unique identifier.
// The ISM_MEM_PROBE macro can be used to build the probe, make sure to check that the
// probeId you use has not already been used for that type for example:
//
//  grep "ISM_MEM_PROBE(<type>" *.c
int32_t ism_common_initializeThreadMemUsage(void);
void ism_common_destroyThreadMemUsage(void);
void *ism_common_malloc(uint32_t probe, size_t size);
void *ism_common_malloc_noeyecatcher(size_t size);
void *ism_common_calloc(uint32_t probe, size_t nmemb, size_t size);
void *ism_common_realloc(uint32_t probe, void *ptr, size_t size);
void ism_common_free_location(ism_common_memoryType type, void *mem,  const char * file, int lineno);
void ism_common_freeStruct(ism_common_memoryType type, void *pStruct, char *pStructId);
void ism_check(ism_common_memoryType type, void * mem);
bool ism_confirm_eyecatcher(void * mem);
bool ism_confirm_memType(ism_common_memoryType type, void * mem);
size_t ism_common_queryMemReservation(ism_common_memoryType type);

void ism_common_free_memaligned(ism_common_memoryType type, void *mem);
void *ism_common_realloc_memaligned(uint32_t probe, void *ptr, size_t size);
void ism_common_free_raw(ism_common_memoryType type, void *mem);
char *ism_common_strdup(uint32_t probe, const char *str);
void ism_common_transfer_memory(ism_common_memoryType srcType, ism_common_memoryType destType, void * mem);

size_t ism_common_usable_size(void *mem);
size_t ism_common_full_size(void *mem);

//If the type is not known at the time of freeing then:
void ism_common_free_anyType(void *mem);
void ism_common_freeStruct_anyType(void *pStruct, char *pStructId);
size_t ism_common_usable_size_anyType(void *mem);
size_t ism_common_full_size_anyType(void *mem);

void ism_common_queryControlledMemory( size_t levels[ism_common_mem_numtypes]);

void ism_common_setMallocStateForType(ism_common_memoryType type, bool allowed);
void ism_common_setMemChunkSize(uint32_t newChunkSizeBytes);
uint32_t ism_common_getMemChunkSize(void);

int32_t ism_common_getMemoryStatistics(ism_MemoryStatistics_t *pStatistics);

typedef struct tag_ism_common_Eyecatcher_t {
    char StructId[4];
    uint16_t point;
    ism_common_memoryType memType;
} __attribute__ (( aligned(16))) ism_common_Eyecatcher_t;

#define ISM_MEM_STRUCTID "ISMM"
#define NOT_ISM_MEM_STRUCTID "OLDI"

#endif //end else ifndef COMMON_MALLOC_WRAPPER

void ism_common_initMemHandler(void);
void ism_common_termMemHandler(void);

void ism_common_traceMemoryStatistics( int32_t TraceLevel );

void ism_engine_showEngineMemory(void);

void ism_common_mem_touch(void *area, uint64_t areaBytes);

const char * ism_common_getMemoryGroupName(uint32_t id);

const char * ism_common_getMemoryTypeName(uint32_t id);

ism_common_memoryGroup ism_common_getMemoryGroupFromType(uint32_t id);

bool ism_common_getPrintableFromGroup(uint32_t id);

#endif /* __ISM_COMMON_MEMHANDLER_DEFINED */
