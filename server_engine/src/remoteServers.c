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
/// @file  remoteServers.c
/// @brief Engine component remote server manipulation functions
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
#include "topicTree.h"           // iett functions & constants
#include "topicTreeInternal.h"   // iett_analyseTopicString
#include "engineMonitoring.h"    // iemn functions & constants
#include "msgCommon.h"           // iem functions

// Forward declaration of freeServer function
void iers_freeServer(ieutThreadData_t *pThreadData,
                     ismEngine_RemoteServer_t *server,
                     bool freeOnly);

//We need to ensure we are not in the midst of a callback when we stop it
//this counter is used to do that.
static uint32_t clusterCallbackActiveUseCount = 0;

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Policy info used for low QoS forwarding queues (initial values)
///////////////////////////////////////////////////////////////////////////////
static iepiPolicyInfo_t iersLowQoSPolicyInfo_DEFAULT =
{
    iepiPOLICY_INFO_STRUCID_ARRAY,
    1,                                       // useCount
    NULL,                                    // name
    0,                                       // maxMessageCount (unlimited)
    0,                                       // maxMessageBytes (unlimited)
    ismMAXIMUM_MESSAGE_TIME_TO_LIVE_DEFAULT, // maxMessageTimeToLive
    true,                                    // concurrentConsumers
    true,                                    // allowSend
    false,                                   // DCNEnabled
    DiscardOldMessages,                      // maxMsgBehavior
    NULL,                                    // defaultSelectionInfo
    CreatedByEngine,                         // creation state
    ismSEC_POLICY_LAST,                      // policy type
};

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Policy info used for high QoS forwarding queues (initial values)
///////////////////////////////////////////////////////////////////////////////
static iepiPolicyInfo_t iersHighQoSPolicyInfo_DEFAULT =
{
    iepiPOLICY_INFO_STRUCID_ARRAY,
    1,                                       // useCount
    NULL,                                    // name
    0,                                       // maxMessageCount (unlimited)
    0,                                       // maxMessageBytes (unlimited)
    ismMAXIMUM_MESSAGE_TIME_TO_LIVE_DEFAULT, // maxMessageTimeToLive
    true,                                    // concurrentConsumers
    true,                                    // allowSend
    false,                                   // DCNEnabled
    DiscardOldMessages,                      // maxMsgBehavior
    NULL,                                    // defaultSelectionInfo
    CreatedByEngine,                         // creation state
    ismSEC_POLICY_LAST,                      // policy type
};

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Policy info used for forwarding queues while seeding a newly created
///    remote server (rejects new messages)
///////////////////////////////////////////////////////////////////////////////
static iepiPolicyInfo_t iersSeedingPolicyInfo_DEFAULT =
{
    iepiPOLICY_INFO_STRUCID_ARRAY,
    1,                                       // useCount
    NULL,                                    // name
    0,                                       // maxMessageCount (unlimited)
    0,                                       // maxMessageBytes (unlimited)
    ismMAXIMUM_MESSAGE_TIME_TO_LIVE_DEFAULT, // maxMessageTimeToLive
    true,                                    // concurrentConsumers
    true,                                    // allowSend
    false,                                   // DCNEnabled
    RejectNewMessages,                       // maxMsgBehavior
    NULL,                                    // defaultSelectionInfo
    CreatedByEngine,                         // creation state
    ismSEC_POLICY_LAST,                      // policy type
};

//****************************************************************************
/// @brief Create and initialize the global engine remote server info
///
/// @remark Will replace the existing global info without checking
///         that this has happened.
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @see iers_destroyEngineRemoteServers
//****************************************************************************
int32_t iers_initEngineRemoteServers(ieutThreadData_t *pThreadData)
{
    int osrc = 0;
    int32_t rc = OK;
    iersRemoteServers_t *remoteServersGlobal = NULL;
    pthread_rwlockattr_t rwlockattr_init;

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    osrc = pthread_rwlockattr_init(&rwlockattr_init);

    if (osrc) goto mod_exit;

    osrc = pthread_rwlockattr_setkind_np(&rwlockattr_init, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);

    if (osrc) goto mod_exit;

    // Create the remote server root structure
    remoteServersGlobal = iemem_calloc(pThreadData,
                                       IEMEM_PROBE(iemem_remoteServers, 1), 1,
                                       sizeof(iersRemoteServers_t));

    if (NULL == remoteServersGlobal)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(remoteServersGlobal->strucId, iersREMOTESERVERS_STRUCID, 4);

    // Initialize the policy info's we'll use for high and low QoS forwarding queues
    rc = iepi_copyPolicyInfo(pThreadData,
                             &iersHighQoSPolicyInfo_DEFAULT,
                             "HighQoSPolicy",
                             &remoteServersGlobal->highQoSPolicyHandle);

    if (rc != OK) goto mod_exit;

    rc = iepi_copyPolicyInfo(pThreadData,
                             &iersLowQoSPolicyInfo_DEFAULT,
                             "LowQoSPolicy",
                             &remoteServersGlobal->lowQoSPolicyHandle);

    if (rc != OK) goto mod_exit;

    rc = iepi_copyPolicyInfo(pThreadData,
                             &iersSeedingPolicyInfo_DEFAULT,
                             "SeedingPolicy",
                             &remoteServersGlobal->seedingPolicyHandle);

    if (rc != OK) goto mod_exit;

    osrc = pthread_rwlock_init(&remoteServersGlobal->listLock, &rwlockattr_init);

    if (osrc) goto mod_exit;

    rc = ieut_createHashTable(pThreadData,
                              100,
                              iemem_remoteServers,
                              &remoteServersGlobal->outOfSyncServers);

    if (rc != OK) goto mod_exit;

    osrc = pthread_spin_init(&remoteServersGlobal->outOfSyncLock, PTHREAD_PROCESS_PRIVATE);

    if (osrc) goto mod_exit;

    // Initialize the callback use count to 1 (it is decremented when the callbacks are stopped)
    clusterCallbackActiveUseCount = 1;

    remoteServersGlobal->currentHealthStatus = ISM_CLUSTER_HEALTH_UNKNOWN;

    // Other fields initialized to 0 / NULL by the use of calloc

mod_exit:

    if (osrc)
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);
    }

    if (rc == OK)
    {
        assert(remoteServersGlobal != NULL);
        ismEngine_serverGlobal.remoteServers = remoteServersGlobal;
    }

    ieutTRACEL(pThreadData, remoteServersGlobal,  ENGINE_FNC_TRACE, FUNCTION_EXIT "remoteServersGlobal=%p, rc=%d\n",
               __func__, remoteServersGlobal, rc);

    return rc;
}

//****************************************************************************
/// @brief Destroy and remove the global engine remote server information
///
/// @remark This will result in all remote server information being discarded.
///
/// @see iers_initEngineRemoteServers
//****************************************************************************
void iers_destroyEngineRemoteServers(ieutThreadData_t *pThreadData)
{
    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    ieutTRACEL(pThreadData, remoteServersGlobal, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (NULL != remoteServersGlobal)
    {
        ismEngine_RemoteServer_t *currentServer = remoteServersGlobal->serverHead;

        while(currentServer != NULL)
        {
            ismEngine_RemoteServer_t *nextServer = currentServer->next;

            iers_freeServer(pThreadData, currentServer, true);

            currentServer = nextServer;
        }

        (void)pthread_rwlock_destroy(&remoteServersGlobal->listLock);

        ieut_destroyHashTable(pThreadData, remoteServersGlobal->outOfSyncServers);

        (void)pthread_spin_destroy(&remoteServersGlobal->outOfSyncLock);

        // No longer need references to the policies created during initialization
        iepi_releasePolicyInfo(pThreadData, remoteServersGlobal->seedingPolicyHandle);
        iepi_releasePolicyInfo(pThreadData, remoteServersGlobal->lowQoSPolicyHandle);
        iepi_releasePolicyInfo(pThreadData, remoteServersGlobal->highQoSPolicyHandle);

        iemem_freeStruct(pThreadData, iemem_remoteServers, remoteServersGlobal, remoteServersGlobal->strucId);

        ismEngine_serverGlobal.remoteServers = NULL;
    }

    ieutTRACEL(pThreadData, 0, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);
}

//****************************************************************************
/// @brief Put all the retained messages originating at a given serverUID on
///        the forwarding queue(s) for a given target server
///
/// @param[in]   serverUID      The UID of the retained message origin server
/// @param[in]   options        Options specified by the requesting server
/// @param[in]   timestamp      Timestamp specified by the requesting server to use
///                             in selecting retained messages.
/// @param[in]   targetServer   Server to put the messages to
/// @param[in]   seeding        Whether this is an initial seeding request
///
/// @remark If the serverUID is NULL, this means we want to put all retained
///         messages whose originServer is flagged with localServer (which means
///         it is now, or once was the UID of this server).
///
/// @remark The useCount for the targetServer is expected to be held by the
///         caller.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static inline int32_t iers_putAllRetained(ieutThreadData_t *pThreadData,
                                          const char *serverUID,
                                          uint32_t options,
                                          ism_time_t timestamp,
                                          ismEngine_RemoteServer_t *targetServer,
                                          bool seeding)
{
    int32_t rc;
    ismEngine_MessageHandle_t *foundMessages;
    uint32_t foundMessageCount;

    ieutTRACEL(pThreadData, targetServer, ENGINE_FNC_TRACE, FUNCTION_ENTRY "serverUID=%s targetServer=%p targetServerUID=%s\n",
               __func__, serverUID ? serverUID : "NULL", targetServer, targetServer->serverUID);

    rc = iett_findOriginServerRetainedMessages(pThreadData,
                                               serverUID,
                                               options,
                                               timestamp,
                                               &foundMessages,
                                               &foundMessageCount);

    if (seeding == true)
    {
        iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

        // We decrement the seedingCount _before_ putting the messages which means that
        // new retained messages are put to the queues potentially before these old retained
        // messages - but the timestamp check at the receiving end will mean the old retained
        // will just be ignored.
        ismEngine_getRWLockForWrite(&(remoteServersGlobal->listLock));
        remoteServersGlobal->seedingCount -= 1;
        ismEngine_unlockRWLock(&(remoteServersGlobal->listLock));
    }

    // Not found simply means we have no messages to put on the queue
    if (rc == ISMRC_NotFound)
    {
        rc = OK;
    }
    else
    {
        if (rc != OK) goto mod_exit;

        // If we found some messages, put them to the queue
        if (foundMessageCount != 0)
        {
            assert(foundMessages != NULL);

            ismEngine_Transaction_t *pTran = NULL;

            rc = ietr_createLocal(pThreadData, NULL, true, true, NULL, &pTran);

            if (rc == OK)
            {
                assert(pTran != NULL);

                rc = ietr_reserve(pThreadData, pTran, 0, foundMessageCount);
                assert(rc == OK);

                uint32_t i = 0;
                ismEngine_Message_t *pMessage;

                for(; i<foundMessageCount; i++)
                {
                    pMessage = (ismEngine_Message_t *)foundMessages[i];

                    ismQHandle_t queueHandle = (pMessage->Header.Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE) ?
                                               targetServer->lowQoSQueueHandle :
                                               targetServer->highQoSQueueHandle;

                    // Should never be any default selection on the policies used by remote server queues
                    assert(((iepiPolicyInfo_t *)ieq_getPolicyInfo(queueHandle))->defaultSelectionInfo == NULL);

                    rc = ieq_put(pThreadData,
                                 queueHandle,
                                 ieqPutOptions_NONE, // RETAINED?
                                 pTran,
                                 pMessage,
                                 IEQ_MSGTYPE_INHERIT, // already incremented
                                 NULL );

                    if (rc != OK) break;
                }

                if (rc != OK)
                {
                    // Release any remaining messages
                    for(;i<foundMessageCount; i++)
                    {
                        pMessage = foundMessages[i];
                        iem_releaseMessage(pThreadData, pMessage);
                    }

                    // Couldn't add all the retained messages during seeding. We allow the
                    // creation to complete and rely on resynchronization to pull missing retained
                    // messages later.
                    if (seeding == true && rc == ISMRC_DestinationFull)
                    {
                        ieutTRACEL(pThreadData, foundMessageCount, ENGINE_INTERESTING_TRACE,
                                   "Skipping retained messages that could not fit for server '%s'\n",
                                   targetServer->serverUID);
                        rc = OK;
                    }
                }

                if (rc == OK)
                {
                    assert(i == foundMessageCount);

                    rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT,
                                     NULL, NULL, NULL);
                }
                else
                {
                    DEBUG_ONLY int32_t rc2 = ietr_rollback(pThreadData, pTran, NULL, IETR_ROLLBACK_OPTIONS_NONE);
                    assert(rc2 == OK);  // Rollback should never fail
                }
            }

            // Release the array of messages (dropping use counts and store ref counts)
            iett_releaseOriginServerRetainedMessages(pThreadData, foundMessages);
        }
    }

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, targetServer=%p\n",
               __func__, rc, targetServer);

    return rc;
}

