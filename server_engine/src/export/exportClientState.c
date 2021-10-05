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
/// @file  exportClientState.c
/// @brief Export / Import functions for clientStates
//*********************************************************************
#define TRACE_COMP Engine

#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "clientState.h"
#include "clientStateInternal.h"
#include "exportClientState.h"
#include "engineStore.h"
#include "engineHashSet.h"
#include "exportMessage.h"
#include "msgCommon.h"          // iem_ functions
#include "topicTree.h"          // iett_ functions
#include "resourceSetStats.h"   // iere_ functions

/// @brief Context used to search for clientIds matching specified regular expression
typedef struct tag_ieieMatchClientIdContext_t
{
    ism_regex_t regexClientId;       ///< Regular expression of clientIds to find
    ieutHashTable_t *clientIdTable;  ///< Hash table of the clientIds found
    bool includeInternalClientIds;   ///< Whether internal clientIds (starting with _) should be included
    int32_t rc;                      ///< Return code used to identify problems during search
} ieieMatchClientIdContext_t;

/// @brief Context used when traversing the hash table of matching clientIds
typedef struct tag_ieieExportClientStateContext_t
{
    int32_t rc;
    ieieExportResourceControl_t *control; ///< Control structure for this export request
    ieieClientStateInfo_t *CSIBuffer;     ///< Buffer used to for each clientId to pull data together
    size_t CSIBufferLength;               ///< Length of the CSI buffer
} ieieExportClientStateContext_t;

typedef enum tag_ieieImportClientStateStage_t
{
    ieieICSS_Start = 0,
    ieieICSS_DiscardOld,
    ieieICSS_CreateNew,
    ieieICSS_RecordCreation,
    ieieICSS_AddUnreleasedMessageStates,
    ieieICSS_AddWillMessage,
    ieieICSS_Finish
} ieieImportClientStateStage_t;

typedef struct tag_ieieImportClientStateCallbackContext_t
{
    ieieImportClientStateStage_t stage;
    bool wentAsync;
    uint64_t asyncId;
    uint64_t dataId;
    const char *clientId;
    const char *userId;
    const char *willTopic;
    uint32_t *UMSArray;
    ismEngine_ClientState_t *client;
    ieieImportResourceControl_t *control;
    ieieClientStateInfo_t info;
} ieieImportClientStateCallbackContext_t;

// Forward declaration of async Import function
void ieie_asyncDoImportClientState(int32_t retcode, void *handle, void *pContext);
void ieie_storeAsyncDoImportClientState(int retcode, void *pContext);

//****************************************************************************
/// @internal
///
/// @brief  Add the clientId to the table if it matches the regex
///
/// @param[in]     pClient           The client state to match against
/// @param[in]     context           ieieMatchClientIdsContext_t
///
/// @return true to continue searching or false
//****************************************************************************
static bool ieie_matchClientId(ieutThreadData_t *pThreadData,
                               ismEngine_ClientState_t *pClient,
                               void *context)
{
    ieieMatchClientIdContext_t *pContext = (ieieMatchClientIdContext_t *)context;
    char *pClientId = pClient->pClientId;

    assert(pClientId != NULL);

    // Skip client-states that don't match our regex
    if (ism_regex_match(pContext->regexClientId, pClientId) != 0) goto mod_exit;

    // Skip internal client-states (those that start __) unless explicitly requested
    if ((pClientId[0] == '_') &&
        (strlen(pClientId) > 2 && pClientId[1] == '_') &&
        (pContext->includeInternalClientIds == false))
    {
        goto mod_exit;
    }

    // Add this clientId to the hash table using the hash that the subscription will refer
    // to (we can then use this table later to see if a non-shared subscription is one we
    // are interested in).
    // Note: We have a value which is the pClient, which will now identify this client in the
    //       export file.
    int32_t rc = ieut_putHashEntry(pThreadData,
                                   pContext->clientIdTable,
                                   ieutFLAG_DUPLICATE_KEY_STRING,
                                   pClientId,
                                   iett_generateClientIdHash(pClientId),
                                   pClient, // Cannot dereference this, but it becomes our data identifier
                                   0);

    // Don't fail if we already had an entry for this clientId
    if (rc != OK && rc != ISMRC_ExistingKey) pContext->rc = rc;

mod_exit:

    return (pContext->rc == OK) ? true : false;
}

