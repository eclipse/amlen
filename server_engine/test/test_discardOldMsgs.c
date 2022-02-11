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
#include <math.h>

#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_client.h"
#include "test_utils_initterm.h"
#include "test_utils_sync.h"
#include "test_utils_log.h"
#include "test_utils_security.h"
#include "test_utils_task.h"

#include "engineInternal.h"
#include "queueCommon.h"
#include "policyInfo.h"
#include "topicTree.h"
#include "lockManager.h"
#include "multiConsumerQInternal.h"
#include "mempool.h"
#include "engineDiag.h"

typedef struct tag_publisherContext_t {
    pthread_t threadId;
    uint64_t threadNum;
    char *topicString;
    uint64_t numMessages;
    bool persistent;
    int reliability;
} publisherContext_t;

//structure for recording details of a message for acking later
typedef struct tag_rcvdMsgDetails_t {
    struct tag_rcvdMsgDetails_t *next;
    ismEngine_DeliveryHandle_t hDelivery; ///< Set by the callback so the processing thread can ack it
    ismMessageState_t  msgState;  ///< Set by the callback so the processing thread knows whether to ack it
} rcvdMsgDetails_t;

typedef struct tag_ackerContext_t {
     char StructId[4];   //"CACK"
     ismEngine_SessionHandle_t hSession;
     int ackQos;
     rcvdMsgDetails_t *firstMsg;   ///< Head pointer for list of received messages
     rcvdMsgDetails_t *lastMsg;   ///< Tail pointer for list of received messages
     pthread_mutex_t msgListMutex; ///< Used to protect the received message list
     pthread_cond_t msgListCond;  ///< This used to broadcast when there are messages in the list to process
     uint64_t acksStarted;
     uint64_t acksCompleted;
     bool stopAcking;
} ackerContext_t;
#define ACKERCONTEXT_STRUCTID "CACK"

//Context used by both the consumer (and for hiugher qoses, the acking thread too)
typedef struct tag_consumerContext_t {
    char StructId[4];
    uint64_t numPublishers;         //Input to the consumer
    bool lastMsgsWasFlaggedAsLast;  //Was the last message we saw marked as last from publisher
    bool seenGaps;
    bool publishersCompleted;
    ackerContext_t ackContext;
    uint64_t lastMsgFromPublisher[0]; //Actually numPublishers entries in array
} consumerContext_t;

typedef struct tag_messageContents_t {
    uint64_t msgNum;
    uint64_t publisherNum;
    bool lastMsgsFromThisPublisher;
} messageContents_t;

typedef struct tag_receiveAckInFlight_t {
    ismEngine_SessionHandle_t hSession;   ///< Use this for subsequent consume
    ismEngine_DeliveryHandle_t hDelivery; ///< Once receive completed, consume this
    uint64_t *pAcksCompleted;              ///< Once subsequent consume completed, increase this
} receiveAckInFlight_t;

static uint64_t numMsgsReceived = 0;


