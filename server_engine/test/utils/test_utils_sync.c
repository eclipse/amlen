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
#include "engine.h"
#include "engineInternal.h"
#include "queueCommon.h"
#include "clientState.h"
#include "resourceSetStats.h"

typedef struct tag_opContext_t {
    volatile int32_t rc;
    void *handle;
    pthread_mutex_t opMutex;
} opContext_t;

#define OPCONTEXT_DEFAULT {0, NULL, PTHREAD_MUTEX_INITIALIZER}

static void initMutex(pthread_mutex_t *pMutex)
{
   /* Ensure that we have defined behaviour in the case of relocking a mutex */
   pthread_mutexattr_t mutexattrs;
   pthread_mutexattr_init(&mutexattrs);
   pthread_mutexattr_settype(&mutexattrs, PTHREAD_MUTEX_NORMAL);
   pthread_mutex_init(pMutex, &mutexattrs);
}

static void operationCompleteCallback(int32_t rc, void *handle, void *pContext)
{
   opContext_t *pOpContext = *(opContext_t **)pContext;
   pOpContext->rc = rc;
   pOpContext->handle = handle;
   pthread_mutex_unlock(&(pOpContext->opMutex));
}

int32_t sync_wait_for_opComplete(opContext_t *opContext, void **pHandle)
{
    int osrc;
    uint32_t timeoutCount = 0;
    struct timespec abs_time;
    clock_gettime(CLOCK_REALTIME , &abs_time);
    uint64_t nanos = abs_time.tv_nsec;
    do
    {
        nanos += 1000L; /* 0.001 milliseconds */
        while ( nanos >= 1000000000L )
        {
          abs_time.tv_sec++ ;
          nanos -= 1000000000L ;
        }
        abs_time.tv_nsec = nanos;
        osrc = pthread_mutex_timedlock(&opContext->opMutex, &abs_time);

        if (osrc == 0)
        {
            break;
        }
        else
        {
            // After 60 seconds of waiting for the lock, give up.
            if (osrc == ETIMEDOUT && timeoutCount < 60000000)
            {
                // Every so often, send a heartbeat to the engine
                if ((timeoutCount++ % 1) == 0)
                {
                    ism_engine_heartbeat();
                }
            }
            else
            {
                abort();
            }
        }
    }
    while(1);

    if (pHandle) *pHandle = opContext->handle;

    return(opContext->rc);
}

