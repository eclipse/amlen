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
/* Module Name: test_expiringGet.c                                   */
/*                                                                   */
/* Description: Main source file for CUnit test of consumers that    */
/* expire after a certain time                                       */
/*                                                                   */
/*********************************************************************/


#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_client.h"
#include "test_utils_initterm.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

#include "engine.h"
#include "engineInternal.h"

#include <stdbool.h>

typedef struct tag_testBasicGetContext_t {
    volatile uint32_t expectedDeliverCBCalls;
    volatile bool     expectedCompletionCBCall;   //whether the completion callback should be called at all
    int32_t  expectedCompletionCBCallRC; //If it should be called, what should the rc be?
    uint32_t numUnackedMsgs;
    ismEngine_DeliveryHandle_t unackedMessages[10];
} testBasicGetContext_t;

bool basicGetCB(
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
    testBasicGetContext_t *pGetContext = *(testBasicGetContext_t **)pConsumerContext;
    uint32_t dlvySlot = __sync_fetch_and_add(&(pGetContext->numUnackedMsgs), 1);
    pGetContext->unackedMessages[dlvySlot] = hDelivery;

    uint32_t expectedCalls = __sync_fetch_and_sub(&(pGetContext->expectedDeliverCBCalls), 1);
    TEST_ASSERT_CUNIT(expectedCalls> 0,
              ("expectedDeliverCBCalls was ZERO\n"));
    ism_engine_releaseMessage(hMessage);
    return false;
}

void basicGetCompletionCB(int rc, void *handle, void *context)
{
    testBasicGetContext_t *pGetContext = *(testBasicGetContext_t **)context;
    TEST_ASSERT_EQUAL(pGetContext->expectedCompletionCBCall, true);
    TEST_ASSERT_EQUAL(pGetContext->expectedCompletionCBCallRC, rc);

    pGetContext->expectedCompletionCBCall = false;
}

