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
/* Module Name: test_willMessage.c                                   */
/*                                                                   */
/* Description: CUnit tests of Engine will message functions         */
/*                                                                   */
/*********************************************************************/
#include "test_willMessage.h"
#include "test_utils_assert.h"
#include "test_utils_security.h"
#include "test_utils_options.h"
#include "test_utils_message.h"
#include "test_utils_sync.h"
#include "test_utils_pubsuback.h"
#include "test_utils_client.h"
#include "test_utils_log.h"
#include "clientState.h"
#include "policyInfo.h" //For setting max subscription depths
#include "topicTree.h"
#include "queueCommon.h"

CU_TestInfo ISM_Engine_CUnit_WillMessageBasic[] = {
    { "WillMessageNoSubscribers",           willMessageTestNoSubscribers           },
    { "WillMessageOneSubscriber",           willMessageTestOneSubscriber           },
    { "WillMessageUnset",                   willMessageTestUnset                   },
    { "WillMessageNondurableNonpersistent", willMessageTestNondurableNonpersistent },
    { "WillMessageNondurablePersistent",    willMessageTestNondurablePersistent    },
    { "WillMessageDurableNonpersistent",    willMessageTestDurableNonpersistent    },
    { "WillMessageDurablePersistent",       willMessageTestDurablePersistent       },
    { "WillMessageDurableDiscard",          willMessageTestDurableDiscard          },
    { "WillMessageDurableSimpQDestFull",    willMessageTestDurableSimpQDestFull      },
    { "WillMessageDurableInterDestFull",    willMessageTestDurableIntermediateDestFull   },
    { "WillMessageDurableMultiDestFull",    willMessageTestDurableMultiConsumerQDestFull },
    { "WillMessageInvalidTopic",            willMessageTestInvalidTopic            },
    { "WillMessageClearRetained",           willMessageTestClearRetained           },
    { "WillMessageRequestInProgress",       willMessageTestRequestInProgress       },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_Engine_CUnit_WillMessageSecurity[] = {
    { "WillMessageSecurity",                willMessageTestSecurity                },
    { "WillMessageMaxMessageTimeToLive",    willMessageMaxMessageTimeToLive        },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_Engine_CUnit_WillDelay[] = {
    { "WillMessageDelay",                   willMessageTestDelay                   },
    { "WillMessageDelayTakeOver",           willMessageTestDelayTakeOver           },
    CU_TEST_INFO_NULL
};

typedef struct callbackContext
{
    int32_t                         MessageCount;
    int32_t                         ExpiringMessageCount;
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


/*
 * Destroy a client-state sending a will message to zero subscribers
 */
void willMessageTestNoSubscribers(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    ismEngine_MessageHandle_t hMessage;
    int32_t rc = OK;

    rc = ism_engine_createClientState(__func__,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
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

    rc = ism_engine_setWillMessage(hClient1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}


/*
 * Destroy a client-state sending a will message to one subscriber
 */
void willMessageTestOneSubscriber(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("willMessageTestOneSubscriber_PUB",
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

    rc = ism_engine_createClientState("willMessageTestOneSubscriber_SUB1",
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
    callbackContext.ExpiringMessageCount = 0;
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

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
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

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 1);
    TEST_ASSERT_EQUAL(callbackContext.ExpiringMessageCount, 0);

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
}


/*
 * Destroy a client-state after unsetting the will message
 */
void willMessageTestUnset(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("willMessageTestUnset_PUB",
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

    rc = ism_engine_createClientState("willMessageTestUnset_SUB1",
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
    callbackContext.ExpiringMessageCount = 0;
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

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_unsetWillMessage(hClientPub,
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

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 0);
    TEST_ASSERT_EQUAL(callbackContext.ExpiringMessageCount, 0);

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
}


/*
 * Destroy a nondurable client-state sending a nonpersistent will message to one subscriber
 */
void willMessageTestNondurableNonpersistent(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("willMessageTestNondurableNonpersistent_PUB",
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

    rc = ism_engine_createClientState("willMessageTestNondurableNonpersistent_SUB1",
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
    callbackContext.ExpiringMessageCount = 0;
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

    // Start by setting a persistent message
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
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

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Unset the will message
    rc = ism_engine_unsetWillMessage(hClientPub,
                                     NULL,
                                     0,
                                     NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now set a nonpersistent message
    header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
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

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 1);
    TEST_ASSERT_EQUAL(callbackContext.ExpiringMessageCount, 0);

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
}


/*
 * Destroy a nondurable client-state sending a persistent will message to one subscriber
 */
void willMessageTestNondurablePersistent(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = ism_engine_createClientState("willMessageTestNondurablePersistent_PUB",
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

    rc = ism_engine_createClientState("willMessageTestNondurablePersistent_SUB1",
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
    callbackContext.ExpiringMessageCount = 0;
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

    // Start by setting a nonpersistent message
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
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

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Unset the will message
    rc = ism_engine_unsetWillMessage(hClientPub,
                                     NULL,
                                     0,
                                     NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now replace it with another nonpersistent message
    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now replace it with a persistent message without unsetting first
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And finally replace it with another persistent message without unsetting first
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
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
}


/*
 * Destroy a durable client-state sending a nonpersistent will message to one subscriber
 */
void willMessageTestDurableNonpersistent(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = sync_ism_engine_createClientState("ClientDurableNonpersistent",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &hClientPub);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("willMessageTestDurableNonpersistent_SUB1",
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
    callbackContext.ExpiringMessageCount = 0;
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

    // Start by setting a persistent message
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
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

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Unset the will message
    rc = ism_engine_unsetWillMessage(hClientPub,
                                     NULL,
                                     0,
                                     NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now set a nonpersistent message
    header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionPub,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroyClientState(hClientPub,
                                            ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 1);
    TEST_ASSERT_EQUAL(callbackContext.ExpiringMessageCount, 0);

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
}


/*
 * Destroy a durable client-state sending a persistent will message to one subscriber
 */
void willMessageTestDurablePersistent(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    rc = sync_ism_engine_createClientState("ClientDurablePersistent",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &hClientPub);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("willMessageTestDurablePersistent_SUB1",
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
    callbackContext.ExpiringMessageCount = 0;
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

    // Start by setting a nonpersistent message
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
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

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Unset the will message
    rc = ism_engine_unsetWillMessage(hClientPub,
                                     NULL,
                                     0,
                                     NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now replace it with another nonpersistent message
    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now replace it with a persistent message without unsetting first
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And finally replace it with another persistent message without unsetting first
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionPub,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClientPub,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 1);
    TEST_ASSERT_EQUAL(callbackContext.ExpiringMessageCount, 0);

    // doing 4 things potentially asynchronously
    test_incrementActionsRemaining(pActionsRemaining, 4);

    rc = ism_engine_stopMessageDelivery(hSessionSub1,
                                        &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroyConsumer(hConsumerSub1,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroySession(hSessionSub1,
                                   &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);
}


/*
 * Destroy a durable client-state discarding durable state
 */
void willMessageTestDurableDiscard(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    rc = sync_ism_engine_createClientState("ClientDurableDiscard",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &hClientPub);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("willMessageTestDurableDiscard_SUB1",
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
    callbackContext.ExpiringMessageCount = 0;
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

    // Start by setting a persistent message
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
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

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 2);
    rc = ism_engine_destroySession(hSessionPub,
                                   &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroyClientState(hClientPub,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 1);
    TEST_ASSERT_EQUAL(callbackContext.ExpiringMessageCount, 0);

    test_incrementActionsRemaining(pActionsRemaining, 4);
    rc = ism_engine_stopMessageDelivery(hSessionSub1,
                                        &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroyConsumer(hConsumerSub1,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroySession(hSessionSub1,
                                   &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);
}

/*
 * Destroy a durable client-state sending a persistent will message to one subscriber who
 * has a full queue
 *
 * extraSubOptions allows the type of queue to be selected
 */
void willMessageTestDurableDestFullOptions(uint64_t extraSubOptions
                                          ,uint64_t consumerOptions)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;

    const char *fullSubClientId = "willMessageTestDurableDestFull_SUBFull";
    const char *fullSubName     = "FillSub";
    ismEngine_ClientStateHandle_t hClientSubFull;

    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;
    uint32_t maxDepth = 5; //Our receiving sub will have a queue full of this many

    rc = sync_ism_engine_createClientState("ClientDurableDestFullPubber",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &hClientPub);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 5;

    rc = sync_ism_engine_createClientState("willMessageTestDurableDestFull_SUB1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                      NULL, NULL, NULL,
                                      &hClientSub1);
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

    subAttrs.subOptions =  ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                         | extraSubOptions;

    rc = sync_ism_engine_createSubscription(hClientSub1,
                                            "NotFullSub",
                                           NULL,
                                           ismDESTINATION_TOPIC,
                                           "Topic/#",
                                           &subAttrs,
                                           hClientSub1);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext_t callbackContext;
    callbackContext.hClient = hClientSub1;
    callbackContext.hSession = hSessionSub1;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext.ExpiringMessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "NotFullSub",
                                   &subAttrs,
                                   NULL,
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   consumerOptions,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Create the subscriber who won't consume the messages (as we won't start message delivery)
    //and will therefore have a full queue.
    rc = ism_engine_createClientState(fullSubClientId,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClientSubFull,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    if ( extraSubOptions & ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE)
    {
        //We can't use these options for the full blocking sub, as as they
        //just discard the message and let the publication work to other destinations
        subAttrs.subOptions =   ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                              | ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    }
    else
    {
        subAttrs.subOptions =  ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                             | extraSubOptions;
    }

    rc = sync_ism_engine_createSubscription(hClientSubFull,
                                            fullSubName,
                                           NULL,
                                           ismDESTINATION_TOPIC,
                                           "Topic/#",
                                           &subAttrs,
                                           hClientSubFull);
    TEST_ASSERT_EQUAL(rc, OK);

    // Start by filling up the non consuming subscription
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    header.Reliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;
    ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
    size_t areaLengths[2] = {0, 13};
    void *areaData[2] = {NULL, "Message body"};

    for (uint32_t i=0; i<maxDepth; i++) {
        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areaData,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = sync_ism_engine_putMessageOnDestination(hSessionPub,
                                                     ismDESTINATION_TOPIC,
                                                     "Topic/Filler",
                                                     NULL,
                                                     hMessage);
        TEST_ASSERT_EQUAL(rc, ISMRC_OK);
    }

    // Setting a persistent message will message
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    header.Reliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClientPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Check our full sub is actually full before we send the will...
    ismEngine_Subscription_t *fullSubscription = NULL;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    rc = iett_findClientSubscription(pThreadData,
                                     fullSubClientId,
                                     iett_generateClientIdHash(fullSubClientId),
                                     fullSubName,
                                     iett_generateSubNameHash(fullSubName),
                                     &fullSubscription);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(fullSubscription);

    ismEngine_QueueStatistics_t stats;
    ieq_getStats(pThreadData, fullSubscription->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.MaxMessages, maxDepth);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, maxDepth);


    //Before we send the will... try a transaction put that we'll have to roll back...
    //printf("Sending transactional pub\n");
    ismEngine_TransactionHandle_t hTran;
    rc = sync_ism_engine_createLocalTransaction(
                                  hSessionPub,
                                  &hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSessionPub,
                                                 ismDESTINATION_TOPIC,
                                                 "Topic/TranPublish",
                                                 hTran,
                                                 hMessage);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationFull);

    rc = sync_ism_engine_rollbackTransaction(hSessionPub, hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionPub,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Send the will...
    //printf("Now sending the will\n");
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClientPub,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    //Check that the full queue rejected the message...
    ieq_getStats(pThreadData, fullSubscription->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.RejectedMsgs, 2); //1 for transactional put and other for will
    TEST_ASSERT_EQUAL(stats.MaxMessages, maxDepth);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, maxDepth);

    //Let the handle we found to the full queue Subscription go
    rc = iett_releaseSubscription(pThreadData, fullSubscription, false);
    TEST_ASSERT_EQUAL(rc,OK);

    //The will message publication will not have been received due to the failed will public (due to full queue)
    test_waitForMessages((volatile uint32_t *)&(callbackContext.MessageCount), NULL, maxDepth, 20);
    TEST_ASSERT_EQUAL(callbackContext.MessageCount, maxDepth);
    TEST_ASSERT_EQUAL(callbackContext.ExpiringMessageCount, 0);

    // doing 4 things potentially asynchronously
    test_incrementActionsRemaining(pActionsRemaining, 4);

    rc = ism_engine_stopMessageDelivery(hSessionSub1,
                                        &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroyConsumer(hConsumerSub1,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroySession(hSessionSub1,
                                   &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroyClientState(hClientSub1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    rc = sync_ism_engine_destroyClientState(hClientSubFull,
                                            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, ISMRC_OK);

    //Reconnect the Pubbing client so we can then fully destroy it
    rc = sync_ism_engine_createClientState("ClientDurableDestFullPubber",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &hClientPub);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    rc = sync_ism_engine_destroyClientState(hClientPub,
                                            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, ISMRC_OK);
}
void willMessageTestDurableSimpQDestFull(void)
{
    //Run the test with an simpQ queue
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);
    willMessageTestDurableDestFullOptions( ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE
                                         , ismENGINE_CONSUMER_OPTION_NONE);
}
void willMessageTestDurableIntermediateDestFull(void)
{
    //Run the test with an intermediate queue
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);
    willMessageTestDurableDestFullOptions(ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                         , ismENGINE_CONSUMER_OPTION_ACK
                                          |ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID );
}

void willMessageTestDurableMultiConsumerQDestFull(void)
{
    //Run the test with an multiConsumerQ queue
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);
    willMessageTestDurableDestFullOptions( ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                                         | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE
                                         , ismENGINE_CONSUMER_OPTION_ACK
                                         | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID);
}


/*
 * Test specifying an invalid topic
 */
void willMessageTestInvalidTopic(void)
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_MessageHandle_t hMessage;
    int32_t rc = OK;

    rc = ism_engine_createClientState("ClientInvalidTopic",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Start by setting a persistent message
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
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

    // Wrong destination type
    rc = ism_engine_setWillMessage(hClient,
                                   ismDESTINATION_QUEUE,
                                   "NON-EXISTENT.QUEUE",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    // Cannot have wildcards on publish topics
    rc = ism_engine_setWillMessage(hClient,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Invalid Will/#",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);

    rc = ism_engine_destroySession(hSession,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

/*
 * Helper functions for the clear retained unit test
 */

void publishRetained(const char *topic)
{
     ismEngine_ClientStateHandle_t hClient = NULL;
     ismEngine_SessionHandle_t hSession = NULL;
     ismEngine_MessageHandle_t hMessage = NULL;
     int32_t rc = OK;
     uint32_t actionsRemaining = 0;
     uint32_t *pActionsRemaining = &actionsRemaining;

     rc = ism_engine_createClientState(__func__,
                                       testDEFAULT_PROTOCOL_ID,
                                       ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                       NULL, NULL, NULL,
                                       &hClient,
                                       NULL,
                                       0,
                                       NULL);
     TEST_ASSERT_EQUAL(rc, OK);

     // Create Session for this test
     rc=ism_engine_createSession( hClient
                                , ismENGINE_CREATE_SESSION_OPTION_NONE
                                , &hSession
                                , NULL
                                , 0
                                , NULL);
     TEST_ASSERT_EQUAL(rc, OK);

     char *messageBody = "This is the payload";

     rc = test_createMessage(1+strlen(messageBody),
                             ismMESSAGE_PERSISTENCE_PERSISTENT,
                             ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                             ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                             0,
                             ismDESTINATION_TOPIC, topic,
                             &hMessage, (void **)&messageBody);
     TEST_ASSERT_EQUAL(rc, OK);

     rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                  ismDESTINATION_TOPIC,
                                                  topic,
                                                  NULL,
                                                  hMessage);
     TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

     test_incrementActionsRemaining(pActionsRemaining, 1);
     rc = ism_engine_destroyClientState(hClient,
                                        ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                        &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
     if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
     else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

     test_waitForRemainingActions(pActionsRemaining);
}

bool isRetainedMessage(const char *topic)
{
    ismEngine_ClientStateHandle_t hSubClient;
    ismEngine_SessionHandle_t hSubSession;
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;
    bool foundRetained = false;

    rc = ism_engine_createClientState(__func__,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hSubClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hSubClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSubSession,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSubSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext_t callbackContext;
    callbackContext.hClient = hSubClient;
    callbackContext.hSession = hSubSession;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext.ExpiringMessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSubSession,
                                   ismDESTINATION_TOPIC,
                                   topic,
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //If there were retained messages, by the time we got here they would have been delivered to our consumer
    if (callbackContext.MessageCount > 0)
    {
        foundRetained = true;
    }

    rc = ism_engine_destroyClientState(hSubClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    return foundRetained;
}

void willMessageTestClearRetained(void)
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_MessageHandle_t hMessage;
    int32_t rc = OK;
    char *testTopic="/test/will/clearRetained";

    rc = ism_engine_createClientState("ClientClearRetained",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(0,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE, // Don't want properties
                            0,
                            ismDESTINATION_TOPIC, testTopic,
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    hMessage->Header.Flags = ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN; // We really want a retained message...
    hMessage->Header.MessageType = MTYPE_NullRetained;              // ... and we really want NullRetained

    // Set up the will message so when we disconnect, retained should be cleared
    rc = ism_engine_setWillMessage(hClient,
                                   ismDESTINATION_TOPIC,
                                   testTopic,
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Check there is no retained message on the topic already
    TEST_ASSERT_EQUAL(isRetainedMessage(testTopic), false);


    //Add a retained message that we're going to clear...
    publishRetained(testTopic);

    //Check it's there
    TEST_ASSERT_EQUAL(isRetainedMessage(testTopic), true);

    //Now destroy our client which will publish the will message and therefore clear retained message
    rc = ism_engine_destroySession(hSession,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Check the message has been cleared
    TEST_ASSERT_EQUAL(isRetainedMessage(testTopic), false);
}

void willMessageTestRequestInProgress(void)
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_MessageHandle_t hMessage;
    int32_t rc = OK;
    char *testTopic="/test/will/requestInProgress";

    rc = ism_engine_createClientState("ClientRequestInProgress",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(0,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE, // Don't want properties
                            0,
                            ismDESTINATION_TOPIC, testTopic,
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    hMessage->Header.Flags = ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN; // We really want a retained message...
    hMessage->Header.MessageType = MTYPE_NullRetained;              // ... and we really want NullRetained

    // Fake up an in-progress request
    TEST_ASSERT_EQUAL(hClient->fWillMessageUpdating, false);
    hClient->fWillMessageUpdating = true;

    // Set up the will message so when we disconnect, retained should be cleared
    rc = ism_engine_setWillMessage(hClient,
                                   ismDESTINATION_TOPIC,
                                   testTopic,
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_RequestInProgress);

    rc = ism_engine_unsetWillMessage(hClient, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_RequestInProgress);

    hClient->fWillMessageUpdating = false;

    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Check the message doesn't existhas been cleared
    TEST_ASSERT_EQUAL(isRetainedMessage(testTopic), false);
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
    int32_t rc = ISMRC_OK;
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;

    pCallbackContext->MessageCount++;

    if (pMsgDetails->Expiry != 0)
    {
        pCallbackContext->ExpiringMessageCount++;
    }

    if(state == ismMESSAGE_STATE_RECEIVED ||state == ismMESSAGE_STATE_DELIVERED)
    {

        rc = ism_engine_confirmMessageDelivery(hConsumer->pSession,
                                               NULL,
                                               hDelivery,
                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                               NULL, 0, NULL);

        TEST_ASSERT_CUNIT(((rc == OK)||(rc == ISMRC_AsyncCompletion)),
                          ("rc was %d",rc));
    }
    ism_engine_releaseMessage(hMessage);

    return moreMessagesPlease;
}

/*
 * Test the setting of a will message with security in place
 */
void willMessageTestSecurity(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    ismEngine_MessageHandle_t hMessage;
    int32_t rc = OK;
    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext=NULL;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport->clientID = "SecureClient";

    rc = ism_engine_createClientState(mockTransport->clientID,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, mockContext,
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

    rc = ism_engine_setWillMessage(hClient1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotAuthorized);

    // Add a messaging policy that allows publish on Topic/*
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.WillTopicPolicy\","
                                "\"Name\":\"WillTopicPolicy\","
                                "\"ClientID\":\"SecureClient\","
                                "\"Topic\":\"Topic/*\","
                                "\"ActionList\":\"publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with this context
    rc = test_addPolicyToSecContext(mockContext, "WillTopicPolicy");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClient1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
}

/*
 * Test the setting of a will message applying a max message time to live
 */
void willMessageMaxMessageTimeToLive(void)
{
    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SessionHandle_t hSession1, hSession2;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;
    ism_listener_t *mockListener=NULL, *mockListener2=NULL;
    ism_transport_t *mockTransport=NULL, *mockTransport2=NULL;
    ismSecurity_t *mockContext=NULL, *mockContext2=NULL;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createSecurityContext(&mockListener2,
                                    &mockTransport2,
                                    &mockContext2);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport->clientID = "SecureTTLClient";

    rc = ism_engine_createClientState(mockTransport->clientID,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, mockContext,
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

    mockTransport2->clientID = "SecureTTLClient2";

    rc = ism_engine_createClientState(mockTransport2->clientID,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, mockContext2,
                                      &hClient2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient2,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession2,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSession2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add a messaging policy that allows publish and subscriber on Topic/* and enforces
    // MaxMessageTimeToLive of 2 minutes
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.MaxTTLTopicPolicy\","
                                "\"Name\":\"MaxTTLTopicPolicy\","
                                "\"ClientID\":\"SecureTTLClient*\","
                                "\"Topic\":\"Topic/*\","
                                "\"ActionList\":\"publish,subscribe\","
                                "\"MaxMessageTimeToLive\":120}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with this contexts
    rc = test_addPolicyToSecContext(mockContext, "MaxTTLTopicPolicy");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext2, "MaxTTLTopicPolicy");
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a consumer to receive the will message
    callbackContext_t callbackContext;
    callbackContext.hClient = hClient2;
    callbackContext.hSession = hSession2;
    callbackContext.hConsumer = 0;
    callbackContext.MessageCount = 0;
    callbackContext.ExpiringMessageCount = 0;

    callbackContext_t *pCallbackContext = &callbackContext;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSession2,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   hClient2,
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer,
                                   NULL, 0, NULL);


    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    size_t areaLengths[2] = {0, 13};
    void *areaData[2] = {NULL, "Message body"};

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(((ismEngine_Message_t *)hMessage)->Header.Expiry, 0);

    rc = ism_engine_setWillMessage(hClient1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/Will",
                                   hMessage,
                                   0,
                                   240, // Should get overridden by policy (to 120)
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(hClient1->WillMessageTimeToLive, 120);

    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 1);
    TEST_ASSERT_EQUAL(callbackContext.ExpiringMessageCount, 1);

    rc = ism_engine_destroyClientState(hClient2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener2,
                                     mockTransport2,
                                     mockContext2);
    TEST_ASSERT_EQUAL(rc, OK);
}

extern uint32_t override_clientStateExpiryInterval; // Function that does the overriding is in test_clientState.c

/*
 * Test will message delay
 */
void willMessageTestDelay(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_MessageHandle_t hMessage;
    int32_t rc = OK;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    TEST_ASSERT_EQUAL(override_clientStateExpiryInterval, UINT32_MAX);

    rc = ism_engine_createClientState(__func__,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient1,
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

    // Check that for a non-durable client, the WillDelayExpiryTime matches the ClientState
    // ExpiryTime.
    rc = ism_engine_setWillMessage(hClient1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/DelayWill",
                                   hMessage,
                                   60,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    hClient1 = NULL;
    iecs_findClientState(pThreadData, __func__, false, &hClient1);
    TEST_ASSERT_NOT_EQUAL(hClient1, NULL);
    TEST_ASSERT_NOT_EQUAL(hClient1->WillDelayExpiryTime, 0);
    TEST_ASSERT_EQUAL(hClient1->WillDelayExpiryTime, hClient1->ExpiryTime);

    for(int32_t loop=0; loop<4; loop++)
    {
        uint32_t willDelay;

        // Release the clientState reference picked up by previous iecs_findClientState
        iecs_releaseClientStateReference(pThreadData, hClient1, false, false);

        switch(loop)
        {
            // Go through normal case, will delay < expiry
            case 0:
                override_clientStateExpiryInterval = 30;
                willDelay = override_clientStateExpiryInterval/2;
                break;
            // Go through unusual case, will delay > expiry
            case 1:
                willDelay = override_clientStateExpiryInterval*2;
                break;
            // Go through normal case will delay == expiry
            case 2:
                willDelay = override_clientStateExpiryInterval;
                break;
            // Go through normal case, will delay and NO expiry
            case 3:
                override_clientStateExpiryInterval = UINT32_MAX;
                willDelay = 60;
                break;
            default:
                TEST_ASSERT_GREATER_THAN(0, loop);
                break;
        }

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_createClientState(__func__,
                                          testDEFAULT_PROTOCOL_ID,
                                          ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                          NULL, NULL, NULL,
                                          &hClient1,
                                          &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);

        // First loop won't resume, because client was non-durable
        if ((loop == 0 && rc == OK) || (loop != 0 && rc == ISMRC_ResumedClientState)) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

        test_waitForRemainingActions(pActionsRemaining);

        // Rather than define a callback routine to be called for async creation, just find the client
        // here.
        iecs_findClientState(pThreadData, __func__, false, &hClient1);
        TEST_ASSERT_NOT_EQUAL(hClient1, NULL);
        iecs_releaseClientStateReference(pThreadData, hClient1, false, false);

        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areaData,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_setWillMessage(hClient1,
                                       ismDESTINATION_TOPIC,
                                       "Topic/DelayWill",
                                       hMessage,
                                       willDelay,
                                       iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_destroyClientState(hClient1,
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                           &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

        test_waitForRemainingActions(pActionsRemaining);

        hClient1 = NULL;
        iecs_findClientState(pThreadData, __func__, false, &hClient1);
        TEST_ASSERT_NOT_EQUAL(hClient1, NULL);
        TEST_ASSERT_NOT_EQUAL(hClient1->WillDelayExpiryTime, 0);

        switch(loop)
        {
            case 0:
                TEST_ASSERT_NOT_EQUAL(hClient1->ExpiryTime, 0);
                TEST_ASSERT_GREATER_THAN(hClient1->ExpiryTime, hClient1->WillDelayExpiryTime);
                break;
            case 1:
            case 2:
                TEST_ASSERT_NOT_EQUAL(hClient1->ExpiryTime, 0);
                TEST_ASSERT_EQUAL(hClient1->WillDelayExpiryTime, hClient1->ExpiryTime);
                break;
            case 3:
                TEST_ASSERT_EQUAL(hClient1->ExpiryTime, 0);
                break;
        }
    }

    // CHEAT -- this effectively does what ism_engine_destroyDisconnectedClientState does,
    // note: we still have a reference from the previous iecs_findClientState, so don't increment it
    // after taking the useCountLock (as ism_engine_destroyDisconnectedClientState would).
    pthread_spin_lock(&hClient1->UseCountLock);
    TEST_ASSERT_EQUAL(hClient1->OpState, iecsOpStateZombie);
    hClient1->OpState = iecsOpStateZombieRemoval;
    pthread_spin_unlock(&hClient1->UseCountLock);
    iecs_releaseClientStateReference(pThreadData, hClient1, false, false);
}

/*
 * Test will message delay TakeOver. According to the MQTTv5 Spec:
 * "If a Network Connection uses a Client Identifier of an existing Network Connection to the Server, the
 * Will Message for the exiting connection is sent unless the new connection specifies Clean Start of 0 and
 * the Will Delay is greater than zero. If the Will Delay is 0 the Will Message is sent at the close of the
 * existing Network Connection, and if Clean Start is 1 the Will Message is sent because the Session ends.
 *
 *
 */
static void willMessageTestDelayTakeover_stealCB( int32_t                       reason,
                                                  ismEngine_ClientStateHandle_t hClient,
                                                  uint32_t                      options,
                                                  void                          *pContext )
{
    TEST_ASSERT_EQUAL(reason, ISMRC_ResumedClientState);
    TEST_ASSERT_EQUAL(options, ismENGINE_STEAL_CALLBACK_OPTION_NONE);

    uint32_t *pStealNotifications = (uint32_t *)pContext;

    __sync_add_and_fetch(pStealNotifications, 1);

    int32_t rc = ism_engine_destroyClientState(hClient,
            ismENGINE_DESTROY_CLIENT_OPTION_NONE,
            NULL, 0, NULL);
    if (rc != OK) TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    return;
}

void willMessageTestDelayTakeOver(void)
{
    ismEngine_ClientStateHandle_t hClientInitial;
    ismEngine_ClientStateHandle_t hClientThief;
    ismEngine_MessageHandle_t hMessage;
    int32_t rc = OK;
    uint32_t StealNotifications = 0;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    TEST_ASSERT_EQUAL(override_clientStateExpiryInterval, UINT32_MAX);

    //Create a subscription on the WillMessageTopic so we can see when messages are publishes
    testGetMsgContext_t   gotMsgsContext = {0};
    testMsgsCounts_t msgDetails = {0};
    const char *willMsgReceiverSubName = "WillDelayTakeOverSub";
    const char *willMsgReceiverTopicFilter = "/Topic/WillDelayTakeover/#";

    gotMsgsContext.pMsgCounts = &msgDetails;

    test_createClientAndSession("SubClient", NULL,  ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                ismENGINE_CREATE_SESSION_OPTION_NONE,
                                &(gotMsgsContext.hClient), &(gotMsgsContext.hSession), true);

    test_makeSub( willMsgReceiverSubName
                , willMsgReceiverTopicFilter
                , ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE
                , gotMsgsContext.hClient
                , gotMsgsContext.hClient);

    //Test1: Create a client with will delay > 0 . Create a new client with CleanStart=False and cause a TakeOver
    //       Expect no publication as new client is considered to be the oldclient and therefore not disconnected
    //       long enough for the will day to cause the will message to be sent.

    rc = ism_engine_createClientState(__func__,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      &StealNotifications, willMessageTestDelayTakeover_stealCB, NULL,
                                      &hClientInitial,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(StealNotifications, 0);

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

    rc = ism_engine_setWillMessage(hClientInitial,
                                   ismDESTINATION_TOPIC,
                                   "/Topic/WillDelayTakeover/Test1",
                                   hMessage,
                                   60,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //ClientId steal with a cleanstart=False client
    rc = sync_ism_engine_createClientState(__func__,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      &StealNotifications, willMessageTestDelayTakeover_stealCB, NULL,
                                      &hClientThief);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(StealNotifications, 1);

    //Check no message was published (the client was not disconnected as long as the will delay)
    test_receive( &gotMsgsContext
                , willMsgReceiverSubName
                , 0
                , 0
                , 10); //Wait 10 milliseconds to see if a message turns up

    TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 0);

    //Destroy Sub and Clients
    rc = sync_ism_engine_destroyClientState(hClientThief,
            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(StealNotifications, 1);

    rc = sync_ism_engine_destroyClientState(gotMsgsContext.hClient,
                ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);

    memset(&gotMsgsContext, 0, sizeof(gotMsgsContext));
    memset(&msgDetails, 0, sizeof(msgDetails));
    gotMsgsContext.pMsgCounts = &msgDetails;
    StealNotifications = 0;

    test_createClientAndSession("SubClient", NULL,  ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                ismENGINE_CREATE_SESSION_OPTION_NONE,
                                &(gotMsgsContext.hClient), &(gotMsgsContext.hSession), true);

    test_makeSub( willMsgReceiverSubName
                , willMsgReceiverTopicFilter
                , ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE
                , gotMsgsContext.hClient
                , gotMsgsContext.hClient);

    //Test2: Create a client with will delay > 0 . Create a new client with CleanStart=True and cause a TakeOver
    //       Expect publication as new client is considered to be DIFFERENT to client and old client's session is ended earlt.

    rc = ism_engine_createClientState(__func__,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      &StealNotifications, willMessageTestDelayTakeover_stealCB, NULL,
                                      &hClientInitial,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(StealNotifications, 0);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClientInitial,
                                   ismDESTINATION_TOPIC,
                                   "/Topic/WillDelayTakeover/Test2",
                                   hMessage,
                                   60,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //ClientId steal with a cleanstart=True client
    rc = sync_ism_engine_createClientState(__func__,
                                     testDEFAULT_PROTOCOL_ID,
                                     ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL | ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                     &StealNotifications, willMessageTestDelayTakeover_stealCB, NULL,
                                     &hClientThief);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(StealNotifications, 1);

    //Check no message was published (the client was not disconnected as long as the will delay)
    test_receive( &gotMsgsContext
                , willMsgReceiverSubName
                , 1
                , 0
                , 10); //Wait 10 milliseconds to see if a message turns up

    //As new client is cleanstart - not the same as initial client and hence old session ended early and will should be sent
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 1);

    //Destroy Sub and Clients
    rc = sync_ism_engine_destroyClientState(hClientThief,
           ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(StealNotifications, 1);

    rc = sync_ism_engine_destroyClientState(gotMsgsContext.hClient,
           ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);

    memset(&gotMsgsContext, 0, sizeof(gotMsgsContext));
    memset(&msgDetails, 0, sizeof(msgDetails));
    gotMsgsContext.pMsgCounts = &msgDetails;
    StealNotifications = 0;

    test_createClientAndSession("SubClient", NULL,  ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                ismENGINE_CREATE_SESSION_OPTION_NONE,
                                &(gotMsgsContext.hClient), &(gotMsgsContext.hSession), true);

    test_makeSub( willMsgReceiverSubName
                , willMsgReceiverTopicFilter
                , ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE
                , gotMsgsContext.hClient
                , gotMsgsContext.hClient);

    //Test3: Create a client with will delay = 0 . Create a new client with CleanStart=True and cause a TakeOver
    //       Expect publication (old client is disconnected so will fires)

    rc = ism_engine_createClientState(__func__,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      &StealNotifications, willMessageTestDelayTakeover_stealCB, NULL,
                                      &hClientInitial,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(StealNotifications, 0);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClientInitial,
                                   ismDESTINATION_TOPIC,
                                   "/Topic/WillDelayTakeover/Test3",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //ClientId steal with a cleanstart=True client
    rc = sync_ism_engine_createClientState(__func__,
                                     testDEFAULT_PROTOCOL_ID,
                                     ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL | ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                     &StealNotifications, willMessageTestDelayTakeover_stealCB, NULL,
                                     &hClientThief);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(StealNotifications, 1);

    //Check message was published
    test_receive( &gotMsgsContext
                , willMsgReceiverSubName
                , 1
                , 0
                , 10); //Wait 10 milliseconds to see if a message turns up

    //NO will delay - so expect publication on takeover
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 1);

    //Destroy Sub and Clients
    rc = sync_ism_engine_destroyClientState(hClientThief,
           ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(StealNotifications, 1);

    rc = sync_ism_engine_destroyClientState(gotMsgsContext.hClient,
           ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);

    memset(&gotMsgsContext, 0, sizeof(gotMsgsContext));
    memset(&msgDetails, 0, sizeof(msgDetails));
    gotMsgsContext.pMsgCounts = &msgDetails;
    StealNotifications = 0;

    test_createClientAndSession("SubClient", NULL,  ismENGINE_CREATE_CLIENT_OPTION_NONE,
                               ismENGINE_CREATE_SESSION_OPTION_NONE,
                               &(gotMsgsContext.hClient), &(gotMsgsContext.hSession), true);

    test_makeSub( willMsgReceiverSubName
                , willMsgReceiverTopicFilter
                , ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE
                , gotMsgsContext.hClient
                , gotMsgsContext.hClient);

    //Test4: Create a client with will delay = 0 . Create a new client with CleanStart=False and cause a TakeOver
    //       Expect publication as old client is disconnected and has no will delay

    rc = ism_engine_createClientState(__func__,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      &StealNotifications, willMessageTestDelayTakeover_stealCB, NULL,
                                      &hClientInitial,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(StealNotifications, 0);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClientInitial,
                                   ismDESTINATION_TOPIC,
                                   "/Topic/WillDelayTakeover/Test4",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //ClientId steal with a cleanstart=False client
    rc = sync_ism_engine_createClientState(__func__,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      &StealNotifications, willMessageTestDelayTakeover_stealCB, NULL,
                                      &hClientThief);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(StealNotifications, 1);

    //Check we got a will message published
    test_receive( &gotMsgsContext
                , willMsgReceiverSubName
                , 1
                , 0
                , 10); //Wait 10 milliseconds to see if a message turns up

    //As old client was disconnected with no will delay - expect send of will
    TEST_ASSERT_EQUAL(msgDetails.msgsArrived, 1);

    //Destroy Sub and Clients
    rc = sync_ism_engine_destroyClientState(hClientThief,
            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(StealNotifications, 1);

    rc = sync_ism_engine_destroyClientState(gotMsgsContext.hClient,
            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
}
