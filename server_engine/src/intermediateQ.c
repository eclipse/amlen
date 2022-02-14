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
///  @file  intermediateQ.c
///
///  @brief
///   Code for intermediate queue that doesn't support persistence/transactions
///
///  @remarks
///    The queue is multi-producer, single consumer designed for MQTT QoS 1
///    & 2 subscriptions.
///    @par
///    The intermediate queue is based upon the simple queue designed for
///    MQTT QoS 0 and many of the functions remain very similar.
///    @par
///    The queue is based upon a structure (ieiqQueue_t) which contains
///    a linked list of pages (ieiqQNodePage_t).  Each page is an array of
///    Nodes (ieiqQNode_t) which contains a reference to a messages as well as
///    the message state. The queue always has at least 2 ieiqQNodePage_t
///    structures in the linked list. A new page is added by the putter
///    responsible for filling the penultimate page. Pages are freed when
///    the last message on a page is consumed.
///    @par
///    The Queue structure contains 3 set of references to the list
///    of pages/messages.
///    @li head       - pointer to the node containing the oldest unconsumed
///    message on the queue. Updating of these fields in protected by the
///    headlock spinlock.
///    @li headPage   - pointer to the oldest page in the queue
///    @li cursor     - pointer to the oldest undelivered messages on the
///    queue. Updating this field is protected by this thread CAS the value
///    in the waiterStatus field to getting.
///    @li tail       - pointer to the next free node in the queue. Updating
///    this field is protected by the the putlock spinlock.
///
///    @see The header file intermediateQ.h contains the definitions and
///    function prototypes for functions which may be called from outwith
///    this source file.
///    @see The header file queueCommon.h contains the definition of the
///    interface to which these external functions must adhere.
///    @see The header file intermediateQInternal.h contains the private
///    data declarations for this source file.
///
///////////////////////////////////////////////////////////////////////////////
#define TRACE_COMP Engine

#include <stddef.h>
#include <assert.h>
#include <stdint.h>

#include "intermediateQ.h"
#include "intermediateQInternal.h"
#include "queueCommonInternal.h"
#include "messageDelivery.h"
#include "engineDump.h"
#include "engineStore.h"
#include "transaction.h"
#include "store.h"
#include "memHandler.h"
#include "clientState.h"
#include "waiterStatus.h"
#include "messageExpiry.h"
#include "engineAsync.h"
#include "mempool.h"
#include "resourceSetStats.h"

///////////////////////////////////////////////////////////////////////////////
// Local function prototypes - see definition for more info
///////////////////////////////////////////////////////////////////////////////
static int32_t ieiq_putToWaitingGetter( ieutThreadData_t *pThreadData
                                      , ieiqQueue_t *q
                                      , ismEngine_Message_t *msg
                                      , uint8_t msgFlags 
                                      , ismEngine_DelivererContext_t * delivererContext );
static inline ieiqQNodePage_t *ieiq_createNewPage(ieutThreadData_t *pThreadData,
                                                  ieiqQueue_t *Q,
                                                  uint32_t nodesInPage,
                                                  ieiqPageSeqNo_t prevSeqNo);
static inline ieiqQNodePage_t *ieiq_getPageFromEnd( ieiqQNode_t *node );
static inline ieiqQNodePage_t *ieiq_moveToNewPage( ieutThreadData_t *pThreadData
                                                 , ieiqQueue_t *Q
                                                 , ieiqQNode_t *endMsg);
static inline int32_t ieiq_locateMessage( ieutThreadData_t *pThreadData
                                        , ieiqQueue_t       *Q
                                        , bool              allowRewindCursor
                                        , ieiqQNode_t       **ppnode);
static uint32_t ieiq_redeliverMessage( ieutThreadData_t *pThreadData
                                     , ieiqQueue_t *Q
                                     , ieiqQNode_t *pnode
                                     , bool updateDeliveryCount );
static inline void ieiq_moveGetCursor( ieutThreadData_t *pThreadData
                                     , ieiqQueue_t *Q
                                     , ieiqQNode_t *pcursor );
static void ieiq_cleanupHeadPage( ieutThreadData_t *pThreadData
                                , ieiqQueue_t *Q );

static inline void ieiq_resetCursor( ieutThreadData_t *pThreadData
                                   , ieiqQueue_t *Q );

static void ieiq_completeDeletion( ieutThreadData_t *pThreadData
                                 , ieiqQueue_t *Q );

static inline uint32_t ieiq_choosePageSize( void );

// Softlog replay - can go async
int32_t ieiq_SLEReplayPut( ietrReplayPhase_t Phase
                      , ieutThreadData_t *pThreadData
                      , ismEngine_Transaction_t *pTran
                      , void *pEntry
                      , ietrReplayRecord_t *pRecord
                      , ismEngine_AsyncData_t *pAsyncData
                      , ietrAsyncTransactionData_t *asyncTran
                      , ismEngine_DelivererContext_t *delivererContext);

int32_t ieiq_consumeAckCommitted(
                ieutThreadData_t               *pThreadData,
                int32_t                         retcode,
                ismEngine_AsyncData_t          *asyncinfo,
                ismEngine_AsyncDataEntry_t     *context);

int32_t ieiq_receiveAckCommitted(
                ieutThreadData_t               *pThreadData,
                int32_t                         retcode,
                ismEngine_AsyncData_t          *asyncinfo,
                ismEngine_AsyncDataEntry_t     *context);

static inline int32_t ieiq_consumeMessage( ieutThreadData_t *pThreadData
                                         , ieiqQueue_t *Q
                                         , ieiqQNode_t *pnode
                                         , bool *pPageCleanupNeeded
                                         , bool *pDeliveryIdsAvailable
                                         , ismEngine_AsyncData_t *asyncInfo);

static inline void ieiq_fullExpiryScan( ieutThreadData_t *pThreadData
                                      , ieiqQueue_t *Q
                                      , uint32_t nowExpire
                                      , bool *pPageCleanupNeeded
                                      , bool *pDeliveryIdsAvailable);

static inline void ieiq_destroyMessageBatch(  ieutThreadData_t *pThreadData
                                           , ieiqQueue_t *Q
                                           , uint32_t batchSize
                                           , ieiqQNode_t *discardNodes[]
                                           , bool removeExpiryData
                                           , bool *doPageCleanup
                                           , bool *deliveryIdsReleased);
                                  
static void ieiq_scanGetCursor( ieutThreadData_t *pThreadData
                              , ieiqQueue_t *Q);

static void ieiq_deliverMessage( ieutThreadData_t *pThreadData
                               , ieiqQueue_t *Q
                               , ieiqQNode_t *pDelivery
                               , ismMessageState_t msgState
                               , uint32_t deliveryId
                               , ismEngine_Message_t *hmsg
                               , ismMessageHeader_t *pMsgHdr
                               , bool *pCompleteWaiterActions
                               , bool *pDeliverMoreMsgs
                               , ismEngine_DelivererContext_t * delivererContext);

static inline void ieiq_rewindCursorToNode( ieutThreadData_t *pThreadData
                                          , ieiqQueue_t *Q
                                          , ieiqQNode_t *newCursor);

static inline void ieiq_checkForNonAckers( ieutThreadData_t  *pThreadData
                                         , ieiqQueue_t       *Q
                                         , uint64_t           headOrderId
                                         , const char        *clientId);

static inline int ieiq_initPutLock(ieiqQueue_t *Q);
static inline int ieiq_destroyPutLock(ieiqQueue_t *Q);
static inline void ieiq_getPutLock(ieiqQueue_t *Q);
static inline void ieiq_releasePutLock(ieiqQueue_t *Q);
static inline int ieiq_initHeadLock(ieiqQueue_t *Q);
static inline int ieiq_destroyHeadLock(ieiqQueue_t *Q);
static inline void ieiq_getHeadLock(ieiqQueue_t *Q);
static inline void ieiq_releaseHeadLock(ieiqQueue_t *Q);
static inline void ieiq_updateResourceSet( ieutThreadData_t *pThreadData
                                         , ieiqQueue_t *Q
                                         , iereResourceSetHandle_t resourceSet);

//////////////////////////////////////////////////////////////////////////////
// Allowing pages to vary in size would allow a balance between the amount
// of memory used for an empty queue and performance. However currently we
// do not have a sensible way of measuring a 'busy' queue so currently we
// create the first page as small, and all further pages as the default if
// there are a small number of subscriptions in the system or the high
// capacity size if there are lots of subs in the system
/////////////////////////////////////////////////////////////////////////////
static uint32_t ieiqPAGESIZE_SMALL        = 8;
static uint32_t ieiqPAGESIZE_DEFAULT      = 32;
static uint32_t ieiqPAGESIZE_HIGHCAPACITY = 8;

// Change this value to ismMESSAGE_RELIABILITY_AT_MOST_ONCE to make
// intermediate queue behave like simple queue.
static uint32_t ieiq_MaxReliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;


typedef struct tag_ieiqConsumeMessageAsyncData_t {
    char        StructId[4];
    ieiqQueue_t *Q;
    ieiqQNode_t *pnode;
    ismEngine_Session_t *pSession;
    bool deliveryIdsAvailable;
} ieiqConsumeMessageAsyncData_t;
#define IEIQ_CONSUMEMESSAGE_ASYNCDATA_STRUCID "IQCA"

#define IEIQ_MAXDELIVERYBATCH_SIZE 2048
typedef struct tag_ieiqAsyncMessageDeliveryInfo_t {
    char                   StructId[4];
    ieiqQueue_t           *Q;
    uint64_t               usedNodes;
    ieiqQNode_t           *pnodes[IEIQ_MAXDELIVERYBATCH_SIZE];
    //Adding things below pnodes should be done with care - when this structure
   //is copied, often the copying stops partway through pnodes (based on the usedNodes count)
} ieiqAsyncMessageDeliveryInfo_t;
#define IEIQ_ASYNCMESSAGEDELIVERYINFO_STRUCID "IQDI"

typedef struct tag_ieiqPutPostCommitInfo_t {
    char                   StructId[4];
    uint32_t               deleteCountDecrement;
    ieiqQueue_t           *Q;
} ieiqPutPostCommitInfo_t;
#define IEIQ_PUTPOSTCOMMITINFO_STRUCID "IQPC"

typedef struct tag_ieiqAsyncDestroyMessageBatchInfo_t {
    char StrucId[4];
    ieiqQueue_t *Q;
    uint32_t batchSize;
    bool removedStoreRefs;
} ieiqAsyncDestroyMessageBatchInfo_t;
#define IEIQ_ASYNCDESTROYMESSAGEBATCHINFO_STRUCID "IQDB"

/// @{
/// @name External Functions
///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Creates an intermediate Q
///  @remarks
///    This function is used to create an intermediate Q. The handle
///    returned can be used to put messages or create a create a consumer
///    on the queue.
///
///  @param[in]  pQName            - Name associated with this queue
///  @param[in]  QOptions          - Options
///  @param[in]  pPolicyInfo       - Initial policy information to use (optional)
///  @param[in]  hStoreObj         - Definition store handle
///  @param[in]  hStoreProps       - Properties store handle
///  @param[out] pQhdl             - Ptr to Q that has been created (if
///                                  returned ok)
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ieiq_createQ( ieutThreadData_t *pThreadData
                    , const char *pQName
                    , ieqOptions_t QOptions
                    , iepiPolicyInfo_t *pPolicyInfo
                    , ismStore_Handle_t hStoreObj
                    , ismStore_Handle_t hStoreProps
                    , iereResourceSetHandle_t resourceSet
                    , ismQHandle_t *pQhdl)
{
    int32_t rc=0, rc2;
    int os_rc=0;
    ieiqQueue_t *Q = NULL;
    ieiqQNodePage_t *firstPage = NULL;
    bool STATUS_CreatedRecordInStore = false;

    ieutTRACEL(pThreadData, QOptions, ENGINE_FNC_TRACE, FUNCTION_ENTRY
               "QOptions(%d) hStoreObj(0x%0lx)\n",
               __func__, QOptions, hStoreObj);

    *pQhdl = NULL;

    // Intermediate queues are not expected to be forwarding queues
    assert((QOptions & ieqOptions_REMOTE_SERVER_QUEUE) == 0);

    // If this queue is not being created to support a subscription and is not for a
    // temporary queue, and we are not in recovery, we need to add a queue definition
    // record & queue properties record to the store now.
    //
    // Note: If the queue DOES support a subscription, the subscription will either be
    //       durable in which case a store handle will have been provided, or non-durable.
    if (((QOptions & ieqOptions_SUBSCRIPTION_QUEUE) == 0) &&
        ((QOptions & ieqOptions_TEMPORARY_QUEUE) == 0) &&
        ((QOptions & ieqOptions_IN_RECOVERY) == 0) &&
        (ieiq_MaxReliability > ismMESSAGE_RELIABILITY_AT_MOST_ONCE))
    {
        // If the store is almost full then we do not want to make things
        // worse by creating more owner records in the store.
        ismEngineComponentStatus_t storeStatus = ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_0];

        if (storeStatus != StatusOk)
        {
            rc = ISMRC_ServerCapacity;

            ieutTRACEL(pThreadData, storeStatus, ENGINE_WORRYING_TRACE,
                       "Rejecting create IEIQ as store status[%d] is %d\n",
                       ismENGINE_STATUS_STORE_MEMORY_0, storeStatus);

            ism_common_setError(rc);
            goto mod_exit;
        }

        rc = iest_storeQueue(pThreadData,
                             intermediate,
                             pQName,
                             &hStoreObj,
                             &hStoreProps);

        if (rc != OK) goto mod_exit;

        STATUS_CreatedRecordInStore = true;
    }

    iere_primeThreadCache(pThreadData, resourceSet);
    size_t memSize = sizeof(ieiqQueue_t);
    Q = (ieiqQueue_t *)iere_calloc(pThreadData, resourceSet, IEMEM_PROBE(iemem_intermediateQ, 1), 1, memSize);

    if (Q == NULL)
    {
        rc = ISMRC_AllocateError;

        // The failure to allocate memory is not necessarily the end of the
        // the world, but we must at least ensure it is correctly handled
        ism_common_setError(rc);

        ieutTRACEL(pThreadData, memSize, ENGINE_ERROR_TRACE,
                   "%s: iere_calloc(%ld) failed! (rc=%d)\n", __func__,
                   memSize, rc);

        goto mod_exit;
    }

    ismEngine_SetStructId(Q->Common.StrucId, ismENGINE_QUEUE_STRUCID);
    // Record it's a intermediate Q
    Q->Common.QType = intermediate;
    Q->Common.pInterface = &QInterface[intermediate];
    Q->Common.resourceSet = resourceSet;

    if (pQName != NULL)
    {
        Q->Common.QName = (char *)iere_malloc(pThreadData, resourceSet, IEMEM_PROBE(iemem_intermediateQ, 2), strlen(pQName)+1);

        if (Q->Common.QName == NULL)
        {
            rc = ISMRC_AllocateError;

            ism_common_setError(rc);

            goto mod_exit;
        }

        strcpy(Q->Common.QName, pQName);
    }

    if ((QOptions & ieqOptions_IN_RECOVERY) != 0)
    {
        // During recovery we keep an array of pointers to each page in the
        // queue. This allows us to add the messages to the queue back in
        // order by locating the correct page for each message.
        memSize = ieqPAGEMAP_SIZE(ieqPAGEMAP_INCREMENT);
        Q->PageMap = (ieqPageMap_t *)iere_malloc(pThreadData,
                                                 resourceSet,
                                                 IEMEM_PROBE(iemem_intermediateQ, 3),
                                                 memSize);
        if (Q->PageMap == NULL)
        {
            rc = ISMRC_AllocateError;

            // The failure to allocate memory is not necessarily the end of the
            // the world, but we must at least ensure it is correctly handled
            ism_common_setError(rc);

            ieutTRACEL(pThreadData, memSize, ENGINE_ERROR_TRACE,
                       "%s: iere_malloc(%ld) failed! (rc=%d)\n", __func__,
                       memSize, rc);

            goto mod_exit;
        }
        ismEngine_SetStructId(Q->PageMap->StrucId, ieqPAGEMAP_STRUCID);
        Q->PageMap->ArraySize = ieqPAGEMAP_INCREMENT;
        Q->PageMap->PageCount = 0;
        Q->PageMap->TranRecoveryCursorIndex = 0;    // Not used for intermediateQ
        Q->PageMap->TranRecoveryCursorOrderId = 0;  // Not used for intermediateQ
        Q->PageMap->InflightMsgs = NULL;            // Not used for intermediateQ

        // Note entries in PageArray are uninitialised for pages less than
        // PageCount
    }

    os_rc = ieiq_initPutLock(Q);

    if (os_rc != 0)
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);

        ieutTRACEL(pThreadData, os_rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: init(putlock) failed! (osrc=%d)\n", __func__,
                   os_rc);

        goto mod_exit;
    }

    os_rc = ieiq_initHeadLock(Q);

    if (os_rc != 0)
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);

        ieutTRACEL(pThreadData, os_rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: init(headlock) failed! (osrc=%d)\n", __func__,
                   os_rc);

        goto mod_exit;
    }

    assert(pPolicyInfo != NULL);

    // Initialise the non-null fields
    Q->waiterStatus = IEWS_WAITERSTATUS_DISCONNECTED;
    Q->Common.PolicyInfo = pPolicyInfo;
    Q->QOptions = QOptions;
    Q->nextOrderId = 1;
    Q->hStoreObj = hStoreObj;
    Q->hStoreProps = hStoreProps;
    Q->maxInflightDeqs = ismEngine_serverGlobal.mqttMsgIdRange;
    Q->preDeleteCount = 1; // The initial +1 is decremented when delete is requested
    if (QOptions & ieqOptions_SUBSCRIPTION_QUEUE)
    {
        sprintf(Q->InternalName, "+SDR:0x%0lx", Q->hStoreObj);
    }
    else
    {
        sprintf(Q->InternalName, "+QDR:0x%0lx", Q->hStoreObj);
    }

    // Unless we are in recovery, add the first 2 node pages. In recovery
    // this is done in the completeRehydrate function.
    if ((QOptions & ieqOptions_IN_RECOVERY) == 0)
    {
        // Allocate a page of queue nodes
        firstPage = ieiq_createNewPage(pThreadData, Q, ieiqPAGESIZE_SMALL, 0);
        if (firstPage == NULL)
        {
            // The failure to allocate memory is not necessarily the end of the
            // the world, but we must at least ensure it is correctly handled
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);

            ieutTRACEL(pThreadData, ieiqPAGESIZE_SMALL, ENGINE_ERROR_TRACE, "%s: ieiq_createNewPage failed!\n",
                       __func__);

            goto mod_exit;
        }
        // Delay the creation of the second page until we actually need
        // it. This should help the memory footprint especially for queues/
        // subscriptions which only every have a few messages.
        firstPage->nextStatus = failed;

        Q->cursor     = &(firstPage->nodes[0]);
        Q->headPage = firstPage;
        Q->head     = &(firstPage->nodes[0]);
        Q->tail     = &(firstPage->nodes[0]);
    }
    else
    {
        Q->cursor     = NULL;
        Q->headPage = NULL;
        Q->head     = NULL;
        Q->tail     = NULL;
    }

    // We have successfully created the queue so commit the store
    // transaction if we updated the store
    if (STATUS_CreatedRecordInStore)
    {
        iest_store_commit( pThreadData, false);
    }

    // And one final thing we have to do is create the QueueRefContext
    // we will use to put messages on the queue. If this fails we have
    // to backout our hard work manually.
    if (Q->hStoreObj != ismSTORE_NULL_HANDLE && (QOptions & ieqOptions_IN_RECOVERY) == 0)
    {
        ismStore_ReferenceStatistics_t RefStats = { 0 };

        rc = ism_store_openReferenceContext( Q->hStoreObj
                                           , &RefStats
                                           , &Q->QueueRefContext );
        if (rc != OK)
        {
            // SDR & SPR will be cleaned up by our caller, otherwise
            // we will need to clear up the QDR & QPR.
            if (STATUS_CreatedRecordInStore)
            {
                rc2 = ism_store_deleteRecord(pThreadData->hStream, Q->hStoreObj);
                if (rc2 != OK)
                {
                    ieutTRACEL(pThreadData, rc2, ENGINE_ERROR_TRACE,
                               "%s: ism_store_deleteRecord failed! (rc=%d)\n",
                               __func__, rc2);
                }
                rc2 = ism_store_deleteRecord(pThreadData->hStream, Q->hStoreProps);
                if (rc2 != OK)
                {
                    ieutTRACEL(pThreadData, rc2, ENGINE_ERROR_TRACE,
                               "%s: ism_store_deleteRecord (+QPR:0x%0lx) failed! (rc=%d)\n",
                               __func__, Q->hStoreProps, rc2);
                }

                iest_store_commit( pThreadData, false );
                STATUS_CreatedRecordInStore = false;
            }

            goto mod_exit;
        }
    }

    *pQhdl = (ismQHandle_t)Q;

