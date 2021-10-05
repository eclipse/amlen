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
/// @file  storeMQRecords.c
/// @brief Engine module for functions to store MQ Records
//****************************************************************************
#define TRACE_COMP Engine

#include <stdio.h>
#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "engineStore.h"
#include "engineRestore.h"
#include "memHandler.h"
#include "transaction.h"
#include "storeMQRecordsInternal.h"

/*
 * Replays a soft-log entry - this is the commit/rollback for a single operation.
 * This runs within the auspices of a store-transaction, but it must not perform
 * operations which can run out of space, nor must it commit or roll back the
 * store-transaction. 
 */
void iesm_SLEReplayAddQMgr(ietrReplayPhase_t phase,
                           ieutThreadData_t *pThreadData,
                           ismEngine_Transaction_t *pTran,
                           void *pEntry,
                           ietrReplayRecord_t *pRecord);

void iesm_SLEReplayRmvQMgr(ietrReplayPhase_t phase,
                           ieutThreadData_t *pThreadData,
                           ismEngine_Transaction_t *pTran,
                           void *pEntry,
                           ietrReplayRecord_t *pRecord);


//****************************************************************************
// @brief  Create Queue-Manager Record
//
// Creates a queue-manager record. This is a persistent record related to
// a WebSphere MQ queue manager.
//
// @param[in]     hSession         Session handle
// @param[in]     pData            Ptr to queue-manager record's data
// @param[in]     dataLength       Length of queue-manager record's data
// @param[out]    phQMgrRec        Returned queue-manager record handle
// @param[in]     pContext         Optional context for completion callback
// @param[in]     contextLength    Length of data pointed to by pContext
// @param[in]     pCallbackFn      Operation-completion callback
//
// @return OK on successful completion or an ISMRC_ value.
//
// @remark The operation may complete synchronously or asynchronously. If it
// completes synchronously, the return code indicates the completion
// status.
//
// If the operation completes asynchronously, the return code from
// this function will be ISMRC_AsyncCompletion. The actual
// completion of the operation will be signalled by a call to the
// operation-completion callback, if one is provided. When the
// operation becomes asynchronous, a copy of the context will be
// made into memory owned by the Engine. This will be freed upon
// return from the operation-completion callback. Because the
// completion is asynchronous, the call to the operation-completion
// callback might occur before this function returns.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createQManagerRecord(
    ismEngine_SessionHandle_t           hSession,
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QManagerRecordHandle_t *  phQMgrRec,
    void *                              pContext,
    size_t                              contextLength,
    ismEngine_CompletionCallback_t      pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    iesmQManagerRecord_t *pQMgrRec = NULL;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p)\n", __func__, hSession);

    // Allocate a new queue-manager record
    rc = iesm_newQManagerRecord(pThreadData, pData, dataLength, &pQMgrRec);
    if (rc == OK)
    {
        rc = iesm_storeBridgeQMgrRecord(pThreadData, pQMgrRec);
        if (rc == OK)
        {
            iesm_lockMQStoreState();
            iesm_addQManagerRecord(pQMgrRec);
            iesm_unlockMQStoreState();

            *phQMgrRec = pQMgrRec;
        }
        else
        {
            iesm_freeQManagerRecord(pThreadData, pQMgrRec);
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, hQMgrRec=%p\n", __func__, rc, *phQMgrRec);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
// @brief  Destroy Queue-Manager Record
//
// Destroys a queue-manager record. This is a persistent record related to
// a WebSphere MQ queue manager.
//
// @param[in]     hSession         Session handle
// @param[in]     hQMgrRec         Queue-manager record handle
// @param[in]     pContext         Optional context for completion callback
// @param[in]     contextLength    Length of data pointed to by pContext
// @param[in]     pCallbackFn      Operation-completion callback
//
// @return OK on successful completion or an ISMRC_ value.
//
// @remark The operation may complete synchronously or asynchronously. If it
// completes synchronously, the return code indicates the completion
// status.
//
// If the operation completes asynchronously, the return code from
// this function will be ISMRC_AsyncCompletion. The actual
// completion of the operation will be signalled by a call to the
// operation-completion callback, if one is provided. When the
// operation becomes asynchronous, a copy of the context will be
// made into memory owned by the Engine. This will be freed upon
// return from the operation-completion callback. Because the
// completion is asynchronous, the call to the operation-completion
// callback might occur before this function returns.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_destroyQManagerRecord(
    ismEngine_SessionHandle_t           hSession,
    ismEngine_QManagerRecordHandle_t    hQMgrRec,
    void *                              pContext,
    size_t                              contextLength,
    ismEngine_CompletionCallback_t      pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    iesmQManagerRecord_t *pQMgrRec = (iesmQManagerRecord_t *)hQMgrRec;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, hQMgrRec=%p)\n", __func__, hSession, hQMgrRec);

    // Remove the queue-manager record from the list
    iesm_lockMQStoreState();
    if (pQMgrRec->UseCount > 0)
    {
        iesm_unlockMQStoreState();

        rc = ISMRC_LockNotGranted;
        ism_common_setError(rc);
    }
    else
    {
        iesm_unlockMQStoreState();

        rc = iesm_unstoreBridgeQMgrRecord(pThreadData, pQMgrRec);
        if (rc == OK)
        {
            iesm_lockMQStoreState();
            iesm_removeQManagerRecord(pQMgrRec);
            iesm_unlockMQStoreState();

            iesm_freeQManagerRecord(pThreadData, pQMgrRec);
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
// @brief  List Queue-Manager Records
//
// Lists the queue-manager records. The callback MUST NOT call any Engine operations.
//
// @param[in]     hSession         Session handle
// @param[in]     pContext         Optional context for queue-manager record callback
// @param[in]     pQMgrRecCallbackFunction  Queue-manager record callback
//
// @return OK on successful completion or an ISMRC_ value.
//
// @remark The callback is called with the queue-manager records. It is
// intended for use during MQ Bridge initialisation to repopulate the message
// delivery information. This callback MUST NOT call any Engine operations.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_listQManagerRecords(
    ismEngine_SessionHandle_t           hSession,
    void *                              pContext,
    ismEngine_QManagerRecordCallback_t  pQMgrRecCallbackFunction)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p)\n", __func__, hSession);

    // Iterate over the list of queue-manager records under the MQ store state lock
    iesm_lockMQStoreState();

    iesmQManagerRecord_t *pQMgrRec = ismEngine_serverGlobal.QueueManagerRecordHead;
    while (pQMgrRec != NULL)
    {
        (*pQMgrRecCallbackFunction)(pQMgrRec->pData,
                                    pQMgrRec->DataLength,
                                    pQMgrRec,
                                    pContext);

        pQMgrRec = pQMgrRec->pNext;
    }

    iesm_unlockMQStoreState();

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
// @brief  Create Queue-Manager XID Record
//
// Creates a queue-manager XID record. This is a persistent record related to
// a transaction with a WebSphere MQ queue manager.
//
// @param[in]     hSession         Session handle
// @param[in]     hQMgrRec         Queue-manager record handle
// @param[in]     hTran            Transaction handle, may be NULL
// @param[in]     pData            Ptr to queue-manager XID record's data
// @param[in]     dataLength       Length of queue-manager XID record's data
// @param[out]    phQMgrXidRec     Returned queue-manager XID record handle
// @param[in]     pContext         Optional context for completion callback
// @param[in]     contextLength    Length of data pointed to by pContext
// @param[in]     pCallbackFn      Operation-completion callback
//
// @return OK on successful completion or an ISMRC_ value.
//
// @remark The operation may complete synchronously or asynchronously. If it
// completes synchronously, the return code indicates the completion
// status.
//
// If the operation completes asynchronously, the return code from
// this function will be ISMRC_AsyncCompletion. The actual
// completion of the operation will be signalled by a call to the
// operation-completion callback, if one is provided. When the
// operation becomes asynchronous, a copy of the context will be
// made into memory owned by the Engine. This will be freed upon
// return from the operation-completion callback. Because the
// completion is asynchronous, the call to the operation-completion
// callback might occur before this function returns.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createQMgrXidRecord(
    ismEngine_SessionHandle_t           hSession,
    ismEngine_QManagerRecordHandle_t    hQMgrRec,
    ismEngine_TransactionHandle_t       hTran,
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QMgrXidRecordHandle_t *   phQMgrXidRec,
    void *                              pContext,
    size_t                              contextLength,
    ismEngine_CompletionCallback_t      pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    iesmQManagerRecord_t *pQMgrRec = (iesmQManagerRecord_t *)hQMgrRec;
    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)hTran;
    iesmQMgrXidRecord_t *pQMgrXidRec = NULL;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, hQMgrRec=%p)\n", __func__, hSession, hQMgrRec);

    // Allocate a new queue-manager XID record
    rc = iesm_newQMgrXidRecord(pThreadData, pData, dataLength, &pQMgrXidRec);
    if (rc == OK)
    {
        iesm_lockMQStoreState();

        iesm_addQMgrXidRecord(pQMgrRec, pQMgrXidRec);
        if (pTran != NULL)
        {
            pQMgrXidRec->fUncommitted = true;
        }

        iesm_unlockMQStoreState();

        // Now store the record
        rc = iesm_storeBridgeXidRecord(pThreadData, pQMgrXidRec, pTran);
        if (rc == OK)
        {
            *phQMgrXidRec = pQMgrXidRec;
        }
        else
        {
            iesm_lockMQStoreState();
            iesm_removeQMgrXidRecord(pQMgrXidRec);
            iesm_unlockMQStoreState();

            iesm_freeQMgrXidRecord(pThreadData, pQMgrXidRec);
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, hQMXidRec=%p\n", __func__, rc, *phQMgrXidRec);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
// @brief  Destroy Queue-Manager XID Record
//
// Destroys a queue-manager XID record. This is a persistent record related to
// a transaction with a WebSphere MQ queue manager.
//
// @param[in]     hSession         Session handle
// @param[in]     hQMgrXidRec      Queue-manager XID record handle
// @param[in]     hTran            Transaction handle, may be NULL
// @param[in]     pContext         Optional context for completion callback
// @param[in]     contextLength    Length of data pointed to by pContext
// @param[in]     pCallbackFn      Operation-completion callback
//
// @return OK on successful completion or an ISMRC_ value.
//
// @remark The operation may complete synchronously or asynchronously. If it
// completes synchronously, the return code indicates the completion
// status.
//
// If the operation completes asynchronously, the return code from
// this function will be ISMRC_AsyncCompletion. The actual
// completion of the operation will be signalled by a call to the
// operation-completion callback, if one is provided. When the
// operation becomes asynchronous, a copy of the context will be
// made into memory owned by the Engine. This will be freed upon
// return from the operation-completion callback. Because the
// completion is asynchronous, the call to the operation-completion
// callback might occur before this function returns.
//***************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_destroyQMgrXidRecord(
    ismEngine_SessionHandle_t           hSession,
    ismEngine_QMgrXidRecordHandle_t     hQMgrXidRec,
    ismEngine_TransactionHandle_t       hTran,
    void *                              pContext,
    size_t                              contextLength,
    ismEngine_CompletionCallback_t      pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    iesmQMgrXidRecord_t *pQMgrXidRec = (iesmQMgrXidRecord_t *)hQMgrXidRec;
    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)hTran;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, hQMgrXidRec=%p)\n", __func__, hSession, hQMgrXidRec);

    // Remove the queue-manager XID record from the list
    iesm_lockMQStoreState();
    if (pQMgrXidRec->fUncommitted)
    {
        rc = ISMRC_LockNotGranted;
        ism_common_setError(rc);
    }
    else
    {
        // Mark as uncommitted while we unstore it
        pQMgrXidRec->fUncommitted = true;
    }
    iesm_unlockMQStoreState();

    if (rc == OK)
    {
        rc = iesm_unstoreBridgeXidRecord(pThreadData, pQMgrXidRec, pTran);
        if (rc == OK)
        {
            if (pTran == NULL)
            {
                iesm_lockMQStoreState();
                iesm_removeQMgrXidRecord(pQMgrXidRec);
                iesm_unlockMQStoreState();

                iesm_freeQMgrXidRecord(pThreadData, pQMgrXidRec);
            }
        }
        else
        {
            iesm_lockMQStoreState();
            pQMgrXidRec->fUncommitted = false;
            iesm_unlockMQStoreState();
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
// @brief  List Queue-Manager XID Records
//
// Lists the queue-manager XID records for the supplied queue-manager record.
// The callback MUST NOT call any Engine operations.
//
// @param[in]     hSession         Session handle
// @param[in]     hQMgrRec         Queue-manager record handle
// @param[in]     pContext         Optional context for queue-manager XID record callback
// @param[in]     pQMgrXidRecCallbackFunction  Queue-manager XID record callback
//
// @return OK on successful completion or an ISMRC_ value.
//
// @remark The callback is called with the queue-manager XID records for the supplied
// queue-manager record. It is intended for use during MQ Bridge initialisation
// to repopulate the message delivery information. This callback MUST NOT call
// any Engine operations.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_listQMgrXidRecords(
    ismEngine_SessionHandle_t           hSession,
    ismEngine_QManagerRecordHandle_t    hQMgrRec,
    void *                              pContext,
    ismEngine_QMgrXidRecordCallback_t   pQMgrXidRecCallbackFunction)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    iesmQManagerRecord_t *pQMgrRec = (iesmQManagerRecord_t *)hQMgrRec;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, hQMgrRec=%p)\n", __func__, hSession, hQMgrRec);

    // Iterate over the list of queue-manager XID records under the MQ store state lock
    iesm_lockMQStoreState();

    iesmQMgrXidRecord_t *pQMgrXidRec = pQMgrRec->pXidHead;
    while (pQMgrXidRec != NULL)
    {
        if (!pQMgrXidRec->fUncommitted)
        {
            (*pQMgrXidRecCallbackFunction)(pQMgrXidRec->pData,
                                           pQMgrXidRec->DataLength,
                                           pQMgrXidRec,
                                           pContext);
        }

        pQMgrXidRec = pQMgrXidRec->pNext;
    }

    iesm_unlockMQStoreState();

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


