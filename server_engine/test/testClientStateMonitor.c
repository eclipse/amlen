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
/* Module Name: testClientStateMonitor.c                             */
/*                                                                   */
/* Description: Test program which exercises the monitoring of       */
/*              client-states and the recovery of the associated     */
/*              from the Store.                                      */
/*                                                                   */
/*********************************************************************/
#include <stdio.h>
#include <assert.h>
#include <getopt.h>

#include "engineInternal.h"
#include "clientState.h"
#include "transaction.h"
#include "exportClientState.h"
#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "resourceSetStats.h"

void test_checkResourceSetStats(ieutThreadData_t *pThreadData, int32_t expectRC, char *expectSetIdOrder[])
{
    ismEngine_ResourceSetMonitor_t *results = NULL;
    uint32_t resultCount = 0;
    int32_t rc = ism_engine_getResourceSetMonitor(&results, &resultCount, ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES, 50, NULL);

    TEST_ASSERT(rc == expectRC, ("LINE:%d. Failed to get resourceset monitor rc = %d", __LINE__, rc));
    if (rc == OK)
    {
        TEST_ASSERT(results != NULL, ("LINE:%d. NULL Results", __LINE__));

        int32_t r=0;
        while(expectSetIdOrder[r] != NULL)
        {
            TEST_ASSERT(strcmp(results[r].resourceSetId, expectSetIdOrder[r]) == 0,
                        ("LINE:%d. Expected resourceSetId '%s' at position %d, got '%s'",
                         __LINE__, expectSetIdOrder[r], r, results[r].resourceSetId));
            iere_traceResourceSet(pThreadData, 0, results[r].resourceSet);
            r++;
        }
        TEST_ASSERT(resultCount == r, ("LINE:%d. ResultCount %d, expected %d.", __LINE__, resultCount, r));
        ism_engine_freeResourceSetMonitor(results);
    }
}

/*********************************************************************/
/* Global data                                                       */
/*********************************************************************/
static ismEngine_ClientStateHandle_t hClientD1 = NULL;
static ismEngine_ClientStateHandle_t hClientD2 = NULL;
static ismEngine_ClientStateHandle_t hClientD3 = NULL;
static ismEngine_ClientStateHandle_t hClientND = NULL;

// 0   Silent
// 1   Test Name only
// 2   + Test Description
// 3   + Validation checks
// 4   + Test Progress
// 5   + Verbose Test Progress
static uint32_t verboseLevel = 5;   /* 0-9 */

static void recoveryCreateClientStateCallback(int32_t rc, void *handle, void *pContext)
{
    ismEngine_ClientStateHandle_t *phClient = *(ismEngine_ClientStateHandle_t **)pContext;
    TEST_ASSERT(rc == OK, ("Unexpected RC=%d\n", rc));
    *phClient = handle;
}

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

int32_t parseArgs( int argc
                 , char *argv[]
                 , uint32_t *pphase
                 , bool *prestart
                 , uint32_t *pverboseLevel
                 , int *ptrclvl
                 , char **padminDir)
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { "recover", 0, NULL, 'r' },
        { "verbose", 1, NULL, 'v' },
        { "adminDir", 1, NULL, 'a' },
        { NULL, 1, NULL, 0 } };
    int long_index;

    *pphase = 1;
    *prestart = true;
    *pverboseLevel = 3;
    *ptrclvl = 0;
    *padminDir = NULL;

    while ((opt = getopt_long(argc, argv, "rv:a:0123456789", long_options, &long_index)) != -1)
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
            case 'r':
                *pphase = 2;
                break;
            case 'v':
               *pverboseLevel = atoi(optarg);
               break;
            case 'a':
                *padminDir = optarg;
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
        fprintf(stderr, "       -r (--recover)   - Run only recovery phase\n");
        fprintf(stderr, "       -v (--verbose)   - Verbosity\n");
        fprintf(stderr, "       -a (--adminDir)  - Admin directory to use\n");
        fprintf(stderr, "       -0 .. -9         - Trace level\n");
    }

   return usage;
}


