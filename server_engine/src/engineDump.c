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

//****************************************************************************
/// @file  engineDump.c
/// @brief Engine component dumping functions
//****************************************************************************
#define TRACE_COMP Engine

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <stdarg.h>
#include <dlfcn.h>

#include "engineDump.h"
#include "engineInternal.h"
#include "engineHashTable.h"
#include "transaction.h"
#include "clientState.h"
#include "topicTree.h"          // iett_ functions
#include "queueNamespace.h"     // queue namespace
#include "queueCommon.h"        // ieq_ functions & constants
#include "simpQ.h"              // iesq_ functions
#include "intermediateQ.h"      // ieiq_ functions
#include "multiConsumerQ.h"     // iemq_ functions
#include "lockManager.h"        // ielm_ functions
#include "dumpFormat.h"         // iefm_ functions
#include "engineSplitList.h"    // Expiry split list information
#include "remoteServers.h"      // iers_ functions

// The mkstemp format string to use when creating temporary dump files
#define TEMPORARY_DUMP_FILE_PATH_FORMAT "/tmp/engineDump_XXXXXX"

#define MIN_RESULT_BUFFER_LENGTH (strlen(TEMPORARY_DUMP_FILE_PATH_FORMAT)+1)

//****************************************************************************
/// @brief  Write descriptions of engine structures to the specified file
///
/// @param[in]     file    The file pointer to which to write descriptions
//****************************************************************************
void iedm_writeDumpDescription(iedmDump_t *dump)
{
    iedmFile_t file = dump->fp;

    char versionString[200];
    sprintf(versionString, "%s %s %s",
            ism_common_getVersion(), ism_common_getBuildLabel(), ism_common_getBuildTime());

    ism_ts_t *ts = ism_common_openTimestamp(ISM_TZF_UTC);
    if (ts != NULL)
    {
        char timeStampBuffer[80];

        ism_common_setTimestamp(ts, ism_common_currentTimeNanos());
        ism_common_formatTimestamp(ts, timeStampBuffer, 80, 6, ISM_TFF_SPACE|ISM_TFF_ISO8601);

        iedm_writeDumpHeader(file, dump->detailLevel, timeStampBuffer, versionString);

        ism_common_closeTimestamp(ts);
    }
    else
    {
        iedm_writeDumpHeader(file, dump->detailLevel, "UNKNOWN", versionString);
    }

    // Add engine structure descriptions to the dump
    iedm_describe_ismEngine_Server_t(file);
    iedm_describe_ismEngine_ClientState_t(file);
    iedm_describe_ismEngine_Session_t(file);
    iedm_describe_ismEngine_Consumer_t(file);
    iedm_describe_ismEngine_Message_t(file);
    iedm_describe_ismEngine_Queue_t(file);
    iedm_describe_ieutHashTable_t(file);
    iedm_describe_ieutSplitList_t(file);
    iedm_describe_ieutSplitListLink_t(file);
    iedm_describe_ieutThreadData_t(file);

    // Add sub-components structure descriptions
    iett_dumpWriteDescriptions((iedmDumpHandle_t)dump); // TopicTree
    ieqn_dumpWriteDescriptions((iedmDumpHandle_t)dump); // QueueNamespace
    iepi_dumpWriteDescriptions((iedmDumpHandle_t)dump); // PolicyInfo
    iesq_dumpWriteDescriptions((iedmDumpHandle_t)dump); // SimpQ
    ieiq_dumpWriteDescriptions((iedmDumpHandle_t)dump); // IntermediateQ
    iemq_dumpWriteDescriptions((iedmDumpHandle_t)dump); // MultiConsumerQ
    ielm_dumpWriteDescriptions((iedmDumpHandle_t)dump); // LockManager
    ietr_dumpWriteDescriptions((iedmDumpHandle_t)dump); // Transactions
    iers_dumpWriteDescriptions((iedmDumpHandle_t)dump); // RemoteServers
}

