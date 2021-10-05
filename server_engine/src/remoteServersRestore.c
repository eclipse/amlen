/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
/// @file  remoteServersRestore.c
/// @brief Engine component remote server restore functions
//****************************************************************************
#define TRACE_COMP Engine

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>

#include "engineInternal.h"
#include "remoteServers.h"
#include "remoteServersInternal.h"
#include "queueCommon.h"         // ieq functions & constants
#include "memHandler.h"          // iemem functions & constants
#include "engineDump.h"          // iedm functions & constants
#include "policyInfo.h"          // iepi functions & constants
#include "engineStore.h"         // iest functions & constants
#include "engineRestore.h"

typedef struct tag_iersLocalRemoteServerInfo_t
{
    char                      strucId[4];          ///< Structure identifier 'ELRS'
    ismStore_Handle_t         hStoreDefn;          ///< Store handle of the RDR
    bool                      isInactive;          ///< Whether this record is actually flagged as deleted or mid-creation
} iersLocalServerInfo_t;

#define iersLOCALSERVERINFO_STRUCID "ELSI"

//****************************************************************************
/// @brief  Rehydrate a remote server definition which will result in the
///         creation of the high QoS queue for the remote server
///
/// @param[in]     recHandle         Store handle of the RDR
/// @param[in]     record            Pointer to the RDR
/// @param[in]     transData         Info about transaction (expect NULL)
/// @param[out]    rehydratedRecord  The queue that is created for this definition
///                                  (may be NULL for a local RDR)
/// @param[in]     pContext          Context (unused)
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The record must be consistent upon return.
//****************************************************************************
int32_t iers_rehydrateServerDefn(ieutThreadData_t *pThreadData,
                                 ismStore_Handle_t recHandle,
                                 ismStore_Record_t *record,
                                 ismEngine_RestartTransactionData_t *transData,
                                 void **rehydratedRecord,
                                 void *pContext)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, recHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Verify this is a remote server definition record
    assert(record->Type == ISM_STORE_RECTYPE_REMSRV);
    assert(transData == NULL);

    iestRemoteServerDefinitionRecord_t *pRDR = (iestRemoteServerDefinitionRecord_t *)(record->pFrags[0]);
    ismEngine_CheckStructId(pRDR->StrucId, iestREMSRV_DEFN_RECORD_STRUCID, ieutPROBE_001);

    bool isLocal;

    if (LIKELY(pRDR->Version == iestRDR_CURRENT_VERSION))
    {
        isLocal = pRDR->Local;
    }
    else
    {
        rc = ISMRC_InvalidValue;
        ism_common_setErrorData(rc, "%u", pRDR->Version);
        goto mod_exit;
    }

    // This is an RDR representing the local server - the object we store in this case is
    // a temporary structure which just holds the handle and information about it which is
    // then dealt with when the RPR is read
    if (isLocal)
    {
        iersLocalServerInfo_t *localServerInfo = iemem_malloc(pThreadData,
                                                              IEMEM_PROBE(iemem_remoteServers, 6),
                                                              sizeof(iersLocalServerInfo_t));

        if (localServerInfo == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        memcpy(localServerInfo->strucId, iersLOCALSERVERINFO_STRUCID, 4);
        localServerInfo->hStoreDefn = recHandle;
        localServerInfo->isInactive = (record->State & (iestRDR_STATE_DELETED | iestRDR_STATE_CREATING)) != 0;

        // Pass the localServerInfo back to the caller.
        *rehydratedRecord = localServerInfo;
    }
    // This is a normal RDR, representing a remote server so we need to create a queue
    else
    {
        ismQHandle_t hQueue;

        iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

        assert(remoteServersGlobal != NULL);
        assert(remoteServersGlobal->highQoSPolicyHandle != NULL);

        ieutTRACEL(pThreadData, pRDR, ENGINE_HIFREQ_FNC_TRACE, "Found non-local RDR.\n");

        // Create the queue
        iepi_acquirePolicyInfoReference(remoteServersGlobal->highQoSPolicyHandle);
        rc = ieq_createQ(pThreadData,
                         multiConsumer,
                         NULL, // Update name when RPR rehydrated
                         ieqOptions_REMOTE_SERVER_QUEUE |
                         ieqOptions_SINGLE_CONSUMER_ONLY |
                         ieqOptions_IN_RECOVERY,
                         remoteServersGlobal->highQoSPolicyHandle,
                         recHandle,
                         ismSTORE_NULL_HANDLE, // Update handle when RPR rehydrated
                         iereNO_RESOURCE_SET,
                         &hQueue);

        if (rc != OK) goto mod_exit;

       // If the RDR is for a remote server that is deleted or is still mid-creation, mark
       // the queue as deleted so that it can be deleted during cleanup / when outstanding
       // transactions complete.
       if (record->State & (iestRDR_STATE_DELETED | iestRDR_STATE_CREATING))
       {
           ieutTRACEL(pThreadData, hQueue, ENGINE_FNC_TRACE, "RDR found with state 0x%016lx for queue %p\n",
                      record->State, hQueue);

           rc = ieq_markQDeleted(pThreadData, hQueue, false);

           if (rc != OK) goto mod_exit;
       }

       // Pass the queue handle back to the caller.
       *rehydratedRecord = hQueue;
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

//****************************************************************************
/// @brief  Rehydrate a remote server properties record read from the store
///
/// Recreates a remote server from the store.
///
/// @param[in]     recHandle         Store handle of the RPR
/// @param[in]     record            Pointer to the RPR
/// @param[in]     transData         info about transaction (expect NULL)
/// @param[in]     requestingRecord  Either the queue, or iersLocalRemoteServer
///                                  created for the RDR (queue) that requested this RPR
/// @param[out]    rehydratedRecord  Pointer to data created from this record if
///                                  the recovery code needs to track it
///                                  (unused for Remote servers)
/// @param[in]     pContext          Context (unused)
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The record must be consistent upon return.
//****************************************************************************
int32_t iers_rehydrateServerProps(ieutThreadData_t *pThreadData,
                                  ismStore_Handle_t recHandle,
                                  ismStore_Record_t *record,
                                  ismEngine_RestartTransactionData_t *transData,
                                  void *requestingRecord,
                                  void **rehydratedRecord,
                                  void *pContext)
{
    int32_t rc = OK;
    iestRemoteServerPropertiesRecord_t *pRPR;
    ismEngine_RemoteServer_t *newServer = NULL;

    ieutTRACEL(pThreadData, recHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Verify this is a subscription properties record
    assert(record->Type == ISM_STORE_RECTYPE_RPROP);
    assert(record->FragsCount == 1);
    assert(requestingRecord != NULL);

    pRPR = (iestRemoteServerPropertiesRecord_t *)(record->pFrags[0]);
    ismEngine_CheckStructId(pRPR->StrucId, iestREMSRV_PROPS_RECORD_STRUCID, ieutPROBE_001);

    uint32_t internalAttrs;
    size_t uidLength;
    size_t nameLength;
    size_t clusterDataLength;
    char *tmpPtr;

    if (LIKELY(pRPR->Version == iestRPR_CURRENT_VERSION))
    {
        internalAttrs = pRPR->InternalAttrs;
        uidLength = pRPR->UIDLength;
        nameLength = pRPR->NameLength;
        clusterDataLength = pRPR->ClusterDataLength;

        tmpPtr = (char *)(pRPR+1);
    }
    else
    {
        rc = ISMRC_InvalidValue;
        ism_common_setErrorData(rc, "%u", pRPR->Version);
        goto mod_exit;
    }

    ieutTRACEL(pThreadData, pRPR->Version, ENGINE_HIFREQ_FNC_TRACE, "Found Version %u RPR.\n", pRPR->Version);

    assert(uidLength != 0);
    assert(nameLength != 0);

    char *serverUID = tmpPtr;
    tmpPtr += uidLength;

    char *serverName = tmpPtr;
    tmpPtr += nameLength;

    char *clusterData;

    if (clusterDataLength != 0)
    {
        clusterData = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_remoteServers, 15), clusterDataLength);

        if (clusterData == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        memcpy(clusterData, tmpPtr, clusterDataLength);
    }
    else
    {
        clusterData = NULL;
    }
    tmpPtr += clusterDataLength;

    newServer = iemem_calloc(pThreadData,
                             IEMEM_PROBE(iemem_remoteServers, 7),
                             1, sizeof(ismEngine_RemoteServer_t)+uidLength);

    if (newServer == NULL)
    {
        iemem_free(pThreadData, iemem_remoteServers, clusterData);
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    newServer->serverName = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_remoteServers, 14), nameLength);

    if (newServer->serverName == NULL)
    {
        iemem_free(pThreadData, iemem_remoteServers, clusterData);
        iemem_freeStruct(pThreadData, iemem_remoteServers, newServer, newServer->StrucId);
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(newServer->StrucId, ismENGINE_REMOTESERVER_STRUCID, 4);
    newServer->serverUID = (char *)(newServer+1);
    strcpy(newServer->serverUID, serverUID);
    strcpy(newServer->serverName, serverName);
    newServer->useCount = 1;
    newServer->internalAttrs = internalAttrs | iersREMSRVATTR_DISCONNECTED;
    newServer->clusterDataLength = clusterDataLength;
    newServer->clusterData = clusterData;

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    // NOTE: Not taking the lock to add to the global list because we are single
    //       threaded during recovery

    if (remoteServersGlobal->serverHead != NULL)
    {
        remoteServersGlobal->serverHead->prev = newServer;
    }

    newServer->next = remoteServersGlobal->serverHead;

    remoteServersGlobal->serverHead = newServer;

    remoteServersGlobal->serverCount += 1;

    // Local remote server definition
    if (newServer->internalAttrs & iersREMSRVATTR_LOCAL)
    {
        iersLocalServerInfo_t *localServerInfo = (iersLocalServerInfo_t *)requestingRecord;

        ismEngine_CheckStructId(localServerInfo->strucId, iersLOCALSERVERINFO_STRUCID, ieutPROBE_001);

        newServer->hStoreDefn = localServerInfo->hStoreDefn;
        newServer->hStoreProps = recHandle;

        // If the server is deemed inactive, mark it as deleted.
        if (localServerInfo->isInactive)
        {
            newServer->internalAttrs |= iersREMSRVATTR_DELETED;
        }
        else if (ismEngine_serverGlobal.clusterEnabled == false)
        {
            newServer->internalAttrs |= iersREMSRVATTR_UNCLUSTERED;
        }
    }
    else
    {
        remoteServersGlobal->remoteServerCount += 1; // This is not the local server, so we need to count it.

        newServer->highQoSQueueHandle = (ismQHandle_t)requestingRecord;
        newServer->hStoreDefn = ieq_getDefnHdl(newServer->highQoSQueueHandle);
        newServer->hStoreProps = recHandle;

        // Should not already have any properties handle associated with this queue
        assert(ieq_getPropsHdl(newServer->highQoSQueueHandle) == ismSTORE_NULL_HANDLE);

        // Need to update the existing queue with information about the remote server
        // and to supply it's properties handle
        ieq_updateProperties(pThreadData,
                             newServer->highQoSQueueHandle,
                             newServer->serverUID,
                             ieqOptions_REMOTE_SERVER_QUEUE | ieqOptions_IN_RECOVERY,
                             recHandle,
                             iereNO_RESOURCE_SET);

        // If the queue is marked as deleted, mark the remote server record as deleted too
        if (ieq_isDeleted(newServer->highQoSQueueHandle))
        {
            newServer->internalAttrs |= iersREMSRVATTR_DELETED;
        }
        else if (ismEngine_serverGlobal.clusterEnabled == false)
        {
            newServer->internalAttrs |= iersREMSRVATTR_UNCLUSTERED;
        }
        // Not a deleted remote server, so we need a lowQoS queue too.
        else
        {
            iepi_acquirePolicyInfoReference(remoteServersGlobal->lowQoSPolicyHandle);
            rc = ieq_createQ(pThreadData,
                             multiConsumer,
                             newServer->serverUID,
                             ieqOptions_REMOTE_SERVER_QUEUE | ieqOptions_SINGLE_CONSUMER_ONLY,
                             remoteServersGlobal->lowQoSPolicyHandle,
                             ismSTORE_NULL_HANDLE,
                             ismSTORE_NULL_HANDLE,
                             iereNO_RESOURCE_SET,
                             &newServer->lowQoSQueueHandle);

            if (rc != OK) goto mod_exit;
        }
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

    ieutTRACEL(pThreadData, newServer, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, remoteServer=%p\n", __func__,
               rc, newServer);

    return rc;
}

//****************************************************************************
/// @brief Reconcile the list of remote servers we have with the cluster
///        component (and tidy up anything that we can)
///
/// @param[in]     pThreadData       Thread data
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_reconcileEngineRemoteServers(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    ieutTRACEL(pThreadData, remoteServersGlobal, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    ismEngine_RemoteServer_t *currentServer = remoteServersGlobal->serverHead;

    uint32_t storeOperations = 0;
    int32_t clusterServerCount = 0;
    ismCluster_RemoteServerData_t clusterServers[100];

    // Go through the remote servers found at restart and either delete them or reconcile
    // them with the cluster component (to receive their cluster handle)
    while(currentServer)
    {
        bool isLocalServer = (currentServer->internalAttrs & iersREMSRVATTR_LOCAL) == iersREMSRVATTR_LOCAL;
        ismEngine_RemoteServer_t *nextServer = currentServer->next;

        ismEngine_CheckStructId(currentServer->StrucId, ismENGINE_REMOTESERVER_STRUCID, ieutPROBE_001);

        // This is a deleted / not fully created server or we are unclustered - free up
        if (currentServer->internalAttrs & (iersREMSRVATTR_DELETED | iersREMSRVATTR_UNCLUSTERED))
        {
            ieutTRACEL(pThreadData, currentServer, ENGINE_FNC_TRACE,
                       "Deleting remoteServer serverName='%s', serverUID='%s', internalAttrs=0x%08x.\n",
                       currentServer->serverName, currentServer->serverUID, currentServer->internalAttrs);

            // Remove from the global list...
            // NOTE: Not taking the lock to do this as we are still single threaded

            if (currentServer->prev != NULL)
            {
                currentServer->prev->next = currentServer->next;
            }
            else
            {
                assert(remoteServersGlobal->serverHead == currentServer);
                remoteServersGlobal->serverHead = currentServer->next;
            }

            if (currentServer->next != NULL)
            {
                currentServer->next->prev = currentServer->prev;
            }

            remoteServersGlobal->serverCount -= 1;

            // If this is a local server, we need to delete the store records
            if (isLocalServer)
            {
                if (LIKELY(currentServer->hStoreDefn != ismSTORE_NULL_HANDLE))
                {
                    rc = ism_store_deleteRecord(pThreadData->hStream, currentServer->hStoreDefn);
                    if (rc != OK)
                    {
                        ism_common_setError(rc);
                        goto mod_exit;
                    }
                    storeOperations++;
                }

                if (LIKELY(currentServer->hStoreProps != ismSTORE_NULL_HANDLE))
                {
                    rc = ism_store_deleteRecord(pThreadData->hStream, currentServer->hStoreProps);
                    if (rc != OK)
                    {
                        ism_common_setError(rc);
                        goto mod_exit;
                    }
                    storeOperations++;
                }
            }
            // For a remote server, the highQoS queue will remove the store records
            // when it is able to do so - if we're discovering this because we've become
            // unclustered we must mark the queue deleted (so that it doesn't come back again)
            else
            {
                assert(currentServer->lowQoSQueueHandle == NULL);
                assert(currentServer->highQoSQueueHandle != NULL);

                if ((currentServer->internalAttrs & iersREMSRVATTR_DELETED) != 0)
                {
                    assert(ieq_isDeleted(currentServer->highQoSQueueHandle) == true);
                }
                else
                {
                    assert((currentServer->internalAttrs & iersREMSRVATTR_UNCLUSTERED) != 0);
                    assert(ieq_isDeleted(currentServer->highQoSQueueHandle) == false);

                    rc = ieq_markQDeleted(pThreadData, currentServer->highQoSQueueHandle, true);
                    if (rc != OK)
                    {
                        ism_common_setError(rc);
                        goto mod_exit;
                    }
                    
                    // Marking the queue deleted will perform a store commit
                    storeOperations = 0;
                }

                remoteServersGlobal->remoteServerCount -= 1;
            }

            // Clean up any cluster data that we read in
            iemem_free(pThreadData, iemem_remoteServers, currentServer->clusterData);
            iemem_free(pThreadData, iemem_remoteServers, currentServer->serverName);

            iemem_freeStruct(pThreadData, iemem_remoteServers, currentServer, currentServer->StrucId);
        }
        // Add this one to the array to call the cluster component with
        else
        {
            clusterServers[clusterServerCount].fLocalServer = (uint8_t)isLocalServer;
            clusterServers[clusterServerCount].hServerHandle = currentServer;
            clusterServers[clusterServerCount].pRemoteServerUID = currentServer->serverUID;
            clusterServers[clusterServerCount].pRemoteServerName = currentServer->serverName;
            clusterServers[clusterServerCount].pData = currentServer->clusterData;
            clusterServers[clusterServerCount].dataLength = (uint32_t)(currentServer->clusterDataLength);
            clusterServers[clusterServerCount].phClusterHandle = &currentServer->clusterHandle;
            clusterServerCount++;
        }

        // Call the cluster component with the remote servers we have if we've filled
        // the array, or this is the last server and there are some in the array
        if ((clusterServerCount == sizeof(clusterServers)/sizeof(clusterServers[0])) ||
            (clusterServerCount != 0 && nextServer == NULL))
        {
            if (ismEngine_serverGlobal.clusterEnabled)
            {
                rc = ism_cluster_restoreRemoteServers(clusterServers, clusterServerCount);

                if (rc != OK)
                {
                    ism_common_setError(rc);
                    goto mod_exit;
                }
            }

            // Tidy up the set we just sent
            do
            {
                currentServer = clusterServers[clusterServerCount-1].hServerHandle;

                iemem_free(pThreadData, iemem_remoteServers, currentServer->clusterData);

                // Make sure a subsequent update doesn't pick this data up
                currentServer->clusterData = NULL;
                currentServer->clusterDataLength = 0;
            }
            while(--clusterServerCount > 0);

            assert(clusterServerCount == 0);
        }

        currentServer = nextServer;
    }

mod_exit:

    // Commit or roll back any store operations we just performed
    if (storeOperations != 0)
    {
        if (rc == OK)
        {
            iest_store_commit(pThreadData, false);
        }
        else
        {
            iest_store_rollback(pThreadData, false);
        }
    }

    ieutTRACEL(pThreadData, remoteServersGlobal, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Tell the cluster component that recovery is now completed
///
/// @param[in]     pThreadData       Thread data
///
/// @remark After this point in time, the cluster component will start to
///         call the engine back to perform cluster related actions.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_declareRecoveryCompleted(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, ismEngine_serverGlobal.clusterEnabled, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (ismEngine_serverGlobal.clusterEnabled)
    {
        iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

        #ifdef NO_MALLOC_WRAPPER
        assert(false); // We rely on the malloc wrapper for memory reporting and forwarding queue limitation
        #else
        // Impose limits on the forwarding queue, and set up a memHandler callback to do the
        // same later.
        iemem_systemMemInfo_t initialSysMemInfo = {0};

        rc = iemem_querySystemMemory(&initialSysMemInfo);

        // If we couldn't get initial system memory info, just continue
        if (rc != OK)
        {
            ieutTRACEL(pThreadData, rc, ENGINE_ERROR_TRACE, "iemem_querySystemMemory rc=%d. Skipping initial clustering memory analysis", rc);
            ism_common_setError(rc);
            rc = OK;
        }
        else
        {
            remoteServersGlobal->reservedForwardingBytes = (initialSysMemInfo.effectiveMemoryBytes * iersRESERVE_FORWARDING_BYTES_PERCENT) / 100;
            iers_analyseMemoryUsage(iememDisableAll, iememDisableAll, iemem_remoteServers, 0, &initialSysMemInfo);
        }

        iemem_setMemoryReduceCallback(iemem_remoteServers, iers_analyseMemoryUsage);
        #endif

        rc = ism_cluster_recoveryCompleted();

        if (rc != OK) ism_common_setError(rc);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}
