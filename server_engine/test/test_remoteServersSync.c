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

/*********************************************************************/
/*                                                                   */
/* Module Name: test_remoteServersSync.c                             */
/*                                                                   */
/* Description: Source file for test of retained message syncing     */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "engineInternal.h"
#include "remoteServersInternal.h"
#include "queueCommon.h"
#include "engineStore.h"            // iest functions & constants
#include "remoteServers.h"          // iers functions & constants
#include "topicTreeInternal.h"      // iett functions & constants
#include "multiConsumerQInternal.h" // iemq functions & constants
#include "engineMonitoring.h"       // iemn functions & constants
#include "engineTimers.h"           // ietm functions & constants

#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_security.h"
#include "test_utils_file.h"
#include "test_utils_client.h"
#include "test_utils_message.h"

static bool invoke_cluster_component = false;

// Override cluster functions to check validity
int32_t ism_cluster_init(void)
{
    int32_t rc;

    static int32_t (*real_ism_cluster_init)(void) = NULL;

    if (real_ism_cluster_init == NULL)
    {
        real_ism_cluster_init = dlsym(RTLD_NEXT, "ism_cluster_init");
    }

    if (invoke_cluster_component && real_ism_cluster_init)
    {
        rc = real_ism_cluster_init();
    }
    else
    {
        rc = OK;
    }

    return rc;
}

int32_t ism_cluster_start(void)
{
    int32_t rc;

    static int32_t (*real_ism_cluster_start)(void) = NULL;

    if (real_ism_cluster_start == NULL)
    {
        real_ism_cluster_start = dlsym(RTLD_NEXT, "ism_cluster_start");
    }

    if (invoke_cluster_component && real_ism_cluster_start)
    {
        rc = real_ism_cluster_start();
    }
    else
    {
        rc = OK;
    }

    return rc;
}


int32_t ism_cluster_term(void)
{
    int32_t rc;

    static int32_t (*real_ism_cluster_term)(void) = NULL;

    if (real_ism_cluster_term == NULL)
    {
        real_ism_cluster_term = dlsym(RTLD_NEXT, "ism_cluster_term");
    }

    if (invoke_cluster_component && real_ism_cluster_term)
    {
        rc = real_ism_cluster_term();
    }
    else
    {
        rc = OK;
    }

    return rc;
}

int32_t ism_cluster_registerProtocolEventCallback(ismProtocol_RemoteServerCallback_t callback, void *pContext)
{
    int32_t rc;

    static int32_t (*real_ism_cluster_registerProtocolEventCallback)(ismProtocol_RemoteServerCallback_t, void *) = NULL;

    if (real_ism_cluster_registerProtocolEventCallback == NULL)
    {
        real_ism_cluster_registerProtocolEventCallback = dlsym(RTLD_NEXT, "ism_cluster_registerProtocolEventCallback");
    }

    if (invoke_cluster_component && real_ism_cluster_registerProtocolEventCallback)
    {
        rc = real_ism_cluster_registerProtocolEventCallback(callback, pContext);
    }
    else
    {
        rc = OK;
    }

    return rc;
}

int ism_cluster_initClusterConfig(void)
{
    int32_t rc;

    static int32_t (*real_ism_cluster_initClusterConfig)(void) = NULL;

    if (real_ism_cluster_initClusterConfig == NULL)
    {
        real_ism_cluster_initClusterConfig = dlsym(RTLD_NEXT, "ism_cluster_initClusterConfig");
    }

    if (invoke_cluster_component && real_ism_cluster_initClusterConfig)
    {
        rc = real_ism_cluster_initClusterConfig();

        // Put back any properties we'd configured
        if (rc == OK)
        {
            rc = test_setConfigProperties(false);
        }
    }
    else
    {
        rc = OK;
    }

    return rc;
}

static ismEngine_RemoteServerCallback_t engineCallback = iers_clusterEventCallback;
static void *engineCallbackContext = NULL;
int32_t ism_cluster_registerEngineEventCallback(ismEngine_RemoteServerCallback_t callback, void *pContext)
{
    int32_t rc;

    static int32_t (*real_ism_cluster_registerEngineEventCallback)(ismEngine_RemoteServerCallback_t, void *) = NULL;

    if (real_ism_cluster_registerEngineEventCallback == NULL)
    {
        real_ism_cluster_registerEngineEventCallback = dlsym(RTLD_NEXT, "ism_cluster_registerEngineEventCallback");
    }

    engineCallback = callback;
    engineCallbackContext = pContext;

    if (invoke_cluster_component && real_ism_cluster_registerEngineEventCallback)
    {
        rc = real_ism_cluster_registerEngineEventCallback(callback, pContext);
    }
    else
    {
        rc = OK;
    }

    return rc;
}

