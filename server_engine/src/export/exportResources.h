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

//****************************************************************************
/// @file  exportResources.h
/// @brief API functions to export (and import) engine resources
//****************************************************************************
#ifndef __ISM_EXPORTRESOURCES_DEFINED
#define __ISM_EXPORTRESOURCES_DEFINED

#include "engineInternal.h"
#include "exportCrypto.h"
#include "engineHashTable.h"
#include "engineHashSet.h"

// Function prototypes for the transport functions which we need to call during export
typedef int (*ism_transport_disableClientSet_f)(const char * regex_str, int rc);
typedef int (*ism_transport_enableClientSet_f)(const char * regex_str);

//****************************************************************************
/// @brief Type of requests
//****************************************************************************
typedef enum tag_ieieRequestType_t
{
    ieieREQUEST_NONE = 0,      ///< No request
    ieieREQUEST_EXPORT = 1,    ///< Export request
    ieieREQUEST_IMPORT = 2,    ///< Import request
} ieieRequestType_t;

#define ieieEXPORT_THREADNAME_PREFIX "export_"
#define ieieIMPORT_THREADNAME_PREFIX "import_"

//****************************************************************************
/// @brief Global structure for import / export resources
//****************************************************************************
typedef struct tag_ieieImportExportGlobal_t
{
    char                              strucId[4];                     ///< Eyecatcher "IMEX"
    uint32_t                          maxActiveRequests;              ///< Maximum number of concurrent active import/export requests
    ism_time_t                        serverInitTime;                 ///< The time at which the server was initialized
    uint64_t                          nextRequestID;                  ///< Next requestID to use for an import or export
    ism_transport_disableClientSet_f  ism_transport_disableClientSet; ///< Function pointer for the transport disableClientSet function
    ism_transport_enableClientSet_f   ism_transport_enableClientSet;  ///< Function pointer for the transport enableClientSet function
    ieutHashTable_t                  *activeImportClientIdTable;      ///< Table to hold the clientIds actively being imported
    pthread_mutex_t                   activeRequestLock;              ///< Lock on the count of active requests
    uint32_t                          activeRequests;                 ///< Current active import/export requests
    bool                              stopCalled;                     ///< Whether the stop function has been called
} ieieImportExportGlobal_t;

#define ieieIMPORTEXPORT_GLOBAL_STRUCID      "IMEX"
#define ieieIMPORTEXPORT_MAX_REQUESTID_SEED  100000UL

//****************************************************************************
/// @brief Header found at the start of an engine resource export file
///
/// @remark Following after this structure in an exported file will be, in order:
///
/// - char[]      ServerName                - The server Name of the exporting server.
/// - char[]      ServerUID                 - The server UID of the exporting server.
/// - char[]      ClientId                  - The RegEx specified for clientId
/// - char[]      Topic                     - The RegEx specified for topic
///
/// If any of the associated lengths are zero, the field is ommitted.
///
/// Character arrays (and lengths) include a NULL terminator.
//****************************************************************************
typedef struct tag_ieieResourceFileHeader_t
{
    char                        StrucId[4];           ///< Eyecatcher "EXRF"
    uint32_t                    Version;              ///< The version of the export file
    uint64_t                    RequestID;            ///< RequestID associated with the export request
    ism_time_t                  StartTime;            ///< The time at which the export was started
    uint32_t                    ServerNameLength;     ///< Length of the serverName of the exporting server
    uint32_t                    ServerUIDLength;      ///< Length of the serverUID of the exporting server
    uint32_t                    Options;              ///< The ismENGINE_EXPORT_RESOURCES_OPTION_* specified
    uint32_t                    ClientIdLength;       ///< Length of (optional) ClientID specified
    uint32_t                    TopicLength;          ///< Length of the topic specified
} ieieResourceFileHeader_t;

#define ieieRESOURCEFILE_STRUCID             "EXRF"

