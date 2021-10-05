/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
/// @file  test_utils_sync.h
///
/// @brief   sync wrappers for some engine functions that go async
///
/// @remarks In addition we have Utility funcs for use when it is unclear
///           what the locking pattern for async put callbacks is
///          At the time of writing, it is not clear whether the callback caused
///          by calls on a single thread will be issued on multiple threads at the
///          same time or just sequentially in the order of the original calls.
///          The decision affects how many memory barriers are needed in the
///          callback hence this header.
//****************************************************************************

#include "engine.h"

#define OVERLAPPING_CALLBACKS 1

#ifdef OVERLAPPING_CALLBACKS
#define ASYNCPUT_CB_ADD_AND_FETCH(valueptr, inc)  __sync_add_and_fetch(valueptr, inc)
#define ASYNCPUT_CB_SUB_AND_FETCH(valueptr, inc)  __sync_sub_and_fetch(valueptr, inc)
#else
#define ASYNCPUT_CB_ADD_AND_FETCH(valueptr, inc)  ((*(valueptr)) += inc)
#define ASYNCPUT_CB_SUB_AND_FETCH(valueptr, inc)  ((*(valueptr)) -= inc)
#endif


int32_t sync_ism_engine_putMessage(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_ProducerHandle_t      hProducer,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage);

int32_t sync_ism_engine_putMessageOnDestination(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage);

int32_t sync_ism_engine_putMessageInternalOnDestination(
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_MessageHandle_t       hMessage);

int32_t sync_ism_engine_putMessageWithDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_ProducerHandle_t      hProducer,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel);

XAPI int32_t sync_ism_engine_putMessageWithDeliveryIdOnDestination(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel);

int32_t sync_ism_engine_unsetRetainedMessageWithDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_ProducerHandle_t      hProducer,
    uint32_t                        options,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel);

int32_t sync_ism_engine_unsetRetainedMessageWithDeliveryIdOnDestination(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    uint32_t                        options,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel);

int32_t sync_ism_engine_confirmMessageDelivery(
        ismEngine_SessionHandle_t       hSession,
        ismEngine_TransactionHandle_t   hTran,
        ismEngine_DeliveryHandle_t      hDelivery,
        uint32_t                        options);

int32_t sync_ism_engine_confirmMessageDeliveryBatch(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    uint32_t                        hdlCount,
    ismEngine_DeliveryHandle_t *    pDeliveryHdls,
    uint32_t                        options);

int32_t sync_ism_engine_destroyClientState(
        ismEngine_ClientStateHandle_t   hClient,
        uint32_t                        options);

int32_t sync_ism_engine_destroyProducer(
        ismEngine_ProducerHandle_t      hProducer);

int32_t sync_ism_engine_destroySession(
          ismEngine_SessionHandle_t       hSession);

int32_t sync_ism_engine_destroyConsumer(
    ismEngine_ConsumerHandle_t      hConsumer);

int32_t sync_ism_engine_commitTransaction(
	    ismEngine_SessionHandle_t       hSession,
	    ismEngine_TransactionHandle_t   hTran,
	    uint32_t                        options);

int32_t sync_ism_engine_commitGlobalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t                      *pXID,
    uint32_t                        option);

int32_t sync_ism_engine_prepareGlobalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t                      *pXID);

int32_t sync_ism_engine_createLocalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t * phTran);

int32_t sync_ism_engine_createGlobalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t                     * pXID,
    uint32_t                        options,
    ismEngine_TransactionHandle_t * phTran);

int32_t sync_ism_engine_prepareTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran);

int32_t sync_ism_engine_forgetGlobalTransaction(
    ism_xid_t *                     pXID);

int32_t sync_ism_engine_completeGlobalTransaction(
        ism_xid_t *                     pXID,
        ismTransactionCompletionType_t  completionType);

int32_t sync_ism_engine_rollbackTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran);

int32_t sync_ism_engine_createSubscription(
    ismEngine_ClientStateHandle_t              hRequestingClient,
    const char *                               pSubName,
    const ism_prop_t *                         pSubProperties,
    uint8_t                                    destinationType,
    const char *                               pDestinationName,
    const ismEngine_SubscriptionAttributes_t * pSubAttributes,
    ismEngine_ClientStateHandle_t              hOwningClient);

int32_t sync_ism_engine_createClientState(
    const char *                    pClientId,
    uint32_t                        protocolId,
    uint32_t                        options,
    void *                          pStealContext,
    ismEngine_StealCallback_t       pStealCallbackFn,
    ismSecurity_t *                 pSecContext,
    ismEngine_ClientStateHandle_t * phClient);

int32_t sync_iecs_createClientState(
    ieutThreadData_t *              pThreadData,
    const char *                    pClientId,
    uint32_t                        protocolId,
    uint32_t                        options,
    uint32_t                        internalOptions,
    void *                          pStealContext,
    ismEngine_StealCallback_t       pStealCallbackFn,
    ismSecurity_t *                 pSecContext,
    ismEngine_ClientStateHandle_t * phClient);

int32_t sync_ism_engine_removeUnreleasedDeliveryId(
        ismEngine_SessionHandle_t       hSession,
        ismEngine_TransactionHandle_t   hTran,
        ismEngine_UnreleasedHandle_t    hUnrel);

int32_t sync_ism_engine_exportResources(
        const char *                    pClientId,
        const char *                    pTopic,
        const char *                    pFilename,
        const char *                    pPassword,
        uint32_t                        options,
        uint64_t *                      pRequestID);

int32_t sync_ism_engine_importResources(
        const char *                    pFilename,
        const char *                    pPassword,
        uint32_t                        options,
        uint64_t *                      pRequestID);

//Wait for 1 or 2 consumers to get the expected number of messages
void test_waitForMessages(volatile uint32_t *cons1count, volatile uint32_t *cons2count, uint32_t expected, uint32_t timeoutSecs);
void test_waitForMessages64(volatile uint64_t *cons1count, volatile uint64_t *cons2count, uint64_t expected, uint32_t timeoutSecs);


void test_waitForEmptyQ( ismQHandle_t Qhdl
                       , uint32_t totalTimeout
                       , uint32_t nochangeTimeout);

void test_incrementActionsRemaining(volatile uint32_t *pActionsRemaining, uint32_t increment);
void test_decrementActionsRemaining( int32_t retcode
                                   , void *handle
                                   , void *pContext);
void test_waitForRemainingActions(uint32_t *pActionsRemaining);
void test_waitForActionsTimeOut(volatile uint32_t *pActionsRemaining, uint32_t timeOutSecs);

void test_usleepAndHeartbeat(__useconds_t useconds);
