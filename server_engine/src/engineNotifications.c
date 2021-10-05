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
/// @file  engineNotifications.c
/// @brief Engine component notification functions
//****************************************************************************
#define TRACE_COMP Engine

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "engineInternal.h"
#include "topicTree.h"
#include "memHandler.h"
#include "clientState.h"
#include "queueCommon.h"
#include "engineNotifications.h"

#define ienf_allocBufferCopy(buf,str) ism_common_allocBufferCopyLen((buf), (str), strlen((str)))

// Remember the clientId of the first client not processed during a failing
// scan (a failing scan being one where we could not publish a result). This
// is then used on the next scan to try and pick up where we left off.
static char *prevScanFailedClientId = NULL;

int32_t ienf_publishNotificationMessage(const char *topicString,
                                        void *payload,
                                        size_t payloadSize)
{
    int32_t rc = OK;

    ismEngine_MessageHandle_t hMessage = NULL;
    ismMessageHeader_t hdr = {0};

    hdr.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
    hdr.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
    hdr.Priority = ismMESSAGE_PRIORITY_DEFAULT;
    hdr.Flags = ismMESSAGE_FLAGS_NONE;
    hdr.MessageType = MTYPE_MQTT_Text;

    // Create the Topic Properties expected for the message
    size_t tlen = strlen(topicString)+5;

    assert(tlen<=248); // Can only handle short topic strings this way

    char topicProps[tlen];

    topicProps[0] = S_ID+1;
    topicProps[1] = ID_Topic;
    topicProps[2] = S_StrLen+1;
    topicProps[3] = (int)tlen-4;       /* Length of the topic string (including the null) */
    strcpy(topicProps+4, topicString); /* The topic string (including the null) */

    // Build the area arrays from properties and payload
    ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
    size_t areaLengths[2];
    void *areaData[2];

    areaLengths[0] = tlen;
    areaData[0] = topicProps;
    areaLengths[1] = payloadSize;
    areaData[1] = payload;

    // Create the message
    rc = ism_engine_createMessage(&hdr,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);

    if (rc != OK)
    {
       ism_common_setError(rc);
       goto mod_exit;
    }

    // Now publish the message we created
    rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                    topicString,
                                                    hMessage,
                                                    NULL, 0, NULL);

    if (rc == ISMRC_AsyncCompletion) rc = OK;

    if (rc != OK)
    {
        assert(rc != ISMRC_SomeDestinationsFull);
        ism_common_setError(rc);
        goto mod_exit;
    }

mod_exit:

    return rc;
}


static bool ienf_getClientStates(ieutThreadData_t *pThreadData,
                                 ismEngine_ClientState_t *pClient,
                                 void *context)
{
    ienfClientStatesContext_t *pContext = (ienfClientStatesContext_t *)context;

    // If we're interested in disconnected client notification subscriptions only,
    // ignore clients that aren't zombies
    if (pContext->DCNClients && pClient->OpState != iecsOpStateZombie)
    {
        goto mod_exit;
    }

    // At the moment we only do disconnected client notification for MQTT clients.
    // If this changes, this test must change.
    if (pClient->protocolId != PROTOCOL_ID_MQTT)
    {
        goto mod_exit;
    }

    // Make sure we've got space for this clientId and userId in the list
    if (pContext->resultCount == pContext->resultSize)
    {
        char **newResults;
        uint32_t newResultSize = pContext->resultSize + 10000;

        newResults = iemem_realloc(pThreadData,
                                   IEMEM_PROBE(iemem_notificationData, 1),
                                   pContext->results,
                                   newResultSize * sizeof(char *));

        if (newResults == NULL)
        {
            pContext->rc = ISMRC_AllocateError;
            ism_common_setError(pContext->rc);
            goto mod_exit;
        }

        pContext->results = newResults;
        pContext->resultSize = newResultSize;
    }

    // If the previous scan failed at this client ID start from here
    if (pContext->startIndex == 0 &&
        prevScanFailedClientId != NULL &&
        strcmp(pClient->pClientId, prevScanFailedClientId) == 0)
    {
        pContext->startIndex = pContext->resultCount;
    }

    size_t clientIdLength = strlen(pClient->pClientId)+1;

    size_t userIdLength;
    if (pClient->pUserId != NULL)
    {
        userIdLength = strlen(pClient->pUserId)+1;
    }
    else
    {
        userIdLength = 0;
    }

    char *newIdstrings = iemem_malloc(pThreadData,
                                      IEMEM_PROBE(iemem_notificationData, 2),
                                      clientIdLength + userIdLength);

    if (newIdstrings == NULL)
    {
        pContext->rc = ISMRC_AllocateError;
        ism_common_setError(pContext->rc);
        goto mod_exit;
    }

    char *tmpPtr = newIdstrings;

    memcpy(tmpPtr, pClient->pClientId, clientIdLength);
    pContext->results[pContext->resultCount] = tmpPtr;
    tmpPtr += clientIdLength;
    pContext->resultCount++;

    if (userIdLength != 0)
    {
        memcpy(tmpPtr, pClient->pUserId, userIdLength);
        pContext->results[pContext->resultCount] = tmpPtr;
    }
    else
    {
        pContext->results[pContext->resultCount] = NULL;
    }
    pContext->resultCount++;

mod_exit:

    return (pContext->rc == OK) ? true : false;
}