//****************************************************************************
/// @brief  Open a dump file with the specified name
///
/// @param[in]     filePath       The file to be opened
/// @param[in]     detailLevel    The detail level for the required file
/// @param[in]     userDataBytes  Max bytes of user data (per area) to write
/// @param[in]     dump           Pointer to dump
///
/// @remark If the filePath contains an empty string, we will create a temporary
///         file to contain the output. If that is the case, the buffer it points to
///         must be at least as long as MIN_RESULT_BUFFER_LENGTH, so that the
///         temporary file path can be written back to it.
///
/// @remark If the dump file is successfully opened, we immediately write out
///         descriptions of each of the engine structures to that file, meaning
///         that the contents are self describing.
///
/// @remark Note we do not make use of the iemem_* functions for memory allocation
///         in order to avoid being blocked by the memory limitations the engine
///         imposes - we are likely to be dumping in 'difficult' circumstances.
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t iedm_openDumpFile(char *filePath,
                          int32_t detailLevel,
                          int64_t userDataBytes,
                          iedmDump_t **dump)
{
    int32_t rc = OK;
    iedmDump_t *localDump;

    TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    localDump = (iedmDump_t *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_engine_misc,44),1, sizeof(iedmDump_t));

    if (localDump == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    localDump->detailLevel = detailLevel;
    localDump->userDataBytes = userDataBytes;

    // Try and allocate a buffer for writes to the dump
    localDump->bufferSize = iedmDUMP_DATA_BUFFER_SIZE_MAX;
    while(1)
    {
        // We're below the minimum, don't use a buffer
        if (localDump->bufferSize < iedmDUMP_DATA_BUFFER_SIZE_MIN)
        {
            localDump->bufferSize = 0;
            break;
        }

        if ((localDump->buffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,45),localDump->bufferSize)) != NULL)
        {
            break;
        }

        localDump->bufferSize = localDump->bufferSize / 2;
    }

    // No file path specified, create a temporary file on which we will perform
    // an in-line format when finished.
    if (filePath[0] == '\0')
    {
        strcpy(filePath, TEMPORARY_DUMP_FILE_PATH_FORMAT);

        localDump->fp = mkstemp(filePath);

        if (localDump->fp != iedmINVALID_FILE)
        {
            TRACE(ENGINE_CEI_TRACE, "Temporary file '%s' opened\n", filePath);
            localDump->temporaryFile = true;
        }
    }
    else
    {
        (void)unlink(filePath); // Never want to append, so delete the file.

        localDump->fp = iedm_open(filePath);

        if (localDump->fp == iedmINVALID_FILE)
        {
            TRACE(ENGINE_ERROR_TRACE, "%s: Failed to open dump file \"%s\" errno=%d\n", __func__,
                  filePath, errno);
            rc = ISMRC_Error;
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    iedm_writeDumpDescription(localDump);

    // Write the server global to all non-temporary dumps
    if (localDump->temporaryFile == false)
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();

        iedm_dumpData(localDump, "ismEngine_Server_t", &ismEngine_serverGlobal, sizeof(ismEngine_Server_t));
        if (pThreadData != NULL)
        {
            iedm_dumpData(localDump, "ieutThreadData_t", pThreadData, sizeof(ieutThreadData_t));
        }
    }

    *dump = localDump;

mod_exit:

    if (rc != OK && localDump != NULL) ism_common_free(ism_memory_engine_misc,localDump);

    TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief  Close a Dump file, renaming it if it was a partial file
///
/// @param[in,out] filePath The file being closed
/// @param[in]     dump     Pointer to dump being closed
/// @param[in,out] prc      Return code of the calling function
///
/// @remark This function will close the file and, if it is a partial file
///         rename it to it's .dat equivalent.
//****************************************************************************
void iedm_closeDumpFile(char *filePath, iedmDump_t *dump, int32_t *prc)
{
    TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Flush the dump buffer
    if (dump->bufferPos != 0)
        iedm_write(dump->fp, dump->buffer, dump->bufferPos);

    (void)iedm_close(dump->fp);

    // If this is a temporary file, we format it in-line and delete it
    if (dump->temporaryFile == true)
    {
        if (*prc == OK)
        {
            void *libHandle = dlopen(iefmDUMP_FORMAT_LIB, RTLD_LAZY | RTLD_GLOBAL);

            if (libHandle != NULL)
            {
                iefmReadAndFormatFile_t iefm_readAndFormatFileFn = dlsym(libHandle, iefmDUMP_FORMAT_FUNC);

                if (iefm_readAndFormatFileFn != NULL)
                {
                    iefmHeader_t dumpHeader = {0};

                    dumpHeader.inputFilePath = filePath;
                    dumpHeader.outputFile = stdout;

                    (void)iefm_readAndFormatFileFn(&dumpHeader);
                }

                dlclose(libHandle);
            }
        }

        (void)unlink(filePath);

        filePath[0] = '\0'; // Don't want this name to persist
    }
    // Not a temporary file
    else
    {
        if (*prc == OK)
        {
            char *partialFile = strstr(filePath, ".partial");

            if (partialFile != NULL)
            {
                char newFilePath[strlen(filePath) + 1];

                strcpy(newFilePath, filePath);
                strcpy(&newFilePath[(size_t)(partialFile-filePath)], ".dat");

                (void)unlink(newFilePath); // Get rid of the target file

                if (rename(filePath, newFilePath) == 0)
                {
                    assert(strlen(newFilePath)<=strlen(filePath));
                    strcpy(filePath, newFilePath);
                }
            }
        }
        // The dump command failed, unlink the file created so-far
        else
        {
            (void)unlink(filePath);
        }
    }

    ism_common_free(ism_memory_engine_misc,dump->buffer);
    ism_common_free(ism_memory_engine_misc,dump);

    TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief Write the specified data to the specified dump file as a block
///
/// @remark The startAddress and endAddress written to the dump file is
///         solely based upon the first block passed, however the length
///         is the sum of the length of all of the data blocks.
/// @param[in]     dump       Pointer to a dump structure
/// @param[in]     dataType   String identifying the content of the data
/// @param[in]     numBlocks  Number of blocks to dump
/// @param[in]       ...        Repeated pairs of parameters
/// @param[in]       address    Address of the data to dump
/// @param[in]       length     Length of the data to dump
//****************************************************************************
void iedm_dumpDataV(iedmDump_t *dump,
                    char *dataType,
                    int numBlocks,
                    ...)
{
    const size_t dataTypeLen = strlen(dataType)+1;
    const size_t hdrLen = 1 + dataTypeLen + sizeof(void *) + sizeof(void *) + sizeof(size_t);
    const size_t bufferSize = dump->bufferSize;
    int index;
    size_t dataLen = 0;
    va_list ap;
    void *endAddress;
    struct 
    {
        void *address;
        size_t length;
    } blocks[numBlocks];

    if (numBlocks == 0)
    {
        return;
    }

    va_start(ap, numBlocks);
    for (index=0; index < numBlocks; index++)
    {
        blocks[index].address = (void *)va_arg(ap, void *);
        blocks[index].length = (size_t)va_arg(ap, size_t);
        dataLen +=  blocks[index].length;
    }
    va_end(ap);
    endAddress = ((char *)blocks[0].address) + blocks[0].length -1;

    // First write out the header
    if (dump->bufferPos + hdrLen > bufferSize)
    {
        // Buffer is full so write out the buffer
        iedm_write(dump->fp, dump->buffer, dump->bufferPos);
        dump->bufferPos = 0;
    }
    if (hdrLen <= bufferSize)
    {
        // Header fits in the buffer, write it to the buffer
        char *bufferPointer = &dump->buffer[dump->bufferPos];

        *(bufferPointer++) = dump->depth;
        memcpy(bufferPointer, dataType, dataTypeLen);
        bufferPointer += dataTypeLen;
        memcpy(bufferPointer, &(blocks[0].address), sizeof(blocks[0].address));
        bufferPointer += sizeof(void *);
        memcpy(bufferPointer, &endAddress, sizeof(endAddress));
        bufferPointer += sizeof(void *);
        memcpy(bufferPointer, &dataLen, sizeof(size_t));
        bufferPointer += sizeof(size_t);

        dump->bufferPos = (size_t)(bufferPointer-dump->buffer);
    }
    else // Very unlikely
    {
        // Header is bigger than buffer - write the header straight out
        iedm_write(dump->fp, &dump->depth, sizeof(dump->depth));
        iedm_write(dump->fp, dataType, dataTypeLen);
        iedm_write(dump->fp, &(blocks[0].address), sizeof(void *));
        iedm_write(dump->fp, &endAddress, sizeof(void *));
        iedm_write(dump->fp, &dataLen, sizeof(size_t));
    }

    // Now write out each block in turn
    for (index=0; index < numBlocks; index++)
    {
        if (dump->bufferPos + blocks[index].length > bufferSize)
        {
            // Buffer is full so write out the buffer
            iedm_write(dump->fp, dump->buffer, dump->bufferPos);
            dump->bufferPos = 0;
        }
        if (blocks[index].length <= bufferSize)
        {
            // New data fits in the buffer, write it to the buffer
            char *bufferPointer = &dump->buffer[dump->bufferPos];
            memcpy(bufferPointer, blocks[index].address, blocks[index].length);
            dump->bufferPos+=blocks[index].length;
        }
        else
        {
            // New data is bigger than buffer - write the new data straight out.
            iedm_write(dump->fp, blocks[index].address, blocks[index].length);
        }
    }
    return;
}

//****************************************************************************
/// @internal
///
/// @brief  Dump information about a Client-state to a file.
///
/// Dumps information about the client-state with a specified client ID to a file.
///
/// @param[in]     clientId       Client ID
/// @param[in]     detailLevel    How much detail to include in the output
/// @param[in]     userDataBytes  Max bytes of user data (per area) to write
/// @param[in,out] resultPath     Requested path on input, result path on output
///
/// @remark If resultPath on input is an empty string, the dump is written to a
///         temporary file and is formatted inline, to stdout - in this case,
///         the temporary file is _not_ returned to the caller.
///
/// @remark If the data is gathered synchronously, the result path should be
///         the requested path on input with '.dat' appended to it, if the data
///         is gathered asynchronously, the result path should instead have
///         '.partial' appended to it, once the request has completed, the file
///         should be renamed to the '.dat' form.
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_dumpClientState(
                                        const char *clientId,
                                        int32_t detailLevel,
                                        int64_t userDataBytes,
                                        char *resultPath)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, clientId, ENGINE_CEI_TRACE,
               FUNCTION_ENTRY "clientId='%s' detailLevel=%d resultPath='%s'\n", __func__,
               clientId ? clientId : "", detailLevel, resultPath);

    if (clientId == NULL)
    {
        rc = ISMRC_NotImplemented;
    }
    else
    {
        char resultBuffer[MIN_RESULT_BUFFER_LENGTH];
        iedmDump_t *dump;

        // If no result path was supplied on input, use a local buffer for the
        // temporary file name.
        if (resultPath[0] == '\0')
        {
            resultBuffer[0] = '\0';
            resultPath = resultBuffer;
        }
        else
        {
            strcat(resultPath, ".dat"); // synchronous
        }

        rc = iedm_openDumpFile(resultPath, detailLevel, userDataBytes, &dump);

        if (rc != OK) goto mod_exit;

        rc = iecs_dumpClientState(pThreadData, clientId, (iedmDumpHandle_t)dump);

        iedm_closeDumpFile(resultPath, dump, &rc);
    }

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, resultPath='%s'\n", __func__,
               rc, resultPath);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Dump information about an individual Topic to a file
