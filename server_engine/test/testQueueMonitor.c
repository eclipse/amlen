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
/* Module Name: testQueueMonitor                                    */
/*                                                                  */
/* Description: This test starts creates 'N' queues, starts a       */
/*              background thread to put messages randomly to the   */
/*              queues while calling the monitorng function.        */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <getopt.h>

#include <ismutil.h>
#include "engine.h"
#include "engineInternal.h"
#include "engineCommon.h"
#include "queueNamespace.h"
#include "messageExpiry.h"

#include "test_utils_initterm.h"
#include "test_utils_log.h"
#include "test_utils_task.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"

/********************************************************************/
/* Data types                                                       */
/********************************************************************/
typedef struct TaskContext_t
{
    ismEngine_SessionHandle_t hSession;
    uint32_t queueCount;
} TaskContext_t;

#define CALLBACK_MAX_MESSAGES 20

typedef struct tag_QueueDetails_t
{
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    ieqnQueueHandle_t QueueHandle;
    uint32_t msgsReceived;
    ismEngine_DeliveryHandle_t DeliveryHandles[CALLBACK_MAX_MESSAGES];
} QueueDetails_t;

/********************************************************************/
/* Function prototypes                                              */
/********************************************************************/
int ProcessArgs( int argc
               , char **argv
               , uint64_t *pSeedVal
               , uint32_t *pQueueCount
               , uint32_t *pIterations
               , uint32_t *pRequestType
               , uint32_t *pMaxResults
               , char **pFilterQueue
               , char **pFilterScope);

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
                     , void *                     pConsumerContext);

void backgroundProducer(void *context);

/********************************************************************/
/* Global data                                                      */
/********************************************************************/
static ismEngine_ClientStateHandle_t hClient = NULL;
static uint32_t logLevel = testLOGLEVEL_CHECK;


