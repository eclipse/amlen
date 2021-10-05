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
/// @file  clientState.h
/// @brief Engine component client-state header file
//*********************************************************************
#ifndef __ISM_CLIENTSTATE_DEFINED
#define __ISM_CLIENTSTATE_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include "engineCommon.h"     /* Engine common internal header file  */
#include "engineInternal.h"   /* Engine internal header file         */
#include "transaction.h"
#include "engineAsync.h"
#include "engineRestoreTable.h"

/*********************************************************************/
/*                                                                   */
/* DATA STRUCTURES                                                   */
/*                                                                   */
/*********************************************************************/

typedef struct tag_iecsNewClientStateInfo_t
{
    const char *pClientId;                       // Used on create, recovery & import
    uint32_t protocolId;                         // Used on create, recovery & import
    iecsDurability_t durability;                 // Used on create, recovery & import
    iereResourceSetHandle_t resourceSet;         // Used on create, recovery & import
    iecsTakeover_t takeover;                     // Used on create
    ismSecurity_t *pSecContext;                  // Used on create
    bool fCleanStart;                            // Used on create
    void *pStealContext;                         // Used on create
    ismEngine_StealCallback_t pStealCallbackFn;  // Used on create
    const char *pUserId;                         // Used on import
    ism_time_t lastConnectedTime;                // Used on import
    uint32_t expiryInterval;                     // Used on import
} iecsNewClientStateInfo_t;

/*********************************************************************/
/*                                                                   */
/* FUNCTION PROTOTYPES                                               */
/*                                                                   */
/*********************************************************************/

// Create the client-state table
int32_t iecs_createClientStateTable(ieutThreadData_t *pThreadData);

// Destroy the client-state table
void iecs_destroyClientStateTable(ieutThreadData_t *pThreadData);

// Callback function used when traversing a hash table
typedef bool (*iecsTraverseCallback_t)(ieutThreadData_t *pThreadData,
                                       ismEngine_ClientState_t *pClient,
                                       void *pContext);

// Traverse the client-state table / a subset of client-state table chains
int32_t iecs_traverseClientStateTable(ieutThreadData_t *pThreadData,
                                      uint32_t *tableGeneration,
                                      uint32_t startChain,
                                      uint32_t maxChains,
                                      uint32_t *lastChain,
                                      iecsTraverseCallback_t callback,
                                      void *context);

// Allocate a new client-state object
int32_t iecs_newClientState(ieutThreadData_t *pThreadData,
                            iecsNewClientStateInfo_t *pInfo,
                            ismEngine_ClientState_t **ppClient);

// Allocate a new client-state object during recovery
int32_t iecs_newClientStateRecovery(ieutThreadData_t *pThreadData,
                                    iecsNewClientStateInfo_t *pInfo,
                                    ismEngine_ClientState_t **ppClient);

// Allocate a new client-state object as part of an import
int32_t iecs_newClientStateImport(ieutThreadData_t *pThreadData,
                                  iecsNewClientStateInfo_t *pInfo,
                                  ismEngine_ClientState_t **ppClient);

// Deallocate a client-state object
void iecs_freeClientState(ieutThreadData_t *pThreadData,
                          ismEngine_ClientState_t *pClient,
                          bool unstore);

// Acquires a reference to a client-state to prevent its deallocation
int32_t iecs_acquireClientStateReference(ismEngine_ClientState_t *pClient);

// Release a reference to a client-state, deallocating the client-state when
// the use-count drops to zero
bool iecs_releaseClientStateReference(ieutThreadData_t *pThreadData,
                                      ismEngine_ClientState_t *pClient,
                                      bool fInline,
                                      bool fInlineThief);

// Adds a client-state to the client-state table, potentially beginning the steal process
int32_t iecs_addClientState(ieutThreadData_t *pThreadData,
                            ismEngine_ClientState_t *pClient,
                            uint32_t options,
                            uint32_t internalOptions,
                            void *pContext,
                            size_t contextLength,
                            ismEngine_CompletionCallback_t pCallbackFn);

int32_t iecs_addClientStateRecovery(ieutThreadData_t *pThreadData,
                                    ismEngine_ClientState_t *pClient);

int32_t iecs_updateClientPropsRecord(ieutThreadData_t *pThreadData,
                                     ismEngine_ClientState_t *pClient,
                                     char *willTopicName,
                                     ismStore_Handle_t willMsgStoreHdl,
                                     uint32_t willMsgTimeToLive,
                                     uint32_t willDelay);