///
/// Dumps information about the specified topic to a file.
///
/// @param[in]     topicString    Topic String
/// @param[in]     detailLevel    How much detail to include in the output
/// @param[in]     userDataBytes  Max bytes of user data (per area) to write
/// @param[in,out] resultPath     Requested path on input, result path on output
///
/// @remark If resultPath on input is an empty string, the dump is written to a
///         temporary file and is formatted inline, to stdout - in this case,
///         the temporary file is _not_ returned to the caller.
///
/// @remark If the data is gathered synchronously, the result path should be
///         the requested path on input with '.dat' appended to it, if the data
///         is gathered asynchronously, the result path should instead have
///         '.partial' appended to it, once the request has completed, the file
///         should be renamed to the '.dat' form.
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_dumpTopic(
                                  const char *topicString,
                                  int32_t detailLevel,
                                  int64_t userDataBytes,
                                  char *resultPath)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    assert(topicString != NULL);

    ieutTRACEL(pThreadData, topicString, ENGINE_CEI_TRACE, FUNCTION_ENTRY "topicString='%s' detailLevel=%d resultPath='%s'\n", __func__,
               topicString, detailLevel, resultPath);

    char resultBuffer[MIN_RESULT_BUFFER_LENGTH];
    iedmDump_t *dump;

    // If no result path was supplied on input, use a local buffer for the
    // temporary file name.
    if (resultPath[0] == '\0')
    {
        resultBuffer[0] = '\0';
        resultPath = resultBuffer;
    }
    else
    {
        strcat(resultPath, ".dat"); // synchronous
    }

    rc = iedm_openDumpFile(resultPath, detailLevel, userDataBytes, &dump);

    if (rc != OK) goto mod_exit;

    rc = iett_dumpTopic(pThreadData, topicString, (iedmDumpHandle_t)dump);

    iedm_closeDumpFile(resultPath, dump, &rc);

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, resultPath='%s'\n", __func__,
               rc, resultPath);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Dump information about the topic tree below a root to a file
