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

/*********************************************************************/
/*                                                                   */
/* Module Name: test_sharedSubs.c                                    */
/*                                                                   */
/* Description: Main source file for CUnit Shared Subscription tests */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "ismutil.h"
#include "topicTree.h"
#include "topicTreeInternal.h"
#include "queueCommon.h"
#include "multiConsumerQInternal.h"
#include "engineDiag.h"

#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_assert.h"
#include "test_utils_security.h"
#include "test_utils_log.h"
#include "test_utils_sync.h"
#include "test_utils_pubsuback.h"
#include "test_utils_phases.h"
#include "test_utils_config.h"

typedef struct tag_msgCallbackContext_t
{
    ismEngine_SessionHandle_t hSession;
    volatile uint32_t received;
    volatile uint32_t receivedWithDeliveryId;
    volatile uint32_t redelivered;
    uint8_t reliability;
    bool confirm;
    const char *expectIdentity;
} msgCallbackContext_t;

typedef struct tag_consumeAfterReceiveContext_t
{
    ismEngine_SessionHandle_t hSession;
    ismEngine_DeliveryHandle_t hDelivery;
} consumeAfterReceiveContext_t;

extern int serverState;

//If we mark a message received and it goes async, this function marks it consumed afterwards
void consumeAfterReceived(int32_t rc, void *handle, void *context)
{
    TEST_ASSERT_EQUAL(rc, OK);
    consumeAfterReceiveContext_t *pDlvInfo = (consumeAfterReceiveContext_t *)context;

    rc = ism_engine_confirmMessageDelivery(pDlvInfo->hSession,
            NULL,
            pDlvInfo->hDelivery,
            ismENGINE_CONFIRM_OPTION_CONSUMED,
            NULL, 0, NULL);
    TEST_ASSERT_CUNIT((rc == OK || rc == ISMRC_AsyncCompletion),
                    ("rc was %d", rc));
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
                      void *                          pContext)
{
    msgCallbackContext_t *context = *((msgCallbackContext_t **)pContext);
    int32_t rc;

    if (pMsgDetails->RedeliveryCount != 0)
    {
        __sync_fetch_and_add(&context->redelivered, 1);
    }

    __sync_fetch_and_add(&context->received, 1);

    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        __sync_fetch_and_add(&context->receivedWithDeliveryId, 1);
    }

    ism_engine_releaseMessage(hMessage);

    if (context->expectIdentity != NULL && ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();

        // Test that we can identify the client which owns this hDelivery (which is just Q and Node)
        const char *suspectClientIds[] = {"NOTTHEEXPECTEDCLIENT", "NOTTHEEXPECTEDCLIENT", NULL};
        ismEngine_DeliveryInternal_t hTempHandle = { hDelivery };
        const char *identifiedClient = NULL;

        int32_t checkExpectedIdentity = rand()%3;

        if (checkExpectedIdentity)
        {
            suspectClientIds[checkExpectedIdentity-1] = context->expectIdentity;
        }

        rc = iecs_identifyMessageDeliveryReferenceOwner(pThreadData,
                                                        suspectClientIds,
                                                        hTempHandle.Parts.Q,
                                                        hTempHandle.Parts.Node,
                                                        &identifiedClient);

        if (checkExpectedIdentity)
        {
            TEST_ASSERT_CUNIT((rc == OK), ("rc was %d", rc));
            TEST_ASSERT_CUNIT((strcmp(identifiedClient, context->expectIdentity) == 0),
                              ("identifiedClient was %s", identifiedClient));
        }
        else
        {
            TEST_ASSERT_CUNIT((rc == ISMRC_NotFound), ("rc was %d", rc));
        }
    }

    if (context->confirm && ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        if (context->reliability == ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE)
        {
            consumeAfterReceiveContext_t dlyInfo = {context->hSession, hDelivery};

            rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                   NULL,
                                                   hDelivery,
                                                   ismENGINE_CONFIRM_OPTION_RECEIVED,
                                                   &dlyInfo, sizeof(dlyInfo),
                                                   consumeAfterReceived);
            TEST_ASSERT_CUNIT((rc == OK || rc == ISMRC_AsyncCompletion),
                              ("rc was %d", rc));

            if ( rc == OK)
            {
                rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                       NULL,
                                                       hDelivery,
                                                       ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                       NULL, 0, NULL);

                TEST_ASSERT_CUNIT((rc == OK || rc == ISMRC_AsyncCompletion),
                                                               ("rc was %d", rc));
            }
        }
        else
        {
            rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                   NULL,
                                                   hDelivery,
                                                   ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                   NULL, 0, NULL);
            TEST_ASSERT_CUNIT((rc == OK || rc == ISMRC_AsyncCompletion),
                                                           ("rc was %d", rc));
        }
    }

    return true; // more messages, please.
}

void test_publishMessages(const char *topicString,
                          uint32_t msgCount,
                          uint8_t reliability,
                          uint8_t persistence)
{
    ismEngine_MessageHandle_t hMessage;
    void *payload=NULL;

    for(uint32_t i=0; i<msgCount; i++)
    {
        payload = NULL;

        int32_t rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                        persistence,
                                        reliability,
                                        ismMESSAGE_FLAGS_NONE,
                                        ism_common_nowExpire()+600, // Use 10 minute expiry
                                        ismDESTINATION_TOPIC, topicString,
                                        &hMessage, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                        topicString,
                                                        hMessage,
                                                        NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        if (payload) free(payload);
    }
}

void test_checkSubscriptionClientIds(ieutThreadData_t *pThreadData,
                                     ismEngine_Subscription_t *subscription,
                                     uint32_t expectCount,
                                     const char **expectClients)
{
    const char **foundClients = NULL;

    int32_t rc = iett_findSubscriptionClientIds(pThreadData, subscription, &foundClients);

    if (expectCount == 0)
    {
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        TEST_ASSERT_PTR_NULL(foundClients);
    }
    else
    {
        const char **expectToSee = malloc(expectCount * sizeof(const char *));
        TEST_ASSERT_PTR_NOT_NULL(expectToSee);
        memcpy(expectToSee, expectClients, expectCount * sizeof(const char *));

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(foundClients);
        const char **thisClient = foundClients;
        while(*thisClient != NULL)
        {
            bool seen = false;
            for(int32_t i=0; i<expectCount; i++)
            {
                if (expectToSee[i] != NULL && strcmp(*thisClient, expectToSee[i]) == 0)
                {
                    expectToSee[i] = NULL;
                    seen = true;
                    break;
                }
            }

            TEST_ASSERT_EQUAL(seen, true);
            thisClient++;
        }

        for(int32_t i=0; i<expectCount; i++)
        {
            TEST_ASSERT_PTR_NULL(expectToSee[i]);
        }

        iett_freeSubscriptionClientIds(pThreadData, foundClients);
        free(expectToSee);
    }
}

#define SUBSCRIPTION_NAME "JMSDurableSharedSub"
#define SUBSCRIPTION_TOPIC "JMS/Global/Durable"
#define LOOP_ITERATIONS 5
void test_globalJMSDurable(void)
{
    ismEngine_ClientStateHandle_t hOwningClient;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_Subscription_t *subscription;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    rc = test_bounceEngine();
    TEST_ASSERT_EQUAL(rc, OK);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE | // JMS
                                                    ismENGINE_SUBSCRIPTION_OPTION_SHARED  |
                                                    ismENGINE_SUBSCRIPTION_OPTION_DURABLE };

    for(int32_t loop=0; loop<LOOP_ITERATIONS; loop++)
    {
        // Restart after the 1st iteration
        if (loop != 0)
        {
            test_bounceEngine();
            pThreadData = ieut_getThreadData();
        }

        // Create a client state for globally shared durable subs
        // Note: We create the client state as DURABLE in order to test the DISCARD option
        //       later in the function, there is no need for the client state to be durable
        //       just because it will own durable subscriptions (indeed the JMS code never
        //       creates durable client states).
        rc = sync_ism_engine_createClientState("__Shared_TEST",
                                               PROTOCOL_ID_SHARED,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               NULL, NULL, NULL,
                                               &hOwningClient);
        if (loop == 0)
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);
        }

        // Create a client state for a client
        rc = test_createClientAndSession("C1",
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hClient, &hSession, true);
        TEST_ASSERT_EQUAL(rc, OK);

        // First and last iterations create the subscription - first we expect it
        // to work, last we expect to be told of an existing subscription
        if ((loop == 0) || (loop != (LOOP_ITERATIONS-1)))
        {
            rc = ism_engine_reuseSubscription(hClient,
                                              SUBSCRIPTION_NAME,
                                              &subAttrs,
                                              hOwningClient);
            if (loop == 0)
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, OK);
            }

            rc = sync_ism_engine_createSubscription(hClient,
                                                    SUBSCRIPTION_NAME,
                                                    NULL,
                                                    ismDESTINATION_TOPIC,
                                                    SUBSCRIPTION_TOPIC,
                                                    &subAttrs,
                                                    hOwningClient);
            if (loop == 0)
            {
                TEST_ASSERT_EQUAL(rc, OK);
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_ExistingSubscription);
            }
        }

        rc = iett_findClientSubscription(pThreadData,
                                         "__Shared_TEST",
                                         iett_generateClientIdHash("__Shared_TEST"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(subscription->subOptions,
                          (subAttrs.subOptions & ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK));
        TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER, 0);
        TEST_ASSERT_EQUAL(ieq_getQType(subscription->queueHandle), multiConsumer);

        // Check that the shared subscription data is as expected
        iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
        TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettANONYMOUS_SHARER_JMS_APPLICATION);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 0);

        // Check we get the right list of clients for this subscription
        test_checkSubscriptionClientIds(pThreadData, subscription, 0, NULL);

        // Check that the queue is set up with expected options
        iemqQueue_t *Q = (iemqQueue_t *)(subscription->queueHandle);
        TEST_ASSERT_EQUAL(Q->QOptions & ieqOptions_IN_RECOVERY,0);
        TEST_ASSERT_EQUAL(Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE, ieqOptions_SUBSCRIPTION_QUEUE);
        TEST_ASSERT_EQUAL(Q->QOptions & ieqOptions_SINGLE_CONSUMER_ONLY, 0);

        // Try dumping the topic tree
        rc = test_log_TopicTree(testLOGLEVEL_VERBOSE, "JMS", 9, -1, "");
        TEST_ASSERT_EQUAL(rc, OK);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // All but last iteration, destroy the owning client and check it's durable is left
        if (loop != (LOOP_ITERATIONS-1))
        {
            // First loop, create a _nondurable_ shared sub to check that _it_ gets
            // destroyed by a ism_engine_destroyClientState for a durable client state
            // _not_ specifying DISCARD...
            if (loop == 0)
            {
                ismEngine_SubscriptionAttributes_t nonDurSubAttrs = { ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                                                                      ismENGINE_SUBSCRIPTION_OPTION_SHARED };

                rc = ism_engine_createSubscription(hClient,
                                                   "NON_DURABLE" SUBSCRIPTION_NAME,
                                                   NULL,
                                                   ismDESTINATION_TOPIC,
                                                   SUBSCRIPTION_TOPIC,
                                                   &nonDurSubAttrs,
                                                   hOwningClient,
                                                   NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);

                rc = iett_findClientSubscription(pThreadData,
                                                 "__Shared_TEST",
                                                 iett_generateClientIdHash("__Shared_TEST"),
                                                 "NON_DURABLE" SUBSCRIPTION_NAME,
                                                 iett_generateSubNameHash("NON_DURABLE" SUBSCRIPTION_NAME),
                                                 &subscription);
                TEST_ASSERT_EQUAL(rc, OK);
                TEST_ASSERT_STRINGS_EQUAL(subscription->subName, "NON_DURABLE" SUBSCRIPTION_NAME);
                TEST_ASSERT_EQUAL(ieq_getQType(subscription->queueHandle), multiConsumer);
                TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER, 0);

                // Check that the queue is set up with expected options
                Q = (iemqQueue_t *)(subscription->queueHandle);
                TEST_ASSERT_EQUAL(Q->QOptions & (ieqOptions_SUBSCRIPTION_QUEUE |
                                                 ieqOptions_SINGLE_CONSUMER_ONLY), ieqOptions_SUBSCRIPTION_QUEUE);

                rc = iett_releaseSubscription(pThreadData, subscription, false);
                TEST_ASSERT_EQUAL(rc, OK);
            }

            rc = sync_ism_engine_destroyClientState(hOwningClient,
                                                    ismENGINE_DESTROY_CLIENT_OPTION_NONE);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = iett_findClientSubscription(pThreadData,
                                             "__Shared_TEST",
                                             iett_generateClientIdHash("__Shared_TEST"),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             &subscription);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = iett_releaseSubscription(pThreadData, subscription, false);
            TEST_ASSERT_EQUAL(rc, OK);

            // Check that the non-durable doesn't exist
            rc = iett_findClientSubscription(pThreadData,
                                             "__Shared_TEST",
                                             iett_generateClientIdHash("__Shared_TEST"),
                                             "NON_DURABLE" SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash("NON_DURABLE" SUBSCRIPTION_NAME),
                                             NULL);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        }
    }

    // Clean up
    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hOwningClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    // Check the  subscription is now gone
    rc = iett_findClientSubscription(pThreadData,
                                     "__Shared_TEST",
                                     iett_generateClientIdHash("__Shared_TEST"),
                                     SUBSCRIPTION_NAME,
                                     iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
}
#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC

