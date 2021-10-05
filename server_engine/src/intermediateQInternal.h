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

//*********************************************************************
/// @file intermediateQInternal.h
/// @brief Internal data structure for the intermediate queue - (externals @see intermediateQ.h)
//*********************************************************************

#ifndef __ISM_ENGINE_INTERMEDIATEQINTERNAL_DEFINED
#define __ISM_ENGINE_INTERMEDIATEQINTERNAL_DEFINED

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <pthread.h>


#include "engineCommon.h"
#include "engine.h"
#include "msgCommon.h"
#include "queueCommonInternal.h"
#include "transaction.h"

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Page Sequence number type
///////////////////////////////////////////////////////////////////////////////
typedef uint64_t ieiqPageSeqNo_t;

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Node structure containing details of the message on the queue
///  @remark 
///    The ieiqNode contains details about a message on the queue.
///////////////////////////////////////////////////////////////////////////////
typedef struct tag_ieiqQNode_t
{
   ismMessageState_t               msgState;          ///< State of message
   uint32_t                        deliveryId;        ///< Delivery Id if set
   uint8_t                         deliveryCount;     ///< Delivery counter
   uint8_t                         msgFlags;          ///< Message flags
   bool                            hasMDR;            ///< Has MDR in store
   bool                            inStore;           ///< Persisted in store
   uint64_t                        orderId;           ///< Order id
   ismEngine_Message_t             *msg;              ///< Pointer to msg
   ismStore_Handle_t               hMsgRef;           ///< Store reference to msg on queue
} ieiqQNode_t;

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    A page of nodes which form part of a queue
///  @remark
///    The Queue is made up of a series of pages in a singly-linked list.
///    Each page contains an array of node structures which conatins a pointer
///    to the message as well as some message state.
///    @par
///    The last node entry in the array is used to identify the end of the 
///    array. It does this by the msgState value being set to
///    ieqMESSAGE_STATE_END_OF_PAGE and the msg pointer actually points back
///    to the start of the page.
///    @par
///    Initially there are two pages in the list (for an empty queue).
///    @par
///    As a putter adds an item which causes the tail to cross to the second
///    of these pages, it becomes the responsibility of that putter to add
///    the 3rd page. If the 2nd page fills up before the putter adds the 3rd
///    page other putters wait for him to add it unless he records he has
///    failed to do so.
///    @par
///    This pattern continues, whenever a putter moves the tail to a new page,
///    they must add the next page.
///////////////////////////////////////////////////////////////////////////////
typedef struct tag_ieiqQNodePage_t
{
    char                       StrucId[4];            ///< Eyecatcher - 'IPQN'
    ieqNextPageStatus_t       nextStatus;             ///< The status of the next page
    struct tag_ieiqQNodePage_t *next;                 ///< Pointer to the next page
    uint32_t                   nodesInPage;           ///< Number of entries in nodes[] array
    ieiqPageSeqNo_t            sequenceNo;            ///< PSQN
    ieiqQNode_t                nodes[1];              ///< Array of nodes pointers
} ieiqQNodePage_t;

