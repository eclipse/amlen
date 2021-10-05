/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
/* Module Name: test_engineTimers.c                                  */
/*                                                                   */
/* Description: Test the startup etc of the engine timer tasks       */
/*                                                                   */
/*********************************************************************/
#include "test_engineTimers.h"
#include "test_utils_assert.h"
#include "test_utils_initterm.h"
#include "engineTimers.h"

CU_TestInfo ISM_Engine_CUnit_Timers[] = {
    { "TimerStartup", test_timerStartup },
    { "DirectCall", test_directCall },
    CU_TEST_INFO_NULL
};

// Override the functions
bool override_ietm_updateServerTimestamp = false;
uint32_t ietm_updateServerTimestampCount = 0;
int ietm_updateServerTimestamp(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    static int (*real_ietm_updateServerTimestamp)(ism_timer_t, ism_time_t, void *) = NULL;

    if (real_ietm_updateServerTimestamp == NULL)
    {
        real_ietm_updateServerTimestamp = dlsym(RTLD_NEXT, __func__);
        TEST_ASSERT_PTR_NOT_NULL(real_ietm_updateServerTimestamp);
    }

    int rc;

    if (override_ietm_updateServerTimestamp)
    {
        TEST_ASSERT_NOT_EQUAL(key, 0);
        __sync_add_and_fetch(&ietm_updateServerTimestampCount, 1);
        rc = 0;
    }
    else
    {
        rc = real_ietm_updateServerTimestamp(key, timestamp, userdata);
    }

    return rc;
}

bool override_ietm_updateRetMinActiveOrderId = false;
uint32_t ietm_updateRetMinActiveOrderIdCount = 0;
int ietm_updateRetMinActiveOrderId(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    static int (*real_ietm_updateRetMinActiveOrderId)(ism_timer_t, ism_time_t, void *) = NULL;

    if (real_ietm_updateRetMinActiveOrderId == NULL)
    {
        real_ietm_updateRetMinActiveOrderId = dlsym(RTLD_NEXT, __func__);
        TEST_ASSERT_PTR_NOT_NULL(real_ietm_updateRetMinActiveOrderId);
    }

    int rc;

    if (override_ietm_updateRetMinActiveOrderId)
    {
        TEST_ASSERT_NOT_EQUAL(key, 0);
        __sync_add_and_fetch(&ietm_updateRetMinActiveOrderIdCount, 1);
        rc = 0;
    }
    else
    {
        rc = real_ietm_updateRetMinActiveOrderId(key, timestamp, userdata);
    }

    return rc;
}

bool override_ietm_syncClusterRetained = false;
uint32_t ietm_syncClusterRetainedCount = 0;
int ietm_syncClusterRetained(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    static int (*real_ietm_syncClusterRetained)(ism_timer_t, ism_time_t, void *) = NULL;

    if (real_ietm_syncClusterRetained == NULL)
    {
        real_ietm_syncClusterRetained = dlsym(RTLD_NEXT, __func__);
        TEST_ASSERT_PTR_NOT_NULL(real_ietm_syncClusterRetained);
    }

    int rc;

    if (override_ietm_syncClusterRetained)
    {
        TEST_ASSERT_NOT_EQUAL(key, 0);
        __sync_add_and_fetch(&ietm_syncClusterRetainedCount, 1);
        rc = 0;
    }
    else
    {
        rc = real_ietm_syncClusterRetained(key, timestamp, userdata);
    }

    return rc;
}

/*
 * Test the startup of the Timer functions
 */
