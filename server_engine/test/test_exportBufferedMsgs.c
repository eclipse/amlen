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
/* Module Name: test_exportBufferedMessages.c                        */
/*                                                                   */
/* Description: Main source file for CUnit test of resource export   */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "engineInternal.h"
#include "engineHashSet.h"
#include "exportCrypto.h"
#include "exportClientState.h"
#include "exportSubscription.h"

#include "test_utils_initterm.h"
#include "test_utils_sync.h"
#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_security.h"
#include "test_utils_client.h"
#include "test_utils_options.h"
#include "test_utils_file.h"

#define MAX_ARGS 15
typedef struct tag_testMsgsCounts_t {
    uint64_t msgsArrived;         //Total number of messages arrived (sum of states below)
    uint32_t msgsArrivedDlvd;     //How many messages arrived in a delivered state
    uint32_t msgsArrivedRcvd;     //How many messages arrived in a recvd state
    uint32_t msgsArrivedConsumed; //How many messages arrived in a consumed state
} testMsgsCounts_t;


typedef struct tag_testMarkRcvdContext_t {
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    uint64_t numToMark;
    volatile uint64_t marksStarted;
    volatile uint64_t marksCompleted;
    volatile uint64_t unmarkableMsgs;
} testMarkRcvdContext_t;

typedef struct tag_testGetMsgContext_t {
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    volatile testMsgsCounts_t *pMsgCounts;
    uint32_t consumerOptions;
    bool overrideConsumerOpts;
    bool consumeMessages;
    volatile uint64_t acksStarted;
    volatile uint64_t acksCompleted;
} testGetMsgContext_t;

typedef struct tag_testClientData_t {
    char     *clientId;
    uint32_t  subOptions;
    uint32_t  msgsToMarkRcvd;
    testMsgsCounts_t expectedMsgs;
} testClientData_t;

#define SIMPLE_MSGS_PUBBED 10
#define SIMPLE_NUM_CLIENTS  3

testClientData_t SimpleClientData[SIMPLE_NUM_CLIENTS]
                                = { {  "SimpleBuffMsgs_simpq"
                                   ,   ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE
                                     | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                   , 0
                                   , {SIMPLE_MSGS_PUBBED, 0, 0, SIMPLE_MSGS_PUBBED} },
                                  {  "SimpleBuffMsgs_interq"
                                   ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                     | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                   , 3
                                   , {SIMPLE_MSGS_PUBBED, SIMPLE_MSGS_PUBBED - 3, 3, 0} },
                                  {  "SimpleBuffMsgs_multiq"
                                   ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                     | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE
                                     | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                   , 3
                                   , {SIMPLE_MSGS_PUBBED, SIMPLE_MSGS_PUBBED - 3, 3, 0} }
};

#define RETTEST_MSGS_PUBBED 1
#define RETTEST_NUM_CLIENTS  3
testClientData_t RetTestClientData[RETTEST_NUM_CLIENTS]
                                = { {  "RetTestBuffMsgs_simpq"
                                   ,   ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE
                                     | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                   , 0
                                   , {RETTEST_MSGS_PUBBED, 0, 0, RETTEST_MSGS_PUBBED} },
                                  {  "RetTestBuffMsgs_interq"
                                   ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                     | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                   , 1
                                   , {RETTEST_MSGS_PUBBED, RETTEST_MSGS_PUBBED - 1, 1, 0} },
                                  {  "RetTestBuffMsgs_multiq"
                                   ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                     | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE
                                     | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                   , 1
                                   , {RETTEST_MSGS_PUBBED, RETTEST_MSGS_PUBBED - 1, 1, 0} }
};


#define UMS_NUM_CLIENTS  2

testClientData_t UMSClientData[UMS_NUM_CLIENTS]
                               = {{  "UMSBuffMsgs_interq"
                                    ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                      | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                    , 2
                                    , {2, 0, 2, 0} },
                                  {  "UMSBuffMsgs_multiq"
                                   ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                     | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE
                                     | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                   , 2
                                   , {2, 0, 2, 0} }
};

#define NUM_SHAREDA_CLIENTS  4

testClientData_t SharedAClientData[NUM_SHAREDA_CLIENTS]
                               = {{  "SharedSubClientA0"
                                    ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                      | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                      | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                                      | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                                    , 3
                                    , {0, 0, 0, 0} /*Messages checked using separate code*/ },
                                  {  "SharedSubClientA1"
                                    ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                      | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                      | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                                      | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                                    , 3
                                    , {0, 0, 0, 0} /*Messages checked using separate code*/ },
                                    {  "SharedSubClientA2"
                                    ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                      | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                      | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                                      | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                                    , 3
                                    , {0, 0, 0, 0} /*Messages checked using separate code*/ },
                                  {  "SharedSubClientA3"
                                    ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                      | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                      | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                                      | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                                    , 3
                                    , {0, 0, 0, 0} /*Messages checked using separate code*/ },
};

#define NUM_SHAREDB_CLIENTS  4

testClientData_t SharedBClientData[NUM_SHAREDB_CLIENTS]
                               = {{  "SharedSubClientB0"
                                    ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                      | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                      | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                                      | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                                    , 3
                                    , {0, 0, 0, 0} /*Messages checked using separate code*/ },
                                  {  "SharedSubClientB1"
                                    ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                      | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                      | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                                      | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                                    , 3
                                    , {0, 0, 0, 0} /*Messages checked using separate code*/ },
                                    {  "SharedSubClientB2"
                                    ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                      | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                      | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                                      | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                                    , 3
                                    , {0, 0, 0, 0} /*Messages checked using separate code*/ },
                                  {  "SharedSubClientB3"
                                    ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                      | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                      | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                                      | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                                    , 3
                                    , {0, 0, 0, 0} /*Messages checked using separate code*/ },
};

#define NUM_SHAREDC_CLIENTS  1

testClientData_t SharedCClientData[NUM_SHAREDC_CLIENTS]
                               = {{  "NotExportedSharedSubClientC0"
                                    ,   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                      | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                      | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                                      | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                                    , 3
                                    , {0, 0, 0, 0} /*Messages checked using separate code*/ },
};

void pubMessages( const char *topicString
                , uint8_t persistence
                , uint8_t reliability
                , uint32_t numMsgs
                , bool  retained)
{
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    int32_t rc;

    rc = test_createClientAndSession("SimplePubClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    test_incrementActionsRemaining(pActionsRemaining, numMsgs);

    for (uint32_t i = 0; i < numMsgs; i++)
    {
        //Put a message
        ismEngine_MessageHandle_t hMessage;

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                retained ? ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN : ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, topicString,
                                &hMessage, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination(hSession,
                ismDESTINATION_TOPIC,
                topicString,
                NULL,
                hMessage,
                &pActionsRemaining, sizeof(pActionsRemaining),
                test_decrementActionsRemaining);

        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }
    test_waitForActionsTimeOut(pActionsRemaining, 20);
    test_destroyClientAndSession(hClient, hSession, false);
}

void pubMessagesWithDeliveryIds( const char *clientId
                               , uint32_t clientOptions
                               , const char *topicString
                               , uint8_t persistence
                               , uint8_t reliability
                               , uint32_t numMsgs
                               , uint32_t deliveryIds[numMsgs])
{
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    int32_t rc;

    rc = test_createClientAndSession(clientId,
                                     NULL,
                                     clientOptions,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    test_incrementActionsRemaining(pActionsRemaining, numMsgs);

    for (uint32_t i = 0; i < numMsgs; i++)
    {
        //Put a message
        ismEngine_MessageHandle_t hMessage;

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, topicString,
                                &hMessage, NULL);
        TEST_ASSERT_EQUAL(rc, OK);


        ismEngine_UnreleasedHandle_t hUnrel;

        rc = ism_engine_putMessageWithDeliveryIdOnDestination(
                                 hSession
                               , ismDESTINATION_TOPIC
                               , topicString
                               , NULL
                               , hMessage
                               , deliveryIds[i]
                               , &hUnrel
                               ,  &pActionsRemaining, sizeof(pActionsRemaining)
                               , test_decrementActionsRemaining);

        if (rc != ISMRC_AsyncCompletion)
        {
            TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc);
            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        }
    }
    test_waitForActionsTimeOut(pActionsRemaining, 20);
    test_destroyClientAndSession(hClient, hSession, false);
}

