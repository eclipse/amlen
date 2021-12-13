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
/* Module Name: test_qos1.c                                          */
/*                                                                   */
/* Description: CUnit tests of Engine MQTT QoS1-style functions      */
/*                                                                   */
/*********************************************************************/
#include "test_qos1.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

CU_TestInfo ISM_Engine_CUnit_QoS1Basic[] =
{
    { "QoS1SingleDelivery", qos1TestSingleDelivery },
    CU_TEST_INFO_NULL
};


typedef struct callbackContext
{
    uint32_t                        MessageCount;
    ismEngine_ClientStateHandle_t   hClient;
    ismEngine_SessionHandle_t       hSession;
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
    void *                          pConsumerContext,
    ismEngine_DelivererContext_t *  _delivererContext);

/*
 * Publish message with a single QoS 1 subscription
 */
void qos1TestSingleDelivery(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_MessageHandle_t hMessage;
    int32_t rc = OK;

    rc = ism_engine_createClientState("qos1TestSingleDelivery_PUB",
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

    rc = ism_engine_createClientState("qos1TestSingleDelivery_SUB1",
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
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
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
    void *                          pConsumerContext,
    ismEngine_DelivererContext_t *  _delivererContext)
{
    bool moreMessagesPlease = true;
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t rc = OK;

    // We got a message!
    pCallbackContext->MessageCount++;

    TEST_ASSERT_EQUAL(state, ismMESSAGE_STATE_DELIVERED);
    TEST_ASSERT(pMsgDetails->OrderId > 0, ("Order ID (%u) must be greater than zero for a persistent message", pMsgDetails->OrderId));

    ism_engine_releaseMessage(hMessage);

    rc = ism_engine_confirmMessageDelivery(pCallbackContext->hSession,
                                           NULL,
                                           hDelivery,
                                           ismENGINE_CONFIRM_OPTION_CONSUMED,
                                           NULL,
                                           0,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    return moreMessagesPlease;
}
