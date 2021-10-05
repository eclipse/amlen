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
/* Module Name: test_retainedMsgs.c                                  */
/*                                                                   */
/* Description: Main source file for CUnit test of Retained messages */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "topicTree.h"
#include "topicTreeInternal.h"
#include "policyInfo.h"
#include "engineMonitoring.h"
#include "engineSplitList.h"
#include "messageExpiry.h"
#include "engineStore.h"
#include "engineTimers.h"
#include "msgCommon.h"
#include "engineUtils.h"

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_assert.h"
#include "test_utils_security.h"
#include "test_utils_file.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_task.h"

bool fullTest = false;
uint32_t testNumber = 0;

// Swallow FDCs if we're forcing failures
uint32_t swallowed_ffdc_count = 0;
bool swallow_ffdcs = false;
bool swallow_expect_core = false;
void ieut_ffdc( const char *function
              , uint32_t seqId
              , bool core
              , const char *filename
              , uint32_t lineNo
              , char *label
              , uint32_t retcode
              , ... )
{
    static void (*real_ieut_ffdc)(const char *, uint32_t, bool, const char *, uint32_t, char *, uint32_t, ...) = NULL;

    if (real_ieut_ffdc == NULL)
    {
        real_ieut_ffdc = dlsym(RTLD_NEXT, "ieut_ffdc");
    }

    if (swallow_ffdcs == true)
    {
        TEST_ASSERT_EQUAL(swallow_expect_core, core);
        __sync_add_and_fetch(&swallowed_ffdc_count, 1);
        return;
    }

    TEST_ASSERT(0, ("Unexpected FFDC from %s:%u", filename, lineNo));
}

//****************************************************************************
/// @brief Don't use normal expiry scan, we want control of expiry
//****************************************************************************
bool doRealExpiry = false;
ieutSplitListCallbackAction_t iett_reapTopicExpiredMessagesCB(ieutThreadData_t *pThreadData,
                                                              void *object,
                                                              void *context)
{
    static ieutSplitListCallbackAction_t (* real_iett_reapTopicExpiredMessagesCB)(ieutThreadData_t *, void *, void *) = NULL;

    if (real_iett_reapTopicExpiredMessagesCB == NULL)
    {
        real_iett_reapTopicExpiredMessagesCB = dlsym(RTLD_NEXT, "iett_reapTopicExpiredMessagesCB");
        if (real_iett_reapTopicExpiredMessagesCB == NULL) abort();
    }

    if (doRealExpiry == true)
    {
        return real_iett_reapTopicExpiredMessagesCB(pThreadData, object, context);
    }
    else
    {
        return ieutSPLIT_LIST_CALLBACK_STOP;
    }
}

//****************************************************************************
/// @brief Publish a message that removes the existing retained msg (at odd
/// times)
//****************************************************************************
void *cuckooThread(void *args)
{
    ismEngine_Message_t *pMessage = (ismEngine_Message_t *)args;

    ism_engine_threadInit(0);

    ismEngine_ClientStateHandle_t hClient = NULL;
    ismEngine_SessionHandle_t hSession = NULL;

    int32_t rc = test_createClientAndSession("CUCKOO",
                                             NULL,
                                             ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                             ismENGINE_CREATE_SESSION_OPTION_NONE,
                                             &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Get the topic string out of the message
    concat_alloc_t  props;
    ism_field_t field = {0};

    iem_locateMessageProperties(pMessage, &props);

    ism_common_findPropertyID(&props, ID_Topic, &field);

    assert(field.type == VT_String);
    assert(field.val.s != NULL);

    const char * topicString = field.val.s;

    void *payload=NULL;
    ismEngine_MessageHandle_t hMsg;

    rc = test_createMessage(10,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, topicString,
                            &hMsg, &payload);
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish message
    uint32_t actionsRemaining = 1;
    uint32_t *pActionsRemaining = &actionsRemaining;
    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            topicString,
                                            NULL,
                                            hMsg,
                                            &pActionsRemaining,
                                            sizeof(pActionsRemaining),
                                            test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    test_waitForRemainingActions(pActionsRemaining);

    test_destroyClientAndSession(hClient, hSession, true);

    ism_engine_threadTerm(0);

    return NULL;
}

void publishCuckoo(ismEngine_Message_t *pMessage)
{
    pthread_t creatingThreadId;
    int os_rc = test_task_startThread(&creatingThreadId,cuckooThread, pMessage,"cuckooThread");
    TEST_ASSERT_EQUAL(os_rc, 0);

    os_rc = pthread_join(creatingThreadId, NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);
}

//****************************************************************************
/// @brief Override iett_updateRetainedMessage
//****************************************************************************
int32_t (* post_iett_updateRetainedMessage_callback)(int32_t, void *) = NULL;
void *post_iett_updateRetainedMessage_context = NULL;
ismEngine_Session_t *putNewRetainedUsingSession = NULL;
ismEngine_Message_t *putNewRetainedAfterUpdateOfMsg = NULL;

int32_t iett_updateRetainedMessage(ieutThreadData_t *pThreadData,
                                   const char *topicString,
                                   ismEngine_Message_t *message,
                                   uint64_t publishSUV,
                                   ismEngine_Transaction_t *pTran,
                                   uint32_t options)
{
    static int32_t (* real_iett_updateRetainedMessage)(ieutThreadData_t *, const char *, ismEngine_Message_t *, uint64_t, ismEngine_Transaction_t *, uint32_t) = NULL;

    if (real_iett_updateRetainedMessage == NULL)
    {
        real_iett_updateRetainedMessage = dlsym(RTLD_NEXT, "iett_updateRetainedMessage");
        if (real_iett_updateRetainedMessage == NULL) abort();
    }

    int32_t rc = real_iett_updateRetainedMessage(pThreadData, topicString, message, publishSUV, pTran, options);

    if (post_iett_updateRetainedMessage_callback != NULL)
    {
        rc = post_iett_updateRetainedMessage_callback(rc,
                                                      post_iett_updateRetainedMessage_context);
    }
    else if (message == putNewRetainedAfterUpdateOfMsg)
    {
        publishCuckoo(message);
        putNewRetainedAfterUpdateOfMsg = NULL;
    }

    return rc;
}

//****************************************************************************
/// @brief Override iem_createMessageCopy
//****************************************************************************
ismEngine_Message_t *putNewRetainedAfterUpdateOfCopiedMsg = NULL;
ismEngine_Message_t *putNewRetainedBeforeCopyOfMsg = NULL;
int32_t iem_createMessageCopy(ieutThreadData_t *pThreadData,
                              ismEngine_Message_t *pMessage,
                              bool simpleCopy,
                              ism_time_t retainedTimestamp,
                              uint32_t retainedRealExpiry,
                              ismEngine_Message_t **ppNewMessage)
{
    static int32_t (* real_iem_createMessageCopy)(ieutThreadData_t *, ismEngine_Message_t *, bool, ism_time_t, uint32_t, ismEngine_Message_t **);

    if (real_iem_createMessageCopy == NULL)
    {
        real_iem_createMessageCopy = dlsym(RTLD_NEXT, "iem_createMessageCopy");
        if (real_iem_createMessageCopy == NULL) abort();
    }

    if (pMessage == putNewRetainedBeforeCopyOfMsg)
    {
        publishCuckoo(pMessage);
    }

    int32_t rc = real_iem_createMessageCopy(pThreadData, pMessage, simpleCopy, retainedTimestamp, retainedRealExpiry, ppNewMessage);

    if (pMessage == putNewRetainedAfterUpdateOfCopiedMsg)
    {
        putNewRetainedAfterUpdateOfMsg = *ppNewMessage;
    }

    return rc;
}

//****************************************************************************
/// @brief Attempt to force an expiry scan and return whether we did
//****************************************************************************
bool forceAnExpiryScan(ieutThreadData_t *pThreadData)
{
    iemeExpiryControl_t *expiryControl = (iemeExpiryControl_t *)ismEngine_serverGlobal.msgExpiryControl;

    // Ensure initial scan has finished
    uint64_t scansStarted = 1;
    while(scansStarted > expiryControl->scansEnded) { usleep(5000); }

    doRealExpiry = true;
    // Force expiry reaper to run
    scansStarted = expiryControl->scansEnded + 1;
    ieme_wakeMessageExpiryReaper(pThreadData); while(scansStarted > expiryControl->scansEnded) { usleep(5000); }
    doRealExpiry = false;

    return true;
}

//****************************************************************************
/// @brief Print some statistics
//****************************************************************************
void printStatsFunc(const char *func)
{
    iemnMessagingStatistics_t internalMsgStats;

    iemn_getMessagingStatistics(ieut_getThreadData(), &internalMsgStats);

    printf("%s ERM: %lu EIRM: %lu\n",
           func ? func : "NOFUNC",
           internalMsgStats.externalStats.RetainedMessages,
           internalMsgStats.InternalRetainedMessages);
}
void printStats(void)
{
    return printStatsFunc(NULL);
}


void test_resetThreadStats(ieutThreadData_t *pThreadData, void *context)
{
    memset(&pThreadData->stats, 0, sizeof(pThreadData->stats));
}

void test_resetAllThreadStats(void)
{
    memset(&ismEngine_serverGlobal.endedThreadStats, 0, sizeof(ismEngine_serverGlobal.endedThreadStats));
    ieut_enumerateThreadData(test_resetThreadStats, NULL);
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_countAsyncPut                                */
/*                                                                   */
/* Description:    Decrement a counter of outstand async puts.       */
/*                                                                   */
/*********************************************************************/
void test_countAsyncPut(int32_t retcode, void *handle, void *pContext)
{
    uint32_t *pPutCount = *(uint32_t **)pContext;

    if (retcode != OK) TEST_ASSERT_EQUAL(retcode, ISMRC_NoMatchingDestinations);

    __sync_fetch_and_sub(pPutCount, 1);
}

//****************************************************************************
/// @brief Copy of old iest_storeTopic to enable creation of 'old' topics
//****************************************************************************
int32_t test_iest_storeV1Topic(ieutThreadData_t        *pThreadData,
                               ismStore_StreamHandle_t  hStream,
                               const char              *topicString,
                               size_t                   topicStringLength,
                               ismStore_Handle_t       *phStoreTopic)
{
    int32_t                         rc = OK;
    ismStore_Record_t               storeRecord;
    ismStore_Handle_t               hStoreTopic;
    iestTopicDefinitionRecord_V1_t  topicDefinitionRecord;
    char                           *Fragments[2];       // Up to 2 fragment for a topic
    uint32_t                        FragmentLengths[2];

    // First fragment is always the topic definition record
    Fragments[0] = (char *)&topicDefinitionRecord;
    FragmentLengths[0] = sizeof(topicDefinitionRecord);
    storeRecord.DataLength = FragmentLengths[0];

    // Fill in the store record and contents of the topic record
    storeRecord.Type = ISM_STORE_RECTYPE_TOPIC;
    storeRecord.Attribute = 0;
    storeRecord.State = iestTDR_STATE_NONE;
    storeRecord.pFrags = Fragments;
    storeRecord.pFragsLengths = FragmentLengths;
    storeRecord.FragsCount = 1;

    memcpy(topicDefinitionRecord.StrucId, iestTOPIC_DEFN_RECORD_STRUCID, 4);
    topicDefinitionRecord.Version = iestTDR_VERSION_1;

    if (topicStringLength != 0)
    {
        topicDefinitionRecord.TopicStringLength = (uint32_t)topicStringLength+1;
        Fragments[storeRecord.FragsCount] = (char *)topicString;
        FragmentLengths[storeRecord.FragsCount] = topicDefinitionRecord.TopicStringLength;
        storeRecord.DataLength += FragmentLengths[storeRecord.FragsCount];
        storeRecord.FragsCount++;
    }
    else
    {
        topicDefinitionRecord.TopicStringLength = 0;
    }

    // Add to the store
    rc = ism_store_createRecord(hStream,
                                &storeRecord,
                                &hStoreTopic);

    if (rc != OK) goto mod_exit;

    assert(NULL != phStoreTopic);

    *phStoreTopic = hStoreTopic;

mod_exit:

    return rc;
}

typedef struct tag_checkDeliveryMessagesCbContext_t
{
    ismEngine_SessionHandle_t hSession;
    int32_t received;
    int32_t retained;
    int32_t published_for_retain;
    int32_t propagate_retained;
    int32_t persistent;
    int32_t nonpersistent;
    char *  retainedPayload;
    size_t  retainedPayloadSize;
} checkDeliveryMessagesCbContext_t;

bool checkDeliveryMessagesCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
    checkDeliveryMessagesCbContext_t *context = *((checkDeliveryMessagesCbContext_t **)pContext);

    context->received++;

    if (pMsgDetails->Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
    {
        context->persistent++;
    }
    else
    {
        context->nonpersistent++;
    }

    if (pMsgDetails->Flags & ismMESSAGE_FLAGS_RETAINED)
    {
        context->retained++;

        if ((context->retained == 1) && (context->retainedPayload != NULL))
        {
            for(uint8_t area = 0; area<areaCount; area++)
            {
                if (areaTypes[area] == ismMESSAGE_AREA_PAYLOAD)
                {
                    TEST_ASSERT_EQUAL(areaLengths[area], context->retainedPayloadSize);
                    TEST_ASSERT_EQUAL(memcmp(pAreaData[area],
                                             context->retainedPayload,
                                             context->retainedPayloadSize),0);
                }
            }
        }
    }

    if (pMsgDetails->Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN)
    {
        context->published_for_retain++;
    }

    if (pMsgDetails->Flags & ismMESSAGE_FLAGS_PROPAGATE_RETAINED)
    {
        context->propagate_retained++;
    }

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

uint32_t CHECKDELIVERY_SUBOPTS[] = {ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                                    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE,
                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                                    ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE};
#define CHECKDELIVERY_NUMSUBOPTS (sizeof(CHECKDELIVERY_SUBOPTS)/sizeof(CHECKDELIVERY_SUBOPTS[0]))

int32_t test_checkRetainedDelivery(ismEngine_SessionHandle_t hSession,
                                   char *topic,
                                   uint32_t subOptions,
                                   int32_t expectedReceived,
                                   int32_t expectedPersistent,
                                   int32_t expectedRetained,
                                   int32_t expectedPublishedForRetained,
                                   int32_t expectedPropagateRetained,
                                   char   *expectedRetainedPayload,
                                   size_t  expectedRetainedPayloadSize)
{
    int32_t rc;

    ismEngine_ConsumerHandle_t    hConsumer;
    ismEngine_SubscriptionAttributes_t subAttrs = { subOptions };

    checkDeliveryMessagesCbContext_t consumerContext = {0};
    checkDeliveryMessagesCbContext_t *consumerCb = &consumerContext;

    consumerCb->retainedPayload = expectedRetainedPayload;
    consumerCb->retainedPayloadSize = expectedRetainedPayloadSize;

    consumerContext.hSession = hSession;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   topic,
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumerCb,
                                   sizeof(checkDeliveryMessagesCbContext_t *),
                                   checkDeliveryMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumerContext.received, expectedReceived);
    TEST_ASSERT_EQUAL(consumerContext.persistent, expectedPersistent);
    TEST_ASSERT_EQUAL(consumerContext.nonpersistent, expectedReceived-expectedPersistent);
    TEST_ASSERT_EQUAL(consumerContext.retained, expectedRetained);
    TEST_ASSERT_EQUAL(consumerContext.published_for_retain, expectedPublishedForRetained);
    TEST_ASSERT_EQUAL(consumerContext.propagate_retained, expectedPropagateRetained);

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    return rc;
}

//****************************************************************************
/// @brief Test matching of retained messages to subscribers
//****************************************************************************
void testRetainedMatching(char  **retainedTopics,
                          char  **subscribeTopics,
                          int    *expectedRetaineds,
                          bool    verbose)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t                rc;

    char                 **currentTopicString;
    int                   *currentExpected;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    iettTopic_t topic;

    const char  *substrings[10];
    uint32_t     substringHashes[10];
    const char  *wildcards[10];
    const char  *multicards[10];

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    rc = test_createClientAndSession(__func__,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    /* Create a message to put */
    void   *payload=NULL;
    ismEngine_MessageHandle_t hMessage;

    /* Publish retained messages */
    currentTopicString = retainedTopics;
    while(*currentTopicString)
    {
        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, *currentTopicString,
                                &hMessage, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

#ifdef NDEBUG // Test non-standard use of ism_engine_putMessageInternalOnDestination
        if (rand()%2 == 0)
        {
            rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                            *currentTopicString,
                                                            hMessage,
                                                            NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
#endif
        {
            rc = ism_engine_putMessageOnDestination(hSession,
                                                    ismDESTINATION_TOPIC,
                                                    *currentTopicString,
                                                    NULL,
                                                    hMessage,
                                                    NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
        }

        currentTopicString += 1;
    }

    iettTopicTree_t *tree = iett_getEngineTopicTree(pThreadData);

    ieutHashSet_t *duplicateTest;

    rc = ieut_createHashSet(pThreadData, iemem_topicsQuery, &duplicateTest);
    TEST_ASSERT_EQUAL(rc, OK);

    /* Test Topics */
    currentTopicString = subscribeTopics;
    currentExpected = expectedRetaineds;
    while(*currentTopicString)
    {
        memset(&topic, 0, sizeof(topic));
        topic.destinationType = ismDESTINATION_TOPIC;
        topic.topicString = *currentTopicString;
        topic.substrings = substrings;
        topic.substringHashes = substringHashes;
        topic.wildcards = wildcards;
        topic.multicards = multicards;
        topic.initialArraySize = 10;

        rc = iett_analyseTopicString(pThreadData, &topic);

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(topic.topicStringCopy);
        TEST_ASSERT_EQUAL(topic.topicStringLength, strlen(*currentTopicString));

        uint32_t          maxNodes = 0;
        uint32_t          nodeCount = 0;
        iettTopicNode_t **topicNodes = NULL;

        rc = iett_findMatchingTopicsNodes(pThreadData,
                                          tree->topics, false,
                                          &topic,
                                          0, 0, 0,
                                          NULL, &maxNodes, &nodeCount, &topicNodes);

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(nodeCount, *currentExpected);

        if (*currentExpected == 0)
        {
            TEST_ASSERT_PTR_NULL(topicNodes);
        }
        else
        {
            TEST_ASSERT_PTR_NOT_NULL(topicNodes);

            // Check for duplicates
            // Add each node to a hash set
            for(int32_t i=0; i<nodeCount; i++)
            {
                rc = ieut_addValueToHashSet(pThreadData, duplicateTest, (uint64_t)topicNodes[i]);
                TEST_ASSERT_EQUAL(rc, OK);
            }
            TEST_ASSERT_EQUAL(nodeCount, duplicateTest->totalCount);

            rc = ieut_findValueInHashSet(duplicateTest, (uint64_t)topicNodes[0]);
            TEST_ASSERT_EQUAL(rc, OK);

            ieut_removeValueFromHashSet(duplicateTest, (uint64_t)topicNodes[0]);

            rc = ieut_findValueInHashSet(duplicateTest, (uint64_t)topicNodes[0]);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

            // Remove the lower half of the nodes (including node 0, again)
            for(int32_t i=0; i<nodeCount/2; i++)
            {
                ieut_removeValueFromHashSet(duplicateTest, (uint64_t)topicNodes[i]);
                rc = ieut_findValueInHashSet(duplicateTest, (uint64_t)topicNodes[i]);
                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
            }

            // Clear the hash set
            ieut_clearHashSet(pThreadData, duplicateTest);

            rc = ieut_findValueInHashSet(duplicateTest, (uint64_t)topicNodes[0]);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

            ieut_removeValueFromHashSet(duplicateTest, (uint64_t)topicNodes[0]);

            ieut_clearHashSet(pThreadData, duplicateTest);

            iemem_free(pThreadData, iemem_topicsQuery, topicNodes);
        }

        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        currentTopicString += 1;
        currentExpected += 1;
    }

    ieut_destroyHashSet(pThreadData, duplicateTest);

    currentTopicString = retainedTopics;
    while(*currentTopicString)
    {
        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                          ismDESTINATION_TOPIC,
                                                          *currentTopicString,
                                                          ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                          ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                          NULL,
                                                          &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc != ISMRC_AsyncCompletion)
        {
            TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        }

        currentTopicString += 1;
    }

    test_waitForRemainingActions(pActionsRemaining);

    currentTopicString = retainedTopics;
    while(*currentTopicString)
    {
        // Make sure we don't have any retained nodes in the topic tree for this string
        rc = iett_removeLocalRetainedMessages(pThreadData, *currentTopicString);
        TEST_ASSERT_EQUAL(rc, OK);

        currentTopicString += 1;
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    if (payload) free(payload);
}

void test_capability_RetainedMatching(void)
{
    printf("Starting %s...\n", __func__);

    char *pubTop1[] = {"A/B/C", "A/B/D", NULL};
    char *subTop1[] = {"X/Y", "A/B/C", "A/B/+", "A/+/C", "+/B/+", "A/B/#", "A/B/C/#", "A/#", "#", NULL};
    int   expectedPubs1[] = {0, 1, 2, 1, 2, 3, 1, 4, 4, 0};

    testRetainedMatching(pubTop1, subTop1, expectedPubs1, false);

    char *pubTop2[] = {"A/B/D/C/C/C", NULL};
    char *subTop2[] = {"#/C/#", "#/D/C/#", "+/#", "#/+", NULL};
    int   expectedPubs2[] = {3, 3, 6, 6, 0};

    testRetainedMatching(pubTop2, subTop2, expectedPubs2, false);

    char *pubTop3[] = {"A////D", NULL};
    char *subTop3[] = {"#/D", "A/#", "A//#", "A////+", NULL};
    int   expectedPubs3[] = {1, 5, 4, 1, 0};

    testRetainedMatching(pubTop3, subTop3, expectedPubs3, false);

    char *pubTop4[] = {"", "/", NULL};
    char *subTop4[] = {"#", "+", "", "+/+", "A", NULL};
    int   expectedPubs4[] = {2, 1, 1, 1, 0, 0};

    testRetainedMatching(pubTop4, subTop4, expectedPubs4, false);

    char *pubTop5[] = {"TEST/TEST/TEST/TEST", NULL};
    char *subTop5[] = {"#/TEST", "#/TEST/#", "+/TEST/#", NULL};
    int   expectedPubs5[] = {4, 4, 3, 0};

    testRetainedMatching(pubTop5, subTop5, expectedPubs5, false);

    // Make sure we don't have any retained nodes in the topic tree
    int32_t rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test basic operation of retained messages
//****************************************************************************
#define BASIC_RETAINED_TOPIC_1 "TEST/RETAINED"
#define BASIC_RETAINED_TOPIC_2 "TEST/RETAINED/SUB/TOPIC"
#define BASIC_RETAINED_TOPIC_3 "TEST/RETAINED/SUB/TOPIC/AND/MORE/LEVELS"
#define BASIC_RETAINED_TOPIC_4 "NOTHING_HERE"

typedef struct tag_retainedMessagesCbContext_t
{
    ismEngine_SessionHandle_t hSession;
    int32_t received;
} retainedMessagesCbContext_t;

bool retainedMessagesCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
    retainedMessagesCbContext_t *context = *((retainedMessagesCbContext_t **)pContext);

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

void test_capability_BasicRetainedMessages(void)
{
    uint32_t rc;

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    ismEngine_MessagingStatistics_t msgStats;
    iemnMessagingStatistics_t internalMsgStats;
    char resultPath[strlen(".partial")+1];

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    ism_engine_getMessagingStatistics(&msgStats);
    TEST_ASSERT_EQUAL(msgStats.RetainedMessages, 0);
    TEST_ASSERT_EQUAL(msgStats.ExpiredMessages, 0);

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(memcmp(&msgStats, &internalMsgStats.externalStats, sizeof(msgStats)), 0);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);

    rc = test_createClientAndSession("BasicRetainedMessageClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...create\n");

    // Try dumping our (empty) part of the topic tree
    resultPath[0]='\0';
    rc = test_log_TopicTree(testLOGLEVEL_VERBOSE, BASIC_RETAINED_TOPIC_1, 3, -1, resultPath);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    ismEngine_ProducerHandle_t    hProducer = NULL;

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_TOPIC,
                                   BASIC_RETAINED_TOPIC_1,
                                   &hProducer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer);

    void *payloadX = NULL;
    ismEngine_MessageHandle_t hMessageX;

    // Unset a retained message when there is no retained already on the topic
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_TOPIC,
                                                      BASIC_RETAINED_TOPIC_4,
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_createOriginMessage(0,
                                  ismMESSAGE_PERSISTENCE_PERSISTENT,
                                  ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                  ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                  0,
                                  ismDESTINATION_TOPIC, BASIC_RETAINED_TOPIC_1,
                                  (char *)ism_common_getServerUID(),
                                  ism_engine_retainedServerTime(),
                                  MTYPE_NullRetained,
                                  &hMessageX, &payloadX);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_UnreleasedHandle_t hUnrel = NULL;
    rc = sync_ism_engine_unsetRetainedMessageWithDeliveryId(hSession,
                                                            hProducer,
                                                            ismENGINE_UNSET_RETAINED_OPTION_PUBLISH,
                                                            NULL,
                                                            hMessageX,
                                                            5,
                                                            &hUnrel);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
    TEST_ASSERT_NOT_EQUAL(hUnrel, 0);

    rc = sync_ism_engine_removeUnreleasedDeliveryId(hSession, NULL, hUnrel);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyProducer(hProducer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_ConsumerHandle_t   consumer1;
    retainedMessagesCbContext_t  context1 = {0};
    retainedMessagesCbContext_t *cb1 = &context1;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };

    context1.hSession = hSession;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   BASIC_RETAINED_TOPIC_1,
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &cb1,
                                   sizeof(retainedMessagesCbContext_t *),
                                   retainedMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &consumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context1.received, 0);

    /* Create a message to put */
    void *payload1=NULL;
    ismEngine_MessageHandle_t hMessage1;

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, BASIC_RETAINED_TOPIC_1,
                            &hMessage1, &payload1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            BASIC_RETAINED_TOPIC_1,
                                            NULL,
                                            hMessage1,
                                            NULL, 0, NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, 1);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 1); // unset one

    // Try dumping the entire topic tree
    resultPath[0]='\0';
    rc = test_log_TopicTree(testLOGLEVEL_VERBOSE, NULL, 9, -1, resultPath);
    TEST_ASSERT_EQUAL(rc, OK);

    // Try dumping the specific node
    rc = test_log_Topic(testLOGLEVEL_VERBOSE, BASIC_RETAINED_TOPIC_1, 9, -1, "");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, BASIC_RETAINED_TOPIC_3,
                            &hMessage1, &payload1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            BASIC_RETAINED_TOPIC_3,
                                            NULL,
                                            hMessage1,
                                            NULL, 0, NULL );
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, 2);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 1); // unset one

    /* Create a persistent message to put */
    void *payload2=NULL;
    ismEngine_MessageHandle_t hMessage2;

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, BASIC_RETAINED_TOPIC_2,
                            &hMessage2, &payload2);
    TEST_ASSERT_EQUAL(rc, OK);

#ifdef NDEBUG // Test non-standard use of ism_engine_putMessageInternalOnDestination
    rc = sync_ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                         BASIC_RETAINED_TOPIC_2,
                                                         hMessage2);
    TEST_ASSERT_EQUAL(rc, OK);
#else
    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 BASIC_RETAINED_TOPIC_2,
                                                 NULL,
                                                 hMessage2);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
#endif

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, 3);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 1); // unset one

    ism_engine_getMessagingStatistics(&msgStats);
    TEST_ASSERT_EQUAL(memcmp(&msgStats, &internalMsgStats.externalStats, sizeof(msgStats)), 0);

    // Create a QoS2 subscription on BASIC_RETAINED_TOPIC_2
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = sync_ism_engine_createSubscription(hClient,
                                            "Durable Subscription",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            BASIC_RETAINED_TOPIC_2,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, "",
                            &hMessage2, &payload2);
    TEST_ASSERT_EQUAL(rc, OK);

    // Put a persistent retained on a zero length topic string (i.e. blank topic)
    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 "",
                                                 NULL,
                                                 hMessage2);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, 4);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 1); // unset one

    printf("  ...use\n");

    // ism_store_dump("stdout");

    ismEngine_ConsumerHandle_t   consumer2;
    retainedMessagesCbContext_t  context2 = {0};
    retainedMessagesCbContext_t *cb2 = &context2;

    context2.hSession = hSession;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   BASIC_RETAINED_TOPIC_1,
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &cb2,
                                   sizeof(retainedMessagesCbContext_t *),
                                   &retainedMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &consumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_TOPIC,
                                                      BASIC_RETAINED_TOPIC_1,
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    test_waitForRemainingActions(pActionsRemaining);

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, 3);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 2); // both unset messages

    if (forceAnExpiryScan(pThreadData) == true)
    {
        iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
        TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);
    }

    ismEngine_ConsumerHandle_t   consumer3;
    retainedMessagesCbContext_t  context3 = {0};
    retainedMessagesCbContext_t *cb3 = &context3;

    context3.hSession = hSession;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   BASIC_RETAINED_TOPIC_1,
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &cb3,
                                   sizeof(retainedMessagesCbContext_t *),
                                   &retainedMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &consumer3,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Switch non-persistent retained to persistent
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, BASIC_RETAINED_TOPIC_3,
                            &hMessage2, &payload2);
    TEST_ASSERT_EQUAL(rc, OK);

#ifdef NDEBUG // Test non-standard use of ism_engine_putMessageInternalOnDestination
    rc = sync_ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                         BASIC_RETAINED_TOPIC_3,
                                                         hMessage2);
    TEST_ASSERT_EQUAL(rc, OK);
#else
    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 BASIC_RETAINED_TOPIC_3,
                                                 NULL,
                                                 hMessage2);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
