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
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/prctl.h>

#include "engineInternal.h"
#include "queueCommon.h"
#include "transaction.h"
#include "ackList.h" // access to ieal_debugAckList
#include "queueNamespace.h"

#include "multiConsumerQInternal.h"

#include "test_utils_initterm.h"
#include "test_utils_log.h"
#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_client.h"
#include "test_utils_options.h"
#include "test_utils_file.h"
#include "test_utils_preload.h"
#include "test_utils_sync.h"
#include "test_utils_task.h"

#if !defined(MIN)
#define MIN(a,b) (((a)<=(b))?(a):(b))
#endif
#if !defined(MAX)
#define MAX(a,b) (((a)>=(b))?(a):(b))
#endif

void ism_engine_getSelectionStats( ismEngine_ConsumerHandle_t hConsumer
                                 , uint64_t *pfailedSelectionCount
                                 , uint64_t *psuccesfullSelectionCount );

int32_t selectionWrapper( ismMessageHeader_t * pMsgDetails
                        , uint8_t              areaCount
                        , ismMessageAreaType_t areaTypes[areaCount]
                        , size_t               areaLengths[areaCount]
                        , void *               pareaData[areaCount]
                        , const char *         topic
                        , const void *         pselectorRule
                        , size_t               selectorRuleLen 
                        , ismMessageSelectionLockStrategy_t * lockStrategy);

ismEngine_MessageSelectionCallback_t OriginalSelection = NULL;


static ismEngine_ClientStateHandle_t hMCQTestClient = NULL;
static pthread_key_t putterContextKey;

static volatile uint32_t acksStarted = 0;
static volatile uint32_t acksCompleted = 0;

int initMultiConsumerQ(void)
{
    int32_t rc;

    rc = sync_ism_engine_createClientState("Consumer Test",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &hMCQTestClient);

    if (rc != OK)
    {
        printf("ERROR:  ism_engine_createClientState() returned %d", rc);
    }

    /************************************************************************/
    /* Set the default policy info to allow deep queues                     */
    /************************************************************************/
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 100000000;

    /************************************************************************/
    /* Set up our wrapper to the selection function                         */
    /************************************************************************/
    rc=pthread_key_create(&putterContextKey, NULL);

    OriginalSelection = ismEngine_serverGlobal.selectionFn;
    ismEngine_serverGlobal.selectionFn = selectionWrapper;

    return rc;
}

int termMultiConsumerQ(void)
{
    return 0;
}
static void finishAck(int rc, void *handle, void *context)
{
    __sync_fetch_and_add(&acksCompleted, 1);
}
static void commitTranAck(int rc, void *handle, void *context)
{
    ismEngine_Transaction_t *cmdTran = *(ismEngine_Transaction_t **)context;
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    //Commit the consuming of the message
    ietr_commit(pThreadData, cmdTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);

    finishAck(rc, NULL, NULL);
}

static bool commitOutOfOrderCB(ismEngine_ConsumerHandle_t  hConsumer,
                               ismEngine_DeliveryHandle_t  hDelivery,
                               ismEngine_MessageHandle_t   hMessage,
                               uint32_t                    deliveryId,
                               ismMessageState_t           state,
                               uint32_t                    destinationOptions,
                               ismMessageHeader_t *        pMsgDetails,
                               uint8_t                     areaCount,
                               ismMessageAreaType_t        areaTypes[areaCount],
                               size_t                      areaLengths[areaCount],
                               void *                      pAreaData[areaCount],
                               void *                      pConsumerContext,
                               ismEngine_DelivererContext_t * delivererContext )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t *pMessagesDelivered = *(uint32_t **)pConsumerContext;


    //Was it the one we expected
    uint32_t msgArrivedNum = *(uint32_t *)pAreaData[0];
    test_log(3, "Message %u arrived",msgArrivedNum);
    TEST_ASSERT_EQUAL(msgArrivedNum, 1 + *pMessagesDelivered); //Was it the one after the last one?

    //Free our copy of the message
    ism_engine_releaseMessage(hMessage);

    __sync_add_and_fetch(&acksStarted, 1);

    //Confirm we received the message... alternating between transaction and non-transactional CMD...
    if (msgArrivedNum % 2 == 0)
    {
        //Let's do it transactionally for giggles
        ismEngine_Transaction_t *cmdTran;
        int32_t rc = ietr_createLocal(pThreadData, NULL, true, false, NULL, &cmdTran);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_confirmMessageDelivery( ((ismEngine_Consumer_t *)hConsumer)->pSession
                                                   , cmdTran
                                                   , hDelivery
                                                   , ismENGINE_CONFIRM_OPTION_CONSUMED
                                                   , &cmdTran, sizeof(cmdTran)
                                                   , commitTranAck);
        TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                          ("rc was %d\n", rc));

        if (rc == OK)
        {
            commitTranAck(rc, NULL, &cmdTran);
        }
    }
    else
    {
        //transactions are for wimps
        int32_t rc = ism_engine_confirmMessageDelivery( ((ismEngine_Consumer_t *)hConsumer)->pSession
                                                           , NULL
                                                           , hDelivery
                                                           , ismENGINE_CONFIRM_OPTION_CONSUMED
                                                           , NULL, 0
                                                           , finishAck);
        TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                          ("rc was %d\n", rc));

        if (rc == OK)
        {
            finishAck(rc, NULL, NULL);
        }
    }

    (*pMessagesDelivered)++; //Finally, record this message arrived...


    return true; //We always want another message
}

