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

//****************************************************************************
/// @file  engineInternal.h
/// @brief Engine component internal header file.
/// Should be included by all Engine component source files.
//****************************************************************************
#ifndef __ISM_ENGINEINTERNAL_DEFINED
#define __ISM_ENGINEINTERNAL_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/**********************************************************************/
#include <engine.h>           /* Engine external header file         */
#include <engineCommon.h>     /* Engine common internal header file  */
#include <store.h>            /* Store external header file          */
#include <pthread.h>          /* pthread header file                 */
#include <stdlib.h>           /* C standard library header file      */
#include <string.h>           /* String and memory header file       */
#include <stdint.h>           /* Standard integer defns header file  */
#include <stdbool.h>          /* Standard boolean defns header file  */
#include <assert.h>           /* Used for assert()                   */
#include <config.h>           /* Config component header file        */
#include <selector.h>         /* Selection header file               */
#include <ismregex.h>         /* Regular expression support          */
#include <ismutil.h>          /* Utils                               */

#include "messageInternal.h"

/*********************************************************************/
/*                                                                   */
/* TYPE DEFINITIONS                                                  */
/*                                                                   */
/*********************************************************************/

// Types defined internally to the Engine in other files...
typedef struct iecsHashTable_t                   *iecsHashTableHandle_t;               // defined in clientStateInternal.h
typedef struct iecsHashEntry_t                   *iecsHashEntryHandle_t;               // defined in clientStateInternal.h
typedef struct iecsMessageDeliveryInfo_t         *iecsMessageDeliveryInfoHandle_t;     // defined in clientStateInternal.h
typedef struct iecsInflightDestination_t         *iecsInflightDestinationHandle_t;     // defined in clientStateInternal.h
typedef struct tag_ieutHashTable_t               *ieutHashTableHandle_t;               // defined in topicTreeInternal.h
typedef struct tag_iemeExpiryControl_t           *iemeExpiryControlHandle_t;           // defined in messageExpiry.h
typedef struct tag_ieceExpiryControl_t           *ieceExpiryControlHandle_t;           // defined in clientStateExpiry.h
typedef struct tag_iettTopicTree_t               *iettTopicTreeHandle_t;               // defined in topicTreeInternal.h
typedef struct tag_iersRemoteServers_t           *iersRemoteServersHandle_t;           // defined in remoteServersInternal.h
typedef struct tag_iettSubsNode_t                *iettSubsNodeHandle_t;                // defined in topicTreeInternal.h
typedef struct tag_iettTopicNode_t               *iettTopicNodeHandle_t;               // defined in topicTreeInternal.h
typedef struct tag_iettTopic_t                   *iettTopicHandle_t;                   // defined in topicTreeInternal.h
typedef struct tag_iettSubscriberList_t          *iettSubscriberListHandle_t;          // defined in topicTree.h
typedef struct tag_iesqQueue_t                   *iesqSimpleQHandle_t;                 // defined in simpQ.h
typedef struct tag_ieiqQueue_t                   *ieiqInterQHandle_t;                  // defined in intermediateQ.h
typedef struct ismEngine_Queue_t                 *ismQHandle_t;                        // defined in queueCommon.h
typedef struct ielmLockManager_t                 *ielmLockManagerHandle_t;             // defined in lockManagerInternal.h
typedef struct ielmLockScope_t                   *ielmLockScopeHandle_t;               // defined in lockManagerInternal.h
typedef struct ielmLockRequest_t                 *ielmLockRequestHandle_t;             // defined in lockManagerInternal.h
typedef struct tag_ieqnQueueNamespace_t          *ieqnQueueNamespaceHandle_t;          // defined in queueNamespace.h
typedef struct tag_ieqnQueue_t                   *ieqnQueueHandle_t;                   // defined in queueNamespace.h
typedef struct tag_iepiPolicyInfoGlobal_t        *iepiPolicyInfoGlobalHandle_t;        // defined in policyInfo.h
typedef struct tag_iepiPolicyInfo_t              *iepiPolicyInfoHandle_t;              // defined in policyInfo.h
typedef struct iesmQManagerRecord_t              *iesmQManagerRecordHandle_t;          // defined in engineMQBridgeStore.h
typedef struct ietrTransactionControl_t          *ietrTransactionControlHandle_t;      // defined in transaction.h
typedef struct ietrSLE_Header_t                  *ietrSLEHeaderHandle_t;               // defined in transaction.h
typedef struct ietrXIDIterator_t                 *ietrXIDIteratorHandle_t;             // defined in transaction.h
typedef struct tag_ietrAsyncTransactionData_t    *ietrAsyncTransactionDataHandle_t;    // defined in transaction.h
typedef struct iedmDump_t                        *iedmDumpHandle_t;                    // defined in engineDump.h
typedef struct tag_iememThreadMemUsage_t         *iememThreadMemUsageHandle_t;         // defined in memHandler.h
typedef struct tag_iempMemPoolPageHeader_t       *iempMemPoolHandle_t;                 // defined in mempool.h
typedef struct tag_ieieImportExportGlobal_t      *ieieImportExportGlobalHandle_t;      // defined in exportResources.h
typedef struct tag_ietjThreadJobControl_t        *ietjThreadJobControlHandle_t;        // defined in threadJobs.h
typedef struct tag_iejqJobQueue_t                *iejqJobQueueHandle_t;                // defined in jobqueue.c
typedef struct tag_iereResourceSetStatsControl_t *iereResourceSetStatsControlHandle_t; // defined in resourceSetStatsInternal.h
typedef struct tag_iereThreadCache_t             *iereThreadCacheHandle_t;             // defined in resourceSetStatsInternal.h
typedef struct tag_iereThreadCacheEntry_t        *iereThreadCacheEntryHandle_t;        // defined in resourceSetStats.h
typedef struct tag_iereResourceSet_t             *iereResourceSetHandle_t;             // defined in resourceSetStats.h
typedef struct tag_ieutDeferredFreeList_t        *ieutDeferredFreeListHandle_t;        // defined in engineDeferredFree.h

#define iereNO_RESOURCE_SET ((iereResourceSetHandle_t)NULL)

struct tag_iemqQNode_t; //forward declaration.. actually in multiConsumerQ.h

//*********************************************************************
/// @brief  ismEngineComponentStatus_t
///
/// ismENGINE_STATUS_STORE_MEMORY_0 - Status of the small buffers in the
/// management generation. Used for records (queues, transactions, ...)
/// ismENGINE_STATUS_STORE_MEMORY_1 - Status of the large buffers in the
/// management generation. Used for references and state records
/// ismENGINE_STATUS_STORE_DISK     - Store disk usage
typedef enum ismEngineComponentStatus_t
{
    StatusOk = 0,
    StatusWarning,
    StatusError
} ismEngineComponentStatus_t;

typedef enum ismEngineComponents_t
{
    ismENGINE_STATUS_STORE_MEMORY_0 = 0,
    ismENGINE_STATUS_STORE_MEMORY_1,
    ismENGINE_STATUS_STORE_DISK,
    ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE,
    ismENGINE_STATUS_COUNT
} ismEngineComponents_t;

typedef enum ismEngineRunPhase_t
{
    EnginePhaseUninitialized         = 0x0000, ///< Nothing is initialized, we also get here *AFTER* ism_engine_term
    EnginePhaseInitializing          = 0x0001, ///< Set at the beginning of ism_engine_init
    EnginePhaseStarting              = 0x0010, ///< Set at the beginning of ism_engine_start
    EnginePhaseRecovery              = 0x0020, ///< Set as the engine begins to perform recovery (ierr_restartRecovery)
    EnginePhaseCompletingRecovery    = 0x0040, ///< Store is open for business, we're tidying up
    EnginePhaseThreadsSelfInitialize = 0x0080, ///< Threads should perform full thread initialization themselves
    EnginePhaseRunning               = 0x0100, ///< Set at the end of ism_engine_startMessaging to indicate that we are running
    EnginePhaseEnding                = 0x1000, ///< Set at the beginning of ism_engine_term
} ismEngineRunPhase_t;

typedef enum __attribute__ ((__packed__)) ieutReservationState_t
{
    Inactive = 0,
    Pending,
    Active
} ieutReservationState_t;


//Define HAINVESTIGATIONSTATS For debugging stats
//#define HAINVESTIGATIONSTATS 1

//*********************************************************************
/// @brief  Per-thread statistics
//*********************************************************************
typedef struct tag_ieutThreadStats_t
{
    uint64_t droppedMsgCount;                  // Count of messages dropped by this thread
    uint64_t expiredMsgCount;                  // Count of messages expired by this thread
    int64_t bufferedMsgCount;                  // including in-flight
    int64_t bufferedExpiryMsgCount;            // excluding in-flight
    int64_t internalRetainedMsgCount;          // including in-flight and NullRetained
    int64_t externalRetainedMsgCount;          // excluding in-flight and NullRetained
    int64_t retainedExpiryMsgCount;            // excluding in-flight
    int64_t remoteServerBufferedMsgBytes;      // including in-flight
    uint64_t fromForwarderRetainedMsgCount;    // Count of retained messages published from forwarder
    uint64_t fromForwarderMsgCount;            // Count of non-retained messages published from forwarder
    uint64_t fromForwarderNoRecipientMsgCount; // Count of non-retained messages published from forwarder with no subscribers
    int64_t zombieSetToExpireCount;            // Count of zombie ClientStates that have a non-zero expiry time set
    uint64_t expiredClientStates;              // Count of ClientStates that have actually expired
    int64_t resourceSetMemBytes;               // Count of all bytes being counted against any resource set
#ifdef HAINVESTIGATIONSTATS
    uint64_t syncCommits;
    uint64_t syncRollbacks;
    uint64_t asyncCommits;
    uint64_t numDeliveryBatches;
    uint64_t minDeliveryBatchSize;
    uint64_t maxDeliveryBatchSize;
    double   avgDeliveryBatchSize;
    uint64_t messagesDeliveryCancelled;
    uint64_t perConsumerFlowControlCount;
    uint64_t perSessionFlowControlCount;
    uint64_t CheckWaitersScheduled;
    uint64_t CheckWaitersCalled;
    uint64_t CheckWaitersGotWaiter;
    uint64_t CheckWaitersInternalAsync;
    uint64_t asyncMsgBatchStartDeliver;
    uint64_t asyncMsgBatchStartLocate;
    uint64_t asyncMsgBatchDeliverWithJobThread;
    uint64_t asyncMsgBatchDeliverNoJobThread;
    uint64_t asyncMsgBatchDeliverScheduledCW;
    uint64_t asyncMsgBatchDeliverDirectCallCW;
    uint64_t asyncDestroyClientJobScheduled;
    uint64_t asyncDestroyClientJobRan;
#define CONS_SLOTS 32
    int64_t   consGapBetweenGets[CONS_SLOTS];
    uint64_t  consBatchSize[CONS_SLOTS];
    int64_t   consBeforeStoreCommitTime[CONS_SLOTS];
    int64_t   consStoreCommitTime[CONS_SLOTS];
    int64_t   consAfterStoreCommitTime[CONS_SLOTS];
    int64_t   consTotalConsumerLockedTime[CONS_SLOTS];
    uint32_t nextConsSlot;
#endif
} ieutThreadStats_t;

//*********************************************************************
/// @brief The type of config callback the current thread is processing.
/// expect this to be mostly 'NoConfigCallback'.
//*********************************************************************
typedef enum __attribute__ ((__packed__)) tag_ieutConfigCallbackType_t
{
    NoConfigCallback = 0,
    QueueConfigCallback = 1,
    TopicMonitorConfigCallback = 2,
    ClientStateConfigCallback = 3,
    SubscriptionConfigCallback = 4,
    PolicyConfigCallback = 5,
    ClusterRequestedTopicsConfigCallback = 6
} ieutConfigCallbackType_t;

//*********************************************************************
/// @brief The thread type if it has been determined, expect these mostly
/// to be 'UnknownThreadType' for most threads.
//*********************************************************************
typedef enum __attribute__ ((__packed__)) tag_ieutThreadType_t
{
    UnknownThreadType = 0,
    AsyncCallbackThreadType = 1,
} ieutThreadType_t;

//*********************************************************************
/// @brief Engine interpretation of the SizeProfile config setting.
//*********************************************************************
typedef enum __attribute__ ((__packed__)) tag_ieutSizeProfile_t
{
    DefaultSizeProfile = 0,
    SmallSizeProfile = 1,
} ieutSizeProfile_t;

// If we are looking for trapped messages (and we're going to deadlock
// and wait to be attached to with gdb when they occur) then we want
// large trace history buffers etc...
#ifdef ieutTRAPPEDMSG_DEADLOCK
#ifndef ieutEXTRADIAGNOSTICS
#define ieutEXTRADIAGNOSTICS 1
#endif
#endif

//*********************************************************************
/// @brief  Thread Data Block
///
/// Main control block for thread-local data
//*********************************************************************

// Define the size of the per-thread trace history buffer. This should be
// kept to a power of 2 to keep ieutTRACEL's modulo from slowing down.
// Setting the buffer size to 0 will turn off (compile out) the history buffer
#ifdef ieutEXTRADIAGNOSTICS
#define ieutTRACEHISTORY_BUFFERSIZE 2097152
#else
#define ieutTRACEHISTORY_BUFFERSIZE 16384
#endif

#define ieutMAXLAZYMSGS 10 //Most messages we can that we need to get around to removing at
                           //some point

/// @brief Per thread context structure
typedef struct ieutThreadData_t
{
    char                            StrucId[4];                           ///< Eye catcher 'IEUT'
    uint32_t                        cacheUpdates;                         ///< TopicTree CacheUpdates value when per-thread cache reset
    struct ieutThreadData_t        *next;                                 ///< Next thread context in engine global list
    struct ieutThreadData_t        *prev;                                 ///< Previous thread context in engine global list
    ismStore_StreamHandle_t         hStream;                              ///< Store stream handle being used by this thread
    ieutHashTableHandle_t           sublistCache;                         ///< Hash table containing cached subscriber lists
    iettSubscriberListHandle_t      sublist;                              ///< Per-thread sublist (contains last sublist used on this thread)
    size_t                          topicStringBufferSize;                ///< Length of the topic string buffer pointed to by the per-thread sublist
    ieutThreadStats_t               stats;                                ///< Statistics generated on this thread (only make sense when summed across all threads)
    bool                            closeStream;                          ///< Whether we should close the store stream during threadTerm
    uint8_t                         isStoreCritical;                      ///< Whether the store stream was initialised as critical (high performance)
    uint8_t                         componentTrcLevel;                    ///< Trace level in place on entry to the engine (used for speed while inside engine)
    ieutConfigCallbackType_t        inConfigCallback;                     ///< Whether this thread is currently in the middle of a callback from config component
    ieutReservationState_t          ReservationState;                     ///< Whether a store reservation is currently active on this thread
    uint32_t                        engineThreadId;                       ///< Simple numeric identifier from this thread (this is *not* a pthread TID or anything)
    uint64_t                        asyncCounter;                         ///< Increased by each async commit as an identifier
    uint32_t                        callDepth;                            ///< Count of nested engine calls, used to identify when we enter for the first and leave for the last time.
    uint32_t                        publishDepth;                         ///< Specific count of nested publish calls (publish-in-publish)
    uint32_t                        numLazyMsgs;                          ///< How many messages have been scheduled for lazy store message removal.
    ismStore_Handle_t               hMsgForLazyRemoval[ieutMAXLAZYMSGS];  ///< Array of lazy store message removal handles.
    uint64_t                        memUpdateCount;                       ///< For threads in the engine, global memUpdateCount when it entered (used for defferred memory freeing)
    iememThreadMemUsageHandle_t     memUsage;                             ///< Per-thread memory accounting
    iereThreadCacheEntryHandle_t    curThreadCacheEntry;                  ///< Which entry in the resourceSet cache is currently 'primed'
    iereThreadCacheHandle_t         resourceSetCache;                     ///< Per-thread cache of resourceSets in use by this thread
    uint32_t                        useCount;                             ///< Use count of the thread
    ieutThreadType_t                threadType;                           ///< Type of thread (for instance, is this one of the store's AsyncCallback threads)
    uint64_t                        entryCount;                           ///< Count of entries in this thread's job queue
    iejqJobQueueHandle_t            jobQueue;                             ///< Job queue of work this thread should perform next time it calls the engine
    uint64_t                        processedJobs;                        ///< How many times this thread processed jobs on entry
#ifndef NDEBUG
    int                             tid;                                  ///< Operating system TID
#endif
#if (ieutTRACEHISTORY_BUFFERSIZE > 0)
    uint64_t                        traceHistoryIdent[ieutTRACEHISTORY_BUFFERSIZE];
    uint64_t                        traceHistoryValue[ieutTRACEHISTORY_BUFFERSIZE];
    uint32_t                        traceHistoryBufPos;
#endif
} ieutThreadData_t;

