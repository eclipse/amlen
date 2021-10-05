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
/* Module Name: test_qos0.c                                          */
/*                                                                   */
/* Description: CUnit tests of Engine MQTT QoS0-style functions      */
/*                                                                   */
/*********************************************************************/
#include "test_qos0.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

CU_TestInfo ISM_Engine_CUnit_QoS0Basic[] =
{
    { "QoS0NoSubscriberDelivery",                 qos0TestNoSubscriberDelivery                 },
    { "QoS0SingleDelivery",                       qos0TestSingleDelivery                       },
    { "QoS0MultipleDelivery",                     qos0TestMultipleDelivery                     },
    { "QoS0DeliveryStopDelivery",                 qos0TestDeliveryStopDelivery                 },
    { "QoS0DeliveryDestroyConsumer",              qos0TestDeliveryDestroyConsumer              },
    { "QoS0DeliveryResumeConsumer",               qos0TestDeliveryResumeConsumer               },
    { "QoS0DeliveryDestroySession",               qos0TestDeliveryDestroySession               },
    { "QoS0DeliveryDestroyClient",                qos0TestDeliveryDestroyClient                },
    { "QoS0DeliveryStopDeliveryRetcode",          qos0TestDeliveryStopDeliveryRetcode          },
    { "QoS0DeliveryStopDeliveryRetcodeThenStart", qos0TestDeliveryStopDeliveryRetcodeThenStart },
    { "QoS0DeliveryStopDeliveryCallback",         qos0TestDeliveryStopDeliveryCallback         },
    { "QoS0DeliveryDestroyConsumerCallback",      qos0TestDeliveryDestroyConsumerCallback      },
    { "QoS0DeliveryDestroySessionCallback",       qos0TestDeliveryDestroySessionCallback       },
    { "QoS0DeliveryDestroyClientCallback",        qos0TestDeliveryDestroyClientCallback        },
    { "QoS0DeliveryLateStart",                    qos0TestDeliveryLateStart                    },
    { "Qos0TestDuplicateConsumer",                qos0TestDuplicateConsumer                    },
    CU_TEST_INFO_NULL
};

static uint32_t remainingMessages = 0;


typedef struct callbackContext
{
    int32_t                         MessageCount;
    ismEngine_ClientStateHandle_t   hClient;
    ismEngine_SessionHandle_t       hSession;
    ismEngine_ConsumerHandle_t      hConsumer;
} callbackContext_t;

static bool deliveryCallback(
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
    void *                          pConsumerContext);

static bool deliveryCallbackStopOnLimit(
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
    void *                          pConsumerContext);

static bool deliveryCallbackStopDelivery(
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
    void *                          pConsumerContext);

static bool deliveryCallbackDestroyConsumer(
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
    void *                          pConsumerContext);

static bool deliveryCallbackDestroySession(
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
    void *                          pConsumerContext);

static bool deliveryCallbackDestroyClientState(
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
    void *                          pConsumerContext);

/*
 * Publish message with no subscriber
 */
