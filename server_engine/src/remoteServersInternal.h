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
/// @file  remoteServersInternal.h
/// @brief Defines functions and structures internal to the remote server code.
//****************************************************************************
#ifndef __ISM_REMOTESERVERSINTERNAL_DEFINED
#define __ISM_REMOTESERVERSINTERNAL_DEFINED

#include <stdint.h>

#include "engineInternal.h"
#include "policyInfo.h"
#include "topicTree.h"
#include "topicTreeInternal.h"
#include "engineHashTable.h"
#include "remoteServers.h"

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Structure used to maintain control information on known remote servers
///  @remark
///    This structure is used as the anchor for a list of known remote servers.
///////////////////////////////////////////////////////////////////////////////
typedef struct tag_iersRemoteServers_t
{
    char                      strucId[4];               ///< Structure identifier 'ERSR'
    ismEngine_RemoteServer_t *serverHead;               ///< Head of the list of servers
    uint32_t                  serverCount;              ///< Count of server entries in the list
    uint32_t                  remoteServerCount;        ///< Count of remote server entries in the list (excluding local)
    pthread_rwlock_t          listLock;                 ///< Lock on the list of remote servers
    iepiPolicyInfoHandle_t    lowQoSPolicyHandle;       ///< Messaging policy for low QoS queues
    iepiPolicyInfoHandle_t    highQoSPolicyHandle;      ///< Messaging policy for high QoS queues
    iepiPolicyInfoHandle_t    seedingPolicyHandle;      ///< Messaging policy for remote servers being seeded
    uint64_t                  reservedForwardingBytes;  ///< Reserved amount of total memory for remote server forwarding queues
    uint32_t                  seedingCount;             ///< Count of remote servers which are currently having retained messsages seeded
    iersMemLimit_t            currentMemLimit;          ///< What the current memory limit is
    ismCluster_HealthStatus_t currentHealthStatus;      ///< What current health status was reported to cluster
    bool                      syncCheckInProgress;      ///< Whether we there is an out-of-sync check currently in progress
    ieutHashTable_t          *outOfSyncServers;         ///< Hash of times at which we went out of sync with servers
    pthread_spinlock_t        outOfSyncLock;            ///< Lock protecting the out-of-sync hash
} iersRemoteServers_t;

#define iersREMOTESERVERS_STRUCID "ERSR"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iersRemoteServers_t(__file)\
    iedm_descriptionStart(__file, iersRemoteServers_t, strucId, iersREMOTESERVERS_STRUCID);\
    iedm_describeMember(char [4],                   strucId);\
    iedm_describeMember(ismEngine_RemoteServer_t *, serverHead);\
    iedm_describeMember(uint32_t,                   serverCount);\
    iedm_describeMember(uint32_t,                   remoteServerCount);\
    iedm_describeMember(pthread_rwlock_t,           listLock);\
    iedm_describeMember(iepiPolicyInfo_t *,         lowQoSPolicyHandle);\
    iedm_describeMember(iepiPolicyInfo_t *,         highQoSPolicyHandle);\
    iedm_describeMember(iepiPolicyInfo_t *,         seedingPolicyHandle);\
    iedm_describeMember(uint64_t,                   reservedForwardingBytes);\
    iedm_describeMember(uint32_t,                   seedingCount);\
    iedm_describeMember(iersMemLimit_t,             currentMemLimit);\
    iedm_describeMember(ismCluster_HealthStatus_t,  currentHealthStatus);\
    iedm_describeMember(bool,                       syncCheckInProgress);\
    iedm_describeMember(ieutHashTable_t *,          outOfSyncServers);\
    iedm_describeMember(pthread_spinlock_t,         outOfSyncLock);\
    iedm_descriptionEnd;

