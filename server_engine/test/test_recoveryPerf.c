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
/* Module Name: test_recoveryPerf.c                                  */
/*                                                                   */
/* Description: Main source file for CUnit test of recovery perf     */
/*                                                                   */
/*********************************************************************/
#include <math.h>
#include <errno.h>
#include <stdbool.h>
#include <engineStore.h>

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_log.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_sync.h"

#define MAX_ARGS 15

#define UNRELATED_SUB_COUNT          200000
#define FILL_GENERATION_COUNT        600
#define RETAINED_MSGS_PER_GENERATION 2000
#define FILL_DATA_MSG_SIZE           (256*1024)

static char *fillData = NULL;

int32_t test_fillGeneration(ismStore_GenId_t *genId)
{
    int32_t rc = OK;

    if (fillData == NULL)
    {
        fillData = malloc(FILL_DATA_MSG_SIZE);
        TEST_ASSERT_PTR_NOT_NULL(fillData);

        memset(fillData, 'F', FILL_DATA_MSG_SIZE);
    }

    ismStore_Record_t storeRecord;

    char *Frags[] = {fillData};
    uint32_t FragsLengths[] = {FILL_DATA_MSG_SIZE};

    storeRecord.Type = ISM_STORE_RECTYPE_MSG;
    storeRecord.FragsCount = 1;
    storeRecord.pFrags = Frags;
    storeRecord.pFragsLengths = FragsLengths;
    storeRecord.DataLength = FILL_DATA_MSG_SIZE;
    storeRecord.Attribute = 0; // Unused
    storeRecord.State = 0; // Unused

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    uint32_t storeHandleCount = 0;
    uint32_t storeHandleCapacity = 0;
    ismStore_Handle_t *storeHandles = NULL;
    ismStore_GenId_t MSGGenId;

    while(1)
    {
        if (storeHandleCount == storeHandleCapacity)
        {
            uint32_t newStoreHandleCapacity = storeHandleCapacity + 4096;

            storeHandles = realloc(storeHandles, newStoreHandleCapacity * sizeof(ismStore_Handle_t));
            TEST_ASSERT_PTR_NOT_NULL(storeHandles);

            storeHandleCapacity = newStoreHandleCapacity;
        }

        rc = ism_store_createRecord( pThreadData->hStream
                                   , &storeRecord
                                   , &storeHandles[storeHandleCount]);

        if (rc == ISMRC_StoreGenerationFull || (++storeHandleCount)%1000 == 0)
        {
            iest_store_commit(pThreadData, false);
        }

        rc = ism_store_getGenIdOfHandle(storeHandles[storeHandleCount-1], &MSGGenId);
        TEST_ASSERT_EQUAL(rc, OK);

        // We've moved to a new generation, so now delete all that garbage we wrote
        if (MSGGenId > *genId)
        {
            iest_store_commit(pThreadData, false);

            test_log(testLOGLEVEL_TESTPROGRESS, "Moved from %hu to %hu with %u records",
                     *genId, MSGGenId, storeHandleCount);

            for(uint32_t i=0; i<storeHandleCount; i++)
            {
                rc = ism_store_deleteRecord(pThreadData->hStream, storeHandles[i]);
                TEST_ASSERT_EQUAL(rc, OK);

                if (i>0 && (i%1000 == 0))
                {
                    iest_store_commit(pThreadData, false);
                }
            }

            iest_store_commit(pThreadData, false);
            free(storeHandles);
            break;
        }
    }

    TEST_ASSERT_NOT_EQUAL(*genId, MSGGenId);

    return rc;
}

