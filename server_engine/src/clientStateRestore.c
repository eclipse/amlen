/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
/// @file  clientStateRestore.c
/// @brief Functions called during client state recovery
//*********************************************************************
#define TRACE_COMP Engine

#include <assert.h>

#include "engineInternal.h"
#include "engineStore.h"
#include "clientState.h"
#include "clientStateInternal.h"
#include "destination.h"
#include "msgCommon.h"
#include "topicTree.h"
#include "resourceSetStats.h"
#include "engineUtils.h"

/*
 * Locally-defined callback function to identify remaining durable subscriptions for a
 * client
 */
typedef struct tag_checkForRemainingSubsContext_t
{
    ismEngine_ClientState_t *pClient;
    bool rememberQueues;
    iereResourceSetHandle_t resourceSet;
    ismQHandle_t *pQueues;
    uint32_t queueCount;
    uint32_t queueMax;
} checkForRemainingSubsContext_t;

static void checkForRemainingSubs(
        ieutThreadData_t *pThreadData,
        ismEngine_SubscriptionHandle_t subHandle,
        const char *pSubName,
        const char *pTopicString,
        void *properties,
        size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttributes,
        uint32_t consumerCount,
        void *pContext)
{
    checkForRemainingSubsContext_t *context = (checkForRemainingSubsContext_t *)pContext;
    ismQHandle_t queueHandle = ((ismEngine_Subscription_t *)subHandle)->queueHandle;

    // Remember the queue handles for all returned subscriptions if requested
    if (context->rememberQueues)
    {
        if (context->queueCount == context->queueMax)
        {
            iere_primeThreadCache(pThreadData, context->resourceSet);
            void *newQueues = iere_realloc(pThreadData,
                                           context->resourceSet,
                                           IEMEM_PROBE(iemem_clientState, 20),
                                           context->pQueues,
                                           sizeof(ismQHandle_t) * (context->queueMax + 100));

            if (newQueues != NULL)
            {
                context->pQueues = newQueues;
                context->queueMax += 100;
            }
        }

        if (context->queueCount < context->queueMax)
        {
            context->pQueues[context->queueCount] = queueHandle;
            context->queueCount += 1;
        }
    }
}

/*
 * Publish will message at the end of restart recovery
 */
