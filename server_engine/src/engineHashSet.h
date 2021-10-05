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
/// @file  engineHashSet.h
/// @brief Defines structures and functions used internally engine hashsets
//****************************************************************************
#ifndef __ISM_ENGINE_ENGINE_HASHSET_DEFINED
#define __ISM_ENGINE_ENGINE_HASHSET_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "memHandler.h"

/*********************************************************************/
/* Structures and constants from engineHashSet.c                     */
/*********************************************************************/

//****************************************************************************
/// @brief An entry in a hash set chain
/// @see ieutHashSetChain_t
//****************************************************************************
typedef struct tag_ieutHashSetEntry_t
{
    uint64_t            value;      ///< Value for this entry
} ieutHashSetEntry_t;

//****************************************************************************
/// @brief A single chain in a hash set
/// @see ieutHashSet_t
/// @see ieutHashSetEntry_t
//****************************************************************************
typedef struct tag_ieutHashSetChain_t
{
    uint32_t             count;    ///< How many entries are in this chain
    uint32_t             size;     ///< The size of the entries array allocated for this chain
    ieutHashSetEntry_t * entries;  ///< Pointer to the entries for this chain
} ieutHashSetChain_t;

//****************************************************************************
/// @brief A hash set
/// @see ieutHashSetChain_t
//****************************************************************************
typedef struct tag_ieutHashSet_t
{
    uint64_t               totalCount;    ///< Total entries in the entire hash set
    ieutHashSetChain_t *   chains;        ///< Array of chains
    uint64_t               capacity;      ///< Size of the chains array
    uint64_t               resizeAtCount; ///< Count at which to resize the hash set
    iemem_memoryType       memType;       ///< Type of memory to allocate for this set
} ieutHashSet_t;

//****************************************************************************
/// @brief Callback function used when traversing a hash set
//****************************************************************************
typedef void (*ieutHashSet_TraverseCallback_t)(ieutThreadData_t *pThreadData,
                                               uint64_t value,
                                               void *context);

/*********************************************************************/
/* FUNCTION PROTOTYPES                                               */
/*********************************************************************/
int32_t  ieut_createHashSet(ieutThreadData_t *pThreadData,
                            iemem_memoryType memType,
                            ieutHashSet_t **set);
void     ieut_traverseHashSet(ieutThreadData_t *pThreadData,
                              ieutHashSet_t *set,
                              ieutHashSet_TraverseCallback_t  callback,
                              void *context);
void     ieut_clearHashSet(ieutThreadData_t *pThreadData, ieutHashSet_t *set);
void     ieut_destroyHashSet(ieutThreadData_t *pThreadData, ieutHashSet_t *set);
int32_t  ieut_addValueToHashSet(ieutThreadData_t *pThreadData, ieutHashSet_t *set, uint64_t value);
int32_t  ieut_findValueInHashSet(ieutHashSet_t *set, uint64_t value);
void     ieut_removeValueFromHashSet(ieutHashSet_t *set, uint64_t value);

#if 0
void ieut_checkHashSet(ieutHashSet_t *set);
#else
#define ieut_checkHashSet(set)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_ENGINE_HASHSET_DEFINED */
