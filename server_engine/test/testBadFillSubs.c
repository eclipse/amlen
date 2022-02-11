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
/********************************************************************/
/*                                                                  */
/* Module Name: testFillSubs                                        */
/*                                                                  */
/* Description: Test what happens when subscriptions fill up.       */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <getopt.h>
#include <sys/prctl.h>

#include <ismutil.h>
#include "engine.h"
#include "engineInternal.h"
#include "engineCommon.h"
#include "policyInfo.h"
#include "queueCommon.h"
#include "topicTree.h"
#include "engineTraceDump.h"

#include "test_utils_initterm.h"
#include "test_utils_log.h"
#include "test_utils_task.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_security.h"
#include "test_utils_client.h"
#include "test_utils_message.h"

/********************************************************************/
/* Global data                                                      */
/********************************************************************/
static uint32_t logLevel = testLOGLEVEL_CHECK;

typedef struct tag_PublisherInfo_t {
    char *topicString;
    bool keepGoing;
    int pubNo;
} PublisherInfo_t;

typedef struct tag_msgCallbackContext_t
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    volatile uint32_t received;
    volatile uint32_t receivedWithDeliveryId;
    volatile uint32_t redelivered;
    uint8_t reliability;
    uint32_t maxDelay;
    uint32_t confirmAfter;
    uint32_t confirmsCompleted;
    ismEngine_DeliveryHandle_t *hDeliveries;
    uint32_t deliveryCount;
    uint64_t lastConfirmTimeNanos;
} msgCallbackContext_t;

void *publishToTopic(void *args)
{
    PublisherInfo_t *pubInfo =(PublisherInfo_t *)args;
    char clientId[50];
    sprintf(clientId, "PubbingClient%d", pubInfo->pubNo);

    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)clientId);
    int32_t rc = OK;

    ism_engine_threadInit(0);

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ism_listener_t *pubbingListener=NULL;
    ism_transport_t *pubbingTransport=NULL;
    ismSecurity_t *pubbingContext;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    rc = test_createSecurityContext(&pubbingListener,
                                    &pubbingTransport,
                                    &pubbingContext);
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));


    pubbingTransport->clientID = clientId;

    rc = test_createClientAndSession(pubbingTransport->clientID,
                                     pubbingContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    uint64_t pubCount=0;

    while(pubInfo->keepGoing)
    {
        ismEngine_MessageHandle_t hMessage;

        // TODO: Mess around with QoS / Persistence
        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                rand()%100>25 ? ismMESSAGE_PERSISTENCE_PERSISTENT : ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                rand()%100>50 ? ismMESSAGE_RELIABILITY_AT_LEAST_ONCE : ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, pubInfo->topicString,
                                &hMessage, NULL);
        TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                pubInfo->topicString,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

        pubCount++;

        if (pubCount%5000000 == 0)
        {
            printf("Thread %d has done %lu msgs (processedJobs=%lu)\n",
                   pubInfo->pubNo, pubCount, pThreadData->processedJobs);
        }
    }

    return NULL;
}

typedef struct tag_consumeAfterReceiveContext_t
{
    ismEngine_SessionHandle_t hSession;
    ismEngine_DeliveryHandle_t *hDeliveries;
    uint32_t deliveryCount;
    uint32_t receivedCount;
    msgCallbackContext_t *mcContext;
} consumeAfterReceiveContext_t;

//If we mark a message received and it goes async, this function marks it consumed afterwards
void consumeAfterReceived(int32_t rc, void *handle, void *context)
{
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
    consumeAfterReceiveContext_t *pDlvInfo = (consumeAfterReceiveContext_t *)context;

#if 0
    rc = ism_engine_confirmMessageDeliveryBatch(pDlvInfo->hSession,
                                                NULL,
                                                pDlvInfo->deliveryCount,
                                                pDlvInfo->hDeliveries,
                                                ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                NULL, 0, NULL);
    TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    __sync_add_and_fetch(&pDlvInfo->mcContext->confirmsCompleted, 1);

    free(pDlvInfo->hDeliveries);
#else
    uint32_t thisRcvdCount = __sync_add_and_fetch(&pDlvInfo->receivedCount, 1);

    if (thisRcvdCount == pDlvInfo->deliveryCount)
    {
        for(int32_t i=0; i<thisRcvdCount; i++)
        {
            rc = ism_engine_confirmMessageDelivery(pDlvInfo->hSession,
                                                   NULL,
                                                   pDlvInfo->hDeliveries[i],
                                                   ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                   NULL, 0, NULL);
            TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
        }

        __sync_add_and_fetch(&pDlvInfo->mcContext->confirmsCompleted, 1);

        free(pDlvInfo->hDeliveries);
    }
#endif
}

