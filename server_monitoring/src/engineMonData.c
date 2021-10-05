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

#define TRACE_COMP Monitoring

#include <monitoring.h>
#include <engineMonData.h>
#include <monitoringutil.h>
#include <config.h>
#include <admin.h>

DEFINE_SUBSCRIPTION_MONITORING_OBJ(XismEngine_SubscriptionMonitor_t);
DEFINE_QUEUE_MONITORING_OBJ(XismEngine_QueueMonitor_t);
DEFINE_TOPIC_MONITORING_OBJ(XismEngine_TopicMonitor_t);
DEFINE_MQTTCLIENT_MONITORING_OBJ(XismEngine_ClientStateMonitor_t);
DEFINE_TRANSACTION_MONITORING_OBJ(XismEngine_TransactionMonitor_t);
DEFINE_RESOURCESET_MONITORING_OBJ(XismEngine_ResourceSetMonitor_t);
DEFINE_RESOURCESET_MONITORING_OBJ_TOTALMEMORYBYTES_SUBSET(X_TOTALMEMORYBYTES_SUBSET_ismEngine_ResourceSetMonitor_t);
DEFINE_RESOURCESET_MONITORING_OBJ_STANDARD_SUBSET(X_STANDARD_SUBSET_ismEngine_ResourceSetMonitor_t);

ism_mon_obj_def_t *allobjs[] = {XismEngine_SubscriptionMonitor_t,
        XismEngine_QueueMonitor_t,
        XismEngine_TopicMonitor_t,
        XismEngine_ClientStateMonitor_t,
        XismEngine_TransactionMonitor_t,
        XismEngine_ResourceSetMonitor_t,
        X_TOTALMEMORYBYTES_SUBSET_ismEngine_ResourceSetMonitor_t,
        X_STANDARD_SUBSET_ismEngine_ResourceSetMonitor_t,
        NULL
};

extern pthread_key_t monitoring_localekey;

static ism_time_t lastime=0, currenttime=0;


/*
 * Holding monitoring data for each snap shot of memory
 */
typedef struct ism_mondata_memory_t {
  	
  	int64_t MemoryTotalBytes;          ///< Total physical memory on the system
  	int64_t MemoryFreeBytes;           ///< Free memory on the system
  	double   MemoryFreePercent;         ///< Free memory as a percentage of total memory
  	int64_t ServerVirtualMemoryBytes;  ///< Virtual memory size of imaserver process
  	int64_t ServerResidentSetBytes;    ///< Resident set size of imaserver process
  	int64_t GroupMessagePayloads;      ///< Group - Message payloads (bytes)
  	int64_t GroupPublishSubscribe;     ///< Group - Publish/subscribe - topics and subscriptions (bytes)
    int64_t GroupDestinations;         ///< Group - Destinations containing messages (bytes)
    int64_t GroupCurrentActivity;      ///< Group - Memory for current activity (bytes)
    int64_t GroupClientStates;	       ///< Group - Memory for ClientStates
  	
}ism_mondata_memory_t;

static ism_monitoring_snap_list_t * monitoringMemorySnapList = NULL;

//static int newMemoryMonData(ism_snapshot_range_t ** list, ismEngine_MemoryStatistics_t * memoryStats) ;
static int32_t getMemoryHistoryStats(ism_monitoring_snap_t *list, char * types, int duration, ism_snaptime_t interval, concat_alloc_t * output_buffer);

/**
 * Returns STATS type from stat action string
 */
static int ismmon_getStatsType(char *action) {
    if ( !action || (action && *action == '\0'))
        return ISMMON_ENGINE_STATE_ACTION_NONE;

    if ( !strcasecmp(action, "Subscription")) {
        return ISMMON_ENGINE_STATE_ACTION_SUBSCRIPTION;
    } else if ( !strcasecmp(action, "Topic")) {
        return ISMMON_ENGINE_STATE_ACTION_TOPIC;
    } else if ( !strcasecmp(action, "Queue")) {
        return ISMMON_ENGINE_STATE_ACTION_QUEUE;
    } else if ( !strcasecmp(action, "MQTTClient")) {
        return ISMMON_ENGINE_STATE_ACTION_MQTTCLIENT;
    } else if ( !strcasecmp(action, "Transaction")) {
         return ISMMON_ENGINE_STATE_ACTION_TRANSACTION;
    } else if (!strcasecmp(action, "ResourceSet")) {
         return ISMMON_ENGINE_STATE_ACTION_RESOURCESET;
    } else {
        return ISMMON_ENGINE_STATE_ACTION_NONE;
    }
}

#define STAT_TYPE( TRUNK , HI, LO , RC_HI , RC_LO )       \
    if (strcasecmp(statTypeStr, TRUNK HI) == 0       \
            || strcasecmp(statTypeStr, TRUNK) == 0)    \
        return RC_HI;                                     \
    if (strcasecmp(statTypeStr, TRUNK LO) == 0         \
            || strcasecmp(statTypeStr, "-" TRUNK) == 0)   \
        return RC_LO;

