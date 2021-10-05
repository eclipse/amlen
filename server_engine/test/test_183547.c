/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
/* Module Name: test_183547.c                                        */
/*                                                                   */
/* Description: Fix a defect where changing the expectedmsgrate of   */
/* a client after the hMsgDlvInfo had been allocated and then        */
/* receiving lots of acking-requiring messages caused a segfault     */
/* because the code assumed hMsgDlvInfo had been resized             */
/*********************************************************************/
#include <sys/prctl.h>

#include "engineInternal.h"
#include "policyInfo.h"
#include "multiConsumerQInternal.h"
#include "topicTree.h"
#include "ismutil.h"

#include "test_utils_initterm.h"
#include "test_utils_sync.h"
#include "test_utils_assert.h"
#include "test_utils_client.h"
#include "test_utils_options.h"
#include "test_utils_pubsuback.h"
#include "test_utils_security.h"


void test_capability_183547(void)
{

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    char *ClientId="client183547";
    char *subName;
    uint64_t maxDepth = 50000;
    int32_t rc;


    /************************************************************************/
    /* Set the default policy info to allow certain depth queues            */
    /************************************************************************/
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = maxDepth;
    iepi_getDefaultPolicyInfo(false)->maxMsgBehavior  = DiscardOldMessages;
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    char *topicString = "BasicTest/183547";
    subName        = "subBasic";

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext=NULL;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);


    rc = sync_ism_engine_createClientState(ClientId,
            testDEFAULT_PROTOCOL_ID,
            ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
            NULL, NULL,
            mockContext,
            &hClient);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient,
            ismENGINE_CREATE_SESSION_OPTION_NONE,
            &hSession,
            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSession,
            ismENGINE_START_DELIVERY_OPTION_NONE,
            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_makeSub( subName
           , topicString
           ,   ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE
             | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
           , hClient
           , hClient);

    //Send  a qos 1 message and a qos 2 message that we leave unacked
    test_pubMessages(topicString,
            ismMESSAGE_PERSISTENCE_PERSISTENT,
            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
            1);

    test_pubMessages(topicString,
            ismMESSAGE_PERSISTENCE_PERSISTENT,
            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
            1);

    uint32_t numMsgsToGet = 2;

    test_markMsgs( subName
            , hSession
            , hClient
            , numMsgsToGet //Total messages we have delivered before disabling consumer
            , 0 //Of the delivered messages, how many to Skip before ack
            , numMsgsToGet //Of the delivered messages, how many to consume
            , 0 //Of the delivered messages, how many to mark received
            , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
            , 0 //Of the delivered messages, how many to return to caller un(n)acked
            , 0 //Of the delivered messages, how many to nack not received
            , 0 //Of the delivered messages, how many to nack not sent
            , 0);//Handles of messages returned to us un(n)acked

    rc = sync_ism_engine_destroyClientState(hClient,
                                            ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    ((mock_ismSecurity_t *)mockContext)->ExpectedMsgRate = EXPECTEDMSGRATE_LOW;
    hClient = NULL;
    hSession = NULL;

    rc = sync_ism_engine_createClientState(ClientId,
            testDEFAULT_PROTOCOL_ID,
            ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
            NULL, NULL,
            mockContext,
            &hClient);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    rc = ism_engine_createSession(hClient,
            ismENGINE_CREATE_SESSION_OPTION_NONE,
            &hSession,
            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSession,
            ismENGINE_START_DELIVERY_OPTION_NONE,
            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_pubMessages(topicString,
            ismMESSAGE_PERSISTENCE_PERSISTENT,
            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
            45000);

    test_markMsgs( subName
                , hSession
                , hClient
                , 45000 //Total messages we have delivered before disabling consumer
                , 0 //Of the delivered messages, how many to Skip before ack
                , 45000 //Of the delivered messages, how many to consume
                , 0 //Of the delivered messages, how many to mark received
                , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
                , 0 //Of the delivered messages, how many to return to caller un(n)acked
                , 0 //Of the delivered messages, how many to nack not received
                , 0 //Of the delivered messages, how many to nack not sent
                , NULL);//Handles of messages returned to us un(n)acked

    rc = sync_ism_engine_destroyClientState(hClient,
                       ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);
    test_log(testLOGLEVEL_TESTNAME, "Finished %s...\n", __func__);
}

CU_TestInfo ISM_ExportResources_CUnit_test_capability_Basic[] =
{    { "recreate183547",                test_capability_183547 },
     CU_TEST_INFO_NULL
};


/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initSuite(void)
{
    return test_engineInit(true, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termSuite(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_ExportResources_CUnit_183547[] =
{
    IMA_TEST_SUITE("Basic", initSuite, termSuite, ISM_ExportResources_CUnit_test_capability_Basic),
    CU_SUITE_INFO_NULL,
};


int main(int argc, char *argv[])
{
    int32_t trclvl = 0;
    uint32_t testLogLevel = testLOGLEVEL_TESTPROGRESS;
    int retval = 0;
    char *adminDir=NULL;

    ism_time_t seedVal = ism_common_currentTimeNanos();

    srand(seedVal);

    CU_initialize_registry();

    //We're going to put a lot of persistent messages - want to use the thread jobs model
    setenv("IMA_TEST_TRANSACTION_THREAD_JOBS_CLIENT_MINIMUM", "1", 0);

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, adminDir);

    char localAdminDir[1024];
    if (adminDir == NULL && test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir)) == true)
    {
        adminDir = localAdminDir;
    }

    if (retval == 0)
    {
        CU_register_suites(ISM_ExportResources_CUnit_183547);

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

    test_log(2, "Success. Test complete!\n");
    test_processTerm(false);
    test_removeAdminDir(adminDir);

    return retval;
}