//****************************************************************************
/// @brief Create a remote server (handle ENGINE_RS_CREATE event)
///
/// @param[in]     hClusterHandle     Cluster component's handle for this server
/// @param[in]     pServerName        NULL terminated server Name
/// @param[in]     pServerUID         NULL terminated server UID
/// @param[in]     local              Whether this is for the local server
/// @param[out]    phServerHandle     Returned server handle
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_createServer(ieutThreadData_t *pThreadData,
                          ismCluster_RemoteServerHandle_t hClusterHandle,
                          const char *pServerName,
                          const char *pServerUID,
                          bool local,
                          ismEngine_RemoteServerHandle_t *phServerHandle)
{
    int32_t rc = OK;
    bool serverAdded = false;
    bool seedingServer = false;

    ieutTRACEL(pThreadData, hClusterHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "clusterHandle=%p, serverName='%s', serverUID='%s'\n",
               __func__, hClusterHandle, pServerName, pServerUID);

    size_t uidLength = strlen(pServerUID)+1;
    size_t structureSize = sizeof(ismEngine_RemoteServer_t)+uidLength;

    ismEngine_RemoteServer_t *newServer = iemem_calloc(pThreadData,
                                                       IEMEM_PROBE(iemem_remoteServers, 2), 1,
                                                       structureSize);

    if (newServer == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    char *newServerName = iemem_malloc(pThreadData,
                                       IEMEM_PROBE(iemem_remoteServers, 12),
                                       strlen(pServerName)+1);

    if (newServerName == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    uint32_t newInternalAttrs = iersREMSRVATTR_DISCONNECTED | iersREMSRVATTR_CREATING;

    if (local)
    {
        newInternalAttrs |= iersREMSRVATTR_LOCAL;
    }

    // Fill in the structure
    memcpy(newServer->StrucId, ismENGINE_REMOTESERVER_STRUCID, 4);
    newServer->serverUID = (char *)(newServer+1);
    strcpy(newServer->serverUID, pServerUID);
    newServer->serverName = (char *)newServerName;
    strcpy(newServer->serverName, pServerName);
    newServer->clusterHandle = hClusterHandle;
    newServer->useCount = 1;
    newServer->internalAttrs = newInternalAttrs;

    rc = iest_storeRemoteServer(pThreadData,
                                newServer,
                                &newServer->hStoreDefn,
                                &newServer->hStoreProps);

    if (rc != OK) goto mod_exit;

    assert((newServer->hStoreDefn != ismSTORE_NULL_HANDLE) &&
           (newServer->hStoreProps != ismSTORE_NULL_HANDLE));

    iest_store_commit(pThreadData, false);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    // For normal remote server definitions (i.e. not local) create queues
    if ((newServer->internalAttrs & iersREMSRVATTR_LOCAL) == 0)
    {
        // Create a queue for high QoS messages
        iepi_acquirePolicyInfoReference(remoteServersGlobal->seedingPolicyHandle);
        rc = ieq_createQ(pThreadData,
                         multiConsumer,
                         pServerUID,
                         ieqOptions_REMOTE_SERVER_QUEUE |
                         ieqOptions_SINGLE_CONSUMER_ONLY,
                         remoteServersGlobal->seedingPolicyHandle,
                         newServer->hStoreDefn,
                         newServer->hStoreProps,
                         iereNO_RESOURCE_SET,
                         &newServer->highQoSQueueHandle);

        if (rc != OK) goto mod_exit;

        // Create a queue for low QoS messages
        iepi_acquirePolicyInfoReference(remoteServersGlobal->seedingPolicyHandle);
        rc = ieq_createQ(pThreadData,
                         multiConsumer,
                         pServerUID,
                         ieqOptions_REMOTE_SERVER_QUEUE |
                         ieqOptions_SINGLE_CONSUMER_ONLY,
                         remoteServersGlobal->seedingPolicyHandle,
                         ismSTORE_NULL_HANDLE, ismSTORE_NULL_HANDLE,
                         iereNO_RESOURCE_SET,
                         &newServer->lowQoSQueueHandle);

        if (rc != OK) goto mod_exit;
    }

    // Add the server to the list
    assert((newServer->internalAttrs & iersREMSRVATTR_LOCAL) != 0 || newServer->lowQoSQueueHandle != NULL);
    assert((newServer->internalAttrs & iersREMSRVATTR_LOCAL) != 0 || newServer->highQoSQueueHandle != NULL);
    assert((newServer->internalAttrs & iersREMSRVATTR_CREATING) == iersREMSRVATTR_CREATING);

    ismEngine_getRWLockForWrite(&(remoteServersGlobal->listLock));

    if (remoteServersGlobal->serverHead != NULL)
    {
        remoteServersGlobal->serverHead->prev = newServer;
    }

    newServer->next = remoteServersGlobal->serverHead;

    remoteServersGlobal->serverHead = newServer;

    remoteServersGlobal->serverCount += 1;

    // We don't count the local server record in the count of remote servers
    // and we don't attempt to seed it with retained messages
    if ((newServer->internalAttrs & iersREMSRVATTR_LOCAL) == 0)
    {
        remoteServersGlobal->remoteServerCount += 1;
        remoteServersGlobal->seedingCount += 1;
        seedingServer = true;
    }

    ismEngine_unlockRWLock(&(remoteServersGlobal->listLock));

    serverAdded = true;

    // We need to seed retained messages to this remote server
    if (seedingServer == true)
    {
        rc = iers_putAllRetained(pThreadData,
                                 NULL, // No UID specified (all flagged localServer)
                                 ismENGINE_FORWARD_RETAINED_OPTION_NONE,
                                 (ism_time_t)0,
                                 newServer,
                                 true);

        if (rc != OK) goto mod_exit;

        // Switch over to the standard policyInfo structures for the two queues
        DEBUG_ONLY bool changed;
        iepi_acquirePolicyInfoReference(remoteServersGlobal->lowQoSPolicyHandle);
        changed = ieq_setPolicyInfo(pThreadData,
                                    newServer->lowQoSQueueHandle,
                                    remoteServersGlobal->lowQoSPolicyHandle);
        assert(changed == true);
        iepi_acquirePolicyInfoReference(remoteServersGlobal->highQoSPolicyHandle);
        changed = ieq_setPolicyInfo(pThreadData,
                                    newServer->highQoSQueueHandle,
                                    remoteServersGlobal->highQoSPolicyHandle);
        assert(changed == true);
    }

    // Everything worked, so mark the store record so that we will not delete
    // this record at startup and will report it to the cluster component.
    rc = ism_store_updateRecord(pThreadData->hStream,
                                newServer->hStoreDefn,
                                ismSTORE_NULL_HANDLE,
                                iestRDR_STATE_NONE,
                                ismSTORE_SET_STATE);

    if (rc == OK)
    {
        iest_store_commit(pThreadData, false);
    }
    else
    {
        assert(rc != ISMRC_StoreGenerationFull);
        ism_common_setError(rc);
        goto mod_exit;
    }

mod_exit:

    // If everything worked return server handle
    if (rc == OK)
    {
        assert(serverAdded == true);

        // unset the internal flag indicating we are mid-creation
        newServer->internalAttrs &= ~iersREMSRVATTR_CREATING;

        *phServerHandle = newServer;
    }
    // Remove the server if we added it
    else if (serverAdded == true)
    {
        ismEngine_getRWLockForWrite(&(remoteServersGlobal->listLock));

        if (newServer->next != NULL)
        {
            newServer->next->prev = newServer->prev;
        }

        if (newServer->prev != NULL)
        {
            newServer->prev->next = newServer->next;
        }
        else
        {
            assert(newServer == remoteServersGlobal->serverHead);

            remoteServersGlobal->serverHead = newServer->next;
        }

        remoteServersGlobal->serverCount -= 1;

        if ((newServer->internalAttrs & iersREMSRVATTR_LOCAL) == 0)
        {
            remoteServersGlobal->remoteServerCount -= 1;
        }

        ismEngine_unlockRWLock(&(remoteServersGlobal->listLock));

        // Make it clear that this server is now not on the list and is logically deleted
        newServer->next = newServer->prev = NULL;
        newServer->internalAttrs |= iersREMSRVATTR_DELETED;

        iers_releaseServer(pThreadData, newServer);

        newServer = NULL;
    }
    // Tidy up if we allocated a server
    else if (newServer != NULL)
    {
        // Remove the store records if they were written and queue deletion is not going to do it
        if (newServer->highQoSQueueHandle == NULL &&
            newServer->hStoreDefn != ismSTORE_NULL_HANDLE)
        {
            // Trick the iers_freeServer function into deleting the RDR and RPR explicitly
            newServer->internalAttrs |= iersREMSRVATTR_LOCAL;
        }

        iers_freeServer(pThreadData, newServer, false);

        newServer = NULL;
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, newServer=%p\n",
               __func__, rc, newServer);

    return rc;
}

//****************************************************************************
/// @brief Remove a remote server (handle ENGINE_RS_REMOVE event)
///
/// @param[in]     server     Remote server to be removed
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_removeServer(ieutThreadData_t *pThreadData,
                          ismEngine_RemoteServer_t *server)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, server, ENGINE_FNC_TRACE, FUNCTION_ENTRY "server=%p, serverName='%s', serverUID='%s'\n",
               __func__, server, server->serverName, server->serverUID);

    assert(server->hStoreDefn != ismSTORE_NULL_HANDLE);
    assert((server->internalAttrs & iersREMSRVATTR_DELETED) == 0); // Not already deleted

    // Mark the store record for this remote server as DELETED, so that
    // if the server restarts before the queue is deleted it will be recognised at
    // startup.
    rc = ism_store_updateRecord(pThreadData->hStream,
                                server->hStoreDefn,
                                ismSTORE_NULL_HANDLE,
                                iestRDR_STATE_DELETED,
                                ismSTORE_SET_STATE);

    if (rc == OK)
    {
        iest_store_commit(pThreadData, false);
    }
    else
    {
        assert(rc != ISMRC_StoreGenerationFull);
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Local server should never be added to the topic tree
    if ((server->internalAttrs & iersREMSRVATTR_LOCAL) == 0)
    {
        rc = iett_purgeRemoteServerFromEngineTopicTree(pThreadData, server);

        if (rc != OK) goto mod_exit;
    }

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    // Remove this remote server from the global list
    ismEngine_getRWLockForWrite(&(remoteServersGlobal->listLock));

    if (server->next != NULL)
    {
        server->next->prev = server->prev;
    }

    if (server->prev != NULL)
    {
        server->prev->next = server->next;
    }
    else
    {
        assert(server == remoteServersGlobal->serverHead);

        remoteServersGlobal->serverHead = server->next;
    }

    remoteServersGlobal->serverCount -= 1;

    // If this is not the local server record, decrement the remote server count
    if ((server->internalAttrs & iersREMSRVATTR_LOCAL) == 0)
    {
        remoteServersGlobal->remoteServerCount -= 1;
    }

    ismEngine_unlockRWLock(&(remoteServersGlobal->listLock));

    // Make it clear that this server is now not on the list and is logically deleted
    server->next = server->prev = NULL;
    server->internalAttrs |= iersREMSRVATTR_DELETED;

    // Release the useCount on this remote server
    iers_releaseServer(pThreadData, server);

    // After this point server cannot be dereferenced as it may be freed.

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, remoteServer=%p\n",
               __func__, rc, server);

    return rc;
}