mod_exit:
    if (rc != OK)
    {
        // If we have failed to define the queue then cleanup, anything
        // we succesfully created.
        if (Q != NULL)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            if (Q->Common.QName != NULL) iere_free(pThreadData, resourceSet, iemem_intermediateQ, Q->Common.QName);
            if (Q->PageMap != NULL) iere_freeStruct(pThreadData, resourceSet, iemem_intermediateQ, Q->PageMap, Q->PageMap->StrucId);

            (void)ieiq_destroyPutLock(Q);
            (void)ieiq_destroyHeadLock(Q);

            if (firstPage != NULL) iere_freeStruct(pThreadData, resourceSet, iemem_intermediateQPage, firstPage, firstPage->StrucId);

            iere_freeStruct(pThreadData, resourceSet, iemem_intermediateQ, Q, Q->Common.StrucId);
        }

        if (STATUS_CreatedRecordInStore)
        {
            iest_store_rollback(pThreadData, false);
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, Q=%p\n", __func__, rc, Q);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Return whether the queue is marked as deleted or not
///  @param[in] Qhdl               - Queue to check
///////////////////////////////////////////////////////////////////////////////
bool ieiq_isDeleted(ismQHandle_t Qhdl)
{
    return ((ieiqQueue_t *)Qhdl)->isDeleted;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Update the in-memory queue and optionally store record as deleted
///
///  @param[in] Qhdl               - Queue to be marked as deleted
///  @param[in] updateStore        - Whether to update the state of the store
///                                  record (effectively forcing deletion at
///                                  restart if not completed now)
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ieiq_markQDeleted(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl, bool updateStore)
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    int32_t rc = OK;

    uint64_t newState;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p updateStore=%d\n", __func__, Q, (int)updateStore);

    // Mark the in-memory queue as deleted
    Q->isDeleted = true;

    // If requested, mark the store definition record as deleted too.
    if (updateStore)
    {
        // If we have not store handle for the definition, there is
        // nothing to do here.
        if (Q->hStoreObj == ismSTORE_NULL_HANDLE)
        {
            //This must be a non-durable subscription or a temporary queue
            assert((Q->QOptions & (ieqOptions_SUBSCRIPTION_QUEUE|ieqOptions_TEMPORARY_QUEUE)) != 0);
            goto mod_exit;
        }

        // Set the state of the definition record to deleted.
        if (Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE)
        {
            newState = iestSDR_STATE_DELETED;
        }
        else
        {
            newState = iestQDR_STATE_DELETED;
        }

        // Mark the store definition record as deleted
        rc = ism_store_updateRecord(pThreadData->hStream,
                                    Q->hStoreObj,
                                    ismSTORE_NULL_HANDLE,
                                    newState,
                                    ismSTORE_SET_STATE);

        if (rc == OK)
        {
            iest_store_commit(pThreadData, false);
        }
        else
        {
            assert(rc != ISMRC_StoreGenerationFull);

            iest_store_rollback(pThreadData, false);

            ism_common_setError(rc);
        }
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Scan entire queue looking for messages with expiry set (and removing
///    expired messages.
///
///  @remarks
///    *MUST* already have expiry lock (i.e. after ieme_startReaperQExpiryScan)
///
///  @param[in] q                  - Queue
///  @param[in] nowExpire          - time that means means have expired
///
///  @return                       - void
///////////////////////////////////////////////////////////////////////////////
static inline void ieiq_fullExpiryScan( ieutThreadData_t *pThreadData
                                      , ieiqQueue_t *Q
                                      , uint32_t nowExpire
                                      , bool *pPageCleanupNeeded
                                      , bool *pDeliveryIdsAvailable)
{
    ieutTRACEL(pThreadData,Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p \n", __func__,Q);

    //Reset expiry data
    ieme_clearQExpiryDataPreLocked( pThreadData
                                  , (ismEngine_Queue_t *)Q);

    ieiqQNode_t *currNode = Q->head;

    uint32_t MaxBatchSize = 50;
    ieiqQNode_t *expiredNodes[MaxBatchSize];
    uint32_t numExpiredNodes = 0;

    while (1)
    {
        ismEngine_Message_t *msg = currNode->msg;

        if ((msg != NULL) && (currNode->msgState == ismMESSAGE_STATE_AVAILABLE))
        {
            uint32_t msgExpiry = msg->Header.Expiry;

            if (msgExpiry != 0)
            {
                if (msgExpiry < nowExpire)
                {
                    expiredNodes[numExpiredNodes] = currNode;
                    numExpiredNodes++;

                    if (numExpiredNodes >= MaxBatchSize)
                    {
                        ieiq_destroyMessageBatch( pThreadData
                                                , Q
                                                , numExpiredNodes
                                                , expiredNodes
                                                , false
                                                , pPageCleanupNeeded
                                                , pDeliveryIdsAvailable);

                        __sync_fetch_and_add(&(Q->expiredMsgs), numExpiredNodes);
                        pThreadData->stats.expiredMsgCount += numExpiredNodes;

                        numExpiredNodes = 0;
                    }
                }
                else
                {
                    iemeBufferedMsgExpiryDetails_t msgExpiryData = { currNode->orderId , currNode, msgExpiry };

                    ieme_addMessageExpiryPreLocked( pThreadData
                                                  , (ismEngine_Queue_t *)Q
                                                  , &msgExpiryData
                                                  , true);
                }
            }
        }

        //Do a dirty read of the end of the queue (if we read a stale value, no problem, a different put will
        //cause another clean up. We want the node after the one we just scanned to be less than the next orderId
        //that will be used in the queue:
        if(currNode->orderId > (Q->nextOrderId - 2))
        {
            //Run out of messages to look at...
            break;
        }

        //Have we reached the end of the page
        if((currNode+1)->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
        {
            ieiqQNodePage_t *currpage = (ieiqQNodePage_t *)((currNode+1)->msg);

            ismEngine_CheckStructId(currpage->StrucId, IEIQ_PAGE_STRUCID, ieutPROBE_001);

            if (currpage->nextStatus < completed)
            {
                //No next page yet, we're done
                break;
            }
            ieiqQNodePage_t *nextpage = currpage->next;
            ismEngine_CheckStructId(nextpage->StrucId, IEIQ_PAGE_STRUCID, ieutPROBE_002);
            currNode = &(nextpage->nodes[0]);
        }
        else
        {
            currNode += 1;
        }
    }

    if (numExpiredNodes > 0)
    {
        ieiq_destroyMessageBatch( pThreadData
                                , Q
                                , numExpiredNodes
                                , expiredNodes
                                , false
                                , pPageCleanupNeeded
                                , pDeliveryIdsAvailable
                                );

        __sync_fetch_and_add(&(Q->expiredMsgs), numExpiredNodes);
        pThreadData->stats.expiredMsgCount += numExpiredNodes;
    }

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_EXIT "Q=%p\n", __func__, Q);
}

void ieiq_reducePreDeleteCount(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl)
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    uint64_t oldCount = __sync_fetch_and_sub(&(Q->preDeleteCount), 1);

    assert(oldCount > 0);

    if (oldCount == 1)
    {
        //We've just removed the last thing blocking deleting the queue, do it now....
        ieiq_completeDeletion(pThreadData, Q);
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Deletes a intermediate Q
///  @remarks
///    This function is used to release the memory and optionally
///    delete the STORE data for an intermediate Q.
///    At the time this function is called there should be no other references
///    to the queue.
///
///  @param[in] pQhdl              - Ptr to Q to be deleted
///  @param[in] freeOnly           - Don't change persistent state, free memory
///                                  only
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ieiq_deleteQ( ieutThreadData_t *pThreadData
                    , ismQHandle_t *pQhdl
                    , bool freeOnly )
{
    int32_t rc = OK;
    ieiqQueue_t *Q = (ieiqQueue_t *)*pQhdl;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p\n", __func__, Q);

    if (Q->isDeleted)
    {
        //Gah! They are trying to delete us twice
        rc = ISMRC_QueueDeleted;
        goto mod_exit;
    }

    //No new msgs should ever be delivered from the queue
    Q->redeliverOnly = true;

    Q->deletionRemovesStoreObjects = !freeOnly;

    // Do we still have an active consumer?
    if (    (Q->waiterStatus != IEWS_WAITERSTATUS_DISCONNECTED)
         && (Q->waiterStatus != IEWS_WAITERSTATUS_DISABLED))
    {
        ieutTRACEL(pThreadData, Q->pConsumer, ENGINE_WORRYING_TRACE, "%s: Q(%p) still has an active Consumer(%p)\n",
                   __func__, Q, Q->pConsumer);
    }

    // The queue should already have been marked as deleted in the store if it's a subscription.
    bool markQDeletedInStore = ((Q->QOptions & ieqOptions_PUBSUB_QUEUE_MASK) == 0) ? Q->deletionRemovesStoreObjects : false;

    rc = ieiq_markQDeleted(pThreadData, *pQhdl, markQDeletedInStore);
    if (rc != OK) goto mod_exit;

    // Complete the deletion if there is nothing blocking it
    ieiq_reducePreDeleteCount(pThreadData, (ismQHandle_t)Q);

    *pQhdl = NULL;

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

static void ieiq_appendPage( ieutThreadData_t *pThreadData
                           , ieiqQueue_t *Q
                           , ieiqQNodePage_t *currPage
                           , ieiqPageSeqNo_t lastPageSeqNo)
{
    uint32_t nodesInPage = ieiq_choosePageSize();
    ieiqQNodePage_t *newPage = ieiq_createNewPage( pThreadData
                                                 , Q
                                                 , nodesInPage
                                                 , lastPageSeqNo);

    if (newPage == NULL)
    {
        // Argh - we've run out of memory - record we failed to add a
        // page so no-one sits waiting for it to appear. This is not
        // a problem for us however we will confinue on our merry way.
        DEBUG_ONLY bool expectedState;
        expectedState=__sync_bool_compare_and_swap(&(currPage->nextStatus),
                                                   unfinished,
                                                   failed);
        assert(expectedState);

        ieutTRACEL(pThreadData, currPage, ENGINE_ERROR_TRACE,
                   "ieiq_createNewPage failed Q=%p nextPage=%p\n", Q, currPage);
    }
    else
    {
        currPage->next = newPage;
    }
}

static inline int32_t ieiq_preparePutStoreData( ieutThreadData_t *restrict pThreadData
                                              , ieiqQueue_t *restrict Q
                                              , ieqPutOptions_t putOptions
                                              , ismEngine_Message_t  *restrict qmsg
                                              , ismMessageState_t msgState
                                              , uint8_t msgFlags
                                              , bool existingStoreTran
                                              , ismStore_Reference_t * restrict pMsgRef
                                              , bool *restrict pMsgInStore)
{
    ismStore_Handle_t hMsg;
    int32_t rc = OK;

    if ((qmsg->Header.Persistence) &&
        (Q->hStoreObj != ismSTORE_NULL_HANDLE) &&
        (qmsg->Header.Reliability > ismMESSAGE_RELIABILITY_AT_MOST_ONCE))
    {
        iestStoreMessageOptions_t storeMessageOptions = existingStoreTran ? iestStoreMessageOptions_EXISTING_TRANSACTION :
                                                                            iestStoreMessageOptions_NONE;

        // If this thread is the only one that can access this message we can
        // enable an optimisation in iest_storeMessage to increment the store
        // reference count without a memory barrier.
        if ((putOptions & ieqPutOptions_THREAD_LOCAL_MESSAGE) == 0)
        {
            storeMessageOptions |= iestStoreMessageOptions_ATOMIC_REFCOUNTING;
        }

        // QoS 1 & 2 messages must be written to the store before they
        // can be linked to the queue in the store. So if the message
        // is not already in the store add it now (before we take the
        // put lock)
        *pMsgInStore = true;

        rc = iest_storeMessage( pThreadData
                              , qmsg
                              , 1
                              , storeMessageOptions
                              , &hMsg );
        if (UNLIKELY(rc != OK))
        {
            // iest_storeMessage will have set the error code if it was
            // unsuccessful so simply propagate any error
            ieutTRACEL(pThreadData, qmsg, ENGINE_ERROR_TRACE, "%s: iest_storeMessage failed! (rc=%d)\n",
                       __func__, rc);

            goto mod_exit;
        }

        assert(hMsg != ismSTORE_NULL_HANDLE);

        // Also at this time prepare the Reference which will associate
        // the message with the queue
        pMsgRef->OrderId = 0;  // Will be set in the put lock
        pMsgRef->hRefHandle = hMsg;
        ieqRefValue_t RefValue;
        RefValue.Value = 0;
        RefValue.Parts.MsgFlags = msgFlags;
        pMsgRef->Value = RefValue.Value;
        pMsgRef->State = qmsg->Header.RedeliveryCount & 0x3f;
        pMsgRef->State |= (msgState << 6);
        pMsgRef->Pad[0] = 0;
        pMsgRef->Pad[1] = 0;
        pMsgRef->Pad[2] = 0;
    }
mod_exit:
    return rc;
}

static inline int32_t ieiq_assignQSlot( ieutThreadData_t *restrict pThreadData
                                      , ieiqQueue_t *restrict Q
                                      , bool inTran
                                      , uint64_t assignedOrderId
                                      , uint8_t msgFlags
                                      , ismStore_Reference_t * restrict pMsgRef
                                      , ieiqQNode_t **restrict ppNode)
{
    int32_t rc = OK;
    ieiqQNode_t *pNode;
    ieiqQNodePage_t *nextPage = NULL;
    uint64_t putDepth;
    ieiqPageSeqNo_t *lastPageSeqNo=NULL;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    //Get the putting lock
    ieiq_getPutLock(Q);

    pNode = Q->tail;

    // Have we run out of space on this page?
    if ((Q->tail+1)->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
    {
        //Page full...move to the next page if it has been added...
        //...if it hasn't been added someone will add it shortly
        ieiqQNodePage_t *currpage=ieiq_getPageFromEnd(Q->tail+1);
        nextPage=ieiq_moveToNewPage(pThreadData, Q, Q->tail+1);

        // A non-null nextPage will be used to identify that a new
        // page must be created once we've released the put lock
        if (UNLIKELY(nextPage == NULL))
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);

            ieutTRACEL(pThreadData, currpage, ENGINE_SEVERE_ERROR_TRACE,
                       "%s: Failed to allocate next NodePage.\n", __func__);

            // Release the putting lock
            ieiq_releasePutLock(Q);

            goto mod_exit;
        }
        //Make the tail point to the first item in the new page
        Q->tail = &(nextPage->nodes[0]);

        //Record on the old page that a getter can free it
        currpage->nextStatus = completed;

        //record that we've used another page so we'll need to create
        //a new one once we've released the put lock, record where we
        //want a pointer to the newPage stored
        lastPageSeqNo = &(nextPage->sequenceNo);
    }
    else
    {
        // Move the tail to the next item in the page
        Q->tail++;
    }

    // Check that the node is empty. This could be just an assert
    if ((pNode->msg != MESSAGE_STATUS_EMPTY) ||
        (pNode->msgState != ismMESSAGE_STATE_AVAILABLE))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                      , "Non empty node selected during put.", ISMRC_Error
                      , "Node", pNode, sizeof(ieiqQNode_t)
                      , "Queue", Q, sizeof(ieiqQueue_t)
                      , NULL );
    }

    // Override the message order-id? This is only every used in testing
    uint64_t orderId;

    if (UNLIKELY(assignedOrderId != 0))
    {
        orderId = assignedOrderId;

        if (Q->nextOrderId <= orderId)
        {
            Q->nextOrderId = orderId +1;
        }
    }
    else
    {
        orderId = Q->nextOrderId++;
    }
    pNode->orderId = orderId;
    pNode->msgFlags = msgFlags;
    pMsgRef->OrderId = orderId;

    // Increase the count of messages on the queue, this must be done
    // before we actually make the message visible
    iere_primeThreadCache(pThreadData, resourceSet);
    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, 1);
    pThreadData->stats.bufferedMsgCount++;
    putDepth = __sync_add_and_fetch(&(Q->bufferedMsgs), 1);

    // Increase the count of enqueues
    if (inTran)
    {
        __sync_add_and_fetch(&(Q->inflightEnqs), 1);
    }
    else
    {
        __sync_add_and_fetch(&(Q->enqueueCount), 1);
    }

    // Update the HighWaterMark on the number of messages on the queue.
    // this is recorded on a best-can-do basis
    if (putDepth > Q->bufferedMsgsHWM)
    {
        Q->bufferedMsgsHWM = putDepth;
    }

    // Release the putting lock
    ieiq_releasePutLock(Q);

    ieutTRACE_HISTORYBUF(pThreadData, pNode->orderId);


mod_exit:

    // If we need to create a new page do that now...
    if (nextPage != NULL)
    {
        ieiq_appendPage( pThreadData, Q, nextPage, *lastPageSeqNo);
        nextPage = NULL;
    }

    *ppNode = pNode;

    return rc;
}

static inline int32_t ieiq_finishPut( ieutThreadData_t *restrict pThreadData
                                    , ieiqQueue_t *restrict Q
                                    , ismEngine_Transaction_t *pTran
                                    , ismEngine_Message_t  *restrict qmsg
                                    , ieiqQNode_t *restrict pNode
                                    , ismStore_Reference_t * restrict pMsgRef
                                    , bool existingStoreTran
                                    , bool msgInStore)
{
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    // Store the message
    if ((qmsg->Header.Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE) &&
        (pTran == NULL))
    {
        //If we are making a message with expiry available, update the stats
        //So the expiry reaper know there is work to do
        if (qmsg->Header.Expiry != 0)
        {
            iemeBufferedMsgExpiryDetails_t msgExpiryData = { pNode->orderId, pNode, qmsg->Header.Expiry };
            ieme_addMessageExpiryData( pThreadData, (ismEngine_Queue_t *)Q,  &msgExpiryData);
        }

        pNode->msg = qmsg;
    }
    else
    {
        // The creating of the queue-to-msg store reference and the
        // transaction reference must be done in the same store transaction.
        if (msgInStore)
        {
            if (!existingStoreTran)
            {
                ismStore_Reservation_t Reservation;

                Reservation.DataLength = 0;
                Reservation.RecordsCount = 0;
                Reservation.RefsCount = (pTran)?2:1;

                // If the message is to be added to the store, then reserve enough
                // space in the store for the records we are going to write.
                rc = ism_store_reserveStreamResources( pThreadData->hStream
                                                     , &Reservation );
                if (rc != OK)
                {
                    // Failure to reserve space in the store is not expected. The
                    // store will already have called ism_common_setError, so
                    // nothing more for us to do except propagate the error.
                    ieutTRACE_FFDC( ieutPROBE_002, true
                                  , "ism_store_reserveStreamResources failed.", rc
                                  , "RecordsCount", &Reservation.RecordsCount, sizeof(uint32_t)
                                  , "RefsCount", &Reservation.RefsCount, sizeof(uint32_t)
                                  , "DataLength", &Reservation.DataLength, sizeof(uint64_t)
                                  , "Reservation", &Reservation,  sizeof(Reservation)
                                  , NULL );

                }
            }

            pNode->inStore = true;

            // First put the message in the store. This is done in a store
            // transaction, the store space for which has already been reserved.
            rc = ism_store_createReference( pThreadData->hStream
                                          , Q->QueueRefContext
                                          , pMsgRef
                                          , 0 // minimumActiveOrderId
                                          , &pNode->hMsgRef );
            if (rc != OK)
            {
                // There are no 'recoverable' failures from the
                // ism_store_createReference call. ism_store_createReference
                // will already have taken all of the require diagnostics
                // so simply return the error up the stack.
                ieutTRACE_FFDC( ieutPROBE_003, true
                              , "ism_store_createReference failed.", rc
                              , "OrderId", &(pMsgRef->OrderId), sizeof(uint64_t)
                              , "msgRef", pMsgRef, sizeof(*pMsgRef)
                              , NULL );
            }

            ieutTRACEL(pThreadData, pNode->hMsgRef, ENGINE_HIGH_TRACE,
                       "Created msgref 0x%0lx in store for message 0x%0lx on queue %p orderid=%ld\n",
                       pNode->hMsgRef, pMsgRef->hRefHandle, Q, pNode->orderId);
        }

        if (pTran)
        {
            ieiqSLEPut_t SLE;

            if (pNode->inStore)
            {
                // And then add this reference to the engine transaction object
                // in the store. Again the store space for this has already
                // been reserved.
                assert(pNode->hMsgRef != 0);

                rc = ietr_createTranRef( pThreadData
                                       , pTran
                                       , pNode->hMsgRef
                                       , iestTOR_VALUE_PUT_MESSAGE
                                       , 0
                                       , &SLE.TranRef);
                if (rc != OK)
                {
                    ieutTRACEL(pThreadData, pTran, ENGINE_SEVERE_ERROR_TRACE,
                               "%s: ietr_createTranRef failed! (rc=%d)\n",
                               __func__, rc);

                    if (!existingStoreTran)
                    {
                        iest_store_rollback(pThreadData, false);
                    }
                    else
                    {
                        ietr_markRollbackOnly( pThreadData, pTran );
                    }

                    // Decrement the bufferedMsgs we incremented
                    iere_primeThreadCache(pThreadData, resourceSet);
                    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -1);
                    pThreadData->stats.bufferedMsgCount--;
                    (void)__sync_sub_and_fetch(&(Q->bufferedMsgs), 1);
                    // No longer in-flight
                    (void)__sync_sub_and_fetch(&(Q->inflightEnqs), 1);

                    // Write-off this node
                    pNode->msgState = ismMESSAGE_STATE_CONSUMED;
                    goto mod_exit;
                }
            }

            // And finally create the softlog entry which will eventually
            // put the message on the queue when commit is called.
            SLE.pQueue = Q;
            SLE.pNode = pNode;
            SLE.bInStore = pNode->inStore;
            SLE.pMsg = qmsg;
            SLE.bSavepointRolledback = false;

            // Add the message to the Soft log to be added to the queue when
            // commit is called
            rc = ietr_softLogAdd( pThreadData
                                , pTran
                                , ietrSLE_IQ_PUT
                                , NULL
                                , ieiq_SLEReplayPut
                                , MemoryCommit | PostCommit | Rollback | PostRollback | SavepointRollback
                                , &SLE
                                , sizeof(SLE)
                                , 0, 1
                                , NULL);

            if (rc != OK)
            {
                ieutTRACEL(pThreadData, pTran, ENGINE_SEVERE_ERROR_TRACE,
                           "%s: ietr_softLogAdd failed! (rc=%d)\n", __func__, rc);

                // Only possible return code is ISMRC_AllocateError. Back-out
                // the changes we made to the store and pass the error back.
                if (!existingStoreTran)
                {
                    iest_store_rollback(pThreadData, false);
                }
                else
                {
                    ietr_markRollbackOnly( pThreadData, pTran );
                }

                // Decrement the bufferedMsgs we incremented
                iere_primeThreadCache(pThreadData, resourceSet);
                iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -1);
                pThreadData->stats.bufferedMsgCount--;
                (void)__sync_sub_and_fetch(&(Q->bufferedMsgs), 1);
                // No longer in-flight
                (void)__sync_sub_and_fetch(&(Q->inflightEnqs), 1);

                // Write-off this node
                pNode->msgState = ismMESSAGE_STATE_CONSUMED;

                goto mod_exit;
            }
        }

        if ((pNode->inStore) && !existingStoreTran)
        {
            // And commit the store transaction
            iest_store_commit( pThreadData, false );
        }

        if (!pTran)
        {
            //If we are making a message with expiry available, update the stats
            //So the expiry reaper know there is work to do
            if (qmsg->Header.Expiry != 0)
            {
                iemeBufferedMsgExpiryDetails_t msgExpiryData = { pNode->orderId, pNode, qmsg->Header.Expiry };
                ieme_addMessageExpiryData( pThreadData, (ismEngine_Queue_t *)Q,  &msgExpiryData);
            }
            // No transaction so place on the queue now.
            pNode->msg = qmsg;
        }
    }

mod_exit:
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Puts a message on an Intermediate Q
///  @remarks
///    This function is called to put a message on an intermediate queue.
///
///  @param[in] Qhdl               - Q to put to
///  @param[in] pTran              - Pointer to transaction
///  @param[in] inmsg              - message to put
///  @param[in] inputMsgTreatment  - IEQ_MSGTYPE_REFCOUNT (re-use & increment refcount)
///                                  or IEQ_MSGTYPE_INHERIT (re-use)
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ieiq_putMessage( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ieqPutOptions_t putOptions
                       , ismEngine_Transaction_t *pTran
                       , ismEngine_Message_t *inmsg
                       , ieqMsgInputType_t inputMsgTreatment
                       , ismEngine_DelivererContext_t * delivererContext)
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    ismEngine_Message_t  *qmsg = NULL;
    int32_t rc = OK;
    ieiqQNode_t *pNode;
    ismStore_Reference_t msgRef;
    uint8_t msgFlags;
    bool msgInStore = false;

    ieutTRACEL(pThreadData, inmsg, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p pTran=%p msg=%p Length=%ld Reliability=%d\n",
               __func__, Q, pTran, inmsg, inmsg->MsgLength, inmsg->Header.Reliability);

#if 0
    for (int i=0; i < inmsg->AreaCount; i++)
    {
        ieutTRACEL(pThreadData, inmsg->AreaLengths[i], ENGINE_HIGH_TRACE,
                   "MSGDETAILS(put): Area(%d) Type(%d) Length(%ld) Data(%.*s)\n",
                   i,
                   inmsg->AreaTypes[i],
                   inmsg->AreaLengths[i],
                   (inmsg->AreaLengths[i] > 100)?100:inmsg->AreaLengths[i],
                   (inmsg->AreaTypes[i] == ismMESSAGE_AREA_PAYLOAD)?inmsg->pAreaData[i]:"");
    }
#endif

    iepiPolicyInfo_t *pPolicyInfo = Q->Common.PolicyInfo;

    assert(pPolicyInfo->maxMessageBytes == 0); // No code to deal with maxMessageBytes

    if ((pPolicyInfo->maxMessageCount > 0) && (Q->bufferedMsgs >= pPolicyInfo->maxMessageCount))
    {
        iereResourceSet_t *resourceSet = Q->Common.resourceSet;

        if (!Q->ReportedQueueFull)
        {
            ism_common_log_context logContext = {0};
            if (resourceSet != iereNO_RESOURCE_SET)
            {
                logContext.resourceSetId = resourceSet->stats.resourceSetIdentifier;
            }

            // No need to separately trace, the log message will be shown in the trace

            Q->ReportedQueueFull = true;

            if ((Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE) == 0)
            {
                LOGCTX( &logContext, ERROR, Messaging, 3006, "%-s%lu",
                        "The limit of {1} messages buffered on queue {0} has been reached.",
                        Q->Common.QName ? Q->Common.QName : "", pPolicyInfo->maxMessageCount  );
            }
            else
            {
                LOGCTX( &logContext, ERROR, Messaging, 3007, "%-s%lu",
                        "The limit of {1} messages buffered for subscription {0} has been reached.",
                        Q->Common.QName ? Q->Common.QName : "", pPolicyInfo->maxMessageCount  );
            }
        }
        else
        {
            ieutTRACEL(pThreadData, Q->bufferedMsgs, ENGINE_HIGH_TRACE,
                       "%s: Queue full. bufferedMsgs=%lu maxMessageCount=%lu\n",
                       __func__, Q->bufferedMsgs, pPolicyInfo->maxMessageCount);
        }

        if (pPolicyInfo->maxMsgBehavior == RejectNewMessages)
        {
            // We have been told to ignore RejectNewMessages - at least trace that.
            if ((putOptions & ieqPutOptions_IGNORE_REJECTNEWMSGS) != 0)
            {
                ieutTRACEL(pThreadData, putOptions, ENGINE_HIGH_TRACE,
                           "Ignoring RejectNewMessages on full queue. putOptions=0x%08x\n",
                           putOptions);
            }
            else
            {
                rc = ISMRC_DestinationFull;
                ism_common_setError(rc);

                __sync_add_and_fetch(&(Q->rejectedMsgs), 1);
                iere_primeThreadCache(pThreadData, resourceSet);
                iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_REJECTEDMSGS, 1);

                goto mod_exit;
            }
        }
        else if (Q->bufferedMsgs >= (100 + (3 * pPolicyInfo->maxMessageCount)))
        {
            ieutTRACEL(pThreadData, Q->bufferedMsgs, ENGINE_WORRYING_TRACE,
                       "%s: Queue/Sub %s Significantly OVERfull. bufferedMsgs=%lu maxMessageCount=%lu\n",
                       __func__, Q->Common.QName ? Q->Common.QName : "",
                       Q->bufferedMsgs, pPolicyInfo->maxMessageCount);
            usleep(50);
        }
    }

    // The sending (putting) of messages to this queue has been disallowed
    if (pPolicyInfo->allowSend == false)
    {
        rc = ISMRC_SendNotAllowed;
        goto mod_exit;
    }

    msgFlags = ismMESSAGE_FLAGS_NONE;
    if (putOptions & ieqPutOptions_RETAINED)
    {
        msgFlags |= ismMESSAGE_FLAGS_RETAINED;
    }

    qmsg = inmsg;

    if (inputMsgTreatment == IEQ_MSGTYPE_REFCOUNT)
    {
        iem_recordMessageUsage(qmsg);
    }
    else
    {
        assert(inputMsgTreatment == IEQ_MSGTYPE_INHERIT);
    }

#if 1
    // If there's nothing on the queue that needs to be given first see if we
    // can pass the message directly to the consumer.
    // This functionality is crucial to QoS-0 performance however for
    // comparison between the QoS1 and QoS2 it is useful to be able to
    // switch it off.
    if ((pTran == NULL) &&
        (qmsg->Header.Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE))
    {
        if (Q->bufferedMsgs == 0)
        {
            rc = ieiq_putToWaitingGetter(pThreadData, Q, qmsg, msgFlags, delivererContext);
            if (rc == OK)
            {
                // We've given the message to someone..we're done
                goto mod_exit;
            }
            else /* (rc == ISMRC_NoAvailWaiter) */
            {
                assert(rc == ISMRC_NoAvailWaiter);
                // No-one wants it, enqueue it
                rc = OK;
            }
        }
    }
#endif

    bool existingStoreTran = ((pTran != NULL) &&
                              (pTran->fAsStoreTran || pTran->fStoreTranPublish));

    rc = ieiq_preparePutStoreData( pThreadData
                                 , Q
                                 , putOptions
                                 , qmsg
                                 , ismMESSAGE_STATE_AVAILABLE
                                 , msgFlags
                                 , existingStoreTran
                                 , &msgRef
                                 , &msgInStore);

    if (UNLIKELY(rc != OK))
    {
        //Will have already traced issue
        goto mod_exit;
    }

    rc = ieiq_assignQSlot( pThreadData
                         , Q
                         , (pTran != NULL)
                         , ((putOptions & ieqPutOptions_SET_ORDERID) ?
                                 qmsg->Header.OrderId : 0)
                         , msgFlags
                         , &msgRef
                         , &pNode );

    if (UNLIKELY(rc != OK))
    {
        // We're going to fail the put through lack of memory
        //TODO: Shouldn't we do the release if the other 2 parts of put fail? Move to modexit if rc!=OK?
        if (inputMsgTreatment != IEQ_MSGTYPE_INHERIT)
        {
            iem_releaseMessage(pThreadData, qmsg);
        }
        goto mod_exit;
    }

    rc = ieiq_finishPut( pThreadData
                       , Q
                       , pTran
                       , qmsg
                       , pNode
                       , &msgRef
                       , existingStoreTran
                       , msgInStore);


mod_exit:
    // If we just put a message (and it was outside of syncpoint) then
    // attempt to deliver any messages to the consumer
    if (pTran == NULL)
    {
        if (Q->bufferedMsgs >= pPolicyInfo->maxMessageCount)
        {
            //Check if we can remove expired messages and get below the max...
            ieme_reapQExpiredMessages(pThreadData, (ismEngine_Queue_t *)Q);

            if (    (pPolicyInfo->maxMsgBehavior == DiscardOldMessages)
                 && (Q->bufferedMsgs > pPolicyInfo->maxMessageCount))
            {
                //If we couldn't deliver the messages, throw away enough to be below max messages
                //We throw away BEFORE checkwaiters so that any messages we send are newer than ones
                //we discard
                ieiq_reclaimSpace(pThreadData, Qhdl, true);
            }
        }

        // We don't care about any error encountered while attempting
        // to deliver a message to a waiter.
        (void)ieiq_checkWaiters(pThreadData, (ismEngine_Queue_t *)Q, NULL, delivererContext);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Recreates a message on an Intermediate Q from the store record
///  @remarks
///    This function is called to recreate a message from the store
///    definition and put it on the queue.
///
///  @param[in] Qhdl               - Q to put to
///  @param[in] pMsg               - message to put
///  @param[in] pTranData          - Transaction data
///  @param[in] hMsgRef            - message handle
///  @param[in] pReference         - Store reference data
///////////////////////////////////////////////////////////////////////////////
int32_t ieiq_rehydrateMsg( ieutThreadData_t *pThreadData
                         , ismQHandle_t Qhdl
                         , ismEngine_Message_t *pMsg
                         , ismEngine_RestartTransactionData_t *pTranData
                         , ismStore_Handle_t hMsgRef
                         , ismStore_Reference_t *pReference )
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    uint64_t nodesInPage = ieiqPAGESIZE_HIGHCAPACITY; //64 bit as used in 64bit bitwise arith
    int32_t rc = 0;
    ieiqQNode_t *pNode;                    // message node
    ieiqQNodePage_t *pPage=NULL;           // Pointer to NodePage
    ieqPageMap_t *PageMap;                 // Local pointer to page map
    ieqPageMap_t *NewPageMap;              // Re-allocated page map
    int64_t pageNum;                       // Must be signed
    uint32_t nodeCounter;                  // Loop counter
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, pMsg, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p msg=%p orderid=%lu state=%u [%s]\n",
               __func__, Q, pMsg, pReference->OrderId, (uint32_t)pReference->State,
               (pTranData)?"Transactional":"");

    assert(hMsgRef != ismSTORE_NULL_HANDLE);

    // Recovery is single-threaded so need to obtain the put lock
    PageMap=Q->PageMap;

    // See if the message is on a page we have already created, we
    // look backward through the list of pages
    if (PageMap->PageCount == 0)
    {
        pageNum = 0;
    }
    else
    {
        for (pageNum = PageMap->PageCount-1; pageNum >=0; pageNum--)
        {
            if ((pReference->OrderId >= PageMap->PageArray[pageNum].InitialOrderId) &&
                (pReference->OrderId <= PageMap->PageArray[pageNum].FinalOrderId))
            {
                pPage = PageMap->PageArray[pageNum].pPage;
                break;
            }
            else if (pReference->OrderId > PageMap->PageArray[pageNum].FinalOrderId)
            {
                pageNum++;
                break;
            }
        }

        if (pageNum < 0)
        {
            ieutTRACEL(pThreadData, pReference->OrderId, ENGINE_NORMAL_TRACE, "Q %p (internalname: %s): Rehydrating oId %lu when  current earliest page starts at %lu\n",
                       Q, Q->InternalName, pReference->OrderId, PageMap->PageArray[0].InitialOrderId);

            //Set up so that we insert a page at the start of the array.
            pageNum = 0;
            pPage = NULL;
        }
    }

    if (pPage == NULL)
    {
        // Have we got room in the array
        if (PageMap->PageCount == PageMap->ArraySize)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            size_t memSize = ieqPAGEMAP_SIZE(PageMap->ArraySize + ieqPAGEMAP_INCREMENT);
            NewPageMap = (ieqPageMap_t *)iere_realloc(pThreadData,
                                                      resourceSet,
                                                      IEMEM_PROBE(iemem_intermediateQ, 4),
                                                      Q->PageMap,
                                                      memSize);
            if (NewPageMap == NULL)
            {
                rc = ISMRC_AllocateError;

                // The failure to allocate memory during restart is close to
                // the end of the world!
                ism_common_setError(rc);

                ieutTRACEL(pThreadData, memSize, ENGINE_SEVERE_ERROR_TRACE,
                           "%s: iere_realloc(%ld) failed! (rc=%d)\n", __func__,
                           memSize, rc);

                goto mod_exit;
            }
            NewPageMap->ArraySize += ieqPAGEMAP_INCREMENT;
            Q->PageMap = NewPageMap;
            PageMap = NewPageMap;
        }
        if (pageNum <= (PageMap->PageCount-1))
        {
            // We are adding a page in the middle of the existing pages so
            // we have some moving to do, pageNum points to where we must
            // insert the next page
            memmove(&PageMap->PageArray[pageNum+1],
                    &PageMap->PageArray[pageNum],
                    sizeof(ieqPageMapEntry_t) * (PageMap->PageCount-pageNum));
        }

        //We don't currently use high capacity pages during rehydration as the page maps assumes each page is the same
        //size....
        pPage = ieiq_createNewPage( pThreadData
                                  , Q
                                  , ieiqPAGESIZE_HIGHCAPACITY
                                  , 0);
        if (pPage == NULL)
        {
            rc = ISMRC_AllocateError;

            // The failure to allocate memory during restart is close to
            // the end of the world!
            ism_common_setError(rc);

            ieutTRACEL(pThreadData, ieiqPAGESIZE_HIGHCAPACITY, ENGINE_SEVERE_ERROR_TRACE,
                       "%s: ieiq_createNewPage() failed! (rc=%d)\n", __func__, rc);

            goto mod_exit;
        }
        // Initialise the page as if all of the messages have been consumed
        for (nodeCounter=0; nodeCounter < pPage->nodesInPage; nodeCounter++)
        {
            pPage->nodes[nodeCounter].msgState = ismMESSAGE_STATE_CONSUMED;
            pPage->nodes[nodeCounter].msg      = MESSAGE_STATUS_EMPTY;
        }
        PageMap->PageArray[pageNum].pPage = pPage;
        PageMap->PageArray[pageNum].InitialOrderId = ((pReference->OrderId-1) & ~(nodesInPage-1)) +1;
        PageMap->PageArray[pageNum].FinalOrderId = ((pReference->OrderId-1) | (nodesInPage-1)) +1;

        ieutTRACEL(pThreadData, pageNum, ENGINE_NORMAL_TRACE, "curPage num=%ld InitialOrderId=%ld FinalOrderId=%ld\n",
                   pageNum, PageMap->PageArray[pageNum].InitialOrderId, PageMap->PageArray[pageNum].FinalOrderId);

        // Check that our calculated InitialPageId looks ok
        assert((pageNum == 0) || (PageMap->PageArray[pageNum].InitialOrderId > PageMap->PageArray[pageNum-1].FinalOrderId));
        PageMap->PageCount++;
    }

    // And finally update the NodePage with the message
    pNode = &(pPage->nodes[ (pReference->OrderId-1) & (nodesInPage-1) ]);

    // Check that the node is empty.
    if ((pNode->msg != MESSAGE_STATUS_EMPTY) ||
        (pNode->msgState != ismMESSAGE_STATE_CONSUMED))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                      , "ieiq_rehydrateMsg called for existing OrderId .", rc
                      , "OrderId", &(pReference->OrderId), sizeof(uint64_t)
                      , "Node", pNode, sizeof(ieiqQNode_t)
                      , NULL );
    }

    pNode->msgState = (pReference->State & 0xc0) >> 6;
    if ((pNode->msgState == ismMESSAGE_STATE_DELIVERED) ||
        (pNode->msgState == ismMESSAGE_STATE_RECEIVED))
    {
        if (Q->inflightDeqs == 0)
        {
            Q->preDeleteCount++; //Having message inflight out of the queue adds one to the predelete count
        }
        Q->inflightDeqs++;
    }
    else if (    (pNode->msgState == ismMESSAGE_STATE_AVAILABLE)
              && (pMsg->Header.Expiry != 0))
    {
        iemeBufferedMsgExpiryDetails_t msgExpiryData = { pReference->OrderId , pNode, pMsg->Header.Expiry };
        ieme_addMessageExpiryData( pThreadData, (ismEngine_Queue_t *)Q,  &msgExpiryData);
    }

    pNode->deliveryId = 0; // The Delivery Id is stored in the MDR and set separately.
    pNode->deliveryCount = pReference->State & 0x3f;
    ieqRefValue_t RefValue;
    RefValue.Value = pReference->Value;
    pNode->msgFlags = RefValue.Parts.MsgFlags;

    pNode->orderId = pReference->OrderId;
    pNode->inStore = true;
    pNode->hMsgRef = hMsgRef;

    pMsg->usageCount++; // Bump the count of message references

    // First bump the store ref count for this message
    rc = iest_rehydrateMessageRef( pThreadData, pMsg );
    if (rc != OK)
    {
        ieutTRACEL(pThreadData, pMsg, ENGINE_ERROR_TRACE,
                   "%s: iest_rehydrateMessageRef failed! (rc=%d)\n",
                   __func__, rc);
        goto mod_exit;
    }

    // Update message resource set information
    iere_updateMessageResourceSet(pThreadData, resourceSet, pMsg, false, false);

    // Increase the count of messages on the queue
    iere_primeThreadCache(pThreadData, resourceSet);
    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, 1);
    pThreadData->stats.bufferedMsgCount++;
    Q->bufferedMsgs++;
    Q->bufferedMsgsHWM = Q->bufferedMsgs;

    if ((pTranData) && (pTranData->pTran))
    {
        // And finally create the softlog entry which will eventually
        // put the message on the queue when commit is called.
        ieiqSLEPut_t SLE;
        SLE.pQueue = Q;
        SLE.pNode = pNode;
        SLE.bInStore = pNode->inStore;
        SLE.pMsg = pMsg;
        SLE.bSavepointRolledback = false;

        // Ensure we have details of the link to the transaction
        SLE.TranRef.hTranRef = pTranData->operationRefHandle;
        SLE.TranRef.orderId = pTranData->operationReference.OrderId;

        // Add the message to the Soft log to be added to the queue when
        // commit is called
        rc = ietr_softLogRehydrate( pThreadData
                                  , pTranData
                                  , ietrSLE_IQ_PUT
                                  , NULL
                                  , ieiq_SLEReplayPut
                                  , MemoryCommit | PostCommit | Rollback | PostRollback | SavepointRollback
                                  , &SLE
                                  , sizeof(SLE)
                                  , 0, 1
                                  , NULL );

        if (rc != OK)
        {
            ieutTRACEL(pThreadData, rc, ENGINE_ERROR_TRACE,
                       "%s: ietr_softLogAdd failed! (rc=%d)\n",
                       __func__, rc);
            goto mod_exit;
        }

        Q->inflightEnqs++;

        // Record that the queue can not be deleted until this put is completed
        // We do so because this is a rehdrated transaction, which does not have
        // any hold over the topic tree - so it does not protect the queue from
        // being deleted.
        //
        // When the rehydrated transaction completes the count will be decremented.
        Q->preDeleteCount++;
    }
    else
    {
        // There is message isn't going to be backed out so update now
        pNode->msg = pMsg;
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Complete the rehydration of a queue
///  @remarks
///    This function is called to complete the rehydration of a queue
///    by linking up the pages of messages, and freeing the array of
///    pages.
///
///  @param[in] Qhdl               - Q to put to
///////////////////////////////////////////////////////////////////////////////
int32_t ieiq_completeRehydrate( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl )
{
    uint32_t rc = OK;
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    ieqPageMap_t *PageMap;                // Local pointer to page map
    ieiqQNodePage_t *pCurPage = NULL;
    ieiqQNodePage_t *pNextPage;
    uint32_t pageNum;
    int32_t nodeNum;  // Must be signed
    ieiqQNode_t *pNode;
    ieiqPageSeqNo_t PageSeqNo = 0;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p\n", __func__, Q);

    if (Q->Common.resourceSet == iereNO_RESOURCE_SET)
    {
        // We expect subscriptions to be assigned to a resource set, if we don't have one now
        // assign it to the default (which may be iereNO_RESOURCE_SET in which case this is a
        // no-op)
        if (Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE)
        {
            iereResourceSetHandle_t defaultResourceSet = iere_getDefaultResourceSet();
            ieiq_updateResourceSet(pThreadData, Q, defaultResourceSet);
        }
    }

    assert(Q->PageMap != NULL);

    // Recovery is single-threaded so need to obtain the put lock
    PageMap=Q->PageMap;

    ieutTRACEL(pThreadData, Q, ENGINE_NORMAL_TRACE, "Q=%p queue has %d pages\n", Q, PageMap->PageCount);

    assert(Q->QueueRefContext == ismSTORE_NULL_HANDLE);
    assert(Q->hStoreObj != ismSTORE_NULL_HANDLE);
    assert(Q->nextOrderId == 1);

    // Need to open the reference context and pick up stats
    ismStore_ReferenceStatistics_t RefStats = { 0 };

    rc = ism_store_openReferenceContext( Q->hStoreObj
                                       , &RefStats
                                       , &Q->QueueRefContext );

    if (rc != OK) goto mod_exit;

    Q->nextOrderId = RefStats.HighestOrderId +1;

    ieutTRACEL(pThreadData, Q->nextOrderId, ENGINE_HIGH_TRACE, "Q->nextOrderId set to %lu\n", Q->nextOrderId);

    if (PageMap->PageCount)
    {
        // Loop through the pages and link them up.
        Q->headPage = PageMap->PageArray[0].pPage;
        Q->head = &(Q->headPage->nodes[0]);
        ((ieiqQNodePage_t *)(PageMap->PageArray[0].pPage))->sequenceNo = ++PageSeqNo;

        Q->cursor = Q->head;

        pCurPage = Q->headPage;
        for (pageNum=1; pageNum < PageMap->PageCount; pageNum++)
        {
            assert(PageMap->PageArray[pageNum].InitialOrderId >
                   PageMap->PageArray[pageNum-1].FinalOrderId);

            pCurPage->nextStatus = completed;
            pCurPage->next = PageMap->PageArray[pageNum].pPage;
            pCurPage = pCurPage->next;
            pCurPage->sequenceNo = ++PageSeqNo;

            ieutTRACEL(pThreadData, pageNum, ENGINE_NORMAL_TRACE, "Page %d = %p\n", pageNum, pCurPage);
        }

        // Set the tail to the first free slot in the last page
        Q->tail = NULL;
        for (nodeNum=pCurPage->nodesInPage-1; nodeNum >= 0; nodeNum--)
        {
            if ((pCurPage->nodes[nodeNum].msg == NULL) &&
                (pCurPage->nodes[nodeNum].msgState == ismMESSAGE_STATE_CONSUMED))
            {
                pCurPage->nodes[nodeNum].msgState = ismMESSAGE_STATE_AVAILABLE;
            }
            else
            {
                Q->tail = &(pCurPage->nodes[nodeNum+1]);
                break;
            }
        }

        // Identify that the next page is pending
        pCurPage->nextStatus = failed;
    }

    // Only create a clean page of messages if we filled the last page
    // completely or the queue is empty
    if ((Q->tail == NULL) ||
        (Q->tail->msgState == ieqMESSAGE_STATE_END_OF_PAGE))
    {
        // Either there are no messages on the queue, or the last message has filled the last
        // page so we have to create a new page... this isn't used with our fixed pagesize page
        // map so can vary. If it's going to be the first page, we create it SMALL (as we would
        // have done when creating the queue for the 1st time) otherwise, choose based on capacity.
        uint32_t nodesInPage = (Q->tail == NULL) ? ieiqPAGESIZE_SMALL : ieiq_choosePageSize();
        pNextPage = ieiq_createNewPage( pThreadData
                                      , Q
                                      , nodesInPage
                                      , PageSeqNo++);
        if (pNextPage == NULL)
        {
            // Argh - we've run out of memory - record we failed to add a
            // page and return the error
            ism_common_setError(rc);
            rc = ISMRC_AllocateError;

            ieutTRACEL(pThreadData, nodesInPage, ENGINE_ERROR_TRACE, "%s: ieiq_createNewPage failed!\n", __func__);
            goto mod_exit;
        }
        pNextPage->nextStatus = failed;

        if (Q->tail == NULL)
        {
            // First page on queue
            Q->headPage = pNextPage;
            Q->head = &(Q->headPage->nodes[0]);

            Q->cursor = Q->head;
        }
        // We can suppress BEAM complaining that we are operating on a NULL pCurPage
        // here because the only way pCurPage can be NULL at this point is if PageMap->PageCount
        // is 0, at which time Q->tail will also be NULL (so we won't enter this logic).
        else
        {
            pCurPage->nextStatus = completed; /* BEAM suppression: operating on NULL */
            pCurPage->next = pNextPage;
        }
        pCurPage = pNextPage;

        Q->tail = &(pCurPage->nodes[0]);
    }
    else
    {
        // mark the remainder of the nodes on the page as available
        for (pNode=Q->tail; pNode->msgState != ieqMESSAGE_STATE_END_OF_PAGE; pNode++)
        {
            pNode->msgState = ismMESSAGE_STATE_AVAILABLE;
        }
    }

    // And last but not least free the page map
    if (Q->PageMap != NULL)
    {
        iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_freeStruct(pThreadData, resourceSet, iemem_intermediateQ, Q->PageMap, Q->PageMap->StrucId);
        Q->PageMap = NULL;
    }

    //Ensure the putsAttempted count is up to date
    Q->putsAttempted = Q->qavoidCount + Q->enqueueCount + Q->rejectedMsgs;

    //This queue is no longer in recovery
    Q->QOptions &= ~ieqOptions_IN_RECOVERY;

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    After rehydration, delete this queue if it is no longer required
///  @remarks
///    This function is called during complete rehydration to ensure queues we
///    have left over but no longer need don't hang around for ever.
///
///  @param[in] Qhdl               - Q to put to
///////////////////////////////////////////////////////////////////////////////
void ieiq_removeIfUnneeded( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl )
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    ieutTRACEL(pThreadData, Q,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "Q=%p\n", __func__, Q);

    // This is our opportunity to delete queues that were rediscovered during rehydration.
    if (Q->isDeleted)
    {
        // If this queue has a message-delivery state, we need to remove this
        // before we delete the SDR so the MDRs aren't orphaned
        if (Q->hMsgDelInfo != NULL)
        {
            int32_t rc2;

            rc2 = iecs_releaseAllMessageDeliveryReferences(pThreadData, Q->hMsgDelInfo, Q->hStoreObj, false);

            if (rc2 != OK)
            {
                ieutTRACEL(pThreadData, rc2, ENGINE_ERROR_TRACE,
                           "%s: iecs_releaseAllMessageDeliveryReferences (%s) failed! (rc=%d)\n",
                           __func__, Q->InternalName, rc2);
            }

            iecs_releaseMessageDeliveryInfoReference(pThreadData, Q->hMsgDelInfo);
            Q->hMsgDelInfo = NULL;
        }

        //Ensure inflight outbound messages don't block the deletion
        ieiq_forgetInflightMsgs(pThreadData, Qhdl);

        Q->deletionRemovesStoreObjects = true; // remove from store

        ieiq_reducePreDeleteCount(pThreadData, (ismQHandle_t)Q);
    }

    ieutTRACEL(pThreadData, Q, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Rehydrates the delivery id for a message on an Intermediate Q
///  @remarks
///    This function is called during recovery to set the delivery id
///    on a message on an intermediate queue.
///
///  @param[in] Qhdl               - Q to put to
///  @param[in] hMsgDelInfo        - handle of message-delivery information
///  @param[in] hMsgRef            - handle of message reference
///  @param[in] deliveryId         - Delivery id
///  @param[out] ppnode            - Pointer to used node (set to NULL for intermediate)
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ieiq_rehydrateDeliveryId( ieutThreadData_t *pThreadData
                                , ismQHandle_t Qhdl
                                , iecsMessageDeliveryInfoHandle_t hMsgDelInfo
                                , ismStore_Handle_t hMsgRef
                                , uint32_t deliveryId
                                , void **ppnode )
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    uint32_t pageNum, nodeNum;
    int32_t rc = 0;

    ieutTRACEL(pThreadData, deliveryId, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p hMsgRef=0x%0lx deliveryId=%u\n",
               __func__, Q, hMsgRef, deliveryId);

    assert(hMsgRef != ismSTORE_NULL_HANDLE);

    // Recovery is single-threaded so no need to obtain the put lock
    ieqPageMap_t *PageMap=Q->PageMap;
    assert(PageMap != NULL);

    if (Q->hMsgDelInfo == NULL)
    {
        Q->hMsgDelInfo = hMsgDelInfo;
        iecs_acquireMessageDeliveryInfoReference(pThreadData, NULL, &Q->hMsgDelInfo);
    }

    // We never return the node for intermediate queue
    *ppnode = NULL;

    // Lookup for the message in the page map
    for (pageNum = 0; pageNum < PageMap->PageCount; pageNum++)
    {
        ieiqQNodePage_t *pPage = PageMap->PageArray[pageNum].pPage;

        for (nodeNum=0; nodeNum < pPage->nodesInPage; nodeNum++)
        {
            if ((pPage->nodes[nodeNum].inStore) &&
                (pPage->nodes[nodeNum].hMsgRef == hMsgRef))
            {
                pPage->nodes[nodeNum].hasMDR = true;
                pPage->nodes[nodeNum].deliveryId = deliveryId;
                goto mod_exit;
            }
        }
    }

    rc = ISMRC_NoMsgAvail;

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Associate the Queue and the Consumer
///  @remarks
///    Associate the Consumer passed with the queue. When the consumer
///    is enabled messages on this wueue will be delivered to the consumer.
///
///  @param[in] Qhdl               - Queue
///  @param[in] pConsumer          - Consumer
///  @return                       - OK
///////////////////////////////////////////////////////////////////////////////
int32_t ieiq_initWaiter( ieutThreadData_t *pThreadData,
                         ismQHandle_t Qhdl,
                         ismEngine_Consumer_t *pConsumer)
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    int32_t rc=OK;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n", __func__, Q);

    //This queue is used for mqtt qos 1 & 2 which require acks and deliveryids, check they are being asked for
    assert(pConsumer->fAcking);
    assert(pConsumer->fShortDeliveryIds);

    bool doneInit = false;
    bool tryAgain;

    do
    {
        tryAgain = false;

        iewsWaiterStatus_t oldStatus = __sync_val_compare_and_swap( &(Q->waiterStatus)
                                                                  , IEWS_WAITERSTATUS_DISCONNECTED
                                                                  , IEWS_WAITERSTATUS_DISABLED);

        if (oldStatus == IEWS_WAITERSTATUS_DISCONNECTED)
        {
            doneInit = true;
        }
        else if (oldStatus == IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT)
        {
            tryAgain = true;
        }
    }
    while(tryAgain);

    if (doneInit)
    {
        __sync_fetch_and_add( &(Q->preDeleteCount), 1); //don't delete the queue whilst this consumer is connected

        Q->pConsumer = pConsumer;

        //We need to check for any inflight messages to deliver
        Q->Redelivering = true;
        Q->resetCursor = true;

        // We don't want to miss out any messages we have tried to redeliver in the past
        Q->redeliverCursorOrderId = 0;

        // Update how many messages can be inflight from this queue...
        //    (given that we are connecting a new wait we'll call checkWaiters when appropriate)
        Q->maxInflightDeqs = pConsumer->pSession->pClient->maxInflightLimit;
        assert(Q->maxInflightDeqs != 0);
    }
    else
    {
        rc = ISMRC_WaiterInUse;
        ism_common_setError(rc);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Remove the consumer from the queue.
///  @remarks
///    May happen async-lu (if the waiter is locked)...
///  ism_engine_deliverStatus() when we're done
///
///  @param[in] Qhdl               - Queue
///  @param[in] pConsumer          - Consumer
///  @return                       - OK
///////////////////////////////////////////////////////////////////////////////
int32_t ieiq_termWaiter( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ismEngine_Consumer_t *pConsumer )
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n", __func__, Q);

    //This type of queue only has one consumer... so they had better be destroying that one!
    assert(Q->pConsumer == pConsumer);

    bool doneDisconnect = false;
    bool waiterInUse = false;
    iewsWaiterStatus_t oldState;

    // First we need to move the waiter into DISCONNECT_PEND state. It may be
    // that DISABLE_PEND will also be on, but that is ok.
    do
    {
        oldState = Q->waiterStatus;
        iewsWaiterStatus_t newState;

        if ((oldState == IEWS_WAITERSTATUS_DISCONNECTED) ||
            ((oldState & IEWS_WAITERSTATUS_DISCONNECT_PEND) != 0))
        {
            // You can't disconnect the waiter if it's already disconnect(ing)
            rc = ISMRC_WaiterInvalid;
            goto mod_exit;
        }
        else if(  (oldState == IEWS_WAITERSTATUS_GETTING)
                ||(oldState == IEWS_WAITERSTATUS_DELIVERING))
        {
            newState = IEWS_WAITERSTATUS_DISCONNECT_PEND;
            waiterInUse = true;
        }
        else if (oldState & IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND)
        {
            //This flag implies an earlier disable was cancelled... this is getting very confusing. For the
            //moment lets just take a deep breathe and wait for things to calm down
            ismEngine_FullMemoryBarrier();
            continue;
        }
        else if( oldState & IEWS_WAITERSTATUSMASK_PENDING)
        {
            //We know the enable flag is not set (special cased above)
            newState = oldState | IEWS_WAITERSTATUS_DISCONNECT_PEND;
            waiterInUse = true;
        }
        else if (oldState == IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT)
        {
            //try again.
            ismEngine_FullMemoryBarrier();
            continue;
        }
        else
        {
            newState = IEWS_WAITERSTATUS_DISCONNECT_PEND;
            waiterInUse = false;
        }

        doneDisconnect = __sync_bool_compare_and_swap( &(Q->waiterStatus)
                                                     , oldState
                                                     , newState);
    } while (!doneDisconnect);

    if (waiterInUse)
    {
        // The waiter is (was) locked by someone else when we switched
        // on the IEWS_WAITERSTATUS_DISCONNECT_PEND state, so our work
        // is done.
        rc = ISMRC_AsyncCompletion;
    }
    else
    {
        bool waiterWasEnabled = (oldState != IEWS_WAITERSTATUS_DISABLED) ?  true : false;

        ieq_completeWaiterActions( pThreadData
                                 , Qhdl
                                 , Q->pConsumer
                                 , IEQ_COMPLETEWAITERACTION_OPTS_NONE
                                 , waiterWasEnabled);
    }

mod_exit:
    //NB: The above callback may have destroyed the queue so
    //we cannot use it at this point
    ieutTRACEL(pThreadData,rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__,rc);
    return rc;
}

static inline void ieiq_destroyMessageBatch_finish( ieutThreadData_t *pThreadData
                                                  , ieiqQueue_t *Q
                                                  , uint32_t batchSize
                                                  , ieiqQNode_t *discardNodes[]
                                                  , bool *doPageCleanup)
{
    for (uint32_t i=0; i < batchSize; i++)
    {
        ieiqQNode_t *pnode = discardNodes[i];

        // Before releasing our hold on the node, should we subsequently call
        // cleanupHeadPage?
        if ((pnode+1)->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
        {
            (*doPageCleanup) = true;
        }

        ismEngine_Message_t *pMsg = pnode->msg;
        pnode->msg = MESSAGE_STATUS_EMPTY;

        iem_releaseMessage(pThreadData, pMsg);

        //Once we mark the node consumed we can no longer access it
        pnode->msgState = ismMESSAGE_STATE_CONSUMED;
    }
}

static inline int32_t ieiq_consumeMessageBatch_unstoreMessages( ieutThreadData_t *pThreadData
                                                              , ieiqQueue_t *Q
                                                              , uint32_t batchSize
                                                              , ieiqQNode_t *discardNodes[]
                                                              , bool *deliveryIdsReleased
                                                              , ismEngine_AsyncData_t *pAsyncData )
{
    uint32_t storeOpsCount = 0;
    int32_t rc = OK;

    for (uint32_t i=0; i < batchSize; i++)
    {
        ieiqQNode_t *pnode = discardNodes[i];

        // Decrement our reference to the msg and possibly schedule it's later deletion from the store
        // from the store.
        if (pnode->inStore)
        {
            if (pnode->hasMDR)
            {
                pnode->hasMDR = false;

                rc = iecs_completeUnstoreMessageDeliveryReference( pThreadData
                                                                 , pnode->msg
                                                                 , Q->hMsgDelInfo
                                                                 , pnode->deliveryId);

                if (rc != OK)
                {
                    // If we didn't have the MDR, that's a bit surprising but we
                    // were trying to remove something that's not there. Don't panic.
                    if (rc == ISMRC_NotFound)
                    {
                        rc = OK;
                    }
                    else if (rc == ISMRC_DeliveryIdAvailable)
                    {
                        //We can deliver messages again
                        *deliveryIdsReleased = true;
                        rc = OK;
                    }
                    else
                    {
                        ieutTRACE_FFDC( ieutPROBE_003, true
                                      , "iecs_completeunstoreMessageDeliveryReference failed.", rc
                                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                                      , "Queue", Q, sizeof(ieiqQueue_t)
                                      , "Delivery Id", &pnode->deliveryId, sizeof(pnode->deliveryId)
                                      , "Order Id", &pnode->orderId, sizeof(pnode->orderId)
                                      , "pNode", pnode, sizeof(ieiqQNode_t)
                                      , NULL );

                    }
                }
            }

            iest_unstoreMessage( pThreadData
                               , pnode->msg
                               , false
                               , false
                               , NULL
                               , &storeOpsCount);
        }

    }

    if (storeOpsCount > 0)
    {
        rc = iead_store_asyncCommit(pThreadData, false, pAsyncData);
        storeOpsCount = 0;
    }

    return rc;
}

static ieiqQNode_t **getDiscardNodesFromAsyncInfo(ismEngine_AsyncData_t *asyncInfo)
{
    ieiqQNode_t **pDiscardNodes = NULL;

    uint32_t entry2 = asyncInfo->numEntriesUsed-1;
    uint32_t entry1 = entry2 - 1;

    if (   (asyncInfo->entries[entry1].Type != ieiqQueueDestroyMessageBatch1)
         ||(asyncInfo->entries[entry2].Type != ieiqQueueDestroyMessageBatch2))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true,
            "asyncInfo stack not as expected (corrupted?)", ISMRC_Error,
            NULL);
    }

    pDiscardNodes = (ieiqQNode_t **)(asyncInfo->entries[entry1].Data);

    return pDiscardNodes;
}


int32_t ieiq_asyncDestroyMessageBatch(
                ieutThreadData_t               *pThreadData,
                int32_t                         retcode,
                ismEngine_AsyncData_t          *asyncinfo,
                ismEngine_AsyncDataEntry_t     *context)
{
    ieiqAsyncDestroyMessageBatchInfo_t *asyncData = (ieiqAsyncDestroyMessageBatchInfo_t *)context->Data;
    ieiqQNode_t **pDiscardNodes = getDiscardNodesFromAsyncInfo(asyncinfo);

    bool deliveryIdsReleased = false;
    bool doPageCleanup = false;
    assert(retcode == OK);
    int32_t rc = OK;

    ieiqQueue_t *Q = asyncData->Q;

    ismEngine_CheckStructId(asyncData->StrucId, IEIQ_ASYNCDESTROYMESSAGEBATCHINFO_STRUCID, ieutPROBE_001);


    if (asyncData->removedStoreRefs == false)
    {
        //This is the first time this callback has been called since the references have been
        //removed from the store. We need to unstore the messages....
        asyncData->removedStoreRefs = true;

        rc = ieiq_consumeMessageBatch_unstoreMessages( pThreadData
                                                     , Q
                                                     , asyncData->batchSize
                                                     , pDiscardNodes
                                                     , &deliveryIdsReleased
                                                     , asyncinfo);

        if (rc == ISMRC_AsyncCompletion)
        {
            goto mod_exit;
        }
    }

    //Don't need this callback to be run again if we go async... there are two entries
    //(we checked they were the correct ones when we looked up pDeliverHdls)
    iead_popAsyncCallback( asyncinfo, context->DataLen);
    iead_popAsyncCallback( asyncinfo,
                                  ((asyncData->batchSize) * sizeof(ieiqQueue_t *)));

    ieiq_destroyMessageBatch_finish( pThreadData
                                   , Q
                                   , asyncData->batchSize
                                   , pDiscardNodes
                                   , &doPageCleanup);

    //We are destroying messages.... they should not have delivery ids associated with them
    assert(!deliveryIdsReleased);

    if (doPageCleanup)
    {
        ieiq_cleanupHeadPage(pThreadData, Q);
    }

    //Increased when we realised destroying this batch might go async
    ieiq_reducePreDeleteCount(pThreadData, (ismQHandle_t)Q);
mod_exit:
   return rc;
}

//waiter must be locked before calling this function
static inline void ieiq_destroyMessageBatch(  ieutThreadData_t *pThreadData
                                              , ieiqQueue_t *Q
                                              , uint32_t batchSize
                                              , ieiqQNode_t *discardNodes[]
                                              , bool removeExpiryData
                                              , bool *doPageCleanup
                                              , bool *deliveryIdsReleased)
{
    uint32_t storeOpsCount = 0;
    bool anyNodesInStore = false;
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    //First thing we do is reduce queue depth as these messages are definitely not available for getting
    //we do this before committing from the store so we don't choose to keep discarding more if, after the
    //discard of these is complete - the queue will be fine
    iere_primeThreadCache(pThreadData, resourceSet);
    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -(int64_t)batchSize);
    pThreadData->stats.bufferedMsgCount -= batchSize;
    DEBUG_ONLY int32_t oldDepth = __sync_fetch_and_sub(&(Q->bufferedMsgs), batchSize);
    assert(oldDepth >= batchSize);

    bool increasedPreDeleteCount = false;

    if (removeExpiryData)
    {
        for (uint32_t i=0; i < batchSize; i++)
        {
            ieiqQNode_t *pnode = discardNodes[i];

            if (pnode->msg->Header.Expiry != 0)
            {
                ieme_removeMessageExpiryData( pThreadData, (ismEngine_Queue_t *)Q,  pnode->orderId);
            }
        }
    }

    for (uint32_t i=0; i < batchSize; i++)
    {
        ieiqQNode_t *pnode = discardNodes[i];
        pnode->msgState = ieqMESSAGE_STATE_DISCARDING;  //don't deliver this node - special state that does not
                                                        //count towards queue depth

        if (pnode->inStore)
        {
            ieutTRACEL(pThreadData, pnode->orderId, ENGINE_HIFREQ_FNC_TRACE, "pnode %p, oId %lu, msg %p\n",
                               pnode, pnode->orderId, pnode->msg);

            anyNodesInStore = true;

            // Delete the reference to the message from the queue
            rc = ism_store_deleteReference( pThreadData->hStream
                                          , Q->QueueRefContext
                                          , pnode->hMsgRef
                                          , pnode->orderId
                                          , 0 );
            if (rc != OK)
            {
                // The failure to delete a store reference means that the
                // queue is inconsistent.
                ieutTRACE_FFDC( ieutPROBE_002, true, "ism_store_deleteReference failed.", rc
                              , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                              , "Queue", Q, sizeof(ieiqQueue_t)
                              , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                              , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                              , "pNode", pnode, sizeof(ieiqQNode_t)
                              , NULL );

            }
            storeOpsCount++;

            // Delete any MDR for this message
            if (pnode->hasMDR)
            {
                //TODO: This takes a lock each time through, could move to lock for whole batch
                rc = iecs_startUnstoreMessageDeliveryReference( pThreadData
                                                              , pnode->msg
                                                              , Q->hMsgDelInfo
                                                              , pnode->deliveryId
                                                              , &storeOpsCount);

                if (rc != OK)
                {
                    // If we didn't have the MDR, that's a bit surprising but we
                    // were trying to remove something that's not there. Don't panic.
                    if (rc == ISMRC_NotFound)
                    {
                        rc = OK;
                    }
                    else
                    {
                        ieutTRACE_FFDC( ieutPROBE_003, true
                                      , "iecs_startUnstoreMessageDeliveryReference failed.", rc
                                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                                      , "Queue", Q, sizeof(ieiqQueue_t)
                                      , "Delivery Id", &pnode->deliveryId, sizeof(pnode->deliveryId)
                                      , "Order Id", &pnode->orderId, sizeof(pnode->orderId)
                                      , "pNode", pnode, sizeof(ieiqQNode_t)
                                      , NULL );

                    }
                }
            }
        }
        else
        {
            if (pnode->deliveryId > 0)
            {
                // Need to free up the deliveryId
                rc = iecs_releaseDeliveryId( pThreadData, Q->hMsgDelInfo, pnode->deliveryId );

                if (rc != OK)
                {
                    if (rc == ISMRC_DeliveryIdAvailable)
                    {
                        //We're destroying messages... they shouldn't have a delivery id associated...right? (right????)
                        assert(false);
                        *deliveryIdsReleased = true;
                        rc = OK;
                    }
                    else if(rc == ISMRC_NotFound)
                    {
                        // We were trying to remove something that's not there. Don't panic. (release will have written an FFDC and continued)
                        rc = OK;
                    }
                    else
                    {
                        ieutTRACE_FFDC( ieutPROBE_004, true
                                , "iecs_releaseDeliveryId failed for free deliveryId case.", rc
                                , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                                , "Queue", Q, sizeof(ieiqQueue_t)
                                , "Delivery Id", &pnode->deliveryId, sizeof(pnode->deliveryId)
                                , "Order Id", &pnode->orderId, sizeof(pnode->orderId)
                                , "pNode", pnode, sizeof(ieiqQNode_t)
                                , NULL );
                    }
                }
            }
        }
    }
    if (anyNodesInStore)
    {
        //Get ready in case we need to go async, we don't need to tie it to completion of any async stack of functions we're already in
        //Increase the predelete count of the queue so the queue won't go away.
        increasedPreDeleteCount = true;
        __sync_fetch_and_add(&(Q->preDeleteCount), 1);

        //If it's possible we will be restarting delivery (i.e. freeing delivery ids) we would need to increase usecount on consumer/session
        //here.... but if it has a deliveryid we shouldn't be destroying it.


        ieiqAsyncDestroyMessageBatchInfo_t asyncBatchData = {
                                                     IEIQ_ASYNCDESTROYMESSAGEBATCHINFO_STRUCID
                                                   , Q
                                                   , batchSize
                                                   , false
                                                };

        ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
                {ismENGINE_ASYNCDATAENTRY_STRUCID, ieiqQueueDestroyMessageBatch1,  discardNodes,  batchSize*sizeof(ieiqQNode_t *), NULL, {.internalFn = ieiq_asyncDestroyMessageBatch } },
                {ismENGINE_ASYNCDATAENTRY_STRUCID, ieiqQueueDestroyMessageBatch2,  &asyncBatchData,   sizeof(asyncBatchData),     NULL, {.internalFn = ieiq_asyncDestroyMessageBatch }}};
        ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID, NULL, IEAD_MAXARRAYENTRIES, 2, 0, true,  0, 0, asyncArray};

        if (storeOpsCount > 0)
        {
            // We need to commit the removal of the reference before we can call iest_unstoreMessage (in case we reduce the usage
            // count to 1, some else decreases it to 0 and commits but we don't commit our transaction. On restart we'd point to a message
            // that has been deleted...
            rc = iead_store_asyncCommit(pThreadData, false, &asyncData);
            storeOpsCount = 0;
        }

        if (rc != ISMRC_AsyncCompletion)
        {
            // We're ready to remove the messages, the removal of references to them has already been committed
            asyncBatchData.removedStoreRefs = true;

            rc = ieiq_consumeMessageBatch_unstoreMessages( pThreadData
                                                         , Q
                                                         , batchSize
                                                         , discardNodes
                                                         , deliveryIdsReleased
                                                         , &asyncData);
        }
    }

    if (rc !=  ISMRC_AsyncCompletion)
    {
        ieiq_destroyMessageBatch_finish( pThreadData
                                       , Q
                                       , batchSize
                                       , discardNodes
                                       , doPageCleanup);
    }

    if (rc == ISMRC_AsyncCompletion)
    {
        //Our caller doesn't need to know that this batch is not entirely destroyed, they are all in a state
        //that won't be delivered when the waiter is unlocked
        rc = OK;
    }
    else
    {
        if (rc != OK)
        {
            ieutTRACE_FFDC( ieutPROBE_005, true
                          , "ieiq_destroyMessageBatch failed in unexpected way", rc
                          , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                          , "Queue", Q, sizeof(ieiqQueue_t)
                          , NULL );
        }
        if (increasedPreDeleteCount)
        {
            ieiq_reducePreDeleteCount(pThreadData, (ismQHandle_t)Q);
            increasedPreDeleteCount = false;
        }
    }

    return;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Remove messages from the front of the queue to make space for new messages
