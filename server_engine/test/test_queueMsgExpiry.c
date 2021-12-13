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
/* Module Name: test_queueMsgExpiry.c                                */
/*                                                                   */
/* Description: Main source file for CUnit test of deleting queues.  */
/* Intention is to have a mix of deterministic and more random       */
/* multi-threaded tests                                              */
/*                                                                   */
/*********************************************************************/
#include <sys/prctl.h>

#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_client.h"
#include "test_utils_initterm.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

#include "engine.h"
#include "engineInternal.h"
#include "queueNamespace.h"
#include "messageExpiry.h"
#include "engineMonitoring.h"

#include "multiConsumerQInternal.h"
#include "lockManager.h"

/*******************************************************************/
/* First some "Deterministic" tests                                */
/*******************************************************************/
static void sentMessage(int32_t rc, void *handle, void *pContext)
{
    TEST_ASSERT_CUNIT(rc == OK, ("rc was %d", rc));
    volatile uint64_t *pNumSent = *(volatile uint64_t **)pContext;

    ASYNCPUT_CB_ADD_AND_FETCH(pNumSent, 1);
}

void sendMessagesWithExpiry( char *topicString
                           , int numMessages
                           , int expiryValues[numMessages])
{
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ProducerHandle_t hProducer=NULL;
    volatile uint64_t numSent = 0;
    volatile uint64_t *pNumSent = &numSent;

    rc = test_createClientAndSession("senderClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_TOPIC,
                                   topicString,
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer);


    for (int i = 0; i < numMessages; i++)
    {
        ismEngine_MessageHandle_t hMessage;

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                i%2,
                                i%3,
                                ismMESSAGE_FLAGS_NONE,
                                expiryValues[i],
                                ismDESTINATION_TOPIC,
                                topicString,
                                &hMessage, NULL);
        TEST_ASSERT_EQUAL(rc, OK);



        rc = ism_engine_putMessage(hSession,
                                   hProducer,
                                   NULL,
                                   hMessage,
                                   &pNumSent,
                                   sizeof(pNumSent),
                                   sentMessage);

        TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                                           ("rc was %d",  rc));

        if (rc == OK)
        {
            ASYNCPUT_CB_ADD_AND_FETCH(pNumSent, 1);
        }
    }

    //Destroy producer, note we haven't committed the transaction yet
    rc = ism_engine_destroyProducer(hProducer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    while (numSent < numMessages)
    {
        sched_yield();
    }
}


typedef struct tag_test_SubscriptionData_t {
        uint32_t subscriptionOptions;
        uint32_t consumerOptions;
        ismEngine_ClientStateHandle_t hClient;
        ismEngine_SessionHandle_t hSession;
        ismEngine_ConsumerHandle_t hConsumer;
        uint32_t ConsumerExpectedExpiry;
        ismQHandle_t qHandle;
        iemeQueueExpiryData_t *pQExpiryData;
        uint64_t numMessagesProtocolClaimExpired;
} test_SubscriptionData_t;


