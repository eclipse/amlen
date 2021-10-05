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
/* Module Name: test_exportResources.c                               */
/*                                                                   */
/* Description: Main source file for CUnit test of resource export   */
/*                                                                   */
/*********************************************************************/
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "engineInternal.h"
#include "engineHashSet.h"
#include "exportCrypto.h"
#include "exportClientState.h"
#include "exportSubscription.h"
#include "exportRetained.h"
#include "topicTree.h"

#include "test_utils_initterm.h"
#include "test_utils_sync.h"
#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_security.h"
#include "test_utils_client.h"
#include "test_utils_options.h"
#include "test_utils_file.h"
#include "test_utils_task.h"

void test_checkStatusFile(char *impexpFilePath, uint64_t requestID,
                          ismEngine_ImportExportResourceStatusType_t expectedStatus,
                          uint32_t expectedDiagnostics,
                          char *expectedRegExClientID, char *expectedRegExTopic, bool expectServerInfo);

bool test_ieie_updateExportStatus_deleteBeforeUpdate = false;
int32_t ieie_updateExportStatus(ieutThreadData_t *pThreadData,
                                ieieExportResourceControl_t *pControl,
                                int32_t reportRC)
{
    static int32_t (*real_ieie_updateExportStatus)(ieutThreadData_t *, ieieExportResourceControl_t *, int32_t) = NULL;

    if (real_ieie_updateExportStatus == NULL)
    {
        real_ieie_updateExportStatus = dlsym(RTLD_NEXT, "ieie_updateExportStatus");
    }

    if (test_ieie_updateExportStatus_deleteBeforeUpdate && pControl->endTime != 0)
    {
        int rc;
        if (pControl->filePath != NULL)
        {
            test_checkStatusFile(pControl->filePath,
                                 pControl->requestID,
                                 ismENGINE_IMPORTEXPORT_STATUS_IN_PROGRESS, 0, NULL, NULL, true);
            rc = unlink(pControl->filePath);
            TEST_ASSERT_EQUAL(rc, 0);
        }
        rc = unlink(pControl->statusFilePath);
        TEST_ASSERT_EQUAL(rc, 0);
    }

    return real_ieie_updateExportStatus(pThreadData, pControl, reportRC);
}

int test_ism_transport_disableClientSetRC = OK;
int test_ism_transport_disableClientSet(const char * regex_str, int rc)
{
    TEST_ASSERT_PTR_NOT_NULL(regex_str);
    return test_ism_transport_disableClientSetRC;
}

int test_ism_transport_enableClientSetRC = OK;
int test_ism_transport_enableClientSet(const char * regex_str)
{
    TEST_ASSERT_PTR_NOT_NULL(regex_str);
    return test_ism_transport_enableClientSetRC;
}

bool test_EVP_get_cipherbynameFAIL = false;
const EVP_CIPHER *EVP_get_cipherbyname(const char *name)
{
    static const EVP_CIPHER *(*real_EVP_get_cipherbyname)(const char *) = NULL;

    if (real_EVP_get_cipherbyname == NULL)
    {
        real_EVP_get_cipherbyname = dlsym(RTLD_NEXT, "EVP_get_cipherbyname");
    }

    const EVP_CIPHER *retCipher;

    if (test_EVP_get_cipherbynameFAIL == true)
    {
        retCipher = NULL;
    }
    else
    {
        retCipher = real_EVP_get_cipherbyname(name);
    }

    return retCipher;
}

