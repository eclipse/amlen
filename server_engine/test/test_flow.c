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
/* Module Name: test_pubflowctl                                     */
/*                                                                  */
/* Description: This file contains a testcase for testing publish   */
/*              flow control for QoS 2 messages.                    */
/*                                                                  */
/*              The engine will only allow a maximum of 'N' unack'd */
/*              messages to be sent to the consumer. This test      */
/*              verifies that.                                      */
/*              - The number of unack'd messages never exceeds 'N'  */
/*              - messages are resent when they are nack'd          */
/*                                                                  */
/*              Design                                              */
/*              ------                                              */
/*                                                                  */
/*              main()                                              */
/*                  Create consumer on topic                        */
/*                  Start AckExpiryThread()                         */
/*                  Start consuming messages                        */
/*                  Publish 'M' messages (where M > N)              */
/*                  Wait to be posted when consumer receives msgs   */
/*                                                                  */
/*              ConsumerCallback()                                  */
/*                  lock consumer mutex                             */
/*                  check unack'd msgCount < N                      */
/*                  if msg->redliveryCount == 0                     */
/*                      Add msg to ack-list                         */
/*                  else if msg->redeliverCount == threshold        */
/*                      Ack message (rcv, consume)                  */
/*                  else                                            */
/*                      find msg in ack list, remove and add to end */
/*                  fi                                              */
/*                  unlock consumer mutex                           */
/*                                                                  */
/*              AckExpiryThread()                                   */
/*                  lock consumer mutex                             */
/*                  while (consumedCount < expectedCount)           */
/*                      look at receive time of first/next message  */
/*                      if rcvtime + 5 seconds has expired          */
/*                          nack message                            */
/*                          continue                                */
/*                      else                                        */
/*                          pthread_cond_timedwait until next exp.  */
/*                                                                  */
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
#include "policyInfo.h"
#include "store.h"

#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_task.h"

/*********************************************************************/
/* Global data                                                       */
/*********************************************************************/
static ismEngine_ClientStateHandle_t hClient = NULL;
// 0   Silent
// 1   Test Name only
// 2   + Test Description
// 3   + Validation checks
// 4   + Test Progress
// 5   + Verbose Test Progress
static uint32_t verboseLevel = 4;   /* 0-9 */

/*********************************************************************/
/* Name:        verbose                                              */
/* Description: Debugging printf routine                             */
/* Parameters:                                                       */
/*     level               - Debug level of the text provided        */
/*     format              - Format string                           */
/*     ...                 - Arguments to format string              */
/*********************************************************************/
static void verbose(int level, char *format, ...)
{
    static char indent[]="--------------------";
    va_list ap;

    if (verboseLevel >= level)
    {
        va_start(ap, format);
        if (format[0] != '\0')
        {
            printf("%.*s ", level+1, indent);
            vprintf(format, ap);
        }
        printf("\n");
        va_end(ap);
    }
}

typedef struct tag_Producer
{
    int32_t producerId;
    char *TopicString;
    char *cpuArray;
    int cpuCount;
    int32_t QoS;
    pthread_t hThread;
    ismEngine_ClientStateHandle_t hClient;
    uint32_t msgsToSend;
    uint32_t msgsSent;
} Producer_t;

