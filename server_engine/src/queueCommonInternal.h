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
///  @file queueCommonInternal.h
///  @brief
///    Common internal structures and itype  for Simple, Intermediate
///    and multiConsumer queues
///////////////////////////////////////////////////////////////////////////////
#ifndef __ISM_ENGINE_QUEUECOMMONINTERNAL_DEFINED
#define __ISM_ENGINE_QUEUECOMMONINTERNAL_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "queueCommon.h" // Engine common externals queue header file

/// Message pointer value when the node does not contain a message.
#define MESSAGE_STATUS_EMPTY    ((void *)NULL)

/// Delivery id when no value has been assigned.
#define ieqDELIVERYID_NULL      0

/// maximum value of the Redelivery count.
#define ieqENGINE_MAX_REDELIVERY_COUNT    63

/// We use this special message status value to indicate that this is
/// the last node in the page. When
#define ieqMESSAGE_STATE_END_OF_PAGE ismMESSAGE_STATE_RESERVED1

// We use this special status value when we are throwing away a message and it
// no longer counts towards the number of messages on the queue (e.g. so we don't keep discarding
// messages unnecessarily) but cannot be removed from the queue (because we can't move the minActiveOrderId
// past it until it is properly deleted)
#define ieqMESSAGE_STATE_DISCARDING  ismMESSAGE_STATE_RESERVED2

/// When the number of subs in the system is above this number we use smaller queue pages.
#define ieqNUMSUBS_HIGH_CAPACITY_THRESHOLD 10000

/// We will update the min active order id in the store whenever it can be increased by at
/// least this much:
#define ieqMINACTIVEORDERID_UPDATE_INTERVAL 1

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Stucture used to form array of pages (during restart only)
///  @remark
///    During restart, as mesages are loaded on the queue, we build
///    an array of each page we add to the queue. This allows each message
///    to be inserted quickly into the correct positio on the queue.
///////////////////////////////////////////////////////////////////////////////
typedef struct tag_ieqPageMapEntry_t
{
    uint64_t InitialOrderId;           ///< First orderId on page
    uint64_t FinalOrderId;             ///< Last orderId on page
    void *pPage;                       ///< Ptr to page
} ieqPageMapEntry_t;

typedef struct ieqPageMap_t
{
    char StrucId[4];                     ///< Eyecatcher - 'IEPM'
    uint32_t ArraySize;                  ///< Size of array
    uint32_t PageCount;                  ///< Number of pages
    uint32_t TranRecoveryCursorIndex;    ///< PageArray index of page containing TranRecoveryCursorOrderId
    uint64_t TranRecoveryCursorOrderId;  ///< OrderId of 1st message available to transactionally ack during recovery
    ieutHashTableHandle_t InflightMsgs;  ///< Hash table of in-flight msg handles to nodes
    ieqPageMapEntry_t PageArray[1];
} ieqPageMap_t;

#define ieqPAGEMAP_STRUCID "IEPM"
#define ieqPAGEMAP_INCREMENT 16
#define ieqPAGEMAP_SIZE(_count) (offsetof(ieqPageMap_t, PageArray) + (sizeof(ieqPageMapEntry_t) * (_count)))


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Structure used to partition the Value field in the queue-message-ref
///////////////////////////////////////////////////////////////////////////////
typedef union ieq_RefValue_t
{
   uint32_t Value;
   struct
   {
       uint32_t MsgFlags : 8;  ///< _Additional_ message header flags for this message reference
       uint32_t Unused : 24;
   } Parts;
} ieqRefValue_t;


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    The status of the next page in the queue
///////////////////////////////////////////////////////////////////////////////
typedef enum tag_ieqNextPageStatus_t
{
    unfinished = 0,  ///< This page of nodes has not been filled up
    failed,          ///< The putter responsible failed to add the page after this one
    repairing,       ///< A putter is trying to repair a failed page
    completed,       ///< The next page has been successful added to the queue and can be readd
    cursor_clear     ///< Intermediate queue only: The get cursor has moved past THIS page
} ieqNextPageStatus_t;

#endif
