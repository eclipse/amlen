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

//*********************************************************************
/// @file  exportResources.c
/// @brief API functions to export (and import) engine resources
//*********************************************************************
#define TRACE_COMP Engine

#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <assert.h>

#include "exportResources.h"
#include "exportClientState.h"
#include "exportSubscription.h"
#include "exportRetained.h"
#include "exportQMessages.h"
#include "exportMessage.h"
#include "msgCommon.h"
#include "engineAsync.h"
#include "engineUtils.h"
#include "clientStateExpiry.h"

static int32_t ieie_continueImportResources( ieutThreadData_t *pThreadData
                                            , ieieImportResourceControl_t *pControl );

//****************************************************************************
/// @brief Create and initialize the global import/export structure
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @see iepi_termImportExport
//****************************************************************************
int32_t ieie_initImportExport(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;
    ieieImportExportGlobal_t *importExportGlobal = NULL;

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Create the structure
    importExportGlobal = iemem_calloc(pThreadData,
                                      IEMEM_PROBE(iemem_exportResources, 4), 1,
                                      sizeof(ieieImportExportGlobal_t));

    if (NULL == importExportGlobal)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(importExportGlobal->strucId, ieieIMPORTEXPORT_GLOBAL_STRUCID, 4);

    // Initialize the requestID to between 1 and the max seed value
    importExportGlobal->nextRequestID = (ism_engine_fastTimeUInt64() % (ieieIMPORTEXPORT_MAX_REQUESTID_SEED-1UL)) + 1UL;
    importExportGlobal->serverInitTime = ism_common_currentTimeNanos();
    importExportGlobal->maxActiveRequests = (uint32_t)ism_common_getIntConfig(ismENGINE_CFGPROP_MAX_CONCURRENT_IMPORTEXPORT_REQUESTS,
                                                                              ismENGINE_DEFAULT_MAX_CONCURRENT_IMPORTEXPORT_REQUESTS);

    // Get the function pointers from the properties structure
    importExportGlobal->ism_transport_disableClientSet = (ism_transport_disableClientSet_f)
                                                         ism_common_getLongConfig(ismENGINE_CFGPROP_DISABLECLIENTSET_FNPTR,
                                                                                  ismENGINE_DEFAULT_DISABLECLIENTSET_FNPTR);
    assert(importExportGlobal->ism_transport_disableClientSet != NULL);
    importExportGlobal->ism_transport_enableClientSet = (ism_transport_enableClientSet_f)
                                                        ism_common_getLongConfig(ismENGINE_CFGPROP_ENABLECLIENTSET_FNPTR,
                                                                                 ismENGINE_DEFAULT_ENABLECLIENTSET_FNPTR);
    assert(importExportGlobal->ism_transport_enableClientSet != NULL);

    int osrc = pthread_mutex_init(&importExportGlobal->activeRequestLock, NULL);

    if (osrc != 0)
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);
        goto mod_exit;
    }

    ismEngine_serverGlobal.importExportGlobal = importExportGlobal;

mod_exit:

    ieutTRACEL(pThreadData, importExportGlobal,  ENGINE_FNC_TRACE, FUNCTION_EXIT "importExportGlobal=%p, rc=%d\n", __func__, importExportGlobal, rc);

    return rc;
}

