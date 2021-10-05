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
/// @file multiConsumerQInternal.h
/// @brief Internal data structure for the intermediate queue - (externals @see intermediateQ.h)
//*********************************************************************
#ifndef __ISM_ENGINE_MULTICONSUMERQINTERNAL_DEFINED
#define __ISM_ENGINE_MULTICONSUMERQINTERNAL_DEFINED

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>


#include "engineCommon.h"
#include "engine.h"
#include "msgCommon.h"
#include "queueCommonInternal.h"
#include "ackList.h"
#include "transaction.h"
#include "clientState.h"

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Node structure containing details of the message on the queue
///  @remark 
///    The iemqNode contains details about a message on the queue.
///////////////////////////////////////////////////////////////////////////////

typedef struct tag_iemqQNode_t
{
   ismMessageState_t               msgState;          ///< State of message
   ismMessageState_t               rehydratedState;   ///< State of the message when rehydrated
   uint8_t                         deliveryCount;     ///< Delivery counter
   bool                            hasMDR;            ///< Have we stored an MDR for this node
   uint32_t                        deliveryId;        ///< Per client unique delivery id if one has been assigned (MQTT only)
   uint8_t                         msgFlags;          ///< Message flags
   bool                            inStore;           ///< Persisted in store
   bool                            deleteAckInFlight; ///< We are processing an ack for this node after queue has been deleted
   uint64_t                        orderId;           ///< Order id
   ismEngine_Message_t             *msg;              ///< Pointer to msg
   ismStore_Handle_t               hMsgRef;           ///< Store reference to msg on queue
   ietrSLEHeaderHandle_t           iemqCachedSLEHdr;  ///< Memory to ensure a delivered message can be transactionally acked
   iealAckDetails_t                ackData;           ///< details about a pending ack
} iemqQNode_t;

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
typedef struct tag_iemqQNodePage_t
{
    char                       StrucId[4];            ///< Eyecatcher - 'IPQN'
    ieqNextPageStatus_t       nextStatus;            ///< The status of the next page
    struct tag_iemqQNodePage_t *next;                 ///< Pointer to the next page
    uint32_t                   nodesInPage;           ///< Number of entries in nodes[] array
    iemqQNode_t                nodes[1];              ///< Array of nodes pointers
} iemqQNodePage_t;

///  Eyecatcher for iemqQNodePage_t structure
#define IEMQ_PAGE_STRUCID "IEQP"


//When we try to scan the get cursor forwards, we don't know where we are going to set it
//to yet... so we need commits of puts at /any/ point in the queue to update the getCursor
//So that when we come to record the results of the scan forward we can accurately see whether
//any puts have been committed (or consumes backed out...before the proposed new position)
//Thus the orderId in this "searching" cursor must be higher than for ANY message on the queue
extern iemqCursor_t IEMQ_GETCURSOR_SEARCHING;

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
typedef pthread_spinlock_t iemqPutLock_t;
#else
// Put locks whose type depends on the "UseSpinLocks" config property
typedef union tag_iemqPutLock_t
{
    pthread_mutex_t mutex;
    pthread_spinlock_t spinlock;
} iemqPutLock_t;
#endif


typedef struct tag_iemqWaiterSchedulingInfo_t {
    pthread_spinlock_t schedLock;                ///< Sometimes taken with waiterListLock
    uint32_t capacity;                           ///< Allocated number of slot in schedThreads
    uint32_t maxSlots;                           ///< Number of schedThreads slots available for use - don't want more than numConsumers
    uint32_t slotsInUse;                         ///< Number of slots that are non-zero
    ietrJobThreadId_t schedThreads[0];           ///< Array of threads that are queued to call checkWaiters
} iemqWaiterSchedulingInfo_t;

