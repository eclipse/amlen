/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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
/// @file resourceSetStats.h
/// @brief Internal-only data structures for resource set stats sub component
//****************************************************************************
#ifndef __ISM_ENGINE_RESOURCESET_STATS_INTERNAL_DEFINED
#define __ISM_ENGINE_RESOURCESET_STATS_INTERNAL_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "engineHashTable.h"
#include "resourceSetStats.h"

typedef struct tag_iereReportingControl_t {
    ism_threadh_t           threadHandle;               ///< Thread handle of the reporting thread (if any)
    volatile bool           endRequested;               ///< Whether the reporting thread should end.
    pthread_cond_t          cond_wakeup;                ///< Used to wake the reporting thread
    pthread_mutex_t         mutex_wakeup;               ///< Used to protect cond_wakeup
    int32_t                 minutesPast;                ///< Minutes past the hour to start reporting cycle (-1 for automatic)
    uint32_t                reportingCyclesCompleted;   ///< How many reporting cycles (hours) has the reporting thread done
    // Requested report values
    ismEngineMonitorType_t  requestedReportMonitorType; ///< Type of report to produce when requested
    uint32_t                requestedReportMaxResults;  ///< Maximum results to report (ignored if type is ismENGINE_MONITOR_ALL_UNSORTED)
    bool                    requestedReportResetStats;  ///< Whether this requested report should reset stats
    // Weekly report values
    int32_t                 weeklyReportDay;            ///< Day of week to perform weekly report (0-6)
    int32_t                 weeklyReportHour;           ///< Hour of day to perform weekly report (0-23)
    ismEngineMonitorType_t  weeklyReportMonitorType;    ///< Type of report to produce weekly
    uint32_t                weeklyReportMaxResults;     ///< Maximum results to report (ignored if type is ismENGINE_MONITOR_ALL_UNSORTED)
    bool                    weeklyReportResetStats;     ///< Whether the weekly report should reset stats
    // Daily report values
    int32_t                 dailyReportHour;            ///< Hour of day to perform daily report (0-23)
    ismEngineMonitorType_t  dailyReportMonitorType;     ///< Type of report to produce daily
    uint32_t                dailyReportMaxResults;      ///< Maximum results to report (ignored if type is ismENGINE_MONITOR_ALL_UNSORTED)
    bool                    dailyReportResetStats;      ///< Whether the daily report should reset stats
    // Hourly report values
    ismEngineMonitorType_t  hourlyReportMonitorType;    ///< Type of report to produce daily
    uint32_t                hourlyReportMaxResults;     ///< Maximum results to report (ignored if type is ismENGINE_MONITOR_ALL_UNSORTED)
    bool                    hourlyReportResetStats;     ///< Whether the hourly report should reset stats
} iereReportingControl_t;

// Minutes past the hour is unspecified, uses a number between 15 & 45 based on serverUID.
#define iereMINUTES_PAST_UNSPECIFIED -1

typedef struct tag_iereResourceSetStatsControl_t {
    char                    strucId[4];          ///< Structure identifier 'RECS'
    ieutHashTable_t        *setStats;            ///< Stats for each of the resource sets
    pthread_rwlock_t        setStatsLock;        ///< controlsaccess to setstats
    ism_time_t              restartTime;         ///< Time at which the resourceSet stats were started (~server restart time)
    ism_time_t              resetTime;           ///< Time at which the resourceSet stats were last reset
    ism_regex_t             topicRegEx;          ///< Gets resource set from 1st sub expr in topic
    ism_regex_t             clientIdRegEx;       ///< Gets resource set from 1st sub expr in client
    iereResourceSet_t      *defaultResourceSet;  ///< Default resource set if none other found
    iereReportingControl_t  reporting;           ///< Control information for reporting thread
    #ifdef MEMDEBUG
    char                   *traceMemorySetId;    ///< Wildcard SetId to trace memory usage of (MEMDEBUG only).
    #endif
} iereResourceSetStatsControl_t;

#define IERE_CONTROL_STRUCID "RECS"

//During a single engine operation we will cache stats for up to this many resource sets in a local cache
//...and update the global stats when the operation finishes (when we flush the cache) or the
//operation touches more resource sets than we have space in the cache for
//NB: The cache entries are stored in a arbitrary order (with an exception for sticky sets see below),
//and we linearly scan looking for a match If this number needs to be increased by much, that needs to be rethought
//(e.g. hash table per thread?)
#define IERE_NUM_THREADCACHE_SETS 3

//If doing an engine operation we touch more resource sets than we have space in the cache for, we don't
//evict the first sets (the "sticky" ones as defined by this constant) as they are likely to be used again
//(instead the first "non-sticky" resource set is evicted
#define IERE_NUM_THREADCACHE_STICKY 2

//Limit the amount of space taken by a setId
#define IERE_MAX_SETID_LENGTH 128

// Amount of time we're willing to wait for timers at shutdown.
#define IERE_MAXIMUM_SHUTDOWN_TIMEOUT_SECONDS 60

typedef struct tag_iereThreadCache_t {
    char                   strucId[4];      ///< Structure identifier 'RETC'
    iereThreadCacheEntry_t stats[IERE_NUM_THREADCACHE_SETS];
} iereThreadCache_t;

#define IERE_THREADCACHE_STRUCID "RETC"

const char *iere_mapMonitorTypeToStatType(ieutThreadData_t *pThreadData,
                                          ismEngineMonitorType_t monitorType);

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_RESOURCESET_STATS_INTERNAL_DEFINED */

/*********************************************************************/
/* End of resourceSetStat.h                                          */
/*********************************************************************/