void markRcvdCompleted(int32_t rc, void *handle, void *context)
{
    testMarkRcvdContext_t *markContext = *(testMarkRcvdContext_t **)context;

    if (rc == OK)
    {
        __sync_fetch_and_add(&(markContext->marksCompleted), 1);
    }
}

bool markRcvdMessagesCallback(
        ismEngine_ConsumerHandle_t      hConsumer,
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
        void *                          pConsumerContext)
{
    testMarkRcvdContext_t *context = *(testMarkRcvdContext_t **)pConsumerContext;

    if (state == ismMESSAGE_STATE_DELIVERED)
    {
        __sync_fetch_and_add(&(context->marksStarted), 1);

        DEBUG_ONLY int32_t rc;
        rc = ism_engine_confirmMessageDelivery(context->hSession,
                                               NULL,
                                               hDelivery,
                                               ismENGINE_CONFIRM_OPTION_RECEIVED,
                                               pConsumerContext, sizeof (testGetMsgContext_t *),
                                               markRcvdCompleted);
        TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion, ("rc was (%d)", rc));

        if (rc == OK)
        {
            __sync_fetch_and_add(&(context->marksCompleted), 1);
        }
    }
    else
    {
        __sync_fetch_and_add(&(context->unmarkableMsgs), 1);
    }

    ism_engine_releaseMessage(hMessage);

    return (context->marksStarted < context->numToMark);
}


void ackCompleted(int32_t rc, void *handle, void *context)
{
    testGetMsgContext_t *getMsgContext = *(testGetMsgContext_t **)context;

    if (rc == OK)
    {
        __sync_fetch_and_add(&(getMsgContext->acksCompleted), 1);
    }
}

bool countMessagesCallback(
        ismEngine_ConsumerHandle_t      hConsumer,
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
        void *                          pConsumerContext)
{
    testGetMsgContext_t *context = *(testGetMsgContext_t **)pConsumerContext;
    bool doAck = false;

    __sync_fetch_and_add(&(context->pMsgCounts->msgsArrived), 1);

    if (state == ismMESSAGE_STATE_RECEIVED)
    {
        if (context->consumeMessages)
        {
            doAck = true;
        }

        __sync_fetch_and_add(&(context->pMsgCounts->msgsArrivedRcvd), 1);
    }
    else if (state == ismMESSAGE_STATE_DELIVERED)
    {
        if (context->consumeMessages)
        {
            doAck = true;
        }

        __sync_fetch_and_add(&(context->pMsgCounts->msgsArrivedDlvd), 1);
    }
    else if (state == ismMESSAGE_STATE_CONSUMED)
    {
        __sync_fetch_and_add(&(context->pMsgCounts->msgsArrivedConsumed), 1);
    }
    else
    {
        //Who the what now?
        TEST_ASSERT_EQUAL(1, 0);
    }

    if (doAck)
    {
        __sync_fetch_and_add(&(context->acksStarted), 1);

        DEBUG_ONLY int32_t rc;
        rc = ism_engine_confirmMessageDelivery(context->hSession,
                                               NULL,
                                               hDelivery,
                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                               pConsumerContext, sizeof (testGetMsgContext_t *),
                                               ackCompleted);
        TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion, ("rc was (%d)", rc));

        if (rc == OK)
        {
            __sync_fetch_and_add(&(context->acksCompleted), 1);
        }
    }

    ism_engine_releaseMessage(hMessage);

    return true;
}


void reopenSubsCallback(
        ismEngine_SubscriptionHandle_t subHandle,
        const char * pSubName,
        const char * pTopicString,
        void * properties,
        size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttrs,
        uint32_t consumerCount,
        void * pContext)
{
    DEBUG_ONLY int32_t rc;
    testGetMsgContext_t *context = (testGetMsgContext_t *)pContext;
    ismEngine_ConsumerHandle_t hConsumer;

    rc = ism_engine_createConsumer(
            context->hSession,
            ismDESTINATION_SUBSCRIPTION,
            pSubName,
            NULL,
            NULL, // Owning client same as session client
            &context,
            sizeof(testGetMsgContext_t *),
            countMessagesCallback,
            NULL,
            context->overrideConsumerOpts ?  context->consumerOptions
                                           : test_getDefaultConsumerOptions(pSubAttrs->subOptions),
            &hConsumer,
            NULL,
            0,
            NULL);

    test_log(testLOGLEVEL_TESTPROGRESS, "Recreating consumer for subscription %s, TopicString %s", pSubName, pTopicString);
    TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));
}

void connectAndGetMsgs( const char *clientId
                      , uint32_t clientOptions
                      , bool consumeMsgs
                      , bool overrideConsumeOptions
                      , uint32_t consumerOptions
                      , uint64_t waitForNumMsgs
                      , uint64_t waitForNumMilliSeconds
                      , testMsgsCounts_t *pMsgDetails)
{
    testGetMsgContext_t ctxt = {0};
    int32_t rc;

    ctxt.pMsgCounts           = pMsgDetails;
    ctxt.consumerOptions      = consumerOptions;
    ctxt.overrideConsumerOpts = overrideConsumeOptions;

    rc = test_createClientAndSession(clientId,
                                     NULL,
                                     clientOptions,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &(ctxt.hClient), &(ctxt.hSession), true);

    rc = ism_engine_listSubscriptions(
                ctxt.hClient,
                NULL,
                &ctxt,
                reopenSubsCallback);
    TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

    uint64_t waits = 0;
    while ( ((ctxt.pMsgCounts->msgsArrived < waitForNumMsgs)
                   || (ctxt.acksStarted > ctxt.acksCompleted))
            && (waits <= waitForNumMilliSeconds))
    {
        //Sleep for a millisecond
        ismEngine_FullMemoryBarrier();
        usleep(1000);
        waits++;
    }

    //If we've got the right number of seconds, wait a mo and see if extra turn up
    if (ctxt.pMsgCounts->msgsArrived == waitForNumMsgs)
    {
        usleep(1000);
    }

    test_destroyClientAndSession(ctxt.hClient, ctxt.hSession, false);
}

void connectReuseReceive( const char *clientId
                        , uint32_t clientOptions
                        , ismEngine_ClientState_t *hOwningClient
                        , const char *subName
                        , bool doReuse
                        , uint32_t subOptions
                        , uint32_t consumerOptions
                        , bool consumeMessages
                        , uint64_t waitForNumMsgs
                        , uint64_t waitForNumMilliSeconds
                        , testMsgsCounts_t *pMsgDetails)
{
    testGetMsgContext_t ctxt = {0};
    testGetMsgContext_t *pContext = &ctxt;
    int32_t rc;

    ctxt.pMsgCounts           = pMsgDetails;
    ctxt.consumerOptions      = consumerOptions;
    ctxt.overrideConsumerOpts = true;
    ctxt.consumeMessages      = consumeMessages;

    rc = test_createClientAndSession(clientId,
                                     NULL,
                                     clientOptions,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &(ctxt.hClient), &(ctxt.hSession), true);

    if (doReuse)
    {
        ismEngine_SubscriptionAttributes_t subAttrs = {subOptions};
        rc = ism_engine_reuseSubscription(ctxt.hClient,
                          subName,
                          &subAttrs,
                          hOwningClient);
                  TEST_ASSERT_EQUAL(rc, OK);
    }

    ismEngine_ConsumerHandle_t hConsumer;

        rc = ism_engine_createConsumer(
                ctxt.hSession,
                ismDESTINATION_SUBSCRIPTION,
                subName,
                NULL,
                NULL, //We're not told what to put here
                &pContext,
                sizeof(testGetMsgContext_t *),
                countMessagesCallback,
                NULL,
                consumerOptions,
                &hConsumer,
                NULL,
                0,
                NULL);

    uint64_t waits = 0;
    while ( ((ctxt.pMsgCounts->msgsArrived < waitForNumMsgs)
                   || (ctxt.acksStarted > ctxt.acksCompleted))
            && (waits <= waitForNumMilliSeconds))
    {
        //Sleep for a millisecond
        ismEngine_FullMemoryBarrier();
        usleep(1000);
        waits++;
    }

    //If we've got the right number of seconds, wait a mo and see if extra turn up
    if (ctxt.pMsgCounts->msgsArrived == waitForNumMsgs)
    {
        usleep(1000);
    }

    test_destroyClientAndSession(ctxt.hClient, ctxt.hSession, false);
}

