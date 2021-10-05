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
/* Module Name: test_nonDurablePersistSharedSubs.c                   */
/*                                                                   */
/* Description: Main source file for CUnit test of nondurable MQTT   */
/* clients connecting to persistent shared subs                      */
/*********************************************************************/
#include <math.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include "engineInternal.h"
#include "engineHashSet.h"
#include "exportCrypto.h"
#include "exportClientState.h"
#include "exportSubscription.h"

#include "test_utils_initterm.h"
#include "test_utils_sync.h"
#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_security.h"
#include "test_utils_client.h"
#include "test_utils_options.h"
#include "test_utils_file.h"
#include "test_utils_pubsuback.h"

#define MAX_ARGS 15


// 1) Create a shared sub with 2 durable clients (client0 & client1
// 2) Publish 18 messages to sub
// 3) Have  15 messages delivered to client0
//                    consume 2
//                    receive 2
//                    do nothing for 3
//                    after delivery stops nack-not-delivered 8
// 4) Have 5 messages delivered to client1 (should be 5 that have been given to client0 already)
//                       consume        1
//                       receive        1
//                       do nothing for 1
//                       after delivery stops nack-not-delivered 2
// Currently have 18-(7 sent and not nacked to client0 + 3 (client1)) = 8 available
// 5) Reconnect client0 to sub...  5 in flight already but they won't be redelivered as original session is not ended
//    Then have 6 more delivered to client1
//                       consume 1
//                       receive 1
//                       do nothing for 4
// 6) Have a new client (client2) come  and do nothing for last 2 available messages
//
//  After step 6:
//  4  consumed (3 by client0, 1 by client1)
//  4  received (3 by client0, 1 by client1)
//  10 inflight (7 by client0, 1 by client1, 2 by client2)
//  0 available

// 7) RESTART (checks we only get one MDR per message etc...)
// 8) Check a new client gets no messages
// 9) Check that client0 gets 10 messages delivered (of which 3 are received (2 in step 3+1 from step 5)
//               client1 gets 2 messages
//               client1 gets 2 messages
//

void test_capability_durable_BeforeRestart(void)
{
    #define NUM_DUR_CLIENTS 3
    ismEngine_ClientStateHandle_t hDurClient[NUM_DUR_CLIENTS];
    ismEngine_SessionHandle_t hDurClientSession[NUM_DUR_CLIENTS];
    char *DurClientIds[NUM_DUR_CLIENTS] = {"DurClient0", "DurClient1", "DurClient2"};

    int32_t rc;

    char *subName = "DurSub1";
    char *topicString = "DurTopic1";

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    //Setup the client that will own the shared namespace
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSharedOwningSession;

    rc = test_createClientAndSessionWithProtocol(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE,
                                                 PROTOCOL_ID_SHARED,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hSharedOwningClient, &hSharedOwningSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningSession);

    // 1) Create a shared sub with 3 durable clients (client0 & client1
    for (uint32_t i = 0 ; i < NUM_DUR_CLIENTS; i++)
    {
        rc = test_createClientAndSession(DurClientIds[i],
                NULL,
                ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                ismENGINE_CREATE_SESSION_OPTION_NONE,
                &(hDurClient[i]), &(hDurClientSession[i]), true);
        TEST_ASSERT_EQUAL(rc, OK);

        test_makeSub(
                 subName
               , topicString
               ,    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                  | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                  | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                  | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
               , hDurClient[i]
               , hSharedOwningClient);
    }

    // 2) Publish 18 messages to sub
    test_pubMessages(topicString,
                     ismMESSAGE_PERSISTENCE_PERSISTENT,
                     ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                     18);

    // 3) Have  15 messages delivered to client0
    //                    consume 2
    //                    receive 2
    //                    do nothing for 3
    //                    after delivery stops nack-not-delivered 8
    test_markMsgs( subName
                 , hDurClientSession[0]
                 , hSharedOwningClient
                 , 15 //Total messages we have delivered before disabling consumer
                 , 0 //Of the delivered messages, how many to Skip before ack
                 , 2 //Of the delivered messages, how many to consume
                 , 2 //Of the delivered messages, how many to mark received
                 , 3 //Of the delivered messages, how many to Skip before handles returned to us/nacked
                 , 0 //Of the delivered messages, how many to return to caller un(n)acked
                 , 0 //Of the delivered messages, how many to nack not received
                 , 8 //Of the delivered messages, how many to nack not sent
                 , NULL); //Handles of messages returned to us un(n)acked


    // 4) Have 5 messages delivered to client1 (should be 5 that have been given to client0 already)
    //                       consume        1
    //                       receive        1
    //                       do nothing for 1
    //                       after delivery stops nack-not-delivered 2
    test_markMsgs( subName
            , hDurClientSession[1]
            , hSharedOwningClient
            , 5 //Total messages we have delivered before disabling consumer
            , 0 //Of the delivered messages, how many to Skip before ack
            , 1 //Of the delivered messages, how many to consume
            , 1 //Of the delivered messages, how many to mark received
            , 1 //Of the delivered messages, how many to Skip before handles returned to us/nacked
            , 0 //Of the delivered messages, how many to return to caller un(n)acked
            , 0 //Of the delivered messages, how many to nack not received
            , 2 //Of the delivered messages, how many to nack not sent
            , NULL);//Handles of messages returned to us un(n)acked

    // Currently have 18-(7 sent and not nacked to client0 + 3 (client1)) = 8 available
    // 5) Reconnect client0 to sub...
    //    Then have 6 more delivered to client1 (5 inflight won't be redelivered as orig session not ended)
    //                       consume 1
    //                       receive 1
    //                       do nothing for 4
    test_markMsgs( subName
            , hDurClientSession[0]
            , hSharedOwningClient
            , 6 //Total messages we have delivered before disabling consumer
            , 0 //Of the delivered messages, how many to Skip before ack
            , 1 //Of the delivered messages, how many to consume
            , 1 //Of the delivered messages, how many to mark received
            , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
            , 0 //Of the delivered messages, how many to return to caller un(n)acked
            , 0 //Of the delivered messages, how many to nack not received
            , 0 //Of the delivered messages, how many to nack not sent
            , NULL);//Handles of messages returned to us un(n)acked

    // 6) Have a new client (client2) come  and do nothing for last 2 available messages
    test_markMsgs( subName
            , hDurClientSession[2]
            , hSharedOwningClient
            , 2 //Total messages we have delivered before disabling consumer
            , 0 //Of the delivered messages, how many to Skip before ack
            , 0 //Of the delivered messages, how many to consume
            , 0 //Of the delivered messages, how many to mark received
            , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
            , 0 //Of the delivered messages, how many to return to caller un(n)acked
            , 0 //Of the delivered messages, how many to nack not received
            , 0 //Of the delivered messages, how many to nack not sent
            , NULL);//Handles of messages returned to us un(n)acked
}

