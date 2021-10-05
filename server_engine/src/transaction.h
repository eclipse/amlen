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
/// @file transaction.h
/// @brief functions for using trsanctions
///
/// Defines functions and data types for manipulating transactions
//****************************************************************************
#ifndef __ISM_ENGINE_TRANSACTION_DEFINED
#define __ISM_ENGINE_TRANSACTION_DEFINED

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <pthread.h>

#include "engineInternal.h"
#include "engineAsync.h"

#define ietrINITIAL_TRANSACTION_CAPACITY  61

#define ietrTRAN_FLAG_NONE                 0x0000
#define ietrTRAN_FLAG_REHYDRATED           0x0001
#define ietrTRAN_FLAG_SUSPENDED            0x0002
#define ietrTRAN_FLAG_IN_GLOBAL_TABLE      0x0004
#define ietrTRAN_FLAG_GLOBAL               0x0100
#define ietrTRAN_FLAG_PERSISTENT           0x0200

// What stage of completion (commit or rollback) the transaction is at
#define ietrCOMPLETION_NONE             0
#define ietrCOMPLETION_STARTED          1
#define ietrCOMPLETION_ENDED            2

// Maximum store operations expected for the commit or rollback of a single SLE in
// a non fAsStoreTran and fAsStoreTran transactions, respectively. The non-fAsStoreTran
// values are used in calculating when a transaction needs to commit incrementally.

// If any SLE needs a higher value then these should simply be increased to the maximum,
// but this will have a knock-on effect to the number of incremental transaction commits
// which perform more store commits.
#define ietrMAX_COMMIT_STORE_OPS               1
#define ietrMAX_AS_STORETRAN_COMMIT_STORE_OPS  2
#define ietrMAX_ROLLBACK_STORE_OPS             2
#define ietrMAX_AS_STORTRAN_ROLLBACK_STORE_OPS 2

#define ietrMAX_TRANSACTION_STATE_VALUE ismTRANSACTION_STATE_HEURISTIC_ROLLBACK

typedef enum ietrReplayPhase_t
{
    None                 = 0x00000000,
    Commit               = 0x00000001,
    MemoryCommit         = 0x00000002, ///<Order is Commit, MemoryCommit, PostCommit
    PostCommit           = 0x00000004,
    Rollback             = 0x00000010,
    MemoryRollback       = 0x00000020, ///<Order is Rollback, MemoryRollback, PostRollback
    PostRollback         = 0x00000040,
    Cleanup              = 0x00000100,
    JobCallback          = 0x00000200,
    SavepointRollback    = 0x01000000,    ///<At the moment - assumes everything attempted under the save point is in a single store tran that is rolled back
}  ietrReplayPhase_t;

typedef enum  __attribute__ ((__packed__)) tag_ietrTranEntryType_t
{
    ietrSLE_IQ_PUT                      = 0x01,  ///< Put a message to intermediate queue
    ietrSLE_TT_OLD_STORE_SUBSC_DEFN     = 0x02,  ///< Persist subscription definition (for migration only)
    ietrSLE_TT_OLD_STORE_SUBSC_PROPS    = 0x03,  ///< Persist subscription properties (for migration only)
    ietrSLE_TT_ADDSUBSCRIPTION          = 0x04,  ///< Create subscription
    ietrSLE_TT_UPDATERETAINED           = 0x05,  ///< Update retained message on topic
    ietrSLE_CS_ADDUNRELMSG              = 0x06,  ///< Remember received but unreleased message
    ietrSLE_CS_RMVUNRELMSG              = 0x07,  ///< Forget unreleased message
    ietrSLE_SQ_PUT                      = 0x08,  ///< Put a message to simple queue
    ietrSLE_TT_RELEASENODES             = 0x09,  ///< Release nodes used during a publish (and updates statistics)
    ietrSLE_SM_ADDQMGRXID               = 0x0A,  ///< Add Queue-Manager XID
    ietrSLE_SM_RMVQMGRXID               = 0x0B,  ///< Remove Queue-Manager XID
    ietrSLE_MQ_PUT                      = 0x0C,  ///< Put to multi-consumer queue
    ietrSLE_MQ_CONSUME_MSG              = 0x0D,  ///< Remove message from multi-consumer queue
    ietrSLE_SAVEPOINT                   = 0x0E,  ///< Savepoint in a transaction
    ietrSLE_PREALLOCATED_MASK           = 0x80,  ///< Mask indicating that this SLE was allocated outside of the mempool
    ietrSLE_PREALLOCATED_MQ_CONSUME_MSG = 0x8D,  ///< Pre-allocated equivalent of ietrSLE_MQ_CONSUME_MSG
} ietrTranEntryType_t;

