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
/* Module Name: test_disconnectedClientNotification.c                */
/*                                                                   */
/* Description: Main source for disconnected client notification     */
/*              tests.                                               */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "topicTree.h"
#include "topicTreeInternal.h"
#include "memHandler.h"
#include "queueCommon.h"
#include "selector.h"
#include "multiConsumerQ.h"     // access to the iemq_ function
#include "engineStore.h"        // access to the iest_ constants
#include "clientState.h"        // access to the iecs_ functions

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_assert.h"
#include "test_utils_security.h"
#include "test_utils_ismlogs.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

bool fullTest = false;

//****************************************************************************
/// @brief callback function used to accept an asynchronous operation
//****************************************************************************
static void test_asyncCallback(int32_t rc, void *handle, void *pContext)
{
   uint32_t *callbacksReceived = *(uint32_t **)pContext;
   TEST_ASSERT_EQUAL(rc, OK);
   TEST_ASSERT_PTR_NULL(handle);
   __sync_fetch_and_add(callbacksReceived, 1);
}

//****************************************************************************
/// @brief Wait for some number of operations to complete
//****************************************************************************
static void test_waitForAsyncCallbacks(uint32_t *pCallbacksReceived, uint32_t expectedCallbacks)
{
    while((volatile uint32_t)*pCallbacksReceived < expectedCallbacks)
    {
        usleep(10);
    }
}

//****************************************************************************
/// @brief Test that an IMA120 client state is migrated
//****************************************************************************
void test_storeIMA120ClientState(const char *clientId)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismStore_Record_t storeRecord;
    iestClientStateRecord_V1_t CSR;
    iestClientPropertiesRecord_V2_t CPR;
    ismStore_Handle_t hStoreClientProps;
    ismStore_Handle_t hStoreClientState;
    uint32_t dataLength;
    uint32_t fragsCount;
    char *fragments[2];
    uint32_t fragmentLengths[2];
    int32_t rc = OK;

    // First fragment is always the CPR itself
    fragments[0] = (char *)&CPR;
    fragmentLengths[0] = sizeof(CPR);

    // Fill in the store record
    storeRecord.Type = ISM_STORE_RECTYPE_CPROP;
    storeRecord.Attribute = ismSTORE_NULL_HANDLE; // No WILL message
    storeRecord.State = iestCPR_STATE_NONE;
    storeRecord.pFrags = fragments;
    storeRecord.pFragsLengths = fragmentLengths;
    storeRecord.FragsCount = 1;
    storeRecord.DataLength = fragmentLengths[0];

    // Build the subscription properties record from the various fragment sources
    memcpy(CPR.StrucId, iestCLIENT_PROPS_RECORD_STRUCID, 4);
    CPR.Version = iestCPR_VERSION_2;
    CPR.Flags = iestCPR_FLAGS_DURABLE;
    CPR.WillTopicNameLength = 0;
    CPR.UserIdLength = 0;

    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreClientProps);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hStoreClientProps, ismSTORE_NULL_HANDLE);

    // Now create a CSR
    fragsCount = 1;
    fragments[0] = (char *)&CSR;
    fragmentLengths[0] = sizeof(CSR);
    dataLength = fragmentLengths[0];

    fragsCount++;
    fragments[1] = (char *)clientId;
    fragmentLengths[1] = strlen(clientId) + 1;
    dataLength += fragmentLengths[1];

    memcpy(CSR.StrucId, iestCLIENT_STATE_RECORD_STRUCID, 4);
    CSR.Version = iestCSR_VERSION_1;
    CSR.Flags = iestCSR_FLAGS_DURABLE;
    CSR.ClientIdLength = fragmentLengths[1];

    storeRecord.Type = ISM_STORE_RECTYPE_CLIENT;
    storeRecord.FragsCount = fragsCount;
    storeRecord.DataLength = dataLength;
    storeRecord.Attribute = hStoreClientProps;
    storeRecord.State = iestCSR_STATE_NONE;

    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreClientState);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hStoreClientState, ismSTORE_NULL_HANDLE);

    rc = ism_store_commit(pThreadData->hStream);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test that an IMA110 client state is migrated
//****************************************************************************
void test_storeIMA110ClientState(const char *clientId)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismStore_Record_t storeRecord;
    iestClientStateRecord_V1_t CSR;
    iestClientPropertiesRecord_V1_t CPR;
    ismStore_Handle_t hStoreClientProps;
    ismStore_Handle_t hStoreClientState;
    uint32_t dataLength;
    uint32_t fragsCount;
    char *fragments[2];
    uint32_t fragmentLengths[2];
    int32_t rc = OK;

    // First fragment is always the CPR itself
    fragments[0] = (char *)&CPR;
    fragmentLengths[0] = sizeof(CPR);

    // Fill in the store record
    storeRecord.Type = ISM_STORE_RECTYPE_CPROP;
    storeRecord.Attribute = ismSTORE_NULL_HANDLE; // No WILL message
    storeRecord.State = iestCPR_STATE_NONE;
    storeRecord.pFrags = fragments;
    storeRecord.pFragsLengths = fragmentLengths;
    storeRecord.FragsCount = 1;
    storeRecord.DataLength = fragmentLengths[0];

    // Build the subscription properties record from the various fragment sources
    memcpy(CPR.StrucId, iestCLIENT_PROPS_RECORD_STRUCID, 4);
    CPR.Version = iestCPR_VERSION_1;
    CPR.Flags = iestCPR_FLAGS_DURABLE;
    CPR.WillTopicNameLength = 0;

    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreClientProps);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hStoreClientProps, ismSTORE_NULL_HANDLE);

    // Now create a CSR
    fragsCount = 1;
    fragments[0] = (char *)&CSR;
    fragmentLengths[0] = sizeof(CSR);
    dataLength = fragmentLengths[0];

    fragsCount++;
    fragments[1] = (char *)clientId;
    fragmentLengths[1] = strlen(clientId) + 1;
    dataLength += fragmentLengths[1];

    memcpy(CSR.StrucId, iestCLIENT_STATE_RECORD_STRUCID, 4);
    CSR.Version = iestCSR_VERSION_1;
    CSR.Flags = iestCSR_FLAGS_DURABLE;
    CSR.ClientIdLength = fragmentLengths[1];

    storeRecord.Type = ISM_STORE_RECTYPE_CLIENT;
    storeRecord.FragsCount = fragsCount;
    storeRecord.DataLength = dataLength;
    storeRecord.Attribute = hStoreClientProps;
    storeRecord.State = iestCSR_STATE_NONE;

    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreClientState);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hStoreClientState, ismSTORE_NULL_HANDLE);

    rc = ism_store_commit(pThreadData->hStream);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test that an IMA100 client state is migrated