static void ienf_buildDCNMessage(ieutThreadData_t *pThreadData,
                                 ismEngine_SubscriptionHandle_t  subHandle,
                                 const char *pSubName,
                                 const char *pTopicString,
                                 void *properties,
                                 size_t propertiesLength,
                                 const ismEngine_SubscriptionAttributes_t *pSubAttributes,
                                 uint32_t consumerCount,
                                 void *context)
{
    ienfBuildDCNMessageContext_t *pContext = (ienfBuildDCNMessageContext_t *)context;
    ismEngine_Subscription_t *subscription = (ismEngine_Subscription_t *)subHandle;

    ismEngine_QueueStatistics_t *stats;

    if (pContext->subsCount == pContext->subsMax)
    {
        uint32_t newSubsMax = pContext->subsMax + 10;

        ienfSubscription_t *newSubsIncluded = iemem_realloc(pThreadData,
                                                            IEMEM_PROBE(iemem_notificationData, 3),
                                                            pContext->subsIncluded,
                                                            sizeof(ienfSubscription_t)*newSubsMax);

        if (newSubsIncluded == NULL)
        {
            pContext->rc = ISMRC_AllocateError;
            ism_common_setError(pContext->rc);
            goto mod_exit;
        }

        pContext->subsIncluded = newSubsIncluded;
        pContext->subsMax = newSubsMax;
    }

    // Add this subscription to our list of included subscriptions
    iett_acquireSubscriptionReference(subscription);
    pContext->subsIncluded[pContext->subsCount].subHandle = subHandle;
    stats = &pContext->subsIncluded[pContext->subsCount].stats;
    pContext->subsCount += 1;

    // Request stats to use in the message
    ieq_getStats(pThreadData, subscription->queueHandle, stats);

    // Make sure we've got space for this subscription in the results
    if (pContext->payload.used == 0)
    {
        // First time through, trace that we are building a message for this client
        ieutTRACEL(pThreadData, pContext->clientId,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "ClientId: '%s'\n", __func__, pContext->clientId);

        // Client ID for the client
        ienf_allocBufferCopy(&pContext->payload, "{\"ClientId\":");
        ism_json_putString(&pContext->payload, pContext->clientId);

        if (pContext->userId != NULL)
        {
            // User ID for the client
            ienf_allocBufferCopy(&pContext->payload, ",\"UserID\":");
            ism_json_putString(&pContext->payload, pContext->userId);
        }

        // Start the array of results
        ienf_allocBufferCopy(&pContext->payload, ",\"MessagesArrived\":[");
    }
    else
    {
        ienf_allocBufferCopy(&pContext->payload, ",");
    }

    // Add this result
    ienf_allocBufferCopy(&pContext->payload, "{\"TopicString\":");
    ism_json_putString(&pContext->payload, pTopicString);

    ienf_allocBufferCopy(&pContext->payload, ",\"MessageCount\":");
    ism_json_putInteger(&pContext->payload, (int)(stats->BufferedMsgs));

    ienf_allocBufferCopy(&pContext->payload, "}");

mod_exit:

    return;
}