int32_t sync_ism_engine_putMessage(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_ProducerHandle_t      hProducer,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage)
{
    opContext_t putContext = OPCONTEXT_DEFAULT;
    opContext_t *pPutContext = &putContext;

    initMutex(&(putContext.opMutex));

    pthread_mutex_lock(&(putContext.opMutex));

    int32_t rc = ism_engine_putMessage(hSession,
                                       hProducer,
                                       hTran,
                                       hMessage,
                                       &pPutContext,
                                       sizeof(pPutContext),
                                       operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pPutContext, NULL);
    }
    pthread_mutex_unlock(&(putContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_putMessageOnDestination(
        ismEngine_SessionHandle_t       hSession,
        ismDestinationType_t            destinationType,
        const char *                    pDestinationName,
        ismEngine_TransactionHandle_t   hTran,
        ismEngine_MessageHandle_t       hMessage)
{
    opContext_t putContext = OPCONTEXT_DEFAULT;
    opContext_t *pPutContext = &putContext;

    initMutex(&(putContext.opMutex));

    pthread_mutex_lock(&(putContext.opMutex));

    int32_t rc = ism_engine_putMessageOnDestination(hSession,
                                                    destinationType,
                                                    pDestinationName,
                                                    hTran,
                                                    hMessage,
                                                    &pPutContext,
                                                    sizeof(pPutContext),
                                                    operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pPutContext, NULL);
    }
    pthread_mutex_unlock(&(putContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_putMessageInternalOnDestination(
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_MessageHandle_t       hMessage)
{
    opContext_t putContext = OPCONTEXT_DEFAULT;
    opContext_t *pPutContext = &putContext;

    initMutex(&(putContext.opMutex));

    pthread_mutex_lock(&(putContext.opMutex));

    int32_t rc = ism_engine_putMessageInternalOnDestination(
                                                    destinationType,
                                                    pDestinationName,
                                                    hMessage,
                                                    &pPutContext,
                                                    sizeof(pPutContext),
                                                    operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pPutContext, NULL);
    }
    pthread_mutex_unlock(&(putContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_putMessageWithDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_ProducerHandle_t      hProducer,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel)
{
    opContext_t putContext = OPCONTEXT_DEFAULT;
    opContext_t *pPutContext = &putContext;

    initMutex(&(putContext.opMutex));

    pthread_mutex_lock(&(putContext.opMutex));

    int32_t rc = ism_engine_putMessageWithDeliveryId(hSession,
                                       hProducer,
                                       hTran,
                                       hMessage,
                                       unrelDeliveryId,
                                       phUnrel,
                                       &pPutContext,
                                       sizeof(pPutContext),
                                       operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pPutContext, NULL);
    }
    pthread_mutex_unlock(&(putContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_putMessageWithDeliveryIdOnDestination(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel)
{
    opContext_t putContext = OPCONTEXT_DEFAULT;
    opContext_t *pPutContext = &putContext;

    initMutex(&(putContext.opMutex));

    pthread_mutex_lock(&(putContext.opMutex));

    int32_t rc = ism_engine_putMessageWithDeliveryIdOnDestination(hSession,
                                       destinationType,
                                       pDestinationName,
                                       hTran,
                                       hMessage,
                                       unrelDeliveryId,
                                       phUnrel,
                                       &pPutContext,
                                       sizeof(pPutContext),
                                       operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pPutContext, NULL);
    }
    pthread_mutex_unlock(&(putContext.opMutex));

    return rc;

}

int32_t sync_ism_engine_unsetRetainedMessageWithDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_ProducerHandle_t      hProducer,
    uint32_t                        options,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel)
{
    opContext_t unsetContext = OPCONTEXT_DEFAULT;
    opContext_t *pUnsetContext = &unsetContext;

    initMutex(&(unsetContext.opMutex));

    pthread_mutex_lock(&(unsetContext.opMutex));

    int32_t rc = ism_engine_unsetRetainedMessageWithDeliveryId(hSession,
                                       hProducer,
                                       options,
                                       hTran,
                                       hMessage,
                                       unrelDeliveryId,
                                       phUnrel,
                                       &pUnsetContext,
                                       sizeof(pUnsetContext),
                                       operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pUnsetContext, NULL);
    }
    pthread_mutex_unlock(&(unsetContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_unsetRetainedMessageWithDeliveryIdOnDestination(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    uint32_t                        options,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel)
{
    opContext_t unsetContext = OPCONTEXT_DEFAULT;
    opContext_t *pUnsetContext = &unsetContext;

    initMutex(&(unsetContext.opMutex));

    pthread_mutex_lock(&(unsetContext.opMutex));

    int32_t rc = ism_engine_unsetRetainedMessageWithDeliveryIdOnDestination(hSession,
                                       destinationType,
                                       pDestinationName,
                                       options,
                                       hTran,
                                       hMessage,
                                       unrelDeliveryId,
                                       phUnrel,
                                       &pUnsetContext,
                                       sizeof(pUnsetContext),
                                       operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pUnsetContext, NULL);
    }
    pthread_mutex_unlock(&(unsetContext.opMutex));

    return rc;

}

int32_t sync_ism_engine_confirmMessageDelivery(
        ismEngine_SessionHandle_t       hSession,
        ismEngine_TransactionHandle_t   hTran,
        ismEngine_DeliveryHandle_t      hDelivery,
        uint32_t                        options)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_confirmMessageDelivery(
                                       hSession,
                                       hTran,
                                       hDelivery,
                                       options,
                                       &pOpContext,
                                       sizeof(pOpContext),
                                       operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_destroyClientState(
        ismEngine_ClientStateHandle_t   hClient,
        uint32_t                        options)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_destroyClientState(
                                       hClient,
                                       options,
                                       &pOpContext,
                                       sizeof(pOpContext),
                                       operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_destroyProducer(
        ismEngine_ProducerHandle_t      hProducer)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_destroyProducer(
                                       hProducer,
                                       &pOpContext,
                                       sizeof(pOpContext),
                                       operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_destroySession(
          ismEngine_SessionHandle_t       hSession)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_destroySession(
                                       hSession,
                                       &pOpContext,
                                       sizeof(pOpContext),
                                       operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_destroyConsumer(
          ismEngine_ConsumerHandle_t  hConsumer)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_destroyConsumer(
                                       hConsumer,
                                       &pOpContext,
                                       sizeof(pOpContext),
                                       operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_confirmMessageDeliveryBatch(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    uint32_t                        hdlCount,
    ismEngine_DeliveryHandle_t *    pDeliveryHdls,
    uint32_t                        options)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_confirmMessageDeliveryBatch(
                        hSession,
                        hTran,
                        hdlCount,
                        pDeliveryHdls,
                        options,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}
int32_t sync_ism_engine_commitTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    uint32_t                        options)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_commitTransaction(
                        hSession,
                        hTran,
                        options,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_commitGlobalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t                      *pXID,
    uint32_t                        option)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_commitGlobalTransaction(
                        hSession,
                        pXID,
                        option,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_prepareGlobalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t                      *pXID)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_prepareGlobalTransaction(
                        hSession,
                        pXID,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_createLocalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t * phTran)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_createLocalTransaction(
                        hSession,
                        phTran,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, (void **)phTran);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_createGlobalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t                     * pXID,
    uint32_t                        options,
    ismEngine_TransactionHandle_t * phTran)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_createGlobalTransaction(
                        hSession,
                        pXID,
                        options,
                        phTran,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, (void **)phTran);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_prepareTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_prepareTransaction(
                        hSession,
                        hTran,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_forgetGlobalTransaction(
    ism_xid_t *                     pXID)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_forgetGlobalTransaction(
                        pXID,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_completeGlobalTransaction(
        ism_xid_t *                     pXID,
        ismTransactionCompletionType_t  completionType)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_completeGlobalTransaction(
                        pXID,
                        completionType,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_rollbackTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_rollbackTransaction(
                        hSession,
                        hTran,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, (void **)NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_createSubscription(
    ismEngine_ClientStateHandle_t             hRequestingClient,
    const char *                              pSubName,
    const ism_prop_t *                        pSubProperties,
    uint8_t                                   destinationType,
    const char *                              pDestinationName,
    const ismEngine_SubscriptionAttributes_t *pSubAttributes,
    ismEngine_ClientStateHandle_t             hOwningClient)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_createSubscription(
                        hRequestingClient,
                        pSubName,
                        pSubProperties,
                        destinationType,
                        pDestinationName,
                        pSubAttributes,
                        hOwningClient,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_createClientState(
    const char *                    pClientId,
    uint32_t                        protocolId,
    uint32_t                        options,
    void *                          pStealContext,
    ismEngine_StealCallback_t       pStealCallbackFn,
    ismSecurity_t *                 pSecContext,
    ismEngine_ClientStateHandle_t * phClient)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_createClientState(
                        pClientId,
                        protocolId,
                        options,
                        pStealContext,
                        pStealCallbackFn,
                        pSecContext,
                        phClient,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, (void **)phClient);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_iecs_createClientState(
    ieutThreadData_t *              pThreadData,
    const char *                    pClientId,
    uint32_t                        protocolId,
    uint32_t                        options,
    uint32_t                        internalOptions,
    void *                          pStealContext,
    ismEngine_StealCallback_t       pStealCallbackFn,
    ismSecurity_t *                 pSecContext,
    ismEngine_ClientStateHandle_t * phClient)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = iecs_createClientState(
                        pThreadData,
                        pClientId,
                        protocolId,
                        options,
                        internalOptions,
                        iere_getResourceSet(pThreadData, pClientId, NULL, iereOP_ADD),
                        pStealContext,
                        pStealCallbackFn,
                        pSecContext,
                        phClient,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, (void **)phClient);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_removeUnreleasedDeliveryId(
        ismEngine_SessionHandle_t       hSession,
        ismEngine_TransactionHandle_t   hTran,
        ismEngine_UnreleasedHandle_t    hUnrel)
{
    opContext_t removeContext = OPCONTEXT_DEFAULT;
    opContext_t *pRemoveContext = &removeContext;

    initMutex(&(removeContext.opMutex));

    pthread_mutex_lock(&(removeContext.opMutex));

    int32_t rc = ism_engine_removeUnreleasedDeliveryId(hSession,
                                                       hTran,
                                                       hUnrel,
                                                       &pRemoveContext,
                                                       sizeof(pRemoveContext),
                                                       operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pRemoveContext, NULL);
    }
    pthread_mutex_unlock(&(removeContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_exportResources(
        const char *                    pClientId,
        const char *                    pTopic,
        const char *                    pFilename,
        const char *                    pPassword,
        uint32_t                        options,
        uint64_t *                      pRequestID)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_exportResources(
                        pClientId,
                        pTopic,
                        pFilename,
                        pPassword,
                        options,
                        pRequestID,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

int32_t sync_ism_engine_importResources(
        const char *                    pFilename,
        const char *                    pPassword,
        uint32_t                        options,
        uint64_t *                      pRequestID)
{
    opContext_t opContext = OPCONTEXT_DEFAULT;
    opContext_t *pOpContext = &opContext;

    initMutex(&(opContext.opMutex));

    pthread_mutex_lock(&(opContext.opMutex));

    int32_t rc = ism_engine_importResources(
                        pFilename,
                        pPassword,
                        options,
                        pRequestID,
                        &pOpContext,
                        sizeof(pOpContext),
                        operationCompleteCallback);

    if (rc == ISMRC_AsyncCompletion)
    {
        rc = sync_wait_for_opComplete(pOpContext, NULL);
    }
    pthread_mutex_unlock(&(opContext.opMutex));

    return rc;
}

void test_usleepAndHeartbeat(__useconds_t useconds)
{
    ismEngine_FullMemoryBarrier();
    usleep(useconds);
    ism_engine_heartbeat();
}

void test_waitForMessages(volatile uint32_t *cons1count, volatile uint32_t *cons2count, uint32_t expected, uint32_t timeoutSecs)
{
    uint32_t waits = 0;
    uint64_t curCount;

    do
    {
        curCount = 0;

        if (cons1count != NULL) curCount += (*cons1count);
        if (cons2count != NULL) curCount += (*cons2count);

        if (curCount < expected)
        {
            //Sleep for a 10 microseconds
            test_usleepAndHeartbeat(10);
            waits++;

            if (waits >= timeoutSecs * 100000) //If we waited the right number of seconds....
            {
                printf("Expected %u messages. Received %lu\n",
                        expected, curCount);
                abort();
            }
        }
    }
    while (curCount < expected);
}

void test_waitForMessages64(volatile uint64_t *cons1count, volatile uint64_t *cons2count, uint64_t expected, uint32_t timeoutSecs)
{
    uint32_t waits = 0;
    uint64_t curCount;

    do
    {
        curCount = 0;

        if (cons1count != NULL) curCount += (*cons1count);
        if (cons2count != NULL) curCount += (*cons2count);

        if (curCount < expected)
        {
            //Sleep for 10 microseconds
            test_usleepAndHeartbeat(10);
            waits++;

            if (waits >= timeoutSecs * 100000) //If we waited the right number of seconds....
            {
                printf("Expected %lu messages. Received %lu\n",
                        expected, curCount);
                abort();
            }
        }
    }
    while (curCount < expected);
}

void test_waitForEmptyQ( ismQHandle_t Qhdl
                       , uint32_t totalTimeout
                       , uint32_t nochangeTimeout)
{
    uint64_t curDepth      = 0;
    uint64_t totalWaits    = 0;
    uint64_t noChangeWaits = 0;

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    do
    {
        uint64_t lastDepth = curDepth;

        ismEngine_QueueStatistics_t stats = {0};
        ieq_getStats(pThreadData, Qhdl, &stats);
        curDepth = stats.BufferedMsgs;

        if (curDepth > 0)
        {
            //Sleep for 10 microseconds
            test_usleepAndHeartbeat(10);

            totalWaits++;

            if (totalWaits > (100000 * totalTimeout))
            {
                printf("After timeout, queue depth was %lu... giving up\n", curDepth);
                abort();
            }

            if( curDepth == lastDepth)
            {
                noChangeWaits++;

                if (noChangeWaits > (100000 * nochangeTimeout))
                {
                    printf("No change in queueDepth (still at %lu) for a while. Giving up\n", curDepth);
                    abort();
                }
            }
            else
            {
                noChangeWaits = 0;
            }
        }
    }
    while(curDepth > 0);
}

void test_incrementActionsRemaining(volatile uint32_t *pActionsRemaining, uint32_t increment)
{
    __sync_fetch_and_add(pActionsRemaining, increment);
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_decrementActionsRemaining                    */
/*                                                                   */
/* Description:    Decrement a counter of actions remaining          */
/*                                                                   */
/*********************************************************************/
void test_decrementActionsRemaining(int32_t retcode, void *handle, void *pContext)
{
    volatile uint32_t *pActionCount = *(volatile uint32_t **)pContext;
    __sync_fetch_and_sub(pActionCount, 1);
}

void test_waitForActionsTimeOut(volatile uint32_t *pActionsRemaining, uint32_t timeOutSecs)
{
    uint32_t waits = 0;

    do
    {
        if (*pActionsRemaining > 0)
        {
            //Sleep for 10 microseconds
            test_usleepAndHeartbeat(10);
            waits++;

            if (waits >= timeOutSecs * 100000) //If we waited the right number of seconds....
            {
                printf("At time out had %u actions outstanding\n",
                         *pActionsRemaining );
                abort();
            }
        }
    }
    while (*pActionsRemaining > 0);
}

void test_waitForRemainingActions(volatile uint32_t *pActionsRemaining)
{
    test_waitForActionsTimeOut(pActionsRemaining, 300);
}