#define ismENGINE_THREADDATA_STRUCID "IEUT"

// Define the members to be described in a dump (can exclude & reorder members)
// Note: Trace history is not included because it is conditional, but the data is
//       written to the dump and can be seen using the -R option in imadumpformatter.
#define iedm_describe_ieutThreadData_t(__file)\
    iedm_descriptionStart(__file, ieutThreadData_t, StrucId, ismENGINE_THREADDATA_STRUCID);\
    iedm_describeMember(char [4],                 StrucId);\
    iedm_describeMember(uint32_t,                 cacheUpdates);\
    iedm_describeMember(ieutThreadData_t *,       next);\
    iedm_describeMember(ieutThreadData_t *,       prev);\
    iedm_describeMember(ismStore_Stream_t *,      hStream);\
    iedm_describeMember(ieutHashTable_t *,        sublistCache);\
    iedm_describeMember(iettSubscriberList_t *,   sublist);\
    iedm_describeMember(size_t,                   topicStringBufferSize);\
    iedm_describeMember(ieutThreadStats_t,        stats);\
    iedm_describeMember(uint8_t,                  isStoreCritical);\
    iedm_describeMember(uint8_t,                  componentTrcLevel);\
    iedm_describeMember(ieutConfigCallbackType_t, inConfigCallback);\
    iedm_describeMember(ieutReservationState_t,   ReservationState);\
    iedm_describeMember(uint32_t,                 callDepth);\
    iedm_describeMember(ismStore_Handle_t,        hMsgForLazyRemoval);\
    iedm_describeMember(uint64_t,                 memUpdateCount);\
    iedm_describeMember(iemem_threadMemSizes_t *, memUsage);\
    iedm_describeMember(uint32_t,                 useCount);\
    iedm_descriptionEnd;

//****************************************************************************
/// @brief Callback function used when enumerating thread data structures
//****************************************************************************
typedef void (ieutThreadData_EnumCallback_t)(ieutThreadData_t *pThreadData,
                                              void *context);


//****************************************************************************
/// @brief After a thread exits we want to keep its trace data around in case 
/// it is needed to diagnose a problem
//****************************************************************************
typedef struct ismEngine_StoredThreadData_t
{
    uint64_t                        traceHistoryIdent[ieutTRACEHISTORY_BUFFERSIZE];
    uint64_t                        traceHistoryValue[ieutTRACEHISTORY_BUFFERSIZE];
    uint32_t                        traceHistoryBufPos;
} ismEngine_StoredThreadData_t;

//*********************************************************************
/// @brief  Server
///
/// Represents the server.
/// This is a singleton for the entire messaging server.
//*********************************************************************
#define ieutTRACEHISTORY_SAVEDTHREADS 2 //As threads end, we save the trace history from the last this-many threads

typedef struct ismEngine_Server_t
{
    char                                   StrucId[4];                              ///< Eyecatcher "ESVR"
    ismStore_Handle_t                      hStoreSCR;                               ///< Store handle of Server Configuration Record
    pthread_mutex_t                        Mutex;                                   ///< Mutex synchronising create/delete of client-states
    iecsHashTableHandle_t                  ClientTable;                             ///< Client-state table
    iettTopicTreeHandle_t                  maintree;                                ///< Server's topic tree
    iersRemoteServersHandle_t              remoteServers;                           ///< Server's remote server information
    ielmLockManagerHandle_t                LockManager;                             ///< Lock manager
    ietrTransactionControlHandle_t         TranControl;                             ///< Transaction control
    ieqnQueueNamespaceHandle_t             queues;                                  ///> Server's named queue namespace
    iepiPolicyInfoGlobalHandle_t           policyInfoGlobal;                        ///> Server's policy info structure cache
    ieieImportExportGlobalHandle_t         importExportGlobal;                      ///> Server's import/export global structure
    iesmQManagerRecordHandle_t             QueueManagerRecordHead;                  ///> Head of MQ Queue-manager record list
    iesmQManagerRecordHandle_t             QueueManagerRecordTail;                  ///> Tail of MQ Queue-manager record list
    pthread_mutex_t                        MQStoreMutex;                            ///< Mutex synchronising MQ store state
    bool                                   disableAutoQueueCreation;                ///< Whether automatic creation of named queues is disabled
    bool                                   useSpinLocks;                            ///< Whether selected (queue) locks should be spin locks
    bool                                   clusterEnabled;                          ///< Whether clustering is enabled (Cluster.EnableCluster is True)
    bool                                   retainedRepositioningEnabled;            ///< Whether we perform retained message repositioning or not
    bool                                   isIoTEnvironment;                        ///< Whether this server is running in an IoT environment (enabling some IoT specific behavior)
    bool                                   reducedMemoryRecoveryMode;               ///< Whether to use less memory during recovery (which will generally make things slower)
    ieutSizeProfile_t                      sizeProfile;                             ///< What SizeProfile has been set for this instance
    uint32_t                               subListCacheCapacity;                    ///< Sublist cache initial capacity
    uint32_t                               ServerTimestamp;                         ///< Timestamp updated periodically in Store as server is running
    uint32_t                               ServerShutdownTimestamp;                 ///< Timestamp of last shutdown (recovered from store record at startup)
    uint32_t                               ServerTimestampInterval;                 ///< Interval in seconds between server timestamp updates
    uint32_t                               RetMinActiveOrderIdInterval;             ///< Interval in seconds between retained min active orderid scans
    uint32_t                               RetMinActiveOrderIdMaxScans;             ///< Maximum number of scans to allow when working out min active orderId
    uint32_t                               ClusterRetainedSyncInterval;             ///< Interval in seconds between cluster retained message synchronization checks
    uint32_t                               ActiveTimerUseCount;                     ///< Use count for Engine initialisation in timer threads
    uint32_t                               AsyncCBQAlertMaxPause;                   ///< Maximum amount of time in seconds to pause threads when async CBQ alert is raised
    uint64_t                               TransactionThreadJobsClientMinimum;      ///< Minimum number of active clients before transactions start using thread job queues
    uint32_t                               ThreadJobAlgorithm;                      ///< How much work to put on thread job queues and how aggressive should the scavenger be
    uint32_t                               DestroyNonAckerRatio;                    ///< Multiple of maxdepth an unacked msgs must be behind tail of queue to treat client as non-acker
    ism_timer_t                            ServerTimestampTimer;                    ///< Timer used to update the server timestamp
    ism_timer_t                            RetMinActiveOrderIdTimer;                ///< Timer used to update the minimum active orderid for retained messages
    ism_timer_t                            ClusterRetainedSyncTimer;                ///< Timer used to check cluster retained message synchronization
    volatile uint64_t                      TimerEventsRequested;                    ///< Count of opne-off timer events requested but not completed
    pthread_mutex_t                        threadDataMutex;                         ///< Mutex synchronising create/delete of thread data structures
    ieutThreadData_t                      *threadDataHead;                          ///< Head of chain of thread data structures
    uint32_t                               threadIdCounter;                         ///< Human friendly number that distinguishes the threads
    ieutThreadStats_t                      endedThreadStats;                        ///< Thread stats from all ended threads
    ism_config_t                          *configCallbackHandle;                    ///< Handle to the configuration callback registration
    uint32_t                               mqttMsgIdRange;                          ///< Number of unacked messages allowed per mqtt client
    uint32_t                               multiConsumerBatchSize;                  ///< Number of messages given to a consumer before cycling to next consumer in round-robin
    uint32_t                               retainedForwardingDelay;                 ///< Number of seconds to delay before requesting forwarding of retained msgs
    uint32_t                               policiesWithDefaultSelection;            ///< Number of policies which have default selection defined
    ismEngine_MessageSelectionCallback_t   selectionFn;                             ///< Message selection callback
    ismEngine_RetainedForwardingCallback_t retainedForwardingFn;                    ///< Retained message forwarding callback function
    ismEngine_DeliveryFailureCallback_t    deliveryFailureFn;                       ///< Called when we fail to deliver messages
    ismEngineRunPhase_t                    runPhase;                                ///< Run phase of the engine
    ismEngineComponentStatus_t             componentStatus[ismENGINE_STATUS_COUNT]; ///< Status of engine components
    volatile uint64_t                      totalSubsCount;                          ///< Overall count of the subscriptions in the maintree
    volatile uint64_t                      totalDCNTopicSubs;                       ///< Overall count of subscriptions that match $SYS/DisconnectedClientNotification
    volatile uint64_t                      totalClientStatesCount;                  ///< Overall count of the (non-internal) clientStates in the client state table
    volatile uint64_t                      totalActiveClientStatesCount;            ///< Overall count of all clientStates that are currently active (i.e. from which work could come)
    volatile uint64_t                      totalNonFatalFFDCs;                      ///< Overall count of nonfatal FFDCs
    iemeExpiryControlHandle_t              msgExpiryControl;                        ///< Server's at-rest message expiry control information
    ieceExpiryControlHandle_t              clientStateExpiryControl;                ///< Server's clientState expiry control information
    ietjThreadJobControlHandle_t           threadJobControl;                        ///< Server's per-thread job control information
    uint64_t                               memUpdateCount;                          ///< Current value of the memory update count used in deferred freeing
    ieutDeferredFreeListHandle_t           deferredFrees;                           ///< Engine-wide deferred free areas
    uint64_t                               freeMemReservedMB;                       ///< On low mem machines (see next var), ignore this much free mem in our count of free mem
    uint64_t                               freeMemRsvdThresholdMB;                  ///< If a system has less than this total memory, reserve soime free mem for non IMA use (see prev param)
    iereResourceSetStatsControlHandle_t    resourceSetStatsControl;                 ///< ResourceSet Stats control
#if (ieutTRACEHISTORY_SAVEDTHREADS > 0)
#if (ieutTRACEHISTORY_BUFFERSIZE > 0)
    ismEngine_StoredThreadData_t           storedThreadData[ieutTRACEHISTORY_SAVEDTHREADS];
    uint32_t                               traceHistoryNextThread;
#endif
#endif
} ismEngine_Server_t;

#define ismENGINE_SERVER_STRUCID       "ESVR"
#define ismENGINE_SERVER_STRUCID_ARRAY {'E','S','V','R'}

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ismEngine_Server_t(__file)\
    iedm_descriptionStart(__file, ismEngine_Server_t, StrucId, ismENGINE_SERVER_STRUCID);\
    iedm_describeMember(char [4],                               StrucId);\
    iedm_describeMember(ismStore_Handle_t,                      hStoreSCR);\
    iedm_describeMember(pthread_mutex_t,                        Mutex);\
    iedm_describeMember(iecsHashTable_t *,                      ClientTable);\
    iedm_describeMember(iettTopicTree_t *,                      maintree);\
    iedm_describeMember(iersRemoteServers_t *,                  remoteServers);\
    iedm_describeMember(ielmLockManager_t *,                    LockManager);\
    iedm_describeMember(ietrTransactionControl_t *,             TranControl);\
    iedm_describeMember(ieqnQueueNamespace_t *,                 queues);\
    iedm_describeMember(iepiPolicyInfoGlobal_t *,               policyInfoGlobal);\
    iedm_describeMember(ieieImportExportGlobal_t *,             importExportGlobal);\
    iedm_describeMember(iesmQManagerRecord_t *,                 QueueManagerRecordHead);\
    iedm_describeMember(iesmQManagerRecord_t *,                 QueueManagerRecordTail);\
    iedm_describeMember(pthread_mutex_t,                        MQStoreMutex);\
    iedm_describeMember(bool,                                   useSpinLocks);\
    iedm_describeMember(bool,                                   clusterEnabled);\
    iedm_describeMember(bool,                                   retainedRepositioningEnabled);\
    iedm_describeMember(bool,                                   isIoTEnvironment);\
    iedm_describeMember(ieutSizeProfile_t,                      sizeProfile);\
    iedm_describeMember(uint32_t,                               subListCacheCapacity);\
    iedm_describeMember(uint32_t,                               ServerTimestamp);\
    iedm_describeMember(uint32_t,                               ServerShutdownTimestamp);\
    iedm_describeMember(uint32_t,                               ServerTimestampInterval);\
    iedm_describeMember(uint32_t,                               RetMinActiveOrderIdInterval);\
    iedm_describeMember(uint32_t,                               RetMinActiveOrderIdMaxScans);\
    iedm_describeMember(uint32_t,                               ClusterRetainedSyncInterval);\
    iedm_describeMember(uint32_t,                               AsyncCBQAlertMaxPause);\
    iedm_describeMember(uint64_t,                               TransactionThreadJobsClientMinimum);\
    iedm_describeMember(uint32_t,                               DestroyNonAckerRatio);\
    iedm_describeMember(uint32_t,                               ActiveTimerUseCount);\
    iedm_describeMember(ism_timer_t,                            ServerTimestampTimer);\
    iedm_describeMember(ism_timer_t,                            RetMinActiveOrderIdTimer);\
    iedm_describeMember(ism_timer_t,                            ClusterRetainedSyncTimer);\
    iedm_describeMember(pthread_mutex_t,                        threadDataMutex);\
    iedm_describeMember(ieutThreadData_t *,                     threadDataHead);\
    iedm_describeMember(ieutThreadStats_t,                      endedThreadStats);\
    iedm_describeMember(ism_config_t *,                         configCallbackHandle);\
    iedm_describeMember(uint32_t,                               mqttMsgIdRange);\
    iedm_describeMember(uint32_t,                               multiConsumerBatchSize);\
    iedm_describeMember(uint32_t,                               retainedForwardingDelay);\
    iedm_describeMember(uint32_t,                               policiesWithDefaultSelection);\
    iedm_describeMember(ismEngine_MessageSelectionCallback_t,   selectionFn);\
    iedm_describeMember(ismEngine_RetainedForwardingCallback_t, retainedForwardingFn);\
    iedm_describeMember(ismEngine_DeliveryFailureCallback_t,    deliveryFailureFn);\
    iedm_describeMember(ismEngineRunPhase_t,                    runPhase);\
    iedm_describeMember(uint64_t,                               totalSubsCount);\
    iedm_describeMember(uint64_t,                               totalDCNTopicSubs);\
    iedm_describeMember(uint64_t,                               totalClientStatesCount);\
    iedm_describeMember(uint64_t,                               totalActiveClientStatesCount);\
    iedm_describeMember(iemeExpiryControl_t *,                  msgExpiryControl);\
    iedm_describeMember(ieceExpiryControl_t *,                  clientStateExpiryControl);\
    iedm_describeMember(ietjThreadJobQueueControl_t *,          threadJobControl);\
    iedm_describeMember(uint64_t,                               memUpdateCount);\
    iedm_describeMember(iereResourceSetStatsControl_t *,        resourceSetStatsControl);\
    iedm_descriptionEnd;

//*********************************************************************
/// @brief  Unreleased Message State
///
/// Represents the unreleased message state for a client-state.
//*********************************************************************
#define ismENGINE_UNRELEASED_CHUNK_SIZE 64

typedef struct ismEngine_UnreleasedState_t
{
    char                            StrucId[4];                ///< Eyecatcher "EUNR"
    uint8_t                         InUse;                     ///< Number of slots currently in use
    uint8_t                         Limit;                     ///< Number of slots that have been used
    uint8_t                         Capacity;                  ///< Number of slots in this structure
    struct
    {
        bool                            fSlotInUse;            ///< Whether this slot is in use
        bool                            fUncommitted;          ///< Whether this slot is uncommitted
        uint32_t                        UnrelDeliveryId;       ///< Unreleased delivery ID used in the protocol
        ismStore_Handle_t               hStoreUnrelStateObject;///< Store handle for Unreleased Message State (only used if client-state is durable)
    } Slot[ismENGINE_UNRELEASED_CHUNK_SIZE];                   ///< Array of unreleased message information
    struct ismEngine_UnreleasedState_t *pNext;                 ///< Ptr to next chunk of unreleased state
} ismEngine_UnreleasedState_t;

#define ismENGINE_UNRELEASED_STATE_STRUCID "EUNR"

//*********************************************************************
/// @brief The operation state of a ClientState
///
/// For durable ClientStates that have not been explicitly discarded:
///
///   Active -> Disconnecting -> CleaningUp -> Zombie -> Freeing
///
/// For nondurable ClientStates, or those that have been explicitly
/// discarded:
///
///   Active -> Disconnecting -> CleaningUp -> Freeing
///
/// The runtime decision about whether to make the client a Zombie is
/// at the end of Cleaning up (iecs_completeCleanupRemainingResources).
///
/// Rehydrated clientStates Always start at Zombie.
///
/// The UseCount, OpState and fInTable for a client are altered under
/// the UseCountLock for that client.
//*********************************************************************
typedef enum __attribute__ ((__packed__)) tag_iecsOpState_t
{
    iecsOpStateActive = 0,
    iecsOpStateDisconnecting = 1,
    iecsOpStateNonDurableCleanup = 2,
    iecsOpStateZombie = 3,
    iecsOpStateZombieRemoval = 4,
    iecsOpStateZombieExpiry = 5,
    iecsOpStateZombieCleanup = 6,
    iecsOpStateFreeing = 7,
    // iecsOpStateLAST must appear at the end
    iecsOpStateLAST
} iecsOpState_t;