void makeSubs( uint32_t numClients
             , testClientData_t clientData[numClients]
             , char *subName
             , char *topicString
             , ismEngine_ClientStateHandle_t hOwningClient)
{
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;
    int32_t rc;

    //Create the subscription to buffer the messages
    for (uint32_t i = 0 ; i < numClients; i++)
    {
        ismEngine_ClientStateHandle_t hClient;
        ismEngine_SessionHandle_t hSession;
        ismEngine_SubscriptionAttributes_t subAttrs = { clientData[i].subOptions };

        rc = test_createClientAndSession(clientData[i].clientId,
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hClient, &hSession, true);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hClient);
        TEST_ASSERT_PTR_NOT_NULL(hSession);

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_createSubscription(hClient,
                                           subName,
                                           NULL,
                                           ismDESTINATION_TOPIC,
                                           topicString,
                                           &subAttrs,
                                           hOwningClient,
                                           &pActionsRemaining, sizeof(pActionsRemaining),
                                           test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else if (rc == ISMRC_ExistingSubscription
                    && (clientData[i].subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED))
        {
            //Sub already exists.... reuse it
            rc = ism_engine_reuseSubscription(hClient,
                    subName,
                    &subAttrs,
                    hOwningClient);
            TEST_ASSERT_EQUAL(rc, OK);

            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        }
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

        test_waitForActionsTimeOut(pActionsRemaining, 20);

        //Disconnect not discard data
        test_destroyClientAndSession(hClient, hSession, false);
    }
}

void markMsgsReceived( uint32_t numClients
                     , testClientData_t clientData[numClients]
                     , char *subName
                     , ismEngine_ClientState_t *hOwningClient)
{
    int32_t rc;

    //If we want to mark some messages received - do that now:
    for (uint32_t i = 0 ; i < numClients; i++)
    {
        if (clientData[i].msgsToMarkRcvd > 0)
        {
            ismEngine_ClientStateHandle_t hClient;
            ismEngine_SessionHandle_t hSession;

            rc = test_createClientAndSession(clientData[i].clientId,
                                             NULL,
                                             ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                             ismENGINE_CREATE_SESSION_OPTION_NONE,
                                              &hClient, &hSession, true);
             TEST_ASSERT_EQUAL(rc, OK);
             TEST_ASSERT_PTR_NOT_NULL(hClient);
             TEST_ASSERT_PTR_NOT_NULL(hSession);

             ismEngine_ConsumerHandle_t hConsumer = NULL;
             testMarkRcvdContext_t markRcvdContext = {0};
             testMarkRcvdContext_t *pMarkRcvdContext = &markRcvdContext;

             markRcvdContext.hClient  = hClient;
             markRcvdContext.hSession = hSession;
             markRcvdContext.numToMark = clientData[i].msgsToMarkRcvd;

             //Use short delivery id == mdr
             rc = ism_engine_createConsumer(
                     hSession,
                     ismDESTINATION_SUBSCRIPTION,
                     subName,
                     0,
                     hOwningClient,
                     &pMarkRcvdContext,
                     sizeof(testMarkRcvdContext_t *),
                     markRcvdMessagesCallback,
                     NULL,
                     ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                     &hConsumer,
                     NULL,
                     0,
                     NULL);
             TEST_ASSERT_EQUAL(rc, OK);

             uint64_t waits = 0;
             while (  markRcvdContext.marksCompleted != clientData[i].msgsToMarkRcvd
                     && (waits <= 20*1000)) //wait for 20 secs at most
             {
                 //Sleep for a millisecond
                 ismEngine_FullMemoryBarrier();
                 usleep(1000);
                 waits++;
             }

             TEST_ASSERT_EQUAL(markRcvdContext.marksCompleted, clientData[i].msgsToMarkRcvd);

             //Disconnect not discard data
             test_destroyClientAndSession(hClient, hSession, false);
        }
    }
}

void deleteClientData( uint32_t numClients
                     , testClientData_t clientData[numClients])
{
    int32_t rc;

    for (uint32_t i = 0 ; i < numClients; i++)
    {
        ismEngine_ClientStateHandle_t hClient;
        ismEngine_SessionHandle_t hSession;

        rc = test_createClientAndSession(clientData[i].clientId,
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hClient, &hSession, true);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = sync_ism_engine_destroyClientState(hClient,
                                                ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
        TEST_ASSERT_EQUAL(rc, OK);

    }
}

void checkClientsReceivedExpectedMessages( uint32_t numClients
                                         , testClientData_t clientData[numClients])
{
    for (uint32_t i = 0 ; i < numClients; i++)
    {
        testMsgsCounts_t msgDetails = {0 };

        uint32_t consumerOptions = test_getDefaultConsumerOptions(clientData[i].subOptions);

        if (consumerOptions & ismENGINE_CONSUMER_OPTION_ACK)
        {
            //We want to test MDRs...
            consumerOptions |= ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID;
        }

        connectAndGetMsgs( clientData[i].clientId
                         , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                         , false
                         , true
                         , consumerOptions
                         , clientData[i].expectedMsgs.msgsArrived
                         , 300000
                         , &msgDetails);

        TEST_ASSERT_EQUAL(msgDetails.msgsArrived,         clientData[i].expectedMsgs.msgsArrived);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd,     clientData[i].expectedMsgs.msgsArrivedDlvd);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd,     clientData[i].expectedMsgs.msgsArrivedRcvd);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, clientData[i].expectedMsgs.msgsArrivedConsumed);
    }
}

typedef struct tag_getDeliveryIdsContext_t {
    uint32_t *pNumIdsStored;
    size_t   resultsArraySize;
    uint32_t *pResultsArray;
} getDeliveryIdsContext_t;