#define ieieRESOURCEFILE_VERSION_1           1
#define ieieRESOURCEFILE_VERSION_2           2
#define ieieRESOURCEFILE_VERSION_3           3
#define ieieRESOURCEFILE_VERSION_4           4
#define ieieRESOURCEFILE_CURRENT_VERSION     ieieRESOURCEFILE_VERSION_4

//****************************************************************************
/// @brief Footer found at the end of an engine resource export file
//****************************************************************************
typedef struct tag_ieieResourceFileFooter_t
{
    char                        StrucId[4];           ///< Eyecatcher "EXRF"
    uint32_t                    Version;              ///< The version of the export file (ought to match header)
    uint64_t                    RequestID;            ///< RequestID (ought to match the header)
    ism_time_t                  EndTime;              ///< The time at which the export ended
} ieieResourceFileFooter_t;

//****************************************************************************
/// @brief Structure holding diagnostics for failed import / export requests
//****************************************************************************
typedef struct tag_ieieDiagnosticInfo_t
{
    ieieDataType_t resourceDataType;         ///< The data type of the object resulting in this diagnostic
    uint64_t resourceDataId;                 ///< The dataId in the import / export file that caused the diagnostic
    char *resourceIdentifier;                ///< A 'human readable' identifier for the object causing the problem
    int32_t resourceRC;                      ///< A return code indicating the nature of the problem
    struct tag_ieieDiagnosticInfo_t *next;   ///< Next entry in the list of diagnostics
} ieieDiagnosticInfo_t;

//****************************************************************************
/// @brief Structure holding information about resources being exported
//****************************************************************************
typedef struct tag_ieieExportResourceControl_t
{
    uint32_t                         options;                          ///< The options specified on the export request
    bool                             exportWentAsync;                  ///< Did the import go async or did the main thread run continuously
    bool                             clientSetDisabled;                ///< Whether ism_protocol_disableClientSet has been called
    uint64_t                         requestID;                        ///< Request identifier for this export request
    char                            *stringRequestID;                  ///< String form of request identifier for inclusion in log msgs
    char                            *fileName;                         ///< The file name specified on the request
    char                            *filePath;                         ///< The fully qualified path to the file
    char                            *statusFilePath;                   ///< The fully qualified path to the status file
    char                            *clientId;                         ///< The clientId requested
    ism_regex_t                      regexClientId;                    ///< Compiled regular expression of clientId (or NULL)
    ieutHashTable_t                 *clientIdTable;                    ///< Table to hold the clientIds being exported
    char                            *topic;                            ///< The topic requested
    ism_regex_t                      regexTopic;                       ///< Compiled regular expression of topic (or NULL)
    ieieEncryptedFileHandle_t        file;                             ///< The file to which we are writing
    char                            *exportServerName;                 ///< The server name of (this) the exporting server
    char                            *exportServerUID;                  ///< The server UID of (this) the exporting server
    ieutHashSet_t                   *exportedMsgs;                     ///< Hash set containing pointer to already exported messages
    int32_t                          dataRecordRC;                     ///< rc for inclusion in the status file
    ismEngine_CompletionCallback_t   pCallerCBFn;                      ///< Callback if we go async
    void                            *pCallerContext;                   ///< Context for engine caller if we go async
    ism_time_t                       serverInitTime;                   ///< Server init time in place when the request started
    ism_time_t                       startTime;                        ///< The start time of this export request
    ism_time_t                       endTime;                          ///< The end time of this export request
    ism_time_t                       statusUpdateTime;                 ///< The time the status file was last updated
    uint64_t                         recordsWritten;                   ///< Number of records written
    uint64_t                         writtenCount[ieieDATATYPE_LAST];  ///< Count of the different data types written
    pthread_rwlock_t                 diagnosticsLock;                  ///< Lock required to add to the list of diagnostics
    ieieDiagnosticInfo_t            *latestDiagnostic;                 ///< Pointer to the latest diagnostic
} ieieExportResourceControl_t;