//****************************************************************************
/// @internal
///
/// @brief  Fill in the passed hash table with all of the clientIds that match
///         the specified regular expression.
///
/// @param[in]     control        ieieExportResourceControl_t for this export
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_getMatchingClientIds(ieutThreadData_t *pThreadData,
                                  ieieExportResourceControl_t *control)
{
    int32_t rc = OK;

    assert(control != NULL);
    assert(control->clientIdTable != NULL);

    ieutTRACEL(pThreadData, control->clientIdTable, ENGINE_FNC_TRACE, FUNCTION_ENTRY "control->clientIdTable=%p\n",
               __func__, control->clientIdTable);

    ieieMatchClientIdContext_t context = {control->regexClientId,
                                          control->clientIdTable,
                                          (control->options & ismENGINE_EXPORT_RESOURCES_OPTION_INCLUDE_INTERNAL_CLIENTIDS) != 0,
                                          OK};

    // Now get the results
    ism_time_t startTime = ism_common_currentTimeNanos();
    iecs_traverseClientStateTable(pThreadData, NULL, 0, 0, NULL, ieie_matchClientId, &context);
    ism_time_t endTime = ism_common_currentTimeNanos();
    ism_time_t diffTime = endTime-startTime;
    double diffTimeSecs = ((double)diffTime) / 1000000000;

    rc = context.rc;

    if (rc != OK) ieut_clearHashTable(pThreadData, control->clientIdTable);

    ieutTRACEL(pThreadData, control->clientIdTable->totalCount, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, totalCount=%lu. (Time taken %.2fsecs)\n",
               __func__, rc, control->clientIdTable->totalCount, diffTimeSecs);

    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Export an individual client state.
///
/// @param[in]     clientId      Regular expression for the clientId
/// @param[in]     keyHash       The hash of the key in the table of clientIds being
///                              exported (which is NOT the hash in the client state
///                              table)
/// @param[in]     value         The value of pClient, which we *cannot dereference*
///                              but is used as the client identifier in our export file
/// @param[in,out] context       The ieieExportClientStateContext_t for this export
///                              request.
//****************************************************************************
void ieie_exportClientState(ieutThreadData_t *pThreadData,
                            char *clientId,
                            uint32_t keyHash,
                            void *value,
                            void *context)
{
    bool unreleasedDeliveryStateLocked = false;

    ieieExportClientStateContext_t *pContext = (ieieExportClientStateContext_t *)context;

    uint64_t dataId = (uint64_t)value;

    ieutTRACEL(pThreadData, dataId, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "clientId='%s' (0x%08x) dataId=0x%0lx\n",
               __func__, clientId, keyHash, dataId);

    if (pContext->rc == OK)
    {
        ismEngine_ClientState_t *pClient;
        uint32_t clientIdHash = iecs_generateClientIdHash(clientId);

        ieieClientStateInfo_t *CSI = pContext->CSIBuffer;
        uint8_t *curDataPos = NULL;
        ismEngine_UnreleasedState_t *pUnrelChunk;

        ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);

        pClient = iecs_getVictimizedClient(pThreadData,
                                           (const char *)clientId,
                                           clientIdHash);

        // We found this client - go get the detail from it while we've got the lock held
        if (pClient != NULL)
        {
            size_t requiredCSIBufferLength = sizeof(ieieClientStateInfo_t);

            assert(pClient->Durability == iecsDurable || pClient->Durability == iecsNonDurable);

            CSI->Durable = (pClient->Durability == iecsDurable);
            CSI->ClientIdLength = (uint32_t)(strlen(clientId)+1);
            CSI->ProtocolId = pClient->protocolId;
            CSI->UserIdLength = (uint32_t)(pClient->pUserId ? strlen(pClient->pUserId)+1 : 0);
            CSI->LastConnectedTime = ism_common_getExpire(pClient->LastConnectedTime);
            CSI->ExpiryInterval = pClient->ExpiryInterval;
            CSI->WillDelay = pClient->WillDelay;
            CSI->UMSCount = 0;

            if (pClient->hWillMessage != NULL)
            {
                assert(pClient->pWillTopicName != NULL);

                CSI->WillMsgId = (uint64_t)(pClient->hWillMessage);
                iem_recordMessageUsage(pClient->hWillMessage);
                CSI->WillMsgTimeToLive = pClient->WillMessageTimeToLive;
                CSI->WillTopicNameLength = (uint32_t)(strlen(pClient->pWillTopicName)+1);
            }
            else
            {
                CSI->WillMsgId = 0;
                CSI->WillMsgTimeToLive = iecsWILLMSG_TIME_TO_LIVE_INFINITE;
                CSI->WillTopicNameLength = 0;
            }

            requiredCSIBufferLength += CSI->ClientIdLength + CSI->UserIdLength + CSI->WillTopicNameLength;

            // Work out the unreleased delivery Ids
            iecs_lockUnreleasedDeliveryState(pClient);
            unreleasedDeliveryStateLocked = true;

            // Loop through the chunks to work out how much buffer we need for them
            pUnrelChunk = pClient->pUnreleasedHead;
            while (pUnrelChunk != NULL)
            {
                CSI->UMSCount += pUnrelChunk->InUse;
                pUnrelChunk = pUnrelChunk->pNext;
            }

            requiredCSIBufferLength += (size_t)(CSI->UMSCount) * sizeof(pUnrelChunk->Slot[0].UnrelDeliveryId);

            if (requiredCSIBufferLength > pContext->CSIBufferLength)
            {
                ieieClientStateInfo_t *newBuffer = iemem_realloc(pThreadData,
                                                                 IEMEM_PROBE(iemem_exportResources, 6),
                                                                 pContext->CSIBuffer, requiredCSIBufferLength);

                if (newBuffer == NULL)
                {
                    pContext->rc = ISMRC_AllocateError;
                    ism_common_setError(pContext->rc);
                    goto skip_remainingWork;
                }
                else
                {
                    CSI = pContext->CSIBuffer = newBuffer;
                    pContext->CSIBufferLength = requiredCSIBufferLength;
                }
            }

            curDataPos = (uint8_t *)(CSI+1);

            memcpy(curDataPos, clientId, CSI->ClientIdLength);
            curDataPos += CSI->ClientIdLength;

            if (CSI->UserIdLength != 0)
            {
                memcpy(curDataPos, pClient->pUserId, CSI->UserIdLength);
                curDataPos += CSI->UserIdLength;
            }

            if (CSI->WillTopicNameLength != 0)
            {
                memcpy(curDataPos, pClient->pWillTopicName, CSI->WillTopicNameLength);
                curDataPos += CSI->WillTopicNameLength;
            }
        }

skip_remainingWork:

        ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);

        if (pClient != NULL && pContext->rc == OK)
        {
            assert(unreleasedDeliveryStateLocked == true);

            // We have some unreleased delivery Ids to deal with - the count so far
            // is only rough, now we actually pull them into the structure.
            if (CSI->UMSCount != 0)
            {
                CSI->UMSCount = 0;

                pUnrelChunk = pClient->pUnreleasedHead;
                while (pUnrelChunk != NULL)
                {
                    if (pUnrelChunk->InUse != 0)
                    {
                        for (int32_t i = 0; i < pUnrelChunk->Limit; i++)
                        {
                            if (pUnrelChunk->Slot[i].fSlotInUse && !pUnrelChunk->Slot[i].fUncommitted)
                            {
                                memcpy(curDataPos, &pUnrelChunk->Slot[i].UnrelDeliveryId, sizeof(pUnrelChunk->Slot[i].UnrelDeliveryId));
                                curDataPos += sizeof(pUnrelChunk->Slot[i].UnrelDeliveryId);
                                CSI->UMSCount += 1;
                            }
                        }
                    }

                    pUnrelChunk = pUnrelChunk->pNext;
                }
            }

            // Once we release this lock, we cannot consider referring to the pClient (as it is, we can only
            // refer to the UMS information anyway).
            iecs_unlockUnreleasedDeliveryState(pClient);

            if (pContext->rc == OK && CSI->WillMsgId != 0)
            {
                pContext->rc = ieie_exportMessage(pThreadData,
                                                  (ismEngine_Message_t *)(CSI->WillMsgId),
                                                  pContext->control,
                                                  true);
            }

            if (pContext->rc == OK)
            {
                uint32_t dataLen = (uint32_t)(curDataPos-(uint8_t *)CSI);

                ieutTRACEL(pThreadData, pClient, ENGINE_HIGH_TRACE, "Exporting clientId='%s' (pClient=%p dataId=0x%0lx dataLen=%u)\n",
                           (char *)(CSI+1), pClient, dataId, dataLen);

                pContext->rc = ieie_writeExportRecord(pThreadData,
                                                      pContext->control,
                                                      ieieDATATYPE_EXPORTEDCLIENTSTATE,
                                                      dataId,
                                                      dataLen,
                                                      (void *)CSI);
            }
        }
        else
        {
            if (CSI->WillMsgId != 0)
            {
                iem_releaseMessage(pThreadData, (ismEngine_Message_t *)(CSI->WillMsgId));
            }

            if (unreleasedDeliveryStateLocked)
            {
                assert(pClient != NULL);
                iecs_unlockUnreleasedDeliveryState(pClient);
            }
        }
    }

    ieutTRACEL(pThreadData, pContext->rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "pContext->rc=%d\n", __func__, pContext->rc);
}

