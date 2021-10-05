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
/// @file  engineRestoreTable.c
/// @brief Tables used for keeping track of records during recovery.
//****************************************************************************
#define TRACE_COMP Engine

#include "engineInternal.h"
#include "engineRestoreTable.h"
#include "memHandler.h"

//These sizes are good primes for hash table sizes based on:
// http://planetmath.org/goodhashtableprimes
#define NUM_TABLESIZES 7
static uint64_t iert_tableSizes[NUM_TABLESIZES] = {389, 3079, 24593, 196613, 1572869, 6291469, 201326611};
static uint64_t iert_maxTableSize = 201326611;

struct tag_iertTable_t {
      uint64_t  numChains;             ///< Current number of chains in the table
      uint64_t  numEntries;            ///< Current number of entries in the table
      uint64_t  resizeThreshold;       ///< Table will be resized when num entries reaches this
      size_t    keyOffset;             ///< Offset of the key in the entry if valueIsEntry is set
      size_t    nextOffset;            ///< Offset of the next pointer in the entry if valueIsEntry is set
      bool      valueIsEntry;          ///< If the entries in the table are in fact the values
      bool      needForStartMessaging; ///< If this table is required by the ierr_startMessaging function
      #if USE_REC_TIME
      double    getTime;
      double    addTime;
      double    removeTime;
      double    getTotal;
      double    addTotal;
      double    removeTotal;
      #endif
      void     *chainArray[];    ///< First entry in each chain
}; //iertTable_t is typdef'd to be this structure (via a forward declaration in engineRestoreTable.h

// Entry used when valueIsEntry is not set
typedef struct tag_iertEntry_t {
    uint64_t key;
    void *value;
    struct tag_iertEntry_t *next;
} iertEntry_t;

#define IERT_RESIZE_LOAD_FACTOR 1.2  ///< When numEntries/numChains is greater than this the