#define IEMQ_WAITERSCHEDULING_CAPACITY 10

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    The main structure used for intermediate queues.
///  @remark 
///    This structure contains the data for the intermediate Queue.
///    The first 2 fields are common to the tag_ieqQHandle_t strcture.
///    The data is organised into 2 main sections.
///    First is the relatively static data which changes infrequently.
///    Next are the pointers to the list of messages (head, headPage, cursor
///    and cursorPage and tail - each near data that is updated at similar times
///    to the message-pointer
///////////////////////////////////////////////////////////////////////////////
typedef struct tag_iemqQueue_t
{
    ismEngine_Queue_t Common;

    // The following fields are either static or are updated infrequently
    char InternalName[32];                            ///< Internal format queue name
    uint32_t qId;                                     ///< The Qid 
    ieqOptions_t QOptions;                            ///< Options specified on create
    bool ReportedQueueFull;                           ///< Have we reported that the queue is full
    ismStore_Handle_t hStoreObj;                      ///< Definition store handle (QDR or SDR)
    ismStore_Handle_t hStoreProps;                    ///< Properties store handle (QDR or SDR)
    void *QueueRefContext;                            ///< Store Reference context 
    ieqPageMap_t *PageMap;                            ///< Node Pages Map
    bool ackListsUpdating;                            ///< When the consumer goes away, do we need to auto-nack his messages
                                                      ///  (for a single-subscriber, non-durable sub, we won't as we'll throw everything away)
    volatile bool isDeleted;                          ///< Have we started the deletion of the queue
    volatile uint8_t fullDeliveryPrevention;             ///< If non-zero, queue is too full for message delivery
    bool deletionRemovesStoreObjects;                 ///< When we complete deletion, should we remove it from the store
    bool deletionCompleted;                           ///< Has completeDeletion been called for this queue
    uint32_t freezeHeadCleanupOps;                    ///< How many things are in progress that prevent us freeing pages from the queue

    //  Queue Head - Pointers to the oldest message on the queue
    // To access these values then the 'headlock' spinlock must be owned
    pthread_rwlock_t   headlock CACHELINE_ALIGNED;    ///< lock protecting head
    iemqQNodePage_t *headPage;                        ///< pointer to head page
    uint64_t deletedStoreRefCount;                    ///< Counter of deleted store references


    ///set when a put is completed or a get rolled back to indicate the get cursor may
    ///need to be reset... set via CAS
    volatile iemqCursor_t getCursor CACHELINE_ALIGNED;

    // Queue Tail - Pointers to the next free node at the end of the queue
    // To access these values then the 'putlock' spinlock must be owned
    iemqPutLock_t     putlock  CACHELINE_ALIGNED;     ///< lock protecting tail
    iemqQNode_t *tail;                                ///< pointer to tail node
    uint64_t nextOrderId;                             ///< Next Order id

    pthread_rwlock_t waiterListLock CACHELINE_ALIGNED;  ///< lock protecting list of waiters
    ismEngine_Consumer_t *firstWaiter;                  ///< first item in list of waiters
    uint32_t           numBrowsingWaiters;              ///< Number of browsers in the list of waiters (write waiterlistlock)
    uint32_t           numSelectingWaiters;             ///< Number of destructive getters using message selection in the list of waiters (write waiterlistlock)
    volatile uint64_t  checkWaitersVal;                 ///< Increased on each call to checkWaiters() if browsing or selecting if >0
    pthread_mutex_t    getlock;                         ///< lock protecting getCursor
    ielmLockScopeHandle_t selectionLockScope;           ///< Temporary lockscope used when selecting a message
    volatile uint64_t scanOrderId;                      ///orderId an item on page that getter is scanning

    iemqWaiterSchedulingInfo_t *schedInfo;            ///< Info about which threads are planning to call discard/checkWaiters
                                                      ///< Work might be scheduled as an extra phase in a transaction or as a standalone
                                                      ///  (but would need to split this if they were going to do different things)

    volatile uint64_t preDeleteCount;                 ///< number of things that must be resolved before a delete can be completed
                                                      ///  These operations are:
                                                      ///     1) +1 per transactional point-to-point put operations inflight
                                                      ///     2) +1 per transactional point-to-point or pubsub put operations
                                                      ///           inflight from a rehydrated transaction
                                                      ///     3) +1 for each consumer connected to the queue
                                                      ///     4) +1 if the queue is not marked as deleted
                                                      ///     5) +1 per transactional get operation in flight
                                                      ///     6) +1 for each checkWaiters where the caller didn't supply an asyncInfo
    volatile uint64_t enqueueCount;                   ///< number of enqueues (__sync)
    volatile uint64_t dequeueCount;                   ///< number of dequeues (__sync)
    uint64_t qavoidCount;                             ///< number of put's direct to consumer (getLock)
    uint64_t bufferedMsgsHWM;                         ///< Highest Q depth so far (putLock)
    volatile uint64_t rejectedMsgs;                   ///< Rejected message count (__sync)
    volatile uint64_t discardedMsgs;                  ///< Discard old message victim count (__sync)
    volatile uint64_t expiredMsgs;                    ///< expired message count (__sync)
    volatile uint64_t bufferedMsgs CACHELINE_ALIGNED; ///< Current depth inc. inflight msgs (__sync)
    volatile uint64_t bufferedMsgBytes;               ///< Current bytes for buffered msgs (__sync) [remoteServer queues only]
    volatile uint64_t inflightEnqs CACHELINE_ALIGNED; ///< number of infight put's and get's (__sync)
    volatile uint64_t inflightDeqs CACHELINE_ALIGNED; ///< number of infight put's and get's (__sync)
    uint64_t putsAttempted;                           ///< Last calculated value of attempted puts (qavoid+enqueued+rejected)
    volatile uint16_t cleanupScanThdCount;            ///< How many threads are waiting to scan the queue and remove pages
    volatile uint16_t discardingWaitingThdCount;      ///< How many threads are waiting to try and discard messages from the queue
    uint16_t cleanupsNeedingFull;                     ///< How may page cleanups have decided we need a full scan since (last full scan
                                                      ///                                                or cleanup that didn't need it)
} iemqQueue_t;

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iemqQueue_t(__file)\
    iedm_descriptionStart(__file, iemqQueue_t,,"");\
    iedm_describeMember(ismEngine_Queue_t,       Common);\
    iedm_describeMember(char [32],               InternalName);\
    iedm_describeMember(uint32_t,                qId);\
    iedm_describeMember(ieqOptions_t,            QOptions);\
    iedm_describeMember(bool,                    ReportedQueueFull);\
    iedm_describeMember(ismStore_Handle_t,       hStoreObj);\
    iedm_describeMember(ismStore_Handle_t,       hStoreProps);\
    iedm_describeMember(void *,                  QueueRefContext);\
    iedm_describeMember(ieqPageMap_t *,          PageMap);\
    iedm_describeMember(bool,                    ackListsUpdating);\
    iedm_describeMember(bool,                    isDeleted);\
    iedm_describeMember(bool,                    deletionRemovesStoreObjects);\
    iedm_describeMember(bool,                    deletionCompleted);\
    iedm_describeMember(pthread_rwlock_t,        headlock);\
    iedm_describeMember(iemqQNodePage_t *,       headPage);\
    iedm_describeMember(uint64_t,                deletedStoreRefCount);\
    iedm_describeMember(iemqCursor_t,            getCursor);\
    iedm_describeMember(iemqPutLock_t,           putlock);\
    iedm_describeMember(iemqQNode_t *,           tail);\
    iedm_describeMember(uint64_t,                nextOrderId);\
    iedm_describeMember(pthread_rwlock_t,        waiterListLock);\
    iedm_describeMember(ismEngine_Consumer_t *,  firstWaiter);\
    iedm_describeMember(uint32_t,                numBrowsingWaiters);\
    iedm_describeMember(uint32_t,                numSelectingWaiters);\
    iedm_describeMember(uint64_t,                checkWaitersVal);\
    iedm_describeMember(pthread_mutex_t,         getlock);\
    iedm_describeMember(ielmLockScopeHandle_t,   selectionLockScope);\
    iedm_describeMember(uint64_t,                scanOrderId);\
    iedm_describeMember(uint64_t,                preDeleteCount);\
    iedm_describeMember(uint64_t,                enqueueCount);\
    iedm_describeMember(uint64_t,                dequeueCount);\
    iedm_describeMember(uint64_t,                qavoidCount);\
    iedm_describeMember(uint64_t,                bufferedMsgsHWM);\
    iedm_describeMember(uint64_t,                rejectedMsgs);\
    iedm_describeMember(uint64_t,                discardedMsgs);\
    iedm_describeMember(uint64_t,                expiredMsgs);\
    iedm_describeMember(uint64_t,                bufferedMsgs);\
    iedm_describeMember(uint64_t,                bufferedMsgBytes);\
    iedm_describeMember(uint64_t,                inflightEnqs);\
    iedm_describeMember(uint64_t,                inflightDeqs);\
    iedm_describeMember(uint64_t,                putsAttempted);\
    iedm_descriptionEnd;

