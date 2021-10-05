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
/* Module Name: storeInternal.h                                      */
/*                                                                   */
/* Description: Store component internal header file                 */
/*                                                                   */
/*********************************************************************/
#ifndef __ISM_STORE_INTERNAL_DEFINED
#define __ISM_STORE_INTERNAL_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include <pthread.h>          /* pthread header file                 */
#include <stdlib.h>           /* C standard library header file      */
#include <string.h>           /* String and memory header file       */
#include <stdint.h>           /* Standard integer defns header file  */
#include <limits.h>           /* System limits header file           */

#include <store.h>            /* Store external header file          */

/*********************************************************************/
/*                                                                   */
/* CONSTANT DEFINITIONS                                              */
/*                                                                   */
/*********************************************************************/

// Storage version
#define ismSTORE_VERSION                 20140101

// Data types of internal records
#define ismSTORE_DATATYPE_MIN_EXTERNAL     0x0001
#define ismSTORE_DATATYPE_MAX_EXTERNAL     0x3FFF

#define ismSTORE_DATATYPE_FREE_GRANULE     0x4000
#define ismSTORE_DATATYPE_NEWLY_HATCHED    0x4001
#define ismSTORE_DATATYPE_MGMT             0x4002
#define ismSTORE_DATATYPE_GENERATION       0x4003
#define ismSTORE_DATATYPE_GEN_IDS          0x4004
#define ismSTORE_DATATYPE_STORE_TRAN       0x4005
#define ismSTORE_DATATYPE_LDATA_EXT        0x4006
#define ismSTORE_DATATYPE_REFERENCES       0x4007
#define ismSTORE_DATATYPE_REFSTATES        0x4008
#define ismSTORE_DATATYPE_STATES           0x4009

#define ismSTORE_DATATYPE_NOT_PRIMARY      0x8000

// Storage is allocated in granules - records can span them
#define ismSTORE_MGMT_SMALL_POOL_INDEX          0
#define ismSTORE_MGMT_POOL_INDEX                1
#define ismSTORE_GRANULE_POOLS_COUNT            2

#define ismSTORE_MGMT_SMALL_POOL_PCT           50
#define ismSTORE_MGMT_POOL_PCT                 30

#define ismSTORE_BITMAPS_COUNT                  2
#define ismSTORE_BITMAP_LIVE_IDX                0
#define ismSTORE_BITMAP_FREE_IDX                1

#define ismSTORE_HANDLE_MASK     0xffffffffffffLL
#define ismSTORE_GEN_SHIFTCOUNT                48

#define ismSTORE_RSRV_GEN_ID                    0
#define ismSTORE_MGMT_GEN_ID                    1
#define ismSTORE_MAX_GEN_ID                 65000
#define ismSTORE_MAX_GEN_RESRV_PCT             75

#define ismSTORE_TOPN_COMPACT_DISK_GENS         5
#define ismSTORE_TOPN_COMPACT_MEM_GENS          1
#define ismSTORE_COMPACT_MEM_HWM               60
#define ismSTORE_COMPACT_MEM_LWM               50
#define ismSTORE_REFSTATES_CHUNKS             100

#define ismSTORE_THREAD_NAME               "store"