#endif

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, 3);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, BASIC_RETAINED_TOPIC_2,
                            &hMessage1, &payload1);
    TEST_ASSERT_EQUAL(rc, OK);

    // Switch persistent retained to non-persistent
    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            BASIC_RETAINED_TOPIC_2,
                                            NULL,
                                            hMessage1,
                                            NULL, 0, NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, 3);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);

    // ism_store_dump("stdout");

    printf("  ...remove\n");

    test_incrementActionsRemaining(pActionsRemaining, 3); // Wait for them all together...

    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_TOPIC,
                                                      BASIC_RETAINED_TOPIC_3,
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_TOPIC,
                                                      BASIC_RETAINED_TOPIC_2,
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_TOPIC,
                                                      "",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    test_waitForRemainingActions(pActionsRemaining);

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, 0);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 3); // The unset messages

    ism_engine_getMessagingStatistics(&msgStats);
    TEST_ASSERT_EQUAL(msgStats.RetainedMessages, 0);

    if (forceAnExpiryScan(pThreadData) == true)
    {
        iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
        TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);
    }

    printf("  ...disconnect\n");

    rc = ism_engine_destroySubscription(hClient, "Durable Subscription", hClient, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(context1.received, 1);
    TEST_ASSERT_EQUAL(context2.received, 1);
    TEST_ASSERT_EQUAL(context3.received, 0);

    if (payload1) free(payload1);
    if (payload2) free(payload2);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test the capability to republish retained messages to an existing sub
//****************************************************************************
typedef struct tag_republishMessagesCbContext_t
{
    ismEngine_SessionHandle_t hSession;
    int32_t received;
    int32_t retained;
    int32_t published_for_retain;
    int32_t propagate_retained;
} republishMessagesCbContext_t;

bool republishMessagesCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
    republishMessagesCbContext_t *context = *((republishMessagesCbContext_t **)pContext);

    context->received++;

    if (pMsgDetails->Flags & ismMESSAGE_FLAGS_RETAINED) context->retained++;
    if (pMsgDetails->Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN) context->published_for_retain++;
    if (pMsgDetails->Flags & ismMESSAGE_FLAGS_PROPAGATE_RETAINED) context->propagate_retained++;

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

char *RETREPUBLISH_TOPICS[] = {"RetRepublish", "+", "RetRepublish/#", "#"};
#define RETREPUBLISH_NUMTOPICS (sizeof(RETREPUBLISH_TOPICS)/sizeof(RETREPUBLISH_TOPICS[0]))

uint32_t RETREPUBLISH_SUBOPTS[] = {ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                                   ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE,
                                   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                                   ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE};
#define RETREPUBLISH_NUMSUBOPTS (sizeof(RETREPUBLISH_SUBOPTS)/sizeof(RETREPUBLISH_SUBOPTS[0]))

void test_capability_RetainedRepublish(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    ismEngine_ConsumerHandle_t hConsumer1,
                               hConsumer2,
                               hConsumer3;
    republishMessagesCbContext_t consumer1Context={0},
                                 consumer2Context={0},
                                 consumer3Context={0};
    republishMessagesCbContext_t *consumer1Cb=&consumer1Context,
                                 *consumer2Cb=&consumer2Context,
                                 *consumer3Cb=&consumer3Context;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("RetainedRepublishClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...create\n");

    consumer1Context.hSession = hSession;
    subAttrs.subOptions = RETREPUBLISH_SUBOPTS[rand()%RETREPUBLISH_NUMSUBOPTS];

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   RETREPUBLISH_TOPICS[rand()%RETREPUBLISH_NUMTOPICS],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer1Cb,
                                   sizeof(republishMessagesCbContext_t *),
                                   republishMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Try a republish when there are no retained messages
    rc = ism_engine_republishRetainedMessages(hSession,
                                              hConsumer1,
                                              NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer1Context.received, 0);
    TEST_ASSERT_EQUAL(consumer1Context.retained, 0);
    TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, 0);
    TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, 0);

    // Create a retained message
    void *payload1="RetPublishMsg";
    ismEngine_MessageHandle_t hMessage1;

    rc = test_createMessage(strlen(payload1)+1,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, RETREPUBLISH_TOPICS[0],
                            &hMessage1, &payload1);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...work\n");

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            RETREPUBLISH_TOPICS[0],
                                            NULL,
                                            hMessage1,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer1Context.received, 1);
    TEST_ASSERT_EQUAL(consumer1Context.retained, 0); // Got published live
    TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, 1);
    TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, 1);

    rc = ism_engine_republishRetainedMessages(hSession,
                                              hConsumer1,
                                              NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer1Context.received, 2);
    TEST_ASSERT_EQUAL(consumer1Context.retained, 1); // Republished as retained
    TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, 2);
    TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, 2);

    consumer2Context.hSession = hSession;
    subAttrs.subOptions = RETREPUBLISH_SUBOPTS[rand()%RETREPUBLISH_NUMSUBOPTS];
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   RETREPUBLISH_TOPICS[rand()%RETREPUBLISH_NUMTOPICS],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer2Cb,
                                   sizeof(republishMessagesCbContext_t *),
                                   republishMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer2Context.received, 1);
    TEST_ASSERT_EQUAL(consumer2Context.retained, 1);
    TEST_ASSERT_EQUAL(consumer2Context.published_for_retain, 1);
    TEST_ASSERT_EQUAL(consumer2Context.propagate_retained, 1);

    rc = ism_engine_republishRetainedMessages(hSession,
                                              hConsumer2,
                                              NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer2Context.received, 2);
    TEST_ASSERT_EQUAL(consumer2Context.retained, 2);
    TEST_ASSERT_EQUAL(consumer2Context.published_for_retain, 2);
    TEST_ASSERT_EQUAL(consumer2Context.propagate_retained, 2);

    // Check nothing got republished to other consumers
    TEST_ASSERT_EQUAL(consumer1Context.received, 2);
    TEST_ASSERT_EQUAL(consumer1Context.retained, 1);
    TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, 2);
    TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, 2);

    // Create a durable subscription
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = sync_ism_engine_createSubscription(hClient,
                                            "TEST_REPUBLISH_SUB",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            RETREPUBLISH_TOPICS[rand()%RETREPUBLISH_NUMTOPICS],
                                            &subAttrs,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "TEST_REPUBLISH_SUB",
                                   NULL,
                                   NULL, // Use the session's client
                                   &consumer3Cb,
                                   sizeof(republishMessagesCbContext_t *),
                                   republishMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumer3,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer3Context.received, 1);
    TEST_ASSERT_EQUAL(consumer3Context.received, 1);
    TEST_ASSERT_EQUAL(consumer3Context.published_for_retain, 1);
    TEST_ASSERT_EQUAL(consumer3Context.propagate_retained, 1);

    rc = ism_engine_republishRetainedMessages(hSession,
                                              hConsumer3,
                                              NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer3Context.received, 2);
    TEST_ASSERT_EQUAL(consumer3Context.retained, 2);
    TEST_ASSERT_EQUAL(consumer3Context.published_for_retain, 2);
    TEST_ASSERT_EQUAL(consumer3Context.propagate_retained, 2);

    rc = ism_engine_destroyConsumer(hConsumer3, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySubscription(hClient, "TEST_REPUBLISH_SUB", hClient, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_TOPIC,
                                                      RETREPUBLISH_TOPICS[0],
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    test_waitForRemainingActions(pActionsRemaining);

    rc = ism_engine_republishRetainedMessages(hSession,
                                              hConsumer1,
                                              NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer1Context.received, 2);
    TEST_ASSERT_EQUAL(consumer1Context.retained, 1);
    TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, 2);
    TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, 2);

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test retained messages when used with selection
//****************************************************************************
typedef struct tag_retSelMessagesCbContext_t
{
    ismEngine_SessionHandle_t hSession;
    volatile int32_t received;
    int32_t retained;
    int32_t published_for_retain;
    int32_t propagate_retained;
} retSelMessagesCbContext_t;

bool retSelMessagesCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
    retSelMessagesCbContext_t *context = *((retSelMessagesCbContext_t **)pContext);

    if (pMsgDetails->Flags & ismMESSAGE_FLAGS_RETAINED) context->retained++;
    if (pMsgDetails->Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN) context->published_for_retain++;
    if (pMsgDetails->Flags & ismMESSAGE_FLAGS_PROPAGATE_RETAINED) context->propagate_retained++;

    ism_engine_releaseMessage(hMessage);

    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        uint32_t rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                        NULL,
                                                        hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                        NULL, 0, NULL);
        TEST_ASSERT_CUNIT(((rc == OK) || (rc == ISMRC_AsyncCompletion)),
                          ("rc was %d", rc));
    }

    __sync_add_and_fetch(&context->received, 1);

    return true; // more messages, please.
}

void test_capability_RetainedSelection(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    ismEngine_ConsumerHandle_t hConsumer1,
                               hConsumer2,
                               hConsumer3,
                               hConsumer4,
                               hConsumer5;
    retSelMessagesCbContext_t consumer1Context={0},
                              consumer2Context={0},
                              consumer3Context={0},
                              consumer4Context={0},
                              consumer5Context={0};
    retSelMessagesCbContext_t *consumer1Cb=&consumer1Context,
                              *consumer2Cb=&consumer2Context,
                              *consumer3Cb=&consumer3Context,
                              *consumer4Cb=&consumer4Context,
                              *consumer5Cb=&consumer5Context;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("RetainedSelectionClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...create\n");

    // Publish a triangular message as a retained message
    ism_prop_t *msgProps = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(msgProps);
    ism_field_t f;

    f.type = VT_String;
    f.val.s = "TRIANGLE";
    rc = ism_common_setProperty(msgProps, "Shape", &f);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaTypes[2];
    size_t areaLengths[2];
    void *areas[2];
    concat_alloc_t FlatProperties = { NULL };
    char localPropBuffer[1024];

    FlatProperties.buf = localPropBuffer;
    FlatProperties.len = 1024;

    rc = ism_common_serializeProperties(msgProps, &FlatProperties);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_common_freeProperties(msgProps);

    // Add the topic to the properties
    char *topicString = "Retained/Selection";
    f.type = VT_String;
    f.val.s = topicString;
    ism_common_putPropertyID(&FlatProperties, ID_Topic, &f);

    // Add the serverUID to the properties
    f.type = VT_String;
    f.val.s = (char *)ism_common_getServerUID();
    ism_common_putPropertyID(&FlatProperties, ID_OriginServer, &f);

    // Add the serverTime to the properties
    f.type = VT_Long;
    f.val.l = ism_engine_retainedServerTime();
    ism_common_putPropertyID(&FlatProperties, ID_ServerTime, &f);

    areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
    areaLengths[0] = FlatProperties.used;
    areas[0] = FlatProperties.buf;
    areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
    areaLengths[1] = strlen(topicString)+1;
    areas[1] = (void *)topicString;

    header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
    header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
    header.Flags = ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;

    ismEngine_MessageHandle_t hMessage = NULL;
    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areas,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            "Retained/Selection",
                                            NULL,
                                            hMessage,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    printf("  ...work\n");

    // Initiliase the properties which contain the selection string
    ism_prop_t *properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);

    // Create a subscription whose selection does not match
    char *selectorString = "Shape='CIRCLE'";
    ism_field_t SelectorField = {VT_String, 0, {.s = selectorString }};
    rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
    TEST_ASSERT_EQUAL(rc, OK);

    consumer1Context.hSession = hSession;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |
                          ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "Retained/Selection",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer1Cb,
                                   sizeof(retSelMessagesCbContext_t *),
                                   retSelMessagesCallback,
                                   properties,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer1Context.received, 0);
    TEST_ASSERT_EQUAL(consumer1Context.retained, 0);
    TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, 0);
    TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, 0);

    // Create a subscription whose selection does match
    SelectorField.val.s = "Shape='TRIANGLE'";
    rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
    TEST_ASSERT_EQUAL(rc, OK);

    consumer2Context.hSession = hSession;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |
                          ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "Retained/Selection",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer2Cb,
                                   sizeof(retSelMessagesCbContext_t *),
                                   retSelMessagesCallback,
                                   properties,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer2Context.received, 1);
    TEST_ASSERT_EQUAL(consumer2Context.retained, 1);
    TEST_ASSERT_EQUAL(consumer2Context.published_for_retain, 1);
    TEST_ASSERT_EQUAL(consumer2Context.propagate_retained, 1);

    // Republish to consumer1 - still should not match
    rc = ism_engine_republishRetainedMessages(hSession,
                                              hConsumer1,
                                              NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer1Context.received, 0);
    TEST_ASSERT_EQUAL(consumer1Context.retained, 0);
    TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, 0);
    TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, 0);

    // No selection
    consumer3Context.hSession = hSession;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "Retained/Selection/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer3Cb,
                                   sizeof(retSelMessagesCbContext_t *),
                                   retSelMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer3,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer3Context.received, 1);
    TEST_ASSERT_EQUAL(consumer3Context.retained, 1);
    TEST_ASSERT_EQUAL(consumer3Context.published_for_retain, 1);
    TEST_ASSERT_EQUAL(consumer3Context.propagate_retained, 1);

    // Reliable selection
    consumer4Context.hSession = hSession;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE | ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "Retained/Selection/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer4Cb,
                                   sizeof(retSelMessagesCbContext_t *),
                                   retSelMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer4,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer4Context.received, 1);
    TEST_ASSERT_EQUAL(consumer4Context.retained, 1);
    TEST_ASSERT_EQUAL(consumer4Context.published_for_retain, 1);
    TEST_ASSERT_EQUAL(consumer4Context.propagate_retained, 1);

    // Unreliable selection
    consumer4Context.hSession = hSession;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE | ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "Retained/Selection/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer5Cb,
                                   sizeof(retSelMessagesCbContext_t *),
                                   retSelMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer5,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer5Context.received, 0);
    TEST_ASSERT_EQUAL(consumer5Context.retained, 0);
    TEST_ASSERT_EQUAL(consumer5Context.published_for_retain, 0);
    TEST_ASSERT_EQUAL(consumer5Context.propagate_retained, 0);

    // Selection on JMS_IBM_Retain - a pseudo-property - testing for match / no match
    ismEngine_ConsumerHandle_t hConsumer6;
    retSelMessagesCbContext_t consumer6Context={0};
    retSelMessagesCbContext_t *consumer6Cb=&consumer6Context;

    for(int32_t i=0; i<2; i++)
    {
        // First loop look for a match, second no-match
        SelectorField.val.s = (i==0) ? "JMS_IBM_Retain=1" : "JMS_IBM_Retain=0";
        rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
        TEST_ASSERT_EQUAL(rc, OK);

        consumer6Context.hSession = hSession;
        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |
                              ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION;
        rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "Retained/Selection",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer6Cb,
                                   sizeof(retSelMessagesCbContext_t *),
                                   retSelMessagesCallback,
                                   properties,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer6,
                                   NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        // Note: We used the same context, so the 2nd loop should be the same result as the first!
        TEST_ASSERT_EQUAL(consumer6Context.received, 1);
        TEST_ASSERT_EQUAL(consumer6Context.retained, 1);
        TEST_ASSERT_EQUAL(consumer6Context.published_for_retain, 1);
        TEST_ASSERT_EQUAL(consumer6Context.propagate_retained, 1);

        rc = ism_engine_destroyConsumer(hConsumer6, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    printf("  ...disconnect\n");

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_TOPIC,
                                                      "Retained/Selection",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    ism_common_freeProperties(properties);

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    test_waitForRemainingActions(pActionsRemaining);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

uint32_t RETFLAGS_DURSUBOPTS[] = {ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                                  ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE,
                                  ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                                  ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE};
#define RETFLAGS_NUMDURSUBOPTS (sizeof(RETFLAGS_DURSUBOPTS)/sizeof(RETFLAGS_DURSUBOPTS[0]))


#define DURSUBOPT1 (ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE)
#define DURSUBOPT2 (ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE)
#define MSGS_TO_PUT 10
void test_capability_RetainedFlags_Phase1(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient[5];
    ismEngine_SessionHandle_t hSession[5];
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    ismEngine_ConsumerHandle_t hConsumer;
    retSelMessagesCbContext_t consumerContext={0};
    retSelMessagesCbContext_t *consumerCb=&consumerContext;
    ismEngine_MessagingStatistics_t msgStats = {0};

    printf("Starting %s...\n", __func__);

    for(uint32_t i=0;i<sizeof(hClient)/sizeof(hClient[0]);i++)
    {
        char clientId[20];

        sprintf(clientId, "FlagsClient%u", i+1);

        rc = test_createClientAndSession(clientId,
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hClient[i], &hSession[i], true);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Create two durable subscription
    subAttrs.subOptions = DURSUBOPT1;
    rc = sync_ism_engine_createSubscription(hClient[0],
                                            "DurableSubOne",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "Flag/Topic",
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = DURSUBOPT2;
    rc = sync_ism_engine_createSubscription(hClient[1],
                                            "DurableSubTwo",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "Flag/Topic/#",
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    // And one nondurable subscription
    consumerContext.hSession = hSession[2];
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSession[2],
                                   ismDESTINATION_TOPIC,
                                   "Flag/+",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumerCb,
                                   sizeof(retSelMessagesCbContext_t *),
                                   retSelMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    // Publish 10 persistent, retained messages
    test_incrementActionsRemaining(pActionsRemaining, MSGS_TO_PUT);
    for (int32_t i=0; i<MSGS_TO_PUT; i++)
    {
        void *payload=NULL;
        ismEngine_MessageHandle_t hMessage;

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, "Flag/Topic",
                                &hMessage, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

#ifdef NDEBUG // Test non-standard use of ism_engine_putMessageInternalOnDestination
        if (i%3 == 0)
        {
            rc = sync_ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                                 "Flag/Topic",
                                                                 hMessage);
        }
        else
#endif
        {
            rc = sync_ism_engine_putMessageOnDestination(hSession[0],
                                                         ismDESTINATION_TOPIC,
                                                         "Flag/Topic",
                                                         NULL,
                                                         hMessage);
        }

        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

        free(payload);
    }

    test_waitForRemainingActions(pActionsRemaining);

    // Check they were received as expected by the non-durable consumer
    TEST_ASSERT_EQUAL(consumerContext.received, 10);
    TEST_ASSERT_EQUAL(consumerContext.published_for_retain, 10);
    TEST_ASSERT_EQUAL(consumerContext.retained, 0);
    TEST_ASSERT_EQUAL(consumerContext.propagate_retained, 10);

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    memset(&consumerContext, 0, sizeof(consumerContext));

    TEST_ASSERT_EQUAL(consumerContext.received, 0);

    // Attach a consumer to one durable subscription
    consumerContext.hSession = hSession[0];
    rc = ism_engine_createConsumer(hSession[0],
                                   ismDESTINATION_SUBSCRIPTION,
                                   "DurableSubOne",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &consumerCb,
                                   sizeof(retSelMessagesCbContext_t *),
                                   retSelMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(DURSUBOPT1),
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    test_waitForMessages((volatile uint32_t *)&(consumerContext.received), NULL, 10, 20);
    TEST_ASSERT_EQUAL(consumerContext.received, 10);
    TEST_ASSERT_EQUAL(consumerContext.published_for_retain, 10);
    TEST_ASSERT_EQUAL(consumerContext.retained, 0);
    TEST_ASSERT_EQUAL(consumerContext.propagate_retained, 1);

    rc = sync_ism_engine_destroyConsumer(hConsumer);
    TEST_ASSERT_EQUAL(rc, OK);

    memset(&consumerContext, 0, sizeof(consumerContext));

    TEST_ASSERT_EQUAL(consumerContext.received, 0);

    ism_engine_getMessagingStatistics(&msgStats);
    TEST_ASSERT_EQUAL(msgStats.RetainedMessages, 1);

    for(uint32_t i=0;i<sizeof(hClient)/sizeof(hClient[0]);i++)
    {
        rc = sync_ism_engine_destroyClientState(hClient[i],
                                                ismENGINE_DESTROY_CLIENT_OPTION_NONE);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

void test_capability_RetainedFlags_Phase2(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient[5];
    ismEngine_SessionHandle_t hSession[5];
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    ismEngine_ConsumerHandle_t hConsumer;
    retSelMessagesCbContext_t consumerContext={0};
    retSelMessagesCbContext_t *consumerCb=&consumerContext;
    ismEngine_MessagingStatistics_t msgStats = {0};

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    ism_engine_getMessagingStatistics(&msgStats);
    TEST_ASSERT_EQUAL(msgStats.RetainedMessages, 1);

    rc = test_createClientAndSession("FlagsClient2",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient[1], &hSession[1], true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Attach a consumer to second durable subscription
    consumerContext.hSession = hSession[1];
    subAttrs.subOptions = DURSUBOPT2;
    rc = ism_engine_createConsumer(hSession[1],
                                   ismDESTINATION_SUBSCRIPTION,
                                   "DurableSubTwo",
                                   &subAttrs,
                                   NULL, // Owning client same as session client
                                   &consumerCb,
                                   sizeof(retSelMessagesCbContext_t *),
                                   retSelMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // All discarded because non-durable
    test_waitForMessages((volatile uint32_t *)&(consumerContext.received), NULL, 10, 20);
    TEST_ASSERT_EQUAL(consumerContext.received, 10);
    TEST_ASSERT_EQUAL(consumerContext.published_for_retain, 10);
    TEST_ASSERT_EQUAL(consumerContext.retained, 0);
    TEST_ASSERT_EQUAL(consumerContext.propagate_retained, 1); // Only the latest will still be flagged

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession[1],
                                                      ismDESTINATION_TOPIC,
                                                      "Flag/Topic",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_destroyClientAndSession(hClient[1], hSession[1], true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_WillMsgRetained_Phase1(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient, hTestClient;
    ismEngine_SessionHandle_t hSession, hTestSession;
    ismEngine_ConsumerHandle_t hConsumer1, hConsumer2;
    retSelMessagesCbContext_t consumer1Context={0}, consumer2Context={0};
    retSelMessagesCbContext_t *consumer1Cb=&consumer1Context, *consumer2Cb=&consumer2Context;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("TestMsgs",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hTestClient, &hTestSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a consumer to get messages live
    consumer1Cb->hSession = hTestSession;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = ism_engine_createConsumer(hTestSession,
                                   ismDESTINATION_TOPIC,
                                   "WILLTOPIC",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer1Cb,
                                   sizeof(retSelMessagesCbContext_t *),
                                   retSelMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer1Context.received, 0);

    // Now create our will message producing client
    rc = test_createClientAndSession("RetainedClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient,
                                     &hSession,
                                     true);
    TEST_ASSERT_EQUAL(rc, OK);

    // create a retained message which we will use as our will message
    ismEngine_MessageHandle_t hMessage;
    void *payload=NULL;
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_NONE, // Don't want properties
                            0,
                            ismDESTINATION_TOPIC, "WILLTOPIC",
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);
    free(payload);

    hMessage->Header.Flags = ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN; // Really want retained

    // Set this as the will message
    rc = ism_engine_setWillMessage(hClient,
                                   ismDESTINATION_TOPIC,
                                   "WILLTOPIC",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Destroy the client
    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer1Context.received, 1);
    TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, 1);
    TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, 1);
    TEST_ASSERT_EQUAL(consumer1Context.retained, 0); // Received it live

    // Check that a new subscriber gets this message - i.e. it's retained
    consumer2Cb->hSession = hTestSession;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = ism_engine_createConsumer(hTestSession,
                                   ismDESTINATION_TOPIC,
                                   "WILLTOPIC",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer2Cb,
                                   sizeof(retSelMessagesCbContext_t *),
                                   retSelMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer2Context.received, 1);
    TEST_ASSERT_EQUAL(consumer2Context.retained, 1);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hTestSession,
                                                      ismDESTINATION_TOPIC,
                                                      "WILLTOPIC",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    test_waitForRemainingActions(pActionsRemaining);

    // Try it with a persistent message
    rc = sync_ism_engine_createClientState("RetainedClient",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &hClient);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    // create a retained message which we will use as our will message
    payload=NULL;
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_NONE, // Don't add the properties for retained
                            0,
                            ismDESTINATION_TOPIC, "WILLTOPIC",
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);
    free(payload);

    hMessage->Header.Flags = ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN; // Really want retained

    // Set this as the will message
    rc = ism_engine_setWillMessage(hClient,
                                   ismDESTINATION_TOPIC,
                                   "WILLTOPIC",
                                   hMessage,
                                   0,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumer1, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_destroyConsumer(hConsumer2, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroyClientState(hTestClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_WillMsgRetained_Phase2(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hTestClient;
    ismEngine_SessionHandle_t hTestSession;
    ismEngine_ConsumerHandle_t hConsumer1;
    retSelMessagesCbContext_t consumer1Context={0};
    retSelMessagesCbContext_t *consumer1Cb=&consumer1Context;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    // Bounce the server - this should result in the message being published (and retained) during restart
    test_bounceEngine();

    rc = test_createClientAndSession("TestMsgs",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hTestClient, &hTestSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a consumer
    memset(&consumer1Context, 0, sizeof(consumer1Context));
    consumer1Cb->hSession = hTestSession;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = ism_engine_createConsumer(hTestSession,
                                   ismDESTINATION_TOPIC,
                                   "WILLTOPIC",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer1Cb,
                                   sizeof(retSelMessagesCbContext_t *),
                                   retSelMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer1Context.received, 1);
    TEST_ASSERT_EQUAL(consumer1Context.retained, 1);
    TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, 1); // Check that PFR is persisted
    TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, 1);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hTestSession,
                                                      ismDESTINATION_TOPIC,
                                                      "WILLTOPIC",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    test_waitForRemainingActions(pActionsRemaining);

    rc = ism_engine_destroyConsumer(hConsumer1, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hTestClient, hTestSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);

    // The retained will message will have actually contributed to a stat on an
    // persistent store thread - so reset all the stats now to keep things clean
    // for further tests.
    test_resetAllThreadStats();
}

//****************************************************************************
/// @brief Test that the engine separates retained messages on system topics
///        from those on non-system topics
//****************************************************************************
void test_capability_SystemSeparation(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("Client",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish retained messages on various topics
    char *RetainTopic[] = { "$SYS/NODE1/QUEUE", "$SYS/NODE1/TOPIC", "$SYS/NODE2/QUEUE",
                            "$SYS/NODE2/TOPIC", "$SYS/NODE3/TOPIC",
                            "USER/NODE1/QUEUE", "USER/NODE2/QUEUE", "NODE1/QUEUE",
                            "/NODE1/QUEUE", "QUEUE/NODE1/$SYS", "$SYSTEM/NODE1/QUEUE",
                            NULL };

    for(int32_t i=0; RetainTopic[i] != NULL; i++)
    {
        void *payload=NULL;
        ismEngine_MessageHandle_t hMessage;

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, RetainTopic[i],
                                &hMessage, &payload);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(payload);

        free(payload);

#ifdef NDEBUG // Test non-standard use of ism_engine_putMessageInternalOnDestination
        if (rand()%2 == 0)
        {
            rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                            RetainTopic[i],
                                                            hMessage,
                                                            NULL, 0, NULL );
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
#endif
        {
            rc = ism_engine_putMessageOnDestination(hSession,
                                                    ismDESTINATION_TOPIC,
                                                    RetainTopic[i],
                                                    NULL,
                                                    hMessage,
                                                    NULL, 0, NULL );
            TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
        }
    }

    test_checkRetainedDelivery(hSession, "$SYS", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               0, 0, 0, 0, 0, NULL, 0);
    test_checkRetainedDelivery(hSession, "$SYS/#", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               5, 0, 5, 5, 5, NULL, 0);
    test_checkRetainedDelivery(hSession, "NODE1/QUEUE", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               1, 0, 1, 1, 1, NULL, 0);
    test_checkRetainedDelivery(hSession, "$SYS/+/TOPIC", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               3, 0, 3, 3, 3, NULL, 0);
    test_checkRetainedDelivery(hSession, "#", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               5, 0, 5, 5, 5, NULL, 0);
    test_checkRetainedDelivery(hSession, "+/NODE1/QUEUE", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               2, 0, 2, 2, 2, NULL, 0);
    test_checkRetainedDelivery(hSession, "#/NODE1/#", ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                               4, 0, 4, 4, 4, NULL, 0);
    // "QUEUE/NODE1/$SYS"
    test_checkRetainedDelivery(hSession, "#/$SYS/#", ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                               1, 0, 1, 1, 1, NULL, 0);
    test_checkRetainedDelivery(hSession, "+/+/TOPIC", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               0, 0, 0, 0, 0, NULL, 0);
    test_checkRetainedDelivery(hSession, "$SYSTEM/#", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               1, 0, 1, 1, 1, NULL, 0);

    for(int32_t i=0; RetainTopic[i] != NULL; i++)
    {
        rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                          ismDESTINATION_TOPIC,
                                                          RetainTopic[i],
                                                          ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                          ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                          NULL,
                                                          NULL, 0, NULL);

        // Make sure we don't have a retained node in the tree for this topic
        rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), RetainTopic[i]);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
}


typedef struct tag_getRetainedMsgCBContext_t {
      bool expectMessage;
      void *expectedPayload;
      uint32_t expectedPayloadLen;
      uint32_t receivedCount;
      uint32_t stopAt;
} getRetainedMsgCBContext_t;

bool getRetainedMsgCB(ismEngine_ConsumerHandle_t      hConsumer,
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
    getRetainedMsgCBContext_t *context = *(getRetainedMsgCBContext_t **)pContext;

    context->receivedCount++;

    TEST_ASSERT_EQUAL(hDelivery, ismENGINE_NULL_DELIVERY_HANDLE);
    TEST_ASSERT_EQUAL(hConsumer, NULL);

    if (context->expectMessage)
    {
        if (context->expectedPayload != NULL)
        {
            TEST_ASSERT_EQUAL(areaCount, 2);
            TEST_ASSERT_EQUAL(areaLengths[1], context->expectedPayloadLen);

            if (memcmp(pAreaData[1], context->expectedPayload, context->expectedPayloadLen) != 0)
            {
                TEST_ASSERT_CUNIT(0, ("Received a message with the wrong payload"));
            }
        }
    }
    else
    {
        TEST_ASSERT_CUNIT(0, ("Received a message when we expected not to"));
    }

    ism_engine_releaseMessage(hMessage);

    return (context->stopAt == 0) || (context->receivedCount < context->stopAt);
}

//****************************************************************************
/// @brief Test that the engine API ism_engine_getRetainedMessage()
/// works as expected
//****************************************************************************
void test_capability_GetRetainedMessage(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("getRetainedMsgClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish retained messages on various topics
    char *RetainTopic = "/test/getRetainedMessage/lvl1/lvl2";

    void *payload=NULL;
    ismEngine_MessageHandle_t hMessage;

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, RetainTopic,
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(payload);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            RetainTopic,
                                            NULL,
                                            hMessage,
                                            NULL, 0, NULL );
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    //Check that we get the correct message on the correct topic:

    getRetainedMsgCBContext_t context = {0};
    getRetainedMsgCBContext_t *pContext = &context;

    context.expectMessage = true;
    context.expectedPayload = payload;
    context.expectedPayloadLen = TEST_SMALL_MESSAGE_SIZE;
    context.receivedCount = 0;

    rc = ism_engine_getRetainedMessage(hSession,
                                       RetainTopic,
                                       &pContext,
                                       sizeof(pContext),
                                       getRetainedMsgCB,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.receivedCount, 1);

    context.receivedCount = 0;
    rc = ism_engine_getRetainedMessage(hSession,
                                       "/test/+/lvl1/lvl2/#",
                                       &pContext,
                                       sizeof(pContext),
                                       getRetainedMsgCB,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.receivedCount, 1);

    //Check that we don't get a messages on other topics
    //...with no topic node first!
    context.expectMessage = false;
    context.expectedPayload = NULL;
    context.expectedPayloadLen = 0;

    rc = ism_engine_getRetainedMessage(hSession,
                                       "/test/getRetainedMessage/lvl1/lvl2/lvl3/lvl4",
                                       &pContext,
                                       sizeof(pContext),
                                       getRetainedMsgCB,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_EQUAL(context.receivedCount, 1);

    //and one where a node exists:
    rc = ism_engine_getRetainedMessage(hSession,
                                       "/test/getRetainedMessage",
                                       &pContext,
                                       sizeof(pContext),
                                       getRetainedMsgCB,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc,  ISMRC_NotFound);
    TEST_ASSERT_EQUAL(context.receivedCount, 1);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_TOPIC,
                                                      RetainTopic,
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == ISMRC_NoMatchingDestinations) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    //Now the message has been removed, we shouldn't get it...
    rc = ism_engine_getRetainedMessage(hSession,
                                       RetainTopic,
                                       &pContext,
                                       sizeof(pContext),
                                       getRetainedMsgCB,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc,  ISMRC_NotFound);
    TEST_ASSERT_EQUAL(context.receivedCount, 1);

    free(payload);

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test the unsetting of retained messages using a regular expression
//****************************************************************************
void test_capability_UnsetUsingRegEx(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("PubRetainedClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t i=0; i<200; i++)
    {
        char topicString[50];

        sprintf(topicString, "%s%c/Test/%08d", i<100 ? "/":"", 'A' + ((i%10)%10), i);

        void *payload=NULL;
        ismEngine_MessageHandle_t hMsg;

        rc = test_createMessage(10,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, topicString,
                                &hMsg, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        // Publish message
        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                NULL,
                                                hMsg,
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

        free(payload);
    }

    // Check that we have all the expected messages
    test_checkRetainedDelivery(hSession, "#", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               200, 0, 200, 200, 200, NULL, 0);

    // Remove all of the topics starting with 'A' twice (to force an empty set)
    for(int32_t loop=0; loop<2; loop++)
    {
        rc = ism_engine_unsetRetainedMessageOnDestination(NULL,
                                                          ismDESTINATION_REGEX_TOPIC,
                                                          "^A.*",
                                                          ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                          ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                          NULL,
                                                          NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check the right set have been removed
        test_checkRetainedDelivery(hSession, "#", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                                   190, 0, 190, 190, 190, NULL, 0);
        test_checkRetainedDelivery(hSession, "A/#", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                                   0, 0, 0, 0, 0, NULL, 0);
        test_checkRetainedDelivery(hSession, "/A/#", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                                   10, 0, 10, 10, 10, NULL, 0);
    }

    // Remove all starting '/B'
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_REGEX_TOPIC,
                                                      "^/B.*.+",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the right set have been removed
    test_checkRetainedDelivery(hSession, "#", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               180, 0, 180, 180, 180, NULL, 0);
    test_checkRetainedDelivery(hSession, "/B/#", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               0, 0, 0, 0, 0, NULL, 0);
    test_checkRetainedDelivery(hSession, "B/Test/#", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               10, 0, 10, 10, 10, NULL, 0);

    // Try a non-anchored regex
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_REGEX_TOPIC,
                                                      ".*002.+",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the right set have been removed
    test_checkRetainedDelivery(hSession, "#", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               171, 0, 171, 171, 171, NULL, 0);
    test_checkRetainedDelivery(hSession, "J/Test/00000029", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               0, 0, 0, 0, 0, NULL, 0);
    test_checkRetainedDelivery(hSession, "J/Test/00000189", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               1, 0, 1, 1, 1, NULL, 0);

    // Remove anything that's left
    rc = ism_engine_unsetRetainedMessageOnDestination(NULL,
                                                      ismDESTINATION_REGEX_TOPIC,
                                                      ".*",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the right set have been removed
    test_checkRetainedDelivery(hSession, "#", CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS],
                               0, 0, 0, 0, 0, NULL, 0);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

char *RETEXCLUDE_TOPICS[] = {"ExcludeRetained", "#"};
#define RETEXCLUDE_NUMTOPICS (sizeof(RETEXCLUDE_TOPICS)/sizeof(RETEXCLUDE_TOPICS[0]))

uint32_t RETEXCLUDE_SUBOPTS[] = {ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE | ismENGINE_SUBSCRIPTION_OPTION_NO_RETAINED_MSGS,
                                 ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE};
#define RETEXCLUDE_NUMSUBOPTS (sizeof(RETEXCLUDE_SUBOPTS)/sizeof(RETEXCLUDE_SUBOPTS[0]))

// Test use of the ismENGINE_SUBSCRIPTION_OPTION_NO_RETAINED_MSGS flag
void test_capability_ExcludeRetained(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    ismEngine_ConsumerHandle_t hConsumer1,
                               hConsumer2;
    republishMessagesCbContext_t consumer1Context={0},
                                 consumer2Context={0};
    republishMessagesCbContext_t *consumer1Cb=&consumer1Context,
                                 *consumer2Cb=&consumer2Context;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("ExcludeRetainedClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...create\n");

    // Create a retained message
    void *payload1="ExcludedMsg";
    ismEngine_MessageHandle_t hMessage1;

    rc = test_createMessage(strlen(payload1)+1,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, RETEXCLUDE_TOPICS[0],
                            &hMessage1, &payload1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            RETEXCLUDE_TOPICS[0],
                                            NULL,
                                            hMessage1,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    printf("  ...work\n");

    consumer1Context.hSession = hSession;
    subAttrs.subOptions = RETEXCLUDE_SUBOPTS[0];

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   RETEXCLUDE_TOPICS[0],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer1Cb,
                                   sizeof(republishMessagesCbContext_t *),
                                   republishMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer1Context.received, 0);
    TEST_ASSERT_EQUAL(consumer1Context.retained, 0);

    subAttrs.subOptions = RETEXCLUDE_SUBOPTS[1];
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   RETEXCLUDE_TOPICS[1],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer2Cb,
                                   sizeof(republishMessagesCbContext_t *),
                                   republishMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer2Context.received, 1);
    TEST_ASSERT_EQUAL(consumer2Context.retained, 1);

    rc = test_createMessage(strlen(payload1)+1,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, RETEXCLUDE_TOPICS[0],
                            &hMessage1, &payload1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            RETEXCLUDE_TOPICS[0],
                                            NULL,
                                            hMessage1,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer1Context.received, 1);
    TEST_ASSERT_EQUAL(consumer1Context.retained, 0);
    TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, 1);
    TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, 1);
    TEST_ASSERT_EQUAL(consumer2Context.received, 2);
    TEST_ASSERT_EQUAL(consumer2Context.retained, 1);
    TEST_ASSERT_EQUAL(consumer2Context.published_for_retain, 2);
    TEST_ASSERT_EQUAL(consumer2Context.propagate_retained, 2);

    rc = ism_engine_republishRetainedMessages(hSession,
                                              hConsumer1,
                                              NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer1Context.received, 2);
    TEST_ASSERT_EQUAL(consumer1Context.retained, 1); // Republished as retained
    TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, 2);
    TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, 2);

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_RetainedMsgs_CUnit_test_Basic[] =
{
    { "BasicRetainedMessages", test_capability_BasicRetainedMessages },
    { "RetainedMatching", test_capability_RetainedMatching },
    { "RetainedRepublish", test_capability_RetainedRepublish },
    { "RetainedSelection", test_capability_RetainedSelection },
    { "SystemSeparation", test_capability_SystemSeparation },
    { "GetRetainedMessage", test_capability_GetRetainedMessage },
    { "UnsetUsingRegEx", test_capability_UnsetUsingRegEx },
    { "ExcludedRetainedMessages", test_capability_ExcludeRetained },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RetainedMsgs_CUnit_test_RetainedFlags_Phase1[] =
{
    { "RetainedFlags", test_capability_RetainedFlags_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RetainedMsgs_CUnit_test_RetainedFlags_Phase2[] =
{
    { "RetainedFlags", test_capability_RetainedFlags_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RetainedMsgs_CUnit_test_WillMsgRetained_Phase1[] =
{
    { "WillMsgRetained", test_capability_WillMsgRetained_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RetainedMsgs_CUnit_test_WillMsgRetained_Phase2[] =
{
    { "WillMsgRetained", test_capability_WillMsgRetained_Phase2 },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Basic test of retained message expiry by the reaper
//****************************************************************************
#define REAPER_EXPIRY_RETAINED_TOPIC_1 "TEST/REAPER_RETEXP/TOPIC1/Expirer"
#define REAPER_EXPIRY_RETAINED_TOPIC_2 "TEST/REAPER_RETEXP/TOPIC2"

void test_capability_ReaperExpiringRetained(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    doRealExpiry = true;

    // Reset stats for this test
    test_bounceEngine();
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    test_resetThreadStats(pThreadData, NULL);

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("ExpiryClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    /* Create messages to put */
    char *payLoad = "MESSAGEDATA";

    ismEngine_MessageHandle_t hMessage1;

    iemnMessagingStatistics_t internalMsgStats; // Purposefully not initializing

    iemeExpiryControl_t *expiryControl = (iemeExpiryControl_t *)ismEngine_serverGlobal.msgExpiryControl;

    // Ensure initial scan has finished
    uint64_t scansStarted = 1;
    while(scansStarted > expiryControl->scansEnded) { usleep(5000); }
    scansStarted++;

    // Put > ieutMAXLAZYMSGS retained messages on different topics so that
    // many will expire at once
    uint32_t expiryTime = ism_common_nowExpire()+1;
    int32_t initialRetainedCount;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;
    test_incrementActionsRemaining(pActionsRemaining, ieutMAXLAZYMSGS+3);
    for(initialRetainedCount=0; initialRetainedCount<ieutMAXLAZYMSGS+3; initialRetainedCount++)
    {
        char topic[strlen(REAPER_EXPIRY_RETAINED_TOPIC_1)+10];

        sprintf(topic, "%s/%d", REAPER_EXPIRY_RETAINED_TOPIC_1, initialRetainedCount);

        rc = test_createMessage(strlen(payLoad)+1,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                expiryTime,
                                ismDESTINATION_TOPIC, topic,
                                &hMessage1,
                                (void **)&payLoad);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topic,
                                                NULL,
                                                hMessage1,
                                                &pActionsRemaining,
                                                sizeof(pActionsRemaining),
                                                test_decrementActionsRemaining);
        if (rc != ISMRC_AsyncCompletion)
        {
            TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        }
    }

    test_waitForRemainingActions(pActionsRemaining);

    // Check the totals look right
    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet+
                      internalMsgStats.externalStats.ExpiredMessages, initialRetainedCount);

    // Force expiry reaper to run
    ieme_wakeMessageExpiryReaper(pThreadData); while(scansStarted > expiryControl->scansEnded) { usleep(5000); }

    // Check the totals look right
    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet+
                      internalMsgStats.externalStats.ExpiredMessages, initialRetainedCount);

    // Sleep for 2 seconds (message is set to expire in 1)
    int32_t sleepTime = 2;
    printf("  ...Sleeping for %d seconds\n", sleepTime);
    sleep(sleepTime);

    // Force expiry reaper to run - Should definitely be expired now
    scansStarted++;

    ieme_wakeMessageExpiryReaper(pThreadData); while(scansStarted > expiryControl->scansEnded) { usleep(5000); }

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.ExpiredMessages, initialRetainedCount);

    // Now put a persistent retained message with expiry on TOPIC_2
    rc = test_createMessage(strlen(payLoad)+1,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            ism_common_nowExpire()+2,
                            ismDESTINATION_TOPIC, REAPER_EXPIRY_RETAINED_TOPIC_2,
                            &hMessage1,
                            (void **)&payLoad);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 REAPER_EXPIRY_RETAINED_TOPIC_2,
                                                 NULL,
                                                 hMessage1);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, 1);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.ExpiredMessages, initialRetainedCount);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 1);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Make sure expiry gets reinstated
    test_bounceEngine();
    pThreadData = ieut_getThreadData();

    // We cannot know for sure whether the 1st scan has completed, but we do expect
    // there to be a single retained message with either expiry set, or to have been
    // expored
    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.ExpiredMessages +
                      internalMsgStats.RetainedMessagesWithExpirySet, 1);

    // Sleep for 3 second (message is set to expire in 2)
    sleepTime = 3;
    printf("  ...Sleeping for %d seconds\n", sleepTime);
    sleep(sleepTime);

    expiryControl = (iemeExpiryControl_t *)ismEngine_serverGlobal.msgExpiryControl;

    // Make sure that a scan has taken place
    scansStarted = 2;
    ieme_wakeMessageExpiryReaper(pThreadData); while(scansStarted > expiryControl->scansEnded) { usleep(5000); }

    // Should now have been another scan since restart which should expire a message
    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, 0);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.ExpiredMessages, 1);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);

    doRealExpiry = false;
}

//****************************************************************************
/// @brief Test a scenario where a node with 0 expiry gets left in expiry
///        reaper list and needs clearing up.
//****************************************************************************
#define REAPER_EXPIRY_ZERO_NODE_TOPIC "TEST/ZERO_NODE"
#define REAPER_EXPIRY_SUB_NODE_TOPIC  "TEST/ZERO_NODE/SUB_NODE"

void test_capability_ReapedZeroExpiryNode(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    doRealExpiry = true;

    // Reset stats for this test
    test_bounceEngine();
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    test_resetThreadStats(pThreadData, NULL);

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("ExpiryClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    /* Create messages to put */
    char *payLoad = "MESSAGEDATA";

    ismEngine_MessageHandle_t hMessage1, hMessage2;

    rc = test_createMessage(strlen(payLoad)+1,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, REAPER_EXPIRY_SUB_NODE_TOPIC,
                            &hMessage1,
                            (void **)&payLoad);
    TEST_ASSERT_EQUAL(rc, OK);

    // Expire 10 second from now
    rc = test_createMessage(strlen(payLoad)+1,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            ism_common_nowExpire()+10,
                            ismDESTINATION_TOPIC, REAPER_EXPIRY_ZERO_NODE_TOPIC,
                            &hMessage2,
                            (void **)&payLoad);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 REAPER_EXPIRY_SUB_NODE_TOPIC,
                                                 NULL,
                                                 hMessage1);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    iemnMessagingStatistics_t internalMsgStats; // Purposefully not initializing

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, 1);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.ExpiredMessages, 0);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);

    iemeExpiryControl_t *expiryControl = (iemeExpiryControl_t *)ismEngine_serverGlobal.msgExpiryControl;

    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 REAPER_EXPIRY_ZERO_NODE_TOPIC,
                                                 NULL,
                                                 hMessage2);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, 2);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.ExpiredMessages, 0);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 1);

    // Ensure initial scan has finished
    uint64_t scansStarted = 1;
    while(scansStarted > expiryControl->scansEnded) { usleep(5000); }

    // Force expiry reaper to run - but shouldn't have expired yet
    scansStarted++;
    ieme_wakeMessageExpiryReaper(pThreadData); while(scansStarted > expiryControl->scansEnded) { usleep(5000); }

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 1);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.ExpiredMessages, 0);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_TOPIC,
                                                      REAPER_EXPIRY_ZERO_NODE_TOPIC,
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == ISMRC_NoMatchingDestinations) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 1); // The unset one
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.ExpiredMessages, 0);

    // Force expiry reaper to run
    scansStarted++;
    ieme_wakeMessageExpiryReaper(pThreadData); while(scansStarted > expiryControl->scansEnded) { usleep(5000); }

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.ExpiredMessages, 0);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_TOPIC,
                                                      REAPER_EXPIRY_SUB_NODE_TOPIC,
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    test_waitForRemainingActions(pActionsRemaining);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);

    doRealExpiry = false;
}