//*********************************************************************
/// @brief The durability of a ClientState
//*********************************************************************
typedef enum __attribute__ ((__packed__)) tag_iecsDurability_t
{
    iecsNonDurable = 0,
    iecsDurable = 1,
    iecsInheritOrNonDurable = 2,
    iecsInheritOrDurable = 3,
} iecsDurability_t;

//*********************************************************************
/// @brief The type of take over this client performed / is performing
//*********************************************************************
typedef enum __attribute__ ((__packed__)) tag_iecsTakeover_t
{
    iecsNoTakeover = 0,
    iecsStealTakeover = 1,
    iecsZombieTakeover = 2,
    iecsNonAckingClient = 3,
} iecsTakeover_t;

#define iecsEXPIRY_INTERVAL_INFINITE       UINT32_MAX    // ClientState never expires
#define iecsWILLMSG_TIME_TO_LIVE_INFINITE  0             // Will Message never expires

//*********************************************************************
/// @brief  Client-State
///
/// Represents a client application.
/// May have a client ID which can be used to provide a way of
/// associating state with a particular client application instance.
//*********************************************************************
typedef struct ismEngine_ClientState_t
{
    char                               StrucId[4];                ///< Eyecatcher "ECLI"
    uint32_t                           UseCount;                  ///< Number of uses of this client-state - must not deallocate when > 0
    pthread_spinlock_t                 UseCountLock;              ///< Lock to take when modifying the UseCount (and OpState)
    iecsOpState_t                      OpState;                   ///< The current operation state of this clientState structure
    bool                               fTableRemovalRequired;     ///< Whether this client state needs to be removed from the table
    bool                               fDiscardDurable;           ///< Whether to discard durable state
    iecsDurability_t                   Durability;                ///< The durability of the client-state
    bool                               fCleanStart;               ///< Whether client requested a clean start or not.
    bool                               fLeaveResourcesAtRestart;  ///< Whether to leave resources at restart (because they've been taken over by another client state)
    iecsTakeover_t                     Takeover;                  ///< The type of takeover performed by this client (from a zombie, active steal etc)
    bool                               fWillMessageUpdating;      ///< Whether will message is being updated
    bool                               fCountExternally;          ///< Whether this clientState is included in external counts
    bool                               fSuspendExpiry;            ///< Whether this clientState is in the middle of an operation that means the expiry reaper should ignore it at the moment
    ismStore_Handle_t                  hStoreCSR;                 ///< Store handle for Client State Record
    ismStore_Handle_t                  hStoreCPR;                 ///< Store handle for Client Properties Record
    ism_time_t                         LastConnectedTime;         ///< For a zombie, the last time the client-state was not a zombie
    ism_time_t                         ExpiryTime;                ///< For a zombie, the time when this client-state will expire (0 for no expiry)
    ism_time_t                         WillDelayExpiryTime;       ///< For a zombie, the time when the requested WillDelay expires / expired (and so the will message can be published)
    pthread_mutex_t                    Mutex;                     ///< Mutex synchronising access to this client-state
    iecsHashEntryHandle_t              pHashEntry;                ///< Hash entry for this client-state
    char                              *pClientId;                 ///< The client ID (or NULL, for anonymous)
    char                              *pUserId;                   ///< The creating userId
    ismSecurity_t                     *pSecContext;               ///< Security info for authorisation
    pthread_mutex_t                    UnreleasedMutex;           ///< Mutex synchronising access to unreleased message state
    ismEngine_UnreleasedState_t       *pUnreleasedHead;           ///< Ptr to head of unreleased message state
    void                              *hUnreleasedStateContext;   ///< Store context for managing Unreleased Message State objects
    iecsMessageDeliveryInfoHandle_t    hMsgDeliveryInfo;          ///< Handle to message-delivery information
    struct ismEngine_Session_t        *pSessionHead;              ///< Ptr to head of session list
    struct ismEngine_Session_t        *pSessionTail;              ///< Ptr to tail of session list
    struct ismEngine_ClientState_t    *pThief;                    ///< Ptr to thief client-state when client ID has been stolen
    void                              *pStealContext;             ///< Context for steal callback
    ismEngine_StealCallback_t          pStealCallbackFn;          ///< Function pointer for steal callback (called when client ID stolen)
    bool                               fCheckSessionUser;         ///< Only allow a steal if the client state userIDs match
    uint32_t                           WillMessageTimeToLive;     ///< Time to live to use for the will message
    uint32_t                           ExpiryInterval;            ///< Interval (in seconds) after last connected time when clienState should expire
    uint32_t                           WillDelay;                 ///< Delay (in seconds) after last connected time after which the will message should be published
    char                              *pWillTopicName;            ///< Topic name for publishing will message
    ismEngine_MessageHandle_t          hWillMessage;              ///< Handle of will message
    void                              *pPendingCreateContext;     ///< Context for completion of create operation
    ismEngine_CompletionCallback_t     pPendingCreateCallbackFn;  ///< Function pointer for completion callback of create operation
    void                              *pPendingDestroyContext;    ///< Context for completion of destroy operation
    ismEngine_CompletionCallback_t     pPendingDestroyCallbackFn; ///< Function pointer for completion callback of destroy operation
    ieqnQueueHandle_t                  pTemporaryQueues;          ///< Temporary queues created for this client
    struct ismEngine_Transaction_t    *pGlobalTransactions;       ///< Ptr to head of transaction list
    iecsInflightDestinationHandle_t    inflightDestinationHead;   ///< Ptr to destination with messages that must finish delivery
    uint32_t                           protocolId;                ///< Protocol identifier specified for this client state
    uint64_t                           durableObjects;            ///< Number of durable objects associated with this clientState
    ExpectedMsgRate_t                  expectedMsgRate;           ///< What throughput of messages do we expect for this client
    uint32_t                           maxInflightLimit;          ///< Max number of inflight msgs allowed (0=any)
    iereResourceSetHandle_t            resourceSet;               ///< The resource set to which this clientState belongs (if any)
} ismEngine_ClientState_t;

#define ismENGINE_CLIENT_STATE_STRUCID "ECLI"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ismEngine_ClientState_t(__file)\
    iedm_descriptionStart(__file, ismEngine_ClientState_t, StrucId, ismENGINE_CLIENT_STATE_STRUCID);\
    iedm_describeMember(char [4],                        StrucId);\
    iedm_describeMember(uint32_t,                        UseCount);\
    iedm_describeMember(pthread_spinlock_t,              UseCountLock);\
    iedm_describeMember(iecsOpState_t,                   OpState);\
    iedm_describeMember(bool,                            fTableRemovalRequired);\
    iedm_describeMember(bool,                            fDiscardDurable);\
    iedm_describeMember(iecsDurability_t,                Durability);\
    iedm_describeMember(bool,                            fCleanStart);\
    iedm_describeMember(bool,                            fLeaveResourcesAtRestart);\
    iedm_describeMember(iecsTakeover_t,                  Takeover);\
    iedm_describeMember(bool,                            fWillMessageUpdating);\
    iedm_describeMember(ismStore_Handle_t,               hStoreCSR);\
    iedm_describeMember(ismStore_Handle_t,               hStoreCPR);\
    iedm_describeMember(ism_time_t,                      LastConnectedTime);\
    iedm_describeMember(ism_time_t,                      ExpiryTime);\
    iedm_describeMember(pthread_mutex_t,                 Mutex);\
    iedm_describeMember(iecsHashEntryHandle_t,           pHashEntry);\
    iedm_describeMember(char *,                          pClientId);\
    iedm_describeMember(char *,                          pUserId);\
    iedm_describeMember(ismSecurity_t *,                 pSecContext);\
    iedm_describeMember(pthread_mutex_t,                 UnreleasedMutex);\
    iedm_describeMember(ismEngine_UnreleasedState_t *,   pUnreleasedHead);\
    iedm_describeMember(void *,                          hUnreleasedStateContext);\
    iedm_describeMember(iecsMessageDeliveryInfoHandle_t, hMsgDeliveryInfo);\
    iedm_describeMember(ismEngine_Session_t *,           pSessionHead);\
    iedm_describeMember(ismEngine_Session_t *,           pSessionTail);\
    iedm_describeMember(ismEngine_ClientState_t *,       pThief);\
    iedm_describeMember(void *,                          pStealContext);\
    iedm_describeMember(ismEngine_StealCallback_t,       pStealCallbackFn);\
    iedm_describeMember(uint32_t,                        WillMessageTimeToLive);\
    iedm_describeMember(uint32_t,                        ExpiryInterval);\
    iedm_describeMember(char *,                          pWillTopicName);\
    iedm_describeMember(ismEngine_MessageHandle_t,       hWillMessage);\
    iedm_describeMember(void *,                          pPendingCreateContext);\
    iedm_describeMember(ismEngine_CompletionCallback_t,  pPendingCreateCallbackFn);\
    iedm_describeMember(void *,                          pPendingDestroyContext);\
    iedm_describeMember(ismEngine_CompletionCallback_t,  pPendingDestroyCallbackFn);\
    iedm_describeMember(ieqnQueueHandle_t,               pTemporaryQueues);\
    iedm_describeMember(ismEngine_Transaction_t *,       pGlobalTransactions);\
    iedm_describeMember(iecsInflightDestinationHandle_t, inflightDestinationHead);\
    iedm_describeMember(uint32_t,                        protocolId);\
    iedm_describeMember(uint64_t,                        durableObjects);\
    iedm_describeMember(ExpectedMsgRate_t,               expectedMsgRate);\
    iedm_describeMember(uint32_t,                        maxInflightLimit);\
    iedm_describeMember(iereResourceSet_t *,             resourceSet);\
    iedm_descriptionEnd;

//*********************************************************************
/// @brief  Session
///
/// Represents a session between a client application and the server.
/// The session is associated with a thread of control so that it is
/// possible to establish the sequence of message operations for the
/// purposes of ordering.
/// Each session can be associated with one or more transactions.
//*********************************************************************
typedef struct ismEngine_Session_t
{
    char                            StrucId[4];                ///< Eyecatcher "ESES"
    pthread_mutex_t                 Mutex;                     ///< Mutex synchronising access to this session
    struct ismEngine_ClientState_t *pClient;                   ///< Ptr to associated client-state
    struct ismEngine_Session_t     *pPrev;                     ///< Ptr to previous session for this client-state
    struct ismEngine_Session_t     *pNext;                     ///< Ptr to next session for this client-state
    struct ismEngine_Transaction_t *pTransactionHead;          ///< Ptr to head of transaction list
    struct ismEngine_Producer_t    *pProducerHead;             ///< Ptr to head of producer list
    struct ismEngine_Producer_t    *pProducerTail;             ///< Ptr to tail of producer list
    struct ismEngine_Consumer_t    *pConsumerHead;             ///< Ptr to head of consumer list
    struct ismEngine_Consumer_t    *pConsumerTail;             ///< Ptr to tail of consumer list
    bool                            fIsDestroyed;              ///< Whether session has been destroyed but not deallocated
    bool                            fRecursiveDestroy;         ///< Whether session has been destroyed by the client-state's destruction
    bool                            fIsDeliveryStarted;        ///< Whether message delivery is started
    bool                            fIsDeliveryStopping;       ///< Whether message delivery is stopping
    uint32_t                        UseCount;                  ///< Number of uses of this session - must not deallocate when > 0
                                                               ///  inc. +1 for each expiring get in progress
    uint32_t                        ActiveCallbacks;           ///< Number of active message-delivery callbacks (+1 if message delivery enabled)
    uint32_t                        PreNackAllCount;           ///< Usecount of things to do before we nack all remaining messages in flight
                                                               ///  +1 if session not destroyed
                                                               ///  +1 for each consumer who has not reached 0 use-count (i.e. non-zombie)
                                                               ///  +1 for each ack (or ack batch) we are currently processing
    void                           *pPendingDestroyContext;    ///< Context for completion of destroy operation
    ismEngine_CompletionCallback_t  pPendingDestroyCallbackFn; ///< Function pointer for completion callback of destroy operation
    bool                            fEngineControlledSuspend;  ///< Has the engine stopped/about to stop delivery (and hence needs to resume it)
    bool                            fIsTransactional;          ///< Does this session use transactions external to the engine
    bool                            fExplicitSuspends;         ///< Does this session call suspendMessageDelivery explicitly
    pthread_spinlock_t              ackListPutLock;            ///< Taken to add to the list of msgs awaiting acks
    struct tag_iemqQNode_t         *lastAck;                   ///< Page of acks to add the next ack to
    pthread_spinlock_t              ackListGetLock;            ///< Taken to remove from the list of msgs awaiting acks
    struct tag_iemqQNode_t         *firstAck;
    ietrXIDIteratorHandle_t         pXARecoverIterator;        ///< Iterator used in XARecover processing
} ismEngine_Session_t;

#define ismENGINE_SESSION_STRUCID "ESES"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ismEngine_Session_t(__file)\
    iedm_descriptionStart(__file, ismEngine_Session_t, StrucId, ismENGINE_SESSION_STRUCID);\
    iedm_describeMember(char [4],                        StrucId);\
    iedm_describeMember(pthread_mutex_t,                 Mutex);\
    iedm_describeMember(ismEngine_Client_t *,            pClient);\
    iedm_describeMember(ismEngine_Session_t *,           pPrev);\
    iedm_describeMember(ismEngine_Session_t *,           pNext);\
    iedm_describeMember(ismEngine_Transaction_t *,       pTransactionHead);\
    iedm_describeMember(ismEngine_Producer_t *,          pProducerHead);\
    iedm_describeMember(ismEngine_Producer_t *,          pProducerTail);\
    iedm_describeMember(ismEngine_Consumer_t *,          pConsumerHead);\
    iedm_describeMember(ismEngine_Consumer_t *,          pConsumerTail);\
    iedm_describeMember(bool,                            fIsDestroyed);\
    iedm_describeMember(bool,                            fRecursiveDestroy);\
    iedm_describeMember(bool,                            fIsDeliveryStarted);\
    iedm_describeMember(bool,                            fIsDeliveryStopping);\
    iedm_describeMember(uint32_t,                        UseCount);\
    iedm_describeMember(uint32_t,                        ActiveCallbacks);\
    iedm_describeMember(void *,                          pPendingDestroyContext);\
    iedm_describeMember(ismEngine_CompletionCallback_t,  pPendingDestroyCallbackFn);\
    iedm_describeMember(bool,                            fIsTransactional);\
    iedm_describeMember(bool,                            fExplicitSuspends);\
    iedm_describeMember(pthread_spinlock_t,              ackListPutLock);\
    iedm_describeMember(iemqQNode_t *,                   lastAck);\
    iedm_describeMember(pthread_spinlock_t,              ackListGetLock);\
    iedm_describeMember(iemqQNode_t *,                   firstAck);\
    iedm_describeMember(ietrXIDIterator_t *,             pXARecoverIterator);\
    iedm_descriptionEnd;

