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
/* Module Name: testClientStateRecovery.c                            */
/*                                                                   */
/* Description: Test program which validates the recovery of client  */
/*              state and associated data from the Store.            */
/*                                                                   */
/*********************************************************************/
#include <stdio.h>
#include <assert.h>
#include <getopt.h>

#include "engineInternal.h"
#include "transaction.h"
#include "policyInfo.h"
#include "clientState.h"
#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

/*********************************************************************/
/* Global data                                                       */
/*********************************************************************/
static ismEngine_ClientStateHandle_t hClientD = NULL;
static ismEngine_SessionHandle_t hSessionD = NULL;
static ismEngine_TransactionHandle_t hTran = NULL;
static ismEngine_UnreleasedHandle_t hUnrel = NULL;
static ismEngine_UnreleasedHandle_t hUnrel2 = NULL;
static ismEngine_ClientStateHandle_t hClientND = NULL;
static ismEngine_SessionHandle_t hSessionND = NULL;
static ismEngine_ClientStateHandle_t hClientD2 = NULL;
static int unreleasedIdCount = 0;

// 0   Silent
// 1   Test Name only
// 2   + Test Description
// 3   + Validation checks
// 4   + Test Progress
// 5   + Verbose Test Progress
static uint32_t verboseLevel = 5;   /* 0-9 */


/*********************************************************************/
/* Name:        verbose                                              */
/* Description: Debugging printf routine                             */
/* Parameters:                                                       */
/*     level               - Debug level of the text provided        */
/*     format              - Format string                           */
/*     ...                 - Arguments to format string              */
/*********************************************************************/
static void verbose(int level, char *format, ...)
{
    static char indent[]="--------------------";
    va_list ap;

    if (verboseLevel >= level)
    {
        va_start(ap, format);
        if (format[0] != '\0')
        {
            printf("%.*s ", level+1, indent);
            vprintf(format, ap);
        }
        printf("\n");
        va_end(ap);
    }
}

int32_t parseArgs(int argc,
                  char *argv[],
                  uint32_t *pphase,
                  bool *pruninline,
                  uint32_t *pverboseLevel,
                  int *ptrclvl,
                  char **padminDir)
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { "phase", 0, NULL, 'p' },
        { "verbose", 1, NULL, 'v' },
        { "adminDir", 1, NULL, 'a' },
        { "runinline", 0, NULL, 'i' },
        { NULL, 1, NULL, 0 } };
    int long_index;

    *pphase = 1;
    *pruninline = false;
    *pverboseLevel = 3;
    *ptrclvl = 0;
    *padminDir = NULL;

    while ((opt = getopt_long(argc, argv, "p:v:a:i0123456789", long_options, &long_index)) != -1)
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
                *pphase = atoi(optarg);
                break;
            case 'v':
               *pverboseLevel = atoi(optarg);
               break;
            case 'a':
                *padminDir = optarg;
                break;
            case 'i':
                *pruninline = true;
                break;
            case '?':
            default:
               usage=1;
               break;
        }
    }

    if (*pverboseLevel > 9) *pverboseLevel=9;

    if (usage)
    {
        fprintf(stderr, "Usage: %s [-r] [-v 0-9] [-0...-9]\n", argv[0]);
        fprintf(stderr, "       -p (--phase)     - Phase\n");
        fprintf(stderr, "       -v (--verbose)   - Verbosity\n");
        fprintf(stderr, "       -a (--adminDir)  - Admin directory to use\n");
        fprintf(stderr, "       -i (--runinline) - Run the test inline (no restart)");
        fprintf(stderr, "       -0 .. -9         - Trace level\n");
    }

   return usage;
}

void unreleasedCB(
    uint32_t                        deliveryId,
    ismEngine_UnreleasedHandle_t    hUnrelHdl,
    void *                          pContext)
{
    verbose(2, "Unreleased %u.", deliveryId);
    unreleasedIdCount++;
}

