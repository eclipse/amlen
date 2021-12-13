/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

// Some generic common simple way of pubbing/subbing/ackking
// messages, many tests will still need custom code but these
// function provide some common ways to assist in test creation

#include "engine.h"
#include "engineInternal.h"

#include "test_utils_client.h"
#include "test_utils_sync.h"
#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_pubsuback.h"
#include "test_utils_options.h"
#include "test_utils_initterm.h"

void test_pubMessages( const char *topicString
                     , uint8_t persistence
                     , uint8_t reliability
                     , uint32_t numMsgs)
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
                                persistence,
                                reliability,
                                ismMESSAGE_FLAGS_NONE,
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

        if (rc != ISMRC_AsyncCompletion)
        {
            TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Allow for information RCs
            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        }
    }
    test_waitForActionsTimeOut(pActionsRemaining, 20);
    test_destroyClientAndSession(hClient, hSession, false);
}



static void consumeCompleted(int32_t rc, void *handle, void *context)
{
    testMarkMsgsContext_t *markContext = *(testMarkMsgsContext_t **)context;

    if (rc == OK)
    {
        __sync_fetch_and_add(&(markContext->consCompleted), 1);
    }
}

static void receiveCompleted(int32_t rc, void *handle, void *context)
{
    testMarkMsgsContext_t *markContext = *(testMarkMsgsContext_t **)context;

    if (rc == OK)
    {
        __sync_fetch_and_add(&(markContext->recvCompleted), 1);
    }
}

bool test_markMessagesCallback(
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
        ismEngine_DelivererContext_t *  _delivererContext )
{
    testMarkMsgsContext_t *context = *(testMarkMsgsContext_t **)pConsumerContext;
    bool considerMsg = true;

    test_log(testLOGLEVEL_VERBOSE, "Client %s: msg oId %lu arrived in state %u",
               hConsumer->pSession->pClient->pClientId, pMsgDetails->OrderId, state);

    uint64_t numDelivered = __sync_add_and_fetch(&(context->numDelivered), 1);


    if (context->numSkippedBeforeAcks < context->numToSkipBeforeAcks)
    {
        context->numSkippedBeforeAcks++;
        considerMsg = false; //We skipped this message
    }

    if (considerMsg && state == ismMESSAGE_STATE_DELIVERED)
    {

        if (context->consStarted < context->numToMarkConsumed)
        {
            __sync_fetch_and_add(&(context->consStarted), 1);

            DEBUG_ONLY int32_t rc;
            rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                   NULL,
                                                   hDelivery,
                                                   ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                   pConsumerContext, sizeof (testGetMsgContext_t *),
                                                   consumeCompleted);
            TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion, ("rc was (%d)", rc));

            if (rc == OK)
            {
                __sync_fetch_and_add(&(context->consCompleted), 1);
            }
        }
        else if (context->recvStarted < context->numToMarkRecv)
        {
            __sync_fetch_and_add(&(context->recvStarted), 1);

            DEBUG_ONLY int32_t rc;
            rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                   NULL,
                                                   hDelivery,
                                                   ismENGINE_CONFIRM_OPTION_RECEIVED,
                                                   pConsumerContext, sizeof (testGetMsgContext_t *),
                                                   receiveCompleted);
            TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion, ("rc was (%d)", rc));

            if (rc == OK)
            {
                __sync_fetch_and_add(&(context->recvCompleted), 1);
            }
        }
        else if (context->numSkippedBeforeStores < context->numToSkipBeforeStore)
        {
            context->numSkippedBeforeStores++;
            considerMsg = false; //We skipped this message
        }
        else if (context->numCallerHandlesStored < context->numToStoreForCaller)
        {
            test_log(testLOGLEVEL_VERBOSE, "Client %s: storing hDelivery for caller for msg oId %lu",
                       hConsumer->pSession->pClient->pClientId, pMsgDetails->OrderId);
            context->pCallerHandles[context->numCallerHandlesStored] = hDelivery;
            context->numCallerHandlesStored++;
        }
        else if (context->numNackHandlesStored < context->numToStoreForNack)
        {
            test_log(testLOGLEVEL_VERBOSE, "Client %s: storing hDelivery for Nack for msg oId %lu",
                       hConsumer->pSession->pClient->pClientId, pMsgDetails->OrderId);
            context->pNackHandles[context->numNackHandlesStored] = hDelivery;
            context->numNackHandlesStored++;
        }
    }
    else
    {
        __sync_fetch_and_add(&(context->unmarkableMsgs), 1);
    }

    ism_engine_releaseMessage(hMessage);

    return (numDelivered < context->totalNumToHaveDelivered);
}