//Put 2 messages to a sub, one non-persistent and one persistent (second goes async)
//Do 2  gets (no acks) and have them deliver
//Do a third get that times out
//Do a 4th get then nack one of the earlier messages so the  get completes async
//Do a 5th get with a long timeout and destroy client and check getcallback doesn't fire
void test_basicGet(void)
{
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    char *getClientId="BasicGetClient";
    char *topicString = "/topic/basicGet";
    char *subName     = "test_basicGets";
    test_log(testLOGLEVEL_TESTNAME, "Starting %s\n",__func__);


    /* Create our clients and sessions */
    rc = test_createClientAndSession(getClientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    //Create the subscription we do it shared so that we don't have to ack the msg from
    //the previous consumer before asking for another.

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                                  | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE
                                                  | ismENGINE_SUBSCRIPTION_OPTION_SHARED };

    rc = sync_ism_engine_createSubscription(
            hClient,
            subName,
            NULL,
            ismDESTINATION_TOPIC,
            topicString,
            &subAttrs,
            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Put a non-persistent message on the topic
    ismEngine_MessageHandle_t hMessage;

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, topicString,
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
            ismDESTINATION_TOPIC,
            topicString,
            NULL,
            hMessage);

    TEST_ASSERT_EQUAL(rc, OK);

    //Get message...
    testBasicGetContext_t getContext = {0};
    getContext.expectedDeliverCBCalls = 1;
    testBasicGetContext_t *pGetContext = &getContext;

    //1st message we get is non-persistent... shouldn't go async.
    rc = ism_engine_getMessage(
                         hSession,
                         ismDESTINATION_SUBSCRIPTION,
                         subName,
                         NULL,     ///< Only used for ismDESTINATION_TOPIC
                         300,
                         NULL,     ///< Which client owns the subscription
                         &pGetContext, sizeof(pGetContext),
                         basicGetCB,
                         NULL,
                         ismENGINE_CONSUMER_OPTION_ACK,
                         &pGetContext, sizeof(pGetContext),
                         basicGetCompletionCB);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(getContext.expectedDeliverCBCalls, 0); //Check msg was delivered

    /* Put another message - this time persistent. If we're using asyc IO, getting the
     * message will go async */

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT ,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, topicString,
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
            ismDESTINATION_TOPIC,
            topicString,
            NULL,
            hMessage);

    TEST_ASSERT_EQUAL(rc, OK);

    getContext.expectedDeliverCBCalls = 1;
    getContext.expectedCompletionCBCall = true;

    rc = ism_engine_getMessage(
                             hSession,
                             ismDESTINATION_SUBSCRIPTION,
                             subName,
                             NULL,     ///< Only used for ismDESTINATION_TOPIC
                             300,
                             NULL,     ///< Which client owns the subscription
                             &pGetContext, sizeof(pGetContext),
                             basicGetCB,
                             NULL,
                             ismENGINE_CONSUMER_OPTION_ACK,
                             &pGetContext, sizeof(pGetContext),
                             basicGetCompletionCB);

     if (rc == OK)
     {
         //We obviously weren't using async IO
         TEST_ASSERT_EQUAL(getContext.expectedDeliverCBCalls, 0); //Check msg was delivered
         //Check that the completion callback wasn't called:
         TEST_ASSERT_EQUAL(getContext.expectedCompletionCBCall, true); //CB would have set to false
     }
     else
     {
         TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

         while(pGetContext->expectedCompletionCBCall)
         {
             ismEngine_FullMemoryBarrier();

             usleep(1000);
         }

         TEST_ASSERT_EQUAL(getContext.expectedDeliverCBCalls, 0); //Check msg was delivered
     }


     getContext.expectedDeliverCBCalls = 0;
     getContext.expectedCompletionCBCall = true;
     getContext.expectedCompletionCBCallRC = ISMRC_NoMsgAvail;

     rc = ism_engine_getMessage(
                              hSession,
                              ismDESTINATION_SUBSCRIPTION,
                              subName,
                              NULL,     ///< Only used for ismDESTINATION_TOPIC
                              300,
                              NULL,     ///< Which client owns the subscription
                              &pGetContext, sizeof(pGetContext),
                              basicGetCB,
                              NULL,
                              ismENGINE_CONSUMER_OPTION_ACK,
                              &pGetContext, sizeof(pGetContext),
                              basicGetCompletionCB);

      TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

      while(pGetContext->expectedCompletionCBCall)
      {
          ismEngine_FullMemoryBarrier();

          usleep(1000);
      }

      //Do another get which should wait as there are no messages avail
      getContext.expectedDeliverCBCalls = 0;
      getContext.expectedCompletionCBCall = false;

      rc = ism_engine_getMessage(
                               hSession,
                               ismDESTINATION_SUBSCRIPTION,
                               subName,
                               NULL,     ///< Only used for ismDESTINATION_TOPIC
                               50000,
                               NULL,     ///< Which client owns the subscription
                               &pGetContext, sizeof(pGetContext),
                               basicGetCB,
                               NULL,
                               ismENGINE_CONSUMER_OPTION_ACK,
                               &pGetContext, sizeof(pGetContext),
                               basicGetCompletionCB);

       TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

       //Now we have a get inflight, nack one of the messages we got earlier and
       //check the get has completed
       getContext.expectedDeliverCBCalls = 1;
       getContext.expectedCompletionCBCall = true;
       getContext.expectedCompletionCBCallRC = OK;

       TEST_ASSERT_EQUAL(pGetContext->numUnackedMsgs, 2);
       ismEngine_DeliveryHandle_t hToAck = pGetContext->unackedMessages[1];
       pGetContext->unackedMessages[1] = ismENGINE_NULL_DELIVERY_HANDLE;
       pGetContext->numUnackedMsgs = 1;

       rc = sync_ism_engine_confirmMessageDelivery( hSession
                                                  , NULL
                                                  , hToAck
                                                  , ismENGINE_CONFIRM_OPTION_NOT_RECEIVED);
       TEST_ASSERT_EQUAL(rc, OK);


       while(pGetContext->expectedCompletionCBCall)
       {
           ismEngine_FullMemoryBarrier();

           usleep(1000);
       }
       TEST_ASSERT_EQUAL(getContext.expectedDeliverCBCalls, 0); //Check msg was delivered


       //Now do a long running get that won't complete before we destroy the client
       getContext.expectedDeliverCBCalls = 0;
       getContext.expectedCompletionCBCall = true;
       getContext.expectedCompletionCBCallRC = ISMRC_Destroyed;

       rc = ism_engine_getMessage(
                                hSession,
                                ismDESTINATION_SUBSCRIPTION,
                                subName,
                                NULL,     ///< Only used for ismDESTINATION_TOPIC
                                50000,
                                NULL,     ///< Which client owns the subscription
                                &pGetContext, sizeof(pGetContext),
                                basicGetCB,
                                NULL,
                                ismENGINE_CONSUMER_OPTION_ACK,
                                &pGetContext, sizeof(pGetContext),
                                basicGetCompletionCB);

        TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

       //Now destroy the client which will make sure our completion callbacks haven't been fired
        rc = sync_ism_engine_destroyClientState(hClient,
                                                ismENGINE_DESTROY_CLIENT_OPTION_NONE);
        TEST_ASSERT_EQUAL(rc, OK);

        //Should have called get callback by now..
        TEST_ASSERT_EQUAL(getContext.expectedCompletionCBCall, false);
}

