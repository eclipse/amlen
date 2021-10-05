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
/// @brief Tracking usage for subsets of resources
//****************************************************************************
#ifndef __ISM_ENGINE_RESOURCESET_STATS_DEFINED
#define __ISM_ENGINE_RESOURCESET_STATS_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct iereResourceSetStats_t *iereResourceSetStatsHandle_t;

typedef enum tag_iereInt64_StatType_t
{
    // Start of cumulative stats (iereFIRST_CUMULATIVE_STAT should be set to following value)
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_MEMORY,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_SUBSCRIPTIONS,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_NONPERSISTENT_SUBSCRIPTIONS,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_SHARED_SUBSCRIPTIONS,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_NONPERSISTENT_SHARED_SUBSCRIPTIONS,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_BUFFEREDMSG_BYTES,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_NONPERSISTENT_BUFFEREDMSG_BYTES,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSGS,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSG_BYTES,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_WILLMSGS,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_WILLMSG_BYTES,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_NONPERSISTENT_WILLMSG_BYTES,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSGS_PUBLISHED,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSGS_PUBLISHED,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSGS_PUBLISHED,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSG_BYTES_PUBLISHED,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSG_BYTES_PUBLISHED,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSG_BYTES_PUBLISHED,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_DISCARDEDMSGS,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_REJECTEDMSGS,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_CONNECTIONS,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_ACTIVE_NONPERSISTENT_CLIENTS,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_ACTIVE_PERSISTENT_CLIENTS,
    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_CLIENT_STATES,
    // End of cumulative stats (iereLAST_CUMULATIVE_STAT should be set to preceding value)
    // Start of maximum stats (iereFIRST_MAXIMUM_STAT should be set to following value)
    ISM_ENGINE_RESOURCESETSTATS_I64_MAX_PUBLISH_RECIPIENTS,
    // End of maximum stats (iereLAST_MAXIMUM_STAT should be set to preceding value)
    // Start of stats used to remember values when a reset is issued
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSGS_PUBLISHED_TO_LASTRESET,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSGS_PUBLISHED_TO_LASTRESET,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSGS_PUBLISHED_TO_LASTRESET,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSG_BYTES_PUBLISHED_TO_LASTRESET,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSG_BYTES_PUBLISHED_TO_LASTRESET,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSG_BYTES_PUBLISHED_TO_LASTRESET,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_DISCARDEDMSGS_TO_LASTRESET,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_REJECTEDMSGS_TO_LASTRESET,
    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_CONNECTIONS_TO_LASTRESET,
    ISM_ENGINE_RESOURCESETSTATS_I64_MAX_PUBLISH_RECIPIENTS_TO_LASTRESET,
    // End of stats used to remember values when a reset is issued
    ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS //<< Add stats above here
} iereResourceSet_I64_StatType_t;

// Which of the stats are expected to be cumulative
#define iereFIRST_CUMULATIVE_STAT ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_MEMORY
#define iereLAST_CUMULATIVE_STAT ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_CLIENT_STATES
// Which of the stats are expected to only update if they are larger than current value.
#define iereFIRST_MAXIMUM_STAT ISM_ENGINE_RESOURCESETSTATS_I64_MAX_PUBLISH_RECIPIENTS
#define iereLAST_MAXIMUM_STAT ISM_ENGINE_RESOURCESETSTATS_I64_MAX_PUBLISH_RECIPIENTS

/// @brief Set of stats for a specific resourceSet identifier
typedef struct tag_iereResourceSetStats_t
{
    char *resourceSetIdentifier;
    int64_t int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS];
} iereResourceSetStats_t;

/// @brief An individual resourceSet
typedef struct tag_iereResourceSet_t
{
    pthread_spinlock_t updateLock; ///< Lock protecting this resourceSet when being updated
    iereResourceSetStats_t stats;  ///< Stats (and identifier) for this resourceSet
#ifdef MEMDEBUG
    bool traceMemory;
#endif
} iereResourceSet_t;

/// @brief Per-thread cache entry for a resourceSet
typedef struct tag_iereThreadCacheEntry_t
{
    iereResourceSet_t *resourceSet;     ///< Pointer to underlying resourceSet
    iereResourceSetStats_t  localStats; ///< Locally updated statistics for this resourceSet
    bool localInUse;                    ///< Whether this entry is in use or can be assigned a new resourceSet
} iereThreadCacheEntry_t;