/*
 * Locks the MQ store state held by the Engine
 */
void iesm_lockMQStoreState(void)
{
    int32_t rc = OK;

    int osrc = pthread_mutex_lock(&ismEngine_serverGlobal.MQStoreMutex);
    if (UNLIKELY(osrc != 0))
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);
        ieutTRACE_FFDC(1, true,
                       "pthread_mutex_lock failed", rc,
                       "osrc", &osrc, sizeof(osrc),
                       NULL);
    }
}


/*
 * Unlocks the MQ store state held by the Engine
 */
void iesm_unlockMQStoreState(void)
{
    (void)pthread_mutex_unlock(&ismEngine_serverGlobal.MQStoreMutex);
}

/*
 * Allocate a new queue manager record
 */
int32_t iesm_newQManagerRecord(ieutThreadData_t *pThreadData,
                               void *pData,
                               size_t dataLength,
                               iesmQManagerRecord_t **ppQMgrRec)
{
    iesmQManagerRecord_t *pQMgrRec = NULL;
    int32_t rc = OK;

    pQMgrRec = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_mqBridgeRecords, 1), sizeof(iesmQManagerRecord_t) + dataLength);
    if (pQMgrRec != NULL)
    {
        memcpy(pQMgrRec->StrucId, iesmQ_MANAGER_RECORD_STRUCID, 4);
        pQMgrRec->UseCount = 0;
        pQMgrRec->pPrev = NULL;
        pQMgrRec->pNext = NULL;
        pQMgrRec->pXidHead = NULL;
        pQMgrRec->pXidTail = NULL;
        pQMgrRec->hStoreBMR = ismSTORE_NULL_HANDLE;
        pQMgrRec->pData = pQMgrRec + 1;
        pQMgrRec->DataLength = dataLength;
        memcpy(pQMgrRec->pData, pData, dataLength);
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

    if (rc == OK)
    {
        *ppQMgrRec = pQMgrRec;
    }

    return rc;
}


