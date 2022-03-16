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

///////////////////////////////////////////////////////////////////////////////
///
///  @file  transaction.c
///
///  @brief 
///    Code for transaction support in the engine
///
///  @remarks
///    @see The header file engineInternal.h contains the private
///    data declarations for this source file.
///    
///////////////////////////////////////////////////////////////////////////////
#define TRACE_COMP Engine

#include <stddef.h>
#include <assert.h>
#include <stdint.h>

#include "engineInternal.h"
#include "engineDump.h"
#include "engineStore.h"
#include "transaction.h"
#include "queueCommon.h"
#include "multiConsumerQ.h"
#include "intermediateQ.h"
#include "simpQ.h"
#include "topicTree.h"
#include "clientState.h"
#include "lockManager.h"
#include "storeMQRecords.h"
#include "engineHashTable.h"
#include "mempool.h"
#include "engineRestore.h"
#include "engineUtils.h"
#include "threadJobs.h"

///////////////////////////////////////////////////////////////////////////////
// Local function prototypes - see definition for more info 
///////////////////////////////////////////////////////////////////////////////
static int32_t ietr_softLogCommit( ietrTransactionControl_t *pControl
                                 , ismEngine_Transaction_t *pTran
                                 , ismEngine_AsyncData_t *pAsyncData
                                 , ietrAsyncTransactionData_t *pAsyncTranData
                                 , ietrReplayRecord_t *pRecord
                                 , ieutThreadData_t *pThreadData
                                 , ietrReplayPhase_t Phase );
static void ietr_softLogRollback( ietrTransactionControl_t *pControl
                                , ismEngine_Transaction_t *pTran
                                , ietrReplayRecord_t *pRecord
                                , ieutThreadData_t *pThreadData
                                , ietrReplayPhase_t Phase );
static uint32_t ietr_genHashId(ism_xid_t *pXID);
static int32_t ietr_addJobCallback(ieutThreadData_t *pThreadData,
                                   ismEngine_Transaction_t *pTran,
                                   ietrAsyncTransactionData_t *pAsyncTranData);

// The amount we need to reserve for use during commit has been calculated by saying its:
// base (48)  + extra engine (<100) + protocol (<200) + extra for future changes:
#define TRANSACTION_MEMPOOL_RESERVEDMEMBYTES      800
#define TRANSACTION_MEMPOOL_INITIALPAGEBYTES     4000
#define TRANSACTION_MEMPOOL_SUBSEQUENTPAGEBYTES 20400