// The type passed to SLEs to identify the thread
typedef ieutThreadData_t *ietrJobThreadId_t;

#define ietrNO_JOB_THREAD 0

/// Record of activity recorded during replay of SLEs
typedef struct ietrReplayRecord_t
{
    uint32_t StoreOpCount;         ///< Count of store operations built up so far
    uint32_t SkippedPutCount;      ///< Count of (simpQ) puts that were skipped
    ietrJobThreadId_t JobThreadId; ///< The thread identifier of the thread on which we will attempt to schedule a JobCallback SLE phase
    bool JobRequired;              ///< Whether a JobCallback SLE phase is required
} ietrReplayRecord_t;

typedef struct tag_ietrAsyncTransactionData_t  ietrAsyncTransactionData_t; //forward declartion (lower in this file)

//An SLE Replay Function that won't go async
typedef void (*ietrSLESyncReplay_t)( ietrReplayPhase_t
                                   , ieutThreadData_t *
                                   , ismEngine_Transaction_t *
                                   , void *
                                   , ietrReplayRecord_t *);

//An SLE Replay Function that might go async
typedef int32_t (*ietrSLEAsyncReplay_t)( ietrReplayPhase_t
                                       , ieutThreadData_t *
                                       , ismEngine_Transaction_t *
                                       , void *
                                       , ietrReplayRecord_t *
                                       , ismEngine_AsyncData_t *
                                       , ietrAsyncTransactionData_t *);

typedef union tag_ietrSLEReplay_t
{
    ietrSLESyncReplay_t   syncFn;
    ietrSLEAsyncReplay_t  asyncFn;
} ietrSLEReplay_t;

typedef struct ietrSLE_Header_t 
{
    char                 StrucId[4];
    ietrTranEntryType_t  Type;
    uint8_t              CommitStoreOps;
    uint8_t              RollbackStoreOps;
    bool                 fSync;
    uint32_t             Phases;
    uint32_t             TotalLength;
    ietrSLEReplay_t      ReplayFn;
    void                *pNext;
    void                *pPrev;
} ietrSLE_Header_t;
#define ietrSLE_STRUC_ID   "TSLE"

typedef struct ietrSavepointDetail_t
{
    ismEngine_Transaction_t *pTran;
    bool active;
} ietrSavepointDetail_t;

typedef ietrSLE_Header_t ietrSavepoint_t;

typedef struct ietrStoreTranRef_t
{
    ismStore_Handle_t    hTranRef;
    uint64_t             orderId;
} ietrStoreTranRef_t;

typedef struct ietrTransactionControl_t
{
    char                    StrucId[4];
    uint32_t                StoreTranRsrvOps;  ///< Store transaction operations reservation size
    ieutHashTableHandle_t   GlobalTranTable;
    pthread_rwlock_t        GlobalTranLock;    ///< Lock on the global transaction table
} ietrTransactionControl_t;
#define ietrTRANCTL_STRUC_ID "TCTL"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ietrTransactionControl_t(__file)\
    iedm_descriptionStart(__file, ietrTransactionControl_t, StrucId, ietrTRANCTL_STRUC_ID);\
    iedm_describeMember(char [4],            StrucId);\
    iedm_describeMember(uint32_t,            StoreTranRsrvOps);\
    iedm_describeMember(ieutHashTable_t *,   GlobalTranTable);\
    iedm_describeMember(pthread_rwlock_t,    GlobalTranLock);\
    iedm_descriptionEnd;

// Define an ism_xid_t in the dump format
#define iedm_describe_ism_xid_t(__file)\
    iedm_descriptionStart(__file, ism_xid_t,,"");\
    iedm_describeMember(int32_t,     formatID);\
    iedm_describeMember(int32_t,     gtrid_length);\
    iedm_describeMember(int32_t,     bqual_length);\
    iedm_describeMember(char [128],  data);\
    iedm_descriptionEnd;

