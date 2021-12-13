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
/********************************************************************/
/*                                                                  */
/* Module Name: testStoreEvents                                     */
/*                                                                  */
/* Description: This test starts creates 'N' durable subscriptions  */
/*              and begins publishing messages to their topic,      */
/*              each message within it's own transaction which is   */
/*              not committed.                                      */
/*              When a request to create a messages fails with      */
/*              return code ISMRC_ServerCapacity then the tran-    */
/*              sactions are committed which should allow process-  */
/*              ing to continue.                                    */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <getopt.h>

#include <ismutil.h>
#include "engine.h"
#include "engineInternal.h"
#include "engineCommon.h"
#include "policyInfo.h"
#include "queueCommon.h"

#include "test_utils_initterm.h"
#include "test_utils_log.h"
#include "test_utils_task.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

/********************************************************************/
/* Function prototypes                                              */
/********************************************************************/
int ProcessArgs( int argc
               , char **argv
               , bool *pRunTest1
               , bool *pRunTest2
               , bool *pRunTest3
               , bool *pRunTest4 );

bool ConsumerCallback( ismEngine_ConsumerHandle_t hConsumer
                     , ismEngine_DeliveryHandle_t hDelivery
                     , ismEngine_MessageHandle_t  hMessage
                     , uint32_t                   deliveryId
                     , ismMessageState_t          state
                     , uint32_t                   destinationOptions
                     , ismMessageHeader_t *       pMsgDetails
                     , uint8_t                    areaCount
                     , ismMessageAreaType_t       areaTypes[areaCount]
                     , size_t                     areaLengths[areaCount]
                     , void *                     pAreaData[areaCount]
                     , void *                     pConsumerContext
                     , ismEngine_DelivererContext_t * _delivererContext );

/********************************************************************/
/* Global data                                                      */
/********************************************************************/
static ismEngine_ClientStateHandle_t hClient = NULL;

static uint32_t logLevel = testLOGLEVEL_CHECK;

#define testClientId "StoreEvents Test"

//****************************************************************************
/// @brief callback function used to accept an asynchronous operation
//****************************************************************************
static void test_asyncCallback(int32_t rc, void *handle, void *pContext)
{
   uint32_t *callbacksReceived = *(uint32_t **)pContext;
   TEST_ASSERT(rc == OK, ("ERROR: rc == %d\n", rc));;
   TEST_ASSERT(handle == NULL, ("ERROR: handle == %p\n", handle));
   __sync_fetch_and_add(callbacksReceived, 1);
}

//****************************************************************************
/// @brief Wait for some number of operations to complete
//****************************************************************************
static void test_waitForAsyncCallbacks(uint32_t *pCallbacksReceived, uint32_t expectedCallbacks)
{
    while((volatile uint32_t)*pCallbacksReceived < expectedCallbacks)
    {
        usleep(10);
    }
}

/****************************************************************************/
/* ConsumerCallback                                                         */
/****************************************************************************/
bool ConsumerCallback( ismEngine_ConsumerHandle_t hConsumer
                     , ismEngine_DeliveryHandle_t hDelivery
                     , ismEngine_MessageHandle_t  hMessage
                     , uint32_t                   deliveryId
                     , ismMessageState_t          state
                     , uint32_t                   destinationOptions
                     , ismMessageHeader_t *       pMsgDetails
                     , uint8_t                    areaCount
                     , ismMessageAreaType_t       areaTypes[areaCount]
                     , size_t                     areaLengths[areaCount]
                     , void *                     pAreaData[areaCount]
                     , void *                     pConsumerContext
                     , ismEngine_DelivererContext_t * _delivererContext )
{
    ismEngine_SessionHandle_t hSession = (ismEngine_SessionHandle_t)pConsumerContext;

    ism_engine_releaseMessage(hMessage);

    if (state == ismMESSAGE_STATE_DELIVERED)
    {
        int32_t rc = ism_engine_confirmMessageDelivery( hSession
                                               , NULL
                                               , hDelivery
                                               , ismENGINE_CONFIRM_OPTION_CONSUMED
                                               , NULL
                                               , 0
                                               , NULL );
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion,
                      ("ERROR:  ism_engine_confirmMessageDelivery() returned %d\n", rc));

    }

    return true;
}

