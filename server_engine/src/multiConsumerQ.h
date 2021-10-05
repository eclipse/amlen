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
/// @file multiConsumerQ.h
/// @brief API functions for using the intermediate Queue
///
/// Defines API functions for putting and getting to a
/// intermediate queue that supports only one getter at a time
//*********************************************************************

#ifndef __ISM_ENGINE_MULTICONSUMERQ_DEFINED
#define __ISM_ENGINE_MULTICONSUMERQ_DEFINED

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <pthread.h>

#include "engineInternal.h"
#include "queueCommon.h"
#include "transaction.h"

// Creates an intermediate Q
int32_t iemq_createQ( ieutThreadData_t *pThreadData
                    , const char *pQName
                    , ieqOptions_t QOptions
                    , iepiPolicyInfo_t *pPolicyInfo
                    , ismStore_Handle_t hStoreObj
                    , ismStore_Handle_t hStoreProps
                    , iereResourceSetHandle_t resourceSet
                    , ismQHandle_t *pQhdl);
// Is the queue marked as deleted?
bool iemq_isDeleted(ismQHandle_t Qhdl);
// Mark the queue as deleted
int32_t iemq_markQDeleted( ieutThreadData_t *pThreadData
                         , ismQHandle_t Qhdl
                         , bool updateStore);
// Deletes a intermediate Q
int32_t iemq_deleteQ( ieutThreadData_t *pThreadData
                    , ismQHandle_t *pQh
                    , bool freeOnly);
// Puts a message on an Intermediate Q
int32_t iemq_putMessage(ieutThreadData_t *pThreadData,
                        ismQHandle_t Qhdl,
                        ieqPutOptions_t putOptions,
                        ismEngine_Transaction_t *pTran,
                        ismEngine_Message_t *pMessage,
                        ieqMsgInputType_t inputMsgTreatment);
// Completes the rehydrate of a queue
int32_t iemq_completeRehydrate( ieutThreadData_t *pThreadData,
                                ismQHandle_t Qhdl );
// Completes the import of a queue
int32_t iemq_completeImport( ieutThreadData_t *pThreadData
                            , ismQHandle_t Qhdl);
// Puts a message on a queue during restart
int32_t iemq_rehydrateMsg( ieutThreadData_t *pThreadData
                         , ismQHandle_t Qhdl
                         , ismEngine_Message_t *pMsg
                         , ismEngine_RestartTransactionData_t *transData
                         , ismStore_Handle_t hMsgRef
                         , ismStore_Reference_t *pReference );
// Rehydrates the delivery id for a message
int32_t iemq_rehydrateDeliveryId( ieutThreadData_t *pThreadData
                                , ismQHandle_t Qhdl
                                , iecsMessageDeliveryInfoHandle_t hMsgDelInfo
                                , ismStore_Handle_t hMsgRef
                                , uint32_t deliveryId
                                , void **ppnode );
// Associate the Queue and the Consumer
int32_t iemq_initWaiter( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ismEngine_Consumer_t *pConsumer);
// Remove association between the Queue and the Consumer
int32_t iemq_termWaiter( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ismEngine_Consumer_t *pConsumer );
// Enable delivery of messages to the consumer
int32_t iemq_enableWaiter( ismQHandle_t Qhdl
                         , ismEngine_Consumer_t *pConsumer );
// Disable delivery of messages to the consumer on the queue
int32_t iemq_disableWaiter( ismQHandle_t Qhdl
                          , ismEngine_Consumer_t *pConsumer );
// Disable delivery of messages to the consumer on the queue whilst you have the consumer locked in delivering state
void iemq_suspendWaiter( ismQHandle_t Qhdl
                       , ismEngine_Consumer_t *pConsumer );

//Increase messages expired counter
void iemq_messageExpired( ieutThreadData_t *pThreadData
                        , ismQHandle_t Qhdl);

// Acknowledge delivery of a message
int32_t iemq_acknowledge( ieutThreadData_t *pThreadData
                        , ismQHandle_t Qhdl
                        , ismEngine_Session_t *pSession
                        , ismEngine_Transaction_t *pTran
                        , void *pDelivery
                        , uint32_t options
                        , ismEngine_AsyncData_t *asyncInfo);

void  iemq_prepareAck( ieutThreadData_t *pThreadData
                     , ismQHandle_t Qhdl
                     , ismEngine_Session_t *pSession
                     , ismEngine_Transaction_t *pTran
                     , void *pDelivery
                     , uint32_t options
                     , uint32_t *pStoreOps);