//****************************************************************************
/// @internal
///
/// @brief  Export the information for client states whose clientId matches
///         the specified regex.
///
/// @param[in]     control        ieieExportResourceControl_t for this export
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_exportClientStates(ieutThreadData_t *pThreadData,
                                ieieExportResourceControl_t *control)
{
    int32_t rc = OK;

    assert(control->file != NULL);

    ieutTRACEL(pThreadData, control->clientId, ENGINE_FNC_TRACE, FUNCTION_ENTRY "clientId='%s' outFile=%p\n", __func__,
               control->clientId, control->file);

    ieieExportClientStateContext_t context = {0};

    context.control = control;

    if (control->clientIdTable->totalCount != 0)
    {
        // Get a buffer ready for the CSI structure (and 1k of additional data)
        context.CSIBufferLength = sizeof(ieieClientStateInfo_t) + 1024;
        context.CSIBuffer = iemem_malloc(pThreadData,
                                         IEMEM_PROBE(iemem_exportResources, 7),
                                         context.CSIBufferLength);
        if (context.CSIBuffer == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        // Initialize the immutable portions of the CSI buffer
        ieieClientStateInfo_t *CSI = context.CSIBuffer;

        memset(CSI, 0, sizeof(*CSI));
        CSI->Version = ieieCLIENTSTATE_CURRENT_VERSION;

        // Traverse the table of found clientIds exporting each one found.
        ieut_traverseHashTable(pThreadData, control->clientIdTable, ieie_exportClientState, &context);

        rc = context.rc;
    }

mod_exit:

    if (context.CSIBuffer != NULL) iemem_free(pThreadData, iemem_exportResources, context.CSIBuffer);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Check that we can import this client state and add it to the
/// active importing clientId hash table and this imports validated set.
///
/// @param[in]     control        ieieImportResourceControl_t for this import
/// @param[in]     dataId         dataId (The dataId of the client)
/// @param[in]     data           ieieClientStateInfo_t
/// @param[in]     dataLen        Length of the clientStateInfo
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieieValidateClientStateImport(ieutThreadData_t *pThreadData,
                                      ieieImportResourceControl_t *control,
                                      uint64_t dataId,
                                      uint8_t *data,
                                      size_t dataLen)
{
    int32_t rc = OK;
    ieieClientStateInfo_t *CSI = (ieieClientStateInfo_t *)data;
    uint32_t clientIdLength;
    uint8_t *variableData;

    ieutTRACEL(pThreadData, dataId, ENGINE_FNC_TRACE, FUNCTION_ENTRY "dataId=0x%0lx\n", __func__, dataId);

    if (CSI->Version == ieieCLIENTSTATE_CURRENT_VERSION)
    {
        clientIdLength = CSI->ClientIdLength;
        variableData = data+sizeof(ieieClientStateInfo_t);
    }
    else if (CSI->Version == ieieCLIENTSTATE_VERSION_2)
    {
        ieieClientStateInfo_V2_t *CSI_V2 = (ieieClientStateInfo_V2_t *)data;
        clientIdLength = CSI_V2->ClientIdLength;
        variableData = data+sizeof(ieieClientStateInfo_V2_t);
    }
    else if (CSI->Version == ieieCLIENTSTATE_VERSION_1)
    {
        ieieClientStateInfo_V1_t *CSI_V1 = (ieieClientStateInfo_V1_t *)data;
        clientIdLength = CSI_V1->ClientIdLength;
        variableData = data+sizeof(ieieClientStateInfo_V1_t);
    }
    else
    {
        rc = ISMRC_FileCorrupt;
        ism_common_setError(rc);
        goto mod_exit;
    }

    assert(clientIdLength != 0);

    char *clientId = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_exportResources, 2), clientIdLength);

    if (clientId == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(clientId, variableData, clientIdLength);
    uint32_t clientIdHash = iecs_generateClientIdHash((const char *)clientId);

    ieieImportExportGlobal_t *importExportGlobal = ismEngine_serverGlobal.importExportGlobal;

    ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);

    ismEngine_ClientState_t *pClient = iecs_getVictimizedClient(pThreadData,
                                                                (const char *)clientId,
                                                                clientIdHash);

    if (pClient == NULL || (pClient->OpState == iecsOpStateZombie && pClient->pThief == NULL))
    {
        if (importExportGlobal->activeImportClientIdTable == NULL)
        {
            rc = ieut_createHashTable(pThreadData,
                                      ieieIMPORTEXPORT_INITIAL_ACTIVE_CLIENTID_TABLE_CAPACITY,
                                      iemem_exportResources,
                                      &importExportGlobal->activeImportClientIdTable);
        }

        if (rc == OK)
        {
            rc = ieut_putHashEntry(pThreadData,
                                   importExportGlobal->activeImportClientIdTable,
                                   ieutFLAG_DUPLICATE_NONE,
                                   clientId,
                                   clientIdHash,
                                   (void *)dataId, // Just for information
                                   0);

            if (rc == ISMRC_ExistingKey)
            {
                rc = ISMRC_ClientIDInUse;
                ism_common_setErrorData(rc, "%s", clientId);
            }
        }
    }
    else
    {
        rc = ISMRC_ClientIDInUse;
        ism_common_setErrorData(rc, "%s", clientId);
    }

    ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);

    // All OK - add it to the private validatedClientIds table
    if (rc == OK)
    {
        rc = ieut_putHashEntry(pThreadData,
                               control->validatedClientIds,
                               ieutFLAG_DUPLICATE_NONE,
                               clientId,
                               clientIdHash,
                               pClient, // Used to determine if we need a discard
                               0);

        // Couldn't add it to our private table - remove it from the global one
        if (rc != OK)
        {
            ism_common_setError(rc);

            ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);
            ieut_removeHashEntry(pThreadData,
                                 importExportGlobal->activeImportClientIdTable,
                                 clientId,
                                 clientIdHash);
            ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);
        }
        else if (pClient != NULL)
        {
            assert(pClient->OpState == iecsOpStateZombie);

            control->validatedZombieClientCount += 1;
        }
    }

    if (rc != OK)
    {
        assert(clientId != NULL);

        if (rc == ISMRC_ClientIDInUse)
        {
            char humanIdentifier[clientIdLength + strlen("ClientID:")+1];
            sprintf(humanIdentifier, "ClientID:%s", clientId);

            ieie_recordImportError(pThreadData,
                                   control,
                                   ieieDATATYPE_EXPORTEDCLIENTSTATE,
                                   dataId,
                                   humanIdentifier,
                                   rc);
        }

        iemem_free(pThreadData, iemem_exportResources, clientId);
    }

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Release clientIds that have been marked as being imported
///
/// @param[in]     key            ClientId
/// @param[in]     keyHash        Hash of the key value
/// @param[in]     value          pClient found when validating (NULL if
///                               none was found - not likely to still be
///                               valid)
/// @param[in]     context        For convenience, the address of the
///                               activeImportClientIdTable.
///
/// @remark Note that once the clientId has been removed from the
/// activeImportClientIdTable the key is freed (rendering it invalid for
/// future queries)
//****************************************************************************
void ieie_releaseValidatedClientId(ieutThreadData_t *pThreadData,
                                   char *key,
                                   uint32_t keyHash,
                                   void *value,
                                   void *context)
{
    ieutHashTable_t *activeImportClientIdTable = (ieutHashTable_t *)context;
    assert(activeImportClientIdTable != NULL);

    // Note: The clientState table mutex is used to lock the activeImportClientIdTable
    // to ensure that the 'being imported' check is atomic with a clientState being
    // added.
    ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);

    ieut_removeHashEntry(pThreadData,
                         activeImportClientIdTable,
                         key,
                         keyHash);

    ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);

    iemem_free(pThreadData, iemem_exportResources, key);
}

