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
/// @file  remoteServersSync.c
/// @brief Engine component remote server synchronization functions
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
#include "engineHashTable.h"
#include "remoteServers.h"
#include "remoteServersInternal.h"
#include "cluster.h"
#include "topicTree.h"
#include "topicTreeInternal.h"

//****************************************************************************
/// @brief Generate a hash value for a specified server UID.
///
/// @param[in]     key  The key for which to generate a hash value
///
/// @remark This is a version of the xor djb2 hashing algorithm
///
/// @return The hash value
//****************************************************************************
uint32_t iers_generateServerUIDHash(const char *key)
{
    uint32_t keyHash = 5381;
    char curChar;

    while((curChar = *key++))
    {
        keyHash = (keyHash * 33) ^ curChar;
    }

    return keyHash;
}

//****************************************************************************
/// @brief Release a use count on the specified known remoteServer and free
///        memory used by it in the knownServers hash
///
/// @param[in]     key      Key of the hash table entry being passed
/// @param[in]     keyHash  Hash of the key
/// @param[in]     value    Value of the hash entry (pointer to serverSyncInfo)
/// @param[in]     context  The context passed to the callback routine
//****************************************************************************
void iers_syncReleaseKnownServer(ieutThreadData_t *pThreadData,
                                 char *key,
                                 uint32_t keyHash,
                                 void *value,
                                 void *context)
{
    iersServerSyncInfo_t *syncInfo = (iersServerSyncInfo_t *)value;

    iers_releaseServer(pThreadData, syncInfo->server);

    iemem_free(pThreadData, iemem_remoteServers, syncInfo);
}

//****************************************************************************
/// @brief Free memory used by servers in allServers hash, that are not also
///        known servers
///
/// @param[in]     key      Key of the hash table entry being passed
/// @param[in]     keyHash  Hash of the key
/// @param[in]     value    Value of the hash entry (pointer to serverSyncInfo)
/// @param[in]     context  The context passed to the callback routine
//****************************************************************************
void iers_syncFreeServer(ieutThreadData_t *pThreadData,
                         char *key,
                         uint32_t keyHash,
                         void *value,
                         void *context)
{
    iersServerSyncInfo_t *syncInfo = (iersServerSyncInfo_t *)value;

    // If the SyncInfo has a non-null server pointer, it means that this is a server
    // that we know about, so it will be cleaned up when the knownServers hash table
    // is cleaned up.
    if (syncInfo->server == NULL)
    {
        iemem_free(pThreadData, iemem_remoteServers, syncInfo);
    }
}

