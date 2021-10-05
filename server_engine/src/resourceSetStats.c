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
/// @file  resourceSetStats.c
/// @brief Engine component tracking statistics into "resourceset" buckets
//****************************************************************************
#define TRACE_COMP Engine

#include "engineInternal.h"
#include "memHandler.h"
#include "resourceSetStatsInternal.h"

static bool iere_trackingResourceSets = false;

typedef struct tag_iereResetStats_t
{
    iereResourceSet_I64_StatType_t stat;             ///< Stat to reset to 0
    iereResourceSet_I64_StatType_t toLastResetStat;  ///< Stat to update to remember old value (ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS for none)
} iereResetStats_t;

// Stats which ought to be reset when a reset is requested
static iereResetStats_t iereRESETTABLE_CUMULATIVE_STATS[] =
{
    { ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSGS_PUBLISHED,
      ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSGS_PUBLISHED_TO_LASTRESET },
    { ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSGS_PUBLISHED,
      ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSGS_PUBLISHED_TO_LASTRESET },
    { ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSGS_PUBLISHED,
      ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSGS_PUBLISHED_TO_LASTRESET },
    { ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSG_BYTES_PUBLISHED,
      ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSG_BYTES_PUBLISHED_TO_LASTRESET },
    { ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSG_BYTES_PUBLISHED,
      ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSG_BYTES_PUBLISHED_TO_LASTRESET },
    { ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSG_BYTES_PUBLISHED,
      ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSG_BYTES_PUBLISHED_TO_LASTRESET },
    { ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_DISCARDEDMSGS,
      ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_DISCARDEDMSGS_TO_LASTRESET },
    { ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_REJECTEDMSGS,
      ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_REJECTEDMSGS_TO_LASTRESET },
    { ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_CONNECTIONS,
      ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_CONNECTIONS_TO_LASTRESET },
    // Sentinel
    { ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS, ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS }
};

static iereResetStats_t iereRESETTABLE_MAXIMUM_STATS[] =
{
    { ISM_ENGINE_RESOURCESETSTATS_I64_MAX_PUBLISH_RECIPIENTS,
      ISM_ENGINE_RESOURCESETSTATS_I64_MAX_PUBLISH_RECIPIENTS_TO_LASTRESET },
    // Sentinel
    { ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS, ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS }
};

#define iereRESOURCESETSTATS_INITIAL_CAPACITY 1000

//****************************************************************************
/// @brief Create the structure for a resource set with given ID
//****************************************************************************
static int32_t iere_createResourceSet(ieutThreadData_t *pThreadData,
                                      iereResourceSetStatsControl_t *resourceSetStatsControl,
                                      const char *setId,
                                      iereResourceSet_t **pResourceSet)
{
    int32_t rc = OK;

    size_t setIdSize = strlen(setId)+1;

    iereResourceSet_t *newResourceSet = iemem_calloc(pThreadData,
                                                     IEMEM_PROBE(iemem_resourceSetStats, 2),
                                                     1,
                                                     sizeof(iereResourceSet_t)+setIdSize);

    if (newResourceSet == iereNO_RESOURCE_SET)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    newResourceSet->stats.resourceSetIdentifier = (char *)(newResourceSet+1);

    memcpy(newResourceSet->stats.resourceSetIdentifier, setId, setIdSize);

    #ifdef MEMDEBUG
    // See if we should trace memory usage of this resourceSetIdentifier...
    if (resourceSetStatsControl->traceMemorySetId != NULL &&
        ism_common_match(newResourceSet->stats.resourceSetIdentifier, resourceSetStatsControl->traceMemorySetId) == 1)
    {
        newResourceSet->traceMemory = true;
    }
    else
    {
        assert(newResourceSet->traceMemory == false);
    }
    #endif

    int32_t os_rc = pthread_spin_init(&newResourceSet->updateLock, PTHREAD_PROCESS_PRIVATE);
    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "Initializing updateLock failed.", os_rc, NULL);
    }

    *pResourceSet = newResourceSet;

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief Write the contents of the resourceSet to the trace
//****************************************************************************
void iere_traceResourceSet(ieutThreadData_t *pThreadData,
                           int32_t traceLevel,
                           iereResourceSet_t *resourceSet)
{
    if (resourceSet != iereNO_RESOURCE_SET)
    {
        iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;

        char buffer[1024+(ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS*10)];

        sprintf(buffer, "ResourceSetId '%s' ResetTime %lu {",
                resourceSet->stats.resourceSetIdentifier, resourceSetStatsControl->resetTime);

        for(int32_t ires=0; ires<ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS; ires++)
        {
            char thisStat[20];
            sprintf(thisStat, "%d:%ld%c", ires, resourceSet->stats.int64Stats[ires], ires == ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS-1 ? '}':',');
            strcat(buffer, thisStat);
        }
        ieutTRACEL(pThreadData, resourceSet, traceLevel, "%s\n", buffer);
    }
}