//Put 2 messages to a sub, one non-persistent and one persistent (second goes async)
//Do 2 gets (no acks with timeout of 0) and have them deliver
//Do another get with a timeout of 0 and check no message.
void test_getTime0(void)
{
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    char *getClientId="GetTime0Client";
    char *topicString = "/topic/getTime0";
    char *subName     = "test_getTime0";
    test_log(testLOGLEVEL_TESTNAME, "Starting %s\n",__func__);


    /* Create our clients and sessions */
    rc = test_createClientAndSession(getClientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    //Create the subscription we do it shared so that we don't have to ack the msg from
    //the previous consumer before asking for another.

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                                                  | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE
                                                  | ismENGINE_SUBSCRIPTION_OPTION_SHARED };

    rc = sync_ism_engine_createSubscription(
            hClient,
            subName,
            NULL,
            ismDESTINATION_TOPIC,
            topicString,
            &subAttrs,
            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Put a non-persistent message on the topic
    ismEngine_MessageHandle_t hMessage;

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, topicString,
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
            ismDESTINATION_TOPIC,
            topicString,
            NULL,
            hMessage);

    TEST_ASSERT_EQUAL(rc, OK);

    //Get message...
    testBasicGetContext_t getContext = {0};
    getContext.expectedDeliverCBCalls = 1;
    testBasicGetContext_t *pGetContext = &getContext;

    //1st message we get is non-persistent... shouldn't go async.
    rc = ism_engine_getMessage(
                         hSession,
                         ismDESTINATION_SUBSCRIPTION,
                         subName,
                         NULL,     ///< Only used for ismDESTINATION_TOPIC
                         0,
                         NULL,     ///< Which client owns the subscription
                         &pGetContext, sizeof(pGetContext),
                         basicGetCB,
                         NULL,
                         ismENGINE_CONSUMER_OPTION_ACK,
                         &pGetContext, sizeof(pGetContext),
                         basicGetCompletionCB);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(getContext.expectedDeliverCBCalls, 0); //Check msg was delivered

    /* Put another message - this time persistent. If we're using asyc IO, getting the
     * message will go async */

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT ,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, topicString,
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
            ismDESTINATION_TOPIC,
            topicString,
            NULL,
            hMessage);

    TEST_ASSERT_EQUAL(rc, OK);

    getContext.expectedDeliverCBCalls = 1;
    getContext.expectedCompletionCBCall = true;

    rc = ism_engine_getMessage(
                             hSession,
                             ismDESTINATION_SUBSCRIPTION,
                             subName,
                             NULL,     ///< Only used for ismDESTINATION_TOPIC
                             0,
                             NULL,     ///< Which client owns the subscription
                             &pGetContext, sizeof(pGetContext),
                             basicGetCB,
                             NULL,
                             ismENGINE_CONSUMER_OPTION_ACK,
                             &pGetContext, sizeof(pGetContext),
                             basicGetCompletionCB);

     if (rc == OK)
     {
         //We obviously weren't using async IO
         TEST_ASSERT_EQUAL(getContext.expectedDeliverCBCalls, 0); //Check msg was delivered
         //Check that the completion callback wasn't called:
         TEST_ASSERT_EQUAL(getContext.expectedCompletionCBCall, true); //CB would have set to false
     }
     else
     {
         TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

         while(pGetContext->expectedCompletionCBCall)
         {
             ismEngine_FullMemoryBarrier();

             usleep(1000);
         }

         TEST_ASSERT_EQUAL(getContext.expectedDeliverCBCalls, 0); //Check msg was delivered
     }


     getContext.expectedDeliverCBCalls = 0;
     getContext.expectedCompletionCBCall = true;
     getContext.expectedCompletionCBCallRC = ISMRC_NoMsgAvail;

     rc = ism_engine_getMessage(
                              hSession,
                              ismDESTINATION_SUBSCRIPTION,
                              subName,
                              NULL,     ///< Only used for ismDESTINATION_TOPIC
                              0,
                              NULL,     ///< Which client owns the subscription
                              &pGetContext, sizeof(pGetContext),
                              basicGetCB,
                              NULL,
                              ismENGINE_CONSUMER_OPTION_ACK,
                              &pGetContext, sizeof(pGetContext),
                              basicGetCompletionCB);

     if (rc == ISMRC_NoMsgAvail)
     {
         //Nothing went async and there was no message
         //Check that the completion callback wasn't called:
         TEST_ASSERT_EQUAL(getContext.expectedCompletionCBCall, true); //CB would have set to false
         TEST_ASSERT_EQUAL(getContext.expectedDeliverCBCalls, 0); //No deliveries should have occurred
         getContext.expectedCompletionCBCall = false;
     }
     else
     {
         TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

         while(pGetContext->expectedCompletionCBCall)
         {
             ismEngine_FullMemoryBarrier();

             usleep(1000);
         }
     }

      //Now destroy the client which will make sure our completion callbacks haven't been fired
      rc = sync_ism_engine_destroyClientState(hClient,
                                                ismENGINE_DESTROY_CLIENT_OPTION_NONE);
      TEST_ASSERT_EQUAL(rc, OK);

      //Should have called get callback by now..
      TEST_ASSERT_EQUAL(getContext.expectedCompletionCBCall, false);
}


