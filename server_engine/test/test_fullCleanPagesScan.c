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
/* Module Name: test_fullCleanPagesScan.c                            */
/*                                                                   */
/* Description: Main source file for CUnit test of freeing memory    */
/*              when some pages at the start of the queue can't be   */
/*              freed.                                               */
/*********************************************************************/

#include "engineInternal.h"

#include "test_utils_initterm.h"
#include "test_utils_sync.h"
#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_client.h"
#include "test_utils_options.h"
#include "test_utils_pubsuback.h"

typedef struct tag_consumerContext_t {
    ismEngine_SessionHandle_t hSession;
    ismEngine_DeliveryHandle_t *pCallerHandles;
    uint64_t numToHaveDelivered;
    volatile uint64_t numDelivered;
} consumerContext_t;


static bool getMessagesCallback(
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
    consumerContext_t *context = *(consumerContext_t **)pConsumerContext;

    //Num delivered is only changed here under the wait list lock so we don't need to
    //worry about races
    bool keepingGoing = ((context->numDelivered +1) < context->numToHaveDelivered);

    test_log(testLOGLEVEL_VERBOSE, "Client %s: msg oId %lu arrived in state %u",
               hConsumer->pSession->pClient->pClientId, pMsgDetails->OrderId, state);


    //If we are stopping, do it *before* we increase count so it's done before the
    //main thread stops waiting...
    if (!keepingGoing)
    {
        ism_engine_suspendMessageDelivery(hConsumer,
                                          ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
    }
    context->pCallerHandles[context->numDelivered] = hDelivery;

    __sync_add_and_fetch(&(context->numDelivered), 1);

    ism_engine_releaseMessage(hMessage);

    return keepingGoing;
}


// Test proves that a redelivery cursor can be pointing at pages freed
// when we do a full page scan but that redeliver proceeds correctly
//
// 0)  Have a shared sub with 1000 messages and two clients connected to it
// 1)  Deliver msg1 to app0 (but don't ack it)
// 2)  Deliver 500 messages to app1 but don't ack them
// 3)  Deliver exactly whole batch to app0 but don't ack them
// 4)  Deliver all but 1 of the remaining messages to app1 but don't ack them
// 5)  Deliver last message to app0 but don't ack it
// 6)  Disconnect and reconnect app0
// 7)  app0 is redelivered msg1 but doesn't ack it
// 8)  app0 is redelivered a message batch, disables it's consumer and acks the batch
// 9)  app1 acks all its messages, should cause full cleanpages scan
// 10) app0 re-enables itself and is redelivered last message

void test_capability_redeliveryDuringFullClean(void)
{
    #define NUM_CLIENTS 2
    ismEngine_ClientStateHandle_t hClient[NUM_CLIENTS];
    ismEngine_SessionHandle_t hClientSession[NUM_CLIENTS];
    char *ClientIds[NUM_CLIENTS] = { "Client0_SparseAck"
                                   , "Client1_DenseAck" };

    int32_t rc;

    char *subName = "redeliveryDuringFullClean";
    char *topicString = "/redeliveryDuringFullClean";

    uint32_t msgsUnDeliveredOnQ = 1000;
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    //Setup the client that will own the shared namespace
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSharedOwningSession;

    rc = test_createClientAndSession("__SharedRedelivFullClean",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hSharedOwningClient, &hSharedOwningSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningSession);

    // 0)  Have a shared sub with 1000 messages and two clients connected to it
    for (uint32_t i = 0 ; i < NUM_CLIENTS; i++)
    {
        rc = test_createClientAndSession(ClientIds[i],
                NULL,
                ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                ismENGINE_CREATE_SESSION_OPTION_NONE,
                &(hClient[i]), &(hClientSession[i]), true);
        TEST_ASSERT_EQUAL(rc, OK);

        test_makeSub( subName
                    , topicString
                    ,    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE
                       | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                       | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                       | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
                    , hClient[i]
                    , hSharedOwningClient);
    }

    //Make sure we can create a lot of messages on client1 without acking them
    hClient[1]->expectedMsgRate  = EXPECTEDMSGRATE_HIGH;
    hClient[1]->maxInflightLimit = 1024;

    test_pubMessages(topicString,
                    ismMESSAGE_PERSISTENCE_PERSISTENT,
                    ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                    msgsUnDeliveredOnQ);

    // 1)  Deliver msg1 to app0 (but don't ack it) and leave consumer existing but disabled
    test_markMsgs( subName
                 , hClientSession[0]
                 , hSharedOwningClient
                 , 1 //Total messages we have delivered before disabling consumer
                 , 0 //Of the delivered messages, how many to Skip before ack
                 , 0 //Of the delivered messages, how many to consume
                 , 0 //Of the delivered messages, how many to mark received
                 , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
                 , 0 //Of the delivered messages, how many to return to caller un(n)acked
                 , 0 //Of the delivered messages, how many to nack not received
                 , 0 //Of the delivered messages, how many to nack not sent
                 , NULL); //Handles of messages returned to us un(n)acked
    msgsUnDeliveredOnQ -= 1;

    // 2)  Deliver 500 messages to app1 but don't ack them
    uint32_t app1_ackBatch1 = 500;

    ismEngine_DeliveryHandle_t app1_ackHandles_batch1[app1_ackBatch1];
    test_markMsgs( subName
                 , hClientSession[1]
                 , hSharedOwningClient
                 , app1_ackBatch1 //Total messages we have delivered before disabling consumer
                 , 0   //Of the delivered messages, how many to Skip before ack
                 , 0   //Of the delivered messages, how many to consume
                 , 0   //Of the delivered messages, how many to mark received
                 , 0   //Of the delivered messages, how many to Skip before handles returned to us/nacked
                 , app1_ackBatch1 //Of the delivered messages, how many to return to caller un(n)acked
                 , 0   //Of the delivered messages, how many to nack not received
                 , 0   //Of the delivered messages, how many to nack not sent
                 , app1_ackHandles_batch1); //Handles of messages returned to us un(n)acked
    msgsUnDeliveredOnQ -= app1_ackBatch1;

    // 3)  Deliver exactly whole batch to app0 but don't ack them
    uint32_t app0_ackBatch1 = test_getBatchSize(hClientSession[0]);

    test_markMsgs( subName
                 , hClientSession[0]
                 , hSharedOwningClient
                 , app0_ackBatch1 //Total messages we have delivered before disabling consumer
                 , 0   //Of the delivered messages, how many to Skip before ack
                 , 0   //Of the delivered messages, how many to consume
                 , 0   //Of the delivered messages, how many to mark received
                 , 0   //Of the delivered messages, how many to Skip before handles returned to us/nacked
                 , 0   //Of the delivered messages, how many to return to caller un(n)acked
                 , 0   //Of the delivered messages, how many to nack not received
                 , 0   //Of the delivered messages, how many to nack not sent
                 , NULL); //Handles of messages returned to us un(n)acked
    msgsUnDeliveredOnQ -= app0_ackBatch1;

    // 4)  Deliver all but 1 of the remaining messages to app1 but don't ack them
    uint32_t app1_ackBatch2 = msgsUnDeliveredOnQ - 1;

    ismEngine_DeliveryHandle_t app1_ackHandles_batch2[app1_ackBatch2];
    test_markMsgs( subName
                 , hClientSession[1]
                 , hSharedOwningClient
                 , app1_ackBatch2 //Total messages we have delivered before disabling consumer
                 , 0   //Of the delivered messages, how many to Skip before ack
                 , 0   //Of the delivered messages, how many to consume
                 , 0   //Of the delivered messages, how many to mark received
                 , 0   //Of the delivered messages, how many to Skip before handles returned to us/nacked
                 , app1_ackBatch2 //Of the delivered messages, how many to return to caller un(n)acked
                 , 0   //Of the delivered messages, how many to nack not received
                 , 0   //Of the delivered messages, how many to nack not sent
                 , app1_ackHandles_batch2); //Handles of messages returned to us un(n)acked
    msgsUnDeliveredOnQ -= app1_ackBatch2;

    // 5)  Deliver last message to app0 but don't ack it
    test_markMsgs( subName
                 , hClientSession[0]
                 , hSharedOwningClient
                 , 1 //Total messages we have delivered before disabling consumer
                 , 0 //Of the delivered messages, how many to Skip before ack
                 , 0 //Of the delivered messages, how many to consume
                 , 0 //Of the delivered messages, how many to mark received
                 , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
                 , 0 //Of the delivered messages, how many to return to caller un(n)acked
                 , 0 //Of the delivered messages, how many to nack not received
                 , 0 //Of the delivered messages, how many to nack not sent
                 , NULL); //Handles of messages returned to us un(n)acked
    msgsUnDeliveredOnQ -= 1;

    TEST_ASSERT_EQUAL(msgsUnDeliveredOnQ, 0);

    // 6)  Disconnect and reconnect app0

    rc = sync_ism_engine_destroyClientState(hClient[0],
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession(ClientIds[0],
            NULL,
            ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
            ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
            &(hClient[0]), &(hClientSession[0]), true);
    TEST_ASSERT_EQUAL(rc, OK);

    // 7)  app0 is redelivered msg1 but doesn't ack it
    ismEngine_ConsumerHandle_t hConsumer = NULL;
    ismEngine_DeliveryHandle_t firstAckHandle[1] = {0};

    consumerContext_t consumerContext = {0};
    consumerContext_t *pConsumerContext = &consumerContext;


    consumerContext.hSession                = hClientSession[0];
    consumerContext.numToHaveDelivered      = 1;
    consumerContext.pCallerHandles          = firstAckHandle;

    //Use short delivery id == mdr
    rc = ism_engine_createConsumer(
            hClientSession[0],
            ismDESTINATION_SUBSCRIPTION,
            subName,
            0,
            hSharedOwningClient,
            &pConsumerContext,
            sizeof(consumerContext_t *),
            getMessagesCallback,
            NULL,
            ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
            &hConsumer,
            NULL,
            0,
            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_waitForMessages64(&(consumerContext.numDelivered),NULL, 1, 20);

    // 8)  app0 is redelivered a message batch, disables its consumer and acks the batch

    ismEngine_DeliveryHandle_t app0_BatchHandles[app0_ackBatch1];

    consumerContext.numToHaveDelivered      = app0_ackBatch1;
    consumerContext.pCallerHandles          = app0_BatchHandles;
    consumerContext.numDelivered            = 0;

    rc = ism_engine_startMessageDelivery(hClientSession[0],
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);

    test_waitForMessages64(&(consumerContext.numDelivered),NULL, app0_ackBatch1, 20);

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    test_incrementActionsRemaining(pActionsRemaining, app0_ackBatch1);

    for (uint32_t i=0 ; i < app0_ackBatch1; i++ )
    {
        rc = ism_engine_confirmMessageDelivery(hClientSession[0],
                                               NULL,
                                               app0_BatchHandles[i],
                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                               &pActionsRemaining, sizeof(pActionsRemaining),
                                               test_decrementActionsRemaining);

        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }

    test_waitForActionsTimeOut(pActionsRemaining, 20);


    // 9)  app1 acks all its messages, should cause full cleanpages scan
    test_incrementActionsRemaining(pActionsRemaining, app1_ackBatch1);

    for (uint32_t i=0 ; i < app1_ackBatch1; i++ )
    {
        rc = ism_engine_confirmMessageDelivery(hClientSession[1],
                                               NULL,
                                               app1_ackHandles_batch1[i],
                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                               &pActionsRemaining, sizeof(pActionsRemaining),
                                               test_decrementActionsRemaining);

        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }

    test_incrementActionsRemaining(pActionsRemaining, app1_ackBatch2);

    for (uint32_t i=0 ; i < app1_ackBatch2; i++ )
    {
        rc = ism_engine_confirmMessageDelivery(hClientSession[1],
                                               NULL,
                                               app1_ackHandles_batch2[i],
                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                               &pActionsRemaining, sizeof(pActionsRemaining),
                                               test_decrementActionsRemaining);

        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }

    test_waitForActionsTimeOut(pActionsRemaining, 20);

    // 10) app0 re-enables itself and is redelivered last message
    ismEngine_DeliveryHandle_t LastAckHandle[1] = {0};
    consumerContext.numToHaveDelivered      = 1;
    consumerContext.pCallerHandles          = LastAckHandle;
    consumerContext.numDelivered            = 0;

    rc = ism_engine_startMessageDelivery(hClientSession[0],
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);

    test_waitForMessages64(&(consumerContext.numDelivered),NULL, 1, 20);


    test_incrementActionsRemaining(pActionsRemaining, 1);

    rc = ism_engine_confirmMessageDelivery(hClientSession[0],
                                           NULL,
                                           LastAckHandle[0],
                                           ismENGINE_CONFIRM_OPTION_CONSUMED,
                                           &pActionsRemaining, sizeof(pActionsRemaining),
                                           test_decrementActionsRemaining);

    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForActionsTimeOut(pActionsRemaining, 20);


    // fini
    for (uint32_t i=0 ; i < NUM_CLIENTS; i++ )
    {
        rc = sync_ism_engine_destroyClientState(hClient[i],
                                           ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = sync_ism_engine_destroyClientState(hSharedOwningClient,
            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);

    test_log(testLOGLEVEL_TESTNAME, "Completed %s\n", __func__);
}

CU_TestInfo ISM_ExportResources_CUnit_test_capability_main[] =
{
    { "redeliveryDuringFullClean",          test_capability_redeliveryDuringFullClean },
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
   //Do nothing we want the engine running at restart
   return 0;
}


CU_SuiteInfo fullClean_allSuites[] =
{
    IMA_TEST_SUITE("Main", initSuite, termSuite, ISM_ExportResources_CUnit_test_capability_main),
    CU_SUITE_INFO_NULL,
};





int32_t parse_args( int argc
                  , char **argv
                  , uint32_t *pPhase
                  , uint32_t *pFinalPhase
                  , uint32_t *pTestLogLevel
                  , int32_t *pTraceLevel
                  , char **adminDir)
{
    int retval = 0;

    if (argc > 1)
    {
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
            else if (argv[i][0] == '-')
            {
                //All these args are of the form -<flag> <Value> so check we have a value:
                if ((i+1) >= argc)
                {
                    printf("Found a flag: %s but there was no following arg!\n", argv[i]);
                    retval = 96;
                    break;
                }

                if (!strcasecmp(argv[i], "-a"))
                {
                    //Admin dir...
                    i++;
                    *adminDir = argv[i];
                }
                else if (!strcasecmp(argv[i], "-p"))
                {
                    //start phase...
                    i++;
                    *pPhase = (uint32_t)atoi(argv[i]);

                }
                else if (!strcasecmp(argv[i], "-f"))
                {
                    //final phase...
                    i++;
                    *pFinalPhase = (uint32_t)atoi(argv[i]);
                }
                else if (!strcasecmp(argv[i], "-v"))
                {
                    //final phase...
                    i++;
                    *pTestLogLevel= (uint32_t)atoi(argv[i]);
                }
                else if (!strcasecmp(argv[i], "-t"))
                {
                    i++;
                    *pTraceLevel= (uint32_t)atoi(argv[i]);
                }
                else
                {
                    printf("Unknown flag arg: %s\n", argv[i]);
                    retval = 95;
                    break;
                }
            }
            else
            {
                printf("Unknown arg: %s\n", argv[i]);
                retval = 94;
                break;
            }
        }
    }
    return retval;
}


int main(int argc, char *argv[])
{
    int32_t trclvl = 0;
    uint32_t testLogLevel = testLOGLEVEL_TESTDESC;
    int retval = 0;
    char *adminDir=NULL;
    uint32_t phase=0;
    uint32_t finalPhase=1;

    ism_time_t seedVal = ism_common_currentTimeNanos();

    srand(seedVal);

    CU_initialize_registry();

    retval = parse_args( argc
                       , argv
                       , &phase
                       , &finalPhase
                       , &testLogLevel
                       , &trclvl
                       , &adminDir);
    if (retval != OK) goto mod_exit;

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, adminDir);

    char localAdminDir[1024];
    if (adminDir == NULL && test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir)) == true)
    {
        adminDir = localAdminDir;
    }

    if (retval == 0)
    {
        CU_register_suites(fullClean_allSuites);

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

mod_exit:

    return retval;
}