///
///  @param[in] Qhdl               - Queue
///  @param[in] takeLock             Should the waiterStatus lock be taken
///                                   (^ should be true unless already taken!!!)
///
///////////////////////////////////////////////////////////////////////////////

void ieiq_reclaimSpace( ieutThreadData_t *pThreadData
                      , ismQHandle_t Qhdl
                      , bool takeLock)
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    iewsWaiterStatus_t oldStatus = Q->waiterStatus;
    iewsWaiterStatus_t newStatus = IEWS_WAITERSTATUS_DISCONNECTED;
    uint64_t removedMsgs=0;
    uint32_t maxStoreBatchSize=300;
    ieiqQNode_t *discardNodes[maxStoreBatchSize];
    uint32_t currentBatchSize = 0;
    uint32_t maxEmptyPages=50;
    ieiqQNodePage_t *emptyPages[maxEmptyPages];
    ieiqPageSeqNo_t  emptyPageSeqNos[maxEmptyPages];
    uint32_t currentEmptyPage = 0;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n", __func__, Q);

    if (takeLock)
    {
        bool gotLock = iews_tryLockForReclaimSpace ( &(Q->waiterStatus)
                                                   , &oldStatus
                                                   , &newStatus );

        if (!gotLock)
        {
            //We didn't get the lock but we did leave a flag that asked someone else to do the clean
            //Good enough!
            goto mod_exit;
        }
    }

    //Work out what the target number of bufferedmsgs is
    iepiPolicyInfo_t *pPolicyInfo = Q->Common.PolicyInfo;

    assert(pPolicyInfo->maxMessageBytes == 0);  // No code to deal with maxMessageBytes

    uint64_t targetBufferedMessages =  1 + (uint64_t)(pPolicyInfo->maxMessageCount * IEQ_RECLAIMSPACE_MSGS_SURVIVE_FRACTION);  //1+ causes target to round up not down

    //Do we need to rewind the get cursor?
    if(Q->resetCursor)
    {
        ieiq_resetCursor(pThreadData, Q);
    }

    ieiqQNode_t *scanCursor = Q->cursor;
    bool doPageCleanup = false;
    bool deliveryIdsReleased = false;
    bool doNonAckerCheck = false;
    uint64_t recordedHeadOrderId=0;

    bool cursorFollowsScan=true; //Unless we skip messages as we scan the queue we can move cursor
    uint32_t consumedNodes = 0;

    //Now look for messages to remove and consume them
    while ((Q->bufferedMsgs - currentBatchSize)  > targetBufferedMessages)
    {
        if (scanCursor->msgState == ismMESSAGE_STATE_AVAILABLE)
        {
            if(scanCursor->msg != NULL)
            {
                discardNodes[currentBatchSize] = scanCursor;
                currentBatchSize++;

                if (currentBatchSize >= maxStoreBatchSize)
                {
                    ieiq_destroyMessageBatch( pThreadData
                                            , Q
                                            , currentBatchSize
                                            , discardNodes
                                            , true
                                            , &doPageCleanup
                                            , &deliveryIdsReleased);

                    iere_primeThreadCache(pThreadData, resourceSet);
                    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_DISCARDEDMSGS, (int64_t)currentBatchSize);
                    __sync_fetch_and_add(&(Q->discardedMsgs), currentBatchSize);
                    removedMsgs += currentBatchSize;
                    currentBatchSize = 0;
                }
            }
            else
            {
                //Either an inflight put or end of queue... try and keep going but don't
                //move get cursor
                cursorFollowsScan=false;
            }
        }
        else if (scanCursor->msgState != ismMESSAGE_STATE_CONSUMED)
        {
            //This is a message being delivered. Skip it for our scan,
            //if we are redelivering inflight messages, leave the getcursor here
            if (Q->Redelivering)
            {
                cursorFollowsScan=false;
            }
        }
        else
        {
            consumedNodes++;
        }

        //Do a dirty read of the end of the queue (if we read a stale value, no problem, a different put will
        //cause another clean up. We want the node after the one we just scanned to be less than the next orderId
        //that will be used in the queue:
        if(scanCursor->orderId > (Q->nextOrderId - 2))
        {
            //Run out of messages to look at...
            break;
        }

        //Have we reached the end of the page
        if((scanCursor+1)->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
        {
            ieiqQNodePage_t *currpage = (ieiqQNodePage_t *)((scanCursor+1)->msg);

            ismEngine_CheckStructId(currpage->StrucId, IEIQ_PAGE_STRUCID, ieutPROBE_001);

            if (currpage->nextStatus < completed)
            {
                //No next page yet, we're done
                break;
            }

            if (currpage->nodesInPage == consumedNodes &&
                currentEmptyPage < maxEmptyPages)
            {
                emptyPages[currentEmptyPage] = currpage;
                emptyPageSeqNos[currentEmptyPage] = currpage->sequenceNo;
                currentEmptyPage++;
            }

            consumedNodes = 0;

            ieiqQNodePage_t *nextpage = currpage->next;
            ismEngine_CheckStructId(nextpage->StrucId, IEIQ_PAGE_STRUCID, ieutPROBE_002);
            scanCursor = &(nextpage->nodes[0]);
        }
        else
        {
            scanCursor +=1;
        }

        //Move the get cursor if it is still in sync with us
        if (cursorFollowsScan)
        {
            ieiq_moveGetCursor(pThreadData, Q, Q->cursor);
            assert(scanCursor == Q->cursor);
        }
    }

    if (currentBatchSize > 0)
    {
        //destroyMessageBatch can go async under the covers, so after restart it is possible for the discarded
        //messages to "reappear".
        ieiq_destroyMessageBatch( pThreadData
                                , Q
                                , currentBatchSize
                                , discardNodes
                                , true
                                , &doPageCleanup
                                , &deliveryIdsReleased);

        iere_primeThreadCache(pThreadData, resourceSet);
        iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_DISCARDEDMSGS, (int64_t)currentBatchSize);
        __sync_fetch_and_add(&(Q->discardedMsgs), currentBatchSize);
        removedMsgs += currentBatchSize;
        currentBatchSize = 0;
    }

    //Restart delivery for the consumer if we freed up delivery ids and the consumer is not disconnected
    //(The only cases where we don't take the lock, the consumer was connected)

    if ( deliveryIdsReleased &&
         ((!takeLock) || (oldStatus != IEWS_WAITERSTATUS_DISCONNECTED)))
    {
        assert(takeLock || (    (Q->waiterStatus != IEWS_WAITERSTATUS_DISCONNECTED)
                            &&  (Q->waiterStatus != IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT)));
        ismEngine_internal_RestartSession (pThreadData, Q->pConsumer->pSession, false);
    }

    // We found some empty pages on our travels, let's remove them if we can...
    if (currentEmptyPage != 0)
    {
        uint32_t removedPages = 0;

        ieiq_getHeadLock(Q);

        ieiqQNodePage_t *prevPage = NULL;
        ieiqQNodePage_t *currPage = Q->headPage;

        // If our first page is the head page, make sure we call page cleanup
        if (currPage == emptyPages[0])
        {
            doPageCleanup = true;
        }
        // Otherwise, free up the fully consumed pages that are 'trapped'.
        else
        {
            doNonAckerCheck = true;
            recordedHeadOrderId = Q->head->orderId;

            iere_primeThreadCache(pThreadData, resourceSet);
            while(currPage != NULL)
            {
                ieiqQNodePage_t *nextPage = currPage->next;

                if (currPage == emptyPages[removedPages])
                {
                    // We can only remove this page if it is the one we expected (it's sequence number
                    // is the one we were looking for) and the cursor is clear of it.
                    if (currPage->sequenceNo == emptyPageSeqNos[removedPages] &&
                        currPage->nextStatus == cursor_clear)
                    {
                        // Expecting this to be a nice safe page in the middle...
                        assert(nextPage != NULL);
                        assert(prevPage != NULL);

                        prevPage->next = nextPage;

                        iere_freeStruct(pThreadData, resourceSet, iemem_intermediateQPage, currPage, currPage->StrucId);

                        if (++removedPages == currentEmptyPage) break;
                    }
                    // This was not the page we were expecting or the cursor isn't clear of it,
                    // stop looking (none of the pages to the 'right' can have cursor clear either.
                    else
                    {
                        break;
                    }
                }
                else
                {
                    prevPage = currPage;
                }

                currPage = nextPage;
            }
        }

        ieiq_releaseHeadLock(Q);

        if (removedPages != 0)
        {
            ieutTRACEL(pThreadData, removedPages, ENGINE_INTERESTING_TRACE, "Removed %u 'trapped' pages.\n", removedPages);
        }
    }

    if (takeLock)
    {
        iews_unlockAfterQOperation( pThreadData
                                  , Qhdl
                                  , Q->pConsumer
                                  , &(Q->waiterStatus)
                                  , newStatus
                                  , oldStatus);
    }

    if (doNonAckerCheck)
    {
        ieiq_checkForNonAckers(pThreadData, Q,recordedHeadOrderId, (Q->pConsumer == NULL )? NULL : Q->pConsumer->pSession->pClient->pClientId);
    }

    if (doPageCleanup)
    {
        ieiq_cleanupHeadPage(pThreadData, Q);
    }

mod_exit:
    ieutTRACEL(pThreadData, removedMsgs,  ENGINE_FNC_TRACE, FUNCTION_EXIT "removed = %lu\n", __func__, removedMsgs);
    return;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Adjust expiry counter as a message expired
///pThreadData, Q
///  @param[in] Qhdl               - Queue
///  @return                       - void
///////////////////////////////////////////////////////////////////////////////
void ieiq_messageExpired( ieutThreadData_t *pThreadData
                        , ismQHandle_t Qhdl)
{
    ieiqQueue_t *Q = (ieiqQueue_t *) Qhdl;
    __sync_fetch_and_add(&(Q->expiredMsgs), 1);
    pThreadData->stats.expiredMsgCount++;
}