bool test_msgCallback(ismEngine_ConsumerHandle_t      hConsumer,
                      ismEngine_DeliveryHandle_t      hDelivery,
                      ismEngine_MessageHandle_t       hMessage,
                      uint32_t                        deliveryId,
                      ismMessageState_t               state,
                      uint32_t                        destinationOptions,
                      ismMessageHeader_t *            pMsgDetails,
                      uint8_t                         areaCount,
                      ismMessageAreaType_t            areaTypes[areaCount],
                      size_t                          areaLengths[areaCount],
                      void *                          pAreaData[areaCount],
                      void *                          pContext,
                      ismEngine_DelivererContext_t *  _delivererContext)
{
    msgCallbackContext_t *context = *((msgCallbackContext_t **)pContext);

    if (context->maxDelay != 0)
    {
        usleep(rand()%context->maxDelay);
    }

    __sync_fetch_and_add(&context->received, 1);

    if (pMsgDetails->RedeliveryCount != 0)
    {
        __sync_fetch_and_add(&context->redelivered, 1);
    }

    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        __sync_fetch_and_add(&context->receivedWithDeliveryId, 1);
    }

    ism_engine_releaseMessage(hMessage);

    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        if (context->confirmAfter != 99999)
        {
            uint64_t now = ism_common_currentTimeNanos();
            context->hDeliveries[context->deliveryCount] = hDelivery;

            context->deliveryCount++;

            if (context->confirmAfter == context->deliveryCount ||
                now > context->lastConfirmTimeNanos+30000000000UL)
            {
                consumeAfterReceiveContext_t dlyInfo = {context->hSession,
                                                        context->hDeliveries,
                                                        context->deliveryCount,
                                                        0,
                                                        context};

                if (context->confirmAfter != context->deliveryCount)
                {
                    // printf("WHOOP %s now:%lu lctn:%lu\n", context->hClient->pClientId, now, context->lastConfirmTimeNanos);
                }

                context->lastConfirmTimeNanos = now;

                if (context->reliability == ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE)
                {
                    int32_t rc;
#if 0
                    rc = ism_engine_confirmMessageDeliveryBatch(context->hSession,
                                                                NULL,
                                                                context->deliveryCount,
                                                                context->hDeliveries,
                                                                ismENGINE_CONFIRM_OPTION_RECEIVED,
                                                                &dlyInfo, sizeof(dlyInfo),
                                                                consumeAfterReceived);
                    TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

                    if (rc != ISMRC_AsyncCompletion)
                    {
                        consumeAfterReceived(rc, NULL, &dlyInfo);
                    }
#else
                    for(int32_t i=0; i<context->deliveryCount; i++)
                    {
                        rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                               NULL,
                                                               context->hDeliveries[i],
                                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                               &dlyInfo, sizeof(dlyInfo),
                                                               consumeAfterReceived);
                        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

                        if (rc != ISMRC_AsyncCompletion)
                        {
                            consumeAfterReceived(rc, NULL, &dlyInfo);
                        }
                    }
#endif
                }
                else
                {
                    dlyInfo.receivedCount = dlyInfo.deliveryCount-1;
                    consumeAfterReceived(OK, NULL, &dlyInfo);
                }

                context->hDeliveries = malloc(sizeof(ismEngine_DeliveryHandle_t) * context->confirmAfter);
                context->deliveryCount = 0;
            }
        }
    }

    return true; // more messages, please. TODO: Anything to play with here?
}

