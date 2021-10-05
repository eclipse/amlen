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
/* Module Name: testStoreMQRecovery.c                                */
/*                                                                   */
/* Description: Test program which validates the recovery of data    */
/*              held in the Store on behalf of MQ Connectivity.      */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <getopt.h>

#include "engineInternal.h"
#include "transaction.h"

#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_sync.h"


/*********************************************************************/
/* Global data                                                       */
/*********************************************************************/
static ismEngine_ClientStateHandle_t hClient = NULL;
static ismEngine_SessionHandle_t hSession = NULL;
static ismEngine_TransactionHandle_t hTran = NULL;
static ismEngine_QManagerRecordHandle_t hQMgrRec = NULL;
static ismEngine_QMgrXidRecordHandle_t hQMgrXidRec1 = NULL;
static ismEngine_QMgrXidRecordHandle_t hQMgrXidRec2 = NULL;

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

int32_t parseArgs( int argc
                 , char *argv[]
                 , uint32_t *pphase
                 , bool *prestart
                 , bool *pruninline
                 , uint32_t *pverboseLevel
                 , int *ptrclvl
                 , char **padminDir )
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
    *pruninline = false;
    *pverboseLevel = 3;
    *ptrclvl = 0;
    *padminDir = NULL;

    while ((opt = getopt_long(argc, argv, "riv:a:0123456789", long_options, &long_index)) != -1)
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
            case 'i':
                *pruninline = true;
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

void listQManagerRecordsCB(
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QManagerRecordHandle_t    hQMgrRecHdl,
    void *                              pContext)
{
    ismEngine_QManagerRecordHandle_t *phQMgrRec = (ismEngine_QManagerRecordHandle_t *)pContext;
    *phQMgrRec = hQMgrRecHdl;
}

void listQMgrXidRecordsCB(
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QMgrXidRecordHandle_t     hQMgrXidRec,
    void *                              pContext)
{
    ismEngine_QMgrXidRecordHandle_t *phQMgrXidRec = (ismEngine_QMgrXidRecordHandle_t *)pContext;
    *phQMgrXidRec = hQMgrXidRec;
}