/*
 * Frees a queue manager record
 */
void iesm_freeQManagerRecord(ieutThreadData_t *pThreadData, iesmQManagerRecord_t *pQMgrRec)
{
    iemem_freeStruct(pThreadData, iemem_mqBridgeRecords, pQMgrRec, pQMgrRec->StrucId);
}


/*
 * Adds a queue-manager record to the list
 */
void iesm_addQManagerRecord(iesmQManagerRecord_t *pQMgrRec)
{
    assert(pQMgrRec->pNext == NULL);
    assert(pQMgrRec->pPrev == NULL);

    // Add the queue-manager record to the tail of the list
    if (ismEngine_serverGlobal.QueueManagerRecordHead == NULL)
    {
        ismEngine_serverGlobal.QueueManagerRecordHead = pQMgrRec;
        ismEngine_serverGlobal.QueueManagerRecordTail = pQMgrRec;
    }
    else
    {
        pQMgrRec->pPrev = ismEngine_serverGlobal.QueueManagerRecordTail;
        pQMgrRec->pPrev->pNext = pQMgrRec;
        ismEngine_serverGlobal.QueueManagerRecordTail = pQMgrRec;
    }
}


/*
 * Removes a queue-manager record from the list
 */
void iesm_removeQManagerRecord(iesmQManagerRecord_t *pQMgrRec)
{
    if (pQMgrRec->pPrev == NULL)
    {
        if (pQMgrRec->pNext == NULL)
        {
            assert(ismEngine_serverGlobal.QueueManagerRecordHead == pQMgrRec);
            assert(ismEngine_serverGlobal.QueueManagerRecordTail == pQMgrRec);

            ismEngine_serverGlobal.QueueManagerRecordHead = NULL;
            ismEngine_serverGlobal.QueueManagerRecordTail = NULL;
        }
        else
        {
            assert(ismEngine_serverGlobal.QueueManagerRecordHead == pQMgrRec);

            pQMgrRec->pNext->pPrev = NULL;
            ismEngine_serverGlobal.QueueManagerRecordHead = pQMgrRec->pNext;
            pQMgrRec->pNext = NULL;
        }
    }
    else
    {
        if (pQMgrRec->pNext == NULL)
        {
            assert(ismEngine_serverGlobal.QueueManagerRecordTail == pQMgrRec);

            pQMgrRec->pPrev->pNext = NULL;
            ismEngine_serverGlobal.QueueManagerRecordTail = pQMgrRec->pPrev;
            pQMgrRec->pPrev = NULL;
        }
        else
        {
            pQMgrRec->pPrev->pNext = pQMgrRec->pNext;
            pQMgrRec->pNext->pPrev = pQMgrRec->pPrev;
            pQMgrRec->pPrev = NULL;
            pQMgrRec->pNext = NULL;
        }
    }
}


