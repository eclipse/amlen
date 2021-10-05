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
/// @file  clientStateInternal.h
/// @brief Engine component client-state internal header file
//*********************************************************************
#ifndef __ISM_CLIENTSTATEINTERNAL_DEFINED
#define __ISM_CLIENTSTATEINTERNAL_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include "engineCommon.h"     /* Engine common internal header file  */
#include "engineInternal.h"   /* Engine internal header file         */

/*********************************************************************/
/*                                                                   */
/* TYPE DEFINITIONS                                                  */
/*                                                                   */
/*********************************************************************/

#define iecsHASH_TABLE_SHIFT_INITIAL   13   ///< Initial number of chains in a table is 2^13 (8192)
#define iecsHASH_TABLE_SHIFT_MAX       22   ///< Maximum number of chains in the table is 2^22 (4194304)
#define iecsHASH_TABLE_SHIFT_FACTOR    3    ///< The table grows by multiplying the chain count by 2^3 (8)
#define iecsHASH_TABLE_CHAIN_INCREMENT 8    ///< Grow the chain by this many entries when full
#define iecsHASH_TABLE_LOADING_LIMIT   8    ///< Grow the table when the number of entries is this much bigger than the number of chains

// The following was arrived at by calculating the amount allocated for
// a client state and wasted by unused memory in a chunk when the mqtt
// msgId range is set to 4.
//
// sizeof *iecsMessageDeliveryReference_t = 8
// sizeof iecsMessageDeliveryReference_t  = 72
//
// Minimum = 4   : ARRAY ((65535/4)+1)*8   + WASTE ((4-4)*72)*2   = 128k
// Minimum = 8   : ARRAY ((65535/8)+1)*8   + WASTE ((8-4)*72)*2   = ~64k
// Minimum = 16  : ARRAY ((65535/16)+1)*8  + WASTE ((16-4)*72)*2  = ~33k
// Minimum = 32  : ARRAY ((65535/32)+1)*8  + WASTE ((32-4)*72)*2  = ~20k
// Minimum = 64  : ARRAY ((65535/64)+1)*8  + WASTE ((64-4)*72)*2  = ~16k
// Minimum = 128 : ARRAY ((65535/128)+1)*8 + WASTE ((128-4)*72)*2 = ~22k
//
// So the 'sweet spot' is 64 chunks (with the current structure size)

#define iecsMINIMUM_MDR_CHUNK_SIZE     64   ///< The minimum below which MDR Chunk sizes will not drop

//****************************************************************************
/// @brief An entry in a client-state hash table chain
//****************************************************************************
typedef struct iecsHashEntry_t
{
    ismEngine_ClientState_t        *pValue;                      ///< The client-state in the table
    uint32_t                        Hash;                        ///< The hash value for the client-state's name
} iecsHashEntry_t;


//****************************************************************************
/// @brief A hash chain in a client-state hash table
//****************************************************************************
typedef struct
{
    uint32_t                        Count;                       ///< Count of entries in the chain
    uint32_t                        Limit;                       ///< Limit for the number of entries in the chain
    iecsHashEntry_t                *pEntries;                    ///< Ptr to array of entries
} iecsHashChain_t;


//****************************************************************************
/// @brief A client-state hash table
//****************************************************************************
typedef struct iecsHashTable_t
{
    char                            StrucId[4];                  ///< Eyecatcher "ECST"
    uint32_t                        Generation;                  ///< Generation number for the current table
    uint32_t                        TotalEntryCount;             ///< Count of entries in the table
    uint32_t                        ChainCount;                  ///< Number of hash chains
    uint32_t                        ChainMask;                   ///< Mask with hash value to calculate hash chain
    uint32_t                        ChainCountMax;               ///< Maximum number of hash chains the table can grow to
    bool                            fCanResize;                  ///< Whether table can resize further
    iecsHashChain_t                *pChains;                     ///< Ptr to array of chains
} iecsHashTable_t;

#define iecsHASH_TABLE_STRUCID "ECST"

//***************************************************************************
/// @brief "Inflight Destination"
// Once we've unsubscribed from an MQTT destination we need to keep a pointer
// to it (as it's no longer in the topic tree) as we need to keep it around
// whilst acks can still arrive for it
//***************************************************************************
typedef struct iecsInflightDestination_t
{
    char                              StrucId[4];                  ///< Eyecatcher "ECID"
    struct iecsInflightDestination_t  *next;                       ///< Next in the chain
    ismQHandle_t                      inflightContainer;           ///< Container of messages that are inflight
} iecsInflightDestination_t;

#define iecsINFLIGHT_DESTINATION_STRUCID "ECID"

//*********************************************************************
// MESSAGE DELIVERY REFERENCES
// Up to 65536 arranged as an array of pointers to chunks. All of this data
// is allocated lazily since the number of active MDRs will be small.
//*********************************************************************

