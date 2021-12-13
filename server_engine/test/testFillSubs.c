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
     uint64_t msgCount;
     uint64_t confirmAfter;
     pthread_mutex_t msgListMutex; ///< Used to protect the received message list
     pthread_cond_t msgListCond;  ///< This used to broadcast when there are messages in the list to process
     uint64_t acksStarted;
     uint64_t acksCompleted;
     bool stopAcking;
     ismEngine_Consumer_t *pConsumer;
} ackerContext_t;
#define ACKERCONTEXT_STRUCTID "CACK"

typedef struct tag_msgCallbackContext_t
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    volatile uint64_t received;
    volatile uint64_t receivedWithDeliveryId;
    volatile uint64_t redelivered;
    uint8_t reliability;
    uint32_t maxDelay;
    ackerContext_t ackContext;
    uint64_t lastConfirmTimeNanos;
    pthread_t threadId;
} msgCallbackContext_t;

typedef struct tag_receiveAckInFlight_t {
    ismEngine_SessionHandle_t hSession;   ///< Use this for subsequent consume
    ismEngine_DeliveryHandle_t hDelivery; ///< Once receive completed, consume this
    uint64_t *pAcksCompleted;              ///< Once subsequent consume completed, increase this
} receiveAckInFlight_t;

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

void consumeCompleted(int rc, void *handle, void *context)
{
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
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
    TEST_ASSERT(((rc == OK)||(rc == ISMRC_AsyncCompletion)), ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

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
        assert(false);

        msg->msgState   = ismMESSAGE_STATE_RECEIVED;
        readyForConsume = false;
        receiveAckInFlight_t ackInfo = {hSession, msg->hDelivery, pAcksCompleted};

        int rc = ism_engine_confirmMessageDelivery(hSession,
                                                   NULL,
                                                   msg->hDelivery,
                                                   ismENGINE_CONFIRM_OPTION_RECEIVED,
                                                   &ackInfo, sizeof(ackInfo),
                                                   rcvdAckCompleted);

        TEST_ASSERT(((rc == OK)||(rc == ISMRC_AsyncCompletion)), ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

        if (rc == OK)
        {
            readyForConsume = true;
        }
    }


    if (readyForConsume)
    {
        TEST_ASSERT((    ((subQos == 3) && (msg->msgState == ismMESSAGE_STATE_DELIVERED))   //subQos == 3 for JMS in this test
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
    TEST_ASSERT(ismEngine_CompareStructId(context->StructId,ACKERCONTEXT_STRUCTID) == true, ("Assertion failed in %s line %d", __func__, __LINE__));

    rcvdMsgDetails_t *msgs[context->confirmAfter];
    while(1)
    {
        int os_rc = pthread_mutex_lock(&(context->msgListMutex));
        TEST_ASSERT(os_rc == 0, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));

        while (!(context->stopAcking))
        {
            struct timespec waituntil;
            clock_gettime(CLOCK_MONOTONIC, &waituntil);

            waituntil.tv_sec += (__time_t)30;

            os_rc = pthread_cond_timedwait(&(context->msgListCond),
                                           &(context->msgListMutex),
                                           &waituntil);
            TEST_ASSERT(os_rc == 0 || os_rc == ETIMEDOUT, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));

            if (os_rc == ETIMEDOUT)
            {
                //TEST_ASSERT(context->pConsumer->fFlowControl == false,
                //            ("Assertion failed in %s line %d [pConsumer=%p]", __func__, __LINE__, context->pConsumer));
                break;
            }
            else if (context->msgCount >= context->confirmAfter)
            {
                break;
            }
        }

        if (context->firstMsg == NULL && context->stopAcking)
        {
            //No more messages coming and we have none to ack...
            os_rc = pthread_mutex_unlock(&(context->msgListMutex));
            TEST_ASSERT(os_rc == 0, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));
            break;
        }

        int64_t i = 0;
        rcvdMsgDetails_t *thisMsg = context->firstMsg;

        for(; thisMsg != NULL && i<context->confirmAfter; i++)
        {
            msgs[i] = thisMsg;
            thisMsg = thisMsg->next;
            context->firstMsg = thisMsg;
            context->msgCount--;
        }

        context->lastMsg = thisMsg;

        os_rc = pthread_mutex_unlock(&(context->msgListMutex));
        TEST_ASSERT(os_rc == 0, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));

        for(--i; i>=0; i--)
        {
            startAck(context->ackQos,
                     context->hSession,
                     msgs[i],
                     &(context->acksStarted),
                     &(context->acksCompleted));

            free(msgs[i]);
        }
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
    TEST_ASSERT(msgDetails != NULL, ("Assertion failed in %s line %d [msgDetails=%p]", __func__, __LINE__, msgDetails));

    msgDetails->hDelivery = hDelivery;
    msgDetails->msgState  = state;
    msgDetails->next      = NULL;

    //Add it to the list of messages....
    int32_t os_rc = pthread_mutex_lock(&(pContext->msgListMutex));
    TEST_ASSERT(os_rc == 0, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));

#if 0
    if(pContext->firstMsg == NULL)
    {
        pContext->firstMsg = msgDetails;
        TEST_ASSERT(pContext->lastMsg == NULL, ("Assertion failed in %s line %d [lastMsg=%p]", __func__, __LINE__, pContext->lastMsg));
        pContext->lastMsg = msgDetails;
    }
    else
    {
        TEST_ASSERT(pContext->lastMsg != NULL, ("Assertion failed in %s line %d [lastMsg=%p]", __func__, __LINE__, pContext->lastMsg));
        pContext->lastMsg->next = msgDetails;
        pContext->lastMsg       = msgDetails;
    }
#else
    msgDetails->next = pContext->firstMsg;
    if (pContext->firstMsg == NULL)
    {
        pContext->lastMsg = msgDetails;
    }
    pContext->firstMsg = msgDetails;
#endif

    pContext->msgCount++;

    //Broadcast that we've added some messages to the list
    os_rc = pthread_cond_broadcast(&(pContext->msgListCond));
    TEST_ASSERT(os_rc == 0, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));

    //Let the processing thread access the list of messages...
    os_rc = pthread_mutex_unlock(&(pContext->msgListMutex));
    TEST_ASSERT(os_rc == 0, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));
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
                      ismEngine_DelivererContext_t *  _delivererContext )
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


    if (state != ismMESSAGE_STATE_CONSUMED)
    {
        addMessageToAckList( &(context->ackContext),
                             hDelivery,
                             state);
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

#define NUMBER_OF_BUS_CONNECTOR_SUBS                    1
#define NUMBER_OF_CONSUMERS_PER_BUS_CONNECTOR_SUB       10
#define NUMBER_OF_OTHER_SHARED_SUBS                     10
#define NUMBER_OF_CONSUMERS_PER_OTHER_SHARED_SUB        5
#define NUMBER_OF_OTHER_SHARED_SUBS_TO_DISPLAY          1
#define NUMBER_OF_INDIVIDUAL_SUBS                       2
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

    pthread_condattr_t attr;
    int os_rc = pthread_condattr_init(&attr);
    TEST_ASSERT(os_rc == 0, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));
    os_rc = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    TEST_ASSERT(os_rc == 0, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));

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
        BusConnectorMsgContext[i].maxDelay = rand()%100;
        BusConnectorMsgContext[i].lastConfirmTimeNanos = ism_common_currentTimeNanos();

        ismEngine_SetStructId(BusConnectorMsgContext[i].ackContext.StructId, ACKERCONTEXT_STRUCTID);
        os_rc = pthread_mutex_init(&(BusConnectorMsgContext[i].ackContext.msgListMutex), NULL);
        TEST_ASSERT(os_rc == 0, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));
        os_rc = pthread_cond_init(&(BusConnectorMsgContext[i].ackContext.msgListCond), &attr);
        TEST_ASSERT(os_rc == 0, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));
        BusConnectorMsgContext[i].ackContext.hSession = BusConnectorMsgContext[i].hSession;
        BusConnectorMsgContext[i].ackContext.firstMsg = NULL;
        BusConnectorMsgContext[i].ackContext.lastMsg = NULL;
        BusConnectorMsgContext[i].ackContext.msgCount = 0;
        BusConnectorMsgContext[i].ackContext.confirmAfter = (rand()%(maxInflight-10))+1;
        BusConnectorMsgContext[i].ackContext.stopAcking = false;
        BusConnectorMsgContext[i].ackContext.ackQos = BusConnectorMsgContext[i].reliability-1;
        BusConnectorMsgContext[i].ackContext.acksStarted = 0;
        BusConnectorMsgContext[i].ackContext.acksCompleted = 0;

        rc = test_task_startThread(&(BusConnectorMsgContext[i].threadId),AckingThread, (void *)(&(BusConnectorMsgContext[i].ackContext)),"AckingThread");
        TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

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
        TopicConsumerMsgContext[i].maxDelay = rand()%100;
        TopicConsumerMsgContext[i].lastConfirmTimeNanos = ism_common_currentTimeNanos();

        ismEngine_SetStructId(TopicConsumerMsgContext[i].ackContext.StructId, ACKERCONTEXT_STRUCTID);
        os_rc = pthread_mutex_init(&(TopicConsumerMsgContext[i].ackContext.msgListMutex), NULL);
        TEST_ASSERT(os_rc == 0, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));
        os_rc = pthread_cond_init(&(TopicConsumerMsgContext[i].ackContext.msgListCond), &attr);
        TEST_ASSERT(os_rc == 0, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));
        TopicConsumerMsgContext[i].ackContext.hSession = TopicConsumerMsgContext[i].hSession;
        TopicConsumerMsgContext[i].ackContext.firstMsg = NULL;
        TopicConsumerMsgContext[i].ackContext.lastMsg = NULL;
        TopicConsumerMsgContext[i].ackContext.msgCount = 0;
        TopicConsumerMsgContext[i].ackContext.confirmAfter = (rand()%90)+1;
        TopicConsumerMsgContext[i].ackContext.stopAcking = false;
        TopicConsumerMsgContext[i].ackContext.ackQos = TopicConsumerMsgContext[i].reliability-1;
        TopicConsumerMsgContext[i].ackContext.acksStarted = 0;
        TopicConsumerMsgContext[i].ackContext.acksCompleted = 0;

        rc = test_task_startThread(&(TopicConsumerMsgContext[i].threadId),AckingThread, (void *)(&(TopicConsumerMsgContext[i].ackContext)),"AckingThread");
        TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

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
            OtherConsumerMsgContext[j].maxDelay = rand()%100;
            OtherConsumerMsgContext[j].lastConfirmTimeNanos = ism_common_currentTimeNanos();

            ismEngine_SetStructId(OtherConsumerMsgContext[j].ackContext.StructId, ACKERCONTEXT_STRUCTID);
            os_rc = pthread_mutex_init(&(OtherConsumerMsgContext[j].ackContext.msgListMutex), NULL);
            TEST_ASSERT(os_rc == 0, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));
            os_rc = pthread_cond_init(&(OtherConsumerMsgContext[j].ackContext.msgListCond), &attr);
            TEST_ASSERT(os_rc == 0, ("Assertion failed in %s line %d [os_rc=%d]", __func__, __LINE__, os_rc));
            OtherConsumerMsgContext[j].ackContext.hSession = OtherConsumerMsgContext[j].hSession;
            OtherConsumerMsgContext[j].ackContext.firstMsg = NULL;
            OtherConsumerMsgContext[j].ackContext.lastMsg = NULL;
            OtherConsumerMsgContext[j].ackContext.msgCount = 0;
            OtherConsumerMsgContext[j].ackContext.confirmAfter = (rand()%(maxInflight-10))+1;
            OtherConsumerMsgContext[j].ackContext.stopAcking = false;
            OtherConsumerMsgContext[j].ackContext.ackQos = OtherConsumerMsgContext[j].reliability-1;
            OtherConsumerMsgContext[j].ackContext.acksStarted = 0;
            OtherConsumerMsgContext[j].ackContext.acksCompleted = 0;

            rc = test_task_startThread(&(OtherConsumerMsgContext[j].threadId),AckingThread, (void *)(&(OtherConsumerMsgContext[j].ackContext)),"AckingThread");
            TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

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

            OtherConsumerMsgContext[j].ackContext.pConsumer = OtherConsumerMsgContext[j].hConsumer;
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
                printf(" %lu %lu/%lu [%lu/%lu] %d",
                       BusConnectorMsgContext[j].received,
                       BusConnectorMsgContext[j].ackContext.msgCount,
                       BusConnectorMsgContext[j].ackContext.confirmAfter,
                       BusConnectorMsgContext[j].ackContext.acksStarted,
                       BusConnectorMsgContext[j].ackContext.acksCompleted,
                       BusConnectorMsgContext[j].hConsumer->fFlowControl);
            }
            printf("\n");