// Store configuration default values
#define ismSTORE_CFG_MEMTYPE_DV                 ismSTORE_MEMTYPE_SHM
#define ismSTORE_CFG_CACHEFLUSH_MODE_DV         ismSTORE_CACHEFLUSH_NORMAL
#define ismSTORE_CFG_NVRAM_OFFSET_DV            0
#define ismSTORE_CFG_COLD_START_DV              0
#define ismSTORE_CFG_RESTORE_DISK_DV            0
#define ismSTORE_CFG_BACKUP_DISK_DV             0
#define ismSTORE_CFG_MGMT_SMALL_GRANULE_ORG   256
#define ismSTORE_CFG_MGMT_GRANULE_SIZE_ORG   4096
#define ismSTORE_CFG_MGMT_SMALL_GRANULE_DV    128
#define ismSTORE_CFG_MGMT_GRANULE_SIZE_DV    1024
#define ismSTORE_CFG_GRANULE_SIZE_DV         1024
#define ismSTORE_CFG_TOTAL_MEMSIZE_MB_DV        0
#define ismSTORE_CFG_MGMT_MEM_PCT_DV           50
#define ismSTORE_CFG_MGMT_SMALL_PCT_DV         70
#define ismSTORE_CFG_INMEM_GENS_COUNT_DV        2
#define ismSTORE_CFG_DISK_BLOCK_SIZE_DV      (1<<25)
#define ismSTORE_CFG_DISK_CLEAR_DV              1
#define ismSTORE_CFG_DISK_ROOT_PATH_DV          "/tmp/com.ibm.ism"
#define ismSTORE_CFG_SHM_NAME_DV                "store"
#define ismSTORE_CFG_RECOVERY_MEMSIZE_MB_DV     0
#define ismSTORE_CFG_MGMT_ALERTON_PCT_DV       90
#define ismSTORE_CFG_MGMT_ALERTOFF_PCT_DV      80
#define ismSTORE_CFG_GEN_ALERTON_PCT_DV        99
#define ismSTORE_CFG_OWNER_LIMIT_PCT_DV        50
#define ismSTORE_CFG_DISK_ALERTON_PCT_DV       90
#define ismSTORE_CFG_DISK_ALERTOFF_PCT_DV      80
#define ismSTORE_CFG_STREAMS_CACHE_PCT_DV       5
#define ismSTORE_CFG_REFCTXT_LOCKS_COUNT_DV   100
#define ismSTORE_CFG_STATECTXT_LOCKS_COUNT_DV 100
#define ismSTORE_CFG_REFGENPOOL_EXT_DV      20000
#define ismSTORE_CFG_STORETRANS_RSRV_OPS_DV  1000
#define ismSTORE_CFG_COMPACT_MEMTH_MB_DV      100
#define ismSTORE_CFG_COMPACT_DISKTH_MB_DV     500
#define ismSTORE_CFG_COMPACT_DISKTH_PCT        75
#define ismSTORE_CFG_COMPACT_MEMTH_PCT         90
#define ismSTORE_CFG_COMPACT_DISK_HWM_DV       70
#define ismSTORE_CFG_COMPACT_DISK_LWM_DV       60
#define ismSTORE_CFG_DISK_ENABLEPERSIST_DV      0
#define ismSTORE_CFG_PERSIST_BUFF_SIZE_DV     (1<<20)
#define ismSTORE_CFG_PERSIST_FILE_SIZE_MB_DV    0
#define ismSTORE_CFG_DISK_PERSIST_PATH_DV     "/store/persist"
#define ismSTORE_CFG_PERSIST_REUSE_SHM_DV       1
#define ismSTORE_CFG_ASYNCCB_STATSMODE_DV       ISM_STORE_ASYNCCBSTATSMODE_ENABLED
#define ismSTORE_CFG_PERSIST_ANY_USER_IN_GROUP_DV   0
#define ismSTORE_CFG_PERSIST_THREAD_POLICY_DV   0
#define ismSTORE_CFG_PERSIST_ASYNC_THREADS_DV   2
#define ismSTORE_CFG_PERSIST_ASYNC_ORDERED_DV   1
#define ismHA_CFG_ENABLEHA_DV                   0
#define ismSTORE_CFG_PERSIST_HATX_THREADS_DV    2
#define ismSTORE_CFG_PERSIST_MAX_ASYNC_CB_DV  (1<<16)
#define ismSTORE_CFG_REFSEARCHCACHESIZE_DV      32

// Store configuration min/max values
#define ismSTORE_MIN_GRANULE_SIZE             128
#define ismSTORE_MAX_GRANULE_SIZE          262144
#define ismSTORE_MIN_INMEM_GENS                 2
#define ismSTORE_MAX_INMEM_GENS                 4
#define ismSTORE_MIN_MEMSIZE_MB                50
#define ismSTORE_MIN_GEN_MEMSIZE_MB            10
//#define ismSTORE_MIN_STREAMS_COUNT              4
#define ismSTORE_MIN_REFCTXT_LOCKS_COUNT       10
#define ismSTORE_MIN_STATECTXT_LOCKS_COUNT     10
#define ismSTORE_MIN_REFGENPOOL_EXT          1000
#define ismSTORE_MIN_COMPACT_TH_MB             10
#define ismSTORE_MIN_COMPACT_DISK_MB           50
#define ismSTORE_MIN_STREAMS_ARRAY_SIZE       512

