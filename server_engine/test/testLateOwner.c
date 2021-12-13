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
/* Module Name: testLateOwner                                       */
/*                                                                  */
/* Description: This test attempts to create a subscription that    */
/*              refers to a message in a much earlier generation.   */
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
#include "topicTree.h"
#include "clientState.h"
#include "engineStore.h"

#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_log.h"
#include "test_utils_task.h"
#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

/********************************************************************/
/* Function prototypes                                              */
/********************************************************************/
int ProcessArgs( int argc
               , char **argv
               , int *phase
               , char **padminDir );

/********************************************************************/
/* Global data                                                      */
/********************************************************************/
static uint32_t logLevel = testLOGLEVEL_CHECK;

#define testClientId "LateOwners Test"

#define createPhase          0 // Creation phase
#define destroyWildPhase     3 // Destroy WILDCARD_SUB
#define destroyStandardPhase 5 // Destroy STANDARD_SUB
#define createNewSubPhase    7 // Create a new subscriber
#define endPhase             9 // End phase

typedef struct tag_retainedCbContext_t
{
    ismEngine_SessionHandle_t hSession;
    int32_t received;
    size_t expectedPayloadLength;
} retainedCbContext_t;

bool retainedDeliveryCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
                              void *                          pContext,
                              ismEngine_DelivererContext_t *  _delivererContext )
{
    retainedCbContext_t *context = *((retainedCbContext_t **)pContext);

    ism_engine_releaseMessage(hMessage);

    for(int32_t i=0; i<areaCount; i++)
    {
        size_t expectedLength = 0;

        if (areaTypes[i] == ismMESSAGE_AREA_PAYLOAD)
        {
            expectedLength = context->expectedPayloadLength;

            TEST_ASSERT(areaLengths[i] == expectedLength,
                        ("Area %d of type %d has length %lu, not expected %lu",
                         i, areaTypes[i], areaLengths[i], expectedLength));
        }
    }

    if (context->received == 0)
    {
        TEST_ASSERT((pMsgDetails->Flags & ismMESSAGE_FLAGS_RETAINED) != 0,
                    ("Message %d not flagged as retained", context->received));
    }
    else
    {
        TEST_ASSERT((pMsgDetails->Flags & ismMESSAGE_FLAGS_RETAINED) == 0,
                    ("Message %d unexpectedly flagged as retained", context->received));
    }

    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        uint32_t rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                        NULL,
                                                        hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                        NULL, 0, NULL);

        TEST_ASSERT((rc == OK || rc == ISMRC_AsyncCompletion), ("%d != %d", rc, OK));
    }

    __sync_fetch_and_add(&context->received, 1);

    return true; // more messages, please.
}


