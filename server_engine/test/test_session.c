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
/* Module Name: test_session                                        */
/*                                                                  */
/* Description: This file contains a series of tests for testing    */
/*              the operation of acknowledging messages as the      */
/*              consumer is being destroyed.                        */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <getopt.h>

#include <ismutil.h>
#include "engine.h"
#include "engineInternal.h"
#include "policyInfo.h"
#include "store.h"

#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

/*********************************************************************/
/* Global data                                                       */
/*********************************************************************/
// 0   Silent
// 1   Test Name only
// 2   + Test Description
// 3   + Validation checks
// 4   + Test Progress
// 5   + Verbose Test Progress
static uint32_t verboseLevel = 3;   /* 0-9 */

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
/********************************************************************/
/* Local types                                                      */
/********************************************************************/
typedef enum 
{
    Never = 0,
    OnMsgReceived,
    OnMsgReceived2,
    AfterAllMsgsReceived,
    AfterConsumerDestroyed,
    AfterSubDestroyed
} WhenToAck_t;

typedef struct tag_UnackdMsg
{
    ismEngine_MessageHandle_t  hMessage;
    ismEngine_DeliveryHandle_t hDelivery;
    uint32_t                   deliveryId;
    struct tag_UnackdMsg       *pNext;
} UnackdMsg_t;

typedef struct tag_Consumer
{
    char *SubName;
    char *TopicString;
    bool Durable;
    bool JMSSub;
    WhenToAck_t whenToAck;
    enum { Stopped, Running } State;
    int32_t QoS;
    uint32_t expectedMsgCount;
    uint32_t MsgsReceived;
    uint32_t MsgsRedelivered;
    uint32_t MsgsConsumed;
    uint32_t consumerSeqNum;
    uint32_t retCode;
    uint32_t stopConsumerRatio;
    uint32_t nackRatio;
    pthread_mutex_t ConsumerMutex;
    pthread_cond_t MsgsReceivedCond;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    UnackdMsg_t *pUnackdMsgs;
    uint32_t UnackdCount;
} Consumer_t;

typedef struct tag_Producer 
{
    int32_t producerId;
    char *TopicString;
    char *cpuArray;
    int cpuCount;
    int32_t QoS;
    pthread_t hThread;
    uint32_t msgsToSend;
    uint32_t msgsSent;
} Producer_t;

typedef struct tag_recvAckInfight_t {
    ismEngine_SessionHandle_t hSession;
    Consumer_t *pConsumer;
    ismEngine_DeliveryHandle_t hDelivery;
    uint32_t FailurePoint;
    bool doSynchronous;
} recvAckInfight_t;

/********************************************************************/
/* Global variables                                                 */
/********************************************************************/

void messageDelivered(Consumer_t *pConsumer, bool increaseConsumedCount, bool takeLock)
{
    int urc;
    uint32_t rc;

    if (takeLock)
    {
        // Lock the consumer mutex
        urc = pthread_mutex_lock( &pConsumer->ConsumerMutex);
        TEST_ASSERT_EQUAL(urc, 0);
    }

    if (increaseConsumedCount)
    {
        __sync_add_and_fetch(&(pConsumer->MsgsConsumed), 1);
    }

    if (pConsumer->MsgsConsumed == pConsumer->expectedMsgCount)
    {
        verbose(4, "All messages received. Posting MsgsReceivedCond");

        pConsumer->State = Stopped;

        urc = pthread_cond_signal(&pConsumer->MsgsReceivedCond);
        TEST_ASSERT_EQUAL(urc, 0);

    }
    else if ((pConsumer->stopConsumerRatio) &&
             ((rand() % pConsumer->stopConsumerRatio) == 0))
    {
        verbose(4, "Stop ratio reached. Posting MsgsReceivedCond");

        // And stop message delivery. This call can go asynchronous, but as
        // all messages should have been delivered there is no reason for it
        // to go async.
        rc = ism_engine_stopMessageDelivery( pConsumer->hSession
                                           , NULL
                                           , 0
                                           , NULL );
        TEST_ASSERT_EQUAL(rc, OK);

        verbose(4, "stopMessageDelivery called. rc = %d", rc);

        pConsumer->State = Stopped;

        urc = pthread_cond_signal(&pConsumer->MsgsReceivedCond);
        TEST_ASSERT_EQUAL(urc, 0);
    }

    if (takeLock)
    {
        urc = pthread_mutex_unlock( &pConsumer->ConsumerMutex);
        TEST_ASSERT_EQUAL(urc, 0);
    }
}

void consumeCompleted(int32_t retcode, void *handle, void *context)
{
    Consumer_t *pConsumer = *(Consumer_t **)context;

    //Now we've finished acking the message we can do anything else we've been meaning to check
    messageDelivered(pConsumer, true, true);
}

int32_t startConsume( ismEngine_SessionHandle_t hSession
                    , Consumer_t *pConsumer
                    , ismEngine_DeliveryHandle_t hDelivery
                    , uint32_t FailurePoint
                    , bool doSynchronous
                    , bool *pStillLocked)
{
    int32_t rc = OK;

    if (FailurePoint > 0)
    {
        goto mod_exit;
    }

    if (*pStillLocked)
    {
        *pStillLocked = false;

        int urc = pthread_mutex_unlock( &pConsumer->ConsumerMutex);
        TEST_ASSERT_EQUAL(urc, 0);
    }

    if (doSynchronous)
    {
        rc = sync_ism_engine_confirmMessageDelivery( hSession
                                                   , NULL
                                                   , hDelivery
                                                   , ismENGINE_CONFIRM_OPTION_CONSUMED);
    }
    else
    {
        rc = ism_engine_confirmMessageDelivery( hSession
                                              , NULL
                                              , hDelivery
                                              , ismENGINE_CONFIRM_OPTION_CONSUMED
                                              , &pConsumer, sizeof(pConsumer)
                                              , consumeCompleted);
        if (rc == ISMRC_AsyncCompletion)
        {
            goto mod_exit;
        }
    }
    TEST_ASSERT_EQUAL(rc, OK);

    consumeCompleted(rc, NULL, &pConsumer);

mod_exit:
    return rc;
}

void receiveCompleted(int32_t retcode, void *handle, void *context)
{
    bool stillLocked = false;
    recvAckInfight_t *pAckData = (recvAckInfight_t *)context;

    startConsume( pAckData->hSession
                , pAckData->pConsumer
                , pAckData->hDelivery
                , pAckData->FailurePoint
                , pAckData->doSynchronous
                , &stillLocked);

}