/*********************************************************************/
/* NAME:        testStoreMQRecovery.c                                */
/* DESCRIPTION: Test program which validates the recovery of data    */
/*              held in the Store on behalf of MQ Connectivity.      */
/*                                                                   */
/*              Each test runs in 2 phases                           */
/*              Phase 1. The MQ data is loaded into the Store.       */
/*              Phase 2. At the end of Phase 1, the program re-execs */
/*                       itself and recovers from the Store.         */
/* USAGE:       testStoreMQRecovery.c [-r] [-v 0-9] [-0...-9]        */
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
    int i;
    uint32_t phase = 1;
    bool restart = true;
    bool runinline = false;
    char *adminDir = NULL;

    if (argc != 1)
    {
        /*************************************************************/
        /* Parse arguments                                           */
        /*************************************************************/
        rc = parseArgs( argc
                      , argv
                      , &phase
                      , &restart
                      , &runinline
                      , &verboseLevel
                      , &trclvl
                      , &adminDir );
        if (rc != 0)
        {
            goto mod_exit;
        }
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
        for (i=1; i <= argc; i++)
        {
            newargv[i+3]=argv[i];
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

        rc = ism_engine_createClientState("Recovery1",
                                          PROTOCOL_ID_MQ,
                                          ismENGINE_CREATE_SESSION_OPTION_NONE,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &hClient,
                                          NULL,
                                          0,
                                          NULL);
        TEST_ASSERT(rc == OK, ("Failed to create client-state. rc = %d", rc));

        rc = ism_engine_createSession(hClient,
                                      ismENGINE_CREATE_SESSION_OPTION_NONE,
                                      &hSession,
                                      NULL,
                                      0,
                                      NULL);
        TEST_ASSERT(rc == OK, ("Failed to create session. rc = %d", rc));

        rc = sync_ism_engine_createLocalTransaction(hSession,
                                               &hTran);
        TEST_ASSERT(rc == OK, ("Failed to create a local transaction. rc = %d", rc));

        rc = ism_engine_createQManagerRecord(hSession,
                                             "QManager1",
                                             10,
                                             &hQMgrRec,
                                             NULL,
                                             0,
                                             NULL);
        TEST_ASSERT(rc == OK, ("Failed to create queue-manager record. rc = %d", rc));

        // NOTE: This no longer results in a record written to the persistence store.
        rc = ism_engine_createQMgrXidRecord(hSession,
                                            hQMgrRec,
                                            NULL,
                                            "QM1XID1",
                                            8,
                                            &hQMgrXidRec1,
                                            NULL,
                                            0,
                                            NULL);
        TEST_ASSERT(rc == OK, ("Failed to create queue-manager XID record. rc = %d", rc));

        // NOTE: This no longer results in a record written to the persistence store.
        rc = ism_engine_createQMgrXidRecord(hSession,
                                            hQMgrRec,
                                            hTran,
                                            "QM1XID2",
                                            8,
                                            &hQMgrXidRec2,
                                            NULL,
                                            0,
                                            NULL);
        TEST_ASSERT(rc == OK, ("Failed to create queue-manager XID record. rc = %d", rc));

        rc = ism_engine_destroyQMgrXidRecord(hSession,
                                             hQMgrXidRec1,
                                             hTran,
                                             NULL,
                                             0,
                                             NULL);
        TEST_ASSERT(rc == OK, ("Failed to destroy queue-manager XID record. rc = %d", rc));


        /*************************************************************/
        /* We leave a dangling transaction here to make sure that    */
        /* it gets rolled back properly by restart.                  */
        /*************************************************************/

        /*************************************************************/
        /* Having initialised the queue, now re-exec ourself in      */
        /* restart mode.                                             */
        /*************************************************************/
        if (restart)
        {
            if (runinline)
            {
                verbose(2, "Ignoring restart, entering phase 2...");
                phase = 2;

                rc = ism_engine_destroyClientState(hClient,
                                                   ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                                   NULL,
                                                   0,
                                                   NULL);
                TEST_ASSERT(rc == OK, ("Destroy client-state failed. rc = %d", rc));
            }
            else
            {
                verbose(2, "Restarting...");
                rc = test_execv(newargv[0], newargv);
                TEST_ASSERT(false, ("'execv' failed. rc = %d", rc));
            }
        }
    }

    if (phase == 2)
    {
        /*************************************************************/
        /* Start up the Engine - Warm Start                          */
        /*************************************************************/
        if (!runinline)
        {
            verbose(2, "Starting Engine - warm start");

            rc = test_engineInit(false,  true,
                                 ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                                 false, /*recovery should complete ok*/
                                 ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                                 testDEFAULT_STORE_SIZE);

            TEST_ASSERT(rc == OK, ("'test_engineInit' failed. rc = %d", rc));
        }

        rc = ism_engine_createClientState("Recovery1",
                                          PROTOCOL_ID_MQ,
                                          ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &hClient,
                                          NULL,
                                          0,
                                          NULL);
        TEST_ASSERT(rc == OK, ("Failed to create client-state. rc = %d", rc));

        rc = ism_engine_createSession(hClient,
                                      ismENGINE_CREATE_SESSION_OPTION_NONE,
                                      &hSession,
                                      NULL,
                                      0,
                                      NULL);
        TEST_ASSERT(rc == OK, ("Failed to create session. rc = %d", rc));

        hQMgrRec = NULL;
        rc = ism_engine_listQManagerRecords(hSession,
                                            &hQMgrRec,
                                            listQManagerRecordsCB);
        TEST_ASSERT(rc == OK, ("List queue-manager records failed. rc = %d", rc));
        TEST_ASSERT(hQMgrRec != NULL, ("List queue-manager records returned nothing."));

        // NOTE: Will only have an XID and associated linkage with the QMgr record if we
        //       have not restarted.
        if (runinline)
        {
            rc = ism_engine_destroyQManagerRecord(hSession,
                                                  hQMgrRec,
                                                  NULL,
                                                  0,
                                                  NULL);
            TEST_ASSERT(rc == ISMRC_LockNotGranted, ("Destroy in-use queue-manager record not rejected. rc = %d", rc));

            hQMgrXidRec1 = NULL;
            rc = ism_engine_listQMgrXidRecords(hSession,
                                               hQMgrRec,
                                               &hQMgrXidRec1,
                                               listQMgrXidRecordsCB);
            TEST_ASSERT(rc == OK, ("List queue-manager XID records failed. rc = %d", rc));
            TEST_ASSERT(hQMgrXidRec1 != NULL, ("List queue-manager XID records returned nothing."));

            rc = ism_engine_destroyQMgrXidRecord(hSession,
                                                 hQMgrXidRec1,
                                                 NULL,
                                                 NULL,
                                                 0,
                                                 NULL);
            TEST_ASSERT(rc == OK, ("Destroy queue-manager XID record failed. rc = %d", rc));
        }

        rc = ism_engine_destroyQManagerRecord(hSession,
                                              hQMgrRec,
                                              NULL,
                                              0,
                                              NULL);
        TEST_ASSERT(rc == OK, ("Destroy queue-manager record failed. rc = %d", rc));

        rc = ism_engine_destroyClientState(hClient,
                                           ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                           NULL,
                                           0,
                                           NULL);
        TEST_ASSERT(rc == OK, ("Destroy client-state failed. rc = %d", rc));

        test_engineTerm(false);
        test_processTerm(false);
        test_removeAdminDir(adminDir);
    }

mod_exit:
    return rc ? 1 : 0;
}
