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
/* Module Name: test_topicTreeDurable.c                              */
/*                                                                   */
/* Description: Main source for CUnit durable topic tree tests       */
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
#include "engineDiag.h"

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_assert.h"
#include "test_utils_security.h"
#include "test_utils_ismlogs.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_config.h"
#include "test_utils_task.h"

#define TEST_ANALYSE_CREATETRANREF       99
#define TEST_ANALYSE_CREATETRANREF_STACK 100

#define IGNORED_HANDLE (ismStore_Handle_t)(-1)

// Allow overriding the ism_store_createRecord function
ismStore_RecordType_t *createRecordIgnoreTypes = NULL;
int32_t ism_store_createRecord(ismStore_StreamHandle_t hStream,
                               const ismStore_Record_t *pRecord,
                               ismStore_Handle_t *pHandle)
{
    int32_t rc = OK;
    bool ignore = false;
    static int32_t (*real_ism_store_createRecord)(ismStore_StreamHandle_t, const ismStore_Record_t *, ismStore_Handle_t *) = NULL;

    if (real_ism_store_createRecord == NULL)
    {
        real_ism_store_createRecord = dlsym(RTLD_NEXT, "ism_store_createRecord");
    }

    if (createRecordIgnoreTypes != NULL)
    {
        ismStore_RecordType_t *ignoreType = createRecordIgnoreTypes;

        for(; *ignoreType; ignoreType++)
        {
            if (*ignoreType == pRecord->Type)
            {
                ignore = true;
                break;
            }
        }
    }

    if (!ignore)
    {
        rc = real_ism_store_createRecord(hStream, pRecord, pHandle);
    }
    else
    {
        // Pass back a 'non-null' handle, emulating a completed request
        *pHandle = IGNORED_HANDLE;
    }

    return rc;
}

// Allow overriding the ism_store_updateRecord function
bool updateRecordIgnoreAll = false;
int32_t ism_store_updateRecord(ismStore_StreamHandle_t hStream,
                               ismStore_Handle_t handle,
                               uint64_t attribute,
                               uint64_t state,
                               uint8_t  flags)
{
    int32_t rc = OK;
    bool ignore = updateRecordIgnoreAll;
    static int32_t (*real_ism_store_updateRecord)(ismStore_StreamHandle_t,ismStore_Handle_t,uint64_t,uint64_t,uint8_t) = NULL;

    if (real_ism_store_updateRecord == NULL)
    {
        real_ism_store_updateRecord = dlsym(RTLD_NEXT, "ism_store_updateRecord");
    }

    if (createRecordIgnoreTypes != NULL)
    {
        if (handle == IGNORED_HANDLE) ignore = true;
    }

    if (!ignore)
    {
        rc = real_ism_store_updateRecord(hStream, handle, attribute, state, flags);
    }

    return rc;
}

//****************************************************************************
/// @brief Check for the existence of a specific durable subscription
//****************************************************************************
int32_t test_checkDurableSubscription(ismEngine_ClientStateHandle_t hClient,
                                      char *pName,
                                      bool bExpected,
                                      ismQueueType_t expectedType,
                                      int32_t expectedDepth,
                                      const ismEngine_SubscriptionAttributes_t *pExpectedSubAttrs,
                                      uint32_t expectedInternalAttrs,
                                      char *expectedNameProperty,
                                      ismEngine_Subscription_t **ppReturnedSub)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    ismEngine_Subscription_t *pSubscription = NULL;

#ifdef ENGINE_FORCE_INTERMEDIATE_TO_MULTI
    if (expectedType == intermediate) expectedType = multiConsumer;
#endif

    rc = iett_findClientSubscription(pThreadData,
                                     ((ismEngine_ClientState_t *)hClient)->pClientId,
                                     iett_generateClientIdHash(((ismEngine_ClientState_t *)hClient)->pClientId),
                                     pName,
                                     iett_generateSubNameHash(pName),
                                     &pSubscription);
    if (bExpected)
    {
        ismEngine_QueueStatistics_t stats;

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(pSubscription);
        TEST_ASSERT_EQUAL(ieq_getQType(pSubscription->queueHandle), expectedType);
        TEST_ASSERT_EQUAL_FORMAT(pSubscription->subOptions,
                                 (pExpectedSubAttrs->subOptions & ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK),
                                 "0x%08x");
        TEST_ASSERT_EQUAL(pSubscription->subId, pExpectedSubAttrs->subId)
        TEST_ASSERT_EQUAL_FORMAT(pSubscription->internalAttrs, expectedInternalAttrs, "0x%08x");
        if (expectedNameProperty != NULL)
        {
            ism_field_t nameField = { 0 };
            concat_alloc_t flatProp = { pSubscription->flatSubProperties
                                      , pSubscription->flatSubPropertiesLength
                                      , pSubscription->flatSubPropertiesLength
                                      , 0
                                      , false };

            rc = ism_common_findPropertyName( &flatProp
                                            , "Name"
                                            , &nameField );
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_STRINGS_EQUAL(expectedNameProperty, nameField.val.s);
        }
        else
        {
            TEST_ASSERT_PTR_NULL(pSubscription->flatSubProperties);
            TEST_ASSERT_EQUAL(pSubscription->flatSubPropertiesLength, 0);
        }

        ieq_getStats(pThreadData, pSubscription->queueHandle, &stats);
        ieq_setStats(pSubscription->queueHandle, NULL, ieqSetStats_RESET_ALL);

        //printf("TM: %d %d\n", stats.BufferedMsgsMsgs, stats.BufferedMsgsHWM);

        // Check we get the right list of clients for this subscription
        const char **foundClients = NULL;
        rc = iett_findSubscriptionClientIds(pThreadData, pSubscription, &foundClients);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(foundClients);
        TEST_ASSERT_STRINGS_EQUAL(foundClients[0], ((ismEngine_ClientState_t *)hClient)->pClientId);
        TEST_ASSERT_PTR_NULL(foundClients[1]);
        iett_freeSubscriptionClientIds(pThreadData, foundClients);

        TEST_ASSERT_EQUAL(stats.BufferedMsgs, expectedDepth);
    }
    else
    {
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }

    if (NULL != ppReturnedSub)
    {
        *ppReturnedSub = pSubscription;
    }
    // Release the subscription if we found it
    else if (rc == OK)
    {
        rc = iett_releaseSubscription(pThreadData, pSubscription, false);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    return rc;
}

//****************************************************************************
/// @brief Test the use of durable subscriptions
//****************************************************************************
#define DURABLE_SUBSCRIPTIONS_TEST_TOPIC "Durable/Topic"

typedef struct tag_listDurableCbContext_t
{
    int32_t             foundSubs;
    char               *expectedTopic; // Optional topic string to check
    bool                checkSubId;    // Whether to check the subId returned
    ismEngine_SubId_t   expectedSubId; // If checking, expected SubId
} listDurableCbContext_t;

void listDurableCallback(ismEngine_SubscriptionHandle_t subHandle,
                         const char * subName,
                         const char * pTopicString,
                         void * subProperties,
                         size_t   subPropertiesLength,
                         const ismEngine_SubscriptionAttributes_t *pSubAttrs,
                         uint32_t consumerCount,
                         void *   pContext)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    listDurableCbContext_t *context = (listDurableCbContext_t *)pContext;
    ismEngine_Subscription_t *subscription = (ismEngine_Subscription_t *)subHandle;

    TEST_ASSERT_PTR_NOT_NULL(subscription);
    TEST_ASSERT_NOT_EQUAL(subscription->useCount, 0);

    if (context->expectedTopic != NULL)
    {
        TEST_ASSERT_STRINGS_EQUAL(pTopicString, context->expectedTopic);
        TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, context->expectedTopic);
    }

    if (context->checkSubId)
    {
        TEST_ASSERT_EQUAL(pSubAttrs->subId, context->expectedSubId);
    }

    // Get and release this subscription, checking use counts are updated as expected
    uint32_t oldUseCount = subscription->useCount;
    ismEngine_Subscription_t *gotSubscription;

    int32_t rc = iett_findClientSubscription(pThreadData,
                                             subscription->clientId,
                                             subscription->clientIdHash,
                                             subName,
                                             iett_generateSubNameHash(subName),
                                             &gotSubscription);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(subscription, gotSubscription);
    TEST_ASSERT_EQUAL(oldUseCount+1, gotSubscription->useCount);

    iett_acquireSubscriptionReference(gotSubscription);

    TEST_ASSERT_EQUAL(oldUseCount+2, gotSubscription->useCount);

    iett_releaseSubscription(pThreadData, gotSubscription, false);

    TEST_ASSERT_EQUAL(oldUseCount+1, gotSubscription->useCount);

    iett_releaseSubscription(pThreadData, gotSubscription, false);

    TEST_ASSERT_EQUAL(subscription->useCount, oldUseCount);

    context->foundSubs++;

#if 0
    printf("SUBHANDLE: %p\n", subHandle);
    printf("SUBNAME: '%s'\n", subName);
    printf("TOPICSTRING: '%s'\n", pTopicString);
    printf("SUBPROPERTIES: %p\n", subProperties);
    printf("SUBPROPERTIESLENGTH: %ld\n", subPropertiesLength);
    printf("SUBOPTIONS: 0x%08x\n", pSubAttrs->subOptions);
    printf("SUBID: %u\n", pSubAttrs->subId);
    printf("CONSUMERCOUNT: %u\n", consumerCount);
    printf("\n");
#endif
}