typedef struct ietrXIDIterator_t
{
    uint32_t arraySize;
    uint32_t arrayUsed;
    uint32_t cursor;
    ism_xid_t XIDArray[1];
} ietrXIDIterator_t;

#define ietrXID_CHUNK_SIZE 50


typedef void (*ietrCommitCompletionCallback_t)(
    ieutThreadData_t        *pThreadData,
    int32_t                  retcode,
    void *                   pContext);

//***********************************************************************
/// @brief Async Engine Transaction Data
///
/// Data about an transaction that needs to use store_commit_async
//***********************************************************************
struct tag_ietrAsyncTransactionData_t
{
    char                              StrucId[4];          ///< Eyecatcher "EATD"
    uint64_t                          asyncId;             ///< Used to tie up commit with callback
    ismEngine_Transaction_t          *pTran;               ///< transaction being committed
    ietrReplayPhase_t                 CurrPhase;           ///< phase of transaction in progress
    uint32_t                          ProcessedPhaseSLEs;  ///< number of SLEs we've processed
    ietrReplayRecord_t                Record;              ///< record of operations in this transaction
    bool                              fMarkedCommitOnly;   ///< Set if we've already marked the transaction commit only
    bool                              fRemovedTran;        ///< set if we've removed transaction object from the store
    iempMemPoolHandle_t               hMemPool;            ///< Memory pool from which data was allocated (or NULL if allocated separately)
    size_t                            memSize;             ///< Amount of the transaction memory allocated
    ietrCommitCompletionCallback_t    pCallbackFn;         ///< Call this function when completed with the custom data that
                                                           ///  was passed to ietr_commit (& which follows this structure) as context
    uint64_t                          parallelOperations;  ///< Count used to determine whether there are any parallel operations in progress
    ///NB: Transaction specific data usually follows this structure
};

#define ietrASYNCTRANSACTION_STRUCID "EATD"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ismEngine_AsyncTransactionData_t(__file)\
    iedm_descriptionStart(__file, ismEngine_AsyncTransactionData_t, StrucId, ietrASYNCTRANSACTION_STRUCID);\
    iedm_describeMember(char [4],                       StrucId);\
    iedm_describeMember(uint64_t,                       asyncId);\
    iedm_describeMember(ietrReplayPhase_t,              CurrPhase);\
    iedm_describeMember(uint32_t,                       ProcessedPhaseSLEs);\
    iedm_describeMember(ietrReplayRecord_t,             Record);\
    iedm_describeMember(bool,                           fMarkedCommitOnly);\
    iedm_describeMember(bool,                           fRemovedTran);\
    iedm_describeMember(iempMemPoolPageHeader_t *,      hMemPool);\
    iedm_describeMember(size_t,                         memSize);\
    iedm_describeMember(ietrCommitCompletionCallback_t, pCallbackFn);\
    iedm_describeMember(uint64_t,                       parallelOperations);\
    iedm_descriptionEnd;


//***********************************************************************
/// @brief Async heuristic commit Data
///
/// Data about an inflight heuristic commit
//***********************************************************************
typedef struct tag_ietrAsyncHeuristicCommitData_t
{
    char                              StrucId[4];               ///< Eyecatcher "ATHC"
    ismEngine_Transaction_t          *pTran;                    ///< Inflight Transaction
    void                             *engineCallerContext;      ///< Data supplied by caller to engine
    size_t                            engineCallerContextSize;  ///< size of above data
    ismEngine_CompletionCallback_t    pCallbackFn;              ///< callback for engine caller
} ietrAsyncHeuristicCommitData_t;

#define ietrASYNCHEURISTICCOMMIT_STRUCID "ATHC"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ietrAsyncHeuristicCommitData_t(__file)\
    iedm_descriptionStart(__file, ietrAsyncHeuristicCommitData_t, StrucId, ietrASYNCHEURISTICCOMMIT_STRUCID);\
    iedm_describeMember(char [4],                       StrucId);\
    iedm_describeMember(ismEngine_Transaction_t *,      pTran);\
    iedm_describeMember(void *,                         engineCallerContext);\
    iedm_describeMember(size_t,                         engineCallerContextSize);\
    iedm_describeMember(ismEngine_CompletionCallback_t, pCallbackFn);\
    iedm_descriptionEnd;

