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
/* Module Name: test_clientStateExpiry.c                             */
/*                                                                   */
/* Description: Source file for CUnit test of ClientState Expiry     */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "clientState.h"
#include "clientStateExpiryInternal.h"
#include "clientStateInternal.h"
#include "engineMonitoring.h"

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_sync.h"
#include "test_utils_security.h"

#define MAX_ARGS 15

#define USE_NONDURABLE_CLIENTSTATES
#define EXPECTED_FIRST_TENSECOND_EXPIRY 10

bool fullTest = false;
uint32_t testNumber = 0;

// NOTE: Need to override a bunch of clientState.c functions to ensure that, in the absence of a security
//       context we can set an expiryInterval that will be persisted.

uint32_t override_clientStateExpiryInterval = UINT32_MAX;
int32_t iecs_addClientState(ieutThreadData_t *pThreadData,
                            ismEngine_ClientState_t *pClient,
                            uint32_t options,
                            uint32_t internalOptions,
                            void *pContext,
                            size_t contextLength,
                            ismEngine_CompletionCallback_t pCallbackFn)
{
    int32_t rc;
    static int32_t (*real_iecs_addClientState)(ieutThreadData_t *, ismEngine_ClientState_t *, uint32_t, uint32_t, void *, size_t, ismEngine_CompletionCallback_t) = NULL;

    if (real_iecs_addClientState == NULL)
    {
        real_iecs_addClientState = dlsym(RTLD_NEXT, "iecs_addClientState");
    }

    rc = real_iecs_addClientState(pThreadData, pClient, options, internalOptions, pContext, contextLength, pCallbackFn);

    // Horridly simplistic way to override the expiry interval (which works because
    // nothing currently takes any notice of the value until the clientState is destroyed).
    if (override_clientStateExpiryInterval != UINT32_MAX)
    {
        pClient->ExpiryInterval = override_clientStateExpiryInterval;
    }

    return rc;
}

int32_t iecs_storeClientState(ieutThreadData_t *pThreadData,
                              ismEngine_ClientState_t *pClient,
                              bool fFromImport,
                              ismEngine_AsyncData_t *pAsyncData)
{
    static int32_t (*real_iecs_storeClientState)(ieutThreadData_t *, ismEngine_ClientState_t *, bool, ismEngine_AsyncData_t *) = NULL;

    if (real_iecs_storeClientState == NULL)
    {
        real_iecs_storeClientState = dlsym(RTLD_NEXT, "iecs_storeClientState");
    }

    // Want to make sure we override the ExpiryInterval earlier for durable clients so that the
    // CPR contains the expiry interval.
    if (override_clientStateExpiryInterval != UINT32_MAX &&
        pClient->ExpiryInterval != override_clientStateExpiryInterval)
    {
        pClient->ExpiryInterval = override_clientStateExpiryInterval;
    }

    return real_iecs_storeClientState(pThreadData, pClient, fFromImport, pAsyncData);
}

int32_t iecs_updateClientPropsRecord(ieutThreadData_t *pThreadData,
                                     ismEngine_ClientState_t *pClient,
                                     char *willTopicName,
                                     ismStore_Handle_t willMsgStoreHdl,
                                     uint32_t willMsgTimeToLive,
                                     uint32_t willDelay)
{
    static int32_t (*real_iecs_updateClientPropsRecord)(ieutThreadData_t *,
                                                        ismEngine_ClientState_t *,
                                                        char *,
                                                        ismStore_Handle_t,
                                                        uint32_t,
                                                        uint32_t) = NULL;

    if (real_iecs_updateClientPropsRecord == NULL)
    {
        real_iecs_updateClientPropsRecord = dlsym(RTLD_NEXT, "iecs_updateClientPropsRecord");
    }

    // Want to make sure we override the ExpiryInterval earlier for durable clients so that the
    // CPR contains the expiry interval.
    if (override_clientStateExpiryInterval != UINT32_MAX &&
        pClient->ExpiryInterval != override_clientStateExpiryInterval)
    {
        pClient->ExpiryInterval = override_clientStateExpiryInterval;
    }

    return real_iecs_updateClientPropsRecord(pThreadData, pClient, willTopicName, willMsgStoreHdl, willMsgTimeToLive, willDelay);
}

