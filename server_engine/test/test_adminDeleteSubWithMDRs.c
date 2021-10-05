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
/* Module Name: test_nonDurablePersistSharedSubs.c                   */
/*                                                                   */
/* Description: Main source file for CUnit test of nondurable MQTT   */
/* clients connecting to persistent shared subs                      */
/*********************************************************************/
#include <math.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include "engineInternal.h"
#include "engineHashSet.h"
#include "exportCrypto.h"
#include "exportClientState.h"
#include "exportSubscription.h"

#include "test_utils_initterm.h"
#include "test_utils_sync.h"
#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_security.h"
#include "test_utils_client.h"
#include "test_utils_options.h"
#include "test_utils_file.h"
#include "test_utils_pubsuback.h"
#include "test_utils_task.h"

#define MAX_ARGS 15

typedef enum tag_testFrenzyState_t
{
    INITIALIZING = 0,
    CREATING = 1,
    SETTING = 2,
    CREATED = 3,
    STOLEN = 4,
    DESTROYING = 5,
    DESTROYED = 6
} testFrenzyState_t;

typedef struct tag_testFrenzyThreadArgs_t
{
    const char *clientId;
    uint32_t createOptions;
    int32_t threadNumber;
    int32_t totalThreads;
    testFrenzyState_t state;
    ismEngine_ClientStateHandle_t hClient;
    uint64_t createRequestCount;
    uint64_t stealCount;
    uint64_t stealDuringCreating;
    uint64_t stealDuringCreated;
    uint64_t stealDuringDestroying;
    uint64_t stealDuringDestroyed;
    uint64_t stealDuringStolen;
    uint64_t createAsyncCount;
    uint64_t createSyncCount;
    uint64_t createSuccessCount;
    uint64_t createFailCount;
    uint64_t createSuccessDuringCleanCreating;
    uint64_t createSuccessDuringCreating;
    uint64_t createSuccessDuringStolen;
    bool running;
    bool stop;
} testFrenzyThreadArgs_t;

static void stealFrenzyDestroyComplete(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext)
{
    ismEngine_ClientStateHandle_t oldHandle;
    testFrenzyState_t oldState;
    testFrenzyThreadArgs_t *threadArgs = *(testFrenzyThreadArgs_t **)pContext;
    ismEngine_ClientStateHandle_t prevHandle = threadArgs->hClient;

    #if 0
    ieutTRACE_HISTORYBUF(ieut_getThreadData(), threadArgs);
    ieutTRACE_HISTORYBUF(ieut_getThreadData(), prevHandle);
    #endif

    // Set up for the next loop
    oldHandle = __sync_val_compare_and_swap(&threadArgs->hClient, prevHandle, NULL);
    TEST_ASSERT(oldHandle == prevHandle,
                ("Assertion failed in %s line %d [oldHandle=%p, prevHandle=%p]",
                 __func__, __LINE__, oldHandle, prevHandle));

    oldState = __sync_val_compare_and_swap(&threadArgs->state, DESTROYING, DESTROYED);

    TEST_ASSERT(oldState == DESTROYING,
                ("Assertion failed in %s line %d [oldState=%d]", __func__, __LINE__, oldState));
}