bool check_ism_cluster_restoreRemoteServers = false;
bool force_ism_cluster_restoreRemoteServers_error = false;
uint32_t restoreRemoteServers_callCount = 0;
uint32_t restoreRemoteServers_serverCount = 0;
int32_t ism_cluster_restoreRemoteServers(const ismCluster_RemoteServerData_t *pServersData,
                                         int numServers)
{
    int32_t rc;

    static int32_t (*real_ism_cluster_restoreRemoteServers)(const ismCluster_RemoteServerData_t *, int) = NULL;

    if (real_ism_cluster_restoreRemoteServers == NULL)
    {
        real_ism_cluster_restoreRemoteServers = dlsym(RTLD_NEXT, "ism_cluster_restoreRemoteServers");
    }

    if (check_ism_cluster_restoreRemoteServers == true)
    {
        restoreRemoteServers_callCount += 1;
        restoreRemoteServers_serverCount += numServers;

        // printf("%s(%p,%d) [%u,%u]\n", __func__, pServersData, numServers, restoreRemoteServers_callCount, restoreRemoteServers_serverCount);

        ismCluster_RemoteServerData_t *thisServer = (ismCluster_RemoteServerData_t *)pServersData;
        for(int32_t i=0; i<numServers; i++)
        {
            TEST_ASSERT_PTR_NOT_NULL(thisServer->hServerHandle);
            TEST_ASSERT_PTR_NOT_NULL(thisServer->pRemoteServerName);
            TEST_ASSERT_PTR_NOT_NULL(thisServer->phClusterHandle);

            ismEngine_RemoteServer_t *remoteServer = thisServer->hServerHandle;
            TEST_ASSERT_EQUAL(memcmp(remoteServer->StrucId, ismENGINE_REMOTESERVER_STRUCID, 4), 0);
            TEST_ASSERT_EQUAL(remoteServer->useCount, 1);
            // Check we've passed the expected server UID
            TEST_ASSERT_EQUAL(strcmp(remoteServer->serverUID, thisServer->pRemoteServerUID), 0);
            // Check we've passed the expected server Name
            TEST_ASSERT_EQUAL(strcmp(remoteServer->serverName, thisServer->pRemoteServerName), 0);
            // All should have store handles
            TEST_ASSERT_NOT_EQUAL(remoteServer->hStoreDefn, ismSTORE_NULL_HANDLE);
            TEST_ASSERT_NOT_EQUAL(remoteServer->hStoreProps, ismSTORE_NULL_HANDLE);
            // None should be deleted
            TEST_ASSERT_EQUAL((remoteServer->internalAttrs & iersREMSRVATTR_DELETED), 0);
            // All should be disconnected
            TEST_ASSERT_NOT_EQUAL((remoteServer->internalAttrs & iersREMSRVATTR_DISCONNECTED), 0);
            // Queues should already exist as appropriate
            if ((remoteServer->internalAttrs & iersREMSRVATTR_LOCAL) == 0)
            {
                TEST_ASSERT_EQUAL(thisServer->fLocalServer, false);
                TEST_ASSERT_PTR_NOT_NULL(remoteServer->lowQoSQueueHandle);
                TEST_ASSERT_PTR_NOT_NULL(remoteServer->highQoSQueueHandle);
            }
            else
            {
                TEST_ASSERT_EQUAL(thisServer->fLocalServer, true);
                TEST_ASSERT_PTR_NULL(remoteServer->lowQoSQueueHandle);
                TEST_ASSERT_PTR_NULL(remoteServer->highQoSQueueHandle);
            }
            // Check cluster data passed as expected
            TEST_ASSERT_EQUAL(remoteServer->clusterDataLength, (size_t)(thisServer->dataLength))
            if (remoteServer->clusterDataLength != 0)
            {
                TEST_ASSERT_EQUAL(memcmp(remoteServer->clusterData, thisServer->pData, remoteServer->clusterDataLength), 0);
            }
            // Check that the cluster handle is currently 0, and we've passed the right address
            TEST_ASSERT_EQUAL(remoteServer->clusterHandle, 0);
            TEST_ASSERT_EQUAL(&remoteServer->clusterHandle, thisServer->phClusterHandle)

            thisServer += 1;
        }
    }

    if (force_ism_cluster_restoreRemoteServers_error)
    {
        rc = 999;
    }
    else if (invoke_cluster_component && real_ism_cluster_restoreRemoteServers)
    {
        // Don't pass our test data to the cluster component
        ismCluster_RemoteServerData_t *thisServer = (ismCluster_RemoteServerData_t *)pServersData;
        for(int32_t i=0; i<numServers; i++, thisServer += 1)
        {
            thisServer->dataLength = 0;
        }

        rc = real_ism_cluster_restoreRemoteServers(pServersData, numServers);
    }
    else
    {
        rc = OK;
    }

    return rc;
}

bool check_ism_cluster_recoveryCompleted = false;
bool force_ism_cluster_recoveryCompleted_error = false;
uint32_t recoveryCompleted_callCount = 0;
int32_t ism_cluster_recoveryCompleted(void)
{
    int32_t rc;

    static int32_t (*real_ism_cluster_recoveryCompleted)(void) = NULL;

    if (real_ism_cluster_recoveryCompleted == NULL)
    {
        real_ism_cluster_recoveryCompleted = dlsym(RTLD_NEXT, "ism_cluster_recoveryCompleted");
    }

    if (check_ism_cluster_recoveryCompleted == true)
    {
        recoveryCompleted_callCount += 1;

        // printf("%s() [%u]\n", __func__, recoveryCompleted_callCount);
    }

    if (force_ism_cluster_recoveryCompleted_error)
    {
        rc = 999;
    }
    else if (invoke_cluster_component && real_ism_cluster_recoveryCompleted)
    {
        rc = real_ism_cluster_recoveryCompleted();
    }
    else
    {
        rc = OK;
    }

    return rc;
}

bool check_ism_cluster_addSubscriptions = false;
bool force_ism_cluster_addSubscriptions_error = false;
uint32_t addSubscriptions_callCount = 0;
uint32_t addSubscriptions_topicCount = 0;
uint32_t addSubscriptions_wildcardCount = 0;
int32_t ism_cluster_addSubscriptions(const ismCluster_SubscriptionInfo_t *pSubInfo,
                                     int numSubs)
{
    int32_t rc;

    static int32_t (*real_ism_cluster_addSubscriptions)(const ismCluster_SubscriptionInfo_t *, int) = NULL;

    if (real_ism_cluster_addSubscriptions == NULL)
    {
        real_ism_cluster_addSubscriptions = dlsym(RTLD_NEXT, "ism_cluster_addSubscriptions");
    }

    if (check_ism_cluster_addSubscriptions == true)
    {
        // printf("%s(%p,%d)\n", __func__, pSubInfo, numSubs);

        addSubscriptions_callCount += 1;

        const ismCluster_SubscriptionInfo_t *thisSubInfo = pSubInfo;
        for(int i=0; i<numSubs;i++)
        {
            // printf("%s %d\n", thisSubInfo->pSubscription, thisSubInfo->fWildcard);

            TEST_ASSERT_PTR_NOT_NULL(thisSubInfo->pSubscription);

            addSubscriptions_topicCount += 1;
            if (thisSubInfo->fWildcard) addSubscriptions_wildcardCount += 1;
            thisSubInfo += 1;
        }
    }

    if (force_ism_cluster_addSubscriptions_error)
    {
        rc = 999;
    }
    else if (invoke_cluster_component && real_ism_cluster_addSubscriptions)
    {
        rc = real_ism_cluster_addSubscriptions(pSubInfo, numSubs);
    }
    else
    {
        rc = OK;
    }

    return rc;
}