/*
 * Add a Bridge Queue-Manager Record to the Store
 */
int32_t iesm_storeBridgeQMgrRecord(ieutThreadData_t *pThreadData,
                                   iesmQManagerRecord_t *pQMgrRec)
{
    ismStore_Record_t storeRecord;
    ismStore_Handle_t hStoreBMR;
    iestBridgeQMgrRecord_t bridgeQMgrRec;
    uint32_t dataLength;
    uint32_t fragsCount = 1;
    char *pFrags[2];
    uint32_t fragsLength[2];
    int32_t rc = OK;

    fragsCount = 2;
    pFrags[0] = (char *)&bridgeQMgrRec;
    fragsLength[0] = sizeof(bridgeQMgrRec);
    dataLength = sizeof(bridgeQMgrRec);

    pFrags[1] = (char *)pQMgrRec->pData;
    fragsLength[1] = pQMgrRec->DataLength;
    dataLength += pQMgrRec->DataLength;

    memcpy(bridgeQMgrRec.StrucId, iestBRIDGE_QMGR_RECORD_STRUCID, 4);
    bridgeQMgrRec.Version = iestBMR_VERSION_1;
    bridgeQMgrRec.DataLength = pQMgrRec->DataLength;

    storeRecord.Type = ISM_STORE_RECTYPE_BMGR;
    storeRecord.FragsCount = fragsCount;
    storeRecord.pFrags = pFrags;
    storeRecord.pFragsLengths = fragsLength;
    storeRecord.DataLength = dataLength;
    storeRecord.Attribute = ismSTORE_NULL_HANDLE;
    storeRecord.State = iestBMR_STATE_NONE;

    while (true)
    {
        rc = ism_store_createRecord(pThreadData->hStream,
                                    &storeRecord,
                                    &hStoreBMR);
        if (rc == OK)
        {
            pQMgrRec->hStoreBMR = hStoreBMR;
            iest_store_commit(pThreadData, false);
            break;
        }
        else
        {
            if (rc == ISMRC_StoreGenerationFull)
            {
                iest_store_rollback(pThreadData, false);
                // Round we go again
            }
            else
            {
                iest_store_rollback(pThreadData, false);
                break;
            }
        }
    }

    return rc;
}