//****************************************************************************
/// @brief Test a scenario where more than iettREAP_TOPIC_MESSAGE_BATCH_SIZE
///        need to be expired at once.
//****************************************************************************
#define REAPER_BATCHED_REAP_TOPIC  "TEST/BATCHED_REAP/SUB_NODE"
void test_capability_BatchedReap(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    // Reset stats for this test
    test_bounceEngine();
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    test_resetThreadStats(pThreadData, NULL);

    doRealExpiry = false;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("BigReapClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    /* Create messages to put */
    char *payLoad = "MESSAGEDATA";

    ismEngine_MessageHandle_t hMessage;
    uint32_t retainedCount = (iettREAP_TOPIC_MESSAGE_BATCH_SIZE*3)-10;
    uint32_t putCount = retainedCount;
    uint32_t *pPutCount = &putCount;

    for(int32_t i=0; i<retainedCount; i++)
    {
        char topicString[strlen(REAPER_BATCHED_REAP_TOPIC)+50];

        sprintf(topicString, REAPER_BATCHED_REAP_TOPIC "%d", i);

        // Expire now - 1
        rc = test_createMessage(strlen(payLoad)+1,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                ism_common_nowExpire()-1,
                                ismDESTINATION_TOPIC, topicString,
                                &hMessage,
                                (void **)&payLoad);
        TEST_ASSERT_EQUAL(rc, OK);

        // Publish message
        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                NULL,
                                                hMessage,
                                                &pPutCount, sizeof(pPutCount), test_countAsyncPut);
        if (rc != ISMRC_AsyncCompletion)
        {
            TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
            __sync_fetch_and_sub(pPutCount, 1);
        }
    }

    while(putCount > 0)
    {
        sched_yield();
    }

    iemnMessagingStatistics_t internalMsgStats;

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, retainedCount);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.ExpiredMessages, 0);

    doRealExpiry = true;

    iemeExpiryControl_t *expiryControl = (iemeExpiryControl_t *)ismEngine_serverGlobal.msgExpiryControl;

    // Ensure initial scan has finished
    uint64_t scansStarted = 1;
    while(scansStarted > expiryControl->scansEnded) { usleep(5000); }

    // Force expiry reaper to run - but shouldn't have expired yet
    scansStarted++;
    ieme_wakeMessageExpiryReaper(pThreadData); while(scansStarted > expiryControl->scansEnded) { usleep(5000); }

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.ExpiredMessages, retainedCount);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);

    doRealExpiry = false;
}

CU_TestInfo ISM_RetainedMsgs_CUnit_test_Expiry[] =
{
    { "ReaperExpiringRetained", test_capability_ReaperExpiringRetained },
    { "ReaperZeroExpiryNode", test_capability_ReapedZeroExpiryNode },
    { "BatchedReap", test_capability_BatchedReap },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the operation of persistent retained messages
//****************************************************************************
#define PERSISTENT_RETAINED_POLICYNAME "PersistentRetainedPolicy"

void test_capability_PersistentRetained(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
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

    // Prepare a topic string analysis for later use
    iettTopic_t  topic = {0};
    const char  *substrings[5];
    uint32_t     substringHashes[5];
    const char  *wildcards[5];
    const char  *multicards[5];

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    memset(&topic, 0, sizeof(topic));
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = 5;
    topic.topicString = "TEST/PERSISTENT/RETAINED";
    topic.destinationType = ismDESTINATION_TOPIC;

    rc = iett_analyseTopicString(pThreadData, &topic);
    TEST_ASSERT_EQUAL(rc, OK)

    printf("Starting %s...\n", __func__);

    int32_t totalRetainedPublished = 0;
    iettTopicTree_t *pTopicTree = NULL;
    iettTopicNode_t *pTopicNode = NULL;

    uint64_t expectedRefHandle = 0;

    // Bounce the server 10 times after publishing a random number of
    // retained messages, and randomly unsetting the retained message
    for(int32_t outerLoop=0; outerLoop<10; outerLoop++)
    {
        void *payload2=NULL;
        ismEngine_MessageHandle_t hMessage2;

        int32_t innerLoopIterations = rand()%20;

        pTopicTree = iett_getEngineTopicTree(pThreadData);
        TEST_ASSERT_PTR_NOT_NULL(pTopicTree);

        uint64_t expectedNextRetOrderId = pTopicTree->retNextOrderId;

        if (totalRetainedPublished > 0)
        {
            rc = iett_insertOrFindTopicsNode(pThreadData,
                                             pTopicTree->topics,
                                             &topic,
                                             iettOP_FIND,
                                             &pTopicNode);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        test_incrementActionsRemaining(pActionsRemaining, innerLoopIterations);
        for (int32_t innerLoop=0; innerLoop<innerLoopIterations; innerLoop++)
        {
            rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                    ismMESSAGE_PERSISTENCE_PERSISTENT,
                                    ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                    ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                    0,
                                    ismDESTINATION_TOPIC, "TEST/PERSISTENT/RETAINED",
                                    &hMessage2, &payload2);
            TEST_ASSERT_EQUAL(rc, OK);

#ifdef NDEBUG // Test non-standard use of ism_engine_putMessageInternalOnDestination
            if (rand()%2 == 0)
            {
                rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                                "TEST/PERSISTENT/RETAINED",
                                                                hMessage2,
                                                                &pActionsRemaining,
                                                                sizeof(pActionsRemaining),
                                                                test_decrementActionsRemaining);
            }
            else
#endif
            {
                rc = ism_engine_putMessageOnDestination(hSession,
                                                        ismDESTINATION_TOPIC,
                                                        "TEST/PERSISTENT/RETAINED",
                                                        NULL,
                                                        hMessage2,
                                                        &pActionsRemaining,
                                                        sizeof(pActionsRemaining),
                                                        test_decrementActionsRemaining);
            }

            if (rc != ISMRC_AsyncCompletion)
            {
                TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
                test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            }

            expectedNextRetOrderId++;
        }

        test_waitForRemainingActions(pActionsRemaining);

        totalRetainedPublished += innerLoopIterations;

        // Occassionally, and last time through, unset the retained
        if (outerLoop == 9 || rand()%10 > 8)
        {
            test_incrementActionsRemaining(pActionsRemaining, 1);
            rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                              ismDESTINATION_TOPIC,
                                                              "TEST/PERSISTENT/RETAINED",
                                                              ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                              ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                              NULL,
                                                              &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
            if (rc != ISMRC_AsyncCompletion)
            {
                TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
                test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            }

            test_waitForRemainingActions(pActionsRemaining);

            rc = iett_insertOrFindTopicsNode(pThreadData,
                                             pTopicTree->topics,
                                             &topic,
                                             iettOP_FIND,
                                             &pTopicNode);
            if (rc == OK)
            {
                TEST_ASSERT_PTR_NOT_NULL(pTopicNode->currRetMessage);
                TEST_ASSERT_EQUAL(pTopicNode->currRetMessage->Header.MessageType, MTYPE_NullRetained);
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
            }

            totalRetainedPublished = 0;
            expectedRefHandle = 0;
        }
        else
        {
            rc = iett_insertOrFindTopicsNode(pThreadData,
                                             pTopicTree->topics,
                                             &topic,
                                             iettOP_FIND,
                                             &pTopicNode);

            // Make sure the node exists if there have been any retained published, and
            // that the next retained OrderId is as we expect
            if (totalRetainedPublished > 0)
            {
                TEST_ASSERT_EQUAL(rc, OK);
                TEST_ASSERT_PTR_NOT_NULL(pTopicNode);
                TEST_ASSERT_EQUAL(pTopicTree->retNextOrderId, expectedNextRetOrderId);
                expectedRefHandle = pTopicNode->currRetRefHandle;
            }
            else
            {
                if (rc == OK)
                {
                    TEST_ASSERT_PTR_NOT_NULL(pTopicNode->currRetMessage);
                    TEST_ASSERT_EQUAL(pTopicNode->currRetMessage->Header.MessageType, MTYPE_NullRetained);
                }
                else
                {
                    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
                }

                expectedRefHandle = 0;
            }
        }

        rc = test_destroyClientAndSession(hClient, hSession, false);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = test_bounceEngine();
        TEST_ASSERT_EQUAL(rc, OK);

        pThreadData = ieut_getThreadData();

        pTopicTree = iett_getEngineTopicTree(pThreadData);
        TEST_ASSERT_PTR_NOT_NULL(pTopicTree);

        //
        if (totalRetainedPublished == 0)
        {
            rc = iett_insertOrFindTopicsNode(pThreadData,
                                             pTopicTree->topics,
                                             &topic,
                                             iettOP_FIND,
                                             &pTopicNode);
            if (rc == OK)
            {
                TEST_ASSERT_PTR_NOT_NULL(pTopicNode->currRetMessage);
                TEST_ASSERT_EQUAL(pTopicNode->currRetMessage->Header.MessageType, MTYPE_NullRetained);
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
            }
        }
        else
        {
            rc = iett_insertOrFindTopicsNode(pThreadData,
                                             pTopicTree->topics,
                                             &topic,
                                             iettOP_FIND,
                                             &pTopicNode);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(pTopicNode);
            TEST_ASSERT_PTR_NOT_NULL(pTopicNode->currRetMessage);
            TEST_ASSERT_EQUAL_FORMAT(pTopicNode->currRetRefHandle, expectedRefHandle, "%ld");;
            TEST_ASSERT(pTopicTree->retNextOrderId >= expectedNextRetOrderId, ("nextRetOrderId too low\n"));
        }

        rc = test_createClientAndSession(mockTransport->clientID,
                                         mockContext,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hClient, &hSession, false);
        TEST_ASSERT_EQUAL(rc, OK);

        free(payload2);
    }

    pTopicTree = iett_getEngineTopicTree(pThreadData);
    TEST_ASSERT_PTR_NOT_NULL(pTopicTree);

    // Always expect this topic node to have disappeared due to the unsetting of
    // the retained message on the last loop.
    rc = iett_insertOrFindTopicsNode(pThreadData,
                                     pTopicTree->topics,
                                     &topic,
                                     iettOP_FIND,
                                     &pTopicNode);

    if (rc == OK)
    {
        TEST_ASSERT_PTR_NOT_NULL(pTopicNode->currRetMessage);
        TEST_ASSERT_EQUAL(pTopicNode->currRetMessage->Header.MessageType, MTYPE_NullRetained);
    }
    else
    {
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    // Free any topic string analysis data
    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
        if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
        if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);
    }

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_RetainedMsgs_CUnit_test_Persistence[] =
{
    { "PersistentRetained", test_capability_PersistentRetained },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test a simple, non-complicated V1 retained with no message
//****************************************************************************
void test_capability_SimpleV1RetainedNoMsg(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    // Prepare a topic string analysis for later use
    iettTopic_t  topic = {0};
    const char  *substrings[5];
    uint32_t     substringHashes[5];
    const char  *wildcards[5];
    const char  *multicards[5];

    memset(&topic, 0, sizeof(topic));
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = 5;
    topic.topicString = "TEST/SIMPLE/V1/RETAINED/NOMSG";
    topic.destinationType = ismDESTINATION_TOPIC;

    rc = iett_analyseTopicString(pThreadData, &topic);
    TEST_ASSERT_EQUAL(rc, OK)

    printf("Starting %s...\n", __func__);

    for(int32_t outerloop=0; outerloop<2; outerloop++)
    {
        ismStore_Handle_t hV1TopicStoreHandle = ismSTORE_NULL_HANDLE;

        // Create a V1 store record - with nothing against it, prove that it gets handled
        rc = test_iest_storeV1Topic(pThreadData,
                                    pThreadData->hStream,
                                    topic.topicString,
                                    topic.topicStringLength,
                                    &hV1TopicStoreHandle);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_NOT_EQUAL(hV1TopicStoreHandle, ismSTORE_NULL_HANDLE);

        rc = ism_store_commit(pThreadData->hStream);
        TEST_ASSERT_EQUAL(rc, OK);

        if (outerloop == 1)
        {
            rc = ism_store_updateRecord(pThreadData->hStream,
                                        hV1TopicStoreHandle,
                                        ismSTORE_NULL_HANDLE,
                                        iestTDR_STATE_DELETED,
                                        ismSTORE_SET_STATE);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_store_commit(pThreadData->hStream);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        iettTopicTree_t *pTopicTree = NULL;
        iettTopicNode_t *pTopicNode = NULL;

        // Bounce the engine and check the existence of a node in the topic
        // tree indicating whether it was recovered from the store as expected
        for(int32_t loop=0; loop<2; loop++)
        {
            test_bounceEngine();

            pThreadData = ieut_getThreadData();

            pTopicTree = iett_getEngineTopicTree(pThreadData);
            TEST_ASSERT_PTR_NOT_NULL(pTopicTree);

            rc = iett_insertOrFindTopicsNode(pThreadData,
                                             pTopicTree->topics,
                                             &topic,
                                             iettOP_FIND,
                                             &pTopicNode);

            // Expect this topic node to appear because we read the V1 record in,
            // even though it had no associated message.
            if (outerloop == 0 && loop == 0)
            {
                TEST_ASSERT_EQUAL(rc, OK);
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
            }
        }
    }

    // Free any topic string analysis data
    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
        if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
        if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);
    }

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test a simple, non-complicated V1 retained
//****************************************************************************
void test_capability_SimpleV1Retained(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    // Prepare a topic string analysis for later use
    iettTopic_t  topic = {0};
    const char  *substrings[5];
    uint32_t     substringHashes[5];
    const char  *wildcards[5];
    const char  *multicards[5];

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    memset(&topic, 0, sizeof(topic));
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = 5;
    topic.topicString = "TEST/SIMPLE/V1/RETAINED";
    topic.destinationType = ismDESTINATION_TOPIC;

    rc = iett_analyseTopicString(pThreadData, &topic);
    TEST_ASSERT_EQUAL(rc, OK)

    printf("Starting %s...\n", __func__);

    ismStore_Handle_t hV1TopicStoreHandle = ismSTORE_NULL_HANDLE;

    // Create a V1 store record - with nothing against it, prove that it gets handled
    rc = test_iest_storeV1Topic(pThreadData,
                                pThreadData->hStream,
                                topic.topicString,
                                topic.topicStringLength,
                                &hV1TopicStoreHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hV1TopicStoreHandle, ismSTORE_NULL_HANDLE);

    rc = ism_store_commit(pThreadData->hStream);
    TEST_ASSERT_EQUAL(rc, OK);

    void *payload=NULL;
    ismEngine_MessageHandle_t hMessage;
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_NONE, // DON'T WANT UID and TIMESTAMP fields
                            0,
                            ismDESTINATION_TOPIC, topic.topicString,
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(hMessage->StoreMsg.parts.hStoreMsg, ismSTORE_NULL_HANDLE);

    // Make sure it looks retained
    hMessage->Header.Flags |= ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;

    ismStore_Reference_t msgRef = {0};

    msgRef.OrderId = 99; // Arbitrary orderid

    rc = iest_storeMessage(pThreadData,
                           hMessage,
                           1,
                           iestStoreMessageOptions_EXISTING_TRANSACTION,
                           &msgRef.hRefHandle);
    TEST_ASSERT_EQUAL(rc, OK);

    void *topicRefContext = NULL;
    rc = ism_store_openReferenceContext(hV1TopicStoreHandle, NULL, &topicRefContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(topicRefContext);

    ismStore_Handle_t storeRefHandle = ismSTORE_NULL_HANDLE;

    rc = iest_store_createReferenceCommit(pThreadData,
                                          pThreadData->hStream,
                                          topicRefContext,
                                          &msgRef,
                                          0,
                                          &storeRefHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(storeRefHandle, ismSTORE_NULL_HANDLE);

    iettTopicTree_t *pTopicTree = NULL;
    iettTopicNode_t *pTopicNode = NULL;

    // Bounce the engine and check the existence of a node in the topic
    // tree indicating whether it was recovered from the store as expected
    for(int32_t loop=0; loop<3; loop++)
    {
        test_bounceEngine();

        pThreadData = ieut_getThreadData();

        pTopicTree = iett_getEngineTopicTree(pThreadData);
        TEST_ASSERT_PTR_NOT_NULL(pTopicTree);

        rc = iett_insertOrFindTopicsNode(pThreadData,
                                         pTopicTree->topics,
                                         &topic,
                                         iettOP_FIND,
                                         &pTopicNode);

        if (loop < 2)
        {
            // Expect this topic node to appear because it has an associated message and
            // for the message to have a different store handle, but contain the same data.
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_NOT_EQUAL(pTopicNode->currRetMessage->StoreMsg.parts.hStoreMsg,
                                  hMessage->StoreMsg.parts.hStoreMsg);
            TEST_ASSERT_EQUAL(memcmp(pTopicNode->currRetMessage->pAreaData[1],
                                     payload,
                                     pTopicNode->currRetMessage->AreaLengths[1]), 0);
        }
        else
        {
            // We unset the retained message at the end of last loop
            if (rc == OK)
            {
                TEST_ASSERT_PTR_NOT_NULL(pTopicNode->currRetMessage);
                TEST_ASSERT_EQUAL(pTopicNode->currRetMessage->Header.MessageType, MTYPE_NullRetained);
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
            }
        }

        if (loop == 1)
        {
            ismEngine_ClientStateHandle_t hClient;
            ismEngine_SessionHandle_t hSession;

            rc = test_createClientAndSession("RemoveRetainedClient",
                                             NULL,
                                             ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                             ismENGINE_CREATE_SESSION_OPTION_NONE,
                                             &hClient, &hSession, false);
            TEST_ASSERT_EQUAL(rc, OK);

            test_incrementActionsRemaining(pActionsRemaining, 1);
            rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                              ismDESTINATION_TOPIC,
                                                              topic.topicString,
                                                              ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                              ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                              NULL,
                                                              &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
            if (rc != ISMRC_AsyncCompletion)
            {
                TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
                test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            }

            rc = test_destroyClientAndSession(hClient, hSession, true);
            TEST_ASSERT_EQUAL(rc, OK);

            test_waitForRemainingActions(pActionsRemaining);
        }
    }

    ism_engine_releaseMessage(hMessage);
    free(payload);

    // Free any topic string analysis data
    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
        if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
        if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);
    }

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test a transactional V1 retained
//****************************************************************************
void test_capability_TransactionalV1Retained(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    // Prepare a topic string analysis for later use
    iettTopic_t  topic = {0};
    const char  *substrings[5];
    uint32_t     substringHashes[5];
    const char  *wildcards[5];
    const char  *multicards[5];

    memset(&topic, 0, sizeof(topic));
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = 5;
    topic.topicString = "TEST/TRANSACTIONAL/V1/RETAINED";
    topic.destinationType = ismDESTINATION_TOPIC;

    rc = iett_analyseTopicString(pThreadData, &topic);
    TEST_ASSERT_EQUAL(rc, OK)

    printf("Starting %s...\n", __func__);

    for(int32_t outerloop=0; outerloop<2; outerloop++)
    {
        ismStore_Handle_t hV1TopicStoreHandle = ismSTORE_NULL_HANDLE;

        // Create a V1 store record
        rc = test_iest_storeV1Topic(pThreadData,
                                    pThreadData->hStream,
                                    topic.topicString,
                                    topic.topicStringLength,
                                    &hV1TopicStoreHandle);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_NOT_EQUAL(hV1TopicStoreHandle, ismSTORE_NULL_HANDLE);

        rc = ism_store_commit(pThreadData->hStream);
        TEST_ASSERT_EQUAL(rc, OK);

        // Create a transaction
        ismEngine_Transaction_t *pTran = NULL;

        rc = ietr_createLocal(pThreadData, NULL, true, false, NULL, &pTran);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(pTran);

        void *payload=NULL;
        ismEngine_MessageHandle_t hMessage;
        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_NONE, // Don't want UID and TIMESTAMP fields
                                0,
                                ismDESTINATION_TOPIC, topic.topicString,
                                &hMessage, &payload);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(hMessage->StoreMsg.parts.hStoreMsg, ismSTORE_NULL_HANDLE);

        // Make sure it looks retained
        hMessage->Header.Flags |= ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;

        ismStore_Reference_t msgRef = {0};

        msgRef.OrderId = 88; // Arbitrary orderid

        rc = iest_storeMessage(pThreadData,
                               hMessage,
                               1,
                               iestStoreMessageOptions_EXISTING_TRANSACTION,
                               &msgRef.hRefHandle);
        TEST_ASSERT_EQUAL(rc, OK);

        void *topicRefContext = NULL;
        rc = ism_store_openReferenceContext(hV1TopicStoreHandle, NULL, &topicRefContext);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(topicRefContext);

        ismStore_Handle_t storeRefHandle = ismSTORE_NULL_HANDLE;

        rc = ism_store_createReference(pThreadData->hStream,
                                       topicRefContext,
                                       &msgRef,
                                       0,
                                       &storeRefHandle);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_NOT_EQUAL(storeRefHandle, ismSTORE_NULL_HANDLE);

        rc = ism_store_commit(pThreadData->hStream);
        TEST_ASSERT_EQUAL(rc, OK);

        ietrStoreTranRef_t TranRef = {0};
        rc = ietr_createTranRef(pThreadData,
                                pTran,
                                storeRefHandle,
                                iestTOR_VALUE_PUT_MESSAGE,
                                0,
                                &TranRef);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_NOT_EQUAL(TranRef.hTranRef, ismSTORE_NULL_HANDLE);
        TEST_ASSERT_NOT_EQUAL(TranRef.orderId, 0);

        // 2nd time around, make it commit only
        if (outerloop == 1)
        {
            uint32_t now = ism_common_nowExpire();

            // And also update the record in the store
            rc = ism_store_updateRecord(pThreadData->hStream,
                                        pTran->hTran,
                                        0,
                                        ((uint64_t)now << 32) | ismTRANSACTION_STATE_COMMIT_ONLY,
                                        ismSTORE_SET_STATE);
        }

        rc = ism_store_commit(pThreadData->hStream);
        TEST_ASSERT_EQUAL(rc, OK);

        iettTopicTree_t *pTopicTree = NULL;
        iettTopicNode_t *pTopicNode = NULL;

        // Bounce the engine and check the existence of a node in the topic
        // tree indicating whether it was recovered from the store as expected
        for(int32_t loop=0; loop<4; loop++)
        {
            if (loop == 0)
            {
                // Clear up before bouncing the engine
                pTran->TranState = ismTRANSACTION_STATE_NONE;
                ietr_releaseTransaction(pThreadData, pTran);
            }

            test_bounceEngine();

            pThreadData = ieut_getThreadData();

            pTopicTree = iett_getEngineTopicTree(pThreadData);
            TEST_ASSERT_PTR_NOT_NULL(pTopicTree);

            rc = iett_insertOrFindTopicsNode(pThreadData,
                                             pTopicTree->topics,
                                             &topic,
                                             iettOP_FIND,
                                             &pTopicNode);

            // Should be rolled back
            if (outerloop == 0)
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
            }
            // Should be committed
            else if (outerloop == 1)
            {
                if (loop < 3)
                {
                    TEST_ASSERT_EQUAL(rc, OK);
                    TEST_ASSERT_NOT_EQUAL(pTopicNode->currRetMessage->StoreMsg.parts.hStoreMsg,
                                          hMessage->StoreMsg.parts.hStoreMsg);
                    TEST_ASSERT_EQUAL(memcmp(pTopicNode->currRetMessage->pAreaData[1],
                                             payload,
                                             pTopicNode->currRetMessage->AreaLengths[1]), 0);
                }
                else
                {
                    if (rc == OK)
                    {
                        TEST_ASSERT_PTR_NOT_NULL(pTopicNode->currRetMessage);
                        TEST_ASSERT_EQUAL(pTopicNode->currRetMessage->Header.MessageType, MTYPE_NullRetained);
                    }
                    else
                    {
                        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
                    }
                }

                if (loop == 2)
                {
                    ismEngine_ClientStateHandle_t hClient;
                    ismEngine_SessionHandle_t hSession;

                    rc = test_createClientAndSession("CleanupRetainedClient",
                                                     NULL,
                                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                     &hClient, &hSession, false);
                    TEST_ASSERT_EQUAL(rc, OK);

                    test_incrementActionsRemaining(pActionsRemaining, 1);
                    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                                      ismDESTINATION_TOPIC,
                                                                      topic.topicString,
                                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                                      NULL,
                                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
                    if (rc != ISMRC_AsyncCompletion)
                    {
                        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
                        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
                    }

                    test_waitForRemainingActions(pActionsRemaining);

                    rc = test_destroyClientAndSession(hClient, hSession, true);
                    TEST_ASSERT_EQUAL(rc, OK);
                }
            }
        }

        ism_engine_releaseMessage(hMessage);
        free(payload);
    }

    // Free any topic string analysis data
    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
        if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
        if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);
    }

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