//*********************************************************************
/// @brief  Message Delivery Reference
///
/// Represents the information about a Message-Delivery Reference.
//*********************************************************************
typedef struct iecsMessageDeliveryReference_t
{
    bool                            fSlotInUse;                  ///< Whether this slot is in use
    bool                            fSlotPending;                ///< Whether this slot is pending
    bool                            fHandleIsPointer;            ///< Whether the owning record's handle is just a pointer
    uint8_t                         MsgDeliveryRefState;         ///< State of the Message Delivery Reference in the Store
    uint32_t                        DeliveryId;                  ///< Delivery ID used in the protocol
    ismStore_Handle_t               hStoreMsgDeliveryRef1;       ///< Store handle for first Message Delivery Reference
    uint64_t                        MsgDeliveryRef1OrderId;      ///< Order id for first Message Delivery Reference
    ismStore_Handle_t               hStoreMsgDeliveryRef2;       ///< Store handle for second Message Delivery Reference
    uint64_t                        MsgDeliveryRef2OrderId;      ///< Order id for second Message Delivery Reference
    ismStore_Handle_t               hStoreRecord;                ///< Store handle for owning Queue/Subscription Definition Record
    ismStore_Handle_t               hStoreMessage;               ///< Store handle for Message Reference
    ismQHandle_t                    hQueue;                      ///< Memory Handle for (i.e. pointer to) owning Queue/Subscription
    void *                          hNode;                       ///< Memory Handle for (i.e. pointer to) node on queue
} iecsMessageDeliveryReference_t;


//*********************************************************************
/// @brief  Message Delivery Chunk
///
/// A chunk of MDRs.
//*********************************************************************
typedef struct iecsMessageDeliveryChunk_t
{
    uint32_t slotsInUse;                                         ///< Number of the slots storing active orderIds
    iecsMessageDeliveryReference_t  Slot[0];                     ///< Array of information for a chunk of MDRs, will be iecsChunkSize entries
} iecsMessageDeliveryChunk_t;


//*********************************************************************
/// @brief  Message Delivery Information
///
/// Represents the message-delivery information for a client-state. This is
/// used to maintain durable message delivery IDs for protocols that
/// require this, such as MQTT (QoS 1 and 2).
//*********************************************************************
typedef struct iecsMessageDeliveryInfo_t
{
    char                            StrucId[4];                  ///< Eyecatcher "EMDR"
    uint32_t                        UseCount;                    ///< Number of uses of this structure - must not deallocate when > 0
    uint32_t                        MdrsBelowTarget;             ///< Number of MDRs with order id less than target
    uint32_t                        MdrsAboveTarget;             ///< Number of MDRs with order id greater than or equal to target
    uint64_t                        TargetMinimumActiveOrderId;  ///< Target value for minimum active order id
    uint64_t                        NextOrderId;                 ///< Next order id to use for MDRs
    uint32_t                        ChunksInUse;                 ///< Number of chunks actually allocated
    uint32_t                        NumDeliveryIds;              ///< Number of assigned delivery ids in flight
    uint32_t                        BaseDeliveryId;              ///< Lowest permissible value for delivery ID
    uint32_t                        NextDeliveryId;              ///< Next delivery ID to allocate
    uint32_t                        MaxDeliveryId;               ///< Highest permissible value for delivery ID
    //////Start of  params set based on how many messages are allowed in flight
    uint32_t                        MaxInflightMsgs;             ///< How many messages are allowed inflight per client
    uint32_t                        MdrChunkSize;                ///< Allocate memory for this many MDRs in one chunk
    uint32_t                        MdrChunkCount;               ///< How many chunks are needed for the whole range of delivery ids
    uint32_t                        InflightReenable;            ///< Once inflight messages reaches max, we let it drop to this before reenabling
    //////End of  params set based on how many messages are allowed in flight
    bool                            fIdsExhausted;               ///< If we run out of delivery ids we stop the whole client (and set this to true)
    pthread_mutex_t                 Mutex;                       ///< Mutex synchronising access to this information
    ismEngine_ClientStateHandle_t   diagnosticOwner;             ///< For diagnostic purposes, the owning client (cannot RELIABLY be dereferenced)
    ismStore_Handle_t               hStoreCSR;                   ///< Store handle for Client State Record
    void                           *hMsgDeliveryRefContext;      ///< Store context for managing Message Delivery References
    iecsMessageDeliveryChunk_t     *pFreeChunk1;                 ///< Spare chunk allocated but not in use
    iecsMessageDeliveryChunk_t     *pFreeChunk2;                 ///< Spare chunk allocated but not in use
    iereResourceSetHandle_t         resourceSet;                 ///< The resource set to which the owning clientState belongs (if any)
    iecsMessageDeliveryChunk_t     *pChunk[0];                   ///< Array of message delivery information, will be iecsChunkCount entries
} iecsMessageDeliveryInfo_t;

#define iecsMESSAGE_DELIVERY_INFO_STRUCID "EMDR"

///< The number of order ids we wait for before updating our view of minimum active order id.
///< Historically for efficiency, we did not update minimum active order id very often (the
///< value was 10,000) but the store has changed design, so we can update the value often.
#define iecsMDR_ORDER_ID_TARGET_GAP 1