///////////////////////////////////////////////////////////////////////////////
/// @brief
///   Values used when evaluating the local and cluster versions of the sync info
///////////////////////////////////////////////////////////////////////////////
typedef enum tag_iersSyncLevel_t
{
    Unevaluated         = 0x00000000,    // Not yet evaluated
    Synchronized        = 0x00000001,    // Local and Cluster information match
    LocalHTSHigher      = 0x00000010,    // Local has a higher highestTimestampSeen value
    ClusterHTSHigher    = 0x00000020,    // Cluster has a higher highestTimestampSeen value
    LocalHTAHigher      = 0x00000100,    // Local has a higher highestTimestampAvailable value
    ClusterHTAHigher    = 0x00000200,    // Cluster has a higher higherstTimestampAvailable value
    CountMismatch       = 0x00000400,    // Count values don't match
    TopicsMismatch      = 0x00000800,    // Topics identifier doesn't match
    NoneConnected       = 0x00001000,    // No remote servers containing info are connected
    KnownServer         = 0x01000000,    // This is a server that this server knows about
    ConnectedServer     = 0x02000000,    // This server is currently connected
} iersSyncLevel_t;


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Structure used to identify out-of-sync retained messages
///////////////////////////////////////////////////////////////////////////////
#define MAX_BEST_SERVERS 5
typedef struct tag_iersServerSyncInfo_t
{
    ismEngine_RemoteServer_t *server;                        ///< Remote server pointer (non-null for known Servers)
    char *serverUID;                                         ///< Remote server UID
    ismEngine_RemoteServer_t *bestServer[MAX_BEST_SERVERS];  ///< Most up-to-date servers
    iettOriginServerStats_t bestStats[MAX_BEST_SERVERS];     ///< Stats from the most up-to-date servers
    iettOriginServerStats_t localStats;                      ///< Local stats for this server
    iersSyncLevel_t syncLevel;                               ///< The level of synchronization for this server
    struct tag_iersServerSyncInfo_t *next;                   ///< Next server in the list (only on allServers)
} iersServerSyncInfo_t;

typedef struct tag_iersClusterSyncInfo_t
{
    ieutHashTable_t *knownServers;        ///< Servers that this we are in contact with
    ieutHashTable_t *allServers;          ///< All servers (including known ones) for which we have clustered stats
    iersServerSyncInfo_t *firstAllServer; ///< The first of the allServers records
    int32_t rc;                           ///< Return code observed during processing of this information
    bool inSync;                          ///< Whether we believe everything is in-sync or not
} iersClusterSyncInfo_t;

// Percentage of free memory at which we tell cluster component health is YELLOW
// (So if memory is >57% we will report YELLOW health status)
#define iersFREEMEM_LIMIT_CLUSTER_HEALTHSTATUS_YELLOW 43

// Percentage of free memory at which we tell cluster component health is RED
// (So if memory is >77% we will report RED health status)
#define iersFREEMEM_LIMIT_CLUSTER_HEALTHSATUS_RED 23

// Percentage of free memory at which we start imposing limits on low QoS queues
// (So if memory is >78% we will impose limits on low QoS queues)
#define iersFREEMEM_LIMIT_LOW_QOS_FWD_THRESHOLD 22

// Percentage of free memory at which we start imposing limits on high QoS queues
// (So if memory is >82% we will impose limits on high QoS queues)
#define iersFREEMEM_LIMIT_HIGH_QOS_FWD_THRESHOLD 18

// Percentage of free memory at which we throw away unexpired local NullRetained
// (So if memory is >85% we will throw away unexpired NullRetained)
#define iersFREEMEM_LIMIT_DISCARD_LOCAL_NULLRETAINED 15

// Percentage of total memory to reserve for the use of remote server forwarding
// queues when imposing limits.
#define iersRESERVE_FORWARDING_BYTES_PERCENT 10

// Analyse memory usage reporting to the cluster component and and imposing limits
// on forwarding queues as appropriate
void iers_analyseMemoryUsage(iememMemoryLevel_t currState,
                             iememMemoryLevel_t prevState,
                             iemem_memoryType type,
                             uint64_t currentLevel,
                             iemem_systemMemInfo_t *memInfo);
#endif /* __ISM_REMOTESERVERSINTERNAL_DEFINED */

/*********************************************************************/
/* End of remoteServersInternal.h                                     */
/*********************************************************************/
