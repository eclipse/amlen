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

//*********************************************************************
/// @file  engine.c
/// @brief Main module for Engine Component of ISM
//*********************************************************************
#define TRACE_COMP Engine

#include <stdio.h>
#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "engineStore.h"
#include "engineRestore.h"
#include "engineDump.h"
#include "clientState.h"
#include "destination.h"
#include "topicTree.h"
#include "remoteServers.h"      // iers functions & constants
#include "simpQ.h"
#include "msgCommon.h"
#include "memHandler.h"
#include "transaction.h"
#include "lockManager.h"
#include "queueCommon.h"
#include "queueNamespace.h"     // ieqn functions & constants
#include "ackList.h"
#include "selector.h"
#include "engineTimers.h"
#include "waiterStatus.h"
#include "messageExpiry.h"      // ieme functions & constants
#include "clientStateExpiry.h"  // iece functions & constants
#include "engineAsync.h"
#include "mempool.h"
#include "engineUtils.h"
#include "threadJobs.h"
#include "resourceSetStats.h"

#ifdef USEFAKE_ASYNC_COMMIT
#include "fakeAsyncStore.h"
#endif

// Keep the time at which we set some engine phases so we can report overall elapsed time.
double enginePhaseInitializingTime = 0;
double enginePhaseStartingTime = 0;

/*********************************************************************/
/* Structures just used in this file                                 */
/*********************************************************************/

/// @brief For Rescheduling a destroy of a client back to initiating thread
typedef struct tag_iecsDestroyClientAsyncData_t
{
    char                       StructId[4];
    uint64_t                   asyncId;
    ieutThreadData_t          *pJobThread;
    ismEngine_ClientState_t   *pClient;
} iecsDestroyClientAsyncData_t;

#define IECS_DESTROYCLIENTASYNCDATA_STRUCID "ICAD"

/*********************************************************************/
/*                                                                   */
/* INTERNAL FUNCTION PROTOTYPES                                      */
/*                                                                   */
/*********************************************************************/

// Internal function to destroy a session as part of recursive clean-up
static inline int32_t destroySessionInternal(ieutThreadData_t *pThreadData,
                                             ismEngine_Session_t *pSession);

// Internal function to destroy a session
static inline int32_t destroySessionInternalNoRelease(ieutThreadData_t *pThreadData,
                                                      ismEngine_Session_t *pSession);

// Get the next consumer to enable
static inline int32_t getNextConsumerForDelivery(ieutThreadData_t *pThreadData,
                                                 ismEngine_Session_t *pSession,
                                                 ismEngine_Consumer_t **ppConsumer);

// Internal function to stop message delivery as part of recursive clean-up.
// All of the session's consumers are assumed to have been destroyed.
static inline void completeStopMessageDeliveryInternal(ieutThreadData_t *pThreadData,
                                                       ismEngine_Session_t *pSession,
                                                       bool fInline);

// Internal function to destroy a producer as part of recursive clean-up
static inline void destroyProducerInternal(ieutThreadData_t *pThreadData,
                                           ismEngine_Producer_t *pProducer);

// Release a reference to a producer, deallocating the producer when
// the use-count drops to zero
static inline bool releaseProducerReference(ieutThreadData_t *pThreadData,
                                            ismEngine_Producer_t *pProducer,
                                            bool fInline);

// Internal function to destroy a consumer as part of recursive clean-up
static inline void destroyConsumerInternal(ieutThreadData_t *pThreadData,
                                           ismEngine_Consumer_t *pConsumer);

/*********************************************************************/
/*                                                                   */
/* FUNCTION DEFINITIONS                                              */
/*                                                                   */
/*********************************************************************/

/*********************************************************************/
/* Locks a session to enable synchronisation of access to its state. */
/*********************************************************************/
int32_t ism_engine_lockSession(ismEngine_Session_t *pSession)
{
    int32_t rc = OK;

    int osrc = pthread_mutex_lock(&pSession->Mutex);
    if (UNLIKELY(osrc != 0))
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);
        ieutTRACE_FFDC(ieutPROBE_001, true,
                       "pthread_mutex_lock failed", rc,
                       "osrc", &osrc, sizeof(osrc),
                       NULL);
    }

    return rc;
}


/*********************************************************************/
/* Unlocks a locked session at the end of a request.                 */
/*********************************************************************/
void ism_engine_unlockSession(ismEngine_Session_t *pSession)
{
    (void)pthread_mutex_unlock(&pSession->Mutex);
}


