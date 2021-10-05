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
/// @file  engineHashTable.c
/// @brief Engine component topic tree hash table functions
//****************************************************************************
#define TRACE_COMP Engine

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>

#include "engineInternal.h"
#include "engineHashTable.h"
#include "memHandler.h"

/* Prime numbers used in advising table capacity based on expected size. */
/* These come fomr out favourite site: http://planetmath.org/goodhashtableprimes */
static uint32_t CAPACITY_PRIMES[] = {    53,      97,     193,     389,     769,  1543,
                                       3079,    6151,   12289,   24593,   49157, 98317,
                                     196613,  393241,  786433, 1572869, 3145739};

//****************************************************************************
/// @brief Create a hash table for use by the topic tree.
///
/// @param[in]     capacity  Capacity that the hash table should allow
///                          for (number of chains).
/// @param[in]     memType   The iemem_memoryType that this hash table contains
/// @param[out]    table     Allocated table
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieut_createHashTable(ieutThreadData_t *pThreadData,
                             uint32_t capacity,
                             iemem_memoryType memType,
                             ieutHashTable_t **table)
{
    int32_t rc = OK;
    ieutHashTable_t *localTable = NULL;
    ieutHashChain_t *localChains = NULL;

    ieutTRACEL(pThreadData, memType, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    localTable = iemem_calloc(pThreadData,
                              IEMEM_PROBE(memType, 60000),
                              1, sizeof(ieutHashTable_t));

    if (NULL == localTable)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    localChains = iemem_calloc(pThreadData,
                               IEMEM_PROBE(memType, 60001),
                               1, capacity * sizeof(ieutHashChain_t));

    if (NULL == localChains)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    localTable->capacity = capacity;
    localTable->chains = localChains;
    localTable->memType = memType;

    *table = localTable;

mod_exit:
    if (rc != OK)
    {
        if (localTable) iemem_free(pThreadData, memType, localTable);
        if (localChains) iemem_free(pThreadData, memType, localChains);
        localTable = NULL;
    }

    ieutTRACEL(pThreadData, localTable,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Remove all current entries from a hash table but leave the table
///        ready for new additions.
///
/// @param[in]     table  Table to be cleared
//****************************************************************************
void ieut_clearHashTable(ieutThreadData_t *pThreadData, ieutHashTable_t *table)
{
    int32_t chain;

    ieutTRACEL(pThreadData, table, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    for(chain=0;chain<table->capacity;chain++)
    {
        if (table->chains[chain].count > 0)
        {
            int32_t index = table->chains[chain].count;
            ieutHashEntry_t *entry = table->chains[chain].entries;

            while(index > 0)
            {
                if ((entry->flags & ieutFLAG_DUPLICATE_KEY_STRING) && entry->key)
                {
                    iemem_free(pThreadData, table->memType, entry->key);
                }

                if ((entry->flags & ieutFLAG_DUPLICATE_VALUE) && entry->value)
                {
                    iemem_free(pThreadData, table->memType, entry->value);
                }

                // Don't need to reset pointers or counts for clear
                entry++;
                index--;
            }

            table->chains[chain].count = 0;
        }

        if (NULL != table->chains[chain].entries)
        {
            iemem_free(pThreadData, table->memType, table->chains[chain].entries);
            table->chains[chain].entries = NULL;
            table->chains[chain].size = 0;
        }
    }

    table->totalCount = 0;

    ieutTRACEL(pThreadData, table, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief Remove all entries from the hash table whose value is NULL
///
/// @param[in]     table  Table to be cleared
//****************************************************************************
void ieut_removeEmptyHashEntries(ieutThreadData_t *pThreadData, ieutHashTable_t *table)
{
    ieutTRACEL(pThreadData, table, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    ieutHashChain_t *chain = &table->chains[0];

    for(int32_t chain_index=0;chain_index<table->capacity;chain_index++)
    {
        if (chain->count > 0)
        {
            ieutHashEntry_t *entry = chain->entries;

            for(int32_t index=0; index<chain->count;)
            {
                if (entry->value == NULL)
                {
                    if ((entry->flags & ieutFLAG_DUPLICATE_KEY_STRING) && entry->key)
                    {
                        iemem_free(pThreadData, table->memType, entry->key);
                    }

                    chain->count--;
                    table->totalCount--;
                    memmove(entry, entry+1, (chain->count-index)*sizeof(ieutHashEntry_t));
                }
                else
                {
                    index++;
                    entry++;
                }
            }

            if (chain->count == 0)
            {
                iemem_free(pThreadData, table->memType, chain->entries);
                chain->entries = NULL;
                chain->size = 0;
            }
        }

        chain++;
    }

    ieutTRACEL(pThreadData, table, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief Visit a call-back function with all current values in a hash table
///
/// @param[in]     table     Table to be traversed
/// @param[in]     callback  Callback routine to call
/// @param[in]     context   Context information for the callback routine
//****************************************************************************
void ieut_traverseHashTable(ieutThreadData_t *pThreadData,
                            ieutHashTable_t *table,
                            ieutHashTable_TraverseCallback_t callback,
                            void *context)
{
    if (table->totalCount != 0)
    {
        int32_t chain;

        for(chain=0;chain<table->capacity;chain++)
        {
            if (table->chains[chain].count > 0)
            {
                int32_t index = table->chains[chain].count;
                ieutHashEntry_t *entry = table->chains[chain].entries;

                while(index > 0)
                {
                    callback(pThreadData,
                             entry->key,
                             entry->keyHash,
                             entry->value,
                             context);
                    entry++;
                    index--;
                }
            }
        }
    }

    return;
}

//****************************************************************************
/// @brief Visit a call-back function with all current values in a hash table.
///        If the callback returns a non-zero rc, the traverse is aborted.
///
/// @param[in]     table     Table to be traversed
/// @param[in]     callback  Callback routine to call
/// @param[in]     context   Context information for the callback routine
//****************************************************************************
int32_t ieut_traverseHashTableWithRC(ieutThreadData_t *pThreadData,
                                     ieutHashTable_t *table,
                                     ieutHashTable_TraverseCallbackWithRC_t callback,
                                     void *context)
{
    int32_t rc = OK;

    if (table->totalCount != 0)
    {
        int32_t chain;

        for(chain=0;chain<table->capacity;chain++)
        {
            if (table->chains[chain].count > 0)
            {
                int32_t index = table->chains[chain].count;
                ieutHashEntry_t *entry = table->chains[chain].entries;

                while(index > 0)
                {
                    rc = callback(pThreadData,
                                  entry->key,
                                  entry->keyHash,
                                  entry->value,
                                  context);

                    if (rc != OK)
                    {
                        ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "rc=%d\n", __func__, rc);
                        goto mod_exit;
                    }

                    entry++;
                    index--;
                }
            }
        }
    }

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief Put an entry in the hash table.
///
/// @param[in]     table        Table to add the entry to
/// @param[in]     flags        Flags controlling the way the entry is added
/// @param[in]     key          Key string to be put into the table
/// @param[in]     keyHash      A pre-calculated hash for this key string
/// @param[in]     value        Value to be associated with the key
/// @param[in]     valueLength  Length of the value associated with the key
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The key and value are duplicated and added to the hash table,
///         if the pointer to the value is NULL, then a NULL is placed in
///         the hash table value.
///
///         The duplication of keys and values is controlled by the flags
///         which can specify ieutFLAG_DUPLICATE_* values - these values are
///         all ignored if ieutFLAG_DUPLICATE_RESIZE is also set, meaning
///         that the passed values are just copied to the newly added entry
///         having _already_ been appropriately duplicated in the original
///         entry.
///
/// @remark If the key already exists in the hash table, it's value is
///         <b>replaced</b> with the newly specified one.
//****************************************************************************
int32_t ieut_putHashEntry(ieutThreadData_t *pThreadData,
                          ieutHashTable_t *table,
                          uint32_t         flags,
                          const char      *key,
                          uint32_t         keyHash,
                          void            *value,
                          uint32_t         valueLength)
{
    int32_t rc = OK;
    ieutHashChain_t *chain;

    char *newKey = NULL;
    bool  allocatedKey = false;
    void *newValue;
    bool  allocatedValue = false;

    // Make a copy of the value if required
    if (NULL != value &&
        (flags & (ieutFLAG_DUPLICATE_RESIZE | ieutFLAG_DUPLICATE_VALUE)) == ieutFLAG_DUPLICATE_VALUE)
    {
        newValue = iemem_malloc(pThreadData, IEMEM_PROBE(table->memType, 60003), valueLength);

        if (NULL == newValue)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        memcpy(newValue, value, valueLength);

        allocatedValue = true;
    }
    else
    {
        newValue = value;
    }

    chain = &table->chains[keyHash%table->capacity];

    // Resize the chain if required
    if (chain->count == chain->size)
    {
        int32_t newSize;

        if (chain->size == 0)
        {
            newSize = 2; // Assume that we will have few collisions
        }
        else
        {
            newSize = chain->size * 2;
        }

        ieutHashEntry_t *newEntries = iemem_realloc(pThreadData,
                                                    IEMEM_PROBE(table->memType, 60004),
                                                    chain->entries, newSize * sizeof(ieutHashEntry_t));

        if (NULL == newEntries)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        chain->entries = newEntries;
        chain->size = newSize;
    }

    // Check for an existing entry in this hash chain.
    int32_t index;
    bool    replace = false;

    ieutHashEntry_t *entry;

    for(index=0; index < chain->count; index++)
    {
        entry = &chain->entries[index];

        if (entry->keyHash == keyHash)
        {
            if (UNLIKELY((flags & ieutFLAG_NUMERIC_KEY) != 0))
            {
                if (entry->key != key)
                {
                    continue;
                }
            }
            else if (strcmp(entry->key, key) != 0)
            {
                continue;
            }

            replace = true;
            break;
        }
        else if (entry->keyHash > keyHash)
        {
            break;
        }
    }

    if (replace)
    {
        if ((flags & ieutFLAG_REPLACE_EXISTING) == 0)
        {
            rc = ISMRC_ExistingKey;  // Not necessarily a failure, let caller decide
            goto mod_exit;
        }

        // Assert that we have not changed key duplication strategy
        assert((flags & ieutFLAG_DUPLICATE_KEY_STRING) == (entry->flags & ieutFLAG_DUPLICATE_KEY_STRING));
        // Assert that we are not resizing
        assert((flags & ieutFLAG_DUPLICATE_RESIZE) == 0);

        // Re-use the duplicated value from this entry
        if ((flags & ieutFLAG_DUPLICATE_KEY_STRING) == ieutFLAG_DUPLICATE_KEY_STRING)
        {
            newKey = entry->key;
        }
        else
        {
            newKey = (char *)key;
        }

        if (entry->flags & ieutFLAG_DUPLICATE_VALUE)
        {
            iemem_free(pThreadData, table->memType, entry->value);
        }
    }
    else
    {
        // Make a copy of the key if required
        if ((flags & (ieutFLAG_DUPLICATE_RESIZE | ieutFLAG_DUPLICATE_KEY_STRING)) == ieutFLAG_DUPLICATE_KEY_STRING)
        {
            size_t keyLen = strlen(key)+1;

            newKey = iemem_malloc(pThreadData, IEMEM_PROBE(table->memType, 60002), keyLen);

            if (NULL == newKey)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            memcpy(newKey, key, keyLen);

            allocatedKey = true;
        }
        else
        {
            newKey = (char *)key;
        }

        if (index < chain->count)
        {
            memmove(&(chain->entries[index+1]),
                    &(chain->entries[index]),
                    (chain->count-index)*sizeof(ieutHashEntry_t));
        }

        chain->count++;
        table->totalCount++;

        entry = &chain->entries[index];

        entry->keyHash = keyHash;
    }

    entry->key = newKey;
    entry->value = newValue;
    entry->flags = (flags & ~ieutFLAG_DUPLICATE_RESIZE);

mod_exit:

    if (rc != OK)
    {
        if (allocatedKey) iemem_free(pThreadData, table->memType, newKey);
        if (allocatedValue) iemem_free(pThreadData, table->memType, newValue);
    }

    return rc;
}

//****************************************************************************
/// @brief Get an entry from the hash table.
///
/// @param[in]     table    Table to get the entry from
/// @param[in]     key      Key string to get from the table
/// @param[in]     keyHash  A pre-calculated hash for this key string
/// @param[out]    value    Value found
///
/// @return OK if the entry is found, ENTRY_NOT_FOUND if the entry is not
///         found or an ISMRC_ value.
//****************************************************************************
int32_t ieut_getHashEntry(ieutHashTable_t  *table,
                          const char       *key,
                          uint32_t          keyHash,
                          void            **value)
{
    int32_t rc = ISMRC_NotFound;
    ieutHashChain_t *chain;

    chain = &table->chains[keyHash%table->capacity];

    for(int32_t index=0; index<chain->count; index++)
    {
        ieutHashEntry_t *entry = &chain->entries[index];

        if (entry->keyHash == keyHash)
        {
            if (UNLIKELY((entry->flags & ieutFLAG_NUMERIC_KEY) != 0))
            {
                if (entry->key != key)
                {
                    continue;
                }
            }
            else if (strcmp(entry->key, key) != 0)
            {
                continue;
            }

            *value = (void *)entry->value;
            rc = OK;
            break;
        }
        else if (entry->keyHash > keyHash)
        {
            break;
        }
    }

    return rc;
}

//****************************************************************************
/// @brief Remove a specific entry from the hash table
///
/// @param[in]     table    Table to be cleared
/// @param[in]     key      Key string to be removed from the table
/// @param[in]     keyHash  A pre-calculated hash value for the key string
///
/// @remark Note, if the key specified is not found in the table there is no
///         failure returned.
//****************************************************************************
void ieut_removeHashEntry(ieutThreadData_t *pThreadData,
                          ieutHashTable_t  *table,
                          const char       *key,
                          uint32_t          keyHash)
{
    ieutHashChain_t *chain = &table->chains[keyHash%table->capacity];

    for(int32_t index=0; index<chain->count; index++)
    {
        ieutHashEntry_t *entry = &chain->entries[index];

        if (entry->keyHash == keyHash)
        {
            if (UNLIKELY((entry->flags & ieutFLAG_NUMERIC_KEY) != 0))
            {
                if (entry->key != key)
                {
                    continue;
                }
            }
            else if (strcmp(entry->key, key) != 0)
            {
                continue;
            }

            if (entry->flags & ieutFLAG_DUPLICATE_KEY_STRING)
            {
                iemem_free(pThreadData, table->memType, entry->key);
            }

            if (NULL != entry->value && (entry->flags & ieutFLAG_DUPLICATE_VALUE))
            {
                iemem_free(pThreadData, table->memType, entry->value);
            }
            chain->count--;
            table->totalCount--;

            memmove(entry, entry+1, (chain->count-index)*sizeof(ieutHashEntry_t));
            break;
        }
        else if (entry->keyHash > keyHash)
        {
            break;
        }
    }

    return;
}

//****************************************************************************
/// @brief Resize a hash table
///
/// @param[in]     table        Table to be resized
/// @param[in]     newCapacity  New capacity requested for the table
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieut_resizeHashTable(ieutThreadData_t *pThreadData,
                             ieutHashTable_t *table,
                             int32_t newCapacity)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, newCapacity, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    ieutHashChain_t *newChains = iemem_calloc(pThreadData,
                                              IEMEM_PROBE(table->memType, 60005),
                                              1, newCapacity * sizeof(ieutHashChain_t));

    if (NULL == newChains)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    ieutHashTable_t  newTable;

    newTable.capacity = newCapacity;
    newTable.totalCount = 0;
    newTable.chains = newChains;
    newTable.memType = table->memType;

    if (table->totalCount != 0)
    {
        int chainIndex;

        for(chainIndex=0; chainIndex<table->capacity; chainIndex++)
        {
            if (table->chains[chainIndex].count > 0)
            {
                int32_t               entryIndex = table->chains[chainIndex].count;
                ieutHashEntry_t *oldEntry = table->chains[chainIndex].entries;

                while(entryIndex > 0)
                {
                    // Note the use of ieutFLAG_DUPLICATE_RESIZE
                    rc = ieut_putHashEntry(pThreadData,
                                           &newTable,
                                           oldEntry->flags | ieutFLAG_DUPLICATE_RESIZE,
                                           oldEntry->key,
                                           oldEntry->keyHash,
                                           oldEntry->value,
                                           0);

                    if (rc != OK) goto mod_exit;

                    oldEntry++;
                    entryIndex--;
                }
            }

            if (NULL != table->chains[chainIndex].entries)
            {
                iemem_free(pThreadData, table->memType, table->chains[chainIndex].entries);
            }
        }

        iemem_free(pThreadData, table->memType, table->chains);
    }

    table->capacity = newTable.capacity;
    table->totalCount = newTable.totalCount;
    table->chains = newChains;

mod_exit:

    if (rc != OK && NULL != newChains)
    {
        iemem_free(pThreadData, table->memType, newChains);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Destroy a hash table in use by the topic tree, freeing all
//         associated storage.
///
/// @param[in]     table  Table to be destroyed
//****************************************************************************
void ieut_destroyHashTable(ieutThreadData_t *pThreadData,
                           ieutHashTable_t *table)
{
    ieutTRACEL(pThreadData, table, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Start out by clearing the entire table
    ieut_clearHashTable(pThreadData, table);

    // Free the chains
    if (table->chains)
    {
        iemem_free(pThreadData, table->memType, table->chains);
    }

    iemem_free(pThreadData, table->memType, table);

    ieutTRACEL(pThreadData, table, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief Suggest a capacity for a hash table based on the number of entries
/// we expect to add to it.
///
/// @param[in]     expectedCount   The expected count of entries
/// @param[in]     limit           Limit to the returned value
///
/// @return The suggested capacity to use for a table with that many entries.
//****************************************************************************
uint32_t ieut_suggestCapacity(ieutThreadData_t *pThreadData, uint64_t expectedCount, uint64_t limit)
{
    int32_t entry = sizeof(CAPACITY_PRIMES)/sizeof(CAPACITY_PRIMES[0]);
    uint64_t useCapacity;

    assert(entry > 1);

    // limit of 0 - treat as no limit
    if (limit == 0) limit = CAPACITY_PRIMES[entry-1];

    while(--entry >= 0)
    {
        useCapacity = CAPACITY_PRIMES[entry];
        if (limit >= useCapacity && expectedCount >= useCapacity)
        {
            break;
        }
    }

    ieutTRACEL(pThreadData, useCapacity, ENGINE_FNC_TRACE, FUNCTION_IDENT "capacity=%lu [limit=%lu]\n", __func__, useCapacity, limit);

    return (uint32_t)useCapacity;
}