void getDeliveryIdsForClientCB(
    uint32_t                        deliveryId,
    ismEngine_UnreleasedHandle_t    hUnrel,
    void *                          pContext)
{
    getDeliveryIdsContext_t *pDIdContext = (getDeliveryIdsContext_t *)pContext;

    uint32_t arrayPos = *(pDIdContext->pNumIdsStored);

    (*(pDIdContext->pNumIdsStored))++;

    TEST_ASSERT_CUNIT(*(pDIdContext->pNumIdsStored) <= pDIdContext->resultsArraySize,
                      ("Client had more delivery ids %u than the provided results array %u. Latest deliveryId had value %u",
                              *(pDIdContext->pNumIdsStored), pDIdContext->resultsArraySize, deliveryId));

    pDIdContext->pResultsArray[arrayPos] = deliveryId;

}
//Get the delivery ids incoming from a publishing client that haven't been released
void getDeliveryIdsForClient( const char * clientId
                            , uint32_t clientOptions
                            , uint32_t *pNumIdsStored
                            , size_t resultsArraySize
                            , uint32_t resultsArray[resultsArraySize])
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    int32_t rc;

    rc = test_createClientAndSession(clientId,
            NULL,
            clientOptions,
            ismENGINE_CREATE_SESSION_OPTION_NONE,
            &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    *pNumIdsStored = 0; //We haven't recovered any yet
    getDeliveryIdsContext_t context = {pNumIdsStored, resultsArraySize, &(resultsArray[0])};

    rc = ism_engine_listUnreleasedDeliveryIds(
         hClient,
          &context,
         getDeliveryIdsForClientCB);
    TEST_ASSERT_EQUAL(rc, OK);


    rc = sync_ism_engine_destroyClientState(hClient,
            ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

}

//****************************************************************************
/// @brief Not a real unit test... dodgy way of loading a file for study.
//****************************************************************************

void test_capability_LoadFile(void)
{
    int32_t rc;

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    sslInit();

    uint64_t requestID=0;
    char *importFilePath = NULL;

    //ought to get filename from srcfullpath - currently have to set both!!!
    char *filename = "faildata";
    char *srcfullpath = "/var/tmp/faildata";
    char *filepasswd = "notarealpasswd";

    rc = ieie_fullyQualifyResourceFilename(pThreadData, filename, true, &importFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(importFilePath);

    rc = test_copyFile(srcfullpath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);
    rc = sync_ism_engine_importResources(filename,
                                         filepasswd,
                                         ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                         &requestID);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect a non-zero requestID

    sslTerm();

}

//****************************************************************************
/// @brief Test Basic operation of exporting inflight messages
//****************************************************************************

void test_capability_Simple(void)
{
    int32_t rc;

    char *topicString = "/TEST/BASICSUB";

    char fileName[255];
    char *exportFilePath = NULL;
    char *importFilePath = NULL;
    char *suffix = ".enc";
    const char *filePassword = __func__;
    char *subName = "BASICSUB";
    uint64_t requestID;

    strcpy(fileName, __func__);
    strcat(fileName, suffix);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, false, &exportFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(exportFilePath);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, true, &importFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(importFilePath);

    sslInit();


    makeSubs( SIMPLE_NUM_CLIENTS
            , SimpleClientData
            , subName
            , topicString
            , NULL);

    //Load up some messages
    pubMessages(topicString,
                ismMESSAGE_PERSISTENCE_PERSISTENT,
                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                SIMPLE_MSGS_PUBBED, false);

    markMsgsReceived( SIMPLE_NUM_CLIENTS
                    , SimpleClientData
                    , subName
                    , NULL);


    // Export them all
    rc = sync_ism_engine_exportResources("SimpleBuffMsgs",
                                         NULL,
                                         fileName,
                                         filePassword,
                                         ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                         &requestID);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(requestID, 0);


    //Destroy the client data so it is there on import:
    deleteClientData( SIMPLE_NUM_CLIENTS
                    , SimpleClientData);

    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);

    rc = sync_ism_engine_importResources(fileName,
                                         filePassword,
                                         ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                         &requestID);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect a non-zero requestID

    checkClientsReceivedExpectedMessages( SIMPLE_NUM_CLIENTS
                                        , SimpleClientData);

    sslTerm();

    iemem_free(pThreadData, iemem_exportResources, importFilePath);
    iemem_free(pThreadData, iemem_exportResources, exportFilePath);
}

//****************************************************************************
/// @brief Test Basic exporting/importing messages when there are retained
///        messages in the import that match the subscription
//****************************************************************************

void test_capability_Retained(void)
{
    int32_t rc;

    char *topicString = "/TEST/RETAINEDEXPORTTEST";

    char fileName[255];
    char *exportFilePath = NULL;
    char *importFilePath = NULL;
    char *suffix = ".enc";
    const char *filePassword = __func__;
    char *subName = "RETAINEDEXPORTTEST";
    uint64_t requestID;

    strcpy(fileName, __func__);
    strcat(fileName, suffix);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, false, &exportFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(exportFilePath);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, true, &importFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(importFilePath);

    sslInit();


    makeSubs( RETTEST_NUM_CLIENTS
            , RetTestClientData
            , subName
            , topicString
            , NULL);

    //Load up a retained message
    pubMessages(topicString,
                ismMESSAGE_PERSISTENCE_PERSISTENT,
                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                1, true);

    markMsgsReceived( RETTEST_NUM_CLIENTS
                    , RetTestClientData
                    , subName
                    , NULL);

    // Export them all
    rc = sync_ism_engine_exportResources("RetTestBuffMsgs",
                                         topicString,
                                         fileName,
                                         filePassword,
                                         ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                         &requestID);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(requestID, 0);


    //Destroy the client data so it is there on import:
    deleteClientData( RETTEST_NUM_CLIENTS
                    , RetTestClientData);

    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);

    rc = sync_ism_engine_importResources(fileName,
                                         filePassword,
                                         ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                         &requestID);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect a non-zero requestID

    checkClientsReceivedExpectedMessages( RETTEST_NUM_CLIENTS
                                        , RetTestClientData);

    sslTerm();

    iemem_free(pThreadData, iemem_exportResources, importFilePath);
    iemem_free(pThreadData, iemem_exportResources, exportFilePath);
}