bool checkExpiryCB(
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
        void *                          pConsumerContext,
        ismEngine_DelivererContext_t *  _delivererContext)
{
    test_SubscriptionData_t *pSubData = *(test_SubscriptionData_t **)pConsumerContext;
    uint32_t  expectedExpiry = pSubData->ConsumerExpectedExpiry;

    TEST_ASSERT_EQUAL(expectedExpiry, pMsgDetails->Expiry);

    ism_engine_releaseMessage(hMessage);

    if (state != ismMESSAGE_STATE_CONSUMED) {
        int32_t rc = ism_engine_confirmMessageDelivery(pSubData->hSession,
                NULL,
                hDelivery,
                ismENGINE_CONFIRM_OPTION_CONSUMED,
                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    return false;
}


bool claimExpiredCB(
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
        void *                          pConsumerContext,
        ismEngine_DelivererContext_t *  _delivererContext)
{
    test_SubscriptionData_t *pSubData = *(test_SubscriptionData_t **)pConsumerContext;

    ism_engine_releaseMessage(hMessage);

    if (state != ismMESSAGE_STATE_CONSUMED) {
        //Lie and say it expired
        int32_t rc = ism_engine_confirmMessageDelivery(pSubData->hSession,
                                                        NULL,
                                                        hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_EXPIRED,
                                                        NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        pSubData->numMessagesProtocolClaimExpired++;
    }
    return true;
}


bool neverCalledCB(
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
        void *                          pConsumerContext,
        ismEngine_DelivererContext_t *  _delivererContext)
{
    //This callback should never fire
    TEST_ASSERT_EQUAL(1, 0);

    return false;
}

void getMessageCheckExpiry( test_SubscriptionData_t *pSubData
                          , uint32_t expectedExpiry)
{
     //Tell the consumer what expiry to expect
    pSubData->ConsumerExpectedExpiry = expectedExpiry;

    //Deliver a message (only 1 as the consumer will turn itself off)
    int32_t rc = ism_engine_startMessageDelivery( pSubData->hSession
                                   , ismENGINE_START_DELIVERY_OPTION_NONE
                                   , NULL,0,NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_basicCheckMessagesExpiry(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;

    //Set  options for each subData structure (remaining elements initialised to 0)
    test_SubscriptionData_t subData[] = { {   ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE
                                            , ismENGINE_CONSUMER_OPTION_NONE},
                                          {   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                            ,   ismENGINE_CONSUMER_OPTION_ACK
                                              | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID},
                                          {   ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE
                                            ,   ismENGINE_CONSUMER_OPTION_ACK },
                                          {ismENGINE_SUBSCRIPTION_OPTION_NONE} };
    uint32_t numberOfSubs=0; //We'll set this to the number of subs in above array in a sec...
    iemnMessagingStatistics_t overallStats;
    char *topicString = "/test/basic/checkExpiry";

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    //Stop the expiry thread interfering in this test
    ismEngine_serverGlobal.msgExpiryControl->reaperEndRequested = true;
    ieme_wakeMessageExpiryReaper(pThreadData);

    /* Create our clients, sessions and consumers for each sub */
    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        char clientId[32];

        sprintf(clientId, "subClient%u", subNum);

        rc = test_createClientAndSession(clientId,
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &(subData[subNum].hClient),
                                         &(subData[subNum].hSession), false);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(subData[subNum].hClient);
        TEST_ASSERT_PTR_NOT_NULL(subData[subNum].hSession);

        test_SubscriptionData_t *pSubData = &(subData[subNum]);
        ismEngine_SubscriptionAttributes_t subAttrs = { subData[subNum].subscriptionOptions };

        rc = ism_engine_createConsumer( subData[subNum].hSession
                                      , ismDESTINATION_TOPIC
                                      , topicString
                                      , &subAttrs
                                      , NULL // Owning client same as session client
                                      , &pSubData
                                      , sizeof(pSubData)
                                      , checkExpiryCB
                                      , NULL
                                      , subData[subNum].consumerOptions
                                      , &(subData[subNum].hConsumer)
                                      , NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        subData[subNum].qHandle = ((ismEngine_Consumer_t *)(subData[subNum].hConsumer))->queueHandle;

        numberOfSubs++;
    }

    //Put 50 message and check all expiry leaving no messages
    //(we are testing putting more than the pointers we keep in the per
    //queue data for quick reference)
    int numFirstShort=50;
    int firstShortExpiries[numFirstShort];

    for (uint32_t i = 0; i < numFirstShort; i++)
    {
        firstShortExpiries[i] = (ism_common_nowExpire() - 1);
    }
    sendMessagesWithExpiry( topicString, numFirstShort, firstShortExpiries);

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, numberOfSubs*numFirstShort);

    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        ieme_reapQExpiredMessages(pThreadData, (ismEngine_Queue_t *)(subData[subNum].qHandle));

        ismEngine_QueueStatistics_t stats = {0};
        ieq_getStats(pThreadData, subData[subNum].qHandle, &stats);
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
        TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, numFirstShort);
        TEST_ASSERT_EQUAL(stats.ExpiredMsgs, numFirstShort);
        ismEngine_Queue_t *q = (ismEngine_Queue_t *)(subData[subNum].qHandle);
        subData[subNum].pQExpiryData = (iemeQueueExpiryData_t *)(q->QExpiryData);
        TEST_ASSERT_PTR_NOT_NULL(subData[subNum].pQExpiryData);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesWithExpiry, 0);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesInArray, 0);
    }

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, 0);

    //Put 1 message with very long expiry and check it doesn't expire
    int longExpiry[1] = {(ism_common_nowExpire() + 500000)};
    sendMessagesWithExpiry( topicString, 1, longExpiry);


    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        ismEngine_QueueStatistics_t stats = {0};
        ieq_getStats(pThreadData, subData[subNum].qHandle, &stats);

        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 1);
        TEST_ASSERT_EQUAL(stats.ExpiredMsgs, numFirstShort);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesWithExpiry, 1);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesInArray, 1);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->earliestExpiryMessages[0].Expiry, longExpiry[0]);
    }

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, numberOfSubs);


    //Put 1 message and check it expires, leaving the original message
    int shortExpiry[1] = {(ism_common_nowExpire() - 1)};
    sendMessagesWithExpiry( topicString, 1, shortExpiry);

    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        ieme_reapQExpiredMessages(pThreadData, (ismEngine_Queue_t *)(subData[subNum].qHandle));

        ismEngine_QueueStatistics_t stats = {0};
        ieq_getStats(pThreadData, subData[subNum].qHandle, &stats);

        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 1);
        TEST_ASSERT_EQUAL(stats.ExpiredMsgs, numFirstShort+1);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesWithExpiry, 1);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesInArray, 1);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->earliestExpiryMessages[0].Expiry, longExpiry[0]);
    }

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, numberOfSubs);


    //Put 50 message and check all expiry leaving just the original message
    //(we are testing putting more than the pointers we keep in the per
    //queue data for quick reference)
    int numLotsShort=50;
    int lotsShortExpiries[numLotsShort];

    for (uint32_t i = 0; i < numLotsShort; i++)
    {
        lotsShortExpiries[i] = (ism_common_nowExpire() - 1);
    }
    sendMessagesWithExpiry( topicString, numLotsShort, lotsShortExpiries);

    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        ieme_reapQExpiredMessages(pThreadData, (ismEngine_Queue_t *)(subData[subNum].qHandle));

        ismEngine_QueueStatistics_t stats = {0};
        ieq_getStats(pThreadData, subData[subNum].qHandle, &stats);
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 1);
        TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, numLotsShort+1);
        TEST_ASSERT_EQUAL(stats.ExpiredMsgs, numFirstShort+1+numLotsShort);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesWithExpiry, 1);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesInArray, 1);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->earliestExpiryMessages[0].Expiry, longExpiry[0]);
    }

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, numberOfSubs);


    //Put various expiries in a set order and check sorted per queue data
    uint32_t baseTime = ism_common_nowExpire()+10000;
    int numMixedExpiries=12;
    int mixedExpiries[12]  = { baseTime+600,  baseTime+1200,  baseTime+400,  baseTime+800,
                               baseTime+2200, baseTime,       baseTime+1600, baseTime+1000,
                               baseTime+200,  baseTime+1400,  baseTime+1800, baseTime+2000 };
    sendMessagesWithExpiry( topicString, numMixedExpiries, mixedExpiries);

    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        ismEngine_QueueStatistics_t stats = {0};
        ieq_getStats(pThreadData, subData[subNum].qHandle, &stats);
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, numMixedExpiries+1); //original plus ones we've just put
        TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, numLotsShort+1); //Unchanged since last big batch
        TEST_ASSERT_EQUAL(stats.ExpiredMsgs, numFirstShort+1+numLotsShort); //Unchanged
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesWithExpiry, numMixedExpiries+1);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesInArray, NUM_EARLIEST_MESSAGES);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->earliestExpiryMessages[0].Expiry, baseTime);
        for( uint32_t i=1; i < subData[subNum].pQExpiryData->messagesInArray; i++)
        {
            TEST_ASSERT_EQUAL( subData[subNum].pQExpiryData->earliestExpiryMessages[i].Expiry
                             , 200 + subData[subNum].pQExpiryData->earliestExpiryMessages[i-1].Expiry);
        }
    }

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, numberOfSubs*(numMixedExpiries+1));

    //Get 2 messages, the original message with the long expiry and first of the mixed expiry messages we've
    //just put
    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        getMessageCheckExpiry(&(subData[subNum]), longExpiry[0]);
        getMessageCheckExpiry(&(subData[subNum]), mixedExpiries[0]);
    }

    //Should have all but one of the latest batch of messages on the queue, and the removal of that one
    //will have reduced the messages in the expiry array by 1.
    //Work out how the expiry array should look (including the extra entry that won't yet be in the array)
    uint32_t expectedExpiries[NUM_EARLIEST_MESSAGES];
    expectedExpiries[0] = baseTime;

    for( uint32_t i=1; i < NUM_EARLIEST_MESSAGES; i++)
    {
        expectedExpiries[i] = expectedExpiries[i-1]+200;

        if (expectedExpiries[i] == mixedExpiries[0])
        {
            //Expiry of message removed from the queue
            expectedExpiries[i] += 200;
        }
    }

    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        ismEngine_QueueStatistics_t stats = {0};

        ieq_getStats(pThreadData, subData[subNum].qHandle, &stats);
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, numMixedExpiries-1);
        TEST_ASSERT_EQUAL(stats.ExpiredMsgs, numFirstShort+1+numLotsShort); //Unchanged
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesWithExpiry, stats.BufferedMsgs);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesInArray, NUM_EARLIEST_MESSAGES-1);

        for( uint32_t i=0; i < subData[subNum].pQExpiryData->messagesInArray; i++)
        {
            TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->earliestExpiryMessages[i].Expiry, expectedExpiries[i]);
        }

        //Reap... won't do anything as can see earliest message isn't due to expire
        ieme_reapQExpiredMessages(pThreadData, (ismEngine_Queue_t *)(subData[subNum].qHandle));
        ieq_getStats(pThreadData, subData[subNum].qHandle, &stats);
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, numMixedExpiries-1);
        TEST_ASSERT_EQUAL(stats.ExpiredMsgs, numFirstShort+1+numLotsShort); //Unchanged
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesWithExpiry, stats.BufferedMsgs);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesInArray, NUM_EARLIEST_MESSAGES-1);
    }

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, numberOfSubs*(numMixedExpiries-1));

    //Put Messages with a very long expiry
    int numLotsLong=50;
    int lotsLongExpiries[numLotsLong];

    for (uint32_t i = 0; i < numLotsLong; i++)
    {
        lotsLongExpiries[i] = (ism_common_nowExpire() + 50000);
    }
    sendMessagesWithExpiry( topicString, numLotsLong, lotsLongExpiries);

    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        ismEngine_QueueStatistics_t stats = {0};

        ieq_getStats(pThreadData, subData[subNum].qHandle, &stats);
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, numMixedExpiries-1+numLotsLong);
        TEST_ASSERT_EQUAL(stats.ExpiredMsgs, numFirstShort+1+numLotsShort); //Unchanged
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesWithExpiry, stats.BufferedMsgs);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesInArray, NUM_EARLIEST_MESSAGES-1);

        //Get enough messages to clear the per-queue expiry data
        //(i.e. get remaining mixed expiry msgs)
        for (uint32_t i = 1; i < numMixedExpiries; i++)
        {
            getMessageCheckExpiry(&(subData[subNum]), mixedExpiries[i]);
        }
        ieq_getStats(pThreadData, subData[subNum].qHandle, &stats);
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, numLotsLong);
        TEST_ASSERT_EQUAL(stats.ExpiredMsgs, numFirstShort+1+numLotsShort); //Unchanged
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesWithExpiry, stats.BufferedMsgs);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesInArray, 0);

        //Reap and check we have repopulated the expiry array
        ieme_reapQExpiredMessages(pThreadData, (ismEngine_Queue_t *)(subData[subNum].qHandle));
        ieq_getStats(pThreadData, subData[subNum].qHandle, &stats);
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, numLotsLong);
        TEST_ASSERT_EQUAL(stats.ExpiredMsgs, numFirstShort+1+numLotsShort); //Unchanged
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesWithExpiry, stats.BufferedMsgs);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesInArray,
                   (stats.BufferedMsgs > NUM_EARLIEST_MESSAGES)
                               ?  NUM_EARLIEST_MESSAGES
                               :  stats.BufferedMsgs);

        //After a full expiry reap...
        //Get a message and check that expiry array is reduced...
        getMessageCheckExpiry(&(subData[subNum]), lotsLongExpiries[0]);

        ieq_getStats(pThreadData, subData[subNum].qHandle, &stats);
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, numLotsLong-1);
        TEST_ASSERT_EQUAL(stats.ExpiredMsgs, numFirstShort+1+numLotsShort); //Unchanged
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesWithExpiry, stats.BufferedMsgs);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesInArray,
                   ( (NUM_EARLIEST_MESSAGES - 1) < stats.BufferedMsgs)
                               ?  NUM_EARLIEST_MESSAGES -1
                               :  stats.BufferedMsgs);
        TEST_ASSERT_EQUAL( subData[subNum].pQExpiryData->earliestExpiryMessages[0].Expiry
                         , lotsLongExpiries[1]);

        rc = test_destroyClientAndSession(subData[subNum].hClient, subData[subNum].hSession, false);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, 0);
}

