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
/// @file  engine.h
/// @brief Engine component external header file
//****************************************************************************
#ifndef __ISM_ENGINE_DEFINED
#define __ISM_ENGINE_DEFINED
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ismutil.h>
#include <imacontent.h>
#include <ismmessage.h>
#include <config.h>
#include <security.h>
#include <cluster.h> // Needed for ismEngine_*Handle_t

#ifdef __cplusplus
extern "C" {
#endif

/// Maximum length in bytes of a destination name, not including the null-termination byte.
#define ismDESTINATION_NAME_LENGTH  65535

/// Default maximum messages of a queue, overridden by messaging policy or static queue definition
#define ismMAXIMUM_MESSAGES_DEFAULT 5000

/// Default maximum message time to live value on publish, overridden by messaging policy
#define ismMAXIMUM_MESSAGE_TIME_TO_LIVE_DEFAULT 0

/// Encourage checking of the return codes of engine API
#define WARN_CHECKRC __attribute__((warn_unused_result))

/// If you really want to ignore the return code of a function defined with WARN_CHECKRC then
/// can wrap it like IGNORE_WARN_CHECKRC(foo(x));
#define IGNORE_WARN_CHECKRC(x) \
     (__extension__ ({ __typeof__ (x) __x = (x); (void) __x; }))

//****************************************************************************
/** @brief Destination types
 */
//****************************************************************************
typedef enum
{
    ismDESTINATION_NONE              = 0, ///< No value
    ismDESTINATION_QUEUE             = 1, ///< Queue
    ismDESTINATION_TOPIC             = 2, ///< Topic
    ismDESTINATION_SUBSCRIPTION      = 3, ///< Subscription
    ismDESTINATION_REGEX_TOPIC       = 4, ///< A topic string in regular expression format (usable on ism_engine_unsetRetainedMessageOnDestination only)
    ismDESTINATION_REMOTESERVER_LOW  = 5, ///< Remote Server Low QoS queue (engine use only)
    ismDESTINATION_REMOTESERVER_HIGH = 6, ///< Remote Server High QoS queue (engine use only)
} ismDestinationType_t;


//****************************************************************************
/** @brief Message states
 * The states in which messages pass as they are delivered.
 */
//****************************************************************************
typedef enum __attribute__ ((__packed__))
{
    ismMESSAGE_STATE_NONE       = 0, ///< No message
    ismMESSAGE_STATE_AVAILABLE  = 0, ///< Message available for consumption
    ismMESSAGE_STATE_DELIVERED  = 1, ///< Message delivered to callback
    ismMESSAGE_STATE_RECEIVED   = 2, ///< Message received by client
    ismMESSAGE_STATE_CONSUMED   = 3, ///< Message consumed - storage will be released
    ismMESSAGE_STATE_RESERVED2  = 127, ///< Value for internal use
    ismMESSAGE_STATE_RESERVED1  = 128  ///< Value for internal use
} ismMessageState_t;

//****************************************************************************
/** @brief Message-delivery status
 * The status of message delivery.
 */
//****************************************************************************
typedef enum
{
    ismMESSAGE_DELIVERY_STATUS_STOPPED  = 0, ///< Message delivery is stopped
    ismMESSAGE_DELIVERY_STATUS_STARTED  = 1, ///< Message delivery is started
    ismMESSAGE_DELIVERY_STATUS_STARTING = 2, ///< Message delivery is starting
    ismMESSAGE_DELIVERY_STATUS_STOPPING = 3, ///< Message delivery is stopping
} ismMessageDeliveryStatus_t;

//****************************************************************************
/** @brief Transaction states
 * The states that a transaction can pass through in it's lifetime
 */
//****************************************************************************
typedef enum
{
    ismTRANSACTION_STATE_NONE               = 0, ///< No state
    ismTRANSACTION_STATE_IN_FLIGHT          = 1, ///< Transaction is inflight
    ismTRANSACTION_STATE_PREPARED           = 2, ///< Transaction is prepared
    ismTRANSACTION_STATE_COMMIT_ONLY        = 3, ///< Transaction must only be committed
    ismTRANSACTION_STATE_ROLLBACK_ONLY      = 4, ///< Transaction must only be rolled back
    ismTRANSACTION_STATE_HEURISTIC_COMMIT   = 5, ///< Heuristic outcome of commit delivered
    ismTRANSACTION_STATE_HEURISTIC_ROLLBACK = 6, ///< Heuristic outcome of rollback delivered
} ismTransactionState_t;

//****************************************************************************
/** @brief Transaction heuristic completion outcome types
 * The types of heuristic outcomes that can be requested when completing a
 * prepared global transaction
 */
//****************************************************************************
typedef enum
{
    ismTRANSACTION_COMPLETION_TYPE_COMMIT   = 0, ///< Heuristic outcome of 'commit'
    ismTRANSACTION_COMPLETION_TYPE_ROLLBACK = 1, ///< Heuristic outcome of 'rollback'
} ismTransactionCompletionType_t;

//****************************************************************************
/** @brief Status values for engine Import / Export Resource requests
 * The status values that can be found in the .status file for an import or
 * export Resource request to the engine.
 */
//****************************************************************************
typedef enum tag_ismImportExportResourceStatusType_t
{
    ismENGINE_IMPORTEXPORT_STATUS_IN_PROGRESS = 0, ///< The request is in-progress
    ismENGINE_IMPORTEXPORT_STATUS_COMPLETE    = 1, ///< The request completed successfully
    ismENGINE_IMPORTEXPORT_STATUS_FAILED      = 2, ///< The request failed
} ismEngine_ImportExportResourceStatusType_t;

//****************************************************************************
/** @brief JSON field identifier names and values found in the .status file
 * for an import or export Resource request to the engine.
 */
//****************************************************************************

/// Field identifier for import / export request Identifier [UINT64 value]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_REQUESTID               "RequestID"
/// Field identifier for fully qualified path to import / export file if it was created [STRING]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_FILEPATH                "FilePath"
/// Field identifier for the fileName specification if present for export  [STRING]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_FILENAME                "FileName"
/// Field identifier for the clientID specification if present for import / export [Regex STRING]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_CLIENTID                "ClientID"
/// Field identifier for the topic specification if present for import / export [Regex STRING]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_TOPIC                   "Topic"
/// Field identifier for name of the exporting server on import / export [STRING]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_EXPORTSERVERNAME        "ExportingServerName"
/// Field identifier for UID of the exporting server on import / export [STRING]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_EXPORTSERVERUID         "ExportingServerUID"
/// Field identifier for name of the importing server on import only [STRING]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_IMPORTSERVERNAME        "ImportingServerName"
/// Field identifier for UID of the importing server on import only [STRING]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_IMPORTSERVERUID         "ImportingServerUID"
/// Field identifier for import / export server init time [UINT64 of time that import/export was initialized]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_SERVERINITTIME          "ServerInitTime"
/// Field identifier for import / export start time [UINT64 from ism_common_currentTimeNanos]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_STARTTIME               "StartTime"
/// Field identifier for last status file update time [UINT64 from ism_common_currentTimeNanos]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUSUPDATETIME        "StatusUpdateTime"
/// Field identifier for import / export end time [UINT64 from ism_common_currentTimeNanos]
/// Note: This is only present when the import / export has finished (either successfully or having failed)
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_ENDTIME                 "EndTime"
/// Field identifier for status file current status [numeric ismEngine_ImportExportResourceStatusType_t]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_STATUS                  "Status"
/// Field identifier for the status file retcode [numeric ISMRC_ value]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETCODE                 "RetCode"
/// Field identifier for export records written so far [UINT64 value]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_RECORDSWRITTEN          "RecordsWritten"
/// Field identifier for the array of diagnostic information objects on import/export [Array]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_DIAGNOSTICS             "Diagnostics"
/// Field identifier for import records read so far [UINT64 value]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_RECORDSREAD             "RecordsRead"
/// Field identifier for import records for which processing was started [UINT64 value]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_RECORDSSTARTED          "RecordsStarted"
/// Field identifier for import records for which processing was finished [UINT64 value]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_RECORDSFINISHED         "RecordsFinished"
/// Field identifier for number of clients imported [UINT64 value]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_CLIENTSIMPORTED         "ClientsImported"
/// Field identifier for number of retained messages imported [UINT64 value]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETAINEDMSGSIMPORTED    "RetainedMsgsImported"
/// Field identifier for number of subscriptions imported [UINT64 value]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_SUBSCRIPTIONSIMPORTED   "SubscriptionsImported"
/// Field identifier for number of clients exported [UINT64 value]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_CLIENTSEXPORTED         "ClientsExported"
/// Field identifier for number of retained messages exported [UINT64 value]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_RETAINEDMSGSEXPORTED    "RetainedMsgsExported"
/// Field identifier for number of subscriptions exported [UINT64 value]
#define ismENGINE_IMPORTEXPORT_STATUS_FIELD_SUBSCRIPTIONSEXPORTED   "SubscriptionsExported"

/// Field identifier in diagnostic object for the 'human readable' resource identifier (String)
#define ismENGINE_IMPORTEXPORT_DIAGNOSTIC_FIELD_RESOURCEIDENTIFIER "ResourceIdentifier"
/// Field identifier in diagnostic object for the resource data identifier (UINT64 from export file)
#define ismENGINE_IMPORTEXPORT_DIAGNOSTIC_FIELD_RESOURCEDATAID     "ResourceDataID"
/// Field identifier in diagnostic object for the resource return code (numeric ISMRC_ value)
#define ismENGINE_IMPORTEXPORT_DIAGNOSTIC_FIELD_RESOURCERC         "ResourceRC"
/// Field identifier in diagnostic object for the resource type (String)
#define ismENGINE_IMPORTEXPORT_DIAGNOSTIC_FIELD_RESOURCETYPE       "ResourceType"

/// Resource type value for a diagnostic object relating to a Client state
#define ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_CLIENT       "Client"
/// Resource type value for a diagnostic object relating to a Subscription
#define ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_SUBSCRIPTION "Subscription"
/// Resource type value for a diagnostic object relation to a Retained Msg
#define ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_RETAINEDMSG  "RetainedMsg"
/// Resource type value for an unrecognized object type (also has numeric type in brackets following)
#define ismENGINE_IMPORTEXPORT_DIAGNOSTIC_VALUE_UNKNOWN_STEM "Unknown"

//****************************************************************************
/** @name Monitoring statistics
 * Monitoring information for Engine resources.
 */
//****************************************************************************

/**@{*/

//****************************************************************************
/** @brief Messaging statistics
 * Statistics about messages flowing through the engine
 */
//****************************************************************************
typedef struct
{
    ism_time_t ServerShutdownTime;          ///< The last time recorded by the engine prior to previous shutdown
    uint64_t DroppedMessages;               ///< Total number of messages dropped since server restart
    uint64_t ExpiredMessages;               ///< Total number of messages expired since server restart
    uint64_t BufferedMessages;              ///< Current buffered messages on the server (including inflight)
    uint64_t RetainedMessages;              ///< Current retained messages on the server (excluding inflight and NullRetained)
    uint64_t ClientStates;                  ///< Total number of (non-internal) clientStates in the engine's table (including mid-theft stealers)
    uint64_t ExpiredClientStates;           ///< Total number of clientStates expired since server restart
    uint64_t Subscriptions;                 ///< Total number of subscriptions in the topic tree
} ismEngine_MessagingStatistics_t;

//****************************************************************************
/** @brief Memory Statistics
 * Statistics about memory
 */
//****************************************************************************
typedef struct
{
    bool     MemoryCGroupInUse;         ///< Whether a CGroup is being used to impose limits
    uint64_t MemoryTotalBytes;          ///< Total physical memory on the system
    uint64_t MemoryFreeBytes;           ///< Free memory on the system
    double   MemoryFreePercent;         ///< Free memory as a percentage of total memory
    uint64_t ServerVirtualMemoryBytes;  ///< Virtual memory size of imaserver process
    uint64_t ServerResidentSetBytes;    ///< Resident set size of imaserver process

    // Grouped memory types
    uint64_t GroupMessagePayloads;      ///< Group - Message payloads (bytes)
    uint64_t GroupPublishSubscribe;     ///< Group - Publish/subscribe - topics and subscriptions (bytes)
    uint64_t GroupDestinations;         ///< Group - Destinations containing messages (bytes)
    uint64_t GroupCurrentActivity;      ///< Group - Memory for current activity (bytes)
    uint64_t GroupRecovery;             ///< Group - Recovery (bytes)
    uint64_t GroupClientStates;         ///< Group - Client state information (bytes)

    // Individual memory types - for detailed memory statistics only
    uint64_t MessagePayloads;           ///< Message Payloads
    uint64_t TopicAnalysis;             ///< Topic Analysis
    uint64_t SubscriberTree;            ///< Subscriber Tree
    uint64_t NamedSubscriptions;        ///< Named Subscription Hash Table (Durable & Shared)
    uint64_t SubscriberCache;           ///< Subscriber Cache
    uint64_t SubscriberQuery;           ///< Subscriber Query
    uint64_t TopicTree;                 ///< Topic Tree
    uint64_t TopicQuery;                ///< Topic Query
    uint64_t ClientState;               ///< Client State
    uint64_t CallbackContext;           ///< Callback Context
    uint64_t PolicyInfo;                ///< Engine's structures containing config policy information
    uint64_t QueueNamespace;            ///< Named Queues
    uint64_t SimpleQueueDefns;          ///< Simple Queue Definitions
    uint64_t SimpleQueuePages;          ///< Simple Queue Pages
    uint64_t IntermediateQueueDefns;    ///< Intermediate Queue Definitions
    uint64_t IntermediateQueuePages;    ///< Intermediate Queue Pages
    uint64_t MulticonsumerQueueDefns;   ///< Multi-consumer Queue Definitions
    uint64_t MulticonsumerQueuePages;   ///< Multi-consumer Queue Pages
    uint64_t LockManager;               ///< Lock Manager
    uint64_t MQConnectivityRecords;     ///< MQ Connectivity
    uint64_t RecoveryTable;             ///< Recovery Table
    uint64_t ExternalObjects;           ///< External Engine Objects
    uint64_t LocalTransactions;         ///< Local Transactions
    uint64_t GlobalTransactions;        ///< Global Transactions
    uint64_t MonitoringData;            ///< Monitoring Data
    uint64_t NotificationData;          ///< Notification Data
    uint64_t MessageExpiryData;         ///< Message Expiry Data
    uint64_t RemoteServerData;          ///< Remote Server Data
    uint64_t CommitData;                ///< Memory alloc'd for a commit
    uint64_t UnneededRetainedMsgs;      ///< Tracking of unneeded retained messages during recovery
    uint64_t ExpiringGetData;           ///< Memory about get-with-timeouts
    uint64_t ExportResources;           ///< Used for exporting sets of engine resources (e.g. client sets)
    uint64_t Diagnostics;               ///< Used for diagnostics reported by engine
    uint64_t UnneededBufferedMsgs;      ///< Messages rehydrated by not needed
    uint64_t JobQueues;                 ///< Inter-Thread Work Scheduling
    uint64_t ResourceSetStats;          ///< Stats about resourcesets ("orgs")
    uint64_t DeferredFreeLists;         ///< Lists keeping track of deferred memory free requests
} ismEngine_MemoryStatistics_t;

//****************************************************************************
/** @brief Queue Statistics
 * Statistics about a queue
 * Note: Historic values are 'since the server started', they are not persisted
 */
//****************************************************************************
typedef struct tag_ismEngine_QueueStatistics_t
{
    uint64_t BufferedMsgs;              ///< Current buffered messages
    uint64_t BufferedMsgsHWM;           ///< High water mark of buffered messages
    uint64_t RejectedMsgs;              ///< Messages rejected due to MaxMessage limit
    uint64_t DiscardedMsgs;             ///< Messages that had been buffered but were discarded to make
                                        ///  space for new messages
    uint64_t ExpiredMsgs;               ///< Messages that have expired while buffered
    uint64_t InflightEnqs;              ///< In-flight puts to the queue
    uint64_t InflightDeqs;              ///< In-flight 'gets' from the queue
    uint64_t EnqueueCount;              ///< Total messages buffered when no consumer was available
    uint64_t DequeueCount;              ///< Total messages removed from the buffer
    uint64_t QAvoidCount;               ///< Messages put directly to a consumer on this queue
    uint64_t MaxMessages;               ///< The MaxMessageCount value for this queue
    uint64_t PutsAttempted;             ///< The current count of puts attempted on this queue
    uint64_t BufferedMsgBytes;          ///< The current total of buffered message bytes
    uint64_t MaxMessageBytes;           ///< The MaxMessageBytes value for this queue
    // The following are calculated when ieq_getStats is called
    uint64_t ProducedMsgs;              ///< Sum of EnqueueCount & QAvoidCount (Msgs 'put' on to this queue)
    uint64_t ConsumedMsgs;              ///< Sum of DequeueCount & QAvoidCount (Msgs 'got' from this queue)
    uint64_t PutsAttemptedDelta;        ///< Change in puts attempted since last 'activity' stats request
    double BufferedPercent;             ///< Current buffered messages as a percentage of MaxMessage limit
    double BufferedHWMPercent;          ///< Buffered High water mark as a percentage of MaxMessageCount
} ismEngine_QueueStatistics_t;

//****************************************************************************
/** @brief Topic Statistics
 * Statistics about the subtree beneath a topic
 * Note: Values are since the topic monitor on this topic string was reset, which
 *       is available in ResetTime.
 */
//****************************************************************************
typedef struct tag_ismEngine_TopicStatistics_t
{
    ism_time_t  ResetTime;              ///< When these stats were reset (0 if not active)
    uint64_t    PublishedMsgs;          ///< Count of messages successfully published
    uint64_t    RejectedMsgs;           ///< Count of messages rejected by at least one subscription
    uint64_t    FailedPublishes;        ///< Count of publishes that failed because they could not be delivered to a hiqh QoS subscription
} ismEngine_TopicStatistics_t;

//*********************************************************************
/// @brief  Statistics about a remote server
//*********************************************************************
typedef struct tag_ismEngine_RemoteServerStatistics_t
{
    char serverUID[16];                ///< ServerUID of the server being returned
    bool retainedSync;                 ///< Whether the local server has retained messages in-sync with the returned server
    ismEngine_QueueStatistics_t q0;    ///< Statistics from the lowQoS forwarding queue to the returned server
    ismEngine_QueueStatistics_t q1;    ///< Statistics from the highQoS forwarding queue to the returned server
} ismEngine_RemoteServerStatistics_t;

//*********************************************************************
/// @brief  Statistics about a resource set]
///
/// @remark The indentation in comments below is meant to indicate values
/// that make up the stat above them...
//*********************************************************************
typedef struct tag_ismEngine_ResourceSetStatistics_t
{
    uint64_t    TotalMemoryBytes;                     ///< Total bytes of memory allocated to any object assigned to this resourceSet
    uint64_t    Subscriptions;                        ///< Total number of subscriptions assigned to this resourceSet
    uint64_t    PersistentNonSharedSubscriptions;     ///<   Total number of persistent non-shared subscriptions assigned to this resourceSet (based on owning ClientID)
    uint64_t    NonpersistentNonSharedSubscriptions;  ///<   Total number of nonpersistent non-shared subscriptions assigned to this resourceSet (based on owning ClientID)
    uint64_t    PersistentSharedSubscriptions;        ///<   Total number of persistent shared subscriptions assigned to this resourceSet (based on subscribed Topic)
    uint64_t    NonpersistentSharedSubscriptions;     ///<   Total number of nonpersistent shared subscriptions assigned to this resourceSet (based on subscribed Topic)
    uint64_t    BufferedMsgs;                         ///< Sum of all BufferedMsgs counts on all subscriptions assigned to this resourceSet.
    uint64_t    DiscardedMsgsSinceRestart;            ///< Sum of all DiscardedMsgs counts since server restarted.
    uint64_t    DiscardedMsgs;                        ///< Sum of all DiscardedMsgs counts (since reset) on all subscriptions assigned to this resourceSet.
    uint64_t    RejectedMsgsSinceRestart;             ///< Sum of all RejectedMsgs counts since server restarted.
    uint64_t    RejectedMsgs;                         ///< Sum of all RejectedMsgs counts (since reset) on all subscriptions assigned to this resourceSet.
    uint64_t    RetainedMsgs;                         ///< Total number of retained messages assigned to this resourceSet (based on published Topic).
    uint64_t    WillMsgs;                             ///< Total number of will messages set on clients assigned to this resourceSet (based on ClientID)
    uint64_t    BufferedMsgBytes;                     ///< Total bytes in any messages retained by this resourceSet or buffered on any subscription assigned to this resourceSet.
    uint64_t    PersistentBufferedMsgBytes;           ///<   Total bytes in persistent messages retained by this resourceSet or buffered on any subscription assigned to this resourceSet.
    uint64_t    NonpersistentBufferedMsgBytes;        ///<   Total bytes in nonpersistent messages buffered on any subscription assigned to this resourceSet.
    uint64_t    RetainedMsgBytes;                     ///< Total bytes in (persistent) messages retained by this resourceSet.
    uint64_t    WillMsgBytes;                         ///< Total bytes in any messages set as will messages on clients assigned to this resourceSet.
    uint64_t    PersistentWillMsgBytes;               ///<   Total bytes in persistent messages set as will messages on clients assigned to this resourceSet.
    uint64_t    NonpersistentWillMsgBytes;            ///<   Total bytes in nonpersistent messages set as will messages on clients assigned to this resourceSet.
    uint64_t    PublishedMsgsSinceRestart;            ///< Count of messages published since server restarted
    uint64_t    PublishedMsgs;                        ///< Count of messages published (since stats reset) by clients assigned to this resourceSet.
    uint64_t    QoS0PublishedMsgs;                    ///<   Count of QoS 0 messages (since stats reset) published by clients assigned to this resourceSet.
    uint64_t    QoS1PublishedMsgs;                    ///<   Count of QoS 1 messages (since stats reset) published by clients assigned to this resourceSet.
    uint64_t    QoS2PublishedMsgs;                    ///<   Count of QoS 2 messages (since stats reset) published by clients assigned to this resourceSet.
    uint64_t    PublishedMsgBytesSinceRestart;        ///< Sum of bytes published since server restarted
    uint64_t    PublishedMsgBytes;                    ///< Sum of bytes in messages published (since stats reset) by clients assigned to this resourceSet.
    uint64_t    QoS0PublishedMsgBytes;                ///<   Sum of bytes in QoS 0 messages published (since stats reset) by clients assigned to this resourceSet.
    uint64_t    QoS1PublishedMsgBytes;                ///<   Sum of bytes in QoS 1 messages published (since stats reset) by clients assigned to this resourceSet.
    uint64_t    QoS2PublishedMsgBytes;                ///<   Sum of bytes in QoS 2 messages published (since stats reset) by clients assigned to this resourceSet.
    uint64_t    MaxPublishRecipientsSinceRestart;     ///< Largest number of recipients for a single published message since server restarted (ClientID)
    uint64_t    MaxPublishRecipients;                 ///< Largest number of recipients for a single published message (since stats reset) (ClientID)
    uint64_t    ConnectionsSinceRestart;              ///< Count of all connection attempts made since server restarted
    uint64_t    Connections;                          ///< Count of all connection attempts made (since reset) by clients assigned to this resourceSet (based on ClientID).
    uint64_t    ActiveClients;                        ///< Total number of clients assigned to this resourceSet currently connected to the server.
    uint64_t    ActivePersistentClients;              ///<   Total number of persistent (cleanSession=false) clients assigned to this resourceSet currently connected to the server.
    uint64_t    ActiveNonpersistentClients;           ///<   Total number of nonpersistent (cleanSession=true) clients assigned to this resourceSet currently connected to the server.
    uint64_t    PersistentClientStates;               ///< Total number of persistent (cleanSession=false) client states created by clients assigned to this resourceSet.
} ismEngine_ResourceSetStatistics_t;

typedef enum
{
    ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,                        ///< Objects with highest BufferedMsgs (SubscriptionMonitor/QueueMonitor/ResourceSetMonitor)
    ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS,                         ///< Objects with lowest BufferedMsgs (SubscriptionMonitor/QueueMonitor)
    ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGBYTES,                    ///< Objects with the highest amount of buffered msg bytes (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_PERSISTENTBUFFEREDMSGBYTES,          ///< Objects with the highest amount of persistent buffered msg bytes (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_NONPERSISTENTBUFFEREDMSGBYTES,       ///< Objects with the highest amount of non persistent buffered msg bytes (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_BUFFEREDPERCENT,                     ///< Objects with highest BufferedPercent (SubscriptionMonitor/QueueMonitor)
    ismENGINE_MONITOR_LOWEST_BUFFEREDPERCENT,                      ///< Objects with lowest BufferedPercent (SubscriptionMonitor/QueueMonitor)
    ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS,                        ///< Objects with highest RejectedMsgs (SubscriptionMonitor/TopicMonitor/QueueMonitor)
    ismENGINE_MONITOR_LOWEST_REJECTEDMSGS,                         ///< Objects with lowest RejectedMsgs (SubscriptionMonitor/TopicMonitor/QueueMonitor)
    ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME,                    ///< Objects with oldest LastConnectedTime (ClientStateMonitor)
    ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,                       ///< Objects with highest PublishedMsgs (SubscriptionMonitor/TopicMonitor/ResourceSetMonitor)
    ismENGINE_MONITOR_LOWEST_PUBLISHEDMSGS,                        ///< Objects with lowest PublishedMsgs (SubscriptionMonitor/TopicMonitor)
    ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGS,                   ///< Objects with the highest QoS0 PublishedMsgs (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGS,                   ///< Objects with the highest QoS1 PublishedMsgs (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGS,                   ///< Objects with the highest QoS2 PublishedMsgs (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGBYTES,                   ///< Objects with highest amount of published message bytes (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGBYTES,               ///< Objects with the highest amount of QoS0 published message bytes (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGBYTES,               ///< Objects with the highest amount of QoS1 published message bytes (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGBYTES,               ///< Objects with the highest amount of QoS2 published message bytes (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_MAXPUBLISHRECIPIENTS,                ///< Objects with the highest number of recipients for any single publish [widest fanout] (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS,                       ///< Objects with highest number of subscriptions (TopicMonitor/ResourceSetMonitor)
    ismENGINE_MONITOR_LOWEST_SUBSCRIPTIONS,                        ///< Objects with lowest number of subscriptions (TopicMonitor)
    ismENGINE_MONITOR_HIGHEST_PERSISTENTNONSHAREDSUBSCRIPTIONS,    ///< Objects with the highest number of persistent non shared subscriptions (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_NONPERSISTENTNONSHAREDSUBSCRIPTIONS, ///< Objects with the highest number of non persistent non shared subscriptions (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_PERSISTENTSHAREDSUBSCRIPTIONS,       ///< Objects with the highest number of persistent shared subscriptions (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_NONPERSISTENTSHAREDSUBSCRIPTIONS,    ///< Objects with the highest number of non persistent shared subscriptions (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_FAILEDPUBLISHES,                     ///< Objects with highest number of failed publishes (TopicMonitor)
    ismENGINE_MONITOR_LOWEST_FAILEDPUBLISHES,                      ///< Objects with lowest number of failed publishes (TopicMonitor)
    ismENGINE_MONITOR_HIGHEST_PRODUCERS,                           ///< Objects with highest number of current producers (QueueMonitor)
    ismENGINE_MONITOR_LOWEST_PRODUCERS,                            ///< Objects with lowest number of current producers (QueueMonitor)
    ismENGINE_MONITOR_HIGHEST_CONSUMERS,                           ///< Objects with highest number of current consumers (QueueMonitor)
    ismENGINE_MONITOR_LOWEST_CONSUMERS,                            ///< Objects with lowest number of current consumers (QueueMonitor)
    ismENGINE_MONITOR_HIGHEST_CONSUMEDMSGS,                        ///< Objects with highest number of consumed messages (QueueMonitor)
    ismENGINE_MONITOR_LOWEST_CONSUMEDMSGS,                         ///< Objects with lowest number of consumed messages (QueueMonitor)
    ismENGINE_MONITOR_HIGHEST_PRODUCEDMSGS,                        ///< Objects with highest number of produced messages (QueueMonitor)
    ismENGINE_MONITOR_LOWEST_PRODUCEDMSGS,                         ///< Objects with lowest number of produced messages (QueueMonitor)
    ismENGINE_MONITOR_ALL_UNSORTED,                                ///< All results of the particular object type, unsorted (TopicMonitor/ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_BUFFEREDHWMPERCENT,                  ///< Objects with highest Buffered High Water Mark Percent (SubscriptionMonitor/QueueMonitor)
    ismENGINE_MONITOR_LOWEST_BUFFEREDHWMPERCENT,                   ///< Objects with lowest Buffered High Water Mark Percent (SubscriptionMonitor/QueueMonitor)
    ismENGINE_MONITOR_OLDEST_STATECHANGE,                          ///< Objects with the oldest StateChangedTime (TransactionMonitor)
    ismENGINE_MONITOR_HIGHEST_EXPIREDMSGS,                         ///< Objects with the highest number of expired messages (SubscriptionMonitor/QueueMonitor)
    ismENGINE_MONITOR_LOWEST_EXPIREDMSGS,                          ///< Objects with the lowest number of expired messages (SubscriptionMonitor/QueueMonitor)
    ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS,                       ///< Objects with the highest number of discarded messages (SubscriptionMonitor/QueueMonitor/ResourceSetMonitor)
    ismENGINE_MONITOR_LOWEST_DISCARDEDMSGS,                        ///< Objects with the lowest number of discarded messages (SubscriptionMonitor/QueueMonitor)
    ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES,                    ///< Objects with the highest amount of total memory in use (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_RETAINEDMSGS,                        ///< Objects with the highest number of retained messages (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_RETAINEDMSGBYTES,                    ///< Objects with the highest amount of retained message bytes (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_WILLMSGS,                            ///< Objects with the highest number of will messages attached to clients (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_WILLMSGBYTES,                        ///< Objects with the highest amount of will message bytes attached to clients (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_PERSISTENTWILLMSGBYTES,              ///< Objects with the highest amount of persistent will message bytes attached to clients (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_NONPERSISTENTWILLMSGBYTES,           ///< Objects with the highest amount of non persistent will message bytes attached to clients (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_CONNECTIONS,                         ///< Objects with the highest number of connection attempts (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_ACTIVECLIENTS,                       ///< Objects with the highest number of active clients (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_ACTIVEPERSISTENTCLIENTS,             ///< Objects with the highest number of active persistent clients (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_ACTIVENONPERSISTENTCLIENTS,          ///< Objects with the highest number of active nonpersistent clients (ResourceSetMonitor)
    ismENGINE_MONITOR_HIGHEST_PERSISTENTCLIENTSTATES,              ///< Objects with the highest number of persistent client stats (ResourceSetMonitor)
    // Maximum external value... add new external values above this line
    ismENGINE_MONITOR_MAX = ismENGINE_MONITOR_HIGHEST_PERSISTENTCLIENTSTATES,
    // Internal values...
    ismENGINE_MONITOR_NONE,
    ismENGINE_MONITOR_INTERNAL_FAKEHOURLY,
    ismENGINE_MONITOR_INTERNAL_FAKEDAILY,
    ismENGINE_MONITOR_INTENAL_FAKEWEEKLY,
} ismEngineMonitorType_t;

// StatType strings that map to ismEngineMonitorType_t values (Note: this is only a subset)
#define ismENGINE_MONITOR_STATTYPE_BUFFEREDMSGS_HIGHEST                        "BufferedMsgsHighest"
#define ismENGINE_MONITOR_STATTYPE_BUFFEREDMSGBYTES_HIGHEST                    "BufferedMsgBytesHighest"
#define ismENGINE_MONITOR_STATTYPE_PERSISTENTBUFFEREDMSGBYTES_HIGHEST          "PersistentBufferedMsgBytesHighest"
#define ismENGINE_MONITOR_STATTYPE_NONPERSISTENTBUFFEREDMSGBYTES_HIGHEST       "NonpersistentBufferedMsgBytesHighest"
#define ismENGINE_MONITOR_STATTYPE_PUBLISHEDMSGS_HIGHEST                       "PublishedMsgsHighest"
#define ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS0MSGS_HIGHEST                   "QoS0PublishedMsgsHighest"
#define ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS1MSGS_HIGHEST                   "QoS1PublishedMsgsHighest"
#define ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS2MSGS_HIGHEST                   "QoS2PublishedMsgsHighest"
#define ismENGINE_MONITOR_STATTYPE_PUBLISHEDMSGBYTES_HIGHEST                   "PublishedMsgBytesHighest"
#define ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS0MSGBYTES_HIGHEST               "QoS0PublishedMsgBytesHighest"
#define ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS1MSGBYTES_HIGHEST               "QoS1PublishedMsgBytesHighest"
#define ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS2MSGBYTES_HIGHEST               "QoS2PublishedMsgBytesHighest"
#define ismENGINE_MONITOR_STATTYPE_MAXPUBLISHRECIPIENTS_HIGHEST                "MaxPublishRecipientsHighest"
#define ismENGINE_MONITOR_STATTYPE_SUBSCRIPTIONS_HIGHEST                       "SubscriptionsHighest"
#define ismENGINE_MONITOR_STATTYPE_PERSISTENTNONSHAREDSUBSCRIPTIONS_HIGHEST    "PersistentNonSharedSubscriptionsHighest"
#define ismENGINE_MONITOR_STATTYPE_NONPERSISTENTNONSHAREDSUBSCRIPTIONS_HIGHEST "NonpersistentNonSharedSubscriptionsHighest"
#define ismENGINE_MONITOR_STATTYPE_PERSISTENTSHAREDSUBSCRIPTIONS_HIGHEST       "PersistentSharedSubscriptionsHighest"
#define ismENGINE_MONITOR_STATTYPE_NONPERSISTENTSHAREDSUBSCRIPTIONS_HIGHEST    "NonpersistentSharedSubscriptionsHighest"
#define ismENGINE_MONITOR_STATTYPE_ALLUNSORTED                                 "AllUnsorted"
#define ismENGINE_MONITOR_STATTYPE_DISCARDEDMSGS_HIGHEST                       "DiscardedMsgsHighest"
#define ismENGINE_MONITOR_STATTYPE_REJECTEDMSGS_HIGHEST                        "RejectedMsgsHighest"
#define ismENGINE_MONITOR_STATTYPE_TOTALMEMORYBYTES_HIGHEST                    "TotalMemoryBytesHighest"
#define ismENGINE_MONITOR_STATTYPE_RETAINEDMSGS_HIGHEST                        "RetainedMsgsHighest"
#define ismENGINE_MONITOR_STATTYPE_RETAINEDMSGBYTES_HIGHEST                    "RetainedMsgBytesHighest"
#define ismENGINE_MONITOR_STATTYPE_WILLMSGS_HIGHEST                            "WillMsgsHighest"
#define ismENGINE_MONITOR_STATTYPE_WILLMSGBYTES_HIGHEST                        "WillMsgBytesHighest"
#define ismENGINE_MONITOR_STATTYPE_PERSISTENTWILLMSGBYTES_HIGHEST              "PersistentWillMsgBytesHighest"
#define ismENGINE_MONITOR_STATTYPE_NONPERSISTENTWILLMSGBYTES_HIGHEST           "NonpersistentWillMsgBytesHighest"
#define ismENGINE_MONITOR_STATTYPE_CONNECTIONS_HIGHEST                         "ConnectionsHighest"
#define ismENGINE_MONITOR_STATTYPE_ACTIVECLIENTS_HIGHEST                       "ActiveClientsHighest"
#define ismENGINE_MONITOR_STATTYPE_ACTIVEPERSISTENTCLIENTS_HIGHEST             "ActivePersistentClientsHighest"
#define ismENGINE_MONITOR_STATTYPE_ACTIVENONPERSISTENTCLIENTS_HIGHEST          "ActiveNonpersistentClientsHighest"
#define ismENGINE_MONITOR_STATTYPE_PERSISTENTCLIENTSTATES_HIGHEST              "PersistentClientStatesHighest"
#define ismENGINE_MONITOR_STATTYPE_NONE                                        "None"
#define ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEHOURLY                         "FakeHourly"
#define ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEDAILY                          "FakeDaily"
#define ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEWEEKLY                         "FakeWeekly"

/// Filter name for monitoring by subscription name
#define ismENGINE_MONITOR_FILTER_SUBNAME            "SubName"

/// Filter name for monitoring by topic string
#define ismENGINE_MONITOR_FILTER_TOPICSTRING        "TopicString"

/// Filter name for monitoring by standard (i.e. simple string) wildcard client ID
#define ismENGINE_MONITOR_FILTER_CLIENTID           "ClientID"

/// Filter name for monitoring by regular expression client ID
#define ismENGINE_MONITOR_FILTER_REGEX_CLIENTID     "RegExClientID"

/// Filter name for monitoring by client protocol ID
#define ismENGINE_MONITOR_FILTER_PROTOCOLID         "ProtocolID"

/// Values for filtering by protocol ID
#define ismENGINE_MONITOR_FILTER_PROTOCOLID_ALL         "All"
#define ismENGINE_MONITOR_FILTER_PROTOCOLID_DEVICELIKE  "DeviceLike" // MQTT, HTTP & PLUGIN
#define ismENGINE_MONITOR_FILTER_PROTOCOLID_MQTT        "MQTT"
#define ismENGINE_MONITOR_FILTER_PROTOCOLID_HTTP        "HTTP"
#define ismENGINE_MONITOR_FILTER_PROTOCOLID_PLUGIN      "PlugIn"

/// Filter name for monitoring by client connection state
#define ismENGINE_MONITOR_FILTER_CONNECTIONSTATE    "ConnectionState"

/// Values for filtering by client connection state
#define ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_DISCONNECTED "Disconnected"
#define ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_CONNECTED    "Connected"
#define ismENGINE_MONITOR_FILTER_CONNECTIONSTATE_ALL          "All"

/// Filter name for monitoring by subscription type
#define ismENGINE_MONITOR_FILTER_SUBTYPE            "SubType"

/// Values for filtering by subscription type
#define ismENGINE_MONITOR_FILTER_SUBTYPE_ALL        "All"
#define ismENGINE_MONITOR_FILTER_SUBTYPE_DURABLE    "Durable"
#define ismENGINE_MONITOR_FILTER_SUBTYPE_NONDURABLE "Nondurable"
#define ismENGINE_MONITOR_FILTER_SUBTYPE_SHARED     "Shared"
#define ismENGINE_MONITOR_FILTER_SUBTYPE_NONSHARED  "Nonshared"
#define ismENGINE_MONITOR_FILTER_SUBTYPE_ADMIN      "Admin"

/// Filter name for monitoring by subscription messaging policy name
#define ismENGINE_MONITOR_FILTER_MESSAGINGPOLICY    "MessagingPolicy"

/// Filter name for monitoring by queue name
#define ismENGINE_MONITOR_FILTER_QUEUENAME          "QueueName"

/// Filter for monitoring by queue scope
#define ismENGINE_MONITOR_FILTER_SCOPE              "Scope"

// Values for filtering by queue scope
#define ismENGINE_MONITOR_FILTER_SCOPE_ALL          "All"
#define ismENGINE_MONITOR_FILTER_SCOPE_SERVER       "Server"
#define ismENGINE_MONITOR_FILTER_SCOPE_CLIENT       "Client"

// Filter name for monitoring by XID
#define ismENGINE_MONITOR_FILTER_XID                "Xid"

// Filter name for monitoring by transaction state
#define ismENGINE_MONITOR_FILTER_TRANSTATE          "TranState"

/// Filter name for monitoring by resource set identifier
#define ismENGINE_MONITOR_FILTER_RESOURCESET_ID     "ResourceSetID"

// Values for filtering by transaction state
#define ismENGINE_MONITOR_FILTER_TRANSTATE_PREPARED            "Prepared"          ///< Prepared transactions
#define ismENGINE_MONITOR_FILTER_TRANSTATE_HEURISTIC_COMMIT    "HeuristicCommit"   ///< Heuristically committed
#define ismENGINE_MONITOR_FILTER_TRANSTATE_HEURISTIC_ROLLBACK  "HeuristicRollback" ///< Heuristically rolled back
#define ismENGINE_MONITOR_FILTER_TRANSTATE_HEURISTIC           "Heuristic"         ///< Heuristically completed (either committed or rolled back)
#define ismENGINE_MONITOR_FILTER_TRANSTATE_ALL                 "All"               ///< Prepared or Heuristically completed
// Note: The following are defined, but are not expected to be exposed externally
#define ismENGINE_MONITOR_FILTER_TRANSTATE_IN_FLIGHT           "Inflight"          ///< Inflight transactions
#define ismENGINE_MONITOR_FILTER_TRANSTATE_COMMIT_ONLY         "CommitOnly"        ///< Commit only transactions
#define ismENGINE_MONITOR_FILTER_TRANSTATE_ROLLBACK_ONLY       "RollbackOnly"      ///< Rollback only transactions
#define ismENGINE_MONITOR_FILTER_TRANSTATE_ANY                 "Any"               ///< Any transaction state (i.e. all of the above)

typedef struct tag_ismEngine_SubscriptionMonitor_t
{
    char *topicString;                  ///< The topic string this subscription is on
    char *subName;                      ///< Subscription name (durable subscription only)
    char *clientId;                     ///< Client Id
    char *messagingPolicyName;          ///< Current name of the messaging policy used by this subscription
    uint32_t policyType;                ///< ismSecurityPolicyType_t of the specified policy name (cast to a uint32_t)
    uint32_t internalAttrs;             ///< Internal Subscription options at time of processing
    uint32_t consumers;                 ///< Number of consumers on this subscription (might be >1 for shared)
    bool durable;                       ///< Whether this is a durable sub
    bool shared;                        ///< Whether this is a shared subscription
    ismEngine_QueueStatistics_t stats;  ///< Statistics for this subscription
    const char * resourceSetId;         ///< The resourceSet to which this subscription belongs (if any)
    const char * configType;            ///< For an admin subscription, what configuration type it is.
    void *subscription;                 ///< Pointer to the subscription (internal use only)
    void *policyInfo;                   ///< Pointer to the policy info for the subscription (internal use only)
} ismEngine_SubscriptionMonitor_t;

#define ismENGINE_MONITOR_SUBSCRIPTION_BATCHSIZE 100000 ///< When enumerating subscriptions, how many to check before bouncing the subscription lock

typedef struct tag_ismEngine_TopicMonitor_t
{
    char *topicString;                  ///< The topic string being monitored
    uint64_t subscriptions;             ///< Count of subscriptions
    ismEngine_TopicStatistics_t stats;  ///< Statistics for this topic
    void *node;                         ///< Pointer to the subsNode (internal use only)
} ismEngine_TopicMonitor_t;

typedef enum tag_ismQueueScope_t
{
    ismQueueScope_All,
    ismQueueScope_Server,
    ismQueueScope_Client
} ismQueueScope_t;

typedef struct tag_ismEngine_QueueMonitor_t
{
    char *queueName;                    ///< The topic string this subscription is on
    ismQueueScope_t scope;              ///< Client or Server
    ismEngine_QueueStatistics_t stats;  ///< Statistics for this queue
    uint32_t producers;                 ///< Count of producers currently on the queue
    uint32_t consumers;                 ///< Count of consumers currently on the queue
    void *queue;                        ///< Pointer to the queue (internal use only)
} ismEngine_QueueMonitor_t;

typedef struct tag_ismEngine_ResourceSetMonitor_t
{
    char *resourceSetId;                      ///< The resource set identifier string (from 1st regex capture group)
    void *resourceSet;                        ///< Pointer to the resourceSet_t (internal use only)
    ism_time_t resetTime;                     ///< Time at which these stats were reset
    ism_time_t reportTime;                    ///< Time at which these stats are being reported
    ismEngine_ResourceSetStatistics_t stats;  ///< Statistics for this resource set
} ismEngine_ResourceSetMonitor_t;

//*********************************************************************
/// @brief  Client-state monitor
///
/// Represents the monitoring information for a client-state.
//*********************************************************************
typedef struct
{
    const char * ClientId;             ///< The client ID
    uint32_t     ProtocolId;           ///< The numeric protocol type id of the client
    bool         fIsConnected;         ///< Whether client is connected
    bool         fIsDurable;           ///< Whether client is durable (will persist after restart)
    ism_time_t   LastConnectedTime;    ///< The time at which a disconnected client was last connected
    ism_time_t   ExpiryTime;           ///< The time at which this client expires (0 for no expiry)
    ism_time_t   WillDelayExpiryTime;  ///< The time at which the will delay expires (0 for no delay, or no will msg)
    const char * ResourceSetId;        ///< The resourceSet to which this client belongs (if any)
} ismEngine_ClientStateMonitor_t;

//*********************************************************************
/// @brief  Transaction monitor
///
/// Represents the monitoring information for a transaction.
//*********************************************************************
typedef struct tag_ismEngine_TransactionMonitor_t
{
    ism_xid_t XID;               ///< The XID for this transaction
    ismTransactionState_t state; ///< The state of this transaction
    ism_time_t stateChangedTime; ///< The time at which transaction changed to this state
} ismEngine_TransactionMonitor_t;

/**@}*/

//****************************************************************************
/// @brief Subscription Identifier
//****************************************************************************
typedef uint32_t ismEngine_SubId_t;

#define ismENGINE_NO_SUBID     0     ///< No SubId is being supplied

//*********************************************************************
/// @brief  Subscription attributes
//*********************************************************************
typedef struct tag_ismEngine_SubscriptionAttributes_t
{
    uint32_t subOptions;     ///< ismENGINE_SUBSCRIPTION_OPTION_* values
    ismEngine_SubId_t subId; ///< Subscription Identifier for this subscription (ismENGINE_NO_SUBID for none)
} ismEngine_SubscriptionAttributes_t;

/** @name Subscription options
 * Options that can be specified in the ismEngine_SubscriptionAttributes_t subOptions field
 */

/**@{*/

/** @brief
 *  No options. The subscription does not specify the reliability
 *  with which it will deliver messages.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_NONE                            0x0000

/** @brief
 *  The subscription can only deliver messages at most once. Messages can be
 *  discarded on error. The subscription is not properly transaction-capable.
 *  This is suitable for MQTT QoS 0, but not for JMS.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE                    0x0001

/** @brief
 *  The subscription can deliver messages at least once. The subscription
 *  is not properly transaction-capable. This is suitable for MQTT QoS 1,
 *  but not for JMS.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE                   0x0002

/** @brief
 *  The subscription can deliver messages exactly once. The subscription
 *  is not properly transaction-capable. This is suitable for MQTT QoS 2,
 *  but not for JMS.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE                    0x0003

/** @brief
 *  The subscription can deliver messages transactionally.
 *  This is suitable for JMS.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE             0x0004

/** @brief
 *  Mask of options which control the reliability of the subscription.
 *  A combination of at-most-once, at-least-once, exactly-once and
 *  transaction-capable.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK                   0x0007

/** @brief
 * The subscription must not receive messages published from the same client
 * id. This is the engine implementation of the JMS NoLocal flag. The client
 * from which the subscription is being made must specify a non-null client id
 * which is then checked against the client id of the publisher, the subscribe
 * will fail with ISMRC_ClientIDRequired if this is not the case.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL                        0x0008

/** @brief
 * Message selection is being used for this subscription. The "Selector"
 * property gives the compiled selector which the Engine will pass to the
 * message-selection callback.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION               0x0010

/** @brief
 * Subscription is required to be durable, i.e. survive a restart of the
 * server.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_DURABLE                         0x0020

/** @brief
 * Subscription is intended to be shared between multiple consumers.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_SHARED                          0x0040

/** @brief
 * The request specifying this subOptions value comes from a protocol that expects
 * the subscription to be considered in use from the specified client as well as by any
 * consumer that client might create later.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT                      0x0080

/** @brief
 * The subscription is a shared one that allows for mixed durability on its consumers
 * (which implies we need to persist it for durable consumers)
 */
#define ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY         0x0100

/** @brief
 * When the subscription is created, do *not* deliver any retained messages that would
 * match the associated topic filter. By default, retained messages are added to the
 * subscription at creation time.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_NO_RETAINED_MSGS                0x0200

/** @brief
 * The subscription should only have unreliable (e.g. QoS=0) messages published to it.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY            0x0400

/** @brief
 * The subscription should only have reliable (e.g. QoS>0) messages published to it.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY              0x0800

/** @brief
 * Ignored by the engine - this is an indication to the caller about whether to
 * use the ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN message flag to indicate this was a retained
 * message or the more common ismMESSAGE_FLAGS_RETAINED.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_DELIVER_RETAIN_AS_PUBLISHED     0x1000

/** @brief
 * Mask of all options that represent selection being explicitly performed by
 * this subscription
 */
#define ismENGINE_SUBSCRIPTION_OPTION_SELECTION_MASK (ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION |\
                                                      ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL |\
                                                      ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY |\
                                                      ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY)