static int ismmon_getStatType(char *statTypeStr) {
    int rc = -1;
    if ( !statTypeStr )
        return rc;
    char letter = statTypeStr[0];
    if ( letter == '-')
    {
        letter = statTypeStr[1];
    }

    if( letter == '\0'){
        return rc;
    }

    if (letter == 'A' || letter == 'a') {
        if (strcasecmp(statTypeStr, "AllUnsorted") == 0) {
            return ismENGINE_MONITOR_ALL_UNSORTED;
        }
        STAT_TYPE( "ActiveClients","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_ACTIVECLIENTS,-1)
        STAT_TYPE( "ActivePersistentClients","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_ACTIVEPERSISTENTCLIENTS,-1)
        STAT_TYPE( "ActiveNonpersistentClients","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_ACTIVENONPERSISTENTCLIENTS,-1)
    }
    else if (letter == 'B' || letter == 'b') {
        STAT_TYPE( "BufferedMsgsBytes","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGBYTES,-1);
        STAT_TYPE( "BufferedMsgs","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS)
        STAT_TYPE( "BufferedPercent","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_BUFFEREDPERCENT,ismENGINE_MONITOR_LOWEST_BUFFEREDPERCENT)
        STAT_TYPE( "BufferedHWMPercent","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_BUFFEREDHWMPERCENT,ismENGINE_MONITOR_LOWEST_BUFFEREDHWMPERCENT)
    } else if (letter == 'C' || letter == 'c') {
        STAT_TYPE( "Connections","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_CONNECTIONS,-1);
    }

    else if (letter == 'D' || letter == 'd') {
        STAT_TYPE( "DiscardMsgs","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS,ismENGINE_MONITOR_LOWEST_DISCARDEDMSGS)
    } else if (letter == 'E' || letter == 'e') {
        STAT_TYPE( "ExpiredMsgs","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_EXPIREDMSGS,ismENGINE_MONITOR_LOWEST_EXPIREDMSGS)
    } else if ( letter == 'M' || letter == 'm' ) {
        STAT_TYPE( "MaxPublishRecipients","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_MAXPUBLISHRECIPIENTS,-1)
    } else if ( letter == 'N' || letter == 'n' ) {
        STAT_TYPE( "NonpersistentNonSharedSubscriptions","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_NONPERSISTENTNONSHAREDSUBSCRIPTIONS,-1)
        STAT_TYPE( "NonpersistentSharedSubscriptions","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_NONPERSISTENTSHAREDSUBSCRIPTIONS,-1)
        STAT_TYPE( "NonpersistentBufferedMsgBytes","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_NONPERSISTENTBUFFEREDMSGBYTES,-1)
        STAT_TYPE( "NonpersistentWillMsgBytes","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_NONPERSISTENTWILLMSGBYTES,-1)
    } else if (letter == 'P' || letter == 'p') {
        STAT_TYPE( "PublishedMsgs","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,ismENGINE_MONITOR_LOWEST_PUBLISHEDMSGS)
        STAT_TYPE( "PublishedMsgsBytes","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGBYTES,-1)
        STAT_TYPE( "PersistentNonSharedSubscriptions","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_PERSISTENTNONSHAREDSUBSCRIPTIONS,-1)
        STAT_TYPE( "PersistentSharedSubscriptions","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_PERSISTENTSHAREDSUBSCRIPTIONS,-1)
        STAT_TYPE( "PersistentBufferedMsgBytes","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_PERSISTENTBUFFEREDMSGBYTES,-1)
        STAT_TYPE( "PersistentWillMsgBytes","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_PERSISTENTWILLMSGBYTES,-1)
        STAT_TYPE( "PersistentClientStates","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_PERSISTENTCLIENTSTATES,-1)
    } else if (letter == 'Q' || letter == 'q') {
        STAT_TYPE( "QoS0PublishedMsgs","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGS,-1)
        STAT_TYPE( "QoS1PublishedMsgs","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGS,-1)
        STAT_TYPE( "QoS2PublishedMsgs","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGS,-1)
        STAT_TYPE( "QoS0PublishedMsgBytes","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGBYTES,-1)
        STAT_TYPE( "QoS1PublishedMsgBytes","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGBYTES,-1)
        STAT_TYPE( "QoS2PublishedMsgBytes","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGBYTES,-1)
    } else if (letter == 'R' || letter == 'r') {
        STAT_TYPE( "RejectedMsgs","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS,ismENGINE_MONITOR_LOWEST_REJECTEDMSGS)
        STAT_TYPE( "RetainedMsgs","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_RETAINEDMSGS,-1)
        STAT_TYPE( "RetainedMsgsBytes","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_RETAINEDMSGBYTES,-1)
    }
    else if ( *statTypeStr == 'S' || *statTypeStr == 's' ) {
        STAT_TYPE( "Subscriptions","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS,-1)
   } else if ( *statTypeStr == 'T' || *statTypeStr == 't' ) {
       STAT_TYPE( "TotalMemoryBytes","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES,-1)
   } else if ( *statTypeStr == 'W' || *statTypeStr == 'w' ) {
       STAT_TYPE( "WillMsgs","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_WILLMSGS,-1)
       STAT_TYPE( "WillMsgBytes","Highest","Lowest",ismENGINE_MONITOR_HIGHEST_WILLMSGBYTES,-1)
   }

    return rc;
}

/**
 * Returns Subscription stats types (refer to server_engine/include/engine.h for supported monitoring types:
 *
 *   PublishedMsgsHighest   ->  ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS
 *   PublishedMsgsLowest    ->  ismENGINE_MONITOR_LOWEST_PUBLISHEDMSGS
 *   BufferedMsgsHighest    ->  ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS
 *   BufferedMsgsLowest     ->  ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS
 *   BufferedPercentHighest ->  ismENGINE_MONITOR_HIGHEST_BUFFEREDPERCENT
 *   BufferedPercentLowest  ->  ismENGINE_MONITOR_LOWEST_BUFFEREDPERCENT
 *   RejectedMsgsHighest    ->  ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS
 *   RejectedMsgsLowest     ->  ismENGINE_MONITOR_LOWEST_REJECTEDMSGS
 *   BufferedHWMPercentHighest -> ismENGINE_MONITOR_HIGHEST_BUFFEREDHWMPERCENT
 *   BufferedHWMPercentLowest  -> ismENGINE_MONITOR_LOWEST_BUFFEREDHWMPERCENT
 *   DiscardedMsgsHighest -> ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS
 *   DiscardedMsgsLowest  -> ismENGINE_MONITOR_LOWEST_DISCARDEDMSGS
 *   ExpiredMsgsHighest   -> ismENGINE_MONITOR_HIGHEST_EXPIREDMSGS
 *   ExpiredMsgsLowest    -> ismENGINE_MONITOR_LOWEST_EXPIREDMSGS
 *   AllUnsorted          -> ismENGINE_MONITOR_ALL_UNSORTED
 */
static int ismmon_getSubscriptionStatType(char *statTypeStr) {
    int rc = ismmon_getStatType(statTypeStr);
    switch( rc ){
    case ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS:
    case ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS:
    case ismENGINE_MONITOR_HIGHEST_BUFFEREDPERCENT:
    case ismENGINE_MONITOR_LOWEST_BUFFEREDPERCENT:
    case ismENGINE_MONITOR_HIGHEST_BUFFEREDHWMPERCENT:
    case ismENGINE_MONITOR_LOWEST_BUFFEREDHWMPERCENT:
    case ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS:
    case ismENGINE_MONITOR_LOWEST_DISCARDEDMSGS:
    case ismENGINE_MONITOR_HIGHEST_EXPIREDMSGS:
    case ismENGINE_MONITOR_LOWEST_EXPIREDMSGS:
    case ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS:
    case ismENGINE_MONITOR_LOWEST_PUBLISHEDMSGS:
    case ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS:
    case ismENGINE_MONITOR_LOWEST_REJECTEDMSGS:
    case ismENGINE_MONITOR_ALL_UNSORTED:
        return rc;
    default:
        return -1;
    }
}

/**
 * Return ResourceSet stat types (refer to server_engine/include/engine.h for supported monitoring types:
 *
 *   AllUnsorted                                    ->  ismENGINE_MONITOR_ALL_UNSORTED
 *   TotalMemoryBytesHighest                        ->  ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES
 *   SubscriptionsHighest                           ->  ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS
 *   PersistentNonSharedSubscriptionsHighest        ->  ismENGINE_MONITOR_HIGHEST_PERSISTENTNONSHAREDSUBSCRIPTIONS
 *   NonpersistentNonSharedSubscriptionsHighest     ->  ismENGINE_MONITOR_HIGHEST_NONPERSISTENTNONSHAREDSUBSCRIPTIONS
 *   PersistentSharedSubscriptionsHighest           ->  ismENGINE_MONITOR_HIGHEST_PERSISTENTSHAREDSUBSCRIPTIONS
 *   NonpersistentSharedSubscriptionsHighest        ->  ismENGINE_MONITOR_HIGHEST_NONPERSISTENTSHAREDSUBSCRIPTIONS
 *   BufferedMsgsHighest                            ->  ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS
 *   DiscardedMsgsHighest                           ->  ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS
 *   RejectedMsgsHighest                            ->  ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS
 *   RetainedMsgsHighest                            ->  ismENGINE_MONITOR_HIGHEST_RETAINEDMSGS
 *   WillMsgsHighest                                ->  ismENGINE_MONITOR_HIGHEST_WILLMSGS
 *   BufferedMsgBytesHighest                        ->  ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGBYTES
 *   PersistentBufferedMsgBytesHighest              ->  ismENGINE_MONITOR_HIGHEST_PERSISTENTBUFFEREDMSGBYTES
 *   NonpersistentBufferedMsgBytesHighest           ->  ismENGINE_MONITOR_HIGHEST_NONPERSISTENTBUFFEREDMSGBYTES
 *   RetainedMsgBytesHighest                        ->  ismENGINE_MONITOR_HIGHEST_RETAINEDMSGBYTES
 *   WillMsgBytesHighest                            ->  ismENGINE_MONITOR_HIGHEST_WILLMSGBYTES
 *   PersistentWillMsgBytesHighest                  ->  ismENGINE_MONITOR_HIGHEST_PERSISTENTWILLMSGBYTES
 *   NonpersistentWillMsgBytesHighest               ->  ismENGINE_MONITOR_HIGHEST_NONPERSISTENTWILLMSGBYTES
 *   PublishedMsgsHighest                           ->  ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS
 *   QoS0PublishedMsgsHighest                       ->  ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGS
 *   QoS1PublishedMsgsHighest                       ->  ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGS
 *   QoS2PublishedMsgsHighest                       ->  ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGS
 *   PublishedMsgBytesHighest                       ->  ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGBYTES
 *   QoS0PublishedMsgBytesHighest                   ->  ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGBYTES
 *   QoS1PublishedMsgBytesHighest                   ->  ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGBYTES
 *   QoS2PublishedMsgBytesHighest                   ->  ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGBYTES
 *   MaxPublishRecipientsHighest                    ->  ismENGINE_MONITOR_HIGHEST_MAXPUBLISHRECIPIENTS
 *   ConnectionsHighest                             ->  ismENGINE_MONITOR_HIGHEST_CONNECTIONS
 *   ActiveClientsHighest                           ->  ismENGINE_MONITOR_HIGHEST_ACTIVECLIENTS
 *   ActivePersistentClientsHighest                 ->  ismENGINE_MONITOR_HIGHEST_ACTIVEPERSISTENTCLIENTS
 *   ActiveNonpersistentClientsHighest              ->  ismENGINE_MONITOR_HIGHEST_ACTIVENONPERSISTENTCLIENTS
 *   PersistentClientStatesHighest                  ->  ismENGINE_MONITOR_HIGHEST_PERSISTENTCLIENTSTATES
 */
static int ismmon_getResourceSetStatType(char *statTypeStr) {
    int rc = ismmon_getStatType(statTypeStr);
    switch( rc ){
    case ismENGINE_MONITOR_HIGHEST_ACTIVECLIENTS:
    case ismENGINE_MONITOR_HIGHEST_ACTIVEPERSISTENTCLIENTS:
    case ismENGINE_MONITOR_HIGHEST_ACTIVENONPERSISTENTCLIENTS:
    case ismENGINE_MONITOR_ALL_UNSORTED:
    case ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS:
    case ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGBYTES:
    case ismENGINE_MONITOR_HIGHEST_CONNECTIONS:
    case ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS:
    case ismENGINE_MONITOR_HIGHEST_MAXPUBLISHRECIPIENTS:
    case ismENGINE_MONITOR_HIGHEST_NONPERSISTENTNONSHAREDSUBSCRIPTIONS:
    case ismENGINE_MONITOR_HIGHEST_NONPERSISTENTSHAREDSUBSCRIPTIONS:
    case ismENGINE_MONITOR_HIGHEST_NONPERSISTENTBUFFEREDMSGBYTES:
    case ismENGINE_MONITOR_HIGHEST_NONPERSISTENTWILLMSGBYTES:
    case ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS:
    case ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGBYTES:
    case ismENGINE_MONITOR_HIGHEST_PERSISTENTNONSHAREDSUBSCRIPTIONS:
    case ismENGINE_MONITOR_HIGHEST_PERSISTENTSHAREDSUBSCRIPTIONS:
    case ismENGINE_MONITOR_HIGHEST_PERSISTENTBUFFEREDMSGBYTES:
    case ismENGINE_MONITOR_HIGHEST_PERSISTENTWILLMSGBYTES:
    case ismENGINE_MONITOR_HIGHEST_PERSISTENTCLIENTSTATES:
    case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGS:
    case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGS:
    case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGS:
    case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGBYTES:
    case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGBYTES:
    case ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGBYTES:
    case ismENGINE_MONITOR_HIGHEST_RETAINEDMSGS:
    case ismENGINE_MONITOR_HIGHEST_RETAINEDMSGBYTES:
    case ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS:
    case ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS:
    case ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES:
    case ismENGINE_MONITOR_HIGHEST_WILLMSGS:
    case ismENGINE_MONITOR_HIGHEST_WILLMSGBYTES:
        return rc;
    default:
        return -1;
    }
}
/**
 * Returns Queue stats types (refer to server_engine/include/engine.h for supported monitoring types:
 *
 *   BufferedMsgsHighest    ->  ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS
 *   BufferedMsgsLowest     ->  ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS
 *   BufferedPercentHighest ->  ismENGINE_MONITOR_HIGHEST_BUFFEREDPERCENT
 *   BufferedPercentLowest  ->  ismENGINE_MONITOR_LOWEST_BUFFEREDPERCENT
 *   ProducedMsgsHighest    ->  ismENGINE_MONITOR_HIGHEST_PRODUCEDMSGS
 *   ProducedMsgsLowest     ->  ismENGINE_MONITOR_LOWEST_PRODUCEDMSGS
 *   ConsumedMsgsHighest    ->  ismENGINE_MONITOR_HIGHEST_CONSUMEDMSGS
 *   ConsumedMsgsLowest     ->  ismENGINE_MONITOR_LOWEST_CONSUMEDMSGS
 *   ConsumersHighest       ->  ismENGINE_MONITOR_HIGHEST_CONSUMERS
 *   ConsumersLowest        ->  ismENGINE_MONITOR_LOWEST_CONSUMERS
 *   ProducersHighest       ->  ismENGINE_MONITOR_HIGHEST_PRODUCERS
 *   ProducersLowest        ->  ismENGINE_MONITOR_LOWEST_PRODUCERS
 *   BufferedHWMPercentHighest -> ismENGINE_MONITOR_HIGHEST_BUFFEREDHWMPERCENT
 *   BufferedHWMPercentLowest  -> ismENGINE_MONITOR_LOWEST_BUFFEREDHWMPERCENT
 *   ExpiredMsgsHighest     ->  ismENGINE_MONITOR_HIGHEST_EXPIREDMSGS
 *   ExpiredMsgsLowest      ->  ismENGINE_MONITOR_LOWEST_EXPIREDMSGS
 *   AllUnsorted            ->  ismENGINE_MONITOR_ALL_UNSORTED
 */
static int ismmon_getQueueStatType(char *statTypeStr) {
    int rc = -1;
    if ( !statTypeStr || (statTypeStr && *statTypeStr == '\0'))
        return rc;

    if ( *statTypeStr == 'B' || *statTypeStr == 'b' ) {
        if ( strcasecmp(statTypeStr, "BufferedMsgsHighest") == 0 ) {
            return ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS;
        } else if ( strcasecmp(statTypeStr, "BufferedMsgsLowest") == 0 ) {
            return ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS;
        } else if ( strcasecmp(statTypeStr, "BufferedPercentHighest") == 0 ) {
            return ismENGINE_MONITOR_HIGHEST_BUFFEREDPERCENT;
        } else if ( strcasecmp(statTypeStr, "BufferedPercentLowest") == 0 ) {
            return ismENGINE_MONITOR_LOWEST_BUFFEREDPERCENT;
        } else if ( strcasecmp(statTypeStr, "BufferedHWMPercentHighest") == 0 ) {
            return ismENGINE_MONITOR_HIGHEST_BUFFEREDHWMPERCENT;
        } else if ( strcasecmp(statTypeStr, "BufferedHWMPercentLowest") == 0 ) {
            return ismENGINE_MONITOR_LOWEST_BUFFEREDHWMPERCENT;
        }
    } else if ( *statTypeStr == 'C' || *statTypeStr == 'c' ) {
        if ( strcasecmp(statTypeStr, "ConsumedMsgsHighest") == 0 ) {
            return ismENGINE_MONITOR_HIGHEST_CONSUMEDMSGS;
        } else if ( strcasecmp(statTypeStr, "ConsumedMsgsLowest") == 0 ) {
            return ismENGINE_MONITOR_LOWEST_CONSUMEDMSGS;
        } else if ( strcasecmp(statTypeStr, "ConsumersHighest") == 0 ) {
            return ismENGINE_MONITOR_HIGHEST_CONSUMERS;
        } else if ( strcasecmp(statTypeStr, "ConsumersLowest") == 0 ) {
            return ismENGINE_MONITOR_LOWEST_CONSUMERS;
        }
    } else if ( *statTypeStr == 'E' || *statTypeStr == 'e' ) {
        if ( strcasecmp(statTypeStr, "ExpiredMsgsHighest") == 0 ) {
            return ismENGINE_MONITOR_HIGHEST_EXPIREDMSGS;
        } else if ( strcasecmp(statTypeStr, "ExpiredMsgsLowest") == 0 ) {
            return ismENGINE_MONITOR_LOWEST_EXPIREDMSGS;
        }
    } else if ( *statTypeStr == 'P' || *statTypeStr == 'p' ) {
        if ( strcasecmp(statTypeStr, "ProducedMsgsHighest") == 0 ) {
            return ismENGINE_MONITOR_HIGHEST_PRODUCEDMSGS;
        } else if ( strcasecmp(statTypeStr, "ProducedMsgsLowest") == 0 ) {
            return ismENGINE_MONITOR_LOWEST_PRODUCEDMSGS;
        } else if ( strcasecmp(statTypeStr, "ProducersHighest") == 0 ) {
            return ismENGINE_MONITOR_HIGHEST_PRODUCERS;
        } else if ( strcasecmp(statTypeStr, "ProducersLowest") == 0 ) {
            return ismENGINE_MONITOR_LOWEST_PRODUCERS;
        }
    } else if ( *statTypeStr == 'A' || *statTypeStr == 'a' ) {
        if ( strcasecmp(statTypeStr, "AllUnsorted") == 0 ) {
            return ismENGINE_MONITOR_ALL_UNSORTED;
        }
    }
    return rc;
}

#if 0
/**
 * Get number of Topic Monitor objects from the configuration file.
 */
static int ism_config_getNumberOfTopicMonitors(void) {
	int count = 0;
    ism_config_t * thandle = ism_config_getHandle(ISM_CONFIG_COMP_ENGINE, NULL);
    const char *pName = NULL;
    int i;

    if ( thandle ) {
        ism_prop_t *p = ism_config_getProperties(thandle, "TopicMonitor", NULL);
        if ( p ) {
            for (i=0; ism_common_getPropertyIndex(p, i, &pName) == 0; i++) {
            	count += 1;
            }
            ism_common_freeProperties(p);
        }
    }
    return count;
}
#endif

/**
 * Get the Topic Monitor objects and serialize and submit the event 
 * for external monitoring data
 */

void ism_monitoring_publishEngineTopicStatsExternal(ismMonitoringPublishType_t publishType,
													 ismEngineMonitorType_t engineMonType)
{
	int pcount=0;
	
	/*Get and Publish Topic Monitor Stats*/
	char msgPrefixBuf[256];
	concat_alloc_t prefixbuf = { NULL, 0, 0, 0, 0 };
	prefixbuf.buf = (char *) &msgPrefixBuf;
	prefixbuf.len = sizeof(msgPrefixBuf);

	uint64_t currentTime = ism_common_currentTimeNanos();
		
	ism_monitoring_getMsgExternalMonPrefix(ismMonObjectType_Topic,
										 currentTime, 
										 NULL,
										 &prefixbuf);
		
	 /* invoke engine API to get stats */
    ismEngine_TopicMonitor_t *results = NULL;
    uint32_t resultCount = 0;
 
    int rceng = ism_engine_getTopicMonitor( &results, &resultCount, engineMonType, 0, NULL);
    TRACE(8, "Total Topic Monitor to be published for external monitoring: %d  rc=%d.\n", resultCount, rceng);
    if(resultCount> 0){
		ismEngine_TopicMonitor_t * topicMonEngine = results;
		
	    for(pcount=0; pcount< resultCount; pcount++){

	    	char output_buf[1024];
	    	concat_alloc_t outputBuffer =  { NULL, 0, 0, 0, 0 };
	    	outputBuffer.buf = (char *)&output_buf;
	    	outputBuffer.len=1024;

			/* create serialized buffer from received monitoring data from engine */
			ismJsonSerializerData iSerUserData;
			ismSerializerData iSerData;
			memset(&iSerUserData,0, sizeof(ismJsonSerializerData));
			memset(&iSerData,0, sizeof(ismSerializerData));

			iSerUserData.outbuf = &outputBuffer;
			iSerUserData.isExternalMonitoring=1;
			iSerUserData.prefix = alloca(prefixbuf.used+1);
			memcpy(iSerUserData.prefix, prefixbuf.buf, prefixbuf.used);
			iSerUserData.prefix[prefixbuf.used]='\0';

			iSerData.serializer_userdata = &iSerUserData;

	    	 /* Serialize data */
	        ism_common_serializeMonJson(XismEngine_TopicMonitor_t, (void *)topicMonEngine, outputBuffer.buf, 2500, &iSerData );
		        
	        ism_monitoring_submitMonitoringEvent(	ismMonObjectType_Topic,
							   						NULL,
							   						0,
							   						(const char *)outputBuffer.buf,
							   						outputBuffer.used,
							   						publishType);
		        
		        
	    	topicMonEngine++;
	       	if(outputBuffer.inheap) ism_common_freeAllocBuffer(&outputBuffer);

	    }

    	ism_engine_freeTopicMonitor(results);
    }
	    
	    
   	if(prefixbuf.inheap) ism_common_freeAllocBuffer(&prefixbuf);
}


/**
 * Returns Topic stats types (refer to server_engine/include/engine.h for supported monitoring types:
 *
 *   PublishedMsgsHighest   ->  ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS
 *   PublishedMsgsLowest    ->  ismENGINE_MONITOR_LOWEST_PUBLISHEDMSGS
 *   SubscriptionsHighest   ->  ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS
 *   SubscriptionsLowest    ->  ismENGINE_MONITOR_LOWEST_SUBSCRIPTIONS
 *   RejectedMsgsHighest    ->  ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS
 *   RejectedMsgsLowest     ->  ismENGINE_MONITOR_LOWEST_REJECTEDMSGS
 *   FailedPublishesHighest ->  ismENGINE_MONITOR_HIGHEST_FAILEDPUBLISHES
 *   FailedPublishesLowest  ->  ismENGINE_MONITOR_LOWEST_FAILEDPUBLISHES
 *   AllUnsorted            ->  ismENGINE_MONITOR_ALL_UNSORTED
 */
static int ismmon_getTopicStatType(char *statTypeStr) {
    int rc = -1;
    if ( !statTypeStr || (statTypeStr && *statTypeStr == '\0'))
        return rc;

    if ( *statTypeStr == 'F' || *statTypeStr == 'f' ) {
        if ( strcasecmp(statTypeStr, "FailedPublishesHighest") == 0 ) {
            return ismENGINE_MONITOR_HIGHEST_FAILEDPUBLISHES;
        } else if ( strcasecmp(statTypeStr, "FailedPublishesLowest") == 0 ) {
            return ismENGINE_MONITOR_LOWEST_FAILEDPUBLISHES;
        }
    } else if ( *statTypeStr == 'P' || *statTypeStr == 'p' ) {
        if ( strcasecmp(statTypeStr, "PublishedMsgsHighest") == 0 ) {
            return ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS;
        } else if ( strcasecmp(statTypeStr, "PublishedMsgsLowest") == 0 ) {
            return ismENGINE_MONITOR_LOWEST_PUBLISHEDMSGS;
        }
    } else if ( *statTypeStr == 'R' || *statTypeStr == 'r' ) {
        if ( strcasecmp(statTypeStr, "RejectedMsgsHighest") == 0 ) {
            return ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS;
        } else if ( strcasecmp(statTypeStr, "RejectedMsgsLowest") == 0 ) {
            return ismENGINE_MONITOR_LOWEST_REJECTEDMSGS;
        }
    } else if ( *statTypeStr == 'S' || *statTypeStr == 's' ) {
        if ( strcasecmp(statTypeStr, "SubscriptionsHighest") == 0 ) {
            return ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS;
        } else if ( strcasecmp(statTypeStr, "SubscriptionsLowest") == 0 ) {
            return ismENGINE_MONITOR_LOWEST_SUBSCRIPTIONS;
        }
    } else if ( *statTypeStr == 'A' || *statTypeStr == 'a' ) {
        if ( strcasecmp(statTypeStr, "AllUnsorted") == 0) {
            return ismENGINE_MONITOR_ALL_UNSORTED;
        }
    }
    return rc;
}

/**
 * Returns MQTTClient stats types (refer to server_engine/include/engine.h for supported monitoring types:
 *
 *   LastConnectedTimeOldest->  ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME
 *   AllUnsorted            ->  ismENGINE_MONITOR_ALL_UNSORTED
 */
static int ismmon_getMQTTClientStatType(char *statTypeStr) {
    int rc = -1;
    if ( !statTypeStr || (statTypeStr && *statTypeStr == '\0'))
        return rc;

    if ( *statTypeStr == 'L' || *statTypeStr == 'l' ) {
        if ( strcasecmp(statTypeStr, "LastConnectedTimeOldest") == 0 ) {
            return ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME;
        }
    } else if ( *statTypeStr == 'A' || *statTypeStr == 'a' ) {
        if ( strcasecmp(statTypeStr, "AllUnsorted") == 0) {
            return ismENGINE_MONITOR_ALL_UNSORTED;
        }
    }

    return rc;
}

/**
 * Returns Transaction statType (refer to server_engine/include/engine.h for supported monitoring types:
 *
 *   LastStateChangeTime    ->  ismENGINE_MONITOR_OLDEST_STATECHANGE
 *   Unsorted (legacy)      ->  ismENGINE_MONITOR_ALL_UNSORTED
 *   AllUnsorted            ->  ismENGINE_MONITOR_ALL_UNSORTED
 */
static int ismmon_getTransactionStatType(char *statTypeStr) {
    int rc = -1;
    if ( !statTypeStr || (statTypeStr && *statTypeStr == '\0')) {
        return ismENGINE_MONITOR_OLDEST_STATECHANGE;
    } else  if ( *statTypeStr == 'L' || *statTypeStr == 'l' ) {
        if ( strcasecmp(statTypeStr, "LastStateChangeTime") == 0 ) {
            return ismENGINE_MONITOR_OLDEST_STATECHANGE;
        }
    } else if (*statTypeStr == 'U' || *statTypeStr == 'u') {
        if (strcasecmp(statTypeStr, "Unsorted") == 0 ) {
            return ismENGINE_MONITOR_ALL_UNSORTED;
        }
    } else if (*statTypeStr == 'A' || *statTypeStr == 'a') {
        if (strcasecmp(statTypeStr, "AllUnsorted") == 0 ) {
            return ismENGINE_MONITOR_ALL_UNSORTED;
        }
    }
    return rc;
}

static int ismmon_validateTransactionTranState(char *TranStateStr) {
    int rc = -1;
    if ( !TranStateStr || (TranStateStr && *TranStateStr == '\0'))
        return rc;

    if ( strcasecmp(TranStateStr, ismENGINE_MONITOR_FILTER_TRANSTATE_PREPARED) == 0 ) {
        rc = 0;
    } else if ( strcasecmp(TranStateStr, ismENGINE_MONITOR_FILTER_TRANSTATE_HEURISTIC_COMMIT) == 0 ) {
        rc = 0;
    } else if ( strcasecmp(TranStateStr, ismENGINE_MONITOR_FILTER_TRANSTATE_HEURISTIC_ROLLBACK) == 0 ) {
        rc = 0;
    } else if ( strcasecmp(TranStateStr, ismENGINE_MONITOR_FILTER_TRANSTATE_HEURISTIC) == 0 ) {
        rc = 0;
    } else if ( strcasecmp(TranStateStr, ismENGINE_MONITOR_FILTER_TRANSTATE_ALL) == 0 ) {
        rc = 0;
    }

    return rc;
}
/**
 * Returns Subscription statistics
 *
 * This function is called by monitoring action handler ism_monitoring_getEngineStats().
 * Monitoring action handler converts JSON query string into JSON object and
 * passed JSON object as an argument to this function. This function internally
 * uses Subscription stats API provided by Engine component to get results, add
 * results in output buffer.
 *
 * Input JSON string contains:
 *     SubscriptionName - Subscription Name (can include * match)
 *                 If not specified, use *
 *     TopicString - Topic string to match subscriptions
 *     ClientID    - Client ID to match subscriptions
 *     MessagingPolicy - Messaging policy to match subscriptions
 *     SubscriptionType - Type of subscription. Enum
 *                        Options: Durable,Nondurable,All
 *     StatType  - Statistics type - Enum
 *                 Unless specified, function will not return any result.
 *                 Options are:
 *                 - PublishedMsgsHighest
 *                 - PublishedMsgsLowest
 *                 - BufferedMsgsHighest
 *                 - BufferedMsgsLowest
 *                 - BufferedPercentHighest
 *                 - BufferedPercentLowest
 *                 - RejectedMsgsHighest
 *                 - RejectedMsgsLowest
 *     ResultCount - Number of returned results
 *                 Options are: 10, 25, 50, 100 - Default: 25
 *
 * Example query string created in WebUI/CLI:
 * [ { "Action":"Subscription","User":"admin","SubscriptionName":"Test",
 *     "TopicString":"TestTopic","ClientID":"TestClient",
 *     "SubscriptionType":"Durable","StatType":"BufferedMsgsLowest",
 *     "ResultCount":"25" } ]
 *
 *
 * This function intern uses Subscription stats API provided by Engine component
 * to get Queue statistics, serializes the monitoring results in output buffer.
 * Engine API used - ism_engine_getSubscriptionMonitor();
 *
 * Function Arguments:
 *
 * @param[in]  inputJSONObj  JSON object (created from query string)
 * @param[out] outputBuffer  Output buffer containing for required results
 *
 * Returns ISMRC_OK on success or ISMRC_*
 *
 * NOTE: Returned error messages need not be in message catalog
 *       - WebUI and CLI uses returned error number and text to
 *         identify error and create localized messages.
 *
 */

static int32_t ism_monitoring_getSubscriptionStats (
                 ism_json_parse_t *inputJSONObj,
                 concat_alloc_t * outputBuffer )
{
    int32_t rc = ISMRC_OK;
    ismEngineMonitorType_t type;
    const char * repl[3];
	char  msgID[12];
    char tmpbuf[128];
    char  lbuf[1024];
    memset(lbuf, 0, sizeof(lbuf));
    int xlen;

    if ( !inputJSONObj || !outputBuffer ) {
        /* TODO: Log error message */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;
        
        ism_monitoring_getMsgId(6509, msgID);
        
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]="ism_monitoring_getSubscriptionStats";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
        } else {
        	sprintf(tmpbuf, "A NULL argument was passed to the %s call.", "ism_monitoring_getSubscriptionStats");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        
        return rc;
    }

    /* Get input data from JSON object */
    char * SubName = (char *)ism_json_getString(inputJSONObj, "SubName");
    char * TopicString = (char *)ism_json_getString(inputJSONObj, "TopicString");
    char * ClientID = (char *)ism_json_getString(inputJSONObj, "ClientID");
    char * ResourceSetID = (char *)ism_json_getString(inputJSONObj, "ResourceSetID");
    char * SubType = (char *)ism_json_getString(inputJSONObj, "SubType");
    char * MessagingPolicy = (char *)ism_json_getString(inputJSONObj, "MessagingPolicy");
    char * StatTypeStr = (char *)ism_json_getString(inputJSONObj, "StatType");
    uint32_t maxResults = (uint32_t)ism_json_getInt(inputJSONObj, "ResultCount", 25);

    if ( !SubName || (SubName && *SubName == '\0') ) {
        /* TODO: Log error message and return invalid Subscription name - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_NameNotValid;
        
         ism_monitoring_getMsgId(6510, msgID);
        
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]= "SubName";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "The %s property is NULL or empty.", "SubName");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        
        
        return rc;
    }

    if ( !StatTypeStr || (StatTypeStr && *StatTypeStr == '\0') ) {
        /* TODO: Log error message and return invalid argument - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;
        
         ism_monitoring_getMsgId(6511, msgID);
        
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
        } else {
        	sprintf(tmpbuf, "StatType is NULL or empty.");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
   }

    /* Get monitoring type */
    rc = ismmon_getSubscriptionStatType(StatTypeStr);
    if ( rc == -1 )  {
        /* TODO: Log error message and return invalid argument - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;
        
        ism_monitoring_getMsgId(6512, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]=StatTypeStr;
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "Invalid StatType: %s.", StatTypeStr);
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        
        return rc;
    } else {
    	type = rc;
    }

    /* invoke engine API to get stats */
    ismEngine_SubscriptionMonitor_t *results = NULL;
    uint32_t resultCount = 0;
    ism_prop_t *filter = ism_common_newProperties(5);
    ism_field_t f;
    f.type = VT_String;
    /* TopicString filter */
    if (TopicString && *TopicString != '\0') {
        f.val.s = TopicString;
        ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_TOPICSTRING, &f);
    }
    /* ClientID filter */
    if (ClientID && *ClientID != '\0') {
        f.val.s = ClientID;
        ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_CLIENTID, &f);
    }
    /* Subscription name filter */
    if (SubName && *SubName != '\0') {
        f.val.s = SubName;
        ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_SUBNAME, &f);
    }
    /* ResourceSetID filter */
    if (ResourceSetID && *ResourceSetID != '\0') {
        f.val.s = ResourceSetID;
        ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_RESOURCESET_ID, &f);
    }
    /* Subscription type filter */
    if (SubType && *SubType != '\0') {
        f.val.s = SubType;
        ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_SUBTYPE, &f);
    }
    /* MessagingPolicy filter */
    if (MessagingPolicy && *MessagingPolicy != '\0') {
        f.val.s = MessagingPolicy;
        ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_MESSAGINGPOLICY, &f);
    }

    TRACE(9, "Get Subscription stats: SubName=%s TopicString=%s ClientID=%s SubType=%s\n", SubName, TopicString, ClientID, SubType);

    rc = ism_engine_getSubscriptionMonitor( &results, &resultCount, type, maxResults, filter);

    /* free resources */
    ism_common_freeProperties(filter);

    if ( rc != ISMRC_OK ) {
        /* TODO: Log Message */

        /* Set returned error string */
        rc = ISMRC_NotFound;
        ism_monitoring_getMsgId(6513, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			repl[0]="subscription";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "Failed to get the %s monitoring data.", "subscription");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        
        return rc;
    }

    /* Create return result buffer */
    if ( resultCount <= 0 ) {
        /* TODO: Log Message */

        /* Set returned error string */
        rc = ISMRC_NotFound;
        
        ism_monitoring_getMsgId(6508, msgID);
        
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
        } else {
        	sprintf(tmpbuf, "No monitoring data is found for the specified command.");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        
        return rc;
    }

    /* create serialized buffer from received monitoring data from engine */
    ismJsonSerializerData iSerUserData;
    ismSerializerData iSerData;
    memset(&iSerUserData,0, sizeof(ismJsonSerializerData));
    memset(&iSerData,0, sizeof(ismSerializerData));
    ismEngine_SubscriptionMonitor_t * subsMonEngine = results;
    int i;
    int addNext = 0;

    iSerUserData.isExternalMonitoring = 0;
    iSerUserData.outbuf = outputBuffer;
    iSerData.serializer_userdata = &iSerUserData;

    ism_common_allocBufferCopyLen(iSerUserData.outbuf, "[", 1);

    for (i=0; i < resultCount; i++) {
        if ( addNext == 1 ) {
            ism_common_allocBufferCopyLen(iSerUserData.outbuf, ",", 1);
        }

        /* Serialize data */
        ism_common_serializeMonJson(XismEngine_SubscriptionMonitor_t, (void *)subsMonEngine, outputBuffer->buf, 2500, &iSerData );

        subsMonEngine++;
        addNext = 1;
    }

    ism_common_allocBufferCopyLen(iSerUserData.outbuf, "]", 1);

    ism_engine_freeSubscriptionMonitor(results);

    return rc;
}

/**
 * Returns Stats for a ResourceSet matching config item ResourceSetDescriptor
 *
 * This function is called by monitoring action handler.
 * Monitoring action handler converts JSON query string into JSON object and
 * passed JSON object as an argument to this function. This function internally
 * uses Resource Stats API provided by Engine component to get results, add
 * results in output buffer.
 *
 *  Input JSON string contains:
 *     ResourceSetID - (can include * match)
 *                 If not specified, default is *
 *     StatType  - Statistics type - Enum
 *                 Unless specified, function will not return any result.
 *                 Options are:
 *                 - AllUnsorted
 *                 - TotalMemoryBytesHighest
 *                 - SubscriptionsHighest
 *                 - PersistentNonSharedSubscriptionsHighest
 *                 - NonpersistentNonSharedSubscriptionsHighest
 *                 - PersistentSharedSubscriptionsHighest
 *                 - NonpersistentSharedSubscriptionsHighest
 *                 - BufferedMsgsHighest
 *                 - DiscardedMsgsHighest
 *                 - RetainedMsgsHighest
 *                 - BufferedMsgBytesHighest
 *                 - WillMsgsHighest
 *                 - PersistentBufferedMsgBytesHighest
 *                 - NonpersistentBufferedMsgBytesHighest
 *                 - RetainedMsgBytesHighest
 *                 - WillMsgBytesHighest
 *                 - PersistentWillMsgBytesHighest
 *                 - NonpersistentWillMsgBytesHighest
 *                 - PublishedMsgsHighest
 *                 - QoS0PublishedMsgsHighest
 *                 - QoS1PublishedMsgsHighest
 *                 - QoS2PublishedMsgsHighest
 *                 - PublishedMsgBytesHighest
 *                 - QoS0PublishedMsgBytesHighest
 *                 - QoS1PublishedMsgBytesHighest
 *                 - QoS2PublishedMsgBytesHighest
 *                 - MaxPublishRecipientsHighest
 *                 - ConnectionsHighest
 *                 - ActiveClientsHighest
 *                 - ActivePersistentClientsHighest
 *                 - ActiveNonpersistentClientsHighest
 *                 - PersistentClientStatesHighest
 *
 *     ResultType  - Result output type - Enum
 *                 Options are:
 *                 - Full
 *                 - Standard
 *                 - TotalMemoryBytes
 *
 *     ResultCount - Number of returned results
 *                 Options are: 10, 25, 50, 100 - Default: 25
 *
 * Example query string created in WebUI/CLI:
 * [ { "Action":"ResourceSet","User":"admin","ResourceSetID":"*",
 *     "StatType":"TotalMemoryBytesHighest", "ResultType":"Full", "ResultCount":"25" } ]
 */
static int32_t ism_monitoring_getResourceSetStats (
                 ism_json_parse_t *inputJSONObj,
                 concat_alloc_t * outputBuffer )
{
    int rc = ISMRC_OK;
    ismEngineMonitorType_t type;
    ismEngine_ResourceSetMonitor_t *results;

    const char * repl[3];
    char  msgID[12];
    char tmpbuf[128];
    char  lbuf[1024];
    memset(lbuf, 0, sizeof(lbuf));
    int xlen;

    if ( !inputJSONObj || !outputBuffer ) {
        /* Set returned error string */
        rc = ISMRC_ArgNotValid;

        ism_monitoring_getMsgId(6509, msgID);

        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
            repl[0]="ism_monitoring_getResourceSetStats";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
        } else {
            sprintf(tmpbuf, "A NULL argument was passed to the %s call.", "ism_monitoring_getResourceSetStats");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);

        return rc;
    }

    /* Get input data from JSON object */
    char * ResourceSetID = (char *)ism_json_getString(inputJSONObj, "ResourceSetID");
    char * StatTypeStr = (char *)ism_json_getString(inputJSONObj, "StatType");
    char * ResultTypeStr = (char *)ism_json_getString(inputJSONObj, "ResultType");
    uint32_t maxResults = (uint32_t)ism_json_getInt(inputJSONObj, "ResultCount", 25);
    uint32_t resultCount = 0;
    const ism_mon_obj_def_t *outputType = NULL;

    if ( !ResourceSetID || *ResourceSetID == '\0' ) {
        /* TODO: Log error message and return invalid Subscription name - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_NameNotValid;

        ism_monitoring_getMsgId(6510, msgID);

        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
            repl[0]= "ResourceSetID";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
            sprintf(tmpbuf, "The %s property is NULL or empty.", "ResourceSetID");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);

        return rc;
    }

    if ( !StatTypeStr || *StatTypeStr == '\0' ) {
        /* TODO: Log error message and return invalid argument - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;

         ism_monitoring_getMsgId(6511, msgID);

        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
        } else {
            sprintf(tmpbuf, "StatType is NULL or empty.");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    }

    /* Get monitoring type */
    rc = ismmon_getResourceSetStatType(StatTypeStr);
    if ( rc == -1 )  {
        /* TODO: Log error message and return invalid argument - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;

        ism_monitoring_getMsgId(6512, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
            repl[0]=StatTypeStr;
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
            sprintf(tmpbuf, "Invalid StatType: %s.", StatTypeStr);
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);

        return rc;
    } else {
        type = rc;
    }

    /* Get ResultType */
    if ( !ResultTypeStr || *ResultTypeStr == '\0' ) {
        rc = ISMRC_ArgNotValid;

        ism_monitoring_getMsgId(6519, msgID);

        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
        } else {
            sprintf(tmpbuf, "ResultType is NULL or empty.");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    } else {
        if (*ResultTypeStr == 'F' || *ResultTypeStr == 'f') {
            if ( strcasecmp(ResultTypeStr, "Full") == 0 ) {
                outputType = XismEngine_ResourceSetMonitor_t;
            }
        } else if (*ResultTypeStr == 'S' || *ResultTypeStr == 's') {
            if ( strcasecmp(ResultTypeStr, "Standard") == 0 ) {
                outputType = X_STANDARD_SUBSET_ismEngine_ResourceSetMonitor_t;
            }
        } else if (*ResultTypeStr == 'T' || *ResultTypeStr == 't') {
            if ( strcasecmp(ResultTypeStr, "TotalMemoryBytes") == 0 ) {
                outputType = X_TOTALMEMORYBYTES_SUBSET_ismEngine_ResourceSetMonitor_t;
            }
        } else {
            rc = ISMRC_ArgNotValid;

            ism_monitoring_getMsgId(6520, msgID);

            if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
                repl[0]= ResultTypeStr;
                ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
            } else {
                sprintf(tmpbuf, "Invalid ResultType: %s.", ResultTypeStr);
            }
            ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
            return rc;
        }
    }

    ism_prop_t * filter = ism_common_newProperties(1);
    ism_field_t f;
    f.type = VT_String;
    /* ResourceSet filtesr */
    if (ResourceSetID && *ResourceSetID != '\0') {
        f.val.s = ResourceSetID;
        ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_RESOURCESET_ID, &f);
    }

    TRACE(9, "Get ResourceSet Stats Filter: ResourceSetID=%s\n", ResourceSetID);

    rc = ism_engine_getResourceSetMonitor(&results, &resultCount, type, maxResults, filter);

    ism_common_freeProperties(filter);

    if ( rc != ISMRC_OK ) {
        /* TODO: Log Message */

        /* Set returned error string */
        rc = ISMRC_NotFound;
        ism_monitoring_getMsgId(6513, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
            repl[0]="resourceSet";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
            sprintf(tmpbuf, "Failed to get the %s monitoring data.", "resourceSet");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);

        return rc;
    }

    /* Create return result buffer */
    if ( resultCount <= 0 ) {
        /* TODO: Log Message */

        /* Set returned error string */
        rc = ISMRC_NotFound;

        ism_monitoring_getMsgId(6508, msgID);

        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
        } else {
            sprintf(tmpbuf, "No monitoring data is found for the specified command.");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);

        return rc;
    }

    /* create serialized buffer from received monitoring data from engine */
    ismJsonSerializerData iSerUserData;
    ismSerializerData iSerData;
    memset(&iSerUserData,0, sizeof(ismJsonSerializerData));
    memset(&iSerData,0, sizeof(ismSerializerData));
    ismEngine_ResourceSetMonitor_t * resourceSetMonEngine = results;
    int i;
    int addNext = 0;

    iSerUserData.isExternalMonitoring = 0;
    iSerUserData.outbuf = outputBuffer;
    iSerData.serializer_userdata = &iSerUserData;

    ism_common_allocBufferCopyLen(iSerUserData.outbuf, "[", 1);

    for (i=0; i < resultCount; i++) {
        if ( addNext == 1 ) {
            ism_common_allocBufferCopyLen(iSerUserData.outbuf, ",", 1);
        }

        /* Serialize data */
        ism_common_serializeMonJson(outputType, (void *)resourceSetMonEngine, outputBuffer->buf, 2500, &iSerData );

        resourceSetMonEngine++;
        addNext = 1;
    }

    ism_common_allocBufferCopyLen(iSerUserData.outbuf, "]", 1);

    if (results)
        ism_engine_freeResourceSetMonitor(results);

    return rc;
}

/**
 * Returns Queue stats
 *
 * This function is called by monitoring action handler.
 * Monitoring action handler converts JSON query string into JSON object and
 * passed JSON object as an argument to this function. This function internally
 * uses Queue stats API provided by Engine component to get results, add
 * results in output buffer.
 *
 * Input JSON string contains:
 *     QueueName - Queue Name (can include * match)
 *                 If not specified, use *
 *     StatType  - Statistics type - Enum
 *                 Unless specified, function will not return any result.
 *                 Options are:
 *                 - BufferedMsgsHighest
 *                 - BufferedMsgsLowest
 *                 - BufferedPercentHighest
 *                 - BufferedPercentLowest
 *                 - ProducedMsgsHighest
 *                 - ProducedMsgsLowest
 *                 - ConsumedMsgsHighest
 *                 - ConsumedMsgsLowest
 *                 - ConsumersHighest
 *                 - ConsumersLowest
 *                 - ProducersHighest
 *                 - ProducersLowest
 *     ResultCount - Number of returned results
 *                 Options are: 10, 25, 50, 100 - Default: 25
 *
 * Example query string created in WebUI/CLI:
 * [ { "Action":"Queue","User":"admin","QueueName":"TestQ",
 *     "StatType":"ProducedMsgsLowest","ResultCount":"25" } ]
 *
 *
 * Function Arguments:
 *
 * @param[in]  inputJSONObj  JSON object (created from query string)
 * @param[out] outputBuffer  Output buffer containing for required results
 *
 * Returns ISMRC_OK on success or ISMRC_*
 *
 */

XAPI int32_t ism_monitoring_getQueueStats (
                 ism_json_parse_t *inputJSONObj,
                 concat_alloc_t * outputBuffer )
{
    int32_t rc = ISMRC_OK;
    ismEngineMonitorType_t type;
    const char * repl[3];
	char  msgID[12];
    char tmpbuf[128];
    char  lbuf[1024];
    memset(lbuf, 0, sizeof(lbuf));
    int xlen;

    if ( !inputJSONObj || !outputBuffer ) {
        /* TODO: Log error message */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;
        
        ism_monitoring_getMsgId(6509, msgID);
        
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]="ism_monitoring_getQueueStats";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "A NULL argument was passed to the %s call.", "ism_monitoring_getQueueStats");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        
        return rc;
    }

    /* Get input data from JSON object */
    char * QueueName = (char *)ism_json_getString(inputJSONObj, "Name");
    char * StatTypeStr = (char *)ism_json_getString(inputJSONObj, "StatType");
    uint32_t maxResults = (uint32_t)ism_json_getInt(inputJSONObj, "ResultCount", 25);

    if ( !QueueName || (QueueName && *QueueName == '\0') ) {
        /* TODO: Log error message and return invalid Queue name - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_NameNotValid;
        ism_monitoring_getMsgId(6510, msgID);
        
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]= "QueueName";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "The %s property is NULL or empty.", "Name");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    }

    if ( !StatTypeStr || (StatTypeStr && *StatTypeStr == '\0') ) {
        /* TODO: Log error message and return invalid argument - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;
          ism_monitoring_getMsgId(6511, msgID);
        
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
        } else {
        	sprintf(tmpbuf, "StatType is NULL or empty.");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
   }

    /* Get monitoring type */
    rc = ismmon_getQueueStatType(StatTypeStr);
    if ( rc == -1 )  {
        /* TODO: Log error message and return invalid argument - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;
     	ism_monitoring_getMsgId(6512, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]=StatTypeStr;
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "Invalid StatType: %s.", StatTypeStr);
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        
        return rc;
    } else {
        type = rc;
    }

    /* invoke engine API to get stats */
    ismEngine_QueueMonitor_t *results = NULL;
    uint32_t resultCount = 0;
    ism_prop_t *filter = ism_common_newProperties(5);
    ism_field_t f;
    f.type = VT_String;
    /* QueueName filter */
    if (QueueName && *QueueName != '\0') {
        f.val.s = QueueName;
        ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_QUEUENAME, &f);
    }

    TRACE(9, "Get Queue stats: QueueName=%s\n", QueueName);

    rc = ism_engine_getQueueMonitor( &results, &resultCount, type, maxResults, filter);

    /* free resources */
    ism_common_freeProperties(filter);

    if ( rc != ISMRC_OK ) {
        /* TODO: Log Message */
 		rc = ISMRC_NotFound;
        ism_monitoring_getMsgId(6513, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			repl[0]="queue";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "Failed to get the %s monitoring data.", "queue");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    }

    /* Create return result buffer */
    if ( resultCount <= 0 ) {
        /* TODO: Log Message */

        /* Set returned error string */
       rc = ISMRC_NotFound;
        
        ism_monitoring_getMsgId(6508, msgID);
        
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
        } else {
        	sprintf(tmpbuf, "No monitoring data is found for the specified command.");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    }

    /* create serialized buffer from received monitoring data from engine */
    ismJsonSerializerData iSerUserData;
    ismSerializerData iSerData;
    memset(&iSerUserData,0, sizeof(ismJsonSerializerData));
    memset(&iSerData,0, sizeof(ismSerializerData));
    ismEngine_QueueMonitor_t * queueMonEngine = results;
    int i;
    int addNext = 0;

    iSerUserData.isExternalMonitoring = 0;
    iSerUserData.outbuf = outputBuffer;
    iSerData.serializer_userdata = &iSerUserData;

    ism_common_allocBufferCopyLen(iSerUserData.outbuf, "[", 1);

    for (i=0; i < resultCount; i++) {
        if ( addNext == 1 ) {
            ism_common_allocBufferCopyLen(iSerUserData.outbuf, ",", 1);
        }

        /* Serialize data */
        ism_common_serializeMonJson(XismEngine_QueueMonitor_t, (void *)queueMonEngine, outputBuffer->buf, 2500, &iSerData );

        queueMonEngine++;
        addNext = 1;
    }

    ism_common_allocBufferCopyLen(iSerUserData.outbuf, "]", 1);

    ism_engine_freeQueueMonitor(results);

    return rc;
}


/**
 * Returns Topic stats
 *
 * This function is called by monitoring action handler.
 * Monitoring action handler converts JSON query string into JSON object and
 * passed JSON object as an argument to this function. This function internally
 * uses Topic stats API provided by Engine component to get results, add
 * results in output buffer.
 *
 * Input JSON string contains:
 *     TopicString - Topic string (can include * match)
 *                 If not specified, use *
 *     StatType  - Statistics type - Enum
 *                 Unless specified, function will not return any result,
 *                 if ResultCount is not "All".
 *                 Options are:
 *                 - PublishedMsgsHighest
 *                 - PublishedMsgsLowest
 *                 - SubscriptionsHighest
 *                 - SubscriptionsLowest
 *                 - RejectedMsgsHighest
 *                 - RejectedMsgsLowest
 *                 - FailedPublishesHighest
 *                 - FailedPublishesLowest
 *     ResultCount - Number of returned results
 *                 Options are: 10, 25, 50, 100, All - Default: 25
 *                 If All is specified, the function will return
 *                 unsorted list.
 *
 * Example query string created in WebUI/CLI:
 * [ { "Action":"Topic","User":"admin","TopicString":"T*",
 *     "StatType":"PublishedMsgsHighest","ResultCount":"25" } ]
 *
 *
 * Function Arguments:
 *
 * @param[in]  inputJSONObj  JSON object (created from query string)
 * @param[out] outputBuffer  Output buffer containing for required results
 *
 * Returns ISMRC_OK on success or ISMRC_*
 *
 */

XAPI int32_t ism_monitoring_getTopicStats (
                 ism_json_parse_t *inputJSONObj,
                 concat_alloc_t * outputBuffer )
{
    int32_t rc = ISMRC_OK;
    char rbuf[256];
    ismEngineMonitorType_t type;
    const char * repl[3];
	char  msgID[12];
    char tmpbuf[128];
    char  lbuf[1024];
    memset(lbuf, 0, sizeof(lbuf));
    int xlen;
    

    if ( !inputJSONObj || !outputBuffer ) {
        /* TODO: Log error message */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;
          ism_monitoring_getMsgId(6509, msgID);
        
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]="ism_monitoring_getTopicStats";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "A NULL argument was passed to the %s call.", "ism_monitoring_getTopicStats");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    }

    /* Get input data from JSON object */
    char * TopicString = (char *)ism_json_getString(inputJSONObj, "TopicString");
    char * StatTypeStr = (char *)ism_json_getString(inputJSONObj, "StatType");
    char * maxResultsStr = (char *)ism_json_getString(inputJSONObj, "ResultCount");
    uint32_t maxResults = 25;

    if ( maxResultsStr && *maxResultsStr != '\0' ) {
    	if (*maxResultsStr == 'A') {
    		maxResults = 0;
    	} else {
    		maxResults = atoi(maxResultsStr);
    	}
    }

    if ( !TopicString || (TopicString && *TopicString == '\0') ) {
        /* TODO: Log error message and return invalid Topic string - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_NameNotValid;
       ism_monitoring_getMsgId(6510, msgID);
        
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]= "TopicString";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "The %s property is NULL or empty.", "TopicString");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    }

    if ( !StatTypeStr || (StatTypeStr && *StatTypeStr == '\0') ) {
    	/* TODO: Log error message and return invalid argument - may have to create an return error code */

    	/* Set returned error string */
    	rc = ISMRC_ArgNotValid;
    	ism_monitoring_getMsgId(6512, msgID);
    	if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
    		repl[0]=StatTypeStr;
    		ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
    	} else {
    		sprintf(tmpbuf, "Invalid StatType: (null).");
    	}
    	ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);

    	return rc;
    }

    /* Get monitoring type, if maxResultsStr is not 0 */
    if ( maxResults > 0 ) {
        rc = ismmon_getTopicStatType(StatTypeStr);
        if ( rc == -1 )  {
            /* Set returned error string */
            rc = ISMRC_ArgNotValid;
            sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"Invalid StatType: %s\" }", rc, StatTypeStr);
            ism_common_allocBufferCopyLen(outputBuffer,rbuf,strlen(rbuf));
            return rc;
        } else {
    	    type = rc;
        }
    } else {
    	type = ismENGINE_MONITOR_ALL_UNSORTED;
    }

    /* invoke engine API to get stats */
    ismEngine_TopicMonitor_t *results = NULL;
    uint32_t resultCount = 0;
    ism_prop_t *filter = ism_common_newProperties(5);
    ism_field_t f;
    f.type = VT_String;
    /* TopicString filter */
    if (TopicString && *TopicString != '\0') {
        f.val.s = TopicString;
        ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_TOPICSTRING, &f);
    }

    TRACE(9, "Get Topic stats: TopicString=%s\n", TopicString);

    rc = ism_engine_getTopicMonitor( &results, &resultCount, type, maxResults, filter);

    /* free resources */
    ism_common_freeProperties(filter);

    if ( rc != ISMRC_OK ) {
        /* TODO: Log Message */

        /* Set returned error string */
      	rc = ISMRC_NotFound;
        ism_monitoring_getMsgId(6513, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			repl[0]="topic";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "Failed to get the %s monitoring data.", "topic");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    }

    /* Create return result buffer */
    if ( resultCount <= 0 ) {
        /* TODO: Log Message */

        /* Set returned error string */
        rc = ISMRC_NotFound;
        ism_monitoring_getMsgId(6508, msgID);
        
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
        } else {
        	sprintf(tmpbuf, "No monitoring data is found for the specified command.");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    }

    /* create serialized buffer from received monitoring data from engine */
    ismJsonSerializerData iSerUserData;
    ismSerializerData iSerData;
    memset(&iSerUserData, 0, sizeof(ismJsonSerializerData));
    memset(&iSerData, 0, sizeof(ismSerializerData));
    ismEngine_TopicMonitor_t * topicMonEngine = results;
    int i;
    int addNext = 0;

    iSerUserData.isExternalMonitoring = 0;
    iSerUserData.outbuf = outputBuffer;
    iSerData.serializer_userdata = &iSerUserData;

    ism_common_allocBufferCopyLen(iSerUserData.outbuf, "[", 1);

    for (i=0; i < resultCount; i++) {
        if ( addNext == 1 ) {
            ism_common_allocBufferCopyLen(iSerUserData.outbuf, ",", 1);
        }

        /* Serialize data */
        ism_common_serializeMonJson(XismEngine_TopicMonitor_t, (void *)topicMonEngine, outputBuffer->buf, 2500, &iSerData );

        topicMonEngine++;
        addNext = 1;
    }

    ism_common_allocBufferCopyLen(iSerUserData.outbuf, "]", 1);

    ism_engine_freeTopicMonitor(results);

    return rc;
}


/**
 * Returns MQTT Client stats
 *
 * This function is called by monitoring action handler.
 * Monitoring action handler converts JSON query string into JSON object and
 * passed JSON object as an argument to this function. This function internally
 * uses MQTT Client stats API provided by Engine component to get results, add
 * results in output buffer.
 *
 * Input JSON string contains:
 *     ClientID    - Client ID to match
 *                 If not specified, use *
 *     StatType  - Statistics type - Enum
 *                 Unless specified, function will not return any result.
 *                 Options are:
 *                 - LastConnectedTimeOldest
 *     ResultCount - Number of returned results
 *                 Options are: 10, 25, 50, 100 - Default: 25
 *
 * Example query string created in WebUI/CLI:
 * [ { "Action":"MQTTClient","User":"admin","ClientID":"T*",
 *     "StatType":"LastConnectedTimeOldest","ResultCount":"25" } ]
 *
 *
 * Function Arguments:
 *
 * @param[in]  inputJSONObj  JSON object (created from query string)
 * @param[out] outputBuffer  Output buffer containing for required results
 *
 * Returns ISMRC_OK on success or ISMRC_*
 *
 */

XAPI int32_t ism_monitoring_getMQTTClientStats (
                 ism_json_parse_t *inputJSONObj,
                 concat_alloc_t * outputBuffer )
{
    int32_t rc = ISMRC_OK;
    ismEngineMonitorType_t type;
    const char * repl[3];
	char  msgID[12];
    char tmpbuf[128];
    char  lbuf[1024];
    memset(lbuf, 0, sizeof(lbuf));
    int xlen;

    if ( !inputJSONObj || !outputBuffer ) {
        /* TODO: Log error message */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;
        ism_monitoring_getMsgId(6509, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]="ism_monitoring_getMQTTClientStats";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "A NULL argument was passed to the %s call.", "ism_monitoring_getMQTTClientStats");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    }

    /* Get input data from JSON object */
    char * ClientID = (char *)ism_json_getString(inputJSONObj, "ClientID");
    char * StatTypeStr = (char *)ism_json_getString(inputJSONObj, "StatType");
    char * ResourceSetID = (char *)ism_json_getString(inputJSONObj, "ResourceSetID");
    char * ConnectionState = (char *)ism_json_getString(inputJSONObj, "ConnectionState");
    char * Protocol = (char *)ism_json_getString(inputJSONObj, "Protocol");
    uint32_t maxResults = (uint32_t)ism_json_getInt(inputJSONObj, "ResultCount", 25);

    if ( !ClientID || (ClientID && *ClientID == '\0') ) {
        /* TODO: Log error message and return invalid Client ID - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_NameNotValid;
        ism_monitoring_getMsgId(6510, msgID);
        
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]= "ClientID";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "The %s property is NULL or empty.", "ClientID");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    }

    if ( !StatTypeStr || (StatTypeStr && *StatTypeStr == '\0') ) {
    	/* TODO: Log error message and return invalid argument - may have to create an return error code */

    	/* Set returned error string */
    	rc = ISMRC_ArgNotValid;
    	ism_monitoring_getMsgId(6512, msgID);
    	if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
    		repl[0]=StatTypeStr;
    		ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
    	} else {
    		sprintf(tmpbuf, "Invalid StatType: (null).");
    	}
    	ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);

    	return rc;
    }

    /* Get monitoring type */
    rc = ismmon_getMQTTClientStatType(StatTypeStr);
    if ( rc == -1 )  {
        /* TODO: Log error message and return invalid argument - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;
      	ism_monitoring_getMsgId(6512, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]=StatTypeStr;
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "Invalid StatType: %s.", StatTypeStr);
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        
        return rc;
    } else {
    	type = rc;
    }

    /* invoke engine API to get stats */
    ismEngine_ClientStateMonitor_t *results = NULL;
    uint32_t resultCount = 0;
    ism_prop_t *filter = ism_common_newProperties(5);
    ism_field_t f;
    f.type = VT_String;
    /* ClientID filter */
    if (ClientID && *ClientID != '\0') {
        f.val.s = ClientID;
        ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_CLIENTID, &f);
    }
    /* ResourceSetID filter */
    if (ResourceSetID && *ResourceSetID != '\0') {
        f.val.s = ResourceSetID;
        ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_RESOURCESET_ID, &f);
    }
    /* ConnectionState filter */
    if (ConnectionState && *ConnectionState != '\0') {
        f.val.s = ConnectionState;
        ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_CONNECTIONSTATE, &f);
    }
    /* Protocol filter */
    if (Protocol && *Protocol != '\0') {
        f.val.s = Protocol;
        ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_PROTOCOLID, &f);
    }
    TRACE(9, "Get MQTT Client stats: ClientID=%s\n", ClientID);

    rc = ism_engine_getClientStateMonitor( &results, &resultCount, type, maxResults, filter);

    /* free resources */
    ism_common_freeProperties(filter);

    if ( rc != ISMRC_OK ) {
        /* TODO: Log Message */

       rc = ISMRC_NotFound;
        ism_monitoring_getMsgId(6513, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			repl[0]="topic";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "Failed to get the %s monitoring data.", "topic");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        
        return rc;
    }

    /* Create return result buffer */
    if ( resultCount <= 0 ) {
        /* TODO: Log Message */

        /* Set returned error string */
        rc = ISMRC_NotFound;
      	ism_monitoring_getMsgId(6508, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
        } else {
        	sprintf(tmpbuf, "No monitoring data is found for the specified command.");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    }

    /* create serialized buffer from received monitoring data from engine */
    ismJsonSerializerData iSerUserData;
    ismSerializerData iSerData;
    memset(&iSerUserData,0, sizeof(ismJsonSerializerData));
    memset(&iSerData,0, sizeof(ismSerializerData));
    ismEngine_ClientStateMonitor_t * clientMonEngine = results;
    int i;
    int addNext = 0;

    iSerUserData.isExternalMonitoring = 0;
    iSerUserData.outbuf = outputBuffer;
    iSerData.serializer_userdata = &iSerUserData;

    ism_common_allocBufferCopyLen(iSerUserData.outbuf, "[", 1);

    for (i=0; i < resultCount; i++) {
        if ( addNext == 1 ) {
            ism_common_allocBufferCopyLen(iSerUserData.outbuf, ",", 1);
        }

        /* Serialize data */
        ism_common_serializeMonJson(XismEngine_ClientStateMonitor_t, (void *)clientMonEngine, outputBuffer->buf, 2500, &iSerData );

        clientMonEngine++;
        addNext = 1;
    }

    ism_common_allocBufferCopyLen(iSerUserData.outbuf, "]", 1);

    ism_engine_freeClientStateMonitor(results);

    return rc;
}

/**
 * Returns Transaction stats
 *
 * This function is called by monitoring action handler.
 * Monitoring action handler converts JSON query string into JSON object and
 * passed JSON object as an argument to this function. This function internally
 * uses MQTT Client stats API provided by Engine component to get results, add
 * results in output buffer.
 *
 * Input JSON string contains:
 *     XID       - Transaction ID to match
 *                 If not specified, use *
 *     StatType  - Statistics type - Enum
 *                 The defaul type is LastStateChangeTime.
 *                 Options are:
 *                 - LastStateChangeTime
 *                 - Unsorted
 *     TranState - The state of transactions that are to be included in the results.
 *                 The defaul TranState is All.
 *                 Options are:
 *                 - Prepared
 *                 - HeuristicCommit
 *                 - HeuristicRollback
 *                 - Heuristic
 *                 - All
 *     ResultCount - Number of returned results
 *                 Options are: 10, 25, 50, 100 - Default: 25
 *
 * Example query string created in WebUI/CLI:
 * [ { "Action":"Transaction","User":"admin","Xid":"57415344:0000014247FEBB3D000000012008CC78A1A981840C3350B9E23A3C2EFB8E5A6D864A89C0:00000001",
 *     "StatType":"LastStateChangeTime","ResultCount":"25" } ]
 *
 *
 * Function Arguments:
 *
 * @param[in]  inputJSONObj  JSON object (created from query string)
 * @param[out] outputBuffer  Output buffer containing for required results
 *
 * Returns ISMRC_OK on success or ISMRC_*
 *
 */

XAPI int32_t ism_monitoring_getTransactionStats (
                 ism_json_parse_t *inputJSONObj,
                 concat_alloc_t * outputBuffer )
{
    int32_t rc = ISMRC_OK;
    ismEngineMonitorType_t type;
    char *xidStr = NULL;
    char *StatTypeStr = NULL;
    char *TranStateStr = NULL;
    uint32_t maxResults = 0;
    ism_xid_t xid;
    const char * repl[3];
	char  msgID[12];
    char tmpbuf[128];
    char  lbuf[1024];
    memset(lbuf, 0, sizeof(lbuf));
    int xlen;
    

    if ( !inputJSONObj || !outputBuffer ) {
        /* TODO: Log error message */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;
        ism_monitoring_getMsgId(6509, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]="ism_monitoring_getTransactionStats";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "A NULL argument was passed to the %s call.", "ism_monitoring_getTransactionStats");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        
        return rc;
    }

    memset(&xid, 0, sizeof(ism_xid_t));

    /* Get input data from JSON object */
    xidStr = (char *)ism_json_getString(inputJSONObj, "XID");
    StatTypeStr = (char *)ism_json_getString(inputJSONObj, "StatType");
    TranStateStr = (char *)ism_json_getString(inputJSONObj, "TranState");
    maxResults = (uint32_t)ism_json_getInt(inputJSONObj, "ResultCount", 25);

    if ( xidStr && strstr(xidStr, "*") == NULL) {
    	rc = ism_common_StringToXid(xidStr, &xid);
    	if (rc) {
			/* Set returned error string */
    		ism_common_setError(rc);
			ism_monitoring_getMsgId(6510, msgID);
        
	        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
	        	repl[0]= "XID";
	            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
	        } else {
	        	sprintf(tmpbuf, "The %s property is NULL or empty.", "XID");
	        }
	        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
			return rc;
    	}
    }

    if ( !StatTypeStr || (StatTypeStr && *StatTypeStr == '\0') ) {
    	/* TODO: Log error message and return invalid argument - may have to create an return error code */

    	/* Set returned error string */
    	rc = ISMRC_ArgNotValid;
    	ism_monitoring_getMsgId(6512, msgID);
    	if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
    		repl[0]=StatTypeStr;
    		ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
    	} else {
    		sprintf(tmpbuf, "Invalid StatType: (null).");
    	}
    	ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
    	return rc;
    }

    /* Get monitoring type */
    rc = ismmon_getTransactionStatType(StatTypeStr);
    if ( rc == -1 )  {
        /* TODO: Log error message and return invalid argument - may have to create an return error code */

        /* Set returned error string */
        rc = ISMRC_ArgNotValid;
         ism_monitoring_getMsgId(6512, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]=StatTypeStr;
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "Invalid StatType: %s.", StatTypeStr);
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    } else {
    	type = rc;
    }

    /* validate TranState */
    if (TranStateStr) {
		rc = ismmon_validateTransactionTranState(TranStateStr);
		if ( rc == -1 )  {
			/* TODO: Log error message and return invalid argument - may have to create an return error code */

			/* Set returned error string */
			rc = ISMRC_ArgNotValid;
			 ism_monitoring_getMsgId(6512, msgID);
	        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
	        	repl[0]=StatTypeStr? StatTypeStr:"LastStateChangeTime";
	            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
	        } else {
	        	sprintf(tmpbuf, "Invalid StatType: %s.", StatTypeStr);
	        }
	        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
			return rc;
		}
    }

    /* invoke engine API to get stats */
    ismEngine_TransactionMonitor_t *results = NULL;
    uint32_t resultCount = 0;
    ism_prop_t *filter = ism_common_newProperties(5);
    ism_field_t f;
    int inHeap = 0;

    /* If no xidStr, use "*" */
    if (!xidStr) {
    	xidStr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_monitoring_misc,1000),"*");
    	inHeap = 1;
    }

    f.len = 0;
	f.type = VT_String;
	f.val.s = xidStr;
	ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_XID, &f);
	TRACE(9, "Get Transaction stats: xid=%s\n", xidStr);

	if (TranStateStr) {
		f.val.s = TranStateStr;
		ism_common_setProperty(filter, ismENGINE_MONITOR_FILTER_TRANSTATE, &f);
		TRACE(9, "Get Transaction stats: TranState=%s\n", TranStateStr);
	}

    rc = ism_engine_getTransactionMonitor( &results, &resultCount, type, maxResults, filter);


    /* free resources */
    if (inHeap) {
    	ism_common_free(ism_memory_monitoring_misc,xidStr);
    }
    ism_common_freeProperties(filter);

    if ( rc != ISMRC_OK ) {
        /* TODO: Log Message */

        /* Set returned error string */
         ism_monitoring_getMsgId(6513, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			repl[0]="transaction";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "Failed to get the %s monitoring data.", "transaction");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    }

    /* Create return result buffer */
    if ( resultCount <= 0 ) {
        /* TODO: Log Message */

        /* Set returned error string */
        rc = ISMRC_NotFound;
        ism_monitoring_getMsgId(6508, msgID);
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
        } else {
        	sprintf(tmpbuf, "No monitoring data is found for the specified command.");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
        return rc;
    }

    /* create serialized buffer from received monitoring data from engine */
    ismJsonSerializerData iSerUserData;
    ismSerializerData iSerData;
    memset(&iSerUserData,0, sizeof(ismJsonSerializerData));
    memset(&iSerData,0, sizeof(ismSerializerData));
    ismEngine_TransactionMonitor_t * transactionMonEngine = results;
    int i;
    int addNext = 0;

    iSerUserData.isExternalMonitoring = 0;
    iSerUserData.outbuf = outputBuffer;
    iSerData.serializer_userdata = &iSerUserData;

    ism_common_allocBufferCopyLen(iSerUserData.outbuf, "[", 1);

    for (i=0; i < resultCount; i++) {
        if ( addNext == 1 ) {
            ism_common_allocBufferCopyLen(iSerUserData.outbuf, ",", 1);
        }

        /* Serialize data */
        ism_common_serializeMonJson(XismEngine_TransactionMonitor_t, (void *)transactionMonEngine, outputBuffer->buf, 2500, &iSerData );

        transactionMonEngine++;
        addNext = 1;
    }

    ism_common_allocBufferCopyLen(iSerUserData.outbuf, "]", 1);

    ism_engine_freeTransactionMonitor(results);

    return rc;
}


/**
 * Engine monitoring data action handler
 *
 * The monitoring actions are invoked by users from WebUI or CLI.
 * The WebUI or CLI, creates an action string in JSON format and sends the
 * request to ISM server process on Admin port, using MQTT WebSockets protocol.
 *
 * Statistics of the following objects are maintained by Engine component:
 * - Subscription
 * - Topic
 * - Queue
 * - MQTTClient
 * Engine provides a set of APIs to expose these monitoring data.
 *
 * The ISM server monitoring component calls these APIs to collect 
 * the required statistics.
 */

XAPI int32_t ism_monitoring_getEngineStats (
        char               * action,
        ism_json_parse_t   * inputJSONObj,
        concat_alloc_t     * outputBuffer )
{
    int32_t rc = ISMRC_OK;
    char rbuf[512];

    int actionType = ismmon_getStatsType(action);
    TRACE(9, "MonitoringAction: %s\n", action);

    switch (actionType) {
        case ISMMON_ENGINE_STATE_ACTION_SUBSCRIPTION:
        {
            rc = ism_monitoring_getSubscriptionStats(inputJSONObj, outputBuffer);
            break;
        }

        case ISMMON_ENGINE_STATE_ACTION_TOPIC:
        {
            rc = ism_monitoring_getTopicStats(inputJSONObj, outputBuffer);
            break;
        }

        case ISMMON_ENGINE_STATE_ACTION_QUEUE:
        {
            rc = ism_monitoring_getQueueStats(inputJSONObj, outputBuffer);
            break;
        }

        case ISMMON_ENGINE_STATE_ACTION_MQTTCLIENT:
        {
            rc = ism_monitoring_getMQTTClientStats(inputJSONObj, outputBuffer);
            break;
        }

        case ISMMON_ENGINE_STATE_ACTION_TRANSACTION:
		{
			rc = ism_monitoring_getTransactionStats(inputJSONObj, outputBuffer);
			break;
		}

        case ISMMON_ENGINE_STATE_ACTION_RESOURCESET:
        {
            rc = ism_monitoring_getResourceSetStats(inputJSONObj, outputBuffer);
            break;
        }
        default:
        {
            /* TODO: log error */
            rc = ISMRC_ArgNotValid;
            sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"Invalid option.\" }", rc);
            ism_common_allocBufferCopyLen(outputBuffer,rbuf,strlen(rbuf));
            break;
        }
    }

    return rc;
}


/**
 * Memory monitoring data action handler
 *
 * The monitoring actions are invoked by users from WebUI or CLI.
 * The WebUI or CLI, creates an action string in JSON format and sends the
 * request to ISM server process on Admin port, using MQTT WebSockets protocol.
 *
 * Statistics of memory are maintained by Engine component.
 * Engine provides a set of APIs to expose these monitoring data.
 *
 * The ISM server monitoring component calls these APIs to collect
 * the required statistics.
 */

XAPI int32_t ism_monitoring_getMemoryStats (
        char               * action,
        ism_json_parse_t   * inputJSONObj,
        concat_alloc_t     * outputBuffer, 
        int isExternalMonitoring)
{
    ismEngine_MemoryStatistics_t memoryStats = {0};
    int rc = ISMRC_OK;
    char rbuf[1024+256];
    int isHistoryDataRequest=0;
    
    char *subtype=NULL;
    if(inputJSONObj!=NULL){
    	subtype = (char *)ism_json_getString(inputJSONObj, "SubType");
    }
    if(subtype==NULL || *subtype=='\0'){
    	subtype="current";
    }
    
    memset(&rbuf,0,sizeof(rbuf));
    
    rc = ism_engine_getMemoryStatistics(&memoryStats);
    
	
    
    if ( rc != ISMRC_OK ) {
       int xlen;
		char  msgID[12];
		char  lbuf[1024];
    	const char * repl[3];
		ism_monitoring_getMsgId(6517, msgID);
		char mtmpbuf[1024];
		if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			ism_common_formatMessage(mtmpbuf, sizeof(mtmpbuf), lbuf, repl, 0);
		} else {
			sprintf(mtmpbuf, "Failed to query the memory statistics.");
		}
        sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"%s\" }", rc, mtmpbuf);
    } else {
    
    	//Update History Data for Memory for every 6 seconds
    	currenttime = (uint64_t) ism_common_readTSC() ;
	    if ( currenttime >= ((DEFAULT_SNAPSHOTS_SHORTRANGE6_INTERVAL) + lastime)) {
			TRACE(8, "Start process the short range snap shot for Memory data.\n");
	       // int irc = ism_monitoring_checkMemorySnapshotAction(currenttime, &memoryStats);
	        int irc = ism_monitoring_updateSnapshot(currenttime, &memoryStats, ismMON_STAT_Memory, monitoringMemorySnapList);
	        if (irc) {
			    TRACE(8, "Failed to  process the short range snap shot for Memory data.\n");
	        }else{
	        	TRACE(8, "End process the short range snap shot for Memory data.\n");
	        }
	          
	        lastime = currenttime;
	    }
    
    	if(!strcmpi(subtype, "current")){
    		//Get Current Memory Data
    		
	    	if(isExternalMonitoring==0){
		        sprintf(rbuf, "{ \"MemoryTotalBytes\":%lu, \"MemoryFreeBytes\":%lu, \"MemoryFreePercent\":%.0f,"
		                       " \"ServerVirtualMemoryBytes\":%lu, \"ServerResidentSetBytes\":%lu,"
		                       " \"MessagePayloads\":%lu, \"PublishSubscribe\":%lu, \"Destinations\":%lu,"
		                       " \"CurrentActivity\":%lu , \"ClientStates\":%lu }",
		                       memoryStats.MemoryTotalBytes, memoryStats.MemoryFreeBytes, memoryStats.MemoryFreePercent,
		                       memoryStats.ServerVirtualMemoryBytes, memoryStats.ServerResidentSetBytes,
		                       memoryStats.GroupMessagePayloads, memoryStats.GroupPublishSubscribe, memoryStats.GroupDestinations,
		                       memoryStats.GroupCurrentActivity,
		                       memoryStats.GroupClientStates );
	       }else{
	       		char msgPrefixBuf[256];
	       		concat_alloc_t prefixbuf = { NULL, 0, 0, 0, 0 };
	       		prefixbuf.buf = (char *)&msgPrefixBuf;
	       		prefixbuf.len = sizeof(msgPrefixBuf);
	       		
	       		uint64_t currentTime = ism_common_currentTimeNanos();
	       		
	       		ism_monitoring_getMsgExternalMonPrefix(ismMonObjectType_Memory,
	       											 currentTime,
													 NULL,
													 &prefixbuf);
				char  * msgPrefix = alloca(prefixbuf.used+1);
				
				memcpy(msgPrefix, prefixbuf.buf, prefixbuf.used);
				msgPrefix[prefixbuf.used]='\0';
	       		
	       		sprintf(rbuf, "{ %s, \"MemoryTotalBytes\":%lu, \"MemoryFreeBytes\":%lu, \"MemoryFreePercent\":%.0f,"
		                       " \"ServerVirtualMemoryBytes\":%lu, \"ServerResidentSetBytes\":%lu,"
		                       " \"MessagePayloads\":%lu, \"PublishSubscribe\":%lu, \"Destinations\":%lu,"
		                       " \"CurrentActivity\":%lu, \"ClientStates\":%lu }",
		                        msgPrefix, memoryStats.MemoryTotalBytes, memoryStats.MemoryFreeBytes, memoryStats.MemoryFreePercent,
		                       memoryStats.ServerVirtualMemoryBytes, memoryStats.ServerResidentSetBytes,
		                       memoryStats.GroupMessagePayloads, memoryStats.GroupPublishSubscribe, memoryStats.GroupDestinations,
		                       memoryStats.GroupCurrentActivity,
		                       memoryStats.GroupClientStates  );
	       		
	       		if(prefixbuf.inheap) ism_common_free(ism_memory_monitoring_misc,prefixbuf.buf);
	       }
       }else{
       		//Get history data of memory
       		isHistoryDataRequest=1;
       		char *dur = (char *)ism_json_getString(inputJSONObj, "Duration");
			if (dur == NULL) {
				dur="1800";
			}
			int duration = atoi(dur);
			if (duration > 0 && duration < 5) {
				duration = 5;
			}
			int interval = 0;
			
			if (duration <= 60*60) {
				//TRACE(9, "Monitoring: get data from short list\n");
				interval = DEFAULT_SNAPSHOTS_SHORTRANGE6_INTERVAL;
			} else {
				//TRACE(9, "Monitoring: get data from long list\n");
				interval = DEFAULT_SNAPSHOTS_LONGRANGE_INTERVAL;
			}
			ism_monitoring_snap_t * list = ism_monitoring_getSnapshotListByInterval(interval, monitoringMemorySnapList);
			
			char * types = (char *)ism_json_getString(inputJSONObj, "StatType");
			
			rc = getMemoryHistoryStats(list, types, duration, interval, outputBuffer);
       		
       }
    }
    
    if(!isHistoryDataRequest)
    	ism_common_allocBufferCopyLen(outputBuffer,rbuf,strlen(rbuf));
    return rc;
}


/**
 * Detailed memory monitoring data action handler
 *
 * The monitoring actions are invoked by users from WebUI or CLI.
 * The WebUI or CLI, creates an action string in JSON format and sends the
 * request to ISM server process on Admin port, using MQTT WebSockets protocol.
 *
 * Statistics of memory are maintained by Engine component.
 * Engine provides a set of APIs to expose these monitoring data.
 *
 * The ISM server monitoring component calls these APIs to collect
 * the required statistics.
 */

XAPI int32_t ism_monitoring_getDetailedMemoryStats (
        char               * action,
        ism_json_parse_t   * inputJSONObj,
        concat_alloc_t     * outputBuffer )
{
    ismEngine_MemoryStatistics_t memoryStats = {0};
    int rc = ISMRC_OK;
    char rbuf[1536];
    rc = ism_engine_getMemoryStatistics(&memoryStats);
    if ( rc != ISMRC_OK ) {
		int xlen;
		char  msgID[12];
		char  lbuf[1024];
    	const char * repl[3];
		ism_monitoring_getMsgId(6517, msgID);
		char mtmpbuf[1024];
		if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			ism_common_formatMessage(mtmpbuf, sizeof(mtmpbuf), lbuf, repl, 0);
		} else {
			sprintf(mtmpbuf, "Failed to query the memory statistics.");
		}
        sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"%s\" }", rc, mtmpbuf);
    } else {
        sprintf(rbuf, "{ \"MemoryTotalBytes\":%lu, \"MemoryFreeBytes\":%lu, \"MemoryFreePercent\":\"%.0f\","
                       " \"ServerVirtualMemoryBytes\":%lu, \"ServerResidentSetBytes\":%lu,"
                       " \"GroupMessagePayloads\":%lu, \"GroupPublishSubscribe\":%lu, \"GroupDestinations\":%lu,"
                       " \"GroupCurrentActivity\":%lu, \"GroupClientStates\":%lu, \"GroupRecovery\":%lu,"
                       " \"MessagePayloads\":%lu, \"TopicAnalysis\":%lu, \"SubscriberTree\":%lu,"
                       " \"NamedSubscriptions\":%lu, \"SubscriberCache\":%lu, \"SubscriberQuery\":%lu,"
                       " \"TopicTree\":%lu, \"TopicQuery\":%lu, \"ClientState\":%lu,"
                       " \"CallbackContext\":%lu, \"PolicyInfo\":%lu, \"QueueNamespace\":%lu,"
                       " \"SimpleQueueDefns\":%lu, \"SimpleQueuePages\":%lu, \"IntermediateQueueDefns\":%lu,"
                       " \"IntermediateQueuePages\":%lu, \"MulticonsumerQueueDefns\":%lu, \"MulticonsumerQueuePages\":%lu,"
                       " \"LockManager\":%lu, \"MQConnectivityRecords\":%lu, \"RecoveryTable\":%lu,"
                       " \"ExternalObjects\":%lu, \"LocalTransactions\":%lu, \"GlobalTransactions\":%lu,"
                       " \"MonitoringData\":%lu, \"NotificationData\":%lu, \"MessageExpiryData\":%lu,"
                       " \"RemoteServerData\":%lu, \"CommitData\":%lu, \"UnneededRetainedMsgs\":%lu,"
                       " \"ExpiringGetData\":%lu, \"ExportResources\":%lu, \"Diagnostics\":%lu,"
                       " \"UnneededBufferedMsgs\":%lu, \"JobQueues\":%lu, \"ResourceSetStats\":%lu,"
                       " \"DeferredFreeLists\":%lu}",
                       memoryStats.MemoryTotalBytes, memoryStats.MemoryFreeBytes, memoryStats.MemoryFreePercent,
                       memoryStats.ServerVirtualMemoryBytes, memoryStats.ServerResidentSetBytes,
                       memoryStats.GroupMessagePayloads, memoryStats.GroupPublishSubscribe, memoryStats.GroupDestinations,
                       memoryStats.GroupCurrentActivity, memoryStats.GroupClientStates, memoryStats.GroupRecovery,
                       memoryStats.MessagePayloads, memoryStats.TopicAnalysis, memoryStats.SubscriberTree,
                       memoryStats.NamedSubscriptions, memoryStats.SubscriberCache, memoryStats.SubscriberQuery,
                       memoryStats.TopicTree, memoryStats.TopicQuery, memoryStats.ClientState,
                       memoryStats.CallbackContext, memoryStats.PolicyInfo, memoryStats.QueueNamespace,
                       memoryStats.SimpleQueueDefns, memoryStats.SimpleQueuePages, memoryStats.IntermediateQueueDefns,
                       memoryStats.IntermediateQueuePages, memoryStats.MulticonsumerQueueDefns, memoryStats.MulticonsumerQueuePages,
                       memoryStats.LockManager, memoryStats.MQConnectivityRecords, memoryStats.RecoveryTable,
                       memoryStats.ExternalObjects, memoryStats.LocalTransactions, memoryStats.GlobalTransactions,
                       memoryStats.MonitoringData, memoryStats.NotificationData, memoryStats.MessageExpiryData,
                       memoryStats.RemoteServerData, memoryStats.CommitData, memoryStats.UnneededRetainedMsgs,
                       memoryStats.ExpiringGetData, memoryStats.ExportResources, memoryStats.Diagnostics,
                       memoryStats.UnneededBufferedMsgs, memoryStats.JobQueues, memoryStats.ResourceSetStats,
                       memoryStats.DeferredFreeLists
                       );
    }
    ism_common_allocBufferCopyLen(outputBuffer,rbuf,strlen(rbuf));
    return rc;
}
/*******************************************************SNAPSHOT OF MEMORY***********************/


/*Initialize the engine monitoring data structures*/
int ism_monitoring_initEngineMonData(void)  {
	int rc = ism_monitoring_initMonitoringSnapList(&monitoringMemorySnapList, DEFAULT_SNAPSHOTS_SHORTRANGE6_INTERVAL, DEFAULT_SNAPSHOTS_LONGRANGE_INTERVAL);
	if (!rc) {
	    /* If a resource set descriptor is defined, go through the data and set anything of
               type ISMOBJD_RSETID to be displayable */
	    if (ism_admin_isResourceSetDescriptorDefined(0)) {
	        ism_mon_obj_def_t **thisobj = allobjs;
	        while(*thisobj) {
	            ism_mon_obj_def_t *thisdef = *thisobj;
	            while(thisdef->wtype) {
	                if (thisdef->wtype == ISMOBJD_RSETID) {
	                    thisdef->displayable = 1;
	                }
	                thisdef++;
	            }
	            thisobj++;
	        }
	    }
	}
	return rc;
}

/*Create new Memory Historical object*/
void * ism_monitoring_newMemoryHistDataObject(void)
{
	ism_mondata_memory_t *ret = ism_common_calloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,38),1, sizeof(ism_mondata_memory_t));
	memset(ret, -1, sizeof(ism_mondata_memory_t));
	return (void *) ret;
}