typedef struct tag_testTimingWindowGetContext_t {
    volatile uint32_t expectedDeliverCBCalls;
    volatile bool     expectingCompletionCBCall;   //whether the completion callback should be called at all
    int32_t           actualCompletionCBCallRC;   //When it was called, what was the rc actually?
    pthread_mutex_t   *pMutex;
} testTimingWindowGetContext_t;

bool HangUntilReadyGetCB(
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
    testTimingWindowGetContext_t *pGetContext = *(testTimingWindowGetContext_t **)pConsumerContext;

    if (pGetContext->pMutex != NULL)
    {
        //Wait until we can lock the mutex
        int os_rc = pthread_mutex_lock(pGetContext->pMutex);
        TEST_ASSERT_EQUAL(os_rc, 0);
        os_rc = pthread_mutex_unlock(pGetContext->pMutex);
        TEST_ASSERT_EQUAL(os_rc, 0);
    }

    uint32_t expectedCalls = __sync_fetch_and_sub(&(pGetContext->expectedDeliverCBCalls), 1);
    TEST_ASSERT_CUNIT(expectedCalls> 0,
              ("expectedDeliverCBCalls was ZERO\n"));

    if (state != ismMESSAGE_STATE_CONSUMED) {
        int32_t rc = ism_engine_confirmMessageDelivery(hConsumer->pSession,
                                          NULL,
                                          hDelivery,
                                          ismENGINE_CONFIRM_OPTION_CONSUMED,
                                          NULL, 0, NULL);
        TEST_ASSERT_CUNIT(rc == OK, ("rc from ack was %d\n", rc));
    }
    ism_engine_releaseMessage(hMessage);
    return false;
}