/** @brief
 * Mask of the options which represent QoS filtering.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_QOSFILTER_MASK (ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY |\
                                                      ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY)

/** @brief
 * Mask of the options which should be persisted (both in-memory and in the store) in
 * the overall subscription subOptions (i.e. not including those for a specific client)
 */
#define ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK (ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |\
                                                       ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |\
                                                       ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |\
                                                       ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL |\
                                                       ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION |\
                                                       ismENGINE_SUBSCRIPTION_OPTION_DURABLE |\
                                                       ismENGINE_SUBSCRIPTION_OPTION_SHARED |\
                                                       ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY |\
                                                       ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY |\
                                                       ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY |\
                                                       ismENGINE_SUBSCRIPTION_OPTION_DELIVER_RETAIN_AS_PUBLISHED)

/** @brief
 * Mask of the options which we allow to be altered on a subscription.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_ALTERABLE_MASK (ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL |\
                                                      ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY |\
                                                      ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY |\
                                                      ismENGINE_SUBSCRIPTION_OPTION_DELIVER_RETAIN_AS_PUBLISHED)

/** @brief
 * Mask of the options which should be persisted (both in-memory and in the store) for
 * a specific sharing client.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_SHARING_CLIENT_PERSISTENT_MASK  (ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |\
                                                                       ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |\
                                                                       ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |\
                                                                       ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL |\
                                                                       ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION |\
                                                                       ismENGINE_SUBSCRIPTION_OPTION_DURABLE |\
                                                                       ismENGINE_SUBSCRIPTION_OPTION_SHARED |\
                                                                       ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT |\
                                                                       ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY |\
                                                                       ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY |\
                                                                       ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY |\
                                                                       ismENGINE_SUBSCRIPTION_OPTION_DELIVER_RETAIN_AS_PUBLISHED)

/** @brief
 * Mask of the options which we allow to be altered for a specific sharing client.
 */