//****************************************************************************
/// @brief Ask the cluster component about stats for a known remote server
///
/// @param[in]     key      Key of the hash table entry being passed
/// @param[in]     keyHash  Hash of the key
/// @param[in]     value    Value of the hash entry (pointer to serverSyncInfo)
/// @param[in]     context  The context passed to the callback routine
//****************************************************************************
void iers_syncGetClusterStats(ieutThreadData_t *pThreadData,
                              char *key,
                              uint32_t keyHash,
                              void *value,
                              void *context)
{
    iersClusterSyncInfo_t *clusterInfo = (iersClusterSyncInfo_t *)context;

    if (clusterInfo->rc == OK)
    {
        iersServerSyncInfo_t *knownServerInfo = (iersServerSyncInfo_t *)value;

        assert(knownServerInfo->server != NULL);
        assert(knownServerInfo->serverUID != NULL);

        // Don't ask about ourselves
        // TODO: This might need to change if we allow clean store of cluster members
        if ((knownServerInfo->server->internalAttrs & iersREMSRVATTR_LOCAL) == 0)
        {
            ismCluster_LookupRetainedStatsInfo_t *lookupInfo = NULL;

            clusterInfo->rc = ism_cluster_lookupRetainedStats(knownServerInfo->serverUID,
                                                              &lookupInfo);

            // If the serverUID is unknown, the return value is OK but lookupInfo will be NULL
            if (clusterInfo->rc == OK && lookupInfo != NULL)
            {
                iettOriginServerStats_t originServerStats;

                for(int32_t statNo=lookupInfo->numStats-1; statNo >= 0; statNo--)
                {
                    clusterInfo->rc = iett_convertDataToOriginServerStats(pThreadData,
                                                                          lookupInfo->pStats[statNo].pData,
                                                                          lookupInfo->pStats[statNo].dataLength,
                                                                          &originServerStats);

                    // We have some stats
                    if (clusterInfo->rc == OK)
                    {
                        iersServerSyncInfo_t *serverInfo = NULL;
                        char *serverUID = lookupInfo->pStats[statNo].pServerUID;
                        uint32_t serverUIDHash = iers_generateServerUIDHash(serverUID);

                        // Already in the allServers hash?
                        int32_t localRc = ieut_getHashEntry(clusterInfo->allServers,
                                                            serverUID,
                                                            serverUIDHash,
                                                            (void **)&serverInfo);

                        if (localRc == ISMRC_NotFound)
                        {
                            // Already in our knownServers hash?
                            localRc = ieut_getHashEntry(clusterInfo->knownServers,
                                                        serverUID,
                                                        serverUIDHash,
                                                        (void **)&serverInfo);

                            if (localRc == ISMRC_NotFound)
                            {
                                serverInfo = iemem_calloc(pThreadData,
                                                          IEMEM_PROBE(iemem_remoteServers, 18),
                                                          1, sizeof(iersServerSyncInfo_t)+strlen(serverUID)+1);

                                if (serverInfo == NULL)
                                {
                                    clusterInfo->rc = ISMRC_AllocateError;
                                    break;
                                }

                                serverInfo->serverUID = (char *)(serverInfo+1);
                                strcpy(serverInfo->serverUID, serverUID);
                            }

                            clusterInfo->rc = ieut_putHashEntry(pThreadData,
                                                                clusterInfo->allServers,
                                                                ieutFLAG_DUPLICATE_NONE,
                                                                serverInfo->serverUID,
                                                                serverUIDHash,
                                                                serverInfo,
                                                                0);

                            if (clusterInfo->rc != OK) break;

                            // Add to the linked list of all servers
                            assert(serverInfo->next == NULL);
                            serverInfo->next = clusterInfo->firstAllServer;
                            clusterInfo->firstAllServer = serverInfo;
                        }

                        assert(serverInfo != NULL);

                        // See if this is one of our best servers.
                        for(int32_t thisSrv=0; thisSrv < MAX_BEST_SERVERS; thisSrv++)
                        {
                            if (iett_compareOriginServerStats(&originServerStats, &serverInfo->bestStats[thisSrv]) > 0)
                            {
                                int32_t nextSrv = thisSrv+1;
                                int32_t elementsToMove = MAX_BEST_SERVERS-nextSrv;

                                // Shuffle down if necessary
                                if (elementsToMove != 0)
                                {
                                    memmove(&serverInfo->bestStats[nextSrv],
                                            &serverInfo->bestStats[thisSrv],
                                            sizeof(serverInfo->bestStats[0]) * elementsToMove);
                                    memmove(&serverInfo->bestServer[nextSrv],
                                            &serverInfo->bestServer[thisSrv],
                                            sizeof(serverInfo->bestServer[0]) * elementsToMove);
                                }

                                // Add our new entry in
                                memcpy(&serverInfo->bestStats[thisSrv], &originServerStats, sizeof(serverInfo->bestStats[thisSrv]));
                                serverInfo->bestServer[thisSrv] = knownServerInfo->server;
                                break;
                            }
                        }
                    }
                }

                clusterInfo->rc = ism_cluster_freeRetainedStats(lookupInfo);
            }
        }
    }
}

//****************************************************************************
/// @brief Get statistics from the local origin server information
///
/// @param[in]     serverInfo   The server to get local information for
//****************************************************************************
void iers_syncGetLocalStats(ieutThreadData_t *pThreadData,
                            iersServerSyncInfo_t *serverInfo)
{
    iettOriginServer_t *originServer;

    assert(serverInfo->serverUID != NULL);

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ismEngine_getRWLockForRead(&tree->topicsLock);

    // Access our local origin server stats for this same server
    int32_t localRc = iett_insertOrFindOriginServer(pThreadData,
                                                    serverInfo->serverUID,
                                                    iettOP_FIND,
                                                    &originServer);

    if (localRc == OK)
    {
        memcpy(&serverInfo->localStats, &originServer->stats, sizeof(serverInfo->localStats));
    }
    else
    {
        assert(localRc == ISMRC_NotFound);
    }

    ismEngine_unlockRWLock(&tree->topicsLock);
}

