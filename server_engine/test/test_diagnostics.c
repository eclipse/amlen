/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
/* Module Name: test_diagnostics.c                                   */
/*                                                                   */
/* Description: Main source file for CUnit test of                   */
/* ism_engine_diagnostics()  and engineDiag.c                        */
/*                                                                   */
/*********************************************************************/
#include <string.h>

#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_client.h"
#include "test_utils_initterm.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_preload.h"
#include "test_utils_security.h"
#include "test_utils_task.h"

#include "test_diagnosticsFileContent.h"

#include "engine.h"
#include "engineInternal.h"
#include "engineDiag.h"
#include "engineStore.h"
#include "engineTraceDump.h"

// Function to call if the checking of a diagnostics file failed
static void checkFailFunc(const char *function, int lineNumber, unsigned long int info)
{
    char failInfo[strlen(function)+200];

    sprintf(failInfo, "Failure in %s line %d [info=%lu]\n", function, lineNumber, info);

    TEST_ASSERT(false, (failInfo));
}

// -----------------------------------------------------------------
// Test invalid requests
// -----------------------------------------------------------------
void test_invalid(void)
{
    char *outputStr;

    // Invalid mode
    int32_t rc =  ism_engine_diagnostics("Invalid",
                                         NULL,
                                         &outputStr,
                                         NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    // NULL mode
    rc =  ism_engine_diagnostics(NULL,
                                 NULL,
                                 &outputStr,
                                 NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    // NULL diagnostics pointer
    rc = ism_engine_diagnostics(ediaVALUE_MODE_ECHO,
                                NULL,
                                NULL,
                                NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    // No filename or password on dumpClientStates
    rc = ism_engine_diagnostics(ediaVALUE_MODE_DUMPCLIENTSTATES,
                                NULL,
                                &outputStr,
                                NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    // Filename only on dumpClientStates
    rc = ism_engine_diagnostics(ediaVALUE_MODE_DUMPCLIENTSTATES,
                                "filename",
                                &outputStr,
                                NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    // Naughty filename on dumpClientStates
    rc = ism_engine_diagnostics(ediaVALUE_MODE_DUMPCLIENTSTATES,
                                "../filename password",
                                &outputStr,
                                NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    // Too many args?
    rc = ism_engine_diagnostics(ediaVALUE_MODE_DUMPCLIENTSTATES,
                                "1 2 3 4 5 6 7 8",
                                &outputStr,
                                NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

}

// -----------------------------------------------------------------
// Test Echo
// -----------------------------------------------------------------
typedef struct tag_testEchoData_t
{
    char *outputstr;
    volatile uint32_t CBFires;
} testEchoData_t;

void test_echo_cb(int32_t rc, void *handle, void *context)
{
    testEchoData_t *pDiagData = *(testEchoData_t **)context;
    TEST_ASSERT_EQUAL(rc, OK);

    pDiagData->outputstr = (char *)handle;

    __sync_add_and_fetch(&(pDiagData->CBFires), 1);
}

void test_echo(void)
{
    int32_t rc;
    bool expectAsync;
    testEchoData_t diagData;
    testEchoData_t *pDiagData = &diagData;
    ism_json_parse_t parseObj;
    ism_json_entry_t ents[10];

    for (uint32_t i = 0; i < 10; i++)
    {
        char argbuffer[64];

        diagData.outputstr = NULL;
        diagData.CBFires = 0; //not gone async at all yet

        sprintf(argbuffer, "\"\\testLoop%u\"%% 2nd Third", i);

        rc =  ism_engine_diagnostics(ediaVALUE_MODE_ECHO,
                                     argbuffer,
                                     &(diagData.outputstr),
                                     &pDiagData,
                                     sizeof(pDiagData),
                                     test_echo_cb);

        if (rc == ISMRC_AsyncCompletion)
        {
            test_waitForMessages(&(diagData.CBFires), NULL, 1, 20);
            expectAsync=true;
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
            expectAsync=false;
        }

        TEST_ASSERT_PTR_NOT_NULL(diagData.outputstr);

        // Check that what comes back is what we expect
        memset(&parseObj, 0, sizeof(parseObj));

        parseObj.ent = ents;
        parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
        parseObj.source = strdup(diagData.outputstr);
        parseObj.src_len = strlen(parseObj.source);

        rc = ism_json_parse(&parseObj);
        TEST_ASSERT_EQUAL(rc, OK);

        TEST_ASSERT_EQUAL(parseObj.ent_count, 11);
        TEST_ASSERT_EQUAL(parseObj.ent[0].objtype, JSON_Object);
        TEST_ASSERT_EQUAL(parseObj.ent[0].count, 10);

        for(int32_t x=1; x<parseObj.ent_count; x++)
        {
            ism_json_entry_t *entry = &parseObj.ent[x];

            if (entry->name != NULL)
            {
                if (strcmp(entry->name, "Mode") == 0)
                {
                    TEST_ASSERT_STRINGS_EQUAL(entry->value, ediaVALUE_MODE_ECHO);
                }
                else if (strcmp(entry->name, "Args") == 0)
                {
                    TEST_ASSERT_STRINGS_EQUAL(entry->value, argbuffer);
                }
                else if (strcmp(entry->name, "SimpleArgs") == 0)
                {
                    TEST_ASSERT_EQUAL(entry->objtype, JSON_Array);
                    TEST_ASSERT_EQUAL(entry->count, 3);
                }
                else if (strcmp(entry->name, "Async") == 0)
                {
                    TEST_ASSERT_EQUAL(entry->objtype, expectAsync ? JSON_True : JSON_False);
                }
            }
        }

        if (parseObj.free_ent) ism_common_free(ism_memory_utils_parser,parseObj.ent);
        free(parseObj.source);

        ism_engine_freeDiagnosticsOutput(diagData.outputstr);
    }

    // Test calling Echo with NULL args
    diagData.outputstr = NULL;
    diagData.CBFires = 0;
    rc =  ism_engine_diagnostics(ediaVALUE_MODE_ECHO,
                                 NULL,
                                 &(diagData.outputstr),
                                 &pDiagData,
                                 sizeof(pDiagData),
                                 test_echo_cb);
    if (rc == ISMRC_AsyncCompletion) test_waitForMessages(&(diagData.CBFires), NULL, 1, 20);
    else TEST_ASSERT_EQUAL(rc, OK);

    ism_engine_freeDiagnosticsOutput(diagData.outputstr);

    // Test calling Echo with empty args
    diagData.outputstr = NULL;
    diagData.CBFires = 0;
    rc =  ism_engine_diagnostics(ediaVALUE_MODE_ECHO,
                                 NULL,
                                 &(diagData.outputstr),
                                 &pDiagData,
                                 sizeof(pDiagData),
                                 test_echo_cb);
    if (rc == ISMRC_AsyncCompletion) test_waitForMessages(&(diagData.CBFires), NULL, 1, 20);
    else TEST_ASSERT_EQUAL(rc, OK);

    ism_engine_freeDiagnosticsOutput(diagData.outputstr);
}

// -----------------------------------------------------------------
// Test the accessing of memory details
// -----------------------------------------------------------------
void test_memoryDetails(void)
{
    int32_t rc;
    char *outputString = NULL;

    rc =  ism_engine_diagnostics(ediaVALUE_MODE_MEMORYDETAILS,
                                 NULL,
                                 &outputString,
                                 NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(outputString);

    // Check that it's parsable
    ism_json_parse_t parseObj;
    ism_json_entry_t ents[10];

    memset(&parseObj, 0, sizeof(parseObj));

    parseObj.ent = ents;
    parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseObj.source = strdup(outputString);
    parseObj.src_len = strlen(parseObj.source);

    rc = ism_json_parse(&parseObj);
    TEST_ASSERT_EQUAL(rc, OK);

    // Could extract some values here potentially

    if (parseObj.free_ent) ism_common_free(ism_memory_utils_parser,parseObj.ent);
    free(parseObj.source);

    ism_engine_freeDiagnosticsOutput(outputString);
}

// -----------------------------------------------------------------
// Test the counting of durable owner objects
// -----------------------------------------------------------------
void test_ownerCounts(uint32_t *clientCount, uint32_t *subCount)
{
    int32_t rc;
    char *outputString = NULL;

    rc =  ism_engine_diagnostics(ediaVALUE_MODE_OWNERCOUNTS,
                                 NULL,
                                 &outputString,
                                 NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(outputString);

    // Extract the contents of the output

    ism_json_parse_t parseObj;
    ism_json_entry_t ents[10];

    memset(&parseObj, 0, sizeof(parseObj));

    parseObj.ent = ents;
    parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseObj.source = strdup(outputString);
    parseObj.src_len = strlen(parseObj.source);

    rc = ism_json_parse(&parseObj);
    TEST_ASSERT_EQUAL(rc, OK);

    *clientCount = (uint32_t)(ism_json_getInt(&parseObj, ediaVALUE_OUTPUT_CLIENTOWNERCOUNT, -1));
    *subCount = (uint32_t)(ism_json_getInt(&parseObj, ediaVALUE_OUTPUT_SUBOWNERCOUNT, -1));

    if (parseObj.free_ent) ism_common_free(ism_memory_utils_parser,parseObj.ent);
    free(parseObj.source);

    ism_engine_freeDiagnosticsOutput(outputString);
}

// -----------------------------------------------------------------
// Test the Dumping of client states
// -----------------------------------------------------------------
static void clientStateStealCallback(int32_t reason,
                                     ismEngine_ClientStateHandle_t hClient,
                                     uint32_t options,
                                     void *pContext)
{
    // DO NOTHING - test will clean up
}

static void callbackCreateClientStateComplete(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext)
{
    ismEngine_ClientStateHandle_t **ppHandle = (ismEngine_ClientStateHandle_t **)pContext;
    ismEngine_ClientStateHandle_t *pHandle = *ppHandle;

    if (retcode != OK) TEST_ASSERT_EQUAL(retcode, ISMRC_ResumedClientState);

    *pHandle = handle;
}

#define TEST_CSDUMP_NAME "testCSDump.enc"
#define TEST_CSDUMP_PWORD "password"
void test_dumpClientStates(void)
{
    int32_t rc;
    char *outputString = NULL;

    //Initialise OpenSSL
    sslInit();

    ismEngine_ClientStateHandle_t hZombie[20] = {NULL};
    ismEngine_ClientStateHandle_t hClient[500] = {NULL};
    ismEngine_ClientStateHandle_t hThief[100] = {NULL};

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    TEST_ASSERT_PTR_NOT_NULL(pThreadData);

    char clientId[50];
    uint32_t randomPart = rand()%65535;
    uint32_t options;

    int32_t i;
    int32_t createdZombies = 0;
    int32_t createdClients = 0;
    int32_t createdThieves = 0;

    // Create Zombies
    for(; createdZombies<(sizeof(hZombie)/sizeof(hZombie[0])); createdZombies++)
    {
        ismEngine_ClientStateHandle_t *phZombie = &hZombie[createdZombies];
        options = ismENGINE_CREATE_CLIENT_OPTION_DURABLE;

        sprintf(clientId, "Zombie_%u_%d", randomPart, createdZombies+1);

        rc = ism_engine_createClientState(clientId,
                                          PROTOCOL_ID_MQTT,
                                          options,
                                          NULL, NULL, NULL,
                                          phZombie,
                                          &phZombie, sizeof(phZombie), callbackCreateClientStateComplete);
        if (rc != ISMRC_AsyncCompletion) TEST_ASSERT_EQUAL(rc, OK);
    }

    // Wait for all the (future) zombies to be created
    for(i=0; i<createdZombies; i++)
    {
        if (hZombie[i] == NULL)
        {
            sched_yield();
            i = 0;
        }
    }

    // Take this opportunity -- while we have some owners -- to test OwnerCounts code
    uint32_t clientOwnerCount = 0;
    uint32_t subOwnerCount = 0;
    uint32_t expectClientOwnerCount = 20;

    test_ownerCounts(&clientOwnerCount, &subOwnerCount);

    TEST_ASSERT_EQUAL(clientOwnerCount, expectClientOwnerCount);
    TEST_ASSERT_EQUAL(subOwnerCount, 0);

    // Make them zombies
    for(i=0; i<createdZombies; i++)
    {
        rc = sync_ism_engine_destroyClientState(hZombie[i],
                                                ismENGINE_DESTROY_CLIENT_OPTION_NONE);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Create Clients & Thieves
    for(; createdClients<(sizeof(hClient)/sizeof(hClient[0])); createdClients++)
    {
        ismEngine_ClientStateHandle_t *phClient = &hClient[createdClients];
        options = ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL;

        if (rand()%10 >= 5)
        {
            options |= ismENGINE_CREATE_CLIENT_OPTION_DURABLE;
            expectClientOwnerCount += 1;
        }
        else
        {
            options |= ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART;
        }

        sprintf(clientId, "Client_%u_%08x_%d", randomPart, options, createdClients+1);

        rc = ism_engine_createClientState(clientId,
                                          PROTOCOL_ID_MQTT,
                                          options,
                                          phClient,
                                          clientStateStealCallback,
                                          NULL,
                                          phClient,
                                          &phClient, sizeof(phClient), callbackCreateClientStateComplete);
        if (rc != ISMRC_AsyncCompletion) TEST_ASSERT_EQUAL(rc, OK);

        // Add a thief for a subset
        if (createdClients < (sizeof(hThief)/sizeof(hThief[0])))
        {
            ismEngine_ClientStateHandle_t *phThief = &hThief[createdClients];
            rc = ism_engine_createClientState(clientId,
                                              PROTOCOL_ID_MQTT,
                                              options,
                                              phClient,
                                              clientStateStealCallback,
                                              NULL,
                                              phThief,
                                              &phThief, sizeof(phThief), callbackCreateClientStateComplete);
            if (rc != ISMRC_AsyncCompletion) TEST_ASSERT_EQUAL(rc, OK);
            createdThieves++;
        }
    }

    // Wait for all the clients to be created
    for(i=0; i<createdClients; i++)
    {
        if (hClient[i] == NULL)
        {
            sched_yield();
            i = 0;
        }
    }

    // Get the information
    rc =  ism_engine_diagnostics(ediaVALUE_MODE_DUMPCLIENTSTATES,
                                 "   "TEST_CSDUMP_NAME"\t  "TEST_CSDUMP_PWORD" ",
                                 &outputString,
                                 NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(outputString);

    // printf("OUTPUT: %s\n", outputString);

    // Check the output string.
    ism_json_parse_t parseObj;
    ism_json_entry_t ents[10];

    memset(&parseObj, 0, sizeof(parseObj));

    parseObj.ent = ents;
    parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseObj.source = strdup(outputString);
    parseObj.src_len = strlen(parseObj.source);

    rc = ism_json_parse(&parseObj);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(parseObj.ent_count, 5);
    TEST_ASSERT_EQUAL(parseObj.ent[0].objtype, JSON_Object);
    TEST_ASSERT_EQUAL(parseObj.ent[0].count, 4);

    const char *outputFilePath = NULL;

    for(int32_t x=1; x<parseObj.ent_count; x++)
    {
        ism_json_entry_t *entry = &parseObj.ent[x];

        if (strcmp(entry->name, "rc") == 0)
        {
            TEST_ASSERT_EQUAL(entry->objtype, JSON_Integer);
            TEST_ASSERT_EQUAL(entry->count, 0);
        }
        else if (strcmp(entry->name, "TotalClientCount") == 0)
        {
            TEST_ASSERT_EQUAL(entry->objtype, JSON_Integer);
            TEST_ASSERT_EQUAL(entry->count, createdClients+createdThieves+createdZombies+3); // +3 for the shared namespaces
        }
        else if (strcmp(entry->name, "ChainCount") == 0)
        {
            TEST_ASSERT_EQUAL(entry->objtype, JSON_Integer);
        }
        else if (strcmp(entry->name, "FilePath") == 0)
        {
            TEST_ASSERT_EQUAL(entry->objtype, JSON_String);
            outputFilePath = entry->value;
        }
    }
    TEST_ASSERT_PTR_NOT_NULL(outputFilePath);
    TEST_ASSERT_PTR_NOT_NULL(strstr(outputFilePath, TEST_CSDUMP_NAME));

    test_checkDumpClientStatesFile(outputFilePath, TEST_CSDUMP_PWORD, checkFailFunc, displayType_NONE);

    if (parseObj.free_ent) ism_common_free(ism_memory_utils_parser,parseObj.ent);
    free(parseObj.source);

    ism_engine_freeDiagnosticsOutput(outputString);

    // Check the counts again
    test_ownerCounts(&clientOwnerCount, &subOwnerCount);

    TEST_ASSERT_EQUAL(clientOwnerCount, expectClientOwnerCount);
    TEST_ASSERT_EQUAL(subOwnerCount, 0);

    for(i=0; i<createdClients; i++)
    {
        rc = sync_ism_engine_destroyClientState(hClient[i],
                                                ismENGINE_DESTROY_CLIENT_OPTION_NONE);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Wait for all the thieves...
    for(i=0; i<(sizeof(hThief)/sizeof(hThief[0])); i++)
    {
        if (hThief[i] == NULL)
        {
            sched_yield();
            i = 0;
        }
    }

    // Destroy all the thieves
    for(i--; i>=0; i--)
    {
        rc = sync_ism_engine_destroyClientState(hThief[i],
                                                ismENGINE_DESTROY_CLIENT_OPTION_NONE);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Destroy all zombies
    for(i=0; i<createdZombies; i++)
    {
        sprintf(clientId, "Zombie_%u_%d", randomPart, i+1);

        rc = ism_engine_destroyDisconnectedClientState(clientId, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    sslTerm();
}

// -----------------------------------------------------------------
// Test some unusual actions on a diagnostics file
// -----------------------------------------------------------------
void test_diagFileActions(void)
{
    int32_t rc;

    //Initialise OpenSSL
    sslInit();

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    char *filePath = NULL;
    ieieEncryptedFileHandle_t diagFile = NULL;

    rc = edia_createEncryptedDiagnosticFile(pThreadData,
                                            &filePath,
                                            "test",
                                            "testFile",
                                            "testPassword",
                                            &diagFile);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(diagFile);
    TEST_ASSERT_PTR_NOT_NULL(filePath);

    rc = edia_createEncryptedDiagnosticFile(pThreadData,
                                            &filePath,
                                            "test",
                                            "testFile",
                                            "testPassword",
                                            &diagFile);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(diagFile);
    TEST_ASSERT_PTR_NOT_NULL(filePath);

    iemem_free(pThreadData, iemem_diagnostics, filePath);

    sslTerm();
}

// -----------------------------------------------------------------
// Display a diagnostic file
// -----------------------------------------------------------------
void justDisplayThisFile(const char *filePath, const char *password, const char *displayString)
{
    int32_t rc;

    rc = test_engineInit(true, true,
                         true, // Disable Auto creation of named queues
                         false, /*recovery should complete ok*/
                         ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                         testDEFAULT_STORE_SIZE);
    if (rc != OK) abort();

    //Initialise OpenSSL
    sslInit();

    displayType display = displayType_RAW;

    if (strcasecmp(displayString, "RAW") == 0)
    {
        display = displayType_RAW;
    }
    else if (strcasecmp(displayString, "PARSED") == 0)
    {
        display = displayType_PARSED;
    }

    printf("\n");
    test_checkDumpClientStatesFile(filePath, password, NULL, display);
    printf("\n");

    sslTerm();

    rc = test_engineTerm(true);

    if (rc != OK) abort();
}

/* Check the contents of the export file specified are as we expect */
void test_checkTraceDumpFile(ieutThreadData_t *pThreadData, const char *filePath, const char *filePassword,
                             uint32_t *pNumThreads, uint32_t *pNumThreadsTraced)
{
    int32_t rc;
    uint64_t dataId;
    size_t dataLen = 0;
    ieieDataType_t dataType;
    ieieDataType_t prevDataType = ieieDATATYPE_LAST; //set to an invalid type when we haven't read a prev yet
    void *inData = NULL;
    uint32_t typeCount[ieieDATATYPE_LAST] = {0};

    ieieEncryptedFileHandle_t inFile = ieie_OpenEncryptedFile(pThreadData, iemem_exportResources, filePath, filePassword);

    TEST_ASSERT_PTR_NOT_NULL(inFile);

    do
    {
        rc = ieie_importData(pThreadData, inFile,  &dataType, &dataId, &dataLen, (void **)&inData);

        if (rc == OK)
        {
            TEST_ASSERT_GREATER_THAN(ieieDATATYPE_LAST, dataType);
            typeCount[dataType] += 1;

            switch(dataType)
            {
                case ieieDATATYPE_TRACEDUMPHEADER:
                    //Shouldn't have read any other records yet
                    TEST_ASSERT_EQUAL(prevDataType, ieieDATATYPE_LAST);
                    break;

                case ieieDATATYPE_TRACEDUMPFOOTER:
                    break;

                case ieieDATATYPE_TRACEDUMPTHREADHEADER:
                    TEST_ASSERT_EQUAL(typeCount[ieieDATATYPE_TRACEDUMPFOOTER], 0);
                    break;

                case ieieDATATYPE_TRACEDUMPTHREADIDENTS:
                    TEST_ASSERT_EQUAL(prevDataType, ieieDATATYPE_TRACEDUMPTHREADHEADER);
                    break;

                case ieieDATATYPE_TRACEDUMPTHREADVALUES:
                    TEST_ASSERT_EQUAL(prevDataType, ieieDATATYPE_TRACEDUMPTHREADIDENTS);
                    break;

                default:
                    //We don't expect other types
                    TEST_ASSERT_EQUAL(1, 0);
                    break;
            }

            prevDataType = dataType;
        }
    }
    while(rc == OK);

    TEST_ASSERT_EQUAL(rc, ISMRC_EndOfFile);
    TEST_ASSERT_EQUAL(1, typeCount[ieieDATATYPE_TRACEDUMPHEADER]);
    TEST_ASSERT_EQUAL(1, typeCount[ieieDATATYPE_TRACEDUMPFOOTER]);
    TEST_ASSERT_EQUAL(typeCount[ieieDATATYPE_TRACEDUMPTHREADIDENTS]
                     ,typeCount[ieieDATATYPE_TRACEDUMPTHREADVALUES]);

    ieie_freeReadExportedData(pThreadData, iemem_exportResources, inData);

    *pNumThreads        = typeCount[ieieDATATYPE_TRACEDUMPTHREADHEADER];
    *pNumThreadsTraced  = typeCount[ieieDATATYPE_TRACEDUMPTHREADIDENTS];

    rc = ieie_finishReadingEncryptedFile(pThreadData, inFile);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_dumpTraceHistory(void)
{
    //Initialise OpenSSL
    sslInit();

    //Could do something like this:
    //iest_storeEventHandler(ISM_STORE_EVENT_CBQ_ALERT_ON, NULL);
    //iest_storeEventHandler(ISM_STORE_EVENT_CBQ_ALERT_OFF, NULL);
    //But would then have to find and validate file

    char *testPassword = "testPassword";
    char *testFilename = "test_InMemoryTraceDump_testfile";

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    TEST_ASSERT_PTR_NOT_NULL(pThreadData);

    // Go in via the ism_engine_diagnostics function (to test that layer)
    char requestString[strlen(testPassword)+strlen(testFilename)+2];
    char *outputString = NULL;

    sprintf(requestString, "%s %s", testFilename, testPassword);
    int32_t rc = ism_engine_diagnostics(ediaVALUE_MODE_DUMPTRACEHISTORY,
                                        requestString,
                                        &outputString,
                                        NULL, 0, NULL);
#if (ieutTRACEHISTORY_BUFFERSIZE > 0)
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(outputString);

    ism_json_parse_t parseObj;
    ism_json_entry_t ents[10];

    memset(&parseObj, 0, sizeof(parseObj));

    parseObj.ent = ents;
    parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseObj.source = strdup(outputString);
    parseObj.src_len = strlen(parseObj.source);

    rc = ism_json_parse(&parseObj);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(parseObj.ent_count, 2);
    TEST_ASSERT_EQUAL(parseObj.ent[0].objtype, JSON_Object);
    TEST_ASSERT_EQUAL(parseObj.ent[0].count, 1);

    const char *outputFilePath = NULL;

    for(int32_t x=1; x<parseObj.ent_count; x++)
    {
        ism_json_entry_t *entry = &parseObj.ent[x];

        if (strcmp(entry->name, "FilePath") == 0)
        {
            TEST_ASSERT_EQUAL(entry->objtype, JSON_String);
            outputFilePath = entry->value;
        }
    }

    TEST_ASSERT_PTR_NOT_NULL(outputFilePath);
    TEST_ASSERT_PTR_NOT_NULL(strstr(outputFilePath, testFilename));

    uint32_t numThreads = 0;
    uint32_t numThreadsTraced = 0;

    test_checkTraceDumpFile( pThreadData
                           , outputFilePath, testPassword
                           , &numThreads, &numThreadsTraced);

    //We expect all threads to have output some tracepoints:
    TEST_ASSERT_EQUAL(numThreads, numThreadsTraced);

    //Expect some threads to have traced
    TEST_ASSERT_GREATER_THAN(numThreads, 0);

    ism_engine_freeDiagnosticsOutput(outputString);

    if (parseObj.free_ent) ism_common_free(ism_memory_utils_parser,parseObj.ent);
    free(parseObj.source);
#else
    TEST_ASSERT_EQUAL(rc, ISMRC_NotImplemented);
    TEST_ASSERT_PTR_NULL(outputString);
#endif

    // Call the internal in-memory function with no filename to test that
    char *filePath = NULL;

    rc = ietd_dumpInMemoryTrace( pThreadData, NULL, NULL, &filePath);
    TEST_ASSERT_EQUAL(rc, OK);

    iemem_free(pThreadData, iemem_diagnostics, filePath);

    sslTerm();

    // Test calling without SSL initialized
    filePath = NULL;

    rc = ietd_dumpInMemoryTrace( pThreadData, NULL, NULL, &filePath);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    //In Openssl 1.1 this does not fail as resources allocated and freed automatically:
    //https://www.openssl.org/docs/man1.1.0/crypto/OPENSSL_init_crypto.html
    TEST_ASSERT_NOT_EQUAL(rc, OK);
#endif
}

CU_TestInfo ISM_Diagnostics_CUnit_test_Basic[] =
{
    { "testInvalid", test_invalid },
    { "testEcho", test_echo },
    { "testMemoryDetails", test_memoryDetails },
    { "testDumpClientStates", test_dumpClientStates },
    { "testDiagFileActions", test_diagFileActions },
    { "testInMemoryTraceDump", test_dumpTraceHistory },
    CU_TEST_INFO_NULL
};

int initSuite(void)
{
    return test_engineInit(true, true,
                           true, // Disable Auto creation of named queues
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termSuite(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_Diagnostics_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("Basic", initSuite, termSuite, ISM_Diagnostics_CUnit_test_Basic),
    CU_SUITE_INFO_NULL
};

int setup_CU_registry(int argc, char **argv, CU_SuiteInfo *allSuites)
{
    CU_SuiteInfo *tempSuites = NULL;

    int retval = 0;

    if (argc > 1)
    {
        int suites = 0;

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
            else if (!strcasecmp(argv[i], "verbose"))
            {
                CU_basic_set_mode(CU_BRM_VERBOSE);
            }
            else if (!strcasecmp(argv[i], "silent"))
            {
                CU_basic_set_mode(CU_BRM_SILENT);
            }
            else
            {
                bool suitefound = false;
                int index = atoi(argv[i]);
                int totalSuites = 0;

                CU_SuiteInfo *curSuite = allSuites;

                while(curSuite->pTests)
                {
                    if (!strcasecmp(curSuite->pName, argv[i]))
                    {
                        suitefound = true;
                        break;
                    }

                    totalSuites++;
                    curSuite++;
                }

                if (!suitefound)
                {
                    if (index > 0 && index <= totalSuites)
                    {
                        curSuite = &allSuites[index-1];
                        suitefound = true;
                    }
                }

                if (suitefound)
                {
                    tempSuites = realloc(tempSuites, sizeof(CU_SuiteInfo) * (suites+2));
                    memcpy(&tempSuites[suites++], curSuite, sizeof(CU_SuiteInfo));
                    memset(&tempSuites[suites], 0, sizeof(CU_SuiteInfo));
                }
                else
                {
                    printf("Invalid test suite '%s' specified, the following are valid:\n\n", argv[i]);

                    index=1;

                    curSuite = allSuites;

                    while(curSuite->pTests)
                    {
                        printf(" %2d : %s\n", index++, curSuite->pName);
                        curSuite++;
                    }

                    printf("\n");

                    retval = 99;
                    break;
                }
            }
        }
    }

    if (retval == 0)
    {
        if (tempSuites)
        {
            CU_register_suites(tempSuites);
        }
        else
        {
            CU_register_suites(allSuites);
        }
    }

    if (tempSuites) free(tempSuites);

    return retval;
}

int main(int argc, char *argv[])
{
    int trclvl = 0;
    int retval = 0;
    int testLogLevel = 2;

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    // If we've been passed 2 or more parameters, we will interpret this as
    // a request to display a file (<FILEPATH> <PASSWORD> [RAW|PARSED]).
    //
    // This is really only here as a stop-gap to give us some way of displaying
    // an encrypted diagnostics file, really we need a dedicated (standalone)
    // program to do it.
    if (argc > 2)
    {
        const char *filePath = argv[1];
        const char *password = argv[2];
        const char *displayString;

        if (argc > 3)
        {
            displayString = argv[3];
        }
        else
        {
            displayString = "";
        }

        justDisplayThisFile(filePath, password, displayString);
    }
    else
    {
        CU_initialize_registry();

        retval = setup_CU_registry(argc, argv, ISM_Diagnostics_CUnit_allsuites);

        if (retval == 0)
        {
            CU_basic_run_tests();

            CU_RunSummary * CU_pRunSummary_Final;
            CU_pRunSummary_Final = CU_get_run_summary();

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
    }

    test_processTerm(retval == 0);

mod_exit:

    return retval;
}