int commitTransactions(ismEngine_SessionHandle_t hSession,
                       ismEngine_TransactionHandle_t *tranTable,
                       int Count)
{
    int loop;
    int rc;

    test_log(testLOGLEVEL_TESTPROGRESS, "Committing %d transactions", Count);
    for (loop=0; loop < Count; loop++)
    {
        rc = ism_engine_commitTransaction( hSession
                                         , tranTable[loop]
                                         , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT
                                         , NULL
                                         , 0
                                         , NULL );
        if (rc != 0)
        {
            fprintf(stderr, "failed to commit transaction. rc=%d\n", rc);
        }
        assert(rc == 0);
    }
    return 0;
}

uint32_t TransactionalPublishTest( ismEngine_SessionHandle_t hSession
                                 , uint32_t maxIterations )
{
    ismEngine_ConsumerHandle_t hConsumer = NULL;
    char subName[]="TransactionalPublishTest";
    int32_t rc;
    int32_t pubCount = 0;
    int32_t iterationCount = 0;
    ismEngine_TransactionHandle_t *tranTable = NULL;
    char TopicString[]="/Test/StoreEvents";

    test_log(testLOGLEVEL_TESTDESC, "Create large number of transactions each containing a single publication request.");

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };

    rc=sync_ism_engine_createSubscription( hClient
                                         , subName
                                         , NULL
                                         , ismDESTINATION_TOPIC
                                         , TopicString
                                         , &subAttrs
                                         , NULL ); // Owning client same as requesting client
    TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createSubscription() returned %d\n", rc));

    // Create the consumer
    rc=ism_engine_createConsumer( hSession
                                , ismDESTINATION_SUBSCRIPTION
                                , subName
                                , NULL
                                , NULL // Owning client same as session client
                                , (void *)hSession
                                , 0
                                , ConsumerCallback
                                , NULL
                                , test_getDefaultConsumerOptions(subAttrs.subOptions)
                                , &hConsumer
                                , NULL
                                , 0
                                , NULL);
    TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createConsumer() returned %d\n", rc));

    test_log(testLOGLEVEL_VERBOSE, "Created subscription on Topic %s",
             TopicString);
  
    /************************************************************************/
    /* Start the message delivery                                           */
    /************************************************************************/
    rc = ism_engine_startMessageDelivery( hSession
                                      , 0
                                      , NULL
                                      , 0
                                      , NULL);
    TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_startMessageDelivery() returned %d\n", rc));

    tranTable = (ismEngine_TransactionHandle_t *)malloc(sizeof(ismEngine_TransactionHandle_t) * 10240);
    assert(tranTable != NULL);
    int tranTableSize = 10240;
    int tranIndex = 0;

    while (iterationCount < maxIterations)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismEngine_MessageHandle_t  hMessage;
        ismMessageAreaType_t areaTypes[] = { ismMESSAGE_AREA_PAYLOAD };
        char Buffer[128];
        void *areas[1];
        size_t areaLengths[1];

        pubCount++;

        header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
        header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
        sprintf(Buffer, "Test Store Events message - %d", pubCount);

        areas[0] = Buffer;
        areaLengths[0] = strlen(Buffer) +1;

        rc=ism_engine_createMessage( &header
                                   , 1
                                   , areaTypes
                                   , areaLengths
                                   , areas
                                   , &hMessage);
        if (rc == ISMRC_ServerCapacity)
        {
            test_log(testLOGLEVEL_CHECK, "Server capacity reached. Iteration %d of %d.",
                     iterationCount+1, maxIterations);

            commitTransactions(hSession, tranTable, tranIndex);
            tranIndex = 0;
            tranTableSize = 10240;
            iterationCount++;

            rc = OK;
            continue;
        }

        TEST_ASSERT(rc == OK, ("Unexpected error %d from ism_engine_createMessage", rc));

        // Grow the transaction table if necessary
        if (tranIndex == tranTableSize)
        {
            int newSize = tranTableSize + 10240;
            ismEngine_TransactionHandle_t *newTable;
            newTable = (ismEngine_TransactionHandle_t *)realloc(tranTable, sizeof(ismEngine_TransactionHandle_t) * newSize);
            TEST_ASSERT(newTable != NULL, ("Failed to allocate tran table for %d entries\n", newSize));

            test_log(testLOGLEVEL_TESTPROGRESS, "Increased transaction table from %d to %d", tranTableSize, newSize);

            tranTableSize = newSize;
            tranTable = newTable;
        }

        rc = sync_ism_engine_createLocalTransaction(hSession,
                                                    &tranTable[tranIndex]);
        if (rc == ISMRC_StoreFull)
        {
            ism_engine_releaseMessage(hMessage);
            test_log(testLOGLEVEL_TESTPROGRESS, "sync_ism_engine_createLocalTransaction() returned %d\n", rc);
            rc = OK;
            goto mod_exit;
        }
        else
        {
            TEST_ASSERT(rc == OK, ("ERROR: ism_engine_createLocalTransaction() returned %d\n", rc));
        }

        rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                     ismDESTINATION_TOPIC,
                                                     TopicString,
                                                     tranTable[tranIndex],
                                                     hMessage);
        TEST_ASSERT(rc == OK, ("ERROR:  sync_ism_engine_putMessageOnDestination() returned %d\n", rc));

        tranIndex++;
    }