int32_t iemq_processAck( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ismEngine_Session_t *pSession
                       , ismEngine_Transaction_t *pTran
                       , void *pDelivery
                       , uint32_t options
                       , ismStore_Handle_t *phMsgToUnstore
                       , bool *pTriggerSessionRedelivery
                       , ismEngine_BatchAckState_t *pAckState
                       , ieutThreadData_t **ppJobThread);

void iemq_completeAckBatch( ieutThreadData_t *pThreadData
                          , ismQHandle_t Qhdl
                          , ismEngine_Session_t *pSession
                          , ismEngine_BatchAckState_t *pAckState );
int32_t iemq_relinquish( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , void *pDelivery
                       , ismEngine_RelinquishType_t relinquishType
                       , uint32_t *pStoreOpCount );

// Drain messages from a queue
int32_t iemq_drainQ(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl);
// Query queue statistics from a queue
void iemq_getStats(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl, ismEngine_QueueStatistics_t *stats);
// Explicitly set / reset statistic for a queue
void iemq_setStats(ismQHandle_t Qhdl, ismEngine_QueueStatistics_t *stats, ieqSetStatsType_t setType);
// see whether there are available messages for a given consumer
int32_t iemq_checkAvailableMsgs(ismQHandle_t Qhdl, ismEngine_Consumer_t *pConsumer);
// see whether there are available messages for a queue
int32_t iemq_checkWaiters( ieutThreadData_t *pThreadData
                         , ismQHandle_t Qhdl
                         , ismEngine_AsyncData_t *asyncInfo);


// Queue Debug
void iemq_debugQ( ieutThreadData_t *pThreadData
                , ismQHandle_t Qhdl
                , ismEngine_DebugOptions_t debugOptions
                , ismEngine_DebugHdl_t *phDebug);
// Write iemq structure descriptions to dump
void iemq_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl);
// Queue Dump
void iemq_dumpQ( ieutThreadData_t *pThreadData
               , ismQHandle_t  Qhdl
               , iedmDumpHandle_t dumpHdl);

// Return definition store handle
ismStore_Handle_t iemq_getDefnHdl( ismQHandle_t Qhdl );
// Return properties store handle
ismStore_Handle_t iemq_getPropsHdl( ismQHandle_t Qhdl );
// Set properties store handle
void iemq_setPropsHdl( ismQHandle_t Qhdl,
                       ismStore_Handle_t propsHdl );
// Update the properties of the queue
int32_t iemq_updateProperties( ieutThreadData_t *pThreadData,
                               ismQHandle_t Qhdl,
                               const char *pQName,
                               ieqOptions_t QOptions,
                               ismStore_Handle_t propsHdl,
                               iereResourceSetHandle_t resourceSet );

int32_t iemq_markMessageGotInTran( ieutThreadData_t *pThreadData
                                 , ismQHandle_t qhdl
                                 , uint64_t orderIdOnQ
                                 , ismEngine_RestartTransactionData_t *pTransactionData);

//Remove from MDR table and the queue all messages for this client
void iemq_relinquishAllMsgsForClient( ieutThreadData_t *pThreadData
                                    , ismQHandle_t Qhdl
                                    , iecsMessageDeliveryInfoHandle_t hMsgDelInfo
                                    , ismEngine_RelinquishType_t relinquishType);

//Needed in completeWaiterActions
void iemq_reducePreDeleteCount(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl);

//forget about inflight_messages
void iemq_forgetInflightMsgs( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl );

//This queue has no messages that the delivery must complete
// so this function is a stub...
bool iemq_redeliverEssentialOnly( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl );

//Post rehydration remove any queues that are deleted
void iemq_removeIfUnneeded( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl );

//Scan the queue looking for expired msgs to reap
ieqExpiryReapRC_t iemq_reapExpiredMsgs( ieutThreadData_t *pThreadData
                                      , ismQHandle_t Qhdl
                                      , uint32_t nowExpire
                                      , bool forcefullscan
                                      , bool allowMsgDelivery);

//On restart, remove any nodes we rehydrated but were marked consumed...
int32_t iemq_removeRehydratedConsumedNodes(ieutThreadData_t *pThreadData);

//Get for each consumer connected to the queue
int32_t iemq_getConsumerStats( ieutThreadData_t *pThreadData
                             , ismQHandle_t Qhdl
                             , iempMemPoolHandle_t memPoolHdl
                             , size_t *pNumConsumers
                             , ieqConsumerStats_t consDataArray[]);

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_MULTICONSUMERQ_DEFINED */

/*********************************************************************/
/* End of intermediateQ.h                                            */
/*********************************************************************/
