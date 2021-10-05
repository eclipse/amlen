/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
#include <stdbool.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>


#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_client.h"
#include "test_utils_initterm.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_task.h"

#include "engine.h"
#include "queueNamespace.h"
#include "clientStateInternal.h" //For asserting num deliveryIds in hMsgDeliveryInfo TODO:remove?

//Create 8 clients each of which makes subscription to 4 shared subs.
// 2 that are going to act like JMS
// For each MQTT qos 2 clients.
//Have threads publishing to shared subs.
// Subbing clients should attempt to keep ~ 125 messages unacked (separate acking thread for each client).
// Subbing clients should occasionally nack messages (a not_delivered ack in case of MQTT).
// Subbing clients should check they get delivery ids (or not) as specified by their sub type.
// Subbing clients disconnect repeatedly and check they get messages redelivered correctly

typedef struct tag_PublisherInfo_t {
    char *topicPrefix;
    uint32_t numTopics;
} PublisherInfo_t;

typedef enum
{
    ackingThread_JMS,
    ackingThread_MQTT_QOS0,
    ackingThread_MQTT_QOS1,
    ackingThread_MQTT_QOS2
} ackingThreadType_t;

typedef struct tag_rcvdMsgDetails_t {
    struct tag_rcvdMsgDetails_t *next;
    ismEngine_DeliveryHandle_t hDelivery; ///< Set by the callback so the processing thread can ack it
    ismEngine_MessageHandle_t  hMessage; ///<Set by the callback so the processing thread can release it
    ismMessageState_t  msgState;  ///< Set by the callback so the processing thread knows whether to ack it
} rcvdMsgDetails_t;

typedef struct tag_reconnectInfo_t {
    volatile bool doReconnection;            //Whether the acking thread should do reconnections
    volatile bool ReconnectionsInProgress;   //Whether the acking thread has noticed being told to cease reconnections
    ismEngine_ClientStateHandle_t hSubOwningClient;
    ismEngine_ClientStateHandle_t hClient;
    uint32_t consOptions;
    uint32_t subOptions;
    ExpectedMsgRate_t expectedMsgRate;
    uint32_t maxInflight;
    uint32_t numSharedSubs;
    const char *subNamePrefix;
    const char *topicPrefix;
    ismEngine_ConsumerHandle_t *phFirstConsumer;
    uint64_t targetRedeliveredMessages;
    uint64_t targetDeliveredMessages;
    uint64_t numReconnects;
} reconnectInfo_t;

typedef struct tag_receiveMsgContext_t {
    ismEngine_SessionHandle_t hSession; ///< Used to ack messages by processor thread
    uint32_t clientNumber;               ///< Receiving messages for which client
    ackingThreadType_t ackingThreadType; ///< What type of consumer are we mimicing
    rcvdMsgDetails_t *firstMsg;   ///< Head pointer for list of received messages
    rcvdMsgDetails_t *lastMsg;   ///< Tail pointer for list of received messages
    pthread_mutex_t msgListMutex; ///< Used to protect the received message list
    pthread_cond_t msgListCond;  ///< This used to broadcast when there are messages in the list to process
    uint64_t msgsInList;   ///< Number of messages in the ack list
    bool doubleAckRcv; ///< should we acknowledge the receipt of msgs twice (PMR 90255,000,858)
    bool expectExtraUndelivered; //< If there are only MQTT subscribers, the only messages redelivered to us should be only we haven't acked
    reconnectInfo_t reconnectInfo; ///< Info only needed when (re)connecting the client
    uint64_t messagesRedeliveredSinceReconnect; ///< Only messages precviously delivered count
    uint64_t messagesDeliveredSinceReconnect; ///< Includes new and redelivered messages
    uint64_t messagesInflightAtLastConnect;
    uint32_t targetNumInflight;       //How many unacked messages to keep
} receiveMsgContext_t;

volatile bool keepPublishing   = true;
volatile bool keepAcking       = true;
volatile bool allowAckingPause = true;
volatile bool readyForDisconnects = false;

volatile uint64_t msgsArrived              = 0;
volatile uint64_t msgsAcked                = 0;
volatile uint64_t msgsDoubleAckRcvd        = 0;
volatile uint64_t msgsSent                 = 0;
volatile uint64_t msgsInFlightAtDisconnect = 0;