void test_capability_durable_PostRestart(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    char *subName = "DurSub1";

    //Setup the client that will own the shared namespace
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSharedOwningSession;
    int32_t rc;

    rc = test_createClientAndSessionWithProtocol(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE,
                                                 PROTOCOL_ID_SHARED,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hSharedOwningClient, &hSharedOwningSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningSession);

    // 8) Check a new client gets no messages

    testMsgsCounts_t recvdMsgDetails = {0};
    test_connectReuseReceive( "extraCheckingClient"
                           , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                           , hSharedOwningClient
                           , subName
                           , true
                           ,   ismENGINE_SUBSCRIPTION_OPTION_SHARED
                           | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                           | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                           , ismENGINE_CONSUMER_OPTION_ACK
                           , true
                           , 0
                           , 10
                           , &recvdMsgDetails);
    TEST_ASSERT_EQUAL(recvdMsgDetails.msgsArrived, 0); //msgs sent to the qos1 client that he didn't ack


    // 9) Check that client0 gets 10 messages delivered (of which 3 are received (2 in step 3+1 from step 5)
    //               client1 gets 2 messages
    //               client1 gets 2 messages
    testMsgsCounts_t msgDetails = {0};
    test_connectAndGetMsgs( "DurClient0"
                          , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                          , true
                          , false
                          , 0
                          , 10
                          , 5*60*1000 //5 mins in milliseconds
                          , &msgDetails);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived,         10);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd,     7);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd,     3);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);

    memset(&msgDetails, 0, sizeof(recvdMsgDetails));
    test_connectAndGetMsgs( "DurClient1"
                          , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                          , true
                          , false
                          , 0
                          , 2
                          , 5*60*1000 //5 mins in milliseconds
                          , &msgDetails);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived,         2);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd,     1);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd,     1);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);

    memset(&msgDetails, 0, sizeof(recvdMsgDetails));
    test_connectAndGetMsgs( "DurClient2"
                          , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                          , true
                          , false
                          , 0
                          , 2
                          , 5*60*1000 //5 mins in milliseconds
                          , &msgDetails);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived,         2);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd,     2);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd,     0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);


    test_log(testLOGLEVEL_TESTNAME, "Finished %s...\n", __func__);
}

//Check that if a batch of messages are only partially delivered
//After restart, the messages are available to others
//
// 1) Create a shared sub with 1 durable clients (client0
// 2) Publish 15 messages to sub
// 3) Have  3 messages delivered to client0
//                    do nothing for 3
// 4) RESTART
// 5) Check a new client gets 12 (15-3) messages
// 6) Check that client0 gets 3 messages
//
void test_capability_durableUndoDelivery_BeforeRestart(void)
{
    #define NUM_CLIENTS 1
    ismEngine_ClientStateHandle_t hClient[NUM_DUR_CLIENTS];
    ismEngine_SessionHandle_t hClientSession[NUM_DUR_CLIENTS];
    char *ClientIds[NUM_DUR_CLIENTS] = {"DurUndoDeliverClient0"};

    int32_t rc;

    char *subName = "DurUndoDeliverySub1";
    char *topicString = "DurUndoDeliverysTopic1";

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    //Setup the client that will own the shared namespace
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSharedOwningSession;

    rc = test_createClientAndSession("__Shared2",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hSharedOwningClient, &hSharedOwningSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningSession);

    // 1) Create a shared sub with 1 durable clients (client0)
    for (uint32_t i = 0 ; i < NUM_CLIENTS; i++)
    {
        rc = test_createClientAndSession(ClientIds[i],
                NULL,
                ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                ismENGINE_CREATE_SESSION_OPTION_NONE,
                &(hClient[i]), &(hClientSession[i]), true);
        TEST_ASSERT_EQUAL(rc, OK);

        test_makeSub( subName
                    , topicString
                    ,    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                       | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                       | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                       | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                    , hClient[i]
                    , hSharedOwningClient);
    }

    // 2) Publish 15 messages to sub
    test_pubMessages(topicString,
                    ismMESSAGE_PERSISTENCE_PERSISTENT,
                    ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                    15);

    test_markMsgs( subName
            , hClientSession[0]
            , hSharedOwningClient
            , 3 //Total messages we have delivered before disabling consumer
            , 0 //Of the delivered messages, how many to Skip before ack
            , 0 //Of the delivered messages, how many to consume
            , 0 //Of the delivered messages, how many to mark received
            , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
            , 0 //Of the delivered messages, how many to return to caller un(n)acked
            , 0 //Of the delivered messages, how many to nack not received
            , 0 //Of the delivered messages, how many to nack not sent
            , NULL);//Handles of messages returned to us un(n)acked

}

void test_capability_durableUndoDelivery_PostRestart(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    char *subName = "DurUndoDeliverySub1";

    //Setup the client that will own the shared namespace
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSharedOwningSession;
    int32_t rc;

    rc = test_createClientAndSession("__Shared2",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hSharedOwningClient, &hSharedOwningSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningSession);

    // 5) Check a new client gets 12 (15-3) messages
    testMsgsCounts_t recvdMsgDetails = {0};
    test_connectReuseReceive( "extraCheckingClient"
                       , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                       , hSharedOwningClient
                       , subName
                       , true
                       ,   ismENGINE_SUBSCRIPTION_OPTION_SHARED
                         | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                         | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                       , ismENGINE_CONSUMER_OPTION_ACK
                       , true
                       , 12
                       , 5*60*1000 //5 mins in milliseconds
                       , &recvdMsgDetails);
    TEST_ASSERT_EQUAL(recvdMsgDetails.msgsArrived, 12); //msgs sent to the qos1 client that he didn't ack



    // 6) Check that client0 gets 3 messages
    testMsgsCounts_t msgDetails = {0};
    test_connectAndGetMsgs( "DurUndoDeliverClient0"
                     , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                     , true
                     , false
                     , 0
                     , 3
                     , 5*60*1000 //5 mins in milliseconds
                     , &msgDetails);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived,         3);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd,     3);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd,     0);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);



    test_log(testLOGLEVEL_TESTNAME, "Finished %s...\n", __func__);
}