///////////////////////////////////////////////////////////////////////////////
///  @brief 
///     Transaction Control Initialise
///  @remark
///     This function initialises the control block for transactions
///////////////////////////////////////////////////////////////////////////////
uint32_t ietr_initTransactions( ieutThreadData_t *pThreadData )
{
    int32_t rc = OK;
    int osrc;
    ietrTransactionControl_t *pControl;
    pthread_rwlockattr_t rwlockattr_init;

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(ismEngine_serverGlobal.TranControl == NULL);

    osrc = pthread_rwlockattr_init(&rwlockattr_init);

    if (osrc) goto mod_exit;

    osrc = pthread_rwlockattr_setkind_np(&rwlockattr_init, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);

    if (osrc) goto mod_exit;

    pControl = iemem_calloc(pThreadData
                           , IEMEM_PROBE(iemem_localTransactions, 1)
                           , 1
                           , sizeof(ietrTransactionControl_t));
    if (pControl == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    ismEngine_SetStructId(pControl->StrucId, ietrTRANCTL_STRUC_ID);    

    osrc = pthread_rwlock_init(&pControl->GlobalTranLock, &rwlockattr_init);

    if (osrc) goto mod_exit;

    rc = ieut_createHashTable( pThreadData
                             , ietrINITIAL_TRANSACTION_CAPACITY
                             , iemem_globalTransactions
                             , &pControl->GlobalTranTable);

    if (rc != OK) goto mod_exit;

    ismStore_Statistics_t storeStats = {0};

    rc = ism_store_getStatistics(&storeStats);

    if (rc != OK) goto mod_exit;

    pControl->StoreTranRsrvOps = storeStats.StoreTransRsrvOps;
    ismEngine_serverGlobal.TranControl = pControl;

mod_exit:

    if (osrc)
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief 
///     Transaction Control Destroy
///  @remark
///     This function terminates the control block for transactions
///////////////////////////////////////////////////////////////////////////////
void ietr_destroyTransactions( ieutThreadData_t *pThreadData )
{
    ietrTransactionControl_t *pControl = (ietrTransactionControl_t *)ismEngine_serverGlobal.TranControl;

    ieutTRACEL(pThreadData, pControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (pControl != NULL)
    {
        ismEngine_serverGlobal.TranControl = NULL;

        assert(pControl != NULL);
        ismEngine_CheckStructId(pControl->StrucId, ietrTRANCTL_STRUC_ID, ieutPROBE_015);

        ieut_destroyHashTable(pThreadData, pControl->GlobalTranTable);

        (void)pthread_rwlock_destroy(&pControl->GlobalTranLock);

        iemem_freeStruct(pThreadData, iemem_localTransactions, pControl, pControl->StrucId);
    }

    ieutTRACEL(pThreadData, 0, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);
    return;
}

void ietr_linkTranSession(ieutThreadData_t *pThreadData,
                          ismEngine_Session_t *pSession,
                          ismEngine_Transaction_t *pTran)
{
    (void)ism_engine_lockSession(pSession);
    pTran->pSession = pSession;

    pTran->pNext = pSession->pTransactionHead;
    pSession->pTransactionHead = pTran;
    (void)ism_engine_unlockSession(pSession);
}

void ietr_unlinkTranSession(ieutThreadData_t *pThreadData,
                            ismEngine_Transaction_t *pTran)
{
     if (pTran->pSession != NULL)
     {
         // This transaction was bound to a session, so ensure it is unlinked.
         ismEngine_Session_t *pSession  = pTran->pSession;
         bool found = false;

         (void)ism_engine_lockSession(pSession);

         if (pSession->pTransactionHead == pTran)
         {
             found = true;
             pSession->pTransactionHead = pTran->pNext;
         }
         else if (pSession->pTransactionHead != NULL)
         {
             ismEngine_Transaction_t *pTranPrev = pSession->pTransactionHead;
             while ((pTranPrev != NULL) && (pTranPrev->pNext != pTran))
             {
                 pTranPrev = pTranPrev->pNext;
             }

             if ((pTranPrev != NULL) && (pTranPrev->pNext == pTran))
             {
                 found = true;
                 pTranPrev->pNext = pTran->pNext;
             }
         }

         (void)ism_engine_unlockSession(pSession);

         if (!found)
         {
             ieutTRACE_FFDC( ieutPROBE_001, false
                           , "Transaction with link to session not found in session list.", ISMRC_Failure
                           , "Transaction", pTran, sizeof(ismEngine_Transaction_t)
                           , "Session", pSession, sizeof(ismEngine_Session_t)
                           , NULL );
         }

         pTran->pNext = NULL;
         __sync_lock_release(&pTran->pSession);
         assert(pTran->pSession == NULL);
     }
}

int32_t ietr_completeCreateLocal( ieutThreadData_t *pThreadData
                                , ismEngine_Session_t *pSession
                                , ismEngine_Transaction_t *pTran
                                , uint32_t createTime)
{
    ieutTRACEL(pThreadData, pTran,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "pTran=%p\n", __func__, pTran);

    int32_t rc = 0;

    if ((pTran->TranFlags & ietrTRAN_FLAG_PERSISTENT) == ietrTRAN_FLAG_PERSISTENT && !(pTran->fAsStoreTran))
    {
        // Open a store reference context for this transaction
        rc = ism_store_openReferenceContext( pTran->hTran
                                           , NULL
                                           , &(pTran->pTranRefContext) );

        if (rc != OK)
        {
            ieutTRACE_FFDC( ieutPROBE_001, false
                          , "ism_store_openReferenceContext failed.", rc
                          , NULL );

            uint32_t rc2 = ism_store_deleteRecord( pThreadData->hStream
                                                 , pTran->hTran);
            if (rc2 == OK)
            {
                iest_store_commit(pThreadData, false);
            }

            goto mod_exit;
        }
    }

    pTran->TranState = ismTRANSACTION_STATE_IN_FLIGHT;
    pTran->StateChangedTime = ism_common_convertExpireToTime(createTime);

    // Optionally link the transaction into the list of transactions for
    // the session.  This means if the transaction is still active when
    // the session is destroyed the transaction will be rolled back.
    if (pSession)
    {
        ietr_linkTranSession(pThreadData, pSession, pTran);
    }

mod_exit:
    ieutTRACEL(pThreadData, pSession,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

typedef struct tag_ietrInflightCreateTran_t {
    ismEngine_Session_t *pSession;
    ismEngine_Transaction_t *pTran;
    uint32_t createTime;
} ietrInflightCreateTran_t;


static int32_t ietr_asyncCreateLocal(
        ieutThreadData_t               *pThreadData,
        int32_t                         rc,
        ismEngine_AsyncData_t          *asyncInfo,
        ismEngine_AsyncDataEntry_t     *context)
{
    assert(context->Type == TranCreateLocal);
    assert(rc == OK); //Store commit shouldn't fail (fatal fdc here?)

    ietrInflightCreateTran_t *tranData = (ietrInflightCreateTran_t *)(context->Data);

    //Pop this entry off the stack now so it doesn't get called again
    iead_popAsyncCallback( asyncInfo, context->DataLen);

    if (rc == OK)
    {
        rc = ietr_completeCreateLocal( pThreadData
                                     , tranData->pSession
                                     , tranData->pTran
                                     , tranData->createTime);
    }

    if (rc == OK)
    {
        iead_setEngineCallerHandle(asyncInfo, tranData->pTran);
    }
    else
    {
        ietr_unlinkTranSession(pThreadData, tranData->pTran);
        ietr_releaseTransaction(pThreadData, tranData->pTran);
    }

    //We increased the usecount on the session so we could link..undo that now
    if (tranData->pSession != NULL)
    {
        releaseSessionReference(pThreadData, tranData->pSession, false);
    }

    return rc;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Creates a local transaction
///  @remarks
///    This function allocates a transaction object, and creates the 
///    object in the store which will be used to store references to the
///    store updates made as part of this transaction.
///
///  @param[in] pSession           - Ptr to the session (optional)
///  @param[in] fPersistent        - TRUE if the persistent (store) updates are to be made
///  @param[in] fAsStoreTran       - TRUE if implemented as store tran
///  @param[out] ppTran            - Ptr to the transaction
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_createLocal( ieutThreadData_t *pThreadData
                        , ismEngine_Session_t *pSession
                        , bool fPersistent
                        , bool fAsStoreTran
                        , ismEngine_AsyncData_t *asyncData
                        , ismEngine_Transaction_t **ppTran )
{
    int32_t rc=0;
    ismEngine_Transaction_t *pTran;
    ismStore_Record_t storeTran;
    iestTransactionRecord_t tranRec;
    uint32_t dataLength;
    char *pFrags[1];
    uint32_t fragsLength[1];

    ieutTRACEL(pThreadData, pSession, ENGINE_FNC_TRACE, FUNCTION_ENTRY "fAsStoreTran=%s\n", __func__,
               fAsStoreTran?"true":"false");

    assert(pThreadData->ReservationState == Inactive);

    // We need to allocate a new memory object
    pTran = (ismEngine_Transaction_t *)iemem_calloc( pThreadData
                                                   , IEMEM_PROBE(iemem_localTransactions, 2), 1
                                                   , sizeof(ismEngine_Transaction_t));
    if (pTran == NULL)
    {
       rc = ISMRC_AllocateError;
       ism_common_setError(rc);
       goto mod_exit;
    }

    ismEngine_SetStructId(pTran->StrucId, ismENGINE_TRANSACTION_STRUCID);
    pTran->useCount = 1;
    pTran->pNext = NULL;

    rc = iemp_createMemPool( pThreadData
                           , IEMEM_PROBE(iemem_localTransactions, 6)
                           , TRANSACTION_MEMPOOL_RESERVEDMEMBYTES
                           , TRANSACTION_MEMPOOL_INITIALPAGEBYTES
                           , TRANSACTION_MEMPOOL_SUBSEQUENTPAGEBYTES
                           , &(pTran->hTranMemPool));
    if( rc != OK )
    {
        goto mod_exit;
    }

    rc = ielm_allocateLockScope(pThreadData, ielmLOCK_SCOPE_COMMIT_CAPABLE,
                                   pTran->hTranMemPool, &(pTran->hLockScope));
    if( rc != OK )
    {
        goto mod_exit;
    }

    assert(pTran->TranState == ismTRANSACTION_STATE_NONE);
    assert((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) == 0); // This is a local txn

    pTran->fRollbackOnly = false; // Clear and previous rollback only flag
    pTran->TranFlags |= fPersistent ? ietrTRAN_FLAG_PERSISTENT : ietrTRAN_FLAG_NONE;
    pTran->fIncremental = false;
    pTran->fLockManagerUsed = false;
    pTran->fDelayedRollback = false;
    pTran->preResolveCount = 1;
    pTran->nextOrderId = 1;
    pTran->fAsStoreTran = fAsStoreTran;
    pTran->fStoreTranPublish = false; //No fan out in progress at moment

    // Remember the time the transaction was created
    uint32_t now = ism_common_nowExpire();

    if (fPersistent)
    {
        if (fAsStoreTran)
        {
            pTran->hTran = ismSTORE_NULL_HANDLE;
            pTran->pTranRefContext = NULL;

            pTran->StoreRefReserve=0;
            pTran->StoreRefCount=0;

            pThreadData->ReservationState = Pending;
        }
        else
        {
            // Add the transaction to the store
            // We can do this before we take the queue put lock, as it is only
            // the message reference which we must add to the store in the 
            // context of the put lock in order to preserve message order.
            pFrags[0] = (char *)&tranRec;
            fragsLength[0] = sizeof(tranRec);
            dataLength = sizeof(tranRec);
            ismEngine_SetStructId(tranRec.StrucId, iestTRANSACTION_RECORD_STRUCID);
            tranRec.Version = iestTR_CURRENT_VERSION;
            tranRec.GlobalTran = false;
            tranRec.TransactionIdLength = 0;
            storeTran.Type = ISM_STORE_RECTYPE_TRANS;
            storeTran.FragsCount = 1;
            storeTran.pFrags = pFrags;
            storeTran.pFragsLengths = fragsLength;
            storeTran.DataLength = dataLength;
            storeTran.Attribute = 0; // Unused
            storeTran.State = ((uint64_t)now << 32) | ismTRANSACTION_STATE_IN_FLIGHT;

            rc = ism_store_createRecord( pThreadData->hStream
                                       , &storeTran
                                       , &pTran->hTran);
            if (rc != OK)
            {
                iest_store_rollback(pThreadData, false);
                goto mod_exit;
            }

            if (asyncData != NULL)
            {
                if (pSession != NULL)
                {
                    //We're going to link the transaction to the session, make
                    //sure the session exists when the callback completes
                    DEBUG_ONLY uint32_t oldCount = __sync_fetch_and_add( &(pSession->UseCount)
                                                                       , 1);
                    assert (oldCount > 0);
                }

                ietrInflightCreateTran_t tranInfo = {pSession, pTran, now};

                ismEngine_AsyncDataEntry_t newEntry =
                                               { ismENGINE_ASYNCDATAENTRY_STRUCID
                                               , TranCreateLocal
                                               , &tranInfo, sizeof(tranInfo)
                                               , NULL
                                               , {.internalFn = ietr_asyncCreateLocal}};
                iead_pushAsyncCallback(pThreadData, asyncData, &newEntry);

                rc = iead_store_asyncCommit(pThreadData, false, asyncData);

                if (rc == OK)
                {
                    //Don't need our callback
                    iead_popAsyncCallback(asyncData, newEntry.DataLen);

                    if (pSession != NULL)
                    {
                        releaseSessionReference(pThreadData, pSession, false);
                    }
                }
            }
            else
            {
                iest_store_commit(pThreadData, false);
            }
        }
    }

    if (rc == OK)
    {
        rc = ietr_completeCreateLocal( pThreadData
                                     , pSession
                                     , pTran
                                     , now);

        *ppTran = pTran;
    }

mod_exit:
    if (rc != OK && rc != ISMRC_AsyncCompletion)
    {
        if (pTran)
        {
            ietr_unlinkTranSession(pThreadData, pTran);
            ietr_releaseTransaction(pThreadData, pTran);
        }
    }

    ieutTRACEL(pThreadData, *ppTran,  ENGINE_FNC_TRACE, FUNCTION_EXIT "pTran=%p\n", __func__, *ppTran);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Find a global transaction with specified XID
///  @remarks
///    This function looks for the transaction with specified XID in the global
///    hash table.
///
///  @param[in] pXID               - XID to be located
///  @param[out] ppTran            - Ptr to the transaction
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_findGlobalTransaction(ieutThreadData_t *pThreadData,
                                   ism_xid_t *pXID,
                                   ismEngine_Transaction_t **ppTran)
{
    int32_t rc;
    ietrTransactionControl_t *pControl = (ietrTransactionControl_t *)ismEngine_serverGlobal.TranControl;
    ismEngine_Transaction_t *pTran = NULL;
    uint32_t XIDHashValue;
    char XIDBuffer[300]; // Maximum string size from ism_common_xidToString is 278
    const char *XIDString;

    // Generate the string representation of the XID
    XIDString = ism_common_xidToString(pXID, XIDBuffer, sizeof(XIDBuffer));

    assert(XIDString == XIDBuffer);

    // Generate the hash value for the XID
    XIDHashValue = ietr_genHashId(pXID);

    ieutTRACEL(pThreadData, XIDHashValue, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pXID=%p XIDString=\"%s\"\n", __func__,
               pXID, XIDString);

    ismEngine_getRWLockForRead(&pControl->GlobalTranLock);

    // Look for the transaction in the global transaction table
    rc = ieut_getHashEntry( pControl->GlobalTranTable
                          , XIDString
                          , XIDHashValue
                          , (void **)&pTran);

    if (rc == OK)
    {
        (void)__sync_fetch_and_add(&pTran->useCount, 1);
        *ppTran = pTran;
    }

    ismEngine_unlockRWLock(&pControl->GlobalTranLock);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d pTran=%p\n", __func__, rc, pTran);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Check if the specified transaction has been heuristically completed
///  @remarks
///    This function determines if the transaction is already in either a
///    heuristically committed or rolled back state.
///
///  @param[out] pTran            - Ptr to the transaction
///  @return                      - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_checkForHeuristicCompletion(ismEngine_Transaction_t *pTran)
{
    int32_t rc = OK;

    if ((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) == ietrTRAN_FLAG_GLOBAL)
    {
        if (pTran->TranState == ismTRANSACTION_STATE_HEURISTIC_COMMIT)
        {
            rc = ISMRC_HeuristicallyCommitted;
        }
        else if (pTran->TranState == ismTRANSACTION_STATE_HEURISTIC_ROLLBACK)
        {
            rc = ISMRC_HeuristicallyRolledBack;
        }
    }
    else
    {
        assert((pTran->TranState != ismTRANSACTION_STATE_HEURISTIC_COMMIT) &&
               (pTran->TranState != ismTRANSACTION_STATE_HEURISTIC_ROLLBACK));
    }

    return rc;
}

#define ietr_GLOBALTRAN_SIZE (sizeof(ismEngine_Transaction_t) + sizeof(ism_xid_t))

static int32_t ietr_completeCreateGlobal( ieutThreadData_t *pThreadData
                                        , ismEngine_Session_t *pSession
                                        , ismEngine_Transaction_t *pTran
                                        , uint32_t createTime)
{
    ieutTRACEL(pThreadData, pTran,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "pTran=%p\n", __func__, pTran);

    // Open a store reference context for this transaction
    int32_t rc = ism_store_openReferenceContext( pTran->hTran
                                               , NULL
                                               , &(pTran->pTranRefContext) );
    if (rc != OK)
    {
        ieutTRACE_FFDC( ieutPROBE_001, false
                        , "ism_store_openReferenceContext failed.", rc
                        , NULL );
        goto mod_exit;
    }

    pTran->TranState = ismTRANSACTION_STATE_IN_FLIGHT;
    pTran->StateChangedTime = ism_common_convertExpireToTime(createTime);

    // Associate the transaction to the session and vice-e-versa now that we know the
    // transaction has been successfully created
    ietr_linkTranSession(pThreadData, pSession, pTran);

mod_exit:

    // Need to remove any store resources that would otherwise be left behind
    if (rc != OK)
    {
        if (pTran->hTran != ismSTORE_NULL_HANDLE)
        {
            if (pTran->pTranRefContext != NULL)
            {
                (void)ism_store_closeReferenceContext(pTran->pTranRefContext);
                pTran->pTranRefContext = NULL;
            }

            if (ism_store_deleteRecord(pThreadData->hStream, pTran->hTran) == OK)
            {
                ism_store_commit(pThreadData->hStream);
            }
            pTran->hTran = ismSTORE_NULL_HANDLE;
        }
    }

    return rc;
}

static int32_t ietr_asyncCreateGlobal(
        ieutThreadData_t               *pThreadData,
        int32_t                         rc,
        ismEngine_AsyncData_t          *asyncInfo,
        ismEngine_AsyncDataEntry_t     *context)
{
    assert(context->Type == TranCreateGlobal);
    assert(rc == OK); //store commit shouldn't fail (fatal fdc here?)
    ietrInflightCreateTran_t *tranData = (ietrInflightCreateTran_t *)(context->Data);

    //Pop this entry off the stack now so it doesn't get called again
    iead_popAsyncCallback( asyncInfo, context->DataLen);

    if (rc == OK)
    {
        rc = ietr_completeCreateGlobal( pThreadData
                                      , tranData->pSession
                                      , tranData->pTran
                                      , tranData->createTime);
    }

    if (rc == OK)
    {
        iead_setEngineCallerHandle(asyncInfo, tranData->pTran);
    }
    else
    {
        ietr_unlinkTranSession(pThreadData, tranData->pTran);
        ietr_releaseTransaction(pThreadData, tranData->pTran);
    }

    //We increased the usecount on the session so we could link..undo that now
    if (tranData->pSession != NULL)
    {
        releaseSessionReference(pThreadData, tranData->pSession, false);
    }

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Creates a global transaction
///  @remarks
///    This function allocates a transaction object, and creates the 
///    object in the store which will be used to store references to the
///    store updates made as part of this transaction.
///
///  @param[in] pSession           - Ptr to the session    
///  @param[in] pXID               - XID 
///  @param[in] options            - ismENGINE_CREATE_TRANSACTION_OPTION values
///  @param[out] ppTran            - Ptr to the transaction
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////

int32_t ietr_createGlobal( ieutThreadData_t *pThreadData
                         , ismEngine_Session_t *pSession
                         , ism_xid_t *pXID
                         , uint32_t options
                         , ismEngine_AsyncData_t *asyncData
                         , ismEngine_Transaction_t **ppTran )
{
    int32_t rc=0;
    ismEngine_Transaction_t *pTran = NULL;
    ismStore_Record_t storeTran;
    iestTransactionRecord_t tranRec;
    uint32_t dataLength;
    char *pFrags[2];
    uint32_t fragsLength[2];
    bool bNeedRelease = false;

    ieutTRACEL(pThreadData, pSession,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pXID=%p options=0x%08x\n", __func__, pXID, options);

    // Explicit request to resume a transaction, it must be suspended on this session
    if ((options & ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMRESUME) != 0)
    {
        (void)ism_engine_lockSession(pSession);
        pTran = pSession->pTransactionHead;
        while(pTran != NULL)
        {
            const ism_xid_t *pTranXID = pTran->pXID;

            // Looks like this could be the transaction we're looking for
            if (pTranXID != NULL &&
                pTranXID->formatID == pXID->formatID &&
                pTranXID->gtrid_length == pXID->gtrid_length &&
                pTranXID->bqual_length == pXID->bqual_length)
            {
                // Yes - the data matches too, we've found our transaction
                if (memcmp(pTranXID->data, pXID->data, pTranXID->gtrid_length+pTranXID->bqual_length) == 0)
                {
                    break;
                }
            }

            pTran = pTran->pNext;
        }
        (void)ism_engine_unlockSession(pSession);

        // Didn't find the transaction specified
        if (pTran == NULL)
        {
            rc = ISMRC_NotFound;
            ism_common_setError(rc);
            goto mod_exit;
        }

        // Should be a suspended global
        assert((pTran->TranFlags & (ietrTRAN_FLAG_SUSPENDED | ietrTRAN_FLAG_GLOBAL)) == (ietrTRAN_FLAG_SUSPENDED | ietrTRAN_FLAG_GLOBAL));
    }
    // Not a resume request, look for the requested XID
    else
    {
        rc = ietr_findGlobalTransaction(pThreadData, pXID, &pTran);

        if (rc == OK) bNeedRelease = true;

        // Found the transaction, but were expecting not to
        if (rc == OK && options == ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS)
        {
            rc = ISMRC_TransactionInUse;
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    // We found an existing transaction, bind it to this session
    if (pTran != NULL)
    {
        // Bound to a client, we must unlink it first
        if (pTran->pClient != NULL)
        {
            iecs_unlinkTransaction( pTran->pClient, pTran);
        }

        if (pTran->pClient == NULL)
        {
            ismEngine_Session_t *Existing = __sync_val_compare_and_swap( &pTran->pSession, NULL, pSession);
            if (Existing == NULL)
            {
                // If the transaction was marked as suspended, it no longer is.
                pTran->TranFlags &= ~ietrTRAN_FLAG_SUSPENDED;

                // Associate the transaction with the session and vice-e-versa
                ietr_linkTranSession(pThreadData, pSession, pTran);

                ieutTRACEL(pThreadData, pTran, ENGINE_NORMAL_TRACE,
                           "Associating existing Transaction(%p) with our session %p\n", pTran, pSession );

                *ppTran = pTran;
            }
            else if (Existing == pSession)
            {
                // If the transaction was marked as suspended, it no longer is.
                pTran->TranFlags &= ~ietrTRAN_FLAG_SUSPENDED;

                ieutTRACEL(pThreadData, pTran, ENGINE_NORMAL_TRACE,
                           "Transaction(%p) already associated with our session %p\n", pTran, pSession );

                *ppTran = pTran;
            }
            else
            {
                // This transaction is already bound to another session so reject
                // the create transaction call.
                ieutTRACEL(pThreadData, Existing, ENGINE_NORMAL_TRACE,
                           "Transaction (%p) is currently bound to Session %p\n", pTran, Existing);

                rc = ISMRC_TransactionInUse;
                ism_common_setError(rc);
            }
        }
        else
        {
            ieutTRACEL(pThreadData, pTran->pClient, ENGINE_NORMAL_TRACE,
                       "Transaction (%p) is currently bound to Client %p\n", pTran, pTran->pClient);

            rc = ISMRC_TransactionInUse;
            ism_common_setError(rc);
        }

        // And we are done
        goto mod_exit;
    }
    // We did not find an existing transaction, create a new one
    else
    {
        ietrTransactionControl_t *pControl = (ietrTransactionControl_t *)ismEngine_serverGlobal.TranControl;

        //  We need to allocate a new memory object
        pTran = (ismEngine_Transaction_t *)iemem_calloc( pThreadData
                                                       , IEMEM_PROBE(iemem_globalTransactions, 1)
                                                       , 1
                                                       , ietr_GLOBALTRAN_SIZE );
        if (pTran == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        ismEngine_SetStructId(pTran->StrucId, ismENGINE_TRANSACTION_STRUCID);
        pTran->useCount = 1;
        pTran->TranFlags = ietrTRAN_FLAG_GLOBAL; // This is a global transaction
        pTran->pXID = (ism_xid_t *)(pTran+1);
        memcpy(pTran->pXID, pXID, sizeof(ism_xid_t));

        bNeedRelease = true;

        rc = iemp_createMemPool( pThreadData
                               , IEMEM_PROBE(iemem_globalTransactions, 7)
                               , TRANSACTION_MEMPOOL_RESERVEDMEMBYTES
                               , TRANSACTION_MEMPOOL_INITIALPAGEBYTES
                               , TRANSACTION_MEMPOOL_SUBSEQUENTPAGEBYTES
                               , &(pTran->hTranMemPool));

        if( rc != OK ) goto mod_exit;

        rc = ielm_allocateLockScope(pThreadData, ielmLOCK_SCOPE_COMMIT_CAPABLE,
                                              pTran->hTranMemPool, &(pTran->hLockScope));

        if( rc != OK ) goto mod_exit;

        char XIDBuffer[300]; // Maximum string size from ism_common_xidToString is 278
        // Generate the string representation of the XID
        const char *XIDString = ism_common_xidToString(pXID, XIDBuffer, sizeof(XIDBuffer));

        assert(XIDString == XIDBuffer);

        // Generate the hash value for the XID
        uint32_t XIDHashValue = ietr_genHashId(pXID);

        ismEngine_getRWLockForWrite(&pControl->GlobalTranLock);

        // We add the transaction to the hash table _before_ writing to the store to ensure
        // that if someone tries to use the same XID again, we won't be in the situation
        // where two store transaction records appear to represent the same transaction
        // XID - which recovery doesn't have a way to handle.
        rc = ieut_putHashEntry( pThreadData
                              , pControl->GlobalTranTable
                              , ieutFLAG_DUPLICATE_KEY_STRING
                              , XIDString
                              , XIDHashValue
                              , pTran
                              , ietr_GLOBALTRAN_SIZE );

        ismEngine_unlockRWLock(&pControl->GlobalTranLock);

        if( rc != OK ) goto mod_exit;

        pTran->TranFlags |= ietrTRAN_FLAG_IN_GLOBAL_TABLE;

        assert(pTran->TranState == ismTRANSACTION_STATE_NONE);
        pTran->fRollbackOnly = false;
        pTran->fIncremental = false;
        pTran->fLockManagerUsed = false;
        pTran->fDelayedRollback = false;
        pTran->preResolveCount = 1;
        pTran->nextOrderId = 1;
        pTran->fAsStoreTran = false;
        pTran->fStoreTranPublish = false; //No fan out in progress at moment
        pTran->TranFlags |= ietrTRAN_FLAG_PERSISTENT;

        // Remember the time the transaction was created
        uint32_t now = ism_common_nowExpire();

        // Add the transaction to the store
        pFrags[0] = (char *)&tranRec;
        fragsLength[0] = sizeof(tranRec);
        pFrags[1] = (char *)pXID;
        fragsLength[1] = sizeof(ism_xid_t);
        dataLength = sizeof(tranRec) + sizeof(ism_xid_t);
        ismEngine_SetStructId(tranRec.StrucId, iestTRANSACTION_RECORD_STRUCID);
        tranRec.Version = iestTR_CURRENT_VERSION;
        tranRec.GlobalTran = true;
        tranRec.TransactionIdLength = sizeof(ism_xid_t);
        storeTran.Type = ISM_STORE_RECTYPE_TRANS;
        storeTran.FragsCount = 2;
        storeTran.pFrags = pFrags;
        storeTran.pFragsLengths = fragsLength;
        storeTran.DataLength = dataLength;
        storeTran.Attribute = 0; // Unused
        storeTran.State = ((uint64_t)now << 32) | ismTRANSACTION_STATE_IN_FLIGHT;

        rc = ism_store_createRecord( pThreadData->hStream
                                   , &storeTran
                                   , &pTran->hTran);
        if (rc != OK)
        {
            iest_store_rollback(pThreadData, false);
            goto mod_exit;
        }

        bNeedRelease = false;

        if (asyncData != NULL)
        {
            if (pSession != NULL)
            {
                //We're going to link the transaction to the session, make
                //sure the session exists when the callback completes
                DEBUG_ONLY uint32_t oldCount = __sync_fetch_and_add( &(pSession->UseCount)
                                                                   , 1);
                assert (oldCount > 0);
            }

            ietrInflightCreateTran_t tranInfo = {pSession, pTran, now};
            ismEngine_AsyncDataEntry_t newEntry =
                        { ismENGINE_ASYNCDATAENTRY_STRUCID
                                , TranCreateGlobal
                                , &tranInfo, sizeof(tranInfo)
                                , NULL
                                , {.internalFn = ietr_asyncCreateGlobal}};
            iead_pushAsyncCallback(pThreadData, asyncData, &newEntry);

            rc = iead_store_asyncCommit(pThreadData, false, asyncData);
            assert(rc == OK || rc == ISMRC_AsyncCompletion);

            if (rc == OK)
            {
                //Don't need our callback
                iead_popAsyncCallback(asyncData, newEntry.DataLen);

                if (pSession != NULL)
                {
                    releaseSessionReference(pThreadData, pSession, false);
                }
            }
            else if (rc == ISMRC_AsyncCompletion)
            {
                assert(bNeedRelease == false);
                goto mod_exit;
            }
        }
        else
        {
            iest_store_commit(pThreadData, false);
        }

        if (rc == OK)
        {
            rc = ietr_completeCreateGlobal( pThreadData
                                          , pSession
                                          , pTran
                                          , now);

            if (rc == OK)
            {
                *ppTran = pTran;
            }
            else
            {
                assert(rc != ISMRC_AsyncCompletion);
                ietr_unlinkTranSession(pThreadData, pTran);
                bNeedRelease = true;
            }
        }
    }

mod_exit:

    if (bNeedRelease)
    {
        assert(pTran != NULL);
        ietr_releaseTransaction(pThreadData, pTran);
    }

    ieutTRACEL(pThreadData, *ppTran,  ENGINE_FNC_TRACE, FUNCTION_EXIT "pTran=%p\n", __func__, *ppTran);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Free the transactions and recovery scan associated with a session
///  @remarks
///    This function rolls back any transactions associated with the
///    session and frees the associated memory. It also frees the memory for
///    a recovery scan iterator if one still exists.
///
///  @param[out] pSession          - Ptr to the session
///////////////////////////////////////////////////////////////////////////////
void ietr_freeSessionTransactionList( ieutThreadData_t *pThreadData
                                    , ismEngine_Session_t *pSession )
{
    DEBUG_ONLY int32_t rc2;
    ismEngine_Transaction_t *pTran;
    uint32_t freed=0;

    ieutTRACEL(pThreadData, pSession, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pSession %p pSession->pTransactionHead %p\n",
               __func__, pSession, pSession->pTransactionHead);

    (void)ism_engine_lockSession(pSession);

    while (pSession->pTransactionHead != NULL)
    {
        pTran = pSession->pTransactionHead;
        pSession->pTransactionHead=pTran->pNext;

        // Unlink this transaction from the session which means it
        // that the rollback operation won;t have to maintain the 
        // session list of transactions
        assert(pTran->pSession == pSession);
        pTran->pSession = NULL;


        //Only fiddle with transactions that are not already completing
        if (pTran->CompletionStage == ietrCOMPLETION_NONE)
        {
            if (pTran->TranState == ismTRANSACTION_STATE_PREPARED)
            {
                // Nothing more for us to do for prepared global transactions
                // as we have already unlinked it from the session
                assert((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) == ietrTRAN_FLAG_GLOBAL);
            }
            else if (pTran->TranState == ismTRANSACTION_STATE_COMMIT_ONLY)
            {
                // we should never have a commit only transaction linked as
                // part of a session which is about to be destroyed, but if
                // we have then we ought to honour it.
                rc2 = ietr_commit( pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, pSession, NULL, NULL );
                assert(rc2 == OK);

                freed++;
            }
            else
            {
                // And rollback the transaction
                rc2 = ietr_rollback( pThreadData, pTran, pSession, IETR_ROLLBACK_OPTIONS_SESSIONCLOSE );
                assert(rc2 == OK);

                freed++;
            }
        }
    }

    pSession->pTransactionHead = NULL;
    (void)ism_engine_unlockSession(pSession);

    // End any active rescan
    ietr_XARecover(pThreadData, (ismEngine_SessionHandle_t)pSession, NULL, 0, 0, ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN);

    ieutTRACEL(pThreadData, freed,  ENGINE_FNC_TRACE, FUNCTION_EXIT "freed=%d\n", __func__, freed);

    return;
}
    
///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Free the unprepared global transactions associated with a client
///  @remarks
///    This function rolls back any transactions associated with the
///    client and frees the associated memory.
///
///  @param[out] pSession          - Ptr to the session
///////////////////////////////////////////////////////////////////////////////
void ietr_freeClientTransactionList( ieutThreadData_t *pThreadData
                                   , ismEngine_ClientState_t *pClient )
{
    DEBUG_ONLY int32_t rc2;
    ismEngine_Transaction_t *pTran;
    uint32_t freed=0;

    ieutTRACEL(pThreadData, pClient, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pClient %p pClient->pGlobalTransactions %p\n",
               __func__, pClient, pClient->pGlobalTransactions);

    while (pClient->pGlobalTransactions != NULL)
    {
        pTran = pClient->pGlobalTransactions;
        pClient->pGlobalTransactions = pTran->pNext;

        assert((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) == ietrTRAN_FLAG_GLOBAL);

        if (pTran->TranState == ismTRANSACTION_STATE_IN_FLIGHT)
        {
            // Don't want freeTransaction attempting to tidy up client state
            pTran->pClient = NULL;

            rc2 = ietr_rollback( pThreadData, pTran, NULL, IETR_ROLLBACK_OPTIONS_NONE );
            assert(rc2 == OK);

            freed++;
        }
        else
        {
            ieutTRACE_FFDC( ieutPROBE_020, false
                          , "Unexpected transaction state found.", ISMRC_Failure
                          , "TranState", pTran->TranState, sizeof(pTran->TranState)
                          , "Transaction", pTran, sizeof(ismEngine_Transaction_t)
                          , "Client", pClient, sizeof(ismEngine_ClientState_t)
                          , NULL );
        }
    }

    pClient->pGlobalTransactions = NULL;

    ieutTRACEL(pThreadData, freed,  ENGINE_FNC_TRACE, FUNCTION_EXIT "freed=%d\n", __func__, freed);

    return;
}
    
///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Rehydrates a transaction          
///  @remarks
///    This function allocates a transaction object during restart in order
///    to allow operations part of an uncommitted transaction when the 
///    engine ended can be committed or backed out correctly.
///
///  @param[out] ppTran            - Ptr to the transaction
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_rehydrate( ieutThreadData_t *pThreadData
                      , ismStore_Handle_t recHandle
                      , ismStore_Record_t *record
                      , ismEngine_RestartTransactionData_t *transData
                      , void **outData
                      , void *pContext )
{
    int32_t rc=0;
    ismEngine_Transaction_t *pTran = NULL;
    ietrTransactionControl_t *pControl = (ietrTransactionControl_t *)ismEngine_serverGlobal.TranControl;

    assert(transData == NULL);
    assert(pControl != NULL);

    assert(record->FragsCount == 1);

    ieutTRACEL(pThreadData, recHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    iestTransactionRecord_t *pTR = (iestTransactionRecord_t *)record->pFrags[0];
    ismEngine_CheckStructId(pTR->StrucId, iestTRANSACTION_RECORD_STRUCID, ieutPROBE_016);

    bool globalTran;
    char *tmpPtr;

    if (LIKELY(pTR->Version == iestTR_CURRENT_VERSION))
    {
        globalTran = pTR->GlobalTran;

        tmpPtr = (char *)(pTR+1);
    }
    else
    {
        rc = ISMRC_InvalidValue;
        ism_common_setErrorData(rc, "%u", pTR->Version);
        goto mod_exit;
    }

    // And rehydrate details from the transaction store record
    if (globalTran == false)
    {
        // Create the transaction object in memory
        pTran = (ismEngine_Transaction_t *)iemem_calloc( pThreadData
                                                       , IEMEM_PROBE(iemem_localTransactions, 3)
                                                       , 1
                                                       , sizeof(ismEngine_Transaction_t));
        if (pTran == NULL)
        {
           rc = ISMRC_AllocateError;
           ism_common_setError(rc);
           goto mod_exit;
        }

        assert((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) != ietrTRAN_FLAG_GLOBAL);

        // Extract the transaction state and change time
        pTran->TranState = (record->State & 0xffffffff);
        pTran->StateChangedTime = ism_common_convertExpireToTime((uint32_t)(record->State >> 32));
    
        rc = iemp_createMemPool( pThreadData
                               , IEMEM_PROBE(iemem_localTransactions, 8)
                               , TRANSACTION_MEMPOOL_RESERVEDMEMBYTES
                               , TRANSACTION_MEMPOOL_INITIALPAGEBYTES
                               , TRANSACTION_MEMPOOL_SUBSEQUENTPAGEBYTES
                               , &(pTran->hTranMemPool));
        if( rc != OK )
        {
            goto mod_exit;
        }
    }
    else
    {
        ism_xid_t *pXID = (ism_xid_t *)tmpPtr;
        uint32_t XIDHashValue;
        char XIDBuffer[300]; // Maximum string size from ism_common_xidToString is 278

        // Create the transaction object in memory
        pTran = (ismEngine_Transaction_t *)iemem_calloc( pThreadData
                                                       , IEMEM_PROBE(iemem_globalTransactions, 3)
                                                       , 1
                                                       , ietr_GLOBALTRAN_SIZE);
        if (pTran == NULL)
        {
           rc = ISMRC_AllocateError;
           ism_common_setError(rc);
           goto mod_exit;
        }

        memcpy(pTran+1, pXID, sizeof(ism_xid_t));
        pTran->pXID = (ism_xid_t *)(pTran+1);
        pTran->TranFlags = ietrTRAN_FLAG_GLOBAL;
        // Extract the transaction state and change time
        pTran->TranState = (record->State & 0xffffffff);
        pTran->StateChangedTime = ism_common_convertExpireToTime((uint32_t)(record->State >> 32));

        // Generate the string representation of the XID
        const char *XIDString = ism_common_xidToString(pXID, XIDBuffer, sizeof(XIDBuffer));

        assert(XIDString == XIDBuffer);

        // Generate the hash value for the XID
        XIDHashValue = ietr_genHashId(pXID);

        ieutTRACEL(pThreadData, XIDHashValue, ENGINE_NORMAL_TRACE,
                   "Rehydrating global transaction for %s - HashValue %d\n", XIDString, XIDHashValue);

        // Note: We do not bother taking the lock during rehydration since the rehydration process
        //       is currently single threaded - if that changes, we will need to take the lock and
        //       release it.

        // And last but not least add the transaction to the hash table
        rc = ieut_putHashEntry( pThreadData
                              , pControl->GlobalTranTable
                              , ieutFLAG_DUPLICATE_KEY_STRING
                              , XIDString
                              , XIDHashValue
                              , pTran
                              , ietr_GLOBALTRAN_SIZE );
        if( rc != OK )
        {
            ieutTRACE_FFDC( ieutPROBE_010, true
                          , "ieut_putHashEntry failed.", rc
                          , NULL );
            goto mod_exit;
        }

        pTran->TranFlags |= ietrTRAN_FLAG_IN_GLOBAL_TABLE;

        rc = iemp_createMemPool( pThreadData
                           , IEMEM_PROBE(iemem_localTransactions, 9)
                           , TRANSACTION_MEMPOOL_RESERVEDMEMBYTES
                           , TRANSACTION_MEMPOOL_INITIALPAGEBYTES
                           , TRANSACTION_MEMPOOL_SUBSEQUENTPAGEBYTES
                           , &(pTran->hTranMemPool));
        if( rc != OK )
        {
           goto mod_exit;
        }
    }

    *outData = pTran;

    ismEngine_SetStructId(pTran->StrucId, ismENGINE_TRANSACTION_STRUCID);
    pTran->useCount = 1;
    pTran->preResolveCount = 1;
    pTran->hTran = recHandle;
    pTran->TranFlags |= (ietrTRAN_FLAG_REHYDRATED | ietrTRAN_FLAG_PERSISTENT);
    pTran->fDelayedRollback = false;

    rc = ielm_allocateLockScope(pThreadData, ielmLOCK_SCOPE_COMMIT_CAPABLE,
                                          pTran->hTranMemPool, &(pTran->hLockScope));
    if( rc != OK )
    {
        ieutTRACE_FFDC( ieutPROBE_007, true
                      , "ielm_allocateLockScope failed.", rc
                      , NULL );
        goto mod_exit;
    }

mod_exit:
    if (rc != OK)
    {
        if (pTran != NULL)
        {
            iemem_freeStruct(pThreadData,
                             (pTran->TranFlags & ietrTRAN_FLAG_GLOBAL)?iemem_globalTransactions:iemem_localTransactions,
                             pTran, pTran->StrucId);
        }
        *outData = NULL;

        ierr_recordBadStoreRecord( pThreadData
                                 , record->Type
                                 , recHandle
                                 , NULL
                                 , rc);
    }

    ieutTRACEL(pThreadData, pTran,  ENGINE_FNC_TRACE, FUNCTION_EXIT "pTran=%p\n", __func__, pTran);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Complete the rehydration of a transaction
///  @remarks
///    This function performs actions required after recovery has completed to
///    complete the rehydration of a transaction, this includes opening the reference
///    context for the transaction and committing/rolling back unprepared transactions
///
///  @param[in] transactionHandle      - Store handle of the transaction
///  @param[in] rehydratedTransaction  - Rehydrated transaction object
///  @param[in] pContext               - Context for the call (unused)
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_completeRehydration( ieutThreadData_t *pThreadData
                                , uint64_t transactionHandle
                                , void *rehydratedTransaction
                                , void *pContext)
{
    int32_t rc = OK;
    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)rehydratedTransaction;

    ieutTRACEL(pThreadData, rehydratedTransaction, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Hdl(%ld) State(%d) Flags(0x%04x)\n",
               __func__, transactionHandle, pTran->TranState, pTran->TranFlags );

    assert((pTran->TranFlags & ietrTRAN_FLAG_REHYDRATED) == ietrTRAN_FLAG_REHYDRATED);
    assert(transactionHandle == pTran->hTran);

    // Open the store reference context for this transaction
    rc = ism_store_openReferenceContext( pTran->hTran
                                       , NULL
                                       , &(pTran->pTranRefContext) );
    if (rc != OK)
    {
        // If we failed to open the reference context for the transaction
        // we are going to be unable to manipulate the transaction so
        // simply bail out now.
        ieutTRACE_FFDC( ieutPROBE_001, true
                      , "ism_store_openReferenceContext for transaction failed.", rc
                      , NULL );
    }

    // We have a forwarder transaction but we are no longer a member of the cluster...
    bool exClusterForwarderTransaction = false;
    if ((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) == ietrTRAN_FLAG_GLOBAL &&
        pTran->pXID->formatID == ISM_FWD_XID &&
        ismEngine_serverGlobal.clusterEnabled == false)
    {
        ieutTRACEL(pThreadData, pTran, ENGINE_INTERESTING_TRACE, FUNCTION_IDENT "Forwarder transaction found with no cluster. Hdl(%ld) State(%d) pTran(%p)\n",
                   __func__, transactionHandle, pTran->TranState, pTran);
        exClusterForwarderTransaction = true;
    }

    if (pTran->TranState == ismTRANSACTION_STATE_HEURISTIC_COMMIT)
    {
        assert((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) == ietrTRAN_FLAG_GLOBAL);
        pTran->useCount++; // Need to remember this transaction
        rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);

        if (rc == OK && exClusterForwarderTransaction)
        {
            rc = ietr_forget(pThreadData, pTran, NULL);
        }
    }
    else if (pTran->TranState == ismTRANSACTION_STATE_HEURISTIC_ROLLBACK)
    {
        assert((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) == ietrTRAN_FLAG_GLOBAL);
        pTran->useCount++; // Need to remember this transaction
        rc = ietr_rollback(pThreadData, pTran, NULL, IETR_ROLLBACK_OPTIONS_NONE);

        if (rc == OK && exClusterForwarderTransaction)
        {
            rc = ietr_forget(pThreadData, pTran, NULL);
        }
    }
    else if (pTran->TranState == ismTRANSACTION_STATE_COMMIT_ONLY)
    {
        rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
    }
    else if ((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) != ietrTRAN_FLAG_GLOBAL ||
             pTran->TranState != ismTRANSACTION_STATE_PREPARED ||
             exClusterForwarderTransaction)
    {
        rc = ietr_rollback(pThreadData, pTran, NULL, IETR_ROLLBACK_OPTIONS_NONE);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int32_t ietr_finishParallelOperation(ieutThreadData_t *pThreadData,
                                     ismEngine_Transaction_t *pTran,
                                     ietrAsyncTransactionData_t *pAsyncTranData,
                                     bool goneAsync)
{
    int32_t rc = OK;

    if (pAsyncTranData != NULL)
    {
        uint64_t parallelAsyncCount = __sync_fetch_and_sub(&pAsyncTranData->parallelOperations, 1);

        assert(parallelAsyncCount != 0);

        ieutTRACEL(pThreadData, parallelAsyncCount, ENGINE_HIGH_TRACE,
                   FUNCTION_IDENT "pTran=%p parallelAsyncCount=%lu\n", __func__, pTran, parallelAsyncCount);

        if (parallelAsyncCount == 1)
        {
            pTran->CompletionStage = ietrCOMPLETION_ENDED;

            // Let the caller know that are done, if we went async
            if (goneAsync && pAsyncTranData->pCallbackFn != NULL)
            {
                pAsyncTranData->pCallbackFn(pThreadData, ISMRC_OK, ietr_getCustomDataPtr(pAsyncTranData));
            }

            // OK... free the memory used for the asyncness (whether or not we used it)
            ietr_freeAsyncTransactionData(pThreadData, &pAsyncTranData);

            // Release the transaction
            ietr_releaseTransaction(pThreadData, pTran);
        }
        else
        {
            rc = ISMRC_AsyncCompletion;
        }
    }
    else
    {
        pTran->CompletionStage = ietrCOMPLETION_ENDED;

        // Release the transaction
        ietr_releaseTransaction(pThreadData, pTran);
    }
    return rc;
}

int32_t ietr_finishCommit( ieutThreadData_t *pThreadData
                         , ismEngine_Transaction_t *pTran
                         , ietrAsyncTransactionData_t *pAsyncTranData
                         , ismEngine_AsyncData_t *pAsyncData
                         , bool goneAsync)
{

    // Initialize a replay record that is passed between phases
    ietrReplayRecord_t Record = { 0, 0, ietrNO_JOB_THREAD, false };
    ietrReplayRecord_t *pRecord = ( pAsyncTranData == NULL ? &Record: &(pAsyncTranData->Record) );
    int32_t rc = OK;
    ieutTRACEL(pThreadData, pTran, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "pTran=%p\n", __func__, pTran);

    ietrTransactionControl_t *pControl = (ietrTransactionControl_t *)ismEngine_serverGlobal.TranControl;

    if (pTran->pJobThread != NULL)
    {
        pRecord->JobThreadId = (ietrJobThreadId_t)pTran->pJobThread;
    }
    else
    {
        pRecord->JobThreadId = ietrNO_JOB_THREAD;
    }

    rc = ietr_softLogCommit( pControl, pTran, pAsyncData, pAsyncTranData, pRecord, pThreadData, Commit );
    assert (rc == OK || rc == ISMRC_AsyncCompletion);

    if ( rc == ISMRC_AsyncCompletion)
    {
        //Get out of here... the callback could have already happened and the transaction no longer valid
        goto mod_exit;
    }

    if (pTran->fLockManagerUsed) ielm_releaseAllLocksBegin(pTran->hLockScope);

    // And now process the softlog for item that must be done during MemoryCommit. This phase
    // is NOT ALLOWED to go async (so should not write to the store), hence we pass pAsyncData as NULL.
    rc = ietr_softLogCommit( pControl, pTran, NULL, pAsyncTranData, pRecord, pThreadData, MemoryCommit );

    assert (rc == OK); //Will never allowed to go async... long operations should happen after

    if (pTran->fLockManagerUsed) ielm_releaseAllLocksComplete(pThreadData,  pTran->hLockScope);

    // Mark the transaction as no-longer initialised, this indicated
    // that the store handles are no longer valid.
    if (pTran->TranState != ismTRANSACTION_STATE_HEURISTIC_COMMIT) pTran->TranState = ismTRANSACTION_STATE_NONE;

    // And now process the softlog, PostCommit phase first
    rc = ietr_softLogCommit( pControl, pTran, pAsyncData, pAsyncTranData, pRecord, pThreadData, PostCommit );
    assert (rc == OK || rc == ISMRC_AsyncCompletion);

    if ( rc == ISMRC_AsyncCompletion)
    {
        //Get out of here... the callback could have already happened and the transaction no longer valid
        goto mod_exit;
    }

    rc = ietr_softLogCommit( pControl, pTran, pAsyncData, pAsyncTranData, pRecord, pThreadData, Cleanup );
    assert (rc == OK || rc == ISMRC_AsyncCompletion);

    if ( rc == ISMRC_AsyncCompletion)
    {
        //Get out of here... the callback could have already happened and the transaction no longer valid
        goto mod_exit;
    }

    // We have some JobCallbacks to do -- let's schedule them (or do them ourselves)
    if (pTran->pSoftLogHead != NULL)
    {
        // We only want to do this before we begin the JobCallback phase
        assert(Cleanup == 0x00000100 && JobCallback == 0x00000200);

        // If we're already on the job thread, we don't need to add a job!
        if (pAsyncTranData &&
            pAsyncTranData->CurrPhase == Cleanup &&
            pTran->pJobThread != NULL && pThreadData != pTran->pJobThread)
        {
            pAsyncTranData->CurrPhase = JobCallback;
            pAsyncTranData->ProcessedPhaseSLEs = 0;

            rc = ietr_addJobCallback(pThreadData, pTran, pAsyncTranData);

            // We added the job to the job queue -- and that will be processed at some point
            if (rc == ISMRC_AsyncCompletion)
            {
                goto mod_exit;
            }

            // The queue was full, so we drop through and do it now...
            assert(rc == ISMRC_DestinationFull);
        }

        rc = ietr_softLogCommit( pControl, pTran, pAsyncData, pAsyncTranData, pRecord, pThreadData, JobCallback );

        if ( rc == ISMRC_AsyncCompletion)
        {
            //Get out of here... the callback could have already happened and the transaction no longer valid
            goto mod_exit;
        }
    }

    // Finish, taking into consideration parallel operations - this may now return
    // ISMRC_AsyncCompletion if one hasn't yet finished.
    rc = ietr_finishParallelOperation(pThreadData, pTran, pAsyncTranData, goneAsync);

mod_exit:

    // NOTE: Cannot dereference pTran, but trace it for correlation with entry
    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "pTran=%p rc=%d\n", __func__, pTran, rc);
    return rc;
}

// We are being called to perform job callback
void ietr_jobCallback(ieutThreadData_t *pThreadData, void *context)
{
    ietrAsyncTransactionData_t *pAsyncTranData = (ietrAsyncTransactionData_t *)context;

    ismEngine_ClientState_t *pClient = (pAsyncTranData->pTran->pSession ?
                                        pAsyncTranData->pTran->pSession->pClient:
                                        NULL);

    ietj_updateThreadData(pThreadData, pClient);

    ieutTRACEL(pThreadData, pAsyncTranData->asyncId, ENGINE_CEI_TRACE,
               FUNCTION_ENTRY "ietrACId=0x%016lx, pTran=%p\n", __func__, pAsyncTranData->asyncId, pAsyncTranData->pTran);

    ismEngine_Transaction_t *pTran = pAsyncTranData->pTran;

    // We are not calling this callback on the expected thread, update the transaction to reflect that.
    if (pTran->pJobThread != pThreadData)
    {
        assert(pTran->pJobThread != NULL);

        ieut_releaseThreadDataReference(pTran->pJobThread);
        if (pThreadData->jobQueue != NULL)
        {
           ieut_acquireThreadDataReference(pThreadData);
           pTran->pJobThread = pThreadData;
        }
        else
        {
           pTran->pJobThread = NULL;
        }
    }

    ietr_finishCommit(pThreadData, pTran, pAsyncTranData, NULL, true);

    ieutTRACEL(pThreadData, pTran,  ENGINE_CEI_TRACE, FUNCTION_EXIT "pTran=%p\n", __func__, pTran);
}

static int32_t ietr_addJobCallback(ieutThreadData_t *pThreadData,
                                   ismEngine_Transaction_t *pTran,
                                   ietrAsyncTransactionData_t *pAsyncTranData)
{
    assert(pTran->pJobThread != NULL);
    assert(pThreadData->threadType == AsyncCallbackThreadType);

    pAsyncTranData->asyncId = pThreadData->asyncCounter++;

    ieutTRACEL(pThreadData, pAsyncTranData->asyncId, ENGINE_CEI_TRACE, FUNCTION_IDENT "ietrACId=0x%016lx\n",
                          __func__, pAsyncTranData->asyncId);

    int32_t rc = iejq_addJob( pThreadData
                            , pTran->pJobThread->jobQueue
                            , ietr_jobCallback
                            , pAsyncTranData
#ifdef IEJQ_JOBQUEUE_PUTLOCK
                            , true
#endif
                            );

    if (rc == OK)
    {
        rc = ISMRC_AsyncCompletion;
    }
    else
    {
        assert(rc == ISMRC_DestinationFull);

        // TODO: Do we change the pJobThread and pRecord->JobThreadId now? We didn't do what was asked...
    }

    return rc;
}

//If part way through the commit of an engine tran, the trans does an async commit
//this is the call back
void ietr_asyncCommitted(int32_t rc, void *context)
{
    //A store commit that was part of ietr_commit has finished,
    //finish committing the engine level transaction....

    ietrAsyncTransactionData_t *pAsyncTranData = (ietrAsyncTransactionData_t *)context;

    ismEngine_ClientState_t *pClient = (pAsyncTranData->pTran->pSession ?
                                          pAsyncTranData->pTran->pSession->pClient:
                                          NULL);
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pClient);

    pThreadData->threadType = AsyncCallbackThreadType;

    ieutTRACEL(pThreadData, pAsyncTranData->asyncId, ENGINE_CEI_TRACE,
               FUNCTION_ENTRY "ietrACId=0x%016lx, pTran=%p\n", __func__, pAsyncTranData->asyncId, pAsyncTranData->pTran);

    ismEngine_Transaction_t *pTran = pAsyncTranData->pTran;

    if (pTran->pJobThread != NULL && pAsyncTranData->CurrPhase == JobCallback)
    {
        rc = ietr_addJobCallback(pThreadData, pTran, pAsyncTranData);

        if (rc == ISMRC_AsyncCompletion)
        {
            goto mod_exit;
        }
    }

    ietr_finishCommit(pThreadData, pAsyncTranData->pTran, pAsyncTranData, NULL, true);

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
}

//If part way through a commit of an engine tran, something uses the engineAsync mechanism
//to go async in parallel with the transaction, this is the callback
int32_t ietr_asyncFinishParallelOperation(
        ieutThreadData_t               *pThreadData,
        int32_t                         retcode,
        ismEngine_AsyncData_t          *pAsyncData,
        ismEngine_AsyncDataEntry_t     *context)
{
    ietrAsyncTransactionData_t *pAsyncTranData = (ietrAsyncTransactionData_t *)(context->Handle);
    assert(retcode == OK);

    (void)ietr_finishParallelOperation(pThreadData, pAsyncTranData->pTran, pAsyncTranData, true);

    //We've finished, remove entry so if we go async we don't try to finish the tran again
    iead_popAsyncCallback(pAsyncData, context->DataLen);

    ieutTRACEL(pThreadData, pAsyncTranData, ENGINE_FNC_TRACE,
               FUNCTION_IDENT "pAsyncTranData=%p retcode=%d\n", __func__, pAsyncTranData, retcode);

    return retcode;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Assign the pThreadData as the job thread for a transaction if it is
///     appropriate to do so based on server settings.
///////////////////////////////////////////////////////////////////////////////
void ietr_assignTranJobThread( ieutThreadData_t *pThreadData,
                               ismEngine_Transaction_t *pTran )
{
    assert(pTran->pJobThread == NULL); // Don't expect to be called twice!

    if (pThreadData->jobQueue != NULL &&
        ismEngine_serverGlobal.totalActiveClientStatesCount >=
        ismEngine_serverGlobal.TransactionThreadJobsClientMinimum)
    {
        ieut_acquireThreadDataReference(pThreadData);
        pTran->pJobThread = pThreadData;
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Commits a transaction          
///  @remarks
///    This function commits a transaction.                             
///
///    The commit operation occurs in 2 phases,
///    1. Delete the store-transaction object which contains the 
///       rollback details
///    2. Roll-forward the soft-log to complete the transactional
///       operations
///
///  @param[in] pTran              - Ptr to the transaction
///  @param[in] options            - options controlling behaviour of call
///  @param[in] pSession           - Ptr to the session (may be NULL)
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_commit( ieutThreadData_t *pThreadData
                   , ismEngine_Transaction_t *pTran
                   , uint32_t options
                   , ismEngine_Session_t *pSession
                   , ietrAsyncTransactionData_t *pAsyncTranData
                   , ietrCommitCompletionCallback_t  pCallbackFn)
{
    int32_t rc=0;

    ieutTRACEL(pThreadData, pTran, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p options=0x%08x global=%s fAsStoreTran=%s\n", __func__,
               pTran, options, (pTran->TranFlags & ietrTRAN_FLAG_GLOBAL)?"true":"false", pTran->fAsStoreTran?"true":"false");

    if (!ismEngine_CompareStructId(pTran->StrucId, ismENGINE_TRANSACTION_STRUCID))
    {
        // We have been called to commit a transaction, but the transaction we 
        // have been passed in invalid. 
        rc = ISMRC_ArgNotValid;

        ieutTRACE_FFDC( ieutPROBE_005, false
                      , "Invalid transaction passed to Commit.", rc
                      , "pTran", pTran, sizeof(ismEngine_Transaction_t)
                      , "pTran->pSession", pTran->pSession, sizeof(ismEngine_Session_t)
                      , NULL );
        goto mod_exit;
    }

    // If this is a global transaction then we have a bit of validation we
    // have to do before proceeding with the commit
    if ((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) == ietrTRAN_FLAG_GLOBAL)
    {
        // Only allow commit of an in-flight transaction if TMONEPHASE specified
        if (pTran->TranState == ismTRANSACTION_STATE_IN_FLIGHT)
        {
            if ((options & ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMONEPHASE) == 0)
            {
                rc = ISMRC_InvalidOperation;
                ism_common_setError(rc);
                goto mod_exit;
            }
        }
        else
        {
            // Check that the transaction is in a valid state
            if ((pTran->TranState != ismTRANSACTION_STATE_PREPARED) &&
                (pTran->TranState != ismTRANSACTION_STATE_COMMIT_ONLY) &&
                (pTran->TranState != ismTRANSACTION_STATE_HEURISTIC_COMMIT))
            {
                rc = ISMRC_InvalidOperation;
                ism_common_setError(rc);
                goto mod_exit;
            }
        }

        // Either the transaction has no session, or the session is the requesting one
        if ((pTran->pSession != NULL) && (pSession != pTran->pSession))
        {
            rc = ISMRC_TransactionInUse;
            ism_common_setError(rc);
            goto mod_exit;
        }
    }
    else
    {
        // Verify that the transaction is initialised
        if ((pTran->TranState != ismTRANSACTION_STATE_IN_FLIGHT) &&
            (pTran->TranState != ismTRANSACTION_STATE_COMMIT_ONLY))
        {
            rc = ISMRC_InvalidOperation;
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    if (pTran->preResolveCount != 1)
    {
        //There are remaining operations as part of this operation...
        //We don't allow the commit to happen until they have completed
        ieutTRACEL(pThreadData, pTran->preResolveCount, ENGINE_ERROR_TRACE,
                       "preResolveCount %lu\n", pTran->preResolveCount);

        rc = ISMRC_InvalidOperation;
        ism_common_setError(rc);
        goto mod_exit;
    }
    else
    {
        //Reduce the count as we are starting the resolve
        DEBUG_ONLY uint64_t oldCount = __sync_fetch_and_sub(&(pTran->preResolveCount), 1);
        assert (oldCount == 1);
    }

    // Make sure any savepoint left behind is now marked as inactive
    if (pTran->pActiveSavepoint != NULL)
    {
        ietr_endSavepoint(pThreadData, pTran, (ietrSavepoint_t *)(pTran->pActiveSavepoint), None);
        assert(pTran->pActiveSavepoint == NULL);
    }

    // If the transaction has been marked rollback only, then we rollback now.
    if (pTran->fRollbackOnly)
    {
        // We don't need the async data - so we should free it now (note that
        // this also sets pAsyncData to NULL, meaning the call at the end of
        // this function will be a no-op)
        ietr_freeAsyncTransactionData(pThreadData, &pAsyncTranData);
        assert(pAsyncTranData == NULL);

        //We will have set the pre-resolve count to 0, but in this case we actually
        //want to call both commit and rollback... so reset the count
        DEBUG_ONLY uint64_t oldCount = __sync_fetch_and_add(&(pTran->preResolveCount), 1);
        assert (oldCount == 0);

        rc = ietr_rollback(pThreadData, pTran, pSession, IETR_ROLLBACK_OPTIONS_NONE);
        if (rc == OK)
        {
            // We have completed the rollback so let the caller know.
            rc = ISMRC_RolledBack;
            ism_common_setError(rc);
        }
        else
        {
            // Even the rollback failed, propagate the failure code
            ieutTRACEL(pThreadData, rc, ENGINE_ERROR_TRACE,
                       "%s: ietr_rollback failed! (rc=%d)\n", __func__, rc);
        }
        goto mod_exit;
    }

    if ((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) == ietrTRAN_FLAG_GLOBAL)
    {
        // Someone else has already committed or rolled this transaction back
        if (!__sync_bool_compare_and_swap(&pTran->CompletionStage,
                                          ietrCOMPLETION_NONE,
                                          ietrCOMPLETION_STARTED))
        {
            rc = ISMRC_InvalidOperation;
            ism_common_setError(rc);
            goto mod_exit;
        }

        // If this transaction is currently bound to the client, break this link now
        if (pTran->pClient != NULL)
        {
            iecs_unlinkTransaction( pTran->pClient, pTran);
        }
    }
    else
    {
        assert(pTran->CompletionStage == ietrCOMPLETION_NONE);
        pTran->CompletionStage = ietrCOMPLETION_STARTED;
    }

    ietrTransactionControl_t *pControl = (ietrTransactionControl_t *)ismEngine_serverGlobal.TranControl;

    // If the number of store operations (plus overhead for deleting transaction
    // store records etc) exceeds the number of operations that are guaranteed to
    // fit in a store transaction then we must do incremental commit.
    if ((pTran->TranFlags & ietrTRAN_FLAG_PERSISTENT) == ietrTRAN_FLAG_PERSISTENT && !pTran->fAsStoreTran)
    {
        uint32_t TranOpThreshold = pControl->StoreTranRsrvOps / (ietrMAX_COMMIT_STORE_OPS + 1);

        if (pTran->TranOpCount >= TranOpThreshold)
        {
            pTran->fIncremental = true;

            ieutTRACEL(pThreadData, TranOpThreshold, ENGINE_ERROR_TRACE,
                       "The limit on store operations(%u) is exceeded by this transaction(%u). Setting incremental commit\n",
                       TranOpThreshold, pTran->TranOpCount);
        }
    }

    //Break the link between the session and the transaction...
    ietr_unlinkTranSession(pThreadData, pTran);

    if (pAsyncTranData != NULL)
    {
        pAsyncTranData->parallelOperations = 1;
        pAsyncTranData->pTran        = pTran;
        pAsyncTranData->pCallbackFn  = pCallbackFn;
    }

    ietr_assignTranJobThread(pThreadData, pTran);

    rc = ietr_finishCommit( pThreadData
                          , pTran
                          , pAsyncTranData
                          , NULL
                          , false);

mod_exit:

    if (rc != OK && rc != ISMRC_AsyncCompletion)
    {
        ietr_freeAsyncTransactionData(pThreadData, &pAsyncTranData);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}
void ietr_finishRollback( ieutThreadData_t *pThreadData
                           , ismEngine_Transaction_t *pTran)
{
    ieutTRACEL(pThreadData, pTran, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    ietrTransactionControl_t *pControl = (ietrTransactionControl_t *)ismEngine_serverGlobal.TranControl;

    // If the number of store operations (plus overhead for deleting transaction
    // store records etc) exceeds the number of operations that are guaranteed to
    // fit in a store transaction then we must do incremental commit.
    if ((pTran->TranFlags & ietrTRAN_FLAG_PERSISTENT) == ietrTRAN_FLAG_PERSISTENT && !pTran->fAsStoreTran)
    {
        uint32_t TranOpThreshold = pControl->StoreTranRsrvOps / (ietrMAX_ROLLBACK_STORE_OPS + 1);

        if (pTran->TranOpCount >= TranOpThreshold)
        {
            pTran->fIncremental = true;

            ieutTRACEL(pThreadData, TranOpThreshold, ENGINE_ERROR_TRACE,
                    "The limit on store operations(%u) is exceeded by this transaction(%u). Setting incremental commit\n",
                    TranOpThreshold, pTran->TranOpCount);
        }
    }

    // Initialize a replay record that is passed between phases
    ietrReplayRecord_t Record = { 0, 0, pTran->pJobThread, false};

    // First process the softlog to remove the records from the store
    ietr_softLogRollback( pControl, pTran, &Record, pThreadData, Rollback );

    if (pTran->fLockManagerUsed) ielm_releaseAllLocksBegin( pTran->hLockScope);

    // First process the softlog to remove the records from the store
    ietr_softLogRollback( pControl, pTran, &Record, pThreadData, MemoryRollback );

    if (pTran->fLockManagerUsed) ielm_releaseAllLocksComplete(pThreadData, pTran->hLockScope);

    // Mark the transaction has no-longer initialised, this indicated
    // that the store handles are no longer valid.
    if (pTran->TranState != ismTRANSACTION_STATE_HEURISTIC_ROLLBACK) pTran->TranState = ismTRANSACTION_STATE_NONE;

    // Finally execute and post-commit logic
    ietr_softLogRollback( pControl, pTran, &Record, pThreadData, PostRollback );

    ietr_softLogRollback( pControl, pTran, &Record, pThreadData, Cleanup );

    // Don't currently expect anything to request callback
    assert(pTran->pSoftLogHead == NULL);

    pTran->CompletionStage = ietrCOMPLETION_ENDED;

    // And finally release the transaction
    ietr_releaseTransaction(pThreadData, pTran);

    ieutTRACEL(pThreadData, pTran, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Rollback a transaction          
///  @remarks
///    This function rollback a transaction.                             
///
///    The rollback operation occurs in 2 phases,
///    1. Roll-backward the soft-log to undo the transactional
///       operations on the queue and in the store
///    2. Delete the store-transaction object which contains the 
///       rollback details
///
///  @param[in] pTran              - Ptr to the transaction
///  @param[in] pSession           - Ptr to the session (may be NULL)
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_rollback( ieutThreadData_t *pThreadData
                     , ismEngine_Transaction_t *pTran
                     , ismEngine_Session_t *pSession
                     , uint64_t options)
{
    int32_t rc=0;

    ieutTRACEL(pThreadData, pTran, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p fAsStoreTran=%s\n", __func__,
               pTran, pTran->fAsStoreTran?"true":"false");

    if (!ismEngine_CompareStructId(pTran->StrucId, ismENGINE_TRANSACTION_STRUCID))
    {
        // We have been called to rollback a transaction, but the transaction
        // we have been passed is invalid. 
        rc = ISMRC_ArgNotValid;

        ieutTRACE_FFDC( ieutPROBE_006, false
                      , "Invalid transaction passed to Rollback.", rc
                      , "pTran", pTran, sizeof(ismEngine_Transaction_t)
                      , "pTran->pSession", pTran->pSession, sizeof(ismEngine_Session_t)
                      , NULL );
        goto mod_exit;
    }

    //If there are operations still in progress on this transaction, we can't call rollback until
    //they have finished, we make a special exception for the case of session close, which can happen
    //at any time and is simpler to deal with - we don't need a callback etc, the transactions contribution
    //(and eventual removal) from the session prenack count is enough.
    if (pTran->preResolveCount != 1)
    {
        if ((options & IETR_ROLLBACK_OPTIONS_SESSIONCLOSE) == 0)
        {
            ieutTRACEL(pThreadData, pTran->preResolveCount, ENGINE_ERROR_TRACE,
                          "Error: Inprogress operations count=%lu\n", pTran->preResolveCount);
            rc = ISMRC_InvalidOperation;
            ism_common_setError(rc);
            goto mod_exit;
        }
        else
        {
            ieutTRACEL(pThreadData, pTran->preResolveCount, ENGINE_NORMAL_TRACE,
                                     "Inprogress operations count=%lu\n", pTran->preResolveCount);
            pTran->fDelayedRollback = true;
        }
    }

    // If this is a global transaction then we have a bit of validation we
    // have to do before proceeding with the commit
    if ((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) == ietrTRAN_FLAG_GLOBAL)
    {
        if ((pTran->pSession != NULL) && (pSession != pTran->pSession))
        {
            rc = ISMRC_TransactionInUse;
            ism_common_setError(rc);
            goto mod_exit;
        }

        // Someone else has already committed or rolled this transaction back
        if (!__sync_bool_compare_and_swap(&pTran->CompletionStage,
                                          ietrCOMPLETION_NONE,
                                          ietrCOMPLETION_STARTED))
        {
            rc = ISMRC_InvalidOperation;
            ism_common_setError(rc);
            goto mod_exit;
        }

        // If this transaction is currently bound to the client, break this link now
        if (pTran->pClient != NULL)
        {
            iecs_unlinkTransaction( pTran->pClient, pTran);
        }
    }
    else
    {
        // Verify that the transaction is initialised
        if ((pTran->TranState != ismTRANSACTION_STATE_IN_FLIGHT) &&
            (pTran->TranState != ismTRANSACTION_STATE_ROLLBACK_ONLY))
        {
            ieutTRACEL(pThreadData, pTran->TranState, ENGINE_ERROR_TRACE,
                       "Invalid tran state(%d) for rollback\n", pTran->TranState);

            rc = ISMRC_InvalidOperation;
            ism_common_setError(rc);
            goto mod_exit;
        }

        assert(pTran->CompletionStage == ietrCOMPLETION_NONE);
        pTran->CompletionStage = ietrCOMPLETION_STARTED;
    }
    
    //Break any link between the session and the transaction
    ietr_unlinkTranSession(pThreadData, pTran);
    
    // Make sure any savepoint left behind is now marked as inactive
    if (pTran->pActiveSavepoint != NULL)
    {
        ietr_endSavepoint(pThreadData, pTran, (ietrSavepoint_t *)(pTran->pActiveSavepoint), None);
        assert(pTran->pActiveSavepoint == NULL);
    }

    ietr_assignTranJobThread(pThreadData, pTran);

    //Assuming that there are not puts acks inflight... decreasing this
    //count will call finish_rollback
    ietr_decreasePreResolveCount( pThreadData
                                , pTran
                                , false);

mod_exit:
    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return rc;
}


static void ietr_completePrepare( ieutThreadData_t *pThreadData
                                , ismEngine_Transaction_t *pTran
                                , uint32_t time)
{
    // Update the state of the transaction to be prepared (success)
    pTran->TranState = ismTRANSACTION_STATE_PREPARED;
    pTran->StateChangedTime = ism_common_convertExpireToTime(time);
}

typedef struct tag_ietrInflightPrepare_t {
    char StructId[4]; //ETIP
    ismEngine_Transaction_t *pTran;
    uint32_t time;
} ietrInflightPrepare_t;
#define IETR_INFLIGHTPREPARE_STRUCTID "ETIP"

static int32_t ietr_asyncPrepare(
        ieutThreadData_t               *pThreadData,
        int32_t                         retcode,
        ismEngine_AsyncData_t          *asyncInfo,
        ismEngine_AsyncDataEntry_t     *context)
{
    assert(context->Type == TranPrepare);
    assert(retcode == OK);

    ietrInflightPrepare_t *prepareData = (ietrInflightPrepare_t *)(context->Data);
    ismEngine_CheckStructId(prepareData->StructId, IETR_INFLIGHTPREPARE_STRUCTID, ieutPROBE_001);

    //Pop this entry off the stack now so it doesn't get called again
    iead_popAsyncCallback( asyncInfo, context->DataLen);

    ietr_completePrepare(pThreadData, prepareData->pTran, prepareData->time);

    return retcode;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Prepares a transaction for commit
///
///  @param[in] pTran              - Ptr to the transaction
///  @param[in] pSession           - Ptr to the session (may be NULL)
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_prepare( ieutThreadData_t *pThreadData
                    , ismEngine_Transaction_t *pTran
                    , ismEngine_Session_t *pSession
                    , ismEngine_AsyncData_t *asyncData)
{
    int32_t rc=0;

    ieutTRACEL(pThreadData, pTran, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p fAsStoreTran=%s\n", __func__,
               pTran, pTran->fAsStoreTran?"true":"false");

    if (!ismEngine_CompareStructId(pTran->StrucId, ismENGINE_TRANSACTION_STRUCID))
    {
        // We have been called to commit a transaction, but the transaction we 
        // have been passed in invalid. 
        rc = ISMRC_ArgNotValid;
        ism_common_setError(rc);

        ieutTRACE_FFDC( ieutPROBE_009, false
                      , "Invalid transaction passed to Prepare.", rc
                      , "pTran", pTran, sizeof(ismEngine_Transaction_t)
                      , "pTran->pSession", pTran->pSession, sizeof(ismEngine_Session_t)
                      , NULL );
        goto mod_exit;
    }

    // Only global transactions can be prepared
    if ((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) != ietrTRAN_FLAG_GLOBAL)
    {
        rc = ISMRC_InvalidOperation;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Make sure that, if an endTransaction has just happened on another thread we
    // pick up the actual values in the transaction
    __sync_synchronize();

    if (pTran->TranState != ismTRANSACTION_STATE_IN_FLIGHT)
    {
        rc = ISMRC_InvalidOperation;
        ism_common_setError(rc);
        goto mod_exit;
    }

    if (pTran->pSession == NULL)
    {
        // A transaction which is not bound to a session may be linked to
        // a client so we must break this connection when we prepare
        if (pTran->pClient != NULL)
        {
            iecs_unlinkTransaction(pTran->pClient, pTran);
            pTran->pClient = NULL;
        }
    }
    else
    {
        if (pSession != pTran->pSession)
        {
            ieutTRACEL(pThreadData, pTran->pSession, ENGINE_INTERESTING_TRACE, "Unexpected pTran->pSession value %p (pSession=%p pTran=%p)\n",
                       pTran->pSession, pSession, pTran);
            rc = ISMRC_TransactionInUse;
            ism_common_setError(rc);
            goto mod_exit;
        }
        // Break the association with the session
        else
        {
            assert(pTran->pClient == NULL);

            ietr_unlinkTranSession(pThreadData, pTran);
        }
    }

    // Make sure any savepoint left behind is now marked as inactive
    if (pTran->pActiveSavepoint != NULL)
    {
        ietr_endSavepoint(pThreadData, pTran, (ietrSavepoint_t *)(pTran->pActiveSavepoint), None);
        assert(pTran->pActiveSavepoint == NULL);
    }

    // If the transaction has been marked rollback only, then we roll
    // the transaction back now and return a notification to the caller
    if (pTran->fRollbackOnly)
    {
        rc=ietr_rollback(pThreadData, pTran, pSession, IETR_ROLLBACK_OPTIONS_NONE);
        if (rc == OK)
        {
            // We have completed the rollback so let the caller know.
            rc = ISMRC_RolledBack;
            ism_common_setError(rc);
        }
        else
        {
            // Even the rollback failed, propagate the failure code
            ieutTRACEL(pThreadData, rc, ENGINE_ERROR_TRACE,
                       "%s: ietr_rollback failed! (rc=%d)\n", __func__, rc);
        }
        goto mod_exit;
    }

    // Remember the time when the state changed - this is also written into the store
    uint32_t now = ism_common_nowExpire();

    // And also update the record in the store
    rc = ism_store_updateRecord(pThreadData->hStream,
                                pTran->hTran,
                                0,
                                ((uint64_t)now << 32) | ismTRANSACTION_STATE_PREPARED,
                                ismSTORE_SET_STATE);
    if (rc != OK)
    {
        // Hmm, really not happy about this update failing. return the
        // error to the caller. We could mark this record as Rollback
        // only but we will roll-back when the client disconnects.
        ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                   "Failed to update transaction state. rc = %d", rc);

        rc = ISMRC_InvalidOperation;
        ism_common_setError(rc);
        goto mod_exit;
    }
  
    // Commit the update and then we are done
    if (asyncData != NULL)
    {
        ietrInflightPrepare_t prepareData = { IETR_INFLIGHTPREPARE_STRUCTID
                                            , pTran
                                            , now };

        ismEngine_AsyncDataEntry_t newEntry =
                               { ismENGINE_ASYNCDATAENTRY_STRUCID
                               , TranPrepare
                               , &prepareData, sizeof(prepareData)
                               , NULL
                               , {.internalFn = ietr_asyncPrepare}};

        iead_pushAsyncCallback( pThreadData
                              , asyncData
                              , &newEntry);

        rc = iead_store_asyncCommit(pThreadData, false, asyncData);
        assert(rc == OK || rc == ISMRC_AsyncCompletion);

        if (rc == OK)
        {
            //Don't need the callback we added:
            iead_popAsyncCallback(asyncData, newEntry.DataLen);
        }
        else
        {
            goto mod_exit;
        }
    }
    else
    {
        iest_store_commit(pThreadData, false);
    }

    ietr_completePrepare(pThreadData, pTran, now);

mod_exit:
    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Perform Heuristic completion of a transaction
///
///  @param[in] pTran              - Ptr to the transaction
///  @param[in] outcome            - required outcome state
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_complete( ieutThreadData_t *pThreadData
                     , ismEngine_Transaction_t *pTran
                     , ismTransactionState_t outcome
                     , ietrCommitCompletionCallback_t pCallbackFn
                     , ietrAsyncHeuristicCommitData_t *pAsyncData)
{
    int32_t rc=0;

    ieutTRACEL(pThreadData, pTran,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p outcome=%d\n", __func__, pTran, (int32_t)outcome);

    assert((outcome == ismTRANSACTION_STATE_HEURISTIC_COMMIT) || (outcome == ismTRANSACTION_STATE_HEURISTIC_ROLLBACK));

    // Only prepared global transactions can be completed
    if ((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) != ietrTRAN_FLAG_GLOBAL ||
        pTran->TranState != ismTRANSACTION_STATE_PREPARED)
    {
        rc = ISMRC_InvalidOperation;
        ism_common_setError(rc);
        goto mod_exit;
    }

    assert(pTran->pSession == NULL);
    assert(pTran->pClient == NULL);
    assert(pTran->fRollbackOnly == false);

    uint32_t now = ism_common_nowExpire();

    // For rollback we update the store record and in-memory transaction state
    // for commit we change in memory stage, store state is changed in the actual commit
    if (outcome == ismTRANSACTION_STATE_HEURISTIC_ROLLBACK)
    {
        rc = ism_store_updateRecord(pThreadData->hStream,
                                    pTran->hTran,
                                    0,
                                    ((uint64_t)now << 32) | (uint8_t)outcome,
                                    ismSTORE_SET_STATE);
        if (rc != OK)
        {
            // Hmm, really not happy about this update failing. return the
            // error to the caller. We could mark this record as Rollback
            // only but we will roll-back when the client disconnects.
            ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                       "Failed to update transaction state. rc = %d", rc);

            rc = ISMRC_InvalidOperation;
            ism_common_setError(rc);
            goto mod_exit;
        }

        // Commit the store update
        iest_store_commit(pThreadData, false);
    }

    //Update the in-memory state
    pTran->TranState = outcome;
    pTran->StateChangedTime = ism_common_convertExpireToTime(now);

    // We don't want the transaction to be removed when we are finished
    // committing / rolling back - forget will do that - so we increment
    // the use count now.
    (void)__sync_fetch_and_add(&pTran->useCount, 1);

    if (outcome == ismTRANSACTION_STATE_HEURISTIC_COMMIT)
    {
        assert(pCallbackFn != NULL);
        assert(pAsyncData  != NULL);

        //Let's go async
        ietrAsyncTransactionDataHandle_t asyncTranHandle = ietr_allocateAsyncTransactionData(
                                                                                 pThreadData
                                                                               , pTran
                                                                               , true
                                                                               , (   sizeof(ietrAsyncHeuristicCommitData_t)
                                                                                   + pAsyncData->engineCallerContextSize));

        if (asyncTranHandle != NULL)
        {
            ietrAsyncHeuristicCommitData_t *pCopiedAsyncData = (ietrAsyncHeuristicCommitData_t *)ietr_getCustomDataPtr(asyncTranHandle);

            //Copy engine data
            memcpy(pCopiedAsyncData, pAsyncData, sizeof(ietrAsyncHeuristicCommitData_t));

            //Copy data for the engine caller
            if (pAsyncData->engineCallerContextSize != 0)
            {
                void *dest = (void *)(pCopiedAsyncData+1);
                memcpy(dest, pAsyncData->engineCallerContext, pAsyncData->engineCallerContextSize);
                pCopiedAsyncData->engineCallerContext = dest;
            }
            else
            {
                assert(pAsyncData->engineCallerContext == NULL);
            }

            rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, asyncTranHandle, pCallbackFn);
        }
        else
        {
            //we'll do a sync commit
            rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
        }
    }
    // If marked RollbackOnly, prepare will have rolled it back already,
    // otherwise we explicitly roll it back now.
    else if (!pTran->fRollbackOnly)
    {
        rc = ietr_rollback(pThreadData, pTran, NULL, IETR_ROLLBACK_OPTIONS_NONE);
    }

mod_exit:
    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return rc;
}

static void ietr_completeForget( ieutThreadData_t *pThreadData
                                , ismEngine_Transaction_t *pTran)
{
    // Release the transaction now
    ietr_releaseTransaction(pThreadData, pTran);
}


static int32_t ietr_asyncForget(
        ieutThreadData_t               *pThreadData,
        int32_t                         retcode,
        ismEngine_AsyncData_t          *asyncInfo,
        ismEngine_AsyncDataEntry_t     *context)
{
    assert(context->Type == TranForget);
    assert(retcode == OK);

    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)(context->Handle);
    ismEngine_CheckStructId(pTran->StrucId, ismENGINE_TRANSACTION_STRUCID, ieutPROBE_001);

    ieutTRACEL(pThreadData, pTran,  ENGINE_FNC_TRACE, FUNCTION_IDENT "pTran=%p\n", __func__, pTran);

    //Pop this entry off the stack now so it doesn't get called again
    iead_popAsyncCallback( asyncInfo, context->DataLen);

    ietr_completeForget(pThreadData, pTran);

    return retcode;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Forget a heuristically completed transaction
///
///  @param[in] pTran              - Ptr to the transaction
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_forget( ieutThreadData_t *pThreadData
                   , ismEngine_Transaction_t *pTran
                   , ismEngine_AsyncData_t *asyncData)
{
    int32_t rc=0;

    assert(pTran != NULL);

    ieutTRACEL(pThreadData, pTran,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p TranState=%d\n", __func__, pTran, (int32_t)(pTran->TranState));

    // Only heuristically completed global transactions can be forgotten
    if (((pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) != ietrTRAN_FLAG_GLOBAL) ||
        (pTran->CompletionStage != ietrCOMPLETION_ENDED) ||
        ((pTran->TranState != ismTRANSACTION_STATE_HEURISTIC_COMMIT) &&
         (pTran->TranState != ismTRANSACTION_STATE_HEURISTIC_ROLLBACK)))
    {
        rc = ISMRC_InvalidOperation;
        ism_common_setError(rc);
        goto mod_exit;
    }

    //Break any link between the transaction and a session
    ietr_unlinkTranSession(pThreadData, pTran);

    // Close the transaction reference context if not already closed
    if (pTran->pTranRefContext != NULL)
    {
        rc = ism_store_closeReferenceContext( pTran->pTranRefContext );
        assert(rc == OK);
        pTran->pTranRefContext = NULL;
    }

    // Delete the transaction object in the store
    rc = ism_store_deleteRecord( pThreadData->hStream
                               , pTran->hTran );
    if (rc != OK)
    {
        // Integrity is broken. Stop the server
        ieutTRACE_FFDC( ieutPROBE_003, true
                      , "ism_store_deleteRecord for transaction failed.", rc
                      , "hTran", &pTran->hTran, sizeof(ismStore_Handle_t)
                      , "pTran", pTran, sizeof(ismEngine_Transaction_t)
                      , NULL );
    }
    pTran->hTran = ismSTORE_NULL_HANDLE;

    // Commit the deletion
    if (asyncData != NULL)
    {
        ismEngine_AsyncDataEntry_t newEntry =
                               { ismENGINE_ASYNCDATAENTRY_STRUCID
                               , TranForget
                               , NULL, 0
                               , pTran
                               , {.internalFn = ietr_asyncForget}};

        iead_pushAsyncCallback( pThreadData
                              , asyncData
                              , &newEntry);

        rc = iead_store_asyncCommit(pThreadData, false, asyncData);
        assert(rc == OK || rc == ISMRC_AsyncCompletion);

        if (rc == OK)
        {
            //Don't need the callback we added:
            iead_popAsyncCallback(asyncData, newEntry.DataLen);
        }
        else
        {
            goto mod_exit;
        }
    }
    else
    {
        iest_store_commit(pThreadData, false);
    }

    ietr_completeForget(pThreadData, pTran);

mod_exit:
    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Mark a transaction as rollback only
///  @remarks
///    This functions marks a transaction so that it cannot be committed,
///    only rolled-back.
///
///  @param[out] pTran             - Ptr to the transaction
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
void ietr_markRollbackOnly( ieutThreadData_t *pThreadData
                          , ismEngine_Transaction_t *pTran )
{
    ieutTRACEL(pThreadData, pTran,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p\n", __func__, pTran);

    assert(pTran->TranState == ismTRANSACTION_STATE_IN_FLIGHT);
    pTran->fRollbackOnly = true;

    ieutTRACEL(pThreadData, pTran, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Create a store transaction reference
///  @remarks
///    Transactional updates to be rolled-back upon restart are
///    identified by a reference to the store transaction record.
///    @par
///    During restart the list of references to each active transaction
///    object is generated, and from their contents a SLE of actions 
///    to be rolled-back is generated. 
///
///  @param[in] hStream           - Stream handle (record will be added
///                                 as part of existing store transaction)
///  @param[in] pTran             - Ptr to the transaction
///  @param[in] hStoreHdl         - Handle to create reference to
///  @param[in] Value             - Reference value
///  @param[in] State             - Reference state
///  @param[out] pTranRef         - Store tran reference
///  @return                      - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
uint32_t ietr_createTranRef( ieutThreadData_t *pThreadData
                           , ismEngine_Transaction_t *pTran
                           , ismStore_Handle_t hStoreHdl
                           , uint32_t Value
                           , uint8_t State
                           , ietrStoreTranRef_t *pTranRef )
{
    int32_t rc=0;
    ismStore_Reference_t tranRef;

    ieutTRACEL(pThreadData, pTran, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p Hdl=0x%lx\n", __func__,
               pTran, hStoreHdl);

    ismEngine_CheckStructId(pTran->StrucId, ismENGINE_TRANSACTION_STRUCID, ieutPROBE_017);

    assert((pTran->TranState == ismTRANSACTION_STATE_IN_FLIGHT) ||
           ((pTran->TranState == ismTRANSACTION_STATE_PREPARED) && (ismEngine_serverGlobal.runPhase == EnginePhaseRecovery)));

    assert((pTran->TranFlags & ietrTRAN_FLAG_PERSISTENT) == ietrTRAN_FLAG_PERSISTENT);

    if (pTran->fAsStoreTran)
    {
      // No reservation required for State objects
        if (Value != iestTOR_VALUE_ADD_UMS)
        {
            assert(pTran->StoreRefCount < pTran->StoreRefReserve);
            pTran->StoreRefCount++;
        }
    }
    else
    {
        // If we are doing the fanout as a single store tran, keep track of our progress against our reservation
        // (but no reservation required for State objects)
        if (pTran->fStoreTranPublish)
        {
            if (Value != iestTOR_VALUE_ADD_UMS)
            {
                pTran->StoreRefCount +=2; //+1 = tran ref, +1 is for the sub/q/remserv ref
            }
            else
            {
                pTran->StoreRefCount +=1; //+1 = tran ref
            }
            assert(pTran->StoreRefCount <= pTran->StoreRefReserve);
        }

        tranRef.OrderId = pTran->nextOrderId++; 
        pTranRef->orderId = tranRef.OrderId;
        tranRef.hRefHandle = hStoreHdl;
        tranRef.Value = Value;
        tranRef.State = State;
        tranRef.Pad[0] = 0;
        tranRef.Pad[1] = 0;
        tranRef.Pad[2] = 0;

        rc = ism_store_createReference( pThreadData->hStream
                                      , pTran->pTranRefContext
                                      , &tranRef
                                      , 0 // minimumActiveOrderId
                                      , &(pTranRef->hTranRef) );
        // The return code from this function is passed back to our caller,
        // The possible return codes we have are
        // - ISMRC_StoreGenerationFull
        //   This is an 'expected' return code, our caller is (more than
        //   likely) going to need to rollback their recent store activity
        //   and redo it.
        // - ISMRC_StoreFull
        //   Similar to the above, but any attempt to redo is unlikely to
        //   be successful so they will want to propagate the failure back
        //   to their caller I suspect.
        // - Something else?
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Remove a reference from the transaction
///  @remarks
///    If the transaction is too big to be rolled back as a single 
///    transaction, the we delete the records/references and the
///    reference to the transaction at the same time (as opposed to
///    just deleting the transaction object). This function is called
///    to remove one of these transaction references.
///
///  @param[in] hStream           - Stream handle (record will be removed
///                                 as part of existing store transaction)
///  @param[in] pTran             - Ptr to the transaction
///  @param[in] hStoreHdl         - Handle to transaction reference to delete
///  @return                      - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
void ietr_deleteTranRef( ieutThreadData_t *pThreadData
                       , ismEngine_Transaction_t *pTran
                       , ietrStoreTranRef_t *pTranRef )
{
    int32_t rc=0;

    ieutTRACEL(pThreadData, pTran, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p hTrafRef=0x%lx OrderId=%ld\n", __func__,
               pTran, pTranRef->hTranRef, pTranRef->orderId);

    ismEngine_CheckStructId(pTran->StrucId, ismENGINE_TRANSACTION_STRUCID, ieutPROBE_018);

    rc = ism_store_deleteReference( pThreadData->hStream
                                  , pTran->pTranRefContext
                                  , pTranRef->hTranRef
                                  , pTranRef->orderId
                                  , 0 );
    if (rc != OK)
    {
        ieutTRACE_FFDC( ieutPROBE_011, true
                      , "ism_store_deleteReference(hTranRef) failed."
                      , rc
                      , "Transaction", pTran, sizeof(ismEngine_Transaction_t)
                      , "Transaction Reference", pTranRef, sizeof(ietrStoreTranRef_t)
                      , NULL );
    }

    ieutTRACEL(pThreadData, pTranRef, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Adds a soft log entry to the transaction   
///  @remarks
///    This function adds an entry to the softlog for the transaction. It is an
///  internal function. External callers should use e.g. ietr_softLogAdd
///
///  @param[in] pTran             - Ptr to the transaction
///  @param[in] Type              - Type of softlog entry
///  @param[in] ReplayFn          - The SLE replay function   
///  @param[in] Phases            - Phases to call softlog
///  @param[in] pSLE              - SLE to add
///  @param[in] CommitStoreOps    - Number of store operations during commit
///  @param[in] RollbackStoreOps  - Number of store operations during rollback
///  @return                      - OK on success or ISMRC_AllocateError
///////////////////////////////////////////////////////////////////////////////
static inline int32_t ietr_softLogAddInternal( ismEngine_Transaction_t *pTran
                                             , ietrTranEntryType_t TranType
                                             , bool fSync
                                             , ietrSLEReplay_t ReplayFn
                                             , uint32_t Phases
                                             , ietrSLE_Header_t *pSLE
                                             , uint8_t CommitStoreOps
                                             , uint8_t RollbackStoreOps)
{
    int32_t rc=0;

    // Check that the number of store operations is under the expected maximum value
    assert((pTran->fAsStoreTran && CommitStoreOps <= ietrMAX_AS_STORETRAN_COMMIT_STORE_OPS) ||
           (!pTran->fAsStoreTran && CommitStoreOps <= ietrMAX_COMMIT_STORE_OPS));
    assert((pTran->fAsStoreTran && RollbackStoreOps <= ietrMAX_AS_STORTRAN_ROLLBACK_STORE_OPS) ||
           (!pTran->fAsStoreTran && RollbackStoreOps <= ietrMAX_ROLLBACK_STORE_OPS));

    if (pTran->TranState != ismTRANSACTION_STATE_IN_FLIGHT)
    {
        // Reject the request
        rc = ISMRC_InvalidOperation;
        ieutTRACE_FFDC( ieutPROBE_014, false
                      , "Prepared transaction(%p) cannot be updated"
                      , rc
                      , "hTran", &pTran->hTran, sizeof(ismStore_Handle_t)
                      , "pTran", pTran, sizeof(ismEngine_Transaction_t)
                      , NULL );
        goto mod_exit;
    }

    pTran->TranOpCount++;
    pSLE->Type = TranType;
    pSLE->ReplayFn = ReplayFn;
    pSLE->Phases = Phases;
    pSLE->fSync = fSync;
    pSLE->CommitStoreOps = CommitStoreOps;
    pSLE->RollbackStoreOps = RollbackStoreOps;

    pSLE->pNext = NULL;
    if (pTran->pSoftLogTail == NULL)
    {
        pSLE->pPrev = NULL;
        pTran->pSoftLogHead = pSLE;
        pTran->pSoftLogTail = pSLE;
    }
    else
    {
        pSLE->pPrev = pTran->pSoftLogTail;
        pTran->pSoftLogTail->pNext = pSLE;
        pTran->pSoftLogTail = pSLE;
    }

mod_exit:
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Adds a soft log entry to the transaction, allocating memory to do so
///  @remarks
///    This function adds an entry to the softlog for the transaction.
///
///  @param[in] pTran             - Ptr to the transaction
///  @param[in] Type              - Type of softlog entry
///  @param[in] ReplayFn          - The SLE replay function
///  @param[in] Phases            - Phases to call softlog
///  @param[in] pEntry            - Softlog entry (can be NULL in which case
///                                 an uninitialized entry is added to the softlog,
///                                 and is expected to be modified later)
///  @param[in] Length            - Length of softlog entry
///  @param[in] CommitStoreOps    - Number of store operations during commit
///  @param[in] RollbackStoreOps  - Number of store operations during rollback
///  @param[out] ppSLE            - Optional pointer to allocated SLE
///
///  @remark There is another version of this function (ietr_softLogAddPreAllocated)
///  which uses memory allocated earlier
///
///  @return                      - OK on success or ISMRC_AllocateError
///////////////////////////////////////////////////////////////////////////////
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
                       , ietrSLE_Header_t **ppSLE )
{
    int32_t rc=0;
    ietrSLE_Header_t *pSLE = NULL;
    ietrSLEReplay_t ReplayFn;
    bool fSync=false;
    uint32_t TotalLength = ((uint32_t)sizeof(ietrSLE_Header_t))+Length;

    ieutTRACEL(pThreadData, pTran,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p Type=%d Phases=0x%x\n", __func__, pTran, TranType, Phases);

    rc = iemp_allocate( pThreadData
                      , pTran->hTranMemPool
                      , TotalLength
                      , (void **)&pSLE);
    if (rc != OK)
    {
       goto mod_exit;
    }

    ismEngine_SetStructId(pSLE->StrucId, ietrSLE_STRUC_ID);
    pSLE->TotalLength = TotalLength;

    // If an initial value for the SLE was specified, copy it into
    // our allocated area now - this is the normal case.
    if (NULL != pEntry)
    {
        memcpy(pSLE + 1, pEntry, Length);
    }
    else
    {
        assert(ppSLE != NULL);
    }

    if (AsyncReplayFn == NULL)
    {
        assert(SyncReplayFn != NULL || TranType == ietrSLE_SAVEPOINT);
        ReplayFn.syncFn = SyncReplayFn;
        fSync = true;
    }
    else
    {
        ReplayFn.asyncFn = AsyncReplayFn;
    }

    rc = ietr_softLogAddInternal( pTran
                                , TranType
                                , fSync
                                , ReplayFn
                                , Phases
                                , pSLE
                                , CommitStoreOps
                                , RollbackStoreOps);


    if (ppSLE != NULL)
    {
        *ppSLE = pSLE;
    }

mod_exit:
    ieutTRACEL(pThreadData, pSLE,  ENGINE_FNC_TRACE, FUNCTION_EXIT "pSLE=%p\n", __func__, pSLE);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Adds a soft log entry for a rehydrated transaction reference
///  @remarks
///    This function adds an entry to the softlog for the transaction.
///
///  @param[in] pTranData         - Ptr to the transaction restart data
///  @param[in] Type              - Type of softlog entry
///  @param[in] ReplayFn          - The SLE replay function
///  @param[in] Phases            - Phases to call softlog
///  @param[in] pEntry            - Softlog entry (can be NULL in which case
///                                 an uninitialized entry is added to the softlog,
///                                 and is expected to be modified later)
///  @param[in] Length            - Length of softlog entry
///  @param[in] CommitStoreOps    - Number of store operations during commit
///  @param[in] RollbackStoreOps  - Number of store operations during rollback
///  @param[out] ppSLE            - Optional pointer to allocated SLE
///
///  @return                      - OK on success or ISMRC_AllocateError
///////////////////////////////////////////////////////////////////////////////
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
                             , ietrSLE_Header_t **ppSLE )
{
    int32_t rc=0;
    ietrSLE_Header_t *pSLE = NULL;
    ismEngine_Transaction_t *pTran = pTranData->pTran;
    uint32_t TotalLength = ((uint32_t)sizeof(ietrSLE_Header_t))+Length;

    ieutTRACEL(pThreadData, pTran,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p Type=%d Phases=0x%x\n", __func__, pTran, TranType, Phases);

    assert(ismEngine_serverGlobal.runPhase == EnginePhaseRecovery);
    assert(CommitStoreOps <= ietrMAX_COMMIT_STORE_OPS);
    assert(RollbackStoreOps <= ietrMAX_ROLLBACK_STORE_OPS);
    assert(pTran != NULL);
    assert(pEntry != NULL);

    rc = iemp_allocate( pThreadData
                      , pTran->hTranMemPool
                      , TotalLength
                      , (void **)&pSLE);
    if (rc != OK)
    {
       goto mod_exit;
    }

    ismEngine_SetStructId(pSLE->StrucId, ietrSLE_STRUC_ID);
    pSLE->TotalLength = TotalLength;

    // Copy initial SLE value specified
    memcpy(pSLE + 1, pEntry, Length);

    // Update the SLE header fields
    pTran->TranOpCount++;
    pSLE->Type = TranType;

    if (AsyncReplayFn == NULL)
    {
        assert(SyncReplayFn != NULL);
        pSLE->ReplayFn.syncFn = SyncReplayFn;
        pSLE->fSync = true;
    }
    else
    {
        pSLE->ReplayFn.asyncFn = AsyncReplayFn;
        pSLE->fSync = false;
    }
    pSLE->Phases = Phases;
    pSLE->CommitStoreOps = CommitStoreOps;
    pSLE->RollbackStoreOps = RollbackStoreOps;

    // At the moment, we simply add the SLEs to the transaction in the order in which they
    // are rehydrated, which means that they may be in a different order to the original
    // transaction. If we need to maintain the original order, we will need to use a
    // different method to add the SLE to the transaction based on orderid.

    pSLE->pNext = NULL;
    if (pTran->pSoftLogTail == NULL)
    {
        pSLE->pPrev = NULL;
        pTran->pSoftLogHead = pSLE;
        pTran->pSoftLogTail = pSLE;
    }
    else
    {
        pSLE->pPrev = pTran->pSoftLogTail;
        pTran->pSoftLogTail->pNext = pSLE;
        pTran->pSoftLogTail = pSLE;
    }

    // Keep the next OrderId for the transaction up-to-date
    if (pTran->nextOrderId <= pTranData->operationReference.OrderId)
    {
        pTran->nextOrderId = pTranData->operationReference.OrderId + 1;
    }

    if (ppSLE != NULL)
    {
        *ppSLE = pSLE;
    }

mod_exit:
    ieutTRACEL(pThreadData, pSLE,  ENGINE_FNC_TRACE, FUNCTION_EXIT "pSLE=%p\n", __func__, pSLE);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Adds a soft log entry to the transaction without allocating memory to do so
///  @remarks
///    This function adds an entry to the softlog for the transaction.
///
///  @param[in] pTran             - Ptr to the transaction
///  @param[in] Type              - Type of softlog entry
///  @param[in] ReplayFn          - The SLE replay function
///  @param[in] Phases            - Phases to call softlog
///  @param[in] pSLE              - Ptr to memory to be added to softlog
///                                   -> After the SLE structure
///                                       (only the structid needs to have been initialised)
///                                      the buffer must contain the context memory
///                                       (normally passed in as pEntry to ietr_softLogAdd)
///  @param[in] CommitStoreOps    - Number of store operations during commit
///  @param[in] RollbackStoreOps  - Number of store operations during rollback
///  @param[out] ppSLE            - Optional pointer to allocated SLE
///
///  @remark There is another version of this function (ietr_softLogAddPreAllocated)
///  which uses memory allocated earlier
///
///  @return                      - OK on success or ISMRC_AllocateError
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_softLogAddPreAllocated( ieutThreadData_t *pThreadData
                                   , ismEngine_Transaction_t *pTran
                                   , ietrTranEntryType_t TranType
                                   , ietrSLESyncReplay_t SyncReplayFn
                                   , ietrSLEAsyncReplay_t AsyncReplayFn
                                   , uint32_t Phases
                                   , ietrSLE_Header_t *pSLE
                                   , uint8_t CommitStoreOps
                                   , uint8_t RollbackStoreOps )
{
    int32_t rc=0;
    ietrSLEReplay_t ReplayFn;
    ieutTRACEL(pThreadData, pTran,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p Type=%d Phases=0x%x\n", __func__, pTran, TranType, Phases);
    bool fSync = false;

    ismEngine_CheckStructId(pSLE->StrucId, ietrSLE_STRUC_ID, ieutPROBE_001);

    if (AsyncReplayFn == NULL)
    {
        assert(SyncReplayFn != NULL);
        ReplayFn.syncFn = SyncReplayFn;
        fSync = true;
    }
    else
    {
        ReplayFn.asyncFn = AsyncReplayFn;
    }

    rc = ietr_softLogAddInternal( pTran
                                , TranType
                                , fSync
                                , ReplayFn
                                , Phases
                                , pSLE
                                , CommitStoreOps
                                , RollbackStoreOps );

    ieutTRACEL(pThreadData, pSLE,  ENGINE_FNC_TRACE, FUNCTION_EXIT "pSLE=%p\n", __func__, pSLE);
    return rc;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Removes a soft log entry from the transaction   
///  @remarks
///    This function allows a softlog entry to be removed from a 
///    transaction 
///
///  @param[in] pTran             - Ptr to the transaction
///  @param[out] pSLE             - Softlog entry to remove
///  @return                      - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
void ietr_softLogRemove( ieutThreadData_t *pThreadData
                       , ismEngine_Transaction_t *pTran
                       , ietrSLE_Header_t *pSLE)
{
    ietrSLE_Header_t *pCurSLE;

    ieutTRACEL(pThreadData, pSLE, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p, pSLE=%p\n", __func__,
               pTran, pSLE);

    ismEngine_CheckStructId(pSLE->StrucId, ietrSLE_STRUC_ID, ieutPROBE_019);

    // Verify that this SLE is part of the transaction
    for (pCurSLE = pTran->pSoftLogTail;
         (pCurSLE != NULL) && (pCurSLE != pSLE);
         pCurSLE = pCurSLE->pPrev)
        ;

    assert(pCurSLE != NULL);

    pTran->TranOpCount--;

    if (pSLE->pPrev == NULL)
    {
        pTran->pSoftLogHead = pSLE->pNext;
    }
    else
    {
        ((ietrSLE_Header_t *)pSLE->pPrev)->pNext = pSLE->pNext;
    }
    if (pSLE->pNext == NULL)
    {
        pTran->pSoftLogTail = pSLE->pPrev;
    }
    else
    {
        ((ietrSLE_Header_t *)pSLE->pNext)->pPrev = pSLE->pPrev;
    }

    // If this was a pre-allocated SLE, rather than part of the mem pool, free it.
    if ((pSLE->Type & ietrSLE_PREALLOCATED_MASK) == ietrSLE_PREALLOCATED_MASK)
    {
        iemem_freeStruct(pThreadData, iemem_localTransactions, pSLE, pSLE->StrucId);
    }

    ieutTRACEL(pThreadData, pTran, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

static int32_t ietr_tranStoreCommit( ieutThreadData_t *pThreadData
                                   , ietrAsyncTransactionData_t *pAsyncData
                                   , bool commitReservation)
{
    int32_t rc = OK;

    if (pAsyncData != NULL)
    {
        pAsyncData->asyncId = pThreadData->asyncCounter++;

        ieutTRACEL(pThreadData, pAsyncData->asyncId, ENGINE_CEI_TRACE, FUNCTION_IDENT "ietrACId=0x%016lx\n",
                              __func__, pAsyncData->asyncId);

        rc = iest_store_asyncCommit( pThreadData, commitReservation,  ietr_asyncCommitted, pAsyncData);
        assert(rc == OK || rc == ISMRC_AsyncCompletion);

        if (rc == ISMRC_AsyncCompletion)
        {
            pThreadData->ReservationState = Inactive;
            goto mod_exit; //The callback will take over and complete the work
        }

    }
    else
    {
        iest_store_commit( pThreadData, commitReservation );
    }
mod_exit:
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Executes the operations in the softlog
///  @remarks
///
///  @param[in] pTran             - Ptr to the transaction
///  @return                      - OK on success or ISMRC_AsynchronousCompletion
///////////////////////////////////////////////////////////////////////////////
static int32_t ietr_softLogCommit( ietrTransactionControl_t *pControl
                                 , ismEngine_Transaction_t *pTran
                                 , ismEngine_AsyncData_t *pAsyncData
                                 , ietrAsyncTransactionData_t *pAsyncTranData
                                 , ietrReplayRecord_t *pRecord
                                 , ieutThreadData_t *pThreadData
                                 , ietrReplayPhase_t Phase )
{
    uint32_t rc=OK;
    ietrSLE_Header_t *pSLE = pTran->pSoftLogHead;
    uint32_t CommitThreshold = pControl->StoreTranRsrvOps / 2;

    ieutTRACEL(pThreadData, pTran,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p Phase=0x%x\n", __func__, pTran, Phase);
    ieutTRACE_HISTORYBUF(pThreadData, Phase);

    if (pAsyncTranData != NULL)
    {
        //If we've already done this phase... we don't need to do it again
        if ( Phase < pAsyncTranData->CurrPhase)
        {
            ieutTRACE_HISTORYBUF(pThreadData, pAsyncTranData->CurrPhase);
            goto mod_exit;
        }
        if (pAsyncTranData->CurrPhase != Phase)
        {
            ieutTRACE_HISTORYBUF(pThreadData, pAsyncTranData->CurrPhase);
            pAsyncTranData->CurrPhase          = Phase;
            pAsyncTranData->ProcessedPhaseSLEs = 0;
        }
    }

    // If we are going to commit incrementally then we need to update
    // the transaction state to commit to ensure we continue to commit
    // at restart
    if ((pTran->fIncremental) && (Phase == Commit))
    {
        //Don't do the update  if we've done it already in an earlier invocation of this call...
        if (((pAsyncTranData == NULL) || !(pAsyncTranData->fMarkedCommitOnly)))
        {
            uint32_t now = ism_common_nowExpire();
            uint64_t newState;

            if (pTran->TranState == ismTRANSACTION_STATE_HEURISTIC_COMMIT)
            {
                newState = ((uint64_t)now << 32) | ismTRANSACTION_STATE_HEURISTIC_COMMIT;
            }
            else
            {
                newState = ((uint64_t)now << 32) | ismTRANSACTION_STATE_COMMIT_ONLY;
            }
            // Update the record in the store
            rc = ism_store_updateRecord(pThreadData->hStream,
                                        pTran->hTran,
                                        0,
                                        newState,
                                        ismSTORE_SET_STATE);
            if (rc != OK)
            {
                // Integrity is broken. Stop the server
                ieutTRACE_FFDC( ieutPROBE_012, true
                              , "ism_store_updateRecord(commit) for transaction failed."
                              , rc
                              , "hTran", &pTran->hTran, sizeof(ismStore_Handle_t)
                              , "pTran", pTran, sizeof(ismEngine_Transaction_t)
                              , NULL );
            }

            if (pAsyncTranData != NULL)
            {
                pAsyncTranData->fMarkedCommitOnly = true;
            }
            pRecord->StoreOpCount = 1;
            pTran->StateChangedTime = ism_common_convertExpireToTime(now);
        }
    }
    else
    {
        pRecord->StoreOpCount = 0;
    }

    //Skip any SLEs we already did in previous invocations of the call
    if (pAsyncTranData != NULL)
    {
        uint32_t numToSkip = pAsyncTranData->ProcessedPhaseSLEs;

        while (numToSkip > 0)
        {
            pSLE = pSLE->pNext;
            numToSkip--;
        }
    }

    pRecord->JobRequired = false;

    while (pSLE != NULL)
    {
        if (pSLE->Phases & Phase)
        {
            ieutTRACEL(pThreadData, pSLE, ENGINE_HIGH_TRACE, "Calling SLEReplay for SLE=%p Type=%d Phases=0x%08x\n", pSLE, pSLE->Type, pSLE->Phases);

            if ((Phase == Commit) &&
                (pTran->fIncremental) &&
                ((pRecord->StoreOpCount + pSLE->CommitStoreOps) >= CommitThreshold))
            {
                pRecord->StoreOpCount = 0;

                rc = ietr_tranStoreCommit(pThreadData, pAsyncTranData, false);

                if (rc == ISMRC_AsyncCompletion)
                {
                    goto mod_exit; //The callback will take over and complete the work
                }
            }

            if (pSLE->fSync)
            {
                pSLE->ReplayFn.syncFn(Phase, pThreadData, pTran, pSLE+1, pRecord, NULL);
            }
            else
            {
                rc = pSLE->ReplayFn.asyncFn(Phase, pThreadData, pTran, pSLE+1, pRecord, pAsyncData, pAsyncTranData, NULL);
                assert (rc == OK || rc == ISMRC_AsyncCompletion);
                assert (Phase != Cleanup || rc == OK);
                if (rc != OK)
                {
                    //Hmm we've gone async - we can no longer safely touch anything another thread may
                    //have altered it... get out of here.
                    goto mod_exit;
                }
            }

            if (pRecord->JobRequired)
            {
                assert(Phase != JobCallback); // No code to handle a job callback
                pSLE->Phases |= JobCallback;
                pRecord->JobRequired = false;
            }
        }

        // If this is the cleanup phase, remove it from the list if it is not needed for
        // the job callback phase
        if (Phase == Cleanup)
        {
            ietrSLE_Header_t *pNextSLE = pSLE->pNext;

            if ((pSLE->Phases & JobCallback) == 0)
            {
                ietrSLE_Header_t *pPrevSLE = pSLE->pPrev;

                if (pPrevSLE != NULL)
                {
                    pPrevSLE->pNext = pNextSLE;
                }
                else
                {
                    pTran->pSoftLogHead = pNextSLE;
                }

                if (pNextSLE != NULL)
                {
                    if (pTran->pSoftLogHead == pNextSLE)
                    {
                        pNextSLE->pPrev = NULL;
                    }
                    else
                    {
                        pNextSLE->pPrev = pPrevSLE;
                    }
                }
                else
                {
                    pTran->pSoftLogTail = pPrevSLE;
                }

                // Free the SLE if it is not part of the mem pool
                if ((pSLE->Type & ietrSLE_PREALLOCATED_MASK) == ietrSLE_PREALLOCATED_MASK)
                {
                    iemem_freeStruct(pThreadData, iemem_localTransactions, pSLE, pSLE->StrucId);
                }
            }

            pSLE = pNextSLE;
        }
        else if (Phase == JobCallback)
        {
            pTran->pSoftLogHead = pSLE->pNext;
            // Free the SLE if it is not part of the mem pool
            if ((pSLE->Type & ietrSLE_PREALLOCATED_MASK) == ietrSLE_PREALLOCATED_MASK)
            {
                iemem_freeStruct(pThreadData, iemem_localTransactions, pSLE, pSLE->StrucId);
            }
            pSLE = pTran->pSoftLogHead;
        }
        else
        {
            pSLE = pSLE->pNext;

            if (pAsyncTranData != NULL)
            {
                pAsyncTranData->ProcessedPhaseSLEs++;
            }
        }
    }

    // If we made updates to the store in this phase then call store commit
    // Note: If we ever inventited a "Pre" phase then we would not want to 
    //       commit it.
    if (Phase == Commit)
    {
        if (pTran->fAsStoreTran)
        {
            const bool commitReservation = (pThreadData->ReservationState == Active);

            // If we made a store reservation, or we have added store operations
            // in this phase, we need to request a store commit...
            //
            // Note: If we haven't done any store work on this thread's stream
            //       this will simply cancel the store reservation, and won't incur
            //       an actual store commit.
            if (commitReservation || pRecord->StoreOpCount != 0)
            {
                rc = ietr_tranStoreCommit(pThreadData, pAsyncTranData, commitReservation);

                if (rc == ISMRC_AsyncCompletion)
                {
                    goto mod_exit; //The callback will take over and complete the work
                }
            }
            else
            {
#ifndef NDEBUG
                // Sanity check that there are no outstanding store operations
                uint32_t storeOpsCount = 0;
                ism_store_getStreamOpsCount(pThreadData->hStream, &storeOpsCount);
                assert(storeOpsCount == 0);
#endif
            }

            assert(pThreadData->ReservationState == Inactive); 
        }
        else if ((pTran->TranFlags & ietrTRAN_FLAG_PERSISTENT) == ietrTRAN_FLAG_PERSISTENT)
        {
            if ((pAsyncTranData == NULL) || (pAsyncTranData->fRemovedTran == false))
            {
                // Assert that we can definitely include two more operation even
                // for an incremental transaction
                assert((pRecord->StoreOpCount + 2) <= pControl->StoreTranRsrvOps);

                // Close the transaction reference context
                rc = ism_store_closeReferenceContext( pTran->pTranRefContext );
                assert(rc == OK);
                pTran->pTranRefContext = NULL;

                // Delete the transaction object in the store
                rc = ism_store_deleteRecord( pThreadData->hStream
                                           , pTran->hTran );
                if (rc != OK)
                {
                    // Integrity is broken. Stop the server
                    ieutTRACE_FFDC( ieutPROBE_003, true
                                  , "ism_store_deleteRecord for transaction failed.", rc
                                  , "hTran", &pTran->hTran, sizeof(ismStore_Handle_t)
                                  , "pTran", pTran, sizeof(ismEngine_Transaction_t)
                                  , NULL );
                }

                pTran->hTran = ismSTORE_NULL_HANDLE;
                pRecord->StoreOpCount++;

                // We still need a record in the store for this transaction if it is
                // being heuristically committed - we create a new one in the same store
                // transaction, preserving XID, State and StateChangedTime.
                if (pTran->TranState == ismTRANSACTION_STATE_HEURISTIC_COMMIT)
                {
                    do
                    {
                        ismStore_Record_t storeTran;
                        iestTransactionRecord_t tranRec;
                        uint32_t dataLength;
                        char *pFrags[2];
                        uint32_t fragsLength[2];
                        uint32_t now = ism_common_getExpire(pTran->StateChangedTime);

                        pFrags[0] = (char *)&tranRec;
                        fragsLength[0] = sizeof(tranRec);
                        pFrags[1] = (char *)pTran->pXID;
                        fragsLength[1] = sizeof(ism_xid_t);
                        dataLength = sizeof(tranRec) + sizeof(ism_xid_t);

                        ismEngine_SetStructId(tranRec.StrucId, iestTRANSACTION_RECORD_STRUCID);
                        tranRec.Version = iestTR_CURRENT_VERSION;
                        tranRec.GlobalTran = true;
                        tranRec.TransactionIdLength = sizeof(ism_xid_t);
                        storeTran.Type = ISM_STORE_RECTYPE_TRANS;
                        storeTran.FragsCount = 2;
                        storeTran.pFrags = pFrags;
                        storeTran.pFragsLengths = fragsLength;
                        storeTran.DataLength = dataLength;
                        storeTran.Attribute = 0; // Unused
                        storeTran.State = ((uint64_t)now << 32) | pTran->TranState;

                        rc = ism_store_createRecord( pThreadData->hStream
                                                   , &storeTran
                                                   , &pTran->hTran);

                        if (rc == OK)
                        {
                            assert(pTran->hTran != ismSTORE_NULL_HANDLE);
                            pRecord->StoreOpCount++;
                        }
                        else if (pRecord->StoreOpCount != 0)
                        {
                            // NOTE: Committing the transaction operations
                            pRecord->StoreOpCount = 0;
                            iest_store_commit( pThreadData, false );
                        }
                    }
                    while(rc == ISMRC_StoreGenerationFull);
                }

                if (pRecord->StoreOpCount != 0)
                {
                    pRecord->StoreOpCount = 0;

                    if (pAsyncTranData)
                    {
                        pAsyncTranData->fRemovedTran = true;
                    }
                    rc = ietr_tranStoreCommit(pThreadData, pAsyncTranData, false);

                    if (rc == ISMRC_AsyncCompletion)
                    {
                        goto mod_exit; //The callback will take over and complete the work
                    }
                }
            }
        }
        else
        {
            if (pRecord->StoreOpCount)
            {
                // Even if the transaction has no store context it is possible
                // that an SLE will make a store update. The Commit phase for
                // the removal of persistent retained message by the publication
                // of a non-persistent message will cause the reference from the
                // persistent retained message to the associated topic node to be
                // deleted, this is one such time when StoreOpCount can be positive
                // for a transaction without any store context.
                pRecord->StoreOpCount = 0;
                rc = ietr_tranStoreCommit(pThreadData, pAsyncTranData, false);

                if (rc == ISMRC_AsyncCompletion)
                {
                    goto mod_exit; //The callback will take over and complete the work
                }
            }
        }
    }
    else if (pRecord->StoreOpCount)
    {
        // Even if the transaction has no store context it is possible 
        // that an SLE will make a store update. The PostCommit phase for 
        // the removal of a persistent retained message by the publication
        // of a non-persistent message is one such time when StoreOpCount
        // can be positive for a transaction without any store context.
        pRecord->StoreOpCount = 0;

        rc = ietr_tranStoreCommit(pThreadData, pAsyncTranData, false);

        if (rc == ISMRC_AsyncCompletion)
        {
            goto mod_exit; //The callback will take over and complete the work
        }
    }

mod_exit:
    ieutTRACEL(pThreadData, pTran, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__,rc);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Executes the operations in the softlog
///  @remarks
///
///  @param[in] pTran             - Ptr to the transaction
///  @return                      - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
static void ietr_softLogRollback( ietrTransactionControl_t *pControl
                                , ismEngine_Transaction_t *pTran
                                , ietrReplayRecord_t *pRecord
                                , ieutThreadData_t *pThreadData
                                , ietrReplayPhase_t Phase )
{
    uint32_t rc;
    ietrSLE_Header_t *pSLE = pTran->pSoftLogTail;
    uint32_t CommitThreshold = pControl->StoreTranRsrvOps / 2;

    ieutTRACEL(pThreadData, pTran,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p Phase=0x%x\n", __func__, pTran, Phase);

    // If we are going to rollback incrementally then we need to update
    // the transaction state to rollback to ensure we continue to rollback
    // at restart
    if ((pTran->fIncremental) && (Phase == Rollback))
    {
        // If we are heuristically rolling back, we will already have updated the store state
        if (pTran->TranState != ismTRANSACTION_STATE_HEURISTIC_ROLLBACK)
        {
            uint32_t now = ism_common_nowExpire();

            // And also update the record in the store
            rc = ism_store_updateRecord(pThreadData->hStream,
                                        pTran->hTran,
                                        0,
                                        ((uint64_t)now << 32) | ismTRANSACTION_STATE_ROLLBACK_ONLY,
                                        ismSTORE_SET_STATE);
            if (rc != OK)
            {
                // Integrity is broken. Stop the server
                ieutTRACE_FFDC( ieutPROBE_013, true
                              , "ism_store_updateRecord(rollback) for transaction failed."
                              , rc
                              , "hTran", &pTran->hTran, sizeof(ismStore_Handle_t)
                              , "pTran", pTran, sizeof(ismEngine_Transaction_t)
                              , NULL );
            }

            pRecord->StoreOpCount = 1;
            pTran->StateChangedTime = ism_common_convertExpireToTime(now);
        }
    }
    else
    {
        pRecord->StoreOpCount = 0;
    }

    while (pSLE != NULL)
    {
        if (pSLE->Phases & Phase)
        {
            ieutTRACEL(pThreadData, pSLE, ENGINE_HIGH_TRACE, "Calling SLEReplay for SLE=%p Type=%d Phases=0x%08x\n", pSLE, pSLE->Type, pSLE->Phases);

            if ((pTran->fIncremental) &&
                ((pRecord->StoreOpCount + pSLE->RollbackStoreOps) >= CommitThreshold))
            {
                iest_store_commit( pThreadData, false );
                pRecord->StoreOpCount = 0;
            }

            if (pSLE->fSync)
            {
                pSLE->ReplayFn.syncFn(Phase, pThreadData, pTran, pSLE+1, pRecord, NULL);
            }
            else
            {
                rc = pSLE->ReplayFn.asyncFn(Phase, pThreadData, pTran, pSLE+1, pRecord, NULL, NULL, NULL);

                assert(rc == OK); //We'll allow an async completion here... but not coded yet
            }
        }

        // If this is the last phase, then remove the entry
        if (Phase == Cleanup)
        {
            pTran->pSoftLogTail = pSLE->pPrev;
            // Free the SLE if it is not part of the mem pool
            if ((pSLE->Type & ietrSLE_PREALLOCATED_MASK) == ietrSLE_PREALLOCATED_MASK)
            {
                iemem_freeStruct(pThreadData, iemem_localTransactions, pSLE, pSLE->StrucId);
            }
            pSLE = pTran->pSoftLogTail;
        }
        else
        {
            pSLE = pSLE->pPrev;
        }
    }

    assert(pRecord->JobRequired == false); // No code to deal with JobCallback for rollbacks

    // If we made updates to the store in this phase then call store commit
    // Note: If we ever inventited a "Pre" phase then we would not want to 
    //       commit it.
    if (Phase == Rollback) 
    {
        if (pTran->fAsStoreTran)
        {
            // If we are running as a store transaction, then the SLE's
            // cannot make store updates that require a commit (as we
            // are going to rollback).
            assert(pRecord->StoreOpCount == 0);

            // And if we didn't get as far as making a reservation, then
            // there is nothing for us to rollback
            if (pThreadData->ReservationState == Active)
            {
                iest_store_rollback( pThreadData, true );
            }
            pThreadData->ReservationState = Inactive;
        }
        else  if ((pTran->TranFlags & ietrTRAN_FLAG_PERSISTENT) == ietrTRAN_FLAG_PERSISTENT)
        {
            // Assert that we can definitely include two more operation even
            // for an incremental transaction
            assert((pRecord->StoreOpCount + 2) <= pControl->StoreTranRsrvOps);

            // Close the transaction reference context
            rc = ism_store_closeReferenceContext( pTran->pTranRefContext );
            assert(rc == OK);
            pTran->pTranRefContext = NULL;

            // And then delete the transaction
            rc = ism_store_deleteRecord( pThreadData->hStream
                                       , pTran->hTran );
            if (rc != OK)
            {
                // Integrity is broken. Stop the server
                ieutTRACE_FFDC( ieutPROBE_004, true
                              , "ism_store_deleteRecord for transaction failed.", rc
                              , "hTran", &pTran->hTran, sizeof(ismStore_Handle_t)
                              , "pTran", pTran, sizeof(ismEngine_Transaction_t)
                              , NULL );
            }

            pTran->hTran = ismSTORE_NULL_HANDLE;
            pRecord->StoreOpCount++;

            // We still need a record in the store for this transaction if it is
            // being heuristically rolled back - we create a new one in the same
            // store transaction, preserving XID, State and StateChangedTime.
            if (pTran->TranState == ismTRANSACTION_STATE_HEURISTIC_ROLLBACK)
            {
                do
                {
                    ismStore_Record_t storeTran;
                    iestTransactionRecord_t tranRec;
                    uint32_t dataLength;
                    char *pFrags[2];
                    uint32_t fragsLength[2];
                    uint32_t now = ism_common_getExpire(pTran->StateChangedTime);

                    pFrags[0] = (char *)&tranRec;
                    fragsLength[0] = sizeof(tranRec);
                    pFrags[1] = (char *)pTran->pXID;
                    fragsLength[1] = sizeof(ism_xid_t);
                    dataLength = sizeof(tranRec) + sizeof(ism_xid_t);

                    ismEngine_SetStructId(tranRec.StrucId, iestTRANSACTION_RECORD_STRUCID);
                    tranRec.Version = iestTR_CURRENT_VERSION;
                    tranRec.GlobalTran = true;
                    tranRec.TransactionIdLength = sizeof(ism_xid_t);
                    storeTran.Type = ISM_STORE_RECTYPE_TRANS;
                    storeTran.FragsCount = 2;
                    storeTran.pFrags = pFrags;
                    storeTran.pFragsLengths = fragsLength;
                    storeTran.DataLength = dataLength;
                    storeTran.Attribute = 0; // Unused
                    storeTran.State = ((uint64_t)now << 32) | pTran->TranState;

                    rc = ism_store_createRecord( pThreadData->hStream
                                               , &storeTran
                                               , &pTran->hTran);

                    if (rc == OK)
                    {
                        assert(pTran->hTran != ismSTORE_NULL_HANDLE);
                        pRecord->StoreOpCount++;
                    }
                    else if (pRecord->StoreOpCount != 0)
                    {
                        // NOTE: Committing the transaction operations
                        iest_store_commit( pThreadData, false );
                        pRecord->StoreOpCount = 0;
                    }
                }
                while(rc == ISMRC_StoreGenerationFull);
            }

            if (pRecord->StoreOpCount != 0)
            {
                iest_store_commit( pThreadData, false );
                pRecord->StoreOpCount = 0;
            }
        }
        else
        {
            if (pRecord->StoreOpCount)
            {
                // We don't expect the store to be updated in an SLE for a
                // non persistent transaction during the rollback phase. An
                // assert in debug builds seems like a good idea.
                assert(false);

                // But in production builds do the right thing
                iest_store_commit( pThreadData, false );
                pRecord->StoreOpCount = 0;
            }
        }
    }
    else if (pRecord->StoreOpCount > 0)
    {
        iest_store_commit( pThreadData, false );
        pRecord->StoreOpCount = 0;
    }

    if (Phase == Cleanup)
    {
        pTran->pSoftLogHead = NULL;
        pTran->pSoftLogTail = NULL;
    }

    ieutTRACEL(pThreadData, pTran, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Begins a savepoint in the specified transaction
///  @remarks
///    Only a single savpoint is allowed to be active at a time on a transaction,
///    the savepoint is considered active until either the transaction is committed
///    or rolled back, or it is explicitly ended by a call to ietr_endSavepoint.
///
///  @param[in] pTran             - Ptr to the transaction
///  @param[out] ppSavepoint      - Pointer to allocated Savepoint
///
///  @return                      - OK on success or ISMRC_AllocateError
///  @see ietr_endSavepoint
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_beginSavepoint( ieutThreadData_t *pThreadData
                           , ismEngine_Transaction_t *pTran
                           , ietrSavepoint_t **ppSavepoint )
{
    int32_t rc = OK;
    ietrSavepointDetail_t savepointDetail = {pTran, true};
    ietrSavepoint_t *pSavepoint = NULL;

    ieutTRACEL(pThreadData, pTran,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p\n", __func__, pTran);

    if (pTran->pActiveSavepoint != NULL)
    {
        rc = ISMRC_SavepointActive;
        ism_common_setErrorData(rc, "%p", pTran->pActiveSavepoint);
        goto mod_exit;
    }

    rc  = ietr_softLogAdd( pThreadData
                         , pTran
                         , ietrSLE_SAVEPOINT
                         , NULL
                         , NULL
                         , None
                         , &savepointDetail
                         , sizeof(savepointDetail)
                         , 0
                         , 0
                         , (ietrSLE_Header_t **)&pSavepoint );

    pTran->pActiveSavepoint = pSavepoint;

    *ppSavepoint = pSavepoint;

mod_exit:

    ieutTRACEL(pThreadData, pSavepoint,  ENGINE_FNC_TRACE, FUNCTION_EXIT "pSavepoint=%p, rc=%d\n", __func__,
               pSavepoint, rc);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    End the specified savepoint
///  @remarks
///    This function should be called to end an active savepoint on a transaction
///    with an action to take. The SLE functions added to the transaction since
///    this savepoint was added are called if they are interested in the action
///    specified.
///
///    Passing an action of None will cause no SLEs to be caused, and will just
///    end the active savepoint.
///
///  @param[in] pTran             - Ptr to the transaction
///  @param[in] ppSavepoint       - Pointer to Savepoint
///  @param[in] action            - Phase for all SLEs after savepoint to action
///
///  @return                      - OK on success or ISMRC_AllocateError
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_endSavepoint( ieutThreadData_t *pThreadData
                         , ismEngine_Transaction_t *pTran
                         , ietrSavepoint_t *pSavepoint
                         , uint32_t action )
{
    int32_t rc = OK;
    ietrSavepointDetail_t *pSavepointDetail;

    ieutTRACEL(pThreadData, pSavepoint, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pTran=%p pSavepoint=%p action=0x%x\n",
               __func__, pTran, pSavepoint, action);

    // We only expect a single active savepoint, and so we expect this to match.
    if (pTran->pActiveSavepoint != pSavepoint)
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setErrorData(rc, "%p%p", pSavepoint, pTran->pActiveSavepoint);
        goto mod_exit;
    }

    pSavepointDetail = (ietrSavepointDetail_t *)(pSavepoint+1);

    assert(pSavepointDetail->active == true);
    assert(pSavepointDetail->pTran == pTran);

    // Call all replay functions entries after the savepoint with the specified action
    if (action != None)
    {
        ietrReplayRecord_t record = {0};
        ietrSLE_Header_t *pSLE = pSavepoint->pNext;

        // Don't want actions that will interfere with commit / rollback processing.
        assert(action == SavepointRollback);

        while(pSLE != NULL)
        {
            if (pSLE->Phases & action)
            {
                ieutTRACEL(pThreadData, pSLE, ENGINE_HIGH_TRACE, "Calling SLEReplay for SLE=%p Action=0x%08x\n", pSLE, action);

                if (pSLE->fSync)
                {
                    pSLE->ReplayFn.syncFn(action, pThreadData, pTran, pSLE+1, &record, NULL);
                }
                else if (pSLE->ReplayFn.asyncFn)
                {
                    rc = pSLE->ReplayFn.asyncFn(action, pThreadData, pTran, pSLE+1, &record, NULL, NULL, NULL);
                    assert (rc == OK);
                }

                // Don't currently expect any store operations -- will need work to support them.
                assert(record.StoreOpCount == 0);
            }

            pSLE = pSLE->pNext;
        }
    }

    pSavepointDetail->active = false;

    pTran->pActiveSavepoint = NULL;

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Free a transaction block                      
///  @remarks
///
///  @param[in] pTran             - Ptr to the transaction
///  @return                      - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
void ietr_releaseTransaction( ieutThreadData_t *pThreadData
                            , ismEngine_Transaction_t *pTran )
{
    ietrTransactionControl_t *pControl = (ietrTransactionControl_t *)ismEngine_serverGlobal.TranControl;
    uint32_t oldCount;

    ieutTRACEL(pThreadData, pTran, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // If it's a global transaction, that has been added to the table, remove
    // it from the table first
    if ((pTran->TranFlags & (ietrTRAN_FLAG_GLOBAL | ietrTRAN_FLAG_IN_GLOBAL_TABLE)) ==
           (ietrTRAN_FLAG_GLOBAL | ietrTRAN_FLAG_IN_GLOBAL_TABLE))
    {
        char XIDBuffer[300]; // Maximum string size from ism_common_xidToString is 278

        // Generate the string representation of the XID so we can use it too look
        // the transaction up again, and optionally remove it from the table
        const char *XIDString = ism_common_xidToString(pTran->pXID, XIDBuffer, sizeof(XIDBuffer));

        // Generate the hash value for the XID (before we decrease the usecount)
        uint32_t XIDHashValue = ietr_genHashId(pTran->pXID);

        assert(XIDString == XIDBuffer);

        oldCount = __sync_fetch_and_sub(&pTran->useCount, 1);

        // We might be the last ones out
        if (oldCount == 1)
        {
            ismEngine_Transaction_t *pFoundTran;

            ismEngine_getRWLockForWrite(&pControl->GlobalTranLock);

            // Check the transaction is still in the table, and still has no users
            int32_t rc = ieut_getHashEntry( pControl->GlobalTranTable
                                          , XIDString
                                          , XIDHashValue
                                          , (void **)&pFoundTran );

            // Nobody got in before we took the lock, we are the last ones out
            if (rc == OK && pTran == pFoundTran && pTran->useCount == 0)
            {
                ieut_removeHashEntry( pThreadData
                                    , pControl->GlobalTranTable
                                    , XIDString
                                    , XIDHashValue );

                ismEngine_unlockRWLock(&pControl->GlobalTranLock);

                pTran->TranFlags &= ~ietrTRAN_FLAG_IN_GLOBAL_TABLE;
            }
            // Things have changed since we took the lock
            else
            {
                ismEngine_unlockRWLock(&pControl->GlobalTranLock);
                goto mod_exit;
            }
        }
        // We are not the last ones out
        else
        {
            goto mod_exit;
        }

        // Shouldn't be proceeding if useCount is not zero
        assert(pTran->useCount == 0);
    }
    // Local transactions should never have a modified useCount
    else
    {
        // local transactions should never have an updated use count
        assert(pTran->useCount == 1);

        oldCount = 1;

        // Make it 0 for any subsequent analysis.
        pTran->useCount = 0;
    }

    assert((pTran->TranState == ismTRANSACTION_STATE_NONE) ||
           (pTran->TranState == ismTRANSACTION_STATE_HEURISTIC_COMMIT) ||
           (pTran->TranState == ismTRANSACTION_STATE_HEURISTIC_ROLLBACK));

    // If the transaction is bound to a client then unlink it. We never actually expect
    // this to be the case but worth ensuring that this is true.
    if (pTran->pClient != NULL)
    {
        // This client is currently bound to another session, if we are going to take
        // ownership of this transaction we must first unlink it from the other client.
        iecs_unlinkTransaction( pTran->pClient, pTran);
        pTran->pClient = NULL;
    }

    //we used to unlink from a session here... but we want to unlink from commit before it goes
    //async etc so we can destroy the session before we destroy the transaction. May decide in the
    //future to reinstate the removal here... and add usecounts to the session during commit to ensure
    //it is still alive here.
    assert(pTran->pSession == NULL);

    // Free any SLE that is not part of the mem pool
    while (pTran->pSoftLogHead != NULL)
    {
        ietrSLE_Header_t *pSLE = pTran->pSoftLogHead;
        pTran->pSoftLogHead = pSLE->pNext;
        if ((pSLE->Type & ietrSLE_PREALLOCATED_MASK) == ietrSLE_PREALLOCATED_MASK)
        {
            iemem_freeStruct(pThreadData, iemem_localTransactions, pSLE, pSLE->StrucId);
        }
    }

    ielm_freeLockScope(pThreadData, &(pTran->hLockScope));
    iemp_destroyMemPool(pThreadData, &(pTran->hTranMemPool));

    // We can let our job thread go now that we're definitely done with it
    if (pTran->pJobThread != NULL)
    {
        ieut_releaseThreadDataReference(pTran->pJobThread);
    }

    iemem_freeStruct(pThreadData,
                     (pTran->TranFlags & ietrTRAN_FLAG_GLOBAL)?iemem_globalTransactions:iemem_localTransactions,
                     pTran, pTran->StrucId);

mod_exit:

    ieutTRACEL(pThreadData, oldCount, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Reserves 'store' space for pending transaction
///  @remarks
///    When the transaction is implemented as a single store transaction
///    it is necessary to pre-reserve space in the store for the records/
///    references which are to be put in the store in order to ensure 
///    that ISMRC_GenerationFull is never returned.
///
///  @param[in] pTran              - Ptr to the transaction
///  @param[in] numRefs            - Number of references created
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_reserve( ieutThreadData_t *pThreadData
                    , ismEngine_Transaction_t *pTran
                    , size_t DataLength
                    , uint32_t numRefs )
{
    int32_t rc=OK;

    ieutTRACEL(pThreadData, numRefs, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(pTran->fAsStoreTran || pTran->fStoreTranPublish);
    assert((pTran->TranFlags & ietrTRAN_FLAG_PERSISTENT) == ietrTRAN_FLAG_PERSISTENT);

    pTran->StoreRefReserve = numRefs;
    pTran->StoreRefCount = 0;

    // We create one reference per put when using as a store tran and
    // assume we will write a single record (that of the message).
    iest_store_reserve( pThreadData
                      , DataLength
                      , 1
                      , numRefs); 

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Return to the caller a list of prepared transactions
///  @remarks
///    This function is equivalent to the xa_recover verb, and returns a list
///    of the prepared or heuristically completed transactions.
///
///  @param[in]     hSession         Session handle
///  @param[in]     pXIDArray        Array in which XIDs are placed
///  @param[in]     arraySize        Maximum number of XIDs to be return
///  @param[in]     rmId             resource manager id (ignored)
///  @param[in]     flags            Flags value - see above
///
///  @return                         The number of XIDs returned
///////////////////////////////////////////////////////////////////////////////
typedef struct tag_ietrXIDIterator_Callback_Context_t
{
    ietrXIDIterator_t *pTranIterator;
    uint32_t           RetCode;
} ietrXIDIterator_Callback_Context_t;

static void ietr_XIDIterator( ieutThreadData_t *pThreadData
                            , char *key
                            , uint32_t keyHash
                            , void *value
                            , void *context)
{
    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)value;
    ism_xid_t *pXID = pTran->pXID;
    ietrXIDIterator_Callback_Context_t *pContext = (ietrXIDIterator_Callback_Context_t *)context;
    ietrXIDIterator_t *pTranIterator = pContext->pTranIterator;

    if (pTran->TranState == ismTRANSACTION_STATE_PREPARED ||
        pTran->TranState == ismTRANSACTION_STATE_HEURISTIC_COMMIT ||
        pTran->TranState == ismTRANSACTION_STATE_HEURISTIC_ROLLBACK)
    {
        if (pTranIterator->arrayUsed == pTranIterator->arraySize)
        {
            uint32_t newArraySize = pTranIterator->arraySize + ietrXID_CHUNK_SIZE;
            ietrXIDIterator_t *pNewTranIterator;

            pNewTranIterator = (ietrXIDIterator_t *)iemem_realloc(pThreadData,
                                                                  IEMEM_PROBE(iemem_globalTransactions, 2),
                                                                  pTranIterator,
                                                                  sizeof(ietrXIDIterator_t) +
                                                                  (sizeof(ism_xid_t) * newArraySize));

            if (pNewTranIterator == NULL)
            {
                pContext->RetCode = ISMRC_AllocateError;
                return;
            }

            pContext->pTranIterator = pTranIterator = pNewTranIterator;
            pTranIterator->arraySize = newArraySize;
        }

        memcpy(&(pTranIterator->XIDArray[pTranIterator->arrayUsed]), pXID, sizeof(ism_xid_t));
        pTranIterator->arrayUsed++;
    }

    return;
}

uint32_t ietr_XARecover( ieutThreadData_t *pThreadData
                       , ismEngine_SessionHandle_t hSession
                       , ism_xid_t *pXIDArray
                       , uint32_t arraySize
                       , uint32_t rmId
                       , uint32_t flags )
{
    uint32_t arrayUsed = 0;
    uint32_t toCopy = 0;
    ietrTransactionControl_t *pControl = (ietrTransactionControl_t *)ismEngine_serverGlobal.TranControl;
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ietrXIDIterator_t *pXARecoverIterator;

    ieutTRACEL(pThreadData, pSession, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (pSession->pXARecoverIterator == NULL)
    {
        // Asked JUST to end a scan, and there was not one active - all done.
        if (flags == ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN) goto mod_exit;

        // Either this was an explicit or implicit start scan request, allocate
        // the iterator for this session.
        pXARecoverIterator = (ietrXIDIterator_t *)iemem_malloc(
            pThreadData,
            IEMEM_PROBE(iemem_globalTransactions, 4),
            sizeof(ietrXIDIterator_t) + (sizeof(ism_xid_t) * ietrXID_CHUNK_SIZE));

        if (pXARecoverIterator == NULL)
        {
            ism_common_setError(ISMRC_AllocateError);
            goto mod_exit;
        }

        pXARecoverIterator->arraySize = ietrXID_CHUNK_SIZE;

        // ensure that we start the scan (might be implicit)
        flags |= ismENGINE_XARECOVER_OPTION_XA_TMSTARTRSCAN;

        pSession->pXARecoverIterator = pXARecoverIterator;
    }
    else
    {
        pXARecoverIterator = pSession->pXARecoverIterator;
    }

    assert(pXARecoverIterator != NULL);

    ietrXIDIterator_Callback_Context_t CallbackContext = { pXARecoverIterator, OK };

    if (flags & ismENGINE_XARECOVER_OPTION_XA_TMSTARTRSCAN)
    {
        pXARecoverIterator->cursor = 0;
        pXARecoverIterator->arrayUsed = 0;

        ismEngine_getRWLockForRead(&pControl->GlobalTranLock);

        ieut_traverseHashTable( pThreadData
                              , pControl->GlobalTranTable
                              , ietr_XIDIterator
                              , &CallbackContext );

        ismEngine_unlockRWLock(&pControl->GlobalTranLock);

        pSession->pXARecoverIterator = pXARecoverIterator = CallbackContext.pTranIterator;
    }

    if (CallbackContext.RetCode != OK)
    {
        flags |= ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN;
    }
    // Only copy entries if not JUST asked to end scan
    else if (flags != ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN)
    {
        arrayUsed = pXARecoverIterator->arrayUsed - pXARecoverIterator->cursor;

        if (arrayUsed)
        {
            if (arrayUsed < arraySize)
            {
                flags |= ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN;
                toCopy = arrayUsed;
            }
            else
            {
                toCopy = arraySize;
            }
            memcpy(pXIDArray, &(pXARecoverIterator->XIDArray[pXARecoverIterator->cursor]),
                   sizeof(ism_xid_t) * toCopy);
            pXARecoverIterator->cursor += toCopy;
        }
        else
        {
            flags |= ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN;
        }
    }

    // End requested (or implied) so tidy up.
    if (flags & ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN)
    {
        iemem_free(pThreadData, iemem_globalTransactions, pSession->pXARecoverIterator);
        pSession->pXARecoverIterator = NULL;
    }

mod_exit:

    ieutTRACEL(pThreadData, toCopy, ENGINE_FNC_TRACE, FUNCTION_EXIT "Number of XIDs returned %d of %d\n",
               __func__, toCopy, arrayUsed );
    return toCopy;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Generate a hash value for an XID
///
///  @param[in] pXID               - XID 
///  @return                       - Hash value
///////////////////////////////////////////////////////////////////////////////
static uint32_t ietr_genHashId(ism_xid_t *pXID)
{
    uint32_t arrayLen = pXID->gtrid_length + pXID->bqual_length;
    uint32_t wordLen = arrayLen / sizeof(uint32_t);
    uint32_t remainder = arrayLen & 0x00000003;
    uint32_t *pKey = (uint32_t *)&(pXID->data);
    uint32_t count;
    uint32_t HashValue=0;

    // Calculate the running hash value of the pseudo-array.
    // Note: pKey must always be uint32_t aligned.
    for (count=0; count < wordLen; count++)
        HashValue ^= pKey[count];

    // And deal with any trailing bytes
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    if (remainder == 1)
    {
        HashValue ^= (pKey[count] & 0x000000ff);
    }
    else if (remainder == 2)
    {
        HashValue ^= (pKey[count] & 0x0000ffff);
    }
    else if (remainder == 3)
    {
        HashValue ^= (pKey[count] & 0x00ffffff);
    }
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    if (remainder == 1)
    {
        HashValue ^= (pKey[count] & 0xff000000);
    }
    else if (remainder == 2)
    {
        HashValue ^= (pKey[count] & 0xffff0000);
    }
    else if (remainder == 3)
    {
        HashValue ^= (pKey[count] & 0xffffff00);
    }
#else
    #error "Unknown Endian"
#endif
  
    /* Multiply by the magic number                                    */
    HashValue *= 2147483587;

    return HashValue;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Write dump descriptions for transaction structures
///
///  @param[in]     dumpHdl   Dump handle to use
///////////////////////////////////////////////////////////////////////////////
void ietr_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    iedm_describe_ietrTransactionControl_t(dump->fp);
    iedm_describe_ismEngine_Transaction_t(dump->fp);
    iedm_describe_ism_xid_t(dump->fp);
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Dump an individual transaction
///  @remark
///     This function dumps an individual transaction
///
///  @param[in]     pTran     Transaction to dump
///  @param[in]     dump      Dump handle to use
///////////////////////////////////////////////////////////////////////////////
void ietr_dumpTransaction( ismEngine_Transaction_t *pTran
                         , iedmDump_t *dump)
{
    if ((iedm_dumpStartObject(dump, pTran)) == true)
    {
        iedm_dumpStartGroup(dump, "Transaction");

        iedm_dumpData(dump, "ismEngine_Transaction_t", pTran,
                      iemem_usable_size(iemem_globalTransactions, pTran));

        // Include SLEs at detail level 6 and above
        if (dump->detailLevel > iedmDUMP_DETAIL_LEVEL_5)
        {
            ietrSLE_Header_t *pSLEHdr = pTran->pSoftLogHead;

            // If we have SLEs include them
            if (pSLEHdr != NULL)
            {
                iedm_dumpStartGroup(dump, "SLEs");

                for (; pSLEHdr != NULL; pSLEHdr=pSLEHdr->pNext)
                {
                    iedm_dumpData(dump, "ietrSLE_Header_t", pSLEHdr,
                                    pSLEHdr->TotalLength);

                    // TODO: Can we drill down any further at higher detail levels
                }

                iedm_dumpEndGroup(dump);
            }
        }

        iedm_dumpEndGroup(dump);
        iedm_dumpEndObject(dump, pTran);
    }

    return;
}

void ietr_dumpCallback( ieutThreadData_t *pThreadData
                      , char *key
                      , uint32_t keyHash
                      , void *value
                      , void *context )
{
    ietr_dumpTransaction((ismEngine_Transaction_t *)value, (iedmDump_t *)context);

    return;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Dump all or specified transactions from the global tran table
///  @remark
///     This function dumps either the requested transaction or all
///
///  @param[in]     pTran     Transaction to dump
///  @param[in]     dump      Dump handle to use
///////////////////////////////////////////////////////////////////////////////
int32_t ietr_dumpTransactions(ieutThreadData_t *pThreadData,
                              const char *XIDString,
                              iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;
    ietrTransactionControl_t *pControl = (ietrTransactionControl_t *)ismEngine_serverGlobal.TranControl;
    ism_time_t startTime, endTime, diffTime;
    double diffTimeSecs;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, XIDString,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "XIDString='%s'\n", __func__, XIDString ? XIDString : "");

    // We measure how long these dumps take for information
    startTime = ism_common_currentTimeNanos();

    iedm_dumpStartGroup(dump, "Transactions");

    ismEngine_getRWLockForRead(&pControl->GlobalTranLock);

    iedm_dumpData(dump, "ietrTransactionControl_t", pControl, iemem_usable_size(iemem_localTransactions, pControl));

    if (XIDString != NULL)
    {
        ism_xid_t XID;
        uint32_t XIDHashValue;

        rc = ism_common_StringToXid(XIDString, &XID);

        if (rc == OK)
        {
            ismEngine_Transaction_t *pTran;

            // Generate the hash value for the XID
            XIDHashValue = ietr_genHashId(&XID);

            // Look for the transaction in the global transaction table
            rc = ieut_getHashEntry( pControl->GlobalTranTable
                                  , XIDString
                                  , XIDHashValue
                                  , (void **)&pTran);

            if (rc == OK) ietr_dumpTransaction(pTran, dump);
        }
    }
    else
    {
        ieut_traverseHashTable( pThreadData
                              , pControl->GlobalTranTable
                              , ietr_dumpCallback
                              , dump );
    }

    ismEngine_unlockRWLock(&pControl->GlobalTranLock);

    iedm_dumpEndGroup(dump);

    endTime = ism_common_currentTimeNanos();

    diffTime = endTime - startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;

    ieutTRACEL(pThreadData, diffTime, ENGINE_HIGH_TRACE, "Dumping transactions took %.2f secs (%ldns)\n", diffTimeSecs, diffTime);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

ietrAsyncTransactionData_t *ietr_allocateAsyncTransactionData( ieutThreadData_t *pThreadData
                                                             , ismEngine_Transaction_t *pTran
                                                             , bool useMemReservedForCommit
                                                             , size_t customDataSize)
{
    ietrAsyncTransactionData_t *asyncInfo = NULL;
    size_t memSize = sizeof(ietrAsyncTransactionData_t) + customDataSize;
    bool usedMemPool = false;

    //Try and get the memory from the transaction mempools area reserved for it if this will be used for the commit.
    if (useMemReservedForCommit)
    {
        int32_t mempoolrc = iemp_useReservedMem( pThreadData
                                               , pTran->hTranMemPool
                                               , &memSize
                                               , (void **)&asyncInfo);

        if (mempoolrc == ISMRC_OK)
        {
            assert(asyncInfo != NULL);
            usedMemPool = true;
        }
        else
        {
            assert(0); //We should have reserved enough memory at the start of the transaction!!!
            ieutTRACEL(pThreadData, memSize, ENGINE_ERROR_TRACE, FUNCTION_IDENT "failed to use %lu reserved memory bytes\n"
                                                              , __func__, memSize);
        }
    }

    if (!usedMemPool)
    {
        //Get the memory from elsewhere
        asyncInfo =  iemem_malloc( pThreadData
                                 , IEMEM_PROBE(iemem_callbackContext, 9)
                                 , memSize);
    }

    if (asyncInfo != NULL)
    {
        ismEngine_SetStructId(asyncInfo->StrucId, ietrASYNCTRANSACTION_STRUCID);
        asyncInfo->pTran                       = NULL;
        asyncInfo->CurrPhase                   = 0;
        asyncInfo->ProcessedPhaseSLEs          = 0;  ///< number of SLEs we've processed
        asyncInfo->Record.SkippedPutCount      = 0;
        asyncInfo->Record.StoreOpCount         = 0;
        asyncInfo->Record.JobThreadId          = ietrNO_JOB_THREAD;
        asyncInfo->Record.JobRequired          = false;
        asyncInfo->fMarkedCommitOnly           = false;
        asyncInfo->fRemovedTran                = false;
        asyncInfo->hMemPool                    = usedMemPool ? pTran->hTranMemPool : 0;
        asyncInfo->memSize                     = memSize;
    }
    return asyncInfo; //Can be null if malloc fails!
}

void *ietr_getCustomDataPtr(ietrAsyncTransactionData_t *pAsyncTranData)
{
    return (void *)(pAsyncTranData+1); //Custom Data is stored after the AsyncTran Data
}

void ietr_freeAsyncTransactionData( ieutThreadData_t *pThreadData
                                  , ietrAsyncTransactionData_t **ppAsyncTranData)
{
    if (*ppAsyncTranData != NULL)
    {
        ietrAsyncTransactionData_t *pAsyncTranData = *ppAsyncTranData;

        //If it's part of the mempool we don't need to free it separately
        if (pAsyncTranData->hMemPool == NULL)
        {
            iemem_freeStruct(pThreadData, iemem_callbackContext, pAsyncTranData, pAsyncTranData->StrucId);
        }
        // If the reserved memory for the transaction is this asyncTranData
        // we should try to give it back (which we can if it's also the last
        // piece of reserved memory in the mempool) - this resolves a problem
        // where failed attempts to commit fail without going async and leave
        // the reserved memory they used in the mempool.
        else
        {
            iemp_tryReleaseReservedMem(pThreadData,
                                       pAsyncTranData->hMemPool,
                                       pAsyncTranData,
                                       pAsyncTranData->memSize);
        }

        *ppAsyncTranData = NULL;
    }
}


void ietr_increasePreResolveCount(ieutThreadData_t *pThreadData
                                 , ismEngine_Transaction_t *pTran)
{
    //We trust ourselves not to resolve transaction early (and have mechanisms to
    //delay destroy client etc and so don't do this for transactions entirely
    //inside the engine
    if (!pTran->fAsStoreTran)
    {
//ieutTRACE_HISTORYBUF(pThreadData, pTran);
        DEBUG_ONLY uint64_t oldCount = __sync_fetch_and_add(&(pTran->preResolveCount), 1);
        assert (oldCount != 0);
//ieutTRACE_HISTORYBUF(pThreadData, oldCount);
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     reduce the number of inflight transactional operations
///
///  @param[in]     pTran                   Transaction
///  @param[in]     issueResolveCallbacks   false if called directly from resolve
///                                         operation, true if called asyncly from
///                                         elsewhere
///////////////////////////////////////////////////////////////////////////////
void ietr_decreasePreResolveCount( ieutThreadData_t *pThreadData
                                 , ismEngine_Transaction_t *pTran
                                 , bool issueResolveCallbacks)
{
    //We trust ourselves not to resolve transaction early (and have mechanisms to
    //delay destroy client etc and so don't do this for transactions entirely
    //inside the engine
    if (pTran->fAsStoreTran)
    {
        //As we are not counting... we finish when the resolve operation wants
        //to finish (== call with issueResolveCallbacks=false)
        if(!issueResolveCallbacks)
        {
            assert(pTran->CompletionStage == ietrCOMPLETION_STARTED);
            ietr_finishRollback(pThreadData, pTran);
        }
    }
    else
    {
//ieutTRACE_HISTORYBUF(pThreadData, pTran);
        DEBUG_ONLY uint64_t oldCount = __sync_fetch_and_sub(&(pTran->preResolveCount), 1);
//ieutTRACE_HISTORYBUF(pThreadData, oldCount);
        assert (oldCount != 0);

        if (oldCount == 1)
        {
            //everything is finished, we can start to do the resolve. Currently
            //the only resolve we expect to be queued at the moment is roll back
            //(from session close)
            if(issueResolveCallbacks && !(pTran->fDelayedRollback))
            {
                ieutTRACE_FFDC( ieutPROBE_001, true
                              , "Transaction with delayed operations not doing delayed rollback", ISMRC_Error
                              , "Transaction", pTran, sizeof(ismEngine_Transaction_t)
                              , NULL );
            }
            assert(pTran->CompletionStage == ietrCOMPLETION_STARTED);
            ietr_finishRollback(pThreadData, pTran);
        }
    }
}
/*********************************************************************/
/*                                                                   */
/* End of Transaction.c                                              */
/*                                                                   */
/*********************************************************************/