//****************************************************************************
/// @internal
///
/// @brief  Release an individual ClientState that was imported
///
/// @param[in]     key            DataId
/// @param[in]     keyHash        Hash of the key value
/// @param[in]     value          pClient imported
/// @param[in]     context        unused
//****************************************************************************
void ieie_releaseImportedClientState(ieutThreadData_t *pThreadData,
                                     char *key,
                                     uint32_t keyHash,
                                     void *value,
                                     void *context)
{
    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)value;
    ieieImportResourceControl_t *pControl = context;

    // Report on the state of this client before we release our useCount, if it's not as expected.
    pthread_spin_lock(&pClient->UseCountLock);
    if (pClient->OpState == iecsOpStateZombieExpiry)
    {
        ieutTRACEL(pThreadData, pClient, ENGINE_UNUSUAL_TRACE, FUNCTION_IDENT "Client '%s' (%p) already expired.\n",
                   __func__, pClient->pClientId ? pClient->pClientId : "NULL", pClient);
    }
    else if (pClient->OpState != iecsOpStateZombie)
    {
        ieutTRACEL(pThreadData, pClient->OpState, ENGINE_UNUSUAL_TRACE, FUNCTION_IDENT "Client '%s' (%p) is in state %d.\n",
                   __func__,  pClient->pClientId ? pClient->pClientId : "NULL", pClient, pClient->OpState);
    }
    pClient->fSuspendExpiry = false;
    pthread_spin_unlock(&pClient->UseCountLock);

    ism_time_t checkScheduleTime;
    iecs_setExpiryFromLastConnectedTime(pThreadData, pClient, &checkScheduleTime);
    if (pClient->ExpiryTime != 0) pThreadData->stats.zombieSetToExpireCount += 1;

    if (checkScheduleTime != 0 && pControl != NULL)
    {
        if (pControl->lowestClientStateExpiryCheckTime == 0 || checkScheduleTime < pControl->lowestClientStateExpiryCheckTime)
        {
            pControl->lowestClientStateExpiryCheckTime = checkScheduleTime;
        }
    }

    iecs_releaseClientStateReference(pThreadData, pClient, false, false);
}

