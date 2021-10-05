/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
/* Module Name: test_nonAcker.c                                      */
/*                                                                   */
/* Description: Main source file for CUnit test of clients not       */
/* acking messages in a timeley fashion                              */
/*********************************************************************/
#include <sys/prctl.h>

#include "engineInternal.h"
#include "policyInfo.h"
#include "multiConsumerQInternal.h"
#include "topicTree.h"

#include "test_utils_initterm.h"
#include "test_utils_sync.h"
#include "test_utils_assert.h"
#include "test_utils_client.h"
#include "test_utils_options.h"
#include "test_utils_message.h"
#include "test_utils_pubsuback.h"
#include "test_utils_task.h"

typedef struct tag_rcvdMsgDetails_t {
    struct tag_rcvdMsgDetails_t *next;
    ismEngine_DeliveryHandle_t hDelivery; ///< Set by the callback so the processing thread can ack it
    ismEngine_MessageHandle_t  hMessage; ///<Set by the callback so the processing thread can release it
    ismMessageState_t  msgState;  ///< Set by the callback so the processing thread knows whether to ack it
} rcvdMsgDetails_t;

typedef struct tag_receiveMsgContext_t {
    ismEngine_ClientStateHandle_t   hClient; ///<used for destroying
    ismEngine_SessionHandle_t hSession; ///< Used to ack messages by processor thread
    uint32_t clientNumber;               ///< Receiving messages for which client
    rcvdMsgDetails_t *firstMsg;   ///< Head pointer for list of received messages
    rcvdMsgDetails_t *lastMsg;   ///< Tail pointer for list of received messages
    pthread_mutex_t msgListMutex; ///< Used to protect the received message list
    pthread_cond_t msgListCond;  ///< This used to broadcast when there are messages in the list to process
    uint64_t msgsInList;   ///< Number of messages in the ack list
    uint64_t msgsToNotAck; ///< Number of messages to have delivered but not ack
    bool ackingThreadRunning;
    bool clientDestroyNeeded;
    bool clientDestroyStarted;
    bool keepAcking;  ///< Should the acking thread keep acking
    bool leaveMsgsUnacked; ///< Do we ack the first two messages we receive
    bool ackMsgsNormally; ///< If  leaveMsgsUnacked this applies to messages after the first 2
    bool pubOnSteal; ///< Whether we should do a publish when called for steal...
} receiveMsgContext_t;

volatile uint64_t msgsArrived       = 0;
volatile uint64_t msgsAckStarted    = 0;
volatile uint64_t msgsAckCompleted  = 0;
uint64_t stealsExpected = 0;
uint64_t badClientDestroysCompleted = 0;

void badClientDestroyedCallback( int32_t rc, void *handle, void *context)
{
    __sync_fetch_and_add(&badClientDestroysCompleted, 1);
}


void destroyClientIfNeeded( receiveMsgContext_t *context)
{
    if (context->clientDestroyNeeded && !context->clientDestroyStarted)
    {
        if (__sync_bool_compare_and_swap(&(context->clientDestroyStarted),false,true))
        {
            int32_t rc = ism_engine_destroyClientState(context->hClient,
                                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                                       NULL,0,badClientDestroyedCallback);
            TEST_ASSERT_CUNIT((rc==OK ||rc == ISMRC_AsyncCompletion), ("rc was %d\n",rc));

            if (rc == OK)
            {
                badClientDestroyedCallback(rc, NULL, NULL);
            }
        }
    }
}