mod_exit:
    if (hConsumer != NULL)
    {
        rc=ism_engine_destroyConsumer( hConsumer
                                     , NULL
                                     , 0
                                     , NULL);
        TEST_ASSERT(rc == OK, ("ism_engine_destroyConsumer() returned %d\n", rc));

        rc=ism_engine_destroySubscription( hClient
                                         , subName
                                         , hClient
                                         , NULL
                                         , 0
                                         , NULL );
        TEST_ASSERT((rc == OK) || (rc == ISMRC_AsyncCompletion), ("ERROR: ism_engine_destroySubscription() returned %d\n", rc));
    }

    test_log(testLOGLEVEL_TESTDESC, "TransactionalPublishTest completed successfully");

    if (tranTable != NULL) free(tranTable);

    return rc;
}

int deleteSubscriptions(ismEngine_SessionHandle_t hSession,
                        int32_t startIndex,
                        int32_t count)
{
    char subName[64];
    int loop;
    int rc;

    test_log(testLOGLEVEL_TESTPROGRESS, "Deleting subscriptions %d through to %d", startIndex, startIndex+(count-1));

    for (loop=startIndex; loop < startIndex+count; loop++)
    {
        sprintf(subName, "DurableSubTest.%d", loop);

        rc=ism_engine_destroySubscription( hClient
                                         , subName
                                         , hClient
                                         , NULL
                                         , 0
                                         , NULL );
        if (rc != OK)
        {
            test_log(testLOGLEVEL_ERROR, "ism_engine_destroySubscription(%s) returned %d\n", subName, rc);
        }

        assert(rc == 0);
    }
    return 0;
}