/*
 * The new node has already been allocated or resigned as the first in the list in initNewData
 */
static int storeMemoryMonData(ism_snapshot_range_t ** list, ismEngine_MemoryStatistics_t * memoryStats) {
	ism_snapshot_range_t *sp;

	if(list!=NULL && *list==NULL){
		ism_monitoring_newSnapshotRange(list, ismMON_STAT_Memory, free);
	}

    /*For Memory snapshot, there is only 1  for snapshot range list*/
    sp = *list;					/* BEAM suppression: dereferencing NULL */
    
	pthread_spin_lock(&sp->snplock);
	
	ism_snapshot_data_node_t * node = (ism_snapshot_data_node_t *)sp->data_nodes;
	
	/*Fill in the data for the node*/
  	((ism_mondata_memory_t *)node->data)->MemoryTotalBytes = memoryStats->MemoryTotalBytes;  
  	((ism_mondata_memory_t *)node->data)->MemoryFreeBytes =  memoryStats->MemoryFreeBytes;    
  	((ism_mondata_memory_t *)node->data)->MemoryFreePercent= memoryStats->MemoryFreePercent;  
  	((ism_mondata_memory_t *)node->data)->ServerVirtualMemoryBytes= memoryStats->ServerVirtualMemoryBytes;  
  	((ism_mondata_memory_t *)node->data)->ServerResidentSetBytes= memoryStats->ServerResidentSetBytes;    
  	((ism_mondata_memory_t *)node->data)->GroupMessagePayloads= memoryStats->GroupMessagePayloads;      
  	((ism_mondata_memory_t *)node->data)->GroupPublishSubscribe= memoryStats->GroupPublishSubscribe;    
    ((ism_mondata_memory_t *)node->data)->GroupDestinations= memoryStats->GroupDestinations;         
    ((ism_mondata_memory_t *)node->data)->GroupCurrentActivity= memoryStats->GroupCurrentActivity;   
    ((ism_mondata_memory_t *)node->data)->GroupClientStates= memoryStats->GroupClientStates;
	
	if (sp->node_idle > 0)
             sp->node_idle--;
	pthread_spin_unlock(&sp->snplock);
	
	return ISMRC_NotFound;
}