/*
 * Remove a Bridge Queue-Manager Record from the Store
 */
int32_t iesm_unstoreBridgeQMgrRecord(ieutThreadData_t *pThreadData,
                                     iesmQManagerRecord_t *pQMgrRec)
{
    int32_t rc = OK;

    if (pQMgrRec->hStoreBMR != ismSTORE_NULL_HANDLE)
    {
        rc = ism_store_deleteRecord(pThreadData->hStream,
                                    pQMgrRec->hStoreBMR);
        if (rc == OK)
        {
            pQMgrRec->hStoreBMR = ismSTORE_NULL_HANDLE;
            iest_store_commit(pThreadData, false);
        }
        else
        {
            assert(rc != ISMRC_StoreGenerationFull);
            iest_store_rollback(pThreadData, false);
        }
    }

    return rc;
}


/*
 * Rehydrates a Bridge Queue-Manager Record from the Store during recovery
 */
int32_t iesm_rehydrateBridgeQMgrRecord(ieutThreadData_t *pThreadData,
                                       ismStore_Record_t *pStoreRecord,
                                       ismStore_Handle_t hStoreRecord,
                                       void **rehydratedRecord)
{
    iestBridgeQMgrRecord_t *pBMR;
    iesmQManagerRecord_t *pQMgrRec = NULL;
    int32_t rc = OK;

    assert(pStoreRecord->Type == ISM_STORE_RECTYPE_BMGR);
    assert(pStoreRecord->FragsCount == 1);
    assert(pStoreRecord->pFragsLengths[0] >= sizeof(iestBridgeQMgrRecord_t));

    pBMR = (iestBridgeQMgrRecord_t *)(pStoreRecord->pFrags[0]);

    uint32_t dataLength;
    char *tmpPtr;

    if (LIKELY(pBMR->Version == iestBMR_CURRENT_VERSION))
    {
        dataLength = pBMR->DataLength;
        tmpPtr = (char *)(pBMR + 1);
    }
    else
    {
        rc = ISMRC_InvalidValue;
        ism_common_setErrorData(rc, "%u", pBMR->Version);
        goto mod_exit;
    }

    rc = iesm_newQManagerRecord(pThreadData,
                                tmpPtr,
                                dataLength,
                                &pQMgrRec);
    if (rc == OK)
    {
        pQMgrRec->hStoreBMR = hStoreRecord;

        iesm_addQManagerRecord(pQMgrRec);
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

mod_exit:

    if (rc == OK)
    {
        *rehydratedRecord = pQMgrRec;
    }
    else
    {
        ierr_recordBadStoreRecord( pThreadData
                                 , pStoreRecord->Type
                                 , hStoreRecord
                                 , NULL
                                 , rc);
    }

    return rc;
}


/*
 * Allocate a new queue-manager XID record
 */
int32_t iesm_newQMgrXidRecord(ieutThreadData_t *pThreadData,
                              void *pData,
                              size_t dataLength,
                              iesmQMgrXidRecord_t **ppQMgrXidRec)
{
    iesmQMgrXidRecord_t *pQMgrXidRec = NULL;
    int32_t rc = OK;

    pQMgrXidRec = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_mqBridgeRecords, 2), sizeof(iesmQMgrXidRecord_t) + dataLength);
    if (pQMgrXidRec != NULL)
    {
        memcpy(pQMgrXidRec->StrucId, iesmQ_MGR_XID_RECORD_STRUCID, 4);
        pQMgrXidRec->fUncommitted = false;
        pQMgrXidRec->pQMgrRec = NULL;
        pQMgrXidRec->pPrev = NULL;
        pQMgrXidRec->pNext = NULL;
        pQMgrXidRec->pData = pQMgrXidRec + 1;
        pQMgrXidRec->DataLength = dataLength;
        memcpy(pQMgrXidRec->pData, pData, dataLength);
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

    if (rc == OK)
    {
        *ppQMgrXidRec = pQMgrXidRec;
    }

    return rc;
}


