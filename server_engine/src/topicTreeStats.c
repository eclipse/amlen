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
/// @file  topicTreeStats.c
/// @brief Engine component topic tree per-thread data manipulation functions
//****************************************************************************
#define TRACE_COMP Engine

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>

#include "engineInternal.h"
#include "topicTree.h"
#include "topicTreeInternal.h"

//****************************************************************************
/// @brief Configuration callback to handle topic monitor objects
///
/// @param[in]     changedProps      Properties qualified by string identifier
///                                  as a suffix
/// @param[in]     changeType        The type of change being made
///
/// @return OK on successful completion or another ISMRC_ value on error.
//****************************************************************************
int iett_topicMonitorConfigCallback(ieutThreadData_t *pThreadData,
                                    ism_prop_t *changedProps,
                                    ism_ConfigChangeType_t changeType)
{
    int rc = OK;

    ieutTRACEL(pThreadData, changeType, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Get the topic string from the properties passed in
    const char *topicString = NULL;

    // Simplest way to find properties is to loop through looking for the prefix
    const char *propertyName;
    for (int32_t i = 0; ism_common_getPropertyIndex(changedProps, i, &propertyName) == 0; i++)
    {
        if (strncmp(propertyName,
                    ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING,
                    strlen(ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING)) == 0)
        {
            topicString = ism_common_getStringProperty(changedProps, propertyName);
            ieutTRACEL(pThreadData, topicString, ENGINE_NORMAL_TRACE, "topicString='%s'\n", topicString);
            break;
        }
    }

    // Didn't get a topic string
    if (topicString == NULL)
    {
        rc = ISMRC_InvalidParameter;
        goto mod_exit;
    }

    // The action taken varies depending on the requested changeType.
    switch(changeType)
    {
        case ISM_CONFIG_CHANGE_PROPS:
            // If we ever support a reset option for a topic monitor, we can do so
            // by passing the value of the appropriate property in here (true means
            // reset).
            rc = ism_engine_startTopicMonitor(topicString, false);
            break;

        case ISM_CONFIG_CHANGE_DELETE:
            rc = ism_engine_stopTopicMonitor(topicString);
            break;

        default:
            rc = ISMRC_InvalidOperation;
            break;
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;

}

//****************************************************************************
/// @brief Activate statistics gathering on the specified topic string
///
/// @param[in]     topicString       Topic string on which to start collecting
///                                  stats (Note: This can be wildcarded)
/// @param[in]     resetActiveStats  Whether to reset already active statistics
///                                  gathering.
///
/// @remark If statistics gathering is already enabled on the specified node, the
///         stats for it are reset by this call only if resetActiveStats is true.
///
/// @return OK on successful completion or another ISMRC_ value on error.
///
/// @see iett_deactivateSubsNodeStats
//****************************************************************************
int32_t iett_activateSubsNodeStats(ieutThreadData_t *pThreadData,
                                   const char *topicString,
                                   bool resetActiveStats)
{
    int32_t rc;
    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *wildcards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *multicards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];

    ieutTRACEL(pThreadData, topicString,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "topicString='%s'\n", __func__, topicString);

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = topicString;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

    rc = iett_analyseTopicString(pThreadData, &topic);

    if (rc != OK) goto mod_exit_no_unlock;

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    // Get the lock for Write
    ismEngine_getRWLockForWrite(&tree->subsLock);

    iettSubsNode_t *subsNode = NULL;

    rc = iett_insertOrFindSubsNode(pThreadData, tree->subs, &topic, iettOP_ADD, &subsNode);

    if (rc == OK)
    {
        // No stats structure already attached to this subsNode, allocate one now.
        if (subsNode->stats == NULL)
        {
            iettSubsNodeStats_t *localStats = iemem_calloc(pThreadData,
                                                           iemem_subsTree, 1,
                                                           sizeof(iettSubsNodeStats_t));

            if (localStats == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            localStats->node = subsNode;
            subsNode->stats = localStats;

            // Add this stats structure to the list of stats structures
            subsNode->stats->next = tree->subNodeStatsHead;
            if (tree->subNodeStatsHead != NULL)
            {
                tree->subNodeStatsHead->prev = subsNode->stats;
            }
            tree->subNodeStatsHead = subsNode->stats;
        }

        assert(subsNode->stats != NULL);

        // If forcing a reset of stats, or if they are not yet active (reset time is zero)
        // reset the values now, and record a new reset time.
        if (resetActiveStats || subsNode->stats->topicStats.ResetTime == 0)
        {
            subsNode->stats->topicStats.PublishedMsgs = 0;
            subsNode->stats->topicStats.RejectedMsgs = 0;
            subsNode->stats->topicStats.FailedPublishes = 0;

            subsNode->stats->topicStats.ResetTime = ism_common_currentTimeNanos();

            // Increment the count of active subNodeStats structures
            (void)__sync_fetch_and_add(&tree->activeSubNodeStats, 1);

            // Update the topic tree's subscription update count to avoid using per-thread cache
            tree->subsUpdates++;
            // Update the topic tree's cache update count to avoid using tree cache
            tree->cacheUpdates++;
        }
        else
        {
            rc = ISMRC_ExistingTopicMonitor;
            goto mod_exit;
        }
    }

mod_exit:

    // Unlock if we locked the subscription tree
    ismEngine_unlockRWLock(&tree->subsLock);

mod_exit_no_unlock:

    // Free any topic string analysis data
    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
        if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
        if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Deactivate statistics gathering on the specified topic node
///
/// @param[in]     topicString    Topic string on which to stop collecting stats
///
/// @remark This function turns gathering off, by setting the reset time to zero,
///         but does not remove the stats structure from the node (or zero the stats).
///         When the node is free'd the stats are also free'd.
///
/// @return OK on successful completion or another ISMRC_ value on error.
///
/// @see iett_activateSubsNodeStats
//****************************************************************************
int32_t iett_deactivateSubsNodeStats(ieutThreadData_t *pThreadData,
                                     const char *topicString)
{
    int32_t rc;
    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *wildcards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *multicards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];

    ieutTRACEL(pThreadData, topicString,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "topicString='%s'\n", __func__, topicString);

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = topicString;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

    rc = iett_analyseTopicString(pThreadData, &topic);

    // Not possible for stats to be active - nothing to do
    if (rc != OK) goto mod_exit_no_unlock;

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    // Get the lock for Read
    ismEngine_getRWLockForRead(&tree->subsLock);

    iettSubsNode_t *subsNode = NULL;

    rc = iett_insertOrFindSubsNode(pThreadData, tree->subs, &topic, iettOP_FIND, &subsNode);

    // Didn't find the node - nothing to do
    if (rc != OK) goto mod_exit;

    assert(subsNode != NULL);

    // If the node is found, has stats and they are active, deactivate them.
    // Note: We are updating the node even though we only hold the lock for read,
    //       this is because we don't actually free anything, and are effectively
    //       just stopping further updates to the statistics.
    if (subsNode->stats != NULL && subsNode->stats->topicStats.ResetTime != 0)
    {
        subsNode->stats->topicStats.ResetTime = 0;

        // Decrement the count of active subNodeStats structures
        (void)__sync_fetch_and_sub(&tree->activeSubNodeStats, 1);
    }
    else
    {
        rc = ISMRC_NotFound;
    }

mod_exit:

    // Unlock if we locked the subscription tree
    ismEngine_unlockRWLock(&tree->subsLock);

mod_exit_no_unlock:

    // Free any topic string analysis data
    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
        if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
        if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}