//****************************************************************************
void test_storeIMA100ClientState(const char *clientId,
                                 const char *willTopicName,
                                 ismStore_Handle_t *hStoreClientState,
                                 ismStore_Handle_t *hStoreClientProps)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismStore_Record_t storeRecord;
    iestClientStateRecord_V1_t CSR;
    iestClientPropertiesRecord_V1_t CPR;
    *hStoreClientProps = ismSTORE_NULL_HANDLE;
    *hStoreClientState = ismSTORE_NULL_HANDLE;

    char *fragments[2]; // Up to 2 fragments for client properties
    uint32_t fragmentLengths[2];

    // Only write a CPR if we have been given a will topic
    if (willTopicName)
    {
        // First fragment is always the CPR itself
        fragments[0] = (char *)&CPR;
        fragmentLengths[0] = sizeof(CPR);

        // Fill in the store record
        storeRecord.Type = ISM_STORE_RECTYPE_CPROP;
        storeRecord.Attribute = 0; // No will message
        storeRecord.State = iestCPR_STATE_NONE;
        storeRecord.pFrags = fragments;
        storeRecord.pFragsLengths = fragmentLengths;
        storeRecord.FragsCount = 1;
        storeRecord.DataLength = fragmentLengths[0];

        memcpy(CPR.StrucId, iestCLIENT_PROPS_RECORD_STRUCID, 4);
        CPR.Version = iestCPR_VERSION_1;

        CPR.WillTopicNameLength = (uint32_t)strlen(willTopicName)+1;
        fragments[storeRecord.FragsCount] = (char *)willTopicName;
        fragmentLengths[storeRecord.FragsCount] = CPR.WillTopicNameLength;
        storeRecord.DataLength += fragmentLengths[storeRecord.FragsCount];
        storeRecord.FragsCount += 1;

        // Add to the store
        rc = ism_store_createRecord(pThreadData->hStream,
                                    &storeRecord,
                                    hStoreClientProps);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Build the CSR from the various fragment sources
    fragments[0] = (char *)&CSR;
    fragmentLengths[0] = sizeof(CSR);

    // Fill in the store record
    storeRecord.Type = ISM_STORE_RECTYPE_CLIENT;
    storeRecord.Attribute = *hStoreClientProps;
    storeRecord.State = iestCSR_STATE_DISCONNECTED;
    storeRecord.pFrags = fragments;
    storeRecord.pFragsLengths = fragmentLengths;
    storeRecord.FragsCount = 1;
    storeRecord.DataLength = fragmentLengths[0];

    memcpy(CSR.StrucId, iestCLIENT_STATE_RECORD_STRUCID, 4);
    CSR.Version = iestCSR_VERSION_1;
    CSR.Flags = iestCSR_FLAGS_DURABLE;

    CSR.ClientIdLength = (uint32_t)strlen(clientId)+1;
    fragments[storeRecord.FragsCount] = (char *)clientId;
    fragmentLengths[storeRecord.FragsCount] = CSR.ClientIdLength;
    storeRecord.DataLength += fragmentLengths[storeRecord.FragsCount];
    storeRecord.FragsCount += 1;

    // Add to the store
    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                hStoreClientState);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_store_commit(pThreadData->hStream);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_storeV1Subscription(const char *topicString,
                              const char *subName,
                              uint32_t subOptions,
                              const char *clientId)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismStore_Record_t storeRecord;
    iestSubscriptionDefinitionRecord_t SDR;
    iestSubscriptionPropertiesRecord_V1_t SPR;
    ismStore_Handle_t hStoreSubscProps;
    ismStore_Handle_t hStoreSubscDefn;

    char *fragments[5]; // Up to 5 fragments for subscription properties
    uint32_t fragmentLengths[5];

    // First fragment is always the SPR itself
    fragments[0] = (char *)&SPR;
    fragmentLengths[0] = sizeof(SPR);

    // Fill in the store record
    storeRecord.Type = ISM_STORE_RECTYPE_SPROP;
    storeRecord.Attribute = 0;
    storeRecord.State = iestSPR_STATE_NONE;
    storeRecord.pFrags = fragments;
    storeRecord.pFragsLengths = fragmentLengths;
    storeRecord.FragsCount = 1;
    storeRecord.DataLength = fragmentLengths[0];

    SPR.MaxMessages = 5000;
    memcpy(SPR.StrucId, iestSUBSC_PROPS_RECORD_STRUCID, 4);
    SPR.Version = iestSPR_VERSION_1;
    SPR.SubOptions = subOptions & ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK;
    SPR.InternalAttrs = iettSUBATTR_PERSISTENT;

    SPR.ClientIdLength = (uint32_t)strlen(clientId)+1;
    fragments[storeRecord.FragsCount] = (char *)clientId;
    fragmentLengths[storeRecord.FragsCount] = SPR.ClientIdLength;
    storeRecord.DataLength += fragmentLengths[storeRecord.FragsCount];
    storeRecord.FragsCount += 1;

    SPR.SubNameLength = (uint32_t)strlen(subName)+1;
    fragments[storeRecord.FragsCount] = (char *)subName;
    fragmentLengths[storeRecord.FragsCount] = SPR.SubNameLength;
    storeRecord.DataLength += fragmentLengths[storeRecord.FragsCount];
    storeRecord.FragsCount += 1;

    // If the topic string is blank ("") then there is no fragment to store
    SPR.TopicStringLength = (uint32_t)strlen(topicString)+1;
    fragments[storeRecord.FragsCount] = (char *)topicString;
    fragmentLengths[storeRecord.FragsCount] = SPR.TopicStringLength;
    storeRecord.DataLength += fragmentLengths[storeRecord.FragsCount];
    storeRecord.FragsCount += 1;

    SPR.SubPropertiesLength = 0;

    // Add to the store
    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreSubscProps);
    TEST_ASSERT_EQUAL(rc, OK);

    ismQueueType_t queueType;

    // Transaction capable and shared subscriptions require a multiConsumer queue,
    // otherwise the type of queue to use is determined by the requested reliability.
    if (subOptions & ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE)
    {
        queueType = multiConsumer;
    }
    else
    {
        queueType = intermediate;
    }

    // Build the subscription definition record from the various fragment sources
    memcpy(SDR.StrucId, iestSUBSC_DEFN_RECORD_STRUCID, 4);
    SDR.Version = iestSDR_VERSION_1;
    SDR.Type = queueType;

    fragments[0] = (char *)&SDR;
    fragmentLengths[0] = sizeof(SDR);

    // Fill in the store record and contents of the subscription record
    storeRecord.Type = ISM_STORE_RECTYPE_SUBSC;
    storeRecord.Attribute = hStoreSubscProps;
    storeRecord.State = iestSDR_STATE_NONE;
    storeRecord.pFrags = fragments;
    storeRecord.pFragsLengths = fragmentLengths;
    storeRecord.FragsCount = 1;
    storeRecord.DataLength = fragmentLengths[0];

    // Add to the store
    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreSubscDefn);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_store_commit(pThreadData->hStream);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test that an IMA15a (v3) client state is migrated
//****************************************************************************
void test_storeIMA15aV3ClientState(const char *clientId, uint32_t protocolId)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismStore_Record_t storeRecord;
    iestClientStateRecord_t CSR;
    iestClientPropertiesRecord_V3_t CPR;
    ismStore_Handle_t hStoreClientProps;
    ismStore_Handle_t hStoreClientState;
    uint32_t dataLength;
    uint32_t fragsCount;
    char *fragments[2];
    uint32_t fragmentLengths[2];
    int32_t rc = OK;

    // First fragment is always the CPR itself
    fragments[0] = (char *)&CPR;
    fragmentLengths[0] = sizeof(CPR);

    // Fill in the store record
    storeRecord.Type = ISM_STORE_RECTYPE_CPROP;
    storeRecord.Attribute = ismSTORE_NULL_HANDLE; // No WILL message
    storeRecord.State = iestCPR_STATE_NONE;
    storeRecord.pFrags = fragments;
    storeRecord.pFragsLengths = fragmentLengths;
    storeRecord.FragsCount = 1;
    storeRecord.DataLength = fragmentLengths[0];

    // Build the subscription properties record from the various fragment sources
    memcpy(CPR.StrucId, iestCLIENT_PROPS_RECORD_STRUCID, 4);
    CPR.Version = iestCPR_VERSION_3;
    CPR.Flags = iestCPR_FLAGS_DURABLE;
    CPR.WillTopicNameLength = 0;
    CPR.UserIdLength = 0;
    CPR.WillMsgTimeToLive = 50;

    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreClientProps);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hStoreClientProps, ismSTORE_NULL_HANDLE);

    // Now create a CSR
    fragsCount = 1;
    fragments[0] = (char *)&CSR;
    fragmentLengths[0] = sizeof(CSR);
    dataLength = fragmentLengths[0];

    fragsCount++;
    fragments[1] = (char *)clientId;
    fragmentLengths[1] = strlen(clientId) + 1;
    dataLength += fragmentLengths[1];

    memcpy(CSR.StrucId, iestCLIENT_STATE_RECORD_STRUCID, 4);
    CSR.Version = iestCSR_VERSION_2;
    CSR.Flags = iestCSR_FLAGS_DURABLE;
    CSR.ClientIdLength = fragmentLengths[1];
    CSR.protocolId = protocolId;

    storeRecord.Type = ISM_STORE_RECTYPE_CLIENT;
    storeRecord.FragsCount = fragsCount;
    storeRecord.DataLength = dataLength;
    storeRecord.Attribute = hStoreClientProps;
    storeRecord.State = iestCSR_STATE_NONE;

    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreClientState);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hStoreClientState, ismSTORE_NULL_HANDLE);

    rc = ism_store_commit(pThreadData->hStream);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test that a v4 client state can be rehydrated
//****************************************************************************
void test_storeIMA15aV4ClientState(const char *clientId,
                                   const char *willTopic,
                                   uint32_t protocolId)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismStore_Record_t storeRecord;
    iestClientStateRecord_t CSR;
    iestClientPropertiesRecord_V4_t CPR;
    ismStore_Handle_t hStoreClientProps;
    ismStore_Handle_t hStoreClientState;
    uint32_t dataLength;
    uint32_t fragsCount;
    char *fragments[2];
    uint32_t fragmentLengths[2];
    int32_t rc = OK;

    // First fragment is always the CPR itself
    fragments[0] = (char *)&CPR;
    fragmentLengths[0] = sizeof(CPR);

    // Fill in the store record
    storeRecord.Type = ISM_STORE_RECTYPE_CPROP;
    storeRecord.Attribute = ismSTORE_NULL_HANDLE; // No WILL message
    storeRecord.State = iestCPR_STATE_NONE;
    storeRecord.pFrags = fragments;
    storeRecord.pFragsLengths = fragmentLengths;
    storeRecord.FragsCount = 1;
    storeRecord.DataLength = fragmentLengths[0];

    // Build the subscription properties record from the various fragment sources
    memcpy(CPR.StrucId, iestCLIENT_PROPS_RECORD_STRUCID, 4);
    CPR.Version = iestCPR_VERSION_5;
    CPR.Flags = iestCPR_FLAGS_DURABLE;
    if (willTopic != NULL)
    {
        CPR.WillTopicNameLength = strlen(willTopic)+1;
        fragments[storeRecord.FragsCount] = (char *)willTopic;
        fragmentLengths[storeRecord.FragsCount] = CPR.WillTopicNameLength;
        storeRecord.DataLength += fragmentLengths[storeRecord.FragsCount];
        storeRecord.FragsCount += 1;
    }
    else
    {
        CPR.WillTopicNameLength = 0;
    }
    CPR.UserIdLength = 0;
    CPR.WillMsgTimeToLive = 50;
    CPR.ExpiryInterval = iecsEXPIRY_INTERVAL_INFINITE;

    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreClientProps);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hStoreClientProps, ismSTORE_NULL_HANDLE);

    // Now create a CSR
    fragsCount = 1;
    fragments[0] = (char *)&CSR;
    fragmentLengths[0] = sizeof(CSR);
    dataLength = fragmentLengths[0];

    fragsCount++;
    fragments[1] = (char *)clientId;
    fragmentLengths[1] = strlen(clientId) + 1;
    dataLength += fragmentLengths[1];

    memcpy(CSR.StrucId, iestCLIENT_STATE_RECORD_STRUCID, 4);
    CSR.Version = iestCSR_VERSION_2;
    CSR.Flags = iestCSR_FLAGS_DURABLE;
    CSR.ClientIdLength = fragmentLengths[1];
    CSR.protocolId = protocolId;

    storeRecord.Type = ISM_STORE_RECTYPE_CLIENT;
    storeRecord.FragsCount = fragsCount;
    storeRecord.DataLength = dataLength;
    storeRecord.Attribute = hStoreClientProps;
    storeRecord.State = iestCSR_STATE_NONE;

    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreClientState);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hStoreClientState, ismSTORE_NULL_HANDLE);

    rc = ism_store_commit(pThreadData->hStream);
    TEST_ASSERT_EQUAL(rc, OK);
}