// Override iecs_traverseClientStateTable with a mechanism to fake a generation mismatch
uint32_t fake_clientTableGenMismatch = 0;
uint32_t clientTableLoopsSeen = 0;
int32_t iecs_traverseClientStateTable(ieutThreadData_t *pThreadData,
                                      uint32_t *tableGeneration,
                                      uint32_t startChain,
                                      uint32_t maxChains,
                                      uint32_t *lastChain,
                                      iecsTraverseCallback_t callback,
                                      void *context)
{
    static int32_t (*real_iecs_traverseClientStateTable)(ieutThreadData_t *,
                                                         uint32_t *,
                                                         uint32_t,
                                                         uint32_t,
                                                         uint32_t *,
                                                         iecsTraverseCallback_t,
                                                         void *) = NULL;

    if (real_iecs_traverseClientStateTable == NULL)
    {
        real_iecs_traverseClientStateTable = dlsym(RTLD_NEXT, "iecs_traverseClientStateTable");
    }

    if (fake_clientTableGenMismatch != 0)
    {
        if (startChain == 0)
        {
            clientTableLoopsSeen = 0;
        }
        else
        {
            clientTableLoopsSeen += 1;

            if (fake_clientTableGenMismatch == clientTableLoopsSeen)
            {
                fake_clientTableGenMismatch = 0;
                return ISMRC_ClientTableGenMismatch;
            }
        }
    }

    return real_iecs_traverseClientStateTable(pThreadData, tableGeneration, startChain, maxChains, lastChain, callback, context);
}

typedef struct tag_test_createClientStateCompleteContext_t
{
    ismEngine_ClientStateHandle_t *phClient;
    uint32_t **ppActionsRemaining;
} test_createClientStateCompleteContext_t;

static void test_createClientStateComplete(int32_t retcode,
                                           void *handle,
                                           void *pContext)
{
    test_createClientStateCompleteContext_t *context = (test_createClientStateCompleteContext_t *)pContext;
    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)handle;

#ifdef USE_NONDURABLE_CLIENTSTATES
    // Enable the test to run with non-durable clients by pretending they have durable objects
    // and so they must become zombies when destroyed... THIS IS HORRIBLE, but means we won't be
    // affected by persistence because we want to check the timings... did I mention it's HORRIBLE?
    if (pClient->Durability == iecsNonDurable)
    {
        pClient->durableObjects = 1;
    }
#else
    TEST_ASSERT_EQUAL(pClient->Durability, iecsDurable);
#endif

    if (retcode != OK)
    {
        TEST_ASSERT_EQUAL(retcode, ISMRC_ResumedClientState);
        TEST_ASSERT_EQUAL(pClient->Durability, iecsDurable);
    }

    *(context->phClient) = handle;

    test_decrementActionsRemaining(retcode, handle, context->ppActionsRemaining);

    return;
}

static void test_StealCallback( int32_t reason,
                                ismEngine_ClientStateHandle_t hClient,
                                uint32_t options,
                                void *pContext )
{
    TEST_ASSERT_EQUAL(reason, ISMRC_ResumedClientState);
    TEST_ASSERT_EQUAL(options, ismENGINE_STEAL_CALLBACK_OPTION_NONE);

    int32_t rc = ism_engine_destroyClientState(hClient,
                                               ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                               NULL, 0, NULL);

    if (rc != OK) TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);


    return;
}