#define ismSTORE_FIRSTREF_OFFSET  (sizeof(ismStore_memDescriptor_t) + sizeof(ismStore_memReferenceChunk_t) - sizeof(ismStore_memReference_t))

#define ismSTORE_NEED_DISK_COMPACT ((pGenMap->PredictedSizeBytes + ismStore_memGlobal.CompactDiskThBytes < pGenMap->PrevPredictedSizeBytes) || \
                                    (100*pGenMap->PredictedSizeBytes < ismSTORE_CFG_COMPACT_DISKTH_PCT*pGenMap->PrevPredictedSizeBytes))
#define ismSTORE_NEED_MEM_COMPACT  ((pGenMap->PredictedSizeBytes + ismStore_memGlobal.CompactMemThBytes < pGenMap->HARemoteSizeBytes) || \
                                    (100*pGenMap->PredictedSizeBytes < ismSTORE_CFG_COMPACT_MEMTH_PCT*pGenMap->HARemoteSizeBytes))

/*********************************************************************/
/*                                                                   */
/* MACRO DEFINITIONS                                                 */
/*                                                                   */
/*********************************************************************/
#define ismSTORE_ROUNDUP(x,y)              (((x) + (y) - 1) / (y) * (y))
#define ismSTORE_ROUND(x,y)                ((x) / (y) * (y))
#define ismSTORE_IS_SPLITITEM(x)  ((x) < ISM_STORE_RECTYPE_MAXOWNER && (x) >= ISM_STORE_RECTYPE_CLIENT)
#define ismSTORE_IS_HEADITEM(x)   \
(((x) & ismSTORE_DATATYPE_NOT_PRIMARY) == 0 && \
 (x) != ismSTORE_DATATYPE_FREE_GRANULE &&      \
 (x) != ismSTORE_DATATYPE_NEWLY_HATCHED &&     \
 (x) != ismSTORE_DATATYPE_STORE_TRAN &&        \
 (x) != 0)
#define ismSTORE_IS_FREEGRANULE(x)   \
((x) == ismSTORE_DATATYPE_FREE_GRANULE ||  \
 (x) == ismSTORE_DATATYPE_NEWLY_HATCHED || \
 (x) == 0)
#define ismSTORE_BUILD_HANDLE(x,y)         (((ismStore_Handle_t)(x) << ismSTORE_GEN_SHIFTCOUNT) | (ismStore_Handle_t)(y))
#define ismSTORE_EXTRACT_OFFSET(x)         ((ismStore_Handle_t)(x) & ismSTORE_HANDLE_MASK)
#define ismSTORE_EXTRACT_GENID(x)          ((ismStore_GenId_t)((ismStore_Handle_t)(x) >> ismSTORE_GEN_SHIFTCOUNT))
#define ismSTORE_SET_ERROR(x)              if (x && x != ISMRC_AsyncCompletion && x != ISMRC_BadPropertyValue && x != ISMRC_ArgNotValid && x != ISMRC_StoreHABadConfigValue && x != ISMRC_StoreHABadNicConfig && x != ISMRC_StoreBufferTooSmall && x != ISMRC_StoreNoMoreEntries) { ism_common_setError(rc); }
#define ismSTORE_FREE(x)                   if (x != NULL) { ism_common_free(ism_memory_store_misc,x); x = NULL; }
#define ismSTORE_FREE_BASIC(x)             if (x != NULL) { free(x); x = NULL; }
#define ismSTORE_CLOSE_FD(x)               if (x > 0) { close(x); }

/*********************************************************************/
/*                                                                   */
/* TYPE DEFINITIONS                                                  */
/*                                                                   */
/*********************************************************************/