//****************************************************************************
/// @brief Find the dataId for a client that is actively being imported
///
/// @param[in]     clientId       ClientId being checked
/// @param[in]     clientIdHash   Hash calculated for the clientId
///
/// @remark The clientState table (ismEngine_serverGlobal.Mutex) should be held
/// when this is being called.
///
/// @return The dataId of the import, or zero if it is not being imported.
//****************************************************************************
uint64_t ieie_findActiveImportClientDataId(ieutThreadData_t *pThreadData,
                                           const char *clientId,
                                           uint32_t clientIdHash)
{
    uint64_t dataId = 0;

    ieieImportExportGlobal_t *importExportGlobal = ismEngine_serverGlobal.importExportGlobal;
    assert(importExportGlobal != NULL);

    if (importExportGlobal->activeImportClientIdTable != NULL &&
        importExportGlobal->activeImportClientIdTable->totalCount != 0)
    {
        (void)ieut_getHashEntry(importExportGlobal->activeImportClientIdTable,
                                clientId,
                                clientIdHash,
                                (void **)&dataId);
    }

    return dataId;
}

//****************************************************************************
/// @brief Check whether the specified clientId is part of a set being imported
///
/// @param[in]     clientId       ClientId being checked
/// @param[in]     clientIdHash   Hash calculated for the clientId
///
/// @remark The clientState table (ismEngine_serverGlobal.Mutex) should be held
/// when this is being checked.
///
/// @return true if the specified clientId is being imported, otherwise false.
//****************************************************************************
bool ieie_isClientIdBeingImported(ieutThreadData_t *pThreadData, const char *clientId, uint32_t clientIdHash)
{
    return (ieie_findActiveImportClientDataId(pThreadData, clientId, clientIdHash) != 0);
}

//****************************************************************************
/// @internal
///
/// @brief  Find an imported ClientState by dataId
///
/// @param[in]     control              Import control structure
/// @param[in]     dataId               Identifier given to the clientState on export
/// @param[out]    ppClient             Client found
///
/// @remark If the client is found, it does NOT have its use count increased.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_findImportedClientState(ieutThreadData_t *pThreadData,
                                     ieieImportResourceControl_t *control,
                                     uint64_t dataId,
                                     ismEngine_ClientState_t **ppClient)
{
    uint32_t dataIdHash = (uint32_t)(dataId>>4);

    ismEngine_getRWLockForRead(&control->importedTablesLock);
    int32_t rc = ieut_getHashEntry(control->importedClientStates,
                                   (const char *)dataId,
                                   dataIdHash,
                                   (void **)ppClient);
    ismEngine_unlockRWLock(&control->importedTablesLock);

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "dataId=0x%0lx pClient=%p rc=%d\n",
               __func__, dataId, *ppClient, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Find an imported ClientState by ClientId string
///
/// @param[in]     control       Import control structure
/// @param[in]     clientId      ClientId string
/// @param[out]    ppClient      Client found
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_findImportedClientStateByClientId(ieutThreadData_t *pThreadData,
                                               ieieImportResourceControl_t *control,
                                               const char *clientId,
                                               ismEngine_ClientState_t **ppClient)
{
    int32_t rc;
    uint32_t clientIdHash = iecs_generateClientIdHash(clientId);
    ismEngine_ClientState_t *foundClient;

    // It should be in the (relatively small) hash table of actively importing clientIds
    ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);
    uint64_t dataId = ieie_findActiveImportClientDataId(pThreadData, clientId, clientIdHash);

#ifndef NDEBUG
    // For debug builds, we use the clientState table to confirm the result - note we need
    // to do it here because we need the mutex locked.
    ismEngine_ClientState_t *victimizedClient = iecs_getVictimizedClient(pThreadData,
                                                                         clientId,
                                                                         clientIdHash);
#endif
    ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);

    if (dataId == 0)
    {
        rc = ISMRC_NotFound;
        ism_common_setError(rc);
        foundClient = NULL;
    }
    else
    {
        rc = ieie_findImportedClientState(pThreadData, control, dataId, &foundClient);

        assert(rc == OK);
        assert(foundClient != NULL && foundClient == victimizedClient);

        *ppClient = foundClient;
    }

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "clientId='%s' dataId=0x%0lx foundClient=%p rc=%d\n",
               __func__, clientId, dataId, foundClient, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Find the imported ClientState which owns the dataId of a queueHandle