///
/// Dumps information about the topic tree below an optional root node to a
/// files - if no root is specified the entire topic tree is dumped.
///
/// @param[in]     rootTopicString  Root topic String (NULL for entire tree)
/// @param[in]     detailLevel      How much detail to include in the output
/// @param[in]     userDataBytes    Max bytes of user data (per area) to write
/// @param[in,out] resultPath       Requested path on input, result path on output
///
/// @remark If resultPath on input is an empty string, the dump is written to a
///         temporary file and is formatted inline, to stdout - in this case,
///         the temporary file is _not_ returned to the caller.
///
/// @remark If the data is gathered synchronously, the result path should be
///         the requested path on input with '.dat' appended to it, if the data
///         is gathered asynchronously, the result path should instead have
///         '.partial' appended to it, once the request has completed, the file
///         should be renamed to the '.dat' form.
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_dumpTopicTree(
                                      const char *rootTopicString,
                                      int32_t detailLevel,
                                      int64_t userDataBytes,
                                      char *resultPath)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "rootTopicString='%s' detailLevel=%d resultPath='%s'\n", __func__,
          rootTopicString ? rootTopicString : "", detailLevel, resultPath);

    char resultBuffer[MIN_RESULT_BUFFER_LENGTH];
    iedmDump_t *dump;

    // If no result path was supplied on input, use a local buffer for the
    // temporary file name.
    if (resultPath[0] == '\0')
    {
        resultBuffer[0] = '\0';
        resultPath = resultBuffer;
    }
    else
    {
        strcat(resultPath, ".dat"); // synchronous
    }

    rc = iedm_openDumpFile(resultPath, detailLevel, userDataBytes, &dump);

    if (rc != OK) goto mod_exit;

    int32_t newRc = iers_dumpServersList(pThreadData, (iedmDumpHandle_t)dump);

    if (rc == OK) rc = newRc;

    newRc = iett_dumpTopicTree(pThreadData, rootTopicString, (iedmDumpHandle_t)dump);

    if (rc == OK) rc = newRc;

    iedm_closeDumpFile(resultPath, dump, &rc);