void *publishMessages(void *context)
{
    int32_t rc;
    publisherContext_t *pubContext = (publisherContext_t *)context;

    char tname[20];
    sprintf(tname, "PubThread-%lu",  pubContext->threadNum);
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    ism_engine_threadInit(0);

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ProducerHandle_t hProducer=NULL;
    char clientId[64];
    sprintf(clientId, "PublishingClient-%lu", pubContext->threadNum);

    rc = test_createClientAndSession(clientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_TOPIC,
                                   pubContext->topicString,
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer);

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    test_incrementActionsRemaining(pActionsRemaining, pubContext->numMessages);
    for (int i = 1; i <= pubContext->numMessages; i++)
    {
        ismEngine_MessageHandle_t hMessage;
        messageContents_t msgContents;
        messageContents_t *pMsgContents = &msgContents;


        msgContents.msgNum = i;
        msgContents.publisherNum = pubContext->threadNum;
        msgContents.lastMsgsFromThisPublisher = (i == pubContext->numMessages);

        rc = test_createMessage(sizeof(msgContents),
                                ((pubContext->persistent) ? ismMESSAGE_PERSISTENCE_PERSISTENT:ismMESSAGE_PERSISTENCE_NONPERSISTENT) ,
                                pubContext->reliability,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, pubContext->topicString,
                                &hMessage, (void **)&pMsgContents);
        TEST_ASSERT_EQUAL(rc, OK);



        rc = ism_engine_putMessage(hSession,
                                   hProducer,
                                   NULL,
                                   hMessage,
                                   &pActionsRemaining,
                                   sizeof(pActionsRemaining),
                                   test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }

    test_waitForRemainingActions(pActionsRemaining);

    rc = sync_ism_engine_destroyProducer(hProducer);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroySession(hSession);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroyClientState(hClient,
                                            ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_engine_threadTerm(1);

    return NULL;
}

void runPublishers( uint64_t numThreads
                  , uint64_t msgsPerThread
                  , char *topicString
                  , bool persistent
                  , int reliability)
{
    publisherContext_t pubContext[numThreads];
    int rc = 0;

    for (uint64_t threadNum = 0; threadNum < numThreads; threadNum++)
    {
        pubContext[threadNum].numMessages = msgsPerThread;
        pubContext[threadNum].topicString = topicString;
        pubContext[threadNum].threadNum   = threadNum;
        pubContext[threadNum].persistent = persistent;
        pubContext[threadNum].reliability = reliability;

        rc = test_task_startThread(&(pubContext[threadNum].threadId),publishMessages, (void *)&(pubContext[threadNum]),"publishMessages");
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //wait for all the publishers to finish
    for (uint64_t i=0; i < numThreads; i++)
    {
        rc = pthread_join(pubContext[i].threadId, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

void consumeCompleted(int rc, void *handle, void *context)
{
    TEST_ASSERT_EQUAL(rc, OK);
    uint64_t *pAcksCompleted = *(uint64_t **)context;

    __sync_fetch_and_add(pAcksCompleted, 1);
}

void startConsume( ismEngine_SessionHandle_t hSession,
                   ismEngine_DeliveryHandle_t hDelivery,
                   uint64_t *pAcksCompleted)
{
    int rc = ism_engine_confirmMessageDelivery(hSession,
                                               NULL,
                                               hDelivery,
                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                               &pAcksCompleted, sizeof(pAcksCompleted),
                                               consumeCompleted);

    TEST_ASSERT_CUNIT(((rc == OK)||(rc == ISMRC_AsyncCompletion)),
                      ("rc was %d", rc));

    if (rc == OK)
    {
        __sync_fetch_and_add(pAcksCompleted, 1);
    }
}

void rcvdAckCompleted(int rc, void *handle, void *context)
{
    receiveAckInFlight_t *pAckInfo = (receiveAckInFlight_t *)context;

    startConsume( pAckInfo->hSession
                , pAckInfo-> hDelivery
                , pAckInfo->pAcksCompleted);
}

void startAck(int subQos,
              ismEngine_SessionHandle_t hSession,
              rcvdMsgDetails_t *msg,
              uint64_t *pAcksStarted,
              uint64_t *pAcksCompleted)
{
    __sync_fetch_and_add(pAcksStarted, 1);

    bool readyForConsume = true;


    //If we are emulating Qos 2, first loop marking them received, before we then mark them consumed
    if ((subQos == 2) && (msg->msgState == ismMESSAGE_STATE_DELIVERED))
    {
        msg->msgState   = ismMESSAGE_STATE_RECEIVED;
        readyForConsume = false;
        receiveAckInFlight_t ackInfo = {hSession, msg->hDelivery, pAcksCompleted};

        int rc = ism_engine_confirmMessageDelivery(hSession,
                                                   NULL,
                                                   msg->hDelivery,
                                                   ismENGINE_CONFIRM_OPTION_RECEIVED,
                                                   &ackInfo, sizeof(ackInfo),
                                                   rcvdAckCompleted);

        TEST_ASSERT_CUNIT(((rc == OK)||(rc == ISMRC_AsyncCompletion)),
                          ("rc was %d", rc));

        if (rc == OK)
        {
            readyForConsume = true;
        }
    }


    if (readyForConsume)
    {
        TEST_ASSERT_CUNIT((    ((subQos == 3) && (msg->msgState == ismMESSAGE_STATE_DELIVERED))   //subQos == 3 for JMS in this test
                            || ((subQos == 2) && (msg->msgState == ismMESSAGE_STATE_RECEIVED))
                            || ((subQos == 1) && (msg->msgState == ismMESSAGE_STATE_DELIVERED))),
                ("Unexpected msg state for message we are about to consume: %u", msg->msgState));

        startConsume(hSession, msg->hDelivery, pAcksCompleted);
    }
}

//Acks messages
void *AckingThread(void *arg)
{
    char tname[20];
    sprintf(tname, "AckingThread");
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    ism_engine_threadInit(0);

    ackerContext_t *context = (ackerContext_t *)arg;
    TEST_ASSERT_EQUAL(ismEngine_CompareStructId(context->StructId,ACKERCONTEXT_STRUCTID), true);

    while(1)
    {
        int os_rc = pthread_mutex_lock(&(context->msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        while (context->firstMsg == NULL && !(context->stopAcking))
        {
            os_rc = pthread_cond_wait(&(context->msgListCond), &(context->msgListMutex));
            TEST_ASSERT_EQUAL(os_rc, 0);
        }

        if (context->firstMsg == NULL && context->stopAcking)
        {
            //No more messages coming and we have none to ack...
            os_rc = pthread_mutex_unlock(&(context->msgListMutex));
            TEST_ASSERT_EQUAL(os_rc, 0);
            break;
        }

        rcvdMsgDetails_t *msg = context->firstMsg;
        context->firstMsg = msg->next;
        if(context->lastMsg == msg)
        {
            TEST_ASSERT_EQUAL(msg->next, NULL);
            context->lastMsg = NULL;
        }

        os_rc = pthread_mutex_unlock(&(context->msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        startAck(context->ackQos,
                 context->hSession,
                 msg,
                 &(context->acksStarted),
                 &(context->acksCompleted));

        free(msg);
    }

    while (context->acksCompleted < context->acksStarted)
    {
        usleep(100);
    }

    ism_engine_threadTerm(1);

    return NULL;
}


void addMessageToAckList(ackerContext_t *pContext,
                         ismEngine_DeliveryHandle_t      hDelivery,
                         ismMessageState_t               state)
{
    //Copy the details we want from the message into some memory....
    rcvdMsgDetails_t *msgDetails = malloc(sizeof(rcvdMsgDetails_t));
    TEST_ASSERT_PTR_NOT_NULL(msgDetails);

    msgDetails->hDelivery = hDelivery;
    msgDetails->msgState  = state;
    msgDetails->next      = NULL;

    //Add it to the list of messages....
    int32_t os_rc = pthread_mutex_lock(&(pContext->msgListMutex));
    TEST_ASSERT_EQUAL(os_rc, 0);

    if(pContext->firstMsg == NULL)
    {
        pContext->firstMsg = msgDetails;
        TEST_ASSERT_EQUAL(pContext->lastMsg, NULL);
        pContext->lastMsg = msgDetails;
    }
    else
    {
        TEST_ASSERT_PTR_NOT_NULL(pContext->lastMsg);
        pContext->lastMsg->next = msgDetails;
        pContext->lastMsg       = msgDetails;
    }

    //Broadcast that we've added some messages to the list
    os_rc = pthread_cond_broadcast(&(pContext->msgListCond));
    TEST_ASSERT_EQUAL(os_rc, 0);

    //Let the processing thread access the list of messages...
    os_rc = pthread_mutex_unlock(&(pContext->msgListMutex));
    TEST_ASSERT_EQUAL(os_rc, 0);
}

static bool MsgFlowConsumerFunction(
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
    consumerContext_t *pContext = *(consumerContext_t **)pConsumerContext;

    TEST_ASSERT_EQUAL(ismEngine_CompareStructId(pContext->ackContext.StructId,ACKERCONTEXT_STRUCTID), true);
    TEST_ASSERT_EQUAL(areaCount, 2);
    messageContents_t msgContents;

    TEST_ASSERT_EQUAL(areaLengths[1], sizeof(msgContents));
    memcpy(&msgContents, pAreaData[1], sizeof(msgContents));

    test_log(testLOGLEVEL_VERBOSE, "Got msg %lu, from publisher %lu %s Last msg from this publisher was %lu (Gap: %lu)",
                             msgContents.msgNum, msgContents.publisherNum,
                             (  (msgContents.lastMsgsFromThisPublisher) ?
                                  "(Marked as last message from this publisher!)":""),
                              pContext->lastMsgFromPublisher[msgContents.publisherNum],
                              (msgContents.msgNum - pContext->lastMsgFromPublisher[msgContents.publisherNum] - 1));

    if (msgContents.msgNum != (1 + pContext->lastMsgFromPublisher[msgContents.publisherNum]))
    {
        pContext->seenGaps = true;
    }

    pContext->lastMsgFromPublisher[msgContents.publisherNum] = msgContents.msgNum;
    pContext->lastMsgsWasFlaggedAsLast = msgContents.lastMsgsFromThisPublisher;

    if (state != ismMESSAGE_STATE_CONSUMED)
    {
        addMessageToAckList( &(pContext->ackContext),
                             hDelivery,
                             state);
    }

    __sync_fetch_and_add(&numMsgsReceived, 1);

    ism_engine_releaseMessage(hMessage);

    return true;
}

void examineConsumerStats( char *clientId
                         , ismQHandle_t qhdl
                         , uint64_t numAckingConsumers
                         , uint64_t numNonAckingConsumers
                         , char *statsSubName )
{
    //Get some consumer stats, mainly to test the stats
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    iempMemPoolHandle_t memPoolHdl = NULL;

    rc = iemp_createMemPool( pThreadData
                           , iemem_diagnostics
                           , 0
                           , 2048
                           , 2048
                           , &memPoolHdl);
    TEST_ASSERT_EQUAL(rc, OK);

    uint64_t numConsumers = numAckingConsumers+numNonAckingConsumers;

    // Go straight for the consumer stats directly...
    ieqConsumerStats_t consumerStats[numConsumers];

    rc = ieq_getConsumerStats(pThreadData, qhdl, memPoolHdl, &numConsumers, consumerStats);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(numConsumers, numAckingConsumers+numNonAckingConsumers);

    uint64_t consumersWithMsgsInflight =0;

    for (uint64_t i=0; i <numConsumers; i++)
    {
        TEST_ASSERT_STRINGS_EQUAL(clientId, consumerStats[i].clientId);
        if (consumerStats[i].messagesInFlightToConsumer > 0)
        {
            consumersWithMsgsInflight++;
        }
        TEST_ASSERT_EQUAL(consumerStats[i].sessionDeliveryStarted,   true);
        TEST_ASSERT_EQUAL(consumerStats[i].sessionDeliveryStopping, false);
    }
    TEST_ASSERT_GREATER_THAN_OR_EQUAL(numAckingConsumers, consumersWithMsgsInflight);

    iemp_destroyMemPool( pThreadData
                       , &memPoolHdl);

    if (statsSubName != NULL)
    {
        //Initialise OpenSSL - in the product - the equivalent is ism_ssl_init();
        //TODO: Do we need to run that in the engine thread that does this or is it done?
        sslInit();

        // Do it twice, once writing to outputStr, and once to a file.
        for(int32_t loop=0; loop<2; loop++)
        {
            char requestStr[4096];

            if (loop == 0)
            {
                sprintf(requestStr, "%s=%s %s=%s", ediaVALUE_FILTER_CLIENTID, clientId, ediaVALUE_FILTER_SUBNAME, statsSubName);
            }
            else
            {
                sprintf(requestStr, "%s=%s %s=%s %s=%s %s=%s",
                        ediaVALUE_FILTER_CLIENTID, clientId,
                        ediaVALUE_FILTER_SUBNAME, statsSubName,
                        ediaVALUE_PARM_FILENAME, "FILENAME",
                        ediaVALUE_PARM_PASSWORD, "PASSWORD");
            }

            // Go through the engine diag interface...
            char *outputStr = NULL;

            rc = ism_engine_diagnostics(ediaVALUE_MODE_SUBDETAILS,
                                        requestStr,
                                        &outputStr,
                                        NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(outputStr);

            if (loop == 0)
            {
                // TODO: Check outputStr is valid JSON and contains the expected values.
            }
            else
            {
                // TODO: Check outputStr contains a filePath
            }

            // printf("OUTPUT: '%s'\n", outputStr);

            ism_engine_freeDiagnosticsOutput(outputStr);
        }

        sslTerm();
    }
}


void test_consume(int subOptions,
                  bool consumeDuringPublish,
                  uint64_t numPublishingThreads,
                  uint64_t numAckingConsumers,
                  uint64_t numNonAckingConsumers)
{
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;

    ismEngine_ConsumerHandle_t hAckingConsumers[numAckingConsumers+1];
    consumerContext_t *pAckingConsumerContext[numAckingConsumers+1];
    pthread_t AckingThreadId[numAckingConsumers+1];

    ismEngine_ConsumerHandle_t hNonAckingConsumers[numNonAckingConsumers+1];
    consumerContext_t *pNonAckingConsumerContext[numNonAckingConsumers+1];

    uint64_t numMsgsPerPublisher  = 1000;
    numMsgsReceived = 0;

    ismEngine_SubscriptionAttributes_t subAttrs = {subOptions};
    int subQosOptions = subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK;
    bool durable = (subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE) ? true : false;
    bool shared  = (subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED)  ? true : false;

    char *topicString = "/topic/test_consume";
    char *subName     = "test_consume";
    char *clientId    = "ConsumeClient";
    test_log(testLOGLEVEL_TESTNAME, "Starting %s(SubQos = %s, %s, %s, NumPublishingThreads = %lu)...", __func__,
                     ((subQosOptions == ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE) ? "2":
                             ((subQosOptions == ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE) ? "1":
                                     ((subQosOptions == ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE) ? "0":
                                             (subQosOptions == ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE)? "JMS" :"???"))),
                     (consumeDuringPublish ? "consumeDURINGPublish"     : "consumeAFTERPublish"),
                     (durable              ? "durable (cleanSession=0)" : "non-durable (cleanSession=1)"),
                     numPublishingThreads);

    if (!durable && !consumeDuringPublish)
    {
        printf("Can't consume after publish for a non-durable subscription");
        abort();
    }

    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 50;
    iepi_getDefaultPolicyInfo(false)->maxMsgBehavior = DiscardOldMessages;

    /* Create our clients and sessions */
    rc = test_createClientAndSession(clientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);


    if (durable || shared)
    {
        rc = sync_ism_engine_createSubscription(
                hClient,
                subName,
                NULL,
                ismDESTINATION_TOPIC,
                topicString,
                &subAttrs,
                NULL); // Owning client same as requesting client
        TEST_ASSERT_EQUAL(rc, OK);

        if (!consumeDuringPublish)
        {
            //Publish all the messages before we connect the consumer(s)
            runPublishers(numPublishingThreads, numMsgsPerPublisher, topicString, true, ismMESSAGE_RELIABILITY_EXACTLY_ONCE);
        }
    }

    //Set up the acking consumers
    for (uint32_t consNum = 0 ;consNum < numAckingConsumers; consNum++)
    {
        pAckingConsumerContext[consNum] = calloc(1, sizeof(consumerContext_t)+(sizeof(uint64_t)*numPublishingThreads));
        pAckingConsumerContext[consNum]->numPublishers = numPublishingThreads;
        ismEngine_SetStructId(pAckingConsumerContext[consNum]->StructId, "CONS");
        ismEngine_SetStructId(pAckingConsumerContext[consNum]->ackContext.StructId, ACKERCONTEXT_STRUCTID);

        if (subQosOptions != ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE)
        {
            int os_rc = pthread_mutex_init(&(pAckingConsumerContext[consNum]->ackContext.msgListMutex), NULL);
            TEST_ASSERT_EQUAL(os_rc, 0);
            os_rc = pthread_cond_init(&(pAckingConsumerContext[consNum]->ackContext.msgListCond), NULL);
            TEST_ASSERT_EQUAL(os_rc, 0);
            pAckingConsumerContext[consNum]->ackContext.hSession = hSession;
            pAckingConsumerContext[consNum]->ackContext.firstMsg  = NULL;
            pAckingConsumerContext[consNum]->ackContext.lastMsg   = NULL;
            pAckingConsumerContext[consNum]->ackContext.stopAcking = false;
            pAckingConsumerContext[consNum]->ackContext.ackQos = subQosOptions -1;


            rc = test_task_startThread(&(AckingThreadId[consNum]),AckingThread, (void *)(&(pAckingConsumerContext[consNum]->ackContext)),"AckingThread");
            TEST_ASSERT_EQUAL(rc, OK);
        }

        rc = ism_engine_createConsumer( hSession
                                     , (durable ? ismDESTINATION_SUBSCRIPTION : ismDESTINATION_TOPIC)
                                     , (durable ? subName : topicString )
                                     , (durable ? NULL : &subAttrs)
                                     , NULL
                                     , &pAckingConsumerContext[consNum]
                                     , sizeof(pAckingConsumerContext[consNum])
                                     , MsgFlowConsumerFunction
                                     , NULL
                                     ,   ismENGINE_CONSUMER_OPTION_ACK
                                     |  (((subQosOptions & ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE) == 0) ? ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID: 0)
                                     , &hAckingConsumers[consNum]
                                     , NULL, 0, NULL);

        TEST_ASSERT_EQUAL(ismEngine_CompareStructId(pAckingConsumerContext[consNum]->ackContext.StructId,ACKERCONTEXT_STRUCTID), true);
  
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hAckingConsumers[consNum]);
    }

    //Set up the non-acking consumers
    for (uint32_t consNum = 0 ; consNum < numNonAckingConsumers;  consNum++)
    {
        pNonAckingConsumerContext[consNum] = calloc(1, sizeof(consumerContext_t)+(sizeof(uint64_t)*numPublishingThreads));
        pNonAckingConsumerContext[consNum]->numPublishers = numPublishingThreads;
        ismEngine_SetStructId(pNonAckingConsumerContext[consNum]->StructId, "CONS");
        ismEngine_SetStructId(pNonAckingConsumerContext[consNum]->ackContext.StructId, ACKERCONTEXT_STRUCTID);

        rc = ism_engine_createConsumer( hSession
                , (durable ? ismDESTINATION_SUBSCRIPTION : ismDESTINATION_TOPIC)
                , (durable ? subName : topicString )
                , (durable ? NULL : &subAttrs)
                , NULL
                , &(pNonAckingConsumerContext[consNum])
                , sizeof(pNonAckingConsumerContext[consNum])
                , MsgFlowConsumerFunction
                , NULL
                , ismENGINE_CONSUMER_OPTION_NONE
                , &(hNonAckingConsumers[consNum])
                , NULL, 0, NULL);

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hNonAckingConsumers[consNum]);
    }

    if (consumeDuringPublish)
    {
        //Publish all the messages now that we have started the consumer(s)
        runPublishers(numPublishingThreads, numMsgsPerPublisher, topicString,true, ismMESSAGE_RELIABILITY_EXACTLY_ONCE);
    }

    char *statsSubName = durable ? subName : NULL;

    //Wait until all the queues are empty...
    for (uint32_t consNum = 0 ;consNum < numAckingConsumers; consNum++)
    {
        ismQHandle_t qhdl = ((ismEngine_Consumer_t *)hAckingConsumers[consNum])->queueHandle;
        examineConsumerStats(clientId, qhdl, numAckingConsumers, numNonAckingConsumers, statsSubName);
        test_waitForEmptyQ(qhdl, 500, 20);
    }
    for (uint32_t consNum = 0 ;consNum < numNonAckingConsumers; consNum++)
    {
        ismQHandle_t qhdl = ((ismEngine_Consumer_t *)hNonAckingConsumers[consNum])->queueHandle;
        examineConsumerStats(clientId, qhdl, numAckingConsumers, numNonAckingConsumers, statsSubName);
        test_waitForEmptyQ(qhdl, 500, 20);
    }

    for (uint32_t consNum = 0 ; consNum < numAckingConsumers; consNum++)
    {
        //Stop the acking thread
        int32_t os_rc = pthread_mutex_lock(&(pAckingConsumerContext[consNum]->ackContext.msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        pAckingConsumerContext[consNum]->ackContext.stopAcking = true;

        //Broadcast that the flag changed state
        os_rc = pthread_cond_broadcast(&(pAckingConsumerContext[consNum]->ackContext.msgListCond));
        TEST_ASSERT_EQUAL(os_rc, 0);

        //Let the processing thread access the list of messages...
        os_rc = pthread_mutex_unlock(&(pAckingConsumerContext[consNum]->ackContext.msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        rc = pthread_join(AckingThreadId[consNum], NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Delete Consumer(s)
    for (uint32_t consNum = 0 ; consNum < numAckingConsumers; consNum++)
    {
        rc = ism_engine_destroyConsumer(hAckingConsumers[consNum], NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        free(pAckingConsumerContext[consNum]);
    }

    for (uint32_t consNum = 0 ;  consNum < numNonAckingConsumers; consNum++)
    {
        rc = ism_engine_destroyConsumer(hNonAckingConsumers[consNum], NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        free(pNonAckingConsumerContext[consNum]);
    }

    //Delete sub if necessary
    if (durable || shared)
    {
        ismEngine_Subscription_t *pSub = NULL;

        ieutThreadData_t *pThreadData = ieut_getThreadData();
        rc = iett_findClientSubscription(pThreadData,
                                         clientId,
                                         iett_generateClientIdHash(clientId),
                                         subName,
                                         iett_generateSubNameHash(subName),
                                         &pSub);
        TEST_ASSERT_EQUAL(rc, OK);

        ismEngine_QueueStatistics_t stats;
        ieq_getStats(pThreadData, pSub->queueHandle, &stats);

        //Check the queue thinks it has no messages on..
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);

        //Check that the msgs received + num rejected is same as number put
        TEST_ASSERT_EQUAL(   numMsgsReceived
                           + stats.DiscardedMsgs,
                           numPublishingThreads * numMsgsPerPublisher);

        iett_releaseSubscription(pThreadData, pSub, false);

        if (stats.DiscardedMsgs > 0)
        {
            test_log(testLOGLEVEL_CHECK, "Queue records messages were discarded");
        }
        else
        {
            test_log(testLOGLEVEL_CHECK, "Queue claims no messages were discarded");
        }

        rc = ism_engine_destroySubscription(hClient, subName, hClient,
                                            NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}


void test_checkDiscard(int subOptions)
{
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer=NULL;

    uint64_t numMsgsPerPublisher = 160;
    numMsgsReceived = 0;

    uint64_t queueMaxDepth = 100;

    ismEngine_SubscriptionAttributes_t subAttrs = {subOptions};
    int subQosOptions = subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK;
    bool durable = (subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE) ? true : false;

    char *topicString = "/topic/test_checkDiscard";
    char *subName     = "test_checkDiscard";
    char *clientId    = "CheckClient";

    test_log(testLOGLEVEL_TESTNAME, "Starting %s(SubQos = %s, %sdurable)...", __func__,
                     ((subQosOptions == ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE) ? "2":
                             ((subQosOptions == ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE) ? "1":
                                     ((subQosOptions == ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE) ? "0":
                                             (subQosOptions == ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE)? "JMS" :"???"))),
                     durable?"":"non-");

    iepi_getDefaultPolicyInfo(false)->maxMessageCount = queueMaxDepth;
    iepi_getDefaultPolicyInfo(false)->maxMsgBehavior = DiscardOldMessages;

    /* Create our clients and sessions */
    rc = test_createClientAndSession(clientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, durable);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    //Create the subscription / consumer
    if (durable)
    {
        rc = sync_ism_engine_createSubscription(
            hClient,
            subName,
            NULL,
            ismDESTINATION_TOPIC,
            topicString,
            &subAttrs,
            NULL); // Owning client same as requesting client
    }
    else
    {
        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       topicString,
                                       &subAttrs,
                                       hClient,
                                       NULL, 0, NULL,
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                       &hConsumer,
                                       NULL, 0, NULL);
    }

    TEST_ASSERT_EQUAL(rc, OK);

    //Publish all the messages
    runPublishers(1, numMsgsPerPublisher, topicString, durable, durable ? ismMESSAGE_RELIABILITY_EXACTLY_ONCE : ismMESSAGE_RELIABILITY_AT_MOST_ONCE);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    //Now check the queue depth is a sensible number
    ismQHandle_t queueHandle = NULL;
    if (durable)
    {
        ismEngine_Subscription_t *pSub = NULL;

        rc = iett_findClientSubscription(pThreadData,
                                         clientId,
                                         iett_generateClientIdHash(clientId),
                                         subName,
                                         iett_generateSubNameHash(subName),
                                         &pSub);
        TEST_ASSERT_EQUAL(rc, OK);

        queueHandle = pSub->queueHandle;

        iett_releaseSubscription(pThreadData, pSub, false);
    }
    else
    {
        queueHandle = hConsumer->queueHandle;
    }

    ismEngine_QueueStatistics_t stats;
    ieq_getStats(pThreadData, queueHandle, &stats);

    test_log(testLOGLEVEL_TESTPROGRESS, "Buffered msgs is %lu\n HWM: %lu ",stats.BufferedMsgs, stats.BufferedMsgsHWM);

    TEST_ASSERT_CUNIT( ((stats.BufferedMsgs >= 0.95 * queueMaxDepth) && (stats.BufferedMsgs <= queueMaxDepth)),
                       ("Buffered msgs was %lu\n",stats.BufferedMsgs));


    if (durable)
    {
        rc = ism_engine_destroySubscription(hClient, subName, hClient,
                                            NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    else
    {
        rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_qos0NondurableConsumeDuring(void)
{
    test_consume( ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE, true,  15, 0, 1);
}
void test_qos0DurableConsumeDuring(void)
{
    test_consume( (ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE | ismENGINE_SUBSCRIPTION_OPTION_DURABLE),
                  true,  15, 0, 1);
}
void test_qos1DurableConsumeDuring(void)
{
    test_consume( (ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE | ismENGINE_SUBSCRIPTION_OPTION_DURABLE),
                  true, 15, 1, 0);
}
void test_qos2DurableConsumeDuring(void)
{
    test_consume( (ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE | ismENGINE_SUBSCRIPTION_OPTION_DURABLE),
                  true,  15, 1, 0);


}

void test_jmsDurableConsumeDuring(void)
{
    test_consume( (ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE | ismENGINE_SUBSCRIPTION_OPTION_DURABLE),
                  true, 15, 1, 0);
}

void test_simpleSharedDurableConsumeDuring(void)
{
    test_consume( (  ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE
                   | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                   | ismENGINE_SUBSCRIPTION_OPTION_SHARED),
                  true, 15, 2, 2);
}

void test_qos0NDCheckDiscard(void)
{
    test_checkDiscard(  ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE );
}
void test_qos0DCheckDiscard(void)
{
    test_checkDiscard(  ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE
                      | ismENGINE_SUBSCRIPTION_OPTION_DURABLE);
}
void test_qos1CheckDiscard(void)
{
    test_checkDiscard(  ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE
                      | ismENGINE_SUBSCRIPTION_OPTION_DURABLE);
}
void test_qos2CheckDiscard(void)
{
    test_checkDiscard(  ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                      | ismENGINE_SUBSCRIPTION_OPTION_DURABLE);
}
void test_JMSCheckDiscard(void)
{
    test_checkDiscard(  ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE
                      | ismENGINE_SUBSCRIPTION_OPTION_DURABLE);
}

typedef enum __attribute__ ((__packed__)) tag_testTransactionState_t
{
    notstarted          = 0,
    creatingtran,
    readyforputs,
    putsinprogress,
    waitingforinflightputs,
    committing
} testTransactionState_t;

typedef struct tag_discardRetainedTestPublisherContext_t {
    pthread_t threadId;
    uint64_t threadNum;
    volatile bool *pKeepPublishing;
    bool retained;
    char *topicStringPrefix;
    uint32_t transPerPubThread;
    uint32_t msgsPerTran;
    uint32_t expiry;
    uint64_t numTopics;
    uint64_t numMessagesPublishStarted;
    uint64_t numMessagesPublishCompleted; ///< Less than above because final trans rolled back
} discardRetainedTestPublisherContext_t;

typedef struct tag_discardRetainedTestConsumerContext_t {
    pthread_t ackingThreadId;
    uint64_t  msgsDelivered;
    uint32_t  disableConsumerEveryNMsgs;
    ackerContext_t ackContext;
} discardRetainedTestConsumerContext_t;

typedef struct tag_testPutTranInfo_t {
    volatile testTransactionState_t  transState;
    ismEngine_TransactionHandle_t    transHandle;
    volatile uint64_t *pInprogressCount;
} testPutTranInfo_t;

void finishDiscardRetainedPublisherPub(int rc, void *handle, void *context)
{
    volatile uint32_t *pMsgsInFlightForTran = *(uint32_t **)context;

    DEBUG_ONLY uint32_t oldVal = __sync_fetch_and_sub(pMsgsInFlightForTran, 1);
    TEST_ASSERT_CUNIT(oldVal > 0, ("oldVal was %u\n", oldVal));
}
void finishDiscardRetainedPublisherCommit(int rc, void *handle, void *context)
{
    testPutTranInfo_t *pTranInfo = *(testPutTranInfo_t  **)context;

    //We've finished with this transaction, try another
    pTranInfo->transHandle = NULL;
    pTranInfo->transState = notstarted;

    DEBUG_ONLY uint64_t oldCount = __sync_fetch_and_sub(pTranInfo->pInprogressCount, 1);

    TEST_ASSERT_CUNIT(oldCount > 0, ("oldVal was %u\n", oldCount));
}
void finishDiscardRetainedPublisherCreateTran(int rc, void *handle, void *context)
{
    testPutTranInfo_t *pTranInfo = *(testPutTranInfo_t  **)context;

    //This transaction has finished creation
    pTranInfo->transHandle = handle;
    ismEngine_FullMemoryBarrier();
    pTranInfo->transState = readyforputs;
}


void *publishMessagesForDiscardRetainedTest(void *context)
{
    int32_t rc;
    discardRetainedTestPublisherContext_t *pubContext = (discardRetainedTestPublisherContext_t *)context;

    char tname[20];
    sprintf(tname, "PubThread-%lu",  pubContext->threadNum);
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    ism_engine_threadInit(0);

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;

    volatile uint64_t inprogressOperations = 0;

    testPutTranInfo_t tranInfo[pubContext->transPerPubThread];

    for (uint32_t i =0 ; i< pubContext->transPerPubThread; i++)
    {
        tranInfo[i].transState  = notstarted;
        tranInfo[i].transHandle = NULL;
        tranInfo[i].pInprogressCount = &inprogressOperations;
    }

    char clientId[64];
    sprintf(clientId, "DiscardRetainedPub-%lu", pubContext->threadNum);

    rc = test_createClientAndSession(clientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    uint32_t tranNum      = 0;
    volatile uint32_t msgsInFlightForTran[pubContext->transPerPubThread];
    uint64_t currTopicNum = 0;

    char topicString[256];
    int numChars = sprintf(topicString,"%s/publisher-%lu/topic-",
                           pubContext->topicStringPrefix,
                           pubContext->threadNum);
    char *topicString_topicNum=topicString+numChars;

    for (uint32_t i =0 ; i< pubContext->transPerPubThread; i++)
    {
        msgsInFlightForTran[i] = 0;
    }

    while (*(pubContext->pKeepPublishing))
    {
        //Find a transaction we can use....
        uint64_t transChecked = 0;

        while(     (transChecked < pubContext->transPerPubThread)
                && (tranInfo[tranNum].transState != readyforputs))
        {
            transChecked++;
            tranNum ++;

            if (tranNum >= pubContext->transPerPubThread)
            {
                tranNum = 0;
            }
        }

        if (tranInfo[tranNum].transState == readyforputs)
        {
            ismEngine_FullMemoryBarrier();
            tranInfo[tranNum].transState = putsinprogress;

            msgsInFlightForTran[tranNum] = 0;

            for (int i = 0; i < pubContext->msgsPerTran; i++)
            {
                ismEngine_MessageHandle_t hMessage;
                messageContents_t msgContents;
                messageContents_t *pMsgContents = &msgContents;


                msgContents.msgNum = i;
                msgContents.publisherNum = pubContext->threadNum;
                msgContents.lastMsgsFromThisPublisher = false;

                sprintf(topicString_topicNum, "%lu", currTopicNum);

                currTopicNum++;
                if (pubContext->numTopics >0 && currTopicNum >= pubContext->numTopics)currTopicNum = 0;

                rc = test_createMessage(sizeof(msgContents),
                                        ismMESSAGE_PERSISTENCE_PERSISTENT ,
                                        ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                        ((pubContext->retained) ?
                                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN
                                           :ismMESSAGE_FLAGS_NONE),
                                        pubContext->expiry,
                                        ismDESTINATION_TOPIC, topicString,
                                        &hMessage, (void **)&pMsgContents);
                TEST_ASSERT_EQUAL(rc, OK);


                volatile uint32_t *pMsgsInFlightForTran = &(msgsInFlightForTran[tranNum]);
                __sync_add_and_fetch(pMsgsInFlightForTran, 1);

                pubContext->numMessagesPublishStarted++;

                rc = ism_engine_putMessageOnDestination(
                                           hSession,
                                           ismDESTINATION_TOPIC, topicString,
                                           tranInfo[tranNum].transHandle,
                                           hMessage,
                                           &pMsgsInFlightForTran,
                                           sizeof(pMsgsInFlightForTran),
                                           finishDiscardRetainedPublisherPub);

                if (rc != ISMRC_AsyncCompletion)
                {
                    TEST_ASSERT_EQUAL(rc, OK);
                    finishDiscardRetainedPublisherPub(rc, NULL, &pMsgsInFlightForTran);
                }
            }
            tranInfo[tranNum].transState = waitingforinflightputs;
        }
        else
        {
            //We didn't find a transaction ready for putting... try and create some more

            transChecked = 0;

            while(transChecked < pubContext->transPerPubThread)
            {
                if (tranInfo[tranNum].transState == notstarted)
                {
                    tranInfo[tranNum].transState = creatingtran;
                    testPutTranInfo_t  *pTranInfo = &(tranInfo[tranNum]);
                    rc = ism_engine_createLocalTransaction(hSession,
                                                           &(tranInfo[tranNum].transHandle),
                                                           &pTranInfo, sizeof(pTranInfo),
                                                           finishDiscardRetainedPublisherCreateTran);

                    if (rc != ISMRC_AsyncCompletion)
                    {
                        TEST_ASSERT_EQUAL(rc, OK);
                        finishDiscardRetainedPublisherCreateTran(rc, tranInfo[tranNum].transHandle, &pTranInfo);
                    }
                }
                transChecked++;
                tranNum ++;

                if (tranNum >= pubContext->transPerPubThread)
                {
                    tranNum = 0;
                }
            }
        }

        //See if any transaction can be committed
        uint32_t tranToCheck = tranNum;
        uint32_t transCheckedForCommit = 0;

        while (transCheckedForCommit < pubContext->transPerPubThread )
        {
            if (tranToCheck >= pubContext->transPerPubThread) tranToCheck = 0;

            if(     (tranInfo[tranToCheck].transState == waitingforinflightputs)
                 && (msgsInFlightForTran[tranToCheck] == 0))
            {
                tranInfo[tranToCheck].transState = committing;
                testPutTranInfo_t  *pTranInfo = &(tranInfo[tranToCheck]);

                __sync_add_and_fetch(&inprogressOperations, 1);

                rc = ism_engine_commitTransaction( hSession
                                                 , tranInfo[tranToCheck].transHandle
                                                 , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT
                                                 , &pTranInfo, sizeof(pTranInfo)
                                                 , finishDiscardRetainedPublisherCommit);



                if (rc != ISMRC_AsyncCompletion)
                {
                    TEST_ASSERT_EQUAL(rc, OK);
                    finishDiscardRetainedPublisherCommit(rc, NULL, &pTranInfo);
                }

                //By the time commit has returned, the transaction will no longer be rolled
                //back even if we destroy client state whilst async commit in progress
                pubContext->numMessagesPublishCompleted += pubContext->msgsPerTran;
            }

            transCheckedForCommit++;
            tranToCheck++;
        }
    }

    //Don't rollback our publishes in flight... let's just see what happens...

    rc = sync_ism_engine_destroySession(hSession);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroyClientState(hClient,
                                            ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    //Now we have to wait for the inprogress commits to complete as the context
    //is on the stack of this thread...
    uint64_t counter = 0;

    while (inprogressOperations > 0 && counter < 120 * 1000) //2mins
    {
        usleep(1000);
        counter++;
    }
    TEST_ASSERT_EQUAL(inprogressOperations, 0);

    ism_engine_threadTerm(1);

    return NULL;
}

void startPublishingThreads( uint32_t numThreads
                           , uint32_t numThreadsUsingRetained
                           , char *topicStringPrefix
                           , uint32_t transPerPubThread
                           , uint32_t msgsPerTran
                           , uint32_t numTopicPerPublisher
                           , uint32_t expiry
                           , bool *pKeepPublishing
                           , discardRetainedTestPublisherContext_t pubberContext[numThreads])
{
    int rc = 0;

    for (uint32_t threadNum = 0; threadNum < numThreads; threadNum++)
    {
        pubberContext[threadNum].threadNum                   = threadNum;
        pubberContext[threadNum].pKeepPublishing             = pKeepPublishing;
        pubberContext[threadNum].retained                    = (threadNum < numThreadsUsingRetained);
        pubberContext[threadNum].transPerPubThread           = transPerPubThread;
        pubberContext[threadNum].msgsPerTran                 = msgsPerTran;
        pubberContext[threadNum].numTopics                   = numTopicPerPublisher;
        pubberContext[threadNum].expiry                      = expiry;
        pubberContext[threadNum].numMessagesPublishStarted   = 0;
        pubberContext[threadNum].numMessagesPublishCompleted = 0;
        pubberContext[threadNum].topicStringPrefix           = topicStringPrefix;


        rc = test_task_startThread(&(pubberContext[threadNum].threadId),publishMessagesForDiscardRetainedTest, (void *)&(pubberContext[threadNum]),"publishMessagesForDiscardRetainedTest");
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

static bool DiscardRetainedConsumerFunction(
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
    discardRetainedTestConsumerContext_t *pConsContext = *(discardRetainedTestConsumerContext_t **)pConsumerContext;

    pConsContext->msgsDelivered++;

    if (state != ismMESSAGE_STATE_CONSUMED && pConsContext->ackingThreadId != 0)
    {
        addMessageToAckList( &(pConsContext->ackContext),
                             hDelivery,
                             state);
    }

    ism_engine_releaseMessage(hMessage);

    bool wantMoreMessages = true;

    if (pConsContext->disableConsumerEveryNMsgs > 0)
    {
        wantMoreMessages = ((rand() % pConsContext->disableConsumerEveryNMsgs) == 0);
    }

    return wantMoreMessages;
}

typedef struct test_checkCurrentRetainedContext_t
{
    iemqQNode_t *pNode;
    uint8_t  *pFlags;
    uint8_t   flagsOut;
} test_checkCurrentRetainedContext_t;

void test_checkWhetherCurrentRetained(void *context)
{
    test_checkCurrentRetainedContext_t *retContext = (test_checkCurrentRetainedContext_t *)context;

    if(retContext->pNode->msgState == ismMESSAGE_STATE_AVAILABLE)
    {
        retContext->flagsOut = *(retContext->pFlags);
    }
    else
    {
        //message in process of delivery/acking/discarding - can't look
        retContext->flagsOut = 0;
    }
}

//Counts messages marked as the current retained for their topic on the fake
//forwarding queue
void countCurrentRetainedMessages( iemqQueue_t *Q
                                 , uint64_t expectedMinCurrentRetained
                                 , uint64_t expectedMaxCurrentRetained)
{
    int os_rc = pthread_rwlock_rdlock(&(Q->headlock));
    TEST_ASSERT_EQUAL(os_rc, 0);

    os_rc = pthread_mutex_lock(&(Q->getlock));
    TEST_ASSERT_EQUAL(os_rc, 0);

    iemqQNodePage_t *page = Q->headPage;
    uint64_t countCurrRetained = 0;
    uint64_t countNodesWithMsgs = 0;
    ielmLockName_t LockName = { .Msg = { ielmLOCK_TYPE_MESSAGE, Q->qId,  0 } };

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    while(page != NULL)
    {
        for (uint32_t i=0; i<page->nodesInPage; i++)
        {
            if (page->nodes[i].msg != NULL)
            {
                LockName.Msg.NodeId = page->nodes[i].orderId;
                uint64_t counter = 0;

                test_checkCurrentRetainedContext_t context = {
                         &(page->nodes[i])
                       , &(page->nodes[i].msg->Header.Flags)
                       , 0
                };

                int32_t rc;

                do
                {
                    rc = ielm_instantLockWithCallback( pThreadData
                            , &LockName
                            , true
                            , test_checkWhetherCurrentRetained
                            , &context);

                    if (rc == ISMRC_LockNotGranted)
                    {
                        usleep(1000);
                    }
                    counter++;
                }
                while(rc == ISMRC_LockNotGranted && counter < 900*1000); //15 mins

                TEST_ASSERT_EQUAL(rc, OK);

                if((context.flagsOut & ismMESSAGE_FLAGS_PROPAGATE_RETAINED) != 0)
                {
                    if (countCurrRetained == 0)
                    {
                        test_log(testLOGLEVEL_CHECK, "oId of first message that is still current retained is %lu\n",
                                          page->nodes[i].orderId );
                    }
                    countCurrRetained++;
                }
                countNodesWithMsgs++;
            }
        }

        page = page->next;
    }

    os_rc = pthread_mutex_unlock(&(Q->getlock));
    TEST_ASSERT_EQUAL(os_rc, 0);

    os_rc = pthread_rwlock_unlock(&(Q->headlock));
    TEST_ASSERT_EQUAL(os_rc, 0);

    test_log(testLOGLEVEL_CHECK, "%lu messages (out of %lu) marked as current retained\n",
                                          countCurrRetained, countNodesWithMsgs );

    TEST_ASSERT_CUNIT(   ((expectedMaxCurrentRetained == 0)
                      ||  (countCurrRetained <= expectedMaxCurrentRetained)),
                      ("msgs flagged as current retained: %lu (expected no more than %lu)\n",
                              countCurrRetained, expectedMaxCurrentRetained));

    TEST_ASSERT_CUNIT( expectedMinCurrentRetained <= countCurrRetained,
                      ("msgs flagged as current retained: %lu (expected no fewer than %lu)\n",
                              countCurrRetained, expectedMinCurrentRetained));
}

//check there are no available messages before the get cursor
//Called at the end of tests the only activity that ought to be remaining is the async discarding
//of messages
void checkForTrappedMsgs( iemqQueue_t *Q)
{
    int os_rc = pthread_rwlock_rdlock(&(Q->headlock));
    TEST_ASSERT_EQUAL(os_rc, 0);

    os_rc = pthread_mutex_lock(&(Q->getlock));
    TEST_ASSERT_EQUAL(os_rc, 0);

    iemqQNodePage_t *page = Q->headPage;
    uint64_t earliestAvailOId = 0;

    while(    (page != NULL)
           && (earliestAvailOId == 0)
           && (page->nodes[0].orderId < Q->getCursor.c.orderId))
    {
        for (uint32_t i=0; i<page->nodesInPage; i++)
        {
            if (page->nodes[i].msg != NULL && page->nodes[i].msgState == ismMESSAGE_STATE_AVAILABLE)
            {
                TEST_ASSERT_EQUAL(earliestAvailOId, 0);
                earliestAvailOId = page->nodes[i].orderId;
                break;
            }
        }
        page = page->next;
    }

    os_rc = pthread_mutex_unlock(&(Q->getlock));
    TEST_ASSERT_EQUAL(os_rc, 0);

    os_rc = pthread_rwlock_unlock(&(Q->headlock));
    TEST_ASSERT_EQUAL(os_rc, 0);

    test_log(testLOGLEVEL_CHECK, "Earliest available msg: %lu, getCursor: %lu\n",
                                     earliestAvailOId    , Q->getCursor.c.orderId );

    TEST_ASSERT_CUNIT( (earliestAvailOId == 0) || (earliestAvailOId >= Q->getCursor.c.orderId),
                      ("Msgs trapped on queue!"));
}

//Reuses code written for testing the non-discarding of messages on remote server queues to
//stress discard code
void test_discardFlowParameters( uint64_t maxDepth
                               , uint32_t numConsumers
                               , uint32_t numBadAckers //Consumers who say they'll ack...but don't!
                               , uint32_t numPublisherThreads
                               , uint32_t numPublisherThreadsPublishingRetained
                               , uint32_t transPerPubThread
                               , uint32_t msgsPerTran
                               , uint64_t minDiscardedMessages
                               , uint32_t disableconsumerEveryNMsgs
                               , ExpectedMsgRate_t consumerExpectedMsgRate
                               , uint32_t maxInflightLimit
                               , uint64_t numMsgsForDisconnectedMQTTClient
                               )
{
    test_log(testLOGLEVEL_TESTNAME,"Starting %s\n", __func__);

    char *subName     = "subQueue";
    char *topicString = "/test/discardFlow/#";
    char *clientId = "discardFlow";

    int rc = OK;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_SessionHandle_t hBadAckersSession;
    ismEngine_ClientStateHandle_t hDisconnectedMQTTClient;
    ismEngine_SessionHandle_t hDisconnectedMQTTSession;
    uint64_t messageCountLimit=maxDepth;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_DURABLE };

    if (numConsumers > 1 || numMsgsForDisconnectedMQTTClient > 0)
    {
        subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_SHARED;
    }

    iepi_getDefaultPolicyInfo(false)->maxMessageCount = messageCountLimit;
    iepi_getDefaultPolicyInfo(false)->maxMsgBehavior = DiscardOldMessages;

    /* Create our clients and sessions */
    rc = test_createClientAndSession(clientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    hClient->expectedMsgRate = consumerExpectedMsgRate;
    hClient->maxInflightLimit = maxInflightLimit;

    if (numBadAckers > 0)
    {
        rc = ism_engine_createSession(hClient,
                           ismENGINE_CREATE_SESSION_OPTION_NONE,
                           &hBadAckersSession,
                           NULL, 0, NULL);

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hBadAckersSession);

        rc = ism_engine_startMessageDelivery(hBadAckersSession,
                                             ismENGINE_START_DELIVERY_OPTION_NONE,
                                             NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    if (  numMsgsForDisconnectedMQTTClient > 0  )
    {
        /* Create our clients and sessions */
        rc = test_createClientAndSession("disconnectedMQTT",
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hDisconnectedMQTTClient, &hDisconnectedMQTTSession, true);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hDisconnectedMQTTClient);
        TEST_ASSERT_PTR_NOT_NULL(hDisconnectedMQTTSession);

        hDisconnectedMQTTClient->expectedMsgRate = EXPECTEDMSGRATE_MAX;
        hDisconnectedMQTTClient->maxInflightLimit = 65000;
    }



    rc = sync_ism_engine_createSubscription(
                hClient,
                subName,
                NULL,
                ismDESTINATION_TOPIC,
                topicString,
                &subAttrs,
                NULL); // Owning client same as requesting client
        TEST_ASSERT_EQUAL(rc, OK);

    //Get the queueHandle...
    ismEngine_Subscription_t *pSub = NULL;

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    rc = iett_findClientSubscription(pThreadData,
                                     clientId,
                                     iett_generateClientIdHash(clientId),
                                     subName,
                                     iett_generateSubNameHash(subName),
                                     &pSub);
    TEST_ASSERT_EQUAL(rc, OK);
    iemqQueue_t *Q = (iemqQueue_t *)(pSub->queueHandle);

    //Set up the acking consumers
    discardRetainedTestConsumerContext_t consContext[numConsumers];

    ismEngine_ConsumerHandle_t hConsumer[numConsumers];

    for (uint32_t consNum = 0 ;consNum < numConsumers; consNum++)
    {
        consContext[consNum].ackContext.hSession = hSession;
        consContext[consNum].msgsDelivered = 0;
        consContext[consNum].disableConsumerEveryNMsgs = disableconsumerEveryNMsgs;
        int os_rc = pthread_mutex_init(&(consContext[consNum].ackContext.msgListMutex), NULL);
        TEST_ASSERT_EQUAL(os_rc, 0);
        os_rc = pthread_cond_init(&(consContext[consNum].ackContext.msgListCond), NULL);
        TEST_ASSERT_EQUAL(os_rc, 0);

        consContext[consNum].ackContext.firstMsg  = NULL;
        consContext[consNum].ackContext.lastMsg   = NULL;
        ismEngine_SetStructId(consContext[consNum].ackContext.StructId, ACKERCONTEXT_STRUCTID);
        consContext[consNum].ackContext.ackQos = 3; //JMS
        consContext[consNum].ackContext.acksCompleted = 0;
        consContext[consNum].ackContext.acksStarted   = 0;
        consContext[consNum].ackContext.stopAcking    = false;

        if (consNum >= numBadAckers)
        {
            os_rc = test_task_startThread(&(consContext[consNum].ackingThreadId),AckingThread, (void *)(&(consContext[consNum].ackContext)),"AckingThread");
            TEST_ASSERT_EQUAL(os_rc, 0);
        }
        else
        {
            consContext[consNum].ackingThreadId = 0;
        }

        discardRetainedTestConsumerContext_t *pConsContext = &(consContext[consNum]);

        rc = ism_engine_createConsumer( (consNum < numBadAckers)? hBadAckersSession : hSession
                                     , ismDESTINATION_SUBSCRIPTION
                                     ,  subName
                                     , NULL
                                     , NULL
                                     , &pConsContext
                                     , sizeof(pConsContext)
                                     , DiscardRetainedConsumerFunction
                                     , NULL
                                     , ismENGINE_CONSUMER_OPTION_ACK
                                     , &(hConsumer[consNum])
                                     , NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hConsumer[consNum]);
    }

    discardRetainedTestConsumerContext_t mqttConsContext;
    ismEngine_ConsumerHandle_t hMQTTConsumer;
    if (  numMsgsForDisconnectedMQTTClient > 0  )
    {
        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                            | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                            | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                            | ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;

        rc = ism_engine_reuseSubscription(hDisconnectedMQTTClient,
            subName,
            &subAttrs,
            hClient);

        TEST_ASSERT_EQUAL(rc, OK);

        mqttConsContext.msgsDelivered = 0;
        mqttConsContext.disableConsumerEveryNMsgs = 0;
        mqttConsContext.ackingThreadId = 0;

        discardRetainedTestConsumerContext_t *pConsContext = &(mqttConsContext);

        rc = ism_engine_createConsumer( hDisconnectedMQTTSession
                                     , ismDESTINATION_SUBSCRIPTION
                                     ,  subName
                                     ,  ismENGINE_SUBSCRIPTION_OPTION_NONE
                                     , NULL
                                     , &pConsContext
                                     , sizeof(pConsContext)
                                     , DiscardRetainedConsumerFunction
                                     , NULL
                                     , ismENGINE_CONSUMER_OPTION_ACK
                                      |ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID
                                     , &(hMQTTConsumer)
                                     , NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMQTTConsumer);
    }

    //Start publishing messages
    uint32_t numTopicsPerPublisher = 30;
    bool keepPublishing = true;

    discardRetainedTestPublisherContext_t pubberContext[numPublisherThreads];

    startPublishingThreads( numPublisherThreads
                          , numPublisherThreadsPublishingRetained
                          , "/test/discardFlow"
                          , transPerPubThread
                          , msgsPerTran
                          , numTopicsPerPublisher
                          , 0 //No Expiry
                          , &keepPublishing
                          , pubberContext );


    //Wait for some number of messages to be discarded...
    //...occassionally restarting the consumer
    ismEngine_QueueStatistics_t stats;

    uint64_t publishesStarted;
    uint64_t LastReportedPublishesStarted = 0;
    uint64_t totalPublished;
    uint64_t totalAcked;
    uint64_t totalDeliveredToBadAckers;

    bool mqttClientStillConnected = true;

    do
    {
       publishesStarted          = 0;
       totalPublished            = 0;
       totalAcked                = 0;
       totalDeliveredToBadAckers = 0;

       ieq_getStats(pThreadData, pSub->queueHandle, &stats);

       for (uint32_t consNum = 0 ; consNum < numConsumers; consNum++)
       {
           totalAcked += consContext[consNum].ackContext.acksCompleted;

           if (consNum < numBadAckers)
           {
               totalDeliveredToBadAckers += consContext[consNum].msgsDelivered;
           }
       }

       if( disableconsumerEveryNMsgs > 0 && totalAcked < stats.DiscardedMsgs)
       {
           rc = ism_engine_startMessageDelivery(hSession,
                                                ismENGINE_START_DELIVERY_OPTION_NONE,
                                                NULL, 0, NULL);
           TEST_ASSERT_EQUAL(rc, OK);

           if (numBadAckers > 0 && totalDeliveredToBadAckers < 0.75 * maxDepth)
           {
               rc = ism_engine_startMessageDelivery(hBadAckersSession,
                                                    ismENGINE_START_DELIVERY_OPTION_NONE,
                                                    NULL, 0, NULL);
               TEST_ASSERT_EQUAL(rc, OK);
           }
       }

       if (    (numMsgsForDisconnectedMQTTClient > 0 )
            && (numMsgsForDisconnectedMQTTClient <= mqttConsContext.msgsDelivered)
            && mqttClientStillConnected)
       {
           rc = test_destroyClientAndSession(hDisconnectedMQTTClient, hDisconnectedMQTTSession, false);
           TEST_ASSERT_EQUAL(rc, OK);
           mqttClientStillConnected = false;
       }


       for (uint32_t publisherNum = 0 ; publisherNum < numPublisherThreads; publisherNum++)
       {
           publishesStarted += pubberContext[publisherNum].numMessagesPublishStarted;
           totalPublished   += pubberContext[publisherNum].numMessagesPublishCompleted;
       }

       if (publishesStarted > 1000 + LastReportedPublishesStarted)
       {
           test_log(testLOGLEVEL_CHECK,
                     "Published %lu (started %lu), Acked %lu, Buffered %lu, Discarded %lu DeliveredToBadAckers %lu DeliveredTo%sMQTTConsumer %lu\n",
                           totalPublished, publishesStarted, totalAcked,
                           stats.BufferedMsgs, stats.DiscardedMsgs,
                           totalDeliveredToBadAckers,
                           mqttClientStillConnected ? "Connected" : "Disconnected",
                           mqttConsContext.msgsDelivered);
           LastReportedPublishesStarted = publishesStarted;
       }
    }
    while(   (stats.DiscardedMsgs < minDiscardedMessages)
          || (      (numMsgsForDisconnectedMQTTClient != 0 )
                &&  mqttClientStillConnected));

    //Stop the publishers and join their threads...
    keepPublishing = false;

    for (uint64_t i=0; i < numPublisherThreads; i++)
    {
        rc = pthread_join(pubberContext[i].threadId, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Delete Consumer(s)
    for (uint32_t consNum = 0 ; consNum < numConsumers; consNum++)
    {
        rc = sync_ism_engine_destroyConsumer(hConsumer[consNum]);
        TEST_ASSERT_EQUAL(rc, OK);

    }

    for (uint32_t consNum = numBadAckers ; consNum < numConsumers; consNum++)
    {
        //Stop the acking thread
        int32_t os_rc = pthread_mutex_lock(&(consContext[consNum].ackContext.msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        consContext[consNum].ackContext.stopAcking = true;

        //Broadcast that the flag changed state
        os_rc = pthread_cond_broadcast(&(consContext[consNum].ackContext.msgListCond));
        TEST_ASSERT_EQUAL(os_rc, 0);

        //Let the processing thread access the list of messages...
        os_rc = pthread_mutex_unlock(&(consContext[consNum].ackContext.msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        rc = pthread_join(consContext[consNum].ackingThreadId, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Although we have waited for the publisher and acker threads to have finished,
    //due to the async nature of the engine, there can still be messages in the process
    //of being discard... so if something looks dodgy, wait a bit and see if it fixes
    //itself

    uint64_t numLoops = 0;

    do
    {
        publishesStarted          = 0;
        totalPublished            = 0;
        totalAcked                = 0;
        totalDeliveredToBadAckers = 0;

        //Check that the msgs received + num discard is same as number put
        ieq_getStats(pThreadData, pSub->queueHandle, &stats);

        for (uint32_t publisherNum = 0 ; publisherNum < numPublisherThreads; publisherNum++)
        {
            publishesStarted += pubberContext[publisherNum].numMessagesPublishStarted;
            totalPublished   += pubberContext[publisherNum].numMessagesPublishCompleted;
        }
        for (uint32_t consNum = 0 ; consNum < numConsumers; consNum++)
        {
            totalAcked += consContext[consNum].ackContext.acksCompleted;

            if (consNum < numBadAckers)
            {
                totalDeliveredToBadAckers += consContext[consNum].msgsDelivered;
            }
        }

        test_log(testLOGLEVEL_CHECK,
                 "Published %lu (started %lu), Acked %lu, Buffered %lu, Discarded %lu DeliveredToBadAckers %lu\n",
                                      totalPublished, publishesStarted, totalAcked,
                                      stats.BufferedMsgs, stats.DiscardedMsgs,
                                      totalDeliveredToBadAckers);

        if (totalPublished != totalAcked + stats.BufferedMsgs +stats.DiscardedMsgs)
        {
            usleep(50000); //50ms;
        }
        numLoops++;
    }
    while( (totalPublished != totalAcked + stats.BufferedMsgs +stats.DiscardedMsgs)
            && (numLoops < 18000)); //If each loop = 50ms then this = 15minutes

    TEST_ASSERT_EQUAL( totalPublished
                     , totalAcked + stats.BufferedMsgs +stats.DiscardedMsgs);

    checkForTrappedMsgs(Q);
    TEST_ASSERT_CUNIT(stats.BufferedMsgs < 1.2 * messageCountLimit,
                      ("Buffered Msgs %lu, limit %lu\n",stats.BufferedMsgs,  messageCountLimit));

    iett_releaseSubscription(pThreadData, pSub, false);

    rc = ism_engine_destroySubscription(hClient, subName, hClient,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_discardFlow(void)
{
    test_discardFlowParameters( 500 //MaxDepth
                              , 1   //Num Consumers
                              , 0   //Num Bad Ackers
                              , 5   //Num Publisher Threads
                              , 1   //Publisher threads publishing retained
                              , 100 //Transactions per publishing thread
                              , 20  //Messages per transaction
                              , 10000 //Min discarded Messages
                              , 8    //disableconsumerEveryNMsgs
                              , EXPECTEDMSGRATE_DEFAULT //expected msg rate
                              , 128   //max inflight
                              , 0     //numMsgsForDisconnectedMQTTClient
                              );
}


void test_discardNastyFlow(void)
{
    test_discardFlowParameters( 50000 //MaxDepth
                              , 4   //Num Consumers
                              , 2   //Num Bad Ackers
                              , 8   //Num Publisher Threads
                              , 1   //Publisher threads publishing retained
                              , 1000 //Transactions per publishing thread
                              , 20  //Messages per transaction
                              , 5000000 //Min discarded Messages - enough that there should be full page scans
                              , 0  //disableconsumerEveryNMsgs
                              , EXPECTEDMSGRATE_MAX
                              , 60000 //max inflight
                              , 9670 //numMsgsForDisconnectedMQTTClient
                              );
}

void test_checkRemSrvDiscardRetained(void)
{
    test_log(testLOGLEVEL_TESTNAME,"Starting %s\n", __func__);

    char *subName     = "fakeRemoteServerQueue";
    char *topicString = "/test/discardRetain/#";
    char *fakeRemServerClientId = "fakeRemoteServerClient";
    int rc = OK;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_DURABLE };

    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 0;
    iepi_getDefaultPolicyInfo(false)->maxMessageBytes = 1000;
    iepi_getDefaultPolicyInfo(false)->maxMsgBehavior = DiscardOldMessages;

    /* Create our clients and sessions */
    rc = test_createClientAndSession(fakeRemServerClientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);


    rc = sync_ism_engine_createSubscription(
                hClient,
                subName,
                NULL,
                ismDESTINATION_TOPIC,
                topicString,
                &subAttrs,
                NULL); // Owning client same as requesting client
        TEST_ASSERT_EQUAL(rc, OK);

    //Get the queueHandle...
    ismEngine_Subscription_t *pSub = NULL;

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    rc = iett_findClientSubscription(pThreadData,
                                     fakeRemServerClientId,
                                     iett_generateClientIdHash(fakeRemServerClientId),
                                     subName,
                                     iett_generateSubNameHash(subName),
                                     &pSub);
    TEST_ASSERT_EQUAL(rc, OK);

    //Change the sub queue to be a fake remoteserver queue...
    iemqQueue_t *Q = (iemqQueue_t *)(pSub->queueHandle);
    Q->QOptions |= ieqOptions_REMOTE_SERVER_QUEUE;

    //Set up the acking consumers
    uint32_t numConsumers = 1;
    discardRetainedTestConsumerContext_t consContext[numConsumers];

    ismEngine_ConsumerHandle_t hConsumer[numConsumers];

    for (uint32_t consNum = 0 ;consNum < numConsumers; consNum++)
    {
        consContext[consNum].ackContext.hSession = hSession;
        consContext[consNum].msgsDelivered = 0;
        consContext[consNum].disableConsumerEveryNMsgs = 0;
        int os_rc = pthread_mutex_init(&(consContext[consNum].ackContext.msgListMutex), NULL);
        TEST_ASSERT_EQUAL(os_rc, 0);
        os_rc = pthread_cond_init(&(consContext[consNum].ackContext.msgListCond), NULL);
        TEST_ASSERT_EQUAL(os_rc, 0);

        consContext[consNum].ackContext.firstMsg  = NULL;
        consContext[consNum].ackContext.lastMsg   = NULL;
        ismEngine_SetStructId(consContext[consNum].ackContext.StructId, ACKERCONTEXT_STRUCTID);
        consContext[consNum].ackContext.ackQos = 3; //JMS
        consContext[consNum].ackContext.acksCompleted = 0;
        consContext[consNum].ackContext.acksStarted   = 0;
        consContext[consNum].ackContext.stopAcking    = false;

        os_rc = test_task_startThread(&(consContext[consNum].ackingThreadId),AckingThread, (void *)(&(consContext[consNum].ackContext)),"AckingThread");
        TEST_ASSERT_EQUAL(os_rc, 0);

        discardRetainedTestConsumerContext_t *pConsContext = &(consContext[consNum]);

        rc = ism_engine_createConsumer( hSession
                                     , ismDESTINATION_SUBSCRIPTION
                                     ,  subName
                                     , NULL
                                     , NULL
                                     , &pConsContext
                                     , sizeof(pConsContext)
                                     , DiscardRetainedConsumerFunction
                                     , NULL
                                     , ismENGINE_CONSUMER_OPTION_ACK
                                     , &(hConsumer[consNum])
                                     , NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hConsumer[consNum]);
    }

    //Start publishing messages
    uint32_t numPublisherThreads   = 5;
    uint32_t numPublisherThreadsPublishingRetained = 3;
    uint32_t transPerPubThread     = 100;
    uint32_t msgsPerTran           = 20;
    uint32_t numTopicsPerPublisher = 30;
    bool keepPublishing = true;

    discardRetainedTestPublisherContext_t pubberContext[numPublisherThreads];

    startPublishingThreads( numPublisherThreads
                          , numPublisherThreadsPublishingRetained
                          , "/test/discardRetain"
                          , transPerPubThread
                          , msgsPerTran
                          , numTopicsPerPublisher
                          , 0 //No Expiry
                          , &keepPublishing
                          , pubberContext );


    //Wait for some number of messages to be discarded...
    //...occassionally restarting the consumer
    ismEngine_QueueStatistics_t stats;

    do
    {
       ieq_getStats(pThreadData, pSub->queueHandle, &stats);
       uint64_t totalAcked = 0;

       for (uint32_t consNum = 0 ; consNum < numConsumers; consNum++)
       {
           totalAcked += consContext[consNum].ackContext.acksCompleted;
       }

       if( totalAcked < stats.DiscardedMsgs)
       {
           rc = ism_engine_startMessageDelivery(hSession,
                                                ismENGINE_START_DELIVERY_OPTION_NONE,
                                                NULL, 0, NULL);
       }
       TEST_ASSERT_EQUAL(rc, OK);
    }
    while(stats.DiscardedMsgs < 10000);

    //Stop the publishers and join their threads...
    keepPublishing = false;

    for (uint64_t i=0; i < numPublisherThreads; i++)
    {
        rc = pthread_join(pubberContext[i].threadId, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Delete Consumer(s)
    for (uint32_t consNum = 0 ; consNum < numConsumers; consNum++)
    {
        rc = sync_ism_engine_destroyConsumer(hConsumer[consNum]);
        TEST_ASSERT_EQUAL(rc, OK);

    }

    for (uint32_t consNum = 0 ; consNum < numConsumers; consNum++)
    {
        //Stop the acking thread
        int32_t os_rc = pthread_mutex_lock(&(consContext[consNum].ackContext.msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        consContext[consNum].ackContext.stopAcking = true;

        //Broadcast that the flag changed state
        os_rc = pthread_cond_broadcast(&(consContext[consNum].ackContext.msgListCond));
        TEST_ASSERT_EQUAL(os_rc, 0);

        //Let the processing thread access the list of messages...
        os_rc = pthread_mutex_unlock(&(consContext[consNum].ackContext.msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        rc = pthread_join(consContext[consNum].ackingThreadId, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Check that the msgs received + num discard is same as number put

    //Although we have waited for the publisher and acker threads to have finished,
    //due to the async nature of the engine, there can still be messages in the process
    //of being discarded... so if something looks dodgy, wait a bit and see if it fixes
    //itself

    uint64_t publishesStarted;
    uint64_t totalPublished;
    uint64_t totalAcked;

    uint64_t numLoops = 0;
    do
    {
        publishesStarted = 0;
        totalPublished   = 0;
        totalAcked       = 0;

        //Check that the msgs received + num discard is same as number put
        ieq_getStats(pThreadData, pSub->queueHandle, &stats);

        for (uint32_t publisherNum = 0 ; publisherNum < numPublisherThreads; publisherNum++)
        {
            publishesStarted += pubberContext[publisherNum].numMessagesPublishStarted;
            totalPublished   += pubberContext[publisherNum].numMessagesPublishCompleted;
        }
        for (uint32_t consNum = 0 ; consNum < numConsumers; consNum++)
        {
            totalAcked += consContext[consNum].ackContext.acksCompleted;
        }

        test_log(testLOGLEVEL_CHECK,
                  "Published %lu (started %lu), Acked %lu, Buffered %lu, Discarded %lu\n",
                        totalPublished, publishesStarted, totalAcked,
                        stats.BufferedMsgs, stats.DiscardedMsgs);

        if (totalPublished != totalAcked + stats.BufferedMsgs +stats.DiscardedMsgs)
        {
            usleep(50000); //50ms;
        }
        numLoops++;
    }
    while( (totalPublished != totalAcked + stats.BufferedMsgs +stats.DiscardedMsgs)
            && (numLoops < 18000)); //If each loop = 50ms then this = 15minutes

    TEST_ASSERT_EQUAL( totalPublished
                     , totalAcked + stats.BufferedMsgs +stats.DiscardedMsgs);

    checkForTrappedMsgs(Q);
    countCurrentRetainedMessages(Q,
            0,
            numPublisherThreadsPublishingRetained*numTopicsPerPublisher);

    iett_releaseSubscription(pThreadData, pSub, false);

    rc = ism_engine_destroySubscription(hClient, subName, hClient,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

//Have a thread publishing retained, and another publishing non-retained
//(each retained message is on a unique topic). Check that all the retained messages are still
//on the queue)
void test_checkRemSrvDontDiscardCurrentRetained(void)
{
    test_log(testLOGLEVEL_TESTNAME,"Starting %s\n", __func__);

    char *subName     = "fakeRemoteServerQueue2";
    char *topicString = "/test/dontdiscardCurrentRetain/#";
    char *fakeRemServerClientId = "fakeRemoteServerClient2";
    int rc = OK;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_DURABLE };

    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 0;
    iepi_getDefaultPolicyInfo(false)->maxMessageBytes = 1000;
    iepi_getDefaultPolicyInfo(false)->maxMsgBehavior = DiscardOldMessages;

    /* Create our clients and sessions */
    rc = test_createClientAndSession(fakeRemServerClientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);


    rc = sync_ism_engine_createSubscription(
                hClient,
                subName,
                NULL,
                ismDESTINATION_TOPIC,
                topicString,
                &subAttrs,
                NULL); // Owning client same as requesting client
        TEST_ASSERT_EQUAL(rc, OK);

    //Get the queueHandle...
    ismEngine_Subscription_t *pSub = NULL;

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    rc = iett_findClientSubscription(pThreadData,
                                     fakeRemServerClientId,
                                     iett_generateClientIdHash(fakeRemServerClientId),
                                     subName,
                                     iett_generateSubNameHash(subName),
                                     &pSub);
    TEST_ASSERT_EQUAL(rc, OK);

    //Change the sub queue to be a fake remoteserver queue...
    iemqQueue_t *Q = (iemqQueue_t *)(pSub->queueHandle);
    Q->QOptions |= ieqOptions_REMOTE_SERVER_QUEUE;


    //Start publishing messages
    uint32_t numPublisherThreads   = 5;
    uint32_t numPublisherThreadsPublishingRetained = 3;
    uint32_t transPerPubThread     = 50;
    uint32_t msgsPerTran           = 20;
    uint32_t numTopicsPerPublisher = 0;
    bool keepPublishing = true;

    discardRetainedTestPublisherContext_t pubberContext[numPublisherThreads];

    startPublishingThreads( numPublisherThreads
                          , numPublisherThreadsPublishingRetained
                          , "/test/dontdiscardCurrentRetain"
                          , transPerPubThread
                          , msgsPerTran
                          , numTopicsPerPublisher
                          , 0 //No Expiry
                          , &keepPublishing
                          , pubberContext );


    //Wait for some number of messages to be discarded...
    ismEngine_QueueStatistics_t stats;

    do
    {
       usleep(100);
       ieq_getStats(pThreadData, pSub->queueHandle, &stats);
    }
    while(stats.DiscardedMsgs < 1000);

    //Stop the publishers and join their threads...
    keepPublishing = false;

    for (uint64_t i=0; i < numPublisherThreads; i++)
    {
        rc = pthread_join(pubberContext[i].threadId, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }


    //Check that the msgs received + num discard is same as number put

    //Although we have waited for the publisher and acker threads to have finished,
    //due to the async nature of the engine, there can still be messages in the process
    //of being discarded... so if something looks dodgy, wait a bit and see if it fixes
    //itself

    uint64_t publishesStarted;
    uint64_t totalPublished;
    uint64_t totalRetainedPublished = 0;

    uint64_t numLoops = 0;
    do
    {
        publishesStarted        = 0;
        totalPublished          = 0;
        totalRetainedPublished  = 0;

        //Check that the msgs received + num discard is same as number put
        ieq_getStats(pThreadData, pSub->queueHandle, &stats);

        for (uint32_t publisherNum = 0 ; publisherNum < numPublisherThreads; publisherNum++)
        {
            publishesStarted += pubberContext[publisherNum].numMessagesPublishStarted;
            totalPublished   += pubberContext[publisherNum].numMessagesPublishCompleted;

            if (publisherNum < numPublisherThreadsPublishingRetained)
            {
                totalRetainedPublished += pubberContext[publisherNum].numMessagesPublishCompleted;
            }
        }


        test_log(testLOGLEVEL_CHECK,
                "Published %lu (started %lu), Buffered %lu, Discarded %lu\n",
                totalPublished, publishesStarted,
                stats.BufferedMsgs, stats.DiscardedMsgs);

        if (totalPublished != stats.BufferedMsgs +stats.DiscardedMsgs)
        {
            usleep(50000); //50ms;
        }
        numLoops++;
    }
    while( (totalPublished != stats.BufferedMsgs +stats.DiscardedMsgs)
            && (numLoops < 18000)); //If each loop = 50ms then this = 15minutes

    TEST_ASSERT_EQUAL( totalPublished
            , stats.BufferedMsgs +stats.DiscardedMsgs);

    checkForTrappedMsgs(Q);

    //Check we  have exactly retained messages than we expect
    countCurrentRetainedMessages(Q,
            totalRetainedPublished,
            totalRetainedPublished);

    iett_releaseSubscription(pThreadData, pSub, false);

    rc = ism_engine_destroySubscription(hClient, subName, hClient,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}


///Test that we don't hang or crash when a sub queue is undergoing expiry scanning
///and discarding at the same time
void test_checkDiscardWithExpiry(uint64_t SubOptions, uint64_t minMsgsToDiscard)
{

    char *subName     = "subDiscardWithExpir";
    char *topicString = "/test/discardWithExpiry/#";
    char *clientId = "discardWithExpiry";

    int rc = OK;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    uint64_t maxQdepth = 5000;
    ismEngine_SubscriptionAttributes_t subAttrs = { SubOptions };

    iepi_getDefaultPolicyInfo(false)->maxMessageCount = maxQdepth;
    iepi_getDefaultPolicyInfo(false)->maxMessageBytes = 0;
    iepi_getDefaultPolicyInfo(false)->maxMsgBehavior = DiscardOldMessages;

    /* Create our clients and sessions */
    rc = test_createClientAndSession(clientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    //hClient->expectedMsgRate = consumerExpectedMsgRate;
    //hClient->maxInflightLimit = maxInflightLimit;

    rc = sync_ism_engine_createSubscription(
                hClient,
                subName,
                NULL,
                ismDESTINATION_TOPIC,
                topicString,
                &subAttrs,
                NULL); // Owning client same as requesting client
        TEST_ASSERT_EQUAL(rc, OK);

    //Get the queueHandle...
    ismEngine_Subscription_t *pSub = NULL;

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    rc = iett_findClientSubscription(pThreadData,
                                     clientId,
                                     iett_generateClientIdHash(clientId),
                                     subName,
                                     iett_generateSubNameHash(subName),
                                     &pSub);
    TEST_ASSERT_EQUAL(rc, OK);

    //Start publishing messages
    bool keepPublishing = true;
    uint32_t numPublisherThreads = 20;
    discardRetainedTestPublisherContext_t pubberContext[numPublisherThreads];

    startPublishingThreads( numPublisherThreads
                          , 0
                          , "/test/discardWithExpiry"
                          , 30
                          , 1
                          , 1
                          , ism_common_nowExpire() + 500000
                          , &keepPublishing
                          , pubberContext );


    //Wait for some number of messages to be discarded...
    //...occassionally restarting the consumer
    ismEngine_QueueStatistics_t stats;

    do
    {
       ieq_getStats(pThreadData, pSub->queueHandle, &stats);
    }
    while(stats.DiscardedMsgs < minMsgsToDiscard);

    //Stop the publishers and join their threads...
    keepPublishing = false;

    for (uint64_t i=0; i < numPublisherThreads; i++)
    {
        rc = pthread_join(pubberContext[i].threadId, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    TEST_ASSERT_CUNIT(stats.BufferedMsgs < 3.5 * maxQdepth,
                      ("Buffered Msgs %lu, limit %lu\n",stats.BufferedMsgs,  maxQdepth));

    iett_releaseSubscription(pThreadData, pSub, false);

    rc = ism_engine_destroySubscription(hClient, subName, hClient,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_checkDiscardWithExpirySimple(void)
{
    test_log(testLOGLEVEL_TESTNAME,"Starting %s\n", __func__);
    test_checkDiscardWithExpiry(ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE, 50000);
}
void test_checkDiscardWithExpiryInter(void)
{
    test_log(testLOGLEVEL_TESTNAME,"Starting %s\n", __func__);
    test_checkDiscardWithExpiry(ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, 50000);
}
void test_checkDiscardWithExpiryMulti(void)
{
   test_log(testLOGLEVEL_TESTNAME,"Starting %s\n", __func__);
   test_checkDiscardWithExpiry(ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE, 30000);
}

CU_TestInfo ISM_DiscardOld_CUnit_test_DeterministicCheck[] =
{
    { "qos0NDCheckDiscard",          test_qos0NDCheckDiscard },
    { "qos0DCheckDiscard",           test_qos0DCheckDiscard },
    { "qos1CheckDiscard",            test_qos1CheckDiscard },
    { "qos2CheckDiscard",            test_qos2CheckDiscard },
    { "JMSCheckDiscard",             test_JMSCheckDiscard },
    { "qos0NondurableConsumeDuring", test_qos0NondurableConsumeDuring },
    { "qos0DurableConsumeDuring",    test_qos0DurableConsumeDuring },
    { "qos1DurableConsumeDuring",    test_qos1DurableConsumeDuring },
    { "qos2DurableConsumeDuring",    test_qos2DurableConsumeDuring },
    { "jmsDurableConsumeDuring",     test_jmsDurableConsumeDuring },
    { "simpleSharedDurableConsumeDuring", test_simpleSharedDurableConsumeDuring },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_DiscardOld_CUnit_test_MsgFlow[] =
{
    { "testDiscardFlow",          test_discardFlow },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_DiscardOld_CUnit_test_NastyMsgFlow[] =
{
    { "testNastyDiscardFlow",          test_discardNastyFlow },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_DiscardOld_CUnit_test_remSrvDiscardRetained[] =
{
    { "checkRemSrvDiscardRetained",            test_checkRemSrvDiscardRetained },
    { "checkRemSrvDontDiscardCurrentRetained", test_checkRemSrvDontDiscardCurrentRetained },
    CU_TEST_INFO_NULL
};
CU_TestInfo ISM_DiscardOld_CUnit_test_discardWithExpiry[] =
{
    { "checkDiscardWithExpirySimple",  test_checkDiscardWithExpirySimple},
    { "checkDiscardWithExpiryInter",   test_checkDiscardWithExpiryInter},
    { "checkDiscardWithExpiryMulti",   test_checkDiscardWithExpiryMulti},
    CU_TEST_INFO_NULL
};
int initTestSuite(void)
{
    return test_engineInit(true, true,
                           true, // Disable Auto creation of named queues
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termTestSuite(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_DiscardOld_CUnit_defaultsuites[] =
{
    IMA_TEST_SUITE("DeterministicCheck",     initTestSuite, termTestSuite, ISM_DiscardOld_CUnit_test_DeterministicCheck),
    IMA_TEST_SUITE("MsgFlow",                initTestSuite, termTestSuite, ISM_DiscardOld_CUnit_test_MsgFlow),
    IMA_TEST_SUITE("RemSrvDiscardRetained",  initTestSuite, termTestSuite, ISM_DiscardOld_CUnit_test_remSrvDiscardRetained),
    IMA_TEST_SUITE("DiscardWithExpiry",      initTestSuite, termTestSuite, ISM_DiscardOld_CUnit_test_discardWithExpiry),
    CU_SUITE_INFO_NULL
};

//We only run nasty flow if we are asked to
CU_SuiteInfo ISM_DiscardOld_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("DeterministicCheck",     initTestSuite, termTestSuite, ISM_DiscardOld_CUnit_test_DeterministicCheck),
    IMA_TEST_SUITE("MsgFlow",                initTestSuite, termTestSuite, ISM_DiscardOld_CUnit_test_MsgFlow),
    IMA_TEST_SUITE("NastyMsgFlow",           initTestSuite, termTestSuite, ISM_DiscardOld_CUnit_test_NastyMsgFlow),
    IMA_TEST_SUITE("RemSrvDiscardRetained",  initTestSuite, termTestSuite, ISM_DiscardOld_CUnit_test_remSrvDiscardRetained),
    IMA_TEST_SUITE("DiscardWithExpiry",      initTestSuite, termTestSuite, ISM_DiscardOld_CUnit_test_discardWithExpiry),
    CU_SUITE_INFO_NULL
};

int setup_CU_registry(int argc, char **argv,
                      CU_SuiteInfo *allSuites,
                      CU_SuiteInfo *defaultSuites)
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
            CU_register_suites(defaultSuites);
        }
    }

    if (tempSuites) free(tempSuites);

    return retval;
}

int main(int argc, char *argv[])
{
    int trclvl = 0;
    int retval = 0;
    testLogLevel_t testLogLevel = testLOGLEVEL_CHECK; //testLOGLEVEL_VERBOSE to see order of msg arrival;

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv,
                              ISM_DiscardOld_CUnit_allsuites,
                              ISM_DiscardOld_CUnit_defaultsuites);

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