static void  ieiq_completeConsumeAck ( ieutThreadData_t *pThreadData
                                     , ieiqQueue_t *Q
                                     , ismEngine_Session_t *pSession
                                     , bool pageCleanup
                                     , bool deliveryIdsAvailable)
{
    __sync_fetch_and_add(&(Q->dequeueCount), 1);
    uint64_t oldInflightDeqs = __sync_fetch_and_sub(&(Q->inflightDeqs), 1);
    assert(oldInflightDeqs > 0);

    //If we need to restart delivery for this session, do it now
    if (deliveryIdsAvailable)
    {
        ismEngine_internal_RestartSession (pThreadData, pSession, false);
    }

    // We possibly need to clean up the nodePage which contained this node
    // so do this now
    if (pageCleanup)
    {
        ieiq_cleanupHeadPage(pThreadData, Q);
    }

    // If we've just fallen below the maximum number of messages being
    // delivered, we need to call checkWaiters
    if (oldInflightDeqs == Q->maxInflightDeqs)
    {
        (void)ieiq_checkWaiters(pThreadData, (ismEngine_Queue_t *)Q, NULL, NULL);
    }
    else if (oldInflightDeqs == 1)
    {
        //We removed the last inflight message....if we are a "zombie" queue, say we're done
        if (Q->redeliverOnly)
        {
            iecs_completedInflightMsgs(pThreadData, pSession->pClient,(ismQHandle_t)Q );
        }
        //Inflight messages count 1 towards the predeletecount
        ieiq_reducePreDeleteCount(pThreadData, (ismQHandle_t)Q);
    }
}
static void  ieiq_completeReceiveAck( ieutThreadData_t *pThreadData
                                    , ieiqQNode_t *pnode)
{
    ieutTRACEL(pThreadData, pnode, ENGINE_FNC_TRACE, FUNCTION_IDENT "pnode=%p deliveryId=%u\n", __func__,
                                                                  pnode, pnode->deliveryId);
    pnode->msgState = ismMESSAGE_STATE_RECEIVED;
}
///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Acknowledge delivery of a message
///  @remarks
///    This function is called by the consumer to acknowledge that they have
///    received a QoS 1 messages and the message can be deleted from the
///    queue.
///
///  @param[in] Qhdl               - Queue
///  @param[in] pSession           - Session
///  @param[in] pDelivery          - Delivery handle
///  @param[in] options            - Options
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ieiq_acknowledge( ieutThreadData_t *pThreadData
                        , ismQHandle_t Qhdl
                        , ismEngine_Session_t *pSession
                        , ismEngine_Transaction_t *pTran
                        , void *pDelivery
                        , uint32_t options
                        , ismEngine_AsyncData_t *asyncInfo)
{
    uint32_t rc = OK;
    uint8_t state = OK;
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    ieiqQNode_t *pnode = (ieiqQNode_t *)pDelivery;

    ieutTRACEL(pThreadData, pSession, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p pDelivery=%p acknowledgeOptions=%d\n", __func__,
               Q, pDelivery, options);

    //IntermediateQ does not do transactional acknowledges
    assert(pTran == NULL);

    if (options == ismENGINE_CONFIRM_OPTION_EXPIRED)
    {
        //This flag means that the client has expired one of the messages
        //in flight to it (and that *this* message should be consumed. The client
        //cannot guarantee exactly which message (just which destination). So we
        //update the counts of messages expired but count of messages with expiry
        //are adjusted based looking at actual expiry in headers of this message
        __sync_fetch_and_add(&(Q->expiredMsgs), 1);
        pThreadData->stats.expiredMsgCount++;
        options = ismENGINE_CONFIRM_OPTION_CONSUMED;
    }

    if (options == ismENGINE_CONFIRM_OPTION_CONSUMED)
    {
        if ((pnode->msgState != ismMESSAGE_STATE_DELIVERED) &&
            (pnode->msgState != ismMESSAGE_STATE_RECEIVED))
        {
            // If we are acknowledging the consuming of a messages which
            // is not in DELIVERED or RECEIVED state then we have a breakdown
            // in the engine protocol. Bring the server down with as much
            // diagnostics as possible.
            ieutTRACE_FFDC( ieutPROBE_001, true
                    , "Invalid msgState when ack-confirming message", rc
                    , "msgState", &pnode->msgState, sizeof(ismMessageState_t)
                    , "OrderId", &pnode->orderId, sizeof(uint64_t)
                    , "Node", pnode, sizeof(ieiqQNode_t)
                    , "Queue", Q, sizeof(ieiqQueue_t)
                    , NULL );
        }

        if (Q->hMsgDelInfo == NULL)
        {
            // If we don't yet have the delivery information handle, get it now
            rc = iecs_acquireMessageDeliveryInfoReference(pThreadData, pSession->pClient, &Q->hMsgDelInfo);
            if (rc != OK)
            {
                goto mod_exit;
            }
        }

        bool pageCleanup = false;
        bool deliveryIdsAvailable = false;

        if (asyncInfo != NULL)
        {
            assert(asyncInfo->numEntriesUsed < asyncInfo->numEntriesAllocated);
            assert(asyncInfo->fOnStack);
            ieiqConsumeMessageAsyncData_t consumeAsyncData = {IEIQ_CONSUMEMESSAGE_ASYNCDATA_STRUCID, Q, pnode, pSession, false};
            ismEngine_AsyncDataEntry_t newEntry =
                 {ismENGINE_ASYNCDATAENTRY_STRUCID, ieiqQueueConsume, &consumeAsyncData, sizeof(consumeAsyncData), NULL, {.internalFn = ieiq_consumeAckCommitted}};
            asyncInfo->entries[asyncInfo->numEntriesUsed] = newEntry;
            asyncInfo->numEntriesUsed++;

            rc = ieiq_consumeMessage( pThreadData, Q, pnode
                                    , &pageCleanup, &consumeAsyncData.deliveryIdsAvailable, asyncInfo);

            if (rc == OK)
            {
                deliveryIdsAvailable = consumeAsyncData.deliveryIdsAvailable;
            }
        }
        else
        {
            rc = ieiq_consumeMessage( pThreadData, Q, pnode
                                    , &pageCleanup, &deliveryIdsAvailable, NULL);
        }

        if (rc == OK)
        {
            ieiq_completeConsumeAck ( pThreadData
                                    , Q
                                    , pSession
                                    , pageCleanup
                                    , deliveryIdsAvailable);
        }
        else
        {
            assert (rc == ISMRC_AsyncCompletion);
        }
    }
    else if (options == ismENGINE_CONFIRM_OPTION_RECEIVED)
    {
        // Message received QOS 2 - The client has acknowledged that it has
        // received the message mark it as received. On return from this call
        // the protocol will call the client to release the message.

        if (pnode->msgState != ismMESSAGE_STATE_DELIVERED)
        {
            // In certain circumstances, the protocol layer will allow a second
            // receive ack through to the engine.  (PMR 90255,000,858).  In these
            // cases, we will just swallow the ack.
            if (pnode->msgState == ismMESSAGE_STATE_RECEIVED) {
                ieutTRACEL(pThreadData, options, ENGINE_INTERESTING_TRACE, "Message in RECEIVED state was acknowledged again.\n");
                goto mod_exit;
            }
            // If we are acknowledging the receipt of a message which
            // is not in DELIVERED state then we have a breakdown in the
            // engine protocol. Bring the server down with as much
            // diagnostics as possible.
            ieutTRACE_FFDC( ieutPROBE_005, true
                    , "Invalid msgState when ack-received message", rc
                    , "msgState", &pnode->msgState, sizeof(ismMessageState_t)
                    , "OrderId", &pnode->orderId, sizeof(uint64_t)
                    , "Node", pnode, sizeof(ieiqQNode_t)
                    , "Queue", Q, sizeof(ieiqQueue_t)
                    , NULL );
        }

        if (pnode->inStore)
        {
            // If the message is in the store, we need to update the store too
            state = (pnode->deliveryCount < 0x3f)?pnode->deliveryCount:0x3f;
            state |= (ismMESSAGE_STATE_RECEIVED << 6);

            iest_AssertStoreCommitAllowed(pThreadData);

            if (asyncInfo != NULL)
            {
                assert(asyncInfo->numEntriesUsed < asyncInfo->numEntriesAllocated);
                assert(asyncInfo->fOnStack);

                ismEngine_AsyncDataEntry_t newEntry =
                     {ismENGINE_ASYNCDATAENTRY_STRUCID, ieiqQueueMsgReceived, NULL, 0, pnode, {.internalFn = ieiq_receiveAckCommitted}};
                asyncInfo->entries[asyncInfo->numEntriesUsed] = newEntry;
                asyncInfo->numEntriesUsed++;

                rc = ism_store_updateReference( pThreadData->hStream
                                              , Q->QueueRefContext
                                              , pnode->hMsgRef
                                              , pnode->orderId
                                              , state
                                              , 0);
                if (rc != OK)
                {
                    // The failure to update a store reference means that the
                    // queue is inconsistent.
                    ieutTRACE_FFDC( ieutPROBE_009, true
                                  , "ism_store_updateReference failed.", rc
                                  , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                                  , "Queue", Q, sizeof(ieiqQueue_t)
                                  , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                                  , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                                  , "pNode", pnode, sizeof(ieiqQNode_t)
                                  , NULL );
                }

                rc = iead_store_asyncCommit(pThreadData, false, asyncInfo);
            }
            else
            {
                // and commit the store transaction.
                rc = iest_store_updateReferenceCommit( pThreadData
                                                     , pThreadData->hStream
                                                     , Q->QueueRefContext
                                                     , pnode->hMsgRef
                                                     , pnode->orderId
                                                     , state
                                                     , 0);
                if (rc != OK)
                {
                    // The failure to update a store reference means that the
                    // queue is inconsistent.
                    ieutTRACE_FFDC( ieutPROBE_006, true
                                  , "ism_store_updateReferenceCommit failed.", rc
                                  , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                                  , "Queue", Q, sizeof(ieiqQueue_t)
                                  , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                                  , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                                  , "pNode", pnode, sizeof(ieiqQNode_t)
                                  , NULL );
                }
            }
        }
        assert (rc == OK || rc == ISMRC_AsyncCompletion);

        if (rc == OK)
        {
            ieiq_completeReceiveAck(pThreadData, pnode);
        }
    }
    else if ( (options == ismENGINE_CONFIRM_OPTION_NOT_DELIVERED ) ||
              (options == ismENGINE_CONFIRM_OPTION_NOT_RECEIVED ) )
    {
        if (pnode->msgState != ismMESSAGE_STATE_DELIVERED)
        {
            // If we are nack-ing the receipt of a message which is not
            // in DELIVERED state then we have a breakdown in the engine
            // protocol. Bring the server down with as much diagnostics
            // as possible.
            ieutTRACE_FFDC( ieutPROBE_007, true
                    , "Invalid msgState when nack-ing message", rc
                    , "msgState", &pnode->msgState, sizeof(ismMessageState_t)
                    , "OrderId", &pnode->orderId, sizeof(uint64_t)
                    , "Options", &options, sizeof(uint32_t)
                    , "Node", pnode, sizeof(ieiqQNode_t)
                    , "Queue", Q, sizeof(ieiqQueue_t)
                    , NULL );
        }

        // We are about to mark the message as available for redelivery,
        // but if we can redeliver it now then we can avoid having to
        // make any changes.

        rc = ieiq_redeliverMessage( pThreadData
                                  , Q
                                  , pnode
                                  , (options == ismENGINE_CONFIRM_OPTION_NOT_RECEIVED ) );
        if (rc == ISMRC_NotDelivered)
        {
            rc = OK; // ISMRC_NotDelivered is for status only

            // We failed to directly redeliver the message, so instead
            // re-mark the message as available and reset the cursor
            // so the next call to checkWaiters will re-deliver it

            // Decrement the delivery count if the protocol layer failed
            // to even send the message. We can do this without a lock
            // as no-one else will updating this value.
            if (options == ismENGINE_CONFIRM_OPTION_NOT_DELIVERED)
            {
                pnode->deliveryCount--;

                // If the message is in the store, we need rollback the
                // deliveryCount in the store too
                if (pnode->inStore)
                {
                    uint32_t msgState = (pnode->deliveryCount == 0)?ismMESSAGE_STATE_AVAILABLE
                                                                  :ismMESSAGE_STATE_RECEIVED;

                    state = pnode->deliveryCount & 0x3f;
                    state |= (msgState << 6);

                    iest_AssertStoreCommitAllowed(pThreadData);

                    rc = iest_store_updateReferenceCommit( pThreadData
                                                         , pThreadData->hStream
                                                         , Q->QueueRefContext
                                                         , pnode->hMsgRef
                                                         , pnode->orderId
                                                         , state
                                                         , 0);
                    if (rc != OK)
                    {
                        // The failure to update a store reference means that the
                        // queue is inconsistent.
                        ieutTRACE_FFDC( ieutPROBE_008, true
                                      , "ism_store_updateReference failed.", rc
                                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                                      , "Queue", Q, sizeof(ieiqQueue_t)
                                      , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                                      , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                                      , "pNode", pnode, sizeof(ieiqQNode_t)
                                      , NULL );
                    }
                }
            }

            // Because the consumer didn't receive the message, request that
            // the getCursor is reset. We could be more efficient here in that
            // rather than just requesting the scan starts from the head of the
            // queue we could ask that the scan start just from the last
            // node, but for the moment we stick with rescanning the from
            // the head.
            pnode->msgState = ismMESSAGE_STATE_AVAILABLE;

            uint64_t oldinflightdeqs = __sync_fetch_and_sub(&(Q->inflightDeqs), 1);
            assert(oldinflightdeqs > 0);

            // We need a WriteMemoryBarriet to ensure pnode->msgState
            // is visible before Q->resetCursor, this _sync call will do
            // nicely.
            Q->resetCursor = true;

            //If it has expiry, update the expiry tables
            if (pnode->msg->Header.Expiry != 0)
            {
                iemeBufferedMsgExpiryDetails_t msgExpiryData = { pnode->orderId , pnode, pnode->msg->Header.Expiry };
                ieme_addMessageExpiryData( pThreadData, (ismEngine_Queue_t *)Q,  &msgExpiryData);
            }

            // Because a message is now available to be sent we need to call
            // checkWaiters to attempt re-delivery.
            (void)ieiq_checkWaiters(pThreadData, (ismEngine_Queue_t *)Q, NULL, NULL);

            if (oldinflightdeqs == 1)
            {
                //We removed the last inflight message....if we are a "zombie" queue, say we're done
                if (Q->redeliverOnly)
                {
                    iecs_completedInflightMsgs(pThreadData, pSession->pClient,(ismQHandle_t)Q );
                }

                //We removed the last inflight message. Inflight messages count 1 towards the predeletecount
                ieiq_reducePreDeleteCount(pThreadData, (ismQHandle_t)Q);
            }
        }
    }
    else
    {
        ieutTRACEL(pThreadData, options, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: calling with invalid options (%d)\n", __func__, options);
        assert(false);
    }

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Drain messages from a queue
///  @remark
///    This function is used to delete all messages from the queue.
///
///  @param[in] Qhdl               - Queue
///  @return                       - OK
///////////////////////////////////////////////////////////////////////////////
int32_t ieiq_drainQ(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl)
{
#define DRAIN_BATCH_SIZE 1000
    int32_t rc = OK;
    uint32_t commitBatchSize = 0;
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    ieiqQNodePage_t *firstPage;
    iewsWaiterStatus_t oldStatus, newStatus;
    bool getLocked = false;
    bool putLocked = false;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY " *Q=%p\n", __func__, Q);

    // First attempt to lock the get lock.
    do
    {
        oldStatus = Q->waiterStatus;
        if ((oldStatus == IEWS_WAITERSTATUS_ENABLED) ||
            (oldStatus == IEWS_WAITERSTATUS_DISABLED) ||
            (oldStatus == IEWS_WAITERSTATUS_DISCONNECTED))
        {
            getLocked = __sync_bool_compare_and_swap( &(Q->waiterStatus)
                                                    , oldStatus
                                                    , IEWS_WAITERSTATUS_DELIVERING);
        }
    } while (!getLocked && ((oldStatus == IEWS_WAITERSTATUS_ENABLED) ||
                            (oldStatus == IEWS_WAITERSTATUS_DISABLED) ||
                            (oldStatus == IEWS_WAITERSTATUS_DISCONNECTED)));

    if (!getLocked)
    {
        ieutTRACEL(pThreadData, oldStatus, ENGINE_WORRYING_TRACE,
                   "DrainQ failed to get get lock. current status %ld\n", oldStatus);
        rc = ISMRC_DestinationInUse;
        goto mod_exit;
    }

    // Next get the put lock. This is the only place in the code where
    // have both the get and put lock's.
    ieiq_getPutLock(Q);
    putLocked = true;

    // Next scan the queue to see if we have any messages which have been
    // delivered, but not yet consumed.
    ieiqQNode_t *msgCursor = Q->head;

    while (msgCursor != NULL)
    {
        if (msgCursor->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
        {
            ieiqQNodePage_t *currpage=(ieiqQNodePage_t *)(msgCursor->msg);
            ismEngine_CheckStructId(currpage->StrucId, IEIQ_PAGE_STRUCID, ieutPROBE_026);

            if (currpage->nextStatus < completed)
                msgCursor = NULL;
            else
            {
                ismEngine_CheckStructId(currpage->next->StrucId, IEIQ_PAGE_STRUCID, ieutPROBE_027);
                msgCursor = &(currpage->next->nodes[0]);
            }
        }
        else
        {
            if ((msgCursor->msgState != ismMESSAGE_STATE_AVAILABLE) &&
                (msgCursor->msg != NULL))
            {
                // We have a message which we cannot discard, so we can-not
                // drain the queue at the current time.

                ieutTRACEL(pThreadData, msgCursor->msg, ENGINE_WORRYING_TRACE,
                           "Message found with outstanding acknowlegde.\n");
                rc = ISMRC_DestinationInUse;
                goto mod_exit;
            }

            msgCursor++;
        }
    }

    // Having reached here, we can be sure that we can clear the queue.
    // First purge all of the message references from the store.
#if 0
    rc = ism_store_clearReferenceContext( pThreadData->hStream
                                        , Q->QueueRefContext );
    if (rc != OK)
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                      , "ism_store_clearReferenceContext as part of DrainQ failed.", rc
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(ieiqQueue_t)
                      , NULL );
    }

    // And commit the clear operation.
    iest_store_commit( pThreadData, false );

#else
    msgCursor = Q->head;
    uint32_t updateCount = 0;
    while (msgCursor != NULL)
    {
        if (msgCursor->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
        {
            ieiqQNodePage_t *currpage=(ieiqQNodePage_t *)(msgCursor->msg);
            ismEngine_CheckStructId(currpage->StrucId, IEIQ_PAGE_STRUCID, ieutPROBE_028);

            if (currpage->nextStatus < completed)
                msgCursor = NULL;
            else
                msgCursor = &(currpage->next->nodes[0]);
        }
        else
        {
            if ((msgCursor->msgState == ismMESSAGE_STATE_AVAILABLE) &&
                (msgCursor->msg != NULL))
            {
                ieutTRACEL(pThreadData, msgCursor->orderId, ENGINE_HIFREQ_FNC_TRACE, "curs %p, oId %lu, msg %p\n",
                                   msgCursor, msgCursor->orderId, msgCursor->msg);

                rc = ism_store_deleteReference( pThreadData->hStream
                                              , Q->QueueRefContext
                                              , msgCursor->hMsgRef
                                              , msgCursor->orderId
                                              , 0 );
                if (rc != OK)
                {
                    ieutTRACE_FFDC( ieutPROBE_002, true
                                  , "Failed to delete msg reference", rc
                                  , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                                  , "Queue", Q, sizeof(ieiqQueue_t)
                                  , "OrderId", &(msgCursor->orderId), sizeof(uint64_t)
                                  , "OrderId", &(msgCursor->hMsgRef), sizeof(ismStore_Handle_t)
                                  , NULL );
                }
                updateCount++;
            }
            else if ((msgCursor->msgState != ismMESSAGE_STATE_CONSUMED) &&
                     (msgCursor->msg != NULL))
            {
                // Eek we have found an un-ack'd message. Abort!
                ieutTRACE_FFDC( ieutPROBE_003, true
                              , "Unack'd message found after queue was checked", ISMRC_DestinationInUse
                              , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                              , "Queue", Q, sizeof(ieiqQueue_t)
                              , NULL );
            }

            msgCursor++;
        }

        // In an attempt to avoid an enormous store transaction, periodically
        // call store_commit
        if (updateCount == 10000)
        {
            iest_store_commit( pThreadData, false );
            updateCount = 0;
        }
    }
    if (updateCount)
    {
        iest_store_commit( pThreadData, false );
    }

#endif

    // From this point forwards there is now way back. We must leave the
    // queue in a consistent state

    while (Q->headPage != NULL)
    {
        ieiqQNodePage_t *pageToFree = NULL;
        ieiqQNode_t *temp = Q->head;

        //Move the head to the next item..
        (Q->head)++;

        //Have we gone off the bottom of the page?
        if(Q->head->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
        {
            pageToFree = Q->headPage;
            ieiqQNodePage_t *nextPage = pageToFree->next;

            if( nextPage != NULL)
            {
                Q->headPage = nextPage;
                Q->head = &(nextPage->nodes[0]);
            }
            else
            {
                Q->headPage = NULL;
                Q->head = NULL;
            }
        }

        // We will discard any messages which are currently unconsumed. Any
        // delivered, but unacknowledged messages we will ignore.
        if (temp->msg != NULL)
        {
            if (temp->msgState == ismMESSAGE_STATE_AVAILABLE)
            {
                iest_unstoreMessage( pThreadData, temp->msg, false, false, NULL,  &commitBatchSize );

                iem_releaseMessage(pThreadData, temp->msg);
                temp->msg = MESSAGE_STATUS_EMPTY;

                if (commitBatchSize >= DRAIN_BATCH_SIZE)
                {
                    // We commit in batches even though the store should be
                    // able to handle large transactions there is no value
                    // in pushing too hard here.
                    iest_store_commit( pThreadData, false );
                    commitBatchSize = 0;
                }
            }
        }

        if(pageToFree != NULL)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_freeStruct(pThreadData, resourceSet, iemem_intermediateQPage, pageToFree, pageToFree->StrucId);
        }
    }

    // And commit the store object
    if (commitBatchSize)
    {
        iest_store_commit(pThreadData, false);
    }

    // Having reached this point here, the queue should be empty so now we
    // reinitialise
    iere_primeThreadCache(pThreadData, resourceSet);
    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -(int64_t)(Q->bufferedMsgs));
    pThreadData->stats.bufferedMsgCount -= Q->bufferedMsgs;
    Q->bufferedMsgs = 0;

    //Allocate a page of queue nodes
    firstPage = ieiq_createNewPage(pThreadData, Q, ieiqPAGESIZE_SMALL, 0);

    if (firstPage == NULL)
    {
        // We failed to allocate a page for the message. never mind we just
        // just set the page pointers to NULL and cross our legs.
        Q->cursor     = NULL;
        Q->headPage = NULL;
        Q->head     = NULL;
        Q->tail     = NULL;

        ieutTRACEL(pThreadData, ieiqPAGESIZE_SMALL, ENGINE_ERROR_TRACE, "%s: ieiq_createNewPage(first) failed!\n",
                   __func__);
    }
    else
    {
        Q->cursor     = &(firstPage->nodes[0]);
        Q->headPage = firstPage;
        Q->head     = &(firstPage->nodes[0]);
        Q->tail     = &(firstPage->nodes[0]);

        firstPage->nextStatus = failed;
    }

mod_exit:
    // And finally
    if (putLocked)
    {
        ieiq_releasePutLock(Q);
    }

    if (getLocked)
    {
        newStatus = __sync_val_compare_and_swap( &(Q->waiterStatus)
                                               , IEWS_WAITERSTATUS_DELIVERING
                                               , oldStatus);

        // If the waiter state was changed while we were
        if (newStatus != IEWS_WAITERSTATUS_DELIVERING)
        {
            ieq_completeWaiterActions( pThreadData
                                     , (ismEngine_Queue_t *)Q
                                     , Q->pConsumer
                                     , IEQ_COMPLETEWAITERACTION_OPT_NODELIVER //going to call checkWaiters
                                     , true);
        }
        ieiq_checkWaiters(pThreadData, (ismEngine_Queue_t *)Q, NULL, NULL);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
///  @brief
///    Query queue statistics from a queue
///  @remarks
///    Returns a structure ismEngine_QueueStatistics_t which contains information
///    about the current state of the queue.
///
///  @param[in] Qhdl              Queue
///  @param[out] stats            Statistics data
///////////////////////////////////////////////////////////////////////////////
void ieiq_getStats(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl, ismEngine_QueueStatistics_t *stats)
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;

    stats->BufferedMsgs = Q->bufferedMsgs;
    stats->BufferedMsgsHWM = Q->bufferedMsgsHWM;
    //Once we've added ways of querying discarded separately from rejected, we should
    //Not do the following addition:
    stats->RejectedMsgs = Q->rejectedMsgs;
    stats->DiscardedMsgs = Q->discardedMsgs;
    stats->ExpiredMsgs = Q->expiredMsgs;
    stats->InflightEnqs = Q->inflightEnqs;
    stats->InflightDeqs = Q->inflightDeqs;
    stats->EnqueueCount = Q->enqueueCount;
    stats->DequeueCount = Q->dequeueCount;
    stats->QAvoidCount = Q->qavoidCount;
    stats->MaxMessages = Q->Common.PolicyInfo->maxMessageCount;
    stats->PutsAttempted = Q->putsAttempted;
    stats->MaxMessageBytes = Q->Common.PolicyInfo->maxMessageBytes;
    assert(stats->MaxMessageBytes == 0); // No code to deal with maxMessageBytes
    stats->BufferedMsgBytes = 0;
    // Calculate some additional statistics
    stats->ProducedMsgs = stats->EnqueueCount+stats->QAvoidCount;
    stats->ConsumedMsgs = stats->DequeueCount+stats->QAvoidCount;
    if (stats->MaxMessages == 0)
    {
        stats->BufferedPercent = 0;
        stats->BufferedHWMPercent = 0;
    }
    else
    {
        stats->BufferedPercent = ((double)stats->BufferedMsgs * 100.0)/(double)stats->MaxMessages;
        stats->BufferedHWMPercent = ((double)stats->BufferedMsgsHWM * 100.0)/(double)stats->MaxMessages;
    }

    stats->PutsAttemptedDelta = (Q->qavoidCount + Q->enqueueCount + Q->rejectedMsgs) - Q->putsAttempted;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, "%s Q=%p msgs=%lu\n",__func__, Q, stats->BufferedMsgs);
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Explicitly set the value of the putsAttempted statistic
///
///  @param[in] Qhdl              Queue
///  @param[in] newPutsAttempted  Value to reset the stat to
///  @param[in] setType           The type of stat setting required
///////////////////////////////////////////////////////////////////////////////
void ieiq_setStats(ismQHandle_t Qhdl, ismEngine_QueueStatistics_t *stats, ieqSetStatsType_t setType)
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;

    if (setType == ieqSetStats_UPDATE_PUTSATTEMPTED)
    {
        // Only update the puts attempted stat if it is unchanged
        if (Q->putsAttempted == stats->PutsAttempted)
        {
            Q->putsAttempted += stats->PutsAttemptedDelta;
        }
    }
    else
    {
        assert(setType == ieqSetStats_RESET_ALL);

        Q->bufferedMsgsHWM = Q->bufferedMsgs;
        Q->rejectedMsgs = 0;
        Q->enqueueCount = 0;
        Q->dequeueCount = 0;
        Q->qavoidCount = 0;
        Q->putsAttempted = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Determine if there  are messages available to be delivered to /this/ consumer.
///  @param[in]   ismQHandle_t             Queue
///  @param[in]   ismEngine_Consumer_t     pConsumer
///
int32_t ieiq_checkAvailableMsgs(ismQHandle_t Qhdl,
                                ismEngine_Consumer_t *pConsumer)
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    uint64_t  inflight = Q->inflightDeqs + Q->inflightEnqs ;
    uint64_t  buffered = Q->bufferedMsgs;
    int32_t rc;

    //This type of queue only has one consumer... so they had better be asking about that one!
    assert(Q->pConsumer == pConsumer);

    if (buffered > inflight)
    {
        //There are messages available
        rc = OK;
    }
    else
    {
       rc = ISMRC_NoMsgAvail;
    }

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Write descriptions of ieiq structures to the dump
///
///  @param[in]     dumpHdl  Dump file handle
///////////////////////////////////////////////////////////////////////////////
void ieiq_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    iedm_describe_ieiqQueue_t(dump->fp);
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     During a dump we need to lock the consumer to ensure the pages we
/// are dumping aren't freed from under us.
///
/// @remark We lock the consumer in delivering state which means other threads
/// shouldn't wait for us but as a consequence we must call checkwaiters after
/// unlock
///
///  @param[in]     Q
///  @param[out]    State of the waiter when we locked it or gave up trying to
///////////////////////////////////////////////////////////////////////////////
bool ieiq_lockConsumerForDump(ieiqQueue_t *Q,
                              iewsWaiterStatus_t *pOldState)
{
    //try to lock the consumer once before we worry about time stamps etc...
    iewsWaiterStatus_t oldState = Q->waiterStatus;
    bool lockedConsumer = false;

    if ((oldState & IEWS_WAITERSTATUSMASK_LOCKED) == 0)
    {
        lockedConsumer = __sync_bool_compare_and_swap( &(Q->waiterStatus)
                                                     , oldState
                                                     , IEWS_WAITERSTATUS_DELIVERING); //Lock the consumer and delivering means we promise to call check waiters....
    }

    //Initial lock didn't work... loop around trying to lock for a while
    if (!lockedConsumer)
    {
        uint64_t starttime = ism_common_currentTimeNanos();
        uint64_t finishtime = 0;
        do
        {
            if ((oldState & IEWS_WAITERSTATUSMASK_LOCKED) == 0)
            {
                lockedConsumer = __sync_bool_compare_and_swap( &(Q->waiterStatus)
                                                               , oldState
                                                               , IEWS_WAITERSTATUS_DELIVERING); //Lock the consumer and delivering means we promise to call check waiters....
            }
            finishtime = ism_common_currentTimeNanos();
        }
        while (   !lockedConsumer
               && (finishtime >= starttime) //system time hasn't changed
               && ((finishtime - starttime) < (3 * 1000000000L))); //and we haven't been trying for 3 seconds
    }

    *pOldState = oldState;

    return lockedConsumer;
}



///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Queue Dump
///  @remark
///     This function dumps information about the supplied Queue. The
///     information is written using the supplied dumpHandle, dumpHdl.
///
///  @param[in]     Qhdl     Queue handle
///  @param[in]     dumpHdl  Dump file handle
///////////////////////////////////////////////////////////////////////////////
void ieiq_dumpQ( ieutThreadData_t *pThreadData
               , ismQHandle_t  Qhdl
               , iedmDumpHandle_t dumpHdl)
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    // Only dump this queue if it is valid, and not repeated in this stack
    if (iedm_dumpStartObject(dump, Q) == true)
    {
        iedm_dumpStartGroup(dump, "IntermediateQ");

        // Dump the QName, PolicyInfo and Queue
        if (Q->Common.QName != NULL)
        {
            iedm_dumpData(dump, "QName", Q->Common.QName,
                          iere_usable_size(iemem_intermediateQ, Q->Common.QName));
        }
        iepi_dumpPolicyInfo(Q->Common.PolicyInfo, dumpHdl);
        iedm_dumpData(dump, "ieiqQueue_t", Q,
                      iere_usable_size(iemem_intermediateQ, Q));

        int32_t detailLevel = dump->detailLevel;

        if (detailLevel >= iedmDUMP_DETAIL_LEVEL_7)
        {
            //Lock the consumer so we can walk the pages safely
            iewsWaiterStatus_t previousState = IEWS_WAITERSTATUS_DISCONNECTED;

            bool lockedConsumer = ieiq_lockConsumerForDump(Q, &previousState);

            if (lockedConsumer)
            {
                // Dump the consumer
                dumpConsumer(pThreadData, Q->pConsumer, dumpHdl);

                //Dump out the state of the consumer before we locked it
                iedm_dumpData(dump, "iewsWaiterStatus_t", &previousState,   sizeof(previousState));

                // Dump a subset of pages
                ieiqQNodePage_t *pDisplayPageStart[2];
                int32_t maxPages, pageNum;

                // Detail level less than 9, we only dump a set of pages at the head and tail
                if (detailLevel < iedmDUMP_DETAIL_LEVEL_9)
                {
                    pageNum = 0;
                    maxPages = 3;

                    pDisplayPageStart[0] = Q->headPage;
                    pDisplayPageStart[1] = Q->headPage;

                    // Work out which nodes we consider to be the start of the head/tail
                    for (ieiqQNodePage_t *pPage = pDisplayPageStart[0]; pPage != NULL; pPage = pPage->next)
                    {
                        if (++pageNum > maxPages) pDisplayPageStart[1] = pDisplayPageStart[1]->next;
                    }
                }
                else
                {
                    maxPages = 0;
                    pDisplayPageStart[0] = Q->headPage;
                    pDisplayPageStart[1] = NULL;
                }

                // Go through the lists of pages dumping maxPages of each
                for(int32_t i=0; i<2; i++)
                {
                    ieiqQNodePage_t *pDumpPage = pDisplayPageStart[i];

                    pageNum = 0;

                    while(pDumpPage != NULL)
                    {
                        pageNum++;

                        // Actually dump the page
                        iedm_dumpData(dump, "ieiqQNodePage_t", pDumpPage,
                                      iere_usable_size(iemem_intermediateQPage, pDumpPage));

                        // If user data is requested, run through the nodes dumping it
                        if (Q->bufferedMsgs != 0 && dump->userDataBytes != 0)
                        {
                            iedm_dumpStartGroup(dump, "Available Msgs");

                            uint32_t nodesInPage = pDumpPage->nodesInPage;

                            for(uint32_t nodeNum = 0; nodeNum < nodesInPage; nodeNum++)
                            {
                                ieiqQNode_t *node = &(pDumpPage->nodes[nodeNum]);

                                // We can only dump the available messages since if the node is in
                                // any other state, the msg pointer can be removed at any time.
                                if ((node->msg != NULL) && (node->msgState == ismMESSAGE_STATE_AVAILABLE))
                                {
                                    iem_dumpMessage(node->msg, dumpHdl);
                                }
                            }

                            iedm_dumpEndGroup(dump);
                        }

                        // If the page we've just dumped is in the tail move the tail.
                        if (i == 0 && pDumpPage == pDisplayPageStart[1])
                        {
                            pDisplayPageStart[1] = pDisplayPageStart[1]->next;
                        }

                        if (pageNum == maxPages) break;

                        pDumpPage = pDumpPage->next;
                    }
                }

                //Finally unlock the consumer and then call checkwaiters (as we had the consumer locked in delivering)
                bool unlocked = __sync_bool_compare_and_swap( &(Q->waiterStatus)
                                                            , IEWS_WAITERSTATUS_DELIVERING
                                                            , previousState);

                //If we couldn't unlock it, it's because someone has flagged we need to something...
                if (!unlocked)
                {
                    ieq_completeWaiterActions(pThreadData,
                                              Qhdl,
                                              Q->pConsumer,
                                              IEQ_COMPLETEWAITERACTION_OPT_NODELIVER,
                                              (previousState == IEWS_WAITERSTATUS_ENABLED));
                }
                (void)ieiq_checkWaiters(pThreadData, Qhdl, NULL, NULL);
            }
            else
            {
                //Dump the state of the consumer that we couldn't lock....
                //Dump out the state of the consumer before we locked it
                iedm_dumpData(dump, "iewsWaiterStatus_t", &previousState,   sizeof(previousState));
            }
        }

        iedm_dumpEndGroup(dump);
        iedm_dumpEndObject(dump, Q);
    }
}

int32_t ieiq_completePutPostCommit(
        ieutThreadData_t               *pThreadData,
        int32_t                         retcode,
        ismEngine_AsyncData_t          *asyncInfo,
        ismEngine_AsyncDataEntry_t     *context)
{
    assert (retcode == OK);

    ieiqPutPostCommitInfo_t *pPutInfo = (ieiqPutPostCommitInfo_t *)(context->Data);

    assert(pPutInfo->Q != NULL);

    if (pPutInfo->deleteCountDecrement != 0)
    {
        ieiq_reducePreDeleteCount(pThreadData, (ismQHandle_t)(pPutInfo->Q));
        if (pPutInfo->deleteCountDecrement != 1)
        {
            ieiq_reducePreDeleteCount(pThreadData, (ismQHandle_t)(pPutInfo->Q));
        }
    }

    //We've finished, remove entry so if we go async we don't try to deliver this message again
    if (asyncInfo != NULL)
    {
        iead_popAsyncCallback( asyncInfo, context->DataLen);
    }

    return retcode;
}

static inline bool ieiq_scheduleWork( ieutThreadData_t *pThreadData
                                    , ieiqQueue_t *Q)
{
    bool scheduleWork;

    scheduleWork = __sync_bool_compare_and_swap(&(Q->postPutWorkScheduled),
                                                false,
                                                true);


    return scheduleWork;
}

static inline void ieiq_clearScheduledWork( ieutThreadData_t *pThreadData
                                          , ieiqQueue_t *Q)
{
    bool workCleared = false;


    workCleared = __sync_bool_compare_and_swap(&(Q->postPutWorkScheduled),
                                               true,
                                               false);

    if(UNLIKELY(!workCleared))
    {
        ieutTRACE_FFDC(ieutPROBE_003, false,
                "Tried to clear work and couldn't.", ISMRC_Error
                , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                , "Queue", Q, sizeof(ieiqQueue_t)
                , NULL);
    }
}
//Called from either post commit or jobcallback to discard messages, do deliveries etc.
static inline int32_t ieiq_postTranPutWork( ieutThreadData_t *pThreadData
                                          , ismEngine_Transaction_t *pTran
                                          , ietrAsyncTransactionData_t *asyncTran
                                          , ismEngine_AsyncData_t *pAsyncData
                                          , ietrReplayRecord_t *pRecord
                                          , ieiqQueue_t *Q)
{
    int rc = OK;
    iepiPolicyInfo_t *pPolicyInfo = Q->Common.PolicyInfo;

    //If our put (or someone else in the meantime) has filled the queue, see if we can get rid of expired
    //messages (and discard message if appropriate)...

    assert(pPolicyInfo->maxMessageBytes == 0); // No code to deal with maxMessageBytes

    if (Q->bufferedMsgs >= pPolicyInfo->maxMessageCount)
    {
        //Check if we can remove expired messages and get below the max...
        ieme_reapQExpiredMessages(pThreadData, (ismEngine_Queue_t *)Q);

        //Discard messages down to ensure there is "space" for the message we put
        if (   (pPolicyInfo->maxMsgBehavior == DiscardOldMessages)
            && (Q->bufferedMsgs > pPolicyInfo->maxMessageCount))
        {
            //We reclaim before checkwaiters so that checkwaiters
            //doesn't put some really old messages in flight forcing us to remove newer ones
            ieiq_reclaimSpace(pThreadData, (ismQHandle_t)Q, true);
        }
    }

    //Set up the data we'd need to continue if delivery goes async
    ieiqPutPostCommitInfo_t completePostCommitInfo = { IEIQ_PUTPOSTCOMMITINFO_STRUCID
                                                     , ((pTran->TranFlags & ietrTRAN_FLAG_REHYDRATED) != 0) ? 2 : 1
                                                     , Q };

    ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
                {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineTranCommit,  NULL,   0,     asyncTran, {.internalFn = ietr_asyncFinishParallelOperation }},
                {ismENGINE_ASYNCDATAENTRY_STRUCID, ieiqQueuePostCommit, &completePostCommitInfo, sizeof(completePostCommitInfo), NULL, {.internalFn = ieiq_completePutPostCommit } }
                };

    if (asyncTran != NULL)
    {
        bool usedLocalAsyncData;

#if 0
        //Adjust Tran so this SLE is not called again in this phase if we go async
        asyncTran->ProcessedPhaseSLEs++;
#endif

        ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID
                                          , (pTran->pSession!= NULL) ? pTran->pSession->pClient : NULL
                                          , IEAD_MAXARRAYENTRIES, 2, 0, true, 0, 0, asyncArray};

        if (pAsyncData == NULL)
        {
            usedLocalAsyncData = true;
            pAsyncData = &asyncData;
        }
        else
        {
            usedLocalAsyncData = false;
            iead_pushAsyncCallback(pThreadData, pAsyncData, &asyncArray[1]);
        }

        __sync_fetch_and_add(&asyncTran->parallelOperations, 1);
        __sync_fetch_and_add(&(Q->preDeleteCount), 1); // Cannot rely on subscription any more

        // Attempt to deliver the messages
        rc = ieiq_checkWaiters(pThreadData, (ismEngine_Queue_t *)Q, pAsyncData, NULL);
        assert(rc == OK || rc == ISMRC_AsyncCompletion);

        if (rc != ISMRC_AsyncCompletion)
        {
            DEBUG_ONLY uint64_t oldCount  = __sync_fetch_and_sub(&asyncTran->parallelOperations, 1);
            assert(oldCount != 1);

            if (usedLocalAsyncData == false)
            {
                iead_popAsyncCallback(pAsyncData, asyncArray[1].DataLen);
            }

#if 0
            //As we didn't go async we need to undo the pre-emptive marking of this SLE complete
            //As it'll be done by our caller
            asyncTran->ProcessedPhaseSLEs--;
#endif
        }
    }
    else
    {
        completePostCommitInfo.deleteCountDecrement--;
        (void) ieiq_checkWaiters(pThreadData, (ismEngine_Queue_t *)Q, NULL, NULL);
    }

    if (rc == OK)
    {
        ieiq_completePutPostCommit(pThreadData, rc, NULL, &asyncArray[1]);
    }

    // Something we've just done, either checking waiters or deleting the queue
    // may have performed a store commit - which means that there are no outstanding
    // store operations.
    //
    // If there were some prior to this call, get the real value from the store.
    if (pRecord->StoreOpCount > 0)
    {
        ism_store_getStreamOpsCount(pThreadData->hStream, &pRecord->StoreOpCount);
    }

    // Don't hold up the rest of the transaction for this - we've set up a parallel
    // transaction operation... In short, lie to the caller.
    if (rc == ISMRC_AsyncCompletion) rc = OK;

    return rc;
}
///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Replay Soft Log
///  @remark
///     This function executes a softlog records for Intermediate Queue
///     for either Commit or Rollback.
///
///  @param[in]     Phase            Commit/PostCommit/Rollback/PostRollback
///  @param[in]     pThreadData      Thread data
///  @param[in]     pTran            Transaction
///  @param[in]     pEntry           Softlog entry
///  @param[in]     pRecord          Transaction state record
///////////////////////////////////////////////////////////////////////////////