#define ismENGINE_SUBSCRIPTION_OPTION_SHARING_CLIENT_ALTERABLE_MASK (ismENGINE_SUBSCRIPTION_OPTION_DELIVER_RETAIN_AS_PUBLISHED)

/** @brief
 * Mask of all valid options used for validity checks
 */
#define ismENGINE_SUBSCRIPTION_OPTION_VALID_MASK                      0x1FFF

/**@}*/

/** @brief
 * Prefix for all shared subscription namespaces
 */
#define ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_PREFIX      "__Shared"

/** @brief
 * NameSpace (owning ClientID) for durable-only shared subscriptions
 * (Note: This is identical to the prefix for backwards compatibility, all
 *        other shared namespaces will have characters following the prefix)
 */
#define ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE             ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_PREFIX

/** @brief
 * NameSpace (owning ClientID) for nondurable-only shared subscriptions.
 */
#define ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_NONDURABLE  ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_PREFIX"ND"

/** @brief
 * NameSpace (owning ClientID) for mixed durability shared subscriptions
 */
#define ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED       ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_PREFIX"M"

//****************************************************************************
/// @brief Debug Handle
//****************************************************************************
typedef void *ismEngine_DebugHdl_t;

//****************************************************************************
/// @brief Debug options
//****************************************************************************
#define ismDEBUGOPT_NONE        0
#define ismDEBUGOPT_STDOUT      0x00000001 ///< Request output to stdout
#define ismDEBUGOPT_TRACE       0x00000002 ///< Request output to trace
#define ismDEBUGOPT_NAMEDFILE   0x00000004 ///< Request output to a named file
#define ismDEBUGOPT_VERBOSE     0xf0000000 ///< Request verbose output

