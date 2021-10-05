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
/// @file intermediateQ.h
/// @brief API functions for using the intermediate Queue
///
/// Defines API functions for putting and getting to a
/// intermediate queue that supports only one getter at a time
//*********************************************************************

#ifndef __ISM_ENGINE_INTERMEDIATEQ_DEFINED
#define __ISM_ENGINE_INTERMEDIATEQ_DEFINED

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <pthread.h>

#include "engineInternal.h"
#include "queueCommon.h"
#include "transaction.h"

// Creates an intermediate Q
int32_t ieiq_createQ( ieutThreadData_t *pThreadData
                    , const char *pQName
                    , ieqOptions_t QOptions
                    , iepiPolicyInfo_t *pPolicyInfo
                    , ismStore_Handle_t hStoreObj
                    , ismStore_Handle_t hStoreProps
                    , iereResourceSetHandle_t resourceSet
                    , ismQHandle_t *pQhdl);
// Is the queue marked as deleted?
bool ieiq_isDeleted(ismQHandle_t Qhdl);
// Mark the queue as deleted
int32_t ieiq_markQDeleted(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl, bool updateStore);
// Deletes a intermediate Q
int32_t ieiq_deleteQ(ieutThreadData_t *pThreadData, ismQHandle_t *pQh, bool freeOnly);
// Puts a message on an Intermediate Q
int32_t ieiq_putMessage(ieutThreadData_t *pThreadData,
                        ismQHandle_t Qhdl,
                        ieqPutOptions_t putOptions,
                        ismEngine_Transaction_t *pTran,
                        ismEngine_Message_t *pMessage,
                        ieqMsgInputType_t inputMsgTreatment);
// Completes the rehydrate of a queue
int32_t ieiq_completeRehydrate( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl );
// Puts a message on a queue during restart
int32_t ieiq_rehydrateMsg( ieutThreadData_t *pThreadData
                         , ismQHandle_t Qhdl
                         , ismEngine_Message_t *pMsg
                         , ismEngine_RestartTransactionData_t *transData
                         , ismStore_Handle_t hMsgRef
                         , ismStore_Reference_t *pReference );
// Rehydrates the delivery id for a message on an Intermediate Q
int32_t ieiq_rehydrateDeliveryId( ieutThreadData_t *pThreadData
                                , ismQHandle_t Qhdl
                                , iecsMessageDeliveryInfoHandle_t hMsgDelInfo
                                , ismStore_Handle_t hMsgRef
                                , uint32_t deliveryId
                                , void **ppnode );
// Associate the Queue and the Consumer
int32_t ieiq_initWaiter( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ismEngine_Consumer_t *pConsumer);
// Remove association between the Queue and the Consumer
int32_t ieiq_termWaiter( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ismEngine_Consumer_t *pConsumer );
// Enable delivery of messages to the consumer
int32_t ieiq_enableWaiter( ismQHandle_t Qhdl
                         , ismEngine_Consumer_t *pConsumer );
// Disable delivery of messages to the consumer on the queue
int32_t ieiq_disableWaiter( ismQHandle_t Qhdl
                          , ismEngine_Consumer_t *pConsumer );
// Disable delivery of messages to the consumer on the queue whilst you have the consumer locked in delivering state
void ieiq_suspendWaiter( ismQHandle_t Qhdl
                       , ismEngine_Consumer_t *pConsumer );
// Record that a message has expired
void ieiq_messageExpired( ieutThreadData_t *pThreadData
                        , ismQHandle_t Qhdl);
// Acknowledge delivery of a message
int32_t ieiq_acknowledge( ieutThreadData_t *pThreadData
                        , ismQHandle_t Qhdl
                        , ismEngine_Session_t *pSession
                        , ismEngine_Transaction_t *pTran
                        , void *pDelivery
                        , uint32_t options
                        , ismEngine_AsyncData_t     *asyncInfo);
int32_t ieiq_setDeliveryId( ismQHandle_t Qhdl
                          , ismEngine_Consumer_t *pConsumer
                          , void *pDelivery
                          , uint32_t deliveryId );
// Drain messages from a queue
int32_t ieiq_drainQ(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl);
// Query queue statistics from a queue
void ieiq_getStats(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl, ismEngine_QueueStatistics_t *stats);
void ieiq_setStats(ismQHandle_t Qhdl, ismEngine_QueueStatistics_t *stats, ieqSetStatsType_t setType);

// see whether there are available messages
int32_t ieiq_checkAvailableMsgs(ismQHandle_t Qhdl, ismEngine_Consumer_t *pConsumer);

// Write ieiq structure descriptions to dump
void ieiq_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl);
// Queue Dump
void ieiq_dumpQ( ieutThreadData_t *pThreadData
               , ismQHandle_t QHdl
               , iedmDumpHandle_t dumpHdl);

//Check whether any messages can be delivered
int32_t ieiq_checkWaiters( ieutThreadData_t *pThreadData
                         , ismEngine_Queue_t *q
                         , ismEngine_AsyncData_t *asyncInfo);

// Return definition store handle
ismStore_Handle_t ieiq_getDefnHdl( ismQHandle_t Qhdl );
// Return properties store handle
ismStore_Handle_t ieiq_getPropsHdl( ismQHandle_t Qhdl );
// Set properties store handle
void ieiq_setPropsHdl( ismQHandle_t Qhdl,
                       ismStore_Handle_t propsHdl );
// Update the properties of the queue
int32_t ieiq_updateProperties( ieutThreadData_t *pThreadData,
                               ismQHandle_t Qhdl,
                               const char *pQName,
                               ieqOptions_t QOptions,
                               ismStore_Handle_t propsHdl,
                               iereResourceSetHandle_t resourceSet );
//reduce usecount
void ieiq_reducePreDeleteCount(ieutThreadData_t *pThreadData,
                               ismQHandle_t Qhdl);


//forget about inflight_messages
void ieiq_forgetInflightMsgs( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl );

//Only redeliver messages whose delivery MUST be completed
bool ieiq_redeliverEssentialOnly( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl );

//Delete old messages to make space for new ones
void ieiq_reclaimSpace( ieutThreadData_t *pThreadData
                      , ismQHandle_t Qhdl
                      , bool takeLock);

//Post rehydration remove any queues that are deleted
void ieiq_removeIfUnneeded( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl );

//Scan the queue looking for expired msgs to reap
ieqExpiryReapRC_t  ieiq_reapExpiredMsgs( ieutThreadData_t *pThreadData
                                       , ismQHandle_t Qhdl
                                       , uint32_t nowExpire
                                       , bool forcefullscan
                                       , bool expiryListLocked);

// Completes the import of a queue
int32_t ieiq_completeImport( ieutThreadData_t *pThreadData
                           , ismQHandle_t Qhdl);

//Get for consumer connected to the queue (if any)
int32_t ieiq_getConsumerStats( ieutThreadData_t *pThreadData
                             , ismQHandle_t Qhdl
                             , iempMemPoolHandle_t memPoolHdl
                             , size_t *pNumConsumers
                             , ieqConsumerStats_t consDataArray[]);

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_INTERMEDIATEQ_DEFINED */

/*********************************************************************/
/* End of intermediateQ.h                                            */
/*********************************************************************/