//****************************************************************************
/// @internal
///
/// @brief  Engine component initialization
///
/// Performs Engine initialization tasks, such as reading configuration
/// parameters.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_init(void)
{
    int32_t rc = OK;

    TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Indicate that we are initializing
    ieut_setEngineRunPhase(EnginePhaseInitializing);
    enginePhaseInitializingTime = ism_common_readTSC();

    // Use 'now' as the last shutdown time until we've read the real value.
    ismEngine_serverGlobal.ServerShutdownTimestamp = ism_common_nowExpire();

    // Assert some basic things we expect to be true...
    assert(sizeof(ismMessageState_t) == 1);

    // Make sure we have basic initialization on this thread
    rc = ieut_createBasicThreadData();

    if (rc != OK) goto mod_exit;

    // Initialize the global information for the memHandler
    iemem_initMemHandler();

    // Initialize the engine-wide deferred frees list.
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    ismEngine_serverGlobal.deferredFrees = iemem_calloc(pThreadData,
                                                        IEMEM_PROBE(iemem_deferredFreeLists, 1),
                                                        1, sizeof(ieutDeferredFreeList_t));

    if (ismEngine_serverGlobal.deferredFrees == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    rc = ieut_initDeferredFreeList(pThreadData, ismEngine_serverGlobal.deferredFrees);

    if (rc != OK) goto mod_exit;

mod_exit:

    TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief Callback registered with the config component
///
/// @param[in]  objectType        Configuration object
/// @param[in]  objectIdentifier  Name or Index of the configuration object
/// @param[in]  changedProps      A properties object (contains relevant properties)
/// @param[in]  changeType        Change type - refer to ism_ConfigChangeType_t
///
/// @remark For Subscription deletion, the call may need to go asynchronous, meaning
///         that the subscription is pending deletion, and will be deleted as soon
///         as possible, in this case ISMRC_AsyncCompletion is returned.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int ism_engine_configCallback(char *objectType,
                              char *objectIdentifier,
                              ism_prop_t *changedProps,
                              ism_ConfigChangeType_t changeType)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    bool reportError = true;
    int rc = OK;

    assert(objectType != NULL);

    ieutTRACEL(pThreadData, changeType, ENGINE_CEI_TRACE,
               FUNCTION_ENTRY "objectType='%s' objectIdentifier='%s' changeType=%d\n", __func__,
               objectType, objectIdentifier ? objectIdentifier : "<NONE>", (int32_t)changeType);

    ieutConfigCallbackType_t oldConfigCallback = pThreadData->inConfigCallback;

    // Admin queue changes are handled by queueNamespace
    if (objectType[0] == ismENGINE_ADMIN_VALUE_QUEUE[0] &&
        strcmp(objectType, ismENGINE_ADMIN_VALUE_QUEUE) == 0)
    {
        pThreadData->inConfigCallback = QueueConfigCallback;
        rc = ieqn_queueConfigCallback(pThreadData, objectIdentifier, changedProps, changeType);
    }
    // Admin topic monitors are handled by topicTree
    else if (objectType[0] == ismENGINE_ADMIN_VALUE_TOPICMONITOR[0] &&
             strcmp(objectType, ismENGINE_ADMIN_VALUE_TOPICMONITOR) == 0)
    {
        pThreadData->inConfigCallback = TopicMonitorConfigCallback;
        rc = iett_topicMonitorConfigCallback(pThreadData, changedProps, changeType);
    }
    // Cluster requested topics are handled by topicTree
    else if (objectType[0] == ismENGINE_ADMIN_VALUE_CLUSTERREQUESTEDTOPICS[0] &&
             strcmp(objectType, ismENGINE_ADMIN_VALUE_CLUSTERREQUESTEDTOPICS) == 0)
    {
        pThreadData->inConfigCallback = ClusterRequestedTopicsConfigCallback;
        rc = iett_clusterRequestedTopicsConfigCallback(pThreadData, changedProps, changeType);
    }
    // Admin MQTT clients are handled by client-states
    else if (objectType[0] == ismENGINE_ADMIN_VALUE_MQTTCLIENT[0] &&
             strcmp(objectType, ismENGINE_ADMIN_VALUE_MQTTCLIENT) == 0)
    {
        pThreadData->inConfigCallback = ClientStateConfigCallback;
        rc = iecs_clientStateConfigCallback(pThreadData, objectIdentifier, changedProps, changeType);
    }
    // Subscriptions (of all types) are handled by the topic tree
    else if ((objectType[0] == ismENGINE_ADMIN_VALUE_SUBSCRIPTION[0] &&
              strcmp(objectType, ismENGINE_ADMIN_VALUE_SUBSCRIPTION) == 0) ||
             (objectType[0] == ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION[0] &&
              strcmp(objectType, ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION) == 0) ||
             (objectType[0] == ismENGINE_ADMIN_VALUE_DURABLENAMESPACEADMINSUB[0] &&
              strcmp(objectType, ismENGINE_ADMIN_VALUE_DURABLENAMESPACEADMINSUB) == 0) ||
             (objectType[0] == ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB[0] &&
              strcmp(objectType, ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB) == 0))
    {
        pThreadData->inConfigCallback = SubscriptionConfigCallback;
        rc = iett_subscriptionConfigCallback(pThreadData, objectIdentifier, changedProps, changeType, objectType);
        reportError = false; // Called function reports any errors
    }
    // Policies are handled by the policy info code
    else
    {
        int32_t index = 0;

        for(; iepiPolicyTypeAdminInfo[index].type != ismSEC_POLICY_LAST; index++)
        {
            if (objectType[0] == iepiPolicyTypeAdminInfo[index].name[0] &&
                strcmp(objectType, iepiPolicyTypeAdminInfo[index].name) == 0)
            {
                pThreadData->inConfigCallback = PolicyConfigCallback;
                rc = iepi_policyInfoConfigCallback(pThreadData, objectType, index, objectIdentifier, changedProps, changeType);
                break;
            }
        }

        // Didn't expect to be called for this objectType -- ignore it, but write a trace line.
        if (iepiPolicyTypeAdminInfo[index].type == ismSEC_POLICY_LAST)
        {
            ieutTRACEL(pThreadData, changeType, ENGINE_WORRYING_TRACE,
                       FUNCTION_IDENT "Ignoring config callback for objectType '%s'. objectIdentifier='%s', changeType=%d.\n",
                       __func__, objectType, objectIdentifier ? objectIdentifier : "NULL", changeType);
        }
    }

    pThreadData->inConfigCallback = oldConfigCallback;

    // Make sure any errors are reported to other components.
    if (rc != OK && reportError) ism_common_setError(rc);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Engine component start
///
/// Performs restart recovery and reconciliation with the server configuration.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_start(void)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, 0, ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Indicate that the engine is starting
    ismEngine_serverGlobal.runPhase = EnginePhaseStarting;
    enginePhaseStartingTime = ism_common_readTSC();

    // Yes, the logically boolean value 'DisableAuthorization' is in fact defined as a string
    // which, when non-empty means 'disable authorization & authority checks' and when empty
    // or non-existent means 'enable authorization & authority checks'...
    const char *sprop = ism_common_getStringConfig("DisableAuthorization");

    // Did we receive a non-empty string? If so disable by using non security component functions
    if ((NULL != sprop) && (*sprop != '\0'))
    {
        ismEngine_security_validate_policy_func = iepi_DA_security_validate_policy;
        ismEngine_security_set_policyContext_func = iepi_DA_security_set_policyContext;

        ieutTRACEL(pThreadData, 0, ENGINE_INTERESTING_TRACE,
                   "Authority checks disabled by DisableAuthorization='%s'\n", sprop);
    }
    else
    {
        ismEngine_security_validate_policy_func = ism_security_validate_policy;
        ismEngine_security_set_policyContext_func = ism_security_set_policyContext;
    }

    // Initialize the value of the subList cache capacity (0 means no cache)
    ismEngine_serverGlobal.subListCacheCapacity = ism_common_getIntConfig(ismENGINE_CFGPROP_INITIAL_SUBLISTCACHE_CAPACITY,
                                                                          ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY);

    if (ismEngine_serverGlobal.subListCacheCapacity == 0)
    {
        ieutTRACEL(pThreadData, 0, ENGINE_INTERESTING_TRACE, "Subscriber list cache disabled\n");
    }
    else
    {
        if (ismEngine_serverGlobal.subListCacheCapacity != ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY)
        {
            ieutTRACEL(pThreadData, ismEngine_serverGlobal.subListCacheCapacity, ENGINE_INTERESTING_TRACE,
                       "Subscriber list cache capacity %u\n", ismEngine_serverGlobal.subListCacheCapacity);
        }
    }

    // Find out whether we have a custom limit of the number of unacked MQTT messages allowed per client...
    ismEngine_serverGlobal.mqttMsgIdRange = ism_common_getIntConfig( ismENGINE_CFGPROP_MAX_MQTT_UNACKED_MESSAGES
                                                                   , ismENGINE_DEFAULT_MAX_MQTT_UNACKED_MESSAGES);

    if (ismEngine_serverGlobal.mqttMsgIdRange != ismENGINE_DEFAULT_MAX_MQTT_UNACKED_MESSAGES)
    {
        ieutTRACEL(pThreadData, ismEngine_serverGlobal.mqttMsgIdRange, ENGINE_INTERESTING_TRACE,
                   "Maximum unacked messages per MQTT client: %u\n", ismEngine_serverGlobal.mqttMsgIdRange);
    }

    // Find out whether, on a small machine we should ignore (i.e. reserve) some free memory for use by e.g. OS
    ismEngine_serverGlobal.freeMemReservedMB = ism_common_getIntConfig( ismENGINE_CFGPROP_FREE_MEM_RESERVED_MB
                                                                      , ismENGINE_DEFAULT_FREE_MEM_RESERVED_MB);

    if (ismEngine_serverGlobal.freeMemReservedMB != ismENGINE_DEFAULT_FREE_MEM_RESERVED_MB)
    {
        ieutTRACEL(pThreadData, ismEngine_serverGlobal.freeMemReservedMB, ENGINE_INTERESTING_TRACE,
                   "Free mem reserved for non-" IMA_PRODUCTNAME_SHORT " use: %lu\n", ismEngine_serverGlobal.freeMemReservedMB);
    }

    // Find out how little memory a machine must have in order to be classed a small machine from the point of view of the freeMemReservedMB
    // param. (On a small machine we need to save some memory for the OS, on a larger machine the 15% where we try to prevent messaging should
    // be enough
    ismEngine_serverGlobal.freeMemRsvdThresholdMB = ism_common_getIntConfig( ismENGINE_CFGPROP_FREE_MEM_RESERVED_THRESHOLD_MB
                                                                           , ismENGINE_DEFAULT_FREE_MEM_RESERVED_THRESHOLD_MB);

    if (ismEngine_serverGlobal.freeMemRsvdThresholdMB != ismENGINE_DEFAULT_FREE_MEM_RESERVED_THRESHOLD_MB)
    {
        ieutTRACEL(pThreadData, ismEngine_serverGlobal.freeMemRsvdThresholdMB, ENGINE_INTERESTING_TRACE,
                   "Memory threshold that determines whether we need (on this machine) to reserve extra message for OS %lu\n", ismEngine_serverGlobal.freeMemRsvdThresholdMB);
    }

    //Find out whether we have a custom limit on the number of messages to give to a consumer before we round-robin to the
    //next consumer...
    ismEngine_serverGlobal.multiConsumerBatchSize = ism_common_getIntConfig(ismENGINE_CFGPROP_MULTICONSUMER_MESSAGE_BATCH
                                                                           ,ismENGINE_DEFAULT_MULTICONSUMER_MESSAGE_BATCH);
    if (ismEngine_serverGlobal.multiConsumerBatchSize != ismENGINE_DEFAULT_MULTICONSUMER_MESSAGE_BATCH)
    {
        ieutTRACEL(pThreadData, ismEngine_serverGlobal.multiConsumerBatchSize, ENGINE_INTERESTING_TRACE,
                   "Multiconsumer message batch size: %u\n", ismEngine_serverGlobal.multiConsumerBatchSize);
    }

    // Initialize whether automatic queue creation is disabled
    ismEngine_serverGlobal.disableAutoQueueCreation = ism_common_getBooleanConfig(ismENGINE_CFGPROP_DISABLE_AUTO_QUEUE_CREATION,
                                                                                  ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION);

    if (!ismEngine_serverGlobal.disableAutoQueueCreation)
    {
        ieutTRACEL(pThreadData, 0, ENGINE_INTERESTING_TRACE, "Automatic named queue creation is enabled\n");
    }

    // Initialise whether queues should use spin locks (or mutexes)
    ismEngine_serverGlobal.useSpinLocks = ism_common_getBooleanConfig(ismENGINE_CFGPROP_USE_SPIN_LOCKS,
                                                                      ismENGINE_DEFAULT_USE_SPIN_LOCKS);

    ieutTRACEL(pThreadData, 0, ENGINE_INTERESTING_TRACE, "Using Queue Spin Locks: %d\n", (int)ismEngine_serverGlobal.useSpinLocks);

    // Initialise the server timestamp interval - how frequently to update the SCR's timestamp
    int32_t iprop = ism_common_getIntConfig(ismENGINE_CFGPROP_SERVER_TIMESTAMP_INTERVAL,
                                            ismENGINE_DEFAULT_SERVER_TIMESTAMP_INTERVAL);
    if (iprop <= 0)
    {
        ismEngine_serverGlobal.ServerTimestampInterval = 0;
        ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "Server timestamp updates disabled\n");
    }
    else
    {
        ismEngine_serverGlobal.ServerTimestampInterval = (uint32_t)iprop;
        if (ismEngine_serverGlobal.ServerTimestampInterval != ismENGINE_DEFAULT_SERVER_TIMESTAMP_INTERVAL)
        {
            ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "Server timestamp interval: %u\n", ismEngine_serverGlobal.ServerTimestampInterval);
        }
    }

    // Initialise the ratio between queue max depth and orderId gap that causes slow ackers to be removed
    iprop = ism_common_getIntConfig(ismENGINE_CFGPROP_DESTROY_NONACKER_RATIO,
                                    ismENGINE_DEFAULT_DESTROY_NONACKER_RATIO);
    if (iprop <= 0)
    {
        ismEngine_serverGlobal.DestroyNonAckerRatio = 0;
        ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "DestroyNonAckerRatio = 0 (i.e. disabled)\n");
    }
    else
    {
        ismEngine_serverGlobal.DestroyNonAckerRatio = (uint32_t)iprop;
        if (ismEngine_serverGlobal.DestroyNonAckerRatio != ismENGINE_DEFAULT_DESTROY_NONACKER_RATIO)
        {
            ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "DestroyNonAckerRatio: %u\n", ismEngine_serverGlobal.DestroyNonAckerRatio);
        }
    }

    // Initialise the retained min active orderId scan interval
    iprop = ism_common_getIntConfig(ismENGINE_CFGPROP_RETAINED_MIN_ACTIVE_ORDERID_INTERVAL,
                                    ismENGINE_DEFAULT_RETAINED_MIN_ACTIVE_ORDERID_INTERVAL);
    if (iprop <= 0)
    {
        ismEngine_serverGlobal.RetMinActiveOrderIdInterval = 0;
        ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "Retained minimum active orderId scans disabled\n");
    }
    else
    {
        ismEngine_serverGlobal.RetMinActiveOrderIdInterval = (uint32_t)iprop;
        if (ismEngine_serverGlobal.RetMinActiveOrderIdInterval != ismENGINE_DEFAULT_RETAINED_MIN_ACTIVE_ORDERID_INTERVAL)
        {
            ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "Retained minimum active orderId scan interval: %u\n", ismEngine_serverGlobal.RetMinActiveOrderIdInterval);
        }
    }

    // Initialise the retained min active orderId scan max scans
    iprop = ism_common_getIntConfig(ismENGINE_CFGPROP_RETAINED_MIN_ACTIVE_ORDERID_MAX_SCANS,
                                    ismENGINE_DEFAULT_RETAINED_MIN_ACTIVE_ORDERID_MAX_SCANS);
    if (iprop <= 0)
    {
        ismEngine_serverGlobal.RetMinActiveOrderIdMaxScans = ismENGINE_DEFAULT_RETAINED_MIN_ACTIVE_ORDERID_MAX_SCANS;
    }
    else
    {
        ismEngine_serverGlobal.RetMinActiveOrderIdMaxScans = (uint32_t)iprop;
        if (ismEngine_serverGlobal.RetMinActiveOrderIdMaxScans != ismENGINE_DEFAULT_RETAINED_MIN_ACTIVE_ORDERID_MAX_SCANS)
        {
            ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "Retained minimum active orderId max scans: %u\n", ismEngine_serverGlobal.RetMinActiveOrderIdMaxScans);
        }
    }

    // Initialize whether to allow the repositioning of retained messages after a minimum active orderId scan
    ismEngine_serverGlobal.retainedRepositioningEnabled = ism_common_getBooleanConfig(ismENGINE_CFGPROP_RETAINED_REPOSITIONING_ENABLED,
                                                                                      ismENGINE_DEFAULT_RETAINED_REPOSITIONING_ENABLED);

    if (ismEngine_serverGlobal.retainedRepositioningEnabled != ismENGINE_DEFAULT_RETAINED_REPOSITIONING_ENABLED)
    {
        ieutTRACEL(pThreadData, ismEngine_serverGlobal.retainedRepositioningEnabled, ENGINE_INTERESTING_TRACE, "Retained Repositioning set to non-default value %s\n",
                ismEngine_serverGlobal.retainedRepositioningEnabled ? "true" : "false");
    }

    // Initialise the cluster retained synchronization check interval
    iprop = ism_common_getIntConfig(ismENGINE_CFGPROP_CLUSTER_RETAINED_SYNC_INTERVAL,
                                    ismENGINE_DEFAULT_CLUSTER_RETAINED_SYNC_INTERVAL);
    if (iprop <= 0)
    {
        ismEngine_serverGlobal.ClusterRetainedSyncInterval = 0;
        ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "Cluster retained sync checking disabled\n");
    }
    else
    {
        ismEngine_serverGlobal.ClusterRetainedSyncInterval = (uint32_t)iprop;
        if (ismEngine_serverGlobal.ClusterRetainedSyncInterval != ismENGINE_DEFAULT_CLUSTER_RETAINED_SYNC_INTERVAL)
        {
            ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "Cluster retained sync check interval: %u\n", ismEngine_serverGlobal.ClusterRetainedSyncInterval);
        }
    }

    // Initialise the cluster retained forwarding delay
    iprop = ism_common_getIntConfig(ismENGINE_CFGPROP_CLUSTER_RETAINED_FORWARDING_DELAY,
                                    ismENGINE_DEFAULT_CLUSTER_RETAINED_FORWARDING_DELAY);
    if (iprop <= 0)
    {
        ismEngine_serverGlobal.retainedForwardingDelay = ismENGINE_DEFAULT_CLUSTER_RETAINED_FORWARDING_DELAY;
        ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "Cluster retained forwarding delay defaulting to %u\n", ismEngine_serverGlobal.retainedForwardingDelay);
    }
    else
    {
        ismEngine_serverGlobal.retainedForwardingDelay = (uint32_t)iprop;
        if (ismEngine_serverGlobal.retainedForwardingDelay != ismENGINE_DEFAULT_CLUSTER_RETAINED_FORWARDING_DELAY)
        {
            ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "Cluster retained forwarding delay : %u\n", ismEngine_serverGlobal.retainedForwardingDelay);
        }
    }

    // Initialise the Async Callback Queue Alert maximum pause (0 means no maximum, pause forever)
    iprop = ism_common_getIntConfig(ismENGINE_CFGPROP_ASYNC_CALLBACK_QUEUE_ALERT_MAX_PAUSE,
                                    ismENGINE_DEFAULT_ASYNC_CALLBACK_QUEUE_ALERT_MAX_PAUSE);
    if (iprop < 0)
    {
        ismEngine_serverGlobal.AsyncCBQAlertMaxPause = ismENGINE_DEFAULT_ASYNC_CALLBACK_QUEUE_ALERT_MAX_PAUSE;
    }
    else
    {
        ismEngine_serverGlobal.AsyncCBQAlertMaxPause = (uint32_t)iprop;
        if (ismEngine_serverGlobal.AsyncCBQAlertMaxPause != ismENGINE_DEFAULT_ASYNC_CALLBACK_QUEUE_ALERT_MAX_PAUSE)
        {
            ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "Async callback queue alert max pause : %u\n", ismEngine_serverGlobal.AsyncCBQAlertMaxPause);
        }
    }

    // Decide whether to disable the threadJobs code
    bool disableThreadJobs = ism_common_getBooleanConfig(ismENGINE_CFGPROP_DISABLE_THREAD_JOB_QUEUES,
                                                         ismENGINE_DEFAULT_DISABLE_THREAD_JOB_QUEUES);

    if (disableThreadJobs != ismENGINE_DEFAULT_DISABLE_THREAD_JOB_QUEUES)
    {
        ieutTRACEL(pThreadData, disableThreadJobs, ENGINE_INTERESTING_TRACE,
                   "Non default %s setting (%d, not %d)\n",
                   ismENGINE_CFGPROP_DISABLE_THREAD_JOB_QUEUES,
                   (int)disableThreadJobs, (int)ismENGINE_DEFAULT_DISABLE_THREAD_JOB_QUEUES);
    }

    iprop = ism_common_getIntConfig(ismENGINE_CFGPROP_THREAD_JOB_QUEUES_ALGORITHM,
                                    ismENGINE_DEFAULT_THREAD_JOB_QUEUES_ALGORITHM);
    if (iprop < 0)
    {
        ismEngine_serverGlobal.ThreadJobAlgorithm = ismENGINE_DEFAULT_THREAD_JOB_QUEUES_ALGORITHM;
    }
    else
    {
        ismEngine_serverGlobal.ThreadJobAlgorithm = (uint64_t)iprop;
        if (ismEngine_serverGlobal.ThreadJobAlgorithm != ismENGINE_DEFAULT_THREAD_JOB_QUEUES_ALGORITHM)
        {
            ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "Thread Job Algorithm : %u\n", ismEngine_serverGlobal.ThreadJobAlgorithm);
        }
    }
    // Initialise the Minimum number of clients that must be active before transactions start queueing work back
    // to originating thread's job queues (Note: We initialize this whether job queues are enabled or not but we don't
    // read it if we are doing "extra" thread jobs as we want to use thread jobs whatever the number of clients)
    if (ismEngine_serverGlobal.ThreadJobAlgorithm != ismENGINE_THREAD_JOB_QUEUES_ALGORITHM_EXTRA)
    {
        iprop = ism_common_getIntConfig(ismENGINE_CFGPROP_TRANSACTION_THREADJOBS_CLIENT_MINIMUM,
                                        ismENGINE_DEFAULT_TRANSACTION_THREADJOBS_CLIENT_MINIMUM);
        if (iprop < 0)
        {
            ismEngine_serverGlobal.TransactionThreadJobsClientMinimum = ismENGINE_DEFAULT_TRANSACTION_THREADJOBS_CLIENT_MINIMUM;
        }
        else
        {
            ismEngine_serverGlobal.TransactionThreadJobsClientMinimum = (uint64_t)iprop;
            if (ismEngine_serverGlobal.TransactionThreadJobsClientMinimum != ismENGINE_DEFAULT_TRANSACTION_THREADJOBS_CLIENT_MINIMUM)
            {
                ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "Transaction thread jobs minimum clients : %lu\n", ismEngine_serverGlobal.TransactionThreadJobsClientMinimum);
            }
        }
    }
    else
    {
        ismEngine_serverGlobal.TransactionThreadJobsClientMinimum = 1; //Always use Thread Jobs
    }

    ismEngine_serverGlobal.isIoTEnvironment = ism_common_getBooleanConfig(ismENGINE_CFGPROP_ENVIRONMENT_IS_IOT,
                                                                          ismENGINE_DEFAULT_ENVIRONMENT_IS_IOT);

    if (ismEngine_serverGlobal.isIoTEnvironment != ismENGINE_DEFAULT_ENVIRONMENT_IS_IOT)
    {
        ieutTRACEL(pThreadData, ismEngine_serverGlobal.isIoTEnvironment, ENGINE_INTERESTING_TRACE,
                   "IoT Environment: %s\n", ismEngine_serverGlobal.isIoTEnvironment ? "true" : "false");
    }

    ismEngine_serverGlobal.reducedMemoryRecoveryMode = ism_common_getBooleanConfig(ismENGINE_CFGPROP_REDUCED_MEMORY_RECOVERY_MODE,
                                                                                   ismENGINE_DEFAULT_REDUCED_MEMORY_RECOVERY_MODE);

    if (ismEngine_serverGlobal.reducedMemoryRecoveryMode != ismENGINE_DEFAULT_REDUCED_MEMORY_RECOVERY_MODE)
    {
        ieutTRACEL(pThreadData, ismEngine_serverGlobal.reducedMemoryRecoveryMode, ENGINE_INTERESTING_TRACE,
                   "Reduced Memory Recovery Mode: %s\n", ismEngine_serverGlobal.reducedMemoryRecoveryMode ? "true" : "false");
    }

    uint32_t retcode = OK;

    // Start the memory monitor task
    rc = iemem_startMemoryMonitorTask(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_001, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "iemem_startMemoryMonitor failed", rc,
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Initialise the lock manager
    rc = ielm_createLockManager(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_002, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "ielm_createLockManager failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Initialise the policy info global structure
    rc = iepi_initEnginePolicyInfoGlobal(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_003, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "iepi_initEnginePolicyInfoGlobal failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Initialise the import/export function structure
    rc = ieie_initImportExport(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_013, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "ieie_initImportExport failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Initialise the topic tree
    rc = iett_initEngineTopicTree(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_004, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "iett_initEngineTopicTree failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Initialise the root of the engine's remote server information
    rc = iers_initEngineRemoteServers(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_005, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "iers_initEngineRemoteServers failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Initialise the queue namespace
    rc = ieqn_initEngineQueueNamespace(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_006, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "ieqn_initEngineQueueNamespace failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Initialise the client-state table
    rc = iecs_createClientStateTable(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_007, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "iecs_createClientStateTable failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Initialise the transaction control
    rc = ietr_initTransactions(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_008, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "ietr_initTransactions failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Register a callback with the config component this must happen pretty
    // early on since several of the sub-component init functions and
    // reconciliation of queues with admin objects need to make use of the
    // returned handle to access configuration properties.
    rc = ism_config_register(ISM_CONFIG_COMP_ENGINE,
                             NULL,
                             ism_engine_configCallback,
                             &ismEngine_serverGlobal.configCallbackHandle);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_009, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "ism_config_register failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Register a callback with the cluster component
    rc = ism_cluster_registerEngineEventCallback(iers_clusterEventCallback, NULL);

    // Initialise whether the cluster is enabled
    if (rc == OK)
    {
        ismEngine_serverGlobal.clusterEnabled = true;
    }
    else if (rc == ISMRC_ClusterDisabled)
    {
        ismEngine_serverGlobal.clusterEnabled = false;
        rc = OK;

        // If the cluster is disabled, we don't bother starting the cluster retained
        // sync check thread - to do this we set the interval to 0
        assert(ismEngine_serverGlobal.ClusterRetainedSyncTimer == NULL); // Check it hasn't already started!
        ieutTRACEL(pThreadData, iprop, ENGINE_INTERESTING_TRACE, "Cluster retained sync checking disabled\n");
        ismEngine_serverGlobal.ClusterRetainedSyncInterval = 0;
    }
    else
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_010, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "ism_cluster_registerEngineEventCallback failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Initialise message expiry
    rc = ieme_initMessageExpiry(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_011, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "ieme_initMessageExpiry failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Initialise clientState expiry
    rc = iece_initClientStateExpiry(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_012, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "iece_initClientStateExpiry failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Initialise resource set stats
    rc = iere_initResourceSetStats(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_013, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "iere_initResourceSetStats failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Initialise this thread's caches for resource set stats early (if they are used)
    iere_initResourceSetThreadCache(pThreadData);

    // Initialize the per-thread jobs code if it is not disabled
    if (!disableThreadJobs)
    {
        rc = ietj_initThreadJobs(pThreadData);
        if (UNLIKELY(rc != OK))
        {
            if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
            {
                //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
                ism_admin_setMaintenanceMode(rc, 0);
            }

            ieutTRACE_FFDC(ieutPROBE_014, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                           "ietj_initThreadJobs failed", rc,
                           "pThreadData", pThreadData, sizeof(*pThreadData),
                           NULL);
            if (retcode == OK) retcode = rc;
        }
    }

    // Initialize the namespaces used by globally shared subscriptions
    rc = iett_initSharedSubNameSpaces(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_015, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "iere_initSharedSubNameSpaces failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Perform restart recovery
    rc = ierr_restartRecovery(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_016, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "ierr_restartRecovery failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    if (retcode != OK)
    {
        // If the engine fails to start then log a message to give as much
        // visibility of the failure to start as possible.
        char messageBuffer[256];

        LOG(ERROR, Messaging, 3009, "%s%d", "The server encountered a problem as it started and is unable to start messaging. Error={0} RC={1}.",
            ism_common_getErrorStringByLocale(retcode, ism_common_getLocale(), messageBuffer, 255),
            retcode);
    }

    ieutTRACEL(pThreadData, retcode,  ENGINE_CEI_TRACE, FUNCTION_EXIT "retcode=%d\n", __func__, retcode);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Engine component start messaging
///
/// Performs final recovery actions to enable messaging to start
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_startMessaging(void)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, 0, ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    uint32_t retcode = OK;

    // Do the recovery tasks which are delayed until messaging is about to start
    rc = ierr_startMessaging(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_001, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "ierr_prepareForMessaging failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Move into running phase
    ieut_setEngineRunPhase(EnginePhaseRunning);

    // Set up timers to perform regular tasks
    ietm_setUpTimers();

    // When the store management generate is close to full we alter
    // our behavior to avoid creating Owner records. Register a callback
    // which the store will use to notify us when the management
    // generation is close to full.
    rc = ism_store_registerEventCallback(iest_storeEventHandler, NULL);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_002, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "ism_store_registerEventCallback failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Start performing message expiry
    rc = ieme_startMessageExpiry(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_003, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "ieme_startMessageExpiry failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // Start performing clientState expiry
    rc = iece_startClientStateExpiry(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_004, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "iece_startClientStateExpiry failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    rc = ietj_startThreadJobScavenger(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_005, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "ietj_startThreadJobScavenger failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    rc = iere_startResourceSetReporting(pThreadData);
    if (UNLIKELY(rc != OK))
    {
        if (ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY)
        {
            //Set maintenance mode but don't restart, that'll happen in a sec (FDC)
            ism_admin_setMaintenanceMode(rc, 0);
        }

        ieutTRACE_FFDC(ieutPROBE_006, ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY,
                       "iere_startResourceSetReporting failed", rc,
                       "pThreadData", pThreadData, sizeof(*pThreadData),
                       NULL);
        if (retcode == OK) retcode = rc;
    }

    // How long did startup take?
    double elapsedTime = ism_common_readTSC()-enginePhaseStartingTime;
    ieutTRACEL(pThreadData, elapsedTime, ENGINE_INTERESTING_TRACE, FUNCTION_IDENT
               "Elapsed time since EnginePhaseStarting: %.2f seconds. EnginePhaseInitializing: %.2f seconds.\n",
               __func__, elapsedTime, (ism_common_readTSC()-enginePhaseInitializingTime));

    ieutTRACEL(pThreadData, retcode,  ENGINE_CEI_TRACE, FUNCTION_EXIT "retcode=%d\n", __func__, retcode);
    ieut_leavingEngine(pThreadData);
    return rc;
}

static void stopCallbackThreads(void)
{
    uint64_t waits = 0;
    bool keepGoing;
    int32_t rc = OK;

    TRACE(ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    do
    {
        keepGoing = false;

        rc = ism_store_stopCallbacks();

        if (rc == ISMRC_StoreBusy)
        {
            usleep(50*1000); //50ms = 0.05s

            waits++;

            if (waits < 5*20) //5 seconds
            {
                keepGoing = true;
            }
        }
    }
    while (keepGoing);

    if (rc != OK)
    {
        ieutTRACE_FFDC(ieutPROBE_001, true,
                "Unable to stop callback threads", rc,
                NULL);
    }

    TRACE(ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
}


//****************************************************************************
/// @internal
///
/// @brief  Engine component shutdown
///
/// Shuts down the Engine.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_term(void)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;
    ismEngineRunPhase_t runPhase = ismEngine_serverGlobal.runPhase;

    ieutTRACEL(pThreadData, runPhase, ENGINE_CEI_TRACE, FUNCTION_ENTRY "runPhase=%d\n", __func__, (int32_t)runPhase);

    // Don't want the config component to call us back any more
    if (ismEngine_serverGlobal.configCallbackHandle != 0)
    {
        (void)ism_config_unregister(ismEngine_serverGlobal.configCallbackHandle);
    }

    ietm_cleanUpTimers();

    // Stop reporting resource set information
    iere_stopResourceSetReporting(pThreadData);

    // Stop the message expiry reaper threads
    ieme_stopMessageExpiry(pThreadData);

    // Stop import / export
    ieie_stopImportExport(pThreadData);

    // Stop the clientState expiry reaper threads
    iece_stopClientStateExpiry(pThreadData);

    // We should only call the cluster and store components if ism_engine_start has been called,
    // otherwise they might not be ready for us to call them.
    if (runPhase >= EnginePhaseStarting)
    {
        // Don't want the cluster component to call us back any more
        iers_stopClusterEventCallbacks(pThreadData);

        // Stop the store callback threads from running so they do
        // not make engine calls after we have terminated
        stopCallbackThreads();
    }

    // Stop the thread job scavenger
    ietj_stopThreadJobScavenger(pThreadData);

#ifdef USEFAKE_ASYNC_COMMIT
    stopFakeCallBacks();
#endif

    ieut_setEngineRunPhase(EnginePhaseEnding);

    // Stop the memory monitor task
    iemem_stopMemoryMonitorTask(pThreadData);

    bool doCleanShutdown = ism_common_getBooleanConfig( ismENGINE_CFGPROP_CLEAN_SHUTDOWN
                                                      , ismENGINE_DEFAULT_CLEAN_SHUTDOWN);

    if (doCleanShutdown)
    {
        ieie_destroyImportExport(pThreadData);
        ietr_destroyTransactions(pThreadData);
        iecs_destroyClientStateTable(pThreadData);
        ieqn_destroyEngineQueueNamespace(pThreadData);
        iers_destroyEngineRemoteServers(pThreadData);
        iett_destroyEngineTopicTree(pThreadData);
        iepi_destroyEnginePolicyInfoGlobal(pThreadData);
        ielm_destroyLockManager(pThreadData);

        // Destroy the message and clientState expiry control information
        ieme_destroyMessageExpiry(pThreadData);
        iece_destroyClientStateExpiry(pThreadData);
        ietj_destroyThreadJobs(pThreadData);
        if (ismEngine_serverGlobal.deferredFrees != NULL)
        {
            ieut_destroyDeferredFreeList(pThreadData, ismEngine_serverGlobal.deferredFrees);
            iemem_free(pThreadData, iemem_deferredFreeLists, ismEngine_serverGlobal.deferredFrees);
            ismEngine_serverGlobal.deferredFrees = NULL;
        }
        iere_destroyResourceSetStats(pThreadData);
    }

    // Terminate the memory handler
    iemem_termMemHandler(pThreadData);

    ismEngine_serverGlobal.hStoreSCR = ismSTORE_NULL_HANDLE;
    ieut_setEngineRunPhase(EnginePhaseUninitialized);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Register selection function
///
/// Registers the function used to perform message selection with the engine
//****************************************************************************
XAPI void ism_engine_registerSelectionCallback(ismEngine_MessageSelectionCallback_t pSelectionFn)
{
    TRACE(ENGINE_CEI_TRACE, FUNCTION_IDENT "Fn=%p\n", __func__, pSelectionFn);

    ismEngine_serverGlobal.selectionFn = pSelectionFn;

    return;
}


//****************************************************************************
/// @internal
///
/// @brief  Register forwarding of retained message request function
///
/// Registers the function used to perform message selection with the engine
//****************************************************************************
XAPI void ism_engine_registerRetainedForwardingCallback(ismEngine_RetainedForwardingCallback_t pRetainedForwardingFn)
{
    TRACE(ENGINE_CEI_TRACE, FUNCTION_IDENT "Fn=%p\n", __func__, pRetainedForwardingFn);

    ismEngine_serverGlobal.retainedForwardingFn = pRetainedForwardingFn;

    return;
}


//****************************************************************************
/// @internal
///
/// @brief  Register delivery failure callback function
///
/// Registers the function used to to report failure to deliver messages
//****************************************************************************
XAPI void ism_engine_registerDeliveryFailureCallback(ismEngine_DeliveryFailureCallback_t pFailCallbackFn)
{
    TRACE(ENGINE_CEI_TRACE, FUNCTION_IDENT "Fn=%p\n", __func__, pFailCallbackFn);

    ismEngine_serverGlobal.deliveryFailureFn = pFailCallbackFn;

    return;
}


//****************************************************************************
/// @brief  Create Client-State
///
/// Creates a client-state handle which represents a client program
/// connecting to the messaging server. If the client-state is stealable,
/// a steal callback must be provided when it is created so that this
/// callback can be called when another client-state is created with the
/// same client ID. The provision of a steal callback indicates that the
/// client-state is stealable.
///
/// @param[in]     pClientId        Client ID (must be a non-empty string)
/// @param[in]     protocolId       Protocol identifier (see ism_protocol_id_e)
/// @param[in]     options          ismENGINE_CREATE_CLIENT_OPTION_* values
/// @param[in]     pStealContext    Optional context for steal callback
/// @param[in]     pStealCallbackFn Callback when client-state is stolen
/// @param[in]     pSecContext      Security context
/// @param[out]    phClient         Returned client-state handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK or ISMRC_ResumedClientState on successful completion
/// or an ISMRC_ value.
///
/// @remark If the specified client id already has an associated client state,
/// for instance for a durable client state that has not been resumed, or a
/// client state from which this client stole then the client state is created
/// successfully but the return code is ISMRC_ResumedClientState rather than
/// OK.
///
/// @remark The operation may complete synchronously or asynchronously. If it
/// completes synchronously, the return code indicates the completion
/// status.
///
/// If the operation completes asynchronously, the return code from
/// this function will be ISMRC_AsyncCompletion. The actual
/// completion of the operation will be signalled by a call to the
/// operation-completion callback, if one is provided. When the
/// operation becomes asynchronous, a copy of the context will be
/// made into memory owned by the Engine. This will be freed upon
/// return from the operation-completion callback. Because the
/// completion is asynchronous, the call to the operation-completion
/// callback might occur before this function returns.
///
/// On success, for synchronous completion, the client-state handle
/// will be returned in an output parameter, while, for asynchronous
/// completion, the client-state handle will be passed to the
/// operation-completion callback.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createClientState(
    const char *                    pClientId,
    uint32_t                        protocolId,
    uint32_t                        options,
    void *                          pStealContext,
    ismEngine_StealCallback_t       pStealCallbackFn,
    ismSecurity_t *                 pSecContext,
    ismEngine_ClientStateHandle_t * phClient,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, options, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(pClientId '%s', options %u, pStealContext %p)\n", __func__,
               (pClientId != NULL) ? pClientId : "(nil)", options, pStealContext);

    // Don't expect engine protocol clients to come in this way
    assert(protocolId != PROTOCOL_ID_ENGINE);

    // A non-blank clientId is required
    if (pClientId == NULL || *pClientId == '\0')
    {
        rc = ISMRC_ClientIDRequired;
        ism_common_setError(rc);
    }
    else
    {
        iereResourceSetHandle_t resourceSet = iecs_getResourceSet(pThreadData,
                                                                  pClientId,
                                                                  protocolId,
                                                                  iereOP_ADD);

        // Actually call the clientState function to allocate the clientState and add it to the table
        rc = iecs_createClientState(pThreadData,
                                    pClientId,
                                    protocolId,
                                    options,
                                    iecsCREATE_CLIENT_OPTION_NONE,
                                    resourceSet,
                                    pStealContext,
                                    pStealCallbackFn,
                                    pSecContext,
                                    phClient,
                                    pContext,
                                    contextLength,
                                    pCallbackFn);

        // Consider this to be an attempted connection if we didn't have an internal
        // allocation error...
        if (rc != ISMRC_AllocateError)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_updateInt64Stat(pThreadData, resourceSet,
                                 ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_CONNECTIONS, 1);
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, hClient=%p\n", __func__, rc, *phClient);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief last piece of ism_engine_destroyClientState()
///
///  Can be called inline, on an async callback thread or via a job queue
///
/// @param[in]     pClient        Client that is being destroyed
/// @param[in]     bInline        True if we called directly (else destroy went async)
//****************************************************************************
int32_t finishDestroyClientState( ieutThreadData_t *pThreadData
                                , ismEngine_ClientState_t *pClient
                                , bool bInline)

{
    int32_t rc;

    ieutTRACEL(pThreadData, pClient, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "(pClient %p, pClient->pThief=%p, pClient->UseCount=%u, pClient->OpState=%d)\n",
               __func__, pClient, pClient->pThief, pClient->UseCount, (int)pClient->OpState);

    // Expiry processing can resume now that we know we've finished updating the store
    pthread_spin_lock(&pClient->UseCountLock);
    pClient->fSuspendExpiry = false;
    pthread_spin_unlock(&pClient->UseCountLock);

    bool fReleasedClientState = iecs_releaseClientStateReference(pThreadData, pClient, bInline, false);
    if (fReleasedClientState)
    {
        rc = OK;
    }
    else
    {
        rc = ISMRC_AsyncCompletion;
    }

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "(pClient %p) rc=%d\n", __func__,
               pClient, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief Async (post-store commit phase) of Destroy Client-State
///
/// If ism_engine_destroyClientState() goes async then the async callback
/// thread tries to schedule this function on the originating this to
/// complete the work
///
/// @param[in]     pContext         iecsDestroyClientAsyncData_t with client info
//****************************************************************************
static void iecs_jobDestroyClientStateCompletion(ieutThreadData_t *pThreadData,
                                                 void *context)

{
    iecsDestroyClientAsyncData_t *jobData = (iecsDestroyClientAsyncData_t *)context;

    ieutTRACEL(pThreadData, jobData->asyncId, ENGINE_CEI_TRACE, FUNCTION_IDENT "csDSACId=0x%016lx\n",
                                                                            __func__, jobData->asyncId);

    iereResourceSetHandle_t resourceSet = jobData->pClient->resourceSet;

#ifdef HAINVESTIGATIONSTATS
            pThreadData->stats.asyncDestroyClientJobRan++;
#endif

    //We are never running inline with client destroy anymore (having been jobQueue scheduled),
    //Hence we call finishDestroyClientState with false
    finishDestroyClientState(pThreadData, jobData->pClient, false);

    ieut_releaseThreadDataReference(jobData->pJobThread);

    iere_primeThreadCache(pThreadData, resourceSet);
    iere_freeStruct(pThreadData, resourceSet, iemem_callbackContext, jobData, jobData->StructId);
}

//****************************************************************************
/// @internal
///
/// @brief Async (post-store commit phase) of Destroy Client-State
///
/// If ism_engine_destroyClientState() goes async this is called on async callback
/// thread - as there are a number of synchronous store commits we try to reschedule
/// the work on the thread that started the destroy.
///
/// @param[in]     rc               return code of previous fn in async stack
/// @param[in]     asyncInfo        async stack of fns to call post-commit
/// @param[in]     pContext         context for this callback (-> handle is used
///                                        as thread for job scheduling)
///
/// @return rc of previous fn in async stack (expected to be OK).
//****************************************************************************
static int32_t iecs_asyncDestroyClientStateCompletion(ieutThreadData_t *pThreadData,
                                                      int32_t rc,
                                                      ismEngine_AsyncData_t *asyncInfo,
                                                      ismEngine_AsyncDataEntry_t *context)
{
    assert(context->Type == ClientStateDestroyCompletionInfo);
    assert(asyncInfo->pClient != NULL);
    assert(context->DataLen == 0);
    assert(rc == OK);

    ieutThreadData_t *pJobThread = context->Handle;
    assert(pJobThread != NULL);

    bool scheduledSuccessfully = false;

    iead_popAsyncCallback(asyncInfo, context->DataLen);

    iereResourceSetHandle_t resourceSet = asyncInfo->pClient->resourceSet;
    iere_primeThreadCache(pThreadData, resourceSet);

    if (pJobThread->jobQueue != NULL)
    {
        iecsDestroyClientAsyncData_t *jobData;

        jobData = (iecsDestroyClientAsyncData_t *)
                    iere_malloc(pThreadData,
                                resourceSet,
                                IEMEM_PROBE(iemem_callbackContext, 21),
                                sizeof(iecsDestroyClientAsyncData_t));

        if (jobData != NULL)
        {
            ismEngine_SetStructId(jobData->StructId,  IECS_DESTROYCLIENTASYNCDATA_STRUCID);
            jobData->asyncId    = pThreadData->asyncCounter++;
            jobData->pJobThread = pJobThread;
            jobData->pClient    = asyncInfo->pClient;

            ieutTRACEL(pThreadData, jobData->asyncId, ENGINE_CEI_TRACE, FUNCTION_IDENT "csDSACId=0x%016lx\n",
                    __func__, jobData->asyncId);

            int32_t rc2 = iejq_addJob( pThreadData
                    , pJobThread->jobQueue
                    , iecs_jobDestroyClientStateCompletion
                    , jobData
#ifdef IEJQ_JOBQUEUE_PUTLOCK
                    , true
#endif
            );

            if (rc2 == OK)
            {
#ifdef HAINVESTIGATIONSTATS
                pThreadData->stats.asyncDestroyClientJobScheduled++;
#endif
                scheduledSuccessfully = true;
            }
            else
            {
                assert(rc2 == ISMRC_DestinationFull);

                iere_primeThreadCache(pThreadData, resourceSet);
                iere_freeStruct(pThreadData, resourceSet, iemem_callbackContext, jobData, jobData->StructId);
            }
        }
    }

    if(!scheduledSuccessfully)
    {
        ieut_releaseThreadDataReference(pJobThread);

        DEBUG_ONLY int32_t rc2 = finishDestroyClientState(pThreadData, asyncInfo->pClient, false);

        assert(rc2 == ISMRC_OK || rc2 == ISMRC_AsyncCompletion);
        //Whether or not the destroy went async, we don't want to delay the
        //clean up of the async call stack etc as finishDestroyClientState()
        //doesn't use it - so we don't change what we return: rc
    }

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Destroy Client-State
///
/// Destroys the client-state handle. This will not result in the
/// durable information associated with durable client-state being deleted
/// unless ismENGINE_DESTROY_CLIENT_OPTION_DISCARD is specified.
///
/// @param[in]     hClient          Client-state handle
/// @param[in]     options          ismENGINE_DESTROY_CLIENT_OPTION_* values
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_destroyClientState(
    ismEngine_ClientStateHandle_t   hClient,
    uint32_t                        options,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)hClient;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pClient);
    void *pPendingDestroyContext = NULL;
    bool fDiscardDurable = (options & ismENGINE_DESTROY_CLIENT_OPTION_DISCARD) ? true : false;
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    ieutTRACEL(pThreadData, hClient,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hClient %p, options %u)\n", __func__, hClient, options);

    ismEngine_CheckStructId(pClient->StrucId, ismENGINE_CLIENT_STATE_STRUCID, ieutPROBE_001);

    // Allocate memory for the callback in case we need to go asynchronous
    if (contextLength > 0)
    {
        iere_primeThreadCache(pThreadData, resourceSet);
        pPendingDestroyContext = iere_malloc(pThreadData,
                                             resourceSet,
                                             IEMEM_PROBE(iemem_callbackContext, 2), contextLength);
        if (pPendingDestroyContext == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }
    }

    if (rc == OK)
    {
        bool fExpiryIntervalChanged;

        // For durable clients, check whether the expiry interval set during creation
        // has been altered.
        if (pClient->Durability == iecsDurable && pClient->pSecContext != NULL)
        {
            uint32_t newExpiryInterval;

            newExpiryInterval = ism_security_context_getClientStateExpiry(pClient->pSecContext);

            fExpiryIntervalChanged = (newExpiryInterval != pClient->ExpiryInterval);

            // The value has changed.
            if (fExpiryIntervalChanged)
            {
                ieutTRACEL(pThreadData, pClient->ExpiryInterval, ENGINE_HIGH_TRACE,
                           "ExpiryInterval altered from %u to %u.\n",
                           pClient->ExpiryInterval, newExpiryInterval);
                pClient->ExpiryInterval = newExpiryInterval;
            }
        }
        else
        {
            fExpiryIntervalChanged = false;
        }

        pthread_spin_lock(&pClient->UseCountLock);
        assert(pClient->OpState == iecsOpStateActive);
        pClient->UseCount += 1;
        pClient->OpState = iecsOpStateDisconnecting;
        // Request immediate discarding of the clientState if it was explicitly requested,
        // or this is a durable client state with a 0 expiry interval now in place.
        pClient->fDiscardDurable = fDiscardDurable ||
                                   (pClient->Durability == iecsDurable && pClient->ExpiryInterval == 0);
        // Don't want expiry processing to happen for this client until we've finished updating it
        pClient->fSuspendExpiry = true;
        pthread_spin_unlock(&pClient->UseCountLock);

        // Shouldn't consider this clientState active any more (no-one can use this hClient)
        iecs_decrementActiveClientStateCount(pThreadData, pClient->Durability, pClient->fCountExternally, resourceSet);

        // Recursively destroy all of the client-state's resources.
        // Some of the resources may clean up asynchronously, but we'll be able to tell
        // because when we release our hold on the client-state below, we'll know whether
        // other references still exist...
        iecs_lockClientState(pClient);

        pClient->pPendingDestroyContext = pPendingDestroyContext;
        if (contextLength > 0)
        {
            memcpy(pPendingDestroyContext, pContext, contextLength);
        }
        pClient->pPendingDestroyCallbackFn = pCallbackFn;

        ismEngine_Session_t *pSession = pClient->pSessionHead;

        while (pSession != NULL)
        {
            ismEngine_Session_t *pSessionNext = pSession->pNext;

            if (!pSession->fIsDestroyed)
            {
                pSession->fIsDestroyed = true;
                pSession->fRecursiveDestroy = true;
            }

            pSession = pSessionNext;
        }

        // If there are any unprepared global transactions linked to
        // the client then roll them back now.
        ietr_freeClientTransactionList(pThreadData, pClient);

        // Forget about any acks that are remaining.. really this should not happen
        // for a durable client until durable state is destroyed...
        iecs_forgetInflightMsgs( pThreadData, pClient );

        iecs_unlockClientState(pClient);

        pSession = pClient->pSessionHead;
        while (pSession != NULL)
        {
            ismEngine_Session_t *pSessionNext = pSession->pNext;

            if (pSession->fRecursiveDestroy)
            {
                destroySessionInternal(pThreadData, pSession);
            }

            pSession = pSessionNext;
        }

        ismEngine_Message_t *pMessage = pClient->hWillMessage;

        // If the expiry interval has changed since the clientState was created,
        // we need to update the CPR so that we remember it upon restart.
        if (fExpiryIntervalChanged)
        {
            assert(rc == OK);

            ismStore_Handle_t willMsgStoreHdl;

            if (pClient->hWillMessage != NULL)
            {
                willMsgStoreHdl = pMessage->StoreMsg.parts.hStoreMsg;
            }
            else
            {
                willMsgStoreHdl = ismSTORE_NULL_HANDLE;
            }

            rc = iecs_updateClientPropsRecord(pThreadData,
                                              pClient,
                                              pClient->pWillTopicName,
                                              willMsgStoreHdl,
                                              pClient->WillMessageTimeToLive,
                                              pClient->WillDelay);
        }

        // If there is a chance we'll need to remember the last connected time, either because it is
        // a) a durable client or one which has associated durable objects, OR b) has a will message
        // which will require delayed will processing. And then, only if c) it isn't being stolen from
        // by a Non-CleanStart client and d) isn't being explicitly discarded.
        if (rc == OK &&
            (((pClient->Durability == iecsDurable) || pClient->durableObjects != 0) || // a
              (pMessage != NULL && pClient->WillDelay != 0)) &&                        // b
              (pClient->pThief == NULL || pClient->pThief->fCleanStart == false) &&    // c
              !pClient->fDiscardDurable)                                               // d
        {
            // Set up for the store commits to go async
            // We're going to send iecs_asyncDestroyClientStateCompletion a link to our pThreadData
            // (so it can schedule remaining work here so start by ensuring it doesn't get freed too soon
            ieut_acquireThreadDataReference(pThreadData);

            ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
                {ismENGINE_ASYNCDATAENTRY_STRUCID, ClientStateDestroyCompletionInfo,
                    NULL, 0, pThreadData,
                    {.internalFn = iecs_asyncDestroyClientStateCompletion}}};

            ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID,
                                               pClient,
                                               IEAD_MAXARRAYENTRIES, 2, 0, true,  0, 0, asyncArray};

            rc = iecs_updateLastConnectedTime(pThreadData, pClient, false, &asyncData);
        }

        if (rc == OK)
        {
            rc = finishDestroyClientState(pThreadData, pClient, true);
        }
    }
    else
    {
        assert(pPendingDestroyContext == NULL);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @brief  Destroy a disconnected Client-State
///
/// Destroys the disconnectd client-state whose clientId matches the one
/// specified, this will result in the durable information associated with durable
/// client-state being deleted.
///
/// @param[in]     pClientId        Client Id to destroy.
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The operation may complete synchronously or asynchronously. If it
/// completes synchronously, the return code indicates the completion
/// status.
///
/// If the operation completes asynchronously, the return code from
/// this function will be ISMRC_AsyncCompletion. The actual
/// completion of the operation will be signalled by a call to the
/// operation-completion callback, if one is provided. When the
/// operation becomes asynchronous, a copy of the context will be
/// made into memory owned by the Engine. This will be freed upon
/// return from the operation-completion callback. Because the
/// completion is asynchronous, the call to the operation-completion
/// callback might occur before this function returns.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_destroyDisconnectedClientState(
        const char *                   pClientId,
        void *                         pContext,
        size_t                         contextLength,
        ismEngine_CompletionCallback_t pCallbackFn)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    assert(pClientId != NULL);

    ieutTRACEL(pThreadData, pClientId,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(pClientId %s)\n", __func__, pClientId);

    rc = iecs_discardZombieClientState(pThreadData, pClientId, false, pContext, contextLength, pCallbackFn);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Set Will Message
///
/// Sets the will message for a client-state. The will message is published
/// when the client-state is destroyed.
///
/// @param[in]     hClient          Client-state handle
/// @param[in]     destinationType  Destination type
/// @param[in]     pDestinationName Destination name
/// @param[in]     hWillMessage     Will message handle
/// @param[in]     delayInterval    Delay (in seconds) before will message is published
/// @param[in]     timeToLive       Time to live (in seconds) of the will message
///                                 (0 for no limit)
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_setWillMessage(
    ismEngine_ClientStateHandle_t   hClient,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        delayInterval,
    uint32_t                        timeToLive,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Message_t *pMessage = (ismEngine_Message_t *)hMessage;
    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)hClient;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pClient);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hClient, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hClient %p, pDestinationName '%s', pMessage %p, delayInterval=%u)\n", __func__,
               hClient, (pDestinationName != NULL) ? pDestinationName : "(nil)", pMessage, delayInterval);

    // The topic string must not contain wildcards
    if ((destinationType != ismDESTINATION_TOPIC) ||
        !iett_validateTopicString(pThreadData, pDestinationName, iettVALIDATE_FOR_PUBLISH))
    {
        rc = ISMRC_DestNotValid;
        ism_common_setErrorData(rc, "%s", pDestinationName);
        goto mod_exit;
    }

    // We check the client's authority to publish the will message now since it
    // is possible it won't actually be published until after restart, and we can't
    // recreate the security context reliably
    iepiPolicyInfo_t *pValidatedPolicyInfo = NULL;

    rc = ismEngine_security_validate_policy_func(pClient->pSecContext,
                                                 ismSEC_AUTH_TOPIC,
                                                 pDestinationName,
                                                 ismSEC_AUTH_ACTION_PUBLISH,
                                                 ISM_CONFIG_COMP_ENGINE,
                                                 (void **)&pValidatedPolicyInfo);

    if (rc != OK) goto mod_exit;

    // The policy can come back with a NULL context for __MQConnectiivty requests and unit tests,
    // so if we are authorized, but got no context, we use the default policy.
    if (pValidatedPolicyInfo == NULL) pValidatedPolicyInfo = iepi_getDefaultPolicyInfo(false);

    // Limit the timeToLive to the maximum of the policy (if a maximum is set)
    if (pValidatedPolicyInfo->maxMessageTimeToLive != 0)
    {
        if (timeToLive == iecsWILLMSG_TIME_TO_LIVE_INFINITE ||
            timeToLive > pValidatedPolicyInfo->maxMessageTimeToLive)
        {
            timeToLive = pValidatedPolicyInfo->maxMessageTimeToLive;
        }
    }

    // Actually set the will message
    rc = iecs_setWillMessage(pThreadData,
                             pClient,
                             pDestinationName,
                             pMessage,
                             timeToLive,
                             delayInterval,
                             pContext,
                             contextLength,
                             pCallbackFn);

mod_exit:

    // It is up to us to release the use count on the message. In time
    // this may be done deeper in the engine.
    iem_releaseMessage(pThreadData, pMessage);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Unset Will Message
///
/// Unsets the will message for a client-state.
///
/// @param[in]     hClient          Client-state handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_unsetWillMessage(
    ismEngine_ClientStateHandle_t   hClient,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)hClient;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pClient);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hClient,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hClient %p)\n", __func__, hClient);

    rc = iecs_unsetWillMessage(pThreadData, pClient);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Create Session
///
/// Creates a session handle which represents a session between a
/// client program and the messaging server.
///
/// @param[in]     hClient          Client-state handle
/// @param[in]     options          ismENGINE_SESSION_OPTION_* values
/// @param[out]    phSession        Returned session handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createSession(
    ismEngine_ClientStateHandle_t   hClient,
    uint32_t                        options,
    ismEngine_SessionHandle_t *     phSession,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)hClient;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pClient);
    ismEngine_Session_t *pSession = NULL;
    int osrc;
    int32_t rc = OK;
    assert(pClient != NULL);
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    ieutTRACEL(pThreadData, hClient,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hClient %p, options %u)\n", __func__, hClient, options);

    ismEngine_CheckStructId(pClient->StrucId, ismENGINE_CLIENT_STATE_STRUCID, ieutPROBE_001);

    // Allocate and initialise the session
    iere_primeThreadCache(pThreadData, resourceSet);
    pSession = (ismEngine_Session_t *)iere_malloc(pThreadData,
                                                  resourceSet,
                                                  IEMEM_PROBE(iemem_externalObjs, 2),
                                                  sizeof(ismEngine_Session_t));
    if (pSession != NULL)
    {
        ismEngine_SetStructId(pSession->StrucId, ismENGINE_SESSION_STRUCID);
        osrc = pthread_mutex_init(&pSession->Mutex, NULL);
        if (osrc == 0)
        {
            pSession->pClient = pClient;
            pSession->pPrev = NULL;
            pSession->pNext = NULL;
            pSession->pTransactionHead = NULL;
            pSession->pProducerHead = NULL;
            pSession->pProducerTail = NULL;
            pSession->pConsumerHead = NULL;
            pSession->pConsumerTail = NULL;
            pSession->fIsDestroyed = false;
            pSession->fRecursiveDestroy = false;
            pSession->fIsDeliveryStarted = false;
            pSession->fIsDeliveryStopping = false;
            pSession->fEngineControlledSuspend= false;
            pSession->ActiveCallbacks = 0;
            pSession->PreNackAllCount = 1; // (+1 for this session not being destroyed)
            pSession->UseCount = 2;        // (+1 for this session not being destroyed, +1 for non-zero PreNackAllCount)
            pSession->pPendingDestroyContext = NULL;
            pSession->pPendingDestroyCallbackFn = NULL;
            pSession->firstAck = NULL;
            pSession->lastAck  = NULL;
            pSession->pXARecoverIterator = NULL;

            if (options & ismENGINE_CREATE_SESSION_TRANSACTIONAL)
            {
                pSession->fIsTransactional = true;
            }
            else
            {
                pSession->fIsTransactional = false;
            }

            if (options & ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND)
            {
                pSession->fExplicitSuspends = true;
            }
            else
            {
                pSession->fExplicitSuspends = false;
            }

            osrc = pthread_spin_init(&pSession->ackListGetLock, PTHREAD_PROCESS_PRIVATE);

            if (osrc == 0)
            {
                osrc = pthread_spin_init(&pSession->ackListPutLock, PTHREAD_PROCESS_PRIVATE);

                if (osrc != 0)
                {
                    ieutTRACEL(pThreadData, osrc,  ENGINE_ERROR_TRACE, "%s: pthread_spin_init failed (rc=%d)\n", __func__, osrc);
                    pthread_spin_destroy(&pSession->ackListGetLock);
                    pthread_mutex_destroy(&pSession->Mutex);
                    iere_freeStruct(pThreadData, resourceSet, iemem_externalObjs, pSession, pSession->StrucId);
                    pSession = NULL;
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                }
            }
            else
            {
                ieutTRACEL(pThreadData, osrc,  ENGINE_ERROR_TRACE, "%s: pthread_spin_init failed (rc=%d)\n", __func__, osrc);
                pthread_mutex_destroy(&pSession->Mutex);
                iere_freeStruct(pThreadData, resourceSet, iemem_externalObjs, pSession, pSession->StrucId);
                pSession = NULL;
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            }
        }
        else
        {
            ieutTRACEL(pThreadData, osrc,  ENGINE_ERROR_TRACE, "%s: pthread_mutex_init failed (rc=%d)\n", __func__, osrc);
            iere_freeStruct(pThreadData, resourceSet, iemem_externalObjs, pSession, pSession->StrucId);
            pSession = NULL;
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

    // Add the session the client-state's session list
    if (rc == OK)
    {
        iecs_lockClientState(pClient);

        if (pClient->pSessionHead != NULL)
        {
            pSession->pNext = pClient->pSessionHead;
            pClient->pSessionHead->pPrev = pSession;
            pClient->pSessionHead = pSession;
        }
        else
        {
            pClient->pSessionHead = pSession;
            pClient->pSessionTail = pSession;
        }

        pthread_spin_lock(&pClient->UseCountLock);
        pClient->UseCount += 1;
        pthread_spin_unlock(&pClient->UseCountLock);

        iecs_unlockClientState(pClient);
    }

    if (rc == OK)
    {
        *phSession = pSession;
    }
    else if (pSession != NULL)
    {
        pthread_mutex_destroy(&pSession->Mutex);
        pthread_spin_destroy(&pSession->ackListGetLock);
        pthread_spin_destroy(&pSession->ackListPutLock);
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_freeStruct(pThreadData, resourceSet, iemem_externalObjs, pSession, pSession->StrucId);
        pSession = NULL;
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, hSession=%p\n", __func__, rc, pSession);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Destroy Session
///
/// Destroys the session.
///
/// @param[in]     hSession         Session handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_destroySession(
    ismEngine_SessionHandle_t       hSession,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    assert(pSession != NULL);
    ismEngine_ClientState_t *pClient = pSession->pClient;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pClient);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p)\n", __func__, hSession);

    ismEngine_CheckStructId(pSession->StrucId, ismENGINE_SESSION_STRUCID, ieutPROBE_001);

    // Take the session out of the client-state's list of sessions and mark it as destroyed
    iecs_lockClientState(pClient);

    if (pSession->fIsDestroyed)
    {
        iecs_unlockClientState(pClient);
        rc = ISMRC_Destroyed;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // The consumer has a use count to which the create/destroy pair contributes one.
    // If the use count is greater than one, we may complete this call asynchronously.
    // We must prepare for this before we start releasing storage.
    if (pSession->UseCount > 1)
    {
        if (contextLength > 0)
        {
            pSession->pPendingDestroyContext = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_callbackContext, 3), contextLength);
            if (pSession->pPendingDestroyContext != NULL)
            {
                memcpy(pSession->pPendingDestroyContext, pContext, contextLength);
            }
            else
            {
                iecs_unlockClientState(pClient);
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }
        }
        pSession->pPendingDestroyCallbackFn = pCallbackFn;
    }

    pSession->fIsDestroyed = true;

    if (pSession->pPrev == NULL)
    {
        if (pSession->pNext == NULL)
        {
            pClient->pSessionHead = NULL;
            pClient->pSessionTail = NULL;
        }
        else
        {
            pSession->pNext->pPrev = NULL;
            pClient->pSessionHead = pSession->pNext;
        }
    }
    else
    {
        if (pSession->pNext == NULL)
        {
            pSession->pPrev->pNext = NULL;
            pClient->pSessionTail = NULL;
        }
        else
        {
            pSession->pPrev->pNext = pSession->pNext;
            pSession->pNext->pPrev = pSession->pPrev;
        }
    }

    iecs_unlockClientState(pClient);

    // Recursively destroy all of the session's resources.
    // Some of the resources may clean up asynchronously, but we'll be able to tell
    // because when we release our hold on the session below, we'll know whether
    // other references still exist...
    rc = destroySessionInternalNoRelease(pThreadData, pSession);

    // The create/destroy pair contributes one to the session's use count,
    // so this operation needs to release its reference
    if (rc == OK)
    {
        bool fReleasedSession = releaseSessionReference(pThreadData, pSession, true);
        if (!fReleasedSession)
        {
            rc = ISMRC_AsyncCompletion;
        }
    }

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


// Internal function to destroy a session as part of recursive clean-up
static inline int32_t destroySessionInternal(ieutThreadData_t *pThreadData,
                                             ismEngine_Session_t *pSession)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pSession,  ENGINE_HIGH_TRACE, FUNCTION_ENTRY "(hSession %p)\n", __func__, pSession);

    // First recursively destroy the session's resources
    rc = destroySessionInternalNoRelease(pThreadData, pSession);

    // Then release the reference on the session so it too can be freed
    if (rc == OK)
    {
        releaseSessionReference(pThreadData, pSession, true);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIGH_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


// Internal function to destroy a session
static inline int32_t destroySessionInternalNoRelease(ieutThreadData_t *pThreadData,
                                                      ismEngine_Session_t *pSession)
{
    bool fStopDeliveryRequired = false;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pSession,  ENGINE_HIGH_TRACE, FUNCTION_ENTRY "(hSession %p)\n", __func__, pSession);

    rc = ism_engine_lockSession(pSession);
    if (rc == OK)
    {
        pSession->fIsDestroyed = true;

        // First mark the producers and consumers as being recursively destroyed
        ismEngine_Producer_t *pProducer = pSession->pProducerHead;
        while (pProducer != NULL)
        {
            ismEngine_Producer_t *pProducerNext = pProducer->pNext;

            if (!pProducer->fIsDestroyed)
            {
                pProducer->fIsDestroyed = true;
                pProducer->fRecursiveDestroy = true;
            }

            pProducer = pProducerNext;
        }

        ismEngine_Consumer_t *pConsumer = pSession->pConsumerHead;
        while (pConsumer != NULL)
        {
            ismEngine_Consumer_t *pConsumerNext = pConsumer->pNext;

            if (!pConsumer->fIsDestroyed)
            {
                pConsumer->fIsDestroyed = true;
                pConsumer->fRecursiveDestroy = true;
            }

            pConsumer = pConsumerNext;
        }

        if (pSession->fIsDeliveryStarted)
        {
            pSession->fIsDeliveryStarted = false;
            pSession->fIsDeliveryStopping = true;
            fStopDeliveryRequired = true;
        }

        // Unlock the session
        ism_engine_unlockSession(pSession);

        //The session being destroyed reduces the count of things to do before we nack
        //all the messages by one
        reducePreNackAllCount(pThreadData, pSession);


        // Now actually destroy the producers and consumers. The storage
        // is freed as the individual use counts hit zero.
        pProducer = pSession->pProducerHead;
        while (pProducer != NULL)
        {
            ismEngine_Producer_t *pProducerNext = pProducer->pNext;

            if (pProducer->fRecursiveDestroy)
            {
                destroyProducerInternal(pThreadData, pProducer);
            }

            pProducer = pProducerNext;
        }

        pConsumer = pSession->pConsumerHead;
        while (pConsumer != NULL)
        {
            ismEngine_Consumer_t *pConsumerNext = pConsumer->pNext;

            if (pConsumer->fRecursiveDestroy)
            {
                destroyConsumerInternal(pThreadData, pConsumer);
            }

            pConsumer = pConsumerNext;
        }

        // And finally, tidy up after message delivery
        if (fStopDeliveryRequired)
        {
            completeStopMessageDeliveryInternal(pThreadData, pSession, false);
        }

        // Free any transactions associated with the session, this will also
        // free any active XA recover scans
        ietr_freeSessionTransactionList(pThreadData, pSession);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIGH_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


// Release a reference to a session, deallocating the session when
// the use-count drops to zero
// If fInline is true, the reference is being released by the operation
// to destroy the session and is thus "inline". When it's done inline,
// the destroy completion callback does not need to be called because the caller
// is synchronously completing the destroy.
bool releaseSessionReference(ieutThreadData_t *pThreadData,
                             ismEngine_Session_t *pSession,
                             bool fInline)
{
    bool fDidRelease = false;

    uint32_t usecount = __sync_sub_and_fetch(&pSession->UseCount, 1);
    if (usecount == 0)
    {
        ieutTRACEL(pThreadData, pSession, ENGINE_HIGH_TRACE, "Deallocating session %p\n", pSession);

        // The session contributes a reference to the client-state
        ismEngine_ClientState_t *pClient = pSession->pClient;
        iereResourceSetHandle_t resourceSet = pClient->resourceSet;
        ismEngine_CompletionCallback_t pPendingDestroyCallbackFn = pSession->pPendingDestroyCallbackFn;
        void *pPendingDestroyContext = pSession->pPendingDestroyContext;

        // Finally deallocate the session itself
        pthread_mutex_destroy(&pSession->Mutex);
        pthread_spin_destroy(&pSession->ackListGetLock);
        pthread_spin_destroy(&pSession->ackListPutLock);

        iere_primeThreadCache(pThreadData, resourceSet);
        iere_freeStruct(pThreadData, resourceSet, iemem_externalObjs, pSession, pSession->StrucId);

        fDidRelease = true;

        // If we're not running in-line, we're completing this asynchronously and
        // may have been given a completion callback for the destroy operation
        if (!fInline && (pPendingDestroyCallbackFn != NULL))
        {
            pPendingDestroyCallbackFn(OK, NULL, pPendingDestroyContext);
        }
        if (pPendingDestroyContext != NULL)
        {
            iemem_free(pThreadData, iemem_callbackContext, pPendingDestroyContext);
        }

        iecs_releaseClientStateReference(pThreadData, pClient, false, false);
    }

    return fDidRelease;
}

void reducePreNackAllCount(ieutThreadData_t *pThreadData, ismEngine_Session_t *pSession)
{
    uint32_t usecount = __sync_sub_and_fetch(&pSession->PreNackAllCount, 1);
    if (usecount == 0)
    {
        //Now no consumers are active, nack any remaing messages for them
        int32_t rc = ieal_nackOutstandingMessages(pThreadData, pSession);
        assert(rc == OK || rc == ISMRC_AsyncCompletion);

        if (rc != ISMRC_AsyncCompletion)
        {
            // We can now release the session reference that corresponds to
            //non-zero PreNackAll count
            releaseSessionReference(pThreadData, pSession, false);
        }
    }
}


//****************************************************************************
/// @internal
///
/// @brief  Start Message Delivery
///
/// Starts or restarts message delivery for a session. Messages are delivered
/// to message consumers via their message-delivery callbacks.
///
/// @param[in]     hSession         Session handle
/// @param[in]     options          ismENGINE_START_DELIVERY_OPTION_* values
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_startMessageDelivery(
    ismEngine_SessionHandle_t       hSession,
    uint32_t                        options,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    assert(pSession != NULL);
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    bool fEnableWaiters = false;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, options 0x%x)\n", __func__, hSession, options);

    ismEngine_CheckStructId(pSession->StrucId, ismENGINE_SESSION_STRUCID, ieutPROBE_001);

    rc = ism_engine_lockSession(pSession);
    if (rc == OK)
    {
        if (pSession->fIsDestroyed)
        {
            rc = ISMRC_Destroyed;
            ism_common_setError(rc);
        }
        else if (pSession->fIsDeliveryStopping)
        {
            rc = ISMRC_RequestInProgress;
        }
        else if ((options & ismENGINE_START_DELIVERY_OPTION_ENGINE_START) && (!(pSession->fEngineControlledSuspend)))
        {
            //The engine tried to restart the session after flow control but the protocol/transport has reasserted control
            rc = ISMRC_NotEngineControlled;
        }
        else if (!pSession->fIsDeliveryStarted)
        {
            fEnableWaiters = true;
            pSession->fEngineControlledSuspend = false;
            pSession->fIsDeliveryStarted = true;

            // Message-delivery contributes one to the session's use-count
            __sync_fetch_and_add(&pSession->UseCount, 1);

            // Message-delivery being started also contributes one to the number of active callbacks
            __sync_fetch_and_add(&pSession->ActiveCallbacks, 1);
        }
        else
        {
            fEnableWaiters = true;
            pSession->fEngineControlledSuspend = false;
        }

        if (fEnableWaiters)
        {
            //Loop around marking all the waiters enabled.
            //We want to actually deliver messages as well as mark them enabled but don't want to do that with the
            //session lock held so we'll loop again in a minute...
            ismEngine_Consumer_t *pConsumer = pSession->pConsumerHead;

            while (pConsumer != NULL)
            {
                acquireConsumerReference(pConsumer); //An enabled waiter counts as a reference
                __sync_fetch_and_add(&pSession->ActiveCallbacks, 1);

                rc = ieq_enableWaiter( pThreadData
                                     , pConsumer->queueHandle
                                     , pConsumer
                                     , IEQ_ENABLE_OPTION_DELIVER_LATER);

                if (rc == ISMRC_WaiterEnabled)
                {
                    //It was already enabled...
                    //(undo increase of active callbacks/usecounts) which were speculatively increased above
                    DEBUG_ONLY uint32_t oldvalue = __sync_fetch_and_sub(&pSession->ActiveCallbacks, 1);
                    assert(oldvalue > 0);
                    releaseConsumerReference(pThreadData, pConsumer, false);
                    rc = OK;
                }
                else if (rc == ISMRC_DisableWaiterCancel)
                {
                    //We've cancelled a disable, the disable callbacks and usecounts reductions
                    //etc... will still happen (but the waiter will just stay enabled) so we do need our
                    //speculative count increases
                    rc = OK;
                }
                else if (rc == ISMRC_WaiterInvalid)
                {
                    //The waiter status is not valid...
                    //(undo increase of active callbacks/usecounts which were increased in getNextConsumerToEnable)
                    DEBUG_ONLY uint32_t oldvalue = __sync_fetch_and_sub(&pSession->ActiveCallbacks, 1);
                    assert(oldvalue > 0);
                    releaseConsumerReference(pThreadData, pConsumer, false);
                }

                pConsumer = pConsumer->pNext;
            }
        }
        ism_engine_unlockSession(pSession);
    }

    ieutTRACEL(pThreadData, fEnableWaiters, ENGINE_NORMAL_TRACE,"Going to call checkwaiters...\n");

    // Now loop to delivery messages to any waiters we enabled...
    if (fEnableWaiters)
    {
        ismEngine_Consumer_t *pConsumer = NULL;
        ismEngine_DelivererContext_t delivererContext;
        delivererContext.lockStrategy.rlac = LS_NO_LOCK_HELD;
        delivererContext.lockStrategy.lock_persisted_counter = 0;
        delivererContext.lockStrategy.lock_dropped_counter = 0;

        while (rc == OK)
        {
            rc = getNextConsumerForDelivery( pThreadData
                                           , pSession
                                           , &pConsumer);

            if ((rc == OK) && (pConsumer != NULL))
            {
                ieq_checkWaiters(pThreadData, pConsumer->queueHandle, NULL, &delivererContext);

                //Undo increase that happened in getNextConsumer as we've finished with it
                releaseConsumerReference(pThreadData, pConsumer, false);
            }
            else
            {
                if (rc == ISMRC_RequestInProgress)
                {
                    // This request came in first and started before being stopped
                    // so we'll consider that an OK result
                    rc = OK;
                }
                break;
            }
        }
        if ( delivererContext.lockStrategy.rlac == LS_READ_LOCK_HELD || delivererContext.lockStrategy.rlac == LS_WRITE_LOCK_HELD ) {
            ieutTRACEL(pThreadData, 0, ENGINE_PERFDIAG_TRACE,
               "RLAC Lock was held and has now been released, debug: %d,%d\n",
               delivererContext.lockStrategy.lock_persisted_counter,delivererContext.lockStrategy.lock_dropped_counter);
            ism_common_unlockACLList();
        } else {
            ieutTRACEL(pThreadData, 0, ENGINE_PERFDIAG_TRACE,
               "RLAC Lock was not held, debug: %d,%d\n",
               delivererContext.lockStrategy.lock_persisted_counter,delivererContext.lockStrategy.lock_dropped_counter);
        } 
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


static inline int32_t getNextConsumerForDelivery(ieutThreadData_t *pThreadData,
                                                 ismEngine_Session_t *pSession,
                                                 ismEngine_Consumer_t **ppConsumer)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pSession,  ENGINE_HIGH_TRACE, FUNCTION_ENTRY "(hSession %p)\n", __func__, pSession);

    rc = ism_engine_lockSession(pSession);
    if (rc == OK)
    {
        if (pSession->fIsDestroyed)
        {
            rc = ISMRC_Destroyed;
            ism_common_setError(rc);
        }
        else if (    (pSession->fIsDeliveryStopping)
                  ||!(pSession->fIsDeliveryStarted))
        {
            //Delivery is stopping or been stopped
            rc = ISMRC_RequestInProgress;
        }
        else
        {
            ismEngine_Consumer_t  *pConsumer = pSession->pConsumerHead;
            bool                   prevConsumerFound = false;

            // If ppConsumer is non-null and points to a consumer in the list,
            // We want the next consumer...otherwise we want the list head
            if (*ppConsumer != NULL)
            {
                // Try and find the next consumer
                while (pConsumer != NULL)
                {
                    if (pConsumer == *ppConsumer)
                    {
                        // Found the previous consumer...the next one is the one we want
                        pConsumer = pConsumer->pNext;
                        prevConsumerFound = true;
                        break;
                    }
                    else
                    {
                        pConsumer = pConsumer->pNext;
                    }
                }

                //Did we find the next consumer...if not use list-head
                if (!prevConsumerFound)
                {
                    pConsumer = pSession->pConsumerHead;
                }
            }

            //We've found where we got to in the list...we'll do delivery for the next consumer
            if (pConsumer != NULL)
            {
                *ppConsumer = pConsumer;
                acquireConsumerReference(pConsumer); //Need to keep hold of this consumer until we've
                                                     //attempted delivery on it
            }
            else
            {
                *ppConsumer = NULL;
            }
        }

        ism_engine_unlockSession(pSession);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIGH_TRACE, FUNCTION_EXIT "rc=%d, hconsumer=%p\n", __func__, rc, *ppConsumer);
    return rc;
}

int32_t stopMessageDeliveryInternal(
        ieutThreadData_t *              pThreadData,
        ismEngine_SessionHandle_t       hSession,
        uint32_t                        InternalFlags,
        void *                          pContext,
        size_t                          contextLength,
        ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ismEngine_Consumer_t *pConsumer = NULL;
    bool fDisableWaiters = false;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p Flags 0x%x)\n", __func__, hSession, InternalFlags);

    assert(pSession != NULL);
    ismEngine_CheckStructId(pSession->StrucId, ismENGINE_SESSION_STRUCID, ieutPROBE_001);

    rc = ism_engine_lockSession(pSession);
    if (rc == OK)
    {
        if (InternalFlags & ISM_ENGINE_INTERNAL_STOPMESSAGEDELIVERY_FLAGS_ENGINE_STOP)
        {
            if(!pSession->fEngineControlledSuspend)
            {
                //The engine is trying to stop the session but it either has not been
                //marked as under engine control or the protocol/transport have subsequently
                //started/stopped it
                rc = ISMRC_NotEngineControlled;
                ism_engine_unlockSession(pSession);
                goto mod_exit;
            }
#ifdef HAINVESTIGATIONSTATS
            pThreadData->stats.perSessionFlowControlCount++;
#endif
        }
        else
        {
            //Someone other than the engine (doing flow control) is telling us to stop. 
            //Make sure the flow control does start delivery again
            pSession->fEngineControlledSuspend = false;
        }

        if (pSession->fIsDeliveryStarted)
        {
            fDisableWaiters = true;
            pSession->fIsDeliveryStarted = false;
            pSession->fIsDeliveryStopping = true;
        }

        ism_engine_unlockSession(pSession);
    }

    ieutTRACEL(pThreadData, fDisableWaiters,
               ENGINE_HIGH_TRACE, "%s: fdisableWaiters is %s\n", __func__, (fDisableWaiters ? "true" : "false"));

    // Now loop to disable the waiters
    if (fDisableWaiters)
    {
        rc = ism_engine_lockSession(pSession);
        if (rc == OK)
        {
            pConsumer = pSession->pConsumerHead;

            while (pConsumer != NULL)
            {
                (void)ieq_disableWaiter(pThreadData, pConsumer->queueHandle, pConsumer);
                pConsumer = pConsumer->pNext;
            }

            ism_engine_unlockSession(pSession);
        }

        // And complete the tidying up after message delivery
        completeStopMessageDeliveryInternal(pThreadData, pSession, true);
    }
mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


//****************************************************************************
/// @brief  Stop Message Delivery
///
/// Stops message delivery for a session.
///
/// @param[in]     hSession         Session handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_stopMessageDelivery(
    ismEngine_SessionHandle_t       hSession,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);

    int32_t rc = stopMessageDeliveryInternal(pThreadData,
                                             hSession,
                                             ISM_ENGINE_INTERNAL_STOPMESSAGEDELIVERY_FLAGS_NONE,
                                             pContext, contextLength, pCallbackFn);

    ieut_leavingEngine(pThreadData);
    return rc;
}


// Internal function to stop message delivery as part of recursive clean-up.
// All of the session's consumers are assumed to have been destroyed.
static inline void completeStopMessageDeliveryInternal(ieutThreadData_t *pThreadData,
                                                       ismEngine_Session_t *pSession,
                                                       bool fInline)
{
    uint32_t cbcount = __sync_sub_and_fetch(&pSession->ActiveCallbacks, 1);
    if (cbcount == 0)
    {
        uint32_t rc = ism_engine_lockSession(pSession);
        if (rc == OK)
        {
            pSession->fIsDeliveryStopping = false;
            ism_engine_unlockSession(pSession);
        }

        releaseSessionReference(pThreadData, pSession, fInline);
    }
}

//****************************************************************************
/// @brief  Resume Consumer Message Delivery
///
/// Resumes message delivery for an individual consumer after it has been disabled
/// by returning false from a message delivery callback.
///
/// @param[in]     hConsumer        Consumer handle
/// @param[in]     options          options from ismENGINE_RESUME_DELIVERY_OPTION_*
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_resumeMessageDelivery(
    ismEngine_ConsumerHandle_t      hConsumer,
    uint32_t                        options,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Consumer_t *pConsumer = (ismEngine_Consumer_t *)hConsumer;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pConsumer->pSession->pClient);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hConsumer,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hConsumer %p)\n", __func__, pConsumer);

    assert(pConsumer != NULL);
    ismEngine_CheckStructId(pConsumer->StrucId, ismENGINE_CONSUMER_STRUCID, ieutPROBE_001);
    ismEngine_Session_t *pSession = pConsumer->pSession;

    assert(pSession != NULL);
    rc = ism_engine_lockSession(pSession);
    if (UNLIKELY(rc != OK))
    {
        goto mod_exit;
    }

    if (!pSession->fIsDeliveryStarted || pSession->fIsDeliveryStopping)
    {
        rc = ISMRC_WaiterDisabled;
        ism_engine_unlockSession(pSession);
        goto mod_exit;
    }
    if(pSession->fIsDestroyed || pConsumer->fIsDestroyed)
    {
        rc = ISMRC_Destroyed;
        ism_engine_unlockSession(pSession);
        goto mod_exit;
    }
    //Speculatively assume we can enable it...
    __sync_fetch_and_add(&pSession->ActiveCallbacks, 1);
    acquireConsumerReference(pConsumer); //An enabled waiter counts as a reference

    rc = ieq_enableWaiter(pThreadData, pConsumer->queueHandle, pConsumer, IEQ_ENABLE_OPTION_DELIVER_LATER);

    if (rc == ISMRC_WaiterEnabled)
    {
        //It was already enabled...reduce the counts we speculatively increased
        releaseConsumerReference(pThreadData, pConsumer, false);
        DEBUG_ONLY uint32_t oldvalue =  __sync_fetch_and_sub(&pSession->ActiveCallbacks, 1);
        assert(oldvalue > 0);
    }
    else if(rc == ISMRC_DisableWaiterCancel)
    {
        //We've cancelled a disable, the disable callbacks and usecounts reductions
        //etc... will still happen (but the waiter will just stay enabled) so we do need our
        //speculative count increases
        rc = OK;
    }
    else if (rc == ISMRC_WaiterInvalid)
    {
        //The waiter status is not valid... reduce the counts we speculatively increased
        releaseConsumerReference(pThreadData, pConsumer, false);
        DEBUG_ONLY uint32_t oldvalue = __sync_fetch_and_sub(&pSession->ActiveCallbacks, 1);
        assert(oldvalue > 0);
    }

    //acquire a reference so the consumer doesn't disappear before our checkWaiters call...
    acquireConsumerReference(pConsumer); //An enabled waiter counts as a reference

    ism_engine_unlockSession(pSession);

    ieq_checkWaiters(pThreadData, pConsumer->queueHandle, NULL, NULL);
    releaseConsumerReference(pThreadData, pConsumer, false);

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Create Local Transaction
///
/// Creates a local transaction associated with the provided
/// session handle. The transaction can be used to group operations
/// into an atomic unit of work.
///
/// @param[in]     hSession         Session handle
/// @param[out]    phTran           Returned transaction handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createLocalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t * phTran,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession ? pSession->pClient : NULL);
    ismEngine_Transaction_t *pTran = NULL;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p)\n", __func__, hSession);

    ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
                            {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineCaller,      pContext,   contextLength,     NULL, {.externalFn = pCallbackFn }}};
    ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID,
                                       (pSession == NULL) ? NULL :pSession->pClient,
                                       IEAD_MAXARRAYENTRIES, 1, 0, true,  0, 0, asyncArray};

    rc = ietr_createLocal(pThreadData, (ismEngine_Session_t *)hSession, true, false, &asyncData, &pTran);

    if (rc == OK)
    {
        *phTran = pTran;
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, hTran=%p\n", __func__, rc, pTran);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Create Global Transaction
///
/// Creates a global transaction associated with the provided
/// session handle. The transaction can be used to group operations
/// into an atomic unit of work. A global transaction is committed using
/// a 2-phase protocol which means it must be prepared before it can
/// by committed.
///
/// @param[in]     hSession         Session handle
/// @param[in]     pXID             Global Transaction identifier
/// @param[in]     options          ismENGINE_CREATE_TRANSACTION_OPTION values
/// @param[out]    phTran           Returned transaction handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark If options is ismENGINE_CREATE_TRANSACTION_DEFAULT is specified,
/// the requested transaction will be resumed if it already exists, and created
/// if it does not.
///
/// In order to implement XA functionality, more restrictive options can be specified
/// ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS will cause the request to fail if
/// a transaction with the requested XID already exists.
///
/// ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMRESUME will resume a transaction
/// that was previously suspended on this session. If the transaction with the
/// requesed XID does not exist the request will fail.
///
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createGlobalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t                     * pXID,
    uint32_t                        options,
    ismEngine_TransactionHandle_t * phTran,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Transaction_t *pTran = NULL;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, options=0x%08x)\n", __func__,
               hSession, options);

    // Validate the XID passed
    uint32_t XIDDataLen =  pXID->gtrid_length + pXID->bqual_length;

    if (pXID->formatID == -1)
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
        goto mod_exit;
    }

    if (/* (XIDDataLen == 0) || */(XIDDataLen > XID_DATASIZE))
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
        goto mod_exit;
    }

    ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
                            {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineCaller,      pContext,   contextLength,     NULL, {.externalFn = pCallbackFn }}};
    ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID,
                                      (pSession == NULL) ? NULL :pSession->pClient,
                                       IEAD_MAXARRAYENTRIES, 1, 0, true,  0, 0, asyncArray};

    rc = ietr_createGlobal(pThreadData, (ismEngine_Session_t *)hSession, pXID, options, &asyncData, &pTran);

    if (rc == OK)
    {
        *phTran = pTran;
    }