/****************************************************************************/
/* ProcessArgs                                                              */
/****************************************************************************/
int ProcessArgs(int argc, char **argv)
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { NULL, 1, NULL, 0 } };
    int long_index;

    while ((opt = getopt_long(argc, argv, "v:", long_options, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'v':
               logLevel = atoi(optarg);
               if (logLevel > testLOGLEVEL_VERBOSE)
                   logLevel = testLOGLEVEL_VERBOSE;
               break;
            case '?':
                usage=1;
                break;
            default:
                printf("%c\n", (char)opt);
                usage=1;
                break;
        }
    }

    if (usage)
    {
        fprintf(stderr, "Usage: %s [-v verbose] [-t testcase 1-4]\n", argv[0]);
        fprintf(stderr, "       -v - logLevel 0-5 [2]\n");
        fprintf(stderr, "\n");
    }

    return usage;
}

#define NUMBER_OF_BUS_CONNECTOR_SUBS                    0 //1
#define NUMBER_OF_CONSUMERS_PER_BUS_CONNECTOR_SUB       10
#define NUMBER_OF_OTHER_SHARED_SUBS                     1 //10
#define NUMBER_OF_CONSUMERS_PER_OTHER_SHARED_SUB        5
#define NUMBER_OF_OTHER_SHARED_SUBS_TO_DISPLAY          1
#define NUMBER_OF_INDIVIDUAL_SUBS                       0 //2
#define NUMBER_OF_INDIVIDUAL_SUBS_TO_DISPLAY            1

/********************************************************************/
/* MAIN                                                             */
/********************************************************************/
int main(int argc, char *argv[])
{
    int trclvl = 5;
    int rc, rc2;
    ismEngine_ClientStateHandle_t hOwningClient, hBusConnectorClient, hOtherClient;
    ismEngine_SessionHandle_t hBusConnectorSession, hOtherClientSession;
    ism_listener_t *busConnectorListener=NULL, *otherClientListener=NULL;
    ism_transport_t *busConnectorTransport=NULL, *otherClientTransport=NULL;
    ismSecurity_t *busConnectorContext, *otherClientContext;

    /************************************************************************/
    /* Parse the command line arguments                                     */
    /************************************************************************/
    rc = ProcessArgs(argc, argv);

    if (rc != OK) goto mod_exit_noProcessTerm;

    test_setLogLevel(logLevel);

    rc = test_processInit(trclvl, NULL);

    if (rc != OK) goto mod_exit_noProcessTerm;

    rc =  test_engineInit(true, false,
                          ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                          false, /*recovery should complete ok*/
                          ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                          1024);

    if (rc != OK) goto mod_exit_noEngineTerm;

    sslInit();

    printf("Starting %s...\n", __func__);

    rc = test_createSecurityContext(&busConnectorListener,
                                    &busConnectorTransport,
                                    &busConnectorContext);
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    busConnectorTransport->clientID = "BusConnector";
    ((mock_ismSecurity_t *)busConnectorContext)->ExpectedMsgRate = EXPECTEDMSGRATE_HIGH;

    rc = test_createSecurityContext(&otherClientListener,
                                    &otherClientTransport,
                                    &otherClientContext);
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    otherClientTransport->clientID = "OtherClient";

    /* Create our clients and sessions */
    rc = ism_engine_createClientState("__Shared_TEST",
                                      PROTOCOL_ID_SHARED,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hOwningClient,
                                      NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    rc = test_createClientAndSession(busConnectorTransport->clientID,
                                     busConnectorContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hBusConnectorClient, &hBusConnectorSession, true);
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    rc = test_createClientAndSession(otherClientTransport->clientID,
                                     otherClientContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hOtherClient, &hOtherClientSession, true);
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    // Allow everyone to publish & subscribe on any topic
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.TP_ALL\","
                                "\"Name\":\"TP_ALL\","
                                "\"ClientID\":\"*\","
                                "\"Topic\":\"*\","
                                "\"ActionList\":\"publish,subscribe\","
                                "\"MaxMessages\":5000,"
                                "\"MaxMessagesBehavior\":\"DiscardOldMessages\"}");
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
    rc = test_addPolicyToSecContext(busConnectorContext, "TP_ALL");
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
    rc = test_addPolicyToSecContext(otherClientContext, "TP_ALL");
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    // Allow 100,000 messages on the 'bus connector' subscription
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY,
                                "{\"UID\":\"UID.SP_BUSCONNECTOR\","
                                "\"Name\":\"SP_BUSCONNECTOR\","
                                "\"ClientID\":\"BusConnector\","
                                "\"ActionList\":\"control,receive\","
                                "\"MaxMessages\":100000,"
                                "\"MaxMessagesBehavior\":\"DiscardOldMessages\"}");
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
    rc = test_addPolicyToSecContext(busConnectorContext, "SP_BUSCONNECTOR");
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
    rc = test_addPolicyToSecContext(otherClientContext, "SP_BUSCONNECTOR");
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    // Allow 5,000 messages on the 'other' subscription
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY,
                                "{\"UID\":\"UID.SP_OTHERCLIENT\","
                                "\"Name\":\"SP_OTHERCLIENT\","
                                "\"ClientID\":\"OtherClient\","
                                "\"ActionList\":\"control,receive\","
                                "\"MaxMessages\":5000,"
                                "\"MaxMessagesBehavior\":\"DiscardOldMessages\"}");
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
    rc = test_addPolicyToSecContext(busConnectorContext, "SP_OTHERCLIENT");
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
    rc = test_addPolicyToSecContext(otherClientContext, "SP_OTHERCLIENT");
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    // Create a 'bus connector' durable shared sub
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                    ismENGINE_SUBSCRIPTION_OPTION_DURABLE };

    ieutThreadData_t *pThreadData = ieut_getThreadData();

