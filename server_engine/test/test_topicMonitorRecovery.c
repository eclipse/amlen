/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
/* Module Name: test_topicMonitorRecovery.c                          */
/*                                                                   */
/* Description: Main source file for testing recovery of topic mons  */
/*                                                                   */
/*********************************************************************/
#include <math.h>
#include <errno.h>
#include <stdbool.h>
#include <engineStore.h>

#include "clientState.h"
#include "clientStateInternal.h"
#include "clientStateExpiry.h"

#include "test_utils_phases.h"
#include "test_utils_file.h"
#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_log.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_sync.h"
#include "test_utils_config.h"
#include "test_utils_options.h"

#define MAX_ARGS 15

//****************************************************************************
/// @brief Test Topic monitor recovery
//****************************************************************************
void test_TopicMonitorRecovery_Phase1(void)
{
    uint32_t rc;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create 2 topic monitors
    rc = test_configProcessPost("{\"TopicMonitor\":[\"testTopic/#\", \"testOtherTopic/#\"]}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Check both monitors exist in the engine
    ismEngine_TopicMonitor_t *topicMonitorResults = NULL;
    uint32_t resultCount = 0;

    rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                    &resultCount,
                                    ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                    10,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 2);
    ism_engine_freeTopicMonitor(topicMonitorResults);
}

void test_TopicMonitorRecovery_Phase2(void)
{
    uint32_t rc;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Check both monitors exist in the engine
    ismEngine_TopicMonitor_t *topicMonitorResults = NULL;
    uint32_t resultCount = 0;

    // Check that both monitors still exist in the engine
    rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                    &resultCount,
                                    ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                    10,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 2);
    ism_engine_freeTopicMonitor(topicMonitorResults);

    // Delete one topic monitor
    rc = test_configProcessDelete("TopicMonitor", "testTopic/#", NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the explicitly deleted topicMonitor doesn't exist
    rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                    &resultCount,
                                    ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                    10,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 1);
    ism_engine_freeTopicMonitor(topicMonitorResults);
}

void test_TopicMonitorRecovery_Phase3(void)
{
    uint32_t rc;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Check both monitors exist in the engine
    ismEngine_TopicMonitor_t *topicMonitorResults = NULL;
    uint32_t resultCount = 0;

    // Check that the explicitly deleted topicMonitor doesn't return
    rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                    &resultCount,
                                    ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                    10,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 1);
    ism_engine_freeTopicMonitor(topicMonitorResults);

    // Check that a deletion attempt will result in not found now.
    rc = test_configProcessDelete("TopicMonitor", "testTopic/#", NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    rc = test_configProcessDelete("TopicMonitor", "testOtherTopic/#", NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                    &resultCount,
                                    ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                    10,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);
    ism_engine_freeTopicMonitor(topicMonitorResults);
}

CU_TestInfo ISM_TopicMonitorRecovery_CUnit_test_Phase1[] =
{
    { "TopicMonitorPhase1",  test_TopicMonitorRecovery_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicMonitorRecovery_CUnit_test_Phase2[] =
{
    { "TopicMonitorPhase2", test_TopicMonitorRecovery_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TopicMonitorRecovery_CUnit_test_Phase3[] =
{
    { "TopicMonitorPhase3", test_TopicMonitorRecovery_Phase3 },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initPhaseCold(void)
{
    return test_engineInit(true, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false,
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           1024);
}

int termPhaseCold(void)
{
    return test_engineTerm(true);
}

//For phases after phase 0
int initPhaseWarm(void)
{
    return test_engineInit(false, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false,
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           1024);
}
//For phases after phase 0
int termPhaseWarm(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_TopicMonitorRecovery_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("Rehydration", initPhaseCold, termPhaseWarm, ISM_TopicMonitorRecovery_CUnit_test_Phase1),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicMonitorRecovery_CUnit_phase2suites[] =
{
    IMA_TEST_SUITE("Rehydration", initPhaseWarm, termPhaseWarm, ISM_TopicMonitorRecovery_CUnit_test_Phase2),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TopicMonitorRecovery_CUnit_phase3suites[] =
{
    IMA_TEST_SUITE("Rehydration", initPhaseWarm, termPhaseCold, ISM_TopicMonitorRecovery_CUnit_test_Phase3),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_TopicMonitorRecovery_CUnit_phase1suites
                              , ISM_TopicMonitorRecovery_CUnit_phase2suites
                              , ISM_TopicMonitorRecovery_CUnit_phase3suites };

int main(int argc, char *argv[])
{
    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