/****************************************************************************/
/* AcknowledgeMsWg                                                           */
/****************************************************************************/
int32_t AcknowledgeMsg( Consumer_t *pConsumer
                      , ismEngine_DeliveryHandle_t hDelivery
                      , uint32_t deliveryId
                      , ismMessageState_t state
                      , ismEngine_MessageHandle_t hMessage
                      , bool Positive
                      , bool doSynchronous
                      , bool *pStillLocked)
{
    uint32_t rc = OK;
    uint32_t Option = ismENGINE_CONFIRM_OPTION_RECEIVED;
    uint32_t FailurePoint = 0;

    *pStillLocked = true;

    verbose(5, "Acknowledging Message DeliveryId(%d) State=(%d)- %s",
            deliveryId, state, ((ismEngine_Message_t *)hMessage)->pAreaData[0]);

    if (!Positive)
    {
        // FailurePoint 0 -Don't fail
        // FailurePoint 1 -Don't send any acknowledgement
        // FailurePoint 2 -For QoS 2 send ismENGINE_CONFIRM_OPTION_NOT_DELIVERED
        // FailurePoint 3 -send ismENGINE_CONFIRM_OPTION_NOT_RECEIVED
        // FailurePoint 4 -Don't send ismENGINE_CONFIRM_OPTION_CONSUMED
        FailurePoint = (rand() % 4) +1;
    }
      
    if (FailurePoint == 1)
    {
        goto mod_exit;
    }
    if ((pConsumer->QoS == 2) && (state == ismMESSAGE_STATE_DELIVERED))
    {
        if (FailurePoint == 2) 
            Option = ismENGINE_CONFIRM_OPTION_NOT_DELIVERED;
        else if (FailurePoint == 3)
            Option = ismENGINE_CONFIRM_OPTION_NOT_RECEIVED;

        // For Exactly-Once we must first acknowledge that the
        // message has been received, before we identify that the
        // message has been consumed.

        recvAckInfight_t ackData = { pConsumer->hSession
                                   , pConsumer
                                   , hDelivery
                                   , FailurePoint
                                   , doSynchronous};

        *pStillLocked = false;

        int urc = pthread_mutex_unlock( &pConsumer->ConsumerMutex);
        TEST_ASSERT_EQUAL(urc, 0);

        if (doSynchronous)
        {
            rc = sync_ism_engine_confirmMessageDelivery( pConsumer->hSession
                                                       , NULL
                                                       , hDelivery
                                                       , Option);
        }
        else
        {
            rc = ism_engine_confirmMessageDelivery( pConsumer->hSession
                                                  , NULL
                                                  , hDelivery
                                                  , Option
                                                  , &ackData, sizeof(ackData)
                                                  , receiveCompleted);
            if(rc == ISMRC_AsyncCompletion)
            {
                goto mod_exit;
            }
        }
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = startConsume( pConsumer->hSession
                     , pConsumer
                     , hDelivery
                     , FailurePoint
                     , doSynchronous
                     , pStillLocked);


mod_exit:
    // After we release this message we cannot access the hMessage
    ism_engine_releaseMessage(hMessage);
    return rc;
}

/****************************************************************************/
/* ConsumerCallback                                                         */
/****************************************************************************/
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
    uint32_t rc;
    Consumer_t *pConsumer = *(Consumer_t **)pConsumerContext;

    // Lock the consumer mutex
    urc = pthread_mutex_lock( &pConsumer->ConsumerMutex);
    TEST_ASSERT_EQUAL(urc, 0);
    bool locked = true;
    bool callMessageDelivered = true;
    bool increaseConsumedCount = false;

    pConsumer->MsgsReceived++;

    if (pMsgDetails->RedeliveryCount)
    {
        pConsumer->MsgsRedelivered++;
    }

    // If this is the first time a message has been delivered, we
    // expect the delivery id to be initalised to zero and we must
    // set an appropriate delivery Id. 
    if (pMsgDetails->RedeliveryCount == 0)
    {
        verbose(5, "Received Message(%d) state(%d) - %s", deliveryId, state, (char *)pAreaData[0]);
    }
    else
    {
        verbose(4, "Received redelivered(%d) Message(%d) - %s",
                pMsgDetails->RedeliveryCount, deliveryId, (char *)pAreaData[0]);
    }

    switch (state)
    {
        case ismMESSAGE_STATE_DELIVERED:
            if ((pConsumer->whenToAck == OnMsgReceived) ||
                ((pConsumer->whenToAck == OnMsgReceived2) && (pMsgDetails->RedeliveryCount > 0)))
            {
                bool Positive = true;
                if ((pConsumer->nackRatio > 0) &&
                    (pConsumer->MsgsReceived % pConsumer->nackRatio) == 0)
                {
                    Positive = false;
                }
                //We can't finish with this messages until the acks have (asyncly) completed
                callMessageDelivered = false;

                rc = AcknowledgeMsg( pConsumer
                                   , hDelivery
                                   , deliveryId
                                   , state
                                   , hMessage
                                   , Positive
                                   , false
                                   , &locked);

                if (rc == ISMRC_AsyncCompletion)
                {
                    goto mod_exit;
                }
            }
            else
            {
                // We are not ack'ing messages, so we must add this message
                // to the linked list of messages received.
                UnackdMsg_t *pUnackdMsg;

                pUnackdMsg=(UnackdMsg_t *)malloc(sizeof(UnackdMsg_t));

                pUnackdMsg->hDelivery = hDelivery;
                pUnackdMsg->hMessage = hMessage;
                pUnackdMsg->deliveryId = deliveryId;
                pUnackdMsg->pNext  = pConsumer->pUnackdMsgs;
                pConsumer->pUnackdMsgs = pUnackdMsg;
                pConsumer->UnackdCount++;
            }
            break;
        case ismMESSAGE_STATE_RECEIVED:
            {
                bool Positive = true;
                if ((pConsumer->nackRatio > 0) &&
                    (pConsumer->MsgsReceived % pConsumer->nackRatio) == 0)
                {
                    Positive = false;
                }

                //We can't finish with this messages until the acks have (asyncly) completed
                callMessageDelivered = false;

                rc = AcknowledgeMsg( pConsumer
                                  , hDelivery
                                  , deliveryId
                                  , state
                                  , hMessage
                                  , Positive
                                  , false
                                  , &locked);
                if (rc == ISMRC_AsyncCompletion)
                {
                    goto mod_exit;
                }
            }
            break;
        case ismMESSAGE_STATE_CONSUMED:
            increaseConsumedCount = true;
            break;
        default:
            fprintf(stderr, "state is %d\n", state);
            TEST_ASSERT(false, ("Unexpected state %d", state));
            break;
    }

    if (callMessageDelivered)
    {
        messageDelivered(pConsumer, increaseConsumedCount, !locked);
    }
    else if (increaseConsumedCount)
    {
        __sync_add_and_fetch(&pConsumer->MsgsConsumed, 1);
    }

    if (locked)
    {
        urc = pthread_mutex_unlock( &pConsumer->ConsumerMutex);
        TEST_ASSERT_EQUAL(urc, 0);
    }
mod_exit:
    return true;
}

/****************************************************************************/
/* FreeAckList                                                              */
/****************************************************************************/
void FreeAckList( Consumer_t *pConsumer
                , bool Acknowledge )
{
    verbose(4, "Freeing Acklist(size %d) Acknowledge(%s)",
            pConsumer->UnackdCount, Acknowledge?"true":"false");

    while (pConsumer->pUnackdMsgs != NULL)
    {
        UnackdMsg_t *pUnackdMsg = pConsumer->pUnackdMsgs;
        pConsumer->pUnackdMsgs = pUnackdMsg->pNext;
        pConsumer->UnackdCount--;

        if (Acknowledge)
        {
            bool stillLocked = true;

            int32_t rc = AcknowledgeMsg( pConsumer
                                       , pUnackdMsg->hDelivery
                                       , pUnackdMsg->deliveryId
                                       , ismMESSAGE_STATE_DELIVERED
                                       , pUnackdMsg->hMessage
                                       , true
                                       , true
                                       , &stillLocked);
            TEST_ASSERT_CUNIT(rc == OK, ("rc was %d", rc));
        }

        free(pUnackdMsg);
    }

    return;
}