void test_checkOnQFull(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;

    //Set  options for each subData structure (remaining elements initialised to 0)
    test_SubscriptionData_t subData[] = { {   ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE
                                            , ismENGINE_CONSUMER_OPTION_NONE},
                                          {   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                            ,   ismENGINE_CONSUMER_OPTION_ACK
                                              | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID},
                                          {   ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE
                                            ,   ismENGINE_CONSUMER_OPTION_ACK },
                                          {ismENGINE_SUBSCRIPTION_OPTION_NONE} };
    uint32_t numberOfSubs=0; //We'll set this to the number of subs in above array in a sec...
    iemnMessagingStatistics_t overallStats;
    char *topicString = "/test/basic/checkQFull";

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    //Stop the expiry thread interfering in this test
    ismEngine_serverGlobal.msgExpiryControl->reaperEndRequested = true;
    ieme_wakeMessageExpiryReaper(pThreadData);

    //Ensure the maxdepth of our queues is not too large:
    uint64_t queueMaxDepth=50;
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = queueMaxDepth;

    //Find out the number of messages that have been expired before the start of this test...
    iemn_getMessagingStatistics(pThreadData, &overallStats);
    uint64_t  origTotalExpired = overallStats.externalStats.ExpiredMessages;

    /* Create our clients, sessions and consumers for each sub */
    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        char clientId[32];

        sprintf(clientId, "subClient%u", subNum);

        rc = test_createClientAndSession(clientId,
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &(subData[subNum].hClient),
                                         &(subData[subNum].hSession), false);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(subData[subNum].hClient);
        TEST_ASSERT_PTR_NOT_NULL(subData[subNum].hSession);

        test_SubscriptionData_t *pSubData = &(subData[subNum]);
        ismEngine_SubscriptionAttributes_t subAttrs = { subData[subNum].subscriptionOptions };
        rc = ism_engine_createConsumer( subData[subNum].hSession
                                      , ismDESTINATION_TOPIC
                                      , topicString
                                      , &subAttrs
                                      , NULL // Owning client same as session client
                                      , &pSubData
                                      , sizeof(pSubData)
                                      , checkExpiryCB
                                      , NULL
                                      , subData[subNum].consumerOptions
                                      , &(subData[subNum].hConsumer)
                                      , NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        subData[subNum].qHandle = ((ismEngine_Consumer_t *)(subData[subNum].hConsumer))->queueHandle;

        numberOfSubs++;
    }

    //Publish a few expired messages and check they don't expire as we don't have the reaper running and we
    //haven't filled the queue and they haven't been delivered
    int numFirstMsgs = 3;
    int firstMsgExpiries[numFirstMsgs];

    for (uint32_t i = 0; i < numFirstMsgs; i++)
    {
        firstMsgExpiries[i] = (ism_common_nowExpire() - 1);
    }
    sendMessagesWithExpiry( topicString, numFirstMsgs, firstMsgExpiries);

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, numberOfSubs*numFirstMsgs);
    TEST_ASSERT_EQUAL(overallStats.externalStats.ExpiredMessages, origTotalExpired);

    //Fill the queue with expired messages and check they get expired
    int numForFill = queueMaxDepth - numFirstMsgs;
    int fillShortExpiries[numForFill];

    for (uint32_t i = 0; i < numForFill; i++)
    {
        fillShortExpiries[i] = (ism_common_nowExpire() - 1);
    }
    sendMessagesWithExpiry( topicString, numForFill, fillShortExpiries);

    uint64_t totalPut = numFirstMsgs + numForFill;

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, 0);
    TEST_ASSERT_EQUAL(overallStats.externalStats.ExpiredMessages, origTotalExpired+(numberOfSubs*totalPut));

    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        ismEngine_QueueStatistics_t stats = {0};
        ieq_getStats(pThreadData, subData[subNum].qHandle, &stats);
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
        TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, totalPut);
        TEST_ASSERT_EQUAL(stats.ExpiredMsgs, totalPut);
        ismEngine_Queue_t *q = (ismEngine_Queue_t *)(subData[subNum].qHandle);
        subData[subNum].pQExpiryData = (iemeQueueExpiryData_t *)(q->QExpiryData);
        TEST_ASSERT_PTR_NOT_NULL(subData[subNum].pQExpiryData);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesWithExpiry, 0);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesInArray, 0);
    }

    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        rc = test_destroyClientAndSession(subData[subNum].hClient, subData[subNum].hSession, false);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, 0);
}