/********************************************************************/
/* MAIN                                                             */
/********************************************************************/
int main(int argc, char *argv[])
{
    int trclvl = 4;
    int rc;
    int phase;
    ismStore_GenId_t retainedMsgGenId;
    ismStore_GenId_t bigMsgGenId;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ClientStateHandle_t hClient;
    char *adminDir = NULL;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    /************************************************************************/
    /* Parse the command line arguments                                     */
    /************************************************************************/
    rc = ProcessArgs( argc
                    , argv
                    , &phase
                    , &adminDir );

    if (rc != OK)
    {
        return rc;
    }

    test_setLogLevel(logLevel);

    rc = test_processInit(trclvl, adminDir);
    if (rc != OK) return rc;
    
    char localAdminDir[1024];
    if (adminDir == NULL && test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir)) == true)
    {
        adminDir = localAdminDir;
    }

    rc =  test_engineInit(phase == createPhase,
                          true,
                          ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                          false, /*recovery should complete ok*/
                          ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                          50);  // 50mb Store
    if (rc != OK) return rc;

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 100000000;

    rc = test_createClientAndSession(testClientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient,
                                     &hSession,
                                     false);
    TEST_ASSERT(rc == OK || rc == ISMRC_ResumedClientState , ("%d != %d || %d", rc, OK, ISMRC_ResumedClientState));

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

    if (phase == createPhase)
    {
        rc = sync_ism_engine_createSubscription( hClient
                                               , "WILDCARD_SUB"
                                               , NULL
                                               , ismDESTINATION_TOPIC
                                               , "NON_RETAINED_TOPIC/#"
                                               , &subAttrs
                                               , NULL ); // Owning client same as requesting client
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

        // Publish a persistent retained message on topic 'RETAINED_TOPIC'
        ismEngine_MessageHandle_t hRetainedMsg;
        rc = test_createMessage(50,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, "RETAINED_TOPIC",
                                &hRetainedMsg, NULL);
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

        rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                     ismDESTINATION_TOPIC,
                                                     "RETAINED_TOPIC",
                                                     NULL,
                                                     hRetainedMsg);
        TEST_ASSERT(rc == ISMRC_NoMatchingDestinations, ("%d != %d", rc, ISMRC_NoMatchingDestinations));

        ism_store_getGenIdOfHandle((((ismEngine_Message_t *)hRetainedMsg)->StoreMsg.parts.hStoreMsg),
                                   &retainedMsgGenId);

        ismEngine_MessageHandle_t hOldWillMsg;
        ismStore_Handle_t hOldWillStoreMsg = ismSTORE_NULL_HANDLE;
        ismStore_GenId_t oldWillMsgGenId;

        rc = test_createMessage(50,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, "NON_RETAINED_TOPIC/WILL_MSGS",
                                &hOldWillMsg, NULL);
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

        rc = iest_storeMessage(pThreadData, hOldWillMsg, 1, iestStoreMessageOptions_NONE, &hOldWillStoreMsg);
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
        TEST_ASSERT(hOldWillStoreMsg != ismSTORE_NULL_HANDLE, ("%lu == %lu", hOldWillStoreMsg, ismSTORE_NULL_HANDLE));

        ism_store_getGenIdOfHandle(hOldWillStoreMsg, &oldWillMsgGenId);

        // Publish lots of big persistent messages on topic 'NON_RETAINED_TOPIC'
        for(int32_t i=0; i<150; i++)
        {
            ismEngine_MessageHandle_t hBigMsg;

            rc = test_createMessage(1000000, // 1MB
                                    ismMESSAGE_PERSISTENCE_PERSISTENT,
                                    ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                    ismMESSAGE_FLAGS_NONE,
                                    0,
                                    ismDESTINATION_TOPIC, "NON_RETAINED_TOPIC",
                                    &hBigMsg, NULL);
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

#ifdef NDEBUG // Test non-standard use of ism_engine_putMessageInternalOnDestination
            if (i%4 == 0)
            {
                rc = sync_ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                                "NON_RETAINED_TOPIC",
                                                                hBigMsg);
            }
            else