bool check_ism_cluster_removeSubscriptions = false;
uint32_t removeSubscriptions_callCount = 0;
uint32_t removeSubscriptions_topicCount = 0;
uint32_t removeSubscriptions_wildcardCount = 0;
int32_t ism_cluster_removeSubscriptions(const ismCluster_SubscriptionInfo_t *pSubInfo,
                                        int numSubs)
{
    int32_t rc;

    static int32_t (*real_ism_cluster_removeSubscriptions)(const ismCluster_SubscriptionInfo_t *, int) = NULL;

    if (real_ism_cluster_removeSubscriptions == NULL)
    {
        real_ism_cluster_removeSubscriptions = dlsym(RTLD_NEXT, "ism_cluster_removeSubscriptions");
    }

    if (check_ism_cluster_removeSubscriptions == true)
    {
        // printf("%s(%p,%d)\n", __func__, pSubInfo, numSubs);

        removeSubscriptions_callCount += 1;

        const ismCluster_SubscriptionInfo_t *thisSubInfo = pSubInfo;
        for(int i=0; i<numSubs;i++)
        {
            // printf("%s %d\n", thisSubInfo->pSubscription, thisSubInfo->fWildcard);

            TEST_ASSERT_PTR_NOT_NULL(thisSubInfo->pSubscription);

            removeSubscriptions_topicCount += 1;
            if (thisSubInfo->fWildcard) removeSubscriptions_wildcardCount += 1;
            thisSubInfo += 1;
        }
    }

    if (invoke_cluster_component && real_ism_cluster_removeSubscriptions)
    {
        rc = real_ism_cluster_removeSubscriptions(pSubInfo, numSubs);
    }
    else
    {
        rc = OK;
    }

    return rc;
}

bool check_ism_cluster_routeLookup = false;
char *check_ism_cluster_routeLookup_topic = NULL;
int check_ism_cluster_routeLookup_numDests = -1;
int check_ism_cluster_routeLookup_forceRealloc = 0;
uint32_t routeLookup_callCount = 0;
int32_t ism_cluster_routeLookup(ismCluster_LookupInfo_t *pLookupInfo)
{
    int32_t rc = OK;

    static int32_t (*real_ism_cluster_routeLookup)(ismCluster_LookupInfo_t *) = NULL;

    if (real_ism_cluster_routeLookup == NULL)
    {
        real_ism_cluster_routeLookup = dlsym(RTLD_NEXT, "ism_cluster_routeLookup");
    }

    bool really_invoke_cluster_component = invoke_cluster_component;

    // Check if we should really invoke the cluster component
    if (really_invoke_cluster_component && pLookupInfo != NULL)
    {
        for(int32_t i=0; i<pLookupInfo->numDests; i++)
        {
            // If this is not a real cluster handle, we don't want to call the cluster component
            if (pLookupInfo->phMatchedServers[i] == NULL)
            {
                really_invoke_cluster_component = false;
                break;
            }
            else
            {
                printf("%p\n", pLookupInfo->phMatchedServers[i]);
            }
        }
    }

    if (check_ism_cluster_routeLookup == true)
    {
        routeLookup_callCount += 1;

        // Check it's the expected topic
        if (check_ism_cluster_routeLookup_topic != NULL)
        {
            TEST_ASSERT_EQUAL(strcmp(pLookupInfo->pTopic, check_ism_cluster_routeLookup_topic), 0);
        }

        if (check_ism_cluster_routeLookup_numDests != -1)
        {
            TEST_ASSERT_EQUAL(pLookupInfo->numDests, check_ism_cluster_routeLookup_numDests);
        }

        TEST_ASSERT(pLookupInfo->numDests < pLookupInfo->destsLen, ("array lengths mismatch"));

        for(int32_t i=0; i<pLookupInfo->numDests; i++)
        {
            ismEngine_RemoteServer_t *remoteServer = pLookupInfo->phDests[i];

            TEST_ASSERT_PTR_NOT_NULL(remoteServer);
            TEST_ASSERT_EQUAL(memcmp(remoteServer->StrucId, ismENGINE_REMOTESERVER_STRUCID, 4), 0);
            TEST_ASSERT_PTR_NOT_NULL(remoteServer->serverUID);
            TEST_ASSERT_PTR_NOT_NULL(remoteServer->serverName);
            TEST_ASSERT_EQUAL(remoteServer->clusterHandle, pLookupInfo->phMatchedServers[i]);
            TEST_ASSERT_NOT_EQUAL(remoteServer->useCount, 0);
            // All should have store handles
            TEST_ASSERT_NOT_EQUAL(remoteServer->hStoreDefn, ismSTORE_NULL_HANDLE);
            TEST_ASSERT_NOT_EQUAL(remoteServer->hStoreProps, ismSTORE_NULL_HANDLE);
            // None should be deleted
            TEST_ASSERT_EQUAL((remoteServer->internalAttrs & iersREMSRVATTR_DELETED), 0);
            // Queues should already exist as appropriate
            TEST_ASSERT_PTR_NOT_NULL(remoteServer->lowQoSQueueHandle);
            TEST_ASSERT_PTR_NOT_NULL(remoteServer->highQoSQueueHandle);
        }

        if (check_ism_cluster_routeLookup_forceRealloc > 0)
        {
            check_ism_cluster_routeLookup_forceRealloc--;
            pLookupInfo->numDests = -1;
            rc = ISMRC_ClusterArrayTooSmall;
        }
    }

    // Test above might have forced an error
    if (rc == OK)
    {
        if (really_invoke_cluster_component && real_ism_cluster_routeLookup)
        {
            rc = real_ism_cluster_routeLookup(pLookupInfo);
        }
    }

    return rc;
}