/********************************************************************/
/* MAIN                                                             */
/********************************************************************/
int main(int argc, char *argv[])
{
    int trclvl = 4;
    int i, loop;
    int rc;
    uint64_t seedVal = 0;
    ismEngine_SessionHandle_t hSession;
    uint32_t queueCount;
    uint32_t iterations;
    uint32_t maxResults;
    uint32_t requestType;
    char *filterQueue;
    char *filterScope;
    ism_time_t startTime, endTime, diffTime;
    double diffTimeSecs;
    TaskContext_t bgProdContext;
    uint64_t backgroundProdRate = 1000;
    void *bghandle;
    char QueueName[64];
    uint32_t resultCount = 0;
    ismEngine_QueueMonitor_t *results = NULL;
    QueueDetails_t *Queues = NULL;

    /************************************************************************/
    /* Parse the command line arguments                                     */
    /************************************************************************/
    rc = ProcessArgs( argc
                    , argv
                    , &seedVal
                    , &queueCount
                    , &iterations
                    , &requestType
                    , &maxResults
                    , &filterQueue
                    , &filterScope );
    TEST_ASSERT(rc == OK, ("ERROR: ProcessArgs returned rc=%d\n", rc));

    test_setLogLevel(logLevel);

    rc = test_processInit(trclvl, NULL);
    TEST_ASSERT(rc == OK, ("ERROR: test_processInit returned rc=%d\n", rc));

    rc = test_engineInit_DEFAULT;
    TEST_ASSERT(rc == OK, ("ERROR: test_engineInit_DEFAULT returned rc=%d\n", rc));

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    /*****************************************************************/
    /* Seed the random number generator                              */
    /*****************************************************************/
    if (seedVal == 0)
        seedVal = time(NULL);
    srand(seedVal);

    test_log(testLOGLEVEL_TESTDESC, "Monitor test create %d queues - seed(%ld)",
             queueCount, seedVal);

    rc = ism_engine_createClientState("QueueMonitor Test",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createClientState() returned %d\n", rc));

    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0 
                               , NULL);
    TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createSession(consumer) returned %d\n", rc));

    /************************************************************************/
    /* Create the queues                                                    */
    /************************************************************************/
    Queues = calloc(sizeof(QueueDetails_t), queueCount);
    QueueDetails_t *pQueueDetails;

    startTime = ism_common_currentTimeNanos();

    for (i=0; i < queueCount; i++)
    {
        sprintf(QueueName, "Queue.%d", i+1);
        pQueueDetails = &(Queues[i]);

        rc=ieqn_createQueue( pThreadData
                           , QueueName
                           , intermediate
                           , ismQueueScope_Server
                           , hClient
                           , NULL
                           , NULL
                           , &(pQueueDetails->QueueHandle));
        TEST_ASSERT(rc == OK, ("Failed to create queue rc=%d\n", rc));

        rc=ism_engine_createSession( hClient
                                   , ismENGINE_CREATE_SESSION_OPTION_NONE
                                   , &(pQueueDetails->hSession)
                                   , NULL
                                   , 0
                                   , NULL);
        TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createSession(bgPubContext) returned %d\n", rc));

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
        rc=ism_engine_createConsumer( pQueueDetails->hSession
                                    , ismDESTINATION_QUEUE
                                    , QueueName
                                    , &subAttrs
                                    , NULL // Unused for QUEUE
                                    , &pQueueDetails
                                    , sizeof(pQueueDetails)
                                    , ConsumerCallback
                                    , NULL
                                    , ismENGINE_CONSUMER_OPTION_ACK|ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID
                                    , &(pQueueDetails->hConsumer)
                                    , NULL
                                    , 0
                                    , NULL);
        TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createConsumer returned %d\n", rc));

        rc=ism_engine_startMessageDelivery( pQueueDetails->hSession
                                          , 0
                                          , NULL
                                          , 0
                                          , NULL);
        TEST_ASSERT(rc == OK, ("Failed to start message delivery"));

        test_log(testLOGLEVEL_VERBOSE, "Created queue %s", QueueName);
    }

    endTime = ism_common_currentTimeNanos();
    diffTime = endTime-startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;

    test_log(testLOGLEVEL_CHECK, "Time to create %d queues is %.2f secs. (%ldns)",
             queueCount, diffTimeSecs, diffTime);

    /************************************************************************/
    /* Start the background threads, to publish messages to the queues      */
    /************************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &bgProdContext.hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createSession returned %d\n", rc));

    bgProdContext.queueCount = queueCount;

    test_task_create("bgProducer", backgroundProducer, &bgProdContext, backgroundProdRate, &bghandle);


    /************************************************************************/
    /* Now loop 'N' times while messaes are being processed query the       */
    /* statistics from the queues                                           */
    /************************************************************************/
    ism_prop_t *filterProperties = ism_common_newProperties(10);
    ism_field_t f;

    f.type = VT_String;
    if (filterQueue != NULL)
    {
        f.val.s = filterQueue;
        ism_common_setProperty(filterProperties, "QueueName", &f);
    }

    if (filterScope != NULL)
    {
        f.val.s = filterScope;
        ism_common_setProperty(filterProperties, "Scope", &f);
    }

    // Set of valid request types
    ismEngineMonitorType_t randomTypes[] = {ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                            ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS,
                                            ismENGINE_MONITOR_HIGHEST_BUFFEREDPERCENT,
                                            ismENGINE_MONITOR_LOWEST_BUFFEREDPERCENT,
                                            ismENGINE_MONITOR_HIGHEST_PRODUCERS,
                                            ismENGINE_MONITOR_LOWEST_PRODUCERS,
                                            ismENGINE_MONITOR_HIGHEST_CONSUMERS,
                                            ismENGINE_MONITOR_LOWEST_CONSUMERS,
                                            ismENGINE_MONITOR_HIGHEST_PRODUCEDMSGS,
                                            ismENGINE_MONITOR_LOWEST_PRODUCEDMSGS,
                                            ismENGINE_MONITOR_HIGHEST_CONSUMEDMSGS,
                                            ismENGINE_MONITOR_LOWEST_CONSUMEDMSGS,
                                            ismENGINE_MONITOR_HIGHEST_BUFFEREDHWMPERCENT,
                                            ismENGINE_MONITOR_LOWEST_BUFFEREDHWMPERCENT,
                                            ismENGINE_MONITOR_HIGHEST_EXPIREDMSGS,
                                            ismENGINE_MONITOR_LOWEST_EXPIREDMSGS,
                                            ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS,
                                            ismENGINE_MONITOR_LOWEST_DISCARDEDMSGS,
                                            ismENGINE_MONITOR_ALL_UNSORTED};

    /* Do each of the random types once */
    for (loop=0; loop<(sizeof(randomTypes)/sizeof(randomTypes[0])); loop++)
    {
        rc = ism_engine_getQueueMonitor( &results
                                       , &resultCount
                                       , randomTypes[loop]
                                       , maxResults
                                       , filterProperties);

        TEST_ASSERT(rc == OK, ("ERROR: ism_engine_getQueueMonitor returned %d\n", rc));

        ism_engine_freeQueueMonitor(results);
    }

    ism_prop_t *filterProperties2 = ism_common_newProperties(10);
    f.type = VT_String;
    f.val.s = "NotAName";
    ism_common_setProperty(filterProperties2, "QueueName", &f);

    /* Do each of the random types once */
    for (loop=0; loop<(sizeof(randomTypes)/sizeof(randomTypes[0])); loop++)
    {
        rc = ism_engine_getQueueMonitor( &results
                                       , &resultCount
                                       , randomTypes[loop]
                                       , maxResults
                                       , filterProperties2);

        TEST_ASSERT(rc == OK, ("ERROR: ism_engine_getQueueMonitor returned %d\n", rc));
        TEST_ASSERT(resultCount == 0, ("ERROR: should not find anything but found %d\n", rc));

        ism_engine_freeQueueMonitor(results);
    }


    /* Do a selection with some delays */
    for (loop=0; loop < iterations; loop++)
    {
        ismEngineMonitorType_t useType = (ismEngineMonitorType_t)requestType;

        if (useType == ismENGINE_MONITOR_NONE)
        {
            useType = randomTypes[rand()%(sizeof(randomTypes)/sizeof(randomTypes[0]))];
        }

        sleep (1);

        /********************************************************************/
        /* Now call the monitoring routine to query the queues              */
        /********************************************************************/
        startTime = ism_common_currentTimeNanos();

        rc = ism_engine_getQueueMonitor( &results
                                       , &resultCount
                                       , useType
                                       , maxResults
                                       , filterProperties);

        endTime = ism_common_currentTimeNanos();

        TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_getQueueMonitor returned %d\n", rc));

        diffTime = endTime-startTime;
        diffTimeSecs = ((double)diffTime) / 1000000000;

        test_log(testLOGLEVEL_CHECK, "Iteration %d: Time to query monitoring is %.2f secs. (%ldns) returning %d entries",
                 loop, diffTimeSecs, diffTime, resultCount);

        if (logLevel >= testLOGLEVEL_TESTPROGRESS)
        {
            for(i=0; i<resultCount; i++)
            {
                test_log(testLOGLEVEL_TESTPROGRESS, "Queue: \"%s\" [%s]"
                         "ProdMsgs: %-6lu ConsMsgs: %-6lu Buff: %-6lu MaxBuff: %-6lu Rej: %-6lu Dis: %-6lu Exp: %-6lu BuffPerc: %3.2f%%",
                         results[i].queueName,
                         results[i].scope ? "Server" : "Client",
                         results[i].stats.ProducedMsgs,
                         results[i].stats.ConsumedMsgs,
                         results[i].stats.BufferedMsgs,
                         results[i].stats.BufferedMsgsHWM,
                         results[i].stats.RejectedMsgs,
                         results[i].stats.DiscardedMsgs,
                         results[i].stats.ExpiredMsgs,
                         results[i].stats.BufferedPercent);
            }
        }

        // Free up the results
        if (resultCount != 0)
        {
            ism_engine_freeQueueMonitor( results );
        }

        // Poke the expiry reaper every so often
        if ((rand()%10) > 5) ieme_wakeMessageExpiryReaper(pThreadData);
    }

    if (filterQueue != NULL) free(filterQueue);
    if (filterScope != NULL) free(filterScope);

    ism_common_freeProperties(filterProperties);

    test_task_stopall(testLOGLEVEL_CHECK);

    rc=ism_engine_destroySession( hSession
                                , NULL
                                , 0
                                , NULL);
    TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_destroySession returned %d\n", rc));

    rc=ism_engine_destroyClientState( hClient
                                    , ismENGINE_DESTROY_CLIENT_OPTION_NONE
                                    , NULL
                                    , 0
                                    , NULL);
    TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_destroyClientState returned %d\n", rc));

    rc = test_engineTerm(true);
    TEST_ASSERT(rc == OK, ("ERROR:  test_engineTerm returned %d\n", rc));

    test_processTerm(true);
    TEST_ASSERT(rc == OK, ("ERROR:  test_processTerm returned %d\n", rc));

    free(Queues);

    return rc;
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
                     , void *                     pConsumerContext)
{
    uint32_t rc;
    uint32_t i;
    bool confirmMessages = false;
    QueueDetails_t *pQueueDetails = *(QueueDetails_t **)pConsumerContext;

    if (state == ismMESSAGE_STATE_DELIVERED)
    {
        pQueueDetails->DeliveryHandles[pQueueDetails->msgsReceived] = hDelivery;

        rc = ism_engine_confirmMessageDelivery( pQueueDetails->hSession
                                              , NULL
                                              , hDelivery
                                              , ismENGINE_CONFIRM_OPTION_RECEIVED
                                              , NULL
                                              , 0
                                              , NULL );
        TEST_ASSERT(rc == OK, ("Failed to acknowledge delivery of message. rc = %d", rc));

        pQueueDetails->msgsReceived++;
        if (pQueueDetails->msgsReceived == CALLBACK_MAX_MESSAGES)
        {
            confirmMessages = true;
        }
        else
        {
            confirmMessages = ((rand() % (CALLBACK_MAX_MESSAGES / 3)) == 0)?true:false;
        }

        if (confirmMessages)
        {
            for (i=0; i < pQueueDetails->msgsReceived; i++)
            {
                rc = ism_engine_confirmMessageDelivery( pQueueDetails->hSession
                                                      , NULL
                                                      , pQueueDetails->DeliveryHandles[i]
                                                      , ismENGINE_CONFIRM_OPTION_CONSUMED
                                                      , NULL
                                                      , 0
                                                      , NULL );

                TEST_ASSERT(rc == OK, ("Failed to acknowledge(CONSUMED) delivery handle"));
            }

            pQueueDetails->msgsReceived = 0;
        }
    }

    ism_engine_releaseMessage(hMessage);


    return true;
}