#define SUBSCRIPTION_NAME "JMSSharedSubscription"
#define SUBSCRIPTION_TOPIC "JMS/Single/Client/Nondurable"
void test_singleClientJMSNondurable(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession1, hSession2;
    ismEngine_ConsumerHandle_t hConsumer1, hConsumer2;
    ismEngine_Subscription_t *subscription;

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_SHARED };

    // Create a shared subscription for a specific client, we loop around twice,
    // to check that destroying works correctly the first time around.
    for(int32_t i=0; i<2; i++)
    {
        rc = test_createClientAndSession(__func__,
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hClient, &hSession1, true);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_createSubscription(hClient,
                                           SUBSCRIPTION_NAME,
                                           NULL,
                                           ismDESTINATION_TOPIC,
                                           SUBSCRIPTION_TOPIC,
                                           &subAttrs,
                                           hClient,
                                           NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check the subscription now exists
        rc = iett_findClientSubscription(pThreadData,
                                         __func__,
                                         iett_generateClientIdHash(__func__),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, SUBSCRIPTION_TOPIC);
        TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);
        TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER, 0);
        TEST_ASSERT_EQUAL(ieq_getQType(subscription->queueHandle), multiConsumer);

        // Check that the shared subscription data is as expected
        iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
        TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettANONYMOUS_SHARER_JMS_APPLICATION);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 0);

        iemqQueue_t *Q = (iemqQueue_t *)(subscription->queueHandle);
        TEST_ASSERT_EQUAL(Q->QOptions & (ieqOptions_SUBSCRIPTION_QUEUE |
                                         ieqOptions_SINGLE_CONSUMER_ONLY), ieqOptions_SUBSCRIPTION_QUEUE);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        if (i==0)
        {
            rc = test_destroyClientAndSession(hClient, hSession1, true);
            TEST_ASSERT_EQUAL(rc, OK);

            // Check the non-durable subscription is removed when the client is destroyed
            rc = iett_findClientSubscription(pThreadData,
                                             __func__,
                                             iett_generateClientIdHash(__func__),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             NULL);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        }
    }

    // Publish 10 messages
    test_publishMessages(SUBSCRIPTION_TOPIC, 10, ismMESSAGE_RELIABILITY_AT_MOST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);

    msgCallbackContext_t MsgContext1 = {hSession1, 0, 0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, true, __func__};
    msgCallbackContext_t *pMsgContext1 = &MsgContext1;

    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_SUBSCRIPTION,
                                   SUBSCRIPTION_NAME,
                                   NULL,
                                   NULL,
                                   &pMsgContext1, sizeof(pMsgContext1), test_msgCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(MsgContext1.received, 10);

    rc = ism_engine_createSession(hClient, ismENGINE_CREATE_SESSION_OPTION_NONE, &hSession2, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    msgCallbackContext_t MsgContext2 = {hSession2, 0, 0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, true, __func__};
    msgCallbackContext_t *pMsgContext2 = &MsgContext2;

    rc = ism_engine_createConsumer(hSession2,
                                   ismDESTINATION_SUBSCRIPTION,
                                   SUBSCRIPTION_NAME,
                                   NULL,
                                   NULL,
                                   &pMsgContext2, sizeof(pMsgContext2), test_msgCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(MsgContext1.received, 10);
    TEST_ASSERT_EQUAL(MsgContext2.received, 0);

    // Publish 100 messages
    MsgContext1.received = 0;
    test_publishMessages(SUBSCRIPTION_TOPIC, 100, ismMESSAGE_RELIABILITY_AT_MOST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);

    // Check that the messages all went to Consumer1
    TEST_ASSERT_EQUAL(MsgContext1.received, 100);

    rc = ism_engine_startMessageDelivery(hSession2, ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish 100 messages
    MsgContext1.received = 0;
    test_publishMessages(SUBSCRIPTION_TOPIC, 100, ismMESSAGE_RELIABILITY_AT_MOST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);

    // Check that the messages got spread between consumers
    TEST_ASSERT_NOT_EQUAL(MsgContext1.received, 100);
    TEST_ASSERT_NOT_EQUAL(MsgContext2.received, 100);
    TEST_ASSERT_EQUAL((MsgContext1.received + MsgContext2.received), 100);

    // Destroy 1 consumer (the 1st)
    rc = ism_engine_destroyConsumer(hConsumer1, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish 100 messages
    MsgContext1.received = 0;
    MsgContext2.received = 0;
    test_publishMessages(SUBSCRIPTION_TOPIC, 100,ismMESSAGE_RELIABILITY_AT_MOST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);

    TEST_ASSERT_EQUAL(MsgContext1.received, 0);
    TEST_ASSERT_EQUAL(MsgContext2.received, 100);

    // Clean up
    rc = test_destroyClientAndSession(hClient, hSession1, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the non-durable subscription is removed
    rc = iett_findClientSubscription(pThreadData,
                                     __func__,
                                     iett_generateClientIdHash(__func__),
                                     SUBSCRIPTION_NAME,
                                     iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
}
#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC

#define SUBSCRIPTION_NAME "JMSSharedSubscription"
#define SUBSCRIPTION_TOPIC "JMS/Global/Nondurable"
void test_globalJMSNondurable(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hOwningClient;
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    ismEngine_ConsumerHandle_t hConsumer1;
    ismEngine_Subscription_t *subscription;
    const char *clientId1 = "C1";

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    rc = test_createClientAndSession(clientId1,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a client state for globally shared nondurable subs (emulating the JMS code)
    rc = ism_engine_createClientState("__SharedND_TEST",
                                      PROTOCOL_ID_SHARED,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hOwningClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                    ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE };

    rc = ism_engine_reuseSubscription(hClient1,
                                      SUBSCRIPTION_NAME,
                                      &subAttrs,
                                      hOwningClient);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    rc = ism_engine_createSubscription(hClient1,
                                       SUBSCRIPTION_NAME,
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       SUBSCRIPTION_TOPIC,
                                       &subAttrs,
                                       hOwningClient,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the subscription now exists
    rc = iett_findClientSubscription(pThreadData,
                                     "__SharedND_TEST",
                                     iett_generateClientIdHash("__SharedND_TEST"),
                                     SUBSCRIPTION_NAME,
                                     iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, SUBSCRIPTION_TOPIC);
    TEST_ASSERT_EQUAL(subscription->subOptions,
                     (subAttrs.subOptions & ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK));
    TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER, 0);
    TEST_ASSERT_EQUAL(ieq_getQType(subscription->queueHandle), multiConsumer);

    // Check that the shared subscription data is as expected
    iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);
    TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
    TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettANONYMOUS_SHARER_JMS_APPLICATION);
    TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 0);

    iemqQueue_t *Q = (iemqQueue_t *)(subscription->queueHandle);
    TEST_ASSERT_EQUAL(Q->QOptions & (ieqOptions_SUBSCRIPTION_QUEUE |
                                     ieqOptions_SINGLE_CONSUMER_ONLY), ieqOptions_SUBSCRIPTION_QUEUE);

    rc = iett_releaseSubscription(pThreadData, subscription, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_reuseSubscription(hClient1,
                                      SUBSCRIPTION_NAME,
                                      &subAttrs,
                                      hOwningClient);
    TEST_ASSERT_EQUAL(rc, OK);

    msgCallbackContext_t MsgContext1 = {hSession1, 0, 0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, true, clientId1};
    msgCallbackContext_t *pMsgContext1 = &MsgContext1;

    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_SUBSCRIPTION,
                                   SUBSCRIPTION_NAME,
                                   NULL,
                                   hOwningClient,
                                   &pMsgContext1, sizeof(pMsgContext1), test_msgCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(MsgContext1.received, 0);

    // Publish 100 messages
    test_publishMessages(SUBSCRIPTION_TOPIC, 100, ismMESSAGE_RELIABILITY_AT_MOST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);

    // Check that the messages were received
    TEST_ASSERT_EQUAL(MsgContext1.received, 100);

    // Clean up
    rc = test_destroyClientAndSession(hClient1, hSession1, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the  subscription is still there by getting a subscriber list
    iettSubscriberList_t list = {0};
    list.topicString = SUBSCRIPTION_TOPIC;

    rc = iett_getSubscriberList(pThreadData, &list);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 1);
    TEST_ASSERT_EQUAL(list.subscriberNodeCount, 1);

    rc = ism_engine_destroyClientState(hOwningClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK); // Shouldn't go async

    // Check that the subscription is now flagged as logically deleted and is accessible (ASAN build
    // would fail if this had already been freed)
    TEST_ASSERT_EQUAL((subscription->internalAttrs & iettSUBATTR_DELETED), iettSUBATTR_DELETED);

    // Check that it is no longer in the client's list of subscriptions
    rc = iett_findClientSubscription(pThreadData,
                                     "__SharedND_TEST",
                                     iett_generateClientIdHash("__SharedND_TEST"),
                                     SUBSCRIPTION_NAME,
                                     iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    // Release the subscriber list and check the subscription disappears
    iett_releaseSubscriberList(pThreadData, &list);
}
#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC

#define SUBSCRIPTION_NAME "MQTTSharedSubscription"
#define SUBSCRIPTION_TOPIC "MQTT/QoS0/Global/Nondurable"
#define C1_SUBID         1
#define C1_CHANGED_SUBID 2
#define C2_SUBID         2 // Same SubID is valid
void test_globalMQTTQoS0Nondurable(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hOwningClient;
    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SessionHandle_t hSession1, hSession2;
    ismEngine_Subscription_t *subscription;

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    // Create a client state for globally shared nondurable subs (emulating the JMS code)
    rc = ism_engine_createClientState("__SharedND_TEST",
                                      PROTOCOL_ID_SHARED,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hOwningClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t loop=0; loop<2; loop++)
    {
        rc = test_createClientAndSessionWithProtocol("C1",
                                                     PROTOCOL_ID_MQTT,
                                                     NULL,
                                                     ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                     &hClient1, &hSession1, true);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = test_createClientAndSessionWithProtocol("C2",
                                                     PROTOCOL_ID_MQTT,
                                                     NULL,
                                                     ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                     &hClient2, &hSession2, true);
        TEST_ASSERT_EQUAL(rc, OK);

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                        ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |
                                                        ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
                                                        C1_SUBID };

        rc = ism_engine_reuseSubscription(hClient1,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs,
                                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

        rc = sync_ism_engine_createSubscription(hClient1,
                                                SUBSCRIPTION_NAME,
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                SUBSCRIPTION_TOPIC,
                                                &subAttrs,
                                                hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check the subscription now exists
        rc = iett_findClientSubscription(pThreadData,
                                         "__SharedND_TEST",
                                         iett_generateClientIdHash("__SharedND_TEST"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, SUBSCRIPTION_TOPIC);
        TEST_ASSERT_EQUAL(subscription->subOptions, (subAttrs.subOptions & ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK));
        TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER, 0);
        TEST_ASSERT_EQUAL(ieq_getQType(subscription->queueHandle), multiConsumer);
        iemqQueue_t *Q = (iemqQueue_t *)(subscription->queueHandle);
        TEST_ASSERT_EQUAL(Q->QOptions & (ieqOptions_SUBSCRIPTION_QUEUE |
                                         ieqOptions_SINGLE_CONSUMER_ONLY), ieqOptions_SUBSCRIPTION_QUEUE);

        // Check the shared sub information
        iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
        TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettNO_ANONYMOUS_SHARER);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 1);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subId, subAttrs.subId);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subOptions, (subAttrs.subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARING_CLIENT_PERSISTENT_MASK));

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check that the subscrption is there on 1st client
        rc = iett_findClientSubscription(pThreadData,
                                         "C1",
                                         iett_generateClientIdHash("C1"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);
        TEST_ASSERT_EQUAL(subscription->subId, C1_SUBID); // This is a side-effect of C1 being the original creator

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Reuse from same client
        subAttrs.subId = C1_CHANGED_SUBID;
        rc = ism_engine_reuseSubscription(hClient1,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs,
                                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);
        sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 1);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subId, subAttrs.subId);

        // Reuse from different client
        subAttrs.subId = C2_SUBID;
        rc = ism_engine_reuseSubscription(hClient2,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs,
                                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);

        sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2);

        // Both should have SUBID the same (because of the way the test sets up)
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subId, sharedSubData->sharingClients[1].subId);

        // Check that the subscrption is there on 2nd client
        rc = iett_findClientSubscription(pThreadData,
                                         "C2",
                                         iett_generateClientIdHash("C2"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        if (loop == 0)
        {
            rc = ism_engine_destroySubscription(hClient1, SUBSCRIPTION_NAME, hOwningClient, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            // Check that the subscrption was removed from the 1st client
            rc = iett_findClientSubscription(pThreadData,
                                             "C1",
                                             iett_generateClientIdHash("C1"),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             NULL);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

            // Check the  subscription is still there
            rc = iett_findClientSubscription(pThreadData,
                                             "__SharedND_TEST",
                                             iett_generateClientIdHash("__SharedND_TEST"),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             &subscription);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);

            rc = iett_releaseSubscription(pThreadData, subscription, false);
            TEST_ASSERT_EQUAL(rc, OK);

            // Clean up 1st client
            rc = test_destroyClientAndSession(hClient1, hSession1, true);
            TEST_ASSERT_EQUAL(rc, OK);

            // Check that the subscrption is there on 2nd client
            rc = iett_findClientSubscription(pThreadData,
                                             "C2",
                                             iett_generateClientIdHash("C2"),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            // Clean up 2nd client
            rc = test_destroyClientAndSession(hClient2, hSession2, true);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            int32_t newClient = 0;
            ismEngine_ClientStateHandle_t hNewClient[10];
            ismEngine_SessionHandle_t hNewSession[10];

            // Add and remove 10 clients to ensure shared data is extended properly
            for(;newClient<10;newClient++)
            {
                char clientId[10];

                sprintf(clientId, "NC%d", newClient);

                rc = test_createClientAndSessionWithProtocol(clientId,
                                                             (rand()%100 > 50) ? PROTOCOL_ID_HTTP :
                                                                                 PROTOCOL_ID_MQTT,
                                                             NULL,
                                                             ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                                             ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                             &hNewClient[newClient], &hNewSession[newClient], true);
                TEST_ASSERT_EQUAL(rc, OK);

                subAttrs.subId = newClient+10;
                rc = ism_engine_reuseSubscription(hNewClient[newClient], SUBSCRIPTION_NAME, &subAttrs, hOwningClient);
                TEST_ASSERT_EQUAL(rc, OK);
                TEST_ASSERT_EQUAL(subscription->subId, C1_SUBID); // Check it hasn't altered the subId of the subscription
            }

            newClient--;
            for(;newClient>=0;newClient--)
            {
                rc = ism_engine_destroySubscription(hNewClient[newClient], SUBSCRIPTION_NAME, hOwningClient, NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);

                rc = test_destroyClientAndSession(hNewClient[newClient], hNewSession[newClient], true);
                TEST_ASSERT_EQUAL(rc, OK);
            }

            // Emulate an administrative deletion with clients connected
            rc = iett_destroySubscriptionForClientId(pThreadData,
                                                     "__SharedND_TEST",
                                                     NULL, // Emulating an admin request
                                                     SUBSCRIPTION_NAME,
                                                     NULL, // Emulating an admin request
                                                     iettSUB_DESTROY_OPTION_NONE,
                                                     NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            // Clean up 1st client
            rc = test_destroyClientAndSession(hClient1, hSession1, true);
            TEST_ASSERT_EQUAL(rc, OK);

            // Clean up 2nd client
            rc = test_destroyClientAndSession(hClient2, hSession2, true);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        // Check that the subscrption was removed from the 2nd client
        rc = iett_findClientSubscription(pThreadData,
                                         "C2",
                                         iett_generateClientIdHash("C2"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

        // Check that the subscrption was removed from the Owning client
        rc = iett_findClientSubscription(pThreadData,
                                         "__SharedND_TEST",
                                         iett_generateClientIdHash("__SharedND_TEST"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }

    rc = ism_engine_destroyClientState(hOwningClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK); // Shouldn't go async
}
#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC

void test_globalMQTTDurable_InitClientStates(ismEngine_ClientStateHandle_t *hOwningClient,
                                             ismEngine_ClientStateHandle_t *hClient1,
                                             ismEngine_SessionHandle_t *hSession1,
                                             ismEngine_ClientStateHandle_t *hClient2,
                                             ismEngine_SessionHandle_t *hSession2)
{
    uint32_t rc;

    if (hOwningClient != NULL)
    {
        // Create a client state for globally shared durable subs (emulating the JMS code)
        rc = ism_engine_createClientState("__Shared_TEST",
                                          PROTOCOL_ID_SHARED,
                                          ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                          NULL, NULL, NULL,
                                          hOwningClient,
                                          NULL, 0, NULL);

        TEST_ASSERT(rc == OK || rc == ISMRC_ResumedClientState, ("Unexpected rc %d\n", rc));
    }

    if (hClient1)
    {
        rc = test_createClientAndSessionWithProtocol("C1",
                                                     PROTOCOL_ID_MQTT,
                                                     NULL,
                                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                     hClient1, hSession1, true);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    if (hClient2)
    {
        rc = test_createClientAndSessionWithProtocol("C2",
                                                     PROTOCOL_ID_MQTT,
                                                     NULL,
                                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                     hClient2, hSession2, true);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

#define SUBSCRIPTION_NAME "MQTTSharedSubscription"
#define SUBSCRIPTION_TOPIC "MQTT/QoS0/Global/Durable"
void test_globalMQTTQoS0Durable(void)
{
    ismEngine_ClientStateHandle_t hOwningClient;
    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SessionHandle_t hSession1, hSession2;
    ismEngine_Subscription_t *subscription;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    rc = test_bounceEngine();
    TEST_ASSERT_EQUAL(rc, OK);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    test_globalMQTTDurable_InitClientStates(&hOwningClient, NULL, NULL, NULL, NULL);

    for(int32_t loop=0; loop<2; loop++)
    {
        test_globalMQTTDurable_InitClientStates(NULL, &hClient1, &hSession1, &hClient2, &hSession2);

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                        ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                        ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |
                                                        ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
                                                        88 };

        rc = ism_engine_reuseSubscription(hClient1,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs,
                                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

        rc = sync_ism_engine_createSubscription(hClient1,
                                                SUBSCRIPTION_NAME,
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                SUBSCRIPTION_TOPIC,
                                                &subAttrs,
                                                hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check the subscription now exists
        rc = iett_findClientSubscription(pThreadData,
                                         "__Shared_TEST",
                                         iett_generateClientIdHash("__Shared_TEST"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, SUBSCRIPTION_TOPIC);
        TEST_ASSERT_EQUAL(subscription->subOptions,
                          (subAttrs.subOptions & ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK));
        TEST_ASSERT_EQUAL(subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE,
                          ismENGINE_SUBSCRIPTION_OPTION_DURABLE);
        TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER, 0);
        TEST_ASSERT_EQUAL(ieq_getQType(subscription->queueHandle), multiConsumer);
        iemqQueue_t *Q = (iemqQueue_t *)(subscription->queueHandle);
        TEST_ASSERT_EQUAL(Q->QOptions & (ieqOptions_SUBSCRIPTION_QUEUE |
                                         ieqOptions_SINGLE_CONSUMER_ONLY), ieqOptions_SUBSCRIPTION_QUEUE);

        // Check the shared sub information
        iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
        TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettNO_ANONYMOUS_SHARER);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 1);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subOptions,
                          (subAttrs.subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARING_CLIENT_PERSISTENT_MASK));
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subId, subAttrs.subId);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check that the subscrption is there on 1st client
        rc = iett_findClientSubscription(pThreadData,
                                         "C1",
                                         iett_generateClientIdHash("C1"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);

        // Check we get the right list of clients for this subscription
        const char *expectClientsA[] = {"C1"};
        test_checkSubscriptionClientIds(pThreadData,
                                        subscription,
                                        sizeof(expectClientsA)/sizeof(expectClientsA[0]),
                                        expectClientsA);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Reuse from same client (changing SubId)
        subAttrs.subId = 89;
        rc = ism_engine_reuseSubscription(hClient1,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs,
                                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);

        test_checkSubscriptionClientIds(pThreadData,
                                        subscription,
                                        sizeof(expectClientsA)/sizeof(expectClientsA[0]),
                                        expectClientsA);

        // Reuse from different client (using same, changed SubId)
        rc = ism_engine_reuseSubscription(hClient2,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs,
                                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);

        const char *expectClientsB[] = {"C1", "C2"};
        test_checkSubscriptionClientIds(pThreadData,
                                        subscription,
                                        sizeof(expectClientsB)/sizeof(expectClientsB[0]),
                                        expectClientsB);

        // Check that the subscription is there on 2nd client
        rc = iett_findClientSubscription(pThreadData,
                                         "C2",
                                         iett_generateClientIdHash("C2"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);
        TEST_ASSERT_EQUAL(subscription->subId, 88); // subscription's subId unchanged by re-using

        sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
        TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettNO_ANONYMOUS_SHARER);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subId, subAttrs.subId);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[1].subId, subAttrs.subId);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        test_bounceEngine();

        pThreadData = ieut_getThreadData();

        test_globalMQTTDurable_InitClientStates(&hOwningClient, &hClient1, &hSession1, &hClient2, &hSession2);

        // Check that the subscription is there on both clients
        rc = iett_findClientSubscription(pThreadData,
                                         "C1",
                                         iett_generateClientIdHash("C1"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);
        TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER, 0);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check that the subscription is there on both clients
        rc = iett_findClientSubscription(pThreadData,
                                         "C2",
                                         iett_generateClientIdHash("C2"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);
        TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER, 0);
        TEST_ASSERT_EQUAL(subscription->subId, 88); // subscription's subId unchanged by re-using

        sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
        TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettNO_ANONYMOUS_SHARER);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subId, subAttrs.subId);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[1].subId, subAttrs.subId);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        if (loop == 0)
        {
            rc = ism_engine_destroySubscription(hClient1, SUBSCRIPTION_NAME, hOwningClient, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            // Check that the subscrption was removed from the 1st client
            rc = iett_findClientSubscription(pThreadData,
                                             "C1",
                                             iett_generateClientIdHash("C1"),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             NULL);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

            // Check the  subscription is still there
            rc = iett_findClientSubscription(pThreadData,
                                             "__Shared_TEST",
                                             iett_generateClientIdHash("__Shared_TEST"),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             &subscription);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);
            TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER, 0);

            rc = iett_releaseSubscription(pThreadData, subscription, false);
            TEST_ASSERT_EQUAL(rc, OK);

            // Destroy 1st client
            rc = ism_engine_destroyClientState(hClient1,
                                               ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                               NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            // Check that the subscrption is there on 2nd client
            rc = iett_findClientSubscription(pThreadData,
                                             "C2",
                                             iett_generateClientIdHash("C2"),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             &subscription);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER, 0);

            test_bounceEngine();

            pThreadData = ieut_getThreadData();

            test_globalMQTTDurable_InitClientStates(&hOwningClient, NULL, NULL, &hClient2, &hSession2);

            // Check that the subscription is NOT there on C1
            rc = iett_findClientSubscription(pThreadData,
                                             "C1",
                                             iett_generateClientIdHash("C1"),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             NULL);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

            // Check that the subscription is there on C2
            rc = iett_findClientSubscription(pThreadData,
                                             "C2",
                                             iett_generateClientIdHash("C2"),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             &subscription);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);
            TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER, 0);

            rc = iett_releaseSubscription(pThreadData, subscription, false);
            TEST_ASSERT_EQUAL(rc, OK);

            // Destroy 2nd client
            test_incrementActionsRemaining(pActionsRemaining, 1);
            rc = ism_engine_destroyClientState(hClient2,
                                               ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                               &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
            if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        }
        else
        {
            // Emulate an administrative deletion with clients connected
            rc = iett_destroySubscriptionForClientId(pThreadData,
                                                     "__Shared_TEST",
                                                     NULL, // Emulating an admin request
                                                     SUBSCRIPTION_NAME,
                                                     NULL, // Emulating an admin request
                                                     iettSUB_DESTROY_OPTION_NONE,
                                                     NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            // Destroy both clients
            test_incrementActionsRemaining(pActionsRemaining, 2);
            rc = ism_engine_destroyClientState(hClient1,
                                               ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                               &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
            if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

            rc = ism_engine_destroyClientState(hClient2,
                                               ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                               &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
            if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        }

        test_waitForRemainingActions(pActionsRemaining);

        // Check that the subscrption was removed from the 2nd client
        rc = iett_findClientSubscription(pThreadData,
                                         "C2",
                                         iett_generateClientIdHash("C2"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

        // Check that the subscrption was removed from the Owning client
        rc = iett_findClientSubscription(pThreadData,
                                         "__Shared_TEST",
                                         iett_generateClientIdHash("__Shared_TEST"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }

    rc = ism_engine_destroyClientState(hOwningClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK); // Shouldn't go async
}
#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC

#define SUBSCRIPTION_NAME "MQTTSharedSubscription"
#define SUBSCRIPTION_TOPIC "MQTT/QoSx/Global/Nondurable"
void test_globalMQTTQoSxNondurable(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hOwningClient;
    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SessionHandle_t hSession1, hSession2;
    ismEngine_Subscription_t *subscription;
    const char *clientId1 = "C1";
    const char *clientId2 = "C2";

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    // Create a client state for globally shared nondurable subs (emulating the JMS code)
    rc = ism_engine_createClientState("__SharedND_TEST",
                                      PROTOCOL_ID_SHARED,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hOwningClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t loop=0; loop<2; loop++)
    {
        rc = test_createClientAndSessionWithProtocol(clientId1,
                                                     PROTOCOL_ID_MQTT,
                                                     NULL,
                                                     ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                     &hClient1, &hSession1, true);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = test_createClientAndSessionWithProtocol(clientId2,
                                                     PROTOCOL_ID_MQTT,
                                                     NULL,
                                                     ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                     &hClient2, &hSession2, true);
        TEST_ASSERT_EQUAL(rc, OK);

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                        ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |
                                                        ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT };

        rc = ism_engine_reuseSubscription(hClient1,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs,
                                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

        rc = sync_ism_engine_createSubscription(hClient1,
                                                SUBSCRIPTION_NAME,
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                SUBSCRIPTION_TOPIC,
                                                &subAttrs,
                                                hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check the subscription now exists
        rc = iett_findClientSubscription(pThreadData,
                                         "__SharedND_TEST",
                                         iett_generateClientIdHash("__SharedND_TEST"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, SUBSCRIPTION_TOPIC);
        // TODO (MQTTSTFIX) TEST_ASSERT_EQUAL(subscription->subOptions, (subOptions & ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK));
        TEST_ASSERT_EQUAL(ieq_getQType(subscription->queueHandle), multiConsumer);
        iemqQueue_t *Q = (iemqQueue_t *)(subscription->queueHandle);
        TEST_ASSERT_EQUAL(Q->QOptions & (ieqOptions_SUBSCRIPTION_QUEUE |
                                         ieqOptions_SINGLE_CONSUMER_ONLY), ieqOptions_SUBSCRIPTION_QUEUE);

        // Check the shared sub information
        iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
        TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettNO_ANONYMOUS_SHARER);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 1);
        // TODO (MQTTSTFIX) TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subOptions, (subOptions & ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK));
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE, 0);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check that the subscrption is there on 1st client
        rc = iett_findClientSubscription(pThreadData,
                                         clientId1,
                                         iett_generateClientIdHash(clientId1),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Reuse from same client
        rc = ism_engine_reuseSubscription(hClient1,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs,
                                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);
        sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 1);

        ismEngine_SubscriptionAttributes_t subAttrs2 = { ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                         ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                                                         ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT };

        // Reuse from different client
        rc = ism_engine_reuseSubscription(hClient2,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs2,
                                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);

        sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2);

        // Check that the subscrption is there on 2nd client
        rc = iett_findClientSubscription(pThreadData,
                                         clientId2,
                                         iett_generateClientIdHash(clientId2),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Create consumers
        msgCallbackContext_t MsgContext1 = {hSession1, 0, 0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, (loop == 0), clientId1};
        msgCallbackContext_t MsgContext2 = {hSession2, 0, 0, 0, ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE, (loop == 0), clientId2};
        msgCallbackContext_t *pMsgContext1 = &MsgContext1, *pMsgContext2 = &MsgContext2;

        ismEngine_ConsumerHandle_t hConsumer1 = NULL, hConsumer2 = NULL;

        rc = ism_engine_createConsumer(hSession1,
                                       ismDESTINATION_SUBSCRIPTION,
                                       SUBSCRIPTION_NAME,
                                       NULL,
                                       hOwningClient,
                                       &pMsgContext1, sizeof(pMsgContext1), test_msgCallback,
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                       &hConsumer1,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hConsumer1);

        rc = ism_engine_createConsumer(hSession2,
                                       ismDESTINATION_SUBSCRIPTION,
                                       SUBSCRIPTION_NAME,
                                       NULL,
                                       hOwningClient,
                                       &pMsgContext2, sizeof(pMsgContext2), test_msgCallback,
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                       &hConsumer2,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hConsumer2);

        uint32_t msgsPerPublish = 40;
        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_AT_LEAST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);
        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_AT_MOST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);
        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_EXACTLY_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);

        // Check we got sharing
        TEST_ASSERT_NOT_EQUAL(pMsgContext1->received, 0);
        TEST_ASSERT_NOT_EQUAL(pMsgContext2->received, 0);

        rc = ism_engine_destroyConsumer(hConsumer1, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        uint32_t initialMsgsReceived2 = pMsgContext2->received;

        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_AT_MOST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);

        TEST_ASSERT_EQUAL(pMsgContext2->received, initialMsgsReceived2+msgsPerPublish);

        // Try and exhaust delivery ids
        if (loop == 1)
        {
            test_publishMessages(SUBSCRIPTION_TOPIC, 300, ismMESSAGE_RELIABILITY_EXACTLY_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);
        }

        rc = ism_engine_destroyConsumer(hConsumer2, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_EXACTLY_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);

        if (loop == 0)
        {
            rc = ism_engine_destroySubscription(hClient1, SUBSCRIPTION_NAME, hOwningClient, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            // Check that the subscrption was removed from the 1st client
            rc = iett_findClientSubscription(pThreadData,
                                             clientId1,
                                             iett_generateClientIdHash(clientId1),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             NULL);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

            // Check the  subscription is still there
            rc = iett_findClientSubscription(pThreadData,
                                             "__SharedND_TEST",
                                             iett_generateClientIdHash("__SharedND_TEST"),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             &subscription);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);

            rc = iett_releaseSubscription(pThreadData, subscription, false);
            TEST_ASSERT_EQUAL(rc, OK);

            // Clean up 1st client
            rc = test_destroyClientAndSession(hClient1, hSession1, true);
            TEST_ASSERT_EQUAL(rc, OK);

            // Check that the subscription is there on 2nd client
            rc = iett_findClientSubscription(pThreadData,
                                             clientId2,
                                             iett_generateClientIdHash(clientId2),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            // Clean up 2nd client
            rc = test_destroyClientAndSession(hClient2, hSession2, true);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            int32_t newClient = 0;
            ismEngine_ClientStateHandle_t hNewClient[10];
            ismEngine_SessionHandle_t hNewSession[10];

            // Add and remove 10 clients to ensure shared data is extended properly
            for(;newClient<10;newClient++)
            {
                char clientId[10];

                sprintf(clientId, "NC%d", newClient);

                rc = test_createClientAndSessionWithProtocol(clientId,
                                                             (rand()%100 > 50) ? PROTOCOL_ID_HTTP :
                                                                                 PROTOCOL_ID_MQTT,
                                                             NULL,
                                                             ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                                             ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                             &hNewClient[newClient], &hNewSession[newClient], true);
                TEST_ASSERT_EQUAL(rc, OK);

                rc = ism_engine_reuseSubscription(hNewClient[newClient], SUBSCRIPTION_NAME, &subAttrs, hOwningClient);
                TEST_ASSERT_EQUAL(rc, OK);
            }

            newClient--;
            for(;newClient>=0;newClient--)
            {
                rc = ism_engine_destroySubscription(hNewClient[newClient], SUBSCRIPTION_NAME, hOwningClient, NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);

                rc = test_destroyClientAndSession(hNewClient[newClient], hNewSession[newClient], true);
                TEST_ASSERT_EQUAL(rc, OK);
            }

            // Emulate an administrative deletion with clients connected
            rc = iett_destroySubscriptionForClientId(pThreadData,
                                                     "__SharedND_TEST",
                                                     NULL, // Emulating an admin request
                                                     SUBSCRIPTION_NAME,
                                                     NULL, // Emulating an admin request
                                                     iettSUB_DESTROY_OPTION_NONE,
                                                     NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            // Clean up 1st client
            rc = test_destroyClientAndSession(hClient1, hSession1, true);
            TEST_ASSERT_EQUAL(rc, OK);

            // Clean up 2nd client
            rc = test_destroyClientAndSession(hClient2, hSession2, true);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        // Check that the subscrption was removed from the 2nd client
        rc = iett_findClientSubscription(pThreadData,
                                         clientId2,
                                         iett_generateClientIdHash(clientId2),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

        // Check that the subscrption was removed from the Owning client
        rc = iett_findClientSubscription(pThreadData,
                                         "__SharedND_TEST",
                                         iett_generateClientIdHash("__SharedND_TEST"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }

    rc = ism_engine_destroyClientState(hOwningClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK); // Shouldn't go async
}
#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC

#define SUBSCRIPTION_NAME "MQTTSharedSubscription"
#define SUBSCRIPTION_TOPIC "MQTT/QoSx/Global/Durable"
void test_globalMQTTQoSxDurable(void)
{
    ismEngine_ClientStateHandle_t hOwningClient;
    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SessionHandle_t hSession1, hSession2;
    ismEngine_Subscription_t *subscription;
    const char *clientId1 = "C1";
    const char *clientId2 = "C2";

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    rc = test_bounceEngine();
    TEST_ASSERT_EQUAL(rc, OK);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    test_globalMQTTDurable_InitClientStates(&hOwningClient, NULL, NULL, NULL, NULL);

    for(int32_t loop=0; loop<5; loop++)
    {
        ismEngine_SubId_t loopSubId = (ismEngine_SubId_t)(loop+1); // All sharers happen to specify the same SubID

        ism_field_t f, *pf;

        // Explicitly set reducedMemoryRecoveryMode in loop 2 to 0 and in loop 4 to 1
        if (loop == 2 || loop == 4)
        {
            f.type = VT_Boolean;
            f.val.i = (loop / 2) - 1;
            pf = &f;
        }
        // And have no explicit setting for other loops
        else
        {
            pf = NULL;
        }

        ism_common_setProperty(ism_common_getConfigProperties(), ismENGINE_CFGPROP_REDUCED_MEMORY_RECOVERY_MODE, pf);

        test_globalMQTTDurable_InitClientStates(NULL, &hClient1, &hSession1, &hClient2, &hSession2);

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                        ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                        ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |
                                                        ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
                                                        loopSubId };

        rc = ism_engine_reuseSubscription(hClient1,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs,
                                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

        rc = sync_ism_engine_createSubscription(hClient1,
                                                SUBSCRIPTION_NAME,
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                SUBSCRIPTION_TOPIC,
                                                &subAttrs,
                                                hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check the subscription now exists
        rc = iett_findClientSubscription(pThreadData,
                                         "__Shared_TEST",
                                         iett_generateClientIdHash("__Shared_TEST"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, SUBSCRIPTION_TOPIC);
        TEST_ASSERT_EQUAL(subscription->subOptions,
                         (subAttrs.subOptions & ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK));
        TEST_ASSERT_EQUAL(subscription->subId, loopSubId);
        TEST_ASSERT_EQUAL(subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE,
                          ismENGINE_SUBSCRIPTION_OPTION_DURABLE);
        TEST_ASSERT_EQUAL(ieq_getQType(subscription->queueHandle), multiConsumer);
        iemqQueue_t *Q = (iemqQueue_t *)(subscription->queueHandle);
        TEST_ASSERT_EQUAL(Q->QOptions & (ieqOptions_SUBSCRIPTION_QUEUE |
                                         ieqOptions_SINGLE_CONSUMER_ONLY), ieqOptions_SUBSCRIPTION_QUEUE);

        // Check the shared sub information
        iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
        TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettNO_ANONYMOUS_SHARER);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 1);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subOptions,
                         (subAttrs.subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARING_CLIENT_PERSISTENT_MASK));
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subId, loopSubId);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Try destroying the subscription but saying that we want to only do so if it is an admin sub (which it isn't)
        rc = iett_destroySubscriptionForClientId(pThreadData,
                                                 "__Shared_TEST",
                                                 NULL, SUBSCRIPTION_NAME, NULL,
                                                 iettSUB_DESTROY_OPTION_DESTROY_ADMINSUBSCRIPTION,
                                                 NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_WrongSubscriptionAPI);

        // Check that the subscrption is there on 1st client
        rc = iett_findClientSubscription(pThreadData,
                                         clientId1,
                                         iett_generateClientIdHash(clientId1),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Reuse from same client
        rc = ism_engine_reuseSubscription(hClient1,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs,
                                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);
        sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 1);

        // Reuse from different client
        ismEngine_SubscriptionAttributes_t subAttrs2 = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                         ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                         ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                                                         ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
                                                         loopSubId };

        rc = ism_engine_reuseSubscription(hClient2,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs2,
                                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);

        sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2);

        // Check that the subscription is there on 2nd client
        rc = iett_findClientSubscription(pThreadData,
                                         clientId2,
                                         iett_generateClientIdHash(clientId2),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        test_bounceEngine();

        pThreadData = ieut_getThreadData();

        test_globalMQTTDurable_InitClientStates(&hOwningClient, &hClient1, &hSession1, &hClient2, &hSession2);

        // Check that the subscription is there on both clients
        rc = iett_findClientSubscription(pThreadData,
                                         clientId1,
                                         iett_generateClientIdHash(clientId1),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check that the subscription is there on both clients
        rc = iett_findClientSubscription(pThreadData,
                                         clientId2,
                                         iett_generateClientIdHash(clientId2),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);

        sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subId, loopSubId);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[1].subId, loopSubId);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Create consumers
        msgCallbackContext_t MsgContext1 = {hSession1, 0, 0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, (loop==0), clientId1};
        msgCallbackContext_t MsgContext2 = {hSession2, 0, 0, 0, ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE, (loop==0), clientId2};
        msgCallbackContext_t *pMsgContext1 = &MsgContext1, *pMsgContext2 = &MsgContext2;

        ismEngine_ConsumerHandle_t hConsumer1 = NULL, hConsumer2 = NULL;

        rc = ism_engine_createConsumer(hSession1,
                                       ismDESTINATION_SUBSCRIPTION,
                                       SUBSCRIPTION_NAME,
                                       NULL,
                                       hOwningClient,
                                       &pMsgContext1, sizeof(pMsgContext1), test_msgCallback,
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                       &hConsumer1,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hConsumer1);

        rc = ism_engine_createConsumer(hSession2,
                                       ismDESTINATION_SUBSCRIPTION,
                                       SUBSCRIPTION_NAME,
                                       NULL,
                                       hOwningClient,
                                       &pMsgContext2, sizeof(pMsgContext2), test_msgCallback,
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                       &hConsumer2,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hConsumer2);

        uint32_t msgsPerPublish = 50;
        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_AT_LEAST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);
        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_AT_MOST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);
        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_EXACTLY_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);

        //Wait for the high qos messages...
        test_waitForMessages( &(pMsgContext1->receivedWithDeliveryId)
                            , &(pMsgContext2->receivedWithDeliveryId)
                            ,  2 * msgsPerPublish
                            , 20);
        ismEngine_FullMemoryBarrier();

        // Check we got sharing
        TEST_ASSERT_NOT_EQUAL(pMsgContext1->received, 0);
        TEST_ASSERT_NOT_EQUAL(pMsgContext2->received, 0);

        //Check that we got the right number of higher qos messages:
        TEST_ASSERT_EQUAL(pMsgContext2->receivedWithDeliveryId + pMsgContext1->receivedWithDeliveryId, 2*msgsPerPublish);

        uint32_t initialMsgsReceived2 = pMsgContext2->received;

        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_EXACTLY_ONCE, ismMESSAGE_PERSISTENCE_PERSISTENT);

        //Wait for the messages to be published...
        test_waitForMessages( &(pMsgContext1->receivedWithDeliveryId)
                            , &(pMsgContext2->receivedWithDeliveryId)
                            ,  3 * msgsPerPublish
                            , 20);
        ismEngine_FullMemoryBarrier();

        //Check all messages didn't go to one guy
        TEST_ASSERT_NOT_EQUAL(pMsgContext2->received, initialMsgsReceived2+msgsPerPublish);

        initialMsgsReceived2 = pMsgContext2->received;

        rc = sync_ism_engine_destroyConsumer(hConsumer1);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = sync_ism_engine_destroyConsumer(hConsumer2);
        TEST_ASSERT_EQUAL(rc, OK);

        uint32_t newPublishes = 10;
        test_publishMessages(SUBSCRIPTION_TOPIC, newPublishes, ismMESSAGE_RELIABILITY_AT_MOST_ONCE, ismMESSAGE_PERSISTENCE_PERSISTENT);

        TEST_ASSERT_EQUAL(pMsgContext2->received, initialMsgsReceived2);
        TEST_ASSERT_EQUAL(pMsgContext2->redelivered, 0);

        // Drive redelivery for one consumer (twice)
        if (loop == 1)
        {
            //printf("Consumer initially received %u msgs and there are %u qos 0 messages since then. Of the ones that have arrived, %u had a deliveryid\n",
            //       pMsgContext2->received, newPublishes,   pMsgContext2->receivedWithDeliveryId);
            rc = sync_ism_engine_destroyClientState(hClient1,  ismENGINE_DESTROY_CLIENT_OPTION_NONE);
            TEST_ASSERT_EQUAL(rc, OK);

            uint32_t redeliveredPerLoop = pMsgContext2->receivedWithDeliveryId;

            for(int32_t xx=0; xx<2; xx++)
            {
                rc = sync_ism_engine_destroyClientState(hClient2, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
                TEST_ASSERT_EQUAL(rc, OK);

                test_globalMQTTDurable_InitClientStates(NULL, NULL, NULL, &hClient2, &hSession2);

                rc = ism_engine_createConsumer(hSession2,
                                               ismDESTINATION_SUBSCRIPTION,
                                               SUBSCRIPTION_NAME,
                                               NULL,
                                               hOwningClient,
                                               &pMsgContext2, sizeof(pMsgContext2), test_msgCallback,
                                               NULL,
                                               ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                               &hConsumer2,
                                               NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);
                TEST_ASSERT_PTR_NOT_NULL(hConsumer2);

                //printf("In loop %d, expected %u+%u+(%d*%u) = %u and got %u\n",
                //   xx+1, initialMsgsReceived2, newPublishes, xx+1, redeliveredPerLoop,
                //   initialMsgsReceived2 + newPublishes +(redeliveredPerLoop*(1+xx)),
                //   pMsgContext2->received);
                test_waitForMessages(&(pMsgContext2->received), NULL,
                                     initialMsgsReceived2 + newPublishes +(redeliveredPerLoop*(1+xx)),
                                     20);
                ismEngine_FullMemoryBarrier();
                TEST_ASSERT_EQUAL(pMsgContext2->received, initialMsgsReceived2 + newPublishes +(redeliveredPerLoop*(1+xx)));
                TEST_ASSERT_EQUAL(pMsgContext2->redelivered, redeliveredPerLoop * (1+xx));

                rc = sync_ism_engine_destroyConsumer(hConsumer2);
                TEST_ASSERT_EQUAL(rc, OK);
            }
            initialMsgsReceived2 = pMsgContext2->received;

            test_globalMQTTDurable_InitClientStates(NULL, &hClient1, &hSession1, NULL, NULL);
        }

        if (loop == 0)
        {
            rc = ism_engine_destroySubscription(hClient1, SUBSCRIPTION_NAME, hOwningClient, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            // Check that the subscrption was removed from the 1st client
            rc = iett_findClientSubscription(pThreadData,
                                             clientId1,
                                             iett_generateClientIdHash(clientId1),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             NULL);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

            // Check the  subscription is still there
            rc = iett_findClientSubscription(pThreadData,
                                             "__Shared_TEST",
                                             iett_generateClientIdHash("__Shared_TEST"),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             &subscription);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);

            rc = iett_releaseSubscription(pThreadData, subscription, false);
            TEST_ASSERT_EQUAL(rc, OK);

            // Destroy 1st client
            rc = sync_ism_engine_destroyClientState(hClient1,
                                                    ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
            TEST_ASSERT_EQUAL(rc, OK);

            // Check that the subscrption is there on 2nd client
            rc = iett_findClientSubscription(pThreadData,
                                             clientId2,
                                             iett_generateClientIdHash(clientId2),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            test_bounceEngine();

            pThreadData = ieut_getThreadData();

            test_globalMQTTDurable_InitClientStates(&hOwningClient, NULL, NULL, &hClient2, &hSession2);

            // Check that the subscription is NOT there on C1
            rc = iett_findClientSubscription(pThreadData,
                                             clientId1,
                                             iett_generateClientIdHash(clientId1),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             NULL);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

            // Check that the subscription is there on C2
            rc = iett_findClientSubscription(pThreadData,
                                             clientId2,
                                             iett_generateClientIdHash(clientId2),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             &subscription);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);

            rc = iett_releaseSubscription(pThreadData, subscription, false);
            TEST_ASSERT_EQUAL(rc, OK);

            // Destroy 2nd client
            rc = sync_ism_engine_destroyClientState(hClient2,
                                               ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            if (loop == 2 || loop == 4)
            {
                test_bounceEngine();

                pThreadData = ieut_getThreadData();

                test_globalMQTTDurable_InitClientStates(&hOwningClient, &hClient1, &hSession1, &hClient2, &hSession2);
            }

            if (loop == 3 || loop == 4)
            {
                // Emulate an administrative deletion with clients connected
                rc = iett_destroySubscriptionForClientId(pThreadData,
                                                         "__Shared_TEST",
                                                         NULL, // Emulating an admin request
                                                         SUBSCRIPTION_NAME,
                                                         NULL, // Emulating an admin request
                                                         iettSUB_DESTROY_OPTION_NONE,
                                                         NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);
            }

            // Destroy both clients
            rc = sync_ism_engine_destroyClientState(hClient1,
                                                    ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = sync_ism_engine_destroyClientState(hClient2,
                                               ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        // Check that the subscrption was removed from the 2nd client
        rc = iett_findClientSubscription(pThreadData,
                                         clientId2,
                                         iett_generateClientIdHash(clientId2),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

        // Check that the subscrption was removed from the Owning client
        rc = iett_findClientSubscription(pThreadData,
                                         "__Shared_TEST",
                                         iett_generateClientIdHash("__Shared_TEST"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }

    rc = ism_engine_destroyClientState(hOwningClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK); // Shouldn't go async
}
#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC

#define SUBSCRIPTION_NAME "MQTTSharedSubscription"
#define SUBSCRIPTION_TOPIC "MQTT/QoSx/Global/RelSelection"

typedef struct tag_testListSubsCallbackContext_t
{
    const char *expectSubName;
    const char *expectTopic;
    uint32_t expectOptions;
    ismEngine_SubId_t expectSubId;
} testListSubsCallbackContext_t;

void test_listSubsCallback(ismEngine_SubscriptionHandle_t subHandle,
                           const char *pSubName,
                           const char *pTopicString,
                           void *properties,
                           size_t propertiesLength,
                           const ismEngine_SubscriptionAttributes_t *pSubAttributes,
                           uint32_t consumerCount,
                           void *pContext)
{
    testListSubsCallbackContext_t *callbackContext = (testListSubsCallbackContext_t *)pContext;

    if (callbackContext->expectSubName) TEST_ASSERT_STRINGS_EQUAL(pSubName, callbackContext->expectSubName);
    if (callbackContext->expectTopic) TEST_ASSERT_STRINGS_EQUAL(pTopicString, callbackContext->expectTopic);
    if (callbackContext->expectSubId) TEST_ASSERT_EQUAL(pSubAttributes->subId, callbackContext->expectSubId);

    // Check we got the right options
    TEST_ASSERT_EQUAL((pSubAttributes->subOptions & callbackContext->expectOptions), callbackContext->expectOptions);
}

void test_globalMQTTQoSxRelSelection(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hOwningClient;
    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SessionHandle_t hSession1, hSession2;
    ismEngine_Subscription_t *subscription, *findByQHSub;
    const char *clientId1 = "C1";
    const char *clientId2 = "C2";

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    // Create a client state for globally shared nondurable subs (emulating the JMS code)
    rc = ism_engine_createClientState("__SharedND_TEST",
                                      PROTOCOL_ID_SHARED,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hOwningClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismQHandle_t queueHandle = NULL;
    for(int32_t loop=0; loop<1; loop++)
    {
        rc = test_createClientAndSessionWithProtocol(clientId1,
                                                     PROTOCOL_ID_MQTT,
                                                     NULL,
                                                     ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                     &hClient1, &hSession1, true);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = test_createClientAndSessionWithProtocol(clientId2,
                                                     PROTOCOL_ID_MQTT,
                                                     NULL,
                                                     ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                     &hClient2, &hSession2, true);
        TEST_ASSERT_EQUAL(rc, OK);

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                        ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |
                                                        ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT |
                                                        ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY,
                                                        88 };

        rc = ism_engine_reuseSubscription(hClient1,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs,
                                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

        rc = sync_ism_engine_createSubscription(hClient1,
                                                SUBSCRIPTION_NAME,
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                SUBSCRIPTION_TOPIC,
                                                &subAttrs,
                                                hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check the subscription now exists
        rc = iett_findClientSubscription(pThreadData,
                                         "__SharedND_TEST",
                                         iett_generateClientIdHash("__SharedND_TEST"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, SUBSCRIPTION_TOPIC);
        queueHandle = subscription->queueHandle;

        rc = iett_findQHandleSubscription(pThreadData, queueHandle, &findByQHSub);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(findByQHSub, subscription);
        rc = iett_releaseSubscription(pThreadData, findByQHSub, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check the shared sub information
        iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
        TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettNO_ANONYMOUS_SHARER);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 1);
        TEST_ASSERT_NOT_EQUAL(sharedSubData->sharingClients[0].subOptions & ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY, 0);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE, 0);

        rc = iett_releaseSubscription(pThreadData, subscription, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check that the subscription is listed on 1st client (and has expected RELIABLY_ONLY flag)
        testListSubsCallbackContext_t listSubContext = {SUBSCRIPTION_NAME,
                                                        SUBSCRIPTION_TOPIC,
                                                        (ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY |
                                                         ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE),
                                                        subAttrs.subId };

        rc = ism_engine_listSubscriptions(hClient1, SUBSCRIPTION_NAME, &listSubContext, test_listSubsCallback);
        TEST_ASSERT_EQUAL(rc, OK);

        ismEngine_SubscriptionAttributes_t subAttrs2 = { ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                         ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                                                         ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT |
                                                         ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY,
                                                         99 };

        // Reuse from different client
        rc = ism_engine_reuseSubscription(hClient2,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs2,
                                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);

        // RELIABLE_MSGS_ONLY should be on in the sharingClients subOptions
        TEST_ASSERT_NOT_EQUAL(sharedSubData->sharingClients[0].subOptions & ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY, 0);

        // Check the subscription is listed on 2nd client
        listSubContext.expectOptions = ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY |
                                       ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
        listSubContext.expectSubId = subAttrs2.subId;

        rc = ism_engine_listSubscriptions(hClient2, SUBSCRIPTION_NAME, &listSubContext, test_listSubsCallback);
        TEST_ASSERT_EQUAL(rc, OK);

        // Test engine diagnostics
        char requestStr[4096];
        char *outputStr = NULL;

        sprintf(requestStr, "%s=%s %s=%s",
                ediaVALUE_FILTER_CLIENTID, hOwningClient->pClientId,
                ediaVALUE_FILTER_SUBNAME, SUBSCRIPTION_NAME);

        rc = ism_engine_diagnostics(ediaVALUE_MODE_SUBDETAILS,
                                    requestStr,
                                    &outputStr,
                                    NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(outputStr);
        // Check we have both C1 and C2 included in the output
        TEST_ASSERT_PTR_NOT_NULL(strstr(outputStr, "\"SharingClientCount\":2"));
        TEST_ASSERT_PTR_NOT_NULL(strstr(outputStr, hClient1->pClientId));
        TEST_ASSERT_PTR_NOT_NULL(strstr(outputStr, hClient2->pClientId));

        //printf("OUTPUT: '%s'\n", outputStr);

        ism_engine_freeDiagnosticsOutput(outputStr);

        // Create consumers
        msgCallbackContext_t MsgContext1 = {hSession1, 0, 0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, (loop == 0), clientId1};
        msgCallbackContext_t MsgContext2 = {hSession2, 0, 0, 0, ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE, (loop == 0), clientId2};
        msgCallbackContext_t *pMsgContext1 = &MsgContext1, *pMsgContext2 = &MsgContext2;

        ismEngine_ConsumerHandle_t hConsumer1 = NULL, hConsumer2 = NULL;

        rc = ism_engine_createConsumer(hSession1,
                                       ismDESTINATION_SUBSCRIPTION,
                                       SUBSCRIPTION_NAME,
                                       NULL,
                                       hOwningClient,
                                       &pMsgContext1, sizeof(pMsgContext1), test_msgCallback,
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                       &hConsumer1,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hConsumer1);

        rc = ism_engine_createConsumer(hSession2,
                                       ismDESTINATION_SUBSCRIPTION,
                                       SUBSCRIPTION_NAME,
                                       NULL,
                                       hOwningClient,
                                       &pMsgContext2, sizeof(pMsgContext2), test_msgCallback,
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                       &hConsumer2,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hConsumer2);

        uint32_t msgsPerPublish = 40;
        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_AT_LEAST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);
        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_AT_MOST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);
        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_EXACTLY_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);

        // Check we got sharing
        TEST_ASSERT_NOT_EQUAL(pMsgContext1->received, 0);
        TEST_ASSERT_NOT_EQUAL(pMsgContext2->received, 0);
        // And only the Reliable msgs
        TEST_ASSERT_EQUAL(pMsgContext1->received + pMsgContext2->received, msgsPerPublish*2);

        rc = ism_engine_destroyConsumer(hConsumer1, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        uint32_t initialMsgsReceived2 = pMsgContext2->received;

        // Shouldn't have get any unreliable msgs
        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_AT_MOST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);
        TEST_ASSERT_EQUAL(pMsgContext2->received, initialMsgsReceived2);

        // Shouldn't get all reliable msgs
        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_AT_LEAST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);
        TEST_ASSERT_EQUAL(pMsgContext2->received, initialMsgsReceived2+msgsPerPublish);

        rc = ism_engine_destroyConsumer(hConsumer2, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // No consumer, so no more msgs
        test_publishMessages(SUBSCRIPTION_TOPIC, msgsPerPublish, ismMESSAGE_RELIABILITY_EXACTLY_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);
        TEST_ASSERT_EQUAL(pMsgContext2->received, initialMsgsReceived2+msgsPerPublish);

        if (loop == 0)
        {
            rc = ism_engine_destroySubscription(hClient1, SUBSCRIPTION_NAME, hOwningClient, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            // Check that the subscription was removed from the 1st client
            rc = iett_findClientSubscription(pThreadData,
                                             clientId1,
                                             iett_generateClientIdHash(clientId1),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             NULL);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

            // Check the  subscription is still there
            rc = iett_findClientSubscription(pThreadData,
                                             "__SharedND_TEST",
                                             iett_generateClientIdHash("__SharedND_TEST"),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             &subscription);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);

            rc = iett_releaseSubscription(pThreadData, subscription, false);
            TEST_ASSERT_EQUAL(rc, OK);

            // Clean up 1st client
            rc = test_destroyClientAndSession(hClient1, hSession1, true);
            TEST_ASSERT_EQUAL(rc, OK);

            // Check that the subscription is there on 2nd client
            rc = iett_findClientSubscription(pThreadData,
                                             clientId2,
                                             iett_generateClientIdHash(clientId2),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            // Clean up 2nd client
            rc = test_destroyClientAndSession(hClient2, hSession2, true);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        // Check that the subscrption was removed from the 2nd client
        rc = iett_findClientSubscription(pThreadData,
                                         clientId2,
                                         iett_generateClientIdHash(clientId2),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

        // Check that the subscrption was removed from the Owning client
        rc = iett_findClientSubscription(pThreadData,
                                         "__SharedND_TEST",
                                         iett_generateClientIdHash("__SharedND_TEST"),
                                         SUBSCRIPTION_NAME,
                                         iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                         NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }

    rc = iett_findQHandleSubscription(pThreadData, queueHandle, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    rc = ism_engine_destroyClientState(hOwningClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK); // Shouldn't go async
}
#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC

CU_TestInfo ISM_SharedSubs_CUnit_basicTests[] =
{
    { "SingleClientJMSNondurable", test_singleClientJMSNondurable },
    { "GlobalJMSNondurable", test_globalJMSNondurable },
    { "GlobalJMSDurable", test_globalJMSDurable },
    { "GlobalMQTTQoS0Nondurable", test_globalMQTTQoS0Nondurable },
    { "GlobalMQTTQoS0Durable", test_globalMQTTQoS0Durable },
    { "GlobalMQTTQoSxNondurable", test_globalMQTTQoSxNondurable },
    { "GlobalMQTTQoSXDurable", test_globalMQTTQoSxDurable },
    { "GlobalMQTTQoSXRelSelection", test_globalMQTTQoSxRelSelection },
    CU_TEST_INFO_NULL
};

/* TESTS WITH SECURITY ENABLED */

#define SUBSCRIPTION_NAME "JMSSecureSharedSubscription"
#define SUBSCRIPTION_TOPIC "JMS/Secure/Single/Client/Nondurable"
#define TOPICSECURITY_POLICYNAME "TopicSecurityPolicy_SND"
#define SUBSCRIPTIONSECURITY_POLICYNAME "SubscriptionSecurityPolicy"
void test_secureSingleClientJMSNondurable(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_SHARED };

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->userid = "GoodUser";

    rc = test_createClientAndSession(__func__,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Try and create subscription with no authorities in place
    rc = ism_engine_createSubscription(hClient,
                                       SUBSCRIPTION_NAME,
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       SUBSCRIPTION_TOPIC,
                                       &subAttrs,
                                       hClient,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotAuthorized);

    // Add a messaging policy that allows publish / subscribe on SUBSCRIPTION_TOPIC
    // For clients whose user id starts "Good".
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID."TOPICSECURITY_POLICYNAME"\","
                                "\"Name\":\""TOPICSECURITY_POLICYNAME"\","
                                "\"UserID\":\"Good*\","
                                "\"Topic\":\""SUBSCRIPTION_TOPIC"\","
                                "\"ActionList\":\"subscribe,publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContext, TOPICSECURITY_POLICYNAME);
    TEST_ASSERT_EQUAL(rc, OK);

    // Try and create subscription with no topic authority in place (all that's needed)
    rc = ism_engine_createSubscription(hClient,
                                       SUBSCRIPTION_NAME,
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       SUBSCRIPTION_TOPIC,
                                       &subAttrs,
                                       hClient,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish 10 messages
    test_publishMessages(SUBSCRIPTION_TOPIC, 10,ismMESSAGE_RELIABILITY_AT_MOST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);

    msgCallbackContext_t MsgContext = {hSession, 0, 0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, true, __func__};
    msgCallbackContext_t *pMsgContext = &MsgContext;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   SUBSCRIPTION_NAME,
                                   NULL,
                                   NULL,
                                   &pMsgContext, sizeof(pMsgContext), test_msgCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(MsgContext.received, 10);

    // Clean up
    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the non-durable subscription is removed
    rc = iett_findClientSubscription(pThreadData,
                                     __func__,
                                     iett_generateClientIdHash(__func__),
                                     SUBSCRIPTION_NAME,
                                     iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
}
#undef SUBSCRIPTIONSECURITY_POLICYNAME
#undef TOPICSECURITY_POLICYNAME
#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC

#define SUBSCRIPTION_NAME "JMSSecureSharedSubscription"
#define SUBSCRIPTION_TOPIC "JMS/Secure/Global/Nondurable"
#define TOPICSECURITY_POLICYNAME "TopicSecurityPolicy_GND"
void test_secureGlobalJMSNondurable(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hOwningClient;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_Subscription_t *subscription;

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_SHARED };

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->userid = "GoodUser";

    rc = test_createClientAndSession(__func__,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a client state for globally shared nondurable subs (emulating the JMS code)
    rc = ism_engine_createClientState("__SharedND_TEST",
                                      PROTOCOL_ID_SHARED,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hOwningClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Not authorized for topic
    rc = ism_engine_createSubscription(hClient,
                                       SUBSCRIPTION_NAME,
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       SUBSCRIPTION_TOPIC,
                                       &subAttrs,
                                       hOwningClient,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotAuthorized);

    // Add a messaging policy that allows publish / subscribe on SUBSCRIPTION_TOPIC
    // For clients whose user id starts "Good".
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID."TOPICSECURITY_POLICYNAME"\","
                                "\"Name\":\""TOPICSECURITY_POLICYNAME"\","
                                "\"UserID\":\"Good*\","
                                "\"Topic\":\""SUBSCRIPTION_TOPIC"\","
                                "\"ActionList\":\"subscribe,publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContext, TOPICSECURITY_POLICYNAME);
    TEST_ASSERT_EQUAL(rc, OK);

    // Authorized for topic do not need to be authorized for subscription
    rc = ism_engine_createSubscription(hClient,
                                       SUBSCRIPTION_NAME,
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       SUBSCRIPTION_TOPIC,
                                       &subAttrs,
                                       hOwningClient,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the subscription now exists
    rc = iett_findClientSubscription(pThreadData,
                                     "__SharedND_TEST",
                                     iett_generateClientIdHash("__SharedND_TEST"),
                                     SUBSCRIPTION_NAME,
                                     iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, SUBSCRIPTION_TOPIC);

    rc = iett_releaseSubscription(pThreadData, subscription, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_reuseSubscription(hClient, SUBSCRIPTION_NAME, &subAttrs, hOwningClient);
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish 100 messages
    test_publishMessages(SUBSCRIPTION_TOPIC, 100, ismMESSAGE_RELIABILITY_AT_MOST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);

    msgCallbackContext_t MsgContext = {hSession, 0, 0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, true, __func__};
    msgCallbackContext_t *pMsgContext = &MsgContext;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   SUBSCRIPTION_NAME,
                                   NULL,
                                   hOwningClient,
                                   &pMsgContext, sizeof(pMsgContext), test_msgCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(MsgContext.received, 100);

    // Clean up
    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hOwningClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the  subscription is now gone
    rc = iett_findClientSubscription(pThreadData,
                                     "__SharedND_TEST",
                                     iett_generateClientIdHash("__SharedND_TEST"),
                                     SUBSCRIPTION_NAME,
                                     iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
}
#undef TOPICSECURITY_POLICYNAME
#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC

#define SUBSCRIPTION_NAME "JMSSecureSharedSubscription"
#define SUBSCRIPTION_TOPIC "JMS/Secure/Global/Durable"
#define TOPICSECURITY_POLICYNAME "TopicSecurityPolicy_GD"
#define SUBSCRIPTIONSECURITY_POLICYNAME_1 "SubscriptionSecurityPolicy_1_GD"
#define SUBSCRIPTIONSECURITY_POLICYNAME_2 "SubscriptionSecurityPolicy_2_GD"
void test_secureGlobalJMSDurable(void)
{
    ismEngine_ClientStateHandle_t hOwningClient;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_Subscription_t *subscription;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                    ismENGINE_SUBSCRIPTION_OPTION_DURABLE };

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->userid = "GoodUser";

    rc = test_createClientAndSession(__func__,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a client state for globally shared nondurable subs (emulating the JMS code)
    rc = ism_engine_createClientState("__Shared_TEST",
                                      PROTOCOL_ID_SHARED,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hOwningClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t i=0; i<4; i++)
    {
        rc = sync_ism_engine_createSubscription(hClient,
                                                SUBSCRIPTION_NAME,
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                SUBSCRIPTION_TOPIC,
                                                &subAttrs,
                                                hOwningClient);
        if (i != 3)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_NotAuthorized);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }

        mockTransport->userid = "BadUser";

        rc = ism_engine_reuseSubscription(hClient,
                                          SUBSCRIPTION_NAME,
                                          &subAttrs,
                                          hOwningClient);
        if (i != 3)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_NotAuthorized);
        }

        mockTransport->userid = "GoodUser";

        switch(i)
        {
            case 0:
                // Add a topic policy that allows publish / subscribe on 'SUBSCRIPTION_TOPIC' for
                // clients whose user id starts "Good".
                rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                            "{\"UID\":\"UID."TOPICSECURITY_POLICYNAME"\","
                                            "\"Name\":\""TOPICSECURITY_POLICYNAME"\","
                                            "\"UserID\":\"Good*\","
                                            "\"Topic\":\""SUBSCRIPTION_TOPIC"\","
                                            "\"ActionList\":\"subscribe,publish\"}");
                TEST_ASSERT_EQUAL(rc, OK);

                rc = test_addPolicyToSecContext(mockContext, TOPICSECURITY_POLICYNAME);
                TEST_ASSERT_EQUAL(rc, OK);
                break;
            case 1:
                // Add a subscription policy that allows receive only for clients whose user id starts "Good".
                rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY,
                                            "{\"UID\":\"UID."SUBSCRIPTIONSECURITY_POLICYNAME_1"\","
                                            "\"Name\":\""SUBSCRIPTIONSECURITY_POLICYNAME_1"\","
                                            "\"UserID\":\"Good*\","
                                            "\"SubscriptionName\":\""SUBSCRIPTION_NAME"*\","
                                            "\"ActionList\":\"receive\"}");
                TEST_ASSERT_EQUAL(rc, OK);

                rc = test_addPolicyToSecContext(mockContext, SUBSCRIPTIONSECURITY_POLICYNAME_1);
                TEST_ASSERT_EQUAL(rc, OK);
                break;
            case 2:
                // Add a subscription policy that allows receive and control for clients whose user id starts "Good".
                rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY,
                                            "{\"UID\":\"UID."SUBSCRIPTIONSECURITY_POLICYNAME_2"\","
                                            "\"Name\":\""SUBSCRIPTIONSECURITY_POLICYNAME_2"\","
                                            "\"UserID\":\"Good*\","
                                            "\"SubscriptionName\":\""SUBSCRIPTION_NAME"*\","
                                            "\"ActionList\":\"receive,control\"}");
                TEST_ASSERT_EQUAL(rc, OK);

                rc = test_addPolicyToSecContext(mockContext, SUBSCRIPTIONSECURITY_POLICYNAME_2);
                TEST_ASSERT_EQUAL(rc, OK);
                break;
        }
    }

    // Check that the subscription exists and has expected properties after all of the above iterations
    ismEngine_SubscriptionMonitor_t *results = NULL;
    uint32_t resultCount;

    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                           50,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 1);
    TEST_ASSERT_EQUAL(results[0].policyType, ismSEC_POLICY_SUBSCRIPTION);
    TEST_ASSERT_STRINGS_EQUAL(results[0].messagingPolicyName, SUBSCRIPTIONSECURITY_POLICYNAME_2);

    subscription = (ismEngine_Subscription_t *)results[0].subscription;
    TEST_ASSERT_PTR_NOT_NULL(subscription);
    TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, SUBSCRIPTION_TOPIC);

    ism_engine_freeSubscriptionMonitor(results);

    // Publish 100 messages
    test_publishMessages(SUBSCRIPTION_TOPIC, 100, ismMESSAGE_RELIABILITY_AT_MOST_ONCE, ismMESSAGE_PERSISTENCE_NONPERSISTENT);

    msgCallbackContext_t MsgContext = {hSession, 0, 0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, true, __func__};
    msgCallbackContext_t *pMsgContext = &MsgContext;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   SUBSCRIPTION_NAME,
                                   NULL,
                                   hOwningClient,
                                   &pMsgContext, sizeof(pMsgContext), test_msgCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(MsgContext.received, 100);

    // Try to destroy the subscription with an active consumer
    rc = ism_engine_destroySubscription(hClient, SUBSCRIPTION_NAME, hOwningClient, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationInUse);

    // Clean up
    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t i=0; i<2; i++)
    {
        if (i == 0) mockTransport->userid = "BadUser";
        else mockTransport->userid = "GoodGuy";

        rc = test_createClientAndSession(__func__,
                                         mockContext,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hClient, &hSession, true);
        TEST_ASSERT_EQUAL(rc, OK);

        // Try to destroy the subscription
        rc = ism_engine_destroySubscription(hClient, SUBSCRIPTION_NAME, hOwningClient, NULL, 0, NULL);
        if (i == 0)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_NotAuthorized);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }

        rc = test_destroyClientAndSession(hClient, hSession, true);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hOwningClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);
}
#undef SUBSCRIPTIONSECURITY_POLICYNAME_2
#undef SUBSCRIPTIONSECURITY_POLICYNAME_1
#undef TOPICSECURITY_POLICYNAME
#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC

CU_TestInfo ISM_SharedSubs_CUnit_securityTests[] =
{
    { "SecureSingleClientJMSNondurable", test_secureSingleClientJMSNondurable },
    { "SecureGlobalJMSNondurable", test_secureGlobalJMSNondurable },
    { "SecureGlobalJMSDurable", test_secureGlobalJMSDurable },
    CU_TEST_INFO_NULL
};

// NOTE: Subscription name and topic are the ones being used in the test that raised this defect
#define SUBSCRIPTION_NAME "SubErrorTests"
#define SUBSCRIPTION_TOPIC "ChangedTheTopic"
void test_defect91020(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hOwningClient;
    ismEngine_ClientStateHandle_t hClient1, hClient2, hClient3;
    ismEngine_SessionHandle_t hSession1, hSession2, hSession3;
    ismEngine_ConsumerHandle_t hConsumer1;
    ismEngine_Subscription_t *subscription;

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    // Create a client state for globally shared nondurable subs (emulating the JMS code)
    rc = ism_engine_createClientState("__SharedND_TEST",
                                      PROTOCOL_ID_SHARED,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hOwningClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create client states for the test
    rc = test_createClientAndSession("sharedsub_error02A",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession("sharedsub_error02B",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient2, &hSession2, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession("GeneratedClientID",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient3, &hSession3, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // SubOptions like the MQTT client would specify
    ismEngine_SubscriptionAttributes_t mqttSubAttrs = { ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                                                        ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                        ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT };

    rc = ism_engine_createSubscription(hClient1,
                                       SUBSCRIPTION_NAME,
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       SUBSCRIPTION_TOPIC,
                                       &mqttSubAttrs,
                                       hOwningClient,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the subscription now exists
    rc = iett_findClientSubscription(pThreadData,
                                     "__SharedND_TEST",
                                     iett_generateClientIdHash("__SharedND_TEST"),
                                     SUBSCRIPTION_NAME,
                                     iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, SUBSCRIPTION_TOPIC);
    TEST_ASSERT_EQUAL(subscription->subOptions, (mqttSubAttrs.subOptions & ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK));
    TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER, 0);
    TEST_ASSERT_EQUAL(ieq_getQType(subscription->queueHandle), multiConsumer);
    TEST_ASSERT_EQUAL(subscription->useCount, 2); /* create + get == 2 */

    iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);
    TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
    TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettNO_ANONYMOUS_SHARER);
    TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 1);

    rc = ism_engine_reuseSubscription(hClient2,
                                      SUBSCRIPTION_NAME,
                                      &mqttSubAttrs,
                                      hOwningClient);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(subscription->useCount, 2); /* create + get == 2 */

    TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettNO_ANONYMOUS_SHARER);
    TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2);

    // Create an MQTT consumer on session1
    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_SUBSCRIPTION,
                                   SUBSCRIPTION_NAME,
                                   NULL,
                                   hOwningClient,
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer1);

    // Reuse from JMS
    ismEngine_SubscriptionAttributes_t jmsSubAttrs = { ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                                                       ismENGINE_SUBSCRIPTION_OPTION_SHARED };

    rc = ism_engine_reuseSubscription(hClient3, SUBSCRIPTION_NAME, &jmsSubAttrs, hOwningClient);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(subscription->useCount, 3); /* create + get + consumer == 3 */
    TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettANONYMOUS_SHARER_JMS_APPLICATION);

    rc = ism_engine_destroySubscription(hClient3, SUBSCRIPTION_NAME, hOwningClient, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettNO_ANONYMOUS_SHARER);

    rc = ism_engine_destroyConsumer(hConsumer1, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(subscription->useCount, 2); /* create + get */

    rc = ism_engine_destroySubscription(hClient2, SUBSCRIPTION_NAME, hOwningClient, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 1);
    TEST_ASSERT_EQUAL(subscription->useCount, 2); /* create + get */

    rc = ism_engine_destroySubscription(hClient1, SUBSCRIPTION_NAME, hOwningClient, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 0);
    TEST_ASSERT_EQUAL(subscription->useCount, 1); /* get */

    // Allow the subscription to be destroyed
    iett_releaseSubscription(pThreadData, subscription, false);

    // Clean up
    rc = test_destroyClientAndSession(hClient1, hSession1, true);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_destroyClientAndSession(hClient2, hSession2, true);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_destroyClientAndSession(hClient3, hSession3, true);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_destroyClientState(hOwningClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}
#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC

// defect 151665
typedef struct tag_test_defect151665_listCallbackContext_t
{
    char *expectedSubName;
    uint32_t subCount;
    bool expectADD_CLIENT;
} test_defect151665_listCallbackContext_t;

static void test_defect151665_listCallback( ismEngine_SubscriptionHandle_t            subHandle,
                                            const char *                              pSubName,
                                            const char *                              pTopicString,
                                            void *                                    properties,
                                            size_t                                    propertiesLength,
                                            const ismEngine_SubscriptionAttributes_t *pSubAttributes,
                                            uint32_t                                  consumerCount,
                                            void *                                    pContext)
{
    test_defect151665_listCallbackContext_t *context = (test_defect151665_listCallbackContext_t *)pContext;

    if (context->expectedSubName != NULL)
    {
        TEST_ASSERT_STRINGS_EQUAL(context->expectedSubName, pSubName);
    }

    if (context->expectADD_CLIENT == false)
    {
        TEST_ASSERT_EQUAL(pSubAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT, 0);
    }
    else
    {
        TEST_ASSERT_NOT_EQUAL(pSubAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT, 0);
    }

    __sync_add_and_fetch(&context->subCount, 1);
}

static void test_defect151665_stealCallback( int32_t reason,
                                             ismEngine_ClientStateHandle_t hClient,
                                             uint32_t options,
                                             void *pContext )
{
    TEST_ASSERT_EQUAL(reason, ISMRC_ResumedClientState);
    TEST_ASSERT_EQUAL(options, ismENGINE_STEAL_CALLBACK_OPTION_NONE);

    int32_t rc = ism_engine_destroyClientState(hClient,
                                               ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                               NULL, 0, NULL);
    if (rc != OK) TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    return;
}

#define SUBSCRIPTION_NAME "DurSubErrorTests"
#define SUBSCRIPTION_TOPIC "ChangedTheTopic"
char *clientId[3] = {"sharedsub_error02A", "sharedsub_error02B", "GeneratedClientID"};
void test_defect151665(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hOwningClient;
    ismEngine_ClientStateHandle_t MQTT_client[4], JMS_client;
    ismEngine_SessionHandle_t MQTT_session[4], JMS_session;
    ismEngine_ConsumerHandle_t MQTT_consumer[4], JMS_consumer;
    ismEngine_Subscription_t *subscription;
    iettSharedSubData_t *sharedSubData = NULL;

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    // Create a client state for globally shared nondurable subs (emulating the JMS code)
    rc = ism_engine_createClientState("__Shared_TEST",
                                      PROTOCOL_ID_SHARED,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hOwningClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_defect151665_listCallbackContext_t context = {SUBSCRIPTION_NAME, 0};
    ismEngine_SubscriptionAttributes_t subAttrs;

    // Create client states, sessions and consumers for MQTT clients
    for(int32_t i=0; i<2; i++)
    {
        rc = sync_ism_engine_createClientState(clientId[i],
                                               PROTOCOL_ID_MQTT,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                               ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                               &MQTT_client[i],
                                               test_defect151665_stealCallback,
                                               NULL,
                                               &MQTT_client[i]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(MQTT_client[i]);

        rc = ism_engine_createSession(MQTT_client[i],
                                      ismENGINE_CREATE_SESSION_OPTION_NONE,
                                      &MQTT_session[i],
                                      NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(MQTT_session[i]);

        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                              ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                              ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT;

        if (i == 0)
        {
            // 1st mqtt client uses QoS=2 and creates the sub
            subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;

            rc = sync_ism_engine_createSubscription(MQTT_client[i],
                                                    SUBSCRIPTION_NAME,
                                                    NULL,
                                                    ismDESTINATION_TOPIC,
                                                    SUBSCRIPTION_TOPIC,
                                                    &subAttrs,
                                                    hOwningClient);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = iett_findClientSubscription(pThreadData,
                                             "__Shared_TEST",
                                             iett_generateClientIdHash("__Shared_TEST"),
                                             SUBSCRIPTION_NAME,
                                             iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                             &subscription);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, SUBSCRIPTION_TOPIC);
            sharedSubData = iett_getSharedSubData(subscription);
            TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
            // In theory, when we call iett_releaseSubscription, we cannot continue to addess
            // either subscription or sharedSubData... in practice, we know it still has a non-zero
            // use count, so rather than give it an inflated use count, we release now and assume
            // we can dereference from here on in
            iett_releaseSubscription(pThreadData, subscription, false);

        }
        else
        {
            // subsequent client uses QoS=0 and expects to reuse
            subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;

            rc = ism_engine_reuseSubscription(MQTT_client[i],
                                              SUBSCRIPTION_NAME,
                                              &subAttrs,
                                              hOwningClient);
            TEST_ASSERT_EQUAL(rc, OK);

            // Look for the subscription in a list from the owning client
            context.subCount = 0;
            context.expectADD_CLIENT = false;

            rc = ism_engine_listSubscriptions(hOwningClient,
                                              SUBSCRIPTION_NAME,
                                              &context,
                                              test_defect151665_listCallback);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_EQUAL(context.subCount, 1);

            // Look for the subscription in a list from this MQTT client
            context.subCount = 0;
            context.expectADD_CLIENT = true;

            rc = ism_engine_listSubscriptions(MQTT_client[i],
                                              SUBSCRIPTION_NAME,
                                              &context,
                                              test_defect151665_listCallback);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_EQUAL(context.subCount, 1);

        }

        rc = ism_engine_createConsumer(MQTT_session[i],
                                       ismDESTINATION_SUBSCRIPTION,
                                       SUBSCRIPTION_NAME,
                                       ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                       NULL,
                                       NULL, 0, NULL,
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_ACK |
                                       ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                       &MQTT_consumer[i],
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(MQTT_consumer[i]);

        // Check the subscription now has correct counts
        TEST_ASSERT_EQUAL(subscription->consumerCount, i+1);
        TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettNO_ANONYMOUS_SHARER);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, i+1);
    }

    TEST_ASSERT_PTR_NOT_NULL(sharedSubData);

    // Steal from the 2nd MQTT client
    rc = sync_ism_engine_createClientState(clientId[1],
                                           PROTOCOL_ID_MQTT,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                           &MQTT_client[3],
                                           test_defect151665_stealCallback,
                                           NULL,
                                           &MQTT_client[3]);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);
    TEST_ASSERT_PTR_NOT_NULL(MQTT_client[3]);
    TEST_ASSERT_STRINGS_EQUAL(MQTT_client[3]->pClientId, clientId[1]);

    rc = ism_engine_createSession(MQTT_client[3],
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &MQTT_session[3],
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(MQTT_session[3]);

    context.subCount = 0;
    context.expectADD_CLIENT = true;

    rc = ism_engine_listSubscriptions(MQTT_client[3],
                                      SUBSCRIPTION_NAME,
                                      &context,
                                      test_defect151665_listCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.subCount, 1);

    TEST_ASSERT_EQUAL(subscription->consumerCount, 1); // Not created the new consumer yet
    TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettNO_ANONYMOUS_SHARER);
    TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2); // Should still have the 'interest'

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                          ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;

    // Add a reuse (which should just be a duplicate since we stole) and create a consumer
    rc = ism_engine_reuseSubscription(MQTT_client[3],
                                      SUBSCRIPTION_NAME,
                                      &subAttrs,
                                      hOwningClient);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createConsumer(MQTT_session[3],
                                   ismDESTINATION_SUBSCRIPTION,
                                   SUBSCRIPTION_NAME,
                                   NULL,
                                   NULL,
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK |
                                   ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                   &MQTT_consumer[3],
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(MQTT_consumer[3]);

    TEST_ASSERT_EQUAL(subscription->consumerCount, 2)
    TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettNO_ANONYMOUS_SHARER);
    TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2); // Should still have the 'interest'

    // Create our JMS client & session, join the subscription & create a consumer
    rc = test_createClientAndSession("GeneratedClientID",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &JMS_client, &JMS_session, true);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                          ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;

    rc = ism_engine_reuseSubscription(JMS_client,
                                      SUBSCRIPTION_NAME,
                                      &subAttrs,
                                      hOwningClient);
    TEST_ASSERT_EQUAL(rc, OK);

    context.subCount = 0;
    context.expectADD_CLIENT = false;

    // Shouldn't find the shared subscription associated with the JMS client
    rc = ism_engine_listSubscriptions(JMS_client,
                                      SUBSCRIPTION_NAME,
                                      &context,
                                      test_defect151665_listCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.subCount, 0);

    // Should find it owned by the owning client (but mustn't have ADD_CLIENT on)
    rc = ism_engine_listSubscriptions(hOwningClient,
                                      SUBSCRIPTION_NAME,
                                      &context,
                                      test_defect151665_listCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.subCount, 1);

    rc = ism_engine_createConsumer(JMS_session,
                                   ismDESTINATION_SUBSCRIPTION,
                                   SUBSCRIPTION_NAME,
                                   NULL,
                                   hOwningClient,
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &JMS_consumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(JMS_consumer);

    TEST_ASSERT_EQUAL(subscription->consumerCount, 3)
    TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettANONYMOUS_SHARER_JMS_APPLICATION);
    TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2);

    rc = sync_ism_engine_destroyConsumer(JMS_consumer);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(subscription->consumerCount, 2);

    TEST_ASSERT_EQUAL(subscription->consumerCount, 2)
    TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettANONYMOUS_SHARER_JMS_APPLICATION);
    TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2);

    // Interesting... unsubscribe would fail in the protocol layer because consumers > 0
    // but if we call the engine to destroy here, it would work (i.e. remove the JMS interest
    // but then return ISMRC_DestinationInUse... is that relevant?

    // Steal from the 1st MQTT client
    rc = sync_ism_engine_createClientState(clientId[0],
                                           PROTOCOL_ID_MQTT,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                           &MQTT_client[2],
                                           test_defect151665_stealCallback,
                                           NULL,
                                           &MQTT_client[2]);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);
    TEST_ASSERT_PTR_NOT_NULL(MQTT_client[2]);
    TEST_ASSERT_STRINGS_EQUAL(MQTT_client[2]->pClientId, clientId[0]);

    rc = ism_engine_createSession(MQTT_client[2],
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &MQTT_session[2],
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(MQTT_session[2]);

    TEST_ASSERT_EQUAL(subscription->consumerCount, 1); // Not created the new consumer yet
    TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettANONYMOUS_SHARER_JMS_APPLICATION);
    TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2);

    rc = ism_engine_createConsumer(MQTT_session[2],
                                   ismDESTINATION_SUBSCRIPTION,
                                   SUBSCRIPTION_NAME,
                                   NULL,
                                   NULL,
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK |
                                   ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                   &MQTT_consumer[2],
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(MQTT_consumer[2]);

    TEST_ASSERT_EQUAL(subscription->consumerCount, 2)
    TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettANONYMOUS_SHARER_JMS_APPLICATION);
    TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2);

    for(int32_t i=2; i<4; i++)
    {
        rc = sync_ism_engine_destroyConsumer(MQTT_consumer[i]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(subscription->consumerCount, 3-i)
        TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettANONYMOUS_SHARER_JMS_APPLICATION);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 4-i);

        rc = ism_engine_destroySubscription(MQTT_client[i],
                                            SUBSCRIPTION_NAME,
                                            hOwningClient,
                                            NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(subscription->consumerCount, 3-i)
        TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettANONYMOUS_SHARER_JMS_APPLICATION);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 3-i);
    }

    TEST_ASSERT_EQUAL(subscription->consumerCount, 0)
    TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettANONYMOUS_SHARER_JMS_APPLICATION);
    TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 0);

    // perform a 'JMS unsubscribe' which should cause the sub to disapper
    rc = ism_engine_destroySubscription(JMS_client,
                                        SUBSCRIPTION_NAME,
                                        hOwningClient,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_findClientSubscription(pThreadData,
                                     "__Shared_TEST",
                                     iett_generateClientIdHash("__Shared_TEST"),
                                     SUBSCRIPTION_NAME,
                                     iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    context.subCount = 0;
    context.expectADD_CLIENT = false;

    // Should no longer find it owned by the owning client
    rc = ism_engine_listSubscriptions(hOwningClient,
                                      SUBSCRIPTION_NAME,
                                      &context,
                                      test_defect151665_listCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.subCount, 0);

    // Clean up by creating non-durable clients (cleanSession=true steals) and then destroying
    for(int32_t i=0; i<2; i++)
    {
        rc = sync_ism_engine_createClientState(clientId[i],
                                               PROTOCOL_ID_MQTT,
                                               ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                               ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                               &MQTT_client[i],
                                               test_defect151665_stealCallback,
                                               NULL,
                                               &MQTT_client[i]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(MQTT_client[i]);

        rc = sync_ism_engine_destroyClientState(MQTT_client[i], ismENGINE_DESTROY_CLIENT_OPTION_NONE);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = test_destroyClientAndSession(JMS_client, JMS_session, true);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_destroyClientState(hOwningClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}
#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC

#define CLIENT_ID "ManyConsumersClient"
#define SUBSCRIPTION_NAME "ManyConsumersTest"
#define SUBSCRIPTION_TOPIC "ManyConsumersTopic"
void test_defect180759(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hOwningClient;
    ismEngine_ClientStateHandle_t MQTT_client;
    ismEngine_SessionHandle_t MQTT_session;
    ismEngine_ConsumerHandle_t MQTT_consumer[2] = {0};
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    // Create a client state for globally shared durable subs
    rc = ism_engine_createClientState("__Shared_TEST",
                                      PROTOCOL_ID_SHARED,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hOwningClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs;

    // Create client state, session and consumers for MQTT clients
    rc = sync_ism_engine_createClientState(CLIENT_ID,
                                           PROTOCOL_ID_MQTT,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                           &MQTT_client,
                                           test_defect151665_stealCallback,
                                           NULL,
                                           &MQTT_client);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(MQTT_client);

    rc = ism_engine_createSession(MQTT_client,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &MQTT_session,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(MQTT_session);

    rc = ism_engine_startMessageDelivery(MQTT_session,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                          ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;

    // Create the subscription and a consumer...
    rc = sync_ism_engine_createSubscription(MQTT_client,
                                            SUBSCRIPTION_NAME,
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            SUBSCRIPTION_TOPIC,
                                            &subAttrs,
                                            hOwningClient);
    TEST_ASSERT_EQUAL(rc, OK);

    testMarkMsgsContext_t markContext1 = {0};
    testMarkMsgsContext_t *pMarkContext1 = &markContext1;

    markContext1.hSession                = MQTT_session;
    markContext1.totalNumToHaveDelivered = 20;
    markContext1.numToSkipBeforeAcks     = 5;
    markContext1.numToMarkConsumed       = 15;
    markContext1.numToMarkRecv           = 0;
    markContext1.numToSkipBeforeStore    = 0;
    markContext1.numToStoreForCaller     = 0;
    markContext1.numToStoreForNack       = 0;
    markContext1.pNackHandles            = NULL;
    markContext1.pCallerHandles          = NULL;

    rc = ism_engine_createConsumer(MQTT_session,
                                   ismDESTINATION_SUBSCRIPTION,
                                   SUBSCRIPTION_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL,
                                   &pMarkContext1,
                                   sizeof(testMarkMsgsContext_t *),
                                   test_markMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK |
                                   ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                   &MQTT_consumer[0],
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(MQTT_consumer[0]);

    // Now reuse the subscription, and create a 2nd consumer...
    rc = ism_engine_reuseSubscription(MQTT_client,
                                      SUBSCRIPTION_NAME,
                                      &subAttrs,
                                      hOwningClient);
    TEST_ASSERT_EQUAL(rc, OK);

    testMarkMsgsContext_t markContext2 = {0};
    testMarkMsgsContext_t *pMarkContext2 = &markContext2;

    markContext2.hSession                = MQTT_session;
    markContext2.totalNumToHaveDelivered = 20;
    markContext2.numToSkipBeforeAcks     = 0;
    markContext2.numToMarkConsumed       = 20;
    markContext2.numToMarkRecv           = 0;
    markContext2.numToSkipBeforeStore    = 0;
    markContext2.numToStoreForCaller     = 0;
    markContext2.numToStoreForNack       = 0;
    markContext2.pNackHandles            = NULL;
    markContext2.pCallerHandles          = NULL;

    rc = ism_engine_createConsumer(MQTT_session,
                                   ismDESTINATION_SUBSCRIPTION,
                                   SUBSCRIPTION_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL,
                                   &pMarkContext2,
                                   sizeof(testMarkMsgsContext_t *),
                                   test_markMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK |
                                   ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                   &MQTT_consumer[1],
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_WaiterInUse);
    TEST_ASSERT_PTR_NULL(MQTT_consumer[1]);

    test_pubMessages(SUBSCRIPTION_TOPIC,
                     ismMESSAGE_PERSISTENCE_PERSISTENT,
                     ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                     50);

    rc = sync_ism_engine_destroyClientState(MQTT_client, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_ClientState_t *pFoundClient = iecs_getVictimizedClient(pThreadData,
                                                                     CLIENT_ID,
                                                                     iecs_generateClientIdHash(CLIENT_ID));
    TEST_ASSERT_NOT_EQUAL(pFoundClient, NULL);
    TEST_ASSERT_EQUAL(pFoundClient->OpState, iecsOpStateZombie);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyDisconnectedClientState(CLIENT_ID,
                                                   &pActionsRemaining,
                                                   sizeof(pActionsRemaining),
                                                   test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    rc = ism_engine_destroyClientState(hOwningClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}
#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC

CU_TestInfo ISM_SharedSubs_CUnit_defectTests[] =
{
    { "test_Defect91020", test_defect91020 },
    { "test_Defect151665", test_defect151665 },
    { "test_DefectXXXXX", test_defect180759 },
    CU_TEST_INFO_NULL
};

void test_ensurePolicyExists(const char *policyName, ismSecurityPolicyType_t policyType)
{
    int32_t rc;
    iepiPolicyInfo_t *policyInfo = NULL;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    rc = iepi_getEngineKnownPolicyInfo(pThreadData, policyName, policyType, &policyInfo);

    if (rc == OK)
    {
        iepi_releasePolicyInfo(pThreadData, policyInfo);
    }
    else
    {
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

        rc = iepi_createPolicyInfo(pThreadData,
                                   policyName,
                                   policyType,
                                   true,
                                   NULL,
                                   &policyInfo);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = iepi_addEngineKnownPolicyInfo(pThreadData, policyName, policyType, policyInfo);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

void test_checkIdent(const char *identLocator, const char *expectedIdent)
{
    if (expectedIdent != NULL)
    {
        char buffer[4096] = "";
        ism_common_formatLastError((char *)&buffer, sizeof(buffer));
        //printf("Error: %s\n", buffer);
        char *propertyString = strstr(buffer, identLocator);
        TEST_ASSERT_PTR_NOT_NULL(propertyString);
        int cmpVal = strncmp(propertyString+strlen(identLocator), expectedIdent, strlen(expectedIdent));
        TEST_ASSERT_EQUAL(cmpVal, 0);
    }
}

void test_badPropertyRC(int rc, const char *expectedProperty)
{
    TEST_ASSERT_EQUAL(rc, ISMRC_BadPropertyValue);
    test_checkIdent("Property: ", expectedProperty);
}

void test_wrongSubscriptionAPIRC(int rc, const char *expectedCorrectAPI)
{
    TEST_ASSERT_EQUAL(rc, ISMRC_WrongSubscriptionAPI);
    test_checkIdent("via the ", expectedCorrectAPI);
}

void test_propertyValueMismatchRC(int rc, const char *expectedProperty)
{
    TEST_ASSERT_EQUAL(rc, ISMRC_PropertyValueMismatch);
    test_checkIdent(" property ", expectedProperty);
}

void test_adminSubCreation_BadInputs(void)
{
    char configPropName[1024];

    printf("Starting %s...\n", __func__);

    ism_prop_t *configProps = ism_common_newProperties(4);
    ism_field_t f;
    f.type = VT_String;

    // No leading slash on an AdminSubscription
    int32_t rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                           "BadSubName/Topic/Filter",
                                           configProps,
                                           ISM_CONFIG_CHANGE_PROPS);
    test_badPropertyRC(rc, ismENGINE_ADMIN_SUBSCRIPTION_SUBSCRIPTIONNAME);

    // No topic filter on an AdminSubscription
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   "/BadSubName",
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    test_badPropertyRC(rc, ismENGINE_ADMIN_SUBSCRIPTION_SUBSCRIPTIONNAME);

    // Empty ShareName on an AdminSubscription
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   "//TopicFilter",
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    test_badPropertyRC(rc, ismENGINE_ADMIN_SUBSCRIPTION_SUBSCRIPTIONNAME);

    // No topic filter and empty share name
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   "/",
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    test_badPropertyRC(rc, ismENGINE_ADMIN_SUBSCRIPTION_SUBSCRIPTIONNAME);

    // Add the name to the properties (ignored -- but admin will pass it)
    char *goodAdminSubName = "/GoodSubName/Topic/Filter";
    sprintf(configPropName, "%s.%s.%s",
            ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, ismENGINE_ADMIN_PROPERTY_NAME, goodAdminSubName);
    f.val.s = goodAdminSubName;
    ism_common_setProperty(configProps, configPropName, &f);

    // Trying to specify TopicFilter separately with AdminSubscription

    sprintf(configPropName, "%s.%s.%s",
            ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, ismENGINE_ADMIN_ALLADMINSUBS_TOPICFILTER, goodAdminSubName);
    f.val.s = "Topic/Filter";
    ism_common_setProperty(configProps, configPropName, &f);
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   goodAdminSubName,
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    TEST_ASSERT_EQUAL(rc, ISMRC_BadPropertyName);

    ism_common_setProperty(configProps, configPropName, NULL);

    // Invalid MaxQualityOfService value
    sprintf(configPropName, "%s.%s.%s", ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, ismENGINE_ADMIN_ALLADMINSUBS_MAXQUALITYOFSERVICE, goodAdminSubName);
    f.val.s = "99";
    ism_common_setProperty(configProps, configPropName, &f);
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   goodAdminSubName,
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    test_badPropertyRC(rc, ismENGINE_ADMIN_ALLADMINSUBS_MAXQUALITYOFSERVICE);

    ism_common_setProperty(configProps, configPropName, NULL);

    // Invalid QualityOfServiceFilter value
    sprintf(configPropName, "%s.%s.%s", ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, ismENGINE_ADMIN_ALLADMINSUBS_QUALITYOFSERVICEFILTER, goodAdminSubName);
    f.val.s = "NOTAFILTER";
    ism_common_setProperty(configProps, configPropName, &f);

    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   goodAdminSubName,
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    test_badPropertyRC(rc, ismENGINE_ADMIN_ALLADMINSUBS_QUALITYOFSERVICEFILTER);

    ism_common_setProperty(configProps, configPropName, NULL);

    // Invalid SubOptions
    sprintf(configPropName, "%s.%s.%s", ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, ismENGINE_ADMIN_ALLADMINSUBS_SUBOPTIONS, goodAdminSubName);
    f.val.s = "TTTT";
    ism_common_setProperty(configProps, configPropName, &f);

    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   goodAdminSubName,
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    test_badPropertyRC(rc, ismENGINE_ADMIN_ALLADMINSUBS_SUBOPTIONS);

    ism_common_setProperty(configProps, configPropName, NULL);

    // Invalid SubOptions (no SHARED option)
    sprintf(configPropName, "%s.%s.%s", ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, ismENGINE_ADMIN_ALLADMINSUBS_SUBOPTIONS, goodAdminSubName);
    f.val.s = "0x1";
    ism_common_setProperty(configProps, configPropName, &f);

    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   goodAdminSubName,
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    test_badPropertyRC(rc, ismENGINE_ADMIN_ALLADMINSUBS_SUBOPTIONS);

    ism_common_setProperty(configProps, configPropName, NULL);


    // Unexpected property name
    sprintf(configPropName, "%s.INVALIDPROPERTYNAME.%s", ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, goodAdminSubName);
    f.val.s = "XYZ";
    ism_common_setProperty(configProps, configPropName, &f);

    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   goodAdminSubName,
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    TEST_ASSERT_EQUAL(rc, ISMRC_BadPropertyName);

    ism_common_setProperty(configProps, configPropName, NULL);

    // Actually create it with explicit subOptions, overriding a MaxQualityOfService explicitly
    test_ensurePolicyExists("MessagingPolicy", ismSEC_POLICY_SUBSCRIPTION);

    sprintf(configPropName, "%s.%s.%s", ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, ismENGINE_ADMIN_ALLADMINSUBS_MAXQUALITYOFSERVICE, goodAdminSubName);
    f.val.s = "0";
    ism_common_setProperty(configProps, configPropName, &f);
    sprintf(configPropName, "%s.%s.%s", ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, ismENGINE_ADMIN_ALLADMINSUBS_SUBOPTIONS, goodAdminSubName);
    f.val.s = "0x42";
    ism_common_setProperty(configProps, configPropName, &f);
    sprintf(configPropName, "%s.%s.%s", ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY, goodAdminSubName);
    f.val.s = "MessagingPolicy";
    ism_common_setProperty(configProps, configPropName, &f);

    ismEngine_ClientState_t *adminSubNamespace = NULL;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    iecs_findClientState(pThreadData, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED, false, &adminSubNamespace);
    TEST_ASSERT_PTR_NOT_NULL(adminSubNamespace);

    uint32_t prevUseCount = adminSubNamespace->UseCount;

    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   goodAdminSubName,
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    TEST_ASSERT_EQUAL(rc, OK);

    // Actual completion is async -- so let's wait until it actually happens
    while(adminSubNamespace->UseCount != prevUseCount) usleep(10);

    // Check that it exists, and has expected properties (QoS from SubOptions, not MaxQoS).
    ismEngine_Subscription_t *subscription = NULL;

    rc = iett_findClientSubscription(pThreadData,
                                     adminSubNamespace->pClientId,
                                     iett_generateClientIdHash(adminSubNamespace->pClientId),
                                     goodAdminSubName,
                                     iett_generateSubNameHash(goodAdminSubName),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subscription);
    TEST_ASSERT_EQUAL((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK),
                       ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE);
    iett_releaseSubscription(pThreadData, subscription, false);

    // Get rid of the MaxQoS and SubOptions properties
    sprintf(configPropName, "%s.%s.%s", ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, ismENGINE_ADMIN_ALLADMINSUBS_MAXQUALITYOFSERVICE, goodAdminSubName);
    ism_common_setProperty(configProps, configPropName, NULL);
    sprintf(configPropName, "%s.%s.%s", ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, ismENGINE_ADMIN_ALLADMINSUBS_SUBOPTIONS, goodAdminSubName);
    ism_common_setProperty(configProps, configPropName, NULL);

    // Specify conflicting... PolicyName
    sprintf(configPropName, "%s.%s.%s", ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY, goodAdminSubName);
    f.val.s = "OtherPolicy";
    ism_common_setProperty(configProps, configPropName, &f);

    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   goodAdminSubName,
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    test_propertyValueMismatchRC(rc, ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY);

    // Specify conflicting... MaxQualityOfService
    f.val.s = "MessagingPolicy";
    ism_common_setProperty(configProps, configPropName, &f);
    sprintf(configPropName, "%s.%s.%s", ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, ismENGINE_ADMIN_ALLADMINSUBS_MAXQUALITYOFSERVICE, goodAdminSubName);
    f.val.s = "2";
    ism_common_setProperty(configProps, configPropName, &f);
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   goodAdminSubName,
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    test_propertyValueMismatchRC(rc, ismENGINE_ADMIN_ALLADMINSUBS_MAXQUALITYOFSERVICE);

    ism_common_setProperty(configProps, configPropName, NULL);

    // Specify conflicting QualityOfServiceFilter
    sprintf(configPropName, "%s.%s.%s", ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, ismENGINE_ADMIN_ALLADMINSUBS_QUALITYOFSERVICEFILTER, goodAdminSubName);
    f.val.s = "QoS=0";
    ism_common_setProperty(configProps, configPropName, &f);
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   goodAdminSubName,
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    test_propertyValueMismatchRC(rc, ismENGINE_ADMIN_ALLADMINSUBS_QUALITYOFSERVICEFILTER);

    ism_common_setProperty(configProps, configPropName, NULL);

    ism_common_clearProperties(configProps);

    // No Topic filter
    char *goodNPASubName = "GoodNPASubName";
    sprintf(configPropName, "%s.%s.%s",
            ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB, ismENGINE_ADMIN_PROPERTY_NAME, goodNPASubName);
    f.val.s = goodNPASubName;
    ism_common_setProperty(configProps, configPropName, &f);

    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB,
                                   goodNPASubName,
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    test_badPropertyRC(rc, ismENGINE_ADMIN_ALLADMINSUBS_TOPICFILTER);

    // No Topic policy
    sprintf(configPropName, "%s.%s.%s", ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB, ismENGINE_ADMIN_ALLADMINSUBS_TOPICFILTER, goodNPASubName);
    f.val.s = "Topic/Filter";
    ism_common_setProperty(configProps, configPropName, &f);

    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB,
                                   goodNPASubName,
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    test_badPropertyRC(rc, ismENGINE_ADMIN_VALUE_TOPICPOLICY);

    // Non-existent topic policy
    sprintf(configPropName, "%s.%s.%s", ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB, ismENGINE_ADMIN_VALUE_TOPICPOLICY, goodNPASubName);
    f.val.s = "NonExistentPolicy";
    ism_common_setProperty(configProps, configPropName, &f);

    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB,
                                   goodNPASubName,
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    test_badPropertyRC(rc, ismENGINE_ADMIN_VALUE_TOPICPOLICY);
}

CU_TestInfo ISM_SharedSubs_CUnit_adminSubs_Phase0[] =
{
    { "BadInputs", test_adminSubCreation_BadInputs },
    CU_TEST_INFO_NULL
};

void test_adminSubLifeCycle_Phase1(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    ismEngine_ClientState_t *shared = NULL;
    ismEngine_ClientState_t *sharedND = NULL;
    ismEngine_ClientState_t *sharedM = NULL;

    // Server always creates the namespaces, but we want to be able to look at them...
    iecs_findClientState(pThreadData, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE, false, &shared);
    TEST_ASSERT_PTR_NOT_NULL(shared);
    iecs_findClientState(pThreadData, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_NONDURABLE, false, &sharedND);
    TEST_ASSERT_PTR_NOT_NULL(sharedND);
    iecs_findClientState(pThreadData, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED, false, &sharedM);
    TEST_ASSERT_PTR_NOT_NULL(sharedM);

    // Don't want artificially raised useCounts during the test
    iecs_releaseClientStateReference(pThreadData, shared, false, false);
    iecs_releaseClientStateReference(pThreadData, sharedND, false, false);
    iecs_releaseClientStateReference(pThreadData, sharedM, false, false);

    ism_prop_t *configProps = ism_common_newProperties(4);

    rc = test_configProcessPost("{\"SubscriptionPolicy\":"
                                   "{\"MessagingPolicy\":"
                                      "{\"Subscription\":\"*\","
                                       "\"ClientID\":\"*\","
                                       "\"ActionList\":\"Receive,Control\"}}}");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_configProcessPost("{\"TopicPolicy\":"
                                   "{\"MessagingPolicy\":"
                                      "{\"Topic\":\"*\","
                                       "\"ClientID\":\"*\","
                                       "\"ActionList\":\"Publish,Subscribe\"}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_ClientState_t *nameSpaceArray[] = { shared, sharedND, sharedM, NULL };
    ismEngine_ClientState_t **pNameSpace = nameSpaceArray;
    ismEngine_Subscription_t *subscription = NULL;
    while(*pNameSpace)
    {
        ismEngine_ClientState_t *thisNameSpace = *pNameSpace;

        char *objectType;
        char *topicFilter;
        char *objectName;
        char *subName;
        const char *messagingPolicyType;
        const char *expectedPolicyPropertyType;
        if (strcmp(thisNameSpace->pClientId, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_NONDURABLE) == 0)
        {
            objectType = ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB;
            objectName = "TestShareName";
            subName = objectName;
            topicFilter = "TestTopic/Filter";
            messagingPolicyType = ismENGINE_ADMIN_VALUE_TOPICPOLICY;
            expectedPolicyPropertyType = ismENGINE_ADMIN_VALUE_TOPICPOLICY;
        }
        else if (strcmp(thisNameSpace->pClientId, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE) == 0)
        {
            objectType = ismENGINE_ADMIN_VALUE_DURABLENAMESPACEADMINSUB;
            objectName = "TestShareName";
            subName = objectName;
            topicFilter = "TestTopic/Filter";
            messagingPolicyType = ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY;
            expectedPolicyPropertyType = ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY;
        }
        else
        {
            objectType = ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION;
            objectName = "/TestShareName/TestTopic/Filter";
            subName = objectName;
            topicFilter = NULL;
            messagingPolicyType = ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY;
            expectedPolicyPropertyType = ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY;
        }

        char configPropName[strlen(objectType)+strlen(objectName)+1024];

        ism_common_clearProperties(configProps);

        ism_field_t f;
        f.type = VT_String;

        sprintf(configPropName, "%s.%s.%s", objectType, messagingPolicyType, objectName);
        f.val.s = "MessagingPolicy";
        ism_common_setProperty(configProps, configPropName, &f);

        if (topicFilter != NULL)
        {
            sprintf(configPropName, "%s.%s.%s", objectType, ismENGINE_ADMIN_ALLADMINSUBS_TOPICFILTER, objectName);
            f.val.s = topicFilter;
            ism_common_setProperty(configProps, configPropName, &f);
        }

        // Arbitrarily choose 'AddRetainedMsgs' value
        int32_t choice = (int32_t)(rand()%100);
        sprintf(configPropName, "%s.%s.%s", objectType, ismENGINE_ADMIN_ALLADMINSUBS_ADDRETAINEDMSGS, objectName);
        f.val.s = choice > 50 ? "1" : "0";
        ism_common_setProperty(configProps, configPropName, &f);

        // Arbitrarily choose 'QualityOfServiceFilter' value
        choice = (int32_t)(rand()%100);
        if (choice>25)
        {
            sprintf(configPropName, "%s.%s.%s", objectType, ismENGINE_ADMIN_ALLADMINSUBS_QUALITYOFSERVICEFILTER, objectName);
            f.val.s = (choice > 75) ? "QoS>0" : ((choice > 50) ? "QoS=0" : "None");
            ism_common_setProperty(configProps, configPropName, &f);
        }

        // Pick MaxQualityOfService default(1) or explicit 1 or 2 (a later test forces mismatch with 0).
        choice = (int32_t)(rand()%100);
        if (choice>33)
        {
            sprintf(configPropName, "%s.%s.%s", objectType, ismENGINE_ADMIN_ALLADMINSUBS_MAXQUALITYOFSERVICE, objectName);
            f.val.s = (choice > 66) ? "2" : "1";
            ism_common_setProperty(configProps, configPropName, &f);

        }

        // Create the subscription
        uint32_t prevUseCount = thisNameSpace->UseCount;
        rc = ism_engine_configCallback(objectType,
                                       objectName,
                                       configProps,
                                       ISM_CONFIG_CHANGE_PROPS);
        TEST_ASSERT_EQUAL(rc, OK);

        while(thisNameSpace->UseCount != prevUseCount) usleep(10); // Wait for actual completion.

        // Try deleting via Subscription interface
        ism_prop_t *deleteProps = ism_common_newProperties(2);

        sprintf(configPropName, "%s%s", ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_SUBSCRIPTIONNAME, subName);
        f.val.s = subName;
        ism_common_setProperty(deleteProps, configPropName, &f);

        sprintf(configPropName, "%s%s", ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_CLIENTID, subName);
        f.val.s = thisNameSpace->pClientId;
        ism_common_setProperty(deleteProps, configPropName, &f);

        // Invalidly delete the subscription (without using config interface)
        rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_SUBSCRIPTION,
                                       subName,
                                       deleteProps,
                                       ISM_CONFIG_CHANGE_DELETE);
        test_wrongSubscriptionAPIRC(rc, objectType);

        ism_common_freeProperties(deleteProps);

        // Go through attempting to recreate with different combinations of attributes specified
        iettSharedSubData_t *sharedSubData = NULL;

        for(int32_t i=0; i<5; i++)
        {
            char *expectedSubName;

            if (strcmp(thisNameSpace->pClientId, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED) == 0)
            {
                expectedSubName = "/TestShareName/TestTopic/Filter";
            }
            else
            {
                expectedSubName = "TestShareName";
            }

            // Check that it exists, and has anonymousSharers shared set as expected
            rc = iett_findClientSubscription(pThreadData,
                                             thisNameSpace->pClientId,
                                             iett_generateClientIdHash(thisNameSpace->pClientId),
                                             expectedSubName,
                                             iett_generateSubNameHash(expectedSubName),
                                             &subscription);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(subscription);
            TEST_ASSERT_NOT_EQUAL((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK),
                                  ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE);
            sharedSubData = iett_getSharedSubData(subscription);
            TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
            TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettANONYMOUS_SHARER_ADMIN);

            bool recreate = true;
            int32_t expectRC = OK;
            const char *expectedPropertyName = NULL;

            switch(i)
            {
                case 0:
                    if (topicFilter != NULL)
                    {
                        sprintf(configPropName, "%s.%s.%s", objectType, ismENGINE_ADMIN_ALLADMINSUBS_TOPICFILTER, objectName);
                        f.val.s = "WrongTopicFilter";
                        ism_common_setProperty(configProps, configPropName, &f);
                        expectRC = ISMRC_PropertyValueMismatch;
                        expectedPropertyName = ismENGINE_ADMIN_ALLADMINSUBS_TOPICFILTER;
                    }
                    else
                    {
                        recreate = false;
                    }
                    break;
                case 1:
                    if (topicFilter != NULL)
                    {
                        ism_common_setProperty(configProps, configPropName, NULL);
                    }
                    sprintf(configPropName, "%s.%s.%s", objectType, messagingPolicyType, objectName);
                    f.val.s = "WrongMessagingPolicy";
                    ism_common_setProperty(configProps, configPropName, &f);
                    expectRC = ISMRC_PropertyValueMismatch;
                    expectedPropertyName = expectedPolicyPropertyType;
                    break;
                case 2:
                    ism_common_setProperty(configProps, configPropName, NULL);
                    sharedSubData->anonymousSharers = iettNO_ANONYMOUS_SHARER; // <- YUCK! Just to force a rewrite
                    break;
                case 3:
                    recreate = false;
                    break;
                case 4:
                    sprintf(configPropName, "%s.%s.%s", objectType, ismENGINE_ADMIN_ALLADMINSUBS_SUBOPTIONS, objectName);
                    f.val.s = "0x0141";
                    ism_common_setProperty(configProps, configPropName, &f);
                    expectRC = ISMRC_PropertyValueMismatch;
                    expectedPropertyName = ismENGINE_ADMIN_ALLADMINSUBS_SUBOPTIONS;
                    break;
                default:
                    TEST_ASSERT(false, ("Unexpected loop %d", i));
                    break;
            }

            iett_releaseSubscription(pThreadData, subscription, false);

            if (recreate)
            {
                rc = ism_engine_configCallback(objectType,
                                               objectName,
                                               configProps,
                                               ISM_CONFIG_CHANGE_PROPS);
                if (expectRC == ISMRC_PropertyValueMismatch)
                {
                    test_propertyValueMismatchRC(rc, expectedPropertyName);
                }
                else if (expectRC == ISMRC_BadPropertyValue)
                {
                    test_badPropertyRC(rc, expectedPropertyName);
                }
                else
                {
                    TEST_ASSERT_EQUAL(expectRC, rc);
                }
            }
        }

        pNameSpace++;
    }

    // Delete the subscriptions so we can create them using config component
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   "/TestShareName/TestTopic/Filter",
                                   NULL,
                                   ISM_CONFIG_CHANGE_DELETE);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_DURABLENAMESPACEADMINSUB,
                                   "TestShareName",
                                   NULL,
                                   ISM_CONFIG_CHANGE_DELETE);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB,
                                   "TestShareName",
                                   NULL,
                                   ISM_CONFIG_CHANGE_DELETE);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t i=0; i<2; i++)
    {
        // Create the AdminSubscrption via config component so that its name is returned
        // in the list on recovery.
        rc = test_configProcessPost("{\"AdminSubscription\":"
                                       "{\"/TestShareName/TestTopic/Filter\":"
                                          "{\"SubscriptionPolicy\":\"MessagingPolicy\","
                                           "\"MaxQualityOfService\":1,"
                                           "\"QualityOfServiceFilter\":\"None\","
                                           "\"AddRetainedMsgs\":false}}}");
        if (i == 0)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_RequireRunningServer);
            serverState = ISM_SERVER_RUNNING; // LIE!
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    // DurableNamespaceAdminSub
    rc = test_configProcessPost("{\"DurableNamespaceAdminSub\":"
                                   "{\"TestShareName\":"
                                      "{\"TopicFilter\":\"TestTopic/Filter\","
                                       "\"SubscriptionPolicy\":\"MessagingPolicy\","
                                       "\"MaxQualityOfService\":1,"
                                       "\"QualityOfServiceFilter\":\"None\","
                                       "\"AddRetainedMsgs\":false}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    // NonpersistentAdminsub
    rc = test_configProcessPost("{\"NonpersistentAdminSub\":"
                                   "{\"TestShareName\":"
                                      "{\"TopicFilter\":\"TestTopic/Filter\","
                                       "\"TopicPolicy\":\"MessagingPolicy\","
                                       "\"MaxQualityOfService\":1,"
                                       "\"QualityOfServiceFilter\":\"None\","
                                       "\"AddRetainedMsgs\":false}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    // NonpersistentAdminsub number prefix
    rc = test_configProcessPost("{\"NonpersistentAdminSub\":"
                                   "{\"22TestShareName\":"
                                      "{\"TopicFilter\":\"TestTopic/Filter\","
                                       "\"TopicPolicy\":\"MessagingPolicy\","
                                       "\"MaxQualityOfService\":1,"
                                       "\"QualityOfServiceFilter\":\"None\","
                                       "\"AddRetainedMsgs\":false}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    // NonpersistentAdminsub allow :
    rc = test_configProcessPost("{\"NonpersistentAdminSub\":"
                                   "{\"Test:Share:Name\":"
                                      "{\"TopicFilter\":\"TestTopic/Filter\","
                                       "\"TopicPolicy\":\"MessagingPolicy\","
                                       "\"MaxQualityOfService\":1,"
                                       "\"QualityOfServiceFilter\":\"None\","
                                       "\"AddRetainedMsgs\":false}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    // NonpersistentAdminsub allow _ as prefix and inside 
    rc = test_configProcessPost("{\"NonpersistentAdminSub\":"
                                   "{\"_Test_Share_Name\":"
                                      "{\"TopicFilter\":\"TestTopic/Filter\","
                                       "\"TopicPolicy\":\"MessagingPolicy\","
                                       "\"MaxQualityOfService\":1,"
                                       "\"QualityOfServiceFilter\":\"None\","
                                       "\"AddRetainedMsgs\":false}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    // NonpersistentAdminsub dont allow : at start
    rc = test_configProcessPost("{\"NonpersistentAdminSub\":"
                                   "{\":Test:Share:Name\":"
                                      "{\"TopicFilter\":\"TestTopic/Filter\","
                                       "\"TopicPolicy\":\"MessagingPolicy\","
                                       "\"MaxQualityOfService\":1,"
                                       "\"QualityOfServiceFilter\":\"None\","
                                       "\"AddRetainedMsgs\":false}}}");
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    // NonpersistentAdminsub dont allow /
    rc = test_configProcessPost("{\"NonpersistentAdminSub\":"
                                   "{\"Test/Share/Name\":"
                                      "{\"TopicFilter\":\"TestTopic/Filter\","
                                       "\"TopicPolicy\":\"MessagingPolicy\","
                                       "\"MaxQualityOfService\":1,"
                                       "\"QualityOfServiceFilter\":\"None\","
                                       "\"AddRetainedMsgs\":false}}}");
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    // Wait for the subscriptions to actually be created...
    subscription = NULL;
    do
    {
        rc = iett_findClientSubscription(pThreadData,
                                         ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED,
                                         iett_generateClientIdHash(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED),
                                         "/TestShareName/TestTopic/Filter",
                                         iett_generateSubNameHash("/TestShareName/TestTopic/Filter"),
                                         &subscription);
        if (rc == OK)
        {
            iett_releaseSubscription(pThreadData, subscription, false);

            rc = iett_findClientSubscription(pThreadData,
                                             ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE,
                                             iett_generateClientIdHash(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE),
                                             "TestShareName",
                                             iett_generateSubNameHash("TestShareName"),
                                             &subscription);

            if (rc == OK)
            {
                iett_releaseSubscription(pThreadData, subscription, false);

                rc = iett_findClientSubscription(pThreadData,
                                                 ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_NONDURABLE,
                                                 iett_generateClientIdHash(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_NONDURABLE),
                                                 "TestShareName",
                                                 iett_generateSubNameHash("TestShareName"),
                                                 &subscription);

                // NOTE: We release this one after the loop has finished.
            }
        }
    }
    while(rc == ISMRC_NotFound);
    TEST_ASSERT_PTR_NOT_NULL(subscription);
    TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, "TestTopic/Filter");
    iett_releaseSubscription(pThreadData, subscription, false);

    //Make sure the create is truly finished otherwise the creates in progress are against a zombie client state
    //(that we are about to delete) and in the product, creates against zombies are only against the __Shared* clients
    //that are not removed.
    //(and in the product the protocol layer does a createClientState so even the Shared clients are not zombies)
    TEST_ASSERT_PTR_NOT_NULL(sharedM);
    while (sharedM->UseCount > 1)
    {
        usleep(100);
        ismEngine_FullMemoryBarrier();
    }


    // Before we go -- destroy the __SharedM namespace and try creating an AdminSubscription
    rc = ism_engine_destroyDisconnectedClientState(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    while(sharedM != NULL)
    {
        iecs_findClientState(pThreadData, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED, false, &sharedM);
        if (sharedM != NULL)
        {
            iecs_releaseClientStateReference(pThreadData, sharedM, false, false);
        }

    }

    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   "/TestShareName/TestTopic/Filter",
                                   configProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    test_badPropertyRC(rc, ismENGINE_ADMIN_ALLADMINSUBS_NAMESPACE);

    ism_common_freeProperties(configProps);
}

CU_TestInfo ISM_SharedSubs_CUnit_adminSubs_Phase1[] =
{
    { "Creation", test_adminSubLifeCycle_Phase1 },
    CU_TEST_INFO_NULL
};

void test_adminSubLifeCycle_Phase2(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    // Take over the shared namespace, just to prove that it doesn't destroy the subs (protocol will do this)
    ismEngine_ClientState_t *shared;
    rc = sync_ism_engine_createClientState(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE,
                                           PROTOCOL_ID_SHARED,
                                           ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                           NULL, NULL, NULL, &shared);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(shared);

    serverState = ISM_SERVER_RUNNING; // LIE!

    rc = sync_ism_engine_destroyClientState(shared, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    char *subName = "/TestShareName/TestTopic/Filter";

    ism_prop_t *configProps = ism_common_newProperties(4);
    ism_field_t f;
    f.type = VT_String;

    char configPropName[strlen(subName)+100];
    sprintf(configPropName, "%s%s", ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_SUBSCRIPTIONNAME, subName);
    f.val.s = subName;
    ism_common_setProperty(configProps, configPropName, &f);

    sprintf(configPropName, "%s%s", ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_CLIENTID, subName);
    f.val.s = ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED;
    ism_common_setProperty(configProps, configPropName, &f);

    // Invalidly delete the subscription (without using AdminSubscription interface)
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_SUBSCRIPTION,
                                   subName,
                                   configProps,
                                   ISM_CONFIG_CHANGE_DELETE);
    test_wrongSubscriptionAPIRC(rc, ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION);

    // Delete the subscription properly, using the admin interface
    rc = test_configProcessDelete(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION, subName, "DiscardSharers=true");
    TEST_ASSERT_EQUAL(rc, OK);

    // Delete the subscription a 2nd time just to prove it's gone
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION,
                                   subName,
                                   NULL,
                                   ISM_CONFIG_CHANGE_DELETE);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    // TODO: MORE TESTS!
}

CU_TestInfo ISM_SharedSubs_CUnit_adminSubs_Phase2[] =
{
    { "LifeCycle", test_adminSubLifeCycle_Phase2 },
    CU_TEST_INFO_NULL
};

void test_adminSubLifeCycle_Phase3(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_Subscription_t *subscription = NULL;

    serverState = ISM_SERVER_RUNNING; // LIE!

    // Subscription should have been properly deleted
    char *subName = "/TestShareName/TestTopic/Filter";
    rc = iett_findClientSubscription(pThreadData,
                                     ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED,
                                     iett_generateClientIdHash(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED),
                                     subName,
                                     iett_generateSubNameHash(subName),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_PTR_NULL(subscription);

    // Subscription should have been created when the reconciliation happened
    subName = "TestShareName";
    rc = iett_findClientSubscription(pThreadData,
                                     ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE,
                                     iett_generateClientIdHash(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE),
                                     subName,
                                     iett_generateSubNameHash(subName),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subscription);
    iett_releaseSubscription(pThreadData, subscription, false);

    // Subscription should have been created when the reconciliation happened
    subName = "TestShareName";
    rc = iett_findClientSubscription(pThreadData,
                                     ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_NONDURABLE,
                                     iett_generateClientIdHash(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_NONDURABLE),
                                     subName,
                                     iett_generateSubNameHash(subName),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subscription);
    iett_releaseSubscription(pThreadData, subscription, false);

    // Delete the remaining subscriptions properly
    rc = test_configProcessDelete(ismENGINE_ADMIN_VALUE_DURABLENAMESPACEADMINSUB, subName, "DiscardSharers=true");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_configProcessDelete(ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB, subName, "DiscardSharers=false");
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_SharedSubs_CUnit_adminSubs_Phase3[] =
{
    { "LifeCycle", test_adminSubLifeCycle_Phase3 },
    CU_TEST_INFO_NULL
};

void test_adminSubLifeCycle_Phase4(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_Subscription_t *subscription = NULL;

    serverState = ISM_SERVER_RUNNING; // LIE!

    // All subscription should have been properly deleted
    char *subName = "/TestShareName/TestTopic/Filter";
    rc = iett_findClientSubscription(pThreadData,
                                     ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED,
                                     iett_generateClientIdHash(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED),
                                     subName,
                                     iett_generateSubNameHash(subName),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_PTR_NULL(subscription);

    subName = "TestShareName";
    rc = iett_findClientSubscription(pThreadData,
                                     ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE,
                                     iett_generateClientIdHash(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE),
                                     subName,
                                     iett_generateSubNameHash(subName),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_PTR_NULL(subscription);

    subName = "TestShareName";
    rc = iett_findClientSubscription(pThreadData,
                                     ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_NONDURABLE,
                                     iett_generateClientIdHash(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_NONDURABLE),
                                     subName,
                                     iett_generateSubNameHash(subName),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_PTR_NULL(subscription);
}

CU_TestInfo ISM_SharedSubs_CUnit_adminSubs_Phase4[] =
{
    { "LifeCycle", test_adminSubLifeCycle_Phase4 },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initSharedSubs(void)
{
    return test_engineInit_DEFAULT;
}

int initSharedSubsWarm(void)
{
    return test_engineInit(false, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int initSharedSubsSecurity(void)
{
    return test_engineInit(true, false,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termSharedSubs(void)
{
    return test_engineTerm(true);
}

int termSharedSubsWarm(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_SharedSubs_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("BasicTests", initSharedSubs, termSharedSubs, ISM_SharedSubs_CUnit_basicTests),
    IMA_TEST_SUITE("SecurityTests", initSharedSubsSecurity, termSharedSubs, ISM_SharedSubs_CUnit_securityTests),
    IMA_TEST_SUITE("DefectTests", initSharedSubs, termSharedSubs, ISM_SharedSubs_CUnit_defectTests),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_SharedSubs_CUnit_phase2suites[] =
{
   IMA_TEST_SUITE("AdminSubs", initSharedSubs, termSharedSubs, ISM_SharedSubs_CUnit_adminSubs_Phase0),
   CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_SharedSubs_CUnit_phase3suites[] =
{
   IMA_TEST_SUITE("AdminSubs", initSharedSubs, termSharedSubsWarm, ISM_SharedSubs_CUnit_adminSubs_Phase1),
   CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_SharedSubs_CUnit_phase4suites[] =
{
   IMA_TEST_SUITE("AdminSubs", initSharedSubsWarm, termSharedSubsWarm, ISM_SharedSubs_CUnit_adminSubs_Phase2),
   CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_SharedSubs_CUnit_phase5suites[] =
{
   IMA_TEST_SUITE("AdminSubs", initSharedSubs, termSharedSubsWarm, ISM_SharedSubs_CUnit_adminSubs_Phase3),
   CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_SharedSubs_CUnit_phase6suites[] =
{
   IMA_TEST_SUITE("AdminSubs", initSharedSubsWarm, termSharedSubs, ISM_SharedSubs_CUnit_adminSubs_Phase4),
   CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_SharedSubs_CUnit_phase1suites,
                                ISM_SharedSubs_CUnit_phase2suites,
                                ISM_SharedSubs_CUnit_phase3suites,
                                ISM_SharedSubs_CUnit_phase4suites,
                                ISM_SharedSubs_CUnit_phase5suites,
                                ISM_SharedSubs_CUnit_phase6suites };

int main(int argc, char *argv[])
{
    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