typedef enum
{
   StoreRC_OK                     = 0,

   StoreRC_Disk_AlreadyOn         = 100,
   StoreRC_Disk_IsNotOn           = 101,
   StoreRC_Disk_TaskInterrupted   = 102,
   StoreRC_Disk_TaskCancelled     = 103,
   StoreRC_Disk_TaskExists        = 104,

   StoreRC_HA_ViewChanged         = 200,
   StoreRC_HA_MsgUnsent           = 201,
   StoreRC_HA_CloseChannel        = 202,
   StoreRC_HA_StoreTerm           = 203,
   StoreRC_HA_WouldBlock          = 204,
   StoreRC_HA_ConnectionBroke     = 205,

   StoreRC_prst_partialRecovery   = 300,

   StoreRC_BadParameter           = 1000,
   StoreRC_AllocateError          = 1001,
   StoreRC_BadNIC                 = 1002,
   StoreRC_InvalidCredentials     = 1003,

   StoreRC_SystemError            = 1100
} ismStore_RC_t;

/*
 * Generic interface to store
 */
typedef int32_t (*ismPSTORE_START)(void);
typedef int32_t (*ismPSTORE_TERM)(uint8_t fHAShutdown);
typedef int32_t (*ismPSTORE_DROP)(void);
typedef int32_t (*ismPSTORE_STOPCB)(void);
typedef int32_t (*ismPSTORE_REGISTEREVENTCALLBACK)(ismPSTOREEVENT callback, void *pContext);
typedef int32_t (*ismPSTORE_GETGENIDOFHANDLE)(ismStore_Handle_t handle, ismStore_GenId_t *pGenId);
typedef int32_t (*ismPSTORE_GETSTATISTICS)(ismStore_Statistics_t *pStatistics);
typedef int32_t (*ismPSTORE_OPENSTREAM)(ismStore_StreamHandle_t *phStream, uint8_t fHighPerf);
typedef int32_t (*ismPSTORE_CLOSESTREAM)(ismStore_StreamHandle_t hStream);
typedef int32_t (*ismPSTORE_RESERVESTREAMRESOURCES)(
                ismStore_StreamHandle_t hStream,
                ismStore_Reservation_t *pReservation);
typedef int32_t (*ismPSTORE_CANCELRESOURCERESERVATION)(ismStore_StreamHandle_t hStream);

typedef int32_t (*ismPSTORE_STARTTRANSACTION)(ismStore_StreamHandle_t hStream, int *fIsActive);
typedef int32_t (*ismPSTORE_CANCELTRANSACTION)(ismStore_StreamHandle_t hStream);
typedef int32_t (*ismPSTORE_GETASYNCCBSTATS)(uint32_t *pTotalReadyCBs, uint32_t *pTotalWaitingCBs,
                                             uint32_t *pNumThreads,
                                             ismStore_AsyncThreadCBStats_t *pCBThreadStats);

typedef int32_t (*ismPSTORE_GETSTREAMOPSCOUNT)(ismStore_StreamHandle_t hStream, uint32_t *pOpsCount);
typedef int32_t (*ismPSTORE_OPENREFERENCECONTEXT)(
                ismStore_Handle_t hOwnerHandle,
                ismStore_ReferenceStatistics_t *pRefStats,
                void **phRefCtxt);
typedef int32_t (*ismPSTORE_CLOSEREFERENCECONTEXT)(void *hRefCtxt);
typedef int32_t (*ismPSTORE_CLEARREFERENCECONTEXT)(
                ismStore_StreamHandle_t hStream,
                void *hRefCtxt);
typedef ismStore_Handle_t (*ismPSTORE_MAPREFERENCECONTEXTTOHANDLE)(void *hRefCtxt);
typedef int32_t (*ismPSTORE_OPENSTATECONTEXT)(
                ismStore_Handle_t hOwnerHandle,
                void **phStateCtxt);
