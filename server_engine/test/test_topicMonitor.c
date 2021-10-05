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
/* Module Name: test_topicMonitor.c                                  */
/*                                                                   */
/* Description: Main source file for CUnit test of Topic Monitor     */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "engineInternal.h"
#include "transaction.h"
#include "topicTree.h"
#include "topicTreeInternal.h"

#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_assert.h"
#include "test_utils_config.h"

//****************************************************************************
/// @brief Test defect 27576
//****************************************************************************
#define DEFECT_27576_TOPIC "DEFECT/27576"

void test_defect_27576(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("TestClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t i=0; i<3; i++)
    {
        void *payload="Defect 27576 Test";
        ismEngine_MessageHandle_t message;

        rc = test_createMessage(strlen((char *)payload)+1,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, DEFECT_27576_TOPIC,
                                &message, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        // Try and put one message - this initializes this thread's cache
        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                DEFECT_27576_TOPIC,
                                                NULL,
                                                message,
                                                NULL, 0, NULL);
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc);

        // First iteration, create a topic monitor
        if (i==0)
        {
            ism_field_t f;
            ism_prop_t *monitorProps = ism_common_newProperties(2);

            f.type = VT_String;
            f.val.s = DEFECT_27576_TOPIC "/#";
            ism_common_setProperty(monitorProps,
                                   ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING "TEST",
                                   &f);

            // Start a topic monitor by emulating a config callback from the admin
            // component (thereby testing more of the engine).
            rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_TOPICMONITOR,
                                           "TEST",
                                           monitorProps,
                                           ISM_CONFIG_CHANGE_PROPS);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        ismEngine_TopicMonitor_t *topicMonitorResults = NULL;
        uint32_t resultCount = 0;

        rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                        &resultCount,
                                        ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS,
                                        1,
                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, 1);
        TEST_ASSERT_STRINGS_EQUAL(topicMonitorResults->topicString, DEFECT_27576_TOPIC "/#");
        TEST_ASSERT_EQUAL(topicMonitorResults->stats.PublishedMsgs, i);

        ism_engine_freeTopicMonitor(topicMonitorResults);
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_deactivateSubsNodeStats(ieut_getThreadData(), DEFECT_27576_TOPIC "/#");
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test rollback defect 36192
//****************************************************************************
#define DEFECT_36192_TOPIC "DEFECT/36192"

void test_defect_36192(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("TestClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_field_t f;
    ism_prop_t *monitorProps = ism_common_newProperties(2);

    f.type = VT_String;
    f.val.s = DEFECT_36192_TOPIC "/#";
    ism_common_setProperty(monitorProps,
                           ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING "TEST",
                           &f);

    // Start a topic monitor by emulating a config callback from the admin
    // component (thereby testing more of the engine).
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_TOPICMONITOR,
                                   "TEST",
                                   monitorProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    TEST_ASSERT_EQUAL(rc, OK);

    uint64_t expectPublished = 0;
    for(int32_t i=0; i<3; i++)
    {
        void *payload="Defect 36192 Test";
        ismEngine_MessageHandle_t message;

        rc = test_createMessage(strlen((char *)payload)+1,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, DEFECT_36192_TOPIC,
                                &message, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        ismEngine_Transaction_t *pTran;
        rc = ietr_createLocal(pThreadData, hSession, false, false, NULL, &pTran);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                DEFECT_36192_TOPIC,
                                                pTran,
                                                message,
                                                NULL, 0, NULL);
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Allow for informational RCs

        for(int32_t j=0; j<2; j++)
        {
            ismEngine_TopicMonitor_t *topicMonitorResults = NULL;
            uint32_t resultCount = 0;

            rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                            &resultCount,
                                            ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS,
                                            1,
                                            NULL);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_EQUAL(resultCount, 1);
            TEST_ASSERT_STRINGS_EQUAL(topicMonitorResults->topicString, DEFECT_36192_TOPIC "/#");
            TEST_ASSERT_EQUAL(topicMonitorResults->stats.PublishedMsgs, expectPublished);
            TEST_ASSERT_EQUAL(topicMonitorResults->stats.FailedPublishes, 0);
            TEST_ASSERT_EQUAL(topicMonitorResults->stats.RejectedMsgs, 0);

            ism_engine_freeTopicMonitor(topicMonitorResults);

            if (j == 0)
            {
                switch(i)
                {
                    case 1:
                        ietr_rollback(pThreadData, pTran, (ismEngine_Session_t *)hSession, IETR_ROLLBACK_OPTIONS_NONE);
                        break;
                    default:
                        ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, (ismEngine_Session_t *)hSession,
                                    NULL, NULL);
                        expectPublished++;
                        break;
                }
            }
        }
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_deactivateSubsNodeStats(pThreadData, f.val.s);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test Monitoring comparison
//****************************************************************************
#define DEFECT_178299_TOPIC1 "DEFECT/178299/TOPIC1/#"
#define DEFECT_178299_TOPIC2 "DEFECT/178299/TOPIC2/#"

void test_defect_178299(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("TestClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_field_t f;
    ism_prop_t *monitorProps = ism_common_newProperties(2);

    f.type = VT_String;
    f.val.s = DEFECT_178299_TOPIC1;
    ism_common_setProperty(monitorProps,
                           ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING "TEST1",
                           &f);

    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_TOPICMONITOR,
                                   "TEST1",
                                   monitorProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_common_setProperty(monitorProps, ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING "TEST1", NULL);

    f.val.s = DEFECT_178299_TOPIC2;
    ism_common_setProperty(monitorProps,
                           ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING "TEST2",
                           &f);

    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_TOPICMONITOR,
                                   "TEST2",
                                   monitorProps,
                                   ISM_CONFIG_CHANGE_PROPS);
    TEST_ASSERT_EQUAL(rc, OK);

    // Make sure topic1 stats are large...
    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;
    iettSubsNodeStats_t *stats = tree->subNodeStatsHead;
    uint32_t activeStats = 0;
    bool adjustedTopicStats = false;
    while(stats)
    {
        if (stats->topicStats.ResetTime != 0) activeStats++;

        if (strcmp(stats->node->topicString, DEFECT_178299_TOPIC1) == 0)
        {
            stats->topicStats.FailedPublishes = 3000000000UL;
            stats->topicStats.PublishedMsgs = 3000000000UL;
            stats->topicStats.RejectedMsgs = 3000000000UL;
            adjustedTopicStats = true;
        }

        stats = stats->next;
    }

    TEST_ASSERT_EQUAL(activeStats, 2);
    TEST_ASSERT_EQUAL(adjustedTopicStats, true);

    ismEngine_TopicMonitor_t *topicMonitorResults = NULL;
    uint32_t resultCount;

    ismEngineMonitorType_t monitorType[] = { ismENGINE_MONITOR_HIGHEST_FAILEDPUBLISHES,
                                             ismENGINE_MONITOR_LOWEST_FAILEDPUBLISHES,
                                             ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                             ismENGINE_MONITOR_LOWEST_PUBLISHEDMSGS,
                                             ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS,
                                             ismENGINE_MONITOR_LOWEST_REJECTEDMSGS
                                           };

    for(int32_t i=0; i<sizeof(monitorType)/sizeof(monitorType[0]); i++)
    {
        rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                        &resultCount,
                                        monitorType[i],
                                        10,
                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, 2);

        TEST_ASSERT_STRINGS_EQUAL(topicMonitorResults[i%2].topicString, DEFECT_178299_TOPIC1);

        ism_engine_freeTopicMonitor(topicMonitorResults);
    }

    #if 0
    uint64_t expectPublished = 0;
    for(int32_t i=0; i<3; i++)
    {
        void *payload="Defect 36192 Test";
        ismEngine_MessageHandle_t message;

        rc = test_createMessage(strlen((char *)payload)+1,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, DEFECT_36192_TOPIC,
                                &message, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        ismEngine_Transaction_t *pTran;
        rc = ietr_createLocal(pThreadData, hSession, false, false, NULL, &pTran);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                DEFECT_36192_TOPIC,
                                                pTran,
                                                message,
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        for(int32_t j=0; j<2; j++)
        {
            ismEngine_TopicMonitor_t *topicMonitorResults = NULL;
            uint32_t resultCount = 0;

            rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                            &resultCount,
                                            ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS,
                                            1,
                                            NULL);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_EQUAL(resultCount, 1);
            TEST_ASSERT_STRINGS_EQUAL(topicMonitorResults->topicString, DEFECT_36192_TOPIC "/#");
            TEST_ASSERT_EQUAL(topicMonitorResults->stats.PublishedMsgs, expectPublished);
            TEST_ASSERT_EQUAL(topicMonitorResults->stats.FailedPublishes, 0);
            TEST_ASSERT_EQUAL(topicMonitorResults->stats.RejectedMsgs, 0);

            ism_engine_freeTopicMonitor(topicMonitorResults);

            if (j == 0)
            {
                switch(i)
                {
                    case 1:
                        ietr_rollback(pThreadData, pTran, (ismEngine_Session_t *)hSession, IETR_ROLLBACK_OPTIONS_NONE);
                        break;
                    default:
                        ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, (ismEngine_Session_t *)hSession,
                                    NULL, NULL);
                        expectPublished++;
                        break;
                }
            }
        }
    }
#endif

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_deactivateSubsNodeStats(pThreadData, DEFECT_178299_TOPIC1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_deactivateSubsNodeStats(pThreadData, DEFECT_178299_TOPIC2);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_TopicMonitor_CUnit_test_defects[] =
{
    { "Defect27576", test_defect_27576 },
    { "Defect36192", test_defect_36192 },
    { "Defect178299", test_defect_178299 },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test Operation of getTopicMonitor with ismENGINE_MONITOR_ALL_UNSORTED
//****************************************************************************
void test_AllUnsorted(void)
{
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    ismEngine_TopicMonitor_t *topicMonitorResults = NULL;
    uint32_t resultCount = 0;

    // Start with no topic monitors
    rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                    &resultCount,
                                    ismENGINE_MONITOR_ALL_UNSORTED,
                                    1,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);

    ism_engine_freeTopicMonitor(topicMonitorResults);

    // Create 5 topic monitors
    char *monitorTopics[6] = {"A/B/#", "C/D/#", "B/A/#", "D/C/#", "A/A/#", NULL };
    bool monitorFound[5] = {0};

    char **thisMonitor = &monitorTopics[0];

    while(*thisMonitor)
    {
        ism_field_t f;
        ism_prop_t *monitorProps = ism_common_newProperties(2);
        char topicMonitorName[strlen(ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING)+strlen(*thisMonitor)+1];

        f.type = VT_String;
        f.val.s = *thisMonitor;
        strcpy(topicMonitorName, ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING);
        strcat(topicMonitorName, *thisMonitor);
        ism_common_setProperty(monitorProps, topicMonitorName, &f);

        // Start a topic monitor by emulating a config callback from the admin
        // component (thereby testing more of the engine).
        rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_TOPICMONITOR,
                                       *thisMonitor,
                                       monitorProps,
                                       ISM_CONFIG_CHANGE_PROPS);
        TEST_ASSERT_EQUAL(rc, OK);

        thisMonitor++;
    }

    // Request only 2 results (should be ignored)
    topicMonitorResults = NULL;
    resultCount = 0;
    rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                    &resultCount,
                                    ismENGINE_MONITOR_ALL_UNSORTED,
                                    2,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 5);

    // Check we found all the topics expected
    uint32_t i=0;
    for(i=0; i<resultCount; i++)
    {
        thisMonitor = &monitorTopics[0];

        while(*thisMonitor)
        {
            if (strcmp(topicMonitorResults[i].topicString, *thisMonitor) == 0)
            {
                monitorFound[(int)(thisMonitor-&monitorTopics[0])] = true;
                break;
            }
            thisMonitor++;
        }
    }

    thisMonitor = &monitorTopics[0];
    i = 0;
    while(*thisMonitor)
    {
        TEST_ASSERT_EQUAL(monitorFound[i], true);
        i++;
        thisMonitor++;
    }

    ism_engine_freeTopicMonitor(topicMonitorResults);

    // Query again, but filtering
    ism_field_t f;
    ism_prop_t *filterProps = ism_common_newProperties(2);

    f.type = VT_String;
    f.val.s = "*A*";
    ism_common_setProperty(filterProps, ismENGINE_MONITOR_FILTER_TOPICSTRING, &f);

    topicMonitorResults = NULL;
    resultCount = 0;
    rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                    &resultCount,
                                    ismENGINE_MONITOR_ALL_UNSORTED,
                                    0,
                                    filterProps);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 3);

    // Check we found all the topics expected
    memset(&monitorFound, 0, sizeof(monitorFound));
    for(i=0; i<resultCount; i++)
    {
        thisMonitor = &monitorTopics[0];

        while(*thisMonitor)
        {
            if (strcmp(topicMonitorResults[i].topicString, *thisMonitor) == 0)
            {
                monitorFound[(int)(thisMonitor-&monitorTopics[0])] = true;
                break;
            }
            thisMonitor++;
        }
    }

    TEST_ASSERT_EQUAL(monitorFound[0], true);
    TEST_ASSERT_EQUAL(monitorFound[1], false);
    TEST_ASSERT_EQUAL(monitorFound[2], true);
    TEST_ASSERT_EQUAL(monitorFound[3], false);
    TEST_ASSERT_EQUAL(monitorFound[4], true);

    ism_engine_freeTopicMonitor(topicMonitorResults);

    // Filter resulting in 0 results
    f.type = VT_String;
    f.val.s = "*NOTTHERE*";
    ism_common_setProperty(filterProps, ismENGINE_MONITOR_FILTER_TOPICSTRING, &f);

    topicMonitorResults = NULL;
    resultCount = 0;
    rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                    &resultCount,
                                    ismENGINE_MONITOR_ALL_UNSORTED,
                                    0,
                                    filterProps);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);
    TEST_ASSERT_PTR_NULL(topicMonitorResults);

    // Stop the first topic monitor by emulating a config callback from the admin
    // component (thereby testing more of the engine).
    ism_prop_t *monitorProps = ism_common_newProperties(2);
    char topicMonitorName[strlen(ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING)+strlen(monitorTopics[0])+1];

    f.type = VT_String;
    f.val.s = monitorTopics[0];
    strcpy(topicMonitorName, ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING);
    strcat(topicMonitorName, monitorTopics[0]);

    // Try once without specifying a topic string!
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_TOPICMONITOR,
                                   monitorTopics[0],
                                   monitorProps,
                                   ISM_CONFIG_CHANGE_DELETE);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    // Now with an invalid change type
    ism_common_setProperty(monitorProps, topicMonitorName, &f);
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_TOPICMONITOR,
                                   monitorTopics[0],
                                   monitorProps,
                                   ISM_CONFIG_CHANGE_NAME);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);

    // Now correctly
    rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_TOPICMONITOR,
                                   monitorTopics[0],
                                   monitorProps,
                                   ISM_CONFIG_CHANGE_DELETE);
    TEST_ASSERT_EQUAL(rc, OK);

    //rc = ism_engine_stopTopicMonitor(monitorTopics[0]);
    //TEST_ASSERT_EQUAL(rc, OK);

    // Check that we only get active monitors
    topicMonitorResults = NULL;
    resultCount = 0;
    rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                    &resultCount,
                                    ismENGINE_MONITOR_ALL_UNSORTED,
                                    1000,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 4);

    for(i=0; i<resultCount; i++)
    {
        TEST_ASSERT_NOT_EQUAL(strcmp(topicMonitorResults[i].topicString, monitorTopics[0]), 0);
    }

    ism_engine_freeTopicMonitor(topicMonitorResults);

    // Re-enable and recheck
    rc = ism_engine_startTopicMonitor(monitorTopics[0], true);
    TEST_ASSERT_EQUAL(rc, OK);

    topicMonitorResults = NULL;
    resultCount = 0;
    rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                    &resultCount,
                                    ismENGINE_MONITOR_ALL_UNSORTED,
                                    1000,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 5);

    ism_engine_freeTopicMonitor(topicMonitorResults);
}