// -----------------------------------------------------------------
// Test the counting of durable owner objects
// -----------------------------------------------------------------
void test_ownerCounts(uint32_t *clientCount, uint32_t *subCount)
{
    int32_t rc;
    char *outputString = NULL;

    rc =  ism_engine_diagnostics(ediaVALUE_MODE_OWNERCOUNTS,
                                 NULL,
                                 &outputString,
                                 NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(outputString);

    // Extract the contents of the output

    ism_json_parse_t parseObj;
    ism_json_entry_t ents[10];

    memset(&parseObj, 0, sizeof(parseObj));

    parseObj.ent = ents;
    parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseObj.source = strdup(outputString);
    parseObj.src_len = strlen(parseObj.source);

    rc = ism_json_parse(&parseObj);
    TEST_ASSERT_EQUAL(rc, OK);

    *clientCount = (uint32_t)(ism_json_getInt(&parseObj, ediaVALUE_OUTPUT_CLIENTOWNERCOUNT, -1));
    *subCount = (uint32_t)(ism_json_getInt(&parseObj, ediaVALUE_OUTPUT_SUBOWNERCOUNT, -1));

    if (parseObj.free_ent) ism_common_free(ism_memory_utils_parser,parseObj.ent);
    free(parseObj.source);

    ism_engine_freeDiagnosticsOutput(outputString);
}

void test_capability_DurableSubscriptions(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SessionHandle_t     hSession1, hSession2;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("UT_Client1",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession("UT_Client2",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient2, &hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);

    iett_destroyEngineTopicTree(pThreadData);

    iett_initEngineTopicTree(pThreadData);

    ismEngine_Subscription_t *tmpSubscription[3] = {NULL};

    // Access a non-existent subscription name
    rc = iett_findClientSubscription(pThreadData,
                                     ((ismEngine_ClientState_t *)hClient1)->pClientId,
                                     iett_generateClientIdHash(((ismEngine_ClientState_t *)hClient1)->pClientId),
                                     "Non Existent Subscription Name",
                                     iett_generateSubNameHash("Non Existent Subscription Name"),
                                     NULL);

    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    // Delete a non-existent named subscription
    rc = ism_engine_destroySubscription(hClient1,
                                        "Non Existent Subscription Name",
                                        hClient1,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    // Fake the store being in trouble, and try and create a subscription...
    iest_storeEventHandler(ISM_STORE_EVENT_MGMT0_ALERT_ON, NULL);
    iest_storeEventHandler(ISM_STORE_EVENT_MGMT1_ALERT_ON, NULL);
    iest_storeEventHandler(ISM_STORE_EVENT_DISK_ALERT_ON, NULL);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

    rc = ism_engine_createSubscription(hClient1,
                                       "No Memory Subscription",
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       DURABLE_SUBSCRIPTIONS_TEST_TOPIC,
                                       &subAttrs,
                                       NULL, // Owning client same as requesting client
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ServerCapacity);

    // Remove the fake store panic...
    iest_storeEventHandler(ISM_STORE_EVENT_MGMT0_ALERT_OFF, NULL);
    iest_storeEventHandler(ISM_STORE_EVENT_MGMT1_ALERT_OFF, NULL);
    iest_storeEventHandler(ISM_STORE_EVENT_DISK_ALERT_OFF, NULL);

    // We know the topic and subId we expect to see
    listDurableCbContext_t context = {0, DURABLE_SUBSCRIPTIONS_TEST_TOPIC};

    rc = ism_engine_listSubscriptions(hClient1, NULL, &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 0);

    // Look for a specific subscription, that should not exist yet
    context.foundSubs = 0;
    rc = ism_engine_listSubscriptions(hClient1,
                                      "Durable Subscription 1",
                                      &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 0);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
    subAttrs.subId = (ismEngine_SubId_t)((rand()%9999)+1);

    rc = sync_ism_engine_createSubscription(hClient1,
                                            "Durable Subscription 1",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            DURABLE_SUBSCRIPTIONS_TEST_TOPIC,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client

    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_findClientSubscription(pThreadData,
                                     ((ismEngine_ClientState_t *)hClient1)->pClientId,
                                     iett_generateClientIdHash(((ismEngine_ClientState_t *)hClient1)->pClientId),
                                     "Durable Subscription 1",
                                     iett_generateSubNameHash("Durable Subscription 1"),
                                     &tmpSubscription[0]);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(tmpSubscription[0]);
    TEST_ASSERT_STRINGS_EQUAL(tmpSubscription[0]->subName, "Durable Subscription 1");
    TEST_ASSERT_STRINGS_EQUAL(tmpSubscription[0]->clientId, "UT_Client1");
    TEST_ASSERT_EQUAL(tmpSubscription[0]->subId, subAttrs.subId);

    rc = iett_findQHandleSubscription(pThreadData, tmpSubscription[0]->queueHandle, &tmpSubscription[1]);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(tmpSubscription[0], tmpSubscription[1]);

    iett_releaseSubscription(pThreadData, tmpSubscription[0], false);
    iett_releaseSubscription(pThreadData, tmpSubscription[1], false);

    // Take the opportunity to check the client and subCount logic
    uint32_t clientCount = 0;
    uint32_t subCount = 0;

    test_ownerCounts(&clientCount, &subCount);

    TEST_ASSERT_EQUAL(clientCount, 2);
    TEST_ASSERT_EQUAL(subCount, 1);

    iettSubscriberList_t list = {0};
    list.topicString = DURABLE_SUBSCRIPTIONS_TEST_TOPIC;
    rc = iett_getSubscriberList(pThreadData, &list);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 1);
    TEST_ASSERT_EQUAL_FORMAT(tmpSubscription[0], list.subscribers[0], "%p");

    iett_releaseSubscriberList(pThreadData, &list);

    context.checkSubId = true;
    context.expectedSubId = subAttrs.subId;

    rc = ism_engine_listSubscriptions(hClient1, NULL, &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 1);

    context.foundSubs = 0;
    rc = ism_engine_listSubscriptions(hClient1,
                                      "Durable Subscription 1",
                                      &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 1);

    context.foundSubs = 0;
    rc = ism_engine_listSubscriptions(hClient1,
                                      "Non-existent durable",
                                      &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 0);

    context.foundSubs = 0;
    rc = ism_engine_listSubscriptions(hClient2, NULL, &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 0);

    context.foundSubs = 0;
    rc = ism_engine_listSubscriptions(hClient2,
                                      "Durable Subscription 1",
                                      &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 0);

    // Create Sample Property2
    char *subProperty2 = "Properties 2";
    ism_prop_t *testProperties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(testProperties);
    ism_field_t NameProperty = {VT_String, 0, {.s = subProperty2 }};
    rc = ism_common_setProperty(testProperties, "Name", &NameProperty);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;

    rc = sync_ism_engine_createSubscription(hClient2,
                                            "Durable Subscription 2",
                                            testProperties,
                                            ismDESTINATION_TOPIC,
                                            DURABLE_SUBSCRIPTIONS_TEST_TOPIC,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_findClientSubscription(pThreadData,
                                     ((ismEngine_ClientState_t *)hClient2)->pClientId,
                                     iett_generateClientIdHash(((ismEngine_ClientState_t *)hClient2)->pClientId),
                                     "Durable Subscription 2",
                                     iett_generateSubNameHash("Durable Subscription 2"),
                                     &tmpSubscription[1]);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(tmpSubscription[1]);
    TEST_ASSERT_STRINGS_EQUAL(tmpSubscription[1]->clientId, "UT_Client2");

    rc = iett_releaseSubscription(pThreadData, tmpSubscription[1], false);

    // Wrong subscription name for this client (right for other)
    rc = iett_findClientSubscription(pThreadData,
                                     ((ismEngine_ClientState_t *)hClient2)->pClientId,
                                     iett_generateClientIdHash(((ismEngine_ClientState_t *)hClient2)->pClientId),
                                     "Durable Subscription 1",
                                     iett_generateSubNameHash("Durable Subscription 1"),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    memset(&list, 0, sizeof(iettSubscriberList_t));
    list.topicString = DURABLE_SUBSCRIPTIONS_TEST_TOPIC;

    rc = iett_getSubscriberList(pThreadData, &list);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 2);
    TEST_ASSERT_PTR_NOT_NULL(list.subscribers[0]);
    TEST_ASSERT_PTR_NOT_NULL(list.subscribers[1]);

    iett_releaseSubscriberList(pThreadData, &list);

    int32_t               messageCount;
    ismEngine_Consumer_t *consumer;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;

    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "Durable Subscription 1",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &messageCount,
                                   sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &consumer,
                                   NULL, 0, NULL);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL_FORMAT(consumer->engineObject, tmpSubscription[0], "%p");

    // Check unable to destroy with active consumer
    rc = ism_engine_destroySubscription(hClient1,
                                        "Durable Subscription 1",
                                        hClient1,
                                        NULL, 0, NULL);

    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationInUse);

    // Check attempt to create consumer on non-existent subscription
    rc = ism_engine_createConsumer(hSession2,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "Non Existent subscription Name",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &messageCount,
                                   sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &consumer,
                                   NULL, 0, NULL);

    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);

    // Create a non-durable consumer on the same topic
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_TOPIC,
                                   DURABLE_SUBSCRIPTIONS_TEST_TOPIC,
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &messageCount,
                                   sizeof(int32_t),
                                   NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &consumer,
                                   NULL, 0, NULL);

    TEST_ASSERT_EQUAL(rc, OK);

    memset(&list, 0, sizeof(iettSubscriberList_t));
    list.topicString = DURABLE_SUBSCRIPTIONS_TEST_TOPIC;

    rc = iett_getSubscriberList(pThreadData, &list);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 3);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = sync_ism_engine_createSubscription(hClient2,
                                            "Subscription 2B",
                                            testProperties,
                                            ismDESTINATION_TOPIC,
                                            DURABLE_SUBSCRIPTIONS_TEST_TOPIC,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    // Create the same subscription a second time - expect an error
    rc = sync_ism_engine_createSubscription(hClient2,
                                            "Subscription 2B",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            DURABLE_SUBSCRIPTIONS_TEST_TOPIC,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, ISMRC_ExistingSubscription);

    test_ownerCounts(&clientCount, &subCount);

    TEST_ASSERT_EQUAL(clientCount, 2);
    TEST_ASSERT_EQUAL(subCount, 3);

    // Force a failure from a lower level function - which we will only ever hit (we think)
    // if multiple threads are creating the same durable at the same time...
    ismEngine_Subscription_t *foundSubscription = NULL;
    rc = iett_findClientSubscription(pThreadData,
                                     "UT_Client2",
                                     iett_generateClientIdHash("UT_Client2"),
                                     "Subscription 2B",
                                     iett_generateSubNameHash("Subscription 2B"),
                                     &foundSubscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(foundSubscription);

    rc = iett_addSubToEngineTopic(pThreadData, DURABLE_SUBSCRIPTIONS_TEST_TOPIC, foundSubscription, NULL, false, false);
    TEST_ASSERT_EQUAL(rc, ISMRC_ExistingSubscription);

    rc = iett_releaseSubscription(pThreadData, foundSubscription, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // ism_store_dump("stdout");

    /* Purposefully not releasing list for the moment */

    iettSubscriberList_t list2 = {0};
    list2.topicString = DURABLE_SUBSCRIPTIONS_TEST_TOPIC;

    rc = iett_getSubscriberList(pThreadData, &list2);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list2.subscriberCount, 4);

    iett_releaseSubscriberList(pThreadData, &list2);

    rc = ism_engine_listSubscriptions(hClient2, NULL, &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 2);

    context.foundSubs = 0;
    rc = ism_engine_listSubscriptions(hClient2, "Subscription 2B", &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 1);

    rc = ism_engine_destroySubscription(hClient2,
                                        "Subscription 2B",
                                        hClient2,
                                        NULL, 0, NULL);

    // Check that destroy returns OK even though the subscription could not yet be freed
    TEST_ASSERT_EQUAL(rc, OK);

    // To prove ASAN fails if freed: iett_freeSubscription(foundSubscription, false);

    // Prove that it's been logically deleted and not yet freed. The first by checking the
    // internal attributes, the second by dereferencing the subscription (which, if it has
    // been freed will cause an address sanitizer fault in an ASAN_BUILD=yes build).
    TEST_ASSERT_EQUAL((foundSubscription->internalAttrs & iettSUBATTR_DELETED), iettSUBATTR_DELETED)
    TEST_ASSERT_NOT_EQUAL(foundSubscription->StrucId[0], '?'); // iett_freeSubscription not called

    rc = iett_findClientSubscription(pThreadData,
                                     ((ismEngine_ClientState_t *)hClient2)->pClientId,
                                     iett_generateClientIdHash(((ismEngine_ClientState_t *)hClient2)->pClientId),
                                     "Subscription 2B",
                                     iett_generateSubNameHash("Subscription 2B"),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    iett_releaseSubscriberList(pThreadData, &list); // Now release the list

    context.foundSubs = 0;

    rc = ism_engine_listSubscriptions(hClient1, NULL, &context, listDurableCallback);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 1);

    context.foundSubs = 0;
    rc = ism_engine_listSubscriptions(hClient2, "Subscription 2B", &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 0);

    rc = ism_engine_destroySession(hSession1,
                                   NULL,
                                   0,
                                   NULL);

    TEST_ASSERT_EQUAL(rc, OK);

    /* Session1 gone */

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    /* Client1 gone */

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    /* Client2 & Session2 both gone */

    memset(&list, 0, sizeof(iettSubscriberList_t));
    list.topicString = DURABLE_SUBSCRIPTIONS_TEST_TOPIC;

    rc = iett_getSubscriberList(pThreadData, &list);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 2); // Durables survive destroyClientState
    TEST_ASSERT_PTR_NOT_NULL(list.subscribers[0]);
    TEST_ASSERT_PTR_NOT_NULL(list.subscribers[1]);

    iett_releaseSubscriberList(pThreadData, &list);

    rc = sync_ism_engine_createClientState("UT_Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &hClient1);

    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    context.foundSubs = 0;

    rc = ism_engine_createSession(hClient1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_listSubscriptions(hClient1, NULL, &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 1);

    context.foundSubs = 0;
    rc = ism_engine_listSubscriptions(hClient1, "Durable Subscription 1", &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 1);

    rc = iett_findClientSubscription(pThreadData,
                              ((ismEngine_ClientState_t *)hClient1)->pClientId,
                              iett_generateClientIdHash(((ismEngine_ClientState_t *)hClient1)->pClientId),
                              "Durable Subscription 1",
                              iett_generateSubNameHash("Durable Subscription 1"),
                              &tmpSubscription[0]);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(tmpSubscription[0]);
    TEST_ASSERT_STRINGS_EQUAL(tmpSubscription[0]->subName, "Durable Subscription 1");
    TEST_ASSERT_STRINGS_EQUAL(tmpSubscription[0]->clientId, "UT_Client1");

    rc = iett_releaseSubscription(pThreadData, tmpSubscription[0], false);

    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySubscription(hClient1,
                                        "Durable Subscription 1",
                                        hClient1,
                                        NULL, 0, NULL);

    TEST_ASSERT_EQUAL(rc, OK);

    // Check attempt to create consumer on previously valid subscription now deleted
    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "Durable Subscription 1",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &messageCount,
                                   sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &consumer,
                                   NULL, 0, NULL);

    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);

    (void)ism_common_freeProperties(testProperties);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);
}

#define INVALID_SHARING_TEST_TOPIC "Invalid/Sharing"
uint32_t INVALID_SHARING_SUBOPT[] = { ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE,
                                      ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                                      ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                                      ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE };
#define INVALID_SHARING_SUBOPT_COUNT (sizeof(INVALID_SHARING_SUBOPT)/sizeof(INVALID_SHARING_SUBOPT[0]))