// Publish messages
void test_publishMessages(const char *topicString,
                          const char *serverUID,
                          size_t payloadSize,
                          uint32_t msgCount,
                          uint8_t reliability,
                          uint8_t persistence,
                          uint8_t flags,
                          uint32_t protocolId,
                          char *clientId,
                          ismSecurity_t *secContext,
                          int32_t expectRC)
{
    ismEngine_MessageHandle_t hMessage;
    void *payload=NULL;

    int32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;

    uint32_t clientOpts;
    if (protocolId == PROTOCOL_ID_MQTT || protocolId == PROTOCOL_ID_HTTP)
    {
        clientOpts = ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART;
    }
    else
    {
        clientOpts = ismENGINE_CREATE_CLIENT_OPTION_NONE;
    }

    /* Create our clients and sessions...nondurable client as we are mimicing JMS */
    rc = test_createClientAndSessionWithProtocol(clientId,
                                                 protocolId,
                                                 secContext,
                                                 clientOpts,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    for(uint32_t i=0; i<msgCount; i++)
    {
        payload = NULL;

        if (serverUID == NULL)
        {
            rc = test_createMessage(payloadSize,
                                    persistence,
                                    reliability,
                                    flags,
                                    0,
                                    ismDESTINATION_TOPIC, topicString,
                                    &hMessage, &payload);
        }
        else
        {
            uint8_t messageType;
            uint32_t expiry = 0;

            // Allow us to publish null retained messages
            if ((payloadSize == 0) && ((flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN) != 0))
            {
                messageType = MTYPE_NullRetained;
                // NullRetained messages from the forwarder should always have a non-zero expiry
                if (protocolId == PROTOCOL_ID_FWD) expiry = ism_common_nowExpire() + 30;
            }
            else
            {
                messageType = MTYPE_JMS; // Normal default
            }

            rc = test_createOriginMessage(payloadSize,
                                          persistence,
                                          reliability,
                                          flags,
                                          expiry,
                                          ismDESTINATION_TOPIC, topicString,
                                          serverUID, ism_engine_retainedServerTime(),
                                          messageType,
                                          &hMessage, &payload);
        }
        TEST_ASSERT_EQUAL(rc, OK);

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                NULL,
                                                hMessage,
                                                &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc != ISMRC_AsyncCompletion)
        {
            if (expectRC == OK)
            {
                TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
            }
            else TEST_ASSERT_EQUAL(rc, expectRC);
            test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        }

        if (payload) free(payload);
    }

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

/* Check the contents of the export file specified are as we expect */
void test_checkExportFileContents(ieutThreadData_t *pThreadData, char *filePath, const char *filePassword,
                                  char *clientIdString, char *topicString,
                                  char expectedClientIdChar, char expectedUserIdChar, char expectedTopicChar,
                                  uint32_t *expectedTypeCount)
{
    int32_t rc;
    uint64_t dataId;
    size_t dataLen = 0;
    ieieDataType_t dataType;
    void *inData = NULL;
    uint32_t typeCount[ieieDATATYPE_LAST] = {0};
    uint32_t otherTypeCount = 0;

    ieieEncryptedFileHandle_t inFile =  ieie_OpenEncryptedFile(pThreadData, iemem_exportResources, filePath, filePassword);

    TEST_ASSERT_PTR_NOT_NULL(inFile);

    uint8_t *extraData;
    ieieResourceFileHeader_t RFH;
    uint64_t headerDataId = 0;
    ieieResourceFileFooter_t RFF;
    ieieClientStateInfo_t CSI;
    ieieSubscriptionInfo_t SI;
    const char *serverName = ism_common_getServerName();
    const char *serverUID = ism_common_getServerUID();
    do
    {
        rc = ieie_importData(pThreadData, inFile,  &dataType, &dataId, &dataLen, (void **)&inData);

        if (rc == OK)
        {
            TEST_ASSERT_GREATER_THAN(ieieDATATYPE_LAST, dataType);
            typeCount[dataType] += 1;
            char *clientId;
            char *userId = NULL;
            char *willTopicName = NULL;
            switch(dataType)
            {
                case ieieDATATYPE_EXPORTEDCLIENTSTATE:
                    TEST_ASSERT_GREATER_THAN_OR_EQUAL(dataLen, sizeof(CSI));
                    memcpy(&CSI, inData, sizeof(CSI));
                    extraData = inData+sizeof(CSI);
                    TEST_ASSERT_EQUAL(CSI.Version, ieieCLIENTSTATE_CURRENT_VERSION);
                    if (CSI.ProtocolId != PROTOCOL_ID_MQTT &&
                        CSI.ProtocolId != PROTOCOL_ID_JMS &&
                        CSI.ProtocolId != PROTOCOL_ID_SHARED)
                    {
                        TEST_ASSERT_EQUAL(CSI.ProtocolId, PROTOCOL_ID_MQTT);
                    }
                    clientId = (char *)extraData;
                    TEST_ASSERT_NOT_EQUAL(CSI.ClientIdLength, 0);
                    if (expectedClientIdChar != 0) TEST_ASSERT_EQUAL(clientId[0], expectedClientIdChar);
                    extraData += CSI.ClientIdLength;
                    if (CSI.UserIdLength != 0)
                    {
                        userId = (char *)extraData;
                        if (expectedUserIdChar != 0) TEST_ASSERT_EQUAL(userId[0], expectedUserIdChar);
                        extraData += CSI.UserIdLength;
                    }
                    if (CSI.WillTopicNameLength != 0)
                    {
                        willTopicName = (char *)extraData;
                        if (expectedTopicChar != 0) TEST_ASSERT_EQUAL(willTopicName[0], expectedTopicChar);
                        extraData += CSI.WillTopicNameLength;
                        TEST_ASSERT_NOT_EQUAL(CSI.WillMsgId, 0);
                    }
                    for(uint32_t i=0; i<CSI.UMSCount; i++)
                    {
                        uint32_t UMS = 0;
                        memcpy(&UMS, extraData, sizeof(uint32_t));
                        TEST_ASSERT_NOT_EQUAL(UMS, 0);
                        extraData += sizeof(uint32_t);
                    }
                    size_t readLen = extraData-(uint8_t*)inData;
                    TEST_ASSERT_EQUAL(readLen, dataLen);
                    break;
                case ieieDATATYPE_EXPORTEDMESSAGERECORD:
                    // TODO: Verify data format
                    break;
                case ieieDATATYPE_EXPORTEDSUBSCRIPTION:
                    TEST_ASSERT_GREATER_THAN(dataLen, sizeof(SI));
                    memcpy(&SI, inData, sizeof(SI));
                    extraData = inData+sizeof(SI);
                    TEST_ASSERT_EQUAL(SI.Version, ieieSUBSCRIPTION_CURRENT_VERSION);
                    TEST_ASSERT_NOT_EQUAL(SI.ClientIdLength, 0);
                    if (clientIdString != NULL)
                    {
                        TEST_ASSERT_GREATER_THAN_OR_EQUAL(SI.ClientIdLength, strlen(clientIdString)+1);
                        TEST_ASSERT_PTR_NOT_NULL(strstr((char *)extraData, clientIdString));
                    }
                    extraData += SI.ClientIdLength;
                    TEST_ASSERT_NOT_EQUAL(SI.TopicStringLength, 0);
                    if (expectedTopicChar != 0) TEST_ASSERT_EQUAL(*(char *)extraData, expectedTopicChar);
                    extraData += SI.TopicStringLength;
                    if (SI.SubNameLength != 0)
                    {
                        // extraData points at subscription name
                    }
                    extraData += SI.SubNameLength;
                    if (SI.SubPropertiesLength != 0)
                    {
                        // extraData points at subscription properties
                    }
                    extraData += SI.SubPropertiesLength;
                    TEST_ASSERT_NOT_EQUAL(SI.PolicyNameLength, 0);
                    // extraData points at the policy name
                    extraData += SI.PolicyNameLength;
                    TEST_ASSERT_EQUAL(SI.SharingClientCount, 0);
                    TEST_ASSERT_EQUAL(SI.SharingClientIdsLength, 0);
                    break;
                case ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB:
                    TEST_ASSERT_GREATER_THAN(dataLen, sizeof(SI));
                    memcpy(&SI, inData, sizeof(SI));
                    extraData = inData+sizeof(SI);
                    TEST_ASSERT_EQUAL(SI.Version, ieieSUBSCRIPTION_CURRENT_VERSION);
                    TEST_ASSERT_NOT_EQUAL(SI.ClientIdLength, 0);
                    TEST_ASSERT_EQUAL(strncmp((const char *)extraData, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_PREFIX, 8), 0);
                    extraData += SI.ClientIdLength;
                    TEST_ASSERT_NOT_EQUAL(SI.TopicStringLength, 0);
                    if (expectedTopicChar != 0) TEST_ASSERT_EQUAL(*(char *)extraData, expectedTopicChar);
                    extraData += SI.TopicStringLength;
                    if (SI.SubNameLength != 0)
                    {
                        // extraData points at subscription name
                    }
                    extraData += SI.SubNameLength;
                    if (SI.SubPropertiesLength != 0)
                    {
                        // extraData points at subscription properties
                    }
                    extraData += SI.SubPropertiesLength;
                    TEST_ASSERT_NOT_EQUAL(SI.PolicyNameLength, 0);
                    // extraData points at the policy name
                    extraData += SI.PolicyNameLength;
                    if (SI.SharingClientCount == 0)
                    {
                        TEST_ASSERT_EQUAL(SI.AnonymousSharers, iettANONYMOUS_SHARER_JMS_APPLICATION);

                        // NOTE: There may also be explicit sharing clients, but not
                        // in the current set of tests.
                    }
                    else
                    {
                        TEST_ASSERT_NOT_EQUAL(SI.SharingClientIdsLength, 0);
                        // extraData points at the sharing client ids
                        extraData += SI.SharingClientIdsLength;
                        // extraData points at the sharing client subOptions array
                        TEST_ASSERT_EQUAL(SI.SharingClientSubOptionsLength, SI.SharingClientCount * sizeof(uint32_t));
                        extraData += SI.SharingClientSubOptionsLength;
                    }
                    break;
                case ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER:
                    TEST_ASSERT_GREATER_THAN_OR_EQUAL(dataLen, sizeof(RFH));
                    memcpy(&RFH, inData, sizeof(RFH));
                    extraData = inData+sizeof(RFH);
                    TEST_ASSERT_EQUAL(RFH.Version, ieieRESOURCEFILE_CURRENT_VERSION);
                    if (serverName != NULL)
                    {
                        TEST_ASSERT_EQUAL(RFH.ServerNameLength, strlen(serverName)+1);
                        TEST_ASSERT_STRINGS_EQUAL((char *)extraData, serverName);
                    }
                    else
                    {
                        TEST_ASSERT_EQUAL(RFH.ServerNameLength, 0);
                    }
                    extraData += RFH.ServerNameLength;
                    if (serverUID != NULL)
                    {
                        TEST_ASSERT_EQUAL(RFH.ServerUIDLength, strlen(serverUID)+1);
                        TEST_ASSERT_STRINGS_EQUAL((char *)extraData, serverUID);
                    }
                    else
                    {
                        TEST_ASSERT_EQUAL(RFH.ServerUIDLength, 0);
                    }
                    extraData += RFH.ServerUIDLength;
                    if (clientIdString != NULL)
                    {
                        TEST_ASSERT_EQUAL(RFH.ClientIdLength, strlen(clientIdString)+1);
                        TEST_ASSERT_STRINGS_EQUAL((char *)extraData, clientIdString);
                    }
                    else
                    {
                        TEST_ASSERT_EQUAL(RFH.ClientIdLength, 0);
                    }
                    extraData += RFH.ClientIdLength;
                    if (topicString != NULL)
                    {
                        TEST_ASSERT_EQUAL(RFH.TopicLength, strlen(topicString)+1);
                        TEST_ASSERT_STRINGS_EQUAL((char *)extraData, topicString);
                    }
                    else
                    {
                        TEST_ASSERT_EQUAL(RFH.TopicLength, 0);
                    }
                    extraData += RFH.TopicLength;
                    TEST_ASSERT_EQUAL(RFH.StartTime, dataId);
                    headerDataId = dataId;
                    break;
                case ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER:
                    TEST_ASSERT_EQUAL(dataLen, sizeof(RFF));
                    memcpy(&RFF, inData, sizeof(RFF));
                    extraData = inData+sizeof(RFF);
                    TEST_ASSERT_EQUAL(RFF.Version, ieieRESOURCEFILE_CURRENT_VERSION);
                    TEST_ASSERT_EQUAL(dataId, headerDataId);
                    break;
                default:
                    //printf("%d\n", (int)dataType);
                    otherTypeCount++;
                    break;
            }
        }
    }
    while(rc == OK);

    TEST_ASSERT_EQUAL(rc, ISMRC_EndOfFile);
    TEST_ASSERT_EQUAL(1, typeCount[ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER]);
    TEST_ASSERT_EQUAL(1, typeCount[ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER]);
    for(int32_t i=0; i<ieieDATATYPE_LAST; i++)
    {
        TEST_ASSERT_EQUAL(expectedTypeCount[i], typeCount[i]);
    }

    ieie_freeReadExportedData(pThreadData, iemem_exportResources, inData);

    rc = ieie_finishReadingEncryptedFile(pThreadData, inFile);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Check the contents & properties of an import/export status file
//****************************************************************************
void test_checkStatusFile(char *impexpFilePath, uint64_t requestID,
                          ismEngine_ImportExportResourceStatusType_t expectedStatus,
                          uint32_t expectedDiagnostics,
                          char *expectedRegExClientID, char *expectedRegExTopic, bool expectServerInfo)
{
    char statusPath[strlen(impexpFilePath)+10];

    bool forImport = (strstr(impexpFilePath, "/import/") != NULL); // Determine if this is for import

    strcpy(statusPath, impexpFilePath);
    sprintf(strrchr(statusPath, '/')+1, ieieSTATUSFILE_PREFIX "%lu" ieieSTATUSFILE_TYPE, requestID);

    struct stat statBuf = {0};

    // Check the file type & permissions are as expected
    int32_t rc = stat(statusPath, &statBuf);
    TEST_ASSERT_EQUAL(rc, 0);
    TEST_ASSERT_EQUAL(statBuf.st_mode, S_IFREG | ieieSTATUSFILE_PERMISSIONS);
    TEST_ASSERT_EQUAL(statBuf.st_uid, getuid());
    TEST_ASSERT_EQUAL(statBuf.st_gid, getgid());

    FILE *fp = fopen(statusPath, "rb");
    TEST_ASSERT_PTR_NOT_NULL(fp);

    fseek(fp, 0, SEEK_END);
    int64_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *statusContents = malloc(size+1);
    TEST_ASSERT_PTR_NOT_NULL(statusContents);

    if (fread(statusContents, size, 1, fp) != 1)
    {
        TEST_ASSERT(false, ("Failed to fread() %ld bytes from %s, errno %d\n", size, statusPath, errno));
    }

    fclose(fp);

    statusContents[size] = '\0';

    ism_json_parse_t parseObj = {0};
    ism_json_entry_t ents[10];

    parseObj.ent = ents;
    parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseObj.source = statusContents;
    parseObj.src_len = (int)size;

    rc = ism_json_parse(&parseObj);
    TEST_ASSERT_EQUAL(rc, OK);

    int32_t intValue;
    const char *stringValue;

    intValue = ism_json_getInt(&parseObj, ismENGINE_IMPORTEXPORT_STATUS_FIELD_REQUESTID, 99);
    TEST_ASSERT_EQUAL(intValue, (int32_t)requestID);

    stringValue = ism_json_getString(&parseObj, ismENGINE_IMPORTEXPORT_STATUS_FIELD_FILEPATH);
    if (expectedStatus != ismENGINE_IMPORTEXPORT_STATUS_FAILED)
    {
        TEST_ASSERT_PTR_NOT_NULL(stringValue);
        TEST_ASSERT_STRINGS_EQUAL(stringValue, impexpFilePath);
    }
    else if (stringValue != NULL)
    {
        TEST_ASSERT_STRINGS_EQUAL(stringValue, impexpFilePath);
    }

    if (expectServerInfo)
    {
        stringValue = ism_json_getString(&parseObj, ismENGINE_IMPORTEXPORT_STATUS_FIELD_EXPORTSERVERNAME);
        TEST_ASSERT_PTR_NOT_NULL(stringValue);
        TEST_ASSERT_STRINGS_EQUAL(stringValue, ism_common_getServerName());

        stringValue = ism_json_getString(&parseObj, ismENGINE_IMPORTEXPORT_STATUS_FIELD_EXPORTSERVERUID);
        TEST_ASSERT_PTR_NOT_NULL(stringValue);
        TEST_ASSERT_STRINGS_EQUAL(stringValue, ism_common_getServerUID());

        if (forImport)
        {
            stringValue = ism_json_getString(&parseObj, ismENGINE_IMPORTEXPORT_STATUS_FIELD_IMPORTSERVERNAME);
            TEST_ASSERT_PTR_NOT_NULL(stringValue);
            TEST_ASSERT_STRINGS_EQUAL(stringValue, ism_common_getServerName());

            stringValue = ism_json_getString(&parseObj, ismENGINE_IMPORTEXPORT_STATUS_FIELD_IMPORTSERVERUID);
            TEST_ASSERT_PTR_NOT_NULL(stringValue);
            TEST_ASSERT_STRINGS_EQUAL(stringValue, ism_common_getServerUID());
        }
    }

    intValue = ism_json_getInt(&parseObj, ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUS, 99);
    TEST_ASSERT_EQUAL(intValue, expectedStatus);

    for(int32_t x=1; x<parseObj.ent_count; x++)
    {
        ism_json_entry_t *entry = &parseObj.ent[x];

        if (entry->name != NULL)
        {
            if (strcmp(entry->name, ismENGINE_IMPORTEXPORT_STATUS_FIELD_DIAGNOSTICS) == 0)
            {
                TEST_ASSERT_EQUAL(entry->objtype, JSON_Array);
                // Each array entry counts +1 and each (of 4) entries in the object counts +1
                TEST_ASSERT_EQUAL(entry->count, expectedDiagnostics*(1+ieieJSON_VALUES_PER_DIAGNOSTIC));
            }
            else if (strcmp(entry->name, ismENGINE_IMPORTEXPORT_STATUS_FIELD_CLIENTID) == 0)
            {
                TEST_ASSERT_EQUAL(entry->objtype, JSON_String);
                if (expectedRegExClientID != NULL)
                {
                    TEST_ASSERT_STRINGS_EQUAL(entry->value, expectedRegExClientID);
                }
            }
            else if (strcmp(entry->name, ismENGINE_IMPORTEXPORT_STATUS_FIELD_TOPIC) == 0)
            {
                TEST_ASSERT_EQUAL(entry->objtype, JSON_String);
                if (expectedRegExTopic != NULL)
                {
                    TEST_ASSERT_STRINGS_EQUAL(entry->value, expectedRegExTopic);
                }
            }
        }
    }

    if (parseObj.free_ent) ism_common_free(ism_memory_utils_parser,parseObj.ent);
    free(statusContents);
}

//****************************************************************************
/// @brief Test Operation of large client export
//****************************************************************************
void test_capability_InvalidRequest(void)
{
    int32_t rc;

    char fileName[255] = "../naughtyFilename";
    char *exportFilePath = NULL;
    char *importFilePath = NULL;
    char *suffix = ".enc";
    const char *filePassword = __func__;
    uint64_t requestID;

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    ieieImportExportGlobal_t *importExportGlobal = ismEngine_serverGlobal.importExportGlobal;
    TEST_ASSERT_PTR_NOT_NULL(importExportGlobal);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, false, &exportFilePath);
    TEST_ASSERT_EQUAL(rc, ISMRC_BadPropertyValue);
    TEST_ASSERT_PTR_NULL(exportFilePath);

    strcpy(fileName, __func__);
    strcat(fileName, suffix);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, false, &exportFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(exportFilePath);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, true, &importFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(importFilePath);

    printf("Starting %s...\n", __func__);

    //Initialise OpenSSL - in the product - the equivalent is ism_ssl_init();
    //TODO: Do we need to run that in the engine thread that does this or is it done?
    sslInit();

    // Make a *DIRECTORY* as the exportFilePath to cause the attempt to unlink / open
    // it to fail...
    rc = mkdir(exportFilePath, S_IRWXU);
    TEST_ASSERT_EQUAL(rc, 0);

    // File cannot be unlinked
    rc = ism_engine_exportResources(".*",
                                    NULL,
                                    fileName,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE,
                                    &requestID,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_FileCorrupt);
    TEST_ASSERT_EQUAL(requestID, 0);

    // Remove the directory so the following tests can proceed properly
    rc = rmdir(exportFilePath);
    TEST_ASSERT_EQUAL(rc, 0);

    // Fail to create export file
    test_EVP_get_cipherbynameFAIL = true;
    rc = ism_engine_exportResources(".*",
                                    NULL,
                                    fileName,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_FileCorrupt);
    TEST_ASSERT_EQUAL(requestID, 0);
    test_EVP_get_cipherbynameFAIL = false;

    unlink(exportFilePath); // Don't want to test OVERWRITE

    // Invalid clientId Regex
    rc = ism_engine_exportResources("**",
                                    NULL,
                                    fileName,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_BadPropertyValue);
    TEST_ASSERT_EQUAL(requestID, 0);

    // Invalid Topic Regex
    rc = ism_engine_exportResources(NULL,
                                    "**",
                                    fileName,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_BadPropertyValue);
    TEST_ASSERT_EQUAL(requestID, 0);

    // Invalid filename
    rc = ism_engine_exportResources(".*",
                                    NULL,
                                    "/",
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_BadPropertyValue);
    TEST_ASSERT_EQUAL(requestID, 0);

    // Invalid filename (empty)
    rc = ism_engine_exportResources(".*",
                                    NULL,
                                    "",
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_BadPropertyValue);
    TEST_ASSERT_EQUAL(requestID, 0);

    // Invalid filename (contains .status)
    rc = ism_engine_exportResources(".*",
                                    NULL,
                                    "my"ieieSTATUSFILE_TYPE".filename",
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_BadPropertyValue);
    TEST_ASSERT_EQUAL(requestID, 0);

    // Invalid filename (ends in .status)
    rc = ism_engine_importResources("my"ieieSTATUSFILE_TYPE,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_BadPropertyValue);
    TEST_ASSERT_EQUAL(requestID, 0);

    // Invalid filename (empty)
    rc = ism_engine_importResources("",
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_BadPropertyValue);
    TEST_ASSERT_EQUAL(requestID, 0);

    // Force us to hit the maxActive limit & try to call after stopCalled
    for(int32_t i=0; i<2; i++)
    {
        if (i == 0)
        {
            TEST_ASSERT_EQUAL(importExportGlobal->activeRequests, 0);
            importExportGlobal->activeRequests = importExportGlobal->maxActiveRequests;
        }
        else
        {
            importExportGlobal->activeRequests = 0;
            importExportGlobal->stopCalled = true;
        }

        rc = sync_ism_engine_exportResources(".*",
                                             NULL,
                                             "failMaxActive",
                                             filePassword,
                                             ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE,
                                             &requestID);
        TEST_ASSERT_EQUAL(rc, i == 0 ? ISMRC_TooManyActiveRequests : ISMRC_InvalidOperation);
        TEST_ASSERT_EQUAL(requestID, 0);

        rc = sync_ism_engine_importResources("failMaxActive",
                                             filePassword,
                                             ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                             &requestID);
        TEST_ASSERT_EQUAL(rc, i == 0 ? ISMRC_TooManyActiveRequests : ISMRC_InvalidOperation);
        TEST_ASSERT_EQUAL(requestID, 0);
    }

    importExportGlobal->stopCalled = false;

    // File doesn't exist
    rc = ism_engine_importResources("NOFILE", filePassword, ismENGINE_IMPORT_RESOURCES_OPTION_NONE, &requestID, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_FileCorrupt);
    TEST_ASSERT_NOT_EQUAL(requestID, 0);

    // Do an export that should result in no file (because we simulated a deletion)
    test_ieie_updateExportStatus_deleteBeforeUpdate = true;
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(".*",
                                    NULL,
                                    "missing",
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0);

    test_waitForRemainingActions(pActionsRemaining);
    test_ieie_updateExportStatus_deleteBeforeUpdate = false;

    // Make the ism_transport_disableClientSet fail to test that code path.
    test_ism_transport_disableClientSetRC = ISMRC_ClientSetNotValid;
    rc = sync_ism_engine_exportResources(".*",
                                         NULL,
                                         "failDisable",
                                         filePassword,
                                         ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                         &requestID);
    TEST_ASSERT_EQUAL(rc, ISMRC_ClientSetNotValid);
    test_ism_transport_disableClientSetRC = OK;

    // Make the ism_transport_enableClientSet fail to test that code path.
    test_ism_transport_enableClientSetRC = ISMRC_Error;
    rc = sync_ism_engine_exportResources(".*",
                                         NULL,
                                         "failDisable",
                                         filePassword,
                                         ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE,
                                         &requestID);
    TEST_ASSERT_EQUAL(rc, ISMRC_Error);
    TEST_ASSERT_NOT_EQUAL(requestID, 0);
    test_ism_transport_enableClientSetRC = OK;

    // Create a client which we can then export
    ismEngine_ClientState_t *hClient;
    ismEngine_Session_t *hSession;

    rc = test_createClientAndSession("ExportClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Try hijacking the permissions on the status file
    char statusPath[strlen(exportFilePath)+10];

    for(int32_t i=0; i<2; i++)
    {
        strcpy(statusPath, exportFilePath);
        if (i == 0) // Status file first
        {
            sprintf(strrchr(statusPath, '/')+1,
                    ieieSTATUSFILE_PREFIX "%lu" ieieSTATUSFILE_TYPE,
                    requestID+1);
        }
        else // tmp file
        {
            sprintf(strrchr(statusPath, '/')+1,
                    ieieSTATUSFILE_PREFIX "%lu" ieieSTATUSFILE_TYPE ieieSTATUSFILE_TMP_TYPE ".1",
                    requestID+1);
        }

        int fd = open(statusPath, O_CREAT|O_WRONLY|O_EXCL, S_IRWXU|S_IRWXG|S_IRWXO);
        TEST_ASSERT_NOT_EQUAL(fd, -1);
        rc = close(fd);
        TEST_ASSERT_EQUAL(rc, 0);

        rc = sync_ism_engine_exportResources("ExportClient",
                                             NULL,
                                             fileName,
                                             filePassword,
                                             ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                             &requestID);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_NOT_EQUAL(requestID, 0);
        test_checkStatusFile(exportFilePath, requestID, ismENGINE_IMPORTEXPORT_STATUS_COMPLETE, 0, "ExportClient", NULL, false);
        unlink(exportFilePath);
    }

    // Write some invalid files
    ieieEncryptedFileHandle_t fileHandle;

    // Empty file
    fileHandle = ieie_createEncryptedFile(pThreadData, iemem_exportResources, exportFilePath, filePassword);
    rc = ieie_finishWritingEncryptedFile(pThreadData, fileHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);
    rc = ism_engine_importResources(fileName, filePassword, ismENGINE_IMPORT_RESOURCES_OPTION_NONE, &requestID, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_FileCorrupt);
    TEST_ASSERT_NOT_EQUAL(requestID, 0);
    test_checkStatusFile(importFilePath, requestID, ismENGINE_IMPORTEXPORT_STATUS_FAILED, 0, NULL, NULL, false);

    // Bad Header version
    const char *serverName = ism_common_getServerName();
    const char *serverUID = ism_common_getServerUID();
    const char *clientID = "INVALID_TESTS";
    size_t serverNameLength = serverName ? strlen(serverName)+1 : 0;
    size_t serverUIDLength = serverUID ? strlen(serverUID)+1 : 0;
    size_t clientIDLength = strlen(clientID)+1;

    size_t headerSize = sizeof(ieieResourceFileHeader_t) + serverNameLength + serverUIDLength + clientIDLength;

    ieieResourceFileHeader_t *header = calloc(headerSize, 1);
    TEST_ASSERT_PTR_NOT_NULL(header);

    char *extraData = (char *)(header+1);
    if (serverNameLength != 0)
    {
        header->ServerNameLength = (uint32_t)serverNameLength;
        memcpy(extraData, serverName, serverNameLength);
        extraData += serverNameLength;
    }
    if (serverUIDLength != 0)
    {
        header->ServerUIDLength = (uint32_t)serverUIDLength;
        memcpy(extraData, serverUID, serverUIDLength);
        extraData += serverUIDLength;
    }
    header->ClientIdLength = (uint32_t)clientIDLength;
    memcpy(extraData, clientID, clientIDLength);
    extraData += clientIDLength;

    // Higher version number
    header->Version = ieieRESOURCEFILE_CURRENT_VERSION + 1;
    fileHandle = ieie_createEncryptedFile(pThreadData, iemem_exportResources, exportFilePath, filePassword);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER, 0, headerSize, header);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_finishWritingEncryptedFile(pThreadData, fileHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);
    rc = ism_engine_importResources(fileName, filePassword, ismENGINE_IMPORT_RESOURCES_OPTION_NONE, &requestID, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_FileCorrupt);
    TEST_ASSERT_NOT_EQUAL(requestID, 0);
    test_checkStatusFile(importFilePath, requestID, ismENGINE_IMPORTEXPORT_STATUS_FAILED, 0, NULL, NULL, false);

    // Two headers
    header->Version = ieieRESOURCEFILE_CURRENT_VERSION;
    fileHandle = ieie_createEncryptedFile(pThreadData, iemem_exportResources, exportFilePath, filePassword);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER, 0, headerSize, header);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER, 0, headerSize, header);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_finishWritingEncryptedFile(pThreadData, fileHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);
    rc = ism_engine_importResources(fileName, filePassword, ismENGINE_IMPORT_RESOURCES_OPTION_NONE, &requestID, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_FileCorrupt);
    TEST_ASSERT_NOT_EQUAL(requestID, 0);
    test_checkStatusFile(importFilePath, requestID, ismENGINE_IMPORTEXPORT_STATUS_FAILED, 0, NULL, NULL, true);

    // Invalid data type
    fileHandle = ieie_createEncryptedFile(pThreadData, iemem_exportResources, exportFilePath, filePassword);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER, 0, headerSize, header);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_LAST, 0, headerSize, header);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_finishWritingEncryptedFile(pThreadData, fileHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);
    rc = ism_engine_importResources(fileName, filePassword, ismENGINE_IMPORT_RESOURCES_OPTION_NONE, &requestID, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_FileCorrupt);
    TEST_ASSERT_NOT_EQUAL(requestID, 0);
    test_checkStatusFile(importFilePath, requestID, ismENGINE_IMPORTEXPORT_STATUS_FAILED, 0, NULL, NULL, true);

    // No Footer
    fileHandle = ieie_createEncryptedFile(pThreadData, iemem_exportResources, exportFilePath, filePassword);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER, 0, headerSize, header);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_finishWritingEncryptedFile(pThreadData, fileHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);
    rc = ism_engine_importResources(fileName, filePassword, ismENGINE_IMPORT_RESOURCES_OPTION_NONE, &requestID, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_FileCorrupt)
    TEST_ASSERT_NOT_EQUAL(requestID, 0);
    test_checkStatusFile(importFilePath, requestID, ismENGINE_IMPORTEXPORT_STATUS_FAILED, 0, NULL, NULL, true);

    // Mismatching footer (mismatch of version number)
    ieieResourceFileFooter_t footer = {{0},0};
    footer.Version = ieieRESOURCEFILE_CURRENT_VERSION + 1;
    fileHandle = ieie_createEncryptedFile(pThreadData, iemem_exportResources, exportFilePath, filePassword);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER, 0, headerSize, header);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER, 0, sizeof(footer), &footer);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_finishWritingEncryptedFile(pThreadData, fileHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);
    rc = ism_engine_importResources(fileName, filePassword, ismENGINE_IMPORT_RESOURCES_OPTION_NONE, &requestID, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_FileCorrupt)
    TEST_ASSERT_NOT_EQUAL(requestID, 0);
    test_checkStatusFile(importFilePath, requestID, ismENGINE_IMPORTEXPORT_STATUS_FAILED, 0, NULL, NULL, true);

    // Mismatching footer (mismatch of dataId)
    footer.Version = ieieRESOURCEFILE_CURRENT_VERSION;
    fileHandle = ieie_createEncryptedFile(pThreadData, iemem_exportResources, exportFilePath, filePassword);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER, 0, headerSize, header);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER, 99, sizeof(footer), &footer);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_finishWritingEncryptedFile(pThreadData, fileHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);
    rc = ism_engine_importResources(fileName, filePassword, ismENGINE_IMPORT_RESOURCES_OPTION_NONE, &requestID, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_FileCorrupt)
    TEST_ASSERT_NOT_EQUAL(requestID, 0);
    test_checkStatusFile(importFilePath, requestID, ismENGINE_IMPORTEXPORT_STATUS_FAILED, 0, NULL, NULL, true);

    // Mismatching footer (mismatch of requestID)
    footer.RequestID = 99;
    fileHandle = ieie_createEncryptedFile(pThreadData, iemem_exportResources, exportFilePath, filePassword);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER, 0, headerSize, header);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER, 0, sizeof(footer), &footer);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_finishWritingEncryptedFile(pThreadData, fileHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);
    rc = ism_engine_importResources(fileName, filePassword, ismENGINE_IMPORT_RESOURCES_OPTION_NONE, &requestID, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_FileCorrupt)
    TEST_ASSERT_NOT_EQUAL(requestID, 0);
    test_checkStatusFile(importFilePath, requestID, ismENGINE_IMPORTEXPORT_STATUS_FAILED, 0, NULL, NULL, true);
    footer.RequestID = header->RequestID;

    // Trailing record
    for(int32_t i=0; i<2; i++)
    {
        fileHandle = ieie_createEncryptedFile(pThreadData, iemem_exportResources, exportFilePath, filePassword);
        rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER, 0, headerSize, header);
        TEST_ASSERT_EQUAL(rc, OK);
        rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER, 0, sizeof(footer), &footer);
        TEST_ASSERT_EQUAL(rc, OK);
        switch(i)
        {
            case 0:
                rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDCLIENTSTATE, (uint64_t)header, headerSize, header);
                break;
            case 1:
                rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRETAINEDMSG, (uint64_t)header, headerSize, header);
                break;
        }
        TEST_ASSERT_EQUAL(rc, OK);
        rc = ieie_finishWritingEncryptedFile(pThreadData, fileHandle);
        TEST_ASSERT_EQUAL(rc, OK);
        rc = test_copyFile(exportFilePath, importFilePath);
        TEST_ASSERT_EQUAL(rc, 0);
        rc = ism_engine_importResources(fileName, filePassword, ismENGINE_IMPORT_RESOURCES_OPTION_NONE, &requestID, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_FileCorrupt)
        TEST_ASSERT_NOT_EQUAL(requestID, 0);
        test_checkStatusFile(importFilePath, requestID, ismENGINE_IMPORTEXPORT_STATUS_FAILED, 0, NULL, NULL, true);
    }

    // Retained message referencing a missing message
    ieieRetainedMsgInfo_t retainedInfo = {ieieRETAINEDMSG_CURRENT_VERSION};
    fileHandle = ieie_createEncryptedFile(pThreadData, iemem_exportResources, exportFilePath, filePassword);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER, 0, headerSize, header);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRETAINEDMSG, 0xdeaddeaddeadbeef, sizeof(retainedInfo), &retainedInfo);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_exportData(pThreadData, fileHandle, ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER, 0, sizeof(footer), &footer);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieie_finishWritingEncryptedFile(pThreadData, fileHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);
    rc = sync_ism_engine_importResources(fileName, filePassword, ismENGINE_IMPORT_RESOURCES_OPTION_NONE, &requestID);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound)
    TEST_ASSERT_NOT_EQUAL(requestID, 0);
    test_checkStatusFile(importFilePath, requestID, ismENGINE_IMPORTEXPORT_STATUS_FAILED, 1, NULL, NULL, true);

    // TODO: Write out some invalid files and try to import them.

    sslTerm();

    iemem_free(pThreadData, iemem_exportResources, exportFilePath);
    iemem_free(pThreadData, iemem_exportResources, importFilePath);
    free(header);
}