//send qos=2 Message, export/import resend check no dups and
//                   & restart resend and check no dups
//                   then release message state and check can send message with same id
void test_capability_UnreleasedMessageStates(void)
{
    int32_t rc;

     char *topicString = "/TEST/UMS";
     char *pubClientFewId = "pubUMSClientFew";

     char fileName[255];
     char *exportFilePath = NULL;
     char *importFilePath = NULL;
     char *suffix = ".enc";
     const char *filePassword = __func__;
     uint64_t requestID;

     strcpy(fileName, __func__);
     strcat(fileName, suffix);

     ieutThreadData_t *pThreadData = ieut_getThreadData();

     test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

     rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, false, &exportFilePath);
     TEST_ASSERT_EQUAL(rc, OK);
     TEST_ASSERT_PTR_NOT_NULL(exportFilePath);

     rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, true, &importFilePath);
     TEST_ASSERT_EQUAL(rc, OK);
     TEST_ASSERT_PTR_NOT_NULL(importFilePath);

     sslInit();

     uint32_t deliveryIds[2] = {1, 10 };

     pubMessagesWithDeliveryIds( pubClientFewId
                               , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                               , topicString
                               , ismMESSAGE_PERSISTENCE_PERSISTENT
                               , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                               , 2
                               , deliveryIds);


     // Export the pub client with few deliveryIds
     rc = sync_ism_engine_exportResources(pubClientFewId,
                                          NULL,
                                          fileName,
                                          filePassword,
                                          ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                          &requestID);
     TEST_ASSERT_EQUAL(rc, OK);
     TEST_ASSERT_NOT_EQUAL(requestID, 0);


     //Delete state for the client we exported

     ismEngine_ClientStateHandle_t hClient;
     ismEngine_SessionHandle_t hSession;

     rc = test_createClientAndSession(pubClientFewId,
                                      NULL,
                                      ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                      ismENGINE_CREATE_SESSION_OPTION_NONE,
                                      &hClient, &hSession, true);
     TEST_ASSERT_EQUAL(rc, OK);

     rc = sync_ism_engine_destroyClientState(hClient,
                                             ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
     TEST_ASSERT_EQUAL(rc, OK);


     rc = test_copyFile(exportFilePath, importFilePath);
     TEST_ASSERT_EQUAL(rc, 0);

     rc = sync_ism_engine_importResources(fileName,
                                          filePassword,
                                          ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                          &requestID);
     TEST_ASSERT_EQUAL(rc, OK);
     TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect a non-zero requestID

     uint32_t storedDeliveryIds[2];
     uint32_t numIdsStored = 0;
     getDeliveryIdsForClient(pubClientFewId, ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                            , &numIdsStored
                            , 2, storedDeliveryIds);

     TEST_ASSERT_EQUAL(numIdsStored, 2);
     TEST_ASSERT_CUNIT(   (storedDeliveryIds[0] == deliveryIds[0] && storedDeliveryIds[1] == deliveryIds[1])
                       || (storedDeliveryIds[0] == deliveryIds[1] && storedDeliveryIds[1] == deliveryIds[0]),
                       ("Wrong deliveryIds"));


     //So if we managed to sucessfully import a couple of delivery ids - check that we can do more
     //than we store in a single memory chunk....

     char *pubClientLotsId = "pubUMSClientLots";
     uint32_t numLotsDeliveryIds = 278;
     uint32_t deliveryIdsLots[numLotsDeliveryIds];

     //Set the delivery ids - we don't want them all to be consecutive as they aren't always in real life
     for (uint32_t i=0; i<numLotsDeliveryIds; i++)
     {
         deliveryIdsLots[i] = i * 2 + 1;
     }

     pubMessagesWithDeliveryIds( pubClientLotsId
             , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
             , topicString
             , ismMESSAGE_PERSISTENCE_PERSISTENT
             , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
             , numLotsDeliveryIds
             , deliveryIdsLots);


     // Export the pub client with publishes in flight

     char LotsFileName[255];
     char *LotsExportFilePath = NULL;
     char *LotsImportFilePath = NULL;

     sprintf(LotsFileName, "Lots%s%s",__func__, suffix);

     test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

     rc = ieie_fullyQualifyResourceFilename(pThreadData, LotsFileName, false, &LotsExportFilePath);
     TEST_ASSERT_EQUAL(rc, OK);
     TEST_ASSERT_PTR_NOT_NULL(exportFilePath);

     rc = ieie_fullyQualifyResourceFilename(pThreadData, LotsFileName, true, &LotsImportFilePath);
     TEST_ASSERT_EQUAL(rc, OK);
     TEST_ASSERT_PTR_NOT_NULL(importFilePath);

     rc = sync_ism_engine_exportResources(pubClientLotsId,
             NULL,
             LotsFileName,
             filePassword,
             ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
             &requestID);
     TEST_ASSERT_EQUAL(rc, OK);
     TEST_ASSERT_NOT_EQUAL(requestID, 0);


     //Delete state for the clients we exported
     rc = test_createClientAndSession(pubClientLotsId,
             NULL,
             ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
             ismENGINE_CREATE_SESSION_OPTION_NONE,
             &hClient, &hSession, true);
     TEST_ASSERT_EQUAL(rc, OK);

     rc = sync_ism_engine_destroyClientState(hClient,
             ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
     TEST_ASSERT_EQUAL(rc, OK);

     rc = test_copyFile(LotsExportFilePath, LotsImportFilePath);
     TEST_ASSERT_EQUAL(rc, 0);

     rc = sync_ism_engine_importResources(LotsFileName,
             filePassword,
             ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
             &requestID);
     TEST_ASSERT_EQUAL(rc, OK);
     TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect a non-zero requestID

     uint32_t storedLotsDeliveryIds[numLotsDeliveryIds];
     getDeliveryIdsForClient(pubClientLotsId, ismENGINE_CREATE_CLIENT_OPTION_DURABLE
             , &numIdsStored
             , numLotsDeliveryIds, storedLotsDeliveryIds);

     TEST_ASSERT_EQUAL(numIdsStored, numLotsDeliveryIds);

     for (uint32_t i=0; i < numLotsDeliveryIds; i++)
     {
         uint32_t matches = 0;

         //Check that it was one we were supposed to store
         for (uint32_t j=0; j < numLotsDeliveryIds; j++)
         {
             if (storedLotsDeliveryIds[i] == deliveryIdsLots[j])
             {
                 matches++;
             }
         }

         TEST_ASSERT_CUNIT(matches == 1,
                 ("Wrong number of matches for stored deliveryid in position %u (value %u)",
                         i, storedLotsDeliveryIds[i]));

         //Check this id has only been stored once
         matches = 0;
         for (uint32_t j=i+1; j < numLotsDeliveryIds; j++)
         {
             if (storedLotsDeliveryIds[i] == storedLotsDeliveryIds[j])
             {
                 matches++;
             }
         }
         TEST_ASSERT_CUNIT(matches == 0,
                 ("Duplicates found for stored deliveryid in position %u (value %u)",
                         i, storedLotsDeliveryIds[i]));
     }

     sslTerm();

     iemem_free(pThreadData, iemem_exportResources, importFilePath);
     iemem_free(pThreadData, iemem_exportResources, exportFilePath);
     iemem_free(pThreadData, iemem_exportResources, LotsImportFilePath);
     iemem_free(pThreadData, iemem_exportResources, LotsExportFilePath);
}

//Make shared sub ("exportedSharedSub") on /test/shared/norestart
//Make another sub ("exportedSharedSubPostRestart") on /test/shared/postrestart
//Connect 4 clients to each (SharedSubClientA[0-3])
//Pub 20 messages on /test/shared/norestart
//get each client in turn to connect and to mark 3 messages received
//Pub 20 messages on /test/shared/postrestart
//get each client in turn to connect and to mark 3 messages received
//Now - on both subs each client has received 3 messages and there are 8 unacked

//Make shared sub ("NOTexportedSharedSub") on NOTEXPORTED/test/shared/norestart
//Make another sub ("NOTexportedSharedSubPostRestart") on NOTEXPORTED/test/shared/postrestart
//Make another sub NOTexportedSharedSubNoInflight on  NOTEXPORTED/test/shared/norestart
//Connect a different 4 clients to all 3 (SharedSubClientB[0-3])
//Pub 20 messages on NOTEXPORTED/test/shared/norestart
//get each client in turn to connect to NOTexportedSharedSub and to mark 3 messages received
//Pub 20 messages on NOTEXPORTED/test/shared/postrestart
//get each client in turn to connect to NOTexportedSharedSubPostRestart and to mark 3 messages received

//Make another sharedSub: "NOTExportedSharedSubNoExportedConnectedClients" on NOTEXPORTED/test/shared/norestart/noexportedclients
//Connect a new client to it: NOTExportedSharedSubClientC
//Publish 20 messages to it

//Make another shared sub: NOTExportedSharedSubPartialExportedConnectedClients on NOTEXPORTED/test/shared/norestart/partialexportedclients
//By partial exported we mean some of the clients connect will be exported and some won't
//Connect NOTExportedSharedSubClientC client to it
//Connect SharedSubA clients to it
//Publish 20 messages to it and make each of the SharedSubA & SharedSubC clients mark 3 received

//export/delete all clients/delete the sub/import

//Connect a new client to each of the norestart subs:
//    exportedSharedSub - should connect and get 8 messages
//    NOTexportedSharedSub - should get 8 messages (all clients connected to this some matched so we bring avail messages)
//    NOTexportedSharedSubNoInflight - should get 20 messages (all clients connected to this some matched so we bring avail messages)
//    NOTExportedSharedSubNoExportedConnectedClients - should not be able to connect - no reason it was included in export
//    NOTExportedSharedSubPartialExportedConnectedClients - should connect but get no messages - some clients not exported so we left avail msgs
//
//Reconnect the sharedsubA clients and check they get received messages with the right delivery ids
//RESTART
//Connect a new client to each of the 2 restart subs (both should work)
//New client should receive 8 messages from each of the 2 he connected to
//Reconnect the sharedsubB clients and check they get received messages with the right delivery ids
void test_capability_Shared(void)
{
    int32_t rc;

    char *topicStringExportedNoRestart                       = "/test/shared/norestart";
    char *topicStringNotExportedNoRestart                    = "notexported/test/shared/norestart";
    char *topicStringExportedPostRestart                     = "/test/shared/postrestart";
    char *topicStringNotExportedPostRestart                  = "notexported/test/shared/postrestart";
    char *topicStringNotExportedNoRestartNoExportedClients   = "notexported/test/shared/norestart/noexportedclients";
    char *topicStringNotExportedNoRestartSomeExportedClients = "notexported/test/shared/norestart/partialexportedclients";


    char *subNameExportedNoRestart                         = "exportedSharedSub";
    char *subNameExportedPostRestart                       = "exportedPostRestart";
    char *subNameNotExportedNoRestartNoInflight            = "NOTexportedSharedSubNoInflight";
    char *subNameNotExportedNoRestart                      = "NOTexportedSharedSub";
    char *subNameNotExportedPostRestart                    = "NOTexportedSharedSubPostRestart";
    char *subNameNotExportedNoExportedConnectedClients     = "NOTExportedSharedSubNoExportedConnectedClients";
    char *subNameNotExportedPartialExportedConnectedClient = "NOTExportedSharedSubPartialExportedConnectedClients";

    char fileName[255];
    char *exportFilePath = NULL;
    char *importFilePath = NULL;
    char *suffix = ".enc";
    const char *filePassword = __func__;

    uint64_t requestID;

    strcpy(fileName, __func__);
    strcat(fileName, suffix);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, false, &exportFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(exportFilePath);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, true, &importFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(importFilePath);

    sslInit();

    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSession;

    rc = test_createClientAndSessionWithProtocol(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE,
                                                 PROTOCOL_ID_SHARED,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hSharedOwningClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    //Make shared sub ("exportedSub") on /test/shared/norestart
    //Make another sub ("exportedSubPostRestart") on /test/shared/postrestart
    //Connect 4 clients to each (SharedSubClientA[0-3])
    makeSubs( NUM_SHAREDA_CLIENTS
            , SharedAClientData
            , subNameExportedNoRestart
            , topicStringExportedNoRestart
            , hSharedOwningClient);

    makeSubs( NUM_SHAREDA_CLIENTS
            , SharedAClientData
            , subNameExportedPostRestart
            , topicStringExportedPostRestart
            , hSharedOwningClient);

    //Pub 20 messages on /test/shared/norestart
    pubMessages(topicStringExportedNoRestart,
                ismMESSAGE_PERSISTENCE_PERSISTENT,
                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                20, false);

    //Mark 3 received on each of the 4 clients
    markMsgsReceived( NUM_SHAREDA_CLIENTS
                    , SharedAClientData
                    , subNameExportedNoRestart
                    , hSharedOwningClient);

    //Pub 20 messages on /test/shared/postrestart
    pubMessages(topicStringExportedPostRestart,
                ismMESSAGE_PERSISTENCE_PERSISTENT,
                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                20, false);

    //get each client in turn to connect and to mark 3 messages
    markMsgsReceived( NUM_SHAREDA_CLIENTS
                    , SharedAClientData
                    , subNameExportedPostRestart
                    , hSharedOwningClient);
    //Now - on both subs each client has received 3 messages and there are 8 unacked

    //Make shared sub ("NOTexportedSharedSub") on NOTEXPORTED/test/shared/norestart
    //Make another sub ("NOTexportedSharedSubPostRestart") on NOTEXPORTED/test/shared/postrestart
    //Make another sub NOTexportedSharedSubNoInflight on  NOTEXPORTED/test/shared/norestart
    //Connect a different 4 clients to all 3 (SharedSubClientB[0-3])
    makeSubs( NUM_SHAREDB_CLIENTS
            , SharedBClientData
            , subNameNotExportedNoRestart
            , topicStringNotExportedNoRestart
            , hSharedOwningClient);

    makeSubs( NUM_SHAREDB_CLIENTS
            , SharedBClientData
            , subNameNotExportedPostRestart
            , topicStringNotExportedPostRestart
            , hSharedOwningClient);

    makeSubs( NUM_SHAREDB_CLIENTS
            , SharedBClientData
            , subNameNotExportedNoRestartNoInflight
            , topicStringNotExportedNoRestart
            , hSharedOwningClient);

    //Pub 20 messages on NOTEXPORTED/test/shared/norestart
    pubMessages(topicStringNotExportedNoRestart,
                ismMESSAGE_PERSISTENCE_PERSISTENT,
                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                20, false);

    //get each client in turn to connect to NOTexportedSharedSub and to mark 3 messages received
    markMsgsReceived( NUM_SHAREDB_CLIENTS
                    , SharedBClientData
                    , subNameNotExportedNoRestart
                    , hSharedOwningClient);

    //Pub 20 messages on NOTEXPORTED/test/shared/postrestart
    pubMessages(topicStringNotExportedPostRestart,
                ismMESSAGE_PERSISTENCE_PERSISTENT,
                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                20, false);

    //get each client in turn to connect to NOTexportedSharedSubPostRestart and to mark 3 messages received
    markMsgsReceived( NUM_SHAREDB_CLIENTS
                    , SharedBClientData
                    , subNameNotExportedPostRestart
                    , hSharedOwningClient);

    //Make another sharedSub: "NOTExportedSharedSubNoExportedConnectedClients" on NOTEXPORTED/test/shared/norestart/noexportedclients
    //Connect a new client to it: NOTExportedSharedSubClientC
    //Publish 20 messages to it
    makeSubs( NUM_SHAREDC_CLIENTS
            , SharedCClientData
            , subNameNotExportedNoExportedConnectedClients
            , topicStringNotExportedNoRestartNoExportedClients
            , hSharedOwningClient);

    pubMessages(topicStringNotExportedNoRestartNoExportedClients,
                ismMESSAGE_PERSISTENCE_PERSISTENT,
                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                20, false);

    //Make another shared sub: NOTExportedSharedSubPartialExportedConnectedClients on NOTEXPORTED/test/shared/norestart/partialexportedclients
    //By partial exported we mean some of the clients connect will be exported and some won't
    //Connect NOTExportedSharedSubClientC client to it
    //Connect SharedSubA clients to it
    //Publish 20 messages to it and make each of the SharedSubA & SharedSubC clients mark 3 received
    makeSubs( NUM_SHAREDC_CLIENTS
            , SharedCClientData
            , subNameNotExportedPartialExportedConnectedClient
            , topicStringNotExportedNoRestartSomeExportedClients
            , hSharedOwningClient);

    makeSubs( NUM_SHAREDA_CLIENTS
            , SharedAClientData
            , subNameNotExportedPartialExportedConnectedClient
            , topicStringNotExportedNoRestartSomeExportedClients
            , hSharedOwningClient);

    pubMessages(topicStringNotExportedNoRestartSomeExportedClients,
                ismMESSAGE_PERSISTENCE_PERSISTENT,
                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                20, false);

    markMsgsReceived( NUM_SHAREDC_CLIENTS
                    , SharedCClientData
                    , subNameNotExportedPartialExportedConnectedClient
                    , hSharedOwningClient);

    markMsgsReceived( NUM_SHAREDA_CLIENTS
                    , SharedAClientData
                    , subNameNotExportedPartialExportedConnectedClient
                    , hSharedOwningClient);


    // Export them all
    rc = sync_ism_engine_exportResources("^SharedSubClient",
                                         "^/test/shared",
                                         fileName,
                                         filePassword,
                                         ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                         &requestID);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(requestID, 0);

    //Destroy the client data so it is there on import:
    deleteClientData( NUM_SHAREDA_CLIENTS
                    , SharedAClientData);

    deleteClientData( NUM_SHAREDB_CLIENTS
                    , SharedBClientData);

    deleteClientData( NUM_SHAREDC_CLIENTS
                    , SharedCClientData);

    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);

    rc = sync_ism_engine_importResources(fileName,
                                         filePassword,
                                         ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                         &requestID);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect a non-zero requestID

    //Connect a new client to each of the norestart subs:
    //    exportedSharedSub - should connect and get 8 messages
    //    NOTexportedSharedSub - should get 8 messages (all clients connected to this some matched so we bring avail messages)
    //    NOTexportedSharedSubNoInflight - should get 20 messages (all clients connected to this some matched so we bring avail messages)
    //    NOTExportedSharedSubNoExportedConnectedClients - should not be able to connect - no reason it was included in export
    //    NOTExportedSharedSubPartialExportedConnectedClients - should connect but get no messages - some clients not exported so we left avail msgs


    testMsgsCounts_t msgDetails = {0};
    char *tempClientId = "newNonImportedClient";
    uint32_t tempClientSubOptions =  ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                   | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                   | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                                   | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT;
    uint32_t tempClientConsumerOptions =   ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID
                                         | ismENGINE_CONSUMER_OPTION_ACK;

    connectReuseReceive( tempClientId
                       , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                       , hSharedOwningClient
                       , subNameExportedNoRestart
                       , true
                       , tempClientSubOptions
                       , tempClientConsumerOptions
                       , true
                       , 8
                       , 20000
                       , &msgDetails);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 8);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd, 0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd, msgDetails.msgsArrived);

    memset(&msgDetails, 0, sizeof(msgDetails));

    connectReuseReceive( tempClientId
                       , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                       , hSharedOwningClient
                       , subNameNotExportedNoRestart
                       , true
                       , tempClientSubOptions
                       , tempClientConsumerOptions
                       , true
                       , 8
                       , 20000
                       , &msgDetails);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 8);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd, 0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd, msgDetails.msgsArrived);

    memset(&msgDetails, 0, sizeof(msgDetails));

    connectReuseReceive( tempClientId
                       , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                       , hSharedOwningClient
                       , subNameNotExportedNoRestartNoInflight
                       , true
                       , tempClientSubOptions
                       , tempClientConsumerOptions
                       , true
                       , 20
                       , 20000
                       , &msgDetails);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 20);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd, 0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd, msgDetails.msgsArrived);

    //On a subscription where the topic string doesn't match and only some of the
    //subs match... we don't bring the available messages....
    memset(&msgDetails, 0, sizeof(msgDetails));
    connectReuseReceive( tempClientId
                       , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                       , hSharedOwningClient
                       , subNameNotExportedPartialExportedConnectedClient
                       , true
                       , tempClientSubOptions
                       , tempClientConsumerOptions
                       , true
                       , 0
                       , 5000
                       , &msgDetails);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd, 0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd, msgDetails.msgsArrived);

    //Check we can't reuse the sub who didn't match the topic string and had no exported subscriptions connected:
    ismEngine_ClientStateHandle_t htmpClient;
    ismEngine_SessionHandle_t htmpSession;
    rc = test_createClientAndSession(tempClientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &htmpClient, &htmpSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = { tempClientSubOptions };
    rc = ism_engine_reuseSubscription(htmpClient,
                          subNameNotExportedNoExportedConnectedClients,
                          &subAttrs,
                          hSharedOwningClient);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    test_destroyClientAndSession(htmpClient, htmpSession, false);

    for (uint32_t i = 0 ; i < NUM_SHAREDA_CLIENTS; i++)
    {
        memset(&msgDetails, 0, sizeof(msgDetails));

        connectReuseReceive(SharedAClientData[i].clientId
                           , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                           , hSharedOwningClient
                           , subNameExportedNoRestart
                           , true
                           , SharedAClientData[i].subOptions
                           , ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID
                           | ismENGINE_CONSUMER_OPTION_ACK
                           , true
                           , 3
                           , 20000
                           , &msgDetails);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 3);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd, msgDetails.msgsArrived);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd, 0);

        memset(&msgDetails, 0, sizeof(msgDetails));

        connectReuseReceive(SharedAClientData[i].clientId
                           , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                           , hSharedOwningClient
                           , subNameNotExportedPartialExportedConnectedClient
                           , true
                           ,  SharedAClientData[i].subOptions
                           ,  ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID
                            | ismENGINE_CONSUMER_OPTION_ACK
                           , true
                           , 3
                           , 20000
                           , &msgDetails);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 3);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd, msgDetails.msgsArrived);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd, 0);
    }


    sslTerm();

    iemem_free(pThreadData, iemem_exportResources, importFilePath);
    iemem_free(pThreadData, iemem_exportResources, exportFilePath);
}