void test_InvalidSharing(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession1, hSession2;
    ismEngine_Consumer_t         *consumer1, *consumer2;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("InvalidSharing",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession2,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t i=0; i<INVALID_SHARING_SUBOPT_COUNT; i++)
    {
        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                        INVALID_SHARING_SUBOPT[i] };

        rc = sync_ism_engine_createSubscription(hClient,
                                                "Non-Shared Subscription",
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                INVALID_SHARING_TEST_TOPIC,
                                                &subAttrs,
                                                hClient);
        TEST_ASSERT_EQUAL(rc, OK);

        // Create a consumer on Session1
        rc = ism_engine_createConsumer(hSession1,
                                       ismDESTINATION_SUBSCRIPTION,
                                       "Non-Shared Subscription",
                                       NULL,
                                       hClient,
                                       NULL, 0, NULL, /* No delivery callback */
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &consumer1,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // Try to create a second consumer on Session2
        rc = ism_engine_createConsumer(hSession2,
                                       ismDESTINATION_SUBSCRIPTION,
                                       "Non-Shared Subscription",
                                       NULL,
                                       hClient,
                                       NULL, 0, NULL, /* No delivery callback */
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &consumer2,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_WaiterInUse);

        rc = sync_ism_engine_destroyConsumer(consumer1);
        TEST_ASSERT_EQUAL(rc, OK);

        // Try a consumer from the 2nd session again, it should now work
        rc = ism_engine_createConsumer(hSession2,
                                       ismDESTINATION_SUBSCRIPTION,
                                       "Non-Shared Subscription",
                                       NULL,
                                       hClient,
                                       NULL, 0, NULL, /* No delivery callback */
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &consumer2,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = sync_ism_engine_destroyConsumer(consumer2);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_destroySubscription(hClient,
                                            "Non-Shared Subscription",
                                            hClient, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = sync_ism_engine_destroyClientState(hClient,
                                            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
}

#define NONSHARED_UPDATE_TEST_TOPIC "NonShared/Update/Topic"
#define NONSHARED_UPDATE_SUBNAME "Non-Shared Update Sub"
void test_capability_UpdateNonShared(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t     hSession1;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("UT_Client1",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_Subscription_t *tmpSubscription = NULL;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

    subAttrs.subId = 100;

    rc = sync_ism_engine_createSubscription(hClient1,
                                            NONSHARED_UPDATE_SUBNAME,
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            NONSHARED_UPDATE_TEST_TOPIC,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    // We know the topic and subId we expect to see
    listDurableCbContext_t context = {0, NONSHARED_UPDATE_TEST_TOPIC, true, subAttrs.subId};

    // Find it with no specified subName
    rc = ism_engine_listSubscriptions(hClient1,
                                      NULL,
                                      &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 1);

    // Find it with a specified subName
    context.foundSubs = 0;
    rc = ism_engine_listSubscriptions(hClient1,
                                      NONSHARED_UPDATE_SUBNAME,
                                      &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 1);

    // Get access to the ismEngine_Subscription_t itself
    rc = iett_findClientSubscription(pThreadData,
                                     ((ismEngine_ClientState_t *)hClient1)->pClientId,
                                     iett_generateClientIdHash(((ismEngine_ClientState_t *)hClient1)->pClientId),
                                     NONSHARED_UPDATE_SUBNAME,
                                     iett_generateSubNameHash(NONSHARED_UPDATE_SUBNAME),
                                     &tmpSubscription);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(tmpSubscription);
    TEST_ASSERT_STRINGS_EQUAL(tmpSubscription->clientId, "UT_Client1");

    ismStore_Handle_t originalProps = ieq_getPropsHdl(tmpSubscription->queueHandle);
    TEST_ASSERT_NOT_EQUAL(originalProps, ismSTORE_NULL_HANDLE);

    context.expectedSubId = subAttrs.subId;

    // Try and change the subId using ism_engine_reuseSubscription
    rc = ism_engine_reuseSubscription(hClient1, NONSHARED_UPDATE_SUBNAME, &subAttrs, hClient1);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);

    // Properties should not have been updated
    ismStore_Handle_t newProps = ieq_getPropsHdl(tmpSubscription->queueHandle);
    TEST_ASSERT_NOT_EQUAL(newProps, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_EQUAL(newProps, originalProps);

    context.foundSubs = 0;
    rc = ism_engine_listSubscriptions(hClient1,
                                      NULL,
                                      &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 1);

    // Update with the new subId (50)
    ism_prop_t *propertyUpdates = ism_common_newProperties(10);
    ism_field_t f = {0};

    f.type = VT_UInt;
    context.expectedSubId = f.val.u = 50;

    ism_common_setProperty(propertyUpdates, ismENGINE_UPDATE_SUBSCRIPTION_PROPERTY_SUBID, &f);
    rc = ism_engine_updateSubscription(hClient1,
                                       NONSHARED_UPDATE_SUBNAME,
                                       propertyUpdates,
                                       hClient1,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // A store update should have occurred
    newProps = ieq_getPropsHdl(tmpSubscription->queueHandle);
    TEST_ASSERT_NOT_EQUAL(originalProps, newProps);
    originalProps = newProps;

    context.foundSubs = 0;
    rc = ism_engine_listSubscriptions(hClient1,
                                      NONSHARED_UPDATE_SUBNAME,
                                      &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 1);

    // Try altering an option I shouldn't be allowed to (ismENGINE_SUBSCRIPTION_OPTION_DURABLE not there)
    f.val.u = ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    ism_common_setProperty(propertyUpdates, ismENGINE_UPDATE_SUBSCRIPTION_PROPERTY_SUBOPTIONS, &f);
    rc = ism_engine_updateSubscription(hClient1,
                                       NONSHARED_UPDATE_SUBNAME,
                                       propertyUpdates,
                                       hClient1,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    // UPDATE SubOptions with one I should be able to change
    subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DELIVER_RETAIN_AS_PUBLISHED;
    f.val.u = subAttrs.subOptions;
    ism_common_setProperty(propertyUpdates, ismENGINE_UPDATE_SUBSCRIPTION_PROPERTY_SUBOPTIONS, &f);
    rc = ism_engine_updateSubscription(hClient1,
                                       NONSHARED_UPDATE_SUBNAME,
                                       propertyUpdates,
                                       hClient1,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // A store update should have occurred
    newProps = ieq_getPropsHdl(tmpSubscription->queueHandle);
    TEST_ASSERT_NOT_EQUAL(originalProps, newProps);
    originalProps = newProps;

    TEST_ASSERT_EQUAL(tmpSubscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVER_RETAIN_AS_PUBLISHED,
                      ismENGINE_SUBSCRIPTION_OPTION_DELIVER_RETAIN_AS_PUBLISHED);

    // Update SubOptions a couple more times, changing selection
    subAttrs.subOptions &= ~ismENGINE_SUBSCRIPTION_OPTION_DELIVER_RETAIN_AS_PUBLISHED;
    uint32_t prevActiveSelection = tmpSubscription->node->activeSelection;
    for(int32_t loop=0; loop<4; loop++)
    {
        switch(loop)
        {
            case 0:
                f.val.u = subAttrs.subOptions | ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL;
                break;
            case 1:
                f.val.u = subAttrs.subOptions | ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY;
                break;
            case 2:
                f.val.u = subAttrs.subOptions | ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY;
                break;
            case 3:
                f.val.u = subAttrs.subOptions;
                break;
            case 4:
                break;
            default:
                TEST_ASSERT_GREATER_THAN(3, loop);
                break;
        }

        ism_common_setProperty(propertyUpdates, ismENGINE_UPDATE_SUBSCRIPTION_PROPERTY_SUBOPTIONS, &f);
        rc = ism_engine_updateSubscription(hClient1,
                                           NONSHARED_UPDATE_SUBNAME,
                                           propertyUpdates,
                                           hClient1,
                                           NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        newProps = ieq_getPropsHdl(tmpSubscription->queueHandle);

        // Check that the properties handle changed as expected
        if (loop < 4)
        {
            TEST_ASSERT_NOT_EQUAL(originalProps, newProps);
            originalProps = newProps;
        }
        else
        {
            TEST_ASSERT_EQUAL(originalProps, newProps);
        }

        // Check that active selection on the node changed as expected.
        if (loop < 3)
        {
            TEST_ASSERT_NOT_EQUAL(prevActiveSelection, tmpSubscription->node->activeSelection);
        }
        else
        {
            TEST_ASSERT_EQUAL(prevActiveSelection, tmpSubscription->node->activeSelection);
        }
    }

    iett_releaseSubscription(pThreadData, tmpSubscription, false);

    // Destroy the subscriptions
    rc = ism_engine_destroySubscription(hClient1,
                                        NONSHARED_UPDATE_SUBNAME,
                                        hClient1,
                                        NULL, 0, NULL);

    // Check that destroy returns OK
    TEST_ASSERT_EQUAL(rc, OK);

    ism_common_freeProperties(propertyUpdates);

    context.foundSubs = 0;
    rc = ism_engine_listSubscriptions(hClient1,
                                      NULL,
                                      &context, listDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 0);

    // Destroy the client
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);
}

CU_TestInfo ISM_TopicTree_CUnit_test_capability_DurableSubscriptions[] =
{
    { "DurableSubscriptions", test_capability_DurableSubscriptions },
    { "InvalidSharing", test_InvalidSharing },
    { "Update non-shared", test_capability_UpdateNonShared },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test rehydration of subscriptions after simulated restart
//****************************************************************************
char *REHYDRATION_TOPICS[] = {"TEST/REHYDRATION/TOPIC", "TEST/REHYDRATION/+", "TEST/REHYDRATION/#"};
#define REHYDRATION_NUMTOPICS (sizeof(REHYDRATION_TOPICS)/sizeof(REHYDRATION_TOPICS[0]))

typedef struct tag_rehydrationMessagesCbContext_t
{
    ismEngine_SessionHandle_t hSession;
    int32_t received;
} rehydrationMessagesCbContext_t;

bool rehydrationMessagesCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
    rehydrationMessagesCbContext_t *context = *((rehydrationMessagesCbContext_t **)pContext);

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

#define NON_JMS_DURABLE_SIMP_SUBNAME                  "NON.JMS.DURABLE.SIMP"
#define NON_JMS_DURABLE_SIMP_SUBID                    11
#define EMPTY_TOPIC_STRING_DURABLE_SIMP_SUBNAME       "EMPTY.TOPIC.STRING.DURABLE.SIMP"
#define EMPTY_TOPIC_STRING_DURABLE_SIMP_SUBID         22
#define EMPTY_TOPIC_STRING_DURABLE_SIMP_CHANGED_SUBID 99
#define NON_JMS_DURABLE_INTER_SUBNAME                 "NON.JMS.DURABLE.INTER"
#define NON_JMS_DURABLE_INTER_SUBID                   33
#define JMS_DURABLE_1_SUBNAME                         "JMS.DURABLE.1"
#define JMS_DURABLE_1_SUBID                           ismENGINE_NO_SUBID
#define JMS_DURABLE_2_SUBNAME                         "JMS.DURABLE.2"
#define JMS_DURABLE_2_SUBID                           ismENGINE_NO_SUBID

void test_capability_Rehydration_Phase1(void)
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient[5];
    ismEngine_SessionHandle_t hSession[5];
    ismEngine_MessageHandle_t hMessage;
    void *pPayload=NULL;

    printf("Starting %s...\n", __func__);

    for(uint32_t i=0;i<sizeof(hClient)/sizeof(hClient[0]);i++)
    {
        char clientId[10];

        sprintf(clientId, "Client%u", i+1);

        rc = test_createClientAndSession(clientId,
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hClient[i], &hSession[i], true);
    }

    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                                                    NON_JMS_DURABLE_SIMP_SUBID };

    rc = sync_ism_engine_createSubscription(hClient[0],
                                            NON_JMS_DURABLE_SIMP_SUBNAME,
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            REHYDRATION_TOPICS[rand()%REHYDRATION_NUMTOPICS],
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    // Empty topic string
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    subAttrs.subId = EMPTY_TOPIC_STRING_DURABLE_SIMP_SUBID;

    rc = sync_ism_engine_createSubscription(hClient[1],
                                            EMPTY_TOPIC_STRING_DURABLE_SIMP_SUBNAME,
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "",
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    subAttrs.subId = NON_JMS_DURABLE_INTER_SUBID;

    rc = sync_ism_engine_createSubscription(hClient[2],
                                            NON_JMS_DURABLE_INTER_SUBNAME,
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            REHYDRATION_TOPICS[rand()%REHYDRATION_NUMTOPICS],
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                          ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    subAttrs.subId = 0;
    rc = sync_ism_engine_createSubscription(hClient[3],
                                            JMS_DURABLE_1_SUBNAME,
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            REHYDRATION_TOPICS[rand()%REHYDRATION_NUMTOPICS],
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    // Create Sample Property
    char *subProperty = JMS_DURABLE_2_SUBNAME;
    ism_prop_t *testProperties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(testProperties);
    ism_field_t NameProperty = {VT_String, 0, {.s = subProperty }};
    rc = ism_common_setProperty(testProperties, "Name", &NameProperty);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                          ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    subAttrs.subId =
    rc = sync_ism_engine_createSubscription(hClient[4],
                                            JMS_DURABLE_2_SUBNAME,
                                            testProperties,
                                            ismDESTINATION_TOPIC,
                                            REHYDRATION_TOPICS[rand()%REHYDRATION_NUMTOPICS],
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish a single persistent message to all subscribers
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, REHYDRATION_TOPICS[0],
                            &hMessage,&pPayload);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession[0],
                                            ismDESTINATION_TOPIC,
                                            REHYDRATION_TOPICS[0],
                                            NULL,
                                            hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, "",
                            &hMessage,&pPayload);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession[0],
                                            ismDESTINATION_TOPIC,
                                            "",
                                            NULL,
                                            hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    free(pPayload);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    subAttrs.subId = NON_JMS_DURABLE_SIMP_SUBID;
    (void)test_checkDurableSubscription(hClient[0],
                                        NON_JMS_DURABLE_SIMP_SUBNAME,
                                        true,
                                        simple, 1,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER,
                                        NULL,
                                        NULL);
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    subAttrs.subId = EMPTY_TOPIC_STRING_DURABLE_SIMP_SUBID;
    (void)test_checkDurableSubscription(hClient[1],
                                        EMPTY_TOPIC_STRING_DURABLE_SIMP_SUBNAME,
                                        true,
                                        simple, 1,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER,
                                        NULL,
                                        NULL);
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    subAttrs.subId = NON_JMS_DURABLE_INTER_SUBID;
    (void)test_checkDurableSubscription(hClient[2],
                                        NON_JMS_DURABLE_INTER_SUBNAME,
                                        true,
                                        intermediate, 1,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER,
                                        NULL,
                                        NULL);
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                          ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    subAttrs.subId = JMS_DURABLE_1_SUBID;
    (void)test_checkDurableSubscription(hClient[3],
                                        JMS_DURABLE_1_SUBNAME,
                                        true,
                                        multiConsumer, 1,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT,
                                        NULL,
                                        NULL);
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                          ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    subAttrs.subId = JMS_DURABLE_2_SUBID;
    (void)test_checkDurableSubscription(hClient[4],
                                        JMS_DURABLE_2_SUBNAME,
                                        true,
                                        multiConsumer, 1,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT,
                                        subProperty,
                                        NULL);
}

void test_capability_Rehydration_PhaseN(int32_t phase)
{
    int32_t rc;

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hClient[5];
    ismEngine_SessionHandle_t hSession[5];

    printf("Starting %s (N=%d)...\n", __func__, phase);

    for(uint32_t i=0;i<sizeof(hClient)/sizeof(hClient[0]);i++)
    {
        char clientId[10];

        sprintf(clientId, "Client%u", i+1);

        rc = test_createClientAndSession(clientId,
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hClient[i], &hSession[i], true);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Create Sample Property
    char *subProperty = JMS_DURABLE_2_SUBNAME;
    ism_prop_t *testProperties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(testProperties);
    ism_field_t NameProperty = {VT_String, 0, {.s = subProperty }};
    rc = ism_common_setProperty(testProperties, "Name", &NameProperty);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                                                    NON_JMS_DURABLE_SIMP_SUBID };
    (void)test_checkDurableSubscription(hClient[0],
                                        NON_JMS_DURABLE_SIMP_SUBNAME,
                                        (phase<3)?true:false,
                                        simple, 0,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER | iettSUBATTR_REHYDRATED,
                                        NULL,
                                        NULL);
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    subAttrs.subId = phase > 2 ? EMPTY_TOPIC_STRING_DURABLE_SIMP_CHANGED_SUBID : EMPTY_TOPIC_STRING_DURABLE_SIMP_SUBID;
    (void)test_checkDurableSubscription(hClient[1],
                                        EMPTY_TOPIC_STRING_DURABLE_SIMP_SUBNAME,
                                        true,
                                        simple, 0,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER | iettSUBATTR_REHYDRATED,
                                        NULL,
                                        NULL);

    // Change the SUBID for this one subscription
    if (phase == 2)
    {
        ism_prop_t *pPropertyUpdates = ism_common_newProperties(1);
        ism_field_t f = {0};

        f.type = VT_UInt;
        f.val.u = subAttrs.subId = EMPTY_TOPIC_STRING_DURABLE_SIMP_CHANGED_SUBID;

        ism_common_setProperty(pPropertyUpdates, ismENGINE_UPDATE_SUBSCRIPTION_PROPERTY_SUBID, &f);

        rc = ism_engine_updateSubscription(hClient[1],
                                           EMPTY_TOPIC_STRING_DURABLE_SIMP_SUBNAME,
                                           pPropertyUpdates,
                                           hClient[1],
                                           NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        ism_common_freeProperties(pPropertyUpdates);

        (void)test_checkDurableSubscription(hClient[1],
                                            EMPTY_TOPIC_STRING_DURABLE_SIMP_SUBNAME,
                                            true,
                                            simple, 0,
                                            &subAttrs,
                                            iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER | iettSUBATTR_REHYDRATED,
                                            NULL,
                                            NULL);
    }

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    subAttrs.subId = NON_JMS_DURABLE_INTER_SUBID;
    (void)test_checkDurableSubscription(hClient[2],
                                        NON_JMS_DURABLE_INTER_SUBNAME,
                                        (phase<4)?true:false,
                                        intermediate, 1,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER | iettSUBATTR_REHYDRATED,
                                        NULL,
                                        NULL);
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                          ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    subAttrs.subId = JMS_DURABLE_1_SUBID;
    (void)test_checkDurableSubscription(hClient[3],
                                        JMS_DURABLE_1_SUBNAME,
                                        (phase<5)?true:false,
                                        multiConsumer, 1,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_REHYDRATED,
                                        NULL,
                                        NULL);
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                          ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    subAttrs.subId = JMS_DURABLE_2_SUBID;
    (void)test_checkDurableSubscription(hClient[4],
                                        JMS_DURABLE_2_SUBNAME,
                                        true,
                                        multiConsumer, 1,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_REHYDRATED,
                                        subProperty,
                                        NULL);

    switch(phase)
    {
    case 1:
        exit(111);
        break;
        case 2:
            rc = ism_engine_destroySubscription(hClient[0],
                                                NON_JMS_DURABLE_SIMP_SUBNAME,
                                                hClient[0],
                                                NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
            break;
        case 3:
            rc = ism_engine_destroySubscription(hClient[2],
                                                NON_JMS_DURABLE_INTER_SUBNAME,
                                                hClient[2],
                                                NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
            break;
        case 4:
            rc = ism_engine_destroySubscription(hClient[3],
                                                JMS_DURABLE_1_SUBNAME,
                                                hClient[3],
                                                NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
            break;
        default:
            break;
    }

    if (phase == 5)
    {
        // Force a deletion that should only occur at restart
        ismEngine_Subscription_t *pSubscription = NULL;

        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                              ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                              ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
        subAttrs.subId = JMS_DURABLE_2_SUBID;
        rc = test_checkDurableSubscription(hClient[4],
                                           JMS_DURABLE_2_SUBNAME,
                                           true,
                                           multiConsumer, 1,
                                           &subAttrs,
                                           iettSUBATTR_PERSISTENT | iettSUBATTR_REHYDRATED,
                                           subProperty,
                                           &pSubscription);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = iemq_markQDeleted(pThreadData, pSubscription->queueHandle, true);

        rc = iett_releaseSubscription(pThreadData, pSubscription, false);

        // Should still be there until restart
        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                              ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                              ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
        subAttrs.subId = JMS_DURABLE_2_SUBID;
        (void)test_checkDurableSubscription(hClient[4],
                                            JMS_DURABLE_2_SUBNAME,
                                            true,
                                            multiConsumer, 1,
                                            &subAttrs,
                                            iettSUBATTR_PERSISTENT | iettSUBATTR_REHYDRATED,
                                            subProperty,
                                            NULL);
    }

    (void)ism_common_freeProperties(testProperties);
}

void test_capability_Rehydration_Phase2(void)
{
    test_capability_Rehydration_PhaseN(2);
}

void test_capability_Rehydration_Phase3(void)
{
    test_capability_Rehydration_PhaseN(3);
}

void test_capability_Rehydration_Phase4(void)
{
    test_capability_Rehydration_PhaseN(4);
}

void test_capability_Rehydration_Phase5(void)
{
    test_capability_Rehydration_PhaseN(5);
}

void test_capability_Rehydration_PhaseFinal(void)
{
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    printf("Starting %s...\n", __func__);

    // Should now not be there
    rc = test_createClientAndSession("Client4",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    (void)test_checkDurableSubscription(hClient,
                                        JMS_DURABLE_2_SUBNAME,
                                        false,
                                        0, 0, 0, 0,
                                        NULL,
                                        NULL);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test what happens if during restart, an SPR is seen without an SDR
/// (which will happen if a subscription whose SPR has gone to a disk generation
/// is deleted)
//****************************************************************************
void test_capability_DiskGenerationSub_Phase1(void)
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    char *topicString = "/test/diskGenerationSub";

    // Create a client and session
    rc = test_createClientAndSession("diskGenerationSub",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    printf("Starting %s...\n", __func__);

    // Create a QoS0 durable subscription which should come back
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
    rc = sync_ism_engine_createSubscription(hClient,
                                            "DiskGenerationSubOK",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            topicString,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    // Make ism_store_createRecord ignore SDR creation, this means any subscriptions created
    // will only write an SPR, meaning they will appear to be orphaned during recovery.
    ismStore_RecordType_t ignoreSDR[] = {ISM_STORE_RECTYPE_SUBSC, 0};
    createRecordIgnoreTypes = ignoreSDR;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = sync_ism_engine_createSubscription(hClient,
                                            "DiskGenerationSubNOSDR",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            topicString,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    // All back to normal
    createRecordIgnoreTypes = NULL;

    updateRecordIgnoreAll = true;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = sync_ism_engine_createSubscription(hClient,
                                            "DiskGenerationSubSTILLCREATING",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            topicString,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    updateRecordIgnoreAll = false;

    // Check that all subscriptions appear in the topic tree
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    (void)test_checkDurableSubscription(hClient,
                                        "DiskGenerationSubOK",
                                        true,
                                        simple, 0,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER,
                                        NULL,
                                        NULL);
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    (void)test_checkDurableSubscription(hClient,
                                        "DiskGenerationSubNOSDR",
                                        true,
                                        simple, 0,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER,
                                        NULL,
                                        NULL);
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    (void)test_checkDurableSubscription(hClient,
                                        "DiskGenerationSubSTILLCREATING",
                                        true,
                                        simple, 0,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER,
                                        NULL,
                                        NULL);
}

void test_capability_DiskGenerationSub_PhaseN(int32_t phase)
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    printf("Starting %s (N=%d)...\n", __func__, phase);

    rc = test_createClientAndSession("diskGenerationSub",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);

    // Check that only the one with an SDR appears in the topic tree
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    (void)test_checkDurableSubscription(hClient,
                                        "DiskGenerationSubOK",
                                        true,
                                        simple, 0,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER | iettSUBATTR_REHYDRATED,
                                        NULL,
                                        NULL);
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    (void)test_checkDurableSubscription(hClient,
                                        "DiskGenerationSubNOSDR",
                                        false,
                                        simple, 0,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER,
                                        NULL,
                                        NULL);
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    (void)test_checkDurableSubscription(hClient,
                                        "DiskGenerationSubSTILLCREATING",
                                        false,
                                        simple, 0,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER,
                                        NULL,
                                        NULL);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_DiskGenerationSub_Phase2(void)
{
    test_capability_DiskGenerationSub_PhaseN(2);
}

void test_capability_DiskGenerationSub_Phase3(void)
{
    test_capability_DiskGenerationSub_PhaseN(3);
}

//****************************************************************************
/// @brief Test Asynchronous deletion of a durable when server restarts before
///        subscription has actually been deleted
//****************************************************************************
void test_capability_AsyncDeletion_Phase1(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t     hSession1;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("UT_Client1",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };
    // Should use intermediateQ
    rc = sync_ism_engine_createSubscription(hClient1,
                                            "Durable Subscription 1",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            DURABLE_SUBSCRIPTIONS_TEST_TOPIC,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    // Should use multiConsumerQ
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = sync_ism_engine_createSubscription(hClient1,
                                            "Durable Subscription 2",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            DURABLE_SUBSCRIPTIONS_TEST_TOPIC,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    // We won't delete this one, as a control
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = sync_ism_engine_createSubscription(hClient1,
                                            "Durable Subscription 3",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            DURABLE_SUBSCRIPTIONS_TEST_TOPIC,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    iettSubscriberList_t list = {0};
    list.topicString = DURABLE_SUBSCRIPTIONS_TEST_TOPIC;
    rc = iett_getSubscriberList(pThreadData, &list);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 3);

    // Get pointers to both subscriptions so we can check they are logically deleted
    // even though they could not be actually deleted yet
    ismEngine_Subscription_t *pSub1, *pSub2;

    rc = iett_findClientSubscription(pThreadData,
                                     "UT_Client1",
                                     iett_generateClientIdHash("UT_Client1"),
                                     "Durable Subscription 1",
                                     iett_generateSubNameHash("Durable Subscription 1"),
                                     &pSub1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL((pSub1->internalAttrs & iettSUBATTR_DELETED), 0);

    rc = iett_findClientSubscription(pThreadData,
                                     "UT_Client1",
                                     iett_generateClientIdHash("UT_Client1"),
                                     "Durable Subscription 2",
                                     iett_generateSubNameHash("Durable Subscription 2"),
                                     &pSub2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL((pSub2->internalAttrs & iettSUBATTR_DELETED), 0);

    // Check destroy returns OK even though node is in use
    rc = ism_engine_destroySubscription(hClient1,
                                        "Durable Subscription 1",
                                        hClient1,
                                        NULL, 0, NULL);

    TEST_ASSERT_EQUAL(rc, OK);
    // It is logically deleted
    TEST_ASSERT_EQUAL((pSub1->internalAttrs & iettSUBATTR_DELETED), iettSUBATTR_DELETED);

    rc = ism_engine_destroySubscription(hClient1,
                                        "Durable Subscription 2",
                                        hClient1,
                                        NULL, 0, NULL);

    TEST_ASSERT_EQUAL(rc, OK);
    // It is logically deleted
    TEST_ASSERT_EQUAL((pSub2->internalAttrs & iettSUBATTR_DELETED), iettSUBATTR_DELETED);

    iett_releaseSubscription(pThreadData, pSub1, false);
    iett_releaseSubscription(pThreadData, pSub2, false);
}

void test_capability_AsyncDeletion_Phase2(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    // Confirm that the deleted subs no longer come back in the new subscriber
    // lists - which confirms that there would be a publication gap if any
    // subscriptions were to come back once ISMRC_AsyncCompletion had been
    // returned on delete.
    iettSubscriberList_t newlist = {0};
    newlist.topicString = DURABLE_SUBSCRIPTIONS_TEST_TOPIC;
    rc = iett_getSubscriberList(pThreadData, &newlist);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(newlist.subscriberCount, 1);

    ismEngine_Subscription_t *pSubscription = NULL;

    // Check that the deleted ones have indeed been deleted
    rc = iett_findClientSubscription(pThreadData,
                                     "UT_Client1",
                                     iett_generateClientIdHash("UT_Client1"),
                                     "Durable Subscription 1",
                                     iett_generateSubNameHash("Durable Subscription 1"),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    rc = iett_findClientSubscription(pThreadData,
                                     "UT_Client1",
                                     iett_generateClientIdHash("UT_Client1"),
                                     "Durable Subscription 2",
                                     iett_generateSubNameHash("Durable Subscription 2"),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    // Check that the not deleted one is there
    rc = iett_findClientSubscription(pThreadData,
                                     "UT_Client1",
                                     iett_generateClientIdHash("UT_Client1"),
                                     "Durable Subscription 3",
                                     iett_generateSubNameHash("Durable Subscription 3"),
                                     &pSubscription);
    TEST_ASSERT_EQUAL(rc, OK);

    iett_releaseSubscription(pThreadData, pSubscription, false);

    // Confirm the result with iett_getSubscriberList
    iettSubscriberList_t list = {0};
    list.topicString = DURABLE_SUBSCRIPTIONS_TEST_TOPIC;

    rc = iett_getSubscriberList(pThreadData, &list);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 1);

    iett_releaseSubscriberList(pThreadData, &list);
}

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Rehydration_Phase1[] =
{
    { "Rehydration", test_capability_Rehydration_Phase1 },
    { "DiskGeneration", test_capability_DiskGenerationSub_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Rehydration_Phase2[] =
{
    { "Rehydration", test_capability_Rehydration_Phase2 },
    { "DiskGeneration", test_capability_DiskGenerationSub_Phase2 },
    { "AsyncDeletion", test_capability_AsyncDeletion_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Rehydration_Phase3[] =
{
    { "Rehydration", test_capability_Rehydration_Phase3 },
    { "DiskGeneration", test_capability_DiskGenerationSub_Phase3 },
    { "AsyncDeletion", test_capability_AsyncDeletion_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Rehydration_Phase4[] =
{
    { "Rehydration", test_capability_Rehydration_Phase4 },
    { "AsyncDeletion", test_capability_AsyncDeletion_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Rehydration_Phase5[] =
{
    { "Rehydration", test_capability_Rehydration_Phase5 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Rehydration_PhaseFinal[] =
{
    { "Rehydration", test_capability_Rehydration_PhaseFinal },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the creation of durable subscriptions with security in place
//****************************************************************************
#define TOPICSECURITY_POLICYNAME "TopicSecurityPolicy"

void test_capability_TopicSecurity_Phase1(void)
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient, hClient2;
    ismEngine_SessionHandle_t hSession, hSession2;
    ism_listener_t *mockListener=NULL, *mockListener2=NULL;
    ism_transport_t *mockTransport=NULL, *mockTransport2=NULL;
    ismSecurity_t *mockContext, *mockContext2;

    printf("Starting %s...\n", __func__);

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "SecureClient";

    rc = test_createSecurityContext(&mockListener2,
                                    &mockTransport2,
                                    &mockContext2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener2);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport2);
    TEST_ASSERT_PTR_NOT_NULL(mockContext2);

    mockTransport2->clientID = "Client";

    /* Create our clients and sessions */
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    rc = test_createClientAndSession(mockTransport2->clientID,
                                     mockContext2,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient2, &hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient2);
    TEST_ASSERT_PTR_NOT_NULL(hSession2);

    // Add a messaging policy that allows publish / subscribe on 'TEST.TOPICSECURITY.AUTHORIZED*'
    // For clients whose ID starts with 'SecureClient'.
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID."TOPICSECURITY_POLICYNAME"\","
                                "\"Name\":\""TOPICSECURITY_POLICYNAME"\","
                                "\"ClientID\":\"SecureClient*\","
                                "\"Topic\":\"TEST.TOPICSECURITY.AUTHORIZED*\","
                                "\"ActionList\":\"subscribe,publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContext, TOPICSECURITY_POLICYNAME);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext2, TOPICSECURITY_POLICYNAME);
    TEST_ASSERT_EQUAL(rc, OK);

    // Attempt to create various authorized / not-authorized subscriptions
    for(int32_t i=0; i<4; i++)
    {
        ismEngine_ClientStateHandle_t useClient;
        char *useTopic;
        char *useSubscriptionName;
        bool expectCreation;

        switch(i)
        {
            // AUTH: Topic not using wildcard
            case 0:
                useSubscriptionName = "DurableSubscription1";
                useClient = hClient;
                useTopic = "TEST.TOPICSECURITY.AUTHORIZED";
                expectCreation = true;
                break;
            // AUTH: Topic using wildcard
            case 1:
                useSubscriptionName = "DurableSubscription2";
                useClient = hClient;
                useTopic = "TEST.TOPICSECURITY.AUTHORIZED.SUBTOPIC";
                expectCreation = true;
                break;
            // NOTAUTH: Topic not authorized.
            case 2:
                useSubscriptionName = "FailedSubscription_WRONGTOPIC";
                useClient = hClient;
                useTopic = "TEST.TOPICSECURITY.NOT.AUTHORIZED";
                expectCreation = false;
                break;
            // NOTAUTH: Client not authorized
            case 3:
                useSubscriptionName = "FailedSubscription_WRONGCLIENT";
                useClient = hClient2;
                useTopic = "TEST.TOPICSECURITY.AUTHORIZED";
                expectCreation = false;
                break;
            default:
                TEST_ASSERT(false, ("Invalid loop value %d\n", i));
                break;
        }

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                        ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

        rc = sync_ism_engine_createSubscription(useClient,
                                                useSubscriptionName,
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                useTopic,
                                                &subAttrs,
                                                NULL); // Owning client same as requesting client
        if (expectCreation)
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            TEST_ASSERT_NOT_EQUAL(rc, OK);
        }

        (void)test_checkDurableSubscription(useClient,
                                            useSubscriptionName,
                                            expectCreation,
                                            intermediate, 0,
                                            &subAttrs,
                                            iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER,
                                            NULL,
                                            NULL);
    }

    int32_t               messageCount;
    ismEngine_Consumer_t *consumer;

    // Create consumer
    uint32_t subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "DurableSubscription1",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &messageCount,
                                   sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   test_getDefaultConsumerOptions(subOptions),
                                   &consumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_TopicSecurity_Phase2(void)
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    printf("Starting %s...\n", __func__);

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "SecureClient";

    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    (void)test_checkDurableSubscription(hClient,
                                        "DurableSubscription1",
                                        true,
                                        intermediate, 0,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER | iettSUBATTR_REHYDRATED,
                                        NULL,
                                        NULL);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    (void)test_checkDurableSubscription(hClient,
                                        "DurableSubscription2",
                                        true,
                                        intermediate, 0,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER | iettSUBATTR_REHYDRATED,
                                        NULL,
                                        NULL);

    // Change to disallow subscribe
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID."TOPICSECURITY_POLICYNAME"\","
                                "\"Name\":\""TOPICSECURITY_POLICYNAME"\","
                                "\"ClientID\":\"SecureClient*\","
                                "\"Topic\":\"TEST.TOPICSECURITY.AUTHORIZED*\","
                                "\"ActionList\":\"publish\","
                                "\"MaxMessageTimeToLive\":10,"
                                "\"Update\":\"true\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Try and create a consumer - that should now be unauthorized
    int32_t               messageCount;
    ismEngine_Consumer_t *consumer;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "DurableSubscription2",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &messageCount,
                                   sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &consumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    // Try publishing a message
    ismEngine_MessageHandle_t hMessage;
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, "TEST.TOPICSECURITY.AUTHORIZED.TTLTEST",
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(((ismEngine_Message_t *)hMessage)->Header.Expiry, 0);

    ismEngine_TransactionHandle_t hTran = NULL;
    rc = sync_ism_engine_createLocalTransaction(hSession, &hTran);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hTran);

    ismEngine_UnreleasedHandle_t hUnrel = NULL;

    rc = sync_ism_engine_putMessageWithDeliveryIdOnDestination(hSession,
                                                               ismDESTINATION_TOPIC,
                                                               "TEST.TOPICSECURITY.AUTHORIZED.TTLTEST",
                                                               hTran,
                                                               hMessage,
                                                               1,
                                                               &hUnrel);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
    TEST_ASSERT_PTR_NOT_NULL(hUnrel);

    rc = sync_ism_engine_commitTransaction(hSession,
                                           hTran,
                                           ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_removeUnreleasedDeliveryId(hSession, NULL, hUnrel);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_TopicSecurity_Phase3(void)
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    printf("Starting %s...\n", __func__);

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "SecureClient";

    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    (void)test_checkDurableSubscription(hClient,
                                        "DurableSubscription1",
                                        true,
                                        intermediate, 0,
                                        &subAttrs,
                                        iettSUBATTR_PERSISTENT | iettSUBATTR_SHARE_WITH_CLUSTER | iettSUBATTR_REHYDRATED,
                                        NULL,
                                        NULL);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test that updating a subscriptions policy works correctly
//****************************************************************************
void test_func_UpdatePolicy(void)
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    char *topicString = "/test/updatePolicy";

    printf("Starting %s...\n", __func__);

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "UpdatePolicyClient";

    // Create a non-durable client
    rc = test_createClientAndSession("UpdatePolicyClient",
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    // Add a messaging policy that allows publish / subscribe on '/test/updatePolicy'
    // For clients whose ID starts with 'SecureClient'.
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.UPDATEPOLICYNAME\","
                                "\"Name\":\"UPDATEPOLICYNAME\","
                                "\"ClientID\":\"UpdatePolicyClient*\","
                                "\"Topic\":\"/test/updatePolicy\","
                                "\"ActionList\":\"subscribe,publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContext, TOPICSECURITY_POLICYNAME);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE };

    rc = sync_ism_engine_createSubscription(hClient,
                                            "UpdatePolicySub",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            topicString,
                                            &subAttrs,
                                            hClient);
    TEST_ASSERT_EQUAL(rc, OK);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    ismEngine_Subscription_t *subscription;
    rc = iett_findClientSubscription(pThreadData,
                                     hClient->pClientId,
                                     iett_generateClientIdHash(hClient->pClientId),
                                     "UpdatePolicySub",
                                     iett_generateSubNameHash("UpdatePolicySub"),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);

    iepiPolicyInfo_t templatePolicyInfo;

    templatePolicyInfo.maxMessageCount = 333;
    templatePolicyInfo.maxMsgBehavior = RejectNewMessages;
    templatePolicyInfo.DCNEnabled = false;

    iepiPolicyInfo_t *fakePolicy1 = NULL;
    rc = iepi_createPolicyInfo(pThreadData, "fakePolicy1", ismSEC_POLICY_QUEUE, true, &templatePolicyInfo, &fakePolicy1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(fakePolicy1);

    rc = iett_setSubscriptionPolicyInfo(pThreadData, subscription, fakePolicy1);
    TEST_ASSERT_EQUAL(rc, OK);

    iepiPolicyInfo_t *fakePolicy2 = NULL;
    rc = iepi_copyPolicyInfo(pThreadData, fakePolicy1, "fakePolicy2", &fakePolicy2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(fakePolicy2);

    rc = iett_setSubscriptionPolicyInfo(pThreadData, subscription, fakePolicy2);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySubscription(hClient,
                                        "UpdatePolicySub",
                                        hClient,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    iepi_releasePolicyInfo(pThreadData, fakePolicy1);

    TEST_ASSERT_EQUAL((subscription->internalAttrs & iettSUBATTR_DELETED), iettSUBATTR_DELETED);

    iett_releaseSubscription(pThreadData, subscription, true);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    test_destroySecurityContext(mockListener, mockTransport, mockContext);
}

CU_TestInfo ISM_TopicTree_CUnit_test_Security_Phase1[] =
{
    { "TopicSecurity", test_capability_TopicSecurity_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_Security_Phase2[] =
{
    { "TopicSecurity", test_capability_TopicSecurity_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_Security_Phase3[] =
{
    { "TopicSecurity", test_capability_TopicSecurity_Phase3 },
    { "UpdatePolicy", test_func_UpdatePolicy },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the code path exposed by defect 20114
//****************************************************************************
#define DEFECT20114_POLICYNAME "Defect20114Policy"

void test_path_Defect20114(void)
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "SecureClient";

    // Create a client and session
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    printf("Starting %s...\n", __func__);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

    rc = sync_ism_engine_createSubscription(hClient,
                                            "Defect20114Sub",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "TEST.DEFECT20114",
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    int32_t               messageCount;
    ismEngine_Consumer_t *consumer;

    // Create consumer
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "Defect20114Sub",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &messageCount,
                                   sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &consumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Attempt to destroy subscription in use
    rc = ism_engine_destroySubscription(hClient,
                                        "Defect20114Sub",
                                        hClient,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationInUse);

    // Destroy consumer
    rc = ism_engine_destroyConsumer(consumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Use the dynamic config interface to delete the subscription
    char dynamicConfigString[1024];

    // Attempt to destroy subscription but ommitting client id specified
    rc = test_configProcessDelete("Subscription", "Defect20114Sub", NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_BadRESTfulRequest); // (ISMRC_ConfigError)

    // Try an invalid operation (changing)
    sprintf(dynamicConfigString, "%s/Defect20114Sub", mockTransport->clientID);
    rc = test_configProcessDelete("Subscription", dynamicConfigString, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Non-existent subscription
    sprintf(dynamicConfigString, "%s/NONEXISTENTSUB", mockTransport->clientID);
    rc = test_configProcessDelete("Subscription", dynamicConfigString, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound); // (ISMRC_ConfigError)

    // Check it no longer exists
    (void)test_checkDurableSubscription(hClient,
                                        "Defect20114Sub",
                                        false,
                                        intermediate, 0, 0, 0,
                                        NULL,
                                        NULL);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_TopicTree_CUnit_test_Defect20114[] =
{
    { "Defect20114", test_path_Defect20114 },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the code path exposed by defect 26781
/// Publishing to full durable subscribers and empty ones at the same time caused
/// the server to come down
//****************************************************************************
void test_path_Defect26781(void)
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    char *topicString = "/test/defect26781";
    uint32_t maxMessagesPerSub = 1;
    ismEngine_MessageHandle_t hMessage;

    // Create a client and session
    rc = test_createClientAndSession("defect26781",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    printf("Starting %s...\n", __func__);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

    //Set max messages for each subscription to be a low number...
    uint32_t origMMC = iepi_getDefaultPolicyInfo(false)->maxMessageCount;
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = maxMessagesPerSub;

    rc = sync_ism_engine_createSubscription(hClient,
                                            "Defect26781SubOne",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            topicString,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    //Publish messages until the first subscription is full...
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    test_incrementActionsRemaining(pActionsRemaining, maxMessagesPerSub);
    for (uint32_t i=0; i < maxMessagesPerSub; i++) /* BEAM suppression: loop doesn't iterate */
    {
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

    //Check that it is full...
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
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationFull);

    //Add another subscription...
    rc = sync_ism_engine_createSubscription(hClient,
                                            "Defect26781SubTwo",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            topicString,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    //Try and publish again...
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
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationFull);

    // Attempt to destroy subscriptions
    rc = ism_engine_destroySubscription(hClient,
                                        "Defect26781SubOne",
                                        hClient,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySubscription(hClient,
                                        "Defect26781SubTwo",
                                        hClient,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Put back original MMC
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = origMMC;
}

CU_TestInfo ISM_TopicTree_CUnit_test_Defect26781MaxMessages[] =
{
    { "Defect26781", test_path_Defect26781 },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test Deletion of durable subscriptions using the API
///        ism_engine_listSubscriptions
//****************************************************************************
typedef struct tag_listDurableCallbackContext_t
{
    ismEngine_ClientStateHandle_t hClient;
    uint32_t *listed;
    uint32_t *destroyed;
    uint32_t *async;
} listDurableCallbackContext_t;

typedef struct tag_destroyDurableCallbackContext_t
{
    listDurableCallbackContext_t *listContext;
    uint32_t subNumber;
} destroyDurableCallbackContext_t;

void destroyDurableCallback(int32_t retcode,
                            void *handle,
                            void *pContext)
{
    destroyDurableCallbackContext_t *durableContext = (destroyDurableCallbackContext_t *)pContext;

    (durableContext->listContext->destroyed[durableContext->subNumber])++;
}

void listDeletionDurableCallback(ismEngine_SubscriptionHandle_t subHandle,
                                 const char *pSubName,
                                 const char *pTopicString,
                                 void *properties,
                                 size_t propertiesLength,
                                 const ismEngine_SubscriptionAttributes_t *pSubAttrs,
                                 uint32_t consumerCount,
                                 void *pContext)
{
    listDurableCallbackContext_t *listContext = (listDurableCallbackContext_t *)pContext;

    int32_t subNumber = strtod(pSubName, NULL);

    (listContext->listed[subNumber])++;

    destroyDurableCallbackContext_t destroyContext = {listContext, subNumber};

    int32_t rc = ism_engine_destroySubscription(listContext->hClient,
                                                pSubName,
                                                listContext->hClient,
                                                &destroyContext,
                                                sizeof(destroyDurableCallbackContext_t),
                                                destroyDurableCallback);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_EQUAL(rc, OK);
        (listContext->destroyed[subNumber])++;
    }
    else
    {
        (listContext->async[subNumber])++;
    }
}

void test_ListDeletion(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t     hSession1;
    listDurableCallbackContext_t  context = {0};

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("ListDeletion_Client1",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    int32_t i;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };
    for(i=0; i<10; i++)
    {
        char subName[5];
        sprintf(subName, "%d", i);

        rc = sync_ism_engine_createSubscription(hClient1,
                                                subName,
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                "Test/ListDeletion",
                                                &subAttrs,
                                                NULL); // Owning client same as requesting client
        TEST_ASSERT_EQUAL(rc, OK);
    }
    context.listed = calloc(i,sizeof(uint32_t));
    TEST_ASSERT_PTR_NOT_NULL(context.listed);
    context.destroyed = calloc(i,sizeof(uint32_t));
    TEST_ASSERT_PTR_NOT_NULL(context.destroyed);
    context.async = calloc(i,sizeof(uint32_t));
    TEST_ASSERT_PTR_NOT_NULL(context.async);

    context.hClient = hClient1;

    // Confirm the result with iett_getSubscriberList
    iettSubscriberList_t list = {0};
    list.topicString = "Test/ListDeletion";

    rc = iett_getSubscriberList(pThreadData, &list);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 10);

    rc = ism_engine_listSubscriptions(hClient1, NULL, &context, listDeletionDurableCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    for(i=0; i<10; i++)
    {
        TEST_ASSERT_EQUAL(context.listed[i], 1);
        TEST_ASSERT_EQUAL(context.async[i], 0);
        TEST_ASSERT_EQUAL(context.destroyed[i], 1);
    }

    iett_releaseSubscriberList(pThreadData, &list);

    for(i=0; i<10; i++)
    {
        TEST_ASSERT_EQUAL(context.listed[i], 1);
        TEST_ASSERT_EQUAL(context.async[i], 0);
        TEST_ASSERT_EQUAL(context.destroyed[i], 1);
    }

    rc = test_destroyClientAndSession(hClient1, hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    free(context.listed);
    free(context.async);
    free(context.destroyed);
}

typedef struct tag_destroyClientStateCallbackContext_t
{
  uint32_t called;
} destroyClientStateCallbackContext_t;

void destroyClientStateCallback(int32_t retcode,
                                void *handle,
                                void *pContext)
{
    destroyClientStateCallbackContext_t *context = *(destroyClientStateCallbackContext_t **)pContext;
    (context->called)++;
}

void test_ImplicitDeletion(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    ismEngine_ClientStateHandle_t hClient1;

    printf("Starting %s...\n", __func__);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_DURABLE };
    for(int32_t i=0; i<2; i++)
    {
        rc = sync_ism_engine_createClientState("ImplicitDeletion_Client1",
                                          testDEFAULT_PROTOCOL_ID,
                                          ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                          NULL, NULL,
                                          NULL,
                                          &hClient1);
        TEST_ASSERT_EQUAL(rc, OK);

        uint32_t currentClientUseCount = ((ismEngine_ClientState_t *)hClient1)->UseCount;

        rc = sync_ism_engine_createSubscription(hClient1,
                                                "ImplicitDeletionSub",
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                "Test/ImplicitDeletion",
                                                &subAttrs,
                                                hClient1);
        TEST_ASSERT_EQUAL(rc, OK);

        //dirty hack to wait for the async remains of createSubscription to let go of the client....
        while (((ismEngine_ClientState_t *)hClient1)->UseCount > currentClientUseCount)
        {
            sched_yield();
        }

        // Confirm the result with iett_getSubscriberList
        iettSubscriberList_t list = {0};
        list.topicString = "Test/ImplicitDeletion";

        rc = iett_getSubscriberList(pThreadData, &list);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(list.subscriberCount, 1);

        // First loop, release the subscriber list now so destroyClientState does not
        // need to go async.
        if (i==0)
        {
            iett_releaseSubscriberList(pThreadData, &list);
        }

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_destroyClientState(hClient1,
                                           ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                           &pActionsRemaining,
                                           sizeof(pActionsRemaining),
                                           test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

        if (i!=0)
        {
            iett_releaseSubscriberList(pThreadData, &list);
        }

        test_waitForRemainingActions(pActionsRemaining);
    }
}

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Deletion[] =
{
    //{ "ListDeletion", test_ListDeletion },
    { "ImplicitDeletion", test_ImplicitDeletion },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Create a v1 persistent local transaction in the store
//****************************************************************************
void *test_storeV1LocalTransaction(ismTransactionState_t tranState)
{
    int32_t rc = OK;
    ismStore_Handle_t storeHandle = ismSTORE_NULL_HANDLE;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    // This assumes that the current transaction record is version 1, if we
    // have a new transaction record, we'll need a new function to write a
    // version 1 one.
    TEST_ASSERT_EQUAL(iestTR_CURRENT_VERSION, iestTR_VERSION_1);

    iestTransactionRecord_t tranRec;
    ismStore_Record_t storeTran;

    char *fragments[1]; // Up to 1 fragment
    uint32_t fragmentLengths[1];

    // Add the transaction to the store
    // We can do this before we take the queue put lock, as it is only
    // the message reference which we must add to the store in the
    // context of the put lock in order to preserve message order.
    fragments[0] = (char *)&tranRec;
    fragmentLengths[0] = sizeof(tranRec);
    ismEngine_SetStructId(tranRec.StrucId, iestTRANSACTION_RECORD_STRUCID);
    tranRec.Version = iestTR_CURRENT_VERSION;
    tranRec.GlobalTran = false;
    tranRec.TransactionIdLength = 0;
    storeTran.Type = ISM_STORE_RECTYPE_TRANS;
    storeTran.FragsCount = 1;
    storeTran.pFrags = fragments;
    storeTran.pFragsLengths = fragmentLengths;
    storeTran.DataLength = sizeof(tranRec);
    storeTran.Attribute = 0; // Unused
    storeTran.State = ((uint64_t)ism_common_nowExpire() << 32) | tranState;

    rc = ism_store_createRecord( pThreadData->hStream
                               , &storeTran
                               , &storeHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(storeHandle, ismSTORE_NULL_HANDLE);

    // Need to commit to be able to open ref context
    ism_store_commit(pThreadData->hStream);

    void *pTranRefContext = NULL;

    rc = ism_store_openReferenceContext(storeHandle, NULL, &pTranRefContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pTranRefContext);

    return pTranRefContext;
}

void test_createV1TranRef(void *pTranRefContext,
                          uint64_t orderId,
                          ismStore_Handle_t hObject,
                          uint32_t Value,
                          uint8_t State)
{
    int32_t rc = OK;
    ismStore_Handle_t hRefHandle = ismSTORE_NULL_HANDLE;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    ismStore_Reference_t tranRef;

    tranRef.OrderId = orderId;
    tranRef.hRefHandle = hObject;
    tranRef.Value = Value;
    tranRef.State = State;
    tranRef.Pad[0] = 0;
    tranRef.Pad[1] = 0;
    tranRef.Pad[2] = 0;

    rc = ism_store_createReference(pThreadData->hStream,
                                   pTranRefContext,
                                   &tranRef,
                                   0, // Minimum active orderid
                                   &hRefHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hRefHandle, ismSTORE_NULL_HANDLE);
}

//****************************************************************************
/// @brief Test Migration of a V1 subscription
//****************************************************************************
void test_storeV1Subscription(const char *topicString,
                              const char *subName,
                              uint32_t subOptions,
                              const char *clientId,
                              ismTransactionState_t tranState)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismStore_Record_t storeRecord;
    iestSubscriptionDefinitionRecord_t SDR;
    iestSubscriptionPropertiesRecord_V1_t SPR;
    ismStore_Handle_t hStoreSubscProps;
    ismStore_Handle_t hStoreSubscDefn;
    void *pTranRefContext = NULL;

    char *fragments[5]; // Up to 5 fragments for subscription properties
    uint32_t fragmentLengths[5];

    // Asked to put this in a transaction with specified state
    if (tranState != ismTRANSACTION_STATE_NONE)
    {
        pTranRefContext = test_storeV1LocalTransaction(tranState);
        TEST_ASSERT_PTR_NOT_NULL(pTranRefContext);
    }

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

    SPR.MaxMessages = 1000;
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
    if (subOptions & (ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                      ismENGINE_SUBSCRIPTION_OPTION_SHARED))
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

    // Create version appropriate transaction references
    if (pTranRefContext != NULL)
    {
        uint64_t tranOrderId = 1;
        test_createV1TranRef(pTranRefContext,
                             tranOrderId++,
                             hStoreSubscProps,
                             3 /* iestTOR_VALUE_STORE_SUBSC_PROPS */,
                             0);

        test_createV1TranRef(pTranRefContext,
                             tranOrderId++,
                             hStoreSubscDefn,
                             2 /* iestTOR_VALUE_STORE_SUBSC_DEFN */,
                             0);

        rc = ism_store_commit(pThreadData->hStream);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_store_closeReferenceContext(pTranRefContext);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    else
    {
        rc = ism_store_commit(pThreadData->hStream);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

//****************************************************************************
/// @brief Test Migration of a V2 subscription
//****************************************************************************
void test_storeV2Subscription(const char *topicString,
                              const char *subName,
                              uint32_t subOptions,
                              const char *clientId,
                              bool DCNEnabled,
                              ismTransactionState_t tranState)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismStore_Record_t storeRecord;
    iestSubscriptionDefinitionRecord_t SDR;
    iestSubscriptionPropertiesRecord_V2_t SPR;
    ismStore_Handle_t hStoreSubscProps;
    ismStore_Handle_t hStoreSubscDefn;
    void *pTranRefContext = NULL;

    char *fragments[5]; // Up to 5 fragments for subscription properties
    uint32_t fragmentLengths[5];

    // Asked to put this in a transaction with specified state
    if (tranState != ismTRANSACTION_STATE_NONE)
    {
        pTranRefContext = test_storeV1LocalTransaction(tranState);
        TEST_ASSERT_PTR_NOT_NULL(pTranRefContext);
    }

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

    SPR.MaxMessages = 2000;
    SPR.DCNEnabled = DCNEnabled;
    memcpy(SPR.StrucId, iestSUBSC_PROPS_RECORD_STRUCID, 4);
    SPR.Version = iestSPR_VERSION_2;
    SPR.SubOptions = subOptions & 0x7F; // VALID MASK
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
    if (subOptions & (ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                      ismENGINE_SUBSCRIPTION_OPTION_SHARED))
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

    // Create version appropriate transaction references
    if (pTranRefContext != NULL)
    {
        uint64_t tranOrderId = 1;
        test_createV1TranRef(pTranRefContext,
                             tranOrderId++,
                             hStoreSubscProps,
                             3 /* iestTOR_VALUE_STORE_SUBSC_PROPS */,
                             0);

        test_createV1TranRef(pTranRefContext,
                             tranOrderId++,
                             hStoreSubscDefn,
                             2 /* iestTOR_VALUE_STORE_SUBSC_DEFN */,
                             0);

        rc = ism_store_commit(pThreadData->hStream);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_store_closeReferenceContext(pTranRefContext);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    else
    {
        rc = ism_store_commit(pThreadData->hStream);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

//****************************************************************************
/// @brief Test Migration of a V3 subscription
//****************************************************************************
void test_storeV3Subscription(const char *topicString,
                              const char *subName,
                              uint32_t subOptions,
                              const char *clientId,
                              bool DCNEnabled,
                              ismTransactionState_t tranState)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismStore_Record_t storeRecord;
    iestSubscriptionDefinitionRecord_t SDR;
    iestSubscriptionPropertiesRecord_V3_t SPR;
    ismStore_Handle_t hStoreSubscProps;
    ismStore_Handle_t hStoreSubscDefn;
    void *pTranRefContext = NULL;

    char *fragments[5]; // Up to 5 fragments for subscription properties
    uint32_t fragmentLengths[5];

    // Asked to put this in a transaction with specified state
    if (tranState != ismTRANSACTION_STATE_NONE)
    {
        pTranRefContext = test_storeV1LocalTransaction(tranState);
        TEST_ASSERT_PTR_NOT_NULL(pTranRefContext);
    }

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

    SPR.MaxMessages = 3000;
    SPR.DCNEnabled = DCNEnabled;
    SPR.MaxMsgsBehavior = DiscardOldMessages;
    memcpy(SPR.StrucId, iestSUBSC_PROPS_RECORD_STRUCID, 4);
    SPR.Version = iestSPR_VERSION_3;
    SPR.SubOptions = subOptions & 0x7F; // PERSISTENT MASK
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
    if (subOptions & (ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                      ismENGINE_SUBSCRIPTION_OPTION_SHARED))
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

    // Create version appropriate transaction references
    if (pTranRefContext != NULL)
    {
        uint64_t tranOrderId = 1;
        test_createV1TranRef(pTranRefContext,
                             tranOrderId++,
                             hStoreSubscProps,
                             3 /* iestTOR_VALUE_STORE_SUBSC_PROPS */,
                             0);

        test_createV1TranRef(pTranRefContext,
                             tranOrderId++,
                             hStoreSubscDefn,
                             2 /* iestTOR_VALUE_STORE_SUBSC_DEFN */,
                             0);

        rc = ism_store_commit(pThreadData->hStream);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_store_closeReferenceContext(pTranRefContext);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    else
    {
        rc = ism_store_commit(pThreadData->hStream);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

//****************************************************************************
/// @brief Test Migration of a V4 subscription
//****************************************************************************
void test_storeV4Subscription(const char *topicString,
                              const char *subName,
                              uint32_t subOptions,
                              const char *clientId,
                              bool DCNEnabled,
                              ismTransactionState_t tranState)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismStore_Record_t storeRecord;
    iestSubscriptionDefinitionRecord_t SDR;
    iestSubscriptionPropertiesRecord_V4_t SPR;
    ismStore_Handle_t hStoreSubscProps;
    ismStore_Handle_t hStoreSubscDefn;
    void *pTranRefContext = NULL;

    char *fragments[6]; // Up to 6 fragments for subscription properties
    uint32_t fragmentLengths[6];

    // Asked to put this in a transaction with specified state
    if (tranState != ismTRANSACTION_STATE_NONE)
    {
        pTranRefContext = test_storeV1LocalTransaction(tranState);
        TEST_ASSERT_PTR_NOT_NULL(pTranRefContext);
    }

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

    SPR.MaxMessages = 4000;
    SPR.DCNEnabled = DCNEnabled;
    SPR.MaxMsgsBehavior = DiscardOldMessages;
    memcpy(SPR.StrucId, iestSUBSC_PROPS_RECORD_STRUCID, 4);
    SPR.Version = iestSPR_VERSION_4;
    SPR.SubOptions = subOptions & 0x7F; // PERSISTENT MASK
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

    SPR.GenericallyShared = false;
    SPR.SharingClientCount = 0;
    SPR.SharingClientIdsLength = 0;

    // Add to the store
    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreSubscProps);
    TEST_ASSERT_EQUAL(rc, OK);

    ismQueueType_t queueType;

    // Transaction capable and shared subscriptions require a multiConsumer queue,
    // otherwise the type of queue to use is determined by the requested reliability.
    if (subOptions & (ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                      ismENGINE_SUBSCRIPTION_OPTION_SHARED))
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

    // Create version appropriate transaction references
    if (pTranRefContext != NULL)
    {
        uint64_t tranOrderId = 1;
        test_createV1TranRef(pTranRefContext,
                             tranOrderId++,
                             hStoreSubscProps,
                             3 /* iestTOR_VALUE_STORE_SUBSC_PROPS */,
                             0);

        test_createV1TranRef(pTranRefContext,
                             tranOrderId++,
                             hStoreSubscDefn,
                             2 /* iestTOR_VALUE_STORE_SUBSC_DEFN */,
                             0);

        rc = ism_store_commit(pThreadData->hStream);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_store_closeReferenceContext(pTranRefContext);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    else
    {
        rc = ism_store_commit(pThreadData->hStream);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

//****************************************************************************
/// @brief Test Migration of a V5 subscription
//****************************************************************************
void test_storeV5Subscription(const char *topicString,
                              const char *subName,
                              uint32_t subOptions,
                              uint32_t internalAttrs,
                              const char *clientId,
                              const char *policyUUID,
                              bool DCNEnabled,
                              ismTransactionState_t tranState)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismStore_Record_t storeRecord;
    iestSubscriptionDefinitionRecord_t SDR;
    iestSubscriptionPropertiesRecord_V5_t SPR;
    ismStore_Handle_t hStoreSubscProps;
    ismStore_Handle_t hStoreSubscDefn;
    void *pTranRefContext = NULL;

    char *fragments[9]; // Up to 9 fragments for subscription properties
    uint32_t fragmentLengths[9];

    // Asked to put this in a transaction with specified state
    if (tranState != ismTRANSACTION_STATE_NONE)
    {
        pTranRefContext = test_storeV1LocalTransaction(tranState);
        TEST_ASSERT_PTR_NOT_NULL(pTranRefContext);
    }

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
    SPR.DCNEnabled = DCNEnabled;
    SPR.MaxMsgsBehavior = DiscardOldMessages;
    memcpy(SPR.StrucId, iestSUBSC_PROPS_RECORD_STRUCID, 4);
    SPR.Version = iestSPR_VERSION_5;
    SPR.SubOptions = subOptions & 0x7F; // PERSISTENT MASK
    SPR.InternalAttrs = internalAttrs & iettSUBATTR_PERSISTENT_MASK;

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

    SPR.PolicyUUIDLength = (uint32_t)strlen(policyUUID)+1;
    fragments[storeRecord.FragsCount] = (char *)policyUUID;
    fragmentLengths[storeRecord.FragsCount] = SPR.PolicyUUIDLength;
    storeRecord.DataLength += fragmentLengths[storeRecord.FragsCount];
    storeRecord.FragsCount += 1;

    SPR.SubPropertiesLength = 0;

    // Set it up as a generically shared subscription (so with no sharing clients)
    SPR.GenericallyShared = (subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) != 0;
    SPR.SharingClientCount = 0;
    SPR.SharingClientIdsLength = 0;

    // Add to the store
    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreSubscProps);
    TEST_ASSERT_EQUAL(rc, OK);

    ismQueueType_t queueType;

    // Transaction capable and shared subscriptions require a multiConsumer queue,
    // otherwise the type of queue to use is determined by the requested reliability.
    if (subOptions & (ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                      ismENGINE_SUBSCRIPTION_OPTION_SHARED))
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

    // Create version appropriate transaction references
    if (pTranRefContext != NULL)
    {
        uint64_t tranOrderId = 1;
        test_createV1TranRef(pTranRefContext,
                             tranOrderId++,
                             hStoreSubscProps,
                             3 /* iestTOR_VALUE_STORE_SUBSC_PROPS */,
                             0);

        test_createV1TranRef(pTranRefContext,
                             tranOrderId++,
                             hStoreSubscDefn,
                             2 /* iestTOR_VALUE_STORE_SUBSC_DEFN */,
                             0);

        rc = ism_store_commit(pThreadData->hStream);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_store_closeReferenceContext(pTranRefContext);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    else
    {
        rc = ism_store_commit(pThreadData->hStream);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

//****************************************************************************
/// @brief Test migration of a V6 subscription
//****************************************************************************
void test_storeV6Subscription(const char *topicString,
                              const char *subName,
                              uint32_t subOptions,
                              uint32_t internalAttrs,
                              const char *clientId,
                              const char *policyName,
                              bool DCNEnabled)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismStore_Record_t storeRecord;
    iestSubscriptionDefinitionRecord_t SDR;
    iestSubscriptionPropertiesRecord_V6_t SPR;
    ismStore_Handle_t hStoreSubscProps;
    ismStore_Handle_t hStoreSubscDefn;

    char *fragments[9]; // Up to 9 fragments for subscription properties
    uint32_t fragmentLengths[9];

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

    SPR.MaxMessageCount = 5000;
    SPR.DCNEnabled = DCNEnabled;
    SPR.MaxMsgBehavior = DiscardOldMessages;
    memcpy(SPR.StrucId, iestSUBSC_PROPS_RECORD_STRUCID, 4);
    SPR.Version = iestSPR_VERSION_6;
    SPR.SubOptions = subOptions & 0x17F; // PERSISTENT MASK
    SPR.InternalAttrs = internalAttrs & iettSUBATTR_PERSISTENT_MASK;

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

    SPR.PolicyNameLength = (uint32_t)strlen(policyName)+1;
    fragments[storeRecord.FragsCount] = (char *)policyName;
    fragmentLengths[storeRecord.FragsCount] = SPR.PolicyNameLength;
    storeRecord.DataLength += fragmentLengths[storeRecord.FragsCount];
    storeRecord.FragsCount += 1;

    SPR.SubPropertiesLength = 0;

    // Set it up as an anonymously shared subscription (so with no sharing clients)
    SPR.AnonymousSharers = ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) != 0) ?
                            iettANONYMOUS_SHARER_JMS_APPLICATION : iettNO_ANONYMOUS_SHARER;
    SPR.SharingClientCount = 0;
    SPR.SharingClientIdsLength = 0;

    // Add to the store
    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreSubscProps);
    TEST_ASSERT_EQUAL(rc, OK);

    ismQueueType_t queueType;

    // Transaction capable and shared subscriptions require a multiConsumer queue,
    // otherwise the type of queue to use is determined by the requested reliability.
    if (subOptions & (ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                      ismENGINE_SUBSCRIPTION_OPTION_SHARED))
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

void test_capability_SubscriptionMigration_Phase1(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("MigrationClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // One old V1 subscriptions
    test_storeV1Subscription("Migration/Topic",
                             "1",
                             ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             "MigrationClient",
                             ismTRANSACTION_STATE_NONE);

    // Two old V2 subscriptions
    test_storeV2Subscription("Migration/Topic",
                             "2",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                             "MigrationClient",
                             true,
                             ismTRANSACTION_STATE_NONE);

    test_storeV2Subscription("Migration/Topic",
                             "3",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             "MigrationClient",
                             false,
                             ismTRANSACTION_STATE_NONE);

    // One old V3 subscription
    test_storeV3Subscription("Migration/Topic",
                             "4",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                             "MigrationClient",
                             true,
                             ismTRANSACTION_STATE_NONE);

    // One old V4 subscription
    test_storeV4Subscription("Migration/Topic",
                             "5",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             "MigrationClient",
                             false,
                             ismTRANSACTION_STATE_NONE);

    // One old V5 subscription on a $SYS topic
    test_storeV5Subscription("$SYS/Topic",
                             "6",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                             iettSUBATTR_PERSISTENT | 0x00000004 /* iettSUBATTR_DOLLARSYS_TOPIC */,
                             "__SysClient",
                             "POLICYUUID_2",
                             false,
                             ismTRANSACTION_STATE_NONE);

    // One old V5 subscription on a $ topic
    test_storeV5Subscription("$SomeOtherTopic/Topic",
                             "7",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             iettSUBATTR_PERSISTENT,
                             "MigrationClient",
                             "POLICYUUID_2",
                             false,
                             ismTRANSACTION_STATE_NONE);

    // Two old subscriptions on another topic (one V5 and one V6)
    test_storeV5Subscription("Migration/OtherTopic",
                             "8",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                             iettSUBATTR_PERSISTENT,
                             "MigrationClient",
                             "POLICYUUID_1",
                             false,
                             ismTRANSACTION_STATE_NONE);

    test_storeV6Subscription("Migration/OtherTopic",
                             "9",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             iettSUBATTR_PERSISTENT,
                             "MigrationClient",
                             "MappedTopicPolicy",
                             false);

    // One old V6 shared subscription
    test_storeV6Subscription("SharedTopic/Topic",
                             "10",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_SHARED,
                             iettSUBATTR_PERSISTENT,
                             ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE,
                             "POLICYNAME",
                             false);

    // One new one
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };

    rc = sync_ism_engine_createSubscription(hClient1,
                                            "NEW",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "Migration/Topic",
                                            &subAttrs,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a mapping file
    const char *configDir = ism_common_getStringConfig("ConfigDir");
    TEST_ASSERT_PTR_NOT_NULL(configDir);
    char MappingFilename[strlen(configDir)+strlen(iepiPOLICY_NAME_MAPPING_FILE)+2];
    sprintf(MappingFilename, "%s/%s", configDir, iepiPOLICY_NAME_MAPPING_FILE);

    FILE *fp = fopen(MappingFilename, "w");
    TEST_ASSERT_PTR_NOT_NULL(fp);
    fprintf(fp, "\t POLICYUUID_1   MappedTopicPolicy\t\n");
    fprintf(fp, "NOT_A_UUID_WE_USE NotATopicPolicy");
    fclose(fp);

    // Confirm that only one is visible (because only one was created properly!)
    iettSubscriberList_t list = {0};
    list.topicString = "Migration/Topic";

    rc = iett_getSubscriberList(pThreadData, &list);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 1);

    iett_releaseSubscriberList(pThreadData, &list);
}

void test_capability_SubscriptionMigration_Phase2(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    // Check the mapping file is still there after the first restart (because
    // it will have been used).
    const char *configDir = ism_common_getStringConfig("ConfigDir");
    TEST_ASSERT_PTR_NOT_NULL(configDir);
    char MappingFilename[strlen(configDir)+strlen(iepiPOLICY_NAME_MAPPING_FILE)+2];
    sprintf(MappingFilename, "%s/%s", configDir, iepiPOLICY_NAME_MAPPING_FILE);

    FILE *fp = fopen(MappingFilename, "r");
    TEST_ASSERT_PTR_NOT_NULL(fp);
    fclose(fp);

    pThreadData = ieut_getThreadData();

    // Check we now have 6 subscriptions because they've all been rebuilt
    iettSubscriberList_t list = {0};
    list.topicString = "Migration/Topic";

    rc = iett_getSubscriberList(pThreadData, &list);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 6);

    bool defCC = iepi_getDefaultPolicyInfo(false)->concurrentConsumers;
    bool defDCNE = iepi_getDefaultPolicyInfo(false)->DCNEnabled;
    iepiMaxMsgBehavior_t defMMB  = iepi_getDefaultPolicyInfo(false)->maxMsgBehavior;

    // Check the properties are defaulting / set as expected
    for(uint32_t i=0; i<list.subscriberCount; i++)
    {
        iepiPolicyInfo_t *policyInfo = list.subscribers[i]->queueHandle->PolicyInfo;

        // All will be using an anonymous policy because validation is disabled
        TEST_ASSERT_PTR_NULL(policyInfo->name);

        if (strcmp(list.subscribers[i]->subName, "1") == 0)
        {
            TEST_ASSERT_EQUAL(policyInfo->concurrentConsumers, defCC);
            TEST_ASSERT_EQUAL(policyInfo->maxMsgBehavior, defMMB);
            TEST_ASSERT_EQUAL(policyInfo->maxMessageCount, 1000);
            TEST_ASSERT_EQUAL(policyInfo->DCNEnabled, defDCNE);
        }
        else if (strcmp(list.subscribers[i]->subName, "2") == 0)
        {
            TEST_ASSERT_EQUAL(policyInfo->concurrentConsumers, defCC);
            TEST_ASSERT_EQUAL(policyInfo->maxMsgBehavior, defMMB);
            TEST_ASSERT_EQUAL(policyInfo->maxMessageCount, 2000);
            TEST_ASSERT_EQUAL(policyInfo->DCNEnabled, true);
        }
        else if (strcmp(list.subscribers[i]->subName, "3") == 0)
        {
            TEST_ASSERT_EQUAL(policyInfo->concurrentConsumers, defCC);
            TEST_ASSERT_EQUAL(policyInfo->maxMsgBehavior, defMMB);
            TEST_ASSERT_EQUAL(policyInfo->maxMessageCount, 2000);
            TEST_ASSERT_EQUAL(policyInfo->DCNEnabled, false);
        }
        else if (strcmp(list.subscribers[i]->subName, "4") == 0)
        {
            TEST_ASSERT_EQUAL(policyInfo->concurrentConsumers, defCC);
            TEST_ASSERT_EQUAL(policyInfo->maxMsgBehavior, DiscardOldMessages);
            TEST_ASSERT_EQUAL(policyInfo->maxMessageCount, 3000);
            TEST_ASSERT_EQUAL(policyInfo->DCNEnabled, true);
        }
    }

    iett_releaseSubscriberList(pThreadData, &list);

    ismEngine_Subscription_t *subscription = NULL;

    // Check the old globally shared subscription is there and looks right
    rc = iett_findClientSubscription(pThreadData,
                                     ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE,
                                     iett_generateClientIdHash(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE),
                                     "10",
                                     iett_generateSubNameHash("10"),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subscription);
    TEST_ASSERT_EQUAL(subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED, ismENGINE_SUBSCRIPTION_OPTION_SHARED);
    TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_GLOBALLY_SHARED, iettSUBATTR_GLOBALLY_SHARED);
    iett_releaseSubscription(pThreadData, subscription, false);

    // Destroy four of the subscriptions, three earlier version, and one this version
    rc = iett_destroySubscriptionForClientId(pThreadData, "MigrationClient", NULL, "1", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = iett_destroySubscriptionForClientId(pThreadData, "MigrationClient", NULL, "3", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = iett_destroySubscriptionForClientId(pThreadData, "MigrationClient", NULL, "5", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = iett_destroySubscriptionForClientId(pThreadData, "MigrationClient", NULL, "NEW", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Look for the subscribers on Migration/OtherTopic
    memset(&list, 0, sizeof(list));
    list.topicString = "Migration/OtherTopic";
    rc = iett_getSubscriberList(pThreadData, &list);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 2);

    for(uint32_t i=0; i<list.subscriberCount; i++)
    {
        iepiPolicyInfo_t *policyInfo = list.subscribers[i]->queueHandle->PolicyInfo;

        if (strcmp(list.subscribers[i]->subName, "8") == 0)
        {
            // Not anonymous - we've mapped it to a name.
            TEST_ASSERT_EQUAL(strcmp(policyInfo->name, "MappedTopicPolicy"), 0);
        }
        else
        {
            TEST_ASSERT_EQUAL(strcmp(list.subscribers[i]->subName, "9"), 0);
            TEST_ASSERT_EQUAL(strcmp(policyInfo->name, "MappedTopicPolicy"), 0);
        }
    }

    iett_releaseSubscriberList(pThreadData, &list);
}

void test_capability_SubscriptionMigration_Phase3(void)
{
    printf("Starting %s...\n", __func__);

    // Check the mapping file is now not there (because it wasn't used).
    const char *configDir = ism_common_getStringConfig("ConfigDir");
    TEST_ASSERT_PTR_NOT_NULL(configDir);
    char MappingFilename[strlen(configDir)+strlen(iepiPOLICY_NAME_MAPPING_FILE)+2];
    sprintf(MappingFilename, "%s/%s", configDir, iepiPOLICY_NAME_MAPPING_FILE);

    FILE *fp = fopen(MappingFilename, "r");
    TEST_ASSERT_PTR_NULL(fp);

    // Write a new mapping file for the other policy
    fp = fopen(MappingFilename, "w");
    TEST_ASSERT_PTR_NOT_NULL(fp);
    fprintf(fp, "BADENTRYWITHNOSPACES\n");
    fprintf(fp, "BADWITHEMPTYPOLICYNAME \n");
    fprintf(fp, "\n");
    fprintf(fp, "SourceString SourceString\n");
    fprintf(fp, "Mary had a little lamb it was not a UUID to policy name mapping\n");
    #if defined(ALLOW_POLICY_NAME_TO_NAME_MAPPING)
    fprintf(fp, "MappedTopicPolicy MappedTopicPolicy2\n");
    fprintf(fp, "POLICYUUID_3 MappedTopicPolicy2\n");
    #else
    fprintf(fp, "POLICYUUID_3 MappedTopicPolicy\n");
    #endif
    fclose(fp);
}

void test_capability_SubscriptionMigration_Phase4(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    // Check the mapping file is now not there (because it wasn't used).
    const char *configDir = ism_common_getStringConfig("ConfigDir");
    TEST_ASSERT_PTR_NOT_NULL(configDir);
    char MappingFilename[strlen(configDir)+strlen(iepiPOLICY_NAME_MAPPING_FILE)+2];
    sprintf(MappingFilename, "%s/%s", configDir, iepiPOLICY_NAME_MAPPING_FILE);

    pThreadData = ieut_getThreadData();

    // Check previously mapped subscription still has expected policy name and newly mapped one
    iettSubscriberList_t list = {0};
    list.topicString = "Migration/OtherTopic";
    rc = iett_getSubscriberList(pThreadData, &list);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 2);
    #if defined(ALLOW_POLICY_NAME_TO_NAME_MAPPING)
    TEST_ASSERT_EQUAL(strcmp(list.subscribers[0]->queueHandle->PolicyInfo->name, "MappedTopicPolicy2"), 0);
    TEST_ASSERT_EQUAL(strcmp(list.subscribers[1]->queueHandle->PolicyInfo->name, "MappedTopicPolicy2"), 0);
    #else
    TEST_ASSERT_EQUAL(strcmp(list.subscribers[0]->queueHandle->PolicyInfo->name, "MappedTopicPolicy"), 0);
    TEST_ASSERT_EQUAL(strcmp(list.subscribers[1]->queueHandle->PolicyInfo->name, "MappedTopicPolicy"), 0);
    #endif

    iett_releaseSubscriberList(pThreadData, &list);

    // Check we now have 2 subscriptions on Migration/Topic
    memset(&list, 0, sizeof(list));
    list.topicString = "Migration/Topic";

    rc = iett_getSubscriberList(pThreadData, &list);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 2);

    iett_releaseSubscriberList(pThreadData, &list);

    ismEngine_Subscription_t *subscription = NULL;

    // One which was on $SYS topic
    rc = iett_findClientSubscription(pThreadData,
                                     "__SysClient",
                                     iett_generateClientIdHash("__SysClient"),
                                     "6",
                                     iett_generateSubNameHash("6"),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subscription);
    TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_SYSTEM_TOPIC, iettSUBATTR_SYSTEM_TOPIC);
    iett_releaseSubscription(pThreadData, subscription, false);

    // One which was previously on $ topic
    rc = iett_findClientSubscription(pThreadData,
                                     "MigrationClient",
                                     iett_generateClientIdHash("MigrationClient"),
                                     "7",
                                     iett_generateSubNameHash("7"),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subscription);
    TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_SYSTEM_TOPIC, iettSUBATTR_SYSTEM_TOPIC);
    iett_releaseSubscription(pThreadData, subscription, false);

    // A globally shared subscription that didn't have internalAttrs set
    rc = iett_findClientSubscription(pThreadData,
                                     ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE,
                                     iett_generateClientIdHash(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE),
                                     "10",
                                     iett_generateSubNameHash("10"),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subscription);
    TEST_ASSERT_EQUAL(subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED, ismENGINE_SUBSCRIPTION_OPTION_SHARED);
    TEST_ASSERT_EQUAL(subscription->internalAttrs & iettSUBATTR_GLOBALLY_SHARED, iettSUBATTR_GLOBALLY_SHARED);
    iett_releaseSubscription(pThreadData, subscription, false);

    // Get rid of the stragglers
    rc = iett_destroySubscriptionForClientId(pThreadData, "MigrationClient", NULL, "2", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = iett_destroySubscriptionForClientId(pThreadData, "MigrationClient", NULL, "4", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = iett_destroySubscriptionForClientId(pThreadData, "__SysClient", NULL, "6", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = iett_destroySubscriptionForClientId(pThreadData, "MigrationClient", NULL, "7", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = iett_destroySubscriptionForClientId(pThreadData, "MigrationClient", NULL, "8", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = iett_destroySubscriptionForClientId(pThreadData, "MigrationClient", NULL, "9", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = iett_destroySubscriptionForClientId(pThreadData, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE, NULL, "10", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

#if 0
void test_capability_VNextToleration_Phase1(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("VNextClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add a VNext subscription
    test_storeVNextSubscription("VNext/Topic",
                                "VNextSub",
                                ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                                iettSUBATTR_PERSISTENT,
                                "VNextClient",
                                "POLICYNAME_1",
                                false);
}

void test_capability_VNextToleration_Phase2(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    // Confirm that the subscription exists
    iettSubscriberList_t list = {0};
    list.topicString = "VNext/Topic";

    rc = iett_getSubscriberList(pThreadData, &list);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 1);

    iett_releaseSubscriberList(pThreadData, &list);

    rc = iett_destroySubscriptionForClientId(pThreadData, "VNextClient", NULL, "VNextSub", NULL, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}
#endif

void test_capability_TransactionalSubscriptionMigration_Phase1(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("TxnlMigClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Two V1 Subscriptions (one to be rolled back, one to be committed)
    test_storeV1Subscription("Transactional/Migration/Topic",
                             "1",
                             ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             "TxnlMigClient",
                             ismTRANSACTION_STATE_IN_FLIGHT);

    test_storeV1Subscription("Transactional/Migration/Topic",
                             "2",
                             ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             "TxnlMigClient",
                             ismTRANSACTION_STATE_COMMIT_ONLY);

    // Two V2 Subscriptions (one to be rolled back, one to be committed)
    test_storeV2Subscription("Transactional/Migration/Topic",
                             "3",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                             "TxnlMigClient",
                             true,
                             ismTRANSACTION_STATE_IN_FLIGHT);

    test_storeV2Subscription("Transactional/Migration/Topic",
                             "4",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             "TxnlMigClient",
                             false,
                             ismTRANSACTION_STATE_COMMIT_ONLY);

    // Two V2 Subscriptions (one to be rolled back, one to be committed)
    test_storeV3Subscription("Transactional/Migration/Topic",
                             "5",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                             "TxnlMigClient",
                             true,
                             ismTRANSACTION_STATE_IN_FLIGHT);

    test_storeV3Subscription("Transactional/Migration/Topic",
                             "6",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                             "TxnlMigClient",
                             true,
                             ismTRANSACTION_STATE_COMMIT_ONLY);

    // Two V4 Subscriptions (one to be rolled back, one to be committed)
    test_storeV4Subscription("Transactional/Migration/Topic",
                             "7",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             "TxnlMigClient",
                             false,
                             ismTRANSACTION_STATE_IN_FLIGHT);

    test_storeV4Subscription("Transactional/Migration/Topic",
                             "8",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE,
                             "TxnlMigClient",
                             false,
                             ismTRANSACTION_STATE_COMMIT_ONLY);

    // Two V5 Subscriptions (one to be rolled back, one to be committed)
    test_storeV5Subscription("Transactional/Migration/Topic",
                             "9",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                             iettSUBATTR_PERSISTENT,
                             "TxnlMigClient",
                             "POLICYUUID_1",
                             false,
                             ismTRANSACTION_STATE_IN_FLIGHT);

    test_storeV5Subscription("Transactional/Migration/Topic",
                             "10",
                             ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             iettSUBATTR_PERSISTENT,
                             "TxnlMigClient",
                             "POLICYUUID_1",
                             false,
                             ismTRANSACTION_STATE_COMMIT_ONLY);

    // Create a mapping file
    const char *configDir = ism_common_getStringConfig("ConfigDir");
    TEST_ASSERT_PTR_NOT_NULL(configDir);
    char MappingFilename[strlen(configDir)+strlen(iepiPOLICY_NAME_MAPPING_FILE)+2];
    sprintf(MappingFilename, "%s/%s", configDir, iepiPOLICY_NAME_MAPPING_FILE);

    FILE *fp = fopen(MappingFilename, "w");
    TEST_ASSERT_PTR_NOT_NULL(fp);
    fprintf(fp, "POLICYUUID_1 MappedTopicPolicy");
    fclose(fp);

    // Confirm that none are visible
    iettSubscriberList_t list = {0};
    list.topicString = "Transactional/Migration/Topic";

    rc = iett_getSubscriberList(pThreadData, &list);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
}

void test_capability_TransactionalSubscriptionMigration_PhaseN(int32_t phase)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    printf("Starting %s (N=%d)...\n", __func__, phase);

    // Check the mapping file is there after the first restart and not
    // after the 2nd.
    const char *configDir = ism_common_getStringConfig("ConfigDir");
    TEST_ASSERT_PTR_NOT_NULL(configDir);
    char MappingFilename[strlen(configDir)+strlen(iepiPOLICY_NAME_MAPPING_FILE)+2];
    sprintf(MappingFilename, "%s/%s", configDir, iepiPOLICY_NAME_MAPPING_FILE);

    FILE *fp = fopen(MappingFilename, "r");
    if (phase == 2)
    {
        TEST_ASSERT_PTR_NOT_NULL(fp);
        fclose(fp);
    }
    else
    {
        TEST_ASSERT_PTR_NULL(fp);
    }

    // Check we now have the subscriptions that should have been committed
    iettSubscriberList_t list = {0};
    list.topicString = "Transactional/Migration/Topic";

    rc = iett_getSubscriberList(pThreadData, &list);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 5);

    for(int32_t sub=0; sub<list.subscriberCount; sub++)
    {
        iepiPolicyInfo_t *policyInfo = list.subscribers[sub]->queueHandle->PolicyInfo;

        // Check that the UUID mapping worked in the transactional case
        if (strcmp(list.subscribers[sub]->subName, "10") == 0)
        {
            TEST_ASSERT_EQUAL(strcmp(policyInfo->name, "MappedTopicPolicy"), 0);
        }
        else
        {
            TEST_ASSERT_PTR_NULL(policyInfo->name);
        }
    }

    iett_releaseSubscriberList(pThreadData, &list);

    // Get rid of the remaining subscriptions
    if (phase == 3)
    {
        rc = iett_destroySubscriptionForClientId(pThreadData, "TxnlMigClient", NULL, "2", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        rc = iett_destroySubscriptionForClientId(pThreadData, "TxnlMigClient", NULL, "4", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        rc = iett_destroySubscriptionForClientId(pThreadData, "TxnlMigClient", NULL, "6", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        rc = iett_destroySubscriptionForClientId(pThreadData, "TxnlMigClient", NULL, "8", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        rc = iett_destroySubscriptionForClientId(pThreadData, "TxnlMigClient", NULL, "10", NULL, iettSUB_DESTROY_OPTION_NONE, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

void test_capability_TransactionalSubscriptionMigration_Phase2(void)
{
    test_capability_TransactionalSubscriptionMigration_PhaseN(2);
}

void test_capability_TransactionalSubscriptionMigration_Phase3(void)
{
    test_capability_TransactionalSubscriptionMigration_PhaseN(3);
}

void test_capability_MassMigration_Phase1(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("MigrationClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t subCount = 0;

    // First time create the subscriptions to hit the number of reserved records precisely
    for(;subCount<iettUPDATE_MIGRATION_RESERVATION_RECORDS;subCount++)
    {
        char subName[strlen("MMSub")+10];

        sprintf(subName, "MMSub.%u", subCount);

        test_storeV5Subscription("Mass/Migration/Topic",
                                 subName,
                                 ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                                 iettSUBATTR_PERSISTENT,
                                 "MigrationClient",
                                 "UUID_NOT_NAME",
                                 false,
                                 ismTRANSACTION_STATE_NONE);
    }

    // Create a mapping file
    const char *configDir = ism_common_getStringConfig("ConfigDir");
    TEST_ASSERT_PTR_NOT_NULL(configDir);
    char MappingFilename[strlen(configDir)+strlen(iepiPOLICY_NAME_MAPPING_FILE)+2];
    sprintf(MappingFilename, "%s/%s", configDir, iepiPOLICY_NAME_MAPPING_FILE);

    FILE *fp = fopen(MappingFilename, "w");
    TEST_ASSERT_PTR_NOT_NULL(fp);
    fprintf(fp, "OTHER_UUID    OtherTopicPolicy\n");
    fprintf(fp, "UUID_NOT_NAME MassTopicPolicy\n");
    fclose(fp);
}


void test_capability_MassMigration_Phase2(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    // Check the mapping file is still there
    const char *configDir = ism_common_getStringConfig("ConfigDir");
    TEST_ASSERT_PTR_NOT_NULL(configDir);
    char MappingFilename[strlen(configDir)+strlen(iepiPOLICY_NAME_MAPPING_FILE)+2];
    sprintf(MappingFilename, "%s/%s", configDir, iepiPOLICY_NAME_MAPPING_FILE);

    FILE *fp = fopen(MappingFilename, "r");
    TEST_ASSERT_PTR_NOT_NULL(fp);
    fclose(fp);

    // Check we now have subCount subscriptions because they've all been rebuilt
    iettSubscriberList_t list = {0};
    list.topicString = "Mass/Migration/Topic";

    uint32_t subCount = 2000; // Expected from previous phase

    rc = iett_getSubscriberList(pThreadData, &list);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, subCount);

    for(uint32_t i=0; i<list.subscriberCount; i++)
    {
        iepiPolicyInfo_t *policyInfo = list.subscribers[i]->queueHandle->PolicyInfo;
        TEST_ASSERT_EQUAL(strcmp(policyInfo->name, "MassTopicPolicy"), 0);
    }

    iett_releaseSubscriberList(pThreadData, &list);

    // Add some more subscriptions!!
    for(;subCount<12345;subCount++)
    {
        char subName[strlen("MMSub")+10];

        sprintf(subName, "MMSub.%u", subCount);

        test_storeV5Subscription("Mass/Migration/Topic",
                                 subName,
                                 ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                                 iettSUBATTR_PERSISTENT,
                                 "MigrationClient",
                                 "UUID_NOT_NAME",
                                 false,
                                 ismTRANSACTION_STATE_NONE);
    }

    TEST_ASSERT_GREATER_THAN(subCount, iettUPDATE_MIGRATION_RESERVATION_RECORDS);
}

void test_capability_MassMigration_Phase3(void)
{
    printf("Starting %s...\n", __func__);

    // Check the mapping file is still there
    const char *configDir = ism_common_getStringConfig("ConfigDir");
    TEST_ASSERT_PTR_NOT_NULL(configDir);
    char MappingFilename[strlen(configDir)+strlen(iepiPOLICY_NAME_MAPPING_FILE)+2];
    sprintf(MappingFilename, "%s/%s", configDir, iepiPOLICY_NAME_MAPPING_FILE);

    FILE *fp = fopen(MappingFilename, "r");
    TEST_ASSERT_PTR_NOT_NULL(fp);
    fclose(fp);
}

void test_capability_MassMigration_Phase4(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    // Check the mapping file is now not there (because it wasn't used).
    const char *configDir = ism_common_getStringConfig("ConfigDir");
    TEST_ASSERT_PTR_NOT_NULL(configDir);
    char MappingFilename[strlen(configDir)+strlen(iepiPOLICY_NAME_MAPPING_FILE)+2];
    sprintf(MappingFilename, "%s/%s", configDir, iepiPOLICY_NAME_MAPPING_FILE);

    FILE *fp = fopen(MappingFilename, "r");
    TEST_ASSERT_PTR_NULL(fp);

    // Check all subscriptions still have expected policy name
    iettSubscriberList_t list = {0};
    list.topicString = "Mass/Migration/Topic";

    uint32_t subCount = 12345; // Expected from previous phase

    rc = iett_getSubscriberList(pThreadData, &list);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, subCount);

    for(uint32_t i=0; i<list.subscriberCount; i++)
    {
        iepiPolicyInfo_t *policyInfo = list.subscribers[i]->queueHandle->PolicyInfo;
        TEST_ASSERT_EQUAL(strcmp(policyInfo->name, "MassTopicPolicy"), 0);
    }

    iett_releaseSubscriberList(pThreadData, &list);

    for(uint32_t destroyCount = 0; destroyCount < subCount; destroyCount++)
    {
        char subName[strlen("MMSub")+10];

        sprintf(subName, "MMSub.%u", destroyCount);

        rc = iett_destroySubscriptionForClientId(pThreadData,
                                                 "MigrationClient",
                                                 NULL, subName, NULL,
                                                 iettSUB_DESTROY_OPTION_NONE,
                                                 NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

void test_capability_MassMigration_Phase5(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    pThreadData = ieut_getThreadData();

    // Check all subscriptions still has expected policy name
    iettSubscriberList_t list = {0};
    list.topicString = "Mass/Migration/Topic";

    rc = iett_getSubscriberList(pThreadData, &list);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
}

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Migration_Phase1[] =
{
    { "Migration", test_capability_SubscriptionMigration_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Migration_Phase2[] =
{
    { "Migration", test_capability_SubscriptionMigration_Phase2 },
    //{ "VNextToleration", test_capability_VNextToleration_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Migration_Phase3[] =
{
    { "Migration", test_capability_SubscriptionMigration_Phase3 },
    //{ "VNextToleration", test_capability_VNextToleration_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Migration_Phase4[] =
{
    { "Migration", test_capability_SubscriptionMigration_Phase4 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Migration_Phase5[] =
{
   { "TransactionalMigration", test_capability_TransactionalSubscriptionMigration_Phase1 },
   CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Migration_Phase6[] =
{
   { "TransactionalMigration", test_capability_TransactionalSubscriptionMigration_Phase2 },
   CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Migration_Phase7[] =
{
   { "TransactionalMigration", test_capability_TransactionalSubscriptionMigration_Phase3 },
   CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Migration_Phase8[] =
{
   { "MassMigration", test_capability_MassMigration_Phase1 },
   CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Migration_Phase9[] =
{
   { "MassMigration", test_capability_MassMigration_Phase2 },
   CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Migration_Phase10[] =
{
   { "MassMigration", test_capability_MassMigration_Phase3 },
   CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Migration_Phase11[] =
{
   { "MassMigration", test_capability_MassMigration_Phase4 },
   CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Migration_Phase12[] =
{
   { "MassMigration", test_capability_MassMigration_Phase5 },
   CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initTopicTreeDurable(void)
{
    return test_engineInit(true, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int initTopicTreeDurableWarm(void)
{
    return test_engineInit(false, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int initTopicTreeSecurity(void)
{
    return test_engineInit(true, false,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int initTopicTreeSecurityWarm(void)
{
    return test_engineInit(false, false,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termTopicTreeDurable(void)
{
    return test_engineTerm(true);
}

int termTopicTreeDurableWarm(void)
{
    return test_engineTerm(false);
}

int termTopicTreeDurableNoOp(void)
{
    return 0;
}

CU_SuiteInfo ISM_TopicTree_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("DurableSubscriptions", initTopicTreeDurable, termTopicTreeDurable, ISM_TopicTree_CUnit_test_capability_DurableSubscriptions),
    IMA_TEST_SUITE("Defect20114", initTopicTreeDurable, termTopicTreeDurable, ISM_TopicTree_CUnit_test_Defect20114),
    IMA_TEST_SUITE("Defect26781MaxMessages", initTopicTreeDurable, termTopicTreeDurable, ISM_TopicTree_CUnit_test_Defect26781MaxMessages),
    IMA_TEST_SUITE("Deletion", initTopicTreeDurable, termTopicTreeDurable, ISM_TopicTree_CUnit_test_capability_Deletion),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase2suites[] =
{
    IMA_TEST_SUITE("Rehydration", initTopicTreeDurable, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_capability_Rehydration_Phase1),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase3suites[] =
{
    IMA_TEST_SUITE("Rehydration", initTopicTreeDurableWarm, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_capability_Rehydration_Phase2),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase4suites[] =
{
    IMA_TEST_SUITE("Rehydration", initTopicTreeDurableWarm, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_capability_Rehydration_Phase3),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase5suites[] =
{
    IMA_TEST_SUITE("Rehydration", initTopicTreeDurableWarm, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_capability_Rehydration_Phase4),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase6suites[] =
{
    IMA_TEST_SUITE("Rehydration", initTopicTreeDurableWarm, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_capability_Rehydration_Phase5),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase7suites[] =
{
    IMA_TEST_SUITE("Rehydration", initTopicTreeDurableWarm, termTopicTreeDurable, ISM_TopicTree_CUnit_test_capability_Rehydration_PhaseFinal),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase8suites[] =
{
    IMA_TEST_SUITE("Migration", initTopicTreeDurable, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_capability_Migration_Phase1),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase9suites[] =
{
    IMA_TEST_SUITE("Migration", initTopicTreeDurableWarm, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_capability_Migration_Phase2),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase10suites[] =
{
    IMA_TEST_SUITE("Migration", initTopicTreeDurableWarm, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_capability_Migration_Phase3),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase11suites[] =
{
    IMA_TEST_SUITE("Migration", initTopicTreeDurableWarm, termTopicTreeDurable, ISM_TopicTree_CUnit_test_capability_Migration_Phase4),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase12suites[] =
{
    IMA_TEST_SUITE("Migration", initTopicTreeDurable, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_capability_Migration_Phase5),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase13suites[] =
{
    IMA_TEST_SUITE("Migration", initTopicTreeDurableWarm, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_capability_Migration_Phase6),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase14suites[] =
{
    IMA_TEST_SUITE("Migration", initTopicTreeDurableWarm, termTopicTreeDurable, ISM_TopicTree_CUnit_test_capability_Migration_Phase7),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase15suites[] =
{
    IMA_TEST_SUITE("Migration", initTopicTreeDurable, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_capability_Migration_Phase8),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase16suites[] =
{
    IMA_TEST_SUITE("Migration", initTopicTreeDurableWarm, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_capability_Migration_Phase9),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase17suites[] =
{
    IMA_TEST_SUITE("Migration", initTopicTreeDurableWarm, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_capability_Migration_Phase10),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase18suites[] =
{
    IMA_TEST_SUITE("Migration", initTopicTreeDurableWarm, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_capability_Migration_Phase11),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase19suites[] =
{
    IMA_TEST_SUITE("Migration", initTopicTreeDurableWarm, termTopicTreeDurable, ISM_TopicTree_CUnit_test_capability_Migration_Phase12),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase20suites[] =
{
    IMA_TEST_SUITE("Security", initTopicTreeSecurity, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_Security_Phase1),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase21suites[] =
{
    IMA_TEST_SUITE("Security", initTopicTreeSecurityWarm, termTopicTreeDurableWarm, ISM_TopicTree_CUnit_test_Security_Phase2),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicTree_CUnit_phase22suites[] =
{
    IMA_TEST_SUITE("Security", initTopicTreeSecurityWarm, termTopicTreeDurable, ISM_TopicTree_CUnit_test_Security_Phase3),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_TopicTree_CUnit_phase1suites
                              , ISM_TopicTree_CUnit_phase2suites
                              , ISM_TopicTree_CUnit_phase3suites
                              , ISM_TopicTree_CUnit_phase4suites
                              , ISM_TopicTree_CUnit_phase5suites
                              , ISM_TopicTree_CUnit_phase6suites
                              , ISM_TopicTree_CUnit_phase7suites
                              , ISM_TopicTree_CUnit_phase8suites
                              , ISM_TopicTree_CUnit_phase9suites
                              , ISM_TopicTree_CUnit_phase10suites
                              , ISM_TopicTree_CUnit_phase11suites
                              , ISM_TopicTree_CUnit_phase12suites
                              , ISM_TopicTree_CUnit_phase13suites
                              , ISM_TopicTree_CUnit_phase14suites
                              , ISM_TopicTree_CUnit_phase15suites
                              , ISM_TopicTree_CUnit_phase16suites
                              , ISM_TopicTree_CUnit_phase17suites
                              , ISM_TopicTree_CUnit_phase18suites
                              , ISM_TopicTree_CUnit_phase19suites
                              , ISM_TopicTree_CUnit_phase20suites
                              , ISM_TopicTree_CUnit_phase21suites
                              , ISM_TopicTree_CUnit_phase22suites };

int main(int argc, char *argv[])
{
    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