volatile uint64_t nextPublisherId = 0;

void doConnect(receiveMsgContext_t *pRcvMsgContext, bool startMessageDelivery);

static void putCompleted(int32_t rc, void *handle, void *pContext)
{
    TEST_ASSERT_EQUAL(rc, OK);
    __sync_fetch_and_add(&msgsSent, 1);

    uint64_t *pPubsInflight = *(uint64_t **)pContext;
    uint64_t oldInflight = __sync_fetch_and_sub(pPubsInflight, 1);

    TEST_ASSERT_CUNIT(oldInflight > 0, ("Old inflight was %lu", oldInflight));
}


void *publishToTopics(void *args)
{
    uint64_t pubId = __sync_fetch_and_add ( &nextPublisherId, 1);

    char clientname[20];
    sprintf(clientname, "Publisher%lu",pubId);

    PublisherInfo_t *pubInfo =(PublisherInfo_t *)args;

    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)clientname);
    int32_t rc = OK;

    ism_engine_threadInit(0);

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    uint64_t pubsInflight = 0;
    uint64_t maxInFlight  = 200;

    rc = test_createClientAndSession(clientname,
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
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, topicString,
                                &hMessage, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        uint64_t *pPubsInflight = &pubsInflight;
        __sync_fetch_and_add(pPubsInflight, 1);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                NULL,
                                                hMessage,
                                                &pPubsInflight, sizeof(pPubsInflight), putCompleted);
        TEST_ASSERT_CUNIT(rc < ISMRC_Error, ("rc was %d", rc));
        if (rc == OK)
        {
            __sync_fetch_and_add(&msgsSent, 1);
        }
        if (rc != ISMRC_AsyncCompletion)
        {
            uint64_t oldInflight = __sync_fetch_and_sub(pPubsInflight, 1);
            TEST_ASSERT_CUNIT(oldInflight > 0, ("Old inflight was %lu", oldInflight));
        }

        while (pubsInflight >= maxInFlight)
        {
            test_usleepAndHeartbeat(100);
        }
    }

    while (pubsInflight > 0)
    {
        test_usleepAndHeartbeat(100);
    }

    test_log(testLOGLEVEL_CHECK, "Publisher Thread exiting after sending %lu messages", msgsSent);
    return NULL;
}