#define iereDEFAULT_RESOURCESET_ID "__DefaultResourceSet" ///< ResourceSet identifier used when one cannot be determined from ResourceSetDescriptor
#define iereOTHER_RESOURCESETS_ID  "__OtherResourceSets"  ///< ResourceSet identifier used when reporting a collection of other resourceSets ('everything else')

/// @brief Get the default resource set (might be iereNO_RESOURCE_SET)
iereResourceSet_t *iere_getDefaultResourceSet(void);

#define iereOP_ADD    0  ///< Add a resourceSet
#define iereOP_FIND   1  ///< Find a resourceSet

/// @brief From a clientid & topic string, get the resource set they indicate
iereResourceSet_t *iere_getResourceSet(ieutThreadData_t *pThreadData,
                                       const char *clientId,
                                       const char *topicString,
                                       int32_t operation);

/// @brief Trace stats for this resource set at level traceLevel
void iere_traceResourceSet(ieutThreadData_t *pThreadData,
                           int32_t traceLevel,
                           iereResourceSet_t *resourceSet);

/// @brief From a resourceSet get a spot in the threadCache in which to store stats.
iereThreadCacheEntry_t *iere_getThreadCacheEntryForResourceSet(ieutThreadData_t *pThreadData,
                                                               iereResourceSet_t *resourceSet);

/// @brief Update the overall resourceSet stats with the values built up in the threadCache entries
void iere_flushResourceSetThreadCache(ieutThreadData_t *pThreadData);

/// @brief Callback function used when enumerating resourceSets
typedef void (*iereEnumerateCallback_t)(ieutThreadData_t *pThreadData,
                                        iereResourceSet_t *pResourceSet,
                                        ism_time_t resetTime,
                                        void *pContext);

/// @brief Enumerate all of the resourceSets and call a callback function
void iere_enumerateResourceSets(ieutThreadData_t *pThreadData,
                                iereEnumerateCallback_t callback,
                                void *context);

/// @brief Use the enumerate callback interface, but for a specific resourceSet
void iere_enumerateSingleResourceSet(ieutThreadData_t *pThreadData,
                                     const char *resourceSetId,
                                     iereEnumerateCallback_t callback,
                                     void *context);

//****************************************************************************
/// @brief Initialize the resourceSetStats subcomponent
///
/// @return OK on success or an ISMRC_ return code.
//****************************************************************************
int32_t iere_initResourceSetStats(ieutThreadData_t *pThreadData);
void    iere_destroyResourceSetStats(ieutThreadData_t *pThreadData);
int32_t iere_startResourceSetReporting(ieutThreadData_t *pThreadData);
int32_t iere_requestResourceSetReport(ieutThreadData_t *pThreadData,
                                      ismEngineMonitorType_t monitorType,
                                      uint32_t maxResults,
                                      bool resetStats);
void    iere_stopResourceSetReporting(ieutThreadData_t *pThreadData);
void    iere_resetResourceSetStats(ieutThreadData_t *pThreadData);
bool    iere_isResourceSetTrackingEnabled(void);
ismEngineMonitorType_t iere_mapStatTypeToMonitorType(ieutThreadData_t *pThreadData,
                                                     const char *statType,
                                                     bool allowFakeReports);

void    iere_initResourceSetThreadCache(ieutThreadData_t *pThreadData);
void    iere_destroyResourceSetThreadCache(ieutThreadData_t *pThreadData);

void *iere_malloc(ieutThreadData_t *pThreadData,
                  iereResourceSetHandle_t resourceSet,
                  uint32_t probe, size_t size);

void *iere_calloc(ieutThreadData_t *pThreadData,
                  iereResourceSetHandle_t resourceSet,
                  uint32_t probe, size_t nmemb, size_t size);

void *iere_realloc(ieutThreadData_t *pThreadData,
                   iereResourceSetHandle_t resourceSet,
                   uint32_t probe, void *ptr, size_t size);

void iere_free(ieutThreadData_t *pThreadData,
               iereResourceSetHandle_t resourceSet,
               iemem_memoryType type,
               void *mem);

void iere_freeStruct(ieutThreadData_t *pThreadData,
                     iereResourceSetHandle_t resourceSet,
                     iemem_memoryType type,
                     void *pStruct,
                     char *pStructId);