typedef int32_t (*ismPSTORE_CLOSESTATECONTEXT)(void *hStateCtxt);
typedef ismStore_Handle_t (*ismPSTORE_MAPSTATECONTEXTTOHANDLE)(void *hStateCtxt);
typedef int32_t (*ismPSTORE_ENDSTORETRANSACTION)(ismStore_StreamHandle_t hStream, uint8_t fCommit,
                ismStore_CompletionCallback_t pCallback,
                void *pContext);
typedef int32_t (*ismPSTORE_ASSIGNRECORDALLOCATION)(
                ismStore_StreamHandle_t hStream,
                const ismStore_Record_t *pRecord,
                ismStore_Handle_t *pHandle);
typedef int32_t (*ismPSTORE_FREERECORDALLOCATION)(
                ismStore_StreamHandle_t hStream,
                ismStore_Handle_t handle);
typedef int32_t (*ismPSTORE_UPDATERECORD)(
                ismStore_StreamHandle_t hStream,
                ismStore_Handle_t handle,
                uint64_t attribute,
                uint64_t state,
                uint8_t flags);
typedef int32_t (*ismPSTORE_ADDREFERENCE)(
                ismStore_StreamHandle_t hStream,
                void *hRefCtxt,
                const ismStore_Reference_t *pReference,
                uint64_t minimumActiveOrderId,
                uint8_t fAutoCommit,
                ismStore_Handle_t *pHandle);
typedef int32_t (*ismPSTORE_UPDATEREFERENCE)(
                ismStore_StreamHandle_t hStream,
                void *hRefCtxt,
                ismStore_Handle_t handle,
                uint64_t orderId,
                uint8_t state,
                uint64_t minimumActiveOrderId,
                uint8_t fAutoCommit);
typedef int32_t (*ismPSTORE_DELETEREFERENCE)(
                ismStore_StreamHandle_t hStream,
                void *hRefCtxt,
                ismStore_Handle_t handle,
                uint64_t orderId,
                uint64_t minimumActiveOrderId,
                uint8_t fAutoCommit);
typedef int32_t (*ismPSTORE_PRUNEREFERENCES)(
                ismStore_StreamHandle_t hStream,
                void *hRefCtxt,
                uint64_t minimumActiveOrderId);
typedef int32_t (*ismPSTORE_ADDSTATE)(
                ismStore_StreamHandle_t hStream,
                void *hStateCtxt,
                const ismStore_StateObject_t *pState,
                ismStore_Handle_t *pHandle);
typedef int32_t (*ismPSTORE_DELETESTATE)(
                ismStore_StreamHandle_t hStream,
                void *hStateCtxt,
                ismStore_Handle_t handle);
typedef int32_t (*ismPSTORE_GETNEXTGENID)(
                ismStore_IteratorHandle *pIterator,
                ismStore_GenId_t *pGenId);
typedef int32_t (*ismPSTORE_READRECORD)(
                ismStore_Handle_t handle,
                ismStore_Record_t *pRecord,
                uint8_t fBlock);
typedef int32_t (*ismPSTORE_GETNEXTRECORDFORTYPE)(
                ismStore_IteratorHandle *pIterator,
                ismStore_RecordType_t recordType,
                ismStore_GenId_t genId,
                ismStore_Handle_t *pHandle,
                ismStore_Record_t *pRecord);
typedef int32_t (*ismPSTORE_READREFERENCE)(
                ismStore_Handle_t handle,
                ismStore_Reference_t *pRef);
typedef int32_t (*ismPSTORE_GETNEXTREFERENCEFOROWNER)(
                ismStore_IteratorHandle *pIterator,
                ismStore_Handle_t hOwnerHandle,
                ismStore_GenId_t genId,
                ismStore_Handle_t    *pHandle,
                ismStore_Reference_t *pReference);
typedef int32_t (*ismPSTORE_GETNEXTSTATEFOROWNER)(
                ismStore_IteratorHandle *pIterator,
                ismStore_Handle_t hOwnerHandle,
                ismStore_Handle_t    *pHandle,
                ismStore_StateObject_t *pState);
