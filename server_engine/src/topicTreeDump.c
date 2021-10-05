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
/// @file  topicTreeDump.c
/// @brief Engine component topic tree dumping functions
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
#include "queueCommon.h"          // ieq functions & constants
#include "engineDump.h"           // iedm functions & constants
#include "resourceSetStats.h"     // iere functions & constants

//****************************************************************************
/// @internal
///
/// @brief Write the descriptions of topic tree structures to the dump
///
/// @param[in]     dumpHdl  Pointer to a dump structure
//****************************************************************************
void iett_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    iedm_describe_iettTopicTree_t(dump->fp);
    iedm_describe_iettSubsNode_t(dump->fp);
    iedm_describe_iettSubscriptionList_t(dump->fp);
    iedm_describe_ismEngine_Subscription_t(dump->fp);
    iedm_describe_iettSharedSubData_t(dump->fp);
    iedm_describe_iettNewSubCreationData_t(dump->fp);
    iedm_describe_iettSharingClient_t(dump->fp);
    iedm_describe_iettTopicNode_t(dump->fp);
    iedm_describe_iettRemSrvNode_t(dump->fp);
    iedm_describe_iettRemoteServerList_t(dump->fp);
    iedm_describe_iettOriginServerStats_t(dump->fp);
    iedm_describe_iettOriginServer_t(dump->fp);
}

//****************************************************************************
/// @brief Dump the contents of a subscription
///
/// @param[in]     subscription  Subscription to dump
/// @param[in]     dumpHdl       Handle to the dump
//****************************************************************************
void iett_dumpSubscription(ieutThreadData_t *pThreadData,
                           ismEngine_Subscription_t *subscription,
                           iedmDumpHandle_t  dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    if (iedm_dumpStartObject(dump, subscription) == true)
    {
        iedm_dumpStartGroup(dump, "Subscription");

        iedm_dumpData(dump, "ismEngine_Subscription_t", subscription,
                      iere_usable_size(iemem_subsTree, subscription));

        // Explicitly dump the sharedSubData (even though it's included in the subscription area)
        iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);

        if (sharedSubData != NULL)
        {
            iedm_dumpData(dump, "iettSharedSubData_t", sharedSubData, sizeof(*sharedSubData));
        }

        // Explicitly dump the newSubCreationData
        iettNewSubCreationData_t *newSubCreationData = iett_getNewSubCreationData(subscription);

        if (newSubCreationData != NULL)
        {
            iedm_dumpData(dump, "iettNewSubCreationData_t", newSubCreationData, sizeof(*newSubCreationData));
        }

        if ((NULL != subscription->queueHandle) && (dump->detailLevel >= iedmDUMP_DETAIL_LEVEL_5))
        {
            ieq_dump(pThreadData, subscription->queueHandle, dumpHdl);
        }

        iedm_dumpEndGroup(dump);
        iedm_dumpEndObject(dump, subscription);
    }
}

//****************************************************************************
/// @brief Dump the contents of a subcription node
///
/// @param[in]     node  Node to dump
/// @param[in]     dump  Pointer to a dump structure
//****************************************************************************
void iett_dumpSubsNode(ieutThreadData_t *pThreadData,
                       iettSubsNode_t *node,
                       iedmDump_t     *dump)
{
    if (iedm_dumpStartObject(dump, node) == true)
    {
        iedm_dumpStartGroup(dump, "SubsNode");

        iedm_dumpData(dump, "iettSubsNode_t", node,
                      iemem_usable_size(iemem_subsTree, node));

        if (NULL != node->children)
        {
            iedm_dumpData(dump, "ieutHashTable_t", node->children,
                          iemem_usable_size(iemem_subsTree, node->children));
        }

        // Dump each of the active subscriptions
        if (dump->detailLevel >= iedmDUMP_DETAIL_LEVEL_3)
        {
            uint32_t subscriberCount;

            if ((subscriberCount = node->activeSubs.count) != 0)
            {
                iedm_dumpStartGroup(dump, "Active");
                for(int32_t count=0; count<subscriberCount; count++)
                {
                    iett_dumpSubscription(pThreadData,
                                          node->activeSubs.list[count],
                                          (iedmDumpHandle_t)dump);
                }
                iedm_dumpEndGroup(dump);
            }

            if ((subscriberCount = node->delPendSubs.count) != 0)
            {
                iedm_dumpStartGroup(dump, "DeletePending");
                for(int32_t count=0; count<subscriberCount; count++)
                {
                    iett_dumpSubscription(pThreadData,
                                          node->delPendSubs.list[count],
                                          (iedmDumpHandle_t)dump);
                }
                iedm_dumpEndGroup(dump);
            }
        }

        iedm_dumpEndGroup(dump);
        iedm_dumpEndObject(dump, node);
    }
}