size_t iere_usable_size(iemem_memoryType type, void *mem);
size_t iere_full_size(iemem_memoryType type, void *mem);

#ifdef MEMDEBUG
void iere_updateMem(ieutThreadData_t *pThreadData,
                    iereResourceSetHandle_t resourceSet,
                    uint32_t probe, void *mem,
                    size_t size);
#else
#define iere_updateMem(__pThreadData, __resourceSet, __probe, __mem, __size) \
iere_updateTotalMemStat(__pThreadData, __resourceSet, __probe, __mem, __size);
#endif

//****************************************************************************
/// @brief Get the previously prepared thread cache entry resourceSet
//****************************************************************************
static inline iereResourceSetHandle_t iere_getPrimedResourceSet(ieutThreadData_t *pThreadData)
{
    iereResourceSetHandle_t resourceSet = iereNO_RESOURCE_SET;

    const iereThreadCacheEntry_t *threadCacheEntryHint = pThreadData->curThreadCacheEntry;

    if (threadCacheEntryHint != NULL && threadCacheEntryHint->localInUse == true)
    {
        resourceSet = threadCacheEntryHint->resourceSet;
    }

    return resourceSet;
}

//****************************************************************************
/// @brief Prepare the thread cache entry in the pThreadData for specified
/// resourceSet stats.
//****************************************************************************
static inline void iere_primeThreadCache(ieutThreadData_t *pThreadData,
                                         iereResourceSetHandle_t resourceSet)
{
    if (resourceSet == iereNO_RESOURCE_SET)
    {
        pThreadData->curThreadCacheEntry = NULL;
    }
    else
    {
        const iereThreadCacheEntry_t *threadCacheEntryHint = pThreadData->curThreadCacheEntry;

        if (threadCacheEntryHint == NULL ||
            threadCacheEntryHint->resourceSet != resourceSet ||
            threadCacheEntryHint->localInUse == false)
        {
            pThreadData->curThreadCacheEntry = iere_getThreadCacheEntryForResourceSet(pThreadData, resourceSet);
        }
    }
}

//****************************************************************************
/// @brief Update a particular stat in a thread cache entry
//****************************************************************************
static inline void iere_updateInt64Stat(ieutThreadData_t *pThreadData,
                                        iereResourceSetHandle_t resourceSet,
                                        iereResourceSet_I64_StatType_t statType,
                                        int64_t delta)
{
    assert(statType >= iereFIRST_CUMULATIVE_STAT && statType <= iereLAST_CUMULATIVE_STAT);

    if (resourceSet != iereNO_RESOURCE_SET)
    {
        iereThreadCacheEntry_t *threadCacheEntry = pThreadData->curThreadCacheEntry;
        assert(threadCacheEntry->resourceSet == resourceSet);
        threadCacheEntry->localStats.int64Stats[statType] += delta;
    }
}

//****************************************************************************
/// @brief Update the total memory statistic for a resource set
//****************************************************************************
static inline void iere_updateTotalMemStat(ieutThreadData_t *pThreadData,
                                           iereResourceSetHandle_t resourceSet,
                                           uint32_t probe, void *mem,
                                           int64_t delta)
{
    if (resourceSet != iereNO_RESOURCE_SET)
    {
        iereThreadCacheEntry_t *threadCacheEntry = pThreadData->curThreadCacheEntry;
        assert(threadCacheEntry->resourceSet == resourceSet);
        threadCacheEntry->localStats.int64Stats[ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_MEMORY] += delta;
        pThreadData->stats.resourceSetMemBytes += delta;
#ifdef MEMDEBUG
        if (resourceSet->traceMemory)
        {
            ieutTRACEL(pThreadData, delta, 0, "ResourceSetID '%s' %s size(%ld) IEMEM_PROBE(%s, %d) %p\n",
                       resourceSet->stats.resourceSetIdentifier, delta > 0 ? "ALLOC" : "FREE", delta,
                       iemem_getTypeName(IEMEM_GET_MEMORY_TYPE(probe)), IEMEM_GET_MEMORY_PROBEID(probe), mem);
        }
#endif
    }
}