/****************************************************************************/
/* ProducerThread                                                           */
/****************************************************************************/
static void putComplete(int32_t rc, void *handle, void *pContext)
{
   TEST_ASSERT_CUNIT(rc == OK, ("rc was %d", rc));
   Producer_t *Producer = *(Producer_t **)pContext;

   ASYNCPUT_CB_ADD_AND_FETCH(&(Producer->msgsSent), 1);
}

void PublishMessages(Producer_t *Producer)
{
    int rc = 0;
    uint32_t msgCount=0;
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismEngine_SessionHandle_t hSession = NULL;
    ismEngine_ProducerHandle_t hProducer;
    ismEngine_MessageHandle_t  hMessage;
    ismEngine_ClientStateHandle_t hClient=NULL;
    header.Reliability = Producer->QoS ;
    header.Persistence = (Producer->QoS == 0) ? ismMESSAGE_PERSISTENCE_NONPERSISTENT
                                              : ismMESSAGE_PERSISTENCE_PERSISTENT;

    (void)ism_common_setAffinity(pthread_self(), Producer->cpuArray, Producer->cpuCount);


    rc = ism_engine_createClientState("MsgPublisher",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);

    TEST_ASSERT_EQUAL(rc, OK);

    /************************************************************************/
    /* Create a session for the producer                                    */
    /************************************************************************/
    rc=ism_engine_createSession( hClient
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

    rc=sync_ism_engine_destroyProducer( hProducer );
    TEST_ASSERT_EQUAL(rc, OK);

    rc=sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);

    rc=sync_ism_engine_destroyClientState( hClient
                                         , ismENGINE_DESTROY_CLIENT_OPTION_NONE );
    TEST_ASSERT_EQUAL(rc, OK);

    while((volatile int32_t)Producer->msgsSent < Producer->msgsToSend)
    {
        sched_yield();
    }

    return;
}


void consumerOperationComplete( int32_t retcode
                              , void *handle
                              , void *pcontext )
{
    int urc = 0;
    Consumer_t *pConsumer = *(Consumer_t **)pcontext;

    pConsumer->retCode = retcode;

    urc = pthread_mutex_unlock(&pConsumer->ConsumerMutex);
    TEST_ASSERT_EQUAL(urc, 0);
}

