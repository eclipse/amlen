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
/// @file  engineRestoreTable.h
/// @brief Interface for Tables used for keeping track of records during
/// recovery.
//*******************************************************************
#ifndef __ISM_ENGINERESTOREBASICTABLE_DEFINED
#define __ISM_ENGINERESTOREBASICTABLE_DEFINED

//Names for various table sizes... the numbers correspond to indexes into iert_tableSizes
typedef enum tag_iertTableCapacities_t {
    iertTensOfItems                = 0,
    iertHundredsOfItems            = 1,
    iertThousandsOfItems           = 2,
    iertTensOfThousandsOfItems     = 3,
    iertHundredsOfThousandsOfItems = 4,
    iertMillionsOfItems            = 5,
    iertTensOfMillionsOfItems      = 6,
} iertTableCapacities_t;

typedef int32_t (*iert_IterationFunction_t)(ieutThreadData_t *pThreadData,
                                            uint64_t key,
                                            void *value,
                                            void *context);

struct tag_iertTable_t;
typedef struct tag_iertTable_t iertTable_t;

int32_t iert_createTable( ieutThreadData_t *pThreadData
                        , iertTable_t **outTable
                        , iertTableCapacities_t initialCapacity
                        , bool valueIsEntry
                        , bool needForStartMessaging
                        , size_t keyOffset
                        , size_t nextOffset);

void iert_freeTable(ieutThreadData_t *pThreadData, iertTable_t **inTable);

bool iert_needForStartMessaging( iertTable_t *table );

int32_t iert_addTableEntry(ieutThreadData_t *pThreadData, iertTable_t **table, uint64_t key, void *value);

int32_t iert_removeTableEntry(ieutThreadData_t *pThreadData, iertTable_t *table, uint64_t key);

void *iert_getTableEntry(const iertTable_t *table, const uint64_t key);

int32_t iert_iterateOverTable(ieutThreadData_t *pThreadData,
                              iertTable_t *table,
                              iert_IterationFunction_t pCallback,
                              void *context);

#if USE_REC_TIME
void iert_displayAndResetRecTimes(iertTable_t *table, char *label);
#endif

#define NUM_CHAINLENGTHBOUNDARIES 14
void iert_getChainLengthDistribution(iertTable_t *table,
                                     uint32_t **boundaryArray);

void iert_dumpTableKeys(iertTable_t *table, char *filename);
#endif /* __ISM_ENGINERESTOREBASICTABLE_DEFINED */

/*********************************************************************/
/* End of engineRestoreBasicTable.h                                              */
/*********************************************************************/