//****************************************************************************
/// @brief Update a particular stat in a thread cache entry
//****************************************************************************
static inline void iere_updateInt64MaxStat(ieutThreadData_t *pThreadData,
                                           iereResourceSetHandle_t resourceSet,
                                           iereResourceSet_I64_StatType_t statType,
                                           int64_t value)
{
    assert(statType >= iereFIRST_MAXIMUM_STAT && statType <= iereLAST_MAXIMUM_STAT);

    if (resourceSet != iereNO_RESOURCE_SET)
    {
        iereThreadCacheEntry_t *threadCacheEntry = pThreadData->curThreadCacheEntry;
        assert(threadCacheEntry->resourceSet == resourceSet);
        if (value > threadCacheEntry->localStats.int64Stats[statType])
        {
            threadCacheEntry->localStats.int64Stats[statType] = value;
        }
    }
}

//****************************************************************************
/// @brief Update a message to be owned by a resourceSet
//****************************************************************************
static inline void iere_updateMessageResourceSet(ieutThreadData_t *pThreadData,
                                                 iereResourceSetHandle_t resourceSet,
                                                 ismEngine_Message_t *pMessage,
                                                 bool requireAtomic,
                                                 bool retainedMsgUpdate)
{
    if (resourceSet != iereNO_RESOURCE_SET)
    {
        iereResourceSetHandle_t fromResourceSet = pMessage->resourceSet;

        // Only want to try switching if the message has no resource set, we are updating
        // it to be retained, or it is currently set to the default and we're trying to
        // set it to something other than the default.
        if (fromResourceSet != iereNO_RESOURCE_SET && retainedMsgUpdate == false)
        {
            iereResourceSetHandle_t defaultResourceSet = iere_getDefaultResourceSet();

            if (fromResourceSet != defaultResourceSet || resourceSet == defaultResourceSet)
            {
                return;
            }

            assert(defaultResourceSet != iereNO_RESOURCE_SET);
            assert(fromResourceSet == defaultResourceSet);
        }

        bool updatedResourceSet;

        if (requireAtomic == true)
        {
            assert(retainedMsgUpdate == false);
            updatedResourceSet = __sync_bool_compare_and_swap(&pMessage->resourceSet, fromResourceSet, resourceSet);
        }
        else
        {
            assert(pMessage->resourceSet == fromResourceSet); // Should be using atomic if there is any doubt...
            pMessage->resourceSet = resourceSet;
            updatedResourceSet = true;
        }

        if (updatedResourceSet == true)
        {
            int64_t fullMemSize = pMessage->fullMemSize;
            iereResourceSet_I64_StatType_t buffMsgStatType = (pMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT) ?
                    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_BUFFEREDMSG_BYTES :
                    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_NONPERSISTENT_BUFFEREDMSG_BYTES;

            // If the message had a resourceSet, we owe that resourceSet some updates.
            if (fromResourceSet != iereNO_RESOURCE_SET)
            {
                iere_primeThreadCache(pThreadData, fromResourceSet);
                iere_updateMem(pThreadData, fromResourceSet, IEMEM_PROBE(iemem_messageBody, 7), pMessage, -fullMemSize);
                iere_updateInt64Stat(pThreadData, fromResourceSet, buffMsgStatType, -fullMemSize);
            }

            iere_primeThreadCache(pThreadData, resourceSet);
            iere_updateMem(pThreadData, resourceSet, IEMEM_PROBE(iemem_messageBody, 8), pMessage, fullMemSize);
            iere_updateInt64Stat(pThreadData, resourceSet, buffMsgStatType, fullMemSize);
        }
    }
}

//****************************************************************************
/// @brief Return the resourceSet identifier for the specified resourceSet
//****************************************************************************
static inline const char *iere_getResourceSetIdentifier(iereResourceSet_t *resourceSet)
{
    return (resourceSet == iereNO_RESOURCE_SET) ? NULL : resourceSet->stats.resourceSetIdentifier;
}

// Trace something using a non-NULL defaultResourceSet (probably something unexpected)
#define iereDRS_TRACEL(_threadData, _resourceSet, _level, _fmt...)                           \
if ((_resourceSet != iereNO_RESOURCE_SET) && (_resourceSet == iere_getDefaultResourceSet())) \
{                                                                                            \
    ieutTRACEL(_threadData, _resourceSet, _level, _fmt);                                     \
}

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_RESOURCESET_STATS_DEFINED */

/*********************************************************************/
/* End of resourceSetStat.h                                          */
/*********************************************************************/