//****************************************************************************
/// @brief Update a remote server (handle ENGINE_RS_UPDATE event) or add a
///        remote server update to a batch of pending updates
///
/// @param[in,out] pPendingUpdate       Address of pointer to a pending update structure
/// @param[in]     commitUpdate         Whether this update should be committed TO THE STORE
///                                     or added as a pending update. In-memory changes are
///                                     are made immediately.
/// @param[in]     remoteServer         Remote server to be updated
/// @param[in]     pServerName          Remote server name [NULL if not being updated]
/// @param[in]     clusterData          Cluster data to update for the remote server
/// @param[in]     clusterDataLength    Length of the cluster data to update
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_updateServer(ieutThreadData_t *pThreadData,
                          ismEngine_RemoteServer_PendingUpdate_t **pPendingUpdate,
                          bool commitUpdate,
                          ismEngine_RemoteServer_t *server,
                          const char *pServerName,
                          void *clusterData,
                          size_t clusterDataLength)
{
    int32_t rc = OK;
    ismEngine_RemoteServer_PendingUpdate_t *pendingUpdate = NULL;
    ismEngine_RemoteServer_t **serversToCommit = NULL;
    uint32_t serversToCommitCount;

    ieutTRACEL(pThreadData, server, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pendingUpdate=%p, server==%p, serverName='%s', serverUID='%s', newServerName='%s', commitUpdate=%d\n",
               __func__, pendingUpdate, server,
               server ? server->serverName : "NONE",
               server ? server->serverUID : "NONE",
               pServerName ? pServerName : "NONE",
               (int)commitUpdate);

    // Generically update some attributes
    if (server != NULL)
    {
        if (pServerName != NULL && strcmp(pServerName, server->serverName) != 0)
        {
            char *newServerName = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_remoteServers, 13), strlen(pServerName)+1);

            if (newServerName == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            ieutDeferredFreeList_t *engineDeferredFrees = ismEngine_serverGlobal.deferredFrees;

            strcpy(newServerName, pServerName);
            ieut_addDeferredFree(pThreadData,
                                 engineDeferredFrees,
                                 server->serverName, NULL,
                                 iemem_remoteServers,
                                 iereNO_RESOURCE_SET);
            server->serverName = newServerName;
        }
    }

    // Not using the pending update interface, or this is a single update.
    if (pPendingUpdate == NULL || ((commitUpdate == true) && *pPendingUpdate == NULL))
    {
        if (server == NULL)
        {
            rc = ISMRC_ArgNotValid;
            ism_common_setError(rc);
            goto mod_exit;
        }

        iers_acquireServerReference(server);

        server->clusterData = clusterData;
        server->clusterDataLength = clusterDataLength;
        serversToCommit = &server;
        serversToCommitCount = 1;
    }
    else
    {
        pendingUpdate = *pPendingUpdate;

        if (pendingUpdate == NULL)
        {
            pendingUpdate = iemem_malloc(pThreadData,
                                         IEMEM_PROBE(iemem_remoteServers, 3),
                                         sizeof(ismEngine_RemoteServer_PendingUpdate_t));

            if (pendingUpdate == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            memcpy(pendingUpdate->StrucId, ismENGINE_REMOTESERVER_PENDINGUPDATE_STRUCID, 4);
            pendingUpdate->remoteServerCount = pendingUpdate->remoteServerMax = 0;
            pendingUpdate->remoteServers = NULL;
        }

        assert(pendingUpdate != NULL);

        // We have a remote server
        if (server != NULL)
        {
            // Not already part of a pending update
            if ((server->internalAttrs & iersREMSRVATTR_PENDING_UPDATE) == 0)
            {
                // Make sure we have space for it.
                if (pendingUpdate->remoteServerCount == pendingUpdate->remoteServerMax)
                {
                    size_t newSize = sizeof(ismEngine_RemoteServer_t **) * (pendingUpdate->remoteServerMax+100);

                    ismEngine_RemoteServer_t **newRemoteServers = iemem_realloc(pThreadData,
                                                                                IEMEM_PROBE(iemem_remoteServers, 4),
                                                                                pendingUpdate->remoteServers,
                                                                                newSize);

                    if (newRemoteServers == NULL)
                    {
                        if (*pPendingUpdate == NULL)
                        {
                            iemem_freeStruct(pThreadData, iemem_remoteServers, pendingUpdate, pendingUpdate->StrucId);
                        }
                        rc = ISMRC_AllocateError;
                        ism_common_setError(rc);
                        goto mod_exit;
                    }

                    pendingUpdate->remoteServers = newRemoteServers;
                }
            }

            // Setting clusterData to NULL
            if (clusterData == NULL)
            {
                assert(clusterDataLength == 0);

                if (server->clusterData != NULL)
                {
                    iemem_free(pThreadData, iemem_remoteServers, server->clusterData);
                }

                server->clusterData = clusterData;
                server->clusterDataLength = clusterDataLength;
            }
            // Setting some real cluster data
            else
            {
                assert(clusterDataLength != 0);

                void *newClusterData = iemem_realloc(pThreadData,
                                                     IEMEM_PROBE(iemem_remoteServers, 5),
                                                     server->clusterData,
                                                     clusterDataLength);

                if (newClusterData == NULL)
                {
                    if (*pPendingUpdate == NULL)
                    {
                        iemem_free(pThreadData, iemem_remoteServers, pendingUpdate->remoteServers);
                        iemem_freeStruct(pThreadData, iemem_remoteServers, pendingUpdate, pendingUpdate->StrucId);
                    }
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    goto mod_exit;
                }

                memcpy(newClusterData, clusterData, clusterDataLength);
                server->clusterData = newClusterData;
                server->clusterDataLength = clusterDataLength;
            }

            // Add this remote server to the pending updates.
            if ((server->internalAttrs & iersREMSRVATTR_PENDING_UPDATE) == 0)
            {
                iers_acquireServerReference(server);

                server->internalAttrs |= iersREMSRVATTR_PENDING_UPDATE;
                pendingUpdate->remoteServers[pendingUpdate->remoteServerCount] = server;
                pendingUpdate->remoteServerCount += 1;
            }
        }

        // This request has asked to commit.
        if (commitUpdate == true)
        {
            serversToCommit = pendingUpdate->remoteServers;
            serversToCommitCount = pendingUpdate->remoteServerCount;
        }
        // Tell the caller about the pendingUpdate structure
        else if (*pPendingUpdate == NULL)
        {
            *pPendingUpdate = pendingUpdate;
        }
    }

    // We have some remote servers to commit...
    if (serversToCommit != NULL)
    {
        // Request the updates
        rc = iest_updateRemoteServers(pThreadData, serversToCommit, serversToCommitCount);

        if (rc != OK)
        {
            if (pendingUpdate == NULL)
            {
                assert(server == serversToCommit[0]);
                iers_releaseServer(pThreadData, serversToCommit[0]);
            }

            goto mod_exit;
        }

        // Update the remote servers we've just updated
        for(uint32_t i=0; i<serversToCommitCount; i++)
        {
            // Update the queue
            if (serversToCommit[i]->highQoSQueueHandle != NULL)
            {
                ieq_setPropsHdl(serversToCommit[i]->highQoSQueueHandle, serversToCommit[i]->hStoreProps);
            }

            if ((pendingUpdate != NULL) && (serversToCommit[i]->clusterData != NULL))
            {
                iemem_free(pThreadData, iemem_remoteServers, serversToCommit[i]->clusterData);
            }

            serversToCommit[i]->internalAttrs &= ~iersREMSRVATTR_PENDING_UPDATE;
            serversToCommit[i]->clusterData = NULL;
            serversToCommit[i]->clusterDataLength = 0;

            iers_releaseServer(pThreadData, serversToCommit[i]);
        }

        if (pendingUpdate)
        {
            assert(pendingUpdate->remoteServers != NULL);
            assert(pPendingUpdate != NULL);

            iemem_free(pThreadData, iemem_remoteServers, pendingUpdate->remoteServers);
            iemem_freeStruct(pThreadData, iemem_remoteServers, pendingUpdate, pendingUpdate->StrucId);
            *pPendingUpdate = NULL;
        }
    }

mod_exit:

    ieutTRACEL(pThreadData, pendingUpdate, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, pendingUpdate=%p, server=%p\n",
               __func__, rc, pendingUpdate, server);

    return rc;
}