void test_timerStartup(void)
{
    int32_t rc = OK;

    uint32_t initial_ServerTimestampInterval = ismEngine_serverGlobal.ServerTimestampInterval;
    uint32_t initial_ietm_updateServerTimestampCount = ietm_updateServerTimestampCount;

    uint32_t initial_RetMinActiveOrderIdInterval = ismEngine_serverGlobal.RetMinActiveOrderIdInterval;
    uint32_t initial_ietm_updateRetMinActiveOrderIdCount = ietm_updateRetMinActiveOrderIdCount;

    uint32_t initial_ClusterRetainedSyncInterval = ismEngine_serverGlobal.ClusterRetainedSyncInterval;
    uint32_t initial_ietm_syncClusterRetainedCount = ietm_syncClusterRetainedCount;

    // Try setting up the timers
    ismEngine_serverGlobal.ServerTimestampInterval = 1;
    ismEngine_serverGlobal.RetMinActiveOrderIdInterval = 1;
    ismEngine_serverGlobal.ClusterRetainedSyncInterval = 1;

    override_ietm_updateServerTimestamp = true;
    override_ietm_updateRetMinActiveOrderId = true;
    override_ietm_syncClusterRetained = true;

    rc = ietm_setUpTimers();
    TEST_ASSERT_EQUAL(rc, OK);

    sleep(2);

    TEST_ASSERT_EQUAL(ietm_updateServerTimestampCount, initial_ietm_updateServerTimestampCount+1);
    TEST_ASSERT_EQUAL(ietm_updateRetMinActiveOrderIdCount, initial_ietm_updateRetMinActiveOrderIdCount+1);
    TEST_ASSERT_EQUAL(ietm_syncClusterRetainedCount, initial_ietm_syncClusterRetainedCount+1);

    override_ietm_updateServerTimestamp = false;
    override_ietm_updateRetMinActiveOrderId = false;
    override_ietm_syncClusterRetained = false;

    ietm_cleanUpTimers();

    ismEngine_serverGlobal.ServerTimestampInterval = initial_ServerTimestampInterval;
    ismEngine_serverGlobal.RetMinActiveOrderIdInterval = initial_RetMinActiveOrderIdInterval;
    ismEngine_serverGlobal.ClusterRetainedSyncInterval = initial_ClusterRetainedSyncInterval;
}

/*
 * Call the timer task functions directly in different active states
 */
void test_directCall(void)
{
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.ActiveTimerUseCount, 0);
    ismEngine_serverGlobal.ActiveTimerUseCount = 1;
    uint32_t knownServerTimestamp = ismEngine_serverGlobal.ServerTimestamp;

    int runAgain = ietm_updateServerTimestamp(NULL, 0, NULL);
    // Try again immediately so that, if async is turned on we'll hit the
    // already running logic.
    int runAgain2 = ietm_updateServerTimestamp(NULL, 0, NULL);

    TEST_ASSERT_EQUAL(runAgain, 1);
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.ActiveTimerUseCount, 1);
    TEST_ASSERT_EQUAL(runAgain2, 1);

    // Wait for the timestamp to be updated
    while((volatile uint32_t)ismEngine_serverGlobal.ServerTimestamp == knownServerTimestamp) sched_yield();
    knownServerTimestamp = ismEngine_serverGlobal.ServerTimestamp;

    // Wait for a new expiry time
    while(knownServerTimestamp == ism_common_nowExpire()) sched_yield();

    // Run again and wait for it to be updated again
    runAgain = ietm_updateServerTimestamp(NULL, 0, NULL);
    TEST_ASSERT_EQUAL(runAgain, 1);
    while((volatile uint32_t)ismEngine_serverGlobal.ServerTimestamp == knownServerTimestamp) sched_yield();

    runAgain = ietm_updateRetMinActiveOrderId(NULL, 0, NULL);
    TEST_ASSERT_EQUAL(runAgain, 1);
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.ActiveTimerUseCount, 1);

    runAgain = ietm_syncClusterRetained(NULL, 0, NULL);
    TEST_ASSERT_EQUAL(runAgain, 1);
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.ActiveTimerUseCount, 1);

    ismEngine_serverGlobal.ActiveTimerUseCount = 0;

    runAgain = ietm_updateServerTimestamp(NULL, 0, NULL);
    TEST_ASSERT_EQUAL(runAgain, 0);
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.ActiveTimerUseCount, 0);

    runAgain = ietm_updateRetMinActiveOrderId(NULL, 0, NULL);
    TEST_ASSERT_EQUAL(runAgain, 0);
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.ActiveTimerUseCount, 0);

    runAgain = ietm_syncClusterRetained(NULL, 0, NULL);
    TEST_ASSERT_EQUAL(runAgain, 0);
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.ActiveTimerUseCount, 0);

    ietm_cleanUpTimers();
}