CU_TestInfo ISM_TopicMonitor_CUnit_test_allUnsorted[] =
{
    { "AllUnsorted", test_AllUnsorted },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test Operation of getTopicMonitor with various monitor types
//****************************************************************************
void test_MonitorTypes(void)
{
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    ismEngine_TopicMonitor_t *topicMonitorResults = NULL;
    uint32_t resultCount = 0;

    // Try an invalid type
    rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                    &resultCount,
                                    ismENGINE_MONITOR_NONE,
                                    1,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);
    ism_engine_freeTopicMonitor(topicMonitorResults);

    // Try an invalid maxResults
    rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                    &resultCount,
                                    ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);
    ism_engine_freeTopicMonitor(topicMonitorResults);

    // Now test valid types
    ismEngineMonitorType_t testTypes[] = {ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                          ismENGINE_MONITOR_LOWEST_PUBLISHEDMSGS,
                                          ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS,
                                          ismENGINE_MONITOR_LOWEST_SUBSCRIPTIONS,
                                          ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS,
                                          ismENGINE_MONITOR_LOWEST_REJECTEDMSGS,
                                          ismENGINE_MONITOR_HIGHEST_FAILEDPUBLISHES,
                                          ismENGINE_MONITOR_LOWEST_FAILEDPUBLISHES};

    // Start with no topic monitors
    int32_t type=0;
    for(type=0; type < (sizeof(testTypes)/sizeof(testTypes[0])); type++)
    {
        rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                        &resultCount,
                                        testTypes[type],
                                        10,
                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, 0);
        ism_engine_freeTopicMonitor(topicMonitorResults);
    }

    // Create 5 topic monitors
    char *monitorTopics[6] = {"1/2/#", "2/3/#", "3/4/#", "4/5/#", "5/6/#", NULL };
    char **thisMonitor = &monitorTopics[0];

    while(*thisMonitor)
    {
        ism_field_t f;
        ism_prop_t *monitorProps = ism_common_newProperties(2);
        char topicMonitorName[strlen(ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING)+strlen(*thisMonitor)+1];

        f.type = VT_String;
        f.val.s = *thisMonitor;
        strcpy(topicMonitorName, ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING);
        strcat(topicMonitorName, *thisMonitor);
        ism_common_setProperty(monitorProps, topicMonitorName, &f);

        // Start a topic monitor by emulating a config callback from the admin
        // component (thereby testing more of the engine).
        rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_TOPICMONITOR,
                                       *thisMonitor,
                                       monitorProps,
                                       ISM_CONFIG_CHANGE_PROPS);
        TEST_ASSERT_EQUAL(rc, OK);

        // Publish a small number of messages on that topic
        char *pubTopic = strdup(*thisMonitor);
        char *lastSlash = strrchr(pubTopic, '/');
        TEST_ASSERT_PTR_NOT_NULL(lastSlash);
        *lastSlash = '\0';
        for(uint32_t x=rand()%30; x>0; x--)
        {
            void *payload=*thisMonitor;
            ismEngine_MessageHandle_t message;

            rc = test_createMessage(strlen((char *)payload)+1,
                                    ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                    ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                    ismMESSAGE_FLAGS_NONE,
                                    0,
                                    ismDESTINATION_TOPIC, pubTopic,
                                    &message, &payload);
            TEST_ASSERT_EQUAL(rc, OK);

            // Try and put one message - this initializes this thread's cache
            rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                            pubTopic,
                                                            message,
                                                            NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        free(pubTopic);

        thisMonitor++;
    }

    // Query 2 results each type
    type=0;
    for(type=0; type < (sizeof(testTypes)/sizeof(testTypes[0])); type++)
    {
        rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                        &resultCount,
                                        testTypes[type],
                                        2,
                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, 2);
        ism_engine_freeTopicMonitor(topicMonitorResults);
    }

    // Query again, but filtering
    ism_field_t f;
    ism_prop_t *filterProps = ism_common_newProperties(2);

    f.type = VT_String;
    f.val.s = "*/*";
    ism_common_setProperty(filterProps, ismENGINE_MONITOR_FILTER_TOPICSTRING, &f);

    type=0;
    for(type=0; type < (sizeof(testTypes)/sizeof(testTypes[0])); type++)
    {
        rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                        &resultCount,
                                        testTypes[type],
                                        2,
                                        filterProps);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, 2);
        ism_engine_freeTopicMonitor(topicMonitorResults);
    }

    // And again with a filter that matches none
    f.val.s = "Not/Gonna/Happen";
    ism_common_setProperty(filterProps, ismENGINE_MONITOR_FILTER_TOPICSTRING, &f);

    type=0;
    for(type=0; type < (sizeof(testTypes)/sizeof(testTypes[0])); type++)
    {
        rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                        &resultCount,
                                        testTypes[type],
                                        2,
                                        filterProps);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, 0);
        ism_engine_freeTopicMonitor(topicMonitorResults);
    }
}

//****************************************************************************
/// @brief Test Invalid options
//****************************************************************************
void test_InvalidOptions(void)
{
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    rc = ism_engine_startTopicMonitor("A/B/C", true);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidTopicMonitor);

    rc = ism_engine_stopTopicMonitor("A/B/C");
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidTopicMonitor);
}

CU_TestInfo ISM_TopicMonitor_CUnit_test_monitorTypes[] =
{
    { "MonitorTypes", test_MonitorTypes },
    { "InvalidOptions", test_InvalidOptions },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initTopicMonitor(void)
{
    return test_engineInit_DEFAULT;
}

int termTopicMonitor(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_TopicMonitor_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("Defects", initTopicMonitor, termTopicMonitor, ISM_TopicMonitor_CUnit_test_defects),
    IMA_TEST_SUITE("AllUnsorted", initTopicMonitor, termTopicMonitor, ISM_TopicMonitor_CUnit_test_allUnsorted),
    IMA_TEST_SUITE("MonitorTypes", initTopicMonitor, termTopicMonitor, ISM_TopicMonitor_CUnit_test_monitorTypes),
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

    retval = setup_CU_registry(argc, argv, ISM_TopicMonitor_CUnit_allsuites);

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