#endif
            {
                rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                        ismDESTINATION_TOPIC,
                                                        "NON_RETAINED_TOPIC",
                                                        NULL,
                                                        hBigMsg);
            }
            TEST_ASSERT(rc == OK, ("rc was  %d", rc));

            ism_store_getGenIdOfHandle((((ismEngine_Message_t *)hBigMsg)->StoreMsg.parts.hStoreMsg),
                                       &bigMsgGenId);
        }

        // Lots of generations used?
        TEST_ASSERT(bigMsgGenId >= retainedMsgGenId+10, ("%hu < %hu", bigMsgGenId, retainedMsgGenId+10));

        // Do something highly irregular to set a will message (to show will msgs can be in earlier
        // generations)
        rc = iecs_updateClientPropsRecord(pThreadData,
                                          hClient,
                                          "NON_RETAINED_TOPIC/WILL_MSGS",
                                          hOldWillStoreMsg,
                                          500,
                                          0);
        TEST_ASSERT(rc == OK, ("rc=%d unexpected from iecs_updateClientPropsRecord\n"));

        ismStore_GenId_t newClientCPRGenId;
        ism_store_getGenIdOfHandle(hClient->hStoreCPR, &newClientCPRGenId);
        TEST_ASSERT(newClientCPRGenId >= oldWillMsgGenId+10, ("%hu < %hu", newClientCPRGenId, oldWillMsgGenId+10));

        // Subscribe on topic 'RETAINED_TOPIC'
        rc = sync_ism_engine_createSubscription( hClient
                                               , "STANDARD_SUB"
                                               , NULL
                                               , ismDESTINATION_TOPIC
                                               , "RETAINED_TOPIC"
                                               , &subAttrs
                                               , NULL ); // Owning client same as requesting client
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

        // Publish a new persistent retained message on topic 'RETAINED_TOPIC'
        rc = test_createMessage(50,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, "RETAINED_TOPIC",
                                &hRetainedMsg, NULL);
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

        rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                     ismDESTINATION_TOPIC,
                                                     "RETAINED_TOPIC",
                                                     NULL,
                                                     hRetainedMsg);
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
    }
    else
    {
        uint64_t expectBufferedMsgs;
        ismEngine_Subscription_t *subscription = NULL;
        ismEngine_QueueStatistics_t stats;

        rc = iett_findClientSubscription(pThreadData,
                                         testClientId,
                                         iett_generateClientIdHash(testClientId),
                                         "WILDCARD_SUB",
                                         iett_generateSubNameHash("WILDCARD_SUB"),
                                         &subscription);

        // If we should have destroyed WILDCARD_SUB, check it is destroyed
        if (phase > destroyWildPhase)
        {
            TEST_ASSERT(rc == ISMRC_NotFound, ("%d != %d", rc, ISMRC_NotFound));
        }
        else
        {
            // 150 published originally, plus the 1 will msg published at 1st restart.
            expectBufferedMsgs = 151;

            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
            TEST_ASSERT(subscription != NULL, ("subscription is NULL"));

            ieq_getStats(pThreadData, subscription->queueHandle, &stats);
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
            TEST_ASSERT(stats.BufferedMsgs == expectBufferedMsgs,
                        ("%d != %d", stats.BufferedMsgs, expectBufferedMsgs));

            rc = iett_releaseSubscription(pThreadData, subscription, false);
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
        }

        rc = iett_findClientSubscription(pThreadData,
                                         testClientId,
                                         iett_generateClientIdHash(testClientId),
                                         "STANDARD_SUB",
                                         iett_generateSubNameHash("STANDARD_SUB"),
                                         &subscription);

        // If we should have destroyed STANDARD_SUB, check it is destroyed
        if (phase > destroyStandardPhase)
        {
            TEST_ASSERT(rc == ISMRC_NotFound, ("%d != %d", rc, ISMRC_NotFound));
        }
        else
        {
            expectBufferedMsgs = 2;
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
            TEST_ASSERT(subscription != NULL, ("subscription is NULL"));

            ieq_getStats(pThreadData, subscription->queueHandle, &stats);
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
            TEST_ASSERT(stats.BufferedMsgs == expectBufferedMsgs,
                        ("%d != %d", stats.BufferedMsgs, expectBufferedMsgs));

            rc = iett_releaseSubscription(pThreadData, subscription, false);
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
        }

        // Destroy WILDCARD_SUB
        if (phase == destroyWildPhase)
        {
            rc = ism_engine_destroySubscription( hClient
                                               , "WILDCARD_SUB"
                                               , hClient
                                               , NULL
                                               , 0
                                               , NULL );
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
        }

        // Destroy STANDARD_SUB
        if (phase == destroyStandardPhase)
        {
            // Before destroying it, let's consume the messages from it and check
            // they are as expected.
            retainedCbContext_t retainedContext = {0};
            retainedCbContext_t *retainedCb = &retainedContext;
            ismEngine_ConsumerHandle_t hConsumer = NULL;

            retainedContext.hSession = hSession;
            retainedContext.expectedPayloadLength = 50;

            rc = ism_engine_createConsumer(hSession,
                                           ismDESTINATION_SUBSCRIPTION,
                                           "STANDARD_SUB",
                                           &subAttrs,
                                           NULL, // Owning client same as session client
                                           &retainedCb,
                                           sizeof(retainedCbContext_t *),
                                           retainedDeliveryCallback,
                                           NULL,
                                           test_getDefaultConsumerOptions(subAttrs.subOptions),
                                           &hConsumer,
                                           NULL,
                                           0,
                                           NULL);
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
            TEST_ASSERT(hConsumer != NULL, ("hConsumer is NULL"));

            rc = ism_engine_startMessageDelivery(hSession,
                                                 ismENGINE_START_DELIVERY_OPTION_NONE,
                                                 NULL, 0, NULL);
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

            while(retainedContext.received < 2)
            {
                sleep(1);
            }

            rc = sync_ism_engine_destroyConsumer(hConsumer);
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

            // Now destroy the subscription
            rc = ism_engine_destroySubscription( hClient
                                               , "STANDARD_SUB"
                                               , hClient
                                               , NULL
                                               , 0
                                               , NULL );
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
        }

        // Create a new standard subscription
        if (phase == createNewSubPhase)
        {
            rc = sync_ism_engine_createSubscription( hClient
                                                   , "NEW_STANDARD_SUB"
                                                   , NULL
                                                   , ismDESTINATION_TOPIC
                                                   , "+"
                                                   , &subAttrs
                                                   , NULL ); // Owning client same as requesting client
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
        }

        rc = iett_findClientSubscription(pThreadData,
                                         testClientId,
                                         iett_generateClientIdHash(testClientId),
                                         "NEW_STANDARD_SUB",
                                         iett_generateSubNameHash("NEW_STANDARD_SUB"),
                                         &subscription);

        // If we should have created NEW_STANDARD_SUB, check it exists and has a
        // single message
        if (phase < createNewSubPhase)
        {
            TEST_ASSERT(rc == ISMRC_NotFound, ("%d != %d", rc, ISMRC_NotFound));
        }
        else
        {
            expectBufferedMsgs = 1;
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
            TEST_ASSERT(subscription != NULL, ("subscription is NULL"));

            ieq_getStats(pThreadData, subscription->queueHandle, &stats);
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
            TEST_ASSERT(stats.BufferedMsgs == expectBufferedMsgs,
                        ("%d != %d", stats.BufferedMsgs, expectBufferedMsgs));

            rc = iett_releaseSubscription(pThreadData, subscription, false);
            TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
        }
    }

    test_incrementActionsRemaining(pActionsRemaining, 2);
    rc = ism_engine_destroySession( hSession
                                  , &pActionsRemaining
                                  , sizeof(pActionsRemaining)
                                  , test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("%d != %d", rc, ISMRC_AsyncCompletion));

    rc = ism_engine_destroyClientState( hClient
                                      , ismENGINE_DESTROY_CLIENT_OPTION_NONE
                                      , &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("%d != %d", rc, ISMRC_AsyncCompletion));

    // Purposefully not calling test_waitForRemainingActions(pActionsRemaining);

    if (phase == endPhase)
    {
        rc = test_engineTerm(true);
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
        test_processTerm(false);
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
        test_removeAdminDir(adminDir);
    }
    else
    {
        rc = test_engineTerm(false);
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
        test_processTerm(false);
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

        phase++;

        fprintf(stdout, "== Restarting for phase %d\n", phase);

        // Re-execute ourselves for the next phase
        int32_t newargc = 0;

        char *newargv[argc+8];

        newargv[newargc++] = argv[0];

        for(int32_t i=newargc; i<argc; i++)
        {
            if ((strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "-a") == 0))
            {
                i++;
            }
            else
            {
                newargv[newargc++] = argv[i];
            }
        }

        char phaseString[50];
        sprintf(phaseString, "%d", phase);

        newargv[newargc++] = "-p";
        newargv[newargc++] = phaseString;
        newargv[newargc++] = "-a";
        newargv[newargc++] = adminDir;
        newargv[newargc] = NULL;

        printf("== Command: ");
        for(int32_t i=0; i<newargc; i++)
        {
            printf("%s ", newargv[i]);
        }
        printf("\n");

        rc = test_fork_and_execvp(newargv[0], newargv);
    }

    return rc;
}

/****************************************************************************/
/* ProcessArgs                                                              */
/****************************************************************************/
int ProcessArgs( int argc
               , char **argv
               , int *phase
               , char **padminDir )
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { NULL, 1, NULL, 0 } };
    int long_index;

    *phase = 0;
    *padminDir = NULL;

    while ((opt = getopt_long(argc, argv, "v:p:a:", long_options, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'p':
                *phase = atoi(optarg);
                break;
            case 'v':
               logLevel = atoi(optarg);
               if (logLevel > testLOGLEVEL_VERBOSE)
                   logLevel = testLOGLEVEL_VERBOSE;
               break;
            case 'a':
                *padminDir = optarg;
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
        fprintf(stderr, "Usage: %s [-v verbose]\n", argv[0]);
        fprintf(stderr, "       -v - logLevel 0-5 [2]\n");
        fprintf(stderr, "       -a - Admin directory to use\n");
        fprintf(stderr, "\n");
    }

    return usage;
}