void timingWindowCompletionCB(int rc, void *handle, void *context)
{
    testTimingWindowGetContext_t *pGetContext = *(testTimingWindowGetContext_t **)context;
    TEST_ASSERT_EQUAL(pGetContext->expectingCompletionCBCall, true);

    pGetContext->actualCompletionCBCallRC = rc;
    pGetContext->expectingCompletionCBCall = false;
}

typedef enum tag_testTimingWindowType_t
{
    destroyWhilstExpire = 0,
    putWhilstExpire,
    putANDDestroyWhilstExpire,
} testTimingWindowType_t;

void test_timingWindow(testTimingWindowType_t timingWindow)
{
    int32_t rc;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s, %s\n",__func__,
            ((timingWindow == destroyWhilstExpire)       ? "destroy client during expiry"
                    : ((timingWindow == putWhilstExpire) ? "send Message during expiry"
                                                         : "send and destroy during expiry")));

    uint32_t msgsRecvd = 0;
    uint32_t getsTimedOut = 0;
    uint32_t destroyWhilstExpireConsumersDestroyed = 0;

    for(uint32_t loop = 0; loop < 5; loop++)
    {
        ismEngine_ClientStateHandle_t hClient=NULL;
        ismEngine_SessionHandle_t hSession=NULL;
        ismEngine_MessageHandle_t hMessage=NULL;
        char *getClientId="BasicGetClient";
        char *topicString = "/topic/basicGet";


        /* Create our clients and sessions */
        rc = test_createClientAndSession(getClientId,
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hClient, &hSession, true);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hClient);
        TEST_ASSERT_PTR_NOT_NULL(hSession);

        ismEngine_ClientStateHandle_t hExtraPuttingClient=NULL;
        ismEngine_SessionHandle_t hExtraSession=NULL;
        volatile uint32_t extraPuttingClientActions = 0;

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };

        //Do an array of gets that will hang in the deliver callback if the test isn't
        //ready to destroy the client
        uint32_t numGets = 1000;
        testTimingWindowGetContext_t getContext[numGets];
        uint32_t timeOffsetMillis = 40; //0.04s

        struct timespec beforeGets;

        clock_gettime(CLOCK_REALTIME, &beforeGets);


        for (uint32_t i=0; i <numGets; i++)
        {
            memset(&(getContext[i]), 0, sizeof(testTimingWindowGetContext_t));
            getContext[i].pMutex = NULL; //Not used for this test
            getContext[i].expectingCompletionCBCall = true;

            if (  (timingWindow == putWhilstExpire)
                ||(timingWindow == putANDDestroyWhilstExpire))
            {
                getContext[i].expectedDeliverCBCalls  = 1;
            }
            else
            {
                getContext[i].expectedDeliverCBCalls  = 0;
            }

            testTimingWindowGetContext_t *pGetContext = &getContext[i];

            rc = ism_engine_getMessage(hSession,
                                       ismDESTINATION_TOPIC,
                                       topicString,
                                       &subAttrs,
                                       timeOffsetMillis,
                                       NULL,     ///< Which client owns the subscription
                                       &pGetContext, sizeof(pGetContext),
                                       HangUntilReadyGetCB,
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_NONE,
                                       &pGetContext, sizeof(pGetContext),
                                       timingWindowCompletionCB);

            TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        }


        //wait until our gets start to expire
        struct timespec target = beforeGets;
        target.tv_nsec += timeOffsetMillis*1000000; //millis converted to nanos
        while (beforeGets.tv_nsec > 1000000000)
        {
            target.tv_nsec -= 1000000000;
            target.tv_sec  += 1;
        }

        if (timingWindow == putWhilstExpire)
        {
            rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                    ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                    ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                    ismMESSAGE_FLAGS_NONE,
                                    0,
                                    ismDESTINATION_TOPIC, topicString,
                                    &hMessage, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

        }
        else if (timingWindow == putANDDestroyWhilstExpire)
        {
            //We want to overlap the put with the destroy of the getters, we'll
            //use a separate client (so the put doesn't prevent the destroy starting)
            //and we'll use a persistent message so he put is likely to go async
            rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                    ismMESSAGE_PERSISTENCE_PERSISTENT,
                                    ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                    ismMESSAGE_FLAGS_NONE,
                                    0,
                                    ismDESTINATION_TOPIC, topicString,
                                    &hMessage, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            /* Create our clients and sessions */
            rc = test_createClientAndSession("ExtraPutClient",
                                             NULL,
                                             ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                             ismENGINE_CREATE_SESSION_OPTION_NONE,
                                             &hExtraPuttingClient, &hExtraSession, true);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hExtraPuttingClient);
            TEST_ASSERT_PTR_NOT_NULL(hExtraSession);
        }

        //For debugging we'll see how long we needed to wait
        struct timespec beforeWait;
        clock_gettime(CLOCK_REALTIME, &beforeWait);


        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &target, NULL);

        if (timingWindow == putWhilstExpire)
        {
            rc = sync_ism_engine_putMessageOnDestination(hSession,
                    ismDESTINATION_TOPIC,
                    topicString,
                    NULL,
                    hMessage);

            TEST_ASSERT_EQUAL(rc, OK);
        }
        else if (timingWindow == putANDDestroyWhilstExpire)
        {
            volatile uint32_t *pExtraPuttingClientActions = &(extraPuttingClientActions);
            test_incrementActionsRemaining(pExtraPuttingClientActions, 1);

            rc = ism_engine_putMessageOnDestination(hExtraSession,
                    ismDESTINATION_TOPIC,
                    topicString,
                    NULL,
                    hMessage,
                    &pExtraPuttingClientActions, sizeof(pExtraPuttingClientActions),
                    test_decrementActionsRemaining);

            if (rc == OK)
            {
                test_decrementActionsRemaining(rc, NULL, &pExtraPuttingClientActions);
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
            }
        }

       //Now destroy the client which will make sure our completion callbacks haven't been fired
       rc = sync_ism_engine_destroyClientState(hClient,
                                                 ismENGINE_DESTROY_CLIENT_OPTION_NONE);
       TEST_ASSERT_EQUAL(rc, OK);

       //All the get callbacks should have been fired before client destroy finished
       for (uint32_t i=0; i <numGets; i++)
       {
           //So we should still be expecting the callback
           TEST_ASSERT_EQUAL(getContext[i].expectingCompletionCBCall, false);
       }

       if (timingWindow == putANDDestroyWhilstExpire)
       {
           //Wait for put to finish and destroy extra putting client
           test_waitForActionsTimeOut(&extraPuttingClientActions, 20);

           rc = sync_ism_engine_destroyClientState(hExtraPuttingClient,
                                                   ismENGINE_DESTROY_CLIENT_OPTION_NONE);
           TEST_ASSERT_EQUAL(rc, OK);
       }

       if (timingWindow == destroyWhilstExpire)
       {
           for (uint32_t i=0; i < numGets; i++)
           {
               //callback should already be called
               TEST_ASSERT_EQUAL(getContext[i].expectingCompletionCBCall, false);

               if (getContext[i].actualCompletionCBCallRC == ISMRC_Destroyed)
               {
                   destroyWhilstExpireConsumersDestroyed++;
               }
               else
               {
                   TEST_ASSERT_EQUAL(getContext[i].actualCompletionCBCallRC, ISMRC_NoMsgAvail);
               }
           }
       }
       else
       {
           for (uint32_t i=0; i <numGets; i++)
           {
               TEST_ASSERT_EQUAL(getContext[i].expectingCompletionCBCall, false);

               if (getContext[i].actualCompletionCBCallRC == ISMRC_OK)
               {
                   msgsRecvd++;
               }
               else if (getContext[i].actualCompletionCBCallRC == ISMRC_NoMsgAvail)
               {
                   getsTimedOut++;
               }
               else
               {
                   //Must have been destroyed by the session destroy...
                   TEST_ASSERT_EQUAL(getContext[i].actualCompletionCBCallRC, ISMRC_Destroyed);
               }
           }
       }


       //testLOGLEVEL_CHECK
       test_log(testLOGLEVEL_CHECK, "Waited until           %lu s %lu ns.\n"
                                    "Time at start of wait  %lu s %lu ns.\n",
                                         target.tv_sec, target.tv_nsec,
                                         beforeWait.tv_sec, beforeWait.tv_nsec);
   }

   if (timingWindow == destroyWhilstExpire)
   {
       if (destroyWhilstExpireConsumersDestroyed == 0)
       {
           //Can't fail the test as it's possible all callbacks fired before destroy acted...
           printf("NOTE: d'oh... all callback fired before destroy: consider more callback or longer get expiry!!!");
       }
   }
   else
   {

       if (msgsRecvd == 0)
       {
           //Can't fail the test as it's possible all callbacks fired before put got far enough...
           printf("NOTE: d'oh... all callback fired before put: consider more callback or longer get expiry!!!");
       }
       if (getsTimedOut == 0)
       {
           printf("NOTE: d'oh, no callbacks expired: consider more callbacks or later put\n");
       }
   }
}