static void putComplete(int32_t rc, void *handle, void *pContext)
{
   TEST_ASSERT_EQUAL(rc, OK);
   Producer_t *Producer = *(Producer_t **)pContext;
   ASYNCPUT_CB_ADD_AND_FETCH(&(Producer->msgsSent), 1);
}
/****************************************************************************/
/* Publish operation                                                        */
/****************************************************************************/
void PublishMessages(Producer_t *Producer)
{
    int rc = 0;
    uint32_t msgCount=0;
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismEngine_SessionHandle_t hSession = NULL;
    ismEngine_ProducerHandle_t hProducer;
    ismEngine_MessageHandle_t  hMessage;
    header.Reliability = Producer->QoS ;
    header.Persistence = (Producer->QoS == 0) ? ismMESSAGE_PERSISTENCE_NONPERSISTENT
                                              : ismMESSAGE_PERSISTENCE_PERSISTENT;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    (void)ism_common_setAffinity(pthread_self(), Producer->cpuArray, Producer->cpuCount);

    /************************************************************************/
    /* Create a session for the producer                                    */
    /************************************************************************/
    rc=ism_engine_createSession( Producer->hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc=ism_engine_createProducer( hSession
                                , ismDESTINATION_TOPIC
                                , Producer->TopicString
                                , &hProducer
                                , NULL
                                , 0
                                , NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    verbose(4, "Created publisher");

    /************************************************************************/
    /* Deliver the messages                                                 */
    /************************************************************************/
    for(; msgCount < Producer->msgsToSend; msgCount++)
    {
        ismMessageAreaType_t areaTypes[1];
        size_t areaLengths[1];
        void *areas[1];
        char Buffer[1024];

        sprintf(Buffer, "Consumer_test - Producer(%d) Message(%d)", Producer->producerId, msgCount);
        areaTypes[0] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[0] = strlen(Buffer) +1;
        areas[0] = (void *)Buffer;

        rc=ism_engine_createMessage( &header
                                   , 1
                                   , areaTypes
                                   , areaLengths
                                   , areas
                                   , &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);

        rc=ism_engine_putMessage( hSession
                                , hProducer
                                , NULL
                                , hMessage
                                , &Producer
                                , sizeof(Producer_t *)
                                , putComplete);

        TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                                ("rc was %d",  rc));

        if (rc == OK)
        {
            ASYNCPUT_CB_ADD_AND_FETCH(&(Producer->msgsSent), 1);
        }
    }

    verbose(4, "Published %d messages", msgCount);

    test_incrementActionsRemaining(pActionsRemaining, 2);
    rc = ism_engine_destroyProducer( hProducer,
                                     &pActionsRemaining,
                                     sizeof(pActionsRemaining),
                                     test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroySession( hSession
                                  , &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    while((volatile int32_t)Producer->msgsSent < Producer->msgsToSend)
    {
        sched_yield();
    }

    test_waitForRemainingActions(pActionsRemaining);

    return;
}


/****************************************************************************/
/* ConsumerCallback                                                         */
/****************************************************************************/
typedef struct _MsgDetails_t
{
    ismEngine_MessageHandle_t  hMsg;
    ismEngine_DeliveryHandle_t hDelivery;
    uint32_t                   deliveryId;
    uint32_t                   deliveryCount;
    uint64_t                   rcvTime;
    ismMessageState_t          state;
    struct _MsgDetails_t       *Next;
    struct _MsgDetails_t       *Prev;
} MsgDetails_t;

typedef struct _Consumer_t
{
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    pthread_mutex_t ConsumerMutex;
    pthread_cond_t ConsumerCond;
    int ConsumerListSeqNo;
    char *SubName;
    char *TopicString;
    uint32_t consumerSeqNum;
    uint32_t MsgsExpected;
    uint32_t MsgsReceived;
    uint32_t MsgsRedelivered;
    uint32_t MsgsConsumed;
    uint32_t UnackdMsgCount;
    MsgDetails_t *pMsgListHead;
    MsgDetails_t *pMsgListTail;
    bool ConsumerStopped;
} Consumer_t;

#define MAX_OUTSTANDING_ACKS 1000

bool ConsumerCallback( ismEngine_ConsumerHandle_t hConsumer
                     , ismEngine_DeliveryHandle_t hDelivery
                     , ismEngine_MessageHandle_t  hMessage
                     , uint32_t                   deliveryId
                     , ismMessageState_t          state
                     , uint32_t                   destinationOptions
                     , ismMessageHeader_t *       pMsgDetails
                     , uint8_t                    areaCount
                     , ismMessageAreaType_t       areaTypes[areaCount]
                     , size_t                     areaLengths[areaCount]
                     , void *                     pAreaData[areaCount]
                     , void *                     pConsumerContext)
{
    int urc;
    Consumer_t *pConsumer = *(Consumer_t **)pConsumerContext;
    MsgDetails_t *pCurMsg = NULL;
    struct timespec curtime;

    urc = pthread_mutex_lock( &pConsumer->ConsumerMutex);
    TEST_ASSERT_EQUAL(urc, 0);

    pConsumer->ConsumerListSeqNo++; // We will be changing the Msglist;

    clock_gettime(CLOCK_REALTIME, &curtime);

#if 0
    // CHEAT-START
    if (pMsgDetails->RedeliveryCount == 0)
    {
        uint32_t rc;

        deliveryId = ++pConsumer->consumerSeqNum;

        rc = ism_engine_setMessageDeliveryId( pConsumer->hConsumer
                                            , hDelivery
                                            , deliveryId
                                            , NULL
                                            , 0
                                            , NULL );

        TEST_ASSERT_EQUAL(rc, OK);
    }
    // CHEAT-END
#endif

    verbose(5, "Received message %d - '%s'", deliveryId, pAreaData[0]);

    // If this is the first time a message has been delivered, then
    // add it to our list of unack'd messages
    if (pMsgDetails->RedeliveryCount == 0)
    {
        TEST_ASSERT(pConsumer->UnackdMsgCount <= MAX_OUTSTANDING_ACKS,
                    ("We have been delivered a message when our unack'd message count (%d) has been reached", pConsumer->UnackdMsgCount));

        pConsumer->MsgsReceived++;
        __sync_fetch_and_add(&pConsumer->UnackdMsgCount, 1);

        pCurMsg = pConsumer->pMsgListHead;
        while ((pCurMsg != NULL) && (pCurMsg->deliveryId != deliveryId))
        {
            pCurMsg=pCurMsg->Next;
        }
        TEST_ASSERT_EQUAL(pCurMsg, NULL);

        // First time this consumer has seen this msg
        pCurMsg=(MsgDetails_t *)malloc(sizeof(MsgDetails_t));

        pCurMsg->hMsg = hMessage;
        pCurMsg->hDelivery = hDelivery;
        pCurMsg->deliveryId = deliveryId;
        pCurMsg->deliveryCount = 1;
        pCurMsg->state = state;
        pCurMsg->rcvTime = curtime.tv_sec;
        pCurMsg->Next = NULL;
        pCurMsg->Prev = NULL;
        if (pConsumer->pMsgListHead == NULL)
        {
            pConsumer->pMsgListHead=pCurMsg;
            pConsumer->pMsgListTail=pCurMsg;
        }
        else
        {
           pConsumer->pMsgListTail->Next = pCurMsg;
           pCurMsg->Prev = pConsumer->pMsgListTail;
           pConsumer->pMsgListTail=pCurMsg;
        }
    }
    else
    {
        pCurMsg = pConsumer->pMsgListHead;
        while ((pCurMsg != NULL) && (pCurMsg->deliveryId != deliveryId))
        {
            pCurMsg=pCurMsg->Next;
        }
        TEST_ASSERT_NOT_EQUAL(pCurMsg, NULL);

        pConsumer->MsgsRedelivered++;
        __sync_fetch_and_add(&pConsumer->UnackdMsgCount, 1);

        // Remove the message for the list of unack'd messages
        if (pCurMsg->Prev == NULL)
            pConsumer->pMsgListHead=pCurMsg->Next;
        else
            pCurMsg->Prev->Next=pCurMsg->Next;
        if (pCurMsg->Next == NULL)
            pConsumer->pMsgListTail=pCurMsg->Prev;
        else
            pCurMsg->Next->Prev=pCurMsg->Prev;

        // Read-add the message to the list of unack'd messages
        if (pConsumer->pMsgListHead == NULL)
        {
            pConsumer->pMsgListHead=pCurMsg;
            pConsumer->pMsgListTail=pCurMsg;
        }
        else
        {
           pConsumer->pMsgListTail->Next = pCurMsg;
           pCurMsg->Prev = pConsumer->pMsgListTail;
           pCurMsg->Next = NULL;
           pConsumer->pMsgListTail=pCurMsg;
        }

        pCurMsg->deliveryCount++;
        pCurMsg->hDelivery = hDelivery;
        pCurMsg->rcvTime = curtime.tv_sec;
    }

#if 0
    // Check loop for integrity
    {
        MsgDetails_t *pMsg = pConsumer->pMsgListHead;
        uint32_t counter;

        while (pMsg != NULL)
        {
            counter++;
            pMsg=pMsg->Next;
            TEST_ASSERT(counter < 10005, ("counter was %d", counter));
        }
    }
#endif

    urc = pthread_mutex_unlock( &pConsumer->ConsumerMutex);
    TEST_ASSERT_EQUAL(urc, 0);

    ism_engine_releaseMessage(hMessage);

    return true;
}


/****************************************************************************/
/* ConsumerCallback                                                         */
/****************************************************************************/
void *AckExpiryThread(void *arg)
{
    int urc;
    uint32_t rc;
    Consumer_t *pConsumer = (Consumer_t *)arg;
    struct timespec curtime;
    MsgDetails_t *pCurMsg = NULL;
    char ThreadName[16];
    int SeqNo;

    strcpy(ThreadName, "AckExpiryThread");
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&ThreadName);

    ism_engine_threadInit(0);

    while (pConsumer->MsgsConsumed < pConsumer->MsgsExpected)
    {
        urc = pthread_mutex_lock( &pConsumer->ConsumerMutex);
        TEST_ASSERT_EQUAL(urc, 0);

        clock_gettime(CLOCK_REALTIME, &curtime);

        pCurMsg = pConsumer->pMsgListHead;
        SeqNo = pConsumer->ConsumerListSeqNo;

        while (pCurMsg != NULL)
        {
            // We will positively ack after 2 delivery attempts
            if (pCurMsg->deliveryCount == 2)
            {
                // We must release the lock around this call
                urc = pthread_mutex_unlock( &pConsumer->ConsumerMutex );
                TEST_ASSERT_EQUAL(urc, 0);

                verbose(6, "Acking %u.", pCurMsg->deliveryId);

                rc = sync_ism_engine_confirmMessageDelivery( pConsumer->hSession
                                                      , NULL
                                                      , pCurMsg->hDelivery
                                                      , ismENGINE_CONFIRM_OPTION_RECEIVED );
                TEST_ASSERT_EQUAL(rc, OK);

                rc = sync_ism_engine_confirmMessageDelivery( pConsumer->hSession
                                                           , NULL
                                                           , pCurMsg->hDelivery
                                                           , ismENGINE_CONFIRM_OPTION_CONSUMED);
                TEST_ASSERT_EQUAL(rc, OK);

                urc = pthread_mutex_lock( &pConsumer->ConsumerMutex );
                TEST_ASSERT_EQUAL(urc, 0);


                pCurMsg->deliveryCount = 0;
                pCurMsg->hDelivery = 0;

                __sync_fetch_and_sub(&pConsumer->UnackdMsgCount, 1);
                pConsumer->MsgsConsumed++;
            }
            else if ((pCurMsg->hDelivery != 0) &&
                     (pCurMsg->rcvTime + 3 < curtime.tv_sec))
            {
                __sync_fetch_and_sub(&pConsumer->UnackdMsgCount, 1);
                ismEngine_DeliveryHandle_t hDelivery = pCurMsg->hDelivery;
                pCurMsg->hDelivery = 0;

                // We must release the lock around tis call
                urc = pthread_mutex_unlock( &pConsumer->ConsumerMutex );
                TEST_ASSERT_EQUAL(urc, 0);

                verbose(6, "Nacking %u.", pCurMsg->deliveryId);

                // This message has been unack'd for 3 seconds (ish) so nack it now.
                rc = ism_engine_confirmMessageDelivery( pConsumer->hSession
                                                      , NULL
                                                      , hDelivery
                                                      , ismENGINE_CONFIRM_OPTION_NOT_RECEIVED
                                                      , NULL
                                                      , 0
                                                      , NULL );
                TEST_ASSERT_EQUAL(rc, OK);

                urc = pthread_mutex_lock( &pConsumer->ConsumerMutex );
                TEST_ASSERT_EQUAL(urc, 0);

            }
            else if (pCurMsg->hDelivery != 0)
                break; // Break out of our search for messages as remaining msg
                       // arrived less than 3 seconds ago

            if (pConsumer->ConsumerListSeqNo != SeqNo)
            {
                // Chain changed so reset scan
                pCurMsg = pConsumer->pMsgListHead;
                SeqNo = pConsumer->ConsumerListSeqNo;

                clock_gettime(CLOCK_REALTIME, &curtime);
            }
            else
            {
                pCurMsg = pCurMsg->Next;
            }
        }

        urc = pthread_mutex_unlock( &pConsumer->ConsumerMutex );
        TEST_ASSERT_EQUAL(urc, 0);

        sleep(1);
    }

    urc = pthread_mutex_lock( &pConsumer->ConsumerMutex );
    TEST_ASSERT_EQUAL(urc, 0);

    pConsumer->ConsumerStopped = true;

    // And post the main thread
    urc = pthread_cond_signal(&pConsumer->ConsumerCond);
    TEST_ASSERT_EQUAL(urc, 0);

    urc = pthread_mutex_unlock( &pConsumer->ConsumerMutex );
    TEST_ASSERT_EQUAL(urc, 0);

    verbose(4, "AckExpiryThread ending.");

    return NULL;
}

/*********************************************************************/
/* TestCase:        Test_PubFlowCtl_TT                               */
/*********************************************************************/
void Test_PubFlowCtl_TT( void )
{
    uint32_t rc;
    int urc;
    uint32_t numMsgs = 1000;
    char TopicString[]="/Topic/Test_PubFlowCtl_TT";
    char SubName[]="Test_PubFlowCtl_TT";
    Consumer_t Consumer = { NULL };
    Consumer_t *pConsumer = &Consumer;
    Producer_t Producer = { 0 };
    pthread_t hAckExpiryThread;

    Consumer.SubName = SubName;
    Consumer.TopicString = TopicString;
    Consumer.MsgsExpected = numMsgs;
    urc=pthread_cond_init(&Consumer.ConsumerCond, NULL);
    TEST_ASSERT_EQUAL(urc, 0);
    urc=pthread_mutex_init(&Consumer.ConsumerMutex, NULL);
    TEST_ASSERT_EQUAL(urc, 0);

    // Create a session for the consumer
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &Consumer.hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    verbose(4, "Created consumer session");

    // And create the subscription
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };
    rc=sync_ism_engine_createSubscription( hClient
                                         , pConsumer->SubName
                                         , NULL
                                         , ismDESTINATION_TOPIC
                                         , pConsumer->TopicString
                                         , &subAttrs
                                         , NULL ); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    // Create the consumer for the subscription
    rc=ism_engine_createConsumer( pConsumer->hSession
                                , ismDESTINATION_SUBSCRIPTION
                                , pConsumer->SubName
                                , NULL
                                , NULL // Owning client same as session client
                                , &pConsumer
                                , sizeof(pConsumer)
                                , ConsumerCallback
                                , NULL
                                , test_getDefaultConsumerOptions(subAttrs.subOptions)
                                , &pConsumer->hConsumer
                                , NULL
                                , 0
                                , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And start message delivery
    rc=ism_engine_startMessageDelivery( pConsumer->hSession
                                      , 0
                                      , NULL
                                      , 0
                                      , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Having initialised the consumer, now start the thread which will
    // expire unack'd messages
    urc = test_task_startThread( &hAckExpiryThread ,AckExpiryThread, &Consumer ,"AckExpiryThread");
    TEST_ASSERT(urc == 0, ("Failed to start os thread. osrc=%d\n", urc));

    // Publish the messages
    Producer.cpuCount = 0;
    Producer.producerId = 1;
    Producer.TopicString = TopicString;
    Producer.hClient = hClient;
    Producer.msgsToSend = numMsgs;
    Producer.msgsSent = 0;
    Producer.QoS = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;

    PublishMessages(&Producer);

    // And now wait for the consumer to post us that all of the messages
    // have been received
    urc = pthread_mutex_lock( &Consumer.ConsumerMutex);
    TEST_ASSERT_EQUAL(urc, 0);

    do
    {
        struct timespec ts;

        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;

        if (Consumer.ConsumerStopped == false)
        {
            urc = pthread_cond_timedwait( &Consumer.ConsumerCond,
                                          &Consumer.ConsumerMutex,
                                          &ts);
            TEST_ASSERT((urc == 0) || (urc == ETIMEDOUT),
                        ("Unexpected rc (%d) from pthread_cond_timedwait", urc));
        }

        verbose(4, "Received(%d) Redelivered(%d) Unack'd(%d) Consumed(%d)",
                Consumer.MsgsReceived,
                Consumer.MsgsRedelivered,
                Consumer.UnackdMsgCount,
                Consumer.MsgsConsumed);

    } while (urc == ETIMEDOUT && Consumer.ConsumerStopped == false);

    // And we are done!
    urc = pthread_mutex_unlock( &pConsumer->ConsumerMutex );
    TEST_ASSERT_EQUAL(urc, 0);

    // Ensure that ack expiry thread has ended. 
    urc = pthread_join( hAckExpiryThread, NULL);
    TEST_ASSERT_EQUAL(urc, 0);

    rc=ism_engine_destroyConsumer( Consumer.hConsumer
                                 , NULL
                                 , 0
                                 , NULL );
    TEST_ASSERT_EQUAL(rc, OK);
    verbose(4, "Consumer has been destroyed");


    // Now we must destroy the subscription
    rc = ism_engine_destroySubscription( hClient
                                       , SubName
                                       , hClient
                                       , NULL
                                       , 0
                                       , NULL );
    TEST_ASSERT_EQUAL(rc, OK);
    verbose(4, "Durable subscription has been destroyed");

    // Destroy the session
    rc=ism_engine_destroySession( Consumer.hSession
                                , NULL
                                , 0
                                , NULL );

    // Free the list of messages we've been building up
    MsgDetails_t *pCurMsg = Consumer.pMsgListHead;
    while(pCurMsg != NULL)
    {
        MsgDetails_t *pNextMsg = pCurMsg->Next;
        free(pCurMsg);
        pCurMsg = pNextMsg;
    }

    TEST_ASSERT_EQUAL(rc, OK);
    verbose(4, "Session has been destroyed");

    return;
}


CU_TestInfo PubFlowCtl_TT[] = {
    { "Test_PubFlowCtl_TT",          Test_PubFlowCtl_TT },
    CU_TEST_INFO_NULL
};

int initSession(void)
{
    int32_t rc;
    ism_field_t f;

    ism_prop_t *staticConfigProps = ism_common_getConfigProperties();

    //Allow MQTT clients to have 2000 unacked msgs
    f.type = VT_Integer;
    f.val.i = 2000;
    rc = ism_common_setProperty( staticConfigProps
                               , ismENGINE_CFGPROP_MAX_MQTT_UNACKED_MESSAGES
                               , &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s returned %d\n", ismENGINE_CFGPROP_MAX_MQTT_UNACKED_MESSAGES, rc);
        goto mod_exit;
    }

    rc = test_engineInit_DEFAULT;
    if (rc != OK) goto mod_exit;

    rc = ism_engine_createClientState("AckSession Test",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);

    if (rc != OK)
    {
        printf("ERROR:  ism_engine_createClientState() returned %d\n", rc);
    }

    /************************************************************************/
    /* Set the default policy info to allow deep queues                     */
    /************************************************************************/
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 1000000;

mod_exit:

    return rc;
}

int termSession(void)
{
    return test_engineTerm(true);
}

/*********************************************************************/
/* TestSuite definition                                              */
/*********************************************************************/
CU_SuiteInfo ISM_Engine_CUnit_PubFlowCtl_Suites[] = {
    IMA_TEST_SUITE("Test_PubFlowCtl_TT", initSession, termSession, PubFlowCtl_TT ),
    CU_SUITE_INFO_NULL,
};

int main(int argc, char *argv[])
{
    int trclvl = 0;
    int retval = 0;

    // Level of output
    if (argc >= 2)
    {
        int32_t level=atoi(argv[1]);
        if ((level >=0) && (level <= 9))
        {
            verboseLevel = level;
        }
    }

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();
    CU_register_suites(ISM_Engine_CUnit_PubFlowCtl_Suites);

    CU_basic_run_tests();

    CU_RunSummary * CU_pRunSummary_Final;
    CU_pRunSummary_Final = CU_get_run_summary();
    printf("\n\n[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
            CU_pRunSummary_Final->nTestsRun,
            CU_pRunSummary_Final->nTestsFailed,
            CU_pRunSummary_Final->nAssertsFailed);
    if ((CU_pRunSummary_Final->nTestsFailed > 0) ||
        (CU_pRunSummary_Final->nAssertsFailed > 0))
    {
        retval = 1;
    }

    CU_cleanup_registry();

    int32_t rc = test_processTerm(retval == 0);
    if (retval == 0) retval = rc;

mod_exit:

    return retval;
}