uint32_t CreateDurableSubTest( ismEngine_SessionHandle_t hSession
                             , uint32_t maxIterations )
{
    char subName[64];
    int32_t rc = OK;
    int32_t iterationCount = 0;
    int32_t iterationStart = 0;
    int32_t startIndex = 0;
    int32_t subCount = 0;
    char TopicString[]="/Test/StoreEvents";

    test_log(testLOGLEVEL_TESTDESC, "Create large number of durable subscriptions.");

    uint32_t subsCreated = 0;
    uint32_t expectSubsCreated = 0;
    uint32_t *pSubsCreated = &subsCreated;

    double startTime = ism_common_readTSC() ;
    while (iterationCount < maxIterations)
    {
        sprintf(subName, "DurableSubTest.%d", subCount);

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                        ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };

        rc=ism_engine_createSubscription( hClient
                                        , subName
                                        , NULL
                                        , ismDESTINATION_TOPIC
                                        , TopicString
                                        , &subAttrs
                                        , NULL // Owning client same as requesting client
                                        , &pSubsCreated
                                        , sizeof(pSubsCreated)
                                        , test_asyncCallback );

        if (rc != ISMRC_AsyncCompletion)
        {
            test_asyncCallback(OK, NULL, &pSubsCreated);
        }
        expectSubsCreated++;

        // If we hit the owner limit, or server full capacity, delete some and carry on.
        if (rc == ISMRC_StoreOwnerLimit || rc == ISMRC_ServerCapacity)
        {
            // Wait for the dust to settle
            test_waitForAsyncCallbacks(pSubsCreated, expectSubsCreated);

            double elapsedTime = ism_common_readTSC()-startTime;

            test_log(testLOGLEVEL_CHECK, "Server capacity reached (%d). Created %d Subscriptions created in Iteration %d of %d. Total subscriptions %d, elapsed %.3f seconds.",
                     rc, subCount - iterationStart, iterationCount+1, maxIterations, subCount - startIndex, elapsedTime);

            deleteSubscriptions(hSession, startIndex, 10000);
            startIndex+=10000;
            startTime = ism_common_readTSC();
            iterationCount++;
            iterationStart = subCount;

            rc = OK;
            continue;
        }

        subCount++;

        if ((subCount % 10000) == 0)
        { 
            test_log(testLOGLEVEL_TESTPROGRESS, "Created %d subscriptions", subCount - startIndex);
        }
        test_log(testLOGLEVEL_VERBOSE, "Created subscription %d on Topic %s",
                 subCount, TopicString);
    }

    for (int32_t loop=startIndex; (rc == OK) && (loop < subCount); loop++)
    {
        sprintf(subName, "DurableSubTest.%d", loop);

        rc=ism_engine_destroySubscription( hClient
                                         , subName
                                         , hClient
                                         , NULL
                                         , 0
                                         , NULL );
        TEST_ASSERT(rc == OK, ("ism_engine_destroySubscription(%s) returned %d\n", subName, rc));
    }

    if (rc == OK)
    {
        test_log(testLOGLEVEL_TESTDESC, "CreateDurableSubTest completed successfully");
    }
    else
    {
        test_log(testLOGLEVEL_TESTDESC, "CreateDurableSubTest failed. rc=%d", rc);
    }
    return rc;
}

int deleteQueues(ismQHandle_t *qTable,
                 int32_t startIndex,
                 int32_t count)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int loop;
    int rc;

    test_log(testLOGLEVEL_TESTPROGRESS, "Deleting queues %d through to %d", startIndex, startIndex+(count-1));

    for (loop=startIndex; loop < startIndex+count; loop++)
    {
        rc=ieq_delete(pThreadData, &(qTable[loop]), false);
        if (rc != OK)
        {
            test_log(testLOGLEVEL_ERROR, "ieq_delete(%d) returned %d\n", loop, rc);
        }

        assert(rc == 0);
    }
    return 0;
}