//****************************************************************************
/// @internal
///
/// @brief  Generate the disconnected client notification messages for disconnected
///         MQTT clients for whom messages have arrived.
///
/// @remark The messages are published to topic "$SYS/DisconnectedClientNotification"
///
///         The messages contain information to identify disconnected MQTT clients
///         that have subscriptions to topics on which new messages have been published
///         since the last notification was generated.
///
///         The messages are in the following JSON format:
///
///         { "ClientId":"<CLIENTID>",
///           "UserID":"<USERID>",
///           "MessagesArrived":[ { "TopicString":"<TOPIC>",
///                                 "MessageCount":<MSGCOUNT> },
///                               ... ] }
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_generateDisconnectedClientNotifications(void)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;
    uint64_t totalDCNTopicSubs = ismEngine_serverGlobal.totalDCNTopicSubs;

    ieutTRACEL(pThreadData, totalDCNTopicSubs, ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Don't bother doing all of this work if no-one is actually subscribed
    if (totalDCNTopicSubs == 0) goto mod_exit;

    ienfClientStatesContext_t clientStateContext = {0};
    clientStateContext.DCNClients = true;

    // Visit all client states in the table
    (void)iecs_traverseClientStateTable(pThreadData,
                                        NULL, 0, 0, NULL,
                                        ienf_getClientStates,
                                        &clientStateContext);

    rc = clientStateContext.rc;

    // We now want to forget the last failing client ID.
    if (prevScanFailedClientId != NULL)
    {
        // Free clientId string of the previous failing scan
        iemem_free(pThreadData, iemem_notificationData, prevScanFailedClientId);
        prevScanFailedClientId = NULL;
    }

    // We got some results - so now process them.
    if (clientStateContext.resultCount != 0)
    {
        uint32_t clientResult = clientStateContext.startIndex;
        ienfBuildDCNMessageContext_t buildDCNMessageContext = {0};

        do
        {
            bool thisClientFailed = false;

            buildDCNMessageContext.rc = OK;
            buildDCNMessageContext.clientId = clientStateContext.results[clientResult];
            buildDCNMessageContext.userId = clientStateContext.results[clientResult+1];
            buildDCNMessageContext.subsCount = 0;

            if (rc == OK)
            {
                // Find subscriptions which require a disconnected client notification
                // message because DCN is enabled, and new messages have arrived since
                // the last call.
                rc = iett_listSubscriptions(pThreadData,
                                            buildDCNMessageContext.clientId,
                                            iettFLAG_LISTSUBS_MATCH_DCNMSGNEEDED,
                                            NULL,
                                            &buildDCNMessageContext,
                                            ienf_buildDCNMessage);

                if (rc == OK) rc = buildDCNMessageContext.rc;

                // We built a message - so now we need to publish it.
                if (rc == OK && buildDCNMessageContext.payload.used != 0)
                {
                    // End the message, closing the array of topics and the entire JSON string
                    ienf_allocBufferCopy(&buildDCNMessageContext.payload, "]}");

                    rc = ienf_publishNotificationMessage(ienfDCN_TOPIC,
                                                         buildDCNMessageContext.payload.buf,
                                                         buildDCNMessageContext.payload.used);

                    // Reuse the buffer for the next message
                    buildDCNMessageContext.payload.used = 0;
                }

                // We have some subscriptions to release
                for(uint32_t subIndex=0; subIndex<buildDCNMessageContext.subsCount; subIndex++)
                {
                    ismEngine_Subscription_t *subscription = (ismEngine_Subscription_t *)buildDCNMessageContext.subsIncluded[subIndex].subHandle;

                    // If everything worked, we need to update the PutsAttempted stat for this subscription
                    if (rc == OK)
                    {
                        ieq_setStats(subscription->queueHandle,
                                     &(buildDCNMessageContext.subsIncluded[subIndex].stats),
                                     ieqSetStats_UPDATE_PUTSATTEMPTED);
                    }

                    // Don't need to keep a reference to this subscription any longer
                    iett_releaseSubscription(pThreadData, subscription, false);
                }

                if (rc != OK) thisClientFailed = true;
            }

            // If we failed to publish to this clientId we will try and start from it next scan
            if (thisClientFailed)
            {
                prevScanFailedClientId = buildDCNMessageContext.clientId;
            }
            // Free this clientId string (which includes the data for the userId too)
            else
            {
                iemem_free(pThreadData, iemem_notificationData, buildDCNMessageContext.clientId);
            }

            // Move to the next pair of results, wrapping to the beginning
            if (clientResult+2 == clientStateContext.resultCount)
            {
                clientResult = 0;
            }
            else
            {
                clientResult += 2;
            }
        }
        // When we've wrapped around, stop
        while(clientResult != clientStateContext.startIndex);

        if (buildDCNMessageContext.payload.buf != NULL)
        {
            ism_common_freeAllocBuffer(&buildDCNMessageContext.payload);
        }

        if (buildDCNMessageContext.subsIncluded != NULL)
        {
            iemem_free(pThreadData, iemem_notificationData, buildDCNMessageContext.subsIncluded);
        }

        iemem_free(pThreadData, iemem_notificationData, clientStateContext.results);
    }
    else
    {
        assert(clientStateContext.results == NULL);
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of engineMonitoring.c                                         */
/*                                                                   */
/*********************************************************************/