////////////////////////////////////////////////////////////////////////////////
// for a single consumer queue we just CAS the schedInfo rather than allocate an
// array and point to it from the queue... Here are the two values we CAS
#define IEMQ_SINGLE_CONSUMER_NO_WORK       0x0L
#define IEMQ_SINGLE_CONSUMER_WORKSCHEDULED 0x1L

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    The Softlog entry for PUT
///  @remark 
///    This structure contains the softlog entry for a put message
///    request.
///////////////////////////////////////////////////////////////////////////////
typedef struct iemqSLEPut_t
{
    ietrStoreTranRef_t    TranRef;
    iemqQueue_t          *pQueue;
    iemqQNode_t          *pNode;
    ismEngine_Message_t  *pMsg;
    ietrJobThreadId_t     scheduledThread;
    bool                  bInStore;
    bool                  bCleanHead; ///< Should we clean the head page after rollback (set during rollback)
    bool                  bSavepointRolledback; ///<Did this entry get undone (turns commit+postcommit->rollback+postrollback)
} iemqSLEPut_t;

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    The Softlog entry for ConfirmMessageDelivery(consume)
///  @remark
///    This structure contains the softlog entry for a consume message
///    request.
///////////////////////////////////////////////////////////////////////////////
typedef struct iemqSLEConsume_t
{
    ietrStoreTranRef_t    TranRef;
    iemqQueue_t          *pQueue;
    iemqQNode_t          *pNode;
    ismEngine_Session_t  *pSession;
    ismEngine_Message_t  *pMsg;
    bool                  bInStore;
    bool                  bCleanHead; ///< Should we clean the head page after commit (set during commit)
    ielmLockRequestHandle_t hCachedLockRequest; ///< Not strictly part of the SLE...but we hang enough memory here to
                                                ///< do the lock request in acknowledge
} iemqSLEConsume_t;