// Note: Each migration test is done individually to ensure that no new store objects
//       have been created before they run (like a v2 topic record, for instance)
CU_TestInfo ISM_RetainedMsgs_CUnit_test_Migration_1[] =
{
    { "SimpleV1RetainedNoMsg", test_capability_SimpleV1RetainedNoMsg },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RetainedMsgs_CUnit_test_Migration_2[] =
{
   { "SimpleV1Retained", test_capability_SimpleV1Retained },
   CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RetainedMsgs_CUnit_test_Migration_3[] =
{
   { "TransactionalV1Retained", test_capability_TransactionalV1Retained },
   // TODO: Prepared XA transaction test
   CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test operation of retained messages using internal (local) transactions
//****************************************************************************
#define LOCALTXN_RETAINED_TOPIC_1 "Local Txn Retained/Topic/1"
#define LOCALTXN_RETAINED_TOPIC_2 "Local Txn Retained/Topic/1/Subtopic"

void test_capability_LocalTxn(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    iettTopicTree_t *tree = iett_getEngineTopicTree(pThreadData);

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    TEST_ASSERT_PTR_NOT_NULL(tree);

    rc = iett_ensureEngineStoreTopicRecord(pThreadData);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("LocalTxnClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...create\n");

    /* Create a message to put */
    ismEngine_MessageHandle_t hMessage1, hMessage2;

    printf("  ...work\n");

    // Prepare a topic string analysis for later use
    iettTopic_t  topic = {0};
    const char  *substrings[5];
    uint32_t     substringHashes[5];
    const char  *wildcards[5];
    const char  *multicards[5];

    memset(&topic, 0, sizeof(topic));
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = 5;
    topic.topicString = LOCALTXN_RETAINED_TOPIC_1;
    topic.destinationType = ismDESTINATION_TOPIC;

    rc = iett_analyseTopicString(pThreadData, &topic);

    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t          maxNodes = 0;
    uint32_t          nodeCount = 0;
    iettTopicNode_t **topicNodes = NULL;

    // Look for the topic node - should be none
    rc = iett_findMatchingTopicsNodes(pThreadData,
                                      tree->topics,
                                      false, &topic, 0, 0, 0,
                                      NULL, &maxNodes, &nodeCount, &topicNodes);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(nodeCount, 0);

    // Pseudo publish of the retained message (retaining control of the transaction)
    ismEngine_Transaction_t *pTran;

    rc = ietr_createLocal(pThreadData, (ismEngine_Session_t *)hSession, true, false, NULL, &pTran);
    TEST_ASSERT_EQUAL(rc, OK);

    // Just try some invalid operations on this local transaction
    rc = ietr_prepare(pThreadData, pTran, (ismEngine_Session_t *)hSession, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);

    // Pass an invalid pTran structure into a few transaction functions (we will expect FDCs)
    swallow_ffdcs = true;
    swallow_expect_core = false;

    pTran->StrucId[0] = '?';
    rc = ietr_prepare(pThreadData, pTran, (ismEngine_Session_t *)hSession, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ArgNotValid);
    rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, (ismEngine_Session_t *)hSession,
                     NULL, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ArgNotValid);
    rc = ietr_rollback(pThreadData, pTran, (ismEngine_Session_t *)hSession, IETR_ROLLBACK_OPTIONS_NONE);
    TEST_ASSERT_EQUAL(rc, ISMRC_ArgNotValid);

    pTran->StrucId[0] = ismENGINE_TRANSACTION_STRUCID[0];

    swallow_ffdcs = false;

    // Now on with the test
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, LOCALTXN_RETAINED_TOPIC_1,
                            &hMessage1, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 LOCALTXN_RETAINED_TOPIC_1,
                                                 pTran,
                                                 hMessage1);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    uint64_t expectCurrRetOrderId = 0;
    uint64_t expectNextRetOrderId = 0;
    ism_time_t expectCurrRetTimestamp = 0;

    nodeCount = 0;
    // Look for the topic node - should now exist, no retained
    rc = iett_findMatchingTopicsNodes(pThreadData,
                                      tree->topics,
                                      false, &topic, 0, 0, 0,
                                      NULL, &maxNodes, &nodeCount, &topicNodes);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(nodeCount, 1);
    TEST_ASSERT_EQUAL(topicNodes[0]->currRetOrderId, expectCurrRetOrderId);
    TEST_ASSERT_EQUAL(topicNodes[0]->currRetTimestamp, expectCurrRetTimestamp);
    TEST_ASSERT_EQUAL(tree->retNextOrderId, expectNextRetOrderId);

    // Roll back the transaction
    rc = ietr_rollback(pThreadData, pTran, (ismEngine_Session_t *)hSession, IETR_ROLLBACK_OPTIONS_NONE);

    TEST_ASSERT_EQUAL(rc, OK);

    nodeCount = 0;
    // Look for the topic node - should not exist
    rc = iett_findMatchingTopicsNodes(pThreadData,
                                      tree->topics,
                                      false, &topic, 0, 0, 0,
                                      NULL, &maxNodes, &nodeCount, &topicNodes);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(nodeCount, 0);
    TEST_ASSERT_EQUAL(tree->retNextOrderId, expectNextRetOrderId);

    // Do it again, with a persistent message
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, LOCALTXN_RETAINED_TOPIC_1,
                            &hMessage2, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    iem_recordMessageUsage(hMessage2); // Stop it disappearing until we can check things

    rc = ietr_createLocal(pThreadData, (ismEngine_Session_t *)hSession, true, false, NULL, &pTran);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 LOCALTXN_RETAINED_TOPIC_1,
                                                 pTran,
                                                 hMessage2);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
    expectNextRetOrderId++;

    TEST_ASSERT_NOT_EQUAL(hMessage2->StoreMsg.parts.hStoreMsg, ismSTORE_NULL_HANDLE);

    // Roll back the transaction
    rc = ietr_rollback(pThreadData, pTran, (ismEngine_Session_t *)hSession, IETR_ROLLBACK_OPTIONS_NONE);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(hMessage2->StoreMsg.parts.hStoreMsg, ismSTORE_NULL_HANDLE);

    ism_engine_releaseMessage(hMessage2);

    nodeCount = 0;
    // Look for the topic node - should not exist
    rc = iett_findMatchingTopicsNodes(pThreadData,
                                      tree->topics,
                                      false, &topic, 0, 0, 0,
                                      NULL, &maxNodes, &nodeCount, &topicNodes);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(nodeCount, 0);
    TEST_ASSERT_EQUAL(tree->retNextOrderId, expectNextRetOrderId);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_TOPIC,
                                                      LOCALTXN_RETAINED_TOPIC_1,
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    test_waitForRemainingActions(pActionsRemaining);

    expectNextRetOrderId += 1; // We'll publish an MTYPE_NullRetained
    TEST_ASSERT_EQUAL(tree->retNextOrderId, expectNextRetOrderId);

    nodeCount = 0;
    // Look for the topic node - should either not be there, or contain an MTYPE_NullRetained
    rc = iett_findMatchingTopicsNodes(pThreadData,
                                      tree->topics,
                                      false, &topic, 0, 0, 0,
                                      NULL, &maxNodes, &nodeCount, &topicNodes);
    TEST_ASSERT_EQUAL(rc, OK);
    if (nodeCount != 0)
    {
        TEST_ASSERT_EQUAL(nodeCount, 1);
        TEST_ASSERT_PTR_NOT_NULL(topicNodes[0]);
        TEST_ASSERT_PTR_NOT_NULL(topicNodes[0]->currRetMessage);
        TEST_ASSERT_EQUAL(topicNodes[0]->currRetMessage->Header.MessageType, MTYPE_NullRetained);
    }

    // Now commit a persistent retained
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, LOCALTXN_RETAINED_TOPIC_1,
                            &hMessage2, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    iem_recordMessageUsage(hMessage2); // Stop it disappearing until we can check things

    rc = ietr_createLocal(pThreadData, (ismEngine_Session_t *)hSession, true, false, NULL, &pTran);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 LOCALTXN_RETAINED_TOPIC_1,
                                                 pTran,
                                                 hMessage2);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
    expectNextRetOrderId++;
    expectCurrRetOrderId = expectNextRetOrderId;
    TEST_ASSERT_NOT_EQUAL(hMessage2->StoreMsg.parts.hStoreMsg, ismSTORE_NULL_HANDLE);

    nodeCount = 0;
    // Look for the topic node - should exist
    rc = iett_findMatchingTopicsNodes(pThreadData,
                                      tree->topics,
                                      false, &topic, 0, 0, 0,
                                      NULL, &maxNodes, &nodeCount, &topicNodes);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(nodeCount, 1);
    if (topicNodes[0]->currRetMessage == NULL)
    {
        TEST_ASSERT_EQUAL(topicNodes[0]->currRetOrderId, 0);
        TEST_ASSERT_EQUAL(topicNodes[0]->currRetTimestamp, expectCurrRetTimestamp);
    }
    else
    {
        TEST_ASSERT_EQUAL(topicNodes[0]->currRetMessage->Header.MessageType, MTYPE_NullRetained);
    }

    // Commit the transaction
    rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, (ismEngine_Session_t *)hSession
                    , NULL, NULL);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hMessage2->StoreMsg.parts.hStoreMsg, ismSTORE_NULL_HANDLE);

    ism_engine_releaseMessage(hMessage2);

    TEST_ASSERT_PTR_NOT_NULL(topicNodes[0]->currRetMessage);
    TEST_ASSERT_NOT_EQUAL(topicNodes[0]->currRetRefHandle, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_EQUAL(topicNodes[0]->currRetOrderId, expectCurrRetOrderId);
    TEST_ASSERT_EQUAL(tree->retNextOrderId, expectNextRetOrderId);
    TEST_ASSERT(topicNodes[0]->currRetTimestamp > expectCurrRetTimestamp,
                ("currRetTimestamp %lu < expectCurrRetTimestamp %lu", topicNodes[0]->currRetTimestamp, expectCurrRetTimestamp));
    expectCurrRetTimestamp = topicNodes[0]->currRetTimestamp;

    rc = ietr_createLocal(pThreadData, (ismEngine_Session_t *)hSession, true, false, NULL, &pTran);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create an in-flight retained message
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, LOCALTXN_RETAINED_TOPIC_1,
                            &hMessage1, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 LOCALTXN_RETAINED_TOPIC_1,
                                                 pTran,
                                                 hMessage1);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_TOPIC,
                                                      LOCALTXN_RETAINED_TOPIC_1,
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    test_waitForRemainingActions(pActionsRemaining);

    expectNextRetOrderId += 1; // We'll publish an MTYPE_NullRetained
    expectCurrRetOrderId = expectNextRetOrderId;

    if (topicNodes[0]->currRetMessage == NULL)
    {
        TEST_ASSERT_EQUAL(topicNodes[0]->currRetRefHandle, ismSTORE_NULL_HANDLE);
        TEST_ASSERT_EQUAL(topicNodes[0]->currRetOrderId, 0);
        TEST_ASSERT(topicNodes[0]->currRetTimestamp > expectCurrRetTimestamp,
                    ("currRetTimestamp %lu < expectCurrRetTimestamp %lu", topicNodes[0]->currRetTimestamp, expectCurrRetTimestamp));
        TEST_ASSERT_EQUAL(tree->retNextOrderId, expectNextRetOrderId);
    }
    else
    {
        TEST_ASSERT_EQUAL(topicNodes[0]->currRetMessage->Header.MessageType, MTYPE_NullRetained);
    }
    expectCurrRetTimestamp = topicNodes[0]->currRetTimestamp;

    ismEngine_Transaction_t *pTran2;

    rc = ietr_createLocal(pThreadData, (ismEngine_Session_t *)hSession, true, false, NULL, &pTran2);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now commit a persistent retained
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, LOCALTXN_RETAINED_TOPIC_1,
                            &hMessage2, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 LOCALTXN_RETAINED_TOPIC_1,
                                                 pTran2,
                                                 hMessage2);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
    expectNextRetOrderId++;

    // should have no effect, as is older.
    rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, (ismEngine_Session_t *)hSession
                    , NULL, NULL);

    TEST_ASSERT_EQUAL(rc, OK);
    if (topicNodes[0]->currRetMessage == NULL)
    {
        TEST_ASSERT_EQUAL(topicNodes[0]->currRetRefHandle, ismSTORE_NULL_HANDLE);
        TEST_ASSERT_EQUAL(topicNodes[0]->currRetOrderId, 0);
        TEST_ASSERT_EQUAL(topicNodes[0]->currRetTimestamp, expectCurrRetTimestamp);
    }
    else
    {
        TEST_ASSERT_EQUAL(topicNodes[0]->currRetMessage->Header.MessageType, MTYPE_NullRetained);
    }

    // should retain message
    rc = ietr_commit(pThreadData, pTran2, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, (ismEngine_Session_t *)hSession
                    , NULL, NULL);
    expectCurrRetOrderId++;

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(topicNodes[0]->currRetMessage);
    TEST_ASSERT_NOT_EQUAL(topicNodes[0]->currRetRefHandle, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_EQUAL(topicNodes[0]->currRetOrderId, expectCurrRetOrderId);
    TEST_ASSERT(topicNodes[0]->currRetTimestamp > expectCurrRetTimestamp,
                ("currRetTimestamp %lu < expectCurrRetTimestamp %lu", topicNodes[0]->currRetTimestamp, expectCurrRetTimestamp));
    expectCurrRetTimestamp = topicNodes[0]->currRetTimestamp;
    TEST_ASSERT_EQUAL(tree->retNextOrderId, expectNextRetOrderId);

    ismEngine_MessageHandle_t prevRetMessage = topicNodes[0]->currRetMessage;
    ismStore_Handle_t         prevRetRefHandle = topicNodes[0]->currRetRefHandle;

    // Create an in-flight retained message
    rc = ietr_createLocal(pThreadData, (ismEngine_Session_t *)hSession, true, false, NULL, &pTran);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, LOCALTXN_RETAINED_TOPIC_1,
                            &hMessage1, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 LOCALTXN_RETAINED_TOPIC_1,
                                                 pTran,
                                                 hMessage1);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    rc = ietr_rollback(pThreadData, pTran
                      , (ismEngine_Session_t *)hSession
                      , IETR_ROLLBACK_OPTIONS_NONE); // Nothing should change

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL_FORMAT(topicNodes[0]->currRetMessage, prevRetMessage, "%p");
    TEST_ASSERT_EQUAL(topicNodes[0]->currRetRefHandle, prevRetRefHandle);
    TEST_ASSERT_EQUAL(topicNodes[0]->currRetOrderId, expectCurrRetOrderId);
    TEST_ASSERT_EQUAL(topicNodes[0]->currRetTimestamp, expectCurrRetTimestamp);
    TEST_ASSERT_EQUAL(tree->retNextOrderId, expectNextRetOrderId);

    // Create a subnode to ensure that unsetting doesn't delete
    iettTopic_t  topic2 = {0};
    const char  *substrings2[10];
    uint32_t     substringHashes2[10];
    const char  *wildcards2[10];
    const char  *multicards2[10];

    memset(&topic2, 0, sizeof(topic2));
    topic2.substrings = substrings2;
    topic2.substringHashes = substringHashes2;
    topic2.wildcards = wildcards2;
    topic2.multicards = multicards2;
    topic2.initialArraySize = 10;
    topic2.topicString = LOCALTXN_RETAINED_TOPIC_2;
    topic2.destinationType = ismDESTINATION_TOPIC;

    rc = iett_analyseTopicString(pThreadData, &topic2);

    TEST_ASSERT_EQUAL(rc, OK);

    iettTopicNode_t *newNode = NULL;

    rc = iett_insertOrFindTopicsNode(pThreadData, tree->topics, &topic2, iettOP_ADD, &newNode);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(newNode);

    ismEngine_MessageHandle_t hMessage3;

    rc = test_createOriginMessage(0,
                                  ismMESSAGE_PERSISTENCE_PERSISTENT,
                                  ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                  ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                  0,
                                  ismDESTINATION_TOPIC, LOCALTXN_RETAINED_TOPIC_1,
                                  (char *)ism_common_getServerUID(),
                                  ism_engine_retainedServerTime(),
                                  MTYPE_NullRetained,
                                  &hMessage3, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_UnreleasedHandle_t hUnrel = NULL;
    rc = sync_ism_engine_unsetRetainedMessageWithDeliveryIdOnDestination(hSession,
                                                                         ismDESTINATION_TOPIC,
                                                                         LOCALTXN_RETAINED_TOPIC_1,
                                                                         ismENGINE_UNSET_RETAINED_OPTION_PUBLISH,
                                                                         NULL,
                                                                         hMessage3, 5, &hUnrel);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
    TEST_ASSERT_PTR_NOT_NULL(hUnrel);

    expectNextRetOrderId++;

    rc = ism_engine_removeUnreleasedDeliveryId(hSession, NULL, hUnrel, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(rc, OK);
    if (topicNodes[0]->currRetMessage == NULL)
    {
        TEST_ASSERT_EQUAL(topicNodes[0]->currRetRefHandle, ismSTORE_NULL_HANDLE);
        TEST_ASSERT_EQUAL(topicNodes[0]->currRetOrderId, 0);
        TEST_ASSERT(topicNodes[0]->currRetTimestamp > expectCurrRetTimestamp,
                    ("currRetTimestamp %lu < expectCurrRetTimestamp %lu", topicNodes[0]->currRetTimestamp, expectCurrRetTimestamp));
        TEST_ASSERT_EQUAL(tree->retNextOrderId, expectNextRetOrderId);
    }
    else
    {
        TEST_ASSERT_EQUAL(topicNodes[0]->currRetMessage->Header.MessageType, MTYPE_NullRetained);
    }
    expectCurrRetTimestamp = topicNodes[0]->currRetTimestamp;

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    if (NULL != topic.topicStringCopy)
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);
    if (NULL != topic2.topicStringCopy)
        iemem_free(pThreadData, iemem_topicAnalysis, topic2.topicStringCopy);
    if (topicNodes != NULL) iemem_free(pThreadData, iemem_topicsQuery, topicNodes);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test operation of retained messages using internal (local) transactions
/// that have some savepoint rollback occuring
//****************************************************************************
#define LOCALTXN_SP_ROLLBACK_TOPIC_1 "LocalTxn/SPRollback/Topic"

void test_capability_LocalTxn_SPRollback(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    rc = iett_ensureEngineStoreTopicRecord(pThreadData);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("LTxnSPRollbackClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a subscription which will cause publish to rollback because of RejectNew
    uint64_t useMaxMessageCount = 10;
    uint64_t origMaxMessageCount = iepi_getDefaultPolicyInfo(false)->maxMessageCount;
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = useMaxMessageCount;

    ismEngine_SubscriptionAttributes_t subAttrs = {ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                   ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE,
                                                   ismENGINE_NO_SUBID};

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_createSubscription(hClient,
                                       "LTxnSPSub",
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       LOCALTXN_SP_ROLLBACK_TOPIC_1,
                                       &subAttrs,
                                       NULL,
                                       &pActionsRemaining,
                                       sizeof(pActionsRemaining),
                                       test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    printf("  ...work\n");

    ismEngine_Transaction_t *pTran;

    rc = ietr_createLocal(pThreadData, (ismEngine_Session_t *)hSession, true, false, NULL, &pTran);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t i=0; i < useMaxMessageCount+2; i++)
    {
        /* Create a message to put */
        ismEngine_MessageHandle_t hMessage;

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, LOCALTXN_SP_ROLLBACK_TOPIC_1,
                                &hMessage, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                     ismDESTINATION_TOPIC,
                                                     LOCALTXN_SP_ROLLBACK_TOPIC_1,
                                                     pTran,
                                                     hMessage);
        if (i < useMaxMessageCount)
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_DestinationFull);
        }
    }

    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = LOCALTXN_SP_ROLLBACK_TOPIC_1;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

    rc = iett_analyseTopicString(pThreadData, &topic);
    TEST_ASSERT_EQUAL(rc, OK);

    iettTopicNode_t *topicNode = NULL;
    iettTopicTree_t *tree = iett_getEngineTopicTree(pThreadData);

    rc = iett_insertOrFindTopicsNode(pThreadData, tree->topics, &topic, iettOP_FIND, &topicNode);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(topicNode);

    // Check the number of inflight retained updates matches the number we expect
    iettSLEUpdateRetained_t *pInflight = topicNode->inflightRetUpdates;
    TEST_ASSERT_PTR_NOT_NULL(pInflight);

    uint32_t inflightCount = 0;
    while(pInflight)
    {
        inflightCount++;
        pInflight = pInflight->nextInflightRetUpdate;
    }
    TEST_ASSERT_EQUAL(inflightCount, useMaxMessageCount+2);

    // Try to commit the transaction (should roll back)
    rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, hSession, NULL, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_RolledBack);

    rc = iett_insertOrFindTopicsNode(pThreadData, tree->topics, &topic, iettOP_FIND, &topicNode);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_PTR_NULL(topicNode);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(pThreadData, ".*");
    TEST_ASSERT_EQUAL(rc, OK);

    iepi_getDefaultPolicyInfo(false)->maxMessageCount = origMaxMessageCount;
}

//****************************************************************************
/// @brief Test the delivery of retained messages to late subscribers
//****************************************************************************
typedef struct tag_latesubsMessagesCbContext_t
{
    ismEngine_SessionHandle_t hSession;
    int32_t received;
    int32_t msg_content;
    bool    retained;
    bool    published_for_retain;
    bool    propagate_retained;
} latesubsMessagesCbContext_t;

bool latesubsMessagesCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
    latesubsMessagesCbContext_t *context = *((latesubsMessagesCbContext_t **)pContext);

    context->received++;

    int32_t this_msg_content = strtod(pAreaData[1], NULL);

    if (this_msg_content > context->msg_content)
    {
        context->msg_content = this_msg_content;
    }
    else
    {
        // Ordering was wrong
        context->msg_content = -1;
    }

    context->retained = !!(pMsgDetails->Flags & ismMESSAGE_FLAGS_RETAINED);
    context->published_for_retain = !!(pMsgDetails->Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN);
    context->propagate_retained = !!(pMsgDetails->Flags & ismMESSAGE_FLAGS_PROPAGATE_RETAINED);

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

char *LATESUBS_TOPICS[] = {"Testing/Late Subscribers", "Testing/+", "Testing/#", "#", "Testing/Late Subscribers/#"};
#define LATESUBS_NUMTOPICS (sizeof(LATESUBS_TOPICS)/sizeof(LATESUBS_TOPICS[0]))

uint32_t LATESUBS_SUBOPTS[] = {ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                               ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE,
                               ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                               ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE};
#define LATESUBS_NUMSUBOPTS (sizeof(LATESUBS_SUBOPTS)/sizeof(LATESUBS_SUBOPTS[0]))

typedef struct
{
    ismEngine_ClientStateHandle_t hClient[2];
    ismEngine_SessionHandle_t hSession[2];
    ismEngine_ConsumerHandle_t *phConsumer[2];
    latesubsMessagesCbContext_t **ppConsumerCb[2];
} createConsumer_inLateWindowContext;

