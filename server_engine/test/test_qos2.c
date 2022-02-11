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
/* Module Name: test_qos2.c                                          */
/*                                                                   */
/* Description: CUnit tests of Engine MQTT QoS2-style functions      */
/*                                                                   */
/*********************************************************************/
#include "test_qos2.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

CU_TestInfo ISM_Engine_CUnit_QoS2Basic[] =
{
    { "QoS2Reconnect",      qos2TestReconnect  },
    { "QoS2TestUnreleased", qos2TestUnreleased },
    { "QoS2NoSubs",         qos2TestNoSubs },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_Engine_CUnit_QoS2LowIDRange[] =
{
    { "QoS2Reconnect",      qos2TestReconnect  },
    { "QoS2TestUnreleased", qos2TestUnreleased },
    { "QoS2NoSubs",         qos2TestNoSubs },
    CU_TEST_INFO_NULL
};

typedef struct callbackContext
{
    volatile uint32_t               MessageCount;
    ismEngine_ClientStateHandle_t   hClient;
    ismEngine_SessionHandle_t       hSession;
    ismEngine_ConsumerHandle_t      hConsumer;
} callbackContext_t;

static uint64_t msgsAcked = 0;

#define CALLBACKCONTEXT_DEFAULT { 0, NULL, NULL, NULL };

static bool deliveryCallbackBeforeReconnect(
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
    ismEngine_DelivererContext_t *  _delivererContext);

static bool deliveryCallbackAfterReconnect(
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
    ismEngine_DelivererContext_t *  _delivererContext);

static bool deliveryCallbackAfterRepeatedReconnect(
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
    ismEngine_DelivererContext_t *  _delivererContext);

static void unreleasedCallback(
    uint32_t                        deliveryId,
    ismEngine_UnreleasedHandle_t    hUnrel,
    void *                          pContext);


static void ackCompleted(int32_t rc, void *handle, void *context)
{
    __sync_add_and_fetch(&msgsAcked, 1);
}

/*
 * Very simple reconnect test to validate delivery following reconnection.
 * Send two messages delivered to a durable sub. Confirm just one as received.
 * Reconnect and validate the message states. Confirm them both as consumed.
 * Repeat reconnect and validate nothing is delivered.
 */
void qos2TestReconnect(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_MessageHandle_t hMessage;
    int32_t rc = OK;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    rc = ism_engine_createClientState("qos2TestReconnect_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClientPub,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_createClientState("ClientReconnect",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClientSub1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

    rc = sync_ism_engine_createSubscription(hClientSub1,
                                            "Sub1",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "Topic/#",
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSessionSub1,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    msgsAcked = 0;

    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "Sub1",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallbackBeforeReconnect,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    header.Reliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;
    ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
    size_t areaLengths[2] = {0, 13};
    void *areaData[2] = {NULL, "Message body"};

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSessionPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &hProducerPub,
                                   NULL,
                                   0,
                                   NULL);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 0);

    rc = sync_ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 1);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage);
    TEST_ASSERT_EQUAL(rc, OK);


    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 2);

    rc = ism_engine_stopMessageDelivery(hSessionSub1,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumerSub1,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_CUNIT(((rc == OK)||(rc == ISMRC_AsyncCompletion)),
                      ("rc was %d", rc));

    rc = sync_ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    while(msgsAcked == 0)
    {
        sched_yield();
    }

    rc = sync_ism_engine_createClientState("ClientReconnect",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClientSub1);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSessionSub1,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;

    msgsAcked = 0;

    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "Sub1",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallbackAfterReconnect,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_waitForMessages(&(callbackContext.MessageCount), NULL, 2, 20);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 2);

    rc = ism_engine_stopMessageDelivery(hSessionSub1,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumerSub1,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_CUNIT(((rc == OK)||(rc == ISMRC_AsyncCompletion)),
                      ("rc was %d",rc));

    rc = ism_engine_destroySession(hSessionSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_CUNIT(((rc == OK)||(rc == ISMRC_AsyncCompletion)),
                      ("rc was %d",rc));

    rc = sync_ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    while(msgsAcked < 2)
    {
        sched_yield();
    }

    rc = sync_ism_engine_createClientState("ClientReconnect",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClientSub1);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSessionSub1,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;

    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "Sub1",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallbackAfterRepeatedReconnect,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 0);

    rc = ism_engine_stopMessageDelivery(hSessionSub1,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumerSub1,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_CUNIT(((rc == OK)||(rc == ISMRC_AsyncCompletion)),
                      ("rc was %d", rc));

    rc = sync_ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyProducer(hProducerPub,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionPub,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientPub,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}



/*
 * YOU NEED TO PUT SOME TEXT HERE
 *
 *
 * Very simple reconnect test to validate delivery following reconnection.
 * Send two messages delivered to a durable sub. Confirm just one as received.
 * Reconnect and validate the message states. Confirm them both as consumed.
 * Repeat reconnect and validate nothing is delivered.
 */
void qos2TestUnreleased(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_TransactionHandle_t hTranPub;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_UnreleasedHandle_t hUnrel;
    int32_t rc = OK;

    rc = ism_engine_createClientState("qos2TestUnreleased_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClientPub,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_createClientState("ClientReconnect",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClientSub1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

    rc = sync_ism_engine_createSubscription(hClientSub1,
                                            "Sub1",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "Topic/#",
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSessionSub1,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    msgsAcked = 0;

    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "Sub1",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallbackBeforeReconnect,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    header.Reliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;
    ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
    size_t areaLengths[2] = {0, 13};
    void *areaData[2] = {NULL, "Message body"};

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSessionPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &hProducerPub,
                                   NULL,
                                   0,
                                   NULL);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 0);

    rc = sync_ism_engine_putMessageWithDeliveryId(hSessionPub,
                                             hProducerPub,
                                             NULL,
                                             hMessage,
                                             1,
                                             &hUnrel);
    TEST_ASSERT_EQUAL(rc, OK)
    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 1);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageWithDeliveryId(hSessionPub,
                                             hProducerPub,
                                             NULL,
                                             hMessage,
                                             2,
                                             &hUnrel);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_createLocalTransaction(hSessionPub,
                                                &hTranPub);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_removeUnreleasedDeliveryId(hSessionPub,
                                               hTranPub,
                                               hUnrel,
                                               NULL,
                                               0,
                                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_commitTransaction(hSessionPub,
                                      hTranPub,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 2);

    rc = ism_engine_stopMessageDelivery(hSessionSub1,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumerSub1,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_CUNIT(((rc == OK)||(rc == ISMRC_AsyncCompletion)),
                      ("rc was %d", rc));

    rc = sync_ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    while (msgsAcked == 0)
    {
        sched_yield();
    }

    rc = sync_ism_engine_createClientState("ClientReconnect",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClientSub1);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSessionSub1,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;

    msgsAcked = 0;

    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "Sub1",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallbackAfterReconnect,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_waitForMessages(&(callbackContext.MessageCount), NULL, 2, 20);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 2);

    rc = ism_engine_stopMessageDelivery(hSessionSub1,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumerSub1,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_CUNIT(((rc == OK)||(rc == ISMRC_AsyncCompletion)),
                      ("rc was %d", rc));

    rc = ism_engine_destroySession(hSessionSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_CUNIT(((rc == OK)||(rc == ISMRC_AsyncCompletion)),
                      ("rc was %d", rc));

    rc = sync_ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    while (msgsAcked < 2)
    {
        sched_yield();
    }

    rc = sync_ism_engine_createClientState("ClientReconnect",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClientSub1);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSessionSub1,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;

    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "Sub1",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallbackAfterRepeatedReconnect,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 0);

    rc = ism_engine_stopMessageDelivery(hSessionSub1,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumerSub1,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_CUNIT(((rc == OK)||(rc == ISMRC_AsyncCompletion)),
                      ("rc was %d", rc));

    rc = sync_ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyProducer(hProducerPub,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionPub,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext.hClient = hClientPub;
    callbackContext.hSession = NULL;
    callbackContext.hConsumer = NULL;;
    callbackContext.MessageCount = 0;

    rc = ism_engine_listUnreleasedDeliveryIds(hClientPub,
                                              &callbackContext,
                                              unreleasedCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 1);

    rc = ism_engine_destroyClientState(hClientPub,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}



/*
 * Message-delivery callback before reconnection
 */
static bool deliveryCallbackBeforeReconnect(
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
    bool moreMessagesPlease = true;
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t rc = OK;

    // We got a message!
    pCallbackContext->MessageCount++;

    ism_engine_releaseMessage(hMessage);

    if (pCallbackContext->MessageCount == 1)
    {
        rc = ism_engine_confirmMessageDelivery(pCallbackContext->hSession,
                                               NULL,
                                               hDelivery,
                                               ismENGINE_CONFIRM_OPTION_RECEIVED,
                                               NULL, 0,
                                               ackCompleted);

        TEST_ASSERT_CUNIT(((rc == OK)||(rc == ISMRC_AsyncCompletion)),
                          ("Unexpected rc: %d", rc));

        if (rc == OK)
        {
            ackCompleted(rc, NULL, NULL);
        }
    }

    return moreMessagesPlease;
}


/*
 * Message-delivery callback after reconnection
 */
static bool deliveryCallbackAfterReconnect(
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
    bool moreMessagesPlease = true;
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t rc = OK;

    // We got a message!
    pCallbackContext->MessageCount++;
    TEST_ASSERT(deliveryId != 0, ("Delivery ID must be non-zero"));
    TEST_ASSERT_EQUAL(deliveryId, pMsgDetails->OrderId);

    ism_engine_releaseMessage(hMessage);

    if (deliveryId == 1)
    {
        TEST_ASSERT_EQUAL(state, ismMESSAGE_STATE_RECEIVED);

        rc = ism_engine_confirmMessageDelivery(pCallbackContext->hSession,
                                               NULL,
                                               hDelivery,
                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                               NULL, 0,
                                               ackCompleted);

        TEST_ASSERT_CUNIT(((rc == OK)||(rc == ISMRC_AsyncCompletion)),
                          ("rc was %d",rc));
    }
    else
    {
        TEST_ASSERT_EQUAL(deliveryId, 2);
        TEST_ASSERT_EQUAL(state, ismMESSAGE_STATE_DELIVERED);

        rc = ism_engine_confirmMessageDelivery(pCallbackContext->hSession,
                                               NULL,
                                               hDelivery,
                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                               NULL, 0,
                                               ackCompleted);

        TEST_ASSERT_CUNIT(((rc == OK)||(rc == ISMRC_AsyncCompletion)),
                          ("rc was %d",rc));
    }

    if (rc == OK)
    {
        ackCompleted(rc, NULL, NULL);
    }

    return moreMessagesPlease;
}


/*
 * Message-delivery callback after repeated reconnection
 */
static bool deliveryCallbackAfterRepeatedReconnect(
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
    bool moreMessagesPlease = true;
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;

    // We got a message!
    pCallbackContext->MessageCount++;

    return moreMessagesPlease;
}


/*
 * Unreleased delivery ID callback
 */
static void unreleasedCallback(
    uint32_t                        deliveryId,
    ismEngine_UnreleasedHandle_t    hUnrel,
    void *                          pContext)
{
    callbackContext_t *pCallbackContext = (callbackContext_t *)pContext;

    TEST_ASSERT_EQUAL(pCallbackContext->MessageCount + 1, deliveryId);

    pCallbackContext->MessageCount++;
}

/*
 * Very simple test which publishes a QoS2 message from a durable client
 * and then releases the message. 
 * This testcase was created in order to recreate a defect where the
 * store transcation containing the unreleased message state was not
 * committed becuase there was no subscribers.
 */
void qos2TestNoSubs(void)
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ProducerHandle_t hProducerA, hProducerB;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_UnreleasedHandle_t hUnrelA, hUnrelB;
    callbackContext_t callbackContext = CALLBACKCONTEXT_DEFAULT;
    int32_t rc = OK;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

    // Create the durable client
    rc = sync_ism_engine_createClientState("qos2TestNoSubs",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClient);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a subscription on Topic B
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_createSubscription(hClient,
                                       "SubTopicB",
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       "Topic/B",
                                       &subAttrs,
                                       NULL,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining); // Owning client same as requesting client
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    // Create producer on Topic A
    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &hProducerA,
                                   NULL,
                                   0,
                                   NULL);

    // Create producer on Topic B
    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "Topic/B",
                                   &hProducerB,
                                   NULL,
                                   0,
                                   NULL);

    test_waitForRemainingActions(pActionsRemaining);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 0);

    // Now publish the message
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    header.Reliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;
    ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
    size_t areaLengths[2] = {0, 13};
    void *areaData[2] = {NULL, "Message body"};

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageWithDeliveryId(hSession,
                                                  hProducerA,
                                                  NULL,
                                                  hMessage,
                                                  1,
                                                  &hUnrelA);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    // Now publish the message
    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageWithDeliveryId(hSession,
                                                  hProducerB,
                                                  NULL,
                                                  hMessage,
                                                  1,
                                                  &hUnrelB);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now attempt to acknowledge that the messages have been seen
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_removeUnreleasedDeliveryId(hSession,
                                               NULL,
                                               hUnrelA,
                                               &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    test_incrementActionsRemaining(pActionsRemaining, 4);
    rc = ism_engine_removeUnreleasedDeliveryId(hSession,
                                               NULL,
                                               hUnrelB,
                                               &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroyProducer(hProducerA,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroyProducer(hProducerB,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroySession(hSession,
                                   &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    callbackContext.hClient = hClient;
    callbackContext.hSession = NULL;
    callbackContext.hConsumer = NULL;;
    callbackContext.MessageCount = 0;

    rc = ism_engine_listUnreleasedDeliveryIds(hClient,
                                              &callbackContext,
                                              unreleasedCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 0);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);
}