bool messageDeliveryPreRestartCB(
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
        void *                          pConsumerContext)
{
    verbose(2, "Message order id %lu delivered, delivery id %lu. \"%s\".",
            pMsgDetails->OrderId, deliveryId, pAreaData[1]);

    int32_t rc = ism_engine_confirmMessageDelivery(hSessionD,
                                                   NULL,
                                                   hDelivery,
                                                   ismENGINE_CONFIRM_OPTION_RECEIVED,
                                                   NULL,
                                                   0,
                                                   NULL);

    TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("Failed to ack msg. rc = %d", rc));

    return true;
}

bool messageDeliveryCB(
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
        void *                          pConsumerContext)
{
    verbose(2, "Message order id %lu delivered, delivery id %lu. \"%s\".",
            pMsgDetails->OrderId, deliveryId, pAreaData[1]);

    TEST_ASSERT(deliveryId != 0, ("Failed to recover MDR."));

    int32_t rc = ism_engine_confirmMessageDelivery(hSessionD,
                                                   NULL,
                                                   hDelivery,
                                                   ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                   NULL,
                                                   0,
                                                   NULL);
    TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("Failed to ack msg. rc = %d", rc));

    ism_engine_releaseMessage(hMessage);

    return true;
}

void durableSubCB(
        ismEngine_SubscriptionHandle_t subHandle,
        const char *pSubName,
        const char *pTopicString,
        void *properties,
        size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttributes,
        uint32_t consumerCount,
        void *pContext)
{
    verbose(2, "Durable sub '%s'.", pSubName);

    ismEngine_ConsumerHandle_t hConsumer = NULL;

    int32_t rc = ism_engine_createConsumer(hSessionD,
                                           ismDESTINATION_SUBSCRIPTION,
                                           pSubName,
                                           pSubAttributes,
                                           NULL, // Owning client same as session client
                                           NULL,
                                           0,
                                           &messageDeliveryCB,
                                           NULL,
                                           test_getDefaultConsumerOptions(pSubAttributes->subOptions),
                                           &hConsumer,
                                           NULL,
                                           0,
                                           NULL);
    TEST_ASSERT(rc == OK, ("Failed to create consumer rc = %d", rc));
}

uint32_t phase = 1;

/*********************************************************************/
/* Use reduced memory recovery mode for phase 2, otherwise not       */
/*********************************************************************/
int set_ReducedMemoryRecoveryMode(void)
{
    ism_field_t f;

    f.type = VT_Boolean;
    f.val.i = phase == 2 ? 1 : 0;

    ism_common_setProperty(ism_common_getConfigProperties(), ismENGINE_CFGPROP_REDUCED_MEMORY_RECOVERY_MODE, &f);

    return OK;
}