#define ismDEBUGVERBOSITY(value) ((value & 0x0000000f) << 28)

typedef uint32_t ismEngine_DebugOptions_t;

//****************************************************************************
/** @name Engine handle types
 * These are pointers to internal structures which are internal to the component.
 */
//****************************************************************************

/**@{*/

/// Client-state handle
typedef struct ismEngine_ClientState_t  * ismEngine_ClientStateHandle_t;

/// Session handle
typedef struct ismEngine_Session_t      * ismEngine_SessionHandle_t;

/// Message-producer handle
typedef struct ismEngine_Producer_t     * ismEngine_ProducerHandle_t;

/// Message-consumer handle
typedef struct ismEngine_Consumer_t     * ismEngine_ConsumerHandle_t;

/// Subscription handle
typedef struct ismEngine_Subscription_t * ismEngine_SubscriptionHandle_t;

/// Transaction handle
typedef struct ismEngine_Transaction_t  * ismEngine_TransactionHandle_t;

/// Message handle
typedef struct ismEngine_Message_t      * ismEngine_MessageHandle_t;

#define ismENGINE_NULL_MESSAGE_HANDLE     NULL

/// Delivery handle
typedef __uint128_t                       ismEngine_DeliveryHandle_t;

#define ismENGINE_NULL_DELIVERY_HANDLE    0

/// Unreleased-message handle
typedef struct ismEngine_Unreleased_t   * ismEngine_UnreleasedHandle_t;

/// Queue-manager record handle
typedef void                            * ismEngine_QManagerRecordHandle_t;

/// Queue-manager XID record handle
typedef void                            * ismEngine_QMgrXidRecordHandle_t;

/**@}*/

//*********************************************************************
/// @brief DelivererContext
///
/// Context for when delivering messages onto a queue
/// allows for lock optimizations
//*********************************************************************
typedef struct ismEngine_DelivererContext_t {
    ismMessageSelectionLockStrategy_t lockStrategy;
} ismEngine_DelivererContext_t;

//****************************************************************************
/** @name Callback functions
 * Functions called by the Engine.
 */
//****************************************************************************

/**@{*/

//****************************************************************************
/// @brief  Operation-completion callback
///
/// Called upon asynchronous completion of an Engine operation.
///
/// @param[in]     retcode          Return code indicating success of the operation
/// @param[in]     handle           For operations that create a resource, its handle
/// @param[in]     pContext         Optional context for completion callback
///
/// @remark The callback can call other Engine operations, bearing in mind
/// that the stack must be allowed to unwind.
//****************************************************************************
typedef void (*ismEngine_CompletionCallback_t)(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext);

//****************************************************************************
/// @brief  Message-delivery callback
///
/// Callback used to deliver messages to a message consumer.
///
/// @param[in]     hConsumer        Handle of the consumer
/// @param[in]     hDelivery        Handle of the delivery to this consumer
/// @param[in]     hMessage         Handle of the message
/// @param[in]     deliveryId       A numeric identifier used by a protocol to
///                                 confirm delivery of a message. This can be used
///                                 as an MQTT message ID, for example.
/// @param[in]     state            State of the message
/// @param[in]     destinationOptions  Options used when creating the consumer.
///                                 This can be used to carry the MQTT QoS.
/// @param[in]     pMsgHeader       Pointer to message header
/// @param[in]     msgAreaCount     Number of message areas - may be 0
/// @param[in]     areaTypes[]      Types of message areas - no duplicates
/// @param[in]     areaLengths[]    Array of area lengths - some may be 0
/// @param[in]     pAreaData[]      Pointers to area data - some may be NULL
/// @param[in]     pConsumerContext Consumer context
/// @param[in]     delivererContext Context from the deliverer that allows lock optimizations
///
/// @return The callback controls the delivery of further messages.
/// If the callback returns true and message delivery has not been stopped
/// further messages may be delivered. If the callback returns false
/// or message delivery has been stopped, no further messages will be
/// delivered to this consumer until message delivery is resumed.
///
/// If the session was created using ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
/// ism_engine_suspendMessageDelivery must be in addition to returning false
/// to disable the consumer and must NOT have be called if the callback
/// returns true
///
/// The delivererContext is not guarenteed to be passed all the way to delivery. 
/// It is to hold state for cases where multiple messages are processed as a 
/// batch in a loop where actions within the loop could cause performance
/// problems if knowledge is not passed itterations of the loop.
///
/// @remark The callback can call other Engine operations, bearing in mind
/// that the stack must be allowed to unwind.
//****************************************************************************
typedef bool (*ismEngine_MessageCallback_t)(
    ismEngine_ConsumerHandle_t      hConsumer,
    ismEngine_DeliveryHandle_t      hDelivery,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        deliveryId,
    ismMessageState_t               state,
    uint32_t                        destinationOptions,
    ismMessageHeader_t *            pMsgDetails,
    uint8_t                         msgAreaCount,
    ismMessageAreaType_t            areaTypes[msgAreaCount],
    size_t                          areaLengths[msgAreaCount],
    void *                          pAreaData[msgAreaCount],
    void *                          pConsumerContext,
    ismEngine_DelivererContext_t *  delivererContext);


//****************************************************************************
/// @brief  Subscription callback
///
/// Called by ism_engine_listSubscriptions to process a list
/// of named subscriptions.
///
/// @param[in]     subHandle        Handle of the described subscription
/// @param[in]     pSubName         Subscription name
/// @param[in]     properties       Ptr to the subscription properties
///                                 (list of null terminated strings)
/// @param[in]     propertiesLength Length of the subscription properties
/// @param[in]     pSubAttributes   Ptr to the attributes of this subscription
///                                 associated with the requesting client
/// @param[in]     consumerCount    Current number of consumers from this
///                                 subscription
/// @param[in]     pContext         Optional context
///
/// @remark The callback can call other Engine operations, bearing in mind
/// that the stack must be allowed to unwind.
///
/// The provided subHandle can only be referred to within the context of this
/// callback. The subscription is use counted. The use count is incremented
/// before the callback is called and decremented when the callback returns
/// at which point, if it hits zero, the subscription may be destroyed.
//****************************************************************************
typedef void (*ismEngine_SubscriptionCallback_t)(
    ismEngine_SubscriptionHandle_t             subHandle,
    const char *                               pSubName,
    const char *                               pTopicString,
    void *                                     properties,
    size_t                                     propertiesLength,
    const ismEngine_SubscriptionAttributes_t * pSubAttributes,
    uint32_t                                   consumerCount,
    void *                                     pContext);


//****************************************************************************
/// @brief  Unreleased-message callback
///
/// Called by ism_engine_listUnreleasedDeliveryIDs to process a list
/// of unreleased messages.
///
/// @param[in]     deliveryId       Delivery ID
/// @param[in]     hUnrel           Unreleased-message handle
/// @param[in]     pContext         Optional context
///
/// @remark The callback MUST NOT call any Engine operations.
//****************************************************************************
typedef void (*ismEngine_UnreleasedCallback_t)(
    uint32_t                        deliveryId,
    ismEngine_UnreleasedHandle_t    hUnrel,
    void *                          pContext);


//****************************************************************************
/// @brief  Client-state steal callback
///
/// Called by the Engine to indicate that a client-state has been stolen.
/// The client-state handle supplied should be destroyed in an orderly manner
/// to permit completion of the creation of the stealing client-state.
///
/// @param[in]     reason           The reason for the steal callback being called
///                                 as an ISMRC_ return code.
/// @param[in]     hClient          Client-state handle which has been stolen
/// @param[in]     options          ismENGINE_STEAL_CALLBACK_OPTION_* flags
/// @param[in]     pContext         Optional context supplied when client-state created
///
/// @remark The callback can call other Engine operations, bearing in mind
/// that the stack must be allowed to unwind.
//****************************************************************************
typedef void (*ismEngine_StealCallback_t)(
    int32_t                         reason,
    ismEngine_ClientStateHandle_t   hClient,
    uint32_t                        options,
    void *                          pContext);

/** @name Steal callback options
 *  Options passed to the steal callback function.
 */

/**@{*/

/** @brief
 *  No options.
 */
#define ismENGINE_STEAL_CALLBACK_OPTION_NONE               0x00000000

/**@}*/

//****************************************************************************
/// @brief  Queue-manager record callback
///
/// Called by ism_engine_listQManagerRecords to process a list
/// of queue-manager records.
///
/// @param[in]     pData            Ptr to queue-manager record's data
/// @param[in]     dataLength       Length of queue-manager record's data
/// @param[in]     hQMgrRec         Handle of queue-manager record
/// @param[in]     pContext         Optional context
///
/// @remark The callback MUST NOT call any Engine operations. The pointer to
/// the queue-manager record's data is only valid for the duration of the callback.
//****************************************************************************
typedef void (*ismEngine_QManagerRecordCallback_t)(
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QManagerRecordHandle_t    hQMgrRec,
    void *                              pContext);


//****************************************************************************
/// @brief  Queue-manager XID record callback
///
/// Called by ism_engine_listQMgrXidRecords to process a list
/// of queue-manager XID records.
///
/// @param[in]     pData            Ptr to queue-manager XID record's data
/// @param[in]     dataLength       Length of queue-manager XID record's data
/// @param[in]     hQMgrXidRec      Handle of queue-manager XID record
/// @param[in]     pContext         Optional context
///
/// @remark The callback MUST NOT call any Engine operations. The pointer to
/// the queue-manager XID record's data is only valid for the duration of the
/// callback.
//****************************************************************************
typedef void (*ismEngine_QMgrXidRecordCallback_t)(
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QMgrXidRecordHandle_t     hQMgrXidRec,
    void *                              pContext);


//****************************************************************************
/// @brief  Message selection callback
///
/// Called when publishing a message to a subscription which has a selection
/// string or prior to consuming a message when the consumer has a selection
/// string.
///
/// @param[in]     pMsgDetails      Message Header
/// @param[in]     areaCount        Number of Payload areas
/// @param[in]     areaTypes        Type of each payload area
/// @param[in]     areaLengths      Size of each payload area
/// @param[in]     pAreaData        Payload areas
/// @param[in]     pTopic           Topic of message (NULL if unknown / none)
/// @param[in]     pSelectorRule    Compiled selector
/// @param[in]     selectorRuleLen  Size of compiled selector
///
/// @return (defined in selector.h)
///    SELECT_TRUE - message matches selection string
///    SELECT_FALSE - message does not match selection string
///    SELECT_UNKNOWN - selection did not complete
/// @remark The callback MUST NOT call any Engine operations. 
//****************************************************************************
typedef int32_t (*ismEngine_MessageSelectionCallback_t) (
    ismMessageHeader_t *            pMsgDetails,
    uint8_t                         msgAreaCount,
    ismMessageAreaType_t            areaTypes[msgAreaCount],
    size_t                          areaLengths[msgAreaCount],
    void *                          pAreaData[msgAreaCount],
    const char *                    pTopic,
    const void *                    pSelectorRule,
    size_t                          selectorRuleLen,
    ismMessageSelectionLockStrategy_t * lockStrategy);


//****************************************************************************
/// @brief  Delivery Failure callback
///
/// Called when the engine is unable to delivery a message it ought to have
/// due to resource constraints (e.g. memory).
///
/// The callback is expected to disconnect the affected client so that we do not
/// appear to have gone strangely quiet
///
/// @param[in]    rc        - indicates cause of the problem. Current rc code(s) are:
///                            ISMRC_AllocateError - We ran out of memory
/// @param[in]    hClient   - client that owns the affected consumer
/// @param[in]    hConsumer - consumer we disabled because we could not service
/// @param[in]    consumerContext - context we were given during consumer creation
///
//****************************************************************************
typedef void (*ismEngine_DeliveryFailureCallback_t)(
    int32_t                         rc,
    ismEngine_ClientStateHandle_t   hClient,
    ismEngine_ConsumerHandle_t      hConsumer,
    void *                          consumerContext);


//****************************************************************************
/// @brief  Retained message forwarding request callback
///
/// Called when requesting the forwarding of retained messages from another
/// server in a cluster.
///
/// @param[in] respondingServerUID  The UID of the server to respond to the request
/// @param[in] originServerUID      The UID of the retained message origin server
/// @param[in] options              Options to pass to the remote ism_engine_call
/// @param[in] timestamp            Timestamp to use in selecting retained messages
/// @param[in] correlationId        Identifier to correlate this request with the
///                                 request being serviced on the responding server.
///
/// @remark The requesting server (i.e. the server to which the retained messages
/// should be forwarded) is implicitly this (the calling) server.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
typedef int32_t (*ismEngine_RetainedForwardingCallback_t) (
    const char *respondingServerUID,
    const char *originServerUID,
    uint32_t options,
    ism_time_t timestamp,
    uint64_t correlationId);

/**@}*/


//****************************************************************************
/// @brief  Engine component initialization
///
/// Performs Engine initialization tasks, such as reading configuration
/// parameters.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_init(void);


