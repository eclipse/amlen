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
/// @file  engineRestore.c
/// @brief Functions for restoring engine data from the store
//****************************************************************************
#define TRACE_COMP Engine

#include <stdio.h>
#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "engineRestore.h"
#include "engineRestoreTable.h"
#include "engineStore.h"
#include "memHandler.h"
#include "clientState.h"
#include "queueCommon.h"
#include "topicTree.h"
#include "storeMQRecords.h"
#include "queueNamespace.h"    // ieqn functions & constants
#include "remoteServers.h"     // iers functions & constants
#include "engineUtils.h"
#include "resourceSetStats.h"  // iere functions & constants

#ifdef USEFAKE_ASYNC_COMMIT
#include "fakeAsyncStore.h"
#endif


//Hacky prototype TODO: Alter Engine Makefile so we can find jansson.h and friends
XAPI ism_prop_t * ism_config_json_getObjectProperties(const char * objectName, const char * instName, int getLock);


// Suppress the warning about an empty gnu_printf format string caused by a LOG message with no inserts
#pragma GCC diagnostic ignored "-Wformat-zero-length"

static iertTable_t *recordTable[ISM_STORE_RECTYPE_MAXVAL]; ///<For some types of records (see ierr_initialiseRecordsTables for details) we track each record we restore
static iertTable_t *pairRequesterData[ISM_STORE_RECTYPE_MAXVAL]; ///<Some record types request others (e.g. Subscription Definition Records request Subscription Properties Records)
                                                           /// These tables are keyed off the handle of the desired record (and the value is the reconstructed requested record)
static iertTable_t *transactionMembersTable;   ///< Tracks records/references involved in a transaction

static ismStore_Handle_t *deleteSCRs = NULL; ///< Array of SCR Store handles to delete after restart
static uint32_t deleteSCRCount = 0;          ///< Count of SCRs to delete
static uint32_t deleteSCRCapacity = 0;       ///< Capacity of the SCR deletion array

//How do we handle problems in recovery
bool abortOnFirstRecoveryFailure = false;  ///< If set to true, rather than waiting until the end of recovery we core as soon as possible to get diagnostics
static bool partialRecoveryAllowed = false;       ///< If we find inconsistencies in the store do we stop at the end of recovery (and wait for service or a clean store) - the default
                                                  ///  (or do we carry on and get recover partial data from the store)

//What problems have been found in recovery
static int32_t firstRecoveryRC = OK;        ///< first RC we encountered during recovery. By default recovery will not complete if this is not ok
static iertTable_t *corruptRecords = NULL;  ///< If we are allowing partial store recovery then this table is a list of records we couldn't recover and should be removed
static uint64_t itemsDiscarded = 0;         ///< Records, references etc. that we've thrown away


///When we are iterating through a table of owner-records, this structure provides the context it needs
///to reconstruct the parent-child relationship;
typedef struct tag_ierrReferenceRecoveryContext_t
{
    ismStore_GenId_t genId;  ///< Generation we're currently reading
    ismStore_RecordType_t ownerRecType; ///< record type of the owner of the references
    uint32_t childRecType; ///<If set to non-0, child is assumed to have already been read in
    ierr_RehydrateRefFn_t pRefFn; ///< Function that can reconstruct the relationship
    void *pRehydrationContext; ///< Context the function needs to reconstruct the relationship
} ierrReferenceRecoveryContext_t;

///The first phase of restart is reading data out of the store. This task can be subdivided into the following types of action
typedef enum tag_ierrPhase1OperationType_t {
    ierrP1Record,         ///< Reconstruct a record
    ierrP1RequestedRec,   ///< Reconstruct a record into the record requesting it
    ierrP1References,     ///< Read Owner/Child references
    ierrP1GenerationDone  ///< Finished reading this generation, move to the next one
} ierrPhase1OperationType_t;

///The order of how data is read from the store is recorded by an array of the following structure:
typedef struct tag_ierrOperationsPhase1_t {
    ierrPhase1OperationType_t opType; ///<What type of action does this structure represent...
    ismStore_RecordType_t primaryType;///<For ierrP1Record or ierrP1RequestedRec, this is the type being read.
                                      ///<For ierrP1References, this is the owner type
    uint32_t secondaryType;           ///<For ierrP1Record, this is the type it can request (or  0= None)
                                      ///<For ierrP1RequestedRec, this is the type that can request it
                                      ///<For ierrP1references, this is the child type
    void *pContext; ///< Context to give to the rehydration routines
    ierr_RehydrateRecordFn_t pRecordFn; ///< Function to rehydrate a record
    ierr_PairCompletedFn_t pRecPairFn; ///< Function to reconstruct a record and the record it requested
    ierr_RehydrateRefFn_t pRefFn;   ///< Function to rehydrate a reference
} ierrOperationsPhase1_t;

static inline int32_t ierr_addOfflineMessage(ieutThreadData_t *pThreadData,
                                             ismEngine_Message_t *msg);
static inline int32_t ierr_addOfflineTransactionMemberData(ieutThreadData_t *pThreadData,
                                                           ismEngine_RestartTransactionData_t *pTranData);
static int32_t ierr_loadOfflineData( ieutThreadData_t *pThreadData );
static int32_t ierr_recoverOfflineTransactionMemberData( ieutThreadData_t *pThreadData
                                                       , ismEngine_RestartTransactionData_t *pTransactionData
                                                       , bool allowBlock);

///@brief Set up sizes for the tables used in restart
///
///@param[in] recoveryOps List of operations for each generation...used in determining which tables we need
///
///@remark Chain numbers are picked from: http://planetmath.org/goodhashtableprimes
///@remark Only some record types are tracked. Defining a table for a record type in this function will
///        cause each record of that type read to be added to the table
///@remark if new types of table are created in this function... free them in ierr_freeRecordsTables
static inline int32_t ierr_initialiseRecordsTables(ieutThreadData_t *pThreadData,
                                                   ierrOperationsPhase1_t recoveryOps[])
{
    /* Create tables for record types that are owners  */

    //SDRs own SPRs
    int32_t rc = iert_createTable( pThreadData
                                   , &(recordTable[ISM_STORE_RECTYPE_SUBSC])
                                   , iertTensOfThousandsOfItems
                                   , false
                                   , true
                                   , 0
                                   , 0);

    //CSRs own MDRs
    if (rc == OK)
    {
        rc = iert_createTable( pThreadData
                               , &(recordTable[ISM_STORE_RECTYPE_CLIENT])
                               , iertTensOfThousandsOfItems
                               , false
                               , true
                               , 0
                               , 0);
    }

    //QDRs own QPRs
    if (rc == OK)
    {
        rc = iert_createTable( pThreadData
                               , &(recordTable[ISM_STORE_RECTYPE_QUEUE])
                               , iertThousandsOfItems
                               , false
                               , true
                               , 0
                               , 0);
    }

    //TRs own TORs
    if (rc == OK)
    {
        rc = iert_createTable( pThreadData
                               , &(recordTable[ISM_STORE_RECTYPE_TRANS])
                               , iertThousandsOfItems
                               , false
                               , false
                               , 0
                               , 0);
    }

    //TDRs own RMRs
    if (rc == OK)
    {
        rc = iert_createTable( pThreadData
                               , &(recordTable[ISM_STORE_RECTYPE_TOPIC])
                               , iertThousandsOfItems
                               , false
                               , false
                               , 0
                               , 0);
    }

    //BMRs own BXRs
    if (rc == OK)
    {
        rc = iert_createTable( pThreadData
                               , &(recordTable[ISM_STORE_RECTYPE_BMGR])
                               , iertHundredsOfItems
                               , false
                               , false
                               , 0
                               , 0);
    }

    //RDRs own RPRs
    rc = iert_createTable( pThreadData
                           , &(recordTable[ISM_STORE_RECTYPE_REMSRV])
                           , iertHundredsOfItems
                           , false
                           , false
                           , 0
                           , 0);

    //Need a table for messages as they are children and we need to find them when we have a reference to one
    if (rc == OK)
    {
        rc = iert_createTable( pThreadData
                               , &(recordTable[ISM_STORE_RECTYPE_MSG])
                               , iertHundredsOfThousandsOfItems
                               , true
                               , false
                               , offsetof(ismEngine_Message_t, StoreMsg.parts.hStoreMsg)
                               , offsetof(ismEngine_Message_t, recovNext));
    }

    //Keep track of client property records as we iterate through them to fire will messages
    if (rc == OK)
    {
        rc = iert_createTable( pThreadData
                               , &(recordTable[ISM_STORE_RECTYPE_CPROP])
                               , iertThousandsOfItems
                               , false
                               , true
                               , 0
                               , 0);
    }


    //For each type that is requested by another types, set up a hash table to
    //keep track of the requesting item
    for(int i = 0; (rc == OK) && (recoveryOps[i].opType != ierrP1GenerationDone); i++)
    {
        if(   (recoveryOps[i].opType == ierrP1RequestedRec)
           && (pairRequesterData[recoveryOps[i].primaryType]) == NULL )
        {

            rc = iert_createTable( pThreadData
                                 , &(pairRequesterData[recoveryOps[i].primaryType])
                                 , iertTensOfThousandsOfItems
                                 , false
                                 , false
                                 , 0
                                 , 0);
        }
    }

    //Create a table that tracks records/references involved in a transaction
    if (rc == OK)
    {
        rc = iert_createTable( pThreadData
                             , &transactionMembersTable
                             , iertTensOfThousandsOfItems
                             , false
                             , false
                             , 0
                             , 0);
    }

    return rc;
}