uint32_t CreateQTest( ismEngine_SessionHandle_t hSession
                    , ismQueueType_t QType
                    , uint32_t maxIterations )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    char qName[64];
    int32_t rc = OK;
    int32_t iterationCount = 0;
    int32_t iterationStart = 0;
    int32_t startIndex = 0;
    int32_t qCount = 0;

    test_log(testLOGLEVEL_TESTDESC, "Create large number of %s queues.",
             (QType == intermediate)?"Intermediate":(QType==multiConsumer)?"MultiConsumer":"UNKNOWN");

    ismQHandle_t *qTable = (ismQHandle_t *)malloc(sizeof(ismQHandle_t) * 10240);
    assert(qTable != NULL);
    int qTableSize = 10240;

    double startTime = ism_common_readTSC();
    while (iterationCount < maxIterations)
    {
        // Grow the transaction table if necessary
        if (qCount == qTableSize)
        {
            int newSize = qTableSize + 10240;
            ismQHandle_t *newTable;
            newTable = (ismQHandle_t *)realloc(qTable, sizeof(ismQHandle_t) * newSize);
            if (newTable == NULL)
            {
                test_log(testLOGLEVEL_ERROR, "Failed to allocate q table for %d entries\n", newSize);
                goto mod_exit;
            }

            qTableSize = newSize;
            qTable = newTable;
        }

        sprintf(qName, "Queue.%d", qCount);

        rc=ieq_createQ( pThreadData
                      , QType
                      , qName
                      , ieqOptions_DEFAULT
                      , iepi_getDefaultPolicyInfo(true)
                      , ismSTORE_NULL_HANDLE
                      , ismSTORE_NULL_HANDLE
                      , iereNO_RESOURCE_SET
                      , &(qTable[qCount]) );

        // If we hit the owner limit, or server full capacity, delete some and carry on.
        if (rc == ISMRC_StoreOwnerLimit || rc == ISMRC_ServerCapacity)
        {
            double elapsedTime = ism_common_readTSC()-startTime;

            test_log(testLOGLEVEL_CHECK, "Server capacity reached (%d). Created %d Queues created in Iteration %d of %d. Total queues %d, elapsed %.3f seconds.",
                     rc, qCount - iterationStart, iterationCount+1, maxIterations, qCount - startIndex, elapsedTime);

            deleteQueues(qTable, startIndex, 10000);
            startTime = ism_common_readTSC();
            startIndex+=10000;
            iterationCount++;
            iterationStart = qCount;

            rc = OK;
            continue;
        }

        TEST_ASSERT(rc == OK, ("Unexpected error %d from ieq_createQ", rc));

        // For the 1st 100 iterations, try updating the queue's name.
        if (iterationCount < 100)
        {
            sprintf(qName, "UpdatedQueue.%d", qCount);
            rc=ieq_updateProperties(pThreadData,
                                    qTable[qCount],
                                    qName,
                                    ieqOptions_DEFAULT,
                                    ismSTORE_NULL_HANDLE,
                                    iereNO_RESOURCE_SET);

            TEST_ASSERT(rc == OK, ("Unexpected error %d from ieq_updateProperties", rc));
        }

        qCount++;

        test_log(testLOGLEVEL_VERBOSE, "Created queue %d", qCount);
    }

mod_exit:
    for (int32_t loop=startIndex; (rc == OK) && (loop < qCount); loop++)
    {
        rc=ieq_delete(pThreadData, &(qTable[loop]), false);
        TEST_ASSERT(rc == OK, ("Unexpected error %d from ieq_delete", rc));
    }

    if (rc == OK)
    {
        test_log(testLOGLEVEL_TESTDESC, "CreateQTest(%s) completed successfully",
                 (QType == intermediate)?"Intermediate":(QType==multiConsumer)?"MultiConsumer":"UNKNOWN");
    }
    else
    {
        test_log(testLOGLEVEL_TESTDESC, "CreateQTest(%s) failed. rc=%d",
                 (QType == intermediate)?"Intermediate":(QType==multiConsumer)?"MultiConsumer":"UNKNOWN", rc);
    }

    if (qTable != NULL) free(qTable);

    return rc;
}