///
/// @param[in]     control       Import control structure
/// @param[in]     dataId        DataId of the queue handle being sought
/// @param[out]    ppClient      Client found
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_findImportedClientStateByQueueDataId(ieutThreadData_t *pThreadData,
                                                  ieieImportResourceControl_t *control,
                                                  uint64_t dataId,
                                                  ismEngine_ClientState_t **ppClient)
{
    uint64_t clientDataId = 0;
    ismEngine_ClientState_t *foundClient = NULL;
    uint32_t dataIdHash = (uint32_t)(dataId>>4);

    ismEngine_Subscription_t *subscription = NULL;

    ismEngine_getRWLockForRead(&control->importedTablesLock);
    int32_t rc = ieut_getHashEntry(control->importedSubscriptions,
                                   (const char *)dataId,
                                   dataIdHash,
                                   (void **)&subscription);
    ismEngine_unlockRWLock(&control->importedTablesLock);

    if (rc != OK)
    {
        ism_common_setError(rc);
    }
    else if (subscription != NULL)
    {
        uint32_t clientIdHash = iecs_generateClientIdHash(subscription->clientId);

        // It should be in the (relatively small) hash table of actively importing clientIds
        // NOTE: clientIdHash in the subscription is NOT the same value as the one in the active
        //       import table - so we need to use the iecs_ generated value.
        ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);
        clientDataId = ieie_findActiveImportClientDataId(pThreadData,
                                                         subscription->clientId,
                                                         clientIdHash);
        ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);

        if (clientDataId == 0)
        {
            rc = ISMRC_NotFound;
            ism_common_setError(rc);
        }
        else
        {
            rc = ieie_findImportedClientState(pThreadData, control, clientDataId, &foundClient);

            assert(rc == OK);
            assert(foundClient != NULL);

            *ppClient = foundClient;
        }
    }
    else
    {
        rc = ISMRC_NotFound;
        ism_common_setError(rc);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "dataId=0x%0lx foundClient=%p rc=%d\n",
               __func__, dataId, foundClient, rc);

    return rc;
}