void test_capability_RetainedFill(void)
{
    int32_t rc;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    ismEngine_ClientState_t *hClient;
    ismEngine_Session_t *hSession;

    rc = test_createClientAndSession("FillingClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    ismStore_GenId_t StartGenId = 0;

    char subName[100];
    char topic[100];
    char payloadBuffer[1024] = {'X'};
    char *payload = payloadBuffer;

    TEST_ASSERT_NOT_EQUAL(RETAINED_MSGS_PER_GENERATION, 0);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_DURABLE };

    double startTime = ism_common_readTSC();
    for(uint32_t i=0; i<UNRELATED_SUB_COUNT; i++)
    {
        sprintf(subName, "SUB0x%08x", i);
        sprintf(topic, "/sub/T0x%08x", i);

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_createSubscription(hClient,
                                           subName,
                                           NULL,
                                           ismDESTINATION_TOPIC,
                                           topic,
                                           &subAttrs,
                                           NULL,
                                           &pActionsRemaining,
                                           sizeof(pActionsRemaining),
                                           test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

        if (i == UNRELATED_SUB_COUNT-1 || actionsRemaining > 1000)
        {
            test_waitForRemainingActions(pActionsRemaining);
        }

        if (i%1000 == 0)
        {
            test_log(testLOGLEVEL_TESTPROGRESS, "Created %u/%u of durable subscriptions", i, UNRELATED_SUB_COUNT);
        }
    }
    double elapsedTime = ism_common_readTSC()-startTime;

    test_log(testLOGLEVEL_TESTNAME, "Time to create %u durable subscriptions was %.3f seconds",
             UNRELATED_SUB_COUNT, elapsedTime);

    ismEngine_Message_t *hMessage = NULL;

    // Write all of the retained msgs once...
    uint32_t totalRetainedMsgs = (FILL_GENERATION_COUNT * RETAINED_MSGS_PER_GENERATION);

    startTime = ism_common_readTSC();
    for(uint32_t i=0; i<totalRetainedMsgs; i++)
    {
        sprintf(topic, "/ret/T0x%08x", i);

        rc = test_createMessage(sizeof(payloadBuffer),
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC,
                                topic,
                                &hMessage, (void **)&payload);
        TEST_ASSERT_EQUAL(rc, OK);

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topic,
                                                NULL,
                                                hMessage,
                                                &pActionsRemaining,
                                                sizeof(pActionsRemaining),
                                                test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

        if (i == totalRetainedMsgs-1 || actionsRemaining > 1000)
        {
            test_waitForRemainingActions(pActionsRemaining);
        }

        if (i%1000 == 0)
        {
            test_log(testLOGLEVEL_TESTPROGRESS, "Written %u/%u of retained messages", i, totalRetainedMsgs);
        }
    }
    elapsedTime = ism_common_readTSC()-startTime;

    test_log(testLOGLEVEL_TESTNAME, "Time to create %u retained messages was %.3f seconds",
             totalRetainedMsgs, elapsedTime);

    TEST_ASSERT_PTR_NOT_NULL(hMessage);

    rc = ism_store_getGenIdOfHandle(hMessage->StoreMsg.parts.hStoreMsg, &StartGenId);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(StartGenId, 0);

    ismStore_GenId_t NowGenId = StartGenId;

    uint32_t loop = 0;
    do
    {
        hMessage = NULL;

        // test_incrementActionsRemaining(pActionsRemaining, RETAINED_MSGS_PER_GENERATION);
        uint32_t startMsg = (loop*RETAINED_MSGS_PER_GENERATION);
        uint32_t endMsg = startMsg+RETAINED_MSGS_PER_GENERATION;
        for(uint32_t i = startMsg; i<endMsg; i++)
        {
            sprintf(topic, "/ret/T0x%08x", i);

            rc = test_createMessage(sizeof(payloadBuffer),
                                    ismMESSAGE_PERSISTENCE_PERSISTENT,
                                    ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                    ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                    0,
                                    ismDESTINATION_TOPIC,
                                    topic,
                                    &hMessage, (void **)&payload);
            TEST_ASSERT_EQUAL(rc, OK);

            test_incrementActionsRemaining(pActionsRemaining, 1);
            rc = ism_engine_putMessageOnDestination(hSession,
                                                    ismDESTINATION_TOPIC,
                                                    topic,
                                                    NULL,
                                                    hMessage,
                                                    &pActionsRemaining,
                                                    sizeof(pActionsRemaining),
                                                    test_decrementActionsRemaining);
            if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

            if (i == endMsg-1 || actionsRemaining > 1000)
            {
                test_waitForRemainingActions(pActionsRemaining);
            }
        }

        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = ism_store_getGenIdOfHandle(hMessage->StoreMsg.parts.hStoreMsg, &NowGenId);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_NOT_EQUAL(NowGenId, 0);

        rc = test_fillGeneration(&NowGenId);
        TEST_ASSERT_EQUAL(rc, OK);

        loop++;

        if (loop%10 == 0)
        {
            test_log(testLOGLEVEL_TESTNAME, "Filled %u/%hu generations", loop, FILL_GENERATION_COUNT);
        }
    }
    while(NowGenId < (StartGenId + FILL_GENERATION_COUNT));

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

void displayStoreHandles(void)
{
    ismStore_Handle_t handles[] = {9007200258364416,
                                   9007200307054592,
                                   9007199280330752,
                                   9007200488255488,
                                   9007203351894016,
                                   9007200029166592,
                                   9007200559093760,
                                   9007201736889344,
                                   0};

    int32_t count = 0;
    while(handles[count] != 0)
    {
        ismStore_GenId_t MSGGenId;
        ismStore_Handle_t thisHandle = handles[count];
        int32_t rc = ism_store_getGenIdOfHandle(thisHandle, &MSGGenId);
        printf("HANDLE %d: %lu [0x%016lx] GEN: %hu (rc=%d)\n", count, thisHandle, thisHandle, MSGGenId, rc);
        count++;
    }
}

CU_TestInfo ISM_RecoveryPerf_CUnit_test_capability_PreRestart[] =
{
        { "StoreHandles", displayStoreHandles },
        { "RetainedFill",  test_capability_RetainedFill },
    CU_TEST_INFO_NULL
};

void test_capability_RetainedFillRestart(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);
}

CU_TestInfo ISM_RecoveryPerf_CUnit_test_capability_PostRestart[] =
{
    { "RetainedFillRestart", test_capability_RetainedFillRestart },
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

CU_SuiteInfo ISM_RecoveryPerf_CUnit_phase0suites[] =
{
    IMA_TEST_SUITE("PreRestart", initPhaseCold, termPhaseWarm, ISM_RecoveryPerf_CUnit_test_capability_PreRestart),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_RecoveryPerf_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("PostRestart", initPhaseWarm, termPhaseCold, ISM_RecoveryPerf_CUnit_test_capability_PostRestart),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_RecoveryPerf_CUnit_phase0suites
                              , ISM_RecoveryPerf_CUnit_phase1suites };

int main(int argc, char *argv[])
{
    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