typedef int32_t (*ismPSTORE_RECOVERYCOMPLETED)(void);
typedef int32_t (*ismPSTORE_DUMP)(char *filename);
typedef int32_t (*ismPSTORE_DUMPCB)(ismPSTOREDUMPCALLBACK callback, void *pContext);

/*********************************************************************/
/*                                                                   */
/* STORE LOCK DEFINITIONS                                            */
/*                                                                   */
/*********************************************************************/
#define ismSTORE_SPINLOCK                  0

#if ismSTORE_SPINLOCK
   typedef struct {pthread_cond_t c[1] ; pthread_mutex_t m[1];} cond_mutex_t;
   #define ismSTORE_MUTEX_t                pthread_spinlock_t
   #define ismSTORE_COND_t                 cond_mutex_t
   #define ismSTORE_MUTEX_INIT(x)          pthread_spin_init(x,PTHREAD_PROCESS_PRIVATE)
   #define ismSTORE_COND_INIT(x)           (pthread_cond_init((x)->c,NULL),pthread_mutex_init((x)->m,NULL))
   #define ismSTORE_MUTEX_DESTROY(x)       pthread_spin_destroy(x)
   #define ismSTORE_COND_DESTROY(x)        (pthread_cond_destroy((x)->c),pthread_mutex_destroy((x)->m))
   #define ismSTORE_MUTEX_LOCK(x)          pthread_spin_lock(x)
//   static inline int ismSTORE_MUTEX_LOCK(ismSTORE_MUTEX_t *x) {while(pthread_spin_trylock(x)) sched_yield(); return 0;}
   #define ismSTORE_MUTEX_TRYLOCK(x)       pthread_spin_trylock(x)
   #define ismSTORE_MUTEX_UNLOCK(x)        pthread_spin_unlock(x)
   static inline int ismSTORE_COND_WAIT(ismSTORE_COND_t *x,ismSTORE_MUTEX_t *y) {int rc;pthread_mutex_lock((x)->m);pthread_spin_unlock(y);rc=pthread_cond_wait((x)->c,(x)->m);pthread_mutex_unlock((x)->m);while(pthread_spin_trylock(y)) sched_yield();return rc;}
   #define ismSTORE_COND_SIGNAL(x)         (pthread_mutex_lock((x)->m),pthread_cond_signal((x)->c),pthread_mutex_unlock((x)->m))
   #define ismSTORE_COND_BROADCAST(x)      (pthread_mutex_lock((x)->m),pthread_cond_broadcast((x)->c),pthread_mutex_unlock((x)->m))
#else
   #define ismSTORE_MUTEX_t                pthread_mutex_t
   #define ismSTORE_COND_t                 pthread_cond_t
   #define ismSTORE_MUTEX_INIT(x)          pthread_mutex_init(x, NULL)
   #define ismSTORE_COND_INIT(x)           pthread_cond_init(x, NULL)
   #define ismSTORE_MUTEX_DESTROY(x)       pthread_mutex_destroy(x)
   #define ismSTORE_COND_DESTROY(x)        pthread_cond_destroy(x)
   #define ismSTORE_MUTEX_TRYLOCK(x)       pthread_mutex_trylock(x)
   #define ismSTORE_MUTEX_LOCK(x)          pthread_mutex_lock(x)
   #define ismSTORE_MUTEX_UNLOCK(x)        pthread_mutex_unlock(x)
   #define ismSTORE_COND_WAIT(x,y)         pthread_cond_wait(x,y)
   #define ismSTORE_COND_SIGNAL(x)         pthread_cond_signal(x)
   #define ismSTORE_COND_BROADCAST(x)      pthread_cond_broadcast(x)
#endif