mod_exit:

    TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, resultPath='%s'\n", __func__,
          rc, resultPath);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Dump information about a Queue to a file.
///
/// Dumps information about the named queue to a file.
///
/// @param[in]     queueName      Queue Name
/// @param[in]     detailLevel    How much detail to include in the output
/// @param[in]     userDataBytes  Max bytes of user data (per area) to write
/// @param[in,out] resultPath     Requested path on input, result path on output
///
/// @remark If resultPath on input is an empty string, the dump is written to a
///         temporary file and is formatted inline, to stdout - in this case,
///         the temporary file is _not_ returned to the caller.
///
/// @remark If the data is gathered synchronously, the result path should be
///         the requested path on input with '.dat' appended to it, if the data
///         is gathered asynchronously, the result path should instead have
///         '.partial' appended to it, once the request has completed, the file
///         should be renamed to the '.dat' form.
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_dumpQueue(
                                  const char *queueName,
                                  int32_t detailLevel,
                                  int64_t userDataBytes,
                                  char *resultPath)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, queueName, ENGINE_CEI_TRACE, FUNCTION_ENTRY "queueName='%s' detailLevel=%d resultPath='%s'\n", __func__,
               queueName ? queueName : "", detailLevel, resultPath);

    if (queueName == NULL)
    {
        rc = ISMRC_NotImplemented;
    }
    else
    {
        char resultBuffer[MIN_RESULT_BUFFER_LENGTH];
        iedmDump_t *dump;

        // If no result path was supplied on input, use a local buffer for the
        // temporary file name.
        if (resultPath[0] == '\0')
        {
            resultBuffer[0] = '\0';
            resultPath = resultBuffer;
        }
        else
        {
            strcat(resultPath, ".dat"); // synchronous
        }

        rc = iedm_openDumpFile(resultPath, detailLevel, userDataBytes, &dump);

        if (rc != OK) goto mod_exit;

        rc = ieqn_dumpQueueByName(pThreadData, queueName, (iedmDumpHandle_t)dump);

        iedm_closeDumpFile(resultPath, dump, &rc);
    }

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, resultPath='%s'\n", __func__,
               rc, resultPath);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @brief  Dump information about a Queue handle to a file.