///  Eyecatcher for ieiqQNodePage_t structure
#define IEIQ_PAGE_STRUCID "IEQP"

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
typedef pthread_spinlock_t ieiqPutLock_t;
#else
// Put locks whose type depends on the "UseSpinLocks" config property
typedef union tag_ieiqPutLock_t
{
    pthread_mutex_t mutex;
    pthread_spinlock_t spinlock;
} ieiqPutLock_t;
#endif

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
typedef pthread_spinlock_t ieiqHeadLock_t;
#else
// Head locks whose type depends on the "UseSpinLocks" config property
typedef union tag_ieiqHeadLock_t
{
    pthread_mutex_t mutex;
    pthread_spinlock_t spinlock;
} ieiqHeadLock_t;
#endif

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    The main structure used for intermediate queues.
///  @remark 
///    This structure contains the data for the intermediate Queue.
///    The first 2 fields are common to the tag_ieqQHandle_t strcture.
///    The data is organised into 2 main sections.
///    First is the relatively static data which changes infrequently.
///    Next are the pointers to the list of messages (head, headPage, cursor
///    and cursorPage and tail each near data that is updated at similar times
///    to the message-pointer
///////////////////////////////////////////////////////////////////////////////
typedef struct tag_ieiqQueue_t
{
    ismEngine_Queue_t Common;

    // The following fields are either static or are updated infrequently
    char InternalName[32];                            ///< Internal format queue name
    uint32_t qId;                                     ///< The Qid 
    ieqOptions_t QOptions;                            ///< Options specified on create
    uint32_t maxInflightDeqs;                         ///< maximum number of messages between AVAILABLE and CONSUMED
    bool Redelivering;                                ///< Are we redelivering msgs
    bool ReportedQueueFull;                           ///< Have we reported that the queue is full
    ismStore_Handle_t hStoreObj;                      ///< Definition store handle (QDR or SDR)
    ismStore_Handle_t hStoreProps;                    ///< Properties store handle (QPR or SPR)
    void *QueueRefContext;                            ///< Store Reference context 
    ieqPageMap_t *PageMap;                            ///< Node Pages Map

    bool isDeleted;                                   ///< Have we started the deletion of the queue
    bool deletionRemovesStoreObjects;                 ///< When we complete deletion, should we remove it from the store
    bool deletionCompleted;                           ///< Has completeDeletion been called for this queue
    bool redeliverOnly;                               ///< Only messages that the publication flow has started for should be delivered.

    ieiqHeadLock_t headlock CACHELINE_ALIGNED;        ///< lock protecting head
    //  Queue Head - Pointers to the oldest message on the queue
    // To access these values then the 'headlock' spinlock must be owned
    ieiqQNode_t *head;                                ///< pointer to head node
    ieiqQNodePage_t *headPage;                        ///< pointer to head page

    // Get Cursor - Pointers to the next undelivered messages on the queue
    // To access these values then the thread must have compare-and-swapped the
    // waiterStatus field to be getting
    ieiqQNode_t *cursor CACHELINE_ALIGNED;            ///< pointer to cursor node
    volatile bool resetCursor;                        ///< reset the get cursor?
    volatile bool postPutWorkScheduled;               ///< Have we scheduled discard/checkWaiters etc on a jobqueue
    uint64_t redeliverCursorOrderId;                  ///< progress of redelivery
    volatile iewsWaiterStatus_t waiterStatus;                  ///< waiter status
    uint64_t deletedStoreRefCount;                    ///< Counter of deleted store references
    ismEngine_Consumer_t *pConsumer;                  ///< Pointer to consumer
    iecsMessageDeliveryInfoHandle_t hMsgDelInfo;      ///< Message-delivery information handle for MDRs

    ieiqPutLock_t putlock CACHELINE_ALIGNED;        ///< lock protecting tail
    // Queue Tail - Pointers to the next free node at the end of the queue
    // To access these values then the 'putlock' spinlock must be owned
    ieiqQNode_t *tail;                                ///< pointer to tail node
    uint64_t nextOrderId;                             ///< Next Order id
    uint64_t enqueueCount;                            ///< number of enqueues (__sync)
    uint64_t dequeueCount;                            ///< number of enqueues (__sync)
    uint64_t qavoidCount;                             ///< number of put's direct to consumer (getLock)
    uint64_t bufferedMsgsHWM;                         ///< Highest Q depth so far (putLock)
    uint64_t rejectedMsgs;                            ///< Rejected message count (__sync)
    uint64_t discardedMsgs;                           ///< Discard old message victim count (__sync)
    uint64_t expiredMsgs;                             ///< expired message count (__sync)
    volatile uint64_t bufferedMsgs CACHELINE_ALIGNED;          ///< Current depth inc. inflight msgs (__sync)
    volatile uint64_t inflightEnqs CACHELINE_ALIGNED;          ///< number of infight put's and get's (__sync)
    volatile uint64_t inflightDeqs CACHELINE_ALIGNED; ///< number of infight put's and get's (__sync)
    uint64_t preDeleteCount;                          ///< number of things that must be resolved before a delete can be completed
                                                      ///  These operations are:
                                                      ///     1) +1 per transactional put operations inflight from a
                                                      ///           rehydrated transaction
                                                      ///     2) +1 if the queue is not marked as deleted
                                                      ///     3) +1 if there is a consumer attached
                                                      ///     4) +1 if there is at least 1 message inflight
                                                      ///     5) +1 for each checkWaiters where the caller didn't supply an asyncInfo
                                                      ///     6) +1 for each batch of messages being destroyed
    uint64_t putsAttempted;                           ///< Last calculated value of attempted puts (qavoid+enqueued+rejected)
} ieiqQueue_t;

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ieiqQueue_t(__file)\
    iedm_descriptionStart(__file, ieiqQueue_t,,"");\
    iedm_describeMember(ismEngine_Queue_t,               Common);\
    iedm_describeMember(char [32],                       InternalName);\
    iedm_describeMember(uint32_t,                        qId);\
    iedm_describeMember(ieqOptions_t,                    QOptions);\
    iedm_describeMember(uint32_t,                        maxInflightDeqs);\
    iedm_describeMember(bool,                            Redelivering);\
    iedm_describeMember(bool,                            ReportedQueueFull);\
    iedm_describeMember(ismStore_Handle_t,               hStoreObj);\
    iedm_describeMember(ismStore_Handle_t,               hStoreProps);\
    iedm_describeMember(void *,                          QueueRefContext);\
    iedm_describeMember(ieqPageMap_t *,                  PageMap);\
    iedm_describeMember(bool,                            isDeleted);\
    iedm_describeMember(bool,                            deletionRemovesStoreObjects);\
    iedm_describeMember(bool,                            deletionCompleted);\
    iedm_describeMember(bool,                            redeliverOnly);\
    iedm_describeMember(ieiqHeadLock_t,                  headlock);\
    iedm_describeMember(ieiqQNode_t *,                   head);\
    iedm_describeMember(iemqQNodePage_t *,               headPage);\
    iedm_describeMember(ieiqQNode_t *,                   cursor);\
    iedm_describeMember(bool,                            resetCursor);\
    iedm_describeMember(uint64_t,                        redeliverCursorOrderId);\
    iedm_describeMember(ieiqWaiterStatus_t,              waiterStatus);\
    iedm_describeMember(uint64_t,                        deletedStoreRefCount);\
    iedm_describeMember(ismEngine_Consumer_t *,          pConsumer);\
    iedm_describeMember(iecsMessageDeliveryInfoHandle_t, hMsgDelInfo);\
    iedm_describeMember(ieiqPutLock_t,                   putlock);\
    iedm_describeMember(ieiqQNode_t *,                   tail);\
    iedm_describeMember(uint64_t,                        nextOrderId);\
    iedm_describeMember(uint64_t,                        enqueueCount);\
    iedm_describeMember(uint64_t,                        dequeueCount);\
    iedm_describeMember(uint64_t,                        qavoidCount);\
    iedm_describeMember(uint64_t,                        bufferedMsgsHWM);\
    iedm_describeMember(uint64_t,                        rejectedMsgs);\
    iedm_describeMember(uint64_t,                        discardedMsgs);\
    iedm_describeMember(uint64_t,                        expiredMsgs);\
    iedm_describeMember(uint64_t,                        bufferedMsgs);\
    iedm_describeMember(uint64_t,                        inflightEnqs);\
    iedm_describeMember(uint64_t,                        inflightDeqs);\
    iedm_describeMember(uint64_t,                        preDeleteCount);\
    iedm_describeMember(uint64_t,                        putsAttempted);\
    iedm_descriptionEnd;

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    The Softlog entry for PUT
///  @remark 
///    This structure contains the softlog entry for a put message
///    request.
///////////////////////////////////////////////////////////////////////////////
typedef struct ieiqSLEPut_t
{
    ietrStoreTranRef_t   TranRef;
    ieiqQueue_t          *pQueue;
    ieiqQNode_t          *pNode;
    ismEngine_Message_t  *pMsg;
    bool                 bInStore;
    bool                 bPageCleanup;
    bool                 bSavepointRolledback;
} ieiqSLEPut_t;

//Getting the headlock is an inline function....wrapper it so it can be called
//from export.
void ieiq_getHeadLock_ext(ieiqQueue_t *Q);

//Releasing the headlock is an inline function....wrapper it so it can be called
//from export.
void ieiq_releaseHeadLock_ext(ieiqQueue_t *Q);

//Must be called with headlock locked. Use for export (amongst other things?)
ieiqQNode_t *ieiq_getNextNodeFromPageEnd( ieutThreadData_t *pThreadData
                                        , ieiqQueue_t *Q
                                        , ieiqQNode_t *currNode);

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
                        , ieiqQNode_t **ppNode);

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_INTERMEDIATEQINTERNAL_DEFINED */

/*********************************************************************/
/* End of intermediateQInternal.h                                    */
/*********************************************************************/