/*********************************************************************/
/* NAME:        testClientStateRecovery                              */
/* DESCRIPTION: This program tests the recovery of client state and  */
/*              associated data from the Store.                      */
/*                                                                   */
/*              Each test runs in 2 phases                           */
/*              Phase 1. The client-state information is loaded into */
/*                       the Store.                                  */
/*              Phase 2. At the end of Phase 1, the program re-execs */
/*                       itself and recovers from the Store.         */
/* USAGE:       test_clientStateRecovery [-r] [-v 0-9] [-0...-9]     */
/*                  -r : When -r is not specified the program runs   */
/*                       Phase 1 and loads the data into the Store.  */
/*                       When -r is specified, the program runs      */
/*                       Phase 2 and recovers the data and verifies  */
/*                       everything has been loaded correctly.       */
/*********************************************************************/
int main(int argc, char *argv[])
{
    int trclvl = 0;
    int rc = 0;
    char *newargv[12];
    bool runinline = false;
    char *adminDir = NULL;

    /*************************************************************/
    /* Parse arguments                                           */
    /*************************************************************/
    rc = parseArgs(argc,
                   argv,
                   &phase,
                   &runinline,
                   &verboseLevel,
                   &trclvl,
                   &adminDir);
    if (rc != 0)
    {
        goto mod_exit;
    }


    /*****************************************************************/
    /* Process Initialise                                            */
    /*****************************************************************/
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

    char nextPhase[10];

    sprintf(nextPhase, "%d", phase+1);

    /*************************************************************/
    /* Prepare the restart command                               */
    /*************************************************************/
    newargv[0] = argv[0];
    newargv[1] = "-p";
    newargv[2] = nextPhase;
    newargv[3] = "-a";
    newargv[4] = adminDir;
    newargv[5] = NULL;

    if (phase == 1)
    {
        /*************************************************************/
        /* Start up the Engine - Cold Start                          */
        /*************************************************************/
        verbose(2, "Starting Engine - cold start");
        rc = test_engineInit_DEFAULT;
        if (rc != 0)
        {
            goto mod_exit;
        }

        // Create-destroy-create so that we exercise the zombie client-state code and the restart later
        rc = sync_ism_engine_createClientState("Recovery1",
                                               testDEFAULT_PROTOCOL_ID,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &hClientD);
        TEST_ASSERT(rc == OK, ("Failed to create client-state. rc = %d", rc));

        rc = sync_ism_engine_destroyClientState(hClientD,
                                                ismENGINE_DESTROY_CLIENT_OPTION_NONE);
        TEST_ASSERT(rc == OK, ("Destroy client-state failed. rc = %d", rc));

        rc = sync_ism_engine_createClientState("Recovery1",
                                               testDEFAULT_PROTOCOL_ID,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &hClientD);
        TEST_ASSERT(rc == ISMRC_ResumedClientState, ("Failed to create client-state. rc = %d", rc));

        rc = ism_engine_createSession(hClientD,
                                      ismENGINE_CREATE_SESSION_OPTION_NONE,
                                      &hSessionD,
                                      NULL,
                                      0,
                                      NULL);
        TEST_ASSERT(rc == OK, ("Failed to create session. rc = %d", rc));

        rc = ism_engine_startMessageDelivery(hSessionD,
                                             ismENGINE_START_DELIVERY_OPTION_NONE,
                                             NULL,
                                             0,
                                             NULL);
        TEST_ASSERT(rc == OK, ("Failed to start message delivery. rc = %d", rc));

        rc = ism_engine_createClientState("Recovery2",
                                          testDEFAULT_PROTOCOL_ID,
                                          ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &hClientND,
                                          NULL,
                                          0,
                                          NULL);
        TEST_ASSERT(rc == OK, ("Failed to create client-state. rc = %d", rc));

        rc = ism_engine_createSession(hClientND,
                                      ismENGINE_CREATE_SESSION_OPTION_NONE,
                                      &hSessionND,
                                      NULL,
                                      0,
                                      NULL);
        TEST_ASSERT(rc == OK, ("Failed to create session. rc = %d", rc));

        rc = sync_ism_engine_createClientState("Recovery3",
                                               testDEFAULT_PROTOCOL_ID,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &hClientD2);
        TEST_ASSERT(rc == OK, ("Failed to create client-state. rc = %d", rc));

        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        header.Reliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;
        header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
        ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
        size_t areaLengths[2] = {0, 0};
        void *areaData[2] = {NULL, NULL};
        ismEngine_MessageHandle_t hMessage;

        areaData[1] = "Will message D";
        areaLengths[1] = strlen(areaData[1]) + 1;

        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areaData,
                                      &hMessage);

        // Force a non-zero TTL for the will message (to test that code)
        iepiPolicyInfo_t *defaultPolicy = iepi_getDefaultPolicyInfo(false);
        defaultPolicy->maxMessageTimeToLive = 20;

        rc = ism_engine_setWillMessage(hClientD,
                                       ismDESTINATION_TOPIC,
                                       "Topic/Will",
                                       hMessage,
                                       0,
                                       iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                       NULL, 0, NULL);
        TEST_ASSERT(rc == OK, ("Failed to set will message. rc = %d", rc));

        // Add a will message with a delay to client D2
        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areaData,
                                      &hMessage);

        rc = ism_engine_setWillMessage(hClientD2,
                                       ismDESTINATION_TOPIC,
                                       "Topic/WillDelay",
                                       hMessage,
                                       60,
                                       60,
                                       NULL, 0, NULL);
        TEST_ASSERT(rc == OK, ("Failed to set will message. rc = %d", rc));

        areaData[1] = "Will message ND";
        areaLengths[1] = strlen(areaData[1]) + 1;

        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areaData,
                                      &hMessage);

        rc = ism_engine_setWillMessage(hClientND,
                                       ismDESTINATION_TOPIC,
                                       "Topic/Will",
                                       hMessage,
                                       0,
                                       iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                       NULL, 0, NULL);
        TEST_ASSERT(rc == OK, ("Failed to set will message. rc = %d", rc));

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                        ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

        rc = sync_ism_engine_createSubscription(hClientD,
                                                "Sub1",
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                "Topic/+",
                                                &subAttrs,
                                                NULL); // Owning client same as requesting client
        TEST_ASSERT(rc == OK, ("Failed to create durable sub. rc = %d", rc));

        areaData[1] = "Live message 2";
        areaLengths[1] = strlen(areaData[1]) + 1;

        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areaData,
                                      &hMessage);

        rc = ism_engine_putMessageOnDestination(hSessionD,
                                                ismDESTINATION_TOPIC,
                                                "Topic/Live",
                                                NULL,
                                                hMessage,
                                                NULL,
                                                0,
                                                NULL);

        rc = ism_engine_addUnreleasedDeliveryId(hSessionD,
                                                NULL,
                                                1,
                                                &hUnrel,
                                                NULL,
                                                0,
                                                NULL);
        TEST_ASSERT(rc == OK, ("Failed to add unreleased delivery ID. rc = %d", rc));

        rc = ism_engine_addUnreleasedDeliveryId(hSessionD,
                                                NULL,
                                                3,
                                                &hUnrel,
                                                NULL,
                                                0,
                                                NULL);
        TEST_ASSERT(rc == OK, ("Failed to add unreleased delivery ID. rc = %d", rc));

        rc = sync_ism_engine_createLocalTransaction(hSessionD,
                                                    &hTran);
        TEST_ASSERT(rc == OK, ("Failed to create a local transaction. rc = %d", rc));

        rc = sync_ism_engine_removeUnreleasedDeliveryId(hSessionD,
                                                        hTran,
                                                        hUnrel);
        TEST_ASSERT(rc == OK, ("Failed to remove unreleased delivery ID. rc = %d", rc));

        rc = ism_engine_addUnreleasedDeliveryId(hSessionD,
                                                hTran,
                                                2,
                                                &hUnrel,
                                                NULL,
                                                0,
                                                NULL);
        TEST_ASSERT(rc == OK, ("Failed to add unreleased delivery ID. rc = %d", rc));

        // Try again to force LockNotGranted
        rc = ism_engine_addUnreleasedDeliveryId(hSessionD,
                                                hTran,
                                                2,
                                                &hUnrel2,
                                                NULL,
                                                0,
                                                NULL);
        TEST_ASSERT(rc == ISMRC_LockNotGranted, ("Add unreleased delivery ID. rc = %d", rc));

        rc = sync_ism_engine_removeUnreleasedDeliveryId(hSessionD,
                                                        hTran,
                                                        hUnrel);
        TEST_ASSERT(rc == ISMRC_LockNotGranted, ("Remove unreleased delivery ID. rc = %d", rc));

        // Create a consumer to get the non-will message - this will set delivery IDs on them to make MDRs
        ismEngine_ConsumerHandle_t hConsumer = NULL;
        ismEngine_SubscriptionAttributes_t subAttributes = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                             ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

        rc = ism_engine_createConsumer(hSessionD,
                                       ismDESTINATION_SUBSCRIPTION,
                                       "Sub1",
                                       &subAttributes,
                                       NULL, // Owning client same as session client
                                       NULL,
                                       0,
                                       &messageDeliveryPreRestartCB,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttributes.subOptions),
                                       &hConsumer,
                                       NULL,
                                       0,
                                       NULL);
        TEST_ASSERT(rc == OK, ("Failed to create consumer. rc = %d", rc));

        /*************************************************************/
        /* We leave a dangling transaction here to make sure that    */
        /* it gets rolled back properly by restart.                  */
        /*************************************************************/

        /*************************************************************/
        /* Having initialised the queue, now re-exec ourself in      */
        /* restart mode.                                             */
        /*************************************************************/
        if (runinline)
        {
            verbose(2, "Ignoring restart, entering next phase...");
            phase++;

            rc = sync_ism_engine_destroyClientState(hClientD,
                                                    ismENGINE_DESTROY_CLIENT_OPTION_NONE);
            TEST_ASSERT(rc == OK, ("Destroy client-state failed. rc = %d", rc));

            rc = sync_ism_engine_destroyClientState(hClientND,
                                                    ismENGINE_DESTROY_CLIENT_OPTION_NONE);
            TEST_ASSERT(rc == OK, ("Destroy client-state failed. rc = %d", rc));

            rc = sync_ism_engine_destroyClientState(hClientD2,
                                                    ismENGINE_DESTROY_CLIENT_OPTION_NONE);
            TEST_ASSERT(rc == OK, ("Destroy client-state failed. rc = %d", rc));
        }
        else
        {
            verbose(2, "Restarting...");
            rc = test_execv(newargv[0], newargv);
            TEST_ASSERT(false, ("'test_execv' failed. rc = %d", rc));
        }
    }

    if (phase == 2)
    {
        // Request the setting of the ReducedMemoryRecoveryMode randomly
        test_engineInit_betweenInitAndStartFunc = set_ReducedMemoryRecoveryMode;

        /*************************************************************/
        /* Start up the Engine - Warm Start                          */
        /*************************************************************/
        if (!runinline)
        {
            verbose(2, "Starting Engine - warm start");

            rc = test_engineInit(false,
                                 true,
                                 ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                                 false, /*recovery should complete ok*/
                                 ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                                 testDEFAULT_STORE_SIZE);
            TEST_ASSERT(rc == OK, ("'test_engineInit' failed. rc = %d", rc));
        }

        /*************************************************************/
        /* Having initialised the queue, now re-exec ourself in      */
        /* restart mode.                                             */
        /*************************************************************/
        if (runinline)
        {
            verbose(2, "Ignoring restart, entering next phase...");
            phase++;

            rc = sync_ism_engine_destroyClientState(hClientD,
                                                    ismENGINE_DESTROY_CLIENT_OPTION_NONE);
            TEST_ASSERT(rc == OK, ("Destroy client-state failed. rc = %d", rc));

            rc = sync_ism_engine_destroyClientState(hClientND,
                                                    ismENGINE_DESTROY_CLIENT_OPTION_NONE);
            TEST_ASSERT(rc == OK, ("Destroy client-state failed. rc = %d", rc));

            rc = sync_ism_engine_destroyClientState(hClientD2,
                                                    ismENGINE_DESTROY_CLIENT_OPTION_NONE);
            TEST_ASSERT(rc == OK, ("Destroy client-state failed. rc = %d", rc));
        }
        else
        {
            verbose(2, "Restarting...");
            rc = test_execv(newargv[0], newargv);
            TEST_ASSERT(false, ("'test_execv' failed. rc = %d", rc));
        }
    }

    if (phase == 3)
    {
        // Request the setting of the ReducedMemoryRecoveryMode randomly
        test_engineInit_betweenInitAndStartFunc = set_ReducedMemoryRecoveryMode;

        /*************************************************************/
        /* Start up the Engine - Warm Start                          */
        /*************************************************************/
        if (!runinline)
        {
            verbose(2, "Starting Engine - warm start");

            rc = test_engineInit(false,
                                 true,
                                 ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                                 false, /*recovery should complete ok*/
                                 ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                                 testDEFAULT_STORE_SIZE);
            TEST_ASSERT(rc == OK, ("'test_engineInit' failed. rc = %d", rc));
        }

        rc = sync_ism_engine_createClientState("Recovery1",
                                               testDEFAULT_PROTOCOL_ID,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &hClientD);
        TEST_ASSERT(rc == ISMRC_ResumedClientState, ("Failed to create client-state. rc = %d", rc));

        rc = ism_engine_createSession(hClientD,
                                      ismENGINE_CREATE_SESSION_OPTION_NONE,
                                      &hSessionD,
                                      NULL,
                                      0,
                                      NULL);
        TEST_ASSERT(rc == OK, ("Failed to create session. rc = %d", rc));

        rc = ism_engine_startMessageDelivery(hSessionD,
                                             ismENGINE_START_DELIVERY_OPTION_NONE,
                                             NULL,
                                             0,
                                             NULL);
        TEST_ASSERT(rc == OK, ("Failed to start message delivery. rc = %d", rc));

        rc = ism_engine_listUnreleasedDeliveryIds(hClientD,
                                                  NULL,
                                                  &unreleasedCB);
        TEST_ASSERT(rc == OK, ("Failed to list unreleased delivery IDs. rc = %d", rc));
        TEST_ASSERT(unreleasedIdCount == 2, ("Failed to recover both unreleased delivery IDs."));

        rc = ism_engine_listSubscriptions(hClientD,
                                          NULL,
                                          NULL,
                                          &durableSubCB);
        TEST_ASSERT(rc == OK, ("Failed to list subs. rc = %d", rc));

        rc = sync_ism_engine_destroyClientState(hClientD,
                                                ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);

        TEST_ASSERT(rc == OK, ("Destroy client-state failed. rc = %d", rc));

        ieutThreadData_t *pThreadData = ieut_getThreadData();

        // Check that "Recovery3" which had a delayed will message still has a will message
        hClientD2 = NULL;
        iecs_findClientState(pThreadData, "Recovery3", false, &hClientD2);
        TEST_ASSERT(hClientD2 != NULL, ("NULL client Handle returned."));
        TEST_ASSERT(hClientD2->WillDelayExpiryTime != 0, ("WillDelayExpiryTime is zero"));
        TEST_ASSERT(hClientD2->hWillMessage != NULL, ("hWillMessage is NULL"));

        iecs_releaseClientStateReference(pThreadData, hClientD2, false, false);

        // Resume the clientState and go through logic to update the CPR.
        rc = sync_ism_engine_createClientState("Recovery3",
                                               testDEFAULT_PROTOCOL_ID,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &hClientD2);
        TEST_ASSERT(rc == ISMRC_ResumedClientState, ("Failed to create client-state. rc = %d", rc));
        TEST_ASSERT(hClientD2->WillDelay == 0, ("WillDelay unexpected value %u", hClientD2->WillDelay));
        TEST_ASSERT(hClientD2->hWillMessage == NULL, ("hWillMessage is %p", hClientD2->hWillMessage));
        TEST_ASSERT(hClientD2->WillDelayExpiryTime == 0, ("WillDelayExpiryTime is %lu", hClientD2->WillDelayExpiryTime));

        rc = sync_ism_engine_destroyClientState(hClientD2,
                                                ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
        TEST_ASSERT(rc == OK, ("Destroy client-state failed. rc = %d", rc));

        test_engineTerm(false);
        test_processTerm(false);
        test_removeAdminDir(adminDir);
    }

    verbose(2, "Success. Test complete!");

mod_exit:
    return rc ? 1 : 0;
}
