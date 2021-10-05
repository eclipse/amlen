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
/// @file  engineHashTable.h
/// @brief Defines structures and functions used internally engine hashtables
//****************************************************************************
#ifndef __ISM_ENGINE_ENGINE_HASHTABLE_DEFINED
#define __ISM_ENGINE_ENGINE_HASHTABLE_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "memHandler.h"

/*********************************************************************/
/* Structures and constants from engineHashTable.c                   */
/*********************************************************************/
#define ieutFLAG_DUPLICATE_NONE        0x00000000  ///< None of the key/value data should be duplicated
#define ieutFLAG_DUPLICATE_KEY_STRING  0x00000001  ///< The key string should be duplicated
#define ieutFLAG_DUPLICATE_VALUE       0x00000002  ///< The value should be duplicated
#define ieutFLAG_DUPLICATE_RESIZE      0x00000004  ///< Resizing, so just copy to entry
#define ieutFLAG_REPLACE_EXISTING      0x00000008  ///< Allow the replacement of an existing entry
#define ieutFLAG_NUMERIC_KEY           0x00000010  ///< The key is not a string, but a numeric value

#define ieutFLAG_DUPLICATE_ALL         (ieutFLAG_DUPLICATE_VALUE | ieutFLAG_DUPLICATE_KEY_STRING)

//****************************************************************************
/// @brief An entry in a hash table chain
/// @see ieutHashChain_t
//****************************************************************************
typedef struct tag_ieutHashEntry_t
{
    char                *key;      ///< Key (string) to this hash entry
    void                *value;    ///< Value of this hash entry
    uint32_t             keyHash;  ///< Full key hash for this key
    uint32_t             flags;    ///< Whether the key and value were duplicated
} ieutHashEntry_t;

//****************************************************************************
/// @brief A single hash chain in a hash table
/// @see ieutHashTable_t
/// @see ieutHashEntry_t
//****************************************************************************
typedef struct tag_ieutHashChain_t
{
    uint32_t          count;    ///< How many entries are in this chain
    uint32_t          size;     ///< The size of the entries array allocated for this chain
    ieutHashEntry_t * entries;  ///< Pointer to the entries for this chain
} ieutHashChain_t;

//****************************************************************************
/// @brief A topic hash table
/// @see ieutHashChain_t
//****************************************************************************
typedef struct tag_ieutHashTable_t
{
    uint64_t               totalCount;  ///< Total entries in the entire hash table
    ieutHashChain_t *      chains;      ///< Array of chains
    uint32_t               capacity;    ///< Size of the chains array
    iemem_memoryType       memType;     ///< Type of memory to allocate for this table
} ieutHashTable_t;

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ieutHashTable_t(__file)\
    iedm_descriptionStart(__file, ieutHashTable_t,,"");\
    iedm_describeMember(uint64_t,               totalCount);\
    iedm_describeMember(ieutHashChain_t *,      chains);\
    iedm_describeMember(uint32_t,               capacity);\
    iedm_describeMember(iemem_memoryType,       memType);\
    iedm_descriptionEnd;

//****************************************************************************
/// @brief Callback function used when traversing a hash table
//****************************************************************************
typedef void (*ieutHashTable_TraverseCallback_t)(ieutThreadData_t *pThreadData,
                                                 char *key,
                                                 uint32_t keyHash,
                                                 void *value,
                                                 void *context);

//****************************************************************************
/// @brief Callback function used when traversing a hash table but possibly
/// wanting to abort part way through Non-zero RC from this function ends traverse
//****************************************************************************
typedef int32_t (*ieutHashTable_TraverseCallbackWithRC_t)(ieutThreadData_t *pThreadData,
                                                          char *key,
                                                          uint32_t keyHash,
                                                          void *value,
                                                          void *context);

/*********************************************************************/
/* FUNCTION PROTOTYPES                                               */
/*********************************************************************/
int32_t  ieut_createHashTable(ieutThreadData_t *pThreadData,
                              uint32_t capacity,
                              iemem_memoryType memType,
                              ieutHashTable_t **table);
int32_t  ieut_resizeHashTable(ieutThreadData_t *pThreadData,
                              ieutHashTable_t *table,
                              int32_t newCapacity);
void     ieut_clearHashTable(ieutThreadData_t *pThreadData,
                             ieutHashTable_t *table);
void     ieut_removeEmptyHashEntries(ieutThreadData_t *pThreadData,
                                     ieutHashTable_t *table);
void     ieut_traverseHashTable(ieutThreadData_t *pThreadData,
                                ieutHashTable_t *table,
                                ieutHashTable_TraverseCallback_t callback,
                                void *context);
int32_t ieut_traverseHashTableWithRC(ieutThreadData_t *pThreadData,
                                     ieutHashTable_t *table,
                                     ieutHashTable_TraverseCallbackWithRC_t callback,
                                     void *context);
void     ieut_destroyHashTable(ieutThreadData_t *pThreadData,
                               ieutHashTable_t *table);
uint32_t ieut_suggestCapacity(ieutThreadData_t *pThreadData,
                              uint64_t expectedCount,
                              uint64_t limit);
int32_t  ieut_putHashEntry(ieutThreadData_t *pThreadData,
                           ieutHashTable_t *table,
                           uint32_t         flags,
                           const char      *key,
                           uint32_t         keyHash,
                           void            *value,
                           uint32_t         valueLength);
int32_t  ieut_getHashEntry(ieutHashTable_t *table,
                           const char      *key,
                           uint32_t         keyHash,
                           void           **value);
void     ieut_removeHashEntry(ieutThreadData_t *pThreadData,
                              ieutHashTable_t *table,
                              const char      *key,
                              uint32_t         keyHash);


//****************************************************************************
/// @brief Generate a 32bit hash value for a specified numeric 64bit key.
///
/// @param[in]     key  The key for which to generate a hash value
///
/// @remark This is a version of the xor djb2 hashing algorithm
///
/// @return The hash value
//****************************************************************************
static inline uint32_t ieut_generateUInt64Hash(uint64_t key)
{
    uint32_t keyHash = 5381;
    uint8_t *curByte = (uint8_t *)&key;
    uint8_t *endChar = curByte + sizeof(uint64_t);

    while(curByte < endChar)
    {
        keyHash = (keyHash * 33) ^ *curByte++;
    }

    return keyHash;
}

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_ENGINE_HASHTABLE_DEFINED */
