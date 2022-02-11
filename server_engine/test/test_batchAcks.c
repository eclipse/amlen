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
/* Module Name: test_batchAcks.c                                     */
/*                                                                   */
/* Description: CUnit tests of Engine batched acknowledgements       */
/*                                                                   */
/*********************************************************************/
#include "test_batchAcks.h"
#include "test_utils_assert.h"
#include "test_utils_security.h"
#include "test_utils_options.h"

CU_TestInfo ISM_Engine_CUnit_BatchAcks[] = {
    { "BatchAcksSessionRecoverPubSub",      batchAcksTestSessionRecoverPubSub      },
    CU_TEST_INFO_NULL
};


#define NUM_MSGS 3

typedef struct callbackContext
{
    bool                            fConfirmAsConsumed;
    int32_t                         MessagesReceived;
    int32_t                         MessagesConsumed;
    ismEngine_DeliveryHandle_t      hDelivery[NUM_MSGS];
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
 * Deliver a bunch of messages to a nondurable subscriber, confirm as NOT_RECEIVED and
 * then confirm them as consumed. This mimics JMS Session.recover().
 */
void batchAcksTestSessionRecoverPubSub(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE };
    int32_t rc = OK;

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext_t callbackContext = {false, 0, 0};
    callbackContext.hClient = hClient1;
    callbackContext.hSession = hSession1;
    callbackContext_t *pCallbackContext = &callbackContext;

    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_TOPIC,
                                   "Topic1",
                                   &subAttrs,
                                   NULL, // Owning client same as session client
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    for (int i = 0; i < NUM_MSGS; i++)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
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

        rc = ism_engine_putMessageOnDestination(hSession1,
                                                ismDESTINATION_TOPIC,
                                                "Topic1",
                                                NULL,
                                                hMessage,
                                                NULL,
                                                0,
                                                NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = ism_engine_startMessageDelivery(hSession1,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessagesConsumed, NUM_MSGS);

    rc = ism_engine_destroyClientState(hClient1,
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
    ismEngine_DelivererContext_t *  _delivererContext )
{
    bool moreMessagesPlease = true;
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t rc = OK;

    // We got a message!
    if (pCallbackContext->fConfirmAsConsumed)
    {
        pCallbackContext->MessagesConsumed++;
    }
    else
    {
        pCallbackContext->hDelivery[pCallbackContext->MessagesReceived] = hDelivery;
        pCallbackContext->MessagesReceived++;

        if (pCallbackContext->MessagesReceived == NUM_MSGS)
        {
            pCallbackContext->fConfirmAsConsumed = true;
            rc = ism_engine_confirmMessageDeliveryBatch(pCallbackContext->hSession,
                                                        NULL,
                                                        pCallbackContext->MessagesReceived,
                                                        pCallbackContext->hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_NOT_RECEIVED,
                                                        NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    ism_engine_releaseMessage(hMessage);

    return moreMessagesPlease;
}