//****************************************************************************
/// @internal
///
/// @brief Analyse the results of a synchronization scan.
///
/// @param[in]       clusterInfo   Information gathered during the scan
///****************************************************************************
void iers_syncAnalyseResults(ieutThreadData_t *pThreadData,
                             iersClusterSyncInfo_t *clusterInfo)
{
    ieutTRACEL(pThreadData, clusterInfo, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    iersServerSyncInfo_t *currInfo = clusterInfo->firstAllServer;

    while(currInfo != NULL)
    {
        iersSyncLevel_t newSyncLevel = Unevaluated;

        // Promote the best _connected_ server to be the best server.
        for(int32_t thisSrv=0; thisSrv < MAX_BEST_SERVERS; thisSrv++)
        {
            ismEngine_RemoteServer_t *thisServer = currInfo->bestServer[thisSrv];

            // Make the best connected server the best server
            if (thisServer != NULL && (thisServer->internalAttrs & iersREMSRVATTR_DISCONNECTED) == 0)
            {
                currInfo->bestServer[0] = thisServer;
                memcpy(&currInfo->bestStats[0], &currInfo->bestStats[thisSrv], sizeof(currInfo->bestStats[0]));
                break;
            }
            else
            {
                currInfo->bestServer[thisSrv] = NULL;
            }
        }

        // None of the best servers are flagged as connected
        if (currInfo->bestServer[0] == NULL)
        {
            newSyncLevel =  NoneConnected;
        }
        else
        {
            bool allCountsZero = false;

            // We should only be here if the cluster has a view of what is best
            assert(currInfo->bestServer[0]->serverUID != NULL);
            assert(strcmp(currInfo->bestServer[0]->serverUID, ism_common_getServerUID()) != 0);

            // Compare counts
            if (currInfo->localStats.count != currInfo->bestStats[0].count)
            {
                newSyncLevel |= CountMismatch;
            }
            else if (currInfo->localStats.count == 0)
            {
                assert(currInfo->bestStats[0].count == 0);
                allCountsZero = true;
            }

            // If everyone agrees that there are zero messages from this origin server,
            // then we consider things to be in sync.
            if (allCountsZero == true)
            {
                assert(newSyncLevel == Unevaluated);
            }
            // Someone thinks there is at least one message from this origin server, so
            // we need to check other stat attributes to ensure consistency.
            else
            {
                // Compare highest timestamp seen
                if (currInfo->localStats.highestTimestampSeen > currInfo->bestStats[0].highestTimestampSeen)
                {
                    newSyncLevel |= LocalHTSHigher;
                }
                else if (currInfo->localStats.highestTimestampSeen < currInfo->bestStats[0].highestTimestampSeen)
                {
                    newSyncLevel |= ClusterHTSHigher;
                }

                // Compare highest timestamp available
                if (currInfo->localStats.highestTimestampAvailable > currInfo->bestStats[0].highestTimestampAvailable)
                {
                    newSyncLevel |= LocalHTAHigher;
                }
                else if (currInfo->localStats.highestTimestampAvailable < currInfo->bestStats[0].highestTimestampAvailable)
                {
                    newSyncLevel |= ClusterHTAHigher;
                }

                // Compare Topic identifiers
                if (currInfo->localStats.topicsIdentifier != currInfo->bestStats[0].topicsIdentifier)
                {
                    newSyncLevel |= TopicsMismatch;
                }
            }

            // If we get this far and everything matched, consider ourselves synchronized
            if (newSyncLevel == Unevaluated)
            {
                newSyncLevel = Synchronized;

                ieutTRACEL(pThreadData, newSyncLevel, ENGINE_NORMAL_TRACE,
                           "Server='%s' syncLevel=0x%08X allCountsZero=%s\n",
                           currInfo->serverUID, newSyncLevel, allCountsZero ? "true" : "false");
            }
            else
            {
                assert(allCountsZero == false);

                if (currInfo->server != NULL)
                {
                    newSyncLevel |= KnownServer;

                    if ((currInfo->server->internalAttrs & iersREMSRVATTR_DISCONNECTED) == 0)
                    {
                        newSyncLevel |= ConnectedServer;
                    }
                }

                clusterInfo->inSync = false;

                ieutTRACEL(pThreadData, newSyncLevel, ENGINE_INTERESTING_TRACE,
                           "Server='%s' syncLevel=0x%08X\n", currInfo->serverUID, newSyncLevel);
                ieutTRACEL(pThreadData, currInfo->localStats.highestTimestampSeen, ENGINE_UNUSUAL_TRACE,
                           "Compared to '%s': count=%u:%u highestTimestampSeen=%lu:%lu "
                           "highestTimestampAvailable=%lu:%lu topicsIdentifier=%lu:%lu [localCount=%u:%u]\n",
                           currInfo->bestServer[0]->serverUID,
                           currInfo->localStats.count, currInfo->bestStats[0].count,
                           currInfo->localStats.highestTimestampSeen, currInfo->bestStats[0].highestTimestampSeen,
                           currInfo->localStats.highestTimestampAvailable, currInfo->bestStats[0].highestTimestampAvailable,
                           currInfo->localStats.topicsIdentifier, currInfo->bestStats[0].topicsIdentifier,
                           currInfo->localStats.localCount, currInfo->bestStats[0].localCount);
            }
        }

        assert(newSyncLevel != Unevaluated);
        assert(currInfo->syncLevel == Unevaluated);

        currInfo->syncLevel = newSyncLevel;

        currInfo = currInfo->next;
    }

    ieutTRACEL(pThreadData, clusterInfo->inSync, ENGINE_FNC_TRACE, FUNCTION_EXIT "inSync=%d\n",
               __func__, clusterInfo->inSync);
}

//****************************************************************************
/// @internal
///
/// @brief Reconcile retained message information from the cluster and the
///        local server and decide whether to request retained messages from
///        another server.
///****************************************************************************
int32_t iers_syncClusterRetained(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;
    iersRemoteServers_t *remoteServersGlobal = NULL;
    bool setCheckInProgress = false;
    iersClusterSyncInfo_t clusterInfo = {0};
    clusterInfo.inSync = true;

    ieutTRACEL(pThreadData, ismEngine_serverGlobal.clusterEnabled, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (ismEngine_serverGlobal.clusterEnabled == true)
    {
        remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

        // We only want one instance of this sync check to be running at any time, so make sure
        // another one isn't already running (if one is, we just let it finish)
        setCheckInProgress = __sync_bool_compare_and_swap(&remoteServersGlobal->syncCheckInProgress, false, true);

        if (!setCheckInProgress)
        {
            ieutTRACEL(pThreadData,
                       remoteServersGlobal->syncCheckInProgress,
                       ENGINE_WORRYING_TRACE, FUNCTION_IDENT "Check already in progress.\n", __func__);
            goto mod_exit;
        }

        rc = ieut_createHashTable(pThreadData,
                                  remoteServersGlobal->serverCount + 10,
                                  iemem_remoteServers,
                                  &clusterInfo.knownServers);

        if (rc != OK) goto mod_exit;

        rc = ieut_createHashTable(pThreadData,
                                  100,
                                  iemem_remoteServers,
                                  &clusterInfo.allServers);

        if (rc != OK) goto mod_exit;

        // Start out finding all of the remote servers we are in contact with
        ismEngine_getRWLockForRead(&(remoteServersGlobal->listLock));

        if (remoteServersGlobal->remoteServerCount != 0)
        {
            ismEngine_RemoteServer_t *remoteServer = remoteServersGlobal->serverHead;

            while(remoteServer != NULL)
            {
                iersServerSyncInfo_t *serverInfo = iemem_calloc(pThreadData,
                                                                IEMEM_PROBE(iemem_remoteServers, 19),
                                                                1, sizeof(iersServerSyncInfo_t) + strlen(remoteServer->serverUID) + 1);

                if (serverInfo == NULL)
                {
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    break;
                }

                serverInfo->serverUID = (char *)(serverInfo+1);
                strcpy(serverInfo->serverUID, remoteServer->serverUID);
                serverInfo->server = remoteServer;

                rc = ieut_putHashEntry(pThreadData,
                                       clusterInfo.knownServers,
                                       ieutFLAG_DUPLICATE_NONE,
                                       serverInfo->serverUID,
                                       iers_generateServerUIDHash(serverInfo->serverUID),
                                       serverInfo,
                                       0);

                if (rc != OK) break;

                iers_acquireServerReference(remoteServer);

                remoteServer = remoteServer->next;
            }

            ismEngine_unlockRWLock(&(remoteServersGlobal->listLock));

            if (rc != OK) goto mod_exit;

            // For each server we know about, ask cluster component for it's statistics
            ieut_traverseHashTable(pThreadData,
                                   clusterInfo.knownServers,
                                   iers_syncGetClusterStats,
                                   &clusterInfo);

            if (clusterInfo.rc != OK)
            {
                rc = clusterInfo.rc;
                ism_common_setError(rc);
                goto mod_exit;
            }

            iersServerSyncInfo_t *currInfo = clusterInfo.firstAllServer;

            // Get our local statistics for each server we've discovered.
            while(currInfo != NULL)
            {
                iers_syncGetLocalStats(pThreadData, currInfo);
                currInfo = currInfo->next;
            }

            // Analyse the results
            iers_syncAnalyseResults(pThreadData, &clusterInfo);

            // If we have a function available to request retained messages with, we
            // now need to decide what to do.
            if (ismEngine_serverGlobal.retainedForwardingFn != NULL)
            {
                ism_time_t delayNanos = (ism_time_t)ismEngine_serverGlobal.retainedForwardingDelay * 1000000000;
                ism_time_t timeNow = ism_common_currentTimeNanos();
                uint64_t correlationId = (uint64_t)timeNow;

                currInfo = clusterInfo.firstAllServer;

                // If we are low on memory, we shouldn't make things worse by requesting
                // more messages - but we should go through the process of determining if
                // we are in sync.
                bool lowMemory = iers_queryRemoteServerMemLimit(pThreadData) != NoLimit;

                while(currInfo != NULL)
                {
                    uint32_t UIDHash = iers_generateServerUIDHash(currInfo->serverUID);

                    if ((currInfo->syncLevel & Synchronized) != 0)
                    {
                        pthread_spin_lock(&remoteServersGlobal->outOfSyncLock);
                        (void)ieut_removeHashEntry(pThreadData,
                                                   remoteServersGlobal->outOfSyncServers,
                                                   currInfo->serverUID,
                                                   UIDHash);
                        pthread_spin_unlock(&remoteServersGlobal->outOfSyncLock);
                    }
                    else
                    {
                        ism_time_t timeStamp;

                        pthread_spin_lock(&remoteServersGlobal->outOfSyncLock);
                        int32_t getRc = ieut_getHashEntry(remoteServersGlobal->outOfSyncServers,
                                                          currInfo->serverUID,
                                                          UIDHash,
                                                          (void **)&timeStamp);
                        pthread_spin_unlock(&remoteServersGlobal->outOfSyncLock);

                        if (getRc == ISMRC_NotFound)
                        {
                            timeStamp = timeNow;
                        }

                        // If we have no connected servers, then we can't take any action
                        if (currInfo->syncLevel != NoneConnected)
                        {
                            ism_time_t checkNanos = delayNanos;

                            // If we believe we're connected to the server, let's delay for
                            // longer.
                            if ((currInfo->syncLevel & ConnectedServer) != 0)
                            {
                                assert((currInfo->syncLevel & KnownServer) != 0);
                                checkNanos *= 2;
                            }

                            // If it's time to take action now - do so.
                            if (timeNow-timeStamp >= checkNanos)
                            {
                                uint32_t options;
                                ism_time_t timestamp;

                                // We are ahead of the cluster - someone will ask us for messages
                                if ((currInfo->syncLevel & LocalHTSHigher) != 0)
                                {
                                    rc = OK;
                                }
                                // Ask for messages if we are not low on memory
                                else if (lowMemory == false)
                                {
                                    // Highest timestamp is lower than cluster - ask for newer ones
                                    if ((currInfo->syncLevel & ClusterHTSHigher) != 0 &&
                                         currInfo->localStats.highestTimestampSeen != 0)
                                    {
                                        options = ismENGINE_FORWARD_RETAINED_OPTION_NEWER;
                                        timestamp = currInfo->localStats.highestTimestampSeen;
                                    }
                                    // Ask for them all
                                    else
                                    {
                                        options = ismENGINE_FORWARD_RETAINED_OPTION_NONE;
                                        timestamp = 0;
                                    }

                                    // Make the actual request
                                    rc = ismEngine_serverGlobal.retainedForwardingFn(currInfo->bestServer[0]->serverUID,
                                                                                     currInfo->serverUID,
                                                                                     options,
                                                                                     timestamp,
                                                                                     correlationId);

                                    ieutTRACEL(pThreadData, correlationId, ENGINE_INTERESTING_TRACE,
                                               "respondingServerUID='%s', originServerUID='%s', options=0x%08x, timestamp=%lu, correlationId=0x%016lx, rc=%d\n",
                                               currInfo->bestServer[0]->serverUID, currInfo->serverUID, options, timestamp, correlationId, rc);

                                    correlationId += 1;
                                }
                                else
                                {
                                    assert(lowMemory == true);
                                    ieutTRACEL(pThreadData, timeStamp, ENGINE_INTERESTING_TRACE,
                                               "Low memory, avoiding resync of originServerUID='%s' syncLevel=0x%08x\n",
                                               currInfo->serverUID, currInfo->syncLevel);
                                    rc = ISMRC_ServerCapacity; // Set a non-OK rc so we leave entry in the hash table
                                }

                                if (rc == OK)
                                {
                                    pthread_spin_lock(&remoteServersGlobal->outOfSyncLock);
                                    (void)ieut_removeHashEntry(pThreadData,
                                                               remoteServersGlobal->outOfSyncServers,
                                                               currInfo->serverUID,
                                                               UIDHash);
                                    pthread_spin_unlock(&remoteServersGlobal->outOfSyncLock);
                                    getRc = OK;
                                }

                                rc = OK;
                            }
                        }

                        // If we haven't got an entry, add one for this server now.
                        if (getRc == ISMRC_NotFound)
                        {
                            pthread_spin_lock(&remoteServersGlobal->outOfSyncLock);
                            (void)ieut_putHashEntry(pThreadData,
                                                    remoteServersGlobal->outOfSyncServers,
                                                    ieutFLAG_DUPLICATE_KEY_STRING,
                                                    currInfo->serverUID,
                                                    UIDHash,
                                                    (void *)timeStamp,
                                                    0);
                            pthread_spin_unlock(&remoteServersGlobal->outOfSyncLock);
                        }
                    }

                    currInfo = currInfo->next;
                }
            }
        }
        else
        {
            ismEngine_unlockRWLock(&(remoteServersGlobal->listLock));
        }
    }

mod_exit:

    if (setCheckInProgress)
    {
        assert(remoteServersGlobal != NULL);
        DEBUG_ONLY bool clearedCheckInProgress = __sync_bool_compare_and_swap(&remoteServersGlobal->syncCheckInProgress, true, false);
        assert(clearedCheckInProgress == true);
    }

    if (clusterInfo.knownServers != NULL)
    {
        if (clusterInfo.allServers != NULL)
        {
            ieut_traverseHashTable(pThreadData,
                                   clusterInfo.allServers,
                                   iers_syncFreeServer,
                                   &clusterInfo);
            ieut_destroyHashTable(pThreadData, clusterInfo.allServers);
        }

        // Now we actually release and free the locally known servers
        ieut_traverseHashTable(pThreadData,
                               clusterInfo.knownServers,
                               iers_syncReleaseKnownServer,
                               &clusterInfo);
        ieut_destroyHashTable(pThreadData, clusterInfo.knownServers);
    }

    ieutTRACEL(pThreadData, clusterInfo.inSync, ENGINE_FNC_TRACE, FUNCTION_EXIT "clusterInfo.inSync=%d, rc=%d\n",
               __func__, clusterInfo.inSync, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief Return the time at which this server went into the out-of-sync list.
///
/// @param[in]     serverUID      The UID of the server to check
/// @param[out]    outOfSyncTime  The time at which this server went out of sync
///
/// @return ISMRC_NotFound if the server is not in the out-of-sync list, OK if
///         it is or another ISMRC error.
///****************************************************************************
int32_t iers_getOutOfSyncTime(ieutThreadData_t *pThreadData,
                              const char *serverUID,
                              ism_time_t *outOfSyncTime)
{
    int32_t rc = ISMRC_NotFound;

    ieutTRACEL(pThreadData, ismEngine_serverGlobal.clusterEnabled, ENGINE_FNC_TRACE, FUNCTION_ENTRY "serverUID='%s'\n",
               __func__, serverUID);

    if (ismEngine_serverGlobal.clusterEnabled == true)
    {
        uint32_t serverUIDHash = iers_generateServerUIDHash(serverUID);
        iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

        pthread_spin_lock(&remoteServersGlobal->outOfSyncLock);

        rc = ieut_getHashEntry(remoteServersGlobal->outOfSyncServers,
                               serverUID,
                               serverUIDHash,
                               (void **)outOfSyncTime);

        pthread_spin_unlock(&remoteServersGlobal->outOfSyncLock);
    }

    ieutTRACEL(pThreadData, *outOfSyncTime, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}