//****************************************************************************
/// @brief Add the remote server to the engine topic tree on the specified
///        topics
///
/// @param[in]     remoteServer            Remote server to be updated
/// @param[in]     topics                  Topics to add the server to
/// @param[in]     topicCount              Count of topics
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_addRemoteServerOnTopics(ieutThreadData_t *pThreadData,
                                     ismEngine_RemoteServer_t *remoteServer,
                                     char **topics,
                                     size_t topicCount)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, remoteServer, ENGINE_FNC_TRACE, FUNCTION_ENTRY "remoteServer=%p, topicCount=%lu\n",
               __func__, remoteServer, topicCount);

    assert((remoteServer->internalAttrs & iersREMSRVATTR_LOCAL) == 0);

    for(int32_t topic=0; topic<(int32_t)topicCount; topic++)
    {
        rc = iett_addRemoteServerToEngineTopic(pThreadData,
                                               topics[topic],
                                               remoteServer);

        if (rc != OK)
        {
            topic--;

            // Undo what we did so far
            while(topic >= 0)
            {
                (void)iett_removeRemoteServerFromEngineTopic(pThreadData,
                                                             topics[topic],
                                                             remoteServer);
            }

            goto mod_exit;
        }
    }

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, remoteServer=%p\n",
               __func__, rc, remoteServer);

    return rc;

}

//****************************************************************************
/// @brief Remove the remote server from the specified topics in the engine
///        topic tree.
///
/// @param[in]     remoteServer            Remote server to be updated
/// @param[in]     topics                  Topics to remove the server from
/// @param[in]     topicCount              Count of topics
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_removeRemoteServerFromTopics(ieutThreadData_t *pThreadData,
                                          ismEngine_RemoteServer_t *remoteServer,
                                          char **topics,
                                          size_t topicCount)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, remoteServer, ENGINE_FNC_TRACE, FUNCTION_ENTRY "remoteServer=%p, topicCount=%lu\n",
               __func__, remoteServer, topicCount);

    assert((remoteServer->internalAttrs & iersREMSRVATTR_LOCAL) == 0);

    for(int32_t topic=0; topic<(int32_t)topicCount; topic++)
    {
        rc = iett_removeRemoteServerFromEngineTopic(pThreadData,
                                                    topics[topic],
                                                    remoteServer);

        if (rc == ISMRC_NotFound) rc = OK; // Doesn't really matter

        // Don't try and clean up.
        if (rc != OK) goto mod_exit;
    }

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, remoteServer=%p\n",
               __func__, rc, remoteServer);

    return rc;

}

//****************************************************************************
/// @brief Mark remote server as connected / disconnected (handle
///        ENGINE_RS_CONNECT / ENGINE_RS_DISCONNECT events)
///
/// @param[in]     server       Server to be updated
/// @param[in]     isConnected  Whether the server is now connected
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_setConnectionState(ieutThreadData_t *pThreadData,
                                ismEngine_RemoteServer_t *server,
                                bool isConnected)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, server, ENGINE_FNC_TRACE, FUNCTION_ENTRY "server==%p, serverName='%s', serverUID='%s', isConnected=%d\n",
               __func__, server, server->serverName, server->serverUID, (int)isConnected);

    iers_acquireServerReference(server);

    bool stateChanged = false;

    // Switch off the 'disconnected' flag
    if (isConnected)
    {
        if ((server->internalAttrs & iersREMSRVATTR_DISCONNECTED) == iersREMSRVATTR_DISCONNECTED)
        {
            server->internalAttrs &= ~iersREMSRVATTR_DISCONNECTED;
            stateChanged = true;
        }
    }
    // Switch on the 'disconnected' flag
    else
    {
        if ((server->internalAttrs & iersREMSRVATTR_DISCONNECTED) == 0)
        {
            server->internalAttrs |= iersREMSRVATTR_DISCONNECTED;
            stateChanged = true;
        }
    }

    // We changed state, so there may be other actions to perform
    if (stateChanged == true)
    {
        // TODO: Any other actions we should take here?
    }

    iers_releaseServer(pThreadData, server);

    ieutTRACEL(pThreadData, stateChanged, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, server=%p, stateChanged=%d\n",
               __func__, rc, server, (int)stateChanged);

    return rc;
}

//****************************************************************************
/// @brief Set the 'Route All' state of a remote server (handle ENGINE_RS_ROUTE)
///
/// @param[in]     server      Remote server to be updated
/// @param[in]     isRouteAll  Whether the server is now in route all state
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_setRouteAllState(ieutThreadData_t *pThreadData,
                              ismEngine_RemoteServer_t *server,
                              bool isRouteAll)
{
    int32_t rc = OK;
    bool stateChanged = false;

    ieutTRACEL(pThreadData, server, ENGINE_FNC_TRACE, FUNCTION_ENTRY "server==%p, serverName='%s', serverUID='%s', isRouteAll=%d\n",
               __func__, server, server->serverName, server->serverUID, (int)isRouteAll);

    iers_acquireServerReference(server);

    // Switch on the 'RouteAll' flag
    if (isRouteAll)
    {
        if ((server->internalAttrs & iersREMSRVATTR_ROUTE_ALL_MODE) == 0)
        {
            server->internalAttrs |= iersREMSRVATTR_ROUTE_ALL_MODE;
            stateChanged = true;
        }
    }
    // Switch off the 'RouteAll' flag
    else
    {
        if ((server->internalAttrs & iersREMSRVATTR_ROUTE_ALL_MODE) == iersREMSRVATTR_ROUTE_ALL_MODE)
        {
            server->internalAttrs &= ~iersREMSRVATTR_ROUTE_ALL_MODE;
            stateChanged = true;
        }
    }

    // We changed state, so there may be other actions to perform
    if (stateChanged)
    {
        // TODO: Any other actions we should take here?
    }

    iers_releaseServer(pThreadData, server);

    ieutTRACEL(pThreadData, stateChanged, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, server=%p, stateChanged=%d\n",
               __func__, rc, server, (int)stateChanged);

    return rc;
}

//****************************************************************************
/// @brief Report engine statistics that the cluster component has an interest
///        in
///
/// @param[in]     clusterEngineStats  Cluster statistics structure to fill in
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
void iers_reportEngineStatistics(ieutThreadData_t *pThreadData,
                                 ismCluster_EngineStatistics_t *clusterEngineStats)
{
    iemnMessagingStatistics_t messagingStats;

    ieutTRACEL(pThreadData, clusterEngineStats, ENGINE_FNC_TRACE, FUNCTION_ENTRY "clusterEngineStats=%p\n",
               __func__, clusterEngineStats);

    iemn_getMessagingStatistics(pThreadData, &messagingStats);

    clusterEngineStats->numFwdIn = messagingStats.FromForwarderMessages;
    clusterEngineStats->numFwdInNoConsumer = messagingStats.FromForwarderNoRecipientMessages;
    clusterEngineStats->numFwdInRetained = messagingStats.FromForwarderRetainedMessages;

    ieutTRACEL(pThreadData, clusterEngineStats->numFwdInNoConsumer, ENGINE_FNC_TRACE, FUNCTION_EXIT "numFwdInNoConsumer=%lu\n",
               __func__, clusterEngineStats->numFwdInNoConsumer);
}