int32_t ieiq_SLEReplayPut( ietrReplayPhase_t Phase
                         , ieutThreadData_t *pThreadData
                         , ismEngine_Transaction_t *pTran
                         , void *pEntry
                         , ietrReplayRecord_t *pRecord
                         , ismEngine_AsyncData_t *pAsyncData
                         , ietrAsyncTransactionData_t *asyncTran
                         , ismEngine_DelivererContext_t *delivererContext)
{
    int32_t rc=OK;
    ieiqSLEPut_t *pSLE = (ieiqSLEPut_t *)pEntry;
    ieiqQueue_t *Q = pSLE->pQueue;
    ieiqQNode_t *pNode;
    ismEngine_Message_t *pMsg;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, pSLE, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Phase=%d pEntry=%p\n",
               __func__, Phase, pEntry);

    if (pSLE->bSavepointRolledback)
    {
        if (Phase == Commit)
        {
            Phase = Rollback;
        }
        else if (Phase == PostCommit)
        {
            Phase = PostRollback;
        }
        //NB: For MemoryCommit, there isn't a Rollback equivalent so we check inside the
        //    the MemoryCommit phase itself
    }

    switch (Phase)
    {
        case MemoryCommit:
            if (! pSLE->bSavepointRolledback)
            {
                // mark the message as available. (We can't do this until the
                // transaction object has been deleted from the store, which
                // happens as part of earlier, Commit phase).
                pNode = pSLE->pNode;

                assert(Q->inflightEnqs > 0);
                __sync_sub_and_fetch(&(Q->inflightEnqs), 1);
                __sync_add_and_fetch(&(Q->enqueueCount), 1);

                if (pSLE->pMsg->Header.Expiry != 0)
                {
                    //We're making available a message with expiry
                    iemeBufferedMsgExpiryDetails_t msgExpiryData = { pNode->orderId , pNode, pSLE->pMsg->Header.Expiry };
                    ieme_addMessageExpiryData( pThreadData, (ismEngine_Queue_t *)Q,  &msgExpiryData);
                }
                pNode->msg = pSLE->pMsg;
            }
            break;

        case PostCommit:
            if (pRecord->JobThreadId != ietrNO_JOB_THREAD)
            {
                if (ieiq_scheduleWork(pThreadData, Q))
                {
                    __sync_fetch_and_add(&Q->preDeleteCount, 1);
                    pRecord->JobRequired = true;
                }
            }
            else
            {
                rc = ieiq_postTranPutWork( pThreadData
                                         , pTran
                                         , asyncTran
                                         , pAsyncData
                                         , pRecord
                                         , Q);
            }
            break;

        case JobCallback:
            ieiq_clearScheduledWork(pThreadData, Q);

            rc = ieiq_postTranPutWork( pThreadData
                                     , pTran
                                     , asyncTran
                                     , pAsyncData
                                     , pRecord
                                     , Q);

           //Reduce the count that we increased when we scheduled this
           ieiq_reducePreDeleteCount(pThreadData, (ismQHandle_t)Q);
           break;

        case Rollback:
            pNode = pSLE->pNode;

            // An uncommitted message shouldn't have an MDR in
            // intermediate queue
            assert(!pNode->hasMDR);

            // Delete the reference from the queue to the msg (for store
            // transactions the rollback will do this for us and for Savepoint rollback, the savepoint is done under a store tran that is rolled back)
            if ((pNode->inStore) && !(pTran->fAsStoreTran) && !(pSLE->bSavepointRolledback))
            {
                if (pTran->fIncremental)
                {
                    ietr_deleteTranRef( pThreadData
                                      , pTran
                                      , &pSLE->TranRef );
                }

                ieutTRACEL(pThreadData, pNode->orderId, ENGINE_HIFREQ_FNC_TRACE, "pnode %p, oId %lu, msg %p\n",
                                   pNode, pNode->orderId, pNode->msg);

                rc = ism_store_deleteReference( pThreadData->hStream
                                              , Q->QueueRefContext
                                              , pNode->hMsgRef
                                              , pNode->orderId
                                              , 0 );
                if (rc != OK)
                {
                    ieutTRACE_FFDC( ieutPROBE_001, true
                                  , "ism_store_deleteReference(hMsgRef) failed."
                                  ,  rc
                                  , "Queue", Q, sizeof(ieiqQueue_t)
                                  , "SLE", pSLE, sizeof(ieiqSLEPut_t)
                                  , NULL );
                }

                pRecord->StoreOpCount++;
            }

            // Decide if we may need to do some head page cleanup
            pSLE->bPageCleanup = ((pNode+1)->msgState == ieqMESSAGE_STATE_END_OF_PAGE);

            pNode->msg = MESSAGE_STATUS_EMPTY;
            assert(pNode->msgState == ismMESSAGE_STATE_AVAILABLE);
            pNode->msgState = ismMESSAGE_STATE_CONSUMED;

            assert(Q->inflightEnqs > 0);
            __sync_sub_and_fetch(&(Q->inflightEnqs), 1);
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -1);
            pThreadData->stats.bufferedMsgCount--;
            DEBUG_ONLY int32_t oldDepth = __sync_fetch_and_sub(&(Q->bufferedMsgs),1);
            assert(oldDepth > 0);

            break;

        case PostRollback:
            //Now we have committed the removal of the reference, we can
            //decrement our usage of the message in the store and if necessary
            //remove the message
            //note that we can no longer refer to the node as it is marked consumed
            pMsg = pSLE->pMsg;

            if (pSLE->bInStore && !pSLE->bSavepointRolledback)
            {
                (void)iest_unstoreMessage( pThreadData, pMsg, pTran->fAsStoreTran, false, NULL, &pRecord->StoreOpCount );
            }
            iem_releaseMessage(pThreadData, pMsg);

            // Only need to process consumers or cleanup pages when not
            // rehydrating messages
            if ((Q->QOptions & ieqOptions_IN_RECOVERY) == 0)
            {
                // While this message does not need to be delivered, it may
                // have been blocking the delivery of other messages so attempt
                // to deliver blocked messages now.
                (void)ieiq_checkWaiters(pThreadData, (ismEngine_Queue_t *)Q, NULL, NULL);

                // Having effectively removed a message then we may
                // need to cleanup the page if it was the last
                // message on a page.
                // (Doing this every time may be overkill?)
                if (pSLE->bPageCleanup)
                {
                    ieiq_cleanupHeadPage(pThreadData, Q);
                }
            }

            // Delete the queue if this put was the thing preventing deletion
            // because it was a transacted put from a rehydrated transaction.
            if((pTran->TranFlags & ietrTRAN_FLAG_REHYDRATED) != 0)
            {
                ieiq_reducePreDeleteCount(pThreadData, (ismQHandle_t)Q);
            }

            // Something we've just done, either checking waiters or deleting the queue
            // may have performed a store commit - which means that there are no outstanding
            // store operations.
            //
            // If there were some prior to this call, get the real value from the store.
            if (pRecord->StoreOpCount > 0)
            {
                ism_store_getStreamOpsCount(pThreadData->hStream, &pRecord->StoreOpCount);
            }
            break;

        case SavepointRollback:
            pSLE->bSavepointRolledback = true;
            break;

        default:
             assert(false);
             break;
    }

    assert( rc == OK || rc == ISMRC_AsyncCompletion);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__,rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Return a handle to the store definition record for this queue
///  @remark
///     This function can be called to return a handle to the store record
///     representing this object's definition.
///
///  @return                       - Store record handle
///////////////////////////////////////////////////////////////////////////////
ismStore_Handle_t ieiq_getDefnHdl( ismQHandle_t Qhdl )
{
    return ((ieiqQueue_t *)Qhdl)->hStoreObj;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Return a handle to the store properties record for this queue
///  @remark
///     This function can be called to return a handle to the store record
///     representing this object's properties.
///
///  @return                       - Store record handle
///////////////////////////////////////////////////////////////////////////////
ismStore_Handle_t ieiq_getPropsHdl( ismQHandle_t Qhdl )
{
    return ((ieiqQueue_t *)Qhdl)->hStoreProps;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Update the QPR / SPR of a queue.
///  @remark
///     This function can be called to set a new queue properties record for
///     a queue.
///////////////////////////////////////////////////////////////////////////////
void ieiq_setPropsHdl( ismQHandle_t Qhdl,
                       ismStore_Handle_t propsHdl )
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;

    if (propsHdl != ismSTORE_NULL_HANDLE)
    {
        Q->hStoreProps = propsHdl;
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Update the properties of a queue including update of the QPR if required.
///  @remark
///     This function can be called to set new properties of a queue, if a store
///     handle is specified (e.g. during recovery) or if an update requires a
///     change to the store, the queue's store handle is updated.
///
///  @return                       - OK
///////////////////////////////////////////////////////////////////////////////
int32_t ieiq_updateProperties( ieutThreadData_t *pThreadData,
                               ismQHandle_t Qhdl,
                               const char *pQName,
                               ieqOptions_t QOptions,
                               ismStore_Handle_t propsHdl,
                               iereResourceSetHandle_t resourceSet )
{
    int32_t rc = OK;
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    bool storeChange = false;
    iereResourceSetHandle_t currentResourceSet = Q->Common.resourceSet;

    // Deal with any updates to Queue name
    if (pQName != NULL)
    {
        if (NULL == Q->Common.QName || strcmp(pQName, Q->Common.QName) != 0)
        {
            iere_primeThreadCache(pThreadData, currentResourceSet);
            char *newQName = iere_realloc(pThreadData,
                                          currentResourceSet,
                                          IEMEM_PROBE(iemem_intermediateQ, 5),
                                          Q->Common.QName, strlen(pQName)+1);

            if (newQName == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            Q->Common.QName = newQName;

            strcpy(Q->Common.QName, pQName);

            // Can only make a store change for point-to-point queues
            storeChange = ((Q->QOptions & ieqOptions_PUBSUB_QUEUE_MASK) == 0);
        }
    }
    else
    {
        if (Q->Common.QName != NULL)
        {
            iere_primeThreadCache(pThreadData, currentResourceSet);
            iere_free(pThreadData, currentResourceSet, iemem_intermediateQ, Q->Common.QName);
            Q->Common.QName = NULL;
            // Can only make a store change for point-to-point queues
            storeChange = ((Q->QOptions & ieqOptions_PUBSUB_QUEUE_MASK) == 0);
        }
    }

    // If we are still in recovery, we can update the queue options since they
    // are dealt with when recovery is completed - the queue options are not
    // stored.
    if (Q->QOptions & ieqOptions_IN_RECOVERY)
    {
        assert((QOptions & ieqOptions_IN_RECOVERY) != 0);
        Q->QOptions = QOptions;
    }

    // If the queue has changed, update the QPR in the store.
    if (storeChange && (Q->hStoreProps != ismSTORE_NULL_HANDLE))
    {
        // propsHdl should only be non-null during restart, and only the first
        // time this queue is discovered at restart.
        assert(propsHdl == ismSTORE_NULL_HANDLE);
        assert(pThreadData != NULL);

        rc = iest_updateQueue(pThreadData,
                              Q->hStoreObj,
                              Q->hStoreProps,
                              Q->Common.QName,
                              &propsHdl);

        if (rc != OK) goto mod_exit;

        assert(propsHdl != ismSTORE_NULL_HANDLE && propsHdl != Q->hStoreProps);
    }

    // Update the properties handle (this does nothing if it is ismSTORE_NULL_HANDLE)
    ieiq_setPropsHdl(Qhdl, propsHdl);

    // Update the resourceSet for the queue
    ieiq_updateResourceSet(pThreadData, Q, resourceSet);

mod_exit:

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Update the resource set for a queue that currently has none.
///////////////////////////////////////////////////////////////////////////////
static inline void ieiq_updateResourceSet(ieutThreadData_t *pThreadData,
                                          ieiqQueue_t *Q,
                                          iereResourceSetHandle_t resourceSet)
{
    if (resourceSet != iereNO_RESOURCE_SET)
    {
        assert(Q->Common.resourceSet == iereNO_RESOURCE_SET);

        Q->Common.resourceSet = resourceSet;

        iere_primeThreadCache(pThreadData, resourceSet);

        // Update memory for the Queue
        int64_t fullMemSize = (int64_t)iere_full_size(iemem_intermediateQ, Q);
        iere_updateMem(pThreadData, resourceSet, IEMEM_PROBE(iemem_intermediateQ, 6), Q, fullMemSize);

        if (Q->Common.QName != NULL)
        {
            fullMemSize = (int64_t)iere_full_size(iemem_intermediateQ, Q->Common.QName);
            iere_updateMem(pThreadData, resourceSet, IEMEM_PROBE(iemem_intermediateQ, 7), Q->Common.QName, fullMemSize);
        }

        assert(Q->PageMap != NULL);
        fullMemSize = (int64_t)iere_full_size(iemem_intermediateQ, Q->PageMap);
        iere_updateMem(pThreadData, resourceSet, IEMEM_PROBE(iemem_intermediateQ, 8), Q->PageMap, fullMemSize);

        for(int32_t pageNum=0; pageNum<Q->PageMap->PageCount; pageNum++)
        {
            ieiqQNodePage_t *pCurPage = Q->PageMap->PageArray[pageNum].pPage;

            // Update memory for the current page.
            fullMemSize = (int64_t)iere_full_size(iemem_intermediateQPage, pCurPage);
            iere_updateMem(pThreadData, resourceSet, IEMEM_PROBE(iemem_intermediateQPage, 2), pCurPage, fullMemSize);

            for(int32_t nodeNum=0; nodeNum<pCurPage->nodesInPage; nodeNum++)
            {
                ieiqQNode_t *pCurNode = &pCurPage->nodes[nodeNum];

                // Update rehydrated Messages (this will also update buffered msg counts)
                if (pCurNode->msg != NULL)
                {
                    iere_updateMessageResourceSet(pThreadData, resourceSet, pCurNode->msg, false, false);
                }
            }
        }

        // Update buffered msgs count
        iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, (int64_t)(Q->bufferedMsgs));
    }
}

/////////////////////////////////////////////////////////////////////////////////////
///  @brief Only inflight messages should be delivered from this queue (no new ones)
///
/// @remark
///MQTT messages have to complete their delivery once delivery has started.
///This function marks this queue so that only these "must be completed" messages
///are (re-)delivered from it.
///
/// @return true if there are messages that need to be completed, false otherwise
/////////////////////////////////////////////////////////////////////////////////////

bool ieiq_redeliverEssentialOnly(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl)
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    bool deliveriesNeedCompleting = false;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "q %p\n", __func__, Q);

    Q->redeliverOnly = true;

    //Ensure the redeliver flag is visible before we decide if this queue has messages in flight
    ismEngine_FullMemoryBarrier();

    if (Q->inflightDeqs > 0)
    {
        deliveriesNeedCompleting = true;
    }

    ieutTRACEL(pThreadData,deliveriesNeedCompleting,  ENGINE_FNC_TRACE, FUNCTION_EXIT "%d\n", __func__,deliveriesNeedCompleting);

    return deliveriesNeedCompleting;
}

/////////////////////////////////////////////////////////////////////////////////////
/// @brief Inflight messages from this queue are no longer required
///
/// @remark
/// The messages count will be reduced and if they were blocking deletion, the queue
/// will be deleted. Should only be called if no more messages will be delivered
/// from this queue (as the stats will be wrong if the queue is subsequently used)
///
/////////////////////////////////////////////////////////////////////////////////////
void ieiq_forgetInflightMsgs( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl )
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "q %p\n", __func__, Q);


    bool doneSet = false;
    uint64_t oldInflight;
    do
    {
        oldInflight = Q->inflightDeqs;
        doneSet = __sync_bool_compare_and_swap(&(Q->inflightDeqs), oldInflight, 0);
    }
    while (!doneSet);

    if (oldInflight > 0)
    {
        //We have reduced the inflight count.. having a non-zero count contributes to the use-count
        ieiq_reducePreDeleteCount(pThreadData, Qhdl);
    }

    ieutTRACEL(pThreadData, oldInflight, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Scan the queue looking for expired msgs to reap
///
///  @param[in] Q                - Q to scan
///  @param[in] nowExpire        - current time for expiry purposes
///  @param[in] forcefullscan    - don't trust per queue expiry data, always full scan
///  @param[in] expiryListLocked - If set, must NOT lock the waiter in a state
///                                that causes us to call checkwaiters and drive
///                                message delivery as that may need to remove Q from list
///
///  @remark expiryListLocked is so that (by setting it) we can avoid a deadlock:
///            reaper calls this function with expiry list lock held but checkwaiters
///            may deliver all expired messages and try and remove Q from list
///
///  @return                       - OK or ISMRC... inc:
///                                      LockNotGranted: couldn't get expiry lockl
///////////////////////////////////////////////////////////////////////////////
ieqExpiryReapRC_t  ieiq_reapExpiredMsgs( ieutThreadData_t *pThreadData
                                       , ismQHandle_t Qhdl
                                       , uint32_t nowExpire
                                       , bool forcefullscan
                                       , bool expiryListLocked)
{
    bool reapComplete=false;
    ieqExpiryReapRC_t rc = ieqExpiryReapRC_OK;
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    iewsWaiterStatus_t oldStatus = IEWS_WAITERSTATUS_DISCONNECTED;
    iewsWaiterStatus_t newStatus = IEWS_WAITERSTATUS_DISCONNECTED;
    iemeQueueExpiryData_t *pQExpiryData = (iemeQueueExpiryData_t *)Q->Common.QExpiryData;
    bool pageCleanupNeeded = false;
    bool deliveryIdsAvailable = false;

    ieutTRACEL(pThreadData,Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p \n", __func__,Q);

    bool gotWaiterLock = iews_tryLockForQOperation( &(Q->waiterStatus)
                                                  , &oldStatus
                                                  , &newStatus
                                                  , !expiryListLocked);

    if (!gotWaiterLock)
    {
        //We don't report lock not granted in this case, as someone else
        //was actively using the queue, we don't want to cause a new expiry scan
        //soon.
        //If we don't allow message delivery we could increase predeletecount of Q
        //then issue a return code asking to be redriven allowing it (and then remember
        //to decrease the predeletecount
        goto mod_exit;
    }

    bool gotExpiryLock = ieme_startReaperQExpiryScan( pThreadData, (ismEngine_Queue_t *)Q);

    if (!gotExpiryLock)
    {
        iews_unlockAfterQOperation( pThreadData
                                      , Qhdl
                                      , Q->pConsumer
                                      , &(Q->waiterStatus)
                                      , newStatus
                                      , oldStatus);

        rc = ieqExpiryReapRC_NoExpiryLock;
        goto mod_exit;
    }

    //We need the headlock as we are not just getting messages under the cursor!
    ieiq_getHeadLock(Q);

    if (!forcefullscan)
    {
        ieiqQNode_t *expiredNodes[NUM_EARLIEST_MESSAGES];
        uint32_t numExpiredNodes = 0;

        //Can we remove just the select messages in the array
        for (uint32_t i = 0; i < pQExpiryData->messagesInArray; i++)
        {
            if (pQExpiryData->earliestExpiryMessages[i].Expiry > nowExpire)
            {
                //We've found a message not due to expire yet... we've done all messages we need to
                if (i > 0)
                {
                    pQExpiryData->messagesInArray -= i;

                    memmove( &(pQExpiryData->earliestExpiryMessages[0])
                           , &(pQExpiryData->earliestExpiryMessages[i])
                           , pQExpiryData->messagesInArray * sizeof(iemeBufferedMsgExpiryDetails_t));
                }

                reapComplete = true;
                break;
            }
            else
            {
                //We need to remove this message
                ieiqQNode_t *qnode = (ieiqQNode_t *)pQExpiryData->earliestExpiryMessages[i].qnode;

                if (   (Q->head->orderId <= pQExpiryData->earliestExpiryMessages[i].orderId)
                    && (qnode->msgState == ismMESSAGE_STATE_AVAILABLE)
                    && (qnode->msg != NULL))
                {
                    expiredNodes[numExpiredNodes] = qnode;
                    numExpiredNodes++;
                }

                pQExpiryData->messagesWithExpiry--;
                pThreadData->stats.bufferedExpiryMsgCount--;

                if (pQExpiryData->messagesInArray == (i+1))
                {
                    //We've expired as many messages as are in the array
                    pQExpiryData->messagesInArray = 0;

                    if (pQExpiryData->messagesWithExpiry == 0)
                    {
                        reapComplete = true;
                    }
                    break;
                }
            }
        }

        if (numExpiredNodes > 0)
        {
            ieiq_destroyMessageBatch( pThreadData
                                    , Q
                                    , numExpiredNodes
                                    , expiredNodes
                                    , false
                                    , &pageCleanupNeeded
                                    , &deliveryIdsAvailable);

            __sync_fetch_and_add(&(Q->expiredMsgs), numExpiredNodes);
            pThreadData->stats.expiredMsgCount += numExpiredNodes;
        }
    }

    if (!reapComplete)
    {
        //do full scan of queue rebuilding expiry data
        bool scanCausesCleanup = false;
        bool scanReleasesIds   = false;

        ieiq_fullExpiryScan( pThreadData
                           , Q
                           , nowExpire
                           , &scanCausesCleanup
                           , &scanReleasesIds);

        pageCleanupNeeded     |= scanCausesCleanup;
        deliveryIdsAvailable  |= scanReleasesIds;
    }

    if (pQExpiryData->messagesWithExpiry == 0)
    {
        if (expiryListLocked)
        {
            rc = ieqExpiryReapRC_RemoveQ;
        }
        else
        {
            ieme_removeQueueFromExpiryReaperList( pThreadData
                                                , (ismEngine_Queue_t *)Q);
        }
    }

    ieme_endReaperQExpiryScan(pThreadData, (ismEngine_Queue_t *)Q);

    if ( deliveryIdsAvailable && (oldStatus != IEWS_WAITERSTATUS_DISCONNECTED))
    {
        assert(    (Q->waiterStatus != IEWS_WAITERSTATUS_DISCONNECTED)
               &&  (Q->waiterStatus != IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT));
        ismEngine_internal_RestartSession (pThreadData, Q->pConsumer->pSession, false);
    }

    ieiq_releaseHeadLock(Q);

    iews_unlockAfterQOperation( pThreadData
                                  , Qhdl
                                  , Q->pConsumer
                                  , &(Q->waiterStatus)
                                  , newStatus
                                  , oldStatus);

    if (pageCleanupNeeded)
    {
        ieiq_scanGetCursor(pThreadData, Q);
        ieiq_cleanupHeadPage(pThreadData, Q);
    }

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Complete the importing of a queue
///
///  @remark Dummy function as this queue does not have a complete imorting step
///
///////////////////////////////////////////////////////////////////////////////
int32_t ieiq_completeImport( ieutThreadData_t *pThreadData
                           , ismQHandle_t Qhdl)
{
    return OK;
}
/// @}

/// @{
/// @name Internal Functions

// Functions used to initialise and destroy put locks
static inline int ieiq_initPutLock(ieiqQueue_t *Q)
{
    int os_rc;

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
    os_rc = pthread_spin_init(&(Q->putlock), PTHREAD_PROCESS_PRIVATE);
#else
    if (ismEngine_serverGlobal.useSpinLocks == true)
    {
        os_rc = pthread_spin_init(&(Q->putlock.spinlock), PTHREAD_PROCESS_PRIVATE);
    }
    else
    {
        os_rc = pthread_mutex_init(&(Q->putlock.mutex), NULL);
    }
#endif

    return os_rc;
}

static inline int ieiq_destroyPutLock(ieiqQueue_t *Q)
{
    int os_rc;

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
    os_rc = pthread_spin_destroy(&(Q->putlock));
#else
    if (ismEngine_serverGlobal.useSpinLocks == true)
    {
        os_rc = pthread_spin_destroy(&(Q->putlock.spinlock));
    }
    else
    {
        os_rc = pthread_mutex_destroy(&(Q->putlock.mutex));
    }
#endif

    return os_rc;
}

static inline void ieiq_getPutLock(ieiqQueue_t *Q)
{
    int os_rc;

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
    os_rc = pthread_spin_lock(&(Q->putlock));
#else
    if (ismEngine_serverGlobal.useSpinLocks == true)
    {
        os_rc = pthread_spin_lock(&(Q->putlock.spinlock));
    }
    else
    {
        os_rc = pthread_mutex_lock(&(Q->putlock.mutex));
    }
#endif

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                      , "failed taking put lock.", ISMRC_Error
                      , "Queue", Q, sizeof(ieiqQueue_t)
                      , NULL );
    }
}

static inline void ieiq_releasePutLock(ieiqQueue_t *Q)
{
    int os_rc;

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
    os_rc = pthread_spin_unlock(&(Q->putlock));
#else
    if (ismEngine_serverGlobal.useSpinLocks == true)
    {
        os_rc = pthread_spin_unlock(&(Q->putlock.spinlock));
    }
    else
    {
        os_rc = pthread_mutex_unlock(&(Q->putlock.mutex));
    }
#endif

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                      , "failed release put lock.", ISMRC_Error
                      , "Queue", Q, sizeof(ieiqQueue_t)
                      , NULL );
    }
}

static inline int ieiq_initHeadLock(ieiqQueue_t *Q)
{
    int os_rc;

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
    os_rc = pthread_spin_init(&(Q->headlock), PTHREAD_PROCESS_PRIVATE);
#else
    if (ismEngine_serverGlobal.useSpinLocks == true)
    {
        os_rc = pthread_spin_init(&(Q->headlock.spinlock), PTHREAD_PROCESS_PRIVATE);
    }
    else
    {
        os_rc = pthread_mutex_init(&(Q->headlock.mutex), NULL);
    }
#endif

    return os_rc;
}