mod_exit:
    ieutTRACEL(pThreadData, pTran,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, hTran=%p\n", __func__, rc, pTran);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Dissociate a transaction from the session
///
/// @param[in]     hSession         Session handle
/// @param[in]     hTran            Transaction handle
/// @param[in]     options          ismENGINE_END_TRANSACTION_OPTION values
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark If ismENGINE_END_TRANSACTION_OPTION_DEFAULT is specified, this
/// removes the association between the session and the transaction - the
/// transaction does stay associated with the client state.
///
/// In order to implement XA functionality, other options can be specified.
///
/// ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS is a synonym for DEFAULT,
/// the association between the session and the transaction is removed, but
/// the transaction stays associated with the client state.
///
/// If ismENGINE_END_TRANSACTION_OPTION_XA_TMFAIL is specified, the transaction
/// is marked as rollback only, and the association between the session and the
/// transaction is removed.
///
/// Only one of TMSUCCESS and TMFAIL should be specified.
///
/// If ismENGINE_END_TRANSACTION_OPTION_XA_TMSUSPEND is specified, the transaction's
/// association with the session is suspended. The transaction can be resumed by
/// a call to ism_engine_createGlobalTransaction on the same session, specifying
/// ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMRESUME.
///
///***************************************************************************
uint32_t WARN_CHECKRC ism_engine_endTransaction( ismEngine_SessionHandle_t hSession
                                  , ismEngine_TransactionHandle_t hTran
                                  , uint32_t options
                                  , void *pContext
                                  , size_t contextLength
                                  , ismEngine_CompletionCallback_t pCallbackFn)
{
    uint32_t rc = OK;
    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)hTran;
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);

    ieutTRACEL(pThreadData, hTran, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p hTran %p options 0x%08x)\n", __func__,
               hSession, hTran, options);

    if ((pTran->pSession == NULL) ||
        ((ismEngine_SessionHandle_t)pTran->pSession != hSession))
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Requested to mark the transaction as failed
    if ((options & ismENGINE_END_TRANSACTION_OPTION_XA_TMFAIL) != 0)
    {
        // Must not specify both TMFAIL and TMSUCCESS
        assert((options & ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS) == 0);

        ietr_markRollbackOnly(pThreadData, pTran);
    }

    // If we are just suspending the transaction, mark it as suspended
    if ((options & ismENGINE_END_TRANSACTION_OPTION_XA_TMSUSPEND) != 0)
    {
        pTran->TranFlags |= ietrTRAN_FLAG_SUSPENDED;
    }
    // Remove the transaction's association with this session
    else
    {
        ietr_unlinkTranSession(pThreadData, pTran);

        // If the transaction is not prepared, maintain an association with
        // the client state.
        if (pTran->TranState != ismTRANSACTION_STATE_PREPARED)
        {
            iecs_linkTransaction(pThreadData, pSession->pClient, pTran);
        }
    }

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Prepare Transaction
///
/// Moves a global transaction into prepared state
///
/// @param[in]     hSession         Session handle
/// @param[in]     hTran            Transaction handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_prepareTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)hTran;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, hTran %p)\n", __func__, hSession, hTran);

    if ((pTran->TranFlags & ietrTRAN_FLAG_PERSISTENT) == ietrTRAN_FLAG_PERSISTENT)
    {
        ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
                        {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineCaller,      pContext,   contextLength,     NULL, {.externalFn = pCallbackFn }}};
        ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID, pSession->pClient, IEAD_MAXARRAYENTRIES, 1, 0, true,  0, 0, asyncArray};

        rc = ietr_prepare(pThreadData, pTran, pSession, &asyncData);
    }
    else
    {
        rc = ietr_prepare(pThreadData, pTran, pSession, NULL);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


static void completePrepareGlobalTransaction(
                    ieutThreadData_t *pThreadData
                  , ismEngine_Transaction_t *pTran)
{
    ietr_releaseTransaction(pThreadData, pTran);
}

//Used by both ism_engine_confirmMessageDelivery and ism_engine_confirmMessageDeliveryBatch to do a final reduce of the usecount
static int32_t asyncPrepareGlobalTransaction(
                       ieutThreadData_t *pThreadData
                     , int32_t  rc
                     , ismEngine_AsyncData_t *asyncInfo
                     , ismEngine_AsyncDataEntry_t *asyncEntry)
{
    assert(asyncEntry->Type == EnginePrepareGlobal);
    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)(asyncEntry->Handle);

    ieutTRACEL(pThreadData, pTran, ENGINE_CEI_TRACE, FUNCTION_IDENT "pTran %p\n", __func__,
                 pTran);

    //Don't need this callback to be run again if we go async
    iead_popAsyncCallback( asyncInfo, 0);

    completePrepareGlobalTransaction(pThreadData, pTran);

    return OK;
}

//****************************************************************************
/// @internal
///
/// @brief Prepare the global transaction with specified XID.
///
/// Upon successful completion we guarantee to be able to commit the global
/// transaction.
///
/// @param[in]     hSession         Session handle
/// @param[in]     pXID             XID of global transaction
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_prepareGlobalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t                      *pXID,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Transaction_t *pTran = NULL;

    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p)\n", __func__, hSession);

    int32_t rc = ietr_findGlobalTransaction(pThreadData, pXID, &pTran);

    if (rc == OK)
    {
        assert(pTran != NULL);

        if ((pTran->TranFlags & ietrTRAN_FLAG_PERSISTENT) == ietrTRAN_FLAG_PERSISTENT)
        {
            // NOTE: Call engine caller AFTER asyncPrepareGlobalTransaction
            ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
                {ismENGINE_ASYNCDATAENTRY_STRUCID, EnginePrepareGlobal,   NULL, 0, pTran, {.internalFn = asyncPrepareGlobalTransaction } },
                {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineCaller,      pContext,   contextLength,     NULL, {.externalFn = pCallbackFn }}};
            ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID, pSession->pClient, IEAD_MAXARRAYENTRIES, 2, 0, true,  0, 0, asyncArray};

            rc = ietr_prepare(pThreadData, pTran, pSession, &asyncData);


            if (rc == ISMRC_AsyncCompletion)
            {
                //We don't want to call completePrepareGlobalTransaction()... done in the callback
                goto mod_exit;
            }
        }
        else
        {
            rc = ietr_prepare(pThreadData, pTran, pSession, NULL);
        }
        completePrepareGlobalTransaction(pThreadData, pTran);
    }
mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "pTran=%p, rc=%d\n", __func__, pTran, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


/// Callback data struct used if ism_engine_commitTransaction goes async
typedef struct tag_inflightCommit_t {
    size_t                          contextLength;
    ismEngine_CompletionCallback_t  pCallbackFn;
} inflightCommit_t;

///Callback if ism_engine_commitTransaction goes async
static void ism_engine_completeCommitTransaction(
    ieutThreadData_t                *pThreadData,
    int32_t                         retcode,
    void *                          pContext)
{

    ieutTRACEL(pThreadData, retcode, ENGINE_FNC_TRACE, FUNCTION_IDENT " rc %d\n", __func__,
                    retcode);

    inflightCommit_t *pAsyncCommitData = (inflightCommit_t *)pContext;

    if (pAsyncCommitData->pCallbackFn != NULL)
    {
        void *callerContext = NULL;

        if (pAsyncCommitData->contextLength > 0)
        {
            callerContext = (void *)(pAsyncCommitData+1);
        }

        pAsyncCommitData->pCallbackFn(retcode, NULL, callerContext);
    }
}


//****************************************************************************
/// @internal
///
/// @brief  Commit Transaction
///
/// Commits the local or global transaction with specified handle.
///
/// For local transactions the hSession parameter must match the hSession
/// which created the transaction.
///
/// For global transactions associated with a session, the hSession
/// parameter must match the hSession to which the transaction is
/// associated, otherwise the hSession parameter is unused.
///
/// Once committed, the transaction is freed unless it was previously
/// heuristically completed, in which case a call to
/// ism_engine_forgetGlobalTransaction is required.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hTran            Transaction handle
/// @param[in]     options          ismENGINE_COMMIT_TRANSACTION_OPTION values
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion, an information return code of
/// ISMRC_HeuristicallyCommitted or ISMRC_HeuristicallyRolledBack indicating
/// that the transaction was previously heuristically completed, or another
/// ISMRC_ value on failure.
///
/// @remark If options is set to ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT
/// the transaction is committed unless it is a global transaction and is not
/// prepared.
///
/// In order to implement XA functionality, other options can be specified
/// to restrict the behaviour.
///
/// ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS is a synonym for DEFAULT.
///
/// If ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMONEPHASE is specified, the
/// transaction is committed even if it is an un-prepared global transaction.
///
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_commitTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    uint32_t                        options,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession ? pSession->pClient : NULL);
    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)hTran;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hTran, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, hTran %p, options=0x%08x)\n", __func__,
               hSession, hTran, options);

    rc = ietr_checkForHeuristicCompletion(pTran);

    // Not already completed - request a commit
    if (rc == OK)
    {
        ietrAsyncTransactionDataHandle_t asyncTranHandle = NULL;

        if ((pTran->TranFlags & ietrTRAN_FLAG_PERSISTENT) == ietrTRAN_FLAG_PERSISTENT)
        {
            //We might well go async... get ready
            asyncTranHandle = ietr_allocateAsyncTransactionData(
                                    pThreadData
                                  , pTran
                                  , true
                                  , sizeof(inflightCommit_t) + contextLength);

            if (asyncTranHandle != NULL)
            {
                inflightCommit_t *asyncCommitData = (inflightCommit_t *)
                                                                      ietr_getCustomDataPtr(asyncTranHandle);
                asyncCommitData->pCallbackFn = pCallbackFn;
                asyncCommitData->contextLength = contextLength;

                if (contextLength > 0)
                {
                    void *memForContext = (void *)(asyncCommitData+1);

                    memcpy(memForContext, pContext, contextLength);
                }
            }
            else
            {
                //No memory but we don't want to fail a commit... sync commit
            }
        }
        rc = ietr_commit(pThreadData, pTran, options, pSession, asyncTranHandle,
                ((asyncTranHandle != NULL)?  ism_engine_completeCommitTransaction : NULL));
    }
    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}
/// Callback data struct used if ism_engine_commitGlobalTransaction goes async
typedef struct tag_inflightGlobalCommit_t {
    size_t                          contextLength;
    ismEngine_CompletionCallback_t  pCallbackFn;
    ismEngine_Transaction_t         *pTran;
} inflightGlobalCommit_t;

///Callback if ism_engine_commitGlobalTransaction goes async
static void ism_engine_completeCommitGlobalTransaction(
    ieutThreadData_t                *pThreadData,
    int32_t                         retcode,
    void *                          pContext)
{

    ieutTRACEL(pThreadData, retcode, ENGINE_FNC_TRACE, FUNCTION_IDENT " rc %d\n", __func__,
                    retcode);

    inflightGlobalCommit_t *pAsyncCommitData = (inflightGlobalCommit_t *)pContext;

    // We should release the transaction before calling the callback
    ietr_releaseTransaction(pThreadData, pAsyncCommitData->pTran);

    // Call the callback
    if (pAsyncCommitData->pCallbackFn != NULL)
    {
        void *callerContext = NULL;

        if (pAsyncCommitData->contextLength > 0)
        {
            callerContext = (void *)(pAsyncCommitData+1);
        }

        pAsyncCommitData->pCallbackFn(retcode, NULL, callerContext);
    }
}

//****************************************************************************
/// @internal
///
/// @brief Commit the global transaction with specified XID.
///
/// Commits the global transaction with specified XID.
///
/// Once committed, the transaction is freed unless it was previously
/// heuristically completed, in which case a call to
/// ism_engine_forgetGlobalTransaction is required.
///
/// @param[in]     hSession         Session handle
/// @param[in]     pXID             XID of the global transaction
/// @param[in]     options          ismENGINE_COMMIT_TRANSACTION_OPTION values
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion, an information return code of
/// ISMRC_HeuristicallyCommitted or ISMRC_HeuristicallyRolledBack indicating
/// that the transaction was previously heuristically completed, or another
/// ISMRC_ value on failure.
///
/// @see ism_engine_commitTransaction
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_commitGlobalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t                      *pXID,
    uint32_t                        options,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Transaction_t *pTran = NULL;

    ieutTRACEL(pThreadData, pTran, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, options=0x%08x)\n", __func__,
               hSession, options);

    int32_t rc = ietr_findGlobalTransaction(pThreadData, pXID, &pTran);

    if (rc == OK)
    {
        rc = ietr_checkForHeuristicCompletion(pTran);

        // Not already completed - request a commit
        if (rc == OK)
        {
            ietrAsyncTransactionDataHandle_t asyncTranHandle = NULL;

            if ((pTran->TranFlags & ietrTRAN_FLAG_PERSISTENT) == ietrTRAN_FLAG_PERSISTENT)
            {
                //We might well go async... get ready
                asyncTranHandle = ietr_allocateAsyncTransactionData(
                                        pThreadData
                                      , pTran
                                      , true
                                      , sizeof(inflightGlobalCommit_t) + contextLength);

                if (asyncTranHandle != NULL)
                {
                    inflightGlobalCommit_t *asyncCommitData = (inflightGlobalCommit_t *)
                                                             ietr_getCustomDataPtr(asyncTranHandle);
                    asyncCommitData->pCallbackFn = pCallbackFn;
                    asyncCommitData->contextLength = contextLength;
                    asyncCommitData->pTran = pTran;

                    if (contextLength > 0)
                    {
                        void *memForContext = (void *)(asyncCommitData+1);

                        memcpy(memForContext, pContext, contextLength);
                    }
                }
                else
                {
                    //No memory but we don't want to fail a commit... sync commit
                }
            }
            rc = ietr_commit(pThreadData, pTran, options, pSession, asyncTranHandle,
                    ((asyncTranHandle != NULL)?  ism_engine_completeCommitGlobalTransaction : NULL));

            if (rc == ISMRC_AsyncCompletion)
            {
                //we want to skip the releaseTransaction, that'll be done in the callback
                goto mod_exit;
            }
        }

        ietr_releaseTransaction(pThreadData, pTran);
    }
mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "pTran=%p, rc=%d\n", __func__, pTran, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Roll Back Transaction
///
/// Rolls back the transaction with specified handle.
///
/// For local transactions the hSession parameter must match the hSession
/// which created the transaction.
///
/// For global transactions associated with a session, the hSession
/// parameter must match the hSession to which the transaction is
/// associated, otherwise the hSession parameter is unused.
///
/// Once rolled back, the transaction is freed unless it was previously
/// heuristically completed, in which case a call to
/// ism_engine_forgetGlobalTransaction is required.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hTran            Transaction handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion, an information return code of
/// ISMRC_HeuristicallyCommitted or ISMRC_HeuristicallyRolledBack indicating
/// that the transaction was previously heuristically completed, or another
/// ISMRC_ value on failure.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_rollbackTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession ? pSession->pClient : NULL);
    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)hTran;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, hTran %p)\n", __func__, hSession, hTran);

    rc = ietr_checkForHeuristicCompletion(pTran);

    // Not already completed - request a rollback
    if (rc == OK) rc = ietr_rollback(pThreadData, pTran, pSession, IETR_ROLLBACK_OPTIONS_NONE);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Roll Back Global Transaction
///
/// Rolls back the global transaction with specified XID.
///
/// Once rolled back, the transaction is freed unless it was previously
/// heuristically completed, in which case a call to
/// ism_engine_forgetGlobalTransaction is required.
///
/// @param[in]     hSession         Session handle
/// @param[in]     pXID             XID of global transaction
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion, an information return code of
/// ISMRC_HeuristicallyCommitted or ISMRC_HeuristicallyRolledBack indicating
/// that the transaction was previously heuristically completed, or another
/// ISMRC_ value on failure.
///
/// @see ism_engine_rollbackTransaction
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_rollbackGlobalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t                      *pXID,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Transaction_t *pTran = NULL;

    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p)\n", __func__, hSession);

    int32_t rc = ietr_findGlobalTransaction(pThreadData, pXID, &pTran);

    if (rc == OK)
    {
        rc = ietr_checkForHeuristicCompletion(pTran);

        // Not already completed - request a rollback
        if (rc == OK) rc = ietr_rollback(pThreadData, pTran, pSession, IETR_ROLLBACK_OPTIONS_NONE);

        ietr_releaseTransaction(pThreadData, pTran);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "pTran=%p, rc=%d\n", __func__, pTran, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

static void finishCompleteGlobalTransaction(ieutThreadData_t *pThreadData,
                                            ismEngine_Transaction_t *pTran)
{
    // The transaction has been committed or rolled back, and could potentially stay in this
    // heuristically complete state for some time (until it is forgotten).

    // In theory, the SLEs and lock scope could still be referred to however, in practice
    // they are not and so the memory that they are consuming can be released.

    // BUT the mempool for the transaction may also contain async transaction data which
    // we cannot release - but the good news is that this is allocated in the mempool's
    // reserved area which is not cleared by iemp_clearMemPool.

    // So we perform the following early (before the transaction useCount hits zero) to
    // reduce the memory footprint of the transaction before it is forgotten.

    // We can only do this because we know the useCount is > 0 (because we'll release it!)
    assert(pTran->useCount > 0);

    while (pTran->pSoftLogHead != NULL)
    {
        ietrSLE_Header_t *pSLE = pTran->pSoftLogHead;
        pTran->pSoftLogHead = pSLE->pNext;
        if ((pSLE->Type & ietrSLE_PREALLOCATED_MASK) == ietrSLE_PREALLOCATED_MASK)
        {
            iemem_freeStruct(pThreadData, iemem_localTransactions, pSLE, pSLE->StrucId);
        }
    }

    ielm_freeLockScope(pThreadData, &(pTran->hLockScope));

    // Clear the mempool out, leaving only the first page including reseved memory (which we keep).
    iemp_clearMemPool(pThreadData, pTran->hTranMemPool, true);

    ietr_releaseTransaction(pThreadData, pTran);
}

static void asyncFinishCompleteGlobalTransaction(
    ieutThreadData_t        *pThreadData,
    int32_t                  retcode,
    void *                   pContext)
{
    ietrAsyncHeuristicCommitData_t *pAsyncData = (ietrAsyncHeuristicCommitData_t *)pContext;
    assert(retcode == OK);

    ismEngine_CheckStructId( pAsyncData->StrucId
                           , ietrASYNCHEURISTICCOMMIT_STRUCID
                           , ieutPROBE_001);

    // Call the engine caller if they are non-null
    if (pAsyncData->pCallbackFn != NULL)
    {
        pAsyncData->pCallbackFn(retcode, NULL, pAsyncData->engineCallerContext);
    }

    finishCompleteGlobalTransaction(pThreadData, pAsyncData->pTran);
}

///***************************************************************************
/// @internal
///
/// @brief  Complete a prepared transaction with a "heuristic outcome"
///
/// Completes the prepared global transaction with a supplied outcome.
///
/// @param[in]     pXID             XID of global transaction
/// @param[in]     completionType   The type of heuristic outcome with which to
///                                 complete the transaction
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark A transaction that has been completed in this way is remembered
/// since it is assumed that a transaction manager somewhere still has knowledge
/// of the transaction and we should know of it's existence.
///
/// In order to forget the transaction, ism_engine_forgetGlobalTransaction must
/// be called.
///
/// @remark The operation may complete synchronously or asynchronously. If it
/// completes synchronously, the return code indicates the completion
/// status. If the operation completes asynchronously, the transaction
/// synchronously moves into "rolling back" state and further operations cannot
/// be added to it until the transaction has completed.
///
/// If the operation completes asynchronously, the return code from
/// this function will be ISMRC_AsyncCompletion. The actual
/// completion of the operation will be signalled by a call to the
/// operation-completion callback, if one is provided. When the
/// operation becomes asynchronous, a copy of the context will be
/// made into memory owned by the Engine. This will be freed upon
/// return from the operation-completion callback. Because the
/// completion is asynchronous, the call to the operation-completion
/// callback might occur before this function returns.
///
/// @see ism_engine_prepareGlobalTransaction
/// @see ism_engine_forgetGlobalTransaction
///***************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_completeGlobalTransaction(
    ism_xid_t *                     pXID,
    ismTransactionCompletionType_t  completionType,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ismEngine_Transaction_t *pTran = NULL;

    ieutTRACEL(pThreadData, pTran, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(completionType %d)\n", __func__,
               (int32_t)completionType);

    int32_t rc = ietr_findGlobalTransaction(pThreadData, pXID, &pTran);

    assert((completionType == ismTRANSACTION_COMPLETION_TYPE_COMMIT) ||
           (completionType == ismTRANSACTION_COMPLETION_TYPE_ROLLBACK));

    if (rc == OK)
    {
        rc = ietr_checkForHeuristicCompletion(pTran);

        if (rc == OK)
        {
            if (completionType == ismTRANSACTION_COMPLETION_TYPE_COMMIT)
            {
                ietrAsyncHeuristicCommitData_t asyncData = {
                         ietrASYNCHEURISTICCOMMIT_STRUCID
                       , pTran
                       , pContext
                       , contextLength
                       , pCallbackFn };

                rc = ietr_complete( pThreadData
                                  , pTran
                                  , ismTRANSACTION_STATE_HEURISTIC_COMMIT
                                  , asyncFinishCompleteGlobalTransaction
                                  , &asyncData);

                if (rc == ISMRC_AsyncCompletion)
                {
                    goto mod_exit;
                }

                if (rc == OK)
                {
                    finishCompleteGlobalTransaction(pThreadData, pTran);
                }
                else
                {
                    ietr_releaseTransaction(pThreadData, pTran);
                }
            }
            else
            {
                //Rollback can not yet go async
                rc = ietr_complete( pThreadData
                                  , pTran
                                  , ismTRANSACTION_STATE_HEURISTIC_ROLLBACK
                                  , NULL
                                  , NULL);
                assert (rc != ISMRC_AsyncCompletion);

                ietr_releaseTransaction(pThreadData, pTran);
            }
        }
        else
        {
            ietr_releaseTransaction(pThreadData, pTran);
        }
    }
mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "pTran=%p, rc=%d\n", __func__, pTran, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

static void completeForgetGlobalTransaction(ieutThreadData_t *pThreadData,
                                            ismEngine_Transaction_t *pTran)
{
    ietr_releaseTransaction(pThreadData, pTran);
}

static int32_t asyncForgetGlobalTransaction(
        ieutThreadData_t               *pThreadData,
        int32_t                         retcode,
        ismEngine_AsyncData_t          *asyncInfo,
        ismEngine_AsyncDataEntry_t     *context)
{
    assert(context->Type == EngineTranForget);
    assert(retcode == OK);

    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)(context->Handle);
    ismEngine_CheckStructId(pTran->StrucId, ismENGINE_TRANSACTION_STRUCID, ieutPROBE_001);

    ieutTRACEL(pThreadData, pTran,  ENGINE_CEI_TRACE, FUNCTION_IDENT "pTran=%p\n", __func__, pTran);

    //Pop this entry off the stack now so it doesn't get called again
    iead_popAsyncCallback( asyncInfo, context->DataLen);

    completeForgetGlobalTransaction(pThreadData, pTran);

    return retcode;
}

///***************************************************************************
/// @internal
///
/// @brief  Forget a heuristically completed transaction
///
/// Forget about a transaction which had previously had a heuristic outcome
/// delivered to it via a call to ism_engine_completeGlobalTransaction.
///
/// @param[in]     pXID             XID of global transaction
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark When a transaction has been heuristically completed, we need to
/// remember it for a potential call from a transaction manager referencing it.
///
/// To finally forget the transaction ism_engine_forgetGlobalTransaction must
/// be called.
///
/// @remark The operation may complete synchronously or asynchronously. If it
/// completes synchronously, the return code indicates the completion
/// status. If the operation completes asynchronously, the transaction
/// synchronously moves into "rolling back" state and further operations cannot
/// be added to it until the transaction has completed.
///
/// If the operation completes asynchronously, the return code from
/// this function will be ISMRC_AsyncCompletion. The actual
/// completion of the operation will be signalled by a call to the
/// operation-completion callback, if one is provided. When the
/// operation becomes asynchronous, a copy of the context will be
/// made into memory owned by the Engine. This will be freed upon
/// return from the operation-completion callback. Because the
/// completion is asynchronous, the call to the operation-completion
/// callback might occur before this function returns.
///
/// @see ism_engine_completeGlobalTransaction
///***************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_forgetGlobalTransaction(
    ism_xid_t *                     pXID,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ismEngine_Transaction_t *pTran = NULL;

    ieutTRACEL(pThreadData, pXID,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    int32_t rc = ietr_findGlobalTransaction(pThreadData, pXID, &pTran);

    if (rc == OK)
    {
        // NOTE: Calling the engine caller AFTER we call asyncForgetGlobalTransaction
        ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
                {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineCaller,      pContext,   contextLength,     NULL, {.externalFn = pCallbackFn }},
                {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineTranForget,  NULL, 0, pTran, {.internalFn = asyncForgetGlobalTransaction } }};
        ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID, NULL, IEAD_MAXARRAYENTRIES, 2, 0, true,  0, 0, asyncArray};

        rc = ietr_forget(pThreadData, pTran, &asyncData);

        if (rc == ISMRC_AsyncCompletion)
        {
            goto mod_exit;
        }

        completeForgetGlobalTransaction(pThreadData, pTran);
    }

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "pTran=%p, rc=%d\n", __func__, pTran, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Recover list of prepared or heuristically completed transactions
///
/// Return to the caller a list of prepared or heuristically completed transactions.
///
/// This function is equivalent to the xa_recover verb, and returns a list of
/// the prepared or heuristically completed transactions.
///
/// @param[in]     hSession         Session handle
/// @param[in]     pXIDArray        Array in which XIDs are placed
/// @param[in]     arraySize        Maximum number of XIDs to be return
/// @param[in]     rmId             resource manager id (ignored)
/// @param[in]     flags            ismENGINE_XARECOVER_OPTION_ values - see below
///
/// @return Number of XIDs in the array.
///
/// Where the number of prepared transactions exceeds te size of the buffer
/// provided, as defined in the arraySize parameter, then a subsequent call to
/// ism_engine_XARecover will return the next set of prepared transactions.
/// The flags parameter may be used to control this operation:
///   ismENGINE_XARECOVER_OPTION_XA_TMSTARTRSCAN causes the prepared transaction
///   list to be reset and return the XIDs from the start,
///   ismENGINE_XARECOVER_OPTION_XA_TMNOFLAGS continues an existing scan, if one
///   is not already in progress a scan is started,
///   ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN causes the cursor to be reset upon
///   completing the function call,
///
/// The return value is the number of XID's returned, if this value is less-
/// than or equal to the arraySize parameter then the TMENDRSCAN flag is
/// implied and there is no need for a further call to ism_engine_XARecover to
/// complete the scan.
///
/// Note: This function call must not be called simultaneously from 2 threads
/// concurrently for the same session.
//****************************************************************************
XAPI uint32_t WARN_CHECKRC ism_engine_XARecover( ismEngine_SessionHandle_t hSession
                                  , ism_xid_t *pXIDArray
                                  , uint32_t arraySize
                                  , uint32_t rmId
                                  , uint32_t flags )
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    uint32_t rc;

    ieutTRACEL(pThreadData, hSession, ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    rc = ietr_XARecover( pThreadData
                       , hSession
                       , pXIDArray
                       , arraySize
                       , rmId
                       , flags );


    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "Number of XIDs returned=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Create Message Producer
///
/// Creates a message producer for a destination. This is intended
/// for use when several messages will be put to the same destination.
///
/// @param[in]     hSession         Session handle
/// @param[in]     destinationType  Destination type
/// @param[in]     pDestinationName Destination name
/// @param[out]    phProducer       Returned producer handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createProducer(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_ProducerHandle_t *    phProducer,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    assert(pSession != NULL);
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Destination_t *pDestination = NULL;
    ismEngine_Producer_t *pProducer = NULL;
    iepiPolicyInfo_t *pValidatedPolicyInfo = NULL;
    bool fProducerRegistered = false;
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = pSession->pClient->resourceSet;

    ieutTRACEL(pThreadData, hSession, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, destinationType %d, pDestinationName '%s')\n", __func__,
               hSession, destinationType, (pDestinationName != NULL) ? pDestinationName : "(nil)");

    ismEngine_CheckStructId(pSession->StrucId, ismENGINE_SESSION_STRUCID, ieutPROBE_001);

    // If creating a producer on a topic, the topic string must not contain wildcards
    if (destinationType == ismDESTINATION_TOPIC)
    {
        if (!iett_validateTopicString(pThreadData, pDestinationName, iettVALIDATE_FOR_PUBLISH))
        {
            rc = ISMRC_DestNotValid;
            ism_common_setErrorData(rc, "%s", pDestinationName);
            goto mod_exit;
        }
    }

    //Check with security before we do anything other than trace the request
    if (destinationType == ismDESTINATION_TOPIC)
    {
        rc = ismEngine_security_validate_policy_func(pSession->pClient->pSecContext,
                                                     ismSEC_AUTH_TOPIC,
                                                     pDestinationName,
                                                     ismSEC_AUTH_ACTION_PUBLISH,
                                                     ISM_CONFIG_COMP_ENGINE,
                                                     (void **)&pValidatedPolicyInfo);
    }
    else
    {
        bool isTemporary;

        assert(destinationType == ismDESTINATION_QUEUE);

        rc = ieqn_isTemporaryQueue(pDestinationName, &isTemporary, NULL);

        // We do not check security for temporary queues
        if (rc == OK && isTemporary == false)
        {
            rc = ismEngine_security_validate_policy_func(pSession->pClient->pSecContext,
                                                         ismSEC_AUTH_QUEUE,
                                                         pDestinationName,
                                                         ismSEC_AUTH_ACTION_SEND,
                                                         ISM_CONFIG_COMP_ENGINE,
                                                         (void **)&pValidatedPolicyInfo);
        }
    }

    if (rc != OK) goto mod_exit;

    iere_primeThreadCache(pThreadData, resourceSet);
    rc = ieds_create_newDestination(pThreadData,
                                    resourceSet,
                                    destinationType, pDestinationName,
                                    &pDestination);
    if (rc == OK)
    {
        assert(pDestination != NULL);
        pProducer = (ismEngine_Producer_t *)iere_malloc(pThreadData,
                                                        resourceSet,
                                                        IEMEM_PROBE(iemem_externalObjs, 3), sizeof(ismEngine_Producer_t));
        if (pProducer != NULL)
        {
            ismEngine_SetStructId(pProducer->StrucId, ismENGINE_PRODUCER_STRUCID);
            pProducer->pSession = hSession;
            pProducer->pDestination = pDestination;
            pProducer->pPrev = NULL;
            pProducer->pNext = NULL;
            pProducer->fIsDestroyed = false;
            pProducer->fRecursiveDestroy = false;
            pProducer->UseCount = 1;
            pProducer->pPendingDestroyContext = NULL;
            pProducer->pPendingDestroyCallbackFn = NULL;
            pProducer->engineObject = NULL;
            pProducer->queueHandle = NULL;

            // The policy can come back with a NULL context for __MQConnectiivty requests and unit tests,
            // so if we are authorized, but got no context, we use the default policy.
            //
            // NOTE: We can reference the default because we are not going to override any values, if we
            //       were the producer would have to make a copy of the default with:
            //
            //           rc = iepi_createPolicyInfo(pThreadData, NULL, false, NULL, &pProducer->pPolicyInfo);
            //
            if (pValidatedPolicyInfo == NULL)
            {
                pProducer->pPolicyInfo = iepi_getDefaultPolicyInfo(true);
            }
            else
            {
                iepi_acquirePolicyInfoReference(pValidatedPolicyInfo);

                pProducer->pPolicyInfo = pValidatedPolicyInfo;
            }

            assert(pProducer->pPolicyInfo != NULL);

            if (destinationType == ismDESTINATION_QUEUE)
            {
                rc = ieqn_openQueue(pThreadData,
                                    pSession->pClient,
                                    pDestinationName,
                                    NULL, // No properties
                                    pProducer);

                if (rc == OK)
                {
                    assert(pProducer->engineObject != NULL);
                    assert(pProducer->queueHandle != NULL);

                    fProducerRegistered = true;
                }
            }
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }
    }

    // Lock the session and add the producer to the list of producers
    if (rc == OK)
    {
        rc = ism_engine_lockSession(pSession);
        if (rc == OK)
        {
            if (pSession->fIsDestroyed)
            {
                ism_engine_unlockSession(pSession);
                rc = ISMRC_Destroyed;
                ism_common_setError(rc);
                goto mod_exit;
            }

            if (pSession->pProducerHead != NULL)
            {
                pProducer->pNext = pSession->pProducerHead;
                pSession->pProducerHead->pPrev = pProducer;
                pSession->pProducerHead = pProducer;
            }
            else
            {
                pSession->pProducerHead = pProducer;
                pSession->pProducerTail = pProducer;
            }

            __sync_fetch_and_add(&pSession->UseCount, 1);

            ism_engine_unlockSession(pSession);
        }
    }

mod_exit:
    if (rc == OK)
    {
        *phProducer = pProducer;
    }
    else
    {
        if (pProducer != NULL)
        {
            if (pProducer->pPolicyInfo != NULL)
            {
                // Release our use of the messaging policy info
                iepi_releasePolicyInfo(pThreadData, pProducer->pPolicyInfo);
            }

            if (fProducerRegistered)
            {
                ieqn_unregisterProducer(pThreadData, pProducer);
            }

            iere_primeThreadCache(pThreadData, resourceSet);
            iere_freeStruct(pThreadData, resourceSet, iemem_externalObjs, pProducer, pProducer->StrucId);
        }

        if (pDestination != NULL)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_freeStruct(pThreadData, resourceSet, iemem_externalObjs, pDestination, pDestination->StrucId);
        }

        pProducer = NULL;
    }

    ieutTRACEL(pThreadData, pProducer,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, hProducer=%p\n", __func__, rc, pProducer);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Destroy Message Producer
///
/// Destroys a message producer and releases any associated resources.
///
/// @param[in]     hProducer        Producer handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_destroyProducer(
    ismEngine_ProducerHandle_t      hProducer,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Producer_t *pProducer = (ismEngine_Producer_t *)hProducer;
    assert(pProducer != NULL);
    ismEngine_Session_t *pSession = pProducer->pSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hProducer,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hProducer %p)\n", __func__, hProducer);

    ismEngine_CheckStructId(pProducer->StrucId, ismENGINE_PRODUCER_STRUCID, ieutPROBE_001);

    // Lock the session and mark the producer as destroyed
    rc = ism_engine_lockSession(pSession);
    if (rc == OK)
    {
        if (pSession->fIsDestroyed)
        {
            ism_engine_unlockSession(pSession);
            rc = ISMRC_Destroyed;
            ism_common_setError(rc);
            goto mod_exit;
        }

        // The producer has a use count to which the create/destroy pair contributes one.
        // If the use count is greater than one, we may complete this call asynchronously.
        // We must prepare for this before we start releasing storage.
        if (pProducer->UseCount > 1)
        {
            if (contextLength > 0)
            {
                pProducer->pPendingDestroyContext = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_callbackContext, 5), contextLength);
                if (pProducer->pPendingDestroyContext != NULL)
                {
                    memcpy(pProducer->pPendingDestroyContext, pContext, contextLength);
                }
                else
                {
                    ism_engine_unlockSession(pSession);
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    goto mod_exit;
                }
            }

            pProducer->pPendingDestroyCallbackFn = pCallbackFn;
        }

        pProducer->fIsDestroyed = true;

        if (pProducer->pPrev == NULL)
        {
            if (pProducer->pNext == NULL)
            {
                pSession->pProducerHead = NULL;
                pSession->pProducerTail = NULL;
            }
            else
            {
                pProducer->pNext->pPrev = NULL;
                pSession->pProducerHead = pProducer->pNext;
            }
        }
        else
        {
            if (pProducer->pNext == NULL)
            {
                pProducer->pPrev->pNext = NULL;
                pSession->pProducerTail = NULL;
            }
            else
            {
                pProducer->pPrev->pNext = pProducer->pNext;
                pProducer->pNext->pPrev = pProducer->pPrev;
            }
        }

        ism_engine_unlockSession(pSession);
    }

    if (rc == OK)
    {
        bool didFinalRelease = releaseProducerReference(pThreadData, pProducer, true);

        if (!didFinalRelease)
        {
            rc = ISMRC_AsyncCompletion;
        }
    }
mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


// Internal function to destroy a producer as part of recursive clean-up
static inline void destroyProducerInternal(ieutThreadData_t *pThreadData,
                                           ismEngine_Producer_t *pProducer)
{
    assert(pProducer->fRecursiveDestroy);

    releaseProducerReference(pThreadData, pProducer, true);
}


// Release a reference to a producer, deallocating the producer when
// the use-count drops to zero
// If fInline is true, the reference is being released by the operation
// to destroy the producer and is thus "inline". When it's done inline,
// the destroy completion callback does not need to be called because the caller
// is synchronously completing the destroy.
static inline bool releaseProducerReference(ieutThreadData_t *pThreadData,
                                            ismEngine_Producer_t *pProducer,
                                            bool fInline)
{
    bool fDidRelease = false;

    uint32_t usecount = __sync_sub_and_fetch(&pProducer->UseCount, 1);
    if (usecount == 0)
    {
        ieutTRACEL(pThreadData, pProducer, ENGINE_HIGH_TRACE, "Deallocating producer %p\n", pProducer);

        // The producer contributes a reference to the session
        ismEngine_Session_t *pSession = pProducer->pSession;
        iereResourceSetHandle_t resourceSet = pSession->pClient->resourceSet;

        ismEngine_CompletionCallback_t pPendingDestroyCallbackFn = pProducer->pPendingDestroyCallbackFn;
        void *pPendingDestroyContext = pProducer->pPendingDestroyContext;

        // Finally deallocate the producer itself
        switch(pProducer->pDestination->DestinationType)
        {
            case ismDESTINATION_TOPIC:
            case ismDESTINATION_SUBSCRIPTION:
                // nothing to do here
                break;
            case ismDESTINATION_QUEUE:
                assert(pProducer->engineObject != NULL);
                ieqn_unregisterProducer(pThreadData, pProducer);
                break;
            default:
                assert(false);
                break;
        }

        // Release our use of the messaging policy info
        iepi_releasePolicyInfo(pThreadData, pProducer->pPolicyInfo);

        iere_primeThreadCache(pThreadData, resourceSet);
        if (pProducer->pDestination != NULL) iere_freeStruct(pThreadData,
                                                             resourceSet,
                                                             iemem_externalObjs,
                                                             pProducer->pDestination,
                                                             pProducer->pDestination->StrucId);
        iere_freeStruct(pThreadData, resourceSet, iemem_externalObjs, pProducer, pProducer->StrucId);

        fDidRelease = true;

        // If we're not running in-line, we're completing this asynchronously and
        // may have been given a completion callback for the destroy operation
        if (!fInline && (pPendingDestroyCallbackFn != NULL))
        {
            pPendingDestroyCallbackFn(OK, NULL, pPendingDestroyContext);
        }

        if (pPendingDestroyContext != NULL)
        {
            iemem_free(pThreadData, iemem_callbackContext, pPendingDestroyContext);
        }

        releaseSessionReference(pThreadData, pSession, false);
    }

    return fDidRelease;
}


