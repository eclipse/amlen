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
///  @file  multiConsumerQ.c
///
///  @brief
///   Code for multi-producer, multi-consumer designed for JMS
///
///  @remarks
///    The queue is multi-producer, multi-consumer designed for JMS
///    @par
///    The multiConsumer queue is based upon the intermediate queue designed for
///    MQTT QoS 1 &2 (which in turn was based on simpleQ) and many of the functions
///    remain very similar.
///    @par
///    The queue is based upon a structure (iemqQueue_t) which contains
///    a linked list of pages (iemqQNodePage_t).  Each page is an array of
///    Nodes (iemqQNode_t) which contains a reference to a messages as well as
///    the message state. The queue always has at least 2 iemqQNodePage_t
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
///    @li getCursor     - pointer to the oldest undelivered messages on the
///    queue. Updating this field done via a CAS (in addition to the getter,
///    threads commiting puts may need to rewind it).
///    @li tail       - pointer to the next free node in the queue. Updating
///    this field is protected by the the putlock spinlock.
///
///    @see The header file multiConsumerQ.h contains the definitions and
///    function prototypes for functions which may be called from outwith
///    this source file.
///    @see The header file queueCommon.h contains the definition of the
///    interface to which these external functions must adhere.
///    @see The header file multiConsumerQInternal.h contains the private
///    data declarations for this source file.
///
///////////////////////////////////////////////////////////////////////////////
#define TRACE_COMP Engine

#include <stddef.h>
#include <assert.h>
#include <stdint.h>

#include "multiConsumerQ.h"
#include "multiConsumerQInternal.h"
#include "queueCommonInternal.h"
#include "messageDelivery.h"
#include "engineDump.h"
#include "engineStore.h"
#include "transaction.h"
#include "store.h"
#include "memHandler.h"
#include "lockManager.h"
#include "ackList.h"
#include "waiterStatus.h"
#include "clientState.h"
#include "messageExpiry.h"
#include "mempool.h"
#include "resourceSetStats.h"
#include "engineUtils.h"
#include "threadJobs.h"


/////////////////////////////////////////////////////////////////////
/// Types whose scope is confined to this file
/////////////////////////////////////////////////////////////////////

typedef struct tag_iemqAcknowledgeAsyncData_t {
    char        StructId[4];
    iemqQueue_t *Q;
    iemqQNode_t *pnode;
    ismEngine_Session_t *pSession;
    ismEngine_Transaction_t *pTran;
    uint32_t ackOptions;
    ieutThreadData_t *pJobThread;
} iemqAcknowledgeAsyncData_t;
#define IEMQ_ACKNOWLEDGE_ASYNCDATA_STRUCID "MQAK"

typedef struct tag_iemqPutPostCommitInfo_t {
    char                   StructId[4];
    uint32_t               deleteCountDecrement;
    iemqQueue_t           *Q;
} iemqPutPostCommitInfo_t;
#define IEMQ_PUTPOSTCOMMITINFO_STRUCID "MQPC"

typedef struct tag_iemqAsyncDestroyMessageBatchInfo_t {
    char StrucId[4];
    iemqQueue_t *Q;
    uint32_t batchSize;
    bool removedStoreRefs;
} iemqAsyncDestroyMessageBatchInfo_t;
#define IEMQ_ASYNCDESTROYMESSAGEBATCHINFO_STRUCID "MQDB"

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Return pointer to the iemqQNodePage_t structure from nextpage field
///  @remarks
///    The iemqQNodePage_t structure contains an array of iemqQNode_t
///    structures as well as a pointer to the next page in the list.
///    Given the address of the 'next' field this macro returns the
///    address of the field.
///////////////////////////////////////////////////////////////////////////////
#define iemq_getPageFromNextPtr(_nextptr) ((iemqQNodePage_t *)(((char *)(_nextptr)) - offsetof(iemqQNodePage_t,next)))

///////////////////////////////////////////////////////////////////////////////
// Local function prototypes - see definition for more info
///////////////////////////////////////////////////////////////////////////////
#if 0
static int32_t iemq_putToWaitingGetter( iemqQueue_t *q
                                      , ismEngine_Message_t *msg
                                      , uint8_t msgFlags
                                      , ismEngine_DelivererContext_t * delivererContext );
#endif

static inline iemqQNodePage_t *iemq_createNewPage( ieutThreadData_t *pThreadData
                                                 , iemqQueue_t *Q
                                                 , uint32_t nodesInPage);
static inline iemqQNodePage_t *iemq_getPageFromEnd(iemqQNode_t *node);
static inline iemqQNodePage_t *iemq_moveToNewPage( ieutThreadData_t *pThreadData
                                                 , iemqQueue_t *Q
                                                 , iemqQNode_t *endMsg);

static inline iemqQNode_t *iemq_getSubsequentNode( iemqQueue_t *Q
                                                 , iemqQNode_t *currPos);

static inline bool iemq_needCleanAfterConsume(iemqQNode_t *pnode);

static inline bool iemq_updateGetCursor(ieutThreadData_t *pThreadData,
                                        iemqQueue_t *Q, uint64_t failUpdateIfBefore,
                                        iemqQNode_t *queueCursorPos);

static int32_t iemq_cleanupHeadPages(ieutThreadData_t *pThreadData,
                                     iemqQueue_t *Q);

static inline void iemq_reducePreDeleteCount_internal(
    ieutThreadData_t *pThreadData, iemqQueue_t *Q);

static void iemq_completeDeletion(ieutThreadData_t *pThreadData, iemqQueue_t *Q);

static inline void iemq_updateMsgRefInStore(ieutThreadData_t *pThreadData,
                                            iemqQueue_t *Q,
                                            iemqQNode_t *pnode,
                                            uint8_t msgState,
                                            bool consumeQos2OnRestart,
                                            uint8_t deliveryCount,
                                            bool doStoreCommit);

// Softlog replay - for put
int32_t iemq_SLEReplayPut(ietrReplayPhase_t Phase, ieutThreadData_t *pThreadData,
                          ismEngine_Transaction_t *pTran, void *pEntry,
                          ietrReplayRecord_t *pRecord,
                          ismEngine_AsyncData_t *pAsyncData,
                          ietrAsyncTransactionData_t *asyncTran);

// Softlog replay - for confirmMessageDelivery(consumed)
void iemq_SLEReplayConsume(ietrReplayPhase_t Phase,
                           ieutThreadData_t *pThreadData, ismEngine_Transaction_t *pTran,
                           void *pEntry, ietrReplayRecord_t *pRecord);

static inline uint32_t iemq_choosePageSize(iemqQueue_t *Q);


void iemq_reclaimSpace(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl,
                       bool takeLock);

static inline int32_t iemq_markMessageIfGettable( ieutThreadData_t *pThreadData
                                                , iemqQueue_t *Q
                                                , iemqQNode_t *pnode
                                                , iemqQNode_t **ppNextToTry);

static inline bool iemq_tryPerConsumerFlowControl( ieutThreadData_t *pThreadData
        , iemqQueue_t *Q
        , ismEngine_Consumer_t *pConsumer );

static inline int iemq_initPutLock( iemqQueue_t *Q);
static inline int iemq_destroyPutLock( iemqQueue_t *Q);
static inline void iemq_getPutLock( iemqQueue_t *Q);
static inline void iemq_releasePutLock( iemqQueue_t *Q);
static inline void iemq_takeReadHeadLock( iemqQueue_t *Q);
static inline void iemq_takeWriteHeadLock( iemqQueue_t *Q);
static inline void iemq_releaseHeadLock( iemqQueue_t *Q);

static inline int32_t iemq_createSchedulingInfo(ieutThreadData_t *pThreadData
                                               , iemqQueue_t *Q);
static inline void iemq_destroySchedulingInfo(ieutThreadData_t *pThreadData
                                            , iemqQueue_t *Q);

static inline void iemq_fullExpiryScan( ieutThreadData_t *pThreadData
                                      , iemqQueue_t *Q
                                      , uint32_t nowExpire
                                      , bool *pPageCleanupNeeded);

static inline void iemq_scanGetCursor( ieutThreadData_t *pThreadData
                                     , iemqQueue_t *Q);

static inline int32_t iemq_updateMsgIfAvail( ieutThreadData_t *pThreadData
                                           , iemqQueue_t *Q
                                           , iemqQNode_t *node
                                           , bool increaseMsgUseCount
                                           , ismMessageState_t newState);

static int32_t iemq_asyncMessageDelivery(ieutThreadData_t           *pThreadData,
                                         int32_t                              rc,
                                         ismEngine_AsyncData_t        *asyncInfo,
                                         ismEngine_AsyncDataEntry_t     *context);

inline static bool iemq_checkFullDeliveryStartable( ieutThreadData_t *pThreadData
                                                  , iemqQueue_t *Q);

inline static bool iemq_checkAndSetFullDeliveryPrevention( ieutThreadData_t *pThreadData
                                                         , iemqQueue_t *Q);

inline static bool iemq_forceRemoveBadAcker( ieutThreadData_t *pThreadData
                                           , iemqQueue_t      *Q
                                           , uint64_t          aliveOId);
static inline void iemq_updateResourceSet( ieutThreadData_t *pThreadData
                                         , iemqQueue_t      *Q
                                         , iereResourceSetHandle_t resourceSet);

static bool iemq_scheduleOnJobThreadIfPoss(ieutThreadData_t *pThreadData,
                                           iemqQueue_t *Q,
                                           ieutThreadData_t *pJobThread);

//////////////////////////////////////////////////////////////////////////////
// Allowing pages to vary in size would allow a balance between the amount
// of memory used for an empty queue and performance. However currently we
// do not have a sensible way of measuring a 'busy' queue so currently we
// create the first page as small, and all further pages as the default if
// there are a small number of subscriptions in the system or the high
// capacity size if there are lots of subs in the system
/////////////////////////////////////////////////////////////////////////////

static uint32_t iemqPAGESIZE_SMALL = 8;
static uint32_t iemqPAGESIZE_DEFAULT = 32;
static uint32_t iemqPAGESIZE_HIGHCAPACITY = 8;


/// Periodically we need to try and remove all the consumed nodes from
/// the start of the queue. We set the period by looking at the orderId
/// when we consume a node. If the orderId has it's lower bits unset (compared
/// with the mask below) then we call compare head page.
///*MUST* be of the form 2^n -1 as it's &'d with the oId of a node
#define IEMQ_QUEUECLEAN_MASK (iemqPAGESIZE_DEFAULT -1)

//Counter used to assign ids to queues
static uint32_t nextQId = 1;

static iemqQConsumedNodeInfo_t *pFirstConsumedNode = NULL;
uint64_t numRehydratedConsumedNodes = 0;

//When we try to scan the get cursor forwards, we don't know where we are going to set it
//to yet... so we need commits of puts at /any/ point in the queue to update the getCursor
//So that when we come to record the results of the scan forward we can accurately see whether
//any puts have been committed (or consumes backed out...before the proposed new position)
//Thus the orderId in this "searching" cursor must be higher than for ANY message on the queue
#define  IEMQ_ORDERID_PAST_TAIL 0xFFFFFFFFFFFFFFFF
iemqCursor_t IEMQ_GETCURSOR_SEARCHING = { .c = { IEMQ_ORDERID_PAST_TAIL, NULL } };

// For a remote server queue, we need a measure of the cost of the bytes of a message. At the
// moment this is simply the value of the message's MsgLength, but the following macro provides
// a way to modify this if we see fit - for messages that were / are offline at restart
// we don't know their length, so *for the moment* we don't count the bytes from them
// so that the byte counts are consistent, even if they are missing some subset of messages
//
// An alternative approach could be to always use MsgLength but during iemq_completeRehydrate
// find any of this allocation type and update the stats to include them.
#define IEMQ_MESSAGE_BYTES(_pMsg) (((_pMsg)->Flags & ismENGINE_MSGFLAGS_ALLOCTYPE_1) ? 0 : (_pMsg)->MsgLength)

/// @{
/// @name External Functions

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Creates an multi-consumer Q
///  @remarks
///    This function is used to create an multi-consumer Q. The handle
///    returned can be used to put messages or create a create a consumer
///    on the queue.
///
///  @param[in]  pQName            - Name associated with this queue
///  @param[in]  QOptions          - Options
///  @param[in]  pPolicyInfo       - Initial policy information to use (optional)
///  @param[in]  hStoreObj         - Definition Store handle
///  @param[in]  hStoreProps       - Properties Store handle
///  @param[out] pQhdl             - Ptr to Q that has been created (if
///                                  returned ok)
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iemq_createQ( ieutThreadData_t *pThreadData
                    , const char *pQName
                    , ieqOptions_t QOptions
                    , iepiPolicyInfo_t *pPolicyInfo
                    , ismStore_Handle_t hStoreObj
                    , ismStore_Handle_t hStoreProps
                    , iereResourceSetHandle_t resourceSet
                    , ismQHandle_t *pQhdl)
{
    int32_t rc = 0, rc2;
    int os_rc = 0;
    iemqQueue_t *Q = NULL;
    iemqQNodePage_t *firstPage = NULL;
    bool STATUS_CreatedRecordInStore = false;

    ieutTRACEL(pThreadData, QOptions, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "QOptions(%d) hStoreObj(0x%lx)\n", __func__,
               QOptions, hStoreObj);

    *pQhdl = NULL;

    // If this queue is not being created to support a subscription or forwarding
    // queue, is not for a temporary queue, and we are not in recovery, we need to
    // add a queue definition record & queue properties record to the store now.
    //
    // Note: If the queue DOES support a subscription or forwarding queue, it will either be
    //       durable in which case a store handle will have been provided, or non-durable.

    if ( (QOptions & (  ieqOptions_SUBSCRIPTION_QUEUE
                      | ieqOptions_REMOTE_SERVER_QUEUE
                      | ieqOptions_TEMPORARY_QUEUE
                      | ieqOptions_IN_RECOVERY
                      | ieqOptions_IMPORTING)) == 0)
    {
        // If the store is almost full then we do not want to make things
        // worse by creating more owner records in the store.
        ismEngineComponentStatus_t storeStatus = ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_0];
        if (storeStatus != StatusOk)
        {
            rc = ISMRC_ServerCapacity;

            ieutTRACEL(pThreadData, storeStatus, ENGINE_WORRYING_TRACE,
                       "Rejecting create IEMQ as store status[%d] is %d\n",
                       ismENGINE_STATUS_STORE_MEMORY_0, storeStatus);

            ism_common_setError(rc);
            goto mod_exit;
        }

        rc = iest_storeQueue(pThreadData, multiConsumer, pQName, &hStoreObj,
                             &hStoreProps);

        if (rc != OK)
            goto mod_exit;

        STATUS_CreatedRecordInStore = true;
    }

    iere_primeThreadCache(pThreadData, resourceSet);
    Q = (iemqQueue_t *)iere_calloc(pThreadData,
                                   resourceSet,
                                   IEMEM_PROBE(iemem_multiConsumerQ, 1), 1,
                                   sizeof(iemqQueue_t));

    if (Q == NULL)
    {
        rc = ISMRC_AllocateError;

        // The failure to allocate memory is not necessarily the end of the
        // the world, but we must at least ensure it is correctly handled
        ism_common_setError(rc);

        size_t memSize = sizeof(iemqQueue_t);
        ieutTRACEL(pThreadData, memSize, ENGINE_ERROR_TRACE,
                   "%s: iere_calloc(%ld) failed! (rc=%d)\n", __func__,
                   memSize, rc);

        goto mod_exit;
    }

    ismEngine_SetStructId(Q->Common.StrucId, ismENGINE_QUEUE_STRUCID);
    // Record it's a multiConsumer ("JMS" style) Q
    Q->Common.QType = multiConsumer;
    Q->Common.pInterface = &QInterface[multiConsumer];
    Q->Common.resourceSet = resourceSet;

    if (pQName != NULL)
    {
        Q->Common.QName = (char *)iere_malloc(pThreadData,
                                              resourceSet,
                                              IEMEM_PROBE(iemem_multiConsumerQ, 2),
                                              strlen(pQName) + 1);

        if (Q->Common.QName == NULL)
        {
            rc = ISMRC_AllocateError;

            ism_common_setError(rc);

            goto mod_exit;
        }

        strcpy(Q->Common.QName, pQName);
    }

    if ((QOptions & ( ieqOptions_IN_RECOVERY
                     |ieqOptions_IMPORTING )) != 0)
    {
        // During recovery we keep an array of pointers to each page in the
        // queue. This allows us to add the messages to the queue back in
        // order by locating the correct page for each message.
        Q->PageMap = (ieqPageMap_t *)iere_malloc(pThreadData,
                                                 resourceSet,
                                                 IEMEM_PROBE(iemem_multiConsumerQ, 3),
                                                 ieqPAGEMAP_SIZE(ieqPAGEMAP_INCREMENT));
        if (Q->PageMap == NULL)
        {
            rc = ISMRC_AllocateError;

            // The failure to allocate memory is not necessarily the end of the
            // the world, but we must at least ensure it is correctly handled
            ism_common_setError(rc);

            size_t memSize = ieqPAGEMAP_SIZE(ieqPAGEMAP_INCREMENT);
            ieutTRACEL(pThreadData, memSize, ENGINE_ERROR_TRACE,
                       "%s: iere_malloc(%ld) failed! (rc=%d)\n", __func__,
                       memSize, rc);

            goto mod_exit;
        }
        ismEngine_SetStructId(Q->PageMap->StrucId, ieqPAGEMAP_STRUCID);
        Q->PageMap->ArraySize = ieqPAGEMAP_INCREMENT;
        Q->PageMap->PageCount = 0;
        Q->PageMap->TranRecoveryCursorOrderId = 0;
        Q->PageMap->TranRecoveryCursorIndex = 0;
        Q->PageMap->InflightMsgs = NULL;

        // Note entries in PageArray are uninitisaised for pages less than
        // PageCount
    }

    if ((QOptions & ieqOptions_SINGLE_CONSUMER_ONLY) == 0)
    {
        rc = iemq_createSchedulingInfo(pThreadData, Q);
        if (rc != OK)goto mod_exit;
    }

    //Allocate a lock scope for selection if it might ever happen (for
    //subscription, selection is done by the publish loop before it get
    //to the queue).
    if ((QOptions & ieqOptions_PUBSUB_QUEUE_MASK) == 0)
    {
        rc = ielm_allocateLockScope(pThreadData, ielmLOCK_SCOPE_NO_COMMIT, NULL,
                                    &(Q->selectionLockScope));
        if (rc != OK)
        {
            goto mod_exit;
        }
    }

    os_rc = iemq_initPutLock(Q);
    if (os_rc != 0)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);

        ieutTRACEL(pThreadData, os_rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: init(putlock) failed! (osrc=%d)\n", __func__,
                   os_rc);

        goto mod_exit;
    }

    os_rc = pthread_mutex_init(&(Q->getlock), NULL);
    if (os_rc != 0)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);

        ieutTRACEL(pThreadData, os_rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: pthread_mutex_init(getlock) failed! (osrc=%d)\n", __func__,
                   os_rc);

        goto mod_exit;
    }

    /*We want both the head lock and the waiter lock to be biased in favour of writers....*/
    pthread_rwlockattr_t rwlockattr_init;

    os_rc = pthread_rwlockattr_init(&rwlockattr_init);
    if (os_rc != 0)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);

        ieutTRACEL(pThreadData, os_rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: pthread_rwlockattr_init failed! (osrc=%d)\n", __func__,
                   os_rc);
        goto mod_exit;
    }

    os_rc = pthread_rwlockattr_setkind_np(&rwlockattr_init,
                                          PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    if (os_rc != 0)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);

        ieutTRACEL(pThreadData, os_rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: pthread_rwlockattr_init failed! (osrc=%d)\n", __func__,
                   os_rc);
        goto mod_exit;
    }

    /* When we free pages of the queue, we take a write  lock on head.
     * When we browse the queue we take a readlock whilst we scan the queue.
     */
    os_rc = pthread_rwlock_init(&(Q->headlock), &rwlockattr_init);
    if (os_rc != 0)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);

        ieutTRACEL(pThreadData, os_rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: pthread_mutex_init(headlock) failed! (osrc=%d)\n",
                   __func__, os_rc);

        goto mod_exit;
    }

    os_rc = pthread_rwlock_init(&(Q->waiterListLock), &rwlockattr_init);

    if (os_rc != 0)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);

        ieutTRACEL(pThreadData, os_rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: pthread_rwlock_init(waiterListlock) failed! (osrc=%d)\n",
                   __func__, os_rc);

        goto mod_exit;
    }

    assert(pPolicyInfo != NULL);

    // Initialise the non-null fields
    Q->qId = __sync_fetch_and_add(&(nextQId), 1);
    Q->Common.PolicyInfo = pPolicyInfo;
    Q->QOptions = QOptions;
    Q->nextOrderId = 1;
    Q->hStoreObj = hStoreObj;
    Q->hStoreProps = hStoreProps;
    Q->scanOrderId = IEMQ_ORDERID_PAST_TAIL;
    Q->preDeleteCount = 1; //The initial +1 is decremented when delete is requested....
    Q->checkWaitersVal = 1; // Force all consumers to initially assume msgs are available

    //Need to update acklist if messages can have acks and the
    //queue won't be destroyed when the consumer disconnects...
    if (   ((QOptions & (ieqOptions_IN_RECOVERY|ieqOptions_IMPORTING)) == 0)
        && ((hStoreObj != ismSTORE_NULL_HANDLE) ||
            ((QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0) ||
            ((QOptions & ieqOptions_SINGLE_CONSUMER_ONLY) == 0)))
    {
        Q->ackListsUpdating = true;
    }
    else
    {
        Q->ackListsUpdating = false;
    }

    if (QOptions & ieqOptions_SUBSCRIPTION_QUEUE)
    {
        sprintf(Q->InternalName, "+SDR:0x%0lx", Q->hStoreObj);
    }
    else if (QOptions & ieqOptions_REMOTE_SERVER_QUEUE)
    {
        sprintf(Q->InternalName, "+RDR:0x%0lx", Q->hStoreObj);
    }
    else
    {
        sprintf(Q->InternalName, "+QDR:0x%0lx", Q->hStoreObj);
    }

    // Unless we are in recovery, add the first 2 node pages. In recovery
    // this is done in the completeRehydrate function.
    if ((QOptions & (ieqOptions_IN_RECOVERY|ieqOptions_IMPORTING)) == 0)
    {
        // Allocate a page of queue nodes
        firstPage = iemq_createNewPage(pThreadData, Q, iemqPAGESIZE_SMALL);

        if (firstPage == NULL)
        {
            // The failure to allocate memory is not necessarily the end of the
            // the world, but we must at least ensure it is correctly handled
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);

            ieutTRACEL(pThreadData, iemqPAGESIZE_SMALL, ENGINE_ERROR_TRACE,
                       "%s: iemq_createNewPage failed!\n", __func__);

            goto mod_exit;
        }
        // Delay the creation of the second page until we actually need
        // it. This should help the memory footprint especially for queues/
        // subscriptions which only every have a few messages.
        firstPage->nextStatus = failed;

        Q->headPage = firstPage;
        Q->tail = &(firstPage->nodes[0]);

        //Setup the orderId of the first node so that the getCursor has a non-zero orderId
        Q->headPage->nodes[0].orderId = Q->nextOrderId++;
        Q->getCursor.c.orderId = Q->headPage->nodes[0].orderId;
        Q->getCursor.c.pNode = &(Q->headPage->nodes[0]);
    }
    else
    {
        Q->headPage = NULL;
        Q->tail = NULL;
    }

    // We have succesfully created the queue so commit the store
    // transaction if we updated the store
    if (STATUS_CreatedRecordInStore)
    {
        iest_store_commit(pThreadData, false);
    }

    // And one final thing we have to do is create the QueueRefContext
    // we will use to put messages on the queue. If this fails we have
    // to backout our hard work manually.
    if (   Q->hStoreObj != ismSTORE_NULL_HANDLE
        && (QOptions & ieqOptions_IN_RECOVERY) == 0)
    {
        rc = ism_store_openReferenceContext(Q->hStoreObj, NULL,
                                            &Q->QueueRefContext);
        if (rc != OK)
        {
            // SDR & SPR will be cleaned up by our caller, otherwise
            // we will need to clear up the QDR & QPR.
            if (STATUS_CreatedRecordInStore)
            {
                rc2 = ism_store_deleteRecord(pThreadData->hStream,
                                             Q->hStoreObj);
                if (rc2 != OK)
                {
                    ieutTRACEL(pThreadData, rc2, ENGINE_ERROR_TRACE,
                               "%s: ism_store_deleteRecord failed! (rc=%d)\n",
                               __func__, rc2);
                }
                rc2 = ism_store_deleteRecord(pThreadData->hStream,
                                             Q->hStoreProps);
                if (rc2 != OK)
                {
                    ieutTRACEL(pThreadData, rc2, ENGINE_ERROR_TRACE,
                               "%s: ism_store_deleteRecord (+QPR:0x%0lx) failed! (rc=%d)\n",
                               __func__, Q->hStoreProps, rc2);
                }

                iest_store_commit(pThreadData, false);
                STATUS_CreatedRecordInStore = false;
            }

            goto mod_exit;
        }
    }

    *pQhdl = (ismQHandle_t) Q;

mod_exit:
    if (rc != OK)
    {
        // If we have failed to define the queue then cleanup, anything
        // we succesfully created.
        if (Q != NULL)
        {
            if (Q->selectionLockScope != NULL)
                (void) ielm_freeLockScope(pThreadData, &(Q->selectionLockScope));

            iere_primeThreadCache(pThreadData, resourceSet);
            if (Q->Common.QName != NULL)
                iere_free(pThreadData, resourceSet, iemem_multiConsumerQ, Q->Common.QName);
            if (Q->PageMap != NULL)
                iere_freeStruct(pThreadData, resourceSet, iemem_multiConsumerQ, Q->PageMap, Q->PageMap->StrucId);

            iemq_destroySchedulingInfo(pThreadData, Q);
            (void) iemq_destroyPutLock(Q);
            (void) pthread_mutex_destroy(&(Q->getlock));
            (void) pthread_rwlock_destroy(&(Q->headlock));
            (void) pthread_rwlock_destroy(&(Q->waiterListLock));

            if (firstPage != NULL)
                iere_freeStruct(pThreadData, resourceSet, iemem_multiConsumerQPage, firstPage, firstPage->StrucId);

            iere_freeStruct(pThreadData, resourceSet, iemem_multiConsumerQ, Q, Q->Common.StrucId);
        }

        if (STATUS_CreatedRecordInStore)
        {
            iest_store_rollback(pThreadData, false);
        }
    }

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, Q=%p\n",
               __func__, rc, Q);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Return whether the queue is marked as deleted or not
///  @param[in] Qhdl               - Queue to check
///////////////////////////////////////////////////////////////////////////////
bool iemq_isDeleted(ismQHandle_t Qhdl)
{
    return ((iemqQueue_t *) Qhdl)->isDeleted;
}


static bool iemq_lockMutexWithWaiterLimit( pthread_mutex_t    *lock
                                         , volatile uint16_t  *waiterCount
                                         , uint16_t waiterLimit)
{
    bool gotLock = true;

    int os_rc = pthread_mutex_trylock(lock);

    if (os_rc == EBUSY)
    {
        gotLock = false;
    }
    else if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "Taking lockwithwaiterlimit failed.", os_rc
                , NULL);
    }

    if (!gotLock)
    {
        bool loopAgain;

        do
        {
            loopAgain = false;
            uint16_t newNumWaiters = __sync_add_and_fetch(waiterCount, 1);

            assert(newNumWaiters != 0);

            if (newNumWaiters <= waiterLimit)
            {
                os_rc = pthread_mutex_lock(lock);

                if (UNLIKELY(os_rc != 0))
                {
                    ieutTRACE_FFDC(ieutPROBE_002, true, "Waiting for lockwithwaiterlimit failed.", os_rc
                            , NULL);
                }

                __sync_sub_and_fetch(waiterCount, 1);

                gotLock = true;
            }
            else
            {
                newNumWaiters = __sync_sub_and_fetch(waiterCount, 1);

                if (newNumWaiters < waiterLimit)
                {
                    //Not enough waiters any more - try again to add ourselves
                    loopAgain = true;
                }
            }
        }
        while(loopAgain);
    }

    return gotLock;
}


//Really basic semaphore that you can't wait on.
//Returns true if you have access... then say finished with iemq_releaseUserCount()
static inline bool iemq_proceedIfUsersBelowLimit( volatile uint16_t  *userCount
                                                , uint16_t userLimit)
{
    bool allowedProceed = false;
    bool loopAgain;

    do
    {
        loopAgain = false;
        uint16_t newNumUsers = __sync_add_and_fetch(userCount, 1);

        assert(newNumUsers != 0);

        if (newNumUsers <= userLimit)
        {
            allowedProceed = true;
        }
        else
        {
            newNumUsers = __sync_sub_and_fetch(userCount, 1);

            if (newNumUsers < userLimit)
            {
                //Not enough users any more - try again to add ourselves
                loopAgain = true;
            }
        }
    }
    while(loopAgain);

    return allowedProceed;
}

static inline void  iemq_releaseUserCount( volatile uint16_t  *userCount)
{
    uint16_t oldNumUsers = __sync_fetch_and_sub(userCount, 1);

    if (UNLIKELY(oldNumUsers == 0))
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "Releasing user count when count already 0.", ISMRC_Error
                , NULL);
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Update the in-memory queue and optionally store record as deleted
///  @remarks
///    The waiter list must be locked while this happens to ensure that no
///    new consumers can come in.
///
///  @param[in] Qhdl               - Queue to be marked as deleted
///  @param[in] updateStore        - Whether to update the state of the store
///                                  record (effectively forcing deletion at
///                                  restart if not completed now)
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iemq_markQDeleted( ieutThreadData_t *pThreadData
                         , ismQHandle_t Qhdl
                         , bool updateStore)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    int32_t rc = OK;

    uint64_t newState;

    // Mark the in-memory queue as deleted
    Q->isDeleted = true;

    // If requested, mark the store definition record as deleted too.
    if (updateStore)
    {
        // If we have no store handle for the definition, there is
        // nothing to do here.
        if (Q->hStoreObj == ismSTORE_NULL_HANDLE)
        {
            //This must be a non-durable subscription, a forwarding or a temporary queue
            assert( (Q->QOptions
                        & (ieqOptions_SUBSCRIPTION_QUEUE | ieqOptions_REMOTE_SERVER_QUEUE | ieqOptions_TEMPORARY_QUEUE)) != 0);
            goto mod_exit;
        }

        // Set the state of the definition record to deleted.
        if (Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE)
        {
            newState = iestSDR_STATE_DELETED;
        }
        else if (Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE)
        {
            newState = iestRDR_STATE_DELETED;
        }
        else
        {
            newState = iestQDR_STATE_DELETED;
        }

        // Mark the store definition record as deleted
        rc = ism_store_updateRecord(pThreadData->hStream, Q->hStoreObj,
                                    ismSTORE_NULL_HANDLE, newState,
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

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Deletes a multiConsumer Q
///  @remarks
///    This function is used to release the memory and optionally
///    delete the STORE data for a multiconsumer Q.
///    At the time this function is called there should be no other referneces
///    to the queue.
///
///  @param[in] pQhdl              - Ptr to Q to be deleted
///  @param[in] freeOnly           - Don't change persistent state, free memory
///                                  only
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iemq_deleteQ( ieutThreadData_t *pThreadData
                    , ismQHandle_t *pQhdl
                    , bool freeOnly)
{
    int32_t rc = OK;
    iemqQueue_t *Q = (iemqQueue_t *) *pQhdl;

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p, id: %u\n",
               __func__, Q, Q->qId);

    if (Q->isDeleted)
    {
        //Gah! They are trying to delete us twice
        rc = ISMRC_QueueDeleted;
        goto mod_exit;
    }

    Q->deletionRemovesStoreObjects = !freeOnly;

    // Do we still have any associated consumer(s)?
    int32_t os_rc = pthread_rwlock_rdlock(&(Q->waiterListLock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "pthread_rwlock_rdlock waiterlist lock", rc
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , NULL);
    }

    // The queue should already have been marked as deleted in the store if it's a subscription or
    // remote server forwarding queue.
    bool markQDeletedInStore = ((Q->QOptions & ieqOptions_PUBSUB_QUEUE_MASK) == 0) ? Q->deletionRemovesStoreObjects : false;

    rc = iemq_markQDeleted(pThreadData, *pQhdl, markQDeletedInStore);

    pthread_rwlock_unlock(&(Q->waiterListLock));

    if (rc != OK)
        goto mod_exit;

    //Cause the queue to be deleted if there are no uncommitted puts or consumers with pending acks still connected
    iemq_reducePreDeleteCount_internal(pThreadData, Q);

    *pQhdl = NULL;

mod_exit:
    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, Q=%p\n",
               __func__, rc, Q);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Remove from MDR table and the queue all messages for this client
///  @remarks
///   We can't relinquish whilst a (zombie) consumer is active so if one if we flag the consumer to do
///   later relinquish, otherwise we do it immediately
///
///  @param[in] pQhdl              - Ptr to Q to have msgs removed
///  @param[in] hMsgDelInfo        -  Ptr to affected client delivery data
///
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
void iemq_relinquishAllMsgsForClient(ieutThreadData_t *pThreadData
                                    , ismQHandle_t Qhdl
                                    , iecsMessageDeliveryInfoHandle_t hMsgDelInfo
                                    , ismEngine_RelinquishType_t relinquishType)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    bool clientHasWaiter = false;

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p, id: %u type %u\n",
               __func__, Q, Q->qId, relinquishType);

    assert(    relinquishType == ismEngine_RelinquishType_ACK_HIGHRELIABLITY
            || relinquishType == ismEngine_RelinquishType_NACK_ALL);

    int32_t os_rc = pthread_rwlock_rdlock(&(Q->waiterListLock));
    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "Unable to take waiterListLock", ISMRC_Error
                      , "Q", Q, sizeof(*Q)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , "InternalName", Q->InternalName, sizeof(Q->InternalName)
                      , NULL);
    }

    ismEngine_Consumer_t *firstWaiter = Q->firstWaiter;

    if (firstWaiter != NULL)
    {
        ismEngine_Consumer_t *waiter = firstWaiter;
        do
        {
            if (waiter->hMsgDelInfo == hMsgDelInfo)
            {
                clientHasWaiter = true;
                waiter->relinquishOnTerm = relinquishType;
            }
            waiter = waiter->iemqPNext;
        }
        while (waiter != firstWaiter);
    }
    pthread_rwlock_unlock(&(Q->waiterListLock));

    if (!clientHasWaiter)
    {
        iecsRelinquishType_t iecsRelinqType;

        // Relinquish messages that are on this queue now
        if (relinquishType == ismEngine_RelinquishType_ACK_HIGHRELIABLITY)
        {
            iecsRelinqType = iecsRELINQUISH_ACK_HIGHRELIABILITY_ON_QUEUE;
        }
        else
        {
            iecsRelinqType = iecsRELINQUISH_NACK_ALL_ON_QUEUE;
            assert(relinquishType == ismEngine_RelinquishType_NACK_ALL);
        }
        iecs_relinquishAllMsgs(pThreadData, hMsgDelInfo, &Qhdl, 1,
                                               iecsRelinqType);
    }

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

static inline bool iemq_scheduleWork( ieutThreadData_t *pThreadData
                                    , iemqQueue_t *Q
                                    , ietrJobThreadId_t threadId)
{
    bool scheduleWork;
    assert(threadId != ietrNO_JOB_THREAD);

    if ((Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY) != 0)
    {
        scheduleWork = __sync_bool_compare_and_swap((uintptr_t *)&(Q->schedInfo),
                                                    IEMQ_SINGLE_CONSUMER_NO_WORK,
                                                    IEMQ_SINGLE_CONSUMER_WORKSCHEDULED);
    }
    else
    {
        iemqWaiterSchedulingInfo_t *schedInfo = Q->schedInfo;
        int os_rc = pthread_spin_lock(&(schedInfo->schedLock));

        scheduleWork = false; //Start assuming that we don't need to

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC(ieutPROBE_001, true,
                    "spin lock failed failed.", os_rc
                    , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                    , "Queue", Q, sizeof(iemqQueue_t)
                    , "schedInfo", schedInfo, sizeof(*schedInfo)
                    , NULL);
        }

        for (uint32_t i=0; i < schedInfo->maxSlots; i++)
        {
            if (schedInfo->schedThreads[i] == threadId)
            {
                //This thread is already scheduled to work on this queue... don't do it again
                break;
            }
            else if (schedInfo->schedThreads[i] == NULL)
            {
                //Empty slot - schedule us on that
                schedInfo->schedThreads[i] = threadId;
                scheduleWork = true;
                assert(schedInfo->slotsInUse < schedInfo->maxSlots);
                schedInfo->slotsInUse++;
                break;
            }
        }

        os_rc = pthread_spin_unlock(&(schedInfo->schedLock));

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC(ieutPROBE_002, true,
                    "spin lock failed failed.", os_rc
                    , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                    , "Queue", Q, sizeof(iemqQueue_t)
                    , "schedInfo", schedInfo, sizeof(*schedInfo)
                    , NULL);
        }
    }

    ieutTRACE_HISTORYBUF(pThreadData, threadId);
    ieutTRACEL(pThreadData, scheduleWork, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "Q=%p, thrd=%p : %d \n",
               __func__, Q, threadId, scheduleWork);
    return scheduleWork;
}

static inline void iemq_clearScheduledWork( ieutThreadData_t *pThreadData
                                          , iemqQueue_t *Q
                                          , ietrJobThreadId_t threadId)
{
    ieutTRACEL(pThreadData, threadId, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "Q=%p, thrd=%p\n",
               __func__, Q, threadId);

    assert(threadId != ietrNO_JOB_THREAD);
    bool workCleared = false;
    if ((Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY) != 0)
    {
        workCleared = __sync_bool_compare_and_swap((uintptr_t *)&(Q->schedInfo),
                                                   IEMQ_SINGLE_CONSUMER_WORKSCHEDULED,
                                                   IEMQ_SINGLE_CONSUMER_NO_WORK);

        if(UNLIKELY(!workCleared))
        {
            ieutTRACE_FFDC(ieutPROBE_003, false,
                    "Tried to clear work and couldn't.", ISMRC_Error
                    , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                    , "Queue", Q, sizeof(iemqQueue_t)
                    , NULL);
        }
    }
    else
    {
        iemqWaiterSchedulingInfo_t *schedInfo = Q->schedInfo;
        int os_rc = pthread_spin_lock(&(schedInfo->schedLock));

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC(ieutPROBE_001, true,
                    "spin lock failed failed.", os_rc
                    , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                    , "Queue", Q, sizeof(iemqQueue_t)
                    , "schedInfo", schedInfo, sizeof(*schedInfo)
                    , NULL);
        }

        for (uint32_t i=0; i < schedInfo->capacity; i++)
        {
            if (schedInfo->schedThreads[i] == threadId)
            {
                assert(schedInfo->slotsInUse >= 1);
                schedInfo->slotsInUse -= 1;
                //Move last entry to overwrite this one and then clear last entry.
                //If we are last entry, extra assignment but no worse than branching...
                schedInfo->schedThreads[i] = schedInfo->schedThreads[schedInfo->slotsInUse];
                schedInfo->schedThreads[schedInfo->slotsInUse] = NULL;
                workCleared = true;
                break;
            }
            else if (schedInfo->schedThreads[i] == NULL)
            {
                //Empty slot - no entries after this
                break;
            }
        }

        if(UNLIKELY(!workCleared))
        {
            ieutTRACE_FFDC(ieutPROBE_004, false,
                    "Tried to clear work and couldn't.", ISMRC_Error
                    , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                    , "Queue", Q, sizeof(iemqQueue_t)
                    , "schedInfo", schedInfo, sizeof(*schedInfo)
                    , NULL);
        }

        os_rc = pthread_spin_unlock(&(schedInfo->schedLock));

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC(ieutPROBE_002, true,
                    "spin lock failed failed.", os_rc
                    , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                    , "Queue", Q, sizeof(iemqQueue_t)
                    , "schedInfo", schedInfo, sizeof(*schedInfo)
                    , NULL);
        }
    }
}



static void iemq_appendPage( ieutThreadData_t *pThreadData
                           , iemqQueue_t *Q
                           , iemqQNodePage_t *currPage)
{
    uint32_t nodesInPage = iemq_choosePageSize(Q);
    iemqQNodePage_t *newPage = iemq_createNewPage(pThreadData, Q,
                                       nodesInPage);

    if (newPage == NULL)
    {
        // Argh - we've run out of memory - record we failed to add a
        // page so no-one sits waiting for it to appear. This is not
        // a problem for us however we will continue on our merry way.
        DEBUG_ONLY bool expectedState;
        expectedState = __sync_bool_compare_and_swap(
                            &(currPage->nextStatus), unfinished, failed);
        assert(expectedState);

        ieutTRACEL(pThreadData, currPage, ENGINE_ERROR_TRACE,
                   "iemq_createNewPage failed Q=%p nextPage=%p\n", Q,
                   currPage);
    }
    else
    {
        currPage->next = newPage;
    }
}

static inline int32_t iemq_preparePutStoreData( ieutThreadData_t *restrict pThreadData
                                              , iemqQueue_t *restrict Q
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

    if ((qmsg->Header.Persistence) && (Q->hStoreObj != ismSTORE_NULL_HANDLE)
            && (qmsg->Header.Reliability > ismMESSAGE_RELIABILITY_AT_MOST_ONCE))

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

        rc = iest_storeMessage(pThreadData, qmsg, 1, storeMessageOptions, &hMsg);
        if (rc != OK)
        {
            // iest_storeMessage will have set the error code if it was
            // unsuccessful so simply propogate any error
            ieutTRACEL(pThreadData, rc, ENGINE_ERROR_TRACE,
                       "%s: iest_storeMessage failed! (rc=%d)\n", __func__, rc);

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


///Changes here may well need to be replicated in iemq_importQNode
static inline int32_t iemq_assignQSlot( ieutThreadData_t *restrict pThreadData
                                      , iemqQueue_t *restrict Q
                                      , ismEngine_Transaction_t *pTran
                                      , uint8_t msgFlags
                                      , ismStore_Reference_t * restrict pMsgRef
                                      , iemqQNode_t **restrict ppNode)
{
    iemqQNodePage_t *nextPage = NULL;
    int32_t rc = OK;
    uint64_t putDepth;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

#if TRACETIMESTAMP_PUTLOCK
    uint64_t TTS_Start_PutLock = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_PutLock);
#endif

    //Get the putting lock
    iemq_getPutLock(Q);
    bool putLocked = true;

    iemqQNode_t *pNode = Q->tail;

    // Have we run out of space on this page?
    if ((Q->tail + 1)->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
    {
        //Page full...move to the next page if it has been added...
        //...if it hasn't been added someone will add it shortly
        iemqQNodePage_t *currpage = iemq_getPageFromEnd(Q->tail + 1);
        nextPage = iemq_moveToNewPage(pThreadData, Q, Q->tail + 1);

        // A non-null nextPage will be used to identify that a new
        // page must be created once we've released the put lock
        if (nextPage == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);

            ieutTRACEL(pThreadData, currpage, ENGINE_SEVERE_ERROR_TRACE,
                       "%s: Failed to allocate next NodePage.\n", __func__);

            goto mod_exit;
        }
        //Make the tail point to the first item in the new page
        nextPage->nodes[0].orderId = Q->nextOrderId++;
        Q->tail = &(nextPage->nodes[0]);

        //Record on the old page that a getter can free it
        currpage->nextStatus = completed;
    }
    else
    {
        // Move the tail to the next item in the page
        (Q->tail + 1)->orderId = Q->nextOrderId++;
        Q->tail++;
    }

    // Check that the node is empty. This could be just an assert
    if ((pNode->msg != MESSAGE_STATUS_EMPTY)
            || (pNode->msgState != ismMESSAGE_STATE_AVAILABLE))
    {
        ieutTRACE_FFDC( ieutPROBE_002, true
                      , "Non empty node selected during put.", ISMRC_Error
                      , "Node", pNode, sizeof(iemqQNode_t)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , NULL);
    }

    pMsgRef->OrderId = pNode->orderId;
    pNode->msgFlags = msgFlags;

    //If we're in a transaction we need to lock the message in the lock manager
    //before we put it in the queue so that no-one can get it before commit
    if (pTran != NULL)
    {
        //We can /take/ rather than request a lock from the lockmanager for this
        //message at the moment as we have yet to add the message to the node so no-one can see it
        ielmLockName_t lockName;

        lockName.Msg.LockType = ielmLOCK_TYPE_MESSAGE;
        lockName.Msg.QId = Q->qId;
        lockName.Msg.NodeId = pNode->orderId;

        ietr_recordLockScopeUsed(pTran); //Ensure the locks are cleaned up on transaction end
        rc = ielm_takeLock(pThreadData,
                           pTran->hLockScope, NULL // Lock request memory not already allocated
                           , &lockName, ielmLOCK_MODE_X, ielmLOCK_DURATION_COMMIT, NULL);

        if (LIKELY(rc == OK))
        {
            ieutTRACEL(pThreadData, pNode->orderId, ENGINE_HIFREQ_FNC_TRACE,
                       "LOCKMSG: QId %u OId %lu\n", Q->qId, pNode->orderId);
        }
        else
        {
            ieutTRACE_FFDC( ieutPROBE_003, false, "ielm_takelock failed.", rc
                          , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                          , "Queue", Q, sizeof(iemqQueue_t)
                          , "Reference", &pNode->hMsgRef, sizeof(pNode->hMsgRef)
                          , "OrderId", &pNode->orderId, sizeof(pNode->orderId)
                          , "pNode", pNode, sizeof(iemqQNode_t)
                          , NULL);

            pNode->msgState = ismMESSAGE_STATE_CONSUMED;
            goto mod_exit;
        }
    }

    // Increase the count of messages on the queue, this must be done
    // before we actually make the message visible
    iere_primeThreadCache(pThreadData, resourceSet);
    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, 1);
    pThreadData->stats.bufferedMsgCount++;
    putDepth = __sync_add_and_fetch(&(Q->bufferedMsgs), 1);

    if (pTran)
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
    putLocked = false;
    iemq_releasePutLock(Q);

#if TRACETIMESTAMP_PUTLOCK
    uint64_t TTS_Stop_PutLock = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_PutLock);
#endif

mod_exit:

    // Release the putlock if we still have it
    if (putLocked)
        iemq_releasePutLock(Q);

    // If we need to create a new page do it here...
    if (nextPage != NULL)
    {
        iemq_appendPage(pThreadData, Q, nextPage);
        nextPage = NULL;
    }

    *ppNode = pNode;

    return rc;
}


static inline int32_t iemq_finishPut( ieutThreadData_t *restrict pThreadData
                                    , iemqQueue_t *restrict Q
                                    , ismEngine_Transaction_t *pTran
                                    , ismEngine_Message_t  *restrict qmsg
                                    , iemqQNode_t *restrict pNode
                                    , ismStore_Reference_t * restrict pMsgRef
                                    , bool existingStoreTran
                                    , bool msgInStore)
{
    int32_t rc = OK;
    uint64_t messageBytes;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    // For remote server queues, we maintain statistics on message bytes
    if ((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0)
    {
        messageBytes = IEMQ_MESSAGE_BYTES(qmsg);
        pThreadData->stats.remoteServerBufferedMsgBytes += messageBytes;
        (void)__sync_add_and_fetch(&(Q->bufferedMsgBytes), messageBytes);
    }
    else
    {
        messageBytes = 0;
    }

    //Update expiry info if necessary
    if (    (pTran == NULL)
         && (qmsg->Header.Expiry != 0)
         && (pNode->msgState == ismMESSAGE_STATE_AVAILABLE))
    {
        iemeBufferedMsgExpiryDetails_t msgExpiryData = { pNode->orderId , pNode, qmsg->Header.Expiry };
        ieme_addMessageExpiryData(pThreadData, (ismEngine_Queue_t *)Q, &msgExpiryData);
    }

    // Store the message
    if (   (qmsg->Header.Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
        && (pTran == NULL))
    {
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
                Reservation.RefsCount = (pTran) ? 2 : 1;

                // If the message is to be added to the store, then reserve enough
                // space in the store for the records we are going to write.
                rc = ism_store_reserveStreamResources(pThreadData->hStream,
                                                      &Reservation);
                if (rc != OK)
                {
                    // Failure to reserve space in the store is not expected.
                    ieutTRACE_FFDC( ieutPROBE_004, true,
                                    "ism_store_reserveStreamResources failed.", rc
                                  , "RecordsCount", &Reservation.RecordsCount, sizeof(uint32_t)
                                  , "RefsCount", &Reservation.RefsCount, sizeof(uint32_t)
                                  , "DataLength", &Reservation.DataLength, sizeof(uint64_t)
                                  , "Reservation", &Reservation, sizeof(Reservation)
                                  , NULL);
                }
            }

            pNode->inStore = true;

            // First put the message in the store. This is done in a store
            // transaction which
            rc = ism_store_createReference(pThreadData->hStream,
                                           Q->QueueRefContext, pMsgRef, 0 // minimumActiveOrderId
                                           , &pNode->hMsgRef);
            if (rc != OK)
            {
                // There are no 'recoverable' failures from the ism_store_createReference call.
                ieutTRACE_FFDC( ieutPROBE_005, true, "ism_store_createReference failed.", rc
                              , "OrderId", pMsgRef->OrderId, sizeof(uint64_t)
                              , "msgRef", pMsgRef, sizeof(*pMsgRef), NULL);
            }
        }

        if (pTran)
        {
            iemqSLEPut_t SLE;
            if (pNode->inStore)
            {
                // And then add this reference to the engine transaction object
                // in the store
                assert(pNode->hMsgRef != 0);

                rc = ietr_createTranRef(pThreadData, pTran, pNode->hMsgRef,
                                        iestTOR_VALUE_PUT_MESSAGE, 0, &SLE.TranRef);
                if (rc != OK)
                {
                    // There are no other 'recoverable' failures from the
                    // ietr_createTranRef call. ietr_createTranRef will
                    // already have taken all of the require diagnostics
                    // so simply return the error up the stack.
                    ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                               "%s: ietr_createTranRef failed! (rc=%d)\n",
                               __func__, rc);

                    if (!existingStoreTran)
                    {
                        iest_store_rollback(pThreadData, false);
                    }
                    else
                    {
                        ietr_markRollbackOnly(pThreadData, pTran);
                    }

                    // Decrement the bufferedMsg values we incremented
                    iere_primeThreadCache(pThreadData, resourceSet);
                    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -1);
                    pThreadData->stats.bufferedMsgCount--;
                    (void) __sync_sub_and_fetch(&(Q->bufferedMsgs), 1);
                    if (messageBytes != 0)
                    {
                        pThreadData->stats.remoteServerBufferedMsgBytes -= messageBytes;
                        (void)__sync_sub_and_fetch(&(Q->bufferedMsgBytes), messageBytes);
                    }
                    (void) __sync_sub_and_fetch(&(Q->inflightEnqs), 1);

                    // Write-off this node
                    pNode->msgState = ismMESSAGE_STATE_CONSUMED;

                    //If this queue was so overfull that we stopped delivery, giving up on this node
                    //might need to restart... but this thread isn't in the middle of a transaction....
                    //schedule it for later
                    if (iemq_checkFullDeliveryStartable(pThreadData, Q))
                    {
                        //Keep the queue alive until checkWaiters completes
                        __sync_add_and_fetch(&(Q->preDeleteCount), 1);
                        ieq_scheduleCheckWaiters(pThreadData, (ismEngine_Queue_t *)Q);
                    }

                    goto mod_exit;
                }
            }

            // Create the softlog entry which will eventually
            // put the message on the queue when commit is called.
            SLE.pQueue = Q;
            SLE.pNode = pNode;
            SLE.pMsg = qmsg;
            SLE.bInStore = pNode->inStore;
            SLE.bCleanHead = false;
            SLE.bSavepointRolledback = false;

            // Add the message to the Soft log to be added to the queue when
            // commit is called
            rc = ietr_softLogAdd(pThreadData, pTran, ietrSLE_MQ_PUT,
                                 NULL, iemq_SLEReplayPut,
                                 Commit | MemoryCommit | PostCommit | Rollback | PostRollback | SavepointRollback,
                                 &SLE, sizeof(SLE), 0, 1, NULL);

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
                    ietr_markRollbackOnly(pThreadData, pTran);
                }

                // Decrement the bufferedMsg values we incremented
                iere_primeThreadCache(pThreadData, resourceSet);
                iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -1);
                pThreadData->stats.bufferedMsgCount--;
                (void) __sync_sub_and_fetch(&(Q->bufferedMsgs), 1);
                if (messageBytes != 0)
                {
                    pThreadData->stats.remoteServerBufferedMsgBytes -= messageBytes;
                    (void)__sync_sub_and_fetch(&(Q->bufferedMsgBytes), messageBytes);
                }
                (void) __sync_sub_and_fetch(&(Q->inflightEnqs), 1);

                //If this queue was so overfull that we stopped delivery, giving up on this node
                //might need to restart... but this thread isn't in the middle of a transaction....
                //schedule it for later
                if (iemq_checkFullDeliveryStartable(pThreadData, Q))
                {
                    //Keep the queue alive until checkWaiters completes
                    __sync_add_and_fetch(&(Q->preDeleteCount), 1);
                    ieq_scheduleCheckWaiters(pThreadData, (ismEngine_Queue_t *)Q);
                }

                // Write-off this node
                pNode->msgState = ismMESSAGE_STATE_CONSUMED;
                goto mod_exit;
            }

            //Record that the queue can not be deleted until this put is completed
            //(only relevant for point-to-point queues, the topic tree prevents subscription and
            //forwarding queues being deleted with inflight publishes)
            if ((Q->QOptions & ieqOptions_PUBSUB_QUEUE_MASK) == 0)
            {
                __sync_add_and_fetch(&(Q->preDeleteCount), 1);
            }
        }

        if ((pNode->inStore) && !existingStoreTran)
        {
            // And commit the store transaction
            iest_store_commit(pThreadData, false);
        }

        //Finally add the message to the node
        pNode->msg = qmsg;
    }
mod_exit:
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Puts a message on a multiConsumer Q
///  @remarks
///    This function is called to put a message on a multiConsumer queue.
///
///  @param[in] Qhdl               - Q to put to
///  @param[in] pTran              - Pointer to transaction
///  @param[in] inmsg              - message to put
///  @param[in] inputMsgTreatment  - IEQ_MSGTYPE_REFCOUNT (re-use & increment refcount)
///                                  or IEQ_MSGTYPE_INHERIT (re-use)
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////

int32_t iemq_putMessage( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ieqPutOptions_t putOptions
                       , ismEngine_Transaction_t *pTran
                       , ismEngine_Message_t *inmsg
                       , ieqMsgInputType_t inputMsgTreatment
                       , ismEngine_DelivererContext_t * delivererContext)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    ismEngine_Message_t *qmsg = NULL;
    int32_t rc = 0;
    iemqQNode_t *pNode;
    ismStore_Reference_t msgRef;
    uint8_t msgFlags;
    bool msgInStore = false;

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "Q=%p pTran=%p msg=%p Length=%ld Reliability=%d\n",
               __func__, Q, pTran, inmsg, inmsg->MsgLength,
               inmsg->Header.Reliability);

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

#if TRACETIMESTAMP_PUTMESSAGE
    uint64_t TTS_Start_PutMessage = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_PutMessage);
#endif

    iepiPolicyInfo_t *pPolicyInfo = Q->Common.PolicyInfo;

    bool queueFull;

    if ((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0)
    {
        assert(pPolicyInfo->maxMessageCount == 0);
        queueFull = ((pPolicyInfo->maxMessageBytes > 0) && (Q->bufferedMsgBytes >= pPolicyInfo->maxMessageBytes));
    }
    else
    {
        assert(pPolicyInfo->maxMessageBytes == 0);
        queueFull = ((pPolicyInfo->maxMessageCount > 0) && (Q->bufferedMsgs >= pPolicyInfo->maxMessageCount));
    }

    if (queueFull)
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

            if (Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE)
            {
                LOGCTX(&logContext, ERROR, Messaging, 3007, "%-s%lu",
                       "The limit of {1} messages buffered for subscription {0} has been reached.",
                       Q->Common.QName ? Q->Common.QName : "",
                       pPolicyInfo->maxMessageCount);
            }
            else if (Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE)
            {
                LOGCTX(&logContext, ERROR, Messaging, 3010, "%-s%lu",
                       "The limit of {1} message bytes buffered for remote server {0} has been reached.",
                       Q->Common.QName ? Q->Common.QName : "",
                       pPolicyInfo->maxMessageBytes);
            }
            else
            {
                LOGCTX(&logContext, ERROR, Messaging, 3006, "%-s%lu",
                       "The limit of {1} messages buffered on queue {0} has been reached.",
                       Q->Common.QName ? Q->Common.QName : "",
                       pPolicyInfo->maxMessageCount);
            }
        }
        else
        {
            if (Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE)
            {
                ieutTRACEL(pThreadData, Q->bufferedMsgBytes, ENGINE_HIGH_TRACE,
                           "%s: Queue full. QOptions=0x%08x bufferedMsgBytes=%lu bufferedMsgs=%lu maxMessageBytes=%lu\n",
                           __func__, Q->QOptions, Q->bufferedMsgBytes, Q->bufferedMsgs, pPolicyInfo->maxMessageBytes);
            }
            else
            {
                ieutTRACEL(pThreadData, Q->bufferedMsgs, ENGINE_HIGH_TRACE,
                           "%s: Queue full. QOptions=0x%08x bufferedMsgs=%lu maxMessageCount=%lu\n",
                           __func__, Q->QOptions, Q->bufferedMsgs, pPolicyInfo->maxMessageCount);
            }
        }

        if (pPolicyInfo->maxMsgBehavior == RejectNewMessages)
        {
            // We have been told to ignore RejectNewMessages - at least trace that.
            if ((putOptions & ieqPutOptions_IGNORE_REJECTNEWMSGS) != 0)
            {
                assert((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) == 0);
                ieutTRACEL(pThreadData, putOptions, ENGINE_HIGH_TRACE,
                           "Ignoring RejectNewMessages on full queue. putOptions=0x%08x\n",
                           putOptions);
                queueFull = false;
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
        else if ((pPolicyInfo->maxMessageCount > 0) &&
                 (Q->bufferedMsgs >= (100 + (3 * pPolicyInfo->maxMessageCount))))
        {
            assert((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) == 0);

            ieutTRACEL(pThreadData, Q->bufferedMsgs, ENGINE_WORRYING_TRACE,
                       "%s: Queue/Sub %s Significantly OVERfull. bufferedMsgs=%lu maxMessageCount=%lu\n",
                       __func__, Q->Common.QName ? Q->Common.QName : "",
                       Q->bufferedMsgs, pPolicyInfo->maxMessageCount);
            usleep(50);
        }
        else if ((pPolicyInfo->maxMessageBytes > 0) &&
                 (Q->bufferedMsgBytes >= (2 * pPolicyInfo->maxMessageBytes)))
        {
            assert((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0);

            ieutTRACEL(pThreadData, Q->bufferedMsgBytes, ENGINE_WORRYING_TRACE,
                       "%s: RemoteServer %s Significantly OVERfull. bufferedMsgBytes=%lu maxMessageBytes=%lu\n",
                       __func__, Q->Common.QName ? Q->Common.QName : "",
                       Q->bufferedMsgBytes, pPolicyInfo->maxMessageBytes);
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
        iem_recordMessageUsage(inmsg);
    }
    else
    {
        assert(inputMsgTreatment == IEQ_MSGTYPE_INHERIT);
    }

#if 0
    // If there's nothing on the queue that needs to be given first see if we
    // can pass the message directly to the consumer.
    // This is only possible if the message can't be pushed back to the queue
    // i.e. acking is disabled!
    if ((pTran == NULL) &&
            (qmsg->Header.Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE))
    {
        if (Q->bufferedMsgs == 0)
        {
            rc = iemq_putToWaitingGetter(Q, qmsg, msgFlags, delivererContext);
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

    rc = iemq_preparePutStoreData( pThreadData
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

    rc = iemq_assignQSlot( pThreadData
                         , Q
                         , pTran
                         , msgFlags
                         , &msgRef
                         , &pNode );

    if (UNLIKELY(rc != OK))
    {
        //TODO: Shouldn't we do the release if the other 2 parts of put fail?
        // We're going to fail the put through lack of memory
        if (inputMsgTreatment != IEQ_MSGTYPE_INHERIT)
        {
            iem_releaseMessage(pThreadData, qmsg);
        }
        goto mod_exit;
    }

    rc = iemq_finishPut( pThreadData
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
        if (((pPolicyInfo->maxMessageCount > 0) && (Q->bufferedMsgs >= pPolicyInfo->maxMessageCount)) ||
            ((pPolicyInfo->maxMessageBytes > 0) && (Q->bufferedMsgBytes >= pPolicyInfo->maxMessageBytes)))
        {
            //Check if we can remove expired messages and get below the max before next put...
            ieme_reapQExpiredMessages(pThreadData, (ismEngine_Queue_t *)Q);

            if (    (pPolicyInfo->maxMsgBehavior == DiscardOldMessages)
                 && (((pPolicyInfo->maxMessageCount > 0) && (Q->bufferedMsgs > pPolicyInfo->maxMessageCount)) ||
                     ((pPolicyInfo->maxMessageBytes > 0) && (Q->bufferedMsgBytes > pPolicyInfo->maxMessageBytes))))
            {
                //If we couldn't expire any messages, throw away enough to be below max messages
                //We throw away BEFORE checkwaiters so that any messages we send are newer than ones
                //we discard
                iemq_reclaimSpace(pThreadData, Qhdl, true);
            }
        }

        // We don't care about any error encountered while attempting
        // to deliver a message to a waiter.
        (void) iemq_checkWaiters(pThreadData, (ismQHandle_t) Q, NULL, delivererContext);
    }

#if TRACETIMESTAMP_PUTMESSAGE
    uint64_t TTS_Stop_PutMessage = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_PutMessage);
#endif

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__,
               rc);
    return rc;
}

static inline int32_t iemq_getNodeinPageMap( ieutThreadData_t *pThreadData
                                           , iemqQueue_t *Q
                                           , uint64_t orderId
                                           , int32_t *pPageNum
                                           , iemqQNode_t **ppNode)
{
    iemqQNodePage_t *pPage = NULL;         // Pointer to NodePage
    ieqPageMap_t *PageMap;                 // Local pointer to page map
    ieqPageMap_t *NewPageMap;              // Re-allocated page map
    int64_t      pageNum;                  // Must be signed
    uint64_t nodesInPage = iemqPAGESIZE_HIGHCAPACITY; //64 bit as used in 64bit bitwise arith

    int32_t rc = OK;

    // Recovery is single-threaded so need to obtain the put lock
    PageMap = Q->PageMap;

    // See if the message is on a page we have already created, we
    // look backward through the list of pages
    if (PageMap->PageCount == 0)
    {
        pageNum = 0;
    }
    else
    {
        for (pageNum = PageMap->PageCount - 1; pageNum >= 0; pageNum--)
        {
            if (   (orderId
                         >= PageMap->PageArray[pageNum].InitialOrderId)
                && (orderId
                         <= PageMap->PageArray[pageNum].FinalOrderId))
            {
                pPage = PageMap->PageArray[pageNum].pPage;
                break;
            }
            else if (orderId
                     > PageMap->PageArray[pageNum].FinalOrderId)
            {
                pageNum++;
                break;
            }
        }

        if (pageNum < 0)
        {
            ieutTRACEL(pThreadData, orderId, ENGINE_NORMAL_TRACE,
                       "Q %u (internalname: %s): Rehydrating oId %lu when current earliest page starts at %lu\n",
                       Q->qId, Q->InternalName, orderId,
                       PageMap->PageArray[0].InitialOrderId);

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
            iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

            iere_primeThreadCache(pThreadData, resourceSet);
            size_t memSize = ieqPAGEMAP_SIZE(PageMap->ArraySize + ieqPAGEMAP_INCREMENT);
            NewPageMap = (ieqPageMap_t *)iere_realloc(pThreadData,
                                                      resourceSet,
                                                      IEMEM_PROBE(iemem_multiConsumerQ, 4), Q->PageMap,
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
        if (pageNum <= (PageMap->PageCount - 1))
        {
            // We are adding a page in the middle of the existing pages so
            // we have some moving to do, pageNum points to where we must
            // insert the next page
            memmove(&PageMap->PageArray[pageNum+1],
                    &PageMap->PageArray[pageNum],
                    sizeof(ieqPageMapEntry_t) * (PageMap->PageCount-pageNum));

            // We have inserted a page ahead of the recovery page we were remembering,
            // so we need to move it along.
            if (PageMap->TranRecoveryCursorOrderId != 0 &&
                PageMap->TranRecoveryCursorIndex >= pageNum)
            {
                PageMap->TranRecoveryCursorIndex += 1;

                // Sanity check that we've moved things correctly
                assert(PageMap->TranRecoveryCursorOrderId >= PageMap->PageArray[PageMap->TranRecoveryCursorIndex].InitialOrderId &&
                       PageMap->TranRecoveryCursorOrderId <= PageMap->PageArray[PageMap->TranRecoveryCursorIndex].FinalOrderId);
            }
        }

        //We don't currently ever use a high capacity page size here as the page map assumes a fixed size page...
        pPage = iemq_createNewPage(pThreadData, Q, iemqPAGESIZE_HIGHCAPACITY);
        if (pPage == NULL)
        {
            rc = ISMRC_AllocateError;

            // The failure to allocate memory during restart is close to
            // the end of the world!
            ism_common_setError(rc);

            ieutTRACEL(pThreadData, iemqPAGESIZE_HIGHCAPACITY, ENGINE_SEVERE_ERROR_TRACE,
                       "%s: iemq_createNewPage() failed! (rc=%d)\n", __func__, rc);

            goto mod_exit;
        }
        // Initialise the page as if all of the messages have been consumed
        PageMap->PageArray[pageNum].pPage = pPage;
        PageMap->PageArray[pageNum].InitialOrderId = ((orderId - 1)
                & ~(nodesInPage - 1)) + 1;
        PageMap->PageArray[pageNum].FinalOrderId = ((orderId - 1)
                | (nodesInPage - 1)) + 1;
        for (uint32_t nodeCounter = 0; nodeCounter < pPage->nodesInPage; nodeCounter++)
        {
            pPage->nodes[nodeCounter].orderId =
                PageMap->PageArray[pageNum].InitialOrderId + nodeCounter;
            pPage->nodes[nodeCounter].msgState = ismMESSAGE_STATE_CONSUMED;
            pPage->nodes[nodeCounter].msg = MESSAGE_STATUS_EMPTY;
        }

        ieutTRACEL(pThreadData, pageNum, ENGINE_NORMAL_TRACE,
                   "curPage num=%ld InitialOrderId=%lu FinalOrderId=%lu\n", pageNum,
                   PageMap->PageArray[pageNum].InitialOrderId,
                   PageMap->PageArray[pageNum].FinalOrderId);

        // Check that our calculated InitialPageId looks ok
        assert(
            (pageNum == 0)
            || (PageMap->PageArray[pageNum].InitialOrderId
                > PageMap->PageArray[pageNum - 1].FinalOrderId));
        PageMap->PageCount++;
    }

    // And finally update the NodePage with the message
    *pPageNum = pageNum;
    *ppNode = &(pPage->nodes[(orderId - 1) & (nodesInPage - 1)]);

mod_exit:
    return rc;
}

static void iemq_completeRemoveRehydratedConsumedNodesMsgsRemoved(int retcode, void *pContext)
{

    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    pThreadData->threadType = AsyncCallbackThreadType;

    ieutTRACEL(pThreadData, numRehydratedConsumedNodes, ENGINE_FNC_TRACE,
            FUNCTION_IDENT "numRehydratedConsumedNodes=%lu\n", __func__,numRehydratedConsumedNodes);

    ieut_leavingEngine(pThreadData);
    return;
}

static void iemq_completeRemoveRehydratedConsumedNodes(ieutThreadData_t *pThreadData, int retcode)
{
    uint32_t storeOpCount = 0;
    int32_t rc = OK;

    assert(pFirstConsumedNode != NULL);
    iereResourceSetHandle_t resourceSet = pFirstConsumedNode->pQueue->Common.resourceSet;

    ieutTRACEL(pThreadData, numRehydratedConsumedNodes, ENGINE_FNC_TRACE,
            FUNCTION_ENTRY "numRehydratedConsumedNodes=%lu\n", __func__,numRehydratedConsumedNodes);


    if (retcode != OK)
    {
        ieutTRACE_FFDC(ieutPROBE_001, false,
                "Removing consumed msg references at restart failed.", retcode,
                "numRehydratedConsumedNodes", &numRehydratedConsumedNodes, sizeof(numRehydratedConsumedNodes)
                , NULL);

        //Not much we can do... can't remove messages as then on restart the refs we failed to delete will
        //point to random things....
        goto mod_exit;
    }

    while (pFirstConsumedNode != NULL)
    {
        iemqQConsumedNodeInfo_t *nodeInfo  = pFirstConsumedNode;
        pFirstConsumedNode = pFirstConsumedNode->pNext;

        rc = iest_unstoreMessage( pThreadData
                                , nodeInfo->msg
                                , false
                                , false
                                , NULL
                                , &storeOpCount);

        if (UNLIKELY(rc != OK))
        {
            // If we fail to unstore a message there is not much we
            // can actually do. So continue onwards and try and make
            // our state consistent.
            ieutTRACE_FFDC(ieutPROBE_002, false,
                    "iest_unstoreMessage (multiConsumer) failed.", rc
                    , "Internal Name", nodeInfo->pQueue->InternalName, sizeof(nodeInfo->pQueue->InternalName)
                    , "Queue", nodeInfo->pQueue, sizeof(iemqQueue_t)
                    , "OrderId", &nodeInfo->orderId, sizeof(nodeInfo->orderId)
                    , NULL);
        }

        iem_releaseMessage(pThreadData, nodeInfo->msg);

        iere_primeThreadCache(pThreadData, resourceSet);
        iere_free(pThreadData, resourceSet, iemem_unneededBufferedMsgs, nodeInfo);
    }

    if (storeOpCount > 0)
    {
        rc = iest_store_asyncCommit( pThreadData
                                   , false
                                   , iemq_completeRemoveRehydratedConsumedNodesMsgsRemoved
                                   , NULL);

        if (rc == ISMRC_AsyncCompletion)
        {
            rc = OK; //We don't care - all we do when the commit completes is trace
        }
    }

mod_exit:
    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE,
               FUNCTION_EXIT "rc=%d\n", __func__,rc);

    return;
}

static void iemq_asyncRemoveRehydratedConsumedNodes(int retcode, void *pContext)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    pThreadData->threadType = AsyncCallbackThreadType;
    iemq_completeRemoveRehydratedConsumedNodes(pThreadData, retcode);
    ieut_leavingEngine(pThreadData);
}

//On restart, remove any nodes we rehydrated but were marked consumed...
int32_t iemq_removeRehydratedConsumedNodes(ieutThreadData_t *pThreadData)
{
    iemqQConsumedNodeInfo_t *nodeInfo  = pFirstConsumedNode;
    uint64_t storeOps = 0;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, numRehydratedConsumedNodes, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "numRehydratedConsumedNodes=%lu\n", __func__,numRehydratedConsumedNodes);

    while (nodeInfo != NULL)
    {
        rc = ism_store_deleteReference( pThreadData->hStream
                                      , nodeInfo->pQueue->QueueRefContext
                                      , nodeInfo->hMsgRef
                                      , nodeInfo->orderId
                                      , 0);
        if (UNLIKELY(rc != OK))
        {
            // The failure to delete a store reference means that the
            // queue is inconsistent.
            ieutTRACE_FFDC( ieutPROBE_001, true,
                    "ism_store_deleteReference (multiConsumer) failed.", rc
                    , "Internal Name", nodeInfo->pQueue->InternalName, sizeof(nodeInfo->pQueue->InternalName)
                    , "Queue", nodeInfo->pQueue, sizeof(iemqQueue_t)
                    , "Reference", nodeInfo->hMsgRef, sizeof(nodeInfo->hMsgRef)
                    , "OrderId", &nodeInfo->orderId, sizeof(nodeInfo->orderId)
                    , NULL);
        }
        storeOps++;
        nodeInfo = nodeInfo->pNext;
    }

    if (storeOps > 0)
    {
        rc = iest_store_asyncCommit( pThreadData
                                   , false
                                   , iemq_asyncRemoveRehydratedConsumedNodes
                                   , NULL);

        if (rc == ISMRC_AsyncCompletion)
        {
            //We don't need to worry our caller about the async-ness.
            //All we are doing in the callback is the removal of any unused messages
            //(which MUST be done after the commit of the removal of the references).
            //If we stop before the callback is refired, on restart unused messages won't
            //be read so the callback was not needed....
            ieutTRACEL(pThreadData, numRehydratedConsumedNodes, ENGINE_UNUSUAL_TRACE,
                        "async removal of rehydrated consumed nodes\n");

            rc = OK;
        }
        else
        {
            iemq_completeRemoveRehydratedConsumedNodes(pThreadData, rc);
        }
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE,
               FUNCTION_EXIT "rc=%d\n", __func__,rc);

    return rc;
}


static inline int32_t iemq_addRehydratedConsumedNode( ieutThreadData_t *pThreadData
                                                    , iemqQueue_t *Q
                                                    , uint64_t orderId
                                                    , ismStore_Handle_t hMsgRef
                                                    , ismEngine_Message_t *pMsg)
{
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, orderId, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "orderid=%lu\n", __func__,orderId);

    iere_primeThreadCache(pThreadData, resourceSet);
    iemqQConsumedNodeInfo_t *nodeInfo = (iemqQConsumedNodeInfo_t *)iere_calloc(pThreadData,
                                                                               resourceSet,
                                                                               IEMEM_PROBE(iemem_unneededBufferedMsgs, 1), 1,
                                                                               sizeof(iemqQConsumedNodeInfo_t));

    if (nodeInfo == NULL)
    {
        rc = ISMRC_AllocateError;
        goto mod_exit;
    }

    pMsg->usageCount++; // Bump the count of message references

    // First bump the store ref count for this message
    rc = iest_rehydrateMessageRef(pThreadData, pMsg);
    if (rc != OK)
    {
        ieutTRACEL(pThreadData, rc, ENGINE_ERROR_TRACE,
                "%s: iest_rehydrateMessageRef failed! (rc=%d)\n", __func__, rc);
        goto mod_exit;
    }


    nodeInfo->pQueue  = Q;
    nodeInfo->hMsgRef = hMsgRef;
    nodeInfo->msg     = pMsg;
    nodeInfo->orderId = orderId;
    nodeInfo->pNext   = pFirstConsumedNode;

    pFirstConsumedNode = nodeInfo;
    numRehydratedConsumedNodes++;



mod_exit:
    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
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
int32_t iemq_rehydrateMsg( ieutThreadData_t *pThreadData
                         , ismQHandle_t Qhdl
                         , ismEngine_Message_t *pMsg
                         , ismEngine_RestartTransactionData_t *pTranData
                         , ismStore_Handle_t hMsgRef
                         , ismStore_Reference_t *pReference)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    int32_t rc = 0;
    iemqQNode_t *pNode = NULL;                    // message node
    int32_t pageNum = 0;                         // Must be signed
    uint8_t msgState = (pReference->State & 0xc0) >> 6;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, pMsg, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "Q=%p msg=%p orderid=%lu state=%u [%s]\n", __func__,
               Q, pMsg, pReference->OrderId, (uint32_t )pReference->State,
               (pTranData) ? "Transactional" : "");

    assert(hMsgRef != ismSTORE_NULL_HANDLE);

    //If the node is consumed... don't add it to the queue, add it to
    //the list of
    if (msgState == ismMESSAGE_STATE_CONSUMED)
    {
        rc = iemq_addRehydratedConsumedNode( pThreadData
                                           , Q
                                           , pReference->OrderId
                                           , hMsgRef
                                           , pMsg);
        goto mod_exit;
    }

    rc = iemq_getNodeinPageMap( pThreadData
                              , Q
                              , pReference->OrderId
                              , &pageNum
                              , &pNode);

    if (rc != OK)
    {
        goto mod_exit;
    }

    // Check that the node is empty. Perhaps this should be more than an
    // 'assert' when we have FDCs
    assert(pNode->msg == MESSAGE_STATUS_EMPTY);
    assert(pNode->msgState == ismMESSAGE_STATE_CONSUMED);

    pNode->msgState = msgState;

    if (   (pNode->msgState == ismMESSAGE_STATE_DELIVERED)
        && (pTranData == NULL))
    {
        /* The message was sent but not acked, and no transaction was in progress when */
        /* we ended - we want to send the message again                                */
        pNode->msgState = ismMESSAGE_STATE_AVAILABLE;
    }
    else if  (   (pTranData != NULL)
              && (pTranData->operationReference.Value
                      == iestTOR_VALUE_CONSUME_MSG))
    {
        /* We're getting this as part of a tran... it's not available but we sometimes
         * skip marking it consumed in the store as a performance optimisation...*/
        pNode->msgState = ismMESSAGE_STATE_CONSUMED;
    }

    if (pNode->msgState != ismMESSAGE_STATE_CONSUMED)
    {
        // For messages whose state was originally 'DELIVERED' or 'RECEIVED' we may well recover
        // a deliveryId from an MDR that we want to associate with this node, so if not in
        // reduced memory recovery mode we attempt to build a hash table mapping hMsgRef to
        // queue node.
        if (msgState == ismMESSAGE_STATE_DELIVERED || msgState == ismMESSAGE_STATE_RECEIVED)
        {
            if (!ismEngine_serverGlobal.reducedMemoryRecoveryMode)
            {
                if (Q->PageMap->InflightMsgs == NULL)
                {
                    (void)ieut_createHashTable(pThreadData,
                                               1000, // TODO: Any way to make this a better choice?
                                               iemem_multiConsumerQ,
                                               &Q->PageMap->InflightMsgs);

                    ieutTRACEL(pThreadData, Q->PageMap->InflightMsgs, ENGINE_INTERESTING_TRACE,
                               FUNCTION_IDENT "multiConsumerQ: %p Created InflightMsgs hashtable %p\n",
                               __func__, Q, Q->PageMap->InflightMsgs);
                }

                if (Q->PageMap->InflightMsgs != NULL)
                {
                    (void)ieut_putHashEntry(pThreadData,
                                            Q->PageMap->InflightMsgs,
                                            ieutFLAG_NUMERIC_KEY,
                                            (const char *)hMsgRef,
                                            ieut_generateUInt64Hash(hMsgRef),
                                            pNode,
                                            0);
                }
            }
            else
            {
                assert(Q->PageMap->InflightMsgs == NULL);
            }
        }

        // Processing based on the current node message state (which may not be the state in the
        // stored reference)
        if (   (pNode->msgState == ismMESSAGE_STATE_DELIVERED)
            || (pNode->msgState == ismMESSAGE_STATE_RECEIVED))
        {
            Q->inflightDeqs++;
        }
        else if (    (pNode->msgState == ismMESSAGE_STATE_AVAILABLE)
                  && (pMsg->Header.Expiry != 0))
        {
            iemeBufferedMsgExpiryDetails_t msgExpiryData = { pReference->OrderId , pNode, pMsg->Header.Expiry };
            ieme_addMessageExpiryData( pThreadData, (ismEngine_Queue_t *)Q,  &msgExpiryData);
        }
    }

    pNode->rehydratedState = pReference->State;
    pNode->deliveryCount = pReference->State & 0x3f;
    ieqRefValue_t RefValue;
    RefValue.Value = pReference->Value;
    pNode->msgFlags = RefValue.Parts.MsgFlags;
    pNode->orderId = pReference->OrderId;
    pNode->inStore = true;
    pNode->hMsgRef = hMsgRef;
    pNode->msg = pMsg;

    pMsg->usageCount++; // Bump the count of message references

    iere_updateMessageResourceSet(pThreadData, resourceSet, pMsg, false, false);

    // First bump the store ref count for this message
    rc = iest_rehydrateMessageRef(pThreadData, pMsg);
    if (rc != OK)
    {
        ieutTRACEL(pThreadData, rc, ENGINE_ERROR_TRACE,
                   "%s: iest_rehydrateMessageRef failed! (rc=%d)\n", __func__, rc);
        goto mod_exit;
    }

    if ((pTranData) && (pTranData->pTran))
    {
        if (pTranData->operationReference.Value == iestTOR_VALUE_PUT_MESSAGE)
        {
            ieutTRACEL(pThreadData, pTranData->pTran, ENGINE_NORMAL_TRACE,
                       "Rehydrating message which is currently part of PUT transaction\n");

            ielmLockName_t LockName = { .Msg = { ielmLOCK_TYPE_MESSAGE, Q->qId,
                                                 pNode->orderId
                                               }
                                      };

            ietr_recordLockScopeUsed(pTranData->pTran); //Ensure the locks are cleaned up on transaction end
            rc = ielm_takeLock(pThreadData,
                               pTranData->pTran->hLockScope,
                               NULL,    // Lock request memory not already allocated
                               &LockName,
                               ielmLOCK_MODE_X,
                               ielmLOCK_DURATION_COMMIT,
                               NULL);

            if (rc != OK)
            {
                //no-one else should be fiddling with this message
                ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                           "%s: ielm_takeLock failed rc=%d\n", __func__, rc);
                goto mod_exit;
            }

            // And finally create the softlog entry which will eventually
            // put the message on the queue when commit is called.
            iemqSLEPut_t SLE;
            SLE.pQueue = Q;
            SLE.pNode = pNode;
            SLE.pMsg = pNode->msg;
            SLE.bInStore = pNode->inStore;
            SLE.bCleanHead = false;
            SLE.bSavepointRolledback = false;

            // Ensure we have details of the link to the transaction
            SLE.TranRef.hTranRef = pTranData->operationRefHandle;
            SLE.TranRef.orderId = pTranData->operationReference.OrderId;

            // Add the message to the Soft log to be added to the queue when
            // commit is called
            rc = ietr_softLogRehydrate(pThreadData, pTranData, ietrSLE_MQ_PUT,
                                       NULL, iemq_SLEReplayPut,
                                       Commit | MemoryCommit | PostCommit | Rollback | PostRollback | SavepointRollback,
                                       &SLE, sizeof(SLE), 0, 1, NULL);

            if (rc != OK)
            {
                ieutTRACEL(pThreadData, rc, ENGINE_ERROR_TRACE,
                           "%s: ietr_softLogAdd failed! (rc=%d)\n", __func__, rc);
                goto mod_exit;
            }

            Q->inflightEnqs++;

            // Record that the queue can not be deleted until this put is completed
            // We do this for all queues, whether point-to-point or subscription because
            // this is a rehdrated transaction, which does not have any hold over the topic
            // tree - so it does not protect the queue from being deleted.
            //
            // When the rehydrated transaction completes the count will be decremented.
            Q->preDeleteCount++;
        }
        else if (pTranData->operationReference.Value
                 == iestTOR_VALUE_CONSUME_MSG)
        {
            ieutTRACEL(pThreadData, pTranData->pTran, ENGINE_NORMAL_TRACE,
                       "Rehydrating message which is currently part of GET transaction\n");

            ielmLockName_t LockName = { .Msg = { ielmLOCK_TYPE_MESSAGE, Q->qId,
                                                 pNode->orderId
                                               }
                                      };

            ietr_recordLockScopeUsed(pTranData->pTran); //Ensure the locks are cleaned up on transaction end
            rc = ielm_takeLock(pThreadData,
                               pTranData->pTran->hLockScope,
                               NULL,    // Lock request memory not already allocated
                               &LockName,
                               ielmLOCK_MODE_X,
                               ielmLOCK_DURATION_COMMIT,
                               NULL);

            if (rc != OK)
            {
                //no-one else should be fiddling with this message
                ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                           "%s: ielm_takeLock failed rc=%d\n", __func__, rc);
                goto mod_exit;
            }

            iemqSLEConsume_t SLE;
            SLE.pQueue = Q;
            SLE.pNode = pNode;
            SLE.pMsg = pNode->msg;
            SLE.bInStore = pNode->inStore;
            SLE.pSession = NULL; //Safe as this node is NOT part of a list of pending acks
            SLE.bCleanHead = false;
            SLE.hCachedLockRequest = NULL;

            // Ensure we have details of the link to the transaction
            SLE.TranRef.hTranRef = pTranData->operationRefHandle;
            SLE.TranRef.orderId = pTranData->operationReference.OrderId;

            //Having an inflight get adds 1 to the predelete count..
            __sync_add_and_fetch(&(Q->preDeleteCount), 1);

            Q->inflightDeqs++;

            if (pNode->deliveryCount == 0)
            {
                //We clearly weren't persisting the delivery count for this consumer... but
                //it has been delivered *at least* once
                pNode->deliveryCount = 1;
            }

            // Add the message to the Soft log to be added to the queue when
            // commit is called
            rc = ietr_softLogRehydrate(pThreadData, pTranData,
                                       ietrSLE_MQ_CONSUME_MSG, iemq_SLEReplayConsume, NULL,
                                       Commit | PostCommit | Rollback | MemoryRollback | PostRollback,
                                       &SLE, sizeof(SLE), 1, 1, NULL);
            if (rc != OK)
            {
                ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                           "%s: ietr_softLogAdd failed rc=%d\n", __func__, rc);
                goto mod_exit;
            }
        }
        else
        {
            assert(false);
        }
    }
    // Not part of a transaction, so we need to see if the transaction recovery
    // cursor should be updated.
    else if (Q->PageMap->TranRecoveryCursorOrderId > pNode->orderId)
    {
        assert(Q->PageMap->TranRecoveryCursorIndex >= pageNum);

        Q->PageMap->TranRecoveryCursorOrderId = pNode->orderId;
        Q->PageMap->TranRecoveryCursorIndex = pageNum;
    }

    // Increase the count of messages on the queue
    iere_primeThreadCache(pThreadData, resourceSet);
    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, 1);
    pThreadData->stats.bufferedMsgCount++;
    Q->bufferedMsgs++;
    if ((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0)
    {
        size_t messageBytes = IEMQ_MESSAGE_BYTES(pMsg);
        pThreadData->stats.remoteServerBufferedMsgBytes += messageBytes;
        Q->bufferedMsgBytes += messageBytes;
    }
    Q->bufferedMsgsHWM = Q->bufferedMsgs;

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int32_t iemq_markMessageGotInTran( ieutThreadData_t *pThreadData
                                 , ismQHandle_t qhdl
                                 , uint64_t orderIdOnQ
                                 , ismEngine_RestartTransactionData_t *pTranData)
{
    iemqQueue_t *Q = (iemqQueue_t *) qhdl;
    ieqPageMap_t *PageMap = Q->PageMap;
    int32_t pageNum;                       // Must be signed
    iemqQNodePage_t *pPage = NULL;         // Pointer to NodePage
    int32_t rc = 0;
    iemqQNode_t *pNode;                    // message node
    int32_t nodeNum;

    ieutTRACEL(pThreadData, orderIdOnQ, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "Q=%p (qId = %u) orderid=%lu\n", __func__, Q, Q->qId,
               orderIdOnQ);

    //This function should only be called for transactional gets during recovery,
    //we should know about puts during the original rehydration of the message
    assert(pTranData->operationReference.Value == iestTOR_VALUE_CONSUME_MSG);
    assert(ismEngine_serverGlobal.runPhase == EnginePhaseRecovery);

    // The page must be on one we already created, we scan forward from the 1st
    // page that has been identified as containing a non-transactional message
    if (PageMap->PageCount > 0)
    {
        for (pageNum = PageMap->TranRecoveryCursorIndex;
             pageNum < PageMap->PageCount;
             pageNum++)
        {
            if (   (orderIdOnQ <= PageMap->PageArray[pageNum].FinalOrderId)
                && (orderIdOnQ >= PageMap->PageArray[pageNum].InitialOrderId))
            {
                pPage = PageMap->PageArray[pageNum].pPage;
                break;
            }
        }
    }

    if (pPage == NULL)
    {
        //The node should already have been hydrated so the page should exist...
        ieutTRACE_FFDC( ieutPROBE_001, true
                       , "Page wasn't found in iemq_markMessageGotInTran", ISMRC_Error
                       , "Q", Q, sizeof(*Q)
                       , "InternalName", Q->InternalName, sizeof(Q->InternalName)
                       , "orderId", &orderIdOnQ, sizeof(orderIdOnQ)
                       , NULL);
    }

    //Find the right node in the page...
    nodeNum = (orderIdOnQ - 1) & (pPage->nodesInPage - 1);

    pNode = &pPage->nodes[nodeNum];

    if (pNode->orderId != orderIdOnQ)
    {
        //This should be the correct node and been already rehydrated...
        ieutTRACE_FFDC( ieutPROBE_002, true,
                           "Node wasn't expected orderId in iemq_markMessageGotInTran", ISMRC_Error
                      , "Q", Q, sizeof(*Q)
                      , "InternalName", Q->InternalName, sizeof(Q->InternalName)
                      , "pNode", pNode, sizeof(*pNode)
                      , "orderId", &orderIdOnQ, sizeof(orderIdOnQ)
                      , NULL);
    }

    //Increase the count of inflight deqs if it wasn't already done for this message
    if (    (pNode->msgState != ismMESSAGE_STATE_DELIVERED)
         && (pNode->msgState != ismMESSAGE_STATE_RECEIVED))
    {
        Q->inflightDeqs++;
    }

    if (pNode->deliveryCount == 0)
    {
        //We clearly weren't persisting the delivery count for this consumer... but
        //it has been delivered *at least* once
        pNode->deliveryCount = 1;
    }

    /* We're getting this as part of a tran... it's not available but we sometimes
     * skip marking it consumed in the store as a performance optimisation...*/
    pNode->msgState = ismMESSAGE_STATE_CONSUMED;



    ieutTRACEL(pThreadData, pTranData->pTran, ENGINE_NORMAL_TRACE,
               "Rehydrating message which is currently part of GET transaction\n");

    ielmLockName_t LockName = { .Msg = { ielmLOCK_TYPE_MESSAGE, Q->qId,
                                         pNode->orderId
                                       }
                              };

    ietr_recordLockScopeUsed(pTranData->pTran); //Ensure the locks are cleaned up on transaction end
    rc = ielm_requestLock(pThreadData,
                          pTranData->pTran->hLockScope, &LockName,
                          ielmLOCK_MODE_X,
                          ielmLOCK_DURATION_COMMIT,
                          NULL);

    if (rc != OK)
    {
        //no-one else should be fiddling with this message
        ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: ielm_takeLock failed rc=%d\n", __func__, rc);
        goto mod_exit;
    }

    iemqSLEConsume_t SLE;
    SLE.pQueue = Q;
    SLE.pNode = pNode;
    SLE.pMsg = pNode->msg;
    SLE.bInStore = pNode->inStore;
    SLE.pSession = NULL; //Safe as this node is NOT part of a list of pending acks
    SLE.bCleanHead = false;
    SLE.hCachedLockRequest = NULL;

    // Ensure we have details of the link to the transaction
    SLE.TranRef.hTranRef = pTranData->operationRefHandle;
    SLE.TranRef.orderId = pTranData->operationReference.OrderId;

    //Having an inflight get in a transaction contributes 1 to the inflight count
    __sync_add_and_fetch(&(Q->preDeleteCount), 1);

    // Add the message to the Soft log to be added to the queue when
    // commit is called
    rc = ietr_softLogRehydrate(pThreadData, pTranData, ietrSLE_MQ_CONSUME_MSG,
                               iemq_SLEReplayConsume, NULL,
                               Commit | PostCommit | Rollback | MemoryRollback | PostRollback, &SLE,
                               sizeof(SLE), 1, 1, NULL);
    if (rc != OK)
    {
        ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: ietr_softLogAdd failed rc=%d\n", __func__, rc);
        goto mod_exit;
    }

    // Scan forwards looking for the next message which is not part of a transaction
    if (PageMap->TranRecoveryCursorOrderId == 0 ||
        PageMap->TranRecoveryCursorOrderId == orderIdOnQ)
    {
        // We are going to reuse the LockName - check it is in the expected state
        assert((LockName.Msg.LockType == ielmLOCK_TYPE_MESSAGE) &&
               (LockName.Msg.QId == Q->qId));
        assert(PageMap->PageCount > 0);

        // If we are replacing an initialized (non-zero) TranRecoveryCursorOrderId,
        // we can start looking at the next node on the same page - if we are not,
        // we need to start from the first node on the first page.
        if (PageMap->TranRecoveryCursorOrderId != 0)
        {
            pageNum = PageMap->TranRecoveryCursorIndex;
            nodeNum += 1;
        }
        else
        {
            assert(PageMap->TranRecoveryCursorIndex == 0);
            pageNum = 0;
            nodeNum = 0;
        }

        for(; pageNum < PageMap->PageCount; pageNum++)
        {
            pPage = PageMap->PageArray[pageNum].pPage;

            for(; nodeNum < pPage->nodesInPage; nodeNum++)
            {
                pNode = &pPage->nodes[nodeNum];

                if (pNode->msg != NULL)
                {
                    ismMessageState_t msgState;

                    LockName.Msg.NodeId = pNode->orderId;

                    // Read the msgState underneath an instant-duration lock to check
                    // that we can get the lock (and thus, the message is not in a
                    // transaction)
                    if (ielm_instantLockWithPeek(&LockName,
                                                 &pNode->msgState,
                                                 &msgState) != ISMRC_LockNotGranted)
                    {
                        PageMap->TranRecoveryCursorOrderId = pNode->orderId;
                        PageMap->TranRecoveryCursorIndex = pageNum;

                        ieutTRACEL(pThreadData, pPage, ENGINE_HIFREQ_FNC_TRACE,
                                   "Transaction Recovery Cursor moved to orderId %lu page %p (index %u)\n",
                                   PageMap->TranRecoveryCursorOrderId, pPage, PageMap->TranRecoveryCursorIndex);

                        goto mod_exit; // All done, leave now.
                    }
                }
            }

            // We need to start at the 1st node on the next page
            nodeNum = 0;
        }

        // If we got this far, it means we have no available messages, so set the
        // transaction cursor to the end.
        PageMap->TranRecoveryCursorIndex = PageMap->PageCount-1;
        PageMap->TranRecoveryCursorOrderId = PageMap->PageArray[PageMap->PageCount-1].FinalOrderId;

        ieutTRACEL(pThreadData, PageMap->PageArray[PageMap->TranRecoveryCursorIndex].pPage, ENGINE_HIFREQ_FNC_TRACE,
                   "Transaction Recovery set to last orderId %lu page %p (index %u)\n",
                   PageMap->TranRecoveryCursorOrderId, PageMap->PageArray[PageMap->TranRecoveryCursorIndex].pPage,
                   PageMap->TranRecoveryCursorIndex);
    }

mod_exit:
    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


static int32_t iemq_setupQFromPageMap( ieutThreadData_t *pThreadData
                                  , iemqQueue_t *Q)
{
    iemqQNodePage_t *pCurPage = NULL;
    iemqQNodePage_t *pNextPage;
    uint32_t pageNum;
    int32_t nodeNum;  // Must be signed
    iemqQNode_t *pNode;
    ieqPageMap_t *PageMap;
    int rc = OK;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    assert(Q->PageMap != NULL);

    PageMap = Q->PageMap;

    ieutTRACEL(pThreadData, PageMap->PageCount, ENGINE_NORMAL_TRACE, "Q=%p queue has %d pages\n", Q,
               PageMap->PageCount);

    if (PageMap->PageCount)
    {
        // Loop through the pages and link them up.
        Q->headPage = PageMap->PageArray[0].pPage;

        Q->getCursor.c.orderId = Q->headPage->nodes[0].orderId;
        Q->getCursor.c.pNode = &(Q->headPage->nodes[0]);

        pCurPage = Q->headPage;
        for (pageNum = 1; pageNum < PageMap->PageCount; pageNum++)
        {
            assert(
                PageMap->PageArray[pageNum].InitialOrderId
                > PageMap->PageArray[pageNum - 1].FinalOrderId);

            pCurPage->nextStatus = completed;
            pCurPage->next = PageMap->PageArray[pageNum].pPage;
            pCurPage = pCurPage->next;

            ieutTRACEL(pThreadData, pageNum, ENGINE_NORMAL_TRACE, "Page %d = %p\n",
                       pageNum, pCurPage);
        }

        // Set the tail to the first free slot in the last page
        Q->tail = NULL;
        for (nodeNum = pCurPage->nodesInPage - 1; nodeNum >= 0; nodeNum--)
        {
            if (    (pCurPage->nodes[nodeNum].msg == NULL)
                 && (pCurPage->nodes[nodeNum].msgState  == ismMESSAGE_STATE_CONSUMED))
            {
                pCurPage->nodes[nodeNum].msgState = ismMESSAGE_STATE_AVAILABLE;
            }
            else
            {
                Q->tail = &(pCurPage->nodes[nodeNum + 1]);
                break;
            }
        }

        // Identify that the next page is pending
        pCurPage->nextStatus = failed;
    }

    // Only create a clean page of messages if we filled the last page
    // completely or the queue is empty
    if (    (Q->tail == NULL)
         || (Q->tail->msgState == ieqMESSAGE_STATE_END_OF_PAGE))
    {
        // Either there are no messages on the queue, or the last message has filled the last
        // page so we have to create a new page... this isn't used with our fixed pagesize page
        // map so can vary. If it's going to be the first page, we create it SMALL (as we would
        // have done when creating the queue for the 1st time) otherwise, choose based on capacity.
        uint32_t nodesInPage = (Q->tail == NULL) ? iemqPAGESIZE_SMALL : iemq_choosePageSize(Q);
        pNextPage = iemq_createNewPage(pThreadData, Q, nodesInPage);
        if (pNextPage == NULL)
        {
            // Argh - we've run out of memory - record we failed to add a
            // page and return the error
            ism_common_setError(rc);
            rc = ISMRC_AllocateError;

            ieutTRACEL(pThreadData, nodesInPage, ENGINE_ERROR_TRACE,
                       "%s: iemq_createNewPage failed!\n", __func__);
            goto mod_exit;
        }

        pNextPage->nextStatus = failed;
        pNextPage->nodes[0].orderId = Q->nextOrderId;

        if (Q->tail == NULL)
        {
            // First page on queue
            Q->headPage = pNextPage;

            Q->getCursor.c.orderId = Q->headPage->nodes[0].orderId;
            Q->getCursor.c.pNode = &(Q->headPage->nodes[0]);
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

        Q->nextOrderId++;
    }
    else
    {
        // Ensure we don't repeat a previously used higher orderId
        assert(Q->nextOrderId >= Q->tail->orderId);

        // mark the remainder of the nodes on the page as available
        for (pNode = Q->tail; pNode->msgState != ieqMESSAGE_STATE_END_OF_PAGE;
                pNode++)
        {
            pNode->msgState = ismMESSAGE_STATE_AVAILABLE;
            pNode->orderId = 0;
        }
        // And set the tail orderid
        Q->tail->orderId = Q->nextOrderId++;
    }

    // And last but not least free the page map
    if (Q->PageMap != NULL)
    {
        if (Q->PageMap->InflightMsgs != NULL)
        {
            ieut_destroyHashTable(pThreadData, Q->PageMap->InflightMsgs);
        }
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_freeStruct(pThreadData, resourceSet, iemem_multiConsumerQ, Q->PageMap, Q->PageMap->StrucId);
        Q->PageMap = NULL;
    }

    // During rehydrate ackListsUpdating is set to false. Now set it to the
    // correct value.
    if ((Q->hStoreObj != ismSTORE_NULL_HANDLE) ||
        ((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0) ||
        ((Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY) == 0))
    {
        Q->ackListsUpdating = true;
    }

    //Ensure the putsAttempted count is up to date
    Q->putsAttempted = Q->qavoidCount + Q->enqueueCount + Q->rejectedMsgs;
mod_exit:
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
int32_t iemq_completeRehydrate( ieutThreadData_t *pThreadData
                              , ismQHandle_t Qhdl)
{
    uint32_t rc = OK;
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p\n", __func__,
               Q);

    assert(Q->PageMap != NULL);

    assert(Q->QueueRefContext == ismSTORE_NULL_HANDLE);
    assert(Q->hStoreObj != ismSTORE_NULL_HANDLE);
    assert(Q->nextOrderId == 1);

    if (Q->Common.resourceSet == iereNO_RESOURCE_SET)
    {
        // We expect subscriptions to be assigned to a resource set, if we don't have one
        // now assign it to the default (which may be iereNO_RESOURCE_SET in which case
        // this is a no-op)
        if (Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE)
        {
            iereResourceSetHandle_t defaultResourceSet = iere_getDefaultResourceSet();
            iemq_updateResourceSet(pThreadData, Q, defaultResourceSet);
        }
    }

    // Need to open the reference context and pick up stats
    ismStore_ReferenceStatistics_t RefStats = { 0 };

    rc = ism_store_openReferenceContext(Q->hStoreObj, &RefStats,
                                        &Q->QueueRefContext);

    if (rc != OK)
        goto mod_exit;

    Q->nextOrderId = RefStats.HighestOrderId + 1;

    ieutTRACEL(pThreadData, Q->nextOrderId, ENGINE_HIGH_TRACE, "Q->nextOrderId set to %lu\n",
               Q->nextOrderId);

    // Recovery is single-threaded so don't need to obtain the put lock
    rc = iemq_setupQFromPageMap( pThreadData, Q);

    if (rc != OK) goto mod_exit;

    // During rehydrate ackListsUpdating is set to false. Now set it to the
    // correct value.
    if ((Q->hStoreObj != ismSTORE_NULL_HANDLE) ||
        ((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0) ||
        ((Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY) == 0))
    {
        Q->ackListsUpdating = true;
    }

    //Ensure the putsAttempted count is up to date
    Q->putsAttempted = Q->qavoidCount + Q->enqueueCount + Q->rejectedMsgs;

    //This queue is no longer in recovery
    Q->QOptions &= ~ieqOptions_IN_RECOVERY;

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Complete the import of a queue
///  @remarks
///    This function is called to complete the import of a queue
///    by linking up the pages of messages, and freeing the array of
///    pages.
///
///  @param[in] Qhdl               - Q to open for business
///////////////////////////////////////////////////////////////////////////////
int32_t iemq_completeImport( ieutThreadData_t *pThreadData
                        , ismQHandle_t Qhdl)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p\n", __func__,
               Q);

    assert(Q->PageMap != NULL);

    iemq_getPutLock(Q);
    rc = iemq_setupQFromPageMap( pThreadData, Q);
    iemq_releasePutLock(Q);

    if (rc == OK)
    {
        // During import ackListsUpdating is set to false. Now set it to the
        // correct value.
        if ((Q->hStoreObj != ismSTORE_NULL_HANDLE) ||
            ((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0) ||
            ((Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY) == 0))
        {
            Q->ackListsUpdating = true;
        }

        //Ensure the putsAttempted count is up to date
        Q->putsAttempted = Q->qavoidCount + Q->enqueueCount + Q->rejectedMsgs;

        //This queue is no longer being imported
        Q->QOptions &= ~ieqOptions_IMPORTING;
    }

    ieutTRACEL(pThreadData, Q->bufferedMsgs, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

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
void iemq_removeIfUnneeded( ieutThreadData_t *pThreadData
                          , ismQHandle_t Qhdl)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    ieutTRACEL(pThreadData, Q, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "Q=%p\n",
               __func__, Q);

    // This is our opportunity to delete queues that were rediscovered during
    // rehydration but we don't need during normal running
    if (Q->isDeleted)
    {
        Q->deletionRemovesStoreObjects = true; // remove from store

        iemq_reducePreDeleteCount_internal(pThreadData, Q);
    }
    ieutTRACEL(pThreadData, Q, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "\n",
               __func__);
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Rehydrates the delivery id for a message
///  @remarks
///    This function is called during recovery to set the delivery id
///    on a message.
///
///  @param[in] Qhdl               - Q to put to
///  @param[in] hMsgDelInfo        - handle of message-delivery information
///  @param[in] hMsgRef            - handle of message reference
///  @param[in] deliveryId         - Delivery id
///  @param[out] ppnode            - Pointer to used node
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iemq_rehydrateDeliveryId( ieutThreadData_t *pThreadData
                                , ismQHandle_t Qhdl
                                , iecsMessageDeliveryInfoHandle_t hMsgDelInfo
                                , ismStore_Handle_t hMsgRef
                                , uint32_t deliveryId
                                , void **ppnode)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    uint32_t pageNum, nodeNum;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, deliveryId, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "Q=%p hMsgRef=0x%0lx deliveryId=%u\n", __func__, Q,
               hMsgRef, deliveryId);

    assert(hMsgRef != ismSTORE_NULL_HANDLE);

    // Recovery is single-threaded so no need to obtain the put lock
    ieqPageMap_t *PageMap = Q->PageMap;
    assert(PageMap != NULL);

    iemqQNode_t *pNode;

    // See if this is in our table of inflight messages, which gives us a direct link to the node.
    if (PageMap->InflightMsgs != NULL)
    {
        uint32_t msgRefHash = (uint32_t)ieut_generateUInt64Hash(hMsgRef);

        rc = ieut_getHashEntry(PageMap->InflightMsgs,
                               (const char *)hMsgRef,
                               msgRefHash,
                               (void **)&pNode);

        if (rc == OK)
        {
            assert(pNode != NULL);
            ieut_removeHashEntry(pThreadData, PageMap->InflightMsgs, (const char *)hMsgRef, msgRefHash);
            goto mod_exit;
        }

        rc = OK;
    }

    // Walk through the pages in the page map looking for the message
    for (pageNum = 0; pageNum < PageMap->PageCount; pageNum++)
    {
        iemqQNodePage_t *pPage = PageMap->PageArray[pageNum].pPage;

        for (nodeNum = 0; nodeNum < pPage->nodesInPage; nodeNum++)
        {
            if (   (pPage->nodes[nodeNum].inStore)
                && (pPage->nodes[nodeNum].hMsgRef == hMsgRef))
            {
                pNode = &(pPage->nodes[nodeNum]);
                goto mod_exit;
            }
        }
    }

    // Didn't find a node.
    pNode = NULL;
    rc = ISMRC_NoMsgAvail;

mod_exit:

    *ppnode = pNode;

    if (pNode != NULL)
    {
        pNode->hasMDR = true;
        pNode->deliveryId = deliveryId;

        ismMessageState_t newState = (pNode->rehydratedState & 0xc0) >> 6;

        if (    (newState == ismMESSAGE_STATE_RECEIVED || newState == ismMESSAGE_STATE_DELIVERED)
             && (pNode->msgState == ismMESSAGE_STATE_AVAILABLE))
        {
            Q->inflightDeqs++; //Moving message to an inflight state
        }

        pNode->msgState = newState;

        if (pNode->msg->Header.Expiry != 0)
        {
            //We will have increased the count in iemq_rehydrateMsh but this
            //message is inflight and should NOT count towards the total
            ieme_removeMessageExpiryData(pThreadData, (ismEngine_Queue_t *)Q, pNode->orderId);
        }

        assert(rc == OK);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Imports a multiconsumer node
///  @remarks
///    This function is called to recreate a message from the import file.
///    Leaves store operations uncommitted
///
///  @param[in] Qhdl               - Q to put to
///  @param[in] pMsg               - message to put
///  @param[in] pTranData          - Transaction data
///  @param[in] hMsgRef            - message handle
///  @param[in] pReference         - Store reference data
///////////////////////////////////////////////////////////////////////////////
int32_t iemq_importQNode( ieutThreadData_t *pThreadData
                        , iemqQueue_t *Q
                        , ismEngine_Message_t *inmsg
                        , ismMessageState_t  msgState
                        , uint64_t orderId
                        , uint32_t deliveryId
                        , uint8_t deliveryCount
                        , uint8_t msgFlags
                        , bool hadMDR
                        , bool wasInStore
                        , iemqQNode_t **ppNode )
{
    int32_t rc = 0;
    iemqQNode_t *pNode = NULL;                    // message node
    int32_t pageNum;                       // Must be signed
    ismStore_Reference_t msgRef;
    bool msgInStore = false;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, orderId, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "Q=%p msg=%p orderid=%lu\n", __func__,
               Q, inmsg, orderId);


    iere_updateMessageResourceSet(pThreadData, resourceSet, inmsg, true, false);
    iem_recordMessageUsage(inmsg);
    bool increasedUsage = true;

    bool existingStoreTran = true;


    rc = iemq_preparePutStoreData( pThreadData
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

    iemq_getPutLock(Q);

    rc = iemq_getNodeinPageMap( pThreadData
                              , Q
                              , orderId
                              , &pageNum
                              , &pNode);


    if (rc == OK)
    {
        // Check that the node is empty. Perhaps this should be more than an
        // 'assert' when we have FDCs
        assert(pNode->msg == MESSAGE_STATUS_EMPTY);
        assert(pNode->msgState == ismMESSAGE_STATE_CONSUMED);

        pNode->msgState = msgState;

        if (   (pNode->msgState == ismMESSAGE_STATE_DELIVERED)
            || (pNode->msgState == ismMESSAGE_STATE_RECEIVED))
        {
            __sync_add_and_fetch(&(Q->inflightDeqs), 1);
        }

        //Now we have to emulate the bits of put that are done in assignQSlot
        msgRef.OrderId = pNode->orderId;
        pNode->msgFlags = msgFlags;
        // Increase the count of messages on the queue, this must be done
        // before we actually make the message visible
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, 1);
        pThreadData->stats.bufferedMsgCount++;
        uint64_t putDepth = __sync_add_and_fetch(&(Q->bufferedMsgs), 1);
        __sync_add_and_fetch(&(Q->enqueueCount), 1);

        // Update the HighWaterMark on the number of messages on the queue.
        // this is recorded on a best-can-do basis
        if (putDepth > Q->bufferedMsgsHWM)
        {
            Q->bufferedMsgsHWM = putDepth;
        }
    }

    if ((pNode->orderId +1) > Q->nextOrderId)
    {
        Q->nextOrderId = pNode->orderId +1;
    }

    //Release the lock before we do anything about a bad return code
    iemq_releasePutLock(Q);

    if (rc != OK)
    {
        goto mod_exit;
    }

    //Fix things up so the things not normally set in a put are made
    //to look like they were prior to the export
    pNode->deliveryCount = deliveryCount;
    pNode->deliveryId    = deliveryId;
    pNode->hasMDR        = hadMDR;

    rc = iemq_finishPut( pThreadData
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

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

static inline int32_t iemq_createSchedulingInfo(ieutThreadData_t *pThreadData
                                               , iemqQueue_t *Q)
{
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    iere_primeThreadCache(pThreadData, resourceSet);
    size_t memSize = sizeof(iemqWaiterSchedulingInfo_t)+
                            IEMQ_WAITERSCHEDULING_CAPACITY*sizeof(ietrJobThreadId_t);
    Q->schedInfo = iere_calloc(pThreadData,
                               resourceSet,
                               IEMEM_PROBE(iemem_multiConsumerQ, 6), 1,
                               memSize);

    if(Q->schedInfo == NULL)
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

    Q->schedInfo->capacity = IEMQ_WAITERSCHEDULING_CAPACITY;
    Q->schedInfo->maxSlots = 1; //We need one slot even if there are no consumers for discarding
    int os_rc = pthread_spin_init(&(Q->schedInfo->schedLock), PTHREAD_PROCESS_PRIVATE);
    if (os_rc != 0)
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "Taking initing schedLock failed.", os_rc
                , NULL);
    }
mod_exit:
    return rc;
}
static inline void iemq_destroySchedulingInfo(ieutThreadData_t *pThreadData
                                             , iemqQueue_t *Q)
{
    if (Q->schedInfo != NULL)
    {
        iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;
        pthread_spin_destroy(&(Q->schedInfo->schedLock));
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_free(pThreadData, resourceSet, iemem_multiConsumerQ, Q->schedInfo);
    }
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
int32_t iemq_initWaiter( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ismEngine_Consumer_t *pConsumer)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    int32_t rc = OK;
    bool addingFirstConsumer = false;
    bool usingShortDeliveryIds = pConsumer->fShortDeliveryIds;
    bool msgDeliveryInfoReferenced = false;

    ieutTRACEL(pThreadData, pConsumer, ENGINE_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n",
               __func__, Q);

    if (Q->isDeleted)
    {
        rc = ISMRC_QueueDeleted;
        goto mod_exit;
    }

    assert(pConsumer->iemqWaiterStatus == IEWS_WAITERSTATUS_DISCONNECTED);

    //If this waiter has messages permanently stored as "inflight" for this waiter, let him
    //know which ones are inflight
    if (usingShortDeliveryIds)
    {
        if (pConsumer->hMsgDelInfo == NULL)
        {
            // If we don't yet have the delivery information handle, get it now
            rc = iecs_acquireMessageDeliveryInfoReference(pThreadData,
                    pConsumer->pSession->pClient, &pConsumer->hMsgDelInfo);
            if (rc != OK)
            {
                goto mod_exit;
            }
            assert(pConsumer->hMsgDelInfo != NULL);
            msgDeliveryInfoReferenced = true;
        }
        pConsumer->fRedelivering = true;
    }

    int32_t os_rc = pthread_rwlock_wrlock(&(Q->waiterListLock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "Unable to take waiterListLock", ISMRC_Error
                      , "Q", Q, sizeof(*Q)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , "InternalName", Q->InternalName, sizeof(Q->InternalName)
                      , NULL);
    }

    if (Q->firstWaiter != NULL)
    {
        // We were configured for a single consumer only and this is not the first,
        // so refuse this consumer.
        if ((Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY) != 0)
        {
            pthread_rwlock_unlock(&(Q->waiterListLock));
            rc = ISMRC_WaiterInUse;
            ism_common_setError(rc);
            goto mod_exit;
        }

        // If we're using short delivery Ids, and there is another (active) consumer from the same
        // client, we will get into all kinds of mess, for example, both consumers would have both
        // msgs inflight to the client redelivered to them... So fail the request if that is the case.
        if (usingShortDeliveryIds)
        {
            assert(pConsumer->hMsgDelInfo != NULL);

            ismEngine_ClientState_t *thisClient = pConsumer->pSession->pClient;
            ismEngine_Consumer_t *firstWaiter = Q->firstWaiter;
            ismEngine_Consumer_t *waiter = firstWaiter;
            do
            {
                if (waiter->fDestroyCompleted == false &&
                    waiter->pSession->pClient == thisClient)
                {
                    // Expect consistent use of short deliverIds
                    assert(waiter->fShortDeliveryIds == true);

                    pthread_rwlock_unlock(&(Q->waiterListLock));
                    rc = ISMRC_WaiterInUse;
                    ism_common_setError(rc);
                    goto mod_exit;
                }
                waiter = waiter->iemqPNext;
            }
            while (waiter != firstWaiter);
        }

        Q->firstWaiter->iemqPPrev->iemqPNext = pConsumer;
        pConsumer->iemqPPrev = Q->firstWaiter->iemqPPrev;

        Q->firstWaiter->iemqPPrev = pConsumer;
        pConsumer->iemqPNext = Q->firstWaiter;
    }
    else
    {
        //This waiter is in a circular list on it's own, make it point to itself..
        pConsumer->iemqPPrev = pConsumer;
        pConsumer->iemqPNext = pConsumer;
        addingFirstConsumer = true;
    }

    Q->firstWaiter = pConsumer;

    //Update the number of browsers which affects how checkWaiters works
    if (pConsumer->fDestructiveGet == false)
    {
        Q->numBrowsingWaiters++;
    }
    if (pConsumer->selectionRule != NULL)
    {
        // Note. browsing waiters don't care about rewinds of the get cursor but selecting waiters do
        Q->numSelectingWaiters++;
    }

    //If we decided that we weren't going to keep tracks of acks for this queue, we need to change our
    //mind if the consumer is:
    //a)transactional as we need the queue keep track of inflight messages so it
    //  stays alive until the transaction(s) complete
    //b)recording persistent delivery ids that can be acked later (as this queue
    //  does not currently support the track inflight model used by intermediateQ
    if ((!Q->ackListsUpdating) && (Q->QOptions & (ieqOptions_IN_RECOVERY|ieqOptions_IMPORTING)) == 0)
    {
        //If there were multiple consumers we'd have been updating the ack list
        assert((Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY) != 0);

        if (pConsumer->pSession->fIsTransactional
                || pConsumer->fShortDeliveryIds)
        {
            Q->ackListsUpdating = true;
        }
    }

    //Increase the count of waiter scheduling slots by 1 unless it is at capacity
    if ((Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY) == 0)
    {
        iemqWaiterSchedulingInfo_t *schedInfo = Q->schedInfo;
        os_rc = pthread_spin_lock(&(schedInfo->schedLock));


        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC(ieutPROBE_002, true,
                    "spin lock taking failed.", rc
                    , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                    , "Queue", Q, sizeof(iemqQueue_t)
                    , "schedInfo", schedInfo, sizeof(*schedInfo)
                    , NULL);
        }

        //Both 0 and 1 consumers correspond to 1 slot
        if (   !addingFirstConsumer
            && schedInfo->maxSlots < schedInfo->capacity)
        {
            schedInfo->maxSlots++;
        }

        os_rc = pthread_spin_unlock(&(schedInfo->schedLock));

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC(ieutPROBE_003, true,
                    "spin lock releasing failed.", os_rc
                    , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                    , "Queue", Q, sizeof(iemqQueue_t)
                    , "schedInfo", schedInfo, sizeof(*schedInfo)
                    , NULL);
        }
    }

    pthread_rwlock_unlock(&(Q->waiterListLock));

    //Adding a waiter to an queue requires adding 1 to the count of delete blocking operations...
    __sync_add_and_fetch(&(Q->preDeleteCount), 1);

    DEBUG_ONLY bool connectedWaiter;
    connectedWaiter = iews_bool_changeState(
                          &(pConsumer->iemqWaiterStatus),
                          IEWS_WAITERSTATUS_DISCONNECTED,
                          IEWS_WAITERSTATUS_DISABLED);

    //This should be a fresh consumer not connected to another queue
    assert(connectedWaiter);

mod_exit:
    if (rc != OK && msgDeliveryInfoReferenced)
    {
        assert(pConsumer->hMsgDelInfo != NULL);
        iecs_releaseMessageDeliveryInfoReference(pThreadData, pConsumer->hMsgDelInfo);
        pConsumer->hMsgDelInfo = NULL;
    }
    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
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
int32_t iemq_termWaiter( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ismEngine_Consumer_t *pConsumer)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pConsumer, ENGINE_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n",
               __func__, Q);
    bool doneDisconnect = false;
    bool waiterInUse = false;

    //Remove the waiter from the list of waiters
    int32_t os_rc = pthread_rwlock_wrlock(&(Q->waiterListLock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "Unable to take waiterListLock", ISMRC_Error
                       , "Q", Q, sizeof(*Q)
                       , "os_rc", &os_rc, sizeof(os_rc)
                       , "InternalName", Q->InternalName, sizeof(Q->InternalName)
                       , NULL);
    }

    //If the waiter will relinquish on term, we shouldn't do that if there are other
    //(zombie) consumers for the same client
    if (pConsumer->relinquishOnTerm)
    {
        ismEngine_Consumer_t *firstWaiter = Q->firstWaiter;
        ismEngine_Consumer_t *tmp = firstWaiter;
        if (tmp != NULL)
        {
            do
            {
                if (    (tmp != pConsumer)
                     && (tmp->pSession == pConsumer->pSession) )
                {
                    pConsumer->relinquishOnTerm = false;
                    ieutTRACEL(pThreadData, pConsumer, ENGINE_UNUSUAL_TRACE,
                               "Skipping Relinquish for %p as other consumers on Q=%p\n", pConsumer, Q);
                    break;
                }
                tmp = tmp->iemqPNext;
            }
            while(tmp != firstWaiter);
        }
    }

    if (pConsumer->iemqPNext != pConsumer)
    {
        assert(pConsumer->iemqPPrev != pConsumer);

        pConsumer->iemqPNext->iemqPPrev = pConsumer->iemqPPrev;
        pConsumer->iemqPPrev->iemqPNext = pConsumer->iemqPNext;

        if (Q->firstWaiter == pConsumer)
        {
            Q->firstWaiter = pConsumer->iemqPNext;
        }
    }
    else
    {
        //Must be the only waiter in the list
        assert(Q->firstWaiter == pConsumer);

        Q->putsAttempted = Q->qavoidCount + Q->enqueueCount + Q->rejectedMsgs;
        Q->firstWaiter = NULL;
    }

    pConsumer->iemqPNext = NULL;
    pConsumer->iemqPPrev = NULL;

    //Update the number of consumers (used to control checkWaiters loop)
    if (pConsumer->fDestructiveGet == false)
    {
        assert(Q->numBrowsingWaiters > 0);
        Q->numBrowsingWaiters--;
    }
    if (pConsumer->selectionRule != NULL)
    {
        assert(Q->numSelectingWaiters > 0);
        Q->numSelectingWaiters--;
    }

    //Decrease the count of waiter scheduling slots by 1 unless we are still at capacity
    if ((Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY) == 0)
    {
        iemqWaiterSchedulingInfo_t *schedInfo = Q->schedInfo;
        os_rc = pthread_spin_lock(&(schedInfo->schedLock));

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC(ieutPROBE_002, true,
                    "spin lock taking failed.", os_rc
                    , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                    , "Queue", Q, sizeof(iemqQueue_t)
                    , "schedInfo", schedInfo, sizeof(*schedInfo)
                    , NULL);
        }

        if (schedInfo->maxSlots < schedInfo->capacity)
        {
            if (schedInfo->maxSlots > 1)
            {
                schedInfo->maxSlots--;
            }
        }
        else
        {
            uint64_t numConsumers = 0;
            //Hmmm we were are capacity count waiters to see if we still are
            ismEngine_Consumer_t *firstWaiter = Q->firstWaiter;
            ismEngine_Consumer_t *tmp = firstWaiter;
            if (tmp != NULL)
            {
                do
                {
                    numConsumers++;
                    tmp = tmp->iemqPNext;
                }
                while(tmp != firstWaiter);
            }

            if (numConsumers == 0)
            {
                schedInfo->maxSlots = 1; //Need at least one for discarding when disconnected
            }
            if (numConsumers < schedInfo->capacity)
            {
                schedInfo->maxSlots = numConsumers;
            }
            else
            {
                schedInfo->maxSlots = schedInfo->capacity;
            }
        }

        os_rc = pthread_spin_unlock(&(schedInfo->schedLock));

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC(ieutPROBE_003, true,
                    "spin lock releasing failed.", os_rc
                    , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                    , "Queue", Q, sizeof(iemqQueue_t)
                    , "schedInfo", schedInfo, sizeof(*schedInfo)
                    , NULL);
        }
    }

    pthread_rwlock_unlock(&(Q->waiterListLock));



    // Next we need to move the waiter into DISCONNECT_PEND state. It may be
    // that DISABLE_PEND will also be on, but that is ok.

    iewsWaiterStatus_t oldState;
    do
    {
        oldState = pConsumer->iemqWaiterStatus;
        iewsWaiterStatus_t newState;

        if ((oldState == IEWS_WAITERSTATUS_DISCONNECTED)
                || ((oldState & IEWS_WAITERSTATUS_DISCONNECT_PEND) != 0))
        {
            // You can't disconnect the waiter if it's already disconnect(ing)
            rc = ISMRC_WaiterInvalid;
            goto mod_exit;
        }
        else if ((oldState == IEWS_WAITERSTATUS_GETTING)
                 || (oldState == IEWS_WAITERSTATUS_DELIVERING))
        {
            newState = IEWS_WAITERSTATUS_DISCONNECT_PEND;
            waiterInUse = true;
        }
        else if (oldState & IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND)
        {
            //This flag implies an earlier disable was cancelled... this is getting very confusing. For the
            //moment lets just take a deep breath and wait for things to calm down
            ismEngine_FullMemoryBarrier();
            continue;
        }
        else if (oldState & IEWS_WAITERSTATUSMASK_PENDING)
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

        doneDisconnect = __sync_bool_compare_and_swap(
                             &(pConsumer->iemqWaiterStatus), oldState, newState);
    }
    while (!doneDisconnect);

    if (waiterInUse)
    {
        // The waiter is (was) locked by someone else when we switched
        // on the IEWS_WAITERSTATUS_DISCONNECT_PEND state, so our work
        // is done.
        rc = ISMRC_AsyncCompletion;
    }
    else
    {
        bool waiterWasEnabled =
            (oldState != IEWS_WAITERSTATUS_DISABLED) ? true : false;

        ieq_completeWaiterActions( pThreadData
                                 , Qhdl
                                 , pConsumer
                                 , IEQ_COMPLETEWAITERACTION_OPTS_NONE
                                 , waiterWasEnabled);
    }

mod_exit:
    //NB: The above callback may have destroyed the queue so
    //we cannot use it at this point
    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__,
               rc);
    return rc;
}

//NB: Must be called between LockManager's releaseAllBegin and releaseAllComplete
//or have some memory barriers so that we are not looking at an old value for the
//get cursor (and e.g. have headlock so node doesn't go away during rewind)
static inline void iemq_rewindGetCursor( ieutThreadData_t *pThreadData
                                       , iemqQueue_t *Q
                                       , iemqCursor_t newMsg)
{
    if (newMsg.c.orderId < Q->getCursor.c.orderId)
    {
        bool done;

        do
        {
            iemqCursor_t oldState = Q->getCursor;

            if (oldState.c.orderId > newMsg.c.orderId)
            {
                done = __sync_bool_compare_and_swap(&(Q->getCursor.whole),
                                                    oldState.whole, newMsg.whole);
                if (done)
                {
                    ieutTRACEL(pThreadData, newMsg.c.orderId, ENGINE_NORMAL_TRACE,
                               "Q %u rewound queue cursor from %lu to %lu\n",
                               Q->qId, oldState.c.orderId, newMsg.c.orderId);
                }
            }
            else
            {
                //Someone else has rewound the getCursor to before us...that's fine
                done = true;
                ieutTRACEL(pThreadData, newMsg.c.orderId, ENGINE_NORMAL_TRACE,
                           "Q %u no need to rewind get cursor. Current OId is %lu. rewind to was OId %lu\n",
                           Q->qId, Q->getCursor.c.orderId, newMsg.c.orderId);
            }

        }
        while (!done);
    }
    else
    {
        //Someone else has rewound the getCursor to before us...that's fine
        ieutTRACEL(pThreadData, Q->getCursor.c.orderId, ENGINE_NORMAL_TRACE,
                   "Q %u no need to rewind get cursor. Current OId is %lu. rewind to was OId %lu\n",
                   Q->qId, Q->getCursor.c.orderId, newMsg.c.orderId);
    }

    if (Q->numSelectingWaiters > 0)
    {
        // If there are selecting consumers, then we may need to rewind them too
        int32_t os_rc = pthread_rwlock_rdlock(&(Q->waiterListLock));
        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC( ieutPROBE_001, true, "Unable to take waiterListLock", ISMRC_Error
                          , "Q", Q, sizeof(*Q)
                          , "os_rc", &os_rc, sizeof(os_rc)
                          , "InternalName", Q->InternalName, sizeof(Q->InternalName)
                          , NULL);
        }

        ismEngine_Consumer_t *pFirstConsumer = Q->firstWaiter;
        if (pFirstConsumer)
        {
            ismEngine_Consumer_t *pConsumer = pFirstConsumer;
            do
            {
                if (   (pConsumer->fDestructiveGet)
                    && (pConsumer->selectionRule != NULL))
                {
                    bool done = false;
                    iemqCursor_t oldCursor = pConsumer->iemqCursor;

                    while (!done && (oldCursor.c.orderId > newMsg.c.orderId))
                    {
                        done = __sync_bool_compare_and_swap(
                                   &(pConsumer->iemqCursor.whole), oldCursor.whole,
                                   newMsg.whole);

                        if (!done)
                        {
                            //Update our view of where the cursor is....
                            oldCursor = pConsumer->iemqCursor;
                        }
                    }

                    ieutTRACEL(pThreadData, pConsumer, ENGINE_NORMAL_TRACE,
                               "Consumer cursor rewound to OId %ld for QId %d Consumer %p\n",
                               newMsg.c.orderId, Q->qId, pConsumer);
                }
                pConsumer = pConsumer->iemqPNext;
            }
            while (pConsumer != pFirstConsumer);
        }

        pthread_rwlock_unlock(&(Q->waiterListLock));
    }

    return;
}



/// Periodically we need to try and remove all the consumed nodes from
/// the start of the queue. We set the period by looking at the orderId
/// when we consume a node. If the orderId has it's lower bits unset (compared
/// with the mask below) then we call compare head page.
///
/// This function returns true if we ought to call cleanupHeadPage when the node
/// is consumed.
static inline bool iemq_needCleanAfterConsume(iemqQNode_t *pnode)
{
    if ((pnode->orderId & IEMQ_QUEUECLEAN_MASK) == 0)
    {
        return true;
    }

    return false;
}

static inline bool iemq_isNodeConsumed( ieutThreadData_t *pThreadData
                                      , iemqQueue_t *Q
                                      , iemqQNode_t *node)
{
    ismMessageState_t msgState;
    ielmLockName_t LockName = { .Msg = { ielmLOCK_TYPE_MESSAGE,
                                         Q->qId, node->orderId } };
    bool consumed=false;

    //Read the msgState underneath an instant-duration lock
    int rc = ielm_instantLockWithPeek( &LockName
                                     , &node->msgState
                                     , &msgState);

    if (rc == OK)
    {
        if (msgState == ismMESSAGE_STATE_CONSUMED)
        {
            consumed = true;
        }
    }
    else
    {
        //It's not an error to not be able to lock the node
        if (rc != ISMRC_LockNotGranted)
        {
            ieutTRACE_FFDC(ieutPROBE_001, true,
                             "lockmanager lock failed.", rc
                           , "LockName", &LockName, sizeof(LockName)
                           , "Node", node, sizeof(*node)
                           , NULL);

        }
    }

    return consumed;
}

//Doesn't use the lock manager, just dirty reads node status as a rough guess
//Even if this guesses it can be, some nodes may be locked in lock manager
//so consume may not be possible
//
// Caller must prevent page being freed e.g. with headlock
//
static inline bool iemq_guessPageConsumed ( ieutThreadData_t *pThreadData
                                          , iemqQueue_t *Q
                                          , iemqQNodePage_t *page
                                          , uint64_t *pBlockingOId)
{
    bool consumePage = true; //Assume we can delete it unless we find a reason not too.
    iemqQNode_t *lastNode = &(page->nodes[page->nodesInPage - 1]);

    //Take a quick peak at the msgStates on the page before we try and do anything with the lock manager
    for (iemqQNode_t *node = lastNode; node >= &(page->nodes[0]);
            node--)
    {
        //We only want to try and lock nodes if everything on the page iss consumed
        if (node->msgState != ismMESSAGE_STATE_CONSUMED)
        {
            consumePage = false;
            *pBlockingOId = node->orderId;

#ifdef ieutTRAPPEDMSG_DEADLOCK
            //We're going to deadlock and wait for gdb. This shouldn't happen
            //on a customer system so this code should only be compiled in when
            //looking for a bug internally.
            uint64_t QCursorOId = Q->getCursor.c.orderId;
            uint64_t scanOrderId = Q->scanOrderId;

            uint64_t cursorEstimate = ( (QCursorOId < IEMQ_ORDERID_PAST_TAIL) ?
                                             QCursorOId
                                         :   scanOrderId);

            //We don't want extra barriers but we don't want to lock up
            //just because we're using a stale cursor value. Only lock if the
            //cursor is 100 ahead of the trapped message.
            uint64_t conservativeCursorEstimate = cursorEstimate -100;

            //Rough n' ready approximate check for trapped messages
            if( conservativeCursorEstimate < Q->nextOrderId )
            {
                if (    (aliveOId < conservativeCursorEstimate)
                     && node->msgState == ismMESSAGE_STATE_AVAILABLE)
                {
                    // Is this node locked or not?
                    ielmLockName_t LockName = { .Msg = {
                            ielmLOCK_TYPE_MESSAGE, Q->qId, aliveOId
                           }
                    };

                    int32_t rc = ielm_instantLockWithPeek(&LockName, NULL, NULL);
                    if(rc == OK )
                    {
                        //Unlocked, and avail and before (snapshotted) cursor
                        //looks trapped.
                        ieutTRACEL(pThreadData, aliveOId, 5,
                                       "Q %u POTENTIAL TRAPPED MSG: %lu cursor %lu\n",
                                       Q->qId, aliveOId,  cursorEstimate);

                        //Let's deadlock up and wait for gdb
                        iemq_getPutLock(Q);
                        pthread_mutex_lock(&(Q->getlock));

                        volatile bool stop = true;

                        printf ("Deliberately deadlocked!");
                        while (stop)
                        {
                            usleep(10000);
                        }
                        pthread_mutex_unlock(&(Q->getlock));
                        iemq_releasePutLock(Q);

                    }
                    else
                    {
                        ieutTRACEL(pThreadData, aliveOId, 5,
                                    "Q %u CANDIDATE TRAPPED MSG but still locked so we'll continue: %lu cursor %lu\n",
                                                                   Q->qId, aliveOId, cursorEstimate );
                    }
                }
            }
#endif
            break;
        }
    }

    return consumePage;
}

//If it is fully filled with unlocked, consumed nodes, pNodesInStore is
//updated to say how node nodes on the page were in the store. If it's
//not fully consumed, pBlockingOId contains OId of first node that isn't finished with
static bool iemq_isPageFullyConsumed( ieutThreadData_t *pThreadData
                                    , iemqQueue_t *Q
                                    , iemqQNodePage_t *page
                                    , uint64_t *pBlockingOId
                                    , uint64_t *pNodesInStore)
{
    bool consumePage = true; //Assume we can delete it unless we find a reason not too.
    iemqQNode_t *lastNode = &(page->nodes[page->nodesInPage - 1]);

    //Take a quick peak at the msgStates on the page before we try and do anything with the lock manager
    consumePage = iemq_guessPageConsumed ( pThreadData, Q, page, pBlockingOId);

    //If we still think we can delete the page, check properly with the lockmanager
    if (consumePage)
    {
        uint64_t storedMsgCount = 0;
        for (iemqQNode_t *node = lastNode; node >= &(page->nodes[0]);
                node--)
        {
            if (iemq_isNodeConsumed(pThreadData, Q, node))
            {
                // count how many store references we have freed from
                // the page we may be about to delete
                if (node->inStore)
                    storedMsgCount++;
            }
            else
            {
                consumePage = false;
                *pBlockingOId = node->orderId;
                break;
            }
        }

        if (consumePage)
        {
            *pNodesInStore = storedMsgCount;
        }
    }

    return consumePage;
}

// Used in fullcleanpagesscan when removing a run of pages from the middle of a queue
static void iemq_moveCursorsFromConsumedPages( ieutThreadData_t *pThreadData
                                             , iemqQueue_t *Q
                                             , iemqQNodePage_t *beforePage
                                             , iemqQNodePage_t *afterPage)
{
    assert(beforePage != NULL);
    assert(afterPage  != NULL);

    //Check any consumers who are redelivering, selecting or browsing haven't got cursors at the pages we're freeing
    int os_rc = pthread_rwlock_rdlock(&(Q->waiterListLock));
    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "Unable to take waiterListLock", ISMRC_Error
                , "Q", Q, sizeof(*Q)
                , "os_rc", &os_rc, sizeof(os_rc)
                , "InternalName", Q->InternalName, sizeof(Q->InternalName)
                , NULL);
    }

    ismEngine_Consumer_t *pFirstConsumer = Q->firstWaiter;
    if (pFirstConsumer  != NULL)
    {
        iemqQNode_t *safenode = &(afterPage->nodes[0]);
        uint64_t lastBeforeNodeOId = beforePage->nodes[(beforePage->nodesInPage)-1].orderId; //nodes[nodesInPage]= guard node
        iemqCursor_t newCursor;

        newCursor.c.orderId = safenode->orderId;
        newCursor.c.pNode   = safenode;

        ismEngine_Consumer_t *pConsumer = pFirstConsumer;
        do
        {
            if (  ! (pConsumer->fDestructiveGet)
                ||  (pConsumer->fRedelivering)
                ||  (pConsumer->selectionRule != NULL))
            {
                bool done = false;
                iemqCursor_t oldCursor = pConsumer->iemqCursor;

                while (!done &&
                         (   (oldCursor.c.orderId > lastBeforeNodeOId)
                          && (oldCursor.c.orderId < safenode->orderId)))
                {
                    done = __sync_bool_compare_and_swap(
                            &(pConsumer->iemqCursor.whole), oldCursor.whole,
                            newCursor.whole);

                    if (!done)
                    {
                        //Update our view of where the cursor is....
                        oldCursor = pConsumer->iemqCursor;
                    }
                }

                ieutTRACEL(pThreadData, pConsumer, ENGINE_NORMAL_TRACE,
                        "Consumer cursor moved to to OId %ld for QId %d Consumer %p\n",
                        newCursor.c.orderId, Q->qId, pConsumer);
                ieutTRACE_HISTORYBUF(pThreadData, oldCursor.c.orderId );
                ieutTRACE_HISTORYBUF(pThreadData, newCursor.c.orderId);
            }
            pConsumer = pConsumer->iemqPNext;
        }
        while (pConsumer != pFirstConsumer);
    }

    pthread_rwlock_unlock(&(Q->waiterListLock));
}

static void  iemq_fullCleanPagesScan( ieutThreadData_t *pThreadData
                                    , iemqQueue_t *Q)
{
    ieutTRACEL(pThreadData, Q, ENGINE_UNUSUAL_TRACE, FUNCTION_ENTRY " Q=%p\n",
               __func__, Q);

    uint64_t pagesRemovedByFullScan = 0;

    iemqQNodePage_t *firstPageToFree = NULL;
    iemqQNodePage_t *lastPageToFree = NULL;

    uint64_t discardablePagesEstimate = 0;
    uint64_t pagesCount = 0;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    //First - taking minimal locks roughly count how many pages we could get rid of
    iemq_takeReadHeadLock(Q);
    if (Q->headPage->nextStatus == completed)
    {
        uint64_t nonConsumedOId;

        //We won't look at the head page, that can be freed just by cleanHeadPages,
        iemqQNodePage_t *pageToScan = Q->headPage->next;

        pagesCount = 1;

        while (pageToScan->nextStatus == completed)
        {
            if (iemq_guessPageConsumed( pThreadData, Q, pageToScan, &nonConsumedOId))
            {
                discardablePagesEstimate++;
            }
            pagesCount++;
            pageToScan = pageToScan->next;
        }
    }
    iemq_releaseHeadLock(Q);

    //If it looks like we can throw away ~10% of pages then  it's worth doing, else leave
    //alone
    if (discardablePagesEstimate * 10 <= pagesCount)
    {
        goto mod_exit;
    }

    //We seem to have a lot of wasted pages on this queue take some locks and
    //examine all the pages and see what can go. Although we are not going to
    //alter the headPage - we definitely need a WRITE lock so browsers etc
    //can't be scanning down the queue whilst we remove pages from the middle
    iemq_takeWriteHeadLock(Q);

    int os_rc = pthread_mutex_lock(&(Q->getlock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_003, true, "Taking getlock failed.", ISMRC_Error
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , NULL);
    }

    if (Q->headPage->nextStatus == completed)
    {
        //We won't look at the head page, that can be freed just by cleanHeadPages,
        iemqQNodePage_t *pageToScan = Q->headPage->next;
        iemqQNodePage_t *prevPageToLeave = Q->headPage;
        bool deleteBatchInProgress = false; //When we are scanning consumed pages - this is true

        while (pageToScan->nextStatus == completed)
        {
            uint64_t aliveOId = 0;
            uint64_t storedMsgCount = 0;

            if (iemq_isPageFullyConsumed( pThreadData
                                        , Q, pageToScan, &aliveOId, &storedMsgCount))
            {
                iemqQNodePage_t *nextPageToScan = pageToScan->next;
                prevPageToLeave->next = nextPageToScan;
                deleteBatchInProgress = true;

                //If the getlock is pointing to the page we are going to free, rewind it to
                //safety
                uint64_t cursorOId = Q->getCursor.c.orderId;

                if (     (cursorOId >= pageToScan->nodes[0].orderId)
                      && (cursorOId <= pageToScan->nodes[(pageToScan->nodesInPage)-1].orderId)) //nodes[nodesInPage]= guard node
                {
                    iemqCursor_t msgCursor;

                    iemqQNode_t *pnode = &(prevPageToLeave->nodes[(prevPageToLeave->nodesInPage)-1]);

                    msgCursor.c.orderId = pnode->orderId;
                    msgCursor.c.pNode = pnode;

                    assert(msgCursor.c.orderId > 0);

                    iemq_rewindGetCursor(pThreadData, Q, msgCursor);
                }

                if (firstPageToFree == NULL)
                {
                    firstPageToFree = pageToScan;
                }
                if (lastPageToFree != NULL)
                {
                    assert(lastPageToFree->next == NULL);
                    lastPageToFree->next = pageToScan;
                }
                lastPageToFree = pageToScan;
                lastPageToFree->next = NULL;

                pageToScan = nextPageToScan;

                //We're removing messages from the middle of the queue, but increase the counter
                //It's just a crude measure to encourage iemq_cleanupHeadPages() to update minimumActiveOid
                //next time it deletes a page
                Q->deletedStoreRefCount += storedMsgCount;
            }
            else
            {
                //Found a page we don't want to delete...if this marks the end of a run of consumed pages, move the cursors
                if (deleteBatchInProgress)
                {
                    iemq_moveCursorsFromConsumedPages( pThreadData
                                                     , Q
                                                     , prevPageToLeave
                                                     , pageToScan);
                    deleteBatchInProgress = false;
                }

                prevPageToLeave = pageToScan;
                pageToScan      = pageToScan->next;
            }
        }


        //Were the last pages we scanned, pages that will be freed?
        if (deleteBatchInProgress)
        {
            iemq_moveCursorsFromConsumedPages( pThreadData
                                             , Q
                                             , prevPageToLeave
                                             , pageToScan);
            deleteBatchInProgress = false;
        }
    }

    os_rc = pthread_mutex_unlock(&(Q->getlock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC(ieutPROBE_002, true, "Releasing getlock failed.", os_rc
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , NULL);
    }

    iemq_releaseHeadLock(Q);


    //Finally free the pages that have been removed from the list of pages
    iere_primeThreadCache(pThreadData, resourceSet);
    while (firstPageToFree != NULL)
    {
        iemqQNodePage_t *tmp = firstPageToFree;
        firstPageToFree = firstPageToFree->next;
        
        if ((pagesRemovedByFullScan & 0xFF) == 0) //trace every 256 pages we remove
        {
            ieutTRACEL(pThreadData,tmp->nodes[0].orderId,  ENGINE_NORMAL_TRACE, "oId of firstnode in freedpage: %lu\n",
                tmp->nodes[0].orderId);
        }
        iere_freeStruct(pThreadData, resourceSet, iemem_multiConsumerQPage, tmp, tmp->StrucId);
        pagesRemovedByFullScan++;
    }

mod_exit:
    ieutTRACEL(pThreadData,pagesRemovedByFullScan, ENGINE_UNUSUAL_TRACE, FUNCTION_EXIT " pages=%lu\n",
               __func__, pagesRemovedByFullScan);
}

static int32_t iemq_cleanupHeadPages( ieutThreadData_t *pThreadData
                                    , iemqQueue_t *Q)
{
    int32_t rc = OK;
    iemqQNodePage_t *firstPageToDelete;
    iemqQNodePage_t *lastPageToDelete = NULL;

    uint64_t storedMsgCount;
    uint64_t aliveOId=0; //If we see a message we can't delete yet, record it for diagnostic trace
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    if (iemq_proceedIfUsersBelowLimit( &(Q->cleanupScanThdCount), 1))
    {
        //Dirty read first page... if that shows unconsumed nodes, no point taking write locks
        iemq_takeReadHeadLock(Q);
        bool headPageConsumeGuess = iemq_guessPageConsumed ( pThreadData, Q, Q->headPage, &aliveOId);
        iemq_releaseHeadLock(Q);

        if (headPageConsumeGuess)
        {
            iemq_takeWriteHeadLock(Q);

            bool consumePage = true;
            firstPageToDelete = Q->headPage; //"Optimistically" add head page to list.. won't do anything unless
            lastPageToDelete = NULL;        //last page is set as well

            //Has someone got an operation in progress that means we shouldn't perform a clean?
            //(they can't set that at the mo as we have the write head lock and it is set with read held)

            if (Q->freezeHeadCleanupOps == 0)
            {
                do
                {
                    iemqQNodePage_t *curHead = Q->headPage;

                    if (curHead->nextStatus != completed)
                    {
                        //Can't delete only page!
                        break;
                    }

                    consumePage = iemq_isPageFullyConsumed(pThreadData, Q, curHead, &aliveOId, &storedMsgCount);

                    if (consumePage)
                    {
                        iemqQNode_t *lastNode = &(curHead->nodes[curHead->nodesInPage - 1]);

                        //Want to check that /after/ we know all the page entries are consumed the cursors are pointing after this page
                        //but the releasing of the instantlock counts as a memory barrier so don't need an explicit one here
                        if ((Q->getCursor.c.orderId > lastNode->orderId)
                                && (Q->scanOrderId > lastNode->orderId))
                        {
                            if ((ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_1]
                                    != StatusOk)
                                    || Q->deletedStoreRefCount
                                    > ieqMINACTIVEORDERID_UPDATE_INTERVAL)
                            {
                                uint64_t minimumActiveOrderId = lastNode->orderId + 1;

                                ieutTRACEL(pThreadData, minimumActiveOrderId, ENGINE_HIFREQ_FNC_TRACE,
                                           "Pruning references for MCQ[0x%lx] qId %u minimumActiveOrderId %lu\n",
                                           Q->hStoreObj, Q->qId, minimumActiveOrderId);

                                // Update min active orderId
                                rc = ism_store_setMinActiveOrderId( pThreadData->hStream,
                                                                    Q->QueueRefContext,
                                                                    minimumActiveOrderId );
                                if (rc != OK)
                                {
                                    ieutTRACE_FFDC(ieutPROBE_001, true,
                                                   "ism_store_setMinActiveOrderId failed.", rc
                                                   , "minActiveOrderId", &(minimumActiveOrderId), sizeof(uint64_t)
                                                   , NULL);
                                }

                                Q->deletedStoreRefCount = 0;
                            }

                            lastPageToDelete = curHead;
                            Q->headPage = curHead->next;

                            // Increment the counter on how many references may have
                            // been removed
                            Q->deletedStoreRefCount += storedMsgCount;
                        }
                        else
                        {
                            //don't consume the page whilst the getter can still read it
                            consumePage = false;
                        }
                    }
                }
                while (consumePage);
            }

            iemq_releaseHeadLock(Q);

            if ((rc == OK) && (lastPageToDelete != NULL))
            {
                lastPageToDelete->next = NULL; //break chain between deleted pages and pages to keep

                iere_primeThreadCache(pThreadData, resourceSet);
                while (firstPageToDelete != NULL)
                {
                    iemqQNodePage_t *tmp = firstPageToDelete;
                    firstPageToDelete = firstPageToDelete->next;

                    iere_freeStruct(pThreadData, resourceSet, iemem_multiConsumerQPage, tmp, tmp->StrucId);
                }
            }
        }

        ieutTRACEL(pThreadData, aliveOId, ENGINE_HIFREQ_FNC_TRACE,
                   FUNCTION_IDENT "aliveOId=%lu, nextOId=%lu, buffered=%lu\n", __func__,
                   aliveOId, Q->nextOrderId, Q->bufferedMsgs);

#define IEMQ_FULLGC_WASTEDNODE_RATIO      70

//Even for unlimited Qs you have to ack your messages before this many messages are put afterwards
#define IEMQ_MAX_UNACKED_FOR_UNLIMITEDQ   1000000000

        //If we didn't scan to first alive node, e.g. all nodes on head consumed but cursor still there
        //Then we don't know enough to decide whether we need to e.g. a force remove of a bad acker
        if (aliveOId > 0)
        {
            //Did we get rid of enough pages?
            uint64_t ratio;

            //added 1 to denominator to avoid divide by 0
            uint64_t policyMaxMsgCount = Q->Common.PolicyInfo->maxMessageCount;
            uint64_t maxMsgCount = policyMaxMsgCount;
            uint64_t curDepth = Q->bufferedMsgs;
            if (Q->bufferedMsgs > maxMsgCount)
            {
                //If we're not reclaming space fast enough, don't penalise some poor client
                maxMsgCount = curDepth;
            }

            uint64_t qRange = Q->nextOrderId - aliveOId;
            ratio = qRange/(1+maxMsgCount);
            bool doingRemove = false;

            if (   (    (policyMaxMsgCount > 0)
                     && (ratio > ismEngine_serverGlobal.DestroyNonAckerRatio)
                     && (ismEngine_serverGlobal.DestroyNonAckerRatio > 0) )
                || (    (policyMaxMsgCount == 0)
                     && (Q->bufferedMsgs > IEMQ_MAX_UNACKED_FOR_UNLIMITEDQ)
                     && (ismEngine_serverGlobal.DestroyNonAckerRatio > 0) ) )
            {
                //Things are really severe. If that orderId has not been consumed already,
                //we're going to delete all the state for the client
                doingRemove = iemq_forceRemoveBadAcker(pThreadData, Q, aliveOId);
            }

            if (!doingRemove && qRange/(1+curDepth) > IEMQ_FULLGC_WASTEDNODE_RATIO)
            {
                //Maybe there is something blocking us at the start of the queue (e.g. an old
                //long running transaction) but we have so many wasted pages it is worth scanning
                //the whole queue looking for pages in the middle we can free.. it's painful
                //so we'll only do it every 8 times we think we might need to

                if (Q->cleanupsNeedingFull == 0 )
                {
                    iemq_fullCleanPagesScan(pThreadData, Q);
                }

                if (Q->cleanupsNeedingFull < 7)
                {
                    Q->cleanupsNeedingFull++;
                }
                else
                {
                    Q->cleanupsNeedingFull = 0;
                }
            }
            else
            {
                //Don't need to atomically write... protected by cleanupScanThdCount
                Q->cleanupsNeedingFull = 0;
            }
        }

        //Let another thread do cleanup now
        iemq_releaseUserCount(&(Q->cleanupScanThdCount));
    }

    return rc;
}

static inline iemqSLEConsume_t *iemq_getCachedSLEConsumeMem(
                                             ietrSLE_Header_t *pSLE)
{
    //The consume specific SLE data is stored after the generic SLE header....
    iemqSLEConsume_t *pSLEConsume = (iemqSLEConsume_t *) (pSLE + 1);

    return pSLEConsume;
}

//@returns an SLE entry big enough for consume or NULL if alloc failed
static inline ietrSLE_Header_t *iemq_reserveSLEMemForConsume(ieutThreadData_t *pThreadData)
{
    ietrSLE_Header_t *pSLE = (ietrSLE_Header_t *) iemem_malloc(pThreadData,
                                                               IEMEM_PROBE(iemem_localTransactions, 5),
                                                               sizeof(ietrSLE_Header_t) + sizeof(iemqSLEConsume_t));

    if (pSLE != (ietrSLE_Header_t *) NULL)
    {
        ismEngine_SetStructId(pSLE->StrucId, ietrSLE_STRUC_ID);

        iemqSLEConsume_t *consumeSpecificSLE = iemq_getCachedSLEConsumeMem(pSLE);

        int32_t rc = ielm_allocateCachedLockRequest(pThreadData,
                                                    &(consumeSpecificSLE->hCachedLockRequest));

        if (rc != OK)
        {
            iemem_freeStruct(pThreadData, iemem_localTransactions, pSLE, pSLE->StrucId);

            pSLE = NULL;
        }
    }

    return pSLE;
}

static inline void iemq_releaseReservedSLEMem( ieutThreadData_t *pThreadData
                                             , iemqQNode_t *qnode)
{
    if (qnode->iemqCachedSLEHdr != NULL)
    {
        iemqSLEConsume_t *consumeData = iemq_getCachedSLEConsumeMem(
                                            qnode->iemqCachedSLEHdr);
        if (consumeData->hCachedLockRequest != NULL)
        {
            ielm_freeLockRequest(pThreadData, consumeData->hCachedLockRequest);
            consumeData->hCachedLockRequest = NULL;
        }

        iemem_freeStruct(pThreadData, iemem_localTransactions, qnode->iemqCachedSLEHdr, qnode->iemqCachedSLEHdr->StrucId);
        qnode->iemqCachedSLEHdr = NULL;
    }
}

static inline void iemq_checkCachedMemoryExists( iemqQueue_t *Q
                                               , iemqQNode_t *pnode)
{
    if (pnode->iemqCachedSLEHdr == NULL)
    {
        ieutTRACE_FFDC( ieutPROBE_001, true,
                           "No cached memory for use in transactional acknowledge.", ISMRC_Error
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                      , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                      , "pNode", pnode, sizeof(iemqQNode_t)
                      , NULL);
    }
}

//Performs  one store commit (committing the removal of the reference)
//(Potentially the message record is scheduled for lazy removal)
static inline int32_t iemq_consumeMessageInStore( ieutThreadData_t *pThreadData
                                                , iemqQueue_t *Q
                                                , iemqQNode_t *pnode)
{
    assert(pnode->inStore);

    iest_AssertStoreCommitAllowed(pThreadData);

    //   ism_store_deleteReference() must be in a separate earlier transaction than
    //   iest_unstoreMessage(), as if iest_unstoreMessage decrements
    //   the message use count to 1, another thread might also call
    //   iest_unstoreMessage() for the same message, which will cause
    //   the message to be deleted from the store while our
    //   delete_reference has not yet been committed. If the server
    //   ends at this point we get a failure at restart with a
    //   lost message!
    //
    // We can (and do) lazily remove the message record. If we don't complete
    // the removal of the record, we risk leaving an orphan but that is not
    // a serious problem.
    int32_t rc = iest_store_deleteReferenceCommit( pThreadData
                                                 , pThreadData->hStream
                                                 , Q->QueueRefContext
                                                 , pnode->hMsgRef
                                                 , pnode->orderId
                                                 , 0);
    if (UNLIKELY(rc != OK))
    {
        // The failure to delete a store reference means that the
        // queue is inconsistent.
        ieutTRACE_FFDC( ieutPROBE_001, true,
                                     "ism_store_deleteReference (multiConsumer) failed.", rc
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                      , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                      , "pNode", pnode, sizeof(iemqQNode_t)
                      , NULL);
    }

    // Reduce our usecount on the message and potential schedule it for
    // deletion in the next store transaction.
    uint32_t NeedStoreCommit = 0;
    int32_t rc2 = iest_unstoreMessage(pThreadData, pnode->msg, false, true,
                                      NULL, &NeedStoreCommit);
    if (UNLIKELY(rc2 != OK))
    {
        // If we fail to unstore a message there is not much we
        // can actually do. So continue onwards and try and make
        // our state consistent.
        ieutTRACE_FFDC( ieutPROBE_002, false,
                            "iest_unstoreMessage (multiConsumer) failed.", rc
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                      , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                      , "pNode", pnode, sizeof(iemqQNode_t)
                      , NULL);
    }

    assert(NeedStoreCommit == 0); //any unstore of the message should have been lazy scheduled

    return rc;
}

static inline void iemq_addConsumeMsgSLE(
                               ieutThreadData_t *pThreadData
                             , iemqQueue_t *Q
                             , iemqQNode_t *pNode
                             , ismEngine_Session_t *pSession
                             , ismEngine_Transaction_t *pTran
                             , ismEngine_Consumer_t *pConsumerToDeack)
{
    int32_t rc = OK;

    assert(pTran != NULL);

    // We are going to use the memory we reserved at delivery time for the SLE
    assert(pNode->iemqCachedSLEHdr != NULL);
    iemqSLEConsume_t *pConsumeSLE = iemq_getCachedSLEConsumeMem(
                                        pNode->iemqCachedSLEHdr);

    pConsumeSLE->pQueue = Q;
    pConsumeSLE->pNode = pNode;
    pConsumeSLE->pMsg = pNode->msg;
    pConsumeSLE->bInStore = pNode->inStore;
    pConsumeSLE->bCleanHead = false;
    pConsumeSLE->pSession = pSession;

    // Add the message to the Soft log so we get called back during commit etc
    rc = ietr_softLogAddPreAllocated(pThreadData, pTran, ietrSLE_PREALLOCATED_MQ_CONSUME_MSG,
                                     iemq_SLEReplayConsume, NULL,
                                     Commit | PostCommit | Rollback | MemoryRollback | PostRollback,
                                     pNode->iemqCachedSLEHdr, 1, 1);

    if (rc != OK)
    {
        //As we provide our own memory the only thing that can go wrong are
        //non-recoverable things like transaction is already prepared
        ieutTRACE_FFDC( ieutPROBE_001, false,
                            "ietr_softLogAddPreAllocated failed.", rc
                          , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                          , "Queue", Q, sizeof(iemqQueue_t)
                          , "Reference", &pNode->hMsgRef, sizeof(pNode->hMsgRef)
                          , "OrderId", &pNode->orderId, sizeof(pNode->orderId)
                          , "pNode", pNode, sizeof(iemqQNode_t)
                          , NULL);
    }

    //We no longer own the cached SLE memory now we added it to the soft log...
    pNode->iemqCachedSLEHdr = NULL;

    return;
}

static inline void iemq_destroyMessageBatch_finish( ieutThreadData_t *pThreadData
                                                  , iemqQueue_t *Q
                                                  , uint32_t batchSize
                                                  , iemqQNode_t *discardNodes[]
                                                  , bool *doPageCleanup)
{
    for (uint32_t i=0; i < batchSize; i++)
    {
        iemqQNode_t *pnode = discardNodes[i];

        // Before releasing our hold on the node, should we subsequently call
        // cleanupHeadPage?
        if ((pnode+1)->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
        {
            (*doPageCleanup) = true;
        }

        //Didn't consume the node transactional... free the memory we reserved
        iemq_releaseReservedSLEMem(pThreadData, pnode);

        ismEngine_Message_t *pMsg = pnode->msg;
        pnode->msg = MESSAGE_STATUS_EMPTY;

        iem_releaseMessage(pThreadData, pMsg);

        pnode->msgState = ismMESSAGE_STATE_CONSUMED;
    }
}

static inline int32_t iemq_consumeMessageBatch_unstoreMessages( ieutThreadData_t *pThreadData
                                                              , iemqQueue_t *Q
                                                              , uint32_t batchSize
                                                              , iemqQNode_t *discardNodes[]
                                                              , ismEngine_AsyncData_t *pAsyncData )
{
    uint32_t storeOpsCount = 0;
    int32_t rc = OK;

    for (uint32_t i=0; i < batchSize; i++)
    {
        iemqQNode_t *pnode = discardNodes[i];

        // Decrement our reference to the msg and possibly schedule it's later deletion from the store
        // from the store.
        if (pnode->inStore)
        {
            assert(pnode->hasMDR == false);

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

static iemqQNode_t **getDiscardNodesFromAsyncInfo(ismEngine_AsyncData_t *asyncInfo)
{
    iemqQNode_t **pDiscardNodes = NULL;

    uint32_t entry2 = asyncInfo->numEntriesUsed-1;
    uint32_t entry1 = entry2 - 1;

    if (   (asyncInfo->entries[entry1].Type != iemqQueueDestroyMessageBatch1)
         ||(asyncInfo->entries[entry2].Type != iemqQueueDestroyMessageBatch2))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true,
            "asyncInfo stack not as expected (corrupted?)", ISMRC_Error,
            NULL);
    }

    pDiscardNodes = (iemqQNode_t **)(asyncInfo->entries[entry1].Data);

    return pDiscardNodes;
}


int32_t iemq_asyncDestroyMessageBatch(
                ieutThreadData_t               *pThreadData,
                int32_t                         retcode,
                ismEngine_AsyncData_t          *asyncinfo,
                ismEngine_AsyncDataEntry_t     *context)
{
    iemqAsyncDestroyMessageBatchInfo_t *asyncData = (iemqAsyncDestroyMessageBatchInfo_t *)context->Data;
    iemqQNode_t **pDiscardNodes = getDiscardNodesFromAsyncInfo(asyncinfo);

    bool doPageCleanup = false;
    assert(retcode == OK);
    int32_t rc = OK;

    iemqQueue_t *Q = asyncData->Q;

    ismEngine_CheckStructId(asyncData->StrucId, IEMQ_ASYNCDESTROYMESSAGEBATCHINFO_STRUCID, ieutPROBE_001);


    if (asyncData->removedStoreRefs == false)
    {
        //This is the first time this callback has been called since the references have been
        //removed from the store. We need to unstore the messages....
        asyncData->removedStoreRefs = true;

        rc = iemq_consumeMessageBatch_unstoreMessages( pThreadData
                                                     , Q
                                                     , asyncData->batchSize
                                                     , pDiscardNodes
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
                                  ((asyncData->batchSize) * sizeof(iemqQueue_t *)));

    iemq_destroyMessageBatch_finish( pThreadData
                                   , Q
                                   , asyncData->batchSize
                                   , pDiscardNodes
                                   , &doPageCleanup);

    if (doPageCleanup)
    {
        iemq_cleanupHeadPages(pThreadData, Q);
    }

    //Increased when we realised destroying this batch might go async
    iemq_reducePreDeleteCount(pThreadData, (ismQHandle_t)Q);
mod_exit:
   return rc;
}

///  @remark Nodes should be marked as ieqMESSAGE_STATE_DISCARDING whilst locked prior to calling this function
static inline void iemq_destroyMessageBatch( ieutThreadData_t *pThreadData,
                                             iemqQueue_t *Q,
                                             uint32_t batchSize,
                                             iemqQNode_t *discardNodes[],
                                             bool removeExpiryData,
                                             bool *doPageCleanup)
{
    //If any nodes are in the store, we need two store commits, one to remove the reference
    //then after that is committed, we can reduce our store usecount of the message
    //and maybe remove the message. The order is important if iest_unstoreMessage decrements
    //the message use count to 1, another thread might also call iest_unstoreMessage() for the same message,
    //which will cause the message to be deleted from the store while our delete_reference has
    //not yet been committed. If the server ends at this point we get a failure at restart with a
    //lost message! For single message removal we defer the removal of the message until next time we
    //do a store commit - that's not appropriate for a batch removal

    iest_AssertStoreCommitAllowed(pThreadData);

    bool nodesInStore = false;
    bool increasedPreDeleteCount = false;
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    //Reduce depth and bytes counts early so we don't keep discarding more than we need to
    iere_primeThreadCache(pThreadData, resourceSet);
    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -(int64_t)batchSize);
    pThreadData->stats.bufferedMsgCount -= batchSize;
    DEBUG_ONLY int32_t oldDepth = __sync_fetch_and_sub(&(Q->bufferedMsgs), batchSize);
    assert(oldDepth >= batchSize);

    uint64_t messageBytes  = 0;

    for (uint32_t i=0; i < batchSize; i++)
    {
        iemqQNode_t *node = discardNodes[i];
        assert(node->msgState == ieqMESSAGE_STATE_DISCARDING);
        assert(node->hasMDR == false); // we won't be discarding it, if it's inflight...right?

        if (removeExpiryData && node->msg->Header.Expiry != 0)
        {
            ieme_removeMessageExpiryData( pThreadData
                                        , (ismEngine_Queue_t *)Q
                                        , node->orderId);
        }

        if (node->inStore)
        {
             rc = ism_store_deleteReference( pThreadData->hStream
                                           , Q->QueueRefContext
                                           , node->hMsgRef
                                           , node->orderId
                                           , 0);
           if (UNLIKELY(rc != OK))
           {
               // The failure to delete a store reference means that the
               // queue is inconsistent.
               ieutTRACE_FFDC( ieutPROBE_001, true,
                                            "ism_store_deleteReference (multiConsumer) failed.", rc
                             , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                             , "Queue", Q, sizeof(iemqQueue_t)
                             , "Reference", &node->hMsgRef, sizeof(node->hMsgRef)
                             , "OrderId", &node->orderId, sizeof(node->orderId)
                             , "pNode", node, sizeof(iemqQNode_t)
                             , NULL);
           }
           nodesInStore = true;
        }

        messageBytes += IEMQ_MESSAGE_BYTES(node->msg);
    }
    if ((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0)
    {
        pThreadData->stats.remoteServerBufferedMsgBytes -= messageBytes;
        (void)__sync_fetch_and_sub(&(Q->bufferedMsgBytes), messageBytes);
    }

    if (nodesInStore)
    {
        //Get ready in case we need to go async, we don't need to tie it to completion of any async stack of functions we're already in
        //Increase the predelete count of the queue so the queue won't go away.
        increasedPreDeleteCount = true;
        __sync_fetch_and_add(&(Q->preDeleteCount), 1);

        //If it's possible we will be restarting delivery (i.e. freeing delivery ids) we would need to increase usecount on consumer/session
        //here.... but if it has a deliveryid we shouldn't be destroying it.


        iemqAsyncDestroyMessageBatchInfo_t asyncBatchData = {
                                                     IEMQ_ASYNCDESTROYMESSAGEBATCHINFO_STRUCID
                                                   , Q
                                                   , batchSize
                                                   , false
                                                };

        ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
                {ismENGINE_ASYNCDATAENTRY_STRUCID, iemqQueueDestroyMessageBatch1,  discardNodes,  batchSize*sizeof(iemqQNode_t *), NULL, {.internalFn = iemq_asyncDestroyMessageBatch } },
                {ismENGINE_ASYNCDATAENTRY_STRUCID, iemqQueueDestroyMessageBatch2,  &asyncBatchData,   sizeof(asyncBatchData),     NULL, {.internalFn = iemq_asyncDestroyMessageBatch }}};
        ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID, NULL, IEAD_MAXARRAYENTRIES, 2, 0, true,  0, 0, asyncArray};

        // We need to commit the removal of the reference before we can call iest_unstoreMessage (in case we reduce the usage
        // count to 1, some else decreases it to 0 and commits but we don't commit our transaction. On restart we'd point to a message
        // that has been deleted...
        rc = iead_store_asyncCommit(pThreadData, false, &asyncData);

        if (rc != ISMRC_AsyncCompletion)
        {
            // We're ready to remove the messages, the removal of references to them has already been committed
            asyncBatchData.removedStoreRefs = true;

            rc = iemq_consumeMessageBatch_unstoreMessages( pThreadData
                                                         , Q
                                                         , batchSize
                                                         , discardNodes
                                                         , &asyncData);
        }
    }

    if (rc !=  ISMRC_AsyncCompletion)
    {
        iemq_destroyMessageBatch_finish( pThreadData
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
                          , "iemq_destroyMessageBatch failed in unexpected way", rc
                          , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                          , "Queue", Q, sizeof(iemqQueue_t)
                          , NULL );
        }
        if (increasedPreDeleteCount)
        {
            iemq_reducePreDeleteCount(pThreadData, (ismQHandle_t)Q);
            increasedPreDeleteCount = false;
        }
    }

    return;
}


//On remote servers, we don't want to throw away current retained messages
//So this function is called for each function to decide whether we can discard the node
static inline int32_t iemq_considerReclaimingNode( ieutThreadData_t *pThreadData
                                                 , iemqQueue_t *Q
                                                 , iemqQNode_t *scanNode
                                                 , iemqCursor_t *nextToConsider
                                                 , iemqQNode_t **pFirstSkippedNode)
{
    iemqQNode_t *subsequentNode = NULL;

    assert(Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE);

    int32_t rc = iemq_markMessageIfGettable(pThreadData, Q, scanNode, &subsequentNode);

    if (    (rc == OK)
         && (scanNode->msg->Header.Flags & ismMESSAGE_FLAGS_PROPAGATE_RETAINED))
    {
        //Bad discarder, no biscuit.
        scanNode->msgState = ismMESSAGE_STATE_AVAILABLE;
        if (*pFirstSkippedNode == NULL)
        {
            *pFirstSkippedNode  = scanNode;
            ieutTRACEL(pThreadData, scanNode->orderId, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "skipped=%lu\n",
                                      __func__, scanNode->orderId);
        }
        rc = ISMRC_NoMsgAvail;
    }

    if (subsequentNode != NULL)
    {
        nextToConsider->c.pNode = subsequentNode;
        nextToConsider->c.orderId = subsequentNode->orderId;
    }
    else
    {
        nextToConsider->c.pNode = NULL;
        nextToConsider->c.orderId = 0;
    }

    return rc;
}

static inline void   iemq_resyncDiscardCursors( ieutThreadData_t *pThreadData
                                              , iemqQueue_t *Q
                                              , iemqCursor_t *lastKnownGetCursor
                                              , iemqCursor_t *nextToScanCursor
                                              , bool *pInStepWithGetCursor )
{
    if (  (lastKnownGetCursor->c.orderId == 0)
        ||(lastKnownGetCursor->c.orderId > Q->getCursor.c.orderId))
    {
        //Need to set the discard cursor back to the get cursor
        *lastKnownGetCursor    =  Q->getCursor;
        *nextToScanCursor      = *lastKnownGetCursor;
        *pInStepWithGetCursor  = true;
    }
    else
    {
        *lastKnownGetCursor = Q->getCursor;

        if (nextToScanCursor->c.orderId < lastKnownGetCursor->c.orderId)
        {
            *nextToScanCursor = *lastKnownGetCursor;
            *pInStepWithGetCursor = true;
        }
        else if (nextToScanCursor->c.orderId == lastKnownGetCursor->c.orderId)
        {
            *pInStepWithGetCursor = true;
        }
        else
        {
            *pInStepWithGetCursor = false;
        }
    }
    ieutTRACEL(pThreadData, *pInStepWithGetCursor, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "%c\n",
                                  __func__,  *pInStepWithGetCursor ? '1':'0');
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Remove messages from the front of a remote server queue to make space for new messages
///
///  @param[in] Qhdl               - Queue
///  @param[in] takeLock             Ignored for multiconsumerQ
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////

void iemq_reclaimSpaceRemoteServer( ieutThreadData_t *pThreadData
                                  , iemqQueue_t *Q)
{
    uint64_t removedMsgs = 0;
    int32_t rc = OK;
    bool scannedAllMessages = false;
    bool doPageCleanup = false; //Do we need to try and throw away empty pages at the start of the queue
    uint32_t discardBatchSize = 50; //discard this many messages in a batch

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n",  __func__, Q);

    // Because we're a remote server we reclaim space by bytes
    assert (Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE);
    iepiPolicyInfo_t *pPolicyInfo = Q->Common.PolicyInfo;
    uint64_t targetBufferedMsgBytes;

    assert(pPolicyInfo->maxMessageCount == 0);

    targetBufferedMsgBytes = (uint64_t)(pPolicyInfo->maxMessageBytes * IEQ_RECLAIMSPACE_MSGBYTES_SURVIVE_FRACTION);

    // If we end up trying to get down to less than 1k, let's aim for half the current queue size
    if (UNLIKELY(targetBufferedMsgBytes < 1024))
    {
        targetBufferedMsgBytes = Q->bufferedMsgBytes/2;
    }

    iemqCursor_t lastKnownGetCursor = { .c = {0, NULL} };
    iemqCursor_t nextToScanCursor = { .c = {0, NULL} };

    //If we take lock and start scanning from a point that isn't the get cursor, we have no right to
    //change get cursor at all...
    bool updateGetCursor = true;

    uint64_t counter = 0;
    uint64_t maxToRemoveBytes = 0;
    uint64_t removedBytes = 0;

    while (   (targetBufferedMsgBytes != 0)
           && (Q->bufferedMsgBytes > targetBufferedMsgBytes)
           && !scannedAllMessages
           && ((maxToRemoveBytes == 0)||(removedBytes < maxToRemoveBytes)))
    {
        iemqQNode_t *discardNodes[discardBatchSize];
        uint32_t currentBatchSize = 0;
        uint64_t currentBatchBytes = 0;

        iemqQNode_t *scanNode = NULL;
        iemqQNode_t *firstSkippedNode = NULL;

        //Grab the getlock, remove 50 messages, release getlock repeat
        if (iemq_lockMutexWithWaiterLimit( &(Q->getlock)
                                         , &(Q->discardingWaitingThdCount)
                                         , 1))
        {
            //No scan should be in progress so scan orderId should not point to an orderId for a valid node
            assert(Q->scanOrderId == IEMQ_ORDERID_PAST_TAIL);

            //Now we have the getlock set a limit to the number that this thread will remove based on how
            //overdepth this queue is so this thread does not get trapped discarding messages forever
            uint64_t curbytes = Q->bufferedMsgBytes;
            if (maxToRemoveBytes == 0 && (curbytes > targetBufferedMsgBytes))
            {
                maxToRemoveBytes = 3 * (curbytes - targetBufferedMsgBytes);
            }

            iemq_resyncDiscardCursors( pThreadData
                                     , Q
                                     , &lastKnownGetCursor
                                     , &nextToScanCursor
                                     , &updateGetCursor);


            bool getCursorSetToSearching = false;

            while((currentBatchSize < discardBatchSize) &&
                  ((targetBufferedMsgBytes != 0) && ((Q->bufferedMsgBytes-currentBatchBytes) > targetBufferedMsgBytes)) &&
                   !scannedAllMessages)

            {
                scanNode = nextToScanCursor.c.pNode;


               // A little trace to track our progress (every 512 nodes)
                if (((counter++) & 0x1ff) == 0)
                {
                    ieutTRACEL(pThreadData, scanNode->orderId, ENGINE_HIFREQ_FNC_TRACE,
                               "RECLAIM SCAN: Q=%u, scanCursor counter=%lu oId=%lu node=%p\n",
                               Q->qId, counter, scanNode->orderId, scanNode);
                }

                rc = iemq_considerReclaimingNode( pThreadData
                                                , Q
                                                , scanNode
                                                , &nextToScanCursor
                                                , &firstSkippedNode);


                if (rc == OK)
                {
                    //We've locked a node
                    //Try and set the get cursor on the queue back to the last node
                    //we searched where we did not skip.
                    bool allowedNode = true;

                    if (updateGetCursor)
                    {
                        uint64_t failUpdateIfBefore;
                        iemqQNode_t *newPos;

                        if (firstSkippedNode == NULL)
                        {
                            failUpdateIfBefore = scanNode->orderId;
                            newPos = nextToScanCursor.c.pNode;
                        }
                        else
                        {
                            failUpdateIfBefore = firstSkippedNode->orderId;
                            newPos = firstSkippedNode;
                        }

                        if (iemq_updateGetCursor(pThreadData, Q, failUpdateIfBefore, newPos))
                        {

                            // We successfully updated the get cursor, which means we can
                            // return the node to the caller.
                            getCursorSetToSearching = false;
                            lastKnownGetCursor.c.pNode = newPos;
                            lastKnownGetCursor.c.orderId = newPos->orderId;

                            if(firstSkippedNode != NULL)
                            {
                                //We have skipped some messages... from this point on,
                                //leave get cursor alone
                                updateGetCursor = false;
                                firstSkippedNode = NULL;
                            }
                        }
                        else
                        {
                            allowedNode = false;

                            lastKnownGetCursor = Q->getCursor;
                            nextToScanCursor = lastKnownGetCursor;
                            firstSkippedNode = NULL;
                            getCursorSetToSearching = false;

                        }
                    }

                    if (allowedNode)
                    {
                        assert(scanNode!= NULL);
                        discardNodes[currentBatchSize] = scanNode;
                        scanNode->msgState = ieqMESSAGE_STATE_DISCARDING;
                        currentBatchSize++;
                        currentBatchBytes += IEMQ_MESSAGE_BYTES(scanNode->msg);
                        ieutTRACE_HISTORYBUF(pThreadData, scanNode->orderId);
                    }
                    else
                    {
                        //Some evil thread has put a message before the one we were
                        //going to get and the get cursor has been rewound.
                        //It conceivably could have been put in the same transaction as
                        //the node we found so we need to let this one go but
                        //we can loop again.
                        scanNode->msgState = ismMESSAGE_STATE_AVAILABLE;
                        ieutTRACE_HISTORYBUF(pThreadData, scanNode->orderId);
                    }
                }
                else if (rc == ISMRC_NoMsgAvail)
                {
                    if (nextToScanCursor.c.orderId != 0)
                    {
                        if (updateGetCursor)
                        {
                            if (!getCursorSetToSearching)
                            {
                                //Hmm... can't get that one but there's a subsequent node (so we're
                                //not at the tail) we'll 'clear' the getcursor and scan forward looking
                                //for a message. Note that we must (re)scan the node we've just scanned
                                //after clearing the get cursor in case it has suddenly come available

                                //Note the page we're scanning so it can't get freed out from under us
                                //(The setting of this value is protected by the getLock)
                                Q->scanOrderId = scanNode->orderId;

                                bool successfulCAS = __sync_bool_compare_and_swap(
                                        &(Q->getCursor.whole), lastKnownGetCursor.whole,
                                        IEMQ_GETCURSOR_SEARCHING.whole);

                                if (successfulCAS)
                                {
                                    getCursorSetToSearching = true;
                                    nextToScanCursor.c.pNode   = scanNode; //rescan as per above comment
                                    nextToScanCursor.c.orderId = scanNode->orderId;
                                }
                                else
                                {
                                    //Someone rewound the get cursor...reread get cursor and loop around and
                                    //try again
                                    lastKnownGetCursor =  Q->getCursor;
                                    nextToScanCursor = lastKnownGetCursor;
                                    firstSkippedNode = NULL;
                                    Q->scanOrderId = IEMQ_ORDERID_PAST_TAIL;
                                }
                            }
                            else
                            {
                                //Note the page we're scanning so it can't get freed out from under us
                                //(The setting of this value is protected by the getLock)
                                Q->scanOrderId = nextToScanCursor.c.orderId;
                            }
                        }
                    }
                    else
                    {
                        //No later nodes... scan completed
                        scannedAllMessages = true;
                    }
                }
                else
                {
                    ieutTRACE_FFDC(ieutPROBE_004, true, "Unexpected error locking node for discard", rc,
                                   "Internal Name", Q->InternalName, sizeof(Q->InternalName),
                                   "Queue", Q, sizeof(iemqQueue_t), NULL);
                }
            }

            if (getCursorSetToSearching)
            {
                //We failed to find a node...try and record how far we scanned in the
                //get cursor.
                assert(updateGetCursor);
                uint64_t failUpdateIfBefore;
                iemqQNode_t *newPos;

                if (firstSkippedNode == NULL)
                {
                    failUpdateIfBefore = scanNode->orderId;
                    newPos = scanNode;
                }
                else
                {
                    failUpdateIfBefore = firstSkippedNode->orderId;
                    newPos = firstSkippedNode;
                }

                (void) iemq_updateGetCursor(pThreadData, Q, failUpdateIfBefore,
                                                                         newPos);

                //If the update failed, someone has put an earlier message on the
                //queue...
            }


            //Now remove all the messages that we just marked as delivered for discard. If we do this under the get lock it's obviously
            //slower...but if we don't and crash before we do it old messages that were in the process of being discarded
            //can't come back after later ones have been processed
            if (currentBatchSize > 0)
            {
                iemq_destroyMessageBatch( pThreadData
                                        , Q
                                        , currentBatchSize
                                        , discardNodes
                                        , true
                                        , &doPageCleanup );

                removedMsgs  += currentBatchSize;
                removedBytes += currentBatchBytes;
            }

            int os_rc = pthread_mutex_unlock(&(Q->getlock));

            if (UNLIKELY(os_rc != 0))
            {
                ieutTRACE_FFDC(ieutPROBE_003, true, "Releasing getlock failed.", rc,
                               "Internal Name", Q->InternalName, sizeof(Q->InternalName),
                               "Queue", Q, sizeof(iemqQueue_t), NULL);
            }
        }
        else
        {
            //Somebody else already reclaiming.... we're done
            break;
        }
    }
    //printf("Done discard. Removed %lu scannedAll %s maxToRemove %lu target %lu current %lu\n",
    //          removedMsgs, (scannedAllMessages ? "true":"false"), maxToRemoveBytes, targetBufferedMsgBytes, Q->bufferedMsgBytes);

    if (removedMsgs > 0)
    {
        assert(Q->Common.resourceSet == iereNO_RESOURCE_SET);
        __sync_fetch_and_add(&(Q->discardedMsgs), removedMsgs);
    }

    if (doPageCleanup)
    {
        iemq_cleanupHeadPages(pThreadData, Q);
    }

    ieutTRACEL(pThreadData, removedMsgs, ENGINE_FNC_TRACE, FUNCTION_EXIT "removed = %lu\n",
               __func__, removedMsgs);
    return;
}



///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Remove messages from the front of the queue to make space for new messages
///
///  @param[in] Qhdl               - Queue
///  @param[in] takeLock             Ignored for multiconsumerQ
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////

void iemq_reclaimSpace( ieutThreadData_t *pThreadData
                      , ismQHandle_t Qhdl
                      , bool takeLock)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;

    uint64_t removedMsgs = 0;
    int32_t rc = OK;
    bool scannedAllMessages = false;
    bool doPageCleanup = false; //Do we need to try and throw away empty pages at the start of the queue
    uint32_t discardBatchSize = 50; //discard this many messages in a batch
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n",
               __func__, Q);

    if (Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE)
    {
        //We need to do a much more complex not discarding
        //current retained msgs
        iemq_reclaimSpaceRemoteServer( pThreadData, Q);
        goto mod_exit;
    }

    //Work out what the target number of bufferedmsgs is
    iepiPolicyInfo_t *pPolicyInfo = Q->Common.PolicyInfo;
    uint64_t targetBufferedMessages = 0;

    assert(pPolicyInfo->maxMessageBytes == 0);

    // Allow at least 1 message to survive if there is a non-zero limit set.
    if (pPolicyInfo->maxMessageCount > 0)
    {
        targetBufferedMessages = (uint64_t)(1 + (pPolicyInfo->maxMessageCount * IEQ_RECLAIMSPACE_MSGS_SURVIVE_FRACTION));
    }

    iemqCursor_t lastKnownGetCursor = { .c = {0, NULL} };
    iemqQNode_t *nextToScanNode = NULL;

    uint64_t counter = 0;

    uint64_t maxToRemove = 0;

    while (    (targetBufferedMessages != 0)
            && (Q->bufferedMsgs > targetBufferedMessages)
            && !scannedAllMessages
            && ((maxToRemove == 0)||(removedMsgs < maxToRemove)))
    {
        iemqQNode_t *discardNodes[discardBatchSize];
        uint32_t currentBatchSize = 0;
        uint64_t currentBatchBytes = 0;

        iemqQNode_t *scanNode = NULL;

        //Grab the getlock, remove 50 messages, release getlock repeat
        if (iemq_lockMutexWithWaiterLimit( &(Q->getlock)
                                         , &(Q->discardingWaitingThdCount)
                                         , 1))
        {
            //No scan should be in progress so scan orderId should not point to an orderId for a valid node
            assert(Q->scanOrderId == IEMQ_ORDERID_PAST_TAIL);

            //Now we have the getlock set a limit to the number that this thread will remove based on how
            //overdepth this queue is so this thread does not get trapped discarding messages forever
            uint64_t curdepth = Q->bufferedMsgs;
            if (maxToRemove == 0 && (curdepth > targetBufferedMessages))
            {
                maxToRemove = 3 * (curdepth - targetBufferedMessages);
            }

            lastKnownGetCursor = Q->getCursor;
            nextToScanNode = lastKnownGetCursor.c.pNode;

            bool getCursorSetToSearching = false;

            while((currentBatchSize < discardBatchSize) &&
                  ((targetBufferedMessages != 0) && ((Q->bufferedMsgs-currentBatchSize) > targetBufferedMessages))  &&
                   !scannedAllMessages)

            {
                // 'node' is the first node where we are going to look for messages
                scanNode = nextToScanNode;

                // A little trace to track our progress (every 512 nodes)
                if (((counter++) & 0x1ff) == 0)
                {
                    ieutTRACEL(pThreadData, scanNode->orderId, ENGINE_HIFREQ_FNC_TRACE,
                               "RECLAIM SCAN: Q=%u, qCursor counter=%lu oId=%lu node=%p\n",
                               Q->qId, counter, scanNode->orderId,
                               scanNode);
                }


                rc = iemq_markMessageIfGettable(pThreadData, Q, scanNode, &nextToScanNode);

                if (rc == OK)
                {
                    assert(nextToScanNode != NULL);
                    //We've locked a node
                    //Update get cursor to node after one we grabbed, if something became available before the
                    //one we grabbed
                    assert(scanNode != NULL);
                    if (iemq_updateGetCursor(pThreadData, Q, scanNode->orderId, nextToScanNode))
                    {
                        // We succesfully updated the get cursor, which means we can
                        // return the node to the caller.
                        getCursorSetToSearching = false;
                        lastKnownGetCursor.c.pNode = nextToScanNode;
                        lastKnownGetCursor.c.orderId = nextToScanNode->orderId;

                        discardNodes[currentBatchSize] = scanNode;
                        scanNode->msgState = ieqMESSAGE_STATE_DISCARDING;
                        currentBatchSize++;
                        currentBatchBytes += IEMQ_MESSAGE_BYTES(scanNode->msg);
                        ieutTRACE_HISTORYBUF(pThreadData, scanNode->orderId);
                    }
                    else
                    {
                        //Some evil thread has put a message before the one we were
                        //going to get and the get cursor has been rewound.
                        //It conceivably could have been put in the same transaction as
                        //the node we found so we need to let this one go but
                        //we can loop again.
                        scanNode->msgState = ismMESSAGE_STATE_AVAILABLE;
                        ieutTRACE_HISTORYBUF(pThreadData, scanNode->orderId);

                        lastKnownGetCursor = Q->getCursor;
                        nextToScanNode = lastKnownGetCursor.c.pNode;
                        getCursorSetToSearching = false;
                    }
                }
                else if (rc == ISMRC_NoMsgAvail)
                {
                    if (nextToScanNode != NULL)
                    {
                        if (!getCursorSetToSearching)
                        {
                            //Hmm... can't get that one but there's a subsequent node (so we're
                            //not at the tail) we'll 'clear' the getcursor and scan forward looking
                            //for a message. Note that we must (re)scan the node we've just scanned
                            //after clearing the get cursor in case it has suddenly come available

                            //Note the page we're scanning so it can't get freed out from under us
                            //(The setting of this value is protected by the getLock)
                            Q->scanOrderId = scanNode->orderId;

                            bool successfulCAS = __sync_bool_compare_and_swap(
                                    &(Q->getCursor.whole), lastKnownGetCursor.whole,
                                    IEMQ_GETCURSOR_SEARCHING.whole);

                            if (successfulCAS)
                            {
                                getCursorSetToSearching = true;
                                nextToScanNode = scanNode; //rescan as per above comment
                            }
                            else
                            {
                                //Someone rewound the get cursor...reread get cursor and loop around and
                                //try again
                                lastKnownGetCursor  =  Q->getCursor;
                                nextToScanNode = lastKnownGetCursor.c.pNode;
                                Q->scanOrderId = IEMQ_ORDERID_PAST_TAIL;
                            }
                        }
                        else
                        {
                            //We're going to scan the next node...
                            Q->scanOrderId = nextToScanNode->orderId;
                        }
                    }
                    else
                    {
                        //No later nodes... scan completed
                        scannedAllMessages = true;
                    }
                }
                else
                {
                    ieutTRACE_FFDC(ieutPROBE_004, true, "Unexpected error locking node for discard", rc,
                                   "Internal Name", Q->InternalName, sizeof(Q->InternalName),
                                   "Queue", Q, sizeof(iemqQueue_t), NULL);
                }
            }

            if (getCursorSetToSearching)
            {
                //We failed to find a node...try and record how far we scanned in the
                //get cursor, we put the cursor at the node we failed to scan
                (void) iemq_updateGetCursor(pThreadData, Q, scanNode->orderId,
                                                         scanNode);

                //If the update failed, someone has put an earlier message on the
                //queue...
            }

            int os_rc = pthread_mutex_unlock(&(Q->getlock));

            if (UNLIKELY(os_rc != 0))
            {
                ieutTRACE_FFDC(ieutPROBE_003, true, "Releasing getlock failed.", rc,
                               "Internal Name", Q->InternalName, sizeof(Q->InternalName),
                               "Queue", Q, sizeof(iemqQueue_t), NULL);
            }

            //Now remove all the messages that we just marked as delivered for discard. These messages
            //may get "undiscarded" if we were to restart at this point and then be redelivered after messages
            //put later but to change that would be difficult - e.g. hold the getlock until  the async
            //commit to prevent newer messages being delivered
            if (currentBatchSize > 0)
            {
                ieutTRACE_HISTORYBUF(pThreadData, currentBatchSize);

                iemq_destroyMessageBatch( pThreadData
                                        , Q
                                        , currentBatchSize
                                        , discardNodes
                                        , true
                                        , &doPageCleanup );

                removedMsgs += currentBatchSize;
            }

        }
        else
        {
            //Someone else is now reclaiming space, we're done!
            break;
        }
    }
    //printf("Done discard. Removed %lu scannedAll %s maxToRemove %lu target %lu current %lu\n",
    //          removedMsgs, (scannedAllMessages ? "true":"false"), maxToRemove, targetBufferedMessages, Q->bufferedMsgs);

    if (removedMsgs > 0)
    {
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_DISCARDEDMSGS, (int64_t)removedMsgs);
        __sync_fetch_and_add(&(Q->discardedMsgs), removedMsgs);
    }

    if (doPageCleanup)
    {
        iemq_cleanupHeadPages(pThreadData, Q);
    }

mod_exit:
    ieutTRACEL(pThreadData, removedMsgs, ENGINE_FNC_TRACE, FUNCTION_EXIT "removed = %lu\n",
               __func__, removedMsgs);
    return;
}


static inline void iemq_startReleaseDeliveryId( ieutThreadData_t *pThreadData
                                              , iecsMessageDeliveryInfoHandle_t hMsgDelInfo
                                              , iemqQueue_t *Q
                                              , iemqQNode_t *pnode
                                              , uint32_t *pStoreOpCount)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pnode->deliveryId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_IDENT "Releasing deliveryid %u\n", __func__,
               pnode->deliveryId);

    if (pnode->inStore)
    {
        // Delete any MDR for this message - won't commit
        if (pnode->hasMDR)
        {
            //We don't allow unstoreMDR to do the commit - we *must* call complete after c
            rc = iecs_startUnstoreMessageDeliveryReference(pThreadData, pnode->msg,
                    hMsgDelInfo, pnode->deliveryId, pStoreOpCount);

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
                                        , "iecs_unstoreMessageDeliveryReference failed.", rc
                                  , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                                  , "Queue", Q, sizeof(iemqQueue_t)
                                  , "Delivery Id", &pnode->deliveryId, sizeof(pnode->deliveryId)
                                  , "Order Id", &pnode->orderId, sizeof(pnode->orderId)
                                  , "pNode", pnode, sizeof(iemqQNode_t)
                                  , NULL);

                }
            }
        }
    }
}

static inline void iemq_finishReleaseDeliveryId( ieutThreadData_t *pThreadData
                                               , iecsMessageDeliveryInfoHandle_t hMsgDelInfo
                                               , iemqQueue_t *Q
                                               , iemqQNode_t *pnode
                                               , bool *pTriggerRedelivery)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pnode->deliveryId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_IDENT "Releasing deliveryid %u\n", __func__,
               pnode->deliveryId);

    if (pnode->inStore)
    {
        // Delete any MDR for this message - won't commit
        if (pnode->hasMDR)
        {
            pnode->hasMDR = false;

            //We didn't allow unstoreMDR to do the commit - this is us finishing after commit
            rc = iecs_completeUnstoreMessageDeliveryReference(pThreadData, pnode->msg,
                    hMsgDelInfo, pnode->deliveryId);

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
                    *pTriggerRedelivery = true;
                    rc = OK;
                }
                else
                {
                    ieutTRACE_FFDC( ieutPROBE_003, true
                                        , "iecs_unstoreMessageDeliveryReference failed.", rc
                                  , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                                  , "Queue", Q, sizeof(iemqQueue_t)
                                  , "Delivery Id", &pnode->deliveryId, sizeof(pnode->deliveryId)
                                  , "Order Id", &pnode->orderId, sizeof(pnode->orderId)
                                  , "pNode", pnode, sizeof(iemqQNode_t)
                                  , NULL);

                }
            }
        }
    }
    else
    {
        if (pnode->deliveryId > 0)
        {
            // Need to free up the deliveryId
            rc = iecs_releaseDeliveryId(pThreadData, hMsgDelInfo,
                                        pnode->deliveryId);

            if (rc != OK)
            {
                if (rc == ISMRC_DeliveryIdAvailable)
                {
                    *pTriggerRedelivery = true;
                    rc = OK;
                }
                else if (rc == ISMRC_NotFound)
                {
                    // We were trying to remove something that's not there. Don't panic. (release will have written an FFDC and continued)
                    rc = OK;
                }
                else
                {
                    ieutTRACE_FFDC(ieutPROBE_004, true,
                                        "iecs_releaseDeliveryId failed for free deliveryId case.", rc
                                   , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                                   , "Queue", Q, sizeof(iemqQueue_t)
                                   , "Delivery Id", &pnode->deliveryId, sizeof(pnode->deliveryId)
                                   , "Order Id", &pnode->orderId, sizeof(pnode->orderId)
                                   , "pNode", pnode, sizeof(iemqQNode_t)
                                   , NULL);
                }
            }
        }
    }
    pnode->deliveryId = 0;
}


//Caller must call iemq_finishReleaseDeliveryIdIfNecessary (after any commit)
static inline void iemq_startReleaseDeliveryIdIfNecessary(
                             ieutThreadData_t *pThreadData
                           , ismEngine_Session_t *pSession
                           , uint32_t options
                           , iemqQueue_t *Q
                           , iemqQNode_t *pnode
                           , uint32_t *pStoreOpCount)
{
    //If a deliveryId has been assigned to this node and this ack finishes delivery or makes
    //it never have happened, then release the deliveryid (and if the delivery never happened
    //we need to remove it from the node so it can be assigned to other clients)
    if (pnode->deliveryId != 0)
    {
        iecsMessageDeliveryInfoHandle_t hMsgDelInfo =
            pSession->pClient->hMsgDeliveryInfo;

        assert(hMsgDelInfo != NULL);

        if (   (options == ismENGINE_CONFIRM_OPTION_CONSUMED)
            || (options == ismENGINE_CONFIRM_OPTION_NOT_DELIVERED))
        {
            iemq_startReleaseDeliveryId(pThreadData, hMsgDelInfo, Q, pnode,
                                        pStoreOpCount);
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Adjust expiry counter as a message expired
///
///  @param[in] Qhdl               - Queue
///  @return                       - void
///////////////////////////////////////////////////////////////////////////////
void iemq_messageExpired( ieutThreadData_t *pThreadData
                        , ismQHandle_t Qhdl)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    __sync_fetch_and_add(&(Q->expiredMsgs), 1);
    pThreadData->stats.expiredMsgCount++;
}

void  iemq_prepareConsumeAck( ieutThreadData_t *pThreadData
                            , iemqQueue_t *Q
                            , ismEngine_Session_t *pSession
                            , ismEngine_Transaction_t *pTran
                            , iemqQNode_t  *pnode
                            , uint32_t *pStoreOps)
{
    ieutTRACEL(pThreadData, pnode->orderId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_IDENT "Q %u Node Oid %lu msg %p, state: %u\n",
               __func__, Q->qId, pnode->orderId, pnode->msg, pnode->msgState);

    if (pTran != NULL)
    {
        if (pnode->inStore)
        {
            assert(pnode->hMsgRef != 0);
            iemq_checkCachedMemoryExists(Q, pnode);
            iemqSLEConsume_t *consumeData = iemq_getCachedSLEConsumeMem(
                                                pnode->iemqCachedSLEHdr);

            int32_t rc = ietr_createTranRef(pThreadData, pTran, pnode->hMsgRef,
                                      iestTOR_VALUE_CONSUME_MSG, 0, &(consumeData->TranRef));

            if (UNLIKELY(rc != OK))
            {
                //No recoverable errors...
                ieutTRACE_FFDC(ieutPROBE_001, true,
                                   "Couldn't create tran ref", rc,
                                   NULL);
            }
#if 0
            iemq_updateMsgRefInStore(pThreadData, Q, pnode,
                    ismMESSAGE_STATE_CONSUMED,
                    pnode->deliveryCount,
                    false);

            (*pStoreOps) += 2;
#else
            //We don't need to update the message ref in the store... we can
            //infer that it was transactionally acked from the tran ref.
            //Although the update we are skipping won't affect number of store commits
            //(disk forces), updating message refs on deep queues in the store is slow
            //(find the reference to update).
            (*pStoreOps) += 1;
#endif
        }
    }
    else
    {
        if (pnode->inStore)
        {
            //We can't use iemq_consumeMessageInStore() as we need the deleteReference to
            //be in same transaction as the deletion of the MDR and schedule the removal
            //for after the commit
            int32_t rc = ism_store_deleteReference(pThreadData->hStream,
                                           Q->QueueRefContext,
                                           pnode->hMsgRef,
                                           pnode->orderId,
                                           0);
            if (UNLIKELY(rc != OK))
            {
                // The failure to delete a store reference means that the
                // queue is inconsistent.
                ieutTRACE_FFDC( ieutPROBE_002, true,
                                   "ism_store_deleteReference (multiConsumer) failed.", rc
                              , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                              , "Queue", Q, sizeof(iemqQueue_t)
                              , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                              , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                              , "pNode", pnode, sizeof(iemqQNode_t)
                              , NULL);
            }
            (*pStoreOps)++;
        }

        //Need to release any delivery id used by this pnode...
        iemq_startReleaseDeliveryId(pThreadData, pSession->pClient->hMsgDeliveryInfo,
                                    Q, pnode, pStoreOps);
    }
}

/// If phMsgToUnstore == NULL on input then this function uses lazy removal
/// (So the caller must either supply phMsgToUnstore and later call iest_finishUnstoreMessages()
///  OR must call iest_checkLazyMessages() / store_commit before next potential use of lazy removal
///
/// ppJobThread - if we need to run checkwaiters, it is run on this thread (and this is then NULLd
///               so caller doesn't have to free.
static int32_t iemq_completeConsumeAck( ieutThreadData_t *pThreadData
                                      , iemqQueue_t *Q
                                      , ismEngine_Session_t *pSession
                                      , ismEngine_Transaction_t *pTran
                                      , iemqQNode_t  *pnode
                                      , ismStore_Handle_t *phMsgToUnstore
                                      , bool *pTriggerSessionRedelivery
                                      , ismEngine_BatchAckState_t *pAckState
                                      , ieutThreadData_t **ppJobThread)
{
    ismEngine_Consumer_t *pConsumerToDeack = NULL;
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, pnode->orderId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_IDENT "Q %u Node Oid %lu  msg %p, state: %u\n",
               __func__, Q->qId, pnode->orderId, pnode->msg, pnode->msgState);

    //If this is transactional, lock the message and log what we're doing in the transaction
    if (pTran != NULL)
    {
        assert(pnode->iemqCachedSLEHdr != NULL);
        iemqSLEConsume_t *consumeData = iemq_getCachedSLEConsumeMem(
                                            pnode->iemqCachedSLEHdr);

        if (pnode->inStore)
        {
            assert(consumeData->TranRef.hTranRef != ismSTORE_NULL_HANDLE); //Filled in during prepare
        }

        ielmLockRequestHandle_t hCachedLockRequest =
            consumeData->hCachedLockRequest;
        consumeData->hCachedLockRequest = NULL; //We are going to give this memory to the lockManager...

        ielmLockName_t LockName = { .Msg = { ielmLOCK_TYPE_MESSAGE, Q->qId,
                                             pnode->orderId
                                           }
                                  };

        ietr_recordLockScopeUsed(pTran); //Ensure the locks are cleaned up on transaction end
        rc = ielm_takeLock(pThreadData,
                           pTran->hLockScope, hCachedLockRequest, // Lock request memory is already allocated
                           &LockName,
                           ielmLOCK_MODE_X,
                           ielmLOCK_DURATION_COMMIT,
                           NULL);

        if (rc != OK)
        {
            //We should be able to lock this message... we've already given it to the client
            //so no-one else should be fiddling with it!
            ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                       "%s: ielm_takeLock failed rc=%d\n", __func__, rc);
            goto mod_exit;
        }

        if (Q->ackListsUpdating)
        {
            ieal_removeUnackedMessage(pThreadData, pSession, pnode,
                                      &pConsumerToDeack);
        }

        iemq_addConsumeMsgSLE(pThreadData, Q, pnode,
                                pSession, pTran, pConsumerToDeack);

        //Adding a get to a transaction increases the predelete count by one... so the queue can't be deleted
        //until the transaction is committed or rolled back
        if (pAckState == NULL)
        {
            __sync_add_and_fetch(&(Q->preDeleteCount), 1);
        }
        else
        {
            assert(pAckState->pConsumer == pConsumerToDeack);
            pAckState->preDeleteCountIncrement++;
        }

        pnode->msgState = ismMESSAGE_STATE_CONSUMED;
    }
    else
    {
        //Now we have committed the deletion of the reference, we can schedule the deletion of the message
        if (pnode->inStore)
        {
            uint32_t storeOpCount = 0;
            rc = iest_unstoreMessage(pThreadData, pnode->msg, false,
                    ((phMsgToUnstore == NULL ) ? true : false), phMsgToUnstore,
                                           &storeOpCount);

            if (UNLIKELY(rc != OK))
            {
                // If we fail to unstore a message there is not much we
                // can actually do. So continue onwards and try and make
                // our state consistent.
                ieutTRACE_FFDC(ieutPROBE_008, false,
                        "iest_unstoreMessage (multiConsumer) failed.", rc
                        , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                        , "Queue", Q, sizeof(iemqQueue_t)
                        , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                        , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                        , "pNode", pnode, sizeof(iemqQNode_t)
                        , NULL);
            }

            assert(storeOpCount == 0); //any unstore of the message should have been lazy scheduled
        }


        //Need to finish the release of any delivery id used by this pnode...
        iemq_finishReleaseDeliveryId(pThreadData, pSession->pClient->hMsgDeliveryInfo,
                                     Q, pnode, pTriggerSessionRedelivery);

        __sync_fetch_and_sub(&(Q->inflightDeqs), 1);
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -1);
        pThreadData->stats.bufferedMsgCount--;
        DEBUG_ONLY int32_t oldDepth = __sync_fetch_and_sub(&(Q->bufferedMsgs), 1);
        assert(oldDepth > 0);
        if ((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0)
        {
            size_t messageBytes = IEMQ_MESSAGE_BYTES(pnode->msg);
            pThreadData->stats.remoteServerBufferedMsgBytes -= messageBytes;
            (void)__sync_fetch_and_sub((&Q->bufferedMsgBytes), messageBytes);
        }
        __sync_fetch_and_add(&(Q->dequeueCount), 1);

        //Didn't consume the node transactional... free the memory we reserved
        iemq_releaseReservedSLEMem(pThreadData, pnode);

        if (Q->ackListsUpdating && (pnode->ackData.pConsumer != NULL))
        {
            ieal_removeUnackedMessage(pThreadData, pSession, pnode,
                                      &pConsumerToDeack);
        }

        bool needCleanup = iemq_needCleanAfterConsume(pnode);

        ismEngine_Message_t *msgToFree = pnode->msg;
        pnode->msg = MESSAGE_STATUS_EMPTY;
        pnode->msgState = ismMESSAGE_STATE_CONSUMED;

        iem_releaseMessage(pThreadData, msgToFree);

        if (needCleanup)
        {
            iemq_cleanupHeadPages(pThreadData, Q);
        }

        if (iemq_checkFullDeliveryStartable(pThreadData, Q))
        {
            bool needCheckWaiters=true;

            if (ppJobThread != NULL && *ppJobThread != NULL)
            {
                if (iemq_scheduleOnJobThreadIfPoss(pThreadData, Q, *ppJobThread))
                {
                    needCheckWaiters = false;
                }
                //iemq_scheduleOnJobThreadIfPoss will have used our thread ref, don't let anyone else release it
                *ppJobThread = NULL;
            }
            if (needCheckWaiters)
            {
                (void)iemq_checkWaiters( pThreadData
                                       , (ismQHandle_t) Q
                                       , NULL
                                       , NULL);
            }
        }
    }

    if (pConsumerToDeack != NULL)
    {
        bool triggerCheckWaiters = false;

        if(pConsumerToDeack->fFlowControl)
        {
            triggerCheckWaiters = true;
            pConsumerToDeack->fFlowControl = 0;
        }

        if (pAckState == NULL)
        {
            //If there was an ackState, we'd call checkWaiters at the end of a batch as there isn't do it now..
            if (triggerCheckWaiters)
            {
                bool needCheckWaiters=true;

                if (ppJobThread != NULL && *ppJobThread != NULL)
                {
                    if (iemq_scheduleOnJobThreadIfPoss(pThreadData, Q, *ppJobThread))
                    {
                        needCheckWaiters = false;
                    }
                    //iemq_scheduleOnJobThreadIfPoss will have used our thread ref, don't let anyone else release it
                    *ppJobThread = NULL;
                }
                if (needCheckWaiters)
                {
                    (void)iemq_checkWaiters( pThreadData
                                           , (ismQHandle_t) Q
                                           , NULL
                                           , NULL);
                }

            }

            //Can cause consumer and hence queue to be deleted:
            decreaseConsumerAckCount(pThreadData, pConsumerToDeack, 1);
        }
        else
        {
            assert(pAckState->pConsumer == pConsumerToDeack);
            pAckState->ackCount++;
            if (triggerCheckWaiters) pAckState->deliverOnCompletion = true;
        }
    }

mod_exit:
    return rc;
}

void  iemq_prepareReceiveAck( ieutThreadData_t *pThreadData
                            , ismEngine_Session_t *pSession
                            , iemqQueue_t *Q
                            , iemqQNode_t  *pnode
                            , uint32_t *pStoreOps)
{
    ieutTRACEL(pThreadData, pnode->orderId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_IDENT "Q %u Node Oid %lu msg %p, state: %u\n",
               __func__, Q->qId, pnode->orderId, pnode->msg, pnode->msgState);

    if (pnode->msgState != ismMESSAGE_STATE_DELIVERED)
    {
        // In certain circumstances, the protocol layer will allow a second
        // receive ack through to the engine.  (PMR 90255,000,858).  In these
        // cases, we will just swallow the ack.
        if (pnode->msgState == ismMESSAGE_STATE_RECEIVED) {
            ieutTRACEL(pThreadData, pnode, ENGINE_INTERESTING_TRACE, "Message in RECEIVED state was acknowledged again.\n");
            goto mod_exit;
        }
        // If we are acknowledging the receipt of a message which
        // is not in DELIVERED state then we have a breakdown in the
        // engine protocol. Bring the server down with as much
        // diagnostics as possible.
        ieutTRACE_FFDC(ieutPROBE_005, true,
                       "Invalid msgState when ack-received message", ISMRC_Error,
                       "msgState", &pnode->msgState, sizeof(ismMessageState_t),
                       "OrderId", &pnode->orderId, sizeof(uint64_t),
                       "Node", pnode, sizeof(iemqQNode_t),
                       "Queue", Q, sizeof(iemqQueue_t),
                       NULL);
    }

    if (pnode->inStore)
    {
        // If the message is in the store, we need to update the store too
        // If the client is not durable but this sub is then on restart this node should be discarded...
        bool consumeOnRestart = (pSession->pClient->Durability == iecsNonDurable);

        if (consumeOnRestart)
        {
            //The node will have been marked consumed during delivery...
            //....we do nothing
        }
        else
        {
            iemq_updateMsgRefInStore(pThreadData, Q, pnode,
                                     ismMESSAGE_STATE_RECEIVED,
                                     consumeOnRestart,
                                     pnode->deliveryCount,
                                     false);

            (*pStoreOps)++;
        }
    }
mod_exit:
   return;
}

static void iemq_completeReceiveAck( ieutThreadData_t *pThreadData
                                   , iemqQueue_t *Q
                                   , iemqQNode_t  *pnode)
{
    ieutTRACEL(pThreadData, pnode->orderId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_IDENT "Q %u Node Oid %lu  msg %p, state: %u\n",
               __func__, Q->qId, pnode->orderId, pnode->msg, pnode->msgState);

    if (pnode->msgState == ismMESSAGE_STATE_DELIVERED)
    {
        pnode->msgState = ismMESSAGE_STATE_RECEIVED;
    }
}


void iemq_prepareDeletedAck( ieutThreadData_t *pThreadData
                           , iemqQueue_t *Q
                           , ismEngine_Session_t *pSession
                           , ismEngine_Transaction_t *pTran
                           , iemqQNode_t *pnode
                           , uint32_t options
                           , uint32_t *pStoreOps)
{
    ieutTRACEL(pThreadData, pnode->orderId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_IDENT "Q %u Node Oid %lu pTran %p, options %u, msg %p, state: %u\n",
               __func__, Q->qId, pnode->orderId, pTran, options, pnode->msg, pnode->msgState);

    assert(Q->isDeleted);

    //
    //Q is deleted don't need to rewind get cursor, mark message etc..
    //
    pnode->deleteAckInFlight = true;

    //Need to release any delivery id used by this pnode...
    if (pnode->deliveryId > 0)
    {
        iemq_startReleaseDeliveryId(pThreadData, pSession->pClient->hMsgDeliveryInfo,
                                         Q, pnode, pStoreOps);
    }
}

void iemq_completeDeletedAck( ieutThreadData_t *pThreadData
                          , iemqQueue_t *Q
                          , ismEngine_Session_t *pSession
                          , ismEngine_Transaction_t *pTran
                          , iemqQNode_t *pnode
                          , uint32_t options
                          , bool *pTriggerSessionRedelivery
                          , ismEngine_BatchAckState_t *pAckState)
{
    ieutTRACEL(pThreadData, pnode->orderId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_IDENT "Q %u Node Oid %lu pTran %p, options %u, msg %p, state: %u\n",
               __func__, Q->qId, pnode->orderId, pTran, options, pnode->msg, pnode->msgState);

    assert(Q->isDeleted);
    assert(pnode->deleteAckInFlight);
    //
    //Q is deleted don't need to rewind get cursor, mark message etc..
    //
    iemq_releaseReservedSLEMem(pThreadData, pnode);
    assert(pnode->ackData.pConsumer != NULL);

    //Need to release any delivery id used by this pnode...
    iemq_finishReleaseDeliveryId(pThreadData, pSession->pClient->hMsgDeliveryInfo, Q,
                                      pnode, pTriggerSessionRedelivery);

    if (options == ismENGINE_CONFIRM_OPTION_SESSION_CLEANUP)
    {
        ismEngine_Consumer_t *pConsumerToDeack = pnode->ackData.pConsumer;

        pnode->ackData.pConsumer = NULL;
        pnode->ackData.pPrev = NULL;
        pnode->ackData.pNext = NULL;

        if (Q->ackListsUpdating)
        {
            if (pAckState == NULL)
            {
                //Can cause consumer and hence queue to be deleted:
                decreaseConsumerAckCount(pThreadData, pConsumerToDeack, 1);
            }
            else
            {
                assert(pAckState->pConsumer == pConsumerToDeack);
                pAckState->ackCount++;
            }
        }
    }
    else if (options != ismENGINE_CONFIRM_OPTION_RECEIVED)
    {
        assert(pnode->ackData.pConsumer != NULL);
        ismEngine_Consumer_t *pConsumerToDeack = NULL;
        if (Q->ackListsUpdating)
        {
            ieal_removeUnackedMessage(pThreadData, pSession, pnode,
                                      &pConsumerToDeack);

            if (pAckState == NULL)
            {
                //Can cause consumer and hence queue to be deleted:
                decreaseConsumerAckCount(pThreadData, pConsumerToDeack, 1);
            }
            else
            {
                assert(pAckState->pConsumer == pConsumerToDeack);
                pAckState->ackCount++;
            }
        }
        else
        {
            pnode->ackData.pConsumer = NULL;
        }
    }
}

static int32_t iemq_ackCommitted(ieutThreadData_t           *pThreadData,
                                 int32_t                              rc,
                                 ismEngine_AsyncData_t        *asyncInfo,
                                 ismEngine_AsyncDataEntry_t     *context)
{
    assert(context->Type == iemqQueueAcknowledge);
    assert(rc == OK);

    iemqAcknowledgeAsyncData_t *pAckData = (iemqAcknowledgeAsyncData_t *)(context->Data);

    ieutTRACEL(pThreadData, pAckData->pnode->orderId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_IDENT "Q %u Node Oid %lu pTran %p, options %u, msg %p, state: %u\n",
               __func__, pAckData->Q->qId, pAckData->pnode->orderId, pAckData->pTran, pAckData->ackOptions,
               pAckData->pnode->msg, pAckData->pnode->msgState);


    ismEngine_CheckStructId( pAckData->StructId
                           , IEMQ_ACKNOWLEDGE_ASYNCDATA_STRUCID
                           , ieutPROBE_001);

    iead_popAsyncCallback( asyncInfo, context->DataLen);

    if (rc == OK)
    {
        bool triggerSessionRedelivery = false;

        rc = iemq_processAck( pThreadData
                            , (ismQHandle_t)(pAckData->Q)
                            , pAckData->pSession
                            , pAckData->pTran
                            , pAckData->pnode
                            , pAckData->ackOptions
                            , NULL
                            , &triggerSessionRedelivery
                            , NULL
                            , &(pAckData->pJobThread));

        if (pAckData->pTran != NULL)
        {
            //This ack should no longer block commit/rollback
            ietr_decreasePreResolveCount(pThreadData, pAckData->pTran, true);
        }

        if (pAckData->pJobThread != NULL)
        {
            ieut_releaseThreadDataReference(pAckData->pJobThread);
            pAckData->pJobThread = NULL;
        }

        if (triggerSessionRedelivery)
        {
            ismEngine_internal_RestartSession(pThreadData,  pAckData->pSession, false);
        }

        if (rc == OK && pAckData->ackOptions == ismENGINE_CONFIRM_OPTION_CONSUMED)
        {
            rc = iest_checkLazyMessages( pThreadData, asyncInfo);
        }
    }

    return rc;
}


void  iemq_prepareAck( ieutThreadData_t *pThreadData
                     , ismQHandle_t Qhdl
                     , ismEngine_Session_t *pSession
                     , ismEngine_Transaction_t *pTran
                     , void *pDelivery
                     , uint32_t options
                     , uint32_t *pStoreOps)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    iemqQNode_t *pnode = (iemqQNode_t *) pDelivery;

    if (Q->isDeleted && options != ismENGINE_CONFIRM_OPTION_RECEIVED)
    {
        iemq_prepareDeletedAck( pThreadData
                              , Q
                              , pSession
                              , pTran
                              , pnode
                              , options
                              , pStoreOps);
    }
    else
    {
        if (options == ismENGINE_CONFIRM_OPTION_CONSUMED)
        {
            iemq_prepareConsumeAck( pThreadData
                                  , Q
                                  , pSession
                                  , pTran
                                  , pnode
                                  , pStoreOps);
        }
        else if (options == ismENGINE_CONFIRM_OPTION_RECEIVED)
        {
            assert(pTran == NULL); //We don't support transactional 2-phase ack at the mo.

            iemq_prepareReceiveAck( pThreadData
                                  , pSession
                                  , Q
                                  , pnode
                                  , pStoreOps);
        }
        else
        {
            assert(  (options == ismENGINE_CONFIRM_OPTION_NOT_DELIVERED)
                   ||(options == ismENGINE_CONFIRM_OPTION_NOT_RECEIVED)
                   ||(options == ismENGINE_CONFIRM_OPTION_SESSION_CLEANUP));

            //These we don't do async-ly yet, so everything is done in process
        }
    }
}


/// When allowAlterAckListMDR is false, we don't remove node from acklist/deliveryidlists
///                            (Used during destroy of client/client interest in sub where mdrs are handled separately)
void iemq_processNack( ieutThreadData_t *pThreadData
                      , iemqQueue_t *Q
                      , ismEngine_Session_t *pSession
                      , ismEngine_Transaction_t *pTran
                      , iemqQNode_t *pnode
                      , uint32_t options
                      , bool allowAlterAckListMDR
                      , bool *pTriggerSessionRedelivery
                      , ismEngine_BatchAckState_t *pAckState)
{
    ieutTRACEL(pThreadData, pnode->orderId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_IDENT "Q %u Node Oid %lu pTran %p, options %u, msg %p, state: %u\n",
               __func__, Q->qId, pnode->orderId, pTran, options, pnode->msg, pnode->msgState);

    assert(  (options == ismENGINE_CONFIRM_OPTION_NOT_DELIVERED)
           ||(options == ismENGINE_CONFIRM_OPTION_NOT_RECEIVED)
           ||(options == ismENGINE_CONFIRM_OPTION_SESSION_CLEANUP));

    //We don't support transactions for these types of acks, they can't be rolled back etc...
    //If someone asks us to do one of them transactionally... FDC then bravely carry on untransactionally

    if (pTran != NULL)
    {
        ieutTRACE_FFDC(ieutPROBE_002, false
                      , "Attempted to do a confirmMessageDelivery transactionally with an invalid option"
                      , ISMRC_Error
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , "Option", &options, sizeof(options)
                      , NULL);
    }

    bool makeMessageAvailable = true;
    bool updateStoreMessage   = false; //In most cases we don't need to update the store, on restart we'll
                                       //act like we crashed whilst the message was in flight and the message
                                       //will be put back on the queue as available

    //Didn't consume the node transactional... free the memory we reserved
    iemq_releaseReservedSLEMem(pThreadData, pnode);

    ismEngine_Consumer_t *pConsumerToDeack = NULL;

    //If the consumer is keeps deliverys ids associated, the only time we should reset the delivery id is if we never sent it
    //But the only time an MQTT consumer nacks messages using ismENGINE_CONFIRM_OPTION_NOT_RECEIVED is unit tests and so putting
    //the message back to available better mimics intermediateQ. If we were to need ismENGINE_CONFIRM_OPTION_NOT_RECEIVED for
    //MQTT on multiconsumer we would probably need code that kept the delivery id and only redelivered the message to the affected
    //consumer.

    if (options == ismENGINE_CONFIRM_OPTION_SESSION_CLEANUP)
    {

        if (pnode->ackData.pConsumer->fShortDeliveryIds)
        {
            makeMessageAvailable = false;
        }
        //Modify the ackData directly rather than calling ieal_RemoveUnackedMessage as when called with
        //this option we have the session acklist locked and that would prevent ieal_RemoveUnackedMessage altering it...
        pConsumerToDeack = pnode->ackData.pConsumer;
        pnode->ackData.pConsumer = NULL;
        pnode->ackData.pPrev = NULL;
        pnode->ackData.pNext = NULL;
    }

    else if (     Q->ackListsUpdating
              && (pnode->ackData.pConsumer != NULL)
              && allowAlterAckListMDR)
    {
        ieal_removeUnackedMessage(pThreadData, pSession, pnode,
                                  &pConsumerToDeack);
    }

    if (options == ismENGINE_CONFIRM_OPTION_NOT_DELIVERED)
    {
        //If we want to act like the message never attempted delivered then
        //reduce the delivery count
        assert(pnode->deliveryCount > 0);
        pnode->deliveryCount--;

        //We also want to update the store count so after restart it doesn't look like it was delivered
        updateStoreMessage = true;
    }

    if (makeMessageAvailable)
    {
        if (updateStoreMessage && pnode->inStore)
        {
            iemq_updateMsgRefInStore(pThreadData, Q, pnode,
                                     ismMESSAGE_STATE_AVAILABLE, false, pnode->deliveryCount,
                                     false);

            uint32_t storeOpCount = 1; //The update above will have left an uncommitted operation...

            if (allowAlterAckListMDR)
            {
                iemq_startReleaseDeliveryIdIfNecessary(pThreadData, pSession,
                                                       options, Q, pnode, &storeOpCount);
            }

            if (storeOpCount > 0)
            {
                iest_store_commit(pThreadData, false);
                storeOpCount = 0;

                if (allowAlterAckListMDR)
                {
                    iemq_finishReleaseDeliveryId(pThreadData, pSession->pClient->hMsgDeliveryInfo,
                                                 Q, pnode, pTriggerSessionRedelivery);
                }
            }

        }
        iemqCursor_t newMsg;

        newMsg.c.orderId = pnode->orderId;
        newMsg.c.pNode = pnode;

        if (pnode->msg->Header.Expiry != 0)
        {
            iemeBufferedMsgExpiryDetails_t msgExpiryData = { pnode->orderId , pnode, pnode->msg->Header.Expiry };
            ieme_addMessageExpiryData( pThreadData, (ismEngine_Queue_t *)Q,  &msgExpiryData);
        }

        //Marking the message available and then rewinding might (if the message gets consumed+removed
        //before the rewind) mean we rewind to a page that has been freed. If we do it the other way
        //around a getter might try to get it before it's available leaving it trapped on the queue, we
        //need to tie the two operations together
        //At least temporarily we use the head lock to prevent the message going away after it is available but
        //before the rewind
        //We could use the lockmanager to tie the two operations together with the rewind happening in an instantlock callback
        iemq_takeReadHeadLock(Q);

        pnode->msgState = ismMESSAGE_STATE_AVAILABLE;

        __sync_fetch_and_sub(&(Q->inflightDeqs), 1);

        //Now the message is available, cause the getCursor to be rewound if necessary
        iemq_rewindGetCursor(pThreadData, Q, newMsg);

        iemq_releaseHeadLock(Q);


        //If there were too many inflight messages we may need to restart delivery
        if (iemq_checkFullDeliveryStartable(pThreadData, Q))
        {
            (void)iemq_checkWaiters( pThreadData
                                   , (ismQHandle_t) Q
                                   , NULL
                                   , NULL);
        }
    }

    //We've nacked a message, turn on taps for this consumer if they were off...
    //We'll call checkwaiters in a sec as we do for a nacked message...
    if (pConsumerToDeack != NULL)
    {
        pConsumerToDeack->fFlowControl = 0;
    }

    // Because a message is now available to be sent we need to call
    // checkWaiters to attempt re-delivery, except when we are in
    // a batch as that will make it's own explicit call to
    // check-waiters in that case.
    if (pAckState == NULL)
    {
        (void) iemq_checkWaiters(pThreadData, (ismQHandle_t) Q, NULL, NULL);

        //Now deack the consumer which could cause Q to be deleted...
        if (pConsumerToDeack != NULL)
        {
            decreaseConsumerAckCount(pThreadData, pConsumerToDeack, 1);
        }
    }
    else if (pConsumerToDeack != NULL)
    {
        assert(pAckState->pConsumer == pConsumerToDeack);
        pAckState->ackCount++;
        pAckState->deliverOnCompletion = true;
    }
}

/// If phMsgToUnstore == NULL on input then this function uses lazy removal
/// (So the caller must either supply phMsgToUnstore and later call iest_finishUnstoreMessages()
///  OR must call iest_checkLazyMessages() / iest_store_(async)commit before next potential use
/// of lazy removal
///
/// ppJobThread - if we need to run checkwaiters, it is run on this thread (and this is then NULLd
///               so caller doesn't have to free.
int32_t iemq_processAck( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ismEngine_Session_t *pSession
                       , ismEngine_Transaction_t *pTran
                       , void *pDelivery
                       , uint32_t options
                       , ismStore_Handle_t *phMsgToUnstore
                       , bool *pTriggerSessionRedelivery
                       , ismEngine_BatchAckState_t *pAckState
                       , ieutThreadData_t **ppJobThread)
{
    int rc = OK;
    iemqQueue_t *Q     = (iemqQueue_t *) Qhdl;
    iemqQNode_t *pnode = (iemqQNode_t *) pDelivery;


    if (pAckState != NULL)
    {
        ismEngine_Consumer_t *pConsumer = pnode->ackData.pConsumer;

        //For MQTT consumer we can end a session before the messages have been deliver but these messages still show up in the
        //message delivery info list we are using, if the node does not have a consumer, skip the ack
        if (options == ismENGINE_CONFIRM_OPTION_SESSION_CLEANUP && pConsumer == NULL)
        {
            //A messages from an MDR list that has not yet been redelivered, ignore it
            goto mod_exit;
        }

        //We're processing this ack as part of a batch
        if (pAckState->pConsumer != pConsumer || pAckState->Qhdl != Qhdl)
        {
            if (pAckState->ackCount != 0)
            {
                iemq_completeAckBatch(pThreadData, pAckState->Qhdl, pSession,
                                      pAckState);
            }
            pAckState->pConsumer = pConsumer;
            pAckState->Qhdl = Qhdl;
        }
    }

    if (pnode->deleteAckInFlight)
    {
        iemq_completeDeletedAck( pThreadData
                               , Q
                               , pSession
                               , pTran
                               , pnode
                               , options
                               , pTriggerSessionRedelivery
                               , pAckState);
    }
    else if (options == ismENGINE_CONFIRM_OPTION_CONSUMED)
    {
        rc = iemq_completeConsumeAck( pThreadData
                                    , Q
                                    , pSession
                                    , pTran
                                    , pnode
                                    , phMsgToUnstore
                                    , pTriggerSessionRedelivery
                                    , pAckState
                                    , ppJobThread);
    }
    else if (options == ismENGINE_CONFIRM_OPTION_RECEIVED)
    {
        iemq_completeReceiveAck( pThreadData
                               , Q
                               , pnode);
    }
    else
    {
        assert(  (options == ismENGINE_CONFIRM_OPTION_NOT_DELIVERED)
               ||(options == ismENGINE_CONFIRM_OPTION_NOT_RECEIVED)
               ||(options == ismENGINE_CONFIRM_OPTION_SESSION_CLEANUP));

        iemq_processNack( pThreadData
                        , Q
                        , pSession
                        , pTran
                        , pnode
                        , options
                        , true
                        , pTriggerSessionRedelivery
                        , pAckState);
    }
mod_exit:
    return rc;
}
int32_t iemq_acknowledge( ieutThreadData_t *pThreadData
                        , ismQHandle_t Qhdl
                        , ismEngine_Session_t *pSession
                        , ismEngine_Transaction_t *pTran
                        , void *pDelivery
                        , uint32_t options
                        , ismEngine_AsyncData_t *asyncInfo)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    iemqQNode_t *pnode = (iemqQNode_t *) pDelivery;
    int32_t rc = OK;
    bool triggerSessionRedelivery = false;
    uint32_t storeOps = 0;

    ieutTRACEL(pThreadData, pnode->orderId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "Acking Q %u Node Oid %lu pTran %p, options %u, msg %p, state: %u\n",
               __func__, Q->qId, pnode->orderId, pTran, options, pnode->msg,
               pnode->msgState);

    //We don't currently support transactional ack for delivery ids mqtt is the only user
    //of the 16bit delivery id and it doesn't transactionally ack
    assert((pnode->deliveryId == 0) || (pTran == NULL));

    if (options == ismENGINE_CONFIRM_OPTION_EXPIRED)
    {
        //This flag means that the client has expired one of the messages
        //in flight to it (and that *this* message should be consumed. The client
        //cannot guarantee exactly which message (just which destination). So we
        //update the counts of messages expired but count of messages with expiry
        //are adjusted based looking at actual expiry in headers of this message
        iemq_messageExpired(pThreadData, Qhdl);
        options = ismENGINE_CONFIRM_OPTION_CONSUMED;
    }

    //If there is a transaction, don't try and commit/roll it back etc until we have processed this nack
    if (pTran != NULL)
    {
        ietr_increasePreResolveCount(pThreadData, pTran);
    }

    iemq_prepareAck( pThreadData
                   , Qhdl
                   , pSession
                   , pTran
                   , pDelivery
                   , options
                   , &storeOps);

    if (storeOps > 0)
    {
        if (asyncInfo != NULL)
        {
            iemqAcknowledgeAsyncData_t ackData = {
                    IEMQ_ACKNOWLEDGE_ASYNCDATA_STRUCID,
                    Q,
                    pnode,
                    pSession,
                    pTran,
                    options,
                    NULL
            };

            if (       pThreadData->jobQueue != NULL
                    && pThreadData->threadType != AsyncCallbackThreadType
                    && ismEngine_serverGlobal.ThreadJobAlgorithm == ismENGINE_THREAD_JOB_QUEUES_ALGORITHM_EXTRA)
            {
                ieut_acquireThreadDataReference(pThreadData);
                ackData.pJobThread = pThreadData;
            }

            ismEngine_AsyncDataEntry_t newEntry =
            { ismENGINE_ASYNCDATAENTRY_STRUCID
                    , iemqQueueAcknowledge
                    , &ackData, sizeof(ackData)
                    , NULL
                    , {.internalFn = iemq_ackCommitted}};

            iead_pushAsyncCallback(pThreadData, asyncInfo, &newEntry);
            rc = iead_store_asyncCommit(pThreadData, false, asyncInfo);

            assert (rc == OK || rc == ISMRC_AsyncCompletion);

            if (rc == ISMRC_AsyncCompletion)
            {
                goto mod_exit;
            }
            else
            {
                if(ackData.pJobThread)
                {
                    ieut_releaseThreadDataReference(ackData.pJobThread);
                    ackData.pJobThread = NULL;
                }
                iead_popAsyncCallback( asyncInfo, newEntry.DataLen);
            }
        }
        else
        {
            iest_store_commit(pThreadData, false);
        }
    }

    rc = iemq_processAck( pThreadData
                        , Qhdl
                        , pSession
                        , pTran
                        , pnode
                        , options
                        , NULL
                        , &triggerSessionRedelivery
                        , NULL
                        , NULL);

    if (pTran != NULL)
    {
        //This ack should no longer block commit/rollback
        ietr_decreasePreResolveCount(pThreadData, pTran, true);
    }

mod_exit:
    if (triggerSessionRedelivery)
    {
        ismEngine_internal_RestartSession(pThreadData,  pSession, false);
    }

    if (rc == OK && options == ismENGINE_CONFIRM_OPTION_CONSUMED)
    {
        rc = iest_checkLazyMessages( pThreadData, asyncInfo);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n",
               __func__, rc);
    return rc;

}

void iemq_completeAckBatch( ieutThreadData_t *pThreadData
                          , ismQHandle_t Qhdl
                          , ismEngine_Session_t *pSession
                          , ismEngine_BatchAckState_t *pAckState)
{
    ieutTRACEL(pThreadData, pAckState->pConsumer, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "Completing ack batch for queue %p consumer %p. ackCount %u predeleteIncrement %u\n",
               __func__, Qhdl, pAckState->pConsumer, pAckState->ackCount,
               pAckState->preDeleteCountIncrement);

    if (pAckState->preDeleteCountIncrement > 0)
    {
        //Increase the predelete count if we have been told to...do before decreases so queue can't get deleted
        //in between.
        __sync_fetch_and_add(&(((iemqQueue_t *) Qhdl)->preDeleteCount),
                             pAckState->preDeleteCountIncrement);
    }

    if (pAckState->ackCount != 0)
    {
        if (pAckState->deliverOnCompletion)
        {
            (void) iemq_checkWaiters(pThreadData, Qhdl, NULL, NULL);
        }

        // We've now finished with the queue handle, reduce the ackcount
        // (which might cause this queue to be deleted)
        if (pAckState->pConsumer != NULL)
        {
            decreaseConsumerAckCount(pThreadData, pAckState->pConsumer,
                                     pAckState->ackCount);
        }
    }

    pAckState->pConsumer = NULL;
    pAckState->Qhdl = NULL;
    pAckState->ackCount = 0;
    pAckState->preDeleteCountIncrement = 0;
    pAckState->deliverOnCompletion = false;

    return;
}

int32_t iemq_relinquish( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , void *pDelivery
                       , ismEngine_RelinquishType_t relinquishType
                       , uint32_t *pStoreOpCount)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    iemqQNode_t *pnode = (iemqQNode_t *) pDelivery;
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, pnode->orderId, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "Relinquishing Q %u Node Oid %lu, msg %p, type %u state: %u\n",
               __func__, Q->qId, pnode->orderId, pnode->msg, relinquishType, pnode->msgState);

    if (   (pnode->msg == NULL)
        || (    (pnode->msgState != ismMESSAGE_STATE_DELIVERED)
             && (pnode->msgState != ismMESSAGE_STATE_RECEIVED))
        || (pnode->ackData.pConsumer != NULL) || (pnode->deliveryId == 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "Invalid Node relinquished", ISMRC_Error
                      , "pnode", pnode, sizeof(iemqQNode_t)
                      , "pnode->msgState", &pnode->msgState, sizeof(&pnode->msgState)
                      , "pnode->ackData.pConsumer", &pnode->ackData.pConsumer, sizeof(&pnode->ackData.pConsumer)
                      , "pnode->msg", pnode->msg, sizeof(ismEngine_Message_t)
                      , NULL);
    }

    if (     (relinquishType == ismEngine_RelinquishType_ACK_HIGHRELIABLITY)
        &&   (pnode->msg->Header.Reliability == ismMESSAGE_RELIABILITY_EXACTLY_ONCE))
    {
        //Like ack consume but without removing MDR
        if (pnode->inStore)
        {
            assert(pnode->hasMDR);

            rc = iemq_consumeMessageInStore(pThreadData, Q, pnode);

            if (rc != OK)
            {
                //Hmm that's bad!
                goto mod_exit;
            }

            // Consume will perform a store commit
            *pStoreOpCount = 0;
        }

        __sync_fetch_and_sub(&(Q->inflightDeqs), 1);
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -1);
        pThreadData->stats.bufferedMsgCount--;

        if (pnode->msg->Header.Expiry != 0)
        {
            //We're removing a message with expiry
            ieme_removeMessageExpiryData(pThreadData, (ismEngine_Queue_t *)Q, pnode->orderId);
        }

        DEBUG_ONLY int32_t oldDepth = __sync_fetch_and_sub(&(Q->bufferedMsgs), 1);
        assert(oldDepth > 0);
        if ((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0)
        {
            size_t messageBytes = IEMQ_MESSAGE_BYTES(pnode->msg);
            pThreadData->stats.remoteServerBufferedMsgBytes -= messageBytes;
            (void)__sync_fetch_and_sub((&Q->bufferedMsgBytes), messageBytes);
        }
        __sync_fetch_and_add(&(Q->dequeueCount), 1);

        iemq_releaseReservedSLEMem(pThreadData, pnode);

        bool needCleanup = iemq_needCleanAfterConsume(pnode);

        ismEngine_Message_t *msgToFree = pnode->msg;
        pnode->msg = MESSAGE_STATUS_EMPTY;
        pnode->msgState = ismMESSAGE_STATE_CONSUMED;

        iem_releaseMessage(pThreadData, msgToFree);

        if (needCleanup)
        {
            iemq_cleanupHeadPages(pThreadData, Q);
        }

        //If there were too many inflight messages we may need to restart delivery
        if (iemq_checkFullDeliveryStartable(pThreadData, Q))
        {
            (void)iemq_checkWaiters( pThreadData
                                   , (ismQHandle_t) Q
                                   , NULL
                                   , NULL);
        }
    }
    else if ( (relinquishType == ismEngine_RelinquishType_NACK_ALL)
              || (    (relinquishType == ismEngine_RelinquishType_ACK_HIGHRELIABLITY)
                   && (pnode->msg->Header.Reliability != ismMESSAGE_RELIABILITY_EXACTLY_ONCE)))
    {
        //Like nack not received - don't update store as delivered state will
        //be changed to avail on restart (as our caller will remove any mdr separately)
        pnode->hasMDR = false;
        pnode->deliveryId = 0;

        bool triggerSessionRedelivery = false;

        iemq_processNack( pThreadData
                         , Q
                         , NULL //allowed null session as !allowAlterAckListMDR
                         , NULL
                         , pnode
                         , ismENGINE_CONFIRM_OPTION_NOT_RECEIVED
                         , false
                         , &triggerSessionRedelivery
                         , NULL);

        assert (!triggerSessionRedelivery);
    }
    else
    {
        ieutTRACE_FFDC(ieutPROBE_001, true,
                       "Illegal relinquish type.", relinquishType,
                       NULL);
    }

mod_exit:
    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d \n",
               __func__, rc);
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
int32_t iemq_drainQ(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl)
{
    int32_t rc = OK;
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_ENTRY " *Q=%p\n",
               __func__, Q);

    rc = ISMRC_NotImplemented;

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__,
               rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Query queue statistics from a queue
///  @remarks
///    Returns a structure ismEngine_QueueStatistics_t which contains information
///    about the current state of the queue.
///
///  @param[in] Qhdl              Queue
///  @param[out] stats            Statistics data
///////////////////////////////////////////////////////////////////////////////
void iemq_getStats( ieutThreadData_t *pThreadData
                  , ismQHandle_t Qhdl
                  , ismEngine_QueueStatistics_t *stats)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;

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
    stats->BufferedMsgBytes = Q->bufferedMsgBytes;
    // Calculate some additional statistics
    stats->ProducedMsgs = stats->EnqueueCount + stats->QAvoidCount;
    stats->ConsumedMsgs = stats->DequeueCount + stats->QAvoidCount;
    if (stats->MaxMessages == 0)
    {
        stats->BufferedPercent = 0;
        stats->BufferedHWMPercent = 0;
    }
    else
    {
        stats->BufferedPercent = ((double) stats->BufferedMsgs * 100.0)
                                 / (double) stats->MaxMessages;
        stats->BufferedHWMPercent = ((double) stats->BufferedMsgsHWM * 100.0)
                                    / (double) stats->MaxMessages;
    }

    stats->PutsAttemptedDelta = (Q->qavoidCount + Q->enqueueCount
                                 + Q->rejectedMsgs) - Q->putsAttempted;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, "%s Q=%p msgs=%lu\n", __func__, Q,
               stats->BufferedMsgs);
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Explicitly set the value of the putsAttempted statistic
///
///  @param[in] Qhdl              Queue
///  @param[in] newPutsAttempted  Value to reset the stat to
///  @param[in] setType           The type of stat setting required
///////////////////////////////////////////////////////////////////////////////
void iemq_setStats( ismQHandle_t Qhdl
                  , ismEngine_QueueStatistics_t *stats
                  , ieqSetStatsType_t setType)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;

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
int32_t iemq_checkAvailableMsgs( ismQHandle_t Qhdl
                               ,
                                ismEngine_Consumer_t *pConsumer)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    int32_t rc = OK;

    if ((!pConsumer->fDestructiveGet) || (pConsumer->selectionRule != NULL))
    {
        //We're a browser or consumer with selection
        if (pConsumer->iemqNoMsgCheckVal == Q->checkWaitersVal)
        {
            //This consumer is uptodate
            rc = ISMRC_NoMsgAvail;
        }
    }
    else
    {
        uint64_t inflight = Q->inflightDeqs + Q->inflightEnqs;
        uint64_t buffered = Q->bufferedMsgs;

        if (buffered <= inflight)
        {
            //None of the messages look available...
            rc = ISMRC_NoMsgAvail;
        }
    }

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Write descriptions of iemq structures to the dump
///
///  @param[in]     dumpHdl  Dump file handle
///////////////////////////////////////////////////////////////////////////////
void iemq_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *) dumpHdl;

    iedm_describe_iemqQueue_t(dump->fp);
    iedm_describe_iemqCursor_t(dump->fp);
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
void iemq_dumpQ( ieutThreadData_t *pThreadData
               , ismQHandle_t Qhdl
               , iedmDumpHandle_t dumpHdl)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    iedmDump_t *dump = (iedmDump_t *) dumpHdl;

    if (iedm_dumpStartObject(dump, Q) == true)
    {
        iedm_dumpStartGroup(dump, "MultiConsumerQ");

        // Dump the QName, PolicyInfo and Queue
        if (Q->Common.QName != NULL)
        {
            iedm_dumpData(dump, "QName", Q->Common.QName,
                          iere_usable_size(iemem_multiConsumerQ, Q->Common.QName));
        }
        iepi_dumpPolicyInfo(Q->Common.PolicyInfo, dumpHdl);
        iedm_dumpData(dump, "iemqQueue_t", Q,
                      iere_usable_size(iemem_multiConsumerQ, Q));

        int32_t detailLevel = dump->detailLevel;

        if (detailLevel >= iedmDUMP_DETAIL_LEVEL_7)
        {
            // Need to get locks for the rest of the information
            if (pthread_rwlock_wrlock(&(Q->headlock)) != 0)
                goto mod_exit;
            if (pthread_rwlock_rdlock(&(Q->waiterListLock)) != 0)
            {
                pthread_rwlock_unlock(&(Q->headlock));
                goto mod_exit;
            }
            if (pthread_mutex_lock(&(Q->getlock)) != 0)
            {
                pthread_rwlock_unlock(&(Q->headlock));
                pthread_rwlock_unlock(&(Q->waiterListLock));
                goto mod_exit;
            }

            // Dump Consumers
            ismEngine_Consumer_t *firstWaiter = Q->firstWaiter;

            if (firstWaiter != NULL)
            {
                ismEngine_Consumer_t *waiter = firstWaiter;
                do
                {
                    dumpConsumer(pThreadData, waiter, dumpHdl);
                    waiter = waiter->iemqPNext;
                }
                while (waiter != firstWaiter);
            }

            // Dump a subset of pages
            iemqQNodePage_t *pDisplayPageStart[2];
            int32_t maxPages, pageNum;

            // Detail level less than 9, we only dump a set of pages at the head and tail
            if (detailLevel < iedmDUMP_DETAIL_LEVEL_9)
            {
                pageNum = 0;
                maxPages = 3;

                pDisplayPageStart[0] = Q->headPage;
                pDisplayPageStart[1] = Q->headPage;

                for (iemqQNodePage_t *pPage = pDisplayPageStart[0];
                        pPage != NULL; pPage = pPage->next)
                {
                    if (++pageNum > maxPages)
                        pDisplayPageStart[1] = pDisplayPageStart[1]->next;
                }
            }
            else
            {
                maxPages = 0;
                pDisplayPageStart[0] = Q->headPage;
                pDisplayPageStart[1] = NULL;
            }

            // Go through the lists of pages dumping maxPages of each
            for (int32_t i = 0; i < 2; i++)
            {
                iemqQNodePage_t *pDumpPage = pDisplayPageStart[i];

                pageNum = 0;

                while (pDumpPage != NULL)
                {
                    pageNum++;

                    uint32_t nodeNum;
                    uint32_t nodesInPage = pDumpPage->nodesInPage;
                    uint32_t firstMsgNode = nodesInPage;

                    // Run through the nodes on the page once calculating lock state
                    bool nodeLockState[nodesInPage];

                    for (nodeNum = 0; nodeNum < nodesInPage; nodeNum++)
                    {
                        iemqQNode_t *node = &(pDumpPage->nodes[nodeNum]);

                        if (firstMsgNode == nodesInPage && node->msg != NULL)
                            firstMsgNode = nodeNum;

                        // Is this node locked or not?
                        ielmLockName_t LockName = { .Msg = {
                                ielmLOCK_TYPE_MESSAGE, Q->qId, node->orderId
                            }
                        };

                        int32_t rc = ielm_instantLockWithPeek(&LockName, NULL,
                                                              NULL);
                        assert((rc == OK) || (rc == ISMRC_LockNotGranted));

                        nodeLockState[nodeNum] = (rc != OK);
                    }

                    // Dump the lock state of the nodes in the page
                    // Actually dump the page and lock data
                    size_t pageSize = offsetof(iemqQNodePage_t, nodes)
                                      + (sizeof(iemqQNode_t) * pDumpPage->nodesInPage);

                    iedm_dumpDataV(dump, "iemqQNodePageAndLocks", 2, pDumpPage,
                                   pageSize, nodeLockState, sizeof(nodeLockState));

                    // If this page has messages, and user data is requested display them.
                    if (firstMsgNode != nodesInPage
                            && dump->userDataBytes != 0)
                    {
                        iedm_dumpStartGroup(dump, "Available Msgs");

                        for (nodeNum = firstMsgNode; nodeNum < nodesInPage;
                                nodeNum++)
                        {
                            iemqQNode_t *node = &(pDumpPage->nodes[nodeNum]);

                            // We can only dump the available messages since if the node is in
                            // any other state, the msg pointer can be removed at any time.
                            if (   (node->msg != NULL)
                                && (node->msgState == ismMESSAGE_STATE_AVAILABLE))
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

                    if (pageNum == maxPages)
                        break;

                    pDumpPage = pDumpPage->next;
                }
            }

            //Release locks
            (void) pthread_mutex_unlock(&(Q->getlock));
            (void) pthread_rwlock_unlock(&(Q->waiterListLock));
            (void) pthread_rwlock_unlock(&(Q->headlock));
        }

        iedm_dumpEndGroup(dump);
        iedm_dumpEndObject(dump, Q);
    }

mod_exit:

    return;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Replay Soft Log
///  @remark
///     This function executes a softlog records for MultiConsumer Queue
///     for either Commit or Rollback.
///
///  @param[in]     Phase            Commit/PostCommit/Rollback/PostRollback
///  @param[in]     pThreadData      Thread data
///  @param[in]     pTran            Transaction
///  @param[in]     pEntry           Softlog entry
///  @param[in]     pRecord          Transaction state record
///////////////////////////////////////////////////////////////////////////////
void iemq_SLEReplayConsume( ietrReplayPhase_t Phase
                          , ieutThreadData_t *pThreadData
                          , ismEngine_Transaction_t *pTran
                          , void *pEntry
                          , ietrReplayRecord_t *pRecord)
{
    int32_t rc;
    iemqSLEConsume_t *pSLE = (iemqSLEConsume_t *) pEntry;
    iemqQueue_t *Q = pSLE->pQueue;
    iemqQNode_t *pNode;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, pSLE, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "Phase=%d pEntry=%p\n", __func__, Phase, pEntry);

    switch (Phase)
    {
        case Commit:
            pNode = pSLE->pNode;

            // Delete the reference from the queue to the msg
            if (pNode->inStore)
            {
                // Part of store transaction controlled by the engine transaction
                if (pTran->fIncremental)
                {
                    ietr_deleteTranRef(pThreadData, pTran, &pSLE->TranRef);
                }

                rc = ism_store_deleteReference(pThreadData->hStream,
                                               Q->QueueRefContext, pNode->hMsgRef, pNode->orderId, 0);
                if (UNLIKELY(rc != OK))
                {
                    ieutTRACE_FFDC(ieutPROBE_001, true,
                                   "ism_store_deleteReference (multiConsumer) failed.", rc,
                                   "Internal Name", Q->InternalName, sizeof(Q->InternalName),
                                   "Queue", Q,sizeof(iemqQueue_t),
                                   "Reference", &pNode->hMsgRef, sizeof(pNode->hMsgRef),
                                   "OrderId", &pNode->orderId, sizeof(pNode->orderId),
                                   "pNode", pNode, sizeof(iemqQNode_t),
                                   NULL);
                }

                pRecord->StoreOpCount++;
            }

            pSLE->bCleanHead = iemq_needCleanAfterConsume(pNode);
            size_t messageBytes = IEMQ_MESSAGE_BYTES(pNode->msg);
            pNode->msg = MESSAGE_STATUS_EMPTY;

            iere_primeThreadCache(pThreadData, resourceSet);
            iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -1);
            pThreadData->stats.bufferedMsgCount--;
            DEBUG_ONLY int32_t oldDepth = __sync_fetch_and_sub(&(Q->bufferedMsgs), 1);
            assert(oldDepth > 0);
            if ((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0)
            {
                pThreadData->stats.remoteServerBufferedMsgBytes -= messageBytes;
                (void)__sync_fetch_and_sub((&Q->bufferedMsgBytes), messageBytes);
            }
            __sync_fetch_and_add(&(Q->dequeueCount), 1);
            __sync_fetch_and_sub(&(Q->inflightDeqs), 1);

            break;

       case PostCommit:
           if (pSLE->bInStore)
           {
               //Only now we have committed the removal of the reference we can potentially unstore the message
               rc = iest_unstoreMessage(pThreadData, pSLE->pMsg, false, false,
                                        NULL, &(pRecord->StoreOpCount));

               if (UNLIKELY(rc != OK))
               {
                   ieutTRACE_FFDC(ieutPROBE_002, false,
                                  "iest_unstoreMessage (multiConsumer) failed.", rc,
                                  "SLE", pSLE, sizeof(*pSLE), NULL);
               }
           }
           iem_releaseMessage(pThreadData, pSLE->pMsg);

           // Attempt to remove the messages we consumed
           if (pSLE->bCleanHead)
           {
               iemq_cleanupHeadPages(pThreadData, Q);
           }

           //If there were too many inflight messages we may need to restart delivery
           if (iemq_checkFullDeliveryStartable(pThreadData, Q))
           {
               (void)iemq_checkWaiters( pThreadData
                                      , (ismQHandle_t) Q
                                      , NULL
                                      , NULL);
           }

           //Now reduce the pre delete count which Q to be deleted...
           iemq_reducePreDeleteCount_internal(pThreadData, Q);

           // Something we've just done e.g. deleting the queue
           // may have performed a store commit - which means that there are no outstanding
           // store operations.
           //
           // If there were some prior to this call, get the real value from the store.
           if (pRecord->StoreOpCount > 0)
           {
               ism_store_getStreamOpsCount(pThreadData->hStream,
                                           &pRecord->StoreOpCount);
           }
           break;

       case Rollback:
           pNode = pSLE->pNode;
           pNode->msgState = ismMESSAGE_STATE_AVAILABLE;

           if (pNode->inStore)
           {
               // The rollback phase cannot make updates to the store
               // which must be committed when the transaction is
               // implemented as a store transaction
               assert(!(pTran->fAsStoreTran));

               iemq_updateMsgRefInStore(pThreadData, Q, pNode,
                                        ismMESSAGE_STATE_AVAILABLE,
                                        false,
                                        pNode->deliveryCount,
                                        false);

               pRecord->StoreOpCount++;
           }

           __sync_fetch_and_sub(&(Q->inflightDeqs), 1);


           //Adjust expiry stat if necessary...could move to post (outside commit lock)
           //if add a way of remembering in the SLE...
           if (pSLE->pMsg->Header.Expiry != 0)
           {
               //We're making available a message with expiry (through rolling back a get)
               iemeBufferedMsgExpiryDetails_t msgExpiryData = { pNode->orderId , pNode, pSLE->pMsg->Header.Expiry };
               ieme_addMessageExpiryData( pThreadData, (ismEngine_Queue_t *)Q,  &msgExpiryData);
           }

           break;

       case MemoryRollback:
           pNode = pSLE->pNode;
           iemqCursor_t cursor;

           cursor.c.orderId = pNode->orderId;
           cursor.c.pNode = pNode;
           iemq_rewindGetCursor(pThreadData, Q, cursor);
           break;

       case PostRollback:
           // Attempt to deliver the messages
           (void) iemq_checkWaiters(pThreadData, (ismQHandle_t) Q, NULL, NULL);

           //Now reduce the pre delete count which Q to be deleted...
           iemq_reducePreDeleteCount_internal(pThreadData, Q);

           //check waiters may have done a store commit, resync store op-count
           if (pRecord->StoreOpCount > 0)
           {
               ism_store_getStreamOpsCount(pThreadData->hStream,
                                           &pRecord->StoreOpCount);
           }
           break;

       default:
           ieutTRACE_FFDC( ieutPROBE_003, true, "Invalid phase called", ISMRC_Error
                         , "SLE", pSLE, sizeof(*pSLE)
                         , "Phase", &Phase, sizeof(Phase)
                         , NULL);
           break;
    }

    ieutTRACEL(pThreadData, pSLE, ENGINE_CEI_TRACE, FUNCTION_EXIT "\n", __func__);
    return;
}



static void iemq_completePutPostCommit(
        ieutThreadData_t               *pThreadData,
        iemqPutPostCommitInfo_t *pPutInfo)
{
    assert(pPutInfo->Q != NULL);

    if (pPutInfo->deleteCountDecrement != 0)
    {
        iemq_reducePreDeleteCount_internal(pThreadData, pPutInfo->Q);
        if (pPutInfo->deleteCountDecrement > 1)
        {
            assert(pPutInfo->deleteCountDecrement == 2);
            iemq_reducePreDeleteCount_internal(pThreadData, pPutInfo->Q);
        }
    }

    return;
}

static int32_t iemq_completePutPostCommit_asyncCommit(
        ieutThreadData_t               *pThreadData,
        int32_t                         retcode,
        ismEngine_AsyncData_t          *asyncInfo,
        ismEngine_AsyncDataEntry_t     *context)
{
    assert (retcode == OK);
    iemqPutPostCommitInfo_t *pPutInfo = (iemqPutPostCommitInfo_t *)(context->Data);

    iemq_completePutPostCommit(pThreadData, pPutInfo);

    //We've finished, remove entry so if we go async we don't try to deliver this message again
    iead_popAsyncCallback( asyncInfo, context->DataLen);

    return retcode;
}

//Called from either post commit or jobcallback to discard messages, do deliveries etc.
static inline int32_t iemq_postTranPutWork( ieutThreadData_t *pThreadData
                                          , ismEngine_Transaction_t *pTran
                                          , ietrAsyncTransactionData_t *asyncTran
                                          , ismEngine_AsyncData_t *pAsyncData
                                          , ietrReplayRecord_t *pRecord
                                          , iemqQueue_t *Q)
{
    int32_t rc = OK;

    //Discard messages down to ensure there is "space" for the message we put
    iepiPolicyInfo_t *pPolicyInfo = Q->Common.PolicyInfo;

    if (((pPolicyInfo->maxMessageCount > 0) && (Q->bufferedMsgs >= pPolicyInfo->maxMessageCount)) ||
        ((pPolicyInfo->maxMessageBytes > 0) && (Q->bufferedMsgBytes >= pPolicyInfo->maxMessageBytes)))
     {
         //Check if we can remove expired messages and get below the max...
         ieme_reapQExpiredMessages(pThreadData, (ismEngine_Queue_t *)Q);

         if (    (pPolicyInfo->maxMsgBehavior == DiscardOldMessages)
              && (((pPolicyInfo->maxMessageCount > 0) && (Q->bufferedMsgs > pPolicyInfo->maxMessageCount)) ||
                  ((pPolicyInfo->maxMessageBytes > 0) && (Q->bufferedMsgBytes > pPolicyInfo->maxMessageBytes))))
         {
             //We reclaim before checkwaiters so that checkwaiters
             //doesn't put some really old messages in flight forcing us to remove newer ones
             iemq_reclaimSpace(pThreadData, (ismQHandle_t) Q, true);
         }
     }

     //Set up the data we'd need to continue if delivery goes async...
     // Delete the queue if this put was the thing preventing the queue being deleted
     // which could have been a transacted put from a rehydrated transaction or one
     // from a live transaction to a point-to-point queue
     iemqPutPostCommitInfo_t completePostCommitInfo = { IEMQ_PUTPOSTCOMMITINFO_STRUCID
                                                      ,((  (pTran->TranFlags & ietrTRAN_FLAG_REHYDRATED)   != 0
                                                         ||(Q->QOptions & ieqOptions_PUBSUB_QUEUE_MASK) == 0) ? 2 : 1)
                                                     , Q };

     if (asyncTran != NULL)
     {
         bool usedLocalAsyncData;

         ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
                 {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineTranCommit,  NULL,   0,     asyncTran, {.internalFn = ietr_asyncFinishParallelOperation }},
                 {ismENGINE_ASYNCDATAENTRY_STRUCID, iemqQueuePostCommit, &completePostCommitInfo, sizeof(completePostCommitInfo),
                                                               NULL, {.internalFn = iemq_completePutPostCommit_asyncCommit } }
         };
         ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID
                 , (pTran->pSession!= NULL) ? pTran->pSession->pClient : NULL
                         , IEAD_MAXARRAYENTRIES, 2, 0, true, 0, 0, asyncArray};

#if 0
         //Adjust Tran so this SLE is not called again in this phase if we go async
         asyncTran->ProcessedPhaseSLEs++;
         ieutTRACE_HISTORYBUF(pThreadData, asyncTran->ProcessedPhaseSLEs);
#endif

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

         // Allow for this to go async as a parallel operation
         __sync_fetch_and_add(&asyncTran->parallelOperations, 1);
         __sync_fetch_and_add(&Q->preDeleteCount, 1);

         // Attempt to deliver the messages
         rc = iemq_checkWaiters(pThreadData, (ismEngine_Queue_t *)Q, pAsyncData, NULL);
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
             ieutTRACE_HISTORYBUF(pThreadData, asyncTran->ProcessedPhaseSLEs);
#endif
         }
     }
     else
     {
         completePostCommitInfo.deleteCountDecrement--;
         (void) iemq_checkWaiters(pThreadData, (ismEngine_Queue_t *)Q, NULL, NULL);
     }

     if (rc == OK)
     {
         iemq_completePutPostCommit(pThreadData, &completePostCommitInfo);
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
///     This function executes a softlog records for MultiConsumer Queue
///     for either Commit or Rollback.
///
///  @param[in]     Phase            Commit/PostCommit/Rollback/PostRollback
///  @param[in]     pThreadData      Thread data
///  @param[in]     pTran            Transaction
///  @param[in]     pEntry           Softlog entry
///  @param[in]     pRecord          Transaction state record
///
///  @returns OK or AsyncCompletion
///
///////////////////////////////////////////////////////////////////////////////
int32_t iemq_SLEReplayPut( ietrReplayPhase_t Phase
                         , ieutThreadData_t *pThreadData
                         , ismEngine_Transaction_t *pTran
                         , void *pEntry
                         , ietrReplayRecord_t *pRecord
                         , ismEngine_AsyncData_t *pAsyncData
                         , ietrAsyncTransactionData_t *asyncTran)
{
    int32_t rc = OK;
    iemqSLEPut_t *pSLE = (iemqSLEPut_t *) pEntry;
    iemqQueue_t *Q = pSLE->pQueue;
    iemqQNode_t *pNode;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, pSLE, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "Phase=%d pEntry=%p\n", __func__, Phase, pEntry);

    iemqCursor_t cursor;

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
       case Commit:
           pNode = pSLE->pNode;

           assert(Q->inflightEnqs > 0);
           (void) __sync_sub_and_fetch(&(Q->inflightEnqs), 1);
           (void) __sync_add_and_fetch(&(Q->enqueueCount), 1);


           //Adjust expiry data if necessary...could move to post (outside commit lock)
           //if add a way of remembering in the SLE...
           if (pSLE->pMsg->Header.Expiry != 0)
           {
               //We're making available a message with expiry
               iemeBufferedMsgExpiryDetails_t msgExpiryData = { pNode->orderId , pNode, pSLE->pMsg->Header.Expiry };
               ieme_addMessageExpiryData( pThreadData, (ismEngine_Queue_t *)Q,  &msgExpiryData);
           }
           break;

       case MemoryCommit:
           //Reset get cursor if necessary, we still have the message
           //locked in the lockmanager so we can look at the orderId....
           if (!pSLE->bSavepointRolledback)
           {
               pNode = pSLE->pNode;

               cursor.c.pNode = pNode;
               cursor.c.orderId = pNode->orderId;

               iemq_rewindGetCursor(pThreadData, Q, cursor);
           }
           break;

       case PostCommit:
           if (pRecord->JobThreadId != ietrNO_JOB_THREAD)
           {
               if (iemq_scheduleWork(pThreadData, Q, pRecord->JobThreadId))
               {
                   __sync_fetch_and_add(&Q->preDeleteCount, 1);
                   pSLE->scheduledThread = pRecord->JobThreadId;
                   pRecord->JobRequired = true;
               }
           }
           else
           {
               rc = iemq_postTranPutWork( pThreadData
                                        , pTran
                                        , asyncTran
                                        , pAsyncData
                                        , pRecord
                                        , Q);
           }
           break;

       case JobCallback:
           iemq_clearScheduledWork(pThreadData, Q, pSLE->scheduledThread);

           rc = iemq_postTranPutWork( pThreadData
                                    , pTran
                                    , asyncTran
                                    , pAsyncData
                                    , pRecord
                                    , Q);

          //Reduce the count that we increased when we scheduled this
          iemq_reducePreDeleteCount_internal(pThreadData, Q);
          break;

       case Rollback:
           pNode = pSLE->pNode;

           // Delete the reference from the queue to the msg (for store
           // transactions the rollback will do this for us and for savepoint rolback, the adding the msg & queue ref
           // was done under a store tran that has been rolled back so... in memory only for this bit)
           if ((pNode->inStore) && !(pTran->fAsStoreTran) && !(pSLE->bSavepointRolledback))
           {
               if (pTran->fIncremental)
               {
                   ietr_deleteTranRef(pThreadData, pTran, &pSLE->TranRef);
               }

               rc = ism_store_deleteReference(pThreadData->hStream,
                                              Q->QueueRefContext, pNode->hMsgRef, pNode->orderId, 0);
               if (UNLIKELY(rc != OK))
               {
                   ieutTRACE_FFDC( ieutPROBE_001, true,
                                       "ism_store_deleteReference (multiConsumer) failed.", rc
                                 , "Internal Name", Q->InternalName,  sizeof(Q->InternalName)
                                 , "Queue", Q, sizeof(iemqQueue_t)
                                 , "Reference", &pNode->hMsgRef, sizeof(pNode->hMsgRef)
                                 , "OrderId", &pNode->orderId, sizeof(pNode->orderId)
                                 , "pNode", pNode, sizeof(iemqQNode_t)
                                 , NULL);
               }

               pRecord->StoreOpCount++;
           }

           // Only need to process consumers or cleanup pages when not
           // rehydrating messages
           if ((Q->QOptions & (ieqOptions_IN_RECOVERY|ieqOptions_IMPORTING)) == 0)
           {
               pSLE->bCleanHead = iemq_needCleanAfterConsume(pNode);
           }
           else
           {
               pSLE->bCleanHead = false;
           }
           size_t messageBytes = IEMQ_MESSAGE_BYTES(pNode->msg);
           pNode->msg = MESSAGE_STATUS_EMPTY;
           pNode->msgState = ismMESSAGE_STATE_CONSUMED;

           DEBUG_ONLY int32_t oldDepth;
           iere_primeThreadCache(pThreadData, resourceSet);
           iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -1);
           pThreadData->stats.bufferedMsgCount--;
           oldDepth = __sync_fetch_and_sub(&(Q->bufferedMsgs), 1);
           if ((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0)
           {
               pThreadData->stats.remoteServerBufferedMsgBytes -= messageBytes;
               (void)__sync_fetch_and_sub((&Q->bufferedMsgBytes), messageBytes);
           }
           (void) __sync_sub_and_fetch(&(Q->inflightEnqs), 1);
           assert(oldDepth > 0);

           break;

       case PostRollback:
           //Now we've removed our reference to the message, we can remove it from the store
           //if no-one else is using it
           if (pSLE->bInStore && !pSLE->bSavepointRolledback)
           {
               (void) iest_unstoreMessage(pThreadData, pSLE->pMsg,
                                          pTran->fAsStoreTran, false,
                                          NULL, &(pRecord->StoreOpCount));
           }
           iem_releaseMessage(pThreadData, pSLE->pMsg);

           if (pSLE->bCleanHead)
           {
               iemq_cleanupHeadPages(pThreadData, Q);
           }

           //If there were too many inflight messages we may need to restart delivery
           if (iemq_checkFullDeliveryStartable(pThreadData, Q))
           {
               (void)iemq_checkWaiters( pThreadData
                                      , (ismQHandle_t) Q
                                      , NULL
                                      , NULL);
           }

           // Delete the queue if this put was the thing preventing the queue being deleted
           // which could have been a transacted put from a rehydrated transaction or one
           // from a live transaction to a point-to-point queue
           if (  (pTran->TranFlags & ietrTRAN_FLAG_REHYDRATED) != 0
              || (Q->QOptions & ieqOptions_PUBSUB_QUEUE_MASK) == 0)
           {
               iemq_reducePreDeleteCount_internal(pThreadData, Q);
           }

           // Something we've just done may have performed a store commit - which
           // means that there are no outstanding store operations.
           //
           // If there were some prior to this call, get the real value from the store.
           if (pRecord->StoreOpCount > 0)
           {
               ism_store_getStreamOpsCount(pThreadData->hStream,
                                           &pRecord->StoreOpCount);
           }
           break;

       case SavepointRollback:
           pSLE->bSavepointRolledback = true; //When we commit+postcommit, do rollback+postropllback instead
           break;

       default:
           ieutTRACE_FFDC( ieutPROBE_002, true, "Invalid phase called", ISMRC_Error
                         , "SLE", pSLE, sizeof(*pSLE)
                         , "Phase", &Phase, sizeof(Phase)
                         , NULL);
           break;
    }

    ieutTRACEL(pThreadData, pSLE, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
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
ismStore_Handle_t iemq_getDefnHdl(ismQHandle_t Qhdl)
{
    return ((iemqQueue_t *) Qhdl)->hStoreObj;
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
ismStore_Handle_t iemq_getPropsHdl(ismQHandle_t Qhdl)
{
    return ((iemqQueue_t *) Qhdl)->hStoreProps;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Update the QPR / SPR of a queue.
///  @remark
///     This function can be called to set a new queue properties record for
///     a queue.
///
///  @return                       - OK
///////////////////////////////////////////////////////////////////////////////
void iemq_setPropsHdl( ismQHandle_t Qhdl
                     , ismStore_Handle_t propsHdl)
{
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;

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
int32_t iemq_updateProperties( ieutThreadData_t *pThreadData
                             , ismQHandle_t Qhdl
                             , const char *pQName
                             , ieqOptions_t QOptions
                             , ismStore_Handle_t propsHdl
                             , iereResourceSetHandle_t resourceSet )
{
    int32_t rc = OK;
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    bool storeChange = false;
    iereResourceSetHandle_t currentResourceSet = Q->Common.resourceSet;

    // Deal with any updates to Queue name
    if (pQName != NULL)
    {
        iere_primeThreadCache(pThreadData, currentResourceSet);

        if (NULL == Q->Common.QName || strcmp(pQName, Q->Common.QName) != 0)
        {
            char *newQName = iere_realloc(pThreadData,
                                          currentResourceSet,
                                          IEMEM_PROBE(iemem_multiConsumerQ, 5),
                                          Q->Common.QName, strlen(pQName) + 1);

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
            iere_free(pThreadData, currentResourceSet, iemem_multiConsumerQ, Q->Common.QName);
            Q->Common.QName = NULL;
            // Can only make a store change for point-to-point queues
            storeChange = ((Q->QOptions & ieqOptions_PUBSUB_QUEUE_MASK) == 0);
        }
    }

    // If we are still in recovery, we can update the queue options since they
    // are dealt with when recovery is completed - the queue options are not
    // stored.
    if (Q->QOptions & (ieqOptions_IN_RECOVERY|ieqOptions_IMPORTING))
    {
        assert((QOptions & (ieqOptions_IN_RECOVERY|ieqOptions_IMPORTING)) != 0);

        if(  (QOptions    & ieqOptions_SINGLE_CONSUMER_ONLY) == 0
           &&(Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY) != 0)
        {
            rc = iemq_createSchedulingInfo(pThreadData, Q);

            if (rc != OK)goto mod_exit;
        }
        else if (  (QOptions    & ieqOptions_SINGLE_CONSUMER_ONLY) != 0
                 &&(Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY) == 0)
        {
            iemq_destroySchedulingInfo(pThreadData, Q);
        }

        Q->QOptions = QOptions;
    }

    // If the queue store information has changed, update the QPR in the store.
    if (storeChange && (Q->hStoreProps != ismSTORE_NULL_HANDLE))
    {
        // propsHdl should only be non-null during restart, and only the first
        // time this queue is discovered at restart.
        assert(propsHdl == ismSTORE_NULL_HANDLE);
        assert(pThreadData != NULL);

        rc = iest_updateQueue(pThreadData, Q->hStoreObj, Q->hStoreProps,
                              Q->Common.QName, &propsHdl);

        if (rc != OK)
            goto mod_exit;

        assert(propsHdl != ismSTORE_NULL_HANDLE && propsHdl != Q->hStoreProps);
    }

    // Update the properties handle (this does nothing if it is ismSTORE_NULL_HANDLE)
    iemq_setPropsHdl(Qhdl, propsHdl);

    // Update the resourceSet for the queue
    iemq_updateResourceSet(pThreadData, Q, resourceSet);

mod_exit:

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Update the resource set for a queue that currently has none.
///////////////////////////////////////////////////////////////////////////////
void iemq_updateResourceSet( ieutThreadData_t *pThreadData
                           , iemqQueue_t *Q
                           , iereResourceSetHandle_t resourceSet)
{
    if (resourceSet != iereNO_RESOURCE_SET)
    {
        assert(Q->Common.resourceSet == iereNO_RESOURCE_SET);

        Q->Common.resourceSet = resourceSet;

        iere_primeThreadCache(pThreadData, resourceSet);

        // Update memory for the Queue
        int64_t fullMemSize = (int64_t)iere_full_size(iemem_multiConsumerQ, Q);
        iere_updateMem(pThreadData, resourceSet, IEMEM_PROBE(iemem_multiConsumerQ, 7), Q, fullMemSize);

        if (Q->Common.QName != NULL)
        {
            fullMemSize = (int64_t)iere_full_size(iemem_multiConsumerQ, Q->Common.QName);
            iere_updateMem(pThreadData, resourceSet, IEMEM_PROBE(iemem_multiConsumerQ, 8), Q->Common.QName, fullMemSize);
        }

        if (Q->schedInfo != NULL)
        {
            fullMemSize = (int64_t)iere_full_size(iemem_multiConsumerQ, Q->schedInfo);
            iere_updateMem(pThreadData, resourceSet, IEMEM_PROBE(iemem_multiConsumerQ, 9), Q->schedInfo, fullMemSize);
        }

        assert(Q->PageMap != NULL);
        fullMemSize = (int64_t)iere_full_size(iemem_multiConsumerQ, Q->PageMap);
        iere_updateMem(pThreadData, resourceSet, IEMEM_PROBE(iemem_multiConsumerQ, 10), Q->PageMap, fullMemSize);


        iemqQConsumedNodeInfo_t *nodeInfo  = pFirstConsumedNode;
        while (nodeInfo != NULL)
        {
            fullMemSize = (int64_t)iere_full_size(iemem_multiConsumerQ, nodeInfo);
            iere_updateMem(pThreadData, resourceSet, IEMEM_PROBE(iemem_multiConsumerQ, 11), nodeInfo, fullMemSize);
            nodeInfo = nodeInfo->pNext;
        }

        for(int32_t pageNum=0; pageNum<Q->PageMap->PageCount; pageNum++)
        {
            iemqQNodePage_t *pCurPage = Q->PageMap->PageArray[pageNum].pPage;

            // Update memory for the current page.
            fullMemSize = (int64_t)iere_full_size(iemem_multiConsumerQPage, pCurPage);
            iere_updateMem(pThreadData, resourceSet, IEMEM_PROBE(iemem_multiConsumerQPage, 2), pCurPage, fullMemSize);

            for(int32_t nodeNum=0; nodeNum<pCurPage->nodesInPage; nodeNum++)
            {
                iemqQNode_t *pCurNode = &pCurPage->nodes[nodeNum];

                // Update rehydrated Messages (this will also update buffered msg counts)
                if (pCurNode->msg != NULL)
                {
                    iere_updateMessageResourceSet(pThreadData, resourceSet, pCurNode->msg, false, false);
                }
            }
        }
    }
}

//forget about inflight_messages - no effect for this queue. The queue doesn't track message counts,
//consumers and sessions do.
void iemq_forgetInflightMsgs( ieutThreadData_t *pThreadData
                            , ismQHandle_t Qhdl)
{
}

//This queue has no messages that the delivery must complete
//for (even after unsub/reconnect etc)- that's an oddity of MQTT which this queue does do.
bool iemq_redeliverEssentialOnly( ieutThreadData_t *pThreadData
                                , ismQHandle_t Qhdl)
{
    return false;
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
ieqExpiryReapRC_t  iemq_reapExpiredMsgs( ieutThreadData_t *pThreadData
                                       , ismQHandle_t Qhdl
                                       , uint32_t nowExpire
                                       , bool forcefullscan
                                       , bool expiryListLocked)
{
    bool reapComplete=false;
    ieqExpiryReapRC_t rc = ieqExpiryReapRC_OK;
    iemqQueue_t *Q = (iemqQueue_t *)Qhdl;
    iemeQueueExpiryData_t *pQExpiryData = (iemeQueueExpiryData_t *)Q->Common.QExpiryData;
    bool pageCleanupNeeded = false;

    ieutTRACEL(pThreadData,Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p \n", __func__,Q);

    iemq_takeReadHeadLock(Q);

    bool gotExpiryLock = ieme_startReaperQExpiryScan( pThreadData, (ismEngine_Queue_t *)Q);

    if (!gotExpiryLock)
    {
        iemq_releaseHeadLock(Q);

        rc = ieqExpiryReapRC_NoExpiryLock;
        goto mod_exit;
    }



    if (!forcefullscan)
    {
        iemqQNode_t *expiredNodes[NUM_EARLIEST_MESSAGES];
        uint32_t numExpiredNodes = 0;

        //Work out the earliest valid orderId that points to something in the queue
        uint64_t earliestOrderId = Q->headPage->nodes[0].orderId;

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
                iemqQNode_t *qnode = (iemqQNode_t *)pQExpiryData->earliestExpiryMessages[i].qnode;

                if (earliestOrderId <= pQExpiryData->earliestExpiryMessages[i].orderId)
                {
                    int gotnoderc = iemq_updateMsgIfAvail( pThreadData, Q, qnode, false, ieqMESSAGE_STATE_DISCARDING);

                    if (gotnoderc == OK)
                    {
                        expiredNodes[numExpiredNodes] = qnode;
                        numExpiredNodes++;
                        pQExpiryData->messagesWithExpiry--;
                        pThreadData->stats.bufferedExpiryMsgCount--;
                    }
                    else if (gotnoderc == ISMRC_NoMsgAvail)
                    {
                        //Message was locked... its been got/discarded/browsed
                        //...We'll abort this fast scan to keep the code here simple and try again on the
                        //next reap... could do this a few times and then do a full scan
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
                        ieutTRACE_FFDC( ieutPROBE_001, true, "Marking node consumed", gotnoderc
                                                             , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                                                             , "Queue", Q, sizeof(iemqQueue_t)
                                                             , NULL);
                    }
                }

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
            iemq_destroyMessageBatch( pThreadData
                                    , Q
                                    , numExpiredNodes
                                    , expiredNodes
                                    , false
                                    , &pageCleanupNeeded);

            __sync_fetch_and_add(&(Q->expiredMsgs), numExpiredNodes);
            pThreadData->stats.expiredMsgCount += numExpiredNodes;
        }
    }

    if (!reapComplete)
    {
        //do full scan of queue rebuilding expiry data
        bool scanNeedsCleanup = false;

        iemq_fullExpiryScan( pThreadData
                           , Q
                           , nowExpire
                           , &scanNeedsCleanup);

        pageCleanupNeeded |= scanNeedsCleanup;
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

    iemq_releaseHeadLock(Q);


    if (pageCleanupNeeded)
    {
        iemq_scanGetCursor(pThreadData, Q);
        iemq_cleanupHeadPages(pThreadData, Q);
    }

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}
/// @}

/// @{
/// @name Internal Functions

static inline int iemq_initPutLock(iemqQueue_t *Q)
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

static inline int iemq_destroyPutLock(iemqQueue_t *Q)
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

static inline void iemq_getPutLock( iemqQueue_t *Q)
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
        ieutTRACE_FFDC( ieutPROBE_001, true, "Unable to get producer lock.", ISMRC_Error
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }
}

static inline void iemq_releasePutLock( iemqQueue_t *Q)
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
        ieutTRACE_FFDC( ieutPROBE_001, true, "Failed to release producer lock.", ISMRC_Error
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL );
    }
}

static inline void iemq_takeReadHeadLock( iemqQueue_t *Q)
{
    int os_rc = pthread_rwlock_rdlock(&(Q->headlock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "Taking read headlock failed.",ISMRC_Error
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , NULL);
    }
}

static inline void iemq_takeWriteHeadLock( iemqQueue_t *Q)
{
    int os_rc = pthread_rwlock_wrlock(&(Q->headlock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "Taking write headlock failed.",ISMRC_Error
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , NULL);
    }
}

static inline void iemq_releaseHeadLock( iemqQueue_t *Q)
{
    int os_rc = pthread_rwlock_unlock(&(Q->headlock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC(  ieutPROBE_002, true, "Releasing headlock failed.", ISMRC_Error
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , NULL);
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Return pointer to nodePage structure from final Node in array
///  @remarks
///    The iemqQNodePage_t structure contains an array of iemqQNode_t
///    structures. This function accepts a pointer top the last
///    iemqQNode_t entry in the array and returns a pointer to the
///    iemqQNodePage_t structure.
///  @param[in] node               - points to the end of page marker
///  @return                       - pointer to the CURRENT page
///////////////////////////////////////////////////////////////////////////////
static inline iemqQNodePage_t *iemq_getPageFromEnd(iemqQNode_t *node)
{
    iemqQNodePage_t *page;

    //Check we've been given a pointer to the end of the page
    assert(node->msgState == ieqMESSAGE_STATE_END_OF_PAGE);

    page = (iemqQNodePage_t *) node->msg;

    ismEngine_CheckStructId(page->StrucId, IEMQ_PAGE_STRUCID, ieutPROBE_001);

    return page;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Allocate and initialise a new queue page
///  @remarks
///    Allocate (malloc) the storage for a new iemqQNodePage_t structure
///    and initialise it as an 'empty' page. This function does not chain
///    the allocate page onto the queue.
///
/// @param[in] Q                   - Queue handle
/// @return                        - NULL if a failure occurred or a pointer
///                                  to newly created iemqQNodePage_t
///////////////////////////////////////////////////////////////////////////////
static inline iemqQNodePage_t *iemq_createNewPage( ieutThreadData_t *pThreadData
                                                 , iemqQueue_t *Q
                                                 , uint32_t nodesInPage)
{
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    iere_primeThreadCache(pThreadData, resourceSet);
    size_t pageSize = offsetof(iemqQNodePage_t, nodes)
                      + (sizeof(iemqQNode_t) * (nodesInPage + 1));
    iemqQNodePage_t *page = (iemqQNodePage_t *)iere_calloc(pThreadData,
                                                           resourceSet,
                                                           IEMEM_PROBE(iemem_multiConsumerQPage, 1), 1,
                                                           pageSize);

    if (page != NULL)
    {
        //Add the end of page marker to the page
        ismEngine_SetStructId(page->StrucId, IEMQ_PAGE_STRUCID);
        page->nodes[nodesInPage].msgState = ieqMESSAGE_STATE_END_OF_PAGE;
        page->nodes[nodesInPage].msg = (ismEngine_Message_t *) page;
        page->nodesInPage = nodesInPage;

        ieutTRACEL(pThreadData, page, ENGINE_FNC_TRACE,
                   FUNCTION_IDENT "Q %p, size %lu (nodes %u)\n", __func__, Q,
                   pageSize, nodesInPage);
    }
    else
    {
        ieutTRACEL(pThreadData, pageSize, ENGINE_FNC_TRACE,
                   FUNCTION_IDENT "Q %p, size %lu - no mem\n", __func__, Q,
                   pageSize);
    }
    return page;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    return pointer to next iemqQNodePage_t structure in queue
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
static inline iemqQNodePage_t *iemq_moveToNewPage( ieutThreadData_t *pThreadData
                                                 , iemqQueue_t *Q
                                                 , iemqQNode_t *endMsg)
{
    DEBUG_ONLY bool result;
    iemqQNodePage_t *currpage = iemq_getPageFromEnd(endMsg);

    if (currpage->next != NULL)
    {
        return currpage->next;
    }

#if TRACETIMESTAMP_MOVETONEWPAGE
    uint64_t TTS_Start_MoveToNewPage = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_MoveToNewPage);
#endif

    //Hmmm... no new page yet! Someone should be adding it, wait while they do
    while (currpage->next == NULL)
    {
        if (currpage->nextStatus == failed)
        {
            ieutTRACEL(pThreadData, currpage, ENGINE_NORMAL_TRACE,
                       "%s: noticed next page addition to %p has not occurred\n",
                       __func__, currpage);

            // The person who should add the page failed, are we the one to fix it?
            if (__sync_bool_compare_and_swap(&(currpage->nextStatus), failed,
                                             repairing))
            {
                // We have marked the status as repairing - we get to do it
                uint32_t nodesInPage = iemq_choosePageSize(Q);
                iemqQNodePage_t *newpage = iemq_createNewPage(pThreadData, Q,
                                           nodesInPage);
                ieqNextPageStatus_t newStatus;

                if (newpage != NULL)
                {
                    currpage->next = newpage;
                    newStatus = unfinished;

                    ieutTRACEL(pThreadData, newpage, ENGINE_NORMAL_TRACE,
                               "%s: successful new page addition to Q %p currpage %p newPage %p\n",
                               __func__, Q, currpage, newpage);
                }
                else
                {
                    // We couldn't create a page either
                    ieutTRACEL(pThreadData, currpage, ENGINE_WORRYING_TRACE,
                               "%s: failed new page addition to Q %p currpage %p\n",
                               __func__, Q, currpage);
                    newStatus = failed;
                }
                result = __sync_bool_compare_and_swap(&(currpage->nextStatus),
                                                      repairing, newStatus);
                assert(result);

                break; // Leave the loop
            }
        }
        // We need the memory barrier here to ensure we see the change
        // made to 'currpage->next' by an other thread.
        ismEngine_ReadMemoryBarrier();
    }

#if TRACETIMESTAMP_MOVETONEWPAGE
    uint64_t TTS_Stop_MoveToNewPage = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_MoveToNewPage);
#endif
    return currpage->next;
}

//returns NULL if next node hasn't been added yet

static inline iemqQNode_t *iemq_getSubsequentNode( iemqQueue_t *Q
                                                 , iemqQNode_t *currPos)
{
    iemqQNode_t *newPos;

    newPos = currPos + 1;

    if (newPos->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
    {
        iemqQNodePage_t *currpage = (iemqQNodePage_t *) (newPos->msg);

        ismEngine_CheckStructId(currpage->StrucId, IEMQ_PAGE_STRUCID,
                                ieutPROBE_001);

        if (currpage->nextStatus != completed)
        {
            newPos = NULL;
        }
        else
        {
            currpage = currpage->next;
            ismEngine_CheckStructId(currpage->StrucId, IEMQ_PAGE_STRUCID,
                                    ieutPROBE_002);
            newPos = &(currpage->nodes[0]);
        }
    }

    return newPos;
}


//structure only used by iemq_updateMsg*
typedef struct tag_iemq_updateMessageContext_t
{
    //Input fields to the callback
    ismMessageState_t   newState;
    bool increaseMsgUseCount;
    //Output fields from the call
    iemqQNode_t *pnode;
    bool nodeUpdated;
} iemq_updateMessageContext_t;

void iemq_updateMsgIfAvailCallback(void *context)
{
    iemq_updateMessageContext_t *usageContext = (iemq_updateMessageContext_t *)context;

    if (   (usageContext->pnode->msgState == ismMESSAGE_STATE_AVAILABLE)
            && (usageContext->pnode->msg != NULL))
    {
        usageContext->pnode->msgState = usageContext->newState;

        if (usageContext->increaseMsgUseCount)
        {
            iem_recordMessageUsage(usageContext->pnode->msg);
        }
        usageContext->nodeUpdated = true;
    }
}

static inline int32_t iemq_updateMsgIfAvail( ieutThreadData_t *pThreadData
                                           , iemqQueue_t *Q
                                           , iemqQNode_t *node
                                           , bool increaseMsgUseCount
                                           , ismMessageState_t newState)
{
    int32_t rc;
    ielmLockName_t LockName = { .Msg = { ielmLOCK_TYPE_MESSAGE, Q->qId,  node->orderId } };
    iemq_updateMessageContext_t context;

    context.pnode = node;
    context.nodeUpdated = false;
    context.increaseMsgUseCount = increaseMsgUseCount;
    context.newState = newState;

    rc = ielm_instantLockWithCallback( pThreadData
                                     , &LockName
                                     , true
                                     , iemq_updateMsgIfAvailCallback
                                     , &context);

    ieutTRACEL(pThreadData, node->orderId, ENGINE_HIFREQ_FNC_TRACE,
               "UPDMSG: Q %u, OrderId %lu, rc %d nodeUpd %d\n",
               Q->qId, node->orderId, rc, context.nodeUpdated);

    if (rc == ISMRC_LockNotGranted)
    {
        //Someone had it locked... we can't have it
        ieutTRACE_HISTORYBUF(pThreadData, ISMRC_LockNotGranted);
        rc = ISMRC_NoMsgAvail;
    }

    if (rc == OK && !context.nodeUpdated)
    {
        //message wasn't available, the callers needs to know...
        ieutTRACE_HISTORYBUF(pThreadData, ISMRC_NoMsgAvail);
        rc = ISMRC_NoMsgAvail;
    }

    return rc;
}



//If returns ok...message usage count increased...otherwise there is no message locked
static inline int32_t iemq_increaseMessageUsageIfGettable( ieutThreadData_t *pThreadData
                                                         , iemqQueue_t *Q
                                                         , iemqQNode_t *pnode
                                                         , ismEngine_Consumer_t *pConsumer
                                                         , iemqQNode_t **ppNextToTry
                                                         , bool *pSkippedSelectMsg)
{
    int32_t rc = ISMRC_NoMsgAvail;

    *ppNextToTry = iemq_getSubsequentNode(Q, pnode);

    if (*ppNextToTry == NULL)
    {
        //We can only grab a node once the next page has been
        //added so we have somewhere for the get cursor to point to
        goto mod_exit;
    }

    //Have a sneaky look at the message before we try and lock it...
    //We don't really have the right to look at the message, but if someelse
    //makes a message available since our last barrier (getting the read lock if not more recent) they'll
    //call checkWaiters anyway...
    ismMessageState_t msgState = pnode->msgState;
    if ((msgState != ismMESSAGE_STATE_AVAILABLE) || (pnode->msg == NULL))
    {
        //If we've available node with no msg....
        //It marks the end of the queue
        if (msgState == ismMESSAGE_STATE_AVAILABLE)
        {
            *ppNextToTry = NULL;
        }
        goto mod_exit;
    }

    //Try increase the use count of the message whilst it's and lock it in the lock Manager
    rc = iemq_updateMsgIfAvail( pThreadData, Q, pnode, true, ismMESSAGE_STATE_AVAILABLE);


    if (rc == OK)
    {
        ieutTRACEL(pThreadData, pnode->orderId, ENGINE_HIFREQ_FNC_TRACE,
                   "INCREASEDMSGUSAGE: Q %u,  oId %lu  msg %p\n", Q->qId, pnode->orderId, pnode->msg);

        if (pConsumer->selectionRule != NULL)
        {
            //We don't want to do selection with the consumer in getting - other threads will be waiting...
            if (pConsumer->iemqWaiterStatus == IEWS_WAITERSTATUS_GETTING)
            {
                //don't worry if this doesn't work, we'll spot it as we release the waiter
                (void) iews_bool_changeState( &(pConsumer->iemqWaiterStatus)
                                            , IEWS_WAITERSTATUS_GETTING
                                            , IEWS_WAITERSTATUS_DELIVERING);
            }
            ismEngine_Message_t *pMessage = pnode->msg;

            int32_t selResult = ismEngine_serverGlobal.selectionFn(
                                    &(pMessage->Header), pMessage->AreaCount,
                                    pMessage->AreaTypes, pMessage->AreaLengths,
                                    pMessage->pAreaData, NULL,
                                    pConsumer->selectionRule,
                                    (size_t) pConsumer->selectionRuleLen,
                                    NULL);

            ieutTRACEL(pThreadData, selResult, ENGINE_HIFREQ_FNC_TRACE,
                       "Selection function for selection string (%s) (%d:%p) completed with rc=%d. Q %u,  oId %lu\n",
                       (pConsumer->selectionString==NULL)?"[COMPILED]":pConsumer->selectionString,
                       pConsumer->selectionRuleLen, pConsumer->selectionRule,
                       selResult, Q->qId, pnode->orderId);

            if (selResult != SELECT_TRUE)
            {
                // This is not the message you are looking for
                iem_releaseMessage(pThreadData, pnode->msg);

                *pSkippedSelectMsg = true;
                rc = ISMRC_NoMsgAvail;

                pConsumer->failedSelectionCount++;
            }
            else
            {
                pConsumer->successfulSelectionCount++;
            }
        }
    }
    else if (rc == ISMRC_LockNotGranted)
    {
        //That's acceptable...someone else has this message... move on
        rc = ISMRC_NoMsgAvail;
    }
mod_exit:
    return rc;
}


//structure only used by iemq_isPotentialForRedelivery & iemq_isPotentialForRedeliveryCallback
typedef struct tag_iemq_redeliveryContext_t
{
    iemqQNode_t *pnode;
    bool didIncrease;
    uint32_t deliveryId;
} iemq_redeliveryContext_t;

//Determine if the node needs redelivery for someone, we'll check afterwards if it's us
void iemq_isPotentialForRedeliveryCallback(void *context)
{
    iemq_redeliveryContext_t *usageContext = (iemq_redeliveryContext_t *)context;
    iemqQNode_t *pnode=usageContext->pnode;

    if (   (    (pnode->msgState == ismMESSAGE_STATE_DELIVERED)
                || (pnode->msgState == ismMESSAGE_STATE_RECEIVED))
            && (pnode->msg != NULL)
            && (pnode->ackData.pConsumer == NULL)
            && (pnode->deliveryId != 0))
    {
        usageContext->deliveryId = pnode->deliveryId;
        usageContext->didIncrease = true;
    }
}

static inline int32_t iemq_isPotentialForRedelivery( ieutThreadData_t *pThreadData
                                                   , iemqQueue_t *Q
                                                   , iemqQNode_t *node
                                                   , uint32_t *pDeliveryId)
{
    int32_t rc;
    ielmLockName_t LockName = { .Msg = { ielmLOCK_TYPE_MESSAGE, Q->qId,  node->orderId } };
    iemq_redeliveryContext_t context;

    context.pnode = node;
    context.didIncrease = false;
    context.deliveryId = 0;

    rc = ielm_instantLockWithCallback( pThreadData
                                     , &LockName
                                     , false
                                     , iemq_isPotentialForRedeliveryCallback
                                     , &context);

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE,
               "MSGREDELIVSCAN: Q %u, OrderId %lu, rc %d didIncrease %d dId %u \n",
               Q->qId, node->orderId, rc, context.didIncrease, context.deliveryId);

    if (rc == OK)
    {
        if (context.didIncrease)
        {
            *pDeliveryId = context.deliveryId;
        }
        else
        {
            //message wasn't available, the callers needs to know...
            rc = ISMRC_NoMsgAvail;
        }
    }

    return rc;
}
//If returns ok...message is marked as delivered for this consumer...
//It's not locked in the lockManager but other threads will not touch a delivered
//message for this client
static inline int32_t iemq_isSuitableForRedelivery( ieutThreadData_t *pThreadData
                                                  , iemqQueue_t *Q
                                                  , iemqQNode_t *pnode
                                                  , ismEngine_Consumer_t *pConsumer
                                                  , iemqQNode_t **ppNextToTry)
{
    int32_t rc = ISMRC_NoMsgAvail;

    //Consumers that we do redelivery to (MQTT style) don't do selection or browsing
    assert(pConsumer->fDestructiveGet);
    assert(pConsumer->selectionRule == NULL);

    *ppNextToTry = iemq_getSubsequentNode(Q, pnode);

    if (*ppNextToTry == NULL)
    {
        //We can only deliver a node once the next page has been
        //added, we looking to redeliver a node, this node can't have
        //been delivered ever... we're done.
        goto mod_exit;
    }

    //Have a sneaky look at the message before we try and lock it...
    //We don't really have the right to look at the message, but the messages
    //we are looking for aren't new ones just arriving...
    ismMessageState_t msgState = pnode->msgState;
    if ((msgState == ismMESSAGE_STATE_AVAILABLE) && (pnode->msg == NULL))
    {
        //If we've available node with no msg....
        //It marks the end of the queue
        *ppNextToTry = NULL;
        goto mod_exit;
    }

    //If the message is not inflight
    //   or if the message belongs to a consumer already
    //   or if it doesn't have a deliveryid already
    //then it's not for us...
    if (  ((msgState != ismMESSAGE_STATE_DELIVERED) && (msgState != ismMESSAGE_STATE_RECEIVED))
            || (pnode->ackData.pConsumer != NULL)
            || (pnode->deliveryId == 0))
    {
        goto mod_exit;
    }

    bool loopAgain;

    do
    {
        loopAgain = false;
        //Try and momentarily lock the message in the lockmanager and get deliveryid if it looks like a
        //message that needs redelivery to someone, we'll do the potentially more costly check whether it's
        //for *us* in a sec, after the lockManager code.
        uint32_t deliveryId = 0;

        rc = iemq_isPotentialForRedelivery( pThreadData
                                            , Q
                                            , pnode
                                            , &deliveryId);

        if (rc == OK)
        {
            //Is this message for us? (On a single consumer queue, all messages are for us)
            //We don't have the node locked but we have the headlock so we can pass it as an opaque pointer to
            //iecs_* code as with the headlock locked, the pointer has to remain valid even if someone alters fields
            //in it...
            if (    (Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY)
                    || iecs_msgInFlightForClient( pThreadData
                                                  , pConsumer->hMsgDelInfo
                                                  , deliveryId
                                                  , pnode))
            {
                // We've potentially got a message for this waiter
                // Change our state to reflect that. If someone else has
                // disabled/enabled us continue anyway for the moment. We'll
                // notice when we try and enable/disable the waiter after the
                // callback
                ieutTRACEL(pThreadData, pnode->orderId, ENGINE_HIFREQ_FNC_TRACE,
                           "GETREDELIVERY FOUND: Q %u,  oId %lu\n", Q->qId, pnode->orderId);
            }
            else
            {
                //Not a message for this client...
                rc = ISMRC_NoMsgAvail;
            }
        }
        else if (rc == ISMRC_LockNotGranted)
        {
            //Someone else has the message (temporarily) locked.
            //If it's a message for us we need to wait for them to let us go.
            if (    (Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY)
                    || iecs_msgInFlightForClient( pThreadData
                                                  , pConsumer->hMsgDelInfo
                                                  , pnode->deliveryId, pnode))
            {
                loopAgain = true;
            }
            else
            {
                //Don't need this message, move on...
                rc = ISMRC_NoMsgAvail;
            }
        }
    }
    while (loopAgain);

mod_exit:
    return rc;
}

static inline int32_t iemq_tryLockRequest( ieutThreadData_t *pThreadData
                                         , ielmLockScopeHandle_t hLockScope
                                         , iemqQueue_t *Q
                                         , iemqQNode_t *node
                                         , ielmLockRequestHandle_t *phLockRequest)
{
    int32_t rc;
    ielmLockName_t LockName = { .Msg = { ielmLOCK_TYPE_MESSAGE, Q->qId, node->orderId } };

    rc = ielm_requestLock(pThreadData, hLockScope, &LockName, ielmLOCK_MODE_X,
                          ielmLOCK_DURATION_REQUEST, phLockRequest);

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE,
               "TRYLOCKREQ: Q %u, OrderId %lu, rc %d\n", Q->qId, node->orderId,
               rc);
    return rc;
}


static inline bool iemq_updateGetCursor( ieutThreadData_t *pThreadData
                                       , iemqQueue_t *Q
                                       , uint64_t failUpdateIfBefore
                                       , iemqQNode_t *queueCursorPos)
{
    iemqCursor_t curCursorPos;

    //We can't point a getCursor at it if it doesn't have a valid orderId else
    //we struggle to do comparisions...
    assert(queueCursorPos->orderId > 0);

    bool successfulUpdate = false;
    iemqCursor_t newQueueCursor = { .c = { queueCursorPos->orderId,
                                           queueCursorPos
                                         }
                                  };

    do
    {
        curCursorPos = Q->getCursor;

        if (failUpdateIfBefore <= curCursorPos.c.orderId)
        {
            //Doesn't look like anyone has rewound the cursor to before we want to set it to...W00t!

            successfulUpdate = __sync_bool_compare_and_swap(
                                   &(Q->getCursor.whole), curCursorPos.whole,
                                   newQueueCursor.whole);
            if (successfulUpdate)
            {
                ieutTRACEL(pThreadData, newQueueCursor.c.orderId, ENGINE_HIFREQ_FNC_TRACE,
                           "GETCURSOR: Q %u, getCursor updated to oId %lu\n",
                           Q->qId, newQueueCursor.c.orderId);
                ieutTRACE_HISTORYBUF(pThreadData, curCursorPos.c.orderId);
            }
        }
        else
        {
            //D'oh new message(s) have appeared before us... give up the update... we're going to have to try again
            ieutTRACEL(pThreadData, curCursorPos.c.orderId, ENGINE_HIFREQ_FNC_TRACE,
                       "GETCURSOR: Q %u: Update to %lu aborted. getCursor rewound to %lu\n",
                       Q->qId, newQueueCursor.c.orderId, curCursorPos.c.orderId);

            break;
        }
    }
    while (!successfulUpdate);

    //When one we try and update the getCursor, the getting thread no longer
    //needs the scanOrderId set to prevent the page we're scanning being freed
    Q->scanOrderId = IEMQ_ORDERID_PAST_TAIL;

    return successfulUpdate;
}


//If returns ok...message locked...otherwise there is no message locked
static inline int32_t iemq_lockMessageIfGettable( ieutThreadData_t *pThreadData
                                                , iemqQueue_t *Q
                                                , iemqQNode_t *pnode
                                                , ismEngine_Consumer_t *pConsumer
                                                , iemqQNode_t **ppNextToTry
                                                , ielmLockScopeHandle_t hLockScope
                                                , ielmLockRequestHandle_t *phLockRequest
                                                , bool *pSkippedSelectMsg)
{
    int32_t rc = ISMRC_NoMsgAvail;

    *ppNextToTry = iemq_getSubsequentNode(Q, pnode);

    if (*ppNextToTry == NULL)
    {
        //We can only grab a node once the next page has been
        //added so we have somewhere for the get cursor to point to
        goto mod_exit;
    }

    //Have a sneaky look at the message before we try and lock it...
    //We don't really have the right to look at the message, but if someelse
    //makes a message available since our last barrier (getting the read lock if not more recent) they'll
    //call checkWaiters anyway...
    ismMessageState_t msgState = pnode->msgState;
    if ((msgState != ismMESSAGE_STATE_AVAILABLE) || (pnode->msg == NULL))
    {
        //If we've available node with no msg....
        //It marks the end of the queue
        if (msgState == ismMESSAGE_STATE_AVAILABLE)
        {
            *ppNextToTry = NULL;
        }
        goto mod_exit;
    }

    //Try and lock it in the lock Manager
    rc = iemq_tryLockRequest(pThreadData, hLockScope, Q, pnode, phLockRequest);

    if (rc == OK)
    {
        //Now we have the message locked we can validly look at it...
        msgState = pnode->msgState;
        if ((msgState != ismMESSAGE_STATE_AVAILABLE) || (pnode->msg == NULL))
        {
            ielm_releaseLock(pThreadData, hLockScope, *phLockRequest);

            //We could lock+unlock it but it's not a valid message to get
            rc = ISMRC_NoMsgAvail;

            //If we've found an unlocked, available node with no msg....
            //It marks the end of the queue
            if (msgState == ismMESSAGE_STATE_AVAILABLE)
            {
                *ppNextToTry = NULL;
            }
        }
        else
        {
            if (pConsumer->selectionRule != NULL)
            {
                //We don't want to do selection with the consumer in getting - other threads will be waiting...
                if (pConsumer->iemqWaiterStatus == IEWS_WAITERSTATUS_GETTING)
                {
                    //don't worry if this doesn't work, we'll spot it as we release the waiter
                    (void) iews_bool_changeState( &(pConsumer->iemqWaiterStatus)
                                                , IEWS_WAITERSTATUS_GETTING
                                                , IEWS_WAITERSTATUS_DELIVERING);
                }

                ismEngine_Message_t *pMessage = pnode->msg;

                int32_t selResult = ismEngine_serverGlobal.selectionFn(
                                        &(pMessage->Header), pMessage->AreaCount,
                                        pMessage->AreaTypes, pMessage->AreaLengths,
                                        pMessage->pAreaData, NULL,
                                        pConsumer->selectionRule,
                                        (size_t) pConsumer->selectionRuleLen,
                                        NULL);

                ieutTRACEL(pThreadData, selResult, ENGINE_HIFREQ_FNC_TRACE,
                           "Selection function for selection string (%s) (%d:%p) completed with rc=%d. Q %u,  oId %lu\n",
                           (pConsumer->selectionString==NULL)?"[COMPILED]":pConsumer->selectionString,
                           pConsumer->selectionRuleLen, pConsumer->selectionRule,
                           selResult, Q->qId, pnode->orderId);

                if (selResult != SELECT_TRUE)
                {
                    // This is not the message you are looking for
                    ielm_releaseLock(pThreadData, hLockScope, *phLockRequest);

                    *pSkippedSelectMsg = true;
                    rc = ISMRC_NoMsgAvail;

                    pConsumer->failedSelectionCount++;
                }
                else
                {
                    pConsumer->successfulSelectionCount++;
                }
            }
            else
            {
                ieutTRACEL(pThreadData, pnode->orderId, ENGINE_HIFREQ_FNC_TRACE,
                           "GET LOCKED: Q %u,  oId %lu\n", Q->qId, pnode->orderId);
            }
        }
    }
    else if (rc == ISMRC_LockNotGranted)
    {
        //That's acceptable...someone else has this message... move on
        rc = ISMRC_NoMsgAvail;
    }
mod_exit:
    return rc;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Moves the cursor passed consumed nodes
///  @remarks
///    Used after expiry reaper has removed messages so we want to move
///    the cursor forward so early pages can be freed
///
///  @param[in]  Q                 - Queue to move cursor on
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
static inline void iemq_scanGetCursor( ieutThreadData_t *pThreadData
                                     , iemqQueue_t *Q)
{
    int32_t os_rc = 0;
    uint64_t counter = 0;
    iemqCursor_t QCursorPos;

    ieutTRACEL(pThreadData, Q, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY " Q=%p\n", __func__, Q);

    os_rc = pthread_mutex_lock(&(Q->getlock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "Taking getlock failed.", ISMRC_Error
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , NULL);
    }
    //No scan should be in progress so scan orderId should not point to an orderId for a valid node
    assert(Q->scanOrderId == IEMQ_ORDERID_PAST_TAIL);

    // 'node' is the first node where we are going to start
    QCursorPos = Q->getCursor;
    iemqQNode_t *node = QCursorPos.c.pNode;
    iemqQNode_t *subsequentNode = NULL;

    //Note the page we're scanning so it can't get freed out from under us
    //(The setting of this value is protected by the getLock)
    Q->scanOrderId = node->orderId;

    bool successfulCAS = __sync_bool_compare_and_swap(&(Q->getCursor.whole),
                                                      QCursorPos.whole,
                                                      IEMQ_GETCURSOR_SEARCHING.whole);

    if (successfulCAS)
    {
        bool checkNextNode;

        //Move the cursor past expired nodes
        do
        {
            // A little trace to track our progress (every 512 nodes)
            if (((counter++) & 0x1ff) == 0)
            {
                ieutTRACEL(pThreadData, QCursorPos.c.orderId, ENGINE_HIFREQ_FNC_TRACE,
                           "MOVE CURSOR SCAN: Q=%u, qCursor counter=%lu oId=%lu node=%p\n",
                           Q->qId, counter, QCursorPos.c.orderId, node);
            }
            checkNextNode = false;

            if (iemq_isNodeConsumed(pThreadData, Q, node))
            {
                subsequentNode = iemq_getSubsequentNode(Q, node);

                if (subsequentNode)
                {
                    checkNextNode = true;
                }
            }

            if (checkNextNode)
            {
                node = subsequentNode;
                assert(node->orderId != 0);
                Q->scanOrderId = node->orderId;
            }
        }
        while (checkNextNode);
    }
    else
    {
        //Someone else has rewound the get cursor, they'll call
        //checkwaiters... we're out of here
        //Record we're no longer scanning this page so it can be freed
        Q->scanOrderId = IEMQ_ORDERID_PAST_TAIL;
    }

    //Either we've found a node that isn't consumed and we can point the
    //get cursor at the new node, or we need to put the cursor back after the scan
    //(we don't care if the update succeeded, if someone else is
    //rewinding the get cursor, that's fine)
    iemq_updateGetCursor(pThreadData, Q, node->orderId, node);

    os_rc = pthread_mutex_unlock(&(Q->getlock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_002, true, "Releasing getlock failed.", ISMRC_Error
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , NULL);
    }

    ieutTRACEL(pThreadData, node, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "\n",
               __func__);
    return;
}



static inline int32_t iemq_markMessageIfGettable( ieutThreadData_t *pThreadData
                                                , iemqQueue_t *Q
                                                , iemqQNode_t *pnode
                                                , iemqQNode_t **ppNextToTry)
{
    int32_t rc = ISMRC_NoMsgAvail;

    *ppNextToTry = iemq_getSubsequentNode(Q, pnode);

    if (*ppNextToTry == NULL)
    {
        //We can only grab a node once the next page has been
        //added so we have somewhere for the get cursor to point to
        goto mod_exit;
    }

    //Have a sneaky look at the message before we try and lock it...
    //We don't really have the right to look at the message, but if someelse
    //makes a message available since our last barrier (getting the read lock if not more recent) they'll
    //call checkWaiters anyway...
    assert(NULL != pnode);
    ismMessageState_t msgState = pnode->msgState;
    if ((msgState != ismMESSAGE_STATE_AVAILABLE) || (pnode->msg == NULL))
    {
        //If we've available node with no msg....
        //It marks the end of the queue
        if (msgState == ismMESSAGE_STATE_AVAILABLE)
        {
            *ppNextToTry = NULL;
        }
        goto mod_exit;
    }

    //We set it to delivered here even if we want to mark it consumed shortly so the
    //page can't be thrown away until we've read the message out
    rc = iemq_updateMsgIfAvail( pThreadData, Q, pnode, false, ismMESSAGE_STATE_DELIVERED);

mod_exit:
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Returns a pointer to the iemqQNode_t containing the next msg to be
///    delivered to a destructive getter. The message is marked delivered
///    (whilst locked in the lock manager).
///  @remarks
///    This function returns a pointer to the iemqQNode_t structure containing
///    the next message to be delivered (according to the get cursor) and updates
///    the cursor to point at the next message.
///
///  @param[in]  Q                 - Queue to get the message from
///  @param[in]  pConsumer         - Receiver for the message
///  @param[out] ppnode            - On successful completion, a pointer to
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
static inline int32_t iemq_locateMessageForGetter( ieutThreadData_t *pThreadData
                                                 , iemqQueue_t *Q
                                                 , ismEngine_Consumer_t *pConsumer
                                                 , iemqQNode_t **ppnode)
{
    int32_t rc = OK;
    int32_t os_rc = 0;
    uint64_t counter = 0;
    iemqCursor_t QCursorPos;

    ieutTRACEL(pThreadData, pConsumer, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY " Q=%p Consumer=%p\n", __func__, Q, pConsumer);

    os_rc = pthread_mutex_lock(&(Q->getlock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "Taking getlock failed.", ISMRC_Error
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , NULL);
    }
    //No scan should be in progress so scan orderId should not point to an orderId for a valid node
    assert(Q->scanOrderId == IEMQ_ORDERID_PAST_TAIL);

    // 'node' is the first node where we are going to look for messages
    QCursorPos = Q->getCursor;
    iemqQNode_t *node = QCursorPos.c.pNode;
    iemqQNode_t *subsequentNode;

    //We set it to delivered here even if we want to mark it consumed shortly so the
    //page can't be thrown away until we've read the message out
    rc = iemq_markMessageIfGettable( pThreadData, Q, node, &subsequentNode);

    if ((rc == ISMRC_NoMsgAvail) && (subsequentNode != NULL))
    {
        //Note the page we're scanning so it can't get freed out from under us
        //(The setting of this value is protected by the getLock)
        Q->scanOrderId = node->orderId;

        //Hmm... can't get that one but there's a subsequent node (so we're
        //not at the tail) we'll 'clear' the getcursor and scan forward looking
        //for a message. Note that we must (re)scan the node we've just scanned
        //after clearing the get cursor in case it has suddenly come available
        bool successfulCAS = __sync_bool_compare_and_swap(&(Q->getCursor.whole),
                             QCursorPos.whole, IEMQ_GETCURSOR_SEARCHING.whole);

        if (successfulCAS)
        {
            bool checkNextNode;

            //Look for a node we can get
            do
            {
                // A little trace to track our progress (every 512 nodes)
                if (((counter++) & 0x1ff) == 0)
                {
                    ieutTRACEL(pThreadData, QCursorPos.c.orderId, ENGINE_HIFREQ_FNC_TRACE,
                               "GET SCAN: Q=%u, qCursor counter=%lu oId=%lu node=%p\n",
                               Q->qId, counter, QCursorPos.c.orderId, node);
                }

                rc = iemq_markMessageIfGettable( pThreadData, Q, node, &subsequentNode);

                checkNextNode = ((rc == ISMRC_NoMsgAvail)
                                 && (subsequentNode != NULL));
                if (checkNextNode)
                {
                    assert(subsequentNode->orderId != 0);
                    node = subsequentNode;
                    Q->scanOrderId = node->orderId;
                }
            }
            while (checkNextNode);
        }
        else
        {
            //Someone else has rewound the get cursor, they'll call
            //checkwaiters... we're out of here
            //Record we're no longer scanning this page so it can be freed
            Q->scanOrderId = IEMQ_ORDERID_PAST_TAIL;
        }
    }

    if (rc == OK)
    {
        //We've locked a node... try and update the Q-cursor
        //Try and set the get cursor on the queue back to the last node
        //we sarched where we did not skip due to selection. We fail
        //this update if the get cursor has been rewound to previous to
        //the message we are about to return.
        assert(node != NULL);
        if (iemq_updateGetCursor(pThreadData, Q, node->orderId,
                                 subsequentNode))
        {
            // We succesfully updates the get cursor, which means we can
            // return the node to the caller.
            *ppnode = node;
        }
        else
        {
            //Some evil thread has put a message before the one we were
            //going to get and the get cursor has been rewound.
            //It conceivably could have been put in the same transaction as
            //the node we found so we need to let this one go but
            //as we are in DELIVERING state we must ensure that we loop
            //around again and attempt to deliver messages.
            node->msgState = ismMESSAGE_STATE_AVAILABLE;

            rc = IMSRC_RecheckQueue;

            ieutTRACEL(pThreadData, node->orderId, ENGINE_UNUSUAL_TRACE,
                          "Q %u Returning message (cursor rewound): %lu\n",
                                                       Q->qId, node->orderId);
        }
    }
    else if (Q->scanOrderId != IEMQ_ORDERID_PAST_TAIL)
    {
        //We failed to find a node...try and record how far we scanned in the
        //get cursor. If we skipped messages due to selection then reset
        //get cursor back to the original get cursor position.
        (void) iemq_updateGetCursor(pThreadData, Q, node->orderId, node);

        //If the update failed, someone has put an earlier message on the
        //queue...but they'll call checkWaiters so we can just leave them
        // to it.
    }

    os_rc = pthread_mutex_unlock(&(Q->getlock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_002, true, "Releasing getlock failed.", rc
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , NULL);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n",
               __func__, rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Returns a pointer to the iemqQNode_t containing the next msg to be
///    delivered to a destructive getter using selection
///  @remarks
///    This function returns a pointer to the iemqQNode_t structure containing
///    the next message to be delivered (according to the get cursor) and updates
///    the cursor to point at the next message.
///
///  @param[in]  Q                 - Queue to get the message from
///  @param[in]  pConsumer         - Receiver for the message
///  @param[out] ppnode            - On successful completion, a pointer to
///  @param[in]  hLockScope        - scope to lock the message under
///  @param[out] phLockRequest     - Handle to unlock the message
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
static inline int32_t iemq_locateMessageForSelector(
                             ieutThreadData_t *pThreadData
                           , iemqQueue_t *Q
                           , ismEngine_Consumer_t *pConsumer
                           , iemqQNode_t **ppnode)
{
    int32_t rc = ISMRC_NoMsgAvail;
    int32_t os_rc = 0;
    bool SkippedSelectMsg = false;
    bool SearchedGetCursor = false;
    iemqCursor_t consumerCursor;
    iemqCursor_t initialConsumerCursor;
    iemqCursor_t QCursorPos;
    iemqQNode_t *QCursorNode;
    ielmLockRequestHandle_t hLockRequest;
    bool done;

    ieutTRACEL(pThreadData, pConsumer, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY " Q=%p Consumer=%p\n", __func__, Q, pConsumer);

    os_rc = pthread_mutex_lock(&(Q->getlock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "Taking getlock failed.", ISMRC_Error
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , NULL);
    }
    //No scan should be in progress so scan orderId should not point to an orderId for a valid node
    assert(Q->scanOrderId == IEMQ_ORDERID_PAST_TAIL);

    // First check if we can use the consumer's own cursor.
    iemqQNode_t *subsequentNode = NULL;

    // We need a read barrier after reading the check values to ensure
    // they are read before we read the actualy cursor values.
    ismEngine_ReadMemoryBarrier();

    QCursorPos = Q->getCursor;
    QCursorNode = QCursorPos.c.pNode;
    consumerCursor = pConsumer->iemqCursor;
    initialConsumerCursor = consumerCursor;

    // We would prefer to use the cursor value in the consumer to start
    // our search for messages, however if an out-of order put has been
    // committed before the Q Cursor then we must rewind back to the
    // actual queue cursor.
    // Note. initialCursor is left at the original consumer cursor value
    // as we will compare and swap the consumer cursor value after we
    // have serached for a message
    if (consumerCursor.c.orderId < QCursorPos.c.orderId)
    {
        consumerCursor = QCursorPos;
    }

    // 'node' is the first node where we are ging to look for messages
    iemqQNode_t *node = consumerCursor.c.pNode;

    // If the getCursor and the consumer cursor are the same value
    // then avoid setting the getCursor value to SEARCHING.
    if (consumerCursor.c.orderId == QCursorPos.c.orderId)
    {
        SearchedGetCursor = true;
        rc = iemq_lockMessageIfGettable(pThreadData, Q, node, pConsumer,
                                        &subsequentNode, Q->selectionLockScope,
                                        &hLockRequest, &SkippedSelectMsg);
    }

    if ((rc == ISMRC_NoMsgAvail)
            && ((SearchedGetCursor == false) || (subsequentNode != NULL)))
    {
        // If we are looking for messages starting at the Q-Cursor position
        // then we have not skipped any messages because they do not match
        // out selector. (We could have skipped messages which have been
        // locked by another consumer).
        SkippedSelectMsg =
            (consumerCursor.c.orderId > QCursorPos.c.orderId) ?
            true : false;

        ieutTRACEL(pThreadData, consumerCursor.c.orderId, ENGINE_HIFREQ_FNC_TRACE,
                   "GET SCAN: Q %u, consumerCursor oId %lu (qCursor oId %lu)\n",
                   Q->qId, consumerCursor.c.orderId, QCursorPos.c.orderId);

        //Note the page we're scanning so it can't get freed out from under us
        //(The setting of this value is protected by the getLock)
        Q->scanOrderId = node->orderId;

        //Hmm... can't get that one but there's a subsequent node (so we're
        //not at the tail) we'll 'clear' the getcursor and scan forward looking
        //for a message. Note that we must (re)scan the node we've just scanned
        //after clearing the get cursor in case it has suddenly come available
        bool successfulCAS = __sync_bool_compare_and_swap(&(Q->getCursor.whole),
                             QCursorPos.whole, IEMQ_GETCURSOR_SEARCHING.whole);

        if (successfulCAS)
        {
            bool checkNextNode;

            //Look for a node we can get
            do
            {
                rc = iemq_lockMessageIfGettable(pThreadData, Q, node, pConsumer,
                                                &subsequentNode, Q->selectionLockScope,
                                                &hLockRequest, &SkippedSelectMsg);

                // If there is a subsequent node, then set the consumer
                // cursor to that node.
                if (subsequentNode)
                {
                    if (!SkippedSelectMsg)
                    {
                        // Save the position the queue cursor can be updated to
                        QCursorNode = subsequentNode;
                    }

                    // Save the next consumerCursor position
                    consumerCursor.c.pNode = subsequentNode;
                    consumerCursor.c.orderId = subsequentNode->orderId;
                }

                checkNextNode = ((rc == ISMRC_NoMsgAvail)
                                 && (subsequentNode != NULL));
                if (checkNextNode)
                {
                    // If we are moving on to the next node to search for a
                    // message then we must update in the consumer cursor before
                    // we move onto the next node. This ensures if an earlier
                    // message is unlocked that our consumer cursor is correctly
                    // rewound.
                    done = __sync_bool_compare_and_swap(
                               &(pConsumer->iemqCursor.whole),
                               initialConsumerCursor.whole, consumerCursor.whole);
                    if (!done)
                    {
                        // Some evil thread has unlocked a message prior to
                        // our currently position and rewound our get cursor
                        // Just pick-up the new consumer cursor value and
                        // resume our search.
                        consumerCursor = pConsumer->iemqCursor;
                    }
                    initialConsumerCursor = consumerCursor;

                    node = subsequentNode;
                    assert(node->orderId != 0);
                    Q->scanOrderId = node->orderId;
                }
            }
            while (checkNextNode);
        }
        else
        {
            rc = ISMRC_NoMsgAvail;
            //Someone else has rewound the get cursor, they'll call
            //checkwaiters... we're out of here
            //Record we're no longer scanning this page so it can be freed
            Q->scanOrderId = IEMQ_ORDERID_PAST_TAIL;
        }
    }
    else if (rc == OK)
    {
        consumerCursor.c.pNode = subsequentNode;
        assert(NULL != subsequentNode);
        consumerCursor.c.orderId = subsequentNode->orderId;

        if (!SkippedSelectMsg)
        {
            // Save the position the queue cursor can be updated to
            QCursorNode = subsequentNode;
        }
    }

    // Update the point we will start selecting the next time we search for
    // a message for this consumer
    done = __sync_bool_compare_and_swap(&(pConsumer->iemqCursor.whole),
                                        initialConsumerCursor.whole, consumerCursor.whole);
    if (!done)
    {
        //Some evil thread has put a message before the one we were
        //going to get and the get cursor has been rewound.
        //It conceivably could have been put in the same transaction as
        //the node we found so we need to let this one go but
        //as we are in DELIVERING state we must ensure that we loop
        //around again and attempt to deliver messages.
        if (rc == OK)
        {
            ielm_releaseLock(pThreadData, Q->selectionLockScope, hLockRequest);
        }
        rc = IMSRC_RecheckQueue;
    }

    if (rc == OK)
    {
        //If we have set the get cursor to IEMQ_GETCURSOR_SEARCHING then we want to fail the update if
        //the place we are moving it to is after a rewind. Otherwise, we will be returning the 1st node
        //under the get cursor, so we want to fail the update if it's before this node.
        assert(node != NULL);
        uint64_t failUpdateIfBefore = node->orderId < QCursorNode->orderId ? node->orderId : QCursorNode->orderId;

        //We've locked a node... try and update the Q-cursor
        //Try and set the get cursor on the queue back to the last node
        //we sarched where we did not skip due to selection. We fail
        //this update if the get cursor has been rewound to previous to
        //the message we are about to return.
        if (iemq_updateGetCursor(pThreadData, Q, failUpdateIfBefore, QCursorNode))
        {
            // We succesfully updates the get cursor, which means we can
            // return the node to the caller.
            *ppnode = node;
        }
        else
        {
            //Some evil thread has put a message before the one we were
            //going to get and the get cursor has been rewound.
            //It conceivably could have been put in the same transaction as
            //the node we found so we need to let this one go but
            //as we are in DELIVERING state we must ensure that we loop
            //around again and attempt to deliver messages.
            ielm_releaseLock(pThreadData, Q->selectionLockScope, hLockRequest);
            rc = IMSRC_RecheckQueue;
        }
    }
    else if (Q->scanOrderId != IEMQ_ORDERID_PAST_TAIL)
    {
        //We failed to find a node...try and record how far we scanned in the
        //get cursor. If we skipped messages due to selection then reset
        //get cursor back to the original get cursor position.
        (void) iemq_updateGetCursor(pThreadData, Q, QCursorNode->orderId,
                                    QCursorNode);

        //If the update failed, someone has put an earlier message on the
        //queue...but they'll call checkWaiters so we can just leave them
        // to it.
    }

    os_rc = pthread_mutex_unlock(&(Q->getlock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC(ieutPROBE_002, true, "Releasing getlock failed.", rc
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , NULL);
    }

    if (rc == OK)
    {
        //Mark the node as claimed, once we've done that we can release our hold on it in the lock manager
        node->msgState = ismMESSAGE_STATE_DELIVERED;
        ielm_releaseLock(pThreadData, Q->selectionLockScope, hLockRequest);
    }
    if (rc == ISMRC_NoMsgAvail)
    {
        if (SkippedSelectMsg)
        {
            rc = ISMRC_NoMsgAvailForConsumer;
        }
    }

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n",
               __func__, rc);
    return rc;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Prepares to browse a given message
///  @remarks
///    This function is called prior to delivering a message to the consumer for browsing
///    It sets up a message header. Caller needs to ensure pnode doesn't disappear out
///    from under us during this call
///
///  @param[in] Q                  - Queue to get the message from
///  @param[in] pnode              - A pointer to the node containing the msg
///  @param[out] phmsg             - Message handle
///  @param[out] pmsgHdr           - Message header
///  @return                       - FDCs+core are produce if message can't be delivered
///////////////////////////////////////////////////////////////////////////////
static void iemq_prepareMessageForBrowse( ieutThreadData_t *pThreadData
                                        , iemqQueue_t *Q
                                        , iemqQNode_t *pnode
                                        , ismEngine_Message_t **phmsg
                                        , ismMessageHeader_t *pmsgHdr )
{
    ieutTRACEL(pThreadData, pnode->msg, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY " pnode=%p pMessage=%p Reliability=%d\n", __func__,
               pnode, pnode->msg, pnode->msg->Header.Reliability);

    // The locate function increased the use count of the message. The callback will be
    // responsible for performing the decrement.... we don't need to alter it in this prepare...

    // Update the message header with the delivery count in the message.
    ismEngine_CheckStructId(pnode->msg->StrucId, ismENGINE_MESSAGE_STRUCID,
                            ieutPROBE_001);

    // Copy the message header from the one published
    *pmsgHdr = pnode->msg->Header;

    // Add the delivery count from the node
    pmsgHdr->RedeliveryCount += pnode->deliveryCount;

    // Ensure the delivery count does not exceedd the maximum
    if (pmsgHdr->RedeliveryCount > ieqENGINE_MAX_REDELIVERY_COUNT)
    {
        pmsgHdr->RedeliveryCount = ieqENGINE_MAX_REDELIVERY_COUNT;
    }

    // Include any additional flags
    pmsgHdr->Flags |= pnode->msgFlags;

    // And set the order ID on the message
    pmsgHdr->OrderId =
        (pmsgHdr->Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT) ?
        pnode->orderId : 0;

    // Return the message to be browsed...
    *phmsg = pnode->msg;

    ieutTRACEL(pThreadData, pnode->msg, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "\n",
               __func__);
    return;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Returns a pointer to the iemqQNode_t containing the next msg to be
///    delivered to a destructive getter
///  @remarks
///    This function returns a pointer to the iemqQNode_t structure containing
///    the next message to be delivered (according to the get cursor) and updates
///    the cursor to point at the next message.
///
///  @param[in]  Q                 - Queue to get the message from
///  @param[in]  pConsumer         - Receiver for the messageW
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
static inline int32_t  iemq_locateAndPrepareMessageForBrowse(
                                         ieutThreadData_t *pThreadData
                                       , iemqQueue_t *Q
                                       , ismEngine_Consumer_t *pConsumer
                                       , ismEngine_Message_t **ppMessage
                                       , ismMessageHeader_t    *pmsgHdr)
{
    int32_t rc = OK;
    bool SkippedSelectMsg = false;

    ieutTRACEL(pThreadData, pConsumer, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY " Q=%p, pConsumer=%p\n", __func__, Q, pConsumer);

    iemq_takeReadHeadLock(Q);

    iemqQNode_t *node;

    iemqQNode_t *headNode = &(Q->headPage->nodes[0]);

    if (pConsumer->iemqCursor.c.orderId < headNode->orderId)
    {
        node = headNode;
    }
    else
    {
        node = pConsumer->iemqCursor.c.pNode;
    }
    iemqQNode_t *subsequentNode = NULL;

    ieutTRACEL(pThreadData, node->orderId, ENGINE_HIFREQ_FNC_TRACE,
               "BROWSE SCAN: Q %u, browseCursor oId %lu\n", Q->qId, node->orderId);

    //Look for a node we can get
    bool checkNextNode;
    do
    {
        rc = iemq_increaseMessageUsageIfGettable(pThreadData, Q, node, pConsumer,
                &subsequentNode,  &SkippedSelectMsg);

        checkNextNode = ((rc == ISMRC_NoMsgAvail) && (subsequentNode != NULL));

        if (checkNextNode)
        {
            node = subsequentNode;
            assert(node->orderId != 0);
        }
    }
    while (checkNextNode);

    if (rc == OK)
    {
        iemq_prepareMessageForBrowse( pThreadData
                                    , Q
                                    , node
                                    , ppMessage
                                    , pmsgHdr );

        assert(subsequentNode != NULL && subsequentNode->orderId != 0);

        pConsumer->iemqCursor.c.pNode = subsequentNode;
        pConsumer->iemqCursor.c.orderId = subsequentNode->orderId;
    }
    else
    {
        pConsumer->iemqCursor.c.pNode = node;
        pConsumer->iemqCursor.c.orderId = node->orderId;
    }

    iemq_releaseHeadLock(Q);

    if (rc == ISMRC_NoMsgAvail)
    {
        if (SkippedSelectMsg)
        {
            rc = ISMRC_NoMsgAvailForConsumer;
        }
    }

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n",
               __func__, rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Returns a pointer to the iemqQNode_t containing the next msg to be
///    delivered to consumer who is having messages redelivered
///  @remarks
///    This function returns a pointer to the iemqQNode_t structure containing
///    the next message to be redelivered
///
///  @param[in]  Q                 - Queue to get the message from
///  @param[in]  pConsumer         - Receiver for the message
///  @param[out] ppnode            - On successful completion, a pointer to
///  @param[in]  hLockScope        - scope to lock the message under
///  @param[out] phLockRequest     - Handle to unlock the message
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
static inline int32_t iemq_locateMessageForRedelivery(
                             ieutThreadData_t *pThreadData
                           , iemqQueue_t *Q
                           , ismEngine_Consumer_t *pConsumer, iemqQNode_t **ppnode)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pConsumer, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY " Q=%p, pConsumer=%p\n", __func__, Q, pConsumer);

    iemq_takeReadHeadLock(Q);

    iemqCursor_t QHeadPos;

    QHeadPos.c.pNode = &(Q->headPage->nodes[0]);
    QHeadPos.c.orderId = QHeadPos.c.pNode->orderId;

    iemqQNode_t *node;

    if (pConsumer->iemqCursor.c.orderId < QHeadPos.c.orderId)
    {
        node = QHeadPos.c.pNode;
    }
    else
    {
        node = pConsumer->iemqCursor.c.pNode;
    }
    iemqQNode_t *subsequentNode = NULL;

    ieutTRACEL(pThreadData, node->orderId, ENGINE_HIFREQ_FNC_TRACE,
               "REDELIVERY SCAN: Q %u, cursor oId %lu\n", Q->qId, node->orderId);

    //Look for a node we can get
    bool checkNextNode;
    do
    {
        rc = iemq_isSuitableForRedelivery( pThreadData
                                           , Q
                                           , node
                                           , pConsumer
                                           , &subsequentNode);

        checkNextNode = ((rc == ISMRC_NoMsgAvail) && (subsequentNode != NULL));

        if (checkNextNode)
        {
            node = subsequentNode;
            assert(node->orderId != 0);
        }
    }
    while (checkNextNode);

    if (rc == OK)
    {
        *ppnode = node;
        assert(subsequentNode != NULL && subsequentNode->orderId != 0);

        pConsumer->iemqCursor.c.pNode = subsequentNode;
        pConsumer->iemqCursor.c.orderId = subsequentNode->orderId;
    }
    else
    {
        pConsumer->iemqCursor.c.pNode = node;
        pConsumer->iemqCursor.c.orderId = node->orderId;
    }

    iemq_releaseHeadLock(Q);

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n",
               __func__, rc);
    return rc;
}

static int32_t iemq_assignAndRecordDeliveryId( ieutThreadData_t *pThreadData
                                             , iemqQueue_t *Q
                                             , ismEngine_Consumer_t *pConsumer
                                             , iemqQNode_t *pnode
                                             , ismMessageState_t newMsgState
                                             , uint8_t deliveryCount
                                             , uint64_t *pStoreOps)
{
    int32_t rc = OK;

    if (!pnode->inStore)
    {
        if (pnode->deliveryId == 0)
        {
            if (Q->hStoreObj != ismSTORE_NULL_HANDLE)
            {
                rc = iecs_assignDeliveryId(pThreadData,
                                           pConsumer->hMsgDelInfo,
                                           pConsumer->pSession, Q->hStoreObj, (ismQHandle_t) Q,
                                           pnode,
                                           false, &pnode->deliveryId);
            }
            else
            {
                rc = iecs_assignDeliveryId(pThreadData,
                                           pConsumer->hMsgDelInfo,
                                           pConsumer->pSession, (ismStore_Handle_t) Q, // The owner cannot be a Store handle
                                           (ismQHandle_t) Q, pnode,
                                           true, &pnode->deliveryId);
            }
        }
    }
    else
    {
        iest_AssertStoreCommitAllowed(pThreadData);

        if (!pnode->hasMDR)
        {
            //If this function is successful, it can leave an uncommitted
            //store transaction for us to commit (if it fails it may have called rollback)
            rc = iecs_storeMessageDeliveryReference(pThreadData,
                                                    pConsumer->hMsgDelInfo, pConsumer->pSession, Q->hStoreObj,
                                                    (ismQHandle_t) Q, pnode, pnode->hMsgRef, &pnode->deliveryId,
                                                    &pnode->hasMDR);

            if (rc != OK)
            {
                goto mod_exit;
            }
        }

        iemq_updateMsgRefInStore(pThreadData, Q, pnode,
                                 newMsgState, pConsumer->fConsumeQos2OnDisconnect, deliveryCount,
                                 false);

        (*pStoreOps) ++; //It doesn't matter that iecs_storeMessageDeliveryReference() might have added ops
                         //all we care about is whether != 0
    }

mod_exit:
    return rc;
}


//Should be called with a consumer locked
//After returning, completeWaiterActions *must* be call
static void iemq_handleDeliveryFailure( ieutThreadData_t *pThreadData
                                      , iemqQueue_t *q
                                      , ismEngine_Consumer_t *pConsumer)
{
    ieutTRACEL(pThreadData, pConsumer, ENGINE_WORRYING_TRACE, FUNCTION_IDENT "pCons=%p \n",
               __func__, pConsumer);

    //Notify the delivery failure callback /before/ unlocking the consumer so
    //that we know that the consumer will be valid...
    if (ismEngine_serverGlobal.deliveryFailureFn != NULL)
    {
        ismEngine_serverGlobal.deliveryFailureFn( ISMRC_AllocateError
                                                , pConsumer->pSession->pClient
                                                , pConsumer
                                                , pConsumer->pMsgCallbackContext );
    }
    else
    {
        //We can't ask the protocol to disconnect as they haven't registered the callback... we're going to
        //come down in a ball of flames....
        ieutTRACE_FFDC(ieutPROBE_001, true,
              "Out of memory during message delivery and no delivery failure handler was registered.", ISMRC_AllocateError
            , "Queue", q, sizeof(iemqQueue_t)
            , "Internal Name", q->InternalName, sizeof(q->InternalName)
            , "consumer", pConsumer, sizeof(ismEngine_Consumer_t)
            , NULL);
    }

    iews_addPendFlagWhileLocked( &(pConsumer->iemqWaiterStatus)
                               , IEWS_WAITERSTATUS_DISABLE_PEND);
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Find someone we can give a message to.
///  @remarks
///    Normally we need to find a consumer we can grab so we can deliver as
///  many messages to as many consumers as possible on as many different threads as
///  we can.
///  If it is taking a while to  then the bare minim we need is we need just want to
///  know that someone has started a fresh get since the operation that cause us to
///  checkWaiters completed.... having a waiter in delivering
///  is a promise that they will try and get again... so if we can prove a waiter
///  is in delivering then we can give up
///

///////////////////////////////////////////////////////////////////////////////

static int32_t iemq_lockWillingWaiter( ieutThreadData_t *pThreadData
                                     , iemqQueue_t *q
                                     , iewsWaiterStatus_t lockedState
                                     , ismEngine_Consumer_t **ppLockedConsumer)
{
    bool loopAgain;
    bool worthChecking;
    int32_t rc = ISMRC_NoAvailWaiter;
    ismEngine_Consumer_t *pConsumer = NULL;
    uint64_t numLoops=0;
    bool redeliverOnly = false;

#if TRACETIMESTAMP_LOCKWAITER
    uint64_t TTS_Start_LockWaiter = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_LockWaiter);
#endif

    if (!iemq_checkAndSetFullDeliveryPrevention(pThreadData, q))
    {
        //Too many inflight messages to deliver more... just redeliver any already in flight
        redeliverOnly = true;
    }

    do
    {
        loopAgain = false;
        numLoops++;
        uint32_t numGetting=0;
        uint32_t numDelivering=0;

        int32_t os_rc = pthread_rwlock_rdlock(&(q->waiterListLock));

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC( ieutPROBE_001, true, "called with invalid message state (multiConsumer) failed.",
                                 ISMRC_Error
                          , "os_rc", &os_rc, sizeof(os_rc)
                          , "Queue", q, sizeof(iemqQueue_t)
                          , "Internal Name", q->InternalName, sizeof(q->InternalName)
                          , NULL);
        }

        //Look through the list of waiters trying to find one that is enabled that
        //we can grab and set to getting

        ismEngine_Consumer_t *pFirstConsumer = q->firstWaiter;

        if (pFirstConsumer != NULL)
        {
            pConsumer = pFirstConsumer;

            //Loop until we have got back to the first consumer....
            do
            {
                if ((!pConsumer->fDestructiveGet)
                        || (pConsumer->selectionRule != NULL))
                {
                    worthChecking =
                        (pConsumer->iemqNoMsgCheckVal < q->checkWaitersVal) ?
                        true : false;
                }
                else
                {
                    worthChecking = !pConsumer->fFlowControl;
                }

                //Don't deliver new messages if we are only redelivering old ones
                if (redeliverOnly && !(pConsumer->fRedelivering))
                {
                    worthChecking = false;
                }

                if (worthChecking)
                {
                    iewsWaiterStatus_t localStatus = pConsumer->iemqWaiterStatus;

                    if (localStatus == IEWS_WAITERSTATUS_ENABLED)
                    {
                        iewsWaiterStatus_t oldStatus =
                            __sync_val_compare_and_swap(
                                &(pConsumer->iemqWaiterStatus),
                                IEWS_WAITERSTATUS_ENABLED, lockedState);

                        if (oldStatus == IEWS_WAITERSTATUS_ENABLED)
                        {
                            //Woot!, we bagged ourselves a consumer
                            *ppLockedConsumer = pConsumer;
                            rc = OK;
                            break;
                        }
                        else if (oldStatus == IEWS_WAITERSTATUS_GETTING)
                        {
                            ieutTRACEL(pThreadData, pConsumer, ENGINE_HIFREQ_FNC_TRACE,
                                       "Didn't lock getting waiter(%p)\n",
                                       pConsumer);

                            //The get for that waiter might fail...and ours starting slightly later might succeed
                            //...so we should loop and try and give him a message if we can't find an actual enabled waiter...
                            numGetting++;
                        }
                        else if (oldStatus == IEWS_WAITERSTATUS_DELIVERING)
                        {
                            //We've looked through everyone and couldn't find a waiter to use and
                            //as this guy was in delivering... he'll definitely do another get...
                            //so give up
                            numDelivering++;
                        }
                        else
                        {
                            ieutTRACEL(pThreadData, oldStatus, ENGINE_HIFREQ_FNC_TRACE,
                                       "Didn't lock non-enabled waiter(%p) (Status=%lu)\n",
                                       pConsumer, oldStatus);
                        }
                    }
                    else if (localStatus == IEWS_WAITERSTATUS_GETTING)
                    {
                        ieutTRACEL(pThreadData, pConsumer, ENGINE_HIFREQ_FNC_TRACE,
                                   "Didn't try to lock getting waiter(%p)\n",
                                   pConsumer);

                        //The get for that waiter might fail...and ours starting slightly later might succeed
                        //...so we should loop and try and give him a message if we can't find an actual enabled waiter...
                        numGetting++;
                    }
                    else if (localStatus == IEWS_WAITERSTATUS_DELIVERING)
                    {
                        //We've looked through everyone and couldn't find a waiter to use and
                        //as this guy was in delivering... he'll definitely do another get...
                        //so give up
                        numDelivering++;
                    }
                }
                pConsumer = pConsumer->iemqPNext;
            }
            while (pConsumer != pFirstConsumer);

            if (rc == OK)
            {
                //set the "firstWaiter" so future waiter searches start in a different place
                // We only have a read lock on the list... but it is still safe to update the firstWaiter pointer... as no-one can come or
                //go whilst we have the readlock. We don't check another thread hasn't moved it etc.. (extra barriers)... it doesn't /really/ matter which consumer is
                //considered the first one as long as it moves around.
#if 0
                q->firstWaiter = q->firstWaiter->iemqPNext;
#else
                //We need to introduce a (low overhead as possible) degree of random stutter into walking
                //the circle so we don't get into consistent patterns where some waiters are given no messages
                /* Uses Read TSC instruction on x86 (number processor cycles since power-on)*/
                uint32_t i[2];
                ism_engine_fastTime(i[0], i[1]);

                //Have about a 1 in 5 chance of skipping a person be the first waiter
                if ((i[0] & 255) < 50)
                {
                    q->firstWaiter = q->firstWaiter->iemqPNext;
                }
                else
                {
                    q->firstWaiter = q->firstWaiter->iemqPNext->iemqPNext;
                }
#endif
            }
        }

        pthread_rwlock_unlock(&(q->waiterListLock));

        //If this is a waiter who uses transactions....make sure we have enough memory to add a get to a transaction. If not we won't give them a message.
        if ((rc == OK) && (pConsumer->pSession->fIsTransactional)
                && (pConsumer->iemqCachedSLEHdr == NULL))
        {
            pConsumer->iemqCachedSLEHdr = iemq_reserveSLEMemForConsume(pThreadData);

            if (pConsumer->iemqCachedSLEHdr == NULL)
            {
                ieutTRACEL(pThreadData, pConsumer, ENGINE_WORRYING_TRACE,
                           "Failed to allocate memory for delivery... disconnecting a consumer\n");
                //disable the consumer and notify the protocol to ask for it to be disconnected
                iemq_handleDeliveryFailure(pThreadData, q, pConsumer);

                ieq_completeWaiterActions( pThreadData
                                         , (ismEngine_Queue_t *)q
                                         , pConsumer
                                         , IEQ_COMPLETEWAITERACTION_OPTS_NONE
                                         , true);

                //Try and find another consumer...
                rc = ISMRC_NoAvailWaiter;
                loopAgain = true;
            }
        }
        
        //If there were getting waiters...their get might fail because it started too early
        //but the message that has caused us to try delivery will get trapped unless we are sure another
        //get *started* after we arrived in checkWaiters. A thread in delivering promises to call
        //get again so we can give up if there are delivering threads
        if (numGetting > 0 && numDelivering == 0)
        {
            ieutTRACEL(pThreadData, numLoops, ENGINE_FNC_TRACE,
                "Looping because of getting but no delivering threads\n");
            loopAgain = true;
        }
    }
    while (loopAgain && (rc == ISMRC_NoAvailWaiter));

#if TRACETIMESTAMP_LOCKWAITER
    uint64_t TTS_Stop_LockWaiter = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_LockWaiter);
#endif

    return rc;
}


static ismMessageState_t iemq_getMessageStateWhenDelivered( ismEngine_Consumer_t  *pConsumer
                                                          , iemqQNode_t *pnode)
{
    // Update the status of the message, to indicate that it has
    // been delivered...do we expect an ack for it?
    if (   (pnode->msg->Header.Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
        || (pConsumer->fAcking == false))
    {
        return ismMESSAGE_STATE_CONSUMED;
    }
    else if (pnode->msgState == ismMESSAGE_STATE_RECEIVED)
    {
        return ismMESSAGE_STATE_RECEIVED;
    }
    else
    {
        return ismMESSAGE_STATE_DELIVERED;
    }
}

static inline int32_t iemq_initialPrepareMessageForDelivery( ieutThreadData_t      *pThreadData
                                                           , iemqQueue_t           *Q
                                                           , ismEngine_Consumer_t  *pConsumer
                                                           , iemqQNode_t           *pnode
                                                           , uint64_t              *pStoreOpCount
                                                           , bool                  *pContinueBatch)
{
    int32_t rc = OK;

    //The only updates we do in the initial prepare delivery phase are for getters not browsers....
    if (pConsumer->fDestructiveGet)
    {
        ismMessageState_t newMsgState = iemq_getMessageStateWhenDelivered(pConsumer, pnode);

        if (   (newMsgState == ismMESSAGE_STATE_DELIVERED)
            || (newMsgState == ismMESSAGE_STATE_RECEIVED))
        {
            //If this consumer wants a 16bit deliveryid unique in client scope (a la MQTT) assign it now
            if (pConsumer->fShortDeliveryIds)
            {
                //updates store incl. delivery count and msgstate if appropriate
                rc = iemq_assignAndRecordDeliveryId(pThreadData,
                                                    Q, pConsumer,
                                                    pnode,
                                                    newMsgState, (pnode->deliveryCount+1),
                                                    pStoreOpCount);

                if (rc != OK)
                {
                    goto mod_exit;
                }
            }
            //If we're not using a delivery id, we still need to update the store if applicable
            else if (pnode->inStore)
            {
                if (pConsumer->fNonPersistentDlvyCount == false)
                {
                    iemq_updateMsgRefInStore(pThreadData, Q, pnode,
                                             newMsgState,
                                             pConsumer->fConsumeQos2OnDisconnect,
                                             (pnode->deliveryCount+1),
                                             false);
                    (*pStoreOpCount)++;
                }
            }

            //If we are delivering the message transactionally, give the node enough memory
            //to be able to add the consuming of it to a transaction....
            if (pConsumer->pSession->fIsTransactional)
            {
                assert(pConsumer->iemqCachedSLEHdr != NULL);
                assert(pnode->iemqCachedSLEHdr == NULL);
                pnode->iemqCachedSLEHdr = pConsumer->iemqCachedSLEHdr;

                //try and allocate memory for a get for another SLE, if we fail it's not a big
                //issue - we'll end the delivery batch and we'll retry the allocate when we grab the waiternext time...
                pConsumer->iemqCachedSLEHdr = iemq_reserveSLEMemForConsume(pThreadData);

                if (pConsumer->iemqCachedSLEHdr == NULL)
                {
                    ieutTRACEL(pThreadData, pConsumer, ENGINE_WORRYING_TRACE,
                               "Failed to allocate memory for delivery...ending delivery batch\n");
                    *pContinueBatch = false;
                }
            }
        }
        else if (newMsgState == ismMESSAGE_STATE_CONSUMED)
        {
            if (pnode->inStore)
            {
                iest_AssertStoreCommitAllowed(pThreadData);

                rc = ism_store_deleteReference(pThreadData->hStream,
                                               Q->QueueRefContext, pnode->hMsgRef, pnode->orderId, 0);
                if (UNLIKELY(rc != OK))
                {
                    // The failure to delete a store reference means that the
                    // queue is inconsistent.
                    ieutTRACE_FFDC( ieutPROBE_001, true, "ism_store_updateReference (multiConsumer) failed.", rc
                                  , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                                  , "Queue", Q, sizeof(iemqQueue_t)
                                  , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                                  , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                                  , "pNode", pnode, sizeof(iemqQNode_t)
                                  , NULL);
                }
                (*pStoreOpCount)++;

                //Unstore of the message done in iemq_completePreparingMessage();
            }
        }
    }

mod_exit:
    return rc;
}

static inline void iemq_completePreparingMessage(
                                          ieutThreadData_t      *pThreadData
                                        , iemqQueue_t           *Q
                                        , ismEngine_Consumer_t *pConsumer
                                        , iemqQNode_t           *pnode
                                        , ismMessageState_t      oldMsgState
                                        , ismMessageState_t      newMsgState
                                        , ismEngine_Message_t  **phmsg
                                        , ismMessageHeader_t    *pmsgHdr
                                        , uint32_t              *pdeliveryId
                                        , iemqQNode_t          **pnodeDelivery)
{
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, pnode->msg, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_IDENT " pnode=%p pMessage=%p Reliability=%d msgState=%d\n",
               __func__, pnode, pnode->msg, pnode->msg->Header.Reliability,
               newMsgState);

    // Update the message header with the delivery count in the message.
    // We can't update the RedeliveryCount in the message header after we
    // have delivered the message as the consumer may well actually send
    // the message over the wire asynchronously.
    ismEngine_CheckStructId(pnode->msg->StrucId, ismENGINE_MESSAGE_STRUCID,
                            ieutPROBE_004);

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

    if (oldMsgState == ismMESSAGE_STATE_AVAILABLE && pmsgHdr->Expiry != 0)
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

        if (oldMsgState == ismMESSAGE_STATE_AVAILABLE)
        {
             __sync_fetch_and_add(&(Q->inflightDeqs), 1);
        }

        pnode->msgState = newMsgState;

        if (Q->ackListsUpdating)
        {
            ieal_addUnackedMessage(pThreadData, pConsumer, pnode);
        }

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

        if ((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0)
        {
            size_t messageBytes = IEMQ_MESSAGE_BYTES(pnode->msg);
            pThreadData->stats.remoteServerBufferedMsgBytes -= messageBytes;
            (void)__sync_fetch_and_sub((&Q->bufferedMsgBytes), messageBytes);
        }

        __sync_fetch_and_add(&(Q->dequeueCount), 1);

        bool needCleanup = iemq_needCleanAfterConsume(pnode);

        pnode->msg = MESSAGE_STATUS_EMPTY;
        
        // Having set this state to CONSUMED we cannot access the node
        // again. Ensure we have a write barrier to ensure all the
        // previous memory accesses are completed.
        if (   (oldMsgState == ismMESSAGE_STATE_DELIVERED)
            || (oldMsgState == ismMESSAGE_STATE_RECEIVED))
        {
            __sync_fetch_and_sub(&(Q->inflightDeqs), 1);
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
        if (needCleanup)
        {
            iemq_cleanupHeadPages(pThreadData, Q);
        }

        *pnodeDelivery = NULL;
        *pdeliveryId = 0;
    }
}
///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Returns a pointer to the iemqQNode_t containing the next msg to be
///    delivered.
///  @remarks
///    This function returns a pointer to the iemqQNode_t structure containing
///    the next message to be delivered (according to the cursor) and updates
///    the cursor to point at the next message.
///
///  @param[in]  Q                 - Queue to get the message from
///  @param[in]  pConsumer         - Receiver for the message
///  @param[out] ppnode            - On successful completion, a pointer to
///  @param[in]  hLockScope        - scope to lock the message under
///  @param[out] phLockRequest     - Handle to unlock the message
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
static inline int32_t iemq_locateMessage(
    ieutThreadData_t *pThreadData,
    iemqQueue_t *Q,
    ismEngine_Consumer_t *pConsumer,
    iemqQNode_t **ppNode,
    ismMessageState_t *pOrigMsgState)
{
    int32_t rc = OK;
    iemqQNode_t *pnode = NULL;

    bool loopAgain;

    //Browsers can't use this function as we don't mark nodes as delivered
    //for a browse, so by the time this function returned a node to a browser, the
    //node could be freed
    assert(pConsumer->fDestructiveGet);

    do
    {
        loopAgain = false;

        if (pConsumer->fRedelivering)
        {
            rc = iemq_locateMessageForRedelivery( pThreadData
                                                , Q
                                                , pConsumer
                                                , &pnode);

            if (rc == OK)
            {
                *ppNode        = pnode;
                *pOrigMsgState = pnode->msgState;
            }
            else if (rc == ISMRC_NoMsgAvail)
            {
                //We've completed redelivery.
                ieutTRACEL(pThreadData, pConsumer, ENGINE_FNC_TRACE,
                           FUNCTION_IDENT "Redelivery complete for cons %p\n",
                           __func__, pConsumer);

                pConsumer->fRedelivering = false;
                pConsumer->iemqCursor.whole = 0;

                //Look for an undelivered message to send if we are giving out new msgs
                rc = IMSRC_RecheckQueue;
            }
        }
        else
        {
            if (pConsumer->selectionRule != NULL)
            {
                rc = iemq_locateMessageForSelector(pThreadData, Q, pConsumer, &pnode);
            }
            else
            {
                rc = iemq_locateMessageForGetter(pThreadData, Q, pConsumer, &pnode);
            }
            if (rc == OK)
            {
                *ppNode        = pnode;
                *pOrigMsgState = ismMESSAGE_STATE_AVAILABLE;
            }
        }
    }
    while ((rc == OK) && loopAgain);

    return rc;
}

static inline void iemq_abortDelivery(ieutThreadData_t *pThreadData,
                                      iemqQueue_t *Q,
                                      ismEngine_Consumer_t *pConsumer,
                                      iemqQNode_t *pnode)
{
    //Browsers are catered for elsewhere
    assert(pConsumer->fDestructiveGet);

    if (pConsumer->fRedelivering)
    {
        //Prepare failed...make this message looked at again next time we deliver to this consumer.
        //As the message was (and still is) marked as inflight to us, and we have the consumer
        //locked no other thread should touch the message at this point
        ieutTRACE_HISTORYBUF(pThreadData, pConsumer);
        ieutTRACEL(pThreadData, pnode->orderId, ENGINE_PERFDIAG_TRACE,
                      "Q %u Returning delivered message (delivery cancelled) : %lu - %s\n",
                                                   Q->qId, pnode->orderId, pConsumer->pSession->pClient->pClientId);

        pConsumer->iemqCursor.c.pNode = pnode;
        pConsumer->iemqCursor.c.orderId = pnode->orderId;
    }
    else
    {
        //Prepare failed... put message back into queue and don't deliver
        //Take headlock so the message page can't be freed whilst we are rewinding to it
        iemq_takeReadHeadLock(Q);
        pnode->msgState = ismMESSAGE_STATE_AVAILABLE;

        ieutTRACEL(pThreadData, pnode->orderId, ENGINE_PERFDIAG_TRACE,
                      "Q %u Returning message (delivery cancelled) : %lu -%s\n",
                                 Q->qId, pnode->orderId, pConsumer->pSession->pClient->pClientId);

        //Need a memory barrier in between making the message available and starting to look at
        //get cursor
        ismEngine_FullMemoryBarrier();

        iemqCursor_t msgCursor;

        msgCursor.c.orderId = pnode->orderId;
        msgCursor.c.pNode = pnode;

        iemq_rewindGetCursor(pThreadData, Q, msgCursor);

        iemq_releaseHeadLock(Q);
    }
}


static void iemq_startUndoInitialPrepareMessage( ieutThreadData_t *pThreadData
                                               , iemqQueue_t *Q
                                               , ismEngine_Consumer_t *pConsumer
                                               , bool prepareCommitted
                                               , uint32_t nodecountToUndo
                                               , iemqAsyncMessageDeliveryInfoPerNode_t *perNodeInfo
                                               , uint32_t *pStoreOps)
{
    ieutTRACEL(pThreadData, nodecountToUndo, ENGINE_PERFDIAG_TRACE, FUNCTION_IDENT "queue=%p - nodecount %u - %s\n",
                          __func__, Q, nodecountToUndo, pConsumer->pSession->pClient->pClientId);

    assert(nodecountToUndo > 0);

#ifdef HAINVESTIGATIONSTATS
    pThreadData->stats.messagesDeliveryCancelled += nodecountToUndo;
#endif

    if (!prepareCommitted)
    {
        iest_store_rollback(pThreadData, false);
    }

    for (uint64_t i=0; i < nodecountToUndo; i++)
    {
        iemqQNode_t *pnode = perNodeInfo[i].node;

        ismMessageState_t newMsgState = iemq_getMessageStateWhenDelivered(pConsumer, pnode);

        if ( (newMsgState == ismMESSAGE_STATE_DELIVERED) ||
             (newMsgState == ismMESSAGE_STATE_RECEIVED) )
        {
            if (prepareCommitted && pnode->inStore)
            {
                if (pConsumer->fNonPersistentDlvyCount == false)
                {
                    //Undo delivery count & any change of state
                    iemq_updateMsgRefInStore(pThreadData, Q, pnode,
                                             perNodeInfo[i].origMsgState,
                                             pConsumer->fConsumeQos2OnDisconnect,
                                             pnode->deliveryCount,
                                             false);

                    *pStoreOps += 1;
                }
                //If we're making the node available for other people, we need to remove our id
                if (pnode->hasMDR && perNodeInfo[i].origMsgState == ismMESSAGE_STATE_AVAILABLE)
                {
                    iemq_startReleaseDeliveryId( pThreadData
                                               , pConsumer->hMsgDelInfo
                                               , Q
                                               , pnode
                                               , pStoreOps);
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
                    iest_unstoreMessage( pThreadData, pnode->msg, false, false, NULL, pStoreOps);
                    pnode->inStore = false;
                    pnode->hMsgRef = 0;
                }
            }
            else
            {
                //Nothing to do - we'll do it in complete
            }
        }
    }
}

static void iemq_completeUndoInitialPrepareMessage( ieutThreadData_t *pThreadData
                                                  , iemqQueue_t *Q
                                                  , ismEngine_Consumer_t *pConsumer
                                                  , uint32_t nodecountToUndo
                                                  , iemqAsyncMessageDeliveryInfoPerNode_t *perNodeInfo)
{
    ieutTRACEL(pThreadData, nodecountToUndo, ENGINE_UNUSUAL_TRACE, FUNCTION_IDENT "queue=%p - nodecount %u\n",
                          __func__, Q, nodecountToUndo);

    assert(nodecountToUndo > 0);

    //Stop nodes we are about to make available being freed before we rewind the
    //cursor to them...
    iemq_takeReadHeadLock(Q);

    //Due to cursor rewinding, the first node in the array may not be the earliest...
    iemqCursor_t earliestMsg = { .c = {IEMQ_ORDERID_PAST_TAIL, NULL} };


    for (uint64_t i=0; i < nodecountToUndo; i++)
    {
        iemqQNode_t *pnode = perNodeInfo[i].node;

        //Even if we not expecting an ack and the node will be consumed upon delivery
        //it should still be in a delivery state at the moment so its reserved for us
        assert (   (pnode->msgState == ismMESSAGE_STATE_DELIVERED)
                || (pnode->msgState == ismMESSAGE_STATE_RECEIVED) );

        iemq_releaseReservedSLEMem(pThreadData,  pnode);

        if (pnode->orderId < earliestMsg.c.orderId)
        {
            earliestMsg.c.orderId = pnode->orderId;
            earliestMsg.c.pNode   = pnode;
        }

        //Remove the deliveryId/MDR for node if we'll make it available for others
        if (pnode->hasMDR && perNodeInfo[i].origMsgState == ismMESSAGE_STATE_AVAILABLE)
        {
            bool triggerRedelivery = false;
            iemq_finishReleaseDeliveryId(pThreadData, pConsumer->hMsgDelInfo, Q, pnode, &triggerRedelivery);

            if (triggerRedelivery)
            {
                ismEngine_internal_RestartSession(pThreadData, pConsumer->pSession, true );
            }
        }
        pnode->msgState =  perNodeInfo[i].origMsgState;
    }

    //We need to make sure the updates to the msgStates are completed before
    //we start looking at the cursors to rewind
    ismEngine_FullMemoryBarrier();

    //Rewind get cursor and per-consumer cursors
    iemq_rewindGetCursor(pThreadData, Q, earliestMsg);

    //Rewind cursor for *this* consumer (covers browse & redelivery)
    if (pConsumer->iemqCursor.c.orderId > earliestMsg.c.orderId)
    {
        ieutTRACE_HISTORYBUF(pThreadData, pConsumer);
        ieutTRACE_HISTORYBUF(pThreadData, pConsumer->iemqCursor.c.orderId);
        ieutTRACE_HISTORYBUF(pThreadData, earliestMsg.c.orderId);

        pConsumer->iemqCursor.c.orderId = earliestMsg.c.orderId;
        pConsumer->iemqCursor.c.pNode   = earliestMsg.c.pNode;
    }

    iemq_releaseHeadLock(Q);
}

static int32_t iemq_asyncCancelDelivery(ieutThreadData_t           *pThreadData,
                                        int32_t                              rc,
                                        ismEngine_AsyncData_t        *asyncInfo,
                                        ismEngine_AsyncDataEntry_t     *context)
{
    assert(rc == OK);
    assert(context->Type == iemqQueueCancelDeliver);
    iemqAsyncMessageDeliveryInfo_t *deliveryInfo = (iemqAsyncMessageDeliveryInfo_t *)(context->Data);
    ismEngine_CheckStructId(deliveryInfo->StructId, IEMQ_ASYNCMESSAGEDELIVERYINFO_CANCEL_STRUCID, ieutPROBE_001);

    ismEngine_Consumer_t *pConsumer = deliveryInfo->pConsumer;
    iemqQueue_t *Q                  = deliveryInfo->Q;
    uint32_t firstCancelledNode     = deliveryInfo->firstCancelledNode;

    iemq_completeUndoInitialPrepareMessage( pThreadData
                                          , Q
                                          , pConsumer
                                          , deliveryInfo->usedNodes - firstCancelledNode
                                          , &(deliveryInfo->perNodeInfo[firstCancelledNode]));

    //Pop this entry off the stack now incase complete goes async...
    iead_popAsyncCallback( asyncInfo, context->DataLen);

    //We put the waiter into disable pending we *know* we need to completeWaiter actions
    ieq_completeWaiterActions( pThreadData
                             , (ismEngine_Queue_t *)Q
                             , pConsumer
                             , IEQ_COMPLETEWAITERACTION_OPTS_NONE
                             , true);

    return rc;
}

/////////////////////////////////////////////////////////////////////////////////////
/// @brief
///     If checkWaiters (or sub functions) went async without being given an asyncInfo
///     structure, this function tidies up
//////////////////////////////////////////////////////////////////////////////////////
static int32_t iemq_completeCheckWaiters(ieutThreadData_t           *pThreadData,
                                         int32_t                              rc,
                                         ismEngine_AsyncData_t        *asyncInfo,
                                         ismEngine_AsyncDataEntry_t     *context)
{
    assert(context->Type == iemqQueueCompleteCheckWaiters);
    ismQHandle_t Qhdl = (ismQHandle_t)(context->Handle);

    ieutTRACEL(pThreadData, Qhdl, ENGINE_FNC_TRACE, FUNCTION_IDENT "Qhdl=%p\n", __func__,
                                                                  Qhdl);

    //Pop this of the list so we don't double decrement the predelete count
    iead_popAsyncCallback( asyncInfo, 0);
    //Undo the count we did just before the commit
    iemq_reducePreDeleteCount(pThreadData, Qhdl);

    return rc;
}


static void iemq_initialiseStackAsyncInfo(  iemqQueue_t *Q
                                         ,  ismEngine_AsyncData_t *stackAsyncInfo
                                         ,  ismEngine_AsyncDataEntry_t localstackAsyncArray[])
{
    ismEngine_SetStructId(localstackAsyncArray[0].StrucId, ismENGINE_ASYNCDATAENTRY_STRUCID);
    localstackAsyncArray[0].Data = NULL;
    localstackAsyncArray[0].DataLen = 0;
    localstackAsyncArray[0].Handle = Q;
    localstackAsyncArray[0].Type = iemqQueueCompleteCheckWaiters;
    localstackAsyncArray[0].pCallbackFn.internalFn = iemq_completeCheckWaiters;

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


static int32_t iemq_undoInitialPrepareMessage( ieutThreadData_t *pThreadData
                                             , iemqQueue_t *Q
                                             , bool prepareCommitted
                                             , iemqAsyncMessageDeliveryInfo_t *pDeliveryData
                                             , ismEngine_AsyncData_t *asyncInfo)
{
    uint32_t storeOps = 0;
    uint32_t firstCancelledNode = pDeliveryData->firstCancelledNode;
    int32_t rc = OK;

    //We only delivered some of the batch, we need to allow the remainder to
    //be delivered again...
    iemq_startUndoInitialPrepareMessage( pThreadData
                                       , Q
                                       , pDeliveryData->pConsumer
                                       , prepareCommitted
                                       , pDeliveryData->usedNodes - firstCancelledNode
                                       , &(pDeliveryData->perNodeInfo[firstCancelledNode])
                                       , &storeOps);

    if (storeOps > 0)
    {
        //We want to async whether caller of checkWaiters wants to or not. If not, we just don't tell them.
        //As checkWaiters is something of a "black-box" to our caller, that shouldn't be a problem,
        //However having a put call checkWaiters with an asyncInfo means that we can hold up the
        //put for a while on a deep queue, giving some level of pacing
        ismEngine_AsyncDataEntry_t localstackAsyncArray[IEAD_MAXARRAYENTRIES];
        ismEngine_AsyncData_t localStackAsyncInfo;
        bool usedlocalStackAsyncInfo = false;

        if (asyncInfo == NULL)
        {
            ieutTRACE_HISTORYBUF(pThreadData, &localStackAsyncInfo);
            usedlocalStackAsyncInfo = true;

            iemq_initialiseStackAsyncInfo(Q, &localStackAsyncInfo, localstackAsyncArray);

            asyncInfo = &localStackAsyncInfo;

            //We're not going to worry our caller with the info that we're going async, so
            //we need to ensure the queue doesn't get deleted out from under us
            DEBUG_ONLY uint64_t oldCount = __sync_fetch_and_add(&(Q->preDeleteCount), 1);
            assert(oldCount > 0);
        }

        //Argh, we need to commit before we can finish resetting the nodes
        ismEngine_SetStructId(pDeliveryData->StructId, IEMQ_ASYNCMESSAGEDELIVERYINFO_CANCEL_STRUCID);

        //Don't copy unused nodes...
        size_t dataSize =   offsetof(iemqAsyncMessageDeliveryInfo_t,perNodeInfo)
                           + (sizeof(iemqAsyncMessageDeliveryInfoPerNode_t) * pDeliveryData->usedNodes);
        assert(dataSize > (    ((void *)&(pDeliveryData->perNodeInfo[pDeliveryData->usedNodes-1].origMsgState))
                                     - ((void *)pDeliveryData)));
        assert(dataSize <= sizeof(*pDeliveryData));
        
        ismEngine_AsyncDataEntry_t newEntry =
                       { ismENGINE_ASYNCDATAENTRY_STRUCID
                       , iemqQueueCancelDeliver
                       , pDeliveryData, dataSize
                       , NULL
                       , {.internalFn = iemq_asyncCancelDelivery}};

        iead_pushAsyncCallback(pThreadData, asyncInfo, &newEntry);

        rc = iead_store_asyncCommit(pThreadData, false, asyncInfo);

        if (rc == ISMRC_AsyncCompletion)
        {
            //Gone async... get out of here
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
                iemq_reducePreDeleteCount(pThreadData, (ismEngine_Queue_t *)Q);
            }

            if ( rc != OK)
            {
                ieutTRACE_FFDC( ieutPROBE_003, true, "iead_store_commit failed.", rc
                              , NULL);

                goto mod_exit;
            }
        }
    }

    iemq_completeUndoInitialPrepareMessage( pThreadData
                                          , Q
                                          , pDeliveryData->pConsumer
                                          , pDeliveryData->usedNodes - firstCancelledNode
                                          , &(pDeliveryData->perNodeInfo[firstCancelledNode]));
mod_exit:
    return rc;
}


//rc is ok or ISMRC_AsyncCompletion
static uint32_t iemq_deliverMessages( ieutThreadData_t *pThreadData
                                    , iemqAsyncMessageDeliveryInfo_t *pDeliveryData
                                    , ismEngine_AsyncData_t *asyncInfo
                                    , ismEngine_DelivererContext_t * delivererContext )
{
    ieutTRACE_HISTORYBUF(pThreadData, pDeliveryData->pConsumer);
    ieutTRACEL(pThreadData, pDeliveryData->usedNodes, ENGINE_HIFREQ_FNC_TRACE,
            FUNCTION_ENTRY " Consumer %p usedNodes: %u firstNode %u\n",
               __func__, pDeliveryData->pConsumer, pDeliveryData->usedNodes, pDeliveryData->firstDeliverNode);

    //Does the user explicitly call suspend or do we disable if the return code is false
    iemqQueue_t *Q = pDeliveryData->Q;
    int32_t rc = OK;
    bool loopAgain; //We normally only go once through the outer loop

    assert(pDeliveryData->usedNodes > 0); //NB: We check there *were* messages to deliver, but we might
                                          //have delivered them and just need to release the waiter and
                                          //call checkwaiters...

    if (! (pDeliveryData->consumerLocked))
    {
        //We can't deliver them any messages...
        goto mod_exit;
    }

    ismEngine_Consumer_t *pConsumer = pDeliveryData->pConsumer;
    bool fExplicitSuspends = pConsumer->pSession->fExplicitSuspends;

    bool wantsMoreMessages = true;

    do
    {
        loopAgain = false;

        uint32_t msgsDelivered = pDeliveryData->firstDeliverNode; //We may starting part way through batch

        bool completeWaiterActions = false;
        bool needStoreCommit       = false;

        while (wantsMoreMessages &&  !needStoreCommit && (msgsDelivered <  pDeliveryData->usedNodes))
        {
            iemqQNode_t *pnode =  pDeliveryData->perNodeInfo[msgsDelivered].node;
            iemqQNode_t *pnodeDelivery = NULL; //If the node is still valid during delivery, this will point to it
            ismMessageHeader_t    msgHdr;
            ismEngine_Message_t  *hmsg;
            uint32_t              deliveryId;

            ismMessageState_t  newState = iemq_getMessageStateWhenDelivered(pConsumer, pnode);

            iemq_completePreparingMessage( pThreadData
                                         , Q
                                         , pConsumer
                                         , pnode
                                         , pDeliveryData->perNodeInfo[msgsDelivered].origMsgState
                                         , newState
                                         , &hmsg
                                         , &msgHdr
                                         , &deliveryId
                                         , &pnodeDelivery);

            //waiter should be in delivering (or other threads can have added flags)
            assert(
                  ((pConsumer->iemqWaiterStatus) & (IEWS_WAITERSTATUSMASK_LOCKED & (~(IEWS_WAITERSTATUS_GETTING|IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT)))) != 0);

            wantsMoreMessages = ism_engine_deliverMessage(
                                          pThreadData,
                                          pConsumer,
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
                    iews_addPendFlagWhileLocked( &(pConsumer->iemqWaiterStatus)
                                               , IEWS_WAITERSTATUS_DISABLE_PEND);
                }
                //We need to call delivery callback etc...
                completeWaiterActions = true;
            }

            msgsDelivered++;

            if (pThreadData->numLazyMsgs == ieutMAXLAZYMSGS)
            {
                needStoreCommit = true;
            }
        }

        if (wantsMoreMessages && (msgsDelivered == pDeliveryData->usedNodes))
        {

#ifdef HAINVESTIGATIONSTATS
                clock_gettime(CLOCK_REALTIME, &(pConsumer->lastDeliverEnd));
                ism_engine_recordConsumerStats(pThreadData, pConsumer);
#endif
            //We don't have any more to give it....put it back into enabled
            //and ask for more to sent along...
            iewsWaiterStatus_t oldState = iews_val_tryUnlockNoPending(
                                              &(pConsumer->iemqWaiterStatus)
                                            , IEWS_WAITERSTATUS_DELIVERING
                                            , IEWS_WAITERSTATUS_ENABLED);

            if (oldState != IEWS_WAITERSTATUS_DELIVERING)
            {
                //Other people have added actions for us to do...
                completeWaiterActions = true;
            }
            else
            {
                pDeliveryData->consumerLocked = false;
            }
        }


        if(completeWaiterActions)
        {
            pDeliveryData->consumerLocked = false;

            if(needStoreCommit)
            {
                //We need a store commit before the undo, it ought to be async
                //...but it is a corner case
                iest_store_commit(pThreadData, false);
                needStoreCommit = false;
            }
#ifdef HAINVESTIGATIONSTATS
                clock_gettime(CLOCK_REALTIME, &(pConsumer->lastDeliverEnd));
                ism_engine_recordConsumerStats(pThreadData, pConsumer);
#endif

            if (msgsDelivered != pDeliveryData->usedNodes)
            {
                //We've got msgs to deliver but can't deliver them as the waiter is
                //being disabled
                pDeliveryData->firstCancelledNode = msgsDelivered;

                rc = iemq_undoInitialPrepareMessage( pThreadData
                        , Q
                        , true //We've committed the delivery
                        , pDeliveryData
                        , asyncInfo);

                assert (rc == OK || rc == ISMRC_AsyncCompletion);
            }
            //If rc is async, the callback will complete the waiteractions

            if (rc == OK)
            {
                ieq_completeWaiterActions( pThreadData
                        , (ismEngine_Queue_t *)Q
                        , pConsumer
                        , IEQ_COMPLETEWAITERACTION_OPTS_NONE
                        , true);
            }
        }
        else
        {
            if (needStoreCommit)
            {
                //Even if we have no messages left to deliver we call ourselves
                //again/loop around so we can release the waiter and call checkwaiters...
                pDeliveryData->firstDeliverNode = msgsDelivered;

                //We want to async whether caller of checkWaiters wants to or not. If not, we just don't tell them.
                //As checkWaiters is something of a "black-box" to its caller, that shouldn't be a problem,
                //However having a put call checkWaiters with an asyncInfo means that we can hold up the
                //put for a while on a deep queue, giving some level of pacing
                ismEngine_AsyncDataEntry_t localstackAsyncArray[IEAD_MAXARRAYENTRIES];
                ismEngine_AsyncData_t localStackAsyncInfo;
                bool usedlocalStackAsyncInfo = false;

                if (asyncInfo == NULL)
                {
                    ieutTRACE_HISTORYBUF(pThreadData, &localStackAsyncInfo);
                    usedlocalStackAsyncInfo = true;
                }
#ifdef HAINVESTIGATIONSTATS
               pThreadData->stats.asyncMsgBatchStartDeliver++;
#endif

                if (    pThreadData->jobQueue != NULL
                     && pThreadData->threadType != AsyncCallbackThreadType
                     && ismEngine_serverGlobal.ThreadJobAlgorithm == ismENGINE_THREAD_JOB_QUEUES_ALGORITHM_EXTRA)
                {
                    //We want to try and keep the delivery off the AsyncCallBack thread as much as possible so ask the call
                    //back to send further checkWaiters to this thread.
                    ieutTRACE_HISTORYBUF(pThreadData, &localStackAsyncInfo);
                    usedlocalStackAsyncInfo = true; //Whether or not we used it before, use the local one now //TODO: Should we force local stack in this case or use callers?

                    assert(pDeliveryData->pJobThread == NULL);
                    ieut_acquireThreadDataReference(pThreadData);
                    pDeliveryData->pJobThread = pThreadData;
                }
#ifdef HAINVESTIGATIONSTATS
                else
                {
                    ieutTRACE_HISTORYBUF(pThreadData, pThreadData->stats.asyncMsgBatchStartDeliver);
                }
#endif
                if (usedlocalStackAsyncInfo)
                {
                    iemq_initialiseStackAsyncInfo(Q, &localStackAsyncInfo, localstackAsyncArray);

                    asyncInfo = &localStackAsyncInfo;

                    //We're not going to worry our caller with the info that we're going async, so
                    //we need to ensure the queue doesn't get deleted out from under us
                    DEBUG_ONLY uint64_t oldCount = __sync_fetch_and_add(&(Q->preDeleteCount), 1);
                    assert(oldCount > 0);
                }

                //don't copy unused nodes in batch
                size_t dataSize =   offsetof(iemqAsyncMessageDeliveryInfo_t,perNodeInfo)
                                  + (sizeof(iemqAsyncMessageDeliveryInfoPerNode_t) * pDeliveryData->usedNodes);

                assert(dataSize > (    ((void *)&(pDeliveryData->perNodeInfo[pDeliveryData->usedNodes-1].origMsgState))
                                     - ((void *)pDeliveryData)));
                assert(dataSize <= sizeof(*pDeliveryData));

                ismEngine_AsyncDataEntry_t newEntry =
                                           { ismENGINE_ASYNCDATAENTRY_STRUCID
                                           , iemqQueueDeliver
                                           , pDeliveryData, dataSize
                                           , NULL
                                           , {.internalFn = iemq_asyncMessageDelivery}};

                iead_pushAsyncCallback(pThreadData, asyncInfo, &newEntry);

                rc = iead_store_asyncCommit(pThreadData, false, asyncInfo);
                assert (rc == OK || rc == ISMRC_AsyncCompletion);

                if (rc == ISMRC_AsyncCompletion)
                {
                    //Gone async... get out of here
                    if (usedlocalStackAsyncInfo)
                    {
                        //We went async but nothing on the async stack will notify caller
                        //so as far as they are concerned, we are done.
                        rc = ISMRC_OK;
                    }
                    goto mod_exit;
                }
                else
                {
                    //We didn't go async, we don't need the delivery callback to be on the asyncinfo stack
                    iead_popAsyncCallback(asyncInfo, newEntry.DataLen);

                    if(pDeliveryData->pJobThread)
                    {
                        ieut_releaseThreadDataReference(pDeliveryData->pJobThread);
                        pDeliveryData->pJobThread = NULL;
                    }

                    if (usedlocalStackAsyncInfo)
                    {
                        asyncInfo = NULL;
                        usedlocalStackAsyncInfo = false;
                        //Undo the count we did just before the commit
                        iemq_reducePreDeleteCount(pThreadData, (ismEngine_Queue_t *)Q);
                    }

                    if ( rc == OK)
                    {
                        //We've done the commit, now carry on...
                        if (pDeliveryData->consumerLocked)
                        {
                            //waiter should be in delivering (or other threads can have added flags)
                            assert(
                                  ((pConsumer->iemqWaiterStatus) & (IEWS_WAITERSTATUSMASK_LOCKED & (~(IEWS_WAITERSTATUS_GETTING|IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT)))) != 0);
                            loopAgain=true;
                        }
                    }
                    else
                    {
                        ieutTRACE_FFDC( ieutPROBE_003, true, "iead_store_commit failed.", rc
                                , NULL);

                        goto mod_exit;
                    }
                }
            }
            else
            {
                //We haven't disabled the waiter (callComplete is false) and we
                //don't need a store commit, so we must have delivered all the messages
                assert(msgsDelivered == pDeliveryData->usedNodes);
            }
        }
    }
    while(loopAgain);

mod_exit:
    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE,
        FUNCTION_EXIT " rc=%d\n", __func__, rc);
    return rc;
}

static inline int32_t iemq_locateAndDeliverMessageBatchToBrowser(
                                  ieutThreadData_t *pThreadData,
                                  iemqQueue_t *Q,
                                  ismEngine_Consumer_t *pConsumer)
{
    //Record what "up-to-date" mean just before our get...d
    uint64_t checkWaitersValBeforeGet = Q->checkWaitersVal;
    bool fExplicitSuspends = pConsumer->pSession->fExplicitSuspends;
    bool completeWaiterActions = false;
    bool wantsMoreMessages = true;
    uint32_t msgsDelivered = 0;
    iewsWaiterStatus_t currState = IEWS_WAITERSTATUS_GETTING;
    int32_t rc = OK;

    do
    {
        ismMessageHeader_t    msgHdr;
        ismEngine_Message_t *pMessage = NULL;

        rc = iemq_locateAndPrepareMessageForBrowse( pThreadData
                                                  , Q
                                                  , pConsumer
                                                  , &pMessage
                                                  , &msgHdr);

        if (rc == OK)
        {
            if (currState == IEWS_WAITERSTATUS_GETTING)
            {
               iews_bool_changeState( &(pConsumer->iemqWaiterStatus)
                                    , IEWS_WAITERSTATUS_GETTING
                                    , IEWS_WAITERSTATUS_DELIVERING);
               //Don't worry whether it worked, if flags have been
               //added we'll notice when we unlock the waiter
               currState = IEWS_WAITERSTATUS_DELIVERING;
            }

            wantsMoreMessages = ism_engine_deliverMessage(
                                            pThreadData,
                                            pConsumer,
                                            NULL,
                                            pMessage,
                                            &msgHdr,
                                            ismMESSAGE_STATE_CONSUMED,
                                            0,
                                            NULL);

            msgsDelivered++;

            if (!wantsMoreMessages)
            {

                if (!fExplicitSuspends)
                {
                    iews_addPendFlagWhileLocked( &(pConsumer->iemqWaiterStatus)
                                               , IEWS_WAITERSTATUS_DISABLE_PEND);
                }
                //We need to call delivery callback etc...
                completeWaiterActions = true;
            }
        }
        else if (pConsumer->selectionRule != NULL)
        {
            //During locating a message for a selector we can put
            //the waiter into delivering to avoid other threads waiting even
            //if we never find a message that matches the selector
            if (pConsumer->iemqWaiterStatus == IEWS_WAITERSTATUS_DELIVERING)
            {
                currState = IEWS_WAITERSTATUS_DELIVERING;
            }
        }
    }
    while(   (rc == OK)
          && (wantsMoreMessages)
          && (msgsDelivered < IEMQ_MAXDELIVERYBATCH_SIZE));

    if (rc != OK)
    {
        if (rc == ISMRC_NoMsgAvail)
        {
            //This browser has received the last gettable message...
            //other waiters may be able to get more messages
            pConsumer->iemqNoMsgCheckVal = checkWaitersValBeforeGet;

            //We can clear noMsgAvail...other waiters might still see messages
            rc = OK;
        }
        else if (rc == ISMRC_NoMsgAvailForConsumer)
        {
            // There are no more messages for this consumer which matches it's
            // selection string but other messages are available on the queue.
            // Clear ISMRC_NoMsgAvailForConsumer so we will retry with another
            // consumer
            rc = OK;

            // Save the value indicating when the last message was received, this
            // will prevent this waiter from being choosen unless another
            // messages becomes available.
            pConsumer->iemqNoMsgCheckVal = checkWaitersValBeforeGet;
        }
        else if (rc == IMSRC_RecheckQueue)
        {
            // There has been a change to te queue while we were in delivering
            // state that meant we could not deliver the message (or perhaps
            // the selector didn't match). Nevertheless we must ensure we loop
            // round and check the queue again.
            rc = OK;  // reset return code ensures we will check again
        }
        else
        {
            ieutTRACE_FFDC(ieutPROBE_001, true,
                               "Unexpected rc", rc
                          , NULL);
        }
    }

    if (wantsMoreMessages)
    {
        //Unlock the waiter into enable state so it gets more messages
        iewsWaiterStatus_t oldState = iews_val_tryUnlockNoPending(
                                                &(pConsumer->iemqWaiterStatus)
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
        //Someone requested a callback whilst we had the waiter in getting
        //or delivering. It's our responsibility to fire the callback(s)
        ieq_completeWaiterActions( pThreadData
                , (ismEngine_Queue_t *) Q
                , pConsumer
                , IEQ_COMPLETEWAITERACTION_OPT_NODELIVER
                , true);
        assert(rc == OK); //We asked complete not to deliver so we need to ensure
                          //delivery happens - it will as rc will be ok
    }

    return rc;
}

//This has to do the same DiscardExpiryCheckWaiters as done in the jobThread phase of the transactions
// as if one is schedule, we don't schedule the other type. If they diverge we'd need to have two
// versions of Q->schedInfo for the two types of job.
void iemq_jobDiscardExpiryCheckWaiters(ieutThreadData_t *pThreadData,
                                       void *context)
{
    iemqJobDiscardExpiryCheckWaiterData_t *jobData = (iemqJobDiscardExpiryCheckWaiterData_t *)context;

    ieutTRACEL(pThreadData, jobData->asyncId, ENGINE_CEI_TRACE, FUNCTION_IDENT "iemqACId=0x%016lx\n",
                                                                            __func__, jobData->asyncId);
    iemqQueue_t *Q = jobData->Q;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    iemq_clearScheduledWork(pThreadData, Q, jobData->pJobThread);

    //Discard messages down to ensure there is "space" for the message we put
   iepiPolicyInfo_t *pPolicyInfo = Q->Common.PolicyInfo;

   if (((pPolicyInfo->maxMessageCount > 0) && (Q->bufferedMsgs >= pPolicyInfo->maxMessageCount)) ||
       ((pPolicyInfo->maxMessageBytes > 0) && (Q->bufferedMsgBytes >= pPolicyInfo->maxMessageBytes)))
    {
        //Check if we can remove expired messages and get below the max...
        ieme_reapQExpiredMessages(pThreadData, (ismEngine_Queue_t *)Q);

        if (    (pPolicyInfo->maxMsgBehavior == DiscardOldMessages)
             && (((pPolicyInfo->maxMessageCount > 0) && (Q->bufferedMsgs > pPolicyInfo->maxMessageCount)) ||
                 ((pPolicyInfo->maxMessageBytes > 0) && (Q->bufferedMsgBytes > pPolicyInfo->maxMessageBytes))))
        {
            //We reclaim before checkwaiters so that checkwaiters
            //doesn't put some really old messages in flight forcing us to remove newer ones
            iemq_reclaimSpace(pThreadData, (ismQHandle_t) Q, true);
        }
    }

    DEBUG_ONLY int32_t rc = iemq_checkWaiters( pThreadData
                                             , (ismEngine_Queue_t *)Q
                                             , NULL
                                             , NULL);
    assert (rc == OK || rc == ISMRC_AsyncCompletion);

    ieut_releaseThreadDataReference(jobData->pJobThread);
    iemq_reducePreDeleteCount(pThreadData, (ismEngine_Queue_t *)(jobData->Q));

    iere_primeThreadCache(pThreadData, resourceSet);
    iere_freeStruct(pThreadData, resourceSet, iemem_callbackContext, jobData, jobData->StructId);
}

//NOTE: Must have a Reference to the JobThread (if nonNULL) which this function will hand on to the callback
//or release if not scheduled

//returns true if scheduled (or determined already scheduled
//returns false if CheckWaiters still needs to be called
//                   (whatever the return code - reference to jobthread no longer valid to caller)

static bool iemq_scheduleOnJobThreadIfPoss(ieutThreadData_t *pThreadData,
                                           iemqQueue_t *Q,
                                           ieutThreadData_t *pJobThread)
{
    ieutTRACEL(pThreadData, Q,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "%p\n",__func__, Q);
    bool scheduled = false;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    if (pJobThread)
    {
        if (iemq_scheduleWork(pThreadData, Q, pJobThread))
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iemqJobDiscardExpiryCheckWaiterData_t *jobData;

            jobData = (iemqJobDiscardExpiryCheckWaiterData_t *)
                        iere_malloc(pThreadData,
                                    resourceSet,
                                    IEMEM_PROBE(iemem_callbackContext, 20),
                                    sizeof(iemqJobDiscardExpiryCheckWaiterData_t));

            if (jobData == NULL)
            {
                iemq_clearScheduledWork(pThreadData, Q, pJobThread);
                goto mod_exit;
            }

            //Ensure the queue doesn't go away before the job gets processed
            DEBUG_ONLY uint64_t oldCount = __sync_fetch_and_add(&(Q->preDeleteCount), 1);
            assert(oldCount > 0);

            ismEngine_SetStructId(jobData->StructId, IEMQ_JOBDISCARDEXPIRYCHECKWAITER_STRUCID);
            jobData->Q          = Q;
            jobData->asyncId    = pThreadData->asyncCounter++;
            jobData->pJobThread = pJobThread;

            ieutTRACEL(pThreadData, jobData->asyncId, ENGINE_CEI_TRACE, FUNCTION_IDENT "iemqACId=0x%016lx\n",
                                  __func__, jobData->asyncId);

            int32_t rc = iejq_addJob( pThreadData
                                    , pJobThread->jobQueue
                                    , iemq_jobDiscardExpiryCheckWaiters
                                    , jobData
#ifdef IEJQ_JOBQUEUE_PUTLOCK
                                    , true
#endif
                                    );

            if (rc == OK)
            {
#ifdef HAINVESTIGATIONSTATS
                pThreadData->stats.CheckWaitersScheduled++;
#endif
                scheduled = true;
            }
            else
            {
                assert(rc == ISMRC_DestinationFull);

                iemq_clearScheduledWork(pThreadData, Q, pJobThread);

                iere_primeThreadCache(pThreadData, resourceSet);
                iere_freeStruct(pThreadData, resourceSet, iemem_callbackContext, jobData, jobData->StructId);

                iemq_reducePreDeleteCount(pThreadData, (ismEngine_Queue_t *)Q);
                ieut_releaseThreadDataReference(pJobThread);
            }
        }
        else
        {
            //That thread will already checkWaiters later... no future action required
            scheduled = true; //already scheduled
            ieut_releaseThreadDataReference(pJobThread);
        }
    }
#ifdef HAINVESTIGATIONSTATS
    if(scheduled)pThreadData->stats.asyncMsgBatchDeliverScheduledCW++;
#endif

mod_exit:
    ieutTRACEL(pThreadData, scheduled, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "scheduled=%d\n",
               __func__, scheduled);
    return scheduled;
}


static int32_t iemq_asyncMessageDelivery(ieutThreadData_t           *pThreadData,
                                         int32_t                              rc,
                                         ismEngine_AsyncData_t        *asyncInfo,
                                         ismEngine_AsyncDataEntry_t     *context)
{
    iemqAsyncMessageDeliveryInfo_t *deliveryInfo = (iemqAsyncMessageDeliveryInfo_t *)(context->Data);
    ismEngine_CheckStructId(deliveryInfo->StructId, IEMQ_ASYNCMESSAGEDELIVERYINFO_STRUCID, ieutPROBE_001);
    assert(context->Type == iemqQueueDeliver);

    assert(rc == OK);
    assert(deliveryInfo->usedNodes > 0);

#ifdef HAINVESTIGATIONSTATS
    if ( deliveryInfo->pConsumer->lastDeliverCommitEnd.tv_sec == 0)
    {
        //If we haven't recorded a time for the first store commit in this get and there
        //has been a store commit in this get, record the end of the commit
        clock_gettime(CLOCK_REALTIME, &(deliveryInfo->pConsumer->lastDeliverCommitEnd));
    }
#endif
    ieutTRACE_HISTORYBUF(pThreadData, rc);

    //Remove the delivery record from the stack,
    //Deliver messages might re-add it if it needs to go async
    iead_popAsyncCallback( asyncInfo, context->DataLen);

    rc = iemq_deliverMessages( pThreadData
                             , deliveryInfo
                             , asyncInfo
                             , NULL );

    assert(rc == OK || rc == ISMRC_AsyncCompletion);

    if (rc == OK)
    {
        bool needCheckWaiters = true;

        //We delivered all those, check again
        if (deliveryInfo->pJobThread)
        {
#ifdef HAINVESTIGATIONSTATS
            pThreadData->stats.asyncMsgBatchDeliverWithJobThread++;
#endif
            if(iemq_scheduleOnJobThreadIfPoss(pThreadData, deliveryInfo->Q, deliveryInfo->pJobThread))
            {
                needCheckWaiters = false;
            }

            //The above call will have used our thread reference to the jobthread - we can no longer safely access it
            deliveryInfo->pJobThread=NULL;
        }
        else
        {
#ifdef HAINVESTIGATIONSTATS
               ieutTRACE_HISTORYBUF(pThreadData, pThreadData->stats.asyncMsgBatchDeliverNoJobThread);
               pThreadData->stats.asyncMsgBatchDeliverNoJobThread++;
#endif
        }

        if (needCheckWaiters)
        {
#ifdef HAINVESTIGATIONSTATS
               pThreadData->stats.asyncMsgBatchDeliverDirectCallCW++;
#endif

            rc = iemq_checkWaiters(pThreadData
                                  , (ismEngine_Queue_t *)(deliveryInfo->Q)
                                  , asyncInfo
                                  , NULL);
            assert (rc == OK || rc == ISMRC_AsyncCompletion);
        }
    }

    return rc;
}

static inline uint32_t iemq_chooseDeliveryBatchSizeFromMaxInflight(uint32_t maxInflightLimit)
{
    uint32_t batchSize = maxInflightLimit >> 2; //4 batches before we saturate our limit and turn everything off

    if (batchSize == 0)
    {
        batchSize = 1;
    }
    else if (batchSize > IEMQ_MAXDELIVERYBATCH_SIZE)
    {
        batchSize = IEMQ_MAXDELIVERYBATCH_SIZE;
    }

    return batchSize;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Finds a message for a waiter and delivers it to them
///  @remarks
///     Waiter must be locked (in getting state) on entry
///
///  @param[in] Q                  - Queue to get the message from
///  @param[in] pConsumer          - A pointer to the consumer to give the message
///  @param[inout] msgsLeftInBatch - On input:  1 if this is the last message in this batch
///                                  On output: Set to 0 if this should be the last message in batc
///                                             (otherwise 1 less than the input value)
///
///  @return OK
///          or ISMRC_NoMsgAvail (no more messages are available for any waiter)
///          or an error
///////////////////////////////////////////////////////////////////////////////
static inline int32_t iemq_locateAndDeliverMessageBatchToWaiter(
                                  ieutThreadData_t *pThreadData,
                                  iemqQueue_t *Q,
                                  ismEngine_Consumer_t *pConsumer,
                                  ismEngine_AsyncData_t *asyncInfo)
{

    //waiter should be in getting (or other threads can have added flags)
    assert(
        ((pConsumer->iemqWaiterStatus) & (IEWS_WAITERSTATUSMASK_LOCKED & (~(IEWS_WAITERSTATUS_DELIVERING|IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT)))) != 0);

    ieutTRACEL(pThreadData, pConsumer,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "%p\n",__func__, pConsumer);
    // If someone else changes the waiter state after we moved
    // it out of enabled, we'll set this to true
    bool completeWaiterActions = false;

    //Record what "up-to-date" mean just before our get...
    uint64_t checkWaitersValBeforeGet = Q->checkWaitersVal;

    iemqAsyncMessageDeliveryInfo_t deliveryData; //Stores a lists of nodes to deliver
    ismEngine_SetStructId(deliveryData.StructId, IEMQ_ASYNCMESSAGEDELIVERYINFO_STRUCID);
    deliveryData.Q = Q;
    deliveryData.pConsumer = pConsumer;
    deliveryData.consumerLocked = true;
    deliveryData.usedNodes = 0; //Not found any nodes to deliver yet
    deliveryData.firstDeliverNode = 0;
    deliveryData.pJobThread = NULL;


    iewsWaiterStatus_t currState = IEWS_WAITERSTATUS_GETTING;
    bool continueBatch = true;
    uint64_t storeOps = 0;
    int32_t rc = OK;

    if (! (pConsumer->fDestructiveGet))
    {
        //Browsers use a separate delivery function
        rc = iemq_locateAndDeliverMessageBatchToBrowser(
                pThreadData, Q, pConsumer);

        goto mod_exit;
    }

    uint32_t maxBatchSize = 32;
    uint32_t perClientLimit = pConsumer->pSession->pClient->maxInflightLimit;

    if (perClientLimit != 0)
    {
        maxBatchSize = iemq_chooseDeliveryBatchSizeFromMaxInflight(perClientLimit);
    }

    do
    {
        rc = iemq_locateMessage( pThreadData
                               , Q
                               , pConsumer
                               , &(deliveryData.perNodeInfo[deliveryData.usedNodes].node)
                               , &(deliveryData.perNodeInfo[deliveryData.usedNodes].origMsgState));

        if (rc == OK)
        {

            rc = iemq_initialPrepareMessageForDelivery( pThreadData
                                                      , Q
                                                      , pConsumer
                                                      , deliveryData.perNodeInfo[deliveryData.usedNodes].node
                                                      , &storeOps
                                                      , &continueBatch);

            if (rc == OK)
            {
                if (deliveryData.usedNodes == 0)
                {
                    // We've got a message for this waiter...change our state to
                    // reflect that. If someone else has disabled/enabled us...
                    // continue anyway for the moment. We'll notice when we try
                    // and enable/disable the waiter after the callback
                    iews_bool_changeState( &(pConsumer->iemqWaiterStatus)
                                         , IEWS_WAITERSTATUS_GETTING
                                         , IEWS_WAITERSTATUS_DELIVERING);
                    currState = IEWS_WAITERSTATUS_DELIVERING;

                    //TODO: *Need* to audit that at this point on we recheck the queue
                    //      maybe need a separate loopAgain funcparam?
                }

                deliveryData.usedNodes++;
            }
            else
            {
                //Ensure per consumer cursor and queue cursor rewound to here...
                //We don't need to worry about browse (that's a separate code path)
                iemq_abortDelivery(pThreadData, Q, pConsumer,
                                   deliveryData.perNodeInfo[deliveryData.usedNodes].node);
            }
        }
        else if (pConsumer->selectionRule != NULL)
        {
            //During locating a message for a selector we can put
            //the waiter into delivering to avoid other threads waiting even
            //if we never find a message that matches the selector
            if (pConsumer->iemqWaiterStatus == IEWS_WAITERSTATUS_DELIVERING)
            {
                currState = IEWS_WAITERSTATUS_DELIVERING;
            }
        }
    }
    while(rc == OK  && continueBatch && deliveryData.usedNodes < maxBatchSize);


    ieutTRACE_HISTORYBUF(pThreadData, rc);

    if ( rc != OK )
    {
        if (rc == ISMRC_MaxDeliveryIds)
        {
            //(temporarily) turn off messages for this consumer or client until acks arrive
            ismEngine_Session_t *pSession = pConsumer->pSession;

            //If there are acks outstanding for this consumer we can just turn off delivery for
            //this consumer rather than whole client (which is a costly operation

            bool donePerConsumerFlowControl =
                              iemq_tryPerConsumerFlowControl(pThreadData, Q, pConsumer);

            if (!donePerConsumerFlowControl)
            {
                //We didn't couldn't stop just this consumer so we should stop the whole session...
                rc = stopMessageDeliveryInternal(pThreadData, pSession,
                                                 ISM_ENGINE_INTERNAL_STOPMESSAGEDELIVERY_FLAGS_ENGINE_STOP,
                                                 NULL, 0, NULL);

                if (rc == ISMRC_NotEngineControlled)
                {
                    //That's fine protocol/transport have change the start/stop state and we shouldn't fiddle
                    rc = OK;
                }
                else if (rc != OK)
                {
                    ieutTRACE_FFDC(ieutPROBE_001, true,
                                       "Failed to stop session.", rc
                                  , "pSession", pSession, sizeof(ismEngine_Session_t)
                                  , NULL);
                }
            }

            //We want to deliver any messages we put in the batch before this return code...
            rc = OK;
        }
        else if (rc == ISMRC_NoMsgAvail)
        {
            if (pConsumer->selectionRule != NULL)
            {
                //This selecting consumer has received the last
                //gettable message save the value indicating when
                //the last message was received.
                pConsumer->iemqNoMsgCheckVal = checkWaitersValBeforeGet;
            }

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
                    ism_engine_deliverStatus( pThreadData, pConsumer, ISMRC_NoMsgAvail);
                }

                // If we reported queue full in the past, let's reset the flag
                // now that the queue is empty again.
                Q->ReportedQueueFull = false;
            }
            else
            {
                rc = OK; //No probs as we found some messages before we couldn't find one
            }
        }
        else if (rc == ISMRC_NoMsgAvailForConsumer)
        {
            // There are no more messages for this consumer which matches it's
            // selection string but other messages are available on the queue.
            // Clear ISMRC_NoMsgAvailForConsumer so we will retry with another
            // consumer
            rc = OK;

            // Save the value indicating when the last message was received, this
            // will prevent this waiter from being choosen unless another
            // messages becomes available.
            pConsumer->iemqNoMsgCheckVal = checkWaitersValBeforeGet;
        }
        else if (rc == IMSRC_RecheckQueue)
        {
            // There has been a change to te queue while we were in delivering
            // state that meant we could not deliver the message (or perhaps
            // the selector didn't match). Nevertheless we must ensure we loop
            // round and check the queue again.
            rc = OK;  // reset return code ensures we will check again
        }
        else if (rc == ISMRC_AllocateError)
        {
            //We failed to prepare the message for delivery....
            //Put all messages so far back on queue. No need to pass

            deliveryData.firstCancelledNode = 0;

            rc = iemq_undoInitialPrepareMessage( pThreadData
                                               , Q
                                               , false
                                               , &deliveryData
                                               , asyncInfo);

            //Won't have gone async, can have rolled back changes,
            //No need for (async) commit
            assert(rc == OK);

            //Disconnect the client if we can or come down in a flaming heap.
            //Should be in delivering to call the failure function...
            if (currState != IEWS_WAITERSTATUS_DELIVERING)
            {
                iews_bool_changeState( &(pConsumer->iemqWaiterStatus)
                                     , IEWS_WAITERSTATUS_GETTING
                                     , IEWS_WAITERSTATUS_DELIVERING);
            }
            iemq_handleDeliveryFailure(pThreadData, Q, pConsumer);

            //Consumer will now have pending flag set. Ensure they are actioned
            completeWaiterActions = true;

            //Put messages back on queue
            deliveryData.usedNodes = 0;
            storeOps = 0;

            //try delivery to other consumers
            rc = OK;
        }
        else
        {
            ieutTRACE_FFDC( ieutPROBE_001, true, "locate/iemq_initPrepareMessageForDelivery failed.", rc
                    , NULL);
        }
    }

    if (deliveryData.usedNodes > 0)
    {
#ifdef HAINVESTIGATIONSTATS
        pThreadData->stats.avgDeliveryBatchSize = (   (  pThreadData->stats.avgDeliveryBatchSize
                                                       * pThreadData->stats.numDeliveryBatches)
                                                   +  deliveryData.usedNodes)
                                                 / (1+pThreadData->stats.numDeliveryBatches);
        pThreadData->stats.numDeliveryBatches++;

        if (deliveryData.usedNodes < pThreadData->stats.minDeliveryBatchSize)
        {
            pThreadData->stats.minDeliveryBatchSize = deliveryData.usedNodes;
        }
        if (deliveryData.usedNodes > pThreadData->stats.maxDeliveryBatchSize)
        {
            pThreadData->stats.maxDeliveryBatchSize = deliveryData.usedNodes;
        }

        pConsumer->lastBatchSize = deliveryData.usedNodes;
#endif

        if (storeOps > 0)
        {
            //We want to async whether our caller wants to or not. If not, we just don't tell them.
            //As checkWaiters is something of a "black-box" to our caller, that shouldn't be a problem,
            //However having a put call checkWaiters with an asyncInfo means that we can hold up the
            //put for a while on a deep queue, giving some level of pacing
            ismEngine_AsyncDataEntry_t localstackAsyncArray[IEAD_MAXARRAYENTRIES];
            ismEngine_AsyncData_t localStackAsyncInfo;
            bool usedlocalStackAsyncInfo = false;

            if (asyncInfo == NULL)
            {
                ieutTRACE_HISTORYBUF(pThreadData, &localStackAsyncInfo);
                usedlocalStackAsyncInfo = true;
            }
#ifdef HAINVESTIGATIONSTATS
               pThreadData->stats.asyncMsgBatchStartLocate++;
#endif
            if (    pThreadData->jobQueue != NULL
                 && pThreadData->threadType != AsyncCallbackThreadType
                 && ismEngine_serverGlobal.ThreadJobAlgorithm == ismENGINE_THREAD_JOB_QUEUES_ALGORITHM_EXTRA)
            {
                //We want to try and keep the delivery off the AsyncCallBack thread as much as possible so ask the call
                //back to send further checkWaiters to this thread.
                ieutTRACE_HISTORYBUF(pThreadData, &localStackAsyncInfo);
                usedlocalStackAsyncInfo = true; //Whether or not we used it before, use the local one now //TODO: Should we force local stack in this case or use callers?

                ieut_acquireThreadDataReference(pThreadData);
                deliveryData.pJobThread = pThreadData;
            }
#ifdef HAINVESTIGATIONSTATS
            else
            {
                ieutTRACE_HISTORYBUF(pThreadData, pThreadData->stats.asyncMsgBatchStartLocate);
            }
#endif

            if (usedlocalStackAsyncInfo)
            {
                iemq_initialiseStackAsyncInfo(Q, &localStackAsyncInfo, localstackAsyncArray);

                asyncInfo = &localStackAsyncInfo;

                //We're not going to worry our caller with the info that we're going async, so
                //we need to ensure the queue doesn't get deleted out from under us
                DEBUG_ONLY uint64_t oldCount = __sync_fetch_and_add(&(Q->preDeleteCount), 1);
                assert(oldCount > 0);
            }

            //don't copy unused nodes in batch
            size_t dataSize =   offsetof(iemqAsyncMessageDeliveryInfo_t,perNodeInfo)
                              + (sizeof(iemqAsyncMessageDeliveryInfoPerNode_t) * deliveryData.usedNodes);

            assert(dataSize > (    ((void *)&(deliveryData.perNodeInfo[deliveryData.usedNodes-1].origMsgState))
                                 - ((void *)&deliveryData)));
            assert(dataSize <= sizeof(deliveryData));

            ismEngine_AsyncDataEntry_t newEntry =
                 {ismENGINE_ASYNCDATAENTRY_STRUCID, iemqQueueDeliver, &deliveryData, dataSize,
                         NULL, {.internalFn = iemq_asyncMessageDelivery}};

            iead_pushAsyncCallback(pThreadData, asyncInfo, &newEntry);

#ifdef HAINVESTIGATIONSTATS
            clock_gettime(CLOCK_REALTIME, &(pConsumer->lastDeliverCommitStart));
#endif

            rc = iead_store_asyncCommit(pThreadData, false, asyncInfo);

            if (rc == ISMRC_AsyncCompletion)
            {
                //Gone async... get out of here
                if (usedlocalStackAsyncInfo)
                {
                    //We went async but nothing on the async stack will notify caller
                    //so as far as they are concerned, we are done.
                    rc = ISMRC_OK;
                }
                goto mod_exit;
            }
            else
            {
                //We didn't go async, we don't need the delivery callback to be on the asyncinfo stack
#ifdef HAINVESTIGATIONSTATS
                clock_gettime(CLOCK_REALTIME, &(pConsumer->lastDeliverCommitEnd));
#endif
                iead_popAsyncCallback(asyncInfo, newEntry.DataLen);

                if (usedlocalStackAsyncInfo)
                {
                    asyncInfo = NULL;
                    usedlocalStackAsyncInfo = false;
                    //Undo the count we did just before the commit
                    iemq_reducePreDeleteCount(pThreadData, (ismEngine_Queue_t *)Q);

                    if(deliveryData.pJobThread)
                    {
                        ieut_releaseThreadDataReference(deliveryData.pJobThread);
                        deliveryData.pJobThread = NULL;
                    }
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

            rc = iemq_deliverMessages( pThreadData
                                     , &deliveryData
                                     , asyncInfo
                                     , NULL);

            //deliverMessages will have released our grip on the waiter...
            //check we aren't fiddling
            assert(!completeWaiterActions);
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
        iewsWaiterStatus_t oldState = iews_val_tryUnlockNoPending(
                                                 &(pConsumer->iemqWaiterStatus)
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
        //Someone requested a callback whilst we had the waiter in getting
        //or delivering. It's our responsibility to fire the callback(s)
        ieq_completeWaiterActions( pThreadData
                                 , (ismEngine_Queue_t *) Q
                                 , pConsumer
                                 , IEQ_COMPLETEWAITERACTION_OPT_NODELIVER
                                 , true);
        assert(rc == OK || rc == ISMRC_NoMsgAvail);
        rc = OK; //check again as the _PEND states count as delivering...
    }

mod_exit:
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    See if there are too many inflight messages to deliver more
///  @remarks
///    If queue has too many messages inflight, this function sets the fullDeliveryPrevention flag
///
///  @param[in] q                  - Queue
///  @return                       - whether delivery on queue is allowed (true=delivery allowed)
///////////////////////////////////////////////////////////////////////////////
inline static bool iemq_checkAndSetFullDeliveryPrevention( ieutThreadData_t *pThreadData
                                                         , iemqQueue_t *Q)
{
    bool deliveryAllowed = true;
    iepiPolicyInfo_t *pPolicyInfo = Q->Common.PolicyInfo;

    // If we have enough inflight enqs and deqs to fill the queue, don't give out any more.
    if (pPolicyInfo->maxMessageCount > 0 && (pPolicyInfo->maxMsgBehavior == DiscardOldMessages))
    {
        uint64_t targetBufferedMessages = 0;
        targetBufferedMessages = (uint64_t)(1 + (pPolicyInfo->maxMessageCount * IEQ_RECLAIMSPACE_MSGS_SURVIVE_FRACTION));

        if (Q->inflightEnqs + Q->inflightDeqs >= targetBufferedMessages)
        {
            //Don't give out any more messages
            deliveryAllowed = false;

            __sync_lock_test_and_set(&(Q->fullDeliveryPrevention), 1);

            //After we've set it, check it's still true so we don't end up in quiet queue
            if (Q->inflightEnqs + Q->inflightDeqs < targetBufferedMessages)
            {
                //d'oh - already fixed... unset the flag... our caller will check queue
                __sync_lock_release(&(Q->fullDeliveryPrevention));
                deliveryAllowed = true;
            }
        }
        else if (Q->fullDeliveryPrevention)
        {
            __sync_lock_release(&(Q->fullDeliveryPrevention)); //Caller will check the queue
        }
    }

    return deliveryAllowed;
}

static inline uint8_t atomicExchange_uint8( volatile uint8_t *ptr, uint8_t newValue)
{
    uint8_t oldValue = *ptr; //Last value of *ptr we know about before the cas works
    uint8_t fromValue; //from value we'll use in the CAS
    do
    {
        fromValue = oldValue;

        oldValue = __sync_val_compare_and_swap(ptr, fromValue, newValue);
    }
    while(fromValue != oldValue);

    return oldValue;
}

//
inline static bool iemq_checkFullDeliveryStartable( ieutThreadData_t *pThreadData
                                                  , iemqQueue_t *Q)
{
    bool needDelivery = false;
    iepiPolicyInfo_t *pPolicyInfo = Q->Common.PolicyInfo;

    if(  (pPolicyInfo == NULL)
       ||(pPolicyInfo->maxMessageCount == 0)
       ||(pPolicyInfo->maxMsgBehavior == RejectNewMessages))
    {
        goto mod_exit;
    }
    uint64_t targetBufferedMessages = 0;

    targetBufferedMessages = (uint64_t)(1 + (pPolicyInfo->maxMessageCount * IEQ_RECLAIMSPACE_MSGS_SURVIVE_FRACTION));


    if (   (Q->fullDeliveryPrevention)
        && (Q->inflightEnqs + Q->inflightDeqs < targetBufferedMessages))
    {
        uint8_t oldValue = atomicExchange_uint8(&(Q->fullDeliveryPrevention), 0);

        if( oldValue != 0)
        {
            needDelivery = true;
        }
    }

mod_exit:
    return needDelivery;
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
///  @remarks
///    Even if no asyncInfo is supplied, this function (well sub functions)
///    will still go async, but the caller won't be able to find out when
///    it completes
///
///  @param[in] q                  - Queue
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iemq_checkWaiters( ieutThreadData_t *pThreadData
                         , ismQHandle_t Qhdl
                         , ismEngine_AsyncData_t * asyncInfo
                         , ismEngine_DelivererContext_t *delivererContext)
{
    int32_t rc = OK;
    bool loopAgain = true;
    iemqQueue_t *Q = (iemqQueue_t *) Qhdl;
    bool tellCallerWentAsync = false;

    ieutTRACEL(pThreadData, Q, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n",
               __func__, Q);

#if TRACETIMESTAMP_CHECKWAITERS
    uint64_t TTS_Start_CheckWaiters = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_CheckWaiters);
#endif



#ifdef HAINVESTIGATIONSTATS
    pThreadData->stats.CheckWaitersCalled++;
#endif
#ifndef NDEBUG
    bool checkonExit = false;
    {
        uint32_t storeOpsCount = 0;
        int32_t store_rc = ism_store_getStreamOpsCount(pThreadData->hStream, &storeOpsCount);

        if ( store_rc == OK && storeOpsCount == 0)
        {
            checkonExit = true;
        }
        else
        {
            ieutTRACE_HISTORYBUF( pThreadData, storeOpsCount);
        }
    }
#endif

    if (Q->bufferedMsgs == 0)
    {
        //There appear to be no messages on this queue, anyone who adds a message will
        //call checkWaiters themselves so we don't need to set a memory barrier on this number or
        //anything... we're done.
        goto mod_exit;
    }

    //If there are browsers on the queue, we need a concept of "uptodate", as checkwaiters has been called, we suspect an
    //update may have occurred so we increase the checkWaitersVal which changes our opinion of what uptodate means.
    if ((Q->numBrowsingWaiters > 0) || (Q->numSelectingWaiters > 0))
    {
        __sync_add_and_fetch(&(Q->checkWaitersVal), 1);
    }

    // We loop giving messages until we run out of messages to give or
    // we can't lock a waiter who is enabled (we don't lock browsers who are uptodate)
    do
    {
        loopAgain = true;
        ismEngine_Consumer_t *pConsumer = NULL;

        rc = iemq_lockWillingWaiter(pThreadData, Q,
                                    IEWS_WAITERSTATUS_GETTING, &pConsumer);

        if (rc == OK)
        {
#ifdef HAINVESTIGATIONSTATS
            clock_gettime(CLOCK_REALTIME, &(pConsumer->lastGetTime));

            if (pConsumer->lastDeliverEnd.tv_sec != 0)
            {
                pConsumer->lastGapBetweenGets = timespec_diff_int64( &(pConsumer->lastDeliverEnd)
                                                                   , &(pConsumer->lastGetTime));
            }
            else
            {
                //first ever get... no sensible value... but don't use time since clock start...
                pConsumer->lastGapBetweenGets = 0;
            }

            pConsumer->lastDeliverCommitStart.tv_sec  = 0;
            pConsumer->lastDeliverCommitStart.tv_nsec = 0;
            pConsumer->lastDeliverCommitEnd.tv_sec = 0;
            pConsumer->lastDeliverCommitEnd.tv_nsec = 0;
            pConsumer->lastDeliverEnd.tv_sec = 0;
            pConsumer->lastDeliverEnd.tv_nsec = 0;
            pConsumer->lastBatchSize = 0;

            pThreadData->stats.CheckWaitersGotWaiter++;
#endif
            //waiter is now in getting state for us to get...give it a batch of messages
             rc = iemq_locateAndDeliverMessageBatchToWaiter( pThreadData
                                                           , Q
                                                           , pConsumer
                                                           , asyncInfo);

            if (rc == ISMRC_NoMsgAvail)
            {
                //No more messages...we're done, and that's not an error condition!
                loopAgain = false;
                rc = OK;
            }
        }
        else if (rc == ISMRC_NoAvailWaiter)
        {
            //No waiter wanted a message....that's not an error condition but we are finished
            loopAgain = false;
            rc = OK;
        }

        if (rc == ISMRC_AsyncCompletion)
        {
            //If the caller passed us an asyncInfo, they want to know that we've committed it - so remember
            //but clear our copy of it so we don't commit it again.
            if (asyncInfo != NULL)
            {
                tellCallerWentAsync = true;
                asyncInfo = NULL;
            }

            //We want to see if there's anyone else we can give a batch of messages to...
            rc = OK;
        }
    }
    while ((rc == OK) && loopAgain);

mod_exit:

#ifndef NDEBUG
    {
        uint32_t storeOpsCount = 0;
        int32_t store_rc = ism_store_getStreamOpsCount(pThreadData->hStream, &storeOpsCount);

        if(checkonExit)
        {
            if ( store_rc == OK && storeOpsCount != 0)
            {
                abort();
            }
        }
        else
        {
            ieutTRACE_HISTORYBUF( pThreadData, storeOpsCount);
        }
    }
#endif
#ifdef HAINVESTIGATIONSTATS
    if (rc == ISMRC_AsyncCompletion)pThreadData->stats.CheckWaitersInternalAsync++;
#endif
    if (tellCallerWentAsync && rc == OK)
    {
        //Our caller wanted to know if we went async so...
        rc = ISMRC_AsyncCompletion;
    }

#if TRACETIMESTAMP_CHECKWAITERS
{
    uint64_t TTS_Stop_CheckWaiters = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_CheckWaiters);
}
#endif

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n",
               __func__, rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Attempt to deliver message directly to consumer
///  @remarks
///    This function is called directly from the iemq_putMessage function
///    to attempt to deliver a QoS 0 message directly to the conusmer. The
///    consumer must be in enabled state in order for the message to be
///    delivered.
///
///  @param[in] q                  - Queue
///  @param[in] msg                - Message
///  @return                       - OK or ISMRC_NoAvailWaiter
///////////////////////////////////////////////////////////////////////////////
#if 0
static int32_t iemq_putToWaitingGetter( iemqQueue_t *q
                                      , ismEngine_Message_t *msg
                                      , uint8_t msgFlags
                                      , ismEngine_DelivererContext_t * delivererContext )
{
    int32_t rc = OK;
    bool deliveredMessage = false;
    bool loopAgain = true;

    TRACE(ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p\n", __func__,q);

    do
    {
        loopAgain = false;
        ismEngine_Consumer_t *pConsumer = NULL;

        rc = iemq_lockWillingWaiter(q, IEWS_WAITERSTATUS_DELIVERING, &pConsumer);

        if (rc == OK)
        {
            bool reenableWaiter = false;
            ismMessageHeader_t msgHdr = msg->Header;

            // Include any additional flags
            msgHdr.Flags |= msgFlags;

            // Update the record for the number of messages delivered directly
            // to the consumer
            q->qavoidCount++;

            reenableWaiter = ism_engine_deliverMessage(
                                 pConsumer
                                 , NULL
                                 , msg
                                 , &msgHdr
                                 , ismMESSAGE_STATE_CONSUMED
                                 , 0
                                 , delivererContext);

            iewsWaiterStatus_t oldStatus;

            if (reenableWaiter)
            {
                oldStatus = __sync_val_compare_and_swap( &(pConsumer->iemqWaiterStatus)
                            , IEWS_WAITERSTATUS_DELIVERING
                            , IEWS_WAITERSTATUS_ENABLED );
            }
            else
            {
                oldStatus = __sync_val_compare_and_swap( &(pConsumer->iemqWaiterStatus)
                            , IEWS_WAITERSTATUS_DELIVERING
                            , IEWS_WAITERSTATUS_DISABLED );
            }

            // If someone requested a callback  whilst we had
            // the waiter in state: got, It's our responsibility to fire
            // the callback
            if (oldStatus != IEWS_WAITERSTATUS_DELIVERING)
            {
                iemq_completeWaiterActions(q, pConsumer);
            }

            //We've successfully delivered the message, we're done
            deliveredMessage = true;
            break;
        }
        else if (rc == ISMRC_NoAvailWaiter)
        {
            //No waiter wanted a message....that's not an error!
            rc = OK;
        }
    }
    while (loopAgain);

    // While we had the waiter in got state it may have missed puts so
    // we need to check waiters
    if (deliveredMessage)
    {
        if (q->bufferedMsgs > 0)
        {
            // The only valid return codes from this function is OK
            // or ISMRC_NoAvailWaiter. If checkWaiters encounters a
            // problem we do not care.
            (void) iemq_checkWaiters((ismQHandle_t)q);
        }
    }
    else
    {
        rc = ISMRC_NoAvailWaiter;
    }

    TRACE(ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d,Q=%p\n", __func__, rc,q);
    return rc;
}
#endif

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
static inline void iemq_fullExpiryScan( ieutThreadData_t *pThreadData
                                      , iemqQueue_t *Q
                                      , uint32_t nowExpire
                                      , bool *pPageCleanupNeeded)
{
    ieutTRACEL(pThreadData,Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p \n", __func__,Q);

    //Reset expiry data
    ieme_clearQExpiryDataPreLocked( pThreadData
                                  , (ismEngine_Queue_t *)Q);

    iemqQNode_t *currNode = &(Q->headPage->nodes[0]);

    uint32_t MaxBatchSize = 50;
    iemqQNode_t *expiredNodes[MaxBatchSize];
    uint32_t numExpiredNodes = 0;

    while (1)
    {
        ismEngine_Message_t *msg = currNode->msg;
        iemqQNode_t *nextNode = iemq_getSubsequentNode(Q, currNode);

        if (nextNode == NULL)
        {
            //We don't destroy/get etc a node until the subsequent node has
            //been alloced so the cursor can point at that.
            break;
        }

        if (   (msg != NULL)
            && (currNode->msgState == ismMESSAGE_STATE_AVAILABLE))
        {
            uint32_t msgExpiry = msg->Header.Expiry;

            if (msgExpiry != 0)
            {
                bool consumedMessage = false;

                if (msgExpiry < nowExpire)
                {
                    int rc = iemq_updateMsgIfAvail( pThreadData, Q, currNode, false, ieqMESSAGE_STATE_DISCARDING);

                    if (rc == OK)
                    {
                        consumedMessage=true;
                    }
                }

                if (consumedMessage)
                {
                    expiredNodes[numExpiredNodes] = currNode;
                    numExpiredNodes++;

                    if (numExpiredNodes >= MaxBatchSize)
                    {
                        iemq_destroyMessageBatch( pThreadData
                                                , Q
                                                , numExpiredNodes
                                                , expiredNodes
                                                , false
                                                , pPageCleanupNeeded);

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

        currNode = nextNode;
    }

    if (numExpiredNodes > 0)
    {
        iemq_destroyMessageBatch( pThreadData
                                , Q
                                , numExpiredNodes
                                , expiredNodes
                                , false
                                , pPageCleanupNeeded);

        __sync_fetch_and_add(&(Q->expiredMsgs), numExpiredNodes);
        pThreadData->stats.expiredMsgCount += numExpiredNodes;
    }

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_EXIT "Q=%p\n", __func__, Q);
}



static inline void iemq_reducePreDeleteCount_internal(
                             ieutThreadData_t *pThreadData
                           , iemqQueue_t *Q)
{
    uint64_t oldCount = __sync_fetch_and_sub(&(Q->preDeleteCount), 1);

    assert(oldCount > 0);

    if (oldCount == 1)
    {
        //We've just removed the last thing blocking deleting the queue, do it now....
        iemq_completeDeletion(pThreadData, Q);
    }
}

void iemq_reducePreDeleteCount(ieutThreadData_t *pThreadData,
                               ismEngine_Queue_t *Q)
{
    iemq_reducePreDeleteCount_internal(pThreadData, (iemqQueue_t *) Q);
}

//The queue has already been marked deleted so most failures can be corrected
//at restart...
static void iemq_completeDeletion(ieutThreadData_t *pThreadData, iemqQueue_t *Q)
{
#define DELETE_BATCH_SIZE 1000

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p, id: %u\n",
               __func__, Q, Q->qId);
    int32_t rc = OK;
    uint32_t commitBatchSize = 0;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    assert(Q->isDeleted);
    assert(!(Q->deletionCompleted));
    Q->deletionCompleted = true;

    ieme_freeQExpiryData(pThreadData, (ismEngine_Queue_t *)Q);

    // Close the queue reference context before we (possibly) delete the queue record
    if (Q->QueueRefContext != NULL)
    {
        rc = ism_store_closeReferenceContext(Q->QueueRefContext);
        if (UNLIKELY(rc != OK))
        {
            ieutTRACE_FFDC( ieutPROBE_001, false, "close reference context failed!", rc
                          , "Q", Q, sizeof(*Q)
                          , "InternalName", Q->InternalName, sizeof(Q->InternalName)
                          , NULL);
        }
    }

    // If there is a store definition record associated with this queue (which may be a QDR or an
    // SDR) then delete it and it's associated properties record (a QPR or an SPR).
    if ((rc == OK) && (Q->hStoreObj != ismSTORE_NULL_HANDLE))
    {
        assert(Q->hStoreProps != ismSTORE_NULL_HANDLE);

        if (Q->deletionRemovesStoreObjects)
        {
            rc = ism_store_deleteRecord(pThreadData->hStream, Q->hStoreObj);
            if (rc != OK)
            {
                ieutTRACE_FFDC(ieutPROBE_002, false,
                                   "ism_store_deleteRecord (hStoreObj) failed!", rc
                              , "Q", Q, sizeof(*Q)
                              , "InternalName", Q->InternalName, sizeof(Q->InternalName)
                              , NULL);
            }
            rc = ism_store_deleteRecord(pThreadData->hStream, Q->hStoreProps);
            if (rc != OK)
            {
                ieutTRACE_FFDC(ieutPROBE_003, false,
                                    "ism_store_deleteRecord (hStoreProps) failed!", rc
                              , "Q", Q, sizeof(*Q)
                              , "RecType", (Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE) ? "SPR" :
                                             (Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) ? "RPR" : "QPR", 3
                              , "InternalName", Q->InternalName, sizeof(Q->InternalName)
                              , NULL);
            }
            iest_store_commit(pThreadData, false);
        }
    }

    size_t messageBytes = 0;

    //Now any references from the queue to the messages have been removed (and
    //committed) we can go through and remove the messages from memory
    if (rc == OK)
    {
        iemqQNode_t *head = &(Q->headPage->nodes[0]);

        while (Q->headPage != NULL)
        {
            iemqQNodePage_t *pageToFree = NULL;
            iemqQNode_t *temp = head;

            // Move the head to the next item..
            head++;

            //Have we gone off the bottom of the page?
            if (head->msgState == ieqMESSAGE_STATE_END_OF_PAGE)
            {
                pageToFree = Q->headPage;
                iemqQNodePage_t *nextPage = pageToFree->next;

                if (nextPage != NULL)
                {
                    Q->headPage = nextPage;
                    head = &(nextPage->nodes[0]);
                }
                else
                {
                    Q->headPage = NULL;
                    head = NULL;
                }
            }

            if (temp->msg != MESSAGE_STATUS_EMPTY)
            {
                // First if we put the message in the store, then also
                // decrement the store reference to the message
                if (Q->deletionRemovesStoreObjects && (temp->inStore))
                {
                    iest_unstoreMessage(pThreadData, temp->msg, false, false,
                                        NULL, &commitBatchSize);

                    if (commitBatchSize >= DELETE_BATCH_SIZE)
                    {
                        // We commit in batches even though the store should be
                        // able to handle large transactions there is no value
                        // in pushing too hard here.
                        iest_store_commit(pThreadData, false);
                        commitBatchSize = 0;
                    }
                }

                messageBytes += IEMQ_MESSAGE_BYTES(temp->msg);

                // Then release our reference to the message memory
                iem_releaseMessage(pThreadData, temp->msg);
            }

            if (pageToFree != NULL)
            {
                iere_primeThreadCache(pThreadData, resourceSet);
                iere_freeStruct(pThreadData, resourceSet, iemem_multiConsumerQPage, pageToFree, pageToFree->StrucId);
            }
        }

        // And commit the store changes
        if (commitBatchSize)
        {
            iest_store_commit(pThreadData, false);
        }
    }

    int32_t os_rc = pthread_rwlock_destroy(&(Q->headlock));
    if (UNLIKELY(os_rc != OK))
    {
        ieutTRACE_FFDC( ieutPROBE_004, true, "destroy headlock failed!", ISMRC_Error
                      , "Q", Q, sizeof(*Q)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    os_rc = iemq_destroyPutLock(Q);
    if (UNLIKELY(os_rc != OK))
    {
        ieutTRACE_FFDC( ieutPROBE_005, true, "destroy putlock failed!", ISMRC_Error
                      , "Q", Q, sizeof(*Q)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }
    os_rc = pthread_mutex_destroy(&(Q->getlock));
    if (UNLIKELY(os_rc != OK))
    {
        ieutTRACE_FFDC( ieutPROBE_006, true, "destroy getlock failed!", ISMRC_Error
                      , "Q", Q, sizeof(*Q)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    os_rc = pthread_rwlock_destroy(&(Q->waiterListLock));
    if (UNLIKELY(os_rc != OK))
    {
        ieutTRACE_FFDC( ieutPROBE_007, true, "destroy waiterlistlock failed!", ISMRC_Error
                      , "Q", Q, sizeof(*Q)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    if (Q->selectionLockScope != NULL)
    {
        (void)ielm_freeLockScope(pThreadData, &(Q->selectionLockScope));
    }

    // Release our use of the messaging policy info
    iepi_releasePolicyInfo(pThreadData, Q->Common.PolicyInfo);

    //All the messages that were buffered on this queue need to be removed from the global
    //buffered message counts
    iere_primeThreadCache(pThreadData, resourceSet);
    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -(int64_t)(Q->bufferedMsgs));
    pThreadData->stats.bufferedMsgCount -= Q->bufferedMsgs;
    if ((Q->QOptions & ieqOptions_REMOTE_SERVER_QUEUE) != 0)
    {
        pThreadData->stats.remoteServerBufferedMsgBytes -= messageBytes;
    }

    iemq_destroySchedulingInfo(pThreadData, Q);

    if (Q->Common.QName != NULL)
    {
        iere_free(pThreadData, resourceSet, iemem_multiConsumerQ, Q->Common.QName);
    }

    iere_freeStruct(pThreadData, resourceSet, iemem_multiConsumerQ, Q, Q->Common.StrucId);

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return;
}

static inline uint32_t iemq_choosePageSize(iemqQueue_t *Q)
{
    uint32_t pageSize;

    if (    (Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE)
         && (ismEngine_serverGlobal.totalSubsCount > ieqNUMSUBS_HIGH_CAPACITY_THRESHOLD))
    {
        pageSize = iemqPAGESIZE_HIGHCAPACITY;
    }
    // IANTODO: ieqOptions_REMOTE_SERVER_QUEUE?
    else
    {
        pageSize = iemqPAGESIZE_DEFAULT;
    }

    return pageSize;
}

// If the message is in the store, we need to update the store too
static inline void iemq_updateMsgRefInStore(ieutThreadData_t *pThreadData,
                                            iemqQueue_t *Q,
                                            iemqQNode_t *pnode,
                                            uint8_t msgState,
                                            bool consumeQos2OnRestart,
                                            uint8_t deliveryCount,
                                            bool doStoreCommit)
{
    ieutTRACEL(pThreadData, pnode->hMsgRef, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "Q=%p, msgref=0x%lx, msgState=%u %c\n", __func__, Q,
               pnode->hMsgRef, msgState, consumeQos2OnRestart ? '1':'0');
    int32_t rc = OK;
    uint8_t state = OK;

    // mark the message as available,
    uint8_t storedCount = (deliveryCount < ieqENGINE_MAX_REDELIVERY_COUNT) ?
                                    deliveryCount : ieqENGINE_MAX_REDELIVERY_COUNT;
    state = storedCount & 0x3f;

    if (   consumeQos2OnRestart
        && (msgState != ismMESSAGE_STATE_AVAILABLE)
        && (pnode->msg->Header.Reliability == ismMESSAGE_RELIABILITY_EXACTLY_ONCE) )
    {
        //If we are re-reading the store the this consumer is disconnected and hence
        //node should be discarded on restart
        msgState = ismMESSAGE_STATE_CONSUMED;
    }

    state |= (msgState << 6);

    if (doStoreCommit)
    {
        iest_AssertStoreCommitAllowed(pThreadData);

        rc = iest_store_updateReferenceCommit(pThreadData, pThreadData->hStream,
                                              Q->QueueRefContext, pnode->hMsgRef, pnode->orderId, state, 0);
    }
    else
    {
        rc = ism_store_updateReference(pThreadData->hStream, Q->QueueRefContext,
                                       pnode->hMsgRef, pnode->orderId, state, 0);
    }
    if (UNLIKELY(rc != OK))
    {
        // The failure to update a store reference means that something has gone seriously wrong!
        ieutTRACE_FFDC( ieutPROBE_001, true,
                            "ism_store_updateReference (multiConsumer) failed.", rc
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , "Reference", &pnode->hMsgRef, sizeof(pnode->hMsgRef)
                      , "OrderId", &pnode->orderId, sizeof(pnode->orderId)
                      , "pNode", pnode, sizeof(iemqQNode_t)
                      , NULL);
    }

    ieutTRACEL(pThreadData, msgState, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return;
}


static uint64_t iemq_getConsumerAckCount( iemqQueue_t *Q
                                        , ismEngine_Consumer_t *pConsumer)
{
    if (Q->ackListsUpdating)
    {
        //We are keeping perconsumer-acklists, we can use the per-consumer count
        return pConsumer->counts.parts.pendingAckCount;
    }
    else
    {
        //We don't have a per consumer count but that implies we can use
        //the queue count as if there were multiple consumers we'd need per
        //consumer counts
        return Q->inflightDeqs;
    }
}

static inline bool iemq_tryPerConsumerFlowControl( ieutThreadData_t *pThreadData
                                                 , iemqQueue_t *Q
                                                 , ismEngine_Consumer_t *pConsumer )
{
    //If the acks are spread out across too many consumers, turning each off individually
    //will be more costly than the (costly) operation of turning off the whole session...
    const int minAcksPerConsumer = 5;
    bool doingPerConsumer = true;

    if (iemq_getConsumerAckCount(Q, pConsumer) < minAcksPerConsumer )
    {
        //Too few acks for this consumer, better to try the whole session...
        doingPerConsumer = false;
    }
    else
    {
        //Ok... it's worth a shot, turn on the per consumer flag with appropriate barriers...
        (void)__sync_lock_test_and_set(&(pConsumer->fFlowControl), 1);

        //Now the per consumer flag has been set, are there still enough acks that an
        //ack will definitely arrive and trigger the flow to be turned back on for this
        //consumer? The minimum needs to be strictly greater than 1 in case another thread
        //is in the process of performing the last ack and has already checked the flow control
        //flag

        if (iemq_getConsumerAckCount(Q, pConsumer) < minAcksPerConsumer )
        {
            //The taps may not come back on if we try this, give up and use whole session...
            __sync_lock_release(&(pConsumer->fFlowControl));

            doingPerConsumer  = false;
        }
        else
        {
#ifdef HAINVESTIGATIONSTATS
            pThreadData->stats.perConsumerFlowControlCount++;
#endif
        }
    }
    ieutTRACEL(pThreadData, doingPerConsumer,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "%d\n",__func__, doingPerConsumer);
    return doingPerConsumer;
}

static inline bool iemq_forceRemoveBadAcker( ieutThreadData_t *pThreadData
                                           , iemqQueue_t      *Q
                                           , uint64_t         aliveOId)
{
    bool headLocked=true;
    bool waiterListLocked = false;
    char clientIdBuffer[256];
    char *nonAckingClientId = NULL;
    bool nonAckingClientIdAlloced = false;
    bool doingRemove = false;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    assert(aliveOId != 0);

    iemq_takeReadHeadLock(Q);

    iemqQNodePage_t *curHead = Q->headPage;

    if (curHead->nextStatus != completed)
    {
        //Can't delete only page!
        ieutTRACEL(pThreadData, curHead->nextStatus,  ENGINE_INTERESTING_TRACE,
                FUNCTION_IDENT "head nextStatus: %u\n",__func__,  curHead->nextStatus);
        goto mod_exit;
    }

    uint64_t headFirstOId = curHead->nodes[0].orderId;

    if (headFirstOId > aliveOId)
    {
        //Head has moved on... no action
        ieutTRACEL(pThreadData, headFirstOId,  ENGINE_INTERESTING_TRACE,
                FUNCTION_IDENT "headFirstOId: %lu NodeCausingConcern %lu already gone\n",
                   __func__,  headFirstOId, aliveOId);
        goto mod_exit;
    }

    //Ok find the node and look at it's state
    iemqQNode_t *badNode = NULL;

    for ( uint32_t nodeNum = 0; nodeNum < curHead->nodesInPage; nodeNum++)
    {
        if (curHead->nodes[nodeNum].orderId == aliveOId)
        {
            badNode = &(curHead->nodes[nodeNum]);
            break;
        }
    }

    if (badNode == NULL)
    {
        ieutTRACE_FFDC( ieutPROBE_001, true,
                           "Trapped Node NOT on first page.", ISMRC_Error
                      , "Internal Name", Q->InternalName, sizeof(Q->InternalName)
                      , "Queue", Q, sizeof(iemqQueue_t)
                      , "TrappedOrderId", &aliveOId, sizeof(aliveOId)
                      , "HeadOrderId", &aliveOId, sizeof(aliveOId)
                      , "HeadPageSize", &(curHead->nodesInPage), sizeof(curHead->nodesInPage)
                      , "NextQOId", &(Q->nextOrderId), sizeof(Q->nextOrderId)
                      , NULL);
    }

    //Check the current state of the msgNode:
    ismMessageState_t msgState;
    ielmLockName_t LockName = { .Msg = { ielmLOCK_TYPE_MESSAGE,
                                         Q->qId, badNode->orderId } };

    int rc = ielm_instantLockWithPeek( &LockName
                                     , &badNode->msgState
                                     , &msgState);

    if (rc == OK)
    {
        if (   msgState != ismMESSAGE_STATE_DELIVERED
            && msgState != ismMESSAGE_STATE_RECEIVED)
        {
            //Not an ackable node.
            ieutTRACEL(pThreadData, msgState,  ENGINE_INTERESTING_TRACE,
                    FUNCTION_IDENT "NodeCausingConcern %lu State: %u\n",
                       __func__,  aliveOId, msgState);
            badNode = NULL;
            goto mod_exit;
        }
    }
    else if (rc == ISMRC_LockNotGranted)
    {
        ieutTRACEL(pThreadData, aliveOId,  ENGINE_INTERESTING_TRACE,
                FUNCTION_IDENT "NodeCausingConcern %lu can't lock\n",
                   __func__,  aliveOId);
        goto mod_exit;
    }
    else
    {
        //Lock Manager not working.
        ieutTRACE_FFDC(ieutPROBE_002, true,
                         "lockmanager lock failed in bad acker removal.", rc
                       , "LockName", &LockName, sizeof(LockName)
                       , "Node", badNode, sizeof(*badNode)
                       , NULL);

    }

    if (badNode != NULL)
    {
        uint32_t newOpsCount = __sync_fetch_and_add(&(Q->freezeHeadCleanupOps), 1);

        ieutTRACEL(pThreadData, aliveOId,  ENGINE_INTERESTING_TRACE,
                FUNCTION_IDENT "Removing non-acking client associated with node %lu on queue %u (%p)! - (Next qmsg will be %lu, Ops in progress %u)\n",
                   __func__,  aliveOId, Q->qId, Q, Q->nextOrderId, newOpsCount);

        iemq_releaseHeadLock(Q);
        headLocked = false;
        void *mdrNode = badNode;

        if (badNode->ackData.pConsumer != NULL)
        {
            //They are connected but we can't safely deference that pointer
            //(we don't have any lock that would prevent that ack being processed)
            //If we can find the same pointer in the waiter list whilst we have it locked, we can deference then!
            int os_rc = pthread_rwlock_rdlock(&(Q->waiterListLock));
            if (UNLIKELY(os_rc != 0))
            {
                ieutTRACE_FFDC( ieutPROBE_001, true, "Unable to take waiterListLock", ISMRC_Error
                        , "Q", Q, sizeof(*Q)
                        , "os_rc", &os_rc, sizeof(os_rc)
                        , "InternalName", Q->InternalName, sizeof(Q->InternalName)
                        , NULL);
            }
            waiterListLocked = true;

            ismEngine_Consumer_t *pConsumerToLookFor = badNode->ackData.pConsumer;

            if (  pConsumerToLookFor != NULL  )
            {
                ismEngine_Consumer_t *pFirstConsumer = Q->firstWaiter;
                ismEngine_Consumer_t *pConsumer = pFirstConsumer;
                do
                {
                    if (pConsumer == pConsumerToLookFor)
                    {
                        char *clientIdToCopy = pConsumer->pSession->pClient->pClientId;

                        if (strlen(clientIdToCopy) < sizeof(clientIdBuffer))
                        {
                            strcpy(clientIdBuffer, clientIdToCopy);
                            nonAckingClientId = clientIdBuffer;
                        }
                        else
                        {
                            iere_primeThreadCache(pThreadData, resourceSet);
                            nonAckingClientId = (char *)iere_malloc(pThreadData,
                                                                    resourceSet,
                                                                    IEMEM_PROBE(iemem_callbackContext, 16),
                                                                    1 + strlen(clientIdToCopy));
                            if (nonAckingClientId != NULL)
                            {
                                nonAckingClientIdAlloced = true;
                                strcpy(nonAckingClientId, clientIdToCopy);
                            }
                            else
                            {
                                rc = ISMRC_AllocateError;
                                goto mod_exit;
                            }
                        }

                        break;
                    }
                    pConsumer = pConsumer->iemqPNext;
                }
                while (pConsumer != pFirstConsumer);
            }
            pthread_rwlock_unlock(&(Q->waiterListLock));
            waiterListLocked = false;
        }

        if (Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY)
        {
            //Single consumer queue we don't need to tell the client layer
            //to look for the node... whoever owns the queue is the guilty party.
            mdrNode = NULL;
        }

        doingRemove = iecs_discardClientForUnreleasedMessageDeliveryReference( pThreadData
                                                                             , (ismQHandle_t)Q
                                                                             , mdrNode
                                                                             , nonAckingClientId);

        DEBUG_ONLY uint32_t preDecrementOpsCount = __sync_fetch_and_sub(&(Q->freezeHeadCleanupOps), 1);
        assert(preDecrementOpsCount > 0);
    }

mod_exit:
    if (waiterListLocked)
    {
        pthread_rwlock_unlock(&(Q->waiterListLock));
    }
    if (headLocked)
    {
        iemq_releaseHeadLock(Q);
    }

    if (nonAckingClientId != NULL && nonAckingClientIdAlloced)
    {
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_free(pThreadData, resourceSet, iemem_callbackContext, nonAckingClientId);
    }

    return doingRemove;
}


//Taking the headlock is an inline function....wrapper it so it can be called
//from export.
void iemq_takeReadHeadLock_ext(iemqQueue_t *Q)
{
    iemq_takeReadHeadLock(Q);
}

//Releasing the headlock is an inline function....wrapper it so it can be called
//from export.
void iemq_releaseHeadLock_ext(iemqQueue_t *Q)
{
    iemq_releaseHeadLock(Q);
}

//Inline function, wrapper for export
iemqQNode_t *iemq_getSubsequentNode_ext( iemqQueue_t *Q
                                       , iemqQNode_t *currPos)
{
    return iemq_getSubsequentNode(Q, currPos);
}


int32_t iemq_getConsumerStats( ieutThreadData_t *pThreadData
                             , ismQHandle_t Qhdl
                             , iempMemPoolHandle_t memPoolHdl
                             , size_t *pNumConsumers
                             , ieqConsumerStats_t consDataArray[])
{
    int32_t rc = OK;
    iemqQueue_t *Q = (iemqQueue_t *)Qhdl;
    bool waiterListLocked = false;

    int32_t os_rc = pthread_rwlock_rdlock(&(Q->waiterListLock));
    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "Unable to take waiterListLock", ISMRC_Error
                , "Q", Q, sizeof(*Q)
                , "os_rc", &os_rc, sizeof(os_rc)
                , "InternalName", Q->InternalName, sizeof(Q->InternalName)
                , NULL);
    }
    waiterListLocked = true;

    ismEngine_Consumer_t *firstWaiter = Q->firstWaiter;

    uint64_t numConsumers = 0;

    //Count the consumers
    if (firstWaiter != NULL)
    {
        ismEngine_Consumer_t *waiter = firstWaiter;
        do
        {
            //Consumer can't go away we have the list locked!
            numConsumers++;
            waiter = waiter->iemqPNext;
        }
        while (waiter != firstWaiter);
    }

    if (numConsumers > *pNumConsumers)
    {
        //Not enough space to write data for all the consumers
        //Say how many we have an get out of here
        *pNumConsumers = numConsumers;
        rc = ISMRC_TooManyConsumers;
        goto mod_exit;
    }

    if (firstWaiter != NULL)
    {
        ismEngine_Consumer_t *waiter = firstWaiter;
        uint64_t consumerPos = 0;

        do
        {
            //Consumer can't go away we still have the list locked!
            memset(&consDataArray[consumerPos], 0, sizeof(consDataArray[consumerPos]));

            char *pClientId = waiter->pSession->pClient->pClientId;
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

            consDataArray[consumerPos].clientId  = clientIdBuf;
            consDataArray[consumerPos].messagesInFlightToConsumer = iemq_getConsumerAckCount( Q, waiter);
            consDataArray[consumerPos].consumerState = waiter->iemqWaiterStatus;
            consDataArray[consumerPos].sessionDeliveryStarted  = waiter->pSession->fIsDeliveryStarted;
            consDataArray[consumerPos].sessionDeliveryStopping = waiter->pSession->fIsDeliveryStopping;
            
            //Get the stats we may need to take locks for.
            if (waiter->fShortDeliveryIds)
            {
                if (waiter->hMsgDelInfo)
                {
                    bool mqttIdsExhausted             = false;
                    uint32_t messagesInFlightToClient = 0;
                    uint32_t maxInflightToClient      = 0;
                    uint32_t inflightReenable         = 0;

                    iecs_getDeliveryStats( pThreadData
                                         , waiter->hMsgDelInfo
                                         , &mqttIdsExhausted
                                         , &messagesInFlightToClient
                                         , &maxInflightToClient
                                         , &inflightReenable);

                    consDataArray[consumerPos].mqttIdsExhausted         = mqttIdsExhausted;
                    consDataArray[consumerPos].messagesInFlightToClient = messagesInFlightToClient;
                    consDataArray[consumerPos].maxInflightToClient      = maxInflightToClient;
                    consDataArray[consumerPos].inflightReenable         = inflightReenable;
                }
            }
            
            waiter = waiter->iemqPNext;
            consumerPos++;
        }
        while (waiter != firstWaiter);

        assert(consumerPos == numConsumers);
    }

    //Record how many we are returning
    *pNumConsumers = numConsumers;

mod_exit:
    if (waiterListLocked)
    {
        pthread_rwlock_unlock(&(Q->waiterListLock));
    }
    return rc;
}
/// @}
/*********************************************************************/
/*                                                                   */
/* End of MultiConsumerQ.c                                           */
/*                                                                   */
/*********************************************************************/
