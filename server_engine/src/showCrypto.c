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
/* Module Name: showCrypto.c                                         */
/*                                                                   */
/* Description: Main source file for utility to display contents of  */
/*              an encrypted output file.                            */
/*                                                                   */
/*********************************************************************/

// To take an encrypted engine trace dump and turn it into formatted trace history:
//
// showCrypto <ENCRYPTED_FILE> <PASSWORD> trcDump | go run trchistfmt.go server_engine/src/*[ch] server_engine/src/export/*[ch]
//

#include <string.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include "engine.h"
#include "engineInternal.h"
#include "engineDiag.h"
#include "engineTraceDump.h"
#include "memHandler.h"
#include "exportClientState.h" //for showCryptoProcessing_UNRELEASEDMSGSTATES

#if OPENSSL_VERSION_NUMBER < 0x10100000L
//Initialise OpenSSL
static inline void sslInit(void)
{
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_config(NULL);
}
#else
#define sslInit()
#endif

#if OPENSSL_VERSION_NUMBER < 0x10100000L
//Terminate OpenSSL
static inline void sslTerm(void)
{
    EVP_cleanup();
    ERR_free_strings();
}
#else
#define sslTerm()
#endif

typedef enum tag_showCryptoProcessingType_t
{
    showCryptoProcessing_DEFAULT,
    showCryptoProcessing_TRACEDUMP,
    showCryptoProcessing_UNRELEASEDMSGSTATES, //Locking for exported that that will trigger bug RTC244910
} showCryptoProcessingType_t;

// Function to call if the reading of an encrypted file failed
static void checkFailFunc(const char *function, int lineNumber, unsigned long int info)
{
    printf("Failure in %s line %d [info=%lu]\n", function, lineNumber, info);
    abort();
}

// Replacement functions for memHandler
void *iemem_malloc(ieutThreadData_t *pThreadData, uint32_t probe, size_t size)
{
    return ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,5),size);
}

void iemem_free(ieutThreadData_t *pThreadData, iemem_memoryType type, void *mem)
{
    ism_common_free(ism_memory_engine_misc,mem);
}

void *iemem_calloc(ieutThreadData_t *pThreadData, uint32_t probe, size_t nmemb, size_t size)
{
    return ism_common_calloc(ISM_MEM_PROBE(ism_memory_engine_misc,7),nmemb, size);
}

void *iemem_realloc(ieutThreadData_t *pThreadData, uint32_t probe, void *ptr, size_t size)
{
    return ism_common_realloc(ISM_MEM_PROBE(ism_memory_engine_misc,8),ptr, size);
}

void localTrace(int level, int opt, const char * file, int lineno, const char * fmt, ...)
{
    printf("%s:%d\n", file, lineno);
}

ism_common_TraceFunction_t traceFunction = localTrace;

#define FAIL(_fn, _inf) _fn(__func__, __LINE__, _inf);

