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
/// @file  remoteServers.h
/// @brief Defines functions available to engine code outside of the
///        remote server code.
//****************************************************************************
#ifndef __ISM_REMOTESERVERS_DEFINED
#define __ISM_REMOTESERVERS_DEFINED

#include <stdint.h>

#include "ismutil.h"
#include "engineInternal.h"
#include "topicTree.h"        // iett functions and constants

#define iersREMSRVATTR_NONE                0x00000000  ///< No special attributes
#define iersREMSRVATTR_LOCAL               0x00000001  ///< This is a record for the local server
#define iersREMSRVATTR_DISCONNECTED        0x00000100  ///< The server is disconnected (not set == connected)
#define iersREMSRVATTR_ROUTE_ALL_MODE      0x00000200  ///< The server is in 'route all' mode (not set == not in mode)
#define iersREMSRVATTR_PENDING_UPDATE      0x00001000  ///< The server is part of a pending update batch
#define iersREMSRVATTR_CREATING            0x01000000  ///< Server has not yet completed creation processing
#define iersREMSRVATTR_DELETED             0x02000000  ///< Logically deleted servers have this set
#define iersREMSRVATTR_UNCLUSTERED         0x04000000  ///< Non-deleted servers that are rehydrated when cluster not enabled

#define iersREMSRVATTR_PERSISTENT_MASK     iersREMSRVATTR_LOCAL

/// @brief Memory limits currently in place
typedef enum __attribute__ ((__packed__)) tag_iersMemLimit_t
{
    NoLimit = 0,                  ///< No memory limits
    LowQoSLimit = 1,              ///< Reached the memory to limit low QoS queues
    HighQoSLimit = 2,             ///< Reached the memory to limit high QoS queues
    DiscardLocalNullRetained = 3  ///< Discard NullRetained messages that originated here
} iersMemLimit_t;

//****************************************************************************
/// @brief Initialize the remote server control information root
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_initEngineRemoteServers(ieutThreadData_t *pThreadData);

//****************************************************************************
/// @brief Destroy the remote server control information
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
void iers_destroyEngineRemoteServers(ieutThreadData_t *pThreadData);

//****************************************************************************
/// @brief Callback used by cluster component to handle events
///
/// @param[in]     eventType               Event type
/// @param[in]     hRemoteServer           Engine handle for the remote server [!CREATE]
/// @param[in]     hClusterHandle          Cluster component's handle for this server [CREATE]
/// @param[in]     pServerName             NULL terminated server Name [CREATE]
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
                                  ismEngine_RemoteServerHandle_t *phRemoteServerHandle);

//****************************************************************************
/// @brief Deregister the callback function and wait for any active callbacks
///        to complete.
//****************************************************************************
void iers_stopClusterEventCallbacks(ieutThreadData_t *pThreadData);

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
                                              ismEngine_Message_t *message);

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
void iers_acquireServerReference(ismEngine_RemoteServer_t *server);

//****************************************************************************
/// @brief  Release the remote server
///
/// Decrement the use count for the remote server freeing it if it drops to zero
///
/// @param[in]     pThreadData   Thread Data to use
/// @param[in]     remoteServer  Remote server to release
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_releaseServer(ieutThreadData_t *pThreadData,
                           ismEngine_RemoteServer_t * remoteServer);

//****************************************************************************
/// @brief Write the descriptions of remote server structures to the dump
///
/// @param[in]     dumpHdl  Pointer to a dump structure
//****************************************************************************
void iers_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl);

//****************************************************************************
/// @brief Dump the contents of a remote server
///
/// @param[in]     remoteServer  Remote server to dump
/// @param[in]     dumpHdl       Handle to the dump
//****************************************************************************
void iers_dumpServer(ieutThreadData_t *pThreadData,
                     ismEngine_RemoteServer_t *remoteServer,
                     iedmDumpHandle_t  dumpHdl);

//****************************************************************************
/// @brief Dump the remote servers list.
///
/// @param[in]     dumpHdl          Dump to write to
///
/// @returns OK or an ISMRC_ value on error.
//****************************************************************************
int32_t iers_dumpServersList(ieutThreadData_t *pThreadData,
                             iedmDumpHandle_t dumpHdl);

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
                                 void *pContext);

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
                                  void *pContext);

//****************************************************************************
/// @brief Reconcile the list of remote servers we have with the cluster
///        component (and tidy up anything that we can)
///
/// @param[in]     pThreadData       Thread data
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iers_reconcileEngineRemoteServers(ieutThreadData_t *pThreadData);

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
int32_t iers_declareRecoveryCompleted(ieutThreadData_t *pThreadData);

//****************************************************************************
/// @brief Register a consumer on a remote server queue
///
/// @param[in]     remoteServer     Remote server upon which consumer is registered
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
                           ismEngine_RemoteServer_t *remoteServer,
                           ismEngine_Consumer_t *consumer,
                           ismDestinationType_t destinationType);

//****************************************************************************
/// @brief Unregister a consumer previously registered with iers_registerConsumer
///
/// @param[in]     consumer         Consumer being unregistered
/// @param[in]     destinationType  Destination type specified for consumer
///
/// @see iett_registerConsumer
//****************************************************************************
void iers_unregisterConsumer(ieutThreadData_t *pThreadData,
                             ismEngine_Consumer_t *consumer,
                             ismDestinationType_t destinationType);

//****************************************************************************
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
iersMemLimit_t iers_queryRemoteServerMemLimit(ieutThreadData_t *pThreadData);

//****************************************************************************
/// @brief Reconcile retained message information from the cluster and the
///        local server and decide whether to request retained messages from
///        another server.
///
/// @return OK on successful completion or an ISMRC_ value.
///****************************************************************************
int32_t iers_syncClusterRetained(ieutThreadData_t *pThreadData);

//****************************************************************************
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
                              ism_time_t *outOfSyncTime);

#endif /* __ISM_REMOTESERVERS_DEFINED */

/*********************************************************************/
/* End of remoteServers.h                                             */
/*********************************************************************/
