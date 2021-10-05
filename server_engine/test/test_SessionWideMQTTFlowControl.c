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
#include <sys/prctl.h>
#include <unistd.h>

#include "engine.h"
#include "engineInternal.h"
#include "queueCommon.h"
#include "policyInfo.h" //For setting maxdepth to infinity..
#include "clientState.h"
#include "clientStateInternal.h" //For seeing max deliveryid
#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_client.h"
#include "test_utils_initterm.h"
#include "test_utils_log.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_task.h"

typedef struct tag_PublisherInfo_t {
    char *topicPrefix;
    uint32_t numTopics;
} PublisherInfo_t;

typedef struct tag_rcvdMsgDetails_t {
    struct tag_rcvdMsgDetails_t *next;
    ismEngine_DeliveryHandle_t hDelivery; ///< Set by the callback so the processing thread can ack it
    ismEngine_MessageHandle_t  hMessage; ///<Set by the callback so the processing thread can release it
    ismMessageState_t  msgState;  ///< Set by the callback so the processing thread knows whether to ack it
} rcvdMsgDetails_t;

typedef struct tag_receiveMsgContext_t {
    ismEngine_SessionHandle_t hSession; ///< Used to ack messages by processor thread
    rcvdMsgDetails_t *firstMsg;   ///< Head pointer for list of received messages
    rcvdMsgDetails_t *lastMsg;   ///< Tail pointer for list of received messages
    pthread_mutex_t msgListMutex; ///< Used to protect the received message list
    pthread_cond_t msgListCond;  ///< This used to broadcast when there are messages in the list to process
} receiveMsgContext_t;

volatile bool keepPublishing = true;
volatile bool keepAcking     = true;
volatile bool keepStartStopping = true; //Only used in testExternalStartStop

volatile uint64_t msgsArrived = 0;
volatile uint64_t msgsAcked   = 0;
volatile uint64_t msgsSent    = 0;