/**
 * Record Memory Snapshot
 */
XAPI int ism_monitoring_recordMemorySnapShot(ism_monitoring_snap_t * slist, ism_monitoring_snap_t * llist, void *stat) {
	
	int rc = ISMRC_OK;
	ismEngine_MemoryStatistics_t memoryStats = {0};
	ismEngine_MemoryStatistics_t *memStat=NULL;
	
	
	if(stat!=NULL){
		memStat = (ismEngine_MemoryStatistics_t *)stat;
	}else{
		rc = ism_engine_getMemoryStatistics(&memoryStats);
		if(rc!=ISMRC_OK){
			return rc;
		}
		memStat =  (ismEngine_MemoryStatistics_t *)&memoryStats;
	}
	
	storeMemoryMonData(&(slist->range_list), memStat);
	
	if(llist!=NULL){
		storeMemoryMonData(&(llist->range_list), memStat);
	}
  
	return rc;
}

/*Memory Data Type for Historical data*/
typedef enum {
    ismMonMem_NONE          	    = 0,
    ismMonMem_TOTAL_BYTES      		= 1,   	//MemoryTotalBytes
    ismMonMem_FREE_BYTES      		= 2,	//MemoryFreeBytes
    ismMonMem_FREE_PERCENT     		= 3,	//MemoryFreePercent
    ismMonMem_SVR_VRTL_MEM_BYTES	= 4,	//ServerVirtualMemoryBytes
    ismMonMem_SVR_RSDT_SET_BYTES	= 5,	//ServerResidentSetBytes
    ismMonMem_GRP_MSG_PAYLOADS		= 6,	//GroupMessagePayloads
    ismMonMem_GRP_PUB_SUB			= 7,	//GroupPublishSubscribe
    ismMonMem_GRP_DESTS				= 8,	//GroupDestinations
    ismMonMem_GRP_CUR_ACTIVITIES	= 9,	//GroupCurrentActivity
    ismMonMem_GRP_CLIENT_STATES		= 10,	//GroupClientStates
    
} ismMonitoringMemDataType_t;

