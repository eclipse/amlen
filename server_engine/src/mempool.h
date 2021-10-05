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
/// @brief Engine component lock manager header file
//*********************************************************************
#ifndef __ISM_MEMPOOL_DEFINED
#define __ISM_MEMPOOL_DEFINED

#include <stdint.h>

#include "engineCommon.h"     /* Engine common internal header file  */
#include "engineInternal.h"   /* Engine internal header file         */
#include "memHandler.h"

//Defined in engineInternal.h:
//typedef struct tag_iempMemPoolPageHeader_t *iempMemPoolHandle_t;

int32_t iemp_createMemPool( ieutThreadData_t *pThreadData
                          , iemem_memoryType memTypeAndProbe
                          , size_t reservedMem
                          , size_t initialPageSize
                          , size_t subsequentPageSize
                          , iempMemPoolHandle_t *pMemPoolHdl);

void iemp_destroyMemPool( ieutThreadData_t *pThreadData
                        , iempMemPoolHandle_t *pMemPoolHdl);

// On entry memAmount is the amount requested, on exit it is the amount allocated
// (which will be at least the amount requested)
int32_t iemp_useReservedMem( ieutThreadData_t *pThreadData
                           , iempMemPoolHandle_t memPoolHdl
                           , size_t *memAmount
                           , void **mem);

void iemp_tryReleaseReservedMem( ieutThreadData_t *pThreadData
                               , iempMemPoolHandle_t memPoolHdl
                               , void *mem
                               , size_t memAmount);

int32_t iemp_allocate( ieutThreadData_t *pThreadData
                     , iempMemPoolHandle_t memPoolHdl
                     , size_t memAmount
                     , void **mem);

void iemp_clearMemPool( ieutThreadData_t *pThreadData
                      , iempMemPoolHandle_t memPoolHdl
                      , bool keepReservedMem);

#endif
