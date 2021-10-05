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
/* Module Name: test_remoteServers.c                                 */
/*                                                                   */
/* Description: Main source file for test of engine Remote Servers   */
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

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_security.h"
#include "test_utils_file.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_sync.h"

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
static int32_t ism_cluster_registerEngineEventCallback_RC = OK;
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
        rc = ism_cluster_registerEngineEventCallback_RC;
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
            TEST_ASSERT(thisServer->hServerHandle != NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));
            TEST_ASSERT(thisServer->pRemoteServerName != NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));
            TEST_ASSERT(thisServer->phClusterHandle != NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));

            ismEngine_RemoteServer_t *remoteServer = thisServer->hServerHandle;
            TEST_ASSERT(memcmp(remoteServer->StrucId, ismENGINE_REMOTESERVER_STRUCID, 4) == 0, ("Assert in %s line %d failed\n", __func__, __LINE__));
            TEST_ASSERT(remoteServer->useCount == 1, ("Assert in %s line %d failed\n", __func__, __LINE__));
            // Check we've passed the expected server UID
            TEST_ASSERT(strcmp(remoteServer->serverUID, thisServer->pRemoteServerUID) == 0, ("Assert in %s line %d failed\n", __func__, __LINE__));
            // Check we've passed the expected server Name
            TEST_ASSERT(strcmp(remoteServer->serverName, thisServer->pRemoteServerName) == 0, ("Assert in %s line %d failed\n", __func__, __LINE__));
            // All should have store handles
            TEST_ASSERT(remoteServer->hStoreDefn != ismSTORE_NULL_HANDLE, ("Assert in %s line %d failed\n", __func__, __LINE__));
            TEST_ASSERT(remoteServer->hStoreProps != ismSTORE_NULL_HANDLE, ("Assert in %s line %d failed\n", __func__, __LINE__));
            // None should be deleted
            TEST_ASSERT((remoteServer->internalAttrs & iersREMSRVATTR_DELETED) == 0, ("Assert in %s line %d failed\n", __func__, __LINE__));
            // All should be disconnected
            TEST_ASSERT((remoteServer->internalAttrs & iersREMSRVATTR_DISCONNECTED) != 0, ("Assert in %s line %d failed\n", __func__, __LINE__));
            // Queues should already exist as appropriate
            if ((remoteServer->internalAttrs & iersREMSRVATTR_LOCAL) == 0)
            {
                TEST_ASSERT(thisServer->fLocalServer == false, ("Assert in %s line %d failed\n", __func__, __LINE__));
                TEST_ASSERT(remoteServer->lowQoSQueueHandle != NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));
                TEST_ASSERT(remoteServer->highQoSQueueHandle != NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));
            }
            else
            {
                TEST_ASSERT(thisServer->fLocalServer == true, ("Assert in %s line %d failed\n", __func__, __LINE__));
                TEST_ASSERT(remoteServer->lowQoSQueueHandle == NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));
                TEST_ASSERT(remoteServer->highQoSQueueHandle == NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));
            }
            // Check cluster data passed as expected
            TEST_ASSERT(remoteServer->clusterDataLength == (size_t)(thisServer->dataLength), ("Assert in %s line %d failed\n", __func__, __LINE__));
            if (remoteServer->clusterDataLength != 0)
            {
                TEST_ASSERT(memcmp(remoteServer->clusterData, thisServer->pData, remoteServer->clusterDataLength) == 0, ("Assert in %s line %d failed\n", __func__, __LINE__));
            }
            // Check that the cluster handle is currently 0, and we've passed the right address
            TEST_ASSERT(remoteServer->clusterHandle == 0, ("Assert in %s line %d failed\n", __func__, __LINE__));
            TEST_ASSERT(&remoteServer->clusterHandle == thisServer->phClusterHandle, ("Assert in %s line %d failed\n", __func__, __LINE__));

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

            TEST_ASSERT(thisSubInfo->pSubscription != NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));

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

            TEST_ASSERT(thisSubInfo->pSubscription != NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));

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
            TEST_ASSERT(strcmp(pLookupInfo->pTopic, check_ism_cluster_routeLookup_topic) == 0, ("Assert in %s line %d failed\n", __func__, __LINE__));
        }

        if (check_ism_cluster_routeLookup_numDests != -1)
        {
            TEST_ASSERT(pLookupInfo->numDests == check_ism_cluster_routeLookup_numDests, ("Assert in %s line %d failed\n", __func__, __LINE__));
        }

        TEST_ASSERT(pLookupInfo->numDests < pLookupInfo->destsLen, ("array lengths mismatch"));

        for(int32_t i=0; i<pLookupInfo->numDests; i++)
        {
            ismEngine_RemoteServer_t *remoteServer = pLookupInfo->phDests[i];

            TEST_ASSERT(remoteServer != NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));
            TEST_ASSERT(memcmp(remoteServer->StrucId, ismENGINE_REMOTESERVER_STRUCID, 4) == 0, ("Assert in %s line %d failed\n", __func__, __LINE__));
            TEST_ASSERT(remoteServer->serverUID != NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));
            TEST_ASSERT(remoteServer->serverName != NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));
            TEST_ASSERT(remoteServer->clusterHandle == pLookupInfo->phMatchedServers[i], ("Assert in %s line %d failed\n", __func__, __LINE__));
            TEST_ASSERT(remoteServer->useCount != 0, ("Assert in %s line %d failed\n", __func__, __LINE__));
            // All should have store handles
            TEST_ASSERT(remoteServer->hStoreDefn != ismSTORE_NULL_HANDLE, ("Assert in %s line %d failed\n", __func__, __LINE__));
            TEST_ASSERT(remoteServer->hStoreProps != ismSTORE_NULL_HANDLE, ("Assert in %s line %d failed\n", __func__, __LINE__));
            // None should be deleted
            TEST_ASSERT((remoteServer->internalAttrs & iersREMSRVATTR_DELETED) == 0, ("Assert in %s line %d failed\n", __func__, __LINE__));
            // Queues should already exist as appropriate
            TEST_ASSERT(remoteServer->lowQoSQueueHandle != NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));
            TEST_ASSERT(remoteServer->highQoSQueueHandle != NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));
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
    TEST_ASSERT(pServerUID != NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));
    TEST_ASSERT(pData != NULL, ("Assert in %s line %d failed\n", __func__, __LINE__));
    TEST_ASSERT(dataLength == sizeof(iettOriginServerStats_t), ("Assert in %s line %d failed\n", __func__, __LINE__));

    iettOriginServerStats_t *pStats = (iettOriginServerStats_t *)pData;
    TEST_ASSERT(pStats->version == iettORIGIN_SERVER_STATS_VERSION_1, ("Assert in %s line %d failed\n", __func__, __LINE__));

    __sync_fetch_and_add(&updateRetainedStats_callCount, 1);

    // Cannot reliably know that the localCount is >= count (another thread could be actively
    // reducing the values, and have reduced localCount before reducing count) - in general,
    // this should be the case.

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

int32_t ism_cluster_setHealthStatus(ismCluster_HealthStatus_t  healthStatus)
{
    int32_t rc = OK;

    static int32_t (*real_ism_cluster_setHealthStatus)(ismCluster_HealthStatus_t);

    if (real_ism_cluster_setHealthStatus == NULL)
    {
        real_ism_cluster_setHealthStatus = dlsym(RTLD_NEXT, "ism_cluster_setHealthStatus");
    }

    if (invoke_cluster_component && real_ism_cluster_setHealthStatus)
    {
        rc = real_ism_cluster_setHealthStatus(healthStatus);
    }

    return rc;
}

// Swallow FDCs if we're forcing failures
bool swallow_ffdcs = false;
void ieut_ffdc( const char *function
              , uint32_t seqId
              , bool core
              , const char *filename
              , uint32_t lineNo
              , char *label
              , uint32_t retcode
              , ... )
{
    static void (*real_ieut_ffdc)(const char *, uint32_t, bool, const char *, uint32_t, char *, uint32_t, ...) = NULL;

    if (real_ieut_ffdc == NULL)
    {
        real_ieut_ffdc = dlsym(RTLD_NEXT, "ieut_ffdc");
    }

    if (swallow_ffdcs == true)
    {
        return;
    }

    TEST_ASSERT(0, ("Unexpected FFDC from %s:%u", filename, lineNo));
}


iemem_systemMemInfo_t *fakeMemInfo = NULL;

// Override the querySystemMemory call to enable limiting
int32_t iemem_querySystemMemory(iemem_systemMemInfo_t *sysmeminfo)
{
    int32_t rc = OK;

    static int32_t (*real_iemem_querySystemMemory)(iemem_systemMemInfo_t *) = NULL;

    if (real_iemem_querySystemMemory == NULL)
    {
        real_iemem_querySystemMemory = dlsym(RTLD_NEXT, "iemem_querySystemMemory");
    }

    if (fakeMemInfo != NULL)
    {
        memcpy(sysmeminfo, fakeMemInfo, sizeof(iemem_systemMemInfo_t));
    }
    else
    {
        rc = real_iemem_querySystemMemory(sysmeminfo);
    }

    return rc;
}

// Just directly call the remote server's memory usage callback without needing memory
// to really be low.
void test_analyseMemoryUsage(iememMemoryLevel_t currState,
                             iememMemoryLevel_t prevState,
                             iemem_memoryType type,
                             uint64_t currentLevel,
                             iemem_systemMemInfo_t *memInfo)
{
    //Check we are being called for the remoteServer type we expected
    TEST_ASSERT_EQUAL(type, iemem_remoteServers);

    // Call the real remote server memory usage function
    iers_analyseMemoryUsage(currState, prevState, type, currentLevel, memInfo);
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_countAsyncPut                                */
/*                                                                   */
/* Description:    Decrement a counter of outstand async puts.       */
/*                                                                   */
/*********************************************************************/
void test_countAsyncPut(int32_t retcode, void *handle, void *pContext)
{
    uint32_t *pPutCount = *(uint32_t **)pContext;

    if (retcode != OK) TEST_ASSERT_EQUAL(retcode, ISMRC_NoMatchingDestinations);

    __sync_fetch_and_sub(pPutCount, 1);
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_dumpTopicTree                                */
/*                                                                   */
/* Description:    Test ism_engine_dumpTopicTree (part of which is   */
/*                 to dump the remote server records).               */
/*                                                                   */
/*********************************************************************/
void test_dumpTopicTree(const char *rootTopicString)
{
    char tempDirectory[500] = {0};

    test_utils_getTempPath(tempDirectory);
    TEST_ASSERT_NOT_EQUAL(tempDirectory[0], 0);

    strcat(tempDirectory, "/test_remoteServers_debug_XXXXXX");

    if (mkdtemp(tempDirectory) == NULL)
    {
        TEST_ASSERT(false, ("mkdtemp() failed with %d\n", errno));
    }

    // Test the topic debugging function with 10 subscribers
    char cwdbuf[500];
    char *cwd = getcwd(cwdbuf, sizeof(cwdbuf));
    TEST_ASSERT_PTR_NOT_NULL(cwd);

    if (chdir(tempDirectory) == 0)
    {
        char tempFilename[strlen(tempDirectory)+30];
        int32_t rc;

        sprintf(tempFilename, "%s/dumpTopicTree_1", tempDirectory);
        rc = ism_engine_dumpTopicTree(rootTopicString, 1, 0, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpTopicTree_1.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(tempFilename, "%s/dumpTopicTree_7", tempDirectory);
        rc = ism_engine_dumpTopicTree(rootTopicString, 7, 0, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpTopicTree_7.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(tempFilename, "%s/dumpTopicTree_9_0", tempDirectory);
        rc = ism_engine_dumpTopicTree(rootTopicString, 9, 0, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpTopicTree_9_0.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(tempFilename, "%s/dumpTopicTree_9_50", tempDirectory);
        rc = ism_engine_dumpTopicTree(rootTopicString, 9, 50, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpTopicTree_9_50.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(tempFilename, "%s/dumpTopicTree_9_All", tempDirectory);
        rc = ism_engine_dumpTopicTree(rootTopicString, 9, -1, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpTopicTree_9_All.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        if(chdir(cwd) != 0)
        {
            TEST_ASSERT(false, ("chdir() back to old cwd failed with errno %d\n", errno));
        }
    }

    test_removeDirectory(tempDirectory);
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
    rc = test_createClientAndSessionWithProtocol("PublishingClient",
                                                 protocolId,
                                                 NULL,
                                                 clientOpts,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

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

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        // Allow for informational return codes
        if (expectRC == OK)
        {
            TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, expectRC);
        }

        if (payload) free(payload);
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test that structures are initialized as expected
//****************************************************************************
void test_capability_BasicInitialization(void)
{
    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    TEST_ASSERT_PTR_NOT_NULL(remoteServersGlobal);
    TEST_ASSERT_EQUAL(memcmp(remoteServersGlobal->strucId, iersREMOTESERVERS_STRUCID, 4), 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);
}

//****************************************************************************
/// @brief Test the basic creation / removal of remote servers
//****************************************************************************
void test_capability_BasicCreateRemove(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    for(int32_t loop=0; loop<3; loop++)
    {
        TEST_ASSERT_PTR_NOT_NULL(remoteServersGlobal);
        TEST_ASSERT_EQUAL(memcmp(remoteServersGlobal->strucId, iersREMOTESERVERS_STRUCID, 4), 0);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
        TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
        TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);

        ismEngine_RemoteServerHandle_t localServerHandle = ismENGINE_NULL_REMOTESERVER_HANDLE;

#if defined(NDEBUG)
        // Try passing an invalid eventType - can only do this if we don't check for NULL remoteServer
        rc = engineCallback(0,
                            ismENGINE_NULL_REMOTESERVER_HANDLE,
                            (ismCluster_RemoteServerHandle_t)NULL,
                            "GarbageName",
                            "GarbageUID",
                            NULL, 0,
                            NULL, 0,
                            false, false, NULL, NULL,
                            engineCallbackContext,
                            &localServerHandle);
        TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);
        TEST_ASSERT_EQUAL(localServerHandle, ismENGINE_NULL_REMOTESERVER_HANDLE);
#endif

        // Create a local server structure
        rc = engineCallback(ENGINE_RS_CREATE_LOCAL,
                            ismENGINE_NULL_REMOTESERVER_HANDLE,
                            (ismCluster_RemoteServerHandle_t)NULL,
                            "LOCALSERVER",
                            "LSUID",
                            NULL, 0,
                            NULL, 0,
                            false, false, NULL, NULL,
                            engineCallbackContext,
                            &localServerHandle);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(localServerHandle);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 1);
        TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
        TEST_ASSERT_EQUAL(localServerHandle->internalAttrs, iersREMSRVATTR_LOCAL | iersREMSRVATTR_DISCONNECTED);

        ismEngine_RemoteServerHandle_t createdHandle[3] = {ismENGINE_NULL_REMOTESERVER_HANDLE};

        // Use cluster callback interface to create 3 servers
        for(uint64_t i=1; i<=3; i++)
        {
            char serverName[10];
            char serverUID[10];

            sprintf(serverName, "TEST%lu", i);
            sprintf(serverUID, "UID%lu", i);

            rc = engineCallback(ENGINE_RS_CREATE,
                                ismENGINE_NULL_REMOTESERVER_HANDLE,
                                (ismCluster_RemoteServerHandle_t)NULL,
                                serverName,
                                serverUID,
                                NULL, 0,
                                NULL, 0,
                                false, false, NULL, NULL,
                                engineCallbackContext,
                                &createdHandle[i-1]);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(createdHandle[i-1]);
            TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, i+1);
            TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, i);
            TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle[i-1]);

            // Check that the remote server looks as expected
            ismEngine_RemoteServer_t *remoteServer = createdHandle[i-1];
            TEST_ASSERT_EQUAL(memcmp(remoteServer->StrucId, ismENGINE_REMOTESERVER_STRUCID, 4), 0);
            TEST_ASSERT_EQUAL(remoteServer->useCount, 1);
            TEST_ASSERT_EQUAL(remoteServer->clusterHandle, (ismCluster_RemoteServerHandle_t)NULL);
            TEST_ASSERT_EQUAL(remoteServer->internalAttrs, iersREMSRVATTR_DISCONNECTED);

            // Randomly change between disconnected and connected state
            uint32_t r=0;
            for(int32_t j=rand()%50; j>=0; j--)
            {
                r = rand()%2;
                rc = engineCallback(r==0?ENGINE_RS_DISCONNECT:ENGINE_RS_CONNECT,
                                    remoteServer,
                                    0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
            }

            if (r == 0)
            {
                TEST_ASSERT_EQUAL(remoteServer->internalAttrs, iersREMSRVATTR_DISCONNECTED);
            }
            else
            {
                TEST_ASSERT_EQUAL(remoteServer->internalAttrs, iersREMSRVATTR_NONE);
            }

            TEST_ASSERT_PTR_NOT_NULL(remoteServer->lowQoSQueueHandle);
            ismEngine_Queue_t *pQueue = (ismEngine_Queue_t *)remoteServer->lowQoSQueueHandle;
            TEST_ASSERT_EQUAL(strcmp(pQueue->PolicyInfo->name, "LowQoSPolicy"), 0);

            TEST_ASSERT_PTR_NOT_NULL(remoteServer->highQoSQueueHandle);
            pQueue = (ismEngine_Queue_t *)remoteServer->highQoSQueueHandle;
            TEST_ASSERT_EQUAL(strcmp(pQueue->PolicyInfo->name, "HighQoSPolicy"), 0);
            TEST_ASSERT_EQUAL(remoteServer->hStoreDefn, ieq_getDefnHdl(remoteServer->highQoSQueueHandle));
            TEST_ASSERT_EQUAL(remoteServer->hStoreProps, ieq_getPropsHdl(remoteServer->highQoSQueueHandle));

            ismEngine_RemoteServerStatistics_t remoteServerStats;
            rc = ism_engine_getRemoteServerStatistics((ismEngine_RemoteServerHandle_t)remoteServer,
                                                      &remoteServerStats);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_EQUAL(remoteServerStats.retainedSync, true);
            TEST_ASSERT_STRINGS_EQUAL(remoteServerStats.serverUID, remoteServer->serverUID);
            TEST_ASSERT_EQUAL(remoteServerStats.q0.BufferedMsgs, 0);
            TEST_ASSERT_EQUAL(remoteServerStats.q1.BufferedMsgs, 0);
        }

        // Just test the dumping routines
        if (loop == 2) test_dumpTopicTree(NULL);

        // Remove in a prescribed order to try all list removal types
        rc = engineCallback(ENGINE_RS_REMOVE,
                            createdHandle[1],
                            0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 3);
        TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 2);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle[2]);

        rc = engineCallback(ENGINE_RS_REMOVE,
                            localServerHandle,
                            0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 2);
        TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 2);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle[2]);

        rc = engineCallback(ENGINE_RS_REMOVE,
                            createdHandle[0],
                            0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 1);
        TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 1);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle[2]);

        rc = engineCallback(ENGINE_RS_REMOVE,
                            createdHandle[2],
                            0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
        TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
        TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);
    }
}

