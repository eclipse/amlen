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
/* Module Name: testSubMonitor                                      */
/*                                                                  */
/* Description: This test starts creates 'N' subscriptions,         */
/*              publishes some messages to a subset of the          */
/*              subscription then queue monitoring information      */
/*                                                                  */
/*              The test has some background threads, which         */
/*                  subscribe      300/sec                          */
/*                  unsubscribe    300/sec                          */
/*                  publish        775/sec                          */
/*              Assuming the number of subscriptions is 1,000,000   */
/*              the default rates for these are shown above and     */
/*              are calculated based upon a subscription which      */
/*              unsubscribes and re-subscribes 20 times in an 18    */
/*              hour period, and during the same 18 hour period     */
/*              each subscription receives approximatley 50         */
/*              messages.                                           */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <getopt.h>

#include <ismutil.h>
#include "engine.h"
#include "engineCommon.h"
#include "topicTree.h"

#include "test_utils_initterm.h"
#include "test_utils_log.h"
#include "test_utils_task.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"

#define TOPIC_PATTERN "/Bank/Customers/Accounts/ABC_%010d"

/********************************************************************/
/* Data types                                                       */
/********************************************************************/
typedef struct TaskContext_t
{
    ismEngine_SessionHandle_t hSession;
    bool Durable;
    uint32_t topicCount;
    char TopicPattern[256];
} TaskContext_t;

typedef struct freeSub_t 
{
    uint32_t accountNo;
    struct freeSub_t *next;
} freeSub_t;

/********************************************************************/
/* Function prototypes                                              */
/********************************************************************/
int ProcessArgs( int argc
               , char **argv
               , uint64_t *pSeedVal
               , bool *pDurable
               , uint32_t *pSubCount
               , uint32_t *pPubCount
               , uint32_t *pPubMsgCount
               , uint32_t *pbgSubRate
               , uint32_t *pbgPubRate
               , bool *pBackgroundPubs
               , bool *pBackgroundSubs
               , uint32_t *pRequestType
               , uint32_t *pMaxResults
               , char **pFilterTopic
               , char **pFilterClientId
               , char **pFilterSubName
               , char **pFilterSubType
               , char **pDumpFile
               , int32_t *pDumpDetail
               , int32_t *pDumpUserData
               , bool *pRandomSubOpts
               , bool *pSingleTopic );

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

void backgroundPublisher(void *context);
void backgroundSubscriber(void *context);
void backgroundUnsubscriber(void *context);

void backgroundUnsubscriberAsync( int32_t retCode
                                , void *handle
                                , void *pContext);

/********************************************************************/
/* Global data                                                      */
/********************************************************************/
static ismEngine_ClientStateHandle_t hClient = NULL;
static ismEngine_ConsumerHandle_t *Consumers = NULL;

static struct freeSub_t *FreeSubList = NULL;
static pthread_mutex_t FreeSubListLock = PTHREAD_MUTEX_INITIALIZER;

static uint32_t logLevel = testLOGLEVEL_CHECK;

static uint32_t RANDOM_SUBOPTS[] = {ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                                    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE,
                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                                    ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE};
#define NUM_RANDOM_SUBOPTS (sizeof(RANDOM_SUBOPTS)/sizeof(RANDOM_SUBOPTS[0]))

#define testClientId "SubMonitor Test"