//*********************************************************************
/// @brief  Callback / Steal context attached to a PROTOCOL_ID_ENGINE clientState
//*********************************************************************
typedef struct tag_iecsEngineClientContext_t
{
    char StrucId[4];                          ///< Eyecatcher "IECC"
    int32_t reason;                           ///< Reason (ismrc) for this clientState
    char *pClientId;                          ///< ClientId of the clientState
    ismEngine_ClientStateHandle_t hClient;    ///< Handle of the client (once created)
    uint32_t options;                         ///< Options specified when client was created
} iecsEngineClientContext_t;

#define iecsENGINE_CLIENT_CONTEXT_STRUCID "IECC"

//*********************************************************************
/// @brief  Context used when completing clientState recovery
//*********************************************************************
typedef struct tag_iecsRecoveryCompletionContext_t
{
    volatile uint32_t remainingActions; // How many asynchronous actions we are waiting for
} iecsRecoveryCompletionContext_t;

//*********************************************************************
/// @brief  Options that can be specified on call to iecs_cleanupRemainingResources
//*********************************************************************
typedef enum tag_iecsCleanupOptions_t
{
    iecsCleanup_None                   = 0x00000000,
    iecsCleanup_PublishWillMsg         = 0x00000001,
    iecsCleanup_Subs                   = 0x00000002,
    iecsCleanup_Include_DurableSubs    = 0x00000004
} iecsCleanupOptions_t;

/*********************************************************************/
/*                                                                   */
/* FUNCTION PROTOTYPES                                               */
/*                                                                   */
/*********************************************************************/
void iecs_freeClientStateTable(ieutThreadData_t *pThreadData,
                               iecsHashTable_t *pTable,
                               bool fFreeClientStates);

int32_t iecs_resizeClientStateTable(ieutThreadData_t *pThreadData,
                                    iecsHashTable_t *pOldTable,
                                    iecsHashTable_t **ppNewTable);

void iecs_completeReleaseClientState(ieutThreadData_t *pThreadData,
                                     ismEngine_ClientState_t *pClient,
                                     bool fInline,
                                     bool fInlineThief);

void iecs_completeUnstoreClientState(ieutThreadData_t *pThreadData,
                                     ismEngine_ClientState_t *pClient,
                                     bool fInline,
                                     bool fInlineThief);

void iecs_completeUnstoreZombieClientState(ieutThreadData_t *pThreadData,
                                           ismEngine_ClientState_t *pClient);

int32_t iecs_storeClientState(ieutThreadData_t *pThreadData,
                              ismEngine_ClientState_t *pClient,
                              bool fFromImport,
                              ismEngine_AsyncData_t *asyncData);

void iecs_unstoreZombieClientState(ieutThreadData_t *pThreadData,
                                   ismEngine_ClientState_t *pClient);

int32_t iecs_storeWillMessage(ieutThreadData_t *pThreadData,
                              ismEngine_ClientState_t *pClient,
                              char *pWillTopicName,
                              ismEngine_Message_t *pMessage,
                              uint32_t timeToLive,
                              uint32_t willDelay);

int32_t iecs_unstoreWillMessage(ieutThreadData_t *pThreadData,
                                ismEngine_ClientState_t *pClient);

void iecs_lockUnreleasedDeliveryState(ismEngine_ClientState_t *pClient);

void iecs_unlockUnreleasedDeliveryState(ismEngine_ClientState_t *pClient);

int32_t iecs_newMessageDeliveryInfo(ieutThreadData_t *pThreadData,
                                    ismEngine_ClientState_t *pClient,
                                    iecsMessageDeliveryInfo_t **ppMsgDelInfo,
                                    bool rehydrating);

void iecs_freeMessageDeliveryInfo(ieutThreadData_t *pThreadData,
                                  iecsMessageDeliveryInfo_t *pMsgDelInfo);

void iecs_cleanupMessageDeliveryInfo(ieutThreadData_t *pThreadData,
                                     iecsMessageDeliveryInfo_t *pMsgDelInfo);

int32_t iecs_compareEngineClientStates(ieutThreadData_t *pThreadData,
                                       ismEngine_ClientStateHandle_t hClient1,
                                       ismEngine_ClientStateHandle_t hClient2);

void iecs_setLCTandExpiry(ieutThreadData_t *pThreadData,
                          ismEngine_ClientState_t *pClient,
                          ism_time_t now,
                          ism_time_t *pCheckScheduleTime);

void iecs_updateWillMsgStats(ieutThreadData_t *pThreadData,
                             iereResourceSetHandle_t resourceSet,
                             ismEngine_Message_t *pMessage,
                             int64_t multiplier);

bool iecs_cleanupRemainingResources(ieutThreadData_t *pThreadData,
                                    ismEngine_ClientState_t *pClient,
                                    iecsCleanupOptions_t cleanupOptions,
                                    bool fInline,
                                    bool fInlineThief);

#endif /* __ISM_CLIENTSTATEINTERNAL_DEFINED */

/*********************************************************************/
/* End of clientStateInternal.h                                      */
/*********************************************************************/