#define CREATION_LOOPS       3
#define NON_EXPIRING_COUNT   10
#define TEN_SECOND_EXPIRERS  5
#define FIVE_SECOND_EXPIRERS (ieceMAX_CLIENTSEXPIRY_BATCH_SIZE*10)+10
#define DESTRUCTION_LOOPS    7
void test_capability_MixedClientStateExpiry_Phase1(void)
{
    uint32_t rc;
    uint32_t createdSoFar = 0;
    uint32_t totalCreatedAfterLoop;

    uint32_t actionsThisLoop;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;
    ismEngine_ClientStateHandle_t foundClient = NULL;
    ismEngine_ClientStateHandle_t *hClient = NULL;
    char clientId[50];

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    printf("Starting %s...\n", __func__);

    uint32_t firstNonExpiring = 0;
    uint32_t firstTenSecondExpiry = 0;
    uint32_t firstFiveSecondExpiry = 0;
    uint32_t lastFiveSecondExpiry = 0;

    hClient = malloc(sizeof(ismEngine_ClientStateHandle_t) * (NON_EXPIRING_COUNT  +
                                                              TEN_SECOND_EXPIRERS +
                                                              FIVE_SECOND_EXPIRERS));
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    uint32_t createOptions = ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL;

#ifndef USE_NONDURABLE_CLIENTSTATES
    createOptions |= ismENGINE_CREATE_CLIENT_OPTION_DURABLE;
#endif

    iemnClientStateStatistics_t stats;
    ismEngine_MessagingStatistics_t msgStats;

    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, 0);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, 0);
    ism_engine_getMessagingStatistics(&msgStats);
    TEST_ASSERT_NOT_EQUAL(msgStats.ServerShutdownTime, 0);
    TEST_ASSERT_EQUAL(msgStats.ExpiredClientStates, 0);

    // Create a bunch of clientStates with different expiry set
    for(int32_t i=0; i<CREATION_LOOPS; i++)
    {
        switch(i)
        {
            case 0:
                // 1) Add some non-expiring clientStates
                actionsThisLoop = NON_EXPIRING_COUNT;
                firstNonExpiring = createdSoFar;
                break;
            case 1:
                // 2) Add some 10 second expiring clientStates
                actionsThisLoop = TEN_SECOND_EXPIRERS;
                firstTenSecondExpiry = createdSoFar;
                TEST_ASSERT_EQUAL(firstTenSecondExpiry, EXPECTED_FIRST_TENSECOND_EXPIRY);
                override_clientStateExpiryInterval = 10;
                break;
            case 2:
                // 3) Add some 5 second expiring clientStates
                actionsThisLoop = FIVE_SECOND_EXPIRERS;
                firstFiveSecondExpiry = createdSoFar;
                lastFiveSecondExpiry = createdSoFar+FIVE_SECOND_EXPIRERS-1;
                override_clientStateExpiryInterval = 5;
                break;
            default:
                TEST_ASSERT(false, ("Unexpected case %d\n", i));
                break;
        }

        totalCreatedAfterLoop = createdSoFar+actionsThisLoop;
        test_incrementActionsRemaining(pActionsRemaining, actionsThisLoop);
        for(;createdSoFar < totalCreatedAfterLoop; createdSoFar++)
        {
            uint32_t useCreateOptions;

            test_createClientStateCompleteContext_t context = { &hClient[createdSoFar], &pActionsRemaining };

            // Make two specific ones durable (regardless of durability of test) for test of steal/takeover later.
            if (createdSoFar >= firstTenSecondExpiry+1 && createdSoFar <= firstTenSecondExpiry+2)
            {
                useCreateOptions = ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                   ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL;
            }
            else
            {
                useCreateOptions = createOptions;
            }

            sprintf(clientId, "%s%u", "ExpiryClient", createdSoFar);

            rc = ism_engine_createClientState(clientId,
                                              PROTOCOL_ID_MQTT,
                                              useCreateOptions,
                                              NULL, test_StealCallback,
                                              NULL,       // NO SECURITY CONTEXT (YET)
                                              &hClient[createdSoFar],
                                              &context,
                                              sizeof(context),
                                              test_createClientStateComplete);
            if (rc == OK || rc == ISMRC_ResumedClientState)
            {
                test_createClientStateComplete(rc, hClient[createdSoFar], &context);
            }
            else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        }

        test_waitForRemainingActions(pActionsRemaining);
    }

    ism_engine_getMessagingStatistics(&msgStats);
    TEST_ASSERT_NOT_EQUAL(msgStats.ServerShutdownTime, 0);
    TEST_ASSERT_EQUAL(msgStats.ClientStates, createdSoFar);

    TEST_ASSERT_NOT_EQUAL(firstTenSecondExpiry, 0);
    TEST_ASSERT_NOT_EQUAL(firstFiveSecondExpiry, 0);

    ieceExpiryControl_t *expiryControl = ismEngine_serverGlobal.clientStateExpiryControl;

    // Check stats (should now show none expired, and none with expiry set)
    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, 0);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, 0);

    ism_time_t prevScheduledScan = expiryControl->nextScheduledScan;

    // Destroy the clientStates in different orders
    uint64_t expectExpired = 0;
    uint64_t expectExpirySet = 0;
    for(int32_t i=0; i<DESTRUCTION_LOOPS; i++)
    {
        uint32_t destroyOptions = ismENGINE_DESTROY_CLIENT_OPTION_NONE;
        uint32_t firstClientNo;

        switch(i)
        {
            // 1) First non-expiring clientState
            case 0:
                // Check scan schedule is as expected
                TEST_ASSERT_EQUAL(expiryControl->nextScheduledScan, ieceNO_TIMED_SCAN_SCHEDULED);
                firstClientNo = firstNonExpiring;
                actionsThisLoop = 1;
                break;
            // 2) First 10 second expiry clientState
            case 1:
                // Check scan schedule is as expected
                TEST_ASSERT_EQUAL(expiryControl->nextScheduledScan, ieceNO_TIMED_SCAN_SCHEDULED);
                TEST_ASSERT_EQUAL(prevScheduledScan, expiryControl->nextScheduledScan);
                firstClientNo = firstTenSecondExpiry;
                actionsThisLoop = 1;
                expectExpirySet += actionsThisLoop;
                break;
             // 3) First 5 second expiry clientState
            case 2:
                // Check scan schedule is as expected (allow 1 second for the scheduled scan to change)
                for(int32_t allowRetries = 20;
                    expiryControl->nextScheduledScan == ieceNO_TIMED_SCAN_SCHEDULED;
                    allowRetries--)
                {
                    if (allowRetries == 0) break;
                    usleep(50000); // Let the code run
                }
                TEST_ASSERT_NOT_EQUAL(expiryControl->nextScheduledScan, ieceNO_TIMED_SCAN_SCHEDULED);
                firstClientNo = firstFiveSecondExpiry;
                actionsThisLoop = 1;
                expectExpirySet += actionsThisLoop;
                break;
            // 4) Second & third 5 second expiry client DISCARDING
            case 3:
                // Check scan schedule is as expected (allow 1 second for the scheduled scan to change)
               for(int32_t allowRetries = 20;
                    expiryControl->nextScheduledScan == prevScheduledScan;
                    allowRetries--)
                {
                    if (allowRetries == 0) break;
                    usleep(50000); // Let the code run
                }
                TEST_ASSERT_GREATER_THAN(prevScheduledScan, expiryControl->nextScheduledScan);
                firstClientNo = firstFiveSecondExpiry+1;
                destroyOptions = ismENGINE_DESTROY_CLIENT_OPTION_DISCARD;
                actionsThisLoop = 2;
                break;
            // 5) Bulk of the 10 second expiry clientStates
            case 4:
                // Check scan schedule is as expected
                TEST_ASSERT_EQUAL(prevScheduledScan, expiryControl->nextScheduledScan);
                firstClientNo = firstTenSecondExpiry+2;
                actionsThisLoop = TEN_SECOND_EXPIRERS-2;
                expectExpirySet += actionsThisLoop;
                break;
            // 6) Bulk of the non-expiring clientStates
            case 5:
                // Check scan schedule is as expected
                TEST_ASSERT_EQUAL(prevScheduledScan, expiryControl->nextScheduledScan);
                firstClientNo = firstNonExpiring+1;
                actionsThisLoop = NON_EXPIRING_COUNT-1;
                break;
            // 7) Bulk of the five second expiry clientStates (leaving the last one still connected)
            case 6:
                // Check scan schedule is as expected
                TEST_ASSERT_EQUAL(prevScheduledScan, expiryControl->nextScheduledScan);
                firstClientNo = firstFiveSecondExpiry+3;
                actionsThisLoop = FIVE_SECOND_EXPIRERS-4;
                expectExpirySet += actionsThisLoop;
                break;
            default:
                TEST_ASSERT(false, ("Unexpected case %d\n", i));
                break;
        }

        prevScheduledScan = expiryControl->nextScheduledScan;

        test_incrementActionsRemaining(pActionsRemaining, actionsThisLoop);

        for(uint32_t clientNo=firstClientNo; clientNo<firstClientNo+actionsThisLoop; clientNo++)
        {
            rc = ism_engine_destroyClientState(hClient[clientNo],
                                               destroyOptions,
                                               &pActionsRemaining,
                                               sizeof(pActionsRemaining),
                                               test_decrementActionsRemaining);
            if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        }

        test_waitForRemainingActions(pActionsRemaining);

        // Check stats
        iemn_getClientStateStatistics(pThreadData, &stats);
        TEST_ASSERT_EQUAL(stats.ExpiredClientStates, expectExpired);
        TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, expectExpirySet);
        ism_engine_getMessagingStatistics(&msgStats);
        TEST_ASSERT_NOT_EQUAL(msgStats.ServerShutdownTime, 0);
        TEST_ASSERT_EQUAL(msgStats.ExpiredClientStates, expectExpired);
    }

    TEST_ASSERT_GREATER_THAN_OR_EQUAL(TEN_SECOND_EXPIRERS, 5);


    // Steal a 10 second expiry and take over a disconnected durable client with ones that expires in 100 seconds
    TEST_ASSERT_EQUAL(hClient[firstTenSecondExpiry+1]->ExpiryTime, 0);     // still connected
    TEST_ASSERT_NOT_EQUAL(hClient[firstTenSecondExpiry+2]->ExpiryTime, 0); // disconnected
    for(uint32_t thisClientNo = firstTenSecondExpiry+1; thisClientNo <= firstTenSecondExpiry+2; thisClientNo++)
    {
        TEST_ASSERT_EQUAL(hClient[thisClientNo]->Durability, iecsDurable);
        sprintf(clientId, "%s%u", "ExpiryClient", thisClientNo);

        test_createClientStateCompleteContext_t context = { &hClient[thisClientNo], &pActionsRemaining };

        test_incrementActionsRemaining(pActionsRemaining, 1);
        override_clientStateExpiryInterval = 100;
        rc = ism_engine_createClientState(clientId,
                                          PROTOCOL_ID_MQTT,
                                          ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                          ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                          NULL, test_StealCallback,
                                          NULL,       // NO SECURITY CONTEXT (YET)
                                          &hClient[thisClientNo],
                                          &context,
                                          sizeof(context),
                                          test_createClientStateComplete);
        if (rc == ISMRC_ResumedClientState)
        {
            test_createClientStateComplete(rc, hClient[thisClientNo], &context);
        }
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

        test_waitForRemainingActions(pActionsRemaining);
    }

    test_waitForRemainingActions(pActionsRemaining);

    TEST_ASSERT_EQUAL(hClient[firstTenSecondExpiry+1]->ExpiryInterval, 100);
    TEST_ASSERT_EQUAL(hClient[firstTenSecondExpiry+2]->ExpiryInterval, 100);

    // Check stats
    expectExpirySet -= 1;
    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, expectExpired);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, expectExpirySet);

    fake_clientTableGenMismatch = 3; // Fake a gen mismatch after 3 loops

    // Sleep long enough for the 5 second ones to expire
    int sleepSeconds = 7;
    printf("Sleeping %d seconds to allow expiry to happen...\n", sleepSeconds);
    sleep(sleepSeconds);

    // Stats should indicate that FIVE_SECOND_EXPIRERS-3 clients have expired
    // (2 were discarded, 1 is still connected)
    expectExpired += (FIVE_SECOND_EXPIRERS-3);
    expectExpirySet -= (FIVE_SECOND_EXPIRERS-3);
    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, expectExpired);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, expectExpirySet);

    sprintf(clientId, "%s%u", "ExpiryClient", firstFiveSecondExpiry);
    foundClient = iecs_getVictimizedClient(pThreadData, clientId, iecs_generateClientIdHash(clientId));
    TEST_ASSERT_PTR_NULL(foundClient);

    sprintf(clientId, "%s%u", "ExpiryClient", firstTenSecondExpiry);
    foundClient = iecs_getVictimizedClient(pThreadData, clientId, iecs_generateClientIdHash(clientId));
    TEST_ASSERT_EQUAL(foundClient, hClient[firstTenSecondExpiry]);

    TEST_ASSERT_NOT_EQUAL(expiryControl->nextScheduledScan, ieceNO_TIMED_SCAN_SCHEDULED);

    sleepSeconds = 5;
    printf("Sleeping %d seconds to allow expiry to happen...\n", sleepSeconds);
    sleep(sleepSeconds);

    // Stats should indicate that additional TEN_SECOND_EXPIRERS-2 clients have expired
    // (1 was stolen, 1 taken over and both are still connected)
    expectExpired += TEN_SECOND_EXPIRERS-2;
    expectExpirySet -= TEN_SECOND_EXPIRERS-2;
    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, expectExpired);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, expectExpirySet);

    sprintf(clientId, "%s%u", "ExpiryClient", firstTenSecondExpiry);
    foundClient = iecs_getVictimizedClient(pThreadData, clientId, iecs_generateClientIdHash(clientId));
    TEST_ASSERT_PTR_NULL(foundClient);

    TEST_ASSERT_EQUAL(expiryControl->nextScheduledScan, ieceNO_TIMED_SCAN_SCHEDULED);

    // Check for and destroy the last 5 second expiry client to check that a scan is rescheduled
    sprintf(clientId, "%s%u", "ExpiryClient", lastFiveSecondExpiry);
    foundClient = iecs_getVictimizedClient(pThreadData, clientId, iecs_generateClientIdHash(clientId));
    TEST_ASSERT_EQUAL(foundClient, hClient[lastFiveSecondExpiry]);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient[lastFiveSecondExpiry],
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining,
                                       sizeof(pActionsRemaining),
                                       test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(expiryControl->nextScheduledScan, ieceNO_TIMED_SCAN_SCHEDULED);
    prevScheduledScan = expiryControl->nextScheduledScan;

    test_waitForRemainingActions(pActionsRemaining);

    expectExpirySet += 1;
    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, expectExpired);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, expectExpirySet);

    for(uint32_t thisClientNo=firstTenSecondExpiry+1; thisClientNo <= firstTenSecondExpiry+2; thisClientNo++)
    {
        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_destroyClientState(hClient[thisClientNo],
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                           &pActionsRemaining,
                                           sizeof(pActionsRemaining),
                                           test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }

    test_waitForRemainingActions(pActionsRemaining);

    expectExpirySet += 2;
    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, expectExpired);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, expectExpirySet);

    // Shouldn't have changed the schedule
    TEST_ASSERT_EQUAL(prevScheduledScan, expiryControl->nextScheduledScan);

    // Check that one of the non-expiring clients is still there, and take a useCount on it
    // to force one of the ism_engine_destroyDisconnectedClientState calls to go async.
    sprintf(clientId, "%s%u", "ExpiryClient", firstNonExpiring+1);
    iecs_findClientState(pThreadData, clientId, false, &foundClient);
    TEST_ASSERT_EQUAL(foundClient, hClient[firstNonExpiring+1]);

    uint32_t wentAsyncCount = 0;

    // Clean up by destroying the remaining clients
    for(int32_t clientNo=firstNonExpiring; clientNo<firstNonExpiring+NON_EXPIRING_COUNT; clientNo++)
    {
        sprintf(clientId, "%s%u", "ExpiryClient", clientNo);
        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_destroyDisconnectedClientState(clientId,
                                                       &pActionsRemaining,
                                                       sizeof(pActionsRemaining),
                                                       test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
            wentAsyncCount++;
        }
    }

     // We expect the clientState we 'found' to go async causing actions to remain
    TEST_ASSERT_EQUAL(wentAsyncCount, 1);
    TEST_ASSERT_NOT_EQUAL(actionsRemaining, 0);

    iecs_releaseClientStateReference(pThreadData, foundClient, false, false);

    printf("ABOUT TO WAIT\n");
    test_waitForActionsTimeOut(pActionsRemaining, 1);
    printf("DONE WAITING\n");
    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, expectExpired);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, expectExpirySet);

    uint32_t clientsToCleanup[] = {firstTenSecondExpiry+2, lastFiveSecondExpiry};
    for(int32_t i=0; i<(int32_t)(sizeof(clientsToCleanup)/sizeof(clientsToCleanup[0])); i++)
    {
        sprintf(clientId, "%s%u", "ExpiryClient", clientsToCleanup[i]);
        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_destroyDisconnectedClientState(clientId,
                                                       &pActionsRemaining,
                                                       sizeof(pActionsRemaining),
                                                       test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }

    test_waitForRemainingActions(pActionsRemaining);

    expectExpirySet -= (sizeof(clientsToCleanup)/sizeof(clientsToCleanup[0]));
    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, expectExpired);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, expectExpirySet);

    iecsHashTable_t *pTable = ismEngine_serverGlobal.ClientTable;

    TEST_ASSERT_EQUAL(pTable->TotalEntryCount, 4); /* One client, and the 3 shared subscription namespaces */
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.totalClientStatesCount, pTable->TotalEntryCount-3); // shared sub namespaces not included here

    free(hClient);
}