void qos0TestNoSubscriberDelivery(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_MessageHandle_t hMessage;
    int32_t rc = OK;
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    rc = ism_engine_createClientState(__func__,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessageOnDestination(hSessionPub,
                                            ismDESTINATION_TOPIC,
                                            "Topic/A",
                                            NULL,
                                            hMessage,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

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
 * Publish message with a single QoS 0 subscription
 */
void qos0TestSingleDelivery(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    rc = ism_engine_createClientState("qos0TestSingleDelivery_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_createClientState("qos0TestSingleDelivery_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 1);

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
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
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
 * Publish message with multiple QoS 0 subscriptions
 */
void qos0TestMultipleDelivery(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_ClientStateHandle_t hClientSub2;
    ismEngine_SessionHandle_t hSessionSub2;
    ismEngine_ConsumerHandle_t hConsumerSub2;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("qos0TestMultipleDelivery_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_createClientState("qos0TestMultipleDelivery_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestMultipleDelivery_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub2,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub2,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSessionSub2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 4);

    rc = ism_engine_stopMessageDelivery(hSessionSub2,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumerSub2,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
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
 * Publish message then stop delivery and ensure it stops
 */
void qos0TestDeliveryStopDelivery(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_ClientStateHandle_t hClientSub2;
    ismEngine_SessionHandle_t hSessionSub2;
    ismEngine_ConsumerHandle_t hConsumerSub2;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("qos0TestDeliveryStopDelivery_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_createClientState("qos0TestDeliveryStopDelivery_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestDeliveryStopDelivery_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub2,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSessionSub2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub2,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_stopMessageDelivery(hSessionSub2,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 3);

    rc = ism_engine_destroyConsumer(hConsumerSub2,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
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
 * Publish message then destroy the consumer, ensuring delivery stops
 */
void qos0TestDeliveryDestroyConsumer(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_ClientStateHandle_t hClientSub2;
    ismEngine_SessionHandle_t hSessionSub2;
    ismEngine_ConsumerHandle_t hConsumerSub2;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("qos0TestDeliveryDestroyConsumer_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_createClientState("qos0TestDeliveryDestroyConsumer_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestDeliveryDestroyConsumer_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub2,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSessionSub2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub2,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumerSub2,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 3);

    rc = ism_engine_destroySession(hSessionSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
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

//Supplied as a callback if we want to check the callback /isn't/ being called
void unexpectedCB(int32_t retcode, void *handle, void *pContext)
{
    //Should not be called as we expected the pause to happen inline
    TEST_ASSERT_CUNIT(0, ("unexpectedCB was called"));
}
/*
 * Publish message which causes consumer the consumer to ask for no more messages,
 * then resume message delivery to the consumer
 */
void qos0TestDeliveryResumeConsumer(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("qos0TestDeliveryResumeConsumer_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_createClientState("qos0TestDeliveryResumeConsumer_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    //Only receive 1 message before stopping...
    remainingMessages = 1;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallbackStopOnLimit,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    //The consumer will have paused itself when it was delivered the message

    //Check there are no messages available
    rc = ism_engine_checkAvailableMessages(hConsumerSub1);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMsgAvail);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 1); //Only the first message has been received so far...

    //Check that there are now message(s) available
    rc = ism_engine_checkAvailableMessages(hConsumerSub1);
    TEST_ASSERT_EQUAL(rc, OK);

    remainingMessages = 1; //Receive 1 more and turn off again...

    rc = ism_engine_resumeMessageDelivery(hConsumerSub1
                                         ,ismENGINE_RESUME_DELIVERY_OPTION_NONE
                                         , NULL, 0, NULL);

    //Messages should now have been delivered so our received message count should have increased
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 2);

    //...and the consumer should now have no gettable messages
    rc = ism_engine_checkAvailableMessages(hConsumerSub1);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMsgAvail);

    rc = ism_engine_destroyConsumer(hConsumerSub1,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
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
 * Publish message then destroy consuming session, ensuring delivery stops
 */
void qos0TestDeliveryDestroySession(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_ClientStateHandle_t hClientSub2;
    ismEngine_SessionHandle_t hSessionSub2;
    ismEngine_ConsumerHandle_t hConsumerSub2;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("qos0TestDeliveryDestroySession_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_createClientState("qos0TestDeliveryDestroySession_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestDeliveryDestroySession_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub2,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSessionSub2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub2,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 3);

    rc = ism_engine_destroyClientState(hClientSub2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
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
 * Publish message then destroy consuming client-state, ensuring delivery stops
 */
void qos0TestDeliveryDestroyClient(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_ClientStateHandle_t hClientSub2;
    ismEngine_SessionHandle_t hSessionSub2;
    ismEngine_ConsumerHandle_t hConsumerSub2;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("qos0TestDeliveryDestroyClient_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_createClientState("qos0TestDeliveryDestroyClient_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestDeliveryDestroyClient_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub2,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSessionSub2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub2,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 3);

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
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
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
 * Publish message and stop delivery using message-delivery callback return code
 */
void qos0TestDeliveryStopDeliveryRetcode(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_ClientStateHandle_t hClientSub2;
    ismEngine_SessionHandle_t hSessionSub2;
    ismEngine_ConsumerHandle_t hConsumerSub2;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("qos0TestDeliveryStopDeliveryRetcode_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_createClientState("qos0TestDeliveryStopDeliveryRetcode_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestDeliveryStopDeliveryRetcode_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub2,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub2,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallbackStopOnLimit,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    remainingMessages = 1;

    rc = ism_engine_startMessageDelivery(hSessionSub2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 3);

    rc = ism_engine_stopMessageDelivery(hSessionSub2,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumerSub2,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
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
 * Publish message and stop delivery using message-delivery callback return code,
 * then restart delivery
 */
void qos0TestDeliveryStopDeliveryRetcodeThenStart(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_ClientStateHandle_t hClientSub2;
    ismEngine_SessionHandle_t hSessionSub2;
    ismEngine_ConsumerHandle_t hConsumerSub2;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("qos0TestDeliveryStopDeliveryRetcodeThenStart_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_createClientState("qos0TestDeliveryStopDeliveryRetcodeThenStart_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestDeliveryStopDeliveryRetcodeThenStart_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub2,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub2,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallbackStopOnLimit,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    remainingMessages = 1;

    rc = ism_engine_startMessageDelivery(hSessionSub2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 3);

    remainingMessages = 1;

    rc = ism_engine_startMessageDelivery(hSessionSub2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 4);

    rc = ism_engine_stopMessageDelivery(hSessionSub2,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumerSub2,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
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
 * Publish message and stop delivery in the message-delivery callback
 */
void qos0TestDeliveryStopDeliveryCallback(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_ClientStateHandle_t hClientSub2;
    ismEngine_SessionHandle_t hSessionSub2;
    ismEngine_ConsumerHandle_t hConsumerSub2;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("qos0TestDeliveryStopDeliveryCallback_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_createClientState("qos0TestDeliveryStopDeliveryCallback_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestDeliveryStopDeliveryCallback_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub2,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext_t callbackContext2;
    callbackContext2.hClient = hClientSub2;
    callbackContext2.hSession = hSessionSub2;
    callbackContext2.hConsumer = 0;
    callbackContext2.MessageCount = 0;
    callbackContext_t *pCallbackContext2 = &callbackContext2;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub2,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext2,
                                   sizeof(callbackContext_t *),
                                   deliveryCallbackStopDelivery,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext2.hConsumer = hConsumerSub2;

    rc = ism_engine_startMessageDelivery(hSessionSub2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL((callbackContext.MessageCount + callbackContext2.MessageCount), 3);

    rc = ism_engine_startMessageDelivery(hSessionSub2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL((callbackContext.MessageCount + callbackContext2.MessageCount), 4);

    rc = ism_engine_stopMessageDelivery(hSessionSub2,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumerSub2,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
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
 * Publish message and destroy the consumer in the message-delivery callback
 */
void qos0TestDeliveryDestroyConsumerCallback(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_ClientStateHandle_t hClientSub2;
    ismEngine_SessionHandle_t hSessionSub2;
    ismEngine_ConsumerHandle_t hConsumerSub2;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("qos0TestDeliveryDestroyConsumerCallback_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_createClientState("qos0TestDeliveryDestroyConsumerCallback_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestDeliveryDestroyConsumerCallback_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub2,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext_t callbackContext2;
    callbackContext2.hClient = hClientSub2;
    callbackContext2.hSession = hSessionSub2;
    callbackContext2.hConsumer = 0;
    callbackContext2.MessageCount = 0;
    callbackContext_t *pCallbackContext2 = &callbackContext2;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub2,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext2,
                                   sizeof(callbackContext_t *),
                                   deliveryCallbackDestroyConsumer,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext2.hConsumer = hConsumerSub2;

    rc = ism_engine_startMessageDelivery(hSessionSub2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL((callbackContext.MessageCount + callbackContext2.MessageCount), 3);

    rc = ism_engine_destroySession(hSessionSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
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
 * Publish message and destroy the session in the message-delivery callback
 */
void qos0TestDeliveryDestroySessionCallback(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_ClientStateHandle_t hClientSub2;
    ismEngine_SessionHandle_t hSessionSub2;
    ismEngine_ConsumerHandle_t hConsumerSub2;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    rc = ism_engine_createClientState("qos0TestDeliveryDestroySessionCallback_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_createClientState("qos0TestDeliveryDestroySessionCallback_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestDeliveryDestroySessionCallback_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub2,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext_t callbackContext2;
    callbackContext2.hClient = hClientSub2;
    callbackContext2.hSession = hSessionSub2;
    callbackContext2.hConsumer = 0;
    callbackContext2.MessageCount = 0;
    callbackContext_t *pCallbackContext2 = &callbackContext2;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub2,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext2,
                                   sizeof(callbackContext_t *),
                                   deliveryCallbackDestroySession,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext2.hConsumer = hConsumerSub2;

    rc = ism_engine_startMessageDelivery(hSessionSub2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL((callbackContext.MessageCount + callbackContext2.MessageCount), 3);

    rc = ism_engine_destroyClientState(hClientSub2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
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
 * Publish message and destroy the client-state in the message-delivery callback
 */
void qos0TestDeliveryDestroyClientCallback(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_ClientStateHandle_t hClientSub2;
    ismEngine_SessionHandle_t hSessionSub2;
    ismEngine_ConsumerHandle_t hConsumerSub2;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("qos0TestDeliveryDestroyClientCallback_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_createClientState("qos0TestDeliveryDestroyClientCallback_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestDeliveryDestroyClientCallback_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub2,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext_t callbackContext2;
    callbackContext2.hClient = hClientSub2;
    callbackContext2.hSession = hSessionSub2;
    callbackContext2.hConsumer = 0;
    callbackContext2.MessageCount = 0;
    callbackContext_t *pCallbackContext2 = &callbackContext2;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub2,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext2,
                                   sizeof(callbackContext_t *),
                                   deliveryCallbackDestroyClientState,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext2.hConsumer = hConsumerSub2;

    rc = ism_engine_startMessageDelivery(hSessionSub2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL((callbackContext.MessageCount + callbackContext2.MessageCount), 3);

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
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
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
 * Publish message when there's a subscriber but delivery is not started, then start delivery
 */
void qos0TestDeliveryLateStart(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_ClientStateHandle_t hClientSub2;
    ismEngine_SessionHandle_t hSessionSub2;
    ismEngine_ConsumerHandle_t hConsumerSub2;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("qos0TestDeliveryLateStart_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_createClientState("qos0TestDeliveryLateStart_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestDeliveryLateStart_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub2,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub2,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 2);

    rc = ism_engine_startMessageDelivery(hSessionSub2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 4);

    rc = ism_engine_stopMessageDelivery(hSessionSub2,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumerSub2,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionSub2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

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
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
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
 * Create a subscription then try and connect 2 consumers to it
 */
void qos0TestDuplicateConsumer(void)
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer1;
    ismEngine_ConsumerHandle_t hConsumer2;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = sync_ism_engine_createClientState("DupConsumerClient",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &hClient);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;

    rc = sync_ism_engine_createSubscription(hClient,
                                       "sub1",
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       "Topic1",
                                       &subAttrs,
                                       NULL ); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "sub1",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   NULL,
                                   0,
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "sub1",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   NULL,
                                   0,
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer2,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_WaiterInUse);

    rc = sync_ism_engine_destroyClientState(hClient,
                                            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
}

/*
 * Message-delivery callbacks used by this unit test
 */
static bool deliveryCallback(
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
    bool moreMessagesPlease = true;
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;

    // We got a message!
    pCallbackContext->MessageCount++;

    ism_engine_releaseMessage(hMessage);

    return moreMessagesPlease;
}

static bool deliveryCallbackStopOnLimit(
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
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;

    // We got a message!
    pCallbackContext->MessageCount++;

    ism_engine_releaseMessage(hMessage);

    if (remainingMessages > 0)
    {
        remainingMessages--;
    }

    return (remainingMessages > 0) ? true : false;
}

static bool deliveryCallbackStopDelivery(
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
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t __attribute__((unused)) rc = OK;

    // We got a message!
    pCallbackContext->MessageCount++;

    ism_engine_releaseMessage(hMessage);

    rc = ism_engine_stopMessageDelivery(pCallbackContext->hSession, NULL, 0, NULL);

    return false;
}

static bool deliveryCallbackDestroyConsumer(
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
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t __attribute__((unused)) rc = OK;

    // We got a message!
    pCallbackContext->MessageCount++;

    ism_engine_releaseMessage(hMessage);

    rc = ism_engine_destroyConsumer(pCallbackContext->hConsumer, NULL, 0, NULL);

    return false;
}

static bool deliveryCallbackDestroySession(
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
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t __attribute__((unused)) rc = OK;

    // We got a message!
    pCallbackContext->MessageCount++;

    ism_engine_releaseMessage(hMessage);

    rc = ism_engine_destroySession(pCallbackContext->hSession, NULL, 0, NULL);

    return false;
}

static bool deliveryCallbackDestroyClientState(
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
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t __attribute__((unused)) rc = OK;

    // We got a message!
    pCallbackContext->MessageCount++;

    ism_engine_releaseMessage(hMessage);

    rc = ism_engine_destroyClientState(pCallbackContext->hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);

    return false;
}