//****************************************************************************
/// @brief Stop the current import / export activity
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @see iepi_destroyEnginePolicyInfo
//****************************************************************************
int32_t ieie_stopImportExport(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, ismEngine_serverGlobal.importExportGlobal, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Wait for active tasks to end
    if (ismEngine_serverGlobal.importExportGlobal != NULL)
    {
        ieieImportExportGlobal_t *importExportGlobal = ismEngine_serverGlobal.importExportGlobal;

        ismEngine_lockMutex(&importExportGlobal->activeRequestLock);
        assert(importExportGlobal->stopCalled == false);
        importExportGlobal->stopCalled = true;
        ismEngine_unlockMutex(&importExportGlobal->activeRequestLock);

        int pauseMs = 20000;   // Initial pause is 0.02s
        uint32_t Loop = 0;

        // Wait for active tasks to complete
        Loop = 0;
        while ((volatile uint32_t)importExportGlobal->activeRequests > 0)
        {
            ieutTRACEL(pThreadData, importExportGlobal->activeRequests, ENGINE_NORMAL_TRACE, "%s: activeRequests is %u\n",
                       __func__, importExportGlobal->activeRequests);

            // And pause for a short time to allow the tasks to end
            ism_common_sleep(pauseMs);
            if (++Loop > 290) // After 2 minutes
            {
                pauseMs = 5000000; // Upgrade pause to 5 seconds
            }
            else if (Loop > 50) // After 1 Seconds
            {
                pauseMs = 500000; // Upgrade pause to .5 second
            }
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Terminate and destroy the import/export functions
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @see ieie_initImportExport
//****************************************************************************
int32_t ieie_destroyImportExport(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, ismEngine_serverGlobal.importExportGlobal, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    ieieImportExportGlobal_t *importExportGlobal = ismEngine_serverGlobal.importExportGlobal;

    // Destroy the structure
    if (importExportGlobal != NULL)
    {
        assert(importExportGlobal->activeRequests == 0);
        if (importExportGlobal->activeImportClientIdTable != NULL)
        {
            ieut_destroyHashTable(pThreadData, importExportGlobal->activeImportClientIdTable);
        }
        (void)pthread_mutex_destroy(&importExportGlobal->activeRequestLock);
        iemem_free(pThreadData, iemem_exportResources, importExportGlobal);
        ismEngine_serverGlobal.importExportGlobal = NULL;
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Release messages kept in the hash set of exported messages
///
/// @param[in]     value          Pointers to ismEngine_Message_t whose use count was
///                               increased during export processing
/// @param[in,out] context        Not used.
//****************************************************************************
void ieie_releaseExportedMessage(ieutThreadData_t *pThreadData,
                                 uint64_t value,
                                 void *context)
{
    ismEngine_Message_t *message = (ismEngine_Message_t *)value;

    iem_releaseMessage(pThreadData, message);
}

//****************************************************************************
/// @internal
///
/// @brief  Release messages kept in the hash table of imported messages
///
/// @param[in]     key            DataId of the message as it was previously exported
/// @param[in]     keyHash        Hash of the key value
/// @param[in]     value          Pointers to ismEngine_Message_t whose use count was
///                               increased during import processing
/// @param[in,out] context        Not used.
//****************************************************************************
void ieie_releaseImportedMessage(ieutThreadData_t *pThreadData,
                                 char *key,
                                 uint32_t keyHash,
                                 void *value,
                                 void *context)
{
    ismEngine_Message_t *message = (ismEngine_Message_t *)value;

    iem_releaseMessage(pThreadData, message);
}

//****************************************************************************
/// @internal
///
/// @brief  Write a header to an engine resource export file
///
/// @param[in]     pControl     ieieExportResourceControl_t of the export request
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static inline int32_t ieie_writeResourceFileHeader(ieutThreadData_t *pThreadData,
                                                   ieieExportResourceControl_t *pControl)
{
    int32_t rc = OK;

    assert(pControl != NULL);

    ieutTRACEL(pThreadData, pControl->startTime, ENGINE_FNC_TRACE, FUNCTION_ENTRY "(pControl=%p pControl->startTime=0x%016lx)\n", __func__,
               pControl, (uint64_t)(pControl->startTime));

    size_t serverNameLength = pControl->exportServerName == NULL ? 0 : strlen(pControl->exportServerName)+1;
    size_t serverUIDLength = pControl->exportServerUID == NULL ? 0 : strlen(pControl->exportServerUID)+1;
    size_t clientIdLength = pControl->clientId == NULL ? 0 : strlen(pControl->clientId)+1;
    size_t topicLength = pControl->topic == NULL ? 0 : strlen(pControl->topic)+1;

    size_t requiredDataLength = sizeof(ieieResourceFileHeader_t) +
                                serverNameLength + serverUIDLength + clientIdLength + topicLength;

    ieieResourceFileHeader_t *header = iemem_malloc(pThreadData,
                                                    IEMEM_PROBE(iemem_exportResources, 3),
                                                    requiredDataLength);
    if (header == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    ismEngine_SetStructId(header->StrucId, ieieRESOURCEFILE_STRUCID);

    header->Version = ieieRESOURCEFILE_CURRENT_VERSION;
    header->RequestID = pControl->requestID;
    header->Options = pControl->options;
    header->StartTime = pControl->startTime;

    uint8_t *curDataPos = (uint8_t *)(header+1);

    header->ServerNameLength = (uint32_t)serverNameLength;
    if (header->ServerNameLength != 0) memcpy(curDataPos, pControl->exportServerName, serverNameLength);
    curDataPos += serverNameLength;

    header->ServerUIDLength = (uint32_t)serverUIDLength;
    if (header->ServerUIDLength != 0) memcpy(curDataPos, pControl->exportServerUID, serverUIDLength);
    curDataPos += serverUIDLength;

    header->ClientIdLength = (uint32_t)clientIdLength;
    if (header->ClientIdLength != 0) memcpy(curDataPos, pControl->clientId, clientIdLength);
    curDataPos += clientIdLength;

    header->TopicLength = (uint32_t)topicLength;
    if (header->TopicLength != 0) memcpy(curDataPos, pControl->topic, topicLength);
    curDataPos += topicLength;

    assert(curDataPos-(uint8_t *)header == requiredDataLength);

    rc = ieie_writeExportRecord(pThreadData,
                                pControl,
                                ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER,
                                pControl->startTime, (uint32_t)requiredDataLength, (void *)header);

    iemem_free(pThreadData, iemem_exportResources, header);

    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Write a footer to the an engine resource export file
///
/// @param[in]     pControl     ieieExportResourceControl_t of the export request
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static inline int32_t ieie_writeResourceFileFooter(ieutThreadData_t *pThreadData,
                                                   ieieExportResourceControl_t *pControl)
{
    int32_t rc = OK;

    assert(pControl != NULL);
    assert(pControl->endTime != 0);

    ieutTRACEL(pThreadData, pControl->endTime, ENGINE_FNC_TRACE, FUNCTION_ENTRY "(pControl=%p endTime=0x%016lx)\n", __func__,
               pControl, pControl->endTime);

    size_t requiredDataLength = sizeof(ieieResourceFileFooter_t);

    ieieResourceFileFooter_t *footer = iemem_malloc(pThreadData,
                                                    IEMEM_PROBE(iemem_exportResources, 8),
                                                    requiredDataLength);
    if (footer == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    ismEngine_SetStructId(footer->StrucId, ieieRESOURCEFILE_STRUCID);

    footer->Version = ieieRESOURCEFILE_CURRENT_VERSION;
    footer->RequestID = pControl->requestID;
    footer->EndTime = pControl->endTime;

    rc = ieie_writeExportRecord(pThreadData,
                                pControl,
                                ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER,
                                pControl->startTime, (uint32_t)requiredDataLength, (void *)footer);

    iemem_free(pThreadData, iemem_exportResources, footer);

    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Fully qualify the expected path to a specified resources filename
///
/// @param[in]     fileName     The unqualified filename
/// @param[in]     forImport    The file is being opened for import (true) or
///                             export (false)
/// @param[out]    filePath     The qualified file path
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieie_fullyQualifyResourceFilename(ieutThreadData_t *pThreadData,
                                          const char *fileName,
                                          bool forImport,
                                          char **filePath)
{
    int32_t rc = OK;
    char *localFilePath;
    char *dataDirProperty = forImport ? ieieRESOURCEFILE_IMPORTDIRECTORY_PROPERTY : ieieRESOURCEFILE_EXPORTDIRECTORY_PROPERTY;

    ieutTRACEL(pThreadData, fileName, ENGINE_FNC_TRACE, FUNCTION_ENTRY "fileName='%s' forImport=%d filePath=%p\n",
               __func__, fileName, (int)forImport, filePath);

    // Don't allow a filename that includes a directory specification
    if (strchr(fileName, '/') != NULL)
    {
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(rc, "%s%s", ismENGINE_IMPORTEXPORT_STATUS_FIELD_FILENAME, fileName);
        goto mod_exit;
    }

    // Don't allow the status file extension in the name produced
    if (strstr(fileName, ieieSTATUSFILE_TYPE) != NULL)
    {
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(rc, "%s%s", ismENGINE_IMPORTEXPORT_STATUS_FIELD_FILENAME, fileName);
        goto mod_exit;
    }

    // Get the expected parent directory
    const char *dataDir = ism_common_getStringConfig(dataDirProperty);

    if (dataDir == NULL)
    {
        ieutTRACEL(pThreadData, dataDir, ENGINE_INTERESTING_TRACE, "Property %s not found\n", dataDirProperty);
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(rc, "%s%s", dataDirProperty, "");
        goto mod_exit;
    }

    localFilePath = iemem_malloc(pThreadData,
                                 IEMEM_PROBE(iemem_exportResources, 1),
                                 strlen(dataDir)+
                                 strlen(fileName)+2);

    if (localFilePath == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Make sure the directory exists
    sprintf(localFilePath, "%s/", dataDir);
    int osrc = mkdir(localFilePath, S_IRWXU | S_IRGRP | S_IXGRP);

    // Create the import/export subdirectory
    if (osrc == -1 && errno != EEXIST)
    {
        int error = errno;
        rc = ISMRC_FileUpdateError;
        ism_common_setErrorData(rc, "%s%d", localFilePath, error);
        ieutTRACEL(pThreadData, error, ENGINE_INTERESTING_TRACE, "Failed to create / access directory '%s' errno=%d\n", localFilePath, error);
        iemem_free(pThreadData, iemem_exportResources, localFilePath);
        goto mod_exit;
    }

    strcat(localFilePath, fileName);

    *filePath = localFilePath;

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d *filePath=%p(%s)\n",
               __func__, rc, *filePath, *filePath ? *filePath : "NULL");
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Update the specified status file with specified text
///
/// @param[in] statusFilePath     Fully qualified path of the file to update
/// @param[in] updateText         Text of the update to be written
/// @param[in] updateLength       Length of the update text (in bytes)
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieie_updateStatusFile(ieutThreadData_t *pThreadData,
                              const char *statusFilePath,
                              const char *updateText,
                              int updateLength)
{
    int32_t rc = OK;

    assert(statusFilePath != NULL);

    char tmpFilePath[strlen(statusFilePath) + strlen(ieieSTATUSFILE_TMP_TYPE) + 20];

    struct stat statBuf;

    // We only update the status file if it exists already, so that if it's been
    // deleted, we don't bring it back.
    if (stat(statusFilePath, &statBuf) != 0)
    {
        assert(rc == OK); // We don't regard this as a failure.
        ieutTRACEL(pThreadData, errno, ENGINE_INTERESTING_TRACE,
                   "Status file '%s' not found (errno=%d)\n", statusFilePath, errno);
        goto mod_exit;
    }

    int fd;

    uint32_t loop = 0;
    do
    {
        loop++;

        sprintf(tmpFilePath, "%s%s.%u", statusFilePath, ieieSTATUSFILE_TMP_TYPE, loop);

        fd = open(tmpFilePath,
                  ieieSTATUSFILE_OPEN_FLAGS,
                  ieieSTATUSFILE_PERMISSIONS);
    }
    while(fd == -1);

    FILE *fp = fdopen(fd, "w");

    if (fp == NULL)
    {
        rc = ISMRC_FileUpdateError;
        ism_common_setErrorData(rc, "%s%d", tmpFilePath, errno);
        close(fd);
        goto mod_exit;
    }

    size_t updateByteCount = (size_t)updateLength;
    size_t writtenByteCount = fwrite(updateText, 1, updateByteCount, fp);

    fclose(fp);

    if (updateByteCount != writtenByteCount)
    {
        rc = ISMRC_FileUpdateError;
        ism_common_setErrorData(rc, "%s%d", tmpFilePath, errno);
        goto mod_exit;
    }

    int osrc = rename(tmpFilePath, statusFilePath);

    if (osrc != 0)
    {
        rc = ISMRC_FileUpdateError;
        ism_common_setErrorData(rc, "%s%d", statusFilePath, errno);
        goto mod_exit;
    }

mod_exit:

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Update the export status file with current status
///
/// @param[in] pControl  ieieExportResourceControl_t for the request
/// @param[in] reportRC  Return code to report
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieie_updateExportStatus(ieutThreadData_t *pThreadData,
                                ieieExportResourceControl_t *pControl,
                                int32_t reportRC)
{
    int32_t rc = OK;

    char xbuf[2048];
    ieutJSONBuffer_t JSONBuffer = {true, {xbuf, sizeof(xbuf)}};

    ieut_jsonStartObject(&JSONBuffer, NULL);
    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_REQUESTID,
            pControl->requestID);
    if (pControl->filePath != NULL)
    {
        ieut_jsonAddString(&JSONBuffer,
                           ismENGINE_IMPORTEXPORT_STATUS_FIELD_FILEPATH,
                           pControl->filePath);
    }
    ieut_jsonAddString(&JSONBuffer,
                       ismENGINE_IMPORTEXPORT_STATUS_FIELD_FILENAME,
                       pControl->fileName);
    if (pControl->clientId != NULL)
    {
        ieut_jsonAddString(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_CLIENTID,
                pControl->clientId);
    }
    if (pControl->topic != NULL)
    {
        ieut_jsonAddString(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_TOPIC,
                pControl->topic);
    }
    if (pControl->exportServerName != NULL)
    {
        ieut_jsonAddString(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_EXPORTSERVERNAME,
                pControl->exportServerName);
    }
    if (pControl->exportServerUID != NULL)
    {
        ieut_jsonAddString(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_EXPORTSERVERUID,
                pControl->exportServerUID);
    }
    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_SERVERINITTIME,
            pControl->serverInitTime);
    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_STARTTIME,
            pControl->startTime);

    if (pControl->endTime == 0)
    {
        pControl->statusUpdateTime = ism_common_currentTimeNanos();

        ieut_jsonAddUInt64(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUSUPDATETIME,
                pControl->statusUpdateTime);
        // Status 0 == In-progress
        ieut_jsonAddInt32(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUS,
                ismENGINE_IMPORTEXPORT_STATUS_IN_PROGRESS);
        ieut_jsonAddInt32(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETCODE,
                reportRC);
    }
    else
    {
        pControl->statusUpdateTime = pControl->endTime;

        ieut_jsonAddUInt64(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUSUPDATETIME,
                pControl->statusUpdateTime);
        // Status 1 == Finished, 2 == Failed
        ieut_jsonAddInt32(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUS,
                reportRC == OK ? ismENGINE_IMPORTEXPORT_STATUS_COMPLETE :
                                 ismENGINE_IMPORTEXPORT_STATUS_FAILED);
        ieut_jsonAddInt32(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETCODE,
                reportRC);
        ieut_jsonAddUInt64(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_ENDTIME,
                pControl->endTime);

        // Report any diagnostics as part of the file.
        if (reportRC != OK)
        {
            ieut_jsonStartArray(&JSONBuffer, ismENGINE_IMPORTEXPORT_STATUS_FIELD_DIAGNOSTICS);

            ismEngine_getRWLockForRead(&pControl->diagnosticsLock);

            ieieDiagnosticInfo_t *pCurDiagnostic = pControl->latestDiagnostic;

            while(pCurDiagnostic != NULL)
            {
                char stringBuffer[100];
                char *string = NULL;

                ieut_jsonStartObject(&JSONBuffer, NULL);

                // Resource Type
                switch(pCurDiagnostic->resourceDataType)
                {
                    case ieieDATATYPE_EXPORTEDCLIENTSTATE:
                        string = ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_CLIENT;
                        break;
                    case ieieDATATYPE_EXPORTEDRETAINEDMSG:
                        string = ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_RETAINEDMSG;
                        break;
                    case ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB:
                    case ieieDATATYPE_EXPORTEDSUBSCRIPTION:
                    case ieieDATATYPE_EXPORTEDQNODE_SIMPLE:
                    case ieieDATATYPE_EXPORTEDQNODE_INTER:
                    case ieieDATATYPE_EXPORTEDQNODE_MULTI:
                    case ieieDATATYPE_EXPORTEDQNODE_MULTI_INPROG:
                        string = ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_SUBSCRIPTION;
                        break;
                    default:
                        sprintf(stringBuffer,
                                ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_UNKNOWN_STEM "(%d)",
                                pCurDiagnostic->resourceDataType);
                        string = stringBuffer;
                        break;
                }
                assert(string != NULL);
                ieut_jsonAddString(&JSONBuffer,
                        ismENGINE_IMPORTEXPORT_DIAGNOSTIC_FIELD_RESOURCETYPE,
                        string);

                // Resource Identifier string "human identifier"
                if (pCurDiagnostic->resourceIdentifier != NULL)
                {
                    string = pCurDiagnostic->resourceIdentifier;
                }
                else
                {
                    string = "";
                }
                ieut_jsonAddString(&JSONBuffer,
                        ismENGINE_IMPORTEXPORT_DIAGNOSTIC_FIELD_RESOURCEIDENTIFIER,
                        string);

                // Resource Data identifier "export identifier"
                ieut_jsonAddUInt64(&JSONBuffer,
                        ismENGINE_IMPORTEXPORT_DIAGNOSTIC_FIELD_RESOURCEDATAID,
                        pCurDiagnostic->resourceDataId);

                // Resource RC
                ieut_jsonAddInt32(&JSONBuffer,
                        ismENGINE_IMPORTEXPORT_DIAGNOSTIC_FIELD_RESOURCERC,
                        pCurDiagnostic->resourceRC);

                ieut_jsonEndObject(&JSONBuffer);

                pCurDiagnostic = pCurDiagnostic->next;
            }

            ismEngine_unlockRWLock(&pControl->diagnosticsLock);

            ieut_jsonEndArray(&JSONBuffer);
        }
    }

    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_RECORDSWRITTEN,
            pControl->recordsWritten);

    // Pick out some specific counts
    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_CLIENTSEXPORTED,
            pControl->writtenCount[ieieDATATYPE_EXPORTEDCLIENTSTATE]);
    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_SUBSCRIPTIONSEXPORTED,
            pControl->writtenCount[ieieDATATYPE_EXPORTEDSUBSCRIPTION] +
            pControl->writtenCount[ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB]);
    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETAINEDMSGSEXPORTED,
            pControl->writtenCount[ieieDATATYPE_EXPORTEDRETAINEDMSG]);

    ieut_jsonEndObject(&JSONBuffer);

    rc = ieie_updateStatusFile(pThreadData,
                               pControl->statusFilePath,
                               JSONBuffer.buffer.buf,
                               JSONBuffer.buffer.used);

    ieut_jsonReleaseJSONBuffer(&JSONBuffer);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_IDENT "pControl=%p reportRC=%d rc=%d\n",
               __func__, pControl, reportRC, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Update the import status file with current status
///
/// @param[in] pControl  ieieImportResourceControl_t for the request
/// @param[in] reportRC   Return code to report
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieie_updateImportStatus(ieutThreadData_t *pThreadData,
                                ieieImportResourceControl_t *pControl,
                                int32_t reportRC)
{
    int32_t rc = OK;

    char xbuf[2048];
    ieutJSONBuffer_t JSONBuffer = {true, {xbuf, sizeof(xbuf)}};

    ieut_jsonStartObject(&JSONBuffer, NULL);
    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_REQUESTID,
            pControl->requestID);
    ieut_jsonAddString(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_FILEPATH,
            pControl->filePath);
    if (pControl->clientId != NULL)
    {
        ieut_jsonAddString(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_CLIENTID,
                pControl->clientId);
    }
    if (pControl->topic != NULL)
    {
        ieut_jsonAddString(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_TOPIC,
                pControl->topic);
    }
    if (pControl->exportServerName != NULL)
    {
        ieut_jsonAddString(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_EXPORTSERVERNAME,
                pControl->exportServerName);
    }
    if (pControl->exportServerUID != NULL)
    {
        ieut_jsonAddString(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_EXPORTSERVERUID,
                pControl->exportServerUID);
    }
    if (pControl->importServerName != NULL)
    {
        ieut_jsonAddString(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_IMPORTSERVERNAME,
                pControl->importServerName);
    }
    if (pControl->importServerUID != NULL)
    {
        ieut_jsonAddString(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_IMPORTSERVERUID,
                pControl->importServerUID);
    }
    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_SERVERINITTIME,
            pControl->serverInitTime);
    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_STARTTIME,
            pControl->startTime);

    if (pControl->endTime == 0)
    {
        pControl->statusUpdateTime = ism_common_currentTimeNanos();

        ieut_jsonAddUInt64(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUSUPDATETIME,
                pControl->statusUpdateTime);
        // Status 0 == In-progress
        ieut_jsonAddInt32(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUS,
                ismENGINE_IMPORTEXPORT_STATUS_IN_PROGRESS);
        ieut_jsonAddInt32(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETCODE,
                reportRC);
    }
    else
    {
        pControl->statusUpdateTime = pControl->endTime;

        ieut_jsonAddUInt64(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUSUPDATETIME,
                pControl->statusUpdateTime);
        // Status 1 == Finished, 2 == Failed
        ieut_jsonAddInt32(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUS,
                reportRC == OK ? ismENGINE_IMPORTEXPORT_STATUS_COMPLETE :
                                 ismENGINE_IMPORTEXPORT_STATUS_FAILED);
        ieut_jsonAddInt32(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETCODE,
                reportRC);
        ieut_jsonAddUInt64(&JSONBuffer,
                ismENGINE_IMPORTEXPORT_STATUS_FIELD_ENDTIME,
                pControl->endTime);

        // Report any diagnostics as part of the file.
        if (reportRC != OK)
        {
            ieut_jsonStartArray(&JSONBuffer, ismENGINE_IMPORTEXPORT_STATUS_FIELD_DIAGNOSTICS);

            ismEngine_getRWLockForRead(&pControl->diagnosticsLock);

            ieieDiagnosticInfo_t *pCurDiagnostic = pControl->latestDiagnostic;

            while(pCurDiagnostic != NULL)
            {
                char stringBuffer[100];
                char *string = NULL;

                ieut_jsonStartObject(&JSONBuffer, NULL);

                // Resource Type
                switch(pCurDiagnostic->resourceDataType)
                {
                    case ieieDATATYPE_EXPORTEDCLIENTSTATE:
                        string = ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_CLIENT;
                        break;
                    case ieieDATATYPE_EXPORTEDRETAINEDMSG:
                        string = ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_RETAINEDMSG;
                        break;
                    case ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB:
                    case ieieDATATYPE_EXPORTEDSUBSCRIPTION:
                    case ieieDATATYPE_EXPORTEDQNODE_SIMPLE:
                    case ieieDATATYPE_EXPORTEDQNODE_INTER:
                    case ieieDATATYPE_EXPORTEDQNODE_MULTI:
                    case ieieDATATYPE_EXPORTEDQNODE_MULTI_INPROG:
                        string = ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_SUBSCRIPTION;
                        break;
                    default:
                        sprintf(stringBuffer,
                                ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_UNKNOWN_STEM "(%d)",
                                pCurDiagnostic->resourceDataType);
                        string = stringBuffer;
                        break;
                }
                assert(string != NULL);
                ieut_jsonAddString(&JSONBuffer,
                        ismENGINE_IMPORTEXPORT_DIAGNOSTIC_FIELD_RESOURCETYPE,
                        string);

                // Resource Identifier string "human identifier"
                if (pCurDiagnostic->resourceIdentifier != NULL)
                {
                    string = pCurDiagnostic->resourceIdentifier;
                }
                else
                {
                    string = "";
                }
                ieut_jsonAddString(&JSONBuffer,
                        ismENGINE_IMPORTEXPORT_DIAGNOSTIC_FIELD_RESOURCEIDENTIFIER,
                        string);

                // Resource Data identifier "export identifier"
                ieut_jsonAddUInt64(&JSONBuffer,
                        ismENGINE_IMPORTEXPORT_DIAGNOSTIC_FIELD_RESOURCEDATAID,
                        pCurDiagnostic->resourceDataId);

                // Resource RC
                ieut_jsonAddInt32(&JSONBuffer,
                        ismENGINE_IMPORTEXPORT_DIAGNOSTIC_FIELD_RESOURCERC,
                        pCurDiagnostic->resourceRC);

                ieut_jsonEndObject(&JSONBuffer);

                pCurDiagnostic = pCurDiagnostic->next;
            }

            ismEngine_unlockRWLock(&pControl->diagnosticsLock);

            ieut_jsonEndArray(&JSONBuffer);
        }
    }

    // Calculate how many of all record types have been started & finished
    assert(sizeof(pControl->recCountStarted)/sizeof(pControl->recCountStarted[0]) == ieieDATATYPE_LAST);

    uint64_t recordsStarted = 0;
    uint64_t recordsFinished = 0;
    for(uint32_t i=0; i<ieieDATATYPE_LAST; i++)
    {
        recordsStarted += pControl->recCountStarted[i];
        recordsFinished += pControl->recCountFinished[i];
    }

    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_RECORDSREAD,
            pControl->recordsRead);
    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_RECORDSSTARTED,
            recordsStarted);
    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_RECORDSFINISHED,
            recordsFinished);

    // Pick out some specific counts
    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_CLIENTSIMPORTED,
            pControl->recCountFinished[ieieDATATYPE_EXPORTEDCLIENTSTATE]);
    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_SUBSCRIPTIONSIMPORTED,
            pControl->recCountFinished[ieieDATATYPE_EXPORTEDSUBSCRIPTION] +
            pControl->recCountFinished[ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB]);
    ieut_jsonAddUInt64(&JSONBuffer,
            ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETAINEDMSGSIMPORTED,
            pControl->recCountFinished[ieieDATATYPE_EXPORTEDRETAINEDMSG]);

    ieut_jsonEndObject(&JSONBuffer);

    rc = ieie_updateStatusFile(pThreadData,
                               pControl->statusFilePath,
                               JSONBuffer.buffer.buf,
                               JSONBuffer.buffer.used);

    ieut_jsonReleaseJSONBuffer(&JSONBuffer);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_IDENT "pControl=%p rc=%d\n",
               __func__, pControl, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Allocate a requestId for this request