void test_capability_MixedClientStateExpiry_Phase2(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ieceExpiryControl_t *expiryControl = ismEngine_serverGlobal.clientStateExpiryControl;
    ismEngine_ClientStateHandle_t foundClient = NULL;
    char clientId[50];

    printf("Starting %s...\n", __func__);

    uint32_t firstTenSecondExpiry = EXPECTED_FIRST_TENSECOND_EXPIRY;

    sprintf(clientId, "%s%u", "ExpiryClient", firstTenSecondExpiry+1);
    foundClient = iecs_getVictimizedClient(pThreadData, clientId, iecs_generateClientIdHash(clientId));
    TEST_ASSERT_PTR_NOT_NULL(foundClient);
    TEST_ASSERT_EQUAL(foundClient->ExpiryInterval, 100); // Was changed to 100
    TEST_ASSERT_NOT_EQUAL(foundClient->ExpiryTime, 0);

    uint64_t expectExpired = 0;
    uint64_t expectExpirySet = 1;

    iemnClientStateStatistics_t stats;
    ismEngine_MessagingStatistics_t msgStats;
    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, expectExpired);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, expectExpirySet);
    ism_engine_getMessagingStatistics(&msgStats);
    TEST_ASSERT_NOT_EQUAL(msgStats.ServerShutdownTime, 0);
    TEST_ASSERT_EQUAL(msgStats.ClientStates, 1);
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.totalClientStatesCount, msgStats.ClientStates);

    // Wait for the initial scan to finish...
    while(expiryControl->scansStarted == 0 ||
          expiryControl->scansEnded != expiryControl->scansStarted)
    {
        sched_yield();
    }

    int sleepSeconds = 1;
    printf("Sleeping %d second to allow update of nextScheduledScan...\n", sleepSeconds);
    sleep(sleepSeconds);

    // Check that a scan is scheduled
    TEST_ASSERT_NOT_EQUAL(expiryControl->nextScheduledScan, ieceNO_TIMED_SCAN_SCHEDULED);
}