static void ackCompleted(int32_t rc, void *handle, void *context)
{
    testGetMsgContext_t *getMsgContext = *(testGetMsgContext_t **)context;

    if (rc == OK)
    {
        __sync_fetch_and_add(&(getMsgContext->acksCompleted), 1);
    }
}

static bool countMessagesCallback(
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
        ismEngine_DelivererContext_t *  _delivererContext )
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


static void reopenSubsCallback(
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

void test_connectAndGetMsgs( const char *clientId
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

void test_receive( testGetMsgContext_t *pContext
                 , const char *subName
                 , uint32_t subOptions
                 , uint64_t waitForNumMsgs
                 , uint64_t waitForNumMilliSeconds)
{
    int32_t rc;

    ismEngine_ConsumerHandle_t hConsumer;

    rc = ism_engine_createConsumer(
            pContext->hSession,
            ismDESTINATION_SUBSCRIPTION,
            subName,
            NULL,
            NULL, //We're not told what to put here
            &pContext,
            sizeof(testGetMsgContext_t *),
            countMessagesCallback,
            NULL,
            pContext->consumerOptions,
            &hConsumer,
            NULL,
            0,
            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    uint64_t waits = 0;
    while ( ((pContext->pMsgCounts->msgsArrived < waitForNumMsgs)
            || (pContext->acksStarted > pContext->acksCompleted))
            && (waits <= waitForNumMilliSeconds))
    {
        //Sleep for a millisecond
        ismEngine_FullMemoryBarrier();
        usleep(1000);
        waits++;
    }

    //If we've got the right number of seconds, wait a mo and see if extra turn up
    if (pContext->pMsgCounts->msgsArrived == waitForNumMsgs)
    {
        usleep(1000);
    }

    rc = sync_ism_engine_destroyConsumer(hConsumer);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_connectReuseReceive( const char *clientId
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
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    int32_t rc;

    rc = test_createClientAndSession(clientId,
                                     NULL,
                                     clientOptions,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    if (doReuse)
    {
        ismEngine_SubscriptionAttributes_t subAttrs = { subOptions };

        rc = ism_engine_reuseSubscription(hClient,
                          subName,
                          &subAttrs,
                          hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    testGetMsgContext_t ctxt = {0};

    ctxt.hClient              = hClient;
    ctxt.hSession             = hSession;
    ctxt.pMsgCounts           = pMsgDetails;
    ctxt.consumerOptions      = consumerOptions;
    ctxt.overrideConsumerOpts = true;
    ctxt.consumeMessages      = consumeMessages;

    test_receive(  &ctxt
                ,  subName
                ,  subOptions
                ,  waitForNumMsgs
                ,  waitForNumMilliSeconds );

    test_destroyClientAndSession(hClient, hSession, false);
}

void test_makeSub( const char *subName
                 , const char *topicString
                 , uint32_t subOptions
                 , ismEngine_ClientStateHandle_t hAttachingClient
                 , ismEngine_ClientStateHandle_t hOwningClient)
{
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;
    int32_t rc;
    ismEngine_SubscriptionAttributes_t subAttrs = { subOptions };

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_createSubscription(hAttachingClient,
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
                && (subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED))
    {
        //Sub already exists.... reuse it
        rc = ism_engine_reuseSubscription(hAttachingClient,
                subName,
                &subAttrs,
                hOwningClient);
        TEST_ASSERT_EQUAL(rc, OK);

        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForActionsTimeOut(pActionsRemaining, 20);
}

void test_markMsgs(
               char *subName
             , ismEngine_SessionHandle_t hSession
             , ismEngine_ClientState_t *hOwningClient
             , uint32_t numToHaveDelivered    //Total messages we have delivered before disabling consumer
             , uint32_t numToSkipBeforeAcks   //Of the delivered messages, how many do we skip before we start acking
             , uint32_t numToCons             //Of the delivered messages, how many to consume
             , uint32_t numToMarkRecv         //Of the delivered messages, how many to mark received
             , uint32_t numToSkipBeforeStore  //Of the delivered messages, after we've done acks how many
                                              //    do we skip before we store handles to give to caller or nack
             , uint32_t numToGiveToCaller     //Of the delivered messages, how many handles do we give to the caller unnacked
             , uint32_t numToNackNotRecv      //Of the delivered messages, how many to nack (not received) after delivery stops
             , uint32_t numToNackNotSent      //Of the delivered messages, how many to nack (not sent) after delivery stops
             , ismEngine_DeliveryHandle_t ackHandles[numToGiveToCaller])
{
    int32_t rc;

     ismEngine_ConsumerHandle_t hConsumer = NULL;

     uint64_t numToNack = numToNackNotRecv + numToNackNotSent;
     ismEngine_DeliveryHandle_t nackHandles[numToNack];

     if (numToNack > 0)
     {
         memset(&nackHandles[0], 0, numToNack*sizeof(nackHandles[0]));
     }

     testMarkMsgsContext_t markContext = {0};
     testMarkMsgsContext_t *pMarkContext = &markContext;


     markContext.hSession                = hSession;
     markContext.totalNumToHaveDelivered = numToHaveDelivered;
     markContext.numToSkipBeforeAcks     = numToSkipBeforeAcks;
     markContext.numToMarkConsumed       = numToCons;
     markContext.numToMarkRecv           = numToMarkRecv;
     markContext.numToSkipBeforeStore    = numToSkipBeforeStore;
     markContext.numToStoreForCaller     = numToGiveToCaller;
     markContext.numToStoreForNack       = numToNack;
     markContext.pNackHandles            = nackHandles;
     markContext.pCallerHandles          = ackHandles;

     //Use short delivery id == mdr
     rc = ism_engine_createConsumer(
             hSession,
             ismDESTINATION_SUBSCRIPTION,
             subName,
             0,
             hOwningClient,
             &pMarkContext,
             sizeof(testMarkMsgsContext_t *),
             test_markMessagesCallback,
             NULL,
             ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
             &hConsumer,
             NULL,
             0,
             NULL);
     TEST_ASSERT_EQUAL(rc, OK);

     uint64_t waits = 0;
     uint64_t waitLimit = 5000+ numToHaveDelivered/3; //Wait upto 5second + 0.3millisecond per msg
     //There can be multiple tests accessing the disk e.g. building or this test so this can be very slow
     if (usingDiskPersistence)
     {
         waitLimit = 10000+(16*numToHaveDelivered); //Wait upto 10second + 16 milliseconds per msg - 50,000 msgs ~12 minutes
     }

     while (    (    (markContext.numDelivered  < numToHaveDelivered)
                  || (markContext.consCompleted < numToCons)
                  || (markContext.recvCompleted < numToMarkRecv))
             && (waits <=waitLimit))
     {
         //Sleep for a millisecond
         ismEngine_FullMemoryBarrier();
         usleep(1000);
         waits++;
     }

     TEST_ASSERT_EQUAL(markContext.numDelivered,           numToHaveDelivered);
     TEST_ASSERT_EQUAL(markContext.consCompleted,          numToCons);
     TEST_ASSERT_EQUAL(markContext.recvCompleted,          numToMarkRecv);
     TEST_ASSERT_EQUAL(markContext.numNackHandlesStored,   numToNack);
     TEST_ASSERT_EQUAL(markContext.numCallerHandlesStored, numToGiveToCaller);

     if (numToNack > 0)
     {
         uint64_t nackHandlePos = 0;

         uint32_t actionsRemaining = 0;
         uint32_t *pActionsRemaining = &actionsRemaining;

         test_incrementActionsRemaining(pActionsRemaining, numToNack);

         for (uint32_t i=0 ; i < numToNackNotRecv; i++ )
         {
             rc = ism_engine_confirmMessageDelivery(hSession,
                                                    NULL,
                                                    nackHandles[nackHandlePos],
                                                    ismENGINE_CONFIRM_OPTION_NOT_RECEIVED,
                                                    &pActionsRemaining, sizeof(pActionsRemaining),
                                                    test_decrementActionsRemaining);
             nackHandlePos++;

             if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
             else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
         }

         for (uint32_t i=0 ; i < numToNackNotSent; i++ )
         {
             rc = ism_engine_confirmMessageDelivery(hSession,
                                                    NULL,
                                                    nackHandles[nackHandlePos],
                                                    ismENGINE_CONFIRM_OPTION_NOT_DELIVERED,
                                                    &pActionsRemaining, sizeof(pActionsRemaining),
                                                    test_decrementActionsRemaining);
             nackHandlePos++;

             if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
             else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
         }

         test_waitForActionsTimeOut(pActionsRemaining, 20);
     }

     sync_ism_engine_destroyConsumer(hConsumer);

}

uint32_t test_getBatchSize(ismEngine_SessionHandle_t hSession)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;

    uint32_t maxInflight = pSession->pClient->maxInflightLimit;

    uint32_t batchSize = maxInflight >> 2;

    if (batchSize == 0)
    {
        batchSize = 1;
    }

    return batchSize;
}