/*
 * Frees a queue-manager XID record
 */
void iesm_freeQMgrXidRecord(ieutThreadData_t *pThreadData, iesmQMgrXidRecord_t *pQMgrXidRec)
{
    iemem_freeStruct(pThreadData, iemem_mqBridgeRecords, pQMgrXidRec, pQMgrXidRec->StrucId);
}


/*
 * Adds a queue-manager XID record to the list
 */
void iesm_addQMgrXidRecord(iesmQManagerRecord_t *pQMgrRec, iesmQMgrXidRecord_t *pQMgrXidRec)
{
    assert(pQMgrXidRec->pNext == NULL);
    assert(pQMgrXidRec->pPrev == NULL);
    assert(pQMgrXidRec->pQMgrRec == NULL);

    // Add the queue-manager XID record to the tail of the list
    pQMgrXidRec->pQMgrRec = pQMgrRec;
    pQMgrXidRec->pQMgrRec->UseCount++;

    if (pQMgrRec->pXidHead == NULL)
    {
        pQMgrRec->pXidHead = pQMgrXidRec;
        pQMgrRec->pXidTail = pQMgrXidRec;
    }
    else
    {
        pQMgrXidRec->pPrev = pQMgrRec->pXidTail;
        pQMgrRec->pXidTail->pNext = pQMgrXidRec;
        pQMgrRec->pXidTail = pQMgrXidRec;
    }
}


