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
/// @file  engineGlobal.c
/// @brief Global data for Engine Component of ISM
//*********************************************************************
#define TRACE_COMP Engine
#include "engineInternal.h"

ismEngine_Server_t ismEngine_serverGlobal =
{
    ismENGINE_SERVER_STRUCID_ARRAY,                          // Eyecatcher
    ismSTORE_NULL_HANDLE,                                    // Store handle of Server Configuration Record
    PTHREAD_MUTEX_INITIALIZER,                               // Mutex synchronising create/delete of client-states
    NULL,                                                    // Client-state table
    NULL,                                                    // Server's topic tree
    NULL,                                                    // Server's remote server information root
    NULL,                                                    // Lock manager
    NULL,                                                    // Transaction Control
    NULL,                                                    // Server's named queue namespace
    NULL,                                                    // Policy info global
    NULL,                                                    // Server's import/export global structure
    NULL,                                                    // Head of MQ Queue-manager record list
    NULL,                                                    // Tail of MQ Queue-manager record list
    PTHREAD_MUTEX_INITIALIZER,                               // Mutex synchronising MQ store state
    ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,           // Whether automatic named queue creation is disabled
    ismENGINE_DEFAULT_USE_SPIN_LOCKS,                        // Whether the queues should use spin locks
    ismENGINE_DEFAULT_CLUSTER_ENABLED,                       // Whether clustering is enabled
    ismENGINE_DEFAULT_RETAINED_REPOSITIONING_ENABLED,        // Whether retained message repositioning is enabled or not
    ismENGINE_DEFAULT_ENVIRONMENT_IS_IOT,                    // Whether this server is running in an IoT environment (enabling some IoT specific behavior)
    ismENGINE_DEFAULT_REDUCED_MEMORY_RECOVERY_MODE,          // Whether to use less memory during recovery (which will generally make things slower)
    ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,         // Sublist cache initial capacity
    0,                                                       // Timestamp updated periodically in Store as server is running
    0,                                                       // Timestamp of last shutdown (recovered from store record at startup)
    ismENGINE_DEFAULT_SERVER_TIMESTAMP_INTERVAL,             // Interval in seconds between server timestamp updates
    ismENGINE_DEFAULT_RETAINED_MIN_ACTIVE_ORDERID_INTERVAL,  // Interval in seconds between retained minimum active orderid scans
    ismENGINE_DEFAULT_RETAINED_MIN_ACTIVE_ORDERID_MAX_SCANS, // Maximum number of rescans to allow when working out min active orderId
    ismENGINE_DEFAULT_CLUSTER_RETAINED_SYNC_INTERVAL,        // Interval in seconds between cluster retained message synchronization checks
    0,                                                       // Use count for Engine initialisation in timer threads
    ismENGINE_DEFAULT_ASYNC_CALLBACK_QUEUE_ALERT_MAX_PAUSE,  // Maximum amount of time in seconds to pause threads when async CBQ alert is raised
    ismENGINE_DEFAULT_TRANSACTION_THREADJOBS_CLIENT_MINIMUM, // Minimum number of active clients before transactions start using thread job queues
    ismENGINE_DEFAULT_THREAD_JOB_QUEUES_ALGORITHM,           // How much work to put on thread job queues and how aggressive should the scavenger be
    ismENGINE_DEFAULT_DESTROY_NONACKER_RATIO,                // Multiple of maxdepth an unacked msgs must be behind tail of queue to treat client as non-acker
    NULL,                                                    // Timer used to update the server timestamp
    NULL,                                                    // Timer used to update the minimum active orderid for retained msgs
    NULL,                                                    // Timer used to check cluster retained message synchronization
    0,                                                       // Count of one-off Timer events requested but not completed
    PTHREAD_MUTEX_INITIALIZER,                               // Mutex synchronising create/delete of thread data structures
    NULL,                                                    // Head of chain of thread data structures
    0,                                                       // ThreadId counter
    {0},                                                     // Ended thread statistics
    NULL,                                                    // Config callback handle for the engine
    ismENGINE_DEFAULT_MAX_MQTT_UNACKED_MESSAGES,             // Number of unacked messages allowed per mqtt client
    ismENGINE_DEFAULT_MULTICONSUMER_MESSAGE_BATCH,           // Number of messages given to a consumer before cycling to next consumer in round-robin
    ismENGINE_DEFAULT_CLUSTER_RETAINED_FORWARDING_DELAY,     // Number of synchronization loops to delay before requesting forwarding of out-of-sync retaineds
    0,                                                       // Number of subscription or topic policies with default selection policy set    NULL,                                                    // Message selection callback
    NULL,                                                    // Message selection callback
    NULL,                                                    // Retained message forwarding callback function
    NULL,                                                    // Delivery failure callback
    EnginePhaseUninitialized,                                // Run phase of engine
    {StatusOk},                                              // Engine Component statuses
    0,                                                       // Total subscribers count
    0,                                                       // Total disconnected client notification subscriptions
    0,                                                       // Total clientStates count
    0,                                                       // Total active clientStates count
    0,                                                       // Total Number of nonFatal FFDCs hit
    NULL,                                                    // Message Expiry control data
    NULL,                                                    // ClientState Expiry control data
    NULL,                                                    // Per-thread job queue control data
    1,                                                       // Mem update count
    NULL,                                                    // Engine-wide deferred free areas
    300,                                                     // Free Memory (in MiB) not included (on low memory machines) in memory manager free mem calculation
    100000,                                                  // If System Mem (in MiB) is greater than this we don't reserve any extra free memory (prev parameter)
    NULL,                                                    // Control structure for ResourceSet support
#if (ieutTRACEHISTORY_SAVEDTHREADS > 0)
#if (ieutTRACEHISTORY_BUFFERSIZE > 0)
    { { { 0 } } },
    0,
#endif
#endif
};

__thread ieutThreadData_t *ismEngine_threadData = NULL;

int32_t (*ismEngine_security_validate_policy_func)(ismSecurity_t *,
                                                   ismSecurityAuthObjectType_t,
                                                   const char *,
                                                   ismSecurityAuthActionType_t,
                                                   ism_ConfigComponentType_t,
                                                   void **) = NULL;
int32_t (*ismEngine_security_set_policyContext_func)(const char *,
                                                     ismSecurityPolicyType_t,
                                                     ism_ConfigComponentType_t,
                                                     void *) = NULL;

/*********************************************************************/
/*                                                                   */
/* End of engineGlobal.c                                             */
/*                                                                   */
/*********************************************************************/