///
/// @param[in]     importExportGlobal  The global structure for reference
/// @param[in]     forImport           This is for an import (true) or export (false)
/// @param[in,out] pControl            ieieExportResourceControl_t or
///                                    ieieImportResourceControl_t for the request
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static int32_t ieie_allocateRequestId(ieutThreadData_t *pThreadData,
                                      ieieImportExportGlobal_t *importExportGlobal,
                                      bool forImport,
                                      void *pControl)
{
    int32_t rc = OK;
    uint64_t localRequestId = 0;
    char *localFilePath = NULL;
    char *dataDirProperty = forImport ? ieieRESOURCEFILE_IMPORTDIRECTORY_PROPERTY : ieieRESOURCEFILE_EXPORTDIRECTORY_PROPERTY;

    ieutTRACEL(pThreadData, forImport, ENGINE_FNC_TRACE, FUNCTION_ENTRY "forImport=%d pControl=%p\n",
               __func__, (int)forImport, pControl);

    char statusFilename[strlen(ieieEXAMPLE_STATUSFILE_FULLNAME)+1];

    // Get the expected parent directory
    const char *dataDir = ism_common_getStringConfig(dataDirProperty);

    // We allocate enough memory for the fully qualified path PLUS a string version
    // of the request ID to put out in messages
    localFilePath = iemem_malloc(pThreadData,
                                 IEMEM_PROBE(iemem_exportResources, 16),
                                 strlen(dataDir)+
                                 strlen(ieieEXAMPLE_STATUSFILE_FULLNAME) + 2 +
                                 strlen(ieieEXAMPLE_STATUSFILE_REQUESTID) + 1);

    if (localFilePath == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Make sure the directory exists
    sprintf(localFilePath, "%s/", dataDir);
    int osrc = mkdir(localFilePath, S_IRWXU | S_IRGRP | S_IXGRP);

    if (osrc == -1 && errno != EEXIST)
    {
        int error = errno;
        rc = ISMRC_FileUpdateError;
        ism_common_setErrorData(rc, "%s%d", localFilePath, error);
        ieutTRACEL(pThreadData, error, ENGINE_INTERESTING_TRACE, "Failed to create / access directory '%s' errno=%d\n", localFilePath, error);
        iemem_free(pThreadData, iemem_exportResources, localFilePath);
        goto mod_exit;
    }

    uint64_t triedNames = 0;
    size_t localFilePathLen = strlen(localFilePath);
    int fd = -1;
    do
    {
        localFilePath[localFilePathLen] = '\0';

        localRequestId = __sync_fetch_and_add(&importExportGlobal->nextRequestID, 1);

        if (triedNames++ == UINT64_MAX)
        {
            iemem_free(pThreadData, iemem_exportResources, localFilePath);
            rc = ISMRC_Error;
            ism_common_setError(rc);
            goto mod_exit;
        }
        else if (localRequestId != 0) // Don't want one with a value 0.
        {
            statusFilename[strlen(ieieEXAMPLE_STATUSFILE_NAME)] = '\0';

            sprintf(statusFilename, ieieSTATUSFILE_PREFIX "%lu" ieieSTATUSFILE_TYPE, localRequestId);
            strcat(localFilePath, statusFilename);

            fd = open(localFilePath,
                      ieieSTATUSFILE_OPEN_FLAGS,
                      ieieSTATUSFILE_PERMISSIONS);
        }
    }
    while(fd == -1);

    if (forImport)
    {
        ieieImportResourceControl_t *pImportControl = (ieieImportResourceControl_t *)pControl;

        pImportControl->requestID = localRequestId;
        pImportControl->statusFilePath = localFilePath;
        pImportControl->stringRequestID = localFilePath + strlen(localFilePath) + 1;

        sprintf(pImportControl->stringRequestID, "%lu", localRequestId);

        rc = ieie_updateImportStatus(pThreadData, pImportControl, rc);
    }
    else
    {
        ieieExportResourceControl_t *pExportControl = (ieieExportResourceControl_t *)pControl;

        pExportControl->requestID = localRequestId;
        pExportControl->statusFilePath = localFilePath;
        pExportControl->stringRequestID = localFilePath + strlen(localFilePath) + 1;

        sprintf(pExportControl->stringRequestID, "%lu", localRequestId);

        rc = ieie_updateExportStatus(pThreadData, pExportControl, rc);
    }

    close(fd);

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d localRequestId=%lu localFilePath=%p(%s)\n",
               __func__, rc, localRequestId, localFilePath, localFilePath ? localFilePath : "NULL");
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Write a fragmented record to the export file.
///
/// @param[in]     pControl          Control structure for the export.
/// @param[in]     recordType        The datatype of the record to write.
/// @param[in]     dataId            Data identifier for the record
/// @param[in]     dataFrags         ieieFragmentedExportData_t for record.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieie_writeExportRecordFrags(ieutThreadData_t *pThreadData,
                                    ieieExportResourceControl_t *pControl,
                                    ieieDataType_t recordType,
                                    uint64_t dataId,
                                    ieieFragmentedExportData_t *dataFrags)
{
    int32_t rc = OK;

    rc = ieie_exportDataFrags(pThreadData,
                              pControl->file,
                              recordType,
                              dataId,
                              dataFrags);

    if (rc == OK)
    {
        assert(recordType < ieieDATATYPE_LAST);

        pControl->recordsWritten += 1;
        pControl->writtenCount[recordType] += 1;

        if ((pControl->recordsWritten % ieieEXPORT_UPDATE_FREQUENCY) == 0)
        {
            rc = ieie_updateExportStatus(pThreadData, pControl, rc);
        }
    }

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Write a record to the export file.
///
/// @param[in]     pControl          Control structure for the export.
/// @param[in]     recordType        The datatype of the record to write.
/// @param[in]     dataId            Data Identifier for the record
/// @param[in]     dataLen           Length of the record data
/// @param[in]     data              Record data
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieie_writeExportRecord(ieutThreadData_t *pThreadData,
                               ieieExportResourceControl_t *pControl,
                               ieieDataType_t recordType,
                               uint64_t dataId,
                               uint32_t dataLen,
                               void *data)
{
    ieieFragmentedExportData_t dataFrags = {1, &data, &dataLen};

    return ieie_writeExportRecordFrags(pThreadData,
                                       pControl,
                                       recordType,
                                       dataId,
                                       &dataFrags);
}

//****************************************************************************
/// @internal
///
/// @brief  Complete the export of a set of resources.
///
/// @param[in]     pControl               Control structure for the export.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static int32_t ieie_completeExportResources( ieutThreadData_t *pThreadData
                                           , ieieExportResourceControl_t *pControl)
{
    ieutTRACEL(pThreadData, pControl,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pControl=%p\n", __func__, pControl);

    assert(pControl != NULL);

    int32_t rc = pControl->dataRecordRC;

    ieieImportExportGlobal_t *importExportGlobal = ismEngine_serverGlobal.importExportGlobal;

    assert(importExportGlobal != NULL);

    int32_t internal_rc;

    // If we called ism_transport_disableClientSet, we need to re-enable the ClientSet
    if (pControl->clientSetDisabled == true)
    {
        assert(pControl->clientId != NULL);
        assert(importExportGlobal->ism_transport_enableClientSet != NULL);

        internal_rc = importExportGlobal->ism_transport_enableClientSet(pControl->clientId);

        if (internal_rc != OK)
        {
            ieutTRACEL(pThreadData, internal_rc, ENGINE_WORRYING_TRACE,
                       "Unexpected return code %d from ism_transport_enableClientSet rc=%d requestID=%lu\n",
                       internal_rc, rc, pControl->requestID);
            if (rc == OK) rc = internal_rc;
        }
    }

    pControl->endTime = ism_common_currentTimeNanos();

    if (pControl->file != NULL)
    {
        internal_rc = ieie_writeResourceFileFooter(pThreadData, pControl);
        if (rc == OK) rc = internal_rc;

        internal_rc = ieie_finishWritingEncryptedFile(pThreadData, pControl->file);
        if (rc == OK) rc = internal_rc;

        // Only got a header and footer - so delete this file
        if (rc == OK && pControl->recordsWritten <= 2)
        {
            assert(pControl->writtenCount[ieieDATATYPE_EXPORTEDCLIENTSTATE] == 0);
            assert(pControl->writtenCount[ieieDATATYPE_EXPORTEDSUBSCRIPTION] == 0);
            assert(pControl->writtenCount[ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB] == 0);
            assert(pControl->writtenCount[ieieDATATYPE_EXPORTEDRETAINEDMSG] == 0);

            rc = ISMRC_NotFound;
        }

        // No point in keeping this file if there is nothing in it
        if (rc == ISMRC_NotFound)
        {
            unlink(pControl->filePath);

            // Free up the file path so that we don't include it in the status file which
            // we are about to update.
            iemem_free(pThreadData, iemem_exportResources, pControl->filePath);
            pControl->filePath = NULL;
        }
    }

    // Log success or failure.
    if (rc == OK)
    {
        assert(pControl->stringRequestID != NULL);
        LOG(INFO, Messaging, 3015, "%s", "Export of resources with request ID {0} succeeded.",
            pControl->stringRequestID);
    }
    else if (pControl->stringRequestID != NULL)
    {
        LOG(ERROR, Messaging, 3016, "%s%d", "Export of resources with request ID {0} failed with return code {1}.",
            pControl->stringRequestID, rc);
    }
    else
    {
        LOG(ERROR, Messaging, 3016, "%lu%d", "Export of resources with request ID {0} failed with return code {1}.",
            pControl->requestID, rc);
    }

    pControl->dataRecordRC = rc;

    if (pControl->requestID != 0)
    {
        (void)ieie_updateExportStatus(pThreadData, pControl, rc);
    }

    if (pControl->regexClientId != NULL) ism_regex_free(pControl->regexClientId);
    if (pControl->regexTopic != NULL) ism_regex_free(pControl->regexTopic);
    if (pControl->clientIdTable != NULL) ieut_destroyHashTable(pThreadData, pControl->clientIdTable);
    if (pControl->exportedMsgs != NULL)
    {
        ieut_traverseHashSet(pThreadData, pControl->exportedMsgs, ieie_releaseExportedMessage, NULL);
        ieut_destroyHashSet(pThreadData, pControl->exportedMsgs);
    }
    if (pControl->exportWentAsync && pControl->pCallerCBFn)
    {
        pControl->pCallerCBFn(rc, (void *)pControl->requestID, pControl->pCallerContext);
    }

    if (pControl->filePath != NULL)
    {
        ieieDiagnosticInfo_t *pCurDiagnostic = pControl->latestDiagnostic;

        while(pCurDiagnostic != NULL)
        {
            ieieDiagnosticInfo_t *next = pCurDiagnostic->next;
            iemem_free(pThreadData, iemem_exportResources, pCurDiagnostic);
            pCurDiagnostic = next;
        }

        // NOTE: filePath being allocated implies that the diagnostics lock was initialized.
        pthread_rwlock_destroy(&pControl->diagnosticsLock);

        iemem_free(pThreadData, iemem_exportResources, pControl->filePath);
    }

    iemem_free(pThreadData, iemem_exportResources, pControl->statusFilePath);
    iemem_free(pThreadData, iemem_exportResources, pControl);

    DEBUG_ONLY uint32_t oldActiveRequests = __sync_fetch_and_sub(&importExportGlobal->activeRequests, 1);
    assert(oldActiveRequests != 0);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Thread used to perform an export resources request
///
/// @param[in]     pControl               Control structure for the export.
//****************************************************************************
void *ieie_exportThread(void *arg, void *context, int value)
{
    char threadName[16];
    ism_common_getThreadName(threadName, sizeof(threadName));

    ieieImportExportGlobal_t *importExportGlobal = ismEngine_serverGlobal.importExportGlobal;
    ieieExportResourceControl_t *pControl = (ieieExportResourceControl_t *)context;

    // Make sure we're thread-initialised.
    ism_engine_threadInit(0);

    // Not working on behalf of a particular client
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    assert(pControl->exportWentAsync == true);
    assert(pControl->requestID != 0);

    ieutTRACEL(pThreadData, pControl, ENGINE_CEI_TRACE, FUNCTION_ENTRY "Started thread %s with control %p (requestID=%lu)\n",
               __func__, threadName, pControl, pControl->requestID);

    // Log that the export has started
    LOG(INFO, Messaging, 3013, "%-s%s%-s%-s", "Export of resources to file {0} with request ID {1} started. ClientID={2}, Topic={3}.",
        pControl->fileName, pControl->stringRequestID, pControl->clientId ? pControl->clientId : "", pControl->topic ? pControl->topic : "");

    // HashSet to hold all messages that we've exported
    int32_t rc = ieut_createHashSet(pThreadData, iemem_exportResources, &pControl->exportedMsgs);
    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }
    assert(pControl->exportedMsgs);

    // Add a header describing the resource file being created
    rc = ieie_writeResourceFileHeader(pThreadData, pControl);
    if (rc != OK) goto mod_exit;

    assert(pControl->clientSetDisabled == false);

    // Call the transport layer to disable the set of clients we're about to export
    if (pControl->clientId != NULL)
    {
        assert(importExportGlobal->ism_transport_disableClientSet != NULL);
        rc = importExportGlobal->ism_transport_disableClientSet(pControl->clientId, ISMRC_ClosedByServer);
        if (rc != OK)
        {
            ism_common_setError(rc);
            goto mod_exit;
        }
        pControl->clientSetDisabled = true;
    }

    // We have a request which includes a topic specification. Find the messages we're dealing with
    if (pControl->regexTopic != NULL)
    {
        rc = ieie_exportRetainedMsgs(pThreadData, pControl);

        if (rc != OK)
        {
            assert(rc != ISMRC_AsyncCompletion); // Not yet set up to deal with async export
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    // We have a request which includes a clientId specification. Find the clients we're dealing with
    if (pControl->regexClientId != NULL)
    {
        assert(pControl->clientIdTable);

        rc = ieie_getMatchingClientIds(pThreadData, pControl);

        if (rc != OK)
        {
            ism_common_setError(rc);
            goto mod_exit;
        }

        // Export clients and subscriptions found...
        if (pControl->clientIdTable->totalCount != 0)
        {
            rc = ieie_exportClientStates(pThreadData, pControl);

            if (rc != OK)
            {
                assert(rc != ISMRC_AsyncCompletion); // Not yet set up to deal with async export
                ism_common_setError(rc);
                goto mod_exit;
            }

            rc = ieie_exportSubscriptions(pThreadData, pControl);

            if (rc != OK)
            {
                assert(rc != ISMRC_AsyncCompletion); // Not yet set up to deal with async export
                ism_common_setError(rc);
                goto mod_exit;
            }
        }
    }

    // Export inflight messages for included client from multiconsumer queues
    // (messages on single consumer queues will be exported at the same time as the queues)
    if (   pControl->regexClientId != NULL
        && pControl->clientIdTable->totalCount != 0)
    {
        rc = ieie_exportInflightMessages(pThreadData, pControl);

        if (rc != OK)
        {
            assert(rc != ISMRC_AsyncCompletion); // Not yet set up to deal with async export
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

mod_exit:

    assert(rc != ISMRC_AsyncCompletion);

    pControl->dataRecordRC = rc;

    (void)ieie_completeExportResources(pThreadData, pControl);

    ieutTRACEL(pThreadData, pControl, ENGINE_CEI_TRACE, FUNCTION_ENTRY "Ending thread %s with control %p\n",
               __func__, threadName, pControl);

    ieut_leavingEngine(pThreadData);

    // No longer need the thread to be initialized
    ism_engine_threadTerm(1);

    return NULL;
}

//****************************************************************************
/// @internal
///
/// @brief  Thread used to perform an import resources request
///
/// @param[in]     pControl               Control structure for the import.
//****************************************************************************
void *ieie_importThread(void *arg, void *context, int value)
{
    char threadName[16];
    ism_common_getThreadName(threadName, sizeof(threadName));

    ieieImportResourceControl_t *pControl = (ieieImportResourceControl_t *)context;

    // Make sure we're thread-initialised
    ism_engine_threadInit(0);

    // Not working on behalf of a particular client
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    assert(pControl->importWentAsync == true);
    assert(pControl->requestID != 0);

    ieutTRACEL(pThreadData, pControl, ENGINE_CEI_TRACE, FUNCTION_ENTRY "Started thread %s with control %p (requestID=%lu)\n",
               __func__, threadName, pControl, pControl->requestID);

    int32_t unused_rc = ieie_continueImportResources(pThreadData, pControl);

    ieutTRACEL(pThreadData, pControl, ENGINE_CEI_TRACE, FUNCTION_ENTRY "Ending thread %s with control %p (unused_rc=%d)\n",
               __func__, threadName, pControl, unused_rc);

    ieut_leavingEngine(pThreadData);

    // No longer need the thread to be initialized
    ism_engine_threadTerm(1);

    return NULL;
}

//****************************************************************************
/// @internal
///
/// @brief  Start an import / export request thread
///
/// @param[in]     importExportGlobal  Global structure for import/export
/// @param[in]     type                The type of request to add
/// @param[in]     pControl            Control structure to pass to the request
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieie_startRequest(ieutThreadData_t *pThreadData,
                          ieieImportExportGlobal_t *importExportGlobal,
                          ieieRequestType_t type,
                          void *pControl)
{
    int threadRC;
    int32_t rc = OK;
    char threadName[16];

    ieutTRACEL(pThreadData, pControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "(type %d, pControl %p)\n",
               __func__, type, pControl);

    ism_threadh_t threadHandle = 0;

    if (type == ieieREQUEST_EXPORT)
    {
        ieieExportResourceControl_t *pExportControl = (ieieExportResourceControl_t *)pControl;

        if (strlen(pExportControl->stringRequestID) < sizeof(threadName)-strlen(ieieEXPORT_THREADNAME_PREFIX)-1)
        {
            sprintf(threadName, ieieEXPORT_THREADNAME_PREFIX "%s", pExportControl->stringRequestID);
        }
        else
        {
            sprintf(threadName, ieieEXPORT_THREADNAME_PREFIX "thread");
        }

        pExportControl->exportWentAsync = true;

        threadRC = ism_common_startThread(&threadHandle,
                                          ieie_exportThread,
                                          NULL, pControl, 0,
                                          ISM_TUSAGE_NORMAL,
                                          0,
                                          threadName,
                                          "Export_Resources");

        if (threadRC != 0)
        {
            pExportControl->exportWentAsync = false;
            ieutTRACEL(pThreadData, threadRC, ENGINE_ERROR_TRACE,
                       "ism_common_startThread for %s failed with %d\n", threadName, threadRC);
            rc = ISMRC_Error;
            ism_common_setError(rc);
            goto mod_exit;
        }

        assert(threadHandle != 0);

        rc = ISMRC_AsyncCompletion;
    }
    else if (type == ieieREQUEST_IMPORT)
    {
        ieieImportResourceControl_t *pImportControl = (ieieImportResourceControl_t *)pControl;

        if (strlen(pImportControl->stringRequestID) < sizeof(threadName)-strlen(ieieIMPORT_THREADNAME_PREFIX)-1)
        {
            sprintf(threadName, ieieIMPORT_THREADNAME_PREFIX "%s", pImportControl->stringRequestID);
        }
        else
        {
            sprintf(threadName, ieieIMPORT_THREADNAME_PREFIX "thread");
        }

        pImportControl->importWentAsync = true;

        threadRC = ism_common_startThread(&threadHandle,
                                          ieie_importThread,
                                          NULL, pControl, 0,
                                          ISM_TUSAGE_NORMAL,
                                          0,
                                          threadName,
                                          "Import_Resources");

        if (threadRC != 0)
        {
            pImportControl->importWentAsync = false;
            ieutTRACEL(pThreadData, threadRC, ENGINE_ERROR_TRACE,
                       "ism_common_startThread for %s failed with %d\n", threadName, threadRC);
            rc = ISMRC_Error;
            ism_common_setError(rc);
            goto mod_exit;
        }

        assert(threadHandle != 0);

        rc = ISMRC_AsyncCompletion;
    }
    else
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
    }

    // The thread was created joinable, but no-one will join it, so we should detach it now to
    // avoid the associated system resources being leaked.
    if (threadHandle != 0)
    {
        threadRC = ism_common_detachThread(threadHandle);

        if (threadRC != 0)
        {
            ieutTRACEL(pThreadData, threadRC, ENGINE_ERROR_TRACE,
                       "ism_common_detachThread for %s failed with %d, ignoring.\n", threadName, threadRC);
        }
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Export a set of engine resources to a specified file
///
/// Export a set of engine resources specified.
///
/// @param[in]     pClientId        String specifying regular expression of clientIds
///                                 to be exported (may be NULL for no clients).
/// @param[in]     pTopic           String specifying regular expression of retained
///                                 message topics to be exported (may be NULL for no
///                                 retained messages).
/// @param[in]     pFilename        File name to use for the resultant exported data
/// @param[in]     pPassword        Password to use when encrypting the file.
/// @param[in]     options          ismENGINE_EXPORT_RESOURCES_OPTION_* values
/// @param[out]    pRequestID       Pointer to the uint64_t value to receive the
///                                 request identifier.
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The resources exported depend on the specified clientId / topic regular
/// expressions, and the specified options.
///
/// @remark The operation may complete synchronously or asynchronously. If it
/// completes synchronously, the return code indicates the completion
/// status.
///
/// If the operation completes asynchronously, the return code from
/// this function will be ISMRC_AsyncCompletion. The actual
/// completion of the operation will be signalled by a call to the
/// operation-completion callback, if one is provided. When the
/// operation becomes asynchronous, a copy of the context will be
/// made into memory owned by the Engine. This will be freed upon
/// return from the operation-completion callback. Because the
/// completion is asynchronous, the call to the operation-completion
/// callback might occur before this function returns.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_exportResources(
        const char *                    pClientId,
        const char *                    pTopic,
        const char *                    pFilename,
        const char *                    pPassword,
        uint32_t                        options,
        uint64_t *                      pRequestID,
        void *                          pContext,
        size_t                          contextLength,
        ismEngine_CompletionCallback_t  pCallbackFn)
{
    ieieExportResourceControl_t *pControl = NULL;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;
    pthread_rwlockattr_t rwlockattr_init;
    bool rwlockAttrInitialized = false;

    assert(pFilename != NULL);
    assert(pRequestID != NULL);

    *pRequestID = 0; // Make sure we initialize the requestID being returned

    ieutTRACEL(pThreadData, options, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(pClientId '%s', pTopic '%s', pFilename '%s', options 0x%08x)\n", __func__,
               (pClientId != NULL) ? pClientId : "(nil)", (pTopic != NULL) ? pTopic : "(nil)",
               pFilename, options);

    if (strlen(pFilename) == 0)
    {
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(rc, "%s%s", ismENGINE_IMPORTEXPORT_STATUS_FIELD_FILENAME, pFilename);
        goto mod_exit;
    }

    size_t fileNameLength = strlen(pFilename);
    size_t clientIdLength = pClientId == NULL ? 0 : strlen(pClientId);
    size_t topicLength = pTopic == NULL ? 0 : strlen(pTopic);

    if (clientIdLength == 0 && topicLength == 0)
    {
        rc = ISMRC_BadPropertyValue;
        if (pTopic != NULL)
        {
            ism_common_setErrorData(rc, "%s%s", ismENGINE_IMPORTEXPORT_STATUS_FIELD_TOPIC, pTopic);
        }
        else
        {
            ism_common_setErrorData(rc, "%s%s", ismENGINE_IMPORTEXPORT_STATUS_FIELD_CLIENTID, pClientId ? pClientId : "");
        }
        ism_common_setError(rc);
        goto mod_exit;
    }

    ieieImportExportGlobal_t *importExportGlobal = ismEngine_serverGlobal.importExportGlobal;
    assert(importExportGlobal != NULL);

    ismEngine_lockMutex(&importExportGlobal->activeRequestLock)

    if (importExportGlobal->stopCalled == true)
    {
        rc = ISMRC_InvalidOperation;
        ism_common_setError(rc);
        ismEngine_unlockMutex(&importExportGlobal->activeRequestLock);
        goto mod_exit;
    }

    uint32_t activeRequests = __sync_add_and_fetch(&importExportGlobal->activeRequests, 1);

    ismEngine_unlockMutex(&importExportGlobal->activeRequestLock);

    if (activeRequests > importExportGlobal->maxActiveRequests)
    {
        rc = ISMRC_TooManyActiveRequests;
        ism_common_setErrorData(rc, "%u%u", activeRequests, importExportGlobal->maxActiveRequests);
        DEBUG_ONLY uint32_t oldActiveRequests = __sync_fetch_and_sub(&importExportGlobal->activeRequests, 1);
        assert(oldActiveRequests != 0);
        goto mod_exit;
    }

    const char *serverName = ism_common_getServerName();
    const char *serverUID = ism_common_getServerUID();

    size_t serverNameLength = serverName == NULL ? 0 : strlen(serverName);
    size_t serverUIDLength = serverUID == NULL ? 0 : strlen(serverUID);

    pControl = iemem_calloc(pThreadData,
                            IEMEM_PROBE(iemem_exportResources, 9),
                            1,
                            sizeof(ieieExportResourceControl_t) + contextLength +
                            fileNameLength + 1 +
                            serverNameLength + 1 +
                            serverUIDLength + 1 +
                            clientIdLength + 1 +
                            topicLength + 1);

    if (pControl == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        DEBUG_ONLY uint32_t oldActiveRequests = __sync_fetch_and_sub(&importExportGlobal->activeRequests, 1);
        assert(oldActiveRequests != 0);
        goto mod_exit;
    }

    pControl->options = options;
    pControl->serverInitTime = importExportGlobal->serverInitTime;
    pControl->startTime = ism_common_currentTimeNanos();
    pControl->pCallerCBFn = pCallbackFn;

    char *extraData = (char *)(pControl+1);

    if (contextLength != 0)
    {
        memcpy((void *)extraData, pContext, contextLength);
        pControl->pCallerContext = (void *)extraData;
        extraData += contextLength;
    }

    if (fileNameLength != 0)
    {
        fileNameLength += 1;
        pControl->fileName = extraData;
        memcpy(pControl->fileName, pFilename, fileNameLength);
        extraData += fileNameLength;
    }

    if (serverNameLength != 0)
    {
        serverNameLength += 1; // Include the null terminator
        pControl->exportServerName = extraData;
        memcpy(pControl->exportServerName, serverName, serverNameLength);
        extraData += serverNameLength;
    }

    if (serverUIDLength != 0)
    {
        serverUIDLength += 1; // Include the null terminator
        pControl->exportServerUID = extraData;
        memcpy(pControl->exportServerUID, serverUID, serverUIDLength);
        extraData += serverUIDLength;
    }

    if (clientIdLength != 0)
    {
        clientIdLength += 1; // Include the null terminator
        pControl->clientId = extraData;
        memcpy(pControl->clientId, pClientId, clientIdLength);
        rc = ism_regex_compile(&pControl->regexClientId, pControl->clientId);
        if (rc != OK)
        {
            ieutTRACEL(pThreadData, rc, ENGINE_WORRYING_TRACE, "ism_regex_compile of '%s' returned %d\n",
                       pControl->clientId, rc);
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", ismENGINE_IMPORTEXPORT_STATUS_FIELD_CLIENTID, pControl->clientId);
            goto mod_exit;
        }
        assert(pControl->regexClientId != NULL);
        extraData += clientIdLength;
    }

    if (topicLength != 0)
    {
        topicLength += 1; // Include the null terminator
        pControl->topic = extraData;
        memcpy(pControl->topic, pTopic, topicLength);
        rc = ism_regex_compile(&pControl->regexTopic, pControl->topic);
        if (rc != OK)
        {
            ieutTRACEL(pThreadData, rc, ENGINE_WORRYING_TRACE, "ism_regex_compile of '%s' returned %d\n",
                       pControl->topic, rc);
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", ismENGINE_IMPORTEXPORT_STATUS_FIELD_TOPIC, pControl->topic);
            goto mod_exit;
        }
        assert(pControl->regexTopic != NULL);
        extraData += topicLength;
    }

    // Initilaize the rwlock attr structure for our read/write locks
    int osrc = pthread_rwlockattr_init(&rwlockattr_init);
    if (osrc) goto mod_exit;
    rwlockAttrInitialized = true;

    osrc = pthread_rwlockattr_setkind_np(&rwlockattr_init, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    if (osrc) goto mod_exit;

    // Initialize the diagnostics lock
    osrc = pthread_rwlock_init(&pControl->diagnosticsLock, &rwlockattr_init);
    if (osrc) goto mod_exit;

    rc = ieie_fullyQualifyResourceFilename(pThreadData, pFilename, false, &pControl->filePath);
    if (rc != OK) goto mod_exit;

    assert(pControl->filePath != NULL);

    // If the caller didn't specify the overwrite option, check that the requested file
    // doesn't already exist.
    if ((pControl->options & ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE) == 0)
    {
        struct stat statBuf;

        if (stat(pControl->filePath, &statBuf) == 0)
        {
            rc = ISMRC_FileAlreadyExists;
            ism_common_setErrorData(rc, "%s", pFilename);
            goto mod_exit;
        }
    }

    // Create the encrypted file
    pControl->file = ieie_createEncryptedFile(pThreadData,
                                              iemem_exportResources,
                                              pControl->filePath, pPassword);
    if (pControl->file == NULL)
    {
        rc = ISMRC_FileCorrupt;
        ism_common_setError(rc);
        goto mod_exit;
    }

    rc = ieie_allocateRequestId(pThreadData, importExportGlobal, false, (void *)pControl);
    if (rc != OK) goto mod_exit;

    assert(pControl->requestID != 0);
    assert(pControl->stringRequestID != NULL);

    *pRequestID = pControl->requestID;

    if (clientIdLength != 0)
    {
        // Table to hold all matching clientIds
        rc = ieut_createHashTable(pThreadData,
                                  ieieIMPORTEXPORT_INITIAL_EXPORT_CLIENTID_TABLE_CAPACITY,
                                  iemem_exportResources,
                                  &pControl->clientIdTable);
        if (rc != OK)
        {
            ism_common_setError(rc);
            goto mod_exit;
        }
        assert(pControl->clientIdTable != NULL);
    }

    rc = ieie_startRequest(pThreadData, importExportGlobal, ieieREQUEST_EXPORT, pControl);

mod_exit:

    if (rwlockAttrInitialized) (void)pthread_rwlockattr_destroy(&rwlockattr_init);

    if (rc != ISMRC_AsyncCompletion && pControl != NULL)
    {
        assert(pControl->exportWentAsync == false);

        pControl->dataRecordRC = rc;

        rc = ieie_completeExportResources( pThreadData
                                         , pControl);

    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "*pRequestID=%lu, rc=%d\n", __func__, *pRequestID, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Validate that the file being imported is a valid and consistent resource
///         file
///
/// @param[in]     pControl     ieieImportResourceControl_t of the import request
/// @param[in]     pPassword    Password to use with the file
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieie_validateExportedResourceFile(ieutThreadData_t *pThreadData,
                                          ieieImportResourceControl_t *pControl,
                                          const char *pPassword)
{
    int32_t rc = OK;
    int32_t validationRC = OK;
    uint64_t headerVersion = 0;
    uint64_t headerRequestID = 0;

    ieutTRACEL(pThreadData, pControl->filePath, ENGINE_FNC_TRACE, FUNCTION_ENTRY "(filePath '%s')\n",
               __func__, pControl->filePath);

    ieieEncryptedFileHandle_t file = ieie_OpenEncryptedFile(pThreadData,
                                                            iemem_exportResources,
                                                            pControl->filePath, pPassword);
    if (file == NULL)
    {
        rc = ISMRC_FileCorrupt;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Read in the 1st record, which we expect to be a header
    rc = ieie_importData(pThreadData,
                         file,
                         &pControl->dataType,
                         &pControl->dataId,
                         &pControl->dataLen,
                         (void **)&pControl->data);

    uint64_t headerDataId = 0;

    if (pControl->dataType != ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER)
    {
        rc = ISMRC_FileCorrupt;
        ism_common_setError(rc);
        goto mod_exit;
    }

    ieieResourceFileHeader_t *header = (ieieResourceFileHeader_t *)(pControl->data);

    headerVersion = header->Version;
    headerRequestID = header->RequestID;

    // Can handle resource files of this or earlier version, anything newer will contain unknown
    // fields.
    if (LIKELY(headerVersion <= ieieRESOURCEFILE_CURRENT_VERSION))
    {
        headerDataId = pControl->dataId;

        char *extraData = (char *)(header+1);

        if (header->ServerNameLength != 0)
        {
            pControl->exportServerName = iemem_malloc(pThreadData,
                                                      IEMEM_PROBE(iemem_exportResources, 23),
                                                      header->ServerNameLength);

            if (pControl->exportServerName == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            memcpy(pControl->exportServerName, extraData, header->ServerNameLength);
            extraData += header->ServerNameLength;
        }

        if (rc == OK && header->ServerUIDLength != 0)
        {
            pControl->exportServerUID = iemem_malloc(pThreadData,
                                                     IEMEM_PROBE(iemem_exportResources, 24),
                                                     header->ServerUIDLength);

            if (pControl->exportServerUID == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            memcpy(pControl->exportServerUID, extraData, header->ServerUIDLength);
            extraData += header->ServerUIDLength;
        }

        if (header->ClientIdLength != 0)
        {
            pControl->clientId = iemem_malloc(pThreadData,
                                              IEMEM_PROBE(iemem_exportResources, 21),
                                              header->ClientIdLength);

            if (pControl->clientId == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            memcpy(pControl->clientId, extraData, header->ClientIdLength);
            extraData += header->ClientIdLength;
        }

        if (header->TopicLength != 0)
        {
            pControl->topic = iemem_malloc(pThreadData,
                                           IEMEM_PROBE(iemem_exportResources, 22),
                                           header->TopicLength);

            if (pControl->topic == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            memcpy(pControl->topic, extraData, header->TopicLength);
            extraData += header->TopicLength;
        }
    }
    // Will need to extend this to cover future versions of the structure
    else
    {
        rc = ISMRC_FileCorrupt;
        ism_common_setError(rc);
        goto mod_exit;
    }

    ieieResourceFileFooter_t *footer = NULL;

    // Assert that no new types have been added to ieieDataType_t
    // (if they have, but don't need to be catered for, just update this
    // assert so that it is correct)
    assert(ieieDATATYPE_LAST == ieieDATATYPE_DIAGSUBDETAILSFOOTER + 1);

    while(rc == OK)
    {
        // Remember what the last record successfully checked was
        pControl->validatedCount[pControl->dataType] += 1;

        rc = ieie_importData(pThreadData,
                             file,
                             &pControl->dataType,
                             &pControl->dataId,
                             &pControl->dataLen,
                             (void **)&pControl->data);

        if (rc == OK)
        {
            switch(pControl->dataType)
            {
                case ieieDATATYPE_EXPORTEDCLIENTSTATE:
                    // Don't expect to see anything after the footer
                    if (footer != NULL)
                    {
                        rc = ISMRC_FileCorrupt;
                        ism_common_setError(rc);
                    }
                    else
                    {
                        rc = ieieValidateClientStateImport(pThreadData,
                                                           pControl,
                                                           pControl->dataId,
                                                           pControl->data,
                                                           pControl->dataLen);

                        // Want to remember that this client was in use, but keep on
                        // checking so we can report them all
                        if (rc == ISMRC_ClientIDInUse && validationRC == OK)
                        {
                            validationRC = rc;
                            rc = OK;
                        }
                    }
                    break;
                case ieieDATATYPE_EXPORTEDSUBSCRIPTION:
                    // No specific validation
                case ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB:
                    // No specific validation
                case ieieDATATYPE_EXPORTEDMESSAGERECORD:
                case ieieDATATYPE_EXPORTEDQNODE_SIMPLE:
                case ieieDATATYPE_EXPORTEDQNODE_INTER:
                case ieieDATATYPE_EXPORTEDQNODE_MULTI:
                case ieieDATATYPE_EXPORTEDQNODE_MULTI_INPROG:
                case ieieDATATYPE_EXPORTEDRETAINEDMSG:
                    // Don't expect to see anything after the footer
                    if (footer != NULL)
                    {
                        rc = ISMRC_FileCorrupt;
                        ism_common_setError(rc);
                    }
                    break;
                case ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER:
                    if (pControl->dataId != headerDataId)
                    {
                        rc = ISMRC_FileCorrupt;
                        ism_common_setError(rc);
                        break;
                    }

                    footer = (ieieResourceFileFooter_t *)(pControl->data);
                    if (footer->Version != headerVersion)
                    {
                        rc = ISMRC_FileCorrupt;
                        ism_common_setError(rc);
                    }
                    if (footer->RequestID != headerRequestID)
                    {
                        rc = ISMRC_FileCorrupt;
                        ism_common_setError(rc);
                    }
                    break;
                // Don't expect to see another header
                case ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER:
                    rc = ISMRC_FileCorrupt;
                    ism_common_setError(rc);
                    break;
                default:
                    rc = ISMRC_FileCorrupt;
                    ism_common_setError(rc);
                    break;
            }
        }
    }

    // We read to the end of the file - if everything looks good we consider the file valid
    if (rc == ISMRC_EndOfFile)
    {
        if (pControl->validatedCount[ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER] == 1 &&
            pControl->validatedCount[ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER] == 1)
        {
            rc = OK;
        }
        else
        {
            rc = ISMRC_FileCorrupt;
            ism_common_setError(rc);
        }
    }

mod_exit:

    // What return code should we return from the validation
    if (validationRC == OK) validationRC = rc;

    if (file != NULL) ieie_finishReadingEncryptedFile(pThreadData, file);

    ieutTRACEL(pThreadData, validationRC,  ENGINE_FNC_TRACE,
               FUNCTION_EXIT "validationRC=%d headerVersion=%lu headerRequestID=%lu\n",
               __func__, validationRC, headerVersion, headerRequestID);

    return validationRC;
}

//****************************************************************************
/// @internal
///
/// @brief  Complete the import of a set of resources.
///
/// @param[in]     pControl               Control structure for the import.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static int32_t  ieie_completeImportResources( ieutThreadData_t *pThreadData
                                            , ieieImportResourceControl_t *pControl)
{
    ieutTRACEL(pThreadData, pControl,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pControl=%p\n", __func__,
                      pControl);

    assert(pControl != NULL);

    int32_t rc = pControl->dataRecordRC;

    ieieImportExportGlobal_t *importExportGlobal = ismEngine_serverGlobal.importExportGlobal;

    assert(importExportGlobal != NULL);

    // We read to the end of the file - if everything looks good we can finalize
    if (rc == ISMRC_EndOfFile)
    {
        assert(pControl->recCountStarted[ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER] == 1);
        assert(pControl->recCountStarted[ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER] == 1);
        for(int32_t i=0; i<ieieDATATYPE_LAST; i++)
        {
            assert(pControl->recCountFinished[i] == pControl->validatedCount[i]);
            assert(pControl->recCountFinished[i] == pControl->validatedCount[i]);
        }

        rc = OK;
    }
    else if (rc == OK)
    {
        rc = ISMRC_FileCorrupt;
        ism_common_setError(rc);
    }

    // Log success or failure.
    if (rc == OK)
    {
        assert(pControl->stringRequestID != NULL);

        LOG(INFO, Messaging, 3019, "%s", "Import of resources with request ID {0} succeeded.",
                pControl->stringRequestID);
    }
    else if (pControl->stringRequestID != NULL)
    {
        LOG(ERROR, Messaging, 3020, "%s%d", "Import of resources with request ID {0} failed with return code {1}.",
            pControl->stringRequestID, rc);
    }
    else
    {
        LOG(ERROR, Messaging, 3020, "%lu%d", "Import of resources with request ID {0} failed with return code {1}.",
            pControl->requestID, rc);
    }

    pControl->endTime = ism_common_currentTimeNanos();

    if (pControl->file != NULL) (void)ieie_finishReadingEncryptedFile(pThreadData, pControl->file);
    if (pControl->importedSubscriptions != NULL)
    {
        ieieReleaseImportedSubContext_t context = {0, ismEngine_serverGlobal.maintree};

        int32_t rc2 = ieut_traverseHashTableWithRC(pThreadData,
                                                   pControl->importedSubscriptions,
                                                   ieie_releaseImportedSubscription,
                                                   &context);

        if (rc == OK) rc = rc2;

        // Invalidate any caches if we changed some subscriptions
        if (context.releasedCount != 0)
        {
            ismEngine_getRWLockForWrite(&context.tree->subsLock);
            context.tree->subsUpdates += 1;
            ismEngine_unlockRWLock(&context.tree->subsLock);
        }

        ieut_destroyHashTable(pThreadData, pControl->importedSubscriptions);
    }

    if (pControl->requestID != 0)
    {
        (void)ieie_updateImportStatus(pThreadData, pControl, rc);
    }

    if (pControl->importedClientStates != NULL)
    {
        ieut_traverseHashTable(pThreadData,
                               pControl->importedClientStates,
                               ieie_releaseImportedClientState,
                               pControl);
        ieut_destroyHashTable(pThreadData, pControl->importedClientStates);
    }

    if (pControl->validatedClientIds != NULL)
    {
        if (pControl->validatedClientIds->totalCount != 0)
        {
            assert(importExportGlobal->activeImportClientIdTable != NULL);
            assert(importExportGlobal->activeImportClientIdTable->totalCount != 0);

            ieut_traverseHashTable(pThreadData,
                                   pControl->validatedClientIds,
                                   ieie_releaseValidatedClientId,
                                   importExportGlobal->activeImportClientIdTable);
        }

        ieut_destroyHashTable(pThreadData, pControl->validatedClientIds);
    }

    if (pControl->importedMsgs != NULL)
    {
        assert(pthread_rwlock_trywrlock(&pControl->importedTablesLock) == 0);

        ieut_traverseHashTable(pThreadData, pControl->importedMsgs, ieie_releaseImportedMessage, NULL);
        ieut_destroyHashTable(pThreadData, pControl->importedMsgs);

        pthread_rwlock_destroy(&pControl->importedTablesLock);
    }

    if (pControl->filePath != NULL)
    {
        ieieDiagnosticInfo_t *pCurDiagnostic = pControl->latestDiagnostic;

        while(pCurDiagnostic != NULL)
        {
            ieieDiagnosticInfo_t *next = pCurDiagnostic->next;
            iemem_free(pThreadData, iemem_exportResources, pCurDiagnostic);
            pCurDiagnostic = next;
        }

        // NOTE: filePath being allocated implies that the diagnostics lock was initialized.
        pthread_rwlock_destroy(&pControl->diagnosticsLock);

        iemem_free(pThreadData, iemem_exportResources, pControl->filePath);
    }

    ieie_freeReadExportedData(pThreadData, iemem_exportResources, pControl->data);
    iemem_free(pThreadData, iemem_exportResources, pControl->statusFilePath);
    if (pControl->importWentAsync && pControl->pCallerCBFn)
    {
        pControl->pCallerCBFn(rc, (void *)pControl->requestID, pControl->pCallerContext);
    }
    iemem_free(pThreadData, iemem_exportResources, pControl->exportServerName);
    iemem_free(pThreadData, iemem_exportResources, pControl->exportServerUID);
    iemem_free(pThreadData, iemem_exportResources, pControl->clientId);
    iemem_free(pThreadData, iemem_exportResources, pControl->topic);
    // Inform the clientState expiry thread that there it might need to schedule work.
    if (pControl->lowestClientStateExpiryCheckTime != 0)
    {
        iece_checkTimeWithScheduledScan(pThreadData, pControl->lowestClientStateExpiryCheckTime);
    }
    iemem_free(pThreadData, iemem_exportResources, pControl);

    DEBUG_ONLY uint32_t oldActiveRequests = __sync_fetch_and_sub(&importExportGlobal->activeRequests, 1);
    assert(oldActiveRequests != 0);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Decide whether this failure should abort the import.
///
/// @param[in]     pControl               Control structure for the import.
/// @param[in]     recordType             The record dataType causing the error.
/// @param[in]     rc                     The error rc.
///
/// @return true if the import should be aborted, otherwise false.
//****************************************************************************
static bool ieie_doesErrorAbortImport( ieutThreadData_t *pThreadData
                                     , ieieImportResourceControl_t *pControl
                                     , ieieDataType_t recordType
                                     , int32_t rc)
{
    bool abortImport;

    if (rc == ISMRC_NonDurableImport && (recordType == ieieDATATYPE_EXPORTEDSUBSCRIPTION ||
                                         recordType == ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB))
    {
        abortImport = false;
    }
    else
    {
        abortImport = true;
    }
    return abortImport;
}

//****************************************************************************
/// @internal
///
/// @brief  Record an error which occurred during import.
///
/// @param[in]     pControl               Control structure for the import.
/// @param[in]     recordType             The record dataType causing the error.
/// @param[in]     internalImportDataId   Numeric record data identifier
/// @param[in]     humanIdentifier        String to help identify the record
///                                       may be NULL.
/// @param[in]     rc                     The error rc.
//****************************************************************************
void ieie_recordImportError( ieutThreadData_t *pThreadData
                           , ieieImportResourceControl_t *pControl
                           , ieieDataType_t recordType
                           , uint64_t internalImportDataId
                           , const char *humanIdentifier
                           , int32_t rc)
{
    ieutTRACEL(pThreadData, pControl,  ENGINE_ERROR_TRACE, FUNCTION_IDENT
            "pControl=%p recordType=%u dataId=0x%0lx rc=%d %s\n", __func__,
                   pControl, recordType, internalImportDataId, rc, humanIdentifier);

    if (ieie_doesErrorAbortImport(pThreadData, pControl, recordType, rc))
    {
        if (pControl->dataRecordRC == OK) pControl->dataRecordRC = rc;

        char stringBuffer[100];
        char *typeString = NULL;

        // Resource Type
        switch(recordType)
        {
            case ieieDATATYPE_EXPORTEDCLIENTSTATE:
                typeString = ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_CLIENT;
                break;
            case ieieDATATYPE_EXPORTEDRETAINEDMSG:
                typeString = ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_RETAINEDMSG;
                break;
            case ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB:
            case ieieDATATYPE_EXPORTEDSUBSCRIPTION:
            case ieieDATATYPE_EXPORTEDQNODE_SIMPLE:
            case ieieDATATYPE_EXPORTEDQNODE_INTER:
            case ieieDATATYPE_EXPORTEDQNODE_MULTI:
            case ieieDATATYPE_EXPORTEDQNODE_MULTI_INPROG:
                typeString = ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_SUBSCRIPTION;
                break;
            default:
                sprintf(stringBuffer,
                        ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_UNKNOWN_STEM "(%d)",
                        recordType);
                typeString = stringBuffer;
                break;
        }

        // Log the fact that we had an error
        assert(pControl->stringRequestID != NULL);

        LOG(ERROR, Messaging, 3018, "%-s%-s%s%d",
                "Import of resource type {0} with identifier {1} as part of request ID {2} failed with return code {3}.",
                typeString, humanIdentifier ? humanIdentifier : "", pControl->stringRequestID, rc);

        // Add this failure to the list of diagnostics
        size_t diagnosticInfoSize = sizeof(ieieDiagnosticInfo_t) +
                                    (humanIdentifier == NULL ? 0 : strlen(humanIdentifier) + 1);

        ieieDiagnosticInfo_t *newDiagnostic = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_exportResources, 17), diagnosticInfoSize);

        if (newDiagnostic)
        {
            newDiagnostic->resourceDataType = recordType;
            newDiagnostic->resourceDataId = internalImportDataId;
            if (humanIdentifier != NULL)
            {
                newDiagnostic->resourceIdentifier = (char *)(newDiagnostic+1);
                strcpy(newDiagnostic->resourceIdentifier, humanIdentifier);
            }
            else
            {
                newDiagnostic->resourceIdentifier = NULL;
            }
            newDiagnostic->resourceRC = rc;

            ismEngine_getRWLockForWrite(&pControl->diagnosticsLock);

            newDiagnostic->next = pControl->latestDiagnostic;
            pControl->latestDiagnostic = newDiagnostic;

            ismEngine_unlockRWLock(&pControl->diagnosticsLock);
        }
        else
        {
            ieutTRACEL(pThreadData, internalImportDataId, ENGINE_SEVERE_ERROR_TRACE,
                       "Failed to allocate memory for pControl=%p recordType=%u dataId=0x%0lx %s\n",
                       pControl, recordType, internalImportDataId, humanIdentifier);
        }

        pControl->readRecordsCompleted = true;
    }
}

//****************************************************************************
/// @internal
///
/// @brief  Decide whether this failure should abort the export.
///
/// @param[in]     pControl               Control structure for the export.
/// @param[in]     recordType             The record dataType causing the error.
/// @param[in]     rc                     The error rc.
///
/// @return true if the export should be aborted, otherwise false.
//****************************************************************************
static bool ieie_doesErrorAbortExport( ieutThreadData_t *pThreadData
                                     , ieieExportResourceControl_t *pControl
                                     , ieieDataType_t recordType
                                     , int32_t rc)
{
    bool abortExport = true;

    // At the moment, every error aborts the export

    return abortExport;
}

//****************************************************************************
/// @internal
///
/// @brief  Record an error which occurred during export.
///
/// @param[in]     pControl               Control structure for the export.
/// @param[in]     recordType             The record dataType causing the error.
/// @param[in]     internalExportDataId   Numeric record data identifier
/// @param[in]     humanIdentifier        String to help identify the record or
///                                       problem may be NULL.
/// @param[in]     rc                     The error rc.
//****************************************************************************
void ieie_recordExportError( ieutThreadData_t *pThreadData
                           , ieieExportResourceControl_t *pControl
                           , ieieDataType_t recordType
                           , uint64_t internalExportDataId
                           , const char *humanIdentifier
                           , int32_t rc)
{
    ieutTRACEL(pThreadData, pControl,  ENGINE_ERROR_TRACE, FUNCTION_IDENT
            "pControl=%p recordType=%u dataId=0x%0lx rc=%d %s\n", __func__,
                   pControl, recordType, internalExportDataId, rc, humanIdentifier);

    if (ieie_doesErrorAbortExport(pThreadData, pControl, recordType, rc))
    {
        if (pControl->dataRecordRC == OK) pControl->dataRecordRC = rc;

        char stringBuffer[100];
        char *typeString = NULL;

        // Resource Type
        switch(recordType)
        {
            case ieieDATATYPE_EXPORTEDCLIENTSTATE:
                typeString = ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_CLIENT;
                break;
            case ieieDATATYPE_EXPORTEDRETAINEDMSG:
                typeString = ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_RETAINEDMSG;
                break;
            case ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB:
            case ieieDATATYPE_EXPORTEDSUBSCRIPTION:
            case ieieDATATYPE_EXPORTEDQNODE_SIMPLE:
            case ieieDATATYPE_EXPORTEDQNODE_INTER:
            case ieieDATATYPE_EXPORTEDQNODE_MULTI:
            case ieieDATATYPE_EXPORTEDQNODE_MULTI_INPROG:
                typeString = ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_SUBSCRIPTION;
                break;
            default:
                sprintf(stringBuffer,
                        ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_UNKNOWN_STEM "(%d)",
                        recordType);
                typeString = stringBuffer;
                break;
        }

        // Log the fact that we had an error
        assert(pControl->stringRequestID != NULL);

        LOG(ERROR, Messaging, 3014, "%-s%-s%s%d",
            "Export of resource type {0} with identifier {1} as part of request ID {2} failed with return code {3}.",
            typeString, humanIdentifier ? humanIdentifier : "", pControl->stringRequestID, rc);

        // Add this failure to the list of diagnostics
        size_t diagnosticInfoSize = sizeof(ieieDiagnosticInfo_t) +
                                    (humanIdentifier == NULL ? 0 : strlen(humanIdentifier) + 1);

        ieieDiagnosticInfo_t *newDiagnostic = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_exportResources, 25), diagnosticInfoSize);

        if (newDiagnostic)
        {
            newDiagnostic->resourceDataType = recordType;
            newDiagnostic->resourceDataId = internalExportDataId;
            if (humanIdentifier != NULL)
            {
                newDiagnostic->resourceIdentifier = (char *)(newDiagnostic+1);
                strcpy(newDiagnostic->resourceIdentifier, humanIdentifier);
            }
            else
            {
                newDiagnostic->resourceIdentifier = NULL;
            }
            newDiagnostic->resourceRC = rc;

            ismEngine_getRWLockForWrite(&pControl->diagnosticsLock);

            newDiagnostic->next = pControl->latestDiagnostic;
            pControl->latestDiagnostic = newDiagnostic;

            ismEngine_unlockRWLock(&pControl->diagnosticsLock);
        }
        else
        {
            ieutTRACEL(pThreadData, internalExportDataId, ENGINE_SEVERE_ERROR_TRACE,
                       "Failed to allocate memory for pControl=%p recordType=%u dataId=0x%0lx %s\n",
                       pControl, recordType, internalExportDataId, humanIdentifier);
        }
    }
}

//****************************************************************************
/// @internal
///
/// @brief  Increment the finished count for the specified record type.
///
/// @param[in]     pControl           Control structure for the import.
/// @param[in]     recordType         The record dataType whose finish count
///                                   to increment.
//****************************************************************************
void ieie_finishImportRecord(ieutThreadData_t *pThreadData,
                             ieieImportResourceControl_t *pControl,
                             ieieDataType_t recordType )
{
    __sync_add_and_fetch(&(pControl->recCountFinished[recordType]), 1);
}

//****************************************************************************
/// @internal
///
/// @brief  Return whether we are ready to process this type of record at this
/// stage of the import.
///
/// @param[in]     pControl               Control structure for the import.
///
/// @remark Some artifacts in an import file will require the creation of others
/// to have completed before they can be created, and others can be processed in
/// parallel. This function is used to determine, for the current record in the
/// pControl whether we need to wait or can process it now.
///
/// @return true if we can process this record now, or false.
//****************************************************************************
static bool ieie_readyForRecord( ieutThreadData_t *pThreadData
                               , ieieImportResourceControl_t *pControl )
{
    bool readyForRecord=false;

    assert(pControl->dataReady == true);

    switch(pControl->dataType)
    {
        //Types we can always read....
        case ieieDATATYPE_EXPORTEDMESSAGERECORD:
        case ieieDATATYPE_EXPORTEDCLIENTSTATE:
        case ieieDATATYPE_EXPORTEDRETAINEDMSG:
            //Always can read a message/client - has no pre-reqs
            readyForRecord = true;
#ifndef NDEBUG
            if (pControl->dataType == ieieDATATYPE_EXPORTEDRETAINEDMSG)
            {
                //We ought to wait for messages but they don't go async...
                assert(    pControl->recCountStarted[ieieDATATYPE_EXPORTEDMESSAGERECORD]
                        == pControl->recCountFinished[ieieDATATYPE_EXPORTEDMESSAGERECORD]);
            }
#endif
            break;

        case ieieDATATYPE_EXPORTEDQNODE_SIMPLE:
        case ieieDATATYPE_EXPORTEDQNODE_INTER:
        case ieieDATATYPE_EXPORTEDQNODE_MULTI:
        case ieieDATATYPE_EXPORTEDQNODE_MULTI_INPROG:
            //qnodes just need to wait for subs (and one day queues),
            //the subs will wait for clients
            if (   (    pControl->recCountStarted[ieieDATATYPE_EXPORTEDSUBSCRIPTION]
                     == pControl->recCountFinished[ieieDATATYPE_EXPORTEDSUBSCRIPTION])
                && (    pControl->recCountStarted[ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB]
                     == pControl->recCountFinished[ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB]))
            {
                readyForRecord=true;
            }

            //We ought to wait for messages but they don't go async...
            assert(    pControl->recCountStarted[ieieDATATYPE_EXPORTEDMESSAGERECORD]
                    == pControl->recCountFinished[ieieDATATYPE_EXPORTEDMESSAGERECORD]);
            break;

        // Can proceed with subscriptions as long as no clientStates are outstanding...
        case ieieDATATYPE_EXPORTEDSUBSCRIPTION:
        case ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB:
            readyForRecord=(pControl->recCountStarted[ieieDATATYPE_EXPORTEDCLIENTSTATE] ==
                            pControl->recCountFinished[ieieDATATYPE_EXPORTEDCLIENTSTATE]);
            break;

        //Types we can't read in parallel with anything else
        default:
            readyForRecord=true;

            for (size_t i = 0; i<ieieDATATYPE_LAST; i++)
            {
                if (pControl->recCountStarted[i] != pControl->recCountFinished[i])
                {
                    readyForRecord=false;
                    break;
                }
            }
            break;
    }

    return readyForRecord;
}

//****************************************************************************
/// @internal
///
/// @brief  Read the next record from the import file, and decide whether it
/// should be processed yet.
///
/// @param[in]     pControl               Control structure for the import.
/// @param[out]    pProcessRecord         Pointer to return boolean of whether
///                                       this record should be processed now.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieie_readImportRecord(ieutThreadData_t *pThreadData,
                              ieieImportResourceControl_t *pControl,
                              bool *pProcessRecord)
{
    int32_t rc = OK;
    bool processRecord = false;

    assert(pProcessRecord != NULL);

    if (!pControl->dataReady)
    {
        rc = ieie_importData(pThreadData,
                             pControl->file,
                             &pControl->dataType,
                             &pControl->dataId,
                             &pControl->dataLen,
                             (void **)&pControl->data);

        if (rc == OK)
        {
            pControl->dataReady = true;
        }
    }

    if (rc == OK && pControl->dataReady)
    {
        if (ieie_readyForRecord(pThreadData, pControl))
        {
            pControl->recordsRead += 1;

            if ((pControl->recordsRead % ieieIMPORT_UPDATE_FREQUENCY) == 0)
            {
                rc = ieie_updateImportStatus(pThreadData, pControl, rc);
            }

            processRecord = true;
        }
        else
        {
            assert(processRecord == false);
        }
    }

    *pProcessRecord = processRecord;

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Start a potentially async task during import
///
/// @param[in]     pControl               Control structure for the import.
//****************************************************************************
static void ieie_importTaskStart( ieutThreadData_t *pThreadData
                                , ieieImportResourceControl_t *pControl )
{
    uint64_t newNumTasks = __sync_add_and_fetch(&(pControl->importTasksInProgress), 1);

    ieutTRACEL(pThreadData, newNumTasks,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "newNumTasks=%lu\n", __func__,
                   newNumTasks);
}

//****************************************************************************
/// @internal
///
/// @brief  Report a (potentially) async import task as finished.
///
/// @param[in]     pControl               Control structure for the import.
/// @param[in]     doRestartIfNecessary   Whether to resume import if needed
/// @param[out]    pRestartNeeded         Request for whether restart is needed
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieie_importTaskFinish( ieutThreadData_t *pThreadData
                             , ieieImportResourceControl_t *pControl
                             , bool doRestartIfNecessary
                             , bool *pRestartNeeded)
{
    uint64_t  newNumTasks = __sync_sub_and_fetch(&(pControl->importTasksInProgress), 1);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, newNumTasks,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "newNumTasks=%lu\n", __func__,
                   newNumTasks);

    if (newNumTasks == 0)
    {
        newNumTasks = __sync_add_and_fetch(&(pControl->importTasksInProgress), 1);
        assert(newNumTasks == 1);

        if (doRestartIfNecessary)
        {
            //Hmmm the main import thread finished
            pControl->importWentAsync = true;

            rc = ieie_continueImportResources( pThreadData
                                              , pControl );
        }
        else
        {
            //If they won't let us do the restart, we need to tell them to do it
            assert(pRestartNeeded != NULL);
            ieutTRACEL(pThreadData, pControl,  ENGINE_HIFREQ_FNC_TRACE, "Need restart\n");
            *pRestartNeeded = true;
        }
    }

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Continue with the process of importing resources.
///
/// @param[in]     pControl         Control structure for the import.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static int32_t ieie_continueImportResources( ieutThreadData_t *pThreadData
                                           , ieieImportResourceControl_t *pControl )
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pControl,  ENGINE_FNC_TRACE, FUNCTION_EXIT "pControl=%p\n", __func__,
                   pControl);

    while(rc == OK && !pControl->readRecordsCompleted)
    {
        bool processRecord = true;

        rc = ieie_readImportRecord(pThreadData, pControl, &processRecord);

        if (rc == OK)
        {
            if (processRecord)
            {
                bool taskStarted = false;

                ieutTRACEL(pThreadData, pControl->dataType,  ENGINE_HIGH_TRACE,
                               "type: %u dataId %lu\n", pControl->dataType, pControl->dataId);

                // Remember what the last record we started was...
                pControl->recCountStarted[pControl->dataType] += 1;

                //don't start it again...
                pControl->dataReady = false;

                switch(pControl->dataType)
                {
                    case ieieDATATYPE_EXPORTEDMESSAGERECORD:
                        rc = ieie_importMessage(pThreadData,
                                pControl,
                                pControl->dataId,
                                pControl->data,
                                pControl->dataLen);
                        break;
                    case ieieDATATYPE_EXPORTEDCLIENTSTATE:
                        ieie_importTaskStart(pThreadData, pControl);
                        rc = ieie_importClientState(pThreadData,
                                                    pControl,
                                                    pControl->dataId,
                                                    pControl->data,
                                                    pControl->dataLen);
                        taskStarted = true;
                        break;
                    case ieieDATATYPE_EXPORTEDSUBSCRIPTION:
                    case ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB:
                        ieie_importTaskStart(pThreadData, pControl);
                        rc = ieie_importSubscription(pThreadData,
                                                     pControl,
                                                     pControl->dataType,
                                                     pControl->dataId,
                                                     pControl->data,
                                                     pControl->dataLen);
                        taskStarted = true;
                        break;
                    case ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER:
                    case ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER:
                        // Used during validation.
                        break;
                    case ieieDATATYPE_EXPORTEDQNODE_SIMPLE:
                        rc = ieie_importSimpQNode(pThreadData,
                                pControl,
                                pControl->dataId,
                                pControl->data,
                                pControl->dataLen);

                        //Won't go async.. no store writes
                        assert(rc != ISMRC_AsyncCompletion);
                        break;
                    case ieieDATATYPE_EXPORTEDQNODE_INTER:
                        ieie_importTaskStart(pThreadData, pControl);

                        rc = ieie_importInterQNode(pThreadData,
                                                   pControl,
                                                   pControl->dataId,
                                                   pControl->data,
                                                   pControl->dataLen);
                        taskStarted = true;
                        break;

                    case ieieDATATYPE_EXPORTEDQNODE_MULTI:
                    case ieieDATATYPE_EXPORTEDQNODE_MULTI_INPROG:
                        ieie_importTaskStart(pThreadData, pControl);

                        rc = ieie_importMultiConsumerQNode(pThreadData,
                                                           pControl,
                                                           pControl->dataType,
                                                           pControl->dataId,
                                                           pControl->data,
                                                           pControl->dataLen);
                        taskStarted = true;
                        break;

                    case ieieDATATYPE_EXPORTEDRETAINEDMSG:
                        ieie_importTaskStart(pThreadData, pControl);
                        rc = ieie_importRetainedMsg(pThreadData,
                                pControl,
                                pControl->dataId,
                                pControl->data,
                                pControl->dataLen);
                        taskStarted = true;
                        break;
                    default:
                        assert(false);
                        break;
                }

                if (rc == ISMRC_AsyncCompletion)
                {
                    assert(taskStarted == true);

                    //It's gone async, that's fine returning Aync
                    //would mean the async callback own our count on the
                    //parallel tasks usecount - but it has it's own
                    //separate task there so we can carry on
                    rc = OK;
                }
                else
                {
                    //whether or not it was processed successfully... we are finished processing it
                    ieie_finishImportRecord(pThreadData, pControl, pControl->dataType);

                    if (taskStarted)
                    {
                        bool restartNeeded = false;

                        (void)ieie_importTaskFinish( pThreadData
                                                   , pControl
                                                   , false //don't let it restart - avoid recursion
                                                   , &restartNeeded);

                        if (restartNeeded)
                        {
                            //This thread should still be counting towards the number of running tasks
                            ieutTRACE_FFDC(ieutPROBE_001, true,
                                    "Incorrect number of parallel tasks for import", ISMRC_Error,
                                    "pControl", pControl, sizeof(*pControl),
                                    NULL);
                        }
                    }
                }
            }
            else //else for if(processRecord)
            {
                ieutTRACEL(pThreadData, pControl->dataType,  ENGINE_HIGH_TRACE,
                               "Not ready to read - type: %u dataId %lu\n", pControl->dataType, pControl->dataId);

                //Not ready to read this record... get out of here unless everyone
                //else has finished in the meantime.
                bool restartNeeded = false;

                rc = ieie_importTaskFinish( pThreadData
                        , pControl
                        , false //don't let it restart - avoid recursion
                        , &restartNeeded);

                if (rc == OK)
                {
                    if (restartNeeded)
                    {
                        //Excellent - we're the only thread remaning...carry on and clean up
                    }
                    else
                    {
                        //The last task out will restart reading records and eventually finish asynchronously.
                        rc = ISMRC_AsyncCompletion;
                    }
                }
            }
        }
    }

    //If we've got here (aside from because it's gone async..) check all other tasks are complete and only carry on if they are
    //Getting rc=ISMRC_AsyncCompletion at this stage we take to mean that the callback from that operation "owns" the our count on the
    //parallel tasks usecount - so we'll wait for that to complete before finishing...
    if (rc != ISMRC_AsyncCompletion)
    {
        //We're done reading records... future invocations of this function should just tidy up

        if(pControl->dataRecordRC == OK)
        {
            pControl->dataRecordRC = rc;
        }
        pControl->readRecordsCompleted = OK;

        //This thread is finishing... is it the last
        bool restartNeeded = false;

        rc = ieie_importTaskFinish( pThreadData
                                  , pControl
                                  , false //don't let it restart - avoid recursion
                                  , &restartNeeded);

        if (rc == OK)
        {
            if (restartNeeded)
            {
                //Excellent - we're the only thread remaning...carry on and clean up
            }
            else
            {
                //The last task out will restart this function and finish asynchronously.
                rc = ISMRC_AsyncCompletion;
            }
        }
    }


    if (rc != ISMRC_AsyncCompletion)
    {
        rc = ieie_completeImportResources( pThreadData
                                         , pControl );
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Import a set of engine resources from a file (previously exported
///         with ism_engine_exportResources).
///
/// Import a set of engine resources from the specified encrypted file.
///
/// @param[in]     pFilename        File name of the encrypted exported resources
/// @param[in]     pPassword        Password to use when decrypting the file.
/// @param[in]     options          ismENGINE_IMPORT_RESOURCES_OPTION_* values
/// @param[out]    pRequestID       Pointer to the uint64_t value to receive the
///                                 request identifier.
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The operation may complete synchronously or asynchronously. If it
/// completes synchronously, the return code indicates the completion
/// status.
///
/// If the operation completes asynchronously, the return code from
/// this function will be ISMRC_AsyncCompletion. The actual
/// completion of the operation will be signalled by a call to the
/// operation-completion callback, if one is provided. When the
/// operation becomes asynchronous, a copy of the context will be
/// made into memory owned by the Engine. This will be freed upon
/// return from the operation-completion callback. Because the
/// completion is asynchronous, the call to the operation-completion
/// callback might occur before this function returns.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_importResources(
        const char *                    pFilename,
        const char *                    pPassword,
        uint32_t                        options,
        uint64_t *                      pRequestID,
        void *                          pContext,
        size_t                          contextLength,
        ismEngine_CompletionCallback_t  pCallbackFn)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = ISMRC_NotImplemented;
    pthread_rwlockattr_t rwlockattr_init;
    bool rwlockAttrInitialized = false;

    assert(pFilename != NULL);
    assert(pRequestID != NULL);
    assert(options == ismENGINE_IMPORT_RESOURCES_OPTION_NONE);

    *pRequestID = 0; // Make sure we initialize the requestID being returned

    ieutTRACEL(pThreadData, options, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(pFilename '%s', options 0x%08x)\n", __func__,
               pFilename, options);

    ieieImportResourceControl_t *pControl = NULL;

    if (strlen(pFilename) == 0)
    {
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(rc, "%s%s", ismENGINE_IMPORTEXPORT_STATUS_FIELD_FILENAME, pFilename);
        goto mod_exit;
    }

    ieieImportExportGlobal_t *importExportGlobal = ismEngine_serverGlobal.importExportGlobal;
    assert(importExportGlobal != NULL);

    ismEngine_lockMutex(&importExportGlobal->activeRequestLock)

    if ((volatile bool)importExportGlobal->stopCalled == true)
    {
        rc = ISMRC_InvalidOperation;
        ism_common_setError(rc);
        ismEngine_unlockMutex(&importExportGlobal->activeRequestLock);
        goto mod_exit;
    }

    uint32_t activeRequests = __sync_add_and_fetch(&importExportGlobal->activeRequests, 1);

    ismEngine_unlockMutex(&importExportGlobal->activeRequestLock);

    if (activeRequests > importExportGlobal->maxActiveRequests)
    {
        rc = ISMRC_TooManyActiveRequests;
        ism_common_setErrorData(rc, "%u%u", activeRequests, importExportGlobal->maxActiveRequests);
        DEBUG_ONLY uint32_t oldActiveRequests = __sync_fetch_and_sub(&importExportGlobal->activeRequests, 1);
        assert(oldActiveRequests != 0);
        goto mod_exit;
    }

    const char *serverName = ism_common_getServerName();
    const char *serverUID = ism_common_getServerUID();

    size_t serverNameLength = serverName == NULL ? 0 : strlen(serverName);
    size_t serverUIDLength = serverUID == NULL ? 0 : strlen(serverUID);

    pControl = iemem_calloc(pThreadData,
                            IEMEM_PROBE(iemem_exportResources, 10),
                            1,
                            sizeof(ieieImportResourceControl_t) +
                            contextLength +
                            serverNameLength + 1 +
                            serverUIDLength + 1);

    if (pControl == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        DEBUG_ONLY uint32_t oldActiveRequests = __sync_fetch_and_sub(&importExportGlobal->activeRequests, 1);
        assert(oldActiveRequests != 0);
        goto mod_exit;
    }

    char *extraData = (char *)(pControl+1);

    if (contextLength != 0)
    {
        memcpy((void *)extraData, pContext, contextLength);
        pControl->pCallerContext = (void *)extraData;
        extraData += contextLength;
    }

    if (serverNameLength != 0)
    {
        serverNameLength += 1; // Include the null terminator
        memcpy(extraData, serverName, serverNameLength);
        pControl->importServerName = extraData;
        extraData += serverNameLength;
    }

    if (serverUIDLength != 0)
    {
        serverUIDLength += 1; // Include the null terminator
        memcpy(extraData, serverUID, serverUIDLength);
        pControl->importServerUID = extraData;
        extraData += serverUIDLength;
    }

    pControl->pCallerCBFn = pCallbackFn;
    pControl->importTasksInProgress = 1; //This thread is processing records
    pControl->serverInitTime = importExportGlobal->serverInitTime;
    pControl->startTime = ism_common_currentTimeNanos();

    // Initilaize the rwlock attr structure for our read/write locks
    int osrc = pthread_rwlockattr_init(&rwlockattr_init);
    if (osrc) goto mod_exit;
    rwlockAttrInitialized = true;

    osrc = pthread_rwlockattr_setkind_np(&rwlockattr_init, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    if (osrc) goto mod_exit;

    // Initialize the diagnostics lock
    osrc = pthread_rwlock_init(&pControl->diagnosticsLock, &rwlockattr_init);
    if (osrc) goto mod_exit;

    rc = ieie_fullyQualifyResourceFilename(pThreadData, pFilename, true, &pControl->filePath);
    if (rc != OK)
    {
        pthread_rwlock_destroy(&pControl->diagnosticsLock);
        goto mod_exit;
    }

    assert(pControl->filePath != NULL);

    // Allocate a requestId
    rc = ieie_allocateRequestId(pThreadData, importExportGlobal, true, (void *)pControl);
    if (rc != OK) goto mod_exit;

    assert(pControl->requestID != 0);
    assert(pControl->stringRequestID != NULL);

    // Return this to the caller
    *pRequestID = pControl->requestID;

    // Log that the import has started
    LOG(INFO, Messaging, 3017, "%s%s", "Import with request ID {0} from file \"{1}\" started.",
        pControl->stringRequestID, pFilename);

    // Create a hash table to hold all the clientIds that we've validated as being available for import
    rc = ieut_createHashTable(pThreadData,
                              ieieIMPORTEXPORT_INITIAL_IMPORT_CLIENTID_TABLE_CAPACITY,
                              iemem_exportResources, &pControl->validatedClientIds);
    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Perform some simple validation of the resource file
    rc = ieie_validateExportedResourceFile(pThreadData, pControl, pPassword);
    if (rc != OK) goto mod_exit;

    // Create a lock to serialize access to the 'imported' tables
    osrc = pthread_rwlock_init(&pControl->importedTablesLock, &rwlockattr_init);
    if (osrc) goto mod_exit;

    // Create a hash table to hold all the messages being imported
    uint32_t initialCapacity = ieut_suggestCapacity(pThreadData,
                                                    pControl->validatedCount[ieieDATATYPE_EXPORTEDMESSAGERECORD],
                                                    ieieIMPORTEXPORT_IMPORTED_MSG_TABLE_CAPACITY_LIMIT);

    rc = ieut_createHashTable(pThreadData,
                              initialCapacity,
                              iemem_exportResources, &pControl->importedMsgs);
    if (rc != OK)
    {
        pthread_rwlock_destroy(&pControl->importedTablesLock);
        ism_common_setError(rc);
        goto mod_exit;
    }
    assert(pControl->importedMsgs != NULL);

    // Create a hash table to hold all clientStates being imported
    initialCapacity = ieut_suggestCapacity(pThreadData,
                                           pControl->validatedCount[ieieDATATYPE_EXPORTEDCLIENTSTATE],
                                           ieieIMPORTEXPORT_IMPORTED_CLIENT_TABLE_CAPACITY_LIMIT);

    rc = ieut_createHashTable(pThreadData,
                              initialCapacity,
                              iemem_exportResources, &pControl->importedClientStates);
    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }
    assert(pControl->importedClientStates != NULL);

    // Create a hash table to hold all subscriptions being imported
    initialCapacity = ieut_suggestCapacity(pThreadData,
                                           pControl->validatedCount[ieieDATATYPE_EXPORTEDSUBSCRIPTION]+
                                           pControl->validatedCount[ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB],
                                           ieieIMPORTEXPORT_IMPORTED_SUBSCRIPTION_TABLE_CAPACITY_LIMIT);

    rc = ieut_createHashTable(pThreadData,
                              initialCapacity,
                              iemem_exportResources, &pControl->importedSubscriptions);
    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }
    assert(pControl->importedSubscriptions != NULL);

    // Open the encrypted file
    pControl->file = ieie_OpenEncryptedFile(pThreadData,
                                            iemem_exportResources,
                                            pControl->filePath, pPassword);
    if (pControl->file == NULL)
    {
        rc = ISMRC_FileCorrupt;
        ism_common_setError(rc);
        goto mod_exit;
    }

    rc = ieie_startRequest(pThreadData, importExportGlobal, ieieREQUEST_IMPORT, pControl);

mod_exit:

    if (rwlockAttrInitialized) (void)pthread_rwlockattr_destroy(&rwlockattr_init);

    if (rc != ISMRC_AsyncCompletion && pControl != NULL)
    {
        assert(pControl->importWentAsync == false);

        pControl->dataRecordRC = rc;

        rc = ieie_completeImportResources( pThreadData
                                         , pControl);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "*pRequestID=%lu rc=%d\n", __func__, *pRequestID, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of exportResources.c                                          */
/*                                                                   */
/*********************************************************************/