/****************************************************************************/
/* Background publish thread                                                */
/****************************************************************************/
void backgroundProducer(void *context)
{
    uint32_t rc;
    TaskContext_t *pContext = (TaskContext_t *)context;
    char QueueName[256];

    // Pick a queue number to which we are going to producer a message
    uint32_t QueueNo = (rand() % pContext->queueCount) + 1;
    sprintf(QueueName, "Queue.%d", QueueNo);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismEngine_MessageHandle_t  hMessage;
    ismMessageAreaType_t areaTypes[] = { ismMESSAGE_AREA_PAYLOAD };
    char Buffer[]="Simple message";
    void *areas[] = { (void *)Buffer };
    size_t areaLengths[] = { strlen(Buffer) +1 };
    header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
    // 20% of the time, add expiry to the messages
    if (rand()%100 > 80)
    {
        header.Expiry = ism_common_nowExpire() + (uint32_t)(rand()%10);
    }

    rc=ism_engine_createMessage( &header
                               , 1
                               , areaTypes
                               , areaLengths
                               , areas
                               , &hMessage);
    if (rc == OK)
    {
        test_log(testLOGLEVEL_VERBOSE, "Background publisher sending message on queue %s", QueueName);

        rc = ism_engine_putMessageOnDestination(pContext->hSession,
                                                ismDESTINATION_QUEUE,
                                                QueueName,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        if (rc != OK)
        {
            test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_putMessageOnDestination() returned %d\n", rc);
        }
    }

    return;
}

/****************************************************************************/
/* ProcessArgs                                                              */
/****************************************************************************/
int ProcessArgs( int argc
               , char **argv
               , uint64_t *pSeedVal
               , uint32_t *pQueueCount
               , uint32_t *pIterations
               , uint32_t *pRequestType
               , uint32_t *pMaxResults
               , char **pFilterQueue
               , char **pFilterScope)
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { "seed", 1, NULL, 0 }, 
        { "iterations", 1, NULL, 'i' },
        { "requestType", 1, NULL, 't' },
        { "numQueues", 1, NULL, 'n' },
        { "maxResults", 1, NULL, 'r' },
        { "filterQueue", 1, NULL, 'Q' },
        { "filterScope", 1, NULL, 'S' },
        { NULL, 1, NULL, 0 } };
    int long_index;

    *pSeedVal = 0;
    *pQueueCount = 100;
    *pIterations = 10;
    *pRequestType = ismENGINE_MONITOR_NONE;
    *pMaxResults = 15;
    *pFilterQueue = NULL;
    *pFilterScope = NULL;

    while ((opt = getopt_long(argc, argv, ":i:t:n:r:v:Q:S:", long_options, &long_index)) != -1)
    {
        switch (opt)
        {
            case 0:
                if (long_index == 0)
                {
                    *pSeedVal = atol(optarg);
                }
                break;
            case 'i':
                *pIterations = atoi(optarg);
                break;
            case 't':
                *pRequestType = atoi(optarg);
                break;
            case 'n':
                *pQueueCount = atoi(optarg);
                break;
            case 'r':
                *pMaxResults = atoi(optarg);
                break;
            case 'v':
               logLevel = atoi(optarg);
               if (logLevel > testLOGLEVEL_VERBOSE)
                   logLevel = testLOGLEVEL_VERBOSE;
               break;
            case 'Q':
                *pFilterQueue = strdup(optarg);
                break;
            case 'S':
                *pFilterScope = strdup(optarg);
                break;
            default: 
                usage=1;
                break;
        }
    }

    if ( (*pIterations == 0) || (*pMaxResults == 0))
    {
        usage=1;
    }
    if ( (*pFilterScope != NULL) &&
         (strcmp(*pFilterScope, "Temporary") != 0) &&
         (strcmp(*pFilterScope, "Permanent") != 0) )
    {
        usage=1;
    }

    if (usage)
    {
        fprintf(stderr, "Usage: %s [-i iterations] [-n numQueues] [-v] [-t requestType] [-r maxResults]\n", argv[0]);
        fprintf(stderr, "       -v - logLevel 0-5 [2]\n");
        fprintf(stderr, "       -i - number of itertions [10]\n");
        fprintf(stderr, "       -n - number of queues to create [100]\n");
        fprintf(stderr, "       -t - type of request 0-%d (see defn of ismEngineMonitorType_t) [RANDOM]\n",
                ismENGINE_MONITOR_MAX-1);
        fprintf(stderr, "       -r - specify the maximum number of results to return [50]\n");
        fprintf(stderr, "       --seed - Random seed value\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Various filters can be applied to limit the results with the following additional switches:\n");
        fprintf(stderr, "       -Q - Filter by wildcarded queue string [none]\n");
        fprintf(stderr, "       -S - Filter by Queue Scope 'Server' or 'Client [none]'\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "The test program creates a number of queues.\n");
        fprintf(stderr, "It then starts a number of threads to randomly put messages\n");
        fprintf(stderr, "to the queues while monitoirng information is queried about\n");
        fprintf(stderr, "the queues.\n");
    }

    return usage;
}