/********************************************************************/
/* MAIN                                                             */
/********************************************************************/
int main(int argc, char *argv[])
{
    int trclvl = 4;
    int i, j;
    int rc;
    uint64_t seedVal = 0;
    ismEngine_SessionHandle_t hSession;
    bool Durable;
    uint32_t subCount;
    uint32_t pubCount;
    uint32_t pubMsgCount;
    bool backgroundPubs;
    bool backgroundSubs;
    uint32_t backgroundSubRate;
    uint32_t backgroundPubRate;
    uint32_t maxResults;
    uint32_t requestType;
    char *filterTopic;
    char *filterClientId;
    char *filterSubName;
    char *filterSubType;
    char *dumpFile;
    int32_t dumpDetail;
    int32_t dumpUserData;
    bool randomSubOpts;
    bool singleTopic;
    char TopicString[256];
    ism_time_t startTime, endTime, diffTime;
    double diffTimeSecs;
    TaskContext_t bgPubContext;
    TaskContext_t bgDestroySubContext;
    TaskContext_t bgCreateSubContext;
    void *bghandle;

    /************************************************************************/
    /* Parse the command line arguments                                     */
    /************************************************************************/
    rc = ProcessArgs( argc
                    , argv
                    , &seedVal
                    , &Durable
                    , &subCount
                    , &pubCount
                    , &pubMsgCount
                    , &backgroundSubRate
                    , &backgroundPubRate
                    , &backgroundPubs
                    , &backgroundSubs
                    , &requestType
                    , &maxResults
                    , &filterTopic
                    , &filterClientId
                    , &filterSubName
                    , &filterSubType
                    , &dumpFile
                    , &dumpDetail
                    , &dumpUserData
                    , &randomSubOpts
                    , &singleTopic );
    if (rc != OK)
    {
        return rc;
    }

    test_setLogLevel(logLevel);

    rc = test_processInit(trclvl, NULL);
    if (rc != OK) return rc;
    
    rc = test_engineInit_DEFAULT;
    if (rc != OK) return rc;

    /*****************************************************************/
    /* Seed the random number generator                              */
    /*****************************************************************/
    if (seedVal == 0)
        seedVal = time(NULL);
    srand(seedVal);

    test_log(testLOGLEVEL_TESTDESC, "Monitor test create %d %ssubscriptions - seed(%ld)",
             subCount, Durable?"durable ":"", seedVal);

    rc = ism_engine_createClientState(testClientId,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    if (rc != OK)
    {
        test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_createClientState() returned %d\n", rc);
        return rc;
    }

    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0 
                               , NULL);
    if (rc != OK)
    {
        test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_createSession(consumer) returned %d\n", rc);
        return rc;
    }
    
    /************************************************************************/
    /* Create the subscriptions                                             */
    /************************************************************************/
    Consumers = calloc(sizeof(ismEngine_ConsumerHandle_t), subCount);

    startTime = ism_common_currentTimeNanos();

    for (i=0; i < subCount; i++)
    {
        ismEngine_SubscriptionAttributes_t subAttributes = {0};

        if (randomSubOpts == false)
        {
            subAttributes.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
        }
        else
        {
            subAttributes.subOptions = RANDOM_SUBOPTS[rand()%NUM_RANDOM_SUBOPTS];
        }

        sprintf(TopicString, TOPIC_PATTERN, singleTopic ? 1 : i+1);

        uint32_t consumerOptions = ((subAttributes.subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK) == ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE)
                                    ? ismENGINE_CONSUMER_OPTION_NONE
                                    : ismENGINE_CONSUMER_OPTION_ACK;
        if (Durable)
        {
            char subName[64];
            sprintf(subName, "subMonitor.%d", i+1);

            subAttributes.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;

            rc=ism_engine_createSubscription( hClient
                                            , subName
                                            , NULL
                                            , ismDESTINATION_TOPIC
                                            , TopicString
                                            , &subAttributes
                                            , NULL // Owning client same as requesting client
                                            , NULL
                                            , 0
                                            , NULL );
            if (rc != OK)
            {
                test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_createDurableSubscription() returned %d\n", rc);
                return rc;
            }

            rc=ism_engine_createConsumer( hSession
                                        , ismDESTINATION_SUBSCRIPTION
                                        , subName
                                        , NULL // Consuming from an existing subscription
                                        , NULL // Owning client same as session client
                                        , NULL
                                        , 0
                                        , ConsumerCallback
                                        , NULL
                                        , consumerOptions
                                        , &Consumers[i]
                                        , NULL
                                        , 0
                                        , NULL);
            if (rc != OK)
            {
                test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_createConsumer() returned %d\n", rc);
                return rc;
            }
        }
        else
        {
            rc=ism_engine_createConsumer( hSession
                                        , ismDESTINATION_TOPIC
                                        , TopicString
                                        , &subAttributes
                                        , NULL // Unused for TOPIC
                                        , NULL
                                        , 0
                                        , ConsumerCallback
                                        , NULL
                                        , consumerOptions
                                        , &Consumers[i]
                                        , NULL
                                        , 0
                                        , NULL);
            if (rc != OK)
            {
                test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_createConsumer() returned %d\n", rc);
                return rc;
            }
        }
        test_log(testLOGLEVEL_VERBOSE, "Created subscription %d on Topic %s",
                 i+1, TopicString);
    }

    endTime = ism_common_currentTimeNanos();
    diffTime = endTime-startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;

    test_log(testLOGLEVEL_CHECK, "Time to create %d subscriptions is %.2f secs. (%ldns)",
             subCount, diffTimeSecs, diffTime);

    // Find the 1st subscription by queue handle...
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_Subscription_t *subscription = NULL;
    rc = iett_findQHandleSubscription(pThreadData, Consumers[0]->queueHandle, &subscription);
    if (rc != OK)
    {
        test_log(testLOGLEVEL_ERROR, "ERROR:  iett_findQHandleSubscription() returned %d\n", rc);
        return rc;
    }
    if (subscription->queueHandle != Consumers[0]->queueHandle)
    {
        test_log(testLOGLEVEL_ERROR, "ERROR:  iett_findQHandleSubscription() returned wrong subscription\n");
        return ISMRC_NotFound;
    }
    iett_releaseSubscription(pThreadData, subscription, false);

    // Try and find a subscription using a 'queueHandle' we know is not there (make it run the entire list)
    rc = iett_findQHandleSubscription(pThreadData, (ismQHandle_t)&endTime, NULL);
    if (rc != ISMRC_NotFound)
    {
        test_log(testLOGLEVEL_ERROR, "ERROR:  iett_findQHandleSubscription() returned %d\n", rc);
        return rc;
    }

    /************************************************************************/
    /* Publish the messages                                                 */
    /************************************************************************/
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismEngine_MessageHandle_t  hMessage;
    ismMessageAreaType_t areaTypes[] = { ismMESSAGE_AREA_PAYLOAD };
    char Buffer[]="Simple message";
    void *areas[] = { (void *)Buffer };
    size_t areaLengths[] = { strlen(Buffer) +1 };

    for (i=0; i < pubCount; i++)
    {
        // Pick an account number to which we are going to publish messages
        uint32_t accountNo = (rand() % subCount) + 1;
        sprintf(TopicString, TOPIC_PATTERN, accountNo);

        // Pick the number of messages which we are going to publish
        uint32_t msgCount = (rand() % pubMsgCount) + 1; 

        test_log(testLOGLEVEL_VERBOSE, "Publishing %d messages on Topic %s",
                 msgCount, TopicString);

        for (j=0; j < msgCount; j++)
        {
            rc=ism_engine_createMessage( &header
                                       , 1
                                       , areaTypes
                                       , areaLengths
                                       , areas
                                       , &hMessage);
            if (rc != OK)
            {
                test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_createMessage() returned %d\n", rc);
                return rc;
            }

            rc = ism_engine_putMessageOnDestination(hSession,
                                                    ismDESTINATION_TOPIC,
                                                    TopicString,
                                                    NULL,
                                                    hMessage,
                                                    NULL, 0, NULL);
            if (rc != OK)
            {
                test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_putMessageOnDestination() returned %d\n", rc);
                return rc;
            }
        }
    }

    if (backgroundSubs)
    {
        if (backgroundSubRate == 0)
        {
            backgroundSubRate = ((uint64_t)subCount * 20) / (60 * 60 * 18);
            if (backgroundSubRate == 0) backgroundSubRate=1;
        }

        /********************************************************************/
        /* Start the background thread to create subscriptions             */
        /********************************************************************/ 
        rc=ism_engine_createSession( hClient
                                   , ismENGINE_CREATE_SESSION_OPTION_NONE
                                   , &bgCreateSubContext.hSession
                                   , NULL
                                   , 0 
                                   , NULL);
        if (rc != OK)
        {
            test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_createSession(bgCreateSub) returned %d\n", rc);
            return rc;
        }

        bgCreateSubContext.Durable = Durable;
        bgCreateSubContext.topicCount = subCount;
        strcpy(bgCreateSubContext.TopicPattern, TOPIC_PATTERN);

        test_task_create("bgSubscriber", backgroundSubscriber, &bgCreateSubContext, backgroundSubRate, &bghandle);

        /********************************************************************/
        /* Start the background thread to destroy subscriptions             */
        /********************************************************************/ 
        rc=ism_engine_createSession( hClient
                                   , ismENGINE_CREATE_SESSION_OPTION_NONE
                                   , &bgDestroySubContext.hSession
                                   , NULL
                                   , 0 
                                   , NULL);
        if (rc != OK)
        {
            test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_createSession(bgDestroySub) returned %d\n", rc);
            return rc;
        }

        bgDestroySubContext.Durable = Durable;
        bgDestroySubContext.topicCount = subCount;
        strcpy(bgDestroySubContext.TopicPattern, TOPIC_PATTERN);

        test_task_create("bgUnsubscriber", backgroundUnsubscriber, &bgDestroySubContext, backgroundSubRate, &bghandle);
    }
    else
    {
        test_log(testLOGLEVEL_TESTPROGRESS, "Background subscribe/unsubscribe thread disabled");
    }

    if (backgroundPubs)
    {
        if (backgroundPubRate == 0)
        {
            backgroundPubRate = ((uint64_t)subCount * 50) / (60 * 60 * 18);
            if (backgroundPubRate == 0) backgroundPubRate=1;
        }

        /********************************************************************/
        /* Start the background thread to start sending publications        */
        /********************************************************************/ 
        rc=ism_engine_createSession( hClient
                                   , ismENGINE_CREATE_SESSION_OPTION_NONE
                                   , &bgPubContext.hSession
                                   , NULL
                                   , 0 
                                   , NULL);
        if (rc != OK)
        {
            test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_createSession(bgPubContext) returned %d\n", rc);
            return rc;
        }
    
        bgPubContext.Durable = Durable;
        bgPubContext.topicCount = subCount;
        strcpy(bgPubContext.TopicPattern, TOPIC_PATTERN);

        test_task_create("bgPublisher", backgroundPublisher, &bgPubContext, backgroundPubRate, &bghandle);
    }
    else
    {
        test_log(testLOGLEVEL_TESTPROGRESS, "Background publish thread disabled");
    }

    /* Call the dump routine to dump the entire topic tree */
    if (dumpFile != NULL)
    {
        char dumpResults[strlen(dumpFile)+strlen(".partial")+50];

        strcpy(dumpResults, dumpFile);
        strcat(dumpResults, ".TopicTree");

        startTime = ism_common_currentTimeNanos();
        rc = ism_engine_dumpTopicTree(NULL, dumpDetail, dumpUserData, dumpResults); // Including up to 10 bytes of user data
        endTime = ism_common_currentTimeNanos();

        if (rc != OK)
        {
            test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_dumpTopicTree returned %d\n", rc);
            test_task_stopall(testLOGLEVEL_CHECK);
            return rc;
        }

        diffTime = endTime-startTime;
        diffTimeSecs = ((double)diffTime) / 1000000000;

        test_log(testLOGLEVEL_CHECK, "Time to dump topic tree at detail %d to file '%s' is %.2f secs. (%ldns)",
                 dumpDetail, dumpResults, diffTimeSecs, diffTime);

        strcpy(dumpResults, dumpFile);
        strcat(dumpResults, ".ClientState");

        startTime = ism_common_currentTimeNanos();
        rc = ism_engine_dumpClientState(testClientId, dumpDetail, dumpUserData, dumpResults);
        endTime = ism_common_currentTimeNanos();

        if (rc != OK)
        {
            test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_dumpClientState returned %d\n", rc);
            test_task_stopall(testLOGLEVEL_CHECK);
            return rc;
        }

        diffTime = endTime-startTime;
        diffTimeSecs = ((double)diffTime) / 1000000000;

        test_log(testLOGLEVEL_CHECK, "Time to dump client state for '%s' at detail %d to file '%s' is %.2f secs. (%ldns)",
                 testClientId, dumpDetail, dumpResults, diffTimeSecs, diffTime);

        free(dumpFile);
    }

    /************************************************************************/
    /* Now call the monitoring routine to query the subscriptions            */
    /************************************************************************/
    ismEngine_SubscriptionMonitor_t *results = NULL;
    uint32_t resultCount = 0;
    ism_prop_t *filterProperties = ism_common_newProperties(10);
    ism_field_t f;

    f.type = VT_String;

    if (filterTopic != NULL)
    {
        f.val.s = filterTopic;
        ism_common_setProperty(filterProperties, ismENGINE_MONITOR_FILTER_TOPICSTRING, &f);
    }

    if (filterClientId != NULL)
    {
        f.val.s = filterClientId;
        ism_common_setProperty(filterProperties, ismENGINE_MONITOR_FILTER_CLIENTID, &f);
    }

    if (filterSubName != NULL)
    {
        f.val.s = filterSubName;
        ism_common_setProperty(filterProperties, ismENGINE_MONITOR_FILTER_SUBNAME, &f);
    }

    if (filterSubType != NULL)
    {
        f.val.s = filterSubType;
        ism_common_setProperty(filterProperties, ismENGINE_MONITOR_FILTER_SUBTYPE, &f);
    }

    startTime = ism_common_currentTimeNanos();

    rc = ism_engine_getSubscriptionMonitor( &results
                                          , &resultCount
                                          , requestType
                                          , maxResults
                                          , filterProperties);

    endTime = ism_common_currentTimeNanos();

    ism_common_freeProperties(filterProperties);

    if (rc != OK)
    {
        test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_getSubscriptionMonitor returned %d\n", rc);
        test_task_stopall(testLOGLEVEL_CHECK);
        return rc;
    }

    diffTime = endTime-startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;

    test_log(testLOGLEVEL_CHECK, "Time to query monitoring is %.2f secs. (%ldns)",
             diffTimeSecs, diffTime);

    test_log(testLOGLEVEL_TESTPROGRESS, "Returned %d results for request type %u.\n", resultCount, requestType);
    if (logLevel >= testLOGLEVEL_TESTPROGRESS)
    {
        for(i=0; i<resultCount; i++)
        {
            test_log(testLOGLEVEL_TESTPROGRESS, "SubName: \"%s\" Topic: \"%s\" CId: \"%s\" [%s]"
                     "ProdMsgs: %-6lu ConsMsgs: %-6lu Buff: %-6lu MaxBuff: %-6lu Rej: %-6lu Dis: %-6lu Exp: %-6lu BuffPct: %3.2f%%",
                     results[i].subName ? results[i].subName : "<NONE>",
                     results[i].topicString,
                     results[i].clientId ? results[i].clientId : "<NONE>",
                     results[i].durable ? "Durable" : "Nondurable",
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

    if (filterTopic != NULL) free(filterTopic);
    if (filterClientId != NULL) free(filterClientId);
    if (filterSubName != NULL) free(filterSubName);
    if (filterSubType != NULL) free(filterSubType);

    // Free up the results
    if (resultCount != 0)
    {
        ism_engine_freeSubscriptionMonitor( results );
    }

    test_task_stopall(testLOGLEVEL_CHECK);

    rc=ism_engine_destroySession( hSession
                                , NULL
                                , 0
                                , NULL);
    if (rc != OK)
    {
        test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_destroySession returned %d\n", rc);
        return rc;
    }

    rc=ism_engine_destroyClientState( hClient
                                    , ismENGINE_DESTROY_CLIENT_OPTION_NONE
                                    , NULL
                                    , 0
                                    , NULL);
    if (rc != OK)
    {
        test_log(testLOGLEVEL_ERROR, "ERROR:  ism_engine_destroyClientState() returned %d\n", rc);
        return rc;
    }

    rc = test_engineTerm(true);
    if (rc != OK) return rc;

    test_processTerm(true);

    return 0;
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
    ism_engine_releaseMessage(hMessage);

    return true;
}


/****************************************************************************/
/* Background publish thread                                                */
/****************************************************************************/
void backgroundPublisher(void *context)
{
    uint32_t rc;
    TaskContext_t *pContext = (TaskContext_t *)context;
    char TopicString[256];

    // Pick an account number to which we are going to publish messages
    uint32_t accountNo = (rand() % pContext->topicCount) + 1;
    sprintf(TopicString, pContext->TopicPattern, accountNo);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismEngine_MessageHandle_t  hMessage;
    ismMessageAreaType_t areaTypes[] = { ismMESSAGE_AREA_PAYLOAD };
    char Buffer[]="Simple message";
    void *areas[] = { (void *)Buffer };
    size_t areaLengths[] = { strlen(Buffer) +1 };

    rc=ism_engine_createMessage( &header
                               , 1
                               , areaTypes
                               , areaLengths
                               , areas
                               , &hMessage);
    if (rc == OK)
    {
        test_log(testLOGLEVEL_VERBOSE, "Background publisher sending message on topic %s", TopicString);

        rc = ism_engine_putMessageOnDestination(pContext->hSession,
                                                ismDESTINATION_TOPIC,
                                                TopicString,
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
/* Background unsubscriber thread                                           */
/****************************************************************************/
void backgroundUnsubscriber(void *context)
{
    uint32_t rc;
    DEBUG_ONLY int osrc;
    freeSub_t *pfreeSub;
    TaskContext_t *pContext = (TaskContext_t *)context;

    // Pick an account number which we are going to unsubscribe
    uint32_t accountNo = (rand() % pContext->topicCount) + 1;

    // Close the consumer and delete the subscription
    if (Consumers[accountNo-1] != 0)
    {
        rc = ism_engine_destroyConsumer( Consumers[accountNo-1]
                                       , NULL, 0, NULL);
        TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

        Consumers[accountNo-1] = 0;

        if (pContext->Durable)
        {
            char subName[64];
            sprintf(subName, "subMonitor.%d", accountNo);

            // And destroy the durable subscription
            rc = ism_engine_destroySubscription( hClient
                                               , subName
                                               , hClient
                                               , &accountNo
                                               , sizeof(accountNo)
                                               , backgroundUnsubscriberAsync);
            if (rc == OK)
            {
                // Add it to the list of deleted susbcriptions.
                pfreeSub = (freeSub_t *)malloc(sizeof(freeSub_t));
                pfreeSub->accountNo = accountNo;
   
                osrc = pthread_mutex_lock(&FreeSubListLock);
                TEST_ASSERT(osrc == 0, ("osrc (%d) != 0", osrc));

                pfreeSub->next = FreeSubList;
                FreeSubList = pfreeSub;

                osrc = pthread_mutex_unlock(&FreeSubListLock);
                TEST_ASSERT(osrc == 0, ("osrc (%d) != 0", osrc));
            }
            else
            {
                TEST_ASSERT(rc == ISMRC_AsyncCompletion,
                            ("rc (%d) != ISMRC_AsyncCompletion (%d)", rc, ISMRC_AsyncCompletion));
            }
        }

    }

    return;
}

void backgroundUnsubscriberAsync( int32_t retCode
                                , void *handle
                                , void *pContext)
{
    DEBUG_ONLY int osrc;
    uint32_t accountNo = *((uint32_t *)pContext);
    assert (retCode == OK);
    
    if (retCode == OK)
    {
        // Add it to the list of deleted susbcriptions. 
        freeSub_t *pfreeSub = (freeSub_t *)malloc(sizeof(freeSub_t));
        pfreeSub->accountNo = accountNo;
  
        osrc = pthread_mutex_lock(&FreeSubListLock);
        TEST_ASSERT(osrc == 0, ("osrc (%d) != 0", osrc));

        pfreeSub->next = FreeSubList;
        FreeSubList = pfreeSub;

        osrc = pthread_mutex_unlock(&FreeSubListLock);
        TEST_ASSERT(osrc == 0, ("osrc (%d) != 0", osrc));
    }

    return;
}

/****************************************************************************/
/* Background subscriber thread                                             */
/****************************************************************************/
void backgroundSubscriber(void *context)
{
    uint32_t rc;
    DEBUG_ONLY int osrc;
    char TopicString[256];
    TaskContext_t *pContext = (TaskContext_t *)context;
    freeSub_t *pfreeSub = NULL;

    // Take the account number we are going to recreate the subscription for
    osrc = pthread_mutex_lock(&FreeSubListLock);
    TEST_ASSERT(osrc == 0, ("osrc (%d) != 0", osrc));

    if (FreeSubList)
    {
        pfreeSub = FreeSubList;
        FreeSubList = FreeSubList->next;
    }
    osrc = pthread_mutex_unlock(&FreeSubListLock);
    TEST_ASSERT(osrc == 0, ("osrc (%d) != 0", osrc));

    if (pfreeSub)
    {
        sprintf(TopicString, pContext->TopicPattern, pfreeSub->accountNo);
     
        ismEngine_SubscriptionAttributes_t subAttributes = {0};

        if (pContext->Durable)
        {
            char subName[64];
            sprintf(subName, "subMonitor.%d", pfreeSub->accountNo);

            subAttributes.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                       ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;

            rc=ism_engine_createSubscription( hClient
                                            , subName
                                            , NULL
                                            , ismDESTINATION_TOPIC
                                            , TopicString
                                            , &subAttributes
                                            , NULL // Owning client same as requesting client
                                            , NULL
                                            , 0
                                            , NULL );
            TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

            rc=ism_engine_createConsumer( pContext->hSession
                                        , ismDESTINATION_SUBSCRIPTION
                                        , subName
                                        , NULL
                                        , NULL // Owning client same as session client
                                        , NULL
                                        , 0
                                        , ConsumerCallback
                                        , NULL
                                        , test_getDefaultConsumerOptions(subAttributes.subOptions)
                                        , &Consumers[pfreeSub->accountNo-1]
                                        , NULL
                                        , 0
                                        , NULL);
            TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));
        }
        else
        {
            subAttributes.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;

            rc=ism_engine_createConsumer( pContext->hSession
                                        , ismDESTINATION_TOPIC
                                        , TopicString
                                        , &subAttributes
                                        , NULL // Unused for TOPIC
                                        , NULL
                                        , 0
                                        , ConsumerCallback
                                        , NULL
                                        , ismENGINE_CONSUMER_OPTION_NONE
                                        , &Consumers[pfreeSub->accountNo-1]
                                        , NULL
                                        , 0
                                        , NULL);
            TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));
        }

        free(pfreeSub);
    }

    return;
}

/****************************************************************************/
/* ProcessArgs                                                              */
/****************************************************************************/
int ProcessArgs( int argc
               , char **argv
               , uint64_t *pSeedVal
               , bool *pDurable
               , uint32_t *pSubCount
               , uint32_t *pPubCount
               , uint32_t *pPubMsgCount
               , uint32_t *pbgSubRate
               , uint32_t *pbgPubRate
               , bool *pBackgroundPubs
               , bool *pBackgroundSubs
               , uint32_t *pRequestType
               , uint32_t *pMaxResults
               , char **pFilterTopic
               , char **pFilterClientId
               , char **pFilterSubName
               , char **pFilterSubType
               , char **pDumpFile
               , int32_t *pDumpDetail
               , int32_t *pDumpUserData
               , bool *pRandomSubOpts
               , bool *pSingleTopic )
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { "durable", 0, NULL, 'd' }, 
        { "subCount", 1, NULL, 's' }, 
        { "pubCount", 1, NULL, 'p' }, 
        { "pubMsgCount", 1, NULL, 'm' },
        { "seed", 1, NULL, 0 }, 
        { "bgSubRate", 1, NULL, 0 }, 
        { "bgPubRate", 1, NULL, 0 }, 
        { "noBackgroundPubs", 0, NULL, 0 }, 
        { "noBackgroundSubs", 0, NULL, 0 }, 
        { "requestType", 1, NULL, 't' },
        { "maxResults", 1, NULL, 'r' },
        { "filterTopic", 1, NULL, 'T' },
        { "filterClientId", 1, NULL, 'C' },
        { "filterSubName", 1, NULL, 'S' },
        { "filterSubType", 1, NULL, 'D' },
        { "dumpFile", 1, NULL, 0 },
        { "dumpDetail", 1, NULL, 0 },
        { "dumpUserData", 1, NULL, 0 },
        { "randomSubOpts", 0, NULL, 0 },
        { "singleTopic", 0, NULL, 0 },
        { NULL, 1, NULL, 0 } };
    int long_index;

    *pSeedVal = 0;
    *pSubCount = 200000;
    *pPubCount = 200;
    *pPubMsgCount = 100;
    *pDurable = false;
    *pbgSubRate = 0;
    *pbgPubRate = 0;
    *pBackgroundPubs = true;
    *pBackgroundSubs = true;
    *pRequestType = ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS;
    *pMaxResults = 50;
    *pFilterTopic = NULL;
    *pFilterClientId = NULL;
    *pFilterSubName = NULL;
    *pFilterSubType = NULL;
    *pDumpFile = NULL;
    *pDumpDetail = 5;
    *pDumpUserData = 0;
    *pRandomSubOpts = false;
    *pSingleTopic = false;

    while ((opt = getopt_long(argc, argv, ":ds:p:m:v:t:r:T:C:S:D:", long_options, &long_index)) != -1)
    {
        switch (opt)
        {
            case 0:
                if (long_index == 4)
                {
                    *pSeedVal = atol(optarg);
                }
                else if (long_index == 5)
                {
                    *pbgSubRate = atoi(optarg);
                }
                else if (long_index == 6)
                {
                    *pbgPubRate = atoi(optarg);
                }
                else if (long_index == 7)
                {
                    *pBackgroundPubs = false;
                }
                else if (long_index == 8)
                {
                    *pBackgroundSubs = false;
                }
                else if (long_index == 15)
                {
                    *pDumpFile = strdup(optarg);
                }
                else if (long_index == 16)
                {
                    *pDumpDetail = atoi(optarg);
                }
                else if (long_index == 17)
                {
                    *pDumpUserData = atoi(optarg);
                }
                else if (long_index == 18)
                {
                    *pRandomSubOpts = true;
                }
                else if (long_index == 19)
                {
                    *pSingleTopic = true;
                }
                break;
            case 'd':
                *pDurable=true;
                break;
            case 's':
                *pSubCount = atoi(optarg);
                break;
            case 'p':
                *pPubCount = atoi(optarg);
                break;
            case 'm':
                *pPubMsgCount = atoi(optarg);
                break;
            case 'v':
               logLevel = atoi(optarg);
               if (logLevel > testLOGLEVEL_VERBOSE)
                   logLevel = testLOGLEVEL_VERBOSE;
               break;
            case 't':
                *pRequestType = atoi(optarg);
                break;
            case 'r':
                *pMaxResults = atoi(optarg);
                break;
            case 'T':
                *pFilterTopic = strdup(optarg);
                break;
            case 'C':
                *pFilterClientId = strdup(optarg);
                break;
            case 'S':
                *pFilterSubName = strdup(optarg);
                break;
            case 'D':
                *pFilterSubType = strdup(optarg);
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
        fprintf(stderr, "Usage: %s [-d] [-s subCount] [-p pubCount] [-m pubMsgCount] [-v] [-t requestType] [-r maxResults]\n", argv[0]);
        fprintf(stderr, "       -v - logLevel 0-5 [2]\n");
        fprintf(stderr, "       -d - durable subscriptions [false]\n");
        fprintf(stderr, "       -s - number of subscriptions to create [200,000]\n");
        fprintf(stderr, "       -p - number of subscriptions to publish message s to [200]\n");
        fprintf(stderr, "       -m - maximum number of messages to publish to selected subscription [100]\n");
        fprintf(stderr, "       -t - type of request 0-%d (see defn of ismEngineMonitorType_t) [%d]\n",
                ismENGINE_MONITOR_MAX-1, ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS);
        fprintf(stderr, "       -r - specify the maximum number of results to return [50]\n");
        fprintf(stderr, "       --seed - Random seed value\n");
        fprintf(stderr, "       --bgSubRate - rate (per/second) of background sub/unsub [%d]\n", (1000000 * 20) / (18 * 60 * 60));
        fprintf(stderr, "       --bgPubRate - rate (per/second) of background sub/unsub [%d]\n", (1000000 * 50) / (18 * 60 * 60));
        fprintf(stderr, "       --noBackgroundPubs - disable background publish thread\n");
        fprintf(stderr, "       --noBackgroundSubs - disable background subscribe/unsubscribe threads\n");
        fprintf(stderr, "       --dumpFile - write the topic tree to the specified dump file\n");
        fprintf(stderr, "       --dumpDetail - specify a detail level for the dump file (def: 5)\n");
        fprintf(stderr, "       --dumpUserData - specify the amount of user data to dump (def: 0)\n");
        fprintf(stderr, "       --randomSubOpts - randomize subscription optisons\n");
        fprintf(stderr, "       --singleTopic - Create all subscriptions on a single topic\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Various filters can be applied to limit the results with the following additional switches:\n");
        fprintf(stderr, "       -T - Filter by wildcarded topic string [none]\n");
        fprintf(stderr, "       -C - Filter by wildcarded client Id [none]\n");
        fprintf(stderr, "       -S - Filter by wildcarded subscription name [none]\n");
        fprintf(stderr, "       -D - Filter by subscription type (durability) [none]\n");
        fprintf(stderr, "            Valid values 'All', 'Durable' or 'Nondurable'\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "The test program creates a large set of subscriptions (-s), each\n");
        fprintf(stderr, "to it's open topic. It then publishes a number of messages (-m) to\n");
        fprintf(stderr, "a number of the subscriptions (-p). It then calls the engine to\n");
        fprintf(stderr, "query a set (-r) of the subscriptions and reports how long the query\n");
        fprintf(stderr, "operation took to execute, optionally displaying the results (-v).\n");
    }

    return usage;
}