//*********************************************************************
/// @brief  Transaction
///
/// Used to group messaging operations into atomic units of work.
/// Each transaction is associated with at most one session.
//*********************************************************************
typedef struct ismEngine_Transaction_t
{
    char                            StrucId[4];                ///< Eyecatcher "ETRN"
    uint16_t                        TranFlags;                 ///< Transaction flags
    uint8_t                         TranState;                 ///< Transaction state
    uint8_t                         CompletionStage;           ///< Stage of transaction completion (commit or rollback)
    bool                            fRollbackOnly;             ///< Whether the transaction may only be rolled back
    bool                            fAsStoreTran;              ///< Whether the transaction is implemented as a single store transaction.
    bool                            fStoreTranPublish;         ///< Whether ieds_publish is currently using a single store transaction inside this engine transaction
    bool                            fIncremental;              ///< Whether each SLE must delete their own transaction references.
    bool                            fLockManagerUsed;          ///< Whether any SLE uses the lock manager
    bool                            fDelayedRollback;          ///< Whether rollback should occur when inflight operations finish
    ism_xid_t                      *pXID;                      ///< Pointer to XID
    uint32_t                        StoreRefReserve;           ///< Number of store reservations made
    uint32_t                        StoreRefCount;             ///< Count of references in store transaction
    struct ismEngine_Session_t     *pSession;                  ///< Ptr to associated session
    struct ismEngine_ClientState_t *pClient;                   ///< Association to client (only unprepared unassociated transactions)
    struct ismEngine_Transaction_t *pNext;                     ///< Ptr to next transaction for this session
    ismStore_Handle_t               hTran;                     ///< Handle to transaction in the store
    void                           *pTranRefContext;           ///< Pointer to store context
    ielmLockScopeHandle_t           hLockScope;                ///< Handle to scope for LockManager
    uint64_t                        nextOrderId;               ///< Next record order id
    uint32_t                        TranOpCount;               ///< Number of transactional operations
    uint32_t                        useCount;                  ///< Number of uses of this transaction - must not deallocate when > 0
    struct ietrSLE_Header_t        *pSoftLogHead;              ///< Pointer to softlog start for this transaction
    struct ietrSLE_Header_t        *pSoftLogTail;              ///< Pointer to softlog start for this transaction
    struct ietrSLE_Header_t        *pActiveSavepoint;          ///< Pointer to active savepoint
    ism_time_t                      StateChangedTime;          ///< Time that the transaction state last changed
    iempMemPoolHandle_t             hTranMemPool;              ///< Pool of memory all freed at end of transaction
    uint64_t                        preResolveCount;           ///< Inflight operations etc that should be finished before the transaction can complete
                                                               ///< +1 unless commit,rollback, (end?) are called, +1 for each put and ack
    ieutThreadData_t               *pJobThread;                ///< pThreadData of the thread that can accept jobs (may be NULL)
} ismEngine_Transaction_t;

#define ismENGINE_TRANSACTION_STRUCID "ETRN"

/** @name Transaction states
 * The states of an Engine transaction.
 */

/**@{*/

#define ismENGINE_TRANSTATE_EMPTY               0 ///< Transaction is empty
#define ismENGINE_TRANSTATE_IN_FLIGHT           1 ///< Transaction is in-flight
#define ismENGINE_TRANSTATE_PREPARED            2 ///< Transaction is prepared to commit
#define ismENGINE_TRANSTATE_COMMITTING          3 ///< Transaction is committing
#define ismENGINE_TRANSTATE_ROLLING_BACK        4 ///< Transaction is rolling back
#define ismENGINE_TRANSTATE_HEURISTIC_COMMIT    5 ///< Transaction is heuristically committed
#define ismENGINE_TRANSTATE_HEURISTIC_ROLLBACK  6 ///< Transaction is heuristically rolled back

/**@}*/

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ismEngine_Transaction_t(__file)\
    iedm_descriptionStart(__file, ismEngine_Transaction_t, StrucId, ismENGINE_TRANSACTION_STRUCID);\
    iedm_describeMember(char [4],                  StrucId);\
    iedm_describeMember(uint16_t,                  TranFlags);\
    iedm_describeMember(uint8_t,                   TranState);\
    iedm_describeMember(uint8_t,                   CompletionStage);\
    iedm_describeMember(bool,                      fRollbackOnly);\
    iedm_describeMember(bool,                      fAsStoreTran);\
    iedm_describeMember(bool,                      fStoreTranPublish);\
    iedm_describeMember(bool,                      fIncremental);\
    iedm_describeMember(bool,                      fLockManagerUsed);\
    iedm_describeMember(bool,                      fDelayedRollback);\
    iedm_describeMember(ism_xid_t *,               pXID);\
    iedm_describeMember(uint32_t,                  StoreRefReserve);\
    iedm_describeMember(uint32_t,                  StoreRefCount);\
    iedm_describeMember(ismEngine_Session_t *,     pSession);\
    iedm_describeMember(ismEngine_ClientState_t *, pClient);\
    iedm_describeMember(ismEngine_Transaction_t *, pNext);\
    iedm_describeMember(ismStore_Handle_t,         hTran);\
    iedm_describeMember(void *,                    pTranRefContext);\
    iedm_describeMember(ielmLockScope_t *,         hLockScope);\
    iedm_describeMember(uint64_t,                  nextOrderId);\
    iedm_describeMember(uint32_t,                  TranOpCount);\
    iedm_describeMember(uint32_t,                  useCount);\
    iedm_describeMember(ietrSLE_Header_t *,        pSoftLogHead);\
    iedm_describeMember(ietrSLE_Header_t *,        pSoftLogTail);\
    iedm_describeMember(ism_time_t,                StateChangedTime);\
    iedm_descriptionEnd;

//*********************************************************************
/// @brief  BatchAckState
///
/// State information for acknowledging messages
//*********************************************************************
typedef struct ismEngine_BatchAckState_t
{
    struct ismEngine_Consumer_t *pConsumer;
    ismQHandle_t Qhdl;
    uint32_t ackCount;
    uint32_t preDeleteCountIncrement; //If the acks have been added to a transaction
                                      //we may want to increase the count so the
                                      //queue is not deleted before commit/rollback
    bool deliverOnCompletion; //If set we need to check whether any messages need to be redelivered on completion
} ismEngine_BatchAckState_t;


//*********************************************************************
/// @brief  Destination
///
/// The destination of messaging operations.
//*********************************************************************
typedef struct ismEngine_Destination_t
{
    char                            StrucId[4];                ///< Eyecatcher "EDST"
    uint8_t                         DestinationType;           ///< Destination type
    char                           *pDestinationName;          ///< Destination name
} ismEngine_Destination_t;

#define ismENGINE_DESTINATION_STRUCID "EDST"


//*********************************************************************
/// @brief  Message Producer
///
/// Produces messages to a destination on behalf of a session.
//*********************************************************************
typedef struct ismEngine_Producer_t
{
    char                            StrucId[4];                ///< Eyecatcher "EPRD"
    uint32_t                        UseCount;                  ///< Number of uses of this producer - must not deallocate when > 0
    struct ismEngine_Session_t     *pSession;                  ///< Ptr to associated session
    struct ismEngine_Destination_t *pDestination;              ///< Ptr to destination
    struct ismEngine_Producer_t    *pPrev;                     ///< Ptr to previous producer for this session
    struct ismEngine_Producer_t    *pNext;                     ///< Ptr to next producer for this session
    iepiPolicyInfoHandle_t          pPolicyInfo;               ///< Messaging policy being used by this producer
    bool                            fIsDestroyed;              ///< Whether producer has been destroyed but not deallocated
    bool                            fRecursiveDestroy;         ///< Whether producer has been destroyed by the session's destruction
    void                           *pPendingDestroyContext;    ///< Context for completion of destroy operation
    ismEngine_CompletionCallback_t  pPendingDestroyCallbackFn; ///< Function pointer for completion callback of destroy operation
    void                           *engineObject;              ///< Ptr to associated engine object (for named queues only)
    ismQHandle_t                    queueHandle;               ///< Handle to the associated (internal) queue (for named queues only)
} ismEngine_Producer_t;

#define ismENGINE_PRODUCER_STRUCID "EPRD"


//*********************************************************************
/// @brief Subscription
///
/// Subscribes to messages on a particular topic
//*********************************************************************
typedef struct ismEngine_Subscription_t
{
    char                             StrucId[4];                 ///< Eyecatcher "ESUB"
    uint32_t                         useCount;                   ///< Number of users of (references to) this subscription
    uint32_t                         consumerCount;              ///< Count of consumers (subscribers) on this subscription
    uint32_t                         subOptions;                 ///< Options specified when the subscription was created (ismENGINE_SUBSCRIPTION_OPTION_* values)
    volatile uint32_t                internalAttrs;              ///< Internal options (iettSUBATTR_ values)
    uint32_t                         nodeListIndex;              ///< Index of the subscription in (unsorted) node list
    char                            *subName;                    ///< Subscription name
    char                            *clientId;                   ///< Client Id of the owning client for durable subs
    ismQHandle_t                     queueHandle;                ///< Handle of associated queue
    iettSubsNodeHandle_t             node;                       ///< Associated node in the subscription tree
    void                            *flatSubProperties;          ///< Properties supplied with this subscription at creation time
    size_t                           flatSubPropertiesLength;    ///< Length of the subscription properties
    ismRule_t                       *selectionRule;              ///< Pointer to compiled selection string
    uint32_t                         selectionRuleLen;           ///< Selection string length
    uint32_t                         clientIdHash;               ///< Hash of the clientId for this subscription
    uint32_t                         subNameHash;                ///< Hash of the subscription name (0 for no subname)
    ismEngine_SubId_t                subId;                      ///< SubId associated with this subscription (0 for none)
    iereResourceSetHandle_t          resourceSet;                ///< Resource set to which this subscription belongs (if any)
    struct ismEngine_Subscription_t *prev;                       ///< Previous subscription
    struct ismEngine_Subscription_t *next;                       ///< Next subscription
} ismEngine_Subscription_t;

#define ismENGINE_SUBSCRIPTION_STRUCID "ESUB"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ismEngine_Subscription_t(__file)\
    iedm_descriptionStart(__file, ismEngine_Subscription_t, StrucId, ismENGINE_SUBSCRIPTION_STRUCID);\
    iedm_describeMember(char [4],                       StrucId);\
    iedm_describeMember(uint32_t,                       useCount);\
    iedm_describeMember(uint32_t,                       consumerCount);\
    iedm_describeMember(uint32_t,                       subOptions);\
    iedm_describeMember(uint32_t,                       internalAttrs);\
    iedm_describeMember(uint32_t,                       nodeListIndex);\
    iedm_describeMember(char *,                         subName);\
    iedm_describeMember(char *,                         clientId);\
    iedm_describeMember(ismQHandle_t,                   queueHandle);\
    iedm_describeMember(iettSubsNode_t *,               node);\
    iedm_describeMember(void *,                         flatSubProperties);\
    iedm_describeMember(size_t,                         flatSubPropertiesLength);\
    iedm_describeMember(ismRule_t *,                    selectionRule);\
    iedm_describeMember(uint32_t,                       selectionRuleLen);\
    iedm_describeMember(uint32_t,                       clientIdHash);\
    iedm_describeMember(uint32_t,                       subNameHash);\
    iedm_describeMember(ismEngine_SubId_t,              subId);\
    iedm_describeMember(iereResourceSet_t *,            resourceSet);\
    iedm_describeMember(ismEngine_Subscription_t *,     prev);\
    iedm_describeMember(ismEngine_Subscription_t *,     next);\
    iedm_descriptionEnd;

//*********************************************************************
/// @brief Remote Server
///
/// Remote Server
//*********************************************************************
typedef struct ismEngine_RemoteServer_t
{
    char                             StrucId[4];                 ///< Eyecatcher "ERSV"
    uint32_t                         useCount;                   ///< Number of users of (references to) this remote server
    uint32_t                         consumerCount;              ///< Count of consumers on this remote server
    uint32_t                         internalAttrs;              ///< Internal options (iersREMSRVATTR_ values)
    char                            *serverName;                 ///< Name of this server
    char                            *serverUID;                  ///< Unique ID for remote server
    ismCluster_RemoteServerHandle_t  clusterHandle;              ///< The handle the cluster knows for this server
    ismStore_Handle_t                hStoreDefn;                 ///< The handle of the RDR
    ismStore_Handle_t                hStoreProps;                ///< The handle of the current RPR
    ismQHandle_t                     lowQoSQueueHandle;          ///< Handle of associated low (0) QoS queue
    ismQHandle_t                     highQoSQueueHandle;         ///< Handle of associated high (1/2) QoS queue
    void                            *clusterData;                ///< Cluster Data pointer (used during recovery and update)
    size_t                           clusterDataLength;          ///< Cluster Data length (used during recovery and update)
    struct ismEngine_RemoteServer_t *prev;                       ///< Previous remote server
    struct ismEngine_RemoteServer_t *next;                       ///< Next remote server
} ismEngine_RemoteServer_t;

#define ismENGINE_REMOTESERVER_STRUCID "ERSV"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ismEngine_RemoteServer_t(__file)\
    iedm_descriptionStart(__file, ismEngine_RemoteServer_t, StrucId, ismENGINE_REMOTESERVER_STRUCID);\
    iedm_describeMember(char [4],                       StrucId);\
    iedm_describeMember(uint32_t,                       useCount);\
    iedm_describeMember(uint32_t,                       consumerCount);\
    iedm_describeMember(uint32_t,                       internalAttrs);\
    iedm_describeMember(char *,                         serverName);\
    iedm_describeMember(char *,                         serverUID);\
    iedm_describeMember(ismCluster_RemoteServer_t *,    clusterHandle);\
    iedm_describeMember(ismStore_Handle_t,              hStoreDefn);\
    iedm_describeMember(ismStore_Handle_t,              hStoreProps);\
    iedm_describeMember(ismQHandle_t,                   lowQoSQueueHandle);\
    iedm_describeMember(ismQHandle_t,                   highQoSQueueHandle);\
    iedm_describeMember(void *,                         clusterData);\
    iedm_describeMember(size_t,                         clusterDataLength);\
    iedm_describeMember(ismEngine_RemoteServer_t *,     prev);\
    iedm_describeMember(ismEngine_RemoteServer_t *,     next);\
    iedm_descriptionEnd;

//*********************************************************************
/// @brief Remote Server Pending Update
///
/// Remote Server Pending Update
//*********************************************************************
typedef struct ismEngine_RemoteServer_PendingUpdate_t
{
    char                              StrucId[4];         ///< Eyecatcher "ERPU"
    uint32_t                          remoteServerCount;  ///< Number of servers
    ismEngine_RemoteServer_t        **remoteServers;      ///< Array of servers
    uint32_t                          remoteServerMax;    ///< Maximum number of array elements
} ismEngine_RemoteServer_PendingUpdate_t;

#define ismENGINE_REMOTESERVER_PENDINGUPDATE_STRUCID "ERPU"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ismEngine_RemoteServer_PendingUpdate_t(__file)\
    iedm_descriptionStart(__file, ismEngine_RemoteServer_PendingUpdate_t, StrucId, ismENGINE_REMOTESERVER_PENDINGUPDATE_STRUCID);\
    iedm_describeMember(char [4],                       StrucId);\
    iedm_describeMember(uint32_t,                       remoteServerCount);\
    iedm_describeMember(ismEngine_RemoteServer_t **,    remoteServers);\
    iedm_describeMember(uint32_t,                       remoteServerMax);\
    iedm_descriptionEnd;


//***********************************************************************
/// @brief Async Put Information
///
/// Data about an inflight put that is using an asynchronous store commit
//***********************************************************************
typedef struct ismEngine_AsyncPut_t
{
    char                            StrucId[4];            ///< Eyecatcher "EAPD"
    ismEngine_UnreleasedHandle_t    unrelDeliveryIdHandle; ///< Handle to the message being put
    int32_t                         callerRC;              ///< The return code we should pass back to the caller
    bool                            engineLocalTran;       ///< Did the engine create the transaction or is it external
    ismEngine_TransactionHandle_t   hTran;                 ///< Tran to be committed (not valid in callback)
    ismEngine_Session_t            *pSessionToRelease;     ///< Session to release after commit
    ismEngine_Producer_t           *pProducerToRelease;    ///< Producer to release after commit
    size_t                          contextLength;         ///< Engine-caller context length
                                                           /// (context stored after this structure)
    ismEngine_CompletionCallback_t  pCallbackFn;           ///< Engine-caller completion callback
    uint64_t                        asyncId;               ///< When not using a engine transaction, this is the commit id used
} ismEngine_AsyncPut_t;

#define ismENGINE_ASYNCPUT_STRUCID "EAPD"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ismEngine_AsyncPut_t(__file)\
    iedm_descriptionStart(__file, ismEngine_AsyncPut_t, StrucId, ismENGINE_ASYNCPUT_STRUCID);\
    iedm_describeMember(char [4],                       StrucId);\
    iedm_describeMember(ismEngine_UnreleasedHandle_t,   unrelDeliveryIdHandle);\
    iedm_describeMember(int32_t,                        callerRC);\
    iedm_describeMember(bool,                           engineLocalTran);\
    iedm_describeMember(ismEngine_TransactionHandle_t,  hTran);\
    iedm_describeMember(ismEngine_Session_t,            pSessionToRelease);\
    iedm_describeMember(ismEngine_Producer_t,           pProducerToRelease);\
    iedm_describeMember(size_t,                         contextLength);\
    iedm_describeMember(ismEngine_CompletionCallback_t, pCallbackFn);\
    iedm_describeMember(uint64_t,                       asyncId);\
    iedm_descriptionEnd;


//***********************************************************************
/// @brief Async Acknowledge Information
///
/// Reduces a usecount once an inflight ack has been completed
//***********************************************************************
typedef struct ismEngine_AsyncAckCompleted_t
{
    char                            StrucId[4];            ///< Eyecatcher "EAAD"
    ismEngine_Session_t            *pSession;              ///< Session doing the ack
    uint32_t                        ackOptions;            ///< Type of ack being completed
} ismEngine_AsyncAckCompleted_t;