CU_TestInfo ISM_ExportResources_CUnit_test_capability_PreRestart[] =
{
//A simple way to load in an export file to a unit test env
//    { "LoadFile", test_capability_LoadFile },
    { "Basic",    test_capability_Simple },
    { "UMS",      test_capability_UnreleasedMessageStates },
    { "Shared",   test_capability_Shared },
    { "Retained", test_capability_Retained },
    CU_TEST_INFO_NULL
};


void test_capability_SimplePostRestart(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    for (uint32_t i = 0 ; i < SIMPLE_NUM_CLIENTS; i++)
    {
        testMsgsCounts_t msgDetails = {0 };
        uint64_t msgsExpected = SIMPLE_MSGS_PUBBED;

        uint32_t consumerOptions = test_getDefaultConsumerOptions(SimpleClientData[i].subOptions);

        if (consumerOptions & ismENGINE_CONSUMER_OPTION_ACK)
        {
            //We want to test MDRs...
            consumerOptions |= ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID;
        }

        if ((SimpleClientData[i].subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK) == ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE)
        {
            //After a restart these messages will be gone.
            msgsExpected = 0;
        }
        connectAndGetMsgs( SimpleClientData[i].clientId
                         , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                         , true
                         , true
                         , consumerOptions
                         , msgsExpected
                         , (msgsExpected > 0) ? 300000 : 400
                         , &msgDetails);

        if ( msgsExpected > 0)
        {
            TEST_ASSERT_EQUAL(msgDetails.msgsArrived,         SimpleClientData[i].expectedMsgs.msgsArrived);
            TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd,     SimpleClientData[i].expectedMsgs.msgsArrivedDlvd);
            TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd,     SimpleClientData[i].expectedMsgs.msgsArrivedRcvd);
            TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, SimpleClientData[i].expectedMsgs.msgsArrivedConsumed);
        }
        else
        {
            TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 0);
        }
    }
}