int32_t ism_cluster_setLocalForwardingInfo(const char *pServerName, const char *pServerUID,
                                           const char *pServerAddress, int serverPort,
                                           uint8_t fUseTLS)
{
    int32_t rc = OK;

    static int32_t (*real_ism_cluster_setLocalForwardingInfo)(const char *, const char *, const char *, int, uint8_t) = NULL;

    if (real_ism_cluster_setLocalForwardingInfo == NULL)
    {
        real_ism_cluster_setLocalForwardingInfo = dlsym(RTLD_NEXT, "ism_cluster_setLocalForwardingInfo");
    }

    if (invoke_cluster_component && real_ism_cluster_setLocalForwardingInfo)
    {
        rc = real_ism_cluster_setLocalForwardingInfo(pServerName, pServerUID, pServerAddress, serverPort, fUseTLS);
    }

    return rc;
}

uint32_t updateRetainedStats_callCount = 0;
XAPI int32_t ism_cluster_updateRetainedStats(const char *pServerUID,
                                             void *pData,
                                             uint32_t dataLength)
{
    int32_t rc = OK;

    static int32_t (*real_ism_cluster_updateRetainedStats)(const char *, void *, uint32_t);

    if (real_ism_cluster_updateRetainedStats == NULL)
    {
        real_ism_cluster_updateRetainedStats = dlsym(RTLD_NEXT, "ism_cluster_updateRetainedStats");
    }

    // Check the fields we are sending to the cluster component
    TEST_ASSERT_PTR_NOT_NULL(pServerUID);
    TEST_ASSERT_PTR_NOT_NULL(pData);
    TEST_ASSERT_EQUAL(dataLength, sizeof(iettOriginServerStats_t));

    iettOriginServerStats_t *pStats = (iettOriginServerStats_t *)pData;
    TEST_ASSERT_EQUAL(pStats->version, iettORIGIN_SERVER_STATS_VERSION_1);

    __sync_fetch_and_add(&updateRetainedStats_callCount, 1);

    TEST_ASSERT_GREATER_THAN_OR_EQUAL(pStats->localCount, pStats->count);

#if 0
    printf("SUID: %s count: %u localCount: %u [%u]\n",
           pServerUID, pStats->count, pStats->localCount,
           updateRetainedStats_callCount);
#endif

    if (invoke_cluster_component && real_ism_cluster_updateRetainedStats)
    {
        rc = real_ism_cluster_updateRetainedStats(pServerUID, pData, dataLength);
    }

    return rc;
}

typedef struct tag_fakeLookupRetainedStats_t
{
    char *serverUID;
    uint32_t version;
    char **otherServerUIDs;
    ism_time_t *otherServerHTSs;
    ism_time_t *otherServerHTAs;
    uint32_t *otherServerCounts;
    uint64_t *otherServerTopicsIdentifiers;
} fakeLookupRetainedStats_t;

fakeLookupRetainedStats_t fakeLookupStats[10] = {{0}};

XAPI int32_t ism_cluster_lookupRetainedStats(const char *pServerUID,
                                             ismCluster_LookupRetainedStatsInfo_t **pLookupInfo)
{
    int32_t rc = ISMRC_OK;

    ismCluster_LookupRetainedStatsInfo_t *lookupInfo = NULL;

    for(int32_t i=0; i<sizeof(fakeLookupStats)/sizeof(fakeLookupStats[0]); i++)
    {
        if (fakeLookupStats[i].serverUID == NULL) break;

        if (strcmp(pServerUID, fakeLookupStats[i].serverUID) == 0)
        {
            int32_t count = 0;
            char **otherServerUID = fakeLookupStats[i].otherServerUIDs;
            ism_time_t *otherServerHTS;
            ism_time_t *otherServerHTA;
            uint32_t *otherServerCount;
            uint64_t *otherServerTopicsIdentifier;

            while(*otherServerUID != NULL)
            {
                count++;
                otherServerUID += 1;
            }

            lookupInfo = malloc(sizeof(ismCluster_LookupRetainedStatsInfo_t) +
                                count * (sizeof(ismCluster_RetainedStats_t) + sizeof(iettOriginServerStats_t)));

            TEST_ASSERT_PTR_NOT_NULL(lookupInfo);

            otherServerUID = fakeLookupStats[i].otherServerUIDs;
            otherServerHTS = fakeLookupStats[i].otherServerHTSs;
            otherServerHTA = fakeLookupStats[i].otherServerHTAs;
            otherServerCount = fakeLookupStats[i].otherServerCounts;
            otherServerTopicsIdentifier = fakeLookupStats[i].otherServerTopicsIdentifiers;

            iettOriginServerStats_t originServerStats;
            ismCluster_RetainedStats_t retStats;

            char *curRetStat = (char *)(lookupInfo+1);
            char *curOriginServerStats = curRetStat + (sizeof(ismCluster_RetainedStats_t)*count);

            lookupInfo->pStats = (ismCluster_RetainedStats_t *)curRetStat;
            lookupInfo->numStats = count;

            while(*otherServerUID != NULL)
            {
                retStats.pServerUID = *otherServerUID;
                retStats.pData = curOriginServerStats;
                retStats.dataLength = sizeof(originServerStats);
                memcpy(curRetStat, &retStats, sizeof(retStats));
                curRetStat += sizeof(retStats);

                memset(&originServerStats, 0, sizeof(originServerStats));
                originServerStats.version = fakeLookupStats[i].version;
                originServerStats.highestTimestampSeen = *otherServerHTS;
                originServerStats.highestTimestampAvailable = *otherServerHTA;
                originServerStats.count = *otherServerCount;
                originServerStats.topicsIdentifier = *otherServerTopicsIdentifier;

                memcpy(curOriginServerStats, &originServerStats, sizeof(originServerStats));
                curOriginServerStats += sizeof(originServerStats);

                otherServerUID += 1;
                otherServerHTS += 1;
                otherServerHTA += 1;
                otherServerCount += 1;
                otherServerTopicsIdentifier += 1;
            }
        }
    }

    *pLookupInfo = lookupInfo;

    return rc;
}

