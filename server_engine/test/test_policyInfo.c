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
/* Module Name: test_policyInfo.c                                    */
/*                                                                   */
/* Description: Main source file for CUnit test of engine policy     */
/*              information module.                                  */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "engine.h"
#include "engineInternal.h"
#include "policyInfo.h"
#include "admin.h"
#include "queueCommon.h"

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_security.h"
#include "test_utils_config.h"
#include "test_utils_sync.h"

//****************************************************************************
/// @brief Emulate the deletion of a subscription from a config callback
//****************************************************************************
void adminDeleteSub(char *clientId, char *subName)
{
    ism_prop_t *subProps = ism_common_newProperties(2);
    char *objectidentifier = "BOB"; //Unused by engine for delete sub
    char subNamePropName[strlen(ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_SUBSCRIPTIONNAME)+strlen(objectidentifier)+1];
    char clientIdPropName[strlen(ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_CLIENTID)+strlen(objectidentifier)+1];
    ism_field_t f;

    sprintf(subNamePropName,"%s%s", ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_SUBSCRIPTIONNAME,
                                    objectidentifier);
    f.type = VT_String;
    f.val.s = subName;
    ism_common_setProperty(subProps, subNamePropName, &f);

    sprintf(clientIdPropName,"%s%s", ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_CLIENTID,
                                     objectidentifier);
    f.type = VT_String;
    f.val.s = clientId;
    ism_common_setProperty(subProps, clientIdPropName, &f);

    int32_t rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_SUBSCRIPTION,
                                           objectidentifier,
                                           subProps,
                                           ISM_CONFIG_CHANGE_DELETE);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_common_freeProperties(subProps);
}

//****************************************************************************
/// @brief Test Authority checking
//****************************************************************************
char *AUTHCHECKS_TOPICS[] = {"TEST/Authority Checks/TOPIC", "TEST/Authority Checks/+", "TEST/Authority Checks/#"};
#define AUTHCHECKS_NUMTOPICS (sizeof(AUTHCHECKS_TOPICS)/sizeof(AUTHCHECKS_TOPICS[0]))

typedef struct tag_authChecksMessagesCbContext_t
{
    ismEngine_SessionHandle_t hSession;
    int32_t received;
} authChecksMessagesCbContext_t;

bool authChecksMessagesCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
                                void *                          pContext)
{
    authChecksMessagesCbContext_t *context = *((authChecksMessagesCbContext_t **)pContext);

    __sync_fetch_and_add(&context->received, 1);

    ism_engine_releaseMessage(hMessage);

    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        uint32_t rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                        NULL,
                                                        hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                        NULL,
                                                        0,
                                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    return true; // more messages, please.
}

#define AUTHCHECKS_POLICYNAME1 "AuthChecksPolicy1"
#define AUTHCHECKS_POLICYNAME2 "AuthChecksPolicy2"
#define AUTHCHECKS_POLICYNAME3 "AuthChecksPolicy3"