// -----------------------------------------------------------------
// Show the contents of a diagnostic / encrypted file
// -----------------------------------------------------------------
void showEncryptedFileContents(const char *filePath,
                               const char *password,
                               showCryptoProcessingType_t processingType,
                               ieieDataType_t *typeArray)
{
    int32_t rc = OK;
    ieutThreadData_t *pThreadData = ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,9),sizeof(ieutThreadData_t));

    // Open the encrypted file and check contents
    ieieEncryptedFileHandle_t inputFile = ieie_OpenEncryptedFile(pThreadData,
                                                                 iemem_diagnostics,
                                                                 filePath,
                                                                 password);
    if (inputFile == NULL) FAIL(checkFailFunc, 0);

    uint64_t inDataId = 0;
    size_t inDataLen =0;
    ieieDataType_t inDataType;
    void *inData = NULL;
    ietdTraceDumpThreadHeader_t *pThreadHeader = NULL;
    uint64_t *pThreadIdents = NULL;

    uint32_t entryCount = 0;
    do
    {
        bool display = false;

        rc = ieie_importData(pThreadData, inputFile, &inDataType, &inDataId, &inDataLen, &inData);

        if (rc != OK)
        {
            if (rc != ISMRC_EndOfFile) FAIL(checkFailFunc, rc);
        }
        else if (typeArray == NULL)
        {
            display = true;
        }
        else
        {
printf("Checking type display\n");
            ieieDataType_t *thisType = typeArray;

            while(*thisType != 0)
            {
                if (*thisType == inDataType)
                {
                    display = true;
                    break;
                }
                thisType += 1;
            }
        }

        if (display)
        {
            if (processingType == showCryptoProcessing_TRACEDUMP)
            {
                if (inDataType == ieieDATATYPE_TRACEDUMPHEADER)
                {
                    ietdTraceDumpFileHeader_t *pHeader = inData;

                    if (inDataLen != sizeof(ietdTraceDumpFileHeader_t))
                    {
                        printf("TraceDump version mismatch found %u expected %u. Switching to default display mode.\n",
                               pHeader->Version, (unsigned int)IETD_TRACEDUMP_FILEHEADER_CURRENT_VERSION);
                        processingType = showCryptoProcessing_DEFAULT;
                    }
                }
                else if (inDataType == ieieDATATYPE_TRACEDUMPTHREADHEADER)
                {
                    ietdTraceDumpThreadHeader_t *pNewThreadHeader = inData;

                    if (pThreadHeader == NULL)
                    {
                        pThreadHeader = ism_common_calloc(ISM_MEM_PROBE(ism_memory_engine_misc,10),1, sizeof(*pThreadHeader));
                    }

                    if (pNewThreadHeader->numTracePoints != pThreadHeader->numTracePoints)
                    {
                        pThreadIdents = ism_common_realloc(ISM_MEM_PROBE(ism_memory_engine_misc,11),pThreadIdents, sizeof(*pThreadIdents) * pNewThreadHeader->numTracePoints);
                    }

                    memcpy(pThreadHeader, pNewThreadHeader, sizeof(*pThreadHeader));

                    bool isActive = 1; // TODO: This should come from the thread header

                    printf("NEWTHREAD-%s: %p\n", isActive ? "active" : "saved", (void *)(pThreadHeader->pThreadData));

                    if (pThreadHeader->numTracePoints == 0)
                    {
                        printf("ENDTHREAD: %p\n", (void *)(pThreadHeader->pThreadData));
                    }
                }
                else if (inDataType == ieieDATATYPE_TRACEDUMPTHREADIDENTS)
                {
                    assert(pThreadIdents != NULL);
                    assert(pThreadHeader->numTracePoints * sizeof(*pThreadIdents) == inDataLen);

                    memcpy(pThreadIdents, inData, inDataLen);
                }
                else if (inDataType == ieieDATATYPE_TRACEDUMPTHREADVALUES)
                {
                    uint64_t *pThreadValues = inData;

                    assert(pThreadHeader->numTracePoints * sizeof(*pThreadValues) == inDataLen);

                    uint64_t startTracePoint = pThreadHeader->startBufferPos % pThreadHeader->numTracePoints;
                    uint64_t endDirtyTracePoint = pThreadHeader->endBufferPos % pThreadHeader->numTracePoints;

                    uint64_t currTracePoint = startTracePoint;

                    // If the BufferPos before and after taking the dump don't match, we need to consider
                    // anything between the two trace points as dirty (we can't be sure it's right)
                    char dirtyTraceIndicator = (startTracePoint != endDirtyTracePoint) ? '*' : ' ';

                    do
                    {
                        if (pThreadIdents[currTracePoint] != 0)
                        {
                            printf("TP[%p,%lu]: %lu %lu %c\n",
                                   (void *)pThreadHeader->pThreadData,
                                   currTracePoint,
                                   pThreadIdents[currTracePoint],
                                   pThreadValues[currTracePoint],
                                   dirtyTraceIndicator);
                        }

                        currTracePoint = (currTracePoint+1) % pThreadHeader->numTracePoints;

                        if (dirtyTraceIndicator != ' ' && (currTracePoint == endDirtyTracePoint))
                        {
                            dirtyTraceIndicator = ' ';
                        }
                    }
                    while(currTracePoint != startTracePoint);

                    printf("ENDTHREAD: %p\n", (void *)(pThreadHeader->pThreadData));
                }
            }
            else if (processingType == showCryptoProcessing_UNRELEASEDMSGSTATES)
            {
                //All we care about are clients and how many UMSs they have (as if they have >64 they
                //will trigger bug RTC244910
                if(inDataType == ieieDATATYPE_EXPORTEDCLIENTSTATE)
                {
                    uint32_t ClientRecordVersion;
                    ieieClientStateInfo_t clientInfo;
                    char *extraData = NULL;
                    bool parsed=false;

                    memcpy(&ClientRecordVersion, inData, sizeof(uint32_t));

                    switch(ClientRecordVersion)
                    {
                        case 3:
                           memcpy(&clientInfo, inData, sizeof(ieieClientStateInfo_t));
                           extraData = ((char *)inData) + sizeof(ieieClientStateInfo_t);
                           parsed=true;
                           break;

                        case 2:
                           memcpy(&clientInfo, inData, sizeof(ieieClientStateInfo_V2_t));
                           extraData = ((char *)inData) + sizeof(ieieClientStateInfo_V2_t);
                           parsed=true;
                           break;

                        case 1:
                            memcpy(&clientInfo, inData, sizeof(ieieClientStateInfo_V1_t));
                            extraData = ((char *)inData) + sizeof(ieieClientStateInfo_V1_t);
                            parsed=true;
                            break;

                        default:
                            printf("Found Client record with unknown version %u - not processing!\n",ClientRecordVersion);
                            break;
                    }

                    if (parsed)
                    {
                        printf("Client %.*s  UMS=%u %s\n",
                                clientInfo.ClientIdLength, extraData,
                                clientInfo.UMSCount,
                                (clientInfo.UMSCount>64 ? "Defect RTC244910 affected": ""));
                    }
                }
            }
            else if (processingType == showCryptoProcessing_DEFAULT)
            {
                printf("%c{\"Type\":%d, \"Id\":%lu, \"Length\":%lu, \"Data\":",
                       entryCount ? ',' : '[',
                       inDataType,
                       inDataId,
                       inDataLen);

                char *inString = (char *)inData;

                if (inString[0] == '{' || inString[0] == '\"' || inString[0] == '[')
                {
                    // Should be NULL terminated strings
                    assert(inString[inDataLen-1] == '\0');

                    printf("%s", inString);
                }
                else
                {
                    size_t i;
                    uint8_t *inBytes = (uint8_t *)inData;

                    for(i=0; i<inDataLen; i++)
                    {
                        printf("%c%d", i ? ',' : '[', (uint8_t)inBytes[i]);
                    }

                    if (i != 0) printf("]");
                }

                printf("}");
            }

            entryCount++;
        }
    }
    while(rc == OK);
    if (rc != ISMRC_EndOfFile) FAIL(checkFailFunc, rc);

    if (entryCount != 0)
    {
        if (processingType == showCryptoProcessing_DEFAULT)
        {
            printf("]\n");
        }
    }

    ieie_freeReadExportedData(pThreadData, iemem_diagnostics, inData);
    rc = ieie_finishReadingEncryptedFile(pThreadData, inputFile);
    if (rc != OK) FAIL(checkFailFunc, rc);

    ism_common_free(ism_memory_engine_misc,pThreadIdents);
    ism_common_free(ism_memory_engine_misc,pThreadHeader);
    ism_common_free(ism_memory_engine_misc,pThreadData);
}