///@brief create an empty ismEngine_Message_t structure
static int32_t ierr_createOfflineMessage( ieutThreadData_t *pThreadData
                                        , ismStore_Handle_t recHandle
                                        , ismEngine_Message_t **ppMessage )
{
    ismEngine_Message_t *pMessage = NULL;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, recHandle, ENGINE_HIGH_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(recHandle != ismSTORE_NULL_HANDLE);

    pMessage = iere_calloc(pThreadData,
                           iereNO_RESOURCE_SET,
                           IEMEM_PROBE(iemem_messageBody, 3), 1, sizeof(ismEngine_Message_t));
    if (pMessage != NULL)
    {
        ismEngine_SetStructId(pMessage->StrucId, ismENGINE_MESSAGE_STRUCID);
        pMessage->usageCount = 0;
        pMessage->StoreMsg.parts.hStoreMsg = recHandle;
        pMessage->StoreMsg.parts.RefCount = 0;
        assert(pMessage->resourceSet == iereNO_RESOURCE_SET);

        // This message is going to be allocated in multiple parts (header and body)
        // indicate this in the message flags now.
        pMessage->Flags = ismENGINE_MSGFLAGS_ALLOCTYPE_1;

        // A bit horrible, but we need this set in order that the various
        // checks during rehydrate work
        pMessage->Header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;

        *ppMessage = pMessage;
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

    ieutTRACEL(pThreadData, pMessage,  ENGINE_HIGH_TRACE, FUNCTION_EXIT "rc=%d, hMessage=%p\n", __func__, rc, pMessage);
    return rc;
}

///@brief free tables initialised by ierr_initialiseRecordsTables
///@remark Which tables to free depends on whether we've started messaging
static inline void ierr_freeRecordsTables(ieutThreadData_t *pThreadData, bool finishedStartMessaging)
{
    for (int i = 0; i<ISM_STORE_RECTYPE_MAXVAL; i++)
    {
        if (recordTable[i] != NULL &&
            (finishedStartMessaging || iert_needForStartMessaging(recordTable[i]) == false))
        {
            iert_freeTable(pThreadData, &(recordTable[i]));
            recordTable[i] = NULL;
        }

        if (pairRequesterData[i] != NULL &&
            (finishedStartMessaging || iert_needForStartMessaging(pairRequesterData[i]) == false))
        {
            iert_freeTable(pThreadData, &(pairRequesterData[i]));
            pairRequesterData[i] = NULL;
        }
    }

    if (transactionMembersTable != NULL &&
        (finishedStartMessaging || iert_needForStartMessaging(transactionMembersTable) == false))
    {
        iert_freeTable(pThreadData, &transactionMembersTable);
        transactionMembersTable = NULL;
    }
}

///@brief Add a table entry for a rehydrated record (if we are tracking that rectype)
///@remark Adds an entry if a table was created in ierr_initialiseRecordsTables()
static int32_t ierr_recordRehydratedRecord(ieutThreadData_t *pThreadData,
                                           ismStore_RecordType_t recType,
                                           ismStore_Handle_t recHandle,
                                           void *rehydratedRecord)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, recHandle, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    //If it's a record type we need to store...
    if(recordTable[recType] != NULL)
    {
        rc = iert_addTableEntry(pThreadData, &recordTable[recType],recHandle, rehydratedRecord);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


///@brief Read in a specific record from the handle
///
///Some records e.g. messages, are only read in when we find something that
///references them. This function reads in a record once we find a reference to it...
///
///@param[in] recHandle          Handle of the requested record
///@param[in] recType            Record type of the requested record
///@param[in] rehydrateFunction  Function to call to rehydrate the requested record
///@param[out] outObject         Rehydrated object that references this message.
static int32_t ierr_recoverRecordFromHandle( ieutThreadData_t *pThreadData
                                           , ismStore_Handle_t    recHandle
                                           , uint32_t recType
                                           , ierr_RehydrateRecordFn_t rehydrateFunction
                                           , void **outObject )
{
    ieutTRACEL(pThreadData, recHandle,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "hdl=0x%lx\n", __func__, recHandle);
    int32_t rc = OK;
    ismStore_Record_t record = {0};

    //Set up the record to point to a 10k buffer on the stack....
    uint32_t stackRecBufferSize = 10240;
    char stackRecBuffer[stackRecBufferSize];

    char *pFrags[1] = { stackRecBuffer };
    uint32_t pFragsLengths[1] = { stackRecBufferSize };
    record.FragsCount = 1;
    record.pFrags = pFrags;
    record.pFragsLengths = pFragsLengths;
    record.DataLength = record.pFragsLengths[0]; // We only pass a single fragment

    // We allow the ism_store_readRecord function to block when reading the record
    // would cause a generation to be loaded for any record type *EXCEPT messages*
    bool allowBlock = (recType != ISM_STORE_RECTYPE_MSG);

    //Try and read the record....
    rc = ism_store_readRecord(recHandle, &record, allowBlock);

    //We can't read the record at the moment... we'll use a partial "stub" instead.
    if (rc == ISMRC_WouldBlock)
    {
        assert(allowBlock == false); // We only expect this return code if weren't prepared to be blocked.

        ismEngine_Message_t *pMessage = NULL;

        // First create the 'offline' message.. we know that message records are not
        // created transactionally (so we don't look in the transactionMembers table)
        rc = ierr_createOfflineMessage( pThreadData
                                      , recHandle
                                      , &pMessage);
        if (rc == OK)
        {
            // Add then add it to the table of offline messages
            rc = ierr_addOfflineMessage(pThreadData, pMessage);

            if (rc == OK)
            {
                //Let the caller know about the record we've rehydrated
                *outObject = pMessage;

                //Add it to the hash tables of rehydrated objects
                rc = ierr_recordRehydratedRecord(pThreadData,
                                                 recType,
                                                 recHandle,
                                                 pMessage);
            }
        }
    }
    else
    {
        if (rc == ISMRC_StoreBufferTooSmall)
        {
            record.pFrags[0] = iemem_malloc( pThreadData
                                           , IEMEM_PROBE(iemem_restoreTable, 4)
                                           , record.DataLength);
            record.pFragsLengths[0] = record.DataLength;
    
            rc = ism_store_readRecord(recHandle, &record, true);
        }

        if (rc == OK)
        {
            ismEngine_RestartTransactionData_t *transData = NULL;

            if (recType != ISM_STORE_RECTYPE_TRANS)
            {
                //Try and find any information about a transaction this record is involved in, if any...
                //(Needed before we try and reconstruct it
                transData = iert_getTableEntry(transactionMembersTable, recHandle);
            }

            if (rc == OK)
            {
                void *rehydratedRecord = NULL;

                rc = rehydrateFunction(pThreadData, recHandle, &record, transData, &rehydratedRecord, NULL);

                if (rc == OK)
                {
                    //Let the caller know about the record we've rehydrated
                    *outObject = rehydratedRecord;

                    //Add it to the hash tables of rehydrated objects (if necessary)...
                    rc = ierr_recordRehydratedRecord(pThreadData,
                                                     recType,
                                                     recHandle,
                                                     rehydratedRecord);
                }
            }


            if (transData != NULL)
            {
                //We've found the entry that this transactional data is referring to, so we no longer need it
                int32_t rc2 = iert_removeTableEntry(pThreadData,
                                                    transactionMembersTable,
                                                    recHandle);
                if (rc == OK) rc = rc2;

                iemem_free(pThreadData, iemem_restoreTable, transData);
            }
        }
        else
        {
            if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
            {
                //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
                ism_admin_setMaintenanceMode(rc, 0);
            }

            ieutTRACE_FFDC(ieutPROBE_001, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                           "Unexpectedly couldn't read record", rc,
                           "Record Type", &recType, sizeof(recType),
                           "Record Handle", &(recHandle), sizeof(recHandle),
                           NULL);
        }

        if (record.pFrags[0] != stackRecBuffer)
        {
            iemem_free(pThreadData, iemem_restoreTable, record.pFrags[0]);
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//*************************************************************************
/// @brief Deserialises a SCR (Server Configuration Record) from the store
///
/// The SCR contains only a timestamp updated periodically as the server
/// is running.
//*************************************************************************
static inline int32_t ierr_rehydrateServerRecord(ieutThreadData_t *pThreadData,
                                                 ismStore_Handle_t recHandle,
                                                 ismStore_Record_t *record,
                                                 ismEngine_RestartTransactionData_t *transdata,
                                                 void **outData,
                                                 void *pContext)
{
    int32_t rc = OK;
    ieutTRACEL(pThreadData, recHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    iestServerConfigRecord_t *pSCR = (iestServerConfigRecord_t *)(record->pFrags[0]);

    uint32_t thisTimestamp = (uint32_t)(record->State >> 32);

    // Trace out the timestamp stored in this record
    ism_ts_t *ts = ism_common_openTimestamp(ISM_TZF_UTC);
    if (ts != NULL)
    {
        char buffer[80];

        ism_time_t dtime = ism_common_convertExpireToTime(thisTimestamp);

        ism_common_setTimestamp(ts, dtime);
        ism_common_formatTimestamp(ts, buffer, 80, 6, ISM_TFF_ISO8601);
        ism_common_closeTimestamp(ts);

        ieutTRACEL(pThreadData, dtime, ENGINE_INTERESTING_TRACE, "Version %u SCR found. Last server timestamp: %s.\n", pSCR->Version, buffer);
    }

    // We only expect a single SCR to be remembered in hStoreSCR
    if (ismEngine_serverGlobal.hStoreSCR == ismSTORE_NULL_HANDLE)
    {
        if (LIKELY(pSCR->Version == iestSCR_CURRENT_VERSION))
        {
            ismEngine_serverGlobal.hStoreSCR = recHandle;

            // The last time at which the server was running is the upper 32 bits of
            // the SCR's state.
            ismEngine_serverGlobal.ServerTimestamp = thisTimestamp;
            ismEngine_serverGlobal.ServerShutdownTimestamp = thisTimestamp;
        }
        // If this is a version 1 SCR, we want to remember the time stamp from it if
        // it's newer than the one we currently have, but rather than remember this
        // as our SCR, we add it to an array to be deleted once we're up.
        else if (pSCR->Version == iestSCR_VERSION_1)
        {
            if (thisTimestamp > ismEngine_serverGlobal.ServerTimestamp)
            {
                ismEngine_serverGlobal.ServerTimestamp = thisTimestamp;
                ismEngine_serverGlobal.ServerShutdownTimestamp = thisTimestamp;
            }

            // Allocate space for this store handle
            if (deleteSCRCount == deleteSCRCapacity)
            {
                ismStore_Handle_t *newDeleteSCRs = iemem_realloc(pThreadData,
                                                                 IEMEM_PROBE(iemem_restoreTable, 6),
                                                                 deleteSCRs,
                                                                 (deleteSCRCapacity+100) * sizeof(ismStore_Handle_t));

                if (newDeleteSCRs == NULL)
                {
                    rc = ISMRC_AllocateError;

                    if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
                    {
                        //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
                        ism_admin_setMaintenanceMode(rc, 0);
                    }

                    ieutTRACE_FFDC(ieutPROBE_004, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                                   "Unable to re-allocate store handle array", rc,
                                   "SCR", pSCR, record->pFragsLengths[0],
                                   "Record", record, sizeof(ismStore_Record_t),
                                   "Delete SCRs", &deleteSCRs, sizeof(ismStore_Handle_t *),
                                   "Delete SCR Count", &deleteSCRCount, sizeof(uint32_t),
                                   "Delete SCR Capacity", &deleteSCRCapacity, sizeof(uint32_t),
                                   NULL);
                    ism_common_setError(rc);
                    goto mod_exit;
                }

                deleteSCRs = newDeleteSCRs;
                deleteSCRCapacity += 100;
            }

            // Add this handle to the array to of handles to be deleted when the store is open
            deleteSCRs[deleteSCRCount++] = recHandle;
        }
        // Future version we cannot tolerate...
        else
        {
            rc = ISMRC_InvalidValue;
            ism_common_setErrorData(rc, "%u", pSCR->Version);
            goto mod_exit;
        }
    }
    else
    {
        rc = ISMRC_Error;
        ieutTRACE_FFDC(ieutPROBE_005, abortOnFirstRecoveryFailure,
                       "Duplicate Server Record", rc,
                       "SCR", pSCR, record->pFragsLengths[0],
                       "Record", record, sizeof(ismStore_Record_t),
                       "New Handle", &recHandle, sizeof(ismStore_Handle_t),
                       "Original Handle", &ismEngine_serverGlobal.hStoreSCR, sizeof(ismStore_Handle_t),
                       "Restart Tran Data", transdata, sizeof(ismEngine_RestartTransactionData_t),
                       NULL);

        ism_common_setError(rc);
    }

mod_exit:

    if (rc != OK)
    {
        ierr_recordBadStoreRecord( pThreadData
                                 , record->Type
                                 , recHandle
                                 , NULL
                                 , rc);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//*************************************************************************
/// @brief Complete the recovery of SCRs from the store
//*************************************************************************
int32_t ierr_completeServerRecordRecovery(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;
    ieutTRACEL(pThreadData, ismEngine_serverGlobal.hStoreSCR, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Create a server definition in the Store as the initial use of the Store
    // Note: We also remove version 1 SCRs if any were found during restart
    if (ismEngine_serverGlobal.hStoreSCR == ismSTORE_NULL_HANDLE)
    {
        ismStore_StreamHandle_t hStoreStream;

        rc = ism_store_openStream(&hStoreStream, 0);
        if (rc == OK)
        {
            ismStore_Record_t storeRecord;
            ismStore_Handle_t hStoreSCR;
            iestServerConfigRecord_t serverCfgRec;
            uint32_t dataLength;
            uint32_t fragsCount = 1;
            char *pFrags[1];
            uint32_t fragsLength[1];
            uint32_t useTimestamp = ismEngine_serverGlobal.ServerTimestamp;

            // If we didn't pick up a timestamp during restart, use now.
            if (useTimestamp == 0) useTimestamp = ism_common_nowExpire();

            dataLength = sizeof(serverCfgRec);
            fragsCount = 1;
            pFrags[0] = (char *)&serverCfgRec;
            fragsLength[0] = sizeof(serverCfgRec);

            memcpy(serverCfgRec.StrucId, iestSERVER_CONFIG_RECORD_STRUCID, 4);
            serverCfgRec.Version = iestSCR_CURRENT_VERSION;

            storeRecord.Type = ISM_STORE_RECTYPE_SERVER;
            storeRecord.FragsCount = fragsCount;
            storeRecord.pFrags = pFrags;
            storeRecord.pFragsLengths = fragsLength;
            storeRecord.DataLength = dataLength;
            storeRecord.Attribute = 0;
            storeRecord.State = (uint64_t)useTimestamp << 32;

            // Ensure we can fit all the updates on the stream.
            ismStore_Reservation_t Reservation = {dataLength, deleteSCRCount + 1, 0};

            rc = ism_store_reserveStreamResources(hStoreStream, &Reservation);

            if (rc != OK)
            {
                ieutTRACEL(pThreadData, Reservation.RecordsCount, ENGINE_WORRYING_TRACE,
                           "Failed to reserve stream resources. rc=%d\n", rc);
                ism_common_setError(rc);
            }
            else
            {
                uint32_t storeOperations = 0;

                // Delete version 1 SCRs
                for(int32_t i=0; i<deleteSCRCount; i++)
                {
                    rc = ism_store_deleteRecord(hStoreStream, deleteSCRs[i]);

                    if (rc != OK)
                    {
                        ieutTRACEL(pThreadData, deleteSCRs[i], ENGINE_WORRYING_TRACE,
                                   "Failed to delete SCR 0x%lx. rc=%d\n", deleteSCRs[i], rc);
                        ism_common_setError(rc);
                        break;
                    }

                    storeOperations += 1;
                }

                if (rc == OK)
                {
                    rc = ism_store_createRecord(hStoreStream,
                                                &storeRecord,
                                                &hStoreSCR);

                    if (rc == OK) storeOperations += 1;
                }

                // We did something in the store, we need to either commit or roll back.
                if (storeOperations != 0)
                {
                    if (rc == OK)
                    {
                        rc = ism_store_commit(hStoreStream);

                        if (rc == OK)
                        {
                            ismEngine_serverGlobal.hStoreSCR = hStoreSCR;
                            ismEngine_serverGlobal.ServerTimestamp = useTimestamp;
                            assert(ismEngine_serverGlobal.ServerShutdownTimestamp != 0);
                        }
                    }
                    else
                    {
                        (void)ism_store_rollback(hStoreStream);
                    }
                }
            }

            if (deleteSCRs != NULL)
            {
                iemem_free(pThreadData, iemem_restoreTable, deleteSCRs);
                deleteSCRs = NULL;
                deleteSCRCount = deleteSCRCapacity = 0;
            }

            (void)ism_store_closeStream(hStoreStream);
        }

        if (UNLIKELY(rc != OK))
        {
            if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
            {
                //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
                ism_admin_setMaintenanceMode(rc, 0);
            }

            ieutTRACE_FFDC(ieutPROBE_011, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                           "Updating / Creating server definition failed", rc,
                           "pThreadData", pThreadData, sizeof(*pThreadData),
                           NULL);
        }
    }
    else
    {
        assert((deleteSCRCount == 0) && (deleteSCRs == NULL));
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int32_t ierr_rehydrateClientStateRecord(ieutThreadData_t *pThreadData,
                                        ismStore_Handle_t recHandle,
                                        ismStore_Record_t *record,
                                        ismEngine_RestartTransactionData_t *transData,
                                        void **outData,
                                        void *pContext)
{
    ismEngine_ClientState_t *pClient = NULL;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, recHandle, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    rc = iecs_rehydrateClientStateRecord(pThreadData,
                                         record,
                                         recHandle,
                                         &pClient);
    if (rc == OK)
    {
        *outData = pClient;
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int32_t ierr_rehydrateMessageDeliveryReference(
        ieutThreadData_t *pThreadData,
        void *owner,
        void *child,
        ismStore_Handle_t refHandle,
        ismStore_Reference_t *reference,
        ismEngine_RestartTransactionData_t *transData,
        void *pContext)
{
    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)owner;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, refHandle, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    rc = iecs_rehydrateMessageDeliveryReference(pThreadData,
                                                pClient,
                                                refHandle,
                                                reference);

    if (rc != OK)
    {
        if (OK == firstRecoveryRC) firstRecoveryRC = rc;

        ieutTRACEL(pThreadData, rc,  ENGINE_ERROR_TRACE, "Continuing after rc=%d rehydrating MDR\n", rc);
        rc = OK;
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int32_t ierr_setMessageDeliveryIdFromMDR(
        ieutThreadData_t *pThreadData,
        iecsMessageDeliveryInfoHandle_t msgDelStateInfo,
        ismStore_Handle_t ownerHandle,
        ismQHandle_t *pQHdl,
        void **ppNode,
        ismStore_RecordType_t ownerType,
        ismStore_Handle_t messageHandle,
        uint32_t deliveryId)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, ownerHandle, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "(msgDelStateInfo %p, ownerHandle 0x%lx, ownerType %d, messageHandle 0x%lx, deliveryId %u)\n",
               __func__, msgDelStateInfo, ownerHandle, ownerType, messageHandle, deliveryId);

    if (ownerType == ISM_STORE_RECTYPE_SUBSC)
    {
        ismQHandle_t pQueue;
        ismQueueType_t queueType = (ismQueueType_t)0;

        // First get the handle of the queue underlying the subscription
        pQueue = iert_getTableEntry(recordTable[ownerType],
                                    ownerHandle);

        // No queue found. So why do we have an MDR that points at it?
        if (pQueue == NULL)
        {
            rc = ISMRC_NotFound;
        }
        // The queue found is a simple Q. MDRs aren't written for simple queues.
        else if ((queueType = ieq_getQType(pQueue)) == simple)
        {
            rc = ISMRC_InvalidOperation;
        }
        else
        {
            rc = ieq_rehydrateDeliveryId(pThreadData, pQueue, msgDelStateInfo, messageHandle, deliveryId, ppNode);

            if (rc == OK)
            {
                // If we got a node pointer set, we need to also record the queue handle
                if (*ppNode != NULL)
                {
                    *pQHdl = pQueue;
                }
                else
                {
                    *pQHdl = NULL;
                }
            }
            // Message reference not found. Why does this MDR have it then?
            else
            {
                assert(rc == ISMRC_NoMsgAvail);
                assert(*ppNode == NULL);
                *pQHdl = NULL;
            }
        }

        if (rc != OK)
        {
            ism_common_setError(rc);

            // If the Queue that the MDR refers to seems to be missing or is the wrong type,
            // or the rehydrate function returned an error, take an FFDC and return the error.
            ieutTRACE_FFDC(ieutPROBE_003, abortOnFirstRecoveryFailure,
                           "Unexpected failure setting message deliveryId from MDR.", rc,
                           "SDR handle", &ownerHandle, sizeof(ownerHandle),
                           "MR handle", &messageHandle, sizeof(messageHandle),
                           "pQueue", &pQueue, sizeof(pQueue),
                           "queueType", &queueType, sizeof(queueType),
                           "deliveryId", &deliveryId, sizeof(deliveryId),
                           NULL);
        }
    }
    else
    {
        assert(false);
        rc = ISMRC_NotImplemented;
        ism_common_setError(rc);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int32_t ierr_rehydrateClientStatePair(ieutThreadData_t *pThreadData,
        ismStore_Handle_t recHandle,
        ismStore_Record_t *record,
        ismEngine_RestartTransactionData_t *transData,
        void *requestingRecord,
        void **rehydratedRecord,
        void *pContext)
{
    ismEngine_ClientState_t *pClient = NULL;
    void *pMessage = NULL;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, recHandle, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // The attribute of a CPR is a cheeky reference to the MR for the will-message
    if (record->Attribute != ismSTORE_NULL_HANDLE)
    {
        pMessage = iert_getTableEntry(recordTable[ISM_STORE_RECTYPE_MSG],
                                      record->Attribute);

        if (pMessage == NULL)
        {
            //Hmm we may not have read this message in yet.... go and get it now
            rc = ierr_recoverRecordFromHandle( pThreadData
                                             , record->Attribute
                                             , ISM_STORE_RECTYPE_MSG
                                             , iest_rehydrateMessage
                                             , &pMessage );

            // If the will-message seems to be missing, take an FFDC but continue. The code to
            // publish will messages and unstore client-states is tolerant of this
            if (rc != OK)
            {
                ieutTRACE_FFDC(ieutPROBE_001, abortOnFirstRecoveryFailure,
                               "Missing will-message.", rc,
                               "MR handle", &record->Attribute, sizeof(record->Attribute),
                               "CPR handle", &recHandle, sizeof(recHandle),
                               NULL );
            }
        }
    }

    rc = iecs_rehydrateClientPropertiesRecord(pThreadData,
                                              record,
                                              recHandle,
                                              (ismEngine_ClientState_t *)requestingRecord,
                                              (ismEngine_MessageHandle_t)pMessage,
                                              &pClient);

    if (rc == OK)
    {
        *rehydratedRecord = pClient;
    }
    else
    {
        ierr_recordBadStoreRecord( pThreadData
                                 , record->Type
                                 , recHandle
                                 , NULL
                                 , rc);

        // Need to discard this client if we decide to do partial store recovery
        ((ismEngine_ClientState_t *)requestingRecord)->fDiscardDurable = true;
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int32_t ierr_rehydrateUnreleasedMessageStates(ieutThreadData_t *pThreadData,
        void *rehydratedClient,
        ismStore_Handle_t stateHandle,
        ismStore_StateObject_t *pState,
        ismEngine_RestartTransactionData_t *transData)
{
    ismEngine_Transaction_t *pTran = NULL;
    ietrTranEntryType_t type = 0;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, stateHandle, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "statehandle=0x%lu value=%u\n",
               __func__, stateHandle, pState->Value);

    if (transData != NULL)
    {
        pTran = transData->pTran;
        type = transData->operationReference.Value;
    }

    rc = iecs_rehydrateUnreleasedMessageState(pThreadData,
                                              (ismEngine_ClientState_t *)rehydratedClient,
                                              pTran,
                                              type,
                                              pState->Value,
                                              stateHandle,
                                              transData);

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///The prototype to this function is fixed being an ierr_RehydrateRefFn_t function
///but a couple of the parameters are not appropriate:
///
///@remark Usually, when rehydrate reference functions are called owner and child point to rehydrated objects
///In the special case of transactional operation references, the child will not have been read
///in yet and is NULL
///
///@remark As this function is reading the transaction data, transData will be NULL
int32_t ierr_rehydrateTransactionReference(ieutThreadData_t *pThreadData,
        void        *owner,
        void        *child,
        ismStore_Handle_t tranrefHandle,
        ismStore_Reference_t *reference,
        ismEngine_RestartTransactionData_t  *transData,
        void        *pContext)
{
    int32_t rc = OK;

    //TODO: different memory type?
    transData = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_restoreTable, 1), sizeof(ismEngine_RestartTransactionData_t));

    if(transData != NULL)
    {

        transData->pTran = owner;
        transData->operationRefHandle = tranrefHandle;
        transData->operationReference = *reference; //Copy the reference

        int comparisonResult = 0;

        // The hRefHandle for UMS operations is a state object, which we cannot pass into
        // ism_store_compareHandles. However, we do know that these are all read at the end
        // of recovery, so we just need to add this state object handle to the transaction members
        // table for later retrieval.
        if (reference->Value == iestTOR_VALUE_ADD_UMS || reference->Value == iestTOR_VALUE_REMOVE_UMS)
        {
            assert(comparisonResult == 0);
        }
        else
        {
            // -ve if handle1 is earlier gen than handle 2... 0 if same
            rc = ism_store_compareHandles( tranrefHandle
                                         , reference->hRefHandle
                                         , &comparisonResult);
            if (rc != OK)
            {
                //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
                ism_admin_setMaintenanceMode(rc, 0);

                ieutTRACE_FFDC(ieutPROBE_001, true,
                               "Unable to compare store handles", rc,
                               "tranrefHandle", &(tranrefHandle), sizeof(tranrefHandle),
                               "reference->hRefHandle", &(reference->hRefHandle), sizeof(reference->hRefHandle),
                               NULL);
            }
        }

        //If the comparisonResult is negative/zero then we've read the tranref before we will read the child we can
        //add an entry to the transaction members table which, when we find the child will tell us to put it in
        //the transaction...
        if (comparisonResult <= 0)
        {
            //Record that when we see an item with handle == reference.hRefHandle, it's part of this transaction
            rc = iert_addTableEntry(pThreadData, &transactionMembersTable, reference->hRefHandle, transData);
        }
        else
        {
            //We've already read the child. Hmm... is this a transactional operation we expect that to be possible for?
            if(reference->Value == iestTOR_VALUE_CONSUME_MSG)
            {
                //Is the generation of the child still in memory... if so we can find out enough information about it to
                //locate the in-memory representation and adjust it to be in the transaction....
                rc =  ierr_recoverOfflineTransactionMemberData(pThreadData, transData, false);

                if (rc == OK)
                {
                    //Hurrah, we've acted on this transaction data we don't need to do any more...
                    iemem_free(pThreadData, iemem_restoreTable, transData);
                }
                else if (rc == ISMRC_WouldBlock)
                {
                    //We can't get enough info... we'll just record that we have work to do and do it after we've read
                    //all the generations once
                    rc = ierr_addOfflineTransactionMemberData(pThreadData, transData);
                }
                else
                {
                    //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
                    ism_admin_setMaintenanceMode(rc, 0);

                    ieutTRACE_FFDC(ieutPROBE_002, true,
                            "Failed trying to read data for late transaction...", rc,
                            "transData", transData, sizeof(*transData),
                            NULL);
                }
            }
            else if(reference->Value == iestTOR_VALUE_OLD_STORE_SUBSC_DEFN)
            {
                ismStore_Handle_t childHandle = reference->hRefHandle;

                ismQHandle_t hQueue = iert_getTableEntry(recordTable[ISM_STORE_RECTYPE_SUBSC],
                                                         childHandle);

                // We should have already read the subscription (it should have come from an earlier generation),
                // If we can't find it - this transaction is inconsistent. Go down in flames before we do any more
                // damage to data integrity
                if (hQueue == NULL)
                {
                    //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
                    ism_admin_setMaintenanceMode(rc, 0);

                    ieutTRACE_FFDC(ieutPROBE_003, true,
                                   "subscription in transaction, unexpectedly not found!", ISMRC_Error,
                                   "tranrefHandle", &(tranrefHandle), sizeof(tranrefHandle),
                                   "subscriptionHandle", &(reference->hRefHandle), sizeof(reference->hRefHandle),
                                   NULL);
                }
                else
                {
                    assert((transData->pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) != ietrTRAN_FLAG_GLOBAL);

                    // We've already rehydrated the sub. add an SLE to the transaction to ensure that the Store handle for
                    // the SDR is removed.
                    rc = iett_rehydrateOldSubscriptionDefnSLE(pThreadData,
                                                              transData,
                                                              hQueue,
                                                              childHandle,
                                                              transData->operationRefHandle,
                                                              transData->operationReference.OrderId);

                    // If we successfully added an SLE, mark the queue as deleted so it can be cleared up
                    // at the end of recovery (assumes this transaction will be rolled back!)
                    if (rc == OK && transData->pTran->TranState != ismTRANSACTION_STATE_COMMIT_ONLY)
                    {
                        ieq_markQDeleted(pThreadData, hQueue, false);
                    }
                }
            }
            else
            {
                //Hmmm we currently only expect to see the child before the tran ref for the types we've handled above
                //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
                ism_admin_setMaintenanceMode(rc, 0);

                ieutTRACE_FFDC(ieutPROBE_004, true,
                               "Unexpected type of reference", ISMRC_Error,
                               "reference", reference, sizeof(*reference),
                               NULL);
            }
        }
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

    return rc;
}

int32_t ierr_rehydrateBridgeQMgrRecord(ieutThreadData_t *pThreadData,
        ismStore_Handle_t recHandle,
        ismStore_Record_t *record,
        ismEngine_RestartTransactionData_t *transData,
        void **rehydratedRecord,
        void *pContext)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, recHandle, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    rc = iesm_rehydrateBridgeQMgrRecord(pThreadData, record, recHandle, rehydratedRecord);

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///@brief For a given record, read in all state records owned by that record
///
///@remark relies on transactional member data being available for the state records
///so should be called after all store generations have been read.
typedef struct tag_ierrRecoverStateRecordsCallbackContext_t
{
    ierr_RehydrateStateFn_t pStateFn;
} ierrRecoverStateRecordsCallbackContext_t;

static inline int32_t ierr_recoverStateRecordsForOwner(ieutThreadData_t *pThreadData,
                                                       uint64_t recHandle,
                                                       void *rehydratedOwner,
                                                       void *context)
{
    int32_t rc = OK;
    ismStore_IteratorHandle stateIterator = NULL;
    ierrRecoverStateRecordsCallbackContext_t *pContext = context;
    ismStore_Handle_t ownerHandle = (ismStore_Handle_t)recHandle;

    ieutTRACEL(pThreadData, recHandle,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "owner=0x%lx\n", __func__, recHandle);

    while (rc == OK)
    {
        ismStore_Handle_t stateHandle = ismSTORE_NULL_HANDLE;
        ismStore_StateObject_t state = {0};
        ismEngine_RestartTransactionData_t *transData = NULL;

        rc = ism_store_getNextStateForOwner( &stateIterator,
                                             ownerHandle,
                                             &stateHandle,
                                             &state);
        if (rc == ISMRC_StoreNoMoreEntries)
        {
            rc = OK;
            break;
        }

        if (rc == OK)
        {
            //Try and find any information about a transaction this state record is involved in, if any...
            //(Needed before we try and reconstruct it)
            transData = iert_getTableEntry(transactionMembersTable, stateHandle);
        }

        if ( rc == OK)
        {
            rc = pContext->pStateFn( pThreadData,
                                     rehydratedOwner,
                                     stateHandle,
                                     &state,
                                     transData);
        }

        if (transData != NULL)
        {
            //We've found the entry that this transactional data is referring to, so we no longer need it
            int32_t rc2 = iert_removeTableEntry(pThreadData,
                                                transactionMembersTable,
                                                stateHandle);
            if (rc == OK) rc = rc2;

            iemem_free(pThreadData, iemem_restoreTable, transData);
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///@brief Iterates through all the records of a given type reading their state records
///
///@param[in] ownerRecType   Type of records to look for state record children on behalf od
///@param[in] pStateFn       Function to rehydrate each state record
static inline int32_t ierr_recoverStateRecords(ieutThreadData_t *pThreadData,
                                               ismStore_RecordType_t ownerRecType,
                                               ierr_RehydrateStateFn_t pStateFn)
{
    int32_t rc = OK;

    ierrRecoverStateRecordsCallbackContext_t context;

    context.pStateFn = pStateFn;

    //We shouldn't be called for record types we aren't tracking.
    assert(recordTable[ownerRecType] != NULL);

    rc = iert_iterateOverTable( pThreadData
                              , recordTable[ownerRecType]
                              , ierr_recoverStateRecordsForOwner
                              , &context );

    return rc;
}

//*************************************************************************
/// @brief Callback function used when recovering requested records
///
/// @param[in] curGenId             The current generation in the store
/// @param[in] currOperation        The operation being performed
//*************************************************************************
typedef struct tag_ierrRecoverRequestedRecordCallbackContext_t
{
    ismStore_GenId_t        curGenId;
    ierrOperationsPhase1_t *currOperation;
    char                   *buffer;
    uint32_t                bufferSize;
} ierrRecoverRequestedRecordsCallbackContext_t;

int32_t ierr_recoverRequestedRecordCallback(ieutThreadData_t *pThreadData,
                                            uint64_t key,
                                            void *value,
                                            void *context)
{
    int32_t rc = OK;

    ierrRecoverRequestedRecordsCallbackContext_t *pContext = (ierrRecoverRequestedRecordsCallbackContext_t *)context;

    ismStore_Handle_t recHandle = (ismStore_Handle_t)key;
    ismStore_GenId_t keyGenId = 0;

    rc = ism_store_getGenIdOfHandle(recHandle, &keyGenId);

    // This one should be in the current generation
    if (rc == OK && (pContext->curGenId == keyGenId))
    {
        ismStore_RecordType_t recType = pContext->currOperation->primaryType;
        ismStore_Record_t record = {0};

        char *pFrags[1] = {0};
        uint32_t pFragsLengths[1] = {0};

        record.FragsCount = 1;
        record.pFrags = pFrags;
        record.pFragsLengths = pFragsLengths;

        do
        {
            pFrags[0] = pContext->buffer;
            pFragsLengths[0] = pContext->bufferSize;
            record.DataLength = record.pFragsLengths[0];

            rc = ism_store_readRecord(recHandle, &record, true);

            if (rc == OK)
            {
                ismEngine_RestartTransactionData_t *transData = iert_getTableEntry(transactionMembersTable, key);

                void *rehydratedRecord = NULL;

                rc = pContext->currOperation->pRecPairFn(pThreadData,
                                                         recHandle,
                                                         &record,
                                                         transData,
                                                         value,
                                                         &rehydratedRecord,
                                                         pContext->currOperation->pContext);

                //If this is a record type we build a table for, add it to the table
                if (rc == OK)
                {
                    rc = ierr_recordRehydratedRecord(pThreadData,
                                                     recType,
                                                     recHandle,
                                                     rehydratedRecord);
                }

                if (rc == OK)
                {
                    //OK.... we've dealt with this pair...remove the "waiting for partner" record
                    rc = iert_removeTableEntry(pThreadData,
                                               pairRequesterData[recType],
                                               key);
                }

                if (transData != NULL)
                {
                    int32_t rc2 = iert_removeTableEntry(pThreadData,
                                                        transactionMembersTable,
                                                        key);
                    if (rc == OK) rc = rc2;
                    iemem_free(pThreadData, iemem_restoreTable, transData);
                }

                break;
            }
            else if (rc == ISMRC_StoreBufferTooSmall)
            {
                record.pFrags[0] = iemem_realloc(pThreadData,
                                                 IEMEM_PROBE(iemem_restoreTable, 7),
                                                 pContext->buffer,
                                                 record.DataLength);

                if (record.pFrags[0] == NULL)
                {
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                }
                else
                {
                    pContext->buffer = record.pFrags[0];
                    pContext->bufferSize = record.DataLength;
                }
            }
            else
            {
                if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
                {
                    //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
                    ism_admin_setMaintenanceMode(rc, 0);
                }

                ieutTRACE_FFDC(ieutPROBE_001, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                               "Unexpectedly couldn't read record", rc,
                               "Record Type", &(recType), sizeof(recType),
                               "Record Handle", &(recHandle), sizeof(recHandle),
                               NULL);
            }
        }
        while(rc == ISMRC_StoreBufferTooSmall);
    }

    if (rc != OK)
    {
        //Oh dear - remember the problem so we can choose whether to abort recovery or remove offending records
        //at the end of recovery
        if (OK == firstRecoveryRC) firstRecoveryRC = rc;

        ieutTRACEL(pThreadData, rc,  ENGINE_ERROR_TRACE, "Continuing after rc=%d rehydrating record\n", rc);
        rc = OK;
    }
    return rc;
}

static inline void ierr_addCorruptRecord(
          ieutThreadData_t *pThreadData
        , ismStore_RecordType_t recType
        , ismStore_Handle_t recHandle
        , ismStore_Record_t *pRecord )
{
    int32_t rc;

    if (corruptRecords == NULL)
    {
        rc = iert_createTable( pThreadData
                             , &corruptRecords
                             , iertHundredsOfItems
                             , false
                             , false
                             , 0
                             , 0);

        if (rc != OK)
        {
            ieutTRACE_FFDC(ieutPROBE_001, abortOnFirstRecoveryFailure,
                            "Unable to create table for corrupt records", rc,
                           NULL);
        }
    }

    if (corruptRecords != NULL)
    {
        rc = iert_addTableEntry( pThreadData
                               , &corruptRecords
                               , recHandle
                               , (void *)recType);

        if (rc != OK)
        {
            ieutTRACE_FFDC(ieutPROBE_002, abortOnFirstRecoveryFailure,
                            "Unable to add record to corruptTrecords table", rc,
                            "recHandle",&recHandle, sizeof(recHandle),
                           NULL);
        }
    }
}

static const char *ierr_getRecordTypeString(ismStore_RecordType_t recType)
{
    const char *recTypeString;

    switch (recType)
    {
        case ISM_STORE_RECTYPE_SERVER:
            recTypeString = "Server";
            break;
        case ISM_STORE_RECTYPE_CLIENT:
            recTypeString = "Client";
            break;
        case ISM_STORE_RECTYPE_QUEUE:
            recTypeString = "Queue";
            break;
        case ISM_STORE_RECTYPE_TOPIC:
            recTypeString = "Topic";
            break;
        case ISM_STORE_RECTYPE_SUBSC:
            recTypeString = "Subscription";
            break;
        case ISM_STORE_RECTYPE_TRANS:
            recTypeString = "Transaction";
            break;
        case ISM_STORE_RECTYPE_BMGR:
            recTypeString = "BridgeQMgr";
            break;
        case ISM_STORE_RECTYPE_REMSRV:
            recTypeString = "RemoteServer";
            break;
        case ISM_STORE_RECTYPE_MSG:
            recTypeString = "Message";
            break;
        case ISM_STORE_RECTYPE_PROP:
            recTypeString = "Transaction";
            break;
        case ISM_STORE_RECTYPE_CPROP:
            recTypeString = "ClientProps";
            break;
        case ISM_STORE_RECTYPE_QPROP:
            recTypeString = "QueueProps";
            break;
        case ISM_STORE_RECTYPE_TPROP:
            recTypeString = "TopicProps";
            break;
        case ISM_STORE_RECTYPE_SPROP:
            recTypeString = "SubProps";
            break;
        case ISM_STORE_RECTYPE_BXR:
            recTypeString = "BridgeXID";
            break;
        case ISM_STORE_RECTYPE_RPROP:
            recTypeString = "RemoteServerProps";
            break;
        default:
            recTypeString = "Unknown";
            break;

    }
    return recTypeString;
}

void ierr_recordBadStoreRecord( ieutThreadData_t      *pThreadData
                              , ismStore_RecordType_t recType
                              , ismStore_Handle_t     recHandle
                              , char                  *identifier
                              , int32_t               rc)
{
    char messageBuffer[256];

    LOG(ERROR, Messaging, 3011, "%lx%s%s%d%s%d", "A record with handle 0x{0} (identifier: {1} type {2} ({3})) could not be recovered for reason {4} ({5})",
                 recHandle,
                 (identifier != NULL) ? identifier: "???",
                 ierr_getRecordTypeString(recType),
                 recType,
                 ism_common_getErrorStringByLocale(rc, ism_common_getLocale(), messageBuffer, 255),
                 rc);
}

///@brief for a given store record, look for references that it owns
///
///@param[in] recHandle     handle of the owner
///@param[in] ownerObject   Rehydrated owner object
///@param[in] context       Pointer to give to the reference rehydration function
int32_t ierr_getReferencesForOwner(ieutThreadData_t *pThreadData,
                                   uint64_t  recHandle,
                                   void     *ownerObject,
                                   void     *context)
{
    ierrReferenceRecoveryContext_t *recoveryContext = (ierrReferenceRecoveryContext_t *)context;
    int32_t rc = OK;
    ismStore_IteratorHandle refIterator = NULL;
    ismStore_Handle_t ownerHandle = (ismStore_Handle_t)recHandle;

    ismStore_Handle_t refHandle = ismSTORE_NULL_HANDLE;
    ismStore_Reference_t reference = {0};

    ieutTRACEL(pThreadData, ownerHandle, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "ownerHandle=0x%lx\n", __func__,ownerHandle);

    while (rc == OK)
    {
        rc = ism_store_getNextReferenceForOwner(&refIterator,
                                                ownerHandle,
                                                recoveryContext->genId,
                                                &refHandle,
                                                &reference);
        if (rc == ISMRC_StoreNoMoreEntries)
        {
            rc = OK;
            break;
        }
        assert(rc == OK);

        if (rc == OK)
        {
            ismEngine_RestartTransactionData_t *transData = NULL;
            void *childObject = NULL;

            if (  (recoveryContext->ownerRecType != ISM_STORE_RECTYPE_CLIENT)
                &&(recoveryContext->ownerRecType != ISM_STORE_RECTYPE_TRANS)  )
            {
                transData = iert_getTableEntry(transactionMembersTable, refHandle);
            }

            //If we have been told a child rectype it means it has already been read and we should look it up
            if ((rc == OK) && (recoveryContext->childRecType != 0))
            {
                childObject = iert_getTableEntry(recordTable[recoveryContext->childRecType],
                                                 reference.hRefHandle);

                if (childObject == NULL)
                {
                    if (recoveryContext->childRecType == ISM_STORE_RECTYPE_MSG)
                    {
                        rc = ierr_recoverRecordFromHandle( pThreadData
                                                         , reference.hRefHandle
                                                         , recoveryContext->childRecType
                                                         , iest_rehydrateMessage
                                                         , &childObject );
                        assert(rc == OK);
                    }
                    else
                    {
                        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
                        {
                            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
                            ism_admin_setMaintenanceMode(rc, 0);
                        }

                        ieutTRACE_FFDC(ieutPROBE_007, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                                       "Child record for Reference not found", rc,
                                       "Parent Type", &(recoveryContext->ownerRecType), sizeof(recoveryContext->ownerRecType),
                                       "Parent Handle", &ownerHandle, sizeof(ownerHandle),
                                       "Child Type", &(recoveryContext->childRecType), sizeof(recoveryContext->childRecType),
                                       "Parent Handle", &(reference.hRefHandle), sizeof(reference.hRefHandle),
                                       NULL);
                    }
                }
            }

            if (rc == OK)
            {
                ieutTRACEL(pThreadData, refHandle, ENGINE_NORMAL_TRACE, "Read reference child type(%d) handle(0x%lx), owner type(%d) handle(0x%lx)\n",
                           recoveryContext->childRecType,
                           refHandle,
                           recoveryContext->ownerRecType,
                           recHandle);

                //Restore the link between parent and child
                rc = recoveryContext->pRefFn(pThreadData,
                                             ownerObject,
                                             childObject,
                                             refHandle,
                                             &reference,
                                             transData,
                                             recoveryContext->pRehydrationContext);
                assert(rc == OK);
            }


            if (transData != NULL)
            {
                //We've found the entry that this transactional data is referring to, so we no longer need it
                int32_t rc2 = iert_removeTableEntry(pThreadData,
                                                    transactionMembersTable,
                                                    refHandle);
                if (rc == OK) rc = rc2;
                iemem_free(pThreadData, iemem_restoreTable, transData);
            }
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///@brief complete the creation of a queue
int32_t ierr_completeQueueRehydration( ieutThreadData_t *pThreadData
                                     , uint64_t queueHandle
                                     , void *rehydratedQueue
                                     , void *pContext)
{
    ieutTRACEL(pThreadData, queueHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);
    int32_t rc = OK;

    ismQHandle_t Qhdl = (ismQHandle_t)rehydratedQueue;
    rc = ieq_completeRehydrate(pThreadData, Qhdl);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///@brief complete the creation of a subscription queue
int32_t ierr_completeSubscRehydration( ieutThreadData_t *pThreadData
                                     , uint64_t subHandle
                                     , void *rehydratedSub
                                     , void *pContext)
{
    ieutTRACEL(pThreadData, subHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);
    int32_t rc = OK;

    ismQHandle_t Qhdl = (ismQHandle_t)rehydratedSub;

    rc = ieq_completeRehydrate(pThreadData, Qhdl);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///@brief remove a queue/sub queue post rehydration if it's not required any more
int32_t ierr_removeUnneededQueue( ieutThreadData_t *pThreadData
                                , uint64_t storeHandle
                                , void *rehydratedQueue
                                , void *pContext)
{
    int32_t rc = OK;
    ismQHandle_t Qhdl = (ismQHandle_t)rehydratedQueue;
    ieq_removeIfUnneeded(pThreadData, Qhdl);
    return rc;
}

///@brief complete the creation of a remote server
int32_t ierr_completeRemSrvRehydration( ieutThreadData_t *pThreadData
                                      , uint64_t remSrvHandle
                                      , void *rehydratedRemSrv
                                      , void *pContext)
{
    ieutTRACEL(pThreadData, remSrvHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "rehydratedRemSrv=%p\n",
               __func__, rehydratedRemSrv);
    int32_t rc = OK;

    assert(rehydratedRemSrv != NULL);

    // The object rehydrated will either be a queue or a local server information
    // record - what we do depends on which this is.
    ismEngine_Queue_t *pQueue = (ismEngine_Queue_t *)(rehydratedRemSrv);

    // This is a queue - so perform queue rehydration completion
    if (ismEngine_CompareStructId(pQueue->StrucId, ismENGINE_QUEUE_STRUCID) == true)
    {
        rc = ieq_completeRehydrate(pThreadData, pQueue);
    }
    else
    {
        // sanity check that this is a local remote server record
        ismEngine_CheckStructId(pQueue->StrucId, "ELSI", ieutPROBE_001);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///@brief remove a remote server queue post rehydration if it's not required any more
int32_t ierr_removeUnneededRemSrv( ieutThreadData_t *pThreadData
                                 , uint64_t storeHandle
                                 , void *rehydratedRemSrv
                                 , void *pContext)
{
    int32_t rc = OK;

    // Might be a queue or a local remote server record
    ismEngine_Queue_t *pQueue = (ismEngine_Queue_t *)(rehydratedRemSrv);

    // This is a queue - so perform queue rehydration completion
    if (ismEngine_CompareStructId(pQueue->StrucId, ismENGINE_QUEUE_STRUCID) == true)
    {
        ieq_removeIfUnneeded(pThreadData, pQueue);
    }
    // This is a local remote server record, so free it.
    else
    {
        iemem_free(pThreadData, iemem_remoteServers, rehydratedRemSrv);
    }

    return rc;
}

///@brief complete the creation of a subscription queue
int32_t ierr_removeCorruptedRecord( ieutThreadData_t *pThreadData
                                  , uint64_t recHandle
                                  , void *Type
                                  , void *pContext)
{
    ismStore_RecordType_t recType = (ismStore_RecordType_t)Type;

    itemsDiscarded++;

    int32_t rc = ism_store_deleteRecord(pThreadData->hStream,
                                        recHandle);

    if (rc == OK)
    {
        ieutTRACEL(pThreadData, recHandle, ENGINE_WORRYING_TRACE,
                  FUNCTION_IDENT "Removed unrecoverable record 0x%lx (type %s (%u))\n", __func__,
                  recHandle, ierr_getRecordTypeString(recType), recType);
    }
    else
    {
        ieutTRACEL(pThreadData, recHandle, ENGINE_ERROR_TRACE,
                         FUNCTION_IDENT "Error removing unrecoverable record 0x%lx (type %s (%u)). rc=%d\n", __func__,
                         recHandle, ierr_getRecordTypeString(recType), recType, rc);
        ieutTRACE_HISTORYBUF(pThreadData, rc);
    }

    iest_store_commit(pThreadData, false);

    if (partialRecoveryAllowed)
    {
        rc = OK;
    }

    return rc;
}

static int32_t ierr_handleRecoveryInconsistencies(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    if (corruptRecords != NULL)
    {
        //delete all the corrupted records
        rc = iert_iterateOverTable( pThreadData
                                  , corruptRecords
                                  , ierr_removeCorruptedRecord
                                  , NULL );

        iert_freeTable(pThreadData, &corruptRecords);
    }

    return rc;
}

//*************************************************************************
/// @brief Recovery actions performed after the store leaves recovery mode
//*************************************************************************
int32_t ierr_completeRecovery(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    /* Complete the creation of the queues */
    rc = iert_iterateOverTable( pThreadData
                              , recordTable[ISM_STORE_RECTYPE_QUEUE]
                              , ierr_completeQueueRehydration
                              , NULL );

    /* Complete the creation of subscriptions */
    if (rc == OK)
    {
        rc = iert_iterateOverTable( pThreadData
                                  , recordTable[ISM_STORE_RECTYPE_SUBSC]
                                  , ierr_completeSubscRehydration
                                  , NULL );
    }

    /* Remove any messages on queues that were marked deleted on rehydration... */
    if (rc == OK)
    {
        rc = ieq_removeRehydratedConsumedNodes(pThreadData);
    }

    /* Update any subscriptions that have just been migrated and need an update */
    if (rc == OK)
    {
        rc = iett_updateMigratedSubscriptions(pThreadData);
    }

    /* Reconcile the set of admin subscriptions with the configuration */
    if (rc == OK)
    {
        rc = iett_reconcileAdminSharedSubscriptions(pThreadData);
    }

    /* Complete initialization of cluster requested topics */
    if (rc == OK)
    {
        // Note: This must happen before iett_reconcileEngineClusterTopics
        //       which will advertise the topics to the cluster.
        rc = iett_reconcileClusterRequestedTopics(pThreadData);
    }

    /* Reconcile the set of recovered subscriptions and topics with the cluster */
    if (rc == OK)
    {
        // Note: This must happen before iers_reconcileEngineRemoteServers which
        //       declares recovery complete
        rc = iett_reconcileEngineClusterTopics(pThreadData);
    }

    /* Reconcile the set of recovered retained message origin servers with the cluster */
    if (rc == OK)
    {
        // Note: This must happen before iers_reconcileEngineRemoteServers which
        //       declares recovery complete
        rc = iett_reconcileEngineRetainedOriginServers(pThreadData);
    }

    /* Complete the creation of remote servers */
    if (rc == OK)
    {
        rc = iert_iterateOverTable( pThreadData
                                  , recordTable[ISM_STORE_RECTYPE_REMSRV]
                                  , ierr_completeRemSrvRehydration
                                  , NULL );
    }

    /* Complete the initialization of remote servers */
    if (rc == OK)
    {
        rc = iers_reconcileEngineRemoteServers(pThreadData);
    }

    /* Complete the initialization of the topic tree */
    if (rc == OK)
    {
        rc = iett_reconcileEngineTopicTree(pThreadData);
    }

    /* Complete the creation of topics */
    if (rc == OK)
    {
        rc = iert_iterateOverTable( pThreadData
                                  , recordTable[ISM_STORE_RECTYPE_TOPIC]
                                  , iett_completeTopicRehydration
                                  , NULL );
    }

    /* Complete initialization of topic monitors */
    if (rc == OK)
    {
        rc = iett_reconcileEngineTopicMonitors(pThreadData);
    }

    /* Complete initialization of transactions */
    if (rc == OK)
    {
        rc = iert_iterateOverTable( pThreadData
                                  , recordTable[ISM_STORE_RECTYPE_TRANS]
                                  , ietr_completeRehydration
                                  , NULL );
    }

    /* Remove any unneeded remote servers */
    if (rc == OK)
    {
        rc = iert_iterateOverTable( pThreadData
                                  , recordTable[ISM_STORE_RECTYPE_REMSRV]
                                  , ierr_removeUnneededRemSrv
                                  , NULL );
    }

    /* Tell the cluster component that they can start calling us now */
    if (rc == OK)
    {
        rc = iers_declareRecoveryCompleted(pThreadData);
    }

    /**************************************************************************/
    /* There is more to complete, but it needs to be delayed until the server */
    /* is ready to start messaging - these final recovery steps can be found  */
    /* in ierr_startMessaging.                                                */
    /**************************************************************************/

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//*************************************************************************
/// @brief Iterate over a table of requested records using a callback to
///        read them
///
/// @param[in] curGenId             The current generation in the store
/// @param[in] currOperation        The operation being performed
//*************************************************************************
static inline int32_t ierr_recoverRequestedRecords(ieutThreadData_t *pThreadData,
                                                   ismStore_GenId_t curGenId,
                                                   ierrOperationsPhase1_t *currOperation)
{
    int32_t rc;

    ieutTRACEL(pThreadData, currOperation, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    ierrRecoverRequestedRecordsCallbackContext_t context;

    context.curGenId = curGenId;
    context.currOperation = currOperation;
    context.buffer = NULL;
    context.bufferSize = 0;

    rc = iert_iterateOverTable(pThreadData,
                               pairRequesterData[currOperation->primaryType],
                               ierr_recoverRequestedRecordCallback,
                               &context);

    if (context.buffer != NULL) iemem_free(pThreadData, iemem_restoreTable, context.buffer);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//*************************************************************************
/// @brief Reads all the record of a given type for one generation in the store.
///
/// @param[in] curGenId             The current generation in the store
/// @param[in] recType              The rectype to read
/// @param[in] pCallback            The function to call to rehydrate each record recovered
/// @param[in] pRehydrationContext  Ptr passed to the callback for it to maintain status
///
/// @remark Each record recovered is given to the callback function. The callback function is
/// free to steal memory allocated to the record but it must leave the record suitable
/// for future reading (so if it steal the memory it could set the first frag to be NULL and
/// the length to be 0 or make it point to a small buffer)
//*************************************************************************
static inline int32_t ierr_recoverRecords(ieutThreadData_t *pThreadData,
                                          ismStore_GenId_t curGenId,
                                          ierrOperationsPhase1_t *currOperation)
{
    ismStore_IteratorHandle recIterator = NULL;
    ismStore_Handle_t recHandle;
    ismStore_Record_t record = {0};
    char *pFrags[1] = { NULL };
    uint32_t pFragsLengths[1] = { 0 };
    int32_t rc = OK;
    ismStore_RecordType_t recType = currOperation->primaryType;

    bool isRequester  = (currOperation->secondaryType != 0); //Does this type request another type?

    record.FragsCount = 1;
    record.pFrags = pFrags;
    record.pFragsLengths = pFragsLengths;

    ieutTRACEL(pThreadData, recType,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "rectype=0x%x\n", __func__, recType);
    while (rc == OK)
    {
        rc = ism_store_getNextRecordForType(&recIterator,
                                            recType,
                                            curGenId,
                                            &recHandle,
                                            &record);

        if (rc == OK)
        {
            ieutTRACEL(pThreadData, recHandle, ENGINE_HIGH_TRACE, "Read record type 0x%x, handle: 0x%lx\n", recType, recHandle);

            ismEngine_RestartTransactionData_t *transData = NULL;

            if (recType != ISM_STORE_RECTYPE_TRANS)
            {
                //Try and find any information about a transaction this record is involved in, if any...
                //(Needed before we try and reconstruct it
                transData = iert_getTableEntry(transactionMembersTable, recHandle);
            }

            if (rc == OK)
            {
                void *rehydratedRecord = NULL;

                rc = currOperation->pRecordFn(pThreadData, recHandle, &record, transData, &rehydratedRecord, currOperation->pContext);

                if (rc == OK)
                {
                    rc = ierr_recordRehydratedRecord(pThreadData,
                                                     recType,
                                                     recHandle,
                                                     rehydratedRecord);
                }

                if ( (rc == OK) && isRequester )
                {
                    //We need to say that we're waiting for our pair record...
                    uint64_t recAttribute  = record.Attribute;

                    if ( recAttribute != 0 )
                    {
                        uint64_t pairRecType = currOperation->secondaryType;
                        rc = iert_addTableEntry(pThreadData,
                                                &pairRequesterData[pairRecType],
                                                recAttribute,
                                                rehydratedRecord);
                    }
                }


                if (rc != OK)
                {
                    //Oh dear - remember the problem so we can choose whether to abort recovery or remove offending records
                    //at the end of recovery
                    if (OK == firstRecoveryRC) firstRecoveryRC = rc;

                    ierr_addCorruptRecord( pThreadData
                                         , recType
                                         , recHandle
                                         , &record );

                    ieutTRACEL(pThreadData, rc, ENGINE_ERROR_TRACE, "Continuing after rc=%d rehydrating record\n", rc);
                    rc = OK;
                }
            }

            if (transData != NULL)
            {
                //We've found the entry that this transactional data is referring to, so we no longer need it
                int32_t rc2 = iert_removeTableEntry(pThreadData,
                                                    transactionMembersTable,
                                                    recHandle);
                if (rc == OK) rc = rc2;
                iemem_free(pThreadData, iemem_restoreTable, transData);
            }
        }
        else if (rc == ISMRC_StoreNoMoreEntries)
        {
            rc = OK;
            break;
        }
        else if (rc == ISMRC_StoreBufferTooSmall)
        {
            void *pNewFrag = ism_common_realloc(ISM_MEM_PROBE(ism_memory_engine_misc,39),record.pFrags[0], record.DataLength);
            if (pNewFrag != NULL)
            {
                record.pFrags[0] = pNewFrag;
                record.pFragsLengths[0] = record.DataLength;
                rc = OK;
            }
            else
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            }
        }
    }

    if (record.pFrags[0] != NULL)
    {
        ism_common_free(ism_memory_engine_misc,record.pFrags[0]);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///@brief Recover references of a certain type from the current store generation
///
///@remark for the moment we'll iterate asking about references for all known owners ("Option 1" in the lingo of the design meetings)
///We need to compare this with asking the store which records have references in the current generation ("Option 3" in the lingo of the design meetings)
static inline int32_t ierr_recoverReferences( ieutThreadData_t *pThreadData,
                                              ismStore_GenId_t curGenId,
                                              ierrOperationsPhase1_t *currOp)
{
    ierrReferenceRecoveryContext_t recoveryContext;
    int32_t rc = OK;

    //We shouldn't be called for record types we aren't tracking.
    assert(recordTable[currOp->primaryType] != NULL);

    recoveryContext.genId = curGenId;
    recoveryContext.ownerRecType = currOp->primaryType;
    recoveryContext.childRecType = currOp->secondaryType;
    recoveryContext.pRefFn = currOp->pRefFn;
    recoveryContext.pRehydrationContext = currOp->pContext;

    rc = iert_iterateOverTable(pThreadData,
                               recordTable[recoveryContext.ownerRecType],
                               ierr_getReferencesForOwner,
                               &recoveryContext);

    return rc;
}

//*************************************************************************
/// @brief Iterate over a list of properties records using a callback to
///        read them
///
/// @param[in] curGenId             The current generation in the store
/// @param[in] currOperation        The operation being performed
///
//*************************************************************************
static inline int32_t ierr_newRecoverRequestedRecords(ieutThreadData_t *pThreadData,
                                                      ismStore_GenId_t curGenId,
                                                      ierrOperationsPhase1_t *currOperation)
{
    int32_t rc = OK;
    ismStore_IteratorHandle recIterator = NULL;
    ismStore_Handle_t ownerHandle;
    ismStore_Handle_t propHandle;
    ismStore_RecordType_t recType = currOperation->secondaryType; // Owner's type

    ieutTRACEL(pThreadData, currOperation, ENGINE_FNC_TRACE, FUNCTION_ENTRY "recType=0x%0hx\n", __func__, recType);

    ierrRecoverRequestedRecordsCallbackContext_t context;

    context.curGenId = curGenId;
    context.currOperation = currOperation;
    context.buffer = NULL;
    context.bufferSize = 0;

    while(rc == OK)
    {
        rc = ism_store_getNextPropOwner(&recIterator, recType, curGenId, &ownerHandle, (uint64_t *)&propHandle);

        if (rc == OK)
        {
            void *ownerObject = iert_getTableEntry(recordTable[recType], ownerHandle);

            // Double-check that the entry is in the pairRequesterData
            assert(iert_getTableEntry(pairRequesterData[currOperation->primaryType], propHandle) == ownerObject);

            if (ownerObject != NULL)
            {
                rc = ierr_recoverRequestedRecordCallback(pThreadData, propHandle, ownerObject, &context);
            }
            else
            {
                // TODO: Didn't find an owner for this props record... we should report that?
            }
        }
    }

    if (context.buffer != NULL) iemem_free(pThreadData, iemem_restoreTable, context.buffer);

    if (rc == ISMRC_StoreNoMoreEntries)
    {
        rc = OK;
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//*************************************************************************
/// @brief Reads all the record of a given type for one generation in the store.
///
/// @param[in] curGenId             The current generation in the store
/// @param[in] recType              The rectype to read
/// @param[in] pCallback            The function to call to rehydrate each record recovered
/// @param[in] pRehydrationContext  Ptr passed to the callback for it to maintain status
///
/// @remark Each record recovered is given to the callback function. The callback function is
/// free to steal memory allocated to the record but it must leave the record suitable
/// for future reading (so if it steal the memory it could set the first frag to be NULL and
/// the length to be 0 or make it point to a small buffer)
//*************************************************************************
static inline int32_t ierr_newRecoverRecords(ieutThreadData_t *pThreadData,
                                             ismStore_GenId_t curGenId,
                                             ierrOperationsPhase1_t *currOperation)
{
    ismStore_IteratorHandle recIterator = NULL;
    ismStore_Handle_t ownerHandle;
    ismStore_Record_t record = {0};
    char *pFrags[1] = { NULL };
    uint32_t pFragsLengths[1] = { 0 };
    int32_t rc = OK;
    ismStore_RecordType_t recType = currOperation->primaryType;

    bool isRequester  = (currOperation->secondaryType != 0); //Does this type request another type?

    record.FragsCount = 1;
    record.pFrags = pFrags;
    record.pFragsLengths = pFragsLengths;

    ieutTRACEL(pThreadData, recType,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "rectype=0x%x\n", __func__, recType);
    while (rc == OK)
    {
        rc = ism_store_getNextNewOwner(&recIterator,
                                       recType,
                                       curGenId,
                                       &ownerHandle,
                                       &record);

        if (rc == OK)
        {
            ieutTRACEL(pThreadData, ownerHandle, ENGINE_HIGH_TRACE, "Read record type 0x%x, handle: 0x%lx\n", recType, ownerHandle);

            ismEngine_RestartTransactionData_t *transData = NULL;

            if (recType != ISM_STORE_RECTYPE_TRANS)
            {
                //Try and find any information about a transaction this record is involved in, if any...
                //(Needed before we try and reconstruct it
                transData = iert_getTableEntry(transactionMembersTable, ownerHandle);
            }

            if (rc == OK)
            {
                void *rehydratedRecord = NULL;

                rc = currOperation->pRecordFn(pThreadData, ownerHandle, &record, transData, &rehydratedRecord, currOperation->pContext);

                if (rc == OK)
                {
                    rc = ierr_recordRehydratedRecord(pThreadData,
                                                     recType,
                                                     ownerHandle,
                                                     rehydratedRecord);
                }

                if ( (rc == OK) && isRequester )
                {
                    //We need to say that we're waiting for our pair record...
                    uint64_t recAttribute  = record.Attribute;

                    if ( recAttribute != 0 )
                    {
                        uint64_t pairRecType = currOperation->secondaryType;
                        rc = iert_addTableEntry(pThreadData,
                                                &pairRequesterData[pairRecType],
                                                recAttribute,
                                                rehydratedRecord);
                    }
                }


                if (rc != OK)
                {
                    //Oh dear - remember the problem so we can choose whether to abort recovery or remove offending records
                    //at the end of recovery
                    if (OK == firstRecoveryRC) firstRecoveryRC = rc;

                    ierr_addCorruptRecord( pThreadData
                                         , recType
                                         , ownerHandle
                                         , &record );

                    ieutTRACEL(pThreadData, rc,  ENGINE_ERROR_TRACE, "Continuing after rc=%d rehydrating record\n", rc);
                    rc = OK;
                }
            }

            if (transData != NULL)
            {
                //We've found the entry that this transactional data is referring to, so we no longer need it
                int32_t rc2 = iert_removeTableEntry(pThreadData,
                                                    transactionMembersTable,
                                                    ownerHandle);
                if (rc == OK) rc = rc2;
                iemem_free(pThreadData, iemem_restoreTable, transData);
            }
        }
        else if (rc == ISMRC_StoreNoMoreEntries)
        {
            rc = OK;
            break;
        }
        else if (rc == ISMRC_StoreBufferTooSmall)
        {
            void *pNewFrag = ism_common_realloc(ISM_MEM_PROBE(ism_memory_engine_misc,41),record.pFrags[0], record.DataLength);
            if (pNewFrag != NULL)
            {
                record.pFrags[0] = pNewFrag;
                record.pFragsLengths[0] = record.DataLength;
                rc = OK;
            }
            else
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            }
        }
    }

    if (record.pFrags[0] != NULL)
    {
        ism_common_free(ism_memory_engine_misc,record.pFrags[0]);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///@brief Recover references of a certain type from the current store generation
///
///@remark for the moment we'll iterate asking about references for all known owners ("Option 1" in the lingo of the design meetings)
///We need to compare this with asking the store which records have references in the current generation ("Option 3" in the lingo of the design meetings)
static inline int32_t ierr_newRecoverReferences( ieutThreadData_t *pThreadData,
                                                 ismStore_GenId_t curGenId,
                                                 ierrOperationsPhase1_t *currOp)
{
    int32_t rc = OK;
    ismStore_IteratorHandle recIterator = NULL;
    ismStore_Handle_t ownerHandle;
    ierrReferenceRecoveryContext_t recoveryContext;
    ismStore_RecordType_t recType = currOp->primaryType;

    ieutTRACEL(pThreadData, recType,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "rectype=0x%0hx\n", __func__, recType);

    //We shouldn't be called for record types we aren't tracking.
    assert(recordTable[recType] != NULL);

    recoveryContext.genId = curGenId;
    recoveryContext.ownerRecType = recType;
    recoveryContext.childRecType = currOp->secondaryType;
    recoveryContext.pRefFn = currOp->pRefFn;
    recoveryContext.pRehydrationContext = currOp->pContext;

    while(rc == OK)
    {
        rc = ism_store_getNextRefOwner(&recIterator, recType, curGenId, &ownerHandle);

        if (rc == OK)
        {
            void *ownerObject = iert_getTableEntry(recordTable[recType], ownerHandle);

            // We only expect a NULL Owner object for certain types
            if (ownerObject == NULL)
            {
                if (recType != ISM_STORE_RECTYPE_TOPIC)
                {
                    rc = ISMRC_NotFound;
                    ism_common_setErrorData(rc, "%lx", ownerHandle);
                    break;
                }
            }

            rc = ierr_getReferencesForOwner(pThreadData, ownerHandle, ownerObject, &recoveryContext);
        }
    }

    if (rc == ISMRC_StoreNoMoreEntries)
    {
        rc = OK;
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int32_t ierr_newRecoverStoreData(ieutThreadData_t *pThreadData,
                                 double startTime,
                                 ierrOperationsPhase1_t *recoveryOps,
                                 int32_t recoveryMethod)
{
    int32_t rc = OK;
    ismStore_IteratorHandle genIterator = NULL;
    ismStore_GenId_t curGenId;

    assert(recoveryMethod == ismENGINE_VALUE_USE_NEW_OWNER_AND_REF_RECOVERY ||
           recoveryMethod == ismENGINE_VALUE_USE_FULL_NEW_RECOVERY);

    ieutTRACEL(pThreadData, recoveryMethod , ENGINE_FNC_TRACE, FUNCTION_ENTRY "recoveryMethod=%d\n",
               __func__, recoveryMethod);

    while (rc == OK)
    {
        rc = ism_store_getNextGenId(&genIterator, &curGenId);

        if (rc == ISMRC_StoreNoMoreEntries)
        {
            rc = OK;
            break;
        }

        LOG(INFO, Messaging, 3001, "%u", "" IMA_PRODUCTNAME_SHORT " data recovery is in progress (recovering data generation {0}).",
            curGenId);

        if (rc != OK)
        {
            ism_common_setError(rc);
            break;
        }

        double elapsedTime;

        for(int i = 0; rc == OK && recoveryOps[i].opType != ierrP1GenerationDone; i++)
        {
            if (recoveryOps[i].opType == ierrP1RequestedRec)
            {
                // Read requested records using the new method if FULL new recovery is
                // requested, or the old method (pairRequesterData) if not.
                if (recoveryMethod == ismENGINE_VALUE_USE_FULL_NEW_RECOVERY)
                {
                    rc = ierr_newRecoverRequestedRecords(pThreadData, curGenId, &(recoveryOps[i]));
                }
                else
                {
                    rc = ierr_recoverRequestedRecords(pThreadData, curGenId, &(recoveryOps[i]));
                }

                assert(rc == OK);
            }
            else if (recoveryOps[i].opType == ierrP1Record)
            {
                if (recoveryOps[i].primaryType == ISM_STORE_RECTYPE_SERVER)
                {
                    // Read these records in the old-fashioned way
                    rc = ierr_recoverRecords(pThreadData, curGenId, &recoveryOps[i]);
                }
                else
                {
                    //Read the records
                    rc = ierr_newRecoverRecords(pThreadData, curGenId, &recoveryOps[i]);
                }

                assert(rc == OK);
            }
            else
            {
                assert(recoveryOps[i].opType == ierrP1References);

                //Read the references
                rc = ierr_newRecoverReferences(pThreadData, curGenId, &(recoveryOps[i]));
                assert(rc == OK);
            }

            elapsedTime = ism_common_readTSC()-startTime;

            ieutTRACEL(pThreadData, elapsedTime, ENGINE_HIGH_TRACE, "Recovered recoveryOp %d (type=%d) in generation %hu. Total elapsed time %.2f seconds.\n",
                       i, recoveryOps[i].opType, curGenId, elapsedTime);
        }

        elapsedTime = ism_common_readTSC()-startTime;

        ieutTRACEL(pThreadData, elapsedTime, ENGINE_INTERESTING_TRACE, "Recovered generation %hu. Total elapsed time %.2f seconds.\n", curGenId, elapsedTime);

        #if USE_REC_TIME
        //iert_displayAndResetRecTimes(recordTable[ISM_STORE_RECTYPE_MSG], "Messages");
        #endif
    }
    assert(rc == OK);

    if (rc == OK)
    {
        // Read records not associated with any generation: state records
        rc = ierr_recoverStateRecords(pThreadData,
                                      ISM_STORE_RECTYPE_CLIENT,
                                      ierr_rehydrateUnreleasedMessageStates);
        assert(rc == OK);
    }

    // And the last thing is to load any messages which weren't able to
    // to be loaded when their references were found and add any messages to transactions
    // that were rehydrated after the message was...
    if (rc == OK)
    {
        rc = ierr_loadOfflineData(pThreadData);
        assert(rc == OK);
    }

    //OK... If everything went to get the store out of recovery mode so we can do restart steps that need to update the store
    if (rc == OK)
    {
        if (   (firstRecoveryRC != OK)
            && !partialRecoveryAllowed)
        {
            rc = firstRecoveryRC;
        }
        else
        {
            rc = ism_store_recoveryCompleted();
            assert(rc == OK);
            #ifdef USEFAKE_ASYNC_COMMIT
            size_t fakeCallbackCapacity = (size_t)ism_common_getIntConfig(ismENGINE_CFGPROP_FAKE_ASYNC_CALLBACK_CAPACITY,
                                                                          ismENGINE_DEFAULT_FAKE_ASYNC_CALLBACK_CAPACITY);
            initFakeCallBacks(fakeCallbackCapacity);
            #endif
        }
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//*************************************************************************
/// @brief Recover data from store generations using existing APIs
//*************************************************************************
int32_t ierr_recoverStoreData(ieutThreadData_t *pThreadData, double startTime, ierrOperationsPhase1_t *recoveryOps)
{
    int32_t rc = OK;
    ismStore_IteratorHandle genIterator = NULL;
    ismStore_GenId_t curGenId;

    ieutTRACEL(pThreadData, NULL , ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    while (rc == OK)
    {
        rc = ism_store_getNextGenId(&genIterator, &curGenId);
        if (rc == ISMRC_StoreNoMoreEntries)
        {
            rc = OK;
            break;
        }

        LOG(INFO, Messaging, 3001, "%u", "" IMA_PRODUCTNAME_SHORT " data recovery is in progress (recovering data generation {0}).",
            curGenId);

        double elapsedTime;

        for(int i = 0; rc == OK && recoveryOps[i].opType != ierrP1GenerationDone; i++)
        {
            if (recoveryOps[i].opType == ierrP1RequestedRec)
            {
                // Read requested records
                rc = ierr_recoverRequestedRecords(pThreadData, curGenId, &(recoveryOps[i]));
                assert(rc == OK);
            }
            else if (recoveryOps[i].opType == ierrP1Record)
            {
                //Read the records
                rc = ierr_recoverRecords(pThreadData, curGenId, &recoveryOps[i]);
                assert(rc == OK);
            }
            else
            {
                assert(recoveryOps[i].opType == ierrP1References);

                //Read the references
                rc = ierr_recoverReferences(pThreadData, curGenId, &(recoveryOps[i]));
                assert(rc == OK);
            }

            elapsedTime = ism_common_readTSC()-startTime;

            ieutTRACEL(pThreadData, elapsedTime, ENGINE_HIGH_TRACE, "Recovered recoveryOp %d (type=%d) in generation %hu. Total elapsed time %.2f seconds.\n",
                       i, recoveryOps[i].opType, curGenId, elapsedTime);
        }

        elapsedTime = ism_common_readTSC()-startTime;

        ieutTRACEL(pThreadData, elapsedTime, ENGINE_INTERESTING_TRACE, "Recovered generation %hu. Total elapsed time %.2f seconds.\n", curGenId, elapsedTime);

        #if USE_REC_TIME
        //iert_displayAndResetRecTimes(recordTable[ISM_STORE_RECTYPE_MSG], "Messages");
        #endif
    }
    assert(rc == OK);

    if (rc == OK)
    {
        // Read records not associated with any generation: state records
        rc = ierr_recoverStateRecords(pThreadData,
                                      ISM_STORE_RECTYPE_CLIENT,
                                      ierr_rehydrateUnreleasedMessageStates);
        assert(rc == OK);
    }

    // And the last thing is to load any messages which weren't able to
    // to be loaded when their references were found and add any messages to transactions
    // that were rehydrated after the message was...
    if (rc == OK)
    {
        rc = ierr_loadOfflineData(pThreadData);
        assert(rc == OK);
    }

    //OK... If everything went to get the store out of recovery mode so we can do restart steps that need to update the store
    if (rc == OK)
    {
        if (   (firstRecoveryRC != OK)
            && !partialRecoveryAllowed)
        {
            rc = firstRecoveryRC;
        }
        else
        {
            rc = ism_store_recoveryCompleted();
            assert(rc == OK);
            #ifdef USEFAKE_ASYNC_COMMIT
            size_t fakeCallbackCapacity = (size_t)ism_common_getIntConfig(ismENGINE_CFGPROP_FAKE_ASYNC_CALLBACK_CAPACITY,
                                                                          ismENGINE_DEFAULT_FAKE_ASYNC_CALLBACK_CAPACITY);
            initFakeCallBacks(fakeCallbackCapacity);
            #endif
        }
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//*************************************************************************
/// @brief Recovers all the engine status from the store
//*************************************************************************
int32_t ierr_restartRecovery(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, ismEngine_serverGlobal.runPhase, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    ierrOperationsPhase1_t recoveryOps[] =
    {
        //Read the Transaction Records - transaction records+refs must be read first
        //so that as we try and reconstruct the other record types we already know whether
        //they are part of a transaction
        {ierrP1Record,       ISM_STORE_RECTYPE_TRANS,                        0, NULL,
                ietr_rehydrate, NULL, NULL},

        //Read the Transaction References
        {ierrP1References,   ISM_STORE_RECTYPE_TRANS,                        0, NULL,
                NULL, NULL, ierr_rehydrateTransactionReference},

        //Read the Server Records
        {ierrP1Record,       ISM_STORE_RECTYPE_SERVER,                       0, NULL,
                ierr_rehydrateServerRecord,  NULL, NULL},

        //Read the Client State Records
        {ierrP1Record,       ISM_STORE_RECTYPE_CLIENT,   ISM_STORE_RECTYPE_CPROP, NULL,
                ierr_rehydrateClientStateRecord, NULL, NULL },

        //Read the Client Props Records, pairing each one with a client state record.
        //This relies on the messages for this generation having already been read.
        {ierrP1RequestedRec, ISM_STORE_RECTYPE_CPROP,   ISM_STORE_RECTYPE_CLIENT, NULL,
                NULL, ierr_rehydrateClientStatePair, NULL },

        //Read the Subscription Definition Records
        {ierrP1Record,       ISM_STORE_RECTYPE_SUBSC,   ISM_STORE_RECTYPE_SPROP, NULL,
                iett_rehydrateSubscriptionDefn, NULL, NULL },

        //Read the Subscription Props Records, pairing each one with a subscription definition record
        {ierrP1RequestedRec, ISM_STORE_RECTYPE_SPROP,   ISM_STORE_RECTYPE_SUBSC, NULL,
                NULL, iett_rehydrateSubscriptionProps, NULL },

        //Read the Subscription-Message references
        {ierrP1References,   ISM_STORE_RECTYPE_SUBSC,    ISM_STORE_RECTYPE_MSG, NULL,
                NULL, NULL, ieq_rehydrateQueueMsgRef},

        //Read the Remote Server Definition Records
        {ierrP1Record,       ISM_STORE_RECTYPE_REMSRV,   ISM_STORE_RECTYPE_RPROP, NULL,
             iers_rehydrateServerDefn, NULL, NULL },

        //Read the Remote Server Props Records, pairing with a remote server definition record
        {ierrP1RequestedRec, ISM_STORE_RECTYPE_RPROP,    ISM_STORE_RECTYPE_REMSRV, NULL,
             NULL, iers_rehydrateServerProps, NULL },

        //Read the Remote Server-Message references
        {ierrP1References,   ISM_STORE_RECTYPE_REMSRV,    ISM_STORE_RECTYPE_MSG, NULL,
             NULL, NULL, ieq_rehydrateQueueMsgRef},

        //Read the Topic Definition Records
        {ierrP1Record,       ISM_STORE_RECTYPE_TOPIC,                        0, NULL,
               iett_rehydrateTopicDefn, NULL, NULL },

        //Read the Topic Retained Message references
        {ierrP1References,   ISM_STORE_RECTYPE_TOPIC,    ISM_STORE_RECTYPE_MSG, NULL,
               NULL, NULL, iett_rehydrateRetainedMsgRef},

        //Read the Queue Records
        {ierrP1Record,       ISM_STORE_RECTYPE_QUEUE,   ISM_STORE_RECTYPE_QPROP, NULL,
                ieq_rehydrateQ, NULL, NULL },

        //Read the Queue Props Records, pairing each one with a queue definition record
        {ierrP1RequestedRec, ISM_STORE_RECTYPE_QPROP,   ISM_STORE_RECTYPE_QUEUE, NULL,
                NULL, ieq_rehydrateQueueProps, NULL },

        //Read the Queue-Message references
        {ierrP1References,   ISM_STORE_RECTYPE_QUEUE,    ISM_STORE_RECTYPE_MSG, NULL,
                NULL, NULL, ieq_rehydrateQueueMsgRef },

        //Read the Message Delivery references for each client state
        {ierrP1References,   ISM_STORE_RECTYPE_CLIENT,                       0, NULL,
                NULL, NULL, ierr_rehydrateMessageDeliveryReference },

        //Read the MQ Bridge Queue-Manager Records
        {ierrP1Record,       ISM_STORE_RECTYPE_BMGR,                         0, NULL,
                ierr_rehydrateBridgeQMgrRecord, NULL, NULL },

        {ierrP1GenerationDone}
    };

    /*This reads from the static config file:
     * partialRecoveryAllowed = ism_common_getBooleanConfig(ismENGINE_CFGPROP_TOLERATERECOVERYINCONSISTENCIES,
                                                                                  ismENGINE_DEFAULT_TOLERATERECOVERYINCONSISTENCIES );*/

    ism_prop_t *aprops = ism_config_json_getObjectProperties("TolerateRecoveryInconsistencies", NULL, 1);
    if ( aprops ) {
        ism_field_t val1;
        ism_common_getProperty(aprops, "TolerateRecoveryInconsistencies", &val1);

        if (val1.type == VT_String)
        {
            if (    (strcasecmp(val1.val.s,"true") == 0)
                ||  (strcasecmp(val1.val.s,"1")    == 0) )
            {
                partialRecoveryAllowed = true;
            }
            else if (   (strcasecmp(val1.val.s,"false") != 0)
                     && (strcasecmp(val1.val.s,"0")     != 0) )
            {
                ieutTRACEL(pThreadData, val1.val.s, ENGINE_ERROR_TRACE,
                        "Unexpected value for %s = %s\n", ismENGINE_CFGPROP_TOLERATERECOVERYINCONSISTENCIES,
                        val1.val.s);
            }
        }
        else if (val1.type == VT_Boolean)
        {
            partialRecoveryAllowed = (val1.val.i != 0);
        }
        else
        {
            ieutTRACEL(pThreadData, val1.type, ENGINE_ERROR_TRACE,
                    "Unexpected type for %s = %u\n", ismENGINE_CFGPROP_TOLERATERECOVERYINCONSISTENCIES,
                    val1.type);
        }
        ism_common_freeProperties(aprops);
    }

    ieutTRACEL(pThreadData, partialRecoveryAllowed, ENGINE_INTERESTING_TRACE,
            "%s = %d\n", ismENGINE_CFGPROP_TOLERATERECOVERYINCONSISTENCIES,
            partialRecoveryAllowed);

    char *recoveryInconsistenciesEnv = getenv("IMA_TOLERATE_RECOVERY_INCONSISTENCIES");

    if (  (recoveryInconsistenciesEnv != NULL)
        &&(    (strcmp(recoveryInconsistenciesEnv,"1") == 0)
            || (strcasecmp(recoveryInconsistenciesEnv,"true") == 0)))
    {
        ieutTRACEL(pThreadData, 0, ENGINE_WORRYING_TRACE, "Tolerating recovery inconsistencies due to env var\n");
        partialRecoveryAllowed = true;
    }

    // Move into recovering
    ieut_setEngineRunPhase(EnginePhaseRecovery);

    double startTime = ism_common_readTSC() ;

    // Initialize the topic tree's recovery specific data
    rc = iett_initializeRecovery(pThreadData);

    if (rc == OK)
    {
        rc = ierr_initialiseRecordsTables(pThreadData, recoveryOps);
    }

    // Actually recover all of the data from the store...
    if (rc == OK)
    {
        int32_t recoveryMethod = ism_common_getIntConfig(ismENGINE_CFGPROP_USE_RECOVERY_METHOD,
                                                         ismENGINE_DEFAULT_USE_RECOVERY_METHOD);

        if (recoveryMethod > ismENGINE_VALUE_USE_FULL_NEW_RECOVERY)
        {
            recoveryMethod = ismENGINE_DEFAULT_USE_RECOVERY_METHOD;
        }

        ieutTRACEL(pThreadData, recoveryMethod, ENGINE_INTERESTING_TRACE, "Using recovery method %d\n", recoveryMethod);

        // Perform restart recovery
        if (recoveryMethod == ismENGINE_VALUE_USE_CLASSIC_RECOVERY)
        {
            rc = ierr_recoverStoreData(pThreadData, startTime, recoveryOps);
        }
        else
        {
            rc = ierr_newRecoverStoreData(pThreadData, startTime, recoveryOps, recoveryMethod);
        }
    }

    ieut_setEngineRunPhase(EnginePhaseCompletingRecovery);

    if (rc == OK)
    {
        // Ensure the engine / store are fully initialised for all threads
        ieut_createFullThreadDataForAllThreads();

        if (firstRecoveryRC != OK)
        {
            //We found problems but we chose to continue..
            //...check that we ought to have continued
            if (!partialRecoveryAllowed)
            {
                //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
                ism_admin_setMaintenanceMode(rc, 0);

                ieutTRACE_FFDC(ieutPROBE_001, true,
                               "Found errors during recovery but didn't abort despite settings", firstRecoveryRC,
                               NULL);
            }

            rc = ierr_handleRecoveryInconsistencies(pThreadData);
        }
    }

    if (rc == OK)
    {
        rc = ierr_completeRecovery(pThreadData);
        assert(rc == OK);
    }

    ierr_freeRecordsTables(pThreadData, false);

    //Record in the log how recovery went
    if (rc == OK)
    {
        if (firstRecoveryRC == OK)
        {
            LOG(INFO, Messaging, 3002, NULL, "Data recovery completed successfully.");
        }
        else
        {
            LOG(INFO, Messaging, 3012, "%lu",
                    "Recovery has found inconsistencies in the persistent data store and recovered however {0} items have been discarded",
                    itemsDiscarded);
        }
    }
    else
    {
        char messageBuffer[256];

        LOG(CRIT, Messaging, 3003, "%s%d", "" IMA_PRODUCTNAME_SHORT " data recovery failed: Error={0} RC={1}.",
            ism_common_getErrorStringByLocale(rc, ism_common_getLocale(), messageBuffer, 255),
            rc);

        //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
        ism_admin_setMaintenanceMode(rc, 0);
    }

    double elapsedTime = ism_common_readTSC()-startTime;

    ieutTRACEL(pThreadData, elapsedTime, ENGINE_INTERESTING_TRACE, "Total recovery time %.2f seconds.\n", elapsedTime);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//*************************************************************************
/// @brief Prepare for messaging to start
//*************************************************************************
int32_t ierr_startMessaging(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;
    ieutTRACEL(pThreadData, ismEngine_serverGlobal.runPhase, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    /* Perform tasks on clientStates to complete recovery */
    if (rc == OK)
    {
        rc = iecs_completeClientStateRecovery(pThreadData,
                                              recordTable[ISM_STORE_RECTYPE_CLIENT],
                                              recordTable[ISM_STORE_RECTYPE_CPROP],
                                              partialRecoveryAllowed);
    }

    /* Remove any unneeded queues */
    if (rc == OK)
    {
        rc = iert_iterateOverTable( pThreadData
                                  , recordTable[ISM_STORE_RECTYPE_QUEUE]
                                  , ierr_removeUnneededQueue
                                  , NULL );
    }

    /* Remove any unneeded subscriptions */
    if (rc == OK)
    {
        rc = iert_iterateOverTable( pThreadData
                                  , recordTable[ISM_STORE_RECTYPE_SUBSC]
                                  , ierr_removeUnneededQueue
                                  , NULL );
    }

    /* Complete reconciliation of named queues with admin queues */
    if (rc == OK)
    {
        rc = ieqn_reconcileEngineQueueNamespace(pThreadData);
    }

    /* Ensure that we have a single server record */
    if (rc == OK)
    {
        rc = ierr_completeServerRecordRecovery(pThreadData);
    }

    // Move into running phase
    ieut_setEngineRunPhase(EnginePhaseRunning);

    ierr_freeRecordsTables(pThreadData, true);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//*********************************************************************
/// @brief Array containing pointers to offline messages within a generation
///
/// This structure contains an array of pointers to messages within a
/// generation which have not yet been loaded. At the end of recover 
/// this list of messages will be loaded.
//*********************************************************************
#define ierr_OFFLINE_TABLE_SIZE 4096

typedef struct tag_ierrOfflineMsgSet_t
{
    ismStore_GenId_t GenId;
    uint32_t arrayUsed;
    ismEngine_Message_t *MsgTable[ierr_OFFLINE_TABLE_SIZE];
    struct tag_ierrOfflineMsgSet_t *pnext;
} ierrOfflineMsgSet_t;

static ierrOfflineMsgSet_t *FirstOfflineMsgSet = NULL;

typedef struct tag_ierrOfflineTransactionMemberSet_t
{
    ismStore_GenId_t GenId;
    uint32_t arrayUsed;
    ismEngine_RestartTransactionData_t *TransDataTable[ierr_OFFLINE_TABLE_SIZE];
    struct tag_ierrOfflineTransactionMemberSet_t *pnext;
} ierrOfflineTransactionMemberSet_t;

static ierrOfflineTransactionMemberSet_t *FirstOfflineTransactionMemberSet = NULL;

///@brief add's a message to the list of Offline messages
static inline int32_t ierr_addOfflineMessage(ieutThreadData_t *pThreadData,
                                             ismEngine_Message_t *msg)
{
    int32_t rc = OK;
    ismStore_GenId_t GenId;
    ierrOfflineMsgSet_t *pmsgSet, *pprevMsgSet, *pnewMsgSet;

    // First see if we already have a control block for Offline
    // messages in this generation
    ismStore_Handle_t hStoreMsg = msg->StoreMsg.parts.hStoreMsg;
    rc = ism_store_getGenIdOfHandle(hStoreMsg, &GenId);
    if (rc != OK)
    {
        ieutTRACEL(pThreadData, hStoreMsg, ENGINE_ERROR_TRACE,
                   "Failed to lookup generation id for handle 0x%lx\n",
                   hStoreMsg );
        goto mod_exit;
    }

    for (pprevMsgSet = NULL, pmsgSet = FirstOfflineMsgSet; 
         (pmsgSet != NULL) &&
         ((GenId > pmsgSet->GenId ) ||
          ((GenId == pmsgSet->GenId) && (pmsgSet->arrayUsed == ierr_OFFLINE_TABLE_SIZE)));
         pprevMsgSet=pmsgSet, pmsgSet=pmsgSet->pnext)
        ;

    if ((pmsgSet) && (pmsgSet->GenId == GenId))
    {
        assert(pmsgSet->arrayUsed < ierr_OFFLINE_TABLE_SIZE);

        // We have found a msgSet with space for this message
        pmsgSet->MsgTable[pmsgSet->arrayUsed++] = msg;
    }
    else
    {
        assert((pmsgSet == NULL) || (GenId < pmsgSet->GenId));

        // New message set required, add at the end
        pnewMsgSet = iemem_calloc(pThreadData, IEMEM_PROBE(iemem_restoreTable, 8), 1, sizeof(ierrOfflineMsgSet_t));
        if (pnewMsgSet != NULL)
        {
            if (pprevMsgSet == NULL)
            {
                FirstOfflineMsgSet =  pnewMsgSet;
            }
            else
            {
                pprevMsgSet->pnext = pnewMsgSet;
            }
            pnewMsgSet->GenId = GenId;
            pnewMsgSet->arrayUsed = 1;
            pnewMsgSet->MsgTable[0] = msg;
            pnewMsgSet->pnext = pmsgSet;
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }
    }

mod_exit:
    return rc;
}

///@brief add's a message to the list of Offline transaction members to look up later...
static inline int32_t ierr_addOfflineTransactionMemberData(ieutThreadData_t *pThreadData,
                                                           ismEngine_RestartTransactionData_t *pTranData)
{
    int32_t rc = OK;
    ismStore_GenId_t GenId;
    ierrOfflineTransactionMemberSet_t *pSet, *pprevSet, *pnewSet;

    // First see if we already have a control block for Offline
    // messages in this generation
    ismStore_Handle_t hRefHandle = pTranData->operationReference.hRefHandle;
    rc = ism_store_getGenIdOfHandle(hRefHandle, &GenId);
    if (rc != OK)
    {
        ieutTRACEL(pThreadData, hRefHandle, ENGINE_ERROR_TRACE,
                   "Failed to lookup generation id for handle 0x%lx\n",
                   hRefHandle  );
        goto mod_exit;
    }

    for (pprevSet = NULL, pSet = FirstOfflineTransactionMemberSet;
         (pSet != NULL) &&
         ((GenId > pSet->GenId ) ||
          ((GenId == pSet->GenId) && (pSet->arrayUsed == ierr_OFFLINE_TABLE_SIZE)));
         pprevSet=pSet, pSet=pSet->pnext)
        ;

    if ((pSet) && (pSet->GenId == GenId))
    {
        assert(pSet->arrayUsed < ierr_OFFLINE_TABLE_SIZE);

        // We have found a set with space for this transaction data
        pSet->TransDataTable[pSet->arrayUsed++] = pTranData;
    }
    else
    {
        assert((pSet == NULL) || (GenId < pSet->GenId));

        // New message set required, add at the end
        pnewSet = iemem_calloc(pThreadData, IEMEM_PROBE(iemem_restoreTable, 9), 1, sizeof(ierrOfflineTransactionMemberSet_t));
        if (pnewSet != NULL)
        {
            if (pprevSet == NULL)
            {
                FirstOfflineTransactionMemberSet =  pnewSet;
            }
            else
            {
                pprevSet->pnext = pnewSet;
            }
            pnewSet->GenId = GenId;
            pnewSet->arrayUsed = 1;
            pnewSet->TransDataTable[0] = pTranData;
            pnewSet->pnext = pSet;
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }
    }

mod_exit:
    return rc;
}

static int32_t ierr_recoverOfflineMessage( ieutThreadData_t *pThreadData,
                                           ismEngine_Message_t *pMessage )
{
    ismStore_Handle_t hStoreMsg = pMessage->StoreMsg.parts.hStoreMsg;

    ieutTRACEL(pThreadData, hStoreMsg, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "pMessage=%p hdl=0x%lx\n", __func__, pMessage, hStoreMsg);
    int32_t rc = OK;
    ismStore_Record_t record = {0};

    //Set up the record to point to a buffer on the stack....
    uint32_t stackRecBufferSize = 512;
    char stackRecBuffer[stackRecBufferSize];

    char *pFrags[1] = { stackRecBuffer };
    uint32_t pFragsLengths[1] = { stackRecBufferSize };
    record.FragsCount = 1;
    record.pFrags = pFrags;
    record.pFragsLengths = pFragsLengths;

    rc = ism_store_readRecord(hStoreMsg, &record, true);

    if ( rc == ISMRC_StoreBufferTooSmall)
    {
        record.pFrags[0] = iemem_malloc( pThreadData
                                       , IEMEM_PROBE(iemem_restoreTable, 5)
                                       , record.DataLength);
        record.pFragsLengths[0] = record.DataLength;

        rc = ism_store_readRecord(hStoreMsg, &record, true);
    }

    if (rc == OK)
    {
        void *rehydratedRecord = NULL;

        rc = iest_rehydrateMessage( pThreadData
                                  , hStoreMsg
                                  , &record
                                  , NULL // No transaction
                                  , &rehydratedRecord
                                  , pMessage);
    }
    else
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }
        ieutTRACE_FFDC(ieutPROBE_001, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "Unexpectedly couldn't read message record", rc,
                       "Record Handle", &(hStoreMsg), sizeof(hStoreMsg),
                       NULL);
    }

    if (record.pFrags[0] != stackRecBuffer)
    {
        iemem_free(pThreadData, iemem_restoreTable, record.pFrags[0]);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


static int32_t ierr_recoverOfflineTransactionMemberData( ieutThreadData_t *pThreadData
                                                       , ismEngine_RestartTransactionData_t *pTransactionData
                                                       , bool allowBlock)
{
    ismStore_Handle_t OwnerHandle;
    ismStore_RecordType_t OwnerRecType;
    uint64_t OwnerOrderId;

    ismStore_Handle_t hRefHandle = pTransactionData->operationReference.hRefHandle;

    ieutTRACEL(pThreadData, hRefHandle, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "pTransactionData=%p [%s]\n",
               __func__, pTransactionData, (allowBlock ? "block":"noBlock"));

    // Get information about this reference this gives us the owner of the reference and
    // the orderId of the reference on the queue (this is not the same as the orderId of
    // the transaction reference in the transaction)
    int32_t rc =  ism_store_getReferenceInformation(hRefHandle,
                                                    &OwnerHandle,
                                                    &OwnerRecType,
                                                    &OwnerOrderId,
                                                    allowBlock);

    if (rc == OK)
    {
        ismQHandle_t pQueue = NULL;

        //Locate the in-memory version of the owner
        if (   (OwnerRecType == ISM_STORE_RECTYPE_QUEUE)
             ||(OwnerRecType == ISM_STORE_RECTYPE_SUBSC)
             ||(OwnerRecType == ISM_STORE_RECTYPE_REMSRV))
        {
            pQueue = iert_getTableEntry(recordTable[OwnerRecType],
                                        OwnerHandle);

            if (pQueue == NULL)
            {
                //We have transaction data for a owner we can't find. This is bad
                //Come down before we have half a transaction...

                //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
                ism_admin_setMaintenanceMode(rc, 0);

                ieutTRACE_FFDC(ieutPROBE_001, true,
                               "Rehydrating transaction data for an owner we can't find", ISMRC_Error,
                               "TransactionData", pTransactionData, sizeof(ismEngine_RestartTransactionData_t),
                               "OwnerHandle", &OwnerHandle, sizeof(OwnerHandle),
                               "OwnerRecType", &OwnerRecType, sizeof(OwnerRecType),
                               "OwnerOrderId", &OwnerOrderId, sizeof(OwnerOrderId),
                               NULL);
            }
        }
        else
        {
            //We have transaction data for a owner type we don't understand. This is bad
            //Come down before we have half a transaction...

            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);

            ieutTRACE_FFDC(ieutPROBE_002, true,
                           "Rehydrating transaction data for an owner we didn't understand", ISMRC_Error,
                           "TransactionData", pTransactionData, sizeof(ismEngine_RestartTransactionData_t),
                           "OwnerHandle", &OwnerHandle, sizeof(OwnerHandle),
                           "OwnerRecType", &OwnerRecType, sizeof(OwnerRecType),
                           "OwnerOrderId", &OwnerOrderId, sizeof(OwnerOrderId),
                           NULL);

        }

        //Ask the owner to find the message and mark it got
        assert(NULL != pQueue);
        rc = ieq_markMessageGotInTran( pThreadData
                                     , pQueue
                                     , OwnerOrderId
                                     , pTransactionData);

    }
    // In some places, we call this function speculatively to read the reference if possible,
    // if the failure was because the call would block, we just return that to our caller,
    // otherwise we expected to read the reference - so we had better fail.
    else if (rc != ISMRC_WouldBlock)
    {
        //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
        ism_admin_setMaintenanceMode(rc, 0);

        ieutTRACE_FFDC(ieutPROBE_003, true,
                       "Rehydrating transaction data for an reference we can't read", rc,
                       "TransactionData", pTransactionData, sizeof(ismEngine_RestartTransactionData_t),
                       NULL);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


static int32_t ierr_loadOfflineData( ieutThreadData_t *pThreadData )
{
    int32_t rc = OK;
    uint32_t index;
    uint32_t genCount         = 0;
    uint32_t msgsetCount      = 0;
    uint32_t tranmembsetCount = 0;
    uint64_t msgCount         = 0;
    uint64_t tranmembCount    = 0;
    ismStore_GenId_t lastGenId = 0;

    while ((FirstOfflineMsgSet != NULL) ||(FirstOfflineTransactionMemberSet != NULL) )
    {
        ismStore_GenId_t msgsetGenId = 0;
        ismStore_GenId_t tranmembsetGenId = 0;

        if (FirstOfflineMsgSet != NULL)
            msgsetGenId = FirstOfflineMsgSet->GenId;

        if (FirstOfflineTransactionMemberSet != NULL)
             tranmembsetGenId = FirstOfflineTransactionMemberSet->GenId;

        if (msgsetGenId == 0 && tranmembsetGenId == 0)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);

            ieutTRACE_FFDC(ieutPROBE_001, true,
                           "Gen Ids were zero but we had non-null sets....", ISMRC_Error,
                           "FirstOfflineMsgSet", FirstOfflineMsgSet, sizeof(*FirstOfflineMsgSet),
                           "FirstOfflineTransactionMemberSet", FirstOfflineTransactionMemberSet, sizeof(*FirstOfflineTransactionMemberSet),
                           NULL);
        }

        if (   (tranmembsetGenId != 0)
            && ((tranmembsetGenId < msgsetGenId) || (msgsetGenId == 0)))
        {
            //The generation we need to read for transactions is earlier than for
            //messages... we'll do that next    
            ierrOfflineTransactionMemberSet_t *ptranmembSet = FirstOfflineTransactionMemberSet;
            FirstOfflineTransactionMemberSet = FirstOfflineTransactionMemberSet->pnext;
            
            tranmembsetCount++;
            
            if (ptranmembSet->GenId != lastGenId)
            {
                lastGenId = ptranmembSet->GenId;
                genCount++;
            }

            ieutTRACEL(pThreadData, ptranmembSet->arrayUsed, ENGINE_NORMAL_TRACE,
                       "About to load %u transaction member data records from generation %u\n",
                       ptranmembSet->arrayUsed, ptranmembSet->GenId);

            for (index=0; index < ptranmembSet->arrayUsed; index++)
            {
                //We only allow "blocking" if the index is 0, as all the entries in this set should
                //be in the same generation...
                rc =  ierr_recoverOfflineTransactionMemberData( pThreadData
                                                              , ptranmembSet->TransDataTable[index]
                                                              , (index == 0));
                if (rc != OK)
                {
                    //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
                    ism_admin_setMaintenanceMode(rc, 0);

                    ieutTRACE_FFDC(ieutPROBE_002, true,
                                   "Failed trying to read data for late transaction...", rc,
                                   "ptranmembSet->TransDataTable[index]", ptranmembSet->TransDataTable[index], sizeof(*(ptranmembSet->TransDataTable[index])),
                                   NULL);
                }

                //Unlike with offline messages we need to free the data in the table...
                iemem_free(pThreadData, iemem_restoreTable,ptranmembSet->TransDataTable[index]);
            }
            tranmembCount += ptranmembSet->arrayUsed;

            // Move to the next set and free the current one
            iemem_free(pThreadData, iemem_restoreTable, ptranmembSet);


        }
        else
        {
            assert(msgsetGenId != 0 && FirstOfflineMsgSet != NULL);

            //Do a set of messages....
            ierrOfflineMsgSet_t *pmsgSet = FirstOfflineMsgSet;
            FirstOfflineMsgSet = FirstOfflineMsgSet->pnext;

            msgsetCount++;
            if (pmsgSet->GenId != lastGenId)
            {
                lastGenId = pmsgSet->GenId;
                genCount++;
            }

            ieutTRACEL(pThreadData, pmsgSet->arrayUsed, ENGINE_NORMAL_TRACE,
                       "About to load %u messages from generation %u\n",
                       pmsgSet->arrayUsed, pmsgSet->GenId);

            for (index=0; index < pmsgSet->arrayUsed; index++)
            {
                // Any error will already have been reported
                (void)ierr_recoverOfflineMessage( pThreadData, pmsgSet->MsgTable[index] );
            }
            msgCount += pmsgSet->arrayUsed;

            // Move to the next set and free the current one
            iemem_free(pThreadData, iemem_restoreTable, pmsgSet);
        }
    }

    ieutTRACEL(pThreadData, msgCount+tranmembCount, ENGINE_NORMAL_TRACE,
               "Offline statistics: GenerationsAccessed: %u MsgSetsProcessed: %u OfflineMsgsLoaded %lu TranSetsProcessed %u TransactionMembers Processed %lu\n",
               genCount, msgsetCount, msgCount, tranmembsetCount, tranmembCount);

    return rc;
}