void test_checkOnDelivery(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;

    //Set  options for each subData structure (remaining elements initialised to 0)
    test_SubscriptionData_t subData[] = { {   ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE
                                            , ismENGINE_CONSUMER_OPTION_NONE},
                                          {   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                            ,   ismENGINE_CONSUMER_OPTION_ACK
                                              | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID},
                                          {   ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE
                                            ,   ismENGINE_CONSUMER_OPTION_ACK },
                                          {ismENGINE_SUBSCRIPTION_OPTION_NONE} };
    uint32_t numberOfSubs=0; //We'll set this to the number of subs in above array in a sec...
    iemnMessagingStatistics_t overallStats;
    char *topicString = "/test/basic/checkDelivery";

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    //Stop the expiry thread interfering in this test
    ismEngine_serverGlobal.msgExpiryControl->reaperEndRequested = true;
    ieme_wakeMessageExpiryReaper(pThreadData);

    //Ensure the maxdepth of our queues is not too large:
    uint64_t queueMaxDepth=50;
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = queueMaxDepth;

    //Find out the number of messages that have been expired before the start of this test...
    iemn_getMessagingStatistics(pThreadData, &overallStats);
    uint64_t  origTotalExpired = overallStats.externalStats.ExpiredMessages;

    /* Create our clients, sessions and consumers for each sub */
    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        char clientId[32];

        sprintf(clientId, "subClient%u", subNum);

        rc = test_createClientAndSession(clientId,
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &(subData[subNum].hClient),
                                         &(subData[subNum].hSession), false);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(subData[subNum].hClient);
        TEST_ASSERT_PTR_NOT_NULL(subData[subNum].hSession);

        test_SubscriptionData_t *pSubData = &(subData[subNum]);
        ismEngine_SubscriptionAttributes_t subAttrs = { subData[subNum].subscriptionOptions };

        rc = ism_engine_createConsumer( subData[subNum].hSession
                                      , ismDESTINATION_TOPIC
                                      , topicString
                                      , &subAttrs
                                      , NULL // Owning client same as session client
                                      , &pSubData
                                      , sizeof(pSubData)
                                      , neverCalledCB
                                      , NULL
                                      , subData[subNum].consumerOptions
                                      , &(subData[subNum].hConsumer)
                                      , NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        subData[subNum].qHandle = ((ismEngine_Consumer_t *)(subData[subNum].hConsumer))->queueHandle;

        numberOfSubs++;
    }

    //Publish a few expired messages and check they don't expire as we don't have the reaper running and we
    //haven't filled the queue and they haven't been delivered
    int numFirstMsgs = 3;
    int firstMsgExpiries[numFirstMsgs];

    for (uint32_t i = 0; i < numFirstMsgs; i++)
    {
        firstMsgExpiries[i] = (ism_common_nowExpire() - 1);
    }
    sendMessagesWithExpiry( topicString, numFirstMsgs, firstMsgExpiries);

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, numberOfSubs*numFirstMsgs);
    TEST_ASSERT_EQUAL(overallStats.externalStats.ExpiredMessages, origTotalExpired);

    //Try and get messages from each subscription, queue should figure out all messages now expired...
    //(if a message is actually delivered to the callback, it will abort the test..)
    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        rc = ism_engine_startMessageDelivery( subData[subNum].hSession
                                            , ismENGINE_START_DELIVERY_OPTION_NONE
                                            , NULL,0,NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, 0);
    TEST_ASSERT_EQUAL(overallStats.externalStats.ExpiredMessages, origTotalExpired+(numberOfSubs*numFirstMsgs));

    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        ismEngine_QueueStatistics_t stats = {0};
        ieq_getStats(pThreadData, subData[subNum].qHandle, &stats);
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
        TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, numFirstMsgs);
        TEST_ASSERT_EQUAL(stats.ExpiredMsgs, numFirstMsgs);
        ismEngine_Queue_t *q = (ismEngine_Queue_t *)(subData[subNum].qHandle);
        subData[subNum].pQExpiryData = (iemeQueueExpiryData_t *)(q->QExpiryData);
        TEST_ASSERT_PTR_NOT_NULL(subData[subNum].pQExpiryData);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesWithExpiry, 0);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesInArray, 0);
    }

    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        rc = test_destroyClientAndSession(subData[subNum].hClient, subData[subNum].hSession, false);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, 0);
}

