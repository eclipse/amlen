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
/* Module Name: test_clientStateRrecovery.c                          */
/*                                                                   */
/* Description: Source file for testing clientState recovery where a */
/*              bounce is required.                                  */
/*                                                                   */
/*********************************************************************/
#include <math.h>
#include <errno.h>
#include <stdbool.h>
#include <engineStore.h>

#include "clientState.h"
#include "clientStateInternal.h"
#include "clientStateExpiry.h"
#include "resourceSetStats.h"

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_log.h"
#include "test_utils_sync.h"
#include "test_utils_options.h"

//****************************************************************************
/// @brief Test ClientState recovery
//****************************************************************************
typedef struct durableSubsContext
{
    uint32_t                        Count;
    bool                            expectProperties;
    bool                            Found[100];
} durableSubsContext_t;

static void durableSubsCallback(
    ismEngine_SubscriptionHandle_t            subHandle,
    const char *                              pSubName,
    const char *                              pTopicString,
    void *                                    properties,
    size_t                                    propertiesLength,
    const ismEngine_SubscriptionAttributes_t *pSubAttributes,
    uint32_t                                  consumerCount,
    void *                                    pContext)
{
    durableSubsContext_t *pCallbackContext = (durableSubsContext_t *)pContext;
    ismEngine_Subscription_t *subscription = (ismEngine_Subscription_t *)subHandle;

    TEST_ASSERT_PTR_NOT_NULL(subscription);
    TEST_ASSERT_NOT_EQUAL(subscription->useCount, 0);

    if (pCallbackContext->expectProperties)
    {
        TEST_ASSERT_PTR_NOT_NULL(properties);
        uint32_t SubIndex = ism_common_getIntProperty(properties, "ClientStateProperty", 0);
        TEST_ASSERT_NOT_EQUAL(SubIndex, 0);
        TEST_ASSERT_EQUAL(pCallbackContext->Found[SubIndex], false);
        pCallbackContext->Found[SubIndex] = true;
    }

    pCallbackContext->Count++;
}

void test_ClientStateDeletedAndNotProblem_Phase1(void)
{
    const char *clientId = "DeletedAndNotProblem_CLIENT";
    int32_t rc = OK;

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    ismEngine_ClientState_t *pClient = NULL;
    ismEngine_ClientState_t *pFakedClient = NULL;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Fake up the writing of a DELETED clientState to the store to simulate
    // the timing window where a new client state comes in before the old one
    // has actually been deleted from the store.
    iecsNewClientStateInfo_t clientInfo;
    clientInfo.pClientId = clientId;
    clientInfo.protocolId = PROTOCOL_ID_MQTT;
    clientInfo.pSecContext = NULL;
    clientInfo.durability = iecsDurable;
    clientInfo.takeover = iecsNoTakeover;
    clientInfo.fCleanStart = false;
    clientInfo.pStealContext = NULL;
    clientInfo.pStealCallbackFn = NULL;
    clientInfo.resourceSet = iecs_getResourceSet(pThreadData, clientInfo.pClientId, clientInfo.protocolId, iereOP_ADD);

    rc = iecs_newClientState(pThreadData,
                             &clientInfo,
                             &pFakedClient);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pFakedClient);

    ismStore_Handle_t CSR1 = ismSTORE_NULL_HANDLE;
    ismStore_Handle_t CPR1 = ismSTORE_NULL_HANDLE;

    rc = iecs_storeClientState(pThreadData, pFakedClient, false, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(pFakedClient->hStoreCSR, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_NOT_EQUAL(pFakedClient->hStoreCPR, ismSTORE_NULL_HANDLE);

    CSR1 = pFakedClient->hStoreCSR;
    CPR1 = pFakedClient->hStoreCPR;

    // Update the state to be deleted -this won't prevent will messages being published etc.
    rc = ism_store_updateRecord(pThreadData->hStream,
                                CSR1,
                                ismSTORE_NULL_HANDLE,
                                iestCSR_STATE_DELETED,
                                ismSTORE_SET_STATE);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_store_commit(pThreadData->hStream);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_createClientState(clientId,
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL
                                         | ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &pClient);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pClient);

    // Create a subscription... Will this get deleted after restart?
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

    rc = sync_ism_engine_createSubscription(pClient,
                                            "TESTSUB",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "TOPICTEST",
                                            &subAttrs,
                                            pClient);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t clientIdHash = iecs_generateClientIdHash(clientId);

    durableSubsContext_t durableSubsContext = {0, false, {false}};

    // Check subscription exists
    rc = ism_engine_listSubscriptions(pClient,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 1);

    ismEngine_ClientState_t *pFoundClient = iecs_getVictimizedClient(pThreadData, clientId, clientIdHash);
    TEST_ASSERT_EQUAL(pClient, pFoundClient);

    // Add ANOTHER FakedClient
    pFakedClient->hStoreCSR = pFakedClient->hStoreCPR = ismSTORE_NULL_HANDLE;
    rc = iecs_storeClientState(pThreadData, pFakedClient, false, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(pFakedClient->hStoreCSR, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_NOT_EQUAL(pFakedClient->hStoreCSR, CSR1);
    TEST_ASSERT_NOT_EQUAL(pFakedClient->hStoreCPR, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_NOT_EQUAL(pFakedClient->hStoreCPR, CPR1);

    rc = ism_store_commit(pThreadData->hStream);
    TEST_ASSERT_EQUAL(rc, OK);

    // Update the state to be deleted -this won't prevent will messages being published etc.
    rc = ism_store_updateRecord(pThreadData->hStream,
                                pFakedClient->hStoreCSR,
                                ismSTORE_NULL_HANDLE,
                                iestCSR_STATE_DELETED,
                                ismSTORE_SET_STATE);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_store_commit(pThreadData->hStream);
    TEST_ASSERT_EQUAL(rc, OK);

    // Tidy up
    iecs_freeClientState(pThreadData, pFakedClient, true);
}

void test_ClientStateDeletedAndNotProblem_Phase2(void)
{
    const char *clientId = "DeletedAndNotProblem_CLIENT";
    int32_t rc;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    ismEngine_ClientState_t *pClient = NULL;

    rc = sync_ism_engine_createClientState(clientId,
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL
                                         | ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &pClient);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    // Check subscription still exists
    durableSubsContext_t durableSubsContext = {0, false, {false}};

    rc = ism_engine_listSubscriptions(pClient,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 1);

    rc = sync_ism_engine_destroyClientState(pClient, ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_Recovery_CUnit_test_Phase1[] =
{
    { "ClientStateDeletedAndNotPhase1", test_ClientStateDeletedAndNotProblem_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_Recovery_CUnit_test_Phase2[] =
{
    { "ClientStateDeletedAndNotPhase2", test_ClientStateDeletedAndNotProblem_Phase2 },
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

CU_SuiteInfo ISM_Recovery_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("Phase1", initPhaseCold, termPhaseWarm, ISM_Recovery_CUnit_test_Phase1),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_Recovery_CUnit_phase2suites[] =
{
    IMA_TEST_SUITE("Phase2", initPhaseWarm, termPhaseWarm, ISM_Recovery_CUnit_test_Phase2),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_Recovery_CUnit_phase1suites
                              , ISM_Recovery_CUnit_phase2suites };

int main(int argc, char *argv[])
{
    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