#if 0
    uint32_t baseConsumerOpts = ismENGINE_CONSUMER_OPTION_NONPERSISTENT_DELIVERYCOUNTING;
#else
    uint32_t baseConsumerOpts = ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID;
#endif

#if NUMBER_OF_BUS_CONNECTOR_SUBS
    TEST_ASSERT(NUMBER_OF_BUS_CONNECTOR_SUBS == 1, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    ismEngine_Subscription_t *busConnectorSub;

    rc = sync_ism_engine_createSubscription(hBusConnectorClient,
                                            "BusConnector",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "#",
                                            &subAttrs,
                                            hOwningClient);
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    rc = iett_findClientSubscription(pThreadData,
                                     "__Shared_TEST",
                                     iett_generateClientIdHash("__Shared_TEST"),
                                     "BusConnector",
                                     iett_generateSubNameHash("BusConnector"),
                                     &busConnectorSub);
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
    iett_releaseSubscription(pThreadData, busConnectorSub, true);

    // Create some consumers on the BusConnector subscription
    msgCallbackContext_t BusConnectorMsgContext[NUMBER_OF_CONSUMERS_PER_BUS_CONNECTOR_SUB];
    for(int32_t i=0; i<NUMBER_OF_CONSUMERS_PER_BUS_CONNECTOR_SUB; i++)
    {
        char clientId[50];

        sprintf(clientId, "BCConsumer%02d", i);

        // Create a separate client & session
        rc = test_createClientAndSession(clientId,
                                         busConnectorContext,
                                         ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &BusConnectorMsgContext[i].hClient,
                                         &BusConnectorMsgContext[i].hSession,
                                         true);
        TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

        uint32_t maxInflight = BusConnectorMsgContext[i].hClient->maxInflightLimit;
        TEST_ASSERT(maxInflight > 10, ("Assertion failed in %s line %d [maxInflight=%u]",
                    __func__, __LINE__, maxInflight));

        BusConnectorMsgContext[i].received = 0;
        BusConnectorMsgContext[i].receivedWithDeliveryId = 0;
        BusConnectorMsgContext[i].redelivered = 0;
        BusConnectorMsgContext[i].reliability = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
        BusConnectorMsgContext[i].confirmAfter = (rand()%(maxInflight-10))+1;
        BusConnectorMsgContext[i].hDeliveries = malloc(sizeof(ismEngine_DeliveryHandle_t) * BusConnectorMsgContext[i].confirmAfter);
        BusConnectorMsgContext[i].deliveryCount = 0;
        BusConnectorMsgContext[i].confirmsCompleted = 0;
        BusConnectorMsgContext[i].maxDelay = rand()%100;
        BusConnectorMsgContext[i].lastConfirmTimeNanos = ism_common_currentTimeNanos();

        rc = ism_engine_reuseSubscription(BusConnectorMsgContext[i].hClient,
                                          "BusConnector",
                                          &subAttrs,
                                          hOwningClient);
        TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

        msgCallbackContext_t *pMsgContext = &BusConnectorMsgContext[i];
        rc = ism_engine_createConsumer(BusConnectorMsgContext[i].hSession,
                                       ismDESTINATION_SUBSCRIPTION,
                                       "BusConnector",
                                       NULL,
                                       hOwningClient,
                                       &pMsgContext, sizeof(pMsgContext), test_msgCallback,
                                       NULL,
                                       baseConsumerOpts | ismENGINE_CONSUMER_OPTION_ACK,
                                       &BusConnectorMsgContext[i].hConsumer,
                                       NULL, 0, NULL);
        TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
    }
#endif

#if NUMBER_OF_INDIVIDUAL_SUBS
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;

    // Create some consumers on TestTopic
    msgCallbackContext_t TopicConsumerMsgContext[NUMBER_OF_INDIVIDUAL_SUBS];
    for(int32_t i=0; i<NUMBER_OF_INDIVIDUAL_SUBS; i++)
    {
        char clientId[50];

        sprintf(clientId, "TopicConsumer%02d", i);

        // Create a separate client & session
        rc = test_createClientAndSession(clientId,
                                         otherClientContext,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &TopicConsumerMsgContext[i].hClient,
                                         &TopicConsumerMsgContext[i].hSession,
                                         true);
        TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

        TopicConsumerMsgContext[i].received = 0;
        TopicConsumerMsgContext[i].receivedWithDeliveryId = 0;
        TopicConsumerMsgContext[i].redelivered = 0;
        TopicConsumerMsgContext[i].reliability = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
        TopicConsumerMsgContext[i].confirmAfter = (rand()%90)+1;
        TopicConsumerMsgContext[i].hDeliveries = malloc(sizeof(ismEngine_DeliveryHandle_t) * TopicConsumerMsgContext[i].confirmAfter);
        TopicConsumerMsgContext[i].deliveryCount = 0;
        TopicConsumerMsgContext[i].confirmsCompleted = 0;
        TopicConsumerMsgContext[i].maxDelay = rand()%100;
        TopicConsumerMsgContext[i].lastConfirmTimeNanos = ism_common_currentTimeNanos();

        msgCallbackContext_t *pMsgContext = &TopicConsumerMsgContext[i];
        rc = ism_engine_createConsumer(TopicConsumerMsgContext[i].hSession,
                                       ismDESTINATION_TOPIC,
                                       "TestTopic",
                                       &subAttrs,
                                       NULL,
                                       &pMsgContext, sizeof(pMsgContext), test_msgCallback,
                                       NULL,
                                       baseConsumerOpts | ismENGINE_CONSUMER_OPTION_ACK,
                                       &TopicConsumerMsgContext[i].hConsumer,
                                       NULL, 0, NULL);
        TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
    }
#endif

#if NUMBER_OF_OTHER_SHARED_SUBS
    // Create some further shared subs
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_SHARED | ismENGINE_SUBSCRIPTION_OPTION_DURABLE;

    msgCallbackContext_t OtherConsumerMsgContext[NUMBER_OF_OTHER_SHARED_SUBS * NUMBER_OF_CONSUMERS_PER_OTHER_SHARED_SUB];
    ismEngine_Subscription_t *otherSub[NUMBER_OF_OTHER_SHARED_SUBS];
    int32_t cCount=0;
    printf("osCount=%d\n", NUMBER_OF_OTHER_SHARED_SUBS);
    for(int32_t i=0; i<NUMBER_OF_OTHER_SHARED_SUBS; i++)
    {
        char subName[50];

        sprintf(subName, "OtherSub%02d", i);
        rc = sync_ism_engine_createSubscription(hOtherClient,
                                                subName,
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                "TestTopic",
                                                &subAttrs,
                                                hOwningClient);
        TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

        rc = iett_findClientSubscription(pThreadData,
                                         "__Shared_TEST",
                                         iett_generateClientIdHash("__Shared_TEST"),
                                         subName,
                                         iett_generateSubNameHash(subName),
                                         &otherSub[i]);
        TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
        iett_releaseSubscription(pThreadData, otherSub[i], true);

        // Create some consumers on this shared sub
        int32_t j=cCount;
        for(; j<cCount+NUMBER_OF_CONSUMERS_PER_OTHER_SHARED_SUB; j++)
        {
            char clientId[50];

            sprintf(clientId, "OSConsumer%02d", j);

            // Create a separate client & session
            rc = test_createClientAndSession(clientId,
                                             otherClientContext,
                                             ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                             ismENGINE_CREATE_SESSION_OPTION_NONE,
                                             &OtherConsumerMsgContext[j].hClient,
                                             &OtherConsumerMsgContext[j].hSession,
                                             true);
            TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

            uint32_t maxInflight = OtherConsumerMsgContext[j].hClient->maxInflightLimit;
            TEST_ASSERT(maxInflight > 10, ("Assertion failed in %s line %d [maxInflight=%u]",
                        __func__, __LINE__, maxInflight));

            OtherConsumerMsgContext[j].received = 0;
            OtherConsumerMsgContext[j].receivedWithDeliveryId = 0;
            OtherConsumerMsgContext[j].redelivered = 0;
            OtherConsumerMsgContext[j].reliability = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
            OtherConsumerMsgContext[j].confirmAfter = (rand()%(maxInflight-10))+1;
            OtherConsumerMsgContext[j].hDeliveries = malloc(sizeof(ismEngine_DeliveryHandle_t) * OtherConsumerMsgContext[j].confirmAfter);
            OtherConsumerMsgContext[j].deliveryCount = 0;
            OtherConsumerMsgContext[j].confirmsCompleted = 0;
            OtherConsumerMsgContext[j].maxDelay = rand()%100;
            OtherConsumerMsgContext[j].lastConfirmTimeNanos = ism_common_currentTimeNanos();

            rc = ism_engine_reuseSubscription(OtherConsumerMsgContext[j].hClient,
                                              subName,
                                              &subAttrs,
                                              hOwningClient);
            TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

            msgCallbackContext_t *pMsgContext = &OtherConsumerMsgContext[j];
            rc = ism_engine_createConsumer(OtherConsumerMsgContext[j].hSession,
                                           ismDESTINATION_SUBSCRIPTION,
                                           subName,
                                           NULL,
                                           hOwningClient,
                                           &pMsgContext, sizeof(pMsgContext), test_msgCallback,
                                           NULL,
                                           baseConsumerOpts | ismENGINE_CONSUMER_OPTION_ACK,
                                           &OtherConsumerMsgContext[j].hConsumer,
                                           NULL, 0, NULL);
            TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
        }

        cCount=j;
    }
#endif

    pthread_t publishingThreadId[20];
    PublisherInfo_t pubberInfo[20];

    // Publish on 20 threads with a mixture of QoSs
    for(int32_t i=0; i<20; i++)
    {
        pubberInfo[i].keepGoing = true;
        pubberInfo[i].topicString = "TestTopic";
        pubberInfo[i].pubNo = i;
        rc = test_task_startThread( &publishingThreadId[i],publishToTopic, &pubberInfo[i],"publishToTopic");
        TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
    }

    uint32_t loopCount=0;
    uint32_t test_traceDumpInterval = 0;
    while(1)
    {
        sleep(1);

        if (loopCount%5 == 0)
        {
            ismEngine_QueueStatistics_t stats;
#if NUMBER_OF_BUS_CONNECTOR_SUBS
            ieq_getStats(pThreadData, busConnectorSub->queueHandle, &stats);
            printf("%s BufferedMsgs:%lu BufferedMsgsHWM:%lu RejectedMsgs:%lu DequeueCount:%lu DiscardedMsgs:%lu\n",
                   busConnectorSub->subName,
                   stats.BufferedMsgs, stats.BufferedMsgsHWM, stats.RejectedMsgs, stats.DequeueCount, stats.DiscardedMsgs);
            printf("BCConsumers:");
            for(int32_t j=0; j<sizeof(BusConnectorMsgContext)/sizeof(BusConnectorMsgContext[0]); j++)
            {
                printf(" %d %d/%d[%d]", BusConnectorMsgContext[j].received,
                                        BusConnectorMsgContext[j].deliveryCount,
                                        BusConnectorMsgContext[j].confirmAfter,
                                        BusConnectorMsgContext[j].confirmsCompleted);
            }
            printf("\n");
#endif

            int32_t startIndex;

#if NUMBER_OF_OTHER_SHARED_SUBS
#if (NUMBER_OF_OTHER_SHARED_SUBS_TO_DISPLAY == NUMBER_OF_OTHER_SHARED_SUBS)
            startIndex = 0;
#else
            startIndex = rand()%(NUMBER_OF_OTHER_SHARED_SUBS-NUMBER_OF_OTHER_SHARED_SUBS_TO_DISPLAY);
#endif

            for(int32_t i=startIndex; i<startIndex+NUMBER_OF_OTHER_SHARED_SUBS_TO_DISPLAY; i++)
            {
                ieq_getStats(pThreadData, otherSub[i]->queueHandle, &stats);
                printf("%s BufferedMsgs:%lu BufferedMsgsHWM:%lu RejectedMsgs:%lu DequeueCount:%lu DiscardedMsgs:%lu\n",
                       otherSub[i]->subName,
                       stats.BufferedMsgs, stats.BufferedMsgsHWM, stats.RejectedMsgs, stats.DequeueCount, stats.DiscardedMsgs);
                printf("OSConsumers%02d:", i);
                int32_t startConsumer = i*NUMBER_OF_CONSUMERS_PER_OTHER_SHARED_SUB;
                for(int32_t j=startConsumer; j<startConsumer+NUMBER_OF_CONSUMERS_PER_OTHER_SHARED_SUB; j++)
                {
                    printf(" %d %d/%d[%d]", OtherConsumerMsgContext[j].received,
                                            OtherConsumerMsgContext[j].deliveryCount,
                                            OtherConsumerMsgContext[j].confirmAfter,
                                            OtherConsumerMsgContext[j].confirmsCompleted);
                }
                printf("\n");
            }
#endif

#if NUMBER_OF_INDIVIDUAL_SUBS
#if (NUMBER_OF_INDIVIDUAL_SUBS_TO_DISPLAY == NUMBER_OF_INDIVIDUAL_SUBS)
            startIndex = 0;
#else
            startIndex = rand()%(NUMBER_OF_INDIVIDUAL_SUBS-(NUMBER_OF_INDIVIDUAL_SUBS_TO_DISPLAY-1));
#endif

            for(int32_t i=startIndex; i<startIndex+NUMBER_OF_INDIVIDUAL_SUBS_TO_DISPLAY; i++)
            {
                ieq_getStats(pThreadData, TopicConsumerMsgContext[i].hConsumer->queueHandle, &stats);
                printf("Individual%02d BufferedMsgs:%lu BufferedMsgsHWM:%lu RejectedMsgs:%lu DequeueCount:%lu DiscardedMsgs:%lu\n",
                       i, stats.BufferedMsgs, stats.BufferedMsgsHWM, stats.RejectedMsgs, stats.DequeueCount, stats.DiscardedMsgs);
                printf("IndConsumers: %d %d/%d [%d]\n", TopicConsumerMsgContext[i].received,
                                                        TopicConsumerMsgContext[i].deliveryCount,
                                                        TopicConsumerMsgContext[i].confirmAfter,
                                                        TopicConsumerMsgContext[i].confirmsCompleted);
            }
#endif
        }

        loopCount++;

        if (test_traceDumpInterval != 0)
        {
            if (loopCount == 1 || loopCount%test_traceDumpInterval == 0)
            {
                char fileName[50];
                char *password = "password";
                char *filePath = NULL;

                sprintf(fileName, "trcDump_%s_%d", password, loopCount/test_traceDumpInterval);
                ietd_dumpInMemoryTrace( pThreadData, fileName, password, &filePath);

                printf("TraceDump written to %s\n", filePath);
                iemem_free(pThreadData, iemem_diagnostics, filePath);
            }
        }
    }

    sslTerm();

    rc2 = test_engineTerm(true);
    if (rc2 != OK)
    {
        if (rc == OK) rc = rc2;
        goto mod_exit_noProcessTerm;
    }

mod_exit_noEngineTerm:

    test_processTerm(true);

mod_exit_noProcessTerm:

    return rc;
}
