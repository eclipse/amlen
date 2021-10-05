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
/* Module Name: test_diagnosticsFileContent.c                        */
/*                                                                   */
/* Description: Check the contents of diagnostic files               */
/*                                                                   */
/*********************************************************************/
#include <stdbool.h>

#include "test_diagnosticsFileContent.h"
#include "engineDiag.h"
#include "test_utils_task.h"

#define FAIL(_fn, _info) _fn(__func__, __LINE__, (unsigned long int)_info)

void test_localFailFunc(const char *function, int lineNumber, unsigned long int info)
{
    printf("Failure in %s line %d [info=%lu]\n", function, lineNumber, info);
    abort();
}

// -----------------------------------------------------------------
// Check the contents of a file from dumping client states
// -----------------------------------------------------------------
void test_checkDumpClientStatesFile(const char *filePath,
                                    const char *password,
                                    checkDiagnosticFile_AssertFail_t failFunc,
                                    displayType display)
{
    int32_t rc = OK;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    if (failFunc == NULL) failFunc = test_localFailFunc;

    // Open the encrypted file and check contents
    ieieEncryptedFileHandle_t inputFile = ieie_OpenEncryptedFile(pThreadData,
                                                                 iemem_diagnostics,
                                                                 filePath,
                                                                 password);
    if (inputFile == NULL) FAIL(failFunc, 0);

    uint64_t inDataId = 0;
    size_t inDataLen =0;
    ieieDataType_t inDataType;
    void *inData = NULL;

    ism_json_parse_t parseObj;
    ism_json_entry_t ents[50];

    bool seenHeader = false;
    bool seenFooter = false;
    uint32_t seenChains = 0;
    uint32_t seenClients = 0;
    uint64_t lastChainNumber = 0;
    uint64_t headerId = 0;
    do
    {
        rc = ieie_importData(pThreadData, inputFile, &inDataType, &inDataId, &inDataLen, &inData);

        if (rc != OK)
        {
            if (rc != ISMRC_EndOfFile) FAIL(failFunc, rc);
        }
        else
        {
            // Check the order of expected data types & Ids
            switch(inDataType)
            {
                case ieieDATATYPE_DIAGCLIENTDUMPHEADER:
                    if (seenHeader != false) FAIL(failFunc, seenHeader);
                    if (seenFooter != false) FAIL(failFunc, seenFooter);
                    if (seenChains != 0) FAIL(failFunc, seenChains);
                    seenHeader = true;
                    headerId = inDataId;
                    break;
                case ieieDATATYPE_DIAGCLIENTDUMPCHAIN:
                    if (seenHeader != true) FAIL(failFunc, seenHeader);
                    if (seenFooter == true) FAIL(failFunc, seenFooter);
                    seenChains += 1;
                    if (lastChainNumber >= inDataId) FAIL(failFunc, inDataId); // Expect chain in order
                    lastChainNumber = inDataId;
                    break;
                case ieieDATATYPE_DIAGCLIENTDUMPFOOTER:
                    if (seenHeader != true) FAIL(failFunc, seenHeader);
                    if (seenFooter != false) FAIL(failFunc, seenFooter);
                    seenFooter = true;
                    if (inDataId != headerId) FAIL(failFunc, inDataId);
                    break;
                default:
                    FAIL(failFunc, inDataType);
                    break;
            }

            char *inString = (char *)inData;

            // Should be NULL terminated strings
            if (inString[inDataLen-1] != '\0') FAIL(failFunc, inDataLen);

            if (display == displayType_RAW) printf("%s\n", inString);

            memset(&parseObj, 0, sizeof(parseObj));

            parseObj.ent = ents;
            parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
            parseObj.source = inString;
            parseObj.src_len = inDataLen-1;

            rc = ism_json_parse(&parseObj);
            if (rc != OK) FAIL(failFunc, rc);
            if (parseObj.ent[0].objtype != JSON_Object) FAIL(failFunc, parseObj.ent[0].objtype);

            if (inDataType == ieieDATATYPE_DIAGCLIENTDUMPHEADER)
            {
                const char *startTimeString = ism_json_getString(&parseObj, "StartTime");
                if (startTimeString == NULL) FAIL(failFunc, startTimeString);
                uint64_t startTime = strtoul(startTimeString, NULL, 10);
                if (startTime != headerId) FAIL(failFunc, startTime);
                uint32_t tableGeneration = (uint32_t)ism_json_getInt(&parseObj, "TableGeneration", 0);

                if (display == displayType_PARSED)
                {
                    printf("Header\n");
                    printf("  StartTime: %lu\n", startTime); // TODO: Display as a timestamp
                    printf("  TableGeneration: %u\n", tableGeneration);
                }
            }
            else if (inDataType == ieieDATATYPE_DIAGCLIENTDUMPFOOTER)
            {
                const char *endTimeString = ism_json_getString(&parseObj, "EndTime");
                if (endTimeString == NULL) FAIL(failFunc, endTimeString);
                uint64_t endTime = strtoul(endTimeString, NULL, 10);
                uint32_t chainCount = (uint32_t)ism_json_getInt(&parseObj, "ChainCount", 0);
                uint32_t emptyChainCount = (uint32_t)ism_json_getInt(&parseObj, "EmptyChainCount", 0);
                if (chainCount-emptyChainCount != seenChains) FAIL(failFunc, (chainCount-emptyChainCount));
                uint32_t totalClientCount = (uint32_t)ism_json_getInt(&parseObj, "TotalClientCount", 0);
                if (totalClientCount != seenClients) FAIL(failFunc, totalClientCount);

                if (display == displayType_PARSED)
                {
                    printf("Footer\n");
                    printf("  EndTime: %lu\n", endTime); // TODO: Display as timestamp
                    printf("  ChainCount: %u\n", chainCount);
                    printf("  EmptyChainCount: %u\n", emptyChainCount);
                    printf("  TotalClientCount: %u\n", totalClientCount);
                }
            }
            else
            {
                if (inDataType != ieieDATATYPE_DIAGCLIENTDUMPCHAIN) FAIL(failFunc, inDataType);
                uint32_t clientCount = (uint32_t)ism_json_getInt(&parseObj, "ClientCount", 0);
                if (display == displayType_PARSED)
                {
                    printf("Chain %lu\n", inDataId);
                    printf("  ClientCount: %u\n", clientCount);
                }
                int arrayEnt = ism_json_get(&parseObj, 0, "Entries");
                if (arrayEnt == -1) FAIL(failFunc, inDataId);
                ism_json_entry_t *entry = &parseObj.ent[arrayEnt];
                if (entry->objtype != JSON_Array) FAIL(failFunc, entry->objtype);
                int arrayCount = entry->count;
                uint32_t arrayClients = 0;
                for(int i=0; i<arrayCount; i++)
                {
                    entry++;
                    if (entry->objtype == JSON_Object)
                    {
                        seenClients++;
                        if (display == displayType_PARSED)
                        {
                            printf("   Client #%u\n", seenClients);
                        }

                        arrayClients++;
                        int objectCount = entry->count;
                        for(int j=0; j<objectCount; j++)
                        {
                            entry++;
                            if (entry->name == NULL) FAIL(failFunc, arrayEnt+i+j);
                            if (display == displayType_PARSED)
                            {
                                if (entry->objtype == JSON_String) printf("    %s: '%s'\n", entry->name, entry->value);
                                else if (entry->objtype == JSON_Integer) printf("    %s: %d\n", entry->name, entry->count);
                                else if (entry->objtype == JSON_True) printf("    %s\n", entry->name);
                                else if (entry->objtype == JSON_False) printf("    !%s\n", entry->name);
                                else printf("    %s [unhandled type %d]\n", entry->name, entry->objtype);
                            }
                        }
                        i += objectCount;
                    }
                }
                if (arrayClients != clientCount) FAIL(failFunc, arrayClients);
            }

            if (parseObj.free_ent) ism_common_free(ism_memory_utils_parser,parseObj.ent);
        }
    }
    while(rc == OK);
    if (rc != ISMRC_EndOfFile) FAIL(failFunc, rc);

    ieie_freeReadExportedData(pThreadData, iemem_diagnostics, inData);
    rc = ieie_finishReadingEncryptedFile(pThreadData, inputFile);
    if (rc != OK) FAIL(failFunc, rc);
}
