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
/* Module Name: test_systemTopic.c                                   */
/*                                                                   */
/* Description: Main source file for CUnit test of system topics     */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "topicTree.h"
#include "topicTreeInternal.h"

#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

//****************************************************************************
/// @brief Test that the engine disallows durables on system topics
//****************************************************************************
uint32_t DISALLOWDURABLE_SUBOPTS[] = {ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                                      ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE,
                                      ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                                      ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE};
#define DISALLOWDURABLE_NUMSUBOPTS (sizeof(DISALLOWDURABLE_SUBOPTS)/sizeof(DISALLOWDURABLE_SUBOPTS[0]))

void test_DisallowDurable(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient, hSystemClient;
    ismEngine_SessionHandle_t hSession, hSystemSession;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("Client",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient,
                                     &hSession,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession("__SystemClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hSystemClient,
                                     &hSystemSession,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_Subscription_t *subscription;

    // Access a non-existent subscription name
    rc = iett_findClientSubscription(pThreadData,
                                     ((ismEngine_ClientState_t *)hClient)->pClientId,
                                     iett_generateClientIdHash(((ismEngine_ClientState_t *)hClient)->pClientId),
                                     "DDSub",
                                     iett_generateSubNameHash("DDSub"),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    // Check various bad & good topics (0x01 switches expected result)
    bool expectSuccess = false;
    char *TestTopic[] = { "$SYS", "$SYS/TEST", "$SYS/#", "$SYS/+",
                          (char *)0x01, // Switch to expect SUCCESS for following tests
                          "TEST", "#", "TEST/+", "/$SYS", "SYS", "SYS$",
                          "+/$SYS", "#/$SYS/#", "$SYS/UseSystemClient",
                          (char *)0x01, // Switch to expect FAILURE again for following tests
                          "$SYSTEM", "$SYSTEM/+", "$SYS/$SYS", "$SIS", "$SIS/A",
                          "$SIS/TEST", "$$SYS/DoubleDollar", "$sys/should/be/ok",
                          "$Systolic/Blood/Pressure",
                          NULL
                        };

    for(int32_t i=0; TestTopic[i] != NULL; i++)
    {
        // Switch expectation between success & failure
        if (TestTopic[i] == (char *)0x01)
        {
            expectSuccess = !expectSuccess;
            continue;
        }

        ismEngine_ClientStateHandle_t hUseClient;

        if (strstr(TestTopic[i], "UseSystemClient") != NULL)
        {
            hUseClient = hSystemClient;
        }
        else
        {
            hUseClient = hClient;
        }

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE };

        // Some of the topics cannot use MQTT subscriptions (because they are invalid
        // MQTT topic strings)
        switch(i)
        {
            case 17:
                subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
                break;
            default:
                subAttrs.subOptions |= DISALLOWDURABLE_SUBOPTS[rand()%DISALLOWDURABLE_NUMSUBOPTS];
                break;
        }

        rc = sync_ism_engine_createSubscription(hUseClient,
                                                "DDSub",
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                TestTopic[i],
                                                &subAttrs,
                                                NULL); // Owning client same as requesting client
        if (expectSuccess)
        {
            TEST_ASSERT_EQUAL(rc, OK);

            rc = iett_findClientSubscription(pThreadData,
                                             ((ismEngine_ClientState_t *)hUseClient)->pClientId,
                                             iett_generateClientIdHash(((ismEngine_ClientState_t *)hUseClient)->pClientId),
                                             "DDSub",
                                             iett_generateSubNameHash("DDSub"),
                                             &subscription);
            TEST_ASSERT_EQUAL(rc, OK);
            // Check that the iettSUBATTR_SYSTEM_TOPIC is set when we expect it to be.
            if (strcasestr(TestTopic[i], "$SYS") == TestTopic[i])
            {
                TEST_ASSERT_NOT_EQUAL((subscription->internalAttrs & iettSUBATTR_SYSTEM_TOPIC), 0);
            }
            else
            {
                TEST_ASSERT_EQUAL((subscription->internalAttrs & iettSUBATTR_SYSTEM_TOPIC), 0);
            }
            TEST_ASSERT_NOT_EQUAL((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE), 0);

            rc = iett_releaseSubscription(pThreadData, subscription, false);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_engine_destroySubscription(hUseClient, "DDSub", hUseClient, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_BadSysTopic);

            rc = iett_findClientSubscription(pThreadData,
                                             ((ismEngine_ClientState_t *)hClient)->pClientId,
                                             iett_generateClientIdHash(((ismEngine_ClientState_t *)hClient)->pClientId),
                                             "DDSub",
                                             iett_generateSubNameHash("DDSub"),
                                             NULL);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        }
    }

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_destroyClientAndSession(hSystemClient, hSystemSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test that $SYS MQTT subscriptions are not shared with cluster
//****************************************************************************
uint32_t NOTSHAREDWITHCLUSTER_SUBOPTS[] = {ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                                           ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE,
                                           ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE};
#define NOTSHAREDWITHCLUSTER_NUMSUBOPTS (sizeof(NOTSHAREDWITHCLUSTER_SUBOPTS)/sizeof(NOTSHAREDWITHCLUSTER_SUBOPTS[0]))

void test_NotSharedWithCluster(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("Client",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient,
                                     &hSession,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_Subscription_t *subscription;

    // Check various bad topics (0x01 switches expected result)
    bool expectShared = false;
    char *TestTopic[] = { "$SYS", "$SYS/TEST", "$SYS/#", "$SYS/+",
                          (char *)0x01,
                          "TEST", "#", "TEST/+", "/$SYS", "SYS", "SYS$",
                          "+/$SYS", "#/$SYS/#",
                          (char *)0x01,
                          "$SYSTEM", "$SYSTEM/+", "$SYS/$SYS", "$SIS", "$SIS/A",
                          "$SIS/TEST", "$$SYS/DoubleDollar", "$sys/should/be/ok",
                          "$Systolic/Blood/Pressure",
                          NULL
                        };

    for(int32_t i=0; TestTopic[i] != NULL; i++)
    {
        ismEngine_ConsumerHandle_t hConsumer = NULL;

        // Switch expectation between success & failure
        if (TestTopic[i] == (char *)0x01)
        {
            expectShared = !expectShared;
            continue;
        }

        ismEngine_SubscriptionAttributes_t subAttrs = { NOTSHAREDWITHCLUSTER_SUBOPTS[rand()%NOTSHAREDWITHCLUSTER_NUMSUBOPTS] };

        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       TestTopic[i],
                                       &subAttrs,
                                       NULL,
                                       NULL, 0, NULL,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &hConsumer,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hConsumer);

        subscription = (ismEngine_Subscription_t *)(hConsumer->engineObject);
        TEST_ASSERT_PTR_NOT_NULL(subscription);

        if (expectShared)
        {
            TEST_ASSERT_EQUAL((subscription->internalAttrs & iettSUBATTR_SYSTEM_TOPIC), 0);
            TEST_ASSERT_NOT_EQUAL((subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER), 0);
            TEST_ASSERT_EQUAL((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE), 0);
        }
        else
        {
            TEST_ASSERT_NOT_EQUAL((subscription->internalAttrs & iettSUBATTR_SYSTEM_TOPIC), 0);
            TEST_ASSERT_EQUAL((subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER), 0);
            TEST_ASSERT_EQUAL((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE), 0);
        }

        rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_SystemTopic_CUnit_Basic[] =
{
    { "DisallowDurable", test_DisallowDurable },
    { "NotSharedWithCluster", test_NotSharedWithCluster },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initSystemTopic(void)
{
    return test_engineInit_DEFAULT;
}

int termSystemTopic(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_SystemTopic_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("Basic", initSystemTopic, termSystemTopic, ISM_SystemTopic_CUnit_Basic),
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
    int trclvl = 4;
    int retval = 0;

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    ism_time_t seedVal = ism_common_currentTimeNanos();

    srand(seedVal);

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_SystemTopic_CUnit_allsuites);

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
