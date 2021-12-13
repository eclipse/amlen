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
#include <sys/prctl.h>

#include "engine.h"
#include "engineInternal.h"
#include "policyInfo.h" //For setting maxdepth to infinity..
#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_client.h"
#include "test_utils_initterm.h"
#include "test_utils_sync.h"
#include "test_utils_task.h"

extern void ism_common_traceSignal(int signo);

/*
 * Catch a terminating signal and try to flush the trace file.
 */
static void sig_handler(int signo) {
    static volatile int in_handler = 0;
    signal(signo, SIG_DFL);
    if (in_handler)
        raise(signo);
    in_handler = 1;
    ism_common_traceSignal(signo);
    raise(signo);
}

typedef struct tag_PublisherInfo_t {
    char *topicString;
    bool keepGoing;
} PublisherInfo_t;

typedef struct tag_ConsumerInfo_t {
    volatile uint32_t *pMsgsReceived;
    uint32_t minUnackedMsgs; //The consumer will receive (and not ack) at least this many messages before suspending
} ConsumerInfo_t ;



void *publishToTopic(void *args)
{
    PublisherInfo_t *pubInfo =(PublisherInfo_t *)args;
    char *tname="pubbingthread";
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)tname);
    int32_t rc = OK;

    ism_engine_threadInit(0);

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    rc = test_createClientAndSession("PubbingClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    while(pubInfo->keepGoing)
    {
        ismEngine_MessageHandle_t hMessage;

        // Publish a persistent message
        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, pubInfo->topicString,
                                &hMessage, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                pubInfo->topicString,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT(rc < ISMRC_Error, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
    }

    return NULL;
}

void waitForCompletion(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext)
{
    TEST_ASSERT_EQUAL(retcode, OK);

    bool **ppIsCompleted = (bool **)pContext;
    **ppIsCompleted = true;
}


bool ConsumeNoAckExplicitSuspend(
    ismEngine_ConsumerHandle_t      hConsumer,
    ismEngine_DeliveryHandle_t      hDelivery,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        deliveryId,
    ismMessageState_t               state,
    uint32_t                        destinationOptions,
    ismMessageHeader_t *            pMsgDetails,
    uint8_t                         areaCount,
    ismMessageAreaType_t            areaTypes[areaCount],
    size_t                          areaLengths[areaCount],
    void *                          pAreaData[areaCount],
    void *                          pConsumerContext,
    ismEngine_DelivererContext_t *  _delivererContext )
{
    bool keepingGoing = true;
    ConsumerInfo_t *pConsumerInfo =*(ConsumerInfo_t **)pConsumerContext;

    ism_engine_releaseMessage(hMessage);

    (*(pConsumerInfo->pMsgsReceived)) += 1;

    if ( (*(pConsumerInfo->pMsgsReceived)) > pConsumerInfo->minUnackedMsgs)
    {
        keepingGoing = false;
        ism_engine_suspendMessageDelivery(hConsumer, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
    }
    return keepingGoing;
}

void test_31356(void)
{
    volatile uint32_t msgsReceived = 0;
    uint32_t minUnackedMsgs = 3; //The consumer will receive (and not ack) at least this many messages before suspending

    char *topicString="/test/forDefect31356";
    char *subName="defect31356Sub";

    //Set up a thread publishing to a topic...
    pthread_t publishingThreadId;
    PublisherInfo_t pubberInfo = {topicString, true};
    int os_rc = test_task_startThread( &publishingThreadId,publishToTopic, &pubberInfo,"publishToTopic");
    TEST_ASSERT_EQUAL(os_rc, 0);

    ConsumerInfo_t consInfo = {&msgsReceived, minUnackedMsgs};
    ConsumerInfo_t *pConsInfo = &consInfo;

    // Set default maxMessageCount to 0 for the duration of the test
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 0;

    //Loop trying the test lots of times......
    for(uint32_t i=0; i<6000; i++)
    {
        test_log((((i & 1023) == 0) ? testLOGLEVEL_TESTPROGRESS:testLOGLEVEL_VERBOSE), "Starting loop %u", i);

        //Reset messages received by consumer...
        msgsReceived = 0;

        ismEngine_ClientStateHandle_t hClient;
        ismEngine_SessionHandle_t hSession;
        ismEngine_ConsumerHandle_t hConsumer;

        int32_t rc = test_createClientAndSession("SubbingClient",
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                 ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                                 &hClient, &hSession, true);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hClient);
        TEST_ASSERT_PTR_NOT_NULL(hSession);


        //Create a durable subscription and consumer...
        //(after receiving minUnackedMessages... consumer will randomly choose when do suspend...
        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                        ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE };

        rc = sync_ism_engine_createSubscription(hClient,
                                                subName,
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                &subAttrs,
                                                NULL); // Owning client same as requesting client
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_createConsumer( hSession
                                      , ismDESTINATION_SUBSCRIPTION
                                      , subName
                                      , NULL
                                      , NULL // Owning client same as session client
                                      , &pConsInfo
                                      , sizeof(pConsInfo)
                                      , ConsumeNoAckExplicitSuspend
                                      , NULL
                                      , ismENGINE_CONSUMER_OPTION_ACK
                                      , &hConsumer
                                      , NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        //Wait for consumer to have minimum number of unacked messages...
        while(msgsReceived <= minUnackedMsgs);

        //Destroy consumer
        rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);

        TEST_ASSERT_CUNIT( ((rc==OK)||(rc==ISMRC_AsyncCompletion))
                         , ("rc was %d\n", rc));

        test_log(testLOGLEVEL_VERBOSE, "destroyConsumer=%d", rc);

        //Destroy session...
        rc = ism_engine_destroySession(hSession,  NULL, 0, NULL);

        TEST_ASSERT_CUNIT( ((rc==OK)||(rc==ISMRC_AsyncCompletion))
                         , ("rc was %d\n", rc));

        test_log(testLOGLEVEL_VERBOSE, "destroySess=%d", rc);

        //Destroy durable subscription...
        volatile bool subDestroyed = false;
        volatile bool *pSubDestroyed = &subDestroyed;

        while (!subDestroyed)
        {
            rc = ism_engine_destroySubscription( hClient
                                               , subName
                                               , hClient
                                               , &pSubDestroyed
                                               , sizeof(pSubDestroyed)
                                               , waitForCompletion);
            if (rc == ISMRC_AsyncCompletion)
            {
                //wait for durable subscription to be removed...we
                //don't want to call destroy again
                while(!subDestroyed);
            }
            else if (rc == ISMRC_DestinationInUse)
            {
                //subscription not removed...loop around and try again...
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, OK);
                subDestroyed = true;
            }
        }
        test_log(testLOGLEVEL_VERBOSE, "subDestroyed");

        //destroyclient....
        volatile bool clientDestroyed = false;
        volatile bool *pClientDestroyed = &clientDestroyed;

        rc = ism_engine_destroyClientState( hClient
                                            , ismENGINE_DESTROY_CLIENT_OPTION_NONE
                                            , &pClientDestroyed
                                            , sizeof(pClientDestroyed)
                                            , waitForCompletion);
        //...if async... wait for the client destruction to complete..
        if (rc == ISMRC_AsyncCompletion)
        {
            //wait for client to go away...
            while(!clientDestroyed);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    //Tell publish thread to end...
    pubberInfo.keepGoing = false;
    os_rc = pthread_join(publishingThreadId, NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);
}


int initSuite(void)
{
    int rc = test_engineInit(true, true,
                             true, // Disable Auto creation of named queues
                             false, /*recovery should complete ok*/
                             ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                             testDEFAULT_STORE_SIZE);

    signal(SIGINT, sig_handler);
    signal(SIGSEGV, sig_handler);

    return rc;
}

int termSuite(void)
{
    return test_engineTerm(true);
}

CU_TestInfo ISM_Test31356_CUnit_test_31356[] =
{
    { "test_31356", test_31356 },
    CU_TEST_INFO_NULL
};

CU_SuiteInfo ISM_Test31356_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("Test31356Suite", initSuite, termSuite, ISM_Test31356_CUnit_test_31356),
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
    int testLogLevel = testLOGLEVEL_TESTPROGRESS;

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_Test31356_CUnit_allsuites);

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