/*********************************************************************/
/* Global                                                            */
/*                                                                   */
/* Global data for the Store component.                              */
/* This is a singleton for the entire messaging server.              */
/*********************************************************************/
typedef struct ismStore_global_t
{
    char                                  StrucId[4];            /* 'SGBL' */
    uint8_t                               MemType;
    uint8_t                               CacheFlushMode;
    uint8_t                               ColdStartMode;
    uint8_t                               fRestoreFromDisk;
    uint8_t                               fClearStoredFiles;
    uint8_t                               fHAEnabled;
    uint8_t                               fSyncFailed;
    uint8_t                               fRestarting;
    uint64_t                              MachineMemoryBytes;
    uint64_t                              PrimaryMemSizeBytes;


    ismPSTORE_START                       pFnStart;
    ismPSTORE_TERM                        pFnTerm;
    ismPSTORE_DROP                        pFnDrop;
    ismPSTORE_REGISTEREVENTCALLBACK       pFnRegisterEventCallback;
    ismPSTORE_GETGENIDOFHANDLE            pFnGetGenIdOfHandle;
    ismPSTORE_GETSTATISTICS               pFnGetStatistics;
    ismPSTORE_OPENSTREAM                  pFnOpenStream;
    ismPSTORE_CLOSESTREAM                 pFnCloseStream;
    ismPSTORE_RESERVESTREAMRESOURCES      pFnReserveStreamResources;
    ismPSTORE_CANCELRESOURCERESERVATION   pFnCancelResourceReservation;
    ismPSTORE_GETSTREAMOPSCOUNT           pFnGetStreamOpsCount;
    ismPSTORE_OPENREFERENCECONTEXT        pFnOpenReferenceContext;
    ismPSTORE_CLOSEREFERENCECONTEXT       pFnCloseReferenceContext;
    ismPSTORE_CLEARREFERENCECONTEXT       pFnClearReferenceContext;
    ismPSTORE_MAPREFERENCECONTEXTTOHANDLE pFnMapReferenceContextToHandle;
    ismPSTORE_OPENSTATECONTEXT            pFnOpenStateContext;
    ismPSTORE_CLOSESTATECONTEXT           pFnCloseStateContext;
    ismPSTORE_MAPSTATECONTEXTTOHANDLE     pFnMapStateContextToHandle;
    ismPSTORE_ENDSTORETRANSACTION         pFnEndStoreTransaction;
    ismPSTORE_ASSIGNRECORDALLOCATION      pFnAssignRecordAllocation;
    ismPSTORE_FREERECORDALLOCATION        pFnFreeRecordAllocation;
    ismPSTORE_UPDATERECORD                pFnUpdateRecord;
    ismPSTORE_ADDREFERENCE                pFnAddReference;
    ismPSTORE_UPDATEREFERENCE             pFnUpdateReference;
    ismPSTORE_DELETEREFERENCE             pFnDeleteReference;
    ismPSTORE_PRUNEREFERENCES             pFnPruneReferences;
    ismPSTORE_ADDSTATE                    pFnAddState;
    ismPSTORE_DELETESTATE                 pFnDeleteState;
    ismPSTORE_GETNEXTGENID                pFnGetNextGenId;
    ismPSTORE_READRECORD                  pFnReadRecord;
    ismPSTORE_GETNEXTRECORDFORTYPE        pFnGetNextRecordForType;
    ismPSTORE_READREFERENCE               pFnReadReference;
    ismPSTORE_GETNEXTREFERENCEFOROWNER    pFnGetNextReferenceForOwner;
    ismPSTORE_GETNEXTSTATEFOROWNER        pFnGetNextStateForOwner;
    ismPSTORE_RECOVERYCOMPLETED           pFnRecoveryCompleted;
    ismPSTORE_DUMP                        pFnDump;
    ismPSTORE_DUMPCB                      pFnDumpCB;
    ismPSTORE_STOPCB                      pFnStopCB;
    ismPSTORE_STARTTRANSACTION            pFnStartTransaction;
    ismPSTORE_CANCELTRANSACTION           pFnCancelTransaction;
    ismPSTORE_GETASYNCCBSTATS             pFnGetAsyncCBStats;

} ismStore_global_t;

#define ismSTORE_GLOBAL_STRUCID "SGBL"


/*********************************************************************/
/* Internal function prototypes                                      */
/*********************************************************************/

// storeMemory.c
int32_t ism_store_memInit(void);

#endif /* __ISM_STORE_INTERNAL_DEFINED */

/*********************************************************************/
/* End of storeInternal.h                                            */
/*********************************************************************/