//Check after restart that the UMSs are still remembered for the client (so they were written to the store
void test_capability_UnreleasedMessageStatesPostRestart(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    uint32_t storedDeliveryIds[2];
    uint32_t numIdsStored = 0;
    uint32_t expectedDeliveryIds[2] = {1, 10 };
    char *pubClientIdFew = "pubUMSClientFew";

    getDeliveryIdsForClient(pubClientIdFew, ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                           , &numIdsStored
                           , 2, storedDeliveryIds);

    TEST_ASSERT_EQUAL(numIdsStored, 2);
    TEST_ASSERT_CUNIT(   (storedDeliveryIds[0] == expectedDeliveryIds[0] && storedDeliveryIds[1] == expectedDeliveryIds[1])
                      || (storedDeliveryIds[0] == expectedDeliveryIds[1] && storedDeliveryIds[1] == expectedDeliveryIds[0]),
                      ("Wrong deliveryIds"));

    uint32_t expectedDeliveryIdLots=278;
    uint32_t storedDeliveryIdsLots[expectedDeliveryIdLots];
    char *pubClientIdLots = "pubUMSClientLots";

    getDeliveryIdsForClient(pubClientIdLots, ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                           , &numIdsStored
                           , expectedDeliveryIdLots, storedDeliveryIdsLots);
    TEST_ASSERT_EQUAL(numIdsStored, expectedDeliveryIdLots);

}

void test_capability_SharedPostRestart(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    //Shared up the __Shared Client that acts as namespace for shared subs
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSession;
    int32_t rc;

    rc = test_createClientAndSessionWithProtocol(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE,
                                                 PROTOCOL_ID_SHARED,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hSharedOwningClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    //Connect a new client to each of the postrestart subs:
    //    exportedSharedSubPostRestart - should connect and get 8 messages
    //    NOTexportedSharedSubPostRestart - should get 8 messages (all clients connected to this some matched so we bring avail messages)
    char *subNameExportedPostRestart                       = "exportedPostRestart";
    char *subNameNotExportedPostRestart                    = "NOTexportedSharedSubPostRestart";

    testMsgsCounts_t msgDetails = {0};
    char *tempClientId = "newNonImportedClient";
    uint32_t tempClientSubOptions =  ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                   | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                   | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                                   | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT;
    uint32_t tempClientConsumerOptions =   ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID
                                         | ismENGINE_CONSUMER_OPTION_ACK;

    connectReuseReceive( tempClientId
                       , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                       , hSharedOwningClient
                       , subNameExportedPostRestart
                       , true
                       , tempClientSubOptions
                       , tempClientConsumerOptions
                       , true
                       , 8
                       , 20000
                       , &msgDetails);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 8);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd, 0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd, msgDetails.msgsArrived);

    memset(&msgDetails, 0, sizeof(msgDetails));

    connectReuseReceive( tempClientId
                       , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                       , hSharedOwningClient
                       , subNameNotExportedPostRestart
                       , true
                       , tempClientSubOptions
                       , tempClientConsumerOptions
                       , true
                       , 8
                       , 20000
                       , &msgDetails);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 8);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd, 0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd, msgDetails.msgsArrived);

