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
/// @file simpQ.h
/// @brief API functions for using the simple Queue
///
/// Defines API functions for putting and getting to a
/// simple queue that supports only one getter at a time and that
/// doesn't support acking or transactions
//****************************************************************************
#ifndef __ISM_ENGINE_SIMPQFUNCS_DEFINED
#define __ISM_ENGINE_SIMPQFUNCS_DEFINED

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <pthread.h>

#include "engineInternal.h"
#include "queueCommon.h"
#include "transaction.h"

/* Used to put messages */
int32_t iesq_putMessage(ieutThreadData_t *pThreadData,
                        ismQHandle_t Qhdl,
                        ieqPutOptions_t putOptions,
                        ismEngine_Transaction_t *pTran,
                        ismEngine_Message_t *pMessage,
                        ieqMsgInputType_t inputMsgTreatment,
                        ismEngine_DelivererContext_t * unused);

/* Used to import messages */
int32_t iesq_importMessage( ieutThreadData_t *pThreadData
                          , ismQHandle_t Qhdl
                          , ismEngine_Message_t *pMessage);

/* Used at the end of recovery to complete recovery work */
int32_t iesq_completeRehydrate( ieutThreadData_t *pThreadData
                              , ismQHandle_t Qhdl );

/* Used to get messages: do init then enable*/
int32_t iesq_initWaiter( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ismEngine_Consumer_t *pConsumer);
/* Remove link between consumer and queue*/
int32_t iesq_termWaiter( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ismEngine_Consumer_t *pConsumer );

int32_t iesq_createQ( ieutThreadData_t *pThreadData
                    , const char *pQName
                    , ieqOptions_t QOptions
                    , iepiPolicyInfo_t *pPolicyInfo
                    , ismStore_Handle_t hStoreObj
                    , ismStore_Handle_t hStoreProps
                    , iereResourceSetHandle_t resourceSet
                    , ismQHandle_t *pQhdl );
bool iesq_isDeleted(ismQHandle_t Qhdl);
int32_t iesq_markQDeleted( ieutThreadData_t *pThreadData
                         , ismQHandle_t Qhdl
                         , bool updateStore );
int32_t iesq_deleteQ(ieutThreadData_t *pThreadData,
                     ismQHandle_t *pQh,
                     bool freeOnly);

void iesq_getStats(ieutThreadData_t *pThreadData,
                   ismQHandle_t Qhdl,
                   ismEngine_QueueStatistics_t *stats);
void iesq_setStats(ismQHandle_t Qhdl, ismEngine_QueueStatistics_t *stats, ieqSetStatsType_t setType);

void iesq_setPutsAttemptedStat(ismQHandle_t Qhdl, uint64_t newPutsAttempted);

// see whether there are available messages
int32_t iesq_checkAvailableMsgs(ismQHandle_t Qhdl, ismEngine_Consumer_t *pConsumer);

int32_t iesq_checkWaiters( ieutThreadData_t *pThreadData
                         , ismQHandle_t Qhdl
                         , ismEngine_AsyncData_t * asyncInfo
                         , ismEngine_DelivererContext_t * delivererContext);


int32_t iesq_drainQ(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl);
void iesq_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl);
void iesq_dumpQ( ieutThreadData_t *pThreadData
               , ismQHandle_t QHdl
               , iedmDumpHandle_t dumpHdl);

// Return definition store handle (always ismSTORE_NULL_HANDLE)
ismStore_Handle_t iesq_getDefnHdl( ismQHandle_t Qhdl );
// Return properties store handle
ismStore_Handle_t iesq_getPropsHdl( ismQHandle_t Qhdl );
// Set properties store handle
void iesq_setPropsHdl( ismQHandle_t Qhdl,
                       ismStore_Handle_t propsHdl );
// Update the queue properties
int32_t iesq_updateProperties( ieutThreadData_t *pThreadData,
                               ismQHandle_t Qhdl,
                               const char *pQName,
                               ieqOptions_t QOptions,
                               ismStore_Handle_t propsHdl,
                               iereResourceSetHandle_t resourceSet );
//Reduce usecount - when hits 0 queue is deleted
void iesq_reducePreDeleteCount( ieutThreadData_t *pThreadData,
                                ismQHandle_t Q );

//forget about inflight_messages
void iesq_forgetInflightMsgs( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl );

//This queue has no messages that the delivery must complete
//for...there are no acks... delivery is never incomplete so this
//is a stub function: 
bool iesq_redeliverEssentialOnly( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl );

//Make space on a queue for new messages by moving old messages
void iesq_reclaimSpace( ieutThreadData_t *pThreadData
                      , ismQHandle_t Qhdl
                      , bool takeLock);

//Post rehydration remove any queues that are deleted
void iesq_removeIfUnneeded( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl );

//Scan the queue looking for expired msgs to reap
ieqExpiryReapRC_t iesq_reapExpiredMsgs( ieutThreadData_t *pThreadData
                                      , ismQHandle_t Qhdl
                                      , uint32_t nowExpire
                                      , bool forcefullscan
                                      , bool allowMsgDelivery);

// Record that a message has expired
void iesq_messageExpired( ieutThreadData_t *pThreadData
                        , ismQHandle_t Qhdl);

// Completes the import of a queue
int32_t iesq_completeImport( ieutThreadData_t *pThreadData
                           , ismQHandle_t Qhdl);

//Get for consumer connected to the queue (if any)
int32_t iesq_getConsumerStats( ieutThreadData_t *pThreadData
                             , ismQHandle_t Qhdl
                             , iempMemPoolHandle_t memPoolHdl
                             , size_t *pNumConsumers
                             , ieqConsumerStats_t consDataArray[]);

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_SIMPQFUNCS_DEFINED */

/*********************************************************************/
/* End of simpQ.h                                                    */
/*********************************************************************/