///@brief create a table with a specific number of chains
///
///@param[out] outTable          the table that has been created
///@param[in]  initialNumChains  Make sure it's prime!
///
static inline int32_t iert_createTableSize( ieutThreadData_t *pThreadData
                                          , iertTable_t **outTable
                                          , uint64_t initialNumChains
                                          , bool valueIsEntry
                                          , bool needForStartMessaging
                                          , size_t keyOffset
                                          , size_t nextOffset)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, initialNumChains, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    iertTable_t *table = iemem_calloc(pThreadData,
                                      IEMEM_PROBE(iemem_restoreTable, 2), 1,
                                      sizeof(iertTable_t)+(initialNumChains*sizeof(table->chainArray[0])));

    if (table != NULL)
    {
        table->numChains = initialNumChains;
        table->resizeThreshold = table->numChains * IERT_RESIZE_LOAD_FACTOR;
        table->numEntries = 0;
        if (valueIsEntry)
        {
            table->valueIsEntry = true;
            table->keyOffset = keyOffset;
            table->nextOffset = nextOffset;
        }
        else
        {
            table->valueIsEntry = false;
            table->keyOffset = offsetof(iertEntry_t, key);
            table->nextOffset = offsetof(iertEntry_t, next);
        }
        table->needForStartMessaging = needForStartMessaging;
        *outTable = table;
    }
    else
    {
        rc =  ISMRC_AllocateError;
        ism_common_setError(rc);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

// @brief Generate a hash value from a 64-bit key (e.g. a store handle)
//
// The following hash function is based on observations of a large store, filled with 32 byte
// messages, it results in a really good distribution with the largest table size.
static inline __attribute__((always_inline)) uint64_t iert_generateKeyHash(uint64_t key, uint64_t numChains)
{
  return (key ^ (key << 16)) % numChains;
}

///@brief Resizes the supplied table
///
///Resizes the table to the next size in the iert_tableSizes array bigger than the current numEntries
static inline int32_t iert_resizeTable(ieutThreadData_t *pThreadData, iertTable_t **inouttable)
{
    uint64_t newSize=0;
    iertTable_t *oldtable = *inouttable;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, oldtable, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    //We shouldn't be called if the table is at maximum size
    assert(oldtable->numChains < iert_maxTableSize);
    for (uint32_t i = 0; i < NUM_TABLESIZES; i++)
    {
        if (iert_tableSizes[i] > oldtable->numEntries)
        {
            newSize = iert_tableSizes[i];
            break;
        }
    }

    assert(newSize > oldtable->numEntries);

    ieutTRACEL(pThreadData, newSize, ENGINE_NORMAL_TRACE, "Resizing table %p to %lu\n", oldtable, newSize);

    iertTable_t *newTable = NULL;
    size_t keyOffset = oldtable->keyOffset;
    size_t nextOffset = oldtable->nextOffset;
    rc = iert_createTableSize(pThreadData,
                              &newTable,
                              newSize,
                              oldtable->valueIsEntry,
                              oldtable->needForStartMessaging,
                              keyOffset,
                              nextOffset);

    if (rc == OK)
    {
        #if USE_REC_TIME
        newTable->getTime = oldtable->getTime;
        newTable->addTime = oldtable->addTime;
        newTable->removeTime = oldtable->removeTime;
        newTable->getTotal = oldtable->getTotal;
        newTable->addTotal = oldtable->addTotal;
        newTable->removeTotal = oldtable->removeTotal;
        #endif

        uint64_t oldNumChains = oldtable->numChains;
        uint64_t newNumChains = newTable->numChains;
        uint64_t movedEntries = 0;

        // Move all of the existing entries to the new table
        for(uint64_t chainNum = 0; chainNum < oldNumChains; chainNum++)
        {
            void *nextEntry;

            for (iertEntry_t *entry = oldtable->chainArray[chainNum];
                 entry != NULL;
                 entry = nextEntry)
            {
                uint64_t key = *(uint64_t *)(((uint8_t *)entry) + keyOffset);
                void **nextEntryPtr = (void **)(((uint8_t *)entry) + nextOffset);
                void **newChainPtr = &(newTable->chainArray[iert_generateKeyHash(key, newNumChains)]);

                nextEntry = *nextEntryPtr;
                *nextEntryPtr = *newChainPtr;
                *newChainPtr = entry;
                movedEntries++;
            }

            oldtable->chainArray[chainNum] = NULL;
        }

        newTable->numEntries = movedEntries;

        //free the old table
        iert_freeTable(pThreadData, &oldtable);

        //Tell the caller about the new table
        *inouttable = newTable;
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int32_t iert_createTable( ieutThreadData_t *pThreadData
                        , iertTable_t **outTable
                        , iertTableCapacities_t initialCapacity
                        , bool valueIsEntry
                        , bool needForStartMessaging
                        , size_t keyOffset
                        , size_t nextOffset)
{
   int32_t rc = iert_createTableSize(pThreadData,
                                     outTable,
                                     iert_tableSizes[initialCapacity],
                                     valueIsEntry,
                                     needForStartMessaging,
                                     keyOffset,
                                     nextOffset);
   return rc;
}

void iert_freeTable(ieutThreadData_t *pThreadData, iertTable_t **inTable )
{
    iertTable_t *table  = *inTable;
    ieutTRACEL(pThreadData, inTable, ENGINE_NORMAL_TRACE, "Freeing table %p\n", *inTable);

    if (!table->valueIsEntry)
    {
        for (uint64_t chainNum = 0; chainNum < table->numChains; chainNum++)
        {
            iertEntry_t *entry = table->chainArray[chainNum];

            while (entry != NULL)
            {
                iertEntry_t *tmp = entry;
                entry = entry->next;

                iemem_free(pThreadData, iemem_restoreTable, tmp);
            }
        }
    }

    iemem_free(pThreadData, iemem_restoreTable, table);
    *inTable = NULL;
}

bool iert_needForStartMessaging( iertTable_t *table )
{
    return table->needForStartMessaging;
}

///
/// @remark does not check for dupes.
///
/// @remark 0 is not a valid key!
#if USE_REC_TIME
int32_t iert_addTableEntry_(ieutThreadData_t *pThreadData, iertTable_t **ppTable, uint64_t key, void *value);
int32_t iert_addTableEntry(ieutThreadData_t *pThreadData, iertTable_t **ppTable, uint64_t key, void *value)
{
  double t;
  int32_t rc ;
  t = ism_common_readTSC() ;
  rc = iert_addTableEntry_(pThreadData,ppTable,key,value);
  (*ppTable)->addTime += ism_common_readTSC()-t ;
  return rc ;
}
int32_t iert_addTableEntry_(ieutThreadData_t *pThreadData, iertTable_t **ppTable, uint64_t key, void *value)
#else
int32_t iert_addTableEntry(ieutThreadData_t *pThreadData, iertTable_t **ppTable, uint64_t key, void *value)
#endif
{
    int32_t rc = OK;
    iertTable_t *table = *ppTable;

    //ieutTRACEL(pThreadData, key, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "table=%p key=0x%lx\n", __func__, table, key);

    assert(key != 0);

    // Is a table resize required?
    if (table->numEntries > table->resizeThreshold)
    {
        // Must not exceed the maximum table size
        if (table->numChains < iert_maxTableSize)
        {
            rc = iert_resizeTable(pThreadData, ppTable);

            if (rc != OK) goto mod_exit;

            table = *ppTable;
        }
        else
        {
            // avoid going into the resize check again
            table->resizeThreshold *= 2;
        }
    }

    void **chain = &(table->chainArray[iert_generateKeyHash(key, table->numChains)]);

    if (table->valueIsEntry)
    {
        assert(key == *(uint64_t *)((uint8_t *)value + table->keyOffset));

        *(void **)((uint8_t *)value + table->nextOffset) = *chain;
        *chain = value;
    }
    else
    {
        iertEntry_t *newEntry = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_restoreTable, 3), sizeof(iertEntry_t));

        if (newEntry != NULL)
        {
            newEntry->key = key;
            newEntry->value = value;
            newEntry->next = *chain;
            *chain = newEntry;
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    table->numEntries++;

mod_exit:
    //ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


///@brief Remove an entry from a table
///
///@remark doesn't check for dupes (so would just remove first entry)
#if USE_REC_TIME
int32_t iert_removeTableEntry_(ieutThreadData_t *pThreadData, iertTable_t *table, uint64_t key);
int32_t iert_removeTableEntry(ieutThreadData_t *pThreadData, iertTable_t *table, uint64_t key)
{
  double t;
  int32_t rc ;
  t = ism_common_readTSC() ;
  rc = iert_removeTableEntry_(pThreadData,table,key);
  table->removeTime += ism_common_readTSC()-t ;
  return rc ;
}
int32_t iert_removeTableEntry_(ieutThreadData_t *pThreadData, iertTable_t *table, uint64_t key)
#else
int32_t iert_removeTableEntry(ieutThreadData_t *pThreadData, iertTable_t *table, uint64_t key)
#endif
{
    int32_t rc = ISMRC_NotFound;
    uint64_t chainNum = iert_generateKeyHash(key, table->numChains);

    //ieutTRACEL(pThreadData, key, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "table=%p key=0x%lx\n", __func__, table, key);

    void **prevNext = &table->chainArray[chainNum];
    void *entry = *prevNext;

    size_t keyOffset = table->keyOffset;
    size_t nextOffset = table->nextOffset;

    while (entry != NULL)
    {
        void **entryNextPtr = (void **)(((uint8_t *)entry) + nextOffset);

        if (key == *(uint64_t *)(((uint8_t *)entry) + keyOffset))
        {
            //take the matching entry out of the list
            *prevNext = *entryNextPtr;

            if (table->valueIsEntry)
            {
                *entryNextPtr = NULL;
            }
            else
            {
                iemem_free(pThreadData, iemem_restoreTable, entry);
            }

            assert(table->numEntries > 0);
            table->numEntries--;

            rc = OK; //We found the entry
            break;
        }

        prevNext = entryNextPtr;
        entry = *entryNextPtr;
    }

    //ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

#if USE_REC_TIME
void *iert_getTableEntry_(const iertTable_t *table, const uint64_t key);
void *iert_getTableEntry(const iertTable_t *table, const uint64_t key)
{
  double t;
  t = ism_common_readTSC() ;
  void *entry = iert_getTableEntry_(table,key);
  table->getTime += ism_common_readTSC()-t ;
  return entry;
}
void *iert_getTableEntry_(const iertTable_t *table, const uint64_t key)
#else
void *iert_getTableEntry(const iertTable_t *table, const uint64_t key)
#endif
{
    if (table->numEntries == 0) return NULL;

    if (table->valueIsEntry)
    {
        for (uint8_t *entry = table->chainArray[iert_generateKeyHash(key, table->numChains)];
             entry != NULL;
             entry = *(uint8_t **)(entry + table->nextOffset))
        {
            if (*(uint64_t *)(entry + table->keyOffset) == key)
            {
                return entry;
            }
        }
    }
    else
    {
        for (iertEntry_t *entry = table->chainArray[iert_generateKeyHash(key, table->numChains)];
             entry != NULL;
             entry = entry->next)
        {
            if (entry->key == key)
            {
                return entry->value;
            }
        }
    }

    return NULL;
}

///@brief For each entry call the supplied function.
int32_t iert_iterateOverTable(ieutThreadData_t *pThreadData,
                              iertTable_t *table,
                              iert_IterationFunction_t pCallback,
                              void *context)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, table, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    size_t keyOffset = table->keyOffset;
    size_t nextOffset = table->nextOffset;
    bool valueIsEntry = table->valueIsEntry;

    if (table->numEntries != 0)
    {
        for(uint64_t chainNum = 0; chainNum < table->numChains && (rc == OK); chainNum++)
        {
            void *nextEntry;

            for (void *entry = table->chainArray[chainNum];
                 entry != NULL;
                 entry = nextEntry)
            {
                uint64_t entryKey = *(uint64_t *)(((uint8_t *)entry) + keyOffset);
                nextEntry = *(void **)(((uint8_t *)entry) + nextOffset);

                if (entryKey != 0)
                {
                    rc = pCallback(pThreadData,
                                   entryKey,
                                   valueIsEntry ? entry : ((iertEntry_t *)entry)->value,
                                   context);
                }
            }
        }
    }

    ieutTRACEL(pThreadData, table, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

/// Below here are function only used in debugging/testing

static uint32_t iert_getChainLength( iertTable_t *table
                                   , uint64_t chainNum)
{
    int chainLength = 0;

    void *entry = table->chainArray[chainNum];
    size_t nextOffset = (size_t)table->nextOffset;

    while (entry != NULL)
    {
        chainLength++;
        entry = *(void **)(((uint8_t *)entry) + nextOffset);
    }

    return chainLength;
}

void iert_getChainLengthDistribution(iertTable_t *table,
                                     uint32_t **boundaryArray)
{
    uint32_t chainLengthBoundaries[NUM_CHAINLENGTHBOUNDARIES]={1,2,3,4,5,6,7,8,9,10,20,30,50,1000};
    uint32_t chainCounts[NUM_CHAINLENGTHBOUNDARIES+1]={0};

    for(uint32_t chainNum = 0; chainNum < table->numChains; chainNum++)
    {
        uint32_t chainLength = iert_getChainLength(table, chainNum);

        int i=0;
        for(i=0; i< NUM_CHAINLENGTHBOUNDARIES; i++)
        {
            if(chainLength < chainLengthBoundaries[i])
            {
                chainCounts[i]++;
                break;
            }
        }
        if(i == NUM_CHAINLENGTHBOUNDARIES)
        {
            /* We weren't less than any boundary */
            chainCounts[NUM_CHAINLENGTHBOUNDARIES]++;
        }
    }

    if (boundaryArray == NULL)
    {
        int i=0;
        for(i=0; i< NUM_CHAINLENGTHBOUNDARIES; i++)
        {
            TRACE(1, "Num chains of length < %u = %u\n", chainLengthBoundaries[i], chainCounts[i]);
        }
        TRACE(1, "Number of longer chains is %u\n", chainCounts[NUM_CHAINLENGTHBOUNDARIES]);
    }
    else
    {
        memcpy(boundaryArray, chainCounts, sizeof(chainCounts));
    }
}

#if USE_REC_TIME
#define ARRAY_LENGTH 1000000
void iert_dumpTableKeys(iertTable_t *table, char *filename)
{
    uint64_t *keyArray;

    FILE *fp = fopen(filename, "ab");

    if (fp == NULL)
    {
        TRACE(1, "Couldn't open %s for writing\n", filename);
        return;
    }

    keyArray = ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,37),sizeof(uint64_t) * ARRAY_LENGTH);

    if (keyArray == NULL)
    {
        TRACE(1, "Couldn't allocate array\n");
        fclose(fp);
        return;
    }

    uint32_t arrayPos = 0;
    size_t keyOffset = (size_t)table->keyOffset;
    size_t nextOffset = (size_t)table->nextOffset;
    for(uint32_t chainNum = 0; chainNum < table->numChains; chainNum++)
    {
        void *entry = table->chainArray[chainNum];

        while (entry != NULL)
        {
            keyArray[arrayPos++] = *(uint64_t *)(((uint8_t *)entry) + keyOffset);
            entry = *(void **)(((uint8_t *)entry) + nextOffset);

            if (arrayPos == ARRAY_LENGTH)
            {
                fwrite(keyArray, sizeof(keyArray[0]), arrayPos, fp);
                arrayPos = 0;
            }
        }
    }

    if (arrayPos != 0)
    {
        fwrite(keyArray, sizeof(keyArray[0]), arrayPos, fp);
        arrayPos = 0;
    }

    ism_common_free(ism_memory_engine_misc,keyArray);
    fclose(fp);
}

void iert_displayAndResetRecTimes(iertTable_t *table, char *label)
{
    TRACE(1, "Table '%s' get: %.5f add: %.5f remove: %.5f (totals: %.2f %.2f %.2f)\n",
          label, table->getTime, table->addTime, table->removeTime,
          table->getTotal, table->addTotal, table->removeTotal);
    //iert_getChainLengthDistribution(table, NULL);
    table->getTotal += table->getTime;
    table->addTotal += table->addTime;
    table->removeTotal += table->removeTime;
    table->getTime = 0e0;
    table->addTime = 0e0;
    table->removeTime = 0e0;
}
#endif