//****************************************************************************
/// @brief Test the batching of ENGINE_RS_UPDATE calls
//****************************************************************************
void test_capability_BatchingUpdates(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;
    ismEngine_RemoteServerHandle_t remoteServerHandle[4] = {ismENGINE_NULL_REMOTESERVER_HANDLE};

    // Create some remote server definitions to play with
    rc = engineCallback(ENGINE_RS_CREATE_LOCAL,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        (ismCluster_RemoteServerHandle_t)NULL,
                        "LOCALSERVER",
                        "LSUID",
                        NULL, 0,
                        NULL, 0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        &remoteServerHandle[0]);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(remoteServerHandle[0]);

    uint32_t handleCount=(sizeof(remoteServerHandle)/sizeof(remoteServerHandle[0]));

    for(uint32_t loop=1; loop<handleCount; loop++)
    {
        char serverName[10];
        char serverUID[10];

        sprintf(serverName, "TEST%u", loop);
        sprintf(serverUID, "UID%u", loop);

        rc = engineCallback(ENGINE_RS_CREATE,
                            ismENGINE_NULL_REMOTESERVER_HANDLE,
                            (ismCluster_RemoteServerHandle_t)(uint64_t)NULL,
                            serverName,
                            serverUID,
                            NULL, 0,
                            NULL, 0,
                            false, false, NULL, NULL,
                            engineCallbackContext,
                            &remoteServerHandle[loop]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(remoteServerHandle[loop]);
    }

    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, handleCount-1); // Not the local server
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, handleCount);

    // Try non-pending updates with no remote server specified
    rc = engineCallback(ENGINE_RS_UPDATE,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ArgNotValid);

    ismEngine_RemoteServer_PendingUpdateHandle_t pendingUpdate = NULL;

    rc = engineCallback(ENGINE_RS_UPDATE,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        0, NULL, NULL, NULL, 0, NULL, 0, false, true, &pendingUpdate, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ArgNotValid);
    TEST_ASSERT_PTR_NULL(pendingUpdate);

    // Try a batch request that doesn't use the batch
    rc = engineCallback(ENGINE_RS_UPDATE,
                        remoteServerHandle[1],
                        NULL, NULL, NULL,
                        "CHANGE", strlen("CHANGE")+1,
                        NULL, 0,
                        false, true, &pendingUpdate, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NULL(pendingUpdate);
    TEST_ASSERT_EQUAL(remoteServerHandle[1]->useCount, 1); // useCount unchanged

    // Try a batch that has only one item in it
    rc = engineCallback(ENGINE_RS_UPDATE,
                        remoteServerHandle[1],
                        NULL, NULL, NULL,
                        "CHANGE", strlen("CHANGE")+1,
                        NULL, 0,
                        false, false, &pendingUpdate, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pendingUpdate);
    TEST_ASSERT_EQUAL(remoteServerHandle[1]->useCount, 2);

    rc = engineCallback(ENGINE_RS_UPDATE,
                        NULL, NULL, NULL, NULL, NULL, 0, NULL, 0, false, true, &pendingUpdate, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NULL(pendingUpdate);
    TEST_ASSERT_EQUAL(remoteServerHandle[1]->useCount, 1);

    // Try an update that then changes to NULL
    for(int32_t i=0; i<2; i++)
    {
        rc = engineCallback(ENGINE_RS_UPDATE,
                            remoteServerHandle[0],
                            NULL, NULL, NULL,
                            "CHANGE", strlen("CHANGE")+1,
                            NULL, 0,
                            false, false, &pendingUpdate, NULL,
                            engineCallbackContext,
                            NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(pendingUpdate);
        TEST_ASSERT_EQUAL(remoteServerHandle[0]->useCount, 2);

        rc = engineCallback(ENGINE_RS_UPDATE,
                            remoteServerHandle[i], // NOTE: 0 on 1st loop, 1 on 2nd.
                            NULL, NULL, NULL,
                            NULL, 0,
                            NULL, 0,
                            false, true, &pendingUpdate, NULL,
                            engineCallbackContext,
                            NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NULL(pendingUpdate);
        TEST_ASSERT_EQUAL(remoteServerHandle[i]->useCount, 1);
    }

    // Try updating all remote servers
    for(uint32_t loop=0; loop<handleCount-1; loop++)
    {
        TEST_ASSERT_EQUAL(remoteServerHandle[loop]->useCount, 1);
        rc = engineCallback(ENGINE_RS_UPDATE,
                            remoteServerHandle[loop],
                            NULL, NULL, NULL,
                            "NEW", strlen("NEW")+1,
                            NULL, 0,
                            false, false, &pendingUpdate, NULL,
                            engineCallbackContext,
                            NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(pendingUpdate);
        TEST_ASSERT_EQUAL(remoteServerHandle[loop]->useCount, 2);
    }

    rc = engineCallback(ENGINE_RS_UPDATE,
                        remoteServerHandle[0],
                        NULL, NULL, NULL,
                        "BOO", strlen("BOO")+1,
                        NULL, 0,
                        false, false, &pendingUpdate, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pendingUpdate);
    TEST_ASSERT_NOT_EQUAL((remoteServerHandle[0]->internalAttrs & iersREMSRVATTR_PENDING_UPDATE), 0);
    TEST_ASSERT_EQUAL(remoteServerHandle[0]->useCount, 2);

    // Remove one that is mid-update and prove that it is still pending
    rc = engineCallback(ENGINE_RS_REMOVE,
                        remoteServerHandle[1],
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(remoteServerHandle[1]->useCount, 1);
    TEST_ASSERT_EQUAL((remoteServerHandle[1]->internalAttrs & (iersREMSRVATTR_DELETED | iersREMSRVATTR_PENDING_UPDATE)),
                      (iersREMSRVATTR_DELETED | iersREMSRVATTR_PENDING_UPDATE));
    TEST_ASSERT_PTR_NULL(remoteServerHandle[1]->next);
    TEST_ASSERT_PTR_NULL(remoteServerHandle[1]->prev);
    TEST_ASSERT_PTR_NOT_NULL(remoteServerHandle[1]->clusterData);
    TEST_ASSERT_NOT_EQUAL(remoteServerHandle[1]->clusterDataLength, 0);

    rc = engineCallback(ENGINE_RS_UPDATE,
                        remoteServerHandle[handleCount-1],
                        NULL, NULL, NULL,
                        "NEW", strlen("NEW")+1,
                        NULL, 0,
                        false, true, &pendingUpdate, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NULL(pendingUpdate);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, handleCount-2); // Servers been removed
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, handleCount-1);

    // Try an update to the name (2nd loop is a no-op update)
    TEST_ASSERT_NOT_EQUAL(strcmp(remoteServerHandle[0]->serverName, "NEWSERVERNAME"), 0);
    for(int32_t loop=0; loop<2; loop++)
    {
        char *prevNamePtr = remoteServerHandle[0]->serverName;
        rc = engineCallback(ENGINE_RS_UPDATE,
                            remoteServerHandle[0],
                            NULL, "NEWSERVERNAME", NULL,
                            "TEST", strlen("TEST")+1,
                            NULL, 0,
                            false, true, NULL, NULL,
                            engineCallbackContext,
                            NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(strcmp(remoteServerHandle[0]->serverName, "NEWSERVERNAME"), 0);
        if (loop == 0)
        {
            TEST_ASSERT_NOT_EQUAL(prevNamePtr, remoteServerHandle[0]->serverName);
        }
        else
        {
            TEST_ASSERT_EQUAL(prevNamePtr, remoteServerHandle[0]->serverName);
        }
    }

    // Force the deferred free
    rc = engineCallback(ENGINE_RS_UPDATE,
                        remoteServerHandle[0],
                        NULL, "DifferentServerName", NULL,
                        "TEST", strlen("TEST")+1,
                        NULL, 0,
                        false, true, NULL, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(strcmp(remoteServerHandle[0]->serverName, "DifferentServerName"), 0);

    // Remove all servers (in reverse order)
    for(int32_t loop=handleCount-1; loop>=0; loop--)
    {
        if (loop != 1)
        {
            TEST_ASSERT_EQUAL((remoteServerHandle[loop]->internalAttrs & iersREMSRVATTR_PENDING_UPDATE), 0);
            TEST_ASSERT_EQUAL(remoteServerHandle[loop]->useCount, 1);
            rc = engineCallback(ENGINE_RS_REMOVE,
                                remoteServerHandle[loop],
                                0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
        }

        TEST_ASSERT_EQUAL(rc, OK);
    }

    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
}

//****************************************************************************
/// @brief Test the basic setting / unsetting of route all mode
//****************************************************************************
void test_capability_BasicRouteAll(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    ismEngine_RemoteServerHandle_t serverHandle = ismENGINE_NULL_REMOTESERVER_HANDLE;

    // Try passing an invalid eventType
    rc = engineCallback(ENGINE_RS_CREATE,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        (ismCluster_RemoteServerHandle_t)NULL,
                        "TestServer",
                        "TUID",
                        NULL, 0,
                        NULL, 0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        &serverHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(serverHandle);
    TEST_ASSERT_EQUAL(strcmp(serverHandle->serverName, "TestServer"), 0);
    TEST_ASSERT_EQUAL((serverHandle->internalAttrs & iersREMSRVATTR_ROUTE_ALL_MODE), 0);

    // set into 'route all mode' twice
    for(int32_t loop=0; loop<2; loop++)
    {
        rc = engineCallback(ENGINE_RS_ROUTE,
                            serverHandle,
                            0,
                            NULL, NULL,
                            NULL, 0,
                            NULL, 0,
                            true, false, NULL, NULL,
                            engineCallbackContext,
                            NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL((serverHandle->internalAttrs & iersREMSRVATTR_ROUTE_ALL_MODE), iersREMSRVATTR_ROUTE_ALL_MODE);
    }

    // set out of 'route all mode' twice
    for(int32_t loop=0; loop<2; loop++)
    {
        rc = engineCallback(ENGINE_RS_ROUTE,
                            serverHandle,
                            0,
                            NULL, NULL,
                            NULL, 0,
                            NULL, 0,
                            false, false, NULL, NULL,
                            engineCallbackContext,
                            NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL((serverHandle->internalAttrs & iersREMSRVATTR_ROUTE_ALL_MODE), 0);
    }

    rc = engineCallback(ENGINE_RS_REMOVE,
                        serverHandle,
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_capability_SubscriptionReporting             */
/*                                                                   */
/* Description:    Test the correctness of subscription reporting to */
/*                 the clustering component.                         */
/*                                                                   */
/*********************************************************************/
#define NUM_TOPICS 50
void test_capability_SubscriptionReporting(void)
{
    int32_t           rc;

    ismEngine_Consumer_t **consumers = NULL;
    ismEngine_Consumer_t **currentConsumer;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    printf("Starting %s...\n", __func__);

    consumers = calloc(1, sizeof(ismEngine_Consumer_t*)*((NUM_TOPICS*3)+1));

    TEST_ASSERT_PTR_NOT_NULL(consumers);

    check_ism_cluster_addSubscriptions = true;
    addSubscriptions_callCount = addSubscriptions_topicCount = addSubscriptions_wildcardCount = 0;
    check_ism_cluster_removeSubscriptions = true;
    removeSubscriptions_callCount = removeSubscriptions_topicCount = removeSubscriptions_wildcardCount = 0;

    rc = test_createClientAndSession("TEST_CLIENT",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    /* Register Subscribers */
    int32_t expectWildcards = 0;
    int32_t expectTopics = 0;
    currentConsumer = consumers;
    for(int32_t i=0; i<NUM_TOPICS; i++)
    {
        char currentTopicString[40];

        sprintf(currentTopicString, "/Topic/%d", i);
        expectTopics += 1;

        int randomNumber = rand()%40;

        if (randomNumber > 35)
        {
            strcat(currentTopicString, "/+/SOMETHING");
            expectWildcards += 1;
        }
        else if (randomNumber > 30)
        {
            strcat(currentTopicString, "/+");
            expectWildcards += 1;
        }
        else if (randomNumber > 20)
        {
            strcat(currentTopicString, "/#");
            expectWildcards += 1;
        }

        // Add between 1 and 3 subscriptions
        randomNumber = (rand()%3)+1;
        for(int32_t c=0; c<randomNumber; c++)
        {
            // This will result in an ism_engine_registerSubscriber call
            subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
            rc = ism_engine_createConsumer(hSession,
                                           ismDESTINATION_TOPIC,
                                           currentTopicString,
                                           &subAttrs,
                                           NULL, // Unused for TOPIC
                                           NULL,
                                           0,
                                           NULL, /* No delivery callback */
                                           NULL,
                                           ismENGINE_CONSUMER_OPTION_NONE,
                                           currentConsumer,
                                           NULL,
                                           0,
                                           NULL);
            TEST_ASSERT_EQUAL(rc, OK);
            currentConsumer += 1;
        }
    }

    TEST_ASSERT_EQUAL(addSubscriptions_callCount, expectTopics);
    TEST_ASSERT_EQUAL(addSubscriptions_topicCount, expectTopics);
    TEST_ASSERT_EQUAL(addSubscriptions_wildcardCount, expectWildcards);
    TEST_ASSERT_EQUAL(removeSubscriptions_callCount, 0);
    TEST_ASSERT_EQUAL(removeSubscriptions_topicCount, 0);
    TEST_ASSERT_EQUAL(removeSubscriptions_wildcardCount, 0);

    currentConsumer = consumers;
    while(*currentConsumer)
    {
        rc = ism_engine_destroyConsumer(*currentConsumer,
                                        NULL,
                                        0,
                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        currentConsumer += 1;
    }

    TEST_ASSERT_EQUAL(addSubscriptions_callCount, expectTopics);
    TEST_ASSERT_EQUAL(addSubscriptions_topicCount, expectTopics);
    TEST_ASSERT_EQUAL(addSubscriptions_wildcardCount, expectWildcards);
    TEST_ASSERT_EQUAL(removeSubscriptions_callCount, expectTopics);
    TEST_ASSERT_EQUAL(removeSubscriptions_topicCount, expectTopics);
    TEST_ASSERT_EQUAL(removeSubscriptions_wildcardCount, expectWildcards);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    free(consumers);

    check_ism_cluster_addSubscriptions = false;
    addSubscriptions_callCount = addSubscriptions_topicCount = addSubscriptions_wildcardCount = 0;
    check_ism_cluster_removeSubscriptions = false;
    removeSubscriptions_callCount = removeSubscriptions_topicCount = removeSubscriptions_wildcardCount = 0;
}
#undef NUM_TOPICS

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_capability_ClusterRequestedTopicReporting    */
/*                                                                   */
/* Description:    Test the correctness of cluster requested topic   */
/*                 reporting to  the clustering component.           */
/*                                                                   */
/*********************************************************************/
#define NUM_TOPICS 50
void test_capability_ClusterRequestedTopicReporting(void)
{
    int32_t           rc;

    printf("Starting %s...\n", __func__);

    check_ism_cluster_addSubscriptions = true;
    addSubscriptions_callCount = addSubscriptions_topicCount = addSubscriptions_wildcardCount = 0;
    check_ism_cluster_removeSubscriptions = true;
    removeSubscriptions_callCount = removeSubscriptions_topicCount = removeSubscriptions_wildcardCount = 0;

    /* Create ClusterRequestedTopics */
    int32_t expectWildcards = 0;
    int32_t expectTopics = 0;
    ism_field_t f;
    ism_prop_t *clusterRequestedTopicProps = ism_common_newProperties(2);
    ism_ConfigChangeType_t changeType = ISM_CONFIG_CHANGE_PROPS;
    while(1)
    {
        for(int32_t i=0; i<NUM_TOPICS; i++)
        {
            char currentTopicString[40];

            sprintf(currentTopicString, "/CRTopic/%d", i);
            expectTopics += 1;

            if (i > 45)
            {
                strcat(currentTopicString, "/+/SOMETHING");
                expectWildcards += 1;
            }
            else if (i > 40)
            {
                strcat(currentTopicString, "/+");
                expectWildcards += 1;
            }
            else if (i > 30)
            {
                strcat(currentTopicString, "/#");
                expectWildcards += 1;
            }

            f.type = VT_String;
            f.val.s = currentTopicString;
            ism_common_setProperty(clusterRequestedTopicProps,
                                   ismENGINE_ADMIN_PREFIX_CLUSTERREQUESTEDTOPICS_TOPICSTRING "TEST",
                                   &f);

            // Request between 1 and 3 times on update (only once on delete)
            int32_t randomNumber = changeType == ISM_CONFIG_CHANGE_DELETE ? 1 : (rand()%3)+1;
            for(int32_t c=0; c<randomNumber; c++)
            {
                rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_CLUSTERREQUESTEDTOPICS,
                                               "TEST",
                                               clusterRequestedTopicProps,
                                               changeType);
                if (c == 0)
                {
                    TEST_ASSERT_EQUAL(rc, OK);
                }
                else
                {
                    TEST_ASSERT_EQUAL(rc, ISMRC_ExistingClusterRequestedTopic);
                }
            }
        }

        TEST_ASSERT_EQUAL(addSubscriptions_callCount, expectTopics);
        TEST_ASSERT_EQUAL(addSubscriptions_topicCount, expectTopics);
        TEST_ASSERT_EQUAL(addSubscriptions_wildcardCount, expectWildcards);

        if (changeType == ISM_CONFIG_CHANGE_PROPS)
        {
            TEST_ASSERT_EQUAL(removeSubscriptions_callCount, 0);
            TEST_ASSERT_EQUAL(removeSubscriptions_topicCount, 0);
            TEST_ASSERT_EQUAL(removeSubscriptions_wildcardCount, 0);
            changeType = ISM_CONFIG_CHANGE_DELETE;
            expectTopics = expectWildcards = 0;
        }
        else
        {
            TEST_ASSERT_EQUAL(changeType, ISM_CONFIG_CHANGE_DELETE);
            TEST_ASSERT_EQUAL(removeSubscriptions_callCount, expectTopics);
            TEST_ASSERT_EQUAL(removeSubscriptions_topicCount, expectTopics);
            TEST_ASSERT_EQUAL(removeSubscriptions_wildcardCount, expectWildcards);
            break;
        }
    }

    TEST_ASSERT_EQUAL(addSubscriptions_callCount, expectTopics);
    TEST_ASSERT_EQUAL(addSubscriptions_topicCount, expectTopics);
    TEST_ASSERT_EQUAL(addSubscriptions_wildcardCount, expectWildcards);
    TEST_ASSERT_EQUAL(removeSubscriptions_callCount, expectTopics);
    TEST_ASSERT_EQUAL(removeSubscriptions_topicCount, expectTopics);
    TEST_ASSERT_EQUAL(removeSubscriptions_wildcardCount, expectWildcards);

    check_ism_cluster_addSubscriptions = false;
    addSubscriptions_callCount = addSubscriptions_topicCount = addSubscriptions_wildcardCount = 0;
    check_ism_cluster_removeSubscriptions = false;
    removeSubscriptions_callCount = removeSubscriptions_topicCount = removeSubscriptions_wildcardCount = 0;
}
#undef NUM_TOPICS

//****************************************************************************
/// @brief Test the basic adding and removal of servers from topics
//****************************************************************************
#define NUM_SERVERS 12
void test_capability_AddRemoveTopics(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);

    ismEngine_RemoteServerHandle_t createdHandle[NUM_SERVERS] = {ismENGINE_NULL_REMOTESERVER_HANDLE};

    // Use cluster callback interface to create the servers
    for(uint64_t i=1; i<=NUM_SERVERS; i++)
    {
        char serverName[20];
        char serverUID[10];

        sprintf(serverName, "ServerNo%02lu", i);
        sprintf(serverUID, "UID%lu", i);

        rc = engineCallback(ENGINE_RS_CREATE,
                            ismENGINE_NULL_REMOTESERVER_HANDLE,
                            (ismCluster_RemoteServerHandle_t)NULL,
                            serverName,
                            serverUID,
                            NULL, 0,
                            NULL, 0,
                            false, false, NULL, NULL,
                            engineCallbackContext,
                            &createdHandle[i-1]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(createdHandle[i-1]);
        TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, i);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, i); // no local
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle[i-1]);
    }

    // Note they don't need to be wild - even though cluster will only use us for
    // wildcard topics.
    char *testTopics[] = {"A/B/C", "A/B/C/#", "A/+/C/#", "A/B/C/+", "#", "/A/B/C/+", "/T"};
    size_t testTopicsCount = sizeof(testTopics)/sizeof(testTopics[0]);

    for(int32_t j=0; j<NUM_SERVERS; j++)
    {
        rc = engineCallback(ENGINE_RS_ADD_SUB,
                            createdHandle[j],
                            0,
                            NULL,
                            NULL,
                            NULL, 0,
                            testTopics, testTopicsCount,
                            false, false, NULL, NULL,
                            engineCallbackContext,
                            NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Re-add on the same topics on one of the remote servers
    rc = engineCallback(ENGINE_RS_ADD_SUB,
                        createdHandle[0],
                        0,
                        NULL,
                        NULL,
                        NULL, 0,
                        testTopics, testTopicsCount,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Test the dumping routines
    test_dumpTopicTree(NULL);

    // Remove from one topic.
    rc = engineCallback(ENGINE_RS_DEL_SUB,
                        createdHandle[0],
                        0,
                        NULL, NULL,
                        NULL, 0,
                        &testTopics[0], 1,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Remove from all topics twice - just to prove unnecessary removal is OK.
    for(int32_t j=0; j<NUM_SERVERS; j++)
    {
        for(int32_t i=0; i<2; i++)
        {
            rc = engineCallback(ENGINE_RS_DEL_SUB,
                                createdHandle[j],
                                0,
                                NULL, NULL,
                                NULL, 0,
                                testTopics, testTopicsCount,
                                false, false, NULL, NULL,
                                engineCallbackContext,
                                NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    // Add to NO topics
    rc = engineCallback(ENGINE_RS_DEL_SUB,
                        createdHandle[0],
                        0,
                        NULL, NULL,
                        NULL, 0,
                        testTopics, 0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    iettTopicTree_t *tree = iett_getEngineTopicTree(pThreadData);

    TEST_ASSERT_EQUAL(tree->multiMultiRemSrvs, 0);

    int32_t z;
    char topicString[100];
    char *topicStrings = topicString;

    for(z=0; z<1000; z++)
    {
        sprintf(topicString, "SUB/LEVEL/%d/OF/1000/#", z+1);

        // Make one in 10 a multi-mutli
        if (z%10 == 1)
        {
            strcat(topicString, "/MULTI/#");
        }

        // Do the add a random number of times.
        for(int32_t x=rand()%3; x>=0; x--)
        {
            rc = engineCallback(ENGINE_RS_ADD_SUB,
                                createdHandle[1],
                                0,
                                NULL, NULL,
                                NULL, 0,
                                &topicStrings, 1,
                                false, false, NULL, NULL,
                                engineCallbackContext,
                                NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    TEST_ASSERT_EQUAL(tree->multiMultiRemSrvs, 100);

    for(int32_t j=0; j<NUM_SERVERS; j++)
    {
        rc = engineCallback(ENGINE_RS_REMOVE,
                            createdHandle[j],
                            0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    TEST_ASSERT_EQUAL(tree->multiMultiRemSrvs, 0);

    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);
}
#undef NUM_SERVERS

//****************************************************************************
/// @brief Test the source (sender) publish loop with remote servers
//****************************************************************************
#define NUM_SERVERS 3
void test_capability_Publishing_Source(void)
{
    int32_t rc;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);

    ismEngine_RemoteServerHandle_t localServerHandle = ismENGINE_NULL_REMOTESERVER_HANDLE;

    rc = engineCallback(ENGINE_RS_CREATE_LOCAL,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        (ismCluster_RemoteServerHandle_t)NULL,
                        "LOCALSERVER",
                        "UID-LOCAL",
                        NULL, 0,
                        NULL, 0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        &localServerHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(localServerHandle);

    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 1);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);

    ismEngine_RemoteServerHandle_t createdHandle[NUM_SERVERS] = {ismENGINE_NULL_REMOTESERVER_HANDLE};

    // Use cluster callback interface to create 'remote' servers
    for(uint64_t i=1; i<=NUM_SERVERS; i++)
    {
        char serverName[20];
        char serverUID[10];

        sprintf(serverName, "ServerNo%02lu", i);
        sprintf(serverUID, "UID%lu", i);

        rc = engineCallback(ENGINE_RS_CREATE,
                            ismENGINE_NULL_REMOTESERVER_HANDLE,
                            (ismCluster_RemoteServerHandle_t)NULL,
                            serverName,
                            serverUID,
                            NULL, 0,
                            NULL, 0,
                            false, false, NULL, NULL,
                            engineCallbackContext,
                            &createdHandle[i-1]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(createdHandle[i-1]);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 1+i);
        TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, i);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle[i-1]);
    }

    char *testTopics0[] = {"A/B/C/#/#", "#", "$SYS/#"};
    size_t testTopicsCount0 = sizeof(testTopics0)/sizeof(testTopics0[0]);

    rc = engineCallback(ENGINE_RS_ADD_SUB,
                        createdHandle[0],
                        0,
                        NULL, NULL,
                        NULL, 0,
                        testTopics0, testTopicsCount0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    char *testTopics1[] = {"A/B/C/+", "$SYS/#"};
    size_t testTopicsCount1 = sizeof(testTopics1)/sizeof(testTopics1[0]);

    rc = engineCallback(ENGINE_RS_ADD_SUB,
                        createdHandle[1],
                        0,
                        NULL, NULL,
                        NULL, 0,
                        testTopics1, testTopicsCount1,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    char *testTopics2[] = {"A/+/C/D", "A/B/C/+", "W/X/Y/Z", "$SYS/#"};
    size_t testTopicsCount2 = sizeof(testTopics2)/sizeof(testTopics2[0]);

    rc = engineCallback(ENGINE_RS_ADD_SUB,
                        createdHandle[2],
                        0,
                        NULL, NULL,
                        NULL, 0,
                        testTopics2, testTopicsCount2,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    uint64_t prevLowQoSBufferedCount[NUM_SERVERS] = {0};
    uint64_t prevHighQoSBufferedCount[NUM_SERVERS] = {0};
    ismEngine_RemoteServerStatistics_t remoteServerStats;

    check_ism_cluster_routeLookup = true;
    check_ism_cluster_routeLookup_numDests = -1;
    check_ism_cluster_routeLookup_topic = NULL;

    routeLookup_callCount = 0;

    uint32_t starting_updateRetainedStats_callCount = updateRetainedStats_callCount;
    uint32_t expectedUpdateRetainedStatsCalls = 0;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;

    rc = test_createClientAndSession("TEST_CLIENT",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a consumer which will adjust the false positive detection stat when we
    // fake publishing from the forwarder - depending on whether it's wildcard or
    // not will change the result.
    bool wildConsumer = (rand()%20 > 10) ? true : false;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   wildConsumer ? "A/+/C/D" : "A/B/C/D",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   NULL,
                                   0,
                                   NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    char *topic[] = {"A/B/C/D", "A/X/C/D", "W/X/Y/Z", "L/M/N/O",
                     "A/B/C/D", "A/B/C/D", "L/M/N/O", "$SYS/TEST",
                     "$SYSTEM", "%SYS/IT/IS/PERCENT/NOT/DOLLAR", NULL};
    bool dupOfPrev[] = {false, false, false, false,
                        true, true, true, false,
                        false, false, false};
    uint64_t expectBufferedCount[][NUM_SERVERS] = {{1,1,1},
                                                   {1,0,1},
                                                   {1,0,1},
                                                   {1,0,0},
                                                   {1,1,1},
                                                   {0,1,1},
                                                   {0,0,0},
                                                   {0,0,0},
                                                   {0,0,0},
                                                   {0,0,0}};
    uint32_t expectRouteLookupCallCount[][4] = {{0,0,0,0},
                                                {1,1,0,0},
                                                {1,1,0,0},
                                                {1,1,0,0},
                                                {0,0,0,0},
                                                {1,1,0,0},
                                                {1,1,0,0},
                                                {0,0,0,0},
                                                {0,0,0,0},
                                                {1,1,0,0}};
    // -5 where not expecting to call at all
    uint32_t expectRouteLookupNumDests[] = {-5,2,2,1,-5,2,0,-5,-5,0};

    uint64_t fwdPublishCount=0;
    int32_t currentTopic = 0;
    while(topic[currentTopic] != NULL)
    {
        // For $SYS topics, we don't even expect retained messages
        int32_t expectRetained;

        if (strncmp(topic[currentTopic], "$SYS", 4) == 0)
            expectRetained = 0;
        else
            expectRetained = 1;

        if (currentTopic == 4 || currentTopic == 5)
        {
            rc = engineCallback(ENGINE_RS_DEL_SUB,
                                createdHandle[0],
                                0,
                                NULL, NULL,
                                NULL, 0,
                                &testTopics0[5-currentTopic], 1,
                                false, false, NULL, NULL,
                                engineCallbackContext,
                                NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        check_ism_cluster_routeLookup_numDests = expectRouteLookupNumDests[currentTopic];

        // Publish a low QoS message
        routeLookup_callCount = 0;
        check_ism_cluster_routeLookup_topic = topic[currentTopic];
        test_publishMessages(topic[currentTopic],
                             NULL,
                             TEST_SMALL_MESSAGE_SIZE,
                             1,
                             ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                             ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                             ismMESSAGE_FLAGS_NONE,
                             PROTOCOL_ID_MQTT,
                             OK);
        TEST_ASSERT_EQUAL(routeLookup_callCount, expectRouteLookupCallCount[currentTopic][0]);

        for(int32_t server=0; server<NUM_SERVERS; server++)
        {
            TEST_ASSERT_EQUAL(createdHandle[server]->useCount, 1);

            rc = ism_engine_getRemoteServerStatistics(createdHandle[server], &remoteServerStats);
            TEST_ASSERT_EQUAL(rc, OK);

            TEST_ASSERT_EQUAL(remoteServerStats.q0.BufferedMsgs, prevLowQoSBufferedCount[server]+expectBufferedCount[currentTopic][server]);
            prevLowQoSBufferedCount[server] = remoteServerStats.q0.BufferedMsgs;
            TEST_ASSERT_EQUAL(remoteServerStats.q1.BufferedMsgs, prevHighQoSBufferedCount[server]);
            prevHighQoSBufferedCount[server] = remoteServerStats.q1.BufferedMsgs;
        }

        // Publish a high QoS message
        routeLookup_callCount = 0;
        check_ism_cluster_routeLookup_topic = topic[currentTopic];
        test_publishMessages(topic[currentTopic],
                             NULL,
                             TEST_SMALL_MESSAGE_SIZE,
                             1,
                             ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                             ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                             ismMESSAGE_FLAGS_NONE,
                             PROTOCOL_ID_MQTT,
                             OK);
        TEST_ASSERT_EQUAL(routeLookup_callCount, expectRouteLookupCallCount[currentTopic][1]);

        for(int32_t server=0; server<NUM_SERVERS; server++)
        {
            TEST_ASSERT_EQUAL(createdHandle[server]->useCount, 1);

            rc = ism_engine_getRemoteServerStatistics(createdHandle[server], &remoteServerStats);
            TEST_ASSERT_EQUAL(rc, OK);

            TEST_ASSERT_EQUAL(remoteServerStats.q0.BufferedMsgs, prevLowQoSBufferedCount[server]);
            prevLowQoSBufferedCount[server] = remoteServerStats.q0.BufferedMsgs;
            TEST_ASSERT_EQUAL(remoteServerStats.q1.BufferedMsgs, prevHighQoSBufferedCount[server]+expectBufferedCount[currentTopic][server]);
            prevHighQoSBufferedCount[server] = remoteServerStats.q1.BufferedMsgs;
        }

        // Publish a retained low QoS message
        routeLookup_callCount = 0;
        check_ism_cluster_routeLookup_topic = topic[currentTopic];
        test_publishMessages(topic[currentTopic],
                             "LowQoSUID",
                             TEST_SMALL_MESSAGE_SIZE,
                             1,
                             ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                             ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                             ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                             PROTOCOL_ID_MQTT,
                             OK);
        TEST_ASSERT_EQUAL(routeLookup_callCount, expectRouteLookupCallCount[currentTopic][2]);

        for(int32_t server=0; server<NUM_SERVERS; server++)
        {
            TEST_ASSERT_EQUAL(createdHandle[server]->useCount, 1);

            rc = ism_engine_getRemoteServerStatistics(createdHandle[server], &remoteServerStats);
            TEST_ASSERT_EQUAL(rc, OK);

            TEST_ASSERT_EQUAL(remoteServerStats.q0.BufferedMsgs, prevLowQoSBufferedCount[server]+expectRetained);
            prevLowQoSBufferedCount[server] = remoteServerStats.q0.BufferedMsgs;
            TEST_ASSERT_EQUAL(remoteServerStats.q1.BufferedMsgs, prevHighQoSBufferedCount[server]);
            prevHighQoSBufferedCount[server] = remoteServerStats.q1.BufferedMsgs;
        }

        expectedUpdateRetainedStatsCalls += 1;
        if (dupOfPrev[currentTopic]) expectedUpdateRetainedStatsCalls += 1;
        dupOfPrev[currentTopic] = true;

        // Publish a retained high QoS message
        routeLookup_callCount = 0;
        check_ism_cluster_routeLookup_topic = topic[currentTopic];
        test_publishMessages(topic[currentTopic],
                             "HighQoSUID",
                             TEST_SMALL_MESSAGE_SIZE,
                             1,
                             ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                             ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                             ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                             PROTOCOL_ID_MQTT,
                             OK);
        TEST_ASSERT_EQUAL(routeLookup_callCount, expectRouteLookupCallCount[currentTopic][3]);

        for(int32_t server=0; server<NUM_SERVERS; server++)
        {
            TEST_ASSERT_EQUAL(createdHandle[server]->useCount, 1);

            rc = ism_engine_getRemoteServerStatistics(createdHandle[server], &remoteServerStats);
            TEST_ASSERT_EQUAL(rc, OK);

            TEST_ASSERT_EQUAL(remoteServerStats.q0.BufferedMsgs, prevLowQoSBufferedCount[server]);
            prevLowQoSBufferedCount[server] = remoteServerStats.q0.BufferedMsgs;
            TEST_ASSERT_EQUAL(remoteServerStats.q1.BufferedMsgs, prevHighQoSBufferedCount[server]+expectRetained);
            prevHighQoSBufferedCount[server] = remoteServerStats.q1.BufferedMsgs;
        }

        expectedUpdateRetainedStatsCalls += 1;
        if (dupOfPrev[currentTopic]) expectedUpdateRetainedStatsCalls += 1;
        dupOfPrev[currentTopic] = true;

        // Publish messages of various type from a faked forwarder client
        for(uint8_t rel=0; rel<6; rel++)
        {
            char serverUID[20];
            uint8_t flags;

            if (rel >= 3)
            {
                flags = ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;
                expectedUpdateRetainedStatsCalls += 2;
            }
            else
            {
                flags = ismMESSAGE_FLAGS_NONE;
            }

            sprintf(serverUID, "REMOTE%2d", rel);

            fwdPublishCount++;
            test_publishMessages(topic[currentTopic],
                                 serverUID,
                                 TEST_SMALL_MESSAGE_SIZE,
                                 1,
                                 rel%3,
                                 ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                 flags,
                                 PROTOCOL_ID_FWD,
                                 OK);
            TEST_ASSERT_EQUAL(routeLookup_callCount, expectRouteLookupCallCount[currentTopic][3]);

            for(int32_t server=0; server<NUM_SERVERS; server++)
            {
                TEST_ASSERT_EQUAL(createdHandle[server]->useCount, 1);

                rc = ism_engine_getRemoteServerStatistics(createdHandle[server], &remoteServerStats);
                TEST_ASSERT_EQUAL(rc, OK);

                TEST_ASSERT_EQUAL(remoteServerStats.q0.BufferedMsgs, prevLowQoSBufferedCount[server]);
                prevLowQoSBufferedCount[server] = remoteServerStats.q0.BufferedMsgs;
                TEST_ASSERT_EQUAL(remoteServerStats.q1.BufferedMsgs, prevHighQoSBufferedCount[server]);
                prevHighQoSBufferedCount[server] = remoteServerStats.q1.BufferedMsgs;
            }
        }

        currentTopic++;
    }

    TEST_ASSERT_EQUAL(updateRetainedStats_callCount, starting_updateRetainedStats_callCount + expectedUpdateRetainedStatsCalls);

    TEST_ASSERT_EQUAL(fwdPublishCount, 60);
    iemnMessagingStatistics_t msgStats;

    iemn_getMessagingStatistics(pThreadData, &msgStats);
    TEST_ASSERT_EQUAL(msgStats.FromForwarderRetainedMessages, fwdPublishCount/2);
    TEST_ASSERT_EQUAL(msgStats.FromForwarderMessages, fwdPublishCount/2);
    if (wildConsumer)
    {
        TEST_ASSERT_EQUAL(msgStats.FromForwarderNoRecipientMessages, 18);
    }
    else
    {
        TEST_ASSERT_EQUAL(msgStats.FromForwarderNoRecipientMessages, 21);
    }

    ismCluster_EngineStatistics_t clusterEngineStats;

    // Prove the stats reported to the cluster callback match what we expect
    rc = engineCallback(ENGINE_RS_REPORT_STATS,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        (ismCluster_RemoteServerHandle_t)1,
                        NULL, NULL,
                        NULL, 0,
                        NULL, 0,
                        false, false, NULL, &clusterEngineStats,
                        engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(clusterEngineStats.numFwdIn, msgStats.FromForwarderMessages);
    TEST_ASSERT_EQUAL(clusterEngineStats.numFwdInNoConsumer, msgStats.FromForwarderNoRecipientMessages);
    TEST_ASSERT_EQUAL(clusterEngineStats.numFwdInRetained, msgStats.FromForwarderRetainedMessages);

    // Make sure remove doesn't make free the queues
    int32_t serverToRemove = NUM_SERVERS-1;

    iers_acquireServerReference(createdHandle[serverToRemove]);
    TEST_ASSERT_EQUAL(createdHandle[serverToRemove]->useCount, 2);

    rc = engineCallback(ENGINE_RS_REMOVE,
                        createdHandle[serverToRemove],
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Test a couple of publishes which should not go to the removed server
    for(int32_t i=0; i<2; i++)
    {
        check_ism_cluster_routeLookup_topic = "A/B/C/D";
        check_ism_cluster_routeLookup_numDests = 1;
        test_publishMessages("A/B/C/D",
                             NULL,
                             TEST_SMALL_MESSAGE_SIZE,
                             1,
                             i == 0 ? ismMESSAGE_RELIABILITY_AT_MOST_ONCE : ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                             ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                             i == 0 ? ismMESSAGE_FLAGS_NONE : ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                             PROTOCOL_ID_MQTT,
                             OK);
    }

    rc = ism_engine_getRemoteServerStatistics(createdHandle[serverToRemove], &remoteServerStats);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(remoteServerStats.q0.BufferedMsgs, prevLowQoSBufferedCount[serverToRemove]);
    TEST_ASSERT_EQUAL(remoteServerStats.q1.BufferedMsgs, prevHighQoSBufferedCount[serverToRemove]);

    TEST_ASSERT_NOT_EQUAL(serverToRemove, 0); // Just check the next tests are right

    rc = ism_engine_getRemoteServerStatistics(createdHandle[0], &remoteServerStats);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(remoteServerStats.q0.BufferedMsgs, prevLowQoSBufferedCount[0]); // Non-retained should not match
    prevLowQoSBufferedCount[0] = remoteServerStats.q0.BufferedMsgs;
    TEST_ASSERT_EQUAL(remoteServerStats.q1.BufferedMsgs, prevHighQoSBufferedCount[0]+1); // Retained should 'match'
    prevHighQoSBufferedCount[0] = remoteServerStats.q1.BufferedMsgs;

    TEST_ASSERT_NOT_EQUAL(serverToRemove, 1); // Just check the next tests are right

    rc = ism_engine_getRemoteServerStatistics(createdHandle[1], &remoteServerStats);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(remoteServerStats.q0.BufferedMsgs, prevLowQoSBufferedCount[1]+1);
    prevLowQoSBufferedCount[1] = remoteServerStats.q0.BufferedMsgs;
    TEST_ASSERT_EQUAL(remoteServerStats.q1.BufferedMsgs, prevHighQoSBufferedCount[1]+1);
    prevHighQoSBufferedCount[1] = remoteServerStats.q1.BufferedMsgs;

    // Let the removed server go
    iers_releaseServer(pThreadData, createdHandle[serverToRemove]);

    // Force a re-allocation of the dests buffer on a publish
    check_ism_cluster_routeLookup_forceRealloc = 1;
    check_ism_cluster_routeLookup_topic = "Z/Z/Z/Z";
    check_ism_cluster_routeLookup_numDests = 0;
    test_publishMessages("Z/Z/Z/Z",
                         NULL,
                         TEST_SMALL_MESSAGE_SIZE,
                         1,
                         ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                         ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                         ismMESSAGE_FLAGS_NONE,
                         PROTOCOL_ID_MQTT,
                         OK);
    for(int32_t j=0; j<NUM_SERVERS; j++)
    {
        if (j != serverToRemove)
        {
            rc = ism_engine_getRemoteServerStatistics(createdHandle[j], &remoteServerStats);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_EQUAL(remoteServerStats.q0.BufferedMsgs, prevLowQoSBufferedCount[j]);
        }
    }

    for(int32_t j=0; j<NUM_SERVERS; j++)
    {
        if (j != serverToRemove)
        {
            rc = engineCallback(ENGINE_RS_REMOVE,
                                createdHandle[j],
                                0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 1);

    rc = engineCallback(ENGINE_RS_REMOVE,
                        localServerHandle,
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);

    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);

    check_ism_cluster_routeLookup = false;
    routeLookup_callCount = 0;

    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_REGEX_TOPIC,
                                                      ".*",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
}
#undef NUM_SERVERS

//****************************************************************************
/// @brief Test the consuming of messages from a remote servers
//****************************************************************************
typedef struct tag_remoteServerMessagesCbContext_t
{
    ismEngine_SessionHandle_t hSession;
    int32_t received;
    int32_t propagateSet;
    volatile uint32_t publishedForRetainSet;
    bool suspendDelivery;
} remoteServerMessagesCbContext_t;

bool remoteServerDeliveryCallback(ismEngine_ConsumerHandle_t      hConsumer,
                                  ismEngine_DeliveryHandle_t      hDelivery,
                                  ismEngine_MessageHandle_t       hMessage,
                                  uint32_t                        deliveryId,
                                  ismMessageState_t               state,
                                  uint32_t                        destinationOptions,
                                  ismMessageHeader_t *            pMsgDetails,
                                  uint8_t                         areaCount,
                                  ismMessageAreaType_t            areaTypes[areaCount],
                                  size_t                          areaLengths[areaCount],
                                  void *                          pAreaData[areaCount],
                                  void *                          pContext)
{
    remoteServerMessagesCbContext_t *context = *((remoteServerMessagesCbContext_t **)pContext);

    __sync_fetch_and_add(&context->received, 1);

    if ((pMsgDetails->Flags & ismMESSAGE_FLAGS_PROPAGATE_RETAINED) == ismMESSAGE_FLAGS_PROPAGATE_RETAINED)
    {
        __sync_fetch_and_add(&context->propagateSet, 1);
    }

    if ((pMsgDetails->Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN) == ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN)
    {
        __sync_fetch_and_add(&context->publishedForRetainSet, 1);
    }

    ism_engine_releaseMessage(hMessage);

    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        uint32_t rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                        NULL,
                                                        hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                        NULL,
                                                        0,
                                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    bool rc = true;

    if (context->suspendDelivery == true)
    {
        context->suspendDelivery = false;
        ism_engine_suspendMessageDelivery(hConsumer, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
        rc = false;
    }

    return rc;
}

void test_capability_Consuming(void)
{
    int32_t rc;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);

    ismEngine_RemoteServerHandle_t remoteServerHandle = ismENGINE_NULL_REMOTESERVER_HANDLE;

    rc = engineCallback(ENGINE_RS_CREATE,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        (ismCluster_RemoteServerHandle_t)NULL,
                        "ConsumingServer",
                        "CSUID",
                        NULL, 0,
                        NULL, 0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        &remoteServerHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(remoteServerHandle);
    TEST_ASSERT_EQUAL(remoteServerHandle->useCount, 1);
    TEST_ASSERT_EQUAL(remoteServerHandle->consumerCount, 0);

    // Check ackList handling is as expected
    TEST_ASSERT_EQUAL(((iemqQueue_t *)remoteServerHandle->lowQoSQueueHandle)->ackListsUpdating, true);
    TEST_ASSERT_EQUAL(((iemqQueue_t *)remoteServerHandle->highQoSQueueHandle)->ackListsUpdating, true);

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;

    rc = test_createClientAndSessionWithProtocol("__Server_ConsumingServer",
                                                 PROTOCOL_ID_FWD,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                                 ismENGINE_CREATE_SESSION_TRANSACTIONAL |
                                                 ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                                 &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    remoteServerMessagesCbContext_t lowMsgContext = {0};
    remoteServerMessagesCbContext_t *lowMsgCb = &lowMsgContext;
    ismEngine_ConsumerHandle_t hLowConsumer;

    lowMsgContext.hSession = hSession;

    rc = ism_engine_createRemoteServerConsumer(hSession,
                                               remoteServerHandle,
                                               &lowMsgCb,
                                               sizeof(lowMsgCb),
                                               remoteServerDeliveryCallback,
                                               ismENGINE_CONSUMER_OPTION_LOW_QOS,
                                               &hLowConsumer,
                                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(remoteServerHandle->useCount, 2);
    TEST_ASSERT_EQUAL(remoteServerHandle->consumerCount, 1);
    TEST_ASSERT_PTR_NOT_NULL(hLowConsumer);

    test_publishMessages("TEST/NO-ONE/THERE",
                         NULL,
                         TEST_SMALL_MESSAGE_SIZE,
                         1,
                         ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                         ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                         ismMESSAGE_FLAGS_NONE,
                         PROTOCOL_ID_MQTT,
                         OK);
    TEST_ASSERT_EQUAL(lowMsgContext.received, 0);

    ismEngine_QueueStatistics_t stats;

    ieq_getStats(pThreadData, remoteServerHandle->lowQoSQueueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
    ieq_getStats(pThreadData, remoteServerHandle->highQoSQueueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);

    char *testTopics[] = {"#"};
    size_t testTopicsCount = sizeof(testTopics)/sizeof(testTopics[0]);

    rc = engineCallback(ENGINE_RS_ADD_SUB,
                        remoteServerHandle,
                        0,
                        NULL, NULL,
                        NULL, 0,
                        testTopics, testTopicsCount,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    for(uint8_t rel=0; rel<3; rel++)
    {
        test_publishMessages("TEST/SIMPLE",
                             NULL,
                             TEST_SMALL_MESSAGE_SIZE,
                             1,
                             rel,
                             ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                             ismMESSAGE_FLAGS_NONE,
                             PROTOCOL_ID_MQTT,
                             OK);
    }

    TEST_ASSERT_EQUAL(lowMsgContext.received, 1);

    ieq_getStats(pThreadData, remoteServerHandle->lowQoSQueueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
    ieq_getStats(pThreadData, remoteServerHandle->highQoSQueueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 2);

    remoteServerMessagesCbContext_t highMsgContext = {0};
    remoteServerMessagesCbContext_t *highMsgCb = &highMsgContext;
    ismEngine_ConsumerHandle_t hHighConsumer;

    highMsgContext.hSession = hSession;

    rc = ism_engine_createRemoteServerConsumer(hSession,
                                               remoteServerHandle,
                                               &highMsgCb,
                                               sizeof(highMsgCb),
                                               remoteServerDeliveryCallback,
                                               ismENGINE_CONSUMER_OPTION_HIGH_QOS,
                                               &hHighConsumer,
                                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(remoteServerHandle->useCount, 3);
    TEST_ASSERT_EQUAL(remoteServerHandle->consumerCount, 2);
    TEST_ASSERT_PTR_NOT_NULL(hHighConsumer);

    int32_t expectLow = 1;
    int32_t expectHigh = 2;

    TEST_ASSERT_EQUAL(lowMsgContext.received, expectLow);
    TEST_ASSERT_EQUAL(highMsgContext.received, expectHigh);

    // Throw 10,000 messages at it, various reliabilities and protocols
    for(int32_t i=0; i<10000; i++)
    {
        uint8_t rel = (uint8_t)(rand()%3);
        uint8_t prot = (rand()%2 == 1) ? PROTOCOL_ID_MQTT : PROTOCOL_ID_FWD;

        test_publishMessages("TEST/10000",
                             NULL,
                             TEST_SMALL_MESSAGE_SIZE,
                             1,
                             rel,
                             ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                             ismMESSAGE_FLAGS_NONE,
                             prot,
                             OK);

        if (prot != PROTOCOL_ID_FWD)
        {
            if (rel == 0) expectLow++;
            else expectHigh++;
        }
    }

    TEST_ASSERT_EQUAL(lowMsgContext.received, expectLow);
    TEST_ASSERT_EQUAL(highMsgContext.received, expectHigh);

    // Try creating (invalid) 2nd consumers
    ismEngine_ConsumerHandle_t hSecondConsumer = NULL;
    for(int32_t i=0; i<2; i++)
    {
        rc = ism_engine_createRemoteServerConsumer(hSession,
                                                   remoteServerHandle,
                                                   NULL, 0, NULL,
                                                   i == 0 ? ismENGINE_CONSUMER_OPTION_LOW_QOS :
                                                            ismENGINE_CONSUMER_OPTION_HIGH_QOS,
                                                   &hSecondConsumer,
                                                   NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_WaiterInUse);
        TEST_ASSERT_PTR_NULL(hSecondConsumer);
        TEST_ASSERT_EQUAL(remoteServerHandle->useCount, 3);
        TEST_ASSERT_EQUAL(remoteServerHandle->consumerCount, 2);
    }

    // Test consumer being suspended
    lowMsgContext.suspendDelivery = true;

    test_publishMessages("TEST/SUSPEND",
                         NULL,
                         TEST_SMALL_MESSAGE_SIZE,
                         10,
                         ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                         ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                         ismMESSAGE_FLAGS_NONE,
                         PROTOCOL_ID_MQTT,
                         OK);
    test_publishMessages("TEST/SUSPEND",
                         NULL,
                         TEST_SMALL_MESSAGE_SIZE,
                         10,
                         ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                         ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                         ismMESSAGE_FLAGS_NONE,
                         PROTOCOL_ID_MQTT,
                         OK);

    expectLow += 1;  // 1st message received will suspend
    expectHigh += 10;
    TEST_ASSERT_EQUAL(lowMsgContext.received, expectLow);
    TEST_ASSERT_EQUAL(highMsgContext.received, expectHigh);
    ieq_getStats(pThreadData, remoteServerHandle->lowQoSQueueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 9);

    rc = ism_engine_resumeMessageDelivery(hLowConsumer,
                                          ismENGINE_RESUME_DELIVERY_OPTION_NONE,
                                          NULL, 0, NULL);

    expectLow += 9;
    TEST_ASSERT_EQUAL(lowMsgContext.received, expectLow);
    TEST_ASSERT_EQUAL(highMsgContext.received, expectHigh);
    ieq_getStats(pThreadData, remoteServerHandle->lowQoSQueueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
    ieq_getStats(pThreadData, remoteServerHandle->highQoSQueueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);

    // Create a subscription and prove that that too will get messages
    remoteServerMessagesCbContext_t subMsgContext = {0};
    remoteServerMessagesCbContext_t *subMsgCb = &subMsgContext;
    ismEngine_ConsumerHandle_t hSubConsumer;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "TEST/WITH/SUBSCRIPTION",
                                   &subAttrs,
                                   NULL,
                                   &subMsgCb,
                                   sizeof(subMsgCb),
                                   remoteServerDeliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hSubConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSubConsumer);
    TEST_ASSERT_EQUAL(remoteServerHandle->useCount, 3);
    TEST_ASSERT_EQUAL(remoteServerHandle->consumerCount, 2);

    // Throw 5,000 messages at it, various reliabilities and protocols
    for(int32_t i=0; i<10000; i++)
    {
        uint8_t rel = (uint8_t)(rand()%3);
        uint8_t prot = (rand()%2 == 1) ? PROTOCOL_ID_MQTT : PROTOCOL_ID_FWD;

        test_publishMessages("TEST/WITH/SUBSCRIPTION",
                             NULL,
                             TEST_SMALL_MESSAGE_SIZE,
                             1,
                             rel,
                             ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                             ismMESSAGE_FLAGS_NONE,
                             prot,
                             OK);

        if (prot != PROTOCOL_ID_FWD)
        {
            if (rel == 0) expectLow++;
            else expectHigh++;
        }
    }

    TEST_ASSERT_EQUAL(lowMsgContext.received, expectLow);
    TEST_ASSERT_EQUAL(highMsgContext.received, expectHigh);
    TEST_ASSERT_EQUAL(subMsgContext.received, 10000);

    // TODO: MORE TESTING IN HERE

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(remoteServerHandle->useCount, 1);
    TEST_ASSERT_EQUAL(remoteServerHandle->consumerCount, 0);

    ieq_getStats(pThreadData, remoteServerHandle->lowQoSQueueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
    ieq_getStats(pThreadData, remoteServerHandle->highQoSQueueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);

    rc = engineCallback(ENGINE_RS_REMOVE,
                        remoteServerHandle,
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test the function to terminate the cluster
//****************************************************************************
#define NUM_SERVERS 3
void test_capability_Terminating(void)
{
    int32_t rc;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);

    // Call terminate when there are no servers - do it twice to test path fixed in 96339
    for(int32_t i=0; i<2; i++)
    {
        rc = engineCallback(ENGINE_RS_TERM,
                            NULL, 0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Re-enable the cluster!
    ismEngine_serverGlobal.clusterEnabled = true;

    ismEngine_RemoteServerHandle_t createdHandle[NUM_SERVERS] = {ismENGINE_NULL_REMOTESERVER_HANDLE};

    // Use cluster callback interface to create 'remote' servers
    for(uint64_t i=1; i<=NUM_SERVERS; i++)
    {
        char serverName[20];
        char serverUID[10];

        sprintf(serverName, "ServerNo%02lu", i);
        sprintf(serverUID, "UID%lu", i);

        rc = engineCallback(ENGINE_RS_CREATE,
                            ismENGINE_NULL_REMOTESERVER_HANDLE,
                            (ismCluster_RemoteServerHandle_t)NULL,
                            serverName,
                            serverUID,
                            NULL, 0,
                            NULL, 0,
                            false, false, NULL, NULL,
                            engineCallbackContext,
                            &createdHandle[i-1]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(createdHandle[i-1]);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, i);
        TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, i);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle[i-1]);
    }

    char *testTopics0[] = {"#"};
    size_t testTopicsCount0 = sizeof(testTopics0)/sizeof(testTopics0[0]);

    rc = engineCallback(ENGINE_RS_ADD_SUB,
                        createdHandle[0],
                        0,
                        NULL, NULL,
                        NULL, 0,
                        testTopics0, testTopicsCount0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    uint64_t prevLowQoSBufferedCount[NUM_SERVERS] = {0};
    uint64_t prevHighQoSBufferedCount[NUM_SERVERS] = {0};
    ismEngine_QueueStatistics_t stats;

    check_ism_cluster_routeLookup = true;
    check_ism_cluster_routeLookup_numDests = -1;
    check_ism_cluster_routeLookup_topic = NULL;

    routeLookup_callCount = 0;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    rc = test_createClientAndSession("TEST_CLIENT",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);


    char *topic[] = {"A/B/C/D", "C/D/E/F", NULL};
    uint64_t expectBufferedCount[][NUM_SERVERS] = {{1,0,0},
                                                   {1,0,0},
                                                   {0,0,0}};
    uint32_t expectRouteLookupCallCount[][4] = {{1},
                                                {1},
                                                {0}};

    check_ism_cluster_addSubscriptions = true;
    check_ism_cluster_removeSubscriptions = true;

    for(int32_t loop=0; loop<3; loop++)
    {
        addSubscriptions_callCount = addSubscriptions_topicCount = addSubscriptions_wildcardCount = 0;
        removeSubscriptions_callCount = removeSubscriptions_topicCount = removeSubscriptions_wildcardCount = 0;

        int32_t currentTopic = 0;
        while(topic[currentTopic] != NULL)
        {
            // Publish a low QoS message
            routeLookup_callCount = 0;
            check_ism_cluster_routeLookup_topic = topic[currentTopic];
            test_publishMessages(topic[currentTopic],
                                 NULL,
                                 TEST_SMALL_MESSAGE_SIZE,
                                 1,
                                 ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                 ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                 ismMESSAGE_FLAGS_NONE,
                                 PROTOCOL_ID_MQTT,
                                 OK);

            if (loop == 0)
            {
                TEST_ASSERT_EQUAL(routeLookup_callCount, expectRouteLookupCallCount[currentTopic][0]);

                for(int32_t server=0; server<NUM_SERVERS; server++)
                {
                    TEST_ASSERT_EQUAL(createdHandle[server]->useCount, 1);

                    ieq_getStats(pThreadData, createdHandle[server]->lowQoSQueueHandle, &stats);
                    TEST_ASSERT_EQUAL(stats.BufferedMsgs, prevLowQoSBufferedCount[server]+expectBufferedCount[currentTopic][server]);
                    prevLowQoSBufferedCount[server] = stats.BufferedMsgs;

                    ieq_getStats(pThreadData, createdHandle[server]->highQoSQueueHandle, &stats);
                    TEST_ASSERT_EQUAL(stats.BufferedMsgs, prevHighQoSBufferedCount[server]);
                    prevHighQoSBufferedCount[server] = stats.BufferedMsgs;
                }
            }
            else
            {
                TEST_ASSERT_EQUAL(routeLookup_callCount, 0);
            }

            // Check that the ism_cluster_addSubscription function is/isn't called as expected
            ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
            rc = ism_engine_createConsumer(hSession,
                                           ismDESTINATION_TOPIC,
                                           topic[currentTopic],
                                           &subAttrs,
                                           NULL,
                                           NULL,
                                           0,
                                           NULL, /* No delivery callback */
                                           NULL,
                                           ismENGINE_CONSUMER_OPTION_NONE,
                                           &hConsumer,
                                           NULL,
                                           0,
                                           NULL);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hConsumer);

            rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            currentTopic++;
        }

        if (loop == 1)
        {
            TEST_ASSERT_EQUAL(addSubscriptions_callCount, 0);
            TEST_ASSERT_EQUAL(removeSubscriptions_callCount, 0);
        }
        else
        {
            TEST_ASSERT_EQUAL(addSubscriptions_callCount, currentTopic);
            TEST_ASSERT_EQUAL(removeSubscriptions_callCount, currentTopic);
        }

        rc = engineCallback(ENGINE_RS_TERM,
                            NULL, 0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(ismEngine_serverGlobal.clusterEnabled, false);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
        TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
        TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);

        // Re-enable the cluster in all but 1st loop
        if (loop != 0) ismEngine_serverGlobal.clusterEnabled = true;
    }

    test_destroyClientAndSession(hClient, hSession, true);

    check_ism_cluster_addSubscriptions = false;
    check_ism_cluster_removeSubscriptions = false;
    check_ism_cluster_routeLookup = false;
    routeLookup_callCount = 0;
}
#undef NUM_SERVERS

//****************************************************************************
/// @brief Test the capability to limit remote server queue size by bytes
//****************************************************************************
void test_capability_Limiting(void)
{
    int32_t rc;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    printf("Starting %s...\n", __func__);

    ismEngine_QueueStatistics_t queueStats;
    iemnMessagingStatistics_t msgStats;

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);

    iemn_getMessagingStatistics(pThreadData, &msgStats);

    uint64_t startRSBMB, nowRSBMB;

    startRSBMB = nowRSBMB = msgStats.RemoteServerBufferedMessageBytes;
    TEST_ASSERT_EQUAL(startRSBMB, 0);

    ismEngine_RemoteServerHandle_t createdHandle = ismENGINE_NULL_REMOTESERVER_HANDLE;

    rc = engineCallback(ENGINE_RS_CREATE,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        (ismCluster_RemoteServerHandle_t)NULL,
                        "Server0001",
                        "UID0001",
                        NULL, 0,
                        NULL, 0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        &createdHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(createdHandle);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 1);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle);

    char *testTopics0[] = {"#"};
    size_t testTopicsCount0 = sizeof(testTopics0)/sizeof(testTopics0[0]);

    rc = engineCallback(ENGINE_RS_ADD_SUB,
                        createdHandle,
                        0,
                        NULL, NULL,
                        NULL, 0,
                        testTopics0, testTopicsCount0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    size_t payloadSize = TEST_SMALL_MESSAGE_SIZE;

    uint64_t msgBytesPerMsg = payloadSize + 7;

    // Publish a low QoS message
    test_publishMessages("A/B",
                         NULL,
                         payloadSize,
                         1,
                         ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                         ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                         ismMESSAGE_FLAGS_NONE,
                         PROTOCOL_ID_MQTT,
                         OK);
    nowRSBMB += msgBytesPerMsg;

    // Check our stats are as expected
    iemn_getMessagingStatistics(pThreadData, &msgStats);
    TEST_ASSERT_EQUAL(msgStats.RemoteServerBufferedMessageBytes, nowRSBMB);
    nowRSBMB = msgStats.RemoteServerBufferedMessageBytes;
    ieq_getStats(pThreadData, createdHandle->lowQoSQueueHandle, &queueStats);
    TEST_ASSERT_EQUAL(queueStats.BufferedMsgBytes, msgBytesPerMsg);
    ieq_getStats(pThreadData, createdHandle->highQoSQueueHandle, &queueStats);
    TEST_ASSERT_EQUAL(queueStats.BufferedMsgBytes, 0);

    // Publish a high QoS message
    test_publishMessages("A/B",
                         NULL,
                         payloadSize,
                         1,
                         ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                         ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                         ismMESSAGE_FLAGS_NONE,
                         PROTOCOL_ID_MQTT,
                         OK);
    nowRSBMB += msgBytesPerMsg;

    // Check our stats are as expected
    iemn_getMessagingStatistics(pThreadData, &msgStats);
    TEST_ASSERT_EQUAL(msgStats.RemoteServerBufferedMessageBytes, nowRSBMB);
    nowRSBMB = msgStats.RemoteServerBufferedMessageBytes;
    ieq_getStats(pThreadData, createdHandle->lowQoSQueueHandle, &queueStats);
    TEST_ASSERT_EQUAL(queueStats.BufferedMsgBytes, msgBytesPerMsg);
    ieq_getStats(pThreadData, createdHandle->highQoSQueueHandle, &queueStats);
    TEST_ASSERT_EQUAL(queueStats.BufferedMsgBytes, msgBytesPerMsg);

    iepiPolicyInfo_t *lowQoSPolicy = createdHandle->lowQoSQueueHandle->PolicyInfo;
    iepiPolicyInfo_t *highQoSPolicy = createdHandle->highQoSQueueHandle->PolicyInfo;

    lowQoSPolicy->maxMessageBytes = msgBytesPerMsg;

    uint64_t expectBufferedMsgs = 1;

    // Publish a low QoS message
    test_publishMessages("A/B",
                         NULL,
                         payloadSize,
                         1,
                         ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                         ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                         ismMESSAGE_FLAGS_NONE,
                         PROTOCOL_ID_MQTT,
                         OK);

    iemn_getMessagingStatistics(pThreadData, &msgStats);
    TEST_ASSERT_EQUAL(msgStats.RemoteServerBufferedMessageBytes, nowRSBMB); // Replaced the message there
    nowRSBMB = msgStats.RemoteServerBufferedMessageBytes;
    ieq_getStats(pThreadData, createdHandle->lowQoSQueueHandle, &queueStats);
    TEST_ASSERT_EQUAL(queueStats.BufferedMsgBytes, msgBytesPerMsg);
    TEST_ASSERT_EQUAL(queueStats.BufferedMsgs, expectBufferedMsgs);

    highQoSPolicy->maxMessageBytes = msgBytesPerMsg;

    // Publish a high QoS message
    expectBufferedMsgs = 1;
    for(int32_t loop=0; loop<4; loop++)
    {
        if (loop >= 2)
        {
            highQoSPolicy->maxMessageBytes = 0;
            expectBufferedMsgs++;
            nowRSBMB += msgBytesPerMsg;
        }

        test_publishMessages("A/B",
                             NULL,
                             payloadSize,
                             1,
                             ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                             ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                             ismMESSAGE_FLAGS_NONE,
                             PROTOCOL_ID_MQTT,
                             OK);

        ieq_getStats(pThreadData, createdHandle->highQoSQueueHandle, &queueStats);
        TEST_ASSERT_EQUAL(queueStats.BufferedMsgs, expectBufferedMsgs);
    }

    lowQoSPolicy->maxMessageBytes = highQoSPolicy->maxMessageBytes = 0;

    // Publish a low QoS message
    test_publishMessages("A/B",
                         NULL,
                         payloadSize,
                         1,
                         ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                         ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                         ismMESSAGE_FLAGS_NONE,
                         PROTOCOL_ID_MQTT,
                         OK);
    nowRSBMB += msgBytesPerMsg;

    iemn_getMessagingStatistics(pThreadData, &msgStats);
    TEST_ASSERT_EQUAL(msgStats.RemoteServerBufferedMessageBytes, nowRSBMB);
    nowRSBMB = msgStats.RemoteServerBufferedMessageBytes;
    ieq_getStats(pThreadData, createdHandle->lowQoSQueueHandle, &queueStats);
    TEST_ASSERT_EQUAL(queueStats.BufferedMsgBytes, (msgBytesPerMsg*2));
    TEST_ASSERT_EQUAL(queueStats.BufferedMsgs, 2);

    // TODO: More testing with real policy limiting etc.

    rc = engineCallback(ENGINE_RS_REMOVE,
                        createdHandle,
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);

    iemn_getMessagingStatistics(pThreadData, &msgStats);
    TEST_ASSERT_EQUAL(startRSBMB, msgStats.RemoteServerBufferedMessageBytes);
}

//****************************************************************************
/// @brief Test the capability to set limits based on memory
//****************************************************************************
void test_capability_LimitSetting(void)
{
    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    TEST_ASSERT_NOT_EQUAL(remoteServersGlobal->reservedForwardingBytes, 0);

    ismEngine_RemoteServerHandle_t createdHandle = ismENGINE_NULL_REMOTESERVER_HANDLE;

    // Disable real calls to the memory usage callback for the duration of this test
    iemem_setMemoryReduceCallback(iemem_remoteServers, NULL);

    int32_t rc = engineCallback(ENGINE_RS_CREATE,
                                ismENGINE_NULL_REMOTESERVER_HANDLE,
                                (ismCluster_RemoteServerHandle_t)NULL,
                                "Server0001",
                                "UID0001",
                                NULL, 0,
                                NULL, 0,
                                false, false, NULL, NULL,
                                engineCallbackContext,
                                &createdHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(createdHandle);

    iemem_systemMemInfo_t realSysMemInfo = {0};

    TEST_ASSERT_PTR_NULL(fakeMemInfo);
    iemem_querySystemMemory(&realSysMemInfo);

    iemem_systemMemInfo_t localSysMemInfo;

    memcpy(&localSysMemInfo, &realSysMemInfo, sizeof(localSysMemInfo));

    fakeMemInfo = &localSysMemInfo;

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    //fake 25% memory (should not affect anything)
    localSysMemInfo.freeIncPercentage = 25;
    localSysMemInfo.currentFreeBytes = (realSysMemInfo.effectiveMemoryBytes * localSysMemInfo.freeIncPercentage-1)/100;
    localSysMemInfo.freeIncBuffersCacheBytes = (realSysMemInfo.effectiveMemoryBytes * localSysMemInfo.freeIncPercentage)/100;
    test_analyseMemoryUsage(iememReduceMemory, iememPlentifulMemory, iemem_remoteServers, 524288, &localSysMemInfo);

    // Not low enough to trigger changes to remote server policies
    TEST_ASSERT_EQUAL(remoteServersGlobal->highQoSPolicyHandle->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->lowQoSPolicyHandle->maxMessageBytes, 0);

    iersMemLimit_t limit = iers_queryRemoteServerMemLimit(pThreadData);
    TEST_ASSERT_EQUAL(limit, NoLimit);
    TEST_ASSERT_EQUAL(remoteServersGlobal->currentHealthStatus, ISM_CLUSTER_HEALTH_YELLOW); // 75% == 57-77%

    // Make it low enough to trigger the lowQoS policy change
    localSysMemInfo.freeIncPercentage = iersFREEMEM_LIMIT_LOW_QOS_FWD_THRESHOLD;
    localSysMemInfo.currentFreeBytes = (realSysMemInfo.effectiveMemoryBytes * localSysMemInfo.freeIncPercentage-1)/100;
    localSysMemInfo.freeIncBuffersCacheBytes = (realSysMemInfo.effectiveMemoryBytes * localSysMemInfo.freeIncPercentage)/100;
    test_analyseMemoryUsage(iememReduceMemory, iememReduceMemory, iemem_remoteServers, 524288, &localSysMemInfo);

    TEST_ASSERT_EQUAL(remoteServersGlobal->highQoSPolicyHandle->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->highQoSPolicyHandle->maxMessageCount, 0);
    TEST_ASSERT_NOT_EQUAL(remoteServersGlobal->lowQoSPolicyHandle->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->lowQoSPolicyHandle->maxMessageCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->currentHealthStatus, ISM_CLUSTER_HEALTH_RED); // 78% > 77%

    limit = iers_queryRemoteServerMemLimit(pThreadData);
    TEST_ASSERT_EQUAL(limit, LowQoSLimit);

    // Make it low enough to trigger the highQoS policy change
    localSysMemInfo.freeIncPercentage = iersFREEMEM_LIMIT_HIGH_QOS_FWD_THRESHOLD;
    localSysMemInfo.currentFreeBytes = (realSysMemInfo.effectiveMemoryBytes * localSysMemInfo.freeIncPercentage-1)/100;
    localSysMemInfo.freeIncBuffersCacheBytes = (realSysMemInfo.effectiveMemoryBytes * localSysMemInfo.freeIncPercentage)/100;
    test_analyseMemoryUsage(iememReduceMemory, iememReduceMemory, iemem_remoteServers, 524288, &localSysMemInfo);

    TEST_ASSERT_NOT_EQUAL(remoteServersGlobal->highQoSPolicyHandle->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->highQoSPolicyHandle->maxMessageCount, 0);
    TEST_ASSERT_NOT_EQUAL(remoteServersGlobal->lowQoSPolicyHandle->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->lowQoSPolicyHandle->maxMessageCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->highQoSPolicyHandle->maxMessageBytes, remoteServersGlobal->lowQoSPolicyHandle->maxMessageBytes);
    TEST_ASSERT_EQUAL(remoteServersGlobal->currentHealthStatus, ISM_CLUSTER_HEALTH_RED); // 82% > 77%

    limit = iers_queryRemoteServerMemLimit(pThreadData);
    TEST_ASSERT_EQUAL(limit, HighQoSLimit);

    // Make it low enough to trigger discarding local NullRetained
    localSysMemInfo.freeIncPercentage = iersFREEMEM_LIMIT_DISCARD_LOCAL_NULLRETAINED;
    localSysMemInfo.currentFreeBytes = (realSysMemInfo.effectiveMemoryBytes * localSysMemInfo.freeIncPercentage-1)/100;
    localSysMemInfo.freeIncBuffersCacheBytes = (realSysMemInfo.effectiveMemoryBytes * localSysMemInfo.freeIncPercentage)/100;
    test_analyseMemoryUsage(iememReduceMemory, iememReduceMemory, iemem_remoteServers, 524288, &localSysMemInfo);

    TEST_ASSERT_NOT_EQUAL(remoteServersGlobal->highQoSPolicyHandle->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->highQoSPolicyHandle->maxMessageCount, 0);
    TEST_ASSERT_NOT_EQUAL(remoteServersGlobal->lowQoSPolicyHandle->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->lowQoSPolicyHandle->maxMessageCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->highQoSPolicyHandle->maxMessageBytes, remoteServersGlobal->lowQoSPolicyHandle->maxMessageBytes);
    TEST_ASSERT_EQUAL(remoteServersGlobal->currentHealthStatus, ISM_CLUSTER_HEALTH_RED); // 85% > 77%

    limit = iers_queryRemoteServerMemLimit(pThreadData);
    TEST_ASSERT_EQUAL(limit, DiscardLocalNullRetained);

    // Back into the lowQoS range
    TEST_ASSERT_GREATER_THAN(iersFREEMEM_LIMIT_LOW_QOS_FWD_THRESHOLD-1, iersFREEMEM_LIMIT_HIGH_QOS_FWD_THRESHOLD);

    localSysMemInfo.freeIncPercentage = iersFREEMEM_LIMIT_LOW_QOS_FWD_THRESHOLD-1;
    localSysMemInfo.currentFreeBytes = (realSysMemInfo.effectiveMemoryBytes * localSysMemInfo.freeIncPercentage-1)/100;
    localSysMemInfo.freeIncBuffersCacheBytes = (realSysMemInfo.effectiveMemoryBytes * localSysMemInfo.freeIncPercentage)/100;
    test_analyseMemoryUsage(iememReduceMemory, iememReduceMemory, iemem_remoteServers, 524288, &localSysMemInfo);

    TEST_ASSERT_EQUAL(remoteServersGlobal->highQoSPolicyHandle->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->highQoSPolicyHandle->maxMessageCount, 0);
    TEST_ASSERT_NOT_EQUAL(remoteServersGlobal->lowQoSPolicyHandle->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->lowQoSPolicyHandle->maxMessageCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->currentHealthStatus, ISM_CLUSTER_HEALTH_RED); // 79% > 77%

    // Straight back to lots of memory
    localSysMemInfo.freeIncPercentage = 90;
    localSysMemInfo.currentFreeBytes = (realSysMemInfo.effectiveMemoryBytes * localSysMemInfo.freeIncPercentage-1)/100;
    localSysMemInfo.freeIncBuffersCacheBytes = (realSysMemInfo.effectiveMemoryBytes * localSysMemInfo.freeIncPercentage)/100;
    test_analyseMemoryUsage(iememPlentifulMemory, iememReduceMemory, iemem_remoteServers, 524288, &localSysMemInfo);

    TEST_ASSERT_EQUAL(remoteServersGlobal->highQoSPolicyHandle->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->lowQoSPolicyHandle->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->currentHealthStatus, ISM_CLUSTER_HEALTH_GREEN); // 10% <40%

    // Back into the 'yellow' status
    localSysMemInfo.freeIncPercentage = 24;
    localSysMemInfo.currentFreeBytes = (realSysMemInfo.effectiveMemoryBytes * localSysMemInfo.freeIncPercentage-1)/100;
    localSysMemInfo.freeIncBuffersCacheBytes = (realSysMemInfo.effectiveMemoryBytes * localSysMemInfo.freeIncPercentage)/100;
    test_analyseMemoryUsage(iememReduceMemory, iememPlentifulMemory, iemem_remoteServers, 524288, &localSysMemInfo);

    limit = iers_queryRemoteServerMemLimit(pThreadData);
    TEST_ASSERT_EQUAL(limit, NoLimit);
    TEST_ASSERT_EQUAL(remoteServersGlobal->highQoSPolicyHandle->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->lowQoSPolicyHandle->maxMessageBytes, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->currentHealthStatus, ISM_CLUSTER_HEALTH_YELLOW); // 76% == 57-77%

    // Get rid of the fake memory
    fakeMemInfo = NULL;

    rc = engineCallback(ENGINE_RS_REMOVE,
                        createdHandle,
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Put real memory usage callbacks back
    iemem_setMemoryReduceCallback(iemem_remoteServers, iers_analyseMemoryUsage);

    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);
}

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_BasicThings[] =
{
    { "BasicInitialization", test_capability_BasicInitialization },
    { "BasicCreateRemove", test_capability_BasicCreateRemove },
    { "BasicRouteAll", test_capability_BasicRouteAll },
    { "BatchingUpdates", test_capability_BatchingUpdates },
    { "SubscriptionReporting", test_capability_SubscriptionReporting },
    { "ClusterRequestedTopicReporting", test_capability_ClusterRequestedTopicReporting },
    { "AddRemoveTopics", test_capability_AddRemoveTopics },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_BasicThings2[] =
{
    { "Publishing (Source)", test_capability_Publishing_Source },
    // TODO: { "Publishing (Destination)", test_capability_Publishing_Destination },
    { "Consuming", test_capability_Consuming },
    { "Limiting", test_capability_Limiting },
    { "LimitSetting", test_capability_LimitSetting },
    { "Terminating", test_capability_Terminating },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the seeding of retained messages at creation of remote server
//****************************************************************************
void test_capability_Seeding_Phase1(void)
{
    int32_t rc;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    printf("Starting %s...\n", __func__);

    // Publish various types of retained messages
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    uint32_t totalRetained    = 50;
    uint32_t totalOtherServer = 30;
    uint32_t totalOldServer   = 10;

    rc = test_createClientAndSession("SeedingRetainedClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    int32_t lowQoSCount = 0;
    int32_t lowQoSUnsetCount = 0;
    char topicString[50];
    int32_t i;
    ismEngine_MessageHandle_t hMsg[totalRetained];
    ismEngine_MessageHandle_t hOtherMsg[totalOtherServer];
    ismEngine_MessageHandle_t hOldMsg[totalOldServer];
    void *payload = NULL;

    // Add a retained message on a system topic that should not get sent by a
    // seeding request
    ismEngine_MessageHandle_t hSysMsg;
    rc = test_createMessage(10, ismMESSAGE_PERSISTENCE_NONPERSISTENT, 0,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN, 0,
                            ismDESTINATION_TOPIC, "$SYS/TEST",
                            &hSysMsg, &payload);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            "$SYS/TEST",
                                            NULL,
                                            hSysMsg,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
    TEST_ASSERT_EQUAL(hSysMsg->usageCount, 1);

    free(payload);

    uint32_t putCount = totalRetained;
    uint32_t *pPutCount = &putCount;

    for(i=0; i<totalRetained; i++) /* BEAM suppression: loop doesn't iterate */
    {
        sprintf(topicString, "Seeding/%08d/Test", i);

        payload=NULL;

        uint8_t persistence, reliability;

        if ((rand()%totalRetained) > (totalRetained/2))
        {
            persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
        }
        else
        {
            persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        }

        if ((reliability =-(uint8_t)(i%3)) == 0)
        {
            if (i<10) lowQoSUnsetCount++;
            lowQoSCount++;
        }

        rc = test_createMessage(10,
                                persistence,
                                reliability,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, topicString,
                                &hMsg[i], &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        // Publish message
        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                NULL,
                                                hMsg[i],
                                                &pPutCount, sizeof(pPutCount), test_countAsyncPut);
        if (rc == ISMRC_NoMatchingDestinations)
        {
            __sync_fetch_and_sub(pPutCount, 1);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        }

        free(payload);
    }

    while(putCount > 0)
    {
        sched_yield();
    }

    // Check the messages
    for(i=0; i<totalRetained; i++)
    {
        TEST_ASSERT_EQUAL(hMsg[i]->usageCount, 1);
        if (hMsg[i]->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
        {
            TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, 1);
        }
        else
        {
            TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, 0);
        }
    }

    // Throw in some retained messages from another server
    putCount = totalOtherServer;
    for(i=0; i<totalOtherServer; i++)
    {
        sprintf(topicString, "OtherServer/%08d/Test", i);

        payload=NULL;

        uint8_t persistence, reliability;

        if ((rand()%totalRetained) > (totalRetained*0.75))
        {
            persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
        }
        else
        {
            persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        }

        reliability =-(uint8_t)(i%3);

        rc = test_createOriginMessage(10,
                                      persistence,
                                      reliability,
                                      ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                      0,
                                      ismDESTINATION_TOPIC, topicString,
                                      "OtherServer", ism_engine_retainedServerTime(),
                                      MTYPE_MQTT_Binary,
                                      &hOtherMsg[i], &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        // Publish message
        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                NULL,
                                                hOtherMsg[i],
                                                &pPutCount, sizeof(pPutCount), test_countAsyncPut);
        if (rc == ISMRC_NoMatchingDestinations)
        {
            __sync_fetch_and_sub(pPutCount, 1);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        }

        free(payload);
    }

    while(putCount > 0)
    {
        sched_yield();
    }

    // And some messages that appear to be from an old UID
    putCount = totalOldServer;
    for(i=0; i<totalOldServer; i++)
    {
        sprintf(topicString, "OldServer/%08d/Test", i);

        payload=NULL;

        uint8_t persistence, reliability;

        if (i == 0)
        {
            persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        }
        else
        {
            if ((rand()%totalOldServer) > (totalOldServer/2))
            {
                persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
            }
            else
            {
                persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
            }
        }

        if ((reliability =-(uint8_t)(i%3)) == 0)
        {
            lowQoSCount++;
        }

        rc = test_createOriginMessage(10,
                                      persistence,
                                      reliability,
                                      ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                      0,
                                      ismDESTINATION_TOPIC, topicString,
                                      "OldServer", ism_engine_retainedServerTime(),
                                      MTYPE_MQTT_Binary,
                                      &hOldMsg[i], &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        // Publish message
        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                NULL,
                                                hOldMsg[i],
                                                &pPutCount, sizeof(pPutCount), test_countAsyncPut);
        if (rc == ISMRC_NoMatchingDestinations)
        {
            __sync_fetch_and_sub(pPutCount, 1);

            if (i == 0)
            {
                iettOriginServer_t *originServer = NULL;

                // Fake up the situation that this is an old localServer
                rc = iett_insertOrFindOriginServer(pThreadData, "OldServer", iettOP_FIND, &originServer);
                TEST_ASSERT_EQUAL(rc, OK);
                TEST_ASSERT_PTR_NOT_NULL(originServer);
                originServer->localServer = true;
            }
        }
        else
        {
            TEST_ASSERT_NOT_EQUAL(i, 0);
            TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        }

        free(payload);
    }

    while(putCount > 0)
    {
        sched_yield();
    }

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;
    TEST_ASSERT_PTR_NOT_NULL(remoteServersGlobal);
    TEST_ASSERT_EQUAL(memcmp(remoteServersGlobal->strucId, iersREMOTESERVERS_STRUCID, 4), 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);

    ismEngine_RemoteServerHandle_t createdHandle = ismENGINE_NULL_REMOTESERVER_HANDLE;

    rc = engineCallback(ENGINE_RS_CREATE,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        (ismCluster_RemoteServerHandle_t)NULL,
                        "TESTXXX",
                        "UIDXXX",
                        NULL, 0,
                        NULL, 0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        &createdHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(createdHandle);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 1);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 1);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle);

    // Check that the remote server looks as expected
    ismEngine_RemoteServer_t *remoteServer = createdHandle;
    TEST_ASSERT_EQUAL(memcmp(remoteServer->StrucId, ismENGINE_REMOTESERVER_STRUCID, 4), 0);
    TEST_ASSERT_EQUAL(remoteServer->useCount, 1);
    TEST_ASSERT_EQUAL(remoteServer->clusterHandle, (ismCluster_RemoteServerHandle_t)NULL);
    TEST_ASSERT_EQUAL(remoteServer->internalAttrs, iersREMSRVATTR_DISCONNECTED);
    TEST_ASSERT_PTR_NOT_NULL(remoteServer->lowQoSQueueHandle);
    TEST_ASSERT_PTR_NOT_NULL(remoteServer->highQoSQueueHandle);

    // Check all the message usage counts are as expected
    for(i=0; i<totalRetained; i++)
    {
        TEST_ASSERT_EQUAL(hMsg[i]->usageCount, 2);

        // Store refcount is only incremented if
        if (hMsg[i]->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
        {
            if (hMsg[i]->Header.Reliability > ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
            {
                TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, 2);
            }
            else
            {
                TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, 1);
            }
        }
        else
        {
            TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, 0);
        }
    }

    uint32_t totalOldAndNew = totalRetained+totalOldServer;
    // Check that the number of messages on each queue is as expected
    ismEngine_QueueStatistics_t stats;

    ieq_getStats(pThreadData, remoteServer->lowQoSQueueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, lowQoSCount);
    ieq_getStats(pThreadData, remoteServer->highQoSQueueHandle, &stats);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, totalOldAndNew-lowQoSCount);

    // Limit the amount that can be put on seeding queues and try creating another
    // server
    remoteServersGlobal->seedingPolicyHandle->maxMessageBytes = 100;

    ismEngine_RemoteServerHandle_t notSeededHandle = ismENGINE_NULL_REMOTESERVER_HANDLE;

    rc = engineCallback(ENGINE_RS_CREATE,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        (ismCluster_RemoteServerHandle_t)NULL,
                        "TESTYYY",
                        "UIDYYY",
                        NULL, 0,
                        NULL, 0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        &notSeededHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(notSeededHandle);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 2);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 2);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, notSeededHandle);

    // TODO: Check depth of HighQoS queue

    rc = engineCallback(ENGINE_RS_REMOVE,
                        notSeededHandle,
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    remoteServersGlobal->seedingPolicyHandle->maxMessageBytes = 0;

    rc = test_createClientAndSession("CleanupRetainedClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Unset 9 of the retained messages
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_REGEX_TOPIC,
                                                      "Seeding/0000000.*/Test",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check all the message usage counts are as expected
    for(i=0; i<totalRetained; i++)
    {
        if (i<10) {TEST_ASSERT_EQUAL(hMsg[i]->usageCount, 1)}
        else {TEST_ASSERT_EQUAL(hMsg[i]->usageCount, 2)};

        // Store refcount is only incremented if
        if (hMsg[i]->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
        {
            if (hMsg[i]->Header.Reliability > ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
            {
                if (i<10) {TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, 1)}
                else {TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, 2)};
            }
            else
            {
                if (i<10) {TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, 0)}
                else {TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, 1)};
            }
        }
        else
        {
            TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, 0);
        }
    }

    rc = test_createClientAndSessionWithProtocol("__Server_ConsumingServer",
                                                 PROTOCOL_ID_FWD,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                                 ismENGINE_CREATE_SESSION_TRANSACTIONAL |
                                                 ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                                 &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    // Check all the messages have expected flags by consuming them
    remoteServerMessagesCbContext_t msgContext = {0};
    remoteServerMessagesCbContext_t *msgCb = &msgContext;
    ismEngine_ConsumerHandle_t hConsumer;

    msgContext.hSession = hSession;

    for(i=0; i<2; i++)
    {
        rc = ism_engine_createRemoteServerConsumer(hSession,
                                                   remoteServer,
                                                   &msgCb,
                                                   sizeof(msgCb),
                                                   remoteServerDeliveryCallback,
                                                   i == 0 ? ismENGINE_CONSUMER_OPTION_LOW_QOS : ismENGINE_CONSUMER_OPTION_HIGH_QOS,
                                                   &hConsumer,
                                                   NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(remoteServer->useCount, 2);
        TEST_ASSERT_EQUAL(remoteServer->consumerCount, 1);
        TEST_ASSERT_PTR_NOT_NULL(hConsumer);

        if (i == 0)
        {
            test_waitForMessages(&(msgContext.publishedForRetainSet), NULL, lowQoSCount, 20);
            TEST_ASSERT_EQUAL(msgContext.publishedForRetainSet, lowQoSCount);
            TEST_ASSERT_EQUAL(msgContext.propagateSet, lowQoSCount-lowQoSUnsetCount);
        }
        else
        {
            test_waitForMessages(&(msgContext.publishedForRetainSet), NULL, totalOldAndNew, 20);
            TEST_ASSERT_EQUAL(msgContext.publishedForRetainSet, totalOldAndNew);
            TEST_ASSERT_EQUAL(msgContext.propagateSet, totalOldAndNew-10);
        }

        rc = sync_ism_engine_destroyConsumer(hConsumer);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    test_destroyClientAndSession(hClient, hSession, true);
}

void test_capability_Seeding_Phase2(void)
{
    int32_t rc;
    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;
    ismEngine_RemoteServerHandle_t createdHandle = createdHandle = remoteServersGlobal->serverHead;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_MessageHandle_t hMsg;
    void *payload = NULL;

    printf("Starting %s...\n", __func__);

    // Need to re-find createdHandle now we've bounced the engine
    while(createdHandle != NULL)
    {
        if (strcmp(createdHandle->serverUID, "UIDXXX") == 0)
        {
            break;
        }
        createdHandle = createdHandle->next;
    }

    TEST_ASSERT_PTR_NOT_NULL(createdHandle);

    rc = test_createClientAndSession("CleanupRetainedClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Unset all retained messages
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_REGEX_TOPIC,
                                                      ".*",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = engineCallback(ENGINE_RS_REMOVE,
                        createdHandle,
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);

    // Publish a single NullRetained message
    rc = test_createOriginMessage(0,
                                  ismMESSAGE_PERSISTENCE_PERSISTENT,
                                  ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                  ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                  ism_common_nowExpire() + 30,
                                  ismDESTINATION_TOPIC, "/TOPIC/WITH/NULLRETAINED",
                                  (char *)ism_common_getServerUID(),
                                  ism_engine_retainedServerTime(),
                                  MTYPE_NullRetained,
                                  &hMsg, &payload);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 "/TOPIC/WITH/NULLRETAINED",
                                                 NULL,
                                                 hMsg);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    rc = engineCallback(ENGINE_RS_CREATE,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        (ismCluster_RemoteServerHandle_t)NULL,
                        "TESTXXX",
                        "UIDXXX",
                        NULL, 0,
                        NULL, 0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        &createdHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(createdHandle);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 1);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 1);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle);

    // Check we got the NullRetained
    TEST_ASSERT_EQUAL(((iemqQueue_t *)createdHandle->highQoSQueueHandle)->bufferedMsgs, 1);

    rc = engineCallback(ENGINE_RS_REMOVE,
                        createdHandle,
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);

    // Unset all retained messages
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_REGEX_TOPIC,
                                                      ".*",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_destroyClientAndSession(hClient, hSession, true);
}

//****************************************************************************
/// @brief Test the synchronization routines for clustered retained msgs
//****************************************************************************
void test_capability_RetainedSync(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    // Fake initialization of the timers
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.ActiveTimerUseCount, 0);

    ismEngine_serverGlobal.ActiveTimerUseCount = 1;

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;
    TEST_ASSERT_PTR_NOT_NULL(remoteServersGlobal);

    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);

    // Prove that the function does return that it should runAgain (with no remote servers)
    int runAgain = ietm_syncClusterRetained(NULL, 0, NULL);
    TEST_ASSERT_EQUAL(runAgain, 1);

    ietm_cleanUpTimers();

    // Publish various types of retained messages
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    const char *serverUID[] = {"originServer1", "originServer2", "originServer3"};
    uint32_t totalServers = sizeof(serverUID)/sizeof(serverUID[0]);
    uint32_t retainedPerServer = 5;

    rc = test_createClientAndSession("SeedingRetainedClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    char topicString[50];
    int32_t i;
    ismEngine_MessageHandle_t hMsg[retainedPerServer*totalServers];
    void *payload = NULL;

    // Add a retained message on a system topic that should not get sent
    ismEngine_MessageHandle_t hSysMsg;
    rc = test_createMessage(10, ismMESSAGE_PERSISTENCE_NONPERSISTENT, 0,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN, 0,
                            ismDESTINATION_TOPIC, "$SYS/TEST",
                            &hSysMsg, &payload);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            "$SYS/TEST",
                                            NULL,
                                            hSysMsg,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
    TEST_ASSERT_EQUAL(hSysMsg->usageCount, 1);

    free(payload);

    // Publish retained messages from several origin servers
    uint32_t lowQoSCount[totalServers];

    ism_time_t earlyTimestamp = ism_engine_retainedServerTime() - 5;

    uint32_t putCount = 0;
    uint32_t *pPutCount = &putCount;
    int32_t totalPut = 0;
    for(int32_t x=0; x<totalServers;x++)
    {
        lowQoSCount[x] = 0;

        for(i=0; i<retainedPerServer; i++) /* BEAM suppression: loop doesn't iterate */
        {
            __sync_fetch_and_add(pPutCount, 1);

            sprintf(topicString, "Forwarding/%08d/From%s/Test", i, serverUID[x]);

            payload=NULL;

            uint8_t persistence, reliability;

            if ((rand()%retainedPerServer) > (retainedPerServer/2))
            {
                persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
            }
            else
            {
                persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
            }

            if ((reliability =-(uint8_t)(i%3)) == 0) lowQoSCount[x]++;

            uint32_t msgIndex = (x*retainedPerServer)+i;

            totalPut++;

            rc = test_createOriginMessage(10,
                                          persistence,
                                          reliability,
                                          ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                          0,
                                          ismDESTINATION_TOPIC, topicString,
                                          serverUID[x], ism_engine_retainedServerTime(),
                                          MTYPE_MQTT_Binary,
                                          &hMsg[msgIndex], &payload);
            TEST_ASSERT_EQUAL(rc, OK);

            // Publish message
            rc = ism_engine_putMessageOnDestination(hSession,
                                                    ismDESTINATION_TOPIC,
                                                    topicString,
                                                    NULL,
                                                    hMsg[msgIndex],
                                                    &pPutCount, sizeof(pPutCount), test_countAsyncPut);

            if (rc == ISMRC_NoMatchingDestinations)
            {
                __sync_fetch_and_sub(pPutCount, 1);
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
            }

            free(payload);
        }
    }

    while(putCount > 0)
    {
        sched_yield();
    }

    for(i=0; i<totalPut; i++)
    {
        TEST_ASSERT_EQUAL(hMsg[i]->usageCount, 1);
        if (hMsg[i]->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
        {
            TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, 1);
        }
        else
        {
            TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, 0);
        }
    }

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(memcmp(remoteServersGlobal->strucId, iersREMOTESERVERS_STRUCID, 4), 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);

    ismEngine_RemoteServerHandle_t createdHandle[2] = {ismENGINE_NULL_REMOTESERVER_HANDLE};

    rc = engineCallback(ENGINE_RS_CREATE,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        (ismCluster_RemoteServerHandle_t)NULL,
                        "requestingServer",
                        "RSUID",
                        NULL, 0,
                        NULL, 0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        &createdHandle[0]);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(createdHandle[0]);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 1);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 1);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle[0]);

    rc = engineCallback(ENGINE_RS_CREATE,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        (ismCluster_RemoteServerHandle_t)NULL,
                        "someOtherServer",
                        "SOUID",
                        NULL, 0,
                        NULL, 0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        &createdHandle[1]);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(createdHandle[1]);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 2);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 2);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle[1]);

    // Check that the remote server looks as expected
    ismEngine_RemoteServer_t *remoteServer = createdHandle[0];
    TEST_ASSERT_EQUAL(memcmp(remoteServer->StrucId, ismENGINE_REMOTESERVER_STRUCID, 4), 0);
    TEST_ASSERT_EQUAL(remoteServer->useCount, 1);
    TEST_ASSERT_EQUAL(remoteServer->clusterHandle, (ismCluster_RemoteServerHandle_t)NULL);
    TEST_ASSERT_EQUAL(remoteServer->internalAttrs, iersREMSRVATTR_DISCONNECTED);

    iemqQueue_t *lowQoSQueue = (iemqQueue_t *)(remoteServer->lowQoSQueueHandle);
    TEST_ASSERT_PTR_NOT_NULL(lowQoSQueue);
    iemqQueue_t *highQoSQueue = (iemqQueue_t *)(remoteServer->highQoSQueueHandle);
    TEST_ASSERT_PTR_NOT_NULL(highQoSQueue);

    TEST_ASSERT_EQUAL(lowQoSQueue->bufferedMsgs, 0);
    TEST_ASSERT_EQUAL(highQoSQueue->bufferedMsgs, 0);

    // Request messages to be forwarded to a non-existent server
    rc = ism_engine_forwardRetainedMessages(serverUID[2],
                                            ismENGINE_FORWARD_RETAINED_OPTION_NONE,
                                            0,
                                            ism_common_currentTimeNanos(),
                                            "NEXUID",
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    uint64_t expectedLowQoSCount = 0;
    uint64_t expectedHighQoSCount = 0;

    // request retained messages twice - thus getting the *SAME* messages on the queue
    // twice.
    for(i=0; i<2; i++)
    {
        uint32_t options;
        ism_time_t timestamp;

        if (i == 0)
        {
            options = ismENGINE_FORWARD_RETAINED_OPTION_NONE;
            timestamp = 0;
        }
        else
        {
            options = ismENGINE_FORWARD_RETAINED_OPTION_NEWER;
            timestamp = earlyTimestamp;
        }

        rc = ism_engine_forwardRetainedMessages(serverUID[0],
                                                options,
                                                timestamp,
                                                ism_common_currentTimeNanos(),
                                                "RSUID",
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        expectedLowQoSCount += lowQoSCount[0];
        expectedHighQoSCount += retainedPerServer-lowQoSCount[0];
        TEST_ASSERT_EQUAL(lowQoSQueue->bufferedMsgs, expectedLowQoSCount);
        TEST_ASSERT_EQUAL(highQoSQueue->bufferedMsgs, expectedHighQoSCount)

        rc = ism_engine_forwardRetainedMessages(serverUID[1],
                                                options,
                                                timestamp,
                                                ism_common_currentTimeNanos(),
                                                "RSUID",
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        expectedLowQoSCount += lowQoSCount[1];
        expectedHighQoSCount += (retainedPerServer-lowQoSCount[1]);
        TEST_ASSERT_EQUAL(lowQoSQueue->bufferedMsgs, expectedLowQoSCount);
        TEST_ASSERT_EQUAL(highQoSQueue->bufferedMsgs, expectedHighQoSCount);
    }

    // Check all the message usage counts are as expected
    for(i=0; i<totalServers*retainedPerServer; i++)
    {
        uint32_t expectedUsageCount;

        if (i<2*retainedPerServer)
        {
            expectedUsageCount = 3;
        }
        else
        {
            expectedUsageCount = 1;
        }

        TEST_ASSERT_EQUAL(hMsg[i]->usageCount, expectedUsageCount);

        // Store refcount is only incremented if
        if (hMsg[i]->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
        {
            if (hMsg[i]->Header.Reliability > ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
            {
                TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, expectedUsageCount);
            }
            else
            {
                TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, 1);
            }
        }
        else
        {
            TEST_ASSERT_EQUAL(hMsg[i]->StoreMsg.parts.RefCount, 0);
        }
    }

    // Check that the number of messages on each queue is as expected
    ismEngine_RemoteServerStatistics_t stats;

    rc = ism_engine_getRemoteServerStatistics(remoteServer, &stats);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(stats.q0.BufferedMsgs, expectedLowQoSCount);
    TEST_ASSERT_EQUAL(stats.q1.BufferedMsgs, expectedHighQoSCount);

    // Remove all retained messages
    rc = test_createClientAndSession("CleanupRetainedClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Unset all of retained messages
    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_REGEX_TOPIC,
                                                      ".*",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSessionWithProtocol("__Server_ConsumingServer",
                                                 PROTOCOL_ID_FWD,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                                 ismENGINE_CREATE_SESSION_TRANSACTIONAL |
                                                 ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                                 &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    // Check all the messages still have PROPAGATE set by consuming them even though they have all
    // been unset as retained messages
    remoteServerMessagesCbContext_t msgContext = {0};
    remoteServerMessagesCbContext_t *msgCb = &msgContext;
    ismEngine_ConsumerHandle_t hConsumer;

    msgContext.hSession = hSession;

    for(i=0; i<2; i++)
    {
        rc = ism_engine_createRemoteServerConsumer(hSession,
                                                   remoteServer,
                                                   &msgCb,
                                                   sizeof(msgCb),
                                                   remoteServerDeliveryCallback,
                                                   i == 0 ? ismENGINE_CONSUMER_OPTION_LOW_QOS : ismENGINE_CONSUMER_OPTION_HIGH_QOS,
                                                   &hConsumer,
                                                   NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(remoteServer->useCount, 2);
        TEST_ASSERT_EQUAL(remoteServer->consumerCount, 1);
        TEST_ASSERT_PTR_NOT_NULL(hConsumer);

        if (i == 0)
        {
            test_waitForMessages(&(msgContext.publishedForRetainSet), NULL, expectedLowQoSCount, 20);
            TEST_ASSERT_EQUAL(msgContext.publishedForRetainSet, expectedLowQoSCount);
            TEST_ASSERT_EQUAL(msgContext.propagateSet, 0);
        }
        else
        {
            test_waitForMessages(&(msgContext.publishedForRetainSet), NULL, expectedHighQoSCount+expectedLowQoSCount, 20);
            TEST_ASSERT_EQUAL(msgContext.publishedForRetainSet, expectedHighQoSCount+expectedLowQoSCount);
            TEST_ASSERT_EQUAL(msgContext.propagateSet, 0);
        }

        rc = sync_ism_engine_destroyConsumer(hConsumer);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    test_destroyClientAndSession(hClient, hSession, true);

    rc = engineCallback(ENGINE_RS_REMOVE,
                        createdHandle[1],
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 1);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 1);

    rc = engineCallback(ENGINE_RS_REMOVE,
                        createdHandle[0],
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_PTR_NULL(remoteServersGlobal->serverHead);
}

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_ClusterRetained_Phase1[] =
{
    { "RetainedSync", test_capability_RetainedSync },
    { "Seeding", test_capability_Seeding_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_ClusterRetained_Phase2[] =
{
    { "Seeding", test_capability_Seeding_Phase2 },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the operation of create / remove with forced store failures
//****************************************************************************
// fake ism_store_createRecord
ismStore_RecordType_t *ism_store_createRecord_FailForRecordType = NULL;
uint32_t *ism_store_createRecord_FailForRecordTypeCount = NULL;
int32_t *ism_store_createRecord_FailForRecordTypeRC = NULL;
int32_t ism_store_createRecord(ismStore_StreamHandle_t hStream,
                               const ismStore_Record_t *pRecord,
                               ismStore_Handle_t *pHandle)
{
    int32_t rc = OK;
    static int32_t (*real_ism_store_createRecord)(ismStore_StreamHandle_t, const ismStore_Record_t *, ismStore_Handle_t *) = NULL;

    if (real_ism_store_createRecord == NULL)
    {
        real_ism_store_createRecord  = dlsym(RTLD_NEXT, "ism_store_createRecord");
    }

    if (ism_store_createRecord_FailForRecordType != NULL &&
        ism_store_createRecord_FailForRecordTypeCount != NULL &&
        ism_store_createRecord_FailForRecordTypeRC != NULL)
    {
        ismStore_RecordType_t *failRecordType = ism_store_createRecord_FailForRecordType;
        uint32_t *callCount = ism_store_createRecord_FailForRecordTypeCount;
        int32_t *failRC = ism_store_createRecord_FailForRecordTypeRC;

        for(; *failRecordType; failRecordType++, callCount++, failRC++)
        {
            if (*failRecordType == pRecord->Type)
            {
                *callCount = *callCount - 1;

                if (*callCount == 0)
                {
                    rc = *failRC;
                    break;
                }
            }
        }
    }

    if (rc == OK) rc = real_ism_store_createRecord(hStream, pRecord, pHandle);

    return rc;
}

// fake ism_store_updateRecord
ismStore_Handle_t *ism_store_updateRecord_FailForHandle = NULL;
uint32_t *ism_store_updateRecord_FailForHandleCount = NULL;
int32_t *ism_store_updateRecord_FailForHandleRC = NULL;
int32_t ism_store_updateRecord(ismStore_StreamHandle_t hStream,
                               ismStore_Handle_t handle,
                               uint64_t attribute,
                               uint64_t state,
                               uint8_t  flags)
{
    int32_t rc = OK;
    static int32_t (*real_ism_store_updateRecord)(ismStore_StreamHandle_t, ismStore_Handle_t, uint64_t, uint64_t, uint8_t) = NULL;

    if (real_ism_store_updateRecord == NULL)
    {
        real_ism_store_updateRecord  = dlsym(RTLD_NEXT, "ism_store_updateRecord");
    }

    if (ism_store_updateRecord_FailForHandle != NULL &&
        ism_store_updateRecord_FailForHandleCount != NULL &&
        ism_store_updateRecord_FailForHandleRC != NULL)
    {
        ismStore_Handle_t *failHandle = ism_store_updateRecord_FailForHandle;
        uint32_t *callCount = ism_store_updateRecord_FailForHandleCount;
        int32_t *failRC = ism_store_updateRecord_FailForHandleRC;

        for(; *failHandle; failHandle++, callCount++, failRC++)
        {
            if (*failHandle == handle)
            {
                *callCount = *callCount - 1;

                if (*callCount == 0)
                {
                    rc = *failRC;
                    break;
                }
            }
        }
    }

    if (rc == OK) rc = real_ism_store_updateRecord(hStream, handle, attribute, state, flags);

    return rc;
}

// fake ism_store_deleteRecord
ismStore_Handle_t *ism_store_deleteRecord_FailForHandle = NULL;
uint32_t *ism_store_deleteRecord_FailForHandleCount = NULL;
int32_t *ism_store_deleteRecord_FailForHandleRC = NULL;
int32_t ism_store_deleteRecord(ismStore_StreamHandle_t hStream,
                               ismStore_Handle_t handle)
{
    int32_t rc = OK;
    static int32_t (*real_ism_store_deleteRecord)(ismStore_StreamHandle_t, ismStore_Handle_t) = NULL;

    if (real_ism_store_deleteRecord == NULL)
    {
        real_ism_store_deleteRecord  = dlsym(RTLD_NEXT, "ism_store_deleteRecord");
    }

    if (ism_store_deleteRecord_FailForHandle != NULL &&
        ism_store_deleteRecord_FailForHandleCount != NULL &&
        ism_store_deleteRecord_FailForHandleRC != NULL)
    {
        ismStore_Handle_t *failHandle = ism_store_deleteRecord_FailForHandle;
        uint32_t *callCount = ism_store_deleteRecord_FailForHandleCount;
        int32_t *failRC = ism_store_deleteRecord_FailForHandleRC;

        for(; *failHandle; failHandle++, callCount++, failRC++)
        {
            if (*failHandle == handle)
            {
                *callCount = *callCount - 1;

                if (*callCount == 0)
                {
                    rc = *failRC;
                    break;
                }
            }
        }
    }

    if (rc == OK) rc = real_ism_store_deleteRecord(hStream, handle);

    return rc;
}

void test_capability_StoreErrorsCreateUpdateRemove(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    swallow_ffdcs = true;

    // ENGINE_RS_CREATE*: Force some store failures on ism_store_createRecord
    ismStore_RecordType_t failForRecordType1[] = {ISM_STORE_RECTYPE_RPROP,
                                                  ISM_STORE_RECTYPE_RPROP,
                                                  ISM_STORE_RECTYPE_REMSRV,
                                                  ISM_STORE_RECTYPE_REMSRV,
                                                  0};
    uint32_t failForRecordTypeCount1[] = {1,1,1,1,0};
    int32_t failForRecordTypeRC1[] = {ISMRC_StoreGenerationFull,
                                      ISMRC_AllocateError,
                                      ISMRC_StoreGenerationFull,
                                      ISMRC_AllocateError,
                                      ISMRC_OK};

    ism_store_createRecord_FailForRecordType = failForRecordType1;
    ism_store_createRecord_FailForRecordTypeCount = failForRecordTypeCount1;
    ism_store_createRecord_FailForRecordTypeRC = failForRecordTypeRC1;

    // Create a local server structure
    ismEngine_RemoteServerHandle_t localServerHandle = NULL;

    for(int32_t loop=0; loop<3; loop++)
    {
        rc = engineCallback(ENGINE_RS_CREATE_LOCAL,
                            ismENGINE_NULL_REMOTESERVER_HANDLE,
                            (ismCluster_RemoteServerHandle_t)NULL,
                            "LOCALSERVER",
                            "LSUID",
                            NULL, 0,
                            NULL, 0,
                            false, false, NULL, NULL,
                            engineCallbackContext,
                            &localServerHandle);
        if (loop != 2)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_AllocateError);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    TEST_ASSERT_PTR_NOT_NULL(localServerHandle);
    TEST_ASSERT_NOT_EQUAL(localServerHandle->hStoreDefn, ismSTORE_NULL_HANDLE);

    ismStore_Handle_t hPrevDefn = localServerHandle->hStoreDefn;
    ismStore_Handle_t hPrevProps = localServerHandle->hStoreProps;

    // Back to normal
    ism_store_createRecord_FailForRecordType = NULL;
    ism_store_createRecord_FailForRecordTypeCount = NULL;
    ism_store_createRecord_FailForRecordTypeRC = NULL;

    // ENGINE_RS_UPDATE*: Force some store failures on ism_store_createRecord
    ismStore_RecordType_t failForRecordType2[] = {ISM_STORE_RECTYPE_RPROP, 0};
    uint32_t failForRecordTypeCount2[] = {1,0};
    int32_t failForRecordTypeRC2[] = {ISMRC_AllocateError, ISMRC_OK};

    ism_store_createRecord_FailForRecordType = failForRecordType2;
    ism_store_createRecord_FailForRecordTypeCount = failForRecordTypeCount2;
    ism_store_createRecord_FailForRecordTypeRC = failForRecordTypeRC2;

    // Update the local server
    rc = engineCallback(ENGINE_RS_UPDATE,
                        localServerHandle,
                        0, NULL, NULL,
                        "UPDATED DATA", strlen("UPDATED DATA")+1,
                        NULL, 0,
                        false, true, NULL, NULL,
                        engineCallbackContext,
                        NULL);

    TEST_ASSERT_EQUAL(rc, ISMRC_AllocateError);
    TEST_ASSERT_EQUAL(localServerHandle->hStoreDefn, hPrevDefn);
    TEST_ASSERT_EQUAL(localServerHandle->hStoreProps, hPrevProps);
    // Left over values
    TEST_ASSERT_PTR_NOT_NULL(localServerHandle->clusterData);
    TEST_ASSERT_NOT_EQUAL(localServerHandle->clusterDataLength, 0);

    // Back to normal
    ism_store_createRecord_FailForRecordType = NULL;
    ism_store_createRecord_FailForRecordTypeCount = NULL;
    ism_store_createRecord_FailForRecordTypeRC = NULL;

    // ENGINE_RS_REMOVE: Force some store failures on ism_store_updateRecord
    ismStore_Handle_t failForHandle1[] = {localServerHandle->hStoreDefn,
                                         localServerHandle->hStoreDefn,
                                         0};
    uint32_t failForHandleCount1[] = {1,1,0};
    int32_t failForHandleRC1[] = {ISMRC_AllocateError, ISMRC_AllocateError, 0};

    ism_store_updateRecord_FailForHandle = failForHandle1;
    ism_store_updateRecord_FailForHandleCount = failForHandleCount1;
    ism_store_updateRecord_FailForHandleRC = failForHandleRC1;

    rc = engineCallback(ENGINE_RS_REMOVE,
                        localServerHandle,
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_AllocateError);

    // Back to normal
    ism_store_updateRecord_FailForHandle = NULL;
    ism_store_updateRecord_FailForHandleCount = NULL;
    ism_store_updateRecord_FailForHandleRC = NULL;

    ismEngine_RemoteServerHandle_t remoteServerHandle = NULL;

    rc = engineCallback(ENGINE_RS_CREATE,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        (ismCluster_RemoteServerHandle_t)NULL,
                        "RemoteServer",
                        "RSUID",
                        NULL, 0,
                        NULL, 0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        &remoteServerHandle);
    TEST_ASSERT_EQUAL(rc, OK);

    // Force some store failures on ism_store_deleteRecord
    ismStore_Handle_t failForHandle2[] = {localServerHandle->hStoreDefn,
                                          localServerHandle->hStoreProps,
                                          remoteServerHandle->hStoreDefn,
                                          remoteServerHandle->hStoreProps,
                                          0};
    uint32_t failForHandleCount2[] = {1,1,1,1,0};
    int32_t failForHandleRC2[] = {ISMRC_AllocateError,
                                  ISMRC_AllocateError,
                                  ISMRC_AllocateError,
                                  ISMRC_AllocateError,
                                  0};

    ism_store_deleteRecord_FailForHandle = failForHandle2;
    ism_store_deleteRecord_FailForHandleCount = failForHandleCount2;
    ism_store_deleteRecord_FailForHandleRC = failForHandleRC2;

    rc = engineCallback(ENGINE_RS_REMOVE,
                        localServerHandle,
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = engineCallback(ENGINE_RS_REMOVE,
                        remoteServerHandle,
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Back to normal
    ism_store_deleteRecord_FailForHandle = NULL;
    ism_store_deleteRecord_FailForHandleCount = NULL;
    ism_store_deleteRecord_FailForHandleRC = NULL;

    swallow_ffdcs = false;
}

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_HorridThings[] =
{
    { "StoreErrorsCreateUpdateRemove", test_capability_StoreErrorsCreateUpdateRemove },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the recovery of remote servers
//****************************************************************************
void test_capability_Recovery_Phase1(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    ismEngine_RemoteServerHandle_t createdHandle[4] = {ismENGINE_NULL_REMOTESERVER_HANDLE};

    // Use cluster callback interface to create 4 servers
    uint64_t i=1;
    for(; i<=4; i++)
    {
        char serverName[10];
        char serverUID[10];

        sprintf(serverName, "TEST%lu", i);
        sprintf(serverUID, "UID%lu", i);

        rc = engineCallback(ENGINE_RS_CREATE,
                            ismENGINE_NULL_REMOTESERVER_HANDLE,
                            (ismCluster_RemoteServerHandle_t)NULL,
                            serverName,
                            serverUID,
                            NULL, 0,
                            NULL, 0,
                            false, false, NULL, NULL,
                            engineCallbackContext,
                            &createdHandle[i-1]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(createdHandle[i-1]);
        TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, i);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, i);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle[i-1]);
    }

    ismEngine_RemoteServerHandle_t localServerHandle = ismENGINE_NULL_REMOTESERVER_HANDLE;

    // Create a local server structure
    rc = engineCallback(ENGINE_RS_CREATE_LOCAL,
                        ismENGINE_NULL_REMOTESERVER_HANDLE,
                        (ismCluster_RemoteServerHandle_t)NULL,
                        "LOCALSERVER",
                        "LSUID",
                        NULL, 0,
                        NULL, 0,
                        false, false, NULL, NULL,
                        engineCallbackContext,
                        &localServerHandle);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(localServerHandle);
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, i-1);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, i);

    rc = engineCallback(ENGINE_RS_UPDATE,
                        localServerHandle,
                        0,
                        NULL, NULL,
                        "LOCALSERVERDATA", strlen("LOCALSERVERDATA")+1,
                        NULL, 0,
                        false, true, NULL, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_Recovery_Phase2(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    uint32_t expectedRemoteServerCount = 4;
    uint32_t expectedServerCount = 5;

    TEST_ASSERT_EQUAL(restoreRemoteServers_callCount, 1);
    TEST_ASSERT_EQUAL(restoreRemoteServers_serverCount, expectedServerCount);
    TEST_ASSERT_EQUAL(recoveryCompleted_callCount, 1);

    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, expectedRemoteServerCount);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, expectedServerCount);

    // Fake partial deletion of a remote server
    ismEngine_RemoteServer_t *remoteServer = NULL;
    ismEngine_RemoteServer_t *loopRemoteServer = remoteServersGlobal->serverHead;
    while(loopRemoteServer)
    {
        // Check queues and attributes of queues have been recovered as expected
        if ((loopRemoteServer->internalAttrs & iersREMSRVATTR_LOCAL) == iersREMSRVATTR_LOCAL)
        {
            TEST_ASSERT_PTR_NULL(loopRemoteServer->lowQoSQueueHandle);
            TEST_ASSERT_PTR_NULL(loopRemoteServer->highQoSQueueHandle);
        }
        else
        {
            TEST_ASSERT_PTR_NOT_NULL(loopRemoteServer->lowQoSQueueHandle);
            iemqQueue_t *lowQueue = (iemqQueue_t *)loopRemoteServer->lowQoSQueueHandle;
            TEST_ASSERT_EQUAL(lowQueue->ackListsUpdating, true);
            TEST_ASSERT_EQUAL(lowQueue->QOptions & ieqOptions_IN_RECOVERY, 0);
            TEST_ASSERT_EQUAL(strstr(lowQueue->InternalName, "+RDR"), lowQueue->InternalName);
            TEST_ASSERT_PTR_NOT_NULL(lowQueue->headPage);
            TEST_ASSERT_PTR_NOT_NULL(lowQueue->tail);

            TEST_ASSERT_PTR_NOT_NULL(loopRemoteServer->highQoSQueueHandle);
            iemqQueue_t *highQueue = (iemqQueue_t *)loopRemoteServer->highQoSQueueHandle;
            TEST_ASSERT_EQUAL(highQueue->ackListsUpdating, true);
            TEST_ASSERT_EQUAL(highQueue->QOptions & ieqOptions_IN_RECOVERY, 0);
            TEST_ASSERT_EQUAL(strstr(highQueue->InternalName, "+RDR"), highQueue->InternalName);
            TEST_ASSERT_PTR_NOT_NULL(highQueue->headPage);
            TEST_ASSERT_PTR_NOT_NULL(highQueue->tail);
        }

        if (strcmp(loopRemoteServer->serverName, "TEST2") == 0) remoteServer = loopRemoteServer;
        loopRemoteServer = loopRemoteServer->next;
    }
    TEST_ASSERT_PTR_NOT_NULL(remoteServer);

    do
    {
        rc = ism_store_updateRecord(pThreadData->hStream,
                                    remoteServer->hStoreDefn,
                                    ismSTORE_NULL_HANDLE,
                                    iestRDR_STATE_DELETED,
                                    ismSTORE_SET_STATE);
    }
    while(rc == ISMRC_StoreGenerationFull);
    TEST_ASSERT_EQUAL(rc, OK);

    iest_store_commit(pThreadData, false);
}

void test_capability_Recovery_Phase3(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    uint32_t expectedRemoteServerCount = 3;
    uint32_t expectedServerCount = 4;

    TEST_ASSERT_EQUAL(restoreRemoteServers_callCount, 1);
    TEST_ASSERT_EQUAL(restoreRemoteServers_serverCount, expectedServerCount);
    TEST_ASSERT_EQUAL(recoveryCompleted_callCount, 1);

    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, expectedRemoteServerCount);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, expectedServerCount);

    // Full deletion of a remote server
    ismEngine_RemoteServer_t *remoteServer = remoteServersGlobal->serverHead;
    while(remoteServer)
    {
        if (strcmp(remoteServer->serverName, "TEST1") == 0)
        {
            TEST_ASSERT_EQUAL(strcmp(remoteServer->serverUID, "UID1"), 0);
            break;
        }
        remoteServer = remoteServer->next;
    }
    TEST_ASSERT_PTR_NOT_NULL(remoteServer);

    rc = engineCallback(ENGINE_RS_REMOVE,
                        remoteServer,
                        0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    expectedRemoteServerCount -= 1;
    expectedServerCount -= 1;
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, expectedRemoteServerCount);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, expectedServerCount);

    // Update a remote server with some data
    remoteServer = remoteServersGlobal->serverHead;
    while(remoteServer)
    {
        if (strcmp(remoteServer->serverName, "TEST3") == 0)
        {
            TEST_ASSERT_EQUAL(strcmp(remoteServer->serverUID, "UID3"), 0);
            break;
        }
        remoteServer = remoteServer->next;
    }
    TEST_ASSERT_PTR_NOT_NULL(remoteServer);

    rc = engineCallback(ENGINE_RS_UPDATE,
                        remoteServer,
                        0,
                        NULL, NULL,
                        "TESTDATA", 9,
                        NULL, 0,
                        false, true, NULL, NULL,
                        engineCallbackContext,
                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_Recovery_Phase4(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    uint32_t expectedRemoteServerCount = 2;
    uint32_t expectedServerCount = 3;

    TEST_ASSERT_EQUAL(restoreRemoteServers_callCount, 1);
    TEST_ASSERT_EQUAL(restoreRemoteServers_serverCount, expectedServerCount);
    TEST_ASSERT_EQUAL(recoveryCompleted_callCount, 1);

    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, expectedRemoteServerCount);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, expectedServerCount);

    // Fake partial creation of a remote server
    ismEngine_RemoteServer_t *remoteServer = remoteServersGlobal->serverHead;
    while(remoteServer)
    {
        if (strcmp(remoteServer->serverName, "TEST4") == 0)
        {
            TEST_ASSERT_EQUAL(strcmp(remoteServer->serverUID, "UID4"), 0);
            break;
        }
        remoteServer = remoteServer->next;
    }
    TEST_ASSERT_PTR_NOT_NULL(remoteServer);

    do
    {
        rc = ism_store_updateRecord(pThreadData->hStream,
                                    remoteServer->hStoreDefn,
                                    ismSTORE_NULL_HANDLE,
                                    iestRDR_STATE_CREATING,
                                    ismSTORE_SET_STATE);
    }
    while(rc == ISMRC_StoreGenerationFull);
    TEST_ASSERT_EQUAL(rc, OK);

    iest_store_commit(pThreadData, false);
}

void test_capability_Recovery_Phase5(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    uint32_t expectedServerCount = 2;

    TEST_ASSERT_EQUAL(restoreRemoteServers_callCount, 1);
    TEST_ASSERT_EQUAL(restoreRemoteServers_serverCount, expectedServerCount);
    TEST_ASSERT_EQUAL(recoveryCompleted_callCount, 1);

    // Fake partial deletion of the local remote server
    ismEngine_RemoteServer_t *remoteServer = remoteServersGlobal->serverHead;
    while(remoteServer)
    {
        if (strcmp(remoteServer->serverName, "LOCALSERVER") == 0)
        {
            TEST_ASSERT_EQUAL(strcmp(remoteServer->serverUID, "LSUID"), 0);
            break;
        }
        remoteServer = remoteServer->next;
    }
    TEST_ASSERT_PTR_NOT_NULL(remoteServer);

    do
    {
        rc = ism_store_updateRecord(pThreadData->hStream,
                                    remoteServer->hStoreDefn,
                                    ismSTORE_NULL_HANDLE,
                                    iestRDR_STATE_DELETED,
                                    ismSTORE_SET_STATE);
    }
    while(rc == ISMRC_StoreGenerationFull);
    TEST_ASSERT_EQUAL(rc, OK);

    iest_store_commit(pThreadData, false);
}

void test_capability_Recovery_Phase6(void)
{
    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    uint32_t expectedRemoteServerCount = 1;
    uint32_t expectedServerCount = 1;

    TEST_ASSERT_EQUAL(restoreRemoteServers_callCount, 1);
    TEST_ASSERT_EQUAL(restoreRemoteServers_serverCount, expectedServerCount);
    TEST_ASSERT_EQUAL(recoveryCompleted_callCount, 1);

    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, expectedRemoteServerCount);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, expectedServerCount);

#if 0
    ismEngine_RemoteServer_t *remoteServer = remoteServersGlobal->serverHead;
    while(remoteServer)
    {
        printf("GUID:%s ATTRS:0x%08x DEFN:0x%lx PROPS:0x%lx LOW:%p HIGH:%p\n", remoteServer->serverUID,
               remoteServer->internalAttrs,
               remoteServer->hStoreDefn, remoteServer->hStoreProps,
               remoteServer->lowQoSQueueHandle, remoteServer->highQoSQueueHandle);
        remoteServer = remoteServer->next;
    }
#endif

    // Allow natural cleanup
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_capability_SubscriptionRecovery              */
/*                                                                   */
/* Description:    Test the correctness of subscription reporting to */
/*                 the clustering component at recovery.             */
/*                                                                   */
/*********************************************************************/
#define NUM_TOPICS 50
void test_capability_SubscriptionRecovery_Phase1(void)
{
    int32_t           rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("TEST_CLIENT",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    /* Register Subscribers */
    for(int32_t i=0; i<NUM_TOPICS; i++)
    {
        char currentTopicString[40];

        sprintf(currentTopicString, "/Topic/%d", i);

        if (i%5 == 0)
        {
            int randomNumber = rand()%40;

            if (randomNumber > 35)
            {
                strcat(currentTopicString, "/+/A/B/C");
            }
            else if (randomNumber > 30)
            {
                strcat(currentTopicString, "/+");
            }
            else
            {
                strcat(currentTopicString, "/#");
            }
        }

        // Add some subscriptions
        int32_t currentSubCount = (i/3)+1;
        for(int32_t c=0; c<currentSubCount; c++)
        {
            char subName[50];

            sprintf(subName, "SUB%02d%02d", i, c);

            // This will result in an ism_engine_registerSubscriber call
            rc = sync_ism_engine_createSubscription(hClient,
                                                    subName,
                                                    NULL,
                                                    ismDESTINATION_TOPIC,
                                                    currentTopicString,
                                                    &subAttrs,
                                                    NULL); // Unused for TOPIC

            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(addSubscriptions_callCount, NUM_TOPICS);
    TEST_ASSERT_EQUAL(addSubscriptions_topicCount, NUM_TOPICS);
    TEST_ASSERT_EQUAL(addSubscriptions_wildcardCount, NUM_TOPICS/5);
    TEST_ASSERT_EQUAL(removeSubscriptions_callCount, 0);
    TEST_ASSERT_EQUAL(removeSubscriptions_topicCount, 0);
    TEST_ASSERT_EQUAL(removeSubscriptions_wildcardCount, 0);
}

void test_capability_SubscriptionRecovery_Phase2(void)
{
    int32_t           rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    printf("Starting %s...\n", __func__);

    TEST_ASSERT_EQUAL(addSubscriptions_callCount, 1); // Should be batched
    TEST_ASSERT_EQUAL(addSubscriptions_topicCount, NUM_TOPICS);
    TEST_ASSERT_EQUAL(addSubscriptions_wildcardCount, NUM_TOPICS/5);
    TEST_ASSERT_EQUAL(removeSubscriptions_callCount, 0);
    TEST_ASSERT_EQUAL(removeSubscriptions_topicCount, 0);
    TEST_ASSERT_EQUAL(removeSubscriptions_wildcardCount, 0);

    rc = test_createClientAndSession("TEST_CLIENT",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    int32_t currentSubCount;
    for(int32_t i=0; i<NUM_TOPICS; i++)
    {
        currentSubCount = (i/3)+1;
        for(int32_t c=0; c<currentSubCount; c++)
        {
            char subName[50];

            sprintf(subName, "SUB%02d%02d", i, c);

            rc = ism_engine_destroySubscription(hClient, subName, hClient, NULL, 0, NULL);

            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(removeSubscriptions_callCount, NUM_TOPICS);
    TEST_ASSERT_EQUAL(removeSubscriptions_topicCount, NUM_TOPICS);
    TEST_ASSERT_EQUAL(removeSubscriptions_wildcardCount, NUM_TOPICS/5);

    // AddSubscription hasn't been called again...
    TEST_ASSERT_EQUAL(addSubscriptions_callCount, 1);
    TEST_ASSERT_EQUAL(addSubscriptions_topicCount, NUM_TOPICS);
    TEST_ASSERT_EQUAL(addSubscriptions_wildcardCount, NUM_TOPICS/5);
}

void test_capability_SubscriptionRecovery_Phase3(void)
{
    printf("Starting %s...\n", __func__);

    TEST_ASSERT_EQUAL(removeSubscriptions_callCount, 0);
    TEST_ASSERT_EQUAL(removeSubscriptions_topicCount, 0);
    TEST_ASSERT_EQUAL(removeSubscriptions_wildcardCount, 0);
    TEST_ASSERT_EQUAL(addSubscriptions_callCount, 0);
    TEST_ASSERT_EQUAL(addSubscriptions_topicCount, 0);
    TEST_ASSERT_EQUAL(addSubscriptions_wildcardCount, 0);
}
#undef NUM_TOPICS

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery_Phase1[] =
{
    { "Recovery", test_capability_Recovery_Phase1 },
    { "SubscriptionRecovery", test_capability_SubscriptionRecovery_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery_Phase2[] =
{
    { "Recovery", test_capability_Recovery_Phase2 },
    { "SubscriptionRecovery", test_capability_SubscriptionRecovery_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery_Phase3[] =
{
    { "Recovery", test_capability_Recovery_Phase3 },
    { "SubscriptionRecovery", test_capability_SubscriptionRecovery_Phase3 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery_Phase4[] =
{
    { "Recovery", test_capability_Recovery_Phase4 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery_Phase5[] =
{
    { "Recovery", test_capability_Recovery_Phase5 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery_Phase6[] =
{
    { "Recovery", test_capability_Recovery_Phase6 },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the recovery of many (>100) remote servers (to drive different
///        logic when calling cluster component).
//****************************************************************************
#define TEST_MANYSERVER_COUNT 250
void test_capability_RecoverMany_Phase1(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    ismEngine_RemoteServerHandle_t createdHandle[TEST_MANYSERVER_COUNT] = {ismENGINE_NULL_REMOTESERVER_HANDLE};

    uint32_t expectedRemoteServerCount = 0;
    uint32_t expectedServerCount = 0;

    // Use cluster callback interface to create 250 servers
    int64_t i=1;
    for(; i<=(sizeof(createdHandle)/sizeof(createdHandle[0])); i++)
    {
        char serverName[20];
        char serverUID[10];

        sprintf(serverName, "Server%ldof%lu", i, (uint64_t)(sizeof(createdHandle)/sizeof(createdHandle[0])));
        sprintf(serverUID, "UID%ld", i);

        // Mix of local and (mostly) remote remote servers
        rc = engineCallback((i%10) == 0 ? ENGINE_RS_CREATE_LOCAL : ENGINE_RS_CREATE,
                            ismENGINE_NULL_REMOTESERVER_HANDLE,
                            (ismCluster_RemoteServerHandle_t)NULL,
                            serverName,
                            serverUID,
                            NULL, 0,
                            NULL, 0,
                            false, false, NULL, NULL,
                            engineCallbackContext,
                            &createdHandle[i-1]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(createdHandle[i-1]);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle[i-1]);

        ismEngine_RemoteServer_t *remoteServer = createdHandle[i-1];

        if ((remoteServer->internalAttrs & iersREMSRVATTR_LOCAL) == 0)
        {
            expectedRemoteServerCount += 1;
        }

        expectedServerCount += 1;

        TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, expectedRemoteServerCount);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, expectedServerCount);

        if (rand()%20>10)
        {
            rc = engineCallback(ENGINE_RS_UPDATE,
                                remoteServer,
                                0,
                                NULL, NULL,
                                serverName, strlen(serverName)+1,
                                NULL, 0,
                                false, true, NULL, NULL,
                                engineCallbackContext,
                                NULL);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NULL(remoteServer->clusterData);       // Check not set during runtime
            TEST_ASSERT_EQUAL(remoteServer->clusterDataLength, 0); // Check not set during runtime
        }
    }

    TEST_ASSERT_EQUAL(expectedServerCount, TEST_MANYSERVER_COUNT);
    TEST_ASSERT_EQUAL(expectedRemoteServerCount, TEST_MANYSERVER_COUNT-(TEST_MANYSERVER_COUNT/10));
}

void test_capability_RecoverMany_Phase1b(void)
{
    printf("Starting %s...\n", __func__);
}

void test_capability_RecoverMany_Phase1c(void)
{
    printf("Starting %s...\n", __func__);
}

void test_capability_RecoverMany_Phase2(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    uint32_t expectedRemoteServerCount = TEST_MANYSERVER_COUNT-(TEST_MANYSERVER_COUNT/10);
    uint32_t expectedServerCount = TEST_MANYSERVER_COUNT;

    TEST_ASSERT_EQUAL(restoreRemoteServers_callCount, 3);
    TEST_ASSERT_EQUAL(restoreRemoteServers_serverCount, expectedServerCount);
    TEST_ASSERT_EQUAL(recoveryCompleted_callCount, 1);

    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, expectedRemoteServerCount);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, expectedServerCount);

    // Delete all servers (in reverse order, for a bit of variety)
    ismEngine_RemoteServer_t *remoteServer = remoteServersGlobal->serverHead;
    while(remoteServer->next != NULL) remoteServer = remoteServer->next;
    while(remoteServer)
    {
        ismEngine_RemoteServer_t *prevRemoteServer = remoteServer->prev;

        TEST_ASSERT_PTR_NULL(remoteServer->clusterData);       // Check not set after recovery
        TEST_ASSERT_EQUAL(remoteServer->clusterDataLength, 0); // Check not set after recovery

        if ((remoteServer->internalAttrs & iersREMSRVATTR_LOCAL) == 0)
        {
            expectedRemoteServerCount -= 1;
        }

        expectedServerCount -= 1;

        rc = engineCallback(ENGINE_RS_REMOVE,
                            remoteServer,
                            0, NULL, NULL, NULL, 0, NULL, 0, false, false, NULL, NULL, engineCallbackContext, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, expectedRemoteServerCount);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, expectedServerCount);

        remoteServer = prevRemoteServer;
    }
}

void test_capability_RecoverMany_Phase3(void)
{
    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    TEST_ASSERT_EQUAL(restoreRemoteServers_callCount, 0);
    TEST_ASSERT_EQUAL(restoreRemoteServers_serverCount, 0);
    TEST_ASSERT_EQUAL(recoveryCompleted_callCount, 1);
    recoveryCompleted_callCount = 0;

    remoteServersGlobal = ismEngine_serverGlobal.remoteServers;
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);

#if 0
    remoteServer = remoteServersGlobal->serverHead;
    while(remoteServer)
    {
        printf("GUID:%s ATTRS:0x%08x DEFN:0x%lx PROPS:0x%lx LOW:%p HIGH:%p\n", remoteServer->serverGUID,
               remoteServer->internalAttrs,
               remoteServer->hStoreDefn, remoteServer->hStoreProps,
               remoteServer->lowQoSQueueHandle, remoteServer->highQoSQueueHandle);
        remoteServer = remoteServer->next;
    }
#endif
}

//****************************************************************************
/// @brief Test what happens when remote servers found when no longer part of
///        a cluster.
//****************************************************************************
#define TEST_NUM_TRANS 5
#define TEST_UNCLUSTERED_COUNT 50
void test_capability_RecoverUnClustered_Phase1(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    ismEngine_RemoteServerHandle_t createdHandle[TEST_UNCLUSTERED_COUNT] = {ismENGINE_NULL_REMOTESERVER_HANDLE};

    uint32_t expectedRemoteServerCount = 0;
    uint32_t expectedServerCount = 0;

    // Use cluster callback interface to create servers
    int64_t i=1;
    for(; i<=TEST_UNCLUSTERED_COUNT; i++)
    {
        char serverName[20];
        char serverUID[10];

        sprintf(serverName, "Server%ldof%lu", i, (uint64_t)TEST_UNCLUSTERED_COUNT);
        sprintf(serverUID, "UID%ld", i);

        rc = engineCallback(ENGINE_RS_CREATE,
                            ismENGINE_NULL_REMOTESERVER_HANDLE,
                            (ismCluster_RemoteServerHandle_t)NULL,
                            serverName,
                            serverUID,
                            NULL, 0,
                            NULL, 0,
                            false, false, NULL, NULL,
                            engineCallbackContext,
                            &createdHandle[i-1]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(createdHandle[i-1]);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverHead, createdHandle[i-1]);

        expectedRemoteServerCount += 1;
        expectedServerCount += 1;

        TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, expectedRemoteServerCount);
        TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, expectedServerCount);
    }

    // Create an initial client and session
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    rc = test_createClientAndSession("UNCLUS1",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient,
                                     &hSession,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create some likely looking transactions
    ismEngine_TransactionHandle_t hTran[TEST_NUM_TRANS];
    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    for(uint64_t t=0; t<TEST_NUM_TRANS; t++)
    {
        memset(&XID, 0, sizeof(ism_xid_t));
        XID.formatID = t < TEST_NUM_TRANS-1 ? ISM_FWD_XID : 0x12345678;
        XID.gtrid_length = sizeof(uint64_t);
        XID.bqual_length = sizeof(uint64_t);
        globalId.gtrid = 0x8877665544332211;
        globalId.bqual = t;
        memcpy(&XID.data, &globalId, sizeof(globalId));

        // Try resuming a non-existent transaction on Session1A
        rc = sync_ism_engine_createGlobalTransaction(hSession,
                                                     &XID,
                                                     ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                                     &hTran[t]);
        TEST_ASSERT_EQUAL(rc, OK);

        // Put a persistent message
        ismEngine_MessageHandle_t hMessage;
        void *pPayload=NULL;
        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, "BLAH",
                                &hMessage,&pPayload);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ieq_put(pThreadData,
                     createdHandle[t]->highQoSQueueHandle,
                     ieqPutOptions_THREAD_LOCAL_MESSAGE,
                     hTran[t],
                     hMessage,
                     IEQ_MSGTYPE_REFCOUNT);
        TEST_ASSERT_EQUAL(rc, OK);

        ism_engine_releaseMessage(hMessage);

        free(pPayload);

        if (t != 3) rc = sync_ism_engine_prepareTransaction(hSession, hTran[t]);
        TEST_ASSERT_EQUAL(rc, OK);

        if (t == 0)
            rc = sync_ism_engine_completeGlobalTransaction(&XID, ismTRANSACTION_COMPLETION_TYPE_COMMIT);
        else if (t == 1)
            rc = sync_ism_engine_completeGlobalTransaction(&XID, ismTRANSACTION_COMPLETION_TYPE_ROLLBACK);

        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = sync_ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_RecoverUnClustered_Phase2(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    // Check the transactions we expect to be there are still there
    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    for(uint64_t t=0; t<TEST_NUM_TRANS; t++)
    {
        memset(&XID, 0, sizeof(ism_xid_t));
        XID.formatID = t < TEST_NUM_TRANS-1 ? ISM_FWD_XID : 0x12345678;
        XID.gtrid_length = sizeof(uint64_t);
        XID.bqual_length = sizeof(uint64_t);
        globalId.gtrid = 0x8877665544332211;
        globalId.bqual = t;
        memcpy(&XID.data, &globalId, sizeof(globalId));

        ismEngine_Transaction_t *pTran = NULL;
        rc = ietr_findGlobalTransaction(pThreadData, &XID, &pTran);
        switch(t)
        {
            case 0:
            case 1:
            case 2:
            case TEST_NUM_TRANS-1:
                TEST_ASSERT_EQUAL(rc, OK);
                TEST_ASSERT_PTR_NOT_NULL(pTran);
                break;
            default:
                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
                break;
        }
    }

    TEST_ASSERT_EQUAL(restoreRemoteServers_callCount, 1);
    TEST_ASSERT_EQUAL(restoreRemoteServers_serverCount, TEST_UNCLUSTERED_COUNT);
    TEST_ASSERT_EQUAL(recoveryCompleted_callCount, 1);
}

void test_capability_RecoverUnClustered_Phase3(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    // Check the transactions we expect to be there are still there
    for(uint64_t t=0; t<TEST_NUM_TRANS; t++)
    {
        memset(&XID, 0, sizeof(ism_xid_t));
        XID.formatID = t < TEST_NUM_TRANS-1 ? ISM_FWD_XID : 0x12345678;
        XID.gtrid_length = sizeof(uint64_t);
        XID.bqual_length = sizeof(uint64_t);
        globalId.gtrid = 0x8877665544332211;
        globalId.bqual = t;
        memcpy(&XID.data, &globalId, sizeof(globalId));

        ismEngine_Transaction_t *pTran = NULL;
        rc = ietr_findGlobalTransaction(pThreadData, &XID, &pTran);
        switch(t)
        {
            case TEST_NUM_TRANS-1:
                TEST_ASSERT_EQUAL(rc, OK);
                TEST_ASSERT_PTR_NOT_NULL(pTran);
                break;
            case 0:
            case 1:
            case 2:
            default:
                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
                break;
        }
    }

    TEST_ASSERT_EQUAL(restoreRemoteServers_callCount, 0);
    TEST_ASSERT_EQUAL(restoreRemoteServers_serverCount, 0);
    TEST_ASSERT_EQUAL(recoveryCompleted_callCount, 0);

    ism_cluster_registerEngineEventCallback_RC = OK;
}

void test_capability_RecoverUnClustered_Phase4(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    iersRemoteServers_t *remoteServersGlobal = ismEngine_serverGlobal.remoteServers;

    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    TEST_ASSERT_EQUAL(restoreRemoteServers_callCount, 0);
    TEST_ASSERT_EQUAL(restoreRemoteServers_serverCount, 0);
    TEST_ASSERT_EQUAL(recoveryCompleted_callCount, 1);

    remoteServersGlobal = ismEngine_serverGlobal.remoteServers;
    TEST_ASSERT_EQUAL(remoteServersGlobal->remoteServerCount, 0);
    TEST_ASSERT_EQUAL(remoteServersGlobal->serverCount, 0);

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    rc = test_createClientAndSession("UNCLUS1",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient,
                                     &hSession,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0x12345678;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 0x8877665544332211;
    globalId.bqual = TEST_NUM_TRANS-1;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    rc = sync_ism_engine_commitGlobalTransaction(hSession, &XID, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_RecoverUnClustered_Phase5(void)
{
    int32_t rc;

    printf("Starting %s...\n", __func__);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    // Check the transactions we expect to be there are still there
    for(uint64_t t=0; t<TEST_NUM_TRANS; t++)
    {
        memset(&XID, 0, sizeof(ism_xid_t));
        XID.formatID = t < TEST_NUM_TRANS-1 ? ISM_FWD_XID : 0x12345678;
        XID.gtrid_length = sizeof(uint64_t);
        XID.bqual_length = sizeof(uint64_t);
        globalId.gtrid = 0x8877665544332211;
        globalId.bqual = t;
        memcpy(&XID.data, &globalId, sizeof(globalId));

        ismEngine_Transaction_t *pTran = NULL;
        rc = ietr_findGlobalTransaction(pThreadData, &XID, &pTran);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }

    check_ism_cluster_restoreRemoteServers = false;
    restoreRemoteServers_callCount = restoreRemoteServers_serverCount = 0;
    check_ism_cluster_recoveryCompleted = false;
    recoveryCompleted_callCount = 0;
}

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase1[] =
{
    { "RecoverMany", test_capability_RecoverMany_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase1b[] =
{
    { "RecoverMany", test_capability_RecoverMany_Phase1b },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase1c[] =
{
    { "RecoverMany", test_capability_RecoverMany_Phase1c },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase2[] =
{
    { "RecoverMany", test_capability_RecoverMany_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase3[] =
{
    { "RecoverMany", test_capability_RecoverMany_Phase3 },
    { "RecoverUnClustered", test_capability_RecoverUnClustered_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase4[] =
{
    { "RecoverUnClustered", test_capability_RecoverUnClustered_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase5[] =
{
    { "RecoverUnClustered", test_capability_RecoverUnClustered_Phase3 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase6[] =
{
    { "RecoverUnClustered", test_capability_RecoverUnClustered_Phase4 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase7[] =
{
    { "RecoverUnClustered", test_capability_RecoverUnClustered_Phase5 },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initRemoteServers(void)
{
    check_ism_cluster_addSubscriptions = true;
    addSubscriptions_callCount = addSubscriptions_topicCount = addSubscriptions_wildcardCount = 0;
    check_ism_cluster_removeSubscriptions = true;
    removeSubscriptions_callCount = removeSubscriptions_topicCount = removeSubscriptions_wildcardCount = 0;
    check_ism_cluster_restoreRemoteServers = true;
    restoreRemoteServers_callCount = restoreRemoteServers_serverCount = 0;
    check_ism_cluster_recoveryCompleted = true;
    recoveryCompleted_callCount = 0;

    return test_engineInit_DEFAULT;
}

int initRemoteServersWarm(void)
{
    check_ism_cluster_addSubscriptions = true;
    addSubscriptions_callCount = addSubscriptions_topicCount = addSubscriptions_wildcardCount = 0;
    check_ism_cluster_removeSubscriptions = true;
    removeSubscriptions_callCount = removeSubscriptions_topicCount = removeSubscriptions_wildcardCount = 0;
    check_ism_cluster_restoreRemoteServers = true;
    restoreRemoteServers_callCount = restoreRemoteServers_serverCount = 0;
    check_ism_cluster_recoveryCompleted = true;
    recoveryCompleted_callCount = 0;

    return test_engineInit_WARM;
}

int initRemoteServersWarmDisableCluster(void)
{
    ism_cluster_registerEngineEventCallback_RC = ISMRC_ClusterDisabled;

    return initRemoteServersWarm();
}

int initRemoteServersWarmFailRestoreRemoteServers(void)
{
    int rc;

    #if defined(NDEBUG)
    force_ism_cluster_restoreRemoteServers_error = true;
    swallow_ffdcs = true;
    rc = initRemoteServersWarm();
    TEST_ASSERT(rc == 999, ("Assert in %s line %d failed\n", __func__, __LINE__));
    swallow_ffdcs = false;
    force_ism_cluster_restoreRemoteServers_error = false;

    rc = 0;
    #else
    rc = initRemoteServersWarm();
    #endif

    return rc;
}

int initRemoteServersWarmFailRecoveryCompleted(void)
{
    int rc;

    #if defined(NDEBUG)
    force_ism_cluster_recoveryCompleted_error = true;
    swallow_ffdcs = true;
    rc = initRemoteServersWarm();
    TEST_ASSERT(rc == 999, ("Assert in %s line %d failed\n", __func__, __LINE__));
    swallow_ffdcs = false;
    force_ism_cluster_recoveryCompleted_error = false;

    rc = 0;
    #else
    rc = initRemoteServersWarm();
    #endif

    return rc;
}

int termRemoteServers(void)
{
    return test_engineTerm(true);
}

int termRemoteServersWarm(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_RemoteServers_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("BasicThings", initRemoteServers, termRemoteServers, ISM_RemoteServers_Cunit_test_capability_BasicThings),
    IMA_TEST_SUITE("BasicThings2", initRemoteServers, termRemoteServers, ISM_RemoteServers_Cunit_test_capability_BasicThings2),
    IMA_TEST_SUITE("HorridThings", initRemoteServers, termRemoteServers, ISM_RemoteServers_Cunit_test_capability_HorridThings),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase2suites[] =
{
    IMA_TEST_SUITE("ClusterRetained", initRemoteServers, termRemoteServersWarm, ISM_RemoteServers_Cunit_test_capability_ClusterRetained_Phase1),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase3suites[] =
{
    IMA_TEST_SUITE("ClusterRetained", initRemoteServersWarm, termRemoteServers, ISM_RemoteServers_Cunit_test_capability_ClusterRetained_Phase2),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase4suites[] =
{
    IMA_TEST_SUITE("Recovery", initRemoteServers, termRemoteServersWarm, ISM_RemoteServers_Cunit_test_capability_Recovery_Phase1),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase5suites[] =
{
    IMA_TEST_SUITE("Recovery", initRemoteServersWarm, termRemoteServersWarm, ISM_RemoteServers_Cunit_test_capability_Recovery_Phase2),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase6suites[] =
{
    IMA_TEST_SUITE("Recovery", initRemoteServersWarm, termRemoteServersWarm, ISM_RemoteServers_Cunit_test_capability_Recovery_Phase3),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase7suites[] =
{
    IMA_TEST_SUITE("Recovery", initRemoteServersWarm, termRemoteServersWarm, ISM_RemoteServers_Cunit_test_capability_Recovery_Phase4),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase8suites[] =
{
    IMA_TEST_SUITE("Recovery", initRemoteServersWarm, termRemoteServersWarm, ISM_RemoteServers_Cunit_test_capability_Recovery_Phase5),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase9suites[] =
{
    IMA_TEST_SUITE("Recovery", initRemoteServersWarm, termRemoteServers, ISM_RemoteServers_Cunit_test_capability_Recovery_Phase6),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase10suites[] =
{
    IMA_TEST_SUITE("Recovery2", initRemoteServers, termRemoteServersWarm, ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase1),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase11suites[] =
{
    IMA_TEST_SUITE("Recovery2", initRemoteServersWarmFailRestoreRemoteServers, termRemoteServersWarm, ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase1b),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase12suites[] =
{
    IMA_TEST_SUITE("Recovery2", initRemoteServersWarmFailRecoveryCompleted, termRemoteServersWarm, ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase1c),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase13suites[] =
{
    IMA_TEST_SUITE("Recovery2", initRemoteServersWarm, termRemoteServersWarm, ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase2),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase14suites[] =
{
    IMA_TEST_SUITE("Recovery2", initRemoteServersWarm, termRemoteServersWarm, ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase3),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase15suites[] =
{
    IMA_TEST_SUITE("Recovery2", initRemoteServersWarm, termRemoteServersWarm, ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase4),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase16suites[] =
{
    IMA_TEST_SUITE("Recovery2", initRemoteServersWarmDisableCluster, termRemoteServersWarm, ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase5),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase17suites[] =
{
    IMA_TEST_SUITE("Recovery2", initRemoteServersWarm, termRemoteServersWarm, ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase6),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_RemoteServers_CUnit_phase18suites[] =
{
    IMA_TEST_SUITE("Recovery2", initRemoteServersWarm, termRemoteServers, ISM_RemoteServers_Cunit_test_capability_Recovery2_Phase7),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo *PhaseSuites[] = { ISM_RemoteServers_CUnit_phase1suites
                              , ISM_RemoteServers_CUnit_phase2suites
                              , ISM_RemoteServers_CUnit_phase3suites
                              , ISM_RemoteServers_CUnit_phase4suites
                              , ISM_RemoteServers_CUnit_phase5suites
                              , ISM_RemoteServers_CUnit_phase6suites
                              , ISM_RemoteServers_CUnit_phase7suites
                              , ISM_RemoteServers_CUnit_phase8suites
                              , ISM_RemoteServers_CUnit_phase9suites
                              , ISM_RemoteServers_CUnit_phase10suites
                              , ISM_RemoteServers_CUnit_phase11suites
                              , ISM_RemoteServers_CUnit_phase12suites
                              , ISM_RemoteServers_CUnit_phase13suites
                              , ISM_RemoteServers_CUnit_phase14suites
                              , ISM_RemoteServers_CUnit_phase15suites
                              , ISM_RemoteServers_CUnit_phase16suites
                              , ISM_RemoteServers_CUnit_phase17suites
                              , ISM_RemoteServers_CUnit_phase18suites };

int main(int argc, char *argv[])
{
    // Force this test to run with clustering enabled
    setenv("IMA_TEST_ENABLE_CLUSTER","True",true);

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

    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
