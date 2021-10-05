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

//#define USE_THREAD_FOR_DESTROY_ON_STEAL
//#define USE_THREAD_FOR_DESTROY_ON_CREATE

/*********************************************************************/
/*                                                                   */
/* Module Name: test_clientStateRestart.c                            */
/*                                                                   */
/* Description: Test program which validates the recovery of clients */
/*              including their rehydration during restart.          */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <execinfo.h>

#include "engineInternal.h"
#include "queueCommon.h"
#include "transaction.h"
#include "queueNamespace.h"

#include "test_utils_initterm.h"
#include "test_utils_log.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_message.h"
#include "test_utils_task.h"

volatile bool firstClientCreated = false;
volatile bool endDelays = false;

bool restarting = false;
pthread_spinlock_t threadCountLock;
uint32_t threadCount = 0;
uint32_t threadMax = 0;

#define STEALFRENZY_THREAD_COUNT         20
#define STEALFRENZY_THREADS_PER_CLIENTID 5
#define STEALFRENZY_MAX_SECONDS          5

char *clientId = "attemptedDupClient";

bool durableClients = false;

uint32_t uniqueSubId = 0;

void createAndRemoveDurableSub(ismEngine_ClientStateHandle_t hClient)
{
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession = NULL;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    rc = ism_engine_createSession(hClient, ismENGINE_CREATE_SESSION_OPTION_NONE, &hSession, NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    uint32_t ourSubId = __sync_add_and_fetch(&uniqueSubId, 1);
    char subname[128];
    sprintf(subname,"sub%u", ourSubId);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    rc = sync_ism_engine_createSubscription(hClient
                                          , subname
                                          , NULL
                                          , ismDESTINATION_TOPIC
                                          , "/wedonotcareaboutthetopic"
                                          , &subAttrs
                                          , hClient);
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    rc = ism_engine_destroySubscription(hClient, subname, hClient, NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    rc = ism_engine_destroySession(hSession, NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));
}

void *createFirstClient(void *args)
{
    ismEngine_ClientStateHandle_t hClient;

    ism_engine_threadInit(0);

    int32_t rc = ism_engine_createClientState(clientId,
                                              testDEFAULT_PROTOCOL_ID,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE
                                                | (durableClients ?ismENGINE_CREATE_CLIENT_OPTION_DURABLE: 0),
                                              NULL, NULL, NULL,
                                              &hClient,
                                              NULL,
                                              0,
                                              NULL);
    if (rc != OK) TEST_ASSERT(rc == ISMRC_ResumedClientState, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    if (!durableClients)
    {
        //If the client is non durable we want some durable objects to ensure we put a client record in the store
        createAndRemoveDurableSub(hClient);
    }

    firstClientCreated = true;

    //Shall we throw away durable state?
    bool discardDurable = false;

    if (durableClients)
    {
        discardDurable = true;
    }

    rc = sync_ism_engine_destroyClientState( hClient
                                           , discardDurable ? ismENGINE_DESTROY_CLIENT_OPTION_DISCARD
                                                            : ismENGINE_DESTROY_CLIENT_OPTION_NONE );
    TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    return NULL;
}

void test_tryCreateDuplicateClientStates(pthread_t *phThread)
{
    ism_engine_threadInit(0);

    //Start the thread that will create the first client
    int osrc = test_task_startThread( phThread ,createFirstClient, NULL ,"createFirstClient");
    TEST_ASSERT(osrc == OK, ("Assertion failed in %s line %d [osrc=%d]", __func__, __LINE__, osrc));

    //Wait for the first client to be created
    while(!firstClientCreated){}


    //Create the second client... keep trying whilst 1st one blocks creation
    int32_t rc;
    ismEngine_ClientStateHandle_t hClient;

    do
    {
        rc = sync_ism_engine_createClientState(clientId,
                                               testDEFAULT_PROTOCOL_ID,
                                               ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE
                                               |(durableClients ?ismENGINE_CREATE_CLIENT_OPTION_DURABLE: 0),
                                               NULL, NULL, NULL,
                                               &hClient);
    }
    while( rc == ISMRC_ClientIDInUse);

    if (rc != OK) TEST_ASSERT(rc == ISMRC_ResumedClientState, ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    if (!durableClients)
    {
        //If the client is non durable we want some durable objects to ensure we put a client record in the store
        createAndRemoveDurableSub(hClient);
    }
}

ismEngine_ClientStateHandle_t hStealFrenzyMonitorClient = NULL;
ismEngine_SessionHandle_t hStealFrenzyMonitorSession = NULL;
ismEngine_ConsumerHandle_t hStealFrenzyMonitorConsumer = NULL;

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

void *bt[1000];

static void checkReasonableStackDepth(void)
{
    #if CHECK_REASONABLE_STACK_DEPTH
    int depth = backtrace((void **)&bt, sizeof(bt)/sizeof(bt[0]));
    TEST_ASSERT(depth < 500,("Assertion failed in %s line %d [depth=%d] ", depth));
    #endif
}

static void stealFrenzyDestroyComplete(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext)
{
    ismEngine_ClientStateHandle_t oldHandle;
    testFrenzyState_t oldState;
    testFrenzyThreadArgs_t *threadArgs = *(testFrenzyThreadArgs_t **)pContext;
    ismEngine_ClientStateHandle_t prevHandle = threadArgs->hClient;

    checkReasonableStackDepth();

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

void *stealFrenzyDestroyThread(void *args)
{
    testFrenzyThreadArgs_t *threadArgs = (testFrenzyThreadArgs_t *)args;

    checkReasonableStackDepth();

    pthread_spin_lock(&threadCountLock);
    threadCount += 1;
    if (threadCount > threadMax) threadMax = threadCount;
    pthread_spin_unlock(&threadCountLock);

    ism_engine_threadInit(0);

    int32_t rc = ism_engine_destroyClientState(threadArgs->hClient,
                                               ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                               &threadArgs, sizeof(threadArgs),
                                               stealFrenzyDestroyComplete);
    TEST_ASSERT(rc == ISMRC_OK || rc == ISMRC_AsyncCompletion,
                ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

    if (rc == ISMRC_OK) stealFrenzyDestroyComplete(rc, NULL, &threadArgs);

    ism_engine_threadTerm(1);

    pthread_spin_lock(&threadCountLock);
    threadCount -= 1;
    pthread_spin_unlock(&threadCountLock);

    return NULL;
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

    checkReasonableStackDepth();

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

        if (hStealFrenzyMonitorSession != NULL)
        {
            void   *payload=NULL;
            ismEngine_MessageHandle_t hMessage;
            char topicString[100];

            sprintf(topicString, "monitor/msg/%s", hClient->pClientId);
            rc = test_createMessage(100,
                                    ismMESSAGE_PERSISTENCE_PERSISTENT,
                                    ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                    ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                    0,
                                    ismDESTINATION_TOPIC, topicString,
                                    &hMessage, &payload);
            TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d", __func__, __LINE__));

            rc = ism_engine_putMessageOnDestination(hStealFrenzyMonitorSession,
                                                    ismDESTINATION_TOPIC,
                                                    topicString,
                                                    NULL,
                                                    hMessage,
                                                    NULL, 0, NULL);
            TEST_ASSERT(rc < ISMRC_Error, ("Assertion failed in %s line %d", __func__, __LINE__));
        }

        #if defined(USE_THREAD_FOR_DESTROY_ON_STEAL)
        pthread_t destroyThread;

        // Start a thread to request the destroy
        int osrc = test_task_startThread( &destroyThread ,stealFrenzyDestroyThread, threadArgs ,"stealFrenzyDestroyThread");
        if (osrc == EAGAIN)
        {
            bool didCAS = __sync_bool_compare_and_swap(&restarting, true, true);
            TEST_ASSERT(didCAS == true, ("Got EAGAIN when not restarting"));
        }
        else
        {
            TEST_ASSERT(osrc == OK, ("Assertion failed in %s line %d", __func__, __LINE__));
        }
        #else
        rc = ism_engine_destroyClientState(threadArgs->hClient,
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                           &threadArgs, sizeof(threadArgs),
                                           stealFrenzyDestroyComplete);
        TEST_ASSERT(rc == ISMRC_OK || rc == ISMRC_AsyncCompletion,
                    ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

        if (rc == ISMRC_OK) stealFrenzyDestroyComplete(rc, NULL, &threadArgs);
        #endif

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

    checkReasonableStackDepth();

    #if 0
    ieutTRACE_HISTORYBUF(ieut_getThreadData(), threadArgs);
    ieutTRACE_HISTORYBUF(ieut_getThreadData(), handle);
    #endif

    bool cleanStart = (threadArgs->createOptions & ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART) != 0;
    bool durable = (threadArgs->createOptions & ismENGINE_CREATE_CLIENT_OPTION_DURABLE) != 0;

    if (cleanStart || !durable)
    {
        TEST_ASSERT(retcode != ISMRC_ResumedClientState,
                    ("Assertion failed in %s line %d [retcode=%d]", __func__, __LINE__, retcode));
    }

    oldState = __sync_val_compare_and_swap(&threadArgs->state, CREATING, SETTING);

    threadArgs->createSuccessCount++;

    if (oldState == CREATING)
    {
        oldHandle = __sync_val_compare_and_swap(&threadArgs->hClient, NULL, handle);
        TEST_ASSERT(oldHandle == handle || oldHandle == NULL,
                    ("Assertion failed in %s line %d [oldHandle=%p]", __func__, __LINE__, oldHandle));

        if (cleanStart)
        {
            TEST_ASSERT(retcode == ISMRC_OK,
                        ("Assertion failed in %s line %d [retcode=%d]", __func__, __LINE__, retcode));
            threadArgs->createSuccessDuringCleanCreating++;
        }
        else
        {
            TEST_ASSERT(retcode == ISMRC_OK || retcode == ISMRC_ResumedClientState,
                    ("Assertion failed in %s line %d [retcode=%d]", __func__, __LINE__, retcode));
            threadArgs->createSuccessDuringCreating++;
        }

        TEST_ASSERT(handle != NULL,
                ("Assertion failed in %s line %d [handle NULL]", __func__, __LINE__));

        ismEngine_ClientState_t *clientState = (ismEngine_ClientState_t *)handle;

        if (durable)
        {
            TEST_ASSERT(clientState->hStoreCSR != ismSTORE_NULL_HANDLE,
                        ("Assertion failed in %s line %d [hStoreCSR=0x%0lx]", __func__, __LINE__, clientState->hStoreCSR));
        }
        else
        {
            // We know that this should be null for *this* type of non-durable (it could be non-null
            // in other cases, where a persistent will message had been set, or if durable subscriptions
            // had been created under the non-durable client state, for instance).
            TEST_ASSERT(clientState->hStoreCSR == ismSTORE_NULL_HANDLE,
                        ("Assertion failed in %s line %d [hStoreCSR=0x%0lx]", __func__, __LINE__, clientState->hStoreCSR));
        }

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

        #if defined(USE_THREAD_FOR_DESTROY_ON_CREATE)
        pthread_t destroyThread;

        // Start a thread to request the destroy
        int osrc = test_task_startThread( &destroyThread ,stealFrenzyDestroyThread, threadArgs ,"stealFrenzyDestroyThread");
        if (osrc == EAGAIN)
        {
            bool didCAS = __sync_bool_compare_and_swap(&restarting, true, true);
            TEST_ASSERT(didCAS == true, ("Got EAGAIN when not restarting"));
        }
        else
        {
            TEST_ASSERT(osrc == OK, ("Assertion failed in %s line %d", __func__, __LINE__));
        }
        #else
        int32_t rc = ism_engine_destroyClientState(threadArgs->hClient,
                                                   ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                                   &threadArgs, sizeof(threadArgs),
                                                   stealFrenzyDestroyComplete);
        TEST_ASSERT(rc == ISMRC_OK || rc == ISMRC_AsyncCompletion,
                    ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

        if (rc == ISMRC_OK) stealFrenzyDestroyComplete(rc, NULL, &threadArgs);
        #endif
        goto mod_exit;
    }

    TEST_ASSERT(false, ("Assertion failed in %s line %d [oldState=%d]", __func__, __LINE__, oldState));

mod_exit:

    return;
}

void *stealFrenzyThread(void *args)
{
    testFrenzyState_t oldState;
    testFrenzyThreadArgs_t *threadArgs = (testFrenzyThreadArgs_t *)args;

    pthread_spin_lock(&threadCountLock);
    threadCount += 1;
    if (threadCount > threadMax) threadMax = threadCount;
    pthread_spin_unlock(&threadCountLock);

    ism_engine_threadInit(0);

    threadArgs->running = true;

    oldState = __sync_val_compare_and_swap(&threadArgs->state, INITIALIZING, CREATING);
    TEST_ASSERT(oldState == INITIALIZING, ("Assertion failed in %s line %d [oldState=%d]",
                __func__, __LINE__, oldState));

    while((volatile bool)threadArgs->stop == false)
    {
        #if 0
        ismEngine_ClientStateHandle_t oldHandle;

        oldState = __sync_val_compare_and_swap(&threadArgs->state, CREATING, CREATING);
        TEST_ASSERT(oldState == CREATING, ("*SANITY* Assertion failed in %s line %d [oldState=%d]",
                    __func__, __LINE__, oldState));
        oldHandle = __sync_val_compare_and_swap(&threadArgs->hClient, NULL, NULL);
        TEST_ASSERT(oldHandle == NULL, ("*SANITY*  Assertion failed in %s line %d [oldHandle=%p]",
                    __func__, __LINE__, oldHandle));
        #endif
        uint32_t protocolId = (rand()%100>80) ? PROTOCOL_ID_MQTT : PROTOCOL_ID_HTTP;
        bool durable        = (rand()%100>75) ? true : false;
        bool cleanStart     = (rand()%100<20) ? true : false;

        threadArgs->createRequestCount++;
        threadArgs->createOptions = ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL
                                  | (durable ? ismENGINE_CREATE_CLIENT_OPTION_DURABLE :
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE)
                                  | (cleanStart ? ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART :
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE);

        int32_t rc = ism_engine_createClientState(threadArgs->clientId,
                                                  protocolId,
                                                  threadArgs->createOptions,
                                                  threadArgs,
                                                  stealFrenzyStealCallback,
                                                  NULL, // Should probably have different sec contexts (different UserIds)
                                                 &threadArgs->hClient,
                                                  &threadArgs, sizeof(threadArgs),
                                                  stealFrenzyCreateClientStateComplete);

        TEST_ASSERT(rc == ISMRC_OK || rc == ISMRC_ResumedClientState || ((rc == ISMRC_ClientIDInUse) && durable) ||
                    rc == ISMRC_AsyncCompletion,
                    ("Assertion failed in %s line %d [rc=%d, durable=%d]", __func__, __LINE__, rc, (int32_t)durable));

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

            // 70% of the time, issue an explicit destroy (if steal hasn't already come in!)
            if  (rand()%100 > 30)
            {
                oldState = __sync_val_compare_and_swap(&threadArgs->state, CREATED, DESTROYING);

                if (oldState == CREATED)
                {
                    //Shall we throw away durable state?
                    bool discardDurable = durable && (rand()%100 > 95);

                    rc = ism_engine_destroyClientState(threadArgs->hClient,
                                                       discardDurable ? ismENGINE_DESTROY_CLIENT_OPTION_DISCARD
                                                                      : ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                                       &threadArgs, sizeof(threadArgs),
                                                       stealFrenzyDestroyComplete);
                    TEST_ASSERT(rc == ISMRC_OK || rc == ISMRC_AsyncCompletion,
                                ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

                    if (rc == ISMRC_OK) stealFrenzyDestroyComplete(rc, NULL, &threadArgs);
                }
            }

            // Wait for the completion of a steal, or our explicit destroy
            while((oldState = __sync_val_compare_and_swap(&threadArgs->state, DESTROYED, CREATING)) != DESTROYED)
            {
                // If we're stopping - clear up this client so that we can shut down cleanly
                if ((volatile bool)threadArgs->stop)
                {
                    oldState = __sync_val_compare_and_swap(&threadArgs->state, CREATED, DESTROYING);
                    if (oldState == CREATED)
                    {
                        rc = ism_engine_destroyClientState(threadArgs->hClient,
                                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                                           &threadArgs, sizeof(threadArgs),
                                                           stealFrenzyDestroyComplete);
                        TEST_ASSERT(rc == ISMRC_OK || rc == ISMRC_AsyncCompletion,
                                    ("Assertion failed in %s line %d [rc=%d]", __func__, __LINE__, rc));

                        if (rc == ISMRC_OK) stealFrenzyDestroyComplete(rc, NULL, &threadArgs);
                    }
                }
                sched_yield();
            }
        }
        else
        {
            threadArgs->createFailCount++;
        }
    }

    pthread_spin_lock(&threadCountLock);
    threadCount -= 1;
    pthread_spin_unlock(&threadCountLock);

    return NULL;
}

void test_stealFrenzy(int32_t secondsToRun, bool quiesceThreads, bool monitoringMessages)
{
    int32_t rc;
    ism_engine_threadInit(0);

    pthread_t frenzyThread[STEALFRENZY_THREAD_COUNT] = {0};
    testFrenzyThreadArgs_t *frenzyThreadArgs[STEALFRENZY_THREAD_COUNT] = {NULL};

    int32_t totalThreads = (int32_t)(sizeof(frenzyThread)/sizeof(frenzyThread[0]));
    int32_t totalClientIds = totalThreads/STEALFRENZY_THREADS_PER_CLIENTID;

    char *clientIds[totalClientIds];

    if (monitoringMessages)
    {
        rc = sync_ism_engine_createClientState("StealFrenzyMonitorClient",
                                               testDEFAULT_PROTOCOL_ID,
                                               ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                               NULL, NULL,
                                               NULL,
                                               &hStealFrenzyMonitorClient);
        TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d", __func__, __LINE__));
        TEST_ASSERT(hStealFrenzyMonitorClient != NULL, ("Assertion failed in %s line %d", __func__, __LINE__));

        rc = ism_engine_createSession(hStealFrenzyMonitorClient,
                                      ismENGINE_CREATE_SESSION_OPTION_NONE,
                                      &hStealFrenzyMonitorSession,
                                      NULL, 0, NULL);
        TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d", __func__, __LINE__));
        TEST_ASSERT(hStealFrenzyMonitorSession != NULL, ("Assertion failed in %s line %d", __func__, __LINE__));

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };

        rc = ism_engine_createConsumer(hStealFrenzyMonitorSession,
                                       ismDESTINATION_TOPIC, "monitor/#",
                                       &subAttrs,
                                       NULL,
                                       NULL, 0, NULL,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &hStealFrenzyMonitorConsumer,
                                       NULL, 0, NULL);
        TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d", __func__, __LINE__));
        TEST_ASSERT(hStealFrenzyMonitorConsumer != NULL, ("Assertion failed in %s line %d", __func__, __LINE__));
    }

    for(int32_t i=0; i<totalThreads; i++)
    {
        int32_t thisClientId = i/STEALFRENZY_THREADS_PER_CLIENTID;

        if (i%STEALFRENZY_THREADS_PER_CLIENTID == 0)
        {
            clientIds[thisClientId] = malloc(50);
            TEST_ASSERT(clientIds[thisClientId] != NULL, ("Assertion failed in %s line %d [failed malloc]", __func__, __LINE__));
            sprintf(clientIds[thisClientId], "FrenzyClient%d", thisClientId+1);
        }
        frenzyThreadArgs[i] = (testFrenzyThreadArgs_t *)calloc(1, sizeof(testFrenzyThreadArgs_t));
        TEST_ASSERT(frenzyThreadArgs[i] != NULL, ("Assertion failed in %s line %d [failed malloc]", __func__, __LINE__));

        frenzyThreadArgs[i]->clientId = clientIds[thisClientId];
        frenzyThreadArgs[i]->threadNumber = i+1;
        frenzyThreadArgs[i]->totalThreads = totalThreads;
        frenzyThreadArgs[i]->state = INITIALIZING;
        frenzyThreadArgs[i]->running = false;

        int osrc = test_task_startThread( &frenzyThread[i] ,stealFrenzyThread, frenzyThreadArgs[i] ,"stealFrenzyThread");
        TEST_ASSERT(osrc == OK, ("Assertion failed in %s line %d", __func__, __LINE__));
    }

    for(int32_t i=0; i<totalThreads;i++)
    {
        if (!(volatile bool)frenzyThreadArgs[i]->running)
        {
            sched_yield();
            i--;
        }
    }

    test_log(testLOGLEVEL_TESTPROGRESS, "%d threads all running - giving them %d seconds", totalThreads, secondsToRun);

    // Let them all run for a while
    sleep(secondsToRun);

    if (quiesceThreads)
    {
        for(int32_t i=0; i<totalThreads;i++)
        {
            frenzyThreadArgs[i]->stop = true;
        }

        for(int32_t i=0; i<totalThreads;i++)
        {
            test_log(testLOGLEVEL_TESTPROGRESS, "Waiting for thread %d to end.",
                     frenzyThreadArgs[i]->threadNumber);
            (void)pthread_join(frenzyThread[i], NULL);
        }

        if (monitoringMessages)
        {
            rc = sync_ism_engine_destroyClientState(hStealFrenzyMonitorClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
            TEST_ASSERT(rc == OK, ("Assertion failed in %s line %d", __func__, __LINE__));
        }
    }

    for(int32_t i=0; i<totalThreads; i++)
    {
        test_log(testLOGLEVEL_VERBOSE,
                 "T%d [%s]: CreateRequests           %lu Async:%lu Sync:%lu Fail:%lu",
                 frenzyThreadArgs[i]->threadNumber,
                 frenzyThreadArgs[i]->clientId,
                 frenzyThreadArgs[i]->createRequestCount,
                 frenzyThreadArgs[i]->createAsyncCount,
                 frenzyThreadArgs[i]->createSyncCount,
                 frenzyThreadArgs[i]->createFailCount);
        test_log(testLOGLEVEL_VERBOSE,
                 "T%d [%s]: StealTransitions         %lu Creating:%lu Created:%lu Destroying:%lu Destroyed:%lu",
                 frenzyThreadArgs[i]->threadNumber,
                 frenzyThreadArgs[i]->clientId,
                 frenzyThreadArgs[i]->stealCount,
                 frenzyThreadArgs[i]->stealDuringCreating,
                 frenzyThreadArgs[i]->stealDuringCreated,
                 frenzyThreadArgs[i]->stealDuringDestroying,
                 frenzyThreadArgs[i]->stealDuringDestroyed);
        test_log(testLOGLEVEL_VERBOSE,
                 "T%d [%s]: CreateSuccessTransitions %lu CleanCreated:%lu Created:%lu Stolen:%lu",
                 frenzyThreadArgs[i]->threadNumber,
                 frenzyThreadArgs[i]->clientId,
                 frenzyThreadArgs[i]->createSuccessCount,
                 frenzyThreadArgs[i]->createSuccessDuringCleanCreating,
                 frenzyThreadArgs[i]->createSuccessDuringCreating,
                 frenzyThreadArgs[i]->createSuccessDuringStolen);

        if (quiesceThreads)
        {
            if (i%STEALFRENZY_THREADS_PER_CLIENTID == (STEALFRENZY_THREADS_PER_CLIENTID-1))
            {
                free(clientIds[i/STEALFRENZY_THREADS_PER_CLIENTID]);
            }
            free(frenzyThreadArgs[i]);
        }
    }

    #if defined(USE_THREAD_FOR_DESTROY_ON_STEAL) || defined(USE_THREAD_FOR_DESTROY_ON_CREATE)
    test_log(testLOGLEVEL_VERBOSE, "threadMax=%u\n", threadMax);
    #endif
}

int32_t parseArgs( int argc
                 , char *argv[]
                 , uint32_t *pphase
                 , uint32_t *pfinalphase
                 , testLogLevel_t *pverboseLevel
                 , int *ptrclvl
                 , char **padminDir
                 , bool *pgrabCore)
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { "phase", 1, NULL, 'p' },
        { "final", 1, NULL, 'f' },
        { "verbose", 1, NULL, 'v' },
        { "adminDir", 1, NULL, 'a' },
        { "grabCore", 1, NULL, 'g' },
        { NULL, 1, NULL, 0 } };
    int long_index;

    *pphase = 0;
    *pfinalphase = 10;
    *pverboseLevel = testLOGLEVEL_CHECK;
    *ptrclvl = 0;
    *padminDir = NULL;
    *pgrabCore = false;

    while ((opt = getopt_long(argc, argv, ":p:f:v:a:g0123456789", long_options, &long_index)) != -1)
    {
        switch (opt)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                *ptrclvl = opt - '0';
                break;
            case 'p':
               *pphase = (uint32_t)atoi(optarg);
               break;
            case 'f':
               *pfinalphase = (uint32_t)atoi(optarg);
               break;
            case 'v':
               *pverboseLevel = (testLogLevel_t)atoi(optarg);
               break;
            case 'a':
                *padminDir = optarg;
                break;
            case 'g':
                *pgrabCore = true;
                break;
            case '?':
            default:
               usage=1;
               break;
        }
    }
    
    if (*pverboseLevel > testLOGLEVEL_VERBOSE) *pverboseLevel=testLOGLEVEL_VERBOSE; /* BEAM suppression: constant condition */

    if (usage)
    {
        fprintf(stderr, "Usage: %s [-v 0-9] [-0...-9] [-p phase] [-f final-phase]\n", argv[0]);
        fprintf(stderr, "       -p (--phase)         - Set initial phase\n");
        fprintf(stderr, "       -f (--final)         - Set final phase\n");
        fprintf(stderr, "       -v (--verbose)       - Test Case verbosity\n");
        fprintf(stderr, "       -a (--adminDir)      - Admin directory to use\n");
        fprintf(stderr, "       -0 .. -9             - ISM Trace level\n");
    }

   return usage;
}

/*********************************************************************/
/* NAME:        test_clientStateRestart                              */
/* DESCRIPTION: This program has two threads creating a durable      */
/*              clientstate with the same name one after other and   */
/*              restarts and check that only one is in the store     */
/*                                                                   */
/* USAGE:       test_queue4 [-r] -t <testno>                         */
/*                  -r : When -r is not specified the program runs   */
/*                       Phase 1 and defines and loads the queue(s). */
/*                       When -r is specified, the program runs      */
/*                       Phase 2 and recovers the queues and messages*/
/*                       from the store and verifies everything has  */
/*                       been loaded correctly.                      */
/*                  -t <no> : Testcase number                        */
/*                     1 - Define queue and load with persistent     */
/*                         messages.                                 */
/*                     2 - Define queue and load with non-persistent */
/*                         messages.                                 */
/*                     3 - Define queue and load with mixture of     */
/*                         persistent and non-persistent messages    */
/*                     4 - Define queue and load with transactional  */
/*                         messages, leaving last transaction        */
/*                         uncommited so messages should not be      */
/*                         recovered.                                */
/*********************************************************************/
int main(int argc, char *argv[])
{
    int trclvl = 5;
    testLogLevel_t testLogLevel = testLOGLEVEL_TESTDESC;
    int rc = 0;
    char *newargv[15];
    int i;
    uint32_t phase;
    uint32_t finalphase;
    time_t curTime;
    struct tm tm = { 0 };
    char *adminDir=NULL;
    pthread_t firstClientThread;
    bool grabCore;

    pthread_spin_init(&threadCountLock, PTHREAD_SCOPE_PROCESS);

    /*****************************************************************/
    /* Parse arguments                                               */
    /*****************************************************************/
    rc = parseArgs( argc
                  , argv
                  , &phase
                  , &finalphase
                  , &testLogLevel
                  , &trclvl
                  , &adminDir
                  , &grabCore);
    if (rc != 0)
    {
        goto mod_exit;
    }


    /*****************************************************************/
    /* Process Initialise                                            */
    /*****************************************************************/
    test_setLogLevel(testLogLevel);

    rc = test_processInit(trclvl, adminDir);
    if (rc != OK)
    {
        goto mod_exit;
    }

    char localAdminDir[1024];
    if (adminDir == NULL && test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir)) == true)
    {
        adminDir = localAdminDir;
    }

    CU_initialize_registry();

    /*************************************************************/
    /* Prepare the restart command                               */
    /*************************************************************/
    bool phasefound=false, adminDirfound=false;
    char textphase[16];
    sprintf(textphase, "%d", phase+1);
    TEST_ASSERT(argc < 12, ("argc was %d", argc));

    for (i=0; i < argc; i++)
    {
        newargv[i]=argv[i];
        if (strcmp(newargv[i], "-p") == 0)
        {
          newargv[i+1]=textphase;
          i++;
          phasefound=true;
        }
        if (strcmp(newargv[i], "-a") == 0)
        {
          newargv[i+1]=adminDir;
          i++;
          adminDirfound=true;
        }
    }

    if (!phasefound)
    {
        newargv[i++]="-p";
        newargv[i++]=textphase;
    }

    if (!adminDirfound)
    {
        newargv[i++]="-a";
        newargv[i++]=adminDir;
    }

    newargv[i]=NULL;

    /*****************************************************************/
    /* Seed the random number generator and prepare the time for     */
    /* printing                                                      */
    /*****************************************************************/
    curTime = time(NULL);
    srand(curTime);

    (void)localtime_r(&curTime, &tm);

    const char *testName;
    int32_t testDuration;

    if (phase < finalphase/2)
    {
        testName = "test_tryCreateDuplicateClientStates";
        testDuration = -1; // unlimited
    }
    else
    {
        testName = "test_stealFrenzy";
        testDuration = (int32_t)(rand()%(STEALFRENZY_MAX_SECONDS+1));

        // Make the callback queues a bit shorter for these tests to drive some
        // different code paths.
        setenv("IMA_TEST_FAKE_ASYNC_CALLBACK_CAPACITY", "6000", false);
    }

    /*****************************************************************/
    /* If this is the first run for this test then cold start the    */
    /* store.                                                        */
    /*****************************************************************/
    if (phase == 0)
    {
        if (testDuration != -1)
        {
            test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Running test phase 0 cold-start [%s] (%d seconds)",
                     tm.tm_hour, tm.tm_min, tm.tm_sec, testName, testDuration);
        }
        else
        {
            test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Running test phase 0 cold-start [%s]",
                     tm.tm_hour, tm.tm_min, tm.tm_sec, testName);
        }

        /*************************************************************/
        /* Start up the Engine - Cold Start                          */
        /*************************************************************/
        rc = test_engineInit_COLD;
        if (rc != 0)
        {
            goto mod_exit;
        }
        test_log(testLOGLEVEL_TESTPROGRESS, "Engine cold started");
    }
    else
    {
        if (testDuration != -1)
        {
            test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Running test phase %d warm-start [%s] (%d seconds)",
                    tm.tm_hour, tm.tm_min, tm.tm_sec, phase, testName, testDuration);
        }
        else
        {
            test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Running test phase %d warm-start [%s]",
                    tm.tm_hour, tm.tm_min, tm.tm_sec, phase, testName);

        }

        char *backupDir;

        if ((backupDir = getenv("IMA_CLIENTSTATERESTART_BACKUPDIR")) != NULL)
        {
            char *qualifyShared = getenv("QUALIFY_SHARED");

            // space for QUALIFY_SHARED value and com.ibm.ism::%u:store (and null terminator)
            char shmName[(qualifyShared?strlen(qualifyShared):0)+41];
            sprintf(shmName, "com.ibm.ism::%u:store%s", getuid(), qualifyShared ? qualifyShared : "");

            // space for /dev/shm (and null terminator)
            char sourceName[strlen(shmName)+10];
            sprintf(sourceName, "/dev/shm/%s", shmName);

            // space for / (and null terminator)
            char backupName[strlen(backupDir)+strlen(shmName)+2];
            sprintf(backupName, "%s/%s", backupDir, shmName);

            // Allow space for 50 bytes of command (and null terminator)
            char cmd[strlen(sourceName)+strlen(backupName)+51];
            struct stat statBuf;

            if (stat(backupDir, &statBuf) != 0)
            {
                (void)mkdir(backupDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            }

            if (stat(sourceName, &statBuf) == 0)
            {
                sprintf(cmd, "/bin/cp -f %s %s", sourceName, backupName);
                if (system(cmd) == 0)
                {
                    test_log(testLOGLEVEL_TESTPROGRESS,
                            "Copied store shm %s to %s\n", sourceName, backupName);
                }
            }
        }

        /*************************************************************/
        /* Start up the Engine - Warm Start                          */
        /*************************************************************/
        test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start");
        rc = test_engineInit_WARMAUTO;
        if (rc != 0)
        {
            goto mod_exit;
        }
        test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm started");

        // If we got this far, the engine started up okay - so clean up the previous
        // core file.
        // NOTE: THIS IS VERY COURSE!
        if (grabCore)
        {
            char *PWD = getenv("PWD");
            struct dirent *ent;

            DIR *dp = opendir(PWD);

            if (dp == NULL)
            {
                rc = 9;
                test_log(testLOGLEVEL_ERROR, "FAILED to open PWD '%s' [errno:%d]\n", PWD, errno);
                goto mod_exit;
            }

            size_t PWDLen = strlen(PWD);

            while ((ent = readdir (dp)) != NULL)
            {
                if (ent->d_type == DT_REG)
                {
                    if ((strstr(ent->d_name, "core") == ent->d_name) || (strcmp(ent->d_name, "doneIt.good") == 0))
                    {
                        char fullPath[PWDLen+strlen(ent->d_name)+2];

                        sprintf(fullPath, "%s/%s", PWD, ent->d_name);

                        printf("REMOVING '%s'\n", fullPath);

                        rc = unlink(fullPath);

                        if (rc != 0)
                        {
                            rc = 10;
                            test_log(testLOGLEVEL_ERROR, "FAILED to unlink core file '%s' [errno:%d]\n", fullPath, errno);
                            goto mod_exit;
                        }
                    }
                }
            }

            closedir(dp);
        }
    }

    /*****************************************************************/
    /* Actually run the test .                                       */
    /*****************************************************************/
    if (!strcmp(testName, "test_tryCreateDuplicateClientStates"))
    {
        test_tryCreateDuplicateClientStates(&firstClientThread);
    }
    else if (!strcmp(testName, "test_stealFrenzy"))
    {
        firstClientThread = 0;
        test_stealFrenzy(testDuration, phase == finalphase, true);
    }

    /*****************************************************************/
    /* And restart the server.                                       */
    /*****************************************************************/
    if (phase < finalphase)
    {
        bool didCAS = __sync_bool_compare_and_swap(&restarting, false, true);
        TEST_ASSERT(didCAS == true, ("Failed to CAS reStarting flag"));

        if (grabCore)
        {
            test_kill_me(newargv[0], newargv);
        }
        else
        {
            rc = test_execv(newargv[0], newargv);
        }
        TEST_ASSERT(false, ("'test_execv' failed. rc = %d", rc));
    }
    else
    {
        test_log(2, "Success. Test complete!\n");
        if (firstClientThread != 0)
        {
            test_log(4, "Waiting for firstClient thread to end.");
            endDelays = true;
            (void)pthread_join(firstClientThread, NULL);
        }
        test_log(5, "Thread ended.");

        test_engineTerm(true);
        test_processTerm(false);
        test_removeAdminDir(adminDir);

        if (grabCore)
        {
            FILE *fp = fopen("doneIt.good", "w");
            fprintf(fp, "WORKED\n");
            fclose(fp);
        }
    }

mod_exit:
    pthread_spin_destroy(&threadCountLock);
    return (rc);
}