//Check we update the expiry stats even if we don't notice the message expired and
//protocol has to tell us
void test_checkOnProtocol(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;

    //Set  options for each subData structure (remaining elements initialised to 0)
    test_SubscriptionData_t subData[] = { {   ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE
                                            , ismENGINE_CONSUMER_OPTION_NONE},
                                          {   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                            ,   ismENGINE_CONSUMER_OPTION_ACK
                                              | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID},
                                          {   ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE
                                            ,   ismENGINE_CONSUMER_OPTION_ACK },
                                          {ismENGINE_SUBSCRIPTION_OPTION_NONE} };

    uint32_t numberOfSubs=0;            //We'll set this to the number of subs in above array in a sec...
    uint32_t numberOfAckingConsumers=0; //We'll set this to the number of subs which ack in above array in a sec...
    iemnMessagingStatistics_t overallStats;
    char *topicString = "/test/basic/checkProtocol";

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    //Stop the expiry thread interfering in this test
    ismEngine_serverGlobal.msgExpiryControl->reaperEndRequested = true;
    ieme_wakeMessageExpiryReaper(pThreadData);

    //Find out the number of messages that have been expired before the start of this test...
    iemn_getMessagingStatistics(pThreadData, &overallStats);
    uint64_t  origTotalExpired = overallStats.externalStats.ExpiredMessages;

    /* Create our clients, sessions and consumers for each sub */
    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        char clientId[32];

        sprintf(clientId, "subClient%u", subNum);

        rc = test_createClientAndSession(clientId,
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &(subData[subNum].hClient),
                                         &(subData[subNum].hSession), false);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(subData[subNum].hClient);
        TEST_ASSERT_PTR_NOT_NULL(subData[subNum].hSession);

        test_SubscriptionData_t *pSubData = &(subData[subNum]);
        ismEngine_SubscriptionAttributes_t subAttrs = { subData[subNum].subscriptionOptions };

        rc = ism_engine_createConsumer( subData[subNum].hSession
                                      , ismDESTINATION_TOPIC
                                      , topicString
                                      , &subAttrs
                                      , NULL // Owning client same as session client
                                      , &pSubData
                                      , sizeof(pSubData)
                                      , claimExpiredCB
                                      , NULL
                                      , subData[subNum].consumerOptions
                                      , &(subData[subNum].hConsumer)
                                      , NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        subData[subNum].qHandle = ((ismEngine_Consumer_t *)(subData[subNum].hConsumer))->queueHandle;

        numberOfSubs++;
        if(subData[subNum].consumerOptions & ismENGINE_CONSUMER_OPTION_ACK)numberOfAckingConsumers++;
    }

    //Publish a few not-expired messages and check they don't expire as we don't have the reaper running and we
    //haven't filled the queue and they haven't been delivered
    int numFirstMsgs = 3;
    int firstMsgExpiries[numFirstMsgs];

    for (uint32_t i = 0; i < numFirstMsgs; i++)
    {
        firstMsgExpiries[i] = (ism_common_nowExpire() + 10000);
    }
    sendMessagesWithExpiry( topicString, numFirstMsgs, firstMsgExpiries);

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, numberOfSubs*numFirstMsgs);
    TEST_ASSERT_EQUAL(overallStats.externalStats.ExpiredMessages, origTotalExpired);

    uint64_t msgsFalselyClaimedExpired = 0;

    //Turn on delivery for each message. The callback will lie for each message and claim it expired for
    //any message it got a handle (i.e. for any consumer that is acking)
    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        rc = ism_engine_startMessageDelivery( subData[subNum].hSession
                                            , ismENGINE_START_DELIVERY_OPTION_NONE
                                            , NULL,0,NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        //Count how many messages fake protocol claims to have expired...
        msgsFalselyClaimedExpired += subData[subNum].numMessagesProtocolClaimExpired;
    }

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, 0);
    TEST_ASSERT_EQUAL(overallStats.externalStats.ExpiredMessages, origTotalExpired+msgsFalselyClaimedExpired);

    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        ismEngine_QueueStatistics_t stats = {0};
        ieq_getStats(pThreadData, subData[subNum].qHandle, &stats);
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
        TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, numFirstMsgs);
        if (subData[subNum].consumerOptions & ismENGINE_CONSUMER_OPTION_ACK)
        {
            TEST_ASSERT_EQUAL(stats.ExpiredMsgs, subData[subNum].numMessagesProtocolClaimExpired);
        }
        else
        {
            TEST_ASSERT_EQUAL(stats.ExpiredMsgs, 0);
        }
        ismEngine_Queue_t *q = (ismEngine_Queue_t *)(subData[subNum].qHandle);
        subData[subNum].pQExpiryData = (iemeQueueExpiryData_t *)(q->QExpiryData);
        TEST_ASSERT_PTR_NOT_NULL(subData[subNum].pQExpiryData);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesWithExpiry, 0);
        TEST_ASSERT_EQUAL(subData[subNum].pQExpiryData->messagesInArray, 0);
    }

    for (uint32_t subNum = 0; subData[subNum].subscriptionOptions != ismENGINE_SUBSCRIPTION_OPTION_NONE; subNum++)
    {
        rc = test_destroyClientAndSession(subData[subNum].hClient, subData[subNum].hSession, false);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, 0);
}