//****************************************************************************
/// @brief  Engine component start
///
/// Performs restart recovery and reconciliation with the server configuration.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_start(void);


//****************************************************************************
/// @brief  Engine component shutdown
///
/// Shuts down the Engine.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_term(void);


//****************************************************************************
/// @internal
///
/// @brief  Engine component start messaging
///
/// Performs final recovery actions to enable messaging to start
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_startMessaging(void);


//****************************************************************************
/// @brief  Engine component thread initialization
///
/// The Engine component requires some initialisation for each thread
/// before it can make engine calls. This function performs this
/// initialisation and MUST be called before a thread makes Engine
/// calls. This function may be called repeatedly.
///
/// @remark The call to ism_engine_threadInit can be made any time after
/// ism_engine_init, however engine calls on the initialised thread can only
/// be made after messaging has started - the engine does not fully initialise
/// the per-thread data until the engine has been started (more specifically
/// when the store is able to accept a request to start a store stream for the
/// thread).
///
/// @param[in]     isStoreCrit      Indicates whether Store performance is critical
///
/// @remark The special value ISM_ENGINE_NO_STORE_STREAM can be specified for
/// isStoreCrit to indicate that no store stream will be opened for this thread,
/// this value should only be used by certain store threads that know it is
/// safe to do so.
//****************************************************************************
XAPI void ism_engine_threadInit(uint8_t isStoreCrit);


//****************************************************************************
/// @brief  Engine component thread termination
///
/// Frees Engine thread-specific data. This function may be called repeatedly.
///
/// @param[in]    closeStoreStream   Indicates whether the store stream associated
///                                  with this thread should be closed
///
/// @remark Ordinarily (and for most threads) the closeStoreStream flag should be
/// 1. However for some threads (most notably store persistence threads) this
/// will be 0 because the store has already terminated.
///
/// @return Almost certainly.
//****************************************************************************
XAPI void ism_engine_threadTerm(uint8_t closeStoreStream);

//****************************************************************************
/// @brief  Register selection callback
///
/// In order to perform messages selection, the engine will invoked a 
/// callback function provided by the protocol component. This function
/// is called by the protocol layer to register the function to be 
/// called to perform message selection.
///
/// @param[in]     pSelectionFn     Message-selection callback
//****************************************************************************
XAPI void ism_engine_registerSelectionCallback(
    ismEngine_MessageSelectionCallback_t pSelectionFn);


//****************************************************************************
/// @brief  Register delivery failure callback
///
/// If the engine is unable to deliver messages to a consumer due to resource
/// shortages, this function is called.
///
/// @param[in]     pFailCallbackFn  Delivery failure callback
//****************************************************************************
XAPI void ism_engine_registerDeliveryFailureCallback(
    ismEngine_DeliveryFailureCallback_t pFailCallbackFn);


//****************************************************************************
/// @brief  Register retained forwarding request callback
///
/// In order to request retained messages to be forwarded from another server,
/// the engine will invoke a callback function provided by the protocol
/// component. This function is called by the protocol layer to register the
/// function to be called to perform retained message forwarding.
///
/// @param[in]     pRetainedForwardingFn   Retained forwarding function
//****************************************************************************
XAPI void ism_engine_registerRetainedForwardingCallback(
    ismEngine_RetainedForwardingCallback_t pRetainedForwardingFn);


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
/// @param[in]     pClientId        Client ID  (must be a non-empty string)
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
    ismEngine_CompletionCallback_t  pCallbackFn);

/** @name Create client-state options
 * Options for the ism_engine_createClientState function.
 */

/**@{*/

/** @brief
 *  No options.
 */
#define ismENGINE_CREATE_CLIENT_OPTION_NONE                0x00000000

/** @brief
 *  The client-state must be durable so reconnection can re-associate with it.
 *  Durable state includes durable subscriptions and message-delivery
 *  information for persistent messages.
 */
#define ismENGINE_CREATE_CLIENT_OPTION_DURABLE             0x00000001

/** @brief
 *  If the client ID matches an active client-state, the existing client-state
 *  and dependent resources are destroyed, and a new client-state is created
 *  with the same client ID. This is used for reconnection which asserts
 *  ownership of the client ID, regardless of existing connections.
 */
#define ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL      0x00000002

/** @brief
 *  If the client ID matches an active client-state, the operation to create
 *  a new client-state bounces off.
 */
#define ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE     0x00000000

/** @brief
 *  The trace level for calls using resources associated with this client-state
 *  is increased. This provides a way for particular connections to be flagged
 *  for extra tracing.
 */
#define ismENGINE_CREATE_CLIENT_OPTION_ENHANCED_TRACE      0x00000004

/** @brief
 *  The client-state must not take over from an existing client state.
 */
#define ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART          0x00000008

/** @brief
 *  The client-state inherits the durability of a client which it either
 *  steals from or resumes. If the client-state doesn't inherit from an
 *  existing clientState, durability is based on the setting of
 *  ismENGINE_CREATE_CLIENT_OPTION_DURABLE.
 */
#define ismENGINE_CREATE_CLIENT_OPTION_INHERIT_DURABILITY  0x00000010

/** @brief
 *  The client-state can only be stolen if the userID of the new and stolen
 *  client states are the same.  This is only meaningful if combined with
 *  ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL .
 */
#define ismENGINE_CREATE_CLIENT_OPTION_CHECK_USER_STEAL    0x00000020

/** @brief
 *  Internal use only
 *  The client is being created (internally) as part of an import resources
 *  request.
 */
#define ismENGINE_CREATE_CLIENT_INTERNAL_OPTION_IMPORTING  0x80000000

/**@}*/


//****************************************************************************
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
XAPI int32_t WARN_CHECKRC ism_engine_destroyClientState(
    ismEngine_ClientStateHandle_t   hClient,
    uint32_t                        options,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

/** @name Destroy client-state options
 * Options for the ism_engine_destroyClientState function.
 */

/**@{*/

/** @brief
 *  No options.
 */
#define ismENGINE_DESTROY_CLIENT_OPTION_NONE    0x00

/** @brief
 *  Discards all information for the client-state, including durable
 *  subscriptions and message-delivery information for persistent messages.
 */
#define ismENGINE_DESTROY_CLIENT_OPTION_DISCARD 0x01

/**@}*/


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
        ismEngine_CompletionCallback_t pCallbackFn);