/*Get Memory Data Type*/
static int getMemDataType(char *type) {

	
	if (!strcasecmp(type, "MemoryTotalBytes"))
		return ismMonMem_TOTAL_BYTES;
	if (!strcasecmp(type, "MemoryFreeBytes"))
		return ismMonMem_FREE_BYTES;
	if (!strcasecmp(type, "MemoryFreePercent"))
		return ismMonMem_FREE_PERCENT;
	if (!strcasecmp(type, "ServerVirtualMemoryBytes"))
		return ismMonMem_SVR_VRTL_MEM_BYTES;
	if (!strcasecmp(type, "ServerResidentSetBytes"))
		return ismMonMem_SVR_RSDT_SET_BYTES;
	if (!strcasecmp(type, "MessagePayloads"))
		return ismMonMem_GRP_MSG_PAYLOADS;
	if (!strcasecmp(type, "PublishSubscribe"))
	    return ismMonMem_GRP_PUB_SUB;
	if (!strcasecmp(type, "Destinations"))
		return ismMonMem_GRP_DESTS;
	if (!strcasecmp(type, "CurrentActivity"))
		return ismMonMem_GRP_CUR_ACTIVITIES;
	if (!strcasecmp(type, "ClientStates"))
		return ismMonMem_GRP_CLIENT_STATES;
	
	return ismMon_NONE;
}