CU_TestInfo ISM_ClientStateExpiry_CUnit_test_ClientStateExpiry_Phase1[] =
{
    { "MixedClientStateExpiry_Phase1", test_capability_MixedClientStateExpiry_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_ClientStateExpiry_CUnit_test_ClientStateExpiry_Phase2[] =
{
    { "MixedClientStateExpiry_Phase2", test_capability_MixedClientStateExpiry_Phase2 },
    CU_TEST_INFO_NULL
};

// TEST the use of security context to set interval
void test_capability_SecContextExpiry_Phase1(void)
{
    uint32_t rc;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_ClientState_t *foundClient;

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    printf("Starting %s...\n", __func__);

    const char *clientId = "SecClient1";

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext=NULL;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport->clientID = clientId;
    mockTransport->userid = "USER1";

    // Add a messaging policy that allows publish and subscribe on Topic/*
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.AllowTopicPolicy\","
                                "\"Name\":\"AllowTopicPolicy\","
                                "\"ClientID\":\"SecClient*\","
                                "\"Topic\":\"Topic/*\","
                                "\"ActionList\":\"publish,subscribe\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    ism_security_context_setClientStateExpiry(mockContext, 100);

    test_createClientStateCompleteContext_t context = { &hClient, &pActionsRemaining };

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_createClientState(clientId,
                                      PROTOCOL_ID_MQTT,
                                      ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      NULL, test_StealCallback,
                                      mockContext,
                                      &hClient,
                                      &context,
                                      sizeof(context),
                                      test_createClientStateComplete);
    if (rc == OK) test_createClientStateComplete(rc, hClient, &context);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    TEST_ASSERT_NOT_EQUAL(hClient->hStoreCPR, ismSTORE_NULL_HANDLE);

    /* Set a *non-persistent* will message which should not cause the CPR to be removed --
     * do it twice to drive the logic which checks it doesn't need to change CPR.
     */
    for(int32_t loop=0; loop<2; loop++)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
        header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        size_t areaLengths[2] = {0, 13};
        void *areaData[2] = {NULL, "Message body"};
        ismEngine_MessageHandle_t hMessage;

        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areaData,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(((ismEngine_Message_t *)hMessage)->Header.Expiry, 0);

        rc = ism_engine_setWillMessage(hClient,
                                       ismDESTINATION_TOPIC,
                                       "Topic/Will",
                                       hMessage,
                                       0,
                                       0,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Check that our client still has a CPR, even though this is a non-persistent will msg.
    TEST_ASSERT_NOT_EQUAL(hClient->hStoreCPR, ismSTORE_NULL_HANDLE);

    // Check stats (should now show none expired, and none with expiry set)
    iemnClientStateStatistics_t stats;

    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, 0);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, 0);

    foundClient = iecs_getVictimizedClient(pThreadData, clientId, iecs_generateClientIdHash(clientId));
    TEST_ASSERT_PTR_NOT_NULL(foundClient);
    TEST_ASSERT_EQUAL(foundClient->ExpiryInterval, 100);

    // Change the timeout to be different, but still not zero
    uint32_t newExpiry = 60;
    ism_security_context_setClientStateExpiry(mockContext, newExpiry);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining,
                                       sizeof(pActionsRemaining),
                                       test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);

    test_waitForRemainingActions(pActionsRemaining);

    // Now expect 1 with expiry set
    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, 0);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, 1);

    foundClient = iecs_getVictimizedClient(pThreadData, clientId, iecs_generateClientIdHash(clientId));
    TEST_ASSERT_PTR_NOT_NULL(foundClient);
    TEST_ASSERT_EQUAL(foundClient->ExpiryInterval, newExpiry);
}