void lockMessage( ieutThreadData_t *pThreadData
                , ielmLockScopeHandle_t hLockScope
                , iemqQueue_t *Q
                , uint64_t orderId
                , ielmLockRequestHandle_t *phLockRequest)
{
   int32_t rc;
   ielmLockName_t LockName = { .Msg = { ielmLOCK_TYPE_MESSAGE, Q->qId, orderId } };

   rc = ielm_requestLock(pThreadData, hLockScope, &LockName, ielmLOCK_MODE_X,
                         ielmLOCK_DURATION_REQUEST, phLockRequest);
   TEST_ASSERT_EQUAL(rc, 0);
}

//Check that if an expiry scan happens whilst a message is locked, the message is not
//harmed un
void test_checkWhilstMsgLocked(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;

    //Set  options for  subData structure (remaining elements initialised to 0)
    test_SubscriptionData_t subData =  {   ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE
                                            ,   ismENGINE_CONSUMER_OPTION_ACK };

    iemnMessagingStatistics_t overallStats;
    char *topicString = "/test/basic/checkWhilstMsgLocked";

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    //Stop the expiry thread interfering in this test
    ismEngine_serverGlobal.msgExpiryControl->reaperEndRequested = true;
    ieme_wakeMessageExpiryReaper(pThreadData);

    //Find out the number of messages that have been expired before the start of this test...
    iemn_getMessagingStatistics(pThreadData, &overallStats);
    uint64_t  origTotalExpired = overallStats.externalStats.ExpiredMessages;

    /* Create our clients, sessions and consumers for each sub */

    char clientId[32];

    sprintf(clientId, "subClient");

    rc = test_createClientAndSession(clientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &(subData.hClient),
                                     &(subData.hSession), false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subData.hClient);
    TEST_ASSERT_PTR_NOT_NULL(subData.hSession);

    test_SubscriptionData_t *pSubData = &subData;
    ismEngine_SubscriptionAttributes_t subAttrs = { subData.subscriptionOptions };

    rc = ism_engine_createConsumer( subData.hSession
                                  , ismDESTINATION_TOPIC
                                  , topicString
                                  , &subAttrs
                                  , NULL // Owning client same as session client
                                  , &pSubData
                                  , sizeof(pSubData)
                                  , claimExpiredCB
                                  , NULL
                                  , subData.consumerOptions
                                  , &(subData.hConsumer)
                                  , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subData.qHandle = ((ismEngine_Consumer_t *)(subData.hConsumer))->queueHandle;

    //Publish an expired msg and check it doesn't expire as we don't have the reaper running and we
    //haven't filled the queue and they haven't been delivered
    int numMsgs = 1;
    int msgExpiries[numMsgs];

    for (uint32_t i = 0; i < numMsgs; i++)
    {
        msgExpiries[i] = (ism_common_nowExpire() - 1);
    }
    sendMessagesWithExpiry( topicString, numMsgs, msgExpiries);

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, numMsgs);
    TEST_ASSERT_EQUAL(overallStats.externalStats.ExpiredMessages, origTotalExpired);

    //Lock the message in the lock manager...
    ielmLockScopeHandle_t lockScope;
    ielmLockRequestHandle_t hLockrequest;

    rc = ielm_allocateLockScope(pThreadData, ielmLOCK_SCOPE_NO_COMMIT,
                                                      NULL, &lockScope);
    TEST_ASSERT_EQUAL(rc, 0);
    lockMessage( pThreadData
               , lockScope
               , (iemqQueue_t *)subData.qHandle
               , 1
               , &hLockrequest);

    //Run the reaper....check nothing expires
    ieme_reapQExpiredMessages(pThreadData, (ismEngine_Queue_t *)(subData.qHandle));

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, numMsgs);
    TEST_ASSERT_EQUAL(overallStats.externalStats.ExpiredMessages, origTotalExpired);

    //Unlock the message and rerun reaper
    ielm_releaseLock(pThreadData, lockScope, hLockrequest);
    ielm_freeLockScope(pThreadData, &lockScope);
    ieme_reapQExpiredMessages(pThreadData, (ismEngine_Queue_t *)(subData.qHandle));


    //Check that now that the message has been expired
    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, 0);
    TEST_ASSERT_EQUAL(overallStats.externalStats.ExpiredMessages, origTotalExpired+numMsgs);

    rc = test_destroyClientAndSession(subData.hClient, subData.hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    iemn_getMessagingStatistics(pThreadData, &overallStats);
    TEST_ASSERT_EQUAL(overallStats.BufferedMessagesWithExpirySet, 0);
}



