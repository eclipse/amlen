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
/* Module Name: test_PMR27280.499.000.c                              */
/*                                                                   */
/* Description: Test of the problem raised by PMR 27280,499,000      */
/*                                                                   */
/*********************************************************************/
#include "engineInternal.h"

#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_task.h"

typedef struct tag_msgDeliveryCBContext_t
{
    uint32_t useCount;
    ismEngine_SessionHandle_t hSession;
    int32_t received;
} msgDeliveryCBContext_t;

bool msgDeliveryCB(ismEngine_ConsumerHandle_t      hConsumer,
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
                   void *                          pContext)
{
    msgDeliveryCBContext_t *context = *((msgDeliveryCBContext_t **)pContext);

    __sync_fetch_and_add(&context->received, 1);

    ism_engine_releaseMessage(hMessage);

    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        uint32_t rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                        NULL,
                                                        hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                        NULL,
                                                        0,
                                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    return true; // more messages, please.
}

typedef struct tag_pmr27280499000_publisherInfo_t
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    pthread_t threadId;
    const char *topicString;
    volatile bool stopNow;
} pmr27820499000_publisherInfo_t;

typedef struct tag_pmr27280499000_consumerInfo_t
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    pthread_t threadId[2];
    const char *topicString;
    volatile bool stopNow;
} pmr27280499000_consumerInfo_t;

static void *pmr27280499000_publisher(void *threadarg)
{
    pmr27820499000_publisherInfo_t *publisherInfo = (pmr27820499000_publisherInfo_t *)threadarg;

    ism_engine_threadInit(0);

    while(publisherInfo->stopNow == false)
    {
        ismEngine_MessageHandle_t hMessage;
        void *payload=NULL;

        int32_t rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                        ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                        ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                        ismMESSAGE_FLAGS_NONE,
                                        0,
                                        ismDESTINATION_TOPIC, publisherInfo->topicString,
                                        &hMessage, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination(publisherInfo->hSession,
                                                ismDESTINATION_TOPIC,
                                                publisherInfo->topicString,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);

        if (payload) free(payload);

        usleep(rand()%100);
    }

    return NULL;
}

void pmr27280499000_destroyConsumerCB(int32_t retcode, void *handle, void *pContext)
{
    msgDeliveryCBContext_t *consumerCB = *(msgDeliveryCBContext_t **)pContext;
    __sync_sub_and_fetch(&consumerCB->useCount, 1);
}

static void *pmr27280499000_consumer(void *threadarg)
{
    msgDeliveryCBContext_t consumerContext = {0};
    pmr27280499000_consumerInfo_t *consumerInfo = (pmr27280499000_consumerInfo_t *)threadarg;

    ism_engine_threadInit(0);

    while(consumerInfo->stopNow == false)
    {
        ismEngine_ConsumerHandle_t hConsumer;
        msgDeliveryCBContext_t *consumerCB = &consumerContext;
        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
        consumerContext.hSession = consumerInfo->hSession;

        // Decremented when the consumer is finally destroyed
        __sync_add_and_fetch(&consumerContext.useCount, 1);

        // This will result in an ism_engine_registerSubscriber call
        int32_t rc = ism_engine_createConsumer(consumerInfo->hSession,
                                               ismDESTINATION_TOPIC,
                                               consumerInfo->topicString,
                                               &subAttrs,
                                               NULL, // Unused for TOPIC
                                               &consumerCB,
                                               sizeof(msgDeliveryCBContext_t *),
                                               msgDeliveryCB,
                                               NULL,
                                               ismENGINE_CONSUMER_OPTION_NONE,
                                               &hConsumer,
                                               NULL,
                                               0,
                                               NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        usleep(rand()%100);

        rc = ism_engine_destroyConsumer(hConsumer,
                                        &consumerCB,
                                        sizeof(msgDeliveryCBContext_t *),
                                        pmr27280499000_destroyConsumerCB);

        if (rc == OK) pmr27280499000_destroyConsumerCB(rc, NULL, &consumerCB);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }

    // Wait for the destroy of all consumers to finish
    while(consumerContext.useCount != 0)
    {
        sched_yield();
        usleep(100);
        ismEngine_FullMemoryBarrier();
    }

    return NULL;
}

static void *pmr27280499000_startMsgDelivery(void *threadarg)
{
    pmr27280499000_consumerInfo_t *consumerInfo = (pmr27280499000_consumerInfo_t *)threadarg;

    ism_engine_threadInit(0);

    while(consumerInfo->stopNow == false)
    {
        int32_t rc = ism_engine_startMessageDelivery(consumerInfo->hSession,
                                                     ismENGINE_START_DELIVERY_OPTION_NONE,
                                                     NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        usleep(rand()%100);
    }

    return NULL;
}

void test_pmr27280499000(void)
{
    pmr27820499000_publisherInfo_t publisherInfo = {0};
    pmr27280499000_consumerInfo_t consumerInfo = {0};

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    /* Create a client / session to publish messages */
    rc = test_createClientAndSession("OneClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &publisherInfo.hClient, &publisherInfo.hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    /* Create a client / session to perform consumer actions on */
    rc = test_createClientAndSession("AnotherClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &consumerInfo.hClient, &consumerInfo.hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    publisherInfo.topicString = consumerInfo.topicString = "DEFECT/PMR/27280/499/000";

    // Start the publishing thread
    rc = test_task_startThread(&publisherInfo.threadId,pmr27280499000_publisher, &publisherInfo,"pmr27280499000_publisher");
    TEST_ASSERT_EQUAL(rc, OK);

    // Start the consumer creation thread
    rc = test_task_startThread(&consumerInfo.threadId[0],pmr27280499000_consumer, &consumerInfo,"pmr27280499000_consumer");
    TEST_ASSERT_EQUAL(rc, OK);

    // Start the message delivery starting thread
    rc = test_task_startThread(&consumerInfo.threadId[1],pmr27280499000_startMsgDelivery, &consumerInfo,"pmr27280499000_startMsgDelivery");
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t sleepSeconds = 10;

    printf("Allowing threads to run for %d seconds...\n", sleepSeconds);

    sleep(sleepSeconds);

    publisherInfo.stopNow = consumerInfo.stopNow = true;

    printf("Waiting for threads to end\n");

    // Wait for the publisher thread to end
    pthread_join(publisherInfo.threadId, NULL);

    rc = test_destroyClientAndSession(publisherInfo.hClient, publisherInfo.hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Wait for the consumer & delivery starting threads to end

    pthread_join(consumerInfo.threadId[0], NULL);
    pthread_join(consumerInfo.threadId[1], NULL);

    rc = test_destroyClientAndSession(consumerInfo.hClient, consumerInfo.hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_CUnit_test_defects[] =
{
    { "PMR 27280,499,000", test_pmr27280499000 },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initTopicTree(void)
{
    return test_engineInit_DEFAULT;
}

int termTopicTree(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_TopicTree_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("Defects", initTopicTree, termTopicTree, ISM_CUnit_test_defects),
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

    retval = setup_CU_registry(argc, argv, ISM_TopicTree_CUnit_allsuites);

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