void test_capability_SecContextExpiry_Phase2(void)
{
    uint32_t rc;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_ClientState_t *foundClient;

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    printf("Starting %s...\n", __func__);

    const char *clientId = "SecClient1";

    // Check it got recovered and with the expected interval...
    foundClient = iecs_getVictimizedClient(pThreadData, clientId, iecs_generateClientIdHash(clientId));
    TEST_ASSERT_PTR_NOT_NULL(foundClient);
    TEST_ASSERT_EQUAL(foundClient->ExpiryInterval, 60);

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext=NULL;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport->clientID = clientId;
    mockTransport->userid = "USER1";

    // Change the expiry back UP to 100
    ism_security_context_setClientStateExpiry(mockContext, 100);

    test_createClientStateCompleteContext_t context = { &hClient, &pActionsRemaining };

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_createClientState(clientId,
                                      PROTOCOL_ID_MQTT,
                                      ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      NULL, test_StealCallback,
                                      mockContext,
                                      &hClient,
                                      &context,
                                      sizeof(context),
                                      test_createClientStateComplete);
    if (rc == OK || rc == ISMRC_ResumedClientState) test_createClientStateComplete(rc, hClient, &context);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    // Check stats (should now show none expired, and none with expiry set)
    iemnClientStateStatistics_t stats;

    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, 0);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, 0);

    foundClient = iecs_getVictimizedClient(pThreadData, clientId, iecs_generateClientIdHash(clientId));
    TEST_ASSERT_PTR_NOT_NULL(foundClient);
    TEST_ASSERT_EQUAL(foundClient->ExpiryInterval, 100);
}

