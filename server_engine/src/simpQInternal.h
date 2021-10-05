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
/// @file simpQInternal.h
/// @brief Internal data structure for the simple queue - (externals @see simpQ.h)
//****************************************************************************
#ifndef __ISM_ENGINE_SIMPQ_DATA_DEFINED
#define __ISM_ENGINE_SIMPQ_DATA_DEFINED

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

/// Message pointer Value in last node in the nodePage array.
#define MESSAGE_STATUS_ENDPAGE  ((void *)-1)

//This message has been removed, skip to the next message (only used in simpQ)
#define MESSAGE_STATUS_REMOVED  ((void *)-2)


/// Queue node
typedef struct tag_iesqQNode_t
{
   uint8_t                         msgFlags;          ///< Message flags
   ismEngine_Message_t             *msg;              ///< Pointer to msg
} iesqQNode_t;


/// The Queue is made up of a series of pages in a singly-linked list.
/// Each page contains an array of pointers to messages.
/// Initially there are two pages in the list (for an empty queue).
///
///As a putter adds an item which causes the tail to cross to the second of
///these pages, it becomes the responsibility of that putter to add the 3rd page
///(if the 2nd page fills up before the putter adds the 3rd page other putters wait
/// for him to add it unless he records he has failed to do so).
///
///This pattern continues, whenever a putter moves the tail to a new page, they must
///add the next page.
typedef struct tag_iesqQNodePage_t
{
    char                       StrucId[4];            /* Eyecatcher - 'IPQN'                */ 
    ieqNextPageStatus_t       nextStatus;            /* The status of the next page        */
    struct tag_iesqQNodePage_t *next;                 /* Pointer to the next page           */
    uint32_t                   nodesInPage;           /* Number of entries in nodes[] array */
    iesqQNode_t                nodes[1];              /* Array of nodes pointers            */
    // uint32_t                nodesInPage2;          /* After the array is a repeat of the */
                                                      /* nodesInPage value                  */
} iesqQNodePage_t;


#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
typedef pthread_spinlock_t iesqPutLock_t;
#else
// Put locks whose type depends on the "UseSpinLocks" config property
typedef union tag_iesqPutLock_t
{
    pthread_mutex_t mutex;
    pthread_spinlock_t spinlock;
} iesqPutLock_t;
#endif

#define IESQ_PAGE_STRUCID "IPQN"

typedef struct tag_iesqQueue_t {
    ismEngine_Queue_t Common;
    ismStore_Handle_t hStoreObj;                      ///< Definition store handle (SDR)
    ismStore_Handle_t hStoreProps;
    ieqOptions_t QOptions;
    uint32_t preDeleteCount;                          ///< number of things that must be resolved before a delete can be completed
                                                      ///<              +1 for not being deleted
                                                      ///<              +1 for a connected consumer
    bool ReportedQueueFull;                           ///< Have we reported that the queue is full
    bool isDeleted;                                   ///< Queue has been marked as deleted
    bool deletionRemovesStoreObjects;                 ///< Just freeing the memory or removing from store as well
    iesqQNode_t *head CACHELINE_ALIGNED;
    iesqQNodePage_t *headPage;
    volatile iewsWaiterStatus_t waiterStatus;
    ismEngine_Consumer_t *pConsumer;
    iesqPutLock_t putlock CACHELINE_ALIGNED;
    iesqQNode_t *tail;
    uint64_t enqueueCount;                            ///< number of enqueues (putLock)
    uint64_t dequeueCount;                            ///< number of dequeues (getLock)
    uint64_t qavoidCount;                             ///< number of put's direct to consumer (getLock)
    uint64_t rejectedMsgs;                            ///< Rejected message count  (__sync)
    uint64_t discardedMsgs;                           ///< Discard old message victim count (__sync)
    uint64_t expiredMsgs;                             ///< expired message count (__sync)
    uint64_t bufferedMsgsHWM;                         ///< Highest Q depth so far (putLock)
    volatile uint64_t bufferedMsgs CACHELINE_ALIGNED;          ///< Current depth inc. inflight msgs (__sync)
    uint64_t putsAttempted;                           ///< Last calculated value of attempted puts (qavoid+enqueued+rejected)
} iesqQueue_t;

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iesqQueue_t(__file)\
    iedm_descriptionStart(__file, iesqQueue_t,,"");\
    iedm_describeMember(ismEngine_Queue_t,       Common);\
    iedm_describeMember(ismStore_Handle_t,       hStoreObj);\
    iedm_describeMember(ismStore_Handle_t,       hStoreProps);\
    iedm_describeMember(ieqOptions_t,            QOptions);\
    iedm_describeMember(bool,                    ReportedQueueFull);\
    iedm_describeMember(bool,                    isDeleted);\
    iedm_describeMember(uint32_t,                preDeleteCount);\
    iedm_describeMember(iesqQNode_t *,           head);\
    iedm_describeMember(iesqQNodePage_t *,       headPage);\
    iedm_describeMember(iewsWaiterStatus_t,      waiterStatus);\
    iedm_describeMember(ismEngine_Consumer_t *,  pConsumer);\
    iedm_describeMember(iesqPutLock_t,           putlock);\
    iedm_describeMember(iesqQNode_t *,           tail);\
    iedm_describeMember(uint64_t,                enqueueCount);\
    iedm_describeMember(uint64_t,                dequeueCount);\
    iedm_describeMember(uint64_t,                qavoidCount);\
    iedm_describeMember(uint64_t,                rejectedMsgs);\
    iedm_describeMember(uint64_t,                bufferedMsgsHWM);\
    iedm_describeMember(uint64_t,                bufferedMsgs);\
    iedm_describeMember(uint64_t,                putsAttempted);\
    iedm_descriptionEnd;

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    The Softlog entry for PUT
///  @remark
///    This structure contains the softlog entry for a put message request.
///////////////////////////////////////////////////////////////////////////////
typedef struct iesqSLEPut_t
{
    iesqQueue_t          *pQueue;
    ismEngine_Message_t  *pMsg;
    ieqPutOptions_t      putOptions;
    bool                 bSavepointRolledback;
} iesqSLEPut_t;

//Allow e.g. export code to chain along the queue
iesqQNode_t *iesq_getNextNodeFromPageEnd(
                ieutThreadData_t *pThreadData,
                iesqQueue_t *Q,
                iesqQNode_t *pPageEndNode);

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_SIMPQ_DATA_DEFINED */

/*********************************************************************/
/* End of simpQData.h                                               */
/*********************************************************************/
