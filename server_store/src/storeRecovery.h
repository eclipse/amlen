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

#ifndef __ISM_STORERECOVERY_DEFINED
#define __ISM_STORERECOVERY_DEFINED

#include "store.h"
#include "storeMemory.h"
#include "ha.h"
#include "storeDiskUtils.h"

typedef struct ismStore_RecoveryParameters_t
{
   uint64_t            MinMemoryBytes;         /* Minimum memory in bytes for the recovery         */
   uint64_t            MaxMemoryBytes;         /* Maximum memory in bytes for the Standby          */
   ismHA_Role_t        Role;                   /* Role: ISM_HA_ROLE_PRIMARY or ISM_HA_ROLE_STANDBY */
   uint8_t             DiskThreadNumber;       /* DiskUtils thread number                          */
} ismStore_RecoveryParameters_t;

XAPI int32_t ism_store_memRecoveryInit(ismStore_RecoveryParameters_t *pRecoveryParams);
XAPI int32_t ism_store_memRecoveryUpdParams(ismStore_RecoveryParameters_t *pRecoveryParams);
XAPI int32_t ism_store_memRecoveryStart(void);
XAPI int32_t ism_store_memRecoveryTerm(void);
XAPI int32_t ism_store_memRecoveryAddGeneration(ismStore_GenId_t genId, char *pData, uint64_t dataLength, uint8_t fActive);
XAPI int32_t ism_store_memRecoveryDelGeneration(ismStore_GenId_t genId);
XAPI int32_t ism_store_memRecoveryCompleted();
XAPI int32_t ism_store_memGetNextGenId(ismStore_IteratorHandle *pIterator, ismStore_GenId_t *pGenId);
XAPI int32_t ism_store_memGetNextRecordForType(ismStore_IteratorHandle *pIterator,
                                            ismStore_RecordType_t recordType,
                                            ismStore_GenId_t genId,
                                            ismStore_Handle_t *pHandle,
                                            ismStore_Record_t *pRecord);
XAPI int32_t ism_store_memGetNextReferenceForOwner(ismStore_IteratorHandle *pIterator,
                                                ismStore_Handle_t hOwnerHandle,
                                                ismStore_GenId_t genId,
                                                ismStore_Handle_t    *pHandle,
                                                ismStore_Reference_t *pReference);
XAPI int32_t ism_store_memGetNextStateForOwner(ismStore_IteratorHandle *pIterator,
                                            ismStore_Handle_t hOwnerHandle,
                                            ismStore_Handle_t    *pHandle,
                                            ismStore_StateObject_t *pState);
XAPI int32_t ism_store_memReadRecord(ismStore_Handle_t  handle,ismStore_Record_t *pRecord, uint8_t block);
XAPI int32_t ism_store_memReadReference(ismStore_Handle_t  handle,ismStore_Reference_t *pReference);
XAPI int32_t ism_store_memDumpCB(ismPSTOREDUMPCALLBACK cbFunc, void *cbCtx);
XAPI int32_t ism_store_memDump(char *path);
XAPI int32_t ism_store_memRecoveryLinkRefChanks(ismStore_memGenHeader_t *pGenHeader);
XAPI int32_t ism_store_memRecoveryUpdGeneration(ismStore_GenId_t genId, uint64_t **pBitMaps, uint64_t predictedSizeBytes);
XAPI int32_t ism_store_memRecoveryGetGeneration(ismStore_GenId_t genId, ismStore_DiskBufferParams_t *pBufferParams);
XAPI int32_t ism_store_memRecoveryGetGenerationData(ismStore_GenId_t genId, ismStore_DiskBufferParams_t *pBufferParams);
XAPI int32_t ism_store_memRecoveryCompletionPct(void);

#endif