#define ismENGINE_ASYNCACKCOMPLETED_STRUCID "EAAD"

typedef struct ismEngine_AsyncProcessAcks_t
{
    char                            StrucId[4];               ///< Eyecatcher "EAPA"
    ismEngine_Session_t            *pSession;                 ///< Session doing the ack
    ismEngine_Transaction_t        *pTran;
    ismEngine_BatchAckState_t       batchState;
    uint32_t                        ackOptions;               ///< Consumed, received etc..
    uint32_t                        nextAck;                  ///< index of next ack to process
    uint32_t                        numAcks;                  ///< number of acks in this batch
    bool                            triggerSessionRedelivery;
} ismEngine_AsyncProcessAcks_t;

#define ismENGINE_ASYNCPROCESSACKS_STRUCID "EAPA"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ismEngine_AsyncAck_t(__file)\
    iedm_descriptionStart(__file, ismEngine_AsyncAck_t, StrucId, ismENGINE_ASYNCACKCOMPLETED_STRUCID);\
    iedm_describeMember(char [4],                       StrucId);\
    iedm_describeMember(ismEngine_Session_t,            pSession);\
    iedm_descriptionEnd;

//*********************************************************************
/// @brief Queue type
///
/// These values must not change, since they're written into the Store.
//*********************************************************************
typedef enum ismQueueType_t
{
    multiConsumer = 1,   ///< General-purpose queue
    simple,              ///< No transactions, no acks - single getter
    intermediate         ///< No transactions - single getter
} ismQueueType_t;


//*********************************************************************
/// @brief  Waiter status
///
/// When a consumer is used with the multi-consumer queue, This 
// records the state of a consumer according to a queue.
//*********************************************************************
typedef uint64_t iewsWaiterStatus_t;
#define IEWS_WAITERSTATUS_DISCONNECTED     0

///********************************************************************
/// @brief Cursor for a multiconsumer queue
///
/// Used by consumers when browsing (or doing message selection)
///********************************************************************
typedef union tag_iemqCursor_t
{
    __uint128_t whole;
    struct
    {
        uint64_t orderId;
        struct tag_iemqQNode_t *pNode;
    } c;
} iemqCursor_t;

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iemqCursor_t(__file)\
    iedm_descriptionStart(__file, iemqCursor_t,,"");\
    iedm_describeMember(uint64_t,     c.orderId);\
    iedm_describeMember(iemQNode_t *, c.pNode);\
    iedm_descriptionEnd;

//*********************************************************************
/// @brief Use-counts for a consumer
///********************************************************************
typedef union ismEngine_ConsumerCounts_t
{
    uint64_t whole;
    struct
    {
        uint32_t useCount;        ///< Number of things that are using this consumer
        uint32_t pendingAckCount; ///< Number of acks remaining for this consumer
    } parts;
} ismEngine_ConsumerCounts_t;

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    When the messages are relinquished should they be acked (removed from the queue)
///                                          or nacked (made available to other clients)
///////////////////////////////////////////////////////////////////////////////
typedef enum __attribute__ ((__packed__)) tag_ismEngine_RelinquishType_t
{
    ismEngine_RelinquishType_NONE = 0,                ///<No relinquish
    ismEngine_RelinquishType_ACK_HIGHRELIABLITY  = 1, ///< Remove messages from queue if qos ==2 to ensure no possibility of redelivery
    ismEngine_RelinquishType_NACK_ALL = 2,            ///< Make all messages available to other clients
} ismEngine_RelinquishType_t;

//*********************************************************************
/// @brief  Message Consumer
///
/// Consumes messages from a destination on behalf of a session.
//*********************************************************************
typedef struct ismEngine_Consumer_t
{
    char                                  StrucId[4];                ///< Eyecatcher "ECNS"
    struct ismEngine_Session_t           *pSession;                  ///< Ptr to associated session
    struct ismEngine_Destination_t       *pDestination;              ///< Ptr to associated destination
    struct ismEngine_Consumer_t          *pPrev;                     ///< Ptr to previous consumer for this session
    struct ismEngine_Consumer_t          *pNext;                     ///< Ptr to next consumer for this session
    ismEngine_RelinquishType_t            relinquishOnTerm;         ///< relinquish all messages for this client when terminated
    bool                                  fDestructiveGet;           ///< true for normal consumer. False for browser
    bool                                  fAcking;                   ///< true if consumer acks messages delivered to it
    bool                                  fShortDeliveryIds;         ///< True if we generate & store 16-bit delivery ids for this consumer (ids unique in scope of client)
    bool                                  fNonPersistentDlvyCount;  ///< If true we don't update delivery count on msgs for this consumer
    bool                                  fConsumeQos2OnDisconnect;      ///< If true, messages delivered to this consumer will be consumed on disconnect if not nacked first
    bool                                  fExpiringGet;              ///< Is there a timeout on this consumer that will destroy it
    bool                                  fRedelivering;             ///< Redelivering messages. (Only used for shared MQTT Subs)
    bool                                  fIsDestroyed;              ///< Whether consumer has been destroyed but not deallocated
    bool                                  fDestroyCompleted;         ///< Whether we have reported that destroyConsumer has completed
    bool                                  fRecursiveDestroy;         ///< Whether consumer has been destroyed by the session's destruction
    volatile bool                         fFlowControl;              ///< Whether delivery has been paused for per-consumer flow control
    uint32_t                              DestinationOptions;        ///< Destination options used when consumer created
    volatile ismEngine_ConsumerCounts_t   counts;                    ///< Number of uses of this consumer - must not deallocate when any > 0
    void                                 *pMsgCallbackContext;       ///< Context for message-delivery callback
    ismEngine_MessageCallback_t           pMsgCallbackFn;            ///< Function pointer for message-delivery callback
    void                                 *pPendingDestroyContext;    ///< Context for completion of destroy operation
    ismEngine_CompletionCallback_t        pPendingDestroyCallbackFn; ///< Function pointer for completion callback of destroy operation
    void                                 *engineObject;              ///< Ptr to associated engine object (subscription or named queue)
    ismQHandle_t                          queueHandle;               ///< Handle to the associated (internal) queue
    char                                 *consumerProperties;        ///< Consumer properties
    char                                 *selectionString;           ///< Pointer to selection string
    ismRule_t                            *selectionRule;             ///< Pointer to compiled selection string
    uint32_t                              selectionRuleLen;          ///< Selection string length
    //Some fields used by the multiConsumerQ
    volatile iewsWaiterStatus_t           iemqWaiterStatus;          ///< An IEWS_ value to say enabled, disabled etc.
    struct ismEngine_Consumer_t          *iemqPNext;                 ///< Next consumer on the same queue
    struct ismEngine_Consumer_t          *iemqPPrev;                 ///< Prev consumer on the same queue
    volatile iemqCursor_t                 iemqCursor;                ///< Consumer cursor
    uint64_t                              iemqNoMsgCheckVal;         ///< Check value used when no more msgs for consumer (value of Q->checkWaitersVal)
    ietrSLEHeaderHandle_t                 iemqCachedSLEHdr;          ///< SLE memory allocated during delivery for use in acknowledge
    iecsMessageDeliveryInfoHandle_t       hMsgDelInfo;               ///< Message-delivery information handle for MDRs (MQTT only)
    uint64_t                              failedSelectionCount;      ///< Temporary
    uint64_t                              successfulSelectionCount;  ///< Temporary
#ifdef HAINVESTIGATIONSTATS
    int64_t                               lastGapBetweenGets;
    struct timespec                       lastGetTime;
    uint64_t                              lastBatchSize;
    struct timespec                       lastDeliverCommitStart;
    struct timespec                       lastDeliverCommitEnd;
    struct timespec                       lastDeliverEnd;
#endif
    //end fields used by the multiConsumerQ
} ismEngine_Consumer_t;

#define ismENGINE_CONSUMER_STRUCID "ECNS"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ismEngine_Consumer_t(__file)\
    iedm_descriptionStart(__file, ismEngine_Consumer_t, StrucId, ismENGINE_CONSUMER_STRUCID);\
    iedm_describeMember(char [4],                        StrucId);\
    iedm_describeMember(ismEngine_Session_t *,           pSession);\
    iedm_describeMember(ismEngine_Destination_t *,       pDestination);\
    iedm_describeMember(ismEngine_Consumer_t *,          pPrev);\
    iedm_describeMember(ismEngine_Consumer_t *,          pNext);\
    iedm_describeMember(ismEngine_RelinquishType_t,      relinquishOnTerm);\
    iedm_describeMember(bool,                            fDestructiveGet);\
    iedm_describeMember(bool,                            fAcking);\
    iedm_describeMember(bool,                            fShortDeliveryIds);\
    iedm_describeMember(uint32_t,                        DestinationOptions);\
    iedm_describeMember(bool,                            fIsDestroyed);\
    iedm_describeMember(bool,                            fDestroyCompleted);\
    iedm_describeMember(bool,                            fRecursiveDestroy);\
    iedm_describeMember(ismEngine_ConsumerCounts_t,      counts);\
    iedm_describeMember(void *,                          pMsgCallbackContext);\
    iedm_describeMember(ismEngine_MessageCallback_t,     pMsgCallbackFn);\
    iedm_describeMember(void *,                          pPendingDestroyContext);\
    iedm_describeMember(ismEngine_CompletionCallback_t,  pPendingDestroyCallbackFn);\
    iedm_describeMember(void *,                          engineObject);\
    iedm_describeMember(ismQHandle_t,                    queueHandle);\
    iedm_describeMember(char *,                          consumerProperties);\
    iedm_describeMember(char *,                          selectionString);\
    iedm_describeMember(ismRule_t *,                     selectionRule);\
    iedm_describeMember(uint32_t,                        selectionRuleLen);\
    iedm_describeMember(iewsWaiterStatus_t,              iemqWaiterStatus);\
    iedm_describeMember(ismEngine_Consumer_t *,          iemqPNext);\
    iedm_describeMember(ismEngine_Consumer_t *,          iemqPPrev);\
    iedm_describeMember(iemqCursor_t,                    iemqCursor);\
    iedm_describeMember(uint64_t,                        iemqNoMsgCheckVal);\
    iedm_describeMember(uint64_t,                        failedSelectionCount);\
    iedm_describeMember(uint64_t,                        successfulSelectionCount);\
    iedm_descriptionEnd;

//*********************************************************************
/// @brief  Message
///
/// Internal representation of a message... definition is in messageInternal.h
/// but we want to describe it for use in engine dumps
///
//********************************************************************

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ismEngine_Message_t(__file)\
    iedm_descriptionStart(__file, ismEngine_Message_t, StrucId, ismENGINE_MESSAGE_STRUCID);\
    iedm_describeMember(char [4],                 StrucId);\
    iedm_describeMember(uint32_t,                 usageCount);\
    iedm_describeMember(ismMessageHeader_t,       Header);\
    iedm_describeMember(uint8_t,                  AreaCount);\
    iedm_describeMember(uint8_t,                  Flags);\
    iedm_describeMember(ismMessagAreaType_t [3],  AreaTypes);\
    iedm_describeMember(size_t [3],               AreaLengths);\
    iedm_describeMember(void * [3],               pAreaData);\
    iedm_describeMember(size_t,                   MsgLength);\
    iedm_describeMember(uint64_t,                 StoreMsg.parts.RefCount);\
    iedm_describeMember(ismStore_Handle_t,        StoreMsg.parts.hStoreMsg);\
    iedm_descriptionEnd;

//*********************************************************************
/// @brief  Engine Restart Transaction Data
///
/// During restart, this structure is given to a rehydration function
/// to say that the record being reconstructed is part of a transaction
//*********************************************************************
typedef struct tag_ismEngine_RestartTransactionData_t {
    ismEngine_Transaction_t *pTran;
    ismStore_Handle_t operationRefHandle;
    ismStore_Reference_t operationReference;
} ismEngine_RestartTransactionData_t;


typedef union
{
    ismEngine_DeliveryHandle_t Full;
    struct
    {
        ismQHandle_t   Q;
        void          *Node;
    } Parts;
} ismEngine_DeliveryInternal_t;

/*********************************************************************/
/*                                                                   */
/* INTERNAL FUNCTION PROTOTYPES                                      */
/*                                                                   */
/*********************************************************************/

// Release a reference to a session, deallocating the session when
// the use-count drops to zero
bool releaseSessionReference(ieutThreadData_t *pThreadData,
                             ismEngine_Session_t *pSession,
                             bool fInline);

//Reduce number of consumers for a session
//(consumers are removed when their usecount hits 0... zombies aren't counted)
void reducePreNackAllCount(ieutThreadData_t *pThreadData,
                                ismEngine_Session_t *pSession);

//Record this consumer has a pending ack
void increaseConsumerAckCount(ismEngine_Consumer_t *pConsumer);

//Reduce the number of pending acks for this consumer
void decreaseConsumerAckCount(ieutThreadData_t *pThreadData,
                              ismEngine_Consumer_t *pConsumer,
                              uint32_t numAcks);

// Acquire a reference to a consumer
void acquireConsumerReference(ismEngine_Consumer_t *pConsumer);

// Release a reference to a consumer, deallocating the consumer when
// the use-count and ack-count drops to zero
uint32_t releaseConsumerReference(ieutThreadData_t *pThreadData,
                                  ismEngine_Consumer_t *pConsumer,
                                  bool fInlineDestroy);

int32_t setupAsyncPublish( ieutThreadData_t *pThreadData
                         , ismEngine_Session_t *pSession
                         , ismEngine_Producer_t *pProducer
                         , void *pContext
                         , size_t contextLength
                         , ismEngine_CompletionCallback_t  pCallbackFn
                         , ietrAsyncTransactionDataHandle_t *phAsyncData);

// Add the session to the specified dump
void dumpSession(ieutThreadData_t *pThreadData,
                 ismEngine_Session_t *pSession,
                 iedmDumpHandle_t dumpHdl);

// Add the producer to the specified dump
void dumpProducer(ieutThreadData_t *pThreadData,
                  ismEngine_Producer_t *pProducer,
                  iedmDumpHandle_t dumpHdl);

// Add the consumer to the specified dump
void dumpConsumer(ieutThreadData_t *pThreadData,
                  ismEngine_Consumer_t *pConsumer,
                  iedmDumpHandle_t dumpHdl);

//Finish destroying a consumer
void finishDestroyConsumer(ieutThreadData_t *pThreadData,
                           ismEngine_Consumer_t *pConsumer,
                           bool fInline);

//Internal version of ism_engine_stopMessageDelivery()
int32_t stopMessageDeliveryInternal(
        ieutThreadData_t *              pThreadData,
        ismEngine_SessionHandle_t       hSession,
        uint32_t                        InternalFlags,
        void *                          pContext,
        size_t                          contextLength,
        ismEngine_CompletionCallback_t  pCallbackFn);

//Mark a session as undergoing engine flow control.
void ies_MarkSessionEngineControl(ismEngine_Session_t *pSession);

void ismEngine_internal_RestartSession( ieutThreadData_t * pThreadData
                                      , ismEngine_Session_t * pSession
                                      , bool restartOnTimer);

#ifdef HAINVESTIGATIONSTATS
    void ism_engine_recordConsumerStats( ieutThreadData_t *pThreadData
                                       , ismEngine_Consumer_t *pConsumer);
#endif

int32_t ism_engine_lockSession(ismEngine_Session_t *pSession);
void ism_engine_unlockSession(ismEngine_Session_t *pSession);

//
//Flags for the internal version of stop message delivery
//
#define ISM_ENGINE_INTERNAL_STOPMESSAGEDELIVERY_FLAGS_NONE            0
#define ISM_ENGINE_INTERNAL_STOPMESSAGEDELIVERY_FLAGS_ENGINE_STOP   0x1

// Dump First Failure Data Capture to trace
void ieut_ffdc( const char *function
              , uint32_t seqId
              , bool core
              , const char *filename
              , uint32_t lineNo
              , char *label
              , uint32_t retcode
              , ... );

#define ieutTRACE_FFDC(_seqId, _core, _label, _retcode...) ieut_ffdc(__func__, _seqId, _core, __FILE__, __LINE__, _label, _retcode)

char * ieut_queueTypeText( ismQueueType_t queueType
                         , char *buffer
                         , size_t bufferLen );

// Utility function to process *this* thread's job queue (only passing pThreadData to avoid a
// call to ieut_getThreadData because the caller always knows the value).
bool ieut_processJobQueue(ieutThreadData_t *pThreadData);