printf("\n\n\n\nDoing reconnect of existing clients...\n");

    for (uint32_t i = 0 ; i < NUM_SHAREDA_CLIENTS; i++)
    {
        memset(&msgDetails, 0, sizeof(msgDetails));

        connectReuseReceive(SharedAClientData[i].clientId
                           , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                           , hSharedOwningClient
                           , subNameExportedPostRestart
                           , true
                           ,  SharedAClientData[i].subOptions
                           ,  ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID
                            | ismENGINE_CONSUMER_OPTION_ACK
                           , true
                           , 3
                           , 20000
                           , &msgDetails);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 3);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd, msgDetails.msgsArrived);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd, 0);
    }

    for (uint32_t i = 0 ; i < NUM_SHAREDB_CLIENTS; i++)
    {
        memset(&msgDetails, 0, sizeof(msgDetails));

        connectReuseReceive(SharedBClientData[i].clientId
                           , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                           , hSharedOwningClient
                           , subNameNotExportedPostRestart
                           , true
                           ,  SharedBClientData[i].subOptions
                           ,  ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID
                            | ismENGINE_CONSUMER_OPTION_ACK
                           , true
                           , 3
                           , 20000
                           , &msgDetails);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 3);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd, msgDetails.msgsArrived);
        TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd, 0);
    }
}

CU_TestInfo ISM_ExportResources_CUnit_test_capability_PostRestart[] =
{
    { "BasicPostRestart", test_capability_SimplePostRestart },
    { "UMSPostRestart",   test_capability_UnreleasedMessageStatesPostRestart },
    { "SharedPostRestart", test_capability_SharedPostRestart },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initPhase0(void)
{
    return test_engineInit(true, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termPhase0(void)
{
    return test_engineTerm(false);
}

//For phases after phase 0
int initPhaseN(void)
{
    return test_engineInit(false, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}
//For phases after phase 0
int termPhaseN(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_ExportResources_CUnit_phase0suites[] =
{
    IMA_TEST_SUITE("Simple", initPhase0, termPhase0, ISM_ExportResources_CUnit_test_capability_PreRestart),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_ExportResources_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("SimplePostRestart", initPhaseN, termPhaseN, ISM_ExportResources_CUnit_test_capability_PostRestart),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_ExportResources_CUnit_phase0suites
                              , ISM_ExportResources_CUnit_phase1suites };


int32_t parse_args( int argc
                  , char **argv
                  , uint32_t *pPhase
                  , uint32_t *pFinalPhase
                  , uint32_t *pTestLogLevel
                  , int32_t *pTraceLevel
                  , char **adminDir)
{
    int retval = 0;

    if (argc > 1)
    {
        for(int i=1; i<argc; i++)
        {
            if (!strcasecmp(argv[i], "FULL"))
            {
                if (i != 1)
                {
                    retval = 97;
                    break;
                }
                // Driven from 'make fulltest' ignore this.
            }
            else if (argv[i][0] == '-')
            {
                //All these args are of the form -<flag> <Value> so check we have a value:
                if ((i+1) >= argc)
                {
                    printf("Found a flag: %s but there was no following arg!\n", argv[i]);
                    retval = 96;
                    break;
                }

                if (!strcasecmp(argv[i], "-a"))
                {
                    //Admin dir...
                    i++;
                    *adminDir = argv[i];
                }
                else if (!strcasecmp(argv[i], "-p"))
                {
                    //start phase...
                    i++;
                    *pPhase = (uint32_t)atoi(argv[i]);

                }
                else if (!strcasecmp(argv[i], "-f"))
                {
                    //final phase...
                    i++;
                    *pFinalPhase = (uint32_t)atoi(argv[i]);
                }
                else if (!strcasecmp(argv[i], "-v"))
                {
                    //final phase...
                    i++;
                    *pTestLogLevel= (uint32_t)atoi(argv[i]);
                }
                else if (!strcasecmp(argv[i], "-t"))
                {
                    i++;
                    *pTraceLevel= (uint32_t)atoi(argv[i]);
                }
                else
                {
                    printf("Unknown flag arg: %s\n", argv[i]);
                    retval = 95;
                    break;
                }
            }
            else
            {
                printf("Unknown arg: %s\n", argv[i]);
                retval = 94;
                break;
            }
        }
    }
    return retval;
}

void setupArgsForNextPhase(int argc, char *argv[], char *nextPhaseText, char *adminDir, char *NextPhaseArgv[])
{
    bool phasefound=false, adminDirfound=false;
    uint32_t i;
    TEST_ASSERT(argc < MAX_ARGS-5, ("argc was %d", argc));

    for (i=0; i < argc; i++)
    {
        NextPhaseArgv[i]=argv[i];
        if (strcmp(NextPhaseArgv[i], "-p") == 0)
        {
            NextPhaseArgv[i+1]=nextPhaseText;
            i++;
            phasefound=true;
        }
        if (strcmp(NextPhaseArgv[i], "-a") == 0)
        {
            NextPhaseArgv[i+1]=adminDir;
            i++;
            adminDirfound=true;
        }
    }

    if (!phasefound)
    {
        NextPhaseArgv[i++]="-p";
        NextPhaseArgv[i++]=nextPhaseText;
    }

    if (!adminDirfound)
    {
        NextPhaseArgv[i++]="-a";
        NextPhaseArgv[i++]=adminDir;
    }

    NextPhaseArgv[i]=NULL;

}

int main(int argc, char *argv[])
{
    int32_t trclvl = 0;
    uint32_t testLogLevel = testLOGLEVEL_TESTDESC;
    int retval = 0;
    char *adminDir=NULL;
    uint32_t phase=0;
    uint32_t finalPhase=1;

    ism_time_t seedVal = ism_common_currentTimeNanos();

    srand(seedVal);

    CU_initialize_registry();

    retval = parse_args( argc
                       , argv
                       , &phase
                       , &finalPhase
                       , &testLogLevel
                       , &trclvl
                       , &adminDir);
    if (retval != OK) goto mod_exit;

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, adminDir);

    char localAdminDir[1024];
    if (adminDir == NULL && test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir)) == true)
    {
        adminDir = localAdminDir;
    }

    if (retval == 0)
    {
        CU_register_suites(PhaseSuites[phase]);

        CU_basic_run_tests();

        CU_RunSummary * CU_pRunSummary_Final;
        CU_pRunSummary_Final = CU_get_run_summary();
        printf("Random Seed =     %"PRId64"\n", seedVal);
        printf("\n[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
               CU_pRunSummary_Final->nTestsRun,
               CU_pRunSummary_Final->nTestsFailed,
               CU_pRunSummary_Final->nAssertsFailed);
        if ((CU_pRunSummary_Final->nTestsFailed > 0) ||
            (CU_pRunSummary_Final->nAssertsFailed > 0))
        {
            retval = 1;
        }
    }

    CU_cleanup_registry();

    if (phase < finalPhase)
    {
        test_log(2, "Restarting for Phase %u...", phase+1);
        char *nextPhaseArgv[MAX_ARGS];
        char nextPhaseText[16];
        sprintf(nextPhaseText, "%d", phase+1);
        setupArgsForNextPhase(argc, argv, nextPhaseText, adminDir, nextPhaseArgv);
        retval = test_execv(nextPhaseArgv[0], nextPhaseArgv);
        TEST_ASSERT(false, ("'test_execv' failed. rc = %d", retval));
    }
    else
    {
        test_log(2, "Success. Test complete!\n");
        test_processTerm(false);
        test_removeAdminDir(adminDir);
    }

mod_exit:

    return retval;
}