//****************************************************************************
/// @brief  Set Will Message
///
/// Sets the will message for a client-state. The will message is published
/// when the client-state is destroyed.
///
/// @param[in]     hClient          Client-state handle
/// @param[in]     destinationType  Destination type
/// @param[in]     pDestinationName Destination name
/// @param[in]     hMessage         Will message handle
/// @param[in]     delayInterval    Delay (in seconds) before will message is published
/// @param[in]     timeToLive       Time to live (in seconds) of the will message
///                                 (0 for no limit)
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
XAPI int32_t WARN_CHECKRC ism_engine_setWillMessage(
    ismEngine_ClientStateHandle_t   hClient,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        delayInterval,
    uint32_t                        timeToLive,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
XAPI int32_t WARN_CHECKRC ism_engine_unsetWillMessage(
    ismEngine_ClientStateHandle_t   hClient,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
/// @brief  Create Session
///
/// Creates a session handle which represents a session between a
/// client program and the messaging server.
///
/// @param[in]     hClient          Client-state handle
/// @param[in]     options          ismENGINE_CREATE_SESSION_OPTION_* values
/// @param[out]    phSession        Returned session handle
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
///
/// On success, for synchronous completion, the session handle
/// will be returned in an output parameter, while, for asynchronous
/// completion, the session handle will be passed to the
/// operation-completion callback.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createSession(
    ismEngine_ClientStateHandle_t   hClient,
    uint32_t                        options,
    ismEngine_SessionHandle_t *     phSession,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

/** @name Session options
 * Options for the ism_engine_createSession function.
 */

/**@{*/

/** @brief
 *  No options.
 */
#define ismENGINE_CREATE_SESSION_OPTION_NONE      0x00

/** @brief
 *  This session will just contain browsers - no transactions, acking etc...
 */
#define ismENGINE_CREATE_SESSION_BROWSE_ONLY      0x01

/** @brief
 *  This session will use transactions
 */
#define ismENGINE_CREATE_SESSION_TRANSACTIONAL    0x02

/** @brief
 *  Message delivery for consumers is suspended explicitly
 */
#define ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND 0x04

/**@}*/


//****************************************************************************
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
XAPI int32_t WARN_CHECKRC ism_engine_destroySession(
    ismEngine_SessionHandle_t       hSession,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
XAPI int32_t WARN_CHECKRC ism_engine_startMessageDelivery(
    ismEngine_SessionHandle_t       hSession,
    uint32_t                        options,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

/** @name Start delivery options
 * Options for the ism_engine_startMessageDelivery function.
 */

/**@{*/

/// No options
#define ismENGINE_START_DELIVERY_OPTION_NONE                0x00

//Internal options for engine use only
#define ismENGINE_START_DELIVERY_OPTION_ENGINE_START        0x100000

/**@}*/


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
///
/// @remark The operation may complete synchronously or asynchronously. If it
/// completes synchronously, the return code indicates the completion
/// status. If the operation completes synchronously, it is possible for a few
/// more messages to be delivered before delivery finally stops.
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
XAPI int32_t WARN_CHECKRC ism_engine_stopMessageDelivery(
    ismEngine_SessionHandle_t       hSession,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


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
    ismEngine_CompletionCallback_t  pCallbackFn);

/** @name Resume delivery options
 * Options for the ism_engine_resumeMessageDelivery function.
 */

/**@{*/

/// No options
#define ismENGINE_RESUME_DELIVERY_OPTION_NONE    0x00

/**@}*/


//****************************************************************************
/// @brief  Suspend Consumer Message Delivery
///
/// Suspends message delivery for an individual consumer. This operation
/// MUST ONLY BE USED in a message delivery callback.
///
/// For a session created using ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
/// the return code from the message delivery callback is ignored and this
/// operation used instead to suspend message delivery.
///
/// @param[in]     hConsumer        Consumer handle
/// @param[in]     options          Options from ismENGINE_SUSPEND_DELIVERY_OPTION_*
///
/// @return Nothing
///
/// @remark If the consumer has been destroyed while delivery is being suspended,
/// suspending delivery will complete its destruction and the consumer handle
/// and consumer context will no longer be addressable.
//****************************************************************************
XAPI void ism_engine_suspendMessageDelivery(
    ismEngine_ConsumerHandle_t      hConsumer,
    uint32_t                        options);

/** @name Suspend delivery options
 * Options for the ism_engine_suspendMessageDelivery function.
 */

/**@{*/

/// No options
#define ismENGINE_SUSPEND_DELIVERY_OPTION_NONE    0x00

/**@}*/


//****************************************************************************
/// @brief  Get Consumer Message Delivery Status
///
/// Gets the message delivery status for an individual consumer.
///
/// @param[in]     hConsumer        Consumer handle
/// @param[out]    pStatus          Message delivery status for this consumer
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI void ism_engine_getConsumerMessageDeliveryStatus(
    ismEngine_ConsumerHandle_t      hConsumer,
    ismMessageDeliveryStatus_t *    pStatus);


//****************************************************************************
/// @brief  Create Local Transaction
///
/// Creates a local transaction associated with the provided
/// session handle. The transaction can be used to group operations
/// into an atomic unit of work. A local transaction follows the
/// chained transaction model, meaning that once committed or rolled
/// back, the local transaction handle can be used again.
///
/// @param[in]     hSession         Session handle
/// @param[out]    phTran           Returned transaction handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark If no session is specified, the transaction is not associated with
///         a session.
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
/// On success, for synchronous completion, the transaction handle
/// will be returned in an output parameter, while, for asynchronous
/// completion, the transaction handle will be passed to the
/// operation-completion callback.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createLocalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t * phTran,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
/// On success, for synchronous completion, the transaction handle
/// will be returned in an output parameter, while, for asynchronous
/// completion, the transaction handle will be passed to the
/// operation-completion callback.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createGlobalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t                     * pXID,
    uint32_t                        options,
    ismEngine_TransactionHandle_t * phTran,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

/** @name Create global transaction options
 * Options for the ism_engine_createGlobalTransaction function
 */

/**@{*/

/// Default behaviour (Automatically create or resume the requested transaction)
#define ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT       0x00000001

/// Request XA functionality provided by TMNOFLAGS (create only)
#define ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS  0x00000000

/// Request XA functionality provided by TMRESUME (resume only)
#define ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMRESUME   0x08000000

/**@}*/


///***************************************************************************
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
uint32_t WARN_CHECKRC ism_engine_endTransaction(
     ismEngine_SessionHandle_t       hSession,
     ismEngine_TransactionHandle_t   hTran,
     uint32_t                        options,
     void *                          pContext,
     size_t                          contextLength,
     ismEngine_CompletionCallback_t  pCallbackFn);

/** @name End global transaction options
 * Options for the ism_engine_endGlobalTransaction function.
 */

/**@{*/

/// Default behaviour - the transaction moves from Active to Idle state - no longer associated
#define ismENGINE_END_TRANSACTION_OPTION_DEFAULT       0x00000000

/// Request XA functionality provided by TMSUCCESS (default)
#define ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS  0x04000000

/// Request XA functionality provided by TMFAIL
/// Rollback-only - the transaction moves from Active to Rollback-Only state - no longer associated
#define ismENGINE_END_TRANSACTION_OPTION_XA_TMFAIL     0x20000000

/// Request XA functionality provided by TMSUSPEND
/// Suspend the transaction - the transaction remains in Active state - association suspended
#define ismENGINE_END_TRANSACTION_OPTION_XA_TMSUSPEND  0x02000000

/**@}*/


//****************************************************************************
/// @brief  Commit Transaction
///
/// Commits the transaction with specified transaction handle.
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
/// @remark The operation may complete synchronously or asynchronously. If it
/// completes synchronously, the return code indicates the completion
/// status. If the operation completes asynchronously, the transaction
/// synchronously moves into "committing" state and further operations cannot
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
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_commitTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    uint32_t                        options,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

/** @name Commit transaction options
 * Options for the ism_engine_commitTransaction function.
 */

/**@{*/

/// Default behaviour - commit the transaction, if it's global, it must be prepared
#define ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT        0x00000000

/// Request XA functionality provided by TMNOFLAGS (default)
#define ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS   0x00000000

/// Request XA functionality provided by TMONEPHASE
/// Commit the transaction, if it's global, do NOT require it to be prepared
#define ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMONEPHASE  0x40000000

/**@}*/

//****************************************************************************
/// @brief  Commit a global transaction
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
/// @remark The operation may complete synchronously or asynchronously. If it
/// completes synchronously, the return code indicates the completion
/// status. If the operation completes asynchronously, the transaction
/// synchronously moves into "committing" state and further operations cannot
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
/// @see ism_engine_commitTransaction
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_commitGlobalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t *                     pXID,
    uint32_t                        options,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_rollbackTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
/// @see ism_engine_rollbackTransaction
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_rollbackGlobalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t *                     pXID,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


///***************************************************************************
/// @brief  Prepare Transaction
///
/// Prepares the global transaction with specified handle.
///
/// Upon successful completion we guarantee to be able to commit the global
/// transaction.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hTran            Transaction handle
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
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
///***************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_prepareTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


///***************************************************************************
/// @brief  Prepare Global transaction
///
/// Prepares the global transaction with specified XID.
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
///***************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_prepareGlobalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t *                     pXID,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


///***************************************************************************
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
    ismEngine_CompletionCallback_t  pCallbackFn);


///***************************************************************************
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
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
/// @brief  Recover list of prepared or heuristically completed transactions
///
/// Return to the caller a list of prepared or heuristically completed
/// transactions.
///
/// This function is equivalent to the xa_recover verb, and returns a list of
/// the prepared and heuristically completed transactions.
///
/// @param[in]     hSession         Session handle
/// @param[in]     pXIDArray        Array in which XIDs are placed
/// @param[in]     arraySize        Maximum number of XIDs to be return
/// @param[in]     rmId             resource manager id (ignored)
/// @param[in]     flags            ismENGINE_XARECOVER_OPTION_ values - see below
///
/// @return Number of XIDs in the array.
///
/// Where the number of prepared transactions exceeds the size of the buffer
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
/// for the same session.
//****************************************************************************
XAPI uint32_t WARN_CHECKRC ism_engine_XARecover(
    ismEngine_SessionHandle_t       hSession,
    ism_xid_t *                     pXIDArray,
    uint32_t                        arraySize,
    uint32_t                        rmId,
    uint32_t                        flags);

/** @name XA Recover options
 * Flags for the ism_engine_XARecover function.
 */

/**@{*/

/// Start a recovery scan for the session (equivalent to TMSTARTRSCAN)
#define ismENGINE_XARECOVER_OPTION_XA_TMSTARTRSCAN  0x01000000

/// Continue an existing recovery scan for the session (TMNOFLAGS)
/// Note: If one does not already exist, a recovery scan is started
#define ismENGINE_XARECOVER_OPTION_XA_TMNOFLAGS     0x00000000

/// End the recovery scan for the session (equivalent to TMENDRSCAN)
#define ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN    0x00800000

/**@}*/


//****************************************************************************
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
XAPI int32_t WARN_CHECKRC ism_engine_createProducer(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_ProducerHandle_t *    phProducer,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
XAPI int32_t WARN_CHECKRC ism_engine_destroyProducer(
    ismEngine_ProducerHandle_t      hProducer,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
///
/// @remark Several informational return codes can be returned by this
/// function indicating that the request was successful, but reporting
/// something that may be of interest to the caller:
///
/// ISMRC_SomeDestinationsFull: Some of the recipient destinations were
///   full and so did not receive the message, but either the recipient
///   or message are low QoS.
/// ISMRC_NoMatchingDestinations: There were no matching recipient
///   destinations (which might mean no subscribers, or they had selection
///   defined and so did not get the message).
/// ISMRC_NoMatchingLocalDestinations: There were no matching local
///   recipients but the message was forwarded to a remote server, meaning
///   it MAY be given to recipients on another server.
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
XAPI int32_t WARN_CHECKRC ism_engine_putMessage(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_ProducerHandle_t      hProducer,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
///
/// @remark Several informational return codes can be returned by this
/// function. For more details, see the remarks for ism_engine_putMessage on
/// informational return codes.
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
XAPI int32_t WARN_CHECKRC ism_engine_putMessageOnDestination(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
///
/// @remark Several informational return codes can be returned by this
/// function. For more details, see the remarks for ism_engine_putMessage on
/// informational return codes.
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
XAPI int32_t WARN_CHECKRC ism_engine_putMessageWithDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_ProducerHandle_t      hProducer,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
///
/// @remark Several informational return codes can be returned by this
/// function. For more details, see the remarks for ism_engine_putMessage on
/// informational return codes.
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
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
/// @remark Unlike other ism_engine_putXXX interfaces, this function does not
/// return informational return codes (e.g. because a destination was full)
/// -- OK does not indicate that the message was put onto any destination,
/// but rather that nothing failed.
///
/// @remark This function is intended to allow a simple publish of a non-persistent
/// QoS 0 message without the need for a client / session. As a result
/// there are several restrictions on it's use - these are not enforced,
/// but asserted.
///
///   destinationType must be ismDESTINATION_TOPIC
///   hMessage must be non-persistent (ismMESSAGE_PERSISTENCE_NONPERSISTENT)
///   hMessage must be QoS0 (ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
///   hMessage must NOT be retained (ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN not set)
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_putMessageInternalOnDestination(
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_MessageHandle_t       hMessage,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
/// @remark Several informational return codes can be returned by this
/// function. For more details, see the remarks for ism_engine_putMessage on
/// informational return codes.
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
    ismEngine_CompletionCallback_t  pCallbackFn);

/** @name Unset retained message options
 * Options for the ism_engine_unsetRetainedMessage* functions.
 */

/**@{*/

/// Message is unset only and is *not* also published
#define ismENGINE_UNSET_RETAINED_OPTION_NONE      0x00000000

/// Message is published and unset
#define ismENGINE_UNSET_RETAINED_OPTION_PUBLISH   0x00000001

/**@}*/

//****************************************************************************
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
/// @remark Several informational return codes can be returned by this
/// function. For more details, see the remarks for ism_engine_putMessage on
/// informational return codes.
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
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
///
/// @remark The session handle can be NULL, but generally should specify the
/// session making the request to enable client-specific tracing to be enabled.
///
/// @remark The destination type can be one of ismDESTINATION_TOPIC when it
/// must not contain wildcards, or, ismDESTINATION_REGEX_TOPIC when it can
/// contain wildcards.
///
/// In the case of a REGEX unset, the retained messages are removed from the
/// local tree but no messages are sent into the cluster to remove equivalent
/// messages from other nodes.
///
/// @remark Several informational return codes can be returned by this
/// function. For more details, see the remarks for ism_engine_putMessage on
/// informational return codes.
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
XAPI int32_t WARN_CHECKRC ism_engine_unsetRetainedMessageOnDestination(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    uint32_t                        options,
    uint64_t                        serverTime,
    ismEngine_TransactionHandle_t   hTran,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

/// Use the default server time when unsetting retained msg
#define ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME 0

//****************************************************************************
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
XAPI int32_t WARN_CHECKRC ism_engine_republishRetainedMessages(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_ConsumerHandle_t      hConsumer,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
XAPI int32_t WARN_CHECKRC ism_engine_forwardRetainedMessages(
    const char                     *originServerUID,
    uint32_t                        options,
    ism_time_t                      timestamp,
    uint64_t                        correlationId,
    const char                     *requestingServerUID,
    void                           *pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


/** @name Forward retained message options
 * Options for the ism_engine_forwardRetainedMessages function.
 */

/**@{*/

/// Default behaviour all retained messages from specified origin are forwarded
#define ismENGINE_FORWARD_RETAINED_OPTION_NONE    0x00000000

/// Message with a timestamp newer than the specified timestamp should be forwarded
#define ismENGINE_FORWARD_RETAINED_OPTION_NEWER   0x00000001

/**@}*/


//****************************************************************************
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
/// On success, for synchronous completion, the unreleased-message handle
/// will be returned in an output parameter, while, for asynchronous
/// completion, the unreleased-message handle will be passed to the
/// operation-completion callback.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_addUnreleasedDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
XAPI int32_t WARN_CHECKRC ism_engine_removeUnreleasedDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_UnreleasedHandle_t    hUnrel,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
///
/// @remark The callback is called with the delivery IDs and the
/// unreleased-message handles for the supplied client-state. It is
/// intended for use during client reconnection to repopulate the message
/// delivery information. This callback MUST NOT call any Engine operations.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_listUnreleasedDeliveryIds(
    ismEngine_ClientStateHandle_t   hClient,
    void *                          pContext,
    ismEngine_UnreleasedCallback_t  pUnrelCallbackFunction);


//****************************************************************************
// @brief  Create Subscription
//
// Creates a subscription for a topic. The durability, reliability of message
// delivery and whether the subscription should be shared or single consumer
// only are also specified in the subAttributes subOptions field.
//
// @param[in]     hRequestingClient    Requesting Client handle
// @param[in]     pSubName             Subscription name
// @param[in]     pSubProperties       Optional context to associate with this subscription
// @param[in]     destinationType      Destination type - must be a TOPIC
// @param[in]     pDestinationName     Destination name - must be a topic string
// @param[in]     pSubAttributes       Attributes to use for this subscription
// @param[in]     hOwningClient        Owning Client handle, may be NULL
// @param[in]     pContext             Optional context for completion callback
// @param[in]     contextLength        Length of data pointed to by pContext
// @param[in]     pCallbackFn          Operation-completion callback
//
// @return OK on successful completion or an ISMRC_ value.
//
// @remark Two client handles are passed into this call one is the requesting
// client, the other is the client that will own the subscription. For non-shared
// durable subscriptions, or for those sharable between sessions of an individual
// client, these are expected to be the same client. For globally shared
// subscriptions, these may differ.
//
// If an owning client is not specified (NULL is passed) the owner is assumed to
// be the same as the requester.
//
// @remark The operation may complete synchronously or asynchronously. If it
// completes synchronously, the return code indicates the completion
// status.
//
// If the operation completes asynchronously, the return code from
// this function will be ISMRC_AsyncCompletion. The actual
// completion of the operation will be signalled by a call to the
// operation-completion callback, if one is provided. When the
// operation becomes asynchronous, a copy of the context will be
// made into memory owned by the Engine. This will be freed upon
// return from the operation-completion callback. Because the
// completion is asynchronous, the call to the operation-completion
// callback might occur before this function returns.
//
// The resulting subscription is a destination which can be used to
// create a message consumer.
//
// To consume from a named subscription, use ism_engine_createConsumer
// specifying a destinationType of ismDESTINATION_SUBSCRIPTION,
// a destinationName of the subname and the owning client handle.
//
// @see ism_engine_destroySubscription
// @see ism_engine_createConsumer
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createSubscription(
    ismEngine_ClientStateHandle_t              hRequestingClient,
    const char *                               pSubName,
    const ism_prop_t *                         pSubProperties,
    uint8_t                                    destinationType,
    const char *                               pDestinationName,
    const ismEngine_SubscriptionAttributes_t * pSubAttributes,
    ismEngine_ClientStateHandle_t              hOwningClient,
    void *                                     pContext,
    size_t                                     contextLength,
    ismEngine_CompletionCallback_t             pCallbackFn);

/** @name Engine properties
 * Properties which may be supplied on the ism_createSubscription call
 * when creating a durable subscription, or on the ism_engine_createConsumer
 * call.
 */

/**@{*/

/** @brief
 * Property used to pass selection string for subscription or consumer
 */
#define ismENGINE_PROPERTY_SELECTOR                       "Selector"

/** @brief
 * Property used to pass selection string for subscription
 */
#define ismENGINE_PROPERTY_NOLOCAL                        "NoLocal"

/**@}*/



//****************************************************************************
/// @brief  Destroy named Subscription
///
/// Destroy a named subscription previously created with
/// ism_engine_createSubscription.
///
/// @param[in]     hRequestingClient  Requesting client handle
/// @param[in]     pSubName           Subscription name
/// @param[in]     hOwningClient      Owning Client handle
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark There must be no active consumers on the subscription
/// in order for it to be successfully destroyed, if there are active
/// consumers, ISMRC_ActiveConsumers is returned.
///
/// The operation may complete synchronously or asynchronously. If it
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
/// @see ism_engine_createSubscription
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_destroySubscription(
    ismEngine_ClientStateHandle_t    hRequestingClient,
    const char *                     pSubName,
    ismEngine_ClientStateHandle_t    hOwningClient,
    void *                           pContext,
    size_t                           contextLength,
    ismEngine_CompletionCallback_t   pCallbackFn);


//****************************************************************************
/// @brief  Update Named subscription
///
/// Update a named subscription previously created with
/// ism_engine_createSubscription.
///
/// @param[in]     hRequestingClient  Requesting client handle
/// @param[in]     pSubName           Subscription name
/// @param[in]     pPropertyUpdates   Policy properties to update
/// @param[in]     hOwningClient      Owning client handle
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark This function can be used to alter the policy based attributes
/// of any subscription using a unique policy (basically, those created
/// by MQConnectivity or a unit test). At the moment, the only policy
/// property that is expected to be altered is MaxMessages (which is the
/// maximum message count) which can be altered while messages are
/// flowing.
///
/// The pPropertyUpdate parameter can include other undocumented policy
/// properties, however, the only one that is currently supported is
/// MaxMessages and it is an error to include any others. If others
/// are specified, then this function will update them, however, the
/// subsequent behaviour of the engine is undefined -- again, these will
/// only be updated for subscriptions using a unique messaging policy.
///
/// In addition to the policy properties, there are other properties
/// that can be updated on any subscription (whether it has a unique
/// policy or not).
///
/// "SubId" (uint32_t) specifies an update to the subscription Id.
/// "SubOptions" (uint32_t) specifies replacement subOptions.
///
/// Only a subset of SubOptions is allowed to be altered, the subset is
/// defined by ismENGINE_SUBSCRIPTION_OPTION_ALTERABLE_MASK. Any attempt
/// to alter other options will result in this function returning
/// ISMRC_InvalidParameter.
///
/// The operation may complete synchronously or asynchronously. If it
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
/// @see ism_engine_createSubscription
/// @see ism_engine_destroySubscription
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_updateSubscription(
    ismEngine_ClientStateHandle_t    hRequestingClient,
    const char *                     pSubName,
    const ism_prop_t *               pPropertyUpdates,
    ismEngine_ClientStateHandle_t    hOwningClient,
    void *                           pContext,
    size_t                           contextLength,
    ismEngine_CompletionCallback_t   pCallbackFn);


//****************************************************************************
/// @brief  List Subscriptions
///
/// List all of the named subscriptions owned by a given client.
///
/// @param[in]     hOwningClient Owning Client handle
/// @param[in]     pSubName      Optional subscription name to match
/// @param[in]     pContext      Optional context for subscription callback
/// @param[in]     pCallbackFn   Subscription callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The named subscriptions owned by the client specified
/// are enumerated, and for each the specified callback function is called.
///
/// @remark If a subscription name is specified, only subscriptions matching
///         that name (for the specified client id) will be returned.
///
/// The callback receives the subscription properties that were associated
/// at create subscription time, as well as context and subscription name.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_listSubscriptions(
    ismEngine_ClientStateHandle_t    hOwningClient,
    const char *                     pSubName,
    void *                           pContext,
    ismEngine_SubscriptionCallback_t pCallbackFn);


//****************************************************************************
/// @brief  Reuse an existing Subscription
///
/// Indicates to the engine that an existing subscription is being reused by the
/// requesting client, enabling the engine to perform any usage counting or authorization
/// required, and to update any required stored state due to altered attributes.
///
/// @param[in]     hRequestingClient    Requesting Client handle
/// @param[in]     pSubName             Subscription name
/// @param[in]     pSubAttributes       Subscription attributes
/// @param[in]     hOwningClient        Owning Client handle
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark Two client handles are passed into this call one is the requesting
/// client, the other is the client that owns the shared subscription
///
/// Both must be specified.
///
/// To consume from the named subscription, use ism_engine_createConsumer
/// specifying a destinationType of ismDESTINATION_SUBSCRIPTION,
/// a destinationName of the subname and the owning client handle.
///
/// The SubId can be updated to any value, however, the SubOptions that can
/// be altered for a sharing client is limited to the set defined in
/// ismENGINE_SUBSCRIPTION_OPTION_SHARING_CLIENT_ALTERABLE_MASK. Any attempt
/// to modify options outside of this mask will result in ISMRC_InvalidParameter
/// being returned.
///
/// @see ism_engine_createSubscription
/// @see ism_engine_createConsumer
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_reuseSubscription(
    ismEngine_ClientStateHandle_t              hRequestingClient,
    const char *                               pSubName,
    const ismEngine_SubscriptionAttributes_t * pSubAttributes,
    ismEngine_ClientStateHandle_t              hOwningClient);


//****************************************************************************
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
/// @param[in]     hOwningClient         Subscription owning client (may be NULL).
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
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The owning client handle is only used for a destination type of
/// ismDESTINATION_SUBSCRIPTION in order to locate the named subscription. If
/// this is NULL, the client owning the specified session handle is used.
///
/// The operation may complete synchronously or asynchronously. If it
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
///
/// In addition to supplying properties that can be used by the caller,
/// when destinationType is ismDESTINATION_TOPIC, pConsumerProperties
/// can be used to supply overrides for messaging policy values, for
/// example, MaxMessages (maximum message count) can be overridden.
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
    ismEngine_CompletionCallback_t             pCallbackFn);


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
    ismEngine_CompletionCallback_t  pCallbackFn);


/** @name consumerOptions
 * Options for the consumerOptions field ism_engine_createConsumer and
 * ism_engine_createRemoteServerConsumer functions.
 */

/** @brief
 * No consumer options
 */
#define ismENGINE_CONSUMER_OPTION_NONE                          0x0

/** @brief
 * Browse the message, don't consume them, leave them unchanged...
 */
#define ismENGINE_CONSUMER_OPTION_BROWSE                        0x1

/** @brief
 * Start the consumer paused (use resumeMessageDelivery to unpause)
 */
#define ismENGINE_CONSUMER_OPTION_PAUSE                         0x2

/** @brief
 * Deliver only messages which match the supplied selection string
 */
#define ismENGINE_CONSUMER_OPTION_MESSAGE_SELECTION             0x10

/** @brief
 * Each message given to this consumer will be acked/nacked
 */
#define ismENGINE_CONSUMER_OPTION_ACK                           0x20

/** @brief
 * Each message given to this consumer will be assigned a 16bit delivery
 * id unique across the scope of the consumer's client. This delivery id
 * will be persisted for the lifetime of the message.
 */
#define ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID      0x40

/** @brief
 *  Delivery counts of messages given to this consumer won't be
 *  persisted to the store, so after restart - delivery counts will
 *  appear to "reset" so e.g. MQTT consumers will lose the QoS1 dup
 *  flag. When a single consumer is a bottleneck - this is
 *  a significant optimisation.
 */
#define ismENGINE_CONSUMER_OPTION_NONPERSISTENT_DELIVERYCOUNTING 0x80

/** @brief
 * For ism_engine_createRemoteServerConsumer specify that the consumer
 * should be created on the low QoS queue of messages associated with
 * the remote server
 */
#define ismENGINE_CONSUMER_OPTION_LOW_QOS                     0x100

/** @brief
 * For ism_engine_createRemoteServerConsumer specify that the consumer
 * should be created on the high QoS queue of messages associated with
 * the remote server
 */
#define ismENGINE_CONSUMER_OPTION_HIGH_QOS                    0x200

/** @brief
 * Mask of options valid on ism_engine_createConsumer
 */
#define ismENGINE_CREATECONSUMER_OPTION_VALID_MASK              0xF3

/** @brief
 * Mask of options valid on ism_engine_createRemoteServerConsumer
 */
#define ismENGINE_CREATEREMOTESERVERCONSUMER_OPTION_VALID_MASK  0x3E2


//****************************************************************************
/// @brief  Get a message, with a timeout
///
/// Creates a message consumer for a destination. If the destination is a
/// topic, the consumer is associated with an anonymous, nondurable subscription.
/// After a timeout (or on receipt of the first message) the consumer is destroyed.
///
/// If a message that can be delivered instantly is not available, the function will
/// return async completion and if a message becomes available then after delivery the 
/// completion callback is called. If the timeout expires, the consumer is destroyed
/// and the completion callback is called with a code of ISMRC_NoMsgAvail 
///
/// @param[in]     hSession              Session handle
/// @param[in]     destinationType       Destination type
/// @param[in]     pDestinationName      Destination name
/// @param[in]     pSubAttributes        Subscription attributes for topic destination
/// @param[in]     deliverTimeOutMillis  milliseconds before get is cancelled
/// @param[in]     hOwningClient         Subscription owning client (may be NULL).
/// @param[in]     pMessageContext       Optional context for message callback
/// @param[in]     messageContextLength  Length of data pointed to by pMessageContext
/// @param[in]     pMessageCallbackFn    Message-delivery callback
/// @param[in]     pConsumerProperties   Consumer properties
/// @param[in]     consumerOptions       Consumer options (ismENGINE_CONSUMER_*)
/// @param[in]     pContext              Optional context for completion callback
/// @param[in]     contextLength         Length of data pointed to by pContext
/// @param[in]     pCallbackFn           Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The owning client handle is only used for a destination type of
/// ismDESTINATION_SUBSCRIPTION in order to locate the named subscription. If
/// this is NULL, the client owning the specified session handle is used.
///
/// The operation may complete synchronously or asynchronously. If it
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
///
/// In addition to supplying properties that can be used by the caller,
/// when destinationType is ismDESTINATION_TOPIC, pConsumerProperties
/// can be used to supply overrides for messaging policy values, for
/// example, MaxMessages (maximum message count) can be overridden.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getMessage(
                         ismEngine_SessionHandle_t                  hSession,
                         ismDestinationType_t                       destinationType,
                         const char *                               pDestinationName,
                         const ismEngine_SubscriptionAttributes_t * pSubAttributes,
                         uint64_t                                   deliverTimeOutMillis,    //< timeout in milliseconds
                         ismEngine_ClientStateHandle_t              hOwningClient,     ///< Which client owns the subscription
                         void *                                     pMessageContext,
                         size_t                                     messageContextLength,
                         ismEngine_MessageCallback_t                pMessageCallbackFn, ///< Must return false to first message
                         const ism_prop_t *                         pConsumerProperties,
                         uint32_t                                   consumerOptions,
                         void *                                     pContext,
                         size_t                                     contextLength,
                         ismEngine_CompletionCallback_t             pCallbackFn);

/**@}*/


//****************************************************************************
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
XAPI int32_t WARN_CHECKRC ism_engine_destroyConsumer(
    ismEngine_ConsumerHandle_t      hConsumer,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


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
    ismEngine_ConsumerHandle_t hConsumer);


//****************************************************************************
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
XAPI int32_t WARN_CHECKRC ism_engine_createTemporaryDestination(
    ismEngine_ClientStateHandle_t   hClient,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ism_prop_t *                    pDestinationProps,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
XAPI int32_t WARN_CHECKRC ism_engine_destroyTemporaryDestination(
    ismEngine_ClientStateHandle_t   hClient,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
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
    ismMessageHeader_t *            pHeader,
    uint8_t                         areaCount,
    ismMessageAreaType_t            areaTypes[areaCount],
    size_t                          areaLengths[areaCount],
    void *                          pAreaData[areaCount],
    ismEngine_MessageHandle_t *     phMessage);


//****************************************************************************
/// @brief  Confirm Message Delivery
///
/// Confirms delivery of one messages. This is used to move
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
XAPI int32_t WARN_CHECKRC ism_engine_confirmMessageDelivery(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_DeliveryHandle_t      hDelivery,
    uint32_t                        options,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


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
/// @param[inout]  pDeliveryHdls    Array of deliver handles to be confirmed
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
XAPI int32_t WARN_CHECKRC ism_engine_confirmMessageDeliveryBatch(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    uint32_t                        hdlCount,
    ismEngine_DeliveryHandle_t *    pDeliveryHdls,
    uint32_t                        options,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);

/** @name Confirm options
 * Options for the ism_engine_confirmMessageDelivery function.
 */

/**@{*/

/// The message has been consumed.
#define ismENGINE_CONFIRM_OPTION_CONSUMED        0x01

/// The message has been received, but is not yet consumed completely
#define ismENGINE_CONFIRM_OPTION_RECEIVED        0x02

/// The message has not been delivered to the client so make it available
/// to be resent without changing the delivery count
#define ismENGINE_CONFIRM_OPTION_NOT_DELIVERED   0x03

/// The message has not been acknowledged by the client with expected
/// parameters so request it to be resent
#define ismENGINE_CONFIRM_OPTION_NOT_RECEIVED    0x04

/// equivalent to ismENGINE_CONFIRM_OPTION_CONSUMED except that it
/// records that the client has expired a message. Note that
/// some protocols cannot guarentee that it was *this* message that
/// expired, just one from the same destination
#define ismENGINE_CONFIRM_OPTION_EXPIRED         0x05

/// (temporary?) Internal option used during recursive delete of a session
#define ismENGINE_CONFIRM_OPTION_SESSION_CLEANUP    0x100

/**@}*/


//****************************************************************************
/// @brief  Release Message
///
/// Releases the storage associated with a message. When a message is
/// consumed from a destination, the storage belongs to the Engine
/// component. This call releases it.
///
/// @param[in]     hMessage         Message handle
//****************************************************************************
XAPI void ism_engine_releaseMessage(
    ismEngine_MessageHandle_t       hMessage);


//****************************************************************************
/// @brief Returns the retained Message on a given topic string
///
/// @param[in]     hSession              Session handle
/// @param[in]     topicString           topicString (not containing wildcards)
/// @param[in]     pMessageContext       Optional context for message callback
/// @param[in]     messageContextLength  Length of data pointed to by pMessageContext
/// @param[in]     pMessageCallbackFn    Message-delivery callback
/// @param[in]     pContext              Optional context for completion callback
/// @param[in]     contextLength         Length of data pointed to by pContext
/// @param[in]     pCallbackFn           Operation-completion callback
///
/// @remark The messaging callback will NOT be called after this function
///  completes (synchronously or asynchronously)
///
/// @remark The topicString is expected to be of type ismDESTINATION_TOPIC and can
/// contain wildcards.
///
/// @remark Messages are delivered using a ismEngine_MessageCallback_t but the supplied
/// delivery handle parameter to that function will be null as acks/nacks have no meaning
///
/// @remark The ismEngine_MessageCallback_t function must release each message before
///  returning with a call to ism_engine_releaseMessage (and cannot refer to it once
///  that has been done)
///
/// @remark If the callback function returns false, the delivery of any subsequent messages
/// is abandoned (the function is responsible for releasing the message it was processing
/// when it chose to return false).
///
/// @return OK on successful completion with retained messages
///         ISMRC_NotFound for no retained messages
///            or an ISMRC_ value for an error
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getRetainedMessage(
           ismEngine_SessionHandle_t       hSession,
           const char *                    topicString,
           void *                          pMessageContext,
           size_t                          messageContextLength,
           ismEngine_MessageCallback_t     pMessageCallbackFn,
           void *                          pContext,
           size_t                          contextLength,
           ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
/// @brief  Create Queue-Manager Record
///
/// Creates a queue-manager record. This is a persistent record related to
/// a WebSphere MQ queue manager.
///
/// @param[in]     hSession         Session handle
/// @param[in]     pData            Ptr to queue-manager record's data
/// @param[in]     dataLength       Length of queue-manager record's data
/// @param[out]    phQMgrRec        Returned queue-manager record handle
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
XAPI int32_t WARN_CHECKRC ism_engine_createQManagerRecord(
    ismEngine_SessionHandle_t           hSession,
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QManagerRecordHandle_t *  phQMgrRec,
    void *                              pContext,
    size_t                              contextLength,
    ismEngine_CompletionCallback_t      pCallbackFn);


//****************************************************************************
/// @brief  Destroy Queue-Manager Record
///
/// Destroys a queue-manager record. This is a persistent record related to
/// a WebSphere MQ queue manager.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hQMgrRec         Queue-manager record handle
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
XAPI int32_t WARN_CHECKRC ism_engine_destroyQManagerRecord(
    ismEngine_SessionHandle_t           hSession,
    ismEngine_QManagerRecordHandle_t    hQMgrRec,
    void *                              pContext,
    size_t                              contextLength,
    ismEngine_CompletionCallback_t      pCallbackFn);


//****************************************************************************
/// @brief  List Queue-Manager Records
///
/// Lists the queue-manager records. The callback MUST NOT call any Engine operations.
///
/// @param[in]     hSession         Session handle
/// @param[in]     pContext         Optional context for queue-manager record callback
/// @param[in]     pQMgrRecCallbackFunction  Queue-manager record callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The callback is called with the queue-manager records. It is
/// intended for use during MQ Bridge initialisation to repopulate the message
/// delivery information. This callback MUST NOT call any Engine operations.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_listQManagerRecords(
    ismEngine_SessionHandle_t           hSession,
    void *                              pContext,
    ismEngine_QManagerRecordCallback_t  pQMgrRecCallbackFunction);


//****************************************************************************
/// @brief  Create Queue-Manager XID Record
///
/// Creates a queue-manager XID record. This is a persistent record related to
/// a transaction with a WebSphere MQ queue manager.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hQMgrRec         Queue-manager record handle
/// @param[in]     hTran            Transaction handle, may be NULL
/// @param[in]     pData            Ptr to queue-manager XID record's data
/// @param[in]     dataLength       Length of queue-manager XID record's data
/// @param[out]    phQMgrXidRec     Returned queue-manager XID record handle
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
XAPI int32_t WARN_CHECKRC ism_engine_createQMgrXidRecord(
    ismEngine_SessionHandle_t           hSession,
    ismEngine_QManagerRecordHandle_t    hQMgrRec,
    ismEngine_TransactionHandle_t       hTran,
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QMgrXidRecordHandle_t *   phQMgrXidRec,
    void *                              pContext,
    size_t                              contextLength,
    ismEngine_CompletionCallback_t      pCallbackFn);


//****************************************************************************
/// @brief  Destroy Queue-Manager XID Record
///
/// Destroys a queue-manager XID record. This is a persistent record related to
/// a transaction with a WebSphere MQ queue manager.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hQMgrXidRec      Queue-manager XID record handle
/// @param[in]     hTran            Transaction handle, may be NULL
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
XAPI int32_t WARN_CHECKRC ism_engine_destroyQMgrXidRecord(
    ismEngine_SessionHandle_t           hSession,
    ismEngine_QMgrXidRecordHandle_t     hQMgrXidRec,
    ismEngine_TransactionHandle_t       hTran,
    void *                              pContext,
    size_t                              contextLength,
    ismEngine_CompletionCallback_t      pCallbackFn);


//****************************************************************************
/// @brief  List Queue-Manager XID Records
///
/// Lists the queue-manager XID records for the supplied queue-manager record.
/// The callback MUST NOT call any Engine operations.
///
/// @param[in]     hSession         Session handle
/// @param[in]     hQMgrRec         Queue-manager record handle
/// @param[in]     pContext         Optional context for queue-manager XID record callback
/// @param[in]     pQMgrXidRecCallbackFunction  Queue-manager XID record callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The callback is called with the queue-manager XID records for the supplied
/// queue-manager record. It is intended for use during MQ Bridge initialisation
/// to repopulate the message delivery information. This callback MUST NOT call
/// any Engine operations.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_listQMgrXidRecords(
    ismEngine_SessionHandle_t           hSession,
    ismEngine_QManagerRecordHandle_t    hQMgrRec,
    void *                              pContext,
    ismEngine_QMgrXidRecordCallback_t   pQMgrXidRecCallbackFunction);


//****************************************************************************
/// @brief  Dump information about a Client-state to a file.
///
/// Dumps information about the client-state with a specified client ID to a file.
///
/// @param[in]     clientId       Client ID
/// @param[in]     detailLevel    How much detail to include in the output
/// @param[in]     userDataBytes  Max bytes of user data (per area) to write
/// @param[in,out] resultPath     Requested path on input, result path on output
///
/// @remark If resultPath on input is an empty string, the dump is written to a
///         temporary file and is formatted inline, to stdout - in this case,
///         the temporary file path is not returned to the caller.
///
/// @remark If the data is gathered synchronously, the result path should be
///         the requested path on input with '.dat' appended to it, if the data
///         is gathered asynchronously, the result path should instead have
///         '.partial' appended to it, once the request has completed, the file
///         should be renamed to the '.dat' form.
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_dumpClientState(
    const char *       clientId,
    int32_t            detailLevel,
    int64_t            userDataBytes,
    char *             resultPath);


//****************************************************************************
/// @brief  Dump information about an individual Topic to a file
///
/// Dumps information about the specified topic to a file.
///
/// @param[in]     topicString    Topic String
/// @param[in]     detailLevel    How much detail to include in the output
/// @param[in]     userDataBytes  Max bytes of user data (per area) to write
/// @param[in,out] resultPath     Requested path on input, result path on output
///
/// @remark If resultPath on input is an empty string, the dump is written to a
///         temporary file and is formatted inline, to stdout - in this case,
///         the temporary file path is not returned to the caller.
///
/// @remark If the data is gathered synchronously, the result path should be
///         the requested path on input with '.dat' appended to it, if the data
///         is gathered asynchronously, the result path should instead have
///         '.partial' appended to it, once the request has completed, the file
///         should be renamed to the '.dat' form.
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_dumpTopic(
    const char *       topicString,
    int32_t            detailLevel,
    int64_t            userDataBytes,
    char *             resultPath);


//****************************************************************************
/// @brief  Dump information about the topic tree below a root to a file
///
/// Dumps information about the topic tree below an optional root node to a
/// files - if no root is specified the entire topic tree is dumped.
///
/// @param[in]     rootTopicString  Root topic String (NULL for entire tree)
/// @param[in]     detailLevel      How much detail to include in the output
/// @param[in]     userDataBytes    Max bytes of user data (per area) to write
/// @param[in,out] resultPath       Requested path on input, result path on output
///
/// @remark If resultPath on input is an empty string, the dump is written to a
///         temporary file and is formatted inline, to stdout - in this case,
///         the temporary file path is not returned to the caller.
///
/// @remark If the data is gathered synchronously, the result path should be
///         the requested path on input with '.dat' appended to it, if the data
///         is gathered asynchronously, the result path should instead have
///         '.partial' appended to it, once the request has completed, the file
///         should be renamed to the '.dat' form.
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_dumpTopicTree(
    const char *       rootTopicString,
    int32_t            detailLevel,
    int64_t            userDataBytes,
    char *             resultPath);


//****************************************************************************
/// @brief  Dump information about a Queue to a file.
///
/// Dumps information about the named queue to a file.
///
/// @param[in]     queueName      Queue Name
/// @param[in]     detailLevel    How much detail to include in the output
/// @param[in]     userDataBytes  Max bytes of user data (per area) to write
/// @param[in,out] resultPath     Requested path on input, result path on output
///
/// @remark If resultPath on input is an empty string, the dump is written to a
///         temporary file and is formatted inline, to stdout - in this case,
///         the temporary file path is not returned to the caller.
///
/// @remark If the data is gathered synchronously, the result path should be
///         the requested path on input with '.dat' appended to it, if the data
///         is gathered asynchronously, the result path should instead have
///         '.partial' appended to it, once the request has completed, the file
///         should be renamed to the '.dat' form.
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_dumpQueue(
    const char *       queueName,
    int32_t            detailLevel,
    int64_t            userDataBytes,
    char *             resultPath);


//****************************************************************************
/// @brief  Dump information about a locks to a file.
///
/// Dumps information about the locks in the lock manager to a file.
///
/// @param[in]     lockName       Lock name - currently unused
/// @param[in]     detailLevel    How much detail to include in the output
/// @param[in]     userDataBytes  Max bytes of user data (per area) to write
/// @param[in,out] resultPath     Requested path on input, result path on output
///
/// @remark If resultPath on input is an empty string, the dump is written to a
///         temporary file and is formatted inline, to stdout - in this case,
///         the temporary file path is not returned to the caller.
///
/// @remark If the data is gathered synchronously, the result path should be
///         the requested path on input with '.dat' appended to it, if the data
///         is gathered asynchronously, the result path should instead have
///         '.partial' appended to it, once the request has completed, the file
///         should be renamed to the '.dat' form.
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_dumpLocks(
    const char *       lockName,
    int32_t            detailLevel,
    int64_t            userDataBytes,
    char *             resultPath);


//****************************************************************************
/// @brief  Dump information about transactions to a file.
///
/// Dumps information about the transactions to a file.
///
/// @param[in]     XIDString      Transaction Id - currently unused
/// @param[in]     detailLevel    How much detail to include in the output
/// @param[in]     userDataBytes  Max bytes of user data (per area) to write
/// @param[in,out] resultPath     Requested path on input, result path on output
///
/// @remark If resultPath on input is an empty string, the dump is written to a
///         temporary file and is formatted inline, to stdout - in this case,
///         the temporary file is _not_ returned to the caller.
///
/// @remark If the data is gathered synchronously, the result path should be
///         the requested path on input with '.dat' appended to it, if the data
///         is gathered asynchronously, the result path should instead have
///         '.partial' appended to it, once the request has completed, the file
///         should be renamed to the '.dat' form.
///
/// @returns OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_dumpTransactions(
    const char *       XIDString,
    int32_t            detailLevel,
    int64_t            userDataBytes,
    char *             resultPath);


//****************************************************************************
/// @brief  Request diagnostics from the engine
///
/// Requests miscellaneous diagnostic information from the engine.
///
/// @param[in]     mode               Type of diagnostics requested
/// @param[in]     args               Arguments to the diagnostics collection
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                       to be freed with ism_engine_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @remark If the return code is OK then diagnosticsOutput will be a string that
/// should be returned to the user and then freed using ism_engine_freeDiagnosticsOutput().
/// If the return code is ISMRC_AsyncCompletion then the callback function will be called
/// and if the retcode is OK, the handle will point to the string to show and then free with
/// ism_engine_freeDiagnosticsOutput()
///
/// @returns OK on successful completion
///          ISMRC_AsyncCompletion if gone asynchronous
///          or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_diagnostics(
    const char *                    mode,
    const char *                    args,
    char **                         diagnosticsOutput,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn);


//****************************************************************************
/// @brief  Free diagnostic info supplied by engine
///
/// @param[in]    diagnosticsOutput   string from ism_engine_diagnostics() or
///                                   returned as the handle to the callback function
///                                   from it
//****************************************************************************
XAPI void ism_engine_freeDiagnosticsOutput(
    char *                         diagnosticsOutput);

//****************************************************************************
/// @brief  Add the engine diagnostics into a ism_json_t object
///
/// @param[in]    jobj    the ism_json_t to add the diagnostics into
/// @param[in]    name    what to call it
//****************************************************************************
XAPI void ism_engine_addDiagnostics(ism_json_t * jobj, const char * name);

//****************************************************************************
/// @brief  Get engine monitoring data for subscriptions
///
/// @param[out]    results            The returned list of subscription monitors
/// @param[out]    resultCount        Count of the results
/// @param[in]     type               The type of requested information
/// @param[in]     maxResults         The maximum number of results to return
/// @param[in]     pFilterProperties  Properties on which to filter results (NULL
///                                   for none)
///
/// @remark ism_engine_freeSubscriptionMonitor must be called to release the results.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getSubscriptionMonitor(
    ismEngine_SubscriptionMonitor_t **results,
    uint32_t *                        resultCount,
    ismEngineMonitorType_t            type,
    uint32_t                          maxResults,
    ism_prop_t *                      pFilterProperties);


//****************************************************************************
/// @brief  Free a subscription monitor list
///
/// @param[in]     list              The subscription monitor list to free
//****************************************************************************
XAPI void ism_engine_freeSubscriptionMonitor(ismEngine_SubscriptionMonitor_t *list);


//****************************************************************************
/// @brief  Start topic monitoring on the subtree below a specified topic
///         string
///
/// @param[in]     topicString         The topic string which ends in a multi-level
///                                    wildcard but contains no other wildcards.
/// @param[in]     resetActiveMonitor  Whether to reset the stats for an already
///                                    active topic monitor.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_startTopicMonitor(
    const char *      topicString,
    bool              resetActiveMonitor);


//****************************************************************************
/// @brief  Stop topic monitoring on the subtree below a specified topic
///         string
///
/// @param[in]     topicString  The topic string which ends in a multi-level
///                             wildcard but contains no other wildcards.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_stopTopicMonitor(const char *topicString);


//****************************************************************************
/// @brief  Get engine monitoring data for subscriptions
///
/// @param[out]    results            The returned list of subscription monitors
/// @param[out]    resultCount        Count of the results
/// @param[in]     type               The type of requested information
/// @param[in]     maxResults         The maximum number of results to return
/// @param[in]     pFilterProperties  Properties on which to filter results (NULL
///                                   for none)
///
/// @remark If result type is ismENGINE_MONITOR_ALL_UNSORTED then maxResults is
///         ignored and all of the active topic monitor results are returned in
///         an unsorted list.
///
/// @remark ism_engine_freeTopicMonitor must be called to release the results.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getTopicMonitor(
    ismEngine_TopicMonitor_t **       results,
    uint32_t *                        resultCount,
    ismEngineMonitorType_t            type,
    uint32_t                          maxResults,
    ism_prop_t *                      pFilterProperties);


//****************************************************************************
/// @brief  Free a topic monitor list
///
/// @param[in]     list  The topic monitor list to free
//****************************************************************************
XAPI void ism_engine_freeTopicMonitor(ismEngine_TopicMonitor_t *list);


//****************************************************************************
/// @brief  Get engine monitoring data for queues
///
/// @param[out]    results            The returned list of queue monitors
/// @param[out]    resultCount        Count of the results
/// @param[in]     type               The type of requested information
/// @param[in]     maxResults         The maximum number of results to return
/// @param[in]     pFilterProperties  Properties on which to filter results (NULL
///                                   for none)
///
/// @remark ism_engine_freeQueueMonitor must be called to release the results.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getQueueMonitor(
    ismEngine_QueueMonitor_t **       results,
    uint32_t *                        resultCount,
    ismEngineMonitorType_t            type,
    uint32_t                          maxResults,
    ism_prop_t *                      pFilterProperties);