/*
 * Removes a queue-manager XID record from the list
 */
void iesm_removeQMgrXidRecord(iesmQMgrXidRecord_t *pQMgrXidRec)
{
    iesmQManagerRecord_t *pQMgrRec = pQMgrXidRec->pQMgrRec;
    assert(pQMgrRec != NULL);

    pQMgrXidRec->pQMgrRec->UseCount--;

    if (pQMgrXidRec->pPrev == NULL)
    {
        if (pQMgrXidRec->pNext == NULL)
        {
            assert(pQMgrRec->pXidHead == pQMgrXidRec);
            assert(pQMgrRec->pXidTail == pQMgrXidRec);

            pQMgrRec->pXidHead = NULL;
            pQMgrRec->pXidTail = NULL;
        }
        else
        {
            assert(pQMgrRec->pXidHead == pQMgrXidRec);

            pQMgrXidRec->pNext->pPrev = NULL;
            pQMgrRec->pXidHead = pQMgrXidRec->pNext;
            pQMgrXidRec->pNext = NULL;
        }
    }
    else
    {
        if (pQMgrXidRec->pNext == NULL)
        {
            assert(pQMgrRec->pXidTail == pQMgrXidRec);

            pQMgrXidRec->pPrev->pNext = NULL;
            pQMgrRec->pXidTail = pQMgrXidRec->pPrev;
            pQMgrXidRec->pPrev = NULL;
        }
        else
        {
            pQMgrXidRec->pPrev->pNext = pQMgrXidRec->pNext;
            pQMgrXidRec->pNext->pPrev = pQMgrXidRec->pPrev;
            pQMgrXidRec->pPrev = NULL;
            pQMgrXidRec->pNext = NULL;
        }
    }

    pQMgrXidRec->pQMgrRec = NULL;
}


/*
 * Add Bridge XID Record to the Store and optionally a transaction
 *
 * NOTE: BXRs are no longer written into the Store, but we support
 *       transactional updates to them in a volatile way.
 */