///
/// Dumps information about the handle (which could be a named queue handle
/// or a subscription queue handle) to a file.
///
/// @param[in]     queueHandle    Handle to the queue
/// @param[in]     detailLevel    How much detail to include in the output
/// @param[in]     userDataBytes  Max bytes of user data (per area) to write
/// @param[in,out] resultPath     Requested path on input, result path on output
///
/// @remark If resultPath on input is an empty string, the dump is written to a
///         temporary file and is formatted inline, to stdout - in this case,
///         the temporary file is _not_ returned to the caller.
///
/// @remark If the data is gathered synchronously, the result path should be
///         the requested path on input with '.dat' appended to it, if the data
///         is gathered asynchronously, the result path should instead have
///         '.partial' appended to it, once the request has completed, the file
///         should be renamed to the '.dat' form.
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t iedm_dumpQueueByHandle(ismQHandle_t queueHandle,
                               int32_t detailLevel,
                               int64_t userDataBytes,
                               char *resultPath)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;

    ieutTRACEL(pThreadData, queueHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "queueHandle='%p' detailLevel=%d resultPath='%s'\n", __func__,
               queueHandle, detailLevel, resultPath);

    assert(queueHandle != NULL);

    char resultBuffer[MIN_RESULT_BUFFER_LENGTH];
    iedmDump_t *dump;

    // If no result path was supplied on input, use a local buffer for the
    // temporary file name.
    if (resultPath[0] == '\0')
    {
        resultBuffer[0] = '\0';
        resultPath = resultBuffer;
    }
    else
    {
        strcat(resultPath, ".dat"); // synchronous
    }

    rc = iedm_openDumpFile(resultPath, detailLevel, userDataBytes, &dump);

    if (rc != OK) goto mod_exit;

    ieq_dump(pThreadData, queueHandle, (iedmDumpHandle_t)dump);

    iedm_closeDumpFile(resultPath, dump, &rc);

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, resultPath='%s'\n", __func__,
               rc, resultPath);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Dump information about a locks to a file.