int32_t iecs_updateLastConnectedTime(ieutThreadData_t *pThreadData,
                                     ismEngine_ClientState_t *pClient,
                                     bool fIsConnected,
                                     ismEngine_AsyncData_t *pAsyncData);

void iecs_setExpiryFromLastConnectedTime(ieutThreadData_t *pThreadData,
                                         ismEngine_ClientState_t *pClient,
                                         ism_time_t *pCheckScheduleTime);

// Rehydrate a Client-State Record from the Store during recovery
int32_t iecs_rehydrateClientStateRecord(ieutThreadData_t *pThreadData,
                                        ismStore_Record_t *pStoreRecord,
                                        ismStore_Handle_t hStoreCSR,
                                        ismEngine_ClientState_t **ppClient);

// Rehydrate a Client Properties Record from the Store during recovery
int32_t iecs_rehydrateClientPropertiesRecord(ieutThreadData_t *pThreadData,
                                             ismStore_Record_t *pStoreRecord,
                                             ismStore_Handle_t hStoreCPR,
                                             ismEngine_ClientState_t *pOwningClient,
                                             ismEngine_MessageHandle_t hWillMessage,
                                             ismEngine_ClientState_t **ppClient);

// Complete recovery of all clientStates as listed in the pair of tables
int32_t iecs_completeClientStateRecovery(ieutThreadData_t *pThreadData,
                                         iertTable_t *CSRTable,
                                         iertTable_t *CPRTable,
                                         bool partialRecoveryAllowed);

// Add an unreleased delivery
int32_t iecs_addUnreleasedDelivery(ieutThreadData_t *pThreadData,
                                   ismEngine_ClientState_t *pClient,
                                   ismEngine_Transaction_t *pTran,
                                   uint32_t unrelDeliveryId);

// Import an array of unreleased delivery Ids
int32_t iecs_importUnreleasedDeliveryIds(ieutThreadData_t *pThreadData,
                                         ismEngine_ClientState_t *pClient,
                                         uint32_t *unrelDeliveryIds,
                                         uint32_t unrelDeliveryIdCount,
                                         ismStore_CompletionCallback_t pCallback,
                                         void *pContext);

// Remove an unreleased delivery
int32_t iecs_removeUnreleasedDelivery(ieutThreadData_t *pThreadData,
                                      ismEngine_ClientState_t *pClient,
                                      ismEngine_Transaction_t *pTran,
                                      uint32_t unrelDeliveryId,
                                      ismEngine_AsyncData_t *pAsyncData);

// List unreleased deliveries
int32_t iecs_listUnreleasedDeliveries(ismEngine_ClientState_t *pClient,
                                      void *pContext,
                                      ismEngine_UnreleasedCallback_t pUnrelCallbackFunction);

// Add or replace a will message
int32_t iecs_setWillMessage(ieutThreadData_t *pThreadData,
                            ismEngine_ClientState_t *pClient,
                            const char *pWillTopicName,
                            ismEngine_Message_t *pMessage,
                            uint32_t timeToLive,
                            uint32_t willDelay,
                            void *pContext,
                            size_t contextLength,
                            ismEngine_CompletionCallback_t  pCallbackFn);

// Remove a will message
int32_t iecs_unsetWillMessage(ieutThreadData_t *pThreadData,
                              ismEngine_ClientState_t *pClient);

// Add an Unreleased Message State to the Store and/or transaction
int32_t iecs_storeUnreleasedMessageState(ieutThreadData_t *pThreadData,
                                         ismEngine_ClientState_t *pClient,
                                         ismEngine_Transaction_t *pTran,
                                         uint32_t unrelDeliveryId,
                                         ismEngine_UnreleasedState_t *pUnrelChunk,
                                         int16_t slotNumber,
                                         ismStore_Handle_t *pHStoreUnrel);

// Remove an Unreleased Message State from the Store and/or transaction
int32_t iecs_unstoreUnreleasedMessageState(ieutThreadData_t *pThreadData,
                                           ismEngine_ClientState_t *pClient,
                                           ismEngine_Transaction_t *pTran,
                                           ismEngine_UnreleasedState_t *pUnrelChunk,
                                           int16_t slotNumber,
                                           ismStore_Handle_t hStoreUnrel,
                                           ismEngine_AsyncData_t *pAsyncData);

// Rehydrate an Unreleased Message State from the Store during recovery
int32_t iecs_rehydrateUnreleasedMessageState(ieutThreadData_t *pThreadData,
                                             ismEngine_ClientState_t *pClient,
                                             ismEngine_Transaction_t *pTran,
                                             ietrTranEntryType_t type,
                                             uint32_t unrelDeliveryId,
                                             ismStore_Handle_t hStoreUnrel,
                                             ismEngine_RestartTransactionData_t *pTranData);