void createSubscription( Consumer_t *pConsumer )
{
    uint32_t rc;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE +
                                                    pConsumer->QoS };

    if (pConsumer->JMSSub)
    {
        subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    }

    if (pConsumer->Durable)
    {
        subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;

        rc=sync_ism_engine_createSubscription( pConsumer->hClient
                                             , pConsumer->SubName
                                             , NULL
                                             , ismDESTINATION_TOPIC
                                             , pConsumer->TopicString
                                             , &subAttrs
                                             , NULL ); // Owning client same as requesting client
        TEST_ASSERT_EQUAL(rc, OK);

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

        verbose(4, "Created durable subscription");
    }
    else 
    {
        rc=ism_engine_createConsumer( pConsumer->hSession
                                    , ismDESTINATION_TOPIC
                                    , pConsumer->TopicString
                                    , &subAttrs
                                    , NULL // Unused for TOPIC
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

        verbose(4, "Created non-durable subscription");
    }

    // And start message delivery
    rc=ism_engine_startMessageDelivery( pConsumer->hSession
                                      , 0
                                      , NULL
                                      , 0
                                      , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_Main                             */
/* Description:     This test creates a subscription to which we     */
/*                  will deliver a number of messages.               */
/*                  When messages are ack'd is controlled by the     */
/*                  whenToAck variable.                              */
/*********************************************************************/
void AckSession_Test_Main( bool JMSSub
                         , bool Durable
                         , WhenToAck_t whenToAck )
{
    uint32_t rc;
    int urc;
    uint32_t numMsgs = 20;
    char TopicString[]="/Topic/Topic1";
    char SubName[]="ISM_Engine_CUnit_AckSession";
    Consumer_t Consumer = { NULL };
    Consumer_t *pConsumer = &Consumer;
    Producer_t Producer = { 0 };

    pConsumer->State = Running;
    Consumer.SubName = SubName;
    Consumer.QoS = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;
    Consumer.TopicString = TopicString;
    Consumer.JMSSub = JMSSub;
    Consumer.whenToAck = whenToAck;
    Consumer.Durable = Durable;
    Consumer.expectedMsgCount = numMsgs;
    urc=pthread_cond_init(&Consumer.MsgsReceivedCond, NULL);
    TEST_ASSERT_EQUAL(urc, 0);
    urc=pthread_mutex_init(&Consumer.ConsumerMutex, NULL);
    TEST_ASSERT_EQUAL(urc, 0);

    //Create client for the consumer
    rc = ism_engine_createClientState("ConsumingClient_TestMain",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &(Consumer.hClient),
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Create a session for the consumer
    rc=ism_engine_createSession( Consumer.hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &Consumer.hSession
                               , NULL
                               , 0 
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    verbose(4, "Created consumer session");

    // And create the subscription
    createSubscription( &Consumer );

    // Publish the messages
    Producer.cpuCount = 0;
    Producer.producerId = 1;
    Producer.TopicString = TopicString;
    Producer.msgsToSend = numMsgs;
    Producer.msgsSent = 0;
    Producer.QoS = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;

    PublishMessages(&Producer);

    verbose(4, "Waiting for MsgsReceivedCond to be posted");
    // Wait for the consumer to receive the expected message count
    urc = pthread_mutex_lock( &Consumer.ConsumerMutex);
    TEST_ASSERT_EQUAL(urc, 0);

    bool end=false;
    do
    {
        switch (whenToAck)
        {
            case OnMsgReceived:
            case OnMsgReceived2:
                if (Consumer.MsgsConsumed == numMsgs)
                {
                    end=true;
                }
                break;
            case Never:
            case AfterAllMsgsReceived:
            case AfterConsumerDestroyed:
            case AfterSubDestroyed:
                if (Consumer.MsgsReceived == numMsgs)
                {
                    end=true;
                }
                break;
        }

        if (!end)
        {
            urc = pthread_cond_wait( &Consumer.MsgsReceivedCond,
                                     &Consumer.ConsumerMutex);
            TEST_ASSERT_EQUAL(urc, 0);
        }
    } while (!end);
 
    verbose(4, "Consumer has received all messages");

    TEST_ASSERT_EQUAL(Consumer.MsgsReceived, numMsgs);
    if ((whenToAck == Never) ||
        (whenToAck == AfterAllMsgsReceived) ||
        (whenToAck == AfterConsumerDestroyed) ||
        (whenToAck == AfterSubDestroyed))
    {
        TEST_ASSERT_EQUAL(Consumer.MsgsConsumed, 0);
    }

    if (whenToAck == AfterAllMsgsReceived)
    {
        FreeAckList( &Consumer, true );

        TEST_ASSERT_EQUAL(Consumer.MsgsConsumed, numMsgs);
    }


    // Destroy the consumer
    // - The destroy subscription could complete asynchronously so we
    //   use the ConsumerMutex which will be unlocked by the callback
    rc=ism_engine_destroyConsumer( Consumer.hConsumer
                                 , &pConsumer
                                 , sizeof(Consumer_t *)
                                 , consumerOperationComplete );
    if (rc == ISMRC_AsyncCompletion)
    {
        // Lock the mutex so we know the destroy consumer has completed
        urc = pthread_mutex_lock( &Consumer.ConsumerMutex);
        TEST_ASSERT_EQUAL(urc, 0);

        rc = pConsumer->retCode;
    }
    TEST_ASSERT_EQUAL(rc, OK);
    verbose(4, "Consumer has been destroyed");

    if (whenToAck == AfterConsumerDestroyed)
    {
        FreeAckList( &Consumer, true );
    }

    if (Durable)
    {
        // For a durable subscription now we must destroy the subscription
        rc = ism_engine_destroySubscription( Consumer.hClient
                                           , SubName
                                           , Consumer.hClient
                                           , &pConsumer
                                           , sizeof(Consumer_t *)
                                           , consumerOperationComplete );
        if (rc == ISMRC_AsyncCompletion)
        {
            // Lock the mutex so we know the destroy consumer has completed
            urc = pthread_mutex_lock( &Consumer.ConsumerMutex);
            TEST_ASSERT_EQUAL(urc, 0);
    
            rc = pConsumer->retCode;
        }
        TEST_ASSERT_EQUAL(rc, OK);
        verbose(4, "Durable subscription has been destroyed");

        if (whenToAck == AfterSubDestroyed)
        {
            FreeAckList( &Consumer, true );
        }
    }

    // Destroy the session
    rc=ism_engine_destroySession( Consumer.hSession
                                , &pConsumer
                                , sizeof(Consumer_t *)
                                , consumerOperationComplete );

    if (rc == ISMRC_AsyncCompletion)
    {
        // Lock the mutex so we know the destroy consumer has completed
        urc = pthread_mutex_lock( &Consumer.ConsumerMutex);
        TEST_ASSERT_EQUAL(urc, 0);

        rc = pConsumer->retCode;
    }
    TEST_ASSERT_EQUAL(rc, OK);
    verbose(4, "Session has been destroyed");

    if (whenToAck == Never)
    {
        FreeAckList( &Consumer, false );
    }

    // Unlock the mutex as we are now done.
    urc = pthread_mutex_unlock( &Consumer.ConsumerMutex);
    TEST_ASSERT_EQUAL(urc, 0);

    rc=sync_ism_engine_destroyClientState( Consumer.hClient
                                         , ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_Redelivery                       */
/* Description:     This test creates a durable subscription to      */
/*                  which we will deliver a number of messages.      */
/*                  When messages are ack'd is controlled by the     */
/*                  whenToAck variable.                              */
/*                  Intermitently, while receiving messages the      */
/*                  receive message operation will stop consuming    */
/*                  messages causing the main program to destroy and */
/*                  then recreate the consumer.                      */
/*********************************************************************/
void AckSession_Test_Redelivery( WhenToAck_t whenToAck )
{
    uint32_t rc;
    int urc;
    uint32_t numMsgs = 2000;
    char TopicString[]="/Topic/Topic2";
    char SubName[]="ISM_Engine_CUnit_Redelivery";
    Consumer_t Consumer = { NULL };
    Consumer_t *pConsumer = &Consumer;
    Producer_t Producer = { 0 };

    pConsumer->State = Running;
    Consumer.SubName = SubName;
    Consumer.QoS = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;
    Consumer.TopicString = TopicString;
    Consumer.JMSSub = false;
    Consumer.whenToAck = whenToAck;
    Consumer.Durable = true;
    Consumer.expectedMsgCount = numMsgs;
    Consumer.stopConsumerRatio = 50;
    Consumer.nackRatio = 12;
    urc=pthread_cond_init(&Consumer.MsgsReceivedCond, NULL);
    TEST_ASSERT_EQUAL(urc, 0);
    urc=pthread_mutex_init(&Consumer.ConsumerMutex, NULL);
    TEST_ASSERT_EQUAL(urc, 0);

    //Create client for the consumer
    rc = ism_engine_createClientState("ConsumingClient_TestRedelivery",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &(Consumer.hClient),
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Create a session for the consumer
    rc=ism_engine_createSession( Consumer.hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &Consumer.hSession
                               , NULL
                               , 0 
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    verbose(4, "Created consumer session");

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    (ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE + pConsumer->QoS) };

    // And create the subscription
    rc=ism_engine_createSubscription( Consumer.hClient
                                    , pConsumer->SubName
                                    , NULL
                                    , ismDESTINATION_TOPIC
                                    , pConsumer->TopicString
                                    , &subAttrs
                                    , NULL // Owning client same as requesting client
                                    , NULL
                                    , 0
                                    , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish the messages
    Producer.cpuCount = 0;
    Producer.producerId = 1;
    Producer.TopicString = TopicString;
    Producer.msgsToSend = numMsgs;
    Producer.msgsSent = 0;
    Producer.QoS = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;

    PublishMessages(&Producer);

    while (Consumer.MsgsConsumed < numMsgs)
    {
        rc=ism_engine_createConsumer( pConsumer->hSession
                                    , ismDESTINATION_SUBSCRIPTION
                                    , pConsumer->SubName
                                    , NULL
                                    + pConsumer->QoS
                                    , NULL // Owning client same as session client
                                    , &pConsumer
                                    , sizeof(pConsumer)
                                    , ConsumerCallback
                                    , NULL
                                    , test_getDefaultConsumerOptions(subAttrs.subOptions + pConsumer->QoS)
                                    , &pConsumer->hConsumer
                                    , NULL
                                    , 0
                                    , NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // And start message delivery
        pConsumer->State = Running;
        rc=ism_engine_startMessageDelivery( pConsumer->hSession
                                          , 0
                                          , NULL
                                          , 0
                                          , NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // Wait for the consumer to receive the expected message count
        urc = pthread_mutex_lock( &Consumer.ConsumerMutex);
       TEST_ASSERT_EQUAL(urc, 0);

        verbose(4, "Number of messages consumed is %d", Consumer.MsgsConsumed);

        if (Consumer.State == Running)
        {
            struct timespec ts;

            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1;

            urc = pthread_cond_timedwait( &Consumer.MsgsReceivedCond,
                                          &Consumer.ConsumerMutex,
                                          &ts);
            TEST_ASSERT(((urc == 0) || (urc == ETIMEDOUT)), ("Unexpected urc %d",urc));

            if (Consumer.State == Running)
            {
                rc = ism_engine_stopMessageDelivery( pConsumer->hSession
                                                   , NULL
                                                   , 0
                                                   , NULL );
                TEST_ASSERT_EQUAL(rc, OK);

                verbose(4, "stopMessageDelivery called. rc = %d", rc);
            }
        }

        // Destroy the consumer
        rc=ism_engine_destroyConsumer( Consumer.hConsumer
                                     , NULL
                                     , 0
                                     , NULL );
        TEST_ASSERT_EQUAL(rc, OK);
        verbose(4, "Consumer has been destroyed");

        // Unlock the mutex as we are now done.
        urc = pthread_mutex_unlock( &Consumer.ConsumerMutex);
        TEST_ASSERT_EQUAL(urc, 0);
    }

    // For a durable subscription now we must destroy the subscription
    rc = ism_engine_destroySubscription( Consumer.hClient
                                       , SubName
                                       , Consumer.hClient
                                       , &pConsumer
                                       , sizeof(Consumer_t *)
                                       , consumerOperationComplete );
    TEST_ASSERT_EQUAL(rc, OK);
    verbose(4, "Durable subscription has been destroyed");

    if (whenToAck == AfterSubDestroyed)
    {
        FreeAckList( &Consumer, true );
    }

    // Destroy the session
    rc=ism_engine_destroySession( Consumer.hSession
                                , &pConsumer
                                , sizeof(Consumer_t *)
                                , consumerOperationComplete );

    TEST_ASSERT_EQUAL(rc, OK);
    verbose(4, "Session has been destroyed");

    if (whenToAck == Never)
    {
        FreeAckList( &Consumer, false );
    }

    // Unlock the mutex as we are now done.
    urc = pthread_mutex_unlock( &Consumer.ConsumerMutex);
    TEST_ASSERT_EQUAL(urc, 0);

    rc=ism_engine_destroyClientState( Consumer.hClient
                                    , ismENGINE_DESTROY_CLIENT_OPTION_NONE
                                    , NULL
                                    , 0
                                    , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/****************************************************************************/
/* DelayedAckCallback                                                       */
/****************************************************************************/
typedef struct _MsgDetails_t
{
    ismEngine_MessageHandle_t  hMsg;
    ismEngine_DeliveryHandle_t hDelivery;
    uint32_t                   deliveryId;
    ismMessageState_t          state;
    struct _MsgDetails_t       *Next;
    struct _MsgDetails_t       *Prev;
} MsgDetails_t;

typedef struct _DAConsumer_t
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    char *SubName;
    char *TopicString;
    uint32_t consumerSeqNum;
    uint32_t InvocationCounter;
    uint32_t MsgsSent;
    uint32_t MsgsReceived;
    uint32_t MsgsConsumed;
    pthread_mutex_t listLock;
    MsgDetails_t *pMsgListHead;
    MsgDetails_t *pMsgListTail;
    uint32_t receiveAcksStarted;
    uint32_t receiveAcksCompleted;
    uint32_t consumeAcksStarted;
    uint32_t consumeAcksCompleted;
} DAConsumer_t;


typedef struct inflightReceivedAck_t {
    DAConsumer_t *pConsumer;
    MsgDetails_t *pCurMsg;
    bool takeLock;
} inflightReceivedAck_t;

static void completedConsumeAck(int rc, void *handle, void *context)
{
    DAConsumer_t *pConsumer = *(DAConsumer_t **)context;

    __sync_fetch_and_add(&(pConsumer->consumeAcksCompleted), 1);
}
static void completedReceiveAck(int rc, void *handle, void *context)
{
    inflightReceivedAck_t *ackInfo = (inflightReceivedAck_t *)context;
    ackInfo->pCurMsg->state = ismMESSAGE_STATE_RECEIVED;
    DAConsumer_t *pConsumer =ackInfo->pConsumer;

    uint64_t completedAcks = __sync_add_and_fetch(&(pConsumer->receiveAcksCompleted), 1);

    // If all messages have arrived at the callback and there are no acks in flight then let's ack all messages
    if (pConsumer->MsgsReceived  == pConsumer->MsgsSent && completedAcks == pConsumer->receiveAcksStarted)
    {
        if (ackInfo->takeLock)
        {
            int osrc = pthread_mutex_lock(&(pConsumer->listLock));
            TEST_ASSERT_EQUAL(osrc , 0);
        }
        verbose(4, "Received Message all messages sent (%d), consuming all messages", pConsumer->MsgsSent);

        while (pConsumer->pMsgListHead != NULL)
        {
            MsgDetails_t *pCurMsg = pConsumer->pMsgListHead;
            pConsumer->pMsgListHead = pCurMsg->Next;
            pConsumer->MsgsConsumed++;

            TEST_ASSERT_CUNIT(pCurMsg->state == ismMESSAGE_STATE_RECEIVED || pCurMsg->state == ismMESSAGE_STATE_DELIVERED,
                              ("msg state was %d\n", pCurMsg->state));
            __sync_fetch_and_add(&(pConsumer->consumeAcksStarted), 1);

            rc = ism_engine_confirmMessageDelivery( pConsumer->hSession
                                                  , NULL
                                                  , pCurMsg->hDelivery
                                                  , ismENGINE_CONFIRM_OPTION_CONSUMED
                                                  , &pConsumer, sizeof(pConsumer)
                                                  , completedConsumeAck);

            TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                              ("rc was %d\n", rc));

            if (rc == OK)
            {
                completedConsumeAck(rc, NULL, &pConsumer);
            }

            free(pCurMsg);
        }
        pConsumer->pMsgListHead = NULL;
        pConsumer->pMsgListTail = NULL;

        if (ackInfo->takeLock)
        {
            int osrc = pthread_mutex_unlock(&(pConsumer->listLock));
            TEST_ASSERT_EQUAL(osrc , 0);
        }
        rc = ism_engine_stopMessageDelivery( pConsumer->hSession
                                           , NULL
                                           , 0
                                           , NULL );
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

bool DelayedAckCallback( ismEngine_ConsumerHandle_t hConsumer
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
    int rc;
    DAConsumer_t *pConsumer = *(DAConsumer_t **)pConsumerContext;
    MsgDetails_t *pCurMsg = NULL;
    MsgDetails_t *pNextMsg = NULL;

    pConsumer->InvocationCounter++;

    if (state == ismMESSAGE_STATE_DELIVERED)
    {
        pConsumer->MsgsReceived++;
    }

    // If this is the first time a message has been delivered, we
    // expect the delivery id to be initalised to zero and we must
    // set an appropriate delivery Id.
    if (pMsgDetails->RedeliveryCount == 0)
    {
        verbose(5, "Received Message(%d) state(%d) - %s", deliveryId, state, (char *)pAreaData[0]);
    }
    else
    {
        verbose(4, "Received redelivered(%d) Message(%d) - %s",
                pMsgDetails->RedeliveryCount, deliveryId, (char *)pAreaData[0]);
    }

    int osrc = pthread_mutex_lock(&pConsumer->listLock);
    TEST_ASSERT_EQUAL(osrc, 0);

    // See if this consumer has already been delivered this message
    pCurMsg = pConsumer->pMsgListHead;
    while ((pCurMsg != NULL) && (pCurMsg->hMsg != hMessage))
    {
        pCurMsg=pCurMsg->Next;
    }

    if (pCurMsg == NULL)
    {
        // First time this consumer has seen this msg
        pCurMsg=(MsgDetails_t *)malloc(sizeof(MsgDetails_t));

        pCurMsg->hMsg = hMessage;
        pCurMsg->hDelivery = hDelivery;
        pCurMsg->deliveryId = deliveryId;
        pCurMsg->state = state;
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
           pCurMsg->Prev=pConsumer->pMsgListTail;
           pConsumer->pMsgListTail=pCurMsg;
        }
    }
    else
    {
        // We have see this message before!

        // Check the message state and deliver Id match
        TEST_ASSERT_EQUAL(pCurMsg->state, state);
        TEST_ASSERT_EQUAL(pCurMsg->deliveryId, deliveryId);
    }

    // Consider whether we should ack messages now
    if ((pConsumer->InvocationCounter % 5) == 0)
    {
        // Nack the oldest message (which has not been received)
        for (pCurMsg=pConsumer->pMsgListHead; (pCurMsg != NULL) && (pCurMsg->state != ismMESSAGE_STATE_DELIVERED); pCurMsg=pCurMsg->Next)
            ;

        if (pCurMsg)
        {
            if (pCurMsg->Prev == NULL)
                pConsumer->pMsgListHead=pCurMsg->Next;
            else
                pCurMsg->Prev->Next=pCurMsg->Next;
            if (pCurMsg->Next == NULL)
                pConsumer->pMsgListTail=pCurMsg->Prev;
            else
                pCurMsg->Next->Prev=pCurMsg->Prev;
            pCurMsg->Prev = NULL;
            pCurMsg->Next = NULL;

            rc = ism_engine_confirmMessageDelivery( pConsumer->hSession
                                                  , NULL
                                                  , pCurMsg->hDelivery
                                                  , ismENGINE_CONFIRM_OPTION_NOT_RECEIVED
                                                  , NULL
                                                  , 0
                                                  , NULL );
            TEST_ASSERT_EQUAL(rc, OK);

            pConsumer->MsgsReceived--;

            free(pCurMsg);
        }
    }
    else if ((pConsumer->InvocationCounter % 13) == 0)
    {
        // 'Receive' the oldest 10 messages, 
        uint32_t msgRecvd=0;
        for (pCurMsg=pConsumer->pMsgListHead; (pCurMsg != NULL) && (msgRecvd < 10); pCurMsg=pCurMsg->Next) /* BEAM suppression: loop doesn't iterate */
        {
            if (pCurMsg->state == ismMESSAGE_STATE_DELIVERED)
            {
                pCurMsg->state = ismMESSAGE_STATE_RESERVED1; //Record the message is in an in between state so we leave it alone
                inflightReceivedAck_t ackInfo = {pConsumer, pCurMsg, true};

                __sync_fetch_and_add(&(pConsumer->receiveAcksStarted), 1);
                rc = ism_engine_confirmMessageDelivery( pConsumer->hSession
                                                      , NULL
                                                      , pCurMsg->hDelivery
                                                      , ismENGINE_CONFIRM_OPTION_RECEIVED
                                                      , &ackInfo, sizeof(ackInfo), completedReceiveAck);

                TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                                  ("rc was %d\n", rc));

                if (rc == OK)
                {
                    ackInfo.takeLock = false;
                    completedReceiveAck(rc, NULL, &ackInfo);
                }
                msgRecvd++;
            }
        }
    }
    else if ((pConsumer->InvocationCounter % 47) == 0)
    {
        // 'Consume' all of the received messages
        for (pNextMsg=pConsumer->pMsgListHead; (pNextMsg != NULL); )
        {
            if (pNextMsg->state == ismMESSAGE_STATE_RECEIVED)
            {
                pCurMsg=pNextMsg;
                pNextMsg=pNextMsg->Next;
                
                __sync_fetch_and_add(&(pConsumer->consumeAcksStarted), 1);

                rc = ism_engine_confirmMessageDelivery( pConsumer->hSession
                                                           , NULL
                                                           , pCurMsg->hDelivery
                                                           , ismENGINE_CONFIRM_OPTION_CONSUMED
                                                           , &pConsumer, sizeof(pConsumer)
                                                           , completedConsumeAck);
                TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                        ("rc was %d\n", rc));

                if (rc == OK)
                {
                    completedConsumeAck(rc, NULL, &pConsumer);
                }

                pCurMsg->state = ismMESSAGE_STATE_CONSUMED;
                pConsumer->MsgsConsumed++;

                if (pCurMsg->Prev == NULL)
                    pConsumer->pMsgListHead=pCurMsg->Next;
                else
                    pCurMsg->Prev->Next=pCurMsg->Next;
                if (pCurMsg->Next == NULL)
                    pConsumer->pMsgListTail=pCurMsg->Prev;
                else
                    pCurMsg->Next->Prev=pCurMsg->Prev;
                pCurMsg->Prev = NULL;
                pCurMsg->Next = NULL;

                free(pCurMsg);
            }
            else
            {
                pNextMsg=pNextMsg->Next;
            }
        }
    }
    else if ((pConsumer->InvocationCounter % 79) == 0)
    {
        // Stop consuming messages

        rc = ism_engine_stopMessageDelivery( pConsumer->hSession
                                           , NULL
                                           , 0
                                           , NULL );
        TEST_ASSERT_EQUAL(rc, OK);

    }

    pthread_mutex_unlock(&(pConsumer->listLock));
   
    return true;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_DelayedAck                       */
/* Description:     This test creates a durable subscription to      */
/*                  which we will deliver a number of messages.      */
/*                  The consumer is then started, as each message is */
/*                  received, it is added to the list of received    */
/*                  messages. When the list of messages received     */
/*                  reaches the defined threshold it will ack some   */
/*                  of the messages on the list and nack 1 message.  */
/*                  Meanwhile, the main thread simulates the client  */
/*                  intermitently disconnecting and reconnecting.    */
/*********************************************************************/
void AckSession_Test_DelayedAck( void )
{
    uint32_t rc;
    uint32_t numMsgs = 2000;
    char TopicString[]="/Topic/Topic2";
    char SubName[]="ISM_Engine_CUnit_DelayedAck";
    DAConsumer_t Consumer = { NULL };
    DAConsumer_t *pConsumer = &Consumer;
    Producer_t Producer = { 0 };

    Consumer.SubName = SubName;
    Consumer.TopicString = TopicString;
    Consumer.MsgsSent = numMsgs;
    pthread_mutex_init(&(Consumer.listLock), NULL);

    //Create client for the consumer
    rc = ism_engine_createClientState("ConsumingClient_DelayedAck",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &(Consumer.hClient),
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Create a session for the consumer
    rc=ism_engine_createSession( Consumer.hClient
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

    rc=sync_ism_engine_createSubscription( pConsumer->hClient
                                         , pConsumer->SubName
                                         , NULL
                                         , ismDESTINATION_TOPIC
                                         , pConsumer->TopicString
                                         , &subAttrs
                                         , NULL ); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish the messages
    Producer.cpuCount = 0;
    Producer.producerId = 1;
    Producer.TopicString = TopicString;
    Producer.msgsToSend = numMsgs;
    Producer.msgsSent = 0;
    Producer.QoS = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;

    PublishMessages(&Producer);

    // Now strart publishing messages
    while (Consumer.MsgsConsumed < numMsgs)
    {
        verbose(4, "Creating consumer\n");
        rc=ism_engine_createConsumer( pConsumer->hSession
                                    , ismDESTINATION_SUBSCRIPTION
                                    , pConsumer->SubName
                                    , NULL
                                    , NULL // Owning client same as session client
                                    , &pConsumer
                                    , sizeof(pConsumer)
                                    , DelayedAckCallback
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

        usleep(20 * 1000); //0.02s

        verbose(4, "Number of messages received(%d) consumed(%d)",
                Consumer.MsgsReceived, Consumer.MsgsConsumed);

        // Destroy the consumer
        rc=sync_ism_engine_destroyConsumer( Consumer.hConsumer);
        TEST_ASSERT_EQUAL(rc, OK);
        verbose(4, "Consumer has been destroyed");

        //we need to destroy the session to wait for acks that are inflight
        //to complete... otherwise on intermediateQ, if we reconnected we'd
        //be redriven for messages that were in the process of being consumed
        rc=sync_ism_engine_destroySession( Consumer.hSession );
        TEST_ASSERT_EQUAL(rc, OK);

        //Now we've destroyed the consumer and session (and are about to recreate them)
        //We can't refer to messages from previous consumer existences...
        //We're about to be given them again
        while (Consumer.pMsgListHead != NULL)
        {
            MsgDetails_t *toDelete = Consumer.pMsgListHead;
            Consumer.pMsgListHead =  Consumer.pMsgListHead->Next;
            free(toDelete);
        }
        Consumer.pMsgListTail = NULL;

        rc=ism_engine_createSession( Consumer.hClient
                                   , ismENGINE_CREATE_SESSION_OPTION_NONE
                                   , &Consumer.hSession
                                   , NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // For a durable subscription now we must destroy the subscription
    rc = ism_engine_destroySubscription( Consumer.hClient
                                       , SubName
                                       , Consumer.hClient
                                       , &pConsumer
                                       , sizeof(Consumer_t *)
                                       , consumerOperationComplete );
    TEST_ASSERT_EQUAL(rc, OK);
    verbose(4, "Durable subscription has been destroyed");

    // Destroy the session
    rc=ism_engine_destroySession( Consumer.hSession
                                , &pConsumer
                                , sizeof(Consumer_t *)
                                , consumerOperationComplete );

    TEST_ASSERT_EQUAL(rc, OK);
    verbose(4, "Session has been destroyed");

    rc=ism_engine_destroyClientState( Consumer.hClient
                                    , ismENGINE_DESTROY_CLIENT_OPTION_NONE
                                    , NULL
                                    , 0
                                    , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_TT_ND_OnMsgReceived              */
/* Description:     This test creates a non durable subscription     */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd as they are received.         */
/*********************************************************************/
void AckSession_Test_TT_ND_OnMsgReceived( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_TT_ND_OnMsgReceived...");

    AckSession_Test_Main(false, false, OnMsgReceived);

    verbose(4, "Completed AckSession_Test_TT_ND_OnMsgReceived");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_TT_ND_AfterAllMsgsReceived       */
/* Description:     This test creates a non durable subscription     */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd after they have all been      */
/*                  received.                                        */
/*********************************************************************/
void AckSession_Test_TT_ND_AfterAllMsgsReceived( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_TT_ND_AfterAllMsgsReceived...");

    AckSession_Test_Main(false, false, AfterAllMsgsReceived);

    verbose(4, "Completed AckSession_Test_TT_ND_AfterAllMsgsReceived");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_TT_ND_AfterConsumerDestroyed     */
/* Description:     This test creates a non durable subscription     */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd after we have destroyed the   */
/*                  consumer.                                        */
/*********************************************************************/
void AckSession_Test_TT_ND_AfterConsumerDestroyed( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_TT_ND_AfterConsumerDestroyed...");

    AckSession_Test_Main(false, false, AfterConsumerDestroyed);

    verbose(4, "Completed AckSession_Test_TT_ND_AfterConsumerDestroyed");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_TT_ND_Never                      */
/* Description:     This test creates a non durable subscription     */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd in a batch after they have    */
/*                  all been received.                               */
/*********************************************************************/
void AckSession_Test_TT_ND_Never( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_TT_ND_Never...");

    AckSession_Test_Main(false, false, Never);

    verbose(4, "Completed AckSession_Test_TT_ND_Never");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_TT_D_OnMsgReceived               */
/* Description:     This test creates a durable subscription         */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd as they are received.         */
/*********************************************************************/
void AckSession_Test_TT_D_OnMsgReceived( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_TT_D_OnMsgReceived...");

    AckSession_Test_Main(false, true, OnMsgReceived);

    verbose(4, "Completed AckSession_Test_TT_D_OnMsgReceived");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_TT_D_AfterAllMsgsReceived        */
/* Description:     This test creates a durable subscription         */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd after they have all been      */
/*                  received.                                        */
/*********************************************************************/
void AckSession_Test_TT_D_AfterAllMsgsReceived( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_TT_D_AfterAllMsgsReceived...");

    AckSession_Test_Main(false, true, AfterAllMsgsReceived);

    verbose(4, "Completed AckSession_Test_TT_D_AfterAllMsgsReceived");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_TT_D_AfterConsumerDestroyed      */
/* Description:     This test creates a durable subscription         */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd after we have destroyed the   */
/*                  consumer.                                        */
/*********************************************************************/
void AckSession_Test_TT_D_AfterConsumerDestroyed( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_TT_D_AfterConsumerDestroyed...");

    AckSession_Test_Main(false, true, AfterConsumerDestroyed);

    verbose(4, "Completed AckSession_Test_TT_D_AfterConsumerDestroyed");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_TT_D_AfterSubDestroyed           */
/* Description:     This test creates a durable subscription         */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd after we have destroyed the   */
/*                  durable subscription.                            */
/*********************************************************************/
void AckSession_Test_TT_D_AfterSubDestroyed( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_TT_D_AfterSubDestroyed...");

    AckSession_Test_Main(false, true, AfterConsumerDestroyed);

    verbose(4, "Completed AckSession_Test_TT_ND_AfterSubDestroyed");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_TT_D_Never                       */
/* Description:     This test creates a durable subscription         */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd in a batch after they have    */
/*                  all been received.                               */
/*********************************************************************/
void AckSession_Test_TT_D_Never( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_TT_D_Never...");

    AckSession_Test_Main(false, true, Never);

    verbose(4, "Completed AckSession_Test_TT_D_Never");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_TT_D_Redelivery_OnMsgReceived    */
/* Description:     This test creates a durable subscription         */
/*                  to which we publish a 2000 messages.             */
/*                  We then create a consumer which will receive the */
/*                  messages. As each message is received it is      */
/*                  ack'd or nack'd. Intermitently we destroy and    */
/*                  recreate the consumer causing message redelivery */
/*                  to occur.                                        */
/*********************************************************************/
void AckSession_Test_TT_D_Redelivery_OnMsgReceived( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_TT_D_Redelivery_OnMsgReceived...");

    AckSession_Test_Redelivery(OnMsgReceived2);

    verbose(4, "Completed AckSession_Test_TT_D_Redelivery_OnMsgReceived");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_TT_D_Redelivery_OnBatch          */
/* Description:     This test creates a durable subscription         */
/*                  to which we publish a 2000 messages.             */
/*                  We then create a consumer which will receive the */
/*                  messages. Messages are ack'd in batches with     */
/*                  the ratio of nack's being around 1 in 10.        */
/*********************************************************************/
void AckSession_Test_TT_D_Redelivery_DelayedAck( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_TT_D_Redelivery_DelayedAck...");

    AckSession_Test_DelayedAck();

    verbose(4, "Completed AckSession_Test_TT_D_Redelivery_DelayedAck");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_JMS_ND_OnMsgReceived             */
/* Description:     This test creates a non durable subscription     */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd as they are received.         */
/*********************************************************************/
void AckSession_Test_JMS_ND_OnMsgReceived( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_JMS_ND_OnMsgReceived...");

    AckSession_Test_Main(false, false, OnMsgReceived);

    verbose(4, "Completed AckSession_Test_JMS_ND_OnMsgReceived");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_JMS_ND_AfterAllMsgsReceived      */
/* Description:     This test creates a non durable subscription     */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd after they have all been      */
/*                  received.                                        */
/*********************************************************************/
void AckSession_Test_JMS_ND_AfterAllMsgsReceived( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_JMS_ND_AfterAllMsgsReceived...");

    AckSession_Test_Main(false, false, AfterAllMsgsReceived);

    verbose(4, "Completed AckSession_Test_JMS_ND_AfterAllMsgsReceived");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_JMS_ND_AfterConsumerDestroyed    */
/* Description:     This test creates a non durable subscription     */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd after we have destroyed the   */
/*                  consumer.                                        */
/*********************************************************************/
void AckSession_Test_JMS_ND_AfterConsumerDestroyed( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_JMS_ND_AfterConsumerDestroyed...");

    AckSession_Test_Main(false, false, AfterConsumerDestroyed);

    verbose(4, "Completed AckSession_Test_JMS_ND_AfterConsumerDestroyed");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_JMS_ND_Never                     */
/* Description:     This test creates a non durable subscription     */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd in a batch after they have    */
/*                  all been received.                               */
/*********************************************************************/
void AckSession_Test_JMS_ND_Never( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_JMS_ND_Never...");

    AckSession_Test_Main(false, false, Never);

    verbose(4, "Completed AckSession_Test_JMS_ND_Never");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_JMS_D_OnMsgReceived              */
/* Description:     This test creates a non durable subscription     */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd as they are received.         */
/*********************************************************************/
void AckSession_Test_JMS_D_OnMsgReceived( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_JMS_D_OnMsgReceived...");

    AckSession_Test_Main(false, true, OnMsgReceived);

    verbose(4, "Completed AckSession_Test_JMS_D_OnMsgReceived");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_JMS_D_AfterAllMsgsReceived       */
/* Description:     This test creates a non durable subscription     */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd after they have all been      */
/*                  received.                                        */
/*********************************************************************/
void AckSession_Test_JMS_D_AfterAllMsgsReceived( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_JMS_D_AfterAllMsgsReceived...");

    AckSession_Test_Main(false, true, AfterAllMsgsReceived);

    verbose(4, "Completed AckSession_Test_JMS_D_AfterAllMsgsReceived");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_JMS_D_AfterConsumerDestroyed     */
/* Description:     This test creates a non durable subscription     */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd after we have destroyed the   */
/*                  consumer.                                        */
/*********************************************************************/
void AckSession_Test_JMS_D_AfterConsumerDestroyed( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_JMS_D_AfterConsumerDestroyed...");

    AckSession_Test_Main(false, true, AfterConsumerDestroyed);

    verbose(4, "Completed AckSession_Test_JMS_D_AfterConsumerDestroyed");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_JMS_D_AfterSubDestroyed          */
/* Description:     This test creates a non durable subscription     */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd after we have destroyed the   */
/*                  consumer.                                        */
/*********************************************************************/
void AckSession_Test_JMS_D_AfterSubDestroyed( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_JMS_D_AfterSubDestroyed...");

    AckSession_Test_Main(false, true, AfterSubDestroyed);

    verbose(4, "Completed AckSession_Test_JMS_D_AfterSubDestroyed");
    return;
}

/*********************************************************************/
/* TestCase:        AckSession_Test_JMS_D_Never                      */
/* Description:     This test creates a non durable subscription     */
/*                  to which we will deliver a number of messages.   */
/*                  Messages are ack'd in a batch after they have    */
/*                  all been received.                               */
/*********************************************************************/
void AckSession_Test_JMS_D_Never( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting AckSession_Test_JMS_D_Never...");

    AckSession_Test_Main(false, true, Never);

    verbose(4, "Completed AckSession_Test_JMS_D_Never");
    return;
}

CU_TestInfo AckSession_Test_TT[] = {
    { "AckSession_Test_TT_ND_OnMsgReceived",          AckSession_Test_TT_ND_OnMsgReceived },
    { "AckSession_Test_TT_ND_AfterAllMsgsReceived",   AckSession_Test_TT_ND_AfterAllMsgsReceived },
//  { "AckSession_Test_TT_ND_AfterConsumerDestroyed", AckSession_Test_TT_ND_AfterConsumerDestroyed },
    { "AckSession_Test_TT_ND_Never",                  AckSession_Test_TT_ND_Never },
    { "AckSession_Test_TT_D_OnMsgReceived",           AckSession_Test_TT_D_OnMsgReceived },
    { "AckSession_Test_TT_D_AfterAllMsgsReceived",    AckSession_Test_TT_D_AfterAllMsgsReceived },
//  { "AckSession_Test_TT_D_AfterConsumerDestroyed",  AckSession_Test_TT_D_AfterConsumerDestroyed },
//  { "AckSession_Test_TT_D_AfterSubDestroyed",       AckSession_Test_TT_D_AfterSubestroyed },
    { "AckSession_Test_TT_D_Never",                   AckSession_Test_TT_D_Never },
//  { "AckSession_Test_TT_D_Redelivery_OnMsgReceived", AckSession_Test_TT_D_Redelivery_OnMsgReceived, },
    { "AckSession_Test_TT_D_Redelivery_DelayedAck",   AckSession_Test_TT_D_Redelivery_DelayedAck, },
    CU_TEST_INFO_NULL
};

CU_TestInfo AckSession_Test_JMS[] = {
    { "AckSession_Test_JMS_ND_OnMsgReceived",          AckSession_Test_JMS_ND_OnMsgReceived },
    { "AckSession_Test_JMS_ND_AfterAllMsgsReceived",   AckSession_Test_JMS_ND_AfterAllMsgsReceived },
//  { "AckSession_Test_JMS_ND_AfterConsumerDestroyed", AckSession_Test_JMS_ND_AfterConsumerDestroyed },
    { "AckSession_Test_JMS_ND_Never",                  AckSession_Test_JMS_ND_Never },
    { "AckSession_Test_JMS_D_OnMsgReceived",           AckSession_Test_JMS_D_OnMsgReceived },
    { "AckSession_Test_JMS_D_AfterAllMsgsReceived",    AckSession_Test_JMS_D_AfterAllMsgsReceived },
//  { "AckSession_Test_JMS_D_AfterConsumerDestroyed",  AckSession_Test_JMS_D_AfterConsumerDestroyed },
//  { "AckSession_Test_JMS_D_AfterSubDestroyed",       AckSession_Test_JMS_D_AfterSubDestroyed },
    { "AckSession_Test_JMS_D_Never",                   AckSession_Test_JMS_D_Never },
    CU_TEST_INFO_NULL
};

int initSession(void)
{
    int32_t rc;
    ism_field_t f;

    ism_prop_t *staticConfigProps = ism_common_getConfigProperties();

    //Allow MQTT clients to have 100 unacked msgs for some of the acking/nacking tests
    f.type = VT_Integer;
    f.val.i = 1000;
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
CU_SuiteInfo ISM_Engine_CUnit_AckSession_Suites[] = {
    IMA_TEST_SUITE("AckSession_Test_TT", initSession, termSession, AckSession_Test_TT),
    IMA_TEST_SUITE("AckSession_Test_JMS", initSession, termSession, AckSession_Test_JMS),
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
    CU_register_suites(ISM_Engine_CUnit_AckSession_Suites);

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