int main(int argc, char *argv[])
{
    int retval = 0;

    if (argc < 3)
    {
        printf("USAGE: %s <FILEPATH> <PASSWORD> [trcDump] [TYPE]\n", argv[0]);
        retval = 1;
    }
    else
    {
        ieieDataType_t *typeArray = NULL;
        const char *filePath = argv[1];
        const char *password = argv[2];
        int typeStart;
        showCryptoProcessingType_t processingType;

        if (argc > 3 && strcmpi(argv[3], "trcDump") == 0)
        {
            processingType = showCryptoProcessing_TRACEDUMP;
            typeStart = 4;
        }
        else if (argc > 3 && strcmpi(argv[3], "ums") == 0)
        {
            processingType = showCryptoProcessing_UNRELEASEDMSGSTATES;
            typeStart = 4;
        }
        else
        {
            typeStart = 3;
            processingType = showCryptoProcessing_DEFAULT;
        }

        if (argc > typeStart)
        {
            typeArray = ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,15),(argc-typeStart-1)*sizeof(ieieDataType_t));

            if (typeArray != NULL)
            {
                int i;
                for(i=typeStart; i<argc; i++)
                {
                    typeArray[i-typeStart] = (ieieDataType_t)strtoul(argv[i], NULL, 10);
                }
                typeArray[i-typeStart]=0;
            }
        }

        sslInit();
        showEncryptedFileContents(filePath, password, processingType, typeArray);
        sslTerm();

        ism_common_free(ism_memory_engine_misc,typeArray);
    }

    return retval;
}