static void completePutMessage(
    ieutThreadData_t               *pThreadData,
    int32_t                         retcode,
    void *                          pContext)
{
    ismEngine_AsyncPut_t *putInfo = (ismEngine_AsyncPut_t *)pContext;

    ieutTRACEL(pThreadData, putInfo->unrelDeliveryIdHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "unrelDeliveryIdHandle %p, rc %d)\n", __func__,
                   putInfo->unrelDeliveryIdHandle, retcode);

    ismEngine_CheckStructId(putInfo->StrucId, ismENGINE_ASYNCPUT_STRUCID, ieutPROBE_001);

    if (retcode == ISMRC_OK)
    {
        // If the publish worked the RC for the caller was decided earlier
        retcode = putInfo->callerRC;
    }
    else
    {
        //Oh oh... commits are not supposed to fail... lets go down in flames before we
        //do more damage
        ieutTRACE_FFDC( ieutPROBE_002, true,
                        "Commit failed in completePutMessage", retcode,
                        "putInfo", putInfo, sizeof(*putInfo),
                        NULL);
    }

    //Call the callback from the caller to the engine
    void *externalContext = (void *)(putInfo+1);

    if (putInfo->pCallbackFn != NULL)
    {
        putInfo->pCallbackFn(retcode,  putInfo->unrelDeliveryIdHandle, externalContext);
    }

    //Release the usecounts
    if (putInfo->pProducerToRelease)releaseProducerReference(pThreadData, putInfo->pProducerToRelease, false);
    if (putInfo->pSessionToRelease)releaseSessionReference(pThreadData, putInfo->pSessionToRelease, false);

    ieutTRACEL(pThreadData, retcode,  ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

//Wrapper around completePutMessage used for iest_store_asyncCommit
static void ism_engine_completePutMessage(int32_t                         retcode,
                                          void *                          pContext)
{
    ietrAsyncTransactionDataHandle_t transactionData = (ietrAsyncTransactionDataHandle_t) pContext;
    ismEngine_AsyncPut_t *putInfo = ietr_getCustomDataPtr(transactionData);

    ismEngine_CheckStructId(putInfo->StrucId, ismENGINE_ASYNCPUT_STRUCID, ieutPROBE_001);
    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)(putInfo->hTran);
    ismEngine_Session_t *pSession = pTran->pSession;
    ismEngine_ClientState_t *pClientState = (pSession == NULL ) ? NULL :pSession->pClient;

    ieutThreadData_t *pThreadData = ieut_enteringEngine( pClientState);

    pThreadData->threadType = AsyncCallbackThreadType;

    ieutTRACEL(pThreadData, putInfo->asyncId, ENGINE_CEI_TRACE, FUNCTION_ENTRY "hClient %p engACId=0x%016lx\n", __func__,
               pClientState, putInfo->asyncId);


    if (putInfo->engineLocalTran)
    {
        //The engine owns the transaction... we want to actually commit the transaction
        //(and this put shouldn't hold up/stop the commit)
        ietr_decreasePreResolveCount(pThreadData, putInfo->hTran, true);

        int32_t rc = ietr_commit( pThreadData
                                , putInfo->hTran
                                , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT
                                , NULL
                                , transactionData
                                , completePutMessage );

        if (rc != OK && rc != ISMRC_AsyncCompletion)
        {
            ieutTRACE_FFDC( ieutPROBE_002, true,
                               "commit failed", rc,
                               NULL);
        }
    }
    else
    {
        //We aren't completing the entire transaction... just finish the put
        ietr_decreasePreResolveCount(pThreadData, putInfo->hTran, true);
        completePutMessage(pThreadData, retcode, (void *)putInfo);
        ietr_freeAsyncTransactionData(pThreadData, &transactionData);
    }

    ieutTRACEL(pThreadData, retcode,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, retcode);
    ieut_leavingEngine(pThreadData);

}



int32_t setupAsyncPublish( ieutThreadData_t *pThreadData
                         , ismEngine_Session_t *pSession
                         , ismEngine_Producer_t *pProducer
                         , void *pContext
                         , size_t contextLength
                         , ismEngine_CompletionCallback_t  pCallbackFn
                         , ietrAsyncTransactionDataHandle_t *phAsyncData)
{
    int32_t rc = OK;

    //Set up any data we need to
    ismEngine_AsyncPut_t    *putInfo = ietr_getCustomDataPtr(*phAsyncData);
    ismEngine_Transaction_t *pTran   = (ismEngine_Transaction_t *)putInfo->hTran;
    putInfo->pProducerToRelease = pProducer;
    putInfo->pSessionToRelease  = pSession;
    putInfo->contextLength = contextLength;
    void *contextPtr = (void *)(putInfo + 1);

    putInfo->pCallbackFn = pCallbackFn;
    if (contextLength > 0)
    {
        assert(pContext != NULL);
        memcpy(contextPtr, pContext, contextLength);
    }

    //Store the caller retcode as commit will free the async data after callbacks+completion
    int32_t callerRC = putInfo->callerRC;

    //If this transaction exists just for this put we can commit the engine-level transaction at this point
    if (pTran->fAsStoreTran)
    {
        rc = ietr_commit( pThreadData
                        , putInfo->hTran
                        , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT
                        , NULL
                        , *phAsyncData
                        , completePutMessage );
    }
    else
    {
        //As we are about to go async, ensure any transaction can't be committed/rolled back until this operation finishes
        ietr_increasePreResolveCount(pThreadData, pTran);

        //Setup AsyncId and trace it so we can tie up commit with callback:
        putInfo->asyncId = pThreadData->asyncCounter++;
        ieutTRACEL(pThreadData, putInfo->asyncId, ENGINE_CEI_TRACE, FUNCTION_IDENT "engACId=0x%016lx\n",
                              __func__,putInfo->asyncId);
 
         rc = iest_store_asyncCommit(pThreadData,
                                   true,
                                   ism_engine_completePutMessage,
                                   *phAsyncData );

         if (rc == OK)
         {
             if (putInfo->engineLocalTran)
             {
                 //The put we are part of shouldn't block the commit...
                 ietr_decreasePreResolveCount(pThreadData, putInfo->hTran, true);

                 //The engine owns the transaction... we want to actually commit the transaction
                 rc = ietr_commit( pThreadData
                                 , putInfo->hTran
                                 , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT
                                 , NULL
                                 ,  *phAsyncData
                                 , completePutMessage );

                 if (rc != OK && rc != ISMRC_AsyncCompletion)
                 {
                     ieutTRACE_FFDC( ieutPROBE_002, true,
                                        "commit failed", rc,
                                        NULL);
                 }
             }
             else
             {
                 //We aren't completing the entire transaction... just free the memory that we would
                 //have used if we'd gone async
                 ietr_decreasePreResolveCount(pThreadData, putInfo->hTran, true);
                 ietr_freeAsyncTransactionData(pThreadData, phAsyncData);
             }
         }
    }

    if (rc == OK)
    {
        rc = callerRC;
    }
    else if (rc != ISMRC_AsyncCompletion)
    {
        ieutTRACE_FFDC( ieutPROBE_001, true,
                        "commit failed", rc,
                        NULL);
    }

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Put Message
///
/// Puts a message on a destination. The message is put inside the
/// supplied transaction, which must be associated with the session.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hProducer        Producer handle
/// @param[in]     hTran            Transaction handle, may be NULL
/// @param[in]     hMessage         Message handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_putMessage(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_ProducerHandle_t      hProducer,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    assert(pSession != NULL);
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Producer_t *pProducer = (ismEngine_Producer_t *)hProducer;
    ismEngine_Message_t *pMessage = (ismEngine_Message_t *)hMessage;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hProducer, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, hProducer %p, hTran %p, hMessage %p)\n", __func__,
               hSession, hProducer, hTran, hMessage);

    ismEngine_CheckStructId(pSession->StrucId, ismENGINE_SESSION_STRUCID, ieutPROBE_001);
    assert(pProducer != NULL);
    ismEngine_CheckStructId(pProducer->StrucId, ismENGINE_PRODUCER_STRUCID, ieutPROBE_002);
    assert(pProducer->pSession == hSession);
    assert(pProducer->pDestination != NULL);
    assert(pProducer->pDestination->DestinationType == ismDESTINATION_TOPIC ||
           pProducer->pDestination->DestinationType == ismDESTINATION_QUEUE);

    //Don't do security check.... the security was checked at createProducer

    rc = ism_engine_lockSession(pSession);
    if (rc == OK)
    {
        if (pSession->fIsDestroyed)
        {
            ism_engine_unlockSession(pSession);
            rc = ISMRC_Destroyed;
            ism_common_setError(rc);
            goto mod_exit;
        }

        __sync_fetch_and_add(&pProducer->UseCount, 1);

        ism_engine_unlockSession(pSession);

        // Enforce Max Message Time to Live if one is specified
        iepiPolicyInfo_t *pPolicyInfo = pProducer->pPolicyInfo;
        if (pPolicyInfo->maxMessageTimeToLive != 0)
        {
            uint32_t maxExpiry = ism_common_nowExpire() + pPolicyInfo->maxMessageTimeToLive;

            if (pMessage->Header.Expiry == 0 || pMessage->Header.Expiry > maxExpiry)
            {
                ieutTRACEL(pThreadData, maxExpiry, ENGINE_HIGH_TRACE,
                           "Overriding message expiry from %u to %u\n", pMessage->Header.Expiry, maxExpiry);
                pMessage->Header.Expiry = maxExpiry;
            }
        }

        if (pProducer->pDestination->DestinationType == ismDESTINATION_TOPIC)
        {
            ietrAsyncTransactionDataHandle_t hAsyncData = NULL;

            rc = ieds_publish(pThreadData,
                              pSession->pClient,
                              pProducer->pDestination->pDestinationName,
                              iedsPUBLISH_OPTION_INFORMATIONAL_RETCODES,
                              (ismEngine_Transaction_t *)hTran,
                              pMessage,
                              0,
                              NULL,
                              contextLength,
                              &hAsyncData);

            if (rc == ISMRC_NeedStoreCommit)
            {
                //The publish wants to go async.... get ready
                rc = setupAsyncPublish( pThreadData
                                      , NULL
                                      , pProducer
                                      , pContext
                                      , contextLength
                                      , pCallbackFn
                                      , &hAsyncData);

                if (rc == ISMRC_AsyncCompletion)
                {
                    //The async completion of the publish will release the producer
                    goto mod_exit;
                }
            }
        }
        else
        {
            rc = ieds_put(pThreadData,
                          pSession->pClient,
                          pProducer,
                          (ismEngine_Transaction_t *)hTran,
                          pMessage);
        }

        releaseProducerReference(pThreadData, pProducer, false);
    }

mod_exit:

    // It is up to us to release the use count on the message. In time
    // this may be done deeper in the engine.
    iem_releaseMessage(pThreadData, hMessage);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Put Message on Destination
///
/// Puts a message on a destination without creating a message producer.
/// The message is put inside the supplied transaction, which must be associated
/// with the session.
///
/// @param[in]     hSession         Session handle
/// @param[in]     destinationType  Destination type
/// @param[in]     pDestinationName Destination name
/// @param[in]     hTran            Transaction handle, may be NULL
/// @param[in]     hMessage         Message handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_putMessageOnDestination(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Message_t *pMessage = (ismEngine_Message_t *)hMessage;
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    assert(pSession != NULL);
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, destinationType %d, pDestinationName '%s', hTran %p, pMessage %p)\n", __func__,
               hSession, destinationType, (pDestinationName != NULL) ? pDestinationName : "(nil)", hTran, pMessage);

    assert(destinationType == ismDESTINATION_TOPIC ||
           destinationType == ismDESTINATION_QUEUE);
    assert(pDestinationName != NULL);
    assert(strlen(pDestinationName) <= ismDESTINATION_NAME_LENGTH);
    ismEngine_CheckStructId(pSession->StrucId, ismENGINE_SESSION_STRUCID, ieutPROBE_001);

    // If putting to a topic, the topic string must not contain wildcards
    if (destinationType == ismDESTINATION_TOPIC)
    {
        if (!iett_validateTopicString(pThreadData, pDestinationName, iettVALIDATE_FOR_PUBLISH))
        {
            rc = ISMRC_DestNotValid;
            ism_common_setErrorData(rc, "%s", pDestinationName);
            goto mod_exit;
        }
    }

    // Check with security before we do anything other than trace the request
    iepiPolicyInfo_t *pValidatedPolicyInfo = NULL;

    if (destinationType == ismDESTINATION_TOPIC)
    {
        rc = ismEngine_security_validate_policy_func(pSession->pClient->pSecContext,
                                                     ismSEC_AUTH_TOPIC,
                                                     pDestinationName,
                                                     ismSEC_AUTH_ACTION_PUBLISH,
                                                     ISM_CONFIG_COMP_ENGINE,
                                                     (void **)&pValidatedPolicyInfo);
    }
    else
    {
        bool isTemporary;

        assert(destinationType == ismDESTINATION_QUEUE);

        rc = ieqn_isTemporaryQueue(pDestinationName, &isTemporary, NULL);

        // We do not check security for temporary queues
        if (rc == OK && isTemporary == false)
        {
            rc = ismEngine_security_validate_policy_func(pSession->pClient->pSecContext,
                                                         ismSEC_AUTH_QUEUE,
                                                         pDestinationName,
                                                         ismSEC_AUTH_ACTION_SEND,
                                                         ISM_CONFIG_COMP_ENGINE,
                                                         (void **)&pValidatedPolicyInfo);
        }
    }

    if (rc != OK) goto mod_exit;

    // The policy can come back with a NULL context for __MQConnectiivty requests and unit tests,
    // so if we are authorized, but got no context, we use the default policy.
    if (pValidatedPolicyInfo == NULL) pValidatedPolicyInfo = iepi_getDefaultPolicyInfo(false);

    assert(pValidatedPolicyInfo != NULL);

    // Impose max message time to live
    if (pValidatedPolicyInfo->maxMessageTimeToLive != 0)
    {
        uint32_t maxExpiry = ism_common_nowExpire() + pValidatedPolicyInfo->maxMessageTimeToLive;

        if (pMessage->Header.Expiry == 0 || pMessage->Header.Expiry > maxExpiry)
        {
            ieutTRACEL(pThreadData, maxExpiry, ENGINE_HIGH_TRACE,
                       "Overriding message expiry from %u to %u\n", pMessage->Header.Expiry, maxExpiry);
            pMessage->Header.Expiry = maxExpiry;
        }
    }

    rc = ism_engine_lockSession(pSession);
    if (rc == OK)
    {
        if (pSession->fIsDestroyed)
        {
            ism_engine_unlockSession(pSession);
            rc = ISMRC_Destroyed;
            ism_common_setError(rc);
            goto mod_exit;
        }

        __sync_fetch_and_add(&pSession->UseCount, 1);
        ism_engine_unlockSession(pSession);

        if (destinationType == ismDESTINATION_TOPIC)
        {
            ietrAsyncTransactionDataHandle_t hAsyncData = NULL;

            rc = ieds_publish(pThreadData,
                              pSession->pClient,
                              pDestinationName,
                              iedsPUBLISH_OPTION_INFORMATIONAL_RETCODES,
                              (ismEngine_Transaction_t *)hTran,
                              pMessage,
                              0,
                              NULL,
                              contextLength,
                              &hAsyncData);

            if (rc == ISMRC_NeedStoreCommit)
            {
                //The publish wants to go async.... get ready
                rc = setupAsyncPublish( pThreadData
                                      , pSession
                                      , NULL
                                      , pContext
                                      , contextLength
                                      , pCallbackFn
                                      , &hAsyncData);

                if (rc == ISMRC_AsyncCompletion)
                {
                    //The async completion of the publish will release the session
                    goto mod_exit;
                }
            }
        }
        else
        {
            rc = ieds_putToQueueName(pThreadData,
                                     pSession->pClient,
                                     pDestinationName,
                                     (ismEngine_Transaction_t *)hTran,
                                     pMessage);
        }

        releaseSessionReference(pThreadData, pSession, false);
    }

mod_exit:

    // It is up to us to release the use count on the message. In time
    // this may be done deeper in the engine.
    iem_releaseMessage(pThreadData, hMessage);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Put Message with Unreleased Delivery ID
///
/// Puts a message on a destination and adds a delivery ID to the list of
/// messages put on behalf of a sending client, but not yet released by the client.
/// The message is put inside the supplied transaction, which must be associated
/// with the session. If there is no unreleased delivery ID for the message,
/// zero may be specified.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hProducer        Producer handle
/// @param[in]     hTran            Transaction handle, may be NULL
/// @param[in]     hMessage         Message handle
/// @param[in]     unrelDeliveryId  Unreleased delivery ID (may be zero)
/// @param[out]    phUnrel          Returned unreleased-message handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_putMessageWithDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_ProducerHandle_t      hProducer,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    assert(pSession != NULL);
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Producer_t *pProducer = (ismEngine_Producer_t *)hProducer;
    ismEngine_Message_t *pMessage = (ismEngine_Message_t *)hMessage;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hProducer, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, hProducer %p, hTran %p, hMessage %p, unrelDeliveryId %u)\n", __func__,
               hSession, hProducer, hTran, hMessage, unrelDeliveryId);

    ismEngine_CheckStructId(pSession->StrucId, ismENGINE_SESSION_STRUCID, ieutPROBE_001);
    assert(pProducer != NULL);
    ismEngine_CheckStructId(pProducer->StrucId, ismENGINE_PRODUCER_STRUCID, ieutPROBE_002);
    assert(pProducer->pSession == hSession);
    assert(pProducer->pDestination != NULL);
    assert(pProducer->pDestination->DestinationType == ismDESTINATION_TOPIC ||
           pProducer->pDestination->DestinationType == ismDESTINATION_QUEUE);

    // Don't do security check.... the security was checked at createProducer

    rc = ism_engine_lockSession(pSession);
    if (rc == OK)
    {
        if (pSession->fIsDestroyed)
        {
            ism_engine_unlockSession(pSession);
            rc = ISMRC_Destroyed;
            ism_common_setError(rc);
            goto mod_exit;
        }

        __sync_fetch_and_add(&pProducer->UseCount, 1);

        ism_engine_unlockSession(pSession);

        // Enforce Max Message Time to Live
        iepiPolicyInfo_t *pPolicyInfo = pProducer->pPolicyInfo;
        if (pPolicyInfo->maxMessageTimeToLive != 0)
        {
            uint32_t maxExpiry = ism_common_nowExpire() + pPolicyInfo->maxMessageTimeToLive;

            if (pMessage->Header.Expiry == 0 || pMessage->Header.Expiry > maxExpiry)
            {
                ieutTRACEL(pThreadData, maxExpiry, ENGINE_HIGH_TRACE,
                           "Overriding message expiry from %u to %u\n", pMessage->Header.Expiry, maxExpiry);
                pMessage->Header.Expiry = maxExpiry;
            }
        }

        if (pProducer->pDestination->DestinationType == ismDESTINATION_TOPIC)
        {
            ietrAsyncTransactionDataHandle_t hAsyncData = NULL;

            rc = ieds_publish(pThreadData,
                              pSession->pClient,
                              pProducer->pDestination->pDestinationName,
                              iedsPUBLISH_OPTION_INFORMATIONAL_RETCODES,
                              (ismEngine_Transaction_t *)hTran,
                              pMessage,
                              unrelDeliveryId,
                              phUnrel,
                              contextLength,
                              &hAsyncData);

            if (rc == ISMRC_NeedStoreCommit)
            {
                //The publish wants to go async.... get ready
                rc = setupAsyncPublish( pThreadData
                                      , NULL
                                      , pProducer
                                      , pContext
                                      , contextLength
                                      , pCallbackFn
                                      , &hAsyncData);

                if (rc == ISMRC_AsyncCompletion)
                {
                    //The async completion of the publish will release the producer
                    goto mod_exit;
                }
            }
        }
        else
        {
            rc = ieds_put(pThreadData,
                          pSession->pClient,
                          pProducer,
                          (ismEngine_Transaction_t *)hTran,
                          pMessage);
        }

        releaseProducerReference(pThreadData, pProducer, false);
    }

mod_exit:

    // It is up to us to release the use count on the message. In time
    // this may be done deeper in the engine.
    iem_releaseMessage(pThreadData, hMessage);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Put Message with Unreleased Delivery ID on Destination
///
/// Puts a message on a destination without creating a message producer, and
/// adds a delivery ID to the list of messages put on behalf of a sending client,
/// but not yet released by the client. The message is put inside the supplied
/// transaction, which must be associated with the session. If there is no
/// unreleased delivery ID for the message, zero may be specified.
///
/// @param[in]     hSession         Session handle
/// @param[in]     destinationType  Destination type
/// @param[in]     pDestinationName Destination name
/// @param[in]     hTran            Transaction handle, may be NULL
/// @param[in]     hMessage         Message handle
/// @param[in]     unrelDeliveryId  Unreleased delivery ID (may be zero)
/// @param[out]    phUnrel          Returned unreleased-message handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_putMessageWithDeliveryIdOnDestination(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Message_t *pMessage = (ismEngine_Message_t *)hMessage;
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    assert(pSession != NULL);
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, destinationType %d, pDestinationName '%s', hTran %p, pMessage %p, unrelDeliveryId %u)\n", __func__,
               hSession, destinationType, (pDestinationName != NULL) ? pDestinationName : "(nil)", hTran, pMessage, unrelDeliveryId);

    assert(destinationType == ismDESTINATION_TOPIC ||
           destinationType == ismDESTINATION_QUEUE);
    assert(pDestinationName != NULL);
    assert(strlen(pDestinationName) <= ismDESTINATION_NAME_LENGTH);
    ismEngine_CheckStructId(pSession->StrucId, ismENGINE_SESSION_STRUCID, ieutPROBE_001);

    // If putting to a topic, the topic string must not contain wildcards
    if (destinationType == ismDESTINATION_TOPIC)
    {
        if (!iett_validateTopicString(pThreadData, pDestinationName, iettVALIDATE_FOR_PUBLISH))
        {
            rc = ISMRC_DestNotValid;
            ism_common_setErrorData(rc, "%s", pDestinationName);
            goto mod_exit;
        }
    }

    // Check with security before we do anything other than trace the request
    iepiPolicyInfo_t *pValidatedPolicyInfo = NULL;

    if (destinationType == ismDESTINATION_TOPIC)
    {
        rc = ismEngine_security_validate_policy_func(pSession->pClient->pSecContext,
                                                     ismSEC_AUTH_TOPIC,
                                                     pDestinationName,
                                                     ismSEC_AUTH_ACTION_PUBLISH,
                                                     ISM_CONFIG_COMP_ENGINE,
                                                     (void **)&pValidatedPolicyInfo);
    }
    else
    {
        bool isTemporary;

        assert(destinationType == ismDESTINATION_QUEUE);

        rc = ieqn_isTemporaryQueue(pDestinationName, &isTemporary, NULL);

        // We only check security for non-temporary
        if (rc == OK && isTemporary == false)
        {
            rc = ismEngine_security_validate_policy_func(pSession->pClient->pSecContext,
                                                         ismSEC_AUTH_QUEUE,
                                                         pDestinationName,
                                                         ismSEC_AUTH_ACTION_SEND,
                                                         ISM_CONFIG_COMP_ENGINE,
                                                         (void **)&pValidatedPolicyInfo);
        }
    }

    if (rc != OK) goto mod_exit;

    // The policy can come back with a NULL context for __MQConnectiivty requests and unit tests,
    // so if we are authorized, but got no context, we use the default policy.
    if (pValidatedPolicyInfo == NULL) pValidatedPolicyInfo = iepi_getDefaultPolicyInfo(false);

    assert(pValidatedPolicyInfo != NULL);

    // Impose max message time to live
    if (pValidatedPolicyInfo->maxMessageTimeToLive != 0)
    {
        uint32_t maxExpiry = ism_common_nowExpire() + pValidatedPolicyInfo->maxMessageTimeToLive;

        if (pMessage->Header.Expiry == 0 || pMessage->Header.Expiry > maxExpiry)
        {
            ieutTRACEL(pThreadData, maxExpiry, ENGINE_HIGH_TRACE,
                       "Overriding message expiry from %u to %u\n", pMessage->Header.Expiry, maxExpiry);
            pMessage->Header.Expiry = maxExpiry;
        }
    }

    rc = ism_engine_lockSession(pSession);
    if (rc == OK)
    {
        if (pSession->fIsDestroyed)
        {
            ism_engine_unlockSession(pSession);
            rc = ISMRC_Destroyed;
            ism_common_setError(rc);
            goto mod_exit;
        }

        __sync_fetch_and_add(&pSession->UseCount, 1);
        ism_engine_unlockSession(pSession);

        if (destinationType == ismDESTINATION_TOPIC)
        {
            ietrAsyncTransactionDataHandle_t hAsyncData = NULL;

            rc = ieds_publish(pThreadData,
                              pSession->pClient,
                              pDestinationName,
                              iedsPUBLISH_OPTION_INFORMATIONAL_RETCODES,
                              (ismEngine_Transaction_t *)hTran,
                              pMessage,
                              unrelDeliveryId,
                              phUnrel,
                              contextLength,
                              &hAsyncData);

            if (rc == ISMRC_NeedStoreCommit)
            {
                //The publish wants to go async.... get ready
                rc = setupAsyncPublish( pThreadData
                                      , pSession
                                      , NULL
                                      , pContext
                                      , contextLength
                                      , pCallbackFn
                                      , &hAsyncData);

                if (rc == ISMRC_AsyncCompletion)
                {
                    //The async completion of the publish will release the session
                    goto mod_exit;
                }
            }
        }
        else
        {
            rc = ieds_putToQueueName(pThreadData,
                                     pSession->pClient,
                                     pDestinationName,
                                     (ismEngine_Transaction_t *)hTran,
                                     pMessage);
        }

        releaseSessionReference(pThreadData, pSession, false);
    }

mod_exit:

    // It is up to us to release the use count on the message. In time
    // this may be done deeper in the engine.
    iem_releaseMessage(pThreadData, hMessage);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Put Message on a destination from an internal source (no session)
///
/// Puts a message on a destination without creating a message producer and
/// without a putting session being specified - because no session is supplied,
/// there is no support for an external transaction.
///
/// @param[in]     destinationType  Destination type
/// @param[in]     pDestinationName Destination name
/// @param[in]     hMessage         Message handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark This function is intended to allow a simple publish of a non-persistent
///         QoS 0 message without the need for a client / session. As a result
///         there are several restrictions on it's use - these are not enforced,
///         but asserted.
///
///         destinationType must be ismDESTINATION_TOPIC
///         hMessage must be non-persistent (ismMESSAGE_PERSISTENCE_NONPERSISTENT)
///         hMessage must be QoS0 (ismMESSAGE_RELIABILITY_AT_MOST_ONCE) or QoS1
///                          (ismMESSAGE_RELIABILITY_AT_LEAST_ONCE)
///         hMessage must NOT be retained (ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN not set)
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_putMessageInternalOnDestination(
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_MessageHandle_t       hMessage,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;
    ismEngine_Message_t *pMessage = (ismEngine_Message_t *)hMessage;

    ieutTRACEL(pThreadData, hMessage, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(destinationType %d, pDestinationName '%s', hMessage %p)\n", __func__,
               destinationType, pDestinationName, hMessage);

    // Assert the restrictions placed on this function
    // (note: NONE of these restrictions actually pose a problem, they are here
    //        purely to limit the scope of this function at this time - if we
    //        ever wanted to put a message on a queue, or publish a retained
    //        or persistent message, we would have to test it, but it should work).

    assert(destinationType == ismDESTINATION_TOPIC); // Must be a topic (i.e. publish)
    assert(pDestinationName != NULL);
    assert(strlen(pDestinationName) <= ismDESTINATION_NAME_LENGTH);
    assert((pMessage->Header.Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN) == 0);

    // No security check - this is an internal caller

    if (destinationType == ismDESTINATION_TOPIC)
    {
        if (!iett_validateTopicString(pThreadData, pDestinationName, iettVALIDATE_FOR_PUBLISH))
        {
            rc = ISMRC_DestNotValid;
            ism_common_setErrorData(rc, "%s", pDestinationName);
            goto mod_exit;
        }

        ietrAsyncTransactionDataHandle_t hAsyncData = NULL;

        rc = ieds_publish(pThreadData,
                          NULL, // No client
                          pDestinationName,
                          iedsPUBLISH_OPTION_NONE,
                          NULL, // No transaction
                          pMessage,
                          0,
                          NULL,
                          0,
                          NULL);


        if (rc == ISMRC_NeedStoreCommit)
        {
            //The publish wants to go async.... get ready
            rc = setupAsyncPublish( pThreadData
                                  , NULL
                                  , NULL
                                  , pContext
                                  , contextLength
                                  , pCallbackFn
                                  , &hAsyncData);

            if (rc == ISMRC_AsyncCompletion)
            {
                goto mod_exit;
            }
        }
    }
    else
    {
        rc = ieds_putToQueueName(pThreadData,
                                 NULL, // No client
                                 pDestinationName,
                                 NULL, // No transaction
                                 pMessage);
    }

mod_exit:

    // It is up to us to release the use count on the message. In time
    // this may be done deeper in the engine.
    iem_releaseMessage(pThreadData, hMessage);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Publish and unset the retained message for a given producer
///
/// Publish and unset the retained message associated with the topic on which
/// the given producer is currently working.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hProducer        Producer handle
/// @param[in]     options          ismENGINE_UNSET_RETAINED* option values
/// @param[in]     hTran            Transaction handle, may be NULL
/// @param[in]     hMessage         Message handle
/// @param[in]     unrelDeliveryId  Unreleased delivery ID (may be zero)
/// @param[out]    phUnrel          Returned unreleased-message handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_unsetRetainedMessageWithDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_ProducerHandle_t      hProducer,
    uint32_t                        options,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    assert(pSession != NULL);
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Producer_t *pProducer = (ismEngine_Producer_t *)hProducer;
    ismEngine_Message_t *pMessage = (ismEngine_Message_t *)hMessage;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hProducer, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, hProducer %p, hTran %p, hMessage %p, unrelDeliveryId %u)\n", __func__,
               hSession, hProducer, hTran, hMessage, unrelDeliveryId);

    ismEngine_CheckStructId(pSession->StrucId, ismENGINE_SESSION_STRUCID, ieutPROBE_001);
    assert(pProducer != NULL);
    ismEngine_CheckStructId(pProducer->StrucId, ismENGINE_PRODUCER_STRUCID, ieutPROBE_002);
    assert(pProducer->pSession == hSession);
    assert(pProducer->pDestination != NULL);
    assert(pProducer->pDestination->DestinationType == ismDESTINATION_TOPIC);
    assert(pMessage->Header.MessageType == MTYPE_NullRetained);

    // Don't do security check.... the security was checked at createProducer

    rc = ism_engine_lockSession(pSession);
    if (rc == OK)
    {
        if (pSession->fIsDestroyed)
        {
            ism_engine_unlockSession(pSession);
            rc = ISMRC_Destroyed;
            ism_common_setError(rc);
            goto mod_exit;
        }

        __sync_fetch_and_add(&pProducer->UseCount, 1);

        ism_engine_unlockSession(pSession);

        uint32_t publishOptions = iedsPUBLISH_OPTION_INFORMATIONAL_RETCODES;

        // If we don't want to publish, tell the publish function.
        if ((options & ismENGINE_UNSET_RETAINED_OPTION_PUBLISH) == 0)
        {
            publishOptions |= iedsPUBLISH_OPTION_ONLY_UPDATE_RETAINED;
        }
        else
        {
            publishOptions |= iedsPUBLISH_OPTION_NONE;
        }

        // Enforce Max Message Time to Live
        iepiPolicyInfo_t *pPolicyInfo = pProducer->pPolicyInfo;
        if (pPolicyInfo->maxMessageTimeToLive != 0)
        {
            uint32_t maxExpiry = ism_common_nowExpire() + pPolicyInfo->maxMessageTimeToLive;

            if (pMessage->Header.Expiry == 0 || pMessage->Header.Expiry > maxExpiry)
            {
                ieutTRACEL(pThreadData, maxExpiry, ENGINE_HIGH_TRACE,
                           "Overriding message expiry from %u to %u\n", pMessage->Header.Expiry, maxExpiry);
                pMessage->Header.Expiry = maxExpiry;
            }
        }

        ietrAsyncTransactionDataHandle_t hAsyncData = NULL;

        rc = ieds_publish(pThreadData,
                          pSession->pClient,
                          pProducer->pDestination->pDestinationName,
                          publishOptions,
                          (ismEngine_Transaction_t *)hTran,
                          pMessage,
                          unrelDeliveryId,
                          phUnrel,
                          contextLength,
                          &hAsyncData);

        if (rc == ISMRC_NeedStoreCommit)
        {
            //The publish wants to go async.... get ready
            rc = setupAsyncPublish( pThreadData
                                  , NULL
                                  , pProducer
                                  , pContext
                                  , contextLength
                                  , pCallbackFn
                                  , &hAsyncData);

            if (rc == ISMRC_AsyncCompletion)
            {
                //The async completion of the publish will release the producer
                goto mod_exit;
            }
        }

        releaseProducerReference(pThreadData, pProducer, false);
    }