//****************************************************************************
/// @brief Remove this server from the cluster (handle ENGINE_RS_TERM event)
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_terminateCluster(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;
    bool listLocked = false;

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    ieutTRACEL(pThreadData, remoteServersGlobal, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (ismEngine_serverGlobal.clusterEnabled == false)
    {
        ieutTRACEL(pThreadData, ismEngine_serverGlobal.clusterEnabled, ENGINE_WORRYING_TRACE,
                   "%s called when cluster not enabled.\n", __func__);
        goto mod_exit;
    }

    // Tell everyone to stop making cluster calls
    ismEngine_serverGlobal.clusterEnabled = false;

    // Run through the remote servers in the global list
    ismEngine_getRWLockForWrite(&(remoteServersGlobal->listLock));
    listLocked = true;

    if (remoteServersGlobal->serverCount != 0)
    {
        ismEngine_RemoteServer_t **serverList = iemem_malloc(pThreadData,
                                                             IEMEM_PROBE(iemem_remoteServers, 16),
                                                             (remoteServersGlobal->serverCount+1)*
                                                              sizeof(ismEngine_RemoteServer_t *));

        if (serverList == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        uint32_t updatesPending = 0;
        uint32_t entryCount = 0;
        ismEngine_RemoteServer_t *currentServer = remoteServersGlobal->serverHead;

        // Mark the store records for all servers as DELETED, so that if the server
        // restarts before we're done we can finish cleaning up.
        while(currentServer != NULL)
        {
            ismEngine_RemoteServer_t *nextServer = currentServer->next;

            serverList[entryCount++] = currentServer;

            assert(currentServer->hStoreDefn != ismSTORE_NULL_HANDLE);

            rc = ism_store_updateRecord(pThreadData->hStream,
                                        currentServer->hStoreDefn,
                                        ismSTORE_NULL_HANDLE,
                                        iestRDR_STATE_DELETED,
                                        ismSTORE_SET_STATE);

            assert(rc != ISMRC_StoreGenerationFull);

            updatesPending++;

            if (currentServer->next != NULL)
            {
                currentServer->next->prev = currentServer->prev;
            }

            if (currentServer->prev != NULL)
            {
                currentServer->prev->next = currentServer->next;
            }
            else
            {
                assert(currentServer == remoteServersGlobal->serverHead);

                remoteServersGlobal->serverHead = currentServer->next;
            }

            remoteServersGlobal->serverCount -= 1;

            // If this is not a local server record, decrement the remote server count
            if ((currentServer->internalAttrs & iersREMSRVATTR_LOCAL) == 0)
            {
                remoteServersGlobal->remoteServerCount -= 1;
            }

            // Make it clear that this server is now not on the list and is logically deleted
            currentServer->next = currentServer->prev = NULL;
            currentServer->internalAttrs |= iersREMSRVATTR_DELETED;

            currentServer = nextServer;
        }

        serverList[entryCount] = NULL;

        // Commit the updates we just made.
        if (updatesPending != 0)
        {
            ieutTRACEL(pThreadData, updatesPending, ENGINE_HIGH_TRACE, "Committing %u updates\n", updatesPending);
            iest_store_commit(pThreadData, false);
        }

        ismEngine_unlockRWLock(&(remoteServersGlobal->listLock));
        listLocked = false;

        entryCount = 0;
        while((currentServer = serverList[entryCount++]) != NULL)
        {
            if ((currentServer->internalAttrs & iersREMSRVATTR_LOCAL) == 0)
            {
                rc = iett_purgeRemoteServerFromEngineTopicTree(pThreadData, currentServer);

                if (rc != OK) break;
            }

            // Release the useCount on this remote server
            iers_releaseServer(pThreadData, currentServer);

            // After this point currentServer cannot be dereferenced as it may be freed.
        }

        // Free our list of remote servers
        iemem_free(pThreadData, iemem_remoteServers, serverList);
    }

mod_exit:

    if (listLocked)
    {
        ismEngine_unlockRWLock(&(remoteServersGlobal->listLock));
    }
    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Callback used by cluster component to handle events
///
/// @param[in]     eventType               Event type
/// @param[in]     hRemoteServer           Engine handle for the remote server [!CREATE]
/// @param[in]     hClusterHandle          Cluster component's handle for this server [CREATE]
/// @param[in]     pServerName             NULL terminated server Name [CREATE/UPDATE]
/// @param[in]     pServerUID              NULL terminated server UID [CREATE]
/// @param[in]     pRemoteServerData       Remote server data to store [UPDATE]
/// @param[in]     remoteServerDataLength  Length of pRemoteServerData in bytes [UPDATE]
/// @param[in]     pSubscriptionTopics     Wildcard topics for this server [ADD_SUB/DEL_SUB]
/// @param[in]     subscriptionTopicCount  Count of topics in pSubscriptionTopics [ADD_SUB/DEL_SUB]
/// @param[in]     fIsRouteAll             Server is currently in Route-All state [ROUTE]
/// @param[in]     fCommitUpdate           Whether the update should be committed [UPDATE]
/// @param[in,out] phPendingUpdateHandle   Handle used to indicate an existing pending update that
///                                        should be added to [UPDATE].
/// @param[in,out] pEngineStatistics       Structure to fill in with engine statistics [REPORT_STATS]
/// @param[in]     pContext                Our context from ism_cluster_registerEngineEventCallback
/// @param[out]    phRemoteServerHandle    Returned server handle [CREATE]
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_clusterEventCallback(ENGINE_RS_EVENT_TYPE_t eventType,
                                  ismEngine_RemoteServerHandle_t hRemoteServer,
                                  ismCluster_RemoteServerHandle_t hClusterHandle,
                                  const char *pServerName,
                                  const char *pServerUID,
                                  void *pRemoteServerData,
                                  size_t remoteServerDataLength,
                                  char **pSubscriptionTopics,
                                  size_t subscriptionsTopicCount,
                                  uint8_t fIsRouteAll,
                                  uint8_t fCommitUpdate,
                                  ismEngine_RemoteServer_PendingUpdateHandle_t  *phPendingUpdateHandle,
                                  ismCluster_EngineStatistics_t *pEngineStatistics,
                                  void *pContext,
                                  ismEngine_RemoteServerHandle_t *phRemoteServerHandle)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int rc = OK;
    ismEngine_RemoteServer_t *remoteServer = (ismEngine_RemoteServer_t *)hRemoteServer;

    assert(pContext == NULL); // Sanity check that we're not specifying a context

    if (hRemoteServer != NULL)
    {
        ieutTRACEL(pThreadData, remoteServer, ENGINE_CEI_TRACE, FUNCTION_ENTRY "type=%d, remoteServer=%p, serverName='%s', serverUID='%s'\n",
                   __func__, eventType, remoteServer, remoteServer->serverName, remoteServer->serverUID);
    }
    else
    {
        // Only a subset of commands can accept a NULL remoteServer (UPDATE only if
        // it is dealing with some pending updated, the iers_updateServer deals
        // with the specifics).
        assert(eventType == ENGINE_RS_CREATE || eventType == ENGINE_RS_CREATE_LOCAL ||
               eventType == ENGINE_RS_UPDATE || eventType == ENGINE_RS_REPORT_STATS || eventType == ENGINE_RS_TERM);

        ieutTRACEL(pThreadData, eventType, ENGINE_CEI_TRACE, FUNCTION_ENTRY "type=%d, serverName='%s', serverUID='%s'\n",
                   __func__, eventType, pServerName ? pServerName : "(nil)", pServerUID ? pServerUID : "(nil)");
    }

    if ((volatile ismEngineRunPhase_t)ismEngine_serverGlobal.runPhase < EnginePhaseEnding)
    {
        __sync_fetch_and_add(&clusterCallbackActiveUseCount, 1);

        switch(eventType)
        {
            // Create the remote server record for storing the local server's state
            case ENGINE_RS_CREATE_LOCAL:
            // Create a new remote server record
            case ENGINE_RS_CREATE:
                rc = iers_createServer(pThreadData,
                                       hClusterHandle,
                                       pServerName,
                                       pServerUID,
                                       (eventType == ENGINE_RS_CREATE_LOCAL),
                                       phRemoteServerHandle);
                break;
            // Remove (destroy) an existing remote server record
            case ENGINE_RS_REMOVE:
                rc = iers_removeServer(pThreadData, remoteServer);
                break;
            // Request the update of an existing remote server record
            case ENGINE_RS_UPDATE:
                rc = iers_updateServer(pThreadData,
                                       phPendingUpdateHandle,
                                       (fCommitUpdate == 1),
                                       remoteServer,
                                       pServerName,
                                       pRemoteServerData,
                                       remoteServerDataLength);
                break;
            // Mark this server as connected
            case ENGINE_RS_CONNECT:
                rc = iers_setConnectionState(pThreadData, remoteServer, true);
                break;
            // Mark this server as disconnected
            case ENGINE_RS_DISCONNECT:
                rc = iers_setConnectionState(pThreadData, remoteServer, false);
                break;
            // Deal with the reporting of 'route all' status change
            case ENGINE_RS_ROUTE:
                rc = iers_setRouteAllState(pThreadData, remoteServer, (fIsRouteAll == 1));
                break;
            // Add the specified remote server to the topic tree at the specified topic
            case ENGINE_RS_ADD_SUB:
                rc = iers_addRemoteServerOnTopics(pThreadData,
                                                  remoteServer,
                                                  pSubscriptionTopics,
                                                  subscriptionsTopicCount);
                break;
            case ENGINE_RS_DEL_SUB:
                rc = iers_removeRemoteServerFromTopics(pThreadData,
                                                       remoteServer,
                                                       pSubscriptionTopics,
                                                       subscriptionsTopicCount);
                break;
            case ENGINE_RS_REPORT_STATS:
                assert(pEngineStatistics != NULL);
                iers_reportEngineStatistics(pThreadData, pEngineStatistics);
                break;
            case ENGINE_RS_TERM:
                rc = iers_terminateCluster(pThreadData);
                break;
            default:
                rc = ISMRC_InvalidOperation;
                break;
        }

        __sync_fetch_and_sub(&clusterCallbackActiveUseCount, 1);
    }
    else
    {
        // Return invalid operation if we're called after we're ending
        rc = ISMRC_InvalidOperation;
    }

    if (rc != OK) ism_common_setError(rc);

    if (hRemoteServer != NULL)
    {
        ieutTRACEL(pThreadData, rc, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    }
    else
    {
        ieutTRACEL(pThreadData, remoteServer, ENGINE_CEI_TRACE, FUNCTION_EXIT "remoteServer=%p rc=%d\n",
                   __func__, remoteServer, rc);
    }

    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @brief Deregister the callback function and wait for any active callbacks
///        to complete.
//****************************************************************************
void iers_stopClusterEventCallbacks(ieutThreadData_t *pThreadData)
{
    ieutTRACEL(pThreadData, clusterCallbackActiveUseCount, ENGINE_FNC_TRACE, FUNCTION_ENTRY "clusterCallbackActiveUseCount=%d\n",
               __func__, clusterCallbackActiveUseCount);

    // Tell the cluster component to stop calling us
    (void)ism_cluster_registerEngineEventCallback(NULL, NULL);

    // Make sure that we have initialized the UseCount in init
    if ((volatile uint32_t)clusterCallbackActiveUseCount > 0)
    {
        int pauseMs = 20000;   // Initial pause is 0.02s
        uint32_t loop = 0;

        // Remove the contribution to useCount from the remoteServers code
        (void)__sync_fetch_and_sub(&clusterCallbackActiveUseCount, 1);

        // Wait for there to be no active callbacks
        while ((volatile uint32_t)clusterCallbackActiveUseCount > 0)
        {
            if ((loop%100) == 0)
            {
                ieutTRACEL(pThreadData, loop, ENGINE_NORMAL_TRACE, "%s: clusterCallbackActiveUseCount is %d\n",
                           __func__, clusterCallbackActiveUseCount);
            }

            // And pause for a short time to allow the timer to end
            ism_common_sleep(pauseMs);

            if (++loop > 290) // After 2 minutes
            {
                pauseMs = 5000000; // Upgrade pause to 5 seconds
            }
            else if (loop > 50) // After 1 Seconds
            {
                pauseMs = 500000; // Upgrade pause to .5 second
            }
        }
    }

    ieutTRACEL(pThreadData, clusterCallbackActiveUseCount, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

//****************************************************************************
/// @brief  Add the set of remote servers interested in the topic specified
///         to the requesting subscriber list
///
/// @param[in,out] subList    Subscriber list to be updated
/// @param[in]     message    The message for which this list is
///                           being constructed (OPTIONAL)
///
/// @return OK on successful completion, ISMRC_NotFound if not found or an
///         ISMRC_ value.
///
/// @remark Once the subscriber list is no longer need, it must be released
///         using iett_releaseSubscriberList.
///
/// @see iett_releaseSubscriberList
//****************************************************************************
int32_t iers_addRemoteServersToSubscriberList(ieutThreadData_t *pThreadData,
                                              iettSubscriberList_t *subList,
                                              ismEngine_Message_t *message)
{
    int32_t rc = ISMRC_NotFound;
    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];

    ieutTRACEL(pThreadData, subList, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "subList=%p, message=%p\n", __func__,
               subList, message);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    // TODO: if ((ismEngine_serverGlobal.clusterEnabled == true) && (remoteServersGlobal->remoteServerCount != 0))
    // is what the following line should be (to avoid extra work when the cluster is not enabled) which would also
    // remove the need to check it again before calling ism_cluster_routeLookup in the middle of this code...
    // Changing to only add servers if cluster is enabled breaks a protocol unit test at the moment, so I'm reverting
    // this code to it's old self.
    if (remoteServersGlobal->remoteServerCount != 0)
    {
        // If this is a system topic, don't add any servers to avoid us delivering this message to
        // any remote servers.
        if (iettTOPIC_IS_SYSTOPIC(subList->topicString))
        {
            ieutTRACEL(pThreadData, topic.sysTopicEndIndex, ENGINE_HIFREQ_FNC_TRACE,
                       "Skipping lookup for topic '%s'\n", subList->topicString);
            rc = ISMRC_NotFound;
            goto mod_exit;
        }

        assert(subList->remoteServerCount == 0);

        // If this is for a retained message, add to all servers
        if (message != NULL && (message->Header.Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN) != 0)
        {
            uint32_t seenSeeding = 0;

            // Get the read lock on the remoteServers delaying while there is a remote server
            // being seeded.
            do
            {
                ismEngine_getRWLockForRead(&(remoteServersGlobal->listLock));

                if ((volatile uint32_t)remoteServersGlobal->seedingCount > 0)
                {
                    ismEngine_unlockRWLock(&(remoteServersGlobal->listLock));
                    ism_common_sleep(50000); // 0.05 seconds

                    if ((seenSeeding % 1000) == 0)
                    {
                        ieutTRACEL(pThreadData, seenSeeding, ENGINE_INTERESTING_TRACE, "Server seen seeding %u times\n", seenSeeding);
                    }

                    seenSeeding++;
                }
                else
                {
                    seenSeeding = 0;
                }
            }
            while(seenSeeding > 0);

            assert(seenSeeding == 0);

            if (remoteServersGlobal->remoteServerCount != 0)
            {
                subList->remoteServers = iemem_malloc(pThreadData,
                                                      IEMEM_PROBE(iemem_subsQuery, 10),
                                                      (remoteServersGlobal->remoteServerCount+1) * sizeof(ismEngine_RemoteServer_t *));

                if (subList->remoteServers == NULL)
                {
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                }
                else
                {
                    rc = OK;

                    ismEngine_RemoteServer_t *currentServer = remoteServersGlobal->serverHead;
                    while(currentServer != NULL)
                    {
                        // Don't include local server entries
                        if ((currentServer->internalAttrs & iersREMSRVATTR_LOCAL) == 0)
                        {
                            assert((currentServer->internalAttrs & iersREMSRVATTR_DELETED) == 0);
                            assert(currentServer->lowQoSQueueHandle != NULL);
                            assert(currentServer->highQoSQueueHandle != NULL);

                            __sync_fetch_and_add(&currentServer->useCount, 1);
                            subList->remoteServers[subList->remoteServerCount++] = currentServer;
                        }

                        currentServer = currentServer->next;
                    }

                    subList->remoteServers[subList->remoteServerCount] = NULL;
                    subList->remoteServerCapacity = subList->remoteServerCount;
                }
            }

            ismEngine_unlockRWLock(&remoteServersGlobal->listLock);
        }
        // Not a retained message - so look for interested remote servers
        else
        {
            topic.destinationType = ismDESTINATION_TOPIC;
            topic.topicString = subList->topicString;
            topic.substrings = substrings;
            topic.substringHashes = substringHashes;
            topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

            rc = iett_analyseTopicString(pThreadData, &topic);

            if (rc != OK) goto mod_exit;

            rc = iett_addRemoteServersToSubscriberList(pThreadData, &topic, subList);

            if (rc == OK || rc == ISMRC_NotFound)
            {
                if (subList->remoteServerCount < remoteServersGlobal->remoteServerCount)
                {
                    ismCluster_LookupInfo_t lookupInfo;

                    lookupInfo.pTopic = (char *)(subList->topicString);
                    lookupInfo.topicLen = strlen(lookupInfo.pTopic);

                    // Allocate and fill in the Matched servers array from the servers already
                    // returned.
                    if (subList->remoteServerCount != 0)
                    {
                        lookupInfo.phMatchedServers = iemem_malloc(pThreadData,
                                                                   IEMEM_PROBE(iemem_subsQuery, 11),
                                                                   sizeof(ismCluster_RemoteServerHandle_t) * subList->remoteServerCount);

                        if (lookupInfo.phMatchedServers == NULL)
                        {
                            rc = ISMRC_AllocateError;
                            ism_common_setError(rc);
                            goto mod_exit;
                        }

                        for(int32_t index=0; index<subList->remoteServerCount; index++)
                        {
                            lookupInfo.phMatchedServers[index] = subList->remoteServers[index]->clusterHandle;
                        }
                    }
                    else
                    {
                        lookupInfo.phMatchedServers = NULL;
                    }

                    uint32_t requiredDestsLen = remoteServersGlobal->remoteServerCount;

                    // TODO: Validate that this does not cause a deadlock - it will if the subsequent cluster
                    //       call takes a lock that the cluster component also holds when it calls ENGINE_RS_CREATE,
                    //       ENGINE_RS_CREATE_LOCAL or ENGINE_RS_REMOVE... An alternative approach might be for
                    //       us to take the lock, increment the useCounts of all remote servers, call the various
                    //       functions to get subs, and reduce the useCount on those that don't appear on the list,
                    //       but this is a bit overkillish.
                    ismEngine_getRWLockForRead(&(remoteServersGlobal->listLock));

                    rc = OK;
                    do
                    {
                        lookupInfo.numDests = (int)(subList->remoteServerCount);

                        if (subList->remoteServerCapacity < requiredDestsLen)
                        {
                            // Make sure the subList->remoteServers array is big enough to pass to cluster component
                            ismEngine_RemoteServer_t **newRemoteServers = iemem_realloc(pThreadData,
                                                                                        IEMEM_PROBE(iemem_subsQuery, 12),
                                                                                        subList->remoteServers,
                                                                                        (requiredDestsLen + 1) * sizeof(ismEngine_RemoteServer_t *));
                            if (newRemoteServers == NULL)
                            {
                                rc = ISMRC_AllocateError;
                                ism_common_setError(rc);
                            }
                            else
                            {
                                subList->remoteServers = newRemoteServers;
                                subList->remoteServerCapacity = requiredDestsLen;
                            }
                        }

                        if (rc != ISMRC_AllocateError)
                        {
                            rc = OK;

                            lookupInfo.destsLen = (int)requiredDestsLen;
                            lookupInfo.phDests = subList->remoteServers;

                            if (ismEngine_serverGlobal.clusterEnabled)
                            {
                                rc = ism_cluster_routeLookup(&lookupInfo);
                            }

                            if (rc == ISMRC_ClusterArrayTooSmall)
                            {
                                requiredDestsLen += 10;
                            }
                            else if (rc != OK)
                            {
                                ism_common_setError(rc);
                            }
                        }
                    }
                    while(rc == ISMRC_ClusterArrayTooSmall);

                    int32_t index = (int32_t)(subList->remoteServerCount);

                    // Increase the use count on newly added remote servers while holding the lock.
                    for(; index<lookupInfo.numDests; index++)
                    {
                        (void)__sync_fetch_and_add(&(subList->remoteServers[index]->useCount), 1);
                    }

                    ismEngine_unlockRWLock(&remoteServersGlobal->listLock);

                    // There are no remote servers, so free up the subscriber list now.
                    if (index == 0)
                    {
                        iemem_free(pThreadData, iemem_subsQuery, subList->remoteServers);
                        subList->remoteServers = NULL;
                    }
                    else
                    {
                        subList->remoteServers[index] = NULL;
                    }

                    subList->remoteServerCount = (uint32_t)index;

                    if (lookupInfo.phMatchedServers != NULL) iemem_free(pThreadData, iemem_subsQuery, lookupInfo.phMatchedServers);
                }
            }
        }
    }

mod_exit:

    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d, subList=%p, remoteServerCount=%u\n",
               __func__, rc, subList, subList->remoteServerCount);

    return rc;
}

//****************************************************************************
/// @brief  Acquire a reference to this remoteServer
///
/// Increment the useCount for this remote server
///
/// @param[in]    server    Remote server to acquire a reference to
///
/// @return OK on successful completion, ISMRC_NotFound if not found or an
///         ISMRC_ value.
///
/// @remark Once it is no longer needed, release the remoteServer using
///         iers_releaseServer.
///
/// @see iers_releaseServer
//****************************************************************************
void iers_acquireServerReference(ismEngine_RemoteServer_t *server)
{
    ismEngine_CheckStructId(server->StrucId, ismENGINE_REMOTESERVER_STRUCID, ieutPROBE_001);

    (void)__sync_fetch_and_add(&server->useCount, 1);
}

//****************************************************************************
/// @brief  Release the remote server
///
/// Decrement the use count for the remote server freeing it if it drops to zero
///
/// @param[in]     pThreadData   Thread Data to use
/// @param[in]     server        Remote server to release
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_releaseServer(ieutThreadData_t *pThreadData,
                           ismEngine_RemoteServer_t *server)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, server, ENGINE_FNC_TRACE, FUNCTION_ENTRY "server=%p\n",
               __func__, server);

    uint32_t oldCount = __sync_fetch_and_sub(&server->useCount, 1);

    assert(oldCount != 0); // useCount just went negative

    // This was the last user free the remote server
    if (oldCount == 1)
    {
        // Must no longer be in the list
        assert(server->prev == NULL && server->next == NULL);

        iers_freeServer(pThreadData, server, false);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d (useCount=%u)\n", __func__, rc, oldCount-1);

    return rc;
}

//****************************************************************************
/// @brief  Free the memory and runtime state for a given remote server.
///
/// @param[in]     server     Remote server to be free'd
/// @param[in]     freeOnly   Whether we should only free the memory and not
///                           store (e.g. during shutdown).
//****************************************************************************
void iers_freeServer(ieutThreadData_t *pThreadData,
                     ismEngine_RemoteServer_t *server,
                     bool freeOnly)
{
    ieutTRACEL(pThreadData, server, ENGINE_FNC_TRACE, FUNCTION_ENTRY "server=%p freeOnly=%d\n", __func__,
               server, (int)freeOnly);

    // We need to explicitly delete the RDR / RPR for the local server
    if (server->internalAttrs & iersREMSRVATTR_LOCAL)
    {
        assert(server->lowQoSQueueHandle == NULL);
        assert(server->highQoSQueueHandle == NULL);

        // We don't need to do this if we're freeing
        if (freeOnly == false)
        {
            int32_t rc;

            if (LIKELY(server->hStoreDefn != ismSTORE_NULL_HANDLE))
            {
                rc = ism_store_deleteRecord(pThreadData->hStream, server->hStoreDefn);
                if (rc != OK)
                {
                    ieutTRACEL(pThreadData, rc, ENGINE_ERROR_TRACE,
                               "%s: ism_store_deleteRecord 0x%0lx failed! (rc=%d)\n",
                               __func__, server->hStoreDefn, rc);
                }
            }

            if (LIKELY(server->hStoreProps != ismSTORE_NULL_HANDLE))
            {
                rc = ism_store_deleteRecord(pThreadData->hStream, server->hStoreProps);
                if (rc != OK)
                {
                    ieutTRACEL(pThreadData, rc, ENGINE_ERROR_TRACE,
                               "%s: ism_store_deleteRecord 0x%0lx failed! (rc=%d)\n",
                               __func__, server->hStoreProps, rc);
                }
            }

            iest_store_commit(pThreadData, false);
        }

    }
    else
    {
        if (LIKELY(server->lowQoSQueueHandle != NULL))
        {
            (void)ieq_delete(pThreadData, &(server->lowQoSQueueHandle), freeOnly);
        }

        // Deleting the high QoS queue will also delete the RDR / RPR from the store
        if (LIKELY(server->highQoSQueueHandle != NULL))
        {
            (void)ieq_delete(pThreadData, &(server->highQoSQueueHandle), freeOnly);
        }
    }

    iemem_free(pThreadData, iemem_remoteServers, server->serverName);
    iemem_freeStruct(pThreadData, iemem_remoteServers, server, server->StrucId);

    ieutTRACEL(pThreadData, freeOnly, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

//****************************************************************************
/// @internal
///
/// @brief Register a consumer on a remote server queue
///
/// @param[in]     server           Remote server upon which consumer is registered
/// @param[in]     consumer         Consumer to be registered as a subscriber
/// @param[in]     destinationType  The type of destination this consumer is registering
///                                 as consumerOptions  Consumer options (to identify low or high QoS)
///
/// @remark Assumes that the global engine topic tree has been initialised
///         by a call to iett_initEngineTopicTree.
///
/// This function expects the remote server to have it's useCount incremented by
//  the caller - it then takes over the useCount.
///
/// @see iers_unregisterConsumer
//****************************************************************************
void iers_registerConsumer(ieutThreadData_t *pThreadData,
                           ismEngine_RemoteServer_t *server,
                           ismEngine_Consumer_t *consumer,
                           ismDestinationType_t destinationType)
{
    ieutTRACEL(pThreadData, consumer, ENGINE_FNC_TRACE, FUNCTION_IDENT "server=%p, consumer=%p, destinationType=%u\n", __func__,
               server, consumer, (uint32_t)destinationType);

    assert((server->internalAttrs & iersREMSRVATTR_LOCAL) == 0);

    // Increment the consumerCount, the useCount should already have an increment
    // which we are going to adopt.
    (void)__sync_fetch_and_add(&server->consumerCount, 1);

    consumer->engineObject = server;

    // Which queue to consumer from is based on the destinationType
    if (destinationType == ismDESTINATION_REMOTESERVER_HIGH)
    {
        consumer->queueHandle = server->highQoSQueueHandle;
    }
    else
    {
        consumer->queueHandle = server->lowQoSQueueHandle;
    }
}

//****************************************************************************
/// @internal
///
/// @brief Unregister a consumer previously registered with iers_registerConsumer
///
/// @param[in]     consumer         Consumer being unregistered
/// @param[in]     destinationType  Destination type specified for consumer
///
/// @see iett_registerConsumer
//****************************************************************************
void iers_unregisterConsumer(ieutThreadData_t *pThreadData,
                             ismEngine_Consumer_t *consumer,
                             ismDestinationType_t destinationType)
{
    ismEngine_RemoteServer_t *server = (ismEngine_RemoteServer_t *)consumer->engineObject;

    ieutTRACEL(pThreadData, consumer, ENGINE_FNC_TRACE, FUNCTION_IDENT "server=%p, consumer=%p, destinationType=%d\n", __func__,
               server, consumer, (uint32_t)destinationType);

    // TODO: Do we need to call iecs_trackInflightMsgs? -- not sure, but I think not.

    // Decrement the consumer count
    DEBUG_ONLY uint32_t oldConsumerCount = __sync_fetch_and_sub(&server->consumerCount, 1);
    assert(oldConsumerCount != 0);

    // Decrement the general useCount - potentially destroying the remote server
    (void)iers_releaseServer(pThreadData, server);

    consumer->engineObject = NULL;
}

//****************************************************************************
/// @internal
///
/// @brief Write the descriptions of remote server structures to the dump
///
/// @param[in]     dumpHdl  Pointer to a dump structure
//****************************************************************************
void iers_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    iedm_describe_iersRemoteServers_t(dump->fp);
    iedm_describe_ismEngine_RemoteServer_t(dump->fp);
    iedm_describe_ismEngine_RemoteServer_PendingUpdate_t(dump->fp);
}

//****************************************************************************
/// @internal
///
/// @brief Return to the caller the current memory limit level in place for
///        remote server forwarding queues.
///
/// @remarks This function is intended to be called by other engine sub-components
/// in order to decide whether it is OK for them to 'spend' more memory for
/// cluster related activities.
///
/// For instance, when operating as part of a cluster, NullRetained messages are
/// kept for quite a long period of time so that conflicts can be resolved.
///
/// In some circumstances, when memory is low for instance, or after a server has
/// been removed from the cluster, we want to get rid of these messages as soon as
/// possible.
///
/// The result of this call can enable such decisions to be made in other
/// sub-components.
///
/// @return Current value of the memory limit being kept for remote servers.
//****************************************************************************
iersMemLimit_t iers_queryRemoteServerMemLimit(ieutThreadData_t *pThreadData)
{
    iersMemLimit_t returnValue = NoLimit;

    ieutTRACEL(pThreadData, ismEngine_serverGlobal.clusterEnabled, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Check if we're currently imposing limits
    if (ismEngine_serverGlobal.clusterEnabled)
    {
        iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

        returnValue = remoteServersGlobal->currentMemLimit;
    }

    ieutTRACEL(pThreadData, returnValue, ENGINE_FNC_TRACE, FUNCTION_EXIT "returnValue=%d\n",
               __func__, (int)returnValue);

    return returnValue;
}

//****************************************************************************
/// @internal
///
/// @brief Analyse memory usage reporting to the cluster component and
///        imposing limits on forwarding queues as appropriate
///
/// @param[in]     currState     Current memory handler state in place
/// @param[in]     prevState     Previous memory handler state
/// @param[in]     type          Memory type for which we're being called
/// @param[in]     currentLevel  The current amount of memory attributed to this
///                              type.
/// @param[in]     memInfo       System memory information
//****************************************************************************
void iers_analyseMemoryUsage(iememMemoryLevel_t currState,
                             iememMemoryLevel_t prevState,
                             iemem_memoryType type,
                             uint64_t currentLevel,
                             iemem_systemMemInfo_t *memInfo)
{
    assert(type == iemem_remoteServers);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    // ***********************************************
    // Work out what health status to tell the cluster component
    // ***********************************************
    ismCluster_HealthStatus_t newHealthStatus = ISM_CLUSTER_HEALTH_GREEN;

    // We won't be called after being called for iememPlentifulMemory, so make sure it will
    // be equivalent to a GREEN result.
    assert(IEMEM_FREEMEM_REDUCETHRESHOLD >= iersFREEMEM_LIMIT_CLUSTER_HEALTHSTATUS_YELLOW);

    if (currState != iememPlentifulMemory)
    {
        if (memInfo->freeIncPercentage <= iersFREEMEM_LIMIT_CLUSTER_HEALTHSATUS_RED)
        {
            newHealthStatus = ISM_CLUSTER_HEALTH_RED;
        }
        else if (memInfo->freeIncPercentage <= iersFREEMEM_LIMIT_CLUSTER_HEALTHSTATUS_YELLOW)
        {
            newHealthStatus = ISM_CLUSTER_HEALTH_YELLOW;
        }
        else
        {
            newHealthStatus = ISM_CLUSTER_HEALTH_GREEN;
        }
    }
    else
    {
        newHealthStatus = ISM_CLUSTER_HEALTH_GREEN;
    }

    assert(newHealthStatus != ISM_CLUSTER_HEALTH_UNKNOWN);

    // The status is different from the last one we successfully reported - so tell cluster
    if (remoteServersGlobal->currentHealthStatus != newHealthStatus)
    {
        int32_t rc = ism_cluster_setHealthStatus(newHealthStatus);

        if (rc == OK)
        {
            remoteServersGlobal->currentHealthStatus = newHealthStatus;
        }
    }

    // ***********************************************
    // Work out which limits to impose on queues
    // ***********************************************
    uint64_t remoteServerQueueCount = remoteServersGlobal->remoteServerCount * 2;

    iepiPolicyInfo_t *lowQoSPolicyInfo = remoteServersGlobal->lowQoSPolicyHandle;
    iepiPolicyInfo_t *seedingPolicyInfo = remoteServersGlobal->seedingPolicyHandle;
    iepiPolicyInfo_t *highQoSPolicyInfo = remoteServersGlobal->highQoSPolicyHandle;

    uint64_t oldLowMaxMessageBytes = lowQoSPolicyInfo->maxMessageBytes;
    uint64_t oldHighMaxMessageBytes = highQoSPolicyInfo->maxMessageBytes;
    uint64_t newLowMaxMessageBytes;
    uint64_t newHighMaxMessageBytes;

    assert(iersFREEMEM_LIMIT_LOW_QOS_FWD_THRESHOLD > iersFREEMEM_LIMIT_HIGH_QOS_FWD_THRESHOLD);
    assert(iersFREEMEM_LIMIT_HIGH_QOS_FWD_THRESHOLD > iersFREEMEM_LIMIT_DISCARD_LOCAL_NULLRETAINED);

    // Work out which limit we would impose at this memory level
    iersMemLimit_t newLimit;

    if (memInfo->freeIncPercentage <= iersFREEMEM_LIMIT_LOW_QOS_FWD_THRESHOLD)
    {
        newLimit = LowQoSLimit;

        if (memInfo->freeIncPercentage <= iersFREEMEM_LIMIT_DISCARD_LOCAL_NULLRETAINED)
        {
            newLimit = DiscardLocalNullRetained;
        }
        else if (memInfo->freeIncPercentage <= iersFREEMEM_LIMIT_HIGH_QOS_FWD_THRESHOLD)
        {
            newLimit = HighQoSLimit;
        }
    }
    else
    {
        newLimit = NoLimit;
    }

    // See if we are below the low QoS threshold of free memory
    if ((remoteServerQueueCount != 0) && (newLimit != NoLimit))
    {
        iemnMessagingStatistics_t msgStats;

        // Find out how much is being used across all remote servers
        iemn_getMessagingStatistics(NULL, &msgStats);

        // Add one to the remoteServerQueueCount for the situation where all servers are in ROUTE_ALL
        // mode - we need to clear some messages.
        remoteServerQueueCount += 1;

        // TODO: Should we actually check if all servers are in ROUTE_ALL mode? We could do so by
        //       keeping a count of those that are - remoteServerQueueCount/2 should == that value.

        // Make the maximum bytes the average for all queues (there are 2 per remote server)
        newLowMaxMessageBytes = (uint64_t)msgStats.RemoteServerBufferedMessageBytes / remoteServerQueueCount;

        // If we have a reservation of bytes for forwarding, make sure this new low is at least that
        if (remoteServersGlobal->reservedForwardingBytes > 0)
        {
            uint64_t reservedMaxMessageBytes =  remoteServersGlobal->reservedForwardingBytes / remoteServerQueueCount;

            // New low is lower than reserved amount, so use the reserved amount.
            if (newLowMaxMessageBytes < reservedMaxMessageBytes)
            {
                newLowMaxMessageBytes = reservedMaxMessageBytes;
            }
        }

        // Check whether we're at or above the high QoS threshold of free memory
        if (newLimit >= HighQoSLimit)
        {
            newHighMaxMessageBytes = newLowMaxMessageBytes;
        }
        else
        {
            newHighMaxMessageBytes = 0;
        }
    }
    else
    {
        newLowMaxMessageBytes = newHighMaxMessageBytes = 0;
    }

    // Something has changed for the lowQoS queues.
    if (oldLowMaxMessageBytes != newLowMaxMessageBytes)
    {
        if (oldLowMaxMessageBytes == 0)
        {
            TRACE(ENGINE_INTERESTING_TRACE, "Imposing MaxMessageBytes %lu on low QoS policy\n", newLowMaxMessageBytes);
        }
        else if (newLowMaxMessageBytes == 0)
        {
            TRACE(ENGINE_INTERESTING_TRACE, "Removing limit on low QoS policy\n");
        }

        lowQoSPolicyInfo->maxMessageBytes = newLowMaxMessageBytes;
    }

    // Something has changed for the highQoS queues and the seeding policy.
    if (oldHighMaxMessageBytes != newHighMaxMessageBytes)
    {
        if (oldHighMaxMessageBytes == 0)
        {
            TRACE(ENGINE_INTERESTING_TRACE, "Imposing MaxMessageBytes %lu on high QoS and seeding policies\n", newHighMaxMessageBytes);
        }
        else if (newHighMaxMessageBytes == 0)
        {
            TRACE(ENGINE_INTERESTING_TRACE, "Removing limit on high QoS and seeding policies\n");
        }

        highQoSPolicyInfo->maxMessageBytes = newHighMaxMessageBytes;
        seedingPolicyInfo->maxMessageBytes = newLowMaxMessageBytes;
    }

    // Remember what limits are being imposed
    if (remoteServersGlobal->currentMemLimit != newLimit)
    {
        TRACE(ENGINE_INTERESTING_TRACE, "RemoteServers limit changing from %d to %d (freeIncPercentage=%u%%, reservedForwardingBytes=%lu, remoteServersCount=%u)\n",
              (int)remoteServersGlobal->currentMemLimit, (int)newLimit,
              (uint32_t)(memInfo->freeIncPercentage),
              remoteServersGlobal->reservedForwardingBytes,
              remoteServersGlobal->remoteServerCount);

        remoteServersGlobal->currentMemLimit = newLimit;
    }

    return;
}

//****************************************************************************
/// @brief Dump the contents of a remote server
///
/// @param[in]     server    Remote server to dump
/// @param[in]     dumpHdl   Handle to the dump
//****************************************************************************
void iers_dumpServer(ieutThreadData_t *pThreadData,
                     ismEngine_RemoteServer_t *server,
                     iedmDumpHandle_t  dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    if (iedm_dumpStartObject(dump, server) == true)
    {
        iedm_dumpStartGroup(dump, "RemoteServer");
        iedm_dumpData(dump, "ismEngine_RemoteServer_t", server,
                      iemem_usable_size(iemem_remoteServers, server));

        // At higher detail levels, dump the contents of the queues for this server
        if (dump->detailLevel >= iedmDUMP_DETAIL_LEVEL_5)
        {
            if (NULL != server->lowQoSQueueHandle)
            {
                ieq_dump(pThreadData, server->lowQoSQueueHandle, dumpHdl);
            }

            if (NULL != server->highQoSQueueHandle)
            {
                ieq_dump(pThreadData, server->highQoSQueueHandle, dumpHdl);
            }
        }

        iedm_dumpEndGroup(dump);

        iedm_dumpEndObject(dump, server);
    }
}

//****************************************************************************
/// @internal
///
/// @brief Dump the remote servers list.
///
/// @param[in]     dumpHdl          Dump to write to
///
/// @returns OK or an ISMRC_ value on error.
//****************************************************************************
int32_t iers_dumpServersList(ieutThreadData_t *pThreadData,
                             iedmDumpHandle_t dumpHdl)
{
    int32_t rc = OK;

    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    ieutTRACEL(pThreadData, dump, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    iedm_dumpStartGroup(dump, "RemoteServers");

    // Always dump the topic tree structure and current values in policies
    iedm_dumpData(dump, "iersRemoteServers_t", remoteServersGlobal, iemem_usable_size(iemem_remoteServers, remoteServersGlobal));
    iedm_dumpData(dump, "iepiPolicyInfo_t",
                  remoteServersGlobal->lowQoSPolicyHandle,
                  iemem_usable_size(iemem_policyInfo, remoteServersGlobal->lowQoSPolicyHandle));
    iedm_dumpData(dump, "iepiPolicyInfo_t",
                  remoteServersGlobal->highQoSPolicyHandle,
                  iemem_usable_size(iemem_policyInfo, remoteServersGlobal->lowQoSPolicyHandle));

    // Dump the current set of remote servers
    ismEngine_getRWLockForRead(&remoteServersGlobal->listLock);

    ismEngine_RemoteServer_t *currentServer = remoteServersGlobal->serverHead;

    while(currentServer != NULL)
    {
        iers_dumpServer(pThreadData, currentServer, dumpHdl);
        currentServer = currentServer->next;
    }

    ismEngine_unlockRWLock(&remoteServersGlobal->listLock);

    iedm_dumpEndGroup(dump);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Forward all retained messages originating at a specified origin
///         serverUID to the specified requesting serverUID.
///
/// @param[in]  originServerUID      Server UID identifying retained messages to forward
/// @param[in]  options              Options specified by the requesting server
/// @param[in]  timestamp            Timestamp specified by the requesting server to use
///                                  in selecting retained messages.
/// @param[in]  correlationId        An identifier for this forwarding request
/// @param[in]  requestingServerUID  Server UID to which messages should be forwarded
/// @param[in]  pContext             Optional context for completion callback
/// @param[in]  contextLength        Length of data pointed to by pContext
/// @param[in]  pCallbackFn          Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_forwardRetainedMessages(const char *originServerUID,
                                                             uint32_t options,
                                                             ism_time_t timestamp,
                                                             uint64_t correlationId,
                                                             const char *requestingServerUID,
                                                             void *pContext,
                                                             size_t contextLength,
                                                             ismEngine_CompletionCallback_t pCallbackFn)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, correlationId, ENGINE_CEI_TRACE, FUNCTION_ENTRY "originServerUID=%s, options=0x%08x, timestamp=%lu, correlationId=0x%016lx, requestingServerUID=%s\n",
               __func__, originServerUID, options, timestamp, correlationId, requestingServerUID);

    // Don't expect to be called by ourselves
    assert(strcmp(ism_common_getServerUID(), requestingServerUID) != 0);

    // Find the requesting server so we know where to send responses
    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    ismEngine_getRWLockForRead(&remoteServersGlobal->listLock);

    ismEngine_RemoteServer_t *requestingServer = remoteServersGlobal->serverHead;

    while(requestingServer != NULL)
    {
        if (strcmp(requestingServer->serverUID, requestingServerUID) == 0)
        {
            iers_acquireServerReference(requestingServer);
            break;
        }

        requestingServer = requestingServer->next;
    }

    ismEngine_unlockRWLock(&remoteServersGlobal->listLock);

    if (requestingServer == NULL)
    {
        rc = ISMRC_NotFound;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Put any retained messages to the requesting server
    rc = iers_putAllRetained(pThreadData,
                             originServerUID,
                             options,
                             timestamp,
                             requestingServer,
                             false);

    iers_releaseServer(pThreadData, requestingServer);

 mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "correlationId=0x%016lx, rc=%d\n", __func__, correlationId, rc);

    return rc;
}