XAPI int32_t ism_cluster_freeRetainedStats(ismCluster_LookupRetainedStatsInfo_t *pLookupInfo)
{
    int32_t rc = ISMRC_OK;

    if (pLookupInfo != NULL) free(pLookupInfo);

    return rc;
}

bool expectToFindServerE;
uint32_t expectTotalCount;
char *expectServerE_bestServerUID;
iersSyncLevel_t expectServerE_syncLevel;
iersSyncLevel_t expectServersAC_syncLevel;

void test_checkSyncStats(ieutThreadData_t *pThreadData,
                         iersServerSyncInfo_t *syncInfo)
{
    TEST_ASSERT_PTR_NOT_NULL(syncInfo->serverUID);

    if (strcmp(syncInfo->serverUID, "SEUID") == 0)
    {
        // Should we know about "SEUID"?
        if (expectToFindServerE == true)
        {
            TEST_ASSERT_PTR_NOT_NULL(syncInfo->server);
        }
        else
        {
            TEST_ASSERT_PTR_NULL(syncInfo->server);
        }

        if (expectServerE_bestServerUID != NULL)
        {
            TEST_ASSERT_STRINGS_EQUAL(syncInfo->bestServer[0]->serverUID, expectServerE_bestServerUID);
            TEST_ASSERT_EQUAL(expectServerE_syncLevel, syncInfo->syncLevel);
        }
    }
    else if (strcmp(syncInfo->serverUID, "SBUID") == 0)
    {
        // We should know about "SBUID"
        TEST_ASSERT_PTR_NOT_NULL(syncInfo->server);
        if (expectServersAC_syncLevel == NoneConnected)
        {
            syncInfo->syncLevel = NoneConnected;
        }
        else
        {
            TEST_ASSERT_EQUAL(syncInfo->syncLevel & KnownServer, KnownServer);
        }
    }
    else if ((strcmp(syncInfo->serverUID, "SCUID") == 0) ||
             (strcmp(syncInfo->serverUID, "SAUID") == 0))
    {
        TEST_ASSERT_EQUAL(syncInfo->syncLevel, expectServersAC_syncLevel);
    }
}

void iers_syncAnalyseResults(ieutThreadData_t *pThreadData, iersClusterSyncInfo_t *clusterInfo)
{
    static void (*real_iers_syncAnalyseResults)(ieutThreadData_t *, iersClusterSyncInfo_t *);

    if (real_iers_syncAnalyseResults == NULL)
    {
        real_iers_syncAnalyseResults = dlsym(RTLD_NEXT, "iers_syncAnalyseResults");
    }

    TEST_ASSERT_EQUAL(clusterInfo->allServers->totalCount, expectTotalCount);
    TEST_ASSERT_EQUAL(clusterInfo->inSync, true);

    // Check all the servers match
    iersServerSyncInfo_t *currInfo = clusterInfo->firstAllServer;
    uint32_t listCount = 0;
    while(currInfo != NULL)
    {
        listCount += 1;
        TEST_ASSERT_PTR_NOT_NULL(currInfo->bestServer[0]->serverUID);
        TEST_ASSERT_EQUAL(currInfo->syncLevel, Unevaluated);
        currInfo = currInfo->next;
    }

    // Make sure all the servers in the hash table are on the list
    TEST_ASSERT_EQUAL(clusterInfo->allServers->totalCount, listCount);

    // Call the real function
    real_iers_syncAnalyseResults(pThreadData, clusterInfo);

    currInfo = clusterInfo->firstAllServer;
    while(currInfo != NULL)
    {
        test_checkSyncStats(pThreadData, currInfo);
        currInfo = currInfo->next;
    }

    return;
}