void *test_createConsumer_inLateWindow(void *args)
{
    createConsumer_inLateWindowContext *pContext = (createConsumer_inLateWindowContext *)args;

    ism_engine_threadInit(0);

    for(uint32_t i=0;i<sizeof(pContext->hClient)/sizeof(pContext->hClient[0]);i++)
    {
        ismEngine_SubscriptionAttributes_t subAttrs = {0};
        int32_t rc;

        if (i == 0)
        {
            subAttrs.subOptions = LATESUBS_SUBOPTS[(rand()%(LATESUBS_NUMSUBOPTS-1))+1] |
                                  ismENGINE_SUBSCRIPTION_OPTION_DURABLE;

            rc = sync_ism_engine_createSubscription(pContext->hClient[i],
                                                    "MAKE_MINE_A_DURABLE",
                                                    NULL,
                                                    ismDESTINATION_TOPIC,
                                                    LATESUBS_TOPICS[rand()%LATESUBS_NUMTOPICS],
                                                    &subAttrs,
                                                    NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_engine_createConsumer(pContext->hSession[i],
                                           ismDESTINATION_SUBSCRIPTION,
                                           "MAKE_MINE_A_DURABLE",
                                           NULL,
                                           NULL, // Unused for TOPIC
                                           pContext->ppConsumerCb[i],
                                           sizeof(latesubsMessagesCbContext_t *),
                                           latesubsMessagesCallback,
                                           NULL,
                                           test_getDefaultConsumerOptions(subAttrs.subOptions),
                                           pContext->phConsumer[i],
                                           NULL, 0, NULL);
        }
        else
        {
            subAttrs.subOptions =  LATESUBS_SUBOPTS[rand()%LATESUBS_NUMSUBOPTS];
            rc = ism_engine_createConsumer(pContext->hSession[i],
                                           ismDESTINATION_TOPIC,
                                           LATESUBS_TOPICS[rand()%LATESUBS_NUMTOPICS],
                                           &subAttrs,
                                           NULL, // Unused for TOPIC
                                           pContext->ppConsumerCb[i],
                                           sizeof(latesubsMessagesCbContext_t *),
                                           latesubsMessagesCallback,
                                           NULL,
                                           test_getDefaultConsumerOptions(subAttrs.subOptions),
                                           pContext->phConsumer[i],
                                           NULL, 0, NULL);
        }

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(*(pContext->phConsumer[i]));
    }

    ism_engine_threadTerm(1);

    return NULL;
}

int32_t test_requestCreateConsumer_inLateWindow(int32_t origRc, void *context)
{
    // Request creation in a separate thread so there is no transaction in progress.
    pthread_t creatingThreadId;
    int os_rc = test_task_startThread(&creatingThreadId,test_createConsumer_inLateWindow, context,"test_createConsumer_inLateWindow");
    TEST_ASSERT_EQUAL(os_rc, 0);

    os_rc = pthread_join(creatingThreadId, NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);

    return origRc;
}

void test_capability_LateSubscribers(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient[9];
    ismEngine_SessionHandle_t hSession[9];
    ismEngine_TransactionHandle_t hTran;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    ismEngine_ConsumerHandle_t hConsumer1,
                               hConsumer2,
                               hConsumer3,
                               hConsumer4,
                               hConsumer5,
                               hConsumer6,
                               hConsumer7,
                               hConsumer8,
                               hConsumerX;
    latesubsMessagesCbContext_t consumer1Context={0},
                                consumer2Context={0},
                                consumer3Context={0},
                                consumer4Context={0},
                                consumer5Context={0},
                                consumer6Context={0},
                                consumer7Context={0},
                                consumer8Context={0},
                                consumerXContext={0};
    latesubsMessagesCbContext_t *consumer1Cb=&consumer1Context,
                                *consumer2Cb=&consumer2Context,
                                *consumer3Cb=&consumer3Context,
                                *consumer4Cb=&consumer4Context,
                                *consumer5Cb=&consumer5Context,
                                *consumer6Cb=&consumer6Context,
                                *consumer7Cb=&consumer7Context,
                                *consumer8Cb=&consumer8Context,
                                *consumerXCb=&consumerXContext;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    /* Create retained messages to put */
    void *payload1="10", *payload2="20", *payload3="30";
    ismEngine_MessageHandle_t hMessage1, hMessage2, hMessage3;

    uint32_t loop;
    for(loop=0; loop < 6; loop++)
    {
        memset(&consumer1Context, 0, sizeof(latesubsMessagesCbContext_t));
        memset(&consumer2Context, 0, sizeof(latesubsMessagesCbContext_t));
        memset(&consumer3Context, 0, sizeof(latesubsMessagesCbContext_t));
        memset(&consumer4Context, 0, sizeof(latesubsMessagesCbContext_t));
        memset(&consumer5Context, 0, sizeof(latesubsMessagesCbContext_t));
        memset(&consumer6Context, 0, sizeof(latesubsMessagesCbContext_t));
        memset(&consumer7Context, 0, sizeof(latesubsMessagesCbContext_t));
        memset(&consumer8Context, 0, sizeof(latesubsMessagesCbContext_t));
        memset(&consumerXContext, 0, sizeof(latesubsMessagesCbContext_t));

        for(uint32_t i=0;i<sizeof(hClient)/sizeof(hClient[0]);i++)
        {
            char clientId[25];

            sprintf(clientId, "LateSubscribersClient%u", i+1);

            rc = test_createClientAndSession(clientId,
                                             NULL,
                                             (i == 7) ? ismENGINE_CREATE_CLIENT_OPTION_DURABLE :
                                                        ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                             ismENGINE_CREATE_SESSION_OPTION_NONE,
                                             &hClient[i], &hSession[i], false);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        uint32_t expiryTime;
        ism_time_t now = ism_common_currentTimeNanos();
        uint32_t expiryNow = ism_common_getExpire(now);

        switch(loop)
        {
            case 0:
                expiryTime = 0;
                consumer1Context.hSession = hSession[0];
                break;
            case 1:
                expiryTime = 0;
                consumer1Context.hSession = NULL;
                break;
            case 2:
                // 1 day in the future
                expiryTime = ism_common_getExpire(now + (24*60*60*1000000000UL)*(1));
                consumer1Context.hSession = hSession[0];
                break;
            case 3:
                // 1 day in the future
                expiryTime = ism_common_getExpire(now + (24*60*60*1000000000UL)*(1));
                consumer1Context.hSession = NULL;
                break;
            case 4:
                // 1 hour in the past
                expiryTime = ism_common_getExpire(now - 60*1000000000UL);
                consumer1Context.hSession = hSession[0];
                break;
            case 5:
                // 1 hour in the past
                expiryTime = ism_common_getExpire(now - 60*1000000000UL);
                consumer1Context.hSession = NULL;
                break;
            default:
                TEST_ASSERT_NOT_EQUAL(0, 0);
                break;
        }

        bool expiringMsgs = !((expiryTime == 0) || (expiryTime > expiryNow));

        subAttrs.subOptions = LATESUBS_SUBOPTS[rand()%LATESUBS_NUMSUBOPTS];

        if (consumer1Context.hSession != NULL)
        {
            rc = ism_engine_createConsumer(consumer1Context.hSession,
                                           ismDESTINATION_TOPIC,
                                           LATESUBS_TOPICS[rand()%LATESUBS_NUMTOPICS],
                                           &subAttrs,
                                           NULL, // Unused for TOPIC
                                           &consumer1Cb,
                                           sizeof(latesubsMessagesCbContext_t *),
                                           latesubsMessagesCallback,
                                           NULL,
                                           test_getDefaultConsumerOptions(subAttrs.subOptions),
                                           &hConsumer1,
                                           NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        TEST_ASSERT_EQUAL(consumer1Context.received, 0);
        TEST_ASSERT_EQUAL(consumer1Context.msg_content, 0);
        TEST_ASSERT_EQUAL(consumer1Context.retained, false);
        TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, false);
        TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, false);

        // Create a transaction
        rc = sync_ism_engine_createLocalTransaction(hSession[0],
                                                    &hTran);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_NOT_EQUAL(hTran, 0);

        rc = test_createMessage(3,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                expiryTime,
                                ismDESTINATION_TOPIC, LATESUBS_TOPICS[0],
                                &hMessage1, &payload1);
        TEST_ASSERT_EQUAL(rc, OK);

        // Set up the creation of consumers in the window between updating the retained
        // message and getting the subscriberList
        createConsumer_inLateWindowContext inLateWindowContext = {{0}};
        inLateWindowContext.hClient[0] = hClient[7];
        inLateWindowContext.hSession[0] = hSession[7];
        inLateWindowContext.phConsumer[0] = &hConsumer7;
        inLateWindowContext.ppConsumerCb[0] = &consumer7Cb;
        inLateWindowContext.hClient[1] = hClient[8];
        inLateWindowContext.hSession[1] = hSession[8];
        inLateWindowContext.phConsumer[1] = &hConsumer8;
        inLateWindowContext.ppConsumerCb[1] = &consumer8Cb;

        post_iett_updateRetainedMessage_callback = test_requestCreateConsumer_inLateWindow;
        post_iett_updateRetainedMessage_context = &inLateWindowContext;

        rc = sync_ism_engine_putMessageOnDestination(hSession[0],
                                                     ismDESTINATION_TOPIC,
                                                     LATESUBS_TOPICS[0],
                                                     hTran,
                                                     hMessage1);
        if (rc != OK) TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

        post_iett_updateRetainedMessage_callback = NULL;

        TEST_ASSERT_EQUAL(consumer1Context.received, 0);
        TEST_ASSERT_EQUAL(consumer1Context.msg_content, 0);
        TEST_ASSERT_EQUAL(consumer1Context.retained, false);
        TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, false);
        TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, false);

        // Create a new consumer while the retained publish is uncommitted
        consumer2Context.hSession = hSession[1];
        subAttrs.subOptions = LATESUBS_SUBOPTS[rand()%LATESUBS_NUMSUBOPTS];
        rc = ism_engine_createConsumer(hSession[1],
                                       ismDESTINATION_TOPIC,
                                       LATESUBS_TOPICS[rand()%LATESUBS_NUMTOPICS],
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &consumer2Cb,
                                       sizeof(latesubsMessagesCbContext_t *),
                                       latesubsMessagesCallback,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &hConsumer2,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        TEST_ASSERT_EQUAL(consumer2Context.received, 0);
        TEST_ASSERT_EQUAL(consumer2Context.msg_content, 0);
        TEST_ASSERT_EQUAL(consumer2Context.retained, false);
        TEST_ASSERT_EQUAL(consumer2Context.published_for_retain, false);
        TEST_ASSERT_EQUAL(consumer2Context.propagate_retained, false);

        rc = sync_ism_engine_commitTransaction(hSession[0], hTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
        TEST_ASSERT_EQUAL(rc, OK);

        // Let the messages start flowing
        for(uint32_t i=0;i<sizeof(hClient)/sizeof(hClient[0]);i++)
        {
            rc = ism_engine_startMessageDelivery(hSession[i],
                                                 ismENGINE_START_DELIVERY_OPTION_NONE,
                                                 NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        if (!expiringMsgs)
        {
            if (consumer1Context.hSession != NULL)
            {
                TEST_ASSERT_EQUAL(consumer1Context.received, 1);
                TEST_ASSERT_EQUAL(consumer1Context.msg_content, 10);
                TEST_ASSERT_EQUAL(consumer1Context.retained, false);
                TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, true);
                TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, true);
            }
            else
            {
                TEST_ASSERT_EQUAL(consumer1Context.received, 0);
                TEST_ASSERT_EQUAL(consumer1Context.msg_content, 0);
                TEST_ASSERT_EQUAL(consumer1Context.retained, false);
                TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, false);
                TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, false);
            }

            TEST_ASSERT_EQUAL(consumer2Context.received, 1);
            TEST_ASSERT_EQUAL(consumer2Context.msg_content, 10);
            TEST_ASSERT_EQUAL(consumer2Context.retained, false);
            TEST_ASSERT_EQUAL(consumer2Context.published_for_retain, true);
            TEST_ASSERT_EQUAL(consumer2Context.propagate_retained, true);

            // Consumer from window between iett_updateRetainedMessage and getting subscriber list
            TEST_ASSERT_EQUAL(consumer7Context.received, 1);
            TEST_ASSERT_EQUAL(consumer7Context.msg_content, 10);
            TEST_ASSERT_EQUAL(consumer7Context.retained, false);
            TEST_ASSERT_EQUAL(consumer7Context.published_for_retain, true);
            TEST_ASSERT_EQUAL(consumer7Context.propagate_retained, true);
            TEST_ASSERT_EQUAL(consumer8Context.received, 1);
            TEST_ASSERT_EQUAL(consumer8Context.msg_content, 10);
            TEST_ASSERT_EQUAL(consumer8Context.retained, false);
            TEST_ASSERT_EQUAL(consumer8Context.published_for_retain, true);
            TEST_ASSERT_EQUAL(consumer8Context.propagate_retained, true);
        }
        else
        {
            TEST_ASSERT_EQUAL(consumer1Context.received, 0);
            TEST_ASSERT_EQUAL(consumer2Context.received, 0);
            TEST_ASSERT_EQUAL(consumer7Context.received, 0);
            TEST_ASSERT_EQUAL(consumer8Context.received, 0);
        }

        // Create a new consumer with no in-flight retaineds
        consumer3Context.hSession = hSession[2];
        subAttrs.subOptions = LATESUBS_SUBOPTS[rand()%LATESUBS_NUMSUBOPTS];
        rc = ism_engine_createConsumer(hSession[2],
                                       ismDESTINATION_TOPIC,
                                       LATESUBS_TOPICS[rand()%LATESUBS_NUMTOPICS],
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &consumer3Cb,
                                       sizeof(latesubsMessagesCbContext_t *),
                                       latesubsMessagesCallback,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &hConsumer3,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        if (!expiringMsgs)
        {
            TEST_ASSERT_EQUAL(consumer3Context.received, 1);
            TEST_ASSERT_EQUAL(consumer3Context.msg_content, 10);
            TEST_ASSERT_EQUAL(consumer3Context.retained, true);
            TEST_ASSERT_EQUAL(consumer3Context.published_for_retain, true);
            TEST_ASSERT_EQUAL(consumer3Context.propagate_retained, true);
        }
        else
        {
            TEST_ASSERT_EQUAL(consumer3Context.received, 0);
        }

        // Create a new transaction
        hTran = 0;
        rc = sync_ism_engine_createLocalTransaction(hSession[0],
                                                &hTran);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_NOT_EQUAL(hTran, 0);

        rc = test_createMessage(3,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                expiryTime,
                                ismDESTINATION_TOPIC, LATESUBS_TOPICS[0],
                                &hMessage2, &payload2);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = sync_ism_engine_putMessageOnDestination(hSession[0],
                                                     ismDESTINATION_TOPIC,
                                                     LATESUBS_TOPICS[0],
                                                     hTran,
                                                     hMessage2);
        TEST_ASSERT_EQUAL(rc, OK);

        // Stop message delivery to enable us to check the retained message usage count
        for(uint32_t i=0;i<sizeof(hClient)/sizeof(hClient[0]);i++)
        {
            rc = ism_engine_stopMessageDelivery(hSession[i], NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        // Create a new consumer
        consumer4Context.hSession = hSession[3];
        subAttrs.subOptions = LATESUBS_SUBOPTS[rand()%LATESUBS_NUMSUBOPTS];
        rc = ism_engine_createConsumer(hSession[3],
                                       ismDESTINATION_TOPIC,
                                       LATESUBS_TOPICS[rand()%LATESUBS_NUMTOPICS],
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &consumer4Cb,
                                       sizeof(latesubsMessagesCbContext_t *),
                                       latesubsMessagesCallback,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &hConsumer4,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // Restart message delivery
        for(uint32_t i=0;i<sizeof(hClient)/sizeof(hClient[0]);i++)
        {
            rc = ism_engine_startMessageDelivery(hSession[i],
                                                 ismENGINE_START_DELIVERY_OPTION_NONE,
                                                 NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        if (!expiringMsgs)
        {
            if (consumer1Context.hSession != NULL)
            {
                TEST_ASSERT_EQUAL(consumer1Context.received, 1);
                TEST_ASSERT_EQUAL(consumer1Context.msg_content, 10);
                TEST_ASSERT_EQUAL(consumer1Context.retained, false);
                TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, true);
                TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, true);
            }
            else
            {
                TEST_ASSERT_EQUAL(consumer1Context.received, 0);
                TEST_ASSERT_EQUAL(consumer1Context.msg_content, 0);
                TEST_ASSERT_EQUAL(consumer1Context.retained, false);
                TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, false);
                TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, false);
            }

            TEST_ASSERT_EQUAL(consumer2Context.received, 1);
            TEST_ASSERT_EQUAL(consumer2Context.msg_content, 10);
            TEST_ASSERT_EQUAL(consumer2Context.retained, false);
            TEST_ASSERT_EQUAL(consumer2Context.published_for_retain, true);
            TEST_ASSERT_EQUAL(consumer2Context.propagate_retained, true);
            TEST_ASSERT_EQUAL(consumer3Context.received, 1);
            TEST_ASSERT_EQUAL(consumer3Context.msg_content, 10);
            TEST_ASSERT_EQUAL(consumer3Context.retained, true);
            TEST_ASSERT_EQUAL(consumer3Context.published_for_retain, true);
            TEST_ASSERT_EQUAL(consumer3Context.propagate_retained, true);
            TEST_ASSERT_EQUAL(consumer4Context.received, 1); // Receive existing retained
            TEST_ASSERT_EQUAL(consumer4Context.msg_content, 10);
            TEST_ASSERT_EQUAL(consumer4Context.retained, true);
            TEST_ASSERT_EQUAL(consumer4Context.published_for_retain, true);
            TEST_ASSERT_EQUAL(consumer4Context.propagate_retained, true);
        }
        else
        {
            TEST_ASSERT_EQUAL(consumer1Context.received, 0);
            TEST_ASSERT_EQUAL(consumer2Context.received, 0);
            TEST_ASSERT_EQUAL(consumer3Context.received, 0);
            TEST_ASSERT_EQUAL(consumer4Context.received, 0);
        }

        rc = sync_ism_engine_commitTransaction(hSession[0], hTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
        TEST_ASSERT_EQUAL(rc, OK);

        if (!expiringMsgs)
        {
            if (consumer1Context.hSession != NULL)
            {
                TEST_ASSERT_EQUAL(consumer1Context.received, 2);
                TEST_ASSERT_EQUAL(consumer1Context.msg_content, 20);
                TEST_ASSERT_EQUAL(consumer1Context.retained, false);
                TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, true);
                TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, true);
            }
            else
            {
                TEST_ASSERT_EQUAL(consumer1Context.received, 0);
                TEST_ASSERT_EQUAL(consumer1Context.msg_content, 0);
                TEST_ASSERT_EQUAL(consumer1Context.retained, false);
                TEST_ASSERT_EQUAL(consumer1Context.published_for_retain, false);
                TEST_ASSERT_EQUAL(consumer1Context.propagate_retained, false);
            }

            TEST_ASSERT_EQUAL(consumer2Context.received, 2);
            TEST_ASSERT_EQUAL(consumer2Context.msg_content, 20);
            TEST_ASSERT_EQUAL(consumer2Context.retained, false);
            TEST_ASSERT_EQUAL(consumer2Context.published_for_retain, true);
            TEST_ASSERT_EQUAL(consumer2Context.propagate_retained, true);
            TEST_ASSERT_EQUAL(consumer3Context.received, 2);
            TEST_ASSERT_EQUAL(consumer3Context.msg_content, 20);
            TEST_ASSERT_EQUAL(consumer3Context.retained, false);
            TEST_ASSERT_EQUAL(consumer3Context.published_for_retain, true);
            TEST_ASSERT_EQUAL(consumer3Context.propagate_retained, true);
            TEST_ASSERT_EQUAL(consumer4Context.received, 2);
            TEST_ASSERT_EQUAL(consumer4Context.msg_content, 20);
            TEST_ASSERT_EQUAL(consumer4Context.retained, false);
            TEST_ASSERT_EQUAL(consumer4Context.published_for_retain, true);
            TEST_ASSERT_EQUAL(consumer4Context.propagate_retained, true);
            TEST_ASSERT_EQUAL(consumer7Context.received, 2);
            TEST_ASSERT_EQUAL(consumer7Context.msg_content, 20);
            TEST_ASSERT_EQUAL(consumer7Context.retained, false);
            TEST_ASSERT_EQUAL(consumer7Context.published_for_retain, true);
            TEST_ASSERT_EQUAL(consumer7Context.propagate_retained, true);
            TEST_ASSERT_EQUAL(consumer8Context.received, 2);
            TEST_ASSERT_EQUAL(consumer8Context.msg_content, 20);
            TEST_ASSERT_EQUAL(consumer8Context.retained, false);
            TEST_ASSERT_EQUAL(consumer8Context.published_for_retain, true);
            TEST_ASSERT_EQUAL(consumer8Context.propagate_retained, true);
        }
        else
        {
            TEST_ASSERT_EQUAL(consumer1Context.received, 0);
            TEST_ASSERT_EQUAL(consumer2Context.received, 0);
            TEST_ASSERT_EQUAL(consumer3Context.received, 0);
            TEST_ASSERT_EQUAL(consumer4Context.received, 0);
            TEST_ASSERT_EQUAL(consumer7Context.received, 0);
            TEST_ASSERT_EQUAL(consumer8Context.received, 0);
        }

        // Create a new transaction
        hTran = 0;
        rc = sync_ism_engine_createLocalTransaction(hSession[0],
                                                    &hTran);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_NOT_EQUAL(hTran, 0);

        rc = test_createMessage(3,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                expiryTime,
                                ismDESTINATION_TOPIC, LATESUBS_TOPICS[0],
                                &hMessage3, &payload3);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = sync_ism_engine_putMessageOnDestination(hSession[0],
                                                     ismDESTINATION_TOPIC,
                                                     LATESUBS_TOPICS[0],
                                                     hTran,
                                                     hMessage3);
        TEST_ASSERT_EQUAL(rc, OK);

        // Create a new consumer
        consumer5Context.hSession = hSession[4];
        subAttrs.subOptions = LATESUBS_SUBOPTS[rand()%LATESUBS_NUMSUBOPTS];
        rc = ism_engine_createConsumer(hSession[4],
                                       ismDESTINATION_TOPIC,
                                       LATESUBS_TOPICS[rand()%LATESUBS_NUMTOPICS],
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &consumer5Cb,
                                       sizeof(latesubsMessagesCbContext_t *),
                                       latesubsMessagesCallback,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &hConsumer5,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        if (!expiringMsgs)
        {
            if (consumer1Context.hSession != NULL)
            {
                TEST_ASSERT_EQUAL(consumer1Context.received, 2);
            }
            else
            {
                TEST_ASSERT_EQUAL(consumer1Context.received, 0);
            }

            TEST_ASSERT_EQUAL(consumer2Context.received, 2);
            TEST_ASSERT_EQUAL(consumer3Context.received, 2);
            TEST_ASSERT_EQUAL(consumer4Context.received, 2);
            TEST_ASSERT_EQUAL(consumer7Context.received, 2);
            TEST_ASSERT_EQUAL(consumer8Context.received, 2);
            TEST_ASSERT_EQUAL(consumer5Context.received, 1);
            TEST_ASSERT_EQUAL(consumer5Context.msg_content, 20);
            TEST_ASSERT_EQUAL(consumer5Context.retained, true);
            TEST_ASSERT_EQUAL(consumer5Context.published_for_retain, true);
            TEST_ASSERT_EQUAL(consumer5Context.propagate_retained, true);
        }
        else
        {
            TEST_ASSERT_EQUAL(consumer1Context.received, 0);
            TEST_ASSERT_EQUAL(consumer2Context.received, 0);
            TEST_ASSERT_EQUAL(consumer3Context.received, 0);
            TEST_ASSERT_EQUAL(consumer4Context.received, 0);
            TEST_ASSERT_EQUAL(consumer7Context.received, 0);
            TEST_ASSERT_EQUAL(consumer8Context.received, 0);
            TEST_ASSERT_EQUAL(consumer5Context.received, 0);
        }

        rc = ism_engine_rollbackTransaction(hSession[0], hTran, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        if (!expiringMsgs)
        {
            if (consumer1Context.hSession != NULL)
            {
                TEST_ASSERT_EQUAL(consumer1Context.received, 2);
            }
            else
            {
                TEST_ASSERT_EQUAL(consumer1Context.received, 0);
            }

            TEST_ASSERT_EQUAL(consumer2Context.received, 2);
            TEST_ASSERT_EQUAL(consumer3Context.received, 2);
            TEST_ASSERT_EQUAL(consumer4Context.received, 2);
            TEST_ASSERT_EQUAL(consumer7Context.received, 2);
            TEST_ASSERT_EQUAL(consumer8Context.received, 2);
            TEST_ASSERT_EQUAL(consumer5Context.received, 1);
        }
        else
        {
            TEST_ASSERT_EQUAL(consumer1Context.received, 0);
            TEST_ASSERT_EQUAL(consumer2Context.received, 0);
            TEST_ASSERT_EQUAL(consumer3Context.received, 0);
            TEST_ASSERT_EQUAL(consumer4Context.received, 0);
            TEST_ASSERT_EQUAL(consumer7Context.received, 0);
            TEST_ASSERT_EQUAL(consumer8Context.received, 0);
            TEST_ASSERT_EQUAL(consumer5Context.received, 0);
        }

        // Create a new consumer
        consumer6Context.hSession = hSession[5];
        subAttrs.subOptions = LATESUBS_SUBOPTS[rand()%LATESUBS_NUMSUBOPTS];
        rc = ism_engine_createConsumer(hSession[5],
                                       ismDESTINATION_TOPIC,
                                       LATESUBS_TOPICS[rand()%LATESUBS_NUMTOPICS],
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &consumer6Cb,
                                       sizeof(latesubsMessagesCbContext_t *),
                                       latesubsMessagesCallback,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &hConsumer6,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        if (!expiringMsgs)
        {
            TEST_ASSERT_EQUAL(consumer6Context.received, 1);
            TEST_ASSERT_EQUAL(consumer6Context.msg_content, 20);
            TEST_ASSERT_EQUAL(consumer6Context.retained, true);
            TEST_ASSERT_EQUAL(consumer6Context.published_for_retain, true);
            TEST_ASSERT_EQUAL(consumer6Context.propagate_retained, true);
        }
        else
        {
            TEST_ASSERT_EQUAL(consumer6Context.received, 0);
        }

        /* Interesting stuff in here */

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_unsetRetainedMessageOnDestination(hSession[0],
                                                          ismDESTINATION_TOPIC,
                                                          LATESUBS_TOPICS[0],
                                                          ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                          ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                          NULL,
                                                          &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc != ISMRC_AsyncCompletion)
        {
            TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        }

        test_waitForRemainingActions(pActionsRemaining);

        // Ensure that a new subscriber now gets no retained message
        consumerXContext.hSession = hSession[6];
        subAttrs.subOptions = LATESUBS_SUBOPTS[rand()%LATESUBS_NUMSUBOPTS];
        rc = ism_engine_createConsumer(hSession[6],
                                       ismDESTINATION_TOPIC,
                                       LATESUBS_TOPICS[rand()%LATESUBS_NUMTOPICS],
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &consumerXCb,
                                       sizeof(latesubsMessagesCbContext_t *),
                                       latesubsMessagesCallback,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &hConsumerX,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        TEST_ASSERT_EQUAL(consumerXContext.received, 0);

        for(uint32_t i=0;i<sizeof(hClient)/sizeof(hClient[0]);i++)
        {
            if (i == 7)
            {
                rc = sync_ism_engine_destroyClientState(hClient[i], ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
            }
            else
            {
                rc = test_destroyClientAndSession(hClient[i], hSession[i], true);
            }
            TEST_ASSERT_EQUAL(rc, OK);
        }

        // Make sure we don't have any retained nodes in the topic tree
        rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

void test_createGlobalTransaction(ismEngine_SessionHandle_t hSession,
                                  uint64_t txnCounter,
                                  ismEngine_TransactionHandle_t *hTransaction)
{
    ism_xid_t XID = {0};
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;
    globalId.gtrid = txnCounter;
    globalId.bqual = txnCounter % 4;
    XID.formatID = 0;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    memcpy(&XID.data, &globalId, sizeof(globalId));

    int32_t rc = sync_ism_engine_createGlobalTransaction(hSession,
                                                    &XID,
                                                    ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT,
                                                    hTransaction);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test operation of retained messages when global transaction commits
//****************************************************************************
typedef struct tag_test_topicAndSubOpts_t
{
    char *topic;
    bool random;
    uint32_t subOptions;
}
test_topicAndSubOpts_t;

test_topicAndSubOpts_t GLOBALTXN_TOPICS[] = { {"Global Transaction/Topic/1", true, 0},
                                              {"Global Transaction/+/1", true, 0},
                                              {"Global Transaction/#", true, 0},
                                              {"Global Transaction/Topic/+", true, 0},
                                              {"Global Transaction/#/1", false, ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE},
                                              {"Global Transaction/Topic/1/#", true, 0},
                                              {"#", true, 0},
                                              {"+/+/+", true, 0},
                                              {"+/Topic/+", true, 0} };
#define GLOBALTXN_NUMTOPICS (sizeof(GLOBALTXN_TOPICS)/sizeof(GLOBALTXN_TOPICS[0]))

void test_capability_GlobalTxnCommit(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient, hClient2;
    ismEngine_SessionHandle_t     hSession, hSession2;
    ismEngine_TransactionHandle_t hTransaction, hTransaction2;
    int32_t topicNum;
    uint32_t subOptions;

    uint64_t txnCounter = ism_common_currentTimeNanos();

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("GTC_Client",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession("GTC_Client2",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient2,
                                     &hSession2,
                                     true);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...create\n");

    void *payload1=NULL, *payload2=NULL;
    size_t payload1Size=10, payload2Size=20;
    ismEngine_MessageHandle_t hMsgNonPersistent, hMsgPersistent;

    rc = test_createMessage(payload1Size,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, GLOBALTXN_TOPICS[0].topic,
                            &hMsgNonPersistent, &payload1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(payload2Size,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, GLOBALTXN_TOPICS[0].topic,
                            &hMsgPersistent, &payload2);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...work\n");

    // Create a transaction
    test_createGlobalTransaction(hSession, txnCounter++, &hTransaction);

    // Publish a retained message in the transaction
    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 GLOBALTXN_TOPICS[0].topic,
                                                 hTransaction,
                                                 hMsgNonPersistent);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    // Check there is no retained
    for(topicNum=0; topicNum<GLOBALTXN_NUMTOPICS; topicNum++)
    {
        subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                         CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                         GLOBALTXN_TOPICS[topicNum].subOptions;
        test_checkRetainedDelivery(hSession,
                                   GLOBALTXN_TOPICS[topicNum].topic,
                                   subOptions,
                                   0, 0, 0, 0, 0,
                                   NULL, 0);
    }

    // Try committing an unprepared transaction
    rc = ism_engine_commitTransaction(hSession, hTransaction, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);

    rc = ism_engine_endTransaction(hSession, hTransaction, ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Prepare the transaction - bit of a cheat to get the XID
    ism_xid_t *pXID = ((ismEngine_Transaction_t *)hTransaction)->pXID;

    // Note: Using a null session
    rc = sync_ism_engine_prepareGlobalTransaction(hSession, pXID);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check there is no retained
    for(topicNum=0; topicNum<GLOBALTXN_NUMTOPICS; topicNum++)
    {
        subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                         CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                         GLOBALTXN_TOPICS[topicNum].subOptions;
        test_checkRetainedDelivery(hSession,
                                   GLOBALTXN_TOPICS[topicNum].topic,
                                   subOptions,
                                   0, 0, 0, 0, 0,
                                   NULL, 0);
    }

    // Commit the transaction
    rc = sync_ism_engine_commitGlobalTransaction(hSession, pXID, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check there is a retained
    for(topicNum=0; topicNum<GLOBALTXN_NUMTOPICS; topicNum++)
    {
        subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                         CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                         GLOBALTXN_TOPICS[topicNum].subOptions;
        test_checkRetainedDelivery(hSession,
                                   GLOBALTXN_TOPICS[topicNum].topic,
                                   subOptions,
                                   1, 0, 1, 1, 1,
                                   payload1, payload1Size);
    }

    // Create a new transaction
    test_createGlobalTransaction(hSession, txnCounter++, &hTransaction);

    // Publish a new (persistent) retained message in the transaction
    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 GLOBALTXN_TOPICS[0].topic,
                                                 hTransaction,
                                                 hMsgPersistent);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    // Check the retained is not yet changed
    topicNum =  rand()%GLOBALTXN_NUMTOPICS;
    subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                     CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                     GLOBALTXN_TOPICS[topicNum].subOptions;

    test_checkRetainedDelivery(hSession,
                               GLOBALTXN_TOPICS[topicNum].topic,
                               subOptions,
                               1, 0, 1, 1, 1,
                               payload1, payload1Size);

    // Prepare the transaction
    pXID = ((ismEngine_Transaction_t *)hTransaction)->pXID;

    rc = sync_ism_engine_prepareGlobalTransaction(hSession, pXID);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the retained is not yet changed
    topicNum =  rand()%GLOBALTXN_NUMTOPICS;
    subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                     CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                     GLOBALTXN_TOPICS[topicNum].subOptions;

    test_checkRetainedDelivery(hSession,
                               GLOBALTXN_TOPICS[topicNum].topic,
                               subOptions,
                               1, 0, 1, 1, 1,
                               payload1, payload1Size);

    // Commit the transaction
    rc = sync_ism_engine_commitTransaction(hSession, hTransaction, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the retained has now changed (to be persistent)
    for(topicNum=0; topicNum<GLOBALTXN_NUMTOPICS; topicNum++)
    {
        subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                         CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                         GLOBALTXN_TOPICS[topicNum].subOptions;

        test_checkRetainedDelivery(hSession,
                                   GLOBALTXN_TOPICS[topicNum].topic,
                                   subOptions,
                                   1, 1, 1, 1, 1,
                                   payload2, payload2Size);
    }

    // Now try some interleaved transactions committing in order
    void *payload3=NULL, *payload4=NULL;
    size_t payload3Size=10, payload4Size=20;

    rc = test_createMessage(payload3Size,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, GLOBALTXN_TOPICS[0].topic,
                            &hMsgNonPersistent, &payload3);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(payload4Size,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, GLOBALTXN_TOPICS[0].topic,
                            &hMsgPersistent, &payload4);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a new transaction on two sessions
    test_createGlobalTransaction(hSession, txnCounter++, &hTransaction);
    test_createGlobalTransaction(hSession2, txnCounter++, &hTransaction2);

    // Publish a new retained message in the first transaction
    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 GLOBALTXN_TOPICS[0].topic,
                                                 hTransaction,
                                                 hMsgNonPersistent);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    // Check the retained is not yet changed
    topicNum =  rand()%GLOBALTXN_NUMTOPICS;
    subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                     CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                     GLOBALTXN_TOPICS[topicNum].subOptions;

    test_checkRetainedDelivery(hSession,
                               GLOBALTXN_TOPICS[topicNum].topic,
                               subOptions,
                               1, 1, 1, 1, 1,
                               payload2, payload2Size);

    // Publish a new retained message in the second transaction
    rc = sync_ism_engine_putMessageOnDestination(hSession2,
                                                 ismDESTINATION_TOPIC,
                                                 GLOBALTXN_TOPICS[0].topic,
                                                 hTransaction2,
                                                 hMsgPersistent);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    // Check the retained is not yet changed
    topicNum =  rand()%GLOBALTXN_NUMTOPICS;
    subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                     CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                     GLOBALTXN_TOPICS[topicNum].subOptions;

    test_checkRetainedDelivery(hSession,
                               GLOBALTXN_TOPICS[topicNum].topic,
                               subOptions,
                               1, 1, 1, 1, 1,
                               payload2, payload2Size);

    // Commit the first transaction
    pXID = ((ismEngine_Transaction_t *)hTransaction)->pXID;
    rc = sync_ism_engine_commitGlobalTransaction(hSession,
                                                 pXID,
                                                 ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMONEPHASE);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the retained has now changed
    for(topicNum=0; topicNum<GLOBALTXN_NUMTOPICS; topicNum++)
    {
        subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                         CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                         GLOBALTXN_TOPICS[topicNum].subOptions;

        test_checkRetainedDelivery(hSession,
                                   GLOBALTXN_TOPICS[topicNum].topic,
                                   subOptions,
                                   1, 0, 1, 1, 1,
                                   payload3, payload3Size);
    }

    // Commit the second transaction
    pXID = ((ismEngine_Transaction_t *)hTransaction2)->pXID;

    rc = sync_ism_engine_prepareGlobalTransaction(hSession2, pXID);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = sync_ism_engine_commitGlobalTransaction(hSession2,
                                            pXID,
                                            ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the retained has now changed again
    for(topicNum=0; topicNum<GLOBALTXN_NUMTOPICS; topicNum++)
    {
        subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                         CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                         GLOBALTXN_TOPICS[topicNum].subOptions;

        test_checkRetainedDelivery(hSession,
                                   GLOBALTXN_TOPICS[topicNum].topic,
                                   subOptions,
                                   1, 1, 1, 1, 1,
                                   payload4, payload4Size);
    }

    // Now try some interleaved transactions committing out of order

    // Create a new transaction on two sessions
    test_createGlobalTransaction(hSession2, txnCounter++, &hTransaction2);
    test_createGlobalTransaction(hSession, txnCounter++, &hTransaction);

    // Publish a new retained message in the first transaction
    rc = test_createMessage(payload2Size,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, GLOBALTXN_TOPICS[0].topic,
                            &hMsgPersistent, &payload2);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 GLOBALTXN_TOPICS[0].topic,
                                                 hTransaction,
                                                 hMsgPersistent);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    // Check the retained is not yet changed
    topicNum =  rand()%GLOBALTXN_NUMTOPICS;
    subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                     CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                     GLOBALTXN_TOPICS[topicNum].subOptions;

    test_checkRetainedDelivery(hSession,
                               GLOBALTXN_TOPICS[topicNum].topic,
                               subOptions,
                               1, 1, 1, 1, 1,
                               payload4, payload4Size);

    // Publish a new retained message in the second transaction
    rc = test_createMessage(payload1Size,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, GLOBALTXN_TOPICS[0].topic,
                            &hMsgNonPersistent, &payload1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession2,
                                                 ismDESTINATION_TOPIC,
                                                 GLOBALTXN_TOPICS[0].topic,
                                                 hTransaction2,
                                                 hMsgNonPersistent);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    // Check the retained is not yet changed
    topicNum =  rand()%GLOBALTXN_NUMTOPICS;
    subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                     CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                     GLOBALTXN_TOPICS[topicNum].subOptions;

    test_checkRetainedDelivery(hSession,
                               GLOBALTXN_TOPICS[topicNum].topic,
                               subOptions,
                               1, 1, 1, 1, 1,
                               payload4, payload4Size);

    // Commit the second transaction
    rc = sync_ism_engine_commitTransaction(hSession2,
                                      hTransaction2,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMONEPHASE);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the retained has now changed
    for(topicNum=0; topicNum<GLOBALTXN_NUMTOPICS; topicNum++)
    {
        subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                         CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                         GLOBALTXN_TOPICS[topicNum].subOptions;

        test_checkRetainedDelivery(hSession,
                                   GLOBALTXN_TOPICS[topicNum].topic,
                                   subOptions,
                                   1, 0, 1, 1, 1,
                                   payload1, payload1Size);
    }

    // Commit the first transaction
    rc = sync_ism_engine_commitTransaction(hSession,
                                      hTransaction,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMONEPHASE);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the retained has NOT changed
    for(topicNum=0; topicNum<GLOBALTXN_NUMTOPICS; topicNum++)
    {
        subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                         CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                         GLOBALTXN_TOPICS[topicNum].subOptions;

        test_checkRetainedDelivery(hSession,
                                   GLOBALTXN_TOPICS[topicNum].topic,
                                   subOptions,
                                   1, 0, 1, 1, 1,
                                   payload1, payload1Size);
    }

    printf("  ...disconnect\n");

    // Clean up
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_TOPIC,
                                                      GLOBALTXN_TOPICS[0].topic,
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient2, hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);

    if (payload1) free(payload1);
    if (payload2) free(payload2);
    if (payload3) free(payload3);
    if (payload4) free(payload4);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_GlobalTxnRollback(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    ismEngine_TransactionHandle_t hTransaction;
    int32_t topicNum;
    uint32_t subOptions;

    uint64_t txnCounter = ism_common_currentTimeNanos();

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("GTR_Client",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient,
                                     &hSession,
                                     true);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...create\n");

    void *payload1=NULL, *payload2=NULL;
    size_t payload1Size=10, payload2Size=20;
    ismEngine_MessageHandle_t hMsgNonPersistent, hMsgPersistent;

    rc = test_createMessage(payload1Size,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, GLOBALTXN_TOPICS[0].topic,
                            &hMsgNonPersistent, &payload1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(payload2Size,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, GLOBALTXN_TOPICS[0].topic,
                            &hMsgPersistent, &payload2);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...work\n");

    // Create a transaction
    test_createGlobalTransaction(hSession, txnCounter++, &hTransaction);

    // Publish a persistent retained message in the transaction
    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 GLOBALTXN_TOPICS[0].topic,
                                                 hTransaction,
                                                 hMsgPersistent);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    // Check there is no retained
    for(topicNum=0; topicNum<GLOBALTXN_NUMTOPICS; topicNum++)
    {
        subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                         CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                         GLOBALTXN_TOPICS[topicNum].subOptions;

        test_checkRetainedDelivery(hSession,
                                   GLOBALTXN_TOPICS[topicNum].topic,
                                   subOptions,
                                   0, 0, 0, 0, 0,
                                   NULL, 0);
    }

    // Prepare the transaction
    ism_xid_t *pXID = ((ismEngine_Transaction_t *)hTransaction)->pXID;
    rc = sync_ism_engine_prepareGlobalTransaction(hSession, pXID);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check there is no retained
    for(topicNum=0; topicNum<GLOBALTXN_NUMTOPICS; topicNum++)
    {
        subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                         CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                         GLOBALTXN_TOPICS[topicNum].subOptions;

        test_checkRetainedDelivery(hSession,
                                   GLOBALTXN_TOPICS[topicNum].topic,
                                   subOptions,
                                   0, 0, 0, 0, 0,
                                   NULL, 0);
    }

    // Rollback the transaction
    rc = ism_engine_rollbackGlobalTransaction(hSession, pXID, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check there is no retained
    for(topicNum=0; topicNum<GLOBALTXN_NUMTOPICS; topicNum++)
    {
        subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                         CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                         GLOBALTXN_TOPICS[topicNum].subOptions;

        test_checkRetainedDelivery(hSession,
                                   GLOBALTXN_TOPICS[topicNum].topic,
                                   subOptions,
                                   0, 0, 0, 0, 0,
                                   NULL, 0);
    }

    // Create a new transaction
    test_createGlobalTransaction(hSession, txnCounter++, &hTransaction);

    // Publish a non-persistent retained message in the transaction
    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 GLOBALTXN_TOPICS[0].topic,
                                                 hTransaction,
                                                 hMsgNonPersistent);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    // Check there is no retained
    topicNum =  rand()%GLOBALTXN_NUMTOPICS;
    subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                     CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                     GLOBALTXN_TOPICS[topicNum].subOptions;

    test_checkRetainedDelivery(hSession,
                               GLOBALTXN_TOPICS[topicNum].topic,
                               subOptions,
                               0, 0, 0, 0, 0,
                               NULL, 0);

    // Rollback the transaction (note: without a prepare)
    rc = ism_engine_rollbackTransaction(hSession, hTransaction, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check there is no retained
    for(topicNum=0; topicNum<GLOBALTXN_NUMTOPICS; topicNum++)
    {
        subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                         CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                         GLOBALTXN_TOPICS[topicNum].subOptions;

        test_checkRetainedDelivery(hSession,
                                   GLOBALTXN_TOPICS[topicNum].topic,
                                   subOptions,
                                   0, 0, 0, 0, 0,
                                   NULL, 0);
    }

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    if (payload1) free(payload1);
    if (payload2) free(payload2);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_GlobalTxnRestart1(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    ismEngine_TransactionHandle_t hTransaction;
    int32_t topicNum;
    uint32_t subOptions;

    uint64_t txnCounter = ism_common_currentTimeNanos();

    printf("Starting %s...\n", __func__);

    char *msgPayload[] = { "Message01", "Message02", "Message03", "Message04",
                           "Message05", "Message06", "Message07", "Message08",
                           "Message09", "Message10" };

    for(int32_t loop=0; loop<10; loop++)
    {
        bool prepare=true, commit1=false, commit2=false;
        uint8_t persistence;

        char *expectPayload;
        char *payload=msgPayload[loop];

        switch(loop)
        {
            case 0:
                persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
                expectPayload = NULL;
                break;
            case 1:
                persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
                commit2 = true;
                expectPayload = NULL;
                break;
            case 2:
                persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
                expectPayload = NULL;
                break;
            case 3:
                persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
                commit2 = true;
                expectPayload = payload;
                break;
            case 4:
                persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
                commit2 = true;
                expectPayload = msgPayload[3];
                break;
            case 5:
                persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
                expectPayload = msgPayload[3];
                break;
            case 6:
                persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
                expectPayload = msgPayload[3];
                break;
            case 7:
                persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
                commit1 = true;
                expectPayload = NULL;
                break;
            case 8:
                persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
                expectPayload = NULL;
                break;
            case 9:
                persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
                prepare = false;
                expectPayload = NULL;
                break;
        }

        rc = test_createClientAndSession("GTRestart_Client",
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                         ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                         &hClient,
                                         &hSession,
                                         true);
        TEST_ASSERT_EQUAL(rc, OK);

        size_t payloadSize=strlen(payload);
        ismEngine_MessageHandle_t hMsg;

        rc = test_createMessage(payloadSize,
                                persistence,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, GLOBALTXN_TOPICS[0].topic,
                                &hMsg, (void**)&payload);
        TEST_ASSERT_EQUAL(rc, OK);

        // Create a transaction
        test_createGlobalTransaction(hSession, txnCounter++, &hTransaction);

        // Publish message in the transaction
        rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                     ismDESTINATION_TOPIC,
                                                     GLOBALTXN_TOPICS[0].topic,
                                                     hTransaction,
                                                     hMsg);
        TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

        // Prepare the transaction
        if (prepare)
        {
            ism_xid_t *pXID = ((ismEngine_Transaction_t *)hTransaction)->pXID;

            rc = sync_ism_engine_prepareGlobalTransaction(hSession, pXID);
            TEST_ASSERT_EQUAL(rc, OK);

            // Commit before bouncing
            if (commit1)
            {
                rc = sync_ism_engine_commitGlobalTransaction(hSession,
                                                        pXID,
                                                        ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
                TEST_ASSERT_EQUAL(rc, OK);
            }
        }

        // *** BOUNCE ***
        test_bounceEngine();

        rc = test_createClientAndSession("GTRestart_Client",
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                         ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                         &hClient,
                                         &hSession,
                                         true);
        TEST_ASSERT_EQUAL(rc, OK);

        ism_xid_t *XIDBuffer = (ism_xid_t *)calloc(sizeof(ism_xid_t), 6);
        TEST_ASSERT_PTR_NOT_NULL(XIDBuffer);

        bool expectPreparedTxn = (prepare && !commit1);
        // Find the XID of our prepared transaction
        uint32_t xidCount = ism_engine_XARecover(hSession,
                                                 XIDBuffer,
                                                 1, 1,
                                                 ismENGINE_XARECOVER_OPTION_XA_TMSTARTRSCAN);

        // Create a transaction from the XID
        if (expectPreparedTxn)
        {
            TEST_ASSERT_EQUAL(xidCount,  1);

            // Either commit or roll back the transaction if didn't commit before restart
            if (commit2)
                rc = sync_ism_engine_commitGlobalTransaction(hSession,
                                                        XIDBuffer,
                                                        ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
            else
                rc = ism_engine_rollbackGlobalTransaction(hSession, XIDBuffer, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        // Don't expect to find a prepared transaction
        else
        {
            TEST_ASSERT_EQUAL(xidCount,  0);
        }

        free(XIDBuffer);

        // Check the state of retained messages
        for(topicNum=0; topicNum<GLOBALTXN_NUMTOPICS; topicNum++)
        {
            subOptions = GLOBALTXN_TOPICS[topicNum].random ?
                             CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] :
                             GLOBALTXN_TOPICS[topicNum].subOptions;

           if (expectPayload == NULL)
            {
                test_checkRetainedDelivery(hSession,
                                           GLOBALTXN_TOPICS[topicNum].topic,
                                           subOptions,
                                           0, 0, 0, 0, 0,
                                           NULL, 0);
            }
            else
            {
                test_checkRetainedDelivery(hSession,
                                           GLOBALTXN_TOPICS[topicNum].topic,
                                           subOptions,
                                           1, 1, 1, 1, 1,
                                           expectPayload, strlen(expectPayload));
            }
        }

        rc = test_destroyClientAndSession(hClient, hSession, false);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_TransactionalPropagate(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SessionHandle_t     hSession1, hSession2;
    ismEngine_TransactionHandle_t hTransaction1, hTransaction2;
    ismEngine_ConsumerHandle_t    hConsumer;
    checkDeliveryMessagesCbContext_t consumerContext = {0};
    checkDeliveryMessagesCbContext_t *consumerCb = &consumerContext;

    uint64_t txnCounter = ism_common_currentTimeNanos();

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("TxnlPropagateClient1",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient1, &hSession1, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession("TxnlPropagateClient2",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient2, &hSession2, true);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = { CHECKDELIVERY_SUBOPTS[rand()%CHECKDELIVERY_NUMSUBOPTS] };

    // Create a consumer
    consumerContext.hSession = hSession1;
    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_TOPIC,
                                   "Transactional/Propagate",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumerCb,
                                   sizeof(checkDeliveryMessagesCbContext_t *),
                                   checkDeliveryMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t i=0; i<2; i++) /* BEAM suppression: loop doesn't iterate */
    {
        for(int32_t j=0; j<2; j++)
        {
            void *payload=NULL;
            ismEngine_MessageHandle_t hMsg;
            ismEngine_TransactionHandle_t *phTran = (j == 0) ? &hTransaction1 : &hTransaction2;
            ismEngine_SessionHandle_t hSession = (j == 0) ? hSession1 : hSession2;

            rc = test_createMessage(10,
                                    ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                    ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                    ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                    0,
                                    ismDESTINATION_TOPIC, "Transactional/Propagate",
                                    &hMsg, &payload);
            TEST_ASSERT_EQUAL(rc, OK);

            // Create a transaction
            test_createGlobalTransaction(hSession, txnCounter++, phTran);

            // Publish message in the transaction
            rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                         ismDESTINATION_TOPIC,
                                                         "Transactional/Propagate",
                                                         *phTran,
                                                         hMsg);
            TEST_ASSERT_EQUAL(rc, OK);

            // Prepare the transaction
            ism_xid_t *pXID = ((ismEngine_Transaction_t *)*phTran)->pXID;

            rc = sync_ism_engine_prepareGlobalTransaction(hSession, pXID);
            TEST_ASSERT_EQUAL(rc, OK);

            free(payload);
        }

        // Commit msg1, then msg2
        if (i==0)
        {
            rc = sync_ism_engine_commitTransaction(hSession1,
                                              hTransaction1,
                                              ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_EQUAL(consumerContext.received, 1);
            TEST_ASSERT_EQUAL(consumerContext.nonpersistent, 1);
            TEST_ASSERT_EQUAL(consumerContext.propagate_retained, 1);
            TEST_ASSERT_EQUAL(consumerContext.published_for_retain, 1);
            TEST_ASSERT_EQUAL(consumerContext.retained, 0);
            rc = sync_ism_engine_commitTransaction(hSession2,
                                              hTransaction2,
                                              ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_EQUAL(consumerContext.received, 2);
            TEST_ASSERT_EQUAL(consumerContext.nonpersistent, 2);
            TEST_ASSERT_EQUAL(consumerContext.propagate_retained, 2);
            TEST_ASSERT_EQUAL(consumerContext.published_for_retain, 2);
            TEST_ASSERT_EQUAL(consumerContext.retained, 0);
        }
        // Commit msg2, then msg1
        else
        {
            rc = sync_ism_engine_commitTransaction(hSession2,
                                              hTransaction2,
                                              ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
            TEST_ASSERT_EQUAL(rc, OK);
            // IntermediateQ does not deliver past an uncommitted message, so don't expect
            // the message yet.
            if ((subAttrs.subOptions == ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE) ||
                (subAttrs.subOptions == ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE))
            {
                TEST_ASSERT_EQUAL(consumerContext.received, 3);
                TEST_ASSERT_EQUAL(consumerContext.nonpersistent, 3);
                TEST_ASSERT_EQUAL(consumerContext.propagate_retained, 3);
                TEST_ASSERT_EQUAL(consumerContext.published_for_retain, 3);
                TEST_ASSERT_EQUAL(consumerContext.retained, 0);
            }
            rc = sync_ism_engine_commitTransaction(hSession1,
                                              hTransaction1,
                                              ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_EQUAL(consumerContext.received, 4);
            TEST_ASSERT_EQUAL(consumerContext.nonpersistent, 4);
            TEST_ASSERT_EQUAL(consumerContext.propagate_retained, 3); // Should not propagate
            TEST_ASSERT_EQUAL(consumerContext.published_for_retain, 4);
            TEST_ASSERT_EQUAL(consumerContext.retained, 0);
        }
    }

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession1,
                                                      ismDESTINATION_TOPIC,
                                                      "Transactional/Propagate",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    test_waitForRemainingActions(pActionsRemaining);

    // Make sure we don't have any retained nodes in the topic tree for this topic
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), "Transactional/Propagate");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient1, hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_destroyClientAndSession(hClient2, hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

#define RMRCONFLICT_TOPIC "/RMRConflict/Topic"
void test_capability_RMRConflict(void)
{
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;
    iemnMessagingStatistics_t originalStats, nowStats;

    iemn_getMessagingStatistics(pThreadData, &originalStats);

    rc = iett_ensureEngineStoreTopicRecord(pThreadData);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_PTR_NOT_NULL(tree);
    TEST_ASSERT_PTR_NOT_NULL(tree->retRefContext);

    // Manufacture the situation with multiple retained messages on the same topic -
    // I'm doing it this way because actually doing it with publishes is quite tough!
    ism_time_t timeStamp[]  = { 20, 30, 50, 40, 60, 70 };
    uint32_t expiry[]       = { 30,  0, 30,  0, 30, 30 };
    char *originServerUID[] = { "A", "B", "C", "D", "E", "E" };

    int32_t msgCount = (int32_t)(sizeof(timeStamp)/sizeof(timeStamp[0]));
    ismStore_Handle_t msgStoreHandle[msgCount];
    ismStore_Handle_t refHandle[msgCount];
    for(int32_t i=0; i<msgCount; i++)
    {
        ismStore_Reference_t msgRef = {0};
        ismEngine_MessageHandle_t hMsg;

        rc = test_createOriginMessage(0,
                                      ismMESSAGE_PERSISTENCE_PERSISTENT,
                                      ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                      ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                      expiry[i] ? ism_common_nowExpire() + expiry[i] : 0,
                                      ismDESTINATION_TOPIC, RMRCONFLICT_TOPIC,
                                      (char *)originServerUID[i],
                                      timeStamp[i],
                                      MTYPE_MQTT_Binary,
                                      &hMsg, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        iest_store_reserve(pThreadData, iest_MessageStoreDataLength(hMsg), 1, 1);

        rc = iest_storeMessage(pThreadData,
                               hMsg,
                               2, // Additional reference for delivery to late subs
                               iestStoreMessageOptions_EXISTING_TRANSACTION |
                               iestStoreMessageOptions_ATOMIC_REFCOUNTING,
                               &msgStoreHandle[i]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_NOT_EQUAL(msgStoreHandle[i], ismSTORE_NULL_HANDLE);
        TEST_ASSERT_EQUAL(hMsg->StoreMsg.parts.hStoreMsg, msgStoreHandle[i]);

        msgRef.OrderId = ++(tree->retNextOrderId);
        msgRef.hRefHandle = msgStoreHandle[i];

        rc = ism_store_createReference(pThreadData->hStream,
                                       tree->retRefContext,
                                       &msgRef,
                                       tree->retMinActiveOrderId,
                                       &refHandle[i]);
        TEST_ASSERT_EQUAL(rc, OK);

        iest_store_commit(pThreadData, true);

        ism_engine_releaseMessage(hMsg);
    }

    iettTopic_t  topic = {0};
    const char  *substrings[5];
    uint32_t     substringHashes[5];

    memset(&topic, 0, sizeof(topic));
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.initialArraySize = 5;
    topic.topicString = RMRCONFLICT_TOPIC;
    topic.destinationType = ismDESTINATION_TOPIC;

    rc = iett_analyseTopicString(pThreadData, &topic);
    TEST_ASSERT_EQUAL(rc, OK)

    iettTopicNode_t *topicNode;
    for(int32_t i=0; i<3; i++)
    {
        test_bounceEngine();

        pThreadData = ieut_getThreadData();
        TEST_ASSERT_PTR_NOT_NULL(pThreadData);

        iemn_getMessagingStatistics(pThreadData, &nowStats);
        TEST_ASSERT_EQUAL(originalStats.externalStats.RetainedMessages+1, nowStats.externalStats.RetainedMessages);
        TEST_ASSERT_EQUAL(originalStats.InternalRetainedMessages+1, nowStats.InternalRetainedMessages);
        TEST_ASSERT_EQUAL(originalStats.RetainedMessagesWithExpirySet+1, nowStats.RetainedMessagesWithExpirySet);

        tree = ismEngine_serverGlobal.maintree;
        TEST_ASSERT_PTR_NOT_NULL(tree);

        rc = iett_insertOrFindTopicsNode(pThreadData, tree->topics, &topic, iettOP_FIND, &topicNode);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(topicNode);
        TEST_ASSERT_PTR_NOT_NULL(topicNode->currRetMessage);
        TEST_ASSERT_EQUAL(topicNode->currRetRefHandle, refHandle[msgCount-1]);
        TEST_ASSERT_EQUAL(topicNode->currRetMessage->StoreMsg.parts.hStoreMsg, msgStoreHandle[msgCount-1]);
        TEST_ASSERT_PTR_NOT_NULL(topicNode->currOriginServer);
        TEST_ASSERT_STRINGS_EQUAL(topicNode->currOriginServer->serverUID, originServerUID[msgCount-1]);
    }

    iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

    rc = iett_removeLocalRetainedMessages(pThreadData, RMRCONFLICT_TOPIC);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_RetainedMsgs_CUnit_test_Transactions[] =
{
    { "LocalTxn", test_capability_LocalTxn },
    { "GlobalTxnCommit", test_capability_GlobalTxnCommit },
    { "GlobalTxnRollback", test_capability_GlobalTxnRollback },
    { "GlobalTxnRestart1", test_capability_GlobalTxnRestart1 },
    // TODO: A mixture of global rollback / commit
    //       A restart with an outstanding (prepared) global
    //       Non-transactional publish mixed in with transactional
    { "TransactionalPropagate", test_capability_TransactionalPropagate },
    { "LateSubscribers", test_capability_LateSubscribers },
    { "RMRConflict", test_capability_RMRConflict },
    { "LocalTxn_SPRollback", test_capability_LocalTxn_SPRollback },
    CU_TEST_INFO_NULL
};

void test_scale(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    ismEngine_ConsumerHandle_t    hConsumer;
    checkDeliveryMessagesCbContext_t consumerContext = {0};
    checkDeliveryMessagesCbContext_t *consumerCb = &consumerContext;
    uint32_t totalRetained = fullTest ? 1000000 : 100000;
    uint32_t actualRetained; // The number we actually managed to retain
    ism_time_t startTime, endTime, diffTime;
    double diffTimeSecs;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("PubRetainedClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    iettTopicTree_t *pTopicTree = iett_getEngineTopicTree(pThreadData);

    uint64_t firstOrderId = 0;
    int32_t firstOrderIdTopic = 0;
    uint64_t lastOrderId = 0;
    int32_t lastOrderIdTopic = 0;

    startTime = ism_common_currentTimeNanos();

    int32_t persistentCount = 0;
    char topicString[50];
    int32_t i;
    for(i=0; i<totalRetained; i++) /* BEAM suppression: loop doesn't iterate */
    {
        sprintf(topicString, "RetTest/%08d/Test/Test", i);

        void *payload=NULL;
        ismEngine_MessageHandle_t hMsg;

        uint8_t persistence;

        if ((rand()%(totalRetained/10000)) == 0)
        {
            persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
        }
        else
        {
            persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        }

        rc = test_createMessage(10,
                                persistence,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, topicString,
                                &hMsg, &payload);

        if (rc == ISMRC_AllocateError) break;

        TEST_ASSERT_EQUAL(rc, OK);

        // Publish message
        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                NULL,
                                                hMsg,
                                                &pActionsRemaining,
                                                sizeof(pActionsRemaining),
                                                test_decrementActionsRemaining);
        if (rc == ISMRC_AllocateError)
        {
            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            break;
        }
        else if (rc == ISMRC_NoMatchingDestinations) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

        // Remember the orderIds of messages that we expect to be remembered as
        // active
        if (persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
        {
            lastOrderId = pTopicTree->retNextOrderId;
            lastOrderIdTopic = i;

            if (firstOrderId == 0)
            {
                firstOrderId = lastOrderId;
                firstOrderIdTopic = i;
            }
            persistentCount++;
        }

        // Every so often, wait for publishes to complete
        if (actionsRemaining > 1000)
        {
            test_waitForRemainingActions(pActionsRemaining);
        }

        free(payload);
    }
    endTime = ism_common_currentTimeNanos();

    // Wait for all the publishes to finish
    test_waitForRemainingActions(pActionsRemaining);

    if ((actualRetained = i) != totalRetained)
    {
        printf("Only %u of %u total could be put\n", actualRetained, totalRetained);
    }

    if (fullTest)
    {
        diffTime = endTime-startTime;
        diffTimeSecs = ((double)diffTime) / 1000000000;
        printf("Time to publish %d retained messages is %.2f secs. (%ldns)\n",
               i, diffTimeSecs, diffTime);
    }

    // Force the timer task to update retained messages
    TEST_ASSERT_EQUAL(pTopicTree->retMinActiveOrderId, 0);

    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.ActiveTimerUseCount, 0);
    ismEngine_serverGlobal.ActiveTimerUseCount = 1;
    int runagain = ietm_updateRetMinActiveOrderId(NULL, 0, NULL);
    ismEngine_serverGlobal.ActiveTimerUseCount = 0;
    TEST_ASSERT_EQUAL(runagain, 1);
    if (firstOrderId != 0)
    {
        // During a full test, we'll have done a reposition
        if (fullTest)
        {
            firstOrderId = pTopicTree->retMinActiveOrderId;
        }
        else
        {
            TEST_ASSERT_EQUAL(pTopicTree->retMinActiveOrderId, firstOrderId);
        }
    }

    // Set default maxMessageCount to 20000000 for the duration of the test
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 20000000;

    char *testTopic[] = {"#", "#/Test/#", "#/Test", "RetTest/+/+/Test", "RetTest/#/#", "RetTest/#", "#/+", "#/+/#", "+/+/#", NULL};
    uint32_t subOptions[] = {ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                             ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                             ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                             ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE,
                             ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE,
                             0};
    int32_t testIndex = 0;
    TEST_ASSERT_EQUAL(sizeof(testTopic)/sizeof(testTopic[0]), sizeof(subOptions)/sizeof(subOptions[0]));

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ieutHashSet_t *duplicateTest;
    rc = ieut_createHashSet(pThreadData, iemem_topicAnalysis, &duplicateTest);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    while(testTopic[testIndex] != NULL)
    {
        consumerCb->retained = consumerCb->received = 0;

        // Create a consumer
        consumerContext.hSession = hSession;
        subAttrs.subOptions = subOptions[testIndex];
        startTime = ism_common_currentTimeNanos();
        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       testTopic[testIndex],
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &consumerCb,
                                       sizeof(checkDeliveryMessagesCbContext_t *),
                                       checkDeliveryMessagesCallback,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &hConsumer,
                                       NULL, 0, NULL);
        endTime = ism_common_currentTimeNanos();
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(consumerCb->received, actualRetained);
        TEST_ASSERT_EQUAL(consumerCb->retained, actualRetained);

        if (fullTest)
        {
            diffTime = endTime-startTime;
            diffTimeSecs = ((double)diffTime) / 1000000000;
            printf("Time to subscribe on \"%s\" is %.2f secs. (%ldns)\n",
                    testTopic[testIndex], diffTimeSecs, diffTime);
        }

        rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check that there are no duplicates in a matching topics node
        uint32_t maxNodes = 0;
        uint32_t nodeCount = 0;
        iettTopicNode_t **topicNodes = NULL;

        iettTopic_t topic = {0};
        const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
        uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
        const char *wildcards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
        const char *multicards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];

        topic.destinationType = ismDESTINATION_TOPIC;
        topic.topicString = testTopic[testIndex];
        topic.substrings = substrings;
        topic.substringHashes = substringHashes;
        topic.wildcards = wildcards;
        topic.multicards = multicards;
        topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

        rc = iett_analyseTopicString(pThreadData, &topic);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = iett_findMatchingTopicsNodes(pThreadData,
                                          tree->topics, false,
                                          &topic,
                                          0, 0, 0,
                                          NULL, &maxNodes, &nodeCount, &topicNodes);
        TEST_ASSERT_EQUAL(rc, OK);

        if (NULL != topic.topicStringCopy)
        {
            iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

            if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
            if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
            if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
            if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);
        }

        if (fullTest)
        {
            printf("Checking for duplicates in %u list entries for \"%s\"\n", nodeCount, testTopic[testIndex]);
        }

        // Add each node to a hash table, expecting no return codes of ISMRC_ExistingKey
        for(i=0; i<nodeCount; i++)
        {
            rc = ieut_addValueToHashSet(pThreadData, duplicateTest, (uint64_t)(topicNodes[i]));
            TEST_ASSERT_EQUAL(rc, OK);
        }

        TEST_ASSERT_EQUAL(nodeCount, duplicateTest->totalCount);

        ieut_clearHashSet(pThreadData, duplicateTest);

        iemem_free(pThreadData, iemem_topicsQuery, topicNodes);

        testIndex++;
    }

    ieut_destroyHashSet(pThreadData, duplicateTest);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    if (firstOrderId != 0)
    {
        sprintf(topicString, "RetTest/%08d/Test/Test", firstOrderIdTopic);
        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_unsetRetainedMessageOnDestination(NULL,
                                                          ismDESTINATION_TOPIC,
                                                          topicString,
                                                          ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                          ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                          NULL,
                                                          &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc != ISMRC_AsyncCompletion)
        {
            TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        }

        test_waitForRemainingActions(pActionsRemaining);

        ismEngine_serverGlobal.ActiveTimerUseCount = 1;
        runagain = ietm_updateRetMinActiveOrderId(NULL, 0, NULL);
        ismEngine_serverGlobal.ActiveTimerUseCount = 0;
        TEST_ASSERT_EQUAL(runagain, 1);

        // First should now be different
        TEST_ASSERT_NOT_EQUAL(pTopicTree->retMinActiveOrderId, firstOrderId);
        firstOrderId = pTopicTree->retMinActiveOrderId;

        sprintf(topicString, "RetTest/%08d/Test/Test", lastOrderIdTopic);
        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_unsetRetainedMessageOnDestination(NULL,
                                                          ismDESTINATION_TOPIC,
                                                          topicString,
                                                          ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                          ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                          NULL,
                                                          &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc != ISMRC_AsyncCompletion)
        {
            TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        }

        test_waitForRemainingActions(pActionsRemaining);

        test_bounceEngine();

        pThreadData = ieut_getThreadData();
        pTopicTree = iett_getEngineTopicTree(pThreadData);

        // We told the store a min active orderId
        TEST_ASSERT_EQUAL(pTopicTree->retMinActiveOrderId, firstOrderId-1);

        ismEngine_serverGlobal.ActiveTimerUseCount = 1;
        runagain = ietm_updateRetMinActiveOrderId(NULL, 0, NULL);
        ismEngine_serverGlobal.ActiveTimerUseCount = 0;
        TEST_ASSERT_EQUAL(runagain, 1);

        // During a full test, we'll have done a reposition
        if (fullTest)
        {
            firstOrderId = pTopicTree->retMinActiveOrderId;
        }
        // We now know the score
        else
        {
            TEST_ASSERT_EQUAL(pTopicTree->retMinActiveOrderId, firstOrderId);
        }

        // Run a scan which would reposition retained messages (but we chose to disable that)
        TEST_ASSERT_EQUAL(ismEngine_serverGlobal.retainedRepositioningEnabled, true);
        ismEngine_serverGlobal.retainedRepositioningEnabled = false;
        bool repositionInitiated = iett_scanForRetMinActiveOrderId(pThreadData, 0, true);
        TEST_ASSERT_EQUAL(repositionInitiated, false);

        // Run a scan which could also reposition retained messages (now that we've reenabled that)
        ismEngine_serverGlobal.retainedRepositioningEnabled = true;
        repositionInitiated = iett_scanForRetMinActiveOrderId(pThreadData, 0, true);
        TEST_ASSERT_EQUAL(repositionInitiated, false);

        uint64_t originalRetMinActiveOrderId = pTopicTree->retMinActiveOrderId;
        ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_0] = StatusWarning;
        // Run a scan that will not perform repositioning, even though memory status is raised.
        repositionInitiated = iett_scanForRetMinActiveOrderId(pThreadData, 0, false);
        TEST_ASSERT_EQUAL(repositionInitiated, false);
        TEST_ASSERT_EQUAL(ismEngine_serverGlobal.RetMinActiveOrderIdMaxScans,1);
        repositionInitiated = iett_scanForRetMinActiveOrderId(pThreadData, 0, true);
        TEST_ASSERT_EQUAL(repositionInitiated, true);
        uint32_t retries = 0;
        do
        {
            if (pTopicTree->retMinActiveOrderId == originalRetMinActiveOrderId)
            {
                usleep(500);
                retries++;
            }
            else
            {
                break;
            }
        }
        while(retries < 6000);
        TEST_ASSERT(pTopicTree->retMinActiveOrderId != originalRetMinActiveOrderId,
                    ("retMinActiveOrderId == %lu originalRetMinActiveOrderId == %lu",
                     pTopicTree->retMinActiveOrderId, originalRetMinActiveOrderId));

        // Change to let it scan as much as it wants
        ismEngine_serverGlobal.RetMinActiveOrderIdMaxScans = 10;
        originalRetMinActiveOrderId = pTopicTree->retMinActiveOrderId;
        repositionInitiated = iett_scanForRetMinActiveOrderId(pThreadData, 0, true);
        TEST_ASSERT_EQUAL(repositionInitiated, true);

        ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_0] = StatusOk;

        // Give up to 30 seconds for update...
        do
        {
            if (pTopicTree->retMinActiveOrderId == originalRetMinActiveOrderId)
            {
                usleep(500);
                retries++;
            }
            else
            {
                break;
            }
        }
        while(retries < 6000);

        TEST_ASSERT(pTopicTree->retMinActiveOrderId != originalRetMinActiveOrderId,
                    ("retMinActiveOrderId == %lu originalRetMinActiveOrderId == %lu",
                     pTopicTree->retMinActiveOrderId, originalRetMinActiveOrderId));

        ismEngine_serverGlobal.RetMinActiveOrderIdMaxScans = ismENGINE_DEFAULT_RETAINED_MIN_ACTIVE_ORDERID_MAX_SCANS;
    }

    rc = ism_engine_unsetRetainedMessageOnDestination(NULL,
                                                      ismDESTINATION_REGEX_TOPIC,
                                                      ".*Test/Test$",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*Test/Test$");
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_repositionWithOptions(bool emulateCBQWarning)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    uint64_t spreadFactor = 10;
    uint32_t totalRetained = (iettORDERID_SPREAD_REPOSITION_LWM*2)/((uint32_t)spreadFactor);
    uint32_t actualRetained; // The number we actually managed to retain
    ism_time_t startTime, endTime, diffTime;
    double diffTimeSecs;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    rc = test_createClientAndSession("PubRetainedClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    iettTopicTree_t *pTopicTree = iett_getEngineTopicTree(pThreadData);

    char topicString[50];
    int32_t i;
    startTime = ism_common_currentTimeNanos();

    // Note: We are forcing a spread by increasing the NextOrderId by the spreadFactor
    //       value.
    for(i=0;
        i<totalRetained;
        i++, pTopicTree->retNextOrderId += spreadFactor) /* BEAM suppression: loop doesn't iterate */
    {
        sprintf(topicString, "RepositionTest/%08d", i);

        void *payload=NULL;
        ismEngine_MessageHandle_t hMsg;

        rc = test_createMessage(10,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, topicString,
                                &hMsg, &payload);

        if (rc == ISMRC_AllocateError) break;

        TEST_ASSERT_EQUAL(rc, OK);

        // Publish message
        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                NULL,
                                                hMsg,
                                                &pActionsRemaining,
                                                sizeof(pActionsRemaining),
                                                test_decrementActionsRemaining);
        if (rc == ISMRC_AllocateError)
        {
            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            break;
        }
        else if (rc != ISMRC_AsyncCompletion)
        {
            TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        }


        // Every so often, wait for publishes to complete
        if (actionsRemaining > 5000)
        {
            test_waitForRemainingActions(pActionsRemaining);
        }

        if (i==1)
        {
            putNewRetainedUsingSession = hSession;
            putNewRetainedBeforeCopyOfMsg = hMsg;
        }
        else if (i == 3)
        {
            TEST_ASSERT_EQUAL(putNewRetainedUsingSession, hSession);
            putNewRetainedAfterUpdateOfCopiedMsg = hMsg;
        }

        free(payload);
    }
    endTime = ism_common_currentTimeNanos();

    // Wait for all the publishes to finish
    test_waitForRemainingActions(pActionsRemaining);

    if ((actualRetained = i) != totalRetained)
    {
        printf("Only %u of %u total could be put initially\n", actualRetained, totalRetained);
    }

    //Check that we get the correct count of messages on the correct topic
    getRetainedMsgCBContext_t context = {0};
    getRetainedMsgCBContext_t *pContext = &context;

    context.expectMessage = true;
    rc = ism_engine_getRetainedMessage(hSession,
                                       "RepositionTest/#",
                                       &pContext,
                                       sizeof(pContext),
                                       getRetainedMsgCB,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.receivedCount, actualRetained);

    //Check that we can limit the results
    context.receivedCount = 0;
    context.stopAt = 100;
    rc = ism_engine_getRetainedMessage(hSession,
                                       "RepositionTest/#",
                                       &pContext,
                                       sizeof(pContext),
                                       getRetainedMsgCB,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.receivedCount, 100);

    if (fullTest)
    {
        diffTime = endTime-startTime;
        diffTimeSecs = ((double)diffTime) / 1000000000;
        printf("Time to publish %d retained messages is %.2f secs. (%ldns)\n",
                i, diffTimeSecs, diffTime);
    }

    if (emulateCBQWarning == true)
    {
        ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE] = StatusWarning;
    }

    // Go around
    for(int32_t loop=0; loop<10; loop++)
    {
        bool repositioningInitiated = iett_scanForRetMinActiveOrderId(pThreadData, 0, true);
        // First time around we know that a scan should be initated -- others might not
        // because the 1st might still be repositioning... unless the CBQ warning is in place,
        // in which case every go around will try and fail on the 1st message
        if (loop == 0 || emulateCBQWarning == true)
        {
            TEST_ASSERT_EQUAL(repositioningInitiated, true);
        }

        iemnMessagingStatistics_t internalMsgStats;

        iemn_getMessagingStatistics(ieut_getThreadData(), &internalMsgStats);
        TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, actualRetained);
    }

    if (emulateCBQWarning == true)
    {
        ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE] = StatusOk;
    }

    rc = ism_engine_unsetRetainedMessageOnDestination(NULL,
                                                      ismDESTINATION_REGEX_TOPIC,
                                                      "RepositionTest/.*",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), "Repositiontest/.*");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    putNewRetainedBeforeCopyOfMsg = NULL;
    putNewRetainedAfterUpdateOfMsg = putNewRetainedAfterUpdateOfCopiedMsg = NULL;
}

void test_reposition(void)
{
    printf("Starting %s...\n", __func__);

    test_repositionWithOptions(false);
}

void test_reposition_CBQ(void)
{
    printf("Starting %s...\n", __func__);

    test_repositionWithOptions(true);
}

typedef struct tag_testObject_t
{
    int rand;
    ieutSplitListLink_t link;
    uint64_t callCount;
} testObject_t;

typedef struct tag_testObjectContext_t
{
    uint64_t objectCount;
    uint64_t removalCount;
    testObject_t **removals;
    uint64_t removalsMax;
    ieutSplitList_t *list;
    int removalMinimum;
} testObjectContext_t;

ieutSplitListCallbackAction_t doNothingWithObjectFromSplitList(ieutThreadData_t *pThreadData,
                                                               void *object,
                                                               void *context)
{
    testObjectContext_t *objectContext = (testObjectContext_t *)context;

    objectContext->objectCount += 1;

    return ieutSPLIT_LIST_CALLBACK_CONTINUE;
}

ieutSplitListCallbackAction_t doRandomThingWithObjectFromSplitList(ieutThreadData_t *pThreadData,
                                                                   void *object,
                                                                   void *context)
{
    testObject_t *testObject = (testObject_t *)object;
    testObjectContext_t *objectContext = (testObjectContext_t *)context;

    objectContext->objectCount += 1;

    if (testObject->rand >= objectContext->removalMinimum)
    {
        if (objectContext->removalCount == objectContext->removalsMax)
        {
            objectContext->removalsMax += 10000;
            objectContext->removals = realloc(objectContext->removals,
                                              objectContext->removalsMax * sizeof(testObject_t *));
        }
        objectContext->removals[objectContext->removalCount++] = testObject;
        return ieutSPLIT_LIST_CALLBACK_REMOVE_OBJECT;
    }

    return ieutSPLIT_LIST_CALLBACK_CONTINUE;
}

ieutSplitListCallbackAction_t stopAfter500Objects(ieutThreadData_t *pThreadData,
                                                  void *object,
                                                  void *context)
{
    testObjectContext_t *objectContext = (testObjectContext_t *)context;

    objectContext->objectCount += 1;

    if (objectContext->objectCount == 500)
    {
        return ieutSPLIT_LIST_CALLBACK_STOP;
    }

    return ieutSPLIT_LIST_CALLBACK_CONTINUE;
}

ieutSplitListCallbackAction_t countCallsFromSplitList(ieutThreadData_t *pThreadData,
                                                      void *object,
                                                      void *context)
{
    testObject_t *testObject = (testObject_t *)object;
    testObjectContext_t *objectContext = (testObjectContext_t *)context;

    testObject->callCount++;

    objectContext->objectCount += 1;

    return ieutSPLIT_LIST_CALLBACK_CONTINUE;
}

ieutSplitListCallbackAction_t checkObjectCallCountNonZero(ieutThreadData_t *pThreadData,
                                                          void *object,
                                                          void *context)
{
    testObject_t *testObject = (testObject_t *)object;

    TEST_ASSERT_EQUAL(testObject->callCount, 1);

    return ieutSPLIT_LIST_CALLBACK_CONTINUE;
}

void test_splitLists(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ism_time_t startTime, endTime, diffTime;
    double diffTimeSecs;

    printf("Starting %s...\n", __func__);

    ieutSplitList_t *testSplitList;

    int32_t rc = ieut_createSplitList(pThreadData,
                                      offsetof(testObject_t, link),
                                      iemem_messageExpiryData,
                                      &testSplitList);
    TEST_ASSERT_EQUAL(rc, OK);


    testObject_t **rememberedObjects = malloc(50000*sizeof(testObject_t *));
    TEST_ASSERT_PTR_NOT_NULL(rememberedObjects);

    uint32_t rememberedObjectCount = 0;
    testObjectContext_t objectContext;

    startTime = ism_common_currentTimeNanos();
    uint64_t i;
    for(i=0; i<5000000; i++)
    {
        testObject_t *myObject = calloc(1, sizeof(testObject_t));

        myObject->rand = rand()%1000;

        ieut_addObjectToSplitList(testSplitList, myObject);

        // Every so often, try re-adding, removing and adding.
        if (myObject->rand > 990)
        {
            // Double add
            ieut_addObjectToSplitList(testSplitList, myObject);
            // Double remove
            ieut_removeObjectFromSplitList(testSplitList, myObject);
            ieut_removeObjectFromSplitList(testSplitList, myObject);
            // Re add
            ieut_addObjectToSplitList(testSplitList, myObject);
        }

        if (myObject->rand <= 500 && rememberedObjectCount<50000)
        {
            rememberedObjects[rememberedObjectCount++] = myObject;
        }
    }
    endTime = ism_common_currentTimeNanos();

    diffTime = endTime-startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;
    printf("Time to add %lu random numbers is %.2f secs. (%ldns)\n",
                i, diffTimeSecs, diffTime);

    ieut_checkSplitList(testSplitList);

    uint64_t testCount = i;

    memset(&objectContext, 0, sizeof(objectContext));

    // Try traversing without removal
    startTime = ism_common_currentTimeNanos();
    ieut_traverseSplitList(pThreadData, testSplitList, doNothingWithObjectFromSplitList, &objectContext);
    endTime = ism_common_currentTimeNanos();

    // None have gone missing
    TEST_ASSERT_EQUAL(objectContext.objectCount, testCount);

    diffTime = endTime-startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;
    printf("Time to traverse is %.2f secs. (%ldns)\n",
                diffTimeSecs, diffTime);

    memset(&objectContext, 0, sizeof(objectContext));

    // Try stopping after 500
    ieut_traverseSplitList(pThreadData, testSplitList, stopAfter500Objects, &objectContext);
    TEST_ASSERT_EQUAL(objectContext.objectCount, 500);

    memset(&objectContext, 0, sizeof(objectContext));

    // Try traversing doing removal or not, randomly
    startTime = ism_common_currentTimeNanos();
    objectContext.removalMinimum = 501;
    ieut_traverseSplitList(pThreadData, testSplitList, doRandomThingWithObjectFromSplitList, &objectContext);
    endTime = ism_common_currentTimeNanos();

    TEST_ASSERT_EQUAL(objectContext.objectCount, testCount);

    testCount -= objectContext.removalCount;

    for(i=0; i<objectContext.removalCount; i++)
    {
        free(objectContext.removals[i]);
    }
    free(objectContext.removals);

    diffTime = endTime-startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;
    printf("Time to traverse randomly removing or not is %.2f secs. (%ldns)\n",
                diffTimeSecs, diffTime);

    // Check removed ones were really removed
    memset(&objectContext, 0, sizeof(objectContext));
    ieut_traverseSplitList(pThreadData, testSplitList, doNothingWithObjectFromSplitList, &objectContext);
    TEST_ASSERT_EQUAL(objectContext.objectCount, testCount);

    // Check that we're called once for each object
    memset(&objectContext, 0, sizeof(objectContext));
    objectContext.list = testSplitList;
    ieut_traverseSplitList(pThreadData, testSplitList, countCallsFromSplitList, &objectContext);

    // Check that every object got called at least once in the recall test
    ieut_traverseSplitList(pThreadData, testSplitList, checkObjectCallCountNonZero, &objectContext);

    // Test removal of 50,000 objects that may or may not still be in the list
    for(i=0; i<rememberedObjectCount; i++)
    {
        ieut_removeObjectFromSplitList(testSplitList, rememberedObjects[i]);
        free(rememberedObjects[i]);
    }

    free(rememberedObjects);

    // Remove all of the remaining entries
    objectContext.removalMinimum = 0;
    ieut_traverseSplitList(pThreadData, testSplitList, doRandomThingWithObjectFromSplitList, &objectContext);

    for(i=0; i<objectContext.removalCount; i++)
    {
        free(objectContext.removals[i]);
    }
    free(objectContext.removals);

    // Confirm they all got removed
    memset(&objectContext, 0, sizeof(objectContext));
    ieut_traverseSplitList(pThreadData, testSplitList, countCallsFromSplitList, &objectContext);
    TEST_ASSERT_EQUAL(objectContext.objectCount, 0);

#if 0
    printf("Called Multiple times: %lu\n", objectContext.multiCalled);
    printf("Max call count in recall test: %lu\n", objectContext.maxCallCount);
    printf("RemovalCount: %lu\n", objectContext.removalCount);
#endif

    ieut_destroySplitList(pThreadData, testSplitList);
}

CU_TestInfo ISM_RetainedMsgs_CUnit_test_Scale[] =
{
    { "Scale Test", test_scale },
    { "ExpiryHashSet Test", test_splitLists },
    { "Reposition Test", test_reposition },
    { "Reposition Test CBQ", test_reposition_CBQ },
    CU_TEST_INFO_NULL
};

void test_overflow(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    uint32_t totalRetained = 10000;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    uint64_t origMaxMessageCount = iepi_getDefaultPolicyInfo(false)->maxMessageCount;
    iepiMaxMsgBehavior_t origMaxMsgBehavior = iepi_getDefaultPolicyInfo(false)->maxMsgBehavior;

    iepi_getDefaultPolicyInfo(false)->maxMessageCount = totalRetained/2;
    iepi_getDefaultPolicyInfo(false)->maxMsgBehavior = DiscardOldMessages;

    rc = test_createClientAndSession("OverflowClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a subscription to receive the publications 'live' and see that overflow
    ismEngine_SubscriptionAttributes_t subAttrs = {ismENGINE_SUBSCRIPTION_OPTION_SHARED};

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_createSubscription(hClient,
                                       "TEST_LIVE_OVERFLOW_SUB",
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       "OverflowTest/#",
                                       &subAttrs,
                                       NULL,
                                       &pActionsRemaining,
                                       sizeof(pActionsRemaining),
                                       test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    char topicString[50];
    int32_t i;
    for(i=0; i<totalRetained; i++) /* BEAM suppression: loop doesn't iterate */
    {
        sprintf(topicString, "OverflowTest/%08d", i);

        void *payload=NULL;
        ismEngine_MessageHandle_t hMsg;

        rc = test_createMessage(10,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, topicString,
                                &hMsg, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        // Publish message
        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                NULL,
                                                hMsg,
                                                &pActionsRemaining,
                                                sizeof(pActionsRemaining),
                                                test_decrementActionsRemaining);
        if (rc == ISMRC_AllocateError)
        {
            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            break;
        }
        else if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

        // Every so often, wait for publishes to complete
        if (actionsRemaining > 1000)
        {
            test_waitForRemainingActions(pActionsRemaining);
        }

        free(payload);
    }

    // Wait for all the publishes to finish
    test_waitForRemainingActions(pActionsRemaining);

    // Create some subscriptions which we will expect to overflow
    ismEngine_QueueStatistics_t stats;
    ismEngine_Subscription_t *subscription = NULL;

    uint32_t testSubOptions[] = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_SHARED,
                                  ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                                  ismENGINE_SUBSCRIPTION_OPTION_SHARED,
                                  ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
    for(i=0; i<sizeof(testSubOptions)/sizeof(testSubOptions[0]); i++)
    {
        subAttrs.subOptions = testSubOptions[i];

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_createSubscription(hClient,
                                           "TEST_RET_OVERFLOW_SUB",
                                           NULL,
                                           ismDESTINATION_TOPIC,
                                           "#",
                                           &subAttrs,
                                           NULL,
                                           &pActionsRemaining,
                                           sizeof(pActionsRemaining),
                                           test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

        test_waitForRemainingActions(pActionsRemaining);

        rc = iett_findClientSubscription(pThreadData,
                                         hClient->pClientId,
                                         iett_generateClientIdHash(hClient->pClientId),
                                         "TEST_RET_OVERFLOW_SUB",
                                         iett_generateSubNameHash("TEST_RET_OVERFLOW_SUB"),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(subscription);

        ieq_getStats(pThreadData, subscription->queueHandle, &stats);
        //printf("0x%08x : Buffered=%lu Discarded=%lu Rejected=%lu\n",
        //       subAttrs.subOptions, stats.BufferedMsgs, stats.DiscardedMsgs, stats.RejectedMsgs);
        TEST_ASSERT_NOT_EQUAL(stats.BufferedMsgs, 0);
        // TODO: TEST_ASSERT_NOT_EQUAL(stats.DiscardedMsgs, 0);
        TEST_ASSERT_EQUAL(stats.RejectedMsgs, 0);

        iett_releaseSubscription(pThreadData, subscription, false);

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_destroySubscription(hClient,
                                            "TEST_RET_OVERFLOW_SUB",
                                            hClient,
                                            &pActionsRemaining,
                                            sizeof(pActionsRemaining),
                                            test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

        test_waitForRemainingActions(pActionsRemaining);
    }

    // Check that the live subscription overflowed too.
    rc = iett_findClientSubscription(pThreadData,
                                     hClient->pClientId,
                                     iett_generateClientIdHash(hClient->pClientId),
                                     "TEST_LIVE_OVERFLOW_SUB",
                                     iett_generateSubNameHash("TEST_LIVE_OVERFLOW_SUB"),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subscription);

    ieq_getStats(pThreadData, subscription->queueHandle, &stats);
    //printf("Buffered=%lu Discarded=%lu Rejected=%lu\n",
    //        stats.BufferedMsgs, stats.DiscardedMsgs, stats.RejectedMsgs);
    TEST_ASSERT_NOT_EQUAL(stats.BufferedMsgs, 0);
    TEST_ASSERT_NOT_EQUAL(stats.DiscardedMsgs, 0);
    TEST_ASSERT_EQUAL(stats.RejectedMsgs, 0);

    iett_releaseSubscription(pThreadData, subscription, false);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_unsetRetainedMessageOnDestination(NULL,
                                                      ismDESTINATION_REGEX_TOPIC,
                                                      "OverflowTest/.*",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    iepi_getDefaultPolicyInfo(false)->maxMessageCount = origMaxMessageCount;
    iepi_getDefaultPolicyInfo(false)->maxMsgBehavior = origMaxMsgBehavior;

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), "OverflowTest/.*");
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_RetainedMsgs_CUnit_test_Overflow[] =
{
    { "Overflowing Subs", test_overflow },
    CU_TEST_INFO_NULL
};

// NOTE: This is not strictly speaking a retained message test - we want to probe
//       the code that deals with the async callback queue filling up, and we will
//       definitely attempt to use the callback queue if we are publishing a retained
// message (because it will always be written to the store).
//
// The following overridden iemem_queryCurrentMallocState is used to allow us to
// turn the callback warning OFF -- basically this code relies on the fact that the
// function which waits for the queue to empty also calls iemem_queryCurrentMallocState

// Fake return code from iemem_queryCurrentMallocState (iememPlentifulMemory means, return the real value)
iememMemoryLevel_t fakeMallocState = iememPlentifulMemory;
// Number of calls to iemem_querytCurrentMallocState before turning off callback queue warning
uint32_t AsyncCallbackWarning = 0;
iememMemoryLevel_t iemem_queryCurrentMallocState(void)
{
    static iememMemoryLevel_t (* real_iemem_queryCurrentMallocState)(void) = NULL;

    if (real_iemem_queryCurrentMallocState == NULL)
    {
        real_iemem_queryCurrentMallocState = dlsym(RTLD_NEXT, "iemem_queryCurrentMallocState");
        TEST_ASSERT_PTR_NOT_NULL(real_iemem_queryCurrentMallocState);
    }

    iememMemoryLevel_t levelToReturn = real_iemem_queryCurrentMallocState();

    if (AsyncCallbackWarning > 0)
    {
        if (--AsyncCallbackWarning == 0)
        {
            ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE] = StatusOk;
            if (fakeMallocState != iememPlentifulMemory)
            {
                levelToReturn = fakeMallocState;
            }
        }
    }

    return levelToReturn;
}

#define AYNC_CALLBACKQUEUE_WARNING_TOPIC "Callback/Queue/Warning"
void test_capability_StoreAsyncCallbackQueueWarning(void)
{
    uint32_t rc;

    //ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("ACQWClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_ProducerHandle_t hProducer = NULL;

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_TOPIC,
                                   AYNC_CALLBACKQUEUE_WARNING_TOPIC,
                                   &hProducer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer);

    void *payload=NULL;
    ismEngine_MessageHandle_t hMessage;

    // Override the max pause to be 1 second
    uint32_t oldPause = ismEngine_serverGlobal.AsyncCBQAlertMaxPause;
    TEST_ASSERT_EQUAL(oldPause, ismENGINE_DEFAULT_ASYNC_CALLBACK_QUEUE_ALERT_MAX_PAUSE);

    ismEngine_serverGlobal.AsyncCBQAlertMaxPause = 1;

    // Pretend there is an Async Callback queue warning when memory is low
    for(int32_t  i=0; i<3; i++)
    {
        int32_t expectRC;

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, AYNC_CALLBACKQUEUE_WARNING_TOPIC,
                                &hMessage, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        switch(i)
        {
            case 0:
                fakeMallocState = iememEmergencyMemory;
                AsyncCallbackWarning = 1;
                expectRC = ISMRC_AllocateError;
                break;
            case 1:
                fakeMallocState = iememEmergencyMemory;
                swallow_ffdcs = true;
                swallow_expect_core = true;
                AsyncCallbackWarning = ((ismEngine_serverGlobal.AsyncCBQAlertMaxPause * 1000000)/IEAD_ASYNC_CBQ_ALERT_PAUSE_SLEEP_USEC)+10;
                expectRC = ISMRC_AllocateError;
                break;
            case 2:
                AsyncCallbackWarning = 5;
                expectRC = ISMRC_NoMatchingDestinations;
                break;
        }

        ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE] = StatusWarning;

        ismEngine_UnreleasedHandle_t hUnrel;
        rc = sync_ism_engine_putMessageWithDeliveryId(hSession, hProducer,
                                                      NULL,
                                                      hMessage,
                                                      1,
                                                      &hUnrel);
        TEST_ASSERT_EQUAL(rc, expectRC);
        TEST_ASSERT_EQUAL(ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE], StatusOk);

        fakeMallocState = iememPlentifulMemory;
        swallow_ffdcs = false;
        swallow_expect_core = false;
    }

    ismEngine_serverGlobal.AsyncCBQAlertMaxPause = oldPause;

    rc = sync_ism_engine_destroyProducer(hProducer);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the message was actually retained
    getRetainedMsgCBContext_t context = {0};
    getRetainedMsgCBContext_t *pContext = &context;

    context.expectMessage = true;
    context.expectedPayload = payload;
    context.expectedPayloadLen = TEST_SMALL_MESSAGE_SIZE;
    context.receivedCount = 0;

    rc = ism_engine_getRetainedMessage(hSession,
                                       AYNC_CALLBACKQUEUE_WARNING_TOPIC,
                                       &pContext,
                                       sizeof(pContext),
                                       getRetainedMsgCB,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.receivedCount, 1);

    if (payload) free(payload);

    rc = test_destroyClientAndSession(hClient, hSession, true);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_RetainedMsgs_CUnit_test_ResourceLimitation[] =
{
    { "Store CallbackQueue warning", test_capability_StoreAsyncCallbackQueueWarning },
    CU_TEST_INFO_NULL
};



void test_checkOriginServerList(iettOriginServer_t *originServer)
{
    uint32_t count1 = 0;
    uint32_t count2 = 0;

    iettTopicNode_t *curr = originServer->tail;
    iettTopicNode_t *prev = NULL;
    iettTopicNode_t *prevExternal = NULL;

    // TAIL TO HEAD
    while(curr)
    {
        TEST_ASSERT_EQUAL(curr->originNext, prev);
        if (prev != NULL)
        {
            if (prev->currRetTimestamp != curr->currRetTimestamp)
            {
                TEST_ASSERT_GREATER_THAN(prev->currRetTimestamp, curr->currRetTimestamp);
            }
        }

        count1++;
        prev = curr;
        curr = curr->originPrev;
    }
    TEST_ASSERT_EQUAL(prev, originServer->head);

    // HEAD TO TAIL
    curr = originServer->head;
    prev = NULL;
    while(curr)
    {
        TEST_ASSERT_EQUAL(curr->originPrev, prev);
        if (prev != NULL)
        {
            if (curr->currRetTimestamp != prev->currRetTimestamp)
            {
                TEST_ASSERT_GREATER_THAN(curr->currRetTimestamp, prev->currRetTimestamp);
            }
        }

        count2++;
        prev = curr;
        if ((prev->nodeFlags & iettNODE_FLAG_LOCAL_ORIGIN_STATS_ONLY_MASK) == 0) prevExternal = prev;
        curr = curr->originNext;
    }
    TEST_ASSERT_EQUAL(prev, originServer->tail);

    TEST_ASSERT_EQUAL(originServer->stats.localCount, count1);
    TEST_ASSERT_EQUAL(originServer->stats.localCount, count2);

    if (count1 == 0)
    {
        TEST_ASSERT_EQUAL(originServer->head, NULL);
        TEST_ASSERT_EQUAL(originServer->tail, NULL);
    }

    if (prevExternal)
    {
        TEST_ASSERT_EQUAL(originServer->stats.highestTimestampAvailable, prevExternal->currRetTimestamp);
    }
    else
    {
        TEST_ASSERT_EQUAL(originServer->stats.highestTimestampAvailable, 0);
    }
}

#define ORIGIN_SERVER_TOPIC_1 "TEST/ORIGIN_SERVER/1"
#define ORIGIN_SERVER_TOPIC_X "TEST/ORIGIN_SERVER/"
void test_capability_OriginServerTracking(void)
{
    uint32_t rc;

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    ismEngine_MessagingStatistics_t msgStats;
    iemnMessagingStatistics_t internalMsgStats;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    ism_engine_getMessagingStatistics(&msgStats);
    uint64_t expectRetainedMessages = msgStats.RetainedMessages;

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(memcmp(&msgStats, &internalMsgStats.externalStats, sizeof(msgStats)), 0);
    TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);
    uint64_t expectInternalRetainedMessages = internalMsgStats.InternalRetainedMessages;

    rc = test_createClientAndSession("OriginServerTestClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    char *serverUID = (char *)ism_common_getServerUID();
    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;
    TEST_ASSERT_PTR_NOT_NULL(tree);
    TEST_ASSERT_EQUAL(tree->originServers->totalCount, 0);

    iettOriginServer_t *originServer;

    rc = iett_insertOrFindOriginServer(pThreadData, serverUID, iettOP_FIND, &originServer);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    ismEngine_MessageHandle_t hMessage1;

    ism_time_t prevHighest = 0;
    expectRetainedMessages += 1;
    expectInternalRetainedMessages += 1;
    int32_t loop;
    for(loop=0; loop<3; loop++)
    {
        /* Create a message to put */
        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, ORIGIN_SERVER_TOPIC_1,
                                &hMessage1, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                ORIGIN_SERVER_TOPIC_1,
                                                NULL,
                                                hMessage1,
                                                NULL, 0, NULL );
        TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

        iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
        TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, expectRetainedMessages);
        TEST_ASSERT_EQUAL(internalMsgStats.InternalRetainedMessages, expectInternalRetainedMessages);
        TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);
        TEST_ASSERT_EQUAL(tree->originServers->totalCount, 1);

        rc = iett_insertOrFindOriginServer(pThreadData, serverUID, iettOP_FIND, &originServer);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(originServer->serverUID, serverUID);
        TEST_ASSERT_EQUAL(originServer->localServer, true);
        TEST_ASSERT_EQUAL(originServer->stats.count, 1);
        TEST_ASSERT_EQUAL(originServer->head, originServer->tail);
        TEST_ASSERT_GREATER_THAN(originServer->stats.highestTimestampSeen, prevHighest);
        TEST_ASSERT_GREATER_THAN(originServer->stats.highestTimestampAvailable, prevHighest);
        prevHighest = originServer->stats.highestTimestampSeen;

        test_checkOriginServerList(originServer);
    }

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_TOPIC,
                                                      ORIGIN_SERVER_TOPIC_1,
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == ISMRC_NoMatchingDestinations) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    TEST_ASSERT_EQUAL(tree->originServers->totalCount, 1); // We don't destroy it when there are no msgs
    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    bool decrementedInternal = false;
    if (originServer->stats.localCount == 0)
    {
        TEST_ASSERT_PTR_NULL(originServer->head);
        TEST_ASSERT_PTR_NULL(originServer->tail);
        TEST_ASSERT_GREATER_THAN(originServer->stats.highestTimestampSeen, prevHighest);
        TEST_ASSERT_EQUAL(originServer->stats.highestTimestampAvailable, 0);
        expectInternalRetainedMessages -= 1;
        decrementedInternal = true;
    }
    else
    {
        TEST_ASSERT_EQUAL(originServer->head, originServer->tail);
        TEST_ASSERT_PTR_NOT_NULL(originServer->head->currRetMessage);
        TEST_ASSERT_EQUAL(originServer->head->currRetMessage->Header.MessageType, MTYPE_NullRetained);
    }
    expectRetainedMessages -= 1;
    TEST_ASSERT_EQUAL(internalMsgStats.InternalRetainedMessages, expectInternalRetainedMessages);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, expectRetainedMessages);

    test_checkOriginServerList(originServer);

    char originServerUID[10];

    for(loop=0; loop<3; loop++)
    {
        sprintf(originServerUID, "UID%02d", loop);

        /* Create a message to put */
        rc = test_createOriginMessage(TEST_SMALL_MESSAGE_SIZE,
                                      ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                      ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                      ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                      0,
                                      ismDESTINATION_TOPIC, ORIGIN_SERVER_TOPIC_1,
                                      originServerUID, ism_engine_retainedServerTime(),
                                      MTYPE_MQTT_Binary,
                                      &hMessage1, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                ORIGIN_SERVER_TOPIC_1,
                                                NULL,
                                                hMessage1,
                                                NULL, 0, NULL );
        TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

        if (loop == 0)
        {
            expectRetainedMessages += 1;
            if (decrementedInternal) expectInternalRetainedMessages += 1;
        }

        iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
        TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, expectRetainedMessages);
        TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);
        TEST_ASSERT_EQUAL(tree->originServers->totalCount, loop+2);

        rc = iett_insertOrFindOriginServer(pThreadData, originServerUID, iettOP_FIND, &originServer);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(originServer->serverUID, originServerUID);
        TEST_ASSERT_EQUAL(originServer->localServer, false);
        TEST_ASSERT_EQUAL(originServer->stats.count, 1);
        TEST_ASSERT_EQUAL(originServer->head, originServer->tail);
        TEST_ASSERT_GREATER_THAN(originServer->stats.highestTimestampSeen, 0);
        TEST_ASSERT_GREATER_THAN(originServer->stats.highestTimestampAvailable, 0);
        prevHighest = originServer->stats.highestTimestampSeen;

        test_checkOriginServerList(originServer);
    }

    /* Create messages with a range of times from one server, put on different topics  */
    char topic[strlen(ORIGIN_SERVER_TOPIC_X)+10];
    loop=0;
    for(ism_time_t timestamp=prevHighest-10; timestamp<prevHighest+20; timestamp+=5, loop++)
    {
        sprintf(topic, "%s%02d", ORIGIN_SERVER_TOPIC_X, loop+2);

        rc = test_createOriginMessage(TEST_SMALL_MESSAGE_SIZE,
                                      ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                      ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                      ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                      0,
                                      ismDESTINATION_TOPIC, topic,
                                      originServerUID, timestamp,
                                      MTYPE_MQTT_Binary,
                                      &hMessage1, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topic,
                                                NULL,
                                                hMessage1,
                                                NULL, 0, NULL );
        TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

        expectRetainedMessages += 1;
        expectInternalRetainedMessages += 1;
        iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
        TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, expectRetainedMessages);
        TEST_ASSERT_EQUAL(internalMsgStats.InternalRetainedMessages, expectInternalRetainedMessages);
        TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);
        TEST_ASSERT_EQUAL(tree->originServers->totalCount, 4);

        rc = iett_insertOrFindOriginServer(pThreadData, originServerUID, iettOP_FIND, &originServer);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_STRINGS_EQUAL(originServer->serverUID, originServerUID);
        TEST_ASSERT_EQUAL(originServer->stats.count, loop+2);
        TEST_ASSERT_NOT_EQUAL(originServer->head, originServer->tail);
        TEST_ASSERT_GREATER_THAN(originServer->tail->currRetTimestamp, originServer->head->currRetTimestamp);

        test_checkOriginServerList(originServer);
    }

    // Randomly unset the retained messages
    bool alreadyUnset[loop+2];
    memset(alreadyUnset, 0, sizeof(alreadyUnset));

    for(int32_t x=0; x<loop; x++)
    {
        int unsetNo = (int)(rand()%loop)+2;

        sprintf(topic, "%s%02d", ORIGIN_SERVER_TOPIC_X, unsetNo);

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                          ismDESTINATION_TOPIC,
                                                          topic,
                                                          ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                          ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                          NULL,
                                                          &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc != ISMRC_AsyncCompletion)
        {
            TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        }

        if (alreadyUnset[unsetNo] == false)
        {
            expectRetainedMessages -= 1;
            // internal will not change - we do have a real unset message
        }
        alreadyUnset[unsetNo] = true;
    }

    test_waitForRemainingActions(pActionsRemaining);

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, expectRetainedMessages);
    TEST_ASSERT_EQUAL(internalMsgStats.InternalRetainedMessages, expectInternalRetainedMessages);
    test_checkOriginServerList(originServer);

    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_REGEX_TOPIC,
                                                      ".*",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_insertOrFindOriginServer(pThreadData, originServerUID, iettOP_FIND, &originServer);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(originServer->stats.count, 0);

    test_checkOriginServerList(originServer);

    // TODO: Test more stuff

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);

    // Now we know there are no topics in the topic tree - try to clear a specific one, just to drive
    // a different code path.
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), "TEST/TOPIC");
    TEST_ASSERT_EQUAL(rc, OK);
}
#undef ORIGIN_SERVER_TOPIC_1
#undef ORIGIN_SERVER_TOPIC_X

#define OLD_TIMESTAMP_SERVER_TOPIC "TEST/OLD_TIMESTAMP"
void test_capability_OldTimestamp(void)
{
    uint32_t rc;

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hClient[2];
    ismEngine_SessionHandle_t     hSession[2];
    ismEngine_MessagingStatistics_t msgStats;
    iemnMessagingStatistics_t internalMsgStats;

    // Prepare a topic string analysis for later use
    iettTopic_t  topic = {0};
    const char  *substrings[3];
    uint32_t     substringHashes[3];

    memset(&topic, 0, sizeof(topic));
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.initialArraySize = 3;
    topic.topicString = OLD_TIMESTAMP_SERVER_TOPIC;
    topic.destinationType = ismDESTINATION_TOPIC;

    rc = iett_analyseTopicString(pThreadData, &topic);
    TEST_ASSERT_EQUAL(rc, OK)
    TEST_ASSERT_EQUAL(topic.wildcardCount, 0);
    TEST_ASSERT_EQUAL(topic.multicardCount, 0);

    printf("Starting %s...\n", __func__);

    ism_engine_getMessagingStatistics(&msgStats);
    uint64_t expectRetainedMessages = msgStats.RetainedMessages+1;

    rc = test_createClientAndSession("OldTimestampLocal",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient[0], &hSession[0], true);
    TEST_ASSERT_EQUAL(rc, OK);


    rc = test_createClientAndSessionWithProtocol("__OldTimestampFwd",
                                                 PROTOCOL_ID_FWD,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                                 ismENGINE_CREATE_SESSION_TRANSACTIONAL |
                                                 ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                                 &hClient[1], &hSession[1], true);

    char *serverUID = "OLDTSUID";
    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;
    TEST_ASSERT_PTR_NOT_NULL(tree);
    uint64_t expectTotalCount = tree->originServers->totalCount + 1;

    iettOriginServer_t *originServer;

    rc = iett_insertOrFindOriginServer(pThreadData, serverUID, iettOP_FIND, &originServer);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    ismEngine_MessageHandle_t hMessage;
    ismEngine_MessageHandle_t hFirstMessage = NULL;
    ism_time_t prevHighest = 0;
    for(uint32_t loop=3; loop>0; loop--)
    {
        for(uint32_t clientLoop=0; clientLoop<2; clientLoop++)
        {
            /* Create a message to put */
            rc = test_createOriginMessage(TEST_SMALL_MESSAGE_SIZE,
                                          ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                          ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                          ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                          0,
                                          ismDESTINATION_TOPIC, OLD_TIMESTAMP_SERVER_TOPIC,
                                          serverUID, (ism_time_t)loop*100,
                                          MTYPE_MQTT_Binary,
                                          &hMessage, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            if (hFirstMessage == NULL) hFirstMessage = hMessage;

            rc = ism_engine_putMessageOnDestination(hSession[clientLoop],
                                                    ismDESTINATION_TOPIC,
                                                    OLD_TIMESTAMP_SERVER_TOPIC,
                                                    NULL,
                                                    hMessage,
                                                    NULL, 0, NULL );
            TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
        }

        rc = iett_insertOrFindOriginServer(pThreadData, serverUID, iettOP_FIND, &originServer);
        TEST_ASSERT_EQUAL(rc, OK);

        iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
        TEST_ASSERT_EQUAL(internalMsgStats.externalStats.RetainedMessages, expectRetainedMessages);
        TEST_ASSERT_EQUAL(internalMsgStats.RetainedMessagesWithExpirySet, 0);
        TEST_ASSERT_EQUAL(tree->originServers->totalCount, expectTotalCount);

        TEST_ASSERT_STRINGS_EQUAL(originServer->serverUID, serverUID);
        TEST_ASSERT_EQUAL(originServer->stats.count, 1);
        TEST_ASSERT_EQUAL(originServer->head, originServer->tail);
        TEST_ASSERT_EQUAL(originServer->head->currRetMessage, hFirstMessage);
        if (prevHighest != 0)
        {
            TEST_ASSERT_EQUAL(originServer->stats.highestTimestampSeen, prevHighest);
            TEST_ASSERT_EQUAL(originServer->stats.highestTimestampAvailable, prevHighest);
        }
        prevHighest = originServer->stats.highestTimestampSeen;

        test_checkOriginServerList(originServer);
    }

    // Check that pendingUpdates on the topic node is 0 -- there should be no outstanding work.
    iettTopicNode_t *pTopicNode = NULL;
    rc = iett_insertOrFindTopicsNode(pThreadData,
                                     tree->topics,
                                     &topic,
                                     iettOP_FIND,
                                     &pTopicNode);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pTopicNode);
    TEST_ASSERT_EQUAL(pTopicNode->pendingUpdates, 0);

    rc = ism_engine_unsetRetainedMessageOnDestination(hSession[0],
                                                      ismDESTINATION_REGEX_TOPIC,
                                                      ".*",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_insertOrFindOriginServer(pThreadData, serverUID, iettOP_FIND, &originServer);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(originServer->stats.count, 0);

    test_checkOriginServerList(originServer);

    rc = test_destroyClientAndSession(hClient[0], hSession[0], false);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_destroyClientAndSession(hClient[1], hSession[1], false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_RetainedMsgs_CUnit_test_OriginServers[] =
{
    { "OriginServerTracking", test_capability_OriginServerTracking },
    { "OldTimestamp", test_capability_OldTimestamp },
    CU_TEST_INFO_NULL
};

/* Tests which require security initialization */
void test_capability_PolicySelection(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient_C, hClient_T;
    ismEngine_SessionHandle_t hSession_C, hSession_T;
    ism_listener_t *mockListener_C=NULL, *mockListener_T=NULL;
    ism_transport_t *mockTransport_C=NULL, *mockTransport_T=NULL;
    ismSecurity_t *mockContext_C=NULL, *mockContext_T=NULL;

    ismEngine_ConsumerHandle_t hConsumer_C,
                               hConsumer_T;
    retSelMessagesCbContext_t consumer_CContext={0},
                              consumer_TContext={0};
    retSelMessagesCbContext_t *consumer_CCb=&consumer_CContext,
                              *consumer_TCb=&consumer_TContext;

    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    rc = test_createSecurityContext(&mockListener_C,
                                    &mockTransport_C,
                                    &mockContext_C);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener_C);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport_C);
    TEST_ASSERT_PTR_NOT_NULL(mockContext_C);

    mockTransport_C->clientID = "CircleSelectionClient";

    rc = test_createSecurityContext(&mockListener_T,
                                    &mockTransport_T,
                                    &mockContext_T);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener_T);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport_T);
    TEST_ASSERT_PTR_NOT_NULL(mockContext_T);

    mockTransport_T->clientID = "TriangleSelectionClient";

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession(mockTransport_C->clientID,
                                     mockContext_C,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient_C, &hSession_C, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession(mockTransport_T->clientID,
                                     mockContext_T,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient_T, &hSession_T, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add a topic policy that allows publish / subscribe on 'Retained/Selection/CIRCLE'
    // and only selects CIRCLEs.
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.SELECTS_CIRCLE_POLICY\","
                                "\"Name\":\"SELECTS_CIRCLE_POLICY\","
                                "\"ClientID\":\"Circle*\","
                                "\"Topic\":\"Retained/Selection\","
                                "\"DefaultSelectionRule\":\"Shape='CIRCLE'\","
                                "\"ActionList\":\"subscribe,publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Add a topic policy that allows publish / subscribe on 'Retained/Selection/#'
    // and only selects TRIANGLEs
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.SELECTS_TRIANGLE_POLICY\","
                                "\"Name\":\"SELECTS_TRIANGLE_POLICY\","
                                "\"ClientID\":\"Triangle*\","
                                "\"Topic\":\"Retained/Selection\","
                                "\"DefaultSelectionRule\":\"Shape='TRIANGLE'\","
                                "\"ActionList\":\"subscribe,publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policies with security contexts
    rc = test_addPolicyToSecContext(mockContext_C, "SELECTS_CIRCLE_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext_C, "SELECTS_TRIANGLE_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext_T, "SELECTS_CIRCLE_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext_T, "SELECTS_TRIANGLE_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish a triangular message as a retained message
    ism_prop_t *msgProps = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(msgProps);
    ism_field_t f;

    f.type = VT_String;
    f.val.s = "TRIANGLE";
    rc = ism_common_setProperty(msgProps, "Shape", &f);
    TEST_ASSERT_EQUAL(rc, OK);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaTypes[2];
    size_t areaLengths[2];
    void *areas[2];
    concat_alloc_t FlatProperties = { NULL };
    char localPropBuffer[1024];

    FlatProperties.buf = localPropBuffer;
    FlatProperties.len = 1024;

    rc = ism_common_serializeProperties(msgProps, &FlatProperties);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_common_freeProperties(msgProps);

    // Add the topic to the properties
    char *topicString = "Retained/Selection";
    f.type = VT_String;
    f.val.s = topicString;
    ism_common_putPropertyID(&FlatProperties, ID_Topic, &f);

    // Add the serverUID to the properties
    f.type = VT_String;
    f.val.s = (char *)ism_common_getServerUID();
    ism_common_putPropertyID(&FlatProperties, ID_OriginServer, &f);

    // Add the serverTime to the properties
    f.type = VT_Long;
    f.val.l = ism_engine_retainedServerTime();
    ism_common_putPropertyID(&FlatProperties, ID_ServerTime, &f);

    areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
    areaLengths[0] = FlatProperties.used;
    areas[0] = FlatProperties.buf;
    areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
    areaLengths[1] = strlen(topicString)+1;
    areas[1] = (void *)topicString;

    header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
    header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
    header.Flags = ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;

    ismEngine_MessageHandle_t hMessage = NULL;
    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areas,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage);

    rc = ism_engine_putMessageOnDestination(hSession_T,
                                            ismDESTINATION_TOPIC,
                                            "Retained/Selection",
                                            NULL,
                                            hMessage,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    printf("  ...work\n");

    // Create a consumer that should only get CIRCLES (i.e. not match)
    consumer_CContext.hSession = hSession_C;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSession_C,
                                   ismDESTINATION_TOPIC,
                                   "Retained/Selection",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer_CCb,
                                   sizeof(retSelMessagesCbContext_t *),
                                   retSelMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer_C,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer_CContext.received, 0);
    TEST_ASSERT_EQUAL(consumer_CContext.retained, 0);
    TEST_ASSERT_EQUAL(consumer_CContext.published_for_retain, 0);
    TEST_ASSERT_EQUAL(consumer_CContext.propagate_retained, 0);

    // Create a subscription that should only get TRIANGLES (i.e. match)
    consumer_TContext.hSession = hSession_T;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSession_T,
                                   ismDESTINATION_TOPIC,
                                   "Retained/Selection",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &consumer_TCb,
                                   sizeof(retSelMessagesCbContext_t *),
                                   retSelMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer_T,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer_TContext.received, 1);
    TEST_ASSERT_EQUAL(consumer_TContext.retained, 1);
    TEST_ASSERT_EQUAL(consumer_TContext.published_for_retain, 1);
    TEST_ASSERT_EQUAL(consumer_TContext.propagate_retained, 1);

    // Republish to consumer1 - still should not match
    rc = ism_engine_republishRetainedMessages(hSession_C,
                                              hConsumer_C,
                                              NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(consumer_CContext.received, 0);
    TEST_ASSERT_EQUAL(consumer_CContext.retained, 0);
    TEST_ASSERT_EQUAL(consumer_CContext.published_for_retain, 0);
    TEST_ASSERT_EQUAL(consumer_CContext.propagate_retained, 0);

    // Do something *REALLY NASTY* to drive the 'late subscription' logic...
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    TEST_ASSERT_PTR_NOT_NULL(pThreadData);

    iettNewSubCreationData_t *creationData_C = iett_getNewSubCreationData(hConsumer_C->engineObject);
    TEST_ASSERT_PTR_NOT_NULL(creationData_C);

    iettNewSubCreationData_t *creationData_T = iett_getNewSubCreationData(hConsumer_T->engineObject);
    TEST_ASSERT_PTR_NOT_NULL(creationData_T);

    uint64_t fakeSUV = 0;
    uint64_t fakeRUV = creationData_C->retUpdatesValue + 1;
    if (fakeRUV <= creationData_T->retUpdatesValue) fakeRUV = creationData_T->retUpdatesValue + 1;

    rc = iett_putRetainedMessageToNewSubs(pThreadData, "Retained/Selection", hMessage, fakeSUV, fakeRUV);
    TEST_ASSERT_EQUAL(rc, OK);

    // Message should have been delivered to the 'TRIANGLE' sub, but not to the 'CIRCLE' sub
    TEST_ASSERT_EQUAL(consumer_CContext.received, 0);
    TEST_ASSERT_EQUAL(consumer_CContext.retained, 0);
    TEST_ASSERT_EQUAL(consumer_TContext.received, 2);
    TEST_ASSERT_EQUAL(consumer_TContext.retained, 1); // Should be delivered 'live', not retained

    printf("  ...disconnect\n");

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession_T,
                                                      ismDESTINATION_TOPIC,
                                                      "Retained/Selection",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    rc = test_destroyClientAndSession(hClient_C, hSession_C, true);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_destroyClientAndSession(hClient_T, hSession_T, true);
    TEST_ASSERT_EQUAL(rc, OK);

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_deleteSecurityPolicy("UID.SELECTS_TRIANGLE_POLICY", "SELECTS_TRIANGLE_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_deleteSecurityPolicy("UID.SELECTS_CIRCLE_POLICY", "SELECTS_CIRCLE_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_RetainedMsgs_CUnit_test_Security[] =
{
    { "PolicySelection", test_capability_PolicySelection },
    CU_TEST_INFO_NULL
};

/* Defects */
#define DEFECT_171313_TOPIC "DEFECT/171313"
#define MAX_MSG_LIMIT 10
#define SUB_NAME "Small_SUB"
void test_defect_171313(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hSubClient, hWillClient;
    ismEngine_SessionHandle_t     hSubSession, hWillSession;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("SUB_CLIENT",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hSubClient, &hSubSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession("WILL_CLIENT",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hWillClient, &hWillSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a QoS2 subscription which can accept up to 10 messages
    uint32_t origMMC = iepi_getDefaultPolicyInfo(false)->maxMessageCount;
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = MAX_MSG_LIMIT;
    TEST_ASSERT_EQUAL(iepi_getDefaultPolicyInfo(false)->maxMsgBehavior, RejectNewMessages);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

    rc = sync_ism_engine_createSubscription(hSubClient,
                                            SUB_NAME,
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            DEFECT_171313_TOPIC,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_MessageHandle_t hMessage;
    void   *payload=NULL;

    for(uint32_t loop=0; loop<iepi_getDefaultPolicyInfo(false)->maxMessageCount; loop++)
    {
        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, DEFECT_171313_TOPIC,
                                &hMessage, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_putMessageOnDestination(hWillSession,
                                                ismDESTINATION_TOPIC,
                                                DEFECT_171313_TOPIC,
                                                NULL,
                                                hMessage,
                                                &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }

    test_waitForRemainingActions(pActionsRemaining);

    // Now set a retained will message and destroy the WillClient
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE, // Don't want properties
                            0,
                            ismDESTINATION_TOPIC, DEFECT_171313_TOPIC,
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);

    hMessage->Header.Flags = ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN; // We really want a retained message...

    test_incrementActionsRemaining(pActionsRemaining, 1);
    uint32_t delayInterval = 1;
    rc = ism_engine_setWillMessage(hWillClient,
                                   ismDESTINATION_TOPIC,
                                   DEFECT_171313_TOPIC,
                                   hMessage,
                                   1,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    // This should publish the will message after a 1 second delay.
    test_destroyClientAndSession(hWillClient, hWillSession, false);

    printf("  ...Sleeping for %u seconds to cope with will delay\n", delayInterval*2);
    sleep((int32_t)(delayInterval*2));

    // Should be MAX_MSG_LIMIT messages on the subscription queue...
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_Subscription_t *pSub = NULL;

    rc = iett_findClientSubscription(pThreadData,
                                     hSubClient->pClientId,
                                     iett_generateClientIdHash(hSubClient->pClientId),
                                     SUB_NAME,
                                     iett_generateSubNameHash(SUB_NAME),
                                     &pSub);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pSub);

    ismEngine_QueueStatistics_t stats = {0};
    ieq_getStats(pThreadData, pSub->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, MAX_MSG_LIMIT);

    iett_releaseSubscription(pThreadData, pSub, false);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroySubscription(hSubClient,
                                        SUB_NAME,
                                        hSubClient,
                                        &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = test_destroyClientAndSession(hSubClient, hSubSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    test_waitForRemainingActions(pActionsRemaining);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);

    // Make subscriptions default size again
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = origMMC;
}
#undef SUB_NAME
#undef MAX_MSG_LIMIT

#define DEFECT_171323_TOPIC "DEFECT/171323"
#define MAX_MSG_LIMIT 10
#define SUB_NAME "Small_SUB"
void test_defect_171323(void)
{
    uint32_t rc;

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    ismEngine_ClientState_t      *pClient;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    ismEngine_Transaction_t      *pTran;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("171323_CLIENT",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    pClient = hClient;

    // Create a QoS2 subscription which can accept up to 10 messages
    uint32_t origMMC = iepi_getDefaultPolicyInfo(false)->maxMessageCount;
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = MAX_MSG_LIMIT;
    TEST_ASSERT_EQUAL(iepi_getDefaultPolicyInfo(false)->maxMsgBehavior, RejectNewMessages);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

    rc = sync_ism_engine_createSubscription(hClient,
                                            SUB_NAME,
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            DEFECT_171323_TOPIC,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_MessageHandle_t hMessage;
    void   *payload=NULL;

    rc = ietr_createLocal(pThreadData, hSession, true, false, NULL, &pTran);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pTran);

    for(uint32_t loop=0; loop<MAX_MSG_LIMIT+1; loop++)
    {
        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, DEFECT_171323_TOPIC,
                                &hMessage, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                DEFECT_171323_TOPIC,
                                                pTran,
                                                hMessage,
                                                &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else if (loop == MAX_MSG_LIMIT)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_DestinationFull);
            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        }
    }

    test_waitForRemainingActions(pActionsRemaining);

    rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_RolledBack);

    // Should be no messages on the subscription queue...
    ismEngine_Subscription_t *pSub = NULL;

    rc = iett_findClientSubscription(pThreadData,
                                     pClient->pClientId,
                                     iett_generateClientIdHash(pClient->pClientId),
                                     SUB_NAME,
                                     iett_generateSubNameHash(SUB_NAME),
                                     &pSub);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pSub);

    ismEngine_QueueStatistics_t stats = {0};
    ieq_getStats(pThreadData, pSub->queueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);

    iett_releaseSubscription(pThreadData, pSub, false);

    test_incrementActionsRemaining(pActionsRemaining, 3);
    rc = ism_engine_destroySubscription(hClient,
                                        SUB_NAME,
                                        hClient,
                                        &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroySession(hSession, &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_DISCARD, &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    // Make sure we don't have any retained nodes in the topic tree
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);

    // Make subscriptions default size again
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = origMMC;
}
#undef SUB_NAME
#undef MAX_MSG_LIMIT

CU_TestInfo ISM_RetainedMsgs_CUnit_test_Defects[] =
{
    { "Defect171313", test_defect_171313 },
    { "Defect171323", test_defect_171323 },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initRetainedMsgs(void)
{
    test_memoryDumpStats_before(NULL, &testNumber);
    return test_engineInit_DEFAULT;
}

int initRetainedMsgsDurable(void)
{
    test_memoryDumpStats_before(NULL, &testNumber);
    return test_engineInit(true, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int initRetainedMsgsOverflow(void)
{
    return test_engineInit(true, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           1024);
}

int initRetainedMsgsWarm(void)
{
    return test_engineInit_WARM;
}

int initRetainedMsgsSecurity(void)
{
    return test_engineInit(true, false,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termRetainedMsgs(void)
{
    int32_t rc = test_engineTerm(true);
    test_memoryDumpStats_after(NULL, testNumber);
    return rc;
}

int termRetainedMsgsDurable(void)
{
    int32_t rc = test_engineTerm(true);
    test_memoryDumpStats_after(NULL, testNumber);
    return rc;
}

int termRetainedMsgsWarm(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_RetainedMsgs_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("Flags", initRetainedMsgs, termRetainedMsgsWarm, ISM_RetainedMsgs_CUnit_test_RetainedFlags_Phase1),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_RetainedMsgs_CUnit_phase2suites[] =
{
    IMA_TEST_SUITE("Flags", initRetainedMsgsWarm, termRetainedMsgs, ISM_RetainedMsgs_CUnit_test_RetainedFlags_Phase2),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_RetainedMsgs_CUnit_phase3suites[] =
{
    IMA_TEST_SUITE("WillMsgRetained", initRetainedMsgs, termRetainedMsgsWarm, ISM_RetainedMsgs_CUnit_test_WillMsgRetained_Phase1),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_RetainedMsgs_CUnit_phase4suites[] =
{
    IMA_TEST_SUITE("WillMsgRetained", initRetainedMsgsWarm, termRetainedMsgs, ISM_RetainedMsgs_CUnit_test_WillMsgRetained_Phase2),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_RetainedMsgs_CUnit_phase5suites[] =
{
    IMA_TEST_SUITE("Basic", initRetainedMsgs, termRetainedMsgs, ISM_RetainedMsgs_CUnit_test_Basic),
    IMA_TEST_SUITE("Expiry", initRetainedMsgs, termRetainedMsgs, ISM_RetainedMsgs_CUnit_test_Expiry),
    IMA_TEST_SUITE("Persistence", initRetainedMsgsDurable, termRetainedMsgsDurable, ISM_RetainedMsgs_CUnit_test_Persistence),
    IMA_TEST_SUITE("Migration_1", initRetainedMsgsDurable, termRetainedMsgsDurable, ISM_RetainedMsgs_CUnit_test_Migration_1),
    IMA_TEST_SUITE("Migration_2", initRetainedMsgsDurable, termRetainedMsgsDurable, ISM_RetainedMsgs_CUnit_test_Migration_2),
    IMA_TEST_SUITE("Migration_3", initRetainedMsgsDurable, termRetainedMsgsDurable, ISM_RetainedMsgs_CUnit_test_Migration_3),
    IMA_TEST_SUITE("Transactions", initRetainedMsgs, termRetainedMsgs, ISM_RetainedMsgs_CUnit_test_Transactions),
    IMA_TEST_SUITE("OriginServers", initRetainedMsgs, termRetainedMsgs, ISM_RetainedMsgs_CUnit_test_OriginServers),
    IMA_TEST_SUITE("Scale", initRetainedMsgs, termRetainedMsgs, ISM_RetainedMsgs_CUnit_test_Scale),
    IMA_TEST_SUITE("Overflow", initRetainedMsgsDurable, termRetainedMsgs, ISM_RetainedMsgs_CUnit_test_Overflow),
    IMA_TEST_SUITE("ResourceLimitation", initRetainedMsgs, termRetainedMsgs, ISM_RetainedMsgs_CUnit_test_ResourceLimitation),
    IMA_TEST_SUITE("Security", initRetainedMsgsSecurity, termRetainedMsgs, ISM_RetainedMsgs_CUnit_test_Security),
    IMA_TEST_SUITE("Defects", initRetainedMsgs, termRetainedMsgs, ISM_RetainedMsgs_CUnit_test_Defects),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_RetainedMsgs_CUnit_phase1suites
                              , ISM_RetainedMsgs_CUnit_phase2suites
                              , ISM_RetainedMsgs_CUnit_phase3suites
                              , ISM_RetainedMsgs_CUnit_phase4suites
                              , ISM_RetainedMsgs_CUnit_phase5suites
                              };

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