uint32_t ietr_initTransactions( ieutThreadData_t *pThreadData );
void ietr_destroyTransactions( ieutThreadData_t *pThreadData );
void ietr_dumpWriteDescriptions( iedmDumpHandle_t dumpHdl );
int32_t ietr_dumpTransactions( ieutThreadData_t *pThreadData
                             , const char *XIDString
                             , iedmDumpHandle_t dumpHdl );
int32_t ietr_createLocal( ieutThreadData_t *pThreadData
                        , ismEngine_Session_t *pSession
                        , bool fPersistent
                        , bool fAsStoreTran
                        , ismEngine_AsyncData_t *asyncData
                        , ismEngine_Transaction_t **ppTran );
int32_t ietr_createGlobal( ieutThreadData_t *pThreadData
                         , ismEngine_Session_t *pSession
                         , ism_xid_t *pXID
                         , uint32_t options
                         , ismEngine_AsyncData_t *asyncData
                         , ismEngine_Transaction_t **ppTran );
int32_t ietr_findGlobalTransaction( ieutThreadData_t *pThreadData
                                  , ism_xid_t *pXID
                                  , ismEngine_Transaction_t **ppTran );
void ietr_releaseTransaction( ieutThreadData_t *pThreadData
                            , ismEngine_Transaction_t *pTran );
int32_t ietr_checkForHeuristicCompletion( ismEngine_Transaction_t *pTran );
void ietr_freeSessionTransactionList( ieutThreadData_t *pThreadData
                                    , ismEngine_Session_t *pSession );
void ietr_freeClientTransactionList( ieutThreadData_t *pThreadData
                                   , ismEngine_ClientState_t *pClient );
int32_t ietr_rehydrate( ieutThreadData_t *pThreadData
                      , ismStore_Handle_t recHandle
                      , ismStore_Record_t *record
                      , ismEngine_RestartTransactionData_t *transData
                      , void **outData
                      , void *pContext );
int32_t ietr_completeRehydration( ieutThreadData_t *pThreadData
                                , uint64_t transactionHandle
                                , void *rehydratedTransaction
                                , void *pContext);
int32_t ietr_softLogAdd( ieutThreadData_t *pThreadData
                       , ismEngine_Transaction_t *pTran
                       , ietrTranEntryType_t TranType
                       , ietrSLESyncReplay_t SyncReplayFn
                       , ietrSLEAsyncReplay_t AsyncReplayFn
                       , uint32_t Phases
                       , void *pEntry
                       , uint32_t Length
                       , uint8_t CommitStoreOps
                       , uint8_t RollbackStoreOps
                       , ietrSLE_Header_t **ppSLE );
int32_t ietr_softLogAddPreAllocated( ieutThreadData_t *pThreadData
                                   , ismEngine_Transaction_t *pTran
                                   , ietrTranEntryType_t TranType
                                   , ietrSLESyncReplay_t SyncReplayFn
                                   , ietrSLEAsyncReplay_t AsyncReplayFn
                                   , uint32_t Phases
                                   , ietrSLE_Header_t *pSLE
                                   , uint8_t CommitStoreOps
                                   , uint8_t RollbackStoreOps );
int32_t ietr_softLogRehydrate( ieutThreadData_t *pThreadData
                             , ismEngine_RestartTransactionData_t *pTranData
                             , ietrTranEntryType_t TranType
                             , ietrSLESyncReplay_t SyncReplayFn
                             , ietrSLEAsyncReplay_t AsyncReplayFn
                             , uint32_t Phases
                             , void *pEntry
                             , uint32_t Length
                             , uint8_t CommitStoreOps
                             , uint8_t RollbackStoreOps
                             , ietrSLE_Header_t **ppSLE );
void ietr_softLogRemove( ieutThreadData_t *pThreadData
                       , ismEngine_Transaction_t *pTran
                       , ietrSLE_Header_t *pSLE );
int32_t ietr_prepare( ieutThreadData_t *pThreadData
                    , ismEngine_Transaction_t *pTran
                    , ismEngine_Session_t *pSession
                    , ismEngine_AsyncData_t *asyncData);