// Utility function to launch GDB against the running process
// This can be very useful for hard to catch problems (especially when using
// ASAN where a core file is not produced) - making a call to this when some
// unexpected circumstance occurs enables you to debug the error immediately.
static inline void ieut_breakIntoDebugger(void)
{
    int pid = fork();

    if(pid == -1)
    {
        fprintf(stderr, "failed to fork\n");
        fflush(stderr);
    }

    if(pid == 0)
    {
        /* child */
        char cmd[256];
        char *argv[4] = {"sh", "-c", cmd, 0};
        sprintf(cmd, "gdb %s %d", argv[0], getppid());
        execve("/bin/sh", argv, NULL);

        fprintf(stderr, "%s Failed to start GDB!\n", __func__);
        while(1)
        {
            fprintf(stderr, "Sleeping for 10 seconds\n");
            sleep(10);
        }

        exit(127);
    }

    // may need to be increased, shouldn't continue until gdb attached
    // other possible solution is a while loop whereby you manually
    // change the loop control variable from within gdb after it started
    sleep(3);
}

#ifdef HAINVESTIGATIONSTATS
#ifndef NANOS_PER_SEC
#define NANOS_PER_SEC 1000000000
#endif
static inline int64_t timespec_diff_int64( struct timespec *pStart
                                         , struct timespec *pEnd)
{
    struct timespec temp;
    if ((pEnd->tv_nsec - pStart->tv_nsec) < 0) {
        temp.tv_sec = pEnd->tv_sec - pStart->tv_sec-1;
        temp.tv_nsec = NANOS_PER_SEC + pEnd->tv_nsec - pStart->tv_nsec;
    } else {
        temp.tv_sec = pEnd->tv_sec - pStart->tv_sec;
        temp.tv_nsec = pEnd->tv_nsec - pStart->tv_nsec;
    }
    return temp.tv_nsec + (NANOS_PER_SEC * temp.tv_sec);
}
//returns true if p1 > p2
static inline bool timespec_isgreater(struct timespec *p1,struct timespec *p2 )
{
    if (  (p1->tv_sec > p2->tv_sec)
        ||(  (p1->tv_sec == p2->tv_sec)
           &&(p1->tv_nsec > p2->tv_nsec)))
    {
        return true;
    }
    return false;
}
#endif

/*********************************************************************/
/* FFDC PROBE values                                                 */
/*********************************************************************/
#define ieutPROBE_001 1
#define ieutPROBE_002 2
#define ieutPROBE_003 3
#define ieutPROBE_004 4
#define ieutPROBE_005 5
#define ieutPROBE_006 6
#define ieutPROBE_007 7
#define ieutPROBE_008 8
#define ieutPROBE_009 9
#define ieutPROBE_010 10
#define ieutPROBE_011 11
#define ieutPROBE_012 12
#define ieutPROBE_013 13
#define ieutPROBE_014 14
#define ieutPROBE_015 15
#define ieutPROBE_016 16
#define ieutPROBE_017 17
#define ieutPROBE_018 18
#define ieutPROBE_019 19
#define ieutPROBE_020 20
#define ieutPROBE_021 21
#define ieutPROBE_022 22
#define ieutPROBE_023 23
#define ieutPROBE_024 24
#define ieutPROBE_025 25
#define ieutPROBE_026 26
#define ieutPROBE_027 27
#define ieutPROBE_028 28
#define ieutPROBE_029 29
#define ieutPROBE_030 30
#define ieutPROBE_031 31
#define ieutPROBE_032 32
#define ieutPROBE_033 33
#define ieutPROBE_034 34
#define ieutPROBE_035 35
#define ieutPROBE_036 36
#define ieutPROBE_037 37
#define ieutPROBE_038 38
#define ieutPROBE_039 39
#define ieutPROBE_040 40
#define ieutPROBE_041 41
#define ieutPROBE_042 42
#define ieutPROBE_043 43
#define ieutPROBE_044 44
#define ieutPROBE_045 45
#define ieutPROBE_046 46
#define ieutPROBE_047 47
#define ieutPROBE_048 48
#define ieutPROBE_049 49

/*********************************************************************/
/*                                                                   */
/* COMMON MACROS                                                     */
/*   (Static in-line functions preferred for type-checking)          */
/*                                                                   */
/*********************************************************************/

#if !defined(NDEBUG) && defined(ENABLE_LOCK_CHECKING)
#define ismEngine_checkRWLockedForWrite(pTD, lock) assert(((lock).__data.__writer == (pTD)->tid))
#else
#define ismEngine_checkRWLockedForWrite(pTD, lock)
#endif

// Take a pthread_rwlock for write, and check the return code
#define ismEngine_getRWLockForWrite(lock)                            \
{                                                                    \
    int osrc = pthread_rwlock_wrlock((lock));                        \
    if (UNLIKELY(osrc != 0))                                         \
    {                                                                \
        TRACE(ENGINE_SEVERE_ERROR_TRACE,                             \
              "Unexpected rc (%d) from pthread_rwlock_wrlock(%p)\n", \
              osrc, lock);                                           \
        ism_common_shutdown(true);                                   \
    }                                                                \
}

// Take a pthread_rwlock for read, and check the return code
#define ismEngine_getRWLockForRead(lock)                             \
{                                                                    \
    int osrc = pthread_rwlock_rdlock((lock));                        \
    if (UNLIKELY(osrc != 0))                                         \
    {                                                                \
        TRACE(ENGINE_SEVERE_ERROR_TRACE,                             \
              "Unexpected rc (%d) from pthread_rwlock_rdlock(%p)\n", \
              osrc, (lock));                                         \
        ism_common_shutdown(true);                                   \
    }                                                                \
}

// Try and take a pthread_rwlock for read setting rc to 'OK' if the lock
// was given, and ISMRC_LockNotGranted if not - also deal with other failures.
#define ismEngine_tryRWLockForRead(lock, rc)                                \
{                                                                           \
    int osrc = pthread_rwlock_tryrdlock((lock));                            \
    if (osrc != 0)                                                          \
    {                                                                       \
        if (osrc != EBUSY)                                                  \
        {                                                                   \
            TRACE(ENGINE_SEVERE_ERROR_TRACE,                                \
                  "Unexpected rc (%d) from pthread_rwlock_tryrdlock(%p)\n", \
                  osrc, (lock));                                            \
            ism_common_shutdown(true);                                      \
        }                                                                   \
        (rc)=ISMRC_LockNotGranted;                                          \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        (rc)=OK;                                                            \
    }                                                                       \
}

// Unlock a pthread_rwlock, checking for errors.
#define ismEngine_unlockRWLock(lock)                                 \
{                                                                    \
    int osrc = pthread_rwlock_unlock((lock));                        \
    if (UNLIKELY(osrc != 0))                                         \
    {                                                                \
        TRACE(ENGINE_SEVERE_ERROR_TRACE,                             \
              "Unexpected rc (%d) from pthread_rwlock_unlock(%p)\n", \
              osrc, (lock));                                         \
        ism_common_shutdown(true);                                   \
    }                                                                \
}

#if !defined(NDEBUG) && defined(ENABLE_LOCK_CHECKING)
#define ismEngine_checkMutexLocked(pTD, lock) assert(((lock).__data.__owner == (pTD)->tid))
#else
#define ismEngine_checkMutexLocked(pTD, lock)
#endif

// Lock a pthread_mutex, and check the return code
#define ismEngine_lockMutex(lock)                                 \
{                                                                 \
    int osrc = pthread_mutex_lock((lock));                        \
    if (UNLIKELY(osrc != 0))                                      \
    {                                                             \
        TRACE(ENGINE_SEVERE_ERROR_TRACE,                          \
              "Unexpected rc (%d) from pthread_mutex_lock(%p)\n", \
              osrc, (lock));                                      \
        ism_common_shutdown(true);                                \
    }                                                             \
}

// Unlock a pthread_mutex, checking for errors.
#define ismEngine_unlockMutex(lock)                                 \
{                                                                   \
    int osrc = pthread_mutex_unlock((lock));                        \
    if (UNLIKELY(osrc != 0))                                        \
    {                                                               \
        TRACE(ENGINE_SEVERE_ERROR_TRACE,                            \
              "Unexpected rc (%d) from pthread_mutex_unlock(%p)\n", \
              osrc, (lock));                                        \
        ism_common_shutdown(true);                                  \
    }                                                               \
}

#if defined(IMA_UBSAN)
__attribute__((no_sanitize_undefined)) // Knowingly using misaligned comparison
#endif
static inline bool ismEngine_CompareStructId(const char *structID, const char *expectedStructID)
{
    return (*((uint32_t *)structID) == *((uint32_t *)expectedStructID ))?true:false;
}

//Check the 4-byte struct ID is as expected
#if defined(IMA_UBSAN)
__attribute__((no_sanitize_undefined)) // Knowingly using misaligned comparison
#endif
static inline void ismEngine_CheckStructIdLocation(const char *structID, const char *expectedStructID, const char *func, uint32_t seqId, const char *file, int line)
{
#ifndef NDEBUG
    if(UNLIKELY(*((uint32_t *)structID)   != *((uint32_t *)expectedStructID )))
    {
        char ErrorString[256];
        snprintf(ErrorString, 256, "file: %s line: %u - Expected StructId %.*s got: %.*s\n",
                file, (unsigned int)line, 4, expectedStructID, 4, structID);

        ieut_ffdc(func, seqId, true, file, line, ErrorString, ISMRC_Error,
                  "Received StructId", structID, 4,
                  "Expected StructId", expectedStructID, 4,
                  NULL);
    }
#endif
}
#define ismEngine_CheckStructId(structID, expectedStructId, seqId) ismEngine_CheckStructIdLocation(structID, expectedStructId, __func__, seqId, __FILE__, __LINE__)

// Set the 4-byte struct ID
#if defined(IMA_UBSAN)
__attribute__((no_sanitize_undefined)) // Knowingly using misaligned comparison
#endif
static inline void ismEngine_SetStructId(char *structID, const char *newStructID)
{
    *((uint32_t *)structID) = *((uint32_t *)newStructID);
}

// Set the 1st byte of a struct ID to '?' to indicate that it has been freed
static inline void ismEngine_InvalidateStructId(char *structID)
{
    *structID = '?';
}

// Just display the name value pairs in an ism_prop_t for diagnostic purposes
static inline void ismEngine_DisplayProperties(ism_prop_t *props)
{
    const char *displayProperty = NULL;

    printf("\n%s [%p] {\n", __func__, props);
    for (int32_t i = 0; ism_common_getPropertyIndex(props, i, &displayProperty) == 0; i++)
    {
        printf("\"%s\" : \"%s\"\n", displayProperty, ism_common_getStringProperty(props, displayProperty));
    }
    printf("}\n");
}

//Full Barrier, read or writes to memory can not the reordered (by compiler or CPU)
//across this barrier
#ifdef __x86_64__
#define ismEngine_FullMemoryBarrier() asm volatile("mfence":::"memory")
#elif __PPC64__
#define ismEngine_FullMemoryBarrier() __sync_synchronize()
#else
#define ismEngine_FullMemoryBarrier() abort() //No support for other arch
#endif
//Read Barrier: reads (loads) from memory can not be reordered (by compiler or CPU)
//across this barrier
#ifdef __x86_64__
#define ismEngine_ReadMemoryBarrier() asm volatile("lfence":::"memory")
#elif __PPC64__
#define ismEngine_ReadMemoryBarrier() __sync_synchronize()
#else
#define ismEngine_ReadMemoryBarrier() abort() //No support for other arch
#endif

//Write Barrier: Stores to memory can not be reordered (by compiler or CPU)
//across this barrier
#ifdef __x86_64__
#define ismEngine_WriteMemoryBarrier() asm volatile("sfence":::"memory")
#elif __PPC64__
#define ismEngine_WriteMemoryBarrier() __sync_synchronize()
#else
#define ismEngine_WriteMemoryBarrier() abort() //No support for other arch
#endif

#define ismENGINE_CFGPROP_INITIAL_SUBLISTCACHE_CAPACITY "Engine.SublistCacheCapacity"
#define ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY 1000   ///< Default initial capacity of the per IOP thread topic tree subscriber list cache

#define ismENGINE_CFGPROP_DISABLE_AUTO_QUEUE_CREATION   "Engine.DisableAutoQueueCreation"
#define ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION   true  ///< Whether the automatic creation of unknown named queues is disabled by default

#define ismENGINE_CFGPROP_USE_RECOVERY_METHOD            "Engine.UseRecoveryMethod"
#define ismENGINE_VALUE_USE_CLASSIC_RECOVERY             0
#define ismENGINE_VALUE_USE_NEW_OWNER_AND_REF_RECOVERY   1
#define ismENGINE_VALUE_USE_FULL_NEW_RECOVERY            2
#define ismENGINE_DEFAULT_USE_RECOVERY_METHOD            ismENGINE_VALUE_USE_NEW_OWNER_AND_REF_RECOVERY

#define ismENGINE_CFGPROP_FAKE_ASYNC_CALLBACK_CAPACITY   "Engine.FakeAsyncCallbackCapacity"
#define ismENGINE_DEFAULT_FAKE_ASYNC_CALLBACK_CAPACITY   64000

#define ismENGINE_CFGPROP_USE_SPIN_LOCKS                "UseSpinLocks"
#define ismENGINE_DEFAULT_USE_SPIN_LOCKS                false

#define ismENGINE_DEFAULT_CLUSTER_ENABLED               false

#define ismENGINE_CFGPROP_CLEAN_SHUTDOWN                "Engine.CleanShutdown"
#define ismENGINE_DEFAULT_CLEAN_SHUTDOWN                false  ///< Whether we should try to tidily free() everything (e.g. for valgrind) at the end or just exit.

#define ismENGINE_CFGPROP_SERVER_TIMESTAMP_INTERVAL     "Engine.ServerTimestampInterval"
#define ismENGINE_DEFAULT_SERVER_TIMESTAMP_INTERVAL      30 ///< Interval in seconds between server timestamp updates

#define ismENGINE_CFGPROP_RETAINED_MIN_ACTIVE_ORDERID_INTERVAL  "Engine.RetMinActiveOrderIdInterval"
#define ismENGINE_DEFAULT_RETAINED_MIN_ACTIVE_ORDERID_INTERVAL  30 ///< Interval in seconds between retained minimum active orderid scans

#define ismENGINE_CFGPROP_RETAINED_MIN_ACTIVE_ORDERID_MAX_SCANS  "Engine.RetMinActiveOrderIdMaxScans"
#define ismENGINE_DEFAULT_RETAINED_MIN_ACTIVE_ORDERID_MAX_SCANS  1 ///< Allow up to this many scans upon async completion of minimum active orderid scan

#define ismENGINE_CFGPROP_RETAINED_REPOSITIONING_ENABLED        "Engine.RetainedRepositioningEnabled"
#define ismENGINE_DEFAULT_RETAINED_REPOSITIONING_ENABLED        true ///< Whether we should reposition retained messages after an orderid scan

#define ismENGINE_CFGPROP_CLUSTER_RETAINED_SYNC_INTERVAL        "Engine.ClusterRetainedSyncInterval"
#define ismENGINE_DEFAULT_CLUSTER_RETAINED_SYNC_INTERVAL        30 ///< Interval in seconds between remote server retained synchronization scans

#define ismENGINE_CFGPROP_CLUSTER_RETAINED_FORWARDING_DELAY   "Engine.ClusterRetainedForwardingDelay"
#define ismENGINE_DEFAULT_CLUSTER_RETAINED_FORWARDING_DELAY   1800 ///< Number of seconds to delay before requesting retained msg forwarding

#define ismENGINE_CFGPROP_ASYNC_CALLBACK_QUEUE_ALERT_MAX_PAUSE  "Engine.AsyncCallbackQueueAlertMaxPause"
#define ismENGINE_DEFAULT_ASYNC_CALLBACK_QUEUE_ALERT_MAX_PAUSE  240 ///< Maximum number of seconds to allow engine threads to be paused on ASYNCCBQ alert

#define ismENGINE_CFGPROP_TRANSACTION_THREADJOBS_CLIENT_MINIMUM  "Engine.TransactionThreadJobsClientMinimum"
#define ismENGINE_DEFAULT_TRANSACTION_THREADJOBS_CLIENT_MINIMUM  1000 ///< Minimum active clients connected before work is scheduled on per-thread job queues

//Limit of the number of unacked MQTT messages allowed per client...
#define ismENGINE_CFGPROP_MAX_MQTT_UNACKED_MESSAGES     "mqttMsgIdRange"
#define ismENGINE_DEFAULT_MAX_MQTT_UNACKED_MESSAGES     128

#define ismENGINE_CFGPROP_MULTICONSUMER_MESSAGE_BATCH   "Engine.MultiConsumerMessageBatchSize"
#define ismENGINE_DEFAULT_MULTICONSUMER_MESSAGE_BATCH   3  ///< Deliver (at most) this many messages to a consumer before round-robining to next one

