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
/* Module Name: test_asyncPublish.c                                  */
/*                                                                   */
/* Description: CUnit tests of Publishes going asunc      */
/*                                                                   */
/*********************************************************************/

#include "test_utils_assert.h"
#include "test_utils_client.h"
#include "test_utils_initterm.h"
#include "test_utils_message.h"
#include "test_utils_sync.h"
#include "test_utils_asyncPause.h"

#include <unistd.h>



//Create durable subscription on topic1
//Create durable subscription on topic2
//Publish msg1 on topic1 (wait for completion)
//Publish msg2 on topic1 (block completion)
//Publish msg3 on topic2 (block completion)
//Complete publish of msg2 - check start of publish of msg3
//                             hasn't meant the callbacks affect topic2
//Complete publish of msg3

static uint64_t asyncPublishesCompleted=0;

void completeAsyncPublish(int32_t retcode, void *handle, void *pContext)
{
    TEST_ASSERT_EQUAL(retcode, OK);
    __sync_fetch_and_add(&asyncPublishesCompleted, 1);
}
void test_SubListCaching(void)
{
    ismEngine_MessageHandle_t hMessage;
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;

    char *topic1 = "topic1";
    char *topic2 = "topic2";

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    /* Create our clients and sessions */
    rc = test_createClientAndSession("SomeClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };

    rc = sync_ism_engine_createSubscription(hClient,
                                            "sub1",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            topic1,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_createSubscription(hClient,
                                            "sub2",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            topic2,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish a persistent message on topic 1 and wait for it to complete
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, topic1,
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 topic1,
                                                 NULL,
                                                 hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    test_utils_pauseAsyncCompletions();

    //Publish another message on topic1 but this time completion will be blocked
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, topic1,
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            topic1,
                                            NULL,
                                            hMessage,
                                            NULL, 0,
                                            completeAsyncPublish);
    TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);


    //Publish a message on topic2 (completion will be blocked)
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, topic2,
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            topic2,
                                            NULL,
                                            hMessage,
                                            NULL, 0,
                                            completeAsyncPublish);
    TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);


    TEST_ASSERT_EQUAL(asyncPublishesCompleted, 0);
    test_utils_restartAsyncCompletions();

    while(asyncPublishesCompleted < 2)
    {
        usleep(50);
    }

}

CU_TestInfo ISM_AsyncPublish_CUnit_test_Deterministic[] =
{
    { "TestSubListCaching", test_SubListCaching},
    CU_TEST_INFO_NULL
};

int initSuite(void)
{
    return test_engineInit(true, true,
                           true, // Disable Auto creation of named queues
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termSuite(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_AwkwardQDelete_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("Deterministic", initSuite, termSuite, ISM_AsyncPublish_CUnit_test_Deterministic),
    CU_SUITE_INFO_NULL
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
    int testLogLevel = 2;

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_AwkwardQDelete_CUnit_allsuites);

    if (retval == 0)
    {
        CU_basic_run_tests();

        CU_RunSummary * CU_pRunSummary_Final;
        CU_pRunSummary_Final = CU_get_run_summary();

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