#define MANYCLIENTS_NUMBER_OF_CLIENTS 100000
//****************************************************************************
/// @brief Test Operation of large client export
//****************************************************************************
void test_capability_ManyClients(void)
{
    int32_t rc;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    uint32_t expectedTypeCounts[ieieDATATYPE_LAST] = {0};

    expectedTypeCounts[ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER] = 1;

    uint32_t setCount[20] = {0};
    uint32_t willMsgCount[20] = {0};

    uint32_t totalWillMsgCount = 0;

    ismEngine_ClientStateHandle_t *hClient;
    int *clientSet;

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    rc = test_createSecurityContext(&mockListener, &mockTransport, &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport->clientID = "MCTEST_SOMETHING";
    mockTransport->userid = "TestUser";

    char fileName1[255];
    char fileName2[255];
    char *exportFilePath1 = NULL;
    char *exportFilePath2 = NULL;
    char *importFilePath1 = NULL;
    char *importFilePath2 = NULL;
    char *suffix = ".enc";
    const char *filePassword = __func__;
    uint64_t requestID;

    strcpy(fileName1, __func__);
    strcat(fileName1, suffix);

    strcpy(fileName2, "IMPORTABLE");
    strcat(fileName2, suffix);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName1, false, &exportFilePath1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(exportFilePath1);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName2, false, &exportFilePath2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(exportFilePath2);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName1, true, &importFilePath1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(importFilePath1);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName2, true, &importFilePath2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(importFilePath2);

    printf("Starting %s...\n", __func__);

    // Add a messaging policy that allows publish / subscribe on 'TEST.TOPICSECURITY.AUTHORIZED*'
    // For clients whose ID starts with 'SecureClient'.
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.MCTEST_POLICY\","
                                "\"Name\":\"MCTEST_POLICY\","
                                "\"ClientID\":\"MCTEST*\","
                                "\"Topic\":\"MCTEST*\","
                                "\"ActionList\":\"publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContext, "MCTEST_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);

    //Initialise OpenSSL - in the product - the equivalent is ism_ssl_init();
    //TODO: Do we need to run that in the engine thread that does this or is it done?
    sslInit();

    hClient = calloc(MANYCLIENTS_NUMBER_OF_CLIENTS, sizeof(ismEngine_ClientStateHandle_t));
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    clientSet = calloc(MANYCLIENTS_NUMBER_OF_CLIENTS, sizeof(int));
    TEST_ASSERT_PTR_NOT_NULL(clientSet);

    int32_t i;
    for(i=0; i<MANYCLIENTS_NUMBER_OF_CLIENTS; i++)
    {
        char clientId[20];
        clientSet[i] = rand()%(sizeof(setCount)/sizeof(setCount[0]));

        sprintf(clientId, "MCTEST_%02d_%d", clientSet[i], i);

        test_incrementActionsRemaining(pActionsRemaining, 1);
        void *secContext = rand()%20>10?mockContext:NULL;
        rc = ism_engine_createClientState(clientId,
                                          PROTOCOL_ID_MQTT,
                                          ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                          ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                          NULL, NULL,
                                          secContext,
                                          &hClient[i],
                                          &pActionsRemaining, sizeof(pActionsRemaining),
                                          test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        setCount[clientSet[i]] += 1;

        // Set a will message for 3/4 of the clients
        if (secContext != NULL && rand()%4 >= 3)
        {
            ismEngine_MessageHandle_t hMessage;
            void *pPayload=NULL;
            rc = test_createMessage(20,
                                    ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                    ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                    ismMESSAGE_FLAGS_NONE,
                                    0,
                                    ismDESTINATION_TOPIC, clientId,
                                    &hMessage,&pPayload);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hMessage);

            test_waitForRemainingActions(pActionsRemaining);

            test_incrementActionsRemaining(pActionsRemaining, 1);
            rc = ism_engine_setWillMessage(hClient[i],
                                           ismDESTINATION_TOPIC, clientId,
                                           hMessage,
                                           rand()%4 >= 2 ? 60 : 0,
                                           iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                           &pActionsRemaining, sizeof(pActionsRemaining),
                                           test_decrementActionsRemaining);
            if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
            totalWillMsgCount += 1;
            willMsgCount[clientSet[i]] += 1;
            free(pPayload);
        }

        // Add some unreleased delivery Ids to some of the clients
        int32_t deliveryIds = rand()%200;
        if (deliveryIds > 180)
        {
            deliveryIds -= 180;
            ismEngine_SessionHandle_t hSession;

            test_waitForRemainingActions(pActionsRemaining);

            test_incrementActionsRemaining(pActionsRemaining, 1);
            rc = ism_engine_createSession(hClient[i],
                                          ismENGINE_CREATE_SESSION_OPTION_NONE,
                                          &hSession,
                                          &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
            if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

            test_waitForRemainingActions(pActionsRemaining);

            // Add some unreleased delivery IDs
            while(deliveryIds >= 0)
            {
                ismEngine_UnreleasedHandle_t unreleasedDeliveryIdHandle;
                test_incrementActionsRemaining(pActionsRemaining, 1);
                rc = ism_engine_addUnreleasedDeliveryId(hSession,
                                                        NULL,
                                                        (rand()%65535)+1,
                                                        &unreleasedDeliveryIdHandle,
                                                        &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
                if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
                else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
                deliveryIds--;
            }
        }
    }

    test_waitForRemainingActions(pActionsRemaining);

    ism_time_t startTime, endTime, diffTime;
    double diffTimeSecs;

    // Export them all
    char *clientIdString = "^MCTEST.*_.*";
    char *topicString = "NORETAINED";

    test_incrementActionsRemaining(pActionsRemaining, 1);
    startTime = ism_common_currentTimeNanos();
    rc = ism_engine_exportResources(clientIdString,
                                    topicString,
                                    fileName1,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously

    test_waitForRemainingActions(pActionsRemaining);
    endTime = ism_common_currentTimeNanos();

    test_checkStatusFile(exportFilePath1, requestID, ismENGINE_IMPORTEXPORT_STATUS_COMPLETE, 0,
                         clientIdString, topicString, true);
    rc = test_copyFile(exportFilePath1, importFilePath1);
    TEST_ASSERT_EQUAL(rc, 0);

    diffTime = endTime-startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;

    expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE] = MANYCLIENTS_NUMBER_OF_CLIENTS;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] = totalWillMsgCount;

    // Check the contents of the file are as expected
    test_checkExportFileContents(pThreadData, exportFilePath1, filePassword, clientIdString, topicString,
                                 'M', 'T', 'M', expectedTypeCounts);

    printf("  ...time to export %d clients matching \"%s\" is %.2f secs. (%ldns)\n",
            expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE], clientIdString, diffTimeSecs, diffTime);

    // Check that we cannot import while the clients are still connected
    rc = ism_engine_importResources(fileName1,
                                    filePassword,
                                    ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ClientIDInUse);
    test_checkStatusFile(importFilePath1, requestID, ismENGINE_IMPORTEXPORT_STATUS_FAILED, 2,
                         clientIdString, topicString, true);

    // Export to another file for import when all the clients have been disconnected
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(clientIdString,
                                    topicString,
                                    fileName2,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_copyFile(exportFilePath2, importFilePath2);
    TEST_ASSERT_EQUAL(rc, 0);

    // Test not specifying OVERWRITE
    rc = ism_engine_exportResources(clientIdString,
                                    topicString,
                                    fileName1,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_FileAlreadyExists);

    // Test not specifying a ClientID
    clientIdString = NULL;
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(clientIdString,
                                    topicString,
                                    fileName1,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously

    test_waitForRemainingActions(pActionsRemaining);

    expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE] = 0;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] = 0;

    // Export the set which are 0*
    for(i=0; i<10; i++)
    {
        expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE] += setCount[i];
        expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] += willMsgCount[i];
    }
    clientIdString = ".*EST_0.*";
    topicString = NULL;
    test_incrementActionsRemaining(pActionsRemaining, 1);
    startTime = ism_common_currentTimeNanos();
    rc = ism_engine_exportResources(clientIdString,
                                    topicString,
                                    fileName1,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously

    test_waitForRemainingActions(pActionsRemaining);

    endTime = ism_common_currentTimeNanos();
    rc = test_copyFile(exportFilePath1, importFilePath1);
    TEST_ASSERT_EQUAL(rc, 0);

    diffTime = endTime-startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;

    // Check the contents of the file are as expected
    test_checkExportFileContents(pThreadData, exportFilePath1, filePassword, clientIdString, topicString,
                                 'M', 'T', 'M', expectedTypeCounts);

    printf("  ...time to export %d clients matching \"%s\" is %.2f secs. (%ldns)\n",
            expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE], clientIdString, diffTimeSecs, diffTime);

    // Just change the requestID so we have to 'scan' for the next one
    uint64_t prevRequestID = requestID;
    ismEngine_serverGlobal.importExportGlobal->nextRequestID -= 1;

    // Match NONE
    clientIdString = ".*NO_MATCH_EXPECTED.*";
    topicString = NULL;
    test_incrementActionsRemaining(pActionsRemaining, 1);
    startTime = ism_common_currentTimeNanos();
    rc = ism_engine_exportResources(clientIdString,
                                    NULL,
                                    fileName1,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously
    TEST_ASSERT_NOT_EQUAL(requestID, prevRequestID);

    test_waitForRemainingActions(pActionsRemaining);

    endTime = ism_common_currentTimeNanos();

    diffTime = endTime-startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;

    expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE] = 0;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] = 0;

    printf("  ...time to export %d clients matching \"%s\" is %.2f secs. (%ldns)\n",
            expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE], clientIdString, diffTimeSecs, diffTime);

    // Test not specifying a clientId or topic
    rc = ism_engine_exportResources(NULL,
                                    NULL,
                                    fileName1,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_BadPropertyValue);

    test_incrementActionsRemaining(pActionsRemaining, MANYCLIENTS_NUMBER_OF_CLIENTS);
    for(i=0; i<MANYCLIENTS_NUMBER_OF_CLIENTS; i++)
    {
        rc = ism_engine_destroyClientState(hClient[i],
                                           ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                           &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    }

    test_waitForRemainingActions(pActionsRemaining);

    // Check that we can import now that the clients are disconnected
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_importResources(fileName2,
                                    filePassword,
                                    ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously

    test_waitForRemainingActions(pActionsRemaining);

    // Import a 2nd time testing that the removal of existing clients works correctly
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_importResources(fileName2,
                                    filePassword,
                                    ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously

    test_waitForRemainingActions(pActionsRemaining);

    for(i=0; i<MANYCLIENTS_NUMBER_OF_CLIENTS; i++)
    {
        char clientId[20];

        sprintf(clientId, "MCTEST_%02d_%d", clientSet[i], i);

        rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_MQTTCLIENT,
                                       clientId,
                                       NULL,
                                       ISM_CONFIG_CHANGE_DELETE);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = test_destroySecurityContext(mockListener, mockTransport, mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    free(hClient);
    free(clientSet);

    sslTerm();

    iemem_free(pThreadData, iemem_exportResources, exportFilePath1);
    iemem_free(pThreadData, iemem_exportResources, importFilePath1);
    iemem_free(pThreadData, iemem_exportResources, exportFilePath2);
    iemem_free(pThreadData, iemem_exportResources, importFilePath2);
}

CU_TestInfo ISM_ExportResources_CUnit_test_capability_Clients[] =
{
    {"InvalidRequest", test_capability_InvalidRequest },
    {"ManyClients", test_capability_ManyClients },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test Basic operation of including a (non-durable) subscription
//****************************************************************************
void test_capability_BasicSubscription(void)
{
    int32_t rc;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    uint32_t expectedTypeCounts[ieieDATATYPE_LAST] = {0};

    expectedTypeCounts[ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER] = 1;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    rc = test_createSecurityContext(&mockListener, &mockTransport, &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport->clientID = "TEST_BASICSUB";
    mockTransport->userid = "TestUser";

    char fileName[255];
    char *exportFilePath = NULL;
    char *importFilePath = NULL;
    char *suffix = ".enc";
    const char *filePassword = __func__;
    uint64_t requestID;

    strcpy(fileName, __func__);
    strcat(fileName, suffix);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, false, &exportFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(exportFilePath);
    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, true, &importFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(importFilePath);

    printf("Starting %s...\n", __func__);

    // Add a messaging policy that allows publish / subscribe on '*BASICSUB*'
    // For all clients whose ID starts with 'TEST_BASICSUB*'.
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.BASICSUB_POLICY\","
                                "\"Name\":\"BASICSUB_POLICY\","
                                "\"ClientID\":\"TEST_BASICSUB\","
                                "\"Topic\":\"*BASICSUB*\","
                                "\"ActionList\":\"publish,subscribe\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContext, "BASICSUB_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);

    //Initialise OpenSSL - in the product - the equivalent is ism_ssl_init();
    //TODO: Do we need to run that in the engine thread that does this or is it done?
    sslInit();

    rc = test_createClientAndSessionWithProtocol("TEST_BASICSUB",
                                                 PROTOCOL_ID_MQTT,
                                                 mockContext,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                                 ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    // This will result in an ism_engine_registerSubscriber call
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "/TEST/BASICSUB",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   NULL,
                                   0,
                                   NULL, /* No delivery callback */
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);

    // Create Property to associate with the subscription
    ism_prop_t *subProperties = ism_common_newProperties(1);
    char buffer [2048];
    memset(buffer, (int) 'b', sizeof(buffer));
    TEST_ASSERT_PTR_NOT_NULL(subProperties);
    ism_field_t NameProperty = {VT_ByteArray, sizeof(buffer), {.s = buffer }};
    rc = ism_common_setProperty(subProperties, "BASICNAME", &NameProperty);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |
                          ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
    rc = ism_engine_createSubscription(hClient,
                                       "BASICSUB",
                                       subProperties,
                                       ismDESTINATION_TOPIC,
                                       "/TEST/BASICSUB",
                                       &subAttrs,
                                       NULL,
                                       &pActionsRemaining, sizeof(pActionsRemaining),
                                       test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    ism_common_freeProperties(subProperties);

    // Publish some low QoS message
    test_publishMessages("/TEST/BASICSUB",
                         NULL,
                         TEST_SMALL_MESSAGE_SIZE,
                         10,
                         ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                         ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                         ismMESSAGE_FLAGS_NONE,
                         PROTOCOL_ID_MQTT,
                         "TEST_BASICSUB_PUBBER",
                         mockContext,
                         OK);

    // Export them all
    char *clientIdString = "TEST_BASICSUB";

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(clientIdString,
                                    NULL,
                                    fileName,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0);

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);

    expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] = 10;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDSUBSCRIPTION] = 2;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDQNODE_SIMPLE] = 10;
#ifndef  ENGINE_FORCE_INTERMEDIATE_TO_MULTI
    expectedTypeCounts[ieieDATATYPE_EXPORTEDQNODE_INTER] = 10;
#else
    expectedTypeCounts[ieieDATATYPE_EXPORTEDQNODE_MULTI] = 10;
#endif

    // Check the contents of the file are as expected
    test_checkExportFileContents(pThreadData, exportFilePath, filePassword, clientIdString, NULL,
                                 'T', 'T', '/', expectedTypeCounts);

    // Try importing the exported file twice...
    for(int32_t i=0; i<3; i++)
    {
        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_destroySubscription(hClient,
                                            "BASICSUB",
                                            hClient,
                                            &pActionsRemaining, sizeof(pActionsRemaining),
                                            test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

        test_waitForRemainingActions(pActionsRemaining);

        rc = test_destroyClientAndSession(hClient, hSession, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Try importing that file we just exported
        if (i < 2)
        {
            rc = sync_ism_engine_importResources(fileName,
                                                 filePassword,
                                                 ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                                 &requestID);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_NOT_EQUAL(requestID, 0);

            rc = test_createClientAndSessionWithProtocol("TEST_BASICSUB",
                                                         PROTOCOL_ID_MQTT,
                                                         mockContext,
                                                         ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                                         ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                         &hClient, &hSession, false);
            TEST_ASSERT_EQUAL(rc, OK); // ISMRC_ResumedClientState not returned if session created
            TEST_ASSERT_PTR_NOT_NULL(hClient);
            TEST_ASSERT_PTR_NOT_NULL(hSession);
        }
    }

    rc = test_createClientAndSessionWithProtocol("TEST_BASICSUB",
                                                 PROTOCOL_ID_MQTT,
                                                 mockContext,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener, mockTransport, mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    sslTerm();

    iemem_free(pThreadData, iemem_exportResources, exportFilePath);
    iemem_free(pThreadData, iemem_exportResources, importFilePath);
}

//****************************************************************************
/// @brief Test operation of exporting a shared subscription
//****************************************************************************
void test_capability_SharedSubscription(void)
{
    int32_t rc;

    ismEngine_ClientStateHandle_t hOwningClient;
    ismEngine_ClientStateHandle_t hClient, hClient2, hClient3;
    ismEngine_SessionHandle_t hSession, hSession2, hSession3;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    uint32_t expectedTypeCounts[ieieDATATYPE_LAST] = {0};

    expectedTypeCounts[ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER] = 1;

    ism_listener_t *mockListener=NULL, *mockListener3=NULL;
    ism_listener_t *mockListener2=NULL;
    ism_transport_t *mockTransport=NULL, *mockTransport3 = NULL;
    ism_transport_t *mockTransport2=NULL;
    ismSecurity_t *mockContext, *mockContext3;
    ismSecurity_t *mockContext2;

    // Create a client state for globally shared subs
    rc = ism_engine_createClientState("__Shared_TEST",
                                      PROTOCOL_ID_SHARED,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hOwningClient,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createSecurityContext(&mockListener, &mockTransport, &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_createSecurityContext(&mockListener2, &mockTransport2, &mockContext2);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_createSecurityContext(&mockListener3, &mockTransport3, &mockContext3);
    TEST_ASSERT_EQUAL(rc, OK);

    char fileName1[255];
    char fileName2[255];
    char *exportFilePath1 = NULL;
    char *exportFilePath2 = NULL;
    char *importFilePath1 = NULL;
    char *importFilePath2 = NULL;
    char *suffix = ".enc";
    const char *filePassword = __func__;
    uint64_t requestID;
    uint64_t prevRequestID = 0;

    strcpy(fileName1, __func__);
    strcat(fileName1, suffix);
    strcpy(fileName2, __func__);
    strcat(fileName2, "2");
    strcat(fileName2, suffix);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName1, false, &exportFilePath1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(exportFilePath1);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName1, true, &importFilePath1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(importFilePath1);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName2, false, &exportFilePath2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(exportFilePath2);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName2, true, &importFilePath2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(importFilePath2);

    printf("Starting %s...\n", __func__);

    // Add a messaging policy that allows publish / subscribe on '*SHAREDSUB*'
    // For clients whose ID contains 'SHAREDSUB'.
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.SHAREDSUB_TOPIC_POLICY\","
                                "\"Name\":\"SHAREDSUB_TOPIC_POLICY\","
                                "\"ClientID\":\"*SHAREDSUB*\","
                                "\"Topic\":\"*SHAREDSUB*\","
                                "\"ActionList\":\"publish,subscribe\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY,
                                "{\"UID\":\"UID.SHAREDSUB_SUBSCRIPTION_POLICY\","
                                "\"Name\":\"SHAREDSUB_SUBSCRIPTION_POLICY\","
                                "\"ClientID\":\"*SHAREDSUB*\","
                                "\"SubscriptionName\":\"TEST_SHAREDSUB_SUBNAME*\","
                                "\"ActionList\":\"receive,control\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContext, "SHAREDSUB_TOPIC_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext, "SHAREDSUB_SUBSCRIPTION_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext2, "SHAREDSUB_TOPIC_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext2, "SHAREDSUB_SUBSCRIPTION_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext3, "SHAREDSUB_TOPIC_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext3, "SHAREDSUB_SUBSCRIPTION_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);

    //Initialise OpenSSL - in the product - the equivalent is ism_ssl_init();
    //TODO: Do we need to run that in the engine thread that does this or is it done?
    sslInit();

    mockTransport->clientID = "TEST_SHAREDSUB";
    mockTransport->userid = "TestUser";

    rc = test_createClientAndSessionWithProtocol("TEST_SHAREDSUB",
                                                 PROTOCOL_ID_MQTT,
                                                 mockContext,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                                 ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT };

    subAttrs.subId = 23;
    rc = sync_ism_engine_createSubscription(hClient,
                                            "TEST_SHAREDSUB_SUBNAME",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "/TEST/SHAREDSUB",
                                            &subAttrs,
                                            hOwningClient);
    TEST_ASSERT_EQUAL(rc, OK);

    // Subscribe from a 2nd client state
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                          ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT;
    subAttrs.subId = ismENGINE_NO_SUBID;

    mockTransport2->clientID = "OTHER_TEST_SHAREDSUB";
    mockTransport2->userid = "OtherTestUser";

    rc = test_createClientAndSessionWithProtocol("OTHER_TEST_SHAREDSUB",
                                                 PROTOCOL_ID_MQTT,
                                                 mockContext2,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                                 ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient2, &hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient2);
    TEST_ASSERT_PTR_NOT_NULL(hSession2);

    // Reuse from different client
    rc = ism_engine_reuseSubscription(hClient2,
                                      "TEST_SHAREDSUB_SUBNAME",
                                      &subAttrs,
                                      hOwningClient);
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish some message
    test_publishMessages("/TEST/SHAREDSUB",
                         NULL,
                         TEST_SMALL_MESSAGE_SIZE,
                         10,
                         ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                         ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                         ismMESSAGE_FLAGS_NONE,
                         PROTOCOL_ID_MQTT,
                         "TEST_SHAREDSUB_PUBBER",
                         mockContext,
                         OK);
    test_publishMessages("/TEST/SHAREDSUB",
                         NULL,
                         TEST_SMALL_MESSAGE_SIZE,
                         10,
                         ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                         ismMESSAGE_PERSISTENCE_PERSISTENT,
                         ismMESSAGE_FLAGS_NONE,
                         PROTOCOL_ID_MQTT,
                         "TEST_SHAREDSUB_PUBBER",
                         mockContext,
                         OK);

    // Export them all
    char *clientIdString = "TEST_SHAREDSUB$";
    char *topicString = "/SHAREDSUB";

    test_incrementActionsRemaining(pActionsRemaining, 2);
    rc = ism_engine_exportResources(clientIdString,
                                    topicString,
                                    fileName1,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously
    TEST_ASSERT_NOT_EQUAL(requestID, prevRequestID);
    prevRequestID = requestID;

    // Make a copy for later import
    rc = ism_engine_exportResources(clientIdString,
                                    topicString,
                                    fileName2,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously
    TEST_ASSERT_NOT_EQUAL(requestID, prevRequestID);
    prevRequestID = requestID;

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_copyFile(exportFilePath1, importFilePath1);
    TEST_ASSERT_EQUAL(rc, 0);

    rc = test_copyFile(exportFilePath2, importFilePath2);
    TEST_ASSERT_EQUAL(rc, 0);

    // Check the contents of the file are as expected
    expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE] = 2;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] = 20;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDQNODE_MULTI] = 20;

    test_checkExportFileContents(pThreadData, exportFilePath1, filePassword, clientIdString, topicString,
                                 '\0', '\0', '/', expectedTypeCounts);

    // Export only one of the 2 client states that are sharing the subscription
    clientIdString = "^TEST_SHAREDSUB$";

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(clientIdString,
                                    topicString,
                                    fileName1,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously
    TEST_ASSERT_NOT_EQUAL(requestID, prevRequestID);
    prevRequestID = requestID;

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_copyFile(exportFilePath1, importFilePath1);
    TEST_ASSERT_EQUAL(rc, 0);

    expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] = 0;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDQNODE_MULTI] = 0;

    // Check the contents of the file are as expected
    test_checkExportFileContents(pThreadData, exportFilePath1, filePassword, clientIdString, topicString,
                                 'T', 'T', '/', expectedTypeCounts);

    rc = ism_engine_destroySubscription(hClient, "TEST_SHAREDSUB_SUBNAME", hOwningClient, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Clean up and destroy the 1st client
    rc = ism_engine_stopMessageDelivery(hSession, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = sync_ism_engine_destroySession(hSession);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = sync_ism_engine_destroyClientState(hClient,
                                            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);

    // Don't expect any to match this strict clientId string
    clientIdString = "^TEST_SHAREDSUB$";

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(clientIdString,
                                    topicString,
                                    fileName1,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously
    TEST_ASSERT_NOT_EQUAL(requestID, prevRequestID);
    prevRequestID = requestID;

    test_waitForRemainingActions(pActionsRemaining);

    expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE] = 0;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] = 0;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB] = 0;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDQNODE_MULTI] = 0;

    // But should match the looser string
    clientIdString = "TEST_SHAREDSUB";

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(clientIdString,
                                    topicString,
                                    fileName1,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously
    TEST_ASSERT_NOT_EQUAL(requestID, prevRequestID);
    prevRequestID = requestID;

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_copyFile(exportFilePath1, importFilePath1);
    TEST_ASSERT_EQUAL(rc, 0);

    expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] = 20;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDQNODE_MULTI] = 20;

    // Check the contents of the file are as expected
    test_checkExportFileContents(pThreadData, exportFilePath1, filePassword, clientIdString, topicString,
                                 'O', 'O', '/', expectedTypeCounts);

    rc = ism_engine_destroySubscription(hClient2, "TEST_SHAREDSUB_SUBNAME", hOwningClient, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Clean up 2nd client
    rc = test_destroyClientAndSession(hClient2, hSession2, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a JMS style globally shared subscription
    mockTransport3->clientID = "JMS_SHAREDSUB";
    mockTransport3->userid = "TestUser";

    rc = test_createClientAndSessionWithProtocol("JMS_SHAREDSUB",
                                                 PROTOCOL_ID_JMS,
                                                 mockContext3,
                                                 ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient3, &hSession3, false);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE | // JMS
                          ismENGINE_SUBSCRIPTION_OPTION_SHARED  |
                          ismENGINE_SUBSCRIPTION_OPTION_DURABLE;

    rc = sync_ism_engine_createSubscription(hClient3,
                                            "TEST_SHAREDSUB_SUBNAME_JMS",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "/TEST/SHAREDSUB",
                                            &subAttrs,
                                            hOwningClient);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_createSubscription(hClient3,
                                            "PRIVATE_SHAREDSUB_SUBNAME_JMS",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "/TEST/SHAREDSUB",
                                            &subAttrs,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish 10 messages
    test_publishMessages("/TEST/SHAREDSUB",
                         NULL,
                         TEST_SMALL_MESSAGE_SIZE,
                         10,
                         ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                         ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                         ismMESSAGE_FLAGS_NONE,
                         PROTOCOL_ID_MQTT,
                         "TEST_SHAREDSUB_PUBBER",
                         mockContext,
                         OK);

    clientIdString = "SHAREDSUB";

    // Shouldn't get the JMS global shared sub as the topic doesn't match, but will get
    // the private sub.
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(clientIdString,
                                    NULL,
                                    fileName1,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously
    TEST_ASSERT_NOT_EQUAL(requestID, prevRequestID);
    prevRequestID = requestID;

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_copyFile(exportFilePath1, importFilePath1);
    TEST_ASSERT_EQUAL(rc, 0);

    expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE] = 2; // Will have the zombie of "OTHER_TEST_SHAREDSUB" client
    expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] = 10;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDSUBSCRIPTION] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB] = 0;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDQNODE_MULTI] = 10;

    // Check the contents of the file are as expected
    test_checkExportFileContents(pThreadData, exportFilePath1, filePassword, clientIdString, NULL,
                                 '\0', '\0', '/', expectedTypeCounts);

    // Should get the JMS shared sub as the topic DOES match
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(clientIdString,
                                    topicString,
                                    fileName1,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously
    TEST_ASSERT_NOT_EQUAL(requestID, prevRequestID);
    prevRequestID = requestID;

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_copyFile(exportFilePath1, importFilePath1);
    TEST_ASSERT_EQUAL(rc, 0);

    expectedTypeCounts[ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDQNODE_MULTI] = 20;

    // Check the contents of the file are as expected
    test_checkExportFileContents(pThreadData, exportFilePath1, filePassword, clientIdString, topicString,
                                 '\0', '\0', '/', expectedTypeCounts);

    // Explicitly request Internal client
    clientIdString = "Shared";
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(clientIdString,
                                    topicString,
                                    fileName1,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_INCLUDE_INTERNAL_CLIENTIDS |
                                    ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously
    TEST_ASSERT_NOT_EQUAL(requestID, prevRequestID);
    prevRequestID = requestID;

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_copyFile(exportFilePath1, importFilePath1);
    TEST_ASSERT_EQUAL(rc, 0);

    // Will have the zombie of "OTHER_TEST_SHAREDSUB" client AND the __Shared clientIDs
    expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE] = 4;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] = 10;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDSUBSCRIPTION] = 0;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDQNODE_MULTI] = 10;

    // Check the contents of the file are as expected
    test_checkExportFileContents(pThreadData, exportFilePath1, filePassword, clientIdString, topicString,
                                 '\0', '\0', '/', expectedTypeCounts);

    rc = ism_engine_destroySubscription(hClient3,
                                        "TEST_SHAREDSUB_SUBNAME_JMS",
                                        hOwningClient,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient3, hSession3, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the imported sub doesn't exist
    ismEngine_Subscription_t *subscription = NULL;
    rc = iett_findClientSubscription(pThreadData,
                                     hOwningClient->pClientId,
                                     iett_generateClientIdHash(hOwningClient->pClientId),
                                     "TEST_SHAREDSUB_SUBNAME",
                                     iett_generateSubNameHash("TEST_SHAREDSUB_SUBNAME"),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_PTR_NULL(subscription);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_importResources(fileName2,
                                    filePassword,
                                    ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously

    test_waitForRemainingActions(pActionsRemaining);

    // Check that the imported sub does now exist
    rc = iett_findClientSubscription(pThreadData,
                                     hOwningClient->pClientId,
                                     iett_generateClientIdHash(hOwningClient->pClientId),
                                     "TEST_SHAREDSUB_SUBNAME",
                                     iett_generateSubNameHash("TEST_SHAREDSUB_SUBNAME"),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subscription);
    TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, "/TEST/SHAREDSUB");

    iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);
    TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
    TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2);
    // Check the SubIDs got imported properly
    for(uint32_t loop=0; loop<sharedSubData->sharingClientCount; loop++)
    {
        iettSharingClient_t *sharingClient = &sharedSubData->sharingClients[loop];

        if (strcmp(sharingClient->clientId, "TEST_SHAREDSUB") == 0)
        {
            TEST_ASSERT_EQUAL(sharingClient->subId, 23);
        }
        else
        {
            TEST_ASSERT_EQUAL(sharingClient->subId, ismENGINE_NO_SUBID);
        }
    }

    iett_releaseSubscription(pThreadData, subscription, false);

    rc = ism_engine_destroyDisconnectedClientState("TEST_SHAREDSUB", NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_destroyDisconnectedClientState("OTHER_TEST_SHAREDSUB", NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = iett_destroySubscriptionForClientId(ieut_getThreadData(),
                                             "__Shared_TEST",
                                             NULL,
                                             "TEST_SHAREDSUB_SUBNAME",
                                             NULL,
                                             iettSUB_DESTROY_OPTION_NONE,
                                             NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    rc = ism_engine_destroyClientState(hOwningClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK); // Shouldn't go async

    sslTerm();

    rc = test_destroySecurityContext(mockListener, mockTransport, mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_destroySecurityContext(mockListener2, mockTransport2, mockContext2);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_destroySecurityContext(mockListener3, mockTransport3, mockContext3);
    TEST_ASSERT_EQUAL(rc, OK);

    iemem_free(pThreadData, iemem_exportResources, exportFilePath1);
    iemem_free(pThreadData, iemem_exportResources, importFilePath1);
    iemem_free(pThreadData, iemem_exportResources, exportFilePath2);
    iemem_free(pThreadData, iemem_exportResources, importFilePath2);
}

void test_capability_OlderVersionSubscription(void)
{
    int32_t rc;

    // File produced by an earlier code level producing output, so
    // the password is the function that produced it in that earler version!
    const char *filePassword = "test_capability_SharedSubscription";
    const char *fileName = "test_importV1_SharedSubscription.enc";

    char *oldExportFilePath = test_getTestResourceFilePath(fileName);

    sslInit();

    if (oldExportFilePath == NULL)
    {
        printf("Skipping %s... (no export file)\n", __func__);
    }
    else
    {
        char *importFilePath = NULL;

        ismEngine_ClientStateHandle_t hOwningClient = NULL;

        printf("Starting %s...\n", __func__);

        // Create a client state for globally shared subs
        rc = ism_engine_createClientState("__Shared_TEST",
                                          PROTOCOL_ID_SHARED,
                                          ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                          NULL, NULL, NULL,
                                          &hOwningClient,
                                          NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hOwningClient);

        ieutThreadData_t *pThreadData = ieut_getThreadData();

        rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, true, &importFilePath);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(importFilePath);

        rc = test_copyFile(oldExportFilePath, importFilePath);
        TEST_ASSERT_EQUAL(rc, 0);

        ismEngine_SubscriptionHandle_t subscription = NULL;

        // Check that the imported sub doesn't exist
        rc = iett_findClientSubscription(pThreadData,
                                         hOwningClient->pClientId,
                                         iett_generateClientIdHash(hOwningClient->pClientId),
                                         "TEST_SHAREDSUB_SUBNAME",
                                         iett_generateSubNameHash("TEST_SHAREDSUB_SUBNAME"),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        TEST_ASSERT_PTR_NULL(subscription);

        uint64_t requestID;

        // Import the subscription
        rc = sync_ism_engine_importResources(fileName,
                                             filePassword,
                                             ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                             &requestID);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check that the imported sub exists and looks right
        rc = iett_findClientSubscription(pThreadData,
                                         hOwningClient->pClientId,
                                         iett_generateClientIdHash(hOwningClient->pClientId),
                                         "TEST_SHAREDSUB_SUBNAME",
                                         iett_generateSubNameHash("TEST_SHAREDSUB_SUBNAME"),
                                         &subscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(subscription);
        TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, "/TEST/SHAREDSUB");

        iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);
        TEST_ASSERT_PTR_NOT_NULL(sharedSubData);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClientCount, 2);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[0].subId, ismENGINE_NO_SUBID);
        TEST_ASSERT_EQUAL(sharedSubData->sharingClients[1].subId, ismENGINE_NO_SUBID);

        iett_releaseSubscription(pThreadData, subscription, false);

        iemem_free(pThreadData, iemem_exportResources, importFilePath);

        rc = sync_ism_engine_destroyClientState(hOwningClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
        TEST_ASSERT_EQUAL(rc, OK);

        free(oldExportFilePath);
    }

    sslTerm();
}

CU_TestInfo ISM_ExportResources_CUnit_test_capability_Subscriptions[] =
{
    { "BasicSubscription", test_capability_BasicSubscription },
    { "SharedSubscription", test_capability_SharedSubscription },
    { "OlderVersionSubscription", test_capability_OlderVersionSubscription },

    // TODO: More complicated subscription examples
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test operation of exporting retained messages
//****************************************************************************
void test_capability_BasicRetained(void)
{
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    uint32_t expectedTypeCounts[ieieDATATYPE_LAST] = {0};

    expectedTypeCounts[ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER] = 1;

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    rc = test_createSecurityContext(&mockListener, &mockTransport, &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    char fileName[255];
    char *exportFilePath = NULL;
    char *importFilePath = NULL;
    char *suffix = ".enc";
    const char *filePassword = __func__;
    uint64_t requestID;

    strcpy(fileName, __func__);
    strcat(fileName, suffix);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, false, &exportFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(exportFilePath);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, true, &importFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(importFilePath);

    printf("Starting %s...\n", __func__);

    // Add a messaging policy that allows publish on '*RETAINED*' for all clients.
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.RETAINED_TOPIC_POLICY\","
                                "\"Name\":\"RETAINED_TOPIC_POLICY\","
                                "\"ClientID\":\"*\","
                                "\"Topic\":\"*RETAINED*\","
                                "\"ActionList\":\"publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security context
    rc = test_addPolicyToSecContext(mockContext, "SHAREDSUB_TOPIC_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);

    //Initialise OpenSSL - in the product - the equivalent is ism_ssl_init();
    //TODO: Do we need to run that in the engine thread that does this or is it done?
    sslInit();

    mockTransport->clientID = "TEST_RETAINED";
    mockTransport->userid = "TestUser";

    rc = test_createClientAndSessionWithProtocol("TEST_RETAINED",
                                                 PROTOCOL_ID_MQTT,
                                                 mockContext,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                                 ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    // Publish some message
    char pubTopic[100];
    for(int32_t i=0; i<5; i++)
    {
        sprintf(pubTopic, "/TEST/RETAINED/TOPIC1/%d", i);

        // Notice: We republish on same topic to prove we only get the
        //         current retained.
        test_publishMessages(pubTopic,
                             NULL,
                             TEST_SMALL_MESSAGE_SIZE,
                             2,
                             ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                             ismMESSAGE_PERSISTENCE_PERSISTENT,
                             ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                             PROTOCOL_ID_MQTT,
                             "TEST_RETAINED_PUBBER",
                             mockContext,
                             OK);

        sprintf(pubTopic, "/TEST/RETAINED/TOPIC2/%d", i);
        test_publishMessages(pubTopic,
                             NULL,
                             TEST_SMALL_MESSAGE_SIZE,
                             1,
                             ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                             ismMESSAGE_PERSISTENCE_PERSISTENT,
                             ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                             PROTOCOL_ID_MQTT,
                             "TEST_RETAINED_PUBBER",
                             mockContext,
                             OK);
    }

    // Export the retained messages
    char *topicString = "RETAINED";

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(NULL,
                                    topicString,
                                    fileName,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);

    // Remove everything we just added & exported
    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), "RETAINED");
    TEST_ASSERT_EQUAL(rc, OK);

    // Import the messages again
    rc = sync_ism_engine_importResources(fileName,
                                         filePassword,
                                         ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                         &requestID);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(requestID, 0);

    // And import them a 2nd time to hit the republish logic
    rc = sync_ism_engine_importResources(fileName,
                                         filePassword,
                                         ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                         &requestID);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(requestID, 0);

    expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE] = 0;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] = 10;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDRETAINEDMSG] = 10;

    // Check the contents of the file are as expected
    test_checkExportFileContents(pThreadData, exportFilePath, filePassword, NULL, topicString,
                                 '\0', '\0', '/', expectedTypeCounts);

    // Export no retained messages
    topicString = "NONEOFTHESE";

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(NULL,
                                    topicString,
                                    fileName,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously

    test_waitForRemainingActions(pActionsRemaining);

    expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] = 0;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDRETAINEDMSG] = 0;

    // Export a Subset
    topicString = "TOPIC2";

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(NULL,
                                    topicString,
                                    fileName,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_NONE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);

    expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] = 5;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDRETAINEDMSG] = 5;

    // Check the contents of the file are as expected
    test_checkExportFileContents(pThreadData, exportFilePath, filePassword, NULL, topicString,
                                 'T', 'T', '/', expectedTypeCounts);

    // Publish 10 more messages
    for(int32_t i=0; i<10; i++)
    {
        sprintf(pubTopic, "/TEST/RETAINED/TOPIC1/%d", i);

        test_publishMessages(pubTopic,
                             NULL,
                             TEST_SMALL_MESSAGE_SIZE,
                             1,
                             ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                             ismMESSAGE_PERSISTENCE_PERSISTENT,
                             ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                             PROTOCOL_ID_MQTT,
                             "TEST_RETAINED_PUBBER",
                             mockContext,
                             OK);
    }

    // Now check that the new messages are included
    topicString = "RETAINED";

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(NULL,
                                    topicString,
                                    fileName,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0); // Expect requestID to be updated synchronously

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);

    expectedTypeCounts[ieieDATATYPE_EXPORTEDMESSAGERECORD] = 15;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDRETAINEDMSG] = 15;

    // Check the contents of the file are as expected
    test_checkExportFileContents(pThreadData, exportFilePath, filePassword, NULL, topicString,
                                 '\0', '\0', '/', expectedTypeCounts);

    // Clean up and destroy the client
    rc = ism_engine_stopMessageDelivery(hSession, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = sync_ism_engine_destroySession(hSession);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = sync_ism_engine_destroyClientState(hClient,
                                            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);

    sslTerm();

    rc = iett_removeLocalRetainedMessages(ieut_getThreadData(), ".*");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener, mockTransport, mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    iemem_free(pThreadData, iemem_exportResources, exportFilePath);
    iemem_free(pThreadData, iemem_exportResources, importFilePath);
}