#define ismENGINE_CFGPROP_FREE_MEM_RESERVED_MB  "Engine.FreeMemReservedMB" ///< On a small machine, ignore ("reserve") this much free memory for non MemManager use
#define ismENGINE_DEFAULT_FREE_MEM_RESERVED_MB  300                        ///< On a small machine, ignore ("reserve") this much free memory for non MemManager use

#define ismENGINE_CFGPROP_FREE_MEM_RESERVED_THRESHOLD_MB "Engine.FreeMemReservedThresholdMB" ///< If a machine has less memory than this we reserve (see above) some mem that we don't count towards free mem
#define ismENGINE_DEFAULT_FREE_MEM_RESERVED_THRESHOLD_MB 102400                              ///< If a machine has less memory than this we reserve (see above) some mem that we don't count towards free mem

#define ismENGINE_CFGPROP_DESTROY_NONACKER_RATIO "Server.DestroyNonAckerRatio" ///< If a message is not acked, the if the number of messages that are put to the queue is this times the maxdepth of the
                                                                              ///< We'll delete the client state for the client
#define ismENGINE_DEFAULT_DESTROY_NONACKER_RATIO 80                           ///< By default, message has to be 80x the maxdepth behind newest message to trigger deletion

#define ismENGINE_CFGPROP_TOLERATERECOVERYINCONSISTENCIES "Server.TolerateRecoveryInconsistencies" ///< If we find problems during recovery do we stop (false=default) or try and recover partial data (=true)
#define ismENGINE_DEFAULT_TOLERATERECOVERYINCONSISTENCIES false                                    ///< By default, stop and wait for service/clean store.

// Property for ism_transport_disableClientSet function pointer
#define ismENGINE_CFGPROP_DISABLECLIENTSET_FNPTR "_ism_transport_disableClientSet_fnptr"
#define ismENGINE_DEFAULT_DISABLECLIENTSET_FNPTR 0L

// Property for ism_transport_enableClientSet function pointer
#define ismENGINE_CFGPROP_ENABLECLIENTSET_FNPTR "_ism_transport_enableClientSet_fnptr"
#define ismENGINE_DEFAULT_ENABLECLIENTSET_FNPTR 0L

#define ismENGINE_CFGPROP_MAX_CONCURRENT_IMPORTEXPORT_REQUESTS "Engine.MaxConcurrentImportExportRequests"
#define ismENGINE_DEFAULT_MAX_CONCURRENT_IMPORTEXPORT_REQUESTS 64  ///< Default maximum concurrent import/export API requests

#define ismENGINE_CFGPROP_DISABLE_THREAD_JOB_QUEUES   "Engine.DisableThreadJobQueues"
#define ismENGINE_DEFAULT_DISABLE_THREAD_JOB_QUEUES   false  ///< Whether per-thread job queues are disabled

#define ismENGINE_CFGPROP_THREAD_JOB_QUEUES_ALGORITHM   "Engine.ThreadJobQueueAlgorithm"
#define ismENGINE_THREAD_JOB_QUEUES_ALGORITHM_LIMITED    1  ///< 1= simple scavenger, disabled below Engine.TransactionThreadJobsClientMinimum
#define ismENGINE_THREAD_JOB_QUEUES_ALGORITHM_EXTRA      2  ///< 2= More aggressive scavenger, backs off if interferes. Checkwaiters uses job queues
#define ismENGINE_DEFAULT_THREAD_JOB_QUEUES_ALGORITHM    ismENGINE_THREAD_JOB_QUEUES_ALGORITHM_EXTRA

// Property to specify the default clientId reg ex to use if no ResourceSetDescriptor is defined
#define ismENGINE_CFGPROP_RESOURCESETDESCRIPTOR_DEFAULT_CLIENTID "Server.DefaultResourceSetDescriptorClientID"
#define ismENGINE_DEFAULT_RESOURCESETDESCRIPTOR_DEFAULT_CLIENTID NULL

// Property to specify the default topic reg exp to use if no ResourceSetDescriptor is defined
#define ismENGINE_CFGPROP_RESOURCESETDESCRIPTOR_DEFAULT_TOPIC "Server.DefaultResourceSetDescriptorTopic"
#define ismENGINE_DEFAULT_RESOURCESETDESCRIPTOR_DEFAULT_TOPIC NULL

// Property to specify (in MEMDEBUG builds) a SetId for which memory allocs & frees are traced
#define ismENGINE_CFGPROP_RESOURCESET_STATS_MEMTRACE_SETID "Server.ResourceSetStatsMemTraceSetId"

// Property to specify whether to gather unmatched resource set stats (if gathering is enabled)
#define ismENGINE_CFGPROP_RESOURCESET_STATS_TRACK_UNMATCHED "Server.ResourceSetStatsTrackUnmatched"
#define ismENGINE_DEFAULT_RESOURCESET_STATS_TRACK_UNMATCHED true

// Properties that specify weekly resourceSetStats reporting
#define ismENGINE_CFGPROP_RESOURCESET_STATS_WEEKLY_REPORT_STATTYPE "Server.ResourceSetStatsWeeklyReportStatType"
#define ismENGINE_DEFAULT_RESOURCESET_STATS_WEEKLY_REPORT_STATTYPE ismENGINE_MONITOR_STATTYPE_NONE
#define ismENGINE_IOTDFLT_RESOURCESET_STATS_WEEKLY_REPORT_STATTYPE ismENGINE_MONITOR_STATTYPE_ALLUNSORTED

#define ismENGINE_CFGPROP_RESOURCESET_STATS_WEEKLY_REPORT_DAYOFWEEK "Server.ResourceSetStatsWeeklyReportDayOfWeek"
#define ismENGINE_DEFAULT_RESOURCESET_STATS_WEEKLY_REPORT_DAYOFWEEK 0 ///< Sunday
#define ismENGINE_IOTDFLT_RESOURCESET_STATS_WEEKLY_REPORT_DAYOFWEEK 0 ///< Sunday

#define ismENGINE_CFGPROP_RESOURCESET_STATS_WEEKLY_REPORT_HOUROFDAY "Server.ResourceSetStatsWeeklyReportHourOfDay"
#define ismENGINE_DEFAULT_RESOURCESET_STATS_WEEKLY_REPORT_HOUROFDAY 0 ///< Midnight
#define ismENGINE_IOTDFLT_RESOURCESET_STATS_WEEKLY_REPORT_HOUROFDAY 0 ///< Midnight

#define ismENGINE_CFGPROP_RESOURCESET_STATS_WEEKLY_REPORT_MAXRESULTS "Server.ResourceSetStatsWeeklyReportMaxResults"
#define ismENGINE_DEFAULT_RESOURCESET_STATS_WEEKLY_REPORT_MAXRESULTS 100
#define ismENGINE_IOTDFLT_RESOURCESET_STATS_WEEKLY_REPORT_MAXRESULTS 100

#define ismENGINE_CFGPROP_RESOURCESET_STATS_WEEKLY_REPORT_RESETSTATS "Server.ResourceSetStatsWeeklyReportResetStats"
#define ismENGINE_DEFAULT_RESOURCESET_STATS_WEEKLY_REPORT_RESETSTATS false
#define ismENGINE_IOTDFLT_RESOURCESET_STATS_WEEKLY_REPORT_RESETSTATS false

// Properties that specify daily resourceSetStats reporting
#define ismENGINE_CFGPROP_RESOURCESET_STATS_DAILY_REPORT_STATTYPE "Server.ResourceSetStatsDailyReportStatType"
#define ismENGINE_DEFAULT_RESOURCESET_STATS_DAILY_REPORT_STATTYPE ismENGINE_MONITOR_STATTYPE_NONE
#define ismENGINE_IOTDFLT_RESOURCESET_STATS_DAILY_REPORT_STATTYPE ismENGINE_MONITOR_STATTYPE_TOTALMEMORYBYTES_HIGHEST

#define ismENGINE_CFGPROP_RESOURCESET_STATS_DAILY_REPORT_HOUROFDAY "Server.ResourceSetStatsDailyReportHourOfDay"
#define ismENGINE_DEFAULT_RESOURCESET_STATS_DAILY_REPORT_HOUROFDAY 0 ///< Midnight
#define ismENGINE_IOTDFLT_RESOURCESET_STATS_DAILY_REPORT_HOUROFDAY 0 ///< Midnight

#define ismENGINE_CFGPROP_RESOURCESET_STATS_DAILY_REPORT_MAXRESULTS "Server.ResourceSetStatsDailyReportMaxResults"
#define ismENGINE_DEFAULT_RESOURCESET_STATS_DAILY_REPORT_MAXRESULTS 50
#define ismENGINE_IOTDFLT_RESOURCESET_STATS_DAILY_REPORT_MAXRESULTS 50

#define ismENGINE_CFGPROP_RESOURCESET_STATS_DAILY_REPORT_RESETSTATS "Server.ResourceSetStatsDailyReportResetStats"
#define ismENGINE_DEFAULT_RESOURCESET_STATS_DAILY_REPORT_RESETSTATS false
#define ismENGINE_IOTDFLT_RESOURCESET_STATS_DAILY_REPORT_RESETSTATS false

// Properties that specify hourly resourceSetStats reporting
#define ismENGINE_CFGPROP_RESOURCESET_STATS_HOURLY_REPORT_STATTYPE "Server.ResourceSetStatsHourlyReportStatType"
#define ismENGINE_DEFAULT_RESOURCESET_STATS_HOURLY_REPORT_STATTYPE ismENGINE_MONITOR_STATTYPE_NONE
#define ismENGINE_IOTDFLT_RESOURCESET_STATS_HOURLY_REPORT_STATTYPE ismENGINE_MONITOR_STATTYPE_TOTALMEMORYBYTES_HIGHEST

#define ismENGINE_CFGPROP_RESOURCESET_STATS_HOURLY_REPORT_MAXRESULTS "Server.ResourceSetStatsHourlyReportMaxResults"
#define ismENGINE_DEFAULT_RESOURCESET_STATS_HOURLY_REPORT_MAXRESULTS 5
#define ismENGINE_IOTDFLT_RESOURCESET_STATS_HOURLY_REPORT_MAXRESULTS 5

#define ismENGINE_CFGPROP_RESOURCESET_STATS_HOURLY_REPORT_RESETSTATS "Server.ResourceSetStatsHourlyReportResetStats"
#define ismENGINE_DEFAULT_RESOURCESET_STATS_HOURLY_REPORT_RESETSTATS false
#define ismENGINE_IOTDFLT_RESOURCESET_STATS_HOURLY_REPORT_RESETSTATS false

// Property that specifies the minutes past the hour on which the various stats should be reported
#define ismENGINE_CFGPROP_RESOURCESET_STATS_MINUTES_PAST_HOUR "Server.ResourceSetStatsReportMinutesPastHour"
#define ismENGINE_DEFAULT_RESOURCESET_STATS_MINUTES_PAST_HOUR iereMINUTES_PAST_UNSPECIFIED
#define ismENGINE_IOTDFLT_RESOURCESET_STATS_MINUTES_PAST_HOUR iereMINUTES_PAST_UNSPECIFIED

// Property that specifies whether to allow the proxy protocol (which is what determines that this is IoT)
#define ismENGINE_CFGPROP_ENVIRONMENT_IS_IOT "Protocol.AllowMqttProxyProtocol" ///< Protocol property indicating whether to allow proxy protocol indicates if we're running in IoT
#define ismENGINE_DEFAULT_ENVIRONMENT_IS_IOT false                             ///< By default assume that proxy protocol is not enabled

// Property that specifies whether to use less memory during engine recovery (the trade off generally being longer recovery times)
#define ismENGINE_CFGPROP_REDUCED_MEMORY_RECOVERY_MODE "Server.ReducedMemoryRecoveryMode"
#define ismENGINE_DEFAULT_REDUCED_MEMORY_RECOVERY_MODE false

// Property that specifies a size profile to use (which alters the amount of various things we allocate & do)
#define ismENGINE_CFGPROP_SIZE_PROFILE       "SizeProfile"

// Properties and values that will come from the Admin component
#define ismENGINE_ADMIN_PROPERTY_NAME                            "Name"
#define ismENGINE_ADMIN_PROPERTY_MAXMESSAGES                     "MaxMessages"                     ///< Maximum messages on a queue (i.e. depth)
#define ismENGINE_ADMIN_PROPERTY_DISCARDMESSAGES                 "DiscardMessages"                 ///< Whether to discard messages when deleting a queue
#define ismENGINE_ADMIN_PROPERTY_CONCURRENTCONSUMERS             "ConcurrentConsumers"             ///< Whether concurrent consumers allowed
#define ismENGINE_ADMIN_PROPERTY_ALLOWSEND                       "AllowSend"                       ///< Whether send (put) of messages is allowed
#define ismENGINE_ADMIN_PROPERTY_DISCONNECTEDCLIENTNOTIFICATION  "DisconnectedClientNotification"  ///< Whether disconnected client notification is needed
#define ismENGINE_ADMIN_PROPERTY_CLIENTID                        "ClientId"                        ///< The client Id specified when deleting a durable subscription
#define ismENGINE_ADMIN_PROPERTY_MAXMESSAGETIMETOLIVE            "MaxMessageTimeToLive"            ///< Maximum time-to-live (expiry) to allow on publish requests
#define ismENGINE_ADMIN_PROPERTY_DEFAULTSELECTIONRULE            "DefaultSelectionRule"            ///< Default selection string to apply if none is supplied by a subscription

#define ismENGINE_ADMIN_PROPERTY_DISCARDSHARERS                  "DiscardSharers"                  ///< Whether the policy is to only discard other sharers when deleting admin subscription
#define ismENGINE_DEFAULT_PROPERTY_DISCARDSHARERS                false                             ///< What the default policy is when deleting admin subscription

#define ismENGINE_ADMIN_VALUE_TOPICPOLICY               "TopicPolicy"
#define ismENGINE_ADMIN_VALUE_QUEUEPOLICY               "QueuePolicy"
#define ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY        "SubscriptionPolicy"
#define ismENGINE_ADMIN_VALUE_QUEUE                     "Queue"
#define ismENGINE_ADMIN_VALUE_TOPICMONITOR              "TopicMonitor"
#define ismENGINE_ADMIN_VALUE_MQTTCLIENT                "MQTTClient"
#define ismENGINE_ADMIN_VALUE_SUBSCRIPTION              "Subscription"
#define ismENGINE_ADMIN_VALUE_RESOURCESETDESCRIPTOR     "ResourceSetDescriptor"
#define ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION         "AdminSubscription"
#define ismENGINE_ADMIN_VALUE_DURABLENAMESPACEADMINSUB  "DurableNamespaceAdminSub"
#define ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB     "NonpersistentAdminSub"
#define ismENGINE_ADMIN_VALUE_CLUSTERREQUESTEDTOPICS    "ClusterRequestedTopics"

// Max Message Behavior property and values
#define ismENGINE_ADMIN_PROPERTY_MAXMESSAGESBEHAVIOR  "MaxMessagesBehavior"   ///< Behavior to use when maximum messages buffered on a subscription
#define ismENGINE_ADMIN_VALUE_REJECTNEWMESSAGES       "RejectNewMessages"     ///< Reject new messages
#define ismENGINE_ADMIN_VALUE_DISCARDOLDMESSAGES      "DiscardOldMessages"    ///< Discard the oldest (first) messages available to the engine

// This must be at least as long as the longest property we want to query
#define ismENGINE_MAX_ADMIN_PROPERTY_LENGTH  50

#define ismENGINE_ADMIN_PREFIX_QUEUE_NAME                          "Queue.Name."                         ///< Prefix used to identify a queue name in config properties
#define ismENGINE_ADMIN_PREFIX_QUEUE_CONCURRENTCONSUMERS           "Queue.ConcurrentConsumers."          ///< Prefix used to identify the ConcurrentConsumers option in config properties
#define ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING            "TopicMonitor.TopicString."           ///< Prefix used to identify a topic monitor topic string in config properties
#define ismENGINE_ADMIN_PREFIX_TOPICPOLICY_NAME                    "TopicPolicy.Name."                   ///< Prefix used to identify a topic messaging policy's name
#define ismENGINE_ADMIN_PREFIX_QUEUEPOLICY_NAME                    "QueuePolicy.Name."                   ///< Prefix used to identify a queue messaging policy's name
#define ismENGINE_ADMIN_PREFIX_SUBSCRIPTIONPOLICY_NAME             "SubscriptionPolicy.Name."            ///< Prefix used to identify a subscription messaging policy's name
#define ismENGINE_ADMIN_PREFIX_TOPICPOLICY_CONTEXT                 "TopicPolicy.Context."                ///< Prefix used to identify a topic messaging policy's context (iepiPolicyInfo_t)
#define ismENGINE_ADMIN_PREFIX_QUEUEPOLICY_CONTEXT                 "QueuePolicy.Context."                ///< Prefix used to identify a queue messaging policy's context (iepiPolicyInfo_t)
#define ismENGINE_ADMIN_PREFIX_SUBSCRIPTIONPOLICY_CONTEXT          "SubscriptionPolicy.Context."         ///< Prefix used to identify a subscription messaging policy's context (iepiPolicyInfo_t)
#define ismENGINE_ADMIN_PREFIX_RESOURCESETDESCRIPTOR_CLIENTID      "ResourceSetDescriptor.ClientID."     ///< Prefix used to identify the resourceSetDescriptor clientID in place at startup
#define ismENGINE_ADMIN_PREFIX_RESOURCESETDESCRIPTOR_TOPIC         "ResourceSetDescriptor.Topic."        ///< Prefix used to identify the resourceSetDescriptor topic in place at startup
#define ismENGINE_ADMIN_PREFIX_CLUSTERREQUESTEDTOPICS_TOPICSTRING  "ClusterRequestedTopics.TopicString." ///< Prefix used to identify a cluster requested topic ('proxy subscription') topic string