static void stealFrenzyStealCallback( int32_t reason,
                                      ismEngine_ClientStateHandle_t hClient,
                                      uint32_t options,
                                      void *pContext )
{
    testFrenzyState_t oldState;
    testFrenzyThreadArgs_t *threadArgs = (testFrenzyThreadArgs_t *)pContext;

    TEST_ASSERT(reason == ISMRC_ResumedClientState,
                ("Assertion failed in %s line %d [reason=%d]", __func__, __LINE__, reason));
    TEST_ASSERT(options == ismENGINE_STEAL_CALLBACK_OPTION_NONE,
                ("Assertion failed in %s line %d [options=0x%08x]", __func__, __LINE__, options));

    threadArgs->stealCount++;

    #if 0
    ieutTRACE_HISTORYBUF(ieut_getThreadData(), threadArgs);
    ieutTRACE_HISTORYBUF(ieut_getThreadData(), hClient);
    #endif

    // If we have finished being created, we do the destroy, otherwise they'll do it.
    oldState = __sync_val_compare_and_swap(&threadArgs->state, CREATING, STOLEN);

    if (oldState == CREATING)
    {
        threadArgs->stealDuringCreating++;
        goto mod_exit;
    }

    // Wait for any setting to finish
    while((oldState = __sync_val_compare_and_swap(&threadArgs->state, SETTING, SETTING)) == SETTING)
    {
        sched_yield();
    }

    oldState = __sync_val_compare_and_swap(&threadArgs->state, CREATED, DESTROYING);

    // Called for steal while still created
    if (oldState == CREATED)
    {
        int32_t rc;

        threadArgs->stealDuringCreated++;

        ismEngine_ClientStateHandle_t TA_hClient = (volatile ismEngine_ClientStateHandle_t)(threadArgs->hClient);

        TEST_ASSERT(TA_hClient == hClient,
                    ("Assertion failed in %s line %d [TA_hClient=%p, hClient=%p]",
                    __func__, __LINE__, TA_hClient, hClient));

        rc = ism_engine_destroyClientState(threadArgs->hClient,
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                           &threadArgs, sizeof(threadArgs),
                                           stealFrenzyDestroyComplete);
        TEST_ASSERT(rc == ISMRC_OK || rc == ISMRC_AsyncCompletion,
                    ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

        if (rc == ISMRC_OK) stealFrenzyDestroyComplete(rc, NULL, &threadArgs);

        goto mod_exit;
    }

    // Called for steal while destroying / destroyed
    if (oldState == DESTROYING)
    {
        threadArgs->stealDuringDestroying++;
        goto mod_exit;
    }

    if (oldState == DESTROYED)
    {
        threadArgs->stealDuringDestroyed++;
        goto mod_exit;
    }

    #if 0
    // Ignore double steal from earlier version of clientState
    if (oldState == STOLEN)
    {
        threadArgs->stealDuringStolen++;
        goto mod_exit;
    }
    #endif

    TEST_ASSERT(false, ("Assertion failed in %s line %d [oldState=%d]", __func__, __LINE__, oldState));

mod_exit:

    return;
}


static void stealFrenzyCreateClientStateComplete(int32_t retcode,
                                                 void *handle,
                                                 void *pContext)
{
    ismEngine_ClientStateHandle_t oldHandle;
    testFrenzyState_t oldState;
    testFrenzyThreadArgs_t *threadArgs = *(testFrenzyThreadArgs_t **)pContext;

    #if 0
    ieutTRACE_HISTORYBUF(ieut_getThreadData(), threadArgs);
    ieutTRACE_HISTORYBUF(ieut_getThreadData(), handle);
    #endif

    oldState = __sync_val_compare_and_swap(&threadArgs->state, CREATING, SETTING);

    threadArgs->createSuccessCount++;

    if (oldState == CREATING)
    {
        oldHandle = __sync_val_compare_and_swap(&threadArgs->hClient, NULL, handle);
        TEST_ASSERT(oldHandle == handle || oldHandle == NULL,
                    ("Assertion failed in %s line %d [oldHandle=%p]", __func__, __LINE__, oldHandle));

        TEST_ASSERT(retcode == ISMRC_OK || retcode == ISMRC_ResumedClientState,
                    ("Assertion failed in %s line %d [retcode=%d]", __func__, __LINE__, retcode));
        threadArgs->createSuccessDuringCreating++;

        TEST_ASSERT(handle != NULL,
                ("Assertion failed in %s line %d [handle NULL]", __func__, __LINE__));

        ismEngine_ClientState_t *clientState = (ismEngine_ClientState_t *)handle;

        TEST_ASSERT(clientState->hStoreCSR != ismSTORE_NULL_HANDLE,
                    ("Assertion failed in %s line %d [hStoreCSR=0x%0lx]", __func__, __LINE__, clientState->hStoreCSR));

        oldState = __sync_val_compare_and_swap(&threadArgs->state, SETTING, CREATED);
        TEST_ASSERT(oldState == SETTING,
                    ("Assertion failed in %s line %d [oldHandle=%p]", __func__, __LINE__, oldHandle));

        goto mod_exit;
    }

    oldState = __sync_val_compare_and_swap(&threadArgs->state, STOLEN, DESTROYING);

    if (oldState == STOLEN)
    {
        oldHandle = __sync_val_compare_and_swap(&threadArgs->hClient, NULL, handle);
        TEST_ASSERT(oldHandle == handle || oldHandle == NULL,
                    ("Assertion failed in %s line %d [oldHandle=%p]", __func__, __LINE__, oldHandle));

        threadArgs->createSuccessDuringStolen++;

        int32_t rc = ism_engine_destroyClientState(threadArgs->hClient,
                                                   ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                                   &threadArgs, sizeof(threadArgs),
                                                   stealFrenzyDestroyComplete);
        TEST_ASSERT(rc == ISMRC_OK || rc == ISMRC_AsyncCompletion,
                    ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

        if (rc == ISMRC_OK) stealFrenzyDestroyComplete(rc, NULL, &threadArgs);

        goto mod_exit;
    }

    //TEST_ASSERT(false, ("Assertion failed in %s line %d [oldState=%d]", __func__, __LINE__, oldState));

mod_exit:

    return;
}

void *stealFrenzyThread(void *args)
{
    testFrenzyState_t oldState;
    testFrenzyThreadArgs_t *threadArgs = (testFrenzyThreadArgs_t *)args;

    ism_engine_threadInit(0);

    threadArgs->running = true;

    oldState = __sync_val_compare_and_swap(&threadArgs->state, INITIALIZING, CREATING);
    TEST_ASSERT(oldState == INITIALIZING, ("Assertion failed in %s line %d [oldState=%d]",
                __func__, __LINE__, oldState));

    while((volatile bool)threadArgs->stop == false)
    {
        threadArgs->createRequestCount++;
        threadArgs->createOptions = ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                    ismENGINE_CREATE_CLIENT_OPTION_DURABLE;

        int32_t rc = ism_engine_createClientState(threadArgs->clientId,
                                                  PROTOCOL_ID_MQTT,
                                                  threadArgs->createOptions,
                                                  threadArgs,
                                                  stealFrenzyStealCallback,
                                                  NULL, // Should probably have different sec contexts (different UserIds)
                                                 &threadArgs->hClient,
                                                  &threadArgs, sizeof(threadArgs),
                                                  stealFrenzyCreateClientStateComplete);

        TEST_ASSERT(rc == ISMRC_OK || rc == ISMRC_ResumedClientState || (rc == ISMRC_ClientIDInUse) ||
                    rc == ISMRC_AsyncCompletion,
                    ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

        if (rc != ISMRC_ClientIDInUse)
        {
            if (rc != ISMRC_AsyncCompletion)
            {
                threadArgs->createSyncCount++;
                stealFrenzyCreateClientStateComplete(rc, threadArgs->hClient, &threadArgs);
            }
            else
            {
                threadArgs->createAsyncCount++;
            }

            // Wait until the client is no longer in 'CREATING' state
            while((oldState = __sync_val_compare_and_swap(&threadArgs->state, CREATING, CREATING)) == CREATING)
            {
                sched_yield();
            }

            // Wait until the client is no longer in 'SETTING' state
            while((oldState = __sync_val_compare_and_swap(&threadArgs->state, SETTING, SETTING)) == SETTING)
            {
                sched_yield();
            }

            // Wait for the completion of a steal, or our explicit destroy
            while((oldState = __sync_val_compare_and_swap(&threadArgs->state, DESTROYED, CREATING)) != DESTROYED)
            {
                sched_yield();
            }
        }
        else
        {
            threadArgs->createFailCount++;
        }
    }

    return NULL;
}


// 1) Create a shared sub with 2 durable clients (client0 & client1
// 2) Publish 18 messages to sub
// 3) Have  15 messages delivered to client0
//                    consume 2
//                    receive 2
//                    do nothing for 3
//                    after delivery stops nack-not-delivered 8
// 4) Have 5 messages delivered to client1 (should be 5 that have been given to client0 already)
//                       consume        1
//                       receive        1
//                       do nothing for 1
//                       after delivery stops nack-not-delivered 2
// Currently have 18-(7 sent and not nacked to client0 + 3 (client1)) = 8 available
// 5) Reconnect client0 to sub...  5 in flight already but they won't be redelivered as original session is not ended
//    Then have 6 more delivered to client1
//                       consume 1
//                       receive 1
//                       do nothing for 4
// 6) Have a new client (client2) come  and do nothing for last 2 available messages
//
//  After step 6:
//  4  consumed (3 by client0, 1 by client1)
//  4  received (3 by client0, 1 by client1)
//  10 inflight (7 by client0, 1 by client1, 2 by client2)
//  0 available

// 7) RESTART (checks we only get one MDR per message etc...)
// 8) Check a new client gets no messages
// 9) Check that client0 gets 10 messages delivered (of which 3 are received (2 in step 3+1 from step 5)
//               client1 gets 2 messages
//               client1 gets 2 messages
//

void test_capability_durable_BeforeRestart(void)
{
    #define NUM_DUR_CLIENTS 3
    ismEngine_SessionHandle_t hDurClientSession[NUM_DUR_CLIENTS];
    char *DurClientIds[NUM_DUR_CLIENTS] = {"DurClient0", "DurClient1", "DurClient2"};
    testFrenzyThreadArgs_t *DurThreadArgs[NUM_DUR_CLIENTS] = {0};
    testFrenzyThreadArgs_t **frenzyThreadArgs = NULL;
    int32_t rc;

    char *subName = "DurSub1";
    char *topicString = "DurTopic1";

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    //Setup the client that will own the shared namespace
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSharedOwningSession;
    char *SharedSubNameSpace = ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE;

    rc = test_createClientAndSessionWithProtocol(SharedSubNameSpace,
                                                 PROTOCOL_ID_SHARED,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hSharedOwningClient, &hSharedOwningSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningSession);

    // 1) Create a shared sub with 3 durable clients (client0 & client1
    for (uint32_t i = 0 ; i < NUM_DUR_CLIENTS; i++)
    {
        DurThreadArgs[i] = (testFrenzyThreadArgs_t *)calloc(1, sizeof(testFrenzyThreadArgs_t));

        rc = sync_ism_engine_createClientState(DurClientIds[i],
                                               PROTOCOL_ID_MQTT,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               DurThreadArgs[i],
                                               stealFrenzyStealCallback,
                                               NULL,
                                               &DurThreadArgs[i]->hClient);
        TEST_ASSERT_EQUAL(rc, OK);
        DurThreadArgs[i]->state = CREATED;

        rc = ism_engine_createSession(DurThreadArgs[i]->hClient,
                                      ismENGINE_CREATE_SESSION_OPTION_NONE,
                                      &hDurClientSession[i],
                                      NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_startMessageDelivery(hDurClientSession[i],
                                             ismENGINE_START_DELIVERY_OPTION_NONE,
                                             NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        test_makeSub(
                 subName
               , topicString
               ,    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE
                  | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                  | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                  | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
               , DurThreadArgs[i]->hClient
               , hSharedOwningClient);
    }

    // 2) Publish 9 qos=2 messages to sub
    test_pubMessages(topicString,
                     ismMESSAGE_PERSISTENCE_PERSISTENT,
                     ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                     9);

    // 3) Publish 9 qos=1 messages to sub
    test_pubMessages(topicString,
                     ismMESSAGE_PERSISTENCE_PERSISTENT,
                     ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                     9);

    // 4) Have 15 messages delivered to client0
    //                    do nothing for each message
    test_markMsgs( subName
                 , hDurClientSession[0]
                 , hSharedOwningClient
                 , 15 //Total messages we have delivered before disabling consumer
                 , 0 //Of the delivered messages, how many to Skip before ack
                 , 0 //Of the delivered messages, how many to consume
                 , 0 //Of the delivered messages, how many to mark received
                 , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
                 , 0 //Of the delivered messages, how many to return to caller un(n)acked
                 , 0 //Of the delivered messages, how many to nack not received
                 , 0 //Of the delivered messages, how many to nack not sent
                 , NULL); //Handles of messages returned to us un(n)acked

    int32_t totalThreads = 3;
    frenzyThreadArgs = malloc(totalThreads * sizeof(testFrenzyThreadArgs_t *));
    for(int32_t i=0; i<totalThreads; i++)
    {
        frenzyThreadArgs[i] = (testFrenzyThreadArgs_t *)calloc(1, sizeof(testFrenzyThreadArgs_t));
        TEST_ASSERT(frenzyThreadArgs[i] != NULL, ("Assertion failed in %s line %d [failed malloc]", __func__, __LINE__));

        frenzyThreadArgs[i]->clientId = DurClientIds[0];
        frenzyThreadArgs[i]->threadNumber = i+1;
        frenzyThreadArgs[i]->totalThreads = totalThreads;
        frenzyThreadArgs[i]->state = INITIALIZING;
        frenzyThreadArgs[i]->running = false;

        pthread_t thisThread;
        int osrc = test_task_startThread( &thisThread ,stealFrenzyThread, frenzyThreadArgs[i] ,"stealFrenzyThread");
        TEST_ASSERT(osrc == OK, ("Assertion failed in %s line %d", __func__, __LINE__));
    }

    for(uint32_t i=0; i<totalThreads; i++)
    {
        while(frenzyThreadArgs[i]->createSuccessCount < 10)
        {
            usleep(100);
        }
    }

    // Emulate an administrative deletion with clients connected
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    rc = iett_destroySubscriptionForClientId(pThreadData,
                                             SharedSubNameSpace,
                                             NULL, // Emulating an admin request
                                             subName,
                                             NULL, // Emulating an admin request
                                             iettSUB_DESTROY_OPTION_NONE,
                                             NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}



void test_capability_durable_SubUnsubWithInflight(void)
{
    #define NUM_DUR_CLIENTS 3
    ismEngine_SessionHandle_t hDurClientSession[NUM_DUR_CLIENTS];
    char *DurClientIds[NUM_DUR_CLIENTS] = {"SubUnsubClient0", "SubUnsubClient1", "SubUnsubClient2"};
    testFrenzyThreadArgs_t *DurThreadArgs[NUM_DUR_CLIENTS] = {0};
    int32_t rc;

    char *subName = "DurSub1";
    char *topicString = "DurTopic1";

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    //Setup the client that will own the shared namespace
    ismEngine_ClientStateHandle_t hSharedOwningClient;
    ismEngine_SessionHandle_t hSharedOwningSession;
    char *SharedSubNameSpace = "__SharedSubUnsub";

    rc = test_createClientAndSession(SharedSubNameSpace,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hSharedOwningClient, &hSharedOwningSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningClient);
    TEST_ASSERT_PTR_NOT_NULL(hSharedOwningSession);

    // 1) Create a shared sub with 3 durable clients (client0 & client1
    for (uint32_t i = 0 ; i < NUM_DUR_CLIENTS; i++)
    {
        DurThreadArgs[i] = (testFrenzyThreadArgs_t *)calloc(1, sizeof(testFrenzyThreadArgs_t));

        rc = sync_ism_engine_createClientState(DurClientIds[i],
                                               PROTOCOL_ID_MQTT,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               DurThreadArgs[i],
                                               stealFrenzyStealCallback,
                                               NULL,
                                               &DurThreadArgs[i]->hClient);
        TEST_ASSERT_EQUAL(rc, OK);
        DurThreadArgs[i]->state = CREATED;

        rc = ism_engine_createSession(DurThreadArgs[i]->hClient,
                                      ismENGINE_CREATE_SESSION_OPTION_NONE,
                                      &hDurClientSession[i],
                                      NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_startMessageDelivery(hDurClientSession[i],
                                             ismENGINE_START_DELIVERY_OPTION_NONE,
                                             NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        test_makeSub(
                 subName
               , topicString
               ,    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE
                  | ismENGINE_SUBSCRIPTION_OPTION_SHARED
                  | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
                  | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
               , DurThreadArgs[i]->hClient
               , hSharedOwningClient);
    }

    // 2) Publish 9 qos=2 messages to sub
    test_pubMessages(topicString,
                     ismMESSAGE_PERSISTENCE_PERSISTENT,
                     ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                     9);

    // 3) Publish 9 qos=1 messages to sub
    test_pubMessages(topicString,
                     ismMESSAGE_PERSISTENCE_PERSISTENT,
                     ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                     9);

    // 4) Have 15 messages delivered to client0
    //                    do nothing for each message
    test_markMsgs( subName
                 , hDurClientSession[0]
                 , hSharedOwningClient
                 , 15 //Total messages we have delivered before disabling consumer
                 , 0 //Of the delivered messages, how many to Skip before ack
                 , 0 //Of the delivered messages, how many to consume
                 , 0 //Of the delivered messages, how many to mark received
                 , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
                 , 0 //Of the delivered messages, how many to return to caller un(n)acked
                 , 0 //Of the delivered messages, how many to nack not received
                 , 0 //Of the delivered messages, how many to nack not sent
                 , NULL); //Handles of messages returned to us un(n)acked

    rc= ism_engine_destroySubscription( DurThreadArgs[0]->hClient
				                      , subName
									  , hSharedOwningClient
									  , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_makeSub(
            subName
          , topicString
          ,    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE
             | ismENGINE_SUBSCRIPTION_OPTION_SHARED
             | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT
             | ismENGINE_SUBSCRIPTION_OPTION_DURABLE
          , DurThreadArgs[0]->hClient
          , hSharedOwningClient);


    test_markMsgs( subName
                  , hDurClientSession[0]
                  , hSharedOwningClient
                  , 2 //Total messages we have delivered before disabling consumer
                  , 0 //Of the delivered messages, how many to Skip before ack
                  , 0 //Of the delivered messages, how many to consume
                  , 0 //Of the delivered messages, how many to mark received
                  , 0 //Of the delivered messages, how many to Skip before handles returned to us/nacked
                  , 0 //Of the delivered messages, how many to return to caller un(n)acked
                  , 0 //Of the delivered messages, how many to nack not received
                  , 0 //Of the delivered messages, how many to nack not sent
                  , NULL); //Handles of messages returned to us un(n)acked

    rc= ism_engine_destroySubscription( DurThreadArgs[0]->hClient
                                      , subName
									  , hSharedOwningClient
									  , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroyClientState( DurThreadArgs[0]->hClient
    		                               , ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_durable_PostRestart(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    char *SharedSubNameSpace = ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE;
    char *subName = "DurSub1";

    ismEngine_Subscription_t *subscription=NULL;

    int rc = iett_findClientSubscription(pThreadData,
                                         SharedSubNameSpace,
                                         iett_generateClientIdHash(SharedSubNameSpace),
                                         subName,
                                         iett_generateSubNameHash(subName),
                                         &subscription);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.totalNonFatalFFDCs , 0);

    test_log(testLOGLEVEL_TESTNAME, "Finished %s...\n", __func__);
}

void test_capability_durable_PostRestart2(void)
{
	test_capability_durable_PostRestart();
}


CU_TestInfo ISM_deleteAdminSubMDRs_CUnit_test_capability_PreRestart[] =
{
    { "durableBeforeRestart",               test_capability_durable_BeforeRestart },
    { "SubUnsubInflight",                   test_capability_durable_SubUnsubWithInflight },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_deleteAdminSubMDRs_CUnit_test_capability_PostRestart[] =
{
    { "durablePostRestart",               test_capability_durable_PostRestart },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_deleteAdminSubMDRs_CUnit_test_capability_PostRestart2[] =
{
    { "durablePostRestart2",               test_capability_durable_PostRestart2 },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initPhase0(void)
{
    return test_engineInit(true, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termPhase0(void)
{
   //Do nothing we want the engine running at restart
   return 0;
}

//For phases after phase 0
int initPhaseN(void)
{
    int rc= test_engineInit(false, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
    return rc;
}
//For phases after phase 0
int termPhaseN(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_deleteAdminSubMDRs_CUnit_phase0suites[] =
{
    IMA_TEST_SUITE("PreRestart", initPhase0, termPhase0, ISM_deleteAdminSubMDRs_CUnit_test_capability_PreRestart),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_deleteAdminSubMDRs_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("AfterRestart1", initPhaseN, termPhaseN, ISM_deleteAdminSubMDRs_CUnit_test_capability_PostRestart),
    CU_SUITE_INFO_NULL,
};
CU_SuiteInfo ISM_deleteAdminSubMDRs_CUnit_phase2suites[] =
{
    IMA_TEST_SUITE("AfterRestart1", initPhaseN, termPhaseN, ISM_deleteAdminSubMDRs_CUnit_test_capability_PostRestart2),
    CU_SUITE_INFO_NULL,
};
CU_SuiteInfo *PhaseSuites[] = { ISM_deleteAdminSubMDRs_CUnit_phase0suites
                              , ISM_deleteAdminSubMDRs_CUnit_phase1suites };


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

void setupArgsForNextPhase(int argc, char *argv[], char *nextPhaseText, char *adminDir, char *NextPhaseArgv[])
{
    bool phasefound=false, adminDirfound=false;
    uint32_t i;
    TEST_ASSERT(argc < MAX_ARGS-5, ("argc was %d", argc));

    for (i=0; i < argc; i++)
    {
        NextPhaseArgv[i]=argv[i];
        if (strcmp(NextPhaseArgv[i], "-p") == 0)
        {
            NextPhaseArgv[i+1]=nextPhaseText;
            i++;
            phasefound=true;
        }
        if (strcmp(NextPhaseArgv[i], "-a") == 0)
        {
            NextPhaseArgv[i+1]=adminDir;
            i++;
            adminDirfound=true;
        }
    }

    if (!phasefound)
    {
        NextPhaseArgv[i++]="-p";
        NextPhaseArgv[i++]=nextPhaseText;
    }

    if (!adminDirfound)
    {
        NextPhaseArgv[i++]="-a";
        NextPhaseArgv[i++]=adminDir;
    }

    NextPhaseArgv[i]=NULL;

}

int main(int argc, char *argv[])
{
    int32_t trclvl = 0;
    uint32_t testLogLevel = testLOGLEVEL_TESTDESC;
    int retval = 0;
    char *adminDir=NULL;
    uint32_t phase=0;
    uint32_t finalPhase=20;

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

    uint32_t realPhase = phase;

    // For this test, we just alternate between phases 0 & 1.
    if (sizeof(PhaseSuites)/sizeof(PhaseSuites[0]) != 2)
    {
       retval = 99;
       goto mod_exit;
    }
    phase = phase%2;

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, adminDir);

    char localAdminDir[1024];
    if (adminDir == NULL && test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir)) == true)
    {
        adminDir = localAdminDir;
    }

    if (retval == 0)
    {
        CU_register_suites(PhaseSuites[phase]);

        CU_basic_run_tests();

        CU_RunSummary * CU_pRunSummary_Final;
        CU_pRunSummary_Final = CU_get_run_summary();
        if ((CU_pRunSummary_Final->nTestsFailed > 0) ||
            (CU_pRunSummary_Final->nAssertsFailed > 0))
        {
            retval = 1;
        }

        if (retval || realPhase == finalPhase)
        {
            printf("Random Seed =     %"PRId64"\n", seedVal);
            printf("\n[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
                   CU_pRunSummary_Final->nTestsRun,
                   CU_pRunSummary_Final->nTestsFailed,
                   CU_pRunSummary_Final->nAssertsFailed);
        }
    }

    CU_cleanup_registry();

    if (retval != 0) goto mod_exit;

    if (realPhase < finalPhase)
    {
        uint32_t nextPhase = realPhase+1;
        test_log(2, "Restarting for Phase %u...", nextPhase);
        char *nextPhaseArgv[MAX_ARGS];
        char nextPhaseText[16];
        sprintf(nextPhaseText, "%d", nextPhase);
        setupArgsForNextPhase(argc, argv, nextPhaseText, adminDir, nextPhaseArgv);
        retval = test_execv(nextPhaseArgv[0], nextPhaseArgv);
        TEST_ASSERT(false, ("'test_execv' failed. rc = %d", retval));
    }
    else
    {
        test_log(2, "Success. Test complete!\n");
        test_processTerm(false);
        test_removeAdminDir(adminDir);
    }

mod_exit:

    return retval;
}