#endif

            int32_t startIndex;

#if NUMBER_OF_OTHER_SHARED_SUBS
#if (NUMBER_OF_OTHER_SHARED_SUBS_TO_DISPLAY == NUMBER_OF_OTHER_SHARED_SUBS)
            startIndex = 0;
#else
            startIndex = rand()%(NUMBER_OF_OTHER_SHARED_SUBS-(NUMBER_OF_OTHER_SHARED_SUBS_TO_DISPLAY-1));
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
                    printf(" %lu %lu/%lu [%lu/%lu] %d",
                           OtherConsumerMsgContext[j].received,
                           OtherConsumerMsgContext[j].ackContext.msgCount,
                           OtherConsumerMsgContext[j].ackContext.confirmAfter,
                           OtherConsumerMsgContext[j].ackContext.acksStarted,
                           OtherConsumerMsgContext[j].ackContext.acksCompleted,
                           OtherConsumerMsgContext[j].hConsumer->fFlowControl);
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
                printf("IndConsumers: %lu %lu/%lu [%lu/%lu] %d\n",
                       TopicConsumerMsgContext[i].received,
                       TopicConsumerMsgContext[i].ackContext.msgCount,
                       TopicConsumerMsgContext[i].ackContext.confirmAfter,
                       TopicConsumerMsgContext[i].ackContext.acksStarted,
                       TopicConsumerMsgContext[i].ackContext.acksCompleted,
                       TopicConsumerMsgContext[i].hConsumer->fFlowControl);
            }
#endif
        }

        loopCount++;

        char *password = "password";
        char *filePath = NULL;

        if (loopCount == 1)
        {
            ietd_dumpInMemoryTrace( pThreadData, NULL, password, &filePath);

            printf("TraceDump written to %s\n", filePath);
            iemem_free(pThreadData, iemem_diagnostics, filePath);
        }
        else if (test_traceDumpInterval != 0 && loopCount%test_traceDumpInterval == 0)
        {
            char fileName[50];

            sprintf(fileName, "trcDump_%s_%d", password, loopCount/test_traceDumpInterval);
            ietd_dumpInMemoryTrace( pThreadData, fileName, password, &filePath);

            printf("TraceDump written to %s\n", filePath);
            iemem_free(pThreadData, iemem_diagnostics, filePath);
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
