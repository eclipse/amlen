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
/* Module Name: test_updateSub.c                                     */
/*                                                                   */
/* Description: Main source file for CUnit Update Subscription tests */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "topicTree.h"
#include "topicTreeInternal.h"
#include "queueCommon.h"
#include "multiConsumerQInternal.h"

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_assert.h"
#include "test_utils_sync.h"

#define DEFAULT_MAXMESSAGES 5000
#define UPDATED_MAXMESSAGES_PHASE1 7000
#define UPDATED_MAXMESSAGES_PHASE2 7001
ism_prop_t *test_createProperties(int newMaxMessages)
{
    ism_prop_t *pSubProperties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(pSubProperties);

    ism_field_t f;
    f.type = VT_Integer;
    f.val.i = newMaxMessages;
    int32_t rc = ism_common_setProperty(pSubProperties, ismENGINE_ADMIN_PROPERTY_MAXMESSAGES, &f);
    TEST_ASSERT_EQUAL(rc, OK);

    return pSubProperties;
}

#define SUBSCRIPTION_NAME "UpdateableSubscription"
#define NONEXISTENT_SUB_NAME "Non-ExistentSubscription"
#define SUBSCRIPTION_TOPIC "Durable/Updateable"
#define SUBSCRIPTION_NAME_ND "NonUpdateableSubscription"
#define SUBSCRIPTION_TOPIC_ND "NonDurable/Updateable"
void test_updateSubscription_Phase1(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_Subscription_t *subscription;
    const char *pClientId = "C1";

    iepiPolicyInfo_t *pPolicyInfo = NULL;

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    // Randomly pick reliability - should not change queue type
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    rand()%4 };

    ism_prop_t *pSubProperties = test_createProperties(UPDATED_MAXMESSAGES_PHASE1);

    // Create a client state for a client
    rc = test_createClientAndSession(pClientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient,
                                     &hSession,
                                     true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a durable subscription (because MQ Connectivity does).
    rc = sync_ism_engine_createSubscription(hClient,
                                            SUBSCRIPTION_NAME,
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            SUBSCRIPTION_TOPIC,
                                            &subAttrs,
                                            hClient);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_findClientSubscription(pThreadData,
                                     pClientId,
                                     iett_generateClientIdHash(pClientId),
                                     SUBSCRIPTION_NAME,
                                     iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the queue is set up with expected options
    pPolicyInfo = ieq_getPolicyInfo(subscription->queueHandle);

    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, DEFAULT_MAXMESSAGES);

    rc = iett_releaseSubscription(pThreadData, subscription, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check we get 'not found' for a non-existent subscription
    rc = ism_engine_updateSubscription(hClient,
                                       NONEXISTENT_SUB_NAME,
                                       pSubProperties,
                                       hClient,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    // And now on with the real test...
    rc = ism_engine_updateSubscription(hClient,
                                       SUBSCRIPTION_NAME,
                                       pSubProperties,
                                       hClient,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_findClientSubscription(pThreadData,
                                     pClientId,
                                     iett_generateClientIdHash(pClientId),
                                     SUBSCRIPTION_NAME,
                                     iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the subscription queue has changed correctly
    pPolicyInfo = ieq_getPolicyInfo(subscription->queueHandle);

    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, UPDATED_MAXMESSAGES_PHASE1);

    rc = iett_releaseSubscription(pThreadData, subscription, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_common_freeProperties(pSubProperties);
}

void test_updateSubscription_Phase2(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_Subscription_t *subscription;
    const char *pClientId = "C1";

    iepiPolicyInfo_t *pPolicyInfo = NULL;

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    // Create a client state for a client
    rc = test_createClientAndSession(pClientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient,
                                     &hSession,
                                     true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_findClientSubscription(pThreadData,
                                     pClientId,
                                     iett_generateClientIdHash(pClientId),
                                     SUBSCRIPTION_NAME,
                                     iett_generateSubNameHash(SUBSCRIPTION_NAME),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_STRINGS_EQUAL(subscription->subName, SUBSCRIPTION_NAME);

    // Check that the queue is set up with expected options
    pPolicyInfo = ieq_getPolicyInfo(subscription->queueHandle);

    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, UPDATED_MAXMESSAGES_PHASE1);

    rc = iett_releaseSubscription(pThreadData, subscription, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a non-durable subscription and then update it to show 
    // that no attempt is made to write anything to the store.

    // Create a client state for a client
    rc = test_createClientAndSession(pClientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient,
                                     &hSession,
                                     true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a non-durable subscription
    // ism_engine_createSubscription requires either durable or
    // shared to be set so we opt for shared in order to get a
    // non-durable subscription.
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                    ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE }; // Must be JMS

    rc = ism_engine_createSubscription(hClient,
                                       SUBSCRIPTION_NAME_ND,
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       SUBSCRIPTION_TOPIC_ND,
                                       &subAttrs,
                                       hClient,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_findClientSubscription(pThreadData,
                                     pClientId,
                                     iett_generateClientIdHash(pClientId),
                                     SUBSCRIPTION_NAME_ND,
                                     iett_generateSubNameHash(SUBSCRIPTION_NAME_ND),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the queue is set up with expected options
    pPolicyInfo = ieq_getPolicyInfo(subscription->queueHandle);

    TEST_ASSERT_EQUAL(pPolicyInfo -> maxMessageCount, DEFAULT_MAXMESSAGES);

    rc = iett_releaseSubscription(pThreadData, subscription, false);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_prop_t *pSubProperties = test_createProperties(UPDATED_MAXMESSAGES_PHASE2);

    rc = ism_engine_updateSubscription(hClient,
                                       SUBSCRIPTION_NAME_ND,
                                       pSubProperties,
                                       hClient,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_findClientSubscription(pThreadData,
                                     pClientId,
                                     iett_generateClientIdHash(pClientId),
                                     SUBSCRIPTION_NAME_ND,
                                     iett_generateSubNameHash(SUBSCRIPTION_NAME_ND),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the subscription queue has changed correctly
    pPolicyInfo = ieq_getPolicyInfo(subscription->queueHandle);

    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, UPDATED_MAXMESSAGES_PHASE2);

    rc = iett_releaseSubscription(pThreadData, subscription, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_common_freeProperties(pSubProperties);
}

#undef SUBSCRIPTION_NAME
#undef SUBSCRIPTION_TOPIC
#undef DEFAULT_MAXMESSAGES

CU_TestInfo ISM_UpdateSub_CUnit_basicTests_Phase1[] =
{
    { "UpdateDurableSub_Phase1", test_updateSubscription_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_UpdateSub_CUnit_basicTests_Phase2[] =
{
    { "UpdateDurableSub_Phase2", test_updateSubscription_Phase2 },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initSuite_COLD(void)
{
    return test_engineInit(true,
                           true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, // Recovery should complete
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int initSuite_WARM(void)
{
    return test_engineInit(false,
                           true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, // Recovery should complete
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termSuite_ALL(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_UpdateSub_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("BasicTests", initSuite_COLD, termSuite_ALL, ISM_UpdateSub_CUnit_basicTests_Phase1),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_UpdateSub_CUnit_phase2suites[] =
{
    IMA_TEST_SUITE("BasicTests", initSuite_WARM, termSuite_ALL, ISM_UpdateSub_CUnit_basicTests_Phase2),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_UpdateSub_CUnit_phase1suites
                              , ISM_UpdateSub_CUnit_phase2suites };

int main(int argc, char *argv[])
{
    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