void test_capability_SecContextExpiry_Phase3(void)
{
    uint32_t rc;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_ClientState_t *foundClient;

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    printf("Starting %s...\n", __func__);

    const char *clientId = "SecClient1";

    // Check it got recovered and with the expected interval...
    foundClient = iecs_getVictimizedClient(pThreadData, clientId, iecs_generateClientIdHash(clientId));
    TEST_ASSERT_PTR_NOT_NULL(foundClient);
    TEST_ASSERT_EQUAL(foundClient->ExpiryInterval, 100);
    TEST_ASSERT_EQUAL(foundClient->LastConnectedTime, ism_common_convertExpireToTime(ismEngine_serverGlobal.ServerShutdownTimestamp));
    TEST_ASSERT_EQUAL(foundClient->ExpiryTime,
                      foundClient->LastConnectedTime + ((ism_time_t)foundClient->ExpiryInterval * 1000000000));

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext=NULL;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport->clientID = clientId;
    mockTransport->userid = "USER1";

    ism_security_context_setClientStateExpiry(mockContext, 100);

    test_createClientStateCompleteContext_t context = { &hClient, &pActionsRemaining };

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_createClientState(clientId,
                                      PROTOCOL_ID_MQTT,
                                      ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      NULL, test_StealCallback,
                                      mockContext,
                                      &hClient,
                                      &context,
                                      sizeof(context),
                                      test_createClientStateComplete);
    if (rc == OK || rc == ISMRC_ResumedClientState) test_createClientStateComplete(rc, hClient, &context);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    // Check stats (should now show none expired, and none with expiry set)
    iemnClientStateStatistics_t stats;

    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, 0);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, 0);

    foundClient = iecs_getVictimizedClient(pThreadData, clientId, iecs_generateClientIdHash(clientId));
    TEST_ASSERT_PTR_NOT_NULL(foundClient);
    TEST_ASSERT_EQUAL(foundClient->ExpiryInterval, 100);

    // Change the timeout to be ZERO
    ism_security_context_setClientStateExpiry(mockContext, 0);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining,
                                       sizeof(pActionsRemaining),
                                       test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);

    test_waitForRemainingActions(pActionsRemaining);

    // Now expect none with expiry set and to not find the clientState
    iemn_getClientStateStatistics(pThreadData, &stats);
    TEST_ASSERT_EQUAL(stats.ExpiredClientStates, 0);
    TEST_ASSERT_EQUAL(stats.ZombieClientStatesWithExpirySet, 0);

    foundClient = iecs_getVictimizedClient(pThreadData, clientId, iecs_generateClientIdHash(clientId));
    TEST_ASSERT_PTR_NULL(foundClient);
}