mod_exit:

    // It is up to us to release the use count on the message. In time
    // this may be done deeper in the engine.
    iem_releaseMessage(pThreadData, hMessage);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Publish and unset the retained message for a given destination
///
/// Publish and unset the retained message associated with the topic specified.
///
/// @param[in]     hSession         Session handle
/// @param[in]     destinationType  Destination type
/// @param[in]     pDestinationName Destination name
/// @param[in]     options          ismENGINE_UNSET_RETAINED* option values
/// @param[in]     hTran            Transaction handle, may be NULL
/// @param[in]     hMessage         Message handle
/// @param[in]     unrelDeliveryId  Unreleased delivery ID (may be zero)
/// @param[out]    phUnrel          Returned unreleased-message handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_unsetRetainedMessageWithDeliveryIdOnDestination(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    uint32_t                        options,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    assert(pSession != NULL);
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Message_t *pMessage = (ismEngine_Message_t *)hMessage;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, destinationType %d, pDestinationName '%s', hTran %p, pMessage %p, unrelDeliveryId %u)\n", __func__,
               hSession, destinationType, (pDestinationName != NULL) ? pDestinationName : "(nil)", hTran, pMessage, unrelDeliveryId);

    ismEngine_CheckStructId(pSession->StrucId, ismENGINE_SESSION_STRUCID, ieutPROBE_001);
    assert(destinationType == ismDESTINATION_TOPIC);
    assert(pDestinationName != NULL);
    assert(strlen(pDestinationName) <= ismDESTINATION_NAME_LENGTH);
    assert(pMessage->Header.MessageType == MTYPE_NullRetained);

    if (!iett_validateTopicString(pThreadData, pDestinationName, iettVALIDATE_FOR_PUBLISH))
    {
        rc = ISMRC_DestNotValid;
        ism_common_setErrorData(rc, "%s", pDestinationName);
        goto mod_exit;
    }

    rc = ism_engine_lockSession(pSession);
    if (rc == OK)
    {
        if (pSession->fIsDestroyed)
        {
            ism_engine_unlockSession(pSession);
            rc = ISMRC_Destroyed;
            ism_common_setError(rc);
            goto mod_exit;
        }

        __sync_fetch_and_add(&pSession->UseCount, 1);

        ism_engine_unlockSession(pSession);

        uint32_t publishOptions = iedsPUBLISH_OPTION_INFORMATIONAL_RETCODES;

        // If we don't want to publish, tell the publish function.
        if ((options & ismENGINE_UNSET_RETAINED_OPTION_PUBLISH) == 0)
        {
            publishOptions |= iedsPUBLISH_OPTION_ONLY_UPDATE_RETAINED;
        }
        else
        {
            publishOptions |= iedsPUBLISH_OPTION_NONE;
        }

        // Check with security before we do anything other than trace the request
        iepiPolicyInfo_t *pValidatedPolicyInfo = NULL;

        rc = ismEngine_security_validate_policy_func(pSession->pClient->pSecContext,
                                                     ismSEC_AUTH_TOPIC,
                                                     pDestinationName,
                                                     ismSEC_AUTH_ACTION_PUBLISH,
                                                     ISM_CONFIG_COMP_ENGINE,
                                                     (void **)&pValidatedPolicyInfo);

        if (rc != OK) goto mod_exit;

        // The policy can come back with a NULL context for __MQConnectivty requests and unit tests,
        // so if we are authorized, but got no context, we use the default policy.
        if (pValidatedPolicyInfo == NULL) pValidatedPolicyInfo = iepi_getDefaultPolicyInfo(false);

        assert(pValidatedPolicyInfo != NULL);

        // Impose max message time to live
        if (pValidatedPolicyInfo->maxMessageTimeToLive != 0)
        {
            uint32_t maxExpiry = ism_common_nowExpire() + pValidatedPolicyInfo->maxMessageTimeToLive;

            if (pMessage->Header.Expiry == 0 || pMessage->Header.Expiry > maxExpiry)
            {
                ieutTRACEL(pThreadData, maxExpiry, ENGINE_HIGH_TRACE,
                           "Overriding message expiry from %u to %u\n", pMessage->Header.Expiry, maxExpiry);
                pMessage->Header.Expiry = maxExpiry;
            }
        }

        ietrAsyncTransactionDataHandle_t hAsyncData = NULL;

        rc = ieds_publish(pThreadData,
                          pSession->pClient,
                          pDestinationName,
                          publishOptions,
                          (ismEngine_Transaction_t *)hTran,
                          pMessage,
                          unrelDeliveryId,
                          phUnrel,
                          contextLength,
                          &hAsyncData);

        if (rc == ISMRC_NeedStoreCommit)
        {
            //The publish wants to go async.... get ready
            rc = setupAsyncPublish( pThreadData
                                  , pSession
                                  , NULL
                                  , pContext
                                  , contextLength
                                  , pCallbackFn
                                  , &hAsyncData);

            if (rc == ISMRC_AsyncCompletion)
            {
                //The async completion of the publish will release the session
                goto mod_exit;
            }
        }

        releaseSessionReference(pThreadData, pSession, false);
    }

mod_exit:

    // It is up to us to release the use count on the message. In time
    // this may be done deeper in the engine.
    iem_releaseMessage(pThreadData, pMessage);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
/// @brief  Unset the retained message on a destination
///
/// Removes the retained message on a destination without creating a message
/// producer.
///
/// @param[in]     hSession           Session handle
/// @param[in]     destinationType    Destination type
/// @param[in]     pDestinationName   Destination name
/// @param[in]     options            ismENGINE_UNSET_RETAINED* option values
/// @param[in]     serverTime         Server time to use (usually ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME)
/// @param[in]     hTran              Transaction handle, may be NULL
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_unsetRetainedMessageOnDestination(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    uint32_t                        options,
    uint64_t                        serverTime,
    ismEngine_TransactionHandle_t   hTran,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession ? pSession->pClient : NULL);
    int32_t rc = OK;
    ismEngine_Message_t *pMessage = NULL;

    ieutTRACEL(pThreadData, hSession, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, destinationType %d, pDestinationName '%s', serverTime=%lu)\n", __func__,
               hSession, destinationType, (pDestinationName != NULL) ? pDestinationName : "(nil)", serverTime);

    assert(destinationType == ismDESTINATION_TOPIC || destinationType == ismDESTINATION_REGEX_TOPIC);
    assert(pDestinationName != NULL);
    assert(destinationType == ismDESTINATION_REGEX_TOPIC || strlen(pDestinationName) <= ismDESTINATION_NAME_LENGTH);

    // If we have a session take a reference on it.
    if (pSession != NULL)
    {
        ismEngine_CheckStructId(pSession->StrucId, ismENGINE_SESSION_STRUCID, ieutPROBE_001);

        rc = ism_engine_lockSession(pSession);

        if (rc == OK && pSession->fIsDestroyed)
        {
            ism_engine_unlockSession(pSession);
            rc = ISMRC_Destroyed;
            ism_common_setError(rc);
            goto mod_exit;
        }

        __sync_fetch_and_add(&pSession->UseCount, 1);

        ism_engine_unlockSession(pSession);

        // For TOPIC requests, make sure that the caller is authorized
        if (destinationType == ismDESTINATION_TOPIC)
        {
            rc = ismEngine_security_validate_policy_func(pSession->pClient->pSecContext,
                                                         ismSEC_AUTH_TOPIC,
                                                         pDestinationName,
                                                         ismSEC_AUTH_ACTION_PUBLISH,
                                                         ISM_CONFIG_COMP_ENGINE,
                                                         NULL);
        }
    }

    if (rc == OK)
    {
        uint32_t publishOptions = iedsPUBLISH_OPTION_INFORMATIONAL_RETCODES;

        if (destinationType == ismDESTINATION_TOPIC)
        {
            #ifndef NDEBUG
            bool valid = iett_validateTopicString(pThreadData, pDestinationName, iettVALIDATE_FOR_PUBLISH);
            assert(valid == true);
            #endif

            if ((options & ismENGINE_UNSET_RETAINED_OPTION_PUBLISH) == 0)
            {
                publishOptions |= iedsPUBLISH_OPTION_ONLY_UPDATE_RETAINED;
            }
            else
            {
                publishOptions |= iedsPUBLISH_OPTION_NONE;
            }

            // Create an MTYPE_NullRetained to publish
            char xbuf[1024];
            concat_alloc_t props = {xbuf, sizeof(xbuf)};
            assert(props.used == 0);

            ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
            ismMessageAreaType_t areaTypes[3];
            size_t areaLengths[3];
            void *areaData[3];
            uint8_t areaCount;

            ism_field_t field;
            field.type = VT_String;
            field.val.s = (char *)pDestinationName;
            ism_common_putPropertyID(&props, ID_Topic, &field);

            // Add the serverUID to the properties
            field.type = VT_String;
            field.val.s = (char *)ism_common_getServerUID();
            ism_common_putPropertyID(&props, ID_OriginServer, &field);

            // Add the serverTime to the properties
            field.type = VT_Long;
            if (serverTime == ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME)
            {
                field.val.l = ism_engine_retainedServerTime();
            }
            else
            {
                field.val.l = serverTime;
            }
            ism_common_putPropertyID(&props, ID_ServerTime, &field);

            areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
            areaLengths[0] = props.used;
            areaData[0] = props.buf;
            areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
            areaLengths[1] = 0;
            areaData[1] = NULL;

            areaCount = 2;

            header.MessageType = MTYPE_NullRetained;
            header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
            header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
            header.Flags = ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;
            header.Expiry = 0;  // TODO: Should we enforce an expiry on this?

            rc = ism_engine_createMessage(&header,
                                          areaCount,
                                          areaTypes,
                                          areaLengths,
                                          areaData,
                                          (ismEngine_MessageHandle_t *)&pMessage);

            if (props.inheap) ism_common_freeAllocBuffer(&props);

            if (rc == OK)
            {
                assert(pMessage != NULL);

                ietrAsyncTransactionDataHandle_t hAsyncData = NULL;

                rc = ieds_publish(pThreadData,
                                  pSession ? pSession->pClient : NULL,
                                  pDestinationName,
                                  publishOptions,
                                  (ismEngine_Transaction_t *)hTran,
                                  pMessage,
                                  0,
                                  NULL,
                                  contextLength,
                                  &hAsyncData);

                if (rc == ISMRC_NeedStoreCommit)
                {
                    //The publish wants to go async.... get ready
                    rc = setupAsyncPublish( pThreadData
                                          , pSession
                                          , NULL
                                          , pContext
                                          , contextLength
                                          , pCallbackFn
                                          , &hAsyncData);

                    if (rc == ISMRC_AsyncCompletion)
                    {
                        //The async completion of the publish will release the session
                        goto mod_exit;
                    }
                }
            }
        }
        else
        {
            assert(destinationType == ismDESTINATION_REGEX_TOPIC);

            rc = iett_removeLocalRetainedMessages(pThreadData, pDestinationName);
        }
    }

    if (pSession != NULL) releaseSessionReference(pThreadData, pSession, false);

mod_exit:

    if (pMessage != NULL) iem_releaseMessage(pThreadData, pMessage);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
/// @brief  Republish the retained messages that the specified consumer is
///         eligible to receive.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hConsumer        Consumer to republish messages to
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_republishRetainedMessages(
    ismEngine_SessionHandle_t hSession,
    ismEngine_ConsumerHandle_t hConsumer,
    void *pContext,
    size_t contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    assert(pSession != NULL);
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Consumer_t *pConsumer = (ismEngine_Consumer_t *)hConsumer;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hConsumer, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p hConsumer %p)\n", __func__,
               hSession, hConsumer);

    ismEngine_CheckStructId(pSession->StrucId, ismENGINE_SESSION_STRUCID, ieutPROBE_001);
    assert(pConsumer != NULL);
    ismEngine_CheckStructId(pConsumer->StrucId, ismENGINE_CONSUMER_STRUCID, ieutPROBE_002);
    assert((pConsumer->pDestination->DestinationType == ismDESTINATION_TOPIC) ||
           (pConsumer->pDestination->DestinationType == ismDESTINATION_SUBSCRIPTION));

    if (pConsumer->fIsDestroyed)
    {
        rc = ISMRC_Destroyed;
        ism_common_setError(rc);
        goto mod_exit;
    }

    rc = iett_republishRetainedMessages(pThreadData,
                                        (ismEngine_Subscription_t *)(pConsumer->engineObject));

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Add Unreleased Delivery ID
///
/// Adds a delivery ID to the list of messages put on behalf of a
/// sending client, but not yet released by the client. The list is
/// associated with the session's client-state.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hTran            Transaction handle, may be NULL
/// @param[in]     unrelDeliveryId  Unreleased delivery ID
/// @param[out]    phUnrel          Returned unreleased-message handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_addUnreleasedDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)hTran;
    ismEngine_ClientState_t *pClient = pSession->pClient;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, hTran %p, unrelDeliveryId %u)\n", __func__,
               hSession, hTran, unrelDeliveryId);

    rc = iecs_addUnreleasedDelivery(pThreadData, pClient, pTran, unrelDeliveryId);
    if (rc == OK)
    {
        *phUnrel = (ismEngine_UnreleasedHandle_t)(uintptr_t)unrelDeliveryId;
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, hUnrel=%p\n", __func__, rc, *phUnrel);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief Callback for when ism_engine_removeUnreleasedDeliveryId goes async
///
/// @return OK
//****************************************************************************
int32_t WARN_CHECKRC ism_engine_removeUnreleasedDeliveryIdCompleted(
         ieutThreadData_t *pThreadData
       , int32_t  rc
       , ismEngine_AsyncData_t *asyncInfo
       , ismEngine_AsyncDataEntry_t *asyncEntry)
{
    assert(asyncEntry->Type == EngineRemoveUnreleasedDeliveryId);

    //Don't need this callback to be run again if we go async
    iead_popAsyncCallback( asyncInfo, asyncEntry->DataLen);

    ieutTRACEL(pThreadData, asyncInfo->pClient, ENGINE_CEI_TRACE, FUNCTION_IDENT "Client %p\n", __func__,
               asyncInfo->pClient);

    iecs_releaseClientStateReference(pThreadData, asyncInfo->pClient, false, false);

    return OK;
}

//****************************************************************************
/// @internal
///
/// @brief  Remove Unreleased Delivery ID
///
/// Removes a delivery ID from the list of messages put on behalf of
/// a sending client, once it has been released by the client. The
/// list is associated with the session's client-state.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hTran            Transaction handle, may be NULL
/// @param[in]     hUnrel           Unreleased-message handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_removeUnreleasedDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_UnreleasedHandle_t    hUnrel,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)hTran;
    ismEngine_ClientState_t *pClient = pSession->pClient;
    uint32_t unrelDeliveryId = (uint32_t)(uintptr_t)hUnrel;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hSession, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, hTran %p, hUnrel %p)\n", __func__,
               hSession, hTran, hUnrel);

    iecs_acquireClientStateReference(pClient);

    // Prepare to go async...
    ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
                {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineRemoveUnreleasedDeliveryId, NULL, 0, NULL,
                        {.internalFn = ism_engine_removeUnreleasedDeliveryIdCompleted } },
                {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineCaller, pContext,   contextLength, NULL,
                        {.externalFn = pCallbackFn }}};

    ismEngine_AsyncData_t asyncInfo = {ismENGINE_ASYNCDATA_STRUCID, pClient, IEAD_MAXARRAYENTRIES, 2, 0, true,  0, 0, asyncArray};

    rc = iecs_removeUnreleasedDelivery(pThreadData, pClient, pTran, unrelDeliveryId, &asyncInfo);

    if (rc != ISMRC_AsyncCompletion)
    {
        iecs_releaseClientStateReference(pThreadData, pClient, false, false);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  List Unreleased Delivery IDs
///
/// Lists the unreleased delivery IDs for the client-state. The callback
/// MUST NOT call any Engine operations.
///
/// @param[in]     hClient                 Client-state handle
/// @param[in]     pContext                Optional context for unreleased-message callback
/// @param[in]     pUnrelCallbackFunction  Unreleased-message callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_listUnreleasedDeliveryIds(
    ismEngine_ClientStateHandle_t   hClient,
    void *                          pContext,
    ismEngine_UnreleasedCallback_t  pUnrelCallbackFunction)
{
    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)hClient;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pClient);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hClient,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hClient %p)\n", __func__, hClient);

    // The client-state remains locked across the iteration of the unreleased delivery
    // information, and also the calls to the supplied callback.
    rc = iecs_listUnreleasedDeliveries(pClient, pContext, pUnrelCallbackFunction);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Create Message Consumer
///
/// Creates a message consumer for a destination. If the destination is a
/// topic, the consumer is associated with an anonymous, nondurable subscription.
///
/// @param[in]     hSession              Session handle
/// @param[in]     destinationType       Destination type
/// @param[in]     pDestinationName      Destination name
/// @param[in]     pSubAttributes        Subscription attributes to use when creating
///                                      the non-persistent subscription to service TOPIC
///                                      destinations (may be NULL for other destination types)
/// @param[in]     hOwningClient         Handle of the subscription owning client
/// @param[in]     pMessageContext       Optional context for message callback
/// @param[in]     messageContextLength  Length of data pointed to by pMessageContext
/// @param[in]     pMessageCallbackFn    Message-delivery callback
/// @param[in]     pConsumerProperties   Consumer properties
/// @param[in]     consumerOptions       Consumer options (ismENGINE_CONSUMER_*)
/// @param[out]    phConsumer            Returned consumer handle
/// @param[in]     pContext              Optional context for completion callback
/// @param[in]     contextLength         Length of data pointed to by pContext
/// @param[in]     pCallbackFn           Operation-completion callback
///
/// @remark The owning client handle is only used for a destination type of
///         ismDESTINATION_SUBSCRIPTION in order to locate the named subscription.
///         If this is NULL, the client owning the specified session handle is
///         assumed.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createConsumer(
    ismEngine_SessionHandle_t                  hSession,
    ismDestinationType_t                       destinationType,
    const char *                               pDestinationName,
    const ismEngine_SubscriptionAttributes_t * pSubAttributes,
    ismEngine_ClientStateHandle_t              hOwningClient,
    void *                                     pMessageContext,
    size_t                                     messageContextLength,
    ismEngine_MessageCallback_t                pMessageCallbackFn,
    const ism_prop_t *                         pConsumerProperties,
    uint32_t                                   consumerOptions,
    ismEngine_ConsumerHandle_t *               phConsumer,
    void *                                     pContext,
    size_t                                     contextLength,
    ismEngine_CompletionCallback_t             pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Consumer_t *pConsumer = NULL;
    ismEngine_Destination_t *pDestination = NULL;
    bool fEnableWaiter = false;
    bool fConsumerRegistered = false;
    int32_t rc = OK;
    iepiPolicyInfo_t *pValidatedPolicyInfo = NULL;
    char *SelectionString = NULL;
    ismRule_t *pSelectionRule = NULL;
    int32_t SelectionRuleLen = 0;
    bool fFreeSelectionRule = false;
    bool fDestructiveGet = ((consumerOptions & ismENGINE_CONSUMER_OPTION_BROWSE) == 0);
    concat_alloc_t serializedProperties = { NULL, 0, 0, 0, 0 };
    iereResourceSetHandle_t resourceSet = pSession->pClient->resourceSet;

    assert(pDestinationName != NULL);
    assert(strlen(pDestinationName) <= ismDESTINATION_NAME_LENGTH);

    ieutTRACEL(pThreadData, hSession, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, destinationType %d, pDestinationName '%s', pSubAttributes %p, consOpts: 0x%08x)\n", __func__,
               hSession, destinationType, pDestinationName, pSubAttributes, consumerOptions);

    // Check with security before we do anything other than trace the request
    if (destinationType == ismDESTINATION_TOPIC)
    {
        rc = ismEngine_security_validate_policy_func(pSession->pClient->pSecContext,
                                                     ismSEC_AUTH_TOPIC,
                                                     pDestinationName,
                                                     ismSEC_AUTH_ACTION_SUBSCRIBE,
                                                     ISM_CONFIG_COMP_ENGINE,
                                                     (void *)&pValidatedPolicyInfo);
    }
    else if (destinationType == ismDESTINATION_QUEUE)
    {
        bool isTemporary;

        ismEngine_ClientState_t *pCreator;

        rc = ieqn_isTemporaryQueue(pDestinationName, &isTemporary, &pCreator);

        if (rc == OK)
        {
            // We only allow consumers from the same client as the creator
            if (isTemporary == true)
            {
                if (pCreator != pSession->pClient)
                {
                    rc = ISMRC_NotAuthorized;
                }
            }
            // Full security check for permanent queues
            else
            {
                rc = ismEngine_security_validate_policy_func(pSession->pClient->pSecContext,
                                                             ismSEC_AUTH_QUEUE,
                                                             pDestinationName,
                                                             fDestructiveGet ? ismSEC_AUTH_ACTION_RECEIVE :
                                                                               ismSEC_AUTH_ACTION_BROWSE,
                                                             ISM_CONFIG_COMP_ENGINE,
                                                             NULL);
            }
        }
    }

    if (rc != OK) goto mod_exit;

    if (pConsumerProperties != NULL)
    {
        rc = ism_common_serializeProperties( (ism_prop_t *)pConsumerProperties
                                           , &serializedProperties);
        if (rc != OK)
        {
            goto mod_exit;
        }
    }

    // We only support selection on the consumer when consuming from
    // a queue for a subscription the selection string is handled
    // within the topicTree function
    if ((destinationType == ismDESTINATION_QUEUE) &&
        (consumerOptions & ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION))
    {
        ism_field_t selectionProperty = { 0 };

        if (serializedProperties.buf != NULL)
        {
            rc = ism_common_findPropertyName( &serializedProperties
                                            , ismENGINE_PROPERTY_SELECTOR
                                            , &selectionProperty );
        }

        if (selectionProperty.val.s == NULL)
        {
            // If the caller has requested that we perform message
            // selection but not provided us with selection strings
            // then we could assert but for the moment write an FFDC
            // and switch of message selection.
            ieutTRACE_FFDC( ieutPROBE_004, false,
                "ism_engine_createConsumer called with missing selection string", rc,
                NULL);
        }
        else if (selectionProperty.type == VT_ByteArray)
        {
            pSelectionRule = (ismRule_t *)selectionProperty.val.s;
            SelectionRuleLen = selectionProperty.len;
        }
        else
        {
            // The protocol will only ever pass a compiled selection string
            // but being able to support a text version is useful for UT
            assert(selectionProperty.type == VT_String);

            // And compile the selection string
            rc = ism_common_compileSelectRule(&pSelectionRule,
                                              (int *)&SelectionRuleLen,
                                              selectionProperty.val.s);
            if (rc == OK)
            {
                fFreeSelectionRule = true;
                SelectionString = selectionProperty.val.s;
            }
            else
            {
                assert(pSelectionRule == NULL);
                assert(SelectionRuleLen == 0);

                // If the caller has requested that we perform message
                // selection but we haven't been able to compile the
                // selection string then we could assert but for the
                // moment write an FFDC and switch off message selection.
                ieutTRACE_FFDC( ieutPROBE_005, false,
                    "Selection string compilation failed.", rc,
                    NULL);
            }
        }
    }

    assert(destinationType == ismDESTINATION_TOPIC ||
           destinationType == ismDESTINATION_SUBSCRIPTION ||
           destinationType == ismDESTINATION_QUEUE);
    assert((fDestructiveGet == true) || (destinationType == ismDESTINATION_QUEUE)); // no browse on subs
    assert((consumerOptions     & ~(ismENGINE_CREATECONSUMER_OPTION_VALID_MASK)) == 0);

    iere_primeThreadCache(pThreadData, resourceSet);
    rc = ieds_create_newDestination(pThreadData,
                                    resourceSet,
                                    destinationType, pDestinationName,
                                    &pDestination);
    if (rc == OK)
    {
        assert(serializedProperties.buf == NULL || serializedProperties.inheap == 1);

        size_t propertiesLength = (serializedProperties.buf == NULL) ? 0 : serializedProperties.used + 1;
        size_t totalContextLength = messageContextLength + propertiesLength;

        pConsumer = (ismEngine_Consumer_t *)iere_malloc(pThreadData,
                                                        resourceSet,
                                                        IEMEM_PROBE(iemem_externalObjs, 4),
                                                        sizeof(ismEngine_Consumer_t) + RoundUp4(totalContextLength) + SelectionRuleLen);
        if (pConsumer != NULL)
        {
            void *pContextData = (void *)(pConsumer + 1);
            void *pContextDataPos = pContextData;

            ismEngine_SetStructId(pConsumer->StrucId, ismENGINE_CONSUMER_STRUCID);
            pConsumer->pSession = pSession;
            pConsumer->pDestination = pDestination;
            pConsumer->pPrev = NULL;
            pConsumer->pNext = NULL;
            pConsumer->fDestructiveGet = fDestructiveGet;
            pConsumer->relinquishOnTerm = ismEngine_RelinquishType_NONE;
            pConsumer->DestinationOptions = 0;
            pConsumer->fIsDestroyed = false;
            pConsumer->fDestroyCompleted = false;
            pConsumer->fRecursiveDestroy = false;
            pConsumer->fFlowControl =false;
            pConsumer->fExpiringGet = false;
            pConsumer->fConsumeQos2OnDisconnect = false;
            pConsumer->fRedelivering = false;
            pConsumer->counts.parts.useCount        = 1; //Increased by 1 if consumer is enabled
            pConsumer->counts.parts.pendingAckCount = 0;
            if (messageContextLength != 0)
            {
                pConsumer->pMsgCallbackContext = pContextDataPos;
                memcpy(pConsumer->pMsgCallbackContext, pMessageContext, messageContextLength);
                pContextDataPos += messageContextLength;
            }
            else
            {
                pConsumer->pMsgCallbackContext = NULL;
            }
            pConsumer->pMsgCallbackFn = pMessageCallbackFn;
            pConsumer->pPendingDestroyContext = NULL;
            pConsumer->pPendingDestroyCallbackFn = NULL;
            pConsumer->engineObject = NULL;
            pConsumer->queueHandle = NULL;
            pConsumer->iemqWaiterStatus = IEWS_WAITERSTATUS_DISCONNECTED;
            pConsumer->iemqCursor.whole = 0;
            pConsumer->iemqNoMsgCheckVal = 0;
            pConsumer->failedSelectionCount = 0;
            pConsumer->successfulSelectionCount = 0;
            pConsumer->iemqPNext = NULL;
            pConsumer->iemqPPrev = NULL;
            pConsumer->iemqCachedSLEHdr = NULL;
            pConsumer->hMsgDelInfo = NULL;
            pConsumer->fShortDeliveryIds = (consumerOptions & ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID) ? true : false;
            pConsumer->fNonPersistentDlvyCount = (consumerOptions & ismENGINE_CONSUMER_OPTION_NONPERSISTENT_DELIVERYCOUNTING) ? true : false;
            pConsumer->fAcking = (consumerOptions & ismENGINE_CONSUMER_OPTION_ACK) ? true : false;

            if (propertiesLength != 0)
            {
                pConsumer->consumerProperties = pContextDataPos;
                memcpy(pConsumer->consumerProperties, serializedProperties.buf, propertiesLength);
                pConsumer->consumerProperties[propertiesLength-1] = '\0';
                pContextDataPos += propertiesLength;
            }
            else
            {
                pConsumer->consumerProperties = NULL;
            }

            if (pSelectionRule != NULL)
            {
                assert(pConsumer->consumerProperties != NULL);

                pConsumer->selectionRuleLen = SelectionRuleLen;
                pConsumer->selectionRule = (ismRule_t *)(pContextData + RoundUp4(totalContextLength));
                memcpy(pConsumer->selectionRule, pSelectionRule, SelectionRuleLen);
                if (fFreeSelectionRule)
                {
                    ism_common_freeSelectRule(pSelectionRule);
                }

                if (SelectionString != NULL && propertiesLength != 0)
                {
                    assert(serializedProperties.buf != NULL);
                    assert(SelectionString > serializedProperties.buf);
                    ptrdiff_t selectionStringOffset = SelectionString-serializedProperties.buf;
                    assert(selectionStringOffset < serializedProperties.used);

                    pConsumer->selectionString = pConsumer->consumerProperties + selectionStringOffset;
                }
                else
                {
                    pConsumer->selectionString = NULL;
                }
            }
            else
            {
                pConsumer->selectionString = NULL;
                pConsumer->selectionRule = NULL;
                pConsumer->selectionRuleLen = 0;
            }

            if (destinationType == ismDESTINATION_TOPIC)
            {
                assert(pSubAttributes != NULL);
                assert((pSubAttributes->subOptions & ~(ismENGINE_SUBSCRIPTION_OPTION_VALID_MASK)) == 0);

                ismEngine_SubscriptionHandle_t subHandle;

                iettCreateSubscriptionClientInfo_t clientInfo = {pSession->pClient, pSession->pClient, false};
                iettCreateSubscriptionInfo_t createSubInfo = {{pConsumerProperties}, 0,
                                                              NULL,
                                                              pValidatedPolicyInfo,
                                                              pDestinationName,
                                                              pSubAttributes->subOptions,
                                                              pSubAttributes->subId,
                                                              iettSUBATTR_NONE,
                                                              iettNO_ANONYMOUS_SHARER,
                                                              0, NULL, NULL, NULL};

                rc = iett_createSubscription(pThreadData,
                                             &clientInfo,
                                             &createSubInfo,
                                             &subHandle,
                                             NULL);
                if (rc == OK)
                {
                    assert(subHandle != NULL);

                    iett_registerConsumer(pThreadData, subHandle, pConsumer);

                    fConsumerRegistered = true;

                    rc = ieq_initWaiter(pThreadData, pConsumer->queueHandle, pConsumer);
                }

                assert(rc != ISMRC_AsyncCompletion);

                iett_createSubscriptionReleaseClients(pThreadData, &clientInfo);
            }
            else if (destinationType == ismDESTINATION_QUEUE)
            {
                rc = ieqn_openQueue(pThreadData,
                                    pSession->pClient,
                                    pDestinationName,
                                    pConsumer,
                                    NULL);

                if (rc == OK)
                {
                    assert(pConsumer->engineObject != NULL);
                    assert(pConsumer->queueHandle != NULL);

                    fConsumerRegistered = true;

                    rc = ieq_initWaiter(pThreadData, pConsumer->queueHandle, pConsumer);
                }
            }
            else if (destinationType == ismDESTINATION_SUBSCRIPTION)
            {
                bool owningClientLocked = false;
                bool subscriptionLocked = false;
                ismEngine_Subscription_t *subscription;
                ismEngine_ClientState_t *pOwningClient = (ismEngine_ClientState_t *)hOwningClient;

                // No owning client was specified, use the session's
                if (pOwningClient == NULL) pOwningClient = pSession->pClient;

                rc = OK;

                // If the client is this session's client, it is implicitly locked, if
                // not we need to get a reference to it to ensure it does not disappear.
                if (pOwningClient != pSession->pClient)
                {
                    rc = iecs_acquireClientStateReference(pOwningClient);

                    if (rc == OK)
                    {
                        owningClientLocked = true;
                    }
                }

                // All OK, find the subscription
                if (rc == OK)
                {
                    rc = iett_findClientSubscription(pThreadData,
                                                     pOwningClient->pClientId,
                                                     iett_generateClientIdHash(pOwningClient->pClientId),
                                                     pDestinationName,
                                                     iett_generateSubNameHash(pDestinationName),
                                                     &subscription);

                    // Didn't get the subscription
                    if (rc == OK)
                    {
                        subscriptionLocked = true;
                    }
                    else
                    {
                        if (rc == ISMRC_NotFound)
                        {
                            rc = ISMRC_DestNotValid;
                            ism_common_setErrorData(rc, "%s", pDestinationName);
                        }
                    }
                }

                // If we found the subscription check it.
                if (rc == OK)
                {
                    char *topicString = iett_getSubscriptionTopic(subscription);

                    assert(topicString != NULL);

                    // Check that the requesting client is authorized to subscribe on the topic for
                    // the subscription, if they are not, we fail the request to create the
                    // consumer.
                    rc = ismEngine_security_validate_policy_func(pSession->pClient->pSecContext,
                                                                 ismSEC_AUTH_TOPIC,
                                                                 topicString,
                                                                 ismSEC_AUTH_ACTION_SUBSCRIBE,
                                                                 ISM_CONFIG_COMP_ENGINE,
                                                                 NULL);

                    // If the requesting client is authorized for the topic, and this is a global
                    // shared persistent subscription, we also check the requesting client's authority to
                    // receive messages from the specified subscription name.
                    if (rc == OK &&
                        (subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) ==
                                ismENGINE_SUBSCRIPTION_OPTION_SHARED &&
                        (subscription->internalAttrs & iettSUBATTR_PERSISTENT) ==
                                iettSUBATTR_PERSISTENT &&
                        strcmp(pSession->pClient->pClientId, subscription->clientId) != 0)

                    {
                        rc = iepi_validateSubscriptionPolicy(pThreadData,
                                                             pSession->pClient->pSecContext,
                                                             subscription->subOptions,
                                                             pDestinationName,
                                                             ismSEC_AUTH_ACTION_RECEIVE,
                                                             NULL);
                    }
                }

                // Release the owning client lock
                if (owningClientLocked) (void)iecs_releaseClientStateReference(pThreadData, pOwningClient, false, false);

                //If we are a non-durable MQTT client connected as qos=2 to a shared sub, when we disconnect
                //messages in flight to us should be discarded to avoid duplicates
                if (rc == OK)
                {
                    //TODO: Code originally tested for only persistent subs: (subscription->internalAttrs & iettSUBATTR_PERSISTENT))
                    //      but if MQTT uses not persistent subs - we need to set the consumer flag for those MQTT consumers too!
                    if ( pSession->pClient->Durability == iecsNonDurable )
                    {
                        uint32_t subOptions = 0;

                        rc = iett_getSharedSubOptionsForClientId(pThreadData,
                                                                 subscription,
                                                                 pSession->pClient->pClientId,
                                                                 &subOptions);
                        if (rc == OK)
                        {
                            if ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK) == ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE)
                            {
                               ieutTRACEL(pThreadData, pConsumer,  ENGINE_UNUSUAL_TRACE, "NonDurable Qos=2 consumer on persistent sub %s (Client %s)\n",
                                           subscription->subName, pSession->pClient->pClientId);
                               pConsumer->fConsumeQos2OnDisconnect = true;
                            }
                        }
                        else
                        {
                            //Not an MQTT client connected to a shared sub.
                            //Nothing for us to do
                            rc = OK;
                        }
                    }
                }
                // If everything looks good, register this consumer
                if (rc == OK)
                {
                    iett_registerConsumer(pThreadData, subscription, pConsumer);

                    assert(pConsumer->engineObject != NULL);
                    assert(pConsumer->queueHandle != NULL);

                    fConsumerRegistered = true;

                    rc = ieq_initWaiter(pThreadData, pConsumer->queueHandle, pConsumer);
                }
                // Otherwise, release the subscription now if we have it
                else if (subscriptionLocked)
                {
                    (void)iett_releaseSubscription(pThreadData, subscription, false);
                }
            }
            else
            {
                ieutTRACE_FFDC( ieutPROBE_008, true,
                                "ism_engine_createConsumer called with invalid destination Type", rc,
                                "DestinationType", &destinationType, sizeof(destinationType),
                                NULL);
            }
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }
    }

    // Lock the session and add the consumer to the list of consumers
    if (rc == OK)
    {
        rc = ism_engine_lockSession(pSession);
        if (rc == OK)
        {
            if (pSession->fIsDestroyed)
            {
                ism_engine_unlockSession(pSession);
                rc = ISMRC_Destroyed;
                goto mod_exit;
            }

            if (pSession->pConsumerHead != NULL)
            {
                pConsumer->pNext = pSession->pConsumerHead;
                pSession->pConsumerHead->pPrev = pConsumer;
                pSession->pConsumerHead = pConsumer;
            }
            else
            {
                pSession->pConsumerHead = pConsumer;
                pSession->pConsumerTail = pConsumer;
            }
            __sync_fetch_and_add(&pSession->UseCount, 1);
            __sync_fetch_and_add(&pSession->PreNackAllCount, 1);

            //We're going to enable if the session is delivering and we've not been asked to start paused....
            if ((pSession->fIsDeliveryStarted)
                && ((consumerOptions & ismENGINE_CONSUMER_OPTION_PAUSE)== 0))
            {
                fEnableWaiter = true;
                acquireConsumerReference(pConsumer); //An enabled waiter counts as a reference
                __sync_fetch_and_add(&pSession->ActiveCallbacks, 1);

                rc = ieq_enableWaiter(pThreadData, pConsumer->queueHandle, pConsumer, IEQ_ENABLE_OPTION_DELIVER_LATER);

                if (rc == ISMRC_WaiterEnabled)
                {
                    //It was already enabled...reduce the counts we speculatively increased
                    releaseConsumerReference(pThreadData, pConsumer, false);
                    DEBUG_ONLY uint32_t oldvalue =  __sync_fetch_and_sub(&pSession->ActiveCallbacks, 1);
                    assert(oldvalue > 0);
                    rc = OK;
                }
                else if(rc == ISMRC_DisableWaiterCancel)
                {
                    //We've cancelled a disable, the disable callbacks and usecounts reductions
                    //etc... will still happen (but the waiter will just stay enabled) so we do need our
                    //speculative count increases
                    rc = OK;
                }
                else if (rc == ISMRC_WaiterInvalid)
                {
                    //The waiter status is not valid... reduce the counts we speculatively increased
                    releaseConsumerReference(pThreadData, pConsumer, false);
                    DEBUG_ONLY uint32_t oldvalue = __sync_fetch_and_sub(&pSession->ActiveCallbacks, 1);
                    assert(oldvalue > 0);
                }
            }

            ism_engine_unlockSession(pSession);

            if ((rc == OK) && fEnableWaiter)
            {
                //We don't need an extra use count on the consumer to protect this call to checkWaiters...
                //we haven't finished creating it yet, no-one should be deleting it.
                ieq_checkWaiters(pThreadData, pConsumer->queueHandle, NULL, NULL);
            }
        }
    }

    // All done