///@brief Prefix used to identify a subscription's client id in config properties (NOTE: Traling '.')
#define ismENGINE_ADMIN_SUBSCRIPTION_CLIENTID                  "ClientID"
///@brief Prefix used to identify a subscription's client id in config properties (NOTE: Traling '.')
#define ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_CLIENTID           "Subscription."ismENGINE_ADMIN_SUBSCRIPTION_CLIENTID"."
///@brief Prefix used to identify a subscription's name in config properties (NOTE: Trailing '.')
#define ismENGINE_ADMIN_SUBSCRIPTION_SUBSCRIPTIONNAME          "SubscriptionName"
///@brief Prefix used to identify a subscription's name in config properties (NOTE: Trailing '.')
#define ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_SUBSCRIPTIONNAME   "Subscription."ismENGINE_ADMIN_SUBSCRIPTION_SUBSCRIPTIONNAME"."

#define ismENGINE_ADMIN_ALLADMINSUBS_SUBOPTIONS                     "SubOptions"
#define ismENGINE_ADMIN_ALLADMINSUBS_TOPICFILTER                    "TopicFilter"
#define ismENGINE_ADMIN_ALLADMINSUBS_NAMESPACE                      "Namespace"
#define ismENGINE_ADMIN_ALLADMINSUBS_MAXQUALITYOFSERVICE            "MaxQualityOfService"
#define ismENGINE_ADMIN_ALLADMINSUBS_ADDRETAINEDMSGS                "AddRetainedMsgs"
#define ismENGINE_ADMIN_ALLADMINSUBS_QUALITYOFSERVICEFILTER         "QualityOfServiceFilter"
#define ismENGINE_ADMIN_VALUE_QUALITYOFSERVICEFILTER_NONE           "None"
#define ismENGINE_ADMIN_VALUE_QUALITYOFSERVICEFILTER_UNRELIABLEONLY "QoS=0"
#define ismENGINE_ADMIN_VALUE_QUALITYOFSERVICEFILTER_RELIABLEONLY   "QoS>0"

/// @brief Prefixes used to identify properties for all admin subs (NOTE: Trailing '.'s)
#define ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_TOPICPOLICY            ismENGINE_ADMIN_VALUE_TOPICPOLICY"."
#define ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_SUBSCRIPTIONPOLICY     ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY"."
#define ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_SUBOPTIONS             ismENGINE_ADMIN_ALLADMINSUBS_SUBOPTIONS"."
#define ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_TOPICFILTER            ismENGINE_ADMIN_ALLADMINSUBS_TOPICFILTER"."
#define ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_MAXQUALITYOFSERVICE    ismENGINE_ADMIN_ALLADMINSUBS_MAXQUALITYOFSERVICE"."
#define ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_ADDRETAINEDMSGS        ismENGINE_ADMIN_ALLADMINSUBS_ADDRETAINEDMSGS"."
#define ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_QUALITYOFSERVICEFILTER ismENGINE_ADMIN_ALLADMINSUBS_QUALITYOFSERVICEFILTER"."
#define ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_NAME                   ismENGINE_ADMIN_PROPERTY_NAME"."

#define ismENGINE_ADMIN_QUEUE_PROPERTY_FORMAT                  "Queue.%s."                   ///< The beginning of the format for a general queue property name
#define ismENGINE_ADMIN_TOPICPOLICY_PROPERTY_FORMAT            "TopicPolicy.%s."             ///< The beginning of the format for a topic messaging policy property name
#define ismENGINE_ADMIN_QUEUEPOLICY_PROPERTY_FORMAT            "QueuePolicy.%s."             ///< The beginning of the format for a queue messaging policy property name
#define ismENGINE_ADMIN_SUBSCRIPTIONPOLICY_PROPERTY_FORMAT     "SubscriptionPolicy.%s."      ///< The beginning of the format for a subscription messaging policy property name

#define ismENGINE_UPDATE_SUBSCRIPTION_PROPERTY_SUBID           "SubId"                       ///< Property name to use when updating a subscription's subId
#define ismENGINE_UPDATE_SUBSCRIPTION_PROPERTY_SUBOPTIONS      "SubOptions"                  ///< Property name to use when updating a subscription's subOptions

/// The string form of the system topic prefix
#define ismENGINE_SYSTOPIC_PREFIX "$"

/// The string form of the $SYS prefix
#define ismENGINE_DOLLARSYS_PREFIX ismENGINE_SYSTOPIC_PREFIX"SYS"

/// The expiry time we give retained messages when the server is a member of the cluster in seconds
/// (when not in a cluster we make the message expire immediately)
#define ismENGINE_CLUSTER_RETAINED_EXPIRY_INTERVAL (7*24*60*60)

/*********************************************************************/
/* Useful macros                                                     */
/*********************************************************************/
#define RoundUp4(val) (((val)+3)&~3L)
#define RoundUp8(val) (((val)+7)&~7L)
#define RoundUp16(val) (((val)+15)&~15L)

/*********************************************************************/
/*                                                                   */
/* GLOBAL EXTERNAL VARIABLES                                         */
/*                                                                   */
/*********************************************************************/
extern ismEngine_Server_t ismEngine_serverGlobal;
extern __thread ieutThreadData_t *ismEngine_threadData;
extern int32_t (*ismEngine_security_validate_policy_func)(ismSecurity_t *,
                                                          ismSecurityAuthObjectType_t,
                                                          const char *,
                                                          ismSecurityAuthActionType_t,
                                                          ism_ConfigComponentType_t,
                                                          void **);
extern int32_t (*ismEngine_security_set_policyContext_func)(const char *,
                                                            ismSecurityPolicyType_t,
                                                            ism_ConfigComponentType_t,
                                                            void *);

// Set the engine's current runPhase
#define ieut_setEngineRunPhase(__runPhase) __sync_lock_test_and_set(&ismEngine_serverGlobal.runPhase, (__runPhase))

// Return a pointer to the thread data
#define ieut_getThreadData() ismEngine_threadData

//Time in arbitary units since an arbitrary point (usually power on)
#ifdef __x86_64__
#define ism_engine_fastTime(low,high)  __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))
#elif __PPC64__
#define ism_engine_fastTime(low,high) {uint64_t ppc64_timebase =  __builtin_ppc_get_timebase(); low = ppc64_timebase & 0xFFFFFF; high = ppc64_timebase >>32;}
#endif
static inline uint64_t ism_engine_fastTimeUInt64(void)
{
#ifdef __x86_64__
    typedef union tag_rdtsc_t
    {
        uint64_t ui64;
        uint32_t ui32[2];
    } rdtsc_t;

    rdtsc_t data;

    ism_engine_fastTime(data.ui32[0], data.ui32[1]);

    return data.ui64;
#elif __PPC64__
    uint64_t ppc64_timebase =  __builtin_ppc_get_timebase();
    return ppc64_timebase;
#endif
}

// The current definition of the values that are put into the ID_ServerTime property for retained messages
#define ism_engine_retainedServerTime()  ism_common_currentTimeNanos()

#if (ieutTRACEHISTORY_BUFFERSIZE > 0)
//TRACEFILE_IDENT should be defined via gcc command line (calculate by common.mk)
#ifndef TRACEFILE_IDENT
#define TRACEFILE_IDENT 0x0
#endif

#define ieutTRACE_HISTORYBUF(_threadData, bufval)  \
                ieutTRACE_HISTORYBUF_IDENT(_threadData, (((uint64_t)(TRACEFILE_IDENT)) << 32) | __LINE__, bufval)

//Mainly called from ieutTRACE_HISTORYBUF() but occasionally used to override the ident e.g. used in header files
//where the ident confusingly mixes the line number in the h file with the ident from the c file
#define ieutTRACE_HISTORYBUF_IDENT(_threadData, ident, bufval)                                                         \
        (_threadData)->traceHistoryIdent[(_threadData)->traceHistoryBufPos] =   ident;                                 \
        (_threadData)->traceHistoryValue[(_threadData)->traceHistoryBufPos] = (uint64_t)bufval;                        \
        (_threadData)->traceHistoryBufPos  =   (((_threadData)->traceHistoryBufPos +1) %  ieutTRACEHISTORY_BUFFERSIZE)

//function in header files require the ident set explicitly
#define ENGINEINTERNAL_H_TRACEIDENT 0x3b15ea9e
#else
#define ieutTRACE_HISTORYBUF(_threaddata, bufval)
#define ieutTRACE_HISTORYBUF_IDENT(_threadData, ident, bufval)
#endif

#define ieutTRACEL(_threadData, bufval, _level, _fmt...)                                                             \
    ieutTRACE_HISTORYBUF(_threadData, bufval);                                                                          \
    if (UNLIKELY((_level) <= (_threadData)->componentTrcLevel)) traceFunction((_level), 0, __FILE__, __LINE__, _fmt)


//Some engine functions can record times of start/stop in fast trace if requested
#ifdef ieutEXTRADIAGNOSTICS
#define TRACETIMESTAMP_LOCKWAITER     1
#define TRACETIMESTAMP_CHECKWAITERS   1
#define TRACETIMESTAMP_STORECOMMITS   1
#define TRACETIMESTAMP_PUTMESSAGE     1
#define TRACETIMESTAMP_PUTLOCK        1
#define TRACETIMESTAMP_MOVETONEWPAGE  1
#define TRACETIMESTAMP_RECLAIM        1
#define TRACETIMESTAMP_EXPIRY         1
#else
#define TRACETIMESTAMP_LOCKWAITER     0
#define TRACETIMESTAMP_CHECKWAITERS   0
#define TRACETIMESTAMP_STORECOMMITS   0
#define TRACETIMESTAMP_PUTMESSAGE     0
#define TRACETIMESTAMP_PUTLOCK        0
#define TRACETIMESTAMP_MOVETONEWPAGE  0
#define TRACETIMESTAMP_RECLAIM        0
#define TRACETIMESTAMP_EXPIRY         0
#endif



// Selectively require some FFDCs to be fatal during the restart / recovery process
#define ismENGINE_USE_FATAL_FFDC_DURING_RECOVERY true


//****************************************************************************
// @brief  At the beginning of any engine call, this function is called to
//         ensure that the thread is initialized. This includes setting the
//         trace level and memory update count.
//
// @param[in]     pClient       The client state to use
//
// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static inline ieutThreadData_t *ieut_enteringEngine(ismEngine_ClientState_t *pClient)
{
    ieutThreadData_t *pThreadData = ismEngine_threadData;

    if (pThreadData->callDepth == 0)
    {
        pThreadData->callDepth = 1;
        pThreadData->entryCount++;

        pThreadData->componentTrcLevel = ism_security_context_getTrcLevel(pClient ? pClient->pSecContext : NULL)->trcComponentLevels[TRACECOMP_XSTR(TRACE_COMP)];
        pThreadData->memUpdateCount = ismEngine_serverGlobal.memUpdateCount;

        #if (ieutTRACEHISTORY_BUFFERSIZE > 0)
        uint64_t entryTime = ism_engine_fastTimeUInt64();
        ieutTRACE_HISTORYBUF_IDENT( pThreadData
                                  , (((uint64_t)(ENGINEINTERNAL_H_TRACEIDENT)) << 32) | __LINE__
                                  , entryTime);
        #endif

        // Any jobs for us to process?
        if (pThreadData->jobQueue != NULL)
        {
            bool processedJobs = ieut_processJobQueue(pThreadData);

            if (processedJobs)
            {
                uint32_t storeOpsCount = 0;
                int32_t rc = ism_store_getStreamOpsCount(pThreadData->hStream, &storeOpsCount);

                if ( rc == OK && storeOpsCount != 0)
                {
                    ieutTRACE_FFDC(ieutPROBE_001, true,
                                   "unfinished store transaction after processing jobs on engine entry", rc,
                                   "storeOpsCount", &storeOpsCount, sizeof(storeOpsCount),
                                   NULL);
                }

                pThreadData->processedJobs += 1;
            }
        }
    }
    else
    {
        pThreadData->callDepth += 1;
    }

    return pThreadData;
}

//****************************************************************************
/// @brief Re-initalize the memory update count as though entering the engine
///
/// @remark This is used for threads that stay 'inside' the engine, in the sense
/// that they don't actually call ieut_enteringEngine and ieut_leavingEngine, but
/// which know that they cannot be referring to engine resources (other than
/// their own).
///
/// @remark Even engine threads should generally use ieut_enteringEngine over this
/// function which is primarily intended for use by the threadJobs scavenger thread
/// since it shouldn't do other things (like run thread jobs, for itself at this
/// time).
///
/// @param[in]     pThreadData      Thread data to use
///
/// @see ieut_enteringEngine
//****************************************************************************
static inline void ieut_virtuallyEnterEngine(ieutThreadData_t *pThreadData)
{
    assert(pThreadData->callDepth == 1);

    pThreadData->memUpdateCount = ismEngine_serverGlobal.memUpdateCount;
}

// Forward declaration of function to flush resource set thread cache
void iere_flushResourceSetThreadCache(ieutThreadData_t *pThreadData);

//****************************************************************************
// @brief  At the end of any engine call, this function is called to leave the
//         thread data in a clean state.
//
// @param[in]     pThreadData       Thread data structure
//****************************************************************************
static inline void ieut_leavingEngine(ieutThreadData_t *pThreadData)
{
    assert(pThreadData->callDepth != 0);

    pThreadData->callDepth -= 1;

    if (pThreadData->callDepth == 0)
    {
        #if (ieutTRACEHISTORY_BUFFERSIZE > 0)
        uint64_t exitTime = ism_engine_fastTimeUInt64();
        ieutTRACE_HISTORYBUF_IDENT( pThreadData
                                  ,  (((uint64_t)(ENGINEINTERNAL_H_TRACEIDENT)) << 32) | __LINE__
                                  , exitTime);
        #endif

        iere_flushResourceSetThreadCache(pThreadData);

        pThreadData->memUpdateCount = 0;

        if (pThreadData->hStream != ismSTORE_NULL_HANDLE)
        {
            // Verify that no store operations are left on the way out of the engine, if
            // there are we shut down to catch the culprit.
            uint32_t storeOpsCount = 0;
            int32_t rc = ism_store_getStreamOpsCount(pThreadData->hStream, &storeOpsCount);

            if ( rc == OK && storeOpsCount != 0)
            {
                ieutTRACE_FFDC(ieutPROBE_001, true,
                               "unfinished store transaction on engine exit", rc,
                               "storeOpsCount", &storeOpsCount, sizeof(storeOpsCount),
                               NULL);
            }
        }
    }
}

//****************************************************************************
/// @brief Leave the thread data in a clean state, but without leaving the engine.
///
/// @remark This is used for threads that stay 'inside' the engine, but which at
/// certain points know they are no longer referring to any state that might
/// be waiting for a deferred free.
///
/// @remark Even engine threads should generally use ieut_leavingEngine over this
/// function which is primarily intended for use by the threadJobs scavenger thread
/// which should just be indicating that it is clear of memUpdates.
///
/// @param[in]     pThreadData      Thread data to use
///
/// @see ieut_leavingEngine
//****************************************************************************
static inline void ieut_virtuallyLeaveEngine(ieutThreadData_t *pThreadData)
{
    assert(pThreadData->callDepth == 1);
    iere_flushResourceSetThreadCache(pThreadData);
    pThreadData->memUpdateCount = 0;
}

#define ieut_initializeThreadTraceLvl(_threadData) (_threadData)->componentTrcLevel = ism_security_context_getTrcLevel(NULL)->trcComponentLevels[TRACECOMP_XSTR(TRACE_COMP)]


#endif /* __ISM_ENGINEINTERNAL_DEFINED */

/*********************************************************************/
/* End of engineInternal.h                                           */
/*********************************************************************/