typedef struct tag_iemqAsyncMessageDeliveryInfoPerNode_t {
    iemqQNode_t         *node;
    ismMessageState_t    origMsgState;
} iemqAsyncMessageDeliveryInfoPerNode_t;

#define IEMQ_MAXDELIVERYBATCH_SIZE 2048
typedef struct tag_iemqAsyncMessageDeliveryInfo_t {
    char                   StructId[4];
    iemqQueue_t           *Q;
    ismEngine_Consumer_t  *pConsumer;
    ieutThreadData_t      *pJobThread;   //If set we'll schedule the work on this thread after async callback completes
    bool                   consumerLocked;
    uint32_t               usedNodes;
    uint32_t               firstCancelledNode; //If delivery of batch doesn't complete, this is first that
                                               //needs "undelivery"
    uint32_t               firstDeliverNode;   //If delivery of a batch goes async, this is the first node to start from
    iemqAsyncMessageDeliveryInfoPerNode_t perNodeInfo[IEMQ_MAXDELIVERYBATCH_SIZE];
    //Adding things below perNodeInfo should be done with care - when this structure
    //Is copied, often the copying stops partway through perNodeInfo (based on the usedNodes count)
} iemqAsyncMessageDeliveryInfo_t;

#define IEMQ_ASYNCMESSAGEDELIVERYINFO_STRUCID "IMDI"

//We reuse the same structure (iemqAsyncMessageDeliveryInfo_t) when
//we are cancelling delivery... but then we change the structid to:
#define IEMQ_ASYNCMESSAGEDELIVERYINFO_CANCEL_STRUCID "IMCD"


/// @brief track info about messages we find we need to consume during rehydration
typedef struct tag_iemqQConsumedNodeInfo_t
{
   struct tag_iemqQConsumedNodeInfo_t *pNext;         ///< Next msg to remove
   iemqQueue_t                     *pQueue;           ///< Pointer to the queue the message was on
   uint64_t                        orderId;           ///< oId on the queue
   ismEngine_Message_t             *msg;              ///< Pointer to msg
   ismStore_Handle_t               hMsgRef;           ///< Store reference to msg on queue
} iemqQConsumedNodeInfo_t;

/// @brief For Rescheduling a deliver from Async thread back to initiating thread
typedef struct tag_iemqJobDiscardExpiryCheckWaiterData_t
{
    char                   StructId[4];
    iemqQueue_t           *Q;
    uint64_t               asyncId;
    ieutThreadData_t      *pJobThread;
} iemqJobDiscardExpiryCheckWaiterData_t;

#define IEMQ_JOBDISCARDEXPIRYCHECKWAITER_STRUCID "IMCD"

//Taking the headlock is an inline function....wrapper it so it can be called
//from export.
void iemq_takeReadHeadLock_ext(iemqQueue_t *Q);

//Releasing the headlock is an inline function....wrapper it so it can be called
//from export.
void iemq_releaseHeadLock_ext(iemqQueue_t *Q);

//Inline function, wrapper for export
iemqQNode_t *iemq_getSubsequentNode_ext( iemqQueue_t *Q
                                       , iemqQNode_t *currPos);

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
                        , iemqQNode_t **ppNode );

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_MULTICONSUMERQINTERNAL_DEFINED */

/*********************************************************************/
/* End of intermediateQInternal.h                                    */
/*********************************************************************/