CU_TestInfo ISM_ExportResources_CUnit_test_capability_RetainedMsgs[] =
{
    { "BasicRetained", test_capability_BasicRetained },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test Defect 152379 with a clashing file
//****************************************************************************
void test_Defect152379(void)
{
    int32_t rc;

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    uint32_t expectedTypeCounts[ieieDATATYPE_LAST] = {0};

    expectedTypeCounts[ieieDATATYPE_EXPORTEDRESOURCEFILEHEADER] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDRESOURCEFILEFOOTER] = 1;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    rc = test_createSecurityContext(&mockListener, &mockTransport, &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport->clientID = "DEFECT_152379";
    mockTransport->userid = "Defect152379User";

    char fileName[255];
    char *exportFilePath = NULL;
    char *importFilePath = NULL;
    char *suffix = ".enc";
    const char *filePassword = __func__;
    uint64_t requestID;

    strcpy(fileName, __func__);
    strcat(fileName, suffix);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, false, &exportFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(exportFilePath);
    rc = ieie_fullyQualifyResourceFilename(pThreadData, fileName, true, &importFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(importFilePath);

    printf("Starting %s...\n", __func__);

    // Add a messaging policy that allows publish / subscribe on '*DEFECT_152379*'
    // For all clients whose ID starts with 'DEFECT_152379'.
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.DEFECT_152379_POLICY\","
                                "\"Name\":\"DEFECT_152379\","
                                "\"ClientID\":\"DEFECT_152379\","
                                "\"Topic\":\"*DEFECT_152379*\","
                                "\"ActionList\":\"publish,subscribe\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContext, "DEFECT_152379");
    TEST_ASSERT_EQUAL(rc, OK);

    FILE *fp = fopen(exportFilePath, "w");
    TEST_ASSERT_PTR_NOT_NULL(fp);
    fprintf(fp, "Should not appear in the exported file...\n");
    fflush(fp);

    //Initialise OpenSSL - in the product - the equivalent is ism_ssl_init();
    //TODO: Do we need to run that in the engine thread that does this or is it done?
    sslInit();

    rc = test_createClientAndSessionWithProtocol("DEFECT_152379",
                                                 PROTOCOL_ID_MQTT,
                                                 mockContext,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |
                          ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
    rc = ism_engine_createSubscription(hClient,
                                       "DEFECT_152379",
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       "/TEST/DEFECT_152379",
                                       &subAttrs,
                                       NULL,
                                       &pActionsRemaining, sizeof(pActionsRemaining),
                                       test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    // Export
    char *clientIdString = "DEFECT_152379";

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_exportResources(clientIdString,
                                    NULL,
                                    fileName,
                                    filePassword,
                                    ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE,
                                    &requestID,
                                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_NOT_EQUAL(requestID, 0);

    test_waitForRemainingActions(pActionsRemaining);

    fprintf(fp, "A bit more test after the export is finished...\n");
    fflush(fp);

    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);

    expectedTypeCounts[ieieDATATYPE_EXPORTEDCLIENTSTATE] = 1;
    expectedTypeCounts[ieieDATATYPE_EXPORTEDSUBSCRIPTION] = 1;

    // Check the contents of the file are as expected
    test_checkExportFileContents(pThreadData, exportFilePath, filePassword, clientIdString, NULL,
                                 'D', 'D', '/', expectedTypeCounts);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroySubscription(hClient,
                                        "DEFECT_152379",
                                        hClient,
                                        &pActionsRemaining, sizeof(pActionsRemaining),
                                        test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener, mockTransport, mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    fclose(fp);

    sslTerm();

    iemem_free(pThreadData, iemem_exportResources, exportFilePath);
    iemem_free(pThreadData, iemem_exportResources, importFilePath);
}

CU_TestInfo ISM_ExportResources_CUnit_test_Defects[] =
{
    { "Defect152379", test_Defect152379 },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initExportResources(void)
{
    return test_engineInit(true, false,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termExportResources(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_ExportResources_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("Clients", initExportResources, termExportResources, ISM_ExportResources_CUnit_test_capability_Clients),
    IMA_TEST_SUITE("Subscriptions", initExportResources, termExportResources, ISM_ExportResources_CUnit_test_capability_Subscriptions),
    IMA_TEST_SUITE("RetainedMsgs", initExportResources, termExportResources, ISM_ExportResources_CUnit_test_capability_RetainedMsgs),
    IMA_TEST_SUITE("Defects", initExportResources, termExportResources, ISM_ExportResources_CUnit_test_Defects),
    CU_SUITE_INFO_NULL,
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

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    // Override the dummy transport functions with our own
    ism_prop_t *staticConfigProps = ism_common_getConfigProperties();
    ism_field_t f;

    f.type = VT_Long;
    f.val.l = (uint64_t)(uintptr_t)test_ism_transport_disableClientSet;
    retval = ism_common_setProperty(staticConfigProps, ismENGINE_CFGPROP_DISABLECLIENTSET_FNPTR, &f);
    if (retval != OK) goto mod_exit;

    f.val.l = (uint64_t)(uintptr_t)test_ism_transport_enableClientSet;
    retval = ism_common_setProperty(staticConfigProps, ismENGINE_CFGPROP_ENABLECLIENTSET_FNPTR, &f);
    if (retval != OK) goto mod_exit;

    ism_time_t seedVal = ism_common_currentTimeNanos();

    srand(seedVal);

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_ExportResources_CUnit_allsuites);

    if (retval == 0)
    {
        CU_basic_run_tests();

        CU_RunSummary * CU_pRunSummary_Final;
        CU_pRunSummary_Final = CU_get_run_summary();
        printf("Random Seed =     %"PRId64"\n", seedVal);
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

    test_processTerm(retval == 0);

mod_exit:

    return retval;
}