void UnsubNoRedelivery(bool durable)
{
    #define NUM_UNSUB_CLIENTS 2
    ismEngine_ClientStateHandle_t hClient[NUM_UNSUB_CLIENTS];
    ismEngine_SessionHandle_t hClientSession[NUM_UNSUB_CLIENTS];
    char *ClientIds[NUM_UNSUB_CLIENTS];
    char *subName;
    char *sharedClientId;
    int32_t rc;

    char *topicString = "UnSubTopic1";
    if (durable)
    {
        ClientIds[0]   = "UnsubDurClient0";
        ClientIds[1]   = "UnsubDurClient1";
        subName        = "UnsubDurSub";
        sharedClientId = "__SharedDurUnsub";
    }
    else
    {
        ClientIds[0]   = "UnsubNonDurClient0";
        ClientIds[1]   = "UnsubNonDurClient1";
        subName        = "UnsubDurSub";
        sharedClientId = "__SharedNonDurUnsub";
    }

    //Setup the client that will own the shared namespace
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSharedOwningSession;

    rc = test_createClientAndSession(sharedClientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hSharedOwningClient, &hSharedOwningSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningSession);

    // 1) Create a shared sub with 2  clients (0 & 1)
    for (uint32_t i = 0 ; i < NUM_UNSUB_CLIENTS; i++)
    {
        rc = test_createClientAndSession(ClientIds[i],
                NULL,
                durable ? ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                         :ismENGINE_CREATE_CLIENT_OPTION_NONE,
                ismENGINE_CREATE_SESSION_OPTION_NONE,
                &(hClient[i]), &(hClientSession[i]), true);
        TEST_ASSERT_EQUAL(rc, OK);

        test_makeSub( subName
               , topicString
               ,    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                  | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                  | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                  | (durable ? ismENGINE_SUBSCRIPTION_OPTION_DURABLE : 0)
               , hClient[i]
               , hSharedOwningClient);
    }

    // 2) Publish 2 messages to sub at qos 1 and qos 2
    test_pubMessages(topicString,
                     ismMESSAGE_PERSISTENCE_PERSISTENT,
                     ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                     1);

    test_pubMessages(topicString,
                     ismMESSAGE_PERSISTENCE_PERSISTENT,
                     ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                     1);

    //receive a few messages with client0
    uint32_t numMsgsToGet = 2;
    ismEngine_DeliveryHandle_t ackHandles[2];
    test_markMsgs( subName
            , hClientSession[0]
            , hSharedOwningClient
            , numMsgsToGet //Total messages we have delivered before disabling consumer
            , 0 //Of the delivered messages, how many to Skip before ack
            , 0 //Of the delivered messages, how many to consume
            , 0 //Of the delivered messages, how many to mark received
            , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
            , numMsgsToGet //Of the delivered messages, how many to return to caller un(n)acked
            , 0 //Of the delivered messages, how many to nack not received
            , 0 //Of the delivered messages, how many to nack not sent
            , ackHandles);//Handles of messages returned to us un(n)acked

    //unsub client0
    rc = ism_engine_destroySubscription(
            hClient[0],
            subName,
            hSharedOwningClient,
            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    testGetMsgContext_t ctxt = {0};
    testMsgsCounts_t msgDetails = {0};

    ctxt.hClient              = hClient[1];
    ctxt.hSession             = hClientSession[1];
    ctxt.pMsgCounts           = &msgDetails;
    ctxt.consumerOptions      = ismENGINE_CONSUMER_OPTION_ACK;
    ctxt.overrideConsumerOpts = true;
    ctxt.consumeMessages      = true;

    test_receive( &ctxt
                , subName
                , ismENGINE_SUBSCRIPTION_OPTION_SHARED
                , 0
                , 10000 );
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 0);

    rc = sync_ism_engine_destroyClientState(hClient[0],
                                           ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);

    //Now the low qos message should be available to the other client but the higher qos
    //message should be gone forever as it started delivery to a qos=2 client
    memset(&msgDetails, 0, sizeof(msgDetails));

    test_receive( &ctxt
                , subName
                , ismENGINE_SUBSCRIPTION_OPTION_SHARED
                , 1
                , 3000 );
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 1);


    rc = sync_ism_engine_destroyClientState(hClient[1],
                                           ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroyClientState(hSharedOwningClient,
                                            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_durableUnsubNoRedelivery(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    UnsubNoRedelivery(true);

    test_log(testLOGLEVEL_TESTNAME, "Finished %s...\n", __func__);
}

void test_capability_nondurableUnsubNoRedelivery(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    UnsubNoRedelivery(false);

    test_log(testLOGLEVEL_TESTNAME, "Finished %s...\n", __func__);
}

//We have mixed qos publishes in test_capability_durableUnsubNoRedelivery
//.... this test shows that the right thing happens after a restart
void test_capability_mixedQosPubs_BeforeRestart(void)
{
    #define NUM_MIXEDQOS_CLIENTS 3
    ismEngine_ClientStateHandle_t hClient[NUM_MIXEDQOS_CLIENTS];
    ismEngine_SessionHandle_t hClientSession[NUM_MIXEDQOS_CLIENTS];
    char *ClientIds[NUM_MIXEDQOS_CLIENTS];
    ismEngine_DeliveryHandle_t ackHandles[NUM_MIXEDQOS_CLIENTS][2];
    char *subName;
    char *sharedClientId;
    int32_t rc;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    char *topicString = "MixQosTopic1";
    ClientIds[0]   = "MixQosClient_QOS1";
    ClientIds[1]   = "MixQosClient_QOS2";
    ClientIds[2]   = "MixQosClient_DurableSleeper"; //Extra client that just ensures sub survives restart
    subName        = "/MixQosSub/MixQosTopic1";
    sharedClientId = ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED;

    //Setup the client that will own the shared namespace
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSharedOwningSession;

    rc = test_createClientAndSessionWithProtocol(sharedClientId,
                                                 PROTOCOL_ID_SHARED,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hSharedOwningClient, &hSharedOwningSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningSession);

    // 1) Create a shared sub with 2  clients (0 & 1)
    for (uint32_t i = 0 ; i < 2; i++)
    {
        rc = test_createClientAndSession(ClientIds[i],
                NULL,
                ismENGINE_CREATE_CLIENT_OPTION_NONE,
                ismENGINE_CREATE_SESSION_OPTION_NONE,
                &(hClient[i]), &(hClientSession[i]), true);
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


        //Send each client a qos 1 message and a qos 2 message that we leave unacked
        test_pubMessages(topicString,
                    ismMESSAGE_PERSISTENCE_PERSISTENT,
                    ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                    1);

        test_pubMessages(topicString,
                    ismMESSAGE_PERSISTENCE_PERSISTENT,
                    ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                    1);

        uint32_t numMsgsToGet = 2;

        test_markMsgs( subName
                , hClientSession[i]
                , hSharedOwningClient
                , numMsgsToGet //Total messages we have delivered before disabling consumer
                , 0 //Of the delivered messages, how many to Skip before ack
                , 0 //Of the delivered messages, how many to consume
                , 0 //Of the delivered messages, how many to mark received
                , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
                , numMsgsToGet //Of the delivered messages, how many to return to caller un(n)acked
                , 0 //Of the delivered messages, how many to nack not received
                , 0 //Of the delivered messages, how many to nack not sent
                , ackHandles[i]);//Handles of messages returned to us un(n)acked
    }

    //Add extra (durable) client to prove sub survives restart:
    rc = test_createClientAndSession(ClientIds[2],
            NULL,
            ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
            ismENGINE_CREATE_SESSION_OPTION_NONE,
            &(hClient[2]), &(hClientSession[2]), true);
    TEST_ASSERT_EQUAL(rc, OK);

    test_makeSub( subName
           , topicString
           ,    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
              | ismENGINE_SUBSCRIPTION_OPTION_SHARED
              | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
              | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
              | ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY
           , hClient[2]
           , hSharedOwningClient);

    test_log(testLOGLEVEL_TESTNAME, "Finished %s...\n", __func__);
}

void test_capability_mixedQosPubs_PostRestart(void)
{
    char *subName;
    char *sharedClientId;
    int32_t rc;

    subName        = "/MixQosSub/MixQosTopic1";
    sharedClientId = ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    //Setup the client that will own the shared namespace
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSharedOwningSession;

    rc = test_createClientAndSessionWithProtocol(sharedClientId,
                                                 PROTOCOL_ID_SHARED,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hSharedOwningClient, &hSharedOwningSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningSession);

    testMsgsCounts_t recvdMsgDetails = {0};
    test_connectReuseReceive( "extraCheckingClient"
                      , ismENGINE_CREATE_CLIENT_OPTION_NONE
                      , hSharedOwningClient
                      , subName
                      , true
                      , ismENGINE_SUBSCRIPTION_OPTION_SHARED
                       |ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                       |ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY
                      , ismENGINE_CONSUMER_OPTION_ACK
                      , true
                      , 3
                      , 10
                      , &recvdMsgDetails);

    TEST_ASSERT_EQUAL(recvdMsgDetails.msgsArrived, 3); //2 msgs sent to the qos1 client that he didn't ack
                                                       //+1 the qos1 message sent to the qos=2 client

    test_log(testLOGLEVEL_TESTNAME, "Finished %s...\n", __func__);
}


// 0) Create a durable MQTT client durClient1
// 1) Use durClient1 to create a shared sub that can have durable and non-durable MQTT clients attached to it
// 2) Connect nonDurableQos1Client[0] and nonDurableQos1Client[1] - two Qos 1 clients (the latter of which we'll examine post restart)
// 3) Connect nonDurableQos2Client[0] and nonDurableQos2Client[1] - two Qos 2 clients (the latter of which we'll examine post restart)
// 4) Publish 30 messages
// 4) Have 6 messages delivered to each of the 5 clients, have 3 messages acked (qos=1 clients consume. qos=2 clients recv then consume 1 recv 2)
// 5) Disconnect both nonDurableQos1Client[0] & nonDurableQos2Client[0] check qos1 messages are available for redelivery, qos2 disappear
// 6) RESTART
// 7) Check the messages for nonDurableQos1Client2 & nonDurableQos2Client2 (who were connected at restart) have qos1 available, qos2 disappeared

void test_capability_nondurableDurableMix_BeforeRestart(void)
{
    ismEngine_ClientStateHandle_t hDurClient1;
    ismEngine_SessionHandle_t hDurClient1Session;
    char *DurClientId = "MixDurClient1";

    #define NUM_NONDUR_QOS1_CLIENTS 2
    ismEngine_ClientStateHandle_t hNonDurQos1Client[NUM_NONDUR_QOS1_CLIENTS];
    ismEngine_SessionHandle_t hNonDurQos1ClientSession[NUM_NONDUR_QOS1_CLIENTS];
    char *NonDurQos1ClientIds[NUM_NONDUR_QOS1_CLIENTS] = {"MixNonDurQos1Client0", "MixNonDurQos1Client1"};

    #define NUM_NONDUR_QOS2_CLIENTS 2
    ismEngine_ClientStateHandle_t hNonDurQos2Client[NUM_NONDUR_QOS2_CLIENTS];
    ismEngine_SessionHandle_t hNonDurQos2ClientSession[NUM_NONDUR_QOS2_CLIENTS];
    char *NonDurQos2ClientIds[NUM_NONDUR_QOS2_CLIENTS] = {"MixNonDurQos2Client0", "MixNonDurQos2Client1"};

    int32_t rc;

    char *subName = "/MixSub1/MixTopic1";
    char *topicString = "MixTopic1";

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);


    //Setup the client that will own the shared namespace
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSharedOwningSession;

    rc = test_createClientAndSession(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED"2",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hSharedOwningClient, &hSharedOwningSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningSession);


    // 0) Create a durable MQTT client durClient1
    // 1) Use durClient1 to create a shared sub that can have durable and non-durable MQTT clients attached to it
    rc = test_createClientAndSession(DurClientId,
            NULL,
            ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
            ismENGINE_CREATE_SESSION_OPTION_NONE,
            &hDurClient1, &hDurClient1Session, true);
    TEST_ASSERT_EQUAL(rc, OK);

    test_makeSub( subName
           , topicString
           ,    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
              | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
              | ismENGINE_SUBSCRIPTION_OPTION_SHARED
              | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
              | ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY
           , hDurClient1
           , hSharedOwningClient);

    // 2) Connect nonDurableQos1Client[0] and nonDurableQos1Client[1] - two Qos 1 clients (the latter of which we'll examine post restart)
    for (uint32_t i = 0 ; i < NUM_NONDUR_QOS1_CLIENTS; i++)
    {
        rc = test_createClientAndSession(NonDurQos1ClientIds[i],
                NULL,
                ismENGINE_CREATE_CLIENT_OPTION_NONE,
                ismENGINE_CREATE_SESSION_OPTION_NONE,
                &(hNonDurQos1Client[i]), &(hNonDurQos1ClientSession[i]), true);
        TEST_ASSERT_EQUAL(rc, OK);

        test_makeSub( subName
               , topicString
               ,    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE
                  | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                  | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                  | ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY
               , hNonDurQos1Client[i]
               , hSharedOwningClient);
    }

    // 3) Connect nonDurableQos2Client[0] and nonDurableQos2Client[1] - two Qos 2 clients (the latter of which we'll examine post restart)
    for (uint32_t i = 0 ; i < NUM_NONDUR_QOS2_CLIENTS; i++)
    {
        rc = test_createClientAndSession(NonDurQos2ClientIds[i],
                NULL,
                ismENGINE_CREATE_CLIENT_OPTION_NONE,
                ismENGINE_CREATE_SESSION_OPTION_NONE,
                &(hNonDurQos2Client[i]), &(hNonDurQos2ClientSession[i]), true);
        TEST_ASSERT_EQUAL(rc, OK);

        test_makeSub( subName
               , topicString
               ,    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                  | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                  | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                  | ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY
               , hNonDurQos2Client[i]
               , hSharedOwningClient);
    }

    // 4) Publish 30 messages
    test_pubMessages(topicString,
                ismMESSAGE_PERSISTENCE_PERSISTENT,
                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                30);


    // 4) Have 6 messages delivered to each of the 5 clients, have 3 messages acked (qos=1 clients consume. qos=2 clients recv then consume 1 recv 2)
    test_markMsgs( subName
            , hDurClient1Session
            , hSharedOwningClient
            , 6 //Total messages we have delivered before disabling consumer
            , 0 //Of the delivered messages, how many to Skip before ack
            , 1 //Of the delivered messages, how many to consume
            , 2 //Of the delivered messages, how many to mark received
            , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
            , 0 //Of the delivered messages, how many to return to caller un(n)acked
            , 0 //Of the delivered messages, how many to nack not received
            , 0 //Of the delivered messages, how many to nack not sent
            , NULL);//Handles of messages returned to us un(n)acked

    for (uint32_t i = 0 ; i < NUM_NONDUR_QOS1_CLIENTS; i++)
    {
        test_markMsgs( subName
                , hNonDurQos1ClientSession[i]
                , hSharedOwningClient
                , 6 //Total messages we have delivered before disabling consumer
                , 0 //Of the delivered messages, how many to Skip before ack
                , 3 //Of the delivered messages, how many to consume
                , 0 //Of the delivered messages, how many to mark received
                , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
                , 0 //Of the delivered messages, how many to return to caller un(n)acked
                , 0 //Of the delivered messages, how many to nack not received
                , 0 //Of the delivered messages, how many to nack not sent
                , NULL);//Handles of messages returned to us un(n)acked
    }

    for (uint32_t i = 0 ; i < NUM_NONDUR_QOS2_CLIENTS; i++)
    {
        test_markMsgs( subName
                , hNonDurQos2ClientSession[i]
                , hSharedOwningClient
                , 6 //Total messages we have delivered before disabling consumer
                , 0 //Of the delivered messages, how many to Skip before ack
                , 1 //Of the delivered messages, how many to consume
                , 2 //Of the delivered messages, how many to mark received
                , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
                , 0 //Of the delivered messages, how many to return to caller un(n)acked
                , 0 //Of the delivered messages, how many to nack not received
                , 0 //Of the delivered messages, how many to nack not sent
                , NULL);//Handles of messages returned to us un(n)acked
    }


    //Check there are no available messages before we start disconnecting people...
    testMsgsCounts_t recvdMsgDetails = {0};

    test_connectReuseReceive( "extraCheckingClient"
                       , ismENGINE_CREATE_CLIENT_OPTION_NONE
                       , hSharedOwningClient
                       , subName
                       , true
                       , ismENGINE_SUBSCRIPTION_OPTION_SHARED
                        |ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                        |ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY
                       , ismENGINE_CONSUMER_OPTION_ACK
                       , true
                       , 0
                       , 10
                       , &recvdMsgDetails);
    TEST_ASSERT_EQUAL(recvdMsgDetails.msgsArrived, 0);


    // 5) Disconnect both nonDurableQos1Client[0] & nonDurableQos2Client[0] check qos1 messages are available for redelivery, qos2 disappear

   test_destroyClientAndSession(hNonDurQos1Client[0], hNonDurQos1ClientSession[0], false);

    memset(&recvdMsgDetails, 0, sizeof(recvdMsgDetails));

    test_connectReuseReceive( "extraCheckingClient"
                       , ismENGINE_CREATE_CLIENT_OPTION_NONE
                       , hSharedOwningClient
                       , subName
                       , true
                       , ismENGINE_SUBSCRIPTION_OPTION_SHARED
                        |ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                        |ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY
                       , ismENGINE_CONSUMER_OPTION_ACK
                       , true
                       , 3
                       , 10000
                       , &recvdMsgDetails);
    TEST_ASSERT_EQUAL(recvdMsgDetails.msgsArrived, 3); //msgs sent to the qos1 client that he didn't ack

    test_destroyClientAndSession(hNonDurQos2Client[0], hNonDurQos2ClientSession[0], false);

    memset(&recvdMsgDetails, 0, sizeof(recvdMsgDetails));
    test_connectReuseReceive( "extraCheckingClient"
                       , ismENGINE_CREATE_CLIENT_OPTION_NONE
                       , hSharedOwningClient
                       , subName
                       , true
                       , ismENGINE_SUBSCRIPTION_OPTION_SHARED
                        |ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                        |ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY
                       , ismENGINE_CONSUMER_OPTION_ACK
                       , true
                       , 0
                       , 10
                       , &recvdMsgDetails);
    TEST_ASSERT_EQUAL(recvdMsgDetails.msgsArrived, 0); //no msgs sent to the qos2 client should be redelivered


}

extern uint64_t numRehydratedConsumedNodes;

void test_capability_nonDurableDurableMix_PostRestart(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    char *subName = "/MixSub1/MixTopic1";

    //nonDurableQos2Client2 had 5 messages in flight to in during restart, those should not have
    //been put back on the queue....
    TEST_ASSERT_CUNIT(numRehydratedConsumedNodes >= 5,("numRehydratedConsumedNodes was %lu - expected at least 5", numRehydratedConsumedNodes));

    //Setup the client that will own the shared namespace
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSharedOwningSession;
    int32_t rc;

    rc = test_createClientAndSession(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED"2",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hSharedOwningClient, &hSharedOwningSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningSession);

    // 7) Check the messages for nonDurableQos1Client2 & nonDurableQos2Client2 (who were connected at restart) have qos1 available, qos2 disappeared
    //     (So there should be 3 unacked for the Qos1 client and none from Qos2
    testMsgsCounts_t recvdMsgDetails = {0};
    test_connectReuseReceive( "extraCheckingClient"
                       , ismENGINE_CREATE_CLIENT_OPTION_NONE
                       , hSharedOwningClient
                       , subName
                       , true
                       , ismENGINE_SUBSCRIPTION_OPTION_SHARED
                        |ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                        |ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY
                       , ismENGINE_CONSUMER_OPTION_ACK
                       , true
                       , 3
                       , 10
                       , &recvdMsgDetails);
    TEST_ASSERT_EQUAL(recvdMsgDetails.msgsArrived, 3); //msgs sent to the qos1 client that he didn't ack


    //Check the messages for the durable client are still there...
    testMsgsCounts_t msgDetails = {0};
    test_connectAndGetMsgs( "MixDurClient1"
                     , ismENGINE_CREATE_CLIENT_OPTION_DURABLE
                     , true
                     , false
                     , 0
                     , 5 //5 messages as we received 6 but consumed 1 before restart
                     , 5*60*1000 //5 mins in milliseconds
                     , &msgDetails);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived,         5);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedDlvd,     3);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedRcvd,     2);
    TEST_ASSERT_EQUAL(msgDetails.msgsArrivedConsumed, 0);

    test_log(testLOGLEVEL_TESTNAME, "Finished %s...\n", __func__);
}