void test_capability_AuthChecks(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient, hClient2;
    ismEngine_SessionHandle_t hSession, hSession2;
    ismEngine_ConsumerHandle_t hConsumer, hConsumerB, hConsumer2;
    ismEngine_ProducerHandle_t hProducer, hProducer2;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    authChecksMessagesCbContext_t Context={0}, Context2={0};
    authChecksMessagesCbContext_t *cb=&Context, *cb2=&Context2;
    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL, *mockTransport2=NULL;
    ismSecurity_t *mockContext, *mockContext2;
    iepiPolicyInfo_t *pPolicyInfo, *pPolicyInfo2, *pDefaultPolicyInfo;

    printf("Starting %s...\n", __func__);

    // Check for the default policy info's existence
    pDefaultPolicyInfo = iepi_getDefaultPolicyInfo(false);
    TEST_ASSERT_PTR_NOT_NULL(pDefaultPolicyInfo);

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "Client1";

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport2,
                                    &mockContext2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport2);
    TEST_ASSERT_PTR_NOT_NULL(mockContext2);

    mockTransport2->clientID = "Client2";

    /* Create our clients and sessions */
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    Context.hSession = hSession;

    rc = test_createClientAndSession(mockTransport2->clientID,
                                     mockContext2,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient2, &hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient2);
    TEST_ASSERT_PTR_NOT_NULL(hSession2);

    Context2.hSession = hSession2;

    printf("  ...test\n");

    // Create a message to play with
    void *payload=NULL;
    ismEngine_MessageHandle_t hMessage;

    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"Name\":\""AUTHCHECKS_POLICYNAME1"\","
                                 "\"ClientID\":\"Client1\","
                                 "\"Topic\":\"TEST/Auth*\","
                                 "\"ActionList\":\"subscribe\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_addPolicyToSecContext(mockContext, AUTHCHECKS_POLICYNAME1);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext2, AUTHCHECKS_POLICYNAME1);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   AUTHCHECKS_TOPICS[rand()%AUTHCHECKS_NUMTOPICS],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &cb,
                                   sizeof(authChecksMessagesCbContext_t *),
                                   authChecksMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);

    pPolicyInfo = ieq_getPolicyInfo(((ismEngine_Consumer_t *)hConsumer)->queueHandle);
    TEST_ASSERT_PTR_NOT_NULL(pPolicyInfo);
    TEST_ASSERT_NOT_EQUAL_FORMAT(pPolicyInfo, pDefaultPolicyInfo, "%p"); // not default
    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, ismMAXIMUM_MESSAGES_DEFAULT);

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   AUTHCHECKS_TOPICS[rand()%AUTHCHECKS_NUMTOPICS],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &cb,
                                   sizeof(authChecksMessagesCbContext_t *),
                                   authChecksMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                   &hConsumerB,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);

    pPolicyInfo2 = ieq_getPolicyInfo(((ismEngine_Consumer_t *)hConsumer)->queueHandle);
    TEST_ASSERT_PTR_NOT_NULL(pPolicyInfo2);
    TEST_ASSERT_EQUAL_FORMAT(pPolicyInfo, pPolicyInfo2,"%p");
    TEST_ASSERT_EQUAL(pPolicyInfo2->maxMessageCount, ismMAXIMUM_MESSAGES_DEFAULT);

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_TOPIC,
                                   AUTHCHECKS_TOPICS[0],
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, AUTHCHECKS_TOPICS[0],
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            AUTHCHECKS_TOPICS[0],
                                            NULL,
                                            hMessage,
                                            NULL, 0, NULL);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
    rc = ism_engine_createConsumer(hSession2,
                                   ismDESTINATION_TOPIC,
                                   AUTHCHECKS_TOPICS[rand()%AUTHCHECKS_NUMTOPICS],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &cb2,
                                   sizeof(authChecksMessagesCbContext_t *),
                                   authChecksMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSession2,
                                   ismDESTINATION_TOPIC,
                                   AUTHCHECKS_TOPICS[0],
                                   &hProducer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, AUTHCHECKS_TOPICS[0],
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage);

    rc = ism_engine_putMessageOnDestination(hSession2,
                                            ismDESTINATION_TOPIC,
                                            AUTHCHECKS_TOPICS[0],
                                            NULL,
                                            hMessage,
                                            NULL, 0, NULL);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    // Add a new policy allowing  Client1 to publish on TEST/Authority Checks/NOT OUR TOPIC only
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID."AUTHCHECKS_POLICYNAME2"\","
                                 "\"Name\":\""AUTHCHECKS_POLICYNAME2"\","
                                 "\"ClientID\":\"Client1\","
                                 "\"Topic\":\"TEST/Authority Checks/NOT OUR TOPIC\","
                                 "\"ActionList\":\"publish\","
                                 "\"MaxMessages\":\"1002\","
                                 "\"MaxMessagesBehavior\":\"RejectNewMessages\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_addPolicyToSecContext(mockContext, AUTHCHECKS_POLICYNAME2);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext2, AUTHCHECKS_POLICYNAME2);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_TOPIC,
                                   AUTHCHECKS_TOPICS[0],
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, AUTHCHECKS_TOPICS[0],
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            AUTHCHECKS_TOPICS[0],
                                            NULL,
                                            hMessage,
                                            NULL, 0, NULL);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
    rc = ism_engine_createConsumer(hSession2,
                                   ismDESTINATION_TOPIC,
                                   AUTHCHECKS_TOPICS[rand()%AUTHCHECKS_NUMTOPICS],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &cb2,
                                   sizeof(authChecksMessagesCbContext_t *),
                                   authChecksMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSession2,
                                   ismDESTINATION_TOPIC,
                                   AUTHCHECKS_TOPICS[0],
                                   &hProducer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, AUTHCHECKS_TOPICS[0],
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage);

    rc = ism_engine_putMessageOnDestination(hSession2,
                                            ismDESTINATION_TOPIC,
                                            AUTHCHECKS_TOPICS[0],
                                            NULL,
                                            hMessage,
                                            NULL, 0, NULL);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    // Add a new policy allowing Client* to publish/subscribe on TEST/Authority Checks/TOPIC only
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID."AUTHCHECKS_POLICYNAME3"\","
                                 "\"Name\":\""AUTHCHECKS_POLICYNAME3"\","
                                 "\"ClientID\":\"Client*\","
                                 "\"Topic\":\"TEST/Authority Checks/TOPIC\","
                                 "\"ActionList\":\"publish,subscribe\","
                                 "\"MaxMessages\":\"1003\","
                                 "\"MaxMessagesBehavior\":\"DiscardOldMessages\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_addPolicyToSecContext(mockContext, AUTHCHECKS_POLICYNAME3);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext2, AUTHCHECKS_POLICYNAME3);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_TOPIC,
                                   AUTHCHECKS_TOPICS[0],
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer);
    TEST_ASSERT_PTR_NULL(((ismEngine_Producer_t *)hProducer)->queueHandle);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, AUTHCHECKS_TOPICS[0],
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            AUTHCHECKS_TOPICS[0],
                                            NULL,
                                            hMessage,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
    rc = ism_engine_createConsumer(hSession2,
                                   ismDESTINATION_TOPIC,
                                   AUTHCHECKS_TOPICS[0],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &cb2,
                                   sizeof(authChecksMessagesCbContext_t *),
                                   authChecksMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer2);
    pPolicyInfo = ieq_getPolicyInfo(((ismEngine_Consumer_t *)hConsumer2)->queueHandle);
    TEST_ASSERT_PTR_NOT_NULL(pPolicyInfo);
    TEST_ASSERT_NOT_EQUAL_FORMAT(pPolicyInfo, pDefaultPolicyInfo, "%p");
    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, 1003);
    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(pPolicyInfo->maxMsgBehavior, DiscardOldMessages);

    rc = ism_engine_createProducer(hSession2,
                                   ismDESTINATION_TOPIC,
                                   AUTHCHECKS_TOPICS[0],
                                   &hProducer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, AUTHCHECKS_TOPICS[0],
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage);

    rc = ism_engine_putMessageOnDestination(hSession2,
                                            ismDESTINATION_TOPIC,
                                            AUTHCHECKS_TOPICS[0],
                                            NULL,
                                            hMessage,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Update an existing policy
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID."AUTHCHECKS_POLICYNAME3"\","
                                 "\"Name\":\""AUTHCHECKS_POLICYNAME3"\","
                                 "\"ClientID\":\"Client*\","
                                 "\"Topic\":\"TEST/Authority Checks/TOPIC\","
                                 "\"ActionList\":\"subscribe\","
                                 "\"MaxMessages\":\"1004\","
                                 "\"MaxMessagesBehavior\":\"RejectNewMessages\","
                                 "\"Update\":\"true\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, AUTHCHECKS_TOPICS[0],
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            AUTHCHECKS_TOPICS[0],
                                            NULL,
                                            hMessage,
                                            NULL, 0, NULL);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    ismEngine_ConsumerHandle_t hConsumerSet[10];

    for(int consumerIndex=0; consumerIndex<10; consumerIndex++)
    {
        int32_t topicIndex = rand()%AUTHCHECKS_NUMTOPICS;
        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
        rc = ism_engine_createConsumer(hSession2,
                                       ismDESTINATION_TOPIC,
                                       AUTHCHECKS_TOPICS[topicIndex],
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &cb2,
                                       sizeof(authChecksMessagesCbContext_t *),
                                       authChecksMessagesCallback,
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                       &hConsumerSet[consumerIndex],
                                       NULL, 0, NULL);
        if (topicIndex == 0)
        {
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hConsumerSet[consumerIndex]);
            pPolicyInfo2 = ieq_getPolicyInfo(((ismEngine_Consumer_t *)hConsumerSet[consumerIndex])->queueHandle);
            TEST_ASSERT_PTR_NOT_NULL(pPolicyInfo2);
            TEST_ASSERT_EQUAL(pPolicyInfo2->maxMessageCount, 1004);
            TEST_ASSERT_EQUAL(pPolicyInfo2->maxMessageBytes, 0);
            TEST_ASSERT_EQUAL(pPolicyInfo2->maxMsgBehavior, RejectNewMessages);
        }
        else
        {
            TEST_ASSERT_NOT_EQUAL(rc, OK);
        }
    }

    // Try to force an override of the MaxMessages for a consumer using consumerProperties
    // which should fail because the policy comes from a defined policy.
    ism_prop_t *consumerProperties = ism_common_newProperties(1);
    ism_field_t f;

    f.type = VT_Integer;
    f.val.i = 1002;
    ism_common_setProperty(consumerProperties, ismENGINE_ADMIN_PROPERTY_MAXMESSAGES, &f);

    ismEngine_ConsumerHandle_t hOverriddenConsumer = NULL;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = ism_engine_createConsumer(hSession2,
                                   ismDESTINATION_TOPIC,
                                   AUTHCHECKS_TOPICS[0],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &cb2,
                                   sizeof(authChecksMessagesCbContext_t *),
                                   authChecksMessagesCallback,
                                   consumerProperties,
                                   ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                   &hOverriddenConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hOverriddenConsumer);
    pPolicyInfo2 = ieq_getPolicyInfo(((ismEngine_Consumer_t *)hOverriddenConsumer)->queueHandle);
    TEST_ASSERT_PTR_NOT_NULL(pPolicyInfo2);
    TEST_ASSERT_EQUAL(pPolicyInfo2->maxMessageCount, 1004);
    TEST_ASSERT_EQUAL(pPolicyInfo2->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(pPolicyInfo2->maxMsgBehavior, RejectNewMessages);

    rc = ism_engine_createProducer(hSession2,
                                   ismDESTINATION_TOPIC,
                                   AUTHCHECKS_TOPICS[0],
                                   &hProducer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, AUTHCHECKS_TOPICS[0],
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage);

    rc = ism_engine_putMessageOnDestination(hSession2,
                                            ismDESTINATION_TOPIC,
                                            AUTHCHECKS_TOPICS[0],
                                            NULL,
                                            hMessage,
                                            NULL, 0, NULL);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    if (payload) free(payload);
}