//****************************************************************************
/// @brief Perform all of the actions required to import a client state handling
/// async completion in any one of them.
///
/// @param[in]     rc             Return code of the previous stage
/// @param[in]     handle         Handle returned by previous stage
/// @param[in]     context       The callback context for this client state
///
/// @return OK on successful completion, ISMRC_AsyncCompletion if the operation
/// needed to go async or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_doImportClientState(ieutThreadData_t *pThreadData,
                                 int32_t rc,
                                 void *handle,
                                 ieieImportClientStateCallbackContext_t *context)
{
    ieieImportClientStateCallbackContext_t *pContext = context;
    ieieImportClientStateCallbackContext_t **ppContext = &pContext;
    ieieImportResourceControl_t *control = context->control;
    uint64_t dataId = context->dataId;

    ieutTRACEL(pThreadData, dataId, ENGINE_FNC_TRACE, FUNCTION_ENTRY "dataId=0x%0lx, clientId=%s\n",
               __func__, dataId, context->clientId);

    while(rc != ISMRC_AsyncCompletion)
    {
        context->stage += 1;

        ieutTRACEL(pThreadData, context->stage, ENGINE_HIFREQ_FNC_TRACE, "dataId=0x%0lx, stage=%u\n",
                   dataId, (uint32_t)context->stage);

        switch(context->stage)
        {
            case ieieICSS_Start:
                // Don't ever expect value to be Start at this stage...
                assert(false);
                break;
            case ieieICSS_DiscardOld:
                rc = iecs_discardZombieClientState(pThreadData,
                                                   context->clientId,
                                                   true,
                                                   ppContext,
                                                   sizeof(ppContext),
                                                   ieie_asyncDoImportClientState);
                if (rc == ISMRC_NotFound) rc = OK;
                break;
            case ieieICSS_CreateNew:
                if (rc == OK)
                {
                    iecsNewClientStateInfo_t clientInfo;

                    clientInfo.pClientId = context->clientId;
                    clientInfo.pUserId = context->userId;
                    clientInfo.lastConnectedTime = ism_common_convertExpireToTime(context->info.LastConnectedTime);
                    clientInfo.protocolId = context->info.ProtocolId;
                    clientInfo.durability = context->info.Durable ? iecsDurable : iecsNonDurable;
                    clientInfo.expiryInterval = context->info.ExpiryInterval;
                    clientInfo.resourceSet = iecs_getResourceSet(pThreadData,
                                                                 clientInfo.pClientId,
                                                                 clientInfo.protocolId,
                                                                 iereOP_ADD);

                    rc = iecs_newClientStateImport(pThreadData,
                                                   &clientInfo,
                                                   &context->client);

                    if (rc == OK)
                    {
                        assert(context->client != NULL);

                        rc = iecs_addClientState(pThreadData,
                                                 context->client,
                                                 ismENGINE_CREATE_CLIENT_INTERNAL_OPTION_IMPORTING,
                                                 iecsCREATE_CLIENT_OPTION_NONE,
                                                 ppContext,
                                                 sizeof(ppContext),
                                                 ieie_asyncDoImportClientState);

                        if (rc != OK && rc != ISMRC_AsyncCompletion)
                        {
                            iecs_freeClientState(pThreadData, context->client, true);
                        }
                    }
                }
                break;
            case ieieICSS_RecordCreation:
                if (rc == OK)
                {
                    assert(handle == NULL || handle == context->client);
                    assert(context->client->OpState == iecsOpStateZombie);

                    uint32_t dataIdHash = (uint32_t)(context->dataId>>4);

                    ismEngine_getRWLockForWrite(&control->importedTablesLock);
                    rc = ieut_putHashEntry(pThreadData,
                                           control->importedClientStates,
                                           ieutFLAG_NUMERIC_KEY,
                                           (const char *)context->dataId,
                                           dataIdHash,
                                           context->client,
                                           0);
                    ismEngine_unlockRWLock(&control->importedTablesLock);

                    if (rc != OK)
                    {
                        ieie_releaseImportedClientState(pThreadData,
                                                        (char *)context->dataId,
                                                        dataIdHash,
                                                        context->client,
                                                        NULL);
                    }
                }
                break;
            case ieieICSS_AddUnreleasedMessageStates:
                if (rc == OK && context->info.UMSCount > 0)
                {
                    assert(context->UMSArray != NULL);

                    context->asyncId = pThreadData->asyncCounter++;
                    ieutTRACEL(pThreadData, context->asyncId, ENGINE_CEI_TRACE, FUNCTION_IDENT "ieieACId=0x%016lx\n",
                               __func__, context->asyncId);

                    rc = iecs_importUnreleasedDeliveryIds(pThreadData,
                                                          context->client,
                                                          context->UMSArray,
                                                          context->info.UMSCount,
                                                          ieie_storeAsyncDoImportClientState,
                                                          pContext);

                    iemem_free(pThreadData, iemem_exportResources, context->UMSArray);
                    context->UMSArray = NULL;
                }
                break;
            case ieieICSS_AddWillMessage:
                // Should only have will messages for seamless move or with delayed will msg publish.
                if (rc == OK && context->info.WillMsgId != 0)
                {
                    ismEngine_Message_t *message;

                    assert(context->willTopic != NULL);

                    rc = ieie_findImportedMessage(pThreadData,
                                                  control,
                                                  context->info.WillMsgId,
                                                  &message);

                    if (rc != OK)
                    {
                        ism_common_setError(rc);
                    }
                    else
                    {
                        assert(message != NULL);

                        rc = iecs_setWillMessage(pThreadData,
                                                 context->client,
                                                 context->willTopic,
                                                 message,
                                                 context->info.WillMsgTimeToLive,
                                                 context->info.WillDelay,
                                                 ppContext,
                                                 sizeof(ppContext),
                                                 ieie_asyncDoImportClientState);

                        if (rc != OK)
                        {
                            ism_common_setError(rc);
                        }

                        // We no longer need to hold a reference to this message
                        iem_releaseMessage(pThreadData, message);
                    }
                }
                break;
            case ieieICSS_Finish:
                // We failed - Report this.
                if (rc != OK)
                {
                    assert(rc != ISMRC_AsyncCompletion);

                    char humanIdentifier[strlen(context->clientId) + strlen("ClientID:")+1];
                    sprintf(humanIdentifier, "ClientID:%s", context->clientId);

                    ieie_recordImportError(pThreadData,
                                           control,
                                           ieieDATATYPE_EXPORTEDCLIENTSTATE,
                                           context->dataId,
                                           humanIdentifier,
                                           rc);
                }

                bool wentAsync = context->wentAsync;

                iemem_free(pThreadData, iemem_exportResources, context);

                if (wentAsync)
                {
                    ieie_finishImportRecord(pThreadData, control, ieieDATATYPE_EXPORTEDCLIENTSTATE);
                    (void)ieie_importTaskFinish(pThreadData, control, true, NULL);
                }

                goto mod_exit;
        }
    }

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "dataId=0x%0lx, rc=%d\n",
               __func__, dataId, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Async callback used to re-invoke ieie_doImportClientState
///
/// @param[in]     retcode        rc of the previous async operation
/// @param[in]     handle         handle from the previous async operation
/// @param[in]     pContext       ieieImportClientStateCallbackContext_t
//****************************************************************************
void ieie_asyncDoImportClientState(int32_t retcode,
                                   void *handle,
                                   void *pContext)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ieieImportClientStateCallbackContext_t *context = *(ieieImportClientStateCallbackContext_t **)pContext;

    context->wentAsync = true;

    (void)ieie_doImportClientState(pThreadData, retcode, handle, context);

    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @internal
///
/// @brief  Store async callback used to re-invoke ieie_doImportClientState
///
/// @param[in]     retcode        rc of the previous async operation
/// @param[in]     pContext       ieieImportClientStateCallbackContext_t
//****************************************************************************
void ieie_storeAsyncDoImportClientState(int retcode, void *pContext)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ieieImportClientStateCallbackContext_t *context = (ieieImportClientStateCallbackContext_t *)pContext;

    pThreadData->threadType = AsyncCallbackThreadType;

    context->wentAsync = true;

    ieutTRACEL(pThreadData, context->asyncId, ENGINE_CEI_TRACE, FUNCTION_IDENT "ieieACId=0x%016lx\n",
               __func__, context->asyncId);

    (void)ieie_doImportClientState(pThreadData, retcode, NULL, context);

    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @internal
///
/// @brief  Import a client state
///
/// @param[in]     control        ieieImportResourceControl_t for this import
/// @param[in]     dataId         dataId (the dataId for this client)
/// @param[in]     data           ieieClientStateInfo_t
/// @param[in]     dataLen        sizeof(ieieClientStateInfo_t plus data)
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_importClientState(ieutThreadData_t *pThreadData,
                               ieieImportResourceControl_t *control,
                               uint64_t dataId,
                               uint8_t *data,
                               size_t dataLen)
{
    int32_t rc = OK;
    ieieClientStateInfo_t *CSI = (ieieClientStateInfo_t *)data;
    size_t extraDataLen;

    ieutTRACEL(pThreadData, dataId, ENGINE_FNC_TRACE, FUNCTION_ENTRY "dataId=0x%0lx\n", __func__, dataId);

    // Cope with different versions of client
    if (CSI->Version == ieieCLIENTSTATE_CURRENT_VERSION)
    {
        extraDataLen = dataLen-sizeof(ieieClientStateInfo_t);
    }
    else if (CSI->Version == ieieCLIENTSTATE_VERSION_2)
    {
        extraDataLen = dataLen-sizeof(ieieClientStateInfo_V2_t);
    }
    else
    {
        assert(CSI->Version == ieieCLIENTSTATE_VERSION_1);
        extraDataLen = dataLen-sizeof(ieieClientStateInfo_V1_t);
    }

    ieieImportClientStateCallbackContext_t *context = iemem_malloc(pThreadData,
                                                                    IEMEM_PROBE(iemem_exportResources, 5),
                                                                    sizeof(ieieImportClientStateCallbackContext_t)+
                                                                    extraDataLen);

    if (context == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    context->stage = ieieICSS_Start;
    context->dataId = dataId;
    context->wentAsync = false;

    const char *extraData = (const char *)(&(context->info)+1);

    // Same version can just copy everything across
    if (CSI->Version == ieieCLIENTSTATE_CURRENT_VERSION)
    {
        memcpy(&context->info, data, dataLen);
    }
    // Other versions need to copy pieces across and initialize new fields.
    else if (CSI->Version == ieieCLIENTSTATE_VERSION_2)
    {
        memcpy(&context->info, data, sizeof(ieieClientStateInfo_V2_t));
        context->info.WillDelay = 0;
        if (context->info.WillMsgTimeToLive == 0)
        {
            context->info.WillMsgTimeToLive = iecsWILLMSG_TIME_TO_LIVE_INFINITE;
        }
        memcpy((uint8_t *)extraData, data+sizeof(ieieClientStateInfo_V2_t), extraDataLen);
    }
    else
    {
        assert(CSI->Version == ieieCLIENTSTATE_VERSION_1);

        memcpy(&context->info, data, sizeof(ieieClientStateInfo_V1_t));
        context->info.ExpiryInterval = iecsEXPIRY_INTERVAL_INFINITE;
        context->info.WillDelay = 0;
        if (context->info.WillMsgTimeToLive == 0)
        {
            context->info.WillMsgTimeToLive = iecsWILLMSG_TIME_TO_LIVE_INFINITE;
        }
        memcpy((uint8_t *)extraData, data+sizeof(ieieClientStateInfo_V1_t), extraDataLen);
    }

    size_t UMSArraySize = context->info.UMSCount * sizeof(uint32_t);
    if (UMSArraySize > 0)
    {
        context->UMSArray = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_exportResources, 13), UMSArraySize);
        if (context->UMSArray == NULL)
        {
            iemem_free(pThreadData, iemem_exportResources, context);
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    // ClientId
    assert(context->info.ClientIdLength != 0);
    context->clientId = extraData;
    extraData += context->info.ClientIdLength;

    // UserId
    if (context->info.UserIdLength != 0)
    {
        context->userId = extraData;
        extraData += context->info.UserIdLength;
    }
    else
    {
        context->userId = NULL;
    }

    // Will Topic
    if (context->info.WillTopicNameLength != 0)
    {
        context->willTopic = extraData;
        extraData += context->info.WillTopicNameLength;
    }
    else
    {
        context->willTopic = NULL;
    }

    if (UMSArraySize != 0)
    {
        assert(context->UMSArray != NULL);
        memcpy(context->UMSArray, extraData, UMSArraySize);
    }

    context->control = control;

    rc = ieie_doImportClientState(pThreadData, OK, NULL, context);

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of exportClientState.c                                        */
/*                                                                   */
/*********************************************************************/