int32_t iesm_storeBridgeXidRecord(ieutThreadData_t *pThreadData,
                                  iesmQMgrXidRecord_t *pQMgrXidRec,
                                  ismEngine_Transaction_t *pTran)
{
    ietrSLE_Header_t *pSLE = NULL;
    int32_t rc = OK;

    // If this is transactional, we need a soft-log entry to make it part of the transaction
    if (pTran)
    {
        iesmSLEAddQMgrXID_t SLEData;

        SLEData.pQMgrXidRec = pQMgrXidRec;
        rc = ietr_softLogAdd( pThreadData
                            , pTran
                            , ietrSLE_SM_ADDQMGRXID
                            , iesm_SLEReplayAddQMgr
                            , NULL
                            , Commit | Rollback
                            , &SLEData
                            , sizeof(SLEData)
                            , 0, 0
                            , &pSLE);
    }

    return rc;
}


/*
 * Remove Bridge XID Record from the Store and optionally a transaction
 *
 * NOTE: BXRs are no longer written into the Store, but we support
 *       transactional updates to them in a volatile way.
 */
int32_t iesm_unstoreBridgeXidRecord(ieutThreadData_t *pThreadData,
                                    iesmQMgrXidRecord_t *pQMgrXidRec,
                                    ismEngine_Transaction_t *pTran)
{
    ietrSLE_Header_t *pSLE = NULL;
    int32_t rc = OK;

    // If this is transactional, we need a soft-log entry to make it part of the transaction
    if (pTran)
    {
        iesmSLERemoveQMgrXID_t SLEData;

        SLEData.pQMgrXidRec = pQMgrXidRec;
        rc = ietr_softLogAdd( pThreadData
                            , pTran
                            , ietrSLE_SM_RMVQMGRXID
                            , iesm_SLEReplayRmvQMgr
                            , NULL
                            , Commit | Rollback
                            , &SLEData
                            , sizeof(SLEData)
                            , 0, 0
                            , &pSLE);
    }

    return rc;
}


/*
 * Replays a soft-log entry - this is the commit/rollback for a single operation.
 * This runs within the auspices of a store-transaction, but it must not perform
 * operations which can run out of space, nor must it commit or roll back the
 * store-transaction.
 */
void iesm_SLEReplayAddQMgr(ietrReplayPhase_t phase,
                           ieutThreadData_t *pThreadData,
                           ismEngine_Transaction_t *pTran,
                           void *pEntry,
                           ietrReplayRecord_t *pRecord)
{
    iesmSLEAddQMgrXID_t *pSLEAdd = (iesmSLEAddQMgrXID_t *)pEntry;
    iesmQMgrXidRecord_t *pQMgrXidRec = pSLEAdd->pQMgrXidRec;

    assert(pQMgrXidRec->fUncommitted);

    switch (phase)
    {
        case Commit:
            {
                iesm_lockMQStoreState();
                pQMgrXidRec->fUncommitted = false;
                iesm_unlockMQStoreState();
            }
            break;

        case Rollback:
            {
                iesm_lockMQStoreState();
                iesm_removeQMgrXidRecord(pQMgrXidRec);
                iesm_unlockMQStoreState();

                iesm_freeQMgrXidRecord(pThreadData, pQMgrXidRec);
            }
            break;

        default:
            assert(false);
            break;
    }
}

void iesm_SLEReplayRmvQMgr(ietrReplayPhase_t phase,
                    ieutThreadData_t *pThreadData,
                    ismEngine_Transaction_t *pTran,
                    void *pEntry,
                    ietrReplayRecord_t *pRecord)
{
    iesmSLERemoveQMgrXID_t *pSLERmv = (iesmSLERemoveQMgrXID_t *)pEntry;
    iesmQMgrXidRecord_t *pQMgrXidRec = pSLERmv->pQMgrXidRec;

    assert(pQMgrXidRec->fUncommitted);

    switch (phase)
    {
        case Commit:
            {
                iesm_lockMQStoreState();
                iesm_removeQMgrXidRecord(pQMgrXidRec);
                iesm_unlockMQStoreState();

                iesm_freeQMgrXidRecord(pThreadData, pQMgrXidRec);
            }
            break;

        case Rollback:
            {
                iesm_lockMQStoreState();
                pQMgrXidRec->fUncommitted = false;
                iesm_unlockMQStoreState();
            }
            break;

        default:
            assert(false);
            break;
    }
}

/*********************************************************************/
/*                                                                   */
/* End of storeMQRecords.c                                           */
/*                                                                   */
/*********************************************************************/