// Simplified Publish Retained messages
void test_publishRetainedMessages(const char *stemTopic,
                                  const char *serverUID,
                                  ism_time_t serverTime,
                                  uint32_t msgCount,
                                  size_t payloadSize,
                                  uint32_t protocolId)
{
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
    rc = test_createClientAndSessionWithProtocol("PublishingClient",
                                                 protocolId,
                                                 NULL,
                                                 clientOpts,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_MessageHandle_t hMessage;
    char pChars[payloadSize];
    void *payload=(void *)&pChars;
    char topicString[strlen(stemTopic)+20];

    if (payloadSize != 0) memset(pChars, 'P', payloadSize);

    for(uint32_t i=1; i<msgCount+1; i++)
    {
        uint8_t messageType;
        uint32_t expiry = 0;

        sprintf(topicString, "%s%u", stemTopic, i);

        // Allow us to publish null retained messages
        if (payloadSize == 0)
        {
            messageType = MTYPE_NullRetained;
            // NullRetained messages from the forwarder should always have a non-zero expiry
            if (protocolId == PROTOCOL_ID_FWD) expiry = ism_common_nowExpire() + 30;
        }
        else
        {
            messageType = MTYPE_MQTT_Text;
        }

        rc = test_createOriginMessage(payloadSize,
                                      ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                      ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                      ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                      expiry,
                                      ismDESTINATION_TOPIC, topicString,
                                      serverUID, serverTime+i,
                                      messageType,
                                      &hMessage, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Accept any informational rc
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

int32_t forwardingCallback_callCount = 0;
int32_t forwardingCallback(const char *respondingServerUID,
                           const char *originServerUID,
                           uint32_t options,
                           ism_time_t timestamp,
                           uint64_t correlationId)
{
    TEST_ASSERT_PTR_NOT_NULL(respondingServerUID);
    TEST_ASSERT_PTR_NOT_NULL(originServerUID);

    if (options == ismENGINE_FORWARD_RETAINED_OPTION_NEWER)
    {
        TEST_ASSERT_NOT_EQUAL(timestamp, 0);
    }
    else
    {
        TEST_ASSERT_EQUAL(options, ismENGINE_FORWARD_RETAINED_OPTION_NONE);
    }

    TEST_ASSERT_NOT_EQUAL(correlationId, 0);

    //printf("%s resp: %s origin: %s options: 0x%08x timestamp: %lu correlationId: 0x%016lx\n",
    //       __func__, respondingServerUID, originServerUID, options, timestamp, correlationId);

    __sync_fetch_and_add(&forwardingCallback_callCount, 1);

    return OK;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_scenario_1                                   */
/*                                                                   */
/* Description:    Test a fixed scenario                             */
/*                                                                   */
/*********************************************************************/
void test_scenario_1(void)
{
    int32_t rc;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    printf("Starting %s...\n", __func__);

    // Don't want to be called back for real low-memory situations!
    iemem_setMemoryReduceCallback(iemem_remoteServers, NULL);

    // Get a forwarding callback in place, and force a short delay.
    ism_engine_registerRetainedForwardingCallback(forwardingCallback);
    ismEngine_serverGlobal.retainedForwardingDelay = 1;

    ismEngine_RemoteServerHandle_t localServerHandle = ismENGINE_NULL_REMOTESERVER_HANDLE;
    ismEngine_RemoteServerHandle_t remoteServerHandle[4] = {ismENGINE_NULL_REMOTESERVER_HANDLE};

    // Create LocalServer record for ServerA
    rc = engineCallback(ENGINE_RS_CREATE_LOCAL, ismENGINE_NULL_REMOTESERVER_HANDLE, (ismCluster_RemoteServerHandle_t)NULL,
                        "ServerA", "SAUID", NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext,
                        &localServerHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(localServerHandle);

    // Create RemoteServer records for ServerB and ServerC
    rc = engineCallback(ENGINE_RS_CREATE, ismENGINE_NULL_REMOTESERVER_HANDLE, (ismCluster_RemoteServerHandle_t)NULL,
                        "ServerB", "SBUID", NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext,
                        &remoteServerHandle[0]);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(remoteServerHandle[0]);
    rc = engineCallback(ENGINE_RS_CREATE, ismENGINE_NULL_REMOTESERVER_HANDLE, (ismCluster_RemoteServerHandle_t)NULL,
                        "ServerC", "SCUID", NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext,
                        &remoteServerHandle[1]);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(remoteServerHandle[1]);
    rc = engineCallback(ENGINE_RS_CREATE, ismENGINE_NULL_REMOTESERVER_HANDLE, (ismCluster_RemoteServerHandle_t)NULL,
                        "ServerX", "SXUID", NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext,
                        &remoteServerHandle[2]);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(remoteServerHandle[2]);
    rc = engineCallback(ENGINE_RS_CREATE, ismENGINE_NULL_REMOTESERVER_HANDLE, (ismCluster_RemoteServerHandle_t)NULL,
                        "ServerY", "SYUID", NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext,
                        &remoteServerHandle[3]);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(remoteServerHandle[3]);

    ism_time_t serverTime = ism_engine_retainedServerTime();

    iettOriginServer_t *originServer;

    // Populate with 10 from ServerA
    test_publishRetainedMessages("/topicsA/", "SAUID", serverTime, 10, 10, PROTOCOL_ID_MQTT);
    rc = iett_insertOrFindOriginServer(pThreadData, "SAUID", iettOP_FIND, &originServer);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(originServer->stats.count, 10);
    TEST_ASSERT_EQUAL(originServer->stats.highestTimestampSeen, serverTime+10);
    TEST_ASSERT_EQUAL(originServer->localServer, true);
    // Populate with 20 from ServerB
    test_publishRetainedMessages("/topicsB/", "SBUID", serverTime, 20, 10, PROTOCOL_ID_FWD);
    rc = iett_insertOrFindOriginServer(pThreadData, "SBUID", iettOP_FIND, &originServer);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(originServer->stats.count, 20);
    TEST_ASSERT_EQUAL(originServer->stats.highestTimestampSeen, serverTime+20);
    TEST_ASSERT_EQUAL(originServer->localServer, false);
    // Populate with 5 from Server C
    test_publishRetainedMessages("/topicsC/", "SCUID", serverTime, 5, 10, PROTOCOL_ID_FWD);
    rc = iett_insertOrFindOriginServer(pThreadData, "SCUID", iettOP_FIND, &originServer);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(originServer->stats.count, 5);
    TEST_ASSERT_EQUAL(originServer->stats.highestTimestampSeen, serverTime+5);
    TEST_ASSERT_EQUAL(originServer->localServer, false);
    // Populate with 10 from Server D
    test_publishRetainedMessages("/topicsD/", "SDUID", serverTime, 10, 10, PROTOCOL_ID_FWD);
    rc = iett_insertOrFindOriginServer(pThreadData, "SDUID", iettOP_FIND, &originServer);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(originServer->stats.count, 10);
    TEST_ASSERT_EQUAL(originServer->stats.highestTimestampSeen, serverTime+10);
    TEST_ASSERT_EQUAL(originServer->localServer, false);
    // Populate with 5 from Server E
    test_publishRetainedMessages("/topicsE/", "SEUID", serverTime, 15, 10, PROTOCOL_ID_FWD);
    rc = iett_insertOrFindOriginServer(pThreadData, "SEUID", iettOP_FIND, &originServer);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(originServer->stats.count, 15);
    TEST_ASSERT_EQUAL(originServer->stats.highestTimestampSeen, serverTime+15);
    TEST_ASSERT_EQUAL(originServer->localServer, false);

    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.ActiveTimerUseCount, 0);
    ismEngine_serverGlobal.ActiveTimerUseCount = 1;

    // Nothing from the cluster (faked or otherwise) - try a sync
    expectTotalCount = 0;
    expectServerE_bestServerUID = NULL;
    ietm_syncClusterRetained(0, 0, NULL);

    // Expect to see all servers in-sync
    ismEngine_RemoteServerStatistics_t stats;
    for(int32_t i=0; i<sizeof(remoteServerHandle)/sizeof(remoteServerHandle[0]); i++)
    {
        rc = ism_engine_getRemoteServerStatistics(remoteServerHandle[i], &stats);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(stats.retainedSync, true);
    }

    // Add information from ServerB and re-run
    char *serverB_otherServerUIDs[] = {"SAUID", "SBUID", "SCUID", "SDUID", "SEUID", "SFUID", "SGUID", NULL};
    ism_time_t serverB_otherServerHTSs[] = {serverTime+10, serverTime+30, serverTime+5, serverTime+20, serverTime+5, serverTime+20, serverTime+30, 0};
    ism_time_t serverB_otherServerHTAs[] = {serverTime+10, serverTime+30, serverTime+5, serverTime+20, serverTime+2, serverTime+20, serverTime+30, 0};
    uint32_t serverB_otherServerCounts[] = {10, 20, 5, 20, 2, 20, 30, 0};
    uint64_t serverB_otherServerTIs[] = {0, 0, 0, 4, 5, 6, 7, 0};

    expectToFindServerE = false;
    fakeLookupStats[0].serverUID = "SBUID";
    fakeLookupStats[0].version = iettORIGIN_SERVER_STATS_VERSION_1;
    fakeLookupStats[0].otherServerUIDs = serverB_otherServerUIDs;
    fakeLookupStats[0].otherServerHTSs = serverB_otherServerHTSs;
    fakeLookupStats[0].otherServerHTAs = serverB_otherServerHTAs;
    fakeLookupStats[0].otherServerCounts = serverB_otherServerCounts;
    fakeLookupStats[0].otherServerTopicsIdentifiers = serverB_otherServerTIs;

    expectServerE_bestServerUID = NULL;
    expectServerE_syncLevel = NoneConnected;
    expectServersAC_syncLevel = NoneConnected;

    for(int32_t loop=0; loop<2; loop++)
    {
        expectTotalCount = 7;
        ietm_syncClusterRetained(0, 0, NULL);

        for(int32_t i=0; i<sizeof(remoteServerHandle)/sizeof(remoteServerHandle[0]); i++)
        {
            rc = ism_engine_getRemoteServerStatistics(remoteServerHandle[i], &stats);
            TEST_ASSERT_EQUAL(rc, OK);

            // TODO: Think about what should be in sync in this loop
        }

        // First loop has nothing connected - subsequent tests we have all connected
        if (loop == 0)
        {
            rc = engineCallback(ENGINE_RS_CONNECT, remoteServerHandle[0], /* ServerB */
                                0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
            rc = engineCallback(ENGINE_RS_CONNECT, remoteServerHandle[1], /* ServerC */
                                0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        expectServerE_bestServerUID = "SBUID";
        expectServerE_syncLevel = LocalHTSHigher|LocalHTAHigher|CountMismatch|TopicsMismatch;
        expectServersAC_syncLevel = Synchronized;
    }

    TEST_ASSERT_EQUAL(forwardingCallback_callCount, 0);

    sleep(1);

    // Fake an existing call being in progress (should result in no callbacks)
    ismEngine_serverGlobal.remoteServers->syncCheckInProgress = true;
    ietm_syncClusterRetained(0, 0, NULL);
    TEST_ASSERT_EQUAL(forwardingCallback_callCount, 0);
    ismEngine_serverGlobal.remoteServers->syncCheckInProgress = false;

    // Actually get a callback
    ietm_syncClusterRetained(0, 0, NULL);
    TEST_ASSERT_EQUAL(forwardingCallback_callCount, 3);

    // Add information from ServerC and re-run
    char *serverC_otherServerUIDs[] = {"SBUID", "SCUID", "SEUID", "SFUID", "SHUID", NULL};
    ism_time_t serverC_otherServerHTSs[] = {serverTime+30, serverTime+5, serverTime+25, serverTime+30, serverTime+10, 0};
    ism_time_t serverC_otherServerHTAs[] = {serverTime+30, serverTime+5, serverTime+25, serverTime+30, serverTime+10, 0};
    uint32_t serverC_otherServerCounts[] = {30, 5, 25, 30, 10, 0};
    uint64_t serverC_otherServerTIs[] = {0, 0, 0, 4, 5, 0};

    fakeLookupStats[1].serverUID = "SCUID";
    fakeLookupStats[1].version = iettORIGIN_SERVER_STATS_CURRENT_VERSION + 1;
    fakeLookupStats[1].otherServerUIDs = serverC_otherServerUIDs;
    fakeLookupStats[1].otherServerHTSs = serverC_otherServerHTSs;
    fakeLookupStats[1].otherServerHTAs = serverC_otherServerHTAs;
    fakeLookupStats[1].otherServerCounts = serverC_otherServerCounts;
    fakeLookupStats[1].otherServerTopicsIdentifiers = serverC_otherServerTIs;

    expectTotalCount = 8;
    expectServerE_bestServerUID = "SCUID";
    expectServerE_syncLevel = ClusterHTSHigher|ClusterHTAHigher|CountMismatch;
    ietm_syncClusterRetained(0, 0, NULL);

    sleep(1);
    ietm_syncClusterRetained(0, 0, NULL);
    TEST_ASSERT_EQUAL(forwardingCallback_callCount, 9);

    sleep(1);
    ietm_syncClusterRetained(0, 0, NULL);

    // Fake a memory limit being in place
    ismEngine_serverGlobal.remoteServers->currentMemLimit = LowQoSLimit;

    sleep(1);
    ietm_syncClusterRetained(0, 0, NULL);
    TEST_ASSERT_EQUAL(forwardingCallback_callCount, 9);

    // Turn off the memory limit
    ismEngine_serverGlobal.remoteServers->currentMemLimit = NoLimit;

    sleep(1);
    ietm_syncClusterRetained(0, 0, NULL);
    TEST_ASSERT_EQUAL(forwardingCallback_callCount, 15);

    ismEngine_serverGlobal.ActiveTimerUseCount = 0;

    // Purposefully not cleaning up - expect test suite cleanup to do it.
}

CU_TestInfo ISM_RemoteServers_Cunit_test_scenario_1[] =
{
    { "Scenario1", test_scenario_1 },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_scenario_2                                   */
/*                                                                   */
/* Description:    Old cluster stats information                     */
/*                                                                   */
/*********************************************************************/
void test_scenario_2(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    // Get a forwarding callback in place, and force a short delay.
    ism_engine_registerRetainedForwardingCallback(forwardingCallback);
    ismEngine_serverGlobal.retainedForwardingDelay = 1;

    ismEngine_RemoteServerHandle_t localServerHandle = ismENGINE_NULL_REMOTESERVER_HANDLE;
    ismEngine_RemoteServerHandle_t remoteServerHandle[1] = {ismENGINE_NULL_REMOTESERVER_HANDLE};

    // Create LocalServer record for ServerA
    rc = engineCallback(ENGINE_RS_CREATE_LOCAL, ismENGINE_NULL_REMOTESERVER_HANDLE, (ismCluster_RemoteServerHandle_t)NULL,
                        "ServerA", "SAUID", NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext,
                        &localServerHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(localServerHandle);

    // Create RemoteServer record for ServerE
    rc = engineCallback(ENGINE_RS_CREATE, ismENGINE_NULL_REMOTESERVER_HANDLE, (ismCluster_RemoteServerHandle_t)NULL,
                        "ServerE", "SEUID", NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext,
                        &remoteServerHandle[0]);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(remoteServerHandle[0]);

    ism_time_t serverTime = ism_engine_retainedServerTime();

    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.ActiveTimerUseCount, 0);
    ismEngine_serverGlobal.ActiveTimerUseCount = 1;

    // Add information from ServerE run
    char *serverB_otherServerUIDs[] = {"SAUID", "SEUID", NULL};
    ism_time_t serverB_otherServerHTSs[] = {serverTime+10, serverTime+30, 0};
    ism_time_t serverB_otherServerHTAs[] = {serverTime+10, serverTime+30, 0};
    uint32_t serverB_otherServerCounts[] = {0, 0, 0};
    uint64_t serverB_otherServerTIs[] = {1, 2, 0};

    fakeLookupStats[0].serverUID = "SEUID";
    fakeLookupStats[0].version = iettORIGIN_SERVER_STATS_VERSION_1;
    fakeLookupStats[0].otherServerUIDs = serverB_otherServerUIDs;
    fakeLookupStats[0].otherServerHTSs = serverB_otherServerHTSs;
    fakeLookupStats[0].otherServerHTAs = serverB_otherServerHTAs;
    fakeLookupStats[0].otherServerCounts = serverB_otherServerCounts;
    fakeLookupStats[0].otherServerTopicsIdentifiers = serverB_otherServerTIs;

    rc = engineCallback(ENGINE_RS_CONNECT, remoteServerHandle[0], /* ServerE */
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    expectToFindServerE = true;
    expectServerE_bestServerUID = "SEUID";
    expectServerE_bestServerUID = NULL;
    expectServerE_syncLevel = Synchronized;
    expectServersAC_syncLevel = Synchronized;

    // Reinitialize the callback count
    forwardingCallback_callCount = 0;

    ismEngine_RemoteServerStatistics_t stats;
    expectTotalCount = 2;
    ietm_syncClusterRetained(0, 0, NULL);

    for(int32_t i=0; i<sizeof(remoteServerHandle)/sizeof(remoteServerHandle[0]); i++)
    {
        rc = ism_engine_getRemoteServerStatistics(remoteServerHandle[i], &stats);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    TEST_ASSERT_EQUAL(forwardingCallback_callCount, 0);

    ismEngine_serverGlobal.ActiveTimerUseCount = 0;

    // Purposefully not cleaning up - expect test suite cleanup to do it.
}

CU_TestInfo ISM_RemoteServers_Cunit_test_scenario_2[] =
{
    { "Scenario2", test_scenario_2 },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initRemoteServersSync(void)
{
    return test_engineInit_DEFAULT;
}

int termRemoteServersSync(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_RemoteServers_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("Scenario1", initRemoteServersSync, termRemoteServersSync, ISM_RemoteServers_Cunit_test_scenario_1),
    IMA_TEST_SUITE("Scenario2", initRemoteServersSync, termRemoteServersSync, ISM_RemoteServers_Cunit_test_scenario_2),
    CU_SUITE_INFO_NULL
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

    // Force this test to run with clustering enabled
    setenv("IMA_TEST_ENABLE_CLUSTER", "True", true);

    // Force the local serverUID to be "SAUID"
    setenv("IMA_TEST_CLUSTER_LOCALSERVERUID", "SAUID", true);

    char *envValue = getenv("IMA_TEST_INVOKE_CLUSTER_COMPONENT");
    if (envValue != NULL && strcmp(envValue, "True") == 0)
    {
        printf("\n");
        printf("*WARNING* This is an engine unit test. It doesn't create remote servers\n");
        printf("          via the cluster component, so expect *failures* when it attempts\n");
        printf("          to interact with the cluster component!\n");

        invoke_cluster_component = true;
    }
    else
    {
        invoke_cluster_component = false;
    }

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    ism_time_t seedVal = ism_common_currentTimeNanos();

    srand(seedVal);

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_RemoteServers_CUnit_allsuites);

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