/********************************************************************/
/* MAIN                                                             */
/********************************************************************/
int main(int argc, char *argv[])
{
    int trclvl = 4;
    int rc, rc2;
    ismEngine_SessionHandle_t hSession;
    bool RunTest1 = true;
    bool RunTest2 = true;
    bool RunTest3 = true;
    bool RunTest4 = true;

    /************************************************************************/
    /* Parse the command line arguments                                     */
    /************************************************************************/
    rc = ProcessArgs( argc
                    , argv
                    , &RunTest1
                    , &RunTest2
                    , &RunTest3
                    , &RunTest4 );

    if (rc != OK) goto mod_exit_noProcessTerm;

    test_setLogLevel(logLevel);

    rc = test_processInit(trclvl, NULL);

    if (rc != OK) goto mod_exit_noProcessTerm;

    // Force v1.1 granule sizes for this test to avoid making too many objects
    rc = test_setStoreMgmtGranuleSizes(256, 4096);

    if (rc != OK) goto mod_exit_noEngineTerm;

    rc =  test_engineInit(true,
                          true,
                          ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                          false, /*recovery should complete ok*/
                          ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                          50);  // 50mb Store

    if (rc != OK) goto mod_exit_noEngineTerm;

    if (usingDiskPersistence)
    {
        printf("This test takes too long (at least partly due to sync store commits during sub deletion) and is disabled on runs with real disk stores\n");
        goto mod_exit;
    }

    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 100000000;

    rc = ism_engine_createClientState(testClientId,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createClientState() returned %d\n", rc));

    rc = ism_engine_createSession( hClient
                                 , ismENGINE_CREATE_SESSION_OPTION_NONE
                                 , &hSession
                                 , NULL
                                 , 0
                                 , NULL);
    TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createSession(consumer) returned %d\n", rc));

    if (RunTest1)
    {
        rc = CreateDurableSubTest(hSession, 5);
    }

    if (RunTest2)
    {
        rc = CreateQTest(hSession, intermediate, 5);
    }

    if (RunTest3)
    {
        rc = CreateQTest(hSession, multiConsumer, 5);
    }

    if (RunTest4)
    {
        rc = TransactionalPublishTest(hSession, 5);
    }


    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL);
    TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_destroySession returned %d\n", rc));

    rc = ism_engine_destroyClientState( hClient
                                      , ismENGINE_DESTROY_CLIENT_OPTION_NONE
                                      , NULL
                                      , 0
                                      , NULL);
    TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_destroyClientState() returned %d\n", rc));

mod_exit:

    rc2 = test_engineTerm(true);
    if (rc2 != OK)
    {
        if (rc == OK) rc = rc2;
        goto mod_exit_noProcessTerm;
    }

mod_exit_noEngineTerm:

    test_processTerm(true);

mod_exit_noProcessTerm:

    return rc;
}

/****************************************************************************/
/* ProcessArgs                                                              */
/****************************************************************************/
int ProcessArgs( int argc
               , char **argv
               , bool *pRunTest1
               , bool *pRunTest2
               , bool *pRunTest3
               , bool *pRunTest4 )
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { "testcase", 1, NULL, 't' },
        { NULL, 1, NULL, 0 } };
    int long_index;

    while ((opt = getopt_long(argc, argv, ":v:t:", long_options, &long_index)) != -1)
    {
        switch (opt)
        {
            case 't':
               {
               int32_t testcaseNum = atoi(optarg);
               switch(testcaseNum)
               {
                   case 1: *pRunTest1 = true; *pRunTest2=false; *pRunTest3=false; *pRunTest4=false; break;
                   case 2: *pRunTest1 = false; *pRunTest2=true; *pRunTest3=false; *pRunTest4=false; break;
                   case 3: *pRunTest1 = false; *pRunTest2=false; *pRunTest3=true; *pRunTest4=false; break;
                   case 4: *pRunTest1 = false; *pRunTest2=false; *pRunTest3=false; *pRunTest4=true; break;
                   default:
                     printf("Unknown testcase number:\n");
                     usage=1;
                     break;
               }
               }
               break;
            case 'v':
               logLevel = atoi(optarg);
               if (logLevel > testLOGLEVEL_VERBOSE)
                   logLevel = testLOGLEVEL_VERBOSE;
               break;
            case '?':
                usage=1;
                break;
            default: 
                printf("%c\n", (char)opt);
                usage=1;
                break;
        }
    }

    if (usage)
    {
        fprintf(stderr, "Usage: %s [-v verbose] [-t testcase 1-4]\n", argv[0]);
        fprintf(stderr, "       -v - logLevel 0-5 [2]\n");
        fprintf(stderr, "       -t - testcase 1-4 [2]\n");
        fprintf(stderr, "\n");
    }

    return usage;
}