// Acquire a reference to the message-delivery information for a client-state
int32_t iecs_acquireMessageDeliveryInfoReference(ieutThreadData_t *pThreadData,
                                                 ismEngine_ClientState_t *pClient,
                                                 iecsMessageDeliveryInfoHandle_t *phMsgDelInfo);

// Release a reference to the message-delivery information for a client-state
void iecs_releaseMessageDeliveryInfoReference(ieutThreadData_t *pThreadData,
                                              iecsMessageDeliveryInfoHandle_t hMsgDelInfo);

// Identify which of an array of ClientIds has the Message Delivery Reference represented by the hQueue and
// hNode pair in its list.
int32_t iecs_identifyMessageDeliveryReferenceOwner(ieutThreadData_t *pThreadData,
                                                   const char **pSuspectClientIds,
                                                   ismQHandle_t hQueue,
                                                   void *hNode,
                                                   const char **pIdentifiedClientId);

typedef enum tag_iecsRelinquishType_t
{
    iecsRELINQUISH_ALL,                              ///> Relinquish all messages regardless of queue
    iecsRELINQUISH_ACK_HIGHRELIABILITY_ON_QUEUE,     ///> Relinquish messages that are on one of the specified queues so they are deleted
    iecsRELINQUISH_NACK_ALL_ON_QUEUE,                ///> Relinquish messages that are on one of the specified queues so they available for other clients
    iecsRELINQUISH_ACK_HIGHRELIABILITY_NOT_ON_QUEUE, ///> Relinquish messages that are NOT on one of the specified queues so they are deleted
    iecsRELINQUISH_NACK_ALL_NOT_ON_QUEUE,            ///> Relinquish messages that are NOT on one of the specified queues so they available for other clients
} iecsRelinquishType_t;

// Relinquish all messages for which we have an MDR associated with particular queues
void iecs_relinquishAllMsgs(ieutThreadData_t *pThreadData,
                            iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
                            ismQHandle_t *hQueues,
                            uint32_t queueCount,
                            iecsRelinquishType_t relinquishType);

// Add a Message Delivery Reference to the Store
int32_t iecs_storeMessageDeliveryReference(ieutThreadData_t *pThreadData,
                                           iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
                                           ismEngine_Session_t *pSession,
                                           ismStore_Handle_t hStoreRecord,
                                           ismQHandle_t hQueue,
                                           void *hNode,
                                           ismStore_Handle_t hStoreMessage,
                                           uint32_t *pDeliveryId,
                                           bool *pfStoredMDR);

// Remove a Message Delivery Reference from the Store
// (can use start & complete variants below to allow the caller to do store commit between them)
int32_t iecs_unstoreMessageDeliveryReference(ieutThreadData_t *pThreadData,
                                             ismEngine_Message_t *pMsg,
                                             iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
                                             uint32_t deliveryId,
                                             uint32_t *pStoreOpCount,
                                             bool *pDeliveryIdsAvailable,
                                             ismEngine_AsyncData_t *asyncInfo);

//Do the pre-commit work required to remove an MDR
//(finish with iecs_completeUnstoreMessageDeliveryReference)
int32_t iecs_startUnstoreMessageDeliveryReference(ieutThreadData_t *pThreadData,
        ismEngine_Message_t *pMsg,
        iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
        uint32_t deliveryId,
        uint32_t *pStoreOpCount);

//Do the post-commit work required to remove an MDR
//(finishes what iecs_startUnstoreMessageDeliveryReference started)
int32_t iecs_completeUnstoreMessageDeliveryReference(ieutThreadData_t *pThreadData,
                                                     ismEngine_Message_t *pMsg,
                                                     iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
                                                     uint32_t deliveryId);

// Rehydrate a Message Delivery Reference from the Store during recovery
int32_t iecs_rehydrateMessageDeliveryReference(ieutThreadData_t *pThreadData,
                                               ismEngine_ClientState_t *pClient,
                                               ismStore_Handle_t hStoreMDR,
                                               ismStore_Reference_t *pMDR);


// Assign the next available message delivery id for a given client
int32_t iecs_assignDeliveryId(ieutThreadData_t *pThreadData,
                              iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
                              ismEngine_Session_t *pSession,
                              ismStore_Handle_t hStoreRecord,
                              ismQHandle_t hQueue,
                              void *hNode,
                              bool fHandleIsPointer,
                              uint32_t *pDeliveryId);