//****************************************************************************
/// @brief  Free a queue monitor list
///
/// @param[in]     list              The queue monitor list to free
//****************************************************************************
XAPI void ism_engine_freeQueueMonitor(ismEngine_QueueMonitor_t *list);


//****************************************************************************
/// @brief  Get monitoring data for client-states
///
/// @param[out]    ppMonitor         The returned list of client-state monitors
/// @param[out]    pResultCount      Count of the results
/// @param[in]     type              The type of requested information
/// @param[in]     maxResults        The maximum number of results to return
/// @param[in]     pFilterProperties Properties on which to filter results (NULL
///                                  for none)
///
/// @remark ism_engine_freeClientStateMonitor must be called to release the results.
///
/// @remark The list of clients returned will only contain those that are currently
/// disconnected (marked as 'Zombie').
///
/// By default only MQTT/HTTP clients are returned, to alter the list of protocols
/// returned use the "ProtocolID" filter property (@see ismENGINE_MONITOR_FILTER_PROTOCOLID)
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getClientStateMonitor(
    ismEngine_ClientStateMonitor_t ** ppMonitor,
    uint32_t *                        pResultCount,
    ismEngineMonitorType_t            type,
    uint32_t                          maxResults,
    ism_prop_t *                      pFilterProperties);


//****************************************************************************
/// @brief  Free monitoring data for client-states
///
/// @param[in]     pMonitor          The monitoring information to free
//****************************************************************************
XAPI void ism_engine_freeClientStateMonitor(ismEngine_ClientStateMonitor_t *pMonitor);