int32_t ietr_commit( ieutThreadData_t *pThreadData
                   , ismEngine_Transaction_t *pTran
                   , uint32_t options
                   , ismEngine_Session_t *pSession
                   , ietrAsyncTransactionDataHandle_t hAsyncData
                   , ietrCommitCompletionCallback_t  pCallbackFn);

int32_t ietr_rollback( ieutThreadData_t *pThreadData
                     , ismEngine_Transaction_t *pTran
                     , ismEngine_Session_t *pSession
                     , uint64_t options);
#define IETR_ROLLBACK_OPTIONS_NONE            0x0
#define IETR_ROLLBACK_OPTIONS_SESSIONCLOSE    0x1

int32_t ietr_complete( ieutThreadData_t *pThreadData
                     , ismEngine_Transaction_t *pTran
                     , ismTransactionState_t outcome
                     , ietrCommitCompletionCallback_t pCallbackFn
                     , ietrAsyncHeuristicCommitData_t *pAsyncData);
int32_t ietr_forget( ieutThreadData_t *pThreadData
                   , ismEngine_Transaction_t *pTran
                   , ismEngine_AsyncData_t *asyncData);
uint32_t ietr_XARecover( ieutThreadData_t *pThreadData
                       , ismEngine_SessionHandle_t hSession
                       , ism_xid_t *pXIDArray
                       , uint32_t arraySize
                       , uint32_t rmId
                       , uint32_t flags );
void ietr_markRollbackOnly( ieutThreadData_t *pThreadData
                          , ismEngine_Transaction_t *pTran );
uint32_t ietr_createTranRef( ieutThreadData_t *pThreadData
                           , ismEngine_Transaction_t *pTran
                           , ismStore_Handle_t hStoreHdl
                           , uint32_t Value
                           , uint8_t State 
                           , ietrStoreTranRef_t *pTranRef );
void ietr_deleteTranRef( ieutThreadData_t *pThreadData
                       , ismEngine_Transaction_t *pTran
                       , ietrStoreTranRef_t *pTranRef );
int32_t ietr_reserve( ieutThreadData_t *pThreadData
                    , ismEngine_Transaction_t *pTran
                    , size_t DataLength
                    , uint32_t numPut );
//If part way through a commit of an engine tran, something uses the engineAsync mechanism
//to go async in parallel with the transaction, this is the callback
int32_t ietr_asyncFinishParallelOperation(
        ieutThreadData_t               *pThreadData,
        int32_t                         retcode,
        ismEngine_AsyncData_t          *asyncInfo,
        ismEngine_AsyncDataEntry_t     *context);


ietrAsyncTransactionData_t *ietr_allocateAsyncTransactionData( ieutThreadData_t *pThreadData
                                                             , ismEngine_Transaction_t *pTran
                                                             , bool useMemReservedForCommit
                                                             , size_t customDataSize);
void *ietr_getCustomDataPtr(ietrAsyncTransactionData_t *pAsyncTranData);
void ietr_freeAsyncTransactionData( ieutThreadData_t *pThreadData
                                  , ietrAsyncTransactionData_t **ppAsyncTranData);
void ietr_unlinkTranSession(ieutThreadData_t *pThreadData,
                            ismEngine_Transaction_t *pTran);

void ietr_increasePreResolveCount(ieutThreadData_t *pThreadData
                                 , ismEngine_Transaction_t *pTran);

void ietr_decreasePreResolveCount( ieutThreadData_t *pThreadData
                                 , ismEngine_Transaction_t *pTran
                                 , bool issueResolveCallbacks);

int32_t ietr_beginSavepoint( ieutThreadData_t *pThreadData
                           , ismEngine_Transaction_t *pTran
                           , ietrSavepoint_t **ppSavepoint );
int32_t ietr_endSavepoint( ieutThreadData_t *pThreadData
                         , ismEngine_Transaction_t *pTran
                         , ietrSavepoint_t *pSavepoint
                         , uint32_t action );

#define ietr_recordLockScopeUsed(pTran) pTran->fLockManagerUsed = true

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_TRANSACTION_DEFINED */

/*********************************************************************/
/* End of transaction.h                                              */
/*********************************************************************/