// Record a delivery id for a message is no longer needed
int32_t iecs_releaseDeliveryId(ieutThreadData_t *pThreadData,
                               iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
                               uint32_t deliveryId);

// Unstore all MDRs and release all delivery ids for a given owning record
int32_t iecs_releaseAllMessageDeliveryReferences(ieutThreadData_t *pThreadData,
                                                 iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
                                                 ismStore_Handle_t hStoreOwner,
                                                 bool fHandleIsPointer);

// Locks a client-state
void iecs_lockClientState(ismEngine_ClientState_t *pClient);

// Unlocks a client-state
void iecs_unlockClientState(ismEngine_ClientState_t *pClient);

// Generate the clientId hash value that is used in the clientState table
uint32_t iecs_generateClientIdHash(const char *pKey);

// Associates a transaction with the client state
void iecs_linkTransaction(ieutThreadData_t *pThreadData,
                          ismEngine_ClientState_t *pClient,
                          ismEngine_Transaction_t *pTran);

// Disassociates a transaction with the client state
void iecs_unlinkTransaction(ismEngine_ClientState_t *pClient,
                            ismEngine_Transaction_t *pTran);

// Configuration callback to handle deletion of client-state objects
int iecs_clientStateConfigCallback(ieutThreadData_t *pThreadData,
                                   char *objectIdentifier,
                                   ism_prop_t *changedProps,
                                   ism_ConfigChangeType_t changeType);

//Record that we have run out of deliveryids so future acks try and restart the session...
uint32_t iecs_markDeliveryIdsExhausted( ieutThreadData_t *pThreadData
                                      , iecsMessageDeliveryInfoHandle_t hMsgDelInfo
                                      , ismEngine_Session_t *pSession);

//If we stopped delivery as we ran out of ids and now can turn it on again... this will tell us
bool iecs_canRestartDelivery( ieutThreadData_t *pThreadData
                            , iecsMessageDeliveryInfoHandle_t hMsgDelInfo);

//Get stats from an hMsgDelInfo
void iecs_getDeliveryStats( ieutThreadData_t *pThreadData
                           , iecsMessageDeliveryInfoHandle_t hMsgDelInfo
                           , bool *pfIdsExhausted
                           , uint32_t *pInflightMessages
                           , uint32_t *pInflightMax
                           , uint32_t *pInflightReenable);

//For some protocol (of the ones we implement: MQTT) we need to complete
//delivery of inflight messages even if they unsub. This call tells the
//client this queue may need to be kept alive because of it and to keep a
//pointer to it so it can notify the queue if it doesn't need the inflight
//msgs any more (e.g. because it has been deleted).
void iecs_trackInflightMsgs(ieutThreadData_t *pThreadData,
                            ismEngine_ClientState_t *pClient,
                            ismQHandle_t queue);

//Forget about any queues we were trying for inflight messages
//NOTE: The client state must be locked before calling the function
void iecs_forgetInflightMsgs( ieutThreadData_t *pThreadData
                            , ismEngine_ClientState_t   *pClient);

//All the inflight messages on this queue for this client have been
//completed, the client can forget about the queue
void  iecs_completedInflightMsgs(ieutThreadData_t *pThreadData,
                                 ismEngine_ClientState_t   *pClient,
                                 ismQHandle_t               queue);


//Used when we have a message and want to know if it has been delivered to this client
bool iecs_msgInFlightForClient(ieutThreadData_t *pThreadData,
                               iecsMessageDeliveryInfoHandle_t  hMsgDelInfo,
                               uint32_t deliveryId,
                               void *hnode);

int32_t iecs_sessionCleanupFromDeliveryInfo( ieutThreadData_t *pThreadData
                                           , ismEngine_Session_t *pSession
                                           , iecsMessageDeliveryInfoHandle_t hMsgDeliveryInfo);

int32_t iecs_incrementDurableObjectCount(ieutThreadData_t *pThreadData,
                                         ismEngine_ClientState_t *pClient);

void iecs_decrementDurableObjectCount(ieutThreadData_t *pThreadData,
                                      ismEngine_ClientState_t *pClient);

int32_t iecs_updateDurableObjectCountForClientId(ieutThreadData_t *pThreadData,
                                                 const char *pClientId,
                                                 uint32_t creationProtocolID,
                                                 bool increment);

typedef int32_t (*iecs_iterateDlvyInfoCB_t)( ieutThreadData_t *pThreadData
                                           , ismQHandle_t Qhdl
                                           , void *pNode
                                           , void *context);