// 0) Create two durable 'MQTT' clients 'DurSubReClient1' and 'DurSubReClient2'.
// 1) Create two non-durable 'MQTT' clients, 'NonDurSubReClient1' and 'NonDurSubReClient2'
// 2) Create a non-durable 'JMS' client 'GenericClient'
// 3) Create 4 mixed durability subs (all on the same topic 'SubReTopic'
//      one shared purely by durable clients '/MixedDurOnlySub/SubReTopic'
//      one shared purely by non-durable clients '/MixedNonDurOnlySub/SubReTopic'
//      one shared by both durable and non-durable clients '/MixedDurMixedSub/SubReTopic'
//      one shared only by non-durable clients but also has anonymous sharers 'MixedWithGenericSub'
//    (for the two used by non-durable, actually create them with the non-durable client to test that it
//     doesn't matter what the creator was).
// 4) Disconnect one of the two durable clients (not discarding)
// 5) RESTART
// 6) Check that
//      '/MixedDurOnlySub/SubReTopic' and '/MixedDurMixedSub/SubReTopic' exist and only have durable clients listed.
//      '/MixedNonDurOnlySub/SubReTopic' doesn't exist.
//      'MixedWithGenericSub' exists and has no sharing clients (but has anonymous sharers)
#define SUBRE_NUM_DUR_CLIENTS 2
#define SUBRE_NUM_NONDUR_CLIENTS 2
#define SUBRE_TOPIC_STRING "SubReTopic"
#define SUBRE_OWNING_CLIENTID "__SharedSubRe"
#define SUBRE_DURONLY_SUBNAME "/MixedDurOnlySub/SubReTopic"
#define SUBRE_NONDURONLY_SUBNAME "/MixedNonDurOnlySub/SubReTopic"
#define SUBRE_DURMIXED_SUBNAME "/MixedDurMixedSub/SubReTopic"
#define SUBRE_ALSOGENERIC_SUBNAME "MixedWithGenericSub"
void test_capability_subRehydration_BeforeRestart(void)
{
    ismEngine_ClientStateHandle_t hDurClient[SUBRE_NUM_DUR_CLIENTS];
    ismEngine_SessionHandle_t hDurSession[SUBRE_NUM_DUR_CLIENTS];
    char *DurClientId[SUBRE_NUM_DUR_CLIENTS] = {"DurSubReClient1", "DurSubReClient2"};

    ismEngine_ClientStateHandle_t hNonDurClient[SUBRE_NUM_NONDUR_CLIENTS];
    ismEngine_SessionHandle_t hNonDurSession[SUBRE_NUM_NONDUR_CLIENTS];
    char *NonDurClientIds[SUBRE_NUM_NONDUR_CLIENTS] = {"NonDurSubReClient1", "NonDurSubReClient2"};

    ismEngine_ClientStateHandle_t hGenericClient;
    ismEngine_SessionHandle_t hGenericSession;
    char *GenericClientId = "GenericClient";

    int32_t rc;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    //Setup the client that will own the shared namespace
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSharedOwningSession;

    rc = test_createClientAndSession(SUBRE_OWNING_CLIENTID,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hSharedOwningClient, &hSharedOwningSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningSession);

    // Create the durable clients
    for(uint32_t i=0; i<SUBRE_NUM_DUR_CLIENTS; i++)
    {
        rc = test_createClientAndSession(DurClientId[i],
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hDurClient[i], &hDurSession[i], false);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Create the non-durable clients
    for (uint32_t i = 0 ; i < SUBRE_NUM_NONDUR_CLIENTS; i++)
    {
        rc = test_createClientAndSession(NonDurClientIds[i],
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hNonDurClient[i], &hNonDurSession[i], false);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Create the generic (e.g. JMS) client
    rc = test_createClientAndSessionWithProtocol(GenericClientId,
                                                 PROTOCOL_ID_JMS,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hGenericClient, &hGenericSession, false);

    // Make the subscription intended to be shared only by durable clients
    test_makeSub(SUBRE_DURONLY_SUBNAME,
            SUBRE_TOPIC_STRING,
            ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
            ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
            ismENGINE_SUBSCRIPTION_OPTION_SHARED |
            ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY |
            ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
            hDurClient[0],
            hSharedOwningClient);

    // Make the subscription intended to be shared only by non-durable clients
    test_makeSub(SUBRE_NONDURONLY_SUBNAME,
            SUBRE_TOPIC_STRING,
            ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |
            ismENGINE_SUBSCRIPTION_OPTION_SHARED |
            ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY |
            ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
            hNonDurClient[0],
            hSharedOwningClient);

    // Make the subscription intended to be shared by a durable / nondurable mix
    test_makeSub(SUBRE_DURMIXED_SUBNAME,
            SUBRE_TOPIC_STRING,
            ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |
            ismENGINE_SUBSCRIPTION_OPTION_SHARED |
            ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY |
            ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
            hNonDurClient[1],
            hSharedOwningClient);

    // Make the subscription intended to be shared by non durables, but also marked generic
    test_makeSub(SUBRE_ALSOGENERIC_SUBNAME,
            SUBRE_TOPIC_STRING,
            ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |
            ismENGINE_SUBSCRIPTION_OPTION_SHARED |
            ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY |
            ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
            hNonDurClient[1],
            hSharedOwningClient);

    // Add in the other clients
    test_makeSub(SUBRE_DURONLY_SUBNAME,
            SUBRE_TOPIC_STRING,
            ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
            ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
            ismENGINE_SUBSCRIPTION_OPTION_SHARED |
            ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY |
            ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
            hDurClient[1],
            hSharedOwningClient);

    test_makeSub(SUBRE_NONDURONLY_SUBNAME,
            SUBRE_TOPIC_STRING,
            ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |
            ismENGINE_SUBSCRIPTION_OPTION_SHARED |
            ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY |
            ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
            hNonDurClient[1],
            hSharedOwningClient);

    for(uint32_t i=0; i<SUBRE_NUM_DUR_CLIENTS; i++)
    {
        test_makeSub(SUBRE_DURMIXED_SUBNAME,
                SUBRE_TOPIC_STRING,
                ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY |
                ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
                hDurClient[i],
                hSharedOwningClient);
    }

    for(uint32_t i=1; i<SUBRE_NUM_NONDUR_CLIENTS; i++)
    {
        test_makeSub(SUBRE_DURMIXED_SUBNAME,
                SUBRE_TOPIC_STRING,
                ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |
                ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY |
                ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT,
                hNonDurClient[i],
                hSharedOwningClient);
    }

    // Add in the generically shared client
    test_makeSub(SUBRE_ALSOGENERIC_SUBNAME,
            SUBRE_TOPIC_STRING,
            ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
            ismENGINE_SUBSCRIPTION_OPTION_SHARED,
            hGenericClient,
            hSharedOwningClient);

    // Disconnect one of the two durable clients
    test_destroyClientAndSession(hDurClient[0], hDurSession[0], false);
}

#define SUBRE_EXPECTED_SHAREDSUBOPTS (ismENGINE_SUBSCRIPTION_OPTION_SHARED | ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY)
void test_capability_subRehydration_PostRestart(void)
{
    int32_t rc;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    iettSharedSubData_t *sharedSubData;
    ismEngine_Subscription_t *subscription;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    // See if we can find the subscriptions we expect to still exist
    char *expectedSubs[] = {SUBRE_DURONLY_SUBNAME, SUBRE_DURMIXED_SUBNAME, SUBRE_ALSOGENERIC_SUBNAME};
    uint32_t expectedClientCount[] = {SUBRE_NUM_DUR_CLIENTS, SUBRE_NUM_DUR_CLIENTS, 0};
    for(uint32_t i=0; i<(uint32_t)(sizeof(expectedSubs)/sizeof(expectedSubs[0])); i++)
    {
        rc = iett_findClientSubscription(pThreadData,
                                         SUBRE_OWNING_CLIENTID,
                                         iett_generateClientIdHash(SUBRE_OWNING_CLIENTID),
                                         expectedSubs[i],
                                         iett_generateSubNameHash(expectedSubs[i]),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(subscription->subOptions & SUBRE_EXPECTED_SHAREDSUBOPTS, SUBRE_EXPECTED_SHAREDSUBOPTS);
        TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_PERSISTENT, iettSUBATTR_PERSISTENT);
        TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_GLOBALLY_SHARED, iettSUBATTR_GLOBALLY_SHARED);
        sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, expectedClientCount[i]);
        for(uint32_t x=0; x<sharedSubData->sharingClientCount; x++)
        {
            TEST_ASSERT_EQUAL(sharedSubData->sharingClients[x].clientId[0], 'D'); // 'Dur'
            TEST_ASSERT_EQUAL(sharedSubData->sharingClients[x].subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE,
                              ismENGINE_SUBSCRIPTION_OPTION_DURABLE);
        }
        if (expectedClientCount[i] == 0)
        {
            TEST_ASSERT_EQUAL(sharedSubData->anonymousSharers, iettANONYMOUS_SHARER_JMS_APPLICATION);
        }
        iett_releaseSubscription(pThreadData, subscription, true);
    }

    // Check that the one that only had non-durables attached is gone
    rc = iett_findClientSubscription(pThreadData,
                                     SUBRE_OWNING_CLIENTID,
                                     iett_generateClientIdHash(SUBRE_OWNING_CLIENTID),
                                     SUBRE_NONDURONLY_SUBNAME,
                                     iett_generateSubNameHash(SUBRE_NONDURONLY_SUBNAME),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
}

CU_TestInfo ISM_ExportResources_CUnit_test_capability_PreRestart[] =
{
    { "durableBeforeRestart",               test_capability_durable_BeforeRestart },
    { "durableUndoDelivery_BeforeRestart",  test_capability_durableUndoDelivery_BeforeRestart },
    { "durableUnsubNoRedelivery",           test_capability_durableUnsubNoRedelivery },
    { "nondurableUnsubNoRedelivery",        test_capability_nondurableUnsubNoRedelivery },
    { "mixedQosPubs_BeforeRestart",         test_capability_mixedQosPubs_BeforeRestart },
    { "nondurableDurableMixBeforeRestart",  test_capability_nondurableDurableMix_BeforeRestart },
    { "subRehydrationBeforeRestart",        test_capability_subRehydration_BeforeRestart },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_ExportResources_CUnit_test_capability_PostRestart[] =
{
    { "durablePostRestart",               test_capability_durable_PostRestart },
    { "durableUndoDelivery_PostRestart",  test_capability_durableUndoDelivery_PostRestart },
    { "mixedQosPubs_BeforeRestart",       test_capability_mixedQosPubs_PostRestart },
    { "nonDurableDurableMix_PostRestart", test_capability_nonDurableDurableMix_PostRestart },
    { "subRehydrationPostRestart",        test_capability_subRehydration_PostRestart },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initPhase0(void)
{
    return test_engineInit(true, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termPhase0(void)
{
   //Do nothing we want the engine running at restart
   return 0;
}

//For phases after phase 0
int initPhaseN(void)
{
    int rc= test_engineInit(false, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
    return rc;
}
//For phases after phase 0
int termPhaseN(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_ExportResources_CUnit_phase0suites[] =
{
    IMA_TEST_SUITE("PreRestart", initPhase0, termPhase0, ISM_ExportResources_CUnit_test_capability_PreRestart),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_ExportResources_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("AfterRestart1", initPhaseN, termPhaseN, ISM_ExportResources_CUnit_test_capability_PostRestart),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_ExportResources_CUnit_phase0suites
                              , ISM_ExportResources_CUnit_phase1suites };


int32_t parse_args( int argc
                  , char **argv
                  , uint32_t *pPhase
                  , uint32_t *pFinalPhase
                  , uint32_t *pTestLogLevel
                  , int32_t *pTraceLevel
                  , char **adminDir)
{
    int retval = 0;

    if (argc > 1)
    {
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
            else if (argv[i][0] == '-')
            {
                //All these args are of the form -<flag> <Value> so check we have a value:
                if ((i+1) >= argc)
                {
                    printf("Found a flag: %s but there was no following arg!\n", argv[i]);
                    retval = 96;
                    break;
                }

                if (!strcasecmp(argv[i], "-a"))
                {
                    //Admin dir...
                    i++;
                    *adminDir = argv[i];
                }
                else if (!strcasecmp(argv[i], "-p"))
                {
                    //start phase...
                    i++;
                    *pPhase = (uint32_t)atoi(argv[i]);

                }
                else if (!strcasecmp(argv[i], "-f"))
                {
                    //final phase...
                    i++;
                    *pFinalPhase = (uint32_t)atoi(argv[i]);
                }
                else if (!strcasecmp(argv[i], "-v"))
                {
                    //final phase...
                    i++;
                    *pTestLogLevel= (uint32_t)atoi(argv[i]);
                }
                else if (!strcasecmp(argv[i], "-t"))
                {
                    i++;
                    *pTraceLevel= (uint32_t)atoi(argv[i]);
                }
                else
                {
                    printf("Unknown flag arg: %s\n", argv[i]);
                    retval = 95;
                    break;
                }
            }
            else
            {
                printf("Unknown arg: %s\n", argv[i]);
                retval = 94;
                break;
            }
        }
    }
    return retval;
}

void setupArgsForNextPhase(int argc, char *argv[], char *nextPhaseText, char *adminDir, char *NextPhaseArgv[])
{
    bool phasefound=false, adminDirfound=false;
    uint32_t i;
    TEST_ASSERT(argc < MAX_ARGS-5, ("argc was %d", argc));

    for (i=0; i < argc; i++)
    {
        NextPhaseArgv[i]=argv[i];
        if (strcmp(NextPhaseArgv[i], "-p") == 0)
        {
            NextPhaseArgv[i+1]=nextPhaseText;
            i++;
            phasefound=true;
        }
        if (strcmp(NextPhaseArgv[i], "-a") == 0)
        {
            NextPhaseArgv[i+1]=adminDir;
            i++;
            adminDirfound=true;
        }
    }

    if (!phasefound)
    {
        NextPhaseArgv[i++]="-p";
        NextPhaseArgv[i++]=nextPhaseText;
    }

    if (!adminDirfound)
    {
        NextPhaseArgv[i++]="-a";
        NextPhaseArgv[i++]=adminDir;
    }

    NextPhaseArgv[i]=NULL;

}

int main(int argc, char *argv[])
{
    int32_t trclvl = 0;
    uint32_t testLogLevel = testLOGLEVEL_TESTDESC;
    int retval = 0;
    char *adminDir=NULL;
    uint32_t phase=0;
    uint32_t finalPhase=1;

    ism_time_t seedVal = ism_common_currentTimeNanos();

    srand(seedVal);

    CU_initialize_registry();

    retval = parse_args( argc
                       , argv
                       , &phase
                       , &finalPhase
                       , &testLogLevel
                       , &trclvl
                       , &adminDir);
    if (retval != OK) goto mod_exit;

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, adminDir);

    char localAdminDir[1024];
    if (adminDir == NULL && test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir)) == true)
    {
        adminDir = localAdminDir;
    }

    if (retval == 0)
    {
        CU_register_suites(PhaseSuites[phase]);

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

    if (phase < finalPhase)
    {
        test_log(2, "Restarting for Phase %u...", phase+1);
        char *nextPhaseArgv[MAX_ARGS];
        char nextPhaseText[16];
        sprintf(nextPhaseText, "%d", phase+1);
        setupArgsForNextPhase(argc, argv, nextPhaseText, adminDir, nextPhaseArgv);
        retval = test_execv(nextPhaseArgv[0], nextPhaseArgv);
        TEST_ASSERT(false, ("'test_execv' failed. rc = %d", retval));
    }
    else
    {
        test_log(2, "Success. Test complete!\n");
        test_processTerm(false);
        test_removeAdminDir(adminDir);
    }

mod_exit:

    return retval;
}