/*********************************************************************/
/* Name:        CommitOutOfOrder                                     */
/* Description:                                                      */
/* Check what happens when a message that is put after another       */
/* message is committed before it                                    */
/*********************************************************************/
static void CommitOutOfOrder( void )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismQHandle_t hQueue;
    ismEngine_QueueStatistics_t stats;
    ismEngine_Consumer_t *pConsumer;
    char queueName[32];
    volatile uint32_t messagesDelivered = 0;
    volatile uint32_t *pMessagesDelivered = &messagesDelivered;

    // Create Session for this test
    rc=ism_engine_createSession( hMCQTestClient
                               , ismENGINE_CREATE_SESSION_TRANSACTIONAL
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Create the queue
    sprintf(queueName, __func__);
    ieqnQueue_t *namedQueue;
    rc = ieqn_createQueue( pThreadData
                         , queueName
                         , multiConsumer
                         , ismQueueScope_Server
                         , NULL
                         , NULL
                         , NULL
                         , &namedQueue );
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(namedQueue);

    test_log(4, "Created queue");

    hQueue = ieqn_getQueueHandle(pThreadData, queueName);
    TEST_ASSERT_PTR_NOT_NULL(hQueue);
    TEST_ASSERT_EQUAL(hQueue, namedQueue->queueHandle);

    test_log(4, "Create consumer");
    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , queueName
                                  , 0
                                  , NULL // Unused for QUEUE
                                  , &pMessagesDelivered
                                  , sizeof(pMessagesDelivered)
                                  , commitOutOfOrderCB
                                  , NULL
                                  , ismENGINE_CONSUMER_OPTION_ACK
                                  , &pConsumer
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    //Start Message Delivery on the consumer
    rc = ism_engine_startMessageDelivery( hSession,
                                          ismENGINE_START_DELIVERY_OPTION_NONE,
                                          NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Create a transaction and put message "2" under it (2 because it'll be committed after 1)
    ismEngine_Transaction_t *secondCommitTran;
    rc = ietr_createLocal(pThreadData, NULL, true, false, NULL, &secondCommitTran);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_MessageHandle_t hMessageMsg2 = NULL;
    ismMessageHeader_t headerMsg2 = ismMESSAGE_HEADER_DEFAULT;
    headerMsg2.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
    headerMsg2.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    ismMessageAreaType_t areaTypesMsg2[1] = {ismMESSAGE_AREA_PAYLOAD};
    size_t areaLengthsMsg2[1] = {sizeof(uint32_t)};
    uint32_t msgNum2 = 2;
    void *areaDataMsg2[1] = {&msgNum2};

    rc = ism_engine_createMessage(
            &headerMsg2,
            1,
            areaTypesMsg2,
            areaLengthsMsg2,
            areaDataMsg2,
            &hMessageMsg2);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieq_put( pThreadData
                , hQueue
                , ieqPutOptions_THREAD_LOCAL_MESSAGE
                , secondCommitTran
                , hMessageMsg2
                , IEQ_MSGTYPE_REFCOUNT 
                , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    ism_engine_releaseMessage(hMessageMsg2);

    //Create another transaction and put message 1 under it.
    ismEngine_Transaction_t *firstCommitTran;
    rc = ietr_createLocal(pThreadData, NULL, true, false, NULL, &firstCommitTran);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_MessageHandle_t hMessageMsg1 = NULL;
    ismMessageHeader_t headerMsg1 = ismMESSAGE_HEADER_DEFAULT;
    headerMsg1.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
    headerMsg1.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    ismMessageAreaType_t areaTypesMsg1[1] = {ismMESSAGE_AREA_PAYLOAD};
    size_t areaLengthsMsg1[1] = {sizeof(uint32_t)};
    uint32_t msgNum1 = 1;
    void *areaDataMsg1[1] = {&msgNum1};

    rc = ism_engine_createMessage(
            &headerMsg1,
            1,
            areaTypesMsg1,
            areaLengthsMsg1,
            areaDataMsg1,
            &hMessageMsg1);

    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieq_put( pThreadData
                , hQueue
                , ieqPutOptions_NONE
                , firstCommitTran
                , hMessageMsg1
                , IEQ_MSGTYPE_REFCOUNT 
                , NULL );
    TEST_ASSERT_EQUAL(rc, OK);
    ism_engine_releaseMessage(hMessageMsg1);

    //Check no messages have been delivered
    TEST_ASSERT_EQUAL(messagesDelivered, 0);

    //Commit the put of message 1
    ietr_commit(pThreadData, firstCommitTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);

    //Check message 1 arrives
    test_waitForMessages(&messagesDelivered, NULL, 1, 20);
    TEST_ASSERT_EQUAL(messagesDelivered, 1);

    // Log the contents of the queue
    test_log_queueHandle(testLOGLEVEL_VERBOSE, hQueue, 9, -1, "");

    //Commit the put of message 2
    ietr_commit(pThreadData, secondCommitTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);

    // Log the contents of the queue again
    test_log_queueHandle(testLOGLEVEL_VERBOSE, hQueue, 9, -1, "");

    //Check message 2 arrives
    test_waitForMessages(&messagesDelivered, NULL, 2, 20);
    TEST_ASSERT_EQUAL(messagesDelivered, 2);

    test_waitForMessages(&acksCompleted, NULL, acksStarted, 20);

    //Check that the queue is now empty
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);

    //Delete the consumer, restart delivery
    rc = ism_engine_destroyConsumer( pConsumer, NULL, 0 , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Cleaup session
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);

    // Delete Q
    rc = ieqn_destroyQueue(pThreadData, queueName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    test_log(4, "Deleted queue");

    return;
}

static bool NackOnSessionCloseCB(ismEngine_ConsumerHandle_t  hConsumer,
                                 ismEngine_DeliveryHandle_t  hDelivery,
                                 ismEngine_MessageHandle_t   hMessage,
                                 uint32_t                    deliveryId,
                                 ismMessageState_t           state,
                                 uint32_t                    destinationOptions,
                                 ismMessageHeader_t *        pMsgDetails,
                                 uint8_t                     areaCount,
                                 ismMessageAreaType_t        areaTypes[areaCount],
                                 size_t                      areaLengths[areaCount],
                                 void *                      pAreaData[areaCount],
                                 void *                      pConsumerContext,
                                 ismEngine_DelivererContext_t * _delivererContext )
{
    uint32_t *pMessagesDelivered = *(uint32_t **)pConsumerContext;

    (*pMessagesDelivered)++; //A message arrived...

    //Was it the one we expected
    uint32_t msgArrivedNum = *(uint32_t *)pAreaData[0];
    test_log(3, "Message %u arrived",msgArrivedNum);
    TEST_ASSERT_EQUAL(msgArrivedNum, *pMessagesDelivered);

    //Free our copy of the message
    ism_engine_releaseMessage(hMessage);

    //NB: We don't ack the message

    return true; //We always want another message
}

/*********************************************************************/
/* Name:    NackOnSessionClose                                       */
/* Description:                                                      */
/* Check that is a session is closed before messages are acked, they */
/* are put back on the queue                                         */
/*********************************************************************/
static void NackOnSessionClose( void )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismQHandle_t hQueue;
    ismEngine_QueueStatistics_t stats;
    ismEngine_Consumer_t *pConsumer;
    char queueName[32];
    volatile uint32_t messagesDelivered = 0;
    volatile uint32_t *pMessagesDelivered = &messagesDelivered;
    uint32_t messagesToPut = 150;

    // Create Session for this test
    rc=ism_engine_createSession( hMCQTestClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Create the queue
    ieqnQueue_t *pNamedQueue;
    strcpy(queueName, __func__);
    rc = ieqn_createQueue( pThreadData
                         , queueName
                         , multiConsumer
                         , ismQueueScope_Server
                         , NULL
                         , NULL
                         , NULL
                         , &pNamedQueue );
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pNamedQueue);

    test_log(4, "Created queue");

    hQueue = ieqn_getQueueHandle(pThreadData, queueName);
    TEST_ASSERT_PTR_NOT_NULL(hQueue);
    TEST_ASSERT_EQUAL(pNamedQueue->queueHandle, hQueue);

    test_log(4, "Create consumer");
    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , queueName
                                  , 0
                                  , NULL // Unused for QUEUE
                                  , &pMessagesDelivered
                                  , sizeof(pMessagesDelivered)
                                  , NackOnSessionCloseCB
                                  , NULL
                                  , ismENGINE_CONSUMER_OPTION_ACK
                                  , &pConsumer
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    //Start Message Delivery on the consumer
    rc = ism_engine_startMessageDelivery( hSession,
                                          ismENGINE_START_DELIVERY_OPTION_NONE,
                                          NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Create a transaction and commit a message under it
    ismEngine_Transaction_t *putTran;
    rc = ietr_createLocal(pThreadData, NULL, true, false, NULL, &putTran);
    TEST_ASSERT_EQUAL(rc, OK);


    TEST_ASSERT_EQUAL(rc, OK);

    for (uint32_t i=0; i<messagesToPut; i++)
    {
        ismEngine_MessageHandle_t hMessageMsg = NULL;
        ismMessageHeader_t headerMsg = ismMESSAGE_HEADER_DEFAULT;
        headerMsg.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
        headerMsg.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
        ismMessageAreaType_t areaTypesMsg[1] = {ismMESSAGE_AREA_PAYLOAD};
        size_t areaLengthsMsg[1] = {sizeof(uint32_t)};
        uint32_t msgNum = 1+i;
        void *areaDataMsg[1] = {&msgNum};

        rc = ism_engine_createMessage(&headerMsg,
                                      1,
                                      areaTypesMsg,
                                      areaLengthsMsg,
                                      areaDataMsg,
                                      &hMessageMsg);
        rc = ieq_put( pThreadData
                    , hQueue
                    , ieqPutOptions_NONE
                    , putTran
                    , hMessageMsg
                    , IEQ_MSGTYPE_REFCOUNT
                    , NULL );
        TEST_ASSERT_EQUAL(rc, OK);

        ism_engine_releaseMessage(hMessageMsg);
    }

    //Check no messages have been delivered
    TEST_ASSERT_EQUAL(messagesDelivered, 0);

    //Commit the put
    rc = ietr_commit(pThreadData, putTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    //Check message 1 arrives
    test_waitForMessages(&messagesDelivered, NULL, messagesToPut, 20);
    TEST_ASSERT_EQUAL(messagesDelivered, messagesToPut);

    //Check that the queue still has the messages as there are no acks
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, messagesToPut);


    // Destroy session which should nack message
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);

    messagesDelivered =0;

    // Create Session for this test
    rc = ism_engine_createSession( hMCQTestClient
                                 , ismENGINE_CREATE_SESSION_OPTION_NONE
                                 , &hSession
                                 , NULL
                                 , 0
                                 , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_log(4, "Recreate consumer");
    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , queueName
                                  , 0
                                  , NULL // Unused for QUEUE
                                  , &pMessagesDelivered
                                  , sizeof(pMessagesDelivered)
                                  , NackOnSessionCloseCB
                                  , NULL
                                  , ismENGINE_CONSUMER_OPTION_ACK
                                  , &pConsumer
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    //Start Message Delivery on the consumer
    rc = ism_engine_startMessageDelivery( hSession,
                                          ismENGINE_START_DELIVERY_OPTION_NONE,
                                          NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Check message  arrives
    test_waitForMessages(&messagesDelivered, NULL, messagesToPut, 20);
    test_log(testLOGLEVEL_TESTPROGRESS, "Delivered:%d Put:%d", messagesDelivered, messagesToPut);

    TEST_ASSERT_EQUAL(messagesDelivered, messagesToPut);

    // Destroy session which should nack message
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);

    // Log the contents of the queue when verbose logging
    test_log_queueHandle(testLOGLEVEL_VERBOSE, hQueue, 9, -1, "");

    // Delete Q
    rc = ieqn_destroyQueue(pThreadData, queueName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    test_log(4, "Deleted queue");

    return;
}


/*****************************************************************/
/* Things for DurableSubReconnect                                */
/*****************************************************************/

typedef struct tag_testMessage_t
{
    char StructId[4]; ///< TMSG
    uint32_t pubberNum;
    uint32_t msgNum;
} testMessage_t;
#define TESTMSG_STRUCTID       "TMSG"
#define TESTMSG_STRUCTID_ARRAY {'T','M','S','G'}

ismEngine_MessageHandle_t createMessage(ismEngine_SessionHandle_t hSession,
                                        uint32_t publisherId,
                                        uint32_t msgNum)
{
    ismEngine_MessageHandle_t hMessage = NULL;
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};

    testMessage_t msgPayload = { TESTMSG_STRUCTID_ARRAY,
            publisherId,
            msgNum };
    size_t areaLengths[2] = {0,sizeof(testMessage_t)};
    void *areaData[2] = {NULL, (void *)&msgPayload};

    header.Flags       = ismMESSAGE_FLAGS_NONE;
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    header.Reliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;

    DEBUG_ONLY int32_t rc;

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    return hMessage;
}

static bool DurableSubReconnectCB(ismEngine_ConsumerHandle_t  hConsumer,
                                 ismEngine_DeliveryHandle_t  hDelivery,
                                 ismEngine_MessageHandle_t   hMessage,
                                 uint32_t                    deliveryId,
                                 ismMessageState_t           state,
                                 uint32_t                    destinationOptions,
                                 ismMessageHeader_t *        pMsgDetails,
                                 uint8_t                     areaCount,
                                 ismMessageAreaType_t        areaTypes[areaCount],
                                 size_t                      areaLengths[areaCount],
                                 void *                      pAreaData[areaCount],
                                 void *                      pConsumerContext,
                                 ismEngine_DelivererContext_t * _delivererContext )
{
    testMessage_t *msg =  pAreaData[1];
    test_log(3, "Message arrived publisher %u, msgNum: %u",msg->pubberNum, msg->msgNum);

    //Free our copy of the message
    ism_engine_releaseMessage(hMessage);

    //NB: We don't ack the message

    return true; //We always want another message
}

/*********************************************************************/
/* Name:    DurableSubReconnect                                      */
/* Description:                                                      */
/* Disconnect/reconnect a consumer to a durable sub,                 */
/*********************************************************************/
static void DurableSubReconnect( void )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession,hSession2;
    ismEngine_ConsumerHandle_t hConsumer,hConsumer2;
    const char *subName     = "sub:DurableSubReconnect";
    const char *topicString = "/topic/DurableSubReconnect";
    uint32_t numMessagesPerBatch    = 270;
    uint32_t numDifferentPublishers = 3;
    uint32_t messagesReceivedFromPublisher[numDifferentPublishers];
    ismEngine_ProducerHandle_t hProducer[numDifferentPublishers];

    for (uint i=0; i<numDifferentPublishers; i++)
    {
        messagesReceivedFromPublisher[i]=0;
        hProducer[i] = NULL;
    }


    // Create Session for this test
    rc=ism_engine_createSession( hMCQTestClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE };

    rc = sync_ism_engine_createSubscription(
                hMCQTestClient,
                subName,
                NULL,
                ismDESTINATION_TOPIC,
                topicString,
                &subAttrs,
                NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_TOPIC,
                                   topicString,
                                   &hProducer[0],
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    test_incrementActionsRemaining(pActionsRemaining, numMessagesPerBatch);
    for (uint32_t i = 0; i < numMessagesPerBatch; i++)
    {
        ismEngine_MessageHandle_t hMessage = createMessage(hSession, 0, i);

        rc = ism_engine_putMessage(hSession,
                                   hProducer[0],
                                   NULL,
                                   hMessage,
                                   &pActionsRemaining,
                                   sizeof(pActionsRemaining),
                                   test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }

    test_waitForRemainingActions(pActionsRemaining);

    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_SUBSCRIPTION
                                  , subName
                                  , NULL
                                  , NULL // Owning client same as session client
                                  , &messagesReceivedFromPublisher
                                  , sizeof(uint32_t **)
                                  , DurableSubReconnectCB
                                  , NULL
                                  , ismENGINE_CONSUMER_OPTION_ACK
                                  , &hConsumer
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(
            hSession,
            ismENGINE_START_DELIVERY_OPTION_NONE,
            NULL,
            0,
            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_TOPIC,
                                   topicString,
                                   &hProducer[1],
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, numMessagesPerBatch);
    for (uint32_t i = 0; i < numMessagesPerBatch; i++)
    {
        ismEngine_MessageHandle_t hMessage = createMessage(hSession, 1, i);

        rc = ism_engine_putMessage(hSession,
                                   hProducer[1],
                                   NULL,
                                   hMessage,
                                   &pActionsRemaining,
                                   sizeof(pActionsRemaining),
                                   test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }

    test_waitForRemainingActions(pActionsRemaining);

    rc = sync_ism_engine_destroySession(hSession);
    TEST_ASSERT_EQUAL(rc, OK);

    test_log(3, "Destroyed first session");

    rc=ism_engine_createSession( hMCQTestClient
                                 , ismENGINE_CREATE_SESSION_OPTION_NONE
                                 , &hSession2
                                 , NULL
                                 , 0
                                 , NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    rc = ism_engine_createProducer(hSession2,
                                   ismDESTINATION_TOPIC,
                                   topicString,
                                   &hProducer[2],
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, numMessagesPerBatch);
    for (uint32_t i = 0; i < numMessagesPerBatch; i++)
    {
        ismEngine_MessageHandle_t hMessage = createMessage(hSession2, 2, i);

        rc = ism_engine_putMessage(hSession2,
                                   hProducer[2],
                                   NULL,
                                   hMessage,
                                   &pActionsRemaining,
                                   sizeof(pActionsRemaining),
                                   test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }

    test_waitForRemainingActions(pActionsRemaining);

    rc = ism_engine_createConsumer( hSession2
                                  , ismDESTINATION_SUBSCRIPTION
                                  , subName
                                  , NULL
                                  , NULL // Owning client same as session client
                                  , &messagesReceivedFromPublisher
                                  , sizeof(uint32_t **)
                                  , DurableSubReconnectCB
                                  , NULL
                                  , ismENGINE_CONSUMER_OPTION_ACK
                                  , &hConsumer2
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(
            hSession2,
            ismENGINE_START_DELIVERY_OPTION_NONE,
            NULL,
            0,
            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroyConsumer(hConsumer2);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySubscription(hMCQTestClient, subName, hMCQTestClient, NULL,0,NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ieal_debugAckList(pThreadData, (ismEngine_Session_t *)hSession2);

    rc = sync_ism_engine_destroySession(hSession2);
    TEST_ASSERT_EQUAL(rc, OK);
}

static uint32_t receivedMsgs = 0;
bool receivedMsgCallback(
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
    __sync_add_and_fetch(&receivedMsgs, 1);


    if (hDelivery != 0)
    {
        int32_t rc = ism_engine_confirmMessageDelivery( ((ismEngine_Consumer_t *)hConsumer)->pSession
                , NULL
                , hDelivery
                , ismENGINE_CONFIRM_OPTION_CONSUMED
                , NULL
                , 0
                , NULL );
        TEST_ASSERT_EQUAL(rc, OK);
    }


    ism_engine_releaseMessage(hMessage);
    //We'd like more messages
    return true;
}

//Just checks the browse option is accepted and it doesn't consume messages....
void test_BasicBrowse(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer=NULL;

    int numMessages = 250;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    /* Create our clients and sessions */
    rc = ism_engine_createClientState("basicbrowser",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL,
                                      NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_BROWSE_ONLY,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    rc = ism_engine_startMessageDelivery(hSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Create the queue
    char *qName = "test_BasicBrowse";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server, NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    QPreloadMessages( qName, numMessages);

    /* And then the consumer (actually a browser) */
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   qName,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Owning client same as session client
                                   NULL,
                                   0,
                                   receivedMsgCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_BROWSE|ismENGINE_CONSUMER_OPTION_PAUSE,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Should be numMessages available messages
    rc = ism_engine_checkAvailableMessages(hConsumer);
    TEST_ASSERT_EQUAL(rc, OK);

    //Because of the pause option, no messages should have been delivered yet despite the start delivery call...
    TEST_ASSERT_EQUAL(receivedMsgs, 0);

    rc = ism_engine_resumeMessageDelivery( hConsumer
                                         , ismENGINE_RESUME_DELIVERY_OPTION_NONE
                                         , NULL, 0, NULL);

    //Messages should now have been delivered so our received message count should have increased
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(receivedMsgs, 250);

    //Check there are still messages on the queue.
    ismEngine_QueueStatistics_t qstats;
    ieq_getStats(pThreadData, ((ismEngine_Consumer_t *)hConsumer)->queueHandle, &qstats);
    ieq_setStats(((ismEngine_Consumer_t *)hConsumer)->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(qstats.BufferedMsgs, numMessages);

    //Check we can browse them again...
    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    receivedMsgs = 0;

    rc = ism_engine_createConsumer(hSession,
            ismDESTINATION_QUEUE,
            qName,
            ismENGINE_SUBSCRIPTION_OPTION_NONE,
            NULL, // Unused for QUEUE
            NULL,
            0,
            receivedMsgCallback,
            NULL,
            ismENGINE_CONSUMER_OPTION_BROWSE,
            &hConsumer,
            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(receivedMsgs, 250);

    // Should be no messages still available for this consumer
    rc = ism_engine_checkAvailableMessages(hConsumer);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMsgAvail);

    //Check there are still messages on the queue.
    ieq_getStats(pThreadData, ((ismEngine_Consumer_t *)hConsumer)->queueHandle, &qstats);
    ieq_setStats(((ismEngine_Consumer_t *)hConsumer)->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(qstats.BufferedMsgs, numMessages);

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that a paused browser sees that messages are available again
    receivedMsgs = 0;

    rc = ism_engine_createConsumer(hSession,
            ismDESTINATION_QUEUE,
            qName,
            ismENGINE_SUBSCRIPTION_OPTION_NONE,
            NULL, // Unused for QUEUE
            NULL,
            0,
            receivedMsgCallback,
            NULL,
            ismENGINE_CONSUMER_OPTION_BROWSE | ismENGINE_CONSUMER_OPTION_PAUSE,
            &hConsumer,
            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(receivedMsgs, 0);

    // Should be messages available for this consumer
    rc = ism_engine_checkAvailableMessages(hConsumer);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Check that a destructive getter can remove the messages...
    receivedMsgs = 0;

    rc = ism_engine_createConsumer(hSession,
            ismDESTINATION_QUEUE,
            qName,
            ismENGINE_SUBSCRIPTION_OPTION_NONE,
            NULL, // Unused for QUEUE
            NULL,
            0,
            receivedMsgCallback,
            NULL,
            ismENGINE_CONSUMER_OPTION_ACK,
            &hConsumer,
            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(receivedMsgs, 250);

    //Check there are no messages on the queue.
    ieq_getStats(pThreadData, ((ismEngine_Consumer_t *)hConsumer)->queueHandle, &qstats);
    ieq_setStats(((ismEngine_Consumer_t *)hConsumer)->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(qstats.BufferedMsgs, 0);

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //The producer is gone so we should be able to delete the queue, discarding messages...
    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

bool MultiBrowseTestMsgCallback(
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
    uint32_t *pMessagesDelivered = *(uint32_t **)pConsumerContext;

    //Check that the message we received is the next one we expect...
    testMessage_t *msg =  pAreaData[1];
    TEST_ASSERT_EQUAL(msg->pubberNum, 0);
    TEST_ASSERT_EQUAL(msg->msgNum, *pMessagesDelivered);

    (*pMessagesDelivered)++; //A message arrived...

    if (hDelivery != 0)
    {
        int32_t rc = ism_engine_confirmMessageDelivery( ((ismEngine_Consumer_t *)hConsumer)->pSession
                , NULL
                , hDelivery
                , ismENGINE_CONFIRM_OPTION_CONSUMED
                , NULL
                , 0
                , NULL );
        TEST_ASSERT_EQUAL(rc, OK);
    }


    ism_engine_releaseMessage(hMessage);
    //We'd like more messages
    return true;
}

//Just test drain queue
void test_DrainQueue(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create the queue
    char *qName = "test_DrainQueue";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server, NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    QPreloadMessages( qName, 10);

    rc = ieq_drain(pThreadData, ieqn_getQueueHandle(pThreadData, qName));
    TEST_ASSERT_EQUAL(rc, ISMRC_NotImplemented);

    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

typedef struct tag_multiBrowseUncommittedPutterContext_t
{
    char *qName;
    ismEngine_ClientStateHandle_t hClient;
} multiBrowseUncommittedPutterContext_t;

void *multiBrowseUncommittedPutter(void *arg)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    multiBrowseUncommittedPutterContext_t *pContext = (multiBrowseUncommittedPutterContext_t *)arg;

    int32_t rc = OK;

    ism_engine_threadInit(0);

    ismEngine_SessionHandle_t hSession=NULL;
    rc = ism_engine_createSession(pContext->hClient,
            ismENGINE_CREATE_SESSION_OPTION_NONE,
            &hSession,
            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    ismEngine_MessageHandle_t hMessage = createMessage(hSession, 1, 0);

    ismEngine_Transaction_t *hTran;
    rc = ietr_createLocal(pThreadData, hSession, true, false, NULL, &hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessageOnDestination( hSession
            , ismDESTINATION_QUEUE
            , pContext->qName
            , hTran
            , hMessage
            , NULL, 0, NULL);

    TEST_ASSERT_EQUAL(rc, OK);

    //Leave the thread without committing the transaction (so the message should not be browsed), we'll roll it back when we delete the client state.
    return NULL;
}

//Creates multiple browsers which read a queue in parallel whilst messages are streamed to it...there are also a number of uncommitted messages added by other putters into the stream
void test_MultiBrowse(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;

    int numMessages = 1000;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);


    // Create the queue
    char *qName = "test_MultiBrowse";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server, NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Set up a client that we'll use for all the puts/browsers
    ismEngine_ClientStateHandle_t hClient=NULL;
    //...and a session for the Browsers
    ismEngine_SessionHandle_t hBrowseSession=NULL;

    rc = ism_engine_createClientState("basicbrowseClient",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL,
                                      NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_BROWSE_ONLY,
                                  &hBrowseSession,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hBrowseSession);

    rc = ism_engine_startMessageDelivery(hBrowseSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Create two different browsers
    ismEngine_ConsumerHandle_t hConsumer1;
    uint32_t  cons1MsgsRcvd = 0;
    uint32_t *pCons1MsgsRcvd = &cons1MsgsRcvd;

    rc = ism_engine_createConsumer(hBrowseSession,
                                   ismDESTINATION_QUEUE,
                                   qName,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   &pCons1MsgsRcvd,
                                   sizeof(pCons1MsgsRcvd),
                                   MultiBrowseTestMsgCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_BROWSE,
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Create two different browsers
    ismEngine_ConsumerHandle_t hConsumer2;
    uint32_t  cons2MsgsRcvd = 0;
    uint32_t *pCons2MsgsRcvd = &cons2MsgsRcvd;
    rc = ism_engine_createConsumer(hBrowseSession,
                                   ismDESTINATION_QUEUE,
                                   qName,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   &pCons2MsgsRcvd,
                                   sizeof(pCons1MsgsRcvd),
                                   MultiBrowseTestMsgCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_BROWSE,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Put Messages on the queue occasionally spawning a thread to put but not commit a message..threads all use the same client..and end after the put
    ismEngine_SessionHandle_t hPutSession;

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hPutSession,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t totalUncommittedPutThreads = 4;
    uint32_t launchedUncommittedPutThreads = 0;
    pthread_t threadIds[totalUncommittedPutThreads];
    multiBrowseUncommittedPutterContext_t threadContext = {qName, hClient};


    uint32_t msgIncrementPerThreadLaunch = numMessages/(totalUncommittedPutThreads+3);
    for (uint32_t i=0; i < numMessages; i++)
    {
        //Shall we launch a thread to do an uncommitted put?
        if (  (launchedUncommittedPutThreads < totalUncommittedPutThreads)
            &&(numMessages <= launchedUncommittedPutThreads*msgIncrementPerThreadLaunch))
        {
            int32_t os_rc = test_task_startThread( &(threadIds[launchedUncommittedPutThreads]) ,multiBrowseUncommittedPutter, &threadContext,"multiBrowseUncommittedPutter");
            TEST_ASSERT_EQUAL(os_rc, OK);

            launchedUncommittedPutThreads++;
        }
        ismEngine_MessageHandle_t hMessage = createMessage(hPutSession, 0, i);

        ismEngine_Transaction_t *hTran;
        rc = ietr_createLocal(pThreadData, hPutSession, true, false, NULL, &hTran);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination( hPutSession
                                               , ismDESTINATION_QUEUE
                                               , qName
                                               , hTran
                                               , hMessage
                                               , NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        //Commit the consuming of the message
        rc = ietr_commit(pThreadData, hTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, hPutSession, NULL, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Join all the threads
    for (uint32_t i=0; i<launchedUncommittedPutThreads; i++)
    {
        int32_t os_rc = pthread_join(threadIds[i], NULL);
        TEST_ASSERT_EQUAL(os_rc, OK);
    }

    //Check the Browsers received the right number of msgs (all the committed ones from this thread and none of the uncommitted ones)
    TEST_ASSERT_EQUAL(cons1MsgsRcvd, numMessages);
    TEST_ASSERT_EQUAL(cons2MsgsRcvd, numMessages);

    //destroy the client which will rollback all the uncommitted puts from the other threads...
    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);



    //The producer is gone so we should be able to delete the queue, discarding messages...
    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

// Creates a queue and load's with messages with a property containing the
// value RED, GREEN or BLUE. A consumer is then created to consume GREEN
// messages.
typedef struct UnConsumedMsgList_t
{
    uint32_t ListSize;
    uint32_t ListUsed;
    ismEngine_DeliveryHandle_t hDelivery[1];
} UnConsumedMsgList_t;

typedef struct SelectionContext_t 
{
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    uint32_t MessagesReceived;
    uint32_t expectedColourIndex;
    bool suspendOnNextMsg;
    bool consumeMsgs;
    UnConsumedMsgList_t *UnConsumedMsgList;
} SelectionContext_t;

bool SelectionCallback( ismEngine_ConsumerHandle_t      hConsumer
                      , ismEngine_DeliveryHandle_t      hDelivery
                      , ismEngine_MessageHandle_t       hMessage
                      , uint32_t                        deliveryId
                      , ismMessageState_t               state
                      , uint32_t                        destinationOptions
                      , ismMessageHeader_t *            pMsgDetails
                      , uint8_t                         areaCount
                      , ismMessageAreaType_t            areaTypes[areaCount]
                      , size_t                          areaLengths[areaCount]
                      , void *                          pAreaData[areaCount]
                      , void *                          pConsumerContext
                      , ismEngine_DelivererContext_t *  delivererContext)
{
    int32_t rc;
    SelectionContext_t *pContext = *(SelectionContext_t **)pConsumerContext;

    // Check that the message we received matches the selector we expect
    // To be done
    TEST_ASSERT_EQUAL(areaCount, 2);

    // Increment the count of messages received
    pContext->MessagesReceived++;

    test_log(testLOGLEVEL_VERBOSE, "Received message '%s'", pAreaData[1]);

    if (hDelivery != 0) 
    {
        if (pContext->consumeMsgs)
        {
            rc = ism_engine_confirmMessageDelivery( pContext->hSession
                                                  , NULL
                                                  , hDelivery
                                                  , ismENGINE_CONFIRM_OPTION_CONSUMED
                                                  , NULL
                                                  , 0
                                                  , NULL );
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else if (pContext->UnConsumedMsgList != NULL)
        {
            UnConsumedMsgList_t *pUnConsumedMsgList = pContext->UnConsumedMsgList;

            if (pUnConsumedMsgList->ListUsed == pUnConsumedMsgList->ListSize)
            {
                uint32_t NewSize = pUnConsumedMsgList->ListSize*2;
                UnConsumedMsgList_t * tempPUnConsumedMsgList = realloc(pUnConsumedMsgList, sizeof(UnConsumedMsgList_t) + ((sizeof(ismEngine_DeliveryHandle_t) * NewSize)-1));
                if(tempPUnConsumedMsgList == NULL){
                    free(pUnConsumedMsgList);
                    abort();
                }
                pUnConsumedMsgList = tempPUnConsumedMsgList;
                assert(pUnConsumedMsgList != NULL);
                pContext->UnConsumedMsgList = pUnConsumedMsgList;
                pContext->UnConsumedMsgList->ListSize = NewSize;
            }

            pUnConsumedMsgList->hDelivery[pUnConsumedMsgList->ListUsed++] = hDelivery;
        }   
    }

    ism_engine_releaseMessage(hMessage);

    //We'd like more messages
    return pContext->suspendOnNextMsg?false:true;
}

// This test validates the basic selection functionality
void test_BasicSelection(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    uint32_t loop;
    uint32_t messageCount = 90;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer;
    char *ColourText[] = { "BLUE", "RED", "GREEN" };
    SelectionContext_t ConsumerContext = { NULL, NULL, 0, 0, false, true };
    SelectionContext_t *pConsumerContext = &ConsumerContext;
    char payload[100];

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create the queue
    char *qName = "test_BasicSelection";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server,
                          NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Set up a client that we'll use for all the puts/browsers
    rc = ism_engine_createClientState("BasicSelectionClient",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL,
                                      NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    // Now publish 90 messages round-robin the property value
    for (loop=0; loop < messageCount; loop++)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismMessageAreaType_t areaTypes[2];
        size_t areaLengths[2];
        void *areas[2];
        concat_alloc_t FlatProperties = { NULL };
        char localPropBuffer[1024];

        ism_prop_t *properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        ism_field_t ColourField = {VT_String, 0, {.s = ColourText[(loop/3) % 3] }};
        rc = ism_common_setProperty(properties, "Colour", &ColourField);
        TEST_ASSERT_EQUAL(rc, OK);

        FlatProperties.buf = localPropBuffer;
        FlatProperties.len = 1024;

        rc = ism_common_serializeProperties(properties, &FlatProperties);
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(payload, "Message - %d. Property is '%s'", loop,
                ColourText[(loop / 3) % 3]);

        areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
        areaLengths[0] = FlatProperties.used;
        areas[0] = FlatProperties.buf;
        areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[1] = strlen(payload) +1;
        areas[1] = (void *)payload;

        header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

        ismEngine_MessageHandle_t hMessage = NULL;
        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areas,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_QUEUE,
                                                qName,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        ism_common_freeProperties(properties);
    }

    // Having populated the queue with messages, now create a consumer with
    // a selection string
    ism_prop_t *properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);

    char *selectorString = "Colour='BLUE'";
    ism_field_t SelectorField = {VT_String, 0, {.s = selectorString }};
    rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now create the subscription
    pConsumerContext->hSession = hSession;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   qName,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   &pConsumerContext,
                                   sizeof(SelectionContext_t *),
                                   SelectionCallback,
                                   properties,
                                   ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION|ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And free the properties we allocated
    ism_common_freeProperties(properties);

    // And now start message delivery. When this call completes all of the
    // matching messages should have been delivered to the consumer.
    rc = ism_engine_startMessageDelivery(hSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Validate that 30 messages were received.
    TEST_ASSERT_EQUAL(ConsumerContext.MessagesReceived, 30);

    uint64_t failedSelectionCount;
    uint64_t succesfullSelectionCount;
    ism_engine_getSelectionStats( hConsumer
                                , &failedSelectionCount
                                , &succesfullSelectionCount );

    test_log(testLOGLEVEL_TESTPROGRESS, "Number of successful selection attempts is %ld", succesfullSelectionCount);
    test_log(testLOGLEVEL_TESTPROGRESS, "Number of failed selection attempts is %ld\n", failedSelectionCount);

    test_log_clientState(testLOGLEVEL_VERBOSE, ((ismEngine_ClientState_t *)hClient)->pClientId, 9, -1, "");

    rc=ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    
    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

// This test validates the basic selection functionality using a pre-compiled
// selection string
void test_BasicSelectionCompiled(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    uint32_t loop;
    uint32_t messageCount = 90;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer;
    char *ColourText[] = { "BLUE", "RED", "GREEN" };
    SelectionContext_t ConsumerContext = { NULL, NULL, 0, 0, false, true };
    SelectionContext_t *pConsumerContext = &ConsumerContext;
    char payload[100];

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create the queue
    char *qName = "test_BasicSelectionCompiled";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server,
                          NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Set up a client that we'll use for all the puts/browsers
    rc = ism_engine_createClientState("CompiledSelectionClient",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL,
                                      NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_BROWSE_ONLY,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    // Now publish 90 messages round-robin the property value
    for (loop=0; loop < messageCount; loop++)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismMessageAreaType_t areaTypes[2];
        size_t areaLengths[2];
        void *areas[2];
        concat_alloc_t FlatProperties = { NULL };
        char localPropBuffer[1024];

        ism_prop_t *properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        ism_field_t ColourField = {VT_String, 0, {.s = ColourText[(loop/3) % 3] }};
        rc = ism_common_setProperty(properties, "Colour", &ColourField);
        TEST_ASSERT_EQUAL(rc, OK);

        FlatProperties.buf = localPropBuffer;
        FlatProperties.len = 1024;

        rc = ism_common_serializeProperties(properties, &FlatProperties);
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(payload, "Message - %d. Property is '%s'", loop,
                ColourText[(loop / 3) % 3]);

        areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
        areaLengths[0] = FlatProperties.used;
        areas[0] = FlatProperties.buf;
        areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[1] = strlen(payload) +1;
        areas[1] = (void *)payload;

        header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

        ismEngine_MessageHandle_t hMessage = NULL;
        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areas,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_QUEUE,
                                                qName,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        ism_common_freeProperties(properties);
    }

    // Having populated the queue with messages, now create a consumer with
    // a selection string
    ism_prop_t *properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);

    char *selectorString = "Colour='BLUE'";
    uint32_t selectionRuleLen;
    struct ismRule_t *selectionRule;
    rc = ism_common_compileSelectRule(&selectionRule,
                                      (int *)&selectionRuleLen,
                                      selectorString);

    ism_field_t SelectorField = {VT_ByteArray, selectionRuleLen, {.s = (char *)selectionRule }};
    rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
    TEST_ASSERT_EQUAL(rc, OK);

    // Free the compiled selection rule
    ism_common_freeSelectRule(selectionRule);

    // Now create the subscription
    pConsumerContext->hSession = hSession;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   qName,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   &pConsumerContext,
                                   sizeof(SelectionContext_t *),
                                   SelectionCallback,
                                   properties,
                                   ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION|ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And free the properties we allocated
    ism_common_freeProperties(properties);

    // And now start message delivery. When this call completes all of the
    // matching messages should have been delivered to the consumer.
    rc = ism_engine_startMessageDelivery(hSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Validate that 30 messages were received.
    TEST_ASSERT_EQUAL(ConsumerContext.MessagesReceived, 30);

    uint64_t failedSelectionCount;
    uint64_t succesfullSelectionCount;
    ism_engine_getSelectionStats( hConsumer
                                , &failedSelectionCount
                                , &succesfullSelectionCount );

    test_log(testLOGLEVEL_TESTPROGRESS, "Number of successful selection attempts is %ld", succesfullSelectionCount);
    test_log(testLOGLEVEL_TESTPROGRESS, "Number of failed selection attempts is %ld\n", failedSelectionCount);

    test_log_queue(testLOGLEVEL_VERBOSE, qName, 9, -1, "");

    rc=ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    
    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

// This test validates the basic selection functionality
void test_BrowseSelection(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    uint32_t loop;
    uint32_t messageCount = 90;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer;
    char *ColourText[] = { "BLUE", "RED", "GREEN" };
    SelectionContext_t ConsumerContext = { NULL, NULL, 0, 0, false, true };
    SelectionContext_t *pConsumerContext = &ConsumerContext;
    char payload[100];

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create the queue
    char *qName = "test_BrowseSelection";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server,
                          NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Set up a client that we'll use for all the puts/browsers
    rc = ism_engine_createClientState("BrowseSelectionClient",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL,
                                      NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_BROWSE_ONLY,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    // Now publish 90 messages round-robin the property value
    for (loop=0; loop < messageCount; loop++)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismMessageAreaType_t areaTypes[2];
        size_t areaLengths[2];
        void *areas[2];
        concat_alloc_t FlatProperties = { NULL };
        char localPropBuffer[1024];

        ism_prop_t *properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        ism_field_t ColourField = {VT_String, 0, {.s = ColourText[(loop/3) % 3] }};
        rc = ism_common_setProperty(properties, "Colour", &ColourField);
        TEST_ASSERT_EQUAL(rc, OK);

        FlatProperties.buf = localPropBuffer;
        FlatProperties.len = 1024;

        rc = ism_common_serializeProperties(properties, &FlatProperties);
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(payload, "Message - %d. Property is '%s'", loop,
                ColourText[(loop / 3) % 3]);

        areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
        areaLengths[0] = FlatProperties.used;
        areas[0] = FlatProperties.buf;
        areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[1] = strlen(payload) +1;
        areas[1] = (void *)payload;

        header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

        ismEngine_MessageHandle_t hMessage = NULL;
        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areas,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_QUEUE,
                                                qName,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        ism_common_freeProperties(properties);
    }

    // Having populated the queue with messages, now create a consumer with
    // a selection string
    ism_prop_t *properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);

    char *selectorString = "Colour='BLUE'";
    ism_field_t SelectorField = {VT_String, 0, {.s = selectorString }};
    rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now create the subscription
    pConsumerContext->hSession = hSession;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   qName,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for queue
                                   &pConsumerContext,
                                   sizeof(SelectionContext_t *),
                                   SelectionCallback,
                                   properties,
                                   ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION
                                 | ismENGINE_CONSUMER_OPTION_BROWSE,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And free the properties we allocated
    ism_common_freeProperties(properties);

    // And now start message delivery. When this call completes all of the
    // matching messages should have been delivered to the consumer.
    rc = ism_engine_startMessageDelivery(hSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Validate that 30 messages were received.
    TEST_ASSERT_EQUAL(ConsumerContext.MessagesReceived, 30);

    // Log the contents of the queue
    test_log_queue(testLOGLEVEL_VERBOSE, qName, 9, -1, "");

    rc=ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

// This test validates that the get cursor moves past uncommitted messages
// but will roll-back when messages are committed which were skipped over.
void test_SelectionLatePutCommit(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    uint32_t loop;
    uint32_t messageCount = 90;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer;
    char *ColourText[] = { "BLUE", "RED", "GREEN" };
    SelectionContext_t ConsumerContext = { NULL, NULL, 0, 0, false, true };
    SelectionContext_t *pConsumerContext = &ConsumerContext;
    char payload[100];
    int transactionSize = 18;  // messages are committed in batches of 18;
    int tranNumber = 0;
    int msgsInTran = 0;
    ismEngine_Transaction_t *transactions[ (messageCount + transactionSize-1) / transactionSize];
    ismEngine_Transaction_t *pTran = NULL;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create the queue
    char *qName = "test_SelectionLatePutCommit";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server,
                          NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Set up a client that we'll use for all the puts/browsers
    rc = ism_engine_createClientState("LateSelectionClient",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL,
                                      NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_BROWSE_ONLY,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    // Now publish 90 messages round-robin the property value
    for (loop=0; loop < messageCount; loop++)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismMessageAreaType_t areaTypes[2];
        size_t areaLengths[2];
        void *areas[2];
        concat_alloc_t FlatProperties = { NULL };
        char localPropBuffer[1024];

        if (pTran == NULL)
        {
            rc = ietr_createLocal( pThreadData
                                 , hSession
                                 , true
                                 , false
								 , NULL
                                 , &transactions[tranNumber]);
            TEST_ASSERT_EQUAL(rc, OK);
            pTran = transactions[tranNumber];
            msgsInTran = 0;
        }

        ism_prop_t *properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        ism_field_t ColourField = {VT_String, 0, {.s = ColourText[(loop/3) % 3] }};
        rc = ism_common_setProperty(properties, "Colour", &ColourField);
        TEST_ASSERT_EQUAL(rc, OK);

        FlatProperties.buf = localPropBuffer;
        FlatProperties.len = 1024;

        rc = ism_common_serializeProperties(properties, &FlatProperties);
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(payload, "Message - %d. Property is '%s'", loop,
                ColourText[(loop / 3) % 3]);

        areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
        areaLengths[0] = FlatProperties.used;
        areas[0] = FlatProperties.buf;
        areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[1] = strlen(payload) +1;
        areas[1] = (void *)payload;

        header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

        ismEngine_MessageHandle_t hMessage = NULL;
        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areas,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_QUEUE,
                                                qName,
                                                pTran,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        msgsInTran++;
        // Commit the transaction if it is full
        if (msgsInTran == transactionSize)
        {
            // Don't commit the 2nd transaction
            if (tranNumber != 1)
            {
                ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, hSession, NULL, NULL);
            }
            pTran = NULL;
            tranNumber++;
        }

        ism_common_freeProperties(properties);
    }

    // Having populated the queue with messages, now create a consumer with
    // a selection string
    ism_prop_t *properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);

    char *selectorString = "Colour='BLUE'";
    ism_field_t SelectorField = {VT_String, 0, {.s = selectorString }};
    rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now create the subscription
    pConsumerContext->hSession = hSession;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   qName,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   &pConsumerContext,
                                   sizeof(SelectionContext_t *),
                                   SelectionCallback,
                                   properties,
                                   ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION|ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And free the properties we allocated
    ism_common_freeProperties(properties);

    // And now start message delivery. When this call completes all of the
    // matching messages should have been delivered to the consumer.
    rc = ism_engine_startMessageDelivery(hSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Validate that 24 messages were received.
    TEST_ASSERT_EQUAL(ConsumerContext.MessagesReceived, 24);

    if (test_getLogLevel() >= testLOGLEVEL_VERBOSE)
    {
        test_log(testLOGLEVEL_VERBOSE, "dumpClientState before messages are committed");
    }
    test_log_queue(testLOGLEVEL_VERBOSE, qName, 9, -1, "");

    // Now commit the 2nd transaction and the messages should be delivered
    ietr_commit(pThreadData, transactions[1], ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, hSession, NULL, NULL);

    // And validate that all 30 messages were received.
    TEST_ASSERT_EQUAL(ConsumerContext.MessagesReceived, 30);

    if (test_getLogLevel() >= testLOGLEVEL_VERBOSE)
    {
        test_log(testLOGLEVEL_VERBOSE, "dumpClientState after messages are committed");
    }
    test_log_queue(testLOGLEVEL_VERBOSE, qName, 9, -1, "");

    rc=ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    
    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

// This test validates the basic selection functionality
void test_2ConsumerSelection(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    uint32_t loop;
    uint32_t messageCount = 90;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer1;
    ismEngine_ConsumerHandle_t hConsumer2;
    char *ColourText[] = { "BLUE", "RED", "GREEN" };
    SelectionContext_t ConsumerContext1 = { NULL, NULL, 0, 0, false, true };
    SelectionContext_t *pConsumerContext1 = &ConsumerContext1;
    SelectionContext_t ConsumerContext2 = { NULL, NULL, 0, 0, false, true };
    SelectionContext_t *pConsumerContext2 = &ConsumerContext2;
    char payload[100];
    ism_prop_t *properties;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create the queue
    char *qName = "test_2ConsumerSelection";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server,
                          NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Set up a client that we'll use for all the puts/browsers
    rc = ism_engine_createClientState("2ConsumerSelectionClient",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL,
                                      NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_BROWSE_ONLY,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    // Now publish 90 messages round-robin the property value
    for (loop=0; loop < messageCount; loop++)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismMessageAreaType_t areaTypes[2];
        size_t areaLengths[2];
        void *areas[2];
        concat_alloc_t FlatProperties = { NULL };
        char localPropBuffer[1024];

        properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        ism_field_t ColourField = {VT_String, 0, {.s = ColourText[(loop/3) % 3] }};
        rc = ism_common_setProperty(properties, "Colour", &ColourField);
        TEST_ASSERT_EQUAL(rc, OK);

        FlatProperties.buf = localPropBuffer;
        FlatProperties.len = 1024;

        rc = ism_common_serializeProperties(properties, &FlatProperties);
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(payload, "Message - %d. Property is '%s'", loop,
                ColourText[(loop / 3) % 3]);

        areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
        areaLengths[0] = FlatProperties.used;
        areas[0] = FlatProperties.buf;
        areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[1] = strlen(payload) +1;
        areas[1] = (void *)payload;

        header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

        ismEngine_MessageHandle_t hMessage = NULL;
        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areas,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_QUEUE,
                                                qName,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        ism_common_freeProperties(properties);
    }

    // Having populated the queue with messages, now create 2 consumers with
    // selection strings. One consumer for blue messages and one consumer
    // for green messages. No-body want's red messages
    // Consumer1 - Colour = 'BLUE'
    properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);

    char *selectorString1 = "Colour='BLUE'";
    ism_field_t SelectorField1 = {VT_String, 0, {.s = selectorString1 }};
    rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField1);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now create the subscription
    pConsumerContext1->hSession = hSession;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   qName,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   &pConsumerContext1,
                                   sizeof(SelectionContext_t *),
                                   SelectionCallback,
                                   properties,
                                   ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION|ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And free the properties we allocated
    ism_common_freeProperties(properties);

    // Consumer2 - Colour = 'GREEN'
    properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);

    char *selectorString2 = "Colour='GREEN'";
    ism_field_t SelectorField2 = {VT_String, 0, {.s = selectorString2 }};
    rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField2);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now create the subscription
    pConsumerContext2->hSession = hSession;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   qName,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   &pConsumerContext2,
                                   sizeof(SelectionContext_t *),
                                   SelectionCallback,
                                   properties,
                                   ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION|ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And free the properties we allocated
    ism_common_freeProperties(properties);

    // And now start message delivery. When this call completes all of the
    // matching messages should have been delivered to the consumer.
    rc = ism_engine_startMessageDelivery(hSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Validate that 30 messages were received by each consumer.
    TEST_ASSERT_EQUAL(ConsumerContext1.MessagesReceived, 30);
    TEST_ASSERT_EQUAL(ConsumerContext2.MessagesReceived, 30);

    uint64_t failedSelectionCount;
    uint64_t succesfullSelectionCount;

    ism_engine_getSelectionStats( hConsumer1
                                , &failedSelectionCount
                                , &succesfullSelectionCount );

    test_log(testLOGLEVEL_TESTPROGRESS, "Consumer1: Number of successful selection attempts is %ld", succesfullSelectionCount);
    test_log(testLOGLEVEL_TESTPROGRESS, "Consumer1: Number of failed selection attempts is %ld", failedSelectionCount);

    ism_engine_getSelectionStats( hConsumer2
                                , &failedSelectionCount
                                , &succesfullSelectionCount );

    test_log(testLOGLEVEL_TESTPROGRESS, "Consumer2: Number of successful selection attempts is %ld", succesfullSelectionCount);
    test_log(testLOGLEVEL_TESTPROGRESS, "Consumer2: Number of failed selection attempts is %ld", failedSelectionCount);

    rc=ism_engine_destroyConsumer(hConsumer1, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc=ism_engine_destroyConsumer(hConsumer2, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

static void putComplete(int rc, void *handle, void *context)
{
	uint64_t *pMsgsPut = *(uint64_t **)context;
	__sync_fetch_and_add(pMsgsPut, 1);
}

typedef struct {
	uint64_t msgsArrived;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	bool consumerRunning;
} dAPFConsumerContext_t;

bool messageArrived( ismEngine_ConsumerHandle_t      hConsumer
        , ismEngine_DeliveryHandle_t      hDelivery
        , ismEngine_MessageHandle_t       hMessage
        , uint32_t                        deliveryId
        , ismMessageState_t               state
        , uint32_t                        destinationOptions
        , ismMessageHeader_t *            pMsgDetails
        , uint8_t                         areaCount
        , ismMessageAreaType_t            areaTypes[areaCount]
        , size_t                          areaLengths[areaCount]
        , void *                          pAreaData[areaCount]
        , void *                          pConsumerContext
        , ismEngine_DelivererContext_t *  _delivererContext)
{
	dAPFConsumerContext_t *pContext = *(dAPFConsumerContext_t **)pConsumerContext;
    bool wantMoreMessages;

	int osrc = pthread_mutex_lock(&(pContext->mutex));
	TEST_ASSERT_EQUAL(osrc, 0);

	pContext->msgsArrived++;
    ism_engine_releaseMessage(hMessage);

    //Consumer should be running (else why are we called?) - we'll turn it off
    TEST_ASSERT_EQUAL(pContext->consumerRunning, true);
    pContext->consumerRunning = false;
    wantMoreMessages = false;
    ism_engine_suspendMessageDelivery(hConsumer, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);

    //Broadcast that we've had a message and stopped deliver
    osrc = pthread_cond_broadcast(&(pContext->cond));
    TEST_ASSERT_EQUAL(osrc, 0);

    osrc = pthread_mutex_unlock(&(pContext->mutex));
    TEST_ASSERT_EQUAL(osrc, 0);

    return wantMoreMessages;
}

//Test we can get persistent messages from a durable sub without acking
void test_disableAckPersistentFlow(void)
{
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer=NULL;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    uint64_t numMessages=200;
    uint64_t messagesPut=0;
    uint64_t *pMessagesPut = &messagesPut;
    int32_t rc = OK;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    char *topicString = "test_disableAckPersistentFlow";
    char *subName    = "disableAckSub";

    test_createClientAndSession("NonAckingClient",
            NULL,
            ismENGINE_CREATE_CLIENT_OPTION_NONE,
            ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
            &hClient, &hSession, true);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_DURABLE;

    rc = sync_ism_engine_createSubscription(
            hClient,
            subName,
            NULL,
            ismDESTINATION_TOPIC,
            topicString,
            &subAttrs,
            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    for (int i = 0; i < numMessages; i++)
    {
        ismEngine_MessageHandle_t hMessage;

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_NONE, NULL,
                                &hMessage, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC, topicString,
                                                NULL,
                                                hMessage,
                                                &pMessagesPut, sizeof(pMessagesPut),
                                                putComplete);
        TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                          ("rc was %d\n", rc));

        if (rc == OK)
        {
            putComplete(rc, NULL, &pMessagesPut);
        }
    }

    while(messagesPut < numMessages)
    {
        usleep(500);
    }
    dAPFConsumerContext_t consumerContext = {0};
    dAPFConsumerContext_t *pconsumerContext = &consumerContext;

    consumerContext.consumerRunning = true; //starts in a running state
    int osrc=pthread_cond_init(&(consumerContext.cond), NULL);
    TEST_ASSERT_EQUAL(osrc, 0);
    osrc=pthread_mutex_init(&(consumerContext.mutex), NULL);
    TEST_ASSERT_EQUAL(osrc, 0);

    //Got all the messages... now consume them without acks
    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_SUBSCRIPTION
                                  , subName
                                  , NULL
                                  , NULL
                                  , &pconsumerContext
                                  , sizeof(pconsumerContext)
                                  , messageArrived
                                  , NULL
                                  , ismENGINE_CONSUMER_OPTION_NONE
                                  , &hConsumer
                                  , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

	osrc = pthread_mutex_lock(&(consumerContext.mutex));
	TEST_ASSERT_EQUAL(osrc, 0);

	while(consumerContext.msgsArrived < numMessages)
    {
    	if (consumerContext.consumerRunning == false)
    	{
    		consumerContext.consumerRunning = true;

    		//Unlock the mutex so we don't deadlock...
    		osrc = pthread_mutex_unlock(&(consumerContext.mutex));
    		TEST_ASSERT_EQUAL(osrc, 0);

    		rc = ism_engine_startMessageDelivery(hSession,
                                                 ismENGINE_START_DELIVERY_OPTION_NONE,
												 NULL, 0, NULL);
    		TEST_ASSERT_EQUAL(rc, OK);

    		osrc = pthread_mutex_lock(&(consumerContext.mutex));
    		TEST_ASSERT_EQUAL(osrc, 0);
    	}
    	else
    	{
    		osrc = pthread_cond_wait(&(consumerContext.cond), &(consumerContext.mutex));
    		TEST_ASSERT_EQUAL(osrc, 0);
    	}
    }

	osrc = pthread_mutex_unlock(&(consumerContext.mutex));
	TEST_ASSERT_EQUAL(osrc, 0);
    rc = sync_ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
}

// Test selection on deep queues
typedef struct putterContext_t
{
    // Input fields
    pthread_t hThread;
    uint32_t putterNum;
    uint32_t numMsgs;
    uint32_t msgProgress;
    char *qName;
    char Colour[20];
    bool Ready;
    pthread_mutex_t *pcontrolMutex;
    pthread_cond_t *pstartEvent;
    uint32_t uPause;
    bool failSelection;
    bool passSelection;

    // Output fields
    uint64_t totalTime;
    uint64_t quickestTime;
    uint64_t slowestTime;
    uint64_t avgTime;
    uint64_t selectionCount;
} putterContext_t;

int32_t selectionWrapper( ismMessageHeader_t * pMsgDetails
                        , uint8_t              areaCount
                        , ismMessageAreaType_t areaTypes[areaCount]
                        , size_t               areaLengths[areaCount]
                        , void *               pareaData[areaCount]
                        , const char *         topic
                        , const void *         pselectorRule
                        , size_t               selectorRuleLen
                        , ismMessageSelectionLockStrategy_t * lockStrategy )
{
    int32_t rc;

    putterContext_t *Context = pthread_getspecific(putterContextKey);
    if (Context != NULL)
    {
        Context->selectionCount++;
        if (Context->uPause > 0)
        {
            usleep(Context->uPause);
        }

        if (Context->failSelection)
            return SELECT_FALSE;
        if (Context->passSelection)
            return SELECT_TRUE;
    }

    rc = OriginalSelection(pMsgDetails, areaCount, areaTypes, areaLengths, pareaData, topic, pselectorRule, selectorRuleLen, lockStrategy);

    return rc;
}

putterContext_t *Producers = NULL;
uint32_t numProducers = 3;

void *putterThread(void *arg)
{
    putterContext_t *Context=(putterContext_t *)arg ;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    char clientId[64];
    struct timeval curTime;
    uint64_t diffTime, startTime, endTime, initialTime;
    uint32_t rc;
    char payload[100];
    char ThreadName[16];

    sprintf(clientId, "SelectionClient.Put.%d", Context->putterNum);

    sprintf(ThreadName, "Putter(%d)", Context->putterNum);
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&ThreadName);

    ism_engine_threadInit(0);

    Context->totalTime = 0;
    Context->quickestTime = (uint64_t)-1;
    Context->slowestTime = 0;
    Context->avgTime = 0;

    rc = pthread_setspecific(putterContextKey, Context);
    TEST_ASSERT_EQUAL(rc, OK);

    //Set up a client that we'll use for all the puts/browsers
    rc = ism_engine_createClientState(clientId,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL,
                                      NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    // Create a seesion for the putter
    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    // Signal that we are ready
    rc = pthread_mutex_lock(Context->pcontrolMutex);
    TEST_ASSERT_EQUAL(rc, OK);
    Context->Ready = true;

    rc = pthread_cond_wait(Context->pstartEvent, Context->pcontrolMutex);
    TEST_ASSERT_EQUAL(rc, OK);

    // Unlock the control mutex
    rc = pthread_mutex_unlock(Context->pcontrolMutex);
    TEST_ASSERT_EQUAL(rc, OK);

    test_log(testLOGLEVEL_TESTPROGRESS, "Producer starting putting '%s' messages", Context->Colour);

    // Take starting timestamp
    rc = gettimeofday(&curTime, NULL);
    TEST_ASSERT_EQUAL(rc, 0);
    initialTime = ((uint64_t)curTime.tv_sec * 1000000) + (uint64_t)curTime.tv_usec;
    endTime = initialTime;

    // Loop putting messages
    for (Context->msgProgress=0; Context->msgProgress < Context->numMsgs; Context->msgProgress++)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismMessageAreaType_t areaTypes[2];
        size_t areaLengths[2];
        void *areas[2];
        concat_alloc_t FlatProperties = { NULL };
        char localPropBuffer[1024];

        ism_prop_t *properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        ism_field_t ColourField = {VT_String, 0, {.s = Context->Colour}};
        rc = ism_common_setProperty(properties, "Colour", &ColourField);
        TEST_ASSERT_EQUAL(rc, OK);

        FlatProperties.buf = localPropBuffer;
        FlatProperties.len = 1024;

        rc = ism_common_serializeProperties(properties, &FlatProperties);
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(payload, "Message - %d. Property is '%s'", Context->msgProgress, Context->Colour);

        areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
        areaLengths[0] = FlatProperties.used;
        areas[0] = FlatProperties.buf;
        areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[1] = strlen(payload) +1;
        areas[1] = (void *)payload;

        header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

        ismEngine_MessageHandle_t hMessage = NULL;
        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areas,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        // Take starting timestamp
        rc = gettimeofday(&curTime, NULL);
        TEST_ASSERT_EQUAL(rc, 0);
        startTime = ((uint64_t)curTime.tv_sec * 1000000) + (uint64_t)curTime.tv_usec;

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_QUEUE,
                                                Context->qName,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // Take ending timestamp
        rc = gettimeofday(&curTime, NULL);
        TEST_ASSERT_EQUAL(rc, 0);
        endTime = ((uint64_t)curTime.tv_sec * 1000000) + (uint64_t)curTime.tv_usec;
        diffTime = endTime-startTime;

        Context->totalTime += diffTime;

        test_log(testLOGLEVEL_VERBOSE, "Producer '%s' message %d put took %lu microseconds", Context->Colour, Context->msgProgress, diffTime);
        if (Context->quickestTime > diffTime) Context->quickestTime = diffTime;
        if (Context->slowestTime < diffTime) Context->slowestTime = diffTime;

        ism_common_freeProperties(properties);
    }
    Context->avgTime = Context->totalTime / Context->numMsgs;

    // Take ending timestamp
    diffTime = endTime-initialTime;
    double ddftm = ((double)diffTime) / 1000000;

    test_log(testLOGLEVEL_TESTPROGRESS, "Producer thread ended. Took %f secs to put %d %s messages", ddftm, Context->numMsgs, Context->Colour);

    if (Producers != NULL)
    {
        for (uint32_t i=0; i < numProducers; i++)
        {
            test_log(testLOGLEVEL_TESTPROGRESS, "Producer %d messageCount %d - Selection attempts %ld", i, Producers[i].msgProgress, Producers[i].selectionCount);
        }
    }


    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);


    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    return NULL;
}

// This test validates how selection operates with a deep queue
void test_DeepSelection(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer;
    SelectionContext_t ConsumerContext = { NULL, NULL, 0, 0, false, true };
    SelectionContext_t *pConsumerContext = &ConsumerContext;
    ism_prop_t *properties;
    pthread_mutex_t controlMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t startEvent = PTHREAD_COND_INITIALIZER;
    uint32_t numMsgs = 250;
    putterContext_t myProducers[2];
    uint32_t producerNum;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create the queue
    char *qName = "test_2ConsumerSelection";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server,
                          NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Set up a client that we'll use for all the puts/browsers
    rc = ism_engine_createClientState("DeepSelectionClient",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL,
                                      NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    // Create a consumer which is searching for RED messages
    properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);

    char *selectorString1 = "Colour='RED'";
    ism_field_t SelectorField1 = {VT_String, 0, {.s = selectorString1 }};
    rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField1);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now create the subscription
    pConsumerContext->hSession = hSession;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   qName,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   &pConsumerContext,
                                   sizeof(SelectionContext_t *),
                                   SelectionCallback,
                                   properties,
                                   ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION|ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And free the properties we allocated
    ism_common_freeProperties(properties);

    rc = ism_engine_startMessageDelivery(hSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Lock the control mutex while we start the threads
    rc = pthread_mutex_lock(&controlMutex);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now create the threads which will put RED messages on the queue.
    memset(&myProducers[0], 0, sizeof(putterContext_t));

    myProducers[0].putterNum = 0;
    myProducers[0].numMsgs = numMsgs;
    myProducers[0].qName = qName;
    strcpy(myProducers[0].Colour, "RED");
    myProducers[0].Ready = false;
    myProducers[0].uPause = 5000;
    myProducers[0].pcontrolMutex = &controlMutex;
    myProducers[0].pstartEvent = &startEvent;

    rc = test_task_startThread(&myProducers[0].hThread,putterThread, (void *)&myProducers[0],"putterThread");
    TEST_ASSERT_EQUAL(rc, OK);

    // Now create the threads which will put BLUE messages on the queue.
    memset(&myProducers[1], 0, sizeof(putterContext_t));

    myProducers[1].putterNum = 1;
    myProducers[1].numMsgs = numMsgs;
    myProducers[1].qName = qName;
    strcpy(myProducers[1].Colour, "BLUE");
    myProducers[1].Ready = false;
    myProducers[1].uPause = 0;
    myProducers[1].pcontrolMutex = &controlMutex;
    myProducers[1].pstartEvent = &startEvent;

    rc = test_task_startThread(&myProducers[1].hThread,putterThread, (void *)&myProducers[1],"putterThread");
    TEST_ASSERT_EQUAL(rc, OK);

    // Ensure the threads are ready
    while ((myProducers[0].Ready == false) || (myProducers[1].Ready == false) )
    {
        rc = pthread_mutex_unlock(&controlMutex);
        TEST_ASSERT_EQUAL(rc, OK);

        (void)sched_yield();

        rc = pthread_mutex_lock(&controlMutex);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Start the threads
    rc = pthread_cond_broadcast(&startEvent);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = pthread_mutex_unlock(&controlMutex);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now wait for the threads to end, and print out the results
    for (producerNum = 0; producerNum < 2; producerNum++)
    {
        rc = pthread_join(myProducers[producerNum].hThread, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        test_log(testLOGLEVEL_CHECK, "Producer %d - %s - MIN(%lu) MAX(%lu) AVG(%lu)",
                 producerNum, myProducers[producerNum].Colour,
                 myProducers[producerNum].quickestTime,
                 myProducers[producerNum].slowestTime,
                 myProducers[producerNum].avgTime);
    }

    rc=ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

// This test validates how selection operates with a deep queue
void test_UnackdSelection(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSessionGreen=NULL;
    ismEngine_SessionHandle_t hSessionRed=NULL;
    ismEngine_SessionHandle_t hSessionPub=NULL;
    uint32_t messageCount = 10000;
    uint32_t i, loop;
    SelectionContext_t *pConsumerContext;
    uint32_t numRedConsumers = 2;
    SelectionContext_t *RedConsumers;
    uint32_t numGreenConsumers = 10;
    SelectionContext_t *GreenConsumers;
    ism_prop_t *properties;
    pthread_mutex_t controlMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t startEvent = PTHREAD_COND_INITIALIZER;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create the queue
    char *qName = "test_2ConsumerSelection";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server,
                          NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Set up a client that we'll use for all the puts/browsers
    rc = ism_engine_createClientState("UnackdSelectionClient",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL,
                                      NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionGreen,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSessionGreen);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionRed,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSessionRed);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionPub,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSessionPub);

    GreenConsumers = (SelectionContext_t *)calloc(sizeof(SelectionContext_t), numGreenConsumers);
    for (i=0; i < numGreenConsumers; i++)
    {
        properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        char *selectorString1 = "Colour='GREEN'";
        ism_field_t SelectorField1 = {VT_String, 0, {.s = selectorString1 }};
        rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField1);
        TEST_ASSERT_EQUAL(rc, OK);

        // Now create the subscription
        GreenConsumers[i].hSession = hSessionGreen;
        GreenConsumers[i].suspendOnNextMsg = true;
        GreenConsumers[i].consumeMsgs = false;

        pConsumerContext = &GreenConsumers[i];
        rc = ism_engine_createConsumer(hSessionGreen,
                                       ismDESTINATION_QUEUE,
                                       qName,
                                       ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                       NULL, // Unused for QUEUE
                                       &pConsumerContext,
                                       sizeof(SelectionContext_t *),
                                       SelectionCallback,
                                       properties,
                                       ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION|ismENGINE_CONSUMER_OPTION_ACK,
                                       &(GreenConsumers[i].hConsumer),
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        test_log(testLOGLEVEL_TESTPROGRESS, "Created (%s) Consumer number %d", selectorString1, i);

        // And free the properties we allocated
        ism_common_freeProperties(properties);
    }

    // Create a consumer which is searching for RED messages
    RedConsumers = (SelectionContext_t *)calloc(sizeof(SelectionContext_t), numRedConsumers);
    for (i=0; i < numRedConsumers; i++)
    {
        properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        char *selectorString2 = "Colour='RED'";
        ism_field_t SelectorField2 = {VT_String, 0, {.s = selectorString2 }};
        rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField2);
        TEST_ASSERT_EQUAL(rc, OK);

        // Now create the subscription
        RedConsumers[i].hSession = hSessionRed;
        RedConsumers[i].suspendOnNextMsg = false;
        RedConsumers[i].consumeMsgs = true;

        pConsumerContext = &RedConsumers[i];
        rc = ism_engine_createConsumer(hSessionRed,
                                       ismDESTINATION_QUEUE,
                                       qName,
                                       ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                       NULL, // Unused for QUEUE
                                       &RedConsumers[i],
                                       sizeof(SelectionContext_t *),
                                       SelectionCallback,
                                       properties,
                                       ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION|ismENGINE_CONSUMER_OPTION_ACK,
                                       &(RedConsumers[i].hConsumer),
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        test_log(testLOGLEVEL_TESTPROGRESS, "Created (%s) Consumer number %d", selectorString2, i);

        // And free the properties we allocated
        ism_common_freeProperties(properties);
    }

    // Now load the queue with a large number of messages (200,000)
    for (loop=0; loop < (messageCount * 2); loop++)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismMessageAreaType_t areaTypes[2];
        size_t areaLengths[2];
        void *areas[2];
        concat_alloc_t FlatProperties = { NULL };
        char localPropBuffer[1024];
        char payload[100];

        properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        if ((loop %0x01) == 0)
        {
            ism_field_t ColourField = {VT_String, 0, {.s = "ORANGE" }};
            rc = ism_common_setProperty(properties, "Colour", &ColourField);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            ism_field_t ColourField = {VT_String, 0, {.s = "GREEN" }};
            rc = ism_common_setProperty(properties, "Colour", &ColourField);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        FlatProperties.buf = localPropBuffer;
        FlatProperties.len = 1024;

        rc = ism_common_serializeProperties(properties, &FlatProperties);
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(payload, "Message - %d. Property is '%s'", loop, "GREEN");

        areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
        areaLengths[0] = FlatProperties.used;
        areas[0] = FlatProperties.buf;
        areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[1] = strlen(payload) +1;
        areas[1] = (void *)payload;

        header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

        ismEngine_MessageHandle_t hMessage = NULL;
        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areas,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = ism_engine_putMessageOnDestination(hSessionPub,
                                                ismDESTINATION_QUEUE,
                                                qName,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        ism_common_freeProperties(properties);
    }
    test_log(testLOGLEVEL_TESTPROGRESS, "Put %d GREEN and %d ORANGE  messages on the queue.", messageCount, messageCount);

#define START_GREEN_CONSUMER 1

#if START_GREEN_CONSUMER
    // Now start message delivery of the green consumers Session. They should only
    // receive one message before suspending delivery. Enabling this code highlights
    // an issue in the selection logic where an undelivered message followed by a
    // delivered but un-acknowledged message causes the checkWaiter function to
    // re-evaluate the selector for every message between the unack'd messages and
    // the end of the queue each time another message is put
    rc = ism_engine_startMessageDelivery(hSessionGreen,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_log(testLOGLEVEL_TESTPROGRESS, "Started message delivery for GREEN Consumer. Should suspend after first message");
#endif

    // Lock the control mutex while we start the thread
    rc = pthread_mutex_lock(&controlMutex);
    TEST_ASSERT_EQUAL(rc, OK);


    Producers = (putterContext_t *)calloc(sizeof(putterContext_t), numProducers);
    for (i=0; i < numProducers; i++)
    {
        Producers[i].putterNum = i;
        Producers[i].numMsgs = messageCount;
        Producers[i].qName = qName;
        strcpy(Producers[i].Colour, "GREEN");
        Producers[i].Ready = false;
        Producers[i].uPause = 0;
        Producers[i].pcontrolMutex = &controlMutex;
        Producers[i].pstartEvent = &startEvent;
        Producers[i].slowestTime = 0;

        rc = test_task_startThread(&Producers[i].hThread,putterThread, (void *)&Producers[i],"putterThread");
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Now start message delivery on Session2. There should be no messages
    // for this consumer, but a lot of messages to search. This call will not
    // complete until we have search all of the messages put so far
    test_log(testLOGLEVEL_TESTPROGRESS, "Started message delivery for RED Consumer. Shouldn't receive any messages");

    // Start a background thread to start publishing messages on the queue
    rc = ism_engine_startMessageDelivery(hSessionRed,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    test_log(testLOGLEVEL_TESTPROGRESS, "Red consumer start completed");


    // Wait for the threads to start
    bool ProducersReady;
    do
    {
        ProducersReady = true;

        for (i=0; (ProducersReady == true) && i < numProducers; i++)
        {
            if (!Producers[i].Ready) ProducersReady=false;
        }

        rc = pthread_mutex_unlock(&controlMutex);
        TEST_ASSERT_EQUAL(rc, OK);

        (void)sched_yield();

        rc = pthread_mutex_lock(&controlMutex);
        TEST_ASSERT_EQUAL(rc, OK);
    } while (ProducersReady == false);

    test_log(testLOGLEVEL_TESTPROGRESS, "Started producer threads for %d %s messages.", 5000, "GREEN");

    // Start the thread
    rc = pthread_cond_broadcast(&startEvent);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = pthread_mutex_unlock(&controlMutex);
    TEST_ASSERT_EQUAL(rc, OK);


    for (i=0; i < numProducers; i++)
    {
        rc = pthread_join(Producers[i].hThread, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    test_log(testLOGLEVEL_TESTPROGRESS, "producer threads ended.");

    // Now start message delivery on Session2. There should be no messages
    rc = ism_engine_stopMessageDelivery(hSessionRed,
                                        NULL, 0, NULL);

    // Now start message delivery on Session2. There should be no messages
    for (i=0; i < numRedConsumers; i++)
    {
        rc=ism_engine_destroyConsumer(RedConsumers[i].hConsumer, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    free(RedConsumers);

    for (i=0; i < numGreenConsumers; i++)
    {
        rc=ism_engine_destroyConsumer(GreenConsumers[i].hConsumer, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    free(GreenConsumers);

    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hSessionRed
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hSessionGreen
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hSessionPub
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

bool NoDeliveryCallback( ismEngine_ConsumerHandle_t      hConsumer
                       , ismEngine_DeliveryHandle_t      hDelivery
                       , ismEngine_MessageHandle_t       hMessage
                       , uint32_t                        deliveryId
                       , ismMessageState_t               state
                       , uint32_t                        destinationOptions
                       , ismMessageHeader_t *            pMsgDetails
                       , uint8_t                         areaCount
                       , ismMessageAreaType_t            areaTypes[areaCount]
                       , size_t                          areaLengths[areaCount]
                       , void *                          pAreaData[areaCount]
                       , void *                          pConsumerContext
                       , ismEngine_DelivererContext_t *  _delivererContext)
{
    int32_t rc;
    SelectionContext_t *pContext = *(SelectionContext_t **)pConsumerContext;

    // Check that the message we received matches the selector we expect
    // To be done
    TEST_ASSERT_EQUAL(areaCount, 2);

    // Increment the count of messages received
    pContext->MessagesReceived++;

    test_log(testLOGLEVEL_VERBOSE, "Received message '%s'", pAreaData[1]);

    if ((hDelivery != 0) && (pContext->consumeMsgs))
    {
        rc = ism_engine_confirmMessageDelivery( pContext->hSession
                                              , NULL
                                              , hDelivery
                                              , ismENGINE_CONFIRM_OPTION_CONSUMED
                                              , NULL
                                              , 0
                                              , NULL );
        TEST_ASSERT_EQUAL(rc, OK);
    }

    ism_engine_releaseMessage(hMessage);

    //We'd like more messages
    return pContext->suspendOnNextMsg?false:true;
}

// This test creates a consumer on a queue. The consumer does not
// acknowledge the messages as they are delivered but adds them to
// the list of messages to be ackd. When all of the messages have been
// received we then negatively acknoweldge all of the messages.
void test_NotDeliveredSelection(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    ismEngine_ClientStateHandle_t hClient=NULL;
    uint32_t numConsumers = 12;
    ismEngine_SessionHandle_t hPubSession;
    ismEngine_SessionHandle_t hSessions[numConsumers];
    uint32_t messageCount = 40000;
    uint32_t messagesReceived = 0;
    uint32_t i, j, loop;
    SelectionContext_t *pConsumerContext;
    SelectionContext_t *Consumers;
    ism_prop_t *properties;
    struct timeval curTime;
    uint64_t startTime, endTime;
    uint32_t numMsgColours=3;
    char *MsgColours[]= { "RED", "GREEN", "BLUE" };
    uint32_t numConsumerColours=3;
    char *ConsumerColours[]= { "RED", "GREEN", "BLUE" };
    uint32_t colourIndex;
    char selectorString[256];

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create the queue
    char *qName = "test_NotDeliveredSelection";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server,
                          NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Set up a client that we'll use for all the puts/browsers
    rc = ism_engine_createClientState("NotDeliveredSelectionClient",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL,
                                      NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hPubSession,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hPubSession);


    Consumers = (SelectionContext_t *)calloc(sizeof(SelectionContext_t), numConsumers);

    for (i=0; i < numConsumers; i++)
    {
        rc = ism_engine_createSession(hClient,
                                      ismENGINE_CREATE_SESSION_OPTION_NONE,
                                      &hSessions[i],
                                      NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hSessions[i]);

        properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        colourIndex = i % numConsumerColours;

        sprintf(selectorString, "Colour='%s'", ConsumerColours[colourIndex]);
        ism_field_t SelectorField={VT_String, 0, {.s = selectorString }};
        rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
        TEST_ASSERT_EQUAL(rc, OK);

        // Now create the subscription
        Consumers[i].hSession = hSessions[i];
        Consumers[i].suspendOnNextMsg = false;
        Consumers[i].consumeMsgs = false;
        Consumers[i].UnConsumedMsgList = (UnConsumedMsgList_t *)malloc(sizeof(UnConsumedMsgList_t) + (sizeof(ismEngine_DeliveryHandle_t) * 99));
        Consumers[i].UnConsumedMsgList->ListSize = 100;
        Consumers[i].UnConsumedMsgList->ListUsed = 0;
        Consumers[i].expectedColourIndex = colourIndex;

        pConsumerContext = &Consumers[i];
        rc = ism_engine_createConsumer(hSessions[i],
                                       ismDESTINATION_QUEUE,
                                       qName,
                                       ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                       NULL, // Unused for QUEUE
                                       &pConsumerContext,
                                       sizeof(SelectionContext_t *),
                                       SelectionCallback,
                                       properties,
                                       ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION|ismENGINE_CONSUMER_OPTION_ACK,
                                       &(Consumers[i].hConsumer),
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        test_log(testLOGLEVEL_TESTPROGRESS, "Created Consumer number %d", i);

        // And free the properties we allocated
        ism_common_freeProperties(properties);

        rc = ism_engine_startMessageDelivery(hSessions[i],
                                             ismENGINE_START_DELIVERY_OPTION_NONE,
                                             NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Take starting timestamp
    rc = gettimeofday(&curTime, NULL);
    TEST_ASSERT_EQUAL(rc, 0);
    startTime = ((uint64_t)curTime.tv_sec * 1000000) + (uint64_t)curTime.tv_usec;

    // Now load the queue with a large number of messages (40,000) all
    // which match the consumer(s) selection string
    for (loop=0; loop < messageCount; loop++)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismMessageAreaType_t areaTypes[2];
        size_t areaLengths[2];
        void *areas[2];
        concat_alloc_t FlatProperties = { NULL };
        char localPropBuffer[1024];
        char payload[100];

        properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        colourIndex = loop % numMsgColours;

        ism_field_t ColourField = {VT_String, 0, {.s = MsgColours[colourIndex] }};
        rc = ism_common_setProperty(properties, "Colour", &ColourField);
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(payload, "Message - %d. Property is '%s'", loop, MsgColours[colourIndex]);

        FlatProperties.buf = localPropBuffer;
        FlatProperties.len = 1024;

        rc = ism_common_serializeProperties(properties, &FlatProperties);
        TEST_ASSERT_EQUAL(rc, OK);


        areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
        areaLengths[0] = FlatProperties.used;
        areas[0] = FlatProperties.buf;
        areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[1] = strlen(payload) +1;
        areas[1] = (void *)payload;

        header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

        ismEngine_MessageHandle_t hMessage = NULL;
        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areas,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = ism_engine_putMessageOnDestination(hPubSession,
                                                ismDESTINATION_QUEUE,
                                                qName,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        ism_common_freeProperties(properties);
    }

    // Take ending timestamp
    rc = gettimeofday(&curTime, NULL);
    TEST_ASSERT_EQUAL(rc, 0);
    endTime = ((uint64_t)curTime.tv_sec * 1000000) + (uint64_t)curTime.tv_usec;

    test_log(testLOGLEVEL_TESTPROGRESS, "Put %d messages on the queue split over %d colours. Took %.02f seconds", messageCount, numMsgColours, (double)(endTime-startTime) / 1000000);



    test_log(testLOGLEVEL_TESTPROGRESS, "Starting message delivery for Consumer(s).");


    messagesReceived = 0;
    for (i=0; i < numConsumers; i++)
    {
        messagesReceived +=Consumers[i].MessagesReceived;
    }

    TEST_ASSERT_EQUAL(messageCount, messagesReceived);

    test_log(testLOGLEVEL_TESTPROGRESS, "%d messages succesfully received", messagesReceived);

    // Now nack all of the messages for the first session
    rc = ism_engine_stopMessageDelivery(hSessions[0],
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_log(testLOGLEVEL_TESTPROGRESS, "About to nack messages for first consumer");

    // Take starting timestamp
    rc = gettimeofday(&curTime, NULL);
    TEST_ASSERT_EQUAL(rc, 0);
    startTime = ((uint64_t)curTime.tv_sec * 1000000) + (uint64_t)curTime.tv_usec;

    // Now nack each message and see how long it takes to complete!
    for (j=0; j < Consumers[0].UnConsumedMsgList->ListUsed; j++)
    {
        rc = ism_engine_confirmMessageDelivery( hSessions[0]
                                              , NULL
                                              , Consumers[0].UnConsumedMsgList->hDelivery[j]
                                              , ismENGINE_CONFIRM_OPTION_NOT_DELIVERED
                                              , NULL
                                              , 0
                                              , NULL );
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Now nack each message and see how long it takes to complete!

    // Take initial timestamp
    rc = gettimeofday(&curTime, NULL);
    TEST_ASSERT_EQUAL(rc, 0);
    endTime = ((uint64_t)curTime.tv_sec * 1000000) + (uint64_t)curTime.tv_usec;

    test_log(testLOGLEVEL_TESTPROGRESS, "%d messages succesfully nack'd for consumer %d. Took(%0.2f)", Consumers[0].UnConsumedMsgList->ListUsed, 0, (double)(endTime-startTime) / 1000000);

    for (i=0; i < numConsumers; i++)
    {
        rc = gettimeofday(&curTime, NULL);
        TEST_ASSERT_EQUAL(rc, 0);
        startTime = ((uint64_t)curTime.tv_sec * 1000000) + (uint64_t)curTime.tv_usec;
        rc=ism_engine_destroyConsumer(Consumers[i].hConsumer, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_destroySession( hSessions[i]
                                      , NULL
                                      , 0
                                      , NULL );
        TEST_ASSERT_EQUAL(rc, OK);

        rc = gettimeofday(&curTime, NULL);
        TEST_ASSERT_EQUAL(rc, 0);
        endTime = ((uint64_t)curTime.tv_sec * 1000000) + (uint64_t)curTime.tv_usec;

        test_log(testLOGLEVEL_TESTPROGRESS, "Destroy consumer %d took %.02f seconds", i, (double)(endTime-startTime) / 1000000);
    }
    free(Consumers);

    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hPubSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

// This test validates more varieties of message selection
void test_MoreSelection(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    uint32_t loop;
    uint32_t messageCount = 8;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer;
    char *ColourText[] = { "BLUE", "YELLOW", "RED", "GREEN" };
    char *GetColourText[] = { "GREEN", "YELLOW", "RED", "BLUE" };
    SelectionContext_t ConsumerContext = { NULL, NULL, 0, 0, false, true };
    SelectionContext_t *pConsumerContext = &ConsumerContext;
    char payload[100];

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create the queue
    char *qName = "test_MoreSelection";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server,
                          NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Set up a client that we'll use for all the puts/browsers
    rc = ism_engine_createClientState("MoreSelectionClient",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL,
                                      NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    ismEngine_TransactionHandle_t hTran;
    rc = sync_ism_engine_createLocalTransaction(hSession, &hTran);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hTran);

    // Now publish messages and round-robin the property value
    for (loop=0; loop < messageCount; loop++)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismMessageAreaType_t areaTypes[2];
        size_t areaLengths[2];
        void *areas[2];
        concat_alloc_t FlatProperties = { NULL };
        char localPropBuffer[1024];

        ism_prop_t *properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        ism_field_t ColourField = {VT_String, 0, {.s = ColourText[loop%4] }};
        rc = ism_common_setProperty(properties, "Colour", &ColourField);
        TEST_ASSERT_EQUAL(rc, OK);

        FlatProperties.buf = localPropBuffer;
        FlatProperties.len = 1024;

        rc = ism_common_serializeProperties(properties, &FlatProperties);
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(payload, "Message - %d. Property is '%s'", loop,
                ColourText[loop % 4]);

        areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
        areaLengths[0] = FlatProperties.used;
        areas[0] = FlatProperties.buf;
        areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[1] = strlen(payload) +1;
        areas[1] = (void *)payload;

        header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

        ismEngine_MessageHandle_t hMessage = NULL;
        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areas,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_QUEUE,
                                                qName,
                                                // Put the yellow messages in a transaction
                                                (loop%4 == 1) ? hTran : NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        ism_common_freeProperties(properties);
    }

    // Having populated the queue with messages, now create a consumer with
    // a selection string for each of our colours (and one non-existent one)
    for(loop=0; loop<7; loop++)
    {
        ism_prop_t *properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        char selectorString[100];
        ism_field_t SelectorField = {VT_String, 0, {.s = selectorString }};
        sprintf(selectorString, "Colour='%s'", GetColourText[loop%4]);

        // Commit the Yellow messages on the loop after trying to select them the
        // first time.
        if (loop == 2)
        {
            rc = sync_ism_engine_commitTransaction(hSession, hTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
        TEST_ASSERT_EQUAL(rc, OK);

        // Now create the subscription
        pConsumerContext->hSession = hSession;
        pConsumerContext->MessagesReceived = 0;

        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_QUEUE,
                                       qName,
                                       ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                       NULL, // Unused for QUEUE
                                       &pConsumerContext,
                                       sizeof(SelectionContext_t *),
                                       SelectionCallback,
                                       properties,
                                       ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION|ismENGINE_CONSUMER_OPTION_ACK,
                                       &hConsumer,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // And free the properties we allocated
        ism_common_freeProperties(properties);

        // And now start message delivery. When this call completes all of the
        // matching messages should have been delivered to the consumer.
        rc = ism_engine_startMessageDelivery(hSession,
                                             ismENGINE_START_DELIVERY_OPTION_NONE,
                                             NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // Don't expect to see new messages if they are not committed or have
        // already been consumed
        if (loop == 1 || (loop > 3 && loop != 5))
        {
            TEST_ASSERT_EQUAL(ConsumerContext.MessagesReceived, 0);
        }
        else
        {
            // Validate that 2 messages were received.
            TEST_ASSERT_EQUAL(ConsumerContext.MessagesReceived, 2);
        }

        uint64_t failedSelectionCount;
        uint64_t succesfullSelectionCount;
        ism_engine_getSelectionStats( hConsumer
                                    , &failedSelectionCount
                                    , &succesfullSelectionCount );

        test_log(testLOGLEVEL_TESTPROGRESS, "Number of successful selection attempts is %ld", succesfullSelectionCount);
        test_log(testLOGLEVEL_TESTPROGRESS, "Number of failed selection attempts is %ld\n", failedSelectionCount);

        test_log_queue(testLOGLEVEL_VERBOSE, qName, 9, -1, "");

        rc=ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

void FullScanConsumerConsumedMsg(int rc, void *handle, void *context)
{
    volatile uint64_t *pMsgsConsumed = *(volatile uint64_t **)context;
    TEST_ASSERT_EQUAL(rc, OK);

    __sync_fetch_and_add(pMsgsConsumed, 1);
}

bool FullCleanPagesScanLockedCallback(
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
    uint64_t *pMsgsConsumed = *(uint64_t **)pConsumerContext;

    int32_t rc = ism_engine_confirmMessageDelivery( ((ismEngine_Consumer_t *)hConsumer)->pSession
                                                  , NULL
                                                  , hDelivery
                                                  , ismENGINE_CONFIRM_OPTION_CONSUMED
                                                  , &pMsgsConsumed, sizeof(pMsgsConsumed)
                                                  , FullScanConsumerConsumedMsg);
    TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                      ("rc was %d\n", rc));

    if (rc == OK)
    {
        FullScanConsumerConsumedMsg(rc, NULL, &pMsgsConsumed);
    }

    //Free our copy of the message
    ism_engine_releaseMessage(hMessage);

    return true;
}

uint64_t countMultiConsumerQPages(ismEngine_ConsumerHandle_t hConsumer)
{
    iemqQueue_t *Q = (iemqQueue_t *)(hConsumer->queueHandle);

    int os_rc = pthread_rwlock_rdlock(&(Q->headlock));
    TEST_ASSERT_EQUAL(os_rc, 0);

    iemqQNodePage_t *page = Q->headPage;
    uint64_t numPages = 0;

    while(page != NULL)
    {
        numPages++;
        page = page->next;
    }

    os_rc = pthread_rwlock_unlock(&(Q->headlock));
    TEST_ASSERT_EQUAL(os_rc, 0);

    return numPages;
}

//Check that if there is an early message locked in the lock manager,
//we clean up pages after it
void test_FullCleanPagesScanMsgLocked(void)
{
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer=NULL;
    char *subName="ForFullScan";
    char *topicString="/test/ForFullScan";
    int32_t rc = OK;

    uint64_t numTransactions=200;
    uint64_t msgsPerTran=60;
    ismEngine_TransactionHandle_t hTran[numTransactions];


    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    rc = test_createClientAndSession("FullScanClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE };

    rc = sync_ism_engine_createSubscription(
                hClient,
                subName,
                NULL,
                ismDESTINATION_TOPIC,
                topicString,
                &subAttrs,
                NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    //Put all messages in transactions (don't commit any yet)
    for (uint64_t tranNum = 0; tranNum < numTransactions; tranNum++)
    {
        rc = ietr_createLocal(pThreadData, NULL, true, false, NULL, &(hTran[tranNum]));
        TEST_ASSERT_EQUAL(rc, OK);

        test_incrementActionsRemaining(pActionsRemaining, msgsPerTran);
        for(uint64_t msgNum = 0; msgNum < msgsPerTran; msgNum++)
        {
            ismEngine_MessageHandle_t hMessage;

            rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                    ismMESSAGE_PERSISTENCE_PERSISTENT,
                                    ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                    ismMESSAGE_FLAGS_NONE,
                                    0,
                                    ismDESTINATION_TOPIC, topicString,
                                    &hMessage, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_engine_putMessageOnDestination(hSession,
                                                    ismDESTINATION_TOPIC,
                                                    topicString,
                                                    hTran[tranNum],
                                                    hMessage,
                                                    &pActionsRemaining,
                                                    sizeof(pActionsRemaining),
                                                    test_decrementActionsRemaining);
            if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        }

        test_waitForRemainingActions(pActionsRemaining);
    }

    //Now set up a consumer to consume any uncommitted messages
    volatile uint64_t msgsConsumed = 0;
    volatile uint64_t *pMsgsConsumed = &msgsConsumed;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   subName,
                                   NULL,
                                   NULL,
                                   &pMsgsConsumed,
                                   sizeof(pMsgsConsumed),
                                   FullCleanPagesScanLockedCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Now check our subscription has the right number of messages and count the pages on the queue
    uint64_t origNumPages = countMultiConsumerQPages(hConsumer);

    //Check the consumer can't consume any yet
    assert(msgsConsumed == 0);

    //commit all except first tran
    for (uint64_t tranNum = 1; tranNum < numTransactions; tranNum++)
    {
        ietr_commit(pThreadData, hTran[tranNum], ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
    }

    //Check that even though the first nodes on the queue (in the first tran) can't be garbage collected,
    //subsequent pages are
    test_waitForMessages64(pMsgsConsumed, NULL, (numTransactions - 1) * msgsPerTran, 20);

    uint64_t numSleeps = 0;
    uint64_t numPagesAfterFirstCommit;

    do
    {
        numPagesAfterFirstCommit = countMultiConsumerQPages(hConsumer);

        if(numPagesAfterFirstCommit >= origNumPages)
        {
            TEST_ASSERT_CUNIT(numSleeps < 16000,
                              ("Taking too long to garbage collect queue.\n", rc));
            usleep(1000);
            numSleeps++;
        }
    }
    while(numPagesAfterFirstCommit >= origNumPages);

    //commit the final tran....
    ietr_commit(pThreadData, hTran[0], ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);

    test_waitForMessages64(pMsgsConsumed, NULL, numTransactions * msgsPerTran, 20);

    //take another look at the queue
    numSleeps = 0;
    uint64_t numPagesAfterAllCommits;

    do
    {
        numPagesAfterAllCommits = countMultiConsumerQPages(hConsumer);

        if(numPagesAfterAllCommits >= numPagesAfterFirstCommit)
        {
            TEST_ASSERT_CUNIT(numSleeps < 8000,
                              ("Taking too long to garbage collect queue\n", rc));
            usleep(1000);
            numSleeps++;
        }
    }
    while(numPagesAfterAllCommits >= numPagesAfterFirstCommit);


    rc = sync_ism_engine_destroyConsumer(hConsumer);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySubscription(
                              hClient,
                              subName,
                              hClient,
                              NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient,
                                      hSession,
                                      false);
    TEST_ASSERT_EQUAL(rc, OK);
}

typedef struct tag_FullCleanPagesScanDlvdConsumer_t {
  volatile uint64_t               msgsArrived;
  volatile uint64_t               msgsConsumed;
  ismEngine_DeliveryHandle_t      hDeliveryStuck;
  ismEngine_MessageHandle_t       hMessageStuck;
} FullCleanPagesScanDlvdConsumer_t;

bool FullCleanPagesScanDlvrdCallback(
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
    FullCleanPagesScanDlvdConsumer_t *pContext = *(FullCleanPagesScanDlvdConsumer_t **)pConsumerContext;

    pContext->msgsArrived++;


    if (pContext->msgsArrived != 4)
    {
        volatile uint64_t *pMsgsConsumed = &(pContext->msgsConsumed);

        int32_t rc = ism_engine_confirmMessageDelivery( ((ismEngine_Consumer_t *)hConsumer)->pSession
                                                      , NULL
                                                      , hDelivery
                                                      , ismENGINE_CONFIRM_OPTION_CONSUMED
                                                      , &pMsgsConsumed, sizeof(pMsgsConsumed)
                                                      , FullScanConsumerConsumedMsg);
        TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                          ("rc was %d\n", rc));

        if (rc == OK)
        {
            FullScanConsumerConsumedMsg(rc, NULL, &pMsgsConsumed);
        }

        //Free our copy of the message
        ism_engine_releaseMessage(hMessage);
    }
    else
    {
        pContext->hDeliveryStuck = hDelivery;
        pContext->hMessageStuck  = hMessage;
    }

    return true;
}

//Check that if there is an early message in delivered but not
//acked state, we clean up pages after it.
void test_FullCleanPagesScanDlvd(void)
{
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer=NULL;
    char *subName="ForFullScanDlvd";
    char *topicString="/test/ForFullScanDlvd";
    int32_t rc = OK;

    uint64_t msgsToPut=12000;


    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    rc = test_createClientAndSession("FullScanClientDlvd",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE };

    rc = sync_ism_engine_createSubscription(
                hClient,
                subName,
                NULL,
                ismDESTINATION_TOPIC,
                topicString,
                &subAttrs,
                NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    //Now set up a consumer to consume messages apart for the 4th message (not yet started)

    FullCleanPagesScanDlvdConsumer_t consContext = { 0 };
    FullCleanPagesScanDlvdConsumer_t *pConsContext = &(consContext);

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   subName,
                                   NULL,
                                   NULL,
                                   &pConsContext,
                                   sizeof(pConsContext),
                                   FullCleanPagesScanDlvrdCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    //Put all messages
    test_incrementActionsRemaining(pActionsRemaining, msgsToPut);
    for (uint64_t msgNum = 0; msgNum < msgsToPut; msgNum++)
    {
        ismEngine_MessageHandle_t hMessage;

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
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
                                                &pActionsRemaining,
                                                sizeof(pActionsRemaining),
                                                test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }

    test_waitForRemainingActions(pActionsRemaining);

    //Now count the pages on the queue
    uint64_t origNumPages = countMultiConsumerQPages(hConsumer);

    rc = ism_engine_startMessageDelivery(hSession, ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Check that even though the first nodes on the queue (ignored/unacked by the callback) can't be garbage collected,
    //subsequent pages are
    test_waitForMessages64(&(consContext.msgsConsumed), NULL, msgsToPut - 1, 20);

    uint64_t numPages = countMultiConsumerQPages(hConsumer);

    TEST_ASSERT_CUNIT(numPages < origNumPages,
                      ("After all but one node consumed, numPages is %lu, originally %lu\n", numPages, origNumPages));

    //Ack the final message
    volatile uint64_t *pMsgsConsumed = &(consContext.msgsConsumed);

    rc = ism_engine_confirmMessageDelivery( ((ismEngine_Consumer_t *)hConsumer)->pSession
                                          , NULL
                                          , consContext.hDeliveryStuck
                                          , ismENGINE_CONFIRM_OPTION_CONSUMED
                                          , &pMsgsConsumed, sizeof(pMsgsConsumed)
                                          , FullScanConsumerConsumedMsg);
    TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                      ("rc was %d\n", rc));

    if (rc == OK)
    {
        FullScanConsumerConsumedMsg(rc, NULL, &pMsgsConsumed);
    }

    //Free our copy of the message
    ism_engine_releaseMessage(consContext.hMessageStuck);

    //Deliberately don't wait before the destroy, just to see what will happen

    rc = sync_ism_engine_destroyConsumer(hConsumer);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySubscription(
                              hClient,
                              subName,
                              hClient,
                              NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient,
                                      hSession,
                                      false);
    TEST_ASSERT_EQUAL(rc, OK);
}

//Checks whether oIds in a queue are missing (if so pages may have been
//discarded or not rehydrated after restart).
bool checkMultiConsumerPagesMissing(ismEngine_ConsumerHandle_t hConsumer)
{
    iemqQueue_t *Q = (iemqQueue_t *)(hConsumer->queueHandle);
    bool missingPages = false;

    int os_rc = pthread_rwlock_rdlock(&(Q->headlock));
    TEST_ASSERT_EQUAL(os_rc, 0);

    iemqQNodePage_t *page = Q->headPage;
    uint64_t lastOId = 0;

    while(page != NULL)
    {
        if ((lastOId != 0) && (page->nodes[0].orderId) != 0)
        {
            if( page->nodes[0].orderId != (lastOId+1))
            {
                //Gap between the end of the last page and the start of this one
                missingPages = true;
            }
        }
        //-2 below as we don't want the page guard node at the end of the page
        lastOId = page->nodes[page->nodesInPage -2].orderId;

        page = page->next;
    }

    os_rc = pthread_rwlock_unlock(&(Q->headlock));
    TEST_ASSERT_EQUAL(os_rc, 0);

    return missingPages;
}

bool FullCleanPagesScanDiscardCallback(
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
    FullCleanPagesScanDlvdConsumer_t *pContext = *(FullCleanPagesScanDlvdConsumer_t **)pConsumerContext;
    bool wantMoreMesssages = true;

    pContext->msgsArrived++;


    if (pContext->msgsArrived < 4)
    {
        volatile uint64_t *pMsgsConsumed = &(pContext->msgsConsumed);

        int32_t rc = ism_engine_confirmMessageDelivery( ((ismEngine_Consumer_t *)hConsumer)->pSession
                                                      , NULL
                                                      , hDelivery
                                                      , ismENGINE_CONFIRM_OPTION_CONSUMED
                                                      , &pMsgsConsumed, sizeof(pMsgsConsumed)
                                                      , FullScanConsumerConsumedMsg);
        TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                          ("rc was %d\n", rc));

        if (rc == OK)
        {
            FullScanConsumerConsumedMsg(rc, NULL, &pMsgsConsumed);
        }

        //Free our copy of the message
        ism_engine_releaseMessage(hMessage);
    }
    else
    {
        assert(pContext->hMessageStuck == NULL);
        pContext->hMessageStuck  = hMessage;
        pContext->hDeliveryStuck = hDelivery;

        //Don't want to deliver other messages we want them to be discarded
        wantMoreMesssages = false;
    }

    return wantMoreMesssages;
}
//Check that if there is an early message in delivered but not
//acked state, we clean up pages after it when messages are being discarded
void test_FullCleanPagesScanDiscard(void)
{
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer=NULL;
    char *subName="ForFullScanDiscard";
    char *topicString="/test/ForFullScanDiscard";
    int32_t rc = OK;

    uint64_t msgsToPut=12000;


    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    uint64_t             oldMaxMessageCount = iepi_getDefaultPolicyInfo(false)->maxMessageCount;
    iepiMaxMsgBehavior_t oldMaxMsgBehavior  = iepi_getDefaultPolicyInfo(false)->maxMsgBehavior;
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 500;
    iepi_getDefaultPolicyInfo(false)->maxMsgBehavior = DiscardOldMessages;

    rc = test_createClientAndSession("FullScanClientDiscard",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE };

    rc = sync_ism_engine_createSubscription(
                hClient,
                subName,
                NULL,
                ismDESTINATION_TOPIC,
                topicString,
                &subAttrs,
                NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    //Now set up a consumer to consume messages apart for the 4th message (not yet started)

    FullCleanPagesScanDlvdConsumer_t consContext = { 0 };
    FullCleanPagesScanDlvdConsumer_t *pConsContext = &(consContext);

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   subName,
                                   NULL,
                                   NULL,
                                   &pConsContext,
                                   sizeof(pConsContext),
                                   FullCleanPagesScanDiscardCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    //Check there are no missing pages yet
    TEST_ASSERT_EQUAL(checkMultiConsumerPagesMissing(hConsumer), false);
    //Put all messages
    test_incrementActionsRemaining(pActionsRemaining, msgsToPut);
    for (uint64_t msgNum = 0; msgNum < msgsToPut; msgNum++)
    {
        ismEngine_MessageHandle_t hMessage;

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
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
                                                &pActionsRemaining,
                                                sizeof(pActionsRemaining),
                                                test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }

    test_waitForRemainingActions(pActionsRemaining);

    //Check (by looking for non-sequential oIds between pages) that pages have been discarded
    uint64_t numSleeps=0;
    bool discardHappened = false;

    do
    {
       discardHappened = checkMultiConsumerPagesMissing(hConsumer);

       if (!discardHappened)
       {
           usleep(1000);
           numSleeps++;
       }
    }
    while (!discardHappened && numSleeps < 2000);

    TEST_ASSERT_EQUAL(discardHappened, true);

    //Ack the final message once we have it...need to have both the lower 64bits (msg) and upper 64bits (q) set
    numSleeps = 0;
    while ( numSleeps < 2000 &&
            (    (consContext.hDeliveryStuck & 0xFFFFFFFF) == 0
              || (consContext.hDeliveryStuck >> 64) == 0 ))
    {
        usleep(1000);
        numSleeps++;
    }
    if ((consContext.hDeliveryStuck & 0xFFFFFFFF) == 0)
    {
        printf("Low was 0:  Low: 0x%lx   High: 0x%lx\n",(uint64_t)(consContext.hDeliveryStuck & 0xFFFFFFFF),  (uint64_t)(consContext.hDeliveryStuck>>64));
    }
    if ((consContext.hDeliveryStuck & 0xFFFFFFFF00000000) == 0)
    {
        printf("Low was 0:  Low: 0x%lx   High: 0x%lx\n",(uint64_t)(consContext.hDeliveryStuck & 0xFFFFFFFF),  (uint64_t)(consContext.hDeliveryStuck>>64));
    }
    TEST_ASSERT_CUNIT((consContext.hDeliveryStuck & 0xFFFFFFFF),
                          ("hDeliveryStuck msg not set - can't ack\n"));
    TEST_ASSERT_CUNIT((consContext.hDeliveryStuck>>64 != 0),
                          ("hDeliveryStuck q not set - can't ack\n"));

    volatile uint64_t *pMsgsConsumed = &(consContext.msgsConsumed);

    rc = ism_engine_confirmMessageDelivery( ((ismEngine_Consumer_t *)hConsumer)->pSession
                                          , NULL
                                          , consContext.hDeliveryStuck
                                          , ismENGINE_CONFIRM_OPTION_CONSUMED
                                          , &pMsgsConsumed, sizeof(pMsgsConsumed)
                                          , FullScanConsumerConsumedMsg);
    TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                      ("rc was %d\n", rc));

    if (rc == OK)
    {
        FullScanConsumerConsumedMsg(rc, NULL, &pMsgsConsumed);
    }

    //Free our copy of the message
    ism_engine_releaseMessage(consContext.hMessageStuck);

    //Deliberately don't wait before the destroy, just to see what will happen

    rc = sync_ism_engine_destroyConsumer(hConsumer);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySubscription(
                              hClient,
                              subName,
                              hClient,
                              NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient,
                                      hSession,
                                      false);
    TEST_ASSERT_EQUAL(rc, OK);

    iepi_getDefaultPolicyInfo(false)->maxMessageCount = oldMaxMessageCount;
    iepi_getDefaultPolicyInfo(false)->maxMsgBehavior = oldMaxMsgBehavior;
}
/*********************************************************************/
/* TestSuite definition                                              */
/*********************************************************************/
CU_TestInfo ISM_Engine_CUnit_MultiConsumerQ_Basic[] = {
    { "DisableAckPersistentFlow", test_disableAckPersistentFlow },
    { "CommitOutOfOrder",   CommitOutOfOrder },
    { "NackOnSessionClose",  NackOnSessionClose },
    { "DurableSubReconnect", DurableSubReconnect },
    { "DrainQueue",          test_DrainQueue },
    { "TestBasicBrowse",     test_BasicBrowse },
    { "TestMultiBrowse",     test_MultiBrowse },
    { "TestBasicSelection",  test_BasicSelection },
    { "TestBasicSelectionCompiled",  test_BasicSelectionCompiled },
    { "TestSelectionLatePutCommit", test_SelectionLatePutCommit },
    { "TestBrowseSelection", test_BrowseSelection },
    { "Test2ConsumerSelection", test_2ConsumerSelection },
    { "TestDeepSelection", test_DeepSelection },
    { "TestUnackdSelection", test_UnackdSelection },
    { "TestNotDeliveredSelection", test_NotDeliveredSelection },
    { "TestMoreSelection", test_MoreSelection },
    { "TestFullCleanPagesScanMsgLocked", test_FullCleanPagesScanMsgLocked},
    { "TestFullCleanPagesScanDlvd", test_FullCleanPagesScanDlvd},
    { "TestFullCleanPagesScanDiscard", test_FullCleanPagesScanDiscard},
    CU_TEST_INFO_NULL
};

CU_SuiteInfo ISM_Engine_CUnit_MultiConsumerQ_Suites[] = {
    IMA_TEST_SUITE("MultiConsumerQBasic", initMultiConsumerQ, termMultiConsumerQ, ISM_Engine_CUnit_MultiConsumerQ_Basic),
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
            test_setLogLevel(level);
        }
    }

    if (argc >= 3)
    {
        if(strcmp(argv[2],"justWarmRestart") == 0 )
        {
            retval = test_processInit(trclvl, NULL);

            if (retval == OK)
            {
                retval = test_engineInit_WARM;

                if (retval == OK)
                {
                    retval = test_engineTerm(true);
                }

                int32_t rc = test_processTerm(retval == 0);
                if (retval == OK) retval = rc;
            }

            return retval;
        }
    }

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    retval = test_engineInit_DEFAULT;
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();
    CU_register_suites(ISM_Engine_CUnit_MultiConsumerQ_Suites);

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

    int32_t rc = test_engineTerm(true);
    if (retval == 0) retval = rc;

    rc = test_processTerm(retval == 0);
    if (retval == 0) retval = rc;

mod_exit:

    return retval;
}