static inline int ieiq_destroyHeadLock(ieiqQueue_t *Q)
{
    int os_rc;

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
    os_rc = pthread_spin_destroy(&(Q->headlock));
#else
    if (ismEngine_serverGlobal.useSpinLocks == true)
    {
        os_rc = pthread_spin_destroy(&(Q->headlock.spinlock));
    }
    else
    {
        os_rc = pthread_mutex_destroy(&(Q->headlock.mutex));
    }
#endif

    return os_rc;
}

//If this and the waiterstatus lock are both taken, waiterstatus is taken first
static inline void ieiq_getHeadLock(ieiqQueue_t *Q)
{
    int os_rc;

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
    os_rc = pthread_spin_lock(&(Q->headlock));
#else
    if (ismEngine_serverGlobal.useSpinLocks == true)
    {
        os_rc = pthread_spin_lock(&(Q->headlock.spinlock));
    }
    else
    {
        os_rc = pthread_mutex_lock(&(Q->headlock.mutex));
    }
#endif

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                      , "failed taking head lock.", ISMRC_Error
                      , "Queue", Q, sizeof(ieiqQueue_t)
                      , NULL );
    }
}

static inline void ieiq_releaseHeadLock(ieiqQueue_t *Q)
{
    int os_rc;

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
    os_rc = pthread_spin_unlock(&(Q->headlock));
#else
    if (ismEngine_serverGlobal.useSpinLocks == true)
    {
        os_rc = pthread_spin_unlock(&(Q->headlock.spinlock));
    }
    else
    {
        os_rc = pthread_mutex_unlock(&(Q->headlock.mutex));
    }
#endif

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                      , "failed release head lock.", ISMRC_Error
                      , "Queue", Q, sizeof(ieiqQueue_t)
                      , NULL );
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Return pointer to nodePage structure from final Node in array
///  @remarks
///    The ieiqQNodePage_t structure contains an array of ieiqQNode_t
///    structures. This function accepts a pointer top the last
///    ieiqQNode_t entry in the array and returns a pointer to the
///    ieiqQNodePage_t structure.
///  @param[in] node               - points to the end of page marker
///  @return                       - pointer to the CURRENT page
///////////////////////////////////////////////////////////////////////////////
static inline ieiqQNodePage_t *ieiq_getPageFromEnd( ieiqQNode_t *node )
{
    ieiqQNodePage_t *page;

    //Check we've been given a pointer to the end of the page
    assert( node->msgState == ieqMESSAGE_STATE_END_OF_PAGE );

    page = (ieiqQNodePage_t *)node->msg;

    ismEngine_CheckStructId(page->StrucId, IEIQ_PAGE_STRUCID, ieutPROBE_029);

    return page;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Allocate and initialise a new queue page
///  @remarks
///    Allocate (malloc) the storage for a new ieiqQNodePage_t structure
///    and initialise it as an 'empty' page. This function does not chain
///    the allocate page onto the queue.
///
/// @param[in] Q                   - Queue handle
/// @param[in] prevSeqNo           - Previous page sequence number
/// @return                        - NULL if a failure occurred or a pointer
///                                  to newly created ieiqQNodePage_t
///////////////////////////////////////////////////////////////////////////////
static inline ieiqQNodePage_t *ieiq_createNewPage( ieutThreadData_t *pThreadData
                                                 , ieiqQueue_t *Q
                                                 , uint32_t nodesInPage
                                                 , ieiqPageSeqNo_t prevSeqNo)
{
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    iere_primeThreadCache(pThreadData, resourceSet);
    size_t pageSize = offsetof(ieiqQNodePage_t, nodes)
                    + (sizeof(ieiqQNode_t) * (nodesInPage+1));
    ieiqQNodePage_t *page = (ieiqQNodePage_t *)iere_calloc(pThreadData,
                                                           resourceSet,
                                                           IEMEM_PROBE(iemem_intermediateQPage, 1), 1, pageSize);

    if (page != NULL)
    {
        //Add the end of page marker to the page
        ismEngine_SetStructId(page->StrucId, IEIQ_PAGE_STRUCID);
        page->nodes[nodesInPage].msgState = ieqMESSAGE_STATE_END_OF_PAGE;
        page->nodes[nodesInPage].msg = (ismEngine_Message_t *)page;
        page->nodesInPage = nodesInPage;
        page->sequenceNo = prevSeqNo+1;
        ieutTRACEL(pThreadData, pageSize, ENGINE_FNC_TRACE, FUNCTION_IDENT "Q %p, page %lu size %lu (nodes %u)\n",
                   __func__, Q, page->sequenceNo, pageSize, nodesInPage);

    }
    else
    {
        ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_IDENT "Q %p, page %lu - no mem\n",__func__, Q, prevSeqNo+1);
    }

    return page;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    return pointer to next ieiqQNodePage_t structure in queue
///  @remarks
///    Given a pointer to an end of page marker, return a pointer to the next
///    page. If the next page has not yet been chained in, we wait for the
///    thread that should do it.
///    If the thread which should have created the pages failed to create
///    the page we try to create the page ourselves.
///
///  @param[in] Q                  - Queue handle
///  @param[in] endMsg             - Pointer to last message on a page
///  @return                       - pointer to the next page or NULL
///////////////////////////////////////////////////////////////////////////////
static inline ieiqQNodePage_t *ieiq_moveToNewPage( ieutThreadData_t *pThreadData
                                                 , ieiqQueue_t *Q
                                                 , ieiqQNode_t *endMsg)
{
    DEBUG_ONLY bool result;
    ieiqQNodePage_t *currpage = ieiq_getPageFromEnd(endMsg);
    uint32_t curSeqNo = currpage->sequenceNo;

    if (currpage->next != NULL)
    {
        return currpage->next;
    }

    //Hmmm... no new page yet! Someone should be adding it, wait while they do
    while (currpage->next == NULL)
    {
        if (currpage->nextStatus == failed)
        {
            if (currpage->sequenceNo != 1)
            {
                ieutTRACEL(pThreadData, currpage, ENGINE_NORMAL_TRACE,
                           "%s: noticed next page addition to %p has not occurred\n",
                           __func__, currpage);
            }

            // The person who should add the page failed, are we the one to fix it?
            if (__sync_bool_compare_and_swap(&(currpage->nextStatus), failed, repairing))
            {
                // We have marked the status as repairing - we get to do it
                uint32_t nodesInPage = ieiq_choosePageSize();
                ieiqQNodePage_t *newpage = ieiq_createNewPage( pThreadData
                                                             , Q
                                                             , nodesInPage
                                                             , currpage->sequenceNo);
                ieqNextPageStatus_t newStatus;

                if (newpage != NULL)
                {
                    currpage->next = newpage;
                    newStatus = unfinished;

                    if (currpage->sequenceNo != 1)
                    {
                        ieutTRACEL(pThreadData, newpage, ENGINE_NORMAL_TRACE,
                                   "%s: successful new page addition to Q %p currpage %p newPage %p\n",
                                   __func__, Q, currpage, newpage);
                    }
                }
                else
                {
                    // We couldn't create a page either
                    ieutTRACEL(pThreadData, nodesInPage, ENGINE_WORRYING_TRACE, "%s: failed new page addition to Q %p currpage %p\n",
                               __func__, Q, currpage);
                    newStatus = failed;
                }
                result=__sync_bool_compare_and_swap(&(currpage->nextStatus), repairing, newStatus);
                assert (result);

                break; // Leave the loop
            }
        }
        // We need the memory barrier here to ensure we see the change
        // made to 'currpage->next' by an other thread.
        ismEngine_ReadMemoryBarrier();
    }

    if (currpage->next)
    {
        if (currpage->next->sequenceNo <= curSeqNo)
        {
            // EEk! The sequence number on our page is not monotonically
            // increasing. End the server when an FDC to see if we can
            // work out why.
            ieutTRACE_FFDC( ieutPROBE_001, false
                          , "Page Sequence number didn't increase ", ISMRC_Error
                          , "Queue", Q, sizeof(ieiqQueue_t)
                          , "Base page number", &curSeqNo, sizeof(uint32_t)
                          , "Base page", currpage, sizeof(ieiqQNodePage_t)
                          , "Next page number", &(currpage->next->sequenceNo), sizeof(uint32_t)
                          , "Next page", currpage->next, sizeof(ieiqQNodePage_t)

                          , NULL );

            // Use the dump queue facility to write debug information
            iedm_dumpQueueByHandle((ismQHandle_t)Q, 9, -1, "");

            // If this error is terminal then bring down the server
            LOG( ERROR, Messaging, 3005, "%s%d",
                 "An unrecoverable failure has occurred at location {0}:{1}. The failure recording routine has been called. The server will now stop and restart.",
                 __func__, 24 );

            ism_common_shutdown(true);
        }
    }

    return currpage->next;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Moves head cursor passed messages that have been removed (e.g. expired
///    messages) freeing unneeded queue pages
///
///  @remark The waiterlock must be held by the caller of this function!
///
///  @param[in] Q                  - Queue to check head pointer on
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////

static void ieiq_scanGetCursor( ieutThreadData_t *pThreadData
                              , ieiqQueue_t *Q)
{
    ieutTRACEL(pThreadData,Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n", __func__,Q);

    while(Q->cursor->msgState == ismMESSAGE_STATE_CONSUMED)
    {
        //Find the next node
        ieiqQNode_t *nextNode = Q->cursor+1;

        if(nextNode->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
        {
            //Yep...move to the next page if it has been added...
            //...if it hasn't been added someone will add it shortly
            ieiqQNodePage_t *currpage =  (ieiqQNodePage_t *)(nextNode->msg);

            ismEngine_CheckStructId(currpage->StrucId, IEIQ_PAGE_STRUCID, ieutPROBE_001);

            if(currpage->nextStatus == completed)
            {
                //Another page has been added so the cursor can point into
                ieiqQNodePage_t *nextPage = currpage->next;
                ismEngine_CheckStructId(nextPage->StrucId, IEIQ_PAGE_STRUCID, ieutPROBE_002);

                Q->cursor     = &(nextPage->nodes[0]);

                DEBUG_ONLY bool setState;
                setState = __sync_bool_compare_and_swap( &(currpage->nextStatus)
                                                       , completed
                                                       , cursor_clear );
                assert(setState);
            }
            else
            {
                //Next page not yet ready, we need to leave the cursor on this one
                break;
            }
        }
        else
        {
            Q->cursor = nextNode;
        }
    }

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}



///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Returns a pointer to the ieiqQNode_t containing the next msg to be
///    delivered.
///  @remarks
///    This function returns a pointer to the ieiqQNode_t structure containing
///    the next message to be delivered (according to the cursor) and updates
///    the cursor to point at the next message.
///    @par
///    This function is only called when bracketed by CASing the waiter
///    state to 'getting'. This CASing acts like a lock&barriers so we can
///    use a simple increment on the cursor pointer.
///
///  @param[in] Q                  - Queue to get the message from
///  @param[in] allowRewindCursor  - If we'e not allowed but we want to,
///                                   we return ISMRC_RecheckQueue
///  @param[out] ppnode            - On successful completion, a pointer to
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
static inline int32_t ieiq_locateMessage( ieutThreadData_t *pThreadData
                                        , ieiqQueue_t       *Q
                                        , bool              allowRewindCursor
                                        , ieiqQNode_t       **ppnode)
{
    int32_t rc = ISMRC_NoMsgAvail;

    ieutTRACEL(pThreadData, Q, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY " Q=%p resetCursor=%s\n", __func__,
               Q, Q->resetCursor?"true":"false");

    *ppnode = NULL;

    // If we have been asked to reset the Get cursor then we do it now
    if (Q->resetCursor)
    {
        if (allowRewindCursor)
        {
            ieiq_resetCursor(pThreadData, Q);
        }
        else
        {
            //We can't rewind halfway through a batch... we'll
            //find the same messages again...look again once we've
            //delivered the batch so far
            rc = IMSRC_RecheckQueue;
            goto mod_exit;
        }
    }
    ieutTRACEL(pThreadData, Q->cursor->orderId, ENGINE_HIFREQ_FNC_TRACE,"SRTSCAN: oId: %lu, Redlv: %u Redlv Cursor: %lu\n",
                  Q->cursor->orderId, Q->Redelivering, Q->redeliverCursorOrderId);

    // Now loop through looking for a message we can deliver. We will
    // skip over any message we can't deliver
    while (rc == ISMRC_NoMsgAvail)
    {
        if((Q->cursor+1)->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
        {
            ieiqQNodePage_t *currpage = (ieiqQNodePage_t *)((Q->cursor+1)->msg);

            ismEngine_CheckStructId(currpage->StrucId, IEIQ_PAGE_STRUCID, ieutPROBE_030);

            // We can only return the last message in a page when the next page
            // has already been added (it would break the queue as we need
            // somewhere for the cursor to sit when we move it on.
            if (currpage->nextStatus < completed)
            {
                break;
            }
        }
        if ((Q->cursor->msgState == ismMESSAGE_STATE_AVAILABLE) &&
            (Q->cursor->msg != NULL))
        {
            if( ! (Q->redeliverOnly))
            {
                rc = OK;
                *ppnode = Q->cursor;
            }
            else
            {
                //If this message is not inflight move the get cursor...
                if(Q->cursor->deliveryCount == 0)
                {
                    ieiq_moveGetCursor(pThreadData, Q, Q->cursor);
                }
            }
        }
        else if ((Q->Redelivering) &&
                 (Q->cursor->msg != NULL) &&
                 (Q->redeliverCursorOrderId <= Q->cursor->orderId) &&
                 ((Q->cursor->msgState == ismMESSAGE_STATE_DELIVERED) ||
                  (Q->cursor->msgState == ismMESSAGE_STATE_RECEIVED)))
        {
            rc = OK;
            *ppnode = Q->cursor;
        }
        else if (Q->cursor->msgState != ismMESSAGE_STATE_AVAILABLE)
        {
            assert(Q->cursor->msgState != ieqMESSAGE_STATE_END_OF_PAGE);
            ieiq_moveGetCursor(pThreadData, Q, Q->cursor);
        }
        else
        {
            // No more messages
            break;
        }
    }

    //If we are redelivering messages after the queue has been re-opened
    //...and we have run out of messages to redeliver
    //then start delivering new messages
    if(Q->Redelivering && (rc == ISMRC_NoMsgAvail))
    {
        ieutTRACEL(pThreadData, Q, ENGINE_NORMAL_TRACE, "Q=%p finished redelivery of inflight messages\n", Q);
        Q->Redelivering = false;
        rc = IMSRC_RecheckQueue;
    }

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d, cursor=%p\n", __func__, rc, Q->cursor);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Returns a pointer to the ieiqQNode_t containing the next msg to be
///    delivered.
///  @remarks
///    This function returns a pointer to the ieiqQNode_t structure containing
///    the next message to be delivered (according to the cursor) and updates
///    the cursor to point at the next message.
///    @par
///    This function is only called when bracketed by CASing the waiter
///    state to 'getting'. This CASing acts like a lock&barriers so we can
///    use a simple increment on the cursor pointer.
///
///  @param[in] Q                  - Queue to get the message from
///  @param[out] ppnode            - On successful completion, a pointer to
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
static inline void ieiq_moveGetCursor( ieutThreadData_t *pThreadData
                                     , ieiqQueue_t       *Q
                                     , ieiqQNode_t       *cursor )
{
    ieiqQNode_t *newcursor;
    ieutTRACEL(pThreadData,Q,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY " Q=%p cursor=%p\n", __func__,Q,cursor);

    assert(cursor == Q->cursor);

    newcursor = Q->cursor+1;
    if(newcursor->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
    {
        ieiqQNodePage_t *currpage = (ieiqQNodePage_t *)((Q->cursor+1)->msg);
        ieiqQNodePage_t *nextpage = currpage->next;

        ismEngine_CheckStructId(currpage->StrucId, IEIQ_PAGE_STRUCID, ieutPROBE_031);
        ismEngine_CheckStructId(nextpage->StrucId, IEIQ_PAGE_STRUCID, ieutPROBE_032);

        DEBUG_ONLY bool setState;
        setState = __sync_bool_compare_and_swap( &(currpage->nextStatus)
                                               , completed
                                               , cursor_clear );
        assert(setState);

        Q->cursor     = &(nextpage->nodes[0]);
    }
    else
    {
        Q->cursor = newcursor;
    }

    ieutTRACEL(pThreadData, newcursor,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "cursor=%p\n", __func__, newcursor);
    return;
}

//Should be called with a consumer in locked state...
//After called *need* to call completeWaiterActions();
static void ieiq_handleDeliveryFailure( ieutThreadData_t *pThreadData
                                      , int32_t       rc
                                      , ieiqQueue_t  *q)
{
    ieutTRACEL(pThreadData, q->pConsumer,  ENGINE_WORRYING_TRACE, FUNCTION_IDENT "pCons=%p \n", __func__, q->pConsumer);

    //Notify the delivery failure callback /before/ unlocking the consumer so
    //that we know that the consumer will be valid...
    if (ismEngine_serverGlobal.deliveryFailureFn != NULL)
    {
        ismEngine_serverGlobal.deliveryFailureFn( rc
                                                , q->pConsumer->pSession->pClient
                                                , q->pConsumer
                                                , q->pConsumer->pMsgCallbackContext);
    }
    else
    {
        //We can't ask the protocol to disconnect as they haven't registered the callback... we're going to
        //come down in a ball of flames....
        ieutTRACE_FFDC( ieutPROBE_001, true
                , "Out of memory during message delivery and no delivery failure handler was registered.", ISMRC_AllocateError
                , "Queue",         q,               sizeof(ieiqQueue_t)
                , "Internal Name", q->InternalName, sizeof(q->InternalName)
                , "consumer",      q->pConsumer,    sizeof(ismEngine_Consumer_t)
                , NULL );
    }

    iews_addPendFlagWhileLocked( &(q->waiterStatus)
                               , IEWS_WAITERSTATUS_DISABLE_PEND);
}


static ismMessageState_t ieiq_getMessageStateWhenDelivered(ieiqQNode_t *pnode)
{
    //Start with the current state...
    ismMessageState_t newMsgState = pnode->msgState;

    // Work out how it will change when delivered...
    if ((pnode->msg->Header.Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
      || (ieiq_MaxReliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE))
    {
        newMsgState = ismMESSAGE_STATE_CONSUMED;
    }
    else if (newMsgState == ismMESSAGE_STATE_AVAILABLE)
    {
        newMsgState = ismMESSAGE_STATE_DELIVERED;
    }

    return newMsgState;
}


static void ieiq_completePreparingMessage( ieutThreadData_t      *pThreadData
                                         , ieiqQueue_t           *Q
                                         , ieiqQNode_t           *pnode
                                         , ismMessageState_t      newMsgState
                                         , ismEngine_Message_t  **phmsg
                                         , ismMessageHeader_t    *pmsgHdr
                                         , uint32_t              *pdeliveryId
                                         , ieiqQNode_t          **pnodeDelivery)
{
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    // Copy the message header from the one published
    *pmsgHdr = pnode->msg->Header;

    // Add the delivery count from the node
    pmsgHdr->RedeliveryCount += pnode->deliveryCount;

    // Update the deliver count in the node,
    pnode->deliveryCount++;

    if (pnode->deliveryCount > ieqENGINE_MAX_REDELIVERY_COUNT)
    {
       pnode->deliveryCount = ieqENGINE_MAX_REDELIVERY_COUNT;
    }

    // Ensure the delivery count does not exceed the maximum
    if (pmsgHdr->RedeliveryCount > ieqENGINE_MAX_REDELIVERY_COUNT)
    {
        pmsgHdr->RedeliveryCount = ieqENGINE_MAX_REDELIVERY_COUNT;
    }

    // Include any additional flags
    pmsgHdr->Flags |= pnode->msgFlags;

    // And set the order ID on the message
    pmsgHdr->OrderId = (pmsgHdr->Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT) ? pnode->orderId : 0;

    if (pnode->msgState == ismMESSAGE_STATE_AVAILABLE && pmsgHdr->Expiry != 0)
    {
        ieme_removeMessageExpiryData( pThreadData, (ismEngine_Queue_t *)Q,  pnode->orderId);
    }

    *phmsg = pnode->msg;

    if ((newMsgState == ismMESSAGE_STATE_DELIVERED) ||
        (newMsgState == ismMESSAGE_STATE_RECEIVED))
    {
        // Increase the use count of the message. The callback will be
        // responsible for performing the decrement.
        iem_recordMessageUsage(pnode->msg);

        if (pnode->msgState == ismMESSAGE_STATE_AVAILABLE)
        {
            uint64_t oldinflight = __sync_fetch_and_add(&(Q->inflightDeqs), 1);

            if (oldinflight == 0)
            {
                //This is the first message in flight and having messages inflight out of a queue add +1 to the predelete count
                __sync_fetch_and_add(&(Q->preDeleteCount), 1);
            }
        }

        // Identify the message as having been delivered
        pnode->msgState = newMsgState;

        ieutTRACEL(pThreadData, pnode, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "Setting node %p to %u\n",
                              __func__, pnode, newMsgState);

        *pnodeDelivery = pnode;
        *pdeliveryId = pnode->deliveryId;
    }
    else
    {
        assert(newMsgState == ismMESSAGE_STATE_CONSUMED);

        if (pnode->inStore)
        {
            DEBUG_ONLY uint32_t storeOps = 0;
            iest_unstoreMessage( pThreadData, pnode->msg, false, true, NULL, &storeOps);

            //As we're using lazy removal, we shouldn't be adding store ops we need to commit
            assert( storeOps == 0);
        }

        iere_primeThreadCache(pThreadData, resourceSet);
        iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -1);
        pThreadData->stats.bufferedMsgCount--;
        DEBUG_ONLY int32_t oldDepth = __sync_fetch_and_sub(&(Q->bufferedMsgs), 1);
        assert(oldDepth > 0);
        __sync_fetch_and_add(&(Q->dequeueCount), 1);

        // Decide if we may need to do some head page cleanup
        bool pageCleanup = ((pnode+1)->msgState == ieqMESSAGE_STATE_END_OF_PAGE);


        pnode->msg = MESSAGE_STATUS_EMPTY;

        // Having set this state to CONSUMED we cannot access the node
        // again. Ensure we have a write barrier to ensure all the
        // previous memory accesses are completed.
        if ((pnode->msgState == ismMESSAGE_STATE_DELIVERED) ||
            (pnode->msgState == ismMESSAGE_STATE_RECEIVED))
        {
            //Hmm we don't expect to deliver as CONSUMED messages that have already been delivered.
            //If this happens we'd need some code like the commented out code below the FFDC
            ieutTRACE_FFDC( ieutPROBE_010, true
                    , "delivering as consumed a message that has already been delivered", ISMRC_Error
                    , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                    , "Queue", Q, sizeof(ieiqQueue_t)
                    , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                    , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                    , "pNode", pnode, sizeof(ieiqQNode_t)
                    , NULL );

            //uint64_t newinflightcount = __sync_fetch_and_sub(&(Q->inflightDeqs), 1);
            //
            //if (newinflightcount == 0)
            //{
            //    //No messages in flight, we need to reduce queue use count but don't do it until we stop accessing the queue
            //    hmmmhalfwaythroughdeliverywhencanwereduceusecount();
            //}
        }
        else
        {
            ismEngine_WriteMemoryBarrier();
        }
        ieutTRACEL(pThreadData, pnode, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "Setting node %p to consumed\n",
                              __func__, pnode);
        pnode->msgState = newMsgState;

        // We possibly need to clean up the nodePage which contained this node
        // so do this now
        if (pageCleanup)
        {
            ieiq_cleanupHeadPage(pThreadData, Q);
        }

        *pnodeDelivery = NULL;
        *pdeliveryId = 0;
    }
}

static void ieiq_undoInitialPrepareMessage( ieutThreadData_t *pThreadData
                                          , ieiqQueue_t *Q
                                          , bool prepareCommitted
                                          , uint64_t nodecountToUndo
                                          , ieiqQNode_t **pnodes
                                          , uint64_t *pStoreOps)
{
    ieutTRACEL(pThreadData, nodecountToUndo, ENGINE_WORRYING_TRACE, FUNCTION_IDENT "queue=%p - nodecount %lu\n",
                          __func__, Q, nodecountToUndo);


#ifdef HAINVESTIGATIONSTATS
    pThreadData->stats.messagesDeliveryCancelled += nodecountToUndo;
#endif

    if (!prepareCommitted)
    {
        iest_store_rollback(pThreadData, false);
        *pStoreOps = 0;
    }

    for (uint64_t i=0; i < nodecountToUndo; i++)
    {
        ieiqQNode_t *pnode = pnodes[i];

        ismMessageState_t newMsgState = ieiq_getMessageStateWhenDelivered(pnode);

          if ( (newMsgState == ismMESSAGE_STATE_DELIVERED) ||
               (newMsgState == ismMESSAGE_STATE_RECEIVED) )
          {
              if (prepareCommitted && pnode->inStore)
              {
                  //Undo delivery count but leave id....
                  uint32_t state = (pnode->deliveryCount)  & 0x3f;
                  state |= (pnode->msgState << 6);

                  int32_t rc = ism_store_updateReference( pThreadData->hStream
                                                        , Q->QueueRefContext
                                                        , pnode->hMsgRef
                                                        , pnode->orderId
                                                        , state
                                                        , 0 ); // minimumActiveOrderId

                  *pStoreOps += 1;

                  if (UNLIKELY(rc != OK))
                  {
                      // The failure to update a store reference (or commit the changes from
                      // iecs_storeMessageDeliveryReference) means that the queue is inconsistent.
                      ieutTRACE_FFDC( ieutPROBE_001, true
                                    , "ism_store_updateReference failed.", rc
                                    , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                                    , "Queue", Q, sizeof(ieiqQueue_t)
                                    , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                                    , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                                    , "pNode", pnode, sizeof(ieiqQNode_t)
                                    , NULL );
                  }

              }
          }
          else
          {
              assert(newMsgState == ismMESSAGE_STATE_CONSUMED);

              if (prepareCommitted)
              {
                  //Argh... we've removed the reference in the store. Complete the removal from the
                  //store (we have no choice but to go forward)... but leave the node in memory in the
                  //hope we can deliver it before we restart
                  if (pnode->inStore)
                  {
                      DEBUG_ONLY uint32_t storeOps = 0;
                      iest_unstoreMessage( pThreadData, pnode->msg, false, true, NULL, &storeOps);

                      //As we're using lazy removal, we shouldn't be adding store ops we need to commit
                      assert( storeOps == 0);
                      pnode->inStore = false;
                      pnode->hMsgRef = 0;
                  }
              }
              else
              {
                  //Nothing to do - all that was required was the rollback
              }
          }
    }
}

static void ieiq_deliverMessages( ieutThreadData_t *pThreadData
                                , ieiqQueue_t *Q
                                , uint64_t foundNodes
                                , ieiqQNode_t **pnodes
                                , bool *pCompleteWaiterActions
                                , bool *pDeliverMoreMessages
                                , uint64_t *pStoreOps
                                , ismEngine_DelivererContext_t * delivererContext)
{
    //Does the user explicitly call suspend or do we disable if the return code is false
    bool fExplicitSuspends = Q->pConsumer->pSession->fExplicitSuspends;
    bool wantsMoreMessages = true;

    *pCompleteWaiterActions = false;

    for (uint64_t i = 0; wantsMoreMessages && i <  foundNodes; i++)
    {
        ieiqQNode_t *pnode =  pnodes[i];
        ieiqQNode_t *pnodeDelivery = NULL; //If the node is still valid during delivery, this will point to it
        ismMessageHeader_t    msgHdr;
        ismEngine_Message_t  *hmsg;
        uint32_t              deliveryId;

        assert(pnode->msgState != ismMESSAGE_STATE_CONSUMED);

        if (Q->Redelivering)
        {
            uint64_t nextOId = pnode->orderId + 1;

            if (nextOId > Q->redeliverCursorOrderId)
            {
                // Also record how far we have got through redelivering. We can attempt to
                //redeliver the new cursor message (as we haven't done that since reconnecting)
                Q->redeliverCursorOrderId = nextOId;
            }
        }

        ismMessageState_t  newState = ieiq_getMessageStateWhenDelivered(pnode);

        ieiq_completePreparingMessage( pThreadData
                                     , Q
                                     , pnode
                                     , newState
                                     , &hmsg
                                     , &msgHdr
                                     , &deliveryId
                                     , &pnodeDelivery);

        wantsMoreMessages = ism_engine_deliverMessage(
                                      pThreadData,
                                      Q->pConsumer,
                                      pnodeDelivery,
                                      hmsg,
                                      &msgHdr,
                                      newState,
                                      deliveryId,
                                      delivererContext);

        if (!wantsMoreMessages)
        {

            if (!fExplicitSuspends)
            {
                iews_addPendFlagWhileLocked( &(Q->waiterStatus)
                                           , IEWS_WAITERSTATUS_DISABLE_PEND);
            }

            //We need to call delivery callback etc...
            *pCompleteWaiterActions = true;

            //Are there message we prepared but don't want to deliver?
            if ( i < (foundNodes - 1))
            {
                //We need to rewind the cursor to redeliver the messages the consumer
                //doesn't want yet.
                //We still have the consumer locked... we didn't rewind it during the message
                //batch (it's only rewound between batches), noone else can have... and the next message
                //to deliver must be the first we haven't delivered.
                ieiq_rewindCursorToNode(pThreadData, Q, pnodes[i+1]);

                ieiq_undoInitialPrepareMessage( pThreadData
                                              , Q
                                              , true
                                              , foundNodes - i -1
                                              , &(pnodes[i+1])
                                              , pStoreOps);
            }
        }
    }

#ifdef HAINVESTIGATIONSTATS
    clock_gettime(CLOCK_REALTIME, &(Q->pConsumer->lastDeliverEnd));
    ism_engine_recordConsumerStats(pThreadData, Q->pConsumer);
#endif

    if (wantsMoreMessages)
    {
        //We don't have any more to give it....put it back into enabled
        //and ask for more to sent along...
        *pDeliverMoreMessages = true;

        iewsWaiterStatus_t oldState = __sync_val_compare_and_swap( &(Q->waiterStatus)
                                        , IEWS_WAITERSTATUS_DELIVERING
                                        , IEWS_WAITERSTATUS_ENABLED);

        if (oldState != IEWS_WAITERSTATUS_DELIVERING)
        {
            //Other people have added actions for us to do...
            *pCompleteWaiterActions = true;
        }
    }
    else
    {
        *pDeliverMoreMessages = false;
    }
}

static int32_t ieiq_asyncCancelDelivery(ieutThreadData_t           *pThreadData,
                                        int32_t                              rc,
                                        ismEngine_AsyncData_t        *asyncInfo,
                                        ismEngine_AsyncDataEntry_t     *context)
{
    assert(rc == OK);
    assert(context->Type == ieiqQueueCancelDeliver);
    ieiqQueue_t *Q = (ieiqQueue_t *)(context->Handle);

    //Pop this entry off the stack now in case complete goes async
    iead_popAsyncCallback( asyncInfo, context->DataLen);

    //We put the waiter into disable pending we *know* we need to completeWaiter actions
    ieq_completeWaiterActions( pThreadData
                             , (ismEngine_Queue_t *)Q
                             , Q->pConsumer
                             , IEQ_COMPLETEWAITERACTION_OPTS_NONE
                             , true);

    return rc;
}


static int32_t ieiq_asyncMessageDelivery(ieutThreadData_t           *pThreadData,
                                         int32_t                              rc,
                                         ismEngine_AsyncData_t        *asyncInfo,
                                         ismEngine_AsyncDataEntry_t     *context)
{
    ieiqAsyncMessageDeliveryInfo_t *deliveryInfo = (ieiqAsyncMessageDeliveryInfo_t *)(context->Data);
    ismEngine_CheckStructId(deliveryInfo->StructId, IEIQ_ASYNCMESSAGEDELIVERYINFO_STRUCID, ieutPROBE_001);
    assert(context->Type == ieiqQueueDeliver);

    assert(rc == OK);

    ieiqQueue_t *Q = deliveryInfo->Q;

#ifdef HAINVESTIGATIONSTATS
    if (   Q->pConsumer->lastDeliverCommitEnd.tv_sec == 0)
    {
        //If we haven't recorded a time for the first store commit in this get
        clock_gettime(CLOCK_REALTIME, &(Q->pConsumer->lastDeliverCommitEnd));
    }
#endif

    bool     callCompleteWaiterActions = false;
    bool     deliverMoreMsgs           = false;
    uint64_t storeOps                  = 0;

    ismEngine_DelivererContext_t delivererContext;
    delivererContext.lockStrategy.rlac = LS_NO_LOCK_HELD;
    delivererContext.lockStrategy.lock_persisted_counter = 0;
    delivererContext.lockStrategy.lock_dropped_counter = 0;

    ieiq_deliverMessages( pThreadData
                        , Q
                        , deliveryInfo->usedNodes
                        , deliveryInfo->pnodes
                        , &callCompleteWaiterActions
                        , &deliverMoreMsgs
                        , &storeOps
                        , &delivererContext );

    if ( delivererContext.lockStrategy.rlac == LS_READ_LOCK_HELD || delivererContext.lockStrategy.rlac == LS_WRITE_LOCK_HELD ) {
        ieutTRACEL(pThreadData, 0, ENGINE_PERFDIAG_TRACE,
                  "RLAC Lock was held and has now been released, debug: %d,%d\n",
                  delivererContext.lockStrategy.lock_persisted_counter,delivererContext.lockStrategy.lock_dropped_counter);
        ism_common_unlockACLList();
    } else {
        ieutTRACEL(pThreadData, 0, ENGINE_PERFDIAG_TRACE,
                  "RLAC Lock was not held, debug: %d,%d\n",
                   delivererContext.lockStrategy.lock_persisted_counter,delivererContext.lockStrategy.lock_dropped_counter);
    }
    delivererContext.lockStrategy.rlac = LS_NO_LOCK_HELD;

    //We've finished delivering this message... remove the entry that calls this function
    //so if checkWaiters goes async we don't try to deliver this message again
    iead_popAsyncCallback( asyncInfo, context->DataLen);

    if (storeOps != 0)
    {
        //Argh, we needed to undo half delivered messages... do that before
        //we unlock waiter
        ismEngine_AsyncDataEntry_t newEntry =
                       { ismENGINE_ASYNCDATAENTRY_STRUCID
                       , ieiqQueueCancelDeliver
                       , NULL, 0
                       , Q /* all we need is the queue */
                       , {.internalFn = ieiq_asyncCancelDelivery}};

        iead_pushAsyncCallback(pThreadData, asyncInfo, &newEntry);

        rc = iead_store_asyncCommit(pThreadData, false, asyncInfo);
        assert (rc == OK || rc == ISMRC_AsyncCompletion);

        if (rc == OK)
        {
            iead_popAsyncCallback( asyncInfo, newEntry.DataLen);
        }
    }

    if (rc == OK)
    {
        if(callCompleteWaiterActions)
        {
            ieq_completeWaiterActions( pThreadData
                                     , (ismEngine_Queue_t *)Q
                                     , Q->pConsumer
                                     , IEQ_COMPLETEWAITERACTION_OPTS_NONE
                                     , true);
        }
        else if (deliverMoreMsgs)
        {
            rc = ieiq_checkWaiters(pThreadData, (ismEngine_Queue_t *)Q, asyncInfo, NULL);
        }
        assert (rc == OK || rc == ISMRC_AsyncCompletion);
    }
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Do parts of delivery preparation that might fail.
///    Chooses a delivery id and update the message state in the store ahead of delivery
///  @remarks
///    Further preparation is done in ieiq_completePreparingMessage
///  @remarks
///    Can be "undone" by ieiq_undoInitialPrepareMessage
///  @remarks
///    This function is called prior to delivering a message to the consumer
///    to indicate that it has been delivered.
///
///  @param[in] Q                  - Queue to get the message from
///  @param[in] pnode              - A pointer to the node containing the msg
///  @param[in] msgState           - The state to change the message to
///  @param[out] storeOpCount      - Updated if we use the store
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
static int32_t ieiq_initialPrepareMessageForDelivery( ieutThreadData_t      *pThreadData
                                                    , ieiqQueue_t           *Q
                                                    , ieiqQNode_t           *pnode
                                                    , uint64_t              *pStoreOpCount)
{
    uint32_t rc = OK;
    uint32_t state;

    ieutTRACEL(pThreadData, pnode->msg, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY " pnode=%p pMessage=%p Reliability=%d\n",
               __func__, pnode, pnode->msg, pnode->msg->Header.Reliability);


    ismEngine_CheckStructId(pnode->msg->StrucId, ismENGINE_MESSAGE_STRUCID, ieutPROBE_033);

    ismMessageState_t newMsgState = ieiq_getMessageStateWhenDelivered(pnode);

    if ( (newMsgState == ismMESSAGE_STATE_DELIVERED) ||
         (newMsgState == ismMESSAGE_STATE_RECEIVED) )
    {
        if (Q->hMsgDelInfo == NULL)
        {
            // If we don't yet have the delivery information handle, get it now
            rc = iecs_acquireMessageDeliveryInfoReference(pThreadData,
                                                          Q->pConsumer->pSession->pClient,
                                                          &Q->hMsgDelInfo);
            if (rc != OK)
            {
                goto mod_exit;
            }
        }

        if (!pnode->inStore)
        {
            if (pnode->deliveryId == 0)
            {
                if (Q->hStoreObj != ismSTORE_NULL_HANDLE)
                {
                    rc = iecs_assignDeliveryId(pThreadData,
                                               Q->hMsgDelInfo,
                                               Q->pConsumer->pSession,
                                               Q->hStoreObj,
                                               NULL, // Don't record hQueue
                                               NULL, // Don't record hNode
                                               false,
                                               &pnode->deliveryId);
                }
                else
                {
                    rc = iecs_assignDeliveryId(pThreadData,
                                               Q->hMsgDelInfo,
                                               Q->pConsumer->pSession,
                                               (ismStore_Handle_t)Q, // The owner cannot be a Store handle
                                               NULL, // Don't record hQueue
                                               NULL, // Don't record hNode
                                               true,
                                               &pnode->deliveryId);
                }
                if (rc != OK)
                {
                    goto mod_exit;
                }
            }
        }
        else
        {
            state = (pnode->deliveryCount+1)  & 0x3f; //+1 as we're about to deliver it
            state |= (newMsgState << 6);

            iest_AssertStoreCommitAllowed(pThreadData);

            if (!pnode->hasMDR)
            {
                //If this function is successful, it can leave an uncommitted
                //store transaction for us to commit (if it fails it may have called rollback)
                rc = iecs_storeMessageDeliveryReference( pThreadData
                                                       , Q->hMsgDelInfo
                                                       , Q->pConsumer->pSession
                                                       , Q->hStoreObj
                                                       , NULL // Don't record hQueue
                                                       , NULL // Don't record hNode
                                                       , pnode->hMsgRef
                                                       , &pnode->deliveryId
                                                       , &pnode->hasMDR);
                if (rc != OK)
                {
                    goto mod_exit;
                }
            }

            //Update the reference and commit any changes from iecs_storeMessageDeliveryReference()
            rc = ism_store_updateReference( pThreadData->hStream
                                          , Q->QueueRefContext
                                          , pnode->hMsgRef
                                          , pnode->orderId
                                          , state
                                          , 0 ); // minimumActiveOrderId

            (*pStoreOpCount) ++; //It doesn't matter that iecs_storeMessageDeliveryReference() might have added ops
                                 //all we care about is whether != 0

            if (UNLIKELY(rc != OK))
            {
                // The failure to update a store reference (or commit the changes from
                // iecs_storeMessageDeliveryReference) means that the queue is inconsistent.
                ieutTRACE_FFDC( ieutPROBE_001, true
                              , "ism_store_updateReference failed.", rc
                              , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                              , "Queue", Q, sizeof(ieiqQueue_t)
                              , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                              , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                              , "pNode", pnode, sizeof(ieiqQNode_t)
                              , NULL );
            }
        }
    }
    else if (newMsgState == ismMESSAGE_STATE_CONSUMED)
    {
        if (pnode->inStore)
        {
            iest_AssertStoreCommitAllowed(pThreadData);

            // This delete reference will hopefully be part of the same
            // store transaction as the unstoreMessage.
            rc = ism_store_deleteReference( pThreadData->hStream
                                          , Q->QueueRefContext
                                          , pnode->hMsgRef
                                          , pnode->orderId
                                          , 0 );
            if (UNLIKELY(rc != OK))
            {
                // The failure to delete a store reference means that the
                // queue is inconsistent.
                ieutTRACE_FFDC( ieutPROBE_002, true
                              , "ism_store_deleteReference failed.", rc
                              , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                              , "Queue", Q, sizeof(ieiqQueue_t)
                              , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                              , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                              , "pNode", pnode, sizeof(ieiqQNode_t)
                              , NULL );
            }
            (*pStoreOpCount) ++;

            //We'll decrease our usecount on the message after we have committed
            //(in completePreparingMessage)
        }
    }
    else
    {
        ieutTRACEL(pThreadData, newMsgState, ENGINE_SEVERE_ERROR_TRACE,
                   "%s:  invalid msgState (%d)\n", __func__, newMsgState);
        assert(false);
    }

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


//NB: Wr're not currently updating the redelivery cursor here but as it's only used to redeliver messages
//..that should be ok
static void ieiq_deliverMessage( ieutThreadData_t *pThreadData
                               , ieiqQueue_t *Q
                               , ieiqQNode_t *pDelivery
                               , ismMessageState_t msgState
                               , uint32_t deliveryId
                               , ismEngine_Message_t *hmsg
                               , ismMessageHeader_t *pMsgHdr
                               , bool *pCompleteWaiterActions
                               , bool *pDeliverMoreMsgs
                               , ismEngine_DelivererContext_t * delivererContext )
{
    //Does the user explicitly call suspend or do we disable if the return code is false
    bool fExplicitSuspends = Q->pConsumer->pSession->fExplicitSuspends;
    iewsWaiterStatus_t oldStatus;

    *pCompleteWaiterActions = false;

    bool reenableWaiter = ism_engine_deliverMessage(
                            pThreadData,
                            Q->pConsumer,
                            pDelivery,
                            hmsg,
                            pMsgHdr,
                            msgState,
                            deliveryId,
                            delivererContext);

    if (reenableWaiter)
    {
        oldStatus = __sync_val_compare_and_swap(   &(Q->waiterStatus)
                                                 , IEWS_WAITERSTATUS_DELIVERING
                                                 , IEWS_WAITERSTATUS_ENABLED);

        if (oldStatus == IEWS_WAITERSTATUS_DELIVERING)
        {
            // We re-enabled it so it's our responsibility to
            //  check for more messages
            *pDeliverMoreMsgs = true;
        }
        else
        {
            *pCompleteWaiterActions = true; //Someone else has changed state, action their requests
        }
    }
    else
    {
        if (!fExplicitSuspends)
        {
            iews_addPendFlagWhileLocked( &(Q->waiterStatus)
                                       , IEWS_WAITERSTATUS_DISABLE_PEND);
        }
        //We need to call delivery callback etc...
        *pCompleteWaiterActions = true;
    }
}



///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Updates the message state to indicate delivery
///  @remarks
///    This function is called prior to delivering a message to the consumer
///    to indcate that it has been delivered. The 'msgState' parameter is
///    used to indicate of the message is being consumed (QoS 0) or just
///    delivered (QoS 1). If the message is being consumed then the head
///    of the queue may also be updated.
///
///  @param[in] Q                  - Queue to get the message from
///  @param[in] pnode              - A pointer to the node containing the msg
///  @param[in] updateDeliveryCount- Increment delivery count for message sent
///  @return                       - OK on success
//                                   ISMRC_NotDelivered - waiter not available
//                                   other ISMRC
///////////////////////////////////////////////////////////////////////////////
static uint32_t ieiq_redeliverMessage( ieutThreadData_t *pThreadData
                                     , ieiqQueue_t       *Q
                                     , ieiqQNode_t       *pnode
                                     , bool              updateDeliveryCount )
{
    uint32_t rc = OK;
    uint32_t state;
    iewsWaiterStatus_t oldStatus;
    ismMessageHeader_t msgHdr;
    bool deliverAnother = false;
    bool completeWaiterActions = false;

    ieutTRACEL(pThreadData, pnode->msg, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY " Q=%p pnode=%p pMessage=%p Reliability=%d\n",
               __func__, Q, pnode, pnode->msg, pnode->msg->Header.Reliability);

    // First try and take control of the waiter, as we can only deliver a
    // message if we are in control. Here we change straight to GOT state
    // (skipping GETTING)
    oldStatus =  __sync_val_compare_and_swap( &(Q->waiterStatus)
                                            , IEWS_WAITERSTATUS_ENABLED
                                            , IEWS_WAITERSTATUS_DELIVERING);
    if (oldStatus != IEWS_WAITERSTATUS_ENABLED)
    {
        // We could not take control of the consumer so we bail out.
        rc = ISMRC_NotDelivered;
        goto mod_exit;
    }

    // Next check that since we were asked to redeliver the message state
    // hasn't changed.
    if (pnode->msgState == ismMESSAGE_STATE_DELIVERED)
    {
        // The message is still marked as delivered (it has not been
        // acknoweldged in the meantime so go ahead and try and re-deliver
        // it.

        ismEngine_CheckStructId(pnode->msg->StrucId, ismENGINE_MESSAGE_STRUCID, ieutPROBE_034);

        // Copy the message header from the one published
        msgHdr = pnode->msg->Header;

        // If we have been asked to incrment the delivery count then do that
        // now. Otherwise set the delivery count in the message header to the
        // same value the last time it was sent.
        if (updateDeliveryCount)
        {
            // Add the delivery count from the node
            msgHdr.RedeliveryCount += pnode->deliveryCount;

            // Update the deliver count in the node,
            pnode->deliveryCount++;
        }
        else
        {
            // Add the delivery count from the node (minus 1)
            msgHdr.RedeliveryCount += (pnode->deliveryCount - 1);
        }

        // Ensure the delivery count does not exceedd the maximum
        if (msgHdr.RedeliveryCount > ieqENGINE_MAX_REDELIVERY_COUNT)
        {
            msgHdr.RedeliveryCount = ieqENGINE_MAX_REDELIVERY_COUNT;
        }

        // Include any additional flags
        msgHdr.Flags |= pnode->msgFlags;

        // And set the order ID on the message
        msgHdr.OrderId = (msgHdr.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT) ? pnode->orderId : 0;

        // Increase the use count of the message. The callback will be
        // responsible for performing the decrement.
        iem_recordMessageUsage(pnode->msg);

        // If we have updated the delivery count, and this message is in
        // the store then update the store
        if (updateDeliveryCount && pnode->inStore)
        {
            state = pnode->deliveryCount & 0x3f;
            state |= (pnode->msgState << 6);

            iest_AssertStoreCommitAllowed(pThreadData);

            rc = iest_store_updateReferenceCommit( pThreadData
                                                 , pThreadData->hStream
                                                 , Q->QueueRefContext
                                                 , pnode->hMsgRef
                                                 , pnode->orderId
                                                 , state
                                                 , 0 ); // minimumActiveOrderId

            if (rc != OK)
            {
                // The failure to update a store reference means that the
                // queue is inconsistent.
                ieutTRACE_FFDC( ieutPROBE_001, true
                              , "ism_store_updateReference failed.", rc
                              , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                              , "Queue", Q, sizeof(ieiqQueue_t)
                              , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                              , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                              , "pNode", pnode, sizeof(ieiqQNode_t)
                              , NULL );
            }
        }

        //Will attempt to unlock the waiter
        ieiq_deliverMessage( pThreadData
                           , Q
                           , pnode
                           , ismMESSAGE_STATE_DELIVERED
                           , pnode->deliveryId
                           , pnode->msg
                           , &msgHdr
                           , &completeWaiterActions
                           , &deliverAnother
                           , NULL);
    }
    else
    {
        // The message is has been changed from DELIVERED state so
        // it has probably been acknowledged. So we can ignore this
        // request for re-delivery but we will need to re-enable the
        // waiter and possibly re-check for any messages which have
        // arrived while we had the waiter locked.
        oldStatus = __sync_val_compare_and_swap( &(Q->waiterStatus)
                               , IEWS_WAITERSTATUS_DELIVERING
                               , IEWS_WAITERSTATUS_ENABLED);

        if (oldStatus != IEWS_WAITERSTATUS_DELIVERING)
        {
            completeWaiterActions = true;
        }
        else
        {
            deliverAnother = true;
        }
    }

    // If the waiter state was changed while we were
    if (completeWaiterActions)
    {
        ieq_completeWaiterActions(pThreadData
                                 , (ismEngine_Queue_t *)Q
                                 , Q->pConsumer
                                 , IEQ_COMPLETEWAITERACTION_OPTS_NONE
                                 , true);
    }
    else if (deliverAnother)
    {
        rc = ieiq_checkWaiters(pThreadData, (ismEngine_Queue_t *)Q, NULL, NULL);
    }

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

/////////////////////////////////////////////////////////////////////////////////////
/// @brief
///     If checkWaiters went async without being given an asyncInfo structure,
///     this function tidies up
//////////////////////////////////////////////////////////////////////////////////////
static int32_t ieiq_completeCheckWaiters(ieutThreadData_t           *pThreadData,
                                         int32_t                              rc,
                                         ismEngine_AsyncData_t        *asyncInfo,
                                         ismEngine_AsyncDataEntry_t     *context)
{
    assert(context->Type == ieiqQueueCompleteCheckWaiters);
    ismQHandle_t Qhdl = (ismQHandle_t)(context->Handle);

    ieutTRACEL(pThreadData, Qhdl, ENGINE_FNC_TRACE, FUNCTION_IDENT "Qhdl=%p\n", __func__,
                                                                  Qhdl);

    //Pop this of the list so we don't double decrement the predelete count
    iead_popAsyncCallback( asyncInfo, 0);
    //Undo the count we did just before the commit
    ieiq_reducePreDeleteCount(pThreadData, Qhdl);

    return rc;
}


static void ieiq_initialiseStackAsyncInfo(  ieiqQueue_t *Q
                                         ,  ismEngine_AsyncData_t *stackAsyncInfo
                                         ,  ismEngine_AsyncDataEntry_t localstackAsyncArray[])
{
    ismEngine_SetStructId(localstackAsyncArray[0].StrucId, ismENGINE_ASYNCDATAENTRY_STRUCID);
    localstackAsyncArray[0].Data = NULL;
    localstackAsyncArray[0].DataLen = 0;
    localstackAsyncArray[0].Handle = Q;
    localstackAsyncArray[0].Type = ieiqQueueCompleteCheckWaiters;
    localstackAsyncArray[0].pCallbackFn.internalFn = ieiq_completeCheckWaiters;

    ismEngine_SetStructId(stackAsyncInfo->StrucId, ismENGINE_ASYNCDATA_STRUCID);
    stackAsyncInfo->pClient             = NULL;
    stackAsyncInfo->numEntriesAllocated = IEAD_MAXARRAYENTRIES;
    stackAsyncInfo->numEntriesUsed      = 1;
    stackAsyncInfo->asyncId             = 0;
    stackAsyncInfo->fOnStack            = true;
    stackAsyncInfo->DataBufferAllocated = 0;
    stackAsyncInfo->DataBufferUsed      = 0;
    stackAsyncInfo->entries             = localstackAsyncArray;
}

static inline uint32_t ieiq_chooseDeliveryBatchSizeFromMaxInflight(uint32_t maxInflightLimit)
{
    uint32_t batchSize = maxInflightLimit >> 2; //4 batches before we saturate our limit and turn everything off

    if (batchSize == 0)
    {
        batchSize = 1;
    }
    else if (batchSize > IEIQ_MAXDELIVERYBATCH_SIZE)
    {
        batchSize = IEIQ_MAXDELIVERYBATCH_SIZE;
    }

    return batchSize;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Attempt to deliver messages to the consumer
///  @remarks
///    This function attempts to deliver message from the queue to the
///    consumer. In order to do this it must change the state of the
///    consumer from enabled to getting.
///    If the consumer is already in getting state then we loop round
///    until it's state changes.
///    If the consumer is in any other state then this function takes no
///    further action.
///
///  @param[in] q                  - Queue
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////

int32_t ieiq_checkWaiters( ieutThreadData_t *pThreadData
                         , ismEngine_Queue_t *Qhdl
                         , ismEngine_AsyncData_t * asyncInfo
                         , ismEngine_DelivererContext_t * delivererContext )
{
    int32_t rc = OK;
    bool loopAgain = true;
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    bool usedlocalStackAsyncInfo = false;
    uint32_t deliveryBatchSize   = 32; //Pick a size incase one isn't set.

    ieutTRACEL(pThreadData, Q,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY " Q=%p, async=%p\n", __func__, Q, asyncInfo);

    if (Q->bufferedMsgs == 0)
    {
        //There appear to be no messages on this queue, anyone who adds a message will
        //call checkWaiters themselves so we don't need to set a memory barrier on this number or
        //anything... we're done.
        goto mod_exit;
    }


    // We loop giving messages until we run out of messages to give or
    // the waiter is no longer enabled (e.g. because someone else has it
    // whilst they are in the message received callback)
    do
    {
        loopAgain = false;

        iewsWaiterStatus_t oldStatus =  __sync_val_compare_and_swap( &(Q->waiterStatus)
                                                                   , IEWS_WAITERSTATUS_ENABLED
                                                                   , IEWS_WAITERSTATUS_GETTING);

        if (oldStatus == IEWS_WAITERSTATUS_ENABLED)
        {
            //waiter is now in getting state for us to get
#ifdef HAINVESTIGATIONSTATS
            clock_gettime(CLOCK_REALTIME, &(Q->pConsumer->lastGetTime));

            if (Q->pConsumer->lastDeliverEnd.tv_sec != 0)
            {
                Q->pConsumer->lastGapBetweenGets = timespec_diff_int64(&(Q->pConsumer->lastDeliverEnd)
                                                                      ,&(Q->pConsumer->lastGetTime));
            }
            else
            {
                //first ever get... no sensible value... but don't use time since clock start...
                Q->pConsumer->lastGapBetweenGets = 0;
            }
            Q->pConsumer->lastDeliverCommitStart.tv_sec  = 0;
            Q->pConsumer->lastDeliverCommitStart.tv_nsec = 0;
            Q->pConsumer->lastDeliverCommitEnd.tv_sec = 0;
            Q->pConsumer->lastDeliverCommitEnd.tv_nsec = 0;
            Q->pConsumer->lastDeliverEnd.tv_sec = 0;
            Q->pConsumer->lastDeliverEnd.tv_nsec = 0;
            Q->pConsumer->lastBatchSize = 0;
#endif

            uint32_t perClientLimit = Q->pConsumer->pSession->pClient->maxInflightLimit;

            if (perClientLimit != 0)
            {
                deliveryBatchSize = ieiq_chooseDeliveryBatchSizeFromMaxInflight(perClientLimit);
            }

            ieiqAsyncMessageDeliveryInfo_t deliveryData; //Stores a lists of nodes to deliver
            ismEngine_SetStructId(deliveryData.StructId, IEIQ_ASYNCMESSAGEDELIVERYINFO_STRUCID);
            deliveryData.Q = Q;
            deliveryData.usedNodes = 0; //Not found any nodes to deliver yet
            uint64_t storeOps   = 0;

            // If someone else changes the waiter state after we moved
            // it out of enabled, we'll set this to true
            bool completeWaiterActions = false;

            iewsWaiterStatus_t currState = IEWS_WAITERSTATUS_GETTING;

            do
            {
                if (!(Q->Redelivering) && (Q->inflightDeqs >= Q->maxInflightDeqs))
                {
#ifdef HAINVESTIGATIONSTATS
                    pThreadData->stats.perConsumerFlowControlCount++;
#endif
                    rc = ISMRC_NoMsgAvail;
                }
                else
                {
                    rc = ieiq_locateMessage( pThreadData
                                           , Q
                                           , (deliveryData.usedNodes == 0)
                                           , &(deliveryData.pnodes[deliveryData.usedNodes]));
                }

                if (rc == OK)
                {
                    rc = ieiq_initialPrepareMessageForDelivery( pThreadData
                                                              , Q
                                                              , deliveryData.pnodes[deliveryData.usedNodes]
                                                              , &storeOps);

                    if (rc == OK)
                    {
                        // Move the getCursor onwards.
                        ieiq_moveGetCursor(pThreadData, Q, deliveryData.pnodes[deliveryData.usedNodes]);

                        if (deliveryData.usedNodes == 0)
                        {
                            // We've got a message for this waiter...change our state to
                            // reflect that. If someone else has disabled/enabled us...
                            // continue anyway for the moment. We'll notice when we try
                            // and enable/disable the waiter after the callback
                            __sync_bool_compare_and_swap( &(Q->waiterStatus)
                                                        , IEWS_WAITERSTATUS_GETTING
                                                        , IEWS_WAITERSTATUS_DELIVERING);
                            loopAgain = true; //If we're in delivering we promise to try get again afterwards
                            currState = IEWS_WAITERSTATUS_DELIVERING;
                        }

                        deliveryData.usedNodes++;
                    }
                }

            }
            while(rc == OK && deliveryData.usedNodes < deliveryBatchSize);


            if (rc == ISMRC_NoMsgAvail)
            {
                if (deliveryData.usedNodes == 0)
                {
                    // If we are supposed to be informing the engine when the
                    // queue hits empty, do it now. Note we have no barriers or
                    // atomic sets etc... so we /might/ make this call twice
                    // We don't need to worry about informing if we found some nodes...
                    // ..as after we deliver those nodes we'll check again
                    if (Q->Common.informOnEmpty)
                    {
                        Q->Common.informOnEmpty = false;
                        ism_engine_deliverStatus( pThreadData, Q->pConsumer, ISMRC_NoMsgAvail);
                    }

                    // If we reported queue full in the past, let's reset the flag
                    // now that the queue is empty again.
                    Q->ReportedQueueFull = false;
                }
                rc = OK; //No probs as we found some messages before we couldn't find one
            }
            else if (rc == ISMRC_MaxDeliveryIds)
            {
                rc = OK; // We still want to deliver the messages we have already batched

                //pause delivery for the whole client and restart it when we receive acks
                //We still have *this* waiter locked so can deliver messages in this batch
                rc = stopMessageDeliveryInternal(
                             pThreadData,
                             Q->pConsumer->pSession,
                             ISM_ENGINE_INTERNAL_STOPMESSAGEDELIVERY_FLAGS_ENGINE_STOP,
                             NULL, 0, NULL);

                if (rc == ISMRC_NotEngineControlled)
                {
                    //That's fine protocol/transport have change the start/stop state and we shouldn't fiddle
                    //Or... the engine controlled flag has been removed by an ack before we even did the
                    //disable.. both fine...we'll be retrying delivery
                    rc = OK;
                }
                else if (rc != OK)
                {
                    ieutTRACE_FFDC( ieutPROBE_001, true, "Failed to stop session.", rc
                            , "pSession", Q->pConsumer->pSession, sizeof(ismEngine_Session_t)
                            , NULL);
                }

                //Recheck in case we were reenabled whilst we still had consumer locked.
                loopAgain = true;
            }
            else if (rc == IMSRC_RecheckQueue)
            {
                loopAgain = true;
                rc = OK; // We still want to deliver the messages we have already batched
                         //(and then we'll check again)
            }
            else if (rc == ISMRC_AllocateError)
            {
                //We failed to prepare the message for delivery....
                //Put messages back on queue
                ieiq_undoInitialPrepareMessage( pThreadData
                                              , Q
                                              , false
                                              , deliveryData.usedNodes
                                              , deliveryData.pnodes
                                              , &storeOps);
                //Because we didn't commit our changes... we shouldn't need to do storeOps
                //to fix them
                assert (storeOps == 0);

                Q->resetCursor = true;

                //Disconnect the client if we can or come down in a flaming heap.
                //Should be in delivering to call the failure function...
                if (currState != IEWS_WAITERSTATUS_DELIVERING)
                {
                    __sync_bool_compare_and_swap( &(Q->waiterStatus)
                                                , IEWS_WAITERSTATUS_GETTING
                                                , IEWS_WAITERSTATUS_DELIVERING);
                }
                ieiq_handleDeliveryFailure(pThreadData, rc, Q);

                //We will have marked the waiter as disabled_pend... we need to sort it out
                completeWaiterActions = true;

                //Put messages back on queue
                deliveryData.usedNodes = 0;

                rc = OK; //We've dealt with the issue
            }
            else if (rc != OK)
            {
                ieutTRACE_FFDC( ieutPROBE_001, true, "ieiq_initPrepareMessageForDelivery failed.", rc
                        , NULL);
            }


            if ((rc == OK) && (deliveryData.usedNodes > 0))
            {
#ifdef HAINVESTIGATIONSTATS
                pThreadData->stats.avgDeliveryBatchSize = (   (  pThreadData->stats.avgDeliveryBatchSize
                                                               * pThreadData->stats.numDeliveryBatches)
                                                           +  deliveryData.usedNodes)
                                                         / (1.0+pThreadData->stats.numDeliveryBatches);
                pThreadData->stats.numDeliveryBatches++;

                if (deliveryData.usedNodes < pThreadData->stats.minDeliveryBatchSize)
                {
                    pThreadData->stats.minDeliveryBatchSize = deliveryData.usedNodes;
                }
                if (deliveryData.usedNodes > pThreadData->stats.maxDeliveryBatchSize)
                {
                    pThreadData->stats.maxDeliveryBatchSize = deliveryData.usedNodes;
                }

                Q->pConsumer->lastBatchSize = deliveryData.usedNodes;
#endif
                if (storeOps > 0)
                {
                    //We want to async whether our caller wants to or not. If not, we just don't tell them.
                    //As checkWaiters is something of a "black-box" to our caller, that shouldn't be a problem,
                    //However having a put call checkWaiters with an asyncInfo means that we can hold up the
                    //put for a while on a deep queue, giving some level of pacing
                    ismEngine_AsyncDataEntry_t localstackAsyncArray[IEAD_MAXARRAYENTRIES];
                    ismEngine_AsyncData_t localStackAsyncInfo;

                    if (asyncInfo == NULL)
                    {
                        ieutTRACE_HISTORYBUF(pThreadData, &localStackAsyncInfo);
                        usedlocalStackAsyncInfo = true;

                        ieiq_initialiseStackAsyncInfo(Q, &localStackAsyncInfo, localstackAsyncArray);

                        asyncInfo = &localStackAsyncInfo;

                        //We're not going to worry our caller with the info that we're going async, so
                        //we need to ensure the queue doesn't get deleted out from under us
                        DEBUG_ONLY uint64_t oldCount = __sync_fetch_and_add(&(Q->preDeleteCount), 1);
                        assert(oldCount > 0);
                    }

                    size_t dataSize =  offsetof(ieiqAsyncMessageDeliveryInfo_t, pnodes)
                                     +(deliveryData.usedNodes * sizeof(deliveryData.pnodes[0]));
                    ismEngine_AsyncDataEntry_t newEntry =
                         {ismENGINE_ASYNCDATAENTRY_STRUCID, ieiqQueueDeliver, &deliveryData, dataSize, NULL, {.internalFn = ieiq_asyncMessageDelivery}};

                    iead_pushAsyncCallback(pThreadData, asyncInfo, &newEntry);

#ifdef HAINVESTIGATIONSTATS
                    clock_gettime(CLOCK_REALTIME, &(Q->pConsumer->lastDeliverCommitStart));
#endif
                    rc = iead_store_asyncCommit(pThreadData, false, asyncInfo);

                    if (rc == ISMRC_AsyncCompletion)
                    {
                        //Gone async... get out of here

                        if (usedlocalStackAsyncInfo)
                        {
                            //Caller doesn't need to know that we went async
                            rc = OK;
                        }
                        goto mod_exit;
                    }
                    else
                    {
#ifdef HAINVESTIGATIONSTATS
                        clock_gettime(CLOCK_REALTIME, &(Q->pConsumer->lastDeliverCommitEnd));
#endif
                        //We didn't go async, we don't need the delivery callback to be on the asyncinfo stack
                        iead_popAsyncCallback(asyncInfo, newEntry.DataLen);

                        // We didn't go async, so we have no outstanding store operations
                        storeOps = 0;

                        if (usedlocalStackAsyncInfo)
                        {
                            asyncInfo = NULL;
                            usedlocalStackAsyncInfo = false;
                            //Undo the count we did just before the commit
                            ieiq_reducePreDeleteCount(pThreadData, Qhdl);
                        }

                        if ( rc != OK)
                        {
                            ieutTRACE_FFDC( ieutPROBE_003, true, "iead_store_commit failed.", rc
                                          , NULL);

                            goto mod_exit;
                        }
                    }
                }

                if (rc == OK)
                {
                    assert(currState == IEWS_WAITERSTATUS_DELIVERING);
                    assert(storeOps == 0);

                    ieiq_deliverMessages( pThreadData
                                        , Q
                                        , deliveryData.usedNodes
                                        , deliveryData.pnodes
                                        , &completeWaiterActions
                                        , &loopAgain
                                        , &storeOps
                                        , delivererContext );

                    if (storeOps > 0)
                    {
                        //Argh, we needed to undo half delivered messages... do that before
                        //we unlock waiter

                        //We want to async whether our caller wants to or not. If not, we just don't tell them.
                        //As checkWaiters is something of a "black-box" to our caller, that shouldn't be a problem,
                        //However having a put call checkWaiters with an asyncInfo means that we can hold up the
                        //put for a while on a deep queue, giving some level of pacing
                        ismEngine_AsyncDataEntry_t localstackAsyncArray[IEAD_MAXARRAYENTRIES];
                        ismEngine_AsyncData_t localStackAsyncInfo;

                        if (asyncInfo == NULL)
                        {
                            assert(usedlocalStackAsyncInfo == false);
                            ieutTRACE_HISTORYBUF(pThreadData, &localStackAsyncInfo);
                            usedlocalStackAsyncInfo = true;

                            ieiq_initialiseStackAsyncInfo(Q, &localStackAsyncInfo, localstackAsyncArray);

                            asyncInfo = &localStackAsyncInfo;

                            //We're not going to worry our caller with the info that we're going async, so
                            //we need to ensure the queue doesn't get deleted out from under us
                            DEBUG_ONLY uint64_t oldCount = __sync_fetch_and_add(&(Q->preDeleteCount), 1);
                            assert(oldCount > 0);
                        }

                        ismEngine_AsyncDataEntry_t newEntry =
                                       { ismENGINE_ASYNCDATAENTRY_STRUCID
                                       , ieiqQueueCancelDeliver
                                       , NULL, 0
                                       , Q /* all we need is the queue */
                                       , {.internalFn = ieiq_asyncCancelDelivery}};

                        iead_pushAsyncCallback(pThreadData, asyncInfo, &newEntry);

                        rc = iead_store_asyncCommit(pThreadData, false, asyncInfo);
                        assert (rc == OK || rc == ISMRC_AsyncCompletion);

                        if (rc == ISMRC_AsyncCompletion)
                        {
                            //Gone async... get out of here

                            if (usedlocalStackAsyncInfo)
                            {
                                //Caller doesn't need to know that we went async
                                rc = OK;
                            }
                            goto mod_exit;
                        }
                        else
                        {
                            //We didn't go async, we don't need the delivery callback to be on the asyncinfo stack
                            iead_popAsyncCallback(asyncInfo, newEntry.DataLen);

                            if (usedlocalStackAsyncInfo)
                            {
                                asyncInfo = NULL;
                                usedlocalStackAsyncInfo = false;
                                //Undo the count we did just before the commit
                                ieiq_reducePreDeleteCount(pThreadData, Qhdl);
                            }

                            if ( rc != OK)
                            {
                                ieutTRACE_FFDC( ieutPROBE_004, true, "iead_store_commit failed.", rc
                                        , NULL);

                                goto mod_exit;
                            }
                        }
                    }
                }
                else
                {
                    //Deal with the messages once the store write is complete
                    assert(rc == ISMRC_AsyncCompletion);
                    goto mod_exit;
                }
            }
            else if (!completeWaiterActions)
            {
                //We have the waiter locked in getting but aren't going to deliver messages
                iewsWaiterStatus_t oldState = __sync_val_compare_and_swap( &(Q->waiterStatus)
                                                  , currState
                                                  , IEWS_WAITERSTATUS_ENABLED);

                if (oldState != currState)
                {
                    //Other people have added actions for us to do...
                    completeWaiterActions = true;
                }
            }

            if (completeWaiterActions)
            {
                //Someone requested a callback whilst we had the waiter in getting or got,
                //It's our responsibility to fire the callback(s)
                ieq_completeWaiterActions( pThreadData
                                         , (ismEngine_Queue_t *)Q
                                         , Q->pConsumer
                                         , IEQ_COMPLETEWAITERACTION_OPT_NODELIVER
                                         , true);

                //We asked completeWA not to do deliver...we need to check again...
                loopAgain = true;
            }
        }
        else if (oldStatus == IEWS_WAITERSTATUS_GETTING)
        {
            // We've found a valid waiter, his get might fail as it was
            // fractionally too early whereas ours would succeed so we'll
            // loop around again and give him another chance
            loopAgain = true;
        }
        else
        {
            ieutTRACEL(pThreadData, oldStatus, ENGINE_HIFREQ_FNC_TRACE,
                       "Not delivering message non-enabled waiter (Status=%lu)\n",
                       oldStatus);
        }
    }
    while ((rc == OK) && loopAgain);

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Attempt to deliver message directly to consumer
///  @remarks
///    This function is called directly from the ieiq_putMessage function
///    to attempt to deliver a QoS 0 message directly to the conusmer. The
///    consumer must be in enabled state in order for the message to be
///    delivered.
///
///  @param[in] Q                  - Queue
///  @param[in] msg                - Message
///  @return                       - OK or ISMRC_NoAvailWaiter
///////////////////////////////////////////////////////////////////////////////
static int32_t ieiq_putToWaitingGetter( ieutThreadData_t *pThreadData
                                      , ieiqQueue_t *Q
                                      , ismEngine_Message_t *msg
                                      , uint8_t msgFlags
                                      , ismEngine_DelivererContext_t * delivererContext )
{
    int32_t rc = OK;
    bool deliveredMessage = false;
    bool loopAgain = true;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p\n", __func__, Q);

    do
    {
        loopAgain = false;

        iewsWaiterStatus_t oldStatus =  __sync_val_compare_and_swap( &(Q->waiterStatus)
                                                                   , IEWS_WAITERSTATUS_ENABLED
                                                                   , IEWS_WAITERSTATUS_DELIVERING);

        if(oldStatus == IEWS_WAITERSTATUS_ENABLED)
        {
            bool reenableWaiter = false;
            ismMessageHeader_t msgHdr = msg->Header;

            // Include any additional flags
            msgHdr.Flags |= msgFlags;

            // Update the record for the number of messages delivered directly
            // to the consumer
            Q->qavoidCount++;

            //Does the user explicitly call suspend or do we disable if the return code is false
            bool fExplicitSuspends = Q->pConsumer->pSession->fExplicitSuspends;

            reenableWaiter = ism_engine_deliverMessage(
                      pThreadData
                    , Q->pConsumer
                    , NULL
                    , msg
                    , &msgHdr
                    , ismMESSAGE_STATE_CONSUMED
                    , 0
                    , delivererContext);

            if (reenableWaiter)
            {
                oldStatus = __sync_val_compare_and_swap( &(Q->waiterStatus)
                                                       , IEWS_WAITERSTATUS_DELIVERING
                                                       , IEWS_WAITERSTATUS_ENABLED );

                // If someone requested a callback whilst we had
                // the waiter in state: got, It's our responsibility to fire
                // the callback
                if (oldStatus != IEWS_WAITERSTATUS_DELIVERING)
                {
                    ieq_completeWaiterActions( pThreadData
                                             , (ismEngine_Queue_t *)Q
                                             , Q->pConsumer
                                             , IEQ_COMPLETEWAITERACTION_OPT_NODELIVER //We're going to call checkWaiters
                                             , true);
                }
            }
            else
            {
                if (!fExplicitSuspends)
                {
                    iews_addPendFlagWhileLocked( &(Q->waiterStatus)
                                               , IEWS_WAITERSTATUS_DISABLE_PEND);
                }
                // Fire the callback...
                ieq_completeWaiterActions( pThreadData
                                         , (ismEngine_Queue_t *)Q
                                         , Q->pConsumer
                                         , IEQ_COMPLETEWAITERACTION_OPT_NODELIVER //We're going to call checkWaiters
                                         , true);
            }

            //We've successfully delivered the message, we're done
            deliveredMessage = true;
            break;
        }
        else if (oldStatus == IEWS_WAITERSTATUS_GETTING)
        {
            // We've found a valid waiter, his get might fail as it was
            // fractionally too early whereas ours would succeed so we'll
            // loop around again and give him another chance
            loopAgain = true;
        }
    }
    while (loopAgain);

    // While we had the waiter in got state it may have missed puts so
    // we need to check waiters
    if (deliveredMessage)
    {
        if (Q->bufferedMsgs > 0)
        {
            // The only valid return codes from this function is OK
            // or ISMRC_NoAvailWaiter. If checkWaiters encounters a
            // problem we do not care.
            (void) ieiq_checkWaiters(pThreadData, (ismEngine_Queue_t *)Q, NULL, NULL);
        }
    }
    else
    {
        rc = ISMRC_NoAvailWaiter;
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d,Q=%p\n", __func__, rc, Q);
    return rc;
}

//The clientId can be passed in if known but it is just as optimisation as we can (slowly) find the
//client from the Q handle
static inline void ieiq_checkForNonAckers( ieutThreadData_t  *pThreadData
                                         , ieiqQueue_t       *Q
                                         , uint64_t           headOrderId
                                         , const char        *clientId)
{
    //We've cleaned up as much as we can... are there pages at the start of this queue stuck there because they
    //haven't been acked? If they don't ack for long enough, we're going to disconnect them and delete all their state
    //(including this queue)
    
    if (headOrderId != 0)
    {
        //Even for unlimited Qs you have to ack your messages before this many messages are put afterwards
        #define IEIQ_MAX_UNACKED_FOR_UNLIMITEDQ   1000000000

        uint64_t ratio;

        //added 1 to denominator to avoid divide by 0
        uint64_t policyMaxMsgCount = Q->Common.PolicyInfo->maxMessageCount;
        uint64_t maxMsgCount = policyMaxMsgCount;

        if (Q->bufferedMsgs > maxMsgCount)
        {
            //If we're not reclaming space fast enough, don't penalise some poor client
            maxMsgCount = Q->bufferedMsgs;
        }

        ratio = (Q->nextOrderId - headOrderId)/(1+maxMsgCount);

        if (   (    (policyMaxMsgCount > 0)
                 && (ratio > ismEngine_serverGlobal.DestroyNonAckerRatio)
                 && (ismEngine_serverGlobal.DestroyNonAckerRatio > 0) )
            || (    (policyMaxMsgCount == 0)
                 && (Q->bufferedMsgs > IEIQ_MAX_UNACKED_FOR_UNLIMITEDQ)
                 && (ismEngine_serverGlobal.DestroyNonAckerRatio > 0) ) )
        {
            //Things are really severe we're going to delete all the state for the client
            iecs_discardClientForUnreleasedMessageDeliveryReference( pThreadData
                                                                   , (ismQHandle_t)Q
                                                                   , NULL
                                                                   , clientId);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Attempts to free NodePages from the head of the queue
///  @remarks
///    This function is called to unlink and free node pages at the
///    head of a queue when they no longer contain messages.
///
///  @param[in] Q                  - Queue to get the message from
///////////////////////////////////////////////////////////////////////////////
static void ieiq_cleanupHeadPage( ieutThreadData_t *pThreadData, ieiqQueue_t       *Q )
{
    ieiqQNode_t *pnextNode;
    uint32_t storedMsgCount;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, Q,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n", __func__, Q);

    ieiq_getHeadLock(Q);

    // First do a quick check on the last message in the head page to see
    // if it worth us trying to cleanup
    if  (Q->headPage->nodes[Q->headPage->nodesInPage-1].msgState == ismMESSAGE_STATE_CONSUMED)
    {
        // Now attempt to free all pages at the start of the queue which
        // contain no messages
        for (;;)
        {
            storedMsgCount=0;

            for (pnextNode = Q->head; pnextNode->msgState == ismMESSAGE_STATE_CONSUMED; pnextNode++)
            {
                // also count how many store references we have freed from
                // the page we may be about to delete
                if (pnextNode->inStore)
                    storedMsgCount++;
            }
            Q->deletedStoreRefCount +=storedMsgCount;

            if (pnextNode->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
            {
                // If it's time to update the minimum active orderid, do so now
                if ((ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_1] != StatusOk) ||
                    Q->deletedStoreRefCount > ieqMINACTIVEORDERID_UPDATE_INTERVAL)
                {
                    uint64_t minimumActiveOrderId = (pnextNode-1)->orderId+1;

                    ieutTRACEL(pThreadData, minimumActiveOrderId, ENGINE_HIFREQ_FNC_TRACE,
                               "Pruning references for Q[0x%lx] minimumActiveOrderId %lu\n",
                               Q->hStoreObj, minimumActiveOrderId);

                    // Update min active orderId
                    uint32_t rc = ism_store_setMinActiveOrderId( pThreadData->hStream,
                                                                 Q->QueueRefContext,
                                                                 minimumActiveOrderId );
                    if (rc != OK)
                    {
                        ieutTRACE_FFDC( ieutPROBE_001, true, "ism_store_setMinActiveOrderId failed.", rc
                                      , "minActiveOrderId", &(minimumActiveOrderId), sizeof(uint64_t)
                                      , NULL);
                    }

                    Q->deletedStoreRefCount = 0;
                }

                ieiqQNodePage_t *pageToFree = Q->headPage;
                ieiqQNodePage_t *nextPage = pageToFree->next;

                if ((pageToFree->nextStatus == cursor_clear) &&
                    (nextPage != NULL))
                {
                    iere_primeThreadCache(pThreadData, resourceSet);

                    Q->headPage = nextPage;
                    Q->head = &(nextPage->nodes[0]);

                    iere_freeStruct(pThreadData, resourceSet, iemem_intermediateQPage, pageToFree, pageToFree->StrucId);
                }
                else
                {
                    break; // Break out of infinite for loop
                }
            }
            else
            {
                // Update head to point to next unconsumed message
                Q->head = pnextNode;
                break; // Break out of infinite for loop
            }
        }
    }

    uint64_t headOrderId = Q->head->orderId; //Used to check the queue is not too sparse in a sec...
    ieiq_releaseHeadLock(Q);

    //If the client for this subscription doesn't ack, eventually we will kick them off and destroy their state.
    ieiq_checkForNonAckers(pThreadData, Q, headOrderId, NULL);


    ieutTRACEL(pThreadData, Q, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return;
}

static inline uint32_t ieiq_choosePageSize( void )
{
    return ((ismEngine_serverGlobal.totalSubsCount > ieqNUMSUBS_HIGH_CAPACITY_THRESHOLD) ?
                               ieiqPAGESIZE_HIGHCAPACITY : ieiqPAGESIZE_DEFAULT);
}

//Must be called with the waiter locked!
static inline void ieiq_resetCursor(ieutThreadData_t *pThreadData, ieiqQueue_t *Q)
{
    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p, id: %u\n", __func__, Q, Q->qId);
    assert(Q->resetCursor); //Check we are only called when the cursor reset is needed

    ieiq_getHeadLock(Q);

    Q->resetCursor = false;

    // If the cursor had moved off of this page, mark it and
    // every subsequent page as 'completed'
    ieiqQNodePage_t  *currpage = Q->headPage;
    while (currpage && (currpage->nextStatus == cursor_clear))
    {
        currpage->nextStatus = completed;
        currpage=currpage->next;
    }

    // Ensure we set the resetCursor flag to false before we actually
    // reset the cursor position (as if these calls are executed in
    // the wrong order we could miss a request for us to reset the
    // cursor).
    ismEngine_WriteMemoryBarrier();

    Q->cursor = Q->head;

    ieiq_releaseHeadLock(Q);

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

//Must be called with the waiter locked and to a node that is not consumed
//Be very careful calling this function that there can't be a earlier node
//we need to rewind to and that the target node isn't consumed
static inline void ieiq_rewindCursorToNode( ieutThreadData_t *pThreadData
                                          , ieiqQueue_t *Q
                                          , ieiqQNode_t *newCursor)
{
    assert(newCursor->msgState != ismMESSAGE_STATE_CONSUMED);

    Q->cursor = newCursor;

    //We need to clear the cursor cleared flag from any pages on/after
    //we're now pointing to...
    //...first find the page head (by find the page end)
    ieiqQNode_t *tmp = newCursor+1;

    while(tmp->msgState != ieqMESSAGE_STATE_END_OF_PAGE)
    {
        tmp++;
    }
    ieiqQNodePage_t *currpage=ieiq_getPageFromEnd(tmp);

    while (currpage && (currpage->nextStatus == cursor_clear))
    {
        currpage->nextStatus = completed;
        currpage=currpage->next;
    }
}

//Must be removed from the store and have had any deliveryId removed before
//this function is called
static inline void ieiq_completeConsumeMessage( ieutThreadData_t *pThreadData
                                              , ieiqQueue_t *Q
                                              , ieiqQNode_t *pnode
                                              , bool *pPageCleanupNeeded)
{
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;
    uint32_t needStoreCommit = 0;

    // Decrement our reference to the msg and possibly schedule it's later deletion from the store
    // from the store.
    if (pnode->inStore)
    {
        iest_unstoreMessage( pThreadData
                           , pnode->msg
                           , false
                           , true
                           , NULL
                           , &needStoreCommit);

        assert(needStoreCommit == 0); //We shouldn't need a store commit as the unstoreMessage should be done using lazy deletion
    }
    // Before releasing our hold on the node, should we subsequently call
    // cleanupHeadPage?
    *pPageCleanupNeeded = ((pnode+1)->msgState == ieqMESSAGE_STATE_END_OF_PAGE);


    ismEngine_Message_t *pMsg = pnode->msg;
    pnode->msg = MESSAGE_STATUS_EMPTY;

    iem_releaseMessage(pThreadData, pMsg);

    //Once we set the node to consumed, we can no longer access it
    pnode->msgState = ismMESSAGE_STATE_CONSUMED;

    iere_primeThreadCache(pThreadData, resourceSet);
    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -1);
    pThreadData->stats.bufferedMsgCount--;
    DEBUG_ONLY int32_t oldDepth = __sync_fetch_and_sub(&(Q->bufferedMsgs), 1);
    assert(oldDepth > 0);
}

//Called after the message ref (and any MDR) have been removed by a store commit.
//(So amongst other things it can schedule the lazy removal of the message record if necessary)
int32_t ieiq_consumeAckCommitted(
                ieutThreadData_t               *pThreadData,
                int32_t                         retcode,
                ismEngine_AsyncData_t          *asyncinfo,
                ismEngine_AsyncDataEntry_t     *context)
{
    ieiqConsumeMessageAsyncData_t *asyncData = (ieiqConsumeMessageAsyncData_t *)context->Data;
    bool pageCleanupNeeded    = false;
    //The clientstate layer will have told us if we can have more inflight...act on it (but don't
    //pass the return code up)
    bool deliveryIdsAvailable = asyncData->deliveryIdsAvailable || (retcode == ISMRC_DeliveryIdAvailable);

    ismEngine_CheckStructId(asyncData->StructId, IEIQ_CONSUMEMESSAGE_ASYNCDATA_STRUCID, ieutPROBE_001);
    assert(retcode == OK || retcode == ISMRC_DeliveryIdAvailable); //Commits shall not fail!


    ieiq_completeConsumeMessage( pThreadData
                               , asyncData->Q
                               , asyncData->pnode
                               , &pageCleanupNeeded );

    ieiq_completeConsumeAck ( pThreadData
                            , asyncData->Q
                            , asyncData->pSession
                            , pageCleanupNeeded
                            , deliveryIdsAvailable);

    //Ensure if we go async in the (below) Lazy Message check... we don't try and do this callback again.
    iead_popAsyncCallback( asyncinfo, context->DataLen);

    int32_t rc = iest_checkLazyMessages( pThreadData
                                       , asyncinfo);

   return rc;
}

//Called after the message has been marked received in the store so now we can update the in memory copy.
int32_t ieiq_receiveAckCommitted(
                ieutThreadData_t               *pThreadData,
                int32_t                         retcode,
                ismEngine_AsyncData_t          *asyncinfo,
                ismEngine_AsyncDataEntry_t     *context)
{
    assert(context->Data == NULL);
    //We stored the ptr to the node in the handle as we just needed a single pointer
    //and therefore we don't need to memcpy a block of mem...
    ieiqQNode_t *pnode = (ieiqQNode_t *)(context->Handle);
    
    //Mark the message received in memory
    ieiq_completeReceiveAck(pThreadData, pnode);
    
    //Ensure if we go async... we don't try and do this callback again.
    iead_popAsyncCallback( asyncinfo, 0);

   return retcode;
}

static inline int32_t ieiq_consumeMessage( ieutThreadData_t *pThreadData
                                         , ieiqQueue_t *Q
                                         , ieiqQNode_t *pnode
                                         , bool *pPageCleanupNeeded
                                         , bool *pDeliveryIdsAvailable
                                         , ismEngine_AsyncData_t *asyncInfo)
{
    uint32_t storeOps = 0;
    int32_t rc = 0;
    ieutTRACEL(pThreadData, pnode->orderId, ENGINE_HIFREQ_FNC_TRACE, "pnode %p, oId %lu, msg %p state %u\n",
                       pnode, pnode->orderId, pnode->msg, pnode->msgState);

    if (pnode->inStore)
    {
        ieutTRACE_HISTORYBUF(pThreadData, pnode->hMsgRef);

        // Delete the reference to the message from the queue
        rc = ism_store_deleteReference( pThreadData->hStream
                                      , Q->QueueRefContext
                                      , pnode->hMsgRef
                                      , pnode->orderId
                                      , 0 );
        if (rc != OK)
        {
            // The failure to delete a store reference means that the
            // queue is inconsistent.
            ieutTRACE_FFDC( ieutPROBE_002, true, "ism_store_deleteReference failed.", rc
                          , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                          , "Queue", Q, sizeof(ieiqQueue_t)
                          , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                          , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                          , "pNode", pnode, sizeof(ieiqQNode_t)
                          , NULL );

        }
        storeOps++;

        // Delete any MDR for this message
        if (pnode->hasMDR)
        {
            pnode->hasMDR = false;

            //This function issues a store commit...could replace with start/finish unstore
            //that don't
            rc = iecs_unstoreMessageDeliveryReference( pThreadData
                                                     , pnode->msg
                                                     , Q->hMsgDelInfo
                                                     , pnode->deliveryId
                                                     , &storeOps
                                                     , pDeliveryIdsAvailable
                                                     , asyncInfo);

            if (rc != OK)
            {
                if (rc == ISMRC_AsyncCompletion)
                {
                    goto mod_exit; //everything else will happen in the callback
                }
                else if (rc == ISMRC_NotFound)
                {
                    // If we didn't have the MDR, that's a bit surprising but we
                    // were trying to remove something that's not there. Don't panic.
                    rc = OK;
                }
                else
                {
                    ieutTRACE_FFDC( ieutPROBE_003, true
                                  , "iecs_unstoreMessageDeliveryReference failed.", rc
                                  , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                                  , "Queue", Q, sizeof(ieiqQueue_t)
                                  , "Delivery Id", &pnode->deliveryId, sizeof(pnode->deliveryId)
                                  , "Order Id", &pnode->orderId, sizeof(pnode->orderId)
                                  , "pNode", pnode, sizeof(ieiqQNode_t)
                                  , NULL );

                }
            }
            //Now the node removal has been committed, we are allowed to unstore the message..
        }
        else
        {
            // We need to commit the removal of the reference before we can call iest_unstoreMessage (in case we reduce the usage
            // count to 1, some else decreases it to 0 and commits but we don't commit our transaction. On restart we'd point to a message
            // that has been deleted...
            rc = iead_store_asyncCommit(pThreadData, false, asyncInfo);
            assert (rc == ISMRC_OK || rc == ISMRC_AsyncCompletion);

            if (rc != OK)
            {
                goto mod_exit;
            }

            storeOps = 0;
        }
    }
    else
    {
        if (pnode->deliveryId > 0)
        {
            // Need to free up the deliveryId
            rc = iecs_releaseDeliveryId( pThreadData, Q->hMsgDelInfo, pnode->deliveryId );

            if (rc != OK)
            {
                if (rc == ISMRC_DeliveryIdAvailable)
                {
                    *pDeliveryIdsAvailable = true;
                    rc = OK;
                }
                else if(rc == ISMRC_NotFound)
                {
                    // We were trying to remove something that's not there. Don't panic. (release will have written an FFDC and continued)
                    rc = OK;
                }
                else
                {
                    ieutTRACE_FFDC( ieutPROBE_004, true
                            , "iecs_releaseDeliveryId failed for free deliveryId case.", rc
                            , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                            , "Queue", Q, sizeof(ieiqQueue_t)
                            , "Delivery Id", &pnode->deliveryId, sizeof(pnode->deliveryId)
                            , "Order Id", &pnode->orderId, sizeof(pnode->orderId)
                            , "pNode", pnode, sizeof(ieiqQNode_t)
                            , NULL );
                }
            }
        }
    }

    ieiq_completeConsumeMessage( pThreadData
                               , Q
                               , pnode
                               , pPageCleanupNeeded);
mod_exit:
    return rc;
}


//The queue has already been marked deleted so most failures can be corrected
//at restart...
static void ieiq_completeDeletion(ieutThreadData_t *pThreadData, ieiqQueue_t *Q)
{
#define DELETE_BATCH_SIZE 1000

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p, id: %u\n", __func__, Q, Q->qId);
    int32_t rc = OK;
    int32_t rc2 = OK;
    uint32_t commitBatchSize = 0;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    assert(Q->isDeleted);
    assert(!(Q->deletionCompleted));
    Q->deletionCompleted = true;

    // If this queue has a message-delivery state, we need to remove this
    // before we delete the SDR so the MDRs aren't orphaned
    if (Q->hMsgDelInfo != NULL)
    {
        if (Q->deletionRemovesStoreObjects)
        {
            if (Q->hStoreObj != ismSTORE_NULL_HANDLE)
            {
                rc2 = iecs_releaseAllMessageDeliveryReferences(pThreadData, Q->hMsgDelInfo, Q->hStoreObj, false);
            }
            else
            {
                rc2 = iecs_releaseAllMessageDeliveryReferences(pThreadData, Q->hMsgDelInfo, (ismStore_Handle_t)Q, true);
            }
            if (rc2 != OK)
            {
                ieutTRACEL(pThreadData, rc2, ENGINE_ERROR_TRACE,
                           "%s: iecs_releaseAllMessageDeliveryReferences (%s) failed! (rc=%d)\n",
                           __func__, Q->InternalName, rc2);
            }
        }

        iecs_releaseMessageDeliveryInfoReference(pThreadData, Q->hMsgDelInfo);
        Q->hMsgDelInfo = NULL;
    }

    //Remove the per-queue expiry data
    ieme_freeQExpiryData(pThreadData, (ismEngine_Queue_t *)Q);

    // Close the queue reference context before we (possibly) delete the
    // queue record
    if (Q->QueueRefContext != NULL)
    {
        rc = ism_store_closeReferenceContext(Q->QueueRefContext);
        if (UNLIKELY(rc != OK))
        {
            ieutTRACE_FFDC( ieutPROBE_001, false
                          , "Failed to close ReferenceContext for Queue .", rc
                          , "Q->QueueRefContext", &(Q->QueueRefContext), sizeof(void *)
                          , "Queue", Q, sizeof(ieiqQueue_t)
                          , NULL );
        }
        Q->QueueRefContext = NULL;
    }

    // Next delete the store definition record associated with this queue
    // (which may be a QDR or an SDR) and its associated properties record
    // (a QPR or an SPR).
    // This will implicitly delete all of the references to messages.
    if (Q->hStoreObj != ismSTORE_NULL_HANDLE)
    {
        assert(Q->hStoreProps != ismSTORE_NULL_HANDLE);

        if (Q->deletionRemovesStoreObjects)
        {
            rc = ism_store_deleteRecord(pThreadData->hStream, Q->hStoreObj);
            if (rc != OK)
            {
                ieutTRACEL(pThreadData, rc, ENGINE_ERROR_TRACE,
                           "%s: ism_store_deleteRecord (%s) failed! (rc=%d)\n",
                           __func__, Q->InternalName, rc);
            }
            rc = ism_store_deleteRecord(pThreadData->hStream, Q->hStoreProps);
            if (rc != OK)
            {
                ieutTRACEL(pThreadData, rc, ENGINE_ERROR_TRACE,
                           "%s: ism_store_deleteRecord (+%s:0x%0lx) failed! (rc=%d)\n",
                           __func__, (Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE) ? "SPR" : "QPR",
                           Q->hStoreProps, rc);
            }

            iest_store_commit(pThreadData, false);
        }
    }

    // Next free the pages, and messages
    while (Q->headPage != NULL)
    {
        ieiqQNodePage_t *pageToFree = NULL;
        ieiqQNode_t *temp = Q->head;

        // Move the head to the next item..
        (Q->head)++;

        //Have we gone off the bottom of the page?
        if(Q->head->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
        {
            pageToFree = Q->headPage;
            ieiqQNodePage_t *nextPage = pageToFree->next;

            if( nextPage != NULL)
            {
                Q->headPage = nextPage;
                Q->head = &(nextPage->nodes[0]);
            }
            else
            {
                Q->headPage = NULL;
                Q->head = NULL;
            }
        }

        if(temp->msg != MESSAGE_STATUS_EMPTY)
        {
            // First if we put the message in the store, then also
            // decrement the store reference to the message
            if (Q->deletionRemovesStoreObjects && (temp->inStore))
            {
                iest_unstoreMessage( pThreadData, temp->msg, false, false, NULL, &commitBatchSize );

                if (commitBatchSize >= DELETE_BATCH_SIZE)
                {
                    // We commit in batches even though the store should be
                    // able to handle large transactions there is no value
                    // in pushing too hard here.
                    iest_store_commit( pThreadData, false );
                    commitBatchSize = 0;
                }
            }

            // Then release our reference to the message memory
            iem_releaseMessage(pThreadData, temp->msg);
        }

        if(pageToFree != NULL)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_freeStruct(pThreadData, resourceSet, iemem_intermediateQPage, pageToFree, pageToFree->StrucId);
        }
    }

    if (commitBatchSize)
    {
        iest_store_commit( pThreadData, false );
    }

    int32_t os_rc = ieiq_destroyHeadLock(Q);

    if (os_rc != OK)
    {
        ieutTRACEL(pThreadData, os_rc, ENGINE_ERROR_TRACE,
                   "%s: destroy headlock failed! (os_rc=%d)\n",
                   __func__, os_rc);
    }

    os_rc = ieiq_destroyPutLock(Q);

    if (os_rc != OK)
    {
        ieutTRACEL(pThreadData, os_rc, ENGINE_ERROR_TRACE, "%s: destroy putlock failed! (os_rc=%d)\n",
                   __func__, os_rc);
    }

    // Release our use of the messaging policy info
    iepi_releasePolicyInfo(pThreadData, Q->Common.PolicyInfo);

    //All the messages that were buffered on this queue need to be removed from the global
    //buffered messages count
    iere_primeThreadCache(pThreadData, resourceSet);
    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -(int64_t)(Q->bufferedMsgs));
    pThreadData->stats.bufferedMsgCount -= Q->bufferedMsgs;

    if (Q->Common.QName != NULL)
    {
        iere_free(pThreadData, resourceSet, iemem_intermediateQ, Q->Common.QName);
    }

    iere_freeStruct(pThreadData, resourceSet, iemem_intermediateQ, Q, Q->Common.StrucId);

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return;
}


//Getting the headlock is an inline function....wrapper it so it can be called
//from export.
void ieiq_getHeadLock_ext(ieiqQueue_t *Q)
{
    ieiq_getHeadLock(Q);
}

//Releasing the headlock is an inline function....wrapper it so it can be called
//from export.
void ieiq_releaseHeadLock_ext(ieiqQueue_t *Q)
{
    ieiq_releaseHeadLock(Q);
}

//Must be called with headlock locked. Use for export (amongst other things?)
ieiqQNode_t *ieiq_getNextNodeFromPageEnd( ieutThreadData_t *pThreadData
                                        , ieiqQueue_t *Q
                                        , ieiqQNode_t *currNode)
{
    assert(currNode->msgState == ieqMESSAGE_STATE_END_OF_PAGE);

    ieiqQNodePage_t *currpage = ieiq_getPageFromEnd(currNode);
    ieiqQNode_t *nextNode = NULL;

    if (currpage->next != NULL)
    {
        nextNode = &(currpage->next->nodes[0]);
    }

    return nextNode;
}

//Leaves uncommitted store tran that may have msgref and/or message
int32_t ieiq_importQNode( ieutThreadData_t *pThreadData
                        , ieiqQueue_t *Q
                        , ismEngine_Message_t *inmsg
                        , ismMessageState_t  msgState
                        , uint64_t orderId
                        , uint32_t deliveryId
                        , uint8_t deliveryCount
                        , uint8_t msgFlags
                        , bool hadMDR
                        , bool wasInStore
                        , ieiqQNode_t **ppNode)
{
    int32_t rc = OK;
    ieiqQNode_t *pNode;
    ismStore_Reference_t msgRef;
    bool msgInStore = false;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, inmsg, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p msg=%p Length=%ld Reliability=%d\n",
               __func__, Q, inmsg, inmsg->MsgLength, inmsg->Header.Reliability);


    iere_updateMessageResourceSet(pThreadData, resourceSet, inmsg, true, false);
    iem_recordMessageUsage(inmsg);
    bool increasedUsage = true;

    bool existingStoreTran = true;

    rc = ieiq_preparePutStoreData( pThreadData
                                 , Q
                                 , ieqPutOptions_NONE
                                 , inmsg
                                 , msgState
                                 , msgFlags
                                 , existingStoreTran
                                 , &msgRef
                                 , &msgInStore);

    if (UNLIKELY(rc != OK))
    {
        //Will have already traced issue
        goto mod_exit;
    }

    assert(msgInStore == wasInStore);

    rc = ieiq_assignQSlot( pThreadData
                         , Q
                         , false
                         , orderId
                         , msgFlags
                         , &msgRef
                         , &pNode );

    if (UNLIKELY(rc != OK))
    {
        goto mod_exit;
    }

    //Fix things up so the things not normally set in a put are made
    //to look like they were prior to the export
    pNode->deliveryCount = deliveryCount;
    pNode->deliveryId    = deliveryId;
    pNode->hasMDR        = hadMDR;
    pNode->msgState      = msgState;

    rc = ieiq_finishPut( pThreadData
                       , Q
                       , NULL
                       , inmsg
                       , pNode
                       , &msgRef
                       , existingStoreTran
                       , msgInStore);


mod_exit:
    if (rc == OK)
    {
        *ppNode = pNode;
    }
    else if (increasedUsage)
    {
        iem_releaseMessage(pThreadData, inmsg);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//Get for consumer connected to the queue (if any)
int32_t ieiq_getConsumerStats( ieutThreadData_t *pThreadData
                             , ismQHandle_t Qhdl
                             , iempMemPoolHandle_t memPoolHdl
                             , size_t *pNumConsumers
                             , ieqConsumerStats_t consDataArray[])
{
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    int32_t rc = OK;
    bool lockedConsumer = false;

    //Lock the consumer so we can walk the pages safely
    iewsWaiterStatus_t preLockedState = IEWS_WAITERSTATUS_DISCONNECTED;
    iewsWaiterStatus_t lockedState;

    if (Q->pConsumer == NULL)
    {
        *pNumConsumers = 0;
        goto mod_exit;
    }

    lockedConsumer = iews_getLockForQOperation( pThreadData
                                              , &(Q->waiterStatus)
                                              , (3 * 1000000000L) //3 seconds
                                              , &preLockedState
                                              , &lockedState
                                              , true);

    if (lockedConsumer)
    {
        if (preLockedState != IEWS_WAITERSTATUS_DISCONNECTED)
        {
            if (*pNumConsumers == 0)
            {
                //No space for stats
                *pNumConsumers = 1;
                rc = ISMRC_TooManyConsumers;
                goto mod_exit;
            }

            //Consumer can't go away we still have the list locked!
            memset(&consDataArray[0], 0, sizeof(consDataArray[0]));

            char *pClientId = Q->pConsumer->pSession->pClient->pClientId;
            char *clientIdBuf = NULL;

            rc = iemp_allocate( pThreadData
                              , memPoolHdl
                              , strlen(pClientId)+1 //+1 for NULL
                              , (void **)&clientIdBuf);

            if (rc != OK)
            {
                goto mod_exit;
            }
            strcpy(clientIdBuf, pClientId);
            consDataArray[0].clientId  = clientIdBuf;

            consDataArray[0].sessionDeliveryStarted  = Q->pConsumer->pSession->fIsDeliveryStarted;
            consDataArray[0].sessionDeliveryStopping = Q->pConsumer->pSession->fIsDeliveryStopping;
            consDataArray[0].consumerState = Q->waiterStatus;

            //Get the stats we may need to take locks for.
            if (Q->pConsumer->fShortDeliveryIds)
            {
                if (Q->pConsumer->hMsgDelInfo)
                {
                    bool mqttIdsExhausted             = false;
                    uint32_t messagesInFlightToClient = 0;
                    uint32_t maxInflightToClient      = 0;
                    uint32_t inflightReenable         = 0;

                    iecs_getDeliveryStats( pThreadData
                                         , Q->pConsumer->hMsgDelInfo
                                         , &mqttIdsExhausted
                                         , &messagesInFlightToClient
                                         , &maxInflightToClient
                                         , &inflightReenable);

                    consDataArray[0].mqttIdsExhausted         = mqttIdsExhausted;
                    consDataArray[0].messagesInFlightToClient = messagesInFlightToClient;
                    consDataArray[0].maxInflightToClient      = maxInflightToClient;
                    consDataArray[0].inflightReenable         = inflightReenable;
                }
            }

            *pNumConsumers = 1;
        }
        else
        {
            *pNumConsumers = 0;
        }

    }
    else
    {
        //Should have been able to lock consumer!
        rc = ISMRC_WaiterInUse;
    }

mod_exit:
    if (lockedConsumer)
    {
        iews_unlockAfterQOperation( pThreadData
                                  , Qhdl
                                  , Q->pConsumer
                                  , &(Q->waiterStatus)
                                  , lockedState
                                  , preLockedState);
    }
    return rc;
}

/// @}
/*********************************************************************/
/*                                                                   */
/* End of IntermediateQ.c                                            */
/*                                                                   */
/*********************************************************************/
