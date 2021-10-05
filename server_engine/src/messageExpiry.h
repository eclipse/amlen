/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
/// @file  messageExpiry.h
/// @brief Engine component header file with internals of tracking
///        expiry of messages
//*********************************************************************
#ifndef __ISM_MESSAGEEXPIRY_DEFINED
#define __ISM_MESSAGEEXPIRY_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include "engineCommon.h"      /* Engine common internal header file */
#include "engineInternal.h"    /* Engine internal header file        */
#include "engineSplitList.h"   /* Engine Split list handling funcs   */
#include "remoteServers.h"     /* iers_ functions and constants      */
#include "queueCommon.h"
#include "topicTree.h"

/*********************************************************************/
/*                                                                   */
/* TYPE DEFINITIONS                                                  */
/*                                                                   */
/*********************************************************************/

typedef struct tag_iemeExpiryControl_t
{
    ieutSplitList_t *queueReaperList;     ///< List of queues that need message expiry processing
    ieutSplitList_t *topicReaperList;     ///< List of topics that need retained message expiry processing
    ism_threadh_t    reaperThreadHandle;  ///< The thread handle of the reaper thread
    volatile bool    reaperEndRequested;  ///< Whether the reaper thread has been asked to end
    pthread_cond_t   cond_wakeup;         ///< used to wake up the reaper
    pthread_mutex_t  mutex_wakeup;        ///< used to protect cond_wakeup
    uint64_t         numWakeups;          ///< count of times reaper has been woken
    uint64_t         scansStarted;        ///< count of the number of scans that have been started
    uint64_t         scansEnded;          ///< count of the number of scans that have ended
} iemeExpiryControl_t;

//*******************************************************************
/// @brief per queue details of messages with Expiry set
//*******************************************************************
typedef struct tag_iemeBufferedMsgExpiryDetails_t
{
    uint64_t orderId; ///< orderId of the message in the queue
    void *qnode;      ///< qnode of one of the queue types
    uint32_t Expiry;  ///< Expiry time - seconds since 2000-01-01T00
} iemeBufferedMsgExpiryDetails_t;

///Limit of number of messages with expiry on the queue we keep
///explicit pointers to
#define NUM_EARLIEST_MESSAGES 8

//*******************************************************************
/// @brief details of a buffered message that needs to be tracked
///        for use with expiry
//*******************************************************************
typedef struct tag_iemeQueueExpiryData_t
{
    pthread_mutex_t    expiryLock; ///< protects other fields in this structure
    int64_t messagesWithExpiry;    ///< can go negative as count decreased after made available so decrease can happen before increase
    uint32_t messagesInArray;      ///<How many slots in the array are filled with valid messages
    iemeBufferedMsgExpiryDetails_t earliestExpiryMessages[NUM_EARLIEST_MESSAGES];
} iemeQueueExpiryData_t;

//****************************************************************************
/// @brief Context used for topic at-rest retained message expiry callback function
//****************************************************************************
typedef struct tag_iemeExpiryReaperTopicContext_t
{
    uint32_t              nowExpire;                ///< Expiry time to use
    uint32_t              callbackCount;            ///< Number of times callback has been called
    uint32_t              statTopicNoWorkRequired;  ///< Message has not yet expired
    uint32_t              statTopicNoExpiry;        ///< Topic node has no expiry removed from list
    uint32_t              statTopicReaped;          ///< Message has expired and was reaped
    bool                  reapStoppedEarly;         ///< Reap was stopped early for some reason
    iersMemLimit_t        remoteServerMemLimit;     ///< Current memory limit being imposed by remote server component
    iettTopicNode_t     **removedSubtrees;          ///< Subtrees removed when reaping performed
    uint32_t              removedSubtreeCount;      ///< Count of entries in the removedSubtrees array
    uint32_t              removedSubtreesCapacity;  ///< Capacity of the removedSubtrees array
    iettOriginServer_t  **originServers;            ///< Origin servers to report on
    uint32_t              originServerCount;        ///< Count of origin servers to report on
    uint32_t              originServerCapacity;     ///< Capacity of the originServers array
    ismEngine_Message_t **removedMessages;          ///< Messages removed
    uint32_t              removedMessagesCount;     ///< Count of messages removed
    uint32_t              removedMessagesCapacity;  ///< Capacity of the removedMessages array
    volatile bool        *reaperEndRequested;       ///< Has the reaper been requested to end?
    uint32_t              earliestObservedExpiry;   ///< The earliest (unexpired) expiry observed during a scan
} iemeExpiryReaperTopicContext_t;

/// Attempt to batch this many topics together when reaping expired retained messages.
/// Note: There was a 'pop' sound when this number was chosen.
#define iettREAP_TOPIC_MESSAGE_BATCH_SIZE 100

// Amount of time we're willing to wait for timers at shutdown.
#define iettMAXIMUM_SHUTDOWN_TIMEOUT_SECONDS 60

/*********************************************************************/
/*                                                                   */
/* FUNCTION PROTOTYPES                                               */
/*                                                                   */
/*********************************************************************/

//****************************************************************************
/// @brief Create and initialize the structures needed to perform at-rest
///        message expiry.
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieme_initMessageExpiry( ieutThreadData_t *pThreadData );

//****************************************************************************
/// @brief Start the thread(s) to perform at-rest message expiry
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieme_startMessageExpiry( ieutThreadData_t *pThreadData );

//****************************************************************************
/// @brief Stop the threads performing message expiry
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void ieme_stopMessageExpiry( ieutThreadData_t *pThreadData );

//****************************************************************************
/// @brief End the threads and destroy the lists of topics and queues.
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void ieme_destroyMessageExpiry( ieutThreadData_t *pThreadData );

//Cause expiry reaper to scan queues and topics
void ieme_wakeMessageExpiryReaper(ieutThreadData_t *pThreadData);

void ieme_addTopicToExpiryReaperList( ieutThreadData_t *pThreadData
                                    , iettTopicNode_t *pTopicNode);

void ieme_removeTopicFromExpiryReaperList( ieutThreadData_t *pThreadData
                                         , iettTopicNode_t *pTopicNode);

void ieme_removeQueueFromExpiryReaperList( ieutThreadData_t *pThreadData
                                         , ismEngine_Queue_t *pQ);

void ieme_addMessageExpiryData( ieutThreadData_t *pThreadData
                              , ismEngine_Queue_t *pQ
                              , iemeBufferedMsgExpiryDetails_t *msgdata);

void ieme_addMessageExpiryPreLocked( ieutThreadData_t *pThreadData
                                   , ismEngine_Queue_t *pQ
                                   , iemeBufferedMsgExpiryDetails_t *msgdata
                                   , bool alreadyInList);

void ieme_removeMessageExpiryData( ieutThreadData_t *pThreadData
                                 , ismEngine_Queue_t *pQ
                                 , uint64_t orderId);

void ieme_freeQExpiryData( ieutThreadData_t *pThreadData
                         , ismEngine_Queue_t *pQ );

void ieme_clearQExpiryDataPreLocked( ieutThreadData_t *pThreadData
                                   , ismEngine_Queue_t *pQ);

bool ieme_startReaperQExpiryScan( ieutThreadData_t *pThreadData
                                , ismEngine_Queue_t *pQ);

void ieme_endReaperQExpiryScan( ieutThreadData_t *pThreadData
                              , ismEngine_Queue_t *pQ);

void ieme_reapQExpiredMessages( ieutThreadData_t *pThreadData
                              , ismEngine_Queue_t *pQ);

#endif /* __ISM_MESSAGEEXPIRY_DEFINED */