CU_TestInfo ISM_NamedQueues_CUnit_test_Deterministic[] =
{
    { "basicCheckExpiry",      test_basicCheckMessagesExpiry },
    { "checkOnQueueFull",      test_checkOnQFull },
    { "checkOnDelivery",       test_checkOnDelivery},
    { "checkOnProtocol",       test_checkOnProtocol},
    { "checkWhilstMsgLocked",  test_checkWhilstMsgLocked},
    CU_TEST_INFO_NULL
};

int initQueueMsgExpiry(void)
{
    return test_engineInit(true, true,
                           true, // Disable Auto creation of named queues
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termQueueMsgExpiry(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_queueMsgExpiry_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("Deterministic", initQueueMsgExpiry, termQueueMsgExpiry, ISM_NamedQueues_CUnit_test_Deterministic),
    CU_SUITE_INFO_NULL
};

int setup_CU_registry(int argc, char **argv, CU_SuiteInfo *allSuites)
{
    CU_SuiteInfo *tempSuites = NULL;

    int retval = 0;

    if (argc > 1)
    {
        int suites = 0;

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
            else if (!strcasecmp(argv[i], "verbose"))
            {
                CU_basic_set_mode(CU_BRM_VERBOSE);
            }
            else if (!strcasecmp(argv[i], "silent"))
            {
                CU_basic_set_mode(CU_BRM_SILENT);
            }
            else
            {
                bool suitefound = false;
                int index = atoi(argv[i]);
                int totalSuites = 0;

                CU_SuiteInfo *curSuite = allSuites;

                while(curSuite->pTests)
                {
                    if (!strcasecmp(curSuite->pName, argv[i]))
                    {
                        suitefound = true;
                        break;
                    }

                    totalSuites++;
                    curSuite++;
                }

                if (!suitefound)
                {
                    if (index > 0 && index <= totalSuites)
                    {
                        curSuite = &allSuites[index-1];
                        suitefound = true;
                    }
                }

                if (suitefound)
                {
                    tempSuites = realloc(tempSuites, sizeof(CU_SuiteInfo) * (suites+2));
                    memcpy(&tempSuites[suites++], curSuite, sizeof(CU_SuiteInfo));
                    memset(&tempSuites[suites], 0, sizeof(CU_SuiteInfo));
                }
                else
                {
                    printf("Invalid test suite '%s' specified, the following are valid:\n\n", argv[i]);

                    index=1;

                    curSuite = allSuites;

                    while(curSuite->pTests)
                    {
                        printf(" %2d : %s\n", index++, curSuite->pName);
                        curSuite++;
                    }

                    printf("\n");

                    retval = 99;
                    break;
                }
            }
        }
    }

    if (retval == 0)
    {
        if (tempSuites)
        {
            CU_register_suites(tempSuites);
        }
        else
        {
            CU_register_suites(allSuites);
        }
    }

    if (tempSuites) free(tempSuites);

    return retval;
}

int main(int argc, char *argv[])
{
    int trclvl = 0;
    int retval = 0;
    int testLogLevel = 2;

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_queueMsgExpiry_CUnit_allsuites);

    if (retval == 0)
    {
        CU_basic_run_tests();

        CU_RunSummary * CU_pRunSummary_Final;
        CU_pRunSummary_Final = CU_get_run_summary();

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

    test_processTerm(retval == 0);

mod_exit:

    return retval;
}