///
/// Dumps information about the locks in the lock manager to a file.
///
/// @param[in]     lockName       Lock name - currently unused
/// @param[in]     detailLevel    How much detail to include in the output
/// @param[in]     userDataBytes  Max bytes of user data (per area) to write
/// @param[in,out] resultPath     Requested path on input, result path on output
///
/// @remark If resultPath on input is an empty string, the dump is written to a
///         temporary file and is formatted inline, to stdout - in this case,
///         the temporary file is _not_ returned to the caller.
///
/// @remark If the data is gathered synchronously, the result path should be
///         the requested path on input with '.dat' appended to it, if the data
///         is gathered asynchronously, the result path should instead have
///         '.partial' appended to it, once the request has completed, the file
///         should be renamed to the '.dat' form.
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_dumpLocks(
    const char *       lockName,
    int32_t            detailLevel,
    int64_t            userDataBytes,
    char *             resultPath)
{
    int32_t rc = OK;

    TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "lockName='%s' detailLevel=%d resultPath='%s'\n", __func__,
          lockName ? lockName : "", detailLevel, resultPath);

    char resultBuffer[MIN_RESULT_BUFFER_LENGTH];
    iedmDump_t *dump;

    // If no result path was supplied on input, use a local buffer for the
    // temporary file name.
    if (resultPath[0] == '\0')
    {
        resultBuffer[0] = '\0';
        resultPath = resultBuffer;
    }
    else
    {
        strcat(resultPath, ".dat"); // synchronous
    }

    rc = iedm_openDumpFile(resultPath, detailLevel, userDataBytes, &dump);

    if (rc != OK) goto mod_exit;

    rc = ielm_dumpLocks(lockName, (iedmDumpHandle_t)dump);

    iedm_closeDumpFile(resultPath, dump, &rc);

mod_exit:

    TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, resultPath='%s'\n", __func__,
          rc, resultPath);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Dump information about transactions to a file.
///
/// Dumps information about the transactions to a file.
///
/// @param[in]     XIDString      Transaction Id - currently unused
/// @param[in]     detailLevel    How much detail to include in the output
/// @param[in]     userDataBytes  Max bytes of user data (per area) to write
/// @param[in,out] resultPath     Requested path on input, result path on output
///
/// @remark If resultPath on input is an empty string, the dump is written to a
///         temporary file and is formatted inline, to stdout - in this case,
///         the temporary file is _not_ returned to the caller.
///
/// @remark If the data is gathered synchronously, the result path should be
///         the requested path on input with '.dat' appended to it, if the data
///         is gathered asynchronously, the result path should instead have
///         '.partial' appended to it, once the request has completed, the file
///         should be renamed to the '.dat' form.
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_dumpTransactions(
    const char *       XIDString,
    int32_t            detailLevel,
    int64_t            userDataBytes,
    char *             resultPath)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, XIDString, ENGINE_CEI_TRACE, FUNCTION_ENTRY "XIDString='%s' detailLevel=%d resultPath='%s'\n", __func__,
               XIDString ? XIDString : "", detailLevel, resultPath);

    char resultBuffer[MIN_RESULT_BUFFER_LENGTH];
    iedmDump_t *dump;

    // If no result path was supplied on input, use a local buffer for the
    // temporary file name.
    if (resultPath[0] == '\0')
    {
        resultBuffer[0] = '\0';
        resultPath = resultBuffer;
    }
    else
    {
        strcat(resultPath, ".dat"); // synchronous
    }

    rc = iedm_openDumpFile(resultPath, detailLevel, userDataBytes, &dump);

    if (rc != OK) goto mod_exit;

    rc = ietr_dumpTransactions(pThreadData, XIDString, (iedmDumpHandle_t)dump);

    iedm_closeDumpFile(resultPath, dump, &rc);

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, resultPath='%s'\n", __func__,
               rc, resultPath);
    ieut_leavingEngine(pThreadData);
    return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of engineDump.c                                               */
/*                                                                   */
/*********************************************************************/