mod_exit:
    if (rc == OK)
    {
        *phConsumer = pConsumer;
    }
    else
    {
        if (pConsumer != NULL)
        {
            if (fConsumerRegistered)
            {
                if (destinationType == ismDESTINATION_QUEUE)
                {
                    ieqn_unregisterConsumer(pThreadData, pConsumer);
                }
                else
                {
                    iett_unregisterConsumer(pThreadData, pConsumer);
                }
            }

            iere_primeThreadCache(pThreadData, resourceSet);
            iere_freeStruct(pThreadData, resourceSet, iemem_externalObjs, pConsumer, pConsumer->StrucId);
        }

        if (pDestination != NULL)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_freeStruct(pThreadData, resourceSet, iemem_externalObjs, pDestination, pDestination->StrucId);
        }

        pConsumer = NULL;
    }

    ism_common_freeAllocBuffer(&serializedProperties);
    ieutTRACEL(pThreadData, pConsumer,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, hConsumer=%p\n", __func__, rc, pConsumer);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @brief  Create Message Consumer on a Remote Server
///
/// Creates a message consumer for a remote server.
///
/// @param[in]     hSession              Session handle
/// @param[in]     hRemoteServer         The remote server handle from which to consume
/// @param[in]     pMessageContext       Optional context for message callback
/// @param[in]     messageContextLength  Length of data pointed to by pMessageContext
/// @param[in]     pMessageCallbackFn    Message-delivery callback
/// @param[in]     consumerOptions       Consumer options (ismENGINE_CONSUMER_*)
/// @param[out]    phConsumer            Returned consumer handle
/// @param[in]     pContext              Optional context for completion callback
/// @param[in]     contextLength         Length of data pointed to by pContext
/// @param[in]     pCallbackFn           Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The operation may complete synchronously or asynchronously. If it
/// completes synchronously, the return code indicates the completion
/// status.
///
/// If the operation completes asynchronously, the return code from
/// this function will be ISMRC_AsyncCompletion. The actual
/// completion of the operation will be signalled by a call to the
/// operation-completion callback, if one is provided. When the
/// operation becomes asynchronous, a copy of the context will be
/// made into memory owned by the Engine. This will be freed upon
/// return from the operation-completion callback. Because the
/// completion is asynchronous, the call to the operation-completion
/// callback might occur before this function returns.
///
/// On success, for synchronous completion, the consumer handle
/// will be returned in an output parameter, while, for asynchronous
/// completion, the consumer handle will be passed to the
/// operation-completion callback.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createRemoteServerConsumer(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_RemoteServerHandle_t  hRemoteServer,
    void *                          pMessageContext,
    size_t                          messageContextLength,
    ismEngine_MessageCallback_t     pMessageCallbackFn,
    uint32_t                        consumerOptions,
    ismEngine_ConsumerHandle_t *    phConsumer,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Consumer_t *pConsumer = NULL;
    ismEngine_RemoteServer_t *pRemoteServer = (ismEngine_RemoteServer_t *)hRemoteServer;
    ismDestinationType_t destinationType;
    ismEngine_Destination_t *pDestination = NULL;
    bool fEnableWaiter = false;
    bool fConsumerRegistered = false;
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = pSession->pClient->resourceSet;

    assert(pRemoteServer != NULL);
    ismEngine_CheckStructId(pRemoteServer->StrucId, ismENGINE_REMOTESERVER_STRUCID, ieutPROBE_001);
    assert(pRemoteServer->serverUID != NULL);
    assert((consumerOptions & ~(ismENGINE_CREATEREMOTESERVERCONSUMER_OPTION_VALID_MASK)) == 0);
    assert(pSession->pClient->protocolId == PROTOCOL_ID_FWD);
    assert((consumerOptions & (ismENGINE_CONSUMER_OPTION_HIGH_QOS | ismENGINE_CONSUMER_OPTION_LOW_QOS)) !=
           (ismENGINE_CONSUMER_OPTION_HIGH_QOS | ismENGINE_CONSUMER_OPTION_LOW_QOS));

    ieutTRACEL(pThreadData, hSession, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, pRemoteServer %p, serverName '%s', serverUID '%s', consOpts: 0x%08x)\n", __func__,
               hSession, pRemoteServer, pRemoteServer->serverName, pRemoteServer->serverUID, consumerOptions);

    // Whether we should consume from the low or high QoS queue is based on the consumer
    // options
    if (consumerOptions & ismENGINE_CONSUMER_OPTION_HIGH_QOS)
    {
        destinationType = ismDESTINATION_REMOTESERVER_HIGH;
    }
    else
    {
        destinationType = ismDESTINATION_REMOTESERVER_LOW;
    }

    iere_primeThreadCache(pThreadData, resourceSet);
    rc = ieds_create_newDestination(pThreadData,
                                    resourceSet,
                                    destinationType, pRemoteServer->serverUID,
                                    &pDestination);
    if (rc == OK)
    {
        pConsumer = (ismEngine_Consumer_t *)iere_malloc(pThreadData,
                                                        resourceSet,
                                                        IEMEM_PROBE(iemem_externalObjs, 9),
                                                        sizeof(ismEngine_Consumer_t) + RoundUp4(messageContextLength));
        if (pConsumer != NULL)
        {
            ismEngine_SetStructId(pConsumer->StrucId, ismENGINE_CONSUMER_STRUCID);
            pConsumer->pSession = pSession;
            pConsumer->pDestination = pDestination;
            pConsumer->pPrev = NULL;
            pConsumer->pNext = NULL;
            pConsumer->fDestructiveGet = true;
            pConsumer->relinquishOnTerm = ismEngine_RelinquishType_NONE;
            pConsumer->DestinationOptions = 0;
            pConsumer->fIsDestroyed = false;
            pConsumer->fDestroyCompleted = false;
            pConsumer->fRecursiveDestroy = false;
            pConsumer->fFlowControl =false;
            pConsumer->fRedelivering = false;
            pConsumer->fExpiringGet = false;
            pConsumer->fConsumeQos2OnDisconnect = false;
            pConsumer->counts.parts.useCount        = 1; //Increased by 1 if consumer is enabled
            pConsumer->counts.parts.pendingAckCount = 0;
            if (messageContextLength != 0)
            {
                pConsumer->pMsgCallbackContext = pConsumer + 1;
                memcpy((pConsumer + 1), pMessageContext, messageContextLength);
            }
            else
            {
                pConsumer->pMsgCallbackContext = NULL;
            }
            pConsumer->pMsgCallbackFn = pMessageCallbackFn;
            pConsumer->pPendingDestroyContext = NULL;
            pConsumer->pPendingDestroyCallbackFn = NULL;
            pConsumer->engineObject = NULL;
            pConsumer->queueHandle = NULL;
            pConsumer->iemqWaiterStatus = IEWS_WAITERSTATUS_DISCONNECTED;
            pConsumer->iemqCursor.whole = 0;
            pConsumer->iemqNoMsgCheckVal = 0;
            pConsumer->failedSelectionCount = 0;
            pConsumer->successfulSelectionCount = 0;
            pConsumer->iemqPNext = NULL;
            pConsumer->iemqPPrev = NULL;
            pConsumer->iemqCachedSLEHdr = NULL;
            pConsumer->hMsgDelInfo = NULL;
            pConsumer->fShortDeliveryIds = (consumerOptions & ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID) ? true : false;
            pConsumer->fAcking = (consumerOptions & ismENGINE_CONSUMER_OPTION_ACK) ? true : false;
            pConsumer->consumerProperties = NULL;
            pConsumer->selectionString = NULL;
            pConsumer->selectionRule = NULL;
            pConsumer->selectionRuleLen = 0;

            // Increment the useCount on the remote server for this consumer
            iers_acquireServerReference(pRemoteServer);

            // Register the consumer
            iers_registerConsumer(pThreadData, pRemoteServer, pConsumer, destinationType);

            assert(pConsumer->engineObject != NULL);
            assert(pConsumer->queueHandle != NULL);

            fConsumerRegistered = true;

            rc = ieq_initWaiter(pThreadData, pConsumer->queueHandle, pConsumer);
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }
    }

    // Lock the session and add the consumer to the list of consumers
    if (rc == OK)
    {
        rc = ism_engine_lockSession(pSession);
        if (rc == OK)
        {
            if (pSession->fIsDestroyed)
            {
                ism_engine_unlockSession(pSession);
                rc = ISMRC_Destroyed;
                goto mod_exit;
            }

            if (pSession->pConsumerHead != NULL)
            {
                pConsumer->pNext = pSession->pConsumerHead;
                pSession->pConsumerHead->pPrev = pConsumer;
                pSession->pConsumerHead = pConsumer;
            }
            else
            {
                pSession->pConsumerHead = pConsumer;
                pSession->pConsumerTail = pConsumer;
            }
            __sync_fetch_and_add(&pSession->UseCount, 1);
            __sync_fetch_and_add(&pSession->PreNackAllCount, 1);

            //We're going to enable if the session is delivering and we've not been asked to start paused....
            if ((pSession->fIsDeliveryStarted)
                && ((consumerOptions & ismENGINE_CONSUMER_OPTION_PAUSE)== 0))
            {
                fEnableWaiter = true;
                acquireConsumerReference(pConsumer); //An enabled waiter counts as a reference
                __sync_fetch_and_add(&pSession->ActiveCallbacks, 1);

                rc = ieq_enableWaiter(pThreadData, pConsumer->queueHandle, pConsumer, IEQ_ENABLE_OPTION_DELIVER_LATER);

                if (rc == ISMRC_WaiterEnabled)
                {
                    //It was already enabled...reduce the counts we speculatively increased
                    releaseConsumerReference(pThreadData, pConsumer, false);
                    DEBUG_ONLY uint32_t oldvalue =  __sync_fetch_and_sub(&pSession->ActiveCallbacks, 1);
                    assert(oldvalue > 0);
                    rc = OK;
                }
                else if(rc == ISMRC_DisableWaiterCancel)
                {
                    //We've cancelled a disable, the disable callbacks and usecounts reductions
                    //etc... will still happen (but the waiter will just stay enabled) so we do need our
                    //speculative count increases
                    rc = OK;
                }
                else if (rc == ISMRC_WaiterInvalid)
                {
                    //The waiter status is not valid... reduce the counts we speculatively increased
                    releaseConsumerReference(pThreadData, pConsumer, false);
                    DEBUG_ONLY uint32_t oldvalue = __sync_fetch_and_sub(&pSession->ActiveCallbacks, 1);
                    assert(oldvalue > 0);
                }
            }

            ism_engine_unlockSession(pSession);

            if ((rc == OK) && fEnableWaiter)
            {
                //We don't need an extra use count on the consumer to protect this call to checkWaiters...
                //we haven't finished creating it yet, no-one should be deleting it.
                ieq_checkWaiters(pThreadData, pConsumer->queueHandle, NULL, NULL);
            }
        }
    }

    // All done
mod_exit:
    if (rc == OK)
    {
        *phConsumer = pConsumer;
    }
    else
    {
        if (pConsumer != NULL)
        {
            if (fConsumerRegistered)
            {
                iers_unregisterConsumer(pThreadData, pConsumer, destinationType);
            }

            iere_primeThreadCache(pThreadData, resourceSet);
            iere_freeStruct(pThreadData, resourceSet, iemem_externalObjs, pConsumer, pConsumer->StrucId);
        }

        if (pDestination != NULL)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_freeStruct(pThreadData, resourceSet, iemem_externalObjs, pDestination, pDestination->StrucId);
        }

        pConsumer = NULL;
    }

    ieutTRACEL(pThreadData, pConsumer,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, hConsumer=%p\n", __func__, rc, pConsumer);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @brief Function to add the contents of an ismEngine_Session_t to a dump
///
/// @param[in]     pConsumer  Pointer to the consumer to dump
/// @param[in]     dumpHdl    Handle to the dump
//****************************************************************************
void dumpSession(ieutThreadData_t *pThreadData,
                 ismEngine_Session_t *pSession,
                 iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    if ((iedm_dumpStartObject(dump, pSession)) == true)
    {
        iedm_dumpStartGroup(dump, "Session");

        iedm_dumpData(dump, "ismEngine_Session_t", pSession,
                      iere_usable_size(iemem_externalObjs, pSession));

        if (dump->detailLevel >= iedmDUMP_DETAIL_LEVEL_3)
        {
            (void)ism_engine_lockSession(pSession);

            // Producers...
            ismEngine_Producer_t *pProducer = pSession->pProducerHead;

            while(pProducer != NULL)
            {
                dumpProducer(pThreadData, pProducer, dumpHdl);
                pProducer = pProducer->pNext;
            }

            // Consumers...
            ismEngine_Consumer_t *pConsumer = pSession->pConsumerHead;

            while (pConsumer != NULL)
            {
                dumpConsumer(pThreadData, pConsumer, dumpHdl);
                pConsumer = pConsumer->pNext;
            }

            ism_engine_unlockSession(pSession);
        }

        iedm_dumpEndGroup(dump);
        iedm_dumpEndObject(dump, pSession);
    }
}

//****************************************************************************
/// @brief Function to add the contents of an ismEngine_Producer_t to a dump
///
/// @param[in]     pProducer  Pointer to the producer to dump
/// @param[in]     dumpHdl    Handle to the dump
//****************************************************************************
void dumpProducer(ieutThreadData_t *pThreadData,
                  ismEngine_Producer_t *pProducer,
                  iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    if (iedm_dumpStartObject(dump, pProducer) == true)
    {
        iedm_dumpData(dump, "ismEngine_Producer_t", pProducer,
                      iere_usable_size(iemem_externalObjs, pProducer));

        // If the destination is non-null, dump it
        if (pProducer->pDestination != NULL)
        {
            iedm_dumpData(dump, "ismEngine_Destination_t", pProducer->pDestination,
                          iere_usable_size(iemem_externalObjs, pProducer->pDestination));

            // Drill down into the subscription / queue for detail level 4 and above
            if (dump->detailLevel >= iedmDUMP_DETAIL_LEVEL_4)
            {
                if (pProducer->engineObject != NULL)
                {
                    assert(pProducer->pDestination->DestinationType == ismDESTINATION_QUEUE);
                    ieqn_dumpQueue(pThreadData, pProducer->engineObject, dumpHdl);
                }
            }
        }

        iedm_dumpEndObject(dump, pProducer);
    }
}

//****************************************************************************
/// @brief Function to add the contents of an ismEngine_Consumer_t to a dump
///
/// @param[in]     pConsumer  Pointer to the consumer to dump
/// @param[in]     dumpHdl    Handle to the dump
//****************************************************************************
void dumpConsumer(ieutThreadData_t *pThreadData,
                  ismEngine_Consumer_t *pConsumer,
                  iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    if (iedm_dumpStartObject(dump, pConsumer) == true)
    {
        iedm_dumpData(dump, "ismEngine_Consumer_t", pConsumer,
                      iere_usable_size(iemem_externalObjs, pConsumer));

        // If the destination is non-null, dump it 
        if (pConsumer->pDestination != NULL)
        {
            iedm_dumpData(dump, "ismEngine_Destination_t", pConsumer->pDestination,
                          iere_usable_size(iemem_externalObjs, pConsumer->pDestination));

            // Drill down into the subscription / queue for detail level 4 and above
            if (dump->detailLevel >= iedmDUMP_DETAIL_LEVEL_4)
            {
                if (pConsumer->engineObject != NULL)
                {
                    switch(pConsumer->pDestination->DestinationType)
                    {
                        case ismDESTINATION_TOPIC:
                        case ismDESTINATION_SUBSCRIPTION:
                            iett_dumpSubscription(pThreadData, pConsumer->engineObject, dumpHdl);
                            break;
                        case ismDESTINATION_QUEUE:
                            ieqn_dumpQueue(pThreadData, pConsumer->engineObject, dumpHdl);
                            break;
                        case ismDESTINATION_REMOTESERVER_HIGH:
                        case ismDESTINATION_REMOTESERVER_LOW:
                            iers_dumpServer(pThreadData, pConsumer->engineObject, dumpHdl);
                            break;
                        default:
                            break;
                    }
                }
            }
        }

        iedm_dumpEndObject(dump, pConsumer);
    }
}

//****************************************************************************
/// @internal
///
/// @brief  Destroy Message Consumer
///
/// Destroys a message consumer. If the consumer is an anonymous,
/// nondurable subscription, the subscription is destroyed and
/// undelivered messages are discarded. Upon completion, no more
/// messages are delivered to the consumer.
///
/// @param[in]     hConsumer        Consumer handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_destroyConsumer(
    ismEngine_ConsumerHandle_t      hConsumer,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Consumer_t *pConsumer = (ismEngine_Consumer_t *)hConsumer;
    assert(pConsumer != NULL);
    ismEngine_Session_t *pSession = pConsumer->pSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hConsumer,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hConsumer %p)\n", __func__, hConsumer);

    ismEngine_CheckStructId(pConsumer->StrucId, ismENGINE_CONSUMER_STRUCID, ieutPROBE_001);

    rc = ism_engine_lockSession(pSession);
    if (rc == OK)
    {
        if (pConsumer->fIsDestroyed)
        {
            ism_engine_unlockSession(pSession);
            rc = ISMRC_Destroyed;
            ism_common_setError(rc);
            goto mod_exit;
        }

        //Prpare in case we need to go asynchronous, we used to only do this prepare if the waiter was
        //enabled, but even if it's not a resumeMessageDelivery could have the waiter in its hand and be about to
        //enable it, in which case we'll still go async so there is no way to be sure here that we won't go async
        //in any case
        if (contextLength > 0)
        {
            pConsumer->pPendingDestroyContext = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_callbackContext, 6), contextLength);
            if (pConsumer->pPendingDestroyContext != NULL)
            {
                memcpy(pConsumer->pPendingDestroyContext, pContext, contextLength);
            }
            else
            {
                ism_engine_unlockSession(pSession);
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }
        }

        pConsumer->pPendingDestroyCallbackFn = pCallbackFn;

        pConsumer->fIsDestroyed = true;

        if (pConsumer->pPrev == NULL)
        {
            if (pConsumer->pNext == NULL)
            {
                assert(pSession->pConsumerHead == pConsumer);
                assert(pSession->pConsumerTail == pConsumer);

                pSession->pConsumerHead = NULL;
                pSession->pConsumerTail = NULL;
            }
            else
            {
                assert(pSession->pConsumerHead == pConsumer);

                pSession->pConsumerHead = pConsumer->pNext;
                pConsumer->pNext->pPrev = NULL;
            }
        }
        else
        {
            if (pConsumer->pNext == NULL)
            {
                assert(pSession->pConsumerTail == pConsumer);

                pSession->pConsumerTail = pConsumer->pPrev;
                pConsumer->pPrev->pNext = NULL;
            }
            else
            {
                pConsumer->pNext->pPrev = pConsumer->pPrev;
                pConsumer->pPrev->pNext = pConsumer->pNext;
            }
        }

        pConsumer->pPrev = NULL;
        pConsumer->pNext = NULL;

        ism_engine_unlockSession(pSession);
    }

    if (rc == OK)
    {

        // We can't report destroyConsumer as complete until we are sure they won't
        // receive more messages...
        rc = ieq_disableWaiter(pThreadData, pConsumer->queueHandle, pConsumer);

        if (rc == ISMRC_WaiterDisabled)
        {
            //Waiter was already disabled....not an error in this case
            rc = OK;
        }
        assert((rc == OK) || (rc == ISMRC_AsyncCompletion));


        uint32_t remainingRefs = releaseConsumerReference(pThreadData, pConsumer, true);

        if (remainingRefs > 0)
        {
            rc = ISMRC_AsyncCompletion;
        }
        else
        {
            rc = ISMRC_OK;
        }
    }

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}


// Internal function to destroy a consumer as part of recursive clean-up
static inline void destroyConsumerInternal(ieutThreadData_t *pThreadData,
                                           ismEngine_Consumer_t *pConsumer)
{
    assert(pConsumer->fRecursiveDestroy);
    int32_t rc = OK;

    rc = ieq_disableWaiter(pThreadData, pConsumer->queueHandle, pConsumer);

    if (rc == ISMRC_WaiterDisabled)
    {
        //Waiter was already disabled....not an error in this case
        rc = OK;
    }
    assert((rc == OK) || (rc == ISMRC_AsyncCompletion));

    //Release the reference that would usually be released at end of destroyConsumer
    releaseConsumerReference(pThreadData, pConsumer, false);
}


// Called automagically when the usecounts drops to 0. Should
// not be called from outside the reduce usecounts functions
static inline void disconnectThenFreeConsumer(ieutThreadData_t *pThreadData, ismEngine_Consumer_t *pConsumer)
{
    iereResourceSetHandle_t resourceSet = pConsumer->pSession->pClient->resourceSet;

    if (pConsumer->pDestination != NULL)
    {
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_freeStruct(pThreadData,
                        resourceSet,
                        iemem_externalObjs,
                        pConsumer->pDestination, pConsumer->pDestination->StrucId);
    }

    if (pConsumer->iemqCachedSLEHdr != NULL)
    {
        iemem_freeStruct(pThreadData, iemem_localTransactions,
                         pConsumer->iemqCachedSLEHdr, pConsumer->iemqCachedSLEHdr->StrucId);
    }

    // Now we're sure we have no acks...finally we can terminate
    // and that will free when it completes
    ieq_termWaiter(pThreadData, pConsumer->queueHandle, pConsumer);
}

// This is called when we can tell the client that destroy is finished...i.e. they
// won't be sent any more messages... the consumer may actually linger on if there are remaining acks.
// If fInlineDestroy is true, it's being called synchronously from destroyConsumer.
// When it's done inline, the destroy completion callback does not need to be called because the caller
// is synchronously completing the destroy.
void finishDestroyConsumer(ieutThreadData_t *pThreadData,
                           ismEngine_Consumer_t *pConsumer,
                           bool fInlineDestroy)
{
    // We can tell the caller the consumer is destroyed (and unregister it) even if we have acks pending
    ieutTRACEL(pThreadData, pConsumer, ENGINE_HIGH_TRACE, "Finishing destroy for consumer %p (inline = %d)\n", pConsumer, (int)fInlineDestroy);

    ismEngine_CompletionCallback_t pPendingDestroyCallbackFn = pConsumer->pPendingDestroyCallbackFn;
    void *pPendingDestroyContext = pConsumer->pPendingDestroyContext;

    //For consumers created by the expiring get code, we use the MsgCallbackContext as the destroycontext to save duplication
    //(and an extra malloc/free)
    void *expGetContext = NULL;
    if (pConsumer->fExpiringGet)
    {
        expGetContext  = *(void **)(pConsumer->pMsgCallbackContext); //Copy as can't refer after we unlock consumer
        ieutTRACEL(pThreadData, expGetContext, ENGINE_HIGH_TRACE, "expiring get context: %p\n", expGetContext);
    }

    switch(pConsumer->pDestination->DestinationType)
    {
        case ismDESTINATION_TOPIC:
        case ismDESTINATION_SUBSCRIPTION:
            iett_unregisterConsumer(pThreadData, pConsumer);
            break;
        case ismDESTINATION_QUEUE:
            ieqn_unregisterConsumer(pThreadData, pConsumer);
            break;
        case ismDESTINATION_REMOTESERVER_LOW:
        case ismDESTINATION_REMOTESERVER_HIGH:
            iers_unregisterConsumer(pThreadData, pConsumer, pConsumer->pDestination->DestinationType);
            break;
        default:
            assert(false);
            break;
    }

    // This consumer may live on zombie if there are pending acks...
    // but we need to release our handle on the Session so it can be destroyed
    // (and tidy up the pending acks)...
    ismEngine_Session_t *pSession = pConsumer->pSession;

    //Mark the destroy as completed...we can no longer refer to the consumer...
    //but we want to do this BEFORE we call the callback as that may clean up our remaining
    //acks which (if the ackcount hits zero) will wait for the destroy we're doing to be
    //completed.
    bool alreadyFinished = __sync_lock_test_and_set(&(pConsumer->fDestroyCompleted), 1);

    if (alreadyFinished)
    {
        //We already have weird memory issues.. we may as well refer to the already
        //freed memory in our FFDC.... if we take a core so be it.
        ieutTRACE_FFDC(ieutPROBE_001, false,
                       "Destroying destroyed consumer", ISMRC_Error,
                       "Consumer", pConsumer, sizeof(ismEngine_Consumer_t),
                       NULL);
    }


    // If we're not running in-line, we're completing this asynchronously and
    // may have been given a completion callback for the destroy operation
    if (!fInlineDestroy && (pPendingDestroyCallbackFn != NULL))
    {
        pPendingDestroyCallbackFn( OK
                                 , NULL
                                 , expGetContext ? &expGetContext : pPendingDestroyContext);
    }
    if (pPendingDestroyContext != NULL)
    {
        iemem_free(pThreadData, iemem_callbackContext, pPendingDestroyContext);
    }

    //Reduce session consumer count (which may cause remaining acks to be
    //processed which might cause the consumer to be deallocated)
    reducePreNackAllCount(pThreadData, pSession);
}

// Acquire a reference to a consumer
void acquireConsumerReference(ismEngine_Consumer_t *pConsumer)
{
    // Unlike when we decrement...during increment we don't care about the value of the
    // pendingAckCount so we can increase the usecount we care about separately
    uint32_t usecount = __sync_fetch_and_add(&(pConsumer->counts.parts.useCount), 1);
    if (usecount == 0)
    {
        ieutTRACE_FFDC(ieutPROBE_002, false,
                       "Acquiring destroyed consumer", ISMRC_Error,
                       "Consumer", pConsumer, sizeof(ismEngine_Consumer_t),
                       NULL);
    }
}

// Release a reference to a consumer, deallocating the consumer when
// the use-count and ack count drops to zero.
// If the usecount is zero but a non-zero ackcount, the consumer is a zombie
// and no longer is detectable from outside the engine (e.g. destroyConsumer can have returned successfully).
uint32_t releaseConsumerReference(ieutThreadData_t *pThreadData,
                                  ismEngine_Consumer_t *pConsumer,
                                  bool fInlineDestroy)
{
    bool doneDecrease = false;
    ismEngine_ConsumerCounts_t newCount;

    // Need to know exactly what /both/ usecounts were as we do the decrease so
    // we CAS them together...
    do
    {
        ismEngine_ConsumerCounts_t oldCount;

        oldCount.whole = pConsumer->counts.whole;

        assert(oldCount.parts.useCount > 0);

        newCount.parts.useCount        = oldCount.parts.useCount - 1;
        newCount.parts.pendingAckCount = oldCount.parts.pendingAckCount;

        doneDecrease = __sync_bool_compare_and_swap( &(pConsumer->counts.whole)
                                                     , oldCount.whole
                                                     , newCount.whole);
    }
    while (!doneDecrease);

    if (newCount.parts.useCount == 0)
    {
        //The consumer is "destroyed" though it may live on as a zombie if there are unacked messages...
        finishDestroyConsumer(pThreadData, pConsumer, fInlineDestroy);
    }

    if (newCount.whole == 0)
    {
        //We know that finishDestroyConsumer() will have been called (a couple of lines above).
        //If the last count removed was an ack it's more complicated (see reduceConsumerAckCount for
        //more details)

        //We can just deallocate...
        ieutTRACEL(pThreadData, pConsumer, ENGINE_HIGH_TRACE, "Disconnecting consumer %p from %s\n", pConsumer, __func__);
        disconnectThenFreeConsumer(pThreadData, pConsumer);
    }

    return newCount.parts.useCount;
}