void test_StealCallback(int32_t                         reason,
                        ismEngine_ClientStateHandle_t   hClient,
                        uint32_t                        options,
                        void *                          pContext)
{
    uint64_t oldValue = __sync_fetch_and_sub(&stealsExpected, 1);
    TEST_ASSERT_GREATER_THAN(oldValue, 0)

    receiveMsgContext_t *pRcvMsgContext = (receiveMsgContext_t *)pContext;

    if (pRcvMsgContext->pubOnSteal)
    {
        int32_t rc;
        ismEngine_MessageHandle_t hMessage;
        const char *topicString = "/test/PUBLISH_ON_STEAL";

        rc = test_createMessage(10,
                                ismMESSAGE_PERSISTENCE_PERSISTENT, // TODO: Fiddle
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,  // TODO: Fiddle
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, topicString,
                                &hMessage, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination(pRcvMsgContext->hSession,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);

        if (rc != ISMRC_AsyncCompletion)
        {
            TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Allow for information RCs
        }

    }

    __sync_bool_compare_and_swap(&(pRcvMsgContext->clientDestroyNeeded), false, true);

    return;
}

void ackCompleted(int rc, void *handle, void *context)
{
    __sync_add_and_fetch(&msgsAckCompleted ,1);
}

//Acks/releases messages
void *ackingThread(void *arg)
{
    ism_engine_threadInit(0);

    receiveMsgContext_t *context = (receiveMsgContext_t *)arg;

    char threadname[20];
    sprintf(threadname, "Acker%u",context->clientNumber);

    test_log(testLOGLEVEL_TESTPROGRESS, "Acking Thread %s starting (hSession=%p)", threadname, context->hSession);

    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&threadname);

    while(1)
    {
        int os_rc = pthread_mutex_lock(&(context->msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        while ((context->firstMsg == NULL) && context->keepAcking)
        {
            os_rc = pthread_cond_wait(&(context->msgListCond), &(context->msgListMutex));
            TEST_ASSERT_EQUAL(os_rc, 0);

            destroyClientIfNeeded(context);
        }
        destroyClientIfNeeded(context); //check here as well as in the wait loop so even if we don't wait (because there are
                                        //messages to ack, we still destroy the client

        if (!context->keepAcking && (context->firstMsg == NULL))
        {   os_rc = pthread_mutex_unlock(&(context->msgListMutex));
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

        context->msgsInList--; //We've just removed one

        os_rc = pthread_mutex_unlock(&(context->msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        ism_engine_releaseMessage(msg->hMessage);

        if (msg->msgState != ismMESSAGE_STATE_CONSUMED)
        {
            uint64_t numAcksStarted;

            if(context->clientDestroyStarted)
            {
                context->keepAcking = false;
                numAcksStarted = msgsAckStarted;
            }
            else
            {
                //don't ack if we've been stolen
                numAcksStarted = __sync_add_and_fetch(&msgsAckStarted ,1);

                int rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                  NULL,
                                                  msg->hDelivery,
                                                  ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                  NULL, 0, ackCompleted);
                if (rc == OK)
                {
                    __sync_add_and_fetch(&msgsAckCompleted ,1);
                }
                else
                {
                    TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
                }
            }

            if( numAcksStarted % 255 == 0)
            {
                test_log(testLOGLEVEL_TESTPROGRESS, "Msgs Ack Started: %lu", numAcksStarted);
            }
        }

        free(msg);
    }

    context->ackingThreadRunning=false;
    ism_engine_threadTerm(1);

    test_log(testLOGLEVEL_TESTPROGRESS, "Acking Thread %s exiting", threadname);
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

    uint64_t numArrived = __sync_add_and_fetch(&msgsArrived, 1);

    if( numArrived % 255 == 0)
    {
        test_log(testLOGLEVEL_TESTPROGRESS, "Msgs Arrived: %lu", numArrived);
    }

    if (context->msgsToNotAck > 0)
    {
        ism_engine_releaseMessage(hMessage);
        context->msgsToNotAck--;
    }
    else
    {
        //Copy the details we want from the message into some memory....
        rcvdMsgDetails_t *msgDetails = malloc(sizeof(rcvdMsgDetails_t));
        TEST_ASSERT_PTR_NOT_NULL(msgDetails);

        msgDetails->hDelivery = hDelivery;
        msgDetails->hMessage  = hMessage;
        msgDetails->msgState  = state;
        msgDetails->next      = NULL;

        //Add it to the list of messages to ack
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
    }

    bool wantMoreMsgs =   (    (context->msgsToNotAck > 0)
                           ||   context->ackMsgsNormally   );
    return wantMoreMsgs;
}


void test_basic(const char *testName,
                bool shared,
                bool connected,
                bool consuming,
                bool wellBehaved,
                int32_t publishDuringPublish, // 0 = no
                                              // 10/11/12 = yes non-persistent, qos 0/1/2
                                              // 20/21/22 = yes persistent qos 0/1/2
                uint32_t ratio)
{
    #define MAX_CLIENTS 2
    ismEngine_ClientStateHandle_t hClient[MAX_CLIENTS];
    ismEngine_SessionHandle_t hClientSession[MAX_CLIENTS];
    char *ClientIds[MAX_CLIENTS];
    char *subName;
    char *sharedClientId;
    uint64_t maxDepth = 500;
    int32_t rc;
    pthread_t ackingThreadId;

    uint32_t oldRatio = ismEngine_serverGlobal.DestroyNonAckerRatio;
    ismEngine_serverGlobal.DestroyNonAckerRatio = ratio;

    /************************************************************************/
    /* Set the default policy info to allow certain depth queues            */
    /************************************************************************/
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = maxDepth;
    iepi_getDefaultPolicyInfo(false)->maxMsgBehavior  = DiscardOldMessages;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s(%s, %s, %s, %s, %s, %d, %u)...\n", __func__, testName,
                shared    ?    "shared":"non-shared",
                connected ? "connected":"disconnected",
                consuming ? "consuming":"not-consuming",
                wellBehaved ?    "good":"bad",
                publishDuringPublish, ratio);

    char *topicString = "BasicTest/topic";
    ClientIds[0]   = "BasicTest_NonAcker";
    ClientIds[1]   = "BasicTest_OtherClient"; //Just to stop the sub going away if shared
    subName        = "subBasic";
    sharedClientId = "__SharedBasicTest";

    badClientDestroysCompleted = 0;

    //Setup context used when consuming (and acking) and during steal
    receiveMsgContext_t consInfo = {0};
    receiveMsgContext_t *pConsInfo = &consInfo;

    int os_rc = pthread_mutex_init(&(consInfo.msgListMutex), NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);
    os_rc = pthread_cond_init(&(consInfo.msgListCond), NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);
    consInfo.firstMsg = NULL;
    consInfo.lastMsg = NULL;
    consInfo.clientNumber = 0;
    consInfo.msgsInList = 0;
    consInfo.keepAcking = true;
    consInfo.ackingThreadRunning = consuming; //if we're consuming we'll have an acking thread
    consInfo.ackMsgsNormally  = consuming; //After the first two messages do we want to receive and ack normally
    consInfo.pubOnSteal = (publishDuringPublish != 0);

    //Setup the client that will own the shared namespace
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSharedOwningSession;
    ismEngine_Subscription_t *subscription = NULL;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    if (shared)
    {
        test_createClientAndSession(sharedClientId,
                                    NULL,
                                    ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                    ismENGINE_CREATE_SESSION_OPTION_NONE,
                                    &hSharedOwningClient, &hSharedOwningSession, true);
        TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
        TEST_ASSERT_PTR_NOT_NULL(hSharedOwningSession);

        // 1) Create a shared sub with 2  clients (0 & 1)
        for (uint32_t i = 0 ; i < 2; i++)
        {
            rc = sync_ism_engine_createClientState(ClientIds[i],
                    testDEFAULT_PROTOCOL_ID,
                    ismENGINE_CREATE_CLIENT_OPTION_NONE,
                    pConsInfo, test_StealCallback,
                    NULL,
                    &(hClient[i]));
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_engine_createSession(hClient[i],
                    ismENGINE_CREATE_SESSION_OPTION_NONE,
                    &(hClientSession[i]),
                    NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_engine_startMessageDelivery(hClientSession[i],
                    ismENGINE_START_DELIVERY_OPTION_NONE,
                    NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            test_makeSub( subName
                   , topicString
                   ,    (i == 0 ? ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE
                               : ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE)
                      | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                      | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                      | ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY
                   , hClient[i]
                   , hSharedOwningClient);
        }

        // Make sure the subscription exists.
        rc = iett_findClientSubscription(pThreadData,
                                         ClientIds[0],
                                         iett_generateClientIdHash(ClientIds[0]),
                                         subName,
                                         iett_generateSubNameHash(subName),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        iett_releaseSubscription(pThreadData, subscription, false);
    }
    else
    {
        rc = sync_ism_engine_createClientState(ClientIds[0],
                testDEFAULT_PROTOCOL_ID,
                ismENGINE_CREATE_CLIENT_OPTION_NONE,
                pConsInfo, test_StealCallback,
                NULL,
                &(hClient[0]));
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_createSession(hClient[0],
                ismENGINE_CREATE_SESSION_OPTION_NONE,
                &(hClientSession[0]),
                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_startMessageDelivery(hClientSession[0],
                ismENGINE_START_DELIVERY_OPTION_NONE,
                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Update the callback context now wehave a client+session
    consInfo.hClient  = hClient[0];
    consInfo.hSession = hClientSession[0];

    // To make this client badly behaved, send  a qos 1 message and a qos 2 message
    // that we leave unacked
    if (!wellBehaved)
    {
        test_pubMessages(topicString,
                ismMESSAGE_PERSISTENCE_PERSISTENT,
                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                1);

        test_pubMessages(topicString,
                ismMESSAGE_PERSISTENCE_PERSISTENT,
                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                1);

        consInfo.msgsToNotAck = 2;
    }

    ismEngine_ConsumerHandle_t hConsumer = NULL;
    ismQHandle_t  queueHandle = NULL;

    if (consuming || !wellBehaved)
    {
        if (consuming)
        {
            //Start the acking thread
            os_rc = test_task_startThread( &ackingThreadId,ackingThread, &consInfo,"ackingThread");
            TEST_ASSERT_EQUAL(os_rc, 0);
        }

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };

        rc = ism_engine_createConsumer(hClientSession[0],
                shared ? ismDESTINATION_SUBSCRIPTION : ismDESTINATION_TOPIC,
                shared ? subName : topicString,
                shared ? NULL : &subAttrs,
                 shared? hSharedOwningClient : hClient[0],
                &pConsInfo, sizeof(pConsInfo), ConsumeCallback,
                NULL,
                ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                &hConsumer,
                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        queueHandle = hConsumer->queueHandle;
    }

    TEST_ASSERT_EQUAL(stealsExpected, 0);

    // We don't expect to be stolen for the wellBehaved case or if the DestroyNonAckerRatio was set to 0.
    bool expectToBeDestroyed = (!wellBehaved) && (ratio != 0);

    stealsExpected = expectToBeDestroyed ? 1 : 0;

    test_pubMessages(topicString,
            ismMESSAGE_PERSISTENCE_PERSISTENT,
            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
            (oldRatio-3)*maxDepth);

    //Publish messages until the client is destroyed, persistence and reliability are altered if we want to
    //force a publish in the middle of an ongoing publish...
    uint64_t loops = 0;
    uint8_t persistence = publishDuringPublish == 0 ? ismMESSAGE_PERSISTENCE_PERSISTENT : ((publishDuringPublish / 10) - 1);
    uint8_t reliability = publishDuringPublish == 0 ? ismMESSAGE_RELIABILITY_AT_LEAST_ONCE : (publishDuringPublish % 10);

    while (badClientDestroysCompleted == 0)
    {
        test_pubMessages(topicString,
                persistence,
                reliability,
                maxDepth);


        if (!pConsInfo->ackingThreadRunning)
        {
            //If the acking thread won't do the destroy, we will
            destroyClientIfNeeded(pConsInfo);
        }

        if (loops > 190 && usingDiskPersistence)
        {
            sleep(10); //If we're using disk persistence, allow the callsbacks to catch up
        }

        if (loops > 200)
        {
            if (!expectToBeDestroyed) break;

            //We're stuck
            abort();
        }
        loops++;
    }
    TEST_ASSERT_EQUAL(stealsExpected, 0);

    loops = 0;
    subscription = NULL;

    if (shared)
    {
        rc = iett_findClientSubscription(pThreadData,
                                         sharedClientId,
                                         iett_generateClientIdHash(sharedClientId),
                                         subName,
                                         iett_generateSubNameHash(subName),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);

        uint64_t headPageOId;
        iemqQueue_t *pQ;
        do
        {
             pQ = (iemqQueue_t *)(subscription->queueHandle);

            //Now put enough messages to clean a headpage... the messages can
            //take a while to discard so we keep trying for a long time.
            test_pubMessages(topicString,
                    ismMESSAGE_PERSISTENCE_PERSISTENT,
                    ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                    100);

            iemq_takeReadHeadLock_ext(pQ);
            headPageOId = pQ->headPage->nodes[0].orderId;
            iemq_releaseHeadLock_ext(pQ);

            if (headPageOId >= maxDepth)
            {
                break;
            }

            loops++;

        }
        while (loops < 200);

        TEST_ASSERT_GREATER_THAN(headPageOId, maxDepth);

        test_log(testLOGLEVEL_TESTPROGRESS, "Messages Discarded %lu\n",pQ->discardedMsgs);
        iett_releaseSubscription(pThreadData, subscription, false);
    }
    else
    {
        //Now put enough messages to clean a headpage:
        test_pubMessages(topicString,
                ismMESSAGE_PERSISTENCE_PERSISTENT,
                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                100);

        rc = iett_findQHandleSubscription(pThreadData,
                                          queueHandle,
                                          &subscription);
        if (expectToBeDestroyed)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
            iett_releaseSubscription(pThreadData, subscription, false);
        }
    }

    if (consuming)
    {
        consInfo.keepAcking = false;

        os_rc = pthread_mutex_lock(&(consInfo.msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);
        os_rc = pthread_cond_broadcast(&(consInfo.msgListCond));
        TEST_ASSERT_EQUAL(os_rc, 0);
        os_rc = pthread_mutex_unlock(&(consInfo.msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        os_rc = pthread_join(ackingThreadId, NULL);
        TEST_ASSERT_EQUAL(os_rc, 0);

        //Clean up memory for any messages we didn't ack
        while(pConsInfo->firstMsg)
        {
            rcvdMsgDetails_t *msg = pConsInfo->firstMsg;

            pConsInfo->firstMsg = msg->next;
            if(pConsInfo->lastMsg == msg)
            {
                TEST_ASSERT_EQUAL(msg->next, NULL);
                pConsInfo->lastMsg = NULL;
            }

            pConsInfo->msgsInList--; //We've just removed one

            ism_engine_releaseMessage(msg->hMessage);
            free(msg);
        }

        test_log(testLOGLEVEL_CHECK, "Msgs Received: %lu, AcksStarted: %lu, AcksReceived %lu\n",
                msgsArrived, msgsAckStarted, msgsAckCompleted);
    }

    //Don't destroy 0th client - he was the now dead bad acker
    if (shared)
    {
        for (uint32_t i = 1 ; i < MAX_CLIENTS; i++)
        {
            rc = sync_ism_engine_destroyClientState(hClient[i],
                                                    ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        rc = sync_ism_engine_destroyClientState(hSharedOwningClient,
                                                ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    if (!expectToBeDestroyed)
    {
        if (__sync_bool_compare_and_swap(&(pConsInfo->clientDestroyNeeded), false, true))
        {
            pConsInfo->clientDestroyStarted = true;
            rc = sync_ism_engine_destroyClientState(hClient[0],
                                                    ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    test_log(testLOGLEVEL_TESTNAME, "Finished %s(%s, %s, %s, %s, %u)...\n", testName,
                shared      ?    "shared":"non-shared",
                connected   ? "connected":"disconnected",
                consuming   ? "consuming":"not-consuming",
                wellBehaved ?    "good":"bad",
                ratio);

    ismEngine_serverGlobal.DestroyNonAckerRatio = oldRatio;
}

void test_capability_nonshared_disconnected(void)
{
    test_basic(__func__, false, false, false, false, 0, ismEngine_serverGlobal.DestroyNonAckerRatio);
}
void test_capability_nonshared_connected_notconsuming(void)
{
    test_basic(__func__, false, true, false, false, 0, ismEngine_serverGlobal.DestroyNonAckerRatio);
}
void test_capability_nonshared_connected_consuming(void)
{
    test_basic(__func__, false, true, true, false, 0, ismEngine_serverGlobal.DestroyNonAckerRatio);
}
void test_capability_disable_nonAckerRatio(void)
{
    test_basic(__func__, false, false, false, false, 0, 0);
}
void test_capability_shared_disconnected(void)
{
    test_basic(__func__, true, false, false, false, 0, ismEngine_serverGlobal.DestroyNonAckerRatio);
}
void test_capability_shared_connected_notconsuming(void)
{
    test_basic(__func__, true, true, false, false, 0, ismEngine_serverGlobal.DestroyNonAckerRatio);
}
void test_capability_shared_connected_consuming(void)
{
    test_basic(__func__, true, true, true, false, 0, ismEngine_serverGlobal.DestroyNonAckerRatio);
}
void test_defect_185088_nonshared(void)
{
    // well behaved, shouldn't get killed.
    test_basic(__func__, false, true, true, true, 0, ismEngine_serverGlobal.DestroyNonAckerRatio);
}
void test_defect_185088_shared(void)
{
    // well behaved, shouldn't get killed.
    test_basic(__func__, true, true, true, true, 0, ismEngine_serverGlobal.DestroyNonAckerRatio);
}
void test_capability_publish_during_publish(void)
{
    // Nonpersistent QoS=0 not consuming (previously caused segfault)
    test_basic(__func__, true, true, false, false, 10, ismEngine_serverGlobal.DestroyNonAckerRatio);
    // Nonpersistent QoS=0 consuming (previously caused segfault)
    test_basic(__func__, true, true, true, false, 10, ismEngine_serverGlobal.DestroyNonAckerRatio);
    // Nonpersistent QoS=1 not consuming
    test_basic(__func__, true, true, false, false, 11, ismEngine_serverGlobal.DestroyNonAckerRatio);
    // Nonpersistent QoS=2 not consuming
    test_basic(__func__, true, true, false, false, 12, ismEngine_serverGlobal.DestroyNonAckerRatio);
    // Persistent QoS=0 not consuming (previously caused segfault)
    test_basic(__func__, true, true, false, false, 20, ismEngine_serverGlobal.DestroyNonAckerRatio);
    // Persistent QoS=0 consuming (previously caused segfault)
    test_basic(__func__, true, true, true, false, 20, ismEngine_serverGlobal.DestroyNonAckerRatio);
    // Persistent QoS=1 not consuming (previously caused segfault)
    test_basic(__func__, true, true, false, false, 21, ismEngine_serverGlobal.DestroyNonAckerRatio);
    // Persistent QoS=2 not consuming (previously caused segfault)
    test_basic(__func__, true, true, false, false, 22, ismEngine_serverGlobal.DestroyNonAckerRatio);
}

CU_TestInfo ISM_ExportResources_CUnit_test_capability_Basic[] =
{
     { "nonshared_disconnected",             test_capability_nonshared_disconnected },
     { "nonshared_connected_notconsuming",   test_capability_nonshared_connected_notconsuming },
     { "nonshared_connected_consuming",      test_capability_nonshared_connected_consuming },
     { "disable_nonAckerRatio",              test_capability_disable_nonAckerRatio },
     { "shared_disconnected",                test_capability_shared_disconnected },
     { "shared_connected_notconsuming",      test_capability_shared_connected_notconsuming },
     { "shared_connected_consuming",         test_capability_shared_connected_consuming },
     { "defect_185088_nonshared",            test_defect_185088_nonshared},
     { "defect_185088_shared",               test_defect_185088_shared},
     { "publish_during_publish",             test_capability_publish_during_publish},
     CU_TEST_INFO_NULL
};


/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initSuite(void)
{
    return test_engineInit(true, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termSuite(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_ExportResources_CUnit_nonAckerSuites[] =
{
    IMA_TEST_SUITE("Basic", initSuite, termSuite, ISM_ExportResources_CUnit_test_capability_Basic),
    CU_SUITE_INFO_NULL,
};


int main(int argc, char *argv[])
{
    int32_t trclvl = 0;
    uint32_t testLogLevel = testLOGLEVEL_CHECK;
    int retval = 0;
    char *adminDir=NULL;

    ism_time_t seedVal = ism_common_currentTimeNanos();

    srand(seedVal);

    CU_initialize_registry();

    //We're going to put a lot of persistent messages - want to use the thread jobs model
    setenv("IMA_TEST_TRANSACTION_THREAD_JOBS_CLIENT_MINIMUM", "1", 0);

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, adminDir);

    char localAdminDir[1024];
    if (adminDir == NULL && test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir)) == true)
    {
        adminDir = localAdminDir;
    }

    if (retval == 0)
    {
        CU_register_suites(ISM_ExportResources_CUnit_nonAckerSuites);

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

    test_log(2, "Success. Test complete!\n");
    test_processTerm(false);
    test_removeAdminDir(adminDir);

    return retval;
}