static int32_t iecs_publishWillMessageRecovery(ieutThreadData_t *pThreadData,
                                               ismEngine_ClientState_t *pClient)
{
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    ieutTRACEL(pThreadData, pClient,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "(pClient %p)\n", __func__, pClient);

    // Although recovery of a disk generation might give us a previously deleted CPR, the CSR which
    // ought to accompany it will have been properly deleted. The recovery code will hide this kind
    // of orphaned CPR from us and automagically delete them at the end of recovery. Thus we do not
    // have to cope with the appearance of orphaned CPRs in this function.
    assert(pClient != NULL);

    // If it has a will-message, publish it
    if (pClient->hWillMessage != NULL)
    {
        // If WillDelay is 0, we should publish this message now, otherwise we leave it for
        // the expiry task to pick up.
        if (pClient->WillDelay == 0)
        {
            ismEngine_Message_t *pOriginalMsg = (ismEngine_Message_t *)pClient->hWillMessage;
            ismEngine_Message_t *pMessage = NULL;

            // Override the message's expiry based on the will message expiry time
            if (pClient->WillMessageTimeToLive == iecsWILLMSG_TIME_TO_LIVE_INFINITE)
            {
                pOriginalMsg->Header.Expiry = 0;
            }
            else
            {
                uint32_t newExpiry = ism_common_getExpire(((ism_time_t)pClient->WillMessageTimeToLive * 1000000000) + ism_common_currentTimeNanos());
                ieutTRACEL(pThreadData, newExpiry, ENGINE_NORMAL_TRACE,
                           "Overriding will message expiry from %u to %u\n", pOriginalMsg->Header.Expiry, newExpiry);
                pOriginalMsg->Header.Expiry = newExpiry;
            }

            // Make a copy of the will message setting the ServerTime to the current time
            rc = iem_createMessageCopy(pThreadData,
                                       pOriginalMsg,
                                       false,
                                       ism_engine_retainedServerTime(),
                                       pOriginalMsg->Header.Expiry,
                                       &pMessage);

            if (rc == OK)
            {
                ismEngine_Transaction_t *pTran = NULL;

                assert(pMessage != NULL);

                // For retained messages, we don't create a transaction because we know the publish we are
                // about to do will use one, which will be more efficient (and also works around defect 171313).
                if (pMessage->Header.Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN)
                {
                    // If the will message is not already marked as a null retained message but is
                    // to be retained and has a 0 length body, change the message type to MTYPE_NullRetained.
                    // We can do this because the pMessage we are dealing with is a copy of the original one
                    // that is not yet stored.
                    if (    (pMessage->Header.MessageType != MTYPE_NullRetained)
                         && (pMessage->AreaCount == 2)
                         && (pMessage->AreaTypes[1] == ismMESSAGE_AREA_PAYLOAD)
                         && (pMessage->AreaLengths[1] == 0))
                    {
                        assert(pMessage->StoreMsg.whole == 0);
                        pMessage->Header.MessageType = MTYPE_NullRetained;
                    }
                }
                else
                {
                    rc = ietr_createLocal(pThreadData,
                                          NULL,
                                          pMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT,
                                          false,
                                          NULL,
                                          &pTran);
                }

                if (rc == OK)
                {
                    rc = ieds_publish(pThreadData,
                                      pClient,
                                      pClient->pWillTopicName,
                                      iedsPUBLISH_OPTION_NONE,
                                      pTran,
                                      pMessage,
                                      0,
                                      NULL,
                                      0,
                                      NULL);

                    if (pTran != NULL)
                    {
                        if (rc == OK)
                        {
                            rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
                        }
                        else
                        {
                            assert(rc != ISMRC_SomeDestinationsFull);
                            (void)ietr_rollback(pThreadData, pTran, NULL, IETR_ROLLBACK_OPTIONS_NONE);
                        }
                    }
                }

                iem_releaseMessage(pThreadData, pMessage);
            }

            if (rc != OK)
            {
                ism_common_log_context logContext = {0};
                logContext.clientId = pClient->pClientId;
                if (pClient->resourceSet != iereNO_RESOURCE_SET)
                {
                    logContext.resourceSetId = pClient->resourceSet->stats.resourceSetIdentifier;
                }
                logContext.topicFilter = pClient->pWillTopicName;

                // The failure to put a will message is logged with an error message, but we carry on
                char messageBuffer[256];

                LOGCTX(&logContext, ERROR, Messaging, 3000, "%-s%s%d", "The server is unable to publish the Will message to topic {0}: Error={1} RC={2}.",
                       pClient->pWillTopicName,
                       ism_common_getErrorStringByLocale(rc, ism_common_getLocale(), messageBuffer, 255),
                       rc);

                rc = OK;
            }
            assert(pOriginalMsg == pClient->hWillMessage);

            iecs_unstoreWillMessage(pThreadData, pClient);
            iere_primeThreadCache(pThreadData, resourceSet);
            iecs_updateWillMsgStats(pThreadData, resourceSet, pMessage, -1);
            iere_free(pThreadData, resourceSet, iemem_clientState, pClient->pWillTopicName);
            iem_releaseMessage(pThreadData, pOriginalMsg);
            pClient->hWillMessage = NULL;
            pClient->pWillTopicName = NULL;
            pClient->WillMessageTimeToLive = iecsWILLMSG_TIME_TO_LIVE_INFINITE;
        }
    }
    else if (pClient->hStoreCPR != ismSTORE_NULL_HANDLE)
    {
        // No will-message, but a will topic or a non-durable client?
        // No problem. Just leave things tidy.
        if (pClient->pWillTopicName != NULL || pClient->Durability == iecsNonDurable)
        {
            iecs_unstoreWillMessage(pThreadData, pClient);
        }

        if (pClient->pWillTopicName != NULL)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_free(pThreadData, resourceSet, iemem_clientState, pClient->pWillTopicName);
            pClient->pWillTopicName = NULL;
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

/*
 * Handle completion of a single CPR recovery
 */
static int32_t iecs_completeCPRRecovery(ieutThreadData_t *pThreadData,
                                        uint64_t CPRhandle,
                                        void *rehydratedCPR,
                                        void *pContext)
{
    int32_t rc = OK;

    rc = iecs_publishWillMessageRecovery(pThreadData, (ismEngine_ClientState_t *)rehydratedCPR);

    return rc;
}


void iecs_completeRecoveryCallback(int rc, void *context)
{
    iecsRecoveryCompletionContext_t *pContext = context;
    __sync_sub_and_fetch(&pContext->remainingActions, 1);
}

/*
 * Complete partial deletion at the end of restart recovery
 */
static inline int32_t iecs_completeRecovery(ieutThreadData_t *pThreadData,
                                            ismEngine_ClientState_t *pClient,
                                            iecsRecoveryCompletionContext_t *pContext)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pClient,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "(pClient %p)\n", __func__, pClient);

    // If the client-state is durable and we've allocated a message delivery info
    // structure, we need to open the reference context for it
    if (pClient->Durability == iecsDurable && pClient->hMsgDeliveryInfo != NULL)
    {
        iecsMessageDeliveryInfo_t *pMsgDelInfo = pClient->hMsgDeliveryInfo;

        ismStore_ReferenceStatistics_t stats = {0};

        assert(pMsgDelInfo->hMsgDeliveryRefContext == ismSTORE_NULL_HANDLE);

        rc = ism_store_openReferenceContext(pClient->hStoreCSR,
                                            &stats,
                                            &pMsgDelInfo->hMsgDeliveryRefContext);

        if (rc != OK) goto mod_exit;

        assert(pMsgDelInfo->hMsgDeliveryRefContext != ismSTORE_NULL_HANDLE);

        ieutTRACEL(pThreadData, stats.HighestOrderId, ENGINE_HIFREQ_FNC_TRACE,
                   "Highest order id %lu\n", stats.HighestOrderId);
        pMsgDelInfo->NextOrderId = stats.HighestOrderId + 1;
    }

    // If the client-state is partially deleted, or is not durable and has no durable objects
    // we can remove it
    if (pClient->fDiscardDurable || (pClient->Durability == iecsNonDurable && pClient->durableObjects == 0))
    {
        assert(pClient->pThief == NULL);

        // We still have a will message, so it will be publishing via the delayed will message code
        if (pClient->hWillMessage != NULL)
        {
            assert(pClient->WillDelayExpiryTime != 0);
        }
        else
        {
            pthread_spin_lock(&pClient->UseCountLock);
            assert(pClient->UseCount == 1); // Need to remove it from the table
            assert(pClient->OpState == iecsOpStateZombie);
            pClient->UseCount += 1;
            pClient->OpState = iecsOpStateZombieRemoval;
            pthread_spin_unlock(&pClient->UseCountLock);

            iecs_releaseClientStateReference(pThreadData, pClient, false, false);
        }
    }
    else
    {
        assert(pClient->OpState == iecsOpStateZombie);

        // Check whether there are any (durable) subscriptions for this client id
        // building a list so we can relinquish MDRs for queues we no longer have any
        // interest in.
        checkForRemainingSubsContext_t remainingSubsContext = {pClient, true, pClient->resourceSet, NULL, 0, 0};

        rc = iett_listSubscriptions(pThreadData,
                                    pClient->pClientId,
                                    iettFLAG_LISTSUBS_NONE,
                                    NULL,
                                    &remainingSubsContext,
                                    checkForRemainingSubs);

        // Relinquish any messages whose MDR is not for one of the remaining subscription's queues
        if (pClient->hMsgDeliveryInfo != NULL)
        {
            iecs_relinquishAllMsgs(pThreadData,
                                   pClient->hMsgDeliveryInfo,
                                   remainingSubsContext.pQueues,
                                   remainingSubsContext.queueCount,
                                   iecsRELINQUISH_ACK_HIGHRELIABILITY_NOT_ON_QUEUE);
        }

        if (remainingSubsContext.queueCount != 0)
        {
            assert(remainingSubsContext.pQueues != NULL);
            iere_primeThreadCache(pThreadData, remainingSubsContext.resourceSet);
            iere_free(pThreadData, remainingSubsContext.resourceSet, iemem_clientState, remainingSubsContext.pQueues);
        }

        if (pClient->LastConnectedTime == 0)
        {
            assert(ismEngine_serverGlobal.ServerShutdownTimestamp != 0); // We should have read the SCR by now.

            uint32_t serverTimestamp = ismEngine_serverGlobal.ServerShutdownTimestamp;
            uint64_t newState = ((uint64_t)serverTimestamp << 32) | iestCSR_STATE_DISCONNECTED;

            iecs_setLCTandExpiry(pThreadData, pClient, ism_common_convertExpireToTime(serverTimestamp), NULL);

            // Update the state to DISCONNECTED - we are NOT setting the attribute
            // to be a null handle value.
            rc = ism_store_updateRecord(pThreadData->hStream,
                                        pClient->hStoreCSR,
                                        ismSTORE_NULL_HANDLE,
                                        newState,
                                        ismSTORE_SET_STATE);
            if (rc == OK)
            {
                __sync_add_and_fetch(&pContext->remainingActions, 1);

                rc = iest_store_asyncCommit(pThreadData, false, iecs_completeRecoveryCallback, pContext);

                if (rc == ISMRC_AsyncCompletion)
                {
                    rc = OK;
                }
                else
                {
                    iecs_completeRecoveryCallback(rc, pContext);
                }
            }
            else
            {
                iest_store_rollback(pThreadData, false);
            }
        }

        if (pClient->ExpiryTime != 0) pThreadData->stats.zombieSetToExpireCount += 1;
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

/*
 * Handle completion of a single CSR recovery
 */
static int32_t iecs_completeCSRRecovery(ieutThreadData_t *pThreadData,
                                        uint64_t CSRhandle,
                                        void *rehydratedCSR,
                                        void *pContext)
{
    int32_t rc = OK;

    rc = iecs_completeRecovery(pThreadData,
                               (ismEngine_ClientState_t *)rehydratedCSR,
                               (iecsRecoveryCompletionContext_t *)pContext);

    return rc;
}

/*
 * Using the recovery tables passed, run through the CSRs and CPRs performing final
 * recovery tasks
 */
int32_t iecs_completeClientStateRecovery(ieutThreadData_t *pThreadData,
                                         iertTable_t *CSRTable,
                                         iertTable_t *CPRTable,
                                         bool partialRecoveryAllowed)
{
    int32_t rc;

    ieutTRACEL(pThreadData, CPRTable,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "CSRTable=%p, CPRTable=%p, partialRecoveryAllowed=%d\n",
               __func__, CSRTable, CPRTable, (int32_t)partialRecoveryAllowed);

    /* Publish will messages and other associated post-recovery actions */
    rc = iert_iterateOverTable(pThreadData,
                               CPRTable,
                               iecs_completeCPRRecovery,
                               NULL);

    if (rc != OK) goto mod_exit;

    /* Complete client-state post-recovery actions */
    iecsRecoveryCompletionContext_t context = {0};

    rc = iert_iterateOverTable(pThreadData,
                               CSRTable,
                               iecs_completeCSRRecovery,
                               &context);

    /* Wait for any asynchronous operations performed to complete */
    int32_t waitRC = ieut_waitForRemainingActions(pThreadData,
                                                  &context.remainingActions,
                                                  __func__, 10);

    if (partialRecoveryAllowed == false && rc == OK) rc = waitRC;

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d \n", __func__, rc);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of clientStateUtils.c                                         */
/*                                                                   */
/*********************************************************************/