//****************************************************************************
/// @brief Structure holding information about resources being imported
//****************************************************************************
typedef struct tag_ieieImportResourceControl_t
{
    uint32_t                        options;                              ///< Options specified on the import request
    uint64_t                        requestID;                            ///< Request identifier for this import
    char                           *stringRequestID;                      ///< String form of request identifier for inclusion in log msgs
    char                           *exportServerName;                     ///< ServerName stored in the export header (i.e. the exporting server name)
    char                           *exportServerUID;                      ///< ServerUID stored in the export header (i.e. the exporting server UID)
    char                           *clientId;                             ///< ClientId specified in export header
    char                           *topic;                                ///< Topic specified in export header
    char                           *filePath;                             ///< The fully qualified path to the file
    char                           *statusFilePath;                       ///< The fully qualified path to the status file
    ieieEncryptedFileHandle_t       file;                                 ///< The file from which we are reading
    char                           *importServerName;                     ///< The server name of (this) the importing server
    char                           *importServerUID;                      ///< The server UID of (this) the importing server
    uint64_t                        validatedCount[ieieDATATYPE_LAST];    ///< Count of the different data types validated
    uint64_t                        recCountStarted[ieieDATATYPE_LAST];   ///< Count of the different data types started processing
    uint64_t                        recCountFinished[ieieDATATYPE_LAST];  ///< Count of the different data types finished processing
    uint64_t                        importTasksInProgress;                ///< Count of how many things are being imported
    uint64_t                        validatedZombieClientCount;           ///< Count of the validated clients that have existing Zombies
    ieutHashTable_t                *validatedClientIds;                   ///< ClientIds that have been validated and added to the activeImportClientIdTable
    ieutHashTable_t                *importedMsgs;                         ///< Hash table mapping message dataId to imported message
    ieutHashTable_t                *importedClientStates;                 ///< Hash table mapping clientState dataId to imported ClientState
    ieutHashTable_t                *importedSubscriptions;                ///< Hash table mapping (subscription) queueHandle dataId to imported Subscription
    pthread_rwlock_t                importedTablesLock;                   ///< Lock covering access to all imported tables (should we have one each?)
    bool                            importWentAsync;                      ///< Did the import go async or did the main thread run continuously
    bool                            readRecordsCompleted;                 ///< Have we finished reading records
    bool                            dataReady;                            ///< Have we got an unencrypted data record ready to process
    uint8_t                        *data;                                 ///< data buffer read from the import file
    size_t                          dataLen;                              ///< length of the data buffer
    uint64_t                        dataId;                               ///< Id of the information in the data buffer
    ieieDataType_t                  dataType;                             ///< Type of the data in the data buffer
    int32_t                         dataRecordRC;                         ///< rc for importing last record
    ismEngine_CompletionCallback_t  pCallerCBFn;                          ///< Callback if we go async
    void                           *pCallerContext;                       ///< Context for engine caller if we go async
    ism_time_t                      serverInitTime;                       ///< Server init time in place when the request started
    ism_time_t                      startTime;                            ///< The start time of this import request
    ism_time_t                      endTime;                              ///< The end time of this import request
    ism_time_t                      statusUpdateTime;                     ///< The time the status file was last updated
    uint64_t                        recordsRead;                          ///< Number of records read
    pthread_rwlock_t                diagnosticsLock;                      ///< Lock required to add to the list of diagnostics
    ieieDiagnosticInfo_t           *latestDiagnostic;                     ///< Pointer to the latest diagnostic
    ism_time_t                      lowestClientStateExpiryCheckTime;     ///< The lowest expiry check schedule time seen during import
} ieieImportResourceControl_t;