CU_TestInfo ISM_PolicyInfo_CUnit_test_capability_AuthChecks[] =
{
    { "AuthChecks", test_capability_AuthChecks },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the ability to create anonymous policy infos
//****************************************************************************
void test_capability_AnonymousPolicyInfos(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    iepiPolicyInfo_t *pPolicyInfo=NULL, *pPolicyInfo2=NULL, *pPolicyInfo3=NULL;

    printf("Starting %s...\n", __func__);


    // Anonymous policy info, with no properties - should get default
    rc = iepi_createPolicyInfoFromProperties(pThreadData, NULL, NULL, ismSEC_POLICY_LAST, false, false, &pPolicyInfo);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pPolicyInfo);
    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, ismMAXIMUM_MESSAGES_DEFAULT);

    ism_field_t f;
    ism_prop_t *pProperties = ism_common_newProperties(10);
    TEST_ASSERT_PTR_NOT_NULL(pProperties);

    f.type = VT_String;
    f.val.s = "3333";
    rc = ism_common_setProperty(pProperties,
                                ismENGINE_ADMIN_PROPERTY_MAXMESSAGES,
                                &f);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create policy info with only valid properties
    rc = iepi_createPolicyInfoFromProperties(pThreadData, NULL, pProperties, ismSEC_POLICY_LAST, false, false, &pPolicyInfo2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pPolicyInfo2);
    TEST_ASSERT_EQUAL(pPolicyInfo2->maxMessageCount, 3333);
    TEST_ASSERT_NOT_EQUAL_FORMAT(pPolicyInfo, pPolicyInfo2, "%p");
    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, ismMAXIMUM_MESSAGES_DEFAULT);

    // Update a policy info with different properties
    bool updated = false;
    rc = iepi_updatePolicyInfoFromProperties(pThreadData, pPolicyInfo, NULL, pProperties, &updated);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(updated, true);
    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, 3333);
    TEST_ASSERT_NOT_EQUAL_FORMAT(pPolicyInfo, pPolicyInfo2, "%p");

    // Retry expecting boolean to indicate that policy info hasn't changed
    rc = iepi_updatePolicyInfoFromProperties(pThreadData, pPolicyInfo, NULL, pProperties, &updated);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(updated, false);

    f.type = VT_String;
    f.val.s = "Value";
    rc = ism_common_setProperty(pProperties,
                                "UnknownProperty",
                                &f);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create policy info with unexpected properties
    rc = iepi_createPolicyInfoFromProperties(pThreadData, NULL, pProperties, ismSEC_POLICY_LAST, true, false, &pPolicyInfo3);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pPolicyInfo3);
    TEST_ASSERT_EQUAL(pPolicyInfo3->maxMessageCount, 3333);

    // Remove the MaxMessages property and prove that the value doesn't revert to the default
    // (Using a changing DCN value to prove that something got picked up)
    TEST_ASSERT_EQUAL(pPolicyInfo3->DCNEnabled, false);
    rc = ism_common_setProperty(pProperties,
                                ismENGINE_ADMIN_PROPERTY_MAXMESSAGES,
                                NULL);
    f.type = VT_Boolean;
    f.val.i = 1;
    rc = ism_common_setProperty(pProperties,
                                ismENGINE_ADMIN_PROPERTY_DISCONNECTEDCLIENTNOTIFICATION,
                                &f);

    rc = iepi_updatePolicyInfoFromProperties(pThreadData, pPolicyInfo3, NULL, pProperties, &updated);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(updated, true);
    TEST_ASSERT_EQUAL(pPolicyInfo3->DCNEnabled, true); // DCNEnabled has been picked up
    TEST_ASSERT_EQUAL(pPolicyInfo3->maxMessageCount, 3333); // MaxMessageCount has not reverted to default (5000)

    rc = ism_common_setProperty(pProperties,
                                ismENGINE_ADMIN_PROPERTY_DISCONNECTEDCLIENTNOTIFICATION,
                                NULL);
    rc = iepi_updatePolicyInfoFromProperties(pThreadData, pPolicyInfo3, NULL, pProperties, &updated);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(updated, false);
    TEST_ASSERT_EQUAL(pPolicyInfo3->DCNEnabled, true); // DCNEnabled not reverted to default
    TEST_ASSERT_EQUAL(pPolicyInfo3->maxMessageCount, 3333); // MaxMessageCount has not reverted to default (5000)

    iepi_releasePolicyInfo(pThreadData, pPolicyInfo);
    iepi_releasePolicyInfo(pThreadData, pPolicyInfo2);
    iepi_releasePolicyInfo(pThreadData, pPolicyInfo3);

    ism_common_freeProperties(pProperties);
}