/*Get Memory Historical data and set the output into the output_buffer for return*/
static int32_t getMemoryHistoryStats(ism_monitoring_snap_t *list, char * types, int duration, ism_snaptime_t interval, concat_alloc_t * output_buffer) 
{
	ism_snapshot_range_t *sp;
	int query_count = 0;
	char buf[4096];
	int rc = ISMRC_OK;
	int i = 0;
	ismMonitoringHistoryData_t histResData[10];
	int count=0;
	
    /*
	 * calculate how many noded need to be returned.
	 */
	
	int master_count = duration/interval + 1;
	query_count = master_count;
	
	int typesLen = strlen(types);
	char * typesTmp = alloca(typesLen+1);
	memcpy(typesTmp, types, typesLen);
	typesTmp[typesLen]='\0';
	
	int typeCount=0;
	char * typeToken = ism_common_getToken(typesTmp, " \t,", " \t,", &typesTmp);
	while (typeToken) {
			histResData[typeCount].typeStr=typeToken;
			histResData[typeCount].type=getMemDataType(typeToken);
			histResData[typeCount].darray = alloca(query_count*sizeof(int64_t));
			for (i = 0; i< master_count; i++)
				histResData[typeCount].darray[i] = -1;
		 	typeCount++;
		 	typeToken = ism_common_getToken(typesTmp, " \t,", " \t,", &typesTmp);
	}


	for(count=0; count < typeCount; count++){
		ismMonitoringHistoryData_t memHistory = histResData[count];
		
		if(memHistory.type!=ismMon_NONE){
			sp = list->range_list;
			while(sp) {
				if (query_count > sp->node_count) {
			    	query_count = sp->node_count;
			    }
				pthread_spin_lock(&sp->snplock);
				ism_snapshot_data_node_t * dataNode = sp->data_nodes;
			    ism_mondata_memory_t * mdata = NULL;
			    if (!dataNode) {
			        pthread_spin_unlock(&sp->snplock);
		
			    	rc = ISMRC_NullPointer;
			    	TRACE(1, "Monitoring: getAllfromList cannot find the monitoring data.\n");
			    	
			    	int xlen;
					char  msgID[12];
					ism_monitoring_getMsgId(6518, msgID);
					char  lbuf[1024];
    				const char * repl[3];
					char mtmpbuf[1024];
					if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
						repl[0]="memory";
						ism_common_formatMessage(mtmpbuf, sizeof(mtmpbuf), lbuf, repl, 1);
					} else {
						sprintf(mtmpbuf, "The memory historical statistics is not available.");
					}
			    	
			    	snprintf(buf, sizeof(buf), "{ \"RC\":\"%d\", \"ErrorString\":\"%s\" }", rc, mtmpbuf);
			    	ism_common_allocBufferCopyLen(output_buffer,buf,strlen(buf));
			    	output_buffer->used = strlen(output_buffer->buf);
			    	return rc;
			    }
			    
			    switch(memHistory.type) {
		        case ismMonMem_TOTAL_BYTES:
		          	for (i= 0; i < query_count; i++) {
		          		mdata = (ism_mondata_memory_t *)dataNode->data;
		          		if(mdata->MemoryTotalBytes>-1)
		            		memHistory.darray[i] = mdata->MemoryTotalBytes;
		            	dataNode = dataNode->next;
		            }
		            break;
		        case ismMonMem_FREE_BYTES:
		        	for (i= 0; i < query_count; i++) {
		        		mdata = (ism_mondata_memory_t*)dataNode->data;
		        		if(mdata->MemoryFreeBytes>-1)
		        	    	memHistory.darray[i] = mdata->MemoryFreeBytes;
		        	    dataNode = dataNode->next;
		        	}
		        	break;
			    case ismMonMem_FREE_PERCENT:
		            for (i= 0; i < query_count; i++) {
		            	mdata = (ism_mondata_memory_t *)dataNode->data;
		            	if(mdata->MemoryFreePercent>-1)
		            		memHistory.darray[i] = mdata->MemoryFreePercent;
		            	dataNode = dataNode->next;
		            }
		            break;
		        case ismMonMem_SVR_VRTL_MEM_BYTES:
		        	for (i= 0; i < query_count; i++) {
		        		mdata = (ism_mondata_memory_t *)dataNode->data;
		        		if(mdata->ServerVirtualMemoryBytes>-1)
		            		memHistory.darray[i] = mdata->ServerVirtualMemoryBytes;
		            	dataNode = dataNode->next;
		            }
		            break;
		        case ismMonMem_SVR_RSDT_SET_BYTES:
		            for (i= 0; i < query_count; i++) {
		            	mdata = (ism_mondata_memory_t *)dataNode->data;
		            	if(mdata->ServerResidentSetBytes>-1)
		            		memHistory.darray[i] = mdata->ServerResidentSetBytes;
		            	dataNode = dataNode->next;
		            }
		            break;
		        case ismMonMem_GRP_MSG_PAYLOADS:
		        	for (i= 0; i < query_count; i++) {
		        		mdata = (ism_mondata_memory_t *)dataNode->data;
		        		if(mdata->GroupMessagePayloads>-1)
		            		memHistory.darray[i] = mdata->GroupMessagePayloads;
		            	dataNode = dataNode->next;
		            }
		            break;
			    case ismMonMem_GRP_PUB_SUB:
		            for (i= 0; i < query_count; i++) {
		            	mdata = (ism_mondata_memory_t *)dataNode->data;
		            	if(mdata->GroupPublishSubscribe>-1)
		            		memHistory.darray[i] = mdata->GroupPublishSubscribe;
		            	dataNode = dataNode->next;
		            }
		            break;
		        case ismMonMem_GRP_DESTS:
		            for (i= 0; i < query_count; i++) {
		            	mdata = (ism_mondata_memory_t *)dataNode->data;
		            	if(mdata->GroupDestinations>-1)
		            		memHistory.darray[i] =mdata->GroupDestinations;
		            	dataNode = dataNode->next;
		            }
		            break;
		         
		        case ismMonMem_GRP_CUR_ACTIVITIES:
		            for (i= 0; i < query_count; i++) {
		            	mdata = (ism_mondata_memory_t *)dataNode->data;
		            	if(mdata->GroupCurrentActivity>-1)
		            		memHistory.darray[i] =mdata->GroupCurrentActivity;
		            	dataNode = dataNode->next;
		            }
		            break;
		         case ismMonMem_GRP_CLIENT_STATES:
		            for (i= 0; i < query_count; i++) {
		            	mdata = (ism_mondata_memory_t *)dataNode->data;
		            	if(mdata->GroupClientStates>-1)
		            		memHistory.darray[i] =mdata->GroupClientStates;
		            	dataNode = dataNode->next;
		            }
		            break;
		       	default:
		       		TRACE(1, "Monitoring: getDatafromList does not support monitoring type %s\n", memHistory.typeStr);
		       		break;
		        }
		
			    pthread_spin_unlock(&sp->snplock);
				sp = sp->next;
				query_count = master_count;
			}
		}
	}
	
	
	/*Construction the Header for the Response*/
	
	memset(buf, 0, sizeof(buf));
    sprintf(buf, "{ \"Duration\":%d, \"Interval\":%llu", duration, (ULL)interval);
    ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
    
    
    /*
     * print out the darray
     */
    for(count=0; count < typeCount; count++){
		ismMonitoringHistoryData_t memHistory = histResData[count];
		
		memset(buf, 0, sizeof(buf));
		sprintf(buf, ", \"%s\":\"",memHistory.typeStr);
        ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
		
		memset(buf, 0, sizeof(buf));
	    char *p = buf;
	    for (i= 0; i < master_count; i++) {
	    	if (i > 0)
	    	    p += sprintf(p, ",");
	    	p += sprintf(p, "%ld", memHistory.darray[i]);
	    	if( p - buf > (sizeof(buf)-512)) {
	    	    ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
	    	    memset(buf, 0, sizeof(buf));
	    	    p = buf;
	    	}
	    }
	    
	    if (p != buf)
	        ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
	
	    /*
	     * Add the end of the json string
	     */
	    sprintf(buf, "\"");
	    ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
		
	}
   
    sprintf(buf, "}");
	ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
  

	return ISMRC_OK;
}