void *publishToTopic(void *args)
{
    PublisherInfo_t *pubInfo =(PublisherInfo_t *)args;
    char *tname="pubbingthread";
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)tname);
    int32_t rc = OK;

    ism_engine_threadInit(0);

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    rc = test_createClientAndSession("PubbingClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    int topicNum=0;

    while(keepPublishing)
    {
        ismEngine_MessageHandle_t hMessage;
        char topicString[256];

        sprintf(topicString, "%s%d", pubInfo->topicPrefix, topicNum);

        topicNum++;
        if(topicNum >= pubInfo->numTopics)topicNum = 0;

        // Publish a persistent message
        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
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

        __sync_fetch_and_add(&msgsSent, 1);
    }

    rc = sync_ism_engine_destroyClientState(hClient,
                                            ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    test_log(testLOGLEVEL_CHECK, "Publisher Thread exiting after sending %lu messages", msgsSent);

    ism_engine_threadTerm(1);
    return NULL;
}

void waitForCompletion(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext)
{
    TEST_ASSERT_EQUAL(retcode, OK);

    bool **ppIsCompleted = (bool **)pContext;
    **ppIsCompleted = true;
}


//When we're given a message, record we got it and add it to a list to ack later
bool ConsumeDelayAck(
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
    receiveMsgContext_t *context = *(receiveMsgContext_t **)pConsumerContext;

    TEST_ASSERT_EQUAL(state, ismMESSAGE_STATE_DELIVERED);

    uint64_t numArrived = __sync_add_and_fetch(&msgsArrived, 1);

    if( numArrived % 16384 == 0)
    {
        test_log(testLOGLEVEL_TESTPROGRESS, "Msgs Arrived: %lu", numArrived);
    }


    //Copy the details we want from the message into some memory....
    rcvdMsgDetails_t *msgDetails = malloc(sizeof(rcvdMsgDetails_t));
    TEST_ASSERT_PTR_NOT_NULL(msgDetails);

    msgDetails->hDelivery = hDelivery;
    msgDetails->hMessage  = hMessage;
    msgDetails->msgState  = state;
    msgDetails->next      = NULL;

    //Add it to the list of messages....
    int32_t os_rc = pthread_mutex_lock(&(context->msgListMutex));
    TEST_ASSERT_EQUAL(os_rc, 0);

    if(context->firstMsg == NULL)
    {
        context->firstMsg = msgDetails;
        TEST_ASSERT_EQUAL(context->lastMsg, NULL);
        context->lastMsg = msgDetails;
    }
    else
    {
        TEST_ASSERT_PTR_NOT_NULL(context->lastMsg);
        context->lastMsg->next = msgDetails;
        context->lastMsg       = msgDetails;
    }

    //Broadcast that we've added some messages to the list
    os_rc = pthread_cond_broadcast(&(context->msgListCond));
    TEST_ASSERT_EQUAL(os_rc, 0);

    //Let the processing thread access the list of messages...
    os_rc = pthread_mutex_unlock(&(context->msgListMutex));
    TEST_ASSERT_EQUAL(os_rc, 0);


    return true; //true - we'd like more messages
}

//Acks/releases messages
void *getProcessorThread(void *arg)
{
    char tname[20];
    sprintf(tname, "GP");
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    ism_engine_threadInit(0);

    receiveMsgContext_t *context = (receiveMsgContext_t *)arg;

    while(keepAcking)
    {
        int os_rc = pthread_mutex_lock(&(context->msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        while ((context->firstMsg == NULL) && keepAcking)
        {
            os_rc = pthread_cond_wait(&(context->msgListCond), &(context->msgListMutex));
            TEST_ASSERT_EQUAL(os_rc, 0);
        }

        if (!keepAcking)
        {
            os_rc = pthread_mutex_unlock(&(context->msgListMutex));
            TEST_ASSERT_EQUAL(os_rc, 0);
            break;
        }

        TEST_ASSERT_PTR_NOT_NULL(context->firstMsg);
        rcvdMsgDetails_t *msg = context->firstMsg;

        context->firstMsg = msg->next;
        if(context->lastMsg == msg)
        {
            TEST_ASSERT_EQUAL(msg->next, NULL);
            context->lastMsg = NULL;
        }

        os_rc = pthread_mutex_unlock(&(context->msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        ism_engine_releaseMessage(msg->hMessage);

        if (msg->msgState != ismMESSAGE_STATE_CONSUMED)
        {

            int rc = sync_ism_engine_confirmMessageDelivery(context->hSession,
                                              NULL,
                                              msg->hDelivery,
                                              ismENGINE_CONFIRM_OPTION_CONSUMED);
            TEST_ASSERT_EQUAL(rc, OK);

            uint64_t numAcked = __sync_add_and_fetch(&msgsAcked ,1);

            if( numAcked % 16384 == 0)
            {
                test_log(testLOGLEVEL_TESTPROGRESS, "Msgs Acked: %lu", numAcked);
            }
        }
        free(msg);
    }

    ism_engine_threadTerm(1);

    test_log(testLOGLEVEL_CHECK, "Acking Thread exiting after acking %lu messages", msgsAcked);
    return NULL;
}

void test_NoDeliveryIds(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    //Set up 5 subscriptions on different topics (publish 30,000 to each and keep publishing on a separate thread) .
    //Create consumers (which don't ack yet) on each sub /then/ start message delivery
    //Wait until we run out of delivery ids via the 65k hard coded limit in MQTT pause then start acking
    //Because we have fiddled with the inflight limit to allow us to hit 65k then we will be spewing FDCs but we
    //run with a low enough trace level that they "disappear"
    //stop publishing
    //check we drain.

    test_log(testLOGLEVEL_TESTNAME, "Starting test_NoDeliveryIds");

    msgsSent    = 0;
    msgsArrived = 0;
    msgsAcked   = 0;

    char *topicPrefix="/test/NoDeliveryId/";
    char *subPrefix="sub";
    uint32_t numSubscriptions = 5;

    receiveMsgContext_t consInfo = {0};
    receiveMsgContext_t *pConsInfo = &consInfo;

    // Set default maxMessageCount to 0 for the duration of the test
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 0;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer[numSubscriptions];

    int32_t rc = test_createClientAndSession("SubbingClient",
                                             NULL,
                                             ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                             ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                             &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    int os_rc = pthread_mutex_init(&(consInfo.msgListMutex), NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);
    os_rc = pthread_cond_init(&(consInfo.msgListCond), NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);
    consInfo.firstMsg = NULL;
    consInfo.lastMsg = NULL;
    consInfo.hSession = hSession;

    for (uint32_t i=0; i < numSubscriptions; i++)
    {
        //Create a durable subscription and consumer...
        //(after receiving minUnackedMessages... consumer will randomly choose when do suspend...
        ismEngine_SubscriptionAttributes_t subAttrs = {0};

        if (i%2 == 0)
        {
            subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
        }
        if (i%3 == 0)
        {
            subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
        }
        else
        {
            subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
        }

        char subName[256];
        char topicString[256];
        sprintf(subName, "%s%d", subPrefix, i);
        sprintf(topicString, "%s%d", topicPrefix, i);

        if (subAttrs.subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE)
        {
            rc = sync_ism_engine_createSubscription(hClient,
                                               subName,
                                               NULL,
                                               ismDESTINATION_TOPIC,
                                               topicString,
                                               &subAttrs,
                                               NULL); // Owning client same as requesting client
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_engine_createConsumer( hSession
                    , ismDESTINATION_SUBSCRIPTION
                    , subName
                    , NULL
                    , NULL // Owning client same as session client
                    , &pConsInfo
                    , sizeof(pConsInfo)
                    , ConsumeDelayAck
                    , NULL
                    , test_getDefaultConsumerOptions(subAttrs.subOptions)
                    , &(hConsumer[i])
                    , NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            rc = ism_engine_createConsumer( hSession
                    , ismDESTINATION_TOPIC
                    , topicString
                    , &subAttrs
                    , NULL // Owning client same as session client
                    , &pConsInfo
                    , sizeof(pConsInfo)
                    , ConsumeDelayAck
                    , NULL
                    , test_getDefaultConsumerOptions(subAttrs.subOptions)
                    , &(hConsumer[i])
                    , NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    rc = ism_engine_startMessageDelivery(hSession,ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL );
    TEST_ASSERT_EQUAL(rc, OK);


    //Set up a thread publishing to a topic...
    keepPublishing = true;
    pthread_t publishingThreadId;
    PublisherInfo_t pubberInfo = {topicPrefix, numSubscriptions};
    os_rc = test_task_startThread( &publishingThreadId,publishToTopic, &pubberInfo,"publishToTopic");
    TEST_ASSERT_EQUAL(os_rc, 0);

    //Find out how many potential delivery ids there are so we know when we'll run out of ids
    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)hClient;

    iecsMessageDeliveryInfo_t *pMsgDelInfo = NULL;
    while (iecs_findClientMsgDelInfo(pThreadData, pClient->pClientId, &pMsgDelInfo) == ISMRC_NotFound)
    {
        usleep(120);
    }
    TEST_ASSERT_PTR_NOT_NULL(pMsgDelInfo);

    uint32_t idmax = pMsgDelInfo->MaxDeliveryId;

    // While we have a pointer to the MsgDeliveryInfo, just make a call to get the stats so we have
    // checked that code.
    bool idsExhausted;
    uint32_t inflight, inflightMax, inflightReenable;
    iecs_getDeliveryStats(pThreadData, pMsgDelInfo, &idsExhausted, &inflight, &inflightMax, &inflightReenable);
    TEST_ASSERT_GREATER_THAN_OR_EQUAL(idmax, inflight);
    iecs_releaseMessageDeliveryInfoReference(pThreadData, pMsgDelInfo);

    //Wait until enough messages have been received to run out of delivery ids
    while (msgsArrived < idmax)
    {
        usleep(120);
    }

    //Start the acking thread
    keepAcking = true;
    pthread_t ackingThreadId;
    os_rc = test_task_startThread( &ackingThreadId,getProcessorThread, &consInfo,"getProcessorThread");
    TEST_ASSERT_EQUAL(os_rc, 0);


    //wait for a few more messages to be published...
    while(msgsSent < 67000)
    {
        usleep(10);
    }

    //Stop publishing
    keepPublishing = false;

    //Wait for the queue to drain
    bool queuesDrained = false;

    uint64_t lastArrivedNum =0;
    uint64_t lastAckedNum =0;

    uint64_t loopsWithNoActivity=0;

    do
    {
        queuesDrained =true;

        for (uint32_t i=0; i < numSubscriptions; i++)
        {
            ismEngine_QueueStatistics_t QStats;
            ismEngine_Consumer_t *pConsumer =(ismEngine_Consumer_t *)hConsumer[i];

            ieq_getStats(pThreadData, pConsumer->queueHandle, &QStats);

            if (QStats.BufferedMsgs > 0)
            {
                queuesDrained = false;
            }
        }

        if (!queuesDrained)
        {
            if ((msgsAcked == lastAckedNum)&&(msgsArrived == lastArrivedNum))
            {
                loopsWithNoActivity++;
            }
            else
            {
                loopsWithNoActivity = 0;
            }
            lastArrivedNum = msgsArrived;
            lastAckedNum = msgsAcked;

            if (loopsWithNoActivity < 60*1000)
            {
                usleep(1000); //sleep for 1 millisecond
            }
            else
            {
                //It's been a minute with no activity! - something has hung
                abort();
            }
        }
    }
    while(!queuesDrained);

    //stop the acking thread
    keepAcking = false;

    //poke the acking thread so it notices
    os_rc = pthread_mutex_lock(&(consInfo.msgListMutex));
    TEST_ASSERT_EQUAL(os_rc, 0);
    os_rc = pthread_cond_broadcast(&(consInfo.msgListCond));
    TEST_ASSERT_EQUAL(os_rc, 0);
    os_rc = pthread_mutex_unlock(&(consInfo.msgListMutex));
    TEST_ASSERT_EQUAL(os_rc, 0);


    //destroyclient....
    volatile bool clientDestroyed = false;
    volatile bool *pClientDestroyed = &clientDestroyed;

    rc = ism_engine_destroyClientState( hClient
            , ismENGINE_DESTROY_CLIENT_OPTION_DISCARD
            , &pClientDestroyed
            , sizeof(pClientDestroyed)
            , waitForCompletion);

    //...if async... wait for the client destruction to complete..
    if (rc == ISMRC_AsyncCompletion)
    {
        //wait for client to go away...
        while(!clientDestroyed);
    }
    else
    {
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Join the threads..
    test_log(testLOGLEVEL_VERBOSE, "Joining publisher...");
    os_rc = pthread_join(publishingThreadId, NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);
    test_log(testLOGLEVEL_VERBOSE, "Joining acker...");
    os_rc = pthread_join(ackingThreadId, NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);


    TEST_ASSERT_EQUAL(msgsAcked,msgsArrived);
    TEST_ASSERT_EQUAL(msgsAcked,msgsSent);
}

//Below Here are things for test_EngineExternalStartStop...
#define TEST_EXTERNALSTARTSTOP_NUMSUBS 5

//Acks/releases messages
void *engineFlowControlThread(void *arg)
{
    char tname[20];
    sprintf(tname, "EngineFlow");
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    int32_t rc = OK;

    ism_engine_threadInit(0);

    ismEngine_SessionHandle_t hSession = *(ismEngine_SessionHandle_t *)arg;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    while(keepStartStopping)
    {
        //Randomly choose to try starting or stopping or sleeping...
        uint32_t operationChoice = rand() % 3;

        switch (operationChoice)
        {
        case 0:
             //start
             rc = ism_engine_startMessageDelivery(hSession,ismENGINE_START_DELIVERY_OPTION_ENGINE_START, NULL, 0, NULL );
             TEST_ASSERT_CUNIT( (  (rc==OK)||(rc==ISMRC_AsyncCompletion)
                                 ||(rc==ISMRC_RequestInProgress)||(rc==ISMRC_NotEngineControlled))
                              , ("rc was %d\n", rc));
            /* no break - fall thru */

        case 1:
             //Stop with no wait for async (so sometimes when we choose to stop and check we can have a stop in progress
             rc = stopMessageDeliveryInternal(pThreadData, hSession, ISM_ENGINE_INTERNAL_STOPMESSAGEDELIVERY_FLAGS_ENGINE_STOP, NULL, 0, NULL);
             TEST_ASSERT_CUNIT( (  (rc==OK)||(rc==ISMRC_AsyncCompletion)
                                 ||(rc==ISMRC_NotEngineControlled))
                              , ("rc was %d\n", rc));
             /* no break - fall thru */

        default:
            usleep(50);
            break;
        }
    }

    ism_engine_threadTerm(1);

    test_log(testLOGLEVEL_VERBOSE, "Fake Engine flow control thread ending");
    return NULL;
}

uint32_t guessBatchSize(ismEngine_SessionHandle_t hSession)
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

void *fakeProtocolThread(void *arg)
{
    char tname[20];
    sprintf(tname, "FakeProtocol");
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    int32_t rc = OK;

    ism_engine_threadInit(0);

    ismEngine_SessionHandle_t hSession = *(ismEngine_SessionHandle_t *)arg;

    uint32_t numChecks = 0;

    do
    {
        //Randomly choose to try starting or stopping or sleeping...
        uint32_t operationChoice = rand() % 5;

        switch (operationChoice)
        {
        case 0:
        case 1:
             //start
             rc = ism_engine_startMessageDelivery(hSession,ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL );
             TEST_ASSERT_CUNIT( ((rc==OK)||(rc==ISMRC_AsyncCompletion)||(rc==ISMRC_RequestInProgress))
                              , ("rc was %d\n", rc));
             /* no break - fall thru */
        case 2:
             //Stop with no wait for async (so sometimes when we choose to stop and check we can have a stop in progress
             rc = ism_engine_stopMessageDelivery(hSession, NULL, 0, NULL);
             TEST_ASSERT_CUNIT( ((rc==OK)||(rc==ISMRC_AsyncCompletion))
                              , ("rc was %d\n", rc));
             /* no break - fall thru */
        case 3:
             //Stop and check message flow stops...
        {
            volatile bool stopCompleted = false;
            volatile bool *pStopCompleted = &stopCompleted;

            rc = ism_engine_stopMessageDelivery(hSession,
                    &pStopCompleted, sizeof(pStopCompleted), waitForCompletion);
            TEST_ASSERT_CUNIT( ((rc==OK)||(rc==ISMRC_AsyncCompletion))
                    , ("rc was %d\n", rc));

            if (rc==ISMRC_AsyncCompletion)
            {
                while(!pStopCompleted)
                {
                    ismEngine_ReadMemoryBarrier();
                }
            }
            ismEngine_ReadMemoryBarrier();

            //Sadly when stopMessageDelivery returns/fires callback each consumer can still be in the process of
            //receiving a message batch so we can only check that each subscription hasn't received more than a batch full
            uint64_t msgsArrivedStart = msgsArrived;

            uint64_t msgsArrivedLoopStart;
            do {
                msgsArrivedLoopStart = msgsArrived;
                usleep(500);
            }
            while(msgsArrived !=  msgsArrivedLoopStart);

            TEST_ASSERT_CUNIT((msgsArrived <= (msgsArrivedStart+(guessBatchSize(hSession)*TEST_EXTERNALSTARTSTOP_NUMSUBS))),
                              ("Too many messages arrived whilst we were waiting: msgsArrived: %lu, msgStart: %lu\n",
                                      msgsArrived, msgsArrivedStart));
            numChecks++;
        }
                break;

        default:
            usleep(50);
            break;
        }
    }
    while (numChecks < 10);

    //Signal we have finished...
    keepStartStopping = false;

    ism_engine_threadTerm(1);

    test_log(testLOGLEVEL_VERBOSE, "Fake Protocol thread ending");
    return NULL;
}

void checkExternalStartStop(ExpectedMsgRate_t msgRate, uint32_t maxInflight)
{
    //Set up 5 subscriptions on different topics
    //Publish messages to them/receive them/ack them
    //Have two threads starting stopping the receiving session, one marked internal, the other using the default options (external).
    //Periodically check that if the external thread thinks the session is disabled, message delivery seems to stop
    //(i.e. that internally we never enable a session that protocol have disabled....

    test_log(testLOGLEVEL_TESTNAME, "Starting test_ExternalStartStop");

    msgsSent    = 0;
    msgsArrived = 0;
    msgsAcked   = 0;

    char *topicPrefix="/test/ExternalStartStop/";
    char *subPrefix="sub";
    uint32_t numSubscriptions = TEST_EXTERNALSTARTSTOP_NUMSUBS;

    receiveMsgContext_t consInfo = {0};
    receiveMsgContext_t *pConsInfo = &consInfo;

    // Set default maxMessageCount to 0 for the duration of the test
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 0;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer[numSubscriptions];

    int32_t rc = test_createClientAndSession("SubbingClient",
                                             NULL,
                                             ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                             ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                             &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);


    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)hClient;

    if (maxInflight != 0)
    {
        pClient->expectedMsgRate = msgRate;
        pClient->maxInflightLimit = maxInflight;
        test_log(testLOGLEVEL_TESTDESC, "Expected MsgRate %u inflight limit: %u",
                                             pClient->expectedMsgRate, pClient->maxInflightLimit);
    }
    else
    {
        //Choose some values for expected message rate and maxInflightLimit
        switch (rand() % 4)
        {
            case 0:
                pClient->expectedMsgRate = EXPECTEDMSGRATE_LOW;
                pClient->maxInflightLimit = 1 + rand()%10;
                test_log(testLOGLEVEL_TESTDESC, "Expected MsgRate Low inflight limit: %u", pClient->maxInflightLimit);
                break;

            case 1:
                pClient->expectedMsgRate = EXPECTEDMSGRATE_DEFAULT;
                pClient->maxInflightLimit = 10 + rand()%118;
                test_log(testLOGLEVEL_TESTDESC, "Expected MsgRate default inflight limit: %u", pClient->maxInflightLimit);
                break;

            case 2:
                pClient->expectedMsgRate = EXPECTEDMSGRATE_HIGH;
                pClient->maxInflightLimit = 256 + rand()%256;
                test_log(testLOGLEVEL_TESTDESC, "Expected MsgRate High inflight limit: %u", pClient->maxInflightLimit);
                break;

            case 3:
                pClient->expectedMsgRate = EXPECTEDMSGRATE_MAX;
                pClient->maxInflightLimit = 1024 + rand()%1024;
                test_log(testLOGLEVEL_TESTDESC, "Expected MsgRate Max inflight limit: %u", pClient->maxInflightLimit);
                break;
        }
    }


    int os_rc = pthread_mutex_init(&(consInfo.msgListMutex), NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);
    os_rc = pthread_cond_init(&(consInfo.msgListCond), NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);
    consInfo.firstMsg = NULL;
    consInfo.lastMsg = NULL;
    consInfo.hSession = hSession;

    for (uint32_t i=0; i < numSubscriptions; i++)
    {
        //Create a durable subscription and consumer...
        //(after receiving minUnackedMessages... consumer will randomly choose when do suspend...
        ismEngine_SubscriptionAttributes_t subAttrs = {0};

        if (i%2 == 0)
        {
            subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
        }
        if (i%3 == 0)
        {
            subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
        }
        else
        {
            subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
        }

        char subName[256];
        char topicString[256];
        sprintf(subName, "%s%d", subPrefix, i);
        sprintf(topicString, "%s%d", topicPrefix, i);

        if (subAttrs.subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE)
        {
            rc = sync_ism_engine_createSubscription(hClient,
                    subName,
                    NULL,
                    ismDESTINATION_TOPIC,
                    topicString,
                    &subAttrs,
                    NULL); // Owning client same as requesting client
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_engine_createConsumer( hSession
                    , ismDESTINATION_SUBSCRIPTION
                    , subName
                    , NULL
                    , NULL // Owning client same as session client
                    , &pConsInfo
                    , sizeof(pConsInfo)
                    , ConsumeDelayAck
                    , NULL
                    , test_getDefaultConsumerOptions(subAttrs.subOptions)
                    , &(hConsumer[i])
                    , NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            rc = ism_engine_createConsumer( hSession
                    , ismDESTINATION_TOPIC
                    , topicString
                    , &subAttrs
                    , NULL // Owning client same as session client
                    , &pConsInfo
                    , sizeof(pConsInfo)
                    , ConsumeDelayAck
                    , NULL
                    , test_getDefaultConsumerOptions(subAttrs.subOptions)
                    , &(hConsumer[i])
                    , NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    rc = ism_engine_startMessageDelivery(hSession,ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    keepAcking = true;
    keepPublishing = true;
    keepStartStopping = true;

    //Set up a thread publishing to a topic...
    pthread_t publishingThreadId;
    PublisherInfo_t pubberInfo = {topicPrefix, numSubscriptions};
    os_rc = test_task_startThread( &publishingThreadId,publishToTopic, &pubberInfo,"publishToTopic");
    TEST_ASSERT_EQUAL(os_rc, 0);

    //Start the acking thread
    pthread_t ackingThreadId;
    os_rc = test_task_startThread( &ackingThreadId,getProcessorThread, &consInfo,"getProcessorThread");
    TEST_ASSERT_EQUAL(os_rc, 0);

    //Start the "engine" start stopping thread (i.e. the thread that pretends to be the engine
    //doing flow control
    pthread_t engineFlowControlThreadId;
    os_rc = test_task_startThread( &engineFlowControlThreadId,engineFlowControlThread, &hSession,"engineFlowControlThread");
    TEST_ASSERT_EQUAL(os_rc, 0);

    //Start the protocol start stopping thread (i.e. the thread that pretends to be the protocol
    //doing flow control and checking when this thread thinks delivery is disabled no messages are delivered
    pthread_t fakeProtocolThreadId;
    os_rc = test_task_startThread( &fakeProtocolThreadId,fakeProtocolThread, &hSession,"fakeProtocolThread");
    TEST_ASSERT_EQUAL(os_rc, 0);


    //When the start/stopping external thread has run the test it turns off the keep
    //starting stopping flag.... wait for that
    while(keepStartStopping)
    {
        usleep(1000);
    }

    //Turn everyone off
    keepAcking     = false;
    keepPublishing = false;

    //poke the acking thread so it notices
    os_rc = pthread_mutex_lock(&(consInfo.msgListMutex));
    TEST_ASSERT_EQUAL(os_rc, 0);
    os_rc = pthread_cond_broadcast(&(consInfo.msgListCond));
    TEST_ASSERT_EQUAL(os_rc, 0);
    os_rc = pthread_mutex_unlock(&(consInfo.msgListMutex));
    TEST_ASSERT_EQUAL(os_rc, 0);


    test_log(testLOGLEVEL_VERBOSE, "Joining publisher...");
    //Join the threads...
    os_rc = pthread_join(publishingThreadId, NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);
    test_log(testLOGLEVEL_VERBOSE, "Joining acker...");
    os_rc = pthread_join(ackingThreadId, NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);
    test_log(testLOGLEVEL_VERBOSE, "Joining fake engine control...");
    os_rc = pthread_join(engineFlowControlThreadId, NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);
    test_log(testLOGLEVEL_VERBOSE, "Joining fake protocol...");
    os_rc = pthread_join(fakeProtocolThreadId, NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);

    //destroyclient....
    volatile bool clientDestroyed = false;
    volatile bool *pClientDestroyed = &clientDestroyed;

    rc = ism_engine_destroyClientState( hClient
            , ismENGINE_DESTROY_CLIENT_OPTION_DISCARD
            , &pClientDestroyed
            , sizeof(pClientDestroyed)
            , waitForCompletion);

    //...if async... wait for the client destruction to complete..
    if (rc == ISMRC_AsyncCompletion)
    {
        //wait for client to go away...
        while(!clientDestroyed)
        {
            usleep(20*100);
        }
    }
    else
    {
        TEST_ASSERT_EQUAL(rc, OK);
    }

    test_log(testLOGLEVEL_TESTPROGRESS, "test_ExternalStartStop: Msgs Sent: %lu (Arrived %lu, Acked %lu) but external stop delivery seemed to work",
             msgsSent, msgsArrived, msgsAcked);
}

void test_ExternalStartStop(void)
{
    //Do test with very small client
    checkExternalStartStop(EXPECTEDMSGRATE_LOW, 1);

    //Do test with very large client
    checkExternalStartStop(EXPECTEDMSGRATE_MAX, 2048);

    //Choose a random inflight limit
    checkExternalStartStop(EXPECTEDMSGRATE_UNDEFINED, 0);
}

int initNoDeliverySuite(void)
{
    // Increase the number of messages in flight  so we can  run out of delivery ids
    // for a client by hitting the hardcoded 65k limit
    ism_field_t f;

    ism_prop_t *staticConfigProps = ism_common_getConfigProperties();

    f.type = VT_Integer;
    f.val.i = 66000;
    int rc = ism_common_setProperty( staticConfigProps, ismENGINE_CFGPROP_MAX_MQTT_UNACKED_MESSAGES, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s=%d returned %d\n", ismENGINE_CFGPROP_MAX_MQTT_UNACKED_MESSAGES, f.val.i, rc);
        return 10;
    }

    rc = test_engineInit(true, true,
                             true, // Disable Auto creation of named queues
                             false, /*recovery should complete ok*/
                             ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                             testDEFAULT_STORE_SIZE);

    return rc;
}

int initNoDeliveryScheduleRestartSuite(void)
{
    // Increase the number of messages in flight  so high that even though we start to turn off
    //delivery at 65k, the "low water mark" at which we turn delivery back on is even higher
    //so as soon as we start to turn it off... we release we need to turn it back on...
    ism_field_t f;

    ism_prop_t *staticConfigProps = ism_common_getConfigProperties();

    f.type = VT_Integer;
    f.val.i = 100000;
    int rc = ism_common_setProperty( staticConfigProps, ismENGINE_CFGPROP_MAX_MQTT_UNACKED_MESSAGES, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s=%d returned %d\n", ismENGINE_CFGPROP_MAX_MQTT_UNACKED_MESSAGES, f.val.i, rc);
        return 10;
    }

    rc = test_engineInit(true, true,
                             true, // Disable Auto creation of named queues
                             false, /*recovery should complete ok*/
                             ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                             testDEFAULT_STORE_SIZE);

    return rc;
}

int initSuite(void)
{
    //Set a normal number of MQTT messages in flight
    ism_field_t f;

    ism_prop_t *staticConfigProps = ism_common_getConfigProperties();

    f.type = VT_Integer;
    f.val.i = 128;
    int rc = ism_common_setProperty( staticConfigProps, ismENGINE_CFGPROP_MAX_MQTT_UNACKED_MESSAGES, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s=%d returned %d\n", ismENGINE_CFGPROP_MAX_MQTT_UNACKED_MESSAGES, f.val.i, rc);
        return 10;
    }

    rc = test_engineInit(true, true,
                             true, // Disable Auto creation of named queues
                             false, /*recovery should complete ok*/
                             ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                             testDEFAULT_STORE_SIZE);
    return rc;
}

int termSuite(void)
{
    return test_engineTerm(true);
}

CU_TestInfo ISM_SessionFlowControl_CUnit_test_NoDeliveryIds[] =
{
    { "test_NoDeliveryIds", test_NoDeliveryIds },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_SessionFlowControl_CUnit_test_ExternalStartStop[] =
{
    { "test_ExternalStartStop", test_ExternalStartStop },
    CU_TEST_INFO_NULL
};

CU_SuiteInfo ISM_SessionFlowControl_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("TestDeliveryIdsSuite", initNoDeliverySuite, termSuite, ISM_SessionFlowControl_CUnit_test_NoDeliveryIds),
    IMA_TEST_SUITE("TestDeliveryIdsScheduleRestartSuite", initNoDeliveryScheduleRestartSuite, termSuite, ISM_SessionFlowControl_CUnit_test_NoDeliveryIds),
    IMA_TEST_SUITE("TestExternalStartStop", initSuite, termSuite, ISM_SessionFlowControl_CUnit_test_ExternalStartStop),
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
    int testLogLevel = testLOGLEVEL_TESTPROGRESS;

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_SessionFlowControl_CUnit_allsuites);

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
