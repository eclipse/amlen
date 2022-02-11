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

CU_TestInfo ISM_Engine_CUnit_QoS0ExplicitSuspend[] =
{
    { "QoS0ExplicitSuspendDestroyConsumer",              qos0TestExplicitSuspendDestroyConsumer              },
    { "QoS0ExplicitSuspendResumeConsumer",               qos0TestExplicitSuspendResumeConsumer               },
    { "QoS0ExplicitSuspendDestroySession",               qos0TestExplicitSuspendDestroySession               },
    { "QoS0ExplicitSuspendDestroyClient",                qos0TestExplicitSuspendDestroyClient                },
    { "QoS0ExplicitSuspendStopDeliveryRetcode",          qos0TestExplicitSuspendStopDeliveryRetcode          },
    { "QoS0ExplicitSuspendStopDeliveryRetcodeThenStart", qos0TestExplicitSuspendStopDeliveryRetcodeThenStart },
    { "QoS0ExplicitSuspendStopDeliveryCallback",         qos0TestExplicitSuspendStopDeliveryCallback         },
    { "QoS0ExplicitSuspendDestroyConsumerCallback",      qos0TestExplicitSuspendDestroyConsumerCallback      },
    { "QoS0ExplicitSuspendDestroySessionCallback",       qos0TestExplicitSuspendDestroySessionCallback       },
    { "QoS0ExplicitSuspendDestroyClientCallback",        qos0TestExplicitSuspendDestroyClientCallback        },
    { "QoS0ExplicitSuspendLateStart",                    qos0TestExplicitSuspendLateStart                    },
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

static bool deliveryExplicitSuspendCallback(
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
    ismEngine_DelivererContext_t *  _delivererContext );

static bool deliveryExplicitSuspendCallbackStopOnLimit(
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
    ismEngine_DelivererContext_t *  _delivererContext );

static bool deliveryCallbackExplicitSuspendStopDelivery(
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
    ismEngine_DelivererContext_t *  _delivererContext );

static bool deliveryCallbackExplicitSuspendDestroyConsumer(
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
    ismEngine_DelivererContext_t *  _delivererContext );

static bool deliveryCallbackExplicitSuspendDestroySession(
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
    ismEngine_DelivererContext_t *  _delivererContext );

static bool deliveryCallbackExplicitSuspendDestroyClientState(
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
    ismEngine_DelivererContext_t *  _delivererContext );



/*
 * Publish message then destroy the consumer, ensuring delivery stops
 */
void qos0TestExplicitSuspendDestroyConsumer(void)
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

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroyConsumer_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientPub,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroyConsumer_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroyConsumer_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallback,
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

/*
 * Publish message which causes consumer the consumer to ask for no more messages,
 * then resume message delivery to the consumer
 */
void qos0TestExplicitSuspendResumeConsumer(void)
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

    rc = ism_engine_createClientState("qos0TestExplicitSuspendResumeConsumer_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientPub,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendResumeConsumer_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallbackStopOnLimit,
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
void qos0TestExplicitSuspendDestroySession(void)
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

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroySession_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientPub,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroySession_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroySession_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallback,
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
void qos0TestExplicitSuspendDestroyClient(void)
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

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroyClient_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientPub,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroyClient_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroyClient_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallback,
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
void qos0TestExplicitSuspendStopDeliveryRetcode(void)
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

    rc = ism_engine_createClientState("qos0TestExplicitSuspendStopDeliveryRetcode_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientPub,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendStopDeliveryRetcode_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendStopDeliveryRetcode_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallbackStopOnLimit,
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
void qos0TestExplicitSuspendStopDeliveryRetcodeThenStart(void)
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

    rc = ism_engine_createClientState("qos0TestExplicitSuspendStopDeliveryRetcodeThenStart_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientPub,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendStopDeliveryRetcodeThenStart_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendStopDeliveryRetcodeThenStart_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallbackStopOnLimit,
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
void qos0TestExplicitSuspendStopDeliveryCallback(void)
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

    rc = ism_engine_createClientState("qos0TestExplicitSuspendStopDeliveryCallback_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientPub,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendStopDeliveryCallback_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendStopDeliveryCallback_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryCallbackExplicitSuspendStopDelivery,
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
void qos0TestExplicitSuspendDestroyConsumerCallback(void)
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

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroyConsumerCallback_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientPub,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroyConsumerCallback_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroyConsumerCallback_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryCallbackExplicitSuspendDestroyConsumer,
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
void qos0TestExplicitSuspendDestroySessionCallback(void)
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

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroySessionCallback_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientPub,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroySessionCallback_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroySessionCallback_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryCallbackExplicitSuspendDestroySession,
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
void qos0TestExplicitSuspendDestroyClientCallback(void)
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

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroyClientCallback_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientPub,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroyClientCallback_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendDestroyClientCallback_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryCallbackExplicitSuspendDestroyClientState,
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
void qos0TestExplicitSuspendLateStart(void)
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

    rc = ism_engine_createClientState("qos0TestExplicitSuspendLateStart_PUB",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientPub,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendLateStart_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("qos0TestExplicitSuspendLateStart_SUB2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSub2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub2,
                                  ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
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
                                   deliveryExplicitSuspendCallback,
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
 * Message-delivery callbacks used by this unit test
 */
static bool deliveryExplicitSuspendCallback(
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

    ism_engine_releaseMessage(hMessage);

    return moreMessagesPlease;
}

static bool deliveryExplicitSuspendCallbackStopOnLimit(
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
    bool reenable = true;

    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;

    // We got a message!
    pCallbackContext->MessageCount++;

    ism_engine_releaseMessage(hMessage);

    if (remainingMessages > 0)
    {
        remainingMessages--;
    }

    if (remainingMessages > 0)
    {
        reenable = true;
    }
    else
    {
        ism_engine_suspendMessageDelivery(hConsumer, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
        reenable = false;
    }

    return reenable;
}

static bool deliveryCallbackExplicitSuspendStopDelivery(
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
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t __attribute__((unused)) rc = OK;

    // We got a message!
    pCallbackContext->MessageCount++;

    ism_engine_releaseMessage(hMessage);

    rc = ism_engine_stopMessageDelivery(pCallbackContext->hSession, NULL, 0, NULL);

    ism_engine_suspendMessageDelivery(hConsumer, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
    return false;
}

static bool deliveryCallbackExplicitSuspendDestroyConsumer(
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
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t __attribute__((unused)) rc = OK;

    // We got a message!
    pCallbackContext->MessageCount++;

    ism_engine_releaseMessage(hMessage);

    rc = ism_engine_destroyConsumer(pCallbackContext->hConsumer, NULL, 0, NULL);

    ism_engine_suspendMessageDelivery(hConsumer, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
    return false;
}

static bool deliveryCallbackExplicitSuspendDestroySession(
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
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t __attribute__((unused)) rc = OK;

    // We got a message!
    pCallbackContext->MessageCount++;

    ism_engine_releaseMessage(hMessage);

    rc = ism_engine_destroySession(pCallbackContext->hSession, NULL, 0, NULL);

    ism_engine_suspendMessageDelivery(hConsumer, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
    return false;
}

static bool deliveryCallbackExplicitSuspendDestroyClientState(
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
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t __attribute__((unused)) rc = OK;

    // We got a message!
    pCallbackContext->MessageCount++;

    ism_engine_releaseMessage(hMessage);

    rc = ism_engine_destroyClientState(pCallbackContext->hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);

    ism_engine_suspendMessageDelivery(hConsumer, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
    return false;
}