//****************************************************************************
/// @brief Destroy the specified resource set
//****************************************************************************
static inline void iere_destroyResourceSet(ieutThreadData_t *pThreadData, iereResourceSet_t *resourceSet)
{
    #if 0
    iere_traceResourceSet(pThreadData, 0, resourceSet);
    #endif

    iemem_free(pThreadData, iemem_resourceSetStats, resourceSet);
}

//****************************************************************************
/// @brief Initialize the RW lock protecting the resourceSet hash table
//****************************************************************************
static void iere_initResourceSetStatsLock(ieutThreadData_t *pThreadData,
                                          iereResourceSetStatsControl_t *resourceSetStatsControl)
{
    pthread_rwlockattr_t rwlockattr_init;

    int32_t os_rc = pthread_rwlockattr_init(&rwlockattr_init);
    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "pthread_rwlockattr_init resourcestats lock", os_rc, NULL);
    }

    os_rc = pthread_rwlockattr_setkind_np(&rwlockattr_init,
                                          PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC(ieutPROBE_002, true, "pthread_rwlockattr_setkind_np resourcesets lock", os_rc, NULL);
    }

    os_rc = pthread_rwlock_init(&(resourceSetStatsControl->setStatsLock), &rwlockattr_init);
    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC(ieutPROBE_003, true, "pthread_rwlock_init resourcesets lock", os_rc, NULL);
    }
}

//****************************************************************************
/// @brief Free one of the sets in the hash table
//****************************************************************************
void iere_freeSetStats(ieutThreadData_t *pThreadData,
                       char *key,
                       uint32_t keyHash,
                       void *value,
                       void *context)
{
    iereResourceSet_t *resourceSet = value;
    assert(strcmp(resourceSet->stats.resourceSetIdentifier,key) == 0);
    iere_destroyResourceSet(pThreadData, resourceSet);
}

//****************************************************************************
/// @brief Destroy the control structure for resourceSets
//****************************************************************************
static void iere_destroyControlStruct(ieutThreadData_t *pThreadData,
                                      iereResourceSetStatsControl_t *resourceSetStatsControl)
{
    // Free the entries in the hash table
    if (resourceSetStatsControl->setStats != NULL)
    {
        ieutHashTableHandle_t localSetStats = resourceSetStatsControl->setStats;

        resourceSetStatsControl->setStats = NULL;

        ieut_traverseHashTable(pThreadData,
                               localSetStats,
                               iere_freeSetStats,
                               NULL);

        ieut_destroyHashTable(pThreadData, localSetStats);
    }

    ism_regex_free(resourceSetStatsControl->clientIdRegEx);
    ism_regex_free(resourceSetStatsControl->topicRegEx);
    iere_destroyResourceSet(pThreadData, resourceSetStatsControl->defaultResourceSet);

    int32_t os_rc = pthread_rwlock_destroy(&(resourceSetStatsControl->setStatsLock));
    if (UNLIKELY(os_rc != OK))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "destroy resourceSetStatsControl lock!", ISMRC_Error
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    iemem_free(pThreadData, iemem_resourceSetStats, resourceSetStatsControl);
}