CU_TestInfo ISM_PolicyInfo_CUnit_test_capability_AnonymousPolicyInfos[] =
{
    { "AnonymousPolicyInfos", test_capability_AnonymousPolicyInfos },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Just prove the iepi functions all return correct result
//****************************************************************************
void test_capability_PolicyInfoFuncs(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    // 'DA' functions
    rc = iepi_DA_security_set_policyContext("BLAH",
                                            ismSEC_POLICY_TOPIC,
                                            ISM_CONFIG_COMP_ENGINE,
                                            test_capability_PolicyInfoFuncs);
    TEST_ASSERT_EQUAL(rc, ISMRC_OK);

    iepiPolicyInfo_t *pPolicyInfo = (iepiPolicyInfo_t *)0xDEADBEEF;
    rc = iepi_DA_security_validate_policy(NULL,
                                          ismSEC_AUTH_TOPIC,
                                          "BLAH",
                                          ismSEC_AUTH_ACTION_PUBLISH,
                                          ISM_CONFIG_COMP_ENGINE,
                                          (void **)&pPolicyInfo);
    TEST_ASSERT_EQUAL(rc, ISMRC_OK);
    TEST_ASSERT_EQUAL(pPolicyInfo, NULL);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    ism_prop_t *changedProps = ism_common_newProperties(2);
    char *objectidentifier = "BOB"; //Unused by engine for delete sub
    char namePropName[strlen(ismENGINE_ADMIN_PREFIX_TOPICPOLICY_NAME)+strlen(objectidentifier)+1];
    ism_field_t f;

    sprintf(namePropName,"%s%s", ismENGINE_ADMIN_PREFIX_TOPICPOLICY_NAME,
                                 objectidentifier);
    f.type = VT_String;
    f.val.s = objectidentifier;
    ism_common_setProperty(changedProps, namePropName, &f);

    // Bad callback type
    rc = iepi_policyInfoConfigCallback(pThreadData,
                                       ismENGINE_ADMIN_VALUE_TOPICPOLICY, 0,
                                       objectidentifier, changedProps, 999);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);

    // CHANGE_DELETE Config callback with UUID but no context
    rc = iepi_policyInfoConfigCallback(pThreadData,
                                       ismENGINE_ADMIN_VALUE_TOPICPOLICY, 0,
                                       objectidentifier, changedProps, ISM_CONFIG_CHANGE_DELETE);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    ism_common_freeProperties(changedProps);
}