void increaseConsumerAckCount(ismEngine_Consumer_t *pConsumer)
{
    //If the usecount is 0, this consumer has been destroyed (though it may
    //live on whilst we wait for unacked messages). No /new/ unacked messages sould be added
    if (pConsumer->counts.parts.useCount == 0)
    {
        ieutTRACE_FFDC(ieutPROBE_001, false,
                       "Adding unacked message to destroyed consumer", ISMRC_Error,
                       "Consumer", pConsumer, sizeof(ismEngine_Consumer_t),
                       NULL);
    }
    // Unlike when we decrement...during increment we don't care about the value of
    // the useCount so we can increase the pendingAckCount we care about separately
    __sync_add_and_fetch(&(pConsumer->counts.parts.pendingAckCount), 1);
}

void decreaseConsumerAckCount(ieutThreadData_t *pThreadData,
                              ismEngine_Consumer_t *pConsumer,
                              uint32_t numAcks)
{
    bool doneDecrease = false;
    ismEngine_ConsumerCounts_t newCount;

    // Need to know exactly what /both/ usecounts were as we do the decrease so
    // we CAS them together...
    do
    {
        ismEngine_ConsumerCounts_t oldCount;

        oldCount.whole = pConsumer->counts.whole;

        assert(oldCount.parts.pendingAckCount >= numAcks);

        newCount.parts.useCount        = oldCount.parts.useCount;
        newCount.parts.pendingAckCount = oldCount.parts.pendingAckCount-numAcks;

        doneDecrease = __sync_bool_compare_and_swap( &(pConsumer->counts.whole)
                , oldCount.whole
                , newCount.whole);
    }
    while (!doneDecrease);

    if (newCount.whole == 0)
    {
        //We were a last count, but another thread could have just made the refcount hit
        //zero and not yet finished destroy consumer...so we wait until the consumer
        //is marked as destroycompleted before we deallocate...
        while(!pConsumer->fDestroyCompleted)
        {
            ismEngine_ReadMemoryBarrier();
        }

        ieutTRACEL(pThreadData, pConsumer, ENGINE_HIGH_TRACE, "Disconnecting consumer %p from %s\n", pConsumer, __func__);
        disconnectThenFreeConsumer(pThreadData, pConsumer);
    }
}

//****************************************************************************
/// @brief  Determines whether there are gettable messages for a consumer
///
/// Returns OK if there are messages available to be delivered to a consumer.
/// Ths does not include uncommitted gets or uncommitted puts.
///
/// @param[in]     hConsumer            Consumer handle
///
/// @return OK if there were (at least for a moment when the check occurred) available messages
///         ISMRC_NoMsgAvail if there were, at least when the check occurred, no available messages
///         or an ISMRC_ value detailing an error
///
//****************************************************************************

XAPI int32_t WARN_CHECKRC ism_engine_checkAvailableMessages(
                ismEngine_Consumer_t *pConsumer)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    assert(pConsumer != NULL);
    int32_t rc = OK;

    ismEngine_CheckStructId(pConsumer->StrucId, ismENGINE_CONSUMER_STRUCID, ieutPROBE_001);

    rc = ieq_checkAvailableMsgs(pConsumer->queueHandle, pConsumer);

    ieutTRACEL(pThreadData, pConsumer,  ENGINE_CEI_TRACE, "%s: pConsumer: %p, rc=%d\n", __func__, pConsumer, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Create a temporary destination
///
/// Creates a temporary destination and associates it with the specified client
///
/// @param[in]     hClient            Client handle
/// @param[in]     destinationType    Destination type
/// @param[in]     pDestinationName   Destination name
/// @param[in]     pDestinationProps  Optional properties to use when creating
///                                   the destination (MaxMessages).
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createTemporaryDestination(
    ismEngine_ClientStateHandle_t   hClient,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ism_prop_t *                    pDestinationProps,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)hClient;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pClient);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hClient, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hClient %p, destinationType %d, pDestinationName '%s')\n", __func__,
               hClient, destinationType, pDestinationName);

    // No authority checking when creating temporary destination

    assert(pClient != NULL);
    assert(pDestinationName != NULL);
    assert(destinationType == ismDESTINATION_QUEUE); // Only queues at the moment
    assert(strlen(pDestinationName) <= ismDESTINATION_NAME_LENGTH);

    ismEngine_CheckStructId(pClient->StrucId, ismENGINE_CLIENT_STATE_STRUCID, ieutPROBE_001);

    if (destinationType == ismDESTINATION_QUEUE)
    {
        ieqnQueueHandle_t pQueue = NULL;

        rc = ieqn_createQueue(pThreadData,
                              pDestinationName,
                              multiConsumer,
                              ismQueueScope_Client,
                              pClient,
                              pDestinationProps,
                              NULL,  // Properties not qualified for temporary queues
                              &pQueue);

        // If the queue was newly created, add it to the group of temporary
        // queues for this client
        if (rc == OK && pQueue != NULL)
        {
            iecs_lockClientState(pClient);
            ieqn_addQueueToGroup(pThreadData, pQueue, &pClient->pTemporaryQueues);
            iecs_unlockClientState(pClient);
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Destroy a temporary destination
///
/// Destroy a temporary destination associated with the specified client
///
/// @param[in]     hClient            Client handle
/// @param[in]     destinationType    Destination type
/// @param[in]     pDestinationName   Destination name
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_destroyTemporaryDestination(
    ismEngine_ClientStateHandle_t   hClient,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)hClient;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pClient);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hClient, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hClient %p, destinationType %d, pDestinationName '%s')\n", __func__,
               hClient, destinationType, pDestinationName);

    // No authority checking when destroying temporary destination

    assert(pClient != NULL);
    assert(pDestinationName != NULL);
    assert(destinationType == ismDESTINATION_QUEUE); // Only queues at the moment
    assert(strlen(pDestinationName) <= ismDESTINATION_NAME_LENGTH);

    ismEngine_CheckStructId(pClient->StrucId, ismENGINE_CLIENT_STATE_STRUCID, ieutPROBE_001);

    if (destinationType == ismDESTINATION_QUEUE)
    {
        // Remove it from our client's list of temporary queues
        iecs_lockClientState(pClient);
        rc = ieqn_removeQueueFromGroup(pThreadData, pDestinationName, &pClient->pTemporaryQueues);
        iecs_unlockClientState(pClient);

        if (rc == OK)
        {
            rc = ieqn_destroyQueue(pThreadData, pDestinationName, ieqnDiscardMessages, false);
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Create Message
///
/// Creates a new message in the Engine.
///
/// @param[in]     pHeader          Message header
/// @param[in]     areaCount        Number of message areas - may be 0
/// @param[in]     areaTypes[]      Types of messages areas - no duplicates
/// @param[in]     areaLengths[]    Array of area lengths
/// @param[in]     pAreaData[]      Pointers to area data
/// @param[out]    phMessage        Returned message handle
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The message is copied into memory managed by the Engine.
/// The message handle should be used to put the message on a destination,
/// or released. The same message handle can be used to put to several destinations.
///
/// It is permissible for elements in areaLengths[] to be zero and
/// elements in pAreaData[] to be NULL with the same array index.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createMessage(
    ismMessageHeader_t              *pHeader,
    uint8_t                         areaCount,
    ismMessageAreaType_t            areaTypes[areaCount],
    size_t                          areaLengths[areaCount],
    void *                          pAreaData[areaCount],
    ismEngine_MessageHandle_t *     phMessage)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ismEngine_Message_t *pMessage = NULL;
    uint32_t MsgLength = 0;
    uint32_t i;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, areaCount, ENGINE_HIGH_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(areaCount <= ismENGINE_MSG_AREAS_MAX);

    // If this is a persistent message then before proceeding with the 
    // create message request we must verify that the management generation
    // status is good before proceeding.
    if (pHeader->Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
    {
        ismEngineComponentStatus_t storeStatus = ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_1];
        if (storeStatus != StatusOk)
        {
            rc = ISMRC_ServerCapacity;

            ieutTRACEL(pThreadData, storeStatus, ENGINE_WORRYING_TRACE,
                       "Rejecting createMessage for persistent message as store status[%d] is %d\n",
                       ismENGINE_STATUS_STORE_MEMORY_1, ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_1]);

            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    for (i = 0; i < areaCount; i++)
    {
        MsgLength += areaLengths[i];
    }

    pMessage = iere_malloc(pThreadData,
                           iereNO_RESOURCE_SET,
                           IEMEM_PROBE(iemem_messageBody, 1), MsgLength + sizeof(ismEngine_Message_t));
    if (pMessage != NULL)
    {
        char *bufPtr = (char *)(pMessage+1);

        ismEngine_SetStructId(pMessage->StrucId, ismENGINE_MESSAGE_STRUCID);
        pMessage->usageCount = 1;
        memcpy(&(pMessage->Header), pHeader, sizeof(ismMessageHeader_t));
        pMessage->AreaCount = areaCount;
        pMessage->Flags = ismENGINE_MSGFLAGS_NONE;
        pMessage->MsgLength = MsgLength;
        pMessage->resourceSet = iereNO_RESOURCE_SET;
        pMessage->fullMemSize = (int64_t)iere_full_size(iemem_messageBody, pMessage);
        for (i = 0; i < areaCount; i++)
        {
            pMessage->AreaTypes[i]   = areaTypes[i];
            pMessage->AreaLengths[i] = areaLengths[i];

            if (areaLengths[i] == 0)
            {
                pMessage->pAreaData[i] = NULL;
            }
            else
            {
                pMessage->pAreaData[i] = bufPtr;
                memcpy(bufPtr, pAreaData[i], areaLengths[i]);
                bufPtr += areaLengths[i];
            }
        }
        pMessage->StoreMsg.parts.hStoreMsg = ismSTORE_NULL_HANDLE;
        pMessage->StoreMsg.parts.RefCount = 0;

        *phMessage = pMessage;
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

mod_exit:
    ieutTRACEL(pThreadData, pMessage,  ENGINE_HIGH_TRACE, FUNCTION_EXIT "rc=%d, hMessage=%p\n", __func__, rc, pMessage);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//Used by both ism_engine_confirmMessageDelivery and ism_engine_confirmMessageDeliveryBatch to do a final reduce of the usecount
int32_t ism_engine_confirmMessageDeliveryCompleted(
         ieutThreadData_t *pThreadData
       , int32_t  rc
       , ismEngine_AsyncData_t *asyncInfo
       , ismEngine_AsyncDataEntry_t *asyncEntry)
{
    ismEngine_AsyncAckCompleted_t *pAckData = (ismEngine_AsyncAckCompleted_t *)(asyncEntry->Data);
    ismEngine_CheckStructId(pAckData->StrucId, ismENGINE_ASYNCACKCOMPLETED_STRUCID, 1);

    ieutTRACEL(pThreadData, pAckData->pSession, ENGINE_CEI_TRACE, FUNCTION_IDENT "Session %p\n", __func__,
                 pAckData->pSession);

    if (pAckData->ackOptions == ismENGINE_CONFIRM_OPTION_SESSION_CLEANUP)
    {
        releaseSessionReference(pThreadData,  pAckData->pSession, false);
    }
    else
    {
        reducePreNackAllCount(pThreadData, pAckData-> pSession);
    }

    //Don't need this callback to be run again if we go async
    iead_popAsyncCallback( asyncInfo, asyncEntry->DataLen);

    return OK;
}

//****************************************************************************
/// @internal
///
/// @brief  Confirm Message Delivery
///
/// Confirms delivery of one or more messages. This is used to move
/// a message through its delivery states. The Engine will check that
/// messages only go through a valid set of state transitions.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hTran            Transaction handle - may be NULL
/// @param[in]     hDelivery        Delivery handle
/// @param[in]     options          One of ismENGINE_CONFIRM_OPTION_* values
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_confirmMessageDelivery(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_DeliveryHandle_t      hDelivery,
    uint32_t                        options,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)hTran;
    int32_t rc = OK;
    ismEngine_AsyncAckCompleted_t asyncInfo =   { ismENGINE_ASYNCACKCOMPLETED_STRUCID, pSession, options };

    ismEngine_DeliveryInternal_t hTempHandle = { hDelivery };

    ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
            {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineAcknowledge, &asyncInfo, sizeof(asyncInfo), NULL, {.internalFn = ism_engine_confirmMessageDeliveryCompleted } },
            {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineCaller,      pContext,   contextLength,     NULL, {.externalFn = pCallbackFn }}};
    ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID, pSession->pClient, IEAD_MAXARRAYENTRIES, 2, 0, true,  0, 0, asyncArray};

    ismQHandle_t QHandle = hTempHandle.Parts.Q;
    void *hDeliveryHandle = hTempHandle.Parts.Node;

    ieutTRACEL(pThreadData, hDeliveryHandle, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, hQueue %p, hTran %p, hDelivery %p, options %u)\n", __func__,
               hSession, QHandle, hTran, hDeliveryHandle, options);

    //Don't try and nack all the messages (or destroy session) whilst we're process this (n/)ack
    __sync_fetch_and_add(&pSession->PreNackAllCount, 1);

    rc = ieq_acknowledge(pThreadData, QHandle, hSession, pTran, hDeliveryHandle, options, &asyncData);


    if (rc != ISMRC_AsyncCompletion)
    {
        //We don't go async if we are just cleaning up the session
        assert(options != ismENGINE_CONFIRM_OPTION_SESSION_CLEANUP);
        reducePreNackAllCount(pThreadData, pSession);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

static ismEngine_DeliveryHandle_t *getDeliveryHandlesFromAsyncInfo(ismEngine_AsyncData_t *asyncInfo)
{
    ismEngine_DeliveryHandle_t *pDeliveryHdls = NULL;

    uint32_t entry2 = asyncInfo->numEntriesUsed-1;
    uint32_t entry1 = entry2 - 1;

    if (   (asyncInfo->entries[entry1].Type != EngineProcessBatchAcks1)
         ||(asyncInfo->entries[entry2].Type != EngineProcessBatchAcks2))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true,
            "asyncInfo stack not as expected (corrupted?)", ISMRC_Error,
            NULL);
    }

    pDeliveryHdls = (ismEngine_DeliveryHandle_t *)(asyncInfo->entries[entry1].Data);

    return pDeliveryHdls;
}

static int ismEngine_internal_RestartSessionTimer(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)userdata;

    // Make sure we're thread-initialised. This can be issued repeatedly.
    ism_engine_threadInit(0);

    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);

    ieutTRACEL(pThreadData, pSession, ENGINE_CEI_TRACE, FUNCTION_IDENT "pSession=%p\n"
                       , __func__, pSession);

    ismEngine_internal_RestartSession( pThreadData, pSession, false);

    releaseSessionReference(pThreadData, pSession, false);

    ieut_leavingEngine(pThreadData);
    ism_common_cancelTimer(key);

    //We did engine threadInit...  term will happen when the engine terms and after the counter
    //of events reaches 0
    __sync_fetch_and_sub(&(ismEngine_serverGlobal.TimerEventsRequested), 1);
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Restarts delivery to a session that the engine paused because it ran out
///    of delivery ids
///
///  @param[in] Qhdl               - Queue
///  @param[in] pSession           - Session to be restarted
///  @param[in] restartOnTimer     - Do the restart on timer thread
///                                  (Used if we have sessions/consumers locked that block stopping)
///  @return                       - OK
///////////////////////////////////////////////////////////////////////////////

void ismEngine_internal_RestartSession( ieutThreadData_t * pThreadData
                                      , ismEngine_Session_t * pSession
                                      , bool restartOnTimer)
{
    ieutTRACEL(pThreadData, pSession, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n",
               __func__);
    //We have freed up a delivery id and should restart messaging

    if (restartOnTimer)
    {
        //Keep the session alive until the timer
        __sync_fetch_and_add(&pSession->UseCount, 1);

        __sync_fetch_and_add(&(ismEngine_serverGlobal.TimerEventsRequested), 1);

       (void)ism_common_setTimerOnce(ISM_TIMER_LOW,
                                     ismEngine_internal_RestartSessionTimer,
                                     pSession, 20);
    }
    else
    {
        bool keepGoing;
        int32_t rc = OK;
        iecsMessageDeliveryInfoHandle_t hMsgDelInfo = pSession->pClient->hMsgDeliveryInfo;

        do
        {
            keepGoing = false;

            rc = ism_engine_startMessageDelivery(pSession,
                                                 ismENGINE_START_DELIVERY_OPTION_ENGINE_START, NULL, 0, NULL);
            if (rc == ISMRC_RequestInProgress)
            {
                //Stopping hasn't completed yet...
                //Need to readd the deliveryids exhausted flag so another ack tries again.
                uint32_t numAcks = iecs_markDeliveryIdsExhausted(pThreadData, hMsgDelInfo, pSession);

                if (numAcks == 0)
                {
                    //We can't rely on another ack to restart delivery... see if the session is still stopping,
                    //If so it'll check when it stops and see it needs to restart
                    ism_engine_lockSession(pSession);
                    bool stillStopping = pSession->fIsDeliveryStopping;
                    ism_engine_unlockSession(pSession);

                    if (!stillStopping)
                    {
                        //Session is now stopped, we should be able to start is again
                        keepGoing = true;
                    }
                    else
                    {
                        //Session isn't stopped, session will notice it needs to restart when it stops
                        rc = OK;
                    }
                }
                else
                {
                    rc = OK;
                }
            }
        }
        while (keepGoing);

        if (rc != OK)
        {
            if (   (rc == ISMRC_NotEngineControlled)
                || (rc == ISMRC_AsyncCompletion)
                || (rc == ISMRC_Destroyed))
            {
                //That's ok... we wanted to restart after engine-flow control but either:
                // a) the protocol has taken over by an explicit stop or start request or
                // b) It went ok but needs to complete asynchronously
                // c) session is destroyed...don't need to worry about flow control
                rc = OK;
            }

            else
            {
                ieutTRACE_FFDC(ieutPROBE_002, true
                              , "Couldn't restart message delivery after running out of delivery ids",rc
                              , "pSession", pSession, sizeof(ismEngine_Session_t)
                              , NULL);
            }
        }
    }

    ieutTRACEL(pThreadData, pSession, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return;
}

int32_t ism_engine_processBatchAcks(
         ieutThreadData_t *pThreadData
       , int32_t  rc
       , ismEngine_AsyncData_t *asyncInfo
       , ismEngine_AsyncDataEntry_t *asyncEntry)
{
    ismEngine_AsyncProcessAcks_t *pAckData = (ismEngine_AsyncProcessAcks_t *)(asyncEntry->Data);
    ismEngine_CheckStructId(pAckData->StrucId, ismENGINE_ASYNCPROCESSACKS_STRUCID, 1);
    ismEngine_DeliveryInternal_t hHandle;
    ismQHandle_t QHandle = NULL;
    void *hDeliveryHandle = NULL;
    ismEngine_DeliveryHandle_t *pDeliveryHdls = getDeliveryHandlesFromAsyncInfo(asyncInfo);
    assert(rc == OK);
    uint32_t maxUnstoreMessages = pAckData->numAcks - pAckData->nextAck;
    ismStore_Handle_t hMsgsToUnstore[maxUnstoreMessages+1]; //+1 so we have at least one entry in the array
    uint32_t numMsgsToUnstore = 0;

    //Initialise first slot so we can tell when it has been filled
    hMsgsToUnstore[0] = ismSTORE_NULL_HANDLE;

    for (uint32_t index = pAckData->nextAck;
         (rc == OK) && (index < pAckData->numAcks);
         index++)
    {
        hHandle.Full = pDeliveryHdls[index];
        QHandle = hHandle.Parts.Q;
        hDeliveryHandle = hHandle.Parts.Node;

        pAckData->nextAck +=1; //Once we call processAck, we don't want to call process again for the same ack

        rc = ieq_processAck(pThreadData,
                            QHandle,
                            pAckData->pSession,
                            pAckData->pTran,
                            hDeliveryHandle,
                            pAckData->ackOptions,
                            &(hMsgsToUnstore[numMsgsToUnstore]),
                            &(pAckData->triggerSessionRedelivery),
                            &(pAckData->batchState),
                            NULL);

        if (rc == OK)
        {
            pDeliveryHdls[index] = ismENGINE_NULL_DELIVERY_HANDLE;
        }
        else if (rc != ISMRC_AsyncCompletion)
        {
            ieutTRACE_FFDC( ieutPROBE_001, true,
                "Unexpect rc from process acks", rc,
                NULL);
        }

        if (hMsgsToUnstore[numMsgsToUnstore] != ismSTORE_NULL_HANDLE)
        {
            numMsgsToUnstore++;
            hMsgsToUnstore[numMsgsToUnstore] = ismSTORE_NULL_HANDLE;
        }
    }

    if (rc == OK)
    {
        if (pAckData->batchState.ackCount != 0)
        {
            ieq_completeAckBatch( pThreadData
                                , pAckData->batchState.Qhdl
                                , pAckData->pSession
                                , &(pAckData->batchState));
        }
        if (pAckData->triggerSessionRedelivery)
        {
            ismEngine_internal_RestartSession(pThreadData, pAckData->pSession, false );
        }

        if (pAckData->pTran != NULL)
        {
            //This ack batch should no longer block commit/rollback
            ietr_decreasePreResolveCount(pThreadData, pAckData->pTran, true);
        }

        //Don't need this callback to be run again if we go async... there are two entries
        //(we checked they were the correct ones when we looked up pDeliverHdls)

        iead_popAsyncCallback( asyncInfo, asyncEntry->DataLen);
        iead_popAsyncCallback( asyncInfo,
                               ((pAckData->numAcks) * sizeof(ismEngine_DeliveryHandle_t)));
    }

    if (numMsgsToUnstore > 0)
    {
        //We don't believe we have any uncommitted store work... so we can go and remove
        //the messages we need to
#ifndef NDEBUG
        uint32_t storeOpsCount = 0;
        int32_t dbg_rc = ism_store_getStreamOpsCount(pThreadData->hStream, &storeOpsCount);
        assert(dbg_rc == OK && storeOpsCount == 0);
#endif
        ismEngine_AsyncDataEntry_t unstoreMsgsAsyncArray[IEAD_MAXARRAYENTRIES];
        ismEngine_AsyncData_t unstoreMsgsAsyncInfo = {ismENGINE_ASYNCDATA_STRUCID, NULL,
                                                      IEAD_MAXARRAYENTRIES, 0, 0, true,  0, 0,
                                                      unstoreMsgsAsyncArray};

        DEBUG_ONLY int32_t rc2 =  iest_finishUnstoreMessages( pThreadData
                                                            , &unstoreMsgsAsyncInfo
                                                            , numMsgsToUnstore
                                                            , hMsgsToUnstore);

        //We don't mind if the removal of these messages goes async or not
        assert(rc2 == OK || rc2 == ISMRC_AsyncCompletion);
    }
    return rc;
}


//****************************************************************************
/// @brief  Confirm Message Delivery Batch
///
/// Causes the engine to repeatedly call the provided callback function
/// to provide a set of delivery handles to be confirmed / negatively
/// confirmed.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hTran            Transaction handle - may be NULL
/// @param[in]     hdlCount         Number of handles in pDeliveryHdls array
/// @param[in]     pDeliveryHdls    Array of deliver handles to be confirmed
/// @param[in]     options          One of ismENGINE_CONFIRM_OPTION_* values
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.  As each delivery 
/// handle is successfully confirmed the entry in the array is set to the 
/// value ismENGINE_NULL_DELIVERY_HANDLE. If the call fails the ISMRC_ value 
/// returned applies to the first non NULL entry in the pDeliveryHdls array.
///
/// @remark The operation will complete synchronously.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_confirmMessageDeliveryBatch(
    ismEngine_SessionHandle_t           hSession,
    ismEngine_TransactionHandle_t       hTran,
    uint32_t                            hdlCount,
    ismEngine_DeliveryHandle_t *        pDeliveryHdls,
    uint32_t                            options,
    void *                              pContext,
    size_t                              contextLength,
    ismEngine_CompletionCallback_t      pCallbackFn)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);
    int32_t rc = OK;
    uint32_t index = 0;
    ismQHandle_t QHandle = NULL;
    void *hDeliveryHandle = NULL;
    ismEngine_DeliveryInternal_t hHandle;
    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)hTran;

    ieutTRACEL(pThreadData, hSession, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p hTran %p hdlCount %d pDeliveryHdls %p options %u)\n",
               __func__, hSession, hTran, hdlCount, pDeliveryHdls, options);

    //If this is a normal ackBatch, delay the final nackall. If this is part of the
    //final nackall - don't try to delay the current process
    __sync_fetch_and_add((options == ismENGINE_CONFIRM_OPTION_SESSION_CLEANUP) ?
                                &(pSession->UseCount)  : &(pSession->PreNackAllCount), 1);

    if (((options == ismENGINE_CONFIRM_OPTION_NOT_DELIVERED) ||
         (options == ismENGINE_CONFIRM_OPTION_NOT_RECEIVED )) &&
        (pTran != NULL))
    {
        rc = ISMRC_ArgNotValid;

        ieutTRACEL(pThreadData, options,  ENGINE_ERROR_TRACE, "%s: Cannot nack(%08x) transactionally\n", __func__, options);
        goto mod_exit;
    }

    //If there is a transaction, don't try and commit/roll it back etc until we have processed this nack
    if (pTran != NULL)
    {
        ietr_increasePreResolveCount(pThreadData, pTran);
    }

    uint32_t storeOps = 0;

    for (index = 0; (rc == OK) && (index < hdlCount); index++)
    {
        hHandle.Full = pDeliveryHdls[index];
        QHandle = hHandle.Parts.Q;
        hDeliveryHandle = hHandle.Parts.Node;

        ieq_prepareAck(pThreadData,
                       QHandle,
                       hSession,
                       pTran,
                       hDeliveryHandle,
                       options,
                       &storeOps);
    }
    ismEngine_AsyncAckCompleted_t asyncAckBatchCompleted =   { ismENGINE_ASYNCACKCOMPLETED_STRUCID, pSession, options };
    ismEngine_AsyncProcessAcks_t   asyncProcessAcks = { ismENGINE_ASYNCPROCESSACKS_STRUCID
                                                      , pSession
                                                      , pTran
                                                      , {0}
                                                      , options
                                                      , 0
                                                      , hdlCount
                                                      , false};

    ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
                {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineAcknowledge, &asyncAckBatchCompleted, sizeof(asyncAckBatchCompleted), NULL,
                                                                                                           {.internalFn = ism_engine_confirmMessageDeliveryCompleted } },
                {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineCaller,      pContext,   contextLength,     NULL, {.externalFn = pCallbackFn }},

                //We want to copy two different memory areas if we go async, so currently we cheat and put the callback on the stack twice
                //...the callback knows to remove both
                {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineProcessBatchAcks1,   pDeliveryHdls,      hdlCount*(sizeof(*pDeliveryHdls)),     NULL,
                                                                                                           {.internalFn = ism_engine_processBatchAcks }},
                {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineProcessBatchAcks2,   &asyncProcessAcks,  sizeof(asyncProcessAcks), NULL,
                                                                                                           {.internalFn = ism_engine_processBatchAcks }}};

    ismEngine_AsyncData_t asyncInfo = {ismENGINE_ASYNCDATA_STRUCID, pSession->pClient, IEAD_MAXARRAYENTRIES, 4, 0, true,  0, 0, asyncArray};


    if (storeOps > 0)
    {
        rc = iead_store_asyncCommit(pThreadData, false, &asyncInfo);

        assert (rc == OK || rc == ISMRC_AsyncCompletion);

        if (rc != OK)
        {
            goto mod_exit;
        }
    }

    rc = ism_engine_processBatchAcks( pThreadData
                                    , rc
                                    , &asyncInfo
                                    , &(asyncArray[asyncInfo.numEntriesUsed -1]));

mod_exit:
    if (rc != ISMRC_AsyncCompletion)
    {
        //Reduce whichever use count we increased at the start of the function
        if (options == ismENGINE_CONFIRM_OPTION_SESSION_CLEANUP)
        {
            releaseSessionReference(pThreadData, pSession, false);
        }
        else
        {
            reducePreNackAllCount(pThreadData, pSession);
        }
    }

    ieutTRACEL(pThreadData, rc, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d Acks(%d of %d)\n", __func__,
               rc, index, hdlCount);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Send a heartbeat to the engine.
///
/// @remark The engine will perform any work that is outstanding for the
/// calling thread -- only threads that have called ism_engine_threadInit() and
/// not yet called ism_engine_threadTerm() can call this function.
//****************************************************************************
XAPI void ism_engine_heartbeat(void)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    // Yes, that's all we do -- ieut_enteringEngine will result in processing any
    // jobs left in the job queue for this thread.

    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @internal
///
/// @brief  Consumer selection Statistics
///
/// A function for querying the selection statistics from a consumer
///
/// @param[in]     QueueName                 The consumer
/// @param[in]     pfailedSelectionCount     Number of failed selections
/// @param[out]    psuccesfullSelectionCount Number of succesfull selections
///
//****************************************************************************
XAPI void ism_engine_getSelectionStats( ismEngine_Consumer_t *pConsumer
                                      , uint64_t *pfailedSelectionCount
                                      , uint64_t *psuccesfullSelectionCount )
{
    *pfailedSelectionCount=pConsumer->failedSelectionCount++;
    *psuccesfullSelectionCount=pConsumer->successfulSelectionCount++;
    return;
}

//Mark a session as undergoing engine flow control.
void ies_MarkSessionEngineControl(ismEngine_Session_t *pSession)
{
    ism_engine_lockSession(pSession);

    //We can only go into engine flow control for a session that is currently running
    if(     pSession->fIsDeliveryStarted
        && (!(pSession->fIsDeliveryStopping)))
    {
        pSession->fEngineControlledSuspend = true;
    }
    ism_engine_unlockSession(pSession);
}

#ifdef HAINVESTIGATIONSTATS
    void ism_engine_recordConsumerStats( ieutThreadData_t *pThreadData
                                       , ismEngine_Consumer_t *pConsumer)
    {
        pThreadData->stats.consGapBetweenGets[pThreadData->stats.nextConsSlot]
                      = pConsumer->lastGapBetweenGets;
        pThreadData->stats.consBatchSize[pThreadData->stats.nextConsSlot]
                      = pConsumer->lastBatchSize;

        pThreadData->stats.consTotalConsumerLockedTime[pThreadData->stats.nextConsSlot]
                  = timespec_diff_int64(&(pConsumer->lastGetTime),
                                        &(pConsumer->lastDeliverEnd));

        if (      (pConsumer->lastDeliverCommitStart.tv_sec > 0)
            &&    (pConsumer->lastDeliverCommitEnd.tv_sec > 0))
        {
            pThreadData->stats.consBeforeStoreCommitTime[pThreadData->stats.nextConsSlot]
                      = timespec_diff_int64(&(pConsumer->lastGetTime),
                                          &(pConsumer->lastDeliverCommitStart));
            pThreadData->stats.consStoreCommitTime[pThreadData->stats.nextConsSlot]
                      = timespec_diff_int64(&(pConsumer->lastDeliverCommitStart),
                                          &(pConsumer->lastDeliverCommitEnd));
            pThreadData->stats.consAfterStoreCommitTime[pThreadData->stats.nextConsSlot]
                     = timespec_diff_int64(&(pConsumer->lastDeliverCommitEnd),
                                          &(pConsumer->lastDeliverEnd));
        }
        else if (    (pConsumer->lastDeliverCommitStart.tv_sec == 0)
                 &&  (pConsumer->lastDeliverCommitEnd.tv_sec == 0))
        {
            //No store commit for this get
            pThreadData->stats.consBeforeStoreCommitTime[pThreadData->stats.nextConsSlot]
                 = pThreadData->stats.consTotalConsumerLockedTime[pThreadData->stats.nextConsSlot];
            pThreadData->stats.consStoreCommitTime[pThreadData->stats.nextConsSlot] = 0;
            pThreadData->stats.consAfterStoreCommitTime[pThreadData->stats.nextConsSlot] = 0;
        }
        else
        {
            //We seem to have a commit start and no end or vice versa
            TRACE(3, "TEMPSTATS4 Error. Consumer start commit: %lu s %lu ns, End: %lu s %lu ns\n",
                    pConsumer->lastDeliverCommitStart.tv_sec,
                    pConsumer->lastDeliverCommitStart.tv_nsec,
                    pConsumer->lastDeliverCommitEnd.tv_sec,
                    pConsumer->lastDeliverCommitEnd.tv_nsec);

            //Disregard it for the moment
            pThreadData->stats.consBeforeStoreCommitTime[pThreadData->stats.nextConsSlot]
                            = pThreadData->stats.consTotalConsumerLockedTime[pThreadData->stats.nextConsSlot];
            pThreadData->stats.consStoreCommitTime[pThreadData->stats.nextConsSlot] = 0;
            pThreadData->stats.consAfterStoreCommitTime[pThreadData->stats.nextConsSlot] = 0;
        }

        pThreadData->stats.nextConsSlot = (1 + pThreadData->stats.nextConsSlot)  % CONS_SLOTS;
    }
#endif

/*********************************************************************/
/*                                                                   */
/* End of engine.c                                                   */
/*                                                                   */
/*********************************************************************/