//****************************************************************************
/// @brief Initialize the resourceSetStats subcomponent
///
/// @return OK on success or an ISMRC_ return code.
//****************************************************************************
int32_t iere_initResourceSetStats(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;
    iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;

    ieutTRACEL(pThreadData, resourceSetStatsControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Initialise resource set stats
    const char *clientIdRegExp = NULL;
    const char *topicStrRegExp = NULL;
    const char *traceMemorySetId = NULL;
    bool trackUnmatched;

    // Admin should already have read these values in...
    ism_admin_getActiveResourceSetDescriptorValues(&clientIdRegExp, &topicStrRegExp);

    // These values are not known to the admin component and come from static config...
    traceMemorySetId = ism_common_getStringConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_MEMTRACE_SETID);
    trackUnmatched = ism_common_getBooleanConfig(ismENGINE_CFGPROP_RESOURCESET_STATS_TRACK_UNMATCHED,
                                                 ismENGINE_DEFAULT_RESOURCESET_STATS_TRACK_UNMATCHED);

    // Treat NULL as empty strings
    if (clientIdRegExp == NULL) clientIdRegExp = "";
    if (topicStrRegExp == NULL) topicStrRegExp = "";
    if (traceMemorySetId == NULL) traceMemorySetId = "";

    assert(clientIdRegExp != NULL && topicStrRegExp != NULL);

    if (clientIdRegExp[0] != '\0' || topicStrRegExp[0] != '\0')
    {
        ieutTRACEL(pThreadData, clientIdRegExp, ENGINE_INTERESTING_TRACE, FUNCTION_IDENT
                   "Tracking ResourceSets ClientIdRegExp=\"%s\" topicStrRegExp=\"%s\" traceMemorySetId=\"%s\" TrackUnmatched=%d\n",
                   __func__, clientIdRegExp, topicStrRegExp, traceMemorySetId, (int)trackUnmatched);

        iere_trackingResourceSets = true;
    }

    if (iere_trackingResourceSets)
    {
        size_t allocSize = sizeof(iereResourceSetStatsControl_t);

        if (traceMemorySetId[0] != '\0')
        {
            #ifdef MEMDEBUG
            allocSize += strlen(traceMemorySetId)+1;
            #else
            ieutTRACEL(pThreadData, traceMemorySetId, ENGINE_ERROR_TRACE, FUNCTION_IDENT "Memory tracing not available for '%s' (need MEMDEBUG)\n",
                       __func__, traceMemorySetId);
            assert(false);
            #endif
        }

        resourceSetStatsControl = iemem_calloc(pThreadData, IEMEM_PROBE(iemem_resourceSetStats, 1), 1, allocSize);

        if (resourceSetStatsControl != NULL)
        {
            ismEngine_SetStructId(resourceSetStatsControl->strucId, IERE_CONTROL_STRUCID);
            #ifdef MEMDEBUG
            if (traceMemorySetId[0] != '\0')
            {
                resourceSetStatsControl->traceMemorySetId = (char *)(resourceSetStatsControl+1);
                strcpy(resourceSetStatsControl->traceMemorySetId, traceMemorySetId);
            }
            #endif
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        int subExprCount;

        if (clientIdRegExp[0] != '\0')
        {
            rc = ism_regex_compile_subexpr(&(resourceSetStatsControl->clientIdRegEx), &subExprCount, clientIdRegExp);

            if (rc != 0 || subExprCount == 0)
            {
                ieutTRACEL(pThreadData, rc, ENGINE_WORRYING_TRACE,
                           "ism_regex_compile_subexpr failed for '%s', subexpcount=%d, rc=%d\n", clientIdRegExp, subExprCount, rc);
                rc = ISMRC_ArgNotValid;
                ism_common_setErrorData(rc, "%s", clientIdRegExp);
                goto mod_exit;
            }
        }

        if (topicStrRegExp[0] != '\0')
        {
            rc = ism_regex_compile_subexpr(&(resourceSetStatsControl->topicRegEx), &subExprCount, topicStrRegExp);

            if (rc != 0 || subExprCount == 0)
            {
                ieutTRACEL(pThreadData, rc, ENGINE_WORRYING_TRACE,
                           "ism_regex_compile_subexpr failed for '%s', subexpcount=%d, rc=%d\n", topicStrRegExp, subExprCount, rc);
                rc = ISMRC_ArgNotValid;
                ism_common_setErrorData(rc, "%s", topicStrRegExp);
                goto mod_exit;
            }
        }

        if (trackUnmatched == true)
        {
            // Create a default resource set in which to track unmatched resources
            rc = iere_createResourceSet(pThreadData,
                                        resourceSetStatsControl,
                                        iereDEFAULT_RESOURCESET_ID,
                                        &resourceSetStatsControl->defaultResourceSet);

            if (rc != OK)
            {
                ism_common_setError(rc);
                goto mod_exit;
            }
        }
        else
        {
            assert(resourceSetStatsControl->defaultResourceSet == iereNO_RESOURCE_SET);
        }

        //Set up the hash table
        rc = ieut_createHashTable(pThreadData,
                                  iereRESOURCESETSTATS_INITIAL_CAPACITY,
                                  iemem_resourceSetStats,
                                  &(resourceSetStatsControl->setStats));

        if (rc != OK)
        {
            ism_common_setError(rc);
            goto mod_exit;
        }

        resourceSetStatsControl->restartTime = ism_common_currentTimeNanos();

        // We're initializing, so the stats are effectively reset as of now.
        resourceSetStatsControl->resetTime = ism_common_currentTimeNanos();

        iere_initResourceSetStatsLock(pThreadData, resourceSetStatsControl);
    }

mod_exit:

    if (rc == OK)
    {
        ismEngine_serverGlobal.resourceSetStatsControl = resourceSetStatsControl;
    }
    else
    {
        if (resourceSetStatsControl != NULL)
        {
            iere_destroyControlStruct(pThreadData, resourceSetStatsControl);
            resourceSetStatsControl = NULL;
        }

        iere_trackingResourceSets = false;
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "resourceSetStatsControl=%p, trackingResourceSets=%d rc=%d\n",
               __func__, resourceSetStatsControl, (int)iere_trackingResourceSets, rc);

    return rc;
}

//****************************************************************************
/// @brief Destroy the resourceSetStats subcomponent
//****************************************************************************
void iere_destroyResourceSetStats(ieutThreadData_t *pThreadData)
{
    iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;

    ieutTRACEL(pThreadData, resourceSetStatsControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (resourceSetStatsControl != NULL)
    {
        iere_trackingResourceSets = false;
        iere_destroyControlStruct(pThreadData, resourceSetStatsControl);
        ismEngine_serverGlobal.resourceSetStatsControl = NULL;
    }

    ieutTRACEL(pThreadData, resourceSetStatsControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "resourceSetStatsControl=%p\n", __func__, resourceSetStatsControl);
}

//****************************************************************************
/// @brief Initialize the per-thread cache of resourceSets
//****************************************************************************
void iere_initResourceSetThreadCache(ieutThreadData_t *pThreadData)
{
    ieutTRACEL(pThreadData, iere_trackingResourceSets, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (iere_trackingResourceSets)
    {
        // Note: Using calloc because this forms a vital part of each threads pThreadData
        pThreadData->resourceSetCache = ism_common_calloc(ISM_MEM_PROBE(ism_memory_engine_misc,17),1, sizeof(iereThreadCache_t));

        assert(pThreadData->resourceSetCache != NULL);
    }
    else
    {
        assert(pThreadData->resourceSetCache == NULL);
    }

    pThreadData->curThreadCacheEntry = NULL;

    ieutTRACEL(pThreadData, pThreadData->resourceSetCache, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);
}

//****************************************************************************
/// @brief Destroy the per-thread cache of resourceSets
//****************************************************************************
void iere_destroyResourceSetThreadCache(ieutThreadData_t *pThreadData)
{
    ism_common_free(ism_memory_engine_misc,pThreadData->resourceSetCache); // used ism_common_free(ism_memory_engine_misc,not iemem_*)
    pThreadData->resourceSetCache = NULL;
}

//****************************************************************************
/// @brief Get a resourceSetId from a specified clientId / topicString
///
/// @remark The returned pointer will point INTO one of the strings - don't
/// free the strings until you've finished with the id.
//****************************************************************************
static int32_t iere_getResourceSetId(ieutThreadData_t *pThreadData,
                                     const char *clientId,
                                     const char *topicString,
                                     const char **IdStart,
                                     uint32_t *IdLen)
{
    int32_t rc = OK;

    assert(iere_trackingResourceSets == true);

    iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;

    // If a clientId has been supplied, use that first.
    if (clientId != NULL && resourceSetStatsControl->clientIdRegEx != NULL)
    {
        ism_regex_matches_t matches[2] = {{0}}; //2 as want overall match and 1st subexpr

        int regexrc = ism_regex_match_subexpr(resourceSetStatsControl->clientIdRegEx,
                                              clientId,
                                              2,
                                              matches);

        if (regexrc == 0)
        {
            //We have a match!!!
            *IdStart = clientId + matches[1].startOffset;
            *IdLen   = matches[1].endOffset - matches[1].startOffset;
            goto mod_exit;
        }
        else if (regexrc == 1)
        {
            //Not matching is not an error.
        }
        else
        {
            ieutTRACEL(pThreadData, regexrc, ENGINE_WORRYING_TRACE, FUNCTION_IDENT
                       "ism_regex_match_subexpr for clientId '%s' returned %d\n",
                       __func__, clientId, regexrc);
        }
    }

    if (topicString != NULL && resourceSetStatsControl->topicRegEx != NULL)
    {
        ism_regex_matches_t matches[2] = {{0}}; //2 as want overall match and 1st subexpr

        int regexrc = ism_regex_match_subexpr(resourceSetStatsControl->topicRegEx,
                                              topicString,
                                              2,
                                              matches);

        if (regexrc == 0)
        {
            //We have a match!!!
            *IdStart = topicString + matches[1].startOffset;
            *IdLen   = matches[1].endOffset - matches[1].startOffset;
            goto mod_exit;
        }
        else if (regexrc == 1)
        {
            //Not matching is not an error.
        }
        else
        {
            ieutTRACEL(pThreadData, regexrc, ENGINE_WORRYING_TRACE, FUNCTION_IDENT
                       "ism_regex_match_subexpr for topicString '%s' returned %d\n",
                       __func__, topicString, regexrc);
        }
    }

    // If we got to here, we didn't match with anything.
    rc = ISMRC_NotFound;

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief Update the global stats for a resourceSet with the values in the
/// specified threadCache slot.
//****************************************************************************
static void iere_copyThreadCacheSlotStats(ieutThreadData_t *pThreadData,
                                          iereThreadCacheEntry_t *chosenSlot)
{
    assert(chosenSlot->resourceSet != iereNO_RESOURCE_SET);
    iereResourceSet_t *resourceSet = chosenSlot->resourceSet;
    const iereResourceSetStats_t *localStats = &chosenSlot->localStats;

    DEBUG_ONLY int osrc = pthread_spin_lock(&resourceSet->updateLock);
    assert(osrc == 0);

    // Cumulative stats, we just add the value whatever it might be
    for (uint32_t i=iereFIRST_CUMULATIVE_STAT; i<(iereLAST_CUMULATIVE_STAT+1); i++)
    {
        resourceSet->stats.int64Stats[i] += localStats->int64Stats[i];
    }

    // Maximum stats, we need to check if our new value is bigger than the biggest so far
    for (uint32_t i=iereFIRST_MAXIMUM_STAT; i<(iereLAST_MAXIMUM_STAT+1); i++)
    {
        if (localStats->int64Stats[i] > resourceSet->stats.int64Stats[i])
        {
            resourceSet->stats.int64Stats[i] = localStats->int64Stats[i];
        }
    }

    DEBUG_ONLY int osrc2 = pthread_spin_unlock(&resourceSet->updateLock);
    assert(osrc2 == 0);

    // Clear the threadCache stats.
    memset(chosenSlot->localStats.int64Stats, 0, sizeof(chosenSlot->localStats.int64Stats));
    chosenSlot->localInUse = false;
    chosenSlot->resourceSet = iereNO_RESOURCE_SET;
}

//****************************************************************************
/// @brief Generate a hash value for a specified resource set Id
///
/// @param[in]     key  The key for which to generate a hash value
///
/// @remark This is a version of the xor djb2 hashing algorithm
///
/// @return The hash value
//****************************************************************************
uint32_t iere_generateResourceSetHash(const char *key)
{
    uint32_t keyHash = 5381;
    char curChar;

    while((curChar = *key++))
    {
        keyHash = (keyHash * 33) ^ curChar;
    }

    return keyHash;
}

//****************************************************************************
/// @brief Find / insert global stats for a resourceSet with specified setId
//****************************************************************************
static int32_t iere_getResourceSetFromResourceSetId(ieutThreadData_t *pThreadData,
                                                    const char *setId,
                                                    iereResourceSet_t **resourceSet,
                                                    int32_t operation)
{
    int32_t rc = OK;
    uint32_t setIdHash = iere_generateResourceSetHash(setId);

    //First find an existing one assuming it is there already - if not we'll add it
    iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;

    ismEngine_getRWLockForRead(&resourceSetStatsControl->setStatsLock);

    rc = ieut_getHashEntry(resourceSetStatsControl->setStats,
                           setId,
                           setIdHash,
                           (void **)resourceSet);

    ismEngine_unlockRWLock(&resourceSetStatsControl->setStatsLock);

    if (rc == ISMRC_NotFound && operation == iereOP_ADD)
    {
        iereResourceSet_t *newResourceSet = iereNO_RESOURCE_SET;

        // Let's make a new one and add it.
        rc = iere_createResourceSet(pThreadData, resourceSetStatsControl, setId, &newResourceSet);

        if (rc != OK) goto mod_exit;

        ismEngine_getRWLockForWrite(&resourceSetStatsControl->setStatsLock);

        rc = ieut_putHashEntry(pThreadData,
                               resourceSetStatsControl->setStats,
                               ieutFLAG_DUPLICATE_NONE,
                               newResourceSet->stats.resourceSetIdentifier,
                               setIdHash,
                               newResourceSet,
                               sizeof(iereResourceSetStats_t));

        if (rc == OK)
        {
            *resourceSet = newResourceSet;
        }
        else if (rc == ISMRC_ExistingKey)
        {
            //Someone else got in and added the resource set... that's fine
            pthread_spin_destroy(&newResourceSet->updateLock);
            iemem_free(pThreadData, iemem_resourceSetStats, newResourceSet);

            rc = ieut_getHashEntry(resourceSetStatsControl->setStats,
                                   setId,
                                   setIdHash,
                                   (void **)resourceSet);

            if (UNLIKELY(rc != OK))
            {
                //We were just told it was there whilst we had a write lock.
                //....Bad, bad things are happening.
                ieutTRACE_FFDC(ieutPROBE_003, true, "couldn't get resource set", rc, NULL);
            }
        }

        ismEngine_unlockRWLock(&resourceSetStatsControl->setStatsLock);
    }

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief Insert a global stat into the local thread cache
/// @remark Doesn't check it's not already there - CALLER MUST DO THAT!
//****************************************************************************
static void iere_insertIntoThreadCache(ieutThreadData_t *pThreadData,
                                       iereResourceSet_t *resourceSet,
                                       iereThreadCacheEntry_t **localCache)
{
    iereThreadCache_t *threadCache = pThreadData->resourceSetCache;
    iereThreadCacheEntry_t *chosenSlot = NULL;

    for (uint32_t i = 0; i < IERE_NUM_THREADCACHE_SETS; i++)
    {
        if (!threadCache->stats[i].localInUse)
        {
            //We'll use this one
            chosenSlot = &(threadCache->stats[i]);
            break;
        }
    }

    if (chosenSlot == NULL)
    {
        //If we've got here, all cache slots are full. Flush first non-sticky set to global stats
        chosenSlot = &(threadCache->stats[IERE_NUM_THREADCACHE_STICKY]);

        assert(chosenSlot->localInUse == true);
        iere_copyThreadCacheSlotStats(pThreadData, chosenSlot);
        assert(chosenSlot->localInUse == false);
    }

    chosenSlot->resourceSet = resourceSet;
    chosenSlot->localStats.resourceSetIdentifier = resourceSet->stats.resourceSetIdentifier;
    chosenSlot->localInUse = true;

    *localCache = chosenSlot;

    return;
}

//****************************************************************************
/// @brief Flush the current thread cache entries into their global sets.
//****************************************************************************
void iere_flushResourceSetThreadCache(ieutThreadData_t *pThreadData)
{
    if (iere_trackingResourceSets)
    {
        iereThreadCache_t *threadCache = pThreadData->resourceSetCache;

        if (threadCache != NULL)
        {
            // For each in use cache entry, push stats, zero stats, mark not-in-use
            for (uint32_t i = 0; i < IERE_NUM_THREADCACHE_SETS; i++)
            {
                iereThreadCacheEntry_t *slot = &(threadCache->stats[i]);
                if (slot->localInUse)
                {
                    iere_copyThreadCacheSlotStats(pThreadData, slot);
                    assert(slot->localInUse == false);
                }
            }
        }
    }
}

//****************************************************************************
/// @brief Return the default resource set (which may be iereNO_RESOURCE_SET)
//****************************************************************************
iereResourceSet_t *iere_getDefaultResourceSet(void)
{
    iereResourceSet_t *defaultResourceSet;

    if (iere_trackingResourceSets)
    {
        iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;
        assert(resourceSetStatsControl != NULL);
        defaultResourceSet = resourceSetStatsControl->defaultResourceSet;
    }
    else
    {
        defaultResourceSet = iereNO_RESOURCE_SET;
    }

    return defaultResourceSet;
}

//****************************************************************************
/// @brief From a clientid & topic string, get global stats.
///
/// @remark If no global set can be determined from either the clientId or
/// topicString, NULL is returned.
//****************************************************************************
iereResourceSet_t *iere_getResourceSet(ieutThreadData_t *pThreadData,
                                       const char *clientId,
                                       const char *topicString,
                                       int32_t operation)
{
    iereResourceSet_t *resourceSet = iereNO_RESOURCE_SET;

    if (iere_trackingResourceSets)
    {
        int32_t rc;

        // Find the resourceSetIdentifier
        const char *setId = NULL;
        uint32_t setIdLen = 0;
        rc = iere_getResourceSetId(pThreadData,
                                   clientId,
                                   topicString,
                                   &setId,
                                   &setIdLen);

        if (rc == ISMRC_NotFound)
        {
            iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;
            assert(resourceSetStatsControl != NULL);
            resourceSet = resourceSetStatsControl->defaultResourceSet;
            goto mod_exit;
        }

        if (rc != OK) goto mod_exit;

        if (setIdLen > IERE_MAX_SETID_LENGTH) setIdLen = IERE_MAX_SETID_LENGTH;

        char localSetId[IERE_MAX_SETID_LENGTH+1];
        memcpy(localSetId, setId, setIdLen);
        localSetId[setIdLen] = '\0';

        // Find (or insert it into) the global stats
        rc = iere_getResourceSetFromResourceSetId(pThreadData,
                                                  localSetId,
                                                  &resourceSet,
                                                  operation);

        assert(rc == OK || resourceSet == iereNO_RESOURCE_SET);
    }

mod_exit:

    return resourceSet;
}

//****************************************************************************
/// @brief Get thread cache entry for a global set
//****************************************************************************
iereThreadCacheEntry_t *iere_getThreadCacheEntryForResourceSet(ieutThreadData_t *pThreadData,
                                                               iereResourceSet_t *resourceSet)
{
    iereThreadCacheEntry_t *localCache = NULL;

    if (iere_trackingResourceSets && resourceSet != iereNO_RESOURCE_SET)
    {
        iereThreadCache_t *threadCache = pThreadData->resourceSetCache;

        //If it's not in the local cache already,
        for (uint32_t i = 0; i < IERE_NUM_THREADCACHE_SETS; i++)
        {
            iereThreadCacheEntry_t *slot = &(threadCache->stats[i]);

            if (slot->resourceSet == resourceSet)
            {
                slot->localInUse = true;
                assert(strcmp(slot->localStats.resourceSetIdentifier, resourceSet->stats.resourceSetIdentifier) == 0);
                localCache = slot;
                goto mod_exit;
            }
        }

        // Insert it in the local cache, flushing oldest non-sticky resource set if cache is full
        iere_insertIntoThreadCache(pThreadData,
                                   resourceSet,
                                   &localCache);
    }

mod_exit:

    return localCache;
}

//****************************************************************************
/// @brief Call the callback function when traversing the stats tree
//****************************************************************************
typedef struct tag_iereTraverseCallbackContext_t
{
    iereEnumerateCallback_t callback;
    ism_time_t resetTime;
    void *callersContext;
} iereTraverseCallbackContext_t;

void iere_traverseCallback(ieutThreadData_t *pThreadData,
                           char *key,
                           uint32_t keyHash,
                           void *value,
                           void *context)
{
    iereTraverseCallbackContext_t *pContext = (iereTraverseCallbackContext_t *)context;
    iereResourceSet_t *resourceSet = value;
    assert(strcmp(resourceSet->stats.resourceSetIdentifier,key) == 0);
    pContext->callback(pThreadData, resourceSet, pContext->resetTime, pContext->callersContext);
}

//****************************************************************************
/// @brief Enumerate all of the resource sets in the table
//****************************************************************************
void iere_enumerateResourceSets(ieutThreadData_t *pThreadData,
                                iereEnumerateCallback_t callback,
                                void *context)
{
    if (iere_trackingResourceSets)
    {
        iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;
        iereResourceSet_t *defaultResourceSet = resourceSetStatsControl->defaultResourceSet;

        ieutTRACEL(pThreadData, callback, ENGINE_FNC_TRACE, FUNCTION_IDENT "callback=%p, context=%p setStats=%p\n",
                   __func__, callback, context, resourceSetStatsControl->setStats);

        ismEngine_getRWLockForRead(&resourceSetStatsControl->setStatsLock);

        // Always call them with the default resource set first...
        if (defaultResourceSet != iereNO_RESOURCE_SET)
        {
            callback(pThreadData, defaultResourceSet, resourceSetStatsControl->resetTime, context);
        }

        // Go through all of the entries in the table
        if (resourceSetStatsControl->setStats != NULL)
        {
            // Now go through the collected set...
            iereTraverseCallbackContext_t traverseCallbackContext = {callback, resourceSetStatsControl->resetTime, context};

            ieut_traverseHashTable(pThreadData,
                                   resourceSetStatsControl->setStats,
                                   iere_traverseCallback,
                                   &traverseCallbackContext);
        }

        ismEngine_unlockRWLock(&resourceSetStatsControl->setStatsLock);
    }
}

//****************************************************************************
/// @brief 'Enumerate' a single specified resource set
//****************************************************************************
void iere_enumerateSingleResourceSet(ieutThreadData_t *pThreadData,
                                     const char *resourceSetId,
                                     iereEnumerateCallback_t callback,
                                     void *context)
{
    if (iere_trackingResourceSets && resourceSetId)
    {
        iereResourceSet_t *resourceSet;
        iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;

        ieutTRACEL(pThreadData, callback, ENGINE_FNC_TRACE, FUNCTION_IDENT "callback=%p, context=%p setStats=%p\n",
                   __func__, callback, context, resourceSetStatsControl->setStats);

        ismEngine_getRWLockForRead(&resourceSetStatsControl->setStatsLock);

        int32_t rc = ieut_getHashEntry(resourceSetStatsControl->setStats,
                                       resourceSetId,
                                       iere_generateResourceSetHash(resourceSetId),
                                       (void **)&resourceSet);

        if (rc == ISMRC_NotFound)
        {
            iereResourceSet_t *defaultResourceSet = resourceSetStatsControl->defaultResourceSet;

            if (defaultResourceSet && strcmp(defaultResourceSet->stats.resourceSetIdentifier, resourceSetId) == 0)
            {
                resourceSet = defaultResourceSet;
                rc = OK;
            }
        }

        ismEngine_unlockRWLock(&resourceSetStatsControl->setStatsLock);

        if (rc == OK)
        {
            callback(pThreadData, resourceSet, resourceSetStatsControl->resetTime, context);
        }
    }
}

//****************************************************************************
/// @brief Reset the resettable stats on a particular resourceSet
//****************************************************************************
static void iere_resetResourceSet(ieutThreadData_t *pThreadData,
                                  iereResourceSet_t *pResourceSet,
                                  ism_time_t prevResetTime,
                                  void *pContext)
{
    DEBUG_ONLY int osrc = pthread_spin_lock(&pResourceSet->updateLock);
    assert(osrc == 0);

    iereResourceSetStats_t *stats = &pResourceSet->stats;

    // Cumulative stats
    iereResetStats_t *thisResetStat = iereRESETTABLE_CUMULATIVE_STATS;

    while(thisResetStat->stat != ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS)
    {
        iereResourceSet_I64_StatType_t thisStat = thisResetStat->stat;
        iereResourceSet_I64_StatType_t thisToLastResetStat = thisResetStat->toLastResetStat;

        assert(thisStat < ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS);

        if (thisToLastResetStat != ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS)
        {
            assert(thisToLastResetStat < ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS);

            stats->int64Stats[thisToLastResetStat] += stats->int64Stats[thisStat];
        }

        stats->int64Stats[thisStat] = 0;

        thisResetStat++;
    }

    // Maximum stats
    thisResetStat = iereRESETTABLE_MAXIMUM_STATS;

    while(thisResetStat->stat != ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS)
    {
        iereResourceSet_I64_StatType_t thisStat = thisResetStat->stat;
        iereResourceSet_I64_StatType_t thisToLastResetStat = thisResetStat->toLastResetStat;

        assert(thisStat < ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS);

        if (thisToLastResetStat != ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS)
        {
            assert(thisToLastResetStat < ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS);

            if (stats->int64Stats[thisStat] > stats->int64Stats[thisToLastResetStat])
            {
                stats->int64Stats[thisToLastResetStat] = stats->int64Stats[thisStat];
            }
        }

        stats->int64Stats[thisStat] = 0;

        thisResetStat++;
    }

    DEBUG_ONLY int osrc2 = pthread_spin_unlock(&pResourceSet->updateLock);
    assert(osrc2 == 0);
}

//****************************************************************************
/// @brief Reset the resettable stats on all resourceSets
//****************************************************************************
void iere_resetResourceSetStats(ieutThreadData_t *pThreadData)
{
    if (iere_trackingResourceSets)
    {
        ism_time_t resetTime = ism_common_currentTimeNanos();

        iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;
        iereResourceSet_t *defaultResourceSet = resourceSetStatsControl->defaultResourceSet;

        ieutTRACEL(pThreadData, resetTime, ENGINE_FNC_TRACE, FUNCTION_IDENT "resetTime=%lu\n",
                   __func__, resetTime);

        // Only want a single thread to be resetting at a time
        ismEngine_getRWLockForWrite(&resourceSetStatsControl->setStatsLock);

        // Always call them with the default resource set first...
        if (defaultResourceSet != iereNO_RESOURCE_SET)
        {
            iere_resetResourceSet(pThreadData, defaultResourceSet, resourceSetStatsControl->resetTime, NULL);
        }

        // Go through all of the entries in the table
        if (resourceSetStatsControl->setStats != NULL)
        {
            // Now go through the collected set...
            iereTraverseCallbackContext_t traverseCallbackContext = {iere_resetResourceSet,
                                                                     resourceSetStatsControl->resetTime,
                                                                     NULL};

            ieut_traverseHashTable(pThreadData,
                                   resourceSetStatsControl->setStats,
                                   iere_traverseCallback,
                                   &traverseCallbackContext);
        }

        resourceSetStatsControl->resetTime = resetTime;

        ismEngine_unlockRWLock(&resourceSetStatsControl->setStatsLock);
    }
}

//****************************************************************************
/// @brief Return whether resource set tracking is enabled
//****************************************************************************
bool iere_isResourceSetTrackingEnabled(void)
{
    return iere_trackingResourceSets;
}
