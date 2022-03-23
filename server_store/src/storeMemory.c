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

/*********************************************************************/
/*                                                                   */
/* Module Name: storeMemory.c                                        */
/*                                                                   */
/* Description: Memory module of Store Component of ISM              */
/*                                                                   */
/*********************************************************************/
#define TRACE_COMP Store
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "storeInternal.h"
#include "storeMemory.h"
#include "storeRecovery.h"
#include "storeLockFile.h"
#include "storeHighAvailability.h"
#include "storeMemoryHA.h"
#include "storeUtils.h"
#include "storeShmPersist.h"
#include <cluster.h>

extern ismStore_global_t ismStore_global;
extern pthread_mutex_t ismStore_FnMutex;
extern void ism_engine_threadInit(uint8_t isStoreCrit);
extern void ism_engine_threadTerm(uint8_t closeStoreStream);

#define ismSTORE_SINGLE_GRANULE (uint32_t)-1

typedef struct ismStore_memDiskWriteCtxt_t
{
   ismStore_memGeneration_t *pGen;
   char                     *pCompData;
   uint64_t                  CompDataSizeBytes;
} ismStore_memDiskWriteCtxt_t;

typedef struct ismStore_memCompactGen_t
{
   ismStore_GenId_t          GenId;
   uint8_t                   fDelete;
   uint64_t                  ExpectedFreeBytes;
} ismStore_memCompactGen_t;

/*********************************************************************/
/* Static functions definitions.                                     */
/*********************************************************************/
static int32_t ism_store_memPrepare(void);

static ismStore_memGeneration_t *ism_store_memInitMgmtGenStruct(void);

static int32_t ism_store_memInitMgmt(
                   uint8_t fStandby2Primary);

static int ism_store_memRefStateCompar(
                   const void *pElm1,
                   const void *pElm2);

static int32_t ism_store_memRecoveryPrepare(
                   uint8_t fStandby2Primary);

static int32_t ism_store_memRestoreFromDisk(void);

static int32_t ism_store_memValidateParameters(void);

static void ism_store_memAdjustVMParameters(void);

static int32_t ism_store_memValidateShmSpace(int fd);

static int32_t ism_store_memValidateNVRAM(void);

static int32_t ism_store_memOpenStreamEx(
                   ismStore_StreamHandle_t *phStream, 
                   uint8_t fHighPerf,
                   uint8_t fIntStream);

static int32_t ism_store_memGetMgmtPoolElements(
                   ismStore_memStream_t *pStream,
                   uint8_t poolId,
                   uint16_t dataType,
                   uint64_t attribute,
                   uint64_t state,
                   uint32_t dataLength,
                   ismStore_Handle_t *pHandle,
                   ismStore_memDescriptor_t **pDesc);

static int32_t ism_store_memGetPoolElements(
                   ismStore_memStream_t *pStream,
                   ismStore_memGeneration_t *pGen,
                   uint8_t poolId,
                   uint16_t dataType,
                   uint64_t attribute,
                   uint64_t state,
                   uint32_t dataLength,
                   uint32_t cacheStreamRsrv,
                   ismStore_Handle_t *pHandle,
                   ismStore_memDescriptor_t **pDesc);

static void ism_store_memDecOwnerCount(
                   uint16_t dataType,
                   int nElements);

static void ism_store_memAssignRsrvPool(
                   uint8_t poolId);

static void ism_store_memInitRsrvPool(void);

static void ism_store_memAttachRsrvPool(void);

static void ism_store_memResetRsrvPool(void);

static void ism_store_memCopyRecordData(
                   ismStore_memDescriptor_t *pDescriptor,
                   const ismStore_Record_t *pRecord);

static int32_t ism_store_memBackup(void);

static int32_t ism_store_memValidateStream(
                   ismStore_StreamHandle_t hStream);

static int32_t ism_store_memValidateRefCtxt(
                   ismStore_memReferenceContext_t *pRefCtxt);

static int32_t ism_store_memValidateStateCtxt(
                   ismStore_memStateContext_t *pStateCtxt);

static int32_t ism_store_memValidateHandle(
                   ismStore_Handle_t handle);

static int32_t ism_store_memValidateRefHandle(
                   ismStore_Handle_t refHandle,
                   uint64_t orderId,
                   ismStore_Handle_t hOwnerHandle);

static void ism_store_fillReferenceStatistics(
                   ismStore_memSplitItem_t *pSplit,
                   ismStore_ReferenceStatistics_t *pRefStats);

static int32_t ism_store_memAllocateRefCtxt(
                   ismStore_memSplitItem_t *pSplit,
                   ismStore_Handle_t hOwnerHandle);

static void ism_store_memFreeRefCtxt(
                   ismStore_memSplitItem_t *pSplit,
                   ismStore_GenId_t genId);

static int32_t ism_store_memAllocateStateCtxt(
                   ismStore_memSplitItem_t *pSplit,
                   ismStore_Handle_t hOwnerHandle);

static void ism_store_memFreeStateCtxt(
                   ismStore_memSplitItem_t *pSplit,
                   ismStore_GenId_t genId);

static int32_t ism_store_memEnsureStoreTransAllocation(
                   ismStore_memStream_t *pStream,
                   ismStore_memDescriptor_t **pDesc);

static int32_t ism_store_memSetStreamActivity(
                   ismStore_memStream_t *pStream, uint8_t fIsActive);

static int32_t ism_store_memEnsureGenIdAllocation(
                   ismStore_memDescriptor_t **pDesc);

static uint8_t ism_store_memMapGenIdToIndex(
                   ismStore_GenId_t genId);

static int32_t ism_store_memAddGenIdToList(
                   ismStore_GenId_t genId);

static void ism_store_memDeleteGenIdFromList(
                   ismStore_GenId_t genId);

static int32_t ism_store_memAllocGenMap(
                   ismStore_GenId_t *genId);

static int32_t ism_store_memCompactGenIdList(
                   ismStore_memDescriptor_t *pPrevDescriptor,
                   ismStore_Handle_t hGenIdChunk);

static void ism_store_memFreeGenMap(
                   ismStore_GenId_t genId);

static uint8_t ism_store_memOffset2PoolId(
                   ismStore_memGenMap_t *pGenMap,
                   ismStore_Handle_t offset);


static int32_t ism_store_memActivateGeneration(
                   ismStore_GenId_t genId,
                   uint8_t genIndex);

static int32_t ism_store_memCloseGeneration(
                   ismStore_GenId_t genId,
                   uint8_t genIndex);

static int32_t ism_store_memDeleteGeneration(
                   ismStore_GenId_t genId);

static int32_t ism_store_memWriteGeneration(
                   ismStore_GenId_t genId,
                   uint8_t genIndex);

static int32_t ism_store_memCompactGeneration(
                   ismStore_GenId_t genId,
                   uint8_t priority,
                   uint8_t fForce);

static int ism_store_memCompactCompar(
                   const void *pElm1,
                   const void *pElm2);

static void ism_store_memCompactTopNGens(
                   int topN,
                   uint8_t priority,
                   uint64_t diskUsedSpaceBytes);

static void ism_store_memIncreaseRefGenPool(
                   ismStore_memRefGenPool_t *pRefGenPool);

static void ism_store_memDecreaseRefGenPool(
                   ismStore_memRefGenPool_t *pRefGenPool);

static ismStore_memRefGen_t *ism_store_memGetRefGen(
                   ismStore_memStream_t *pStream,
                   ismStore_memReferenceContext_t *pRefCtxt);

static void ism_store_memSetRefGenStates(
                   ismStore_memReferenceContext_t *pRefCtxt,
                   ismStore_memDescriptor_t *pOwnerDescriptor,
                   ismStore_memRefStateChunk_t *pRefStateChunk);

static void ism_store_memResetStreamCache(
                   ismStore_memStream_t *pStream,
                   ismStore_GenId_t genId);

static uint32_t ism_store_memGetStreamCacheCount(
                   ismStore_memGeneration_t *pGen,
                   uint8_t poolId);

static int32_t ism_store_memEnsureReferenceAllocation(
                   ismStore_memStream_t *pStream,
                   ismStore_memReferenceContext_t *pRefCtxt,
                   uint64_t orderId,
                   ismStore_Handle_t *pHandle);

static int32_t ism_store_memAssignReferenceAllocation(
                   ismStore_memStream_t *pStream,
                   ismStore_memReferenceContext_t *pRefCtxt,
                   uint64_t baseOrderId,
                   ismStore_Handle_t *pHandle,
                   ismStore_memDescriptor_t **pDesc);

static void ism_store_memPruneReferenceAllocation(
                   ismStore_memStream_t *pStream,
                   ismStore_memReferenceContext_t *pRefCtxt,
                   uint64_t minimumActiveOrderId);

static int32_t ism_store_memEnsureRefStateAllocation(
                   ismStore_memStream_t *pStream,
                   ismStore_memReferenceContext_t *pRefCtxt,
                   uint64_t orderId,
                   ismStore_Handle_t *pHandle, uint8_t fDel);

static int32_t ism_store_memAssignRefStateAllocation(
                   ismStore_memStream_t *pStream,
                   ismStore_memReferenceContext_t *pRefCtxt,
                   uint64_t baseOrderId,
                   ismStore_Handle_t *pHandle,
                   ismStore_memDescriptor_t **pDesc);

static int32_t ism_store_memAssignRecordAllocation_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memAssignRecordAllocation_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memFreeRecordAllocation_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memFreeRecordAllocation_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memUpdate_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memUpdate_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memUpdateAttribute_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memUpdateAttribute_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memUpdateState_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memUpdateState_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memCreateReference_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memCreateReference_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memDeleteReference_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memDeleteReference_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memUpdateReference_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memUpdateReference_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memUpdateRefState_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memUpdateRefState_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memEnsureStateAllocation(
                   ismStore_memStream_t *pStream,
                   ismStore_memStateContext_t *pStateCtxt,
                   const ismStore_StateObject_t *pState,
                   ismStore_Handle_t *pHandle);

static int32_t ism_store_memFreeStateAllocation(
                   ismStore_Handle_t handle);

static int32_t ism_store_memCreateState_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memCreateState_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memDeleteState_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memDeleteState_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp);

static int32_t ism_store_memCommitInternal(
                   ismStore_memStream_t *pStream,
                   ismStore_memDescriptor_t *pDescriptor);

static int32_t ism_store_memRollbackInternal(
                   ismStore_memStream_t *pStream,
                   ismStore_Handle_t handle);

#if ismSTORE_ADMIN_CONFIG
static int ism_store_memConfigCallback(
                   char *pObject,
                   char *pName,
                   ism_prop_t *pProps,
                   ism_ConfigChangeType_t flag);
#endif

static void ism_store_memCheckCompactThreshold(
                   ismStore_GenId_t genId,
                   ismStore_memGenMap_t *pGenMap);

static void ism_store_memHandleHAIntAck(
                   ismStore_memHAAck_t *pAck);

static void ism_store_memHandleHAEvent(
                   ismStore_memJob_t *pJob);

static void ism_store_memDiskWriteBackupComplete(
                   ismStore_GenId_t genId,
                   int32_t retcode,
                   ismStore_DiskGenInfo_t *pDiskGenInfo,
                   void *pContext);

static void ism_store_memDiskDeleteComplete(
                   ismStore_GenId_t genId,
                   int32_t retcode,
                   ismStore_DiskGenInfo_t *pDiskGenInfo,
                   void *pContext);

static void ism_store_memDiskReadComplete(
                   ismStore_GenId_t genId,
                   int32_t retcode,
                   ismStore_DiskGenInfo_t *pDiskGenInfo,
                   void *pContext);

static void ism_store_memDiskCompactComplete(
                   ismStore_GenId_t genId,
                   int32_t retcode,
                   ismStore_DiskGenInfo_t *pDiskGenInfo,
                   void *pContext);

static void ism_store_memDiskTerminated(
                   ismStore_GenId_t GenId,
                   int32_t retcode,
                   ismStore_DiskGenInfo_t *pDiskGenInfo,
                   void *pContext);

static void ism_store_memCheckDiskUsage(void);

static void ism_store_memRaiseUserEvent(
                   ismStore_EventType_t eventType);

static void *ism_store_memThread(
                   void *arg,
                   void *pContext,
                   int value);

static int32_t ism_store_memCheckStoreVersion(uint64_t v1, uint64_t v2);

static int32_t ism_store_memBuildRFFingers(ismStore_memReferenceContext_t *pRefCtxt);


static int32_t ism_store_memStopCB(void);

static int32_t ism_store_memManageCoolPool(void);

/*********************************************************************/
/* Global data for the memory store. Used to anchor server-wide data */
/* structures.                                                       */
/*********************************************************************/
ismStore_memGlobal_t ismStore_memGlobal =
{
   ismSTORE_STATE_CLOSED,
};
static uint64_t ismStore_GenMapSetMask[64];
static uint64_t ismStore_GenMapResetMask[64];
static uint8_t  ismStore_T2T[ISM_STORE_RECTYPE_MAXVAL]; 

/*
 * Initialize memory for the store component
 */
int32_t ism_store_memInit(void)
{
   int ec, i, j;
   const char *pRootPath;
   ism_field_t field;
   const ismStore_HAConfig_t *pHAConfig;
   size_t len;
   uint64_t mask, sizeMB;
   int32_t rc = ISMRC_OK;

   if (ismStore_memGlobal.State != ismSTORE_STATE_CLOSED)
   {
      TRACE(1, "Failed to initialize the store, because the store is already initialized. State %d\n", ismStore_memGlobal.State);
      return ISMRC_StoreBusy;
   }
   memset(&ismStore_memGlobal, '\0', sizeof(ismStore_memGlobal_t));
   ismStore_memGlobal.LockFileDescriptor = -1;

   ismStore_global.MachineMemoryBytes = ism_common_getTotalMemory() << 20 ;
   if (!ismStore_global.MachineMemoryBytes )
   {
     ismStore_global.MachineMemoryBytes = (size_t)sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
     TRACE(3,"ism_common_getTotalMemory() returned zero, using sysconf value = %lu\n",ismStore_global.MachineMemoryBytes);
   }

#if ismSTORE_ADMIN_CONFIG
   // Register the store with the configuration service
   if ((rc = ism_config_register(ISM_CONFIG_COMP_STORE,
                                 NULL,
                                 ism_store_memConfigCallback,
                                 &ismStore_memGlobal.hConfig)) != ISMRC_OK)
   {
      TRACE(1, "Failed to register the store with the configuration service. error code %d", rc);
      goto exit;
   }
   // Get current Store configuration from dynamic config file
   pDynamicProps = ism_config_getProperties(ismStore_memGlobal.hConfig, NULL, NULL);
#endif

   int32_t defaultGranuleSizeBytes, defaultTotalMemSizeMB, defaultRecoveryMemSizeMB;
   int32_t defaultPersistFileSizeMB, defaultDiskTransferBlockSize, defaultPersistBuffSize;
   int32_t defaultRefCtxtLocksCount, defaultStateCtxtLocksCount, defaultRefGenPoolExtElements;
   int32_t defaultTransactionRsrvOpts, defaultCompactMemThresholdMB, defaultCompactDiskThresholdMB;

   switch(ism_common_getIntConfig("SizeProfile", 0 /* TODO: NAMED DEFAULT */))
   {
       case 1: /* TODO: NAMED SMALL */
           defaultGranuleSizeBytes = 512;
           defaultTotalMemSizeMB = 50;
           defaultRecoveryMemSizeMB = 20;
           defaultPersistFileSizeMB = 256;
           defaultDiskTransferBlockSize = 1;
           defaultPersistBuffSize = 524288;
           defaultRefCtxtLocksCount = 10;
           defaultStateCtxtLocksCount = 10;
           defaultRefGenPoolExtElements = 1000;
           defaultTransactionRsrvOpts = 100;
           defaultCompactMemThresholdMB = 10;
           defaultCompactDiskThresholdMB = 10;
           break;
       default:
           defaultGranuleSizeBytes = ismSTORE_CFG_GRANULE_SIZE_DV;
           defaultTotalMemSizeMB = ismSTORE_CFG_TOTAL_MEMSIZE_MB_DV;
           defaultRecoveryMemSizeMB = ismSTORE_CFG_RECOVERY_MEMSIZE_MB_DV;
           defaultPersistFileSizeMB = ismSTORE_CFG_PERSIST_FILE_SIZE_MB_DV;
           defaultDiskTransferBlockSize = ismSTORE_CFG_DISK_BLOCK_SIZE_DV;
           defaultPersistBuffSize = ismSTORE_CFG_PERSIST_BUFF_SIZE_DV;
           defaultRefCtxtLocksCount = ismSTORE_CFG_REFCTXT_LOCKS_COUNT_DV;
           defaultStateCtxtLocksCount = ismSTORE_CFG_STATECTXT_LOCKS_COUNT_DV;
           defaultRefGenPoolExtElements = ismSTORE_CFG_REFGENPOOL_EXT_DV;
           defaultTransactionRsrvOpts = ismSTORE_CFG_STORETRANS_RSRV_OPS_DV;
           defaultCompactMemThresholdMB = ismSTORE_CFG_COMPACT_MEMTH_MB_DV;
           defaultCompactDiskThresholdMB = ismSTORE_CFG_COMPACT_DISKTH_MB_DV;
           break;
   }

   // Read the store configuration
   ismStore_memGlobal.NVRAMOffset = (uint64_t)ism_common_getLongConfig(ismSTORE_CFG_NVRAM_OFFSET, ismSTORE_CFG_NVRAM_OFFSET_DV);
   ismStore_memGlobal.MgmtSmallGranuleSizeBytes = ismSTORE_ROUNDUP(
                                                   ism_common_getIntConfig(ismSTORE_CFG_MGMT_SMALL_GRANULE,
                                                   ismSTORE_CFG_MGMT_SMALL_GRANULE_DV), 64);
   ismStore_memGlobal.MgmtGranuleSizeBytes = ismSTORE_ROUNDUP(
                                              ism_common_getIntConfig(ismSTORE_CFG_MGMT_GRANULE_SIZE,
                                              ismSTORE_CFG_MGMT_GRANULE_SIZE_DV), 256);
   ismStore_memGlobal.GranuleSizeBytes = ismSTORE_ROUNDUP(
                                              ism_common_getIntConfig(ismSTORE_CFG_GRANULE_SIZE,
                                              defaultGranuleSizeBytes), 256);
   ismStore_memGlobal.TotalMemSizeBytes = (uint64_t)ism_common_getLongConfig(ismSTORE_CFG_TOTAL_MEMSIZE_BYTES, 0);
   if ( !ismStore_memGlobal.TotalMemSizeBytes )
   {
   sizeMB = (uint64_t)ism_common_getIntConfig(ismSTORE_CFG_TOTAL_MEMSIZE_MB, defaultTotalMemSizeMB);
   ismStore_memGlobal.TotalMemSizeBytes = sizeMB << 20; // Convert MB to bytes
   }
   sizeMB = (uint64_t)ism_common_getIntConfig(ismSTORE_CFG_RECOVERY_MEMSIZE_MB, defaultRecoveryMemSizeMB);
   ismStore_memGlobal.RecoveryMinMemSizeBytes = sizeMB << 20; // Convert MB to bytes
   sizeMB = (uint64_t)ism_common_getIntConfig(ismSTORE_CFG_COMPACT_MEMTH_MB, defaultCompactMemThresholdMB);
   ismStore_memGlobal.CompactMemThBytes = sizeMB << 20; // Convert MB to bytes
   sizeMB = (uint64_t)ism_common_getIntConfig(ismSTORE_CFG_COMPACT_DISKTH_MB, defaultCompactDiskThresholdMB);
   ismStore_memGlobal.CompactDiskThBytes = sizeMB << 20; // Convert MB to bytes
   ismStore_memGlobal.CompactDiskHWM = ism_common_getIntConfig(ismSTORE_CFG_COMPACT_DISK_HWM, ismSTORE_CFG_COMPACT_DISK_HWM_DV);
   ismStore_memGlobal.CompactDiskLWM = ism_common_getIntConfig(ismSTORE_CFG_COMPACT_DISK_LWM, ismSTORE_CFG_COMPACT_DISK_LWM_DV);
   ismStore_memGlobal.MgmtMemPct = ism_common_getIntConfig(ismSTORE_CFG_MGMT_MEM_PCT, ismSTORE_CFG_MGMT_MEM_PCT_DV);
   ismStore_memGlobal.MgmtSmallGranulesPct = ism_common_getIntConfig(ismSTORE_CFG_MGMT_SMALL_PCT, ismSTORE_CFG_MGMT_SMALL_PCT_DV);
   ismStore_memGlobal.InMemGensCount = ism_common_getIntConfig(ismSTORE_CFG_INMEM_GENS_COUNT, ismSTORE_CFG_INMEM_GENS_COUNT_DV);
   ismStore_memGlobal.MgmtAlertOnPct = ism_common_getIntConfig(ismSTORE_CFG_MGMT_ALERTON_PCT, ismSTORE_CFG_MGMT_ALERTON_PCT_DV);
   ismStore_memGlobal.MgmtAlertOffPct = ism_common_getIntConfig(ismSTORE_CFG_MGMT_ALERTOFF_PCT, ismSTORE_CFG_MGMT_ALERTOFF_PCT_DV);
   ismStore_memGlobal.GenAlertOnPct = ism_common_getIntConfig(ismSTORE_CFG_GEN_ALERTON_PCT, ismSTORE_CFG_GEN_ALERTON_PCT_DV);
   ismStore_memGlobal.OwnerLimitPct = ism_common_getIntConfig(ismSTORE_CFG_OWNER_LIMIT_PCT, ismSTORE_CFG_OWNER_LIMIT_PCT_DV);
   ismStore_memGlobal.RefCtxtLocksCount = ism_common_getIntConfig(ismSTORE_CFG_REFCTXT_LOCKS_COUNT, defaultRefCtxtLocksCount);
   ismStore_memGlobal.StateCtxtLocksCount = ism_common_getIntConfig(ismSTORE_CFG_STATECTXT_LOCKS_COUNT, defaultStateCtxtLocksCount);
   ismStore_memGlobal.RefGenPoolExtElements = ism_common_getIntConfig(ismSTORE_CFG_REFGENPOOL_EXT, defaultRefGenPoolExtElements);
   ismStore_memGlobal.StoreTransRsrvOps = ism_common_getIntConfig(ismSTORE_CFG_STORETRANS_RSRV_OPS, defaultTransactionRsrvOpts);
   ismStore_memGlobal.DiskAlertOnPct = ism_common_getIntConfig(ismSTORE_CFG_DISK_ALERTON_PCT, ismSTORE_CFG_DISK_ALERTON_PCT_DV);
   ismStore_memGlobal.DiskAlertOffPct = ism_common_getIntConfig(ismSTORE_CFG_DISK_ALERTOFF_PCT, ismSTORE_CFG_DISK_ALERTOFF_PCT_DV);
   ismStore_memGlobal.DiskTransferSize = ism_common_getIntConfig(ismSTORE_CFG_DISK_BLOCK_SIZE, defaultDiskTransferBlockSize);
   ismStore_memGlobal.fEnablePersist = ism_common_getIntConfig(ismSTORE_CFG_DISK_ENABLEPERSIST, ismSTORE_CFG_DISK_ENABLEPERSIST_DV);
   ismStore_memGlobal.fReuseSHM = ism_common_getIntConfig(ismSTORE_CFG_PERSIST_REUSE_SHM, ismSTORE_CFG_PERSIST_REUSE_SHM_DV);
   ismStore_memGlobal.AsyncCBStatsMode = ism_common_getIntConfig(ismSTORE_CFG_ASYNCCB_STATSMODE, ismSTORE_CFG_ASYNCCB_STATSMODE_DV);

   if (ism_common_getBooleanConfig(ismSTORE_CFG_PERSIST_ANY_USER_IN_GROUP, ismSTORE_CFG_PERSIST_ANY_USER_IN_GROUP_DV)) 
   {
      ismStore_memGlobal.PersistedFileMode = 00664;
      ismStore_memGlobal.PersistedDirectoryMode = 0775;
   } 
   else 
   {
	   ismStore_memGlobal.PersistedFileMode = 00644;
       ismStore_memGlobal.PersistedDirectoryMode = 0755;
   }

   ismStore_memGlobal.PersistThreadPolicy = ism_common_getIntConfig(ismSTORE_CFG_PERSIST_THREAD_POLICY, ismSTORE_CFG_PERSIST_THREAD_POLICY_DV);
   ismStore_memGlobal.PersistAsyncThreads = ism_common_getIntConfig(ismSTORE_CFG_PERSIST_ASYNC_THREADS, ismSTORE_CFG_PERSIST_ASYNC_THREADS_DV);
   ismStore_memGlobal.PersistAsyncOrdered = ism_common_getIntConfig(ismSTORE_CFG_PERSIST_ASYNC_ORDERED, ismSTORE_CFG_PERSIST_ASYNC_ORDERED_DV);
   ismStore_memGlobal.PersistBuffSize = ism_common_getIntConfig(ismSTORE_CFG_PERSIST_BUFF_SIZE, defaultPersistBuffSize);
   ismStore_memGlobal.PersistHaTxThreads = ism_common_getIntConfig(ismSTORE_CFG_PERSIST_HATX_THREADS, ismSTORE_CFG_PERSIST_HATX_THREADS_DV);
   ismStore_memGlobal.PersistRecoverFromV12 = ism_common_getIntConfig(ismSTORE_CFG_RECOVER_FROM_V12, 0);
   ismStore_memGlobal.PersistCbHwm = ism_common_getIntConfig(ismSTORE_CFG_PERSIST_MAX_ASYNC_CB, ismSTORE_CFG_PERSIST_MAX_ASYNC_CB_DV);
   sizeMB = (uint64_t)ism_common_getIntConfig(ismSTORE_CFG_PERSIST_FILE_SIZE_MB, defaultPersistFileSizeMB);
   ismStore_memGlobal.PersistFileSize = sizeMB << 20; // Convert MB to bytes
   ismStore_memGlobal.StreamsMinCount = ism_common_getIntConfig("TcpThreads", 5) + 7;
   ismStore_memGlobal.RefSearchCacheSize = ism_common_getIntConfig(ismSTORE_CFG_REFSEARCHCACHESIZE, ismSTORE_CFG_REFSEARCHCACHESIZE_DV);
   ismStore_memGlobal.RefChunkHWMpct     = ism_common_getIntConfig(ismSTORE_CFG_REFCHUNKHWMPCT, 2);
   ismStore_memGlobal.RefChunkLWMpct     = ism_common_getIntConfig(ismSTORE_CFG_REFCHUNKLWMPCT, 2);
   ismStore_memGlobal.PersistRecoveryFlags = ism_common_getIntConfig(ismSTORE_CFG_PERSISTRECOVERYFLAGS, 0);

   if (((pRootPath = ism_common_getStringConfig(ismSTORE_CFG_DISK_PERSIST_PATH)) == NULL) || (pRootPath[0] == '\0'))
   {
      pRootPath = ismSTORE_CFG_DISK_PERSIST_PATH_DV;
   }
   len = su_strlcpy(ismStore_memGlobal.PersistRootPath, pRootPath, sizeof(ismStore_memGlobal.PersistRootPath)) ; 
   if ( len > 0 && len < sizeof(ismStore_memGlobal.PersistRootPath) && ismStore_memGlobal.PersistRootPath[len-1] != '/' )
   {
     ismStore_memGlobal.PersistRootPath[len++] = '/' ; 
     ismStore_memGlobal.PersistRootPath[len  ] = 0 ; 
   }

   if (ism_common_getProperty(ism_common_getConfigProperties(), ismSTORE_CFG_STREAMS_CACHE_PCT, &field) != ISMRC_OK)
   {
      field.val.d = ismSTORE_CFG_STREAMS_CACHE_PCT_DV;
   }
   ismStore_memGlobal.StreamsCachePct = field.val.d;

   if (((pRootPath = ism_common_getStringConfig(ismSTORE_CFG_DISK_ROOT_PATH)) == NULL) || (pRootPath[0] == '\0'))
   {
      pRootPath = ismSTORE_CFG_DISK_ROOT_PATH_DV;
   }

   if ((len = strlen(pRootPath)) > 0 && pRootPath[len-1] == '/') {
      snprintf(ismStore_memGlobal.DiskRootPath, PATH_MAX, "%s", pRootPath);
   } else {
      snprintf(ismStore_memGlobal.DiskRootPath, PATH_MAX, "%s/", pRootPath);
   }

   // Initialize the HA module (if HA is enabled)
   if (ismStore_global.fHAEnabled)
   {
      if ((ec = ism_store_memHAInit()) != StoreRC_OK)
      {
         rc = (ec == StoreRC_BadParameter ? ISMRC_BadPropertyValue : ISMRC_StoreHAError);
         goto exit;
      }

      pHAConfig = ism_storeHA_getConfig();
      ismStore_memGlobal.HAInfo.StartupMode = pHAConfig->StartupMode;
      ismStore_memGlobal.HAInfo.AckingPolicy = pHAConfig->AckingPolicy;
      ismStore_memGlobal.HAInfo.RoleValidation = pHAConfig->RoleValidation;
      sizeMB = (uint64_t)pHAConfig->RecoveryMemStandbyMB;
      ismStore_memGlobal.RecoveryMaxMemSizeBytes = sizeMB << 20; // Convert MB to bytes
      sizeMB = (uint64_t)pHAConfig->SyncMemSizeMB;
      ismStore_memGlobal.HAInfo.SyncMaxMemSizeBytes = sizeMB << 20; // Convert MB to bytes
   }
   else
   {
      // If HA is disabled then RoleValidation should be always true
      ismStore_memGlobal.HAInfo.RoleValidation = 1;
   }

   /* Adjust certain config parameters for virtual appliance. Some parameters are adjusted externally in the server config */
   ism_store_memAdjustVMParameters();

#if ismSTORE_SPINLOCK
   TRACE(5, "Store SpinLock is enabled\n");
#else
   TRACE(5, "Store SpinLock is disabled\n");
#endif

   TRACE(5, "Store parameter %s 0x%lx\n", ismSTORE_CFG_NVRAM_OFFSET,             ismStore_memGlobal.NVRAMOffset);
   TRACE(5, "Store parameter %s %lu\n",   ismSTORE_CFG_TOTAL_MEMSIZE_MB,         ismStore_memGlobal.TotalMemSizeBytes >> 20);
   TRACE(5, "Store parameter %s %lu\n",   ismSTORE_CFG_TOTAL_MEMSIZE_BYTES,      ismStore_memGlobal.TotalMemSizeBytes);
   TRACE(5, "Store parameter %s %.2f\n",  ismSTORE_CFG_STREAMS_CACHE_PCT,        ismStore_memGlobal.StreamsCachePct);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_MGMT_MEM_PCT,             ismStore_memGlobal.MgmtMemPct);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_MGMT_SMALL_PCT,           ismStore_memGlobal.MgmtSmallGranulesPct);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_MGMT_SMALL_GRANULE,       ismStore_memGlobal.MgmtSmallGranuleSizeBytes);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_MGMT_GRANULE_SIZE,        ismStore_memGlobal.MgmtGranuleSizeBytes);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_GRANULE_SIZE,             ismStore_memGlobal.GranuleSizeBytes);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_INMEM_GENS_COUNT,         ismStore_memGlobal.InMemGensCount);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_MGMT_ALERTON_PCT,         ismStore_memGlobal.MgmtAlertOnPct);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_MGMT_ALERTOFF_PCT,        ismStore_memGlobal.MgmtAlertOffPct);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_GEN_ALERTON_PCT,          ismStore_memGlobal.GenAlertOnPct);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_OWNER_LIMIT_PCT,          ismStore_memGlobal.OwnerLimitPct);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_REFCTXT_LOCKS_COUNT,      ismStore_memGlobal.RefCtxtLocksCount);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_STATECTXT_LOCKS_COUNT,    ismStore_memGlobal.StateCtxtLocksCount);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_REFGENPOOL_EXT,           ismStore_memGlobal.RefGenPoolExtElements);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_STORETRANS_RSRV_OPS,      ismStore_memGlobal.StoreTransRsrvOps);
   TRACE(5, "Store parameter %s %lu\n",   ismSTORE_CFG_RECOVERY_MEMSIZE_MB,      ismStore_memGlobal.RecoveryMinMemSizeBytes >> 20);
   TRACE(5, "Store parameter %s %lu\n",   ismSTORE_CFG_COMPACT_MEMTH_MB,         ismStore_memGlobal.CompactMemThBytes >> 20);
   TRACE(5, "Store parameter %s %lu\n",   ismSTORE_CFG_COMPACT_DISKTH_MB,        ismStore_memGlobal.CompactDiskThBytes >> 20);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_COMPACT_DISK_HWM,         ismStore_memGlobal.CompactDiskHWM);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_COMPACT_DISK_LWM,         ismStore_memGlobal.CompactDiskLWM);
   TRACE(5, "Store parameter %s %s\n",    ismSTORE_CFG_DISK_ROOT_PATH,           ismStore_memGlobal.DiskRootPath);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_DISK_BLOCK_SIZE,          ismStore_memGlobal.DiskTransferSize);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_DISK_ALERTON_PCT,         ismStore_memGlobal.DiskAlertOnPct);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_DISK_ALERTOFF_PCT,        ismStore_memGlobal.DiskAlertOffPct);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_DISK_ENABLEPERSIST,       ismStore_memGlobal.fEnablePersist);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_PERSIST_REUSE_SHM,        ismStore_memGlobal.fReuseSHM);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_ASYNCCB_STATSMODE,        ismStore_memGlobal.AsyncCBStatsMode);
   TRACE(5, "Store parameter %s %05o\n",  ismSTORE_CFG_PERSISTED_FILE_MODE,      ismStore_memGlobal.PersistedFileMode);
   TRACE(5, "Store parameter %s %04o\n",  ismSTORE_CFG_PERSISTED_DIRECTORY_MODE, ismStore_memGlobal.PersistedDirectoryMode);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_PERSIST_THREAD_POLICY,    ismStore_memGlobal.PersistThreadPolicy);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_PERSIST_BUFF_SIZE,        ismStore_memGlobal.PersistBuffSize);
   TRACE(5, "Store parameter %s %lu\n",   ismSTORE_CFG_PERSIST_FILE_SIZE_MB,     ismStore_memGlobal.PersistFileSize >> 20);
   TRACE(5, "Store parameter %s %s\n",    ismSTORE_CFG_DISK_PERSIST_PATH,        ismStore_memGlobal.PersistRootPath);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_RECOVER_FROM_V12,         ismStore_memGlobal.PersistRecoverFromV12);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_REFSEARCHCACHESIZE,       ismStore_memGlobal.RefSearchCacheSize);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_REFCHUNKHWMPCT,           ismStore_memGlobal.RefChunkHWMpct);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_REFCHUNKLWMPCT,           ismStore_memGlobal.RefChunkLWMpct);
   TRACE(5, "Store parameter %s %u\n",    ismSTORE_CFG_PERSISTRECOVERYFLAGS,     ismStore_memGlobal.PersistRecoveryFlags);


   if ((rc = ism_store_memValidateParameters()) != ISMRC_OK)
   {
      goto exit;
   }

   // Check if the store file is locked
   if (ismStore_global.MemType == ismSTORE_MEMTYPE_SHM || ismStore_global.MemType == ismSTORE_MEMTYPE_MEM)
   {
      sprintf(ismStore_memGlobal.LockFileName, "com.ibm.ism.%d.store.lock", (int)getuid());
   }
   else
   {
      sprintf(ismStore_memGlobal.LockFileName, "com.ibm.ism.store.lock");
   }
   rc = ism_store_checkStoreLock(ismStore_memGlobal.DiskRootPath,
                                 ismStore_memGlobal.LockFileName,
                                 &ismStore_memGlobal.LockFileDescriptor,
                                 ismStore_memGlobal.PersistedFileMode);

   if (rc != ISMRC_OK)
   {
      rc = ISMRC_StoreNotAvailable;
      ismStore_memGlobal.LockFileDescriptor = -1;
      goto exit;
   }

   // Initialize the storeDisk variables
   if (pthread_mutex_init(&ismStore_memGlobal.DiskMutex, NULL))
   {
      TRACE(1, "Failed to initialize mutex (DiskMutexu)\n");
      rc = ISMRC_Error;
      goto exit;
   }
   ismStore_memGlobal.DiskMutexInit++;

   // Initialize the granules pool variables
   for (j=0; j < ismSTORE_GRANULE_POOLS_COUNT; j++)
   {
      if (pthread_mutex_init(&ismStore_memGlobal.MgmtGen.PoolMutex[j], NULL))
      {
         TRACE(1, "Failed to initialize mutex (PoolMutex[%u])\n", j);
         rc = ISMRC_Error;
         goto exit;
      }
      ismStore_memGlobal.MgmtGen.PoolMutexInit++;
   }

   for (i=0; i < ismSTORE_MAX_INMEM_GENS; i++)
   {
     for (j=0; j < ismSTORE_GRANULE_POOLS_COUNT; j++)
     {
          if (pthread_mutex_init(&ismStore_memGlobal.InMemGens[i].PoolMutex[j], NULL))
          {
             TRACE(1, "Failed to initialize mutex (PoolMutex[%u])\n", j);
             rc = ISMRC_Error;
             goto exit;
          }
          ismStore_memGlobal.InMemGens[i].PoolMutexInit++;
      }
   }

   if (pthread_mutex_init(&ismStore_memGlobal.RsrvPoolMutex, NULL))
   {
      TRACE(1, "Failed to initialize mutex (RsrvPoolMutex)\n");
      rc = ISMRC_Error;
      goto exit;
   }
   ismStore_memGlobal.RsrvPoolMutexInit = 1;

   if (pthread_cond_init(&ismStore_memGlobal.RsrvPoolCond, NULL))
   {
      TRACE(1, "Failed to initialize cond (RsrvPoolCond)\n");
      rc = ISMRC_Error;
      goto exit;
   }
   ismStore_memGlobal.RsrvPoolMutexInit = 2;
   ismStore_memGlobal.RsrvPoolId = ismSTORE_GRANULE_POOLS_COUNT;

   if (pthread_mutex_init(&ismStore_memGlobal.StreamsMutex, NULL))
   {
      TRACE(1, "Failed to initialize mutex (StreamsMutex)\n");
      rc = ISMRC_Error;
      goto exit;
   }
   ismStore_memGlobal.StreamsMutexInit = 1;

   if (pthread_cond_init(&ismStore_memGlobal.StreamsCond, NULL))
   {
      TRACE(1, "Failed to initialize cond (StreamsCond)\n");
      rc = ISMRC_Error;
      goto exit;
   }
   ismStore_memGlobal.StreamsMutexInit = 2;


   // Initialize the streams array
   ismStore_memGlobal.StreamsCount = 0;
   ismStore_memGlobal.StreamsSize = ismSTORE_MIN_STREAMS_ARRAY_SIZE;
   ismStore_memGlobal.pStreams = (ismStore_memStream_t **)ism_common_calloc(ISM_MEM_PROBE(ism_memory_store_misc,117),ismStore_memGlobal.StreamsSize,
                                                                 sizeof(ismStore_memStream_t *));
   if (ismStore_memGlobal.pStreams == NULL)
   {
      TRACE(1, "Failed to allocate memory for the Streams array\n");
      rc = ISMRC_AllocateError;
      goto exit;
   }

   if (pthread_mutex_init(&ismStore_memGlobal.CtxtMutex, NULL))
   {
      TRACE(1, "Failed to initialize mutex (CtxtMutex)\n");
      rc = ISMRC_Error;
      goto exit;
   }
   ismStore_memGlobal.CtxtMutexInit = 1;

   // Initialize the RefCtxtMutex array
   ismStore_memGlobal.pRefCtxtMutex = (ismSTORE_MUTEX_t **)ism_common_calloc(ISM_MEM_PROBE(ism_memory_store_misc,118),ismStore_memGlobal.RefCtxtLocksCount, sizeof(ismSTORE_MUTEX_t *));
   if (ismStore_memGlobal.pRefCtxtMutex == NULL)
   {
      TRACE(1, "Failed to allocate memory for the pRefCtxtMutex array\n");
      rc = ISMRC_AllocateError;
      goto exit;
   }

   for (i=0; i < ismStore_memGlobal.RefCtxtLocksCount; i++)
   {
      if (posix_memalign((void **)&ismStore_memGlobal.pRefCtxtMutex[i], 0x40, sizeof(ismSTORE_MUTEX_t)))
      {
         TRACE(1, "Failed to allocate memory for RefCtxtMutex (Index %u)\n", i);
         rc = ISMRC_AllocateError;
         goto exit;
      }

      if (ismSTORE_MUTEX_INIT(ismStore_memGlobal.pRefCtxtMutex[i]))
      {
         TRACE(1, "Failed to initialize the RefCtxtMutex (Index %u)\n", i);
         rc = ISMRC_Error;
         goto exit;
      }
      ismStore_memGlobal.RefCtxtMutexInit = i + 1;
   }

   // Initialize the StateCtxtMutex array
   ismStore_memGlobal.pStateCtxtMutex = (ismSTORE_MUTEX_t **)ism_common_calloc(ISM_MEM_PROBE(ism_memory_store_misc,119),ismStore_memGlobal.StateCtxtLocksCount, sizeof(ismSTORE_MUTEX_t *));
   if (ismStore_memGlobal.pStateCtxtMutex == NULL)
   {
      TRACE(1, "Failed to allocate memory for the pStateCtxtMutex array\n");
      rc = ISMRC_AllocateError;
      goto exit;
   }

   for (i=0; i < ismStore_memGlobal.StateCtxtLocksCount; i++)
   {
      if (posix_memalign((void **)&ismStore_memGlobal.pStateCtxtMutex[i], 0x40, sizeof(ismSTORE_MUTEX_t)))
      {
         TRACE(1, "Failed to allocate memory for StateCtxtMutex (Index %u)\n", i);
         rc = ISMRC_AllocateError;
         goto exit;
      }

      if (ismSTORE_MUTEX_INIT(ismStore_memGlobal.pStateCtxtMutex[i]))
      {
         TRACE(1, "Failed to initialize StateCtxtMutex (Index %u)\n", i);
         rc = ISMRC_Error;
         goto exit;
      }
      ismStore_memGlobal.StateCtxtMutexInit = i + 1;
   }

   // Initialize the RefGenPool array
   ismStore_memGlobal.pRefGenPool = (ismStore_memRefGenPool_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,120),ismStore_memGlobal.RefCtxtLocksCount * sizeof(ismStore_memRefGenPool_t));
   if (ismStore_memGlobal.pRefGenPool == NULL)
   {
      TRACE(1, "Failed to allocate memory for the pRefGenPool array\n");
      rc = ISMRC_AllocateError;
      goto exit;
   }
   memset(ismStore_memGlobal.pRefGenPool, '\0', ismStore_memGlobal.RefCtxtLocksCount * sizeof(ismStore_memRefGenPool_t));
   for (i=0; i < ismStore_memGlobal.RefCtxtLocksCount; i++)
   {
      ismStore_memGlobal.pRefGenPool[i].Index = i;
   }
   ismStore_memGlobal.RefGenPoolHWM = ismStore_memGlobal.RefGenPoolExtElements * 2;
   ismStore_memGlobal.RefGenPoolLWM = ismStore_memGlobal.RefGenPoolExtElements / 2;

   // Initialize the generations array
   ismStore_memGlobal.GensMapSize = 4096;
   ismStore_memGlobal.pGensMap = (ismStore_memGenMap_t **)ism_common_calloc(ISM_MEM_PROBE(ism_memory_store_misc,121),ismStore_memGlobal.GensMapSize, sizeof(ismStore_memGenMap_t *));
   if (!ismStore_memGlobal.pGensMap)
   {
      TRACE(1, "Failed to allocate memory for the pGensMap array (GensMapSize %d)\n", ismStore_memGlobal.GensMapSize);
      ismStore_memGlobal.GensMapSize = 0;
      rc = ISMRC_AllocateError;
      goto exit;
   }
   ismStore_memGlobal.GensMapCount = 1;  // We take into account the reserved generation (ismSTORE_RSRV_GEN_ID)

   // Initialize the generation map mask array
   for (i=0, mask=1; i < 64; i++)
   {
      ismStore_GenMapSetMask[i] = mask;
      ismStore_GenMapResetMask[i] = ~mask;
      mask *= 2;
   }

   // Initialize the store thread variables
   if (pthread_mutex_init(&ismStore_memGlobal.ThreadMutex, NULL))
   {
      TRACE(1, "Failed to initialize mutex (Store.ThreadMutex)\n");
      rc = ISMRC_Error;
      goto exit;
   }
   ismStore_memGlobal.ThreadMutexInit = 1;

   if (ism_common_cond_init_monotonic(&ismStore_memGlobal.ThreadCond))
   {
      TRACE(1, "Failed to initialize cond (Store.ThreadCond)\n");
      rc = ISMRC_Error;
      goto exit;
   }
   ismStore_memGlobal.ThreadMutexInit = 2;


   memset(ismStore_T2T, 0, sizeof(ismStore_T2T));
   i = 0; 
   ismStore_T2T[ISM_STORE_RECTYPE_SERVER] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_CLIENT] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_QUEUE ] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_TOPIC ] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_SUBSC ] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_TRANS ] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_BMGR  ] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_REMSRV] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_MSG   ] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_PROP  ] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_CPROP ] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_QPROP ] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_TPROP ] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_SPROP ] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_BXR   ] = i++; 
   ismStore_T2T[ISM_STORE_RECTYPE_RPROP ] = i++; 

   // Set up the function pointers for this store implementation
   pthread_mutex_lock(&ismStore_FnMutex);
   ismStore_global.pFnStart                       = ism_store_memStart;
   ismStore_global.pFnTerm                        = ism_store_memTerm;
   ismStore_global.pFnDrop                        = ism_store_memDrop;
   ismStore_global.pFnRegisterEventCallback       = ism_store_memRegisterEventCallback;
   ismStore_global.pFnGetGenIdOfHandle            = ism_store_memGetGenIdOfHandle;
   ismStore_global.pFnGetStatistics               = ism_store_memGetStatistics;
   ismStore_global.pFnOpenStream                  = ism_store_memOpenStream;
   ismStore_global.pFnCloseStream                 = ism_store_memCloseStream;
   ismStore_global.pFnReserveStreamResources      = ism_store_memReserveStreamResources;
   ismStore_global.pFnCancelResourceReservation   = ism_store_memCancelResourceReservation;
   ismStore_global.pFnGetStreamOpsCount           = ism_store_memGetStreamOpsCount;
   ismStore_global.pFnOpenReferenceContext        = ism_store_memOpenReferenceContext;
   ismStore_global.pFnCloseReferenceContext       = ism_store_memCloseReferenceContext;
   ismStore_global.pFnClearReferenceContext       = ism_store_memClearReferenceContext;
   ismStore_global.pFnMapReferenceContextToHandle = ism_store_memMapReferenceContextToHandle;
   ismStore_global.pFnOpenStateContext            = ism_store_memOpenStateContext;
   ismStore_global.pFnCloseStateContext           = ism_store_memCloseStateContext;
   ismStore_global.pFnMapStateContextToHandle     = ism_store_memMapStateContextToHandle;
   ismStore_global.pFnEndStoreTransaction         = ism_store_memEndStoreTransaction;
   ismStore_global.pFnAssignRecordAllocation      = ism_store_memAssignRecordAllocation;
   ismStore_global.pFnFreeRecordAllocation        = ism_store_memFreeRecordAllocation;
   ismStore_global.pFnUpdateRecord                = ism_store_memUpdateRecord;
   ismStore_global.pFnAddReference                = ism_store_memAddReference;
   ismStore_global.pFnUpdateReference             = ism_store_memUpdateReference;
   ismStore_global.pFnDeleteReference             = ism_store_memDeleteReference;
   ismStore_global.pFnPruneReferences             = ism_store_memPruneReferences;
   ismStore_global.pFnAddState                    = ism_store_memAddState;
   ismStore_global.pFnDeleteState                 = ism_store_memDeleteState;
   ismStore_global.pFnGetNextGenId                = ism_store_memGetNextGenId;
   ismStore_global.pFnGetNextRecordForType        = ism_store_memGetNextRecordForType;
   ismStore_global.pFnReadRecord                  = ism_store_memReadRecord;
   ismStore_global.pFnGetNextReferenceForOwner    = ism_store_memGetNextReferenceForOwner;
   ismStore_global.pFnReadReference               = ism_store_memReadReference;
   ismStore_global.pFnGetNextStateForOwner        = ism_store_memGetNextStateForOwner;
   ismStore_global.pFnRecoveryCompleted           = ism_store_memRecoveryCompleted;
   ismStore_global.pFnDump                        = ism_store_memDump;
   ismStore_global.pFnDumpCB                      = ism_store_memDumpCB;
   ismStore_global.pFnStopCB                      = ism_store_memStopCB;
   ismStore_global.pFnStartTransaction            = ism_store_memStartTransaction;
   ismStore_global.pFnCancelTransaction           = ism_store_memCancelTransaction;
   ismStore_global.pFnGetAsyncCBStats             = ism_store_memGetAsyncCBStats;

   ismStore_memGlobal.State = ismSTORE_STATE_INIT;
   pthread_mutex_unlock(&ismStore_FnMutex);

   TRACE(5, "Store has been initialized successfully\n");

exit:
   if (rc != ISMRC_OK)
   {
      ism_store_memTerm(0);
   }

   return rc;
}

/*
 * Performs startup tasks.
 */
int32_t ism_store_memStart(void)
{
   int fd = -1, unixrc, ec, i, memsetDone=0, flags;
   const char *pShmName;
   void *pBaseAddress;
   size_t physicalMemBytes;
   ismStore_DiskParameters_t diskParams;
   ismStore_RecoveryParameters_t recoveryParams;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memJob_t job;
   int32_t rc = ISMRC_OK;

   if (ismStore_memGlobal.State != ismSTORE_STATE_INIT)
   {
      TRACE(1, "Failed to start the store, because the store is already started.\n");
      return ISMRC_StoreBusy;
   }
   ismStore_memGlobal.fWithinStart = 1 ; 

   physicalMemBytes = ismStore_global.MachineMemoryBytes; //(size_t)sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
   ismStore_memGlobal.CompactMemBytesHWM = physicalMemBytes * ismSTORE_COMPACT_MEM_HWM / 100;
   ismStore_memGlobal.CompactMemBytesLWM = physicalMemBytes * ismSTORE_COMPACT_MEM_LWM / 100;
   if (ismStore_memGlobal.fEnablePersist)
   {
     ismStore_memGlobal.CompactMemBytesHWM *= 2; // reduce diskUtil and shmPersist contention
     ismStore_memGlobal.CompactMemBytesLWM *= 2;
   }

   // Start the storeDisk component
   memset(&diskParams, '\0', sizeof(diskParams));
   strncpy(diskParams.RootPath, ismStore_memGlobal.DiskRootPath, sizeof(diskParams.RootPath));
   diskParams.TransferBlockSize = ismStore_memGlobal.DiskTransferSize;
   diskParams.ClearStoredFiles = ismStore_global.fClearStoredFiles && (ismStore_global.ColdStartMode > 0);

   TRACE(5, "Store internal parameters: DiskRootPath %s, DiskTransferBlockSize %lu, DiskClearStoredFiles %d, PhysicalMemSizeBytes %lu, CompactMemBytesHWM %lu (%u %%), CompactMemBytesLWM %lu (%u %%)\n",
         diskParams.RootPath, diskParams.TransferBlockSize, diskParams.ClearStoredFiles, physicalMemBytes,
         ismStore_memGlobal.CompactMemBytesHWM, ismSTORE_COMPACT_MEM_HWM, ismStore_memGlobal.CompactMemBytesLWM, ismSTORE_COMPACT_MEM_LWM);

   if ((ec = ism_storeDisk_init(&diskParams)) != StoreRC_OK)
   {
      TRACE(1, "Failed to initialize the StoreDisk component. error code %d\n", ec);
      rc = ISMRC_Error;
      goto exit;
   }
   ismStore_memGlobal.fDiskUp = 1;
   ismStore_memGlobal.DiskBlockSizeBytes = diskParams.DiskBlockSize;

   TRACE(5, "Store internal parameters: DiskBlockSizeBytes %lu\n",
         ismStore_memGlobal.DiskBlockSizeBytes);

   // Start the store thread
   ismStore_memGlobal.fThreadGoOn = 1;
   if (ism_common_startThread(&ismStore_memGlobal.ThreadId, ism_store_memThread,
       NULL, NULL, 0, ISM_TUSAGE_NORMAL, 0, ismSTORE_THREAD_NAME, "Store"))
   {
      TRACE(1, "Failed to create the %s thread - errno %d.\n", ismSTORE_THREAD_NAME, errno);
      rc = ISMRC_Error;
      goto exit;
   }

   if (ismStore_memGlobal.fEnablePersist )
   {
     ismStore_PersistParameters_t persistParams;
     memset(&persistParams, 0, sizeof(ismStore_PersistParameters_t));
     persistParams.TransferBlockSize = ismStore_memGlobal.DiskTransferSize;
     persistParams.PersistFileSize    = ismStore_memGlobal.PersistFileSize;
     persistParams.StreamBuffSize     = ismStore_memGlobal.PersistBuffSize;
     persistParams.PersistCbHwm       = ismStore_memGlobal.PersistCbHwm;
     persistParams.PersistThreadPolicy= ismStore_memGlobal.PersistThreadPolicy;
     persistParams.PersistAsyncThreads= ismStore_memGlobal.PersistAsyncThreads;
     persistParams.PersistHaTxThreads = ismStore_memGlobal.PersistHaTxThreads;
     persistParams.PersistAsyncOrdered= ismStore_memGlobal.PersistAsyncOrdered;
     persistParams.EnableHA           = ismStore_global.fHAEnabled ;
     persistParams.AsyncCBStatsMode   = ismStore_memGlobal.AsyncCBStatsMode;
     persistParams.RecoverFromV12     = ismStore_memGlobal.PersistRecoverFromV12;
     su_strlcpy(persistParams.RootPath, ismStore_memGlobal.PersistRootPath, sizeof(persistParams.RootPath));
     if ((ec = ism_storePersist_init(&persistParams)) != StoreRC_OK )
     {
        TRACE(1, "Failed to initialize the ShmPersist component. error code %d\n", ec);
        rc = ISMRC_Error;
        goto exit;
     }
   }

   // Fill the RefGenPools
   for (i=0; i < ismStore_memGlobal.RefCtxtLocksCount; i++)
   {
      memset(&job, '\0', sizeof(ismStore_memJob_t));
      job.JobType = StoreJob_IncRefGenPool;
      job.RefGenPool.pRefGenPool = &ismStore_memGlobal.pRefGenPool[i];
      ism_store_memAddJob(&job);
      ismStore_memGlobal.pRefGenPool[i].fPendingTask = 1;
   }

   // Initialize the Shared-Memory object
   if (ismStore_global.MemType == ismSTORE_MEMTYPE_SHM)
   {
      struct stat fst[1];
      // Set the name for the shared memory object
      if ((pShmName = ism_common_getStringConfig(ismSTORE_CFG_SHM_NAME)) == NULL)
      {
         pShmName = ismSTORE_CFG_SHM_NAME_DV;
      }
      sprintf(ismStore_memGlobal.SharedMemoryName,
              "/com.ibm.ism::%d:%s",
              (int)getuid(), pShmName);
      TRACE(5, "Store using shared memory object \"%s\".\n", ismStore_memGlobal.SharedMemoryName);

      // Try to attach to an existing Posix shared memory object.
      fd = shm_open(ismStore_memGlobal.SharedMemoryName, O_RDWR, S_IRWXU);
      if (fd == -1)
      {
         // Try to create it instead. It's initially zero bytes long.
         fd = shm_open(ismStore_memGlobal.SharedMemoryName, O_RDWR | O_CREAT | O_EXCL, S_IRWXU);
         if (fd == -1)
         {
            TRACE(1, "Failed to open shared memory object \"%s\" - errno %d.\n", ismStore_memGlobal.SharedMemoryName, errno);
            rc = ISMRC_StoreNotAvailable;
            goto exit;
         }
         TRACE(5, "Shared memory object does not exist. Preparing new store for use\n");
         fst->st_size=0;
      }
      else
      {
         if ( fstat(fd,fst) ) fst->st_size=1;
         TRACE(5, "Shared memory object exist. File size is %lu\n",fst->st_size);
      }

      if (fst->st_size != ismStore_memGlobal.TotalMemSizeBytes)
      {
         if (fst->st_size > 0 )
         {
            TRACE(1, "Shared memory object has incorrect size. Preparing new store for use\n");
         }
         // Resize the shared memory object to the desired size.
         unixrc = ftruncate(fd, ismStore_memGlobal.TotalMemSizeBytes);
         if (unixrc != 0)
         {
            TRACE(1, "Failed to resize shared memory object - errno %d.\n", errno);
            rc = ISMRC_StoreNotAvailable;
            goto exit;
         }

         // Make sure that all the shared-memory space is available
         if ((rc = ism_store_memValidateShmSpace(fd)) != ISMRC_OK)
         {
            goto exit;
         }
         memsetDone = 1 ; 
      }
      ismStore_memGlobal.NVRAMOffset = 0;
      flags = MAP_SHARED ; 
   }
   else if (ismStore_global.MemType == ismSTORE_MEMTYPE_NVRAM)
   {
      // Make sure that the NVRAM is valid
      if ((rc = ism_store_memValidateNVRAM()) != ISMRC_OK)
      {
         goto exit;
      }

      fd = open("/dev/mem", O_RDWR|O_CLOEXEC);
      if (fd == -1)
      {
         TRACE(1, "Failed to open \"/dev/mem\" - errno %d.\n", errno);
         rc = ISMRC_Error;
         goto exit;
      }
      flags = MAP_SHARED ; 
   }
   else if (ismStore_global.MemType == ismSTORE_MEMTYPE_MEM)
   {
      ismStore_memGlobal.NVRAMOffset = 0;
      flags = MAP_PRIVATE | MAP_ANONYMOUS ; 
      fd = -1 ; 
      memsetDone = 1;
   }
   else   // We should not get into this situation, but to be on the safe side ...
   {
      TRACE(1, "The store memory type (%u) is not valid.", ismStore_global.MemType);
      rc = ISMRC_Error;
      goto exit;
   }

  // Map the memory object into the address space.
  pBaseAddress = mmap(NULL, ismStore_memGlobal.TotalMemSizeBytes, PROT_READ | PROT_WRITE, flags, fd, ismStore_memGlobal.NVRAMOffset);
  if (pBaseAddress == MAP_FAILED)
  {
     TRACE(1, "Failed to map store memory - errno %d.\n", errno);
     rc = ISMRC_StoreNotAvailable;
     goto exit;
  }

   ismStore_memGlobal.pStoreBaseAddress = pBaseAddress;
   TRACE(5, "Mapped store memory at address %p.\n", pBaseAddress);
#if 0
   if ( getenv("Pause4GDB") )
   {
     printf("If you want to look around with gdb, that is the time...\nPress any key to continue...");
     getchar();
   }
#endif
   if (!ismStore_global.ColdStartMode)
   {
      if ((ismStore_memGlobal.fEnablePersist && ism_storePersist_coldStart()) ||
          (memsetDone && !ismStore_global.fRestoreFromDisk && !(ismStore_memGlobal.fEnablePersist && ism_storePersist_havePState())) )
      {
         TRACE(5,"Enforcing ColdStart: %u , %u , %u , %u , %u , %u\n", ismStore_memGlobal.fEnablePersist, ism_storePersist_coldStart(),
           memsetDone,    ismStore_global.fRestoreFromDisk,     ismStore_memGlobal.fEnablePersist,   ism_storePersist_havePState());
         ismStore_global.ColdStartMode = 2;
      }
   }

   if (ismStore_global.ColdStartMode > 0)
   {
      // We've been told to cold-start, probably because the server will not start.
      TRACE(5, "Discarding store contents and performing cold start.\n");
      if ( !memsetDone )
      {
        memset(pBaseAddress, '\0', ismStore_memGlobal.TotalMemSizeBytes);
        memsetDone = 1;
      }
      if ((rc = ism_store_memPrepare()) != ISMRC_OK)
      {
         goto exit;
      }
      if ( ismStore_memGlobal.fEnablePersist )
      {
         if ( (rc = ism_storePersist_prepareClean()) != ISMRC_OK)
         {
            goto exit;
         }
      }
      else
      {
         if ( (rc = ism_storePersist_emptyDir(ismStore_memGlobal.PersistRootPath)) != ISMRC_OK) 
         {
           TRACE(5,"ism_storePersist_emptyDir failed with rc=%d ; store_start() continues.\n",rc);
           rc = ISMRC_OK ; 
         }
      }
   }
   else
   if (ismStore_global.fRestoreFromDisk)
   {
      // If the management is stored on the disk, restore it into the store memory
      TRACE(5, "Restoring store management from the disk.\n");
      if ( !memsetDone )
      {
        memset(pBaseAddress, '\0', ismStore_memGlobal.TotalMemSizeBytes);
        memsetDone = 1;
      }
      if ((rc = ism_store_memRestoreFromDisk()) != ISMRC_OK)
      {
         goto exit;
      }
   }
   else
   if (ismStore_memGlobal.fEnablePersist)
   {
      if ( memsetDone || !ismStore_memGlobal.fReuseSHM || ism_storePersist_checkStopToken() != 0)
      {
         TRACE(5, "Recovering store from disk persist data.\n");
         if(!memsetDone)
         {
            memset(pBaseAddress, '\0', ismStore_memGlobal.TotalMemSizeBytes);
            memsetDone = 1;
         }
         if ((rc = ism_storePersist_doRecovery()) != ISMRC_OK)
         {
            if ( rc == StoreRC_prst_partialRecovery && (ismStore_memGlobal.PersistRecoveryFlags&2) )
            {
               TRACE(1,"persistRecovery failed to parse an ST and ismStore_memGlobal.PersistRecoveryFlags&2 is on, so the store continues after skipping some of the persisted STs\n");
               rc = ISMRC_OK;
            }
            else
            {
               rc = ISMRC_StoreNotAvailable;
               goto exit;
            }
         }
      }
      else
      {
         TRACE(5, "PersistStopToken is valid => using store from SHM.\n");
      }
   }

   // Make sure that the memory is valid
   if ((rc = ism_store_memValidate()) != ISMRC_OK)
   {
      goto exit;
   }
   TRACE(5, "Memory store header is valid. Will perform restart recovery\n");
   ismStore_memGlobal.fMemValid = 1;

   // Prepare the store for recovery (we assume that the engine always starts in recovery mode)
   memset(&recoveryParams, '\0', sizeof(ismStore_RecoveryParameters_t));
   recoveryParams.MinMemoryBytes = ismStore_memGlobal.RecoveryMinMemSizeBytes;
   recoveryParams.MaxMemoryBytes = ismStore_memGlobal.RecoveryMaxMemSizeBytes;
   recoveryParams.Role = ISM_HA_ROLE_STANDBY;

   if ((rc = ism_store_memRecoveryInit(&recoveryParams)) != ISMRC_OK)
   {
      TRACE(1, "Failed to initialize the store recovery. error code %d\n", rc);
      goto exit;
   }

   // Start the HA module (if HA is enabled)
   if (ismStore_global.fHAEnabled && (ec = ism_store_memHAStart()) != StoreRC_OK)
   {
      if (ec == StoreRC_BadParameter )
        rc = ISMRC_StoreHABadConfigValue;
      else
      if (ec == StoreRC_BadNIC )
        rc = ISMRC_StoreHABadNicConfig;
      else
        rc = ISMRC_StoreHAError;
      goto exit;
   }


   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
   if (ismStore_memGlobal.HAInfo.State == ismSTORE_HA_STATE_PRIMARY && pMgmtHeader->Role != ismSTORE_ROLE_PRIMARY)
   {
      if (ismStore_memGlobal.HAInfo.RoleValidation || pMgmtHeader->Role == ismSTORE_ROLE_UNSYNC)
      {
         TRACE(1, "The node has been selected to act as the Primary in the HA pair, but the previous role (%d) is not Primary\n", pMgmtHeader->Role);
         rc= ISMRC_StoreHAError;
         goto exit;
      }
      TRACE(4, "The node has been selected to act as the Primary in the HA pair, but the previous role (%d) is not Primary\n", pMgmtHeader->Role);
   }

   // From this point on, we don't want to check the store role
   ismStore_memGlobal.HAInfo.RoleValidation = 0;

   if (!ismStore_global.fHAEnabled || ismStore_memGlobal.HAInfo.State == ismSTORE_HA_STATE_PRIMARY)
   {
      recoveryParams.MaxMemoryBytes = ismStore_memGlobal.RecoveryMinMemSizeBytes;
      recoveryParams.Role = ISM_HA_ROLE_PRIMARY;
      if ((rc = ism_store_memRecoveryUpdParams(&recoveryParams)) != ISMRC_OK)
      {
         TRACE(1, "Failed to update the store recovery params. error code %d\n", rc);
         goto exit;
      }

      if ((rc = ism_store_memRecoveryPrepare(0)) != ISMRC_OK)
      {
         goto exit;
      }

      if (ismStore_global.fHAEnabled && ismStore_memGlobal.HAInfo.SyncNodesCount < ismStore_memGlobal.HAInfo.View.ActiveNodesCount)
      {
         if ((ec = ism_store_memHASyncStart()) != StoreRC_OK)
         {
            if ((rc = ismStore_memGlobal.HAInfo.SyncRC) == ISMRC_OK) { rc = ISMRC_StoreHAError; }
            TRACE(1, "Failed to start the HA synchronization procedure. error code %d\n", rc);
            goto exit;
         }

         // Wait until the Standby node is fully synchronized
         if (ism_store_memHASyncWaitView() != StoreRC_OK)
         {
            if ((rc = ismStore_memGlobal.HAInfo.SyncRC) == ISMRC_OK) { rc = ISMRC_StoreHAError; }
            TRACE(1, "Failed to synchronize the store of the Standby node. error code %d\n", rc);
            goto exit;
         }
      }

#if 0
      // Create a checkpoint here or in recoveryPrepare.
      if ( ismStore_memGlobal.fEnablePersist && (rc = ism_storePersist_createCP(ismStore_global.fHAEnabled)) != ISMRC_OK)
      {
         goto exit;
      }
   
      // Start the ShmPersist thread
      if (ismStore_memGlobal.fEnablePersist )
      {
        if ((ec = ism_storePersist_start()             ) != StoreRC_OK  )
        {
           TRACE(1, "Failed to start the ShmPersist thread. error code %d\n", ec);
           rc = ISMRC_Error;
           goto exit;
        }
      }
#endif

      TRACE(5, "Store has been started successfully. It is now ready for recovery\n");
      ismStore_memGlobal.State = ismSTORE_STATE_RECOVERY;
   }
   else
   {
      // Wait until the Standby node is fully synchronized
      if (ism_store_memHASyncWaitView() != StoreRC_OK)
      {
         if ((rc = ismStore_memGlobal.HAInfo.SyncRC) == ISMRC_OK) { rc = ISMRC_StoreHAError; }
         TRACE(1, "Failed to synchronize the store of the Standby node. error code %d\n", rc);
         ismStore_global.fSyncFailed = 1 ; 
         goto exit;
      }

      //HA_persist: create a Check Point here (CP records the fact that the node is a STANDBY no need for the isStandby flag) 
      // Create a checkpoint here or in recoveryPrepare.
      if ( ismStore_memGlobal.fEnablePersist && (rc = ism_storePersist_createCP(1)) != ISMRC_OK)
      {
         goto exit;
      }

      ismStore_memGlobal.State = ismSTORE_STATE_STANDBY;
      TRACE(5, "Store has been started successfully in a Standby mode\n");
   }

   if (!ismStore_global.fHAEnabled)
     ism_cluster_setHaStatus(ISM_CLUSTER_HA_DISABLED);
   
   ismStore_memGlobal.fActive = 1;

exit:
   ismStore_memGlobal.fWithinStart = 0 ; 
   // We can now close the file descriptor. It's no longer needed.
   if (fd != -1)
   {
      (void)close(fd);
   }

   return rc;
}

static void ism_store_freeDeadStreams(uint8_t force)
{
   ismStore_memStream_t *pStream, *pp , *ns; 
   double ct = ism_common_readTSC() + 1e1 ; 
   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
   for ( pp=NULL, pStream = ismStore_memGlobal.dStreams ; pStream ; pStream=ns )
   {
      ns = pStream->next ; 
      if ( !force &&
          (pStream->pPersist->curCB->TimeStamp > pStream->pPersist->TimeStamp || 
           pStream->pPersist->curCB->TimeStamp > ct) )
        pp = pStream ; 
      else
      {
        if ( pp )
          pp = ns ; 
        else
          ismStore_memGlobal.dStreams = ns ; 
        ismSTORE_FREE(pStream->pPersist);
        ismSTORE_FREE(pStream);
      }
   }
   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
}

static void ism_store_memAdjustVMParameters(void)
{
   uint32_t store_size, maxRsrvOps, num_iop, oval;
   size_t totMem = ismStore_global.MachineMemoryBytes;

   // make sure totMem/8 is a multiple of MB
   totMem >>= 23 ; 
   if (!totMem ) totMem = 1 ; 
   totMem <<= 23 ; 

   if ( ismStore_memGlobal.fEnablePersist && !ismStore_global.ColdStartMode )
   {
     uint64_t storeSize=0 ; 
     if ( ism_storePersist_getStoreSize(ismStore_memGlobal.PersistRootPath, &storeSize) == ISMRC_OK && storeSize > 0 )
     {
       if ( storeSize > totMem / 3 )
       {
         TRACE(3, "There is an existing store with size (%lu) which is greater than TotalMemory/3 (%lu).  The store should fail to start\n",(storeSize>>20),(totMem>>20)/3);
       }
       else
       {
         TRACE(5, "Store parameter %s adjusted to %lu (%lu) using an existing persisted store\n", ismSTORE_CFG_TOTAL_MEMSIZE_MB, (storeSize>>20), (ismStore_memGlobal.TotalMemSizeBytes >> 20));
         ismStore_memGlobal.TotalMemSizeBytes = storeSize ;  
       }
     }
   }
   if ((ismStore_memGlobal.TotalMemSizeBytes>>20) == ismSTORE_CFG_TOTAL_MEMSIZE_MB_DV )
   {
     ismStore_memGlobal.TotalMemSizeBytes = totMem / 8 ; 
     TRACE(5, "Store parameter %s adjusted to %lu (%u)\n", ismSTORE_CFG_TOTAL_MEMSIZE_MB, (ismStore_memGlobal.TotalMemSizeBytes >> 20), ismSTORE_CFG_TOTAL_MEMSIZE_MB_DV);
   }

   if ((ismStore_memGlobal.RecoveryMinMemSizeBytes>>20) == ismSTORE_CFG_RECOVERY_MEMSIZE_MB_DV )
   {
     ismStore_memGlobal.RecoveryMinMemSizeBytes = ismStore_memGlobal.TotalMemSizeBytes / 2 ; 
     TRACE(5, "Store parameter %s adjusted to %lu (%u)\n", ismSTORE_CFG_RECOVERY_MEMSIZE_MB, (ismStore_memGlobal.RecoveryMinMemSizeBytes >> 20), ismSTORE_CFG_RECOVERY_MEMSIZE_MB_DV);
   }

   if (ismStore_global.fHAEnabled)
   {
      if ((ismStore_memGlobal.RecoveryMaxMemSizeBytes>>20) == 0 )
      {
        ismStore_memGlobal.RecoveryMaxMemSizeBytes = totMem / 2 ;
        if ( ismStore_memGlobal.RecoveryMaxMemSizeBytes < ismStore_memGlobal.RecoveryMinMemSizeBytes )
             ismStore_memGlobal.RecoveryMaxMemSizeBytes = ismStore_memGlobal.RecoveryMinMemSizeBytes ;
        TRACE(5, "Store parameter %s adjusted to %lu (%u)\n", ismHA_CFG_RECOVERYMEMSTANDBYMB, (ismStore_memGlobal.RecoveryMaxMemSizeBytes >> 20), 0);
      }

      if ((ismStore_memGlobal.HAInfo.SyncMaxMemSizeBytes>>20) == 0 )
      {
        ismStore_memGlobal.HAInfo.SyncMaxMemSizeBytes = ismStore_memGlobal.TotalMemSizeBytes / 2 ;
        TRACE(5, "Store parameter %s adjusted to %lu (%u)\n", ismHA_CFG_SYNCMEMSIZEMB, (ismStore_memGlobal.HAInfo.SyncMaxMemSizeBytes >> 20), 0);
      }
   } 

   if ( ismStore_memGlobal.fEnablePersist )
   {
     if ((ismStore_memGlobal.PersistFileSize>>20) == ismSTORE_CFG_PERSIST_FILE_SIZE_MB_DV )
     {
       struct statfs sfs[1] ; 
       if ( statfs(ismStore_memGlobal.PersistRootPath, sfs) )
       {
         TRACE(3,"statfs() failed for |%s|, errno is %d (%s)\n",ismStore_memGlobal.PersistRootPath,errno,strerror(errno));
         ismStore_memGlobal.PersistFileSize = ismStore_memGlobal.TotalMemSizeBytes ; 
       }
       else
         ismStore_memGlobal.PersistFileSize = sfs->f_blocks * sfs->f_bsize / 24 ; 
       if ( ismStore_memGlobal.PersistFileSize < (1ULL<<29) )   // 512MB
            ismStore_memGlobal.PersistFileSize = (1ULL<<29) ; 
       else
       if ( ismStore_memGlobal.PersistFileSize > (1ULL<<34) )  // 16GB
            ismStore_memGlobal.PersistFileSize = (1ULL<<34) ;
       TRACE(5, "Store parameter %s adjusted to %lu (%u)\n", ismSTORE_CFG_PERSIST_FILE_SIZE_MB, (ismStore_memGlobal.PersistFileSize >> 20), ismSTORE_CFG_PERSIST_FILE_SIZE_MB_DV);
     }
   }

   num_iop = ism_common_getIntConfig("TcpThreads", 5);

   store_size = ismStore_memGlobal.TotalMemSizeBytes >> 20;
   if (store_size < 16000 && store_size > 0) 
   {
      double ratio = 16384e0 / store_size;  // maintain HW appliance (mem 16GB) ratio 
      /* Memory compaction threshold */
      if ((ismStore_memGlobal.CompactMemThBytes >> 20) == ismSTORE_CFG_COMPACT_MEMTH_MB_DV)  // adjust only if set to default to allow changes via config
      {
         ismStore_memGlobal.CompactMemThBytes = (uint64_t)(ismStore_memGlobal.CompactMemThBytes / ratio);
         if ((ismStore_memGlobal.CompactMemThBytes >> 20) < ismSTORE_MIN_COMPACT_TH_MB)
         {
            ismStore_memGlobal.CompactMemThBytes = (uint64_t)(ismSTORE_MIN_COMPACT_TH_MB << 20);
         }
         TRACE(5, "Store parameter %s adjusted to %lu (%u)\n", ismSTORE_CFG_COMPACT_MEMTH_MB, (ismStore_memGlobal.CompactMemThBytes >> 20), ismSTORE_CFG_COMPACT_MEMTH_MB_DV);
      }

      /* Disk compaction threshold */
      if ((ismStore_memGlobal.CompactDiskThBytes >> 20) == ismSTORE_CFG_COMPACT_DISKTH_MB_DV)  
      {
         ismStore_memGlobal.CompactDiskThBytes = (uint64_t)(ismStore_memGlobal.CompactDiskThBytes / ratio);
         if ((ismStore_memGlobal.CompactDiskThBytes >> 20) < ismSTORE_MIN_COMPACT_TH_MB)
         {
            ismStore_memGlobal.CompactDiskThBytes = (uint64_t)(ismSTORE_MIN_COMPACT_TH_MB << 20);
         }
         TRACE(5, "Store parameter %s adjusted to %lu (%u)\n", ismSTORE_CFG_COMPACT_DISKTH_MB, (ismStore_memGlobal.CompactDiskThBytes >> 20), ismSTORE_CFG_COMPACT_DISKTH_MB_DV);
      }
      /* TransactionRsrvOps - allocate 5% of the large granule pool to ST ops */
      oval = ismStore_memGlobal.StoreTransRsrvOps;
      maxRsrvOps = (uint32_t)(ismStore_memGlobal.TotalMemSizeBytes*5*ismSTORE_MGMT_POOL_PCT*ismStore_memGlobal.MgmtMemPct/1e6/sizeof(ismStore_memStoreOperation_t)/(num_iop+10));
      if (ismStore_memGlobal.StoreTransRsrvOps == ismSTORE_CFG_STORETRANS_RSRV_OPS_DV)
      {  
        ismStore_memGlobal.StoreTransRsrvOps = maxRsrvOps > 100000 ? 100000 : maxRsrvOps;
      }
      if (ismStore_memGlobal.StoreTransRsrvOps < 500)
      {
         ismStore_memGlobal.StoreTransRsrvOps = 500;
      }
      if (ismStore_memGlobal.StoreTransRsrvOps != oval)
      {
         TRACE(5, "Store parameter %s adjusted to %u (%u)\n", ismSTORE_CFG_STORETRANS_RSRV_OPS, ismStore_memGlobal.StoreTransRsrvOps, oval);
      }
   }

   /* Account for disk space consumed by persistence */
   if (ismStore_memGlobal.fEnablePersist)
   {
      /* Set disk compaction high/low WM to 80/70 (default is 70/60) */
      if (ismStore_memGlobal.CompactDiskHWM == ismSTORE_CFG_COMPACT_DISK_HWM_DV)
      {
         ismStore_memGlobal.CompactDiskHWM = 80;
         TRACE(5, "Store parameter %s adjusted to %u (%u)\n", ismSTORE_CFG_COMPACT_DISK_HWM, ismStore_memGlobal.CompactDiskHWM, ismSTORE_CFG_COMPACT_DISK_HWM_DV);
      }
      if (ismStore_memGlobal.CompactDiskLWM == ismSTORE_CFG_COMPACT_DISK_LWM_DV)
      {
         ismStore_memGlobal.CompactDiskLWM = 70;
         TRACE(5, "Store parameter %s adjusted to %u (%u)\n", ismSTORE_CFG_COMPACT_DISK_LWM, ismStore_memGlobal.CompactDiskLWM, ismSTORE_CFG_COMPACT_DISK_LWM_DV);
      }
   }
   /* clflush needed only for NVRAM */
   if (ismStore_global.MemType != ismSTORE_MEMTYPE_NVRAM && ismStore_global.CacheFlushMode == ismSTORE_CACHEFLUSH_ADR)
   {
      ismStore_global.CacheFlushMode = ismSTORE_CACHEFLUSH_NORMAL;
      TRACE(5, "Store parameter %s adjusted to %u (%u)\n", ismSTORE_CFG_CACHEFLUSH_MODE, ismStore_global.CacheFlushMode, ismSTORE_CACHEFLUSH_ADR);
   }
   /* Increase number of Ref/State context locks if needed */
   if (ismStore_memGlobal.RefCtxtLocksCount < num_iop * 20)
   {
      oval = ismStore_memGlobal.RefCtxtLocksCount; 
      ismStore_memGlobal.RefCtxtLocksCount = num_iop * 20;
      TRACE(5, "Store parameter %s adjusted to %u (%u), num_iop %d\n", ismSTORE_CFG_REFCTXT_LOCKS_COUNT, ismStore_memGlobal.RefCtxtLocksCount, oval, num_iop);
   }
   if (ismStore_memGlobal.StateCtxtLocksCount < num_iop * 20)
   {
      oval = ismStore_memGlobal.StateCtxtLocksCount; 
      ismStore_memGlobal.StateCtxtLocksCount = num_iop * 20;
      TRACE(5, "Store parameter %s adjusted to %u (%u), num_iop %d\n", ismSTORE_CFG_STATECTXT_LOCKS_COUNT, ismStore_memGlobal.StateCtxtLocksCount, oval, num_iop);
   }
   if (ismStore_memGlobal.PersistHaTxThreads > 64)
   {
      oval = ismStore_memGlobal.PersistHaTxThreads; 
      ismStore_memGlobal.PersistHaTxThreads = 64;
      TRACE(5, "Store parameter %s adjusted to %u (%u)\n", ismSTORE_CFG_PERSIST_HATX_THREADS, ismStore_memGlobal.PersistHaTxThreads, oval);
   }
   if (ismStore_memGlobal.PersistAsyncThreads > 64)
   {
      oval = ismStore_memGlobal.PersistAsyncThreads; 
      ismStore_memGlobal.PersistAsyncThreads = 64;
      TRACE(5, "Store parameter %s adjusted to %u (%u)\n", ismSTORE_CFG_PERSIST_ASYNC_THREADS, ismStore_memGlobal.PersistAsyncThreads, oval);
   } 
}

/* 
Verify sufficient disk space for store (at least 4 times the memory size.
Must be in a separate function and not static so it can be override by engine unit tests.
*/
int32_t ism_store_memValidateDiskSpace(void)
{
  int32_t rc = ISMRC_OK;
  struct statfs sfs[1] ; 
  if ( statfs(ismStore_memGlobal.DiskRootPath, sfs) )
  {
    TRACE(1, "Warning: statfs failed for the %s parameter (%s)\n",ismSTORE_CFG_DISK_ROOT_PATH,ismStore_memGlobal.DiskRootPath) ; 
  }
  else
  {
    size_t fss = sfs->f_blocks * sfs->f_bsize ; 
    size_t totMem = ismStore_global.MachineMemoryBytes;
     
    if ( fss < 4 * totMem )
    {
       TRACE(1, "Store parameter %s (filesystem size  %lu GB) is not valid. It must be greater than %lu GB\n",
             ismSTORE_CFG_DISK_ROOT_PATH, (fss>>30), (totMem>>28));
       rc = ISMRC_VMDiskIsSmall;
    }
  }
  return rc;
}

static int32_t ism_store_memValidateParameters(void)
{
   uint32_t genMemSizeMB;
   uint64_t o;
   int32_t rc = ISMRC_OK, mrc;


   if (ismStore_memGlobal.MgmtSmallGranuleSizeBytes < ismSTORE_MIN_GRANULE_SIZE || ismStore_memGlobal.MgmtSmallGranuleSizeBytes > ismSTORE_MAX_GRANULE_SIZE)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [%u, %u]\n",
            ismSTORE_CFG_MGMT_SMALL_GRANULE, ismStore_memGlobal.MgmtSmallGranuleSizeBytes, ismSTORE_MIN_GRANULE_SIZE, ismSTORE_MAX_GRANULE_SIZE);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_MGMT_SMALL_GRANULE, ismStore_memGlobal.MgmtSmallGranuleSizeBytes);
   }

   if (ismStore_memGlobal.MgmtGranuleSizeBytes < ismSTORE_MIN_GRANULE_SIZE || ismStore_memGlobal.MgmtGranuleSizeBytes > ismSTORE_MAX_GRANULE_SIZE)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [%u, %u]\n",
           ismSTORE_CFG_MGMT_GRANULE_SIZE, ismStore_memGlobal.MgmtGranuleSizeBytes, ismSTORE_MIN_GRANULE_SIZE, ismSTORE_MAX_GRANULE_SIZE);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_MGMT_GRANULE_SIZE, ismStore_memGlobal.MgmtGranuleSizeBytes);
   }

   if (ismStore_memGlobal.GranuleSizeBytes < ismSTORE_MIN_GRANULE_SIZE || ismStore_memGlobal.GranuleSizeBytes > ismSTORE_MAX_GRANULE_SIZE)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [%u, %u]\n",
            ismSTORE_CFG_GRANULE_SIZE, ismStore_memGlobal.GranuleSizeBytes, ismSTORE_MIN_GRANULE_SIZE, ismSTORE_MAX_GRANULE_SIZE);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_GRANULE_SIZE, ismStore_memGlobal.GranuleSizeBytes);
   }

   if ((ismStore_memGlobal.TotalMemSizeBytes >> 20) < ismSTORE_MIN_MEMSIZE_MB)
   {
      TRACE(1, "Store parameter %s (%lu MB) is not valid. Minimal value %u MB\n",
            ismSTORE_CFG_TOTAL_MEMSIZE_MB, (ismStore_memGlobal.TotalMemSizeBytes >> 20), ismSTORE_MIN_MEMSIZE_MB);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%lu", ismSTORE_CFG_TOTAL_MEMSIZE_MB, (ismStore_memGlobal.TotalMemSizeBytes >> 20));
   }

   if (ismStore_memGlobal.MgmtMemPct < 5 || ismStore_memGlobal.MgmtMemPct > 95)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [5, 95]\n",
            ismSTORE_CFG_MGMT_MEM_PCT, ismStore_memGlobal.MgmtMemPct);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_MGMT_MEM_PCT, ismStore_memGlobal.MgmtMemPct);
   }

   if (ismStore_memGlobal.MgmtSmallGranulesPct < 5 || ismStore_memGlobal.MgmtSmallGranulesPct > 95)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [5, 95]\n",
            ismSTORE_CFG_MGMT_SMALL_PCT, ismStore_memGlobal.MgmtSmallGranulesPct);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_MGMT_SMALL_PCT, ismStore_memGlobal.MgmtSmallGranulesPct);
   }

   if (ismStore_memGlobal.InMemGensCount < ismSTORE_MIN_INMEM_GENS || ismStore_memGlobal.InMemGensCount > ismSTORE_MAX_INMEM_GENS)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [%u, %u]\n",
            ismSTORE_CFG_INMEM_GENS_COUNT, ismStore_memGlobal.InMemGensCount, ismSTORE_MIN_INMEM_GENS, ismSTORE_MAX_INMEM_GENS);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_INMEM_GENS_COUNT, ismStore_memGlobal.InMemGensCount);
   }

   if (ismStore_memGlobal.InMemGensCount > 0)
   {
      genMemSizeMB = (((100 - ismStore_memGlobal.MgmtMemPct) * (ismStore_memGlobal.TotalMemSizeBytes >> 20) / 100) / ismStore_memGlobal.InMemGensCount);
      if (genMemSizeMB < ismSTORE_MIN_GEN_MEMSIZE_MB)
      {
         TRACE(1, "Store generation memory size (%u MB) is not valid. It must be greater than %u MB\n",
               genMemSizeMB, ismSTORE_MIN_GEN_MEMSIZE_MB);
         rc = ISMRC_BadPropertyValue;
         ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_INMEM_GENS_COUNT, ismStore_memGlobal.InMemGensCount);
      }
   }

   if (ismStore_memGlobal.StreamsCachePct > 25)
   {
      TRACE(1, "Store parameter %s (%lf) is not valid. Valid range: [0 25]\n",
            ismSTORE_CFG_STREAMS_CACHE_PCT, ismStore_memGlobal.StreamsCachePct);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%lf", ismSTORE_CFG_STREAMS_CACHE_PCT, ismStore_memGlobal.StreamsCachePct);
   }

   if (ismStore_memGlobal.MgmtAlertOnPct < 50 || ismStore_memGlobal.MgmtAlertOnPct > 100)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [50 100]\n",
            ismSTORE_CFG_MGMT_ALERTON_PCT, ismStore_memGlobal.MgmtAlertOnPct);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_MGMT_ALERTON_PCT, ismStore_memGlobal.MgmtAlertOnPct);
   }

   if (ismStore_memGlobal.MgmtAlertOffPct < 50 || ismStore_memGlobal.MgmtAlertOffPct > 100)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [50 100]\n",
            ismSTORE_CFG_MGMT_ALERTOFF_PCT, ismStore_memGlobal.MgmtAlertOffPct);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_MGMT_ALERTOFF_PCT, ismStore_memGlobal.MgmtAlertOffPct);
   }

   if (ismStore_memGlobal.MgmtAlertOffPct >= ismStore_memGlobal.MgmtAlertOnPct)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. It must be less than parameter %s (%u).\n",
            ismSTORE_CFG_MGMT_ALERTOFF_PCT, ismStore_memGlobal.MgmtAlertOffPct,
            ismSTORE_CFG_MGMT_ALERTON_PCT, ismStore_memGlobal.MgmtAlertOnPct);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_MGMT_ALERTOFF_PCT, ismStore_memGlobal.MgmtAlertOffPct);
   }

   if (ismStore_memGlobal.GenAlertOnPct < 50 || ismStore_memGlobal.GenAlertOnPct > 100)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [50 100]\n",
            ismSTORE_CFG_GEN_ALERTON_PCT, ismStore_memGlobal.GenAlertOnPct);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_GEN_ALERTON_PCT, ismStore_memGlobal.GenAlertOnPct);
   }

   if (ismStore_memGlobal.OwnerLimitPct < 5 || ismStore_memGlobal.OwnerLimitPct > 100)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [5, 100]\n",
            ismSTORE_CFG_OWNER_LIMIT_PCT, ismStore_memGlobal.OwnerLimitPct);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_OWNER_LIMIT_PCT, ismStore_memGlobal.OwnerLimitPct);
   }

   if (ismStore_memGlobal.DiskAlertOnPct > 100)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [0 100]\n",
            ismSTORE_CFG_DISK_ALERTON_PCT, ismStore_memGlobal.DiskAlertOnPct);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_DISK_ALERTON_PCT, ismStore_memGlobal.DiskAlertOnPct);
   }

   if (ismStore_memGlobal.DiskAlertOffPct > 100)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [0 100]\n",
            ismSTORE_CFG_DISK_ALERTOFF_PCT, ismStore_memGlobal.DiskAlertOffPct);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_DISK_ALERTOFF_PCT, ismStore_memGlobal.DiskAlertOffPct);
   }

   if (ismStore_memGlobal.DiskAlertOffPct >= ismStore_memGlobal.DiskAlertOnPct && ismStore_memGlobal.DiskAlertOnPct > 0)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. It must be less than parameter %s (%u).\n",
            ismSTORE_CFG_DISK_ALERTOFF_PCT, ismStore_memGlobal.DiskAlertOffPct,
            ismSTORE_CFG_DISK_ALERTON_PCT, ismStore_memGlobal.DiskAlertOnPct);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_DISK_ALERTOFF_PCT, ismStore_memGlobal.DiskAlertOffPct);
   }

   if (ismStore_memGlobal.RefCtxtLocksCount < ismSTORE_MIN_REFCTXT_LOCKS_COUNT)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. It must be greater than or equal to %u.\n",
            ismSTORE_CFG_REFCTXT_LOCKS_COUNT, ismStore_memGlobal.RefCtxtLocksCount, ismSTORE_MIN_REFCTXT_LOCKS_COUNT);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_REFCTXT_LOCKS_COUNT, ismStore_memGlobal.RefCtxtLocksCount);
   }

   if (ismStore_memGlobal.StateCtxtLocksCount < ismSTORE_MIN_STATECTXT_LOCKS_COUNT)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. It must be greater than or equal to %u.\n",
            ismSTORE_CFG_STATECTXT_LOCKS_COUNT, ismStore_memGlobal.StateCtxtLocksCount, ismSTORE_MIN_STATECTXT_LOCKS_COUNT);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_STATECTXT_LOCKS_COUNT, ismStore_memGlobal.StateCtxtLocksCount);
   }

   if (ismStore_memGlobal.RefGenPoolExtElements < ismSTORE_MIN_REFGENPOOL_EXT)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. It must be greater than or equal to %u.\n",
            ismSTORE_CFG_REFGENPOOL_EXT, ismStore_memGlobal.RefGenPoolExtElements, ismSTORE_MIN_REFGENPOOL_EXT);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_REFGENPOOL_EXT, ismStore_memGlobal.RefGenPoolExtElements);
   }

   /* allow up to 50% of pool to be reserved for ST ops */
   if (ismStore_memGlobal.MgmtGranuleSizeBytes > 0)
   {
     int streams = 12;
      uint32_t maxRsrvOps = (uint32_t)(ismStore_memGlobal.TotalMemSizeBytes * 50 * ismSTORE_MGMT_POOL_PCT *
                                       ismStore_memGlobal.MgmtMemPct / 1e6 / sizeof(ismStore_memStoreOperation_t)) / streams;
      if (ismStore_memGlobal.StoreTransRsrvOps > maxRsrvOps)
      {
         TRACE(1, "Store parameter %s (%u) is not valid. It must be less than or equal to %u.\n",
               ismSTORE_CFG_STORETRANS_RSRV_OPS, ismStore_memGlobal.StoreTransRsrvOps, maxRsrvOps);
         rc = ISMRC_BadPropertyValue;
         ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_STORETRANS_RSRV_OPS, ismStore_memGlobal.StoreTransRsrvOps);
      }
   }

   if ((ismStore_memGlobal.CompactMemThBytes >> 20) < ismSTORE_MIN_COMPACT_TH_MB)
   {
      TRACE(1, "Store parameter %s (%lu) is not valid. It must be greater than or equal to %u.\n",
            ismSTORE_CFG_COMPACT_MEMTH_MB, (ismStore_memGlobal.CompactMemThBytes >> 20), ismSTORE_MIN_COMPACT_TH_MB);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%lu", ismSTORE_CFG_COMPACT_MEMTH_MB, (ismStore_memGlobal.CompactMemThBytes >> 20));
   }

   if ((ismStore_memGlobal.CompactDiskThBytes >> 20) < ismSTORE_MIN_COMPACT_TH_MB)
   {
      TRACE(1, "Store parameter %s (%lu) is not valid. It must be greater than or equal to %u.\n",
            ismSTORE_CFG_COMPACT_DISKTH_MB, (ismStore_memGlobal.CompactDiskThBytes >> 20), ismSTORE_MIN_COMPACT_TH_MB);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%lu", ismSTORE_CFG_COMPACT_DISKTH_MB, (ismStore_memGlobal.CompactDiskThBytes >> 20));
   }

   if (ismStore_memGlobal.CompactDiskHWM > 100)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [0 100]\n",
            ismSTORE_CFG_COMPACT_DISK_HWM, ismStore_memGlobal.CompactDiskHWM);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_COMPACT_DISK_HWM, ismStore_memGlobal.CompactDiskHWM);
   }

   if (ismStore_memGlobal.CompactDiskLWM > 100)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [0 100]\n",
            ismSTORE_CFG_COMPACT_DISK_LWM, ismStore_memGlobal.CompactDiskLWM);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_COMPACT_DISK_LWM, ismStore_memGlobal.CompactDiskLWM);
   }

   if (ismStore_memGlobal.CompactDiskLWM >= ismStore_memGlobal.CompactDiskHWM)
   {
      TRACE(1, "Store parameter %s (%u) is not valid. It must be less than parameter %s (%u).\n",
            ismSTORE_CFG_COMPACT_DISK_LWM, ismStore_memGlobal.CompactDiskLWM,
            ismSTORE_CFG_COMPACT_DISK_HWM, ismStore_memGlobal.CompactDiskHWM);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_COMPACT_DISK_LWM, ismStore_memGlobal.CompactDiskLWM);
   }

   if (ismStore_memGlobal.fEnablePersist)
   {
   if (ismStore_memGlobal.PersistBuffSize < (1<<12) || ismStore_memGlobal.PersistBuffSize > (1<<26) ) 
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [4K 64M]\n",
            ismSTORE_CFG_PERSIST_BUFF_SIZE, ismStore_memGlobal.PersistBuffSize);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_PERSIST_BUFF_SIZE, ismStore_memGlobal.PersistBuffSize);
   }

   if (ismStore_memGlobal.PersistThreadPolicy > 2 )
   {
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [0 2]\n",
            ismSTORE_CFG_PERSIST_THREAD_POLICY, ismStore_memGlobal.PersistThreadPolicy);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_PERSIST_THREAD_POLICY, ismStore_memGlobal.PersistThreadPolicy);
   }

   o = 1;
   if (ismStore_memGlobal.PersistFileSize < (o<<28) || ismStore_memGlobal.PersistFileSize > (o<<36) ) 
   {
      uint32_t v =  ismStore_memGlobal.PersistFileSize >> 20 ; 
      TRACE(1, "Store parameter %s (%u) is not valid. Valid range: [256 64K]\n",
            ismSTORE_CFG_PERSIST_FILE_SIZE_MB, v);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_PERSIST_FILE_SIZE_MB, v);
   }

   o = ismStore_memGlobal.PersistBuffSize * 128;
   if ( o > ismStore_memGlobal.PersistFileSize )
   {
      uint32_t v =  ismStore_memGlobal.PersistFileSize >> 20 ; 
      TRACE(1, "Store parameter %s (%u) is not valid. The file size in bytes must be at least %u times the parameter %s (%u).\n",
            ismSTORE_CFG_PERSIST_FILE_SIZE_MB, v,128,
            ismSTORE_CFG_PERSIST_BUFF_SIZE, ismStore_memGlobal.PersistBuffSize);
      rc = ISMRC_BadPropertyValue;
      ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_PERSIST_FILE_SIZE_MB, v);
   }
   }

   if (ismStore_global.MemType == ismSTORE_MEMTYPE_MEM && !ismStore_memGlobal.fEnablePersist)
   {
      TRACE(1, "Warning the Store is configured with ismSTORE_MEMTYPE_MEM but disk persistence is not enabled!!!\n");
      //TODO: should this cause a failure?
      //rc = ISMRC_BadPropertyValue;
      //ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_MEMTYPE, ismStore_global.MemType);
   }

   if (ismStore_global.fHAEnabled)
   {
      if (ismStore_memGlobal.HAInfo.SyncMaxMemSizeBytes < (ismStore_memGlobal.TotalMemSizeBytes / 2))
      {
         TRACE(1, "Store parameter %s (%lu MB) is not valid. It must be greater than %lu MB\n",
               ismHA_CFG_SYNCMEMSIZEMB, (ismStore_memGlobal.HAInfo.SyncMaxMemSizeBytes>>20), (ismStore_memGlobal.TotalMemSizeBytes >> 21));
         rc = ISMRC_BadPropertyValue;
         ism_common_setErrorData(rc, "%s%u", ismHA_CFG_SYNCMEMSIZEMB, (uint32_t)(ismStore_memGlobal.HAInfo.SyncMaxMemSizeBytes>>20));
      }
   }

   if ((mrc = ism_store_memValidateDiskSpace()) != ISMRC_OK)
   {
     //rc = mrc;  //TODO: enforce once engine changes are comploeted
   }

   return rc;
}


/*
 * Makes sure that all the shared-memory space is available.
 * This method is trying to write dummy data to the whole shared-memory segment.
 */
static int32_t ism_store_memValidateShmSpace(int fd)
{
   char *pBuff;
   size_t offset, blockSizeBytes, bytesWritten;
   int32_t rc = ISMRC_OK;

   blockSizeBytes = fpathconf(fd, _PC_REC_XFER_ALIGN);
   if (posix_memalign((void **)&pBuff, blockSizeBytes, blockSizeBytes))
   {
      TRACE(1, "Failed to allocate memory for validating the shared-memory available space. BlockSizeBytes %lu, TotalMmemSizeBytes %lu.\n",
            blockSizeBytes, ismStore_memGlobal.TotalMemSizeBytes);
      rc = ISMRC_AllocateError;
      return rc;
   }
   memset(pBuff, '\0', blockSizeBytes);

   for (offset=0; offset < ismStore_memGlobal.TotalMemSizeBytes; offset += blockSizeBytes)
   {
      if ((bytesWritten = write(fd, pBuff, blockSizeBytes)) != blockSizeBytes)
      {
         TRACE(1, "Failed to initialize the store, because there is not enough available memory for the shared-memory object. TotalMmemSizeBytes %lu, BlockSizeBytes %lu, Offset %lu, errno %d.\n",
               ismStore_memGlobal.TotalMemSizeBytes, blockSizeBytes, offset, errno);
         rc = ISMRC_StoreNotAvailable;
         break;
      }
   }

   lseek(fd, SEEK_SET, 0);
   ismSTORE_FREE_BASIC(pBuff);

   return rc;
}

/*
 * Makes sure that the NVRAM is valid.
 */
static int32_t ism_store_memValidateNVRAM(void)
{
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);

   if (access("/usr/bin/pcitool", X_OK) < 0)
   {
      TRACE(3, "Could not validate the NVRAM because the pcitool is not accessible\n");
   }
   else if (system("date >> /ima/logs/pcitool.log && /usr/bin/pcitool nvcheck >> /ima/logs/pcitool.log 2>&1") != 0)
   {
      TRACE(1, "The NVRAM validation failed\n");
      rc = ISMRC_NVDIMMIntegrityError;
   }
   else
   {
      TRACE(5, "The NVRAM is valid\n");
   }

   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

int ism_store_memGetGenSizes(uint64_t *mgmSizeBytes, uint64_t *genSizeBytes)
{
   *mgmSizeBytes = ismSTORE_ROUND(ismStore_memGlobal.MgmtMemPct * ismStore_memGlobal.TotalMemSizeBytes / 100, ismStore_memGlobal.DiskBlockSizeBytes);
   *genSizeBytes = ismSTORE_ROUND((ismStore_memGlobal.TotalMemSizeBytes - *mgmSizeBytes) / ismStore_memGlobal.InMemGensCount, ismStore_memGlobal.GranuleSizeBytes);
   return ISMRC_OK;
}

/*
 * Prepares the memory for use. The shared memory is expected to be initialized
 * to zeroes up to StoreMemSizeBytes. This function sets up the management
 * and generations headers for the store.
 */
static int32_t ism_store_memPrepare(void)
{
   int i;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memGranulePool_t *pPool;
   ismStore_memGeneration_t *pGen;
   ismStore_GenId_t genId = ismSTORE_MGMT_GEN_ID;
   uint64_t memSizeBytes, headerSizeBytes, poolMemSizeBytes[ismSTORE_GRANULE_POOLS_COUNT];
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s\n", __FUNCTION__);
   // The start of the store is marked by a management header which describes
   // the rest of the store.
   //
   // NOTE: All the memory regions in the store are aligned by the GranuleSizeBytes.
   pGen = ism_store_memInitMgmtGenStruct();
   pMgmtHeader = (ismStore_memMgmtHeader_t *)pGen->pBaseAddress;
   pMgmtHeader->StrucId = ismSTORE_MEM_MGMTHEADER_STRUCID;
   pMgmtHeader->GenId = genId;
   pMgmtHeader->Version = ismSTORE_VERSION;
   pMgmtHeader->Role = ismSTORE_ROLE_UNSYNC;    // Mark the store role as Unsync
   pMgmtHeader->DescriptorStructSize = sizeof(ismStore_memDescriptor_t);
   pMgmtHeader->SplitItemStructSize = sizeof(ismStore_memSplitItem_t);
   memset((void *)&pMgmtHeader->GenToken, '\0', sizeof(ismStore_memGenToken_t));
   pMgmtHeader->MemSizeBytes = ismSTORE_ROUND(ismStore_memGlobal.MgmtMemPct * ismStore_memGlobal.TotalMemSizeBytes / 100, ismStore_memGlobal.DiskBlockSizeBytes);
   pMgmtHeader->TotalMemSizeBytes = ismStore_memGlobal.TotalMemSizeBytes;
   pMgmtHeader->InMemGensCount = ismStore_memGlobal.InMemGensCount;
   pMgmtHeader->PoolsCount = ismSTORE_GRANULE_POOLS_COUNT;
   pMgmtHeader->State = ismSTORE_GEN_STATE_ACTIVE;
   headerSizeBytes = ismSTORE_ROUNDUP(sizeof(ismStore_memMgmtHeader_t), ismStore_memGlobal.MgmtSmallGranuleSizeBytes);
   memSizeBytes = pMgmtHeader->MemSizeBytes - headerSizeBytes;

   poolMemSizeBytes[ismSTORE_MGMT_POOL_INDEX] = ismSTORE_ROUND(memSizeBytes * (100 - ismStore_memGlobal.MgmtSmallGranulesPct) / 100, ismStore_memGlobal.MgmtGranuleSizeBytes);
   poolMemSizeBytes[ismSTORE_MGMT_SMALL_POOL_INDEX] = ismSTORE_ROUND(memSizeBytes - poolMemSizeBytes[ismSTORE_MGMT_POOL_INDEX], ismStore_memGlobal.MgmtSmallGranuleSizeBytes);
   pMgmtHeader->RsrvPoolMemSizeBytes = 0;
   ismStore_memGlobal.RsrvPoolId = ismSTORE_MGMT_SMALL_POOL_INDEX;
   ismStore_memGlobal.RsrvPoolState = 4;  /* RsrvPool is attached */
   ADR_WRITE_BACK(pMgmtHeader, sizeof(*pMgmtHeader));

   TRACE(5, "Store management parameters: MemSizeBytes %lu, RsrvPoolMemSizeBytes %lu, PoolsCount %u, InMemGensCount %u, MaxTranOpsPerGranule %u, MaxStatesPerGranule %u, MaxRefStatesPerGranule %u.\n",
            pMgmtHeader->MemSizeBytes, pMgmtHeader->RsrvPoolMemSizeBytes, pMgmtHeader->PoolsCount, pMgmtHeader->InMemGensCount,
            pGen->MaxTranOpsPerGranule, pGen->MaxStatesPerGranule, pGen->MaxRefStatesPerGranule);

   for (i=0; i < pMgmtHeader->PoolsCount; i++)
   {
      pPool = &pMgmtHeader->GranulePool[i];
      pPool->GranuleSizeBytes = (i == ismSTORE_MGMT_SMALL_POOL_INDEX ? ismStore_memGlobal.MgmtSmallGranuleSizeBytes : ismStore_memGlobal.MgmtGranuleSizeBytes);
      pPool->GranuleDataSizeBytes = pPool->GranuleSizeBytes - sizeof(ismStore_memDescriptor_t);
      pPool->MaxMemSizeBytes = poolMemSizeBytes[i];
      // The reserved memory is allocated in between the two pools
      pPool->Offset = (i == ismSTORE_MGMT_SMALL_POOL_INDEX ? headerSizeBytes : pMgmtHeader->MemSizeBytes - poolMemSizeBytes[ismSTORE_MGMT_POOL_INDEX]);
      ism_store_memPreparePool(ismSTORE_MGMT_GEN_ID, pGen, pPool, i, 1);
   }

   memSizeBytes = ismSTORE_ROUND((pMgmtHeader->TotalMemSizeBytes - pMgmtHeader->MemSizeBytes) / pMgmtHeader->InMemGensCount, ismStore_memGlobal.DiskBlockSizeBytes);
   for (i=0; i < ismStore_memGlobal.InMemGensCount; i++)
   {
      pMgmtHeader->InMemGenOffset[i] = pMgmtHeader->MemSizeBytes + (i * memSizeBytes);
      pGen = ism_store_memInitGenStruct(i);
      pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
      pGenHeader->GenId = ismSTORE_RSRV_GEN_ID;
   }

   // Add the management generation Id to the list of generations
   if ((rc = ism_store_memAddGenIdToList(genId)) != ISMRC_OK)
   {
      goto exit;
   }

   pMgmtHeader->ActiveGenId = pMgmtHeader->NextAvailableGenId = ismSTORE_RSRV_GEN_ID;
   pMgmtHeader->ActiveGenIndex = 0;
   pMgmtHeader->Role = ismSTORE_ROLE_PRIMARY;    // Mark the store role as Primary
   ADR_WRITE_BACK(pMgmtHeader, sizeof(*pMgmtHeader));
   TRACE(5, "Store role has been set to %d\n", pMgmtHeader->Role);

exit:
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

static ismStore_memGeneration_t *ism_store_memInitMgmtGenStruct(void)
{
   ismStore_memGeneration_t *pGen = &ismStore_memGlobal.MgmtGen;

   pGen->pBaseAddress = ismStore_memGlobal.pStoreBaseAddress;
   pGen->Offset = 0;
   pGen->MaxTranOpsPerGranule = (ismStore_memGlobal.MgmtGranuleSizeBytes - sizeof(ismStore_memDescriptor_t) - offsetof(ismStore_memStoreTransaction_t, Operations)) / sizeof(ismStore_memStoreOperation_t);
   pGen->MaxStatesPerGranule = (ismStore_memGlobal.MgmtGranuleSizeBytes - sizeof(ismStore_memDescriptor_t) - offsetof(ismStore_memStateChunk_t, States)) / sizeof(ismStore_memState_t);
   pGen->MaxRefStatesPerGranule = (ismStore_memGlobal.MgmtSmallGranuleSizeBytes - sizeof(ismStore_memDescriptor_t) - offsetof(ismStore_memRefStateChunk_t, RefStates)) / sizeof(uint8_t);

   return pGen;
}

ismStore_memGeneration_t *ism_store_memInitGenStruct(uint8_t genIndex)
{
   size_t gs;
   ismStore_memGeneration_t *pGen = &ismStore_memGlobal.InMemGens[genIndex];
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t *pGenHeader;

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
   pGen->pBaseAddress =  ismStore_memGlobal.pStoreBaseAddress + pMgmtHeader->InMemGenOffset[genIndex];
   pGen->Offset = pMgmtHeader->InMemGenOffset[genIndex];
   pGen->HACreateSqn = pGen->HAActivateSqn = pGen->HAWriteSqn = 0;
   pGen->HACreateState = pGen->HAActivateState = pGen->HAWriteState = 0;

   pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
   gs = (pGenHeader->GenId == ismSTORE_RSRV_GEN_ID) ? ismStore_memGlobal.GranuleSizeBytes : pGenHeader->GranulePool[0].GranuleSizeBytes;
   pGen->MaxRefsPerGranule = (gs - sizeof(ismStore_memDescriptor_t) - offsetof(ismStore_memReferenceChunk_t, References)) / sizeof(ismStore_memReference_t);

   return pGen;
}

// Sort the array of RefStateChunks in ascending order
static int ism_store_memRefStateCompar(const void *pElm1, const void *pElm2)
{
   const ismStore_memRefStateChunk_t *pRefStateChunk1 = *(ismStore_memRefStateChunk_t * const *)pElm1;
   const ismStore_memRefStateChunk_t *pRefStateChunk2 = *(ismStore_memRefStateChunk_t * const *)pElm2;

   if (pRefStateChunk1->OwnerHandle > pRefStateChunk2->OwnerHandle) return 1;
   if (pRefStateChunk1->OwnerHandle < pRefStateChunk2->OwnerHandle) return -1;

   if (pRefStateChunk1->BaseOrderId > pRefStateChunk2->BaseOrderId) return 1;
   if (pRefStateChunk1->BaseOrderId < pRefStateChunk2->BaseOrderId) return -1;

   return 0;
}

static int32_t ism_store_memInitMgmt(uint8_t fStandby2Primary)
{
   int i, j;
   register size_t descStrucSize;
   char sessionIdStr[32];
   ismStore_HASessionID_t sessionId;
   ismStore_memGeneration_t *pGen;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t *pGenHeader, *pGenHeaderN;
   ismStore_memGranulePool_t *pPool;
   ismStore_memGenMap_t *pGenMap;
   ismStore_memDescriptor_t *pDescriptor, *pOwnerDescriptor, *pDescPrev, *pDescHead, *pDescTail;
   ismStore_memSplitItem_t *pSplit;
   ismStore_memReferenceContext_t *pRefCtxt;
   ismStore_memStateContext_t *pStateCtxt;
   ismStore_memRefStateChunk_t *pRefStateChunk, **pRefStateChunkArray;
   ismStore_memStateChunk_t *pStateChunk;
   ismStore_memRefGen_t *pRefGenState;
   ismStore_memGenIdChunk_t *pGenIdChunk;
   ismStore_Handle_t hOwnerHandle, handle;
   ismStore_GenId_t genId;
   uint8_t genIndex, genIndexN, fMgmtChanged=0;
   uint32_t refCtxtCount, stateCtxtCount, refStatesCount, refStatesOrphan, refStatesEmpty;
   uint32_t statesCount, stChunksCount, stChunksOrphan, stChunksEmpty;
   int32_t rc = ISMRC_OK, ec;
   uint64_t offset, tail;

   // Initialize the management generation structure based on the data in the memory
   pGen = &ismStore_memGlobal.MgmtGen;
   pMgmtHeader = (ismStore_memMgmtHeader_t *)pGen->pBaseAddress;
   descStrucSize = pMgmtHeader->DescriptorStructSize;
   ism_store_memB2H(sessionIdStr, (uint8_t *)(pMgmtHeader->SessionId), 10);

   TRACE(5, "Store management data: GenId %u, Version %lu, MemSizeBytes %lu, PoolsCount %u, DescriptorStructSize %u, " \
         "SplitItemStructSize %u, RsrvPoolMemSizeBytes %lu, MaxTranOpsPerGranule %u, MaxStatesPerGranule %u, MaxRefStatesPerGranule %u, " \
         "pBaseAddress %p, State %u, Offset 0x%lx, InMemGensCount %u, ActiveGenId %u, ActiveGenIndex %u, NextAvailableGenId %u, "\
         "Role %d, SessionId %s, SessionCount %u, HaveData %u, WasPrimary %u\n",
         pMgmtHeader->GenId, pMgmtHeader->Version, pMgmtHeader->MemSizeBytes, pMgmtHeader->PoolsCount,
         pMgmtHeader->DescriptorStructSize, pMgmtHeader->SplitItemStructSize, pMgmtHeader->RsrvPoolMemSizeBytes,
         pGen->MaxTranOpsPerGranule, pGen->MaxStatesPerGranule, pGen->MaxRefStatesPerGranule,
         pGen->pBaseAddress, pMgmtHeader->State, pGen->Offset, pMgmtHeader->InMemGensCount,
         pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex, pMgmtHeader->NextAvailableGenId,
         pMgmtHeader->Role, sessionIdStr, pMgmtHeader->SessionCount, pMgmtHeader->HaveData, pMgmtHeader->WasPrimary);

   // Reset the reserved pool if it is not used
   ism_store_memResetRsrvPool();

   // Initalizes the OwnerGranulesLimit
   pPool = &pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX];
   if (pMgmtHeader->Version < 20140101)  /* For v1.1 or older Stores the owners limit is not enforced */
   {
      ismStore_memGlobal.OwnerLimitPct = 100;
      TRACE(3, "Disabling owner limit for an old Store with version %lu (granule size %u). Setting %s=100\n",pMgmtHeader->Version, pPool->GranuleSizeBytes, ismSTORE_CFG_OWNER_LIMIT_PCT);
   }
   ismStore_memGlobal.OwnerGranulesLimit = ((pPool->MaxMemSizeBytes + pMgmtHeader->RsrvPoolMemSizeBytes) / pPool->GranuleSizeBytes) * ismStore_memGlobal.OwnerLimitPct / 100;
   TRACE(5, "Store owner granules limit: %u\n", ismStore_memGlobal.OwnerGranulesLimit);

   handle = pMgmtHeader->GenIdHandle;
   if (fStandby2Primary || !ismStore_global.ColdStartMode)
   {
      // Make sure that the GenIds list is compacted (it might not be compacted if a failure during Add/Remove GenId occured)
      if ((rc = ism_store_memCompactGenIdList(NULL, handle)) != ISMRC_OK)
      {
         return rc;
      }
   }

   // Scan the list of GenIds and allocate a GenMap for each generation
   TRACE(7, "Scan the list of GenIds and allocate a GenMap for each store generation. Handle 0x%lx\n", pMgmtHeader->GenIdHandle);
   if ((handle = pMgmtHeader->GenIdHandle) == ismSTORE_NULL_HANDLE)
   {
      TRACE(1, "Failed to initialize the store because there are no GenId chunks\n");
      return ISMRC_Error;
   }

   genId = ismSTORE_RSRV_GEN_ID;
   while (handle)
   {
      pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
      if ((pDescriptor->DataType & (~ismSTORE_DATATYPE_NOT_PRIMARY)) != ismSTORE_DATATYPE_GEN_IDS)
      {
         TRACE(1, "The GenId chunk (Handle 0x%lx) is not valid. DataType 0x%x\n", handle, pDescriptor->DataType);
         return ISMRC_Error;
      }
      pGenIdChunk = (ismStore_memGenIdChunk_t *)((uintptr_t)pDescriptor + descStrucSize);
      TRACE(5, "GenIds Chunk: Handle  0x%lx, GenIdCount %u, NextHandle 0x%lx\n", handle, pGenIdChunk->GenIdCount, pDescriptor->NextHandle);
      for (i=0; i < pGenIdChunk->GenIdCount; i++)
      {
         genId = pGenIdChunk->GenIds[i];
         TRACE(7, "Allocate a GenMap object for generation Id %u\n", genId);
         if ((rc = ism_store_memAllocGenMap(&genId)) != ISMRC_OK)
         {
            return rc;
         }
      }
      handle = pDescriptor->NextHandle;
   }

   // See whether the last GenId does not exist in the memory
   // (This case could be happened when the server failed during a creation of a new generation)
   if (genId > ismSTORE_MGMT_GEN_ID)
   {
      if (ism_store_memGetGenerationById(genId) == NULL)
      {
         TRACE(4, "The last store generation Id %u does not exist in the memory, probably because the server failed during the creation of this generation\n", genId);
         ism_store_memFreeGenMap(genId);
         ism_store_memDeleteGenIdFromList(genId);
      }
   }

   // Initialize the GenMap of the management generation
   if ((pGenMap = ismStore_memGlobal.pGensMap[ismSTORE_MGMT_GEN_ID]) == NULL)
   {
      TRACE(1, "Failed to initialize the store because there is no entry for the management generation\n");
      return ISMRC_Error;
   }
   pGenMap->pGen = &ismStore_memGlobal.MgmtGen;

   for (j=0; j < pMgmtHeader->PoolsCount; j++)
   {
      pPool = &pMgmtHeader->GranulePool[j];
      pGenMap->GranulesMap[j].GranuleSizeBytes = pPool->GranuleSizeBytes;
      pGenMap->GranulesMap[j].Offset = pPool->Offset;
      pGenMap->GranulesMap[j].Last = pPool->Offset + pPool->MaxMemSizeBytes;
      pGen->PoolMaxCount[j] = pPool->MaxMemSizeBytes / pPool->GranuleSizeBytes;
      pGen->PoolMaxResrv[j] = ismSTORE_MAX_GEN_RESRV_PCT * (pGen->PoolMaxCount[j] / 100);
      pGen->PoolAlertOnCount[j] = pGen->PoolMaxCount[j] * (100 - ismStore_memGlobal.MgmtAlertOnPct) / 100;
      pGen->PoolAlertOffCount[j] = pGen->PoolMaxCount[j] * (100 - ismStore_memGlobal.MgmtAlertOffPct) / 100;

      TRACE(5, "Store management pool: Id %u, MaxMemSizeBytes %lu, GranuleSizeBytes %u, GranuleDataSizeBytes %u, GranuleCount %u, MaxGranuleCount %u, Offset 0x%lx, Last 0x%lx, AlertOnCount %u, AlertOffCount %u\n",
            j, pPool->MaxMemSizeBytes, pPool->GranuleSizeBytes, pPool->GranuleDataSizeBytes, pPool->GranuleCount,
            pGen->PoolMaxCount[j], pGenMap->GranulesMap[j].Offset, pGenMap->GranulesMap[j].Last,
            pGen->PoolAlertOnCount[j], pGen->PoolAlertOffCount[j]);
   }

   memset(ismStore_memGlobal.OwnerCount, 0, sizeof(ismStore_memGlobal.OwnerCount));
   if (fStandby2Primary || !ismStore_global.ColdStartMode)
   {
      // Scan the management small granules and initialize the pRefCtxt and pStateCtxt of the SplitItem
      TRACE(8, "Scan the Store managenemt small granules and initialize the pRefCtxt and pStateCtxt\n");
      pPool = &pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX];
      refCtxtCount = 0; stateCtxtCount = 0;
      for (offset = pPool->Offset, tail = pPool->Offset + pPool->MaxMemSizeBytes; offset < tail; offset += pPool->GranuleSizeBytes)
      {
         pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + offset);
         if (ismSTORE_IS_SPLITITEM(pDescriptor->DataType))
         {
            pSplit = (ismStore_memSplitItem_t *)((uintptr_t)pDescriptor + descStrucSize);
            pSplit->pRefCtxt = 0;
            pSplit->pStateCtxt = 0;
            hOwnerHandle = ismSTORE_BUILD_HANDLE(ismSTORE_MGMT_GEN_ID, offset);
            if ((rc = ism_store_memAllocateRefCtxt(pSplit, hOwnerHandle)) != ISMRC_OK)
            {
               return rc;
            }
            refCtxtCount++;

            if (pDescriptor->DataType == ISM_STORE_RECTYPE_CLIENT)
            {
               if ((rc = ism_store_memAllocateStateCtxt(pSplit, hOwnerHandle)) != ISMRC_OK)
               {
                  return rc;
               }
               stateCtxtCount++;
            }

            TRACE(9, "Context objects for owner (Handle 0x%lx, DataType 0x%x, MinActiveOrderId %lu, DataLength %lu, hLargeData 0x%lx) have been initialized\n",
                  hOwnerHandle, pDescriptor->DataType, pSplit->MinActiveOrderId, pSplit->DataLength, pSplit->hLargeData);

            ADR_WRITE_BACK(pSplit, sizeof(*pSplit));
            ismStore_memGlobal.OwnerCount[ismStore_T2T[pDescriptor->DataType]]++;
            ismStore_memGlobal.OwnerCount[0                                  ]++;
         }
      }
      TRACE(5, "Map of Context objects: RefCtxtCount %u, StateCtxtCount %u\n", refCtxtCount, stateCtxtCount);

      // Scan the management small granules and re-construct the linked-list of RefState
      TRACE(8, "Scan the Store managenemt small granules and re-construct the linked-list of RefState\n");
      pPool = &pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX];
      pDescHead = pDescTail = NULL;
      refStatesCount = 0; refStatesOrphan = 0; refStatesEmpty = 0;
      for (offset = pPool->Offset, tail = pPool->Offset + pPool->MaxMemSizeBytes; offset < tail; offset += pPool->GranuleSizeBytes)
      {
         handle = ismSTORE_BUILD_HANDLE(ismSTORE_MGMT_GEN_ID, offset);
         pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + offset);
         if (pDescriptor->DataType == ismSTORE_DATATYPE_REFSTATES)
         {
            pRefStateChunk = (ismStore_memRefStateChunk_t *)((uintptr_t)pDescriptor + descStrucSize);
            pOwnerDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(pRefStateChunk->OwnerHandle));
            if (ismSTORE_EXTRACT_GENID(pRefStateChunk->OwnerHandle) != ismSTORE_MGMT_GEN_ID)
            {
               TRACE(1, "The owner (Handle 0x%lx, DataType 0x%x) of the RefStateChunk (Handle 0x%lx, BaseOrderId %lu, RefStateCount %u) is not valid\n",
                     pRefStateChunk->OwnerHandle, pOwnerDescriptor->DataType, handle, pRefStateChunk->BaseOrderId, pRefStateChunk->RefStateCount);
               return ISMRC_Error;
            }

            pSplit = (ismStore_memSplitItem_t *)((uintptr_t)pOwnerDescriptor + descStrucSize);
            // See whether the chunk is orphan
            if (!ismSTORE_IS_SPLITITEM(pOwnerDescriptor->DataType) || pRefStateChunk->OwnerVersion != pSplit->Version)
            {
               pDescriptor->DataType = ismSTORE_DATATYPE_FREE_GRANULE;
               pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
               ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));
               TRACE(7, "The RefStateChunk (Handle 0x%lx, OwnerVersion %u, BaseOrderId %lu) is orphan. OwnerHandle 0x%lx, DataType 0x%x, Version %u\n",
                     handle, pRefStateChunk->OwnerVersion, pRefStateChunk->BaseOrderId, pRefStateChunk->OwnerHandle, pOwnerDescriptor->DataType, pSplit->Version);
               refStatesOrphan++;
               continue;
            }

            if (pRefStateChunk->BaseOrderId + pRefStateChunk->RefStateCount <= pSplit->MinActiveOrderId)
            {
               pDescriptor->DataType = ismSTORE_DATATYPE_FREE_GRANULE;
               pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
               ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));
               TRACE(9, "The RefStateChunk (Handle 0x%lx, BaseOrderId %lu, RefStateCount %u) is no longer needed. OwnerHandle 0x%lx, DataType 0x%x, Version %u, MinActiveOrderId %lu\n",
                     handle, pRefStateChunk->BaseOrderId, pRefStateChunk->RefStateCount, pRefStateChunk->OwnerHandle,
                     pOwnerDescriptor->DataType, pSplit->Version, pSplit->MinActiveOrderId);
               refStatesEmpty++;
               continue;
            }

            if (!pSplit->pRefCtxt)
            {
               TRACE(1, "The owner (Handle 0x%lx, DataType 0x%x, Version %u) of the RefStateChunk (Handle 0x%lx, BaseOrderId %lu, RefStateCount %u) has no reference context\n",
                     pRefStateChunk->OwnerHandle, pOwnerDescriptor->DataType, pSplit->Version, handle, pRefStateChunk->BaseOrderId, pRefStateChunk->RefStateCount);
               return ISMRC_Error;
            }

            pDescriptor->NextHandle = 0;
            if (pDescTail)
            {
               pDescTail->NextHandle = (ismStore_Handle_t)pDescriptor;
            }
            else
            {
               pDescHead = pDescriptor;
            }
            pDescTail = pDescriptor;
            refStatesCount++;
         }
      }
      TRACE(5, "Map of RefStateChunks: Count %u, Orphan %u, Empty %u\n", refStatesCount, refStatesOrphan, refStatesEmpty);

      if (refStatesCount > 0)
      {
         if ((pRefStateChunkArray = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,122),refStatesCount * sizeof(ismStore_memRefStateChunk_t *))) == NULL)
         {
            TRACE(1,"Failed to allocate memory for the RefStateChunks array. RefStatesCount %u\n", refStatesCount);
            return ISMRC_AllocateError ;
         }

         for (i=0, pDescriptor=pDescHead; pDescriptor; pDescriptor = (ismStore_memDescriptor_t *)pDescriptor->NextHandle)
         {
            pRefStateChunk = (ismStore_memRefStateChunk_t *)((uintptr_t)pDescriptor + descStrucSize);
            pRefStateChunkArray[i++] = pRefStateChunk;
         }
         qsort(pRefStateChunkArray, refStatesCount, sizeof(ismStore_memRefStateChunk_t *), ism_store_memRefStateCompar);

         for (i=0, hOwnerHandle=ismSTORE_NULL_HANDLE, pDescPrev=NULL, pRefCtxt=NULL; i < refStatesCount; i++)
         {
            pRefStateChunk = pRefStateChunkArray[i];
            pDescriptor = (ismStore_memDescriptor_t *)((uintptr_t)pRefStateChunk - descStrucSize);
            pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
            offset = (uintptr_t)pDescriptor - (uintptr_t)pMgmtHeader;
            handle = ismSTORE_BUILD_HANDLE(ismSTORE_MGMT_GEN_ID, offset);
            if (hOwnerHandle == pRefStateChunk->OwnerHandle)
            {
               if (pDescPrev)
               {
                  pDescPrev->NextHandle = handle;
               }
               pRefCtxt->pRefGenState->hReferenceTail = handle;
               pRefCtxt->pRefGenState->HighestOrderId = pRefStateChunk->BaseOrderId + pRefStateChunk->RefStateCount - 1;
               pRefCtxt->RFChunksInUse++;
               pRefGenState = pRefCtxt->pRefGenState;
            }
            else
            {
               if (pRefCtxt)
               {
                  ism_store_memSetRefGenStates(pRefCtxt, pOwnerDescriptor, pRefStateChunkArray[i-1]);
               }

               hOwnerHandle = pRefStateChunk->OwnerHandle;
               pOwnerDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(hOwnerHandle));
               pSplit = (ismStore_memSplitItem_t *)((uintptr_t)pOwnerDescriptor + descStrucSize);
               pRefCtxt = (ismStore_memReferenceContext_t *)pSplit->pRefCtxt;

               if (pRefCtxt->pRefGenState)
               {
                  TRACE(1, "A RefGenState object already exists for owner (Handle 0x%lx, DataType 0x%x, Version %u)\n",
                        hOwnerHandle, pOwnerDescriptor->DataType, pSplit->Version);
                  ism_common_free(ism_memory_store_misc,pRefStateChunkArray);
                  return ISMRC_Error;
               }

               ismSTORE_MUTEX_LOCK(pRefCtxt->pMutex);
               pRefGenState = ism_store_memAllocateRefGen(pRefCtxt);
               ismSTORE_MUTEX_UNLOCK(pRefCtxt->pMutex);
               if (pRefGenState == NULL)
               {
                  TRACE(1, "Failed to allocate RefGenState for the RefStateChunk (OwnerHandle 0x%lx, DataType 0x%x, Handle 0x%lx, BaseOrderId %lu, RefStateCount %u)\n",
                        pRefStateChunk->OwnerHandle, pOwnerDescriptor->DataType, offset, pRefStateChunk->BaseOrderId, pRefStateChunk->RefStateCount);
                  ism_common_free(ism_memory_store_misc,pRefStateChunkArray);
                  return ISMRC_AllocateError;
               }

               pRefGenState->hReferenceHead = pRefGenState->hReferenceTail = handle;
               pRefGenState->LowestOrderId = pRefStateChunk->BaseOrderId;
               pRefGenState->HighestOrderId = pRefStateChunk->BaseOrderId + pRefStateChunk->RefStateCount - 1;
               pRefCtxt->NextPruneOrderId = pRefStateChunk->BaseOrderId + pRefStateChunk->RefStateCount;
               pRefCtxt->RFChunksInUse    = 1;
               pRefCtxt->RFChunksInUseLWM = 0;
               pRefCtxt->RFChunksInUseHWM = ismSTORE_REFSTATES_CHUNKS;
               pRefCtxt->pRefGenState = pRefGenState;
            }
            pDescPrev = pDescriptor;

            TRACE(9, "The RefStateChunk (Handle 0x%lx, BaseOrderId %lu, RefStateCount %u, NextHandle 0x%lx) has been added to the linked-list " \
                  "of owner (Handle 0x%lx, Version %u, ChunksInUse %u). hReferenceHead 0x%lx, hReferenceTail 0x%lx, LowestOrderId %lu, HighestOrderId %lu, NextPruneOrderId %lu\n",
                  handle, pRefStateChunk->BaseOrderId, pRefStateChunk->RefStateCount, pDescriptor->NextHandle,
                  pRefStateChunk->OwnerHandle, pRefCtxt->OwnerVersion, pRefCtxt->RFChunksInUse,
                  pRefGenState->hReferenceHead, pRefGenState->hReferenceTail, pRefGenState->LowestOrderId,
                  pRefGenState->HighestOrderId, pRefCtxt->NextPruneOrderId);
         }

         if (pRefCtxt)
         {
            ism_store_memSetRefGenStates(pRefCtxt, pOwnerDescriptor, pRefStateChunkArray[i-1]);
         }

         ism_common_free(ism_memory_store_misc,pRefStateChunkArray);
      }

      // Scan the management small granules and re-construct the linked-list of State objects
      TRACE(8, "Scan the Store managenemt granules and re-construct the linked-list of State objects\n");
      stChunksCount = 0; stChunksOrphan = 0; stChunksEmpty = 0;
      pPool = &pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX];
      for (offset = pPool->Offset, tail = pPool->Offset + pPool->MaxMemSizeBytes; offset < tail; offset += pPool->GranuleSizeBytes)
      {
         handle = ismSTORE_BUILD_HANDLE(ismSTORE_MGMT_GEN_ID, offset);
         pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + offset);
         if (pDescriptor->DataType == ismSTORE_DATATYPE_STATES)
         {
            pStateChunk = (ismStore_memStateChunk_t *)((uintptr_t)pDescriptor + descStrucSize);
            pOwnerDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(pStateChunk->OwnerHandle));
            if (ismSTORE_EXTRACT_GENID(pStateChunk->OwnerHandle) != ismSTORE_MGMT_GEN_ID)
            {
               TRACE(1, "The owner (Handle 0x%lx, DataType 0x%x) of the StateChunk (Handle 0x%lx, StatesCount %u, LastAddedIndex %u) is not valid\n",
                     pStateChunk->OwnerHandle, pOwnerDescriptor->DataType, handle, pStateChunk->StatesCount, pStateChunk->LastAddedIndex);
               return ISMRC_Error;
            }

            pSplit = (ismStore_memSplitItem_t *)((uintptr_t)pOwnerDescriptor + descStrucSize);

            // See whether the chunk is orphan
            if (!ismSTORE_IS_SPLITITEM(pOwnerDescriptor->DataType) || pStateChunk->OwnerVersion != pSplit->Version)
            {
               pDescriptor->DataType = ismSTORE_DATATYPE_FREE_GRANULE;
               pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
               ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));
               TRACE(8, "The StateChunk (Handle 0x%lx, OwnerVersion %u, StatesCount %u) is orphan. OwnerHandle 0x%lx, DataType 0x%x, Version %u\n",
                     handle, pStateChunk->OwnerVersion, pStateChunk->StatesCount, pStateChunk->OwnerHandle, pOwnerDescriptor->DataType, pSplit->Version);
               stChunksOrphan++;
               continue;
            }

            if (!pSplit->pStateCtxt)
            {
               TRACE(1, "The owner (Handle 0x%lx, DataType 0x%x, Version %u) of the StateChunk (Handle 0x%lx, StatesCount %u, LastAddedIndex %u) has no state context\n",
                     pStateChunk->OwnerHandle, pOwnerDescriptor->DataType, pSplit->Version, handle, pStateChunk->StatesCount, pStateChunk->LastAddedIndex);
               return ISMRC_Error;
            }
            pStateCtxt = (ismStore_memStateContext_t *)pSplit->pStateCtxt;

            // Make sure that the chunk contains data
            for (i=0, statesCount=0; i < pGen->MaxStatesPerGranule; i++)
            {
               if (pStateChunk->States[i].Flag != ismSTORE_STATEFLAG_EMPTY)
               {
                  statesCount++;
               }
            }
            if ((pStateChunk->StatesCount = statesCount) == 0)
            {
               pDescriptor->DataType = ismSTORE_DATATYPE_FREE_GRANULE;
               pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
               ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));
               TRACE(9, "The StateChunk (Handle 0x%lx, StatesCount %u, LastAddedIndex %u) is no longer needed. OwnerHandle 0x%lx, DataType 0x%x, Version %u\n",
                     handle, pStateChunk->StatesCount, pStateChunk->LastAddedIndex, pStateChunk->OwnerHandle, pOwnerDescriptor->DataType, pSplit->Version);
               stChunksEmpty++;
               continue;
            }

            // Add the chunk to the linked-list
            pDescriptor->NextHandle = pStateCtxt->hStateHead;
            ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor) + sizeof(*pStateChunk));
            pStateCtxt->hStateHead = handle;
            TRACE(9, "The StateChunk (Handle 0x%lx, OwnerVersion %u, StatesCount %u, LastAddedIndex %u) has been added to the linked-list. OwnerHandle 0x%lx, DataType 0x%x\n",
                  handle, pStateChunk->OwnerVersion, pStateChunk->StatesCount, pStateChunk->LastAddedIndex, pStateChunk->OwnerHandle, pOwnerDescriptor->DataType);
            stChunksCount++;
         }
      }
      ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_MSG]] += stChunksCount;
      TRACE(5, "Map of StateChunks: Count %u, Orphan %u, Empty %u\n", stChunksCount, stChunksOrphan, stChunksEmpty);
   }


   // Initialize the InMemory generations
   for (i=0; i < pMgmtHeader->InMemGensCount; i++)
   {
      genIndex = (pMgmtHeader->ActiveGenIndex + i) % ismStore_memGlobal.InMemGensCount;
      pGen = &ismStore_memGlobal.InMemGens[genIndex];
      pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
      pGen->MaxRefsPerGranule = (pGenHeader->GranulePool[0].GranuleSizeBytes - sizeof(ismStore_memDescriptor_t) -
                                 offsetof(ismStore_memReferenceChunk_t, References)) / sizeof(ismStore_memReference_t);

      TRACE(5, "Store generation data: GenId %u (Index %u), Version %lu, MemSizeBytes %lu, GranuleSizeBytes %u, " \
            "PoolsCount %u, DescriptorStructSize %u, SplitItemStructSize %u, RsrvPoolMemSizeBytes %lu, pBaseAddress %p, " \
            "State %u, Offset 0x%lx, MaxRefsPerGranule %u\n",
            pGenHeader->GenId, genIndex, pGenHeader->Version, pGenHeader->MemSizeBytes,
            pGenHeader->GranulePool[0].GranuleSizeBytes, pGenHeader->PoolsCount,
            pGenHeader->DescriptorStructSize, pGenHeader->SplitItemStructSize,
            pGenHeader->RsrvPoolMemSizeBytes, pGen->pBaseAddress,
            pGenHeader->State, pGen->Offset, pGen->MaxRefsPerGranule);

      if (pGenHeader->GenId == ismSTORE_RSRV_GEN_ID)
      {
         TRACE(5, "Generation Index %u is not initialized yet. It is being initialized\n", genIndex);
         ism_store_memCreateGeneration(genIndex, ismSTORE_RSRV_GEN_ID);
         fMgmtChanged = 1;
         continue;
      }

      // During initialization, there is no generation in write pending mode
      if (pGenHeader->State == ismSTORE_GEN_STATE_WRITE_PENDING && ismStore_memGlobal.State == ismSTORE_STATE_INIT)
      {
         pGenHeader->State = ismSTORE_GEN_STATE_CLOSE_PENDING;
         ADR_WRITE_BACK(&pGenHeader->State, sizeof(pGenHeader->State));
         TRACE(5, "Generation Id %u (Index %u) state has been changed from %d to %d\n", pGenHeader->GenId, genIndex, ismSTORE_GEN_STATE_WRITE_PENDING, pGenHeader->State);
      }

      // the generation is already stored on the disk. we can evacuate it from the memory
      if (pGenHeader->State == ismSTORE_GEN_STATE_WRITE_COMPLETED)
      {
         TRACE(5, "Generation Id %u (Index %u) is already stored on the disk. it is evacuated from the memory.\n", pGenHeader->GenId, genIndex);
         ism_store_memCreateGeneration(genIndex, ismSTORE_RSRV_GEN_ID);
         fMgmtChanged = 1;
         continue;
      }

      if (pGenHeader->State == ismSTORE_GEN_STATE_ACTIVE && pMgmtHeader->ActiveGenId == ismSTORE_RSRV_GEN_ID)
      {
         pMgmtHeader->ActiveGenId = pGenHeader->GenId;
         pMgmtHeader->ActiveGenIndex = genIndex;
         fMgmtChanged = 1;
         TRACE(5, "Active generation Id %u (Index %u) has been updated\n", pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex);
      }

      pGenMap = ismStore_memGlobal.pGensMap[pGenHeader->GenId];
      int nP = pGenHeader->PoolsCount <= ismSTORE_GRANULE_POOLS_COUNT ? pGenHeader->PoolsCount : ismSTORE_GRANULE_POOLS_COUNT ; 
      for (j=0; j < nP; j++)
      {
         pPool = &pGenHeader->GranulePool[j];
         pGen->PoolMaxCount[j] = pPool->MaxMemSizeBytes / pPool->GranuleSizeBytes;
         pGen->PoolMaxResrv[j] = ismSTORE_MAX_GEN_RESRV_PCT * (pGen->PoolMaxCount[j] / 100);
         pGen->PoolAlertOnCount[j] = pGen->PoolMaxCount[j] * (100 - ismStore_memGlobal.GenAlertOnPct) / 100;
         pGen->PoolAlertOffCount[j] = 0; // Not used

         pGen->StreamCacheMaxCount[j] = ism_store_memGetStreamCacheCount(pGen, j);
         pGen->StreamCacheBaseCount[j] = pGen->StreamCacheMaxCount[j] / 2;

         if (pGenMap)
         {
            pGenMap->GranulesMap[j].GranuleSizeBytes = pPool->GranuleSizeBytes;
            pGenMap->GranulesMap[j].Offset = pPool->Offset;
            pGenMap->GranulesMap[j].Last = pPool->Offset + pPool->MaxMemSizeBytes;
            pGenMap->pGen = pGen ; 
         }
         else
         {
            TRACE(4, "There is no GenMap for in-memory generation (GenId %u)\n", pGenHeader->GenId);
         }

         TRACE(5, "Store generation pool: GenId %u, PoolId %u, MaxMemSizeBytes %lu, GranuleSizeBytes %u, GranuleDataSizeBytes %u, " \
               "GranuleCount %u, MaxGranuleCount %u, Offset 0x%lx, AlertOnCount %u, StreamCacheMaxCount %u, StreamCacheBaseCount %u\n",
               pGenHeader->GenId, j, pPool->MaxMemSizeBytes, pPool->GranuleSizeBytes,
               pPool->GranuleDataSizeBytes, pPool->GranuleCount, pGen->PoolMaxCount[j],
               pPool->Offset, pGen->PoolAlertOnCount[j], pGen->StreamCacheMaxCount[j],
               pGen->StreamCacheBaseCount[j]);
      }
   }

   // Set the NextAvailabileGenId
   if (pMgmtHeader->ActiveGenId != ismSTORE_RSRV_GEN_ID)
   {
      genIndexN = (pMgmtHeader->ActiveGenIndex + 1) % pMgmtHeader->InMemGensCount;
      pGenHeaderN = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[genIndexN].pBaseAddress;
      pMgmtHeader->NextAvailableGenId = (pGenHeaderN->State == ismSTORE_GEN_STATE_FREE ? pGenHeaderN->GenId : ismSTORE_RSRV_GEN_ID);
      TRACE(5, "Store ActiveGenId %u (Index %u), NextAvailableGenId %u (Index %u)\n",
            pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex, pMgmtHeader->NextAvailableGenId, genIndexN);
   }

   // Scan the open streams and close (either commit or rollback) the active store-transactions
   if (fStandby2Primary || !ismStore_global.ColdStartMode)
   {
      TRACE(5, "Scan the management generation and close the previous open store-transactions\n");
      pGen = &ismStore_memGlobal.MgmtGen;
      pPool = &pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX];
      for (offset = pPool->Offset, tail = pPool->Offset + pPool->MaxMemSizeBytes; offset < tail; offset += pPool->GranuleSizeBytes)
      {
         handle = ismSTORE_BUILD_HANDLE(ismSTORE_MGMT_GEN_ID, offset);
         pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + offset);
         if (pDescriptor->DataType == ismSTORE_DATATYPE_STORE_TRAN)
         {
            ismStore_memStoreTransaction_t *pTran = (ismStore_memStoreTransaction_t *)((uintptr_t)pDescriptor + descStrucSize);
            ismStore_GenId_t tranGenId = pTran->GenId;
            uint8_t tranState = pTran->State;
            uint32_t tranOpCount = pTran->OperationCount;
            if (pTran->State == ismSTORE_MEM_STORETRANSACTIONSTATE_COMMITTING)
            {
               ism_store_memCommitInternal(NULL, pDescriptor);
               TRACE(5, "A previous store-transaction (Handle 0x%lx, GenId %u, OperationCount %u, State %u) has been committed\n",
                     handle, tranGenId, tranOpCount, tranState);
            }
            else
            {
               ism_store_memRollbackInternal(NULL, handle);
               TRACE(5, "A previous store-transaction (Handle 0x%lx, GenId %u, OperationCount %u, State %u) has been rollbacked\n",
                     handle, tranGenId, tranOpCount, tranState);
            }
            ism_store_memReturnPoolElements(NULL, handle, 0) ;
         }
      }

      // Re-construct the pools of the management generation
      for (j=0; j < pMgmtHeader->PoolsCount; j++)
      {
         pPool = &pMgmtHeader->GranulePool[j];
         ism_store_memPreparePool(ismSTORE_MGMT_GEN_ID, pGen, pPool, j, 0);
         TRACE(5, "Store management pool has been rearranged. PoolId %u, GranuleSizeBytes %u, GranuleCount %u\n", j, pPool->GranuleSizeBytes, pPool->GranuleCount);
      }

      // Re-construct the pools of the active data generation
      pGen = &ismStore_memGlobal.InMemGens[pMgmtHeader->ActiveGenIndex];
      pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
      for (j=0; j < pGenHeader->PoolsCount; j++)
      {
         pPool = &pGenHeader->GranulePool[j];
         ism_store_memPreparePool(pGenHeader->GenId, pGen, pPool, j, 0);
         TRACE(5, "Store generation pool has been rearranged. GenId %u, PoolId %u, GranuleSizeBytes %u, GranuleCount %u\n",
               pGenHeader->GenId, j, pPool->GranuleSizeBytes, pPool->GranuleCount);
      }
   }

   // Set the High-Availability SessionId and SessionCount
   memset(sessionId, 0, sizeof(ismStore_HASessionID_t));
   if (ismStore_global.fHAEnabled)
   {
      // See whether we have to allocate a new SessionId
      if (!fStandby2Primary || memcmp(pMgmtHeader->SessionId, sessionId, sizeof(ismStore_HASessionID_t)) == 0)
      {
         if ((ec = ism_storeHA_allocateSessionId(sessionId)) != StoreRC_OK)
         {
            TRACE(1, "Failed to allocate an High-Availability SessionId. error code %d\n", ec);
            return ISMRC_Error;
         }
         memcpy(pMgmtHeader->SessionId, &sessionId, sizeof(ismStore_HASessionID_t));
         pMgmtHeader->SessionCount = 2;
         ism_store_memB2H(sessionIdStr, (uint8_t *)(pMgmtHeader->SessionId), sizeof(ismStore_HASessionID_t));
         TRACE(5, "A new store High-Availability SessionId has been allocated. SessionId %s, SessionCount %u\n", sessionIdStr, pMgmtHeader->SessionCount);
      }
      else
      {
         pMgmtHeader->SessionCount += (2 + (pMgmtHeader->SessionCount % 2));
         ism_store_memB2H(sessionIdStr, (uint8_t *)(pMgmtHeader->SessionId), sizeof(ismStore_HASessionID_t));
         TRACE(5, "Store High-Availability SessionCount has been incremented. SessionId %s, SessionCount %u\n", sessionIdStr, pMgmtHeader->SessionCount);
      }
   }
   else if (memcmp(pMgmtHeader->SessionId, sessionId, sizeof(ismStore_HASessionID_t)) != 0)
   {
      memset(pMgmtHeader->SessionId, 0, sizeof(ismStore_HASessionID_t));
      pMgmtHeader->SessionCount = 0;
      fMgmtChanged = 1;
      TRACE(5, "Store High-Availability SessionId has been reset\n");
   }

   // If the management generation has been changed, then we have to reset the management generation token
   if (fMgmtChanged)
   {
      memset(&pMgmtHeader->GenToken, '\0', sizeof(ismStore_memGenToken_t));
      TRACE(5, "Store management generation token has been reset\n");
   }
   ADR_WRITE_BACK(pMgmtHeader, sizeof(*pMgmtHeader));

   TRACE(8, "Store management initialization completed successfully\n");

   return rc;
}

static int32_t ism_store_memRecoveryPrepare(uint8_t fStandby2Primary)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t  genHeader;
   ismStore_memGenMap_t *pGenMap;
   ismStore_memGeneration_t *pGen;
   ismStore_GenId_t genId;
   ismStore_DiskBufferParams_t buffParams;
   char nodeStr[40];
   int i;
   uint8_t fRecoveryMem;
   uint64_t diskFileSize;
   int32_t rc = ISMRC_OK, ec;

   TRACE(9, "Entry: %s. fStandby2Primary %u\n", __FUNCTION__, fStandby2Primary);

   if (fStandby2Primary && (ec = ism_storeDisk_removeCompactTasks()) != StoreRC_OK)
   {
      TRACE(1, "Failed to remove store disk compaction tasks. error code %d\n", ec);
   }

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;

   // Initialize the management generation
   if ((rc = ism_store_memInitMgmt(fStandby2Primary)) != ISMRC_OK)
   {
      goto exit;
   }

   pGen = &ismStore_memGlobal.InMemGens[pMgmtHeader->ActiveGenIndex] ; 
   ismStore_memGlobal.RefChunkHWM = ismStore_memGlobal.RefChunkHWMpct * pGen->PoolMaxCount[0]  ; 
   ismStore_memGlobal.RefChunkLWM = ismStore_memGlobal.RefChunkLWMpct * pGen->MaxRefsPerGranule ; 

   // Mark the store role as Primary
   if (pMgmtHeader->Role != ismSTORE_ROLE_PRIMARY)
   {
      TRACE(5, "Store role has been changed from %d to %d\n", pMgmtHeader->Role, ismSTORE_ROLE_PRIMARY);
      pMgmtHeader->Role = ismSTORE_ROLE_PRIMARY;
      ADR_WRITE_BACK(&pMgmtHeader->Role, sizeof(pMgmtHeader->Role));
   }

   // Set the GenToken of the management generation
   pGenMap = ismStore_memGlobal.pGensMap[ismSTORE_MGMT_GEN_ID];
   memcpy(&pGenMap->GenToken, &pMgmtHeader->GenToken, sizeof(ismStore_memGenToken_t));
   pGenMap->DiskFileSize = 0;
   ism_store_memB2H(nodeStr, (uint8_t *)pMgmtHeader->GenToken.NodeId, sizeof(ismStore_HANodeID_t));
   TRACE(5, "Management generation information has been set. GenId %u, GenToken %s:%lu, MemSizeBytes %lu\n",
         ismSTORE_MGMT_GEN_ID, nodeStr, pMgmtHeader->GenToken.Timestamp, pMgmtHeader->MemSizeBytes);

   // Scan the list of GenIds and retrieve the file size and GenToken of each generation that is stored on the disk
   TRACE(5, "Scan the list of GenIds and retrieve the generation information for each generation file. GenMapsSize %u, GanMapsCount %u\n",
         ismStore_memGlobal.GensMapSize, ismStore_memGlobal.GensMapCount);
   for (i=ismSTORE_MGMT_GEN_ID+1; i < ismStore_memGlobal.GensMapSize; i++)
   {
      genId = (ismStore_GenId_t)i;
      if ((pGenMap = ismStore_memGlobal.pGensMap[genId]) == NULL) { continue; }

      if (ism_store_memGetGenerationById(genId) == NULL)
      {
         memset(&genHeader, '\0', sizeof(genHeader));
         fRecoveryMem = 0;

         // Retrieve the disk file size of the generation file
         if ((ec = ism_storeDisk_getGenerationSize(genId, &diskFileSize)) != StoreRC_OK)
         {
            TRACE(1, "Failed to retrieve the file size of a generation file (GenId %u) from the disk. error code %d\n", genId, ec);
            rc = ISMRC_StoreDiskError;
            goto exit;
         }

         if (fStandby2Primary)
         {
            // See whether we can read the generation header directly from the recovery memory
            memset(&buffParams, '\0', sizeof(buffParams));
            buffParams.pBuffer = (char *)&genHeader;
            buffParams.BufferLength = sizeof(genHeader);
            if ((ec = ism_store_memRecoveryGetGenerationData(genId, &buffParams)) == ISMRC_OK && buffParams.BufferLength == sizeof(genHeader))
            {
               fRecoveryMem = 1;
            }
         }

         if (!fRecoveryMem && (ec = ism_storeDisk_getGenerationHeader(genId, sizeof(genHeader), (char *)&genHeader)) != StoreRC_OK)
         {
            TRACE(1, "Failed to retrieve the data header of a generation file (GenId %u) from the disk. error code %d\n", genId, ec);
            rc = ISMRC_StoreDiskError;
            goto exit;
         }
         memcpy(&pGenMap->GenToken, &genHeader.GenToken, sizeof(ismStore_memGenToken_t));
         pGenMap->DiskFileSize = pGenMap->PrevPredictedSizeBytes = pGenMap->HARemoteSizeBytes = diskFileSize;
         pGenMap->StdDevBytes = genHeader.StdDevBytes;
         // Initialize the PredictedSizeBytes to include header size and disk block alignment
         pGenMap->PredictedSizeBytes += genHeader.GranulePool[0].Offset;
         if (genHeader.CompactSizeBytes > 0)
         {
            pGenMap->PredictedSizeBytes += (diskFileSize - genHeader.CompactSizeBytes);
         }

         ism_store_memB2H(nodeStr, (uint8_t *)genHeader.GenToken.NodeId, sizeof(ismStore_HANodeID_t));
         if (fRecoveryMem)
         {
            TRACE(5, "A generation file (Id %u, FileSize %lu) has been found in the recovery memory. StrucId 0x%x, GenId %u, GenToken %s:%lu, State %u, PoolsCount %u, Version %lu, MemSizeBytes %lu, StdDevBytes %lu, DescriptorStructSize %u, SplitStructSize %u, PredictedSizeBytes %lu\n",
                  genId, diskFileSize, genHeader.StrucId, genHeader.GenId, nodeStr,
                  genHeader.GenToken.Timestamp, genHeader.State, genHeader.PoolsCount,
                  genHeader.Version, genHeader.MemSizeBytes, genHeader.StdDevBytes,
                  genHeader.DescriptorStructSize, genHeader.SplitItemStructSize, pGenMap->PredictedSizeBytes);
         }
         else
         {
            TRACE(5, "A generation file (Id %u, FileSize %lu) has been found on the disk. StrucId 0x%x, GenId %u, GenToken %s:%lu, State %u, PoolsCount %u, Version %lu, MemSizeBytes %lu, StdDevBytes %lu, DescriptorStructSize %u, SplitStructSize %u, PredictedSizeBytes %lu\n",
                  genId, diskFileSize, genHeader.StrucId, genHeader.GenId, nodeStr,
                  genHeader.GenToken.Timestamp, genHeader.State, genHeader.PoolsCount,
                  genHeader.Version, genHeader.MemSizeBytes, genHeader.StdDevBytes,
                  genHeader.DescriptorStructSize, genHeader.SplitItemStructSize, pGenMap->PredictedSizeBytes);
         }
      }
      else
      {
         TRACE(5, "Generation %u is stored in the memory\n", genId);
      }
   }

   // Allocate an internal stream for internal used
   if ((rc = ism_store_memOpenStreamEx(&ismStore_memGlobal.hInternalStream, 0, 1)) != ISMRC_OK)
   {
      goto exit;
   }

   if ((rc = ism_store_memRecoveryStart()) != ISMRC_OK)
   {
      TRACE(1, "Failed to start the store recovery. error code %d\n", rc);
      goto exit;
   }

exit:
   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

static int32_t ism_store_memRestoreFromDisk(void)
{
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_DiskTaskParams_t diskTask;
   ism_time_t rTime;
   uint64_t diskGenSize, memSize;
   int ec, state;
   int32_t rc = ISMRC_OK;

   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
   state = ismStore_memGlobal.State;
   rTime = ism_common_currentTimeNanos();

   do
   {
      // Restore the management generation from the disk
      memSize = ismSTORE_ROUND(ismStore_memGlobal.MgmtMemPct * ismStore_memGlobal.TotalMemSizeBytes / 100, ismStore_memGlobal.DiskBlockSizeBytes);
      if ((ec = ism_storeDisk_getGenerationSize(ismSTORE_MGMT_GEN_ID, &diskGenSize)) != StoreRC_OK)
      {
         TRACE(1, "Failed to retrieve the file size of the management generation file (GenId %u) from the disk. error code %d", ismSTORE_MGMT_GEN_ID, ec);
         rc = ISMRC_Error;
         break;
      }

      if (diskGenSize < sizeof(ismStore_memMgmtHeader_t))
      {
         TRACE(1, "Failed to restore the management generation from the disk, because the disk file is corrupted (DiskSizeBytes %lu, HeaderSizeBytes %lu).\n", diskGenSize, sizeof(ismStore_memMgmtHeader_t));
         rc = ISMRC_Error;
         break;
      }

      if (diskGenSize > memSize)
      {
         TRACE(1, "Failed to restore the management generation from the disk, because there is not enough memory in the store (DiskSizeBytes %lu, MemSizeBytes %lu).\n", diskGenSize, memSize);
         rc = ISMRC_StoreAllocateError;
         break;
      }

      memset(&diskTask, '\0', sizeof(diskTask));
      diskTask.GenId = ismSTORE_MGMT_GEN_ID;
      diskTask.Priority = 0;
      diskTask.Callback = ism_store_memDiskReadComplete;
      diskTask.BufferParams->pBuffer = ismStore_memGlobal.pStoreBaseAddress;
      diskTask.BufferParams->BufferLength = diskGenSize;
      if ((ec = ism_storeDisk_readGeneration(&diskTask)) != StoreRC_OK)
      {
         TRACE(1, "Failed to restore the management generation from the disk. error code %d\n", ec);
         rc = ISMRC_Error;
         break;
      }
      TRACE(5, "Restoring the management generation from the disk into the memory (DiskSizeBytes %lu, MemSizeBytes %lu).\n", diskGenSize, memSize);

      // Wait until the disk read operation completes
      ismStore_memGlobal.State = ismSTORE_STATE_RESTORING;
      while (ismStore_memGlobal.State == ismSTORE_STATE_RESTORING)
      {
         pthread_cond_wait(&ismStore_memGlobal.StreamsCond, &ismStore_memGlobal.StreamsMutex);
      }

      if (ismStore_memGlobal.State != ismSTORE_STATE_RESTORED || ism_store_memValidate() != ISMRC_OK)
      {
         rc = ISMRC_Error;
         break;
      }

      // Restore the active generation from the disk
      pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
      if (pMgmtHeader->TotalMemSizeBytes > ismStore_memGlobal.TotalMemSizeBytes)
      {
         TRACE(1, "Failed to restore the active generation from the disk, because there is not enough memory in the store (GenId %u, TotalMemSizeBytes %lu, StoreMemSizeBytes %lu).\n",
               pMgmtHeader->ActiveGenId, pMgmtHeader->TotalMemSizeBytes, ismStore_memGlobal.TotalMemSizeBytes);
         rc = ISMRC_Error;
         break;
      }

      if (pMgmtHeader->ActiveGenId != ismSTORE_RSRV_GEN_ID)
      {
         memSize =  ismSTORE_ROUND((pMgmtHeader->TotalMemSizeBytes - pMgmtHeader->MemSizeBytes) / pMgmtHeader->InMemGensCount, ismStore_memGlobal.GranuleSizeBytes);
         if ((ec = ism_storeDisk_getGenerationSize(pMgmtHeader->ActiveGenId, &diskGenSize)) != StoreRC_OK)
         {
            TRACE(1, "Failed to retrieve the file size of the active generation file (GenId %u, GenIndex %u) from the disk. error code %d", pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex, ec);
            rc = ISMRC_Error;
            break;
         }

         if (diskGenSize < sizeof(ismStore_memGenHeader_t))
         {
            TRACE(1, "Failed to restore the active generation from the disk, because the disk file is corrupted (GenId %u, GenIndex %u, DiskSizeBytes %lu, HeaderSizeBytes %lu).\n",
                  pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex, diskGenSize, sizeof(ismStore_memGenHeader_t));
            rc = ISMRC_Error;
            break;
         }

         if (diskGenSize > memSize)
         {
            TRACE(1, "Failed to restore the active generation from the disk, because there is not enough memory in the store (GenId %u, GenIndex %u, DiskSizeBytes %lu, MemSizeBytes %lu).\n",
                  pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex, diskGenSize, memSize);
            rc = ISMRC_StoreAllocateError;
            break;
         }

         pGenHeader = (ismStore_memGenHeader_t *)(ismStore_memGlobal.pStoreBaseAddress + pMgmtHeader->InMemGenOffset[pMgmtHeader->ActiveGenIndex]);
         memset(&diskTask, '\0', sizeof(diskTask));
         diskTask.GenId = pMgmtHeader->ActiveGenId;
         diskTask.Priority = 0;
         diskTask.Callback = ism_store_memDiskReadComplete;
         diskTask.BufferParams->pBuffer = (char *)pGenHeader;
         diskTask.BufferParams->BufferLength = diskGenSize;
         if ( (ec = ism_storeDisk_readGeneration(&diskTask)) != StoreRC_OK)
         {
            TRACE(1, "Failed to restore the active generation from the disk (GenId %u, GenIndex %u, DiskSizeBytes %lu). error code %d\n",
                  pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex, diskGenSize, ec);
            rc = ISMRC_Error;
            break;
         }
         TRACE(5, "Restoring the active generation from the disk into the memory (GenId %u, GenIndex %u, DiskSizeBytes %lu, MemSizeBytes %lu).\n",
               pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex, diskGenSize, memSize);

         ismStore_memGlobal.State = ismSTORE_STATE_RESTORING;
         while (ismStore_memGlobal.State == ismSTORE_STATE_RESTORING)
         {
            pthread_cond_wait(&ismStore_memGlobal.StreamsCond, &ismStore_memGlobal.StreamsMutex);
         }

         if (ismStore_memGlobal.State != ismSTORE_STATE_RESTORED)
         {
            rc = ISMRC_Error;
            break;
         }

         // Make sure that the generation memory header is valid
         if ((pGenHeader->StrucId != ismSTORE_MEM_GENHEADER_STRUCID) ||
              ism_store_memCheckStoreVersion(pGenHeader->Version, ismSTORE_VERSION) ||
             (pGenHeader->GenId != pMgmtHeader->ActiveGenId) ||
             (pGenHeader->MemSizeBytes > memSize) ||
             (pGenHeader->PoolsCount == 0) ||
             (pGenHeader->PoolsCount > ismSTORE_GRANULE_POOLS_COUNT))
         {
            TRACE(1, "The active generation header is not valid (StructId %u, Version %lu, GenId %u, MemSizeBytes %lu, PoolsCount %u).",
                  pGenHeader->StrucId, pGenHeader->Version, pGenHeader->GenId, pGenHeader->MemSizeBytes, pGenHeader->PoolsCount);
            rc = ISMRC_Error;
            break;
         }

         rTime = (ism_common_currentTimeNanos() - rTime) / 1e3;
         TRACE(5, "Store management and active generations have been restored from the disk. Time %lu usec\n", rTime);
      }
   } while (0);

   if (rc == ISMRC_OK)
      ismStore_memGlobal.State = state;

   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

   return rc;
}

/*
 * Makes an handle to a pointer.
 */
void *ism_store_memMapHandleToAddress(ismStore_Handle_t handle)
{
   ismStore_GenId_t genId   = ismSTORE_EXTRACT_GENID(handle);
   ismStore_Handle_t offset = ismSTORE_EXTRACT_OFFSET(handle);

   if (genId == ismSTORE_MGMT_GEN_ID)
   {
     return (void *)(ismStore_memGlobal.MgmtGen.pBaseAddress + offset);
   }
   else
   {
     int i;
     ismStore_memGenHeader_t *pGenHeader;
    #if ismSTORE_MIN_INMEM_GENS > 0
     pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[0].pBaseAddress;
     if (pGenHeader->GenId == genId )
     {
       if (pGenHeader->State < ismSTORE_GEN_STATE_WRITE_PENDING || (ismStore_global.fHAEnabled && ismStore_memGlobal.HAInfo.State != ismSTORE_HA_STATE_PRIMARY) )
         return (void *)((uintptr_t)pGenHeader + offset);
       return NULL;
     }
    #endif
    #if ismSTORE_MIN_INMEM_GENS > 1
     pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[1].pBaseAddress;
     if (pGenHeader->GenId == genId )
     {
       if (pGenHeader->State < ismSTORE_GEN_STATE_WRITE_PENDING || (ismStore_global.fHAEnabled && ismStore_memGlobal.HAInfo.State != ismSTORE_HA_STATE_PRIMARY) )
         return (void *)((uintptr_t)pGenHeader + offset);
       return NULL;
     }
    #endif
     for (i=ismSTORE_MIN_INMEM_GENS; i < ismStore_memGlobal.InMemGensCount && i < ismSTORE_MAX_INMEM_GENS; i++)
     {
        pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[i].pBaseAddress;
        if (pGenHeader->GenId == genId )
        {
          if (pGenHeader->State < ismSTORE_GEN_STATE_WRITE_PENDING || (ismStore_global.fHAEnabled && ismStore_memGlobal.HAInfo.State != ismSTORE_HA_STATE_PRIMARY) )
            return (void *)((uintptr_t)pGenHeader + offset);
          return NULL;
        }
     }
   }

   return NULL;
}

ismStore_memGeneration_t *ism_store_memGetGenerationById(ismStore_GenId_t genId)
{
   uint8_t genIndex;

   if (genId == ismSTORE_MGMT_GEN_ID)
   {
      return &ismStore_memGlobal.MgmtGen;
   }
   else if ((genIndex = ism_store_memMapGenIdToIndex(genId)) < ismStore_memGlobal.InMemGensCount)
   {
      return &ismStore_memGlobal.InMemGens[genIndex];
   }

   return NULL;
}

static uint8_t ism_store_memMapGenIdToIndex(ismStore_GenId_t genId)
{
   int i;

  #if ismSTORE_MIN_INMEM_GENS > 0
   if (((ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[0].pBaseAddress)->GenId == genId) return 0 ; 
  #endif
  #if ismSTORE_MIN_INMEM_GENS > 1
   if (((ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[1].pBaseAddress)->GenId == genId) return 1 ; 
  #endif
   for (i=ismSTORE_MIN_INMEM_GENS; i < ismStore_memGlobal.InMemGensCount && i < ismSTORE_MAX_INMEM_GENS; i++)
   {
      if (((ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[i].pBaseAddress)->GenId == genId)
      {
          return i;
      }
   }

   return 0xff;
}

/*
 * Add a generation id to the ordered list in the store
 */
static int32_t ism_store_memAddGenIdToList(ismStore_GenId_t genId)
{
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memGenIdChunk_t *pGenIdChunk;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memEnsureGenIdAllocation(&pDescriptor)) == ISMRC_OK)
   {
      pGenIdChunk = (ismStore_memGenIdChunk_t *)(pDescriptor + 1);
      pGenIdChunk->GenIds[pGenIdChunk->GenIdCount] = genId;
      ADR_WRITE_BACK(&pGenIdChunk->GenIds[pGenIdChunk->GenIdCount], sizeof(pGenIdChunk->GenIds[pGenIdChunk->GenIdCount]));
      pGenIdChunk->GenIdCount++;
      ADR_WRITE_BACK(&pGenIdChunk->GenIdCount, sizeof(pGenIdChunk->GenIdCount));
      TRACE(5, "A store generation Id %u has been added to the list. GenIdCount %u\n", genId, pGenIdChunk->GenIdCount);
   }

   return rc;
}

/*
 * Remove a generation id from the ordered list in the store
 */
static void ism_store_memDeleteGenIdFromList(ismStore_GenId_t genId)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memDescriptor_t *pDescriptor, *pPrevDescriptor=NULL;
   ismStore_memGenIdChunk_t *pGenIdChunk=NULL;
   ismStore_Handle_t handle;
   int i;

   TRACE(7, "Entry: %s. GenId %u\n", __FUNCTION__, genId);
   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
   if ((handle = pMgmtHeader->GenIdHandle))
   {
      pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
      while (pDescriptor)
      {
         pGenIdChunk = (ismStore_memGenIdChunk_t *)(pDescriptor + 1);
         for (i=0; i < pGenIdChunk->GenIdCount; i++)
         {
            if (pGenIdChunk->GenIds[i] == genId)
            {
               pGenIdChunk->GenIds[i] = ismSTORE_RSRV_GEN_ID;
               ADR_WRITE_BACK(&pGenIdChunk->GenIds[i], sizeof(ismStore_GenId_t));
               TRACE(5, "The store generation Id %u has removed from the list. GenIdChunk 0x%lx, Index %d, GenIdCount %u, NextHandle 0x%lx\n",
                     genId, handle, i, pGenIdChunk->GenIdCount, pDescriptor->NextHandle);
               ism_store_memCompactGenIdList(pPrevDescriptor, handle);
               break;
            }
         }
         pPrevDescriptor = pDescriptor;
         handle = pDescriptor->NextHandle;
         pDescriptor = (handle ? (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle) : NULL);
      }
   }

   TRACE(7, "Exit %s\n", __FUNCTION__);
}

static int32_t ism_store_memCompactGenIdList(ismStore_memDescriptor_t *pPrevDescriptor, ismStore_Handle_t hGenIdChunk)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memGenIdChunk_t *pGenIdChunk=NULL, *pPrevGenIdChunk;
   ismStore_GenId_t prevGenId = ismSTORE_RSRV_GEN_ID;
   ismStore_Handle_t handle = hGenIdChunk;
   uint8_t fFound;
   uint32_t genIdsPerChunk;
   int32_t rc = ISMRC_OK;
   int i;

   TRACE(7, "Entry: %s\n", __FUNCTION__);
   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
   pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
   pGenIdChunk = (ismStore_memGenIdChunk_t *)(pDescriptor + 1);
   while (pDescriptor)
   {
      if ((pDescriptor->DataType & (~ismSTORE_DATATYPE_NOT_PRIMARY)) != ismSTORE_DATATYPE_GEN_IDS)
      {
         TRACE(1, "The GenId chunk (Handle 0x%lx) is not valid. DataType 0x%x\n", handle, pDescriptor->DataType);
         return ISMRC_Error;
      }
      TRACE(7, "CompactGenIdList: GenIdChunk 0x%lx, GenIdCount %u, NextHandle 0x%lx, fFirst %d\n",
            handle, pGenIdChunk->GenIdCount, pDescriptor->NextHandle, (pPrevDescriptor ? 0 : 1));

      if (pGenIdChunk->GenIdCount == 0)
      {
         if (pPrevDescriptor)
         {
            pPrevDescriptor->NextHandle = pDescriptor->NextHandle;
            ADR_WRITE_BACK(&pPrevDescriptor->NextHandle, sizeof(pPrevDescriptor->NextHandle));
         }
         else
         {
            pMgmtHeader->GenIdHandle = pDescriptor->NextHandle;
            ADR_WRITE_BACK(&pMgmtHeader->GenIdHandle, sizeof(pMgmtHeader->GenIdHandle));
            TRACE(1, "The first GenID chunk (Handle 0x%lx, NextHandle 0x%lx) is empty\n", handle, pDescriptor->NextHandle);
            rc = ISMRC_Error;
         }

         if (pDescriptor->NextHandle != ismSTORE_NULL_HANDLE)
         {
            TRACE(1, "An empty GenID chunk has been found in the middle of the linked-list (Handle 0x%lx, NextHandle 0x%lx, fFirst %d)\n",
                  handle, pDescriptor->NextHandle, (pPrevDescriptor ? 0 : 1));
            pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
            rc = ISMRC_Error;
         }
         ism_store_memReturnPoolElements(NULL, handle, 0);
         TRACE(5, "The GenID chunk (Handle 0x%lx, NextHandle 0x%lx, fFirst %d) has been removed\n",
               handle, pDescriptor->NextHandle, (pPrevDescriptor ? 0 : 1));
         break;
      }

      // See whether there is a free place in the current GenId chunk
      for (i=0, fFound=0; i < pGenIdChunk->GenIdCount; i++)
      {
         if (fFound)
         {
            pGenIdChunk->GenIds[i-1] = pGenIdChunk->GenIds[i];
         }
         else if (pGenIdChunk->GenIds[i] == ismSTORE_RSRV_GEN_ID || pGenIdChunk->GenIds[i] == prevGenId)
         {
            TRACE(5, "The GenId chunk (Handle 0x%lx, NextHandle 0x%lx, fFirst %d) has a free place (Index %u, GenId %u, prevGenId %u). GenIdCount %u, FirstGenId %u, LastGenId %u\n",
                  handle, pDescriptor->NextHandle, (pPrevDescriptor ? 0 : 1), i, pGenIdChunk->GenIds[i], prevGenId, pGenIdChunk->GenIdCount, pGenIdChunk->GenIds[0], pGenIdChunk->GenIds[pGenIdChunk->GenIdCount-1]);
            pGenIdChunk->GenIds[i] = ismSTORE_RSRV_GEN_ID;
            fFound = 1;
         }
         prevGenId = pGenIdChunk->GenIds[i];
      }

      if (fFound)
      {
         ADR_WRITE_BACK(pGenIdChunk, pDescriptor->DataLength);
         // We want to make sure that the GenIds are stored in the NVRAM before the GenIdCount is decremneted
         pGenIdChunk->GenIdCount--;
         ADR_WRITE_BACK(&pGenIdChunk->GenIdCount, sizeof(pGenIdChunk->GenIdCount));
         if (pGenIdChunk->GenIdCount == 0) { continue; }
      }

      // If this is the last chunk, the compaction is completed
      if (pDescriptor->NextHandle == ismSTORE_NULL_HANDLE)
      {
         break;
      }

      pPrevDescriptor = pDescriptor;
      pPrevGenIdChunk = pGenIdChunk;
      pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pDescriptor->NextHandle);
      pGenIdChunk = (ismStore_memGenIdChunk_t *)(pDescriptor + 1);

      genIdsPerChunk = (pPrevDescriptor->DataLength - sizeof(ismStore_memGenIdChunk_t)) / sizeof(ismStore_GenId_t) + 1;
      if (pPrevGenIdChunk->GenIdCount < genIdsPerChunk)
      {
         TRACE(5, "The GenId chunk (Handle 0x%lx, NextHandle 0x%lx, fFirst %d) is not full. GenIdCount %u, MaxGenIdsPerGranule %u, FirstGenId %u, LastGenId %u\n",
               handle, pPrevDescriptor->NextHandle, (pPrevDescriptor ? 0 : 1), pPrevGenIdChunk->GenIdCount,
               genIdsPerChunk, pPrevGenIdChunk->GenIds[0], pPrevGenIdChunk->GenIds[pPrevGenIdChunk->GenIdCount-1]);

         if (pGenIdChunk->GenIdCount > 0 && pGenIdChunk->GenIds[0] != ismSTORE_RSRV_GEN_ID && pGenIdChunk->GenIds[0] != prevGenId)
         {
            pPrevGenIdChunk->GenIds[pPrevGenIdChunk->GenIdCount] = prevGenId = pGenIdChunk->GenIds[0];
            ADR_WRITE_BACK(&pPrevGenIdChunk->GenIds[pPrevGenIdChunk->GenIdCount], sizeof(ismStore_GenId_t));
            pPrevGenIdChunk->GenIdCount++;
            ADR_WRITE_BACK(&pPrevGenIdChunk->GenIdCount, sizeof(pPrevGenIdChunk->GenIdCount));
            pGenIdChunk->GenIds[0] = ismSTORE_RSRV_GEN_ID;
            TRACE(5, "The GenId chunk (Handle 0x%lx, NextHandle 0x%lx, fFirst %d) has been incremented. GenIdCount %u, MaxGenIdsPerGranule %u, FirstGenId %u, LastGenId %u\n",
                  handle, pPrevDescriptor->NextHandle, (pPrevDescriptor ? 0 : 1), pPrevGenIdChunk->GenIdCount,
                  genIdsPerChunk, pPrevGenIdChunk->GenIds[0], pPrevGenIdChunk->GenIds[pPrevGenIdChunk->GenIdCount-1]);
         }
      }
      handle = pPrevDescriptor->NextHandle;
   }

   TRACE(7, "Exit %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

static int32_t ism_store_memAllocGenMap(ismStore_GenId_t *pGenId)
{
   ismStore_memGenMap_t *pGenMap, **tmp;
   ismStore_GenId_t genId;

   // See whether we have to extend the generation list
   while (ismStore_memGlobal.GensMapCount >= ismStore_memGlobal.GensMapSize || *pGenId >= ismStore_memGlobal.GensMapSize)
   {
      if (ismStore_memGlobal.GensMapSize < ismSTORE_MAX_GEN_ID + 1)
      {
         if ((tmp = (ismStore_memGenMap_t **)ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,126),ismStore_memGlobal.pGensMap, 2 * ismStore_memGlobal.GensMapSize * sizeof(ismStore_memGenMap_t *))) == NULL)
         {
            TRACE(1, "Failed to extend the array of store generation map entries due to memory allocation error. GenId %u\n", *pGenId);
            return ISMRC_AllocateError;
         }
         ismStore_memGlobal.pGensMap = tmp;
         memset(ismStore_memGlobal.pGensMap + ismStore_memGlobal.GensMapSize, '\0', ismStore_memGlobal.GensMapSize * sizeof(ismStore_memGenMap_t *));
         ismStore_memGlobal.GensMapSize *= 2;
      }
   }

   if ((genId = *pGenId) == ismSTORE_RSRV_GEN_ID)
   {
      // Search for a free generation id
      for (genId=ismSTORE_MGMT_GEN_ID + 1; genId < ismStore_memGlobal.GensMapSize && genId <= ismSTORE_MAX_GEN_ID; genId++)
      {
         if (!ismStore_memGlobal.pGensMap[genId])
         {
            break;
         }
      }

      if (genId >= ismStore_memGlobal.GensMapSize || genId > ismSTORE_MAX_GEN_ID)
      {
         TRACE(1, "Failed to add a store generation map entry (GenId %u) into the list, because the store is full\n", genId);
         return ISMRC_StoreFull;
      }
   }

   if (ismStore_memGlobal.pGensMap[genId])
   {
      if ( ismStore_memGlobal.PersistCreatedGenId == genId )
      {
         TRACE(1, "Skip creating genMap for genId %u because it was created during persistRecovery\n", genId);
         return ISMRC_OK;
      }
      else
      {
         TRACE(1, "Failed to add a store generation map entry (GenId %u) into the list, because an old entry already exist\n", genId);
         return ISMRC_Error;
      }
   }

   // Create an entry for the new generation
   if ((pGenMap = (ismStore_memGenMap_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,127),sizeof(ismStore_memGenMap_t))) == NULL)
   {
      TRACE(1, "Failed to allocate memory for the store generation map entry (GenId %u)\n", genId);
      return ISMRC_AllocateError;
   }
   memset(pGenMap, '\0', sizeof(*pGenMap));

   if (pthread_mutex_init(&pGenMap->Mutex, NULL))
   {
      TRACE(1, "Failed to initialize mutex (pGenMap->Mutex)\n");
      ismSTORE_FREE(pGenMap);
      return ISMRC_Error;
   }

   if (ism_common_cond_init_monotonic(&pGenMap->Cond))
   {
      TRACE(1, "Failed to initialize cond (pGenMap->Cond)\n");
      pthread_mutex_destroy(&pGenMap->Mutex);
      ismSTORE_FREE(pGenMap);
      return ISMRC_Error;
   }

   ismStore_memGlobal.pGensMap[genId] = pGenMap;
   ismStore_memGlobal.GensMapCount++;
   *pGenId = genId;
   TRACE(7, "A GenMap for generation Id %u has been allocated. GensMapCount %u, GensMapSize %u\n",
         genId, ismStore_memGlobal.GensMapCount, ismStore_memGlobal.GensMapSize);

   return ISMRC_OK;
}

static void ism_store_memFreeGenMap(ismStore_GenId_t genId)
{
   ismStore_memGenMap_t *pGenMap = ismStore_memGlobal.pGensMap[genId];
   uint8_t poolId, i;

   if (pGenMap)
   {
      // Release threads that are blocking on the GenMap
      pthread_mutex_lock(&pGenMap->Mutex);
      pthread_cond_broadcast(&pGenMap->Cond);
      pthread_mutex_unlock(&pGenMap->Mutex);

      for (poolId=0; poolId < pGenMap->GranulesMapCount; poolId++)
      {
         for (i=0; i < ismSTORE_BITMAPS_COUNT; i++)
         {
            ismSTORE_FREE(pGenMap->GranulesMap[poolId].pBitMap[i]);
         }
      }

      pGenMap->GranulesMapCount = 0;
      pthread_mutex_destroy(&pGenMap->Mutex);
      pthread_cond_destroy(&pGenMap->Cond);
      ismSTORE_FREE(pGenMap);
      ismStore_memGlobal.pGensMap[genId] = NULL;
      ismStore_memGlobal.GensMapCount--;
      TRACE(8, "Generation map for generation Id %u has been released. GensMapCount %u, GensMapSize %u\n",
            genId, ismStore_memGlobal.GensMapCount, ismStore_memGlobal.GensMapSize);
   }
}

int32_t ism_store_memCreateGranulesMap(
                   ismStore_memGenHeader_t *pGenHeader,
                   ismStore_memGenMap_t *pGenMap,
                   uint8_t fBuildBitMapFree)
{
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memGranulePool_t *pPool;
   ismStore_memGranulesMap_t *pGranulesMap;
   uint8_t poolId, idx;
   uint32_t recMap[7];
   uint64_t offset, tail, sizeMap[7];
   int32_t rc = ISMRC_OK, i;

   TRACE(7, "Entry %s. GenId %u, PoolsCount %d\n", __FUNCTION__, pGenHeader->GenId, pGenHeader->PoolsCount);
   pGenMap->GranulesMapCount = pGenHeader->PoolsCount;

   for (poolId=0; poolId < pGenHeader->PoolsCount; poolId++)
   {
      pPool = &pGenHeader->GranulePool[poolId];
      pGranulesMap = &pGenMap->GranulesMap[poolId];
      pGranulesMap->Offset = pPool->Offset;
      pGranulesMap->Last = pPool->Offset + pPool->MaxMemSizeBytes;
      pGranulesMap->GranuleSizeBytes = pPool->GranuleSizeBytes;
      pGranulesMap->BitMapSize = (pPool->MaxMemSizeBytes / pPool->GranuleSizeBytes + 63) / 64;

      if ((pGranulesMap->pBitMap[ismSTORE_BITMAP_LIVE_IDX] = (uint64_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,128),pGranulesMap->BitMapSize * sizeof(uint64_t))) == NULL)
      {
         TRACE(1, "Failed to allocate memory for the bitmap of generation Id %u. PoolId %u, Offset 0x%lx, Last 0x%lx, MemSizeBytes %lu, BitMapSize %u, GranuleSizeBytes %u\n",
               pGenHeader->GenId, poolId, pGranulesMap->Offset, pGranulesMap->Last, pPool->MaxMemSizeBytes, pGranulesMap->BitMapSize, pGranulesMap->GranuleSizeBytes);
         rc = ISMRC_AllocateError;
         goto exit;
      }
      memset(pGranulesMap->pBitMap[ismSTORE_BITMAP_LIVE_IDX], '\0', pGranulesMap->BitMapSize * sizeof(uint64_t));

      if (fBuildBitMapFree)
      {
         if ((pGranulesMap->pBitMap[ismSTORE_BITMAP_FREE_IDX] = (uint64_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,129),pGranulesMap->BitMapSize * sizeof(uint64_t))) == NULL)
         {
            TRACE(1, "Failed to allocate memory for the bitmapFree of generation Id %u. PoolId %u, Offset 0x%lx, Last 0x%lx, MemSizeBytes %lu, BitMapSize %u, GranuleSizeBytes %u\n",
                  pGenHeader->GenId, poolId, pGranulesMap->Offset, pGranulesMap->Last, pPool->MaxMemSizeBytes, pGranulesMap->BitMapSize, pGranulesMap->GranuleSizeBytes);
            rc = ISMRC_AllocateError;
            goto exit;
         }
         memset(pGranulesMap->pBitMap[ismSTORE_BITMAP_FREE_IDX], '\0', pGranulesMap->BitMapSize * sizeof(uint64_t));
      }

      memset(recMap, '\0', sizeof(recMap));
      memset(sizeMap, '\0', sizeof(sizeMap));
      for (offset = pPool->Offset, tail = pPool->Offset + pPool->MaxMemSizeBytes; offset < tail; offset += pPool->GranuleSizeBytes)
      {
         pDescriptor = (ismStore_memDescriptor_t *)((char *)pGenHeader + offset);
         if (ismSTORE_IS_HEADITEM(pDescriptor->DataType))
         {
            if (pDescriptor->DataType == ISM_STORE_RECTYPE_MSG) {
               idx = 0;
            } else if ((pDescriptor->DataType >= ISM_STORE_RECTYPE_PROP && pDescriptor->DataType <= ISM_STORE_RECTYPE_SPROP) || pDescriptor->DataType == ISM_STORE_RECTYPE_RPROP) {
               idx = 1;
            } else if (pDescriptor->DataType == ISM_STORE_RECTYPE_BXR) {
               idx = 2;
            } else if (pDescriptor->DataType == ismSTORE_DATATYPE_REFERENCES) {
               idx = 3;
            } else if (pDescriptor->DataType == ismSTORE_DATATYPE_LDATA_EXT) {
               idx = 4;
            } else {
               idx = 5;
            }
            recMap[idx]++;
            sizeMap[idx] += pDescriptor->TotalLength;
            // Set on the bit of this handle in the map
            ism_store_memSetGenMap(pGenMap, ismSTORE_BITMAP_LIVE_IDX, offset, 0);
         }
         else if (fBuildBitMapFree && ismSTORE_IS_FREEGRANULE(pDescriptor->DataType))
         {
            recMap[6]++;
            sizeMap[6] += pDescriptor->TotalLength;
            // Set on the bit of this free handle in the map of free granules
            ism_store_memSetGenMap(pGenMap, ismSTORE_BITMAP_FREE_IDX, offset, 0);
         }
      }

      TRACE(5, "Map of the memory pool %d of generation Id %u: MSG %u (%lu bytes), PROP %u (%lu bytes), BXR %u (%lu bytes), " \
            "REFERENCES %u (%lu bytes), LDATA_EXT %u (%lu bytes), OTHER %u (%lu bytes), FREE_GRANULE %u (%lu bytes)\n",
            poolId, pGenHeader->GenId, recMap[0], sizeMap[0],
            recMap[1], sizeMap[1], recMap[2], sizeMap[2],
            recMap[3], sizeMap[3], recMap[4], sizeMap[4],
            recMap[5], sizeMap[5], recMap[6], sizeMap[6]);

      TRACE(5, "A bitmap for generation Id %u has been created. PoolId %u, Offset 0x%lx, Last 0x%lx, MemSizeBytes %lu, BitMapSize %u, GranuleSizeBytes %u, RecordsCount %u\n",
            pGenHeader->GenId, poolId, pGranulesMap->Offset, pGranulesMap->Last, pPool->MaxMemSizeBytes, pGranulesMap->BitMapSize, pGranulesMap->GranuleSizeBytes, pGenMap->RecordsCount);
   }

   if (pGenMap->RecordsCount == 0)
   {
      TRACE(4, "There are no active records in the store generation Id %u\n", pGenHeader->GenId);
   }

exit:
   // If error occurred, release the memory of the bitmaps
   if (rc != ISMRC_OK)
   {
      for (poolId=0; poolId < pGenMap->GranulesMapCount; poolId++)
      {
         for (i=0; i < ismSTORE_BITMAPS_COUNT; i++)
         {
            ismSTORE_FREE(pGenMap->GranulesMap[poolId].pBitMap[i]);
         }
      }
   }
   TRACE(7, "Exit %s. rc %d\n", __FUNCTION__, rc);

   return rc;
}

int32_t ism_store_memCreateGeneration(uint8_t genIndex, ismStore_GenId_t genId0)
{
   int trclv, ec, i;
   uint64_t headerSizeBytes, memSizeBytes;
   uint32_t granuleSizeBytes;
   ismStore_GenId_t genId = genId0;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memGeneration_t *pGen;
   ismStore_memGranulePool_t *pPool;
   ismStore_memGenMap_t *pGenMap;
   ismStore_memJob_t job;
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry %s. GenIndex %u\n", __FUNCTION__, genIndex);

   // Sanity check: make sure that the generation Index is valid
   if (genIndex >= ismStore_memGlobal.InMemGensCount)
   {
      // We want to make sure that the trace message is written to the trace file
      for (i=0; i < 3; i++)
      {
         TRACE(1, "%s has been called with an invalid generation index (%u). InMemGensCount %u\n",
             __FUNCTION__, genIndex, ismStore_memGlobal.InMemGensCount);
         ism_common_sleep(1000000);  // 1 second
      }
   }

   pGen = &ismStore_memGlobal.InMemGens[genIndex];
   pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;

   granuleSizeBytes = ismStore_memGlobal.GranuleSizeBytes;
   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
   memSizeBytes =  ismSTORE_ROUND((pMgmtHeader->TotalMemSizeBytes - pMgmtHeader->MemSizeBytes) / pMgmtHeader->InMemGensCount, granuleSizeBytes);
   headerSizeBytes = ismSTORE_ROUNDUP(sizeof(ismStore_memGenHeader_t), granuleSizeBytes);

   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);

   if ( pGenHeader->GenId < ismStore_memGlobal.GensMapSize && ismStore_memGlobal.pGensMap[pGenHeader->GenId] )
      ismStore_memGlobal.pGensMap[pGenHeader->GenId]->pGen = NULL ; 

   // First, we mark the generation as free
   // If the server fails before the end of this function, the GenId is marked as ismSTORE_RSRV_GEN_ID
   memset(pGenHeader, '\0', sizeof(ismStore_memGenHeader_t));
   pGenHeader->GenId = ismSTORE_RSRV_GEN_ID;
   ADR_WRITE_BACK(pGenHeader, sizeof(ismStore_memGenHeader_t));

   // Allocate a new GenId and create an entry for the new generation
   if ((rc = ism_store_memAllocGenMap(&genId)) != ISMRC_OK)
   {
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
      goto exit;
   }

   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
   pGenMap = ismStore_memGlobal.pGensMap[genId];
   pGenMap->pGen = pGen ; 

   // Initialize the generation memory in the store
   memset(pGen->pBaseAddress + sizeof(ismStore_memGenHeader_t), '\0', memSizeBytes - sizeof(ismStore_memGenHeader_t));
   pGen->MaxRefsPerGranule = (ismStore_memGlobal.GranuleSizeBytes - sizeof(ismStore_memDescriptor_t) -
                              offsetof(ismStore_memReferenceChunk_t, References)) / sizeof(ismStore_memReference_t);
   pGen->HACreateSqn = pGen->HAActivateSqn = pGen->HAWriteSqn = 0;
   pGen->HACreateState = pGen->HAActivateState = pGen->HAWriteState = 0;

   pGenHeader->StrucId = ismSTORE_MEM_GENHEADER_STRUCID;
   pGenHeader->Version = ismSTORE_VERSION;
   pGenHeader->DescriptorStructSize = sizeof(ismStore_memDescriptor_t);
   pGenHeader->SplitItemStructSize = sizeof(ismStore_memSplitItem_t);
   memset((void *)&pGenHeader->GenToken, '\0', sizeof(ismStore_memGenToken_t));
   pGenHeader->MemSizeBytes = pGenMap->MemSizeBytes = memSizeBytes;
   pGenHeader->PoolsCount = 1;

   for (i=0; i < pGenHeader->PoolsCount; i++)
   {
      pPool = &pGenHeader->GranulePool[i];
      pPool->GranuleSizeBytes = pGenMap->GranulesMap[i].GranuleSizeBytes = granuleSizeBytes;
      pPool->GranuleDataSizeBytes = pPool->GranuleSizeBytes - sizeof(ismStore_memDescriptor_t);
      // Currently, we use only a single pool. The 2nd pool is reserved.
      if (i == 0)
      {
         pPool->MaxMemSizeBytes = memSizeBytes - headerSizeBytes;
         pPool->Offset = pGenMap->GranulesMap[i].Offset = headerSizeBytes;
         pGenMap->GranulesMap[i].Last = pPool->Offset + pPool->MaxMemSizeBytes;
         pGen->PoolMaxCount[i] = pPool->MaxMemSizeBytes / pPool->GranuleSizeBytes;
         pGen->PoolMaxResrv[i] = ismSTORE_MAX_GEN_RESRV_PCT * (pGen->PoolMaxCount[i] / 100);
         pGen->PoolAlertOnCount[i] = pGen->PoolMaxCount[i] * (100 - ismStore_memGlobal.GenAlertOnPct) / 100;
         pGen->PoolAlertOffCount[i] = 0; // Not used

         pGen->StreamCacheMaxCount[i] = ism_store_memGetStreamCacheCount(pGen, i);
         pGen->StreamCacheBaseCount[i] = pGen->StreamCacheMaxCount[i] / 2;
         pGen->fPoolMemAlert[i] = 0;
         ism_store_memPreparePool(genId, pGen, pPool, i, 1);
      }

      TRACE(7, "Store generation pool: GenId %u, PoolId %u, MaxMemSizeBytes %lu, GranuleSizeBytes %u, GranuleDataSizeBytes %u, " \
            "GranuleCount %u, MaxGranuleCount %u, Offset 0x%lx, AlertOnCount %u, StreamCacheMaxCount %u, StreamCacheBaseCount %u\n",
            genId, i, pPool->MaxMemSizeBytes, pPool->GranuleSizeBytes, pPool->GranuleDataSizeBytes,
            pPool->GranuleCount, pGen->PoolMaxCount[i], pPool->Offset, pGen->PoolAlertOnCount[i],
            pGen->StreamCacheMaxCount[i], pGen->StreamCacheBaseCount[i]);
   }

   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
   // Add the new GenId into the ordered list in the store
   if (genId0  == ismSTORE_RSRV_GEN_ID && (rc = ism_store_memAddGenIdToList(genId)) != ISMRC_OK)
   {
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
      goto exit;
   }

   pGenHeader->State = (pMgmtHeader->ActiveGenId == ismSTORE_RSRV_GEN_ID ? ismSTORE_GEN_STATE_ACTIVE : ismSTORE_GEN_STATE_FREE);
   pGenHeader->GenId = genId;
   ADR_WRITE_BACK(pGenHeader, sizeof(ismStore_memGenHeader_t));

   // Set the Active and NextAvailable generation Id (if needed)
   if (pMgmtHeader->ActiveGenId == ismSTORE_RSRV_GEN_ID)
   {
      pMgmtHeader->ActiveGenId = genId;
      pMgmtHeader->ActiveGenIndex = genIndex;
   }
   else if (pMgmtHeader->NextAvailableGenId == ismSTORE_RSRV_GEN_ID)
   {
      pMgmtHeader->NextAvailableGenId = genId;
   }
   ADR_WRITE_BACK(pMgmtHeader, sizeof(*pMgmtHeader));
   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

   TRACE(5, "Store generation Id %u (Index %u) has been created. MemSizeBytes %lu, PoolsCount %u, MaxRefsPerGranule %u, State %u, ActiveGenId %u (Index %u), NextAvailabileGenId %u.\n",
         pGenHeader->GenId, genIndex, pGenHeader->MemSizeBytes, pGenHeader->PoolsCount, pGen->MaxRefsPerGranule,
         pGenHeader->State, pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex, pMgmtHeader->NextAvailableGenId);

   if (ismStore_memGlobal.fEnablePersist )
   {
      if ((ec = ism_storePersist_writeGenST(1, genId, genIndex, StoreHAMsg_CreateGen)) == StoreRC_OK)
      {
         TRACE(5, "A store create generation (GenId %u, Index %u) request has been written to the persist file.\n",genId, genIndex);
      }
      else
      {
         TRACE(1, "Failed to write create generation request (Id %u, Index %u) to the persist file. error code %d\n", genId, genIndex, ec);
      }
      ism_storePersist_onGenCreate(genIndex);
   }

   // If a Standby node exists, we need to update the Standby
   if (ismSTORE_HASSTANDBY && ismStore_memGlobal.HAInfo.pIntChannel)
   {
      pGen->HACreateSqn = ismStore_memGlobal.HAInfo.pIntChannel->MsgSqn;
      if ((ec = ism_store_memHASendGenMsg(ismStore_memGlobal.HAInfo.pIntChannel, genId, genIndex, ismSTORE_BITMAP_LIVE_IDX, StoreHAMsg_CreateGen)) == StoreRC_OK)
      {
         pGen->HACreateState = 1;
         TRACE(5, "A store create generation (GenId %u, Index %u) request has been sent to the Standby node. MsgSqn %lu\n",
               genId, genIndex, pGen->HACreateSqn);
      }
      else
      {
         trclv = (ec == StoreRC_SystemError ? 1 : 5);
         TRACE(trclv, "Failed to create generation Id %u (Index %u) on the Standby node due to an HA error. error code %d\n", genId, genIndex, ec);
      }
   }

   if (pMgmtHeader->ActiveGenId == genId)
   {
      memset(&job, '\0', sizeof(job));
      job.JobType = StoreJob_ActivateGeneration;
      job.Generation.GenId = pMgmtHeader->ActiveGenId;
      job.Generation.GenIndex = pMgmtHeader->ActiveGenIndex;
      ism_store_memAddJob(&job);
   }

   TRACE(5, "Store active generation Id %u (Index %u). NextAvailableGenId %u.\n",
         pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex, pMgmtHeader->NextAvailableGenId);

exit:
   if (rc != ISMRC_OK)
   {
      ism_store_memFreeGenMap(genId);
   }

   TRACE(9, "Exit %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

static int32_t ism_store_memActivateGeneration(ismStore_GenId_t genId, uint8_t genIndex)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGeneration_t *pGen;
   ismStore_memStream_t *pStream;
   int trclv, ec, i;

   TRACE(9, "Entry %s. GenId %u, GenIndex %u\n", __FUNCTION__, genId, genIndex);

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
   if (pMgmtHeader->ActiveGenId != genId || pMgmtHeader->ActiveGenIndex != genIndex)
   {
      TRACE(1, "Failed to activate generation Id %u (Index %u), because of a mismatch. ActiveGenId %u, ActiveGenIndex %u\n",
            genId, genIndex, pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex);
      return ISMRC_Error;
   }

   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);

   pGen = &ismStore_memGlobal.InMemGens[genIndex];
   // If a Standby node exists, we need to update the Standby
   if (ismSTORE_HASSTANDBY && ismStore_memGlobal.HAInfo.pIntChannel && pGen->HAActivateState == 0)
   {
      pGen->HAActivateSqn = ismStore_memGlobal.HAInfo.pIntChannel->MsgSqn;
      if ((ec = ism_store_memHASendGenMsg(ismStore_memGlobal.HAInfo.pIntChannel, genId, genIndex, ismSTORE_BITMAP_LIVE_IDX, StoreHAMsg_ActivateGen)) == StoreRC_OK)
      {
         pGen->HAActivateState = 1;
         TRACE(5, "A store activation generation (GenId %u, Index %u) request has been sent to the Sandby node. MsgSqn %lu\n",
               genId, genIndex, pGen->HAActivateSqn);
      }
      else
      {
         trclv = (ec == StoreRC_SystemError ? 1 : 5);
         TRACE(trclv, "Failed to activate generation Id %u (Index %u) on the Standby node due to an HA error. error code %d\n", genId, genIndex, ec);
      }
   }

   if (pGen->HAActivateState != 1)
   {
      for (i=0; i < ismStore_memGlobal.StreamsSize; i++)
      {
         if ((pStream = ismStore_memGlobal.pStreams[i]) != NULL)
         {
            ismSTORE_MUTEX_LOCK(&pStream->Mutex);
            pStream->ActiveGenId = pMgmtHeader->ActiveGenId;
            pStream->ActiveGenIndex = pMgmtHeader->ActiveGenIndex;
            ismSTORE_COND_BROADCAST(&pStream->Cond);
            ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
         }
      }
      TRACE(5, "Generation Id %u (Index %u) has been activated\n", genId, genIndex);
   }

   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

   if ( ismStore_memGlobal.fEnablePersist )
   {
      if ((ec = ism_storePersist_writeGenST(1, genId, genIndex, StoreHAMsg_ActivateGen)) == StoreRC_OK)
      {
         TRACE(5, "A store activate generation (GenId %u, Index %u) request has been written to the persist file.\n",genId, genIndex);
      }
      else
      {
         TRACE(1, "Failed to write activate generation request (Id %u, Index %u) to the persist file. error code %d\n", genId, genIndex, ec);
      }
   }

   TRACE(9, "Exit %s\n", __FUNCTION__);
   return ISMRC_OK;
}

static int32_t ism_store_memDeleteGeneration(ismStore_GenId_t genId)
{
   ismStore_DiskTaskParams_t diskTask;
   int trclv, ec;
   int32_t rc = ISMRC_OK;

   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);

   if (ismStore_global.fHAEnabled &&
       ismStore_memGlobal.HAInfo.State == ismSTORE_HA_STATE_PRIMARY &&
       ismStore_memGlobal.pGensMap[genId]->HASyncState != StoreHASyncGen_Empty)
   {
      TRACE(5, "Could not delete store generation Id %u, because it is being synchronized\n", genId);
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
      return rc;
   }

   // Delete the generation id from the ordered list in the store
   ism_store_memDeleteGenIdFromList(genId);
   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

   if (ismStore_memGlobal.fEnablePersist )
   {
      if ((ec = ism_storePersist_writeGenST(1, genId, 0, StoreHAMsg_DeleteGen)) == StoreRC_OK)
      {
         TRACE(5, "A store delete generation (GenId %u) request has been written to the persist file.\n",genId);
      }
      else
      {
         TRACE(1, "Failed to write delete generation request (Id %u) to the persist file. error code %d\n", genId, ec);
      }
   }

   TRACE(5, "Store generation Id %u is being deleted from the disk\n", genId);
   memset(&diskTask, '\0', sizeof(diskTask));
   diskTask.GenId = genId;
   diskTask.Priority = 0;
   diskTask.Callback = ism_store_memDiskDeleteComplete;

   if ((ec = ism_storeDisk_deleteGeneration(&diskTask)) != StoreRC_OK)
   {
      TRACE(1, "Failed to delete generation Id %u from the disk. error code %d\n", genId, ec);
      rc = ISMRC_Error;
   }

   // If a Standby node exists, we need to update the Standby
   if (ismSTORE_HASSTANDBY && ismStore_memGlobal.HAInfo.pIntChannel)
   {
      if ((ec = ism_store_memHASendGenMsg(ismStore_memGlobal.HAInfo.pIntChannel, genId, 0, ismSTORE_BITMAP_LIVE_IDX, StoreHAMsg_DeleteGen)) == StoreRC_OK)
      {
         TRACE(5, "A store delete generation (GenId %u) request has been sent to the Sandby node. MsgSqn %lu\n",
               genId, ismStore_memGlobal.HAInfo.pIntChannel->MsgSqn - 1);
      }
      else
      {
         trclv = (ec == StoreRC_SystemError ? 1 : 5);
         TRACE(trclv, "Failed to delete generation Id %u from the Standby node due to an HA error\n", genId);
      }
   }

   TRACE(5, "Store generation Id %u has been deleted from the memory\n", genId);

   return rc;
}

static int32_t ism_store_memCloseGeneration(ismStore_GenId_t genId, uint8_t genIndex)
{
   int fGenUnused=0, i;
   ismStore_memGeneration_t *pGen;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t *pGenHeader, *pGenHeaderN;
   ismStore_memStream_t *pStream;
   ismStore_memJob_t job;
   int32_t rc = ISMRC_OK;

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
   pGen = &ismStore_memGlobal.InMemGens[genIndex];
   pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;

   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
   TRACE(8, "See whether store generation Id %u (Index %u) is ready for close. State %d, GranulesCount %u, fMemAlert %u, ActiveGenId %u, ActiveGenIndex %u\n",
         genId, genIndex, pGenHeader->State, pGenHeader->GranulePool[0].GranuleCount, pGen->fPoolMemAlert[0], pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex);

   if (pMgmtHeader->ActiveGenId == genId && pMgmtHeader->ActiveGenIndex == genIndex &&
       pGenHeader->State == ismSTORE_GEN_STATE_ACTIVE &&
       pGen->fPoolMemAlert[0])
   {
      TRACE(5, "Store generation Id %u (Index %u, State %d, GranulesCount %u (%u), fMemAlert %u) is being closed. ActiveGenId %u, ActiveGenIndex %u\n",
            genId, genIndex, pGenHeader->State, pGenHeader->GranulePool[0].GranuleCount, pGen->CoolPool[0].GranuleCount,pGen->fPoolMemAlert[0], pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex);
      pGenHeader->State = ismSTORE_GEN_STATE_CLOSE_PENDING;
      pMgmtHeader->ActiveGenId = pMgmtHeader->NextAvailableGenId;
      pMgmtHeader->ActiveGenIndex = (pMgmtHeader->ActiveGenIndex + 1) % pMgmtHeader->InMemGensCount;

      // Change the state of the new generation to active
      if (pMgmtHeader->ActiveGenId != ismSTORE_RSRV_GEN_ID)
      {
         pGenHeaderN = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[pMgmtHeader->ActiveGenIndex].pBaseAddress;
         if (pGenHeaderN->State == ismSTORE_GEN_STATE_FREE && pGenHeaderN->GenId == pMgmtHeader->ActiveGenId)
         {
            pGenHeaderN->State = ismSTORE_GEN_STATE_ACTIVE;
            ADR_WRITE_BACK(&pGenHeaderN->State, sizeof(pGenHeaderN->State));
            TRACE(5, "Store generation (GenId %u, Index %u) is now active\n", pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex);
         }
         else
         {
            TRACE(1, "Failed to set generation (GenId %u, Index %u) as an active generation. State %u, ActiveGenId %u\n",
                  pGenHeaderN->GenId, pMgmtHeader->ActiveGenIndex, pGenHeaderN->State, pMgmtHeader->ActiveGenId);
            rc = ISMRC_Error;
         }

         // Set the NextAvailableGenId
         pGenHeaderN = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[(pMgmtHeader->ActiveGenIndex + 1) % pMgmtHeader->InMemGensCount].pBaseAddress;
         pMgmtHeader->NextAvailableGenId = (pGenHeaderN->State == ismSTORE_GEN_STATE_FREE ? pGenHeaderN->GenId : ismSTORE_RSRV_GEN_ID);
      }
      ADR_WRITE_BACK(pMgmtHeader, sizeof(*pMgmtHeader));

      for (i=0, fGenUnused=1; i < ismStore_memGlobal.StreamsSize; i++)
      {
         if ((pStream = ismStore_memGlobal.pStreams[i]) != NULL)
         {
            ismSTORE_MUTEX_LOCK(&pStream->Mutex);
            pStream->ActiveGenId = (ismStore_global.fHAEnabled ? ismSTORE_RSRV_GEN_ID : pMgmtHeader->ActiveGenId);
            pStream->ActiveGenIndex = pMgmtHeader->ActiveGenIndex;
            if ( i != ismStore_memGlobal.hInternalStream && ismStore_memGlobal.fEnablePersist )
               pStream->fLocked |= 1;
            // Reset the local cache of the stream
            if (pStream->MyGenId == ismSTORE_RSRV_GEN_ID)
            {
               ism_store_memResetStreamCache(pStream, genId);
               pStream->CacheMaxGranulesCount = pGen->StreamCacheMaxCount[0];
            }
            else if (pStream->MyGenId == genId)
            {
               fGenUnused = 0;
            }
            ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
         }
      }
      ADR_WRITE_BACK(&pGenHeader->State, sizeof(pGenHeader->State));

      if ((ismStore_memGlobal.fEnablePersist || ismStore_global.fHAEnabled) && pMgmtHeader->ActiveGenId != ismSTORE_RSRV_GEN_ID)
      {
         memset(&job, '\0', sizeof(job));
         job.JobType = StoreJob_ActivateGeneration;
         job.Generation.GenId = pMgmtHeader->ActiveGenId;
         job.Generation.GenIndex = pMgmtHeader->ActiveGenIndex;
         ism_store_memAddJob(&job);
      }

      if (ismStore_memGlobal.fEnablePersist)
      {
        ism_storePersist_onGenClosed(genIndex);
      }           
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

      TRACE(5, "Store generation Id %u (Index %u, GranluesCount %u) has been closed. fGenUsed %u, ActiveGenId %u (Index %u), NextAvailableGenId %u\n",
            genId, genIndex, pGenHeader->GranulePool[0].GranuleCount, !fGenUnused, pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex, pMgmtHeader->NextAvailableGenId);
   }
   else
   {
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
   }


   if (fGenUnused)
   {
      memset(&job, '\0', sizeof(job));
      job.JobType = StoreJob_WriteGeneration;
      job.Generation.GenId = genId;
      job.Generation.GenIndex = genIndex;
      ism_store_memAddJob(&job);
   }
   return rc;
}

static int32_t ism_store_memWriteGeneration(ismStore_GenId_t genId, uint8_t genIndex)
{
   int fGenUnused=0, trclv, ec, i;
   ismStore_memGeneration_t *pGen;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memStream_t *pStream;
   ismStore_memGenMap_t *pGenMap;
   ismStore_DiskTaskParams_t diskTask;
   ismStore_memDiskWriteCtxt_t *pDiskWriteCtxt = NULL;
   double compactRatio;
   char nodeStr[40];
   uint8_t poolId;
   uint64_t *pBitMapsArray[ismSTORE_GRANULE_POOLS_COUNT];
   int32_t rc = ISMRC_OK;

   pGen = &ismStore_memGlobal.InMemGens[genIndex];
   pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;

   TRACE(8, "See whether store generation Id %u (Index %u) is ready for disk write\n", genId, genIndex);
   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
   if (pGenHeader->State == ismSTORE_GEN_STATE_CLOSE_PENDING && pGenHeader->GenId == genId)
   {
      for (i=0, fGenUnused=1; i < ismStore_memGlobal.StreamsSize && fGenUnused; i++)
      {
         if ((pStream = ismStore_memGlobal.pStreams[i]) != NULL)
         {
            ismSTORE_MUTEX_LOCK(&pStream->Mutex);
            if (pStream->MyGenId == genId)
            {
               TRACE(8, "Stream %d is still using generation Id %u (Index %u)\n", i, genId, genIndex);
               fGenUnused = 0;
            }
            ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
         }
      }

      if (fGenUnused)
      {
         do
         {
            // Set the GenToken of the generation when working in High-Availability mode
            if (ismStore_memGlobal.HAInfo.State == ismSTORE_HA_STATE_PRIMARY)
            {
               memcpy(pGenHeader->GenToken.NodeId, ismStore_memGlobal.HAInfo.View.ActiveNodeIds[0], sizeof(ismStore_HANodeID_t));
               pGenHeader->GenToken.Timestamp = ism_common_currentTimeNanos();
               ism_store_memB2H(nodeStr, (uint8_t *)pGenHeader->GenToken.NodeId, sizeof(ismStore_HANodeID_t));
            }
            else
            {
               nodeStr[0] = 0;
            }

            pGenHeader->State = ismSTORE_GEN_STATE_WRITE_PENDING;
            ADR_WRITE_BACK(pGenHeader, sizeof(*pGenHeader));
            pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

            if ((pGenMap = ismStore_memGlobal.pGensMap[genId]) == NULL)
            {
               TRACE(1, "Failed to write generation Id %u (Index %u) to the disk due to an internal error\n", genId, genIndex);
               rc = ISMRC_Error;
               break;
            }

            if ((pDiskWriteCtxt = (ismStore_memDiskWriteCtxt_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,130),sizeof(*pDiskWriteCtxt))) == NULL)
            {
               TRACE(1, "Failed to write generation Id %u (Index %u) to the disk due to a memory allocation error\n", genId, genIndex);
               rc = ISMRC_StoreAllocateError;
               break;
            }
            memset(pDiskWriteCtxt, 0, sizeof(*pDiskWriteCtxt));
            pDiskWriteCtxt->pGen = pGen;

            pthread_mutex_lock(&pGenMap->Mutex);
            if ((rc = ism_store_memCreateGranulesMap(pGenHeader, pGenMap, ismStore_global.fHAEnabled)) != ISMRC_OK)
            {
               TRACE(1, "Failed to write generation Id %u (Index %u) to the disk\n", genId, genIndex);
               ismSTORE_FREE(pDiskWriteCtxt);
               pthread_mutex_unlock(&pGenMap->Mutex);
               break;
            }

            // Compact the generation data before writing it to the disk
            memset(&diskTask, '\0', sizeof(diskTask));
            diskTask.GenId = genId;
            diskTask.Priority = 0;
            diskTask.Callback = ism_store_memDiskWriteComplete;
            diskTask.pContext = pDiskWriteCtxt;

            memset(pBitMapsArray, '\0', sizeof(pBitMapsArray));
            for (i=0; i < pGenMap->GranulesMapCount; i++) { pBitMapsArray[i] = pGenMap->GranulesMap[i].pBitMap[ismSTORE_BITMAP_LIVE_IDX]; }
            diskTask.BufferParams->pBitMaps = pBitMapsArray;
            diskTask.BufferParams->fCompactRefChunks = 1;
            if ((ec = ism_storeDisk_compactGenerationData((void *)pGenHeader, diskTask.BufferParams)) == StoreRC_OK)
            {
               compactRatio = 1.0 - (double)diskTask.BufferParams->BufferLength / pGenHeader->MemSizeBytes;
               pDiskWriteCtxt->pCompData = diskTask.BufferParams->pBuffer;
               pDiskWriteCtxt->CompDataSizeBytes = pGenMap->PredictedSizeBytes = pGenMap->HARemoteSizeBytes = diskTask.BufferParams->BufferLength;
               pGenMap->MeanRecordSizeBytes = (pGenMap->RecordsCount > 0 ? pGenMap->PredictedSizeBytes / pGenMap->RecordsCount : 0);
               pGenMap->StdDevBytes = diskTask.BufferParams->StdDev;
               TRACE(5, "Store generation Id %u (Index %u) has been compacted. PredictedSizeBytes %lu, MeanRecordSizeBytes %u, StdDevBytes %lu, CompactRatio %f\n",
                     genId, genIndex, pGenMap->PredictedSizeBytes, pGenMap->MeanRecordSizeBytes, pGenMap->StdDevBytes, compactRatio);
            }
            else
            {
               diskTask.BufferParams->pBuffer = (char *)pGenHeader;
               diskTask.BufferParams->BufferLength = pGenMap->PredictedSizeBytes = pGenMap->HARemoteSizeBytes = pGenHeader->MemSizeBytes;
               pGenMap->MeanRecordSizeBytes = (pGenMap->RecordsCount > 0 ? pGenMap->PredictedSizeBytes / pGenMap->RecordsCount : 0);
               TRACE(4, "Failed to compact generation Id %u (Index %u). PredictedSizeBytes %lu, MeanRecordSizeBytes %u, error code %d\n",
                     genId, genIndex, pGenMap->PredictedSizeBytes, pGenMap->MeanRecordSizeBytes, ec);
            }
            diskTask.BufferParams->pBitMaps = NULL;

            if (ismStore_memGlobal.fEnablePersist )
            {
               if ((ec = ism_storePersist_writeGenST(1, genId, genIndex, StoreHAMsg_WriteGen)) == StoreRC_OK)
               {
                  TRACE(5, "A store write generation (GenId %u, Index %u) request has been written to the persist file.\n",genId, genIndex);
               }
               else
               {
                  TRACE(1, "Failed to write write generation request (Id %u, Index %u) to the persist file. error code %d\n", genId, genIndex, ec);
               }
            }

            // If a Standby node exists, we need to update the Standby
            if (ismSTORE_HASSTANDBY && ismStore_memGlobal.HAInfo.pIntChannel)
            {
               if (ismStore_memGlobal.fEnablePersist )
               {
                  pthread_mutex_unlock(&pGenMap->Mutex);
                  ism_storePersist_wait4lock();
                  ism_storePersist_flushQ(1,-1) ;
                  pthread_mutex_lock(&pGenMap->Mutex); 
               }
               pGen->HAWriteSqn = ismStore_memGlobal.HAInfo.pIntChannel->MsgSqn;
               if ((ec = ism_store_memHASendGenMsg(ismStore_memGlobal.HAInfo.pIntChannel, genId, genIndex, ismSTORE_BITMAP_FREE_IDX, StoreHAMsg_WriteGen)) == StoreRC_OK)
               {
                  pGen->HAWriteState = 1;
                  TRACE(5, "A store write generation (GenId %u, Index %u) request has been sent to the Sandby node. MsgSqn %lu\n",
                        genId, genIndex, pGen->HAWriteSqn);
               }
               else
               {
                  trclv = (ec == StoreRC_SystemError ? 1 : 5);
                  TRACE(trclv, "Failed to write generation Id %u (Index %u) on the Standby disk due to an HA error. error code %d\n", genId, genIndex, ec);
               }
            }

            // Unblock all thread currently blocking on that generation
            pthread_cond_broadcast(&pGenMap->Cond);
            pthread_mutex_unlock(&pGenMap->Mutex);


            // Release the bitmap of the free granules because it is no longer needed
            for (poolId=0; poolId < pGenMap->GranulesMapCount; poolId++)
            {
               ismSTORE_FREE(pGenMap->GranulesMap[poolId].pBitMap[ismSTORE_BITMAP_FREE_IDX]);
            }

            TRACE(5, "Store generation Id %u (Index %u) is being written to the disk. GenToken %s:%lu\n",
                  genId, genIndex, nodeStr, pGenHeader->GenToken.Timestamp);
            if ((ec = ism_storeDisk_writeGeneration(&diskTask)) != StoreRC_OK)
            {
               TRACE(1, "Failed to write generation Id %u (Index %u) to the disk. error code %d\n", genId, genIndex, ec);
               ismSTORE_FREE(pDiskWriteCtxt->pCompData);
               ismSTORE_FREE(pDiskWriteCtxt);
               rc = ISMRC_Error;
               break;
            }
            if (ismStore_memGlobal.fEnablePersist )
               ism_storePersist_onGenWrite(genIndex,1) ; 
         } while (0);
      }
      else
      {
         pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
      }
   }
   else
   {
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
   }

   if (pGenHeader->State == ismSTORE_GEN_STATE_CLOSE_PENDING)
   {
      TRACE(8, "Store generation Id %u (Index %u) is not ready for disk write\n", genId, genIndex);
   }

   return rc;
}

static int32_t ism_store_memCompactGeneration(ismStore_GenId_t genId, uint8_t priority, uint8_t fForce)
{
   ismStore_memGenMap_t *pGenMap;
   ismStore_DiskTaskParams_t diskTask;
   int trclv, ec, i;
   uint64_t **pBitMaps = NULL;
   int32_t rc = ISMRC_OK;

   TRACE(9, "Entry: %s. GenId %u, Priority %u, fForce %u\n", __FUNCTION__, genId, priority, fForce);
   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);

   if ((pGenMap = ismStore_memGlobal.pGensMap[genId]) == NULL)
   {
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
      TRACE(5, "The compaction of generation Id %u has been cancelled, because the generation is no longer exist\n", genId);
      return ISMRC_Error;
   }

   pthread_mutex_lock(&pGenMap->Mutex);

   if (fForce || pGenMap->fCompactReady == 2 || ismSTORE_NEED_DISK_COMPACT )
   {
      // Allocate an array of pointers to the generation BitMaps. The memory is released by the DiskUtils component
      if ((pBitMaps = (uint64_t **)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,131),ismSTORE_GRANULE_POOLS_COUNT * sizeof(uint64_t *))) == NULL)
      {
         TRACE(1, "Failed to compact generatrion Id %u due to a memory allocation error\n", genId);
         rc = ISMRC_StoreAllocateError;
         goto exit;
      }

      memset(pBitMaps, '\0', ismSTORE_GRANULE_POOLS_COUNT * sizeof(uint64_t *));
      for (i=0; i < pGenMap->GranulesMapCount; i++)
      {
         if ((pBitMaps[i] = (uint64_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,132),pGenMap->GranulesMap[i].BitMapSize * sizeof(uint64_t))) == NULL)
         {
            TRACE(1, "Failed to compact generatrion Id %u due to a memory allocation error. PoolId %u, BitMapSize %u\n", genId, i, pGenMap->GranulesMap[i].BitMapSize);
            rc = ISMRC_StoreAllocateError;
            goto exit;
         }
         memcpy(pBitMaps[i], pGenMap->GranulesMap[i].pBitMap[ismSTORE_BITMAP_LIVE_IDX], pGenMap->GranulesMap[i].BitMapSize * sizeof(uint64_t));
      }

      memset(&diskTask, '\0', sizeof(diskTask));
      diskTask.GenId = genId;
      diskTask.Priority = priority;
      diskTask.fCancelOnTerm = 1;
      diskTask.Callback = ism_store_memDiskCompactComplete;
      diskTask.BufferParams->pBitMaps = pBitMaps;
      diskTask.BufferParams->fFreeMaps = 1;
      ec = ism_storeDisk_compactGeneration(&diskTask);
      if (ec != StoreRC_OK && ec != StoreRC_Disk_TaskExists)
      {
         TRACE(1, "Failed to compact the store generation file (GenId %u, DiskFileSize %lu, PredictedSizeBytes %lu, PrevPredictedSizeBytes %lu). error code %d\n",
               genId, pGenMap->DiskFileSize, pGenMap->PredictedSizeBytes, pGenMap->PrevPredictedSizeBytes, ec);
         rc = ISMRC_Error;
         goto exit;
      }

      TRACE(5, "Store generation Id %u is being compacted. Priority %u, fCompactReady %u, DiskFileSize %lu, PredictedSizeBytes %lu, PrevPredictedSizeBytes %lu, HARemoteSizeBytes %lu, GranulesMapCount %u, RecordsCount %u, DelRecordsCount %u, StdDevBytes %lu, MeanRecordSizeBytes %u, return code %d\n",
            genId, priority, pGenMap->fCompactReady, pGenMap->DiskFileSize,
            pGenMap->PredictedSizeBytes, pGenMap->PrevPredictedSizeBytes,
            pGenMap->HARemoteSizeBytes, pGenMap->GranulesMapCount, pGenMap->RecordsCount,
            pGenMap->DelRecordsCount, pGenMap->StdDevBytes, pGenMap->MeanRecordSizeBytes, ec);

      pGenMap->PrevPredictedSizeBytes = pGenMap->PredictedSizeBytes;
      pGenMap->RecordsCount -= pGenMap->DelRecordsCount;
      pGenMap->DelRecordsCount = 0;
   }

exit:
   // If a Standby node exists, we need to update the Standby
   if (ismSTORE_HASSTANDBY && ismStore_memGlobal.HAInfo.pIntChannel)
   {
      if ((ec = ism_store_memHASendGenMsg(ismStore_memGlobal.HAInfo.pIntChannel, genId, 0, ismSTORE_BITMAP_LIVE_IDX, StoreHAMsg_CompactGen)) == StoreRC_OK)
      {
         TRACE(5, "A store compact generation (GenId %u) request has been sent to the Sandby node. DiskFileSize %lu, PredictedSizeBytes %lu, PrevPredictedSizeBytes %lu, HARemoteSizeBytes %lu, MsgSqn %lu\n",
               genId, pGenMap->DiskFileSize, pGenMap->PredictedSizeBytes, pGenMap->PrevPredictedSizeBytes,
               pGenMap->HARemoteSizeBytes, ismStore_memGlobal.HAInfo.pIntChannel->MsgSqn - 1);
         pGenMap->HARemoteSizeBytes = pGenMap->PredictedSizeBytes;
      }
      else
      {
         trclv = (ec == StoreRC_SystemError ? 1 : 5);
         TRACE(trclv, "Failed to compact generation Id %u on the Standby node due to an HA error. error code %d\n", genId, ec);
      }
   }

   pGenMap->fCompactReady = 0;
   pthread_mutex_unlock(&pGenMap->Mutex);
   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

   if (rc != ISMRC_OK && pBitMaps)
   {
      for (i=0; i < ismSTORE_GRANULE_POOLS_COUNT; i++) { ismSTORE_FREE(pBitMaps[i]); }
      ismSTORE_FREE(pBitMaps);
   }

   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}

// Sort the array of generations in descending order
static int ism_store_memCompactCompar(const void *pElm1, const void *pElm2)
{
   const ismStore_memCompactGen_t *pCompactGen1 = (ismStore_memCompactGen_t *)pElm1;
   const ismStore_memCompactGen_t *pCompactGen2 = (ismStore_memCompactGen_t *)pElm2;

   if (pCompactGen1->fDelete && !pCompactGen2->fDelete) return -1;
   if (!pCompactGen1->fDelete && pCompactGen2->fDelete) return 1;

   if (pCompactGen1->ExpectedFreeBytes > pCompactGen2->ExpectedFreeBytes) return -1;
   if (pCompactGen1->ExpectedFreeBytes < pCompactGen2->ExpectedFreeBytes) return 1;

   return 0;
}

static void ism_store_memCompactTopNGens(int topN, uint8_t priority, uint64_t diskUsedSpaceBytes)
{
   ismStore_memGenMap_t *pGenMap;
   ismStore_memCompactGen_t *pCompactGens=NULL;
   int gensCount, compactGensCount, tasksCount, i;

   TRACE(9, "Entry: %s\n", __FUNCTION__);

   tasksCount = ism_storeDisk_compactTasksCount(priority);
   TRACE(5, "Trying to compact the TopN generations. TopN %d, TasksCount %d, Priority %u, GensMapCount %u, DiskUsedSpaceBytes %lu\n",
         topN, tasksCount, priority, ismStore_memGlobal.GensMapCount, diskUsedSpaceBytes);

   if (tasksCount >= topN || ismStore_memGlobal.GensMapCount <= 2)
   {
      goto exit;
   }

   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
   if ((pCompactGens = (ismStore_memCompactGen_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,133),(ismStore_memGlobal.GensMapCount - 2) * sizeof(ismStore_memCompactGen_t))) == NULL)
   {
      TRACE(1, "Failed to compact the TopN generations due to a memory allocation error. Priority %u, TopN %u, GensMapCount %u\n",
            priority, topN, ismStore_memGlobal.GensMapCount);
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
      goto exit;
   }
   memset(pCompactGens, '\0', (ismStore_memGlobal.GensMapCount - 2) * sizeof(ismStore_memCompactGen_t));

   for (i=ismSTORE_MGMT_GEN_ID+1, gensCount=2, compactGensCount=0; i < ismStore_memGlobal.GensMapSize && gensCount < ismStore_memGlobal.GensMapCount; i++)
   {
      if ((pGenMap = ismStore_memGlobal.pGensMap[i]) == NULL) { continue; }

      if (pGenMap->RecordsCount > 0 && pGenMap->DiskFileSize > 0)
      {
         pCompactGens[compactGensCount].GenId = i;
         pCompactGens[compactGensCount].fDelete = (pGenMap->RecordsCount == pGenMap->DelRecordsCount ? 1 : 0);
         pCompactGens[compactGensCount].ExpectedFreeBytes = (pGenMap->DiskFileSize - pGenMap->PredictedSizeBytes) + (pGenMap->DelRecordsCount * pGenMap->StdDevBytes);
         if (pCompactGens[compactGensCount].fDelete || pCompactGens[compactGensCount].ExpectedFreeBytes > 0)
         {
            TRACE(7, "CompactTopNGens: Generation Id %u has been added to the list. Index %u, RecordsCount %u, DelRecordsCount %u, StdDevBytes %lu, PrevPredictedSizeBytes %lu, PredictedSizeBytes %lu, DiskFileSize %lu\n",
                  i, compactGensCount, pGenMap->RecordsCount, pGenMap->DelRecordsCount, pGenMap->StdDevBytes,
                  pGenMap->PrevPredictedSizeBytes, pGenMap->PredictedSizeBytes, pGenMap->DiskFileSize);
            compactGensCount++;
         }
         else
         {
            TRACE(7, "CompactTopNGens: Generation Id %u has no deleted records. RecordsCount %u, DelRecordsCount %u, PrevPredictedSizeBytes %lu, PredictedSizeBytes %lu, DiskFileSize %lu\n",
                  i, pGenMap->RecordsCount, pGenMap->DelRecordsCount, pGenMap->PrevPredictedSizeBytes,
                  pGenMap->PredictedSizeBytes, pGenMap->DiskFileSize);
         }
      }
      gensCount++;
   }
   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

   TRACE(5, "CompactTopNGens: CompactGensCount %u\n", compactGensCount);

   if (compactGensCount > 0)
   {
      // Sort the array of generations in descending order
      qsort(pCompactGens, compactGensCount, sizeof(ismStore_memCompactGen_t), ism_store_memCompactCompar);
      for (i=0; i < compactGensCount && tasksCount < topN; i++)
      {
         TRACE(5, "CompactTopNGens: Order %d, GenId %u, ExpectedFreeBytes %lu, fDelete %u, TasksCount %d\n",
               i, pCompactGens[i].GenId, pCompactGens[i].ExpectedFreeBytes, pCompactGens[i].fDelete, tasksCount);

         if (pCompactGens[i].fDelete)
         {
            ism_store_memDeleteGeneration(pCompactGens[i].GenId);
            continue;
         }

         if (pCompactGens[i].ExpectedFreeBytes == 0) { break; }

         ism_store_memCompactGeneration(pCompactGens[i].GenId, priority, 1);
         tasksCount = ism_storeDisk_compactTasksCount(priority);
      }
   }

exit:
   ismSTORE_FREE(pCompactGens);
   TRACE(9, "Exit: %s\n", __FUNCTION__);
}

static int32_t ism_store_memEnsureGenIdAllocation(ismStore_memDescriptor_t **pDesc)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memDescriptor_t *pDescriptor, *pTailDescriptor=NULL;
   ismStore_memGenIdChunk_t *pGenIdChunk = NULL;
   ismStore_Handle_t handle;
   uint32_t genIdsPerChunk;
   int32_t rc = ISMRC_OK;

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
   // Find the last generation id chunk
   if (pMgmtHeader->GenIdHandle)
   {
      pTailDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pMgmtHeader->GenIdHandle);
      while (pTailDescriptor->NextHandle)
      {
         pTailDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pTailDescriptor->NextHandle);
      }
      pGenIdChunk = (ismStore_memGenIdChunk_t *)(pTailDescriptor + 1);
      genIdsPerChunk = (pTailDescriptor->DataLength - sizeof(ismStore_memGenIdChunk_t)) / sizeof(ismStore_GenId_t) + 1;
   }

   if (pGenIdChunk == NULL || pGenIdChunk->GenIdCount >= genIdsPerChunk)
   {
      rc = ism_store_memGetMgmtPoolElements(NULL,
                                            ismSTORE_MGMT_POOL_INDEX,
                                            ismSTORE_DATATYPE_GEN_IDS,
                                            0, 0, ismSTORE_SINGLE_GRANULE,
                                            &handle, &pDescriptor);
      if (rc == ISMRC_OK)
      {
         // Initialize the generation ids chunk
         pGenIdChunk = (ismStore_memGenIdChunk_t *)(pDescriptor + 1);
         pGenIdChunk->GenIdCount = 0;

         if (pTailDescriptor)
         {
            pDescriptor->DataType = ismSTORE_DATATYPE_GEN_IDS | ismSTORE_DATATYPE_NOT_PRIMARY;
            pTailDescriptor->NextHandle = handle;
            ADR_WRITE_BACK(&pTailDescriptor->NextHandle, sizeof(pTailDescriptor->NextHandle));
         }
         else
         {
            pDescriptor->DataType = ismSTORE_DATATYPE_GEN_IDS;
            pMgmtHeader->GenIdHandle = handle;
            ADR_WRITE_BACK(&pMgmtHeader->GenIdHandle, sizeof(pMgmtHeader->GenIdHandle));
         }

         // Finally mark the data type so that restart recovery will trust it
         ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor) + sizeof(*pGenIdChunk));

         pTailDescriptor = pDescriptor;
      }
   }

   if (pDesc)
   {
      *pDesc = pTailDescriptor;
   }

   return rc;
}

static uint8_t ism_store_memOffset2PoolId(ismStore_memGenMap_t *pGenMap, ismStore_Handle_t offset)
{
   uint8_t poolId, mapsCount;

   if (offset < pGenMap->GranulesMap[0].Last)
   {
      return 0;
   }
   else
   {
      // mapsCount is used to be BEAM friendly
      mapsCount = (pGenMap->GranulesMapCount < ismSTORE_GRANULE_POOLS_COUNT ? pGenMap->GranulesMapCount : ismSTORE_GRANULE_POOLS_COUNT);
      for (poolId=1; poolId < mapsCount; poolId++)
      {
         if (offset < pGenMap->GranulesMap[poolId].Last)
         {
            return poolId;
         }
      }
   }

   TRACE(1, "The offset 0x%lx is not valid. GranulesMapCount %d, Last[0] 0x%lx\n",
         offset, pGenMap->GranulesMapCount, pGenMap->GranulesMap[0].Last);
   return 0;
}

void ism_store_memSetGenMap(ismStore_memGenMap_t *pGenMap, uint8_t bitMapIndex, ismStore_Handle_t offset, uint32_t dataLength)
{
   uint32_t idx, pos;
   uint8_t poolId, bitpos;
   uint64_t *pBitMap;

   if (pGenMap->GranulesMapCount == 0)
   {
      return;
   }

   poolId = ism_store_memOffset2PoolId(pGenMap, offset);
   idx = (offset - pGenMap->GranulesMap[poolId].Offset) / pGenMap->GranulesMap[poolId].GranuleSizeBytes;
   pos = idx >> 6;
   bitpos = (uint8_t)(idx & 0x3f);
   pBitMap = pGenMap->GranulesMap[poolId].pBitMap[bitMapIndex];

   if (!(pBitMap[pos] & ismStore_GenMapSetMask[bitpos]))
   {
      pBitMap[pos] |= ismStore_GenMapSetMask[bitpos];
      if (bitMapIndex == ismSTORE_BITMAP_LIVE_IDX)
      {
         pGenMap->RecordsCount++;
         pGenMap->PredictedSizeBytes += dataLength;
      }
   }
}

void ism_store_memResetGenMap(ismStore_Handle_t handle)
{
   uint8_t poolId, bitpos;
   uint32_t idx, pos;
   uint64_t *pBitMap;
   ismStore_GenId_t genId = ismSTORE_EXTRACT_GENID(handle);
   ismStore_Handle_t offset = ismSTORE_EXTRACT_OFFSET(handle);
   ismStore_memGenMap_t *pGenMap;
   struct timespec reltime={0,100000000};
   ism_time_t timeout;

   if (genId <= ismSTORE_MGMT_GEN_ID || offset == 0)
   {
      TRACE(1, "Failed to reset a GenMap because the handle 0x%lx is not valid\n", handle);
      return;
   }

   pGenMap = ismStore_memGlobal.pGensMap[genId];
   if (pGenMap == NULL)
   {
      TRACE(1, "Failed to reset a GenMap because the generation (GenId %u) does not exist. handle 0x%lx\n", genId, handle);
      return;
   }

   pthread_mutex_lock(&pGenMap->Mutex);
   if (pGenMap->GranulesMapCount == 0)
   {
      timeout = ism_common_monotonicTimeNanos() + (30 * 1000000000L);
      do
      {
         if (ism_store_memGetGenerationById(genId) == NULL)
         {
            if (!pGenMap->fBitmapWarn)
            {
               TRACE(1, "Failed to reset a GenMap because the generation (GenId %u) is not in the memory and has no bitmap\n", genId);
               pGenMap->fBitmapWarn = 1;
            }
            pthread_mutex_unlock(&pGenMap->Mutex);
            return;
         }
         ism_common_cond_timedwait(&pGenMap->Cond, &pGenMap->Mutex, &reltime, 1);
      }
      while (pGenMap->GranulesMapCount == 0 && timeout > ism_common_monotonicTimeNanos() && ismStore_memGlobal.State == ismSTORE_STATE_ACTIVE);
   }

   if (pGenMap->GranulesMapCount > 0 && ismStore_memGlobal.State == ismSTORE_STATE_ACTIVE)
   {
      poolId = ism_store_memOffset2PoolId(pGenMap, offset);
      idx = (offset - pGenMap->GranulesMap[poolId].Offset) / pGenMap->GranulesMap[poolId].GranuleSizeBytes;
      pos = idx >> 6;
      bitpos = (uint8_t)(idx & 0x3f);
      pBitMap = pGenMap->GranulesMap[poolId].pBitMap[ismSTORE_BITMAP_LIVE_IDX];

      if (pBitMap[pos] & ismStore_GenMapSetMask[bitpos])
      {
         pBitMap[pos] &= ismStore_GenMapResetMask[bitpos];
         pGenMap->DelRecordsCount++;
         pGenMap->PredictedSizeBytes -= pGenMap->MeanRecordSizeBytes;
         ism_store_memCheckCompactThreshold(genId, pGenMap);
      }
   }
   else
   {
      TRACE(1, "Failed to reset a GenMap because the generation (GenId %u) has no bitmap. handle 0x%lx\n", genId, handle);
   }
   pthread_mutex_unlock(&pGenMap->Mutex);
}

void ism_store_memPreparePool(ismStore_GenId_t genId, ismStore_memGeneration_t *pGen, ismStore_memGranulePool_t *pPool, uint8_t poolId, uint8_t fNew)
{
   ismStore_memDescriptor_t *pDescriptor, *pPrevDesc=NULL, *pOwnerDescriptor;
   ismStore_memSplitItem_t *pSplit;
   ismStore_memReferenceChunk_t *pRefChunk;
   ismStore_Handle_t hHead, hTail, offset, tail;
   ismStore_memGranulePool_t *cPool;

   hHead = hTail = ismSTORE_NULL_HANDLE;
   pPool->GranuleCount = 0;

   for (offset = pPool->Offset, tail = pPool->Offset + pPool->MaxMemSizeBytes; offset < tail; offset += pPool->GranuleSizeBytes)
   {
      pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + offset);

      // See whether the RefChunk is orphan
      if (!fNew && ((pDescriptor->DataType & (~ismSTORE_DATATYPE_NOT_PRIMARY)) == ismSTORE_DATATYPE_REFERENCES) && genId != ismSTORE_MGMT_GEN_ID)
      {
         pRefChunk = (ismStore_memReferenceChunk_t *)(pDescriptor + 1);
         pOwnerDescriptor = (ismStore_memDescriptor_t *)(ismStore_memGlobal.MgmtGen.pBaseAddress + ismSTORE_EXTRACT_OFFSET(pRefChunk->OwnerHandle));
         pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);
         if (pRefChunk->OwnerHandle == ismSTORE_NULL_HANDLE || !ismSTORE_IS_SPLITITEM(pOwnerDescriptor->DataType) || pSplit->Version != pRefChunk->OwnerVersion)
         {
            TRACE(8, "The ReferenceChunk (Handle 0x%lx, DataType 0x%x, OwnerVersion %u, BaseOrderId %lu) of owner (Handle 0x%lx, DataType 0x%x, Version %u) is orphan\n",
                  ismSTORE_BUILD_HANDLE(genId, offset), pDescriptor->DataType, pRefChunk->OwnerVersion,
                  pRefChunk->BaseOrderId, pRefChunk->OwnerHandle, pOwnerDescriptor->DataType, pSplit->Version);
            pDescriptor->DataType = ismSTORE_DATATYPE_FREE_GRANULE;
         }
      }

      if (fNew || ismSTORE_IS_FREEGRANULE(pDescriptor->DataType))
      {
         memset((char *)pDescriptor, '\0', (fNew || genId != ismSTORE_MGMT_GEN_ID || poolId != ismSTORE_MGMT_SMALL_POOL_INDEX ? pPool->GranuleSizeBytes : sizeof(*pDescriptor)));
         pDescriptor->DataType = ismSTORE_DATATYPE_FREE_GRANULE;
         pDescriptor->PoolId = poolId;
         pDescriptor->DataLength = 0;
         pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
         hTail = ismSTORE_BUILD_HANDLE(genId, offset);
         if (hHead == ismSTORE_NULL_HANDLE)
         {
            hHead = hTail;
         }
         if (pPrevDesc)
         {
            pPrevDesc->NextHandle = hTail;
            ADR_WRITE_BACK(pPrevDesc, sizeof(*pPrevDesc));
         }
         pPrevDesc = pDescriptor;
         pPool->GranuleCount++;
      }
   }

   if (pPrevDesc)
   {
      ADR_WRITE_BACK(pPrevDesc, sizeof(*pPrevDesc));
   }

   pPool->hHead = hHead;
   pPool->hTail = hTail;
   ADR_WRITE_BACK(pPool, sizeof(*pPool));
   cPool = pGen->CoolPool+poolId ; 
   memcpy(cPool, pPool, sizeof(*cPool));
   cPool->hHead = cPool->hTail = ismSTORE_NULL_HANDLE;
   cPool->GranuleCount = 0;
}

static void ism_store_memAssignRsrvPool(uint8_t poolId)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGeneration_t *pGen;
   ismStore_memJob_t job;
   ismStore_memGranulePool_t *pPool;
   uint32_t ownerGranulesLimit;

   TRACE(9, "Entry: %s. PoolId %u\n", __FUNCTION__, poolId);
   pthread_mutex_lock(&ismStore_memGlobal.RsrvPoolMutex);
   pGen = &ismStore_memGlobal.MgmtGen;
   pMgmtHeader = (ismStore_memMgmtHeader_t *)pGen->pBaseAddress;

   // Sanity check
   if (ismStore_memGlobal.RsrvPoolId < ismSTORE_GRANULE_POOLS_COUNT || pMgmtHeader->RsrvPoolMemSizeBytes == 0 || ismStore_memGlobal.RsrvPool.MaxMemSizeBytes > 0 || ismStore_memGlobal.RsrvPoolState > 0)
   {
      TRACE(4, "Could not assign the management reserved pool for pool Id %u, because the reserved pool is already assigned. RsrvPoolId %u, RsrvPoolMemSizeBytes %lu (%lu), RsrvPoolState %u\n",
            poolId, ismStore_memGlobal.RsrvPoolId, ismStore_memGlobal.RsrvPool.MaxMemSizeBytes, pMgmtHeader->RsrvPoolMemSizeBytes, ismStore_memGlobal.RsrvPoolState);
   }
   else
   {
      ismStore_memGlobal.RsrvPoolId = poolId;
      ismStore_memGlobal.RsrvPoolState = 1;    /* RsrvPool is assigned */
      TRACE(5, "Store management reserved pool is assigned to pool Id %u\n", poolId);

      if (poolId != ismSTORE_MGMT_SMALL_POOL_INDEX)
      {
         // Re-calculates the OwnerGranulesLimit, because the inital assumption was
         // that the reserved pool will be assigned to the ismSTORE_MGMT_SMALL_POOL_INDEX
         ownerGranulesLimit = ismStore_memGlobal.OwnerGranulesLimit; 
         pPool = &pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX];
         ismStore_memGlobal.OwnerGranulesLimit = ((pPool->MaxMemSizeBytes) / pPool->GranuleSizeBytes) * ismStore_memGlobal.OwnerLimitPct / 100;
         TRACE(5, "Store owner granules limit was recalculated. OwnerGranulesLimit %u (%u), OwnersCount %u\n",
               ismStore_memGlobal.OwnerGranulesLimit, ownerGranulesLimit, ismStore_memGlobal.OwnerCount[0]);
               
         if (ismStore_memGlobal.OwnerCount[0] > ismStore_memGlobal.OwnerGranulesLimit)
         {
            TRACE(4, "The total number of active owners (%u) exceeds the limit (%u), due to the new assignment of the reserved pool\n",
                  ismStore_memGlobal.OwnerCount[0], ismStore_memGlobal.OwnerGranulesLimit);
         }
      }

      // Place a job for the Store thread to do the actual job
      memset(&job, '\0', sizeof(job));
      job.JobType = StoreJob_InitRsrvPool;
      job.Generation.GenId = ismSTORE_MGMT_GEN_ID;
      ism_store_memAddJob(&job);
   }

   pthread_mutex_unlock(&ismStore_memGlobal.RsrvPoolMutex);
   TRACE(9, "Exit: %s\n", __FUNCTION__);
}

static void ism_store_memInitRsrvPool(void)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGeneration_t *pGen;
   ismStore_memGranulePool_t *pRsrvPool, *pPool;
   int ec;

   TRACE(9, "Entry: %s. RsrvPoolId %u\n", __FUNCTION__, ismStore_memGlobal.RsrvPoolId);

   do
   {
      pGen = &ismStore_memGlobal.MgmtGen;
      pMgmtHeader = (ismStore_memMgmtHeader_t *)pGen->pBaseAddress;

      pthread_mutex_lock(&ismStore_memGlobal.RsrvPoolMutex);
      // Sanity check
      if (ismStore_memGlobal.RsrvPoolId >= ismSTORE_GRANULE_POOLS_COUNT || pMgmtHeader->RsrvPoolMemSizeBytes == 0 || ismStore_memGlobal.RsrvPool.MaxMemSizeBytes > 0 || ismStore_memGlobal.RsrvPoolState != 1)
      {
         TRACE(3, "Failed to initialize the management reserved pool due to an internal error. PoolId %u, RsrvPoolMemSizeBytes %lu (%lu), RsrvPoolState %u\n",
               ismStore_memGlobal.RsrvPoolId, ismStore_memGlobal.RsrvPool.MaxMemSizeBytes, pMgmtHeader->RsrvPoolMemSizeBytes, ismStore_memGlobal.RsrvPoolState);
         pthread_mutex_unlock(&ismStore_memGlobal.RsrvPoolMutex);
         break;
      }

      pPool = &pMgmtHeader->GranulePool[ismStore_memGlobal.RsrvPoolId];
      pRsrvPool = &ismStore_memGlobal.RsrvPool;
      memset(pRsrvPool, '\0', sizeof(*pRsrvPool));
      pRsrvPool->GranuleSizeBytes = pPool->GranuleSizeBytes;
      pRsrvPool->MaxMemSizeBytes = ismSTORE_ROUND(pMgmtHeader->RsrvPoolMemSizeBytes, pPool->GranuleSizeBytes);
      pRsrvPool->Offset = (ismStore_memGlobal.RsrvPoolId == ismSTORE_MGMT_SMALL_POOL_INDEX ? pPool->Offset + pPool->MaxMemSizeBytes : pPool->Offset - pRsrvPool->MaxMemSizeBytes);
      ismStore_memGlobal.RsrvPoolState = 3;  /* RsrvPool is initialized */

      // We have to mark the reserved pool as assigned in the management generation
      pMgmtHeader->RsrvPoolMemSizeBytes = 0;
      ADR_WRITE_BACK(&pMgmtHeader->RsrvPoolMemSizeBytes, sizeof(pMgmtHeader->RsrvPoolMemSizeBytes));

      if (ismStore_memGlobal.fEnablePersist )
      {
         if ((ec = ism_storePersist_writeGenST(1, ismSTORE_MGMT_GEN_ID, 0, StoreHAMsg_AssignRsrvPool)) == StoreRC_OK)
         {
            TRACE(5, "A store AssignRsrvPool request has been written to the persist file.\n");
         }
         else
         {
            TRACE(1, "Failed to write AssignRsrvPool request to the persist file. error code %d\n", ec);
         }
      }

      // If a Standby node exists, we need to update the Standby
      if (ismSTORE_HASSTANDBY)
      {
         if (pHAInfo->pIntChannel)
         {
            ismStore_memGlobal.RsrvPoolHASqn = pHAInfo->pIntChannel->MsgSqn;
            if ((ec = ism_store_memHASendGenMsg(pHAInfo->pIntChannel, ismSTORE_MGMT_GEN_ID, 0, ismSTORE_BITMAP_LIVE_IDX, StoreHAMsg_AssignRsrvPool)) == StoreRC_OK)
            {
               TRACE(5, "A store assign management reserved pool (PoolId %u) request has been sent to the Sandby node. MsgSqn %lu\n", ismStore_memGlobal.RsrvPoolId, ismStore_memGlobal.RsrvPoolHASqn);
               ismStore_memGlobal.RsrvPoolState = 2;  /* RsrvPool was sent to the Standby node */
            }
            else
            {
               TRACE(1, "Failed to send an assign management reserved pool (PoolId %u) request to the Standby node due to an HA error. error code %d\n", ismStore_memGlobal.RsrvPoolId, ec);
               ismStore_memGlobal.RsrvPoolHASqn = 0;
            }
         }
      }
      pthread_mutex_unlock(&ismStore_memGlobal.RsrvPoolMutex);

      ism_store_memPreparePool(ismSTORE_MGMT_GEN_ID, &ismStore_memGlobal.MgmtGen, pRsrvPool, ismStore_memGlobal.RsrvPoolId, 1);
      TRACE(5, "Store management reserved pool has been initialized. PoolId %u, RsrvPoolState %u, Offset %lu, MaxMemSizeBytes %lu, GranuleSizeBytes %u, GranuleCount %u\n",
            ismStore_memGlobal.RsrvPoolId, ismStore_memGlobal.RsrvPoolState, pRsrvPool->Offset, pRsrvPool->MaxMemSizeBytes, pRsrvPool->GranuleSizeBytes, pRsrvPool->GranuleCount);

      if (ismStore_memGlobal.RsrvPoolState == 3)
      {
         ism_store_memAttachRsrvPool();
      }

   } while (0);

   TRACE(9, "Exit: %s\n", __FUNCTION__);
}

static void ism_store_memAttachRsrvPool(void)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGeneration_t *pGen;
   ismStore_memGranulePool_t *pRsrvPool, *pPool;
   ismStore_Handle_t hTail;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memJob_t job;

   TRACE(9, "Entry: %s. RsrvPoolId %u\n", __FUNCTION__, ismStore_memGlobal.RsrvPoolId);
   pGen = &ismStore_memGlobal.MgmtGen;
   pMgmtHeader = (ismStore_memMgmtHeader_t *)pGen->pBaseAddress;

   do
   {
      pthread_mutex_lock(&ismStore_memGlobal.RsrvPoolMutex);
      // Sanity check
      if (ismStore_memGlobal.RsrvPoolId >= ismSTORE_GRANULE_POOLS_COUNT || pMgmtHeader->RsrvPoolMemSizeBytes > 0 || ismStore_memGlobal.RsrvPool.MaxMemSizeBytes == 0 || ismStore_memGlobal.RsrvPoolState != 3)
      {
         TRACE(3, "Failed to attach the reserved pool due to an internal error. PoolId %u, RsrvPoolMemSizeBytes %lu (%lu), RsrvPoolState %u\n",
               ismStore_memGlobal.RsrvPoolId, ismStore_memGlobal.RsrvPool.MaxMemSizeBytes, pMgmtHeader->RsrvPoolMemSizeBytes, ismStore_memGlobal.RsrvPoolState);
         pthread_mutex_unlock(&ismStore_memGlobal.RsrvPoolMutex);
         break;
      }

      ismStore_memGlobal.RsrvPoolState = 4;  /* RsrvPool is attached */
      pthread_mutex_unlock(&ismStore_memGlobal.RsrvPoolMutex);
      TRACE(5, "Store management reserved pool is attached to pool Id %u\n", ismStore_memGlobal.RsrvPoolId);

      pPool = &pMgmtHeader->GranulePool[ismStore_memGlobal.RsrvPoolId];
      pRsrvPool = &ismStore_memGlobal.RsrvPool;
      pthread_mutex_lock(&pGen->PoolMutex[ismStore_memGlobal.RsrvPoolId]);

      if (ismStore_memGlobal.RsrvPoolId == ismSTORE_MGMT_POOL_INDEX)
      {
         pPool->Offset = pRsrvPool->Offset;
      }
      if ((hTail = pPool->hTail) == ismSTORE_NULL_HANDLE)
      {
         pPool->hHead = pRsrvPool->hHead;
      }
      pPool->hTail = pRsrvPool->hTail;
      pPool->GranuleCount += pRsrvPool->GranuleCount;
      pPool->MaxMemSizeBytes += pRsrvPool->MaxMemSizeBytes;
      ADR_WRITE_BACK(pPool, sizeof(*pPool));

      if (hTail)
      {
         pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(hTail));
         pDescriptor->NextHandle = pRsrvPool->hHead;
         ADR_WRITE_BACK(&pDescriptor->NextHandle, sizeof(pDescriptor->NextHandle));
      }

      pGen->PoolMaxCount[ismStore_memGlobal.RsrvPoolId] += pRsrvPool->GranuleCount;
      pGen->PoolAlertOnCount[ismStore_memGlobal.RsrvPoolId] += (pRsrvPool->GranuleCount * (100 - ismStore_memGlobal.MgmtAlertOnPct) / 100);
      pGen->PoolAlertOffCount[ismStore_memGlobal.RsrvPoolId] += (pRsrvPool->GranuleCount * (100 - ismStore_memGlobal.MgmtAlertOffPct) / 100);

      // See whether we have to turn off the memory alert
      if (pGen->fPoolMemAlert[ismStore_memGlobal.RsrvPoolId] && pPool->GranuleCount > pGen->PoolAlertOffCount[ismStore_memGlobal.RsrvPoolId])
      {
         pGen->fPoolMemAlert[ismStore_memGlobal.RsrvPoolId] = 0;
         TRACE(5, "Store memory pool %u of generation Id %u returned to normal capacity %u.\n",
               ismStore_memGlobal.RsrvPoolId, ismSTORE_MGMT_GEN_ID, pPool->GranuleCount);
         if (ismStore_memGlobal.OnEvent)
         {
            memset(&job, '\0', sizeof(job));
            job.JobType = StoreJob_UserEvent;
            job.Event.EventType = (ismStore_memGlobal.RsrvPoolId == ismSTORE_MGMT_SMALL_POOL_INDEX ? ISM_STORE_EVENT_MGMT0_ALERT_OFF : ISM_STORE_EVENT_MGMT1_ALERT_OFF);
            ism_store_memAddJob(&job);
         }
      }
      pthread_cond_broadcast(&ismStore_memGlobal.RsrvPoolCond);
      pthread_mutex_unlock(&pGen->PoolMutex[ismStore_memGlobal.RsrvPoolId]);

      TRACE(5, "Store pool Id %u has been extended. Offset %lu, MaxMemSizeBytes %lu, GranuleSizeBytes %u, GranuleCount %u, PoolAlertOnCount %u, PoolAlertOffCount %u, fMemAlertOn %u\n",
            ismStore_memGlobal.RsrvPoolId, pPool->Offset, pPool->MaxMemSizeBytes, pPool->GranuleSizeBytes,
            pPool->GranuleCount, pGen->PoolAlertOnCount[ismStore_memGlobal.RsrvPoolId],
            pGen->PoolAlertOffCount[ismStore_memGlobal.RsrvPoolId], pGen->fPoolMemAlert[ismStore_memGlobal.RsrvPoolId]);
   } while (0);

   TRACE(9, "Exit: %s\n", __FUNCTION__);
}

static void ism_store_memResetRsrvPool(void)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGeneration_t *pGen;
   ismStore_memGranulePool_t *pRsrvPool, *pPool;
   //ismStore_memDescriptor_t *pDescriptor;
   //ismStore_memSplitItem_t *pSplit;
   uint64_t memSizeBytes, rsrvPoolMemSizeBytes, poolMemSizeBytes[ismSTORE_GRANULE_POOLS_COUNT], tail;

   TRACE(9, "Entry: %s\n", __FUNCTION__);
   pthread_mutex_lock(&ismStore_memGlobal.RsrvPoolMutex);
   pGen = &ismStore_memGlobal.MgmtGen;
   pMgmtHeader = (ismStore_memMgmtHeader_t *)pGen->pBaseAddress;

   memSizeBytes = pMgmtHeader->MemSizeBytes - ismSTORE_ROUNDUP(sizeof(ismStore_memMgmtHeader_t), pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].GranuleSizeBytes);
   poolMemSizeBytes[ismSTORE_MGMT_SMALL_POOL_INDEX] = ismSTORE_ROUNDUP(memSizeBytes * ismSTORE_MGMT_SMALL_POOL_PCT / 100, pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].GranuleSizeBytes);
   poolMemSizeBytes[ismSTORE_MGMT_POOL_INDEX] = ismSTORE_ROUNDUP(memSizeBytes * ismSTORE_MGMT_POOL_PCT / 100, pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX].GranuleSizeBytes);
   rsrvPoolMemSizeBytes = memSizeBytes - poolMemSizeBytes[ismSTORE_MGMT_SMALL_POOL_INDEX] - poolMemSizeBytes[ismSTORE_MGMT_POOL_INDEX];

   TRACE(5, "Store management pools data: PoolId %u - MaxMemSizeBytes %lu (%lu), PoolId %u - MaxMemSizeBytes %lu (%lu), RsrvPoolMemSizeBytes %lu (%lu)\n",
         ismSTORE_MGMT_SMALL_POOL_INDEX, pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].MaxMemSizeBytes, poolMemSizeBytes[ismSTORE_MGMT_SMALL_POOL_INDEX],
         ismSTORE_MGMT_POOL_INDEX, pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX].MaxMemSizeBytes, poolMemSizeBytes[ismSTORE_MGMT_POOL_INDEX],
         pMgmtHeader->RsrvPoolMemSizeBytes, rsrvPoolMemSizeBytes);

   do
   {
      if (pMgmtHeader->RsrvPoolMemSizeBytes > 0)
      {
         TRACE(5, "Store management reserved pool is not assigned. RsrvPoolMemSizeBytes %lu\n", pMgmtHeader->RsrvPoolMemSizeBytes);
         break;
      }

      // See whether the reserved pool was not fully assigned
      if (pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].MaxMemSizeBytes + pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX].MaxMemSizeBytes + rsrvPoolMemSizeBytes <= memSizeBytes)
      {
         pMgmtHeader->RsrvPoolMemSizeBytes = rsrvPoolMemSizeBytes;
         ADR_WRITE_BACK(&pMgmtHeader->RsrvPoolMemSizeBytes, sizeof(pMgmtHeader->RsrvPoolMemSizeBytes));
         memset(pGen->pBaseAddress + pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].Offset + pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].MaxMemSizeBytes, 0, pMgmtHeader->RsrvPoolMemSizeBytes);
         TRACE(5, "Store management reserved pool has been reset, because it was not fully assigned. RsrvPoolMemSizeBytes %lu\n", pMgmtHeader->RsrvPoolMemSizeBytes);
         break;
      }

      pRsrvPool = &ismStore_memGlobal.RsrvPool;
      // Find the pool that contains the reserved pool
      if (pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].MaxMemSizeBytes > poolMemSizeBytes[ismSTORE_MGMT_SMALL_POOL_INDEX])
      {
         ismStore_memGlobal.RsrvPoolId = ismSTORE_MGMT_SMALL_POOL_INDEX;
         pPool = &pMgmtHeader->GranulePool[ismStore_memGlobal.RsrvPoolId];
         pRsrvPool->Offset = pPool->Offset + poolMemSizeBytes[ismStore_memGlobal.RsrvPoolId];
      }
      else if (pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX].MaxMemSizeBytes > poolMemSizeBytes[ismSTORE_MGMT_POOL_INDEX])
      {
         ismStore_memGlobal.RsrvPoolId = ismSTORE_MGMT_POOL_INDEX;
         pPool = &pMgmtHeader->GranulePool[ismStore_memGlobal.RsrvPoolId];
         pRsrvPool->Offset = pPool->Offset;
      }
      else
      {
         TRACE(3, "Store management reserved pool is NOT assigned properly\n");
         break;
      }

      pRsrvPool->MaxMemSizeBytes = ismSTORE_ROUND(rsrvPoolMemSizeBytes, pPool->GranuleSizeBytes);
      tail = pRsrvPool->Offset + pRsrvPool->MaxMemSizeBytes;
      TRACE(5, "Store management reserved pool is assigned to pool Id %u. Offset 0x%lx, Tail 0x%lx, MaxMemSizeBytes %lu\n",
            ismStore_memGlobal.RsrvPoolId, pRsrvPool->Offset, tail, pRsrvPool->MaxMemSizeBytes);

#if 0
      // See whether the reserved pool is assigned, but is is not used at all
      for (offset = pRsrvPool->Offset; offset < tail; offset += pPool->GranuleSizeBytes)
      {
         pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + offset);
         if (!ismSTORE_IS_FREEGRANULE(pDescriptor->DataType)) { break; }
         // If at least one granule in the reserved pool has OwnerVersion, we couldn't release the reserved pool
         if (ismStore_memGlobal.RsrvPoolId == ismSTORE_MGMT_SMALL_POOL_INDEX)
         {
            pSplit = (ismStore_memSplitItem_t *)(pDescriptor + 1);
            if (pSplit->Version > 0) { break; }
         }
      }

      if (offset >= tail)
      {
         pPool->MaxMemSizeBytes = poolMemSizeBytes[ismStore_memGlobal.RsrvPoolId];
         if (ismStore_memGlobal.RsrvPoolId == ismSTORE_MGMT_POOL_INDEX) {
            pPool->Offset = pMgmtHeader->MemSizeBytes - poolMemSizeBytes[ismSTORE_MGMT_POOL_INDEX];
         }
         ADR_WRITE_BACK(pPool, sizeof(*pPool));

         pMgmtHeader->RsrvPoolMemSizeBytes = rsrvPoolMemSizeBytes;
         ADR_WRITE_BACK(&pMgmtHeader->RsrvPoolMemSizeBytes, sizeof(pMgmtHeader->RsrvPoolMemSizeBytes));
         memset(pGen->pBaseAddress + pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].Offset + pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].MaxMemSizeBytes, pMgmtHeader->RsrvPoolMemSizeBytes, 0);
         ismStore_memGlobal.RsrvPoolId = ismSTORE_GRANULE_POOLS_COUNT;
         memset(pRsrvPool , 0, sizeof(*pRsrvPool));
         TRACE(5, "Store management reserved pool has been reset, because it is not used. RsrvPoolMemSizeBytes %lu\n", pMgmtHeader->RsrvPoolMemSizeBytes);
         break;
      }
#endif      

      ismStore_memGlobal.RsrvPoolState = 4;  /* RsrvPool is attached */
   } while (0);

   pthread_mutex_unlock(&ismStore_memGlobal.RsrvPoolMutex);
   TRACE(9, "Exit: %s\n", __FUNCTION__);
}

static int32_t ism_store_memGetMgmtPoolElements(ismStore_memStream_t *pStream,
                                                uint8_t poolId,
                                                uint16_t dataType,
                                                uint64_t attribute,
                                                uint64_t state,
                                                uint32_t dataLength,
                                                ismStore_Handle_t *pHandle,
                                                ismStore_memDescriptor_t **pDesc)
{
   int i;
   uint32_t nElements=1;
   ismStore_memGeneration_t *pGen;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memDescriptor_t *pDescriptor=NULL;
   ismStore_memGranulePool_t *pPool;
   ismStore_memJob_t job;
   ismStore_Handle_t handle = ismSTORE_NULL_HANDLE;
   int32_t rc = ISMRC_OK;

   *pHandle = ismSTORE_NULL_HANDLE;
   pGen = &ismStore_memGlobal.MgmtGen;
   pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
   pPool = &pGenHeader->GranulePool[poolId];

   if (dataLength == ismSTORE_SINGLE_GRANULE)
   {
      dataLength = pPool->GranuleDataSizeBytes;
   }
   else if (dataLength > pPool->GranuleDataSizeBytes)
   {
      nElements = (dataLength + pPool->GranuleDataSizeBytes - 1) / pPool->GranuleDataSizeBytes;
   }

   pthread_mutex_lock(&pGen->PoolMutex[poolId]);

   do
   {

      if (pPool->hHead == ismSTORE_NULL_HANDLE || pPool->GranuleCount < nElements)
      {
         // See whether the reserved pool is still free. If yes, assign it to the current pool
         if (pGenHeader->RsrvPoolMemSizeBytes > 0)
         {
            ism_store_memAssignRsrvPool(poolId);
         }

         // See whether the reserved pool is being assigned for this pool
         while (ismStore_memGlobal.RsrvPoolId == poolId && ismStore_memGlobal.RsrvPoolState < 4  && ismStore_memGlobal.State == ismSTORE_STATE_ACTIVE)
         {
            pthread_cond_wait(&ismStore_memGlobal.RsrvPoolCond, &pGen->PoolMutex[poolId]);
         }

         if (ismStore_memGlobal.fEnablePersist)
         {
            if (pPool->hHead == ismSTORE_NULL_HANDLE || pPool->GranuleCount < nElements)
            {
               if (pGen->CoolPool[poolId].GranuleCount + pPool->GranuleCount >= nElements)
               {
                  int n = 0 ; 
                  while (n++<10 && pPool->GranuleCount < nElements && pGen->CoolPool[poolId].GranuleCount + pPool->GranuleCount >= nElements)
                  {
                     pthread_mutex_unlock(&pGen->PoolMutex[poolId]);
                     su_sleep(1, SU_SEC) ; 
                     pthread_mutex_lock(&pGen->PoolMutex[poolId]);
                  } 
                  TRACE(8,"After Wait4Cool: n=%u, pPool->GranuleCount=%u, nElements=%u, pGen->CoolPool.GranuleCount=%u\n",
                           n,pPool->GranuleCount,nElements,pGen->CoolPool[poolId].GranuleCount);
               }
            }
         }

         if (pPool->hHead == ismSTORE_NULL_HANDLE || pPool->GranuleCount < nElements)
         {
            TRACE(1, "No more entries in the store memory pool of Generation %u. Head 0x%lx, Count %u, nElements %u\n",
                  pGenHeader->GenId, pPool->hHead, pPool->GranuleCount, nElements);
            rc = ISMRC_StoreFull;
            break;
         }
      }
      
      if (dataType == ISM_STORE_RECTYPE_TRANS)
      {
         /* we allow Transactions even when owner limit is exceeded */ 
         ismStore_memGlobal.OwnerCount[ismStore_T2T[dataType]] += nElements;
         ismStore_memGlobal.OwnerCount[0                     ] += nElements;
      }
      else if (ismSTORE_IS_SPLITITEM(dataType))
      {
         static uint64_t limitHitCount=0;
         if (ismStore_memGlobal.OwnerCount[0] >= ismStore_memGlobal.OwnerGranulesLimit)
         {
            if (!(limitHitCount%10000) )
            {
              if (!limitHitCount )
              {
                 TRACE(1, "No more free entries for owners. OwnerCount %u, OwnerGranulesLimit %u\n",
                       ismStore_memGlobal.OwnerCount[0], ismStore_memGlobal.OwnerGranulesLimit);
              }
              else
              {
                 TRACE(5, "The OwnerGranulesLimit (%u) was hit %lu times since the last time the limit was exceeded.\n",ismStore_memGlobal.OwnerGranulesLimit,limitHitCount);
              }
            }
            limitHitCount++;
            rc = ISMRC_StoreOwnerLimit;
            break;
         }
         else
         {
            limitHitCount = 0;
         }

         ismStore_memGlobal.OwnerCount[ismStore_T2T[dataType]] += nElements;
         ismStore_memGlobal.OwnerCount[0                     ] += nElements;
      }
      else if (dataType == ismSTORE_DATATYPE_STATES)
      {
         ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_MSG]] += nElements;
      }

      handle = pPool->hHead;
      pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(handle));
      for (i=1; i < nElements; i++)
      {
         pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(pDescriptor->NextHandle));
      }
      pPool->GranuleCount -= nElements;
      pPool->hHead = pDescriptor->NextHandle;

      if (pPool->GranuleCount <= 0)
      {
         pPool->hTail = ismSTORE_NULL_HANDLE;
      }
      // We have to WRITE_BACK the hHead, hTail and GranuleCount fields
      ADR_WRITE_BACK(&pPool->hHead, 20);
   } while (0);

   if ((pPool->GranuleCount < pGen->PoolAlertOnCount[poolId] || rc == ISMRC_StoreFull) && !pGen->fPoolMemAlert[poolId])
   {
      // Turn on the memory alert flag
      pGen->fPoolMemAlert[poolId] = 1;
      TRACE(5, "Store memory pool %u of generation Id %u reached the low capacity mark %u (count %u)\n",
            poolId, pGenHeader->GenId, pGen->PoolAlertOnCount[poolId], pPool->GranuleCount);

      if (ismStore_memGlobal.OnEvent)
      {
         memset(&job, '\0', sizeof(job));
         job.JobType = StoreJob_UserEvent;
         job.Event.EventType = (poolId == ismSTORE_MGMT_SMALL_POOL_INDEX ? ISM_STORE_EVENT_MGMT0_ALERT_ON : ISM_STORE_EVENT_MGMT1_ALERT_ON);
         ism_store_memAddJob(&job);
      }

      // See whether the reserved pool is free. If yes, assign it to the current pool
      if (pGenHeader->RsrvPoolMemSizeBytes > 0)
      {
         ism_store_memAssignRsrvPool(poolId);
      }
   }
   pthread_mutex_unlock(&pGen->PoolMutex[poolId]);

   if (rc == ISMRC_OK)
   {
      // Mark the end of the sub-list
      pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
      pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(handle));
      if (pDesc) { *pDesc = pDescriptor; }
      for (i=0; i < nElements; i++)
      {
         // Mark the granule as allocated, but commit will actually mark it correctly
         pDescriptor->DataType = (i == 0 ? ismSTORE_DATATYPE_NEWLY_HATCHED : dataType | ismSTORE_DATATYPE_NOT_PRIMARY);
         pDescriptor->Attribute = attribute;
         pDescriptor->State = state;
         pDescriptor->DataLength = (dataLength > pPool->GranuleDataSizeBytes ? pPool->GranuleDataSizeBytes : dataLength);
         pDescriptor->TotalLength = dataLength;
         dataLength -= pPool->GranuleDataSizeBytes;
         // No need to call WRITE_BACK, because the caller alwayes calls
         //ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));

         pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(pDescriptor->NextHandle));
      }
      *pHandle = handle;
   }

   return rc;
}

static int32_t ism_store_memGetPoolElements(ismStore_memStream_t *pStream,
                                            ismStore_memGeneration_t *pGen,
                                            uint8_t poolId,
                                            uint16_t dataType,
                                            uint64_t attribute,
                                            uint64_t state,
                                            uint32_t dataLength,
                                            uint32_t cacheStreamRsrv,
                                            ismStore_Handle_t *pHandle,
                                            ismStore_memDescriptor_t **pDesc)
{
   int i;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memDescriptor_t *pDescriptor=NULL, *plpDesc;
   ismStore_memGranulePool_t *pPool, *cPool;
   ismStore_Handle_t handle = ismSTORE_NULL_HANDLE, lpHandle = ismSTORE_NULL_HANDLE;
   uint8_t fMemAlert=0;
   uint32_t nElements=1, nCurrElements, cacheFillCount=0;
   int32_t rc = ISMRC_OK;

   if (pHandle) { *pHandle = ismSTORE_NULL_HANDLE; }
   pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
   pPool = &pGenHeader->GranulePool[poolId];
   cPool = &pGen->CoolPool[poolId];

   // (cacheStreamRsrv > 0) indicates that a stream reservation is required.
   // In such a case, we just need to fill the local stream cache up to cacheStreamRsrv granules.
   if (cacheStreamRsrv > 0)
   {
      if (pGen->fPoolMemAlert[poolId])
      {
         TRACE(8, "Could not reserve stream resources from the memory pool of generation %u, because it is in memory alert on. PoolCount %u (%u), RsrvCount %u, CacheGranulesCount %u\n",
               pGenHeader->GenId, pPool->GranuleCount, cPool->GranuleCount, cacheStreamRsrv, pStream->CacheGranulesCount);
         return ISMRC_StoreGenerationFull;
      }
      nElements = cacheStreamRsrv - pStream->CacheGranulesCount;
   }
   else if (dataLength == ismSTORE_SINGLE_GRANULE)
   {
      dataLength = pPool->GranuleDataSizeBytes;
   }
   else if (dataLength > pPool->GranuleDataSizeBytes)
   {
      nElements = (dataLength + pPool->GranuleDataSizeBytes - 1) / pPool->GranuleDataSizeBytes;
   }
   nCurrElements = nElements;

   // Sanity check - makes sure we don't try to allocate too many granules (more than 50% of the pool capacity)
   //if (nElements > (pGen->PoolMaxCount[poolId] >> 1))
   if (nElements > pGen->PoolMaxResrv[poolId])  // for virtual appliances we need to allow up to 60% to be reserved 
   {
      TRACE(1, "Failed to allocate granules from the pool of generation %u, because the allocation is too large. PoolMaxCount %u, Count %u, nElements %u, maxResrv %u\n",
            pGenHeader->GenId, pGen->PoolMaxCount[poolId], pPool->GranuleCount, nElements, pGen->PoolMaxResrv[poolId]);
      return ISMRC_StoreAllocateError;
   }

   // See whether there are free granules in the local cache of the stream
   if (pStream->hStoreTranHead != ismSTORE_NULL_HANDLE)
   {
      if (cacheStreamRsrv == 0)
      {
         for (handle = pStream->hCacheHead; nCurrElements > 0 && pStream->CacheGranulesCount > 0; nCurrElements--, pStream->CacheGranulesCount--)
         {
            pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(pStream->hCacheHead));
            pStream->hCacheHead = pDescriptor->NextHandle;
         }

         if (pGen->StreamCacheBaseCount[poolId] > pStream->CacheGranulesCount)
         {
            cacheFillCount = pGen->StreamCacheBaseCount[poolId] - pStream->CacheGranulesCount;
         }
      }
      else
      {
         cacheFillCount = nCurrElements;
      }
   }

   if (nCurrElements > 0)
   {
      pthread_mutex_lock(&pGen->PoolMutex[poolId]);

      if (ismStore_memGlobal.fEnablePersist)
      {
         if (pPool->hHead == ismSTORE_NULL_HANDLE || pPool->GranuleCount < nCurrElements)
         {
            if (pPool->GranuleCount + cPool->GranuleCount >= nCurrElements)
            {
               int n = 0; 
               while (n++<100 && pPool->GranuleCount < nCurrElements && pPool->GranuleCount + cPool->GranuleCount >= nCurrElements)
               {
                  pthread_mutex_unlock(&pGen->PoolMutex[poolId]);
                  su_sleep(10, SU_MIL) ; 
                  pthread_mutex_lock(&pGen->PoolMutex[poolId]);
               } 
               TRACE(8,"After Wait4Cool: n=%u, pPool->GranuleCount=%u, nElements=%u, pGen->CoolPool.GranuleCount=%u\n",
                        n,pPool->GranuleCount,nCurrElements,cPool->GranuleCount);
            }
         }
      }
      if (pPool->hHead == ismSTORE_NULL_HANDLE || pPool->GranuleCount < nCurrElements)
      {
         TRACE(1, "No more free granules in the store memory pool of generation %u. State %u, Head 0x%lx, Count %u (%u), nElements %u (%u), DataLength %u, fMemAlert %u, StreamGenId %u, CacheGranulesCount %u\n",
               pGenHeader->GenId, pGenHeader->State, pPool->hHead, pPool->GranuleCount, cPool->GranuleCount, nCurrElements, nElements, dataLength, pGen->fPoolMemAlert[poolId], pStream->MyGenId, pStream->CacheGranulesCount);
         rc = ISMRC_StoreGenerationFull;
      }
      else
      {
         if (pDescriptor)
         {
            pDescriptor->NextHandle = pPool->hHead;
         }
         else
         {
            handle = pPool->hHead;
         }
         pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(pPool->hHead));
         if (cacheStreamRsrv == 0)
         {
            for (i=1; i < nCurrElements; i++)
            {
               pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(pDescriptor->NextHandle));
            }
            pPool->GranuleCount -= nCurrElements;
            pPool->hHead = pDescriptor->NextHandle;
         }
         // If the reservation is too small, fill the cache up to the cache base count
         else if (cacheStreamRsrv < pGen->StreamCacheBaseCount[poolId] && pGen->StreamCacheBaseCount[poolId] < pPool->GranuleCount)
         {
            cacheFillCount = pGen->StreamCacheBaseCount[poolId] - pStream->CacheGranulesCount;
         }

         // Fill the local cache of the stream with free elements from the main pool
         if (cacheFillCount > 0 &&
             cacheFillCount < pPool->GranuleCount &&
             !pGen->fPoolMemAlert[poolId])
         {
            lpHandle = pPool->hHead;
            plpDesc = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(lpHandle));
            for (i=1; i < cacheFillCount; i++)
            {
               plpDesc = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(plpDesc->NextHandle));
            }
            pPool->GranuleCount -= cacheFillCount;
            pPool->hHead = plpDesc->NextHandle;
            plpDesc->NextHandle = pStream->hCacheHead;
            ADR_WRITE_BACK(&plpDesc->NextHandle, sizeof(plpDesc->NextHandle));
            pStream->hCacheHead = lpHandle;
            pStream->CacheGranulesCount += cacheFillCount;
            nCurrElements = 0;
         }

         if (pPool->GranuleCount <= 0)
         {
            pPool->hTail = ismSTORE_NULL_HANDLE;
         }
         // We have to WRITE_BACK the hHead, hTail and GranuleCount fields
         ADR_WRITE_BACK(&pPool->hHead, 20);
      }
      #if 0 
      if (ismStore_memGlobal.fEnablePersist && rc == ISMRC_OK)
      {
         if (pPool->GranuleCount < pGen->PoolAlertOnCount[poolId] && !pGen->fPoolMemAlert[poolId])
         {
            if (pPool->GranuleCount + cPool->GranuleCount >= pGen->PoolAlertOnCount[poolId])
            {
               int n = 0; 
               while (n++<100 && !pGen->fPoolMemAlert[poolId] && pPool->GranuleCount < pGen->PoolAlertOnCount[poolId] && pPool->GranuleCount + cPool->GranuleCount >= pGen->PoolAlertOnCount[poolId])
               {
                  pthread_mutex_unlock(&pGen->PoolMutex[poolId]);
                  su_sleep(1, SU_MIL) ; 
                  pthread_mutex_lock(&pGen->PoolMutex[poolId]);
               } 
               TRACE(8,"After Wait4Cool: n=%u, pPool->GranuleCount=%u, pGen->PoolAlertOnCount[poolId]=%u, pGen->CoolPool.GranuleCount=%u\n",
                        n,pPool->GranuleCount,pGen->PoolAlertOnCount[poolId],cPool->GranuleCount);
            }
         }
      }
      #endif
      if ((pPool->GranuleCount < pGen->PoolAlertOnCount[poolId] || rc != ISMRC_OK) && !pGen->fPoolMemAlert[poolId])
      {
         // Turn on the memory alert flag
         pGen->fPoolMemAlert[poolId] = 1;
         TRACE(5, "Store memory pool %u of generation Id %u reached the low capacity mark %u (count %u (%u))\n",
               poolId, pGenHeader->GenId, pGen->PoolAlertOnCount[poolId], pPool->GranuleCount, cPool->GranuleCount);
      }
      fMemAlert = pGen->fPoolMemAlert[poolId];
      pthread_mutex_unlock(&pGen->PoolMutex[poolId]);
   }

   if (rc == ISMRC_OK && cacheStreamRsrv == 0)
   {
      // Mark the end of the sub-list
      pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;  /* BEAM suppression: operating on NULL */

      pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(handle));
      // Mark the granule as allocated, but commit will actually mark it correctly
      pDescriptor->DataType = ismSTORE_DATATYPE_NEWLY_HATCHED;
      if (pDesc) { *pDesc = pDescriptor; }

      while (1)
      {
         pDescriptor->Attribute = attribute;
         pDescriptor->State = state;
         pDescriptor->DataLength = (dataLength > pPool->GranuleDataSizeBytes ? pPool->GranuleDataSizeBytes : dataLength);
         pDescriptor->TotalLength = dataLength;
         dataLength -= pPool->GranuleDataSizeBytes;
         // No need to call WRITE_BACK, because the caller alwayes calls
         //ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));

         if (pDescriptor->NextHandle == ismSTORE_NULL_HANDLE) { break; }
         pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(pDescriptor->NextHandle));
         pDescriptor->DataType = dataType | ismSTORE_DATATYPE_NOT_PRIMARY;
      }

      if (pHandle) { *pHandle = handle; }
   }

   // If the pool reached the low capacity mark, we have to close this generation
   if (fMemAlert)
   {
      ism_store_memCloseGeneration(pGenHeader->GenId, ((char *)pGen - (char *)ismStore_memGlobal.InMemGens) / sizeof(*pGen));
   }

   return rc;
}

int32_t ism_store_memReturnPoolElements(ismStore_memStream_t *pStream, ismStore_Handle_t handle, uint8_t f2Cool)
{
   ismStore_GenId_t genId;
   ismStore_memGeneration_t *pGen;
   ismStore_memGranulePool_t *pPool;
   ismStore_memDescriptor_t *pDescriptor=NULL;
   ismStore_Handle_t hElement, hTail;
   int i, dataType;
   uint8_t poolId;
   uint32_t nElements=0, nExtra;

   genId = ismSTORE_EXTRACT_GENID(handle);
   pGen = ism_store_memGetGenerationById(genId);
   pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
   if (pGen == NULL || pDescriptor == NULL)
   {
     TRACE(1, "pGen (%p) or pDescriptor (%p) are NULL for handle %p\n",pGen,pDescriptor,(void *)handle);
     return ISMRC_Error;
   }
   poolId = pDescriptor->PoolId;

   hElement=hTail=handle;
   while (1)
   {
      pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress +
                                                 ismSTORE_EXTRACT_OFFSET(hElement));
      if (pDescriptor->DataType == ismSTORE_DATATYPE_FREE_GRANULE)
      {
         TRACE(4, "The store handle 0x%lx has already been released. DataLength %u, TotalLength %u, Handle 0x%lx, NextHandle 0x%lx\n",
               hElement, pDescriptor->DataLength, pDescriptor->TotalLength, handle, pDescriptor->NextHandle);
         return ISMRC_OK;
      }
      if (nElements > 1 && pDescriptor->TotalLength == 0)
      {
         TRACE(1, "The state of the store handle 0x%lx is not valid. Owner 0x%lx, DataType 0x%x, DataLength %u, TotalLength %u, NextHandle 0x%lx\n",
               hElement, handle, pDescriptor->DataType, pDescriptor->DataLength, pDescriptor->TotalLength, pDescriptor->NextHandle);
         return ISMRC_Error;
      }
      dataType = pDescriptor->DataType;
      pDescriptor->DataType = ismSTORE_DATATYPE_FREE_GRANULE;
      pDescriptor->DataLength = 0;
      pDescriptor->TotalLength = 0;
      pDescriptor->Attribute = 0;
      pDescriptor->State = 0;
      hTail = hElement;
      nElements++;
      if ((hElement = pDescriptor->NextHandle) == ismSTORE_NULL_HANDLE)
      {
         // To minimize the number of WRITE_BACK operations, we will WRITE_BACK the
         // last element later in this function
         break;
      }
      ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));
   }

   // See whether there is a free place in the local cache of the stream
   if (pStream && genId > ismSTORE_MGMT_GEN_ID && pStream->hStoreTranHead != ismSTORE_NULL_HANDLE)
   {
      if (pStream->CacheGranulesCount + nElements <= pStream->CacheMaxGranulesCount && !pGen->fPoolMemAlert[poolId])
      {
         pDescriptor->NextHandle = pStream->hCacheHead;
         ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));
         pStream->hCacheHead = handle;
         pStream->CacheGranulesCount += nElements;
         return ISMRC_OK;
      }
      else if (pStream->CacheGranulesCount > pGen->StreamCacheBaseCount[poolId])
      {
         nExtra = pStream->CacheGranulesCount - pGen->StreamCacheBaseCount[poolId];
         pDescriptor->NextHandle = pStream->hCacheHead;
         ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));
         // Return free elements from the local cache of the stream to the main pool
         for (i=0, hElement=hTail=pStream->hCacheHead; i < nExtra; i++)
         {
           pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress +
                                                      ismSTORE_EXTRACT_OFFSET(hElement));
           hTail = hElement;
           hElement = pDescriptor->NextHandle;
         }
         pStream->hCacheHead = hElement;
         pStream->CacheGranulesCount -= nExtra;
         pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
         ADR_WRITE_BACK(&pDescriptor->NextHandle, sizeof(pDescriptor->NextHandle));
         nElements += nExtra;
      }
      else
      {
         ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));
      }
   }
   else
   {
      ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));
   }

   if (f2Cool && ismStore_memGlobal.fEnablePersist)
   {
     double now = ism_common_readTSC();
     double *v ; 
     ismStore_Handle_t h;
     for ( h=handle ; h!=ismSTORE_NULL_HANDLE ; h=pDescriptor->NextHandle )
     {
       pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(h));
       v = (double *)&pDescriptor->Attribute ; 
       *v = now ; 
     }
     pPool = &((ismStore_memGenHeader_t *)pGen->pBaseAddress)->GranulePool[poolId];
     nExtra = pPool->GranuleCount;
     pPool = &pGen->CoolPool[poolId];
   }
   else
   {
     pPool = &pGen->CoolPool[poolId];
     nExtra = pPool->GranuleCount;
     pPool = &((ismStore_memGenHeader_t *)pGen->pBaseAddress)->GranulePool[poolId];
   }

   pthread_mutex_lock(&pGen->PoolMutex[poolId]);

   if (pPool->hTail)
   {
      pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress +
                                                 ismSTORE_EXTRACT_OFFSET(pPool->hTail));
      pDescriptor->NextHandle = handle;
      ADR_WRITE_BACK(&pDescriptor->NextHandle, sizeof(pDescriptor->NextHandle));
   }
   else
   {
      pPool->hHead = handle;
   }
   pPool->hTail = hTail;
   pPool->GranuleCount += nElements;

   if (genId == ismSTORE_MGMT_GEN_ID)
   {
      if (ismSTORE_IS_SPLITITEM(dataType))
      {
         ismStore_memGlobal.OwnerCount[ismStore_T2T[dataType]] -= nElements;
         ismStore_memGlobal.OwnerCount[0                     ] -= nElements;
      }
      else if (dataType == ismSTORE_DATATYPE_STATES)
      {
         ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_MSG]] -= nElements;
      }
      if (pGen->fPoolMemAlert[poolId] &&
          pPool->GranuleCount+nExtra > pGen->PoolAlertOffCount[poolId])
      {
         pGen->fPoolMemAlert[poolId] = 0;
         TRACE(5, "Store memory pool %u of generation Id %u returned to normal capacity %u.\n",
               poolId, genId, pPool->GranuleCount+nExtra);
   
         if (ismStore_memGlobal.OnEvent)
         {
            ismStore_memJob_t job;
            memset(&job, '\0', sizeof(job));
            job.JobType = StoreJob_UserEvent;
            job.Event.EventType = (poolId == ismSTORE_MGMT_SMALL_POOL_INDEX ? ISM_STORE_EVENT_MGMT0_ALERT_OFF : ISM_STORE_EVENT_MGMT1_ALERT_OFF);
            ism_store_memAddJob(&job);
         }
      }
   }


   // We have to WRITE_BACK the hHead, hTail and GranuleCount fields
   ADR_WRITE_BACK(&pPool->hHead, 20);

   pthread_mutex_unlock(&pGen->PoolMutex[poolId]);

   return ISMRC_OK;
}

static void ism_store_memDecOwnerCount(uint16_t dataType, int nElements)
{
   ismStore_memGeneration_t *pGen;
   uint8_t poolId = ismSTORE_MGMT_SMALL_POOL_INDEX;

   pGen = &ismStore_memGlobal.MgmtGen;
   pthread_mutex_lock(&pGen->PoolMutex[poolId]);
   ismStore_memGlobal.OwnerCount[ismStore_T2T[dataType]] -= nElements;
   ismStore_memGlobal.OwnerCount[0                     ] -= nElements;
   pthread_mutex_unlock(&pGen->PoolMutex[poolId]);
}

static void ism_store_memCopyRecordData(ismStore_memDescriptor_t *pDescriptor, const ismStore_Record_t *pRecord)
{
   uint32_t dstBytesRem, srcBytesRem, bytesCount;
   char *pDst, *pSrc;
   int i;

   if (pDescriptor->TotalLength != pRecord->DataLength)
   {
      TRACE(1, "Failed to copy a record data (Type 0x%x, DataLength %u) due to internal error. TotalLength %u\n",
            pRecord->Type, pRecord->DataLength, pDescriptor->TotalLength);
      return;
   }

   bytesCount = 0;
   dstBytesRem = pDescriptor->DataLength;
   pDst = (char *)(pDescriptor + 1);
   for (i=0; i < pRecord->FragsCount && bytesCount < pRecord->DataLength; i++)
   {
      srcBytesRem = pRecord->pFragsLengths[i];
      pSrc = pRecord->pFrags[i];
      while (srcBytesRem > 0)
      {
         if (srcBytesRem < dstBytesRem)
         {
            memcpy(pDst, pSrc, srcBytesRem);
            dstBytesRem -= srcBytesRem;
            bytesCount += srcBytesRem;
            pDst += srcBytesRem;
            break;
         }
         else
         {
            memcpy(pDst, pSrc, dstBytesRem);
            ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor) + pDescriptor->DataLength);
            srcBytesRem -= dstBytesRem;
            bytesCount += dstBytesRem;
            pSrc += dstBytesRem;

            if (bytesCount >= pRecord->DataLength)
            {
               break;
            }
            // Skip to the next destination granule
            pDescriptor = ism_store_memMapHandleToAddress(pDescriptor->NextHandle);
            dstBytesRem = pDescriptor->DataLength;
            pDst = (char *)(pDescriptor + 1);
         }
      }
   }
}

static inline int ism_store_memCheckInMemGensCount(int count)
{
   if ( ismStore_memGlobal.fEnablePersist )
     return (count == 2) ; 
   return (count >0 && count <= ismSTORE_MAX_INMEM_GENS);
}
/*
 * Validates the memory for use.
 */
int32_t ism_store_memValidate(void)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   char sessionIdStr[32];
   int32_t rc = ISMRC_OK, i;

   // The header of the store is marked by a management header which describes
   // the rest of the store.
   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
   ism_store_memB2H(sessionIdStr, (uint8_t *)(pMgmtHeader->SessionId), sizeof(ismStore_HASessionID_t));

   if ((pMgmtHeader->StrucId == ismSTORE_MEM_MGMTHEADER_STRUCID) &&
       !ism_store_memCheckStoreVersion(pMgmtHeader->Version, ismSTORE_VERSION) &&
       (pMgmtHeader->GenId == ismSTORE_MGMT_GEN_ID) &&
       (pMgmtHeader->DescriptorStructSize == sizeof(ismStore_memDescriptor_t)) &&
       (pMgmtHeader->SplitItemStructSize == sizeof(ismStore_memSplitItem_t)) &&
       (pMgmtHeader->TotalMemSizeBytes == ismStore_memGlobal.TotalMemSizeBytes) &&
       (pMgmtHeader->MemSizeBytes > 0) &&
       (pMgmtHeader->MemSizeBytes <= pMgmtHeader->TotalMemSizeBytes) &&
       (pMgmtHeader->PoolsCount == ismSTORE_GRANULE_POOLS_COUNT) &&
       (pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].GranuleSizeBytes > 0) &&
       ((pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].GranuleDataSizeBytes + pMgmtHeader->DescriptorStructSize) == pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].GranuleSizeBytes) &&
       (pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX].GranuleSizeBytes > 0) &&
       ((pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX].GranuleDataSizeBytes + pMgmtHeader->DescriptorStructSize) == pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX].GranuleSizeBytes) &&
       (ism_store_memCheckInMemGensCount(pMgmtHeader->InMemGensCount)) &&
       ((ismStore_global.fHAEnabled && ismStore_memGlobal.HAInfo.StartupMode == 0) || pMgmtHeader->Role == ismSTORE_ROLE_PRIMARY || !ismStore_memGlobal.HAInfo.RoleValidation))
   {
      if ( ismStore_memGlobal.MgmtSmallGranuleSizeBytes != pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].GranuleSizeBytes )
      {
        TRACE(5, "The effective value for MgmtSmallGranuleSizeBytes has been modified from the configured value based on the old value found in the Store management header. Effective value is %u, configured value is %u.\n" ,
              pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].GranuleSizeBytes,ismStore_memGlobal.MgmtSmallGranuleSizeBytes);  
      }
      ismStore_memGlobal.MgmtSmallGranuleSizeBytes = pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].GranuleSizeBytes;
      if ( ismStore_memGlobal.MgmtGranuleSizeBytes != pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX].GranuleSizeBytes )
      {
        TRACE(5, "The effective value for MgmtGranuleSizeBytes has been modified from the configured value based on the old value found in the Store management header. Effective value is %u, configured value is %u.\n" ,
              pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX].GranuleSizeBytes,ismStore_memGlobal.MgmtGranuleSizeBytes);  
      }
      ismStore_memGlobal.MgmtGranuleSizeBytes = pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX].GranuleSizeBytes;
      ismStore_memGlobal.InMemGensCount = pMgmtHeader->InMemGensCount;

      TRACE(5, "Store validation completed. StrucId 0x%x, Version %lu, GenId %u, DescriptorStructSize %u, SplitItemStructSize %u, " \
            "TotalMemSizeBytes %lu, MemSizeBytes %lu, MgmtSmallGranuleSizeBytes %u, MgmtGranuleSizeBytes %u, PoolsCount %u, " \
            "InMemGensCount %u, Role %u, SessionId %s, SessionCount %u, HaveData %u, WasPrimary %u, ActiveGenIndex %u, ActiveGenId %u\n",
            pMgmtHeader->StrucId, pMgmtHeader->Version, pMgmtHeader->GenId,
            pMgmtHeader->DescriptorStructSize, pMgmtHeader->SplitItemStructSize,
            pMgmtHeader->TotalMemSizeBytes, pMgmtHeader->MemSizeBytes,
            ismStore_memGlobal.MgmtSmallGranuleSizeBytes, ismStore_memGlobal.MgmtGranuleSizeBytes,
            pMgmtHeader->PoolsCount, pMgmtHeader->Role, pMgmtHeader->InMemGensCount,
            sessionIdStr, pMgmtHeader->SessionCount, pMgmtHeader->HaveData, pMgmtHeader->WasPrimary, pMgmtHeader->ActiveGenIndex, pMgmtHeader->ActiveGenId);

      if ( pMgmtHeader->Version < ismSTORE_VERSION )
      {
        TRACE(5,"Store version (%lu) is updated to the newer code version (%lu)\n",pMgmtHeader->Version, (size_t)ismSTORE_VERSION);
        pMgmtHeader->Version = ismSTORE_VERSION;
        ADR_WRITE_BACK(&pMgmtHeader->Version, sizeof(pMgmtHeader->Version));
      }
      // Initialize the InMemory generations structures
      ism_store_memInitMgmtGenStruct();
      for (i=0; i < ismStore_memGlobal.InMemGensCount; i++)
      {
         ism_store_memInitGenStruct(i);
      }
   }
   else
   {
      rc = ISMRC_StoreUnrecoverable;
      if ((pMgmtHeader->StrucId == ismSTORE_MEM_MGMTHEADER_STRUCID) && 
          (ism_store_memCheckStoreVersion(pMgmtHeader->Version, ismSTORE_VERSION) == 1) )
      rc = ISMRC_StoreUnrecoverable; //TODO rc = ISMRC_StoreVersionConflict ; 
      TRACE(1, "Store validation failed. StrucId 0x%x (0x%x), Version %lu (%lu), GenId %u (%u), DescriptorStructSize %u (%lu), " \
            "SplitItemStructSize %u (%lu), TotalMemSizeBytes %lu (%lu), MemSizeBytes %lu, SmallGranuleSize %u, " \
            "SmallGranuleDataSize %u, GranuleSize %u, GranuleDataSize %u, PoolsCount %u (%u), InMemGensCount %u (%u), " \
            "Role %u (%u), SessionId %s, SessionCount %u, HaveData %u, WasPrimary %u\n",
            pMgmtHeader->StrucId, ismSTORE_MEM_MGMTHEADER_STRUCID,
            pMgmtHeader->Version, (uint64_t)ismSTORE_VERSION,
            pMgmtHeader->GenId, ismSTORE_MGMT_GEN_ID,
            pMgmtHeader->DescriptorStructSize, sizeof(ismStore_memDescriptor_t),
            pMgmtHeader->SplitItemStructSize, sizeof(ismStore_memSplitItem_t),
            pMgmtHeader->TotalMemSizeBytes, ismStore_memGlobal.TotalMemSizeBytes,
            pMgmtHeader->MemSizeBytes,
            pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].GranuleSizeBytes, pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX].GranuleDataSizeBytes,
            pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX].GranuleSizeBytes, pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX].GranuleDataSizeBytes,
            pMgmtHeader->PoolsCount, ismSTORE_GRANULE_POOLS_COUNT,
            pMgmtHeader->InMemGensCount, ismSTORE_MAX_INMEM_GENS,
            pMgmtHeader->Role, ismSTORE_ROLE_PRIMARY,
            sessionIdStr, pMgmtHeader->SessionCount,
            pMgmtHeader->HaveData, pMgmtHeader->WasPrimary);
   }

   return rc;
}

/*********************************************************************/
/* Recovery completed                                                */
/*                                                                   */
/* A recovery function to enable release of resources used during    */
/* the recovery process.                                             */
/*                                                                   */
/*                                                                   */
/*********************************************************************/
int32_t ism_store_memRecoveryCompleted(void)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memGeneration_t *pGen;
   ismStore_memGenMap_t *pGenMap;
   ismStore_memJob_t job;
   ismStore_GenId_t genId;
   int i;
   uint8_t genIndex, wait4write=0;
   int32_t rc;

   if (ismStore_memGlobal.State != ismSTORE_STATE_RECOVERY)
   {
      TRACE(1, "Failed to complete the recovery procedure, because the store is not in recovery process\n");
      rc = ISMRC_StoreNotAvailable;
      return rc;
   }

   // Release recovery resources
   if ((rc = ism_store_memRecoveryTerm()) != ISMRC_OK)
   {
      TRACE(1, "Failed to complete the recovery procedure. error code %d", ISMRC_OK);
      return rc;
   }

   // Reset the management generation token
   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
   //pMgmtHeader->HaveData = 1;  // moved to when the first owner is created
   pMgmtHeader->WasPrimary = 1;
   memset(&pMgmtHeader->GenToken, '\0', sizeof(ismStore_memGenToken_t));
   ADR_WRITE_BACK(pMgmtHeader, sizeof(*pMgmtHeader));
   memset(&ismStore_memGlobal.pGensMap[ismSTORE_MGMT_GEN_ID]->GenToken, '\0', sizeof(ismStore_memGenToken_t));

   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);

   for (i=0; i < ismStore_memGlobal.InMemGensCount; i++)
   {
      genIndex = (pMgmtHeader->ActiveGenIndex + i) % ismStore_memGlobal.InMemGensCount;
      pGen = &ismStore_memGlobal.InMemGens[genIndex];
      pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
      pGenMap = ismStore_memGlobal.pGensMap[pGenHeader->GenId];
      TRACE(5, "Current state of generation Id %u (Index %u) is %d. pGenMap %p, GranulesMapCount %u\n",
            pGenHeader->GenId, genIndex, pGenHeader->State, pGenMap, (pGenMap ? pGenMap->GranulesMapCount : 0));
      if (pGenHeader->State == ismSTORE_GEN_STATE_CLOSE_PENDING)
      {
         memset(&job, '\0', sizeof(job));
         job.JobType = StoreJob_WriteGeneration;
         job.Generation.GenId = pGenHeader->GenId;
         job.Generation.GenIndex = genIndex;
         ism_store_memAddJob(&job);
         wait4write++;
      }
      else if (pGenHeader->State == ismSTORE_GEN_STATE_WRITE_PENDING)
      {
         // See whether there is no bitmap for the specified memory generation
         // The reason for such a case is that the generation has been sent for disk write before
         // the Standby to Primary switch.
         if (pGenMap->GranulesMapCount == 0)
         {
            TRACE(5, "The memory generation (GenId %u, Index %u) has no bitmap\n", pGenHeader->GenId, genIndex);
            if ((rc = ism_store_memCreateGranulesMap(pGenHeader, pGenMap, 0)) != ISMRC_OK)
            {
               pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
               return rc;
            }
         }
         wait4write++;
      }
      else if (pGenHeader->State == ismSTORE_GEN_STATE_WRITE_COMPLETED)
      {
         // See whether there is no bitmap for the specified memory generation
         // The reason for such a case is that the generation has been sent for disk write before
         // the Standby to Primary switch.
         if (pGenMap->GranulesMapCount == 0)
         {
            TRACE(5, "The memory generation (GenId %u, Index %u) has no bitmap\n", pGenHeader->GenId, genIndex);
            if ((rc = ism_store_memCreateGranulesMap(pGenHeader, pGenMap, 0)) != ISMRC_OK)
            {
               pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
               return rc;
            }
         }
         memset(&job, '\0', sizeof(job));
         job.JobType = StoreJob_CreateGeneration;
         job.Generation.GenIndex = genIndex;
         ism_store_memAddJob(&job);
      }
   }

   if ( ismStore_memGlobal.fEnablePersist && ism_storePersist_getState() != PERSIST_STATE_STARTED )
   {
      while ( wait4write )
      {
         wait4write = 0 ; 
         for (i=0; i < ismStore_memGlobal.InMemGensCount; i++)
         {
            genIndex = (pMgmtHeader->ActiveGenIndex + i) % ismStore_memGlobal.InMemGensCount;
            pGen = &ismStore_memGlobal.InMemGens[genIndex];
            pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
            if (pGenHeader->State == ismSTORE_GEN_STATE_CLOSE_PENDING ||
                pGenHeader->State == ismSTORE_GEN_STATE_WRITE_PENDING )
                wait4write++ ; 
          }
          if ( wait4write )
          {
             pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
             ism_common_sleep(1000);
             pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
          }
      }
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

      // Create a checkpoint here or in recoveryPrepare.
      if ( (rc = ism_storePersist_createCP(ismStore_global.fHAEnabled)) != ISMRC_OK)
      {
         return rc;
      }
   
      // Start the ShmPersist thread
      if ((rc = ism_storePersist_start()) != StoreRC_OK  )
      {
         TRACE(1, "Failed to start the ShmPersist thread. error code %d\n", rc);
         rc = ISMRC_Error;
         return rc;
      }
      pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
   }

   TRACE(5, "Start scanning store generations maps to find unused generations\n");
   for (i=ismSTORE_MGMT_GEN_ID + 1; i < ismStore_memGlobal.GensMapSize; i++)
   {
      genId = (ismStore_GenId_t)i;
      genIndex = ism_store_memMapGenIdToIndex(genId);
      if ((pGenMap = ismStore_memGlobal.pGensMap[genId]) != NULL)
      {
         TRACE(5, "Store generation Id %u (Index %u), GranulesMapCount %u, DiskFileSize %lu, PredictedSizeBytes %lu, RecordsCount %u\n",
               genId, genIndex, pGenMap->GranulesMapCount, pGenMap->DiskFileSize, pGenMap->PredictedSizeBytes, pGenMap->RecordsCount);
      }

      // There is no disk file for the in-memory generations
      if (genIndex < ismStore_memGlobal.InMemGensCount)
      {
         continue;
      }

      if (pGenMap)
      {
         if (pGenMap->RecordsCount > 0)
         {
            pGenMap->MeanRecordSizeBytes = pGenMap->PredictedSizeBytes / pGenMap->RecordsCount;
         }

         ism_store_memCheckCompactThreshold(genId, pGenMap);
      }
   }

   ismStore_memGlobal.State = ismSTORE_STATE_ACTIVE;
   TRACE(5, "Store is now ready for use.\n");

   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

   // From now on, the reserved pool is assigned on startup to the small granules pool,
   // so if the pool is still ism_common_free(ism_memory_store_misc,unassigned), we assign it to the small granules pool.
   if (pMgmtHeader->RsrvPoolMemSizeBytes > 0)
   {
      ism_store_memAssignRsrvPool(ismSTORE_MGMT_SMALL_POOL_INDEX);
   }

   return ISMRC_OK;
}

int32_t ism_store_memStopCB(void)
{
  if ( ismStore_memGlobal.fEnablePersist )
    return ism_storePersist_stopCB();
  return ISMRC_OK;
}

/*
 * Terminate memory store component
 *
 * @param fHAShutdown    indicates that an High-Availability graceful shutdown is required
 */
int32_t ism_store_memTerm(uint8_t fHAShutdown)
{
   int fBackupToDisk=0, ec, i, j, enforcCP=0;
   ismStore_memStream_t *pStream;
   ismStore_memGeneration_t *pGen;
   ismStore_memGenMap_t *pGenMap;
   ismStore_memRefGen_t *pRefGen;
   ismStore_memRefGenPool_t *pRefGenPool;
   ismStore_memMgmtHeader_t *pMgmtHeader=NULL;
   ismStore_memGenHeader_t  *pGenHeader;
   ismStore_memGranulePool_t *pPool;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memSplitItem_t *pSplit;
   ismStore_DiskTaskParams_t diskTask;
   ismStore_memHAAck_t ack;
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   void *pRetVal;
   char nodeStr[40];
   uint8_t oldState = ismStore_memGlobal.State, fResetCtxt = 0, genIndex;
   int32_t rc = ISMRC_OK;
   uint64_t offset, tail;

   TRACE(5, "Store is being terminated. State %d, HAState %u, fHAShutdown %u\n",
         ismStore_memGlobal.State, pHAInfo->State, fHAShutdown);
   ismStore_memGlobal.State = ismSTORE_STATE_TERMINATING;
   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;

   if (ismStore_memGlobal.fActive)
   {
      pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);

      if (pHAInfo->State == ismSTORE_HA_STATE_STANDBY)
      {
         genIndex = (pMgmtHeader->ActiveGenIndex + ismStore_memGlobal.InMemGensCount - 1) % ismStore_memGlobal.InMemGensCount;
         pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[genIndex].pBaseAddress;
         if (pGenHeader->State == ismSTORE_GEN_STATE_CLOSE_PENDING && pGenHeader->GenId != ismSTORE_RSRV_GEN_ID)
         {
            ismStore_memJob_t job;
            memset(&job, '\0', sizeof(job));
            job.JobType = StoreJob_WriteGeneration;
            job.Generation.GenId = pGenHeader->GenId;
            job.Generation.GenIndex = genIndex;
            ism_store_memAddJob(&job);
         }
      }

      // Signal the threads that are waiting for a generation bitmap
      for (i=ismSTORE_MGMT_GEN_ID+1; i < ismStore_memGlobal.GensMapSize; i++)
      {
         if ((pGenMap = ismStore_memGlobal.pGensMap[i]))
         {
            pthread_mutex_lock(&pGenMap->Mutex);
            pthread_cond_broadcast(&pGenMap->Cond);
            pthread_mutex_unlock(&pGenMap->Mutex);
         }
      }

      // Make sure that there is no pending store-transaction
      for (i=0; i < ismStore_memGlobal.StreamsSize; i++)
      {
         if ((pStream = ismStore_memGlobal.pStreams[i]) && pStream->MyGenId != ismSTORE_RSRV_GEN_ID)
         {
            TRACE(1, "Failed to terminate the store, because there is an active store-transaction (hStream %u, MyGenId %u, ActiveGenId %u, RefsCount %d)\n",
                  i, pStream->MyGenId, pStream->ActiveGenId, pStream->RefsCount);
            pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
            rc = ISMRC_StoreBusy;
            return rc;
         }
      }

      pthread_cond_signal(&ismStore_memGlobal.StreamsCond);
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
   }

   // Wait until the Store thread is stopped
   if (ismStore_memGlobal.ThreadId)
   {
      pthread_mutex_lock(&ismStore_memGlobal.ThreadMutex);
      ismStore_memGlobal.fThreadGoOn = 0;
      pthread_cond_signal(&ismStore_memGlobal.ThreadCond);
      pthread_mutex_unlock(&ismStore_memGlobal.ThreadMutex);
      ism_common_joinThread(ismStore_memGlobal.ThreadId, &pRetVal);
   }

   // flush persist (+HA) queues and stop persist unit threads
   if (ismStore_memGlobal.fEnablePersist )
   {
      ism_storePersist_flushQ(1,5000) ; 
      ism_storePersist_stop() ; 
   }

   if (ismStore_memGlobal.fActive)
   {
      // If this node acts as the primary of the HA pair
      if (pHAInfo->State == ismSTORE_HA_STATE_PRIMARY)
      {
         // Send a graceful shutdown request to the Standby node
         if (fHAShutdown && ismSTORE_HASSTANDBY && pHAInfo->pIntChannel)
         {
            memcpy(pMgmtHeader->GenToken.NodeId, pHAInfo->View.ActiveNodeIds[0], sizeof(ismStore_HANodeID_t));
            pMgmtHeader->GenToken.Timestamp = ism_common_currentTimeNanos();
            ADR_WRITE_BACK(&pMgmtHeader->GenToken, sizeof(pMgmtHeader->GenToken));

            memset(&ack, '\0', sizeof(ack));
            ack.AckSqn = pHAInfo->pIntChannel->MsgSqn;
            if ((ec = ism_store_memHASendGenMsg(pHAInfo->pIntChannel, ismSTORE_MGMT_GEN_ID, 0, ismSTORE_BITMAP_LIVE_IDX, StoreHAMsg_Shutdown)) != StoreRC_OK)
            {
               TRACE(1, "Failed to shutdown the Standby node due to an HA send error. error code %d\n", ec);
               rc = ISMRC_StoreHAError;
               return rc;
            }
            ism_store_memB2H(nodeStr, (uint8_t *)pMgmtHeader->GenToken.NodeId, sizeof(ismStore_HANodeID_t));
            TRACE(5, "A store shutdown request has been sent to the Standby node. MsgSqn %lu, GenToken %s:%lu\n", ack.AckSqn, nodeStr, pMgmtHeader->GenToken.Timestamp);

            if ((ec = ism_store_memHAReceiveAck(pHAInfo->pIntChannel, &ack, 0)) != StoreRC_OK)
            {
               TRACE(1, "Failed to shutdown the Standby node due to an HA receive error. error code %d\n", ec);
               rc = ISMRC_StoreHAError;
               return rc;
            }

            if (ack.ReturnCode != StoreRC_HA_StoreTerm)
            {
               TRACE(1, "Failed to shutdown the Standby node due to an HA standby error. MsgSqn %lu, AckSqn %lu, SrcMsgType 0x%x, ReturnCode %d\n",
                     pHAInfo->pIntChannel->MsgSqn - 1, ack.AckSqn, ack.SrcMsgType, ack.ReturnCode);
               rc = ISMRC_StoreHAError;
               return rc;
            }
            TRACE(5, "A shutdown response has been received from the Standby node. ReturnCode %d\n", ack.ReturnCode);
         }
      }
      else if (pHAInfo->State == ismSTORE_HA_STATE_STANDBY)
      {
         // Make sure that all the sync generation files have already been written to the Standby disk
         ism_store_memHASyncWaitDisk();
      }

      // Close the open streams (the internal stream should be the last one).
      for (i=ismStore_memGlobal.StreamsSize-1; i >= 0; i--)
      {
         if ((pStream = ismStore_memGlobal.pStreams[i]))
         {
            ism_store_memCloseStream(i);
         }
      }
      ism_store_freeDeadStreams(1);

      fResetCtxt = (oldState == ismSTORE_STATE_ACTIVE || oldState == ismSTORE_STATE_RECOVERY) &&
                   (!ismStore_global.fHAEnabled || pHAInfo->State == ismSTORE_HA_STATE_PRIMARY);
   }

   enforcCP = (pHAInfo->State == ismSTORE_HA_STATE_STANDBY);
   
   // Close the HA module (if HA is enabled)
   if (ismStore_global.fHAEnabled && pHAInfo->State != ismSTORE_HA_STATE_CLOSED &&
       (ec = ism_store_memHATerm()) != StoreRC_OK)
   {
      rc = ISMRC_StoreHAError;
      return rc;
   }

   if (ismStore_memGlobal.fActive)
   {
      // Mark the store role as Primary
      if (pMgmtHeader->Role == ismSTORE_ROLE_STANDBY && pHAInfo->SyncCurMemSizeBytes == 0 && pHAInfo->SyncRC == ISMRC_OK)
      {
         pMgmtHeader->Role = ismSTORE_ROLE_PRIMARY;
         ADR_WRITE_BACK(&pMgmtHeader->Role, sizeof(pMgmtHeader->Role));
         TRACE(5, "Store role has been changed from %d to %d\n", ismSTORE_ROLE_STANDBY, pMgmtHeader->Role);
      }

      // If the fBackupToDisk parameter is set (value != 0) the store will write
      // the in-memory generations (including the management) into the disk.
      // This allows the ismserver to make a full recovery from the disk.
      fBackupToDisk = ism_common_getIntConfig(ismSTORE_CFG_BACKUP_DISK, ismSTORE_CFG_BACKUP_DISK_DV);

      // See whether we need to backup the store memory to disk
      if (fBackupToDisk)
      {
         if ((rc = ism_store_memBackup()) != ISMRC_OK)
         {
            return rc;
         }
      }
   }

   if (ismStore_memGlobal.fEnablePersist ) ism_storePersist_term(enforcCP);  //HA_Persist: ensure that a Check Point is created at this point

#if ismSTORE_ADMIN_CONFIG
   // Un-register the store with the configuration service
   if (ismStore_memGlobal.hConfig && (ec = ism_config_unregister(ismStore_memGlobal.hConfig)) != ISMRC_OK)
   {
      TRACE(1, "Failed to unregister the store with the configuration service. error code %d\n", ec);
   }
   ismStore_memGlobal.hConfig = NULL;
#endif

   // Wait until the StoreDisk terminates
   if (ismStore_memGlobal.fDiskUp)
   {
      pthread_mutex_lock(&ismStore_memGlobal.DiskMutex);
      memset(&diskTask, '\0', sizeof(diskTask));
      diskTask.GenId = 0;
      diskTask.Priority = 2;
      diskTask.Callback = ism_store_memDiskTerminated;
      ism_storeDisk_term(&diskTask);
      while (ismStore_memGlobal.fDiskUp)
      {
         pthread_mutex_unlock(&ismStore_memGlobal.DiskMutex);
         ism_common_sleep(1000);
         pthread_mutex_lock(&ismStore_memGlobal.DiskMutex);
      }
      pthread_mutex_unlock(&ismStore_memGlobal.DiskMutex);
   }

   // Scan the management generation, set the pRefCtxt and pStateCtxt of the SplitItem objects to NULL and release the resources.
   // We must do that AFTER the StoreDisk has been terminated, because the StoreDisk may access the Context objects.
   if (fResetCtxt)
   {
      TRACE(8, "Scan the Store managenemt small granules and reset the pRefCtxt and pStateCtxt\n");
      pGen = &ismStore_memGlobal.MgmtGen;
      pPool = &pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX];
      for (offset = pPool->Offset, tail = pPool->Offset + pPool->MaxMemSizeBytes; offset < tail; offset += pPool->GranuleSizeBytes)
      {
         pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + offset);
         if (ismSTORE_IS_SPLITITEM(pDescriptor->DataType))
         {
            pSplit = (ismStore_memSplitItem_t *)(pDescriptor + 1);
            if (pSplit->pRefCtxt)
            {
               ism_store_memFreeRefCtxt(pSplit, ismSTORE_RSRV_GEN_ID);
            }
            if (pSplit->pStateCtxt)
            {
               ism_store_memFreeStateCtxt(pSplit, ismSTORE_RSRV_GEN_ID);
            }
         }
      }
   }

   // Release recovery resources
   ism_store_memRecoveryTerm();

   pthread_mutex_lock(&ismStore_FnMutex);
   // Mark the store as closed
   ismStore_memGlobal.fMemValid = 0;
   ismStore_memGlobal.fActive = 0;
   ismStore_memGlobal.State = ismSTORE_STATE_CLOSED;

   // Clean the function pointers
   ismStore_global.pFnStart                       = NULL;
   ismStore_global.pFnTerm                        = NULL;
   ismStore_global.pFnDrop                        = NULL;
   ismStore_global.pFnRegisterEventCallback       = NULL;
   ismStore_global.pFnGetGenIdOfHandle            = NULL;
   ismStore_global.pFnGetStatistics               = NULL;
   ismStore_global.pFnOpenStream                  = NULL;
   ismStore_global.pFnCloseStream                 = NULL;
   ismStore_global.pFnReserveStreamResources      = NULL;
   ismStore_global.pFnCancelResourceReservation   = NULL;
   ismStore_global.pFnGetStreamOpsCount           = NULL;
   ismStore_global.pFnOpenReferenceContext        = NULL;
   ismStore_global.pFnCloseReferenceContext       = NULL;
   ismStore_global.pFnClearReferenceContext       = NULL;
   ismStore_global.pFnMapReferenceContextToHandle = NULL;
   ismStore_global.pFnOpenStateContext            = NULL;
   ismStore_global.pFnCloseStateContext           = NULL;
   ismStore_global.pFnMapStateContextToHandle     = NULL;
   ismStore_global.pFnEndStoreTransaction         = NULL;
   ismStore_global.pFnAssignRecordAllocation      = NULL;
   ismStore_global.pFnFreeRecordAllocation        = NULL;
   ismStore_global.pFnUpdateRecord                = NULL;
   ismStore_global.pFnAddReference                = NULL;
   ismStore_global.pFnUpdateReference             = NULL;
   ismStore_global.pFnDeleteReference             = NULL;
   ismStore_global.pFnPruneReferences             = NULL;
   ismStore_global.pFnAddState                    = NULL;
   ismStore_global.pFnDeleteState                 = NULL;
   ismStore_global.pFnGetNextGenId                = NULL;
   ismStore_global.pFnGetNextRecordForType        = NULL;
   ismStore_global.pFnReadRecord                  = NULL;
   ismStore_global.pFnGetNextReferenceForOwner    = NULL;
   ismStore_global.pFnReadReference               = NULL;
   ismStore_global.pFnGetNextStateForOwner        = NULL;
   ismStore_global.pFnRecoveryCompleted           = NULL;
   ismStore_global.pFnDump                        = NULL;
   ismStore_global.pFnDumpCB                      = NULL;
   ismStore_global.pFnStopCB                      = NULL;
   ismStore_global.pFnStartTransaction            = NULL;
   ismStore_global.pFnCancelTransaction           = NULL;
   ismStore_global.pFnGetAsyncCBStats             = NULL;
   pthread_mutex_unlock(&ismStore_FnMutex);

   if (ismStore_memGlobal.DiskMutexInit > 0)
   {
      pthread_mutex_destroy(&ismStore_memGlobal.DiskMutex);
   }

   if (ismStore_memGlobal.ThreadMutexInit)
   {
      pthread_mutex_destroy(&ismStore_memGlobal.ThreadMutex);
      if (ismStore_memGlobal.ThreadMutexInit > 1)
      {
         pthread_cond_destroy(&ismStore_memGlobal.ThreadCond);
      }
   }

   // Release the GenMap array
   for (i=0; i < ismStore_memGlobal.GensMapSize && ismStore_memGlobal.GensMapCount > 1; i++)
   {
      ism_store_memFreeGenMap(i);
   }

   ismSTORE_FREE(ismStore_memGlobal.pGensMap);
   ismStore_memGlobal.GensMapSize = 0;
   ismStore_memGlobal.GensMapCount = 0;

   for (j=0; j < ismSTORE_GRANULE_POOLS_COUNT; j++)
   {
      if (ismStore_memGlobal.MgmtGen.PoolMutexInit > j)
      {
         pthread_mutex_destroy(&ismStore_memGlobal.MgmtGen.PoolMutex[j]);
      }
   }
   ismStore_memGlobal.MgmtGen.PoolMutexInit = 0;

   for (i=0; i < ismSTORE_MAX_INMEM_GENS; i++)
   {
      for (j=0; j < ismSTORE_GRANULE_POOLS_COUNT; j++)
      {
         if (ismStore_memGlobal.InMemGens[i].PoolMutexInit > j)
         {
            pthread_mutex_destroy(&ismStore_memGlobal.InMemGens[i].PoolMutex[j]);
         }
      }
      ismStore_memGlobal.InMemGens[i].PoolMutexInit = 0;
   }

   if (ismStore_memGlobal.RsrvPoolMutexInit)
   {
      pthread_mutex_destroy(&ismStore_memGlobal.RsrvPoolMutex);
      if (ismStore_memGlobal.RsrvPoolMutexInit > 1)
      {
         pthread_cond_destroy(&ismStore_memGlobal.RsrvPoolCond);
      }
      ismStore_memGlobal.RsrvPoolMutexInit = 0;
   }

   // Release RefCtxtMutex array
   if (ismStore_memGlobal.pRefCtxtMutex)
   {
      for (i=0; i < ismStore_memGlobal.RefCtxtLocksCount; i++)
      {
         if (i < ismStore_memGlobal.RefCtxtMutexInit)
         {
            ismSTORE_MUTEX_DESTROY(ismStore_memGlobal.pRefCtxtMutex[i]);
         }

         if (ismStore_memGlobal.pRefCtxtMutex[i])
         {
            ism_common_free_memaligned(ism_memory_store_misc,ismStore_memGlobal.pRefCtxtMutex[i]);
            ismStore_memGlobal.pRefCtxtMutex[i] = NULL;
         }
      }
      ismSTORE_FREE(ismStore_memGlobal.pRefCtxtMutex);
      ismStore_memGlobal.pRefCtxtMutex = NULL;
      ismStore_memGlobal.RefCtxtMutexInit = 0;
   }

   // Release StateCtxtMutex array
   if (ismStore_memGlobal.pStateCtxtMutex)
   {
      for (i=0; i < ismStore_memGlobal.StateCtxtLocksCount; i++)
      {
         if (i < ismStore_memGlobal.StateCtxtMutexInit)
         {
            ismSTORE_MUTEX_DESTROY(ismStore_memGlobal.pStateCtxtMutex[i]);
         }

         if (ismStore_memGlobal.pStateCtxtMutex[i])
         {
        	 ism_common_free_memaligned(ism_memory_store_misc,ismStore_memGlobal.pStateCtxtMutex[i]);
        	 ismStore_memGlobal.pStateCtxtMutex[i] = NULL;
         }
      }
      ismSTORE_FREE(ismStore_memGlobal.pStateCtxtMutex);
      ismStore_memGlobal.pStateCtxtMutex = NULL;
      ismStore_memGlobal.StateCtxtMutexInit = 0;
   }

   // Release RefGenPool array
   if (ismStore_memGlobal.pRefGenPool)
   {
      for (i=0; i < ismStore_memGlobal.RefCtxtLocksCount; i++)
      {
         pRefGenPool = &ismStore_memGlobal.pRefGenPool[i];
         if (pRefGenPool->Count != pRefGenPool->Size)
         {
            TRACE(1, "The RefGenPool (Index %d) does not contain all the elements. Count %d, Size %u\n", pRefGenPool->Index, pRefGenPool->Count, pRefGenPool->Size);
         }

         while (pRefGenPool->pHead)
         {
            pRefGen = pRefGenPool->pHead;
            pRefGenPool->pHead = pRefGen->Next;
            pRefGenPool->Count--;
            pRefGenPool->Size--;
            ism_common_free(ism_memory_store_misc,pRefGen);
         }

         if (pRefGenPool->Count != 0)
         {
            TRACE(1, "The RefGenPool (Index %d) is not balanced. Count %d, Size %u\n", pRefGenPool->Index, pRefGenPool->Count, pRefGenPool->Size);
         }
      }
      ismSTORE_FREE(ismStore_memGlobal.pRefGenPool);
   }

   if (ismStore_memGlobal.CtxtMutexInit)
   {
      pthread_mutex_destroy(&ismStore_memGlobal.CtxtMutex);
      ismStore_memGlobal.CtxtMutexInit = 0;
   }

   if (ismStore_memGlobal.StreamsMutexInit)
   {
      pthread_mutex_destroy(&ismStore_memGlobal.StreamsMutex);
      if (ismStore_memGlobal.StreamsMutexInit > 1)
      {
         pthread_cond_destroy(&ismStore_memGlobal.StreamsCond);
      }
      ismSTORE_FREE(ismStore_memGlobal.pStreams);
      ismStore_memGlobal.StreamsMutexInit = 0;
   }

   if (ismStore_memGlobal.pStoreBaseAddress != NULL)
   {
     munmap(ismStore_memGlobal.pStoreBaseAddress, ismStore_memGlobal.TotalMemSizeBytes);
     ismStore_memGlobal.pStoreBaseAddress = NULL;
   }

   // Close the store lock file
   if (ismStore_memGlobal.LockFileDescriptor != -1)
   {
      (void)ism_store_closeLockFile(ismStore_memGlobal.DiskRootPath, ismStore_memGlobal.LockFileName, ismStore_memGlobal.LockFileDescriptor);
   }

   TRACE(5, "Store terminated successfully\n");

   return rc;
}

/*
 * Drops the memory for the store component
 */
int32_t ism_store_memDrop(void)
{
   int unixrc;
   int32_t rc = ISMRC_OK;

   if (ismStore_global.MemType == ismSTORE_MEMTYPE_SHM)
   {
      // Unlink the shared memory object.
      if ((ismStore_memGlobal.SharedMemoryName != NULL) &&
          (strlen(ismStore_memGlobal.SharedMemoryName) > 0))
      {
         unixrc = shm_unlink(ismStore_memGlobal.SharedMemoryName);
         if (unixrc == 0)
         {
            TRACE(1, "Memory store dropped.\n");
         }
         else
         {
            TRACE(1, "Failed to unlink shared memory object - errno %d.\n", errno);
            rc = ISMRC_Error;
         }
      }
   }
   else if (ismStore_global.MemType == ismSTORE_MEMTYPE_NVRAM || ismStore_global.MemType == ismSTORE_MEMTYPE_MEM)
   {
      if (ismStore_memGlobal.State != ismSTORE_STATE_INIT)
      {
         TRACE(1, "Failed to drop the store, because the store is busy.\n");
         rc = ISMRC_StoreBusy;
      }
      else if (ismStore_memGlobal.pStoreBaseAddress)
      {
         memset(ismStore_memGlobal.pStoreBaseAddress, '\0', ismStore_memGlobal.TotalMemSizeBytes);
      }
   }
   else
   {
      // Should not occur
      rc = ISMRC_Error;
      TRACE(1, "Failed to drop the store, because the store memory type %u is not valid.\n", ismStore_global.MemType);
   }

   // Unlink the store lock file also.
   if (rc == ISMRC_OK && ismStore_memGlobal.LockFileDescriptor != -1)
   {
      (void)ism_store_removeLockFile(ismStore_memGlobal.DiskRootPath, ismStore_memGlobal.LockFileName, ismStore_memGlobal.LockFileDescriptor);
      ismStore_memGlobal.LockFileDescriptor = -1;
   }

   return rc;
}

/*
 * Registers event callback
 */
int32_t ism_store_memRegisterEventCallback(ismPSTOREEVENT callback, void *pContext)
{
   if (!callback)
   {
      TRACE(1, "Failed to register a store event callback, because the callback is NULL\n");
      return ISMRC_NullArgument;
   }

   ismStore_memGlobal.OnEvent = callback;
   ismStore_memGlobal.pEventContext = pContext;
   TRACE(5, "Store event callback (%p) has been registered\n", callback);

   return ISMRC_OK;
}

static int32_t ism_store_memBackup(void)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_DiskTaskParams_t diskTask;
   int ec;

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
   // Write the active generation to the disk
   if (pMgmtHeader->ActiveGenId != ismSTORE_RSRV_GEN_ID)
   {
      pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[pMgmtHeader->ActiveGenIndex].pBaseAddress;
      memset(&diskTask, '\0', sizeof(diskTask));
      diskTask.GenId = pGenHeader->GenId;
      diskTask.Priority = 0;
      diskTask.Callback = ism_store_memDiskWriteBackupComplete;
      diskTask.BufferParams->pBuffer = (char *)pGenHeader;
      diskTask.BufferParams->BufferLength = pGenHeader->MemSizeBytes;
      TRACE(5, "Store active generation Id %u (Index %u) is being written to the disk for backup. MemSizeBytes %lu \n",
            pGenHeader->GenId, pMgmtHeader->ActiveGenIndex, pGenHeader->MemSizeBytes);
      if ((ec = ism_storeDisk_writeGeneration(&diskTask)) != StoreRC_OK)
      {
         TRACE(1, "Failed to write generation Id %u to the disk. error code %d\n", pGenHeader->GenId, ec);
         return ISMRC_Error;
      }
   }

   // Write the management generation to the disk
   memset(&diskTask, '\0', sizeof(diskTask));
   diskTask.GenId = pMgmtHeader->GenId;
   diskTask.Priority = 0;
   diskTask.Callback = ism_store_memDiskWriteBackupComplete;
   diskTask.BufferParams->pBuffer = (char *)pMgmtHeader;
   diskTask.BufferParams->BufferLength = pMgmtHeader->MemSizeBytes;
   TRACE(5, "Store management generation is being written to the disk for backup. MemSizeBytes %lu\n", pMgmtHeader->MemSizeBytes);
   if ((ec = ism_storeDisk_writeGeneration(&diskTask)) != StoreRC_OK)
   {
      TRACE(1, "Failed to write the management generation to the disk. error code %d\n", ec);
      return ISMRC_Error;
   }

   return ISMRC_OK;
}

int32_t ism_store_memGetGenIdOfHandle(ismStore_Handle_t handle, ismStore_GenId_t *pGenId)
{
   int32_t rc = ISMRC_OK;

   if (pGenId)
   {
      *pGenId = ismSTORE_EXTRACT_GENID(handle);
   }
   else
   {
      rc = ISMRC_NullArgument;
   }

   return rc;
}

/*
 * Returns the store statistics.
 */
int32_t ism_store_memGetStatistics(ismStore_Statistics_t *pStatistics)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memGenIdChunk_t *pGenIdChunk;
   ismStore_DiskStatistics_t diskStats;
   ismStore_Handle_t hChunk;
   double pct=0;
   int ec, i;
   uint64_t usedSpaceBytes[ismSTORE_GRANULE_POOLS_COUNT], freeSpaceBytes[ismSTORE_GRANULE_POOLS_COUNT];
   uint32_t gensCount;
   int32_t rc = ISMRC_OK;

   if (pStatistics)
   {
      memset(pStatistics, '\0', sizeof(*pStatistics));
      if (ismStore_memGlobal.fMemValid)
      {
         if ( su_mutex_tryLock(&ismStore_memGlobal.StreamsMutex, 1000) )
         {
            return ISMRC_StoreNotAvailable;
         }

         pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
         pStatistics->ActiveGenId = pMgmtHeader->ActiveGenId;
         pStatistics->StreamsCount = ismStore_memGlobal.StreamsCount;
         // We don't want to take into account the store internal stream (index=0)
         if (ismStore_memGlobal.pStreams &&
             ismStore_memGlobal.hInternalStream < ismStore_memGlobal.StreamsSize &&
             ismStore_memGlobal.pStreams[ismStore_memGlobal.hInternalStream])
         {
            pStatistics->StreamsCount--;
         }
         pStatistics->StoreTransRsrvOps = ismStore_memGlobal.StoreTransRsrvOps;
         pStatistics->MgmtSmallGranuleSizeBytes = ismStore_memGlobal.MgmtSmallGranuleSizeBytes;
         pStatistics->MgmtGranuleSizeBytes      = ismStore_memGlobal.MgmtGranuleSizeBytes;

         // Calculate the number of generations used by the store
         for (hChunk = pMgmtHeader->GenIdHandle, gensCount = 0; hChunk != ismSTORE_NULL_HANDLE; )
         {
            pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hChunk);
            pGenIdChunk = (ismStore_memGenIdChunk_t *)(pDescriptor + 1);
            gensCount += pGenIdChunk->GenIdCount;
            hChunk = pDescriptor->NextHandle;
         }
         pStatistics->GenerationsCount = gensCount;

         for (i=0; i < pMgmtHeader->PoolsCount; i++)
         {
            pthread_mutex_lock(&ismStore_memGlobal.MgmtGen.PoolMutex[i]);
            freeSpaceBytes[i] = (uint64_t)pMgmtHeader->GranulePool[i].GranuleCount * pMgmtHeader->GranulePool[i].GranuleSizeBytes;
            usedSpaceBytes[i] = pMgmtHeader->GranulePool[i].MaxMemSizeBytes - freeSpaceBytes[i];
            pct = 0;
            if (usedSpaceBytes[i] > 0 || freeSpaceBytes[i] > 0) {
               pct = 100 * ((double)usedSpaceBytes[i] / (usedSpaceBytes[i] + freeSpaceBytes[i] + pMgmtHeader->RsrvPoolMemSizeBytes));
            }
            pthread_mutex_unlock(&ismStore_memGlobal.MgmtGen.PoolMutex[i]);
            #if 0
            pStatistics->MgmtFreeSpaceBytes += freeSpaceBytes[i];
            pStatistics->MgmtUsedSpaceBytes += usedSpaceBytes[i];
            if (pct > pStatistics->StoreMemUsagePct) {
               pStatistics->StoreMemUsagePct = (uint8_t)pct;
            }
            #endif
         }
         // pStatistics->MgmtFreeSpaceBytes += pMgmtHeader->RsrvPoolMemSizeBytes;  // to be removed
         pStatistics->PrimaryLastTime = pMgmtHeader->PrimaryTime;
         pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

         memset(&diskStats, '\0', sizeof(ismStore_DiskStatistics_t));
         if ((ec = ism_storeDisk_getStatistics(&diskStats)) == StoreRC_OK)
         {
            pStatistics->DiskFreeSpaceBytes = diskStats.DiskFreeSpaceBytes;
            pStatistics->DiskUsedSpaceBytes = diskStats.DiskUsedSpaceBytes;
            if (diskStats.DiskUsedSpaceBytes > 0 || diskStats.DiskFreeSpaceBytes > 0)
            {
               pct = 100 * ((double)diskStats.DiskUsedSpaceBytes / (diskStats.DiskUsedSpaceBytes + diskStats.DiskFreeSpaceBytes));
               pStatistics->StoreDiskUsagePct = (uint8_t)pct;
            }
         }
         else
         {
            TRACE(1, "Failed to retrieve Store disk statistics. error code %d\n", ec);
            rc = ISMRC_StoreDiskError;
         }

         if (ismStore_global.fHAEnabled)
         {
            pStatistics->HASyncCompletionPct = ism_store_memHAGetSyncCompPct();
         }
         pStatistics->RecoveryCompletionPct = (int8_t)ism_store_memRecoveryCompletionPct();

         /* Total management gen mem ststs */       
         pStatistics->MemStats.MemoryFreeBytes     = freeSpaceBytes[0] + freeSpaceBytes[1];
         pStatistics->MemStats.MemoryTotalBytes    = usedSpaceBytes[0] + usedSpaceBytes[1] + pStatistics->MemStats.MemoryFreeBytes;
         if (pStatistics->MemStats.MemoryTotalBytes > 0 )
           pStatistics->MemStats.MemoryUsedPercent = (int8_t)(100*((double)(pStatistics->MemStats.MemoryTotalBytes - pStatistics->MemStats.MemoryFreeBytes)/pStatistics->MemStats.MemoryTotalBytes));
         else
           pStatistics->MemStats.MemoryUsedPercent = 0;
         pStatistics->MemStats.RecordSize          = ismStore_memGlobal.MgmtSmallGranuleSizeBytes;

         /* Pool 1 mem stats */       
         pStatistics->MemStats.Pool1TotalBytes     = usedSpaceBytes[0] + freeSpaceBytes[0];
         pStatistics->MemStats.Pool1UsedBytes      = usedSpaceBytes[0];
         if (pStatistics->MemStats.Pool1TotalBytes > 0 )
           pStatistics->MemStats.Pool1UsedPercent  = (int8_t)(100*((double)(pStatistics->MemStats.Pool1UsedBytes)/pStatistics->MemStats.Pool1TotalBytes));
         else
           pStatistics->MemStats.Pool1UsedPercent  = 0;
         pStatistics->MemStats.Pool1RecordsLimitBytes  = (uint64_t)ismStore_memGlobal.MgmtSmallGranuleSizeBytes * ismStore_memGlobal.OwnerGranulesLimit;
         pStatistics->MemStats.ClientStatesBytes   = (uint64_t)ismStore_memGlobal.MgmtSmallGranuleSizeBytes * ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_CLIENT]];
         pStatistics->MemStats.QueuesBytes         = (uint64_t)ismStore_memGlobal.MgmtSmallGranuleSizeBytes * ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_QUEUE]];
         pStatistics->MemStats.TopicsBytes         = (uint64_t)ismStore_memGlobal.MgmtSmallGranuleSizeBytes * ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_TOPIC]];
         pStatistics->MemStats.SubscriptionsBytes  = (uint64_t)ismStore_memGlobal.MgmtSmallGranuleSizeBytes * ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_SUBSC]];
         pStatistics->MemStats.TransactionsBytes   = (uint64_t)ismStore_memGlobal.MgmtSmallGranuleSizeBytes * ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_TRANS]];
         pStatistics->MemStats.MQConnectivityBytes = (uint64_t)ismStore_memGlobal.MgmtSmallGranuleSizeBytes * ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_BMGR]];
         pStatistics->MemStats.RemoteServerBytes   = (uint64_t)ismStore_memGlobal.MgmtSmallGranuleSizeBytes * ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_REMSRV]];
         pStatistics->MemStats.Pool1RecordsUsedBytes = pStatistics->MemStats.ClientStatesBytes + pStatistics->MemStats.QueuesBytes + pStatistics->MemStats.TopicsBytes
                                                       + pStatistics->MemStats.SubscriptionsBytes + pStatistics->MemStats.TransactionsBytes + pStatistics->MemStats.MQConnectivityBytes;

         /* Pool 2 mem stats */       
         pStatistics->MemStats.Pool2TotalBytes     = usedSpaceBytes[1] + freeSpaceBytes[1];
         pStatistics->MemStats.Pool2UsedBytes      = usedSpaceBytes[1];
         if (pStatistics->MemStats.Pool2TotalBytes > 0 )
           pStatistics->MemStats.Pool2UsedPercent  = (int8_t)(100*((double)(pStatistics->MemStats.Pool2UsedBytes)/pStatistics->MemStats.Pool2TotalBytes));
         else
           pStatistics->MemStats.Pool2UsedPercent  = 0;
         pStatistics->MemStats.IncomingMessageAcksBytes = (uint64_t)ismStore_memGlobal.MgmtGranuleSizeBytes * ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_MSG]];

         TRACE(8, "Store General statistics: ActiveGenId %u, GenerationsCount %u, StreamsCount %u, StoreTransRsrvOps %u, " \
               "DiskFreeSpaceBytes %lu, DiskUsedSpaceBytes %lu, StoreDiskUsagePct %u, HASyncCompletionPct %d, " \
               "RecoveryCompletionPct %d, PrimaryLastTime %lu, MgmtSmallGranuleSizeBytes %u, MgmtGranuleSizeBytes %u\n",
               pStatistics->ActiveGenId, pStatistics->GenerationsCount, pStatistics->StreamsCount,
               pStatistics->StoreTransRsrvOps, pStatistics->DiskFreeSpaceBytes, pStatistics->DiskUsedSpaceBytes,
               pStatistics->StoreDiskUsagePct, pStatistics->HASyncCompletionPct, pStatistics->RecoveryCompletionPct,
               pStatistics->PrimaryLastTime, pStatistics->MgmtSmallGranuleSizeBytes, pStatistics->MgmtGranuleSizeBytes);

         TRACE(8, "Store Memory statistics: MemoryTotalBytes %lu, MemoryFreeBytes %lu, Pool1TotalBytes %lu, Pool1UsedBytes %lu, " \
               "Pool1RecordsLimitBytes %lu, Pool1RecordsUsedBytes %lu, Pool2TotalBytes %lu, Pool2UsedBytes %lu, " \
               "ClientStatesBytes %lu, QueuesBytes %lu, TopicsBytes %lu, SubscriptionsBytes %lu, " \
               "TransactionsBytes %lu, MQConnectivityBytes %lu, RemoteServerBytes %lu, IncomingMessageAcksBytes %lu, " \
               "RecordSize %u, MemoryUsedPercent %u, Pool1UsedPercent %u, Pool2UsedPercent %u\n",
               pStatistics->MemStats.MemoryTotalBytes, pStatistics->MemStats.MemoryFreeBytes, pStatistics->MemStats.Pool1TotalBytes, pStatistics->MemStats.Pool1UsedBytes,
               pStatistics->MemStats.Pool1RecordsLimitBytes, pStatistics->MemStats.Pool1RecordsUsedBytes, pStatistics->MemStats.Pool2TotalBytes, pStatistics->MemStats.Pool2UsedBytes,
               pStatistics->MemStats.ClientStatesBytes, pStatistics->MemStats.QueuesBytes, pStatistics->MemStats.TopicsBytes, pStatistics->MemStats.SubscriptionsBytes, 
               pStatistics->MemStats.TransactionsBytes, pStatistics->MemStats.MQConnectivityBytes, pStatistics->MemStats.RemoteServerBytes, pStatistics->MemStats.IncomingMessageAcksBytes, 
               pStatistics->MemStats.RecordSize, pStatistics->MemStats.MemoryUsedPercent, pStatistics->MemStats.Pool1UsedPercent, pStatistics->MemStats.Pool2UsedPercent);


         #if 0
         pStatistics->OwnerCount.TotalOwnerRecordsLimit   = ismStore_memGlobal.OwnerGranulesLimit;
         pStatistics->OwnerCount.NumOfClientRecords       = ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_CLIENT]];
         pStatistics->OwnerCount.NumOfQueueRecords        = ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_QUEUE]];
         pStatistics->OwnerCount.NumOfTopicRecords        = ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_TOPIC]];
         pStatistics->OwnerCount.NumOfSubscriptionRecords = ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_SUBSC]];
         pStatistics->OwnerCount.NumOfTransactionRecords  = ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_TRANS]];
         pStatistics->OwnerCount.NumOfMQBridgeRecords     = ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_BMGR ]];
         pStatistics->OwnerCount.NumOfClientStateRecords  = ismStore_memGlobal.OwnerCount[ismStore_T2T[ISM_STORE_RECTYPE_MSG  ]];
         TRACE(8, "Store statistics: ActiveGenId %u, GenerationsCount %u, StreamsCount %u, StoreTransRsrvOps %u, " \
               "MgmtFreeSpaceBytes %lu, MgmtUsedSpaceBytes %lu, StoreMemUsagePct %u, " \
               "DiskFreeSpaceBytes %lu, DiskUsedSpaceBytes %lu, StoreDiskUsagePct %u, HASyncCompletionPct %d, " \
               "RecoveryCompletionPct %d, PrimaryLastTime %lu, MgmtSmallGranuleSizeBytes %u, MgmtGranuleSizeBytes %u, " \
               "TotalOwnerRecordsLimit %u, NumOfClientRecords %u, NumOfQueueRecords %u, NumOfTopicRecords %u, " \
               "NumOfSubscriptionRecords %u, NumOfTransactionRecords %u, NumOfMQBridgeRecords %u, " \
               "NumOfClientStateRecords %u\n",
               pStatistics->ActiveGenId, pStatistics->GenerationsCount, pStatistics->StreamsCount,
               pStatistics->StoreTransRsrvOps, pStatistics->MgmtFreeSpaceBytes, pStatistics->MgmtUsedSpaceBytes,
               pStatistics->StoreMemUsagePct, pStatistics->DiskFreeSpaceBytes, pStatistics->DiskUsedSpaceBytes,
               pStatistics->StoreDiskUsagePct, pStatistics->HASyncCompletionPct, pStatistics->RecoveryCompletionPct,
               pStatistics->PrimaryLastTime, pStatistics->MgmtSmallGranuleSizeBytes, pStatistics->MgmtGranuleSizeBytes,
               pStatistics->OwnerCount.TotalOwnerRecordsLimit, pStatistics->OwnerCount.NumOfClientRecords,
               pStatistics->OwnerCount.NumOfQueueRecords, pStatistics->OwnerCount.NumOfTopicRecords,
               pStatistics->OwnerCount.NumOfSubscriptionRecords, pStatistics->OwnerCount.NumOfTransactionRecords,
               pStatistics->OwnerCount.NumOfMQBridgeRecords, pStatistics->OwnerCount.NumOfClientStateRecords);
         #endif
      }
      else
      {
         rc = ISMRC_StoreNotAvailable;
      }
   }
   else
   {
      rc = ISMRC_NullArgument;
   }

   return rc;
}

/*
 * Allocates a stream and a store-transaction in the memory store.
 */
int32_t ism_store_memOpenStream(ismStore_StreamHandle_t *phStream, uint8_t fHighPerf)
{
   if (ismStore_memGlobal.State != ismSTORE_STATE_ACTIVE)
   {
      return ISMRC_StoreNotAvailable;
   }

   return ism_store_memOpenStreamEx(phStream, fHighPerf, 0);
}

/*
 * Allocates a stream and a store-transaction in the memory store.
 */
static int32_t ism_store_memOpenStreamEx(ismStore_StreamHandle_t *phStream, uint8_t fHighPerf, uint8_t fIntStream)
{
   ismStore_memStream_t **tmp, *pStream = NULL;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGeneration_t *pGen;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_Handle_t handle;
   int iok = 0;
   uint8_t flags = 0;
   uint32_t dataLength;
   int32_t rc = ISMRC_OK, hStream, channelId;

   TRACE(9, "Entry: %s\n", __FUNCTION__);
   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);

   // If the store is locked by the HASync, we have to wait until the new node synchronization completes
   while (ismStore_memGlobal.fLocked)
   {
      TRACE(7, "The %s is blocked, because the store is locked by the HA synchronization\n", __FUNCTION__);
      pthread_cond_wait(&ismStore_memGlobal.StreamsCond, &ismStore_memGlobal.StreamsMutex);
      if (ismStore_memGlobal.State != ismSTORE_STATE_ACTIVE)
      {
         rc = ISMRC_StoreNotAvailable;
         goto exit;
      }
   }

   // Search for a free place in streams array
   for (hStream=0; hStream < ismStore_memGlobal.StreamsSize && ismStore_memGlobal.pStreams[hStream] != NULL; hStream++); /* empty body */
   if (hStream >= ismStore_memGlobal.StreamsSize)
   {
      if ( ismStore_memGlobal.fEnablePersist ) ism_storePersist_wrLock();
      if ((tmp = (ismStore_memStream_t **)ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,138),ismStore_memGlobal.pStreams, 2 * ismStore_memGlobal.StreamsSize * sizeof(ismStore_memStream_t *))) == NULL)
      {
         if ( ismStore_memGlobal.fEnablePersist ) ism_storePersist_wrUnlock();
         rc = ISMRC_AllocateError;
         goto exit;
      }
      ismStore_memGlobal.pStreams = tmp;
      memset(ismStore_memGlobal.pStreams + ismStore_memGlobal.StreamsSize, '\0', ismStore_memGlobal.StreamsSize * sizeof(ismStore_memStream_t *));
      hStream = ismStore_memGlobal.StreamsSize;
      ismStore_memGlobal.StreamsSize *= 2;
      if ( ismStore_memGlobal.fEnablePersist ) ism_storePersist_wrUnlock();
   }
   iok++;

   pStream = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,139),sizeof(ismStore_memStream_t));
   if (pStream == NULL)
   {
      rc = ISMRC_AllocateError;
      goto exit;
   }
   iok++;

   memset(pStream, '\0', sizeof(ismStore_memStream_t));
   pStream->hStream = hStream;
   pStream->hStoreTranHead = pStream->hStoreTranCurrent = pStream->hStoreTranRsrv = ismSTORE_NULL_HANDLE;
   pStream->fHighPerf = fHighPerf;

   if (ismSTORE_MUTEX_INIT(&pStream->Mutex))
   {
      rc = ISMRC_Error;
      goto exit;
   }
   iok++;

   if (ismSTORE_COND_INIT(&pStream->Cond))
   {
      rc = ISMRC_Error;
      goto exit;
   }
   iok++;

   // If a Standby node exists, we create an High-Availability channel
   channelId = hStream;
   if (fHighPerf) { flags |= 0x1; }
   if (fIntStream) { flags |= 0x2; }
// if (ismStore_global.fHAEnabled && ismSTORE_HASSTANDBY && ism_store_memHACreateChannel(channelId, flags, &pStream->pHAChannel) != StoreRC_OK)
   if (ismStore_global.fHAEnabled && ismSTORE_HASSTANDBY )
   {
     int trc = ism_store_memHACreateChannel(channelId, flags, &pStream->pHAChannel);
     if ( trc != StoreRC_OK )
     {
       if ( trc == StoreRC_SystemError )
       {
          rc = ISMRC_StoreHAError;
          goto exit;
       }
       else
       {
         double et ; 
         et = su_sysTime() + 5e0 ; 
         while ( ismSTORE_HASSTANDBY && et > su_sysTime() )
           su_sleep(1,SU_MIC) ; 
         if ( ismSTORE_HASSTANDBY )
         {
           rc = ISMRC_StoreHAError;
           goto exit;
         }
       }
     }
   }

   pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;

   // If this is an application stream we allocate the reserved granules for
   // the store-transaction operations.
   if (!fIntStream)
   {
      dataLength = ismStore_memGlobal.StoreTransRsrvOps * pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX].GranuleDataSizeBytes / ismStore_memGlobal.MgmtGen.MaxTranOpsPerGranule;
      rc = ism_store_memGetMgmtPoolElements(NULL,
                                            ismSTORE_MGMT_POOL_INDEX,
                                            ismSTORE_DATATYPE_STORE_TRAN,
                                            0, 0, dataLength,
                                            &handle, &pDescriptor);
      if (rc != ISMRC_OK)
      {
         goto exit;
      }
      iok++;

      pStream->hStoreTranHead = pStream->hStoreTranCurrent = pStream->hStoreTranRsrv = handle;
      pStream->pDescrTranHead = pDescriptor ; 
      while (pDescriptor)
      {
         // Initialize the store-transaction chunk
         pDescriptor->DataType = (pStream->ChunksInUse++ == 0 ? ismSTORE_DATATYPE_STORE_TRAN : ismSTORE_DATATYPE_STORE_TRAN | ismSTORE_DATATYPE_NOT_PRIMARY);
         pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);
         memset(pTran, '\0', sizeof(*pTran));
         pTran->State = ismSTORE_MEM_STORETRANSACTIONSTATE_ACTIVE;
         pTran->OperationCount = 0;

         // Finally mark the data type so that restart recovery will trust it
         ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor) + sizeof(*pTran) - sizeof(ismStore_memStoreOperation_t));
         if (pDescriptor->NextHandle == ismSTORE_NULL_HANDLE)
         {
            break;
         }
         pStream->hStoreTranRsrv = pDescriptor->NextHandle;
         pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pDescriptor->NextHandle);
      }
   }

   pStream->MyGenId = ismSTORE_RSRV_GEN_ID;
   pStream->ActiveGenId = pMgmtHeader->ActiveGenId;
   pStream->MyGenIndex = pStream->ActiveGenIndex = pMgmtHeader->ActiveGenIndex;
   pStream->ChunksRsrv = pStream->ChunksInUse;
   *phStream = hStream;

   pGen = &ismStore_memGlobal.InMemGens[pMgmtHeader->ActiveGenIndex];
   if (ismStore_memGlobal.StreamsCount > ismStore_memGlobal.StreamsMinCount && pMgmtHeader->ActiveGenId > ismSTORE_MGMT_GEN_ID)
   {
      pGen->StreamCacheMaxCount[0] = ism_store_memGetStreamCacheCount(pGen, 0);
      pGen->StreamCacheBaseCount[0] = pGen->StreamCacheMaxCount[0] / 2;
      TRACE(9, "The stream cache size has been changed. StreamCacheMaxCount %u, StreamCacheBaseCount %u\n",
            pGen->StreamCacheMaxCount[0], pGen->StreamCacheBaseCount[0]);
   }
   pStream->CacheMaxGranulesCount = pGen->StreamCacheMaxCount[0];

   if ( ismStore_memGlobal.fEnablePersist )
   {
      if (!(pStream->pPersist = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,140),sizeof(ismStore_persistInfo_t))) )
      {
         rc = ISMRC_AllocateError;
         goto exit;
      }
      memset(pStream->pPersist, 0, sizeof(ismStore_persistInfo_t));
      iok++ ; 
      if ( (rc=posix_memalign(&pStream->pPersist->Buff, getpagesize(), ismStore_memGlobal.PersistBuffSize)) ) 
      {
         rc = ISMRC_AllocateError;
         goto exit;
      }
      memset(pStream->pPersist->Buff, 0, ismStore_memGlobal.PersistBuffSize);
      if ( (rc=posix_memalign(&pStream->pPersist->Buf0, getpagesize(), ismStore_memGlobal.PersistBuffSize)) ) 
      {
         rc = ISMRC_AllocateError;
         goto exit;
      }
      memset(pStream->pPersist->Buf0, 0, ismStore_memGlobal.PersistBuffSize);
      if ( (rc=posix_memalign(&pStream->pPersist->Buf1, getpagesize(), ismStore_memGlobal.PersistBuffSize)) ) 
      {
         rc = ISMRC_AllocateError;
         goto exit;
      }
      memset(pStream->pPersist->Buf1, 0, ismStore_memGlobal.PersistBuffSize);
      pStream->pPersist->BuffSize = ismStore_memGlobal.PersistBuffSize;
      iok++ ; 
      if ( (rc=ism_storePersist_openStream(pStream)) != ISMRC_OK )
      {
         rc = ISMRC_AllocateError;
         goto exit;
      }
   }

   if ( ismStore_memGlobal.fEnablePersist ) ism_storePersist_wrLock();
   ismStore_memGlobal.pStreams[hStream] = pStream;
   ismStore_memGlobal.StreamsCount++;
   ismStore_memGlobal.StreamsUpdCount++;
   if ( ismStore_memGlobal.fEnablePersist ) ism_storePersist_wrUnlock();

   TRACE(5, "A new store stream (hStream %u, fHighPerf %u, Flags 0x%x, ChunksInUse %u) has been allocated. StreamsCount %u, ActiveGenId %u (Index %u), CacheMaxGranulesCount %u\n",
         hStream, pStream->fHighPerf, flags, pStream->ChunksInUse, ismStore_memGlobal.StreamsCount, pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex, pStream->CacheMaxGranulesCount);

exit:
   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
   if (rc != ISMRC_OK)
   {
      if ( ismStore_memGlobal.fEnablePersist && pStream->pPersist )
      {
          if ( pStream->pPersist->Buff )
            ism_common_free_memaligned(ism_memory_store_misc,pStream->pPersist->Buff);
          if ( pStream->pPersist->Buf0 )
        	  ism_common_free_memaligned(ism_memory_store_misc,pStream->pPersist->Buf0);
          if ( pStream->pPersist->Buf1 )
        	  ism_common_free_memaligned(ism_memory_store_misc,pStream->pPersist->Buf1);
          ismSTORE_FREE(pStream->pPersist);
      }
      if (iok > 1)
      {
         if (iok > 2)
         {
            ismSTORE_MUTEX_DESTROY(&pStream->Mutex);
            if (iok > 3)
            {
               ismSTORE_COND_DESTROY(&pStream->Cond);
               if (ismStore_global.fHAEnabled && pStream->pHAChannel)
               {
                  ism_store_memHACloseChannel(pStream->pHAChannel, 1);
               }
            }
         }
         ismSTORE_FREE(pStream);
      }
      TRACE(1, "Failed to open a store stream. error code %d\n", rc);
   }

   TRACE(9, "Exit: %s. rc %d\n", __FUNCTION__, rc);
   return rc;
}


/*
 * Closes a stream and frees its store-transaction.
 */
int32_t ism_store_memCloseStream(ismStore_StreamHandle_t hStream)
{
   ismStore_memStream_t *pStream;
   int32_t rc = ISMRC_OK;

   if (ismStore_memGlobal.State != ismSTORE_STATE_ACTIVE && ismStore_memGlobal.State != ismSTORE_STATE_TERMINATING)
   {
      TRACE(1, "Failed to close the stream (hStream %u), because the store is not active\n", hStream);
      rc = ISMRC_StoreNotAvailable;
      return rc;
   }

   if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
     TRACE(1, "Failed to close the stream (hStream %u), because the stream is not valid\n", hStream);
     return rc;
   }

   pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];
   ismSTORE_MUTEX_LOCK(&pStream->Mutex);

   // If the stream is locked by the HASync, we have to wait until the new node synchronization completes
   while (pStream->fLocked && ismStore_memGlobal.State == ismSTORE_STATE_ACTIVE)
   {
      TRACE(7, "The %s is blocked , because the store is locked by the HA synchronization. hStream %u\n", __FUNCTION__, hStream);
      ismSTORE_COND_WAIT(&pStream->Cond, &pStream->Mutex);
   }

   if (pStream->MyGenId != ismSTORE_RSRV_GEN_ID)
   {
      TRACE(1, "Failed to close the stream (hStream %u), because it has a pending store-transaction. MyGenId %u, ActiveGenId %u, RefsCount %d\n",
            hStream, pStream->MyGenId, pStream->ActiveGenId, pStream->RefsCount);
      rc = ISMRC_ArgNotValid;
      ism_common_setErrorData(rc, "%s", "hStream");
   }
   else
   {
      if ( ismStore_memGlobal.fEnablePersist && pStream->pPersist )
      {
        int ret;
        ret = ism_storePersist_closeStream(pStream);
        TRACE(5, "(%s) ism_storePersist_closeStream returned %d for hStream %u\n", __FUNCTION__,ret,hStream);
      }

      if (rc == ISMRC_OK && pStream->hStoreTranHead != ismSTORE_NULL_HANDLE)
      {
         // Reset the local cache of the stream
         ism_store_memResetStreamCache(pStream, pStream->ActiveGenId);
         ism_store_memReturnPoolElements(NULL, pStream->hStoreTranHead, 0);
      }
   }
   ismSTORE_COND_BROADCAST(&pStream->Cond);
   ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);

   if (rc == ISMRC_OK)
   {
      pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
      if ( ismStore_memGlobal.fEnablePersist ) ism_storePersist_wrLock();

      ismSTORE_MUTEX_LOCK(&pStream->Mutex);
      if (ismStore_global.fHAEnabled && pStream->pHAChannel)
      {
         if (ism_store_memHACloseChannel(pStream->pHAChannel, 1) == StoreRC_OK)
         {
            pStream->pHAChannel = NULL;
         }
         else
         {
            TRACE(1, "Failed to close the stream (hStream %u) due to HA error\n", hStream);
            rc = ISMRC_StoreHAError;
         }
      }
      ismSTORE_COND_BROADCAST(&pStream->Cond);
      ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
      ismStore_memGlobal.pStreams[hStream] = NULL;
      ism_common_sleep(10000);  // 10 millisecond

      ismSTORE_MUTEX_DESTROY(&pStream->Mutex);
      ismSTORE_COND_DESTROY(&pStream->Cond);
      if ( ismStore_memGlobal.fEnablePersist && pStream->pPersist )
      {
          if ( pStream->pPersist->Buff )
            ism_common_free_memaligned(ism_memory_store_misc,pStream->pPersist->Buff);
          if ( pStream->pPersist->Buf0 )
        	  ism_common_free_memaligned(ism_memory_store_misc,pStream->pPersist->Buf0);
          if ( pStream->pPersist->Buf1 )
        	  ism_common_free_memaligned(ism_memory_store_misc,pStream->pPersist->Buf1);
        //ismSTORE_FREE(pStream->pPersist);
          pStream->hStream = H_STREAM_CLOSED ; 
          pStream->next = ismStore_memGlobal.dStreams ; 
          ismStore_memGlobal.dStreams = pStream ; 
      }
      else
      {
         ismSTORE_FREE(pStream);
      }
      ismStore_memGlobal.StreamsCount--;
      ismStore_memGlobal.StreamsUpdCount++;
      TRACE(5, "Stream %d has been closed. StreamsCount %d\n", hStream, ismStore_memGlobal.StreamsCount);
      if ( ismStore_memGlobal.fEnablePersist ) ism_storePersist_wrUnlock();
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
   }

   return rc;
}

/*
 * Reserve stream resources
 */
int32_t ism_store_memReserveStreamResources(ismStore_StreamHandle_t hStream, ismStore_Reservation_t *pReservation)
{
   ismStore_memGeneration_t *pGen;
   ismStore_memGenHeader_t  *pGenHeader;
   ismStore_memStream_t *pStream;
   uint8_t poolId = 0;
   int32_t rc = ISMRC_OK, nRsrvGranules;

   if (pReservation == NULL)
   {
      rc = ISMRC_NullArgument;
      return rc;
   }

   if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
      TRACE(1, "Failed to reserve stream resources, because the stream is not valid\n");
      return rc;
   }

   pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];

   if (pStream->MyGenId != ismSTORE_RSRV_GEN_ID)
   {
      TRACE(1, "Failed to reserve stream resources because the store-transaction is active (hStream %u, DataLength %lu).\n",
            hStream, pReservation->DataLength);
      rc = ISMRC_StoreTransActive;
      return rc;
   }

   if ((rc = ism_store_memSetStreamActivity(pStream, 1)) != ISMRC_OK)
   {
      return rc;
   }

   pGen = &ismStore_memGlobal.InMemGens[pStream->MyGenIndex];
   pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
   nRsrvGranules = pReservation->DataLength / pGenHeader->GranulePool[poolId].GranuleDataSizeBytes + pReservation->RecordsCount + pReservation->RefsCount;

   if (nRsrvGranules > pStream->CacheMaxGranulesCount)
   {
      pStream->CacheMaxGranulesCount = nRsrvGranules;
   }

   TRACE(9, "A stream resource reservation is being processed. hStream %u, DataLength %lu, RecordsCount %u, RefsCount %u, nRsrvGranules %u, CacheGranulesCount %u, CacheMaxGranulesCount %u (%u)\n",
         hStream, pReservation->DataLength, pReservation->RecordsCount, pReservation->RefsCount, nRsrvGranules, pStream->CacheGranulesCount, pStream->CacheMaxGranulesCount, pGen->PoolMaxCount[0]);

   while (nRsrvGranules > pStream->CacheGranulesCount && rc == ISMRC_OK)
   {
      rc = ism_store_memGetPoolElements(pStream,
                                        pGen,
                                        0,
                                        ismSTORE_DATATYPE_FREE_GRANULE,
                                        0, 0, 0, nRsrvGranules,
                                        NULL, NULL);
      if (rc == ISMRC_StoreGenerationFull)
      {
         if ((rc = ism_store_memSetStreamActivity(pStream, 0)) != ISMRC_OK)
         {
            return rc;
         }
         if ((rc = ism_store_memSetStreamActivity(pStream, 1)) != ISMRC_OK)
         {
            return rc;
         }
         pGen = &ismStore_memGlobal.InMemGens[pStream->MyGenIndex];
         pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
         nRsrvGranules = pReservation->DataLength / pGenHeader->GranulePool[poolId].GranuleDataSizeBytes + pReservation->RecordsCount + pReservation->RefsCount;
      }
   }

   TRACE(9, "A stream resource reservation has been completed. hStream %u, DataLength %lu, RecordsCount %u, RefsCount %u, nRsrvGranules %u, CacheGranulesCount %u, rc %d\n",
         hStream, pReservation->DataLength, pReservation->RecordsCount, pReservation->RefsCount, nRsrvGranules, pStream->CacheGranulesCount, rc);

   return rc;
}

/*
 * Cancel resource reservation without trans ops
 */
int32_t ism_store_memCancelResourceReservation(ismStore_StreamHandle_t hStream)
{
   ismStore_memStream_t *pStream;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
      TRACE(1, "Failed to retrieve stream operations count, because the stream is not valid\n");
      return rc;
   }

   pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];

   if (pStream->MyGenId == ismSTORE_RSRV_GEN_ID)
   {
      TRACE(6, "No active store-transaction (hStream %u).\n",hStream);
      return rc;
   }

   if (pStream->hStoreTranHead != ismSTORE_NULL_HANDLE)
   {
      pDescriptor = pStream->pDescrTranHead;
      pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);
      if (pTran->OperationCount == 0)
      {
         rc = ism_store_memSetStreamActivity(pStream,0);
      }
      else
      {
         TRACE(1, "None empty active store-transaction (hStream %u, nOps %u).\n",hStream,pTran->OperationCount);
         rc = ISMRC_StoreTransActive;
      }
   }

   return rc;
}


int32_t ism_store_memStartTransaction(ismStore_StreamHandle_t hStream, int *fIsActive)
{
   ismStore_memStream_t *pStream;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
      TRACE(1, "Failed to reserve stream resources, because the stream is not valid\n");
      return rc;
   }

   pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];

   if (pStream->MyGenId != ismSTORE_RSRV_GEN_ID)
   {
      if ( fIsActive ) *fIsActive = 1;
      return rc;
   }
   else
   {
      if ( fIsActive ) *fIsActive = 0;
   }

   return ism_store_memSetStreamActivity(pStream, 1);
}

int32_t ism_store_memCancelTransaction(ismStore_StreamHandle_t hStream)
{
   ismStore_memStream_t *pStream;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
      TRACE(1, "Failed to retrieve stream operations count, because the stream is not valid\n");
      return rc;
   }

   pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];

   if (pStream->MyGenId == ismSTORE_RSRV_GEN_ID)
   {
      TRACE(6, "No active store-transaction (hStream %u).\n",hStream);
      return rc;
   }

   if (pStream->hStoreTranHead != ismSTORE_NULL_HANDLE)
   {
      pDescriptor = pStream->pDescrTranHead;
      pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);
      if (pTran->OperationCount == 0)
      {
         rc = ism_store_memSetStreamActivity(pStream,0);
      }
      else
      {
         TRACE(1, "None empty active store-transaction (hStream %u, nOps %u).\n",hStream,pTran->OperationCount);
         rc = ISMRC_StoreTransActive;
      }
   }

   return rc;
}

int ism_store_memGetAsyncCBStats(uint32_t *pTotalReadyCBs, uint32_t *pTotalWaitingCBs,
                                 uint32_t *pNumThreads,
                                 ismStore_AsyncThreadCBStats_t *pCBThreadStats)
{
   int rc = StoreRC_OK;

   if ( ismStore_memGlobal.fEnablePersist)
   {
      rc = ism_storePersist_getAsyncCBStats(pTotalReadyCBs, pTotalWaitingCBs,
                                            pNumThreads,
                                            pCBThreadStats);
   }
   else
   {
      if (pTotalReadyCBs != NULL)
      {
         *pTotalReadyCBs = 0;
      }
      if (pTotalWaitingCBs != NULL)
      {
         *pTotalWaitingCBs = 0;
      }

      memset(pCBThreadStats, 0, *pNumThreads * sizeof(ismStore_AsyncThreadCBStats_t));
      *pNumThreads = 0;
   }

   return rc;
}

/*
 * Return the number of un-committed operations on the Stream
 */
int32_t ism_store_memGetStreamOpsCount(ismStore_StreamHandle_t hStream, uint32_t *pOpsCount)
{
   ismStore_memStream_t *pStream;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_Handle_t hChunk;
   int32_t rc = ISMRC_OK, count = 0;

   if (pOpsCount == NULL)
   {
      rc = ISMRC_NullArgument;
      return rc;
   }

   if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
      TRACE(1, "Failed to retrieve stream operations count, because the stream is not valid\n");
      return rc;
   }

   pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];
   if ((pDescriptor = pStream->pDescrTranHead) )
   {
      for(;;)
      {
         pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);
         if (pTran->OperationCount == 0) { break; }
         count += pTran->OperationCount;
         if ((hChunk = pDescriptor->NextHandle) == ismSTORE_NULL_HANDLE) { break; }
         pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hChunk);
      }
   }

   *pOpsCount = count;
   return ISMRC_OK;
}

static int32_t ism_store_memSetStreamActivity(ismStore_memStream_t *pStream, uint8_t fIsActive)
{
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_memJob_t job;
   ismStore_GenId_t genId=ismSTORE_RSRV_GEN_ID;
   uint8_t genIndex=0;
   int32_t rc = ISMRC_OK;

   ismSTORE_MUTEX_LOCK(&pStream->Mutex);
   if (fIsActive)
   {
      if (pStream->RefsCount)
      {
         pStream->RefsCount++;
      }
      else
      {
         // Wait for an active generation
         while (ismStore_memGlobal.State == ismSTORE_STATE_ACTIVE && (pStream->ActiveGenId == ismSTORE_RSRV_GEN_ID || pStream->fLocked))
         {
            TRACE(9, "There is no active generation (ActiveGenId %u) for the store-transaction or the store is locked (fLocked %u)\n", pStream->ActiveGenId, pStream->fLocked);
            ism_common_backHome();
            ismSTORE_COND_WAIT(&pStream->Cond, &pStream->Mutex);
            ism_common_going2work();
         }
         if (ismStore_memGlobal.State == ismSTORE_STATE_ACTIVE)
         {
            pStream->MyGenId = pStream->ActiveGenId;
            pStream->MyGenIndex = pStream->ActiveGenIndex;
            pStream->RefsCount++;
   
            if (pStream->hStoreTranHead != ismSTORE_NULL_HANDLE)
            {
               pDescriptor = pStream->pDescrTranHead;
               pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);
               pTran->GenId = pStream->MyGenId;
               // No need to call WRITE_BACK for the pTran, because we call WRITE_BACK just before
               // the commit/rollback starts
               //ADR_WRITE_BACK(&pTran->GenId, sizeof(pTran->GenId));
            }
         }
         else if (ismStore_memGlobal.State == ismSTORE_STATE_DISKERROR)
         {
            rc = ISMRC_StoreDiskError;
         }
         else if (ismStore_memGlobal.State == ismSTORE_STATE_ALLOCERROR)
         {
            rc = StoreRC_AllocateError;
         }
         else
         {
            rc = ISMRC_StoreNotAvailable;
         }
      }
   }
   else // fIsActive = 0
   {
      if (pStream->RefsCount == 0)
      {
         if (ismStore_memGlobal.State == ismSTORE_STATE_DISKERROR)
         {
            rc = ISMRC_StoreDiskError;
         }
         else
         {
            TRACE(5, "Failed to set the stream activity, because the stream state is not valid. hStream %u, MyGenId %u, ActiveGenId %u, ChunksInUse %u, StoreState %u\n",
                  pStream->hStream, pStream->MyGenId, pStream->ActiveGenId, pStream->ChunksInUse, ismStore_memGlobal.State);
            rc = ISMRC_Error;
         }
      }
      else if (--pStream->RefsCount == 0)
      {
         if ( pStream->fLocked )
         {
           TRACE(5,"Stream %u is terminating its store transaction in genId %u\n",pStream->hStream,pStream->MyGenId);
         }
         if (pStream->MyGenId != pStream->ActiveGenId && pStream->MyGenId != ismSTORE_RSRV_GEN_ID)
         {
            genId = pStream->MyGenId;
            genIndex = pStream->MyGenIndex;
            TRACE(8, "A CLOSE_PENDING generation Id %u (Index %d) has been released by the store-transaction\n", genId, genIndex);

            // Reset the local cache of the stream
            ism_store_memResetStreamCache(pStream, genId);
            pStream->CacheMaxGranulesCount = ismStore_memGlobal.InMemGens[genIndex].StreamCacheMaxCount[0];
         }
         pStream->MyGenId = ismSTORE_RSRV_GEN_ID;
         pStream->MyGenIndex = 0;
      }
   }
   ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);

   if (genId != ismSTORE_RSRV_GEN_ID)
   {
      memset(&job, '\0', sizeof(job));
      job.JobType = StoreJob_WriteGeneration;
      job.Generation.GenId = genId;
      job.Generation.GenIndex = genIndex;
      ism_store_memAddJob(&job);
   }

   return rc;
}

/*
 * Resets the local cache of the stream.
 */
static void ism_store_memResetStreamCache(ismStore_memStream_t *pStream, ismStore_GenId_t genId)
{
   TRACE(5, "The local cache of stream (hStream %u, CacheGranulesCount %u, hCacheHead 0x%lx) in generation %u has been reset\n",
         pStream->hStream, pStream->CacheGranulesCount, pStream->hCacheHead, genId);
   pStream->CacheGranulesCount = 0;
   pStream->hCacheHead = ismSTORE_NULL_HANDLE;
}

static uint32_t ism_store_memGetStreamCacheCount(ismStore_memGeneration_t *pGen, uint8_t poolId)
{
   double streamCacheCount;

   // The minimum number of streams for the calculation is ismStore_memGlobal.StreamsMinCount
   // The internal stream #0 is not counted, therefore we use StreamsCount - 1.
   int Id = poolId < ismSTORE_GRANULE_POOLS_COUNT ? poolId : ismSTORE_GRANULE_POOLS_COUNT-1 ; 
   streamCacheCount = (ismStore_memGlobal.StreamsCachePct / 100) * pGen->PoolMaxCount[Id] / (ismStore_memGlobal.StreamsCount < ismStore_memGlobal.StreamsMinCount + 1 ? ismStore_memGlobal.StreamsMinCount : ismStore_memGlobal.StreamsCount - 1);
   return (uint32_t)streamCacheCount;
}

/*
 * Validates a stream handle.
 */
static int32_t ism_store_memValidateStream(ismStore_StreamHandle_t hStream)
{
   if (hStream >= ismStore_memGlobal.StreamsSize || ismStore_memGlobal.pStreams[hStream] == NULL)
   {
      TRACE(1, "Stream handle %u is not valid. StreamsSize %d, StreamsCount %d\n",
            hStream, ismStore_memGlobal.StreamsSize, ismStore_memGlobal.StreamsCount);
      ism_common_setErrorData(ISMRC_ArgNotValid, "%s", "hStream");
      return ISMRC_ArgNotValid;
   }
   return ISMRC_OK;
}

/*
 * Validates a store reference context.
 */
static int32_t ism_store_memValidateRefCtxt(ismStore_memReferenceContext_t *pRefCtxt)
{
   ismStore_memDescriptor_t *pOwnerDescriptor;
   ismStore_memSplitItem_t *pSplit;
   int32_t rc = ISMRC_OK;

   do
   {
      if (pRefCtxt == NULL)
      {
         TRACE(1, "The reference context is not valid\n");
         rc = ISMRC_NullArgument;
         break;
      }

      pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pRefCtxt->OwnerHandle);
      if (pOwnerDescriptor == NULL)
      {
         TRACE(1, "The reference context of owner 0x%lx is not valid.\n", pRefCtxt->OwnerHandle);
         rc = ISMRC_ArgNotValid;
         ism_common_setErrorData(rc, "%s", "pRefCtxt");
         break;
      }

      if (!ismSTORE_IS_SPLITITEM(pOwnerDescriptor->DataType))
      {
         TRACE(1, "The reference context of owner 0x%lx is not valid.\n", pRefCtxt->OwnerHandle);
         rc = ISMRC_ArgNotValid;
         ism_common_setErrorData(rc, "%s", "pRefCtxt");
         break;
      }

      pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);
      if (pSplit->pRefCtxt != (uint64_t)pRefCtxt)
      {
         TRACE(1, "The reference context of owner 0x%lx is not valid.\n", pRefCtxt->OwnerHandle);
         rc = ISMRC_ArgNotValid;
         ism_common_setErrorData(rc, "%s", "pRefCtxt");
         break;
      }
   } while (0);

   return rc;
}

/*
 * Validates a store state context.
 */
static int32_t ism_store_memValidateStateCtxt(ismStore_memStateContext_t *pStateCtxt)
{
   ismStore_memDescriptor_t *pOwnerDescriptor;
   ismStore_memSplitItem_t *pSplit;
   int32_t rc = ISMRC_OK;

   do
   {
      if (pStateCtxt == NULL)
      {
         TRACE(1, "The state context is not valid\n");
         rc = ISMRC_NullArgument;
         break;
      }

      pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pStateCtxt->OwnerHandle);
      if (pOwnerDescriptor == NULL)
      {
         TRACE(1, "The state context of owner 0x%lx is not valid.\n", pStateCtxt->OwnerHandle);
         rc = ISMRC_ArgNotValid;
         ism_common_setErrorData(rc, "%s", "pStateCtxt");
         break;
      }

      if (!ismSTORE_IS_SPLITITEM(pOwnerDescriptor->DataType))
      {
         TRACE(1, "The state context of owner 0x%lx is not valid.\n", pStateCtxt->OwnerHandle);
         rc = ISMRC_ArgNotValid;
         ism_common_setErrorData(rc, "%s", "pStateCtxt");
         break;
      }

      pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);
      if (pSplit->pStateCtxt != (uint64_t)pStateCtxt)
      {
         TRACE(1, "The state context of owner 0x%lx is not valid.\n", pStateCtxt->OwnerHandle);
         rc = ISMRC_ArgNotValid;
         ism_common_setErrorData(rc, "%s", "pStateCtxt");
         break;
      }
   } while (0);

   return rc;
}

/*
 * Validates a store handle.
 */
static int32_t ism_store_memValidateHandle(ismStore_Handle_t handle)
{
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_GenId_t genId = ismSTORE_EXTRACT_GENID(handle);
   ismStore_Handle_t offset = ismSTORE_EXTRACT_OFFSET(handle);
   int32_t rc = ISMRC_OK;

   if (genId < ismSTORE_MGMT_GEN_ID || genId > ismSTORE_MAX_GEN_ID)
   {
      TRACE(1, "The handle 0x%lx (GenId %u, Offset 0x%lx) is not valid.\n", handle, genId, offset);
      rc = ISMRC_ArgNotValid;
      ism_common_setErrorData(rc, "%s", "handle");
      return rc;
   }

   if ((pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle)))
   {
      if (pDescriptor->DataType < ismSTORE_DATATYPE_MIN_EXTERNAL ||
          pDescriptor->DataType > ismSTORE_DATATYPE_MAX_EXTERNAL)
      {
         if (pDescriptor == (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle))
         {
            TRACE(1, "The handle 0x%lx (GenId %u, Offset 0x%lx) is not valid. DataType 0x%x\n",
                  handle, genId, offset, pDescriptor->DataType);
            rc = ISMRC_ArgNotValid;
            ism_common_setErrorData(rc, "%s", "handle");
         }
      }
   }
  #if 0
   else
   {
      ismStore_memGenMap_t *pGenMap;
      struct timespec reltime={0,100000000};
      ism_time_t timeout;
      uint8_t poolId, bitpos;
      uint32_t idx, pos;
      uint64_t *pBitMap;
      pGenMap = ismStore_memGlobal.pGensMap[genId];

      if (pGenMap == NULL)
      {
         TRACE(1, "The handle 0x%lx (GenId %u, Offset 0x%lx) is not valid. pGenMap=NULL\n", handle, genId, offset);
         rc = ISMRC_ArgNotValid;
         ism_common_setErrorData(rc, "%s", "handle");
      }
      else
      {
         pthread_mutex_lock(&pGenMap->Mutex);

         if (pGenMap->GranulesMapCount == 0)
         {
            timeout = ism_common_monotonicTimeNanos() + (30 * 1000000000L);
            do
            {
               if (ism_store_memGetGenerationById(genId) == NULL)
               {
                  if (!pGenMap->fBitmapWarn)
                  {
                     TRACE(1, "Failed to validate handle because the generation (GenId %u) is not in the memory and has no bitmap\n", genId);
                     pGenMap->fBitmapWarn = 1;
                  }
                  pthread_mutex_unlock(&pGenMap->Mutex);
                  return rc;
               }
               ism_common_cond_timedwait(&pGenMap->Cond, &pGenMap->Mutex, &reltime, 1);
            }
            while (pGenMap->GranulesMapCount == 0 && timeout > ism_common_monotonicTimeNanos() && ismStore_memGlobal.State == ismSTORE_STATE_ACTIVE);
         }

         if (ismStore_memGlobal.State != ismSTORE_STATE_ACTIVE)
         {
            TRACE(1, "Failed to validate handle 0x%lx, because the store is being terminated. State %u\n", handle, ismStore_memGlobal.State);
            rc = ISMRC_StoreNotAvailable;
         }
         else if (pGenMap->GranulesMapCount == 0)
         {
            TRACE(1, "Failed to validate handle 0x%lx because the generation (GenId %u) has no bitmap\n", handle, genId);
            rc = ISMRC_ArgNotValid;
            ism_common_setErrorData(rc, "%s", "handle");
         }
         else
         {
            poolId = ism_store_memOffset2PoolId(pGenMap, offset);
            idx = (offset - pGenMap->GranulesMap[poolId].Offset) / pGenMap->GranulesMap[poolId].GranuleSizeBytes;
            pos = idx >> 6;
            bitpos = (uint8_t)(idx & 0x3f);
            pBitMap = pGenMap->GranulesMap[poolId].pBitMap[ismSTORE_BITMAP_LIVE_IDX];

            if ((pBitMap[pos] & ismStore_GenMapSetMask[bitpos]) == 0)
            {
               TRACE(1, "The handle 0x%lx (GenId %u, Offset 0x%lx) is not valid. PoolId %u, Pos %u, Mask 0x%lx\n",
                     handle, genId, offset, poolId, pos, pBitMap[pos]);
                rc = ISMRC_ArgNotValid;
                ism_common_setErrorData(rc, "%s", "handle");
             }
         }
         pthread_mutex_unlock(&pGenMap->Mutex);
      }
   }
  #endif

   return rc;
}

/*
 * Validates a store reference handle.
 */
static int32_t ism_store_memValidateRefHandle(ismStore_Handle_t refHandle, uint64_t orderId, ismStore_Handle_t hOwnerHandle)
{
   ismStore_memDescriptor_t *pDescriptor, *pOwnerDescriptor;
   ismStore_memReferenceChunk_t *pRefChunk;
   ismStore_memSplitItem_t *pSplit;
   ismStore_GenId_t genId;
   ismStore_Handle_t offset, chunkHandle;
   ismStore_memGenMap_t *pGenMap;
   int32_t rc = ISMRC_OK;

   genId = ismSTORE_EXTRACT_GENID(refHandle);
   offset = ismSTORE_EXTRACT_OFFSET(refHandle);
   if (genId <= ismSTORE_MGMT_GEN_ID || genId > ismSTORE_MAX_GEN_ID)
   {
      TRACE(1, "The reference handle 0x%lx (GenId %u, Offset 0x%lx, OrderId %lu. hOwnerHandle 0x%lx) is not valid.\n",
            refHandle, genId, offset, orderId, hOwnerHandle);
      rc = ISMRC_ArgNotValid;
      ism_common_setErrorData(rc, "%s", "handle");
      return rc;
   }

   pGenMap = ismStore_memGlobal.pGensMap[genId];
   if (pGenMap == NULL)
   {
      TRACE(1, "The reference handle 0x%lx (GenId %u, Offset 0x%lx, OrderId %lu, hOwnerHandle 0x%lx) is not valid. pGenMap=NULL\n",
            refHandle, genId, offset, orderId, hOwnerHandle);
      rc = ISMRC_ArgNotValid;
      ism_common_setErrorData(rc, "%s", "handle");
      return rc;
   }

   if (offset >= pGenMap->MemSizeBytes)
   {
      TRACE(1, "The reference handle 0x%lx (GenId %u, Offset 0x%lx, OrderId %lu, hOwnerHandle 0x%lx) is not valid. MemSizeBytes %lu\n",
            refHandle, genId, offset, orderId, hOwnerHandle, pGenMap->MemSizeBytes);
      rc = ISMRC_ArgNotValid;
      ism_common_setErrorData(rc, "%s", "handle");
      return rc;
   }

   if ((pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hOwnerHandle)) == NULL)
   {
      TRACE(1, "The reference handle 0x%lx (GenId %u, Offset 0x%lx, OrderId %lu, hOwnerHandle 0x%lx) is not valid.\n",
            refHandle, genId, offset, orderId, hOwnerHandle);
      rc = ISMRC_ArgNotValid;
      ism_common_setErrorData(rc, "%s", "handle");
      return rc;
   }

   pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);
   if (orderId < pSplit->MinActiveOrderId)
   {
      TRACE(1, "The reference handle 0x%lx (GenId %u, Offset 0x%lx, OrderId %lu, hOwnerHandle 0x%lx, OwnerType 0x%x) is not valid. MinActiveOrderId %lu\n",
            refHandle, genId, offset, orderId, hOwnerHandle, pOwnerDescriptor->DataType, pSplit->MinActiveOrderId);
      rc = ISMRC_ArgNotValid;
      ism_common_setErrorData(rc, "%s", "handle");
      return rc;
   }

   chunkHandle = ismSTORE_BUILD_HANDLE(genId, ismSTORE_ROUND(offset, pGenMap->GranulesMap[0].GranuleSizeBytes));
   if ((refHandle - chunkHandle) < ismSTORE_FIRSTREF_OFFSET || ((refHandle - chunkHandle - ismSTORE_FIRSTREF_OFFSET) % sizeof(ismStore_memReference_t) != 0))
   {
      TRACE(1, "The reference handle 0x%lx (GenId %u, Offset 0x%lx, OrderId %lu, hOwnerHandle 0x%lx, OwnerType 0x%x) is not valid. GranuleSizeBytes %u, ChunkHandle 0x%lx, MinActiveOrderId %lu\n",
            refHandle, genId, offset, orderId, hOwnerHandle, pOwnerDescriptor->DataType,
            pGenMap->GranulesMap[0].GranuleSizeBytes, chunkHandle, pSplit->MinActiveOrderId);
      rc = ISMRC_ArgNotValid;
      ism_common_setErrorData(rc, "%s", "handle");
      return rc;
   }

   if ((pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(chunkHandle)))
   {
      if ((pDescriptor->DataType & (~ismSTORE_DATATYPE_NOT_PRIMARY)) != ismSTORE_DATATYPE_REFERENCES)
      {
         if (pDescriptor == (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(chunkHandle))
         {
            TRACE(1, "The reference handle 0x%lx (GenId %u, Offset 0x%lx, OrderId %lu, hOwnerHandle 0x%lx, OwnerType 0x%x) is not valid. ChunkHandle 0x%lx, DataType 0x%x, MinActiveOrderId %lu\n",
                  refHandle, genId, offset, orderId, hOwnerHandle, pOwnerDescriptor->DataType,
                  chunkHandle, pDescriptor->DataType, pSplit->MinActiveOrderId);
            rc = ISMRC_ArgNotValid;
            ism_common_setErrorData(rc, "%s", "handle");
            return rc;
         }
      }

      pRefChunk = (ismStore_memReferenceChunk_t *)(pDescriptor + 1);
      if (orderId < pRefChunk->BaseOrderId || orderId >= pRefChunk->BaseOrderId + pRefChunk->ReferenceCount)
      {
         if (pDescriptor == (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(chunkHandle))
         {
            TRACE(1, "The reference handle 0x%lx (GenId %u, Offset 0x%lx, OrderId %lu, hOwnerHandle 0x%lx, OwnerType 0x%x) is not valid. BaseOrderId %lu, ReferenceCount %u, MinActiveOrderId %lu\n",
                  refHandle, genId, offset, orderId, hOwnerHandle, pOwnerDescriptor->DataType,
                  pRefChunk->BaseOrderId, pRefChunk->ReferenceCount, pSplit->MinActiveOrderId);
            rc = ISMRC_ArgNotValid;
            ism_common_setErrorData(rc, "%s", "handle");
            return rc;
         }
      }

      if (pRefChunk->OwnerHandle != hOwnerHandle)
      {
         if (pDescriptor == (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(chunkHandle))
         {
            TRACE(1, "The reference handle 0x%lx (GenId %u, Offset 0x%lx, OrderId %lu, hOwnerHandle 0x%lx, OwnerType 0x%x) is not valid. OwnerHandle 0x%lx, MinActiveOrderId %lu\n",
                  refHandle, genId, offset, orderId, hOwnerHandle, pOwnerDescriptor->DataType, pRefChunk->OwnerHandle, pSplit->MinActiveOrderId);
            rc = ISMRC_ArgNotValid;
            ism_common_setErrorData(rc, "%s", "handle");
            return rc;
         }
      }
   }

   return rc;
}

static int32_t ism_store_memEnsureStoreTransAllocation(ismStore_memStream_t *pStream, ismStore_memDescriptor_t **pDesc)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memDescriptor_t *pDescriptor, *pCurrentDescriptor=NULL;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_Handle_t handle;
   int32_t rc = ISMRC_OK;

   if (pStream->MyGenId == ismSTORE_RSRV_GEN_ID)
   {
      // Set the active generation of that stream
      if ((rc = ism_store_memSetStreamActivity(pStream, 1)) != ISMRC_OK)
      {
         return rc;
      }
   }

   if (pStream->hStoreTranCurrent == ismSTORE_NULL_HANDLE)
   {
      TRACE(1, "Failed to allocate space for a store-transaction due to internal error. hStream %u, MyGenId %u\n", pStream->hStream, pStream->MyGenId);
      return ISMRC_Error;
   }

   pCurrentDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pStream->hStoreTranCurrent);
   if (!pCurrentDescriptor || (pCurrentDescriptor->DataType & (~ismSTORE_DATATYPE_NOT_PRIMARY)) != ismSTORE_DATATYPE_STORE_TRAN)
   {
      pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
      TRACE(1, "Failed to allocate space for a store-transaction due to internal error. hStream %u, MyGenId %u, hStoreTranCurrent 0x%lx, DataType 0x%x, ActiveGenId %u, ActiveGenIndex %u\n",
            pStream->hStream, pStream->MyGenId,  pStream->hStoreTranCurrent, (pCurrentDescriptor ? pCurrentDescriptor->DataType : 0), pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex);
      return ISMRC_Error;
   }

   pTran = (ismStore_memStoreTransaction_t *)(pCurrentDescriptor + 1);
   if (pTran->OperationCount >= ismStore_memGlobal.MgmtGen.MaxTranOpsPerGranule)
   {
      if (pCurrentDescriptor->NextHandle)
      {
         pStream->hStoreTranCurrent = pCurrentDescriptor->NextHandle;
         pCurrentDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pStream->hStoreTranCurrent);
         if (!pCurrentDescriptor || (pCurrentDescriptor->DataType & (~ismSTORE_DATATYPE_NOT_PRIMARY)) != ismSTORE_DATATYPE_STORE_TRAN)
         {
            pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
            TRACE(1, "Failed to allocate space for a store-transaction due to internal error. hStream %u, MyGenId %u, hStoreTranCurrent 0x%lx, DataType 0x%x, OperationCount %u, ActiveGenId %u, ActiveGenIndex %u\n",
                  pStream->hStream, pStream->MyGenId,  pStream->hStoreTranCurrent, (pCurrentDescriptor ? pCurrentDescriptor->DataType : 0), pTran->OperationCount, pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex);
            return ISMRC_Error;
         }

         pTran = (ismStore_memStoreTransaction_t *)(pCurrentDescriptor + 1);
         pTran->State = ismSTORE_MEM_STORETRANSACTIONSTATE_ACTIVE;
         pTran->GenId = pStream->MyGenId;
         if (pTran->OperationCount > 0)
         {
            pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
            TRACE(1, "Failed to allocate space for a store-transaction due to internal error. hStream %u, MyGenId %u, OperationCount %u, ActiveGenId %u, ActiveGenIndex %u\n",
                  pStream->hStream, pStream->MyGenId, pTran->OperationCount, pMgmtHeader->ActiveGenId, pMgmtHeader->ActiveGenIndex);
            return ISMRC_Error;
         }
      }
      else
      {
         rc = ism_store_memGetMgmtPoolElements(NULL,
                                               ismSTORE_MGMT_POOL_INDEX,
                                               ismSTORE_DATATYPE_STORE_TRAN,
                                               0, 0, ismSTORE_SINGLE_GRANULE,
                                               &handle, &pDescriptor);

         if (rc != ISMRC_OK)
         {
            return rc;
         }

         // Initialize the store-transaction
         pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);
         memset(pTran, '\0', sizeof(*pTran));
         pTran->State = ismSTORE_MEM_STORETRANSACTIONSTATE_ACTIVE;
         pTran->GenId = pStream->MyGenId;
         pTran->OperationCount = 0;

         pDescriptor->DataType = ismSTORE_DATATYPE_STORE_TRAN | ismSTORE_DATATYPE_NOT_PRIMARY;
         // Mark the data type so that restart recovery will trust it
         ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor) + sizeof(*pTran) - sizeof(ismStore_memStoreOperation_t));

         pCurrentDescriptor->NextHandle = handle;
         // To minimize the number of WRITE_BACK operations, we do WRITE_BACK only
         // when the transaction granule becomes full or at the end of the store-transaction.
         ADR_WRITE_BACK(pCurrentDescriptor, sizeof(*pCurrentDescriptor) + pCurrentDescriptor->DataLength);

         pStream->ChunksInUse++;
         pStream->hStoreTranCurrent = handle;
         pCurrentDescriptor = pDescriptor;

         TRACE(9, "A new chunk has been allocated for store transaction %lu. ChunksInUse %u, ChunksRsrv %u.\n",
               pStream->hStoreTranHead, pStream->ChunksInUse, pStream->ChunksRsrv);
      }
   }

   if (pDesc)
   {
      *pDesc = pCurrentDescriptor;
   }

   return rc;
}

int32_t ism_store_getReferenceStatistics(ismStore_Handle_t hOwnerHandle, ismStore_ReferenceStatistics_t *pRefStats)
{
   ismStore_memDescriptor_t *pOwnerDescriptor;
   ismStore_memSplitItem_t *pSplit;

   if (hOwnerHandle == ismSTORE_NULL_HANDLE || pRefStats == NULL)
   {
      TRACE(1, "Failed to get reference statistics for owner 0x%lx, because the parameters are not valid\n", hOwnerHandle);
      return ISMRC_NullArgument;
   }

   pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hOwnerHandle);
   if (pOwnerDescriptor == NULL)
   {
      TRACE(1, "Failed to get reference statistics for owner 0x%lx, because the owner handle is not valid\n", hOwnerHandle);
      return ISMRC_ArgNotValid;
   }

   if (!ismSTORE_IS_SPLITITEM(pOwnerDescriptor->DataType))
   {
      TRACE(1, "Failed to get reference statistics for owner 0x%lx, because the owner record type 0x%x is not valid\n", hOwnerHandle, pOwnerDescriptor->DataType);
      return ISMRC_ArgNotValid;
   }

   pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);
   if (!pSplit->pRefCtxt)
   {
      TRACE(1, "Failed to get reference statistics for owner 0x%lx, because there is no reference context for this owner\n", hOwnerHandle);
      return ISMRC_ArgNotValid;
   }

   ism_store_fillReferenceStatistics(pSplit, pRefStats);

   return ISMRC_OK;
}

static void ism_store_fillReferenceStatistics(ismStore_memSplitItem_t *pSplit, ismStore_ReferenceStatistics_t *pRefStats)
{
   ismStore_memRefGen_t *pRefGen, *pRefGenPrev;
   ismStore_memReferenceContext_t *pRefCtxt = (ismStore_memReferenceContext_t *)pSplit->pRefCtxt;
   uint64_t highestOrderId = 0;

   memset(pRefStats, '\0', sizeof(*pRefStats));
   if ((pRefGen = pRefCtxt->pRefGenHead))
   {
      pRefStats->LowestGenId = ismSTORE_EXTRACT_GENID(pRefGen->hReferenceHead);
      // Find the last GenId and the HighestOrderId
      for (pRefGenPrev = NULL; pRefGen; pRefGenPrev = pRefGen, pRefGen = pRefGen->Next)
      {
         // To be on the safe side we check that the HighestOrderId is ordered across generations
         if (pRefGen->HighestOrderId > highestOrderId)
         {
            highestOrderId = pRefGen->HighestOrderId;
         }
         else if (pRefGenPrev)
         {
            TRACE(7, "The HighestOrderId for owner (Handle 0x%lx, Version %u, MinActiveOrderId %lu, HighestOrderId %lu, %lu) is not ordered. " \
                  "[GenId %u, LowestOrderId %lu, HighestOrderId %lu] > [GenId %u, LowestOrderId %lu, HighestOrderId %lu]\n",
                  pRefCtxt->OwnerHandle, pRefCtxt->OwnerVersion, pSplit->MinActiveOrderId, pRefCtxt->HighestOrderId, highestOrderId,
                  ismSTORE_EXTRACT_GENID(pRefGenPrev->hReferenceHead), pRefGenPrev->LowestOrderId, pRefGenPrev->HighestOrderId,
                  ismSTORE_EXTRACT_GENID(pRefGen->hReferenceHead), pRefGen->LowestOrderId, pRefGen->HighestOrderId);
         }
      }
      pRefStats->HighestGenId = ismSTORE_EXTRACT_GENID(pRefGenPrev->hReferenceHead);
      pRefStats->HighestOrderId = highestOrderId;
      //pRefCtxt->pRefGenLast = pRefGenPrev;
   }

   if (pRefCtxt->HighestOrderId > pRefStats->HighestOrderId)
   {
      pRefStats->HighestOrderId = pRefCtxt->HighestOrderId;
   }

   if ((pRefStats->MinimumActiveOrderId = pSplit->MinActiveOrderId) > pRefStats->HighestOrderId)
   {
      pRefStats->HighestOrderId = pRefStats->MinimumActiveOrderId;
   }

   TRACE(7, "Reference context statistics for owner 0x%lx: MinActiveOrderId %lu, HighestOrderId %lu (%lu, %lu), LowestGenId %u, HighestGenId %u\n",
         pRefCtxt->OwnerHandle, pRefStats->MinimumActiveOrderId, pRefStats->HighestOrderId, highestOrderId, pRefCtxt->HighestOrderId, pRefStats->LowestGenId, pRefStats->HighestGenId);
}

/*
 * Opens a reference context, used to track references for a specific owner handle.
 */
int32_t ism_store_memOpenReferenceContext(ismStore_Handle_t hOwnerHandle,
                                          ismStore_ReferenceStatistics_t *pRefStats,
                                          void **phRefCtxt)
{
   ismStore_memDescriptor_t *pOwnerDescriptor;
   ismStore_memSplitItem_t *pSplit;
   ismStore_memReferenceContext_t *pRefCtxt;
   int32_t rc = ISMRC_OK;

   TRACE(9, "Open a reference context for owner 0x%lx\n", hOwnerHandle);

   do
   {
      if (hOwnerHandle == ismSTORE_NULL_HANDLE || phRefCtxt == NULL)
      {
         TRACE(1, "Failed to open a reference context, because the parameters are not valid\n");
         rc = ISMRC_NullArgument;
         break;
      }

      pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hOwnerHandle);
      if (pOwnerDescriptor == NULL)
      {
         TRACE(1, "Failed to open a reference context for owner 0x%lx, because the owner handle is not valid\n", hOwnerHandle);
         rc = ISMRC_ArgNotValid;
         ism_common_setErrorData(rc, "%s", "hOwnerHandle");
         break;
      }

      if (!ismSTORE_IS_SPLITITEM(pOwnerDescriptor->DataType))
      {
         TRACE(1, "Failed to open a reference context for owner 0x%lx, because the owner record type 0x%x is not valid\n", hOwnerHandle, pOwnerDescriptor->DataType);
         rc = ISMRC_ArgNotValid;
         ism_common_setErrorData(rc, "%s", "hOwnerHandle");
         break;
      }

      pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);
      if (pSplit->pRefCtxt)
      {
         TRACE(9, "A reference context for owner 0x%lx already exists\n", hOwnerHandle);
      }
      else if ((rc = ism_store_memAllocateRefCtxt(pSplit, hOwnerHandle)) != ISMRC_OK)
      {
         break;
      }

      pRefCtxt = (ismStore_memReferenceContext_t *)pSplit->pRefCtxt;
      *phRefCtxt = (void *)pRefCtxt;

      // Fill in the reference statistics information
      if (pRefStats)
      {
         ism_store_fillReferenceStatistics(pSplit, pRefStats);
      }

   } while (0);

   TRACE(9, "Open a reference context for owner 0x%lx returning %d\n", hOwnerHandle, rc);
   return rc;
}

/*
 * Closes a reference context.
 */
int32_t ism_store_memCloseReferenceContext(void *hRefCtxt)
{
   ismStore_memReferenceContext_t *pRefCtxt = (ismStore_memReferenceContext_t *)hRefCtxt;
   ismStore_memDescriptor_t *pOwnerDescriptor;
   ismStore_memSplitItem_t *pSplit;
   ismStore_Handle_t hOwnerHandle;
   uint32_t ownerVersion;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memValidateRefCtxt(pRefCtxt)) != ISMRC_OK)
   {
      TRACE(1, "Failed to close a reference context, because it is not a valid context\n");
      return rc;
   }

   hOwnerHandle = pRefCtxt->OwnerHandle;
   ownerVersion = pRefCtxt->OwnerVersion;

   if (pRefCtxt->pRefGenHead == NULL)
   {
      pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hOwnerHandle);
      pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);
      ism_store_memFreeRefCtxt(pSplit, ismSTORE_RSRV_GEN_ID);

      TRACE(9, "The reference context for owner 0x%lx has been closed. OwnerVersion %u\n", hOwnerHandle, ownerVersion);
   }
   else
   {
      TRACE(9, "The reference context for owner 0x%lx has not been closed, because it has RefGen items. OwnerVersion %u\n",
            hOwnerHandle, ownerVersion);
   }

   return rc;
}

/*
 * Clears a reference context.
 */
int32_t ism_store_memClearReferenceContext(ismStore_StreamHandle_t hStream, void *hRefCtxt)
{
   return ISMRC_NotImplemented;
}

static int32_t ism_store_memAllocateRefCtxt(ismStore_memSplitItem_t *pSplit, ismStore_Handle_t hOwnerHandle)
{
   ismStore_memReferenceContext_t *pRefCtxt;
   uint32_t idx;

   pRefCtxt = (ismStore_memReferenceContext_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,141),sizeof(ismStore_memReferenceContext_t));
   if (pRefCtxt == NULL)
   {
      TRACE(1, "Failed to allocate a reference context for owner 0x%lx\n", hOwnerHandle);
      return ISMRC_AllocateError;
   }

   memset(pRefCtxt, '\0', sizeof(ismStore_memReferenceContext_t));

   pthread_mutex_lock(&ismStore_memGlobal.CtxtMutex);
   idx = ismStore_memGlobal.RefCtxtIndex;
   pRefCtxt->pMutex = ismStore_memGlobal.pRefCtxtMutex[idx];
   pRefCtxt->pRefGenPool = &ismStore_memGlobal.pRefGenPool[idx];
   if (++ismStore_memGlobal.RefCtxtIndex >= ismStore_memGlobal.RefCtxtLocksCount)
   {
      ismStore_memGlobal.RefCtxtIndex = 0;
   }
   pthread_mutex_unlock(&ismStore_memGlobal.CtxtMutex);

   pRefCtxt->OwnerHandle = hOwnerHandle;
   pRefCtxt->OwnerVersion = pSplit->Version;

   pSplit->pRefCtxt = (uint64_t)pRefCtxt;
   ADR_WRITE_BACK(&pSplit->pRefCtxt, sizeof(pSplit->pRefCtxt));

   TRACE(9, "A reference context (Index %u) for owner 0x%lx has been allocated. OwnerVersion %u\n",
         idx, hOwnerHandle, pRefCtxt->OwnerVersion);

   return ISMRC_OK;
}

static void ism_store_memFreeRefCtxt(ismStore_memSplitItem_t *pSplit, ismStore_GenId_t genId)
{
   ismStore_memReferenceContext_t *pRefCtxt;
   ismStore_memRefGen_t *pRefGen, *pRefGenHead, *pRefGenState;

   if ((pRefCtxt = (ismStore_memReferenceContext_t *)pSplit->pRefCtxt))
   {
      ismSTORE_MUTEX_LOCK(pRefCtxt->pMutex);
      pRefGenHead = pRefCtxt->pRefGenHead;
      pRefGenState = pRefCtxt->pRefGenState;
      pRefCtxt->pRefGenHead = NULL;
      pRefCtxt->pRefGenState = NULL;
      ismSTORE_MUTEX_UNLOCK(pRefCtxt->pMutex);

      while ((pRefGen = pRefGenHead))
      {
         // genId != ismSTORE_RSRV_GEN_ID indicates that the record owner is deleted
         // In such a case, we have to free the references and refStates that are linked to the owner.
         if (genId != ismSTORE_RSRV_GEN_ID)
         {
            if (ismSTORE_EXTRACT_OFFSET(pRefGen->hReferenceHead))
            {
               if (ismSTORE_EXTRACT_GENID(pRefGen->hReferenceHead) == genId)
               {
                  ism_store_memReturnPoolElements(NULL, pRefGen->hReferenceHead, 1);
               }
               else
               {
                  ism_store_memResetGenMap(pRefGen->hReferenceHead);
               }
            }
         }
         pRefGenHead = pRefGenHead->Next;
         ismSTORE_MUTEX_LOCK(pRefCtxt->pMutex);
         ism_store_memFreeRefGen(pRefCtxt, pRefGen);
         ismSTORE_MUTEX_UNLOCK(pRefCtxt->pMutex);
      }

      // See whether there are active RefStates
      if ((pRefGen = pRefGenState))
      {
         if (genId != ismSTORE_RSRV_GEN_ID)
         {
             if (ismSTORE_EXTRACT_OFFSET(pRefGen->hReferenceHead))
             {
                ism_store_memReturnPoolElements(NULL, pRefGen->hReferenceHead, 1);
             }
         }
         ismSTORE_MUTEX_LOCK(pRefCtxt->pMutex);
         ism_store_memFreeRefGen(pRefCtxt, pRefGen);
         ismSTORE_MUTEX_UNLOCK(pRefCtxt->pMutex);
      }

      pRefCtxt->pMutex = NULL;
      ism_common_free(ism_memory_store_misc,pRefCtxt);
      pSplit->pRefCtxt = 0;
      ADR_WRITE_BACK(&pSplit->pRefCtxt, sizeof(pSplit->pRefCtxt));
   }
}

/*
 * Makes a reference context to the handle of the owning record.
 */
ismStore_Handle_t ism_store_memMapReferenceContextToHandle(void *hRefCtxt)
{
   ismStore_memReferenceContext_t *pRefCtxt = (ismStore_memReferenceContext_t *)hRefCtxt;

   if (pRefCtxt == NULL)
   {
      return 0;
   }

   return pRefCtxt->OwnerHandle;
}

/*
 * Opens a state context, used to track states for a specific owner handle.
 */
int32_t ism_store_memOpenStateContext(ismStore_Handle_t hOwnerHandle, void **phStateCtxt)
{
   ismStore_memDescriptor_t *pOwnerDescriptor;
   ismStore_memSplitItem_t *pSplit=NULL;
   int32_t rc = ISMRC_OK;

   TRACE(9, "Open a state context for owner 0x%lx\n", hOwnerHandle);

   do
   {
      pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hOwnerHandle);
      if (pOwnerDescriptor == NULL)
      {
         TRACE(1, "Failed to open a state context for owner 0x%lx, because the owner handle is not valid\n", hOwnerHandle);
         rc = ISMRC_NullArgument;
         break;
      }

      if (pOwnerDescriptor->DataType != ISM_STORE_RECTYPE_CLIENT)
      {
         TRACE(1, "Failed to open a state context for owner 0x%lx, because the owner record type 0x%x is not valid\n", hOwnerHandle, pOwnerDescriptor->DataType);
         rc = ISMRC_ArgNotValid;
         ism_common_setErrorData(rc, "%s", "hOwnerHandle");
         break;
      }

      pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);
      if (pSplit->pStateCtxt)
      {
         TRACE(9, "A state context for owner 0x%lx already exists\n", hOwnerHandle);
      }
      else if ((rc = ism_store_memAllocateStateCtxt(pSplit, hOwnerHandle)) != ISMRC_OK)
      {
         break;
      }

      *phStateCtxt = (void *)pSplit->pStateCtxt;
   } while (0);

   TRACE(9, "Open a state context for owner 0x%lx returning %d\n", hOwnerHandle, rc);

   return rc;
}

static int32_t ism_store_memAllocateStateCtxt(ismStore_memSplitItem_t *pSplit, ismStore_Handle_t hOwnerHandle)
{
   ismStore_memStateContext_t *pStateCtxt;
   uint32_t idx;

   pStateCtxt = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,143),sizeof(ismStore_memStateContext_t));
   if (pStateCtxt == NULL)
   {
      TRACE(1, "Failed to allocate a state context for owner 0x%lx\n", hOwnerHandle);
      return ISMRC_AllocateError;
   }
   memset(pStateCtxt, '\0', sizeof(ismStore_memStateContext_t));

   pthread_mutex_lock(&ismStore_memGlobal.CtxtMutex);
   idx = ismStore_memGlobal.StateCtxtIndex;
   pStateCtxt->pMutex = ismStore_memGlobal.pStateCtxtMutex[idx];
   if (++ismStore_memGlobal.StateCtxtIndex >= ismStore_memGlobal.StateCtxtLocksCount)
   {
      ismStore_memGlobal.StateCtxtIndex = 0;
   }
   pthread_mutex_unlock(&ismStore_memGlobal.CtxtMutex);

   pStateCtxt->OwnerHandle = hOwnerHandle;
   pStateCtxt->OwnerVersion = pSplit->Version;
   pSplit->pStateCtxt = (uint64_t)pStateCtxt;
   ADR_WRITE_BACK(&pSplit->pStateCtxt, sizeof(pSplit->pStateCtxt));
   TRACE(9, "A state context (Index %u) for owner 0x%lx has been allocated. OwnerVersion %u\n",
         idx, hOwnerHandle, pStateCtxt->OwnerVersion);

   return ISMRC_OK;
}

/*
 * Closes a state context.
 */
int32_t ism_store_memCloseStateContext(void *hStateCtxt)
{
   ismStore_memStateContext_t *pStateCtxt = (ismStore_memStateContext_t *)hStateCtxt;
   ismStore_memDescriptor_t *pOwnerDescriptor;
   ismStore_memSplitItem_t *pSplit;
   ismStore_Handle_t hOwnerHandle;
   uint32_t ownerVersion;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memValidateStateCtxt(pStateCtxt)) != ISMRC_OK)
   {
      TRACE(1, "Failed to close the state context, because it is not a valid context\n");
      return rc;
   }

   hOwnerHandle = pStateCtxt->OwnerHandle;
   ownerVersion = pStateCtxt->OwnerVersion;

   if (pStateCtxt->hStateHead == ismSTORE_NULL_HANDLE)
   {
      pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hOwnerHandle);
      pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);
      ism_store_memFreeStateCtxt(pSplit, ismSTORE_RSRV_GEN_ID);
      TRACE(9, "The state context for owner 0x%lx has been closed. OwnerVersion %u\n", hOwnerHandle, ownerVersion);
   }
   else
   {
      TRACE(9, "The state context for owner 0x%lx has not been closed, because it has state objects. OwnerVersion %u, hStateHead 0x%lx\n",
            hOwnerHandle, ownerVersion, pStateCtxt->hStateHead);
   }

   return rc;
}

static void ism_store_memFreeStateCtxt(ismStore_memSplitItem_t *pSplit, ismStore_GenId_t genId)
{
   ismStore_memStateContext_t *pStateCtxt;

   if ((pStateCtxt = (ismStore_memStateContext_t *)pSplit->pStateCtxt))
   {
      if (genId != ismSTORE_RSRV_GEN_ID && ismSTORE_EXTRACT_OFFSET(pStateCtxt->hStateHead))
      {
         ism_store_memReturnPoolElements(NULL, pStateCtxt->hStateHead, 1);
      }
      pStateCtxt->pMutex = NULL;
      ism_common_free(ism_memory_store_misc,(void *)pSplit->pStateCtxt);
      pSplit->pStateCtxt = 0;
      ADR_WRITE_BACK(&pSplit->pStateCtxt, sizeof(pSplit->pStateCtxt));
   }
}

/*
 * Makes a state context to the handle of the owning record.
 */
ismStore_Handle_t ism_store_memMapStateContextToHandle(void *hStateCtxt)
{
   ismStore_memStateContext_t *pStateCtxt = (ismStore_memStateContext_t *)hStateCtxt;

   if (pStateCtxt == NULL)
   {
      return 0;
   }

   return pStateCtxt->OwnerHandle;
}

/*
 * Commit/Rollback a store transaction.
 */
int32_t ism_store_memEndStoreTransaction(ismStore_StreamHandle_t hStream, uint8_t fCommit,
                                         ismStore_CompletionCallback_t pCallback,
                                         void *pContext)
{
   ismStore_memStream_t *pStream;
   ismStore_memDescriptor_t *pDescriptor, *pCurrentDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_memHAAck_t ack;
   ismStore_Handle_t nHandle;
   int ec, fHAWaitAck;
   int32_t rc = ISMRC_OK;

   if (fCommit < 2 && (rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
      TRACE(1, "Failed to end a store transaction, because the stream (hStream %d) is not valid\n", hStream);
      return rc;
   }

   pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];

   if (pStream->hStoreTranHead != ismSTORE_NULL_HANDLE)
   {
      pDescriptor = pStream->pDescrTranHead;
      pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);
      if (pTran->OperationCount == 0)
      {
         TRACE(7,"Fast return from commit/rollback for empty store transaction for stream %u\n",hStream);
         ism_store_memSetStreamActivity(pStream,0);
         return rc ; 
      }
   }

   // To minimize the number of WRITE_BACK operations, we do WRITE_BACK only
   // when the transaction granule becomes full or at the end of the store-transaction.
   if (ismStore_global.CacheFlushMode == ismSTORE_CACHEFLUSH_ADR && pStream->hStoreTranCurrent != ismSTORE_NULL_HANDLE)
   {
      pCurrentDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pStream->hStoreTranCurrent);
      pTran = (ismStore_memStoreTransaction_t *)(pCurrentDescriptor + 1);
      // We have to WRITE_BACK the store operations before the OperationsCount field
      ADR_WRITE_BACK(pTran->Operations, pTran->OperationCount * sizeof(ismStore_memStoreOperation_t));
      ADR_WRITE_BACK(pTran, sizeof(*pTran) - sizeof(ismStore_memStoreOperation_t));
   }

   if (fCommit)
   {
      fHAWaitAck = 0;

      if ( ismStore_memGlobal.fEnablePersist )
      {
         pStream->pPersist->curCB->pCallback = pCallback ; 
         pStream->pPersist->curCB->pContext  = pContext  ; 
         pStream->pPersist->curCB->pStream   = pStream   ; 
         ec = ism_storePersist_writeST(pStream, 0);
         if (ec == StoreRC_SystemError)
         {
            TRACE(1, "Failed to end a store transaction due to ShmPersist error. hStream %d, error code %d\n", hStream, ec);
            rc = ISMRC_StoreHAError;
            return rc;
         }
      }
      else
      if (ismSTORE_HASSTANDBY)   // If a Standby node exists, we need to update the Standby
      {
         memset(&ack, '\0', sizeof(ack));
         ack.AckSqn = pStream->pHAChannel->MsgSqn;
         ec = ism_store_memHASendST(pStream->pHAChannel, pStream->hStoreTranHead);
         if (ec == StoreRC_SystemError)
         {
            TRACE(1, "Failed to end a store transaction due to an HA error. hStream %d, error code %d\n", hStream, ec);
            rc = ISMRC_StoreHAError;
            return rc;
         }

         if (ec == StoreRC_OK)
         {
            fHAWaitAck = 1;
         }
      }
      rc = ism_store_memCommitInternal(pStream, pStream->pDescrTranHead);
      if (fHAWaitAck)
      {
         ism_store_memHAReceiveAck(pStream->pHAChannel, &ack, 0);
      }

      if ( rc == ISMRC_OK )
      {
        if ( ismStore_memGlobal.fEnablePersist )
        {
          if ( pCallback )
          {
            pStream->pPersist->curCB->numCBs++ ; 
            rc = ISMRC_AsyncCompletion;
          }
          else
            rc = ism_storePersist_completeST(pStream);
        }
      }
   }
   else
   {
      rc = ism_store_memRollbackInternal(pStream, pStream->hStoreTranHead);
   }

   if (rc == ISMRC_OK || rc == ISMRC_AsyncCompletion )
   {
      // Release the extra store-transaction chunks (if exists)
      if (pStream->ChunksInUse > pStream->ChunksRsrv)
      {
         pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pStream->hStoreTranRsrv);
         nHandle = pDescriptor->NextHandle;
         pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
         ADR_WRITE_BACK(&pDescriptor->NextHandle, sizeof(pDescriptor->NextHandle));

         ism_store_memReturnPoolElements(NULL, nHandle, 0);
         TRACE(7, "The extra store-transaction chunks of the stream (hStream %u) have been released. ChunksInUse %u, ChunksRsrv %u, Handle 0x%lx\n",
               hStream, pStream->ChunksInUse, pStream->ChunksRsrv, nHandle);

         pStream->ChunksInUse = pStream->ChunksRsrv;
      }

      pStream->hStoreTranCurrent = pStream->hStoreTranHead;
      ism_store_memSetStreamActivity(pStream, 0);  // Set the stream as inactive in the generation
   }

   return rc;
}

/*
 * Assigns store memory for a record from the pool.
 */
int32_t ism_store_memAssignRecordAllocation(ismStore_StreamHandle_t hStream,
                                            const ismStore_Record_t *pRecord,
                                            ismStore_Handle_t *pHandle)
{
   ismStore_memGeneration_t *pGen;
   ismStore_memStream_t *pStream;
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memDescriptor_t *pDescriptor, *pExtDescriptor;
   ismStore_memSplitItem_t *pSplit;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_memStoreOperation_t *pOp;
   ismStore_memGranulePool_t *pPool;
   ismStore_Handle_t handle, ldHandle=ismSTORE_NULL_HANDLE;
   int i;
   uint32_t bytesCount;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
      TRACE(1, "Failed to assign a record allocation, because the stream is not valid\n");
      return rc;
   }

   pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];
   if ((rc = ism_store_memEnsureStoreTransAllocation(pStream, &pDescriptor)) != ISMRC_OK)
   {
      return rc;
   }
   pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);
   pGen = &ismStore_memGlobal.InMemGens[pStream->MyGenIndex];

   switch (pRecord->Type)
   {
      case ISM_STORE_RECTYPE_SERVER:
         rc = ism_store_memGetMgmtPoolElements(NULL,
                                               ismSTORE_MGMT_POOL_INDEX,
                                               pRecord->Type,
                                               pRecord->Attribute,
                                               pRecord->State,
                                               pRecord->DataLength,
                                               &handle,
                                               &pDescriptor);
         if (rc == ISMRC_OK)
         {
            ism_store_memCopyRecordData(pDescriptor, pRecord);
         }
         break;
      case ISM_STORE_RECTYPE_MSG:
      case ISM_STORE_RECTYPE_PROP:
      case ISM_STORE_RECTYPE_CPROP:
      case ISM_STORE_RECTYPE_QPROP:
      case ISM_STORE_RECTYPE_SPROP:
      case ISM_STORE_RECTYPE_TPROP:
      case ISM_STORE_RECTYPE_BXR:
      case ISM_STORE_RECTYPE_RPROP:
         rc = ism_store_memGetPoolElements(pStream,
                                           pGen,
                                           0,
                                           pRecord->Type,
                                           pRecord->Attribute,
                                           pRecord->State,
                                           pRecord->DataLength,
                                           0,
                                           &handle,
                                           &pDescriptor);
         if (rc == ISMRC_OK)
         {
            ism_store_memCopyRecordData(pDescriptor, pRecord);
         }
         break;
      case ISM_STORE_RECTYPE_CLIENT:
      case ISM_STORE_RECTYPE_QUEUE:
      case ISM_STORE_RECTYPE_TOPIC:
      case ISM_STORE_RECTYPE_SUBSC:
      case ISM_STORE_RECTYPE_TRANS:
      case ISM_STORE_RECTYPE_BMGR:
      case ISM_STORE_RECTYPE_REMSRV:
         pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
         pPool = &pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX];
         if (pRecord->DataLength + sizeof(ismStore_memSplitItem_t) > pPool->GranuleDataSizeBytes)
         {
            rc = ism_store_memGetMgmtPoolElements(NULL,
                                                  ismSTORE_MGMT_SMALL_POOL_INDEX,
                                                  pRecord->Type,
                                                  pRecord->Attribute,
                                                  pRecord->State,
                                                  sizeof(ismStore_memSplitItem_t),
                                                  &handle,
                                                  &pDescriptor);
            if (rc == ISMRC_OK)
            {
               rc = ism_store_memGetPoolElements(pStream,
                                                 pGen,
                                                 0,
                                                 ismSTORE_DATATYPE_LDATA_EXT,
                                                 0,
                                                 0,
                                                 pRecord->DataLength,
                                                 0,
                                                 &ldHandle,
                                                 &pExtDescriptor);
               if (rc == ISMRC_OK)
               {
                  pExtDescriptor->DataType = ismSTORE_DATATYPE_LDATA_EXT;
                  ism_store_memCopyRecordData(pExtDescriptor, pRecord);
                  pSplit = (ismStore_memSplitItem_t *)(pDescriptor + 1);
                  pSplit->Version++;
                  pSplit->DataLength = pRecord->DataLength;
                  pSplit->hLargeData = ldHandle;
                  pSplit->MinActiveOrderId = 0;
                  pSplit->pRefCtxt = pSplit->pStateCtxt = 0;
                  ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor) +  sizeof(*pSplit));
               }
               else
               {
                  ism_store_memReturnPoolElements(NULL, handle, 0);
                  ism_store_memDecOwnerCount(pRecord->Type, 1);
               }
            }
         }
         else
         {
            rc = ism_store_memGetMgmtPoolElements(NULL,
                                                  ismSTORE_MGMT_SMALL_POOL_INDEX,
                                                  pRecord->Type,
                                                  pRecord->Attribute,
                                                  pRecord->State,
                                                  sizeof(ismStore_memSplitItem_t) + pRecord->DataLength,
                                                  &handle,
                                                  &pDescriptor);
            if (rc == ISMRC_OK)
            {
               pSplit = (ismStore_memSplitItem_t *)(pDescriptor + 1);
               pSplit->Version++;
               pSplit->DataLength = pRecord->DataLength;
               pSplit->hLargeData = ismSTORE_BUILD_HANDLE(pStream->MyGenId, 0);
               pSplit->MinActiveOrderId = 0;
               pSplit->pRefCtxt = pSplit->pStateCtxt = 0;
               for (i=0, bytesCount=0; i < pRecord->FragsCount && bytesCount < pRecord->DataLength; i++)
               {
                  // Make sure that we don't get into a buffer overflow situation
                  if (bytesCount + pRecord->pFragsLengths[i] > pRecord->DataLength)
                  {
                     memcpy((char *)(pSplit + 1) + bytesCount, pRecord->pFrags[i], pRecord->DataLength - bytesCount);
                  }
                  else
                  {
                     memcpy((char *)(pSplit + 1) + bytesCount, pRecord->pFrags[i], pRecord->pFragsLengths[i]);
                  }
                  bytesCount += pRecord->pFragsLengths[i];
               }
               ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor) + sizeof(*pSplit) + pRecord->DataLength);
            }
         }
         break;
      default:
         TRACE(1, "Failed to add a record into the store, because the record type 0x%x is not valid\n", pRecord->Type);
         rc = ISMRC_Error;
   }

   if (rc == ISMRC_OK)
   {
      // Add this operation to the store-transaction
      pOp = &pTran->Operations[pTran->OperationCount];
      pOp->OperationType = Operation_CreateRecord;
      pOp->CreateRecord.Handle = handle;
      pOp->CreateRecord.DataType = pRecord->Type;
      pOp->CreateRecord.LDHandle = ldHandle;
      pTran->OperationCount++;

      // To minimize the number of WRITE_BACK operations, we do WRITE_BACK only
      // when the transaction granule becomes full or at the end of the store-transaction.
      //ADR_WRITE_BACK(pOp, sizeof(*pOp));
      //ADR_WRITE_BACK(&pTran->OperationCount, sizeof(pTran->OperationCount));

      *pHandle = handle;
   }

   return rc;
}

/*
 * Commits allocation of space for a record from the pool.
 */
static inline int32_t ism_store_memAssignRecordAllocation_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp)
{
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memSplitItem_t *pSplit;
   int32_t rc = ISMRC_OK;

   pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pOp->CreateRecord.Handle);
   pDescriptor->DataType = pOp->CreateRecord.DataType;
   ADR_WRITE_BACK(&pDescriptor->DataType, sizeof(pDescriptor->DataType));

   if (ismSTORE_IS_SPLITITEM(pDescriptor->DataType))
   {
      ismStore_memMgmtHeader_t *pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;

      // Set HaveData only after the first owner is created in the Store (instead of in recoveryCompleted). BMGR is created by MQC so need to ignore
      if (pMgmtHeader->HaveData == 0 && pDescriptor->DataType != ISM_STORE_RECTYPE_BMGR )
      {
        pMgmtHeader->HaveData = 1;
        ADR_WRITE_BACK(&pMgmtHeader->HaveData, 1); 
      }

      // This code is required to make sure that the RefCtxt and StateCtxt exist
      // during the recovery procedure (in case that the commit is called during the InitMgmt).
      pSplit = (ismStore_memSplitItem_t *)(pDescriptor + 1);
      if (!pSplit->pRefCtxt && (rc = ism_store_memAllocateRefCtxt(pSplit, pOp->CreateRecord.Handle)) != ISMRC_OK)
      {
         return rc;
      }
      if (pDescriptor->DataType == ISM_STORE_RECTYPE_CLIENT && !pSplit->pStateCtxt)
      {
         if ((rc = ism_store_memAllocateStateCtxt(pSplit, pOp->CreateRecord.Handle)) != ISMRC_OK)
         {
            return rc;
         }
      }
      // update record count when called from ism_store_memInitMgmt committing partial ST
      if (ismStore_memGlobal.State < ismSTORE_STATE_ACTIVE)
      {
        ismStore_memGlobal.OwnerCount[ismStore_T2T[pDescriptor->DataType]]++;
        ismStore_memGlobal.OwnerCount[0                                  ]++;
      }
   }

   return rc;
}

/*
 * Rolls back allocation of space for a record from the pool.
 */
static inline int32_t ism_store_memAssignRecordAllocation_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp)
{
   int32_t rc;

   if (pOp->OperationType == Operation_CreateRecord)
   {
      if (pOp->CreateRecord.LDHandle != ismSTORE_NULL_HANDLE)
      {
         rc = ism_store_memReturnPoolElements(pStream, pOp->CreateRecord.LDHandle, 1);
      }
      
      if (ismSTORE_IS_SPLITITEM(pOp->CreateRecord.DataType) && ismStore_memGlobal.State == ismSTORE_STATE_ACTIVE)
      {
         ism_store_memDecOwnerCount(pOp->CreateRecord.DataType, 1);         
      }
      rc = ism_store_memReturnPoolElements(pStream, pOp->CreateRecord.Handle, 1);
   }
   else
   {
      TRACE(1, "Failed to rollback a store-transaction, because the operation type (%d) is not valid. Valid value %d\n", pOp->OperationType, Operation_CreateRecord);
      rc = ISMRC_Error;
   }

   return rc;
}

/*
 * Frees store memory for a record and return it to the pool.
 */
int32_t ism_store_memFreeRecordAllocation(ismStore_StreamHandle_t hStream,
                                          ismStore_Handle_t handle)
{
   ismStore_memStream_t *pStream;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_memStoreOperation_t *pOp;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
      TRACE(1, "Failed to free a record allocation, because the stream is not valid\n");
      return rc;
   }

   if ((rc = ism_store_memValidateHandle(handle)) != ISMRC_OK)
   {
      TRACE(1, "Failed to free a record, because the handle is not valid. Handle 0x%lx\n", handle);
      return rc;
   }

   pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];
   rc = ism_store_memEnsureStoreTransAllocation(pStream, &pDescriptor);
   if (rc == ISMRC_OK)
   {
      pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);

      // Add this operation to the store-transaction
      pOp = &pTran->Operations[pTran->OperationCount];
      pOp->OperationType = Operation_DeleteRecord;
      pOp->DeleteRecord.Handle = handle;
      pTran->OperationCount++;

      // To minimize the number of WRITE_BACK operations, we do WRITE_BACK only
      // when the transaction granule becomes full or at the end of the store-transaction.
      //ADR_WRITE_BACK(pOp, sizeof(*pOp));
      //ADR_WRITE_BACK(pTran, sizeof(*pTran));
   }

   return rc;
}

/*
 * Commits freeing space for a record.
 */
static inline int32_t ism_store_memFreeRecordAllocation_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp)
{
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memSplitItem_t *pSplit;
   ismStore_GenId_t genId;
   int32_t rc = ISMRC_OK;

   genId = ismSTORE_EXTRACT_GENID(pOp->DeleteRecord.Handle);
   if (genId == pTran->GenId || genId == ismSTORE_MGMT_GEN_ID)
   {
      // See whether there are active references and/or states
      pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pOp->DeleteRecord.Handle);
      if (ismSTORE_IS_SPLITITEM(pDescriptor->DataType))
      {
         pSplit = (ismStore_memSplitItem_t *)(pDescriptor + 1);
         if (pSplit->pRefCtxt)
         {
            ism_store_memFreeRefCtxt(pSplit, pTran->GenId);
         }

         if (pSplit->pStateCtxt)
         {
            ism_store_memFreeStateCtxt(pSplit, pTran->GenId);
         }

         if (ismSTORE_EXTRACT_OFFSET(pSplit->hLargeData))
         {
            if (ismSTORE_EXTRACT_GENID(pSplit->hLargeData) == pTran->GenId)
            {
               rc = ism_store_memReturnPoolElements(pStream, pSplit->hLargeData, 1);
            }
            else
            {
               ism_store_memResetGenMap(pSplit->hLargeData);
            }
         }
      }
      rc = ism_store_memReturnPoolElements(pStream, pOp->DeleteRecord.Handle, 1);
   }
   else
   {
      ism_store_memResetGenMap(pOp->DeleteRecord.Handle);
   }

   return rc;
}

/*
 * Rolls back freeing space for a record.
 */
static inline int32_t ism_store_memFreeRecordAllocation_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp)
{
   int32_t rc = ISMRC_OK;

   if (pOp->OperationType != Operation_DeleteRecord)
   {
      TRACE(1, "Failed to rollback a store-transaction, because the operation type (%d) is not valid. Valid value %d\n", pOp->OperationType, Operation_DeleteRecord);
      rc = ISMRC_Error;
   }

   return rc;
}

/*
 * Updates a record in the Store.
 */
int32_t ism_store_memUpdateRecord(ismStore_StreamHandle_t hStream,
                                  ismStore_Handle_t handle,
                                  uint64_t attribute,
                                  uint64_t state,
                                  uint8_t flags)
{
   ismStore_memStream_t *pStream;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_memStoreOperation_t *pOp;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
      TRACE(1, "Failed to update a record, because the stream is not valid\n");
      return rc;
   }

   if (ismSTORE_EXTRACT_GENID(handle) != ismSTORE_MGMT_GEN_ID)
   {
      rc = ISMRC_ArgNotValid;
      ism_common_setErrorData(rc, "%s", "handle");
      return rc;
   }

   if (!(flags & (ismSTORE_SET_ATTRIBUTE|ismSTORE_SET_STATE)))
   {
      rc = ISMRC_ArgNotValid;
      ism_common_setErrorData(rc, "%s", "flags");
      return rc;
   } 

   if ((rc = ism_store_memValidateHandle(handle)) != ISMRC_OK)
   {
      TRACE(1, "Failed to update a record, because the handle is not valid. Handle 0x%lx\n", handle);
      return rc;
   }

   pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];
   rc = ism_store_memEnsureStoreTransAllocation(pStream, &pDescriptor);
   if (rc == ISMRC_OK)
   {
      pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);

      // Add this operation to the store-transaction
      pOp = &pTran->Operations[pTran->OperationCount];
      if ((flags & ismSTORE_SET_ATTRIBUTE) && (flags & ismSTORE_SET_STATE))
      {
         pOp->OperationType = Operation_UpdateRecord;
      }
      else if (flags & ismSTORE_SET_ATTRIBUTE)
      {
         pOp->OperationType = Operation_UpdateRecordAttr;
      }
      else
      {
         pOp->OperationType = Operation_UpdateRecordState;
      }      
      pOp->UpdateRecord.Handle = handle;
      pOp->UpdateRecord.Attribute = attribute;
      pOp->UpdateRecord.State = state;
      pTran->OperationCount++;

      // To minimize the number of WRITE_BACK operations, we do WRITE_BACK only
      // when the transaction granule becomes full or at the end of the store-transaction.
      //ADR_WRITE_BACK(pOp, sizeof(*pOp));
      //ADR_WRITE_BACK(pTran, sizeof(*pTran));
   }

   return rc;
}

static inline int32_t ism_store_memUpdate_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp)
{
   ismStore_memDescriptor_t *pDescriptor;

   pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pOp->UpdateRecord.Handle);
   pDescriptor->Attribute = pOp->UpdateRecord.Attribute;
   pDescriptor->State = pOp->UpdateRecord.State;

   ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));

   return ISMRC_OK;
}

static inline int32_t ism_store_memUpdate_Rollback(
        ismStore_memStream_t *pStream,
        ismStore_memStoreTransaction_t *pTran,
        ismStore_memStoreOperation_t *pOp)
{
   if (pOp->OperationType != Operation_UpdateRecord)
   {
      TRACE(1, "Failed to rollback a store-transaction, because the operation type (%d) is not valid. Valid value %d\n", pOp->OperationType, Operation_UpdateRecord);
      return ISMRC_Error;
   }

   return ISMRC_OK;
}

static inline int32_t ism_store_memUpdateAttribute_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp)
{
   ismStore_memDescriptor_t *pDescriptor;

   pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pOp->UpdateRecord.Handle);
   pDescriptor->Attribute = pOp->UpdateRecord.Attribute;

   ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));

   return ISMRC_OK;
}

static inline int32_t ism_store_memUpdateAttribute_Rollback(
        ismStore_memStream_t *pStream,
        ismStore_memStoreTransaction_t *pTran,
        ismStore_memStoreOperation_t *pOp)
{
   if (pOp->OperationType != Operation_UpdateRecordAttr)
   {
      TRACE(1, "Failed to rollback a store-transaction, because the operation type (%d) is not valid. Valid value %d\n", pOp->OperationType, Operation_UpdateRecordAttr);
      return ISMRC_Error;
   }

   return ISMRC_OK;
}

static inline int32_t ism_store_memUpdateState_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp)
{
   ismStore_memDescriptor_t *pDescriptor;

   pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pOp->UpdateRecord.Handle);
   pDescriptor->State = pOp->UpdateRecord.State;

   ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));

   return ISMRC_OK;
}

static inline int32_t ism_store_memUpdateState_Rollback(
        ismStore_memStream_t *pStream,
        ismStore_memStoreTransaction_t *pTran,
        ismStore_memStoreOperation_t *pOp)
{
   int32_t rc = ISMRC_OK;

   if (pOp->OperationType != Operation_UpdateRecordState)
   {
      TRACE(1, "Failed to rollback a store-transaction, because the operation type (%d) is not valid. Valid value %d\n", pOp->OperationType, Operation_UpdateRecordState);
      rc = ISMRC_Error;
   }

   return rc;
}

static void ism_store_memIncreaseRefGenPool(ismStore_memRefGenPool_t *pRefGenPool)
{
   ismStore_memRefGen_t *pRefGen, *pHead = NULL, *pTail = NULL;
   int nElements;

   for (nElements=0; nElements < ismStore_memGlobal.RefGenPoolExtElements; nElements++)
   {
      if ((pRefGen = (ismStore_memRefGen_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,145),sizeof(ismStore_memRefGen_t))) == NULL)
      {
         if (nElements == 0)
         {
            TRACE(1, "Failed to increase the RefGenPool (Index %u), due to a memory allocation error. Size %u\n", pRefGenPool->Index, pRefGenPool->Size);
            return;
         }
         TRACE(7, "Failed to increase the RefGenPool (Index %u), due to a memory allocation error. Size %u, nElements %u\n", pRefGenPool->Index, pRefGenPool->Size, nElements);
         break;
      }
      memset(pRefGen, '\0', sizeof(ismStore_memRefGen_t));
      if (pTail) {
         pTail->Next = pRefGen;
      } else {
         pHead = pRefGen;
      }
      pTail = pRefGen;
   }

   ismSTORE_MUTEX_LOCK(ismStore_memGlobal.pRefCtxtMutex[pRefGenPool->Index]);
   pTail->Next = pRefGenPool->pHead;
   pRefGenPool->pHead = pHead;
   pRefGenPool->Count += nElements;
   pRefGenPool->Size += nElements;
   pRefGenPool->fPendingTask = 0;
   ismSTORE_MUTEX_UNLOCK(ismStore_memGlobal.pRefCtxtMutex[pRefGenPool->Index]);

   if (pRefGenPool->Size > ismStore_memGlobal.RefGenPoolExtElements)
   {
      TRACE(5, "The RefGenPool (Index %u) has been increased. Count %u, Size %u, nElements %u\n",
            pRefGenPool->Index, pRefGenPool->Count, pRefGenPool->Size, nElements);
   }
}

static void ism_store_memDecreaseRefGenPool(ismStore_memRefGenPool_t *pRefGenPool)
{
   ismStore_memRefGen_t *pRefGen, *pRefGenHead;
   int i, nElements = 0;

   ismSTORE_MUTEX_LOCK(ismStore_memGlobal.pRefCtxtMutex[pRefGenPool->Index]);
   if (pRefGenPool->Count > ismStore_memGlobal.RefGenPoolExtElements)
   {
      nElements = pRefGenPool->Count - ismStore_memGlobal.RefGenPoolExtElements;
      for (i=0, pRefGen=pRefGenHead=pRefGenPool->pHead; i < nElements; pRefGen = pRefGen->Next, i++);
      pRefGenPool->pHead = pRefGen;
      pRefGenPool->Count -= nElements;
      pRefGenPool->Size -= nElements;
   }
   pRefGenPool->fPendingTask = 0;
   ismSTORE_MUTEX_UNLOCK(ismStore_memGlobal.pRefCtxtMutex[pRefGenPool->Index]);

   TRACE(5, "The RefGenPool (Index %u) has been decreased. Count %u, Size %u, nElements %u\n",
         pRefGenPool->Index, pRefGenPool->Count, pRefGenPool->Size, nElements);

   for (i=0; i < nElements; i++)
   {
      pRefGen = pRefGenHead;
      pRefGenHead = pRefGenHead->Next;
      ism_common_free(ism_memory_store_misc,pRefGen);
   }
}

ismStore_memRefGen_t *ism_store_memAllocateRefGen(ismStore_memReferenceContext_t *pRefCtxt)
{
   ismStore_memRefGenPool_t *pRefGenPool = pRefCtxt->pRefGenPool;
   ismStore_memRefGen_t *pRefGen;
   ismStore_memJob_t job;

   // See whether the pool size is less than the low water mark - if yes, we have to increase it
   if (pRefGenPool->Count < ismStore_memGlobal.RefGenPoolLWM)
   {
      if (!pRefGenPool->fPendingTask)
      {
         memset(&job, '\0', sizeof(job));
         job.JobType = StoreJob_IncRefGenPool;
         job.RefGenPool.pRefGenPool = pRefGenPool;
         ism_store_memAddJob(&job);
         pRefGenPool->fPendingTask = 1;
      }

      // See whether the pool is empty - if yes, we have to allocate a new element
      if (pRefGenPool->Count == 0)
      {
         if (pRefGenPool->pHead)
         {
            TRACE(1, "The RefGenPool (Index %d) is not balanced. Count %d, Size %u, pHead %p\n", pRefGenPool->Index, pRefGenPool->Count, pRefGenPool->Size, pRefGenPool->pHead);
            pRefGenPool->pHead = NULL;
         }

         if ((pRefGen = (ismStore_memRefGen_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,147),sizeof(ismStore_memRefGen_t))) == NULL)
         {
            TRACE(1, "Failed to allocate memory for a new RefGen entry for owner 0x%lx. Size %u, Count %d\n", pRefCtxt->OwnerHandle, pRefGenPool->Size, pRefGenPool->Count);
            return NULL;
         }
         memset(pRefGen, '\0', sizeof(ismStore_memRefGen_t));
         pRefGenPool->Size++;
         return (pRefGen);
      }
   }

   pRefGen = pRefGenPool->pHead;
   pRefGenPool->pHead = pRefGen->Next;
   pRefGenPool->Count--;
   pRefGen->Next = NULL;
   return (pRefGen);
}

void ism_store_memFreeRefGen(ismStore_memReferenceContext_t *pRefCtxt, ismStore_memRefGen_t *pRefGen)
{
   ismStore_memRefGenPool_t *pRefGenPool = pRefCtxt->pRefGenPool;
   ismStore_memJob_t job;

   ismSTORE_FREE(pRefGen->pRefCache); // it is assumed that the macro check for NULL
   memset(pRefGen, '\0', sizeof(ismStore_memRefGen_t));
   pRefGen->Next = pRefGenPool->pHead;
   pRefGenPool->pHead = pRefGen;
   pRefGenPool->Count++;

   // See whether the pool size exceeds the high water mark - if yes, we have to free spare elements
   if (!pRefGenPool->fPendingTask && pRefGenPool->Count > ismStore_memGlobal.RefGenPoolHWM)
   {
      memset(&job, '\0', sizeof(job));
      job.JobType = StoreJob_DecRefGenPool;
      job.RefGenPool.pRefGenPool = pRefGenPool;
      ism_store_memAddJob(&job);
      pRefGenPool->fPendingTask = 1;
   }
}

static ismStore_memRefGen_t *ism_store_memGetRefGen(ismStore_memStream_t *pStream,
                                                    ismStore_memReferenceContext_t *pRefCtxt)
{
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memSplitItem_t *pSplit;
   ismStore_memRefGen_t *pRefGen;

   pRefGen = pRefCtxt->pInMemRefGen[pStream->MyGenIndex];
   // See whether there is no RefGen for the current stream generation
   if (pRefGen == NULL || ismSTORE_EXTRACT_GENID(pRefGen->hReferenceHead) != pStream->MyGenId)
   {
      pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pRefCtxt->OwnerHandle);
      pSplit = (ismStore_memSplitItem_t *)(pDescriptor + 1);
      // See whether a RefGen for the stream generation already exists
      for (pRefGen = (pRefCtxt->pRefGenLast ? pRefCtxt->pRefGenLast : pRefCtxt->pRefGenHead); pRefGen; pRefGen = pRefGen->Next)
      {
         if (ismSTORE_EXTRACT_GENID(pRefGen->hReferenceHead) == pStream->MyGenId)
         {
            if (pRefGen->Next)
            {
               TRACE(3, "The RefGen of owner 0x%lx (OwnerVersion %u, DataType 0x%x) in generation %u is not the last one. hStream %u, hReferenceHead 0x%lx, hReferenceTail 0x%lx, LowestOrderId %lu, HighestOrderId %lu\n",
                     pRefCtxt->OwnerHandle, pSplit->Version, pDescriptor->DataType, pStream->MyGenId,
                     pStream->hStream, pRefGen->hReferenceHead, pRefGen->hReferenceTail,
                     pRefGen->LowestOrderId, pRefGen->HighestOrderId);
            }
            break;
         }
         pRefCtxt->pRefGenLast = pRefGen;
      }

      if (pRefGen == NULL)
      {
         // Allocate a RefGen for the current stream generation
         if ((pRefGen = ism_store_memAllocateRefGen(pRefCtxt)))
         {
            pRefGen->hReferenceHead = ismSTORE_BUILD_HANDLE(pStream->MyGenId, 0);
            if (pRefCtxt->pRefGenLast)
            {
              pRefCtxt->pRefGenLast->Next = pRefGen;
            }
            else
            {
               pRefCtxt->pRefGenHead = pRefGen;
            }
            pRefCtxt->pRefGenLast = pRefGen;

            TRACE(9, "The RefGen Id %u for owner 0x%lx has been allocated\n", pStream->MyGenId, pRefCtxt->OwnerHandle);
         }
         else
         {
            TRACE(1, "Failed to allocate memory for a RefGen item %u for owner 0x%lx\n", pStream->MyGenId, pRefCtxt->OwnerHandle);
         }
      }
      pRefCtxt->pInMemRefGen[pStream->MyGenIndex] = pRefGen;
   }

   return pRefGen;
}



//  The caller MUST hold the pRefCtxt mutex
static int32_t ism_store_memBuildRFFingers(ismStore_memReferenceContext_t *pRefCtxt)
{
  ismStore_memRefStateFingers_t *pF = pRefCtxt->pRFFingers ;
  ismStore_Handle_t h, nh;
  ismStore_memDescriptor_t *pDesc ; 
  ismStore_memRefStateChunk_t *chunk ; 
  ismStore_memRefGen_t *pRefGen = pRefCtxt->pRefGenState ; 
  char *pBase = ismStore_memGlobal.pStoreBaseAddress ; 
  uint32_t n, m;
  size_t s ; 

  if ( pRefCtxt->RFChunksInUse < 2 * ismSTORE_CHUNK_GAP_LWM || !pRefGen ) // code_rev add traces
  {
    ismSTORE_FREE(pRefCtxt->pRFFingers); // it is assumed that the macro check for NULL
    return ISMRC_OK ; 
  }
  if ( pF )
  {
    if ( pRefCtxt->RFChunksInUse >= pF->NumInUse * pF->ChunkGap / 2 &&
         pRefCtxt->RFChunksInUse <= pF->NumInUse * pF->ChunkGap * 2 )
      return ISMRC_OK ; 

    if ( pF->NumInUse && pRefCtxt->RFChunksInUse < 2 * pF->NumInUse * ismSTORE_CHUNK_GAP_LWM ) // cod_rev might be redundant
      return ISMRC_OK ; 

    if ( pF->NumInUse == pF->NumInArray && pF->ChunkGap * 2 < ismSTORE_CHUNK_GAP_HWM )
    {
      n = pF->NumInUse & 1 ; 
      pF->NumInUse /= 2 ; 
      for ( m=1 ; m<pF->NumInUse ; m++ )
      { 
        pF->BaseOid[m] = pF->BaseOid[m+m] ; 
        pF->Handles[m] = pF->Handles[m+m] ; 
      }
      if (!n )
        pF->NumAtEnd += pF->ChunkGap ; 
      pF->ChunkGap *= 2 ; 
      TRACE(5,"RefStateFingers have been compacted: owner %p, RFChunksInUse= %u, NumInUse=%u, ChunkGap=%u\n",
              (void *)pRefCtxt->OwnerHandle,pRefCtxt->RFChunksInUse,pF->NumInUse,pF->ChunkGap);
      return ISMRC_OK ; 
    }
  }

  n = 2 * pRefCtxt->RFChunksInUse / ismSTORE_CHUNK_GAP_LWM ; 
  if ( !pF || n / 2 > pF->NumInArray || n * 2 < pF->NumInArray )
  {
    void *ptr ; 
    if ( n < 64 ) n = 64 ; 
    s = sizeof(*pF) + n * (sizeof(uint64_t) + sizeof(ismStore_Handle_t)) ; 
    if (!(ptr = ism_common_realloc(ISM_MEM_PROBE(ism_memory_store_misc,148),pF,s)) )
    {
      TRACE(1, "Failed to allocate memory for a RefStateFingers item of size %lu for owner 0x%lx\n", s, pRefCtxt->OwnerHandle);
      return ISMRC_AllocateError;
    }
    TRACE(5,"RefStateFingers have been %s : owner %p, RFChunksInUse= %u, NumInUse=%u, ChunkGap=%u\n",pF?"reallocated":"allocated",
            (void *)pRefCtxt->OwnerHandle,pRefCtxt->RFChunksInUse,n/2,pRefCtxt->RFChunksInUse*2/n);
    pF = pRefCtxt->pRFFingers = ptr ; 
    memset(pF, 0, s) ; 
    pF->BaseOid = (uint64_t *)(pF+1) ; 
    pF->Handles = (ismStore_Handle_t *)(pF->BaseOid + n) ; 
    pF->NumInArray = n ; 
  }
  pF->NumInUse   = n / 2 ; 
  for ( pF->ChunkGap = pRefCtxt->RFChunksInUse / pF->NumInUse ; 
        pF->NumInUse < pRefCtxt->RFChunksInUse / pF->ChunkGap + 1 ; 
        pF->ChunkGap++ );

  h=pRefGen->hReferenceHead ; 
  for ( m=0,n=0 ; ismSTORE_EXTRACT_OFFSET(h) ; h=nh )
  {
    if ( ismSTORE_EXTRACT_GENID(h) != ismSTORE_MGMT_GEN_ID )
    {
      TRACE(1,"!!! Internal Error !!! , handle (=%p) is invalid\n",(void *)h);
      ism_common_sleep(1000);  // 1 milli
    //return ISMRC_Error;
    }
    pDesc = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(h);
    if ( (char *)pDesc != (pBase + ismSTORE_EXTRACT_OFFSET(h)) )
    {
      TRACE(1,"!!! Internal Error !!! , pDesc (%p != %p) is invalid\n",pDesc,(pBase + ismSTORE_EXTRACT_OFFSET(h)));
      ism_common_sleep(1000);  // 1 milli
    //return ISMRC_Error;
    }
    nh=pDesc->NextHandle;
    if ( !n )
    {
      chunk = (ismStore_memRefStateChunk_t *)(pDesc + 1);
      pF->BaseOid[m] = chunk->BaseOrderId ; 
      pF->Handles[m] = h ;  
      m++ ; 
    }
    n = (n+1) % pF->ChunkGap ; 
  }
  pF->NumInUse = m ; 
  pF->NumAtEnd = n ; 

  TRACE(5,"RefStateFingers have been rebuilt : owner %p, RFChunksInUse= %u, NumInUse=%u, NumInArray=%u, ChunkGap=%u, NumAtEnd=%u\n",
          (void *)pRefCtxt->OwnerHandle,pRefCtxt->RFChunksInUse,pF->NumInUse,pF->NumInArray,pF->ChunkGap,pF->NumAtEnd);

  return ISMRC_OK ; 
}

static void ism_store_memSetRefGenStates(ismStore_memReferenceContext_t *pRefCtxt, ismStore_memDescriptor_t *pOwnerDescriptor, ismStore_memRefStateChunk_t *pRefStateChunk)
{
   int i;
   ismStore_memRefGen_t *pRefGen = pRefCtxt->pRefGenState;

   // Find the highest OrderId of the previous owner
   for (i = pRefStateChunk->RefStateCount - 1; i >= 0; i--)
   {
      if (pRefStateChunk->RefStates[i] != ismSTORE_REFSTATE_NOT_VALID)
      {
         pRefGen->HighestOrderId = pRefStateChunk->BaseOrderId + i;
         break;
      }
   }

   if (pRefCtxt->NextPruneOrderId > pRefGen->HighestOrderId + 1)
   {
      pRefCtxt->NextPruneOrderId = pRefGen->HighestOrderId + 1;
   }
   TRACE(7, "The owner (Handle 0x%lx, DataType 0x%x, Version %u) has the following RefStates " \
         "(LowestOrderId %lu, HighestOrderId %lu, NextPruneOrderId %lu, ChunksInUse %u)\n",
         pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, pRefGen->LowestOrderId,
         pRefGen->HighestOrderId, pRefCtxt->NextPruneOrderId, pRefCtxt->RFChunksInUse);

   // See whether the owner has a lot of RefState chunks
   if (pRefCtxt->RFChunksInUse > pRefCtxt->RFChunksInUseHWM)
   {
      pRefCtxt->RFChunksInUseLWM = pRefCtxt->RFChunksInUse / 2;
      pRefCtxt->RFChunksInUseHWM = ismSTORE_ROUNDUP(pRefCtxt->RFChunksInUse + 1, ismSTORE_REFSTATES_CHUNKS);
      TRACE(5, "The owner (Handle 0x%lx, DataType 0x%x, Version %u) has a lot of RefState chunks "\
            "(LowestOrderId %lu, HighestOrderId %lu, NextPruneOrderId %lu, ChunksInUse %u, ChunksInUseLWM %u, ChunksInUseHWM %u)\n",
            pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion,
            pRefGen->LowestOrderId, pRefGen->HighestOrderId, pRefCtxt->NextPruneOrderId,
            pRefCtxt->RFChunksInUse, pRefCtxt->RFChunksInUseLWM, pRefCtxt->RFChunksInUseHWM);
   }
   ism_store_memBuildRFFingers(pRefCtxt) ; 
}

int32_t ism_store_memAddReference(ismStore_StreamHandle_t hStream,
                                  void *hRefCtxt,
                                  const ismStore_Reference_t *pReference,
                                  uint64_t minimumActiveOrderId,
                                  uint8_t fAutoCommit,
                                  ismStore_Handle_t *pHandle)
{
   ismStore_memStream_t *pStream;
   ismStore_memReferenceContext_t *pRefCtxt = (ismStore_memReferenceContext_t *)hRefCtxt;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_memStoreOperation_t *pOp;
   ismStore_Handle_t handle=0;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
      TRACE(1, "Failed to add a reference, because the stream is not valid\n");
      return rc;
   }

   if ((rc = ism_store_memValidateRefCtxt(pRefCtxt)) != ISMRC_OK)
   {
      TRACE(1, "Failed to add a reference, because the reference context is not valid\n");
      return rc;
   }

   pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];

   if ((rc = ism_store_memEnsureStoreTransAllocation(pStream, &pDescriptor)) != ISMRC_OK)
   {
      return rc;
   }

   // First ensure that we have a record of references for this owner and this
   // order ID
   rc = ism_store_memEnsureReferenceAllocation(pStream,
                                               pRefCtxt,
                                               pReference->OrderId,
                                               &handle);

   // Add this operation to the store-transaction
   if (rc == ISMRC_OK)
   {
      pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);
      pOp = &pTran->Operations[pTran->OperationCount];
      pOp->OperationType = Operation_CreateReference;
      pOp->CreateReference.Handle = handle;
      pOp->CreateReference.RefHandle = pReference->hRefHandle;
      pOp->CreateReference.Value = pReference->Value;
      pOp->CreateReference.State = pReference->State;
      pTran->OperationCount++;

      // To minimize the number of WRITE_BACK operations, we do WRITE_BACK only
      // when the transaction granule becomes full or at the end of the store-transaction.
      //ADR_WRITE_BACK(pOp, sizeof(*pOp));
      //ADR_WRITE_BACK(pTran, sizeof(*pTran));

      if (minimumActiveOrderId >= pRefCtxt->NextPruneOrderId)
      {
         ism_store_memPruneReferenceAllocation(pStream,
                                               pRefCtxt,
                                               minimumActiveOrderId);
      }
      *pHandle = handle;

      if (fAutoCommit)
      {
         rc = ism_store_memEndStoreTransaction(hStream, 2, NULL, NULL);
      }
   }

   return rc;
}

static void ism_store_addChunk2Cache(ismStore_memRefGen_t *pRefGen, ismStore_Handle_t hChunk)
{
  ismStore_refGenCache_t *pC;
  if (!pRefGen->pRefCache && ismStore_memGlobal.RefSearchCacheSize && pRefGen->numChunks > 6400 )
  {
    if ( (pRefGen->pRefCache = ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,149),sizeof(ismStore_refGenCache_t) + ismStore_memGlobal.RefSearchCacheSize * sizeof(ismStore_Handle_t))) )
      memset(pRefGen->pRefCache, 0, sizeof(ismStore_refGenCache_t) + ismStore_memGlobal.RefSearchCacheSize * sizeof(ismStore_Handle_t));
  }
  if ( (pC = pRefGen->pRefCache) )
  {
    if ( pC->cacheSize < ismStore_memGlobal.RefSearchCacheSize )
      pC->cacheSize++ ; 
    else
      memmove(pC->hRefCache, pC->hRefCache+1 , (pC->cacheSize-1)*sizeof(ismStore_Handle_t));
    pC->hRefCache[pC->cacheSize-1] = hChunk ; 
  }
}

static void ism_store_trimRefCunks(ismStore_memStream_t *pStream, ismStore_memReferenceContext_t *pRefCtxt, ismStore_memRefGen_t *pRefGen, ismStore_memGeneration_t *pGen)
{
  ismStore_refGenCache_t *pC;
  if ( pRefGen->numChunks < ismStore_memGlobal.RefChunkHWM || pRefGen->numRefs > pRefGen->numChunks / 100 * ismStore_memGlobal.RefChunkLWM )
    return ; 
  if ( (pC = pRefGen->pRefCache) && (uint32_t)(ism_common_monotonicTimeNanos()/1000000000) < pC->nextTrimSecs )
    return ; 

  {
    int nDeleted=0;
    ismStore_memDescriptor_t *pDescriptor, *pDescPrev, *delTail=NULL;
    ismStore_Handle_t hChunk, delHead=0, nextChunk, lastChunk=0 ; 
    ismStore_memReferenceChunk_t *pRefChunk ;
    double dt = ism_common_readTSC();
    size_t sz = pGen->MaxRefsPerGranule * sizeof(ismStore_memReference_t);
    for ( pDescPrev=NULL, hChunk=pRefGen->hReferenceHead ; ismSTORE_EXTRACT_OFFSET(hChunk) ; hChunk = nextChunk )
    {
      pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(hChunk)) ; 
      nextChunk = pDescriptor->NextHandle ; 
      pRefChunk = (ismStore_memReferenceChunk_t *)(pDescriptor + 1);
      if (!memcmp(ismStore_memGlobal.zero4cmp, pRefChunk->References, sz) ) 
      {
        int i;
        for ( i=0 ; i<pRefChunk->ReferenceCount && !__sync_fetch_and_add(&pRefChunk->References[i].Flag,0) ; i++ ) ; //empty body
        if ( i<pRefChunk->ReferenceCount )
        {
          TRACE(1,"!!! Ref.Flag = %u for chunk %p of owner %p with oId %lu (%lu %u) !!!\n",
                pRefChunk->References[i].Flag,(void *)hChunk,(void *)pRefChunk->OwnerHandle,pRefChunk->BaseOrderId+i,pRefChunk->BaseOrderId,i);
        }
        else
        {
          nDeleted++ ; 
          if ( pDescPrev )
            pDescPrev->NextHandle = pDescriptor->NextHandle ; 
          else
            pRefGen->hReferenceHead = pDescriptor->NextHandle ;
          if (!delTail )
            delHead = hChunk ; 
          else
            delTail->NextHandle = hChunk ; 
          delTail = pDescriptor ; 
          pDescriptor->DataType = ismSTORE_DATATYPE_NEWLY_HATCHED ;
          TRACE(9," trimRefCunks: chunk %p of owner %p deleted, cmpsz %lu (%u %u %lu) , base oId: %lu\n",
                (void *)hChunk,(void *)pRefChunk->OwnerHandle,sz,pRefChunk->ReferenceCount,pGen->MaxRefsPerGranule,
                sizeof(ismStore_memReference_t),pRefChunk->BaseOrderId);
        }      
      }
      else
      {
        pDescPrev = pDescriptor ; 
        lastChunk = hChunk ; 
      }
    }
    if ( nDeleted )
    {
      if ( (pC = pRefGen->pRefCache) )
      {
        int j,k,l;
        for ( j=pC->cacheSize-1,l=0 ; j>= 0 ; j-- )
        {
          hChunk = pC->hRefCache[j];
          pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(hChunk)) ; 
          if ( pDescriptor->DataType == ismSTORE_DATATYPE_NEWLY_HATCHED )
          {
            pC->hRefCache[j] = 0 ; 
            l++ ; 
          }
        }
        if ( l )
        {
          for ( j=k=0 ; j<pC->cacheSize ; j++ )
          {
            if ( pC->hRefCache[j] )
            {
              if ( k<j )
                pC->hRefCache[k] = pC->hRefCache[j] ; 
              k++ ; 
            }
          }
          pC->cacheSize = k ; 
          if ( !k )
          {
            ismSTORE_FREE(pRefGen->pRefCache);
          }
        }
      }
      delTail->NextHandle = ismSTORE_NULL_HANDLE;
      ism_store_memReturnPoolElements(pStream, delHead, 1);

      if (ismSTORE_EXTRACT_OFFSET(pRefGen->hReferenceHead) != 0)
      {
        hChunk = pRefGen->hReferenceHead;
        pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(hChunk)) ; 
        pDescriptor->DataType = ismSTORE_DATATYPE_REFERENCES;
        ADR_WRITE_BACK(&pDescriptor->DataType, sizeof(pDescriptor->DataType));
        pRefChunk = (ismStore_memReferenceChunk_t *)(pDescriptor + 1);
        pRefGen->LowestOrderId = pRefChunk->BaseOrderId;
        pRefGen->hReferenceTail = lastChunk ; 
      }
      else
      {
         pRefGen->LowestOrderId = 0;
         pRefGen->HighestOrderId = 0;
         pRefGen->hReferenceHead = ismSTORE_BUILD_HANDLE(pStream->MyGenId, 0);
         pRefGen->hReferenceTail = 0;
      }
    }
    if ( (pC = pRefGen->pRefCache) )
      pC->nextTrimSecs = 5 + (uint32_t)(ism_common_monotonicTimeNanos()/1000000000) ;
    dt = ism_common_readTSC() - dt ; 
    TRACE(5,"%s: %u RefChunks out of %u were deleted for owner 0x%lx, time=%f, numRefs=%u, Threshs: %u %u\n",__FUNCTION__,
          nDeleted,pRefGen->numChunks/100,pRefCtxt->OwnerHandle,dt,pRefGen->numRefs/100,ismStore_memGlobal.RefChunkLWM,ismStore_memGlobal.RefChunkHWM);
    pRefGen->numChunks -= nDeleted*100 ; 
  }
}


// This is not transactional by design
static int32_t ism_store_memEnsureReferenceAllocation(ismStore_memStream_t *pStream,
                                                      ismStore_memReferenceContext_t *pRefCtxt,
                                                      uint64_t orderId,
                                                      ismStore_Handle_t *pHandle)
{
 #if USE_REFSTATS
   ismStore_memRefStats_t *pRS ; 
 #endif
   ismStore_memGeneration_t *pGen;
   ismStore_memDescriptor_t *pOwnerDescriptor, *pDescriptor = NULL, *pDescPrev, *pDescNext;
   ismStore_memReferenceChunk_t *pRefChunk = NULL;
   ismStore_memReference_t *pRef;
   ismStore_memSplitItem_t *pSplit;
   ismStore_memRefGen_t *pRefGen, *pRefGenTmp;
   ismStore_Handle_t hChunk = ismSTORE_NULL_HANDLE;
   int i, fTrim=0;
   ismStore_refGenCache_t *pC;
   uint64_t chunkBaseId;
   int32_t rc = ISMRC_OK;

   *pHandle = ismSTORE_NULL_HANDLE;
   pGen = &ismStore_memGlobal.InMemGens[pStream->MyGenIndex];

   ismSTORE_MUTEX_LOCK(pRefCtxt->pMutex);

   pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pRefCtxt->OwnerHandle);
   pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);
   pRefGen = ism_store_memGetRefGen(pStream, pRefCtxt);
   chunkBaseId = ismSTORE_ROUND(orderId, pGen->MaxRefsPerGranule);

   // See whether this owner already includes a reference chunk for this order Id
   if (pRefGen == NULL || orderId < pSplit->MinActiveOrderId)
   {
      TRACE(1, "Failed to add a reference (OrderId %lu, BaseOrderId %lu) for owner (Handle 0x%lx, DataType 0x%x, Version %u, MinActiveOrderId %lu), because the OrderId is not valid. LowestOrderId %lu, HighestOrderId %lu\n",
            orderId, chunkBaseId, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, pSplit->MinActiveOrderId, (pRefGen ? pRefGen->LowestOrderId : 0), (pRefGen ? pRefGen->HighestOrderId : 0));
      for (pRefGen = pRefCtxt->pRefGenHead; pRefGen; pRefGen = pRefGen->Next)
      {
         TRACE(1, "DEBUG: Owner 0x%lx, GenId %u, LowestOrderId %lu, HighestOrderId %lu, Head 0x%lx, Tail 0x%lx\n",
               pRefCtxt->OwnerHandle, ismSTORE_EXTRACT_GENID(pRefGen->hReferenceHead), pRefGen->LowestOrderId, pRefGen->HighestOrderId, pRefGen->hReferenceHead, pRefGen->hReferenceTail);
      }
      rc = ISMRC_ArgNotValid;
      ism_common_setErrorData(rc, "%s", "orderId");
      ismSTORE_MUTEX_UNLOCK(pRefCtxt->pMutex);
      return rc;
   }

  #if USE_REFSTATS
    pRS = pRefCtxt->pRefGenPool->RefStats+0 ; 
  #endif
   do
   {
     #if USE_REFSTATS
       pRS->nCL++ ; 
     #endif
      // See whether a refernce chunk should be added at the head of linked-list
      if (ismSTORE_EXTRACT_OFFSET(pRefGen->hReferenceHead) == 0 || orderId < pRefGen->LowestOrderId)
      {
        #if USE_REFSTATS
          pRS->nBH++ ; 
        #endif
         TRACE(9, "A reference chunk for owner (Handle 0x%lx, DataType 0x%x, OwnerVersion %u) is missing at the head. OrderId %lu, ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
               pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, orderId, chunkBaseId, pRefGen->LowestOrderId, pRefGen->HighestOrderId);

         // Allocate a new chunk and then link it in
         rc = ism_store_memAssignReferenceAllocation(pStream,
                                                     pRefCtxt,
                                                     chunkBaseId,
                                                     &hChunk,
                                                     &pDescriptor);

         if (rc == ISMRC_OK)
         {
            // Add the new chunk to the head of the list
            // Only the first chunk in the list is marked as ismSTORE_DATATYPE_REFERENCES.
            // The additional chunks are marked with the ismSTORE_DATATYPE_NOT_PRIMARY type
            pDescriptor->DataType = ismSTORE_DATATYPE_REFERENCES;

            if (ismSTORE_EXTRACT_OFFSET(pRefGen->hReferenceHead) == 0)
            {
               pDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
               ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));
               pRefGen->hReferenceTail = hChunk;
               pRefGen->HighestOrderId = orderId;
            }
            else
            {
               pDescriptor->NextHandle = pRefGen->hReferenceHead;
               ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));
               pDescNext = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(pRefGen->hReferenceHead)) ; 
               pDescNext->DataType = ismSTORE_DATATYPE_REFERENCES | ismSTORE_DATATYPE_NOT_PRIMARY;
               ADR_WRITE_BACK(&pDescNext->DataType, sizeof(pDescNext->DataType));
               fTrim = 1;
            }
            pRefGen->hReferenceHead = hChunk;
            pRefGen->LowestOrderId = chunkBaseId;
            pRefGen->numChunks += 100;
            pRefChunk = (ismStore_memReferenceChunk_t *)(pDescriptor + 1);
            ism_store_addChunk2Cache(pRefGen, hChunk);

            TRACE(9, "Add a reference chunk (Handle 0x%lx, DataType 0x%x, NextHandle 0x%lx) for owner (Handle 0x%lx, DataType 0x%x, Version %u) at the head. ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
                  hChunk, pDescriptor->DataType, pDescriptor->NextHandle, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, chunkBaseId, pRefGen->LowestOrderId, pRefGen->HighestOrderId);
         }
         else
         {
            TRACE(1, "Failed to allocate a reference chunk (OrderId %lu, BaseOrderId %lu) for owner (Handle 0x%lx, DataType 0x%x, Version %u). LowestOrderId %lu, HighestOrderId %lu\n",
                  orderId, chunkBaseId, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, pRefGen->LowestOrderId, pRefGen->HighestOrderId);
            pDescriptor = NULL;
         }
         break;
      }

      // See whether the order Id exists in the last reference chunk
      hChunk = pRefGen->hReferenceTail;
      pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(hChunk));
      pRefChunk = (ismStore_memReferenceChunk_t *)(pDescriptor + 1);
      if (orderId >= pRefChunk->BaseOrderId && orderId < pRefChunk->BaseOrderId + pRefChunk->ReferenceCount)
      {
        #if USE_REFSTATS
          pRS->nTL++ ; 
        #endif
         if (orderId > pRefGen->HighestOrderId)
         {
            pRefGen->HighestOrderId = orderId;
            // Optimization: If the NextPruneOrderId is in the middle of the last active chunk and
            // there are no references on the disk, then we can update it to the new HighestOrderId
            if (pRefCtxt->NextPruneOrderId > pRefChunk->BaseOrderId && pRefGen->hReferenceHead == pRefGen->hReferenceTail && pRefCtxt->pRefGenHead == pRefGen)
            {
               pRefCtxt->NextPruneOrderId = pRefGen->HighestOrderId + 1;
            }
         }
         break;
      }
      hChunk = ismSTORE_NULL_HANDLE;

      // See whether a refernce chunk should be added at the tail of linked-list
      if (orderId >= pRefChunk->BaseOrderId + pRefChunk->ReferenceCount)
      {
        #if USE_REFSTATS
          pRS->nAT++ ; 
        #endif
         TRACE(9, "A reference chunk for owner (Handle 0x%lx, DataType 0x%x, OwnerVersion %u) is missing at the tail. OrderId %lu, ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
               pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, orderId, chunkBaseId, pRefGen->LowestOrderId, pRefGen->HighestOrderId);

         // Allocate a new chunk and then link it in
         rc = ism_store_memAssignReferenceAllocation(pStream,
                                                     pRefCtxt,
                                                     chunkBaseId,
                                                     &hChunk,
                                                     &pDescriptor);

         if (rc == ISMRC_OK)
         {
            // Add the new chunk to the tail of the list
            pDescriptor->DataType = ismSTORE_DATATYPE_REFERENCES | ismSTORE_DATATYPE_NOT_PRIMARY;
            ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));

            pDescPrev = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(pRefGen->hReferenceTail));
            pDescPrev->NextHandle = hChunk;
            ADR_WRITE_BACK(&pDescPrev->NextHandle, sizeof(pDescPrev->NextHandle));
            pRefGen->hReferenceTail = hChunk;
            pRefGen->HighestOrderId = orderId;
            pRefGen->numChunks += 100;
            pRefChunk = (ismStore_memReferenceChunk_t *)(pDescriptor + 1);
            ism_store_addChunk2Cache(pRefGen, hChunk);
            fTrim = 1;

            TRACE(9, "Add a reference chunk (Handle 0x%lx, DataType 0x%x, NextHandle 0x%lx) for owner (Handle 0x%lx, DataType 0x%x, Version %u) at the tail. ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
                  hChunk, pDescriptor->DataType, pDescriptor->NextHandle, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, chunkBaseId, pRefGen->LowestOrderId, pRefGen->HighestOrderId);
         }
         else
         {
            TRACE(1, "Failed to allocate a reference chunk (OrderId %lu, BaseOrderId %lu) for owner (Handle 0x%lx, DataType 0x%x, Version %u). LowestOrderId %lu, HighestOrderId %lu\n",
                  orderId, chunkBaseId, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, pRefGen->LowestOrderId, pRefGen->HighestOrderId);
            pDescriptor = NULL;
         }
         break;
      }

      // Try to use the cache
      if ( (pC = pRefGen->pRefCache) )
      {
        int j,k;
        ismStore_Handle_t hGLB=ismSTORE_NULL_HANDLE;
        uint64_t oidGLB=0 ; 
        for ( j=k=pC->cacheSize-1 ; j>=0 ; j-- )
        {
          hChunk = pC->hRefCache[j] ; 
          pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(hChunk));
          pRefChunk = (ismStore_memReferenceChunk_t *)(pDescriptor + 1);
          #if USE_REFSTATS
            pRS->nBT++ ; 
          #endif
          if (orderId >= pRefChunk->BaseOrderId && orderId < pRefChunk->BaseOrderId + pRefChunk->ReferenceCount)
          {
            #if USE_REFSTATS
              pRS->nAC++ ; 
            #endif
             if ( (k-j) )
             {
               memmove(pC->hRefCache+j, pC->hRefCache+j+1, (k-j)*sizeof(ismStore_Handle_t)) ;
               pC->hRefCache[k] = hChunk ; 
             }
             break;
          }
          if (orderId >= pRefChunk->BaseOrderId + pRefChunk->ReferenceCount && oidGLB < pRefChunk->BaseOrderId )
          {
            oidGLB = pRefChunk->BaseOrderId ; 
              hGLB = hChunk ; 
          }
        }
        if ( j>=0 )
          break ; 
        hChunk = hGLB ; 
      }

      // Search for the reference chunk that contains the required order Id or create a new chunk
     #if USE_REFSTATS
       pRS->nLL++ ; 
     #endif
      if ( hChunk == ismSTORE_NULL_HANDLE )
         hChunk = pRefGen->hReferenceHead;
      for (pDescPrev = pDescNext = NULL; ismSTORE_EXTRACT_OFFSET(hChunk) != 0; hChunk = pDescPrev->NextHandle)
      {
        #if USE_REFSTATS
          pRS->nEL++ ; 
        #endif
         pDescNext = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(hChunk));

         // Sanity check: make sure that the chunk is valid
         if ((pDescNext->DataType & (~ismSTORE_DATATYPE_NOT_PRIMARY)) != ismSTORE_DATATYPE_REFERENCES)
         {
            TRACE(1, "The Reference chunk (Handle 0x%lx, DataType 0x%x) of the owner (Handle 0x%lx, DataType 0x%x, Version %u) is not valid. OrderId %lu, ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
                  hChunk, pDescNext->DataType, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, orderId, chunkBaseId, pRefGen->LowestOrderId, pRefGen->HighestOrderId);
            hChunk = ismSTORE_NULL_HANDLE;
            break;
         }

         pRefChunk = (ismStore_memReferenceChunk_t *)(pDescNext + 1);
         if (orderId >= pRefChunk->BaseOrderId && orderId < pRefChunk->BaseOrderId + pRefChunk->ReferenceCount)
         {
            // We found the the required chunk
            ism_store_addChunk2Cache(pRefGen, hChunk);
            pDescriptor = pDescNext;
            break;
         }

         if (orderId < pRefChunk->BaseOrderId)
         {
            if (pDescPrev == NULL)
            {
               TRACE(1, "The reference chunk list of owner (Handle 0x%lx, DataType 0x%x, Version %u) is not valid. OrderId %lu, ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
                     pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, orderId, chunkBaseId, pRefGen->LowestOrderId, pRefGen->HighestOrderId);
               hChunk = ismSTORE_NULL_HANDLE;
               break;
            }

            // We need to create a new chunk for the specified order Id
            TRACE(9, "A reference chunk for owner (Handle 0x%lx, DataType 0x%x, Version %u) is missing at the middle. OrderId %lu, ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
                  pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, orderId, chunkBaseId, pRefGen->LowestOrderId, pRefGen->HighestOrderId);

            // Allocate a new chunk and then link it in
            rc = ism_store_memAssignReferenceAllocation(pStream,
                                                        pRefCtxt,
                                                        chunkBaseId,
                                                        &hChunk,
                                                        &pDescriptor);

            if (rc == ISMRC_OK)
            {
               pDescriptor->NextHandle = pDescPrev->NextHandle;
               pDescriptor->DataType = ismSTORE_DATATYPE_REFERENCES | ismSTORE_DATATYPE_NOT_PRIMARY;
               ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor));

               pDescPrev->NextHandle = hChunk;
               ADR_WRITE_BACK(&pDescPrev->NextHandle, sizeof(pDescPrev->NextHandle));
               pRefGen->numChunks += 100;
               pRefChunk = (ismStore_memReferenceChunk_t *)(pDescriptor + 1);
               ism_store_addChunk2Cache(pRefGen, hChunk);
               fTrim = 1;

               TRACE(9, "Add a reference chunk (Handle 0x%lx, DataType 0x%x, NextHandle 0x%lx) for owner (Handle 0x%lx, DataType 0x%x, Version %u). ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
                     hChunk, pDescriptor->DataType, pDescriptor->NextHandle, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, chunkBaseId, pRefGen->LowestOrderId, pRefGen->HighestOrderId);
            }
            else
            {
               TRACE(1, "Failed to allocate a reference chunk (OrderId %lu, BaseOrderId %lu) for owner (Handle 0x%lx, DataType 0x%x, Version %u). LowestOrderId %lu, HighestOrderId %lu\n",
                     orderId, chunkBaseId, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, pRefGen->LowestOrderId, pRefGen->HighestOrderId);
               hChunk = ismSTORE_NULL_HANDLE;
               pDescriptor = NULL;
            }
            break;
         }
         pDescPrev = pDescNext;
      }
   } while (0);

   if (ismSTORE_EXTRACT_OFFSET(hChunk) == 0 || pDescriptor == NULL || pRefChunk == NULL)
   {
      TRACE(1, "Failed to add a reference chunk (OrderId %lu, GenId %u, GenIndex %u, BaseOrderId %lu) " \
            "to owner (Handle 0x%lx, DataType 0x%x, Version %u, MinActiveOrderId %lu). "\
            "hReferenceHead 0x%lx, hReferenceTail 0x%lx, LowestOrderId %lu, HighestOrderId %lu, pRefCtxt %p\n",
            orderId, pStream->MyGenId, pStream->MyGenIndex, chunkBaseId, pRefCtxt->OwnerHandle,
            pOwnerDescriptor->DataType, pSplit->Version, pSplit->MinActiveOrderId,
            pRefGen->hReferenceHead, pRefGen->hReferenceTail, pRefGen->LowestOrderId,
            pRefGen->HighestOrderId, pRefCtxt);
      rc = ISMRC_Error;
      goto exit;
   }

   // Sanity check: verify that the reference chunk is correct
   if (pRefChunk->BaseOrderId != chunkBaseId)
   {
      TRACE(1, "Failed to add a reference (OrderId %lu, BaseOrderId %lu) to owner (Handle 0x%lx, DataType 0x%x, OwnerVersion %u, MinActiveOrderId %lu) " \
            "because the reference chunk (Handle 0x%lx, DataType 0x%x, ChunkBaseOrderId %lu, OwnerHandle 0x%lx, OwnerVersion %u) is not valid. "\
            "hReferenceHead 0x%lx, hReferenceTail 0x%lx, LowestOrderId %lu, HighestOrderId %lu, pRefCtxt %p\n",
            orderId, chunkBaseId, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType,
            pRefCtxt->OwnerVersion, pSplit->MinActiveOrderId, hChunk, pDescriptor->DataType,
            pRefChunk->BaseOrderId, pRefChunk->OwnerHandle, pRefChunk->OwnerVersion,
            pRefGen->hReferenceHead, pRefGen->hReferenceTail, pRefGen->LowestOrderId,
            pRefGen->HighestOrderId, pRefCtxt);
      rc = ISMRC_Error;
      goto exit;
   }

   pRefGen->numRefs += 100 ; 
   pRef = &pRefChunk->References[orderId - pRefChunk->BaseOrderId];
 //pRef->Flag = 1 ; 
   __sync_fetch_and_or(&pRef->Flag,1);
   *pHandle = (ismStore_Handle_t)(hChunk + ((char *)pRef - (char *)pDescriptor));
   if ( fTrim )
     ism_store_trimRefCunks(pStream, pRefCtxt, pRefGen, pGen) ; 
   ismSTORE_MUTEX_UNLOCK(pRefCtxt->pMutex);
   return rc;

exit:
   // We didn't find a valid reference chunk. It means that something wrong happended, so
   // we will try to print as much information as possible.

   // Print the RefGen cache of the current RefCtxt
   for (i=0; i < ismStore_memGlobal.InMemGensCount; i++)
   {
      TRACE(1, "RefGenCache: Owner 0x%lx, Index %u, ptr %p\n", pRefCtxt->OwnerHandle, i, pRefCtxt->pInMemRefGen[i]);
   }

   // Print the RefGen map of the current owner
   for (pRefGenTmp = pRefCtxt->pRefGenHead; pRefGenTmp; pRefGenTmp = pRefGenTmp->Next)
   {
      TRACE(1, "RefGen: ptr %p, GenId %u, Owner 0x%lx, hReferenceHead 0x%lx, hReferenceTail 0x%lx, LowestOrderId %lu, HighestOrderId %lu\n",
            pRefGenTmp, ismSTORE_EXTRACT_GENID(pRefGenTmp->hReferenceHead), pRefCtxt->OwnerHandle, pRefGenTmp->hReferenceHead,
            pRefGenTmp->hReferenceTail, pRefGenTmp->LowestOrderId, pRefGenTmp->HighestOrderId);
   }

   // Print the linked-list of RefChunks in the current generation
   for (hChunk = pRefGen->hReferenceHead; ismSTORE_EXTRACT_OFFSET(hChunk) != 0; hChunk = pDescNext->NextHandle)
   {
      pDescNext = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hChunk);
      pRefChunk = (ismStore_memReferenceChunk_t *)(pDescNext + 1);
      TRACE(1, "RefChunk: Handle 0x%lx, DataType 0x%x, Owner 0x%lx, BaseOrderId %lu, ReferenceCount %u, NextHandle 0x%lx\n",
            hChunk, pDescNext->DataType, pRefChunk->OwnerHandle, pRefChunk->BaseOrderId, pRefChunk->ReferenceCount, pDescNext->NextHandle);

      // Sanity check: make sure that the chunk is valid
      if ((pDescNext->DataType & (~ismSTORE_DATATYPE_NOT_PRIMARY)) != ismSTORE_DATATYPE_REFERENCES)
      {
         TRACE(1, "The Reference chunk (Handle 0x%lx, DataType 0x%x) of the owner (Handle 0x%lx, DataType 0x%x) is not valid. OrderId %lu, ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
               hChunk, pDescNext->DataType, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, orderId, chunkBaseId, pRefGen->LowestOrderId, pRefGen->HighestOrderId);
         break;
      }
   }

   ismSTORE_MUTEX_UNLOCK(pRefCtxt->pMutex);

   return rc;
}

// This is not transactional by design
static int32_t ism_store_memAssignReferenceAllocation(ismStore_memStream_t *pStream,
                                                      ismStore_memReferenceContext_t *pRefCtxt,
                                                      uint64_t baseOrderId,
                                                      ismStore_Handle_t *pHandle,
                                                      ismStore_memDescriptor_t **pDesc)
{
   ismStore_memGeneration_t *pGen;
   ismStore_memDescriptor_t *pDescriptor = NULL;
   ismStore_memReferenceChunk_t *pRefChunk;
   ismStore_Handle_t handle;
   int32_t rc = ISMRC_OK;

   pGen = &ismStore_memGlobal.InMemGens[pStream->MyGenIndex];
   rc = ism_store_memGetPoolElements(pStream,
                                     pGen,
                                     0,
                                     ismSTORE_DATATYPE_REFERENCES,
                                     0, 0, ismSTORE_SINGLE_GRANULE, 0,
                                     &handle, &pDescriptor);
   if (rc != ISMRC_OK)
   {
      return rc;
   }

   if (pDescriptor != NULL) {

       // Initialize the data
       pRefChunk = (ismStore_memReferenceChunk_t *)(pDescriptor + 1);
       pRefChunk->BaseOrderId = baseOrderId;
       pRefChunk->ReferenceCount = pGen->MaxRefsPerGranule;
       pRefChunk->OwnerHandle = pRefCtxt->OwnerHandle;
       pRefChunk->OwnerVersion = pRefCtxt->OwnerVersion;
       memset(pRefChunk->References, '\0', pRefChunk->ReferenceCount * sizeof(ismStore_memReference_t));

       // No need to call WRITE_BACK for the header, because the caller calls
       // We just need to call WRITE_BACK for the array of References
       ADR_WRITE_BACK(pRefChunk->References, pRefChunk->ReferenceCount * sizeof(ismStore_memReference_t));

       *pHandle = handle;
       *pDesc = pDescriptor;
   }
   else {
      // Descriptor should not be NULL if rc is ISMRC_OK
      TRACE(1, "Descriptor is NULL\n");
   }
   return rc;
}

int32_t ism_store_memPruneReferences(ismStore_StreamHandle_t hStream, void *hRefCtxt, uint64_t minimumActiveOrderId)
{
   ismStore_memReferenceContext_t *pRefCtxt = (ismStore_memReferenceContext_t *)hRefCtxt;
   ismStore_memStream_t *pStream;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memValidateRefCtxt(pRefCtxt)) != ISMRC_OK)
   {
      TRACE(1, "Failed to prune references, because the reference context is not valid\n");
      return rc;
   }

   if (minimumActiveOrderId >= pRefCtxt->NextPruneOrderId)
   {
      // We have to take the internal stream (index=0) to make sure that the generation
      // is not being evacuated from the store during the operation
      if ( hStream == (ismStore_StreamHandle_t)(-1) )
         hStream = ismStore_memGlobal.hInternalStream;
      else
      if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
      {
         TRACE(1, "Failed to prune references, because the stream is not valid\n");
         return rc;
      }
      pStream = ismStore_memGlobal.pStreams[hStream];
      ism_store_memSetStreamActivity(pStream, 1);

      ism_store_memPruneReferenceAllocation(pStream, pRefCtxt, minimumActiveOrderId);

      // Release the internal stream
      ism_store_memSetStreamActivity(pStream, 0);
   }

   return rc;
}

/*
 * Prune references for a specified owner using the minimumActiveOrderId.
 *
 * This method assumes that the input is correct. That is, the minimumActiveOrderId is less than the
 * lowest pending (= uncommitted reference) order id.
 *
 * This is not transactional by design
 */
static void ism_store_memPruneReferenceAllocation(ismStore_memStream_t *pStream,
                                                  ismStore_memReferenceContext_t *pRefCtxt,
                                                  uint64_t minimumActiveOrderId)
{
   ismStore_memDescriptor_t *pOwnerDescriptor, *pChunkDescriptor;
   ismStore_memSplitItem_t *pSplit;
   ismStore_memReferenceChunk_t *pRefChunk;
   ismStore_memRefStateChunk_t *pRefStateChunk;
   ismStore_memRefGen_t *pRefGen;
   ismStore_memJob_t job;
   ismStore_Handle_t hChunk;
   uint8_t fHasDiskRefs;
   uint64_t nextChunkBaseOid;
   int nDeleted, i;

   TRACE(9, "Entry: %s. OwnerHandle 0x%lx, NextPruneOrderId %lu, MinActiveOrderId %lu\n",
         __FUNCTION__, pRefCtxt->OwnerHandle, pRefCtxt->NextPruneOrderId, minimumActiveOrderId);

   ismSTORE_MUTEX_LOCK(pRefCtxt->pMutex);
   pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pRefCtxt->OwnerHandle);
   pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);

   if (minimumActiveOrderId < pSplit->MinActiveOrderId)
   {
      TRACE(8, "The new MinActiveOrderId %lu of owner (Handle 0x%lx, DataType 0x%x) is less than the current MinActiveOrderId %lu. NextPruneOrderId %lu\n",
            minimumActiveOrderId, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pSplit->MinActiveOrderId, pRefCtxt->NextPruneOrderId);
      ismSTORE_MUTEX_UNLOCK(pRefCtxt->pMutex);
      return;
   }
   else if (minimumActiveOrderId == pSplit->MinActiveOrderId || minimumActiveOrderId < pRefCtxt->NextPruneOrderId)
   {
      ismSTORE_MUTEX_UNLOCK(pRefCtxt->pMutex);
      return;
   }

   // If a Standby node exists, we need to update the Standby
   // The store thread is repsonisble to send the update message to the Standby node
   if (ismSTORE_HASSTANDBY || ismStore_memGlobal.fEnablePersist)
   {
      memset(&job, '\0', sizeof(job));
      job.JobType = StoreJob_HASendMinActiveOid;
      job.UpdOrderId.OwnerHandle = pRefCtxt->OwnerHandle;
      job.UpdOrderId.MinActiveOid = minimumActiveOrderId;
      ism_store_memAddJob(&job);
   }
   pSplit->MinActiveOrderId = minimumActiveOrderId;
   ADR_WRITE_BACK(&pSplit->MinActiveOrderId, sizeof(pSplit->MinActiveOrderId));

   fHasDiskRefs = (pRefCtxt->pRefGenHead ? 1 : 0);
   while ((pRefGen = pRefCtxt->pRefGenHead))
   {
      // See whether the generation is still in the store memory
      if (ismSTORE_EXTRACT_GENID(pRefGen->hReferenceHead) == pStream->MyGenId)
      {
         ismStore_Handle_t delHead=0;
         ismStore_memDescriptor_t *delTail=NULL;
         fHasDiskRefs = 0;
         nDeleted = 0;
         while (ismSTORE_EXTRACT_OFFSET(pRefGen->hReferenceHead))
         {
            // Link the hReferenceHead field to the second reference chunk
            hChunk = pRefGen->hReferenceHead;
            pChunkDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hChunk);
            pRefChunk = (ismStore_memReferenceChunk_t *)(pChunkDescriptor + 1);
            nextChunkBaseOid = pRefChunk->BaseOrderId + pRefChunk->ReferenceCount;

            if (minimumActiveOrderId < nextChunkBaseOid && minimumActiveOrderId <= pRefGen->HighestOrderId)
            {
               // Update the LowestOrderId of the RefGen
               pRefGen->LowestOrderId = pRefChunk->BaseOrderId;
               pRefCtxt->NextPruneOrderId = (nextChunkBaseOid < pRefGen->HighestOrderId ? nextChunkBaseOid : pRefGen->HighestOrderId + 1);

               TRACE(9, "Reference chunk (Handle 0x%lx, BaseOrderId %lu, ReferenceCount %u) of owner (Handle 0x%lx, " \
                     "DataType 0x%x, LowestOrderId %lu, HighestOrderId %lu, MinActiveOrderId %lu, NextPruneOrderId %lu) in generation Id %u (Index %u) is still valid. nDeleted %d\n",
                     hChunk, pRefChunk->BaseOrderId, pRefChunk->ReferenceCount, pRefCtxt->OwnerHandle,
                     pOwnerDescriptor->DataType, pRefGen->LowestOrderId, pRefGen->HighestOrderId,
                     minimumActiveOrderId, pRefCtxt->NextPruneOrderId, pStream->MyGenId, pStream->MyGenIndex, nDeleted);
               break;
            }

            pRefCtxt->NextPruneOrderId = (nextChunkBaseOid < pRefGen->HighestOrderId ? nextChunkBaseOid : pRefGen->HighestOrderId + 1);
            TRACE(9, "Prune references for owner (Handle 0x%lx, DataType 0x%x) in generation Id %u (Index %u) - " \
                  "Chunk (Handle 0x%lx, BaseOrderId %lu, ReferenceCount %u), LowestOrderId %lu, HighestOrderId %lu, MinActiveOrderId %lu, NextPruneOrderId %lu.\n",
                  pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pStream->MyGenId, pStream->MyGenIndex,
                  hChunk, pRefChunk->BaseOrderId, pRefChunk->ReferenceCount, pRefGen->LowestOrderId,
                  pRefGen->HighestOrderId, minimumActiveOrderId, pRefCtxt->NextPruneOrderId);

            if (!delHead )
              delHead = hChunk ; 
            delTail = pChunkDescriptor ; 
            pRefGen->hReferenceHead = pChunkDescriptor->NextHandle;
          //pChunkDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
          //ism_store_memReturnPoolElements(pStream, hChunk, 1);

            nDeleted++;
         }

         if ( nDeleted )
         {
           ismStore_refGenCache_t *pC ; 
           pRefGen->numChunks -= nDeleted*100 ; 
           if ( (pC = pRefGen->pRefCache) )
           {
             int j,k,l;
             for ( j=pC->cacheSize-1,l=0 ; j>= 0 ; j-- )
             {
               hChunk = pC->hRefCache[j];
               pChunkDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hChunk);
               pRefChunk = (ismStore_memReferenceChunk_t *)(pChunkDescriptor + 1);
               nextChunkBaseOid = pRefChunk->BaseOrderId + pRefChunk->ReferenceCount;
               if (!(minimumActiveOrderId < nextChunkBaseOid && minimumActiveOrderId <= pRefGen->HighestOrderId))
               {
                 pC->hRefCache[j] = 0 ; 
                 l++ ; 
               }
             }
             if ( l )
             {
               for ( j=k=0 ; j<pC->cacheSize ; j++ )
               {
                 if ( pC->hRefCache[j] )
                 {
                   if ( k<j )
                     pC->hRefCache[k] = pC->hRefCache[j] ; 
                   k++ ; 
                 }
               }
               pC->cacheSize = k ; 
               if ( !k )
               {
                 ismSTORE_FREE(pRefGen->pRefCache);
               }
             }
           }
           delTail->NextHandle = ismSTORE_NULL_HANDLE;
           ism_store_memReturnPoolElements(pStream, delHead, 1);
         }

         if (ismSTORE_EXTRACT_OFFSET(pRefGen->hReferenceHead) != 0)
         {
            // Only the first chunk in the list is marked as ismSTORE_DATATYPE_REFERENCES.
            // The additional chunks are marked with the ismSTORE_DATATYPE_NOT_PRIMARY type
            if (nDeleted > 0)
            {
               pChunkDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pRefGen->hReferenceHead);
               pChunkDescriptor->DataType = ismSTORE_DATATYPE_REFERENCES;
               ADR_WRITE_BACK(&pChunkDescriptor->DataType, sizeof(pChunkDescriptor->DataType));
            }
         }
         else
         {
            pRefGen->LowestOrderId = 0;
            pRefGen->HighestOrderId = 0;
            pRefGen->hReferenceHead = ismSTORE_BUILD_HANDLE(pStream->MyGenId, 0);
            pRefGen->hReferenceTail = 0;
            TRACE(9, "There are no more references for owner (Handle 0x%lx, DataType 0x%x) in memory generation Id %u (Index %u).\n",
                  pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pStream->MyGenId, pStream->MyGenIndex);
         }
         break;
      }
      else
      {
         if (minimumActiveOrderId > pRefGen->HighestOrderId)
         {
            // We can delete the whole RefGen, because all the references have been deleted
            TRACE(8, "There are no more references for owner (Handle 0x%lx, DataType 0x%x, MinActiveOrderId %lu, NextPruneOrderId %lu) " \
                  "in disk generation Id %u. pRefGen %p, pRefGenLast %p, hReferenceHead 0x%lx, hReferenceTail 0x%lx, LowestOrderId %lu, HighestOrderId %lu\n",
                  pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, minimumActiveOrderId, pRefCtxt->NextPruneOrderId,
                  ismSTORE_EXTRACT_GENID(pRefGen->hReferenceHead), pRefGen, pRefCtxt->pRefGenLast, pRefGen->hReferenceHead,
                  pRefGen->hReferenceTail, pRefGen->LowestOrderId, pRefGen->HighestOrderId);

            // See whether the pRefGen is still in the RefCtxt cache
            for (i=0; i < ismStore_memGlobal.InMemGensCount; i++)
            {
               TRACE(9, "RefGen (GenId %u, ptr %p) of owner (Handle 0x%lx, DataType 0x%x, MinActiveOrderId %lu). pInMemRefGen[%d] %p\n",
                     ismSTORE_EXTRACT_GENID(pRefGen->hReferenceHead), pRefGen, pRefCtxt->OwnerHandle,
                     pOwnerDescriptor->DataType, minimumActiveOrderId, i, pRefCtxt->pInMemRefGen[i]);
               if (pRefCtxt->pInMemRefGen[i] == pRefGen)
               {
                  pRefCtxt->pInMemRefGen[i] = NULL;
                  TRACE(8, "The RefGen (GenId %u, ptr %p) of owner (Handle 0x%lx, DataType 0x%x, MinActiveOrderId %lu) has been removed from the RefCtxt cache " \
                        "(Index %u). hReferenceHead 0x%lx, hReferenceTail 0x%lx, LowestOrderId %lu, HighestOrderId %lu\n",
                        ismSTORE_EXTRACT_GENID(pRefGen->hReferenceHead), pRefGen, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType,
                        minimumActiveOrderId, i, pRefGen->hReferenceHead, pRefGen->hReferenceTail, pRefGen->LowestOrderId, pRefGen->HighestOrderId);
               }
            }

            // See whether the pRefGen is the pRefGenLast
            if (pRefCtxt->pRefGenLast == pRefGen)
            {
               pRefCtxt->pRefGenLast = NULL;
               TRACE(8, "The RefGen (GenId %u, ptr %p) of owner (Handle 0x%lx, DataType 0x%x, MinActiveOrderId %lu) has been removed from the pRefGenLast cache\n",
                     ismSTORE_EXTRACT_GENID(pRefGen->hReferenceHead), pRefGen, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, minimumActiveOrderId);
            }

            // If the RefGen is not empty, then we have to turn off the bit of the first reference chunk
            if (ismSTORE_EXTRACT_OFFSET(pRefGen->hReferenceHead))
            {
               ism_store_memResetGenMap(pRefGen->hReferenceHead);
            }
            pRefCtxt->pRefGenHead = pRefGen->Next;
            ism_store_memFreeRefGen(pRefCtxt, pRefGen);
         }
         else
         {
            // No need to continue to the next generation, because the RefGen are sorted
            pRefCtxt->NextPruneOrderId = pRefGen->HighestOrderId + 1;
            break;
         }
      }
   }

   // Prune RefState chunks
   if ((pRefGen = pRefCtxt->pRefGenState))
   {
      nDeleted = 0;
      while (ismSTORE_EXTRACT_OFFSET(pRefGen->hReferenceHead) > 0)
      {
         hChunk = pRefGen->hReferenceHead;
         pChunkDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hChunk);
         pRefStateChunk = (ismStore_memRefStateChunk_t *)(pChunkDescriptor + 1);
         nextChunkBaseOid = pRefStateChunk->BaseOrderId + pRefStateChunk->RefStateCount;

         // Optimization: We can remove the RefStateChunk if it does not contain any information and the owner has no
         // references on the disk. However, due to the fact that we don't delete the RefStateChunks on the Standby
         // node, the result of this optimization is that the Standby node might have multiple chunks with the same
         // BaseOrderId. Thus, we have decided at this point to not use that optimization.
         //if (minimumActiveOrderId < nextChunkBaseOid && (minimumActiveOrderId <= pRefGen->HighestOrderId || fHasDiskRefs))

         if (minimumActiveOrderId < nextChunkBaseOid)
         {
            // Update the LowestOrderId
            pRefGen->LowestOrderId = pRefStateChunk->BaseOrderId;
            if (nextChunkBaseOid < pRefCtxt->NextPruneOrderId)
            {
               pRefCtxt->NextPruneOrderId = (nextChunkBaseOid < pRefGen->HighestOrderId ? nextChunkBaseOid : pRefGen->HighestOrderId + 1);
            }
            TRACE(9, "RefStates chunk (Handle 0x%lx, BaseOrderId %lu, ReferenceCount %u) of owner (Handle 0x%lx, " \
                  "DataType 0x%x, LowestOrderId %lu, HighestOrderId %lu, MinActiveOrderId %lu, NextPruneOrderId %lu, ChunksInUse %u) is still valid. nDeleted %d\n",
                  hChunk, pRefStateChunk->BaseOrderId, pRefStateChunk->RefStateCount, pRefCtxt->OwnerHandle,
                  pOwnerDescriptor->DataType, pRefGen->LowestOrderId, pRefGen->HighestOrderId, minimumActiveOrderId,
                  pRefCtxt->NextPruneOrderId, pRefCtxt->RFChunksInUse, nDeleted);
            break;
         }

         // Link the hReferenceHead field to the second RefState chunk
         TRACE(9, "Prune RefStates chunk for owner (Handle 0x%lx, DataType 0x%x) - Chunk (Handle 0x%lx, BaseOrderId %lu, " \
               "RefStateCount %u), LowestOrderId %lu, HighestOrderId %lu, MinActiveOrderId %lu, NextPruneOrderId %lu, ChunksInUse %u, hRFCacheChunk 0x%lx 0x%lx, fHasDiskRefs %u\n",
               pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, hChunk, pRefStateChunk->BaseOrderId,
               pRefStateChunk->RefStateCount, pRefGen->LowestOrderId, pRefGen->HighestOrderId, minimumActiveOrderId,
               pRefCtxt->NextPruneOrderId, pRefCtxt->RFChunksInUse, pRefCtxt->hRFCacheChunk[0], pRefCtxt->hRFCacheChunk[1], fHasDiskRefs);

         pRefGen->hReferenceHead = pChunkDescriptor->NextHandle;
         pChunkDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
         ism_store_memReturnPoolElements(pStream, hChunk, 1);
         if (pRefCtxt->hRFCacheChunk[0] == hChunk)
         {
            pRefCtxt->hRFCacheChunk[0] = ismSTORE_NULL_HANDLE;
         }
         if (pRefCtxt->hRFCacheChunk[1] == hChunk)
         {
            pRefCtxt->hRFCacheChunk[1] = ismSTORE_NULL_HANDLE;
         }
         pRefCtxt->RFChunksInUse--;
         if ( pRefCtxt->pRFFingers )
         {
           ismStore_memRefStateFingers_t *pF = pRefCtxt->pRFFingers ; 
           if ( pF->NumInUse && pF->Handles[0] == hChunk )
           {
             pF->BaseOid[0] = nextChunkBaseOid ; 
             pF->Handles[0] = pRefGen->hReferenceHead ; 
             if ( pF->NumInUse > 1 && pF->Handles[0] == pF->Handles[1] )
             {
               pF->NumInUse-- ; 
               memmove(pF->Handles, pF->Handles+1, pF->NumInUse * sizeof(ismStore_Handle_t)) ; 
               memmove(pF->BaseOid, pF->BaseOid+1, pF->NumInUse * sizeof(uint64_t)) ; 
             }
           }
         }
         nDeleted++;
      }

      if (nDeleted > 0)
      {
        if ( pRefCtxt->pRFFingers )
          ism_store_memBuildRFFingers(pRefCtxt) ; 
        
        if (pRefCtxt->RFChunksInUse < pRefCtxt->RFChunksInUseLWM)
        {
           if ((pRefCtxt->RFChunksInUseLWM = pRefCtxt->RFChunksInUse / 2) < ismSTORE_REFSTATES_CHUNKS)
           {
              pRefCtxt->RFChunksInUseLWM = 0 ; //ismSTORE_REFSTATES_CHUNKS;
           }
           pRefCtxt->RFChunksInUseHWM = ismSTORE_ROUNDUP(pRefCtxt->RFChunksInUse, ismSTORE_REFSTATES_CHUNKS);
           TRACE(5, "The number of RefState chunks of the owner (Handle 0x%lx, DataType 0x%x, Version %u) was reduced to %u " \
                 "(LowestOrderId %lu, HighestOrderId %lu, MinActiveOrderId %lu, NextPruneOrderId %lu, ChunksInUseLWM %u, ChunksInUseHWM %u)\n",
                 pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, pRefCtxt->RFChunksInUse,
                 pRefGen->LowestOrderId, pRefGen->HighestOrderId, minimumActiveOrderId, pRefCtxt->NextPruneOrderId,
                 pRefCtxt->RFChunksInUseLWM, pRefCtxt->RFChunksInUseHWM);
        }
      }

      if (ismSTORE_EXTRACT_OFFSET(pRefGen->hReferenceHead) == 0)
      {
         memset(pRefGen, '\0', sizeof(*pRefGen));
         pRefCtxt->RFChunksInUse = 0;
         pRefCtxt->RFChunksInUseLWM = 0;
         pRefCtxt->RFChunksInUseHWM = ismSTORE_REFSTATES_CHUNKS;
         pRefCtxt->hRFCacheChunk[0] = ismSTORE_NULL_HANDLE;
         pRefCtxt->hRFCacheChunk[1] = ismSTORE_NULL_HANDLE;
         ismSTORE_FREE(pRefCtxt->pRFFingers);
         TRACE(9, "There are no more RefStates chunks for owner (Handle 0x%lx, DataType 0x%x)\n", pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType);
      }
   }

   ismSTORE_MUTEX_UNLOCK(pRefCtxt->pMutex);
   TRACE(9, "Exit: %s. NextPruneOrderId %lu\n", __FUNCTION__, pRefCtxt->NextPruneOrderId);
}

int32_t ism_store_memUpdateReference(ismStore_StreamHandle_t hStream,
                                     void *hRefCtxt,
                                     ismStore_Handle_t handle,
                                     uint64_t orderId,
                                     uint8_t state,
                                     uint64_t minimumActiveOrderId,
                                     uint8_t fAutoCommit)
{
   ismStore_memStream_t *pStream;
   ismStore_memReferenceContext_t *pRefCtxt = (ismStore_memReferenceContext_t *)hRefCtxt;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_memStoreOperation_t *pOp;
   uint8_t fRefState = 0;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
      TRACE(1, "Failed to update a reference, because the stream is not valid\n");
      return rc;
   }

   if ((rc = ism_store_memValidateRefCtxt(pRefCtxt)) != ISMRC_OK)
   {
      TRACE(1, "Failed to update a reference, because the reference context is not valid\n");
      return rc;
   }

   if ((rc = ism_store_memValidateRefHandle(handle, orderId, pRefCtxt->OwnerHandle)) != ISMRC_OK)
   {
      TRACE(1, "Failed to update a reference, because the handle is not valid. Handle 0x%lx, OrderId %lu, OwnerHandle 0x%lx, MinActiveOrderId %lu\n",
            handle, orderId, pRefCtxt->OwnerHandle, minimumActiveOrderId);
      return rc;
   }

   do
   {
      pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];
      if ((rc = ism_store_memEnsureStoreTransAllocation(pStream, &pDescriptor)) != ISMRC_OK)
      {
         break;
      }

      if (ismSTORE_EXTRACT_GENID(handle) != pStream->MyGenId)
      {
         if ((rc = ism_store_memEnsureRefStateAllocation(pStream, pRefCtxt, orderId, &handle, 0)) != ISMRC_OK)
         {
            break;
         }
         fRefState = 1;
      }

      pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);

      // Add this operation to the store-transaction
      pOp = &pTran->Operations[pTran->OperationCount];
      pOp->OperationType = (fRefState ? Operation_UpdateRefState : Operation_UpdateReference);
      pOp->UpdateReference.Handle = handle;
      pOp->UpdateReference.State = state;
      pTran->OperationCount++;

      // To minimize the number of WRITE_BACK operations, we do WRITE_BACK only
      // when the transaction granule becomes full or at the end of the store-transaction.
      //ADR_WRITE_BACK(pOp, sizeof(*pOp));
      //ADR_WRITE_BACK(pTran, sizeof(*pTran));

      if (minimumActiveOrderId >= pRefCtxt->NextPruneOrderId)
      {
         ism_store_memPruneReferenceAllocation(pStream,
                                               pRefCtxt,
                                               minimumActiveOrderId);

      }

      if (fAutoCommit)
      {
         rc = ism_store_memEndStoreTransaction(hStream, 2, NULL, NULL);
      }
   } while (0);

   return rc;
}

int32_t ism_store_memDeleteReference(ismStore_StreamHandle_t hStream,
                                     void *hRefCtxt,
                                     ismStore_Handle_t handle,
                                     uint64_t orderId,
                                     uint64_t minimumActiveOrderId,
                                     uint8_t fAutoCommit)
{
   ismStore_memStream_t *pStream;
   ismStore_memReferenceContext_t *pRefCtxt = (ismStore_memReferenceContext_t *)hRefCtxt;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_memStoreOperation_t *pOp;
   uint8_t fRefState = 0;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
      TRACE(1, "Failed to delete a reference, because the stream is not valid\n");
      return rc;
   }

   if ((rc = ism_store_memValidateRefCtxt(pRefCtxt)) != ISMRC_OK)
   {
      TRACE(1, "Failed to delete a reference, because the reference context is not valid\n");
      return rc;
   }

   if ((rc = ism_store_memValidateRefHandle(handle, orderId, pRefCtxt->OwnerHandle)) != ISMRC_OK)
   {
      TRACE(1, "Failed to delete a reference, because the handle is not valid. Handle 0x%lx, OrderId %lu, OwnerHandle 0x%lx, MinActiveOrderId %lu\n",
            handle, orderId, pRefCtxt->OwnerHandle, minimumActiveOrderId);
      return rc;
   }

   do
   {
      pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];
      if ((rc = ism_store_memEnsureStoreTransAllocation(pStream, &pDescriptor)) != ISMRC_OK)
      {
         break;
      }

      if (ismSTORE_EXTRACT_GENID(handle) != pStream->MyGenId)
      {
         if ((rc = ism_store_memEnsureRefStateAllocation(pStream, pRefCtxt, orderId, &handle, 1)) != ISMRC_OK)
         {
            break;
         }
         fRefState = 1;
      }

      pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);

      // Add this operation to the store-transaction
      pOp = &pTran->Operations[pTran->OperationCount];
      if (fRefState)
      {
         pOp->OperationType = Operation_UpdateRefState;
         pOp->UpdateReference.Handle = handle;
         pOp->UpdateReference.State = ismSTORE_REFSTATE_DELETED;
      }
      else
      {
         ismStore_memRefGen_t *pRefGen;
         pRefGen = pRefCtxt->pInMemRefGen[pStream->MyGenIndex];
         if (pRefGen && ismSTORE_EXTRACT_GENID(pRefGen->hReferenceHead) == pStream->MyGenId)
         {
           pRefGen->numRefs -= 100 ;
         }
         pOp->OperationType = Operation_DeleteReference;
         pOp->DeleteReference.Handle = handle;
      }
      pTran->OperationCount++;

      // To minimize the number of WRITE_BACK operations, we do WRITE_BACK only
      // when the transaction granule becomes full or at the end of the store-transaction.
      //ADR_WRITE_BACK(pOp, sizeof(*pOp));
      //ADR_WRITE_BACK(pTran, sizeof(*pTran));

      if (minimumActiveOrderId >= pRefCtxt->NextPruneOrderId)
      {
         // Prune references using the minimumActiveOrderId
         ism_store_memPruneReferenceAllocation(pStream,
                                               pRefCtxt,
                                               minimumActiveOrderId);
      }

      if (fAutoCommit)
      {
         rc = ism_store_memEndStoreTransaction(hStream, 2, NULL, NULL);
      }
   } while (0);

   return rc;
}

// This is not transactional by design
static int32_t ism_store_memEnsureRefStateAllocation(ismStore_memStream_t *pStream,
                                                     ismStore_memReferenceContext_t *pRefCtxt,
                                                     uint64_t orderId,
                                                     ismStore_Handle_t *pHandle, uint8_t fDel)
{
  #if USE_REFSTATS
    ismStore_memRefStats_t *pRS ; 
  #endif
   ismStore_memGeneration_t *pGen;
   ismStore_memDescriptor_t *pOwnerDescriptor, *pDescriptor = NULL, *pDescPrev, *pDescNext;
   ismStore_memSplitItem_t *pSplit;
   ismStore_memRefGen_t *pRefGenState;
   ismStore_memRefStateChunk_t *pRefStateChunk = NULL;
   uint8_t *pRefState;
   ismStore_Handle_t hChunk = ismSTORE_NULL_HANDLE;
   uint64_t chunkBaseId;
   int32_t rc = ISMRC_OK;

   *pHandle = ismSTORE_NULL_HANDLE;
   pGen = &ismStore_memGlobal.MgmtGen;

   ismSTORE_MUTEX_LOCK(pRefCtxt->pMutex);

   pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pRefCtxt->OwnerHandle);
   pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);

   if (orderId < pSplit->MinActiveOrderId)
   {
      rc = ISMRC_ArgNotValid;
      ism_common_setErrorData(rc, "%s", "orderId");
      ismSTORE_MUTEX_UNLOCK(pRefCtxt->pMutex);
      return rc;
   }

   if ((pRefGenState = pRefCtxt->pRefGenState) == NULL)
   {
      if ((pRefGenState = ism_store_memAllocateRefGen(pRefCtxt)) == NULL)
      {
         rc = ISMRC_AllocateError;
         ismSTORE_MUTEX_UNLOCK(pRefCtxt->pMutex);
         return rc;
      }
      pRefCtxt->RFChunksInUseLWM = 0;
      pRefCtxt->RFChunksInUseHWM = ismSTORE_REFSTATES_CHUNKS;
      pRefCtxt->pRefGenState = pRefGenState;
   }

   chunkBaseId = ismSTORE_ROUND(orderId, pGen->MaxRefStatesPerGranule);

  #if USE_REFSTATS
    pRS = pRefCtxt->pRefGenPool->RefStats+1 ; 
  #endif
   do
   {
     #if USE_REFSTATS
       pRS->nCL++ ; 
     #endif
      // See whether a refernce chunk should be added at the head of linked-list
      if (ismSTORE_EXTRACT_OFFSET(pRefGenState->hReferenceHead) == 0 || orderId < pRefGenState->LowestOrderId)
      {
        #if USE_REFSTATS
          pRS->nBH++ ; 
        #endif
         TRACE(9, "A reference state chunk for owner (Handle 0x%lx, DataType 0x%x, Version %u) is missing at the head. OrderId %lu, ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
               pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, orderId, chunkBaseId, pRefGenState->LowestOrderId, pRefGenState->HighestOrderId);

         // Allocate a new chunk and then link it in at the head
         rc = ism_store_memAssignRefStateAllocation(pStream,
                                                    pRefCtxt,
                                                    chunkBaseId,
                                                    &hChunk,
                                                    &pDescriptor);

         if (rc == ISMRC_OK)
         {
            // Add the new chunk to the head of the list
            pDescriptor->NextHandle = pRefGenState->hReferenceHead;
            ADR_WRITE_BACK(&pDescriptor->NextHandle, sizeof(pDescriptor->NextHandle));

            if (ismSTORE_EXTRACT_OFFSET(pRefGenState->hReferenceHead) == 0)
            {
               pRefGenState->hReferenceTail = hChunk;
               pRefGenState->HighestOrderId = orderId;
            }
            else
            if ( pRefCtxt->pRFFingers && pRefCtxt->pRFFingers->NumInUse && pRefCtxt->pRFFingers->Handles[0] == pRefGenState->hReferenceHead )
            {
              pRefCtxt->pRFFingers->BaseOid[0] = chunkBaseId;
              pRefCtxt->pRFFingers->Handles[0] = hChunk;
            }
            pRefGenState->hReferenceHead = hChunk;
            pRefGenState->LowestOrderId = chunkBaseId;
            pRefStateChunk = (ismStore_memRefStateChunk_t *)(pDescriptor + 1);

            if (++pRefCtxt->RFChunksInUse >= pRefCtxt->RFChunksInUseHWM)
            {
               pRefCtxt->RFChunksInUseLWM = pRefCtxt->RFChunksInUse / 2;
               pRefCtxt->RFChunksInUseHWM += ismSTORE_REFSTATES_CHUNKS;
               TRACE(5, "The owner (Handle 0x%lx, DataType 0x%x, Version %u) has a lot of RefState chunks (LowestOrderId %lu, HighestOrderId %lu, MinActiveOrderId %lu, ChunksInUse %u, ChunksInUseLWM %u, ChunksInUseHWM %u)\n",
                     pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, pRefGenState->LowestOrderId,
                     pRefGenState->HighestOrderId, pSplit->MinActiveOrderId, pRefCtxt->RFChunksInUse,
                     pRefCtxt->RFChunksInUseLWM, pRefCtxt->RFChunksInUseHWM);
            }
            ism_store_memBuildRFFingers(pRefCtxt) ; 

            TRACE(9, "Add a reference state chunk (Handle 0x%lx, DataType 0x%x, NextHandle 0x%lx) for owner (Handle 0x%lx, DataType 0x%x, Version %u). ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu, ChunksInUse %u\n",
                  hChunk, pDescriptor->DataType, pDescriptor->NextHandle, pRefCtxt->OwnerHandle,
                  pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, chunkBaseId, pRefGenState->LowestOrderId,
                  pRefGenState->HighestOrderId, pRefCtxt->RFChunksInUse);
         }
         else
         {
            TRACE(1, "Failed to allocate a reference state chunk (OrderId %lu) for owner (Handle 0x%lx, DataType 0x%x, Version %u). ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu, ChunksInUse %u\n",
                  orderId, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, chunkBaseId, pRefGenState->LowestOrderId, pRefGenState->HighestOrderId, pRefCtxt->RFChunksInUse);
            pDescriptor = NULL;
         }
         break;
      }

      // See whether the order Id exists in the last reference state chunk
      hChunk = pRefGenState->hReferenceTail;
      pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hChunk);
      pRefStateChunk = (ismStore_memRefStateChunk_t *)(pDescriptor + 1);
      if (orderId >= pRefStateChunk->BaseOrderId && orderId < pRefStateChunk->BaseOrderId + pRefStateChunk->RefStateCount)
      {
        #if USE_REFSTATS
          pRS->nTL++ ; 
        #endif
         if (orderId > pRefGenState->HighestOrderId)
         {
            pRefGenState->HighestOrderId = orderId;
         }
         break;
      }

      // See whether a refernce state chunk should be added at the tail of linked-list
      if (orderId >= pRefStateChunk->BaseOrderId + pRefStateChunk->RefStateCount)
      {
        #if USE_REFSTATS
          pRS->nAT++ ; 
        #endif
         TRACE(9, "A reference state chunk for owner (Handle 0x%lx, DataType 0x%x, Version %u) is missing at the tail. OrderId %lu, ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
               pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, orderId, chunkBaseId, pRefGenState->LowestOrderId, pRefGenState->HighestOrderId);

         // Allocate a new chunk and then link it in at the tail
         rc = ism_store_memAssignRefStateAllocation(pStream,
                                                    pRefCtxt,
                                                    chunkBaseId,
                                                    &hChunk,
                                                    &pDescriptor);

         if (rc == ISMRC_OK)
         {
            // Add the new chunk to the tail of the list
            pDescPrev = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pRefGenState->hReferenceTail);
            pDescPrev->NextHandle = hChunk;
            ADR_WRITE_BACK(&pDescPrev->NextHandle, sizeof(pDescPrev->NextHandle));
            pRefGenState->hReferenceTail = hChunk;
            pRefGenState->HighestOrderId = orderId;
            pRefStateChunk = (ismStore_memRefStateChunk_t *)(pDescriptor + 1);

            if (++pRefCtxt->RFChunksInUse >= pRefCtxt->RFChunksInUseHWM)
            {
               pRefCtxt->RFChunksInUseLWM = pRefCtxt->RFChunksInUse / 2;
               pRefCtxt->RFChunksInUseHWM += ismSTORE_REFSTATES_CHUNKS;
               TRACE(5, "The owner (Handle 0x%lx, DataType 0x%x, Version %u) has a lot of RefState chunks (LowestOrderId %lu, HighestOrderId %lu, MinActiveOrderId %lu, ChunksInUse %u, ChunksInUseLWM %u, ChunksInUseHWM %u)\n",
                     pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, pRefGenState->LowestOrderId,
                     pRefGenState->HighestOrderId, pSplit->MinActiveOrderId, pRefCtxt->RFChunksInUse,
                     pRefCtxt->RFChunksInUseLWM, pRefCtxt->RFChunksInUseHWM);
            }
            if (pRefCtxt->pRFFingers )
            {
              ismStore_memRefStateFingers_t *pF = pRefCtxt->pRFFingers;
              if ( pF->NumAtEnd < pF->ChunkGap )
                pF->NumAtEnd++ ; 
              else
              if ( pF->NumInUse < pF->NumInArray )
              {
                pF->BaseOid[pF->NumInUse] = chunkBaseId ; 
                pF->Handles[pF->NumInUse] = hChunk ; 
                pF->NumInUse++ ; 
                pF->NumAtEnd = 1 ; 
              }
              else
                ism_store_memBuildRFFingers(pRefCtxt) ; 
            }
            else
              ism_store_memBuildRFFingers(pRefCtxt) ; 

            TRACE(9, "Add a reference state chunk (Handle 0x%lx, DataType 0x%x, NextHandle 0x%lx) for owner (Handle 0x%lx, DataType 0x%x, Version %u). ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu, ChunksInUse %u\n",
                  hChunk, pDescriptor->DataType, pDescriptor->NextHandle, pRefCtxt->OwnerHandle,
                  pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, chunkBaseId, pRefGenState->LowestOrderId,
                  pRefGenState->HighestOrderId, pRefCtxt->RFChunksInUse);
         }
         else
         {
            TRACE(1, "Failed to allocate a reference state chunk (OrderId %lu) for owner (Handle 0x%lx, DataType 0x%x, Version %u). ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu, ChunksInUse %u\n",
                  orderId, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, chunkBaseId, pRefGenState->LowestOrderId, pRefGenState->HighestOrderId, pRefCtxt->RFChunksInUse);
            pDescriptor = NULL;
         }
         break;
      }

      // Search for the reference state chunk that contains the required order Id or create a new chunk
      hChunk = pRefGenState->hReferenceHead;
      // See whether we can use the local cache (hRFCacheChunk)
      if (pRefCtxt->hRFCacheChunk[fDel] != ismSTORE_NULL_HANDLE)
      {
         pDescNext = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pRefCtxt->hRFCacheChunk[fDel]);
         pRefStateChunk = (ismStore_memRefStateChunk_t *)(pDescNext + 1);
         if (pDescNext->DataType == ismSTORE_DATATYPE_REFSTATES &&
             pRefStateChunk->OwnerHandle == pRefCtxt->OwnerHandle &&
             pRefStateChunk->OwnerVersion == pRefCtxt->OwnerVersion &&
             orderId >= pRefStateChunk->BaseOrderId)
         {
            hChunk = pRefCtxt->hRFCacheChunk[fDel];
            if (orderId < pRefStateChunk->BaseOrderId + pRefStateChunk->RefStateCount)
            {
              #if USE_REFSTATS
                pRS->nAC++ ; 
              #endif
               // We found the the required chunk
               pDescriptor = pDescNext;
               break;
            }
         }
         #if USE_REFSTATS
         else
           pRS->b[0]++ ; 
         #endif
      }
      #if USE_REFSTATS
      else
        pRS->b[1]++ ; 
      #endif

      // See whether we can use the local cache (pRFFingers)
      if (pRefCtxt->pRFFingers && pRefCtxt->pRFFingers->NumInUse)
      {
         ismStore_memRefStateFingers_t *pF = pRefCtxt->pRFFingers ; 
         int i = su_findGLB(orderId, 0, pF->NumInUse-1, pF->BaseOid) ; 
         if ( i >= 0 )
         {
           uint64_t Oid=0 ; 
           if ( hChunk != pRefGenState->hReferenceHead )
             Oid = pRefStateChunk->BaseOrderId ; 
           pDescNext = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pF->Handles[i]);
           if (!pDescNext )
           {
             TRACE(1,"!!! should not be here !!! RefStateFingers: i=%u, pF->Handles[i]= %p, NumInUse=%u, NumInArray=%u\n",
                   i,(void *)pF->Handles[i],pF->NumInUse,pF->NumInArray);
           }
           else
           {
             pRefStateChunk = (ismStore_memRefStateChunk_t *)(pDescNext + 1);
             if (pDescNext->DataType == ismSTORE_DATATYPE_REFSTATES &&
                 pRefStateChunk->OwnerHandle == pRefCtxt->OwnerHandle &&
                 pRefStateChunk->OwnerVersion == pRefCtxt->OwnerVersion &&
                 orderId >= pRefStateChunk->BaseOrderId )
             {
                if ( Oid     <  pRefStateChunk->BaseOrderId )
                {
                  hChunk = pF->Handles[i];
                  if (orderId < pRefStateChunk->BaseOrderId + pRefStateChunk->RefStateCount)
                  {
                    #if USE_REFSTATS
                      pRS->nAF++ ; 
                    #endif
                     // We found the the required chunk
                     pDescriptor = pDescNext;
                     break;
                  }
                  #if USE_REFSTATS
                  else
                    pRS->nUF++ ; 
                  #endif
                }
                #if USE_REFSTATS
                else
                  pRS->b[2]++ ; 
                #endif
             }
             #if USE_REFSTATS
             else
               pRS->b[3]++ ; 
             #endif
           }
         }
         #if USE_REFSTATS
         else
           pRS->b[4]++ ; 
         #endif
      }
      #if USE_REFSTATS
      else
        pRS->b[5]++ ; 
      #endif

     #if USE_REFSTATS
       pRS->nLL++ ; 
     #endif
      for (pDescPrev = pDescNext = NULL; ismSTORE_EXTRACT_OFFSET(hChunk) != 0; hChunk = pDescPrev->NextHandle)
      {
        #if USE_REFSTATS
          pRS->nEL++ ; 
        #endif
         pDescNext = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hChunk);

         // Sanity check: make sure that the RefStates chunk is valid
         if (pDescNext->DataType != ismSTORE_DATATYPE_REFSTATES)
         {
            TRACE(1, "The reference state chunk (Handle 0x%lx, DataType 0x%x) of the owner (Handle 0x%lx, DataType 0x%x, Version %u) is not valid. OrderId %lu, ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
                  hChunk, pDescNext->DataType, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, orderId, chunkBaseId, pRefGenState->LowestOrderId, pRefGenState->HighestOrderId);
            hChunk = ismSTORE_NULL_HANDLE;
            break;
         }

         pRefStateChunk = (ismStore_memRefStateChunk_t *)(pDescNext + 1);
         if (orderId >= pRefStateChunk->BaseOrderId && orderId < pRefStateChunk->BaseOrderId + pRefStateChunk->RefStateCount)
         {
            // We found the the required chunk
            pDescriptor = pDescNext;
            break;
         }

         if (orderId < pRefStateChunk->BaseOrderId)
         {
            if (pDescPrev == NULL)
            {
               TRACE(9, "The reference state chunk list of owner (Handle 0x%lx, DataType 0x%x, Version %u) is not valid. OrderId %lu, ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
                     pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, orderId, chunkBaseId, pRefGenState->LowestOrderId, pRefGenState->HighestOrderId);
               rc = ISMRC_Error;
               break;
            }

            // We need to create a new chunk for the specified order Id
            TRACE(9, "A reference state chunk for owner (Handle 0x%lx, DataType 0x%x, Version %u) is missing at the middle. OrderId %lu, ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
                  pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, orderId, chunkBaseId, pRefGenState->LowestOrderId, pRefGenState->HighestOrderId);

            // Allocate a new chunk and then link it in
            rc = ism_store_memAssignRefStateAllocation(pStream,
                                                       pRefCtxt,
                                                       chunkBaseId,
                                                       &hChunk,
                                                       &pDescriptor);

            if (rc == ISMRC_OK)
            {
               pDescriptor->NextHandle = pDescPrev->NextHandle;
               ADR_WRITE_BACK(&pDescriptor->NextHandle, sizeof(pDescriptor->NextHandle));

               pDescPrev->NextHandle = hChunk;
               ADR_WRITE_BACK(&pDescPrev->NextHandle, sizeof(pDescPrev->NextHandle));

               if (++pRefCtxt->RFChunksInUse >= pRefCtxt->RFChunksInUseHWM)
               {
                  TRACE(5, "The owner (Handle 0x%lx, DataType 0x%x, Version %u) has a lot of RefState chunks (LowestOrderId %lu, HighestOrderId %lu, MinActiveOrderId %lu, ChunksInUse %u, ChunksInUseLWM %u, ChunksInUseHWM %u)\n",
                        pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, pRefGenState->LowestOrderId,
                        pRefGenState->HighestOrderId, pSplit->MinActiveOrderId, pRefCtxt->RFChunksInUse, pRefCtxt->RFChunksInUseLWM, pRefCtxt->RFChunksInUseHWM);
                  pRefCtxt->RFChunksInUseLWM = pRefCtxt->RFChunksInUse / 2;
                  pRefCtxt->RFChunksInUseHWM += ismSTORE_REFSTATES_CHUNKS;
               }
               pRefStateChunk = (ismStore_memRefStateChunk_t *)(pDescriptor + 1);
               ism_store_memBuildRFFingers(pRefCtxt) ; 

               TRACE(9, "Add a reference state chunk (Handle 0x%lx, DataType 0x%x, NextHandle 0x%lx) for owner (Handle 0x%lx, DataType 0x%x, Version %u). ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu, ChunksInUse %u\n",
                     hChunk, pDescriptor->DataType, pDescriptor->NextHandle, pRefCtxt->OwnerHandle,
                     pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion, chunkBaseId, pRefGenState->LowestOrderId,
                     pRefGenState->HighestOrderId, pRefCtxt->RFChunksInUse);
            }
            break;
         }
         pDescPrev = pDescNext;
      }
   } while (0);

   if (ismSTORE_EXTRACT_OFFSET(hChunk) == 0 || pDescriptor == NULL || pRefStateChunk == NULL)
   {
      TRACE(1, "Failed to add a reference state chunk (OrderId %lu, BaseOrderId %lu) to owner (Handle 0x%lx, DataType 0x%x, Version %u, MinActiveOrderId %lu). LowestOrderId %lu, HighestOrderId %lu, ChunksInUse %u\n",
            orderId, chunkBaseId, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion,
            pSplit->MinActiveOrderId, pRefGenState->LowestOrderId,
            pRefGenState->HighestOrderId, pRefCtxt->RFChunksInUse);
      rc = ISMRC_Error;
      goto exit;
   }

   // Sanity check: verify that the chunk is correct
   if (pRefStateChunk->BaseOrderId != chunkBaseId)
   {
      TRACE(1, "Failed to add a reference state chunk (OrderId %lu, BaseOrderId %lu) to owner (Handle 0x%lx, DataType 0x%x, OwnerVersion %u) because the reference chunk is not valid (ChunkBaseOrderId %lu, OwnerHandle 0x%lx, OwnerVersion %u, LowestOrderId %lu, HighestOrderId %lu). LowestOrderId %lu, HighestOrderId %lu, ChunksInUse %u\n",
            orderId, chunkBaseId, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, pRefCtxt->OwnerVersion,
            pRefStateChunk->BaseOrderId, pRefStateChunk->OwnerHandle, pRefStateChunk->OwnerVersion,
            pRefGenState->LowestOrderId, pRefGenState->HighestOrderId, pRefGenState->LowestOrderId,
            pRefGenState->HighestOrderId, pRefCtxt->RFChunksInUse);
      rc = ISMRC_Error;
      goto exit;
   }

   pRefState = &pRefStateChunk->RefStates[orderId - pRefStateChunk->BaseOrderId];
   pRefCtxt->hRFCacheChunk[fDel] = hChunk;
   *pHandle = (ismStore_Handle_t)(hChunk + ((char *)pRefState - (char *)pDescriptor));

   ismSTORE_MUTEX_UNLOCK(pRefCtxt->pMutex);
   return rc;

exit:
   // We didn't find a valid reference state chunk. It means that something wrong happended, so
   // we will try to print as much information as possible.
   for (hChunk = pRefGenState->hReferenceHead; ismSTORE_EXTRACT_OFFSET(hChunk) != 0; hChunk = pDescNext->NextHandle)
   {
      pDescNext = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hChunk);
      pRefStateChunk = (ismStore_memRefStateChunk_t *)(pDescNext + 1);
      TRACE(1, "RefStatesChunk: Handle 0x%lx, DataType 0x%x, Owner 0x%lx, BaseOrderId %lu, RefStateCount %u, NextHandle 0x%lx\n",
            hChunk, pDescNext->DataType, pRefStateChunk->OwnerHandle, pRefStateChunk->BaseOrderId, pRefStateChunk->RefStateCount, pDescNext->NextHandle);

      // Sanity check: make sure that the RefStates chunk is valid
      if (pDescNext->DataType != ismSTORE_DATATYPE_REFSTATES)
      {
         TRACE(1, "The reference state chunk (Handle 0x%lx, DataType 0x%x) of the owner (Handle 0x%lx, DataType 0x%x) is not valid. OrderId %lu, ChunkBaseOrderId %lu, LowestOrderId %lu, HighestOrderId %lu\n",
               hChunk, pDescNext->DataType, pRefCtxt->OwnerHandle, pOwnerDescriptor->DataType, orderId, chunkBaseId, pRefGenState->LowestOrderId, pRefGenState->HighestOrderId);
         break;
      }
   }

   ismSTORE_MUTEX_UNLOCK(pRefCtxt->pMutex);
   return rc;
}

// This is not transactional by design
static int32_t ism_store_memAssignRefStateAllocation(ismStore_memStream_t *pStream,
                                                     ismStore_memReferenceContext_t *pRefCtxt,
                                                     uint64_t baseOrderId,
                                                     ismStore_Handle_t *pHandle,
                                                     ismStore_memDescriptor_t **pDesc)
{
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memRefStateChunk_t *pRefStateChunk;
   ismStore_Handle_t handle;
   int32_t rc = ISMRC_OK;

   rc = ism_store_memGetMgmtPoolElements(NULL,
                                         ismSTORE_MGMT_SMALL_POOL_INDEX,
                                         ismSTORE_DATATYPE_REFSTATES,
                                         0, 0, ismSTORE_SINGLE_GRANULE,
                                         &handle, &pDescriptor);
   if (rc != ISMRC_OK)
   {
      return rc;
   }

   // Initialize the data
   pRefStateChunk = (ismStore_memRefStateChunk_t *)(pDescriptor + 1);
   pRefStateChunk->BaseOrderId = baseOrderId;
   pRefStateChunk->RefStateCount = ismStore_memGlobal.MgmtGen.MaxRefStatesPerGranule;
   pRefStateChunk->OwnerHandle = pRefCtxt->OwnerHandle;
   pRefStateChunk->OwnerVersion = pRefCtxt->OwnerVersion;
   memset(pRefStateChunk->RefStates, ismSTORE_REFSTATE_NOT_VALID, pRefStateChunk->RefStateCount * sizeof(uint8_t));

   pDescriptor->DataType = ismSTORE_DATATYPE_REFSTATES;

   ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor) + pDescriptor->DataLength);

   *pHandle = handle;
   *pDesc = pDescriptor;
   return rc;
}

/*
 * This method assumes that the application handles references properly.
 * That is, the application does not try to prune the reference chunk (using minimumActiveOrderId)
 * before the commit operation is completed (we do not check here if the chunk still exist).
 */
static inline int32_t ism_store_memCreateReference_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp)
{
   ismStore_memReference_t *pRef;

   pRef = (ismStore_memReference_t *)ism_store_memMapHandleToAddress(pOp->CreateReference.Handle);
   pRef->RefHandle = pOp->CreateReference.RefHandle;
   pRef->Value = pOp->CreateReference.Value;
   pRef->State = pOp->CreateReference.State;

   ADR_WRITE_BACK(pRef, sizeof(*pRef));

   return ISMRC_OK;
}

static inline int32_t ism_store_memCreateReference_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp)
{
   ismStore_memReference_t *pRef;

   if (pOp->OperationType != Operation_CreateReference)
   {
      TRACE(1, "Failed to rollback a store-transaction, because the operation type (%d) is not valid. Valid value %d\n", pOp->OperationType, Operation_CreateReference);
      return ISMRC_Error;
   }
   pRef = (ismStore_memReference_t *)ism_store_memMapHandleToAddress(pOp->CreateReference.Handle);
   memset(pRef, '\0', sizeof(*pRef));
//   __sync_fetch_and_and(&pRef->Flag,0);
   ADR_WRITE_BACK(pRef, sizeof(*pRef));

   return ISMRC_OK;
}

/*
 * This method assumes that the application handles references properly.
 * That is, the application does not try to prune the reference chunk (using minimumActiveOrderId)
 * before the commit operation is completed (we do not check here if the chunk still exist).
 */
static inline int32_t ism_store_memDeleteReference_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp)
{
   ismStore_memReference_t *pRef;

   pRef = (ismStore_memReference_t *)ism_store_memMapHandleToAddress(pOp->DeleteReference.Handle);
   memset(pRef, '\0', sizeof(*pRef));
//   __sync_fetch_and_and(&pRef->Flag,0);
   ADR_WRITE_BACK(pRef, sizeof(*pRef));

   return ISMRC_OK;
}

static inline int32_t ism_store_memDeleteReference_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp)
{
   if (pOp->OperationType != Operation_DeleteReference)
   {
      TRACE(1, "Failed to rollback a store-transaction, because the operation type (%d) is not valid. Valid value %d\n", pOp->OperationType, Operation_DeleteReference);
      return ISMRC_Error;
   }

   return ISMRC_OK;
}

/*
 * This method assumes that the application handles references properly.
 * That is, the application does not try to prune the reference chunk (using minimumActiveOrderId)
 * before the commit operation is completed (we do not check here if the chunk still exist).
 */
static inline int32_t ism_store_memUpdateReference_Commit(
        ismStore_memStream_t *pStream,
        ismStore_memStoreTransaction_t *pTran,
        ismStore_memStoreOperation_t *pOp)
{
   ismStore_memReference_t *pRef;

   pRef = (ismStore_memReference_t *)ism_store_memMapHandleToAddress(pOp->UpdateReference.Handle);
   pRef->State = pOp->UpdateReference.State;
   ADR_WRITE_BACK(pRef, sizeof(*pRef));

   return ISMRC_OK;
}

static inline int32_t ism_store_memUpdateReference_Rollback(
        ismStore_memStream_t *pStream,
        ismStore_memStoreTransaction_t *pTran,
        ismStore_memStoreOperation_t *pOp)
{
   if (pOp->OperationType != Operation_UpdateReference)
   {
      TRACE(1, "Failed to rollback a store-transaction, because the operation type (%d) is not valid. Valid value %d\n", pOp->OperationType, Operation_UpdateReference);
      return ISMRC_Error;
   }

   return ISMRC_OK;
}

/*
 * This method assumes that the application handles references properly.
 * That is, the application does not try to prune the reference chunk (using minimumActiveOrderId)
 * before the commit operation is completed (we do not check here if the chunk still exist).
 */
static inline int32_t ism_store_memUpdateRefState_Commit(
        ismStore_memStream_t *pStream,
        ismStore_memStoreTransaction_t *pTran,
        ismStore_memStoreOperation_t *pOp)
{
   uint8_t *pRefState;

   pRefState = (uint8_t *)ism_store_memMapHandleToAddress(pOp->UpdateReference.Handle);
   *pRefState = pOp->UpdateReference.State;
   ADR_WRITE_BACK(pRefState, sizeof(*pRefState));

   return ISMRC_OK;
}

static inline int32_t ism_store_memUpdateRefState_Rollback(
        ismStore_memStream_t *pStream,
        ismStore_memStoreTransaction_t *pTran,
        ismStore_memStoreOperation_t *pOp)
{
   if (pOp->OperationType != Operation_UpdateRefState)
   {
      TRACE(1, "Failed to rollback a store-transaction, because the operation type (%d) is not valid. Valid value %d\n", pOp->OperationType, Operation_UpdateRefState);
      return ISMRC_Error;
   }

   return ISMRC_OK;
}

int32_t ism_store_memAddState(ismStore_StreamHandle_t hStream,
                              void *hStateCtxt,
                              const ismStore_StateObject_t *pState,
                              ismStore_Handle_t *pHandle)
{
   ismStore_memStream_t *pStream;
   ismStore_memStateContext_t *pStateCtxt = (ismStore_memStateContext_t *)hStateCtxt;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_memStoreOperation_t *pOp;
   ismStore_Handle_t handle;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
      TRACE(1, "Failed to add a state object, because the stream is not valid\n");
      return rc;
   }

   if ((rc = ism_store_memValidateStateCtxt(pStateCtxt)) != ISMRC_OK)
   {
      TRACE(1, "Failed to add a state object, because the state context is not valid\n");
      return rc;
   }

   pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];

   rc = ism_store_memEnsureStoreTransAllocation(pStream, &pDescriptor);
   if (rc == ISMRC_OK)
   {
      pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);

      rc = ism_store_memEnsureStateAllocation(pStream, pStateCtxt, pState, &handle);
      if (rc == ISMRC_OK)
      {
       // Add this operation to the store-transaction
       pOp = &pTran->Operations[pTran->OperationCount];
       pOp->OperationType = Operation_CreateState;
       pOp->CreateState.Handle = handle;
       pTran->OperationCount++;

       // To minimize the number of WRITE_BACK operations, we do WRITE_BACK only
       // when the transaction granule becomes full or at the end of the store-transaction.
       //ADR_WRITE_BACK(pOp, sizeof(*pOp));
       //ADR_WRITE_BACK(pTran, sizeof(*pTran));
       *pHandle = handle;
      }
   }

   return rc;
}

// This is not transactional by design
static int32_t ism_store_memEnsureStateAllocation(ismStore_memStream_t *pStream,
                                                  ismStore_memStateContext_t *pStateCtxt,
                                                  const ismStore_StateObject_t *pState,
                                                  ismStore_Handle_t *pHandle)
{
   int i;
   ismStore_memDescriptor_t *pDescriptor, *pTailDescriptor=NULL;
   ismStore_memStateChunk_t *pStateChunk;
   ismStore_memState_t *pStateObject;
   ismStore_Handle_t hChunk;
   int32_t rc = ISMRC_OK, idx;

   ismSTORE_MUTEX_LOCK(pStateCtxt->pMutex);

   // First ensure that we have a space for the new state
   if ((hChunk = pStateCtxt->hStateHead))
   {
      pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hChunk);
      pStateChunk = (ismStore_memStateChunk_t *)(pDescriptor + 1);
      while (pDescriptor && pStateChunk->StatesCount >= ismStore_memGlobal.MgmtGen.MaxStatesPerGranule)
      {
         pTailDescriptor = pDescriptor;
         if ((hChunk = pDescriptor->NextHandle))
         {
            pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hChunk);
            pStateChunk = (ismStore_memStateChunk_t *)(pDescriptor + 1);
         }
         else
         {
            pDescriptor = NULL;
         }
      }
   }
   else
   {
      pDescriptor = NULL;
   }

   if (pDescriptor == NULL)
   {
      rc = ism_store_memGetMgmtPoolElements(NULL,
                                            ismSTORE_MGMT_POOL_INDEX,
                                            ismSTORE_DATATYPE_STATES,
                                            0, 0, ismSTORE_SINGLE_GRANULE,
                                            &hChunk, &pDescriptor);
      if (rc != ISMRC_OK)
      {
         ismSTORE_MUTEX_UNLOCK(pStateCtxt->pMutex);
         return rc;
      }

      // Initialize the data
      pStateChunk = (ismStore_memStateChunk_t *)(pDescriptor + 1);
      pStateChunk->StatesCount = 0;
      pStateChunk->LastAddedIndex = 0;
      pStateChunk->OwnerHandle = pStateCtxt->OwnerHandle;
      pStateChunk->OwnerVersion = pStateCtxt->OwnerVersion;
      memset(pStateChunk->States, '\0', ismStore_memGlobal.MgmtGen.MaxStatesPerGranule * sizeof(ismStore_memState_t));

      pDescriptor->DataType = ismSTORE_DATATYPE_STATES;
      ADR_WRITE_BACK(pDescriptor, sizeof(*pDescriptor) + pDescriptor->DataLength);

      // Add the new state chunk to the list
      if (pTailDescriptor)
      {
          pTailDescriptor->NextHandle = hChunk;
          ADR_WRITE_BACK(&pTailDescriptor->NextHandle, sizeof(pTailDescriptor->NextHandle));
          TRACE(9, "Add a state chunk (Handle 0x%lx, PrevHandle 0x%lx) for owner (Handle 0x%lx)\n",
                hChunk, (uint64_t)((char *)pTailDescriptor - (char *)ismStore_memGlobal.pStoreBaseAddress), pStateCtxt->OwnerHandle);
      }
      else
      {
         pStateCtxt->hStateHead = hChunk;
         TRACE(9, "Add a state chunk (Handle 0x%lx, PrevHandle NULL) for owner (Handle 0x%lx)\n", hChunk, pStateCtxt->OwnerHandle);
      }
   }

   // Find a free place in the chunk
   for (i=0, idx=pStateChunk->LastAddedIndex; i < ismStore_memGlobal.MgmtGen.MaxStatesPerGranule; i++)
   {
      if (pStateChunk->States[idx].Flag == ismSTORE_STATEFLAG_EMPTY)
      {
         break;
      }
      if (++idx >= ismStore_memGlobal.MgmtGen.MaxStatesPerGranule)
      {
         idx=0;
      }
   }

   // Reserve the free place for the state
   if (i < ismStore_memGlobal.MgmtGen.MaxStatesPerGranule)
   {
      pStateObject = &pStateChunk->States[idx];
      pStateObject->Value = pState->Value;
      pStateObject->Flag = ismSTORE_STATEFLAG_RESERVED;        /* Mark the state object as not committed */
      pStateChunk->StatesCount++;
      pStateChunk->LastAddedIndex = idx;
      // We will call WRITE_BACK for the StateObject on the commit
      //ADR_WRITE_BACK(pStateObject, sizeof(*pStateObject));
      ADR_WRITE_BACK(pStateChunk, sizeof(*pStateChunk));

      *pHandle = (ismStore_Handle_t)(hChunk + ((char *)pStateObject - (char *)pDescriptor));

      TRACE(9, "A state object has been added in the state chunk (Handle 0x%lx, StatesCount %u, Index %u) of owner (Handle 0x%lx)\n",
            hChunk, pStateChunk->StatesCount, idx, pStateCtxt->OwnerHandle);
   }
   else
   {
      // Should not occur
      rc = ISMRC_Error;
      TRACE(1, "Failed to find a place for the new state in the state chunk (Handle 0x%lx, StatesCount %u, LastAddedIndex %u) of owner (Handle 0x%lx)\n",
            hChunk, pStateChunk->StatesCount, pStateChunk->LastAddedIndex, pStateCtxt->OwnerHandle);
   }

   ismSTORE_MUTEX_UNLOCK(pStateCtxt->pMutex);

   return rc;
}

int32_t ism_store_memDeleteState(ismStore_StreamHandle_t hStream,
                                 void *hStateCtxt,
                                 ismStore_Handle_t handle)
{
   ismStore_memStream_t *pStream;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memStateContext_t *pStateCtxt = (ismStore_memStateContext_t *)hStateCtxt;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_memStoreOperation_t *pOp;
   int32_t rc = ISMRC_OK;

   if ((rc = ism_store_memValidateStream(hStream)) != ISMRC_OK)
   {
      TRACE(1, "Failed to delete a state object, because the stream is not valid\n");
    return rc;
   }

   if ((rc = ism_store_memValidateStateCtxt(pStateCtxt)) != ISMRC_OK)
   {
      TRACE(1, "Failed to delete a state object, because the state context is not valid\n");
      return rc;
   }

   pStream = (ismStore_memStream_t *)ismStore_memGlobal.pStreams[hStream];
   rc = ism_store_memEnsureStoreTransAllocation(pStream, &pDescriptor);
   if (rc == ISMRC_OK)
   {
      pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);

      // Add this operation to the store-transaction
      pOp = &pTran->Operations[pTran->OperationCount];
      pOp->OperationType = Operation_DeleteState;
      pOp->DeleteState.Handle = handle;
      pTran->OperationCount++;

      // To minimize the number of WRITE_BACK operations, we do WRITE_BACK only
      // when the transaction granule becomes full or at the end of the store-transaction.
      //ADR_WRITE_BACK(pOp, sizeof(*pOp));
      //ADR_WRITE_BACK(pTran, sizeof(*pTran));
   }

   return rc;
}

static int32_t ism_store_memFreeStateAllocation(ismStore_Handle_t handle)
{
   ismStore_memDescriptor_t *pDescriptor, *pOwnerDescriptor, *pChunkDescriptor;
   ismStore_memStateChunk_t *pStateChunk;
   ismStore_memSplitItem_t *pSplit;
   ismStore_memState_t *pStateObject;
   ismStore_memStateContext_t *pStateCtxt;
   ismStore_GenId_t genId;
   ismStore_Handle_t hChunk, hPrev, offset;

   // Find the owner and the StateContext
   genId = ismSTORE_EXTRACT_GENID(handle);
   offset = ismSTORE_ROUND(ismSTORE_EXTRACT_OFFSET(handle), ismStore_memGlobal.MgmtGranuleSizeBytes);
   hChunk = ismSTORE_BUILD_HANDLE(genId, offset);
   pChunkDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hChunk);

   if (pChunkDescriptor->DataType != ismSTORE_DATATYPE_STATES)
   {
      if (ismStore_memGlobal.State != ismSTORE_STATE_INIT)
      {
         return ISMRC_Error;
      }
      else
      {
         return ISMRC_OK;
      }
   }  

   pStateChunk = (ismStore_memStateChunk_t *)(pChunkDescriptor + 1);
   pOwnerDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pStateChunk->OwnerHandle);
   pSplit = (ismStore_memSplitItem_t *)(pOwnerDescriptor + 1);
   pStateCtxt = (ismStore_memStateContext_t *)pSplit->pStateCtxt;
   ismSTORE_MUTEX_LOCK(pStateCtxt->pMutex);

   pStateObject = (ismStore_memState_t *)ism_store_memMapHandleToAddress(handle);
   memset(pStateObject, '\0', sizeof(*pStateObject));
   ADR_WRITE_BACK(pStateObject, sizeof(*pStateObject));

   // See whether the whole state chunk is empty
   if (--pStateChunk->StatesCount <= 0)
   {
      if ((hPrev = pStateCtxt->hStateHead) == hChunk)
      {
         pStateCtxt->hStateHead = pChunkDescriptor->NextHandle;
         pChunkDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
         TRACE(9, "The state chunk 0x%lx of owner 0x%lx has been released. hStateHead 0x%lx\n", hChunk, pStateChunk->OwnerHandle, pStateCtxt->hStateHead);
         ism_store_memReturnPoolElements(NULL, hChunk, 1);
      }
      else
      {
         while (hPrev)
         {
            pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(hPrev);
            if (pDescriptor->NextHandle == hChunk)
            {
               pDescriptor->NextHandle = pChunkDescriptor->NextHandle;
               ADR_WRITE_BACK(&pDescriptor->NextHandle, sizeof(pDescriptor->NextHandle));
               pChunkDescriptor->NextHandle = ismSTORE_NULL_HANDLE;
               TRACE(9, "The state chunk 0x%lx of owner 0x%lx has been released\n", hChunk, pStateChunk->OwnerHandle);
               ism_store_memReturnPoolElements(NULL, hChunk, 1);
               break;
            }
            hPrev = pDescriptor->NextHandle;
         }
      }
   }
   else
   {
      ADR_WRITE_BACK(&pStateChunk->StatesCount, sizeof(pStateChunk->StatesCount));
   }

   ismSTORE_MUTEX_UNLOCK(pStateCtxt->pMutex);

   return ISMRC_OK;
}

static inline int32_t ism_store_memCreateState_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp)
{
   ismStore_memState_t *pStateObject;

   pStateObject = (ismStore_memState_t *)ism_store_memMapHandleToAddress(pOp->CreateState.Handle);
   pStateObject->Flag = ismSTORE_STATEFLAG_VALID;  /* Mark the state object as valid */
   ADR_WRITE_BACK(pStateObject, sizeof(*pStateObject));

   return ISMRC_OK;
}

static inline int32_t ism_store_memCreateState_Rollback(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp)
{
   int32_t rc;

   if (pOp->OperationType == Operation_CreateState)
   {
      rc = ism_store_memFreeStateAllocation(pOp->CreateState.Handle);
   }
   else
   {
      TRACE(1, "Failed to rollback a store-transaction, because the operation type (%d) is not valid. Valid value %d\n", pOp->OperationType, Operation_CreateState);
      rc = ISMRC_Error;
   }

   return rc;
}

static inline int32_t ism_store_memDeleteState_Commit(
                   ismStore_memStream_t *pStream,
                   ismStore_memStoreTransaction_t *pTran,
                   ismStore_memStoreOperation_t *pOp)
{
   int32_t rc;

   rc = ism_store_memFreeStateAllocation(pOp->DeleteState.Handle);

   return rc;
}

static inline int32_t ism_store_memDeleteState_Rollback(
        ismStore_memStream_t *pStream,
        ismStore_memStoreTransaction_t *pTran,
        ismStore_memStoreOperation_t *pOp)
{
   if (pOp->OperationType != Operation_DeleteState)
   {
      TRACE(1, "Failed to rollback a store-transaction, because the operation type (%d) is not valid. Valid value %d\n", pOp->OperationType, Operation_DeleteState);
      return ISMRC_Error;
   }

   return ISMRC_OK;
}

inline void ism_store_memForceWriteBack(void *address, size_t length)
{
   char *pStart, *pEnd;
   pStart = (char *)((uintptr_t)address & (~0x3fL));
   pEnd = (char *)address + length;

#ifdef __X86_64__
   asm("mfence");
#elif __PPC64__
   __sync_synchronize();
#else
   abort(); //Haven't thought about other architectures at all
#endif
   while (pStart < pEnd)
   {
      // Complete previous writes with memory barrier, then write back the cache line
      //asm("mfence; clflush %0"::"m"(*pStart));
#ifdef __X86_64__
      asm("clflush %0"::"m"(*pStart));
#endif
      pStart += 0x40;
   }
}

static int32_t ism_store_memCommitInternal(ismStore_memStream_t *pStream, ismStore_memDescriptor_t *pDescriptor)
{
   ismStore_Handle_t handle;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_memStoreOperation_t *pOp;
   uint8_t hasMoreChunks;
   int32_t rc = ISMRC_OK, opcount;

   pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);
   if ((pTran->State != ismSTORE_MEM_STORETRANSACTIONSTATE_ACTIVE && pTran->State != ismSTORE_MEM_STORETRANSACTIONSTATE_COMMITTING) ||
       (pTran->GenId <= ismSTORE_MGMT_GEN_ID && pTran->OperationCount > 0))
   {
      handle = ismSTORE_BUILD_HANDLE(ismSTORE_MGMT_GEN_ID, (char *)pDescriptor-ismStore_memGlobal.pStoreBaseAddress);
      TRACE(1, "Failed to commit a store-transaction, because the transaction header is not valid. State %d, GenId %u, OperationCount %u, Handle 0x%lx\n",
            pTran->State, pTran->GenId, pTran->OperationCount, handle);
      return ISMRC_Error;
   }

   hasMoreChunks = (pDescriptor->NextHandle != ismSTORE_NULL_HANDLE && (ismStore_memGlobal.State == ismSTORE_STATE_INIT || pTran->OperationCount >= ismStore_memGlobal.MgmtGen.MaxTranOpsPerGranule));
   pTran->State = ismSTORE_MEM_STORETRANSACTIONSTATE_COMMITTING;
   ADR_WRITE_BACK(&pTran->State, sizeof(pTran->State));

   // Apply the store-operations to the data
   opcount = pTran->OperationCount;
   pOp = pTran->Operations;
   while (opcount-- > 0)
   {
      switch(pOp->OperationType)
      {
         case Operation_CreateReference:
            rc = ism_store_memCreateReference_Commit(pStream, pTran, pOp);
            break;
         case Operation_DeleteReference:
            rc = ism_store_memDeleteReference_Commit(pStream, pTran, pOp);
            break;
         case Operation_UpdateReference:
            rc = ism_store_memUpdateReference_Commit(pStream, pTran, pOp);
            break;
         case Operation_CreateRecord:
            rc = ism_store_memAssignRecordAllocation_Commit(pStream, pTran, pOp);
            break;
         case Operation_DeleteRecord:
            rc = ism_store_memFreeRecordAllocation_Commit(pStream, pTran, pOp);
            break;
         case Operation_UpdateRecord:
            rc = ism_store_memUpdate_Commit(pStream, pTran, pOp);
            break;
         case Operation_UpdateRecordAttr:
            rc = ism_store_memUpdateAttribute_Commit(pStream, pTran, pOp);
            break;
         case Operation_UpdateRecordState:
            rc = ism_store_memUpdateState_Commit(pStream, pTran, pOp);
            break;
         case Operation_UpdateRefState:
            rc = ism_store_memUpdateRefState_Commit(pStream, pTran, pOp);
            break;
         case Operation_CreateState:
            rc = ism_store_memCreateState_Commit(pStream, pTran, pOp);
            break;
         case Operation_DeleteState:
            rc = ism_store_memDeleteState_Commit(pStream, pTran, pOp);
            break;
         case Operation_Null:
         case Operation_UpdateActiveOid:
         case Operation_AllocateGranulesMap:
         case Operation_CreateGranulesMap:
         case Operation_SetGranulesMap:
            break;
         default:
            handle = ismSTORE_BUILD_HANDLE(ismSTORE_MGMT_GEN_ID, (char *)pDescriptor-ismStore_memGlobal.pStoreBaseAddress);
            TRACE(1, "Failed to commit a store-transaction, because the operation type %d is not valid. Handle 0x%lx, opcount %u (out of %u), GenId %u\n",
                  pOp->OperationType, handle, opcount, pTran->OperationCount, pTran->GenId);
            rc = ISMRC_Error;
      }
      // Mark the operation as done, so we will not try to re-commit it after a failure.
      pOp->OperationType = Operation_Null;
      //ADR_WRITE_BACK(&pOp->OperationType, sizeof(pOp->OperationType));
      pOp++;
      if (rc != ISMRC_OK) { return rc; }
   }

   pTran->OperationCount = 0;
   // Large store transaction consists of multiple chunks
   if (hasMoreChunks)
   {
      ADR_WRITE_BACK(&pTran->OperationCount, sizeof(pTran->OperationCount));
      pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(pDescriptor->NextHandle);
      rc = ism_store_memCommitInternal(pStream, pDescriptor);
   }
   pTran->GenId = ismSTORE_RSRV_GEN_ID;
   pTran->State = ismSTORE_MEM_STORETRANSACTIONSTATE_ACTIVE;
   ADR_WRITE_BACK(pTran, sizeof(*pTran) - sizeof(*pOp));

   return rc;
}

static int32_t ism_store_memRollbackInternal(ismStore_memStream_t *pStream, ismStore_Handle_t handle)
{
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memStoreTransaction_t *pTran;
   ismStore_memStoreOperation_t *pOp;
   int32_t rc = ISMRC_OK, opcount;

   pDescriptor = (ismStore_memDescriptor_t *)ism_store_memMapHandleToAddress(handle);
   pTran = (ismStore_memStoreTransaction_t *)(pDescriptor + 1);
   if ((pTran->State != ismSTORE_MEM_STORETRANSACTIONSTATE_ACTIVE && pTran->State != ismSTORE_MEM_STORETRANSACTIONSTATE_ROLLING_BACK) ||
       (pTran->GenId <= ismSTORE_MGMT_GEN_ID && pTran->OperationCount > 0))
   {
      TRACE(1, "Failed to rollback a store-transaction, because the transaction header is not valid. State %d, GenId %u, OperationCount %u, Handle 0x%lx\n",
            pTran->State, pTran->GenId, pTran->OperationCount, handle);
      return ISMRC_Error;
   }

   pTran->State = ismSTORE_MEM_STORETRANSACTIONSTATE_ROLLING_BACK;
   ADR_WRITE_BACK(&pTran->State, sizeof(pTran->State));

   // Large store transaction consists of multiple chunks
   if (pDescriptor->NextHandle != ismSTORE_NULL_HANDLE && (ismStore_memGlobal.State == ismSTORE_STATE_INIT || pTran->OperationCount >= ismStore_memGlobal.MgmtGen.MaxTranOpsPerGranule))
   {
      if ((rc = ism_store_memRollbackInternal(pStream, pDescriptor->NextHandle)) != ISMRC_OK)
      {
         return rc;
      }
   }

   // Un-apply the store-operations to the data
   opcount = pTran->OperationCount;
   pOp = &pTran->Operations[opcount - 1];
   while (opcount-- > 0)
   {
      switch(pOp->OperationType)
      {
         case Operation_CreateReference:
            rc = ism_store_memCreateReference_Rollback(pStream, pTran, pOp);
            break;
         case Operation_DeleteReference:
            rc = ism_store_memDeleteReference_Rollback(pStream, pTran, pOp);
            break;
         case Operation_UpdateReference:
            rc = ism_store_memUpdateReference_Rollback(pStream, pTran, pOp);
            break;
         case Operation_CreateRecord:
            rc = ism_store_memAssignRecordAllocation_Rollback(pStream, pTran, pOp);
            break;
         case Operation_DeleteRecord:
            rc = ism_store_memFreeRecordAllocation_Rollback(pStream, pTran, pOp);
            break;
         case Operation_UpdateRecord:
            rc = ism_store_memUpdate_Rollback(pStream, pTran, pOp);
            break;
         case Operation_UpdateRecordAttr:
            rc = ism_store_memUpdateAttribute_Rollback(pStream, pTran, pOp);
            break;
         case Operation_UpdateRecordState:
            rc = ism_store_memUpdateState_Rollback(pStream, pTran, pOp);
            break;
         case Operation_UpdateRefState:
            rc = ism_store_memUpdateRefState_Rollback(pStream, pTran, pOp);
            break;
         case Operation_CreateState:
            rc = ism_store_memCreateState_Rollback(pStream, pTran, pOp);
            break;
         case Operation_DeleteState:
            rc = ism_store_memDeleteState_Rollback(pStream, pTran, pOp);
            break;
         case Operation_Null:
         case Operation_UpdateActiveOid:
         case Operation_AllocateGranulesMap:
         case Operation_CreateGranulesMap:
         case Operation_SetGranulesMap:
            break;
         default:
            TRACE(1, "Failed to rollback a store-transaction, because the operation type %d is not valid. Handle 0x%lx, opcount %u (out of %u), GenId %u\n",
                  pOp->OperationType, handle, opcount, pTran->OperationCount, pTran->GenId);
            rc = ISMRC_Error;
      }
      // Mark the operation as done, so we will not try to re-rollback it after a failure.
      pOp->OperationType = Operation_Null;
      //ADR_WRITE_BACK(&pOp->OperationType, sizeof(pOp->OperationType));
      pOp--;
      if (rc != ISMRC_OK) { return rc; }
   }

   pTran->OperationCount = 0;
   pTran->GenId = ismSTORE_RSRV_GEN_ID;
   pTran->State = ismSTORE_MEM_STORETRANSACTIONSTATE_ACTIVE;
   ADR_WRITE_BACK(pTran, sizeof(*pTran) - sizeof(*pOp));

   return rc;
}

#if ismSTORE_ADMIN_CONFIG
/*********************************************************************/
/* Callbcak to get Store configuration changes from Admin            */
/*********************************************************************/
static int ism_store_memConfigCallback(
                   char *pObject,
                   char *pName,
                   ism_prop_t *pProps,
                   ism_ConfigChangeType_t flag)
{
   uint32_t granuleSize;
   int32_t rc = ISMRC_OK;

   TRACE(8, "Store configuration has been changed. Object %s, Name %s, flag %u\n", pObject, pName, flag);

   if (flag == ISM_CONFIG_CHANGE_PROPS)
   {
      granuleSize = ismSTORE_ROUNDUP(ism_common_getIntProperty(pProps, ismSTORE_CFG_GRANULE_SIZE, ismSTORE_CFG_GRANULE_SIZE_DV), 256);
      if (granuleSize < ismSTORE_MIN_GRANULE_SIZE || granuleSize > ismSTORE_MAX_GRANULE_SIZE)
      {
         TRACE(1, "The new store configuration parameter %s (%u) is not valid. Valid range: [%u, %u]\n",
               ismSTORE_CFG_GRANULE_SIZE, granuleSize, ismSTORE_MIN_GRANULE_SIZE, ismSTORE_MAX_GRANULE_SIZE);
         rc = ISMRC_BadPropertyValue;
         ism_common_setErrorData(rc, "%s%u", ismSTORE_CFG_GRANULE_SIZE, granuleSize);
      }
      else
      {
         TRACE(5, "Store configuration parameter %s has been changed. oldValue %u, newValue %u\n",
               ismSTORE_CFG_GRANULE_SIZE, ismStore_memGlobal.GranuleSizeBytes, granuleSize);
         ismStore_memGlobal.GranuleSizeBytes = granuleSize;
      }
   }

   return rc;
}
#endif

char *ism_store_memB2H(char *pDst, unsigned char *pSrc, int len)
{
   char hexArray[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
   char *ptr = pDst;

   while (len--)
   {
      *ptr = hexArray[(*pSrc)>>4]; ptr++;
      *ptr = hexArray[(*pSrc)&15]; ptr++;
      pSrc++;
   }
   *ptr = 0;
   return pDst;
}

// Lock the Store for new threads and wait until there are no active threads in the Store
int ism_store_memLockStore(int waitMilli, int caller)
{
   ismStore_memStream_t *pStream, *pInternalStream;
   int i;
   uint8_t fBusy, fLocked;

   TRACE(9, "Entry: %s. State %u\n", __FUNCTION__, ismStore_memGlobal.State);
   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
   pInternalStream = ismStore_memGlobal.pStreams[ismStore_memGlobal.hInternalStream];

   if (!ismStore_memGlobal.fLocked)
   {
      while (ismStore_memGlobal.State >= ismSTORE_STATE_INIT && ismStore_memGlobal.State <= ismSTORE_STATE_ACTIVE)  /* BEAM suppression: infinite loop */
      {
         fBusy = (ismStore_memGlobal.State == ismSTORE_STATE_RECOVERY ? 1 : 0);
         for (i=0; i < ismStore_memGlobal.StreamsSize; i++)
         {
            pStream = ismStore_memGlobal.pStreams[i];
            if (pStream != NULL && pStream != pInternalStream)  // skip the internal stream
            {
               ismSTORE_MUTEX_LOCK(&pStream->Mutex);
               pStream->fLocked |= 1;
               if (pStream->MyGenId != ismSTORE_RSRV_GEN_ID)
               {
                  if (!(pStream->fLocked&2) ) 
                  {
                    TRACE(5,"Stream %u is within store transaction, preventing the store from being locked.\n",i);
                    pStream->fLocked |= 2 ; 
                  }
                  fBusy = 1;
               }
               ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
            }
         }
         if (!fBusy)  // lock the internal stream last
         {
            ismSTORE_MUTEX_LOCK(&pInternalStream->Mutex);
            pInternalStream->fLocked |= 1;
            if (pInternalStream->MyGenId != ismSTORE_RSRV_GEN_ID)
            {
               if (!(pInternalStream->fLocked&2) ) 
               {
                 TRACE(5,"Stream %u is within store transaction, preventing the store from being locked.\n",ismStore_memGlobal.hInternalStream);
                 pInternalStream->fLocked |= 2 ; 
               }
               fBusy = 1;
            }
            ismSTORE_MUTEX_UNLOCK(&pInternalStream->Mutex);
         }

         if (!fBusy)
         {
            ismStore_memGlobal.fLocked |= caller;
            TRACE(5, "The Store is locked\n");
            break;
         }
         if ( waitMilli > 0 && ismStore_memGlobal.State != ismSTORE_STATE_RECOVERY ) waitMilli-- ; 
         if (!waitMilli )
           break ; 

         // Give the engine threads a chance to complete the Store-Transaction
         pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
         ism_common_backHome();
         ism_common_sleep(1000);
         ism_common_going2work();
         pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
      }

      if (!ismStore_memGlobal.fLocked && waitMilli < 0 )
      {
         TRACE(5, "Failed to lock the Store, because the Store is being terminated. State %d\n", ismStore_memGlobal.State);
      }
   }
   else
   {
      ismStore_memGlobal.fLocked |= caller;
      TRACE(1, "The Store is already locked, fLocked=%u\n",ismStore_memGlobal.fLocked);
   }
   fLocked = ismStore_memGlobal.fLocked & caller ; 
   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
   TRACE(9, "Exit: %s. fLocked %d\n", __FUNCTION__, fLocked);

   return fLocked;
}

// Unlock the Store and allow threads to access the Store
static void unLockStreams(void)
{
   int i;
   ismStore_memStream_t *pStream;
   for (i=0; i < ismStore_memGlobal.StreamsSize; i++)
   {
      if ((pStream = ismStore_memGlobal.pStreams[i]) != NULL)
      {
         ismSTORE_MUTEX_LOCK(&pStream->Mutex);
         pStream->fLocked = 0;
         ismSTORE_COND_BROADCAST(&pStream->Cond);
         ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
      }
   }
}
void ism_store_memUnlockStore(int caller)
{
   TRACE(9, "Entry: %s\n", __FUNCTION__);
   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
   if ( !(ismStore_memGlobal.fLocked & caller) )
   {
      if ( ismStore_memGlobal.fLocked )
      {
         TRACE(1,"The store is locked, skipping cleanOnly!\n");
      }
      else
      {
         TRACE(1,"Reversing a failed attempt to lock the store.\n");
         unLockStreams();
      } 
   }
   else
   if ( ismStore_memGlobal.fLocked != caller)
   {
      ismStore_memGlobal.fLocked &= ~caller ; 
      TRACE(1,"The store is multiLocked, fLocked=%u, caller=%u\n",ismStore_memGlobal.fLocked,caller);
   }
   else
   {
      unLockStreams();
   ismStore_memGlobal.fLocked = 0;
   TRACE(5, "The Store is un-locked\n");
   }
   pthread_cond_signal(&ismStore_memGlobal.StreamsCond);

   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
   TRACE(9, "Exit: %s\n", __FUNCTION__);
}

static void ism_store_memCheckCompactThreshold(ismStore_GenId_t genId, ismStore_memGenMap_t *pGenMap)
{
   ismStore_memJob_t job;

   if (pGenMap->RecordsCount == pGenMap->DelRecordsCount)
   {
      // The generation is no longer used
      TRACE(5, "Store generation Id %u is no longer used. DiskFileSize %lu, PredictedSizeBytes %lu, RecordsCount %u\n",
            genId, pGenMap->DiskFileSize, pGenMap->PredictedSizeBytes, pGenMap->RecordsCount);
      pGenMap->RecordsCount = pGenMap->DelRecordsCount = 0;

      memset(&job, '\0', sizeof(job));
      job.JobType = StoreJob_DeleteGeneration;
      job.Generation.GenId = genId;
      ism_store_memAddJob(&job);
      return;
   }

   if (pGenMap->fCompactReady)
   {
      return;
   }

   if (ismSTORE_NEED_DISK_COMPACT)
   {
      // The generation should be compacted
      TRACE(5, "Store generation Id %u should be compacted. DiskFileSize %lu, PredictedSizeBytes %lu, PrevPredictedSizeBytes %lu, HARemoteSizeBytes %lu, RecordsCount %u, DelRecordsCount %u, GranulesMapCount %u, fCompactReady %u\n",
            genId, pGenMap->DiskFileSize, pGenMap->PredictedSizeBytes,
            pGenMap->PrevPredictedSizeBytes, pGenMap->HARemoteSizeBytes,
            pGenMap->RecordsCount, pGenMap->DelRecordsCount,
            pGenMap->GranulesMapCount, pGenMap->fCompactReady);

      pGenMap->fCompactReady = 2;
      memset(&job, '\0', sizeof(job));
      job.JobType = StoreJob_CompactGeneration;
      job.Generation.GenId = genId;
      ism_store_memAddJob(&job);
   }
   else if (ismSTORE_HASSTANDBY && ismSTORE_NEED_MEM_COMPACT)
   {
      // The generation should be compacted on the Standby node
      TRACE(5, "Store generation Id %u should be compacted on the Standby node. DiskFileSize %lu, PredictedSizeBytes %lu, PrevPredictedSizeBytes %lu, HARemoteSizeBytes %lu, RecordsCount %u, DelRecordsCount %u, GranulesMapCount %u, fCompactReady %u\n",
            genId, pGenMap->DiskFileSize, pGenMap->PredictedSizeBytes, pGenMap->PrevPredictedSizeBytes,
            pGenMap->HARemoteSizeBytes, pGenMap->RecordsCount, pGenMap->DelRecordsCount,
            pGenMap->GranulesMapCount, pGenMap->fCompactReady);

      pGenMap->fCompactReady = 1;
      memset(&job, '\0', sizeof(job));
      job.JobType = StoreJob_CompactGeneration;
      job.Generation.GenId = genId;
      ism_store_memAddJob(&job);
   }
}

static void ism_store_memHandleHAIntAck(ismStore_memHAAck_t *pAck)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t  *pGenHeader;
   ismStore_memStream_t *pStream;
   ismStore_memJob_t job;
   int i, j;

   TRACE(9, "HAAck: An ACK has been received. ChannelId 0, AckSqn %lu, SrcMsgType 0x%x, ReturnCode %d\n",
         pAck->AckSqn, pAck->SrcMsgType, pAck->ReturnCode);

   if (pAck->ReturnCode != StoreRC_OK)
   {
      TRACE(1, "HAAck: Failed to process the message (ChannelId 0, MsqSqn %lu, MsgType %u) on the Standby node. error code %d\n",
            pAck->AckSqn, pAck->SrcMsgType, pAck->ReturnCode);
   }
   int nInMem = ismStore_memGlobal.InMemGensCount <= ismSTORE_MAX_INMEM_GENS ? ismStore_memGlobal.InMemGensCount : ismSTORE_MAX_INMEM_GENS ; 
   switch (pAck->SrcMsgType)
   {
      case StoreHAMsg_CreateGen:
         pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
         for (i=0; i < nInMem; i++)
         {
            pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[i].pBaseAddress;
            if (ismStore_memGlobal.InMemGens[i].HACreateState == 1 && ismStore_memGlobal.InMemGens[i].HACreateSqn == pAck->AckSqn)
            {
               TRACE(5, "HAAck: Generation Id %u (Index %u) has been created on the Standby node. State %u, ReturnCode %d\n",
                     pGenHeader->GenId, i, pGenHeader->State, pAck->ReturnCode);
               ismStore_memGlobal.InMemGens[i].HACreateState = 2;
            }
         }
         pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
         break;

      case StoreHAMsg_ActivateGen:
         pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
         for (i=0; i < nInMem; i++)
         {
            pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[i].pBaseAddress;
            if (ismStore_memGlobal.InMemGens[i].HAActivateState == 1 && ismStore_memGlobal.InMemGens[i].HAActivateSqn == pAck->AckSqn)
            {
               TRACE(5, "HAAck: Generation Id %u (Index %u) has been activated on the Standby node. State %u, ReturnCode %d\n",
                     pGenHeader->GenId, i, pGenHeader->State, pAck->ReturnCode);
               ismStore_memGlobal.InMemGens[i].HAActivateState = 2;
               pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
               if (pMgmtHeader->ActiveGenId == pGenHeader->GenId)
               {
                  for (j=0; j < ismStore_memGlobal.StreamsSize; j++)
                  {
                     if ((pStream = ismStore_memGlobal.pStreams[j]))
                     {
                        ismSTORE_MUTEX_LOCK(&pStream->Mutex);
                        pStream->ActiveGenId = pMgmtHeader->ActiveGenId;
                        pStream->ActiveGenIndex = pMgmtHeader->ActiveGenIndex;
                        ismSTORE_COND_BROADCAST(&pStream->Cond);
                        ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
                     }
                  }
               }
            }
         }
         pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
         break;

      case StoreHAMsg_WriteGen:
         pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
         for (i=0; i < nInMem; i++)
         {
            pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[i].pBaseAddress;
            if (ismStore_memGlobal.InMemGens[i].HAWriteState == 1 && ismStore_memGlobal.InMemGens[i].HAWriteSqn == pAck->AckSqn)
            {
               TRACE(5, "HAAck: Generation Id %u (Index %u) has been written to the Standby node disk. State %u\n",
                     pGenHeader->GenId, i, pGenHeader->State);
               ismStore_memGlobal.InMemGens[i].HAWriteState = 2;
               if (pGenHeader->State == ismSTORE_GEN_STATE_WRITE_COMPLETED)
               {
                  memset(&job, '\0', sizeof(job));
                  job.JobType = StoreJob_CreateGeneration;
                  job.Generation.GenIndex = i;
                  ism_store_memAddJob(&job);
               }
            }
         }
         pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
         break;

      case StoreHAMsg_AssignRsrvPool:
         if (ismStore_memGlobal.RsrvPoolState == 2 && ismStore_memGlobal.RsrvPoolHASqn == pAck->AckSqn)
         {
            TRACE(5, "HAAck: The management reserved pool (PoolId %u) has been assigned on the Standby node. RsrvPoolHASqn %lu\n",
                  ismStore_memGlobal.RsrvPoolId, ismStore_memGlobal.RsrvPoolHASqn);
            ismStore_memGlobal.RsrvPoolState = 3;  /* RsrvPool is initialized */
            ism_store_memAttachRsrvPool();
         }
         else
         {
            TRACE(3, "HAAck: Failed to assign the reserved pool (PoolId %u) on the Standby node. RsrvPoolState %u, RsrvPoolHASqn %lu (%lu), reason code %d\n",
                  ismStore_memGlobal.RsrvPoolId, ismStore_memGlobal.RsrvPoolState, ismStore_memGlobal.RsrvPoolHASqn, pAck->AckSqn, pAck->ReturnCode);
         }
         break;

      default:
         break;
   }
}

static void ism_store_memCloseHAChannels(void)
{
   ismStore_memStream_t *pStream;
   int i;

   TRACE(9, "Entry: %s\n", __FUNCTION__);
   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
   if ( ismStore_memGlobal.fEnablePersist ) ism_storePersist_wrLock();
   for (i=0; i < ismStore_memGlobal.StreamsSize; i++)
   {
      if ((pStream = ismStore_memGlobal.pStreams[i]) != NULL)
      {
         ismSTORE_MUTEX_LOCK(&pStream->Mutex);
         if (pStream->pHAChannel)
         {
            ism_store_memHACloseChannel(pStream->pHAChannel, 0);
            pStream->pHAChannel = NULL;
         }
         ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
      }
   }
   ismStore_memGlobal.HAInfo.pIntChannel = NULL;
   if ( ismStore_memGlobal.fEnablePersist ) ism_storePersist_wrUnlock();
   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
   TRACE(9, "Exit: %s\n", __FUNCTION__);
}

static void ism_store_memHandleHAEvent(ismStore_memJob_t *pJob)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memMgmtHeader_t *pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memStream_t *pStream;
   ismStore_memJob_t job;
   int ec=ISMRC_OK, i, j;

   TRACE(9, "Entry: %s. JobType %u\n", __FUNCTION__, pJob->JobType);

   TRACE(5, "HAEvent: JobType %u, ViewId %u, NewRole %u, OldRole %u, ActiveNodesCount %u\n",
         pJob->JobType, pJob->HAView.ViewId, pJob->HAView.NewRole, pJob->HAView.OldRole, pJob->HAView.ActiveNodesCount);

   // Make sure that all the sync generation files have already been written to the Standby disk
   if (pJob->HAView.OldRole == ISM_HA_ROLE_STANDBY && pJob->HAView.NewRole != ISM_HA_ROLE_ERROR && pJob->HAView.ActiveNodesCount == 1)
   {
      if (ism_store_memHASyncWaitDisk() == StoreRC_OK)
      {
         // See whether we have to mark the store role as primary
         if (pMgmtHeader->Role == ismSTORE_ROLE_STANDBY)
         {
            pMgmtHeader->Role = ismSTORE_ROLE_PRIMARY;
            ADR_WRITE_BACK(&pMgmtHeader->Role, sizeof(pMgmtHeader->Role));
            TRACE(5, "Store role has been changed from %d to %d\n", ismSTORE_ROLE_STANDBY, pMgmtHeader->Role);
         }
      }
      else
      {
         TRACE(1, "HAEvent: Failed to complete the new node synchronization due to a disk write error\n");
         ec = ISMRC_StoreDiskError;
      }
   }

   if (ec == ISMRC_OK && pJob->JobType == StoreJob_HAStandby2Primary)
   {
      pthread_mutex_lock(&pHAInfo->Mutex);
      while ( ismStore_memGlobal.fWithinStart )
      {
         pthread_mutex_unlock(&pHAInfo->Mutex);
         TRACE(1, "HAEvent: Waiting for imaserver thread to finish ism_store_start()...\n");
         ism_common_sleep(1000000);  // 1 seconds
         pthread_mutex_lock(&pHAInfo->Mutex);
      }
      pthread_mutex_unlock(&pHAInfo->Mutex);

      if ((ec = ism_store_memValidate()) == ISMRC_OK)
      {
         if ((ec = ism_store_memRecoveryPrepare(1)) == ISMRC_OK)
         {
            ismStore_memGlobal.State = ismSTORE_STATE_RECOVERY;
            TRACE(5, "HAEvent: The Store is now ready for recovery\n");
         }
         else
         {
            TRACE(1, "HAEvent: Failed to switch from Standby to Primary due to a store recovery error\n. error code %d", ec);
         }
      }
      else
      {
         TRACE(1, "HAEvent: Failed to switch from Standby to Primary due to a store validation error. error code %d", ec);
      }

      //HA_Persist: synchronously create a checkpoint here and update the PState file 
      pMgmtHeader->WasPrimary = 1;  // make sure WasPrimary is persisted in case the SB2PR fails before a CP is created
      if ( ec == ISMRC_OK && ismStore_memGlobal.fEnablePersist && (ec = ism_storePersist_createCP(0)) != ISMRC_OK)
      {
         TRACE(1, "HAEvent: ism_storePersist_createCP() failed. error code %d", ec);
      }
      // Start the ShmPersist thread
      if (ec == ISMRC_OK && ismStore_memGlobal.fEnablePersist && (ec = ism_storePersist_start()) != StoreRC_OK)
      {
         TRACE(1, "HAEvent: ism_storePersist_start() failed. error code %d", ec);
      }
   }

   // Prepare a view information for the Admin layer
   pthread_mutex_lock(&pHAInfo->Mutex);
   if (ec != ISMRC_OK)
   {
      pHAInfo->View.NewRole = pJob->HAView.NewRole = ISM_HA_ROLE_ERROR;
      pHAInfo->SyncNodesCount = 0;
      pHAInfo->State = ismSTORE_HA_STATE_ERROR;
   }
   memset(&pHAInfo->AdminView, '\0', sizeof(pHAInfo->AdminView));
   pHAInfo->AdminView.ActiveNodesCount = pJob->HAView.ActiveNodesCount;
   pHAInfo->AdminView.SyncNodesCount = pHAInfo->SyncNodesCount;
   pHAInfo->AdminView.NewRole = pJob->HAView.NewRole;
   pHAInfo->AdminView.OldRole = pJob->HAView.OldRole;
   pHAInfo->AdminView.LocalReplicationNIC = pHAInfo->View.LocalReplicationNIC ; 
   pHAInfo->AdminView.LocalDiscoveryNIC   = pHAInfo->View.LocalDiscoveryNIC   ; 
   pHAInfo->AdminView.RemoteDiscoveryNIC  = pHAInfo->View.RemoteDiscoveryNIC  ; 
   if (pHAInfo->View.NewRole == ISM_HA_ROLE_ERROR)
   {
      pHAInfo->AdminView.ReasonCode = pHAInfo->View.ReasonCode;
      pHAInfo->AdminView.pReasonParam = pHAInfo->View.pReasonParam;
   }
   pthread_cond_signal(&pHAInfo->ViewCond);
   pthread_mutex_unlock(&pHAInfo->Mutex);
   ism_common_sleep(10000);  // allow fSyncFailed to be set if needed

   if ( ismStore_global.PrimaryMemSizeBytes || ismStore_global.fSyncFailed )
   {
      TRACE(1, "HAEvent: Skipp Calling ism_ha_admin_viewChanged, since a restart is called for.\n");
   }
   else
   {
      // Inform the Admin layer on the new view
      TRACE(5, "HAEvent: Calling ism_ha_admin_viewChanged. NewRole %u, OldRole %u, ActiveNodesCount %u, SyncNodesCount %u, ReasonCode %d\n",
            pHAInfo->AdminView.NewRole, pHAInfo->AdminView.OldRole, pHAInfo->AdminView.ActiveNodesCount, pHAInfo->AdminView.SyncNodesCount, pHAInfo->AdminView.ReasonCode);
      ism_ha_admin_viewChanged(&pHAInfo->AdminView);
   
      // update Cluster HA status (ignore return code) 
      if (pHAInfo->AdminView.NewRole == ISM_HA_ROLE_PRIMARY)
      {
        if (pHAInfo->AdminView.SyncNodesCount > 1)
          ism_cluster_setHaStatus(ISM_CLUSTER_HA_PRIMARY_PAIR);
        else
          ism_cluster_setHaStatus(ISM_CLUSTER_HA_PRIMARY_SINGLE);
      }
      else if (pHAInfo->AdminView.NewRole == ISM_HA_ROLE_ERROR)
          ism_cluster_setHaStatus(ISM_CLUSTER_HA_ERROR);
      else
          ism_cluster_setHaStatus(ISM_CLUSTER_HA_STANDBY);
   }

   if (pJob->JobType == StoreJob_HAStandbyJoined)
   {
      if ((ec = ism_store_memHASyncStart()) != StoreRC_OK)
      {
         TRACE(1, "HAEvent: Failed to start the new node synchronization procedure. error code %d\n", ec);
      }
   }
   else if (pJob->JobType == StoreJob_HAStandbyLeft)
   {
      for ( i=0 ; i<3 ; i++ )
      {
         if (ism_store_memLockStore(3300,LOCKSTORE_CALLER_STOR))
         {
            ism_store_memCloseHAChannels();
            ism_store_memUnlockStore(LOCKSTORE_CALLER_STOR);
            break ; 
         }
         else
         {
            if ( i < 2 )
            {
               TRACE(1,"ism_store_memLockStore failed after %u secs, will ism_store_memUnlockStore() and retry...\n",i*3300/1000);
               ism_common_stack_trace(0);
               ism_store_memUnlockStore(LOCKSTORE_CALLER_STOR);
               ism_common_sleep(33000);  // 33 ms
            }
            else
            {
               TRACE(1,"ism_store_memLockStore failed after 10 secs, ism_store_memCloseHAChannels was NOT called\n");
               ism_common_stack_trace(0);
               ism_store_memUnlockStore(LOCKSTORE_CALLER_STOR);
            }
         }
      }

      // See whether there are either activate or write pending generations
      pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
      for (i=0; i < ismStore_memGlobal.InMemGensCount; i++)
      {
         pGenHeader = (ismStore_memGenHeader_t *)ismStore_memGlobal.InMemGens[i].pBaseAddress;
         if (ismStore_memGlobal.InMemGens[i].HAWriteState == 1)
         {
            TRACE(5, "HAEvent: Generation Id %u (Index %u) was not written on the Standby node, because the Standby node left. State %u\n",
                  pGenHeader->GenId, i, pGenHeader->State);
            ismStore_memGlobal.InMemGens[i].HAWriteState = 0;
            if (pGenHeader->State == ismSTORE_GEN_STATE_WRITE_COMPLETED)
            {
               memset(&job, '\0', sizeof(job));
               job.JobType = StoreJob_CreateGeneration;
               job.Generation.GenIndex = i;
               ism_store_memAddJob(&job);
            }
         }
         if (ismStore_memGlobal.InMemGens[i].HAActivateState == 1)
         {
            ismStore_memGlobal.InMemGens[i].HAActivateState = 0;
            if (pMgmtHeader->ActiveGenId == pGenHeader->GenId)
            {
               TRACE(5, "HAEvent: Generation Id %u (Index %u) was not activated on the Standby node, because the Standby node left. State %u\n",
                     pGenHeader->GenId, i, pGenHeader->State);
               for (j=0; j < ismStore_memGlobal.StreamsSize; j++)
               {
                  if ((pStream = ismStore_memGlobal.pStreams[j]))
                  {
                     ismSTORE_MUTEX_LOCK(&pStream->Mutex);
                     pStream->ActiveGenId = pMgmtHeader->ActiveGenId;
                     pStream->ActiveGenIndex = pMgmtHeader->ActiveGenIndex;
                     ismSTORE_COND_BROADCAST(&pStream->Cond);
                     ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
                  }
               }
            }
         }
      }
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

      // See whether there is a pending RsrvPool assignment
      if (ismStore_memGlobal.RsrvPoolState == 2)
      {
         TRACE(5, "HAEvent: The management reserved pool (PoolId %u) was not assigned on the Standby node, because the Standby node left. RsrvPoolHASqn %lu\n",
               ismStore_memGlobal.RsrvPoolId, ismStore_memGlobal.RsrvPoolHASqn);
         ismStore_memGlobal.RsrvPoolState = 3;  /* RsrvPool is initialized */
         ism_store_memAttachRsrvPool();
      }
   }
   else if (pJob->JobType != StoreJob_HAViewChanged && pJob->JobType != StoreJob_HAStandby2Primary)
   {
      TRACE(1, "HAEvent: An invalid event has been received. JobType %u\n", pJob->JobType);
   }

   TRACE(9, "Exit: %s\n", __FUNCTION__);
}

/*********************************************************************/
/* Store Disk Operations                                             */
/*********************************************************************/

void ism_store_memSetStoreState(int state, int lock)
{
   int i;
   ismStore_memStream_t *pStream;

   if (lock) pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
   ismStore_memGlobal.State = state;
   // Unblock all the streams that are waiting for a new generation
   for (i=0; i < ismStore_memGlobal.StreamsSize; i++)
   {
       if ((pStream = ismStore_memGlobal.pStreams[i]))
       {
          ismSTORE_MUTEX_LOCK(&pStream->Mutex);
          ismSTORE_COND_BROADCAST(&pStream->Cond);
          ismSTORE_MUTEX_UNLOCK(&pStream->Mutex);
       }
   }
   if (lock) pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
}

void ism_store_memDiskWriteComplete(ismStore_GenId_t genId, int32_t retcode, ismStore_DiskGenInfo_t *pDiskGenInfo, void *pContext)
{
   ismStore_memGeneration_t *pGen;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memGenMap_t *pGenMap;
   ismStore_memDiskWriteCtxt_t *pDiskWriteCtxt = (ismStore_memDiskWriteCtxt_t *)pContext;
   ismStore_memJob_t job;
   uint8_t genIndex;

   TRACE(9, "Entry: %s. GenId %u, retcode %d\n", __FUNCTION__, genId, retcode);
   if (retcode == StoreRC_OK || retcode == StoreRC_Disk_TaskCancelled || retcode == StoreRC_Disk_TaskInterrupted)
   {
      if (retcode == StoreRC_OK)
      {
         TRACE(5, "Store generation Id %u has been written to the disk. State %d, CompactDataSizeBytes %lu\n",
               genId, ismStore_memGlobal.State, pDiskWriteCtxt->CompDataSizeBytes);
      }
      else
      {
         TRACE(5, "Write store generation Id %u has been cancelled. reason code %d\n", genId, retcode);
      }

      pGen = pDiskWriteCtxt->pGen;
      pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
      genIndex = ((char *)pGen - (char *)ismStore_memGlobal.InMemGens) / sizeof(*pGen);
      if (genIndex >= 0 && genIndex < ismStore_memGlobal.InMemGensCount)
      {
         pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
         if (pGenHeader->GenId == genId && pGenHeader->State == ismSTORE_GEN_STATE_WRITE_PENDING)
         {
            pGenHeader->State = ismSTORE_GEN_STATE_WRITE_COMPLETED;
            ADR_WRITE_BACK(&pGenHeader->State, sizeof(pGenHeader->State));
            if ((pGenMap = ismStore_memGlobal.pGensMap[genId]))
            {
               pthread_mutex_lock(&pGenMap->Mutex);
               pGenMap->DiskFileSize = pGenMap->PrevPredictedSizeBytes = (pDiskWriteCtxt->pCompData ? pDiskWriteCtxt->CompDataSizeBytes : pGenHeader->MemSizeBytes);
               pthread_mutex_unlock(&pGenMap->Mutex);
            }
            else
            {
               TRACE(5, "Could not update DiskFileSize (%lu) of generation Id %u, because the generation is being deleted\n", pGenHeader->MemSizeBytes, genId);
            }

            if (ismStore_memGlobal.State == ismSTORE_STATE_ACTIVE && pGen->HAWriteState != 1)
            {
               memset(&job, '\0', sizeof(job));
               job.JobType = StoreJob_CreateGeneration;
               job.Generation.GenIndex = genIndex;
               ism_store_memAddJob(&job);
            }

            if (pGen->HAWriteState == 1)
            {
               TRACE(5, "Store generation Id %u (Index %u) is still being written to the Standby disk\n", genId, genIndex);
            }
         }
         pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
      }
      else
      {
         TRACE(1, "The generation Id %u in the disk write callback is not valid.\n", genId);
      }
      if (ismStore_memGlobal.fEnablePersist)
         ism_storePersist_onGenWrite(genIndex,0) ; 
   }
   else
   {
      TRACE(1, "Failed to write store generation Id %u to the disk. error code %d\n", genId, retcode);
      ism_store_memSetStoreState(ismSTORE_STATE_DISKERROR,1);
   }

   // created via mmap so use the Basic free to skip memory accounting
   ismSTORE_FREE_BASIC(pDiskWriteCtxt->pCompData);
   ismSTORE_FREE(pDiskWriteCtxt);

   // Add a job to check the disk usage
   memset(&job, '\0', sizeof(job));
   job.JobType = StoreJob_CheckDiskUsage;
   ism_store_memAddJob(&job);

   TRACE(9, "Exit: %s\n", __FUNCTION__);
}

static void ism_store_memDiskWriteBackupComplete(ismStore_GenId_t genId, int32_t retcode, ismStore_DiskGenInfo_t *pDiskGenInfo, void *pContext)
{
   TRACE(9, "Entry: %s. GenId %u, retcode %d\n", __FUNCTION__, genId, retcode);

   if (retcode == StoreRC_OK)
   {
      TRACE(5, "Store generation Id %u has been written to the disk. State %d\n", genId, ismStore_memGlobal.State);
   }
   else
   {
      TRACE(1, "Failed to write store generation Id %u to the disk. error code %d\n", genId, retcode);
   }

   TRACE(9, "Exit: %s\n", __FUNCTION__);
}

static void ism_store_memDiskReadComplete(ismStore_GenId_t genId, int32_t retcode, ismStore_DiskGenInfo_t *pDiskGenInfo, void *pContext)
{
   TRACE(9, "Entry: %s. GenId %u, retcode %d\n", __FUNCTION__, genId, retcode);
   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
   if (retcode == StoreRC_OK)
   {
      TRACE(5, "Store generation Id %u has been restored from the disk.\n", genId);
      ismStore_memGlobal.State = ismSTORE_STATE_RESTORED;
   }
   else
   {
      TRACE(1, "Failed to restore store generation Id %u from the disk. error code %d\n", genId, retcode);
      ism_store_memSetStoreState(ismSTORE_STATE_DISKERROR,0);
   }
   pthread_cond_signal(&ismStore_memGlobal.StreamsCond);
   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);
   TRACE(9, "Exit: %s\n", __FUNCTION__);
}

static void ism_store_memDiskDeleteComplete(ismStore_GenId_t genId, int32_t retcode, ismStore_DiskGenInfo_t *pDiskGenInfo, void *pContext)
{
   ismStore_memJob_t job;

   TRACE(9, "Entry: %s. GenId %u, retcode %d\n", __FUNCTION__, genId, retcode);

   if (retcode == StoreRC_OK)
   {
      TRACE(5, "Store generation Id %u has been deleted from the disk.\n", genId);
   }
   else
   {
      TRACE(1, "Failed to delete store generation Id %u from the disk. error code %d\n", genId, retcode);
   }

   // Delete the entry from the pGensMap array
   pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
   ism_store_memFreeGenMap(genId);
   pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

   // Add a job to check the disk usage
   if (ismStore_memGlobal.fDiskAlertOn || ismStore_memGlobal.fCompactDiskAlertOn)
   {
      memset(&job, '\0', sizeof(job));
      job.JobType = StoreJob_CheckDiskUsage;
      ism_store_memAddJob(&job);
   }

   TRACE(9, "Exit: %s\n", __FUNCTION__);
}

static void ism_store_memDiskCompactComplete(ismStore_GenId_t genId, int32_t retcode, ismStore_DiskGenInfo_t *pDiskGenInfo, void *pContext)
{
   ismStore_memGenMap_t *pGenMap;
   ismStore_memJob_t job;
   uint64_t oldFileSize;
   double compactRatio=0;

   TRACE(9, "Entry: %s. GenId %u, retcode %d\n", __FUNCTION__, genId, retcode);
   if (retcode == StoreRC_OK)
   {
      pthread_mutex_lock(&ismStore_memGlobal.StreamsMutex);
      if ((pGenMap = ismStore_memGlobal.pGensMap[genId]))
      {
         pthread_mutex_lock(&pGenMap->Mutex);
         if ((oldFileSize = pGenMap->DiskFileSize) > 0) {
            compactRatio = 1.0 - (double)pDiskGenInfo->DataLength / oldFileSize;
         }
         pGenMap->DiskFileSize = pGenMap->PrevPredictedSizeBytes = pDiskGenInfo->DataLength;

         if (pGenMap->RecordsCount > 0)
         {
            pGenMap->MeanRecordSizeBytes = pGenMap->DiskFileSize / pGenMap->RecordsCount;
            pGenMap->PredictedSizeBytes = pGenMap->DiskFileSize - (pGenMap->DelRecordsCount * pGenMap->MeanRecordSizeBytes);
            pGenMap->StdDevBytes = pDiskGenInfo->StdDev;
         }
         else
         {
            pGenMap->MeanRecordSizeBytes = 0;
            pGenMap->PredictedSizeBytes = 0;
            pGenMap->StdDevBytes = 0;
         }

         if (pGenMap->DiskFileSize > pGenMap->HARemoteSizeBytes)
         {
            pGenMap->HARemoteSizeBytes = pGenMap->DiskFileSize;
         }

         TRACE(5, "Store generation Id %u has been compacted. OldFileSize %lu, DiskFileSize %lu, PredictedSizeBytes %lu, RecordsCount %u, DelRecordsCount %u, MeanRecordSizeBytes %u, StdDevBytes %lu, CompactRatio %f, fComapctReady %u, State %d\n",
               genId, oldFileSize, pGenMap->DiskFileSize, pGenMap->PredictedSizeBytes,
               pGenMap->RecordsCount, pGenMap->DelRecordsCount, pGenMap->MeanRecordSizeBytes,
               pGenMap->StdDevBytes, compactRatio, pGenMap->fCompactReady, ismStore_memGlobal.State);

         pthread_mutex_unlock(&pGenMap->Mutex);
      }
      else
      {
         TRACE(1, "Could not update the CompactDataSizeBytes (%lu) of generation Id %u, due to an internal error\n",
               pDiskGenInfo->DataLength, genId);
      }
      pthread_mutex_unlock(&ismStore_memGlobal.StreamsMutex);

      // Add a job to check the disk usage
      if (ismStore_memGlobal.fDiskAlertOn || ismStore_memGlobal.fCompactDiskAlertOn)
      {
         memset(&job, '\0', sizeof(job));
         job.JobType = StoreJob_CheckDiskUsage;
         ism_store_memAddJob(&job);
      }
   }
   else if (retcode == StoreRC_Disk_TaskCancelled || retcode == StoreRC_Disk_TaskInterrupted)
   {
      TRACE(5, "Compact store generation Id %u has been cancelled. reason code %u\n", genId, retcode);
   }
   else
   {
      TRACE(1, "Failed to compact store generation Id %u. error code %d\n", genId, retcode);
   }

   TRACE(9, "Exit: %s\n", __FUNCTION__);
}

static void ism_store_memDiskTerminated(ismStore_GenId_t GenId, int32_t retcode, ismStore_DiskGenInfo_t *pDiskGenInfo, void *pContext)
{
   pthread_mutex_lock(&ismStore_memGlobal.DiskMutex);
   ismStore_memGlobal.fDiskUp = 0;
   TRACE(5, "Store Disk component has been terminated successfully\n");
   pthread_mutex_unlock(&ismStore_memGlobal.DiskMutex);
}

static void ism_store_memCheckDiskUsage(void)
{
   ismStore_DiskStatistics_t diskStats;
   ismStore_memJob_t job;
   double pct;
   int ec;

   TRACE(9, "Entry: %s\n", __FUNCTION__);
   memset(&diskStats, '\0', sizeof(ismStore_DiskStatistics_t));
   if ((ec = ism_storeDisk_getStatistics(&diskStats)) == StoreRC_OK)
   {
      if (diskStats.DiskUsedSpaceBytes > 0 || diskStats.DiskFreeSpaceBytes > 0)
      {
         pct = 100 * ((double)diskStats.DiskUsedSpaceBytes / (diskStats.DiskUsedSpaceBytes + diskStats.DiskFreeSpaceBytes));
         TRACE(7, "Store disk space usage: DiskUsedSpaceBytes %lu, GensUsedSpaceBytes %lu, DiskFreeSpaceBytes %lu, DiskUsagePct %.2f, fDiskAlertOn %u, fCompactDiskAlertOn %u\n",
               diskStats.DiskUsedSpaceBytes, diskStats.GensUsedSpaceBytes, diskStats.DiskFreeSpaceBytes, pct, ismStore_memGlobal.fDiskAlertOn, ismStore_memGlobal.fCompactDiskAlertOn);

         // See whether a user disk alert event is required
         if (ismStore_memGlobal.DiskAlertOnPct > 0)
         {
            if (!ismStore_memGlobal.fDiskAlertOn && pct > ismStore_memGlobal.DiskAlertOnPct)
            {
               TRACE(5, "Store disk space usage (%.2f) reached the high water mark (%u). DiskUsedSpaceBytes %lu, DiskFreeSpaceBytes %lu\n",
                     pct, ismStore_memGlobal.DiskAlertOnPct, diskStats.DiskUsedSpaceBytes, diskStats.DiskFreeSpaceBytes);
               ismStore_memGlobal.fDiskAlertOn = 1;
               if (ismStore_memGlobal.OnEvent)
               {
                  memset(&job, '\0', sizeof(job));
                  job.JobType = StoreJob_UserEvent;
                  job.Event.EventType = ISM_STORE_EVENT_DISK_ALERT_ON;
                  ism_store_memAddJob(&job);
               }
            }
            else if (ismStore_memGlobal.fDiskAlertOn && pct < ismStore_memGlobal.DiskAlertOffPct)
            {
               TRACE(5, "Store disk space usage (%.2f) returned to normal (%u). DiskUsedSpaceBytes %lu, DiskFreeSpaceBytes %lu\n",
                     pct, ismStore_memGlobal.DiskAlertOnPct, diskStats.DiskUsedSpaceBytes, diskStats.DiskFreeSpaceBytes);
               ismStore_memGlobal.fDiskAlertOn = 0;
               if (ismStore_memGlobal.OnEvent)
               {
                  memset(&job, '\0', sizeof(job));
                  job.JobType = StoreJob_UserEvent;
                  job.Event.EventType = ISM_STORE_EVENT_DISK_ALERT_OFF;
                  ism_store_memAddJob(&job);
               }
            }
         }

         // Check disk space for compaction
         if (!ismStore_memGlobal.fCompactDiskAlertOn && pct > ismStore_memGlobal.CompactDiskHWM)
         {
            TRACE(5, "Store disk space usage (%.2f) reached the compaction high water mark (%u). DiskUsedSpaceBytes %lu, DiskFreeSpaceBytes %lu\n",
                  pct, ismStore_memGlobal.CompactDiskHWM, diskStats.DiskUsedSpaceBytes, diskStats.DiskFreeSpaceBytes);
            ismStore_memGlobal.fCompactDiskAlertOn = 1;
         }
         else if (ismStore_memGlobal.fCompactDiskAlertOn && pct < ismStore_memGlobal.CompactDiskLWM)
         {
            TRACE(5, "Store disk space usage (%.2f) returned to compaction normal (%u). DiskUsedSpaceBytes %lu, DiskFreeSpaceBytes %lu\n",
                  pct, ismStore_memGlobal.CompactDiskLWM, diskStats.DiskUsedSpaceBytes, diskStats.DiskFreeSpaceBytes);
            ismStore_memGlobal.fCompactDiskAlertOn = 0;
         }

         // check disk usage versus total memory
         if (!ismStore_memGlobal.fCompactDiskAlertOn)
         {
            if (!ismStore_memGlobal.fCompactMemAlertOn && diskStats.GensUsedSpaceBytes > ismStore_memGlobal.CompactMemBytesHWM)
            {
               TRACE(5, "Store disk space usage (%lu bytes) reached the memory compaction high water mark (%lu bytes)\n",
                     diskStats.GensUsedSpaceBytes, ismStore_memGlobal.CompactMemBytesHWM);
               ismStore_memGlobal.fCompactMemAlertOn = 1;
            }
            else if (ismStore_memGlobal.fCompactMemAlertOn && diskStats.GensUsedSpaceBytes < ismStore_memGlobal.CompactMemBytesLWM)
            {
               TRACE(5, "Store disk space usage (%lu bytes) returned to memory compaction normal (%lu bytes)\n",
                   diskStats.GensUsedSpaceBytes, ismStore_memGlobal.CompactMemBytesLWM);
               ismStore_memGlobal.fCompactMemAlertOn = 0;
            }
         }

         if (ismStore_memGlobal.fCompactDiskAlertOn)
         {
            ism_store_memCompactTopNGens(ismSTORE_TOPN_COMPACT_DISK_GENS, 0, diskStats.DiskUsedSpaceBytes);
         }
         else if (ismStore_memGlobal.fCompactMemAlertOn)
         {
            ism_store_memCompactTopNGens(ismSTORE_TOPN_COMPACT_MEM_GENS, 2, diskStats.GensUsedSpaceBytes);
         }
      }
      else
      {
         TRACE(1, "Failed to check store disk space usage, because of the disk statistics information are not valid. DiskUsedSpaceBytes %lu, DiskFreeSpaceBytes %lu\n",
               diskStats.DiskUsedSpaceBytes, diskStats.DiskFreeSpaceBytes);
      }
   }
   else
   {
      TRACE(1, "Failed to check store disk space usage, because of a disk statistics failure. error code %d\n", ec);
   }

   TRACE(9, "Exit: %s\n", __FUNCTION__);
}
/****************** END OF Store Disk Operations *********************/

static void ism_store_memRaiseUserEvent(ismStore_EventType_t eventType)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memDescriptor_t *pDescriptor;
   ismStore_memGranulePool_t *pPool;
   int dataType, i;
  #define MAP_SIZE 9
   uint32_t recMap[MAP_SIZE], granulesCount;
   uint64_t offset, tail;
   double pctMap[MAP_SIZE];

   TRACE(9, "Entry: %s. Event %d\n", __FUNCTION__, eventType);

   if (eventType == ISM_STORE_EVENT_MGMT0_ALERT_ON)
   {
      // Scan the management and create a map of records
      // We don't take the StreamsMutex intentionally
      memset(recMap, '\0', sizeof(recMap));
      memset(pctMap, '\0', sizeof(pctMap));
      pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
      pPool = &pMgmtHeader->GranulePool[ismSTORE_MGMT_SMALL_POOL_INDEX];
      for (offset = pPool->Offset, tail = pPool->Offset + pPool->MaxMemSizeBytes, granulesCount = 0; offset < tail; offset += pPool->GranuleSizeBytes, granulesCount++)
      {
         pDescriptor = (ismStore_memDescriptor_t *)(ismStore_memGlobal.MgmtGen.pBaseAddress + offset);
         dataType = (pDescriptor->DataType & (~ismSTORE_DATATYPE_NOT_PRIMARY));
         if (dataType == ISM_STORE_RECTYPE_CLIENT) {
            recMap[0]++;
         } else if (dataType == ISM_STORE_RECTYPE_QUEUE) {
            recMap[1]++;
         } else if (dataType == ISM_STORE_RECTYPE_TOPIC) {
            recMap[2]++;
         } else if (dataType == ISM_STORE_RECTYPE_SUBSC) {
            recMap[3]++;
         } else if (dataType == ISM_STORE_RECTYPE_TRANS) {
            recMap[4]++;
         } else if (dataType == ISM_STORE_RECTYPE_BMGR) {
            recMap[5]++;
         } else if (dataType == ISM_STORE_RECTYPE_REMSRV) {
            recMap[6]++;
         } else if (dataType == ismSTORE_DATATYPE_FREE_GRANULE || dataType == 0) {
            recMap[7]++;
         } else {
            recMap[8]++;
         }
      }

      if (granulesCount > 0)
      {
         for (i=0; i < MAP_SIZE; i++)
         {
            pctMap[i] = ((double)recMap[i] / granulesCount) * 100;
         }
      }

      TRACE(5, "Map of the memory pool %d of generation Id %u: CLIENT %u (%.1f%%), QUEUE %u (%.1f%%), TOPIC %u (%.1f%%), " \
            "SUBSC %u (%.1f%%), TRANS %u (%.1f%%), BMGR %u (%.1f%%), REMSRV %u (%.1f%%), FREE %u (%.1f%%), OTHER %u (%.1f%%)\n",
            ismSTORE_MGMT_SMALL_POOL_INDEX, ismSTORE_MGMT_GEN_ID,
            recMap[0], pctMap[0], recMap[1], pctMap[1], recMap[2], pctMap[2],
            recMap[3], pctMap[3], recMap[4], pctMap[4], recMap[5], pctMap[5],
            recMap[6], pctMap[6], recMap[7], pctMap[7], recMap[8], pctMap[8]);
   }
   else if (eventType == ISM_STORE_EVENT_MGMT1_ALERT_ON)
   {
      // Scan the management and create a map of records
      // We don't take the StreamsMutex intentionally
      memset(recMap, '\0', sizeof(recMap));
      memset(pctMap, '\0', sizeof(pctMap));
      pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.MgmtGen.pBaseAddress;
      pPool = &pMgmtHeader->GranulePool[ismSTORE_MGMT_POOL_INDEX];
      for (offset = pPool->Offset, tail = pPool->Offset + pPool->MaxMemSizeBytes, granulesCount = 0; offset < tail; offset += pPool->GranuleSizeBytes, granulesCount++)
      {
         pDescriptor = (ismStore_memDescriptor_t *)(ismStore_memGlobal.MgmtGen.pBaseAddress + offset);
         dataType = (pDescriptor->DataType & (~ismSTORE_DATATYPE_NOT_PRIMARY));

         if (dataType == ISM_STORE_RECTYPE_SERVER) {
            recMap[0]++;
         } else if (dataType == ismSTORE_DATATYPE_GEN_IDS) {
            recMap[1]++;
         } else if (dataType == ismSTORE_DATATYPE_STORE_TRAN) {
            recMap[2]++;
         } else if (dataType == ismSTORE_DATATYPE_REFSTATES) {
            recMap[3]++;
         } else if (dataType == ismSTORE_DATATYPE_STATES) {
            recMap[4]++;
         } else if (dataType == ismSTORE_DATATYPE_FREE_GRANULE || dataType == 0) {
            recMap[5]++;
         } else {
            recMap[6]++;
         }
      }

      if (granulesCount > 0)
      {
         for (i=0; i < 7; i++)
         {
            pctMap[i] = ((double)recMap[i] / granulesCount) * 100;
         }
      }

      TRACE(5, "Map of the memory pool %d of generation Id %u: SERVER %u (%.1f%%), GEN_IDS %u (%.1f%%), "\
            "STORE_TRAN %u (%.1f%%), REFSTATES %u (%.1f%%), STATES %u (%.1f%%), FREE %u (%.1f%%), OTHER %u (%.1f%%)\n",
            ismSTORE_MGMT_POOL_INDEX, ismSTORE_MGMT_GEN_ID, recMap[0], pctMap[0],
            recMap[1], pctMap[1], recMap[2], pctMap[2], recMap[3], pctMap[3],
            recMap[4], pctMap[4], recMap[5], pctMap[5], recMap[6], pctMap[6]);
   }

   if (ismStore_memGlobal.OnEvent)
   {
      ismStore_memGlobal.OnEvent(eventType, ismStore_memGlobal.pEventContext);
   }

   TRACE(9, "Exit: %s\n", __FUNCTION__);
}

void ism_store_memAddJob(ismStore_memJob_t *pJob)
{
   ismStore_memJob_t *pNewJob;

   pthread_mutex_lock(&ismStore_memGlobal.ThreadMutex);
   if (ismStore_memGlobal.fThreadGoOn)
   {
      if ((pNewJob = (ismStore_memJob_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_store_misc,150),sizeof(ismStore_memJob_t))) != NULL)
      {
         memcpy(pNewJob, pJob, sizeof(*pNewJob));
         pNewJob->pNextJob = NULL;
         if (ismStore_memGlobal.pThreadLastJob)
         {
            ismStore_memGlobal.pThreadLastJob->pNextJob = pNewJob;
         }
         else
         {
            ismStore_memGlobal.pThreadFirstJob = pNewJob;
         }
         ismStore_memGlobal.pThreadLastJob = pNewJob;
         pthread_cond_signal(&ismStore_memGlobal.ThreadCond);
         TRACE(9, "Store job (Type %u, GenId %u, Index %u) has been added.\n", pJob->JobType, pJob->Generation.GenId, pJob->Generation.GenIndex);
      }
      else
      {
         TRACE(1, "Failed to add a store job (Type %u, GenId %u, Index %u) due to a memory allocation error\n", pJob->JobType, pJob->Generation.GenId, pJob->Generation.GenIndex);
      }
   }
   pthread_mutex_unlock(&ismStore_memGlobal.ThreadMutex);
}

/*
 * Implement the store thread.
 *
 * @param arg     Not used
 * @param context Not used
 * @param value   Not used
 * @return NULL
 */
static void *ism_store_memThread(void *arg, void *pContext, int value)
{
   ismStore_memHAInfo_t *pHAInfo = &ismStore_memGlobal.HAInfo;
   ismStore_memMgmtHeader_t *pMgmtHeader = NULL;
   ismStore_memHAAck_t ack;
   ismStore_memJob_t *pJob;
   int ec, i;
   char threadName[64];
   struct timespec reltime;
   ism_time_t currtime=0, nexttime=0, nextmttime=0, nexthealth=0;
  #if USE_REFSTATS
   ism_time_t RSfreq, RSnext;
   ismStore_memRefStats_t  lRS[2];
  #endif

   memset(threadName, '\0', sizeof(threadName));
   ism_common_getThreadName(threadName, 64);
   TRACE(5, "The %s thread is started\n", threadName);

   ismStore_memGlobal.fThreadUp = 1;
   memset(&reltime, '\0', sizeof(struct timespec));
   reltime.tv_nsec = 10000000;

  #if USE_REFSTATS
   RSfreq = 10 ; 
   RSfreq *= 1000000000 ; 
   RSnext = ism_common_monotonicTimeNanos() + 6 * RSfreq ; 
  #endif

   ism_engine_threadInit(ISM_ENGINE_NO_STORE_STREAM);
   while (1)
   {
      ism_common_backHome();
      pthread_mutex_lock(&ismStore_memGlobal.ThreadMutex);
      currtime = ism_common_monotonicTimeNanos();
      while (!(pJob = ismStore_memGlobal.pThreadFirstJob) && ismStore_memGlobal.fThreadGoOn)
      {
         if (currtime > nexttime)
         {
            nexttime = currtime + 10000000;
            break;
         }
         else
         {
            pthread_mutex_unlock(&ismStore_memGlobal.ThreadMutex);
            ism_common_sleep(1000);
            pthread_mutex_lock(&ismStore_memGlobal.ThreadMutex);
         }
            
         ism_common_cond_timedwait(&ismStore_memGlobal.ThreadCond, &ismStore_memGlobal.ThreadMutex, &reltime, 1);
         currtime = ism_common_monotonicTimeNanos();
      }
      ism_common_going2work();
      // We want to give the thread a chance to complete all its pending jobs, before it terminates
      if (pJob)
      {
         TRACE(9, "The next job of the %s thread is %d for generation Id %u (Index %u)\n",
               threadName, pJob->JobType, pJob->Generation.GenId, pJob->Generation.GenIndex);
         if (!(ismStore_memGlobal.pThreadFirstJob = pJob->pNextJob))
         {
            ismStore_memGlobal.pThreadLastJob = NULL;
         }
         pthread_mutex_unlock(&ismStore_memGlobal.ThreadMutex);

         if (pJob->JobType <= StoreJob_LastJobId)   /* BEAM suppression: constant condition */
         {
            ismStore_memGlobal.ThreadJobsHistory[pJob->JobType]++;
         }

         switch (pJob->JobType)
         {
            case StoreJob_CreateGeneration:
               ism_store_memCreateGeneration(pJob->Generation.GenIndex, ismSTORE_RSRV_GEN_ID);
               break;
            case StoreJob_ActivateGeneration:
               ism_store_memActivateGeneration(pJob->Generation.GenId, pJob->Generation.GenIndex);
               break;
            case StoreJob_WriteGeneration:
               ism_store_memWriteGeneration(pJob->Generation.GenId, pJob->Generation.GenIndex);
               break;
            case StoreJob_CompactGeneration:
               ism_store_memCompactGeneration(pJob->Generation.GenId, 2, 0);
               break;
            case StoreJob_DeleteGeneration:
               ism_store_memDeleteGeneration(pJob->Generation.GenId);
               break;
            case StoreJob_CheckDiskUsage:
               ism_store_memCheckDiskUsage();
               nextmttime = currtime + (60 * 1000000000L);
               break;
            case StoreJob_InitRsrvPool:
               ism_store_memInitRsrvPool();
               break;
            case StoreJob_UserEvent:
               ism_store_memRaiseUserEvent(pJob->Event.EventType);
               break;
            case StoreJob_IncRefGenPool:
               ism_store_memIncreaseRefGenPool(pJob->RefGenPool.pRefGenPool);
               break;
            case StoreJob_DecRefGenPool:
               ism_store_memDecreaseRefGenPool(pJob->RefGenPool.pRefGenPool);
               break;
            case StoreJob_HASendMinActiveOid:
               if (ismSTORE_HASSTANDBY && pHAInfo->pIntChannel)
               {
                  if ((ec = ism_store_memHASendActiveOid(pHAInfo->pIntChannel, pJob->UpdOrderId.OwnerHandle, pJob->UpdOrderId.MinActiveOid)) != StoreRC_OK)
                  {
                     if (ec == StoreRC_HA_ConnectionBroke)
                     {
                        TRACE(9, "Failed to send the MinActiveOrderId %lu of owner (Handle 0x%lx) to the Standby node because the connection broke\n",
                              pJob->UpdOrderId.MinActiveOid, pJob->UpdOrderId.OwnerHandle);
                     }
                     else
                     {
                        TRACE(1, "Failed to send the MinActiveOrderId %lu of owner (Handle 0x%lx) to the Standby node. error code %d\n",
                              pJob->UpdOrderId.MinActiveOid, pJob->UpdOrderId.OwnerHandle, ec);
                     }
                  }
               }
               if (ismStore_memGlobal.fEnablePersist && (ec = ism_storePersist_writeActiveOid(pJob->UpdOrderId.OwnerHandle, pJob->UpdOrderId.MinActiveOid)) != StoreRC_OK)
               {
                  TRACE(1, "Failed to write the MinActiveOrderId %lu of owner (Handle 0x%lx) to the Persist ST file. error code %d\n",
                        pJob->UpdOrderId.MinActiveOid, pJob->UpdOrderId.OwnerHandle, ec);
               }
               break;
            case StoreJob_HAViewChanged:
            case StoreJob_HAStandbyJoined:
            case StoreJob_HAStandbyLeft:
            case StoreJob_HAStandby2Primary:
               ism_store_memHandleHAEvent(pJob);
               break;
            default:
               TRACE(1, "The job type %d of the %s thread is not valid. Generation Id %u (Index %u)\n",
                     pJob->JobType, threadName, pJob->Generation.GenId, pJob->Generation.GenIndex);
         }
         TRACE(9, "The job %d of the %s thread has been completed. Generation Id %u (Index %u)\n",
               pJob->JobType, threadName, pJob->Generation.GenId, pJob->Generation.GenIndex);
         ismSTORE_FREE(pJob);
      }
      else
      {
         if (!ismStore_memGlobal.fThreadGoOn)
         {
            break;
         }

         pthread_mutex_unlock(&ismStore_memGlobal.ThreadMutex);

         if (ismStore_memGlobal.State == ismSTORE_STATE_ACTIVE && (!ismStore_global.fHAEnabled || pHAInfo->State == ismSTORE_HA_STATE_PRIMARY))
         {
            if (pMgmtHeader)
            {
               pMgmtHeader->PrimaryTime = ism_common_currentTimeNanos();
               ADR_WRITE_BACK(&pMgmtHeader->PrimaryTime, sizeof(pMgmtHeader->PrimaryTime));
            }
            else
            {
               pMgmtHeader = (ismStore_memMgmtHeader_t *)ismStore_memGlobal.pStoreBaseAddress;
            }

            if ( ismStore_memGlobal.fEnablePersist)
              ism_store_memManageCoolPool() ; 

            // Maintenance task once in 1 minute
            if (currtime > nextmttime)
            {
               if ( ismStore_memGlobal.dStreams && ismStore_memGlobal.fEnablePersist )
                  ism_store_freeDeadStreams(0);
               ism_store_memCheckDiskUsage();
               nextmttime = currtime + (60 * 1000000000L);
            }

           #if USE_REFSTATS
            if (currtime > RSnext )
            {
              ismStore_memRefStats_t *pRS, *tRS ; 
              RSnext = currtime + RSfreq ; 
              memset(lRS,0,2*sizeof(ismStore_memRefStats_t));
              pthread_mutex_lock(&ismStore_memGlobal.CtxtMutex);
              for ( i=0 ; i<ismStore_memGlobal.RefCtxtLocksCount ; i++ )
              {
                ismSTORE_MUTEX_LOCK(ismStore_memGlobal.pRefCtxtMutex[i]) ; 
                pRS = ismStore_memGlobal.pRefGenPool[i].RefStats ; 
                tRS = lRS ; 
                if ( pRS->nCL )
                {
                  tRS->nCL += pRS->nCL;
                  tRS->nBH += pRS->nBH;
                  tRS->nAT += pRS->nAT;
                  tRS->nTL += pRS->nTL;
                  tRS->nAC += pRS->nAC;
                  tRS->nAF += pRS->nAF;
                  tRS->nUF += pRS->nUF;
                  tRS->nBT += pRS->nBT;
                  tRS->nLL += pRS->nLL;
                  tRS->nEL += pRS->nEL;
                  memset(pRS,0,sizeof(*pRS));
                }
                pRS++ ; 
                tRS++ ; 
                if ( pRS->nCL )
                {
                  tRS->nCL += pRS->nCL;
                  tRS->nBH += pRS->nBH;
                  tRS->nAT += pRS->nAT;
                  tRS->nTL += pRS->nTL;
                  tRS->nAC += pRS->nAC;
                  tRS->nAF += pRS->nAF;
                  tRS->nUF += pRS->nUF;
                  tRS->nLL += pRS->nLL;
                  tRS->nEL += pRS->nEL;
                  tRS->b[0]+= pRS->b[0];
                  tRS->b[1]+= pRS->b[1];
                  tRS->b[2]+= pRS->b[2];
                  tRS->b[3]+= pRS->b[3];
                  tRS->b[4]+= pRS->b[4];
                  memset(pRS,0,sizeof(*pRS));
                }
                ismSTORE_MUTEX_UNLOCK(ismStore_memGlobal.pRefCtxtMutex[i]) ; 
              }
              pthread_mutex_unlock(&ismStore_memGlobal.CtxtMutex);
              pRS = lRS ; 
              if ( pRS->nCL )
              {
                TRACE(5,"REFSTATS_InMemRef: Calls=%u, BeforeHead=%u, AfterTail=%u, AtTail=%u, AtCache=%u, CacheElements=%u, LinkListSearch=%u, LinkListElements=%u\n",
                                            pRS->nCL, pRS->nBH,      pRS->nAT,     pRS->nTL,  pRS->nAC,   pRS->nBT,      pRS->nLL,          pRS->nEL);
              }
              pRS++ ; 
              if ( pRS->nCL )
              {
                TRACE(5,"REFSTATS_RefState: Calls=%u, BeforeHead=%u, AfterTail=%u, AtTail=%u, AtCache=%u, AtFinger=%u, UseFinger=%u, LinkListSearch=%u, LinkListElements=%u, bad=%u %u %u %u %u\n",
                                         pRS->nCL, pRS->nBH,pRS->nAT,pRS->nTL,pRS->nAC,pRS->nAF,pRS->nUF,pRS->nLL,pRS->nEL,pRS->b[0],pRS->b[1],pRS->b[2],pRS->b[3],pRS->b[4]);
              }
            }
           #endif
         }

         if (ismSTORE_HASSTANDBY && pHAInfo->pIntChannel)
         {
            for (i=0; i < 10; i++)
            {
               memset(&ack, '\0', sizeof(ack));
               ec = ism_store_memHAReceiveAck(pHAInfo->pIntChannel, &ack, 1);
               if (ec == StoreRC_OK)
               {
                  ism_store_memHandleHAIntAck(&ack);
               }
               else
               {
                  if (ec != StoreRC_HA_WouldBlock)
                  {
                     // The internal HA channel has been broken. Could not use it in the current view
                     TRACE(9, "The HA internal channel has been broken. Could not use it in the current view (ViewId %u)\n", pHAInfo->View.ViewId);
                     pHAInfo->pIntChannel = NULL;
                  }
                  break;
               }
            }
         }
      }
      if (currtime > nexthealth)
      {
         i = ism_common_check_health() ? 1 : 10 ; 
         nexthealth = currtime + (i * 1000000000L);
      }
   }

   while ((pJob = (ismStore_memJob_t *)ismStore_memGlobal.pThreadFirstJob))  /* BEAM suppression: loop doesn't iterate */
   {
      if (!(ismStore_memGlobal.pThreadFirstJob = pJob->pNextJob))
      {
         ismStore_memGlobal.pThreadLastJob = NULL;
      }
      ismSTORE_FREE(pJob);
   }
   ismStore_memGlobal.fThreadUp = 0;
   pthread_mutex_unlock(&ismStore_memGlobal.ThreadMutex);

   TRACE(5, "Jobs history for %s thread: StoreJob_NONE=%u, StoreJob_CreateGeneration=%u, StoreJob_ActivateGeneration=%u, " \
            "StoreJob_WriteGeneration=%u, StoreJob_DeleteGeneration=%u, StoreJob_CompactGeneration=%u, " \
            "StoreJob_CheckDiskUsage=%u, StoreJob_InitRsrvPool=%u, StoreJob_UserEvent=%u, " \
            "StoreJob_IncRefGenPool=%u, StoreJob_DecRefGenPool=%u, " \
            "StoreJob_HASendMinActiveOid=%u, StoreJob_HAViewChanged=%u, StoreJob_HAStandbyJoined=%u, " \
            "StoreJob_HAStandbyLeft=%u, StoreJob_HAStandby2Primary=%u\n",
            threadName, ismStore_memGlobal.ThreadJobsHistory[0],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_CreateGeneration],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_ActivateGeneration],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_WriteGeneration],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_DeleteGeneration],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_CompactGeneration],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_CheckDiskUsage],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_InitRsrvPool],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_UserEvent],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_IncRefGenPool],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_DecRefGenPool],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_HASendMinActiveOid],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_HAViewChanged],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_HAStandbyJoined],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_HAStandbyLeft],
            ismStore_memGlobal.ThreadJobsHistory[StoreJob_HAStandby2Primary]);

   ism_engine_threadTerm(0);
   TRACE(5, "The %s thread is stopped\n", threadName);
   //ism_common_endThread(NULL);
   return NULL;
}

static int32_t ism_store_memCheckStoreVersion(uint64_t v1, uint64_t v2)
{
  int i;
  uint64_t v[2]={v1,v2}, u;

  for ( i=0 ; i<2 ; i++ )
  {
    ldiv_t d;
    u = v[i] ; 
    d = ldiv(u,100) ; 
    if ( d.rem < 1 || d.rem > 31 ) goto err_exit ; 
    u = d.quot ; 
    d = ldiv(u,100) ; 
    if ( d.rem < 1 || d.rem > 12 ) goto err_exit ; 
    u = d.quot ; 
    if ( u < 2012 ) goto err_exit ; 
  }
  if ( v1 > v2 )
  {
    TRACE(1,"Store version mismach: store_version > code_version: %lu > %lu\n",v1,v2);
    return 1 ; 
  }
  return 0 ; 
 err_exit:
  TRACE(1,"Invalid store version found: %lu\n",v[i]);
  return -1 ; 
}


static int32_t ism_store_memManageCoolPool(void)
{
   ismStore_memMgmtHeader_t *pMgmtHeader;
   ismStore_memGenHeader_t *pGenHeader;
   ismStore_memGeneration_t *pGen;
   ismStore_memGranulePool_t *pPool;
   ismStore_memDescriptor_t *pDescriptor=NULL;
   ismStore_Handle_t h, ph;
   int i, j;
   uint32_t n;
   double minTime, *v;
   int32_t rc = ISMRC_OK;


   minTime = ism_storePersist_getTimeStamp() ; 

   for ( i=0 ; i<2 ; i++ )
   {
      if ( i )
      {
         pGen = &ismStore_memGlobal.InMemGens[pMgmtHeader->ActiveGenIndex];
      }
      else
      {
         pGen = &ismStore_memGlobal.MgmtGen;
         pMgmtHeader = (ismStore_memMgmtHeader_t *)pGen->pBaseAddress;
      }
      pGenHeader = (ismStore_memGenHeader_t *)pGen->pBaseAddress;
      for (j=0; j < pGenHeader->PoolsCount; j++)
      {
         pPool = &pGen->CoolPool[j];
         pthread_mutex_lock(&pGen->PoolMutex[j]) ; 
         if ( pPool->GranuleCount )
         {
            for ( n=0, ph=ismSTORE_NULL_HANDLE, h=pPool->hHead ; h!=ismSTORE_NULL_HANDLE ; n++, ph=h, h=pDescriptor->NextHandle )
            {
               pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(h));
               v = (double *)&pDescriptor->Attribute ; 
               if ( *v > minTime )
                  break ; 
            }
            if ( n )
            {
               int m;
               ismStore_Handle_t hHead, hTail;
               hHead = pPool->hHead ;
               hTail = ph ; 
               pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(ph));
               pDescriptor->NextHandle = ismSTORE_NULL_HANDLE ; 
               pPool->hHead = h ;
               pPool->GranuleCount -= n ; 
               if (!pPool->GranuleCount)
                  pPool->hTail = ismSTORE_NULL_HANDLE;
               m = pPool->GranuleCount ; 

               pPool = &((ismStore_memGenHeader_t *)pGen->pBaseAddress)->GranulePool[j];
               if (pPool->hTail)
               {
                  pDescriptor = (ismStore_memDescriptor_t *)(pGen->pBaseAddress + ismSTORE_EXTRACT_OFFSET(pPool->hTail));
                  pDescriptor->NextHandle = hHead ; 
                  ADR_WRITE_BACK(&pDescriptor->NextHandle, sizeof(pDescriptor->NextHandle));
               }
               else
               {
                  pPool->hHead = hHead;
               }
               pPool->hTail = hTail;
               pPool->GranuleCount += n;
            
               TRACE(8," %u elements have been returned to pool %d of generation %u, new Count %u, remaining in CoolPool %u\n",n,j,pGenHeader->GenId,pPool->GranuleCount,m);
            }
         }
         pthread_mutex_unlock(&pGen->PoolMutex[j]) ; 
      }
   }
   return rc ; 
}

/*********************************************************************/
/*                                                                   */
/* End of storeMemory.c                                              */
/*                                                                   */
/*********************************************************************/
