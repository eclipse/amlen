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
/* Module Name: test_clusterRequestedTopicRecovery.c                 */
/*                                                                   */
/* Description: Main source file for testing recovery of cluster     */
/*              requested topics.                                    */
/*                                                                   */
/*********************************************************************/
#include <math.h>
#include <errno.h>
#include <stdbool.h>
#include <engineStore.h>

#include "topicTree.h"
#include "topicTreeInternal.h"

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_config.h"
#include "test_utils_options.h"

void test_checkActiveCluster(const char *topicString, uint32_t value)
{
    iettTopic_t  topic = {0};
    const char  *substrings[10];
    uint32_t     substringHashes[10];
    const char  *wildcards[10];
    const char  *multicards[10];

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = topicString;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = 10;

    /*****************************************************************/
    /* Need to get an analysis of the topic string to use when       */
    /* calling functions using a topic string on the topic tree.     */
    /*****************************************************************/
    int32_t rc = iett_analyseTopicString(pThreadData, &topic);
    TEST_ASSERT_EQUAL(rc, OK);

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;
    TEST_ASSERT_PTR_NOT_NULL(tree);

    iettSubsNode_t *node = NULL;

    rc = iett_insertOrFindSubsNode(pThreadData, tree->subs, &topic, iettOP_FIND, &node);

    if (value == 0)
    {
        if (rc != OK) TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }
    else
    {
        TEST_ASSERT_EQUAL(rc, OK);
    }

    if (rc == OK)
    {
        TEST_ASSERT_PTR_NOT_NULL(node);
        TEST_ASSERT_EQUAL(node->activeCluster, value);
    }

    iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);
}

//****************************************************************************
/// @brief Test Cluster Requested Topics recovery
//****************************************************************************
void test_ClusterRequestedTopicsRecovery_Phase1(void)
{
    uint32_t rc;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create cluster requested topics
    rc = test_configProcessPost("{\"ClusterRequestedTopics\":[\"crTopic/1\", \"crTopic/+/X\", \"crTopic/2/Y/#\"]}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the nodes now exist in the topic tree and have ActiveCluster set
    test_checkActiveCluster("crTopic/1", 1);
    test_checkActiveCluster("crTopic/+/X", 1);
    test_checkActiveCluster("crTopic/2/Y/#", 1);
}

void test_ClusterRequestedTopicsRecovery_Phase2(void)
{
    uint32_t rc;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Check the nodes still exist in the topic tree and have ActiveCluster set
    test_checkActiveCluster("crTopic/1", 1);
    test_checkActiveCluster("crTopic/+/X", 1);
    test_checkActiveCluster("crTopic/2/Y/#", 1);

    // Delete one cluster requested topic
    rc = test_configProcessDelete("ClusterRequestedTopics", "crTopic/1", NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_ClusterRequestedTopicsRecovery_Phase3(void)
{
    uint32_t rc;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Check the nodes still exist in the topic tree and have ActiveCluster set
    test_checkActiveCluster("crTopic/1", 0);
    test_checkActiveCluster("crTopic/+/X", 1);
    test_checkActiveCluster("crTopic/2/Y/#", 1);

    // Check that a deletion attempt will result in not found now.
    rc = test_configProcessDelete("ClusterRequestedTopics", "crTopic/1", NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    rc = test_configProcessDelete("ClusterRequestedTopics", "crTopic/+/X", NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_configProcessDelete("ClusterRequestedTopics", "crTopic/2/Y/#", NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_ClusterRequestedTopicsRecovery_Phase4(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Check no nodes still exist in the topic tree
    test_checkActiveCluster("crTopic/1", 0);
    test_checkActiveCluster("crTopic/+/X", 0);
    test_checkActiveCluster("crTopic/2/Y/#", 0);
}

CU_TestInfo ISM_ClusterRequestedTopicsRecovery_CUnit_test_Phase1[] =
{
    { "ClusterRequestedTopicsPhase1",  test_ClusterRequestedTopicsRecovery_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_ClusterRequestedTopicsRecovery_CUnit_test_Phase2[] =
{
    { "ClusterRequestedTopicsPhase2", test_ClusterRequestedTopicsRecovery_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_ClusterRequestedTopicsRecovery_CUnit_test_Phase3[] =
{
    { "ClusterRequestedTopicsPhase3", test_ClusterRequestedTopicsRecovery_Phase3 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_ClusterRequestedTopicsRecovery_CUnit_test_Phase4[] =
{
    { "ClusterRequestedTopicsPhase4", test_ClusterRequestedTopicsRecovery_Phase4 },
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

CU_SuiteInfo ISM_ClusterRequestedTopicsRecovery_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("Rehydration", initPhaseCold, termPhaseWarm, ISM_ClusterRequestedTopicsRecovery_CUnit_test_Phase1),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_ClusterRequestedTopicsRecovery_CUnit_phase2suites[] =
{
    IMA_TEST_SUITE("Rehydration", initPhaseWarm, termPhaseWarm, ISM_ClusterRequestedTopicsRecovery_CUnit_test_Phase2),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_ClusterRequestedTopicsRecovery_CUnit_phase3suites[] =
{
    IMA_TEST_SUITE("Rehydration", initPhaseWarm, termPhaseCold, ISM_ClusterRequestedTopicsRecovery_CUnit_test_Phase3),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_ClusterRequestedTopicsRecovery_CUnit_phase4suites[] =
{
    IMA_TEST_SUITE("Rehydration", initPhaseWarm, termPhaseCold, ISM_ClusterRequestedTopicsRecovery_CUnit_test_Phase4),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_ClusterRequestedTopicsRecovery_CUnit_phase1suites
                              , ISM_ClusterRequestedTopicsRecovery_CUnit_phase2suites
                              , ISM_ClusterRequestedTopicsRecovery_CUnit_phase3suites
                              , ISM_ClusterRequestedTopicsRecovery_CUnit_phase4suites };

int main(int argc, char *argv[])
{
    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