//Acks/releases messages
void *ackingThread(void *arg)
{
    ism_engine_threadInit(0);

    receiveMsgContext_t *context = (receiveMsgContext_t *)arg;

    char threadname[20];
    sprintf(threadname, "Acker%u",context->clientNumber);

    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&threadname);

    uint64_t loopsInAckingPause = 0;
    uint64_t currentUnAckMsgsInAckingPause=0;

    while(1)
    {
        if ( context->reconnectInfo.ReconnectionsInProgress)
        {
            if (context->reconnectInfo.doReconnection)
            {
                //Shall we disconnect?
                if  (   (context->messagesDeliveredSinceReconnect    >=  context->reconnectInfo.targetDeliveredMessages)
                     && (context->messagesRedeliveredSinceReconnect  >=  context->reconnectInfo.targetRedeliveredMessages)
                     && readyForDisconnects) // only disconnect if we've received messages so we don't destroy before main thread tries
                                             //      to start the session
                {
                    sync_ism_engine_destroyClientState(context->reconnectInfo.hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE);

                    rcvdMsgDetails_t *oldFirstMsg = NULL;
                    int os_rc = pthread_mutex_lock(&(context->msgListMutex));
                    TEST_ASSERT_EQUAL(os_rc, 0);

                    context->messagesInflightAtLastConnect = context->msgsInList;

                    oldFirstMsg = context->firstMsg;
                    context->firstMsg = NULL;
                    context->lastMsg = NULL;
                    context->msgsInList = 0;

                    os_rc = pthread_mutex_unlock(&(context->msgListMutex));
                    TEST_ASSERT_EQUAL(os_rc, 0);

                    context->messagesDeliveredSinceReconnect   = 0;
                    context->messagesRedeliveredSinceReconnect = 0;

                    __sync_add_and_fetch(&msgsInFlightAtDisconnect, context->messagesInflightAtLastConnect);

                    if (context->ackingThreadType == ackingThread_MQTT_QOS1 ||context->ackingThreadType == ackingThread_MQTT_QOS2)
                    {
                        if (   context->reconnectInfo.numReconnects % 2 == 0
                            || !context->expectExtraUndelivered)  //If we don't expect extra undelivered messages, wait for all redelivered to arrived (otherwise we'll need to account for the ones
                                                                  //we didn't collect on future connections
                        {
                            context->reconnectInfo.targetRedeliveredMessages = context->messagesInflightAtLastConnect; //Ensure we redeliver all the messages every other time
                        }
                        else
                        {
                            context->reconnectInfo.targetRedeliveredMessages = context->messagesInflightAtLastConnect/2; //This connect, don't wait for all messages to be redelivered
                        }
                    }

                    if (context->ackingThreadType == ackingThread_MQTT_QOS2 && context->messagesInflightAtLastConnect > 0)
                    {
                        doConnect(context, false);

                        TEST_ASSERT_EQUAL(context->messagesInflightAtLastConnect,
                                          context->reconnectInfo.hClient->hMsgDeliveryInfo->NumDeliveryIds);

                        int32_t rc = ism_engine_startMessageDelivery(context->hSession,
                                ismENGINE_START_DELIVERY_OPTION_NONE,
                                NULL, 0, NULL);

                        TEST_ASSERT_EQUAL(rc, OK);

                    }
                    else
                    {
                        doConnect(context, true);
                    }

                    //Delete the old list
                    while (oldFirstMsg)
                    {
                        rcvdMsgDetails_t *tmp = oldFirstMsg;
                        oldFirstMsg = tmp->next;

                        if (context->ackingThreadType == ackingThread_MQTT_QOS2)
                        {
                            if (tmp->msgState != ismMESSAGE_STATE_RECEIVED) //TODO: DO WE EVER EXPECT RCVD Msgs - I don't think we reconnect with any partially acked
                            {
                                TEST_ASSERT_EQUAL(tmp->msgState, ismMESSAGE_STATE_DELIVERED);
                            }
                        }
                        ism_engine_releaseMessage(tmp->hMessage);
                        free(tmp);
                    }
                    context->reconnectInfo.numReconnects++;
                }
                else
                {
                   /* if (context->messagesDeliveredSinceReconnect % 2048 == 0)
                    {
                        printf("client %u (mdr limit: %u) (reconnects: %lu): msgs dlvrd (target): %lu (%lu). msgs redlvrd (target): %lu (%lu)\n",
                                context->clientNumber,
                                (context->reconnectInfo.hClient->hMsgDeliveryInfo? context->reconnectInfo.hClient->hMsgDeliveryInfo->MaxInflightMsgs : 0),
                                context->reconnectInfo.numReconnects,
                                context->messagesDeliveredSinceReconnect, context->reconnectInfo.targetDeliveredMessages,
                                context->messagesRedeliveredSinceReconnect, context->reconnectInfo.targetRedeliveredMessages);
                    }*/
                }
            }
            else
            {
                //We've noticed we should no longer reconnect...
                context->reconnectInfo.ReconnectionsInProgress = false;
            }
        }
        if (allowAckingPause && (context->msgsInList < context->targetNumInflight))
        {
            usleep(100);
        }

        int os_rc = pthread_mutex_lock(&(context->msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        while ((context->firstMsg == NULL) && keepAcking)
        {
            os_rc = pthread_cond_wait(&(context->msgListCond), &(context->msgListMutex));
            TEST_ASSERT_EQUAL(os_rc, 0);
        }

        if (!keepAcking && (context->firstMsg == NULL))
        {   os_rc = pthread_mutex_unlock(&(context->msgListMutex));
            TEST_ASSERT_EQUAL(os_rc, 0);
            break;
        }

        if (allowAckingPause && (context->msgsInList < context->targetNumInflight))
        {
            //Want to simulate having a number of acks in flight for stressing flow control.
            //If there aren't enough...wait (but not forever as server doesn't have to send us as
            //many messages as we want
            loopsInAckingPause++;
            if (context->msgsInList  > currentUnAckMsgsInAckingPause)
            {
                //Messages are still arriving... we don't need to start acking.
                loopsInAckingPause = 0;
            }
            currentUnAckMsgsInAckingPause = context->msgsInList;

            if (loopsInAckingPause < 2000 || context->msgsInList == 0)
            {
                os_rc = pthread_mutex_unlock(&(context->msgListMutex));
                TEST_ASSERT_EQUAL(os_rc, 0);
                continue;
            }
        }
        loopsInAckingPause = 0;             //Not in acking pause... reset counters
        currentUnAckMsgsInAckingPause = 0;

        TEST_ASSERT_PTR_NOT_NULL(context->firstMsg);
        rcvdMsgDetails_t *msg = context->firstMsg;

        context->firstMsg = msg->next;
        if(context->lastMsg == msg)
        {
            TEST_ASSERT_EQUAL(msg->next, NULL);
            context->lastMsg = NULL;
        }

        context->msgsInList--; //We've just removed one

        rcvdMsgDetails_t *nextMsg = msg->next; //Remember the next message as well - we will mark that one recvd if we're qos2


        os_rc = pthread_mutex_unlock(&(context->msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        ism_engine_releaseMessage(msg->hMessage);

        if (   (nextMsg != NULL)
            && (context->ackingThreadType == ackingThread_MQTT_QOS2))
        {
            int rc = sync_ism_engine_confirmMessageDelivery(context->hSession,
                                                       NULL,
                                                       nextMsg->hDelivery,
                                                       ismENGINE_CONFIRM_OPTION_RECEIVED);

            TEST_ASSERT_EQUAL(rc, OK);

            if (context->doubleAckRcv) {
                rc = sync_ism_engine_confirmMessageDelivery(context->hSession,
                                                       NULL,
                                                       nextMsg->hDelivery,
                                                       ismENGINE_CONFIRM_OPTION_RECEIVED);

                TEST_ASSERT_EQUAL(rc, OK);
                __sync_add_and_fetch(&msgsDoubleAckRcvd ,1);
            }
        }

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

    test_log(testLOGLEVEL_CHECK, "Acking Thread %s exiting", threadname);
    return NULL;
}

//When we're given a message, record we got it and add it to a list to ack later
bool ConsumeCallback(
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

/*static uint64_t numdebugbreaks = 0;
if(context->ackingThreadType == ackingThread_MQTT_QOS2 && context->reconnectInfo.numReconnects > 0)
{
    uint64_t break_num = __sync_fetch_and_add(&numdebugbreaks, 1);

    if (break_num == 0)
    {
        ieut_breakIntoDebugger();
    }
}*/

    if (context->ackingThreadType == ackingThread_MQTT_QOS0)
    {
        TEST_ASSERT_EQUAL(state, ismMESSAGE_STATE_CONSUMED);
    }
    else
    {
        if (state != ismMESSAGE_STATE_RECEIVED) //TODO: DO WE EVER EXPECT RCVD Msgs - I don't think we reconnect with any partially acked
        {
            TEST_ASSERT_EQUAL(state, ismMESSAGE_STATE_DELIVERED);
        }
    }

    if (    (context->ackingThreadType == ackingThread_MQTT_QOS1)
          ||(context->ackingThreadType == ackingThread_MQTT_QOS2))
    {
        TEST_ASSERT_NOT_EQUAL(deliveryId, 0);
    }
    else
    {
        TEST_ASSERT_EQUAL(deliveryId, 0);
    }
    uint64_t numArrived = __sync_add_and_fetch(&msgsArrived, 1);

    if( numArrived % 16384 == 0)
    {
        test_log(testLOGLEVEL_TESTPROGRESS, "Msgs Arrived: %lu", numArrived);
    }

    if (pMsgDetails->RedeliveryCount != 0)
    {
        context->messagesRedeliveredSinceReconnect++; //Only this callback increases it - no sync required

        if (  !context->expectExtraUndelivered
            &&(context->ackingThreadType == ackingThread_MQTT_QOS2 || context->ackingThreadType == ackingThread_MQTT_QOS1)
            &&(context->messagesRedeliveredSinceReconnect > context->reconnectInfo.targetRedeliveredMessages))
        {
            //We've got a redelivered messages but we didn't expect to
            TEST_ASSERT_EQUAL(1, 0);
        }
    }
    else if (   context->ackingThreadType == ackingThread_MQTT_QOS2
              && context->messagesRedeliveredSinceReconnect < context->reconnectInfo.targetRedeliveredMessages)
    {
        //We've got a new message but we haven't received all our old messages yet
        TEST_ASSERT_EQUAL(1, 0);
    }
    context->messagesDeliveredSinceReconnect++; //Only this callback increases it - no sync required

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

    context->msgsInList++; //We've just added one to the list

    //Broadcast that we've added some messages to the list
    os_rc = pthread_cond_broadcast(&(context->msgListCond));
    TEST_ASSERT_EQUAL(os_rc, 0);

    //Let the processing thread access the list of messages...
    os_rc = pthread_mutex_unlock(&(context->msgListMutex));
    TEST_ASSERT_EQUAL(os_rc, 0);


    return true; //true - we'd like more messages
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

void doConnect(receiveMsgContext_t *pRcvMsgContext, bool startMessageDelivery)
{
    char clientId[64];

    sprintf(clientId, "SubbingClient%u_rate%u", pRcvMsgContext->clientNumber, pRcvMsgContext->reconnectInfo.expectedMsgRate);

    int32_t rc = test_createClientAndSession(clientId,
            NULL,
            ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
            ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
            &(pRcvMsgContext->reconnectInfo.hClient),
            &(pRcvMsgContext->hSession), startMessageDelivery);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pRcvMsgContext->reconnectInfo.hClient);
    TEST_ASSERT_PTR_NOT_NULL(pRcvMsgContext->hSession);

    pRcvMsgContext->reconnectInfo.hClient->expectedMsgRate = pRcvMsgContext->reconnectInfo.expectedMsgRate;
    pRcvMsgContext->reconnectInfo.hClient->maxInflightLimit = pRcvMsgContext->reconnectInfo.maxInflight;
    
    //printf("Overriding inflight to %u Client %s. %s\n",
    //               pRcvMsgContext->reconnectInfo.maxInflight,
    //               pRcvMsgContext->reconnectInfo.hClient->pClientId,
    //               pRcvMsgContext->reconnectInfo.hClient->hMsgDeliveryInfo? "Too late": "should be ok");
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    subAttrs.subOptions = pRcvMsgContext->reconnectInfo.subOptions;

    //If this is the first client, create and own the subscriptions (other clients reuse them)
    for (uint32_t j=0; j < pRcvMsgContext->reconnectInfo.numSharedSubs; j++)
    {
        char topicString[256];
        char subName[256];

        sprintf(topicString, "%s%u", pRcvMsgContext->reconnectInfo.topicPrefix, j);
        sprintf(subName, "%s%u", pRcvMsgContext->reconnectInfo.subNamePrefix, j);

        if (pRcvMsgContext->clientNumber == 0)
        {
            //We are the owning client
            pRcvMsgContext->reconnectInfo.hSubOwningClient = pRcvMsgContext->reconnectInfo.hClient;

            rc = sync_ism_engine_createSubscription(pRcvMsgContext->reconnectInfo.hClient,
                                               subName,
                                               NULL,
                                               ismDESTINATION_TOPIC,
                                               topicString,
                                               &subAttrs,
                                               pRcvMsgContext->reconnectInfo.hSubOwningClient);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            rc = ism_engine_reuseSubscription(pRcvMsgContext->reconnectInfo.hClient,
                    subName,
                    &subAttrs,
                    pRcvMsgContext->reconnectInfo.hSubOwningClient);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        rc = ism_engine_createConsumer(pRcvMsgContext->hSession,
                ismDESTINATION_SUBSCRIPTION,
                subName,
                NULL,
                pRcvMsgContext->reconnectInfo.hSubOwningClient,
                &pRcvMsgContext, sizeof(pRcvMsgContext), ConsumeCallback,
                NULL,
                pRcvMsgContext->reconnectInfo.consOptions,
                &(pRcvMsgContext->reconnectInfo.phFirstConsumer[j]),
                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

void doTestFlow(uint32_t numSharedSubs, bool useDurableSubs, bool doDoubleAckRcving, bool doDisconnect, bool onlyMqtt) {

    keepPublishing      = true;
    keepAcking          = true;
    allowAckingPause    = true;
    readyForDisconnects = false;

    msgsArrived     = 0;
    msgsAcked       = 0;
    msgsDoubleAckRcvd = 0;
    msgsSent        = 0;

    uint32_t numSubbingClients = 8;
    uint32_t numPubbingThreads = 4;
    char *topicPrefix="/test/MainSharedFlow";

    char *subNamePrefix="testSub";

    //In this test all subs from same client use same sub options
    uint32_t subOptions[numSubbingClients];

    //In this test all consumers from the same client use the same options
    uint32_t consOptions[numSubbingClients];

    //In this test all consumers from the same client use the same acking thread with the same options
    uint32_t ackingOptions[numSubbingClients];
    //Initialise sub + consumer options
    uint32_t basesubOptions =   ismENGINE_SUBSCRIPTION_OPTION_SHARED
            | (useDurableSubs ? ismENGINE_SUBSCRIPTION_OPTION_DURABLE : 0); //Options for all sub types

    uint32_t subTypeOptions[] = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE|
                                      ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
                                  ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE|
                                      ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
                                  ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE|
                                      ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
                         onlyMqtt ? ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE|
                                      ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                                : ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE };
    uint32_t consTypeOptions[] = { ismENGINE_CONSUMER_OPTION_NONE,
            ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
            ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
            onlyMqtt ? ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID
                     : ismENGINE_CONSUMER_OPTION_ACK };
    ackingThreadType_t ackingTypeOptions[] = { ackingThread_MQTT_QOS0,
            ackingThread_MQTT_QOS1,
            ackingThread_MQTT_QOS2,
            onlyMqtt ? ackingThread_MQTT_QOS2 : ackingThread_JMS };

    uint32_t numSubTypes = sizeof(subTypeOptions)/sizeof(subTypeOptions[0]);
    uint32_t numConsTypes = sizeof(consTypeOptions)/sizeof(consTypeOptions[0]);
    uint32_t numAckingTypes = sizeof(ackingTypeOptions)/sizeof(ackingTypeOptions[0]);

    TEST_ASSERT_EQUAL(numSubTypes,numConsTypes);
    TEST_ASSERT_EQUAL(numSubTypes,numAckingTypes);

    uint32_t currSubType = 0;
    for (uint32_t i = 0; i < numSubbingClients; i++)
    {
        subOptions[i]    = basesubOptions | subTypeOptions[currSubType];
        consOptions[i]   = consTypeOptions[currSubType];
        ackingOptions[i] = ackingTypeOptions[currSubType];

        currSubType++;

        if (currSubType >= numSubTypes)
        {
            currSubType = 0;
        }
    }

    //Set off some threads publishing
    //Set up a thread publishing to a topic...
    pthread_t publishingThreadId[numPubbingThreads];
    PublisherInfo_t pubberInfo[numPubbingThreads];

    for (uint32_t i=0; i < numPubbingThreads; i++)
    {
        pubberInfo[i].topicPrefix = topicPrefix;
        pubberInfo[i].numTopics   = numSharedSubs;

        int os_rc = test_task_startThread( &(publishingThreadId[i]),publishToTopics, &pubberInfo,"publishToTopics");
        TEST_ASSERT_EQUAL(os_rc, 0);
    }

    // Set default maxMessageCount to 0 for the duration of the test
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 0;

    //Create clients/sessions/ackingthreads/subs/consumers
    ismEngine_ConsumerHandle_t hConsumer[numSubbingClients*numSharedSubs];
    pthread_t ackingThreadId[numSubbingClients];
    receiveMsgContext_t consInfo[numSubbingClients];

    for (uint32_t i=0; i < numSubbingClients; i++)
    {
        int os_rc = pthread_mutex_init(&(consInfo[i].msgListMutex), NULL);
        TEST_ASSERT_EQUAL(os_rc, 0);
        os_rc = pthread_cond_init(&(consInfo[i].msgListCond), NULL);
        TEST_ASSERT_EQUAL(os_rc, 0);
        consInfo[i].firstMsg = NULL;
        consInfo[i].lastMsg = NULL;
        consInfo[i].clientNumber = i;
        consInfo[i].msgsInList = 0;
        consInfo[i].messagesInflightAtLastConnect = 0;
        consInfo[i].messagesRedeliveredSinceReconnect = 0;
        consInfo[i].messagesDeliveredSinceReconnect = 0;
        consInfo[i].ackingThreadType = ackingOptions[i];
        consInfo[i].doubleAckRcv = doDoubleAckRcving;
        consInfo[i].expectExtraUndelivered = !onlyMqtt;
        consInfo[i].reconnectInfo.numSharedSubs = numSharedSubs;
        consInfo[i].reconnectInfo.phFirstConsumer = &(hConsumer[i*numSharedSubs]);
        consInfo[i].reconnectInfo.topicPrefix = topicPrefix;
        consInfo[i].reconnectInfo.subNamePrefix = subNamePrefix;
        consInfo[i].reconnectInfo.consOptions = consOptions[i];
        consInfo[i].reconnectInfo.subOptions  = subOptions[i];
        consInfo[i].reconnectInfo.targetRedeliveredMessages = 0;

        /*  TODO: Can't currently do msgrate high as reset during steal.... need to investigate fake security context*/
        /*  Temporarily(?) don't do high rate ever */
        if (doDisconnect && (1 == 0))
        {
            //Want to have lots of messages in flight when disconnecting
            consInfo[i].reconnectInfo.expectedMsgRate = EXPECTEDMSGRATE_HIGH;
            consInfo[i].reconnectInfo.maxInflight = 2048;
            consInfo[i].reconnectInfo.targetDeliveredMessages   = 3000; /// How many messages to receive before disconnecting
            consInfo[i].targetNumInflight = 2000; //Keep close to the max server will allow unacked (High = 2048) to stress flow control
        }
        else
        {
            consInfo[i].reconnectInfo.expectedMsgRate = EXPECTEDMSGRATE_DEFAULT;
            consInfo[i].reconnectInfo.maxInflight = 128;
            consInfo[i].reconnectInfo.targetDeliveredMessages   = 200;
            consInfo[i].targetNumInflight = 125; //Keep close to the max server will allow (Default = 128) unacked to stress flow control
        }
        consInfo[i].reconnectInfo.numReconnects = 0;


        if (i == 0)
        {
            consInfo[i].reconnectInfo.doReconnection = false; //Don't disconnect first client - it owns subs
        }
        else
        {
            consInfo[i].reconnectInfo.doReconnection = doDisconnect;
            consInfo[i].reconnectInfo.hSubOwningClient = consInfo[0].reconnectInfo.hClient;
        }
        consInfo[i].reconnectInfo.ReconnectionsInProgress = consInfo[i].reconnectInfo.doReconnection;


        doConnect(&consInfo[i], false);

        os_rc = test_task_startThread( &(ackingThreadId[i]),ackingThread, &(consInfo[i]),"ackingThread");
        TEST_ASSERT_EQUAL(os_rc, 0);
    }

    //Start message delivery and wait for lots of messages to flow through the system...
    for (uint32_t i=0; i < numSubbingClients; i++)
    {
        //printf("FIRST TIME Starting delivery for %u\n", i);
        int32_t rc = ism_engine_startMessageDelivery(consInfo[i].hSession,
                ismENGINE_START_DELIVERY_OPTION_NONE,
                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Now we've started the sessions and aren't going to touch them the acking threads can start tearing them down
    readyForDisconnects = true;


    uint64_t minReconnects = UINT64_MAX;

    while( (msgsAcked < 20000)
          || (doDisconnect && minReconnects < 5))
    {
        usleep(100);

        if (doDisconnect)
        {
            minReconnects = UINT64_MAX;

            for (uint32_t i=1; i < numSubbingClients; i++) //don't look at client 0 - doesn'tt reconnect
            {
                if (consInfo[i].reconnectInfo.numReconnects < minReconnects)
                {
                    minReconnects = consInfo[i].reconnectInfo.numReconnects;
                }
            }
        }
    }

    //Stop the acking threads disconnecting/reconnecting as we are about to destroy consumers etc...
    if (doDisconnect)
    {
        for (uint32_t i=0; i < numSubbingClients; i++)
        {
            consInfo[i].reconnectInfo.doReconnection = false;
        }

        for (uint32_t i=0; i < numSubbingClients; i++)
        {
            while (consInfo[i].reconnectInfo.ReconnectionsInProgress)
            {
                usleep(50);
            }
        }
    }

    //Delete consumers + Unsub all subs, acks should still continue on the backlog, testing post unsub acks.
    for (int32_t i=numSubbingClients -1; i >= 0; i--)
    {
        for (uint32_t j=0; j < numSharedSubs; j++)
        {
            uint32_t consNumber = (i*numSharedSubs)+j;

            volatile bool destroyed = false;
            volatile bool *pDestroyed = &destroyed;

            int32_t rc = ism_engine_destroyConsumer( hConsumer[consNumber]
                                                   , &pDestroyed
                                                   , sizeof(pDestroyed)
                                                   , waitForCompletion);
            if (rc == ISMRC_AsyncCompletion)
            {
                //wait for client to go away...
                while(!destroyed);
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, OK);
            }

            char subName[256];
            sprintf(subName, "%s%u", subNamePrefix, j);

            rc = ism_engine_destroySubscription(consInfo[i].reconnectInfo.hClient,
                                                subName,
                                                consInfo[i].reconnectInfo.hSubOwningClient,
                                                NULL, 0, NULL);

            if( i == 0 )
            {
                TEST_ASSERT_EQUAL(rc, OK);
            }
            else
            {
                if (consInfo[i].reconnectInfo.subOptions & ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT)
                {
                    //This client was a sharer... when it leaves it should be told that is fine
                    TEST_ASSERT_EQUAL(rc, OK);
                }
                else
                {
                    //Anonymous sharers are told destination in use if there still consumers unless there
                    //are other non-anonymous clients connected in which case it is told ok (as the consumers
                    //might belong to the non-anonymous sharers
                    if (rc != OK)
                    {
                        TEST_ASSERT_EQUAL(rc, ISMRC_DestinationInUse);
                    }
                }
            }
        }
    }

    //Tell the ack threads not to pause for backlogs to build up any more and ping the acking threads to make them notice
    allowAckingPause = false;

    for (uint32_t i=0; i < numSubbingClients; i++)
    {
        int os_rc = pthread_mutex_lock(&(consInfo[i].msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);
        os_rc = pthread_cond_broadcast(&(consInfo[i].msgListCond));
        TEST_ASSERT_EQUAL(os_rc, 0);
        os_rc = pthread_mutex_unlock(&(consInfo[i].msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);
    }

    //tell publishing threads to stop
    keepPublishing   = false;

    //join publishers and acking threads
    for (uint32_t i=0; i < numPubbingThreads; i++)
    {
        int os_rc = pthread_join(publishingThreadId[i], NULL);
        TEST_ASSERT_EQUAL(os_rc, 0);
    }

    keepAcking = false;

    for (uint32_t i=0; i < numSubbingClients; i++)
    {
        int os_rc = pthread_mutex_lock(&(consInfo[i].msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);
        os_rc = pthread_cond_broadcast(&(consInfo[i].msgListCond));
        TEST_ASSERT_EQUAL(os_rc, 0);
        os_rc = pthread_mutex_unlock(&(consInfo[i].msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        os_rc = pthread_join(ackingThreadId[i], NULL);
        TEST_ASSERT_EQUAL(os_rc, 0);
    }

    //destroys sessions & clients
    for (uint32_t i=0; i < numSubbingClients; i++)
    {
        int32_t rc = test_destroyClientAndSession(consInfo[i].reconnectInfo.hClient,
                                                  consInfo[i].hSession, true);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    test_log(testLOGLEVEL_TESTDESC, "Msgs Sent %lu, Msgs Arrived %lu, Msgs Acked %lu, Msgs Double Ack(received) %lu Msgs InFlight at Connect: %lu\n",
            msgsSent, msgsArrived, msgsAcked, msgsDoubleAckRcvd, msgsInFlightAtDisconnect);
}

void test_MainSharedFlowDA(void)
{
    doTestFlow(10, false, true, false, false);
}

void test_MainSharedFlow(void)
{
    doTestFlow(10, false, false, false, false);
}
void test_MainSharedFlowReconnect(void)
{
    doTestFlow(1, true, false, true, true);
}
CU_TestInfo ISM_NamedQueues_CUnit_test_SharedFlow[] =
{
        { "MainSharedFlow", test_MainSharedFlow },
        { "MainSharedFlowDA", test_MainSharedFlowDA },
        { "MainSharedFlowReconnect", test_MainSharedFlowReconnect },
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

CU_SuiteInfo ISM_SharedMQTTFlow_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("SharedFlow", initTestSuite, termTestSuite, ISM_NamedQueues_CUnit_test_SharedFlow),
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

    retval = setup_CU_registry(argc, argv, ISM_SharedMQTTFlow_CUnit_allsuites);

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