void test_timingWindowDestroy(void)
{
    test_timingWindow(destroyWhilstExpire);
}
void test_timingWindowPut(void)
{
    test_timingWindow(putWhilstExpire);
}
void test_timingWindowSendAndDestroy(void)
{
    test_timingWindow(putANDDestroyWhilstExpire);
}

CU_TestInfo ISM_expiringGet_CUnit_testsuite_Basic[] =
{
    { "basicGet",                   test_basicGet },
    { "getTime0",                   test_getTime0 },
    { "timingWindowDestroy",        test_timingWindowDestroy},
    { "timingWindowSendMsg",        test_timingWindowPut},
    { "timingWindowSendAndDestroy", test_timingWindowSendAndDestroy},
    CU_TEST_INFO_NULL
};

int initSuite(void)
{
    return test_engineInit(true, true,
                           true, // Disable Auto creation of named queues
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termSuite(void)
{
    return test_engineTerm(true);
}

//Test ideas:
//Test no async if message arrives immediately
//a) Create arrays of 4000s timers expiring at small intervals that try and take a lock that is already taken
//   Release lock and then destroy session
//b) As above but send message... all getters can be on same topic

CU_SuiteInfo ISM_expiringGet_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("Basic", initSuite, termSuite, ISM_expiringGet_CUnit_testsuite_Basic),
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

extern void ism_common_traceSignal(int signo);

/*
 * Catch a terminating signal and try to flush the trace file.
 */
static void sig_handler(int signo) {
    static volatile int in_handler = 0;
    signal(signo, SIG_DFL);
    if (in_handler)
        raise(signo);
    in_handler = 1;
    ism_common_traceSignal(signo);
    raise(signo);
}

int main(int argc, char *argv[])
{
    int trclvl = 0;
    int retval = 0;
    int testLogLevel = 2;

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    signal(SIGINT, sig_handler);
    signal(SIGSEGV, sig_handler);

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_expiringGet_CUnit_allsuites);

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