typedef struct tag_msgCallbackContext_t
{
    ismEngine_SessionHandle_t hSession;
    uint32_t received;
} msgCallbackContext_t;

bool test_msgCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
    msgCallbackContext_t *context = *((msgCallbackContext_t **)pContext);

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

void test_publishMessages(const char *topicString, uint32_t msgCount)
{
    ismEngine_MessageHandle_t hMessage;
    void *payload=NULL;

    for(uint32_t i=0; i<msgCount; i++)
    {
        payload = NULL;

        int32_t rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                        ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                        ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                        ismMESSAGE_FLAGS_NONE,
                                        0,
                                        ismDESTINATION_TOPIC, topicString,
                                        &hMessage, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                        topicString,
                                                        hMessage,
                                                        NULL, 0, NULL);
        TEST_ASSERT((rc == OK), ("rc=%d", rc));

        if (payload) free(payload);
    }
}

//****************************************************************************
/// @brief Test the creation of subscriptions with DCNEnabled set or not
//****************************************************************************
void test_capability_DisconnectedClientNotificationSetting_Phase1(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClientDefault, hClientTrue, hClientFalse;
    ismEngine_SessionHandle_t hSessionDefault, hSessionTrue, hSessionFalse;
    ism_listener_t *mockListenerDefault=NULL, *mockListenerTrue=NULL, *mockListenerFalse;
    ism_transport_t *mockTransportDefault=NULL, *mockTransportTrue=NULL, *mockTransportFalse;
    ismSecurity_t *mockContextDefault, *mockContextTrue, *mockContextFalse;

    rc = test_createSecurityContext(&mockListenerDefault, &mockTransportDefault, &mockContextDefault);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransportDefault->clientID = "DefaultClient";

    rc = test_createSecurityContext(&mockListenerTrue, &mockTransportTrue, &mockContextTrue);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransportTrue->clientID = "TrueClient";

    rc = test_createSecurityContext(&mockListenerFalse, &mockTransportFalse, &mockContextFalse);
    TEST_ASSERT_EQUAL(rc, OK);
    
    mockTransportFalse->clientID = "FalseClient";

    /* Create our clients and sessions */
    rc = test_createClientAndSession(mockTransportDefault->clientID,
                                     mockContextDefault,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClientDefault, &hSessionDefault, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession(mockTransportTrue->clientID,
                                     mockContextTrue,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClientTrue, &hSessionTrue, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession(mockTransportFalse->clientID,
                                     mockContextFalse,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClientFalse, &hSessionFalse, false);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("Starting %s...\n", __func__);

    // One for "DefaultClient*" that doesn't specify DisconnectedClientNotification explicitly
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.DisconnectedClientNotificationDEFAULT\","
                                "\"Name\":\"DisconnectedClientNotificationDEFAULT\","
                                "\"ClientID\":\"DefaultClient*\","
                                "\"Topic\":\"TEST.DCN*\","
                                "\"ActionList\":\"subscribe,publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContextDefault, "DisconnectedClientNotificationDEFAULT");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContextTrue, "DisconnectedClientNotificationDEFAULT");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContextFalse, "DisconnectedClientNotificationDEFAULT");
    TEST_ASSERT_EQUAL(rc, OK);

    // One for "TrueClient*" that specifies DisconnectedClientNotification=True
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.DisconnectedClientNotificationTRUE\","
                                "\"Name\":\"DisconnectedClientNotificationTRUE\","
                                "\"ClientID\":\"TrueClient*\","
                                "\"Topic\":\"TEST.DCN*\","
                                "\"ActionList\":\"subscribe,publish\","
                                "\"DisconnectedClientNotification\":\"True\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContextDefault, "DisconnectedClientNotificationTRUE");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContextTrue, "DisconnectedClientNotificationTRUE");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContextFalse, "DisconnectedClientNotificationTRUE");
    TEST_ASSERT_EQUAL(rc, OK);

    // One for "FalseClient*" that specifies DisconnectedClientNotification=False
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.DisconnectedClientNotificationFALSE\","
                                "\"Name\":\"DisconnectedClientNotificationFALSE\","
                                "\"ClientID\":\"FalseClient*\","
                                "\"Topic\":\"TEST.DCN*\","
                                "\"ActionList\":\"subscribe,publish\","
                                "\"DisconnectedClientNotification\":\"False\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContextDefault, "DisconnectedClientNotificationFALSE");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContextTrue, "DisconnectedClientNotificationFALSE");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContextFalse, "DisconnectedClientNotificationFALSE");
    TEST_ASSERT_EQUAL(rc, OK);

    // Attempt to create various subscriptions
    for(int32_t i=0; i<3; i++)
    {
        ismEngine_ClientStateHandle_t useClient;
        char *useSubscriptionName;
        bool expectDisconnectedClientNotification;

        switch(i)
        {
            case 0:
                useSubscriptionName = "DefaultSubscription";
                useClient = hClientDefault;
                expectDisconnectedClientNotification = false;
                break;
            case 1:
                useSubscriptionName = "TrueSubscription";
                useClient = hClientTrue;
                expectDisconnectedClientNotification = true;
                break;
            case 2:
                useSubscriptionName = "FalseSubscription";
                useClient = hClientFalse;
                expectDisconnectedClientNotification = false;
                break;
            default:
                break;
        }

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ((rand()%3)+1) };

        rc = sync_ism_engine_createSubscription(useClient,
                                                useSubscriptionName,
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                "TEST.DCN.AUTHORIZED",
                                                &subAttrs,
                                                NULL); // Owning client same as requesting client
        TEST_ASSERT_EQUAL(rc, OK);

        ismEngine_Subscription_t *subscription;
        rc = iett_findClientSubscription(pThreadData,
                                         ((ismEngine_ClientState_t *)useClient)->pClientId,
                                         iett_generateClientIdHash(((ismEngine_ClientState_t *)useClient)->pClientId),
                                         useSubscriptionName,
                                         iett_generateSubNameHash(useSubscriptionName),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(subscription);

        iepiPolicyInfo_t *policyInfo = ieq_getPolicyInfo((subscription->queueHandle));
        TEST_ASSERT_PTR_NOT_NULL(policyInfo);
        TEST_ASSERT_EQUAL(policyInfo->DCNEnabled, expectDisconnectedClientNotification);

        iett_releaseSubscription(pThreadData, subscription, false);
    }

    rc = test_deleteSecurityPolicy("UID.DisconnectedClientNotificationDEFAULT", "DisconnectedClientNotificationDEFAULT");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_deleteSecurityPolicy("UID.DisconnectedClientNotificationTRUE", "DisconnectedClientNotificationTRUE");
    TEST_ASSERT_EQUAL(rc, OK)
    rc = test_deleteSecurityPolicy("UID.DisconnectedClientNotificationFALSE", "DisconnectedClientNotificationFALSE");
    TEST_ASSERT_EQUAL(rc, OK);

    // Mock up an old durable MQTT clientState / subscription...
    ismStore_Handle_t hStoreClientProps = ismSTORE_NULL_HANDLE;
    ismStore_Handle_t hStoreClientState = ismSTORE_NULL_HANDLE;

    test_storeIMA100ClientState("IMA100MQTTClient", NULL, &hStoreClientState, &hStoreClientProps);
    TEST_ASSERT_NOT_EQUAL(hStoreClientState, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_EQUAL(hStoreClientProps, ismSTORE_NULL_HANDLE);

    test_storeV1Subscription("TEST.DCN.AUTHORIZED",
                             "IMA100MQTTSubscription",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                             "IMA100MQTTClient");

    test_storeIMA110ClientState("IMA110MQTTClient");

    test_storeV1Subscription("TEST.DCN.AUTHORIZED",
                             "IMA110MQTTSubscription",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE,
                             "IMA110MQTTClient");

    // Mock up an old durable JMS subscription... (no durable clientState)
    test_storeV1Subscription("TEST.DCN.AUTHORIZED",
                             "IMA110JMSSubscription",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             "IMA110JMSClient");

    // Add a v1.2.0 MQTT Client (with no subscriptions) just to go through that migration path.
    test_storeIMA120ClientState("IMA120MQTTClient");

    // Add a MessageSight v2 (old CPR) with no subscriptions to go through that migration path
    test_storeIMA15aV3ClientState("IMA15AV3HTTPClient", PROTOCOL_ID_HTTP);

    // Add a V4 MessageSight CPR with no subscription to go through that rehydration path
    test_storeIMA15aV4ClientState("IMA15AV4MQTTClient", "WILL/TOPIC", PROTOCOL_ID_MQTT);
}

void test_capability_DisconnectedClientNotificationSetting_Phase2(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    // Check the earlier version clientStates got a sensible expiry and will delay time when migrated
    char *checkClientList[] = {"IMA100MQTTClient", "IMA120MQTTClient", "IMA110JMSClient", "IMA15AV3HTTPClient", "IMA15AV4MQTTClient"};
    for(int32_t loop=0; loop<sizeof(checkClientList)/sizeof(checkClientList[0]); loop++)
    {
        char *clientId = checkClientList[loop];

        uint32_t clientIdHash = iecs_generateClientIdHash(clientId);
        ismEngine_ClientState_t *pFoundClient = iecs_getVictimizedClient(pThreadData, clientId, clientIdHash);
        TEST_ASSERT_PTR_NOT_NULL(pFoundClient);
        TEST_ASSERT_NOT_EQUAL(pFoundClient->LastConnectedTime, 0);
        TEST_ASSERT_EQUAL_FORMAT(pFoundClient->ExpiryInterval, iecsEXPIRY_INTERVAL_INFINITE, "0x%x");
        TEST_ASSERT_EQUAL(pFoundClient->ExpiryTime, 0);
        TEST_ASSERT_EQUAL_FORMAT(pFoundClient->WillDelay, 0, "0x%x");
        TEST_ASSERT_EQUAL(pFoundClient->WillDelayExpiryTime, 0);
        if (strcmp("IMA15AV3HTTPClient", clientId) == 0)
        {
            TEST_ASSERT_EQUAL(pFoundClient->protocolId, PROTOCOL_ID_HTTP);
        }
    }

    for(int32_t i=0; i<6; i++)
    {
        const char *useClientId;
        char *useSubscriptionName;
        bool expectDisconnectedClientNotification;

        switch(i)
        {
            case 0:
                useSubscriptionName = "DefaultSubscription";
                useClientId = "DefaultClient";
                expectDisconnectedClientNotification = false;
                break;
            case 1:
                useSubscriptionName = "TrueSubscription";
                useClientId = "TrueClient";
                expectDisconnectedClientNotification = true;
                break;
            case 2:
                useSubscriptionName = "FalseSubscription";
                useClientId = "FalseClient";
                expectDisconnectedClientNotification = false;
                break;
            case 3:
                useSubscriptionName = "IMA100MQTTSubscription";
                useClientId = "IMA100MQTTClient";
                expectDisconnectedClientNotification = false;
                break;
            case 4:
                useSubscriptionName = "IMA110MQTTSubscription";
                useClientId = "IMA110MQTTClient";
                expectDisconnectedClientNotification = false;
                break;
            case 5:
                useSubscriptionName = "IMA110JMSSubscription";
                useClientId = "IMA110JMSClient";
                expectDisconnectedClientNotification = false;
                break;
            default:
                break;
        }

        ismEngine_Subscription_t *subscription;
        rc = iett_findClientSubscription(pThreadData,
                                         useClientId,
                                         iett_generateClientIdHash(useClientId),
                                         useSubscriptionName,
                                         iett_generateSubNameHash(useSubscriptionName),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(subscription);

        iepiPolicyInfo_t *policyInfo = ieq_getPolicyInfo((subscription->queueHandle));
        TEST_ASSERT_PTR_NOT_NULL(policyInfo);
        TEST_ASSERT_EQUAL(policyInfo->DCNEnabled, expectDisconnectedClientNotification);

        iett_releaseSubscription(pThreadData, subscription, false);

        // Destroy
        rc = iett_destroySubscriptionForClientId(pThreadData,
                                                 useClientId,
                                                 NULL,
                                                 useSubscriptionName,
                                                 NULL,
                                                 iettSUB_DESTROY_OPTION_NONE,
                                                 NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

CU_TestInfo ISM_DisconnectedClientNotification_CUnit_test_Setting_Phase1[] =
{
    { "DisconnectedClientNotificationSetting", test_capability_DisconnectedClientNotificationSetting_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_DisconnectedClientNotification_CUnit_test_Setting_Phase2[] =
{
    { "DisconnectedClientNotificationSetting", test_capability_DisconnectedClientNotificationSetting_Phase2 },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the validity of the PutsAttempted stat
//****************************************************************************
void test_capability_PutsAttemptedStat(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    rc = test_createSecurityContext(&mockListener, &mockTransport, &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport->clientID = "StatTestClient";

    /* Create our client and session */
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("Starting %s...\n", __func__);

    // One for "*Client" that specifies DisconnectedClientNotification=True
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.PutsAttemptedStatPolicy\","
                                "\"Name\":\"PutsAttemptedStatPolicy\","
                                "\"ClientID\":\"*Client\","
                                "\"Topic\":\"TEST.PUTSATTEMPTED*\","
                                "\"ActionList\":\"subscribe,publish\","
                                "\"DisconnectedClientNotification\":\"True\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security context
    rc = test_addPolicyToSecContext(mockContext, "PutsAttemptedStatPolicy");
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t subsCreated = 0;
    uint32_t expectSubsCreated = 0;
    uint32_t *pSubsCreated = &subsCreated;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createSubscription(hClient,
                                       "TestPutsAttemptedAMO",
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       "TEST.PUTSATTEMPTED",
                                       &subAttrs,
                                       NULL, // Owning client same as requesting client
                                       &pSubsCreated, sizeof(pSubsCreated), test_asyncCallback);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_EQUAL(rc, OK);
        test_asyncCallback(rc, NULL, &pSubsCreated);
    }
    expectSubsCreated++;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
    rc = ism_engine_createSubscription(hClient,
                                       "TestPutsAttemptedALO",
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       "TEST.PUTSATTEMPTED",
                                       &subAttrs,
                                       NULL, // Owning client same as requesting client
                                       &pSubsCreated, sizeof(pSubsCreated), test_asyncCallback);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_EQUAL(rc, OK);
        test_asyncCallback(rc, NULL, &pSubsCreated);
    }
    expectSubsCreated++;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    rc = ism_engine_createSubscription(hClient,
                                       "TestPutsAttemptedTXN",
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       "TEST.PUTSATTEMPTED",
                                       &subAttrs,
                                       NULL, // Owning client same as requesting client
                                       &pSubsCreated, sizeof(pSubsCreated), test_asyncCallback);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_EQUAL(rc, OK);
        test_asyncCallback(rc, NULL, &pSubsCreated);
    }
    expectSubsCreated++;

    test_waitForAsyncCallbacks(pSubsCreated, expectSubsCreated);

    ismEngine_Subscription_t *subscriptionAMO, *subscriptionALO, *subscriptionTXN;
    rc = iett_findClientSubscription(pThreadData,
                                     mockTransport->clientID,
                                     iett_generateClientIdHash(mockTransport->clientID),
                                     "TestPutsAttemptedAMO",
                                     iett_generateSubNameHash("TestPutsAttemptedAMO"),
                                     &subscriptionAMO);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subscriptionAMO);

    iett_releaseSubscription(pThreadData, subscriptionAMO, true);

    rc = iett_findClientSubscription(pThreadData,
                                     mockTransport->clientID,
                                     iett_generateClientIdHash(mockTransport->clientID),
                                     "TestPutsAttemptedALO",
                                     iett_generateSubNameHash("TestPutsAttemptedALO"),
                                     &subscriptionALO);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subscriptionALO);

    iett_releaseSubscription(pThreadData, subscriptionALO, true);

    rc = iett_findClientSubscription(pThreadData,
                                     mockTransport->clientID,
                                     iett_generateClientIdHash(mockTransport->clientID),
                                     "TestPutsAttemptedTXN",
                                     iett_generateSubNameHash("TestPutsAttemptedTXN"),
                                     &subscriptionTXN);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subscriptionTXN);

    iett_releaseSubscription(pThreadData, subscriptionTXN, true);

    ismEngine_QueueStatistics_t stats;
    ieq_getStats(pThreadData, subscriptionAMO->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.PutsAttemptedDelta, 0);
    ieq_getStats(pThreadData, subscriptionALO->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.PutsAttemptedDelta, 0);
    ieq_getStats(pThreadData, subscriptionTXN->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.PutsAttemptedDelta, 0);

    // Publish 10 messages
    test_publishMessages("TEST.PUTSATTEMPTED", 10);

    ieq_getStats(pThreadData, subscriptionAMO->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.PutsAttemptedDelta, 10);
    ieq_getStats(pThreadData, subscriptionALO->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.PutsAttemptedDelta, 10);
    ieq_getStats(pThreadData, subscriptionTXN->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.PutsAttemptedDelta, 10);

    ieq_getStats(pThreadData, subscriptionAMO->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.PutsAttemptedDelta, 10);
    ieq_setStats(subscriptionAMO->queueHandle, &stats, ieqSetStats_UPDATE_PUTSATTEMPTED);
    ieq_getStats(pThreadData, subscriptionAMO->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.PutsAttemptedDelta, 0);

    // Consume the messages
    ismEngine_ConsumerHandle_t hConsumerAMO;
    msgCallbackContext_t MsgContext1 = {hSession, 0};
    msgCallbackContext_t *pMsgContext1 = &MsgContext1;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "TestPutsAttemptedAMO",
                                   NULL,
                                   NULL,
                                   &pMsgContext1, sizeof(pMsgContext1), test_msgCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerAMO,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(MsgContext1.received, 0);

    rc = ism_engine_startMessageDelivery(hSession, 0, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(MsgContext1.received, 10);

    ieq_getStats(pThreadData, subscriptionAMO->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.PutsAttemptedDelta, 0);

    ieq_getStats(pThreadData, subscriptionALO->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.PutsAttemptedDelta, 10);

    // Disconnect the client state
    rc = sync_ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish 10 more messages
    test_publishMessages("TEST.PUTSATTEMPTED", 10);

    ieq_getStats(pThreadData, subscriptionAMO->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.PutsAttemptedDelta, 10);
    ieq_setStats(subscriptionAMO->queueHandle, &stats, ieqSetStats_UPDATE_PUTSATTEMPTED);
    ieq_getStats(pThreadData, subscriptionAMO->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.PutsAttemptedDelta, 0);

    ieq_getStats(pThreadData, subscriptionTXN->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.PutsAttemptedDelta, 20);

    rc = test_destroySecurityContext(mockListener, mockTransport, mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_DisconnectedClientNotification_CUnit_test_Basic[] =
{
    { "PutsAttemptedStat", test_capability_PutsAttemptedStat },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test calling the notification function
//****************************************************************************
static void test_stealCreationCompleteCallback(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext)
{
    ismEngine_ClientStateHandle_t **ppHandle = (ismEngine_ClientStateHandle_t **)pContext;
    ismEngine_ClientStateHandle_t *pHandle = *ppHandle;
    *pHandle = handle;

    TEST_ASSERT_EQUAL(retcode, ISMRC_ResumedClientState);
}

static void test_stealCallback(int32_t                         reason,
                               ismEngine_ClientStateHandle_t   hClient,
                               uint32_t                        options,
                               void *                          pContext)
{
    TEST_ASSERT_EQUAL(reason, ISMRC_ResumedClientState);
    TEST_ASSERT_EQUAL(options, ismENGINE_STEAL_CALLBACK_OPTION_NONE);
    TEST_ASSERT_EQUAL(hClient, pContext);
}


typedef struct tag_msgNotificationCallbackContext_t
{
    ismEngine_SessionHandle_t hSession;
    uint32_t received;
    const char *expectClientId;
    const char *expectUserId;
} msgNotificationCallbackContext_t;

bool test_msgNotificationCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
    msgNotificationCallbackContext_t *context = *((msgNotificationCallbackContext_t **)pContext);

    __sync_fetch_and_add(&context->received, 1);

    //printf("%.*s\n", (int)areaLengths[1], (char *)pAreaData[1]);

    // Check that the JSON parses OK
    TEST_ASSERT_EQUAL(areaCount, 2);
    TEST_ASSERT_EQUAL(areaTypes[1], ismMESSAGE_AREA_PAYLOAD);

    ism_json_parse_t parseObj = { 0 };
    ism_json_entry_t ents[20];

    parseObj.ent = ents;
    parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseObj.source = alloca(areaLengths[1]+1);
    memcpy(parseObj.source, pAreaData[1], areaLengths[1]);
    parseObj.source[areaLengths[1]] = '\0';
    parseObj.src_len = strlen(parseObj.source);

    int rc = ism_json_parse(&parseObj);
    TEST_ASSERT_EQUAL(rc, OK);

    char *checkString;
    if (context->expectClientId != NULL)
    {
        checkString = (char *)ism_json_getString(&parseObj, "ClientId");
        TEST_ASSERT_PTR_NOT_NULL(checkString);
        TEST_ASSERT_STRINGS_EQUAL(checkString, context->expectClientId);
    }

    if (context->expectUserId != NULL)
    {
        checkString = (char *)ism_json_getString(&parseObj, "UserID");
        TEST_ASSERT_PTR_NOT_NULL(checkString);
        TEST_ASSERT_STRINGS_EQUAL(checkString, context->expectUserId);
    }

    ism_engine_releaseMessage(hMessage);

    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        rc = ism_engine_confirmMessageDelivery(context->hSession,
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

void test_capability_NotificationFunction(void)
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient, hClientWL, hClientOwner;
    ismEngine_SessionHandle_t hSession, hSessionWL, hSessionOwner;
    ism_listener_t *mockListener=NULL, *mockListenerWL=NULL, *mockListenerOwner=NULL;
    ism_transport_t *mockTransport=NULL, *mockTransportWL=NULL, *mockTransportOwner=NULL;
    ismSecurity_t *mockContext, *mockContextWL, *mockContextOwner;
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    rc = test_createSecurityContext(&mockListener, &mockTransport, &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport->clientID = "FunctionClient";
    mockTransport->userid = "OrigUser";

    /* Create our client and session */
    rc = test_createClientAndSessionWithProtocol(mockTransport->clientID,
                                                 PROTOCOL_ID_MQTT,
                                                 mockContext,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                                 ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    /* Create an owner for a shared subscriptions */
    rc = test_createSecurityContext(&mockListenerOwner, &mockTransportOwner, &mockContextOwner);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransportOwner->clientID = ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE;

    rc = test_createClientAndSessionWithProtocol(mockTransportOwner->clientID,
                                                 PROTOCOL_ID_SHARED,
                                                 mockContextOwner,
                                                 ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClientOwner, &hSessionOwner, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // One for "FunctionClient" that specifies DisconnectedClientNotification=True
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.FunctionPolicy\","
                                "\"Name\":\"FunctionPolicy\","
                                "\"ClientID\":\"FunctionClient\","
                                "\"Topic\":\"*\","
                                "\"ActionList\":\"subscribe,publish\","
                                "\"DisconnectedClientNotification\":\"True\","
                                "\"MaxMessages\":20,"
                                "\"MaxMessagesBehavior\":\"RejectNewMessages\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security context
    rc = test_addPolicyToSecContext(mockContext, "FunctionPolicy");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createSecurityContext(&mockListenerWL, &mockTransportWL, &mockContextWL);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransportWL->clientID = "WorkLight";

    // One for "Worklight" that doesn't specify DisconnectedClientNotification
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.WorkLightPolicy\","
                                "\"Name\":\"WorkLightPolicy\","
                                "\"ClientID\":\"WorkLight\","
                                "\"Topic\":\"*\","
                                "\"ActionList\":\"subscribe\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security context
    rc = test_addPolicyToSecContext(mockContextWL, "WorkLightPolicy");
    TEST_ASSERT_EQUAL(rc, OK);

    printf("Starting %s...\n", __func__);

    msgNotificationCallbackContext_t MsgContext1 = {hSessionWL, 0};
    msgNotificationCallbackContext_t *pMsgContext1 = &MsgContext1;

    uint32_t subsCreated = 0;
    uint32_t expectSubsCreated = 0;
    uint32_t *pSubsCreated = &subsCreated;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createSubscription(hClient,
                                       "TestFunctionSub1",
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       "TEST.NOTIFICATIONFUNCTION/1",
                                       &subAttrs,
                                       NULL, // Owning client same as requesting client
                                       &pSubsCreated, sizeof(pSubsCreated), test_asyncCallback);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_EQUAL(rc, OK);
        test_asyncCallback(rc, NULL, &pSubsCreated);
    }
    expectSubsCreated++;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createSubscription(hClient,
                                       "TestFunctionSub2",
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       "TEST.NOTIFICATIONFUNCTION/\\\":Whacky Characters,",
                                       &subAttrs,
                                       NULL, // Owning client same as requesting client
                                       &pSubsCreated, sizeof(pSubsCreated), test_asyncCallback);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_EQUAL(rc, OK);
        test_asyncCallback(rc, NULL, &pSubsCreated);
    }
    expectSubsCreated++;

    // Create a shared subscription to drive the code to avoid processing for them
    for(uint32_t i=0; i<3; i++)
    {
        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                              ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |
                              ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                              ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT;

        rc = sync_ism_engine_createSubscription(hClient,
                                                "TestFunctionSharedSub",
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                "TEST.NOTIFICATIONFUNCTION/Shared",
                                                &subAttrs,
                                                hClientOwner);
        if (i == 0)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_NotAuthorized);

            // Add a policy for "FunctionClient" to create shared subscriptions
            rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY,
                                        "{\"UID\":\"UID.SharedPolicy\","
                                        "\"Name\":\"SharedPolicy\","
                                        "\"ClientID\":\"FunctionClient\","
                                        "\"SubscriptionName\":\"*\","
                                        "\"ActionList\":\"control,receive\","
                                        "\"MaxMessages\":20,"
                                        "\"MaxMessagesBehavior\":\"RejectNewMessages\"}");
            TEST_ASSERT_EQUAL(rc, OK);

            // Associate the policy with security context
            rc = test_addPolicyToSecContext(mockContext, "SharedPolicy");
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);

            if (i == 1)
            {
                // Update policy to only allow receive
                rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY,
                                            "{\"UID\":\"UID.SharedPolicy\","
                                            "\"Name\":\"SharedPolicy\","
                                            "\"ClientID\":\"FunctionClient\","
                                            "\"SubscriptionName\":\"*\","
                                            "\"ActionList\":\"receive\","
                                            "\"MaxMessages\":20,"
                                            "\"MaxMessagesBehavior\":\"RejectNewMessages\","
                                            "\"Update\":\"true\"}");
                TEST_ASSERT_EQUAL(rc, OK);
            }

            // Try a re-use, just to drive the permissions code there
            rc = ism_engine_reuseSubscription(hClient,
                                              "TestFunctionSharedSub",
                                              &subAttrs,
                                              hClientOwner);
            TEST_ASSERT_EQUAL(rc, OK);

            if (i == 1)
            {
                rc = ism_engine_destroySubscription(hClient,
                                                    "TestFunctionSharedSub",
                                                    hClientOwner,
                                                    NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);

                // Expect a reuse now to fail as the subscription should have gone
                rc = ism_engine_reuseSubscription(hClient,
                                              "TestFunctionSharedSub",
                                              &subAttrs,
                                              hClientOwner);
                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

                // Change back to allow control and receive
                rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY,
                                            "{\"UID\":\"UID.SharedPolicy\","
                                            "\"Name\":\"SharedPolicy\","
                                            "\"ClientID\":\"FunctionClient\","
                                            "\"SubscriptionName\":\"*\","
                                            "\"ActionList\":\"control,receive\","
                                            "\"MaxMessages\":20,"
                                            "\"MaxMessagesBehavior\":\"RejectNewMessages\","
                                            "\"Update\":\"true\"}");
                TEST_ASSERT_EQUAL(rc, OK);
            }
        }
    }

    test_waitForAsyncCallbacks(pSubsCreated, expectSubsCreated);

    // Publish 10 messages
    test_publishMessages("TEST.NOTIFICATIONFUNCTION/1", 10);

    // Should be no notification, as the client is connected
    rc = ism_engine_generateDisconnectedClientNotifications();
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(MsgContext1.received, 0);

    // Disconnect the client state
    rc = sync_ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    for(uint32_t i=0; i<6; i++)
    {
        /* Create a client and session to consume notifications */
        rc = test_createClientAndSessionWithProtocol(mockTransportWL->clientID,
                                                     PROTOCOL_ID_JMS,
                                                     mockContextWL,
                                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                     &hClientWL, &hSessionWL, true);
        TEST_ASSERT_EQUAL(rc, OK);

        // Expect no message (there is no consumer)
        rc = ism_engine_generateDisconnectedClientNotifications();
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(MsgContext1.received, 0);

        MsgContext1.expectClientId = mockTransport->clientID;
        MsgContext1.expectUserId = mockTransport->userid;

        // Create a consumer
        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
        rc = ism_engine_createConsumer(hSessionWL,
                                       ismDESTINATION_TOPIC,
                                       "$SYS/DisconnectedClientNotification",
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &pMsgContext1, sizeof(pMsgContext1), test_msgNotificationCallback,
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_NONE,
                                       &hConsumer,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(MsgContext1.received, 0);

        // Expect a message
        rc = ism_engine_generateDisconnectedClientNotifications();
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(MsgContext1.received, 1);

        // Publish 10 more messages
        test_publishMessages("TEST.NOTIFICATIONFUNCTION/1", 10);

        // And 1 to the topic for a 2nd subscription which needs to be listed
        test_publishMessages("TEST.NOTIFICATIONFUNCTION/\\\":Whacky Characters,", 1);

        // And 1 to the topic for the shared subscription which should not be listed
        test_publishMessages("TEST.NOTIFICATIONFUNCTION/Shared", 1);

        // Expect a message
        rc = ism_engine_generateDisconnectedClientNotifications();
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(MsgContext1.received, 2);

        // Expect no new message - no new publishes
        rc = ism_engine_generateDisconnectedClientNotifications();
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(MsgContext1.received, 2);

        // Publish 1 more message (which should break MaxMessages)
        test_publishMessages("TEST.NOTIFICATIONFUNCTION/1", 1);

        // Expect a message
        rc = ism_engine_generateDisconnectedClientNotifications();
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(MsgContext1.received, 3);

        // Expect no new message - no new publishes
        rc = ism_engine_generateDisconnectedClientNotifications();
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(MsgContext1.received, 3);

        rc = test_destroyClientAndSession(hClientWL, hSessionWL, true);
        TEST_ASSERT_EQUAL(rc, OK);

        // Publish 1 more message (which should increase rejected count)
        test_publishMessages("TEST.NOTIFICATIONFUNCTION/1", 1);

        // Expect no new message (there is no consumer)
        rc = ism_engine_generateDisconnectedClientNotifications();
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(MsgContext1.received, 3);

        if (i < 5)
        {
            MsgContext1.received = 0;

            if (i == 1 || i == 3)
            {
                ismEngine_ClientStateHandle_t hClientThief = NULL;
                ismEngine_ClientStateHandle_t *pStealContext = &hClientThief;

                mockTransport->userid = "NewUser";

                // Pick up the zombie with a new userid
                rc = test_createClientAndSessionWithProtocol(mockTransport->clientID,
                                                             PROTOCOL_ID_MQTT,
                                                             mockContext,
                                                             ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                                             ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                             ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                             &hClient, &hSession, false);
                TEST_ASSERT_EQUAL(rc, OK);

                // Test Stealing
                if (i == 3)
                {
                    mockTransport->userid = "ThiefUser";

                    rc = ism_engine_createClientState(mockTransport->clientID,
                                                      PROTOCOL_ID_MQTT,
                                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                                      ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                      hClient, test_stealCallback,
                                                      mockContext,
                                                      &hClientThief,
                                                      &pStealContext, sizeof(pStealContext), test_stealCreationCompleteCallback);
                    TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

                    // Disconnect the client state
                    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
                    TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("ism_engine_destroyClientState returned rc=%d", rc));

                    // hClientThief should contain the thief's client handle, once any async completion has finished
                    while(hClientThief == NULL)
                    {
                        sched_yield();
                    }

                    TEST_ASSERT_NOT_EQUAL(hClientThief, NULL);
                    rc = sync_ism_engine_destroyClientState(hClientThief, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
                    TEST_ASSERT_EQUAL(rc, OK);
                }
                else
                {
                    // Disconnect the client state
                    rc = sync_ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
                    TEST_ASSERT_EQUAL(rc, OK);
                }
            }

            // Publish 10 messages
            test_publishMessages("TEST.NOTIFICATIONFUNCTION/1", 3);
        }
    }

    // Pick up the zombie...
    rc = test_createClientAndSessionWithProtocol(mockTransport->clientID,
                                                 PROTOCOL_ID_MQTT,
                                                 mockContext,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                                 ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // And actually discard it
    rc = sync_ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener, mockTransport, mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_destroySecurityContext(mockListenerWL, mockTransportWL, mockContextWL);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_DisconnectedClientNotification_CUnit_test_Notification[] =
{
    { "NotificationFunction", test_capability_NotificationFunction },
    CU_TEST_INFO_NULL
};

void test_capability_Migration_Phase1(void)
{
    printf("Starting %s...\n", __func__);

    ismStore_Handle_t hCSR1, hCPR1, hCSR2, hCPR2;

    // Create a CSR only V1 client state
    test_storeIMA100ClientState("MigrationClient1", NULL, &hCSR1, &hCPR1);
    TEST_ASSERT_NOT_EQUAL_FORMAT(hCSR1, ismSTORE_NULL_HANDLE, "%lu");
    TEST_ASSERT_EQUAL_FORMAT(hCPR1, ismSTORE_NULL_HANDLE, "%lu");

    // Create a CPR+CSR V1 client state
    test_storeIMA100ClientState("MigrationClient2", "WILLTOPIC", &hCSR2, &hCPR2);
    TEST_ASSERT_NOT_EQUAL_FORMAT(hCSR2, ismSTORE_NULL_HANDLE, "%lu");
    TEST_ASSERT_NOT_EQUAL_FORMAT(hCPR2, ismSTORE_NULL_HANDLE, "%lu");
}

void test_capability_Migration_PhaseN(int32_t phase)
{
    uint32_t rc;
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SessionHandle_t hSession1, hSession2;
    ism_listener_t *mockListener1=NULL, *mockListener2=NULL;
    ism_transport_t *mockTransport1=NULL, *mockTransport2=NULL;
    ismSecurity_t *mockContext1, *mockContext2;
    ismEngine_ClientState_t *pClient;

    printf("Starting %s (N=%d)...\n", __func__, phase);

    rc = test_createSecurityContext(&mockListener1, &mockTransport1, &mockContext1);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport1->clientID = "MigrationClient1";
    mockTransport1->userid = "MigrationUser1";

    rc = test_createSecurityContext(&mockListener2, &mockTransport2, &mockContext2);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport2->clientID = "MigrationClient2";
    mockTransport2->userid = "MigrationUser2";

    ismStore_Handle_t hCSR1, hCPR1, hCSR2, hCPR2;

    hClient1 = iecs_getVictimizedClient(pThreadData, mockTransport1->clientID, iecs_generateClientIdHash(mockTransport1->clientID));
    TEST_ASSERT_PTR_NOT_NULL(hClient1);
    hCSR1 = hClient1->hStoreCSR;
    hCPR1 = hClient1->hStoreCPR;

    hClient2 = iecs_getVictimizedClient(pThreadData, mockTransport2->clientID, iecs_generateClientIdHash(mockTransport2->clientID));
    TEST_ASSERT_PTR_NOT_NULL(hClient2);
    hCSR2 = hClient2->hStoreCSR;
    hCPR2 = hClient2->hStoreCPR;

    // Create our clients and sessions - should update the CPR because we now have a USERID
    rc = test_createClientAndSessionWithProtocol(mockTransport1->clientID,
                                                 PROTOCOL_ID_MQTT,
                                                 mockContext1,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                                 ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient1, &hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient1);
    pClient = (ismEngine_ClientState_t *)hClient1;
    TEST_ASSERT_PTR_NOT_NULL(pClient->pUserId);
    TEST_ASSERT_STRINGS_EQUAL(mockTransport1->userid, pClient->pUserId);
    TEST_ASSERT_EQUAL_FORMAT(hCSR1, pClient->hStoreCSR, "0x%lx");
    if (phase == 2)
    {
        // Should have changed
        TEST_ASSERT_NOT_EQUAL_FORMAT(hCPR1, pClient->hStoreCPR, "0x%lx");
    }
    else
    {
        // Should not have changed
        TEST_ASSERT_EQUAL_FORMAT(hCPR1, pClient->hStoreCPR, "0x%lx");
    }

    // Create our clients and sessions - should update the CPR because we now have a USERID
    rc = test_createClientAndSessionWithProtocol(mockTransport2->clientID,
                                                 PROTOCOL_ID_MQTT,
                                                 mockContext2,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                                 ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient2, &hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient2);
    pClient = (ismEngine_ClientState_t *)hClient2;
    TEST_ASSERT_PTR_NOT_NULL(pClient->pUserId);
    TEST_ASSERT_STRINGS_EQUAL(mockTransport2->userid, pClient->pUserId);
    TEST_ASSERT_EQUAL_FORMAT(hCSR2, pClient->hStoreCSR, "0x%lx");
    if (phase == 2)
    {
        TEST_ASSERT_NOT_EQUAL_FORMAT(hCPR2, pClient->hStoreCPR, "0x%lx");
        hCPR2 = pClient->hStoreCPR;
    }
    else
    {
        TEST_ASSERT_EQUAL_FORMAT(hCPR2, pClient->hStoreCPR, "0x%lx");
    }

    if (phase == 3)
    {
        // Actually discard clients
        rc = sync_ism_engine_destroyClientState(hClient1, ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
        TEST_ASSERT_EQUAL(rc, OK);
        rc = sync_ism_engine_destroyClientState(hClient2, ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = test_destroySecurityContext(mockListener1, mockTransport1, mockContext1);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_destroySecurityContext(mockListener2, mockTransport2, mockContext2);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_Migration_Phase2(void)
{
    test_capability_Migration_PhaseN(2);
}

void test_capability_Migration_Phase3(void)
{
    test_capability_Migration_PhaseN(3);
}

CU_TestInfo ISM_DisconnectedClientNotification_CUnit_test_Migration_Phase1[] =
{
    { "Migration", test_capability_Migration_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_DisconnectedClientNotification_CUnit_test_Migration_Phase2[] =
{
    { "Migration", test_capability_Migration_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_DisconnectedClientNotification_CUnit_test_Migration_Phase3[] =
{
    { "Migration", test_capability_Migration_Phase3 },
    CU_TEST_INFO_NULL
};

void test_scale(void)
{
    uint32_t rc;
    ism_listener_t *mockListener=NULL, *mockListenerWL=NULL;
    ism_transport_t *mockTransport=NULL, *mockTransportWL=NULL;
    ismSecurity_t *mockContext, *mockContextWL;
    // For a full test, go with 50,000 clients, otherwise 5000 clients
    uint32_t totalClients = fullTest ? 50000 : 5000;
    ism_time_t startTime, endTime, diffTime;
    double diffTimeSecs;

    printf("Starting %s with %d clients...\n", __func__, totalClients);

    rc = test_createSecurityContext(&mockListener, &mockTransport, &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add a security policy to enabled DisconnectedClientNotification
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.FunctionPolicy\","
                                "\"Name\":\"FunctionPolicy\","
                                "\"ClientID\":\"ScaleClient*\","
                                "\"Topic\":\"*\","
                                "\"ActionList\":\"subscribe,publish\","
                                "\"DisconnectedClientNotification\":\"True\","
                                "\"MaxMessages\":10000,"
                                "\"MaxMessagesBehavior\":\"DiscardOldMessages\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security context
    rc = test_addPolicyToSecContext(mockContext, "FunctionPolicy");
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t clientsDestroyed = 0;
    uint32_t expectClientsDestroyed = 0;
    uint32_t *pClientsDestroyed = &clientsDestroyed;

    uint32_t subsCreated = 0;
    uint32_t expectSubsCreated = 0;
    uint32_t *pSubsCreated = &subsCreated;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    startTime = ism_common_currentTimeNanos();
    for(int32_t i=0; i<totalClients; i++)
    {
        char clientID[20];
        char userID[40];
        char subName[20];
        char topicString[20];

        sprintf(clientID, "ScaleClient%d", i);
        sprintf(userID, "user_number%d@test.scale.ibm.com", i);
        sprintf(subName, "ScaleSub%d", i);
        sprintf(topicString, "TEST.SCALE/%d", i);

        mockTransport->clientID = clientID;
        mockTransport->userid = userID;

        // Create a client & sub, then destroy the client
        ismEngine_ClientStateHandle_t hClient;
        rc = sync_ism_engine_createClientState(mockTransport->clientID,
                                               PROTOCOL_ID_MQTT,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               NULL, NULL,
                                               mockContext,
                                               &hClient);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hClient);

        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ((rand()%3)+1);
        rc = ism_engine_createSubscription(hClient,
                                           subName,
                                           NULL,
                                           ismDESTINATION_TOPIC,
                                           topicString,
                                           &subAttrs,
                                           NULL, // Owning client same as requesting client
                                           &pSubsCreated, sizeof(pSubsCreated), test_asyncCallback);
        if (rc != ISMRC_AsyncCompletion)
        {
            TEST_ASSERT_EQUAL(rc, OK);
            test_asyncCallback(rc, NULL, &pSubsCreated);
        }
        expectSubsCreated++;

        // Batches of 100 max
        if (expectSubsCreated == 100 || i==totalClients-1)
        {
            test_waitForAsyncCallbacks(pSubsCreated, expectSubsCreated);
            __sync_lock_test_and_set(&subsCreated, 0);
            __sync_lock_test_and_set(&expectSubsCreated, 0);
        }

        rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, &pClientsDestroyed, sizeof(pClientsDestroyed), test_asyncCallback);
        if (rc != ISMRC_AsyncCompletion)
        {
            TEST_ASSERT_EQUAL(rc, OK);
            test_asyncCallback(rc, NULL, &pClientsDestroyed);
        }
        expectClientsDestroyed++;
    }

    test_waitForAsyncCallbacks(pClientsDestroyed, expectClientsDestroyed);

    endTime = ism_common_currentTimeNanos();

    if (fullTest)
    {
        diffTime = endTime-startTime;
        diffTimeSecs = ((double)diffTime) / 1000000000;
        printf("Time to create %d clients & subscriptions is %.2f secs. (%ldns)\n",
               totalClients, diffTimeSecs, diffTime);
    }

    // Should do nothing, as no-one is consuming from $SYS
    startTime = ism_common_currentTimeNanos();
    rc = ism_engine_generateDisconnectedClientNotifications();
    endTime = ism_common_currentTimeNanos();
    TEST_ASSERT_EQUAL(rc, OK);
    if (fullTest)
    {
        diffTime = endTime-startTime;
        diffTimeSecs = ((double)diffTime) / 1000000000;
        printf("Time to generate %u messages ism_engine_generateDisconnectedClientNotifications is %.2f secs. (%ldns)\n",
               0, diffTimeSecs, diffTime);
    }

    // Consume on $SYS/DisconnectedClientNotification
    ismEngine_ClientStateHandle_t hClientWL;
    ismEngine_SessionHandle_t hSessionWL;
    ismEngine_ConsumerHandle_t hConsumer;

    rc = test_createSecurityContext(&mockListenerWL, &mockTransportWL, &mockContextWL);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransportWL->clientID = "WorkLight";

    // One for "FunctionClient" that specifies DisconnectedClientNotification=True
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.WorkLightPolicy\","
                                "\"Name\":\"WorkLightPolicy\","
                                "\"ClientID\":\"WorkLight\","
                                "\"Topic\":\"*\","
                                "\"ActionList\":\"subscribe\","
                                "\"MaxMessages\":\"1000000\","
                                "\"MaxMessagesBehavior\":\"RejectNewMessages\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security context
    rc = test_addPolicyToSecContext(mockContextWL, "WorkLightPolicy");
    TEST_ASSERT_EQUAL(rc, OK);

    /* Create a client and session to consume notifications */
    rc = test_createClientAndSession(mockTransportWL->clientID,
                                     mockContextWL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClientWL, &hSessionWL, true);
    TEST_ASSERT_EQUAL(rc, OK);

    msgNotificationCallbackContext_t MsgContext1 = {hSessionWL, 0};
    msgNotificationCallbackContext_t *pMsgContext1 = &MsgContext1;

    // Create a consumer
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionWL,
                                   ismDESTINATION_TOPIC,
                                   "$SYS/DisconnectedClientNotification",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pMsgContext1, sizeof(pMsgContext1), test_msgNotificationCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(MsgContext1.received, 0);

    startTime = ism_common_currentTimeNanos();
    rc = ism_engine_generateDisconnectedClientNotifications();
    endTime = ism_common_currentTimeNanos();
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(MsgContext1.received, 0);
    if (fullTest)
    {
        diffTime = endTime-startTime;
        diffTimeSecs = ((double)diffTime) / 1000000000;
        printf("Time to generate %u messages ism_engine_generateDisconnectedClientNotifications is %.2f secs. (%ldns)\n",
               MsgContext1.received, diffTimeSecs, diffTime);
    }
    MsgContext1.received = 0;

    // Publish 10 messages to 1 subscription
    test_publishMessages("TEST.SCALE/21", 10);

    // Should be a notification
    MsgContext1.expectClientId = "ScaleClient21";
    MsgContext1.expectUserId = "user_number21@test.scale.ibm.com";
    startTime = ism_common_currentTimeNanos();
    rc = ism_engine_generateDisconnectedClientNotifications();
    endTime = ism_common_currentTimeNanos();
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(MsgContext1.received, 1);
    if (fullTest)
    {
        diffTime = endTime-startTime;
        diffTimeSecs = ((double)diffTime) / 1000000000;
        printf("Time to generate %u messages ism_engine_generateDisconnectedClientNotifications is %.2f secs. (%ldns)\n",
               MsgContext1.received, diffTimeSecs, diffTime);
    }

    // Publish to a significant number of clients
    uint32_t publishCount[totalClients];

    memset(publishCount, 0, totalClients * sizeof(uint32_t));

    startTime = ism_common_currentTimeNanos();
    for(int32_t i=fullTest ? (totalClients / 5) : (totalClients / 2); i > 0; i--)
    {
        char topicString[20];
        int testClient = rand()%totalClients;

        sprintf(topicString, "TEST.SCALE/%d", testClient);

        // Publish 10 messages to 1 subscription
        test_publishMessages(topicString, 1);

        publishCount[testClient] += 1;
    }
    endTime = ism_common_currentTimeNanos();

    uint32_t publishClients = 0;
    for(int32_t i=0; i<totalClients; i++)
    {
        if (publishCount[i] > 0) publishClients++;
    }

    if (fullTest)
    {
        diffTime = endTime-startTime;
        diffTimeSecs = ((double)diffTime) / 1000000000;
        printf("Time to spray messages to %d subscriptions is %.2f secs. (%ldns)\n",
               publishClients, diffTimeSecs, diffTime);
    }

    MsgContext1.received = 0;
    MsgContext1.expectClientId = MsgContext1.expectUserId = NULL;
    startTime = ism_common_currentTimeNanos();
    rc = ism_engine_generateDisconnectedClientNotifications();
    endTime = ism_common_currentTimeNanos();
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(MsgContext1.received, publishClients);
    if (fullTest)
    {
        diffTime = endTime-startTime;
        diffTimeSecs = ((double)diffTime) / 1000000000;
        printf("Time to generate %u messages ism_engine_generateDisconnectedClientNotifications is %.2f secs. (%ldns)\n",
               MsgContext1.received, diffTimeSecs, diffTime);
    }

    // Disconnect the client state
    rc = sync_ism_engine_destroyClientState(hClientWL, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener, mockTransport, mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_destroySecurityContext(mockListenerWL, mockTransportWL, mockContextWL);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_DisconnectedClientNotification_CUnit_test_Scale[] =
{
    { "Scale Test", test_scale },
    CU_TEST_INFO_NULL
};

void test_failCreateCallback(int32_t retcode,
                             void *handle,
                             void *pContext)
{
    ismEngine_ClientStateHandle_t *phClient = *(ismEngine_ClientStateHandle_t **)pContext;
    TEST_ASSERT_EQUAL(retcode, OK);
    *phClient = handle;
}


void test_fail(void)
{
    uint32_t rc;
    ism_listener_t *mockListener=NULL, *mockListenerWL=NULL;
    ism_transport_t *mockTransport=NULL, *mockTransportWL=NULL;
    ismSecurity_t *mockContext, *mockContextWL;
    uint32_t totalClients = 500;

    printf("Starting %s...\n", __func__);

    rc = test_createSecurityContext(&mockListener, &mockTransport, &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add a security policy to enabled DisconnectedClientNotification
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.FailPolicy\","
                                "\"Name\":\"FailPolicy\","
                                "\"ClientID\":\"FailClient*\","
                                "\"Topic\":\"*\","
                                "\"ActionList\":\"subscribe,publish\","
                                "\"DisconnectedClientNotification\":\"True\","
                                "\"MaxMessages\":10000}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security context
    rc = test_addPolicyToSecContext(mockContext, "FailPolicy");
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    ismEngine_ClientStateHandle_t hClient[totalClients];
    int32_t i;

    memset(hClient, 0, sizeof(hClient));

    char clientID[20];
    char userID[40];

    for(i=0; i<totalClients; i++)
    {
        sprintf(clientID, "FailClient%d", i);
        sprintf(userID, "user%d", i);

        mockTransport->clientID = clientID;
        mockTransport->userid = userID;

        // Create a client & sub, then destroy the client
        ismEngine_ClientStateHandle_t *phClient = &hClient[i];
        rc = ism_engine_createClientState(mockTransport->clientID,
                                          PROTOCOL_ID_MQTT,
                                          ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                          NULL, NULL,
                                          mockContext,
                                          &hClient[i],
                                          &phClient, sizeof(phClient), test_failCreateCallback);
        TEST_ASSERT((rc == OK) || (rc == ISMRC_AsyncCompletion), ("ism_engine_createClientState failed with rc=%d\n", rc));
    }

    // Wait for all clients to be created
    do
    {
        for(i=0; i<totalClients; i++)
        {
            if (hClient[i] == NULL)
            {
                sched_yield();
                break;
            }
        }
    }
    while(i<totalClients);

    // Create a subscription for each client.
    for(i=0; i<totalClients; i++)
    {
        char subName[20];
        char topicString[20];

        sprintf(subName, "FailSub%d", i);
        sprintf(topicString, "TEST.FAIL/%d", i);

        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ((rand()%3)+1);
        rc = sync_ism_engine_createSubscription(hClient[i],
                                                subName,
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                &subAttrs,
                                                NULL); // Owning client same as requesting client
        TEST_ASSERT_EQUAL(rc, OK);

        rc = sync_ism_engine_destroyClientState(hClient[i], ismENGINE_DESTROY_CLIENT_OPTION_NONE);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Consume on $SYS/DisconnectedClientNotification
    ismEngine_ClientStateHandle_t hClientWL;
    ismEngine_SessionHandle_t hSessionWL;
    ismEngine_ConsumerHandle_t hConsumer;

    rc = test_createSecurityContext(&mockListenerWL, &mockTransportWL, &mockContextWL);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransportWL->clientID = "WorkLight";

    // One for "FunctionClient" that specifies DisconnectedClientNotification=True
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.WorkLightPolicy\","
                                "\"Name\":\"WorkLightPolicy\","
                                "\"ClientID\":\"WorkLight\","
                                "\"Topic\":\"*\","
                                "\"ActionList\":\"subscribe\","
                                "\"MaxMessages\":\"100\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security context
    rc = test_addPolicyToSecContext(mockContextWL, "WorkLightPolicy");
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a client and session to consume notifications, not delivery not started
    rc = test_createClientAndSession(mockTransportWL->clientID,
                                     mockContextWL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClientWL, &hSessionWL, false);
    TEST_ASSERT_EQUAL(rc, OK);

    msgNotificationCallbackContext_t MsgContext1 = {hSessionWL, 0};
    msgNotificationCallbackContext_t *pMsgContext1 = &MsgContext1;

    // Create a consumer
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
    rc = ism_engine_createConsumer(hSessionWL,
                                   ismDESTINATION_TOPIC,
                                   "$SYS/DisconnectedClientNotification",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pMsgContext1, sizeof(pMsgContext1), test_msgNotificationCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(MsgContext1.received, 0);

    // Publish to all clients
    for(i=0; i<totalClients; i++)
    {
        char topicString[20];

        sprintf(topicString, "TEST.FAIL/%d", i);

        test_publishMessages(topicString, 1);
    }

    for(i=0; i<6; i++)
    {
        MsgContext1.received = 0;
        MsgContext1.expectClientId = MsgContext1.expectUserId = NULL;
        rc = ism_engine_generateDisconnectedClientNotifications();

        if (i<4)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_DestinationFull);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }

        rc = ism_engine_startMessageDelivery(hSessionWL, ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        if (i<5)
        {
            TEST_ASSERT_EQUAL(MsgContext1.received, 100);
            rc = ism_engine_stopMessageDelivery(hSessionWL, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            TEST_ASSERT_EQUAL(MsgContext1.received, 0);
        }
    }

    // Disconnect the client state
    rc = sync_ism_engine_destroyClientState(hClientWL, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener, mockTransport, mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_destroySecurityContext(mockListenerWL, mockTransportWL, mockContextWL);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_DisconnectedClientNotification_CUnit_test_Fail[] =
{
    { "Fail Test", test_fail },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initDisconnectedClientNotificationSecurity(void)
{
    return test_engineInit(true, false,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int initDisconnectedClientNotificationScale(void)
{
    return test_engineInit(true, false,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           fullTest ? 512 : testDEFAULT_STORE_SIZE);
}

int initDisconnectedClientNotificationSecurityWarm(void)
{
    return test_engineInit(false, false,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termDisconnectedClientNotification(void)
{
    return test_engineTerm(true);
}

int termDisconnectedClientNotificationWarm(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_DisconnectedClientNotification_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("Basic", initDisconnectedClientNotificationSecurity, termDisconnectedClientNotification, ISM_DisconnectedClientNotification_CUnit_test_Basic),
    IMA_TEST_SUITE("Notification", initDisconnectedClientNotificationSecurity, termDisconnectedClientNotification, ISM_DisconnectedClientNotification_CUnit_test_Notification),
    IMA_TEST_SUITE("Scale", initDisconnectedClientNotificationScale, termDisconnectedClientNotification, ISM_DisconnectedClientNotification_CUnit_test_Scale),
    IMA_TEST_SUITE("Fail", initDisconnectedClientNotificationScale,  termDisconnectedClientNotification, ISM_DisconnectedClientNotification_CUnit_test_Fail),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_DisconnectedClientNotification_CUnit_phase2suites[] =
{
        IMA_TEST_SUITE("Setting", initDisconnectedClientNotificationSecurity, termDisconnectedClientNotificationWarm, ISM_DisconnectedClientNotification_CUnit_test_Setting_Phase1),
        CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_DisconnectedClientNotification_CUnit_phase3suites[] =
{
        IMA_TEST_SUITE("Setting", initDisconnectedClientNotificationSecurityWarm, termDisconnectedClientNotification, ISM_DisconnectedClientNotification_CUnit_test_Setting_Phase2),
        CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_DisconnectedClientNotification_CUnit_phase4suites[] =
{
        IMA_TEST_SUITE("Migration", initDisconnectedClientNotificationSecurity, termDisconnectedClientNotificationWarm, ISM_DisconnectedClientNotification_CUnit_test_Migration_Phase1),
        CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_DisconnectedClientNotification_CUnit_phase5suites[] =
{
        IMA_TEST_SUITE("Migration", initDisconnectedClientNotificationSecurityWarm, termDisconnectedClientNotificationWarm, ISM_DisconnectedClientNotification_CUnit_test_Migration_Phase2),
        CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_DisconnectedClientNotification_CUnit_phase6suites[] =
{
        IMA_TEST_SUITE("Migration", initDisconnectedClientNotificationSecurityWarm, termDisconnectedClientNotification, ISM_DisconnectedClientNotification_CUnit_test_Migration_Phase3),
        CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_DisconnectedClientNotification_CUnit_phase1suites
                              , ISM_DisconnectedClientNotification_CUnit_phase2suites
                              , ISM_DisconnectedClientNotification_CUnit_phase3suites
                              , ISM_DisconnectedClientNotification_CUnit_phase4suites
                              , ISM_DisconnectedClientNotification_CUnit_phase5suites
                              , ISM_DisconnectedClientNotification_CUnit_phase6suites };

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        for(int i=1; i<argc; i++)
        {
            if (!strcasecmp(argv[i], "FULL"))
            {
                fullTest = true;
            }
        }
    }

    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