//****************************************************************************
/// @brief Test dynamic update of policies
//****************************************************************************
#define DYNAMIC_POLICYNAME1 "DynamicPolicy1"
#define DYNAMIC_POLICYNAME2 "DynamicPolicy2"
void test_capability_DynamicPolicyUpdates(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient, hClient2;
    ismEngine_SessionHandle_t hSession, hSession2;
    ismEngine_ConsumerHandle_t hConsumer, hConsumer2;
    ismEngine_ProducerHandle_t hProducer;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL, *mockTransport2=NULL;
    ismSecurity_t *mockContext, *mockContext2;
    iepiPolicyInfo_t *pPolicyInfo, *pPolicyInfo2, *pPolicyInfoProd;

    printf("Starting %s...\n", __func__);

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "Client1";

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport2,
                                    &mockContext2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport2);
    TEST_ASSERT_PTR_NOT_NULL(mockContext2);

    mockTransport2->clientID = "Client2";

    /* Create our clients and sessions */
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    rc = test_createClientAndSession(mockTransport2->clientID,
                                     mockContext2,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient2, &hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient2);
    TEST_ASSERT_PTR_NOT_NULL(hSession2);

    printf("  ...test\n");

    // Add a messaging policy that allows Client* to subscribe or publish on TEST/Dynamic* with
    // MaxMessages set to 10.
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID."DYNAMIC_POLICYNAME1"\","
                                 "\"Name\":\""DYNAMIC_POLICYNAME1"\","
                                 "\"ClientID\":\"Client*\","
                                 "\"Topic\":\"TEST/Dynamic*\","
                                 "\"MaxMessages\":10,"
                                 "\"ActionList\":\"subscribe,publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_addPolicyToSecContext(mockContext, DYNAMIC_POLICYNAME1);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext2, DYNAMIC_POLICYNAME1);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add a messaging policy that shouldn't be relevant to this test.
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID."DYNAMIC_POLICYNAME2"\","
                                 "\"Name\":\""DYNAMIC_POLICYNAME2"\","
                                 "\"ClientID\":\"SomeoneElse*\","
                                 "\"Topic\":\"TEST/Dynamic*\","
                                 "\"ActionList\":\"subscribe,publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_addPolicyToSecContext(mockContext, DYNAMIC_POLICYNAME2);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext2, DYNAMIC_POLICYNAME2);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = ism_engine_createConsumer(hSession2,
                                   ismDESTINATION_TOPIC,
                                   "TEST/DynamicPolicyUpdates",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);

    pPolicyInfo = ieq_getPolicyInfo(((ismEngine_Consumer_t *)hConsumer)->queueHandle);
    TEST_ASSERT_PTR_NOT_NULL(pPolicyInfo);
    TEST_ASSERT_EQUAL(pPolicyInfo->allowSend, true);
    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, 10);
    TEST_ASSERT_EQUAL(pPolicyInfo->useCount, 2); // 'Admin' + 1

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "TEST/DynamicAsteroids",
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    pPolicyInfoProd = ((ismEngine_Producer_t *)hProducer)->pPolicyInfo;
    TEST_ASSERT_PTR_NOT_NULL(pPolicyInfoProd);
    TEST_ASSERT_EQUAL_FORMAT(pPolicyInfo, pPolicyInfoProd, "%p");
    TEST_ASSERT_EQUAL(pPolicyInfoProd->allowSend, true);
    TEST_ASSERT_EQUAL(pPolicyInfoProd->maxMessageTimeToLive, 0);
    TEST_ASSERT_EQUAL(pPolicyInfoProd->useCount, 3); // 'Admin' + 2

    // Update the properties in the policy
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID."DYNAMIC_POLICYNAME1"\","
                                 "\"Name\":\""DYNAMIC_POLICYNAME1"\","
                                 "\"ClientID\":\"Client*\","
                                 "\"Topic\":\"TEST/Dynamic*\","
                                 "\"MaxMessages\":20,"
                                 "\"MaxMessageTimeToLive\":60,"
                                 "\"ActionList\":\"subscribe,publish\","
                                 "\"Update\":\"true\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(pPolicyInfo, ieq_getPolicyInfo(((ismEngine_Consumer_t *)hConsumer)->queueHandle));
    TEST_ASSERT_EQUAL(pPolicyInfoProd, ((ismEngine_Producer_t *)hProducer)->pPolicyInfo);

    TEST_ASSERT_EQUAL(pPolicyInfo->allowSend, true);
    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, 20);
    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageTimeToLive, 60);

    // Create a new consumer
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "TEST/DynamicPolicyUpdates/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer2);

    pPolicyInfo2 = ieq_getPolicyInfo(((ismEngine_Consumer_t *)hConsumer2)->queueHandle);
    TEST_ASSERT_PTR_NOT_NULL(pPolicyInfo2);
    TEST_ASSERT_EQUAL_FORMAT(pPolicyInfo, pPolicyInfo2, "%p");
    TEST_ASSERT_EQUAL(pPolicyInfo2->allowSend, true);
    TEST_ASSERT_EQUAL(pPolicyInfo2->maxMessageCount, 20);
    TEST_ASSERT_EQUAL(pPolicyInfo2->useCount, 4); // 'Admin' + 3

    // Check subscription monitor
    ismEngine_SubscriptionMonitor_t *results = NULL;
    uint32_t resultCount;

    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                           10,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 2);
    TEST_ASSERT_PTR_NOT_NULL(results);
    for(int32_t i=0; i<resultCount; i++)
    {
        TEST_ASSERT_STRINGS_EQUAL(results[i].messagingPolicyName, DYNAMIC_POLICYNAME1);
        TEST_ASSERT_EQUAL(results[i].policyType, (uint32_t)ismSEC_POLICY_TOPIC);
    }

    ism_engine_freeSubscriptionMonitor(results);

    // Check filtering by policy name
    ism_prop_t *filterPolicyName = ism_common_newProperties(1);
    ism_field_t f;
    f.type = VT_String;
    f.val.s = DYNAMIC_POLICYNAME1;
    ism_common_setProperty(filterPolicyName, ismENGINE_MONITOR_FILTER_MESSAGINGPOLICY, &f);

    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_LOWEST_BUFFEREDPERCENT,
                                           10,
                                           filterPolicyName);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 2);

    ism_engine_freeSubscriptionMonitor(results);

    f.val.s = DYNAMIC_POLICYNAME2;
    ism_common_setProperty(filterPolicyName, ismENGINE_MONITOR_FILTER_MESSAGINGPOLICY, &f);

    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_LOWEST_BUFFEREDPERCENT,
                                           10,
                                           filterPolicyName);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);

    // Destroy one of the consumers and check the use count on the policy
    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(pPolicyInfo2->useCount, 3); // 'Admin' + 2

    TEST_ASSERT_EQUAL(pPolicyInfo2->creationState, CreatedByConfig);

    // Delete the policy - it should still be addressable but use count will be reduced
    rc = test_deleteSecurityPolicy("UID."DYNAMIC_POLICYNAME1, DYNAMIC_POLICYNAME1);
    // Expecting the delete request to report OK but for it now to have CreatedByEngine state
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(pPolicyInfo2->useCount, 2);
    TEST_ASSERT_EQUAL(pPolicyInfo2->allowSend, true);
    TEST_ASSERT_EQUAL(pPolicyInfo2->maxMessageCount, 20);
    TEST_ASSERT_EQUAL(pPolicyInfo2->creationState, CreatedByEngine);

    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                           10,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 1);
    TEST_ASSERT_PTR_NOT_NULL(results);
    TEST_ASSERT_STRINGS_EQUAL(results[0].messagingPolicyName, DYNAMIC_POLICYNAME1);
    TEST_ASSERT_EQUAL(results[0].policyType, (uint32_t)ismSEC_POLICY_TOPIC);

    // TODO: This is a deleted policy... should we indicate that in some way?

    ism_engine_freeSubscriptionMonitor(results);

    printf("  ...disconnect\n");

    rc = ism_engine_destroyConsumer(hConsumer2, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(pPolicyInfo2->useCount, 1);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(NULL,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient2, hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport2,
                                     mockContext2);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_deleteSecurityPolicy("UID."DYNAMIC_POLICYNAME2, DYNAMIC_POLICYNAME2);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test deletion of a pending delete policy via a config callback
//****************************************************************************
#define DYNAMIC_POLICYNAME3 "DynamicPolicy3"
void test_capability_PolicyDeletionViaConfigCallback(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    printf("Starting %s...\n", __func__);

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "Client1";

    /* Create our clients and sessions */
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    printf("  ...test\n");

    // Add a messaging policy that allows Client* to subscribe or publish on TEST/DeferredDeletion* with
    // MaxMessages set to 10.
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID."DYNAMIC_POLICYNAME3"\","
                                 "\"Name\":\""DYNAMIC_POLICYNAME3"\","
                                 "\"ClientID\":\"Client*\","
                                 "\"Topic\":\"TEST/DeferredDeletion*\","
                                 "\"MaxMessages\":10,"
                                 "\"ActionList\":\"subscribe,publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_addPolicyToSecContext(mockContext, DYNAMIC_POLICYNAME3);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
    rc = sync_ism_engine_createSubscription(hClient,
                                            "DEFDELSUB",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "TEST/DeferredDeletion",
                                            &subAttrs,
                                            hClient);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_deleteSecurityPolicy("UID."DYNAMIC_POLICYNAME3, DYNAMIC_POLICYNAME3);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    // Delete the subscription administratively, to prompt the deletion of the policy
    adminDeleteSub("Client1", "DEFDELSUB");

    // Give the timer task half a second to actually happen
    usleep(500000);
}

CU_TestInfo ISM_PolicyInfo_CUnit_test_capability_DynamicPolicyUpdates[] =
{
    { "PolicyInfoFuncs", test_capability_PolicyInfoFuncs },
    { "DynamicPolicyUpdates", test_capability_DynamicPolicyUpdates },
    { "DeletionViaConfigCallback", test_capability_PolicyDeletionViaConfigCallback },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test interaction between the admin component and engine
//****************************************************************************
void test_capability_AdminInteractions(void)
{
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    // Try a badly formed Topic policy creation
    rc = test_configProcessPost("{\"TopicPolicy\":{\"CREATED_TOPIC_POLICY\":{\"Name\":\"CREATED_TOPIC_POLICY\"}}}");
    TEST_ASSERT_EQUAL(rc, ISMRC_PropertyRequired);

    // Now create a good Topic policy
    rc = test_configProcessPost("{\"TopicPolicy\":{\"CREATED_TOPIC_POLICY\":{\"Topic\":\"TESTING/TOPIC\","
                                                                            "\"ActionList\":\"Publish,Subscribe\","
                                                                            "\"ClientID\":\"*\""
                                                                           "}}}");
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_PolicyInfo_Cunit_test_capability_AdminInteractions[] =
{
    { "AdminInteractions", test_capability_AdminInteractions },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initPolicyInfo(void)
{
    return test_engineInit(true, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int initPolicyInfoAuthChecks(void)
{
    return test_engineInit(true, false,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termPolicyInfo(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_PolicyInfo_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("AuthChecks", initPolicyInfoAuthChecks, termPolicyInfo, ISM_PolicyInfo_CUnit_test_capability_AuthChecks),
    IMA_TEST_SUITE("AnonymousPolicyInfos", initPolicyInfo, termPolicyInfo, ISM_PolicyInfo_CUnit_test_capability_AnonymousPolicyInfos),
    IMA_TEST_SUITE("DynamicPolicyUpdates", initPolicyInfoAuthChecks, termPolicyInfo, ISM_PolicyInfo_CUnit_test_capability_DynamicPolicyUpdates),
    IMA_TEST_SUITE("AdminInteractions", initPolicyInfo, termPolicyInfo, ISM_PolicyInfo_Cunit_test_capability_AdminInteractions),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_PolicyInfo_CUnit_phase1suites };

int main(int argc, char *argv[])
{
    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