//****************************************************************************
/// @brief  Get monitoring data for prepared or heuristically completed transactions
///
/// @param[out]    results          The returned list of transaction monitors
/// @param[out]    resultCount      Count of the results
/// @param[in]     type             The type of requested information
/// @param[in]     maxResults       The maximum number of results to return
/// @param[in]     filterProperties Properties on which to filter results (NULL
///                                 for none)
///
/// @remark ism_engine_freeTransactionMonitor must be called to release the results.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getTransactionMonitor(
    ismEngine_TransactionMonitor_t **results,
    uint32_t *                       resultCount,
    ismEngineMonitorType_t           type,
    uint32_t                         maxResults,
    ism_prop_t *                     filterProperties);


//****************************************************************************
/// @brief  Free monitoring data for transactions
///
/// @param[in]     list              The monitoring information to free
//****************************************************************************
XAPI void ism_engine_freeTransactionMonitor(ismEngine_TransactionMonitor_t *list);


//****************************************************************************
/// @brief  Get engine monitoring data for resource sets
///
/// @param[out]    results            The returned list of resourceSet monitors
/// @param[out]    resultCount        Count of the results
/// @param[in]     type               The type of requested information
/// @param[in]     maxResults         The maximum number of results to return
/// @param[in]     pFilterProperties  Properties on which to filter results (NULL
///                                   for none)
///
/// @remark If type of ismENGINE_MONITOR_ALL_UNSORTED is specified, maxResults is
/// ignored and all of the results matching any specified filters are returned --
/// this could be a very large set of results.
///
/// @remark ism_engine_freeResourceSetMonitor must be called to release the results.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getResourceSetMonitor(
    ismEngine_ResourceSetMonitor_t  **results,
    uint32_t *                        resultCount,
    ismEngineMonitorType_t            type,
    uint32_t                          maxResults,
    ism_prop_t *                      pFilterProperties);


//****************************************************************************
/// @brief  Free a resource set monitor list
///
/// @param[in]     list              The resource set monitor list to free
//****************************************************************************
XAPI void ism_engine_freeResourceSetMonitor(ismEngine_ResourceSetMonitor_t *list);


//****************************************************************************
/// @brief  Generate the disconnected client notification messages for disconnected
///         MQTT clients for whom messages have arrived.
///
/// @remark The messages are published to topic "$SYS/DisconnectedClientNotification"
///
///         The messages contain information to identify disconnected MQTT clients
///         that have subscriptions to topics on which new messages have been published
///         since the last notification was generated.
///
///         The messages are in the following JSON format:
///
///         { "ClientId":"<CLIENTID>",
///           "UserID":"<USERID>",
///           "MessagesArrived":[ { "TopicString":"<TOPIC>",
///                                 "MessageCount":<MSGCOUNT> },
///                               ... ] }
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_generateDisconnectedClientNotifications(void);

//****************************************************************************
/// @brief Callback registered with the config component
///
/// @param[in]  objectType        Configuration object
/// @param[in]  objectIdentifier  Name or Index of the configuration object
/// @param[in]  changedProps      A properties object (contains modified properties only)
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
                              ism_ConfigChangeType_t changeType);


//****************************************************************************
/// @brief Get memory statistics
///
/// @param[out] pStatistics       Memory statistics
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getMemoryStatistics(ismEngine_MemoryStatistics_t *pStatistics);


//****************************************************************************
/// @brief  Get messaging statistics
///
/// @param[out] pStatistics    ismEngine_MessagingStatistics_t to fill in
//****************************************************************************
XAPI void ism_engine_getMessagingStatistics(ismEngine_MessagingStatistics_t *pStatistics);


//****************************************************************************
/// @brief  Get the engine's statistic for the specified remote server Handle
///
/// @param[in]  remoteServerHandle  The engine's handle representing the remote
///                                 server.
/// @param[out] pStatistics         ismEngine_RemoteServerStatistics_t to fill in
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_getRemoteServerStatistics(
                                                  ismEngine_RemoteServerHandle_t remoteServerHandle,
                                                  ismEngine_RemoteServerStatistics_t *pStatistics);


//****************************************************************************
/// @brief  Export a set of engine resources to a specified file
///
/// Export a set of engine resources specified.
///
/// @param[in]     pClientId        String specifying regular expression of clientIds
///                                 to be exported (may be NULL for no clients).
/// @param[in]     pTopic           String specifying regular expression of retained
///                                 message topics to be exported (may be NULL for no
///                                 retained messages).
/// @param[in]     pFilename        File name to use for the resultant exported data
/// @param[in]     pPassword        Password to use when encrypting the file.
/// @param[in]     options          ismENGINE_EXPORT_RESOURCES_OPTION_* values
/// @param[out]    pRequestID       Pointer to the uint64_t value to receive the
///                                 request identifier.
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The resources exported depend on the specified clientId / topic regular
/// expressions, and the specified options.
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
XAPI int32_t WARN_CHECKRC ism_engine_exportResources(
        const char *                    pClientId,
        const char *                    pTopic,
        const char *                    pFilename,
        const char *                    pPassword,
        uint32_t                        options,
        uint64_t *                      pRequestID,
        void *                          pContext,
        size_t                          contextLength,
        ismEngine_CompletionCallback_t  pCallbackFn);


/** @name Export engine resources options
 * Options for the ism_engine_exportResources function.
 */

/**@{*/

/** @brief
 *  No options.
 */
#define ismENGINE_EXPORT_RESOURCES_OPTION_NONE                         0x00000000

/** @brief
 *  If the export file already exists, overwrite it, alternatively the call will
 *  fail if the file already exists.
 */
#define ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE                    0x00000001

/** @brief
 * Include internal client states (those beginning with "__" in the set of clients
 * included in the output (by default they are not)
 */
#define ismENGINE_EXPORT_RESOURCES_OPTION_INCLUDE_INTERNAL_CLIENTIDS   0x00000100

/**@}*/


//****************************************************************************
/// @brief  Import a set of engine resources from a file (previously exported
///         with ism_engine_exportResources).
///
/// Import a set of engine resources from the specified encrypted file.
///
/// @param[in]     pFilename        File name of the encrypted exported resources
/// @param[in]     pPassword        Password to use when decrypting the file.
/// @param[in]     options          ismENGINE_IMPORT_RESOURCES_OPTION_* values
/// @param[out]    pRequestID       Pointer to the uint64_t value to receive the
///                                 request identifier.
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
XAPI int32_t WARN_CHECKRC ism_engine_importResources(
        const char *                    pFilename,
        const char *                    pPassword,
        uint32_t                        options,
        uint64_t *                      pRequestID,
        void *                          pContext,
        size_t                          contextLength,
        ismEngine_CompletionCallback_t  pCallbackFn);


/** @name Import engine resources options
 * Options for the ism_engine_importResources function.
 */

/**@{*/

/** @brief
 *  No options.
 */
#define ismENGINE_IMPORT_RESOURCES_OPTION_NONE               0x00

/**@}*/

//****************************************************************************
/// @brief  Send a heartbeat to the engine.
///
/// @remark The engine will perform any work that is outstanding for the
/// calling thread.
//****************************************************************************
XAPI void ism_engine_heartbeat(void);

//****************************************************************************
/** @name Return codes for Engine component operations
 */
//****************************************************************************

/**@{*/

#define OK                              0   ///< Operation completed successfully

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_DEFINED */

/*********************************************************************/
/* End of engine.h                                                   */
/*********************************************************************/