//****************************************************************************
/// @brief Dump the contents of a remote server node
///
/// @param[in]     node  Node to dump
/// @param[in]     dump  Pointer to a dump structure
//****************************************************************************
void iett_dumpRemSrvNode(ieutThreadData_t *pThreadData,
                         iettRemSrvNode_t *node,
                         iedmDump_t       *dump)
{
    if (iedm_dumpStartObject(dump, node) == true)
    {
        iedm_dumpStartGroup(dump, "RemSrvNode");

        iedm_dumpData(dump, "iettRemSrvNode_t", node,
                      iemem_usable_size(iemem_remoteServers, node));

        if (NULL != node->children)
        {
            iedm_dumpData(dump, "ieutHashTable_t", node->children,
                          iemem_usable_size(iemem_remoteServers, node->children));
        }

        // Dump the active remote servers list
        if (dump->detailLevel >= iedmDUMP_DETAIL_LEVEL_3)
        {
            if (node->activeServers.list != NULL)
            {
                iedm_dumpData(dump, "ismEngine_RemoteServer_t **", node->activeServers.list,
                              iemem_usable_size(iemem_remoteServers, node->activeServers.list));
            }
        }

        iedm_dumpEndGroup(dump);
        iedm_dumpEndObject(dump, node);
    }
}

//****************************************************************************
/// @brief Dump the contents of a topic node
///
/// @param[in]     node  Node to dump
/// @param[in]     dump  Pointer to a dump structure
//****************************************************************************
void iett_dumpTopicNode(iettTopicNode_t *node, iedmDump_t *dump)
{
    if (iedm_dumpStartObject(dump, node) == true)
    {
        iedm_dumpStartGroup(dump, "TopicNode");

        iedm_dumpData(dump, "iettTopicNode_t", node,
                      iemem_usable_size(iemem_topicsTree, node));

        if (NULL != node->children)
        {
            iedm_dumpData(dump, "ieutHashTable_t", node->children,
                          iemem_usable_size(iemem_topicsTree, node->children));
        }

        iedm_dumpEndGroup(dump);
        iedm_dumpEndObject(dump, node);
    }
}

