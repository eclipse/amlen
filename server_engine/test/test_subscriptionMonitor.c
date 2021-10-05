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
/* Module Name: test_topicTree.c                                     */
/*                                                                   */
/* Description: Main source file for CUnit test of Topic Tree        */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "engineInternal.h"
#include "engineMonitoring.h"
#include "messageExpiry.h"
#include "simpQ.h"
#include "simpQInternal.h"

#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

//****************************************************************************
/// @brief Test Operation of getSubscriptionMonitor with different StatTypes
//****************************************************************************
#define STATTYPES_SUBSCRIPTIONS 1000

char *STATTYPES_TOPICS[] = {"A", "A/#", "#", "+"};
#define STATTYPES_NUMTOPICS (sizeof(STATTYPES_TOPICS)/sizeof(STATTYPES_TOPICS[0]))

uint32_t STATTYPES_SUBOPTIONS[] = {ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                                   ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE,
                                   ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                                   ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE};
#define STATTYPES_NUMSUBOPTIONS (sizeof(STATTYPES_SUBOPTIONS)/sizeof(STATTYPES_SUBOPTIONS[0]))

void test_capability_StatTypes(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    char *subscriptionTopics[STATTYPES_SUBSCRIPTIONS] = {0};
    uint64_t publishedMessages[STATTYPES_SUBSCRIPTIONS] = {0};
    uint64_t mostPublishedIndex = 0;
    uint64_t expiringMessages[STATTYPES_SUBSCRIPTIONS] = {0};
    uint64_t mostExpiringIndex = 0;
    ismEngine_ClientStateHandle_t hClient[STATTYPES_SUBSCRIPTIONS];
    ismEngine_SessionHandle_t hSession[STATTYPES_SUBSCRIPTIONS];
    ismEngine_ConsumerHandle_t consumers[STATTYPES_SUBSCRIPTIONS];

    printf("Starting %s...\n", __func__);

    /* Create a message to play with */
    void *payload="StatTypes Test";
    ismEngine_MessageHandle_t message;

    ismEngine_SubscriptionMonitor_t *results = NULL;
    uint32_t resultCount;

    // Test with no subscriptions
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                           50,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);
    TEST_ASSERT_PTR_NULL(results);

    uint64_t totalMsgsPublished = 0;
    uint64_t totalExpiringMsgs = 0;

    // For some messages we will set an expiry time in the future
    uint32_t startTime = ism_common_nowExpire();
    uint32_t expireTestSeconds = 1;

    // Create nondurable subscriptions on various (but matching) topics and publish
    // variable number of messages to them
    for(int32_t i=0; i<STATTYPES_SUBSCRIPTIONS; i++)
    {
        char clientId[50];

        sprintf(clientId, "TestClientId%d", i);

        rc = test_createClientAndSession(clientId,
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hClient[i], &hSession[i], false);
        TEST_ASSERT_EQUAL(rc, OK);

        subscriptionTopics[i] = STATTYPES_TOPICS[rand()%STATTYPES_NUMTOPICS];
        ismEngine_SubscriptionAttributes_t subAttrs = { STATTYPES_SUBOPTIONS[rand()%STATTYPES_NUMSUBOPTIONS] };
        rc = ism_engine_createConsumer(hSession[i],
                                       ismDESTINATION_TOPIC,
                                       subscriptionTopics[i],
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       NULL, 0, NULL,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &consumers[i],
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // Put up to 4 messages - with 1000 subs, max on any queue will be 4000, so no
        // rejected msgs expected
        int32_t msgsToPublish = rand()%4;
        int32_t msgsToExpire = 0;

        for(int32_t j=0; j<msgsToPublish; j++)
        {
            uint32_t expiryValue;

            // 15% of the time, set expiry
            if (rand()%100 > 95)
            {
                expiryValue = startTime + expireTestSeconds;
                msgsToExpire++;
            }
            else
            {
                expiryValue = 0;
            }

            rc = test_createMessage(strlen((char *)payload)+1,
                                    ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                    ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                    ismMESSAGE_FLAGS_NONE,
                                    expiryValue,
                                    ismDESTINATION_TOPIC, STATTYPES_TOPICS[0],
                                    &message, &payload);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                            STATTYPES_TOPICS[0],
                                                            message,
                                                            NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        // Increment our expected counters, and find the one with the most!
        for(int32_t j=0; j<=i; j++)
        {
            totalMsgsPublished += msgsToPublish;
            publishedMessages[j] += msgsToPublish;
            if (publishedMessages[j] > publishedMessages[mostPublishedIndex])
            {
                mostPublishedIndex = j;
            }
            totalExpiringMsgs += msgsToExpire;
            expiringMessages[j] += msgsToExpire;
            if (expiringMessages[j] > expiringMessages[mostExpiringIndex])
            {
                mostExpiringIndex = j;
            }
        }
    }

    // Test for request with 0 maxResults
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                           0,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    // Test for invalid request type
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           0xFFFFFFFF,
                                           50,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    // Test for highest buffered messages
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                           50,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 50);
    TEST_ASSERT_EQUAL(results[0].stats.BufferedMsgs, publishedMessages[mostPublishedIndex]);
    TEST_ASSERT_EQUAL(results[0].stats.BufferedMsgsHWM, results[0].stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(results[0].stats.RejectedMsgs, 0);
    for(int32_t i=1; i<resultCount; i++) // check ordering and constant values
    {
        TEST_ASSERT(results[i-1].stats.BufferedMsgs >= results[i].stats.BufferedMsgs,
                    ("result %d [%lu] < result %d [%lu]",
                     i-1, results[i-1].stats.BufferedMsgs, i, results[i].stats.BufferedMsgs));
        TEST_ASSERT(results[i-1].consumers == 1,
                    ("result %d %u != 1", i-1, results[i-1].consumers));
        TEST_ASSERT(results[i-1].shared == false,
                    ("result %d shared != false", i-1));
        TEST_ASSERT(results[i-1].durable == false,
                    ("result %d durable != false", i-1));
        TEST_ASSERT_PTR_NULL(results[i-1].messagingPolicyName);         // No policy
        TEST_ASSERT_EQUAL(results[i-1].policyType, ismSEC_POLICY_LAST); // LAST indicating 'unique' policy
    }

    ism_engine_freeSubscriptionMonitor(results);

    // Work out which we think should have the lowest buffered / expiring msgs
    uint64_t lowestPublishedIndex = 0;
    uint64_t lowestExpiringIndex = 0;

    for(int32_t i=0; i<STATTYPES_SUBSCRIPTIONS; i++)
    {
        if (publishedMessages[i]<publishedMessages[lowestPublishedIndex])
        {
            lowestPublishedIndex=i;
        }

        if (expiringMessages[i]<expiringMessages[lowestExpiringIndex])
        {
            lowestExpiringIndex=i;
        }
    }

    // Test for lowest buffered messages
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS,
                                           25,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 25);
    TEST_ASSERT_EQUAL(results[0].stats.BufferedMsgs, publishedMessages[lowestPublishedIndex]);
    for(int32_t i=1; i<resultCount; i++) // check ordering
    {
        TEST_ASSERT(results[i-1].stats.BufferedMsgs <= results[i].stats.BufferedMsgs,
                    ("result %d [%lu] > result %d [%lu]",
                     i-1, results[i-1].stats.BufferedMsgs, i, results[i].stats.BufferedMsgs));
        TEST_ASSERT(results[i-1].consumers == 1,
                    ("result %d %u != 1", i-1, results[i-1].consumers));
        TEST_ASSERT(results[i-1].shared == false,
                    ("result %d shared != false", i-1));
        TEST_ASSERT(results[i-1].durable == false,
                    ("result %d durable != false", i-1));
        TEST_ASSERT_PTR_NULL(results[i-1].messagingPolicyName); // No policy
    }

    ism_engine_freeSubscriptionMonitor(results);

    // Test for highest buffered percentage
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_HIGHEST_BUFFEREDPERCENT,
                                           10,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 10);
    for(int32_t i=1; i<resultCount; i++) // check ordering
    {
        TEST_ASSERT(results[i-1].stats.BufferedPercent >= results[i].stats.BufferedPercent,
                    ("result %d [%3.2f] < result %d [%3.2f]",
                     i-1, results[i-1].stats.BufferedPercent, i, results[i].stats.BufferedPercent));
        TEST_ASSERT(results[i-1].consumers == 1,
                    ("result %d %u != 1", i-1, results[i-1].consumers));
        TEST_ASSERT(results[i-1].shared == false,
                    ("result %d shared != false", i-1));
        TEST_ASSERT(results[i-1].durable == false,
                    ("result %d durable != false", i-1));
        TEST_ASSERT_PTR_NULL(results[i-1].messagingPolicyName); // No policy
    }

    ism_engine_freeSubscriptionMonitor(results);

    // Test for lowest buffered percentage
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_LOWEST_BUFFEREDPERCENT,
                                           50,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 50);
    for(int32_t i=1; i<resultCount; i++) // check ordering
    {
        TEST_ASSERT(results[i-1].stats.BufferedPercent <= results[i].stats.BufferedPercent,
                    ("result %d [%3.2f] > result %d [%3.2f]",
                     i-1, results[i-1].stats.BufferedPercent, i, results[i].stats.BufferedPercent));
        TEST_ASSERT(results[i-1].consumers == 1,
                    ("result %d %u != 1", i-1, results[i-1].consumers));
        TEST_ASSERT(results[i-1].shared == false,
                    ("result %d shared != false", i-1));
        TEST_ASSERT(results[i-1].durable == false,
                    ("result %d durable != false", i-1));
        TEST_ASSERT_PTR_NULL(results[i-1].messagingPolicyName); // No policy
    }

    ism_engine_freeSubscriptionMonitor(results);

    // Confirm that the counts of dropped and buffered messages are 0
    ismEngine_MessagingStatistics_t msgStats; // purposefully not initializing
    ism_engine_getMessagingStatistics(&msgStats);
    TEST_ASSERT_NOT_EQUAL(msgStats.ServerShutdownTime, 0);
    TEST_ASSERT_EQUAL(msgStats.DroppedMessages, 0);
    TEST_ASSERT_EQUAL(msgStats.ExpiredMessages, 0);
    TEST_ASSERT_EQUAL(msgStats.BufferedMessages, totalMsgsPublished);
    TEST_ASSERT_EQUAL(msgStats.RetainedMessages, 0);

    // Test for highest rejected messages (all should be 0)
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS,
                                           1, // Unusual number of results
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 1);
    TEST_ASSERT_EQUAL(results[0].stats.RejectedMsgs, 0);

    ism_engine_freeSubscriptionMonitor(results);

    // Test for lowest rejected messages (all should be 0)
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_LOWEST_REJECTEDMSGS,
                                           100,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 100);
    TEST_ASSERT_EQUAL(results[resultCount/2].stats.RejectedMsgs, 0);
    TEST_ASSERT_EQUAL(results[0].stats.RejectedMsgs, results[resultCount-1].stats.RejectedMsgs);

    ism_engine_freeSubscriptionMonitor(results);

    // Test for highest published messages
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                           231, // unusual number of results
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 231);
    TEST_ASSERT_EQUAL(results[0].stats.ProducedMsgs, publishedMessages[mostPublishedIndex]);
    for(int32_t i=1; i<resultCount; i++) // check ordering
    {
        TEST_ASSERT(results[i-1].stats.ProducedMsgs >= results[i].stats.ProducedMsgs,
                    ("result %d [%lu] < result %d [%lu]",
                     i-1, results[i-1].stats.ProducedMsgs, i, results[i].stats.ProducedMsgs));
        TEST_ASSERT_PTR_NULL(results[i-1].messagingPolicyName); // No policy
    }

    ism_engine_freeSubscriptionMonitor(results);

    // Test for lowest published messages
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_LOWEST_PUBLISHEDMSGS,
                                           10,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 10);
    for(int32_t i=1; i<resultCount; i++) // check ordering
    {
        TEST_ASSERT(results[i-1].stats.ProducedMsgs <= results[i].stats.ProducedMsgs,
                    ("result %d [%lu] > result %d [%lu]",
                     i-1, results[i-1].stats.ProducedMsgs, i, results[i].stats.ProducedMsgs));
        TEST_ASSERT_PTR_NULL(results[i-1].messagingPolicyName); // No policy
    }

    ism_engine_freeSubscriptionMonitor(results);

    // Test for highest buffered high water mark percentage
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_HIGHEST_BUFFEREDHWMPERCENT,
                                           10,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 10);
    for(int32_t i=1; i<resultCount; i++) // check ordering
    {
        TEST_ASSERT(results[i-1].stats.BufferedHWMPercent >= results[i].stats.BufferedHWMPercent,
                    ("result %d [%3.2f] < result %d [%3.2f]",
                     i-1, results[i-1].stats.BufferedHWMPercent, i, results[i].stats.BufferedHWMPercent));
    }

    ism_engine_freeSubscriptionMonitor(results);

    // Test for lowest buffered high water mark percentage
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_LOWEST_BUFFEREDHWMPERCENT,
                                           50,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 50);
    for(int32_t i=1; i<resultCount; i++) // check ordering
    {
        TEST_ASSERT(results[i-1].stats.BufferedHWMPercent <= results[i].stats.BufferedHWMPercent,
                    ("result %d [%3.2f] > result %d [%3.2f]",
                     i-1, results[i-1].stats.BufferedHWMPercent, i, results[i].stats.BufferedHWMPercent));
    }

    ism_engine_freeSubscriptionMonitor(results);

    // Test that ismENGINE_MONITOR_ALL_UNSORTED is (now) supported
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_ALL_UNSORTED,
                                           10,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, STATTYPES_SUBSCRIPTIONS);

    ism_engine_freeSubscriptionMonitor(results);

    iemnMessagingStatistics_t internalMsgStats; // Purposefully not initializing

    // Force expiry of messages
    uint32_t endTime = ism_common_nowExpire();

    // Wait for expiry time to pass
    if (endTime - startTime < expireTestSeconds+1)
    {
        iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
        TEST_ASSERT_EQUAL(internalMsgStats.BufferedMessagesWithExpirySet, totalExpiringMsgs);
        uint32_t sleepTime = (expireTestSeconds+1) - (endTime-startTime);
        printf("Sleeping %u seconds for expiry...\n", sleepTime);
        sleep(sleepTime);
    }

    iemeExpiryControl_t *expiryControl = (iemeExpiryControl_t *)ismEngine_serverGlobal.msgExpiryControl;

    // Ensure initial expiry scan has finished
    uint64_t scansStarted = 1;
    while(scansStarted > expiryControl->scansEnded) { usleep(5000); }
    scansStarted = expiryControl->scansEnded;

    // Force expiry reaper to run - but shouldn't have expired yet
    scansStarted++;
    ieme_wakeMessageExpiryReaper(pThreadData); while(scansStarted > expiryControl->scansEnded) { usleep(5000); }

    iemn_getMessagingStatistics(pThreadData, &internalMsgStats);
    TEST_ASSERT_EQUAL(internalMsgStats.BufferedMessagesWithExpirySet, 0);

    // Test for expired message counts (highest & lowest)
    for(ismEngineMonitorType_t type=ismENGINE_MONITOR_HIGHEST_EXPIREDMSGS;
        type<=ismENGINE_MONITOR_LOWEST_DISCARDEDMSGS;
        type++)
    {
        rc = ism_engine_getSubscriptionMonitor(&results,
                                               &resultCount,
                                               type,
                                               100,
                                               NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, 100);

        // Check the values and ordering of the results
        switch(type)
        {
            case ismENGINE_MONITOR_HIGHEST_EXPIREDMSGS:
                TEST_ASSERT_EQUAL(results[0].stats.ExpiredMsgs, expiringMessages[mostExpiringIndex]);
                for(int32_t i=1; i<resultCount; i++) // check ordering
                {
                    TEST_ASSERT(results[i-1].stats.ExpiredMsgs >= results[i].stats.ExpiredMsgs,
                            ("result %d [%lu] < result %d [%lu]",
                                    i-1, results[i-1].stats.ExpiredMsgs, i, results[i].stats.ExpiredMsgs));
                }
                break;
            case ismENGINE_MONITOR_LOWEST_EXPIREDMSGS:
                TEST_ASSERT_EQUAL(results[0].stats.ExpiredMsgs, expiringMessages[lowestExpiringIndex]);
                for(int32_t i=1; i<resultCount; i++) // check ordering
                {
                    TEST_ASSERT_EQUAL(results[i].stats.DiscardedMsgs, 0); // Don't expect any discarded in *this* test
                    TEST_ASSERT(results[i-1].stats.ExpiredMsgs <= results[i].stats.ExpiredMsgs,
                                ("result %d [%lu] > result %d [%lu]",
                                        i-1, results[i-1].stats.ExpiredMsgs, i, results[i].stats.ExpiredMsgs));
                }
                break;
            case ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS:
            case ismENGINE_MONITOR_LOWEST_DISCARDEDMSGS:
                for(int32_t i=1; i<resultCount; i++) // check they're all zero (no discarding in test)
                {
                    TEST_ASSERT_EQUAL(results[i].stats.DiscardedMsgs, 0); // Don't expect any discarded in *this* test
                }
                break;
            default:
                TEST_ASSERT(false, ("invalid type value %d", type));
                break;
        }

        ism_engine_freeSubscriptionMonitor(results);
    }

    for(int32_t i=0; i<STATTYPES_SUBSCRIPTIONS; i++)
    {
        rc = ism_engine_destroyConsumer(consumers[i], NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        rc = test_destroyClientAndSession(hClient[i], hSession[i], false);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

CU_TestInfo ISM_SubscriptionMonitor_CUnit_test_capability_StatTypes[] =
{
    { "StatTypes", test_capability_StatTypes },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test Operation of getSubscriptionMonitor applying filters
//****************************************************************************
char *FILTERS_CLIENT1_TOPICS[] = {"A", "A/B", "B", "B/+"};
#define FILTERS_CLIENT1_NUMTOPICS (sizeof(FILTERS_CLIENT1_TOPICS)/sizeof(FILTERS_CLIENT1_TOPICS[0]))
char *FILTERS_CLIENT2_TOPICS[] = {"A", "X", "X/#", "Y", "Z"};
#define FILTERS_CLIENT2_NUMTOPICS (sizeof(FILTERS_CLIENT2_TOPICS)/sizeof(FILTERS_CLIENT2_TOPICS[0]))

char *FILTERS_TESTTOPICSTRING1_TOPICS[] = {"*", "K", "B", "A", "A*", "*B", "*B*", "B/+", "K*"};
#define FILTERS_TESTTOPICSTRING1_NUMTOPICS (sizeof(FILTERS_TESTTOPICSTRING1_TOPICS)/sizeof(FILTERS_TESTTOPICSTRING1_TOPICS[0]))
uint32_t FILTERS_TESTTOPICSTRING1_EXPECTCOUNT[] = {4, 0, 1, 1, 2, 2, 3, 1, 0};
int32_t FILTERS_TESTTOPICSTRING1_EXPECTRC[] = {OK, OK, OK, OK, OK, OK, OK, OK, OK};

char *FILTERS_CLIENTID1_IDS[] = {"*", "Z", "Z*", "Client1", "Client1*", "*lien*", "*1"};
#define FILTERS_CLIENTID1_NUMIDS (sizeof(FILTERS_CLIENTID1_IDS)/sizeof(FILTERS_CLIENTID1_IDS[0]))
uint32_t FILTERS_CLIENTID1_EXPECTCOUNT[] = {4, 0, 0, 4, 4, 4, 4};
int32_t FILTERS_CLIENTID1_EXPECTRC[] = {OK, OK, OK, OK, OK, OK, OK};

char *FILTERS_REGEX_CLIENTID1_IDS[] = {".*", "Z", "Z.*", "[CX]l+ient1", "^Client1.*", ".*lien.*", ".*1","^[zX]*lient.*", "["};
#define FILTERS_REGEX_CLIENTID1_NUMIDS (sizeof(FILTERS_REGEX_CLIENTID1_IDS)/sizeof(FILTERS_REGEX_CLIENTID1_IDS[0]))
uint32_t FILTERS_REGEX_CLIENTID1_EXPECTCOUNT[] = {4, 0, 0, 4, 4, 4, 4, 0, 0};
int32_t FILTERS_REGEX_CLIENTID1_EXPECTRC[] = {OK, OK, OK, OK, OK, OK, OK, OK, ISMRC_InvalidParameter};

char *FILTERS_SUBTYPES1_TYPES[] = {"All", "Durable", "Nondurable",
                                   "Shared", "Nonshared", "Nondurable,Shared",
                                   "Nondurable,Nonshared", "Nondurable, Nonshared",
                                   "Invalid", "DURABLE  ", "NoNDurAble ,  SharEd ",
                                   "", ",", ",durable"};
#define FILTERS_SUBTYPES1_NUMTYPES (sizeof(FILTERS_SUBTYPES1_TYPES)/sizeof(FILTERS_SUBTYPES1_TYPES[0]))
uint32_t FILTERS_SUBTYPES1_EXPECTCOUNT[] = {5, 1, 4,
                                            1, 4, 0,
                                            4, 4,
                                            0, 1, 0,
                                            0, 0, 0};
int32_t FILTERS_SUBTYPES1_EXPECTRC[] = {OK, OK, OK,
                                        OK, OK, OK,
                                        OK, OK,
                                        ISMRC_InvalidParameter, OK, OK,
                                        ISMRC_InvalidParameter, ISMRC_InvalidParameter, ISMRC_InvalidParameter};

char *FILTERS_SUBNAMES1_NAMES[] = {"*", "XYZ", "ABC*", "SUBNAME*"};
#define FILTERS_SUBNAMES1_NUMNAMES (sizeof(FILTERS_SUBNAMES1_NAMES)/sizeof(FILTERS_SUBNAMES1_NAMES[0]))
uint32_t FILTERS_SUBNAMES1_EXPECTCOUNT[] = {5, 0, 0, 1};
int32_t FILTERS_SUBNAMES1_EXPECTRC[] = {OK, OK, OK, OK};

void test_capability_Filters(void)
{
    uint32_t rc;

    ismEngine_SessionHandle_t hSession1;
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_ConsumerHandle_t consumers1[FILTERS_CLIENT1_NUMTOPICS];
    ism_field_t f;
    ism_prop_t *filterProperties = ism_common_newProperties(10);

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("Client1",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    // Create subscriptions for Client1
    for(int32_t i=0; i<FILTERS_CLIENT1_NUMTOPICS; i++)
    {
        subAttrs.subOptions = STATTYPES_SUBOPTIONS[rand()%STATTYPES_NUMSUBOPTIONS];
        rc = ism_engine_createConsumer(hSession1,
                                       ismDESTINATION_TOPIC,
                                       FILTERS_CLIENT1_TOPICS[i],
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       NULL, 0, NULL,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &consumers1[i],
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    ismEngine_SubscriptionMonitor_t *results = NULL;
    uint32_t resultCount;

    // Test with no filter initially
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                           50,
                                           NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, FILTERS_CLIENT1_NUMTOPICS);
    int32_t seen[FILTERS_CLIENT1_NUMTOPICS]={0};
    for(int32_t i=0; i<resultCount; i++) // check ClientIds
    {
        TEST_ASSERT_STRINGS_EQUAL(results[i].clientId, hClient1->pClientId);

        // Count the occurrences of each topic string seen
        for(int32_t j=0; j<FILTERS_CLIENT1_NUMTOPICS; j++)
        {
            if (strcmp(results[i].topicString, FILTERS_CLIENT1_TOPICS[j])==0)
            {
                seen[j]++;
                break;
            }
        }
    }

    // Make sure we saw each string once
    for(int32_t i=0; i<FILTERS_CLIENT1_NUMTOPICS; i++)
    {
        TEST_ASSERT_EQUAL(seen[i], 1);
    }

    ism_engine_freeSubscriptionMonitor(results);

    f.type = VT_String;

    // Filter on various topic strings
    for(int32_t i=0; i<FILTERS_TESTTOPICSTRING1_NUMTOPICS; i++)
    {
        f.val.s = FILTERS_TESTTOPICSTRING1_TOPICS[i];
        rc = ism_common_setProperty(filterProperties,
                                    ismENGINE_MONITOR_FILTER_TOPICSTRING,
                                    &f);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_getSubscriptionMonitor(&results,
                                               &resultCount,
                                               ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS,
                                               50,
                                               filterProperties);
        TEST_ASSERT_EQUAL(rc, FILTERS_TESTTOPICSTRING1_EXPECTRC[i]);
        TEST_ASSERT_EQUAL(resultCount, FILTERS_TESTTOPICSTRING1_EXPECTCOUNT[i]);

        ism_engine_freeSubscriptionMonitor(results);

        ism_common_clearProperties(filterProperties);
    }

    // Filter on various client Ids
    for(int32_t i=0; i<FILTERS_CLIENTID1_NUMIDS; i++)
    {
        f.val.s = FILTERS_CLIENTID1_IDS[i];
        rc = ism_common_setProperty(filterProperties,
                                    ismENGINE_MONITOR_FILTER_CLIENTID,
                                    &f);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_getSubscriptionMonitor(&results,
                                               &resultCount,
                                               ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                               50,
                                               filterProperties);
        TEST_ASSERT_EQUAL(rc, FILTERS_CLIENTID1_EXPECTRC[i]);
        TEST_ASSERT_EQUAL(resultCount, FILTERS_CLIENTID1_EXPECTCOUNT[i]);

        ism_engine_freeSubscriptionMonitor(results);

        ism_common_clearProperties(filterProperties);
    }

    // Filter on various regular expression client Ids
    for(int32_t i=0; i<FILTERS_REGEX_CLIENTID1_NUMIDS; i++)
    {
        f.val.s = FILTERS_REGEX_CLIENTID1_IDS[i];
        rc = ism_common_setProperty(filterProperties,
                                    ismENGINE_MONITOR_FILTER_REGEX_CLIENTID,
                                    &f);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_getSubscriptionMonitor(&results,
                                               &resultCount,
                                               ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                               50,
                                               filterProperties);
        TEST_ASSERT_EQUAL(rc, FILTERS_REGEX_CLIENTID1_EXPECTRC[i]);
        TEST_ASSERT_EQUAL(resultCount, FILTERS_REGEX_CLIENTID1_EXPECTCOUNT[i]);

        ism_engine_freeSubscriptionMonitor(results);

        ism_common_clearProperties(filterProperties);
    }

    // Filter for resource set id, but no resource set defined so should be ignored.
    f.val.s = "NORESOURCESET";
    rc = ism_common_setProperty(filterProperties, ismENGINE_MONITOR_FILTER_RESOURCESET_ID, &f);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                           50,
                                           filterProperties);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 4);

    ism_engine_freeSubscriptionMonitor(results);

    ism_common_clearProperties(filterProperties);

    // Create one named subscription (must be durable or shared - so picking both)
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ismENGINE_SUBSCRIPTION_OPTION_SHARED;

    rc = sync_ism_engine_createSubscription(hClient1,
                                            "SUBNAME",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "/A/B/C",
                                            &subAttrs,
                                            hClient1);
    TEST_ASSERT_EQUAL(rc, OK);

    // Filter on various sub types
    for(int32_t i=0; i<FILTERS_SUBTYPES1_NUMTYPES; i++)
    {
        f.val.s = FILTERS_SUBTYPES1_TYPES[i];
        rc = ism_common_setProperty(filterProperties,
                                    ismENGINE_MONITOR_FILTER_SUBTYPE,
                                    &f);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_getSubscriptionMonitor(&results,
                                               &resultCount,
                                               ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                               50,
                                               filterProperties);
        TEST_ASSERT_EQUAL(rc, FILTERS_SUBTYPES1_EXPECTRC[i]);
        TEST_ASSERT_EQUAL(resultCount, FILTERS_SUBTYPES1_EXPECTCOUNT[i]);

        ism_engine_freeSubscriptionMonitor(results);

        ism_common_clearProperties(filterProperties);
    }

    // Filter on various sub names (none of our subs have names)
    for(int32_t i=0; i<FILTERS_SUBNAMES1_NUMNAMES; i++)
    {
        f.val.s = FILTERS_SUBNAMES1_NAMES[i];
        rc = ism_common_setProperty(filterProperties,
                                    ismENGINE_MONITOR_FILTER_SUBNAME,
                                    &f);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_getSubscriptionMonitor(&results,
                                               &resultCount,
                                               ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                               50,
                                               filterProperties);
        TEST_ASSERT_EQUAL(rc, FILTERS_SUBNAMES1_EXPECTRC[i]);
        TEST_ASSERT_EQUAL(resultCount, FILTERS_SUBNAMES1_EXPECTCOUNT[i]);

        ism_engine_freeSubscriptionMonitor(results);

        ism_common_clearProperties(filterProperties);
    }

    // Destroy the named subscription
    rc = ism_engine_destroySubscription(hClient1,
                                        "SUBNAME",
                                        hClient1,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    f.val.s = "SUBNAME";
    rc = ism_common_setProperty(filterProperties,
                                ismENGINE_MONITOR_FILTER_SUBNAME,
                                &f);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_getSubscriptionMonitor(&results,
                                           &resultCount,
                                           ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS,
                                           50,
                                           filterProperties);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);

    ism_engine_freeSubscriptionMonitor(results);

    ism_common_clearProperties(filterProperties);

    // Filter on policy name (Note we do some nasty illegal messing with policyInfo)
    iepiPolicyInfo_t *pPolicy0 = ieq_getPolicyInfo(consumers1[0]->queueHandle);
    char *prevName0 = pPolicy0->name;
    iepiPolicyInfo_t *pPolicy1 = ieq_getPolicyInfo(consumers1[1]->queueHandle);
    char *prevName1 = pPolicy1->name;
    pPolicy1->name = "NOTMATCHING";
    for(int32_t loop=0; loop<2; loop++)
    {
        f.val.s = "MATCHING";
        rc = ism_common_setProperty(filterProperties,
                                    ismENGINE_MONITOR_FILTER_MESSAGINGPOLICY,
                                    &f);
        TEST_ASSERT_EQUAL(rc, OK);

        if (loop == 0)
        {
            pPolicy0->name = f.val.s;
        }
        else
        {
            pPolicy0->name = prevName0;
        }

        rc = ism_engine_getSubscriptionMonitor(&results,
                                               &resultCount,
                                               ismENGINE_MONITOR_HIGHEST_BUFFEREDHWMPERCENT,
                                               50,
                                               filterProperties);
        TEST_ASSERT_EQUAL(rc, OK);
        if (loop == 0)
        {
            TEST_ASSERT_EQUAL(resultCount, 1);
        }
        else
        {
            TEST_ASSERT_EQUAL(resultCount, 0);
        }

        ism_engine_freeSubscriptionMonitor(results);

        ism_common_clearProperties(filterProperties);
    }
    pPolicy1->name = prevName1;

    // Clean up consumers
    for(int32_t i=0; i<FILTERS_CLIENT1_NUMTOPICS; i++)
    {
        rc = ism_engine_destroyConsumer(consumers1[i], NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = test_destroyClientAndSession(hClient1, hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_SubscriptionMonitor_CUnit_test_capability_Filters[] =
{
    { "Filters", test_capability_Filters },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test Operation of getSubscriptionMonitor applying filters
//****************************************************************************
void test_defect_178299(void)
{
    uint32_t rc;

    ismEngine_SessionHandle_t hSession1;
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_ConsumerHandle_t consumers[2];

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("Client1",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    for(int32_t i=0; i<(sizeof(consumers)/sizeof(consumers[0])); i++)
    {
        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
        rc = ism_engine_createConsumer(hSession1,
                                       ismDESTINATION_TOPIC,
                                       FILTERS_CLIENT1_TOPICS[i],
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       NULL, 0, NULL,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &consumers[i],
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Make it look like one of the subscriptions (the 1st one) has had lots of work go through it...
    iesqQueue_t *simpQ = (iesqQueue_t *)(consumers[0]->queueHandle);

    simpQ->discardedMsgs = 3000000000UL;
    simpQ->bufferedMsgs = 3000000000UL;
    simpQ->enqueueCount = 3000000000UL;
    simpQ->rejectedMsgs = 3000000000UL;
    simpQ->expiredMsgs = 3000000000UL;

    ismEngine_SubscriptionMonitor_t *results = NULL;
    uint32_t resultCount;

    ismEngineMonitorType_t monitorType[] = { ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS,
                                             ismENGINE_MONITOR_LOWEST_DISCARDEDMSGS,
                                             ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                             ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS,
                                             ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                             ismENGINE_MONITOR_LOWEST_PUBLISHEDMSGS,
                                             ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS,
                                             ismENGINE_MONITOR_LOWEST_REJECTEDMSGS,
                                             ismENGINE_MONITOR_HIGHEST_EXPIREDMSGS,
                                             ismENGINE_MONITOR_LOWEST_EXPIREDMSGS,
                                           };

    // Check that the expected consumer is at the top of these tests...
    for(int32_t i=0; i<sizeof(monitorType)/sizeof(monitorType[0]); i++)
    {
        rc = ism_engine_getSubscriptionMonitor(&results,
                                               &resultCount,
                                               monitorType[i],
                                               10,
                                               NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, 2);
        TEST_ASSERT_EQUAL(results[0].subscription, consumers[i%2]->engineObject);

        ism_engine_freeSubscriptionMonitor(results);
    }

    // Clean up consumers
    for(int32_t i=0; i<(sizeof(consumers)/sizeof(consumers[0])); i++)
    {
        rc = ism_engine_destroyConsumer(consumers[i], NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = test_destroyClientAndSession(hClient1, hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_SubscriptionMonitor_CUnit_test_defects[] =
{
    { "Defect178299", test_defect_178299 },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initSubscriptionMonitor(void)
{
    return test_engineInit_DEFAULT;
}

int termSubscriptionMonitor(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_SubscriptionMonitor_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("StatTypes", initSubscriptionMonitor, termSubscriptionMonitor, ISM_SubscriptionMonitor_CUnit_test_capability_StatTypes),
    IMA_TEST_SUITE("Filters", initSubscriptionMonitor, termSubscriptionMonitor, ISM_SubscriptionMonitor_CUnit_test_capability_Filters),
    IMA_TEST_SUITE("Defects", initSubscriptionMonitor, termSubscriptionMonitor, ISM_SubscriptionMonitor_CUnit_test_defects),
    CU_SUITE_INFO_NULL,
};

int setup_CU_registry(int argc, char **argv, CU_SuiteInfo *allSuites)
{
    CU_SuiteInfo *tempSuites = NULL;

    int retval = 0;

    if (argc > 1)
    {
        int suites = 0;

        for(int i=1; i<argc; i++)
        {
            if (!strcasecmp(argv[i], "FULL"))
            {
                if (i != 1)
                {
                    retval = 97;
                    break;
                }
                // Driven from 'make fulltest' ignore this.
            }
            else if (!strcasecmp(argv[i], "verbose"))
            {
                CU_basic_set_mode(CU_BRM_VERBOSE);
            }
            else if (!strcasecmp(argv[i], "silent"))
            {
                CU_basic_set_mode(CU_BRM_SILENT);
            }
            else
            {
                bool suitefound = false;
                int index = atoi(argv[i]);
                int totalSuites = 0;

                CU_SuiteInfo *curSuite = allSuites;

                while(curSuite->pTests)
                {
                    if (!strcasecmp(curSuite->pName, argv[i]))
                    {
                        suitefound = true;
                        break;
                    }

                    totalSuites++;
                    curSuite++;
                }

                if (!suitefound)
                {
                    if (index > 0 && index <= totalSuites)
                    {
                        curSuite = &allSuites[index-1];
                        suitefound = true;
                    }
                }

                if (suitefound)
                {
                    tempSuites = realloc(tempSuites, sizeof(CU_SuiteInfo) * (suites+2));
                    memcpy(&tempSuites[suites++], curSuite, sizeof(CU_SuiteInfo));
                    memset(&tempSuites[suites], 0, sizeof(CU_SuiteInfo));
                }
                else
                {
                    printf("Invalid test suite '%s' specified, the following are valid:\n\n", argv[i]);

                    index=1;

                    curSuite = allSuites;

                    while(curSuite->pTests)
                    {
                        printf(" %2d : %s\n", index++, curSuite->pName);
                        curSuite++;
                    }

                    printf("\n");

                    retval = 99;
                    break;
                }
            }
        }
    }

    if (retval == 0)
    {
        if (tempSuites)
        {
            CU_register_suites(tempSuites);
        }
        else
        {
            CU_register_suites(allSuites);
        }
    }

    if (tempSuites) free(tempSuites);

    return retval;
}

int main(int argc, char *argv[])
{
    int trclvl = 0;
    int retval = 0;

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    ism_time_t seedVal = ism_common_currentTimeNanos();

    srand(seedVal);

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_SubscriptionMonitor_CUnit_allsuites);

    if (retval == 0)
    {
        CU_basic_run_tests();

        CU_RunSummary * CU_pRunSummary_Final;
        CU_pRunSummary_Final = CU_get_run_summary();
        printf("Random Seed =     %"PRId64"\n", seedVal);
        printf("\n[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
               CU_pRunSummary_Final->nTestsRun,
               CU_pRunSummary_Final->nTestsFailed,
               CU_pRunSummary_Final->nAssertsFailed);
        if ((CU_pRunSummary_Final->nTestsFailed > 0) ||
            (CU_pRunSummary_Final->nAssertsFailed > 0))
        {
            retval = 1;
        }
    }

    CU_cleanup_registry();

    test_processTerm(retval == 0);

mod_exit:

    return retval;
}