/*********************************************************************/
/* NAME:        testClientStateMonitor                               */
/* DESCRIPTION: This program exercises the monitoring of             */
/*              client-states and the recovery of the associated     */
/*              from the Store.                                      */
/*                                                                   */
/*              Each test runs in 2 phases                           */
/*              Phase 1. The client-state information is loaded into */
/*                       the Store.                                  */
/*              Phase 2. At the end of Phase 1, the program re-execs */
/*                       itself and recovers from the Store.         */
/* USAGE:       test_clientStateMonitor [-r] [-v 0-9] [-0...-9]      */
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
    char *newargv[20];
    int i;
    uint32_t phase = 1;
    bool restart = true;
    bool runinline = false;
    uint32_t resultCount;
    ismEngine_ClientStateMonitor_t *pClientMonitor = NULL;
    ismEngine_SubscriptionMonitor_t *pSubMonitor = NULL;
    char *adminDir = NULL;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    if (argc != 1)
    {
        /*************************************************************/
        /* Parse arguments                                           */
        /*************************************************************/
        rc = parseArgs( argc
                      , argv
                      , &phase
                      , &restart
                      , &verboseLevel
                      , &trclvl
                      , &adminDir );
        if (rc != 0)
        {
            goto mod_exit;
        }
    }

    // We want to gather resource set stats (note: not tracking unmatched)
    setenv("IMA_TEST_RESOURCESET_CLIENTID_REGEX", "^([^-]+)-", false);
    setenv("IMA_TEST_RESOURCESET_TOPICSTRING_REGEX", "^topics/([^/]+)/.*", false);
    setenv("IMA_TEST_RESOURCESET_TRACK_UNMATCHED", "0", true);

    /*****************************************************************/
    /* Process Initialise                                            */
    /*****************************************************************/
    rc = test_processInit(trclvl, adminDir);
    if (rc != OK)
    {
        goto mod_exit;
    }

    char localAdminDir[1024];
    if (adminDir == NULL)
    {
        test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir));
        adminDir = localAdminDir;
    }

    ism_field_t f;
    ism_prop_t *filterProperties;
    f.type = VT_String;

    if (phase == 1)
    {
        /*************************************************************/
        /* Prepare the restart command                               */
        /*************************************************************/
        TEST_ASSERT(argc < 8, ("argc (%d) >= 8", argc));

        newargv[0] = argv[0];
        newargv[1] = "-r";
        newargv[2] = "-a";
        newargv[3] = adminDir;
        for (i = 1; i <= argc; i++)
        {
            newargv[i + 3] = argv[i];
        }

        /*************************************************************/
        /* Start up the Engine - Cold Start                          */
        /*************************************************************/
        verbose(2, "Starting Engine - cold start");
        rc = test_engineInit_DEFAULT;
        if (rc != 0)
        {
            goto mod_exit;
        }

        // Initially, there should be an empty list of inactive client-states - try with both
        // valid request types
        for(int32_t loop=0; loop<2; loop++)
        {
            rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                                  &resultCount,
                                                  loop == 0 ? ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME :
                                                              ismENGINE_MONITOR_ALL_UNSORTED,
                                                  5,
                                                  NULL);
            TEST_ASSERT(rc == OK, ("Failed to get empty list of client-states. rc = %d", rc));
            TEST_ASSERT(resultCount == 0, ("List of client-states not empty."));

            ism_engine_freeClientStateMonitor(pClientMonitor);
        }

        // Should tolerate freeing NULL
        ism_engine_freeClientStateMonitor(NULL);

        ieutThreadData_t *pThreadData = ieut_getThreadData();

        ieieExportResourceControl_t control = {0};

        control.options = ismENGINE_EXPORT_RESOURCES_OPTION_NONE;

        rc = ieut_createHashTable(pThreadData, 100, iemem_monitoringData, &control.clientIdTable);
        TEST_ASSERT(rc == OK, ("Failed to create hash table rc = %d", rc));
        TEST_ASSERT(control.clientIdTable != NULL, ("Failed to create hash table testHashTable = NULL"));

        rc = ism_regex_compile(&control.regexClientId, ".*ecov.*");
        TEST_ASSERT(rc == OK, ("Failed to compile regex rc = %d", rc));

        // Try getting a list of clientStates to export matching clientId "^"
        rc = ieie_getMatchingClientIds(pThreadData, &control);
        TEST_ASSERT(rc == OK, ("Failed to get matching client Ids", rc));
        TEST_ASSERT(control.clientIdTable->totalCount == 0, ("Unexpected number of clients returned %lu", control.clientIdTable->totalCount));

        // Try specifying a maxMessages value of 0 - for the LASTCONNECTEDTIME this should fail,
        // for ALL_UNSORTED it should succeed (but find no results)
        for(int32_t loop=0; loop<2; loop++)
        {
            pClientMonitor = NULL;

            rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                                  &resultCount,
                                                  loop == 0 ? ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME :
                                                              ismENGINE_MONITOR_ALL_UNSORTED,
                                                  0,
                                                  NULL);
            if (loop == 0)
            {
                TEST_ASSERT(rc == ISMRC_InvalidParameter, ("Failed to return invalid parameter. rc = %d", rc));
                TEST_ASSERT(pClientMonitor == NULL, ("pClientMonitor %p when expected NULL", pClientMonitor));
            }
            else
            {
                TEST_ASSERT(rc == OK, ("Failed to get empty list of client-states. rc = %d", rc));
                TEST_ASSERT(resultCount == 0, ("List of client-states not empty."));
            }

            // Always free (pClientMonitor = NULL should be tolerated)
            ism_engine_freeClientStateMonitor(pClientMonitor);
        }

        // Create three durable client-states. They are initially in use, so they should
        // not be reported using the monitoring interface
        ismEngine_ClientStateHandle_t *phClientD1 = &hClientD1;
        rc = ism_engine_createClientState("Set1-Recovery1",
                                          PROTOCOL_ID_MQTT,
                                          ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &hClientD1,
                                          &phClientD1,
                                          sizeof(phClientD1),
                                          recoveryCreateClientStateCallback);
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("Failed to create client-state. rc = %d", rc));

        ismEngine_ClientStateHandle_t *phClientD2 = &hClientD2;
        rc = ism_engine_createClientState("Set2-Recovery2",
                                          PROTOCOL_ID_HTTP,
                                          ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &hClientD2,
                                          &phClientD2,
                                          sizeof(phClientD2),
                                          recoveryCreateClientStateCallback);
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("Failed to create client-state. rc = %d", rc));

        ismEngine_ClientStateHandle_t *phClientD3 = &hClientD3;
        rc = ism_engine_createClientState("Set1-Recovery3",
                                          PROTOCOL_ID_MQTT,
                                          ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &hClientD3,
                                          &phClientD3,
                                          sizeof(phClientD3),
                                          recoveryCreateClientStateCallback);
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("Failed to create client-state. rc = %d", rc));

        // Wait for the 3 client states to be created
        while(hClientD1 == NULL || hClientD2 == NULL || hClientD3 == NULL)
        {
            sched_yield();
        }

        // Check what resourceSet monitor gives us
        test_checkResourceSetStats(pThreadData, OK, (char *[]){"Set1","Set2",NULL});

        rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                              &resultCount,
                                              ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME,
                                              5,
                                              NULL);
        TEST_ASSERT(rc == OK, ("Failed to get empty list of client-states. rc = %d", rc));
        TEST_ASSERT(resultCount == 0, ("List of client-states not empty."));

        ism_engine_freeClientStateMonitor(pClientMonitor);

        // Filter the clients by resourceSet and various different connectionStates
        filterProperties = ism_common_newProperties(10);
        f.val.s = "Set1";

        ism_common_setProperty(filterProperties, ismENGINE_MONITOR_FILTER_RESOURCESET_ID, &f);

        const char *connectionStates[] = {ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_DISCONNECTED,
                                          ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_CONNECTED,
                                          ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_ALL,
                                          ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_CONNECTED","ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_DISCONNECTED,
                                          "99",
                                          "  0, 0 ,0   ",
                                          NULL };
        int32_t expectedRC[] = {OK, OK, OK, OK, ISMRC_InvalidParameter, OK, OK};
        uint32_t expectedResultCount[] = {0, 2, 2, 2, 0, 2, 0};

        const char **thisConnectionState = connectionStates;
        int32_t *thisExpectedRC = expectedRC;
        uint32_t *thisExpectedResultCount = expectedResultCount;
        while(*thisConnectionState)
        {
            f.val.s = (char *)(*thisConnectionState);
            ism_common_setProperty(filterProperties, ismENGINE_MONITOR_FILTER_CONNECTIONSTATE, &f);

            resultCount = 0;
            pClientMonitor = NULL;

            rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                                  &resultCount,
                                                  ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME,
                                                  5,
                                                  filterProperties);

            TEST_ASSERT(rc == *thisExpectedRC, ("Failed to get list of client-states. rc = %d", rc));
            TEST_ASSERT(resultCount == *thisExpectedResultCount, ("List of client-states not as expected."));

            thisConnectionState++;
            thisExpectedRC++;
            thisExpectedResultCount++;

            ism_engine_freeClientStateMonitor(pClientMonitor);
        }

        for(int32_t loop=0; loop<2; loop++)
        {
            rc = ieie_getMatchingClientIds(pThreadData, &control);
            TEST_ASSERT(rc == OK, ("Failed to get matching client Ids", rc));
            TEST_ASSERT(control.clientIdTable->totalCount == 3, ("Unexpected number of clients returned %lu", control.clientIdTable->totalCount));

            ieut_clearHashTable(pThreadData, control.clientIdTable);
        }

        // Create a client state that, because it's JMS should not come out in any results
        ismEngine_ClientStateHandle_t hClientJ1 = NULL;
        ismEngine_ClientStateHandle_t *phClientJ1 = &hClientJ1;
        rc = ism_engine_createClientState("JMS1",
                                          PROTOCOL_ID_JMS,
                                          ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &hClientJ1,
                                          &phClientJ1,
                                          sizeof(phClientJ1),
                                          recoveryCreateClientStateCallback);
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("Failed to create client-state. rc = %d", rc));

        while(hClientJ1 == NULL)
        {
            sched_yield();
        }

        rc = ieie_getMatchingClientIds(pThreadData, &control);
        TEST_ASSERT(rc == OK, ("Failed to get matching client Ids", rc));
        TEST_ASSERT(control.clientIdTable->totalCount == 3, ("Unexpected number of clients returned %lu", control.clientIdTable->totalCount));

        // Explicitly not clearing the hash table

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_destroyClientState(hClientJ1,
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                           &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("Failed to destroy client-state. rc = %d", rc));
        test_waitForRemainingActions(pActionsRemaining);

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE };

        // Make a durable subscription so we can make sure it survives restart and
        // it gets cleaned up when the client-state is properly destroyed
        rc = sync_ism_engine_createSubscription(hClientD2,
                                                "MySub",
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                "topics/Set2/T",
                                                &subAttrs,
                                                NULL); // Owning client same as requesting client
        TEST_ASSERT(rc == OK, ("Failed to create durable subscription. rc = %d", rc));

        rc = ism_engine_getSubscriptionMonitor(&pSubMonitor,
                                               &resultCount,
                                               ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                               5,
                                               NULL);
        TEST_ASSERT(rc == OK, ("Failed to get list of subscriptions. rc = %d", rc));
        TEST_ASSERT(resultCount == 1, ("List of subscriptions didn't contain 1."));

        ism_engine_freeSubscriptionMonitor(pSubMonitor);

        // Destroying one makes it "disconnected" but leaves it in the Store. It should now
        // be returned by the monitoring interface
        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_destroyClientState(hClientD1,
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                           &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("Destroy client-state failed. rc = %d", rc));
        test_waitForRemainingActions(pActionsRemaining);

        // Try querying with both types
        for(int32_t loop=0; loop<2; loop++)
        {
            rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                                  &resultCount,
                                                  loop == 0 ? ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME :
                                                              ismENGINE_MONITOR_ALL_UNSORTED,
                                                  5,
                                                  NULL);
            TEST_ASSERT(rc == OK, ("Failed to get list of client-states. rc = %d", rc));
            TEST_ASSERT(resultCount == 1, ("List of client-states didn't contain 1."));
            TEST_ASSERT(strcmp(pClientMonitor->ClientId, "Set1-Recovery1") == 0, ("Client ID was incorrect."));
            TEST_ASSERT(pClientMonitor->fIsConnected == false, ("Monitoring interface returned a connected client-state."));
            TEST_ASSERT(pClientMonitor->LastConnectedTime != 0, ("Last connected time was not initialised."));

            ism_engine_freeClientStateMonitor(pClientMonitor);
        }

        // Create a non-durable client-state. This should not be reported by the monitoring interface, ever.
        rc = ism_engine_createClientState("Set2-Nonrecovery1",
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

        test_checkResourceSetStats(pThreadData, OK, (char *[]){"Set2","Set1",NULL}); // Check what resourceSet monitor gives us

        rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                              &resultCount,
                                              ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME,
                                              5,
                                              NULL);
        TEST_ASSERT(rc == OK, ("Failed to get list of client-states. rc = %d", rc));
        TEST_ASSERT(resultCount == 1, ("List of client-states didn't contain 1."));
        TEST_ASSERT(strcmp(pClientMonitor->ClientId, "Set1-Recovery1") == 0, ("Client ID was incorrect."));
        TEST_ASSERT(pClientMonitor->fIsConnected == false, ("Monitoring interface returned a connected client-state."));
        TEST_ASSERT(pClientMonitor->LastConnectedTime != 0, ("Last connected time was not initialised."));

        ism_engine_freeClientStateMonitor(pClientMonitor);

        // The non-durable client state will come back when we are matching them.
        rc = ieie_getMatchingClientIds(pThreadData, &control);
        TEST_ASSERT(rc == OK, ("Failed to get matching client Ids", rc));
        TEST_ASSERT(control.clientIdTable->totalCount == 4, ("Unexpected number of clients returned %lu", control.clientIdTable->totalCount));

        ieut_destroyHashTable(pThreadData, control.clientIdTable);
        ism_regex_free(control.regexClientId);

        // Even when it's been destroyed, it should not be reported by the monitoring interface.
        rc = ism_engine_destroyClientState(hClientND,
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                           NULL,
                                           0,
                                           NULL);
        TEST_ASSERT(rc == OK, ("Destroy client-state failed. rc = %d", rc));

        rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                              &resultCount,
                                              ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME,
                                              5,
                                              NULL);
        TEST_ASSERT(rc == OK, ("Failed to get list of client-states. rc = %d", rc));
        TEST_ASSERT(resultCount == 1, ("List of client-states didn't contain 1."));
        TEST_ASSERT(strcmp(pClientMonitor->ClientId, "Set1-Recovery1") == 0, ("Client ID was incorrect."));
        TEST_ASSERT(pClientMonitor->fIsConnected == false, ("Monitoring interface returned a connected client-state."));
        TEST_ASSERT(pClientMonitor->LastConnectedTime != 0, ("Last connected time was not initialised."));

        ism_engine_freeClientStateMonitor(pClientMonitor);

        /*************************************************************/
        /* We leave a dangling client-state here to make sure that   */
        /* it gets disconnected properly by restart.                 */
        /*************************************************************/

        /*************************************************************/
        /* Having initialised the state, now re-exec in restart mode */
        /*************************************************************/
        if (restart)
        {
            if (runinline)
            {
                verbose(2, "Ignoring restart, entering phase 2...");
                phase = 2;

                test_incrementActionsRemaining(pActionsRemaining, 2);

                // Restart will cause the second & third client-state to be destroyed
                // only to be recovered from the Store.
                rc = ism_engine_destroyClientState(hClientD2,
                                                   ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                                   &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
                if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
                else TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("Destroy client-state failed. rc = %d", rc));

                rc = ism_engine_destroyClientState(hClientD3,
                                                   ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                                   &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
                if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
                else TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("Destroy client-state failed. rc = %d", rc));

                test_waitForRemainingActions(pActionsRemaining);

                // TODO: Check counts for ResourceSets Set1 and Set2
            }
            else
            {
                verbose(2, "Restarting...");
                rc = test_execv(newargv[0], newargv);
                TEST_ASSERT(false, ("'test_execv' failed. rc = %d", rc));
            }
        }
    }

    if (phase == 2)
    {
        ismEngine_ClientStateMonitor_t *pM;
        char *pFirstClientId = NULL;

        /*************************************************************/
        /* Start up the Engine - Warm Start                          */
        /*************************************************************/
        if (!runinline)
        {
            verbose(2, "Starting Engine - warm start");

            rc = test_engineInit(false, true,
                                 ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                                 false, /*recovery should complete ok*/
                                 ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                                 testDEFAULT_STORE_SIZE);

            TEST_ASSERT(rc == OK, ("'test_engineInit' failed. rc = %d", rc));
        }

        // Try getting with both types
        for(int32_t loop=0; loop<2; loop++)
        {
            pClientMonitor = NULL;
            resultCount = 0;

            rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                                  &resultCount,
                                                  loop == 0 ? ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME :
                                                              ismENGINE_MONITOR_ALL_UNSORTED,
                                                  2,
                                                  NULL);
            TEST_ASSERT(rc == OK, ("Failed to get list of client-states. rc = %d", rc));
            if (loop == 0)
            {
                TEST_ASSERT(resultCount == 2, ("List of client-states didn't contain 2."));
            }
            else
            {
                // maxResults gets ignored for ismENGINE_MONITOR_ALL_UNSORTED
                TEST_ASSERT(resultCount == 3, ("List of client-states didn't contain 3."));
            }

            // The client-states will come out in a different order depending on how the
            // test is run (whether it really restarted)
            for (i = 0, pM = pClientMonitor; i < resultCount; i++, pM++)
            {
                if (loop == 0 && i == 0)
                {
                    pFirstClientId = strdup(pM->ClientId);
                }
                TEST_ASSERT(pM->fIsConnected == false, ("Monitoring interface returned a connected client-state."));
                TEST_ASSERT(pM->LastConnectedTime != 0, ("Last connected time was not initialised."));
            }

            ism_engine_freeClientStateMonitor(pClientMonitor);
        }

        // Try filtering the results in various ways
        filterProperties = ism_common_newProperties(10);

        // Non regular expression filters
        char *FILTERS_CLIENTIDS_IDS[] = {"*", "Z", "Z*", "Nonrecovery1", "Set*Recovery*", "*ecov*", "*1"};
        #define FILTERS_CLIENTIDS_NUMIDS (sizeof(FILTERS_CLIENTIDS_IDS)/sizeof(FILTERS_CLIENTIDS_IDS[0]))
        uint32_t FILTERS_CLIENTIDS_EXPECTCOUNT[] = {3, 0, 0, 0, 3, 3, 1};
        int32_t FILTERS_CLIENTIDS_EXPECTRC[] = {OK, OK, OK, OK, OK, OK, OK};

        for(i=0; i<FILTERS_CLIENTIDS_NUMIDS; i++)
        {
            f.val.s = FILTERS_CLIENTIDS_IDS[i];
            rc = ism_common_setProperty(filterProperties,
                                        ismENGINE_MONITOR_FILTER_CLIENTID,
                                        &f);
            TEST_ASSERT(rc == OK, ("ism_common_setProperty failed with %d", rc));

            for(int32_t loop=0; loop<2; loop++)
            {
                pClientMonitor = NULL;
                resultCount = 0;

                rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                                      &resultCount,
                                                      loop == 0 ? ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME :
                                                                  ismENGINE_MONITOR_ALL_UNSORTED,
                                                      50,
                                                      filterProperties);
                TEST_ASSERT(rc == FILTERS_CLIENTIDS_EXPECTRC[i], ("ism_engine_getClientStateMonitor returned rc %d, expected %d", rc, FILTERS_CLIENTIDS_EXPECTRC[i]));
                TEST_ASSERT(resultCount == FILTERS_CLIENTIDS_EXPECTCOUNT[i], ("ism_engine_getClientStateMonitor returned %d results, expected %d", resultCount, FILTERS_CLIENTIDS_EXPECTCOUNT[i]));

                ism_engine_freeClientStateMonitor(pClientMonitor);
            }

            ism_common_clearProperties(filterProperties);
        }

        ieutThreadData_t *pThreadData = ieut_getThreadData();
        test_checkResourceSetStats(pThreadData, OK, (char *[]){"Set2","Set1",NULL});

        // Regular expression filters
        char *FILTERS_REGEX_CLIENTIDS_IDS[] = {".*", "Z", "Z.*", "^Nonrecovery1", "^Set[12]-[Re]+covery.*", ".*e[cC]ov.*", "^Set2+", "["};
        #define FILTERS_REGEX_CLIENTIDS_NUMIDS (sizeof(FILTERS_REGEX_CLIENTIDS_IDS)/sizeof(FILTERS_REGEX_CLIENTIDS_IDS[0]))
        uint32_t FILTERS_REGEX_CLIENTIDS_EXPECTCOUNT[] = {3, 0, 0, 0, 3, 3, 1, 0};
        int32_t FILTERS_REGEX_CLIENTIDS_EXPECTRC[] = {OK, OK, OK, OK, OK, OK, OK, ISMRC_InvalidParameter};

        for(i=0; i<FILTERS_REGEX_CLIENTIDS_NUMIDS; i++)
        {
            f.val.s = FILTERS_REGEX_CLIENTIDS_IDS[i];
            rc = ism_common_setProperty(filterProperties,
                                        ismENGINE_MONITOR_FILTER_REGEX_CLIENTID,
                                        &f);
            TEST_ASSERT(rc == OK, ("ism_common_setProperty failed with %d", rc));

            for(int32_t loop=0; loop<2; loop++)
            {
                pClientMonitor = NULL;
                resultCount = 0;

                rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                                      &resultCount,
                                                      loop == 0 ? ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME :
                                                                  ismENGINE_MONITOR_ALL_UNSORTED,
                                                      50,
                                                      filterProperties);
                TEST_ASSERT(rc == FILTERS_REGEX_CLIENTIDS_EXPECTRC[i], ("ism_engine_getClientStateMonitor returned rc %d, expected %d", rc, FILTERS_REGEX_CLIENTIDS_EXPECTRC[i]));
                TEST_ASSERT(resultCount == FILTERS_REGEX_CLIENTIDS_EXPECTCOUNT[i], ("ism_engine_getClientStateMonitor returned %d results, expected %d", resultCount, FILTERS_REGEX_CLIENTIDS_EXPECTCOUNT[i]));

                ism_engine_freeClientStateMonitor(pClientMonitor);
            }

            ism_common_clearProperties(filterProperties);
        }

        // Protocol filtering
        char *FILTERS_PROTOCOLIDS_TYPES[] = {"All", " DeviceLike ", "MQTT", "HTTP", "PlugIn ", "HTTP,MQTT", "  NOT-A-TYPE", "2,7,100", "0", "1000"};
        #define FILTERS_PROTOCOLIDS_NUMTYPES (sizeof(FILTERS_PROTOCOLIDS_TYPES)/sizeof(FILTERS_PROTOCOLIDS_TYPES[0]))
        uint32_t FILTERS_PROTOCOLIDS_EXPECTCOUNT[] = {4, 3, 2, 1, 0, 3, 0, 3, 0, 0};
        int32_t FILTERS_PROTOCOLIDS_EXPECTRC[] = {OK, OK, OK, OK, OK, OK, ISMRC_InvalidParameter, OK, ISMRC_InvalidParameter, ISMRC_InvalidParameter};

        for(i=0; i<FILTERS_PROTOCOLIDS_NUMTYPES; i++)
        {
            f.val.s = FILTERS_PROTOCOLIDS_TYPES[i];
            rc = ism_common_setProperty(filterProperties,
                                        ismENGINE_MONITOR_FILTER_PROTOCOLID,
                                        &f);
            TEST_ASSERT(rc == OK, ("ism_common_setProperty failed with %d", rc));

            for(int32_t loop=0; loop<2; loop++)
            {
                pClientMonitor = NULL;
                resultCount = 0;

                rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                                      &resultCount,
                                                      loop == 0 ? ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME :
                                                                  ismENGINE_MONITOR_ALL_UNSORTED,
                                                      50,
                                                      filterProperties);
                TEST_ASSERT(rc == FILTERS_PROTOCOLIDS_EXPECTRC[i], ("ism_engine_getClientStateMonitor returned rc %d, expected %d", rc, FILTERS_PROTOCOLIDS_EXPECTRC[i]));
                TEST_ASSERT(resultCount == FILTERS_PROTOCOLIDS_EXPECTCOUNT[i], ("ism_engine_getClientStateMonitor returned %d results, expected %d", resultCount, FILTERS_PROTOCOLIDS_EXPECTCOUNT[i]));

                // Specific test of protocol types returned
                uint32_t testProtocolId = 0;
                if (i == 2)
                {
                    TEST_ASSERT(strcmp(FILTERS_PROTOCOLIDS_TYPES[i], "MQTT") == 0,
                                ("Unexpected protocol Id %s", FILTERS_PROTOCOLIDS_TYPES[i]));
                    testProtocolId = PROTOCOL_ID_MQTT;
                }
                else if (i == 3)
                {
                    TEST_ASSERT(strcmp(FILTERS_PROTOCOLIDS_TYPES[i], "HTTP") == 0,
                                ("Unexpected protocol Id %s", FILTERS_PROTOCOLIDS_TYPES[i]));
                    testProtocolId = PROTOCOL_ID_HTTP;
                }

                if (testProtocolId != 0)
                {
                    for(int32_t c=0; c<resultCount; c++)
                    {
                        TEST_ASSERT(pClientMonitor[c].ProtocolId == testProtocolId,
                                    ("Wrong protocol type %u returned", pClientMonitor[c].ProtocolId));
                    }
                }

                ism_engine_freeClientStateMonitor(pClientMonitor);
            }

            ism_common_clearProperties(filterProperties);
        }

        // Clean up the properties structure we've been using.
        ism_common_freeProperties(filterProperties);
        filterProperties = NULL;

        // Now try querying just the oldest one, and ensure that we get the right one
        rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                              &resultCount,
                                              ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME,
                                              1,
                                              NULL);
        TEST_ASSERT(rc == OK, ("Failed to get list of client-states. rc = %d", rc));
        TEST_ASSERT(resultCount == 1, ("List of client-states didn't contain 1."));
        TEST_ASSERT(strcmp(pClientMonitor->ClientId, pFirstClientId) == 0, ("Client ID was incorrect."));

        ism_engine_freeClientStateMonitor(pClientMonitor);

        free(pFirstClientId);
        pFirstClientId = NULL;

        // Check that our durable subscription survived - it's owned by "Set2-Recovery2"
        rc = ism_engine_getSubscriptionMonitor(&pSubMonitor,
                                               &resultCount,
                                               ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                               5,
                                               NULL);
        TEST_ASSERT(rc == OK, ("Failed to get list of subscriptions. rc = %d", rc));
        TEST_ASSERT(resultCount == 1, ("List of subscriptions didn't contain 1."));
        TEST_ASSERT(strcmp(pSubMonitor->clientId, "Set2-Recovery2") == 0, ("Unexpected owner '%s'", pSubMonitor->clientId));

        ism_engine_freeSubscriptionMonitor(pSubMonitor);

        // Make the first and 3rd client-states active - they will be removed from the monitoring output
        rc = sync_ism_engine_createClientState("Set1-Recovery1",
                                               PROTOCOL_ID_MQTT,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &hClientD1);
        TEST_ASSERT(rc == ISMRC_ResumedClientState, ("Failed to create client-state. rc = %d", rc));

        rc = sync_ism_engine_createClientState("Set1-Recovery3",
                                               PROTOCOL_ID_MQTT,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &hClientD3);
        TEST_ASSERT(rc == ISMRC_ResumedClientState, ("Failed to create client-state. rc = %d", rc));

        rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                              &resultCount,
                                              ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME,
                                              5,
                                              NULL);
        TEST_ASSERT(rc == OK, ("Failed to get list of client-states. rc = %d", rc));
        TEST_ASSERT(resultCount == 1, ("List of client-states didn't contain 1."));
        TEST_ASSERT(strcmp(pClientMonitor->ClientId, "Set2-Recovery2") == 0, ("Client ID was incorrect."));
        TEST_ASSERT(pClientMonitor->fIsConnected == false, ("Monitoring interface returned a connected client-state."));
        TEST_ASSERT(pClientMonitor->LastConnectedTime != 0, ("Last connected time was not initialised."));

        ism_engine_freeClientStateMonitor(pClientMonitor);

        // Attempt to destroy and discard the active client-state by client ID from admin callback
        rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_MQTTCLIENT,
                                       "Set1-Recovery1",
                                       NULL,
                                       ISM_CONFIG_CHANGE_DELETE);
        TEST_ASSERT(rc == ISMRC_ClientIDConnected, ("Destroy disconnected client-state via callback was not bounced. rc = %d", rc));

        // Destroy Set1-Recovery3 using the engine API to do it - this should fail as Set1-Recovery3 is in use
        rc = ism_engine_destroyDisconnectedClientState("Set1-Recovery3", NULL, 0, NULL);
        TEST_ASSERT(rc == ISMRC_ClientIDConnected, ("Destroy disconnected client-state via API was not bounced rc = %d", rc));

        // Now discard Set1-Recovery1 and Set1-Recovery3
        test_incrementActionsRemaining(pActionsRemaining, 2);
        rc = ism_engine_destroyClientState(hClientD1,
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                           &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("Destroy client-state failed. rc = %d", rc));

        // Make sure that the lastconnectedTime for Set1-Recovery1 ought to be the oldest.
        sleep(1);

        rc = ism_engine_destroyClientState(hClientD3,
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                           &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("Destroy client-state failed. rc = %d", rc));

        test_waitForRemainingActions(pActionsRemaining);

        // Resume Recovery 2 with a wrong protocol (JMS)
        rc = sync_ism_engine_createClientState("Set2-Recovery2",
                                               PROTOCOL_ID_JMS,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &hClientD2);
        TEST_ASSERT(rc == ISMRC_ProtocolMismatch, ("Failed to create client-state. rc = %d", rc));

        // Resume Recovery 2 with MQTT which should be able to resume it
        rc = sync_ism_engine_createClientState("Set2-Recovery2",
                                               PROTOCOL_ID_MQTT,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &hClientD2);
        TEST_ASSERT(rc == ISMRC_ResumedClientState, ("Failed to create client-state. rc = %d", rc));

        // Discard Recovery 2
        rc = sync_ism_engine_destroyClientState(hClientD2,
                                                ismENGINE_DESTROY_CLIENT_OPTION_NONE);
        TEST_ASSERT(rc == OK, ("Destroy client-state failed. rc = %d", rc));

        rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                              &resultCount,
                                              ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME,
                                              1,
                                              NULL);
        TEST_ASSERT(rc == OK, ("Failed to get list of client-states. rc = %d", rc));
        TEST_ASSERT(resultCount == 1, ("List of client-states didn't contain 1."));
        TEST_ASSERT(strcmp(pClientMonitor->ClientId, "Set1-Recovery1") == 0, ("Client ID was incorrect."));
        TEST_ASSERT(pClientMonitor->fIsConnected == false, ("Monitoring interface returned a connected client-state."));
        TEST_ASSERT(pClientMonitor->LastConnectedTime != 0, ("Last connected time was not initialised."));

        ism_engine_freeClientStateMonitor(pClientMonitor);

        // Nasty! mess with the last connected times to change ordering of results
        for(int32_t loop=0; loop<100; loop++)
        {
            hClientD1->LastConnectedTime = 1+rand()%100;
            hClientD2->LastConnectedTime = 1+rand()%100;
            hClientD3->LastConnectedTime = 1+rand()%100;

            rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                                  &resultCount,
                                                  ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME,
                                                  2,
                                                  NULL);
            TEST_ASSERT(rc == OK, ("Failed to get list of client-states. rc = %d", rc));
            TEST_ASSERT(resultCount == 2, ("List of client-states didn't contain 2."));

            ism_engine_freeClientStateMonitor(pClientMonitor);
        }

        // Should be able to destroy Recovery 1 with Admin
        rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_MQTTCLIENT,
                                       "Set1-Recovery1",
                                       NULL,
                                       ISM_CONFIG_CHANGE_DELETE);
        TEST_ASSERT(rc == OK, ("Destroy disconnected client-state was not bounced. rc = %d", rc));

        // Should be able to destroy Recovery 3 & JMS1 with API
        rc = ism_engine_destroyDisconnectedClientState("Set1-Recovery3", NULL, 0, NULL);
        TEST_ASSERT(rc == OK, ("Couldn't destroy a zombie client by clientId rc= %d", rc));
        rc = ism_engine_destroyDisconnectedClientState("JMS1", NULL, 0, NULL);
        TEST_ASSERT(rc == OK, ("Couldn't destroy a zombie client by clientId rc= %d", rc));

        rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                              &resultCount,
                                              ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME,
                                              5,
                                              NULL);
        TEST_ASSERT(rc == OK, ("Failed to get list of client-states. rc = %d", rc));
        TEST_ASSERT(resultCount == 1, ("List of client-states didn't contain 1."));
        TEST_ASSERT(strcmp(pClientMonitor->ClientId, "Set2-Recovery2") == 0, ("Client ID was incorrect."));
        TEST_ASSERT(pClientMonitor->fIsConnected == false, ("Monitoring interface returned a connected client-state."));
        TEST_ASSERT(pClientMonitor->LastConnectedTime != 0, ("Last connected time was not initialised."));

        ism_engine_freeClientStateMonitor(pClientMonitor);

        // Destroy and discard the remaining client-state by client ID
        rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_MQTTCLIENT,
                                       "Set2-Recovery2",
                                       NULL,
                                       ISM_CONFIG_CHANGE_DELETE);
        TEST_ASSERT(rc == OK, ("Destroy disconnected client-state failed. rc = %d", rc));

        rc = ism_engine_getClientStateMonitor(&pClientMonitor,
                                              &resultCount,
                                              ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME,
                                              5,
                                              NULL);
        TEST_ASSERT(rc == OK, ("Failed to get empty list of client-states. rc = %d", rc));
        TEST_ASSERT(resultCount == 0, ("List of client-states not empty."));

        ism_engine_freeClientStateMonitor(pClientMonitor);

        // Check that our durable subscription has now disappeared
        rc = ism_engine_getSubscriptionMonitor(&pSubMonitor,
                                               &resultCount,
                                               ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                               5,
                                               NULL);
        TEST_ASSERT(rc == OK, ("Failed to get list of subscriptions. rc = %d", rc));
        TEST_ASSERT(resultCount == 0, ("List of subscriptions was not empty."));

        ism_engine_freeSubscriptionMonitor(pSubMonitor);

        // Finally, try to destroy a non-existent client-state by client ID
        rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_MQTTCLIENT,
                                       "Set2-Recovery2",
                                       NULL,
                                       ISM_CONFIG_CHANGE_DELETE);
        TEST_ASSERT(rc == ISMRC_NotFound, ("Unexpected result deleting non-existent client-state. rc = %d", rc));

        // Final rc must be zero so test exists cleanly
        rc = OK;

        test_engineTerm(false);

        test_processTerm(false);
        test_removeAdminDir(adminDir);
    }

    verbose(2, "Success. Test complete!");

mod_exit:
    return rc ? 1 : 0;
}