//****************************************************************************
/// @internal
///
/// @brief Dump the subscription and topic node for a specified topic string
///
/// @param[in]     topicString  Topic to dump
/// @param[in]     dumpHdl      Dump to write to
///
/// @returns OK or an ISMRC_ on error.
//****************************************************************************
int32_t iett_dumpTopic(ieutThreadData_t *pThreadData,
                       const char *topicString,
                       iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    int32_t rc;
    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *wildcards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *multicards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ieutTRACEL(pThreadData, topicString, ENGINE_CEI_TRACE, FUNCTION_ENTRY "topicString='%s'\n", __func__,
               topicString);

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = topicString;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

    iedm_dumpStartGroup(dump, "Topic");

    rc = iett_analyseTopicString(pThreadData, &topic);

    if (rc != OK) goto mod_exit;

    // Dump the subscription node for supplied topic string
    iettSubsNode_t *subsNode = NULL;

    ismEngine_getRWLockForRead(&tree->subsLock);

    (void)iett_insertOrFindSubsNode(pThreadData, tree->subs, &topic, iettOP_FIND, &subsNode);

    if (subsNode != NULL)
    {
        iett_dumpSubsNode(pThreadData, subsNode, dump);
    }

    ismEngine_unlockRWLock(&tree->subsLock);

    // Dump the topics node for the supplied topic string
    iettTopicNode_t *topicsNode = NULL;

    ismEngine_getRWLockForRead(&tree->topicsLock);

    (void)iett_insertOrFindTopicsNode(pThreadData, tree->topics, &topic, iettOP_FIND, &topicsNode);

    if (topicsNode != NULL)
    {
        iett_dumpTopicNode(topicsNode, dump);
    }

    ismEngine_unlockRWLock(&tree->topicsLock);

    iedm_dumpEndGroup(dump);

    // Didn't find either a subsNode or a topicsNode
    if (subsNode == NULL && topicsNode == NULL) rc = ISMRC_NotFound;

mod_exit:

    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
        if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
        if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Context for iett_dumpSubsTreeCallback and iett_dumpTopicsTreeCallback
///
/// @see iett_dumpSubsTreeCallback
/// @see iett_dumpRemSrvTreeCallback
/// @see iett_dumTopicsTreeCallback
//****************************************************************************
typedef struct tag_iettDumpTreeCbContext_t
{
    iedmDump_t *dump;
} iettDumpTreeCbContext_t;

//****************************************************************************
// Forward declaration of iett_dumpSubsTreeNode
//****************************************************************************
void iett_dumpSubsTreeNode(ieutThreadData_t *pThreadData,
                           iettSubsNode_t *node,
                           iettDumpTreeCbContext_t *context);

//****************************************************************************
/// @brief Callback used when dumping a subscription tree
///
/// @param[in]     key      Key being processed (topic substring)
/// @param[in]     keyHash  Hash for the key being processed
/// @param[in]     value    Value from the hash table (iettSubsNode_t)
/// @param[in,out] context  Context information
///
/// @see iett_dumpTopicTree
//****************************************************************************
void iett_dumpSubsTreeCallback(ieutThreadData_t *pThreadData,
                               char *key,
                               uint32_t keyHash,
                               void *value,
                               void *context)
{
    iett_dumpSubsTreeNode(pThreadData,
                          (iettSubsNode_t *)value,
                          (iettDumpTreeCbContext_t *)context);
}

//****************************************************************************
/// @brief Write all of the subscription nodes below a given node to the dump.
///
/// @param[in]     node     Node from which to begin printing.
/// @param[in,out] context  Context of the dump
///
/// @see iett_dumpTopicTree
//****************************************************************************
void iett_dumpSubsTreeNode(ieutThreadData_t *pThreadData,
                           iettSubsNode_t *node,
                           iettDumpTreeCbContext_t *context)
{
    iett_dumpSubsNode(pThreadData, node, context->dump);

    if (node->children)
    {
        ieut_traverseHashTable(pThreadData,
                               node->children,
                               iett_dumpSubsTreeCallback,
                               context);
    }

    if (node->wildcardChild)
    {
        iett_dumpSubsTreeNode(pThreadData, node->wildcardChild, context);
    }

    if (node->multicardChild)
    {
        iett_dumpSubsTreeNode(pThreadData, node->multicardChild, context);
    }
}

//****************************************************************************
// Forward declaration of iett_dumpRemSrvTreeNode
//****************************************************************************
void iett_dumpRemSrvTreeNode(ieutThreadData_t *pThreadData,
                             iettRemSrvNode_t *node,
                             iettDumpTreeCbContext_t *context);

//****************************************************************************
/// @brief Callback used when dumping a remote server tree
///
/// @param[in]     key      Key being processed (topic substring)
/// @param[in]     keyHash  Hash for the key being processed
/// @param[in]     value    Value from the hash table (iettRemSrvNode_t)
/// @param[in,out] context  Context information
///
/// @see iett_dumpTopicTree
//****************************************************************************
void iett_dumpRemSrvTreeCallback(ieutThreadData_t *pThreadData,
                                 char *key,
                                 uint32_t keyHash,
                                 void *value,
                                 void *context)
{
    iett_dumpRemSrvTreeNode(pThreadData,
                            (iettRemSrvNode_t *)value,
                            (iettDumpTreeCbContext_t *)context);
}

//****************************************************************************
/// @brief Write all of the remote server nodes below a given node to the dump.
///
/// @param[in]     node     Node from which to begin printing.
/// @param[in,out] context  Context of the dump
///
/// @see iett_dumpTopicTree
//****************************************************************************
void iett_dumpRemSrvTreeNode(ieutThreadData_t *pThreadData,
                             iettRemSrvNode_t *node,
                             iettDumpTreeCbContext_t *context)
{
    iett_dumpRemSrvNode(pThreadData, node, context->dump);

    if (node->children)
    {
        ieut_traverseHashTable(pThreadData,
                               node->children,
                               iett_dumpRemSrvTreeCallback,
                               context);
    }

    if (node->wildcardChild)
    {
        iett_dumpRemSrvTreeNode(pThreadData, node->wildcardChild, context);
    }

    if (node->multicardChild)
    {
        iett_dumpRemSrvTreeNode(pThreadData, node->multicardChild, context);
    }
}

//****************************************************************************
// Forward declaration of iett_dumpSubsTreeNode
//****************************************************************************
void iett_dumpTopicsTreeNode(ieutThreadData_t *pThreadData,
                             iettTopicNode_t *node,
                             iettDumpTreeCbContext_t *context);

//****************************************************************************
/// @brief Callback used when dumping a topics tree
///
/// @param[in]     key      Key being processed (topic substring)
/// @param[in]     keyHash  Hash for the key being processed
/// @param[in]     value    Value from the hash table (iettSubsNode_t)
/// @param[in,out] context  Context information
///
/// @see iett_dumpTopicTree
//****************************************************************************
void iett_dumpTopicsTreeCallback(ieutThreadData_t *pThreadData,
                                 char *key,
                                 uint32_t keyHash,
                                 void *value,
                                 void *context)
{
    iett_dumpTopicsTreeNode(pThreadData,
                            (iettTopicNode_t *)value,
                            (iettDumpTreeCbContext_t *)context);
}

//****************************************************************************
/// @brief Write all of the topics nodes below a given node to the dump.
///
/// @param[in]     node     Node from which to begin printing.
/// @param[in,out] context  Context of the dump
///
/// @see iett_dumpTopicTree
//****************************************************************************
void iett_dumpTopicsTreeNode(ieutThreadData_t *pThreadData,
                             iettTopicNode_t *node,
                             iettDumpTreeCbContext_t *context)
{
    iett_dumpTopicNode(node, context->dump);

    if (node->children)
    {
        ieut_traverseHashTable(pThreadData,
                               node->children,
                               iett_dumpTopicsTreeCallback,
                               context);
    }
}

//****************************************************************************
/// @internal
///
/// @brief Dump the subscription & topic subtrees below the specified topic
///        string.
///
/// @param[in]     rootTopicString  Root of the topic subtree to dump (NULL
///                                 for entire tree)
/// @param[in]     dumpHdl          Dump to write to
///
/// @returns OK or an ISMRC_ on error.
//****************************************************************************
int32_t iett_dumpTopicTree(ieutThreadData_t *pThreadData,
                           const char *rootTopicString,
                           iedmDumpHandle_t dumpHdl)
{
    int32_t rc = OK;

    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *wildcards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *multicards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ieutTRACEL(pThreadData, rootTopicString, ENGINE_CEI_TRACE, FUNCTION_ENTRY "rootTopicString='%s'\n", __func__,
               rootTopicString);

    if (rootTopicString != NULL)
    {
        topic.destinationType = ismDESTINATION_TOPIC;
        topic.topicString = rootTopicString;
        topic.substrings = substrings;
        topic.substringHashes = substringHashes;
        topic.wildcards = wildcards;
        topic.multicards = multicards;
        topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

        rc = iett_analyseTopicString(pThreadData, &topic);

        // If iett_analyseTopicString failed due to lack of memory, dump the entire tree
        if (rc == ISMRC_AllocateError)
        {
            rc = OK;
            rootTopicString = NULL;
        }
        else if (rc != OK)
        {
            goto mod_exit;
        }
    }

    iedm_dumpStartGroup(dump, "TopicTree");

    // Always dump the topic tree structure
    iedm_dumpData(dump, "iettTopicTree_t", tree, iemem_usable_size(iemem_subsTree, tree));

    // Dump the subscription tree either from the root or the supplied topic string
    iettSubsNode_t *rootSubsNode = tree->subs;

    // We measure how long these dumps take for information
    ism_time_t startTime, endTime, diffTime;
    double diffTimeSecs;

    startTime = ism_common_currentTimeNanos();
    ismEngine_getRWLockForRead(&tree->subsLock);

    if (rootTopicString != NULL)
    {
        (void)iett_insertOrFindSubsNode(pThreadData, tree->subs, &topic, iettOP_FIND, &rootSubsNode);
    }

    if (rootSubsNode != NULL)
    {
        iettDumpTreeCbContext_t context = {dump};

        iett_dumpSubsTreeNode(pThreadData, rootSubsNode, &context);
    }

    ismEngine_unlockRWLock(&tree->subsLock);
    endTime = ism_common_currentTimeNanos();

    diffTime = endTime-startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;

    ieutTRACEL(pThreadData, diffTime, ENGINE_HIGH_TRACE, "Dumping subsTree took %.2f secs (%ldns)\n", diffTimeSecs, diffTime);

    // Dump the remote server tree either from the root or the supplied topic string
    iettRemSrvNode_t *rootRemSrvNode = tree->remoteServers;

    startTime = ism_common_currentTimeNanos();
    ismEngine_getRWLockForRead(&tree->remoteServersLock);

    if (rootTopicString != NULL)
    {
        (void)iett_insertOrFindRemSrvNode(pThreadData, tree->remoteServers, &topic, iettOP_FIND, &rootRemSrvNode);
    }

    if (rootRemSrvNode != NULL)
    {
        iettDumpTreeCbContext_t context = {dump};

        iett_dumpRemSrvTreeNode(pThreadData, rootRemSrvNode, &context);
    }

    ismEngine_unlockRWLock(&tree->remoteServersLock);
    endTime = ism_common_currentTimeNanos();

    diffTime = endTime-startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;

    ieutTRACEL(pThreadData, diffTime, ENGINE_HIGH_TRACE, "Dumping remSrvsTree took %.2f secs (%ldns)\n", diffTimeSecs, diffTime);

    // Dump the topics tree either from the root or the supplied topic string
    iettTopicNode_t *rootTopicsNode = tree->topics;

    startTime = ism_common_currentTimeNanos();
    ismEngine_getRWLockForRead(&tree->topicsLock);

    if (rootTopicString != NULL)
    {
        (void)iett_insertOrFindTopicsNode(pThreadData, tree->topics, &topic, iettOP_FIND, &rootTopicsNode);
    }

    if (rootTopicsNode != NULL)
    {
        iettDumpTreeCbContext_t context = {dump};

        iett_dumpTopicsTreeNode(pThreadData, rootTopicsNode, &context);
    }

    ismEngine_unlockRWLock(&tree->topicsLock);
    endTime = ism_common_currentTimeNanos();

    diffTime = endTime-startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;

    ieutTRACEL(pThreadData, diffTime, ENGINE_HIGH_TRACE, "Dumping topicsTree took %.2f secs (%ldns)\n", diffTimeSecs, diffTime);

    iedm_dumpEndGroup(dump);

    // Didn't find anything in the topic tree specified, return 'not found'
    if (rootTopicString != NULL &&
        (rootSubsNode == NULL && rootTopicsNode == NULL))
    {
        rc = ISMRC_NotFound;
    }

mod_exit:

    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
        if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
        if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}