//Iterate through all the message delivery references calling a function
int32_t iecs_iterateMessageDeliveryInfo( ieutThreadData_t * pThreadData
                                       , iecsMessageDeliveryInfoHandle_t  hMsgDelInfo
                                       , iecs_iterateDlvyInfoCB_t dvlyInfoCB
                                       , void *context);

int32_t iecs_importMessageDeliveryReference( ieutThreadData_t * pThreadData
                                           , ismEngine_ClientState_t *pClient
                                           , uint64_t dataId
                                           , uint32_t deliveryId
                                           , ismStore_Handle_t hStoreRecord
                                           , ismStore_Handle_t hStoreMessage
                                           , ismQHandle_t hQueue
                                           , void *hNode);

// Create a new clientState allocating and adding it to the clientState table (clientStateUtils.c)
int32_t iecs_createClientState(ieutThreadData_t *pThreadData,
                               const char *pClientId,
                               uint32_t protocolId,
                               uint32_t options,
                               uint32_t internalOptions,
                               iereResourceSetHandle_t resourceSet,
                               void *pStealContext,
                               ismEngine_StealCallback_t pStealCallbackFn,
                               ismSecurity_t *pSecContext,
                               ismEngine_ClientStateHandle_t * phClient,
                               void *pContext,
                               size_t contextLength,
                               ismEngine_CompletionCallback_t pCallbackFn);

// No internal options
#define iecsCREATE_CLIENT_OPTION_NONE                         0x00000000
// This client is being created to deal with (remove) a non-acking client
#define iecsCREATE_CLIENT_OPTION_NONACKING_TAKEOVER           0x00000001

// Finds the client at the end of a chain of thieves (clientStateUtils.c)
ismEngine_ClientState_t *iecs_getVictimizedClient(ieutThreadData_t *pThreadData,
                                                  const char *pClientId,
                                                  uint32_t clientIdHash);

// Discard a zombie client-state (clientStateUtils.c)
int32_t iecs_discardZombieClientState(ieutThreadData_t *pThreadData,
                                      const char *pClientId,
                                      bool fFromImport,
                                      void *pContext,
                                      size_t contextLength,
                                      ismEngine_CompletionCallback_t pCallbackFn);


// Force the discarding of a client state -- this will discard the client-state
// if it is a zombie, or is stealable (or doesn't exist) but will return ISMRC_ClientIdInUse
// if it was not possible.
int32_t iecs_forceDiscardClientState(ieutThreadData_t *pThreadData,
                                     const char *pClientId,
                                     uint32_t options);

// It is invalid to pass no options

// Force remove the client because it is a non-acking client
#define iecsFORCE_DISCARD_CLIENT_OPTION_NON_ACKING_CLIENT 0x00000001


//Find a clientstate for a given clientid (optionally, only if connected) and acquire a ref to it (clientStateUtils.c)
void iecs_findClientState( ieutThreadData_t *pThreadData
                         , const char *pClientId
                         , bool onlyIfConnected
                         , ismEngine_ClientState_t **ppConnectedClient);

// Find the message delivery info handle for a given client Id (clientStateUtils.c)
int32_t iecs_findClientMsgDelInfo(ieutThreadData_t *pThreadData,
                                  const char *pClientId,
                                  iecsMessageDeliveryInfoHandle_t *phMsgDelInfo);

// Add the specified client state to the dump (clientStateUtils.c)
int32_t iecs_dumpClientState(ieutThreadData_t *pThreadData,
                             const char *pClientId,
                             iedmDumpHandle_t dumpHdl);

bool iecs_discardClientForUnreleasedMessageDeliveryReference( ieutThreadData_t *pThreadData
                                                            , ismQHandle_t Q
                                                            , void *mdrNode
                                                            , const char *pClientId);

// Increase the current count of active clientStates (ones which might send work in)
void iecs_incrementActiveClientStateCount(ieutThreadData_t *pThreadData,
                                          iecsDurability_t durability,
                                          bool updateExternalStats,
                                          iereResourceSetHandle_t resourceSet);

// Decrease the current count of active clientStates
void iecs_decrementActiveClientStateCount(ieutThreadData_t *pThreadData,
                                          iecsDurability_t durability,
                                          bool updateExternalStats,
                                          iereResourceSetHandle_t resourceSet);

// For the specified clientId & protocolId return the resourceSet to use
iereResourceSetHandle_t iecs_getResourceSet(ieutThreadData_t *pThreadData,
                                            const char *pClientId,
                                            uint32_t protocolId,
                                            int32_t operation);

#endif /* __ISM_CLIENTSTATE_DEFINED */

/*********************************************************************/
/* End of clientState.h                                              */
/*********************************************************************/