// Capacities for tables used during import / export
#define ieieIMPORTEXPORT_INITIAL_EXPORT_CLIENTID_TABLE_CAPACITY       5000
#define ieieIMPORTEXPORT_INITIAL_IMPORT_CLIENTID_TABLE_CAPACITY       5000
#define ieieIMPORTEXPORT_INITIAL_ACTIVE_CLIENTID_TABLE_CAPACITY      25000
#define ieieIMPORTEXPORT_IMPORTED_MSG_TABLE_CAPACITY_LIMIT           25000
#define ieieIMPORTEXPORT_IMPORTED_CLIENT_TABLE_CAPACITY_LIMIT        25000
#define ieieIMPORTEXPORT_IMPORTED_SUBSCRIPTION_TABLE_CAPACITY_LIMIT  25000

#define ieieRESOURCEFILE_EXPORTDIRECTORY_PROPERTY "ExportDir"
#define ieieRESOURCEFILE_IMPORTDIRECTORY_PROPERTY "ImportDir"

// NOTE: 18446744073709551615 == UINT64_MAX
#define ieieSTATUSFILE_PREFIX       "request_"
#define ieieSTATUSFILE_TYPE         ".status"
#define ieieSTATUSFILE_TMP_TYPE     ".tmp"

#define ieieSTATUSFILE_OPEN_FLAGS   O_CREAT|O_WRONLY|O_EXCL
#define ieieSTATUSFILE_PERMISSIONS  S_IRUSR|S_IWUSR

#define ieieEXAMPLE_STATUSFILE_REQUESTID "18446744073709551615"
#define ieieEXAMPLE_STATUSFILE_NAME      ieieSTATUSFILE_PREFIX ieieEXAMPLE_STATUSFILE_REQUESTID
#define ieieEXAMPLE_STATUSFILE_FULLNAME  ieieEXAMPLE_STATUSFILE_NAME ieieSTATUSFILE_TYPE

#define ieieSTATUS_IN_PROGRESS   0
#define ieieSTATUS_COMPLETED     1
#define ieieSTATUS_FAILED        2

#define ieieEXPORT_UPDATE_FREQUENCY 1000  ///< After writing how many records do we update the status file?
#define ieieIMPORT_UPDATE_FREQUENCY 1000  ///< After reading how many records do we update the status file?

#define ieieJSON_VALUES_PER_DIAGNOSTIC 4

int32_t ieie_initImportExport(ieutThreadData_t *pThreadData);
int32_t ieie_stopImportExport(ieutThreadData_t *pThreadData);
int32_t ieie_destroyImportExport(ieutThreadData_t *pThreadData);

int32_t ieie_fullyQualifyResourceFilename(ieutThreadData_t *pThreadData,
                                          const char *fileName,
                                          bool forImport,
                                          char **filePath);

int32_t ieie_writeExportRecordFrags(ieutThreadData_t *pThreadData,
                                    ieieExportResourceControl_t *pControl,
                                    ieieDataType_t recordType,
                                    uint64_t dataId,
                                    ieieFragmentedExportData_t *dataFrags);

int32_t ieie_writeExportRecord(ieutThreadData_t *pThreadData,
                               ieieExportResourceControl_t *pControl,
                               ieieDataType_t recordType,
                               uint64_t dataId,
                               uint32_t dataLen,
                               void *data);

void ieie_finishImportRecord(ieutThreadData_t *pThreadData,
                             ieieImportResourceControl_t *pControl,
                             ieieDataType_t recordDataType );

int32_t ieie_importTaskFinish( ieutThreadData_t *pThreadData
                             , ieieImportResourceControl_t *pControl
                             , bool doRestartIfNecessary
                             , bool *pRestartNeeded);

void ieie_recordImportError( ieutThreadData_t *pThreadData
                           , ieieImportResourceControl_t *pControl
                           , ieieDataType_t recordType
                           , uint64_t internalImportDataId
                           , const char *humanIdentifier
                           , int32_t rc);

void ieie_recordExportError( ieutThreadData_t *pThreadData
                           , ieieExportResourceControl_t *pControl
                           , ieieDataType_t recordType
                           , uint64_t internalExportDataId
                           , const char *humanIdentifier
                           , int32_t rc);

#endif
