/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

//*********************************************************************
/// @file  mempool.h
/// @brief details that only the mempool implementation (and its tests) need
//*********************************************************************
#ifndef __ISM_MEMPOOL_INTERNAL_DEFINED
#define __ISM_MEMPOOL_INTERNAL_DEFINED

#include <stdint.h>

#include "engineCommon.h"     /* Engine common internal header file  */
#include "engineInternal.h"   /* Engine internal header file         */
#include "mempool.h"

//first page has pageheader followed by overall header followed by reserved mem, followed by normal, unreserved mem
//subsequent pages have page memory followed by normal unreserved memory
typedef struct tag_iempMemPoolPageHeader_t {
    char StrucId[4];                ///< Eyecatcher "IEMP"
    struct iempMemPoolPageHeader_t * volatile nextPage;
    size_t pageSize;
    volatile size_t nextMemOffset;
} iempMemPoolPageHeader_t;

#define iempSTRUCID "IEMP"

typedef struct tag_iempMemPoolOverallHeader_t {
    struct iempMemPoolPageHeader_t * volatile lastPage;
    volatile size_t reservedMemRemaining;
    size_t reservedMemInitial;
    size_t subsequentPageSize; //We know the initial page size from the page header on first page
    iemem_memoryType memType;
    pthread_spinlock_t listlock;
} iempMemPoolOverallHeader_t;


#endif