CU_TestInfo ISM_ClientStateExpiry_CUnit_test_SecContextExpiry_Phase1[] =
{
    { "SecContextExpiry_Phase1", test_capability_SecContextExpiry_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_ClientStateExpiry_CUnit_test_SecContextExpiry_Phase2[] =
{
    { "SecContextExpiry_Phase2", test_capability_SecContextExpiry_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_ClientStateExpiry_CUnit_test_SecContextExpiry_Phase3[] =
{
    { "SecContextExpiry_Phase3", test_capability_SecContextExpiry_Phase3 },
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
                           512);
}

int initPhaseColdSecurity(void)
{
    return test_engineInit(true, false,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false,
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           512);
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
                           512);
}

int initPhaseWarmSecurity(void)
{
    return test_engineInit(false, false,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false,
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           512);
}

//For phases after phase 0
int termPhaseWarm(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_ClientStateExpiry_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("Expiry", initPhaseCold, termPhaseWarm, ISM_ClientStateExpiry_CUnit_test_ClientStateExpiry_Phase1),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_ClientStateExpiry_CUnit_phase2suites[] =
{
    IMA_TEST_SUITE("Expiry", initPhaseWarm, termPhaseCold, ISM_ClientStateExpiry_CUnit_test_ClientStateExpiry_Phase2),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_ClientStateExpiry_CUnit_phase3suites[] =
{
    IMA_TEST_SUITE("SecContextExpiry", initPhaseColdSecurity, termPhaseWarm, ISM_ClientStateExpiry_CUnit_test_SecContextExpiry_Phase1),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_ClientStateExpiry_CUnit_phase4suites[] =
{
    IMA_TEST_SUITE("SecContextExpiry", initPhaseWarmSecurity, termPhaseWarm, ISM_ClientStateExpiry_CUnit_test_SecContextExpiry_Phase2),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_ClientStateExpiry_CUnit_phase5suites[] =
{
    IMA_TEST_SUITE("SecContextExpiry", initPhaseWarmSecurity, termPhaseCold, ISM_ClientStateExpiry_CUnit_test_SecContextExpiry_Phase3),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_ClientStateExpiry_CUnit_phase1suites
                              , ISM_ClientStateExpiry_CUnit_phase2suites
                              , ISM_ClientStateExpiry_CUnit_phase3suites
                              , ISM_ClientStateExpiry_CUnit_phase4suites
                              , ISM_ClientStateExpiry_CUnit_phase5suites };


int main(int argc, char *argv[])
{
    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
