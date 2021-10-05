/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
/// @file  topicTreeRemote.c
/// @brief Engine component topic tree remote server / cluster functions
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
#include "msgCommon.h"           // iem_ functions
#include "engineStore.h"         // iest_ functions

//****************************************************************************
/// @brief Find an existing node underneath a given parent node that matches a
///        given topic string in a remote servers tree. Optionally, if an
///        existing node is not found, insert one and return the newly inserted
///        node.
///
/// @param[in]     parent         Parent node in remote server tree
/// @param[in]     topic          The topic being inserted / added.
/// @param[in]     operation      The operation being performed requiring
///                               this node.
/// @param[out]    node           A pointer to the node pointer being returned
///
/// @return OK on successful completion, ISMRC_NotFound or another
///         ISMRC_ value on error.
///
/// @remark The lock covering the node under which to search is expected to be
///         held upon entry to this function. When called to potentially insert
///         a node, this must be held for write, otherwise it may be held for
///         read.
///
/// @remark The position arrays are produced by a call to iett_analyseTopicString
///         for a given string.
///
/// @see iett_analyseTopicString
//****************************************************************************
int32_t iett_insertOrFindRemSrvNode(ieutThreadData_t *pThreadData,
                                    iettRemSrvNode_t *parent,
                                    iettTopic_t *topic,
                                    int32_t operation,
                                    iettRemSrvNode_t **node)
{
    iettRemSrvNode_t  *localNode = NULL;
    int32_t            rc = OK;
    uint32_t           multicardsSeen = 0;
    uint32_t           wildcardsSeen = 0;
    const char       **substring = topic->substrings;
    const uint32_t    *substringHash = topic->substringHashes;
    const char       **wildcard = topic->wildcards;
    const char       **multicard = topic->multicards;

    assert(topic->destinationType == ismDESTINATION_TOPIC);

    // assert(NULL != wildcard);
    // assert(NULL != multicard);
    // assert(NULL != substring);
    // assert(NULL != substringHash);

    ieutTRACEL(pThreadData, topic->topicString,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "topicString='%s'\n", __func__, topic->topicString);

    do
    {
        uint32_t  nodeFlags;

        if (*substring == *wildcard)
        {
            wildcard++;
            wildcardsSeen++;
            nodeFlags = iettNODE_FLAG_WILDCARD;
            localNode = parent->wildcardChild;
        }
        else if (*substring == *multicard)
        {
            multicard++;
            multicardsSeen++;
            nodeFlags = iettNODE_FLAG_MULTICARD;
            localNode = parent->multicardChild;
        }
        else
        {
            nodeFlags = iettNODE_FLAG_NORMAL;

            if (NULL == parent->children)
            {
                localNode = NULL;
            }
            else
            {
                // assert(NULL != *substring);

                rc = ieut_getHashEntry(parent->children,
                                       *substring,
                                       *substringHash,
                                       (void **)&localNode);

                if (rc == ISMRC_NotFound)
                {
                    localNode = NULL;
                }
            }
        }

        if (NULL == localNode)
        {
            // If we were asked to look for a node then this is not an error, but the
            // caller will be interested and may treat it as an error so we don't call
            // ism_common_setError here.
            if (operation == iettOP_FIND)
            {
                rc = ISMRC_NotFound;
                goto mod_exit;
            }

            localNode = (iettRemSrvNode_t*)iemem_calloc(pThreadData,
                                                        IEMEM_PROBE(iemem_remoteServers, 9),
                                                        1, sizeof(iettRemSrvNode_t)+strlen(*substring)+1);

            if (NULL == localNode)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            memcpy(localNode->strucId, iettREMOTE_SERVER_NODE_STRUCID, 4);

            // If this node is at or below multiple multi-cards, add to it's flags
            if (multicardsSeen > 1) nodeFlags |= iettNODE_FLAG_BRANCH_MULTIMULTI;
            // If this node is at or below ANY wild-cards or multi-cards, add to it's flags
            if (wildcardsSeen != 0 || multicardsSeen != 0) nodeFlags |= iettNODE_FLAG_BRANCH_WILD_OR_MULTI;
            // If this node is at or below a system topic, add to it's flags
            if (topic->sysTopicEndIndex != 0) nodeFlags |= iettNODE_FLAG_BRANCH_SYSTOPIC;

            localNode->nodeFlags = nodeFlags;
            localNode->parent = parent;
            localNode->topicSubstring = (char *)(localNode+1);

            strcpy(localNode->topicSubstring, *substring);

            switch((nodeFlags & iettNODE_FLAG_TYPE_MASK))
            {
                case iettNODE_FLAG_WILDCARD:
                    parent->wildcardChild = localNode;
                    break;
                case iettNODE_FLAG_MULTICARD:
                    parent->multicardChild = localNode;
                    break;
                case iettNODE_FLAG_NORMAL:
                    if (NULL == parent->children)
                    {
                        rc = ieut_createHashTable(pThreadData,
                                                  iettINITIAL_REMOTE_SERVER_NODE_CAPACITY,
                                                  iemem_remoteServers,
                                                  &parent->children);

                        if (rc != OK)
                        {
                            iemem_freeStruct(pThreadData, iemem_remoteServers, localNode, localNode->strucId);
                            goto mod_exit;
                        }
                    }
                    else
                    {
                        if (parent->children->totalCount > (parent->children->capacity * (iettNODE_LOADINGFACTOR_HIGH_WATER-1)))
                        {
                            rc = ieut_resizeHashTable(pThreadData,
                                                      parent->children,
                                                      parent->children->capacity * iettNODE_CAPACITY_INCREMENT_FACTOR);

                            if (rc != OK)
                            {
                                iemem_freeStruct(pThreadData, iemem_remoteServers, localNode, localNode->strucId);
                                goto mod_exit;
                            }
                        }
                    }

                    rc = ieut_putHashEntry(pThreadData,
                                           parent->children,
                                           ieutFLAG_DUPLICATE_NONE,
                                           localNode->topicSubstring,
                                           *substringHash,
                                           localNode,
                                           0);

                    if (rc != OK)
                    {
                        iemem_freeStruct(pThreadData, iemem_remoteServers, localNode, localNode->strucId);
                        goto mod_exit;
                    }
                    break;
            }
        }

        parent = localNode;
        substring++;
        substringHash++;
    }
    while(*substring != NULL);

mod_exit:

    if (rc == OK)
    {
        *node = localNode;
    }
    else
    {
        *node = NULL;
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d, node=%p\n", __func__, rc, *node);

    return rc;
}

//****************************************************************************
/// @brief Remove the specified node from the tree.
///
/// @param[in]     node            The node on which to start processing
///
/// @remark The lock covering the tree is expected to be held for write.
///
/// @remark The node will be removed, but will not have been freed.
///         Once the lock has been released, the caller should call
///         iett_destroyRemoteServersTreeCallback to destroy it.
///
/// @see iett_destroyRemoteServersTreeCallback
//****************************************************************************
void iett_removeRemSrvNodeFromTree(ieutThreadData_t *pThreadData, iettRemSrvNode_t *node)
{
    uint32_t nodeType = node->nodeFlags & iettNODE_FLAG_TYPE_MASK;

    if (iettNODE_FLAG_WILDCARD == nodeType)
    {
        node->parent->wildcardChild = NULL;
    }
    else if (iettNODE_FLAG_MULTICARD == nodeType)
    {
        node->parent->multicardChild = NULL;
    }
    else
    {
        ieut_removeHashEntry(pThreadData,
                             node->parent->children,
                             node->topicSubstring,
                             iett_generateSubstringHash(node->topicSubstring));
    }

    node->parent = NULL;
}

//****************************************************************************
/// @brief From the specified node, traverse up and find the highest node in
///        the tree that is inactive marking all nodes in that subtree as inactive
///        and returning the node at it's start.
///
/// @param[in]     node             The node on which to start processing
/// @param[out]    inactiveSubtree  The inactive subtree that was found
///
/// @remark The lock covering the tree is expected to be held for write.
///
/// @remark If a node is returned all of the subtree it is the start of will have
///         been marked - it is expected to be removed when possible (before
///         the lock is released) and then destroyed (after the lock is released)
///         with a call to iett_destroyRemoteServersTreeCallback.
///
/// @see iett_removeRemSrvNodeFromTree
/// @see iett_destroyRemoteServersTreeCallback
//****************************************************************************
void iett_identifyInactiveRemSrvNodesFromEngineTopicTree(ieutThreadData_t *pThreadData,
                                                         iettRemSrvNode_t *node,
                                                         iettRemSrvNode_t **inactiveSubtree)
{
    iettRemSrvNode_t *foundNode = NULL;
    uint64_t allowChildren = 0;
    bool allowWildcard = false;
    bool allowMulticard = false;

    ieutTRACEL(pThreadData, node,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "node=%p\n", __func__, node);

    // While we have a node in our hands and it is not the root of the tree (the root has
    // no parent) and the node looks inactive, move our starting point up the tree.
    while (NULL != node &&
           node->parent != NULL &&
           node->activeServers.count == 0)
    {
        // Check if no child nodes left and move the foundNode up to it.
        if ((NULL == node->children || node->children->totalCount == allowChildren) &&
             (allowWildcard || NULL == node->wildcardChild) &&
             (allowMulticard || NULL == node->multicardChild))
        {
            foundNode = node;

            // From this node, decide what the parent can have and still
            // be considered inactive
            uint32_t nodeType = node->nodeFlags & iettNODE_FLAG_TYPE_MASK;

            allowChildren = (nodeType == iettNODE_FLAG_NORMAL) ? 1 : 0;
            allowWildcard = (nodeType == iettNODE_FLAG_WILDCARD);
            allowMulticard = (nodeType == iettNODE_FLAG_MULTICARD);

            node->nodeFlags |= iettNODE_FLAG_INACTIVE;

            node = node->parent;
        }
        else
        {
            assert(node->activeServers.list == NULL);
            node = NULL;
        }
    }

    *inactiveSubtree = foundNode;

    ieutTRACEL(pThreadData, foundNode,  ENGINE_FNC_TRACE, FUNCTION_EXIT "foundNode=%p\n", __func__, foundNode);
}

//****************************************************************************
/// @brief Add a remote server to a list of remote servers
///
/// @param[in]     remoteServer  Remote server being added
/// @param[in out] remSrvList    List being updated
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_addRemoteServerToList(ieutThreadData_t *pThreadData,
                                   ismEngine_RemoteServer_t *remoteServer,
                                   iettRemoteServerList_t *remSrvList)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, remoteServer,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "remoteServer=%p, remSrvList=%p\n", __func__, remoteServer, remSrvList);

    // Ensure there is space in the list for another remote server
    if (remSrvList->count == remSrvList->max)
    {
        uint32_t newMax = remSrvList->max;

        if (newMax == 0)
            newMax = iettINITIAL_REMOTE_SERVER_ARRAY_CAPACITY;
        else
            newMax *= 2;

        ismEngine_RemoteServer_t **newRemSrvList;

        // Note: We allocate one extra entry to allow for a NULL sentinel
        newRemSrvList = iemem_realloc(pThreadData,
                                      IEMEM_PROBE(iemem_remoteServers, 10),
                                      remSrvList->list,
                                      (newMax+1) * sizeof(ismEngine_RemoteServer_t *));

        if (NULL == newRemSrvList)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        remSrvList->max = newMax;
        remSrvList->list = newRemSrvList;
    }

    uint32_t                  start = 0;
    uint32_t                  end = remSrvList->count;
    ismEngine_RemoteServer_t *value = NULL;

    while(start < end)
    {
        uint32_t middle = start + ((end-start)/2);
        value = remSrvList->list[middle];

        if (value == remoteServer)
        {
            rc = ISMRC_ExistingKey;
            end = middle;
            break; // Found this remote server already in the list
        }
        else if (value > remoteServer)
        {
            end = middle;
        }
        else
        {
            start = middle + 1;
        }
    }

    if (value != remoteServer)
    {
        int32_t moveRemoteServers = remSrvList->count-end;

        if (moveRemoteServers != 0)
        {
            memmove(&remSrvList->list[end+1],
                    &remSrvList->list[end],
                    moveRemoteServers*sizeof(ismEngine_RemoteServer_t *));
        }

        remSrvList->list[end] = remoteServer;
        remSrvList->count++;
        remSrvList->list[remSrvList->count] = NULL;
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Remove a remote server from a list of remote servers
///
/// @param[in]     remoteServer  Remote server being removed
/// @param[in out] remSrvList    List being updated
///
/// @return OK on successful completion, ISMRC_NotFound if the remote server
///         is not in the list or an ISMRC_ value.
//****************************************************************************
int32_t iett_removeRemoteServerFromList(ieutThreadData_t *pThreadData,
                                        ismEngine_RemoteServer_t *remoteServer,
                                        iettRemoteServerList_t *remSrvList)
{
    int32_t   rc = ISMRC_NotFound;
    uint32_t  start = 0;
    uint32_t  middle = 0;
    uint32_t  end = remSrvList->count;

    ieutTRACEL(pThreadData, remoteServer,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "remoteServer=%p, remSrvList=%p\n", __func__, remoteServer, remSrvList);

    while(start < end)
    {
        middle = start + ((end-start)/2);

        ismEngine_RemoteServer_t *value = remSrvList->list[middle];

        if (value == remoteServer)
        {
            rc = OK;
            break;
        }
        else if (value > remoteServer)
        {
            end = middle;
        }
        else
        {
            start = middle + 1;
        }
    }

    // Move entries in the list, freeing it if no entries left, and
    // null terminating it if there are
    if (rc == OK)
    {
        // Give ourselves a local variable for the end of the list
        end = remSrvList->count;

        if (end > 1)
        {
            int32_t moveRemoteServers = end-middle;

            if (moveRemoteServers)
            {
                memmove(&remSrvList->list[middle],
                        &remSrvList->list[middle+1],
                        moveRemoteServers*sizeof(ismEngine_RemoteServer_t *));
            }
        }

        remSrvList->count--;

        if (remSrvList->count == 0)
        {
            assert(remSrvList->list != NULL);
            iemem_free(pThreadData, iemem_remoteServers, remSrvList->list);
            remSrvList->list = NULL;
            remSrvList->max = 0;
        }
        else
        {
            remSrvList->list[remSrvList->count] = NULL;
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Add a remote server to a topic in the global engine topic tree.
///
/// @param[in]     topicString   Topic to which to add remote server
/// @param[in]     remoteServer  Remote server being added
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark Assumes that the global engine topic tree has been initialised
///         by a call to iett_initEngineTopicTree.
///
/// @see iett_initEngineTopicTree
/// @see iett_removeRemoteServerFromEngineTopic
//****************************************************************************
int32_t iett_addRemoteServerToEngineTopic(ieutThreadData_t *pThreadData,
                                          const char *topicString,
                                          ismEngine_RemoteServer_t *remoteServer)
{
    int32_t rc;
    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *wildcards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *multicards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    bool locked = false;

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ieutTRACEL(pThreadData, remoteServer, ENGINE_FNC_TRACE, FUNCTION_ENTRY "topicString='%s', remoteServer=%p\n", __func__,
               topicString, remoteServer);

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = topicString;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

    rc = iett_analyseTopicString(pThreadData, &topic);

    if (rc != OK) goto mod_exit;

    // Adding a remote server requires write access to the remoteserver tree
    ismEngine_getRWLockForWrite(&tree->remoteServersLock);
    locked = true;

    iettRemSrvNode_t *remSrvNode = NULL;

    rc = iett_insertOrFindRemSrvNode(pThreadData,
                                     tree->remoteServers,
                                     &topic,
                                     iettOP_ADD,
                                     &remSrvNode);

    if (rc == OK)
    {
        // Add the new remote server to the list of active remote servers at this node.
        rc = iett_addRemoteServerToList(pThreadData,
                                        remoteServer,
                                        &remSrvNode->activeServers);

        if (rc == OK)
        {
            // Increment the count of multiple multi-level wildcard remote servers if appropriate
            if (remSrvNode->nodeFlags & iettNODE_FLAG_BRANCH_MULTIMULTI)
            {
                tree->multiMultiRemSrvs++;
            }

            // TODO: Do we need a tree->remSrvUpdates? like tree->subsUpdates++;
        }
        // An attempt to re-add the same server should be ignored.
        else if (rc == ISMRC_ExistingKey)
        {
            rc = OK;
        }
    }

mod_exit:

    // Release the remote server tree lock if we are holding it.
    if (locked) ismEngine_unlockRWLock(&tree->remoteServersLock);

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
/// @brief Remove a remote server from a topic in the global engine topic tree
///
/// @param[in]     topicString   Topic from which to remove remote server
/// @param[in]     remoteServer  Remote server being removed
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark Assumes that the global engine topic tree has been initialised
///         by a call to iett_initEngineTopicTree.
///
/// @see iett_initEngineTopicTree
/// @see iett_addRemoteServerToEngineTopic
//****************************************************************************
int32_t iett_removeRemoteServerFromEngineTopic(ieutThreadData_t *pThreadData,
                                               const char *topicString,
                                               ismEngine_RemoteServer_t *remoteServer)
{
    int32_t rc;
    iettRemSrvNode_t *inactiveSubtree = NULL;
    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *wildcards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *multicards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    bool locked = false;

    ieutTRACEL(pThreadData, remoteServer, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "topicString='%s', remoteServer=%p\n", __func__,
               topicString, remoteServer);

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = topicString;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

    rc = iett_analyseTopicString(pThreadData, &topic);

    if (rc != OK) goto mod_exit;

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    // Removing a remote server requires write access to the remote servers tree
    ismEngine_getRWLockForWrite(&tree->remoteServersLock);
    locked = true;

    iettRemSrvNode_t *remSrvNode = NULL;

    rc = iett_insertOrFindRemSrvNode(pThreadData,
                                     tree->remoteServers,
                                     &topic,
                                     iettOP_FIND,
                                     &remSrvNode);

    if (NULL != remSrvNode)
    {
        if (remSrvNode->activeServers.count != 0)
        {
            // Remove the remote server from the list of active remote servers at this node.
            rc = iett_removeRemoteServerFromList(pThreadData,
                                                 remoteServer,
                                                 &remSrvNode->activeServers);
        }

        if (rc == OK)
        {
            // Decrement the count of multiple multi-level wildcard remote servers if appropriate
            if (remSrvNode->nodeFlags & iettNODE_FLAG_BRANCH_MULTIMULTI)
            {
                tree->multiMultiRemSrvs--;
            }

            // TODO: Do we need a tree->remSrvUpdates? like tree->subsUpdates++;

            // If this node is inactive, clear up whatever we now can.
            if (remSrvNode->activeServers.count == 0)
            {
                // Identify any nodes that can now be considered inactive from here up
                iett_identifyInactiveRemSrvNodesFromEngineTopicTree(pThreadData,
                                                                    remSrvNode,
                                                                    &inactiveSubtree);

                // We can remove inactive nodes immediately, invalidate cached node lists
                if (inactiveSubtree != NULL)
                {
                    iett_removeRemSrvNodeFromTree(pThreadData, inactiveSubtree);

                    // TODO: Work out the caching
                }
            }
        }
    }

mod_exit:

    if (locked) ismEngine_unlockRWLock(&tree->remoteServersLock);

    // Free any topic string analysis data
    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
        if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
        if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);
    }

    // If a subtree was removed from the tree, destroy it now.
    if (inactiveSubtree != NULL)
    {
        iett_destroyRemoteServersTreeCallback(pThreadData, NULL, 0, inactiveSubtree, NULL);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Context for iett_purgeRemSrvFromTreeNode
///
/// @see iett_purgeRemSrvFromTreeNode
//****************************************************************************
typedef struct tag_iettPurgeRemSrvTreeCbContext_t
{
    ismEngine_RemoteServer_t *remoteServer;
    iettRemSrvNode_t **inactiveSubtrees;
    uint32_t inactiveSubtreeCount;
    uint32_t inactiveSubtreeMax;
    iettTopicTree_t *tree;
    int32_t rc;
    bool print;
} iettPurgeRemSrvTreeCbContext_t;

//****************************************************************************
// Forward declaration of iett_purgeRemSrvFromTreeNode
//****************************************************************************
void iett_purgeRemSrvFromTreeNode(ieutThreadData_t *pThreadData,
                                  iettRemSrvNode_t *node,
                                  iettPurgeRemSrvTreeCbContext_t *context);

//****************************************************************************
/// @brief Callback used when purging a remote server from a remote server tree
///
/// @param[in]     key      Key being processed (topic substring)
/// @param[in]     keyHash  Hash for the key being processed
/// @param[in]     value    Value from the hash table (iettRemSrvNode_t)
/// @param[in,out] context  Context information
///
/// @see iett_purgeRemoteServerFromEngineTopicTree
//****************************************************************************
void iett_purgeRemSrvTreeCallback(ieutThreadData_t *pThreadData,
                                  char *key,
                                  uint32_t keyHash,
                                  void *value,
                                  void *context)
{
    iett_purgeRemSrvFromTreeNode(pThreadData,
                                 (iettRemSrvNode_t *)value,
                                 (iettPurgeRemSrvTreeCbContext_t *)context);
}

//****************************************************************************
/// @brief Remove the specified remote server from this node if it exists
///
/// @param[in]     node     Node from which to begin printing.
/// @param[in,out] context  Context of the purge request
///
/// @see iett_purgeRemoteServerFromEngineTopicTree
//****************************************************************************
void iett_purgeRemSrvFromTreeNode(ieutThreadData_t *pThreadData,
                                  iettRemSrvNode_t *node,
                                  iettPurgeRemSrvTreeCbContext_t *context)
{
    if (context->rc == OK && (node->nodeFlags & iettNODE_FLAG_INACTIVE) == 0)
    {
        int32_t rc = ISMRC_NotFound;

        if (node->activeServers.count != 0)
        {
            // Remove the remote server from the list of active remote servers at this node.
            rc = iett_removeRemoteServerFromList(pThreadData,
                                                 context->remoteServer,
                                                 &node->activeServers);
        }

        if (rc == OK)
        {
            // Decrement the count of multiple multi-level wildcard remote servers if appropriate
            if (node->nodeFlags & iettNODE_FLAG_BRANCH_MULTIMULTI)
            {
                context->tree->multiMultiRemSrvs--;
            }

            // If this node is inactive, clear up whatever we now can.
            if (node->activeServers.count == 0)
            {
                iettRemSrvNode_t *inactiveSubtree = NULL;

                // Identify any nodes that can now be considered inactive from here up
                iett_identifyInactiveRemSrvNodesFromEngineTopicTree(pThreadData,
                                                                    node,
                                                                    &inactiveSubtree);

                // Add this subtree to our list of subtrees to clean up
                if (inactiveSubtree != NULL)
                {
                    if (context->inactiveSubtreeCount == context->inactiveSubtreeMax)
                    {
                        uint32_t newMax = context->inactiveSubtreeMax + 10;
                        iettRemSrvNode_t **newRemovedSubtrees = iemem_realloc(pThreadData,
                                                                              IEMEM_PROBE(iemem_remoteServers, 11),
                                                                              context->inactiveSubtrees,
                                                                              sizeof(iettRemSrvNode_t *) * newMax);

                        if (newRemovedSubtrees == NULL)
                        {
                            context->rc = ISMRC_AllocateError;
                            ism_common_setError(context->rc);
                            goto mod_exit;
                        }

                        context->inactiveSubtrees = newRemovedSubtrees;
                        context->inactiveSubtreeMax = newMax;
                    }

                    context->inactiveSubtrees[context->inactiveSubtreeCount++] = inactiveSubtree;
                }
            }
        }
        else
        {
            assert(rc == ISMRC_NotFound);
        }

        if (node->children)
        {
            ieut_traverseHashTable(pThreadData,
                                   node->children,
                                   iett_purgeRemSrvTreeCallback,
                                   context);
        }

        if (node->wildcardChild)
        {
            iett_purgeRemSrvFromTreeNode(pThreadData, node->wildcardChild, context);
        }

        if (node->multicardChild)
        {
            iett_purgeRemSrvFromTreeNode(pThreadData, node->multicardChild, context);
        }
    }

    context->print=false;

mod_exit:

    return;
}

//****************************************************************************
/// @brief Remove all references to a remote server from the global engine
///        topic tree
///
/// @param[in]     remoteServer  Remote server being removed
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark Assumes that the global engine topic tree has been initialised
///         by a call to iett_initEngineTopicTree.
///
/// @see iett_initEngineTopicTree
//****************************************************************************
int32_t iett_purgeRemoteServerFromEngineTopicTree(ieutThreadData_t *pThreadData,
                                                  ismEngine_RemoteServer_t *remoteServer)
{
    ieutTRACEL(pThreadData, remoteServer, ENGINE_FNC_TRACE, FUNCTION_ENTRY "remoteServer=%p\n", __func__, remoteServer);

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    // Need the write lock to remove entries from the tree
    ismEngine_getRWLockForWrite(&tree->remoteServersLock);

    iettPurgeRemSrvTreeCbContext_t context = { remoteServer, NULL, 0, 0, tree, OK , false};

    // Traverse the tree, removing this remote server from any nodes that have it
    iett_purgeRemSrvFromTreeNode(pThreadData, tree->remoteServers, &context);

    // We identified inactive subtrees, remove them from the tree while we have it locked
    // and invalidate cached node lists
    if (context.inactiveSubtreeCount != 0)
    {
        assert(context.inactiveSubtrees != NULL);

        for(uint32_t index=0; index<context.inactiveSubtreeCount; index++)
        {
            iettRemSrvNode_t *inactiveSubtree = context.inactiveSubtrees[index];

            iett_removeRemSrvNodeFromTree(pThreadData, inactiveSubtree);
        }

        // TODO: Work out the caching
    }

    ismEngine_unlockRWLock(&tree->remoteServersLock);

    // Destroy the subtrees that have been removed, now we've dropped the lock
    if (context.inactiveSubtreeCount != 0)
    {
        for(uint32_t index=0; index<context.inactiveSubtreeCount; index++)
        {
            iettRemSrvNode_t *removedSubtree = context.inactiveSubtrees[index];

            iett_destroyRemoteServersTreeCallback(pThreadData, NULL, 0, removedSubtree, NULL);
        }

        iemem_free(pThreadData, iemem_remoteServers, context.inactiveSubtrees);
    }

    ieutTRACEL(pThreadData, context.rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, context.rc);

    return context.rc;
}

//****************************************************************************
/// @brief Append all nodes containing active remote servers from the given node
///        including multi-level and wildcard child nodes.
///
/// @param[in]     node             Node
/// @param[in,out] maxNodes         Space available in the array of topic nodes
/// @param[in,out] nodeCount        Count of nodes containing remote servers found
/// @param[in]     result           Current array of nodes
///
/// @return Pointer to the array of nodes (re-allocated if required) containing
///         the newly appended nodes, NULL if a problem was encountered.
///
/// @remark The lock covering the nodes being added must be held for read.
//****************************************************************************
int32_t iett_addActiveRemSrvNodes(ieutThreadData_t *pThreadData,
                                  iettRemSrvNode_t *node,
                                  uint32_t *maxNodes,
                                  uint32_t *nodeCount,
                                  iettRemSrvNode_t  ***result)
{
    int32_t          rc = OK;
    uint32_t         entryNodeType = node->nodeFlags & iettNODE_FLAG_TYPE_MASK;

    while(NULL != node)
    {
        // Include nodes that have active remote servers
        if (node->activeServers.count != 0)
        {
            if (*nodeCount == *maxNodes)
            {
                uint32_t newMax = (*maxNodes) + iettFIND_REMOTE_SERVER_RESULTS_INCREMENT;

                iettRemSrvNode_t **newResult;

                newResult = iemem_realloc(pThreadData,
                                          IEMEM_PROBE(iemem_subsQuery, 7),
                                          *result,
                                          newMax * sizeof(iettRemSrvNode_t *));

                if (NULL == newResult)
                {
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    goto mod_exit;
                }

                *maxNodes = newMax;
                *result = newResult;
            }

            (*result)[(*nodeCount)++] = node;
        }

        // For non-multicard nodes on entry, collect any multicard remote servers lower
        // down in the topic tree (A/B should match /A/B/#, /A/B/#/# etc).
        if (entryNodeType == iettNODE_FLAG_MULTICARD)
        {
            node = NULL;
        }
        else
        {
            node = node->multicardChild;
        }
    }

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief Find the nodes in a remote server tree which match a given topic
///        string appending them to an array of nodes.
///
/// @param[in]     parent            Parent node under which to start searching.
/// @param[in]     topic             Topic analysis to use
/// @param[in]     curIndex          Current index into the topic analysis arrays
/// @param[in,out] maxNodes          Space available in the array of topic nodes
/// @param[out]    nodeCount         Count of nodes containing remote servers found.
/// @param[in]     result            Current array of nodes
///
/// @return OK if or another ISMRC_ value on error.
///
/// @return Pointer to the array of nodes containing remote servers, NULL if a
////        problem was encountered.
///
/// @remark The lock covering the nodes being added must be held for read.
///
/// @remark This function calls itself recursively for child nodes.
///
/// @see iett_addRemoteServersToSubscriberList
//****************************************************************************
int32_t iett_findMatchingRemSrvNodes(ieutThreadData_t *pThreadData,
                                     const iettRemSrvNode_t *parent,
                                     const iettTopic_t *topic,
                                     const uint32_t curIndex,
                                     uint32_t *maxNodes,
                                     uint32_t *nodeCount,
                                     iettRemSrvNode_t ***result)
{
    int32_t rc = OK;
    iettRemSrvNode_t *node = NULL;
    uint32_t index;

    if (curIndex == 0)
    {
        assert(topic->destinationType == ismDESTINATION_TOPIC);

        ieutTRACEL(pThreadData, topic->topicString,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "topicString='%s'\n", __func__, topic->topicString);
    }

    // Look through the children of this node.
    if (NULL != parent->children)
    {
        (void)ieut_getHashEntry(parent->children,
                                topic->substrings[curIndex],
                                topic->substringHashes[curIndex],
                                (void **)&node);

        if (NULL != node)
        {
            index = curIndex+1;

            if (index == topic->substringCount)
            {
                rc = iett_addActiveRemSrvNodes(pThreadData, node, maxNodes, nodeCount, result);
            }
            else
            {
                rc = iett_findMatchingRemSrvNodes(pThreadData,
                                                  node,
                                                  topic,
                                                  index,
                                                  maxNodes, nodeCount, result);
            }

            if (rc != OK) goto mod_exit;
        }
    }

    // We will only follow wild cards (# or +) at (or after) the constant string end index
    if (curIndex >= topic->sysTopicEndIndex)
    {
        // Look for single-level wildcards.
        if (NULL != parent->wildcardChild)
        {
            index = curIndex+1;

            node = parent->wildcardChild;

            if (index == topic->substringCount)
            {
                rc = iett_addActiveRemSrvNodes(pThreadData, node, maxNodes, nodeCount, result);
            }
            else
            {
                rc = iett_findMatchingRemSrvNodes(pThreadData,
                                                  node,
                                                  topic,
                                                  index,
                                                  maxNodes, nodeCount, result);
            }

            if (rc != OK) goto mod_exit;
        }

        // Look for multi-level wildcards (multicards).
        if (NULL != parent->multicardChild)
        {
            index = curIndex;

            node = parent->multicardChild;

            rc = iett_addActiveRemSrvNodes(pThreadData, node, maxNodes, nodeCount, result);

            if (rc != OK) goto mod_exit;

            do
            {
                rc = iett_findMatchingRemSrvNodes(pThreadData,
                                                  node,
                                                  topic,
                                                  index++,
                                                  maxNodes, nodeCount, result);

                if (rc != OK) goto mod_exit;
            }
            while (index < topic->substringCount);
        }
    }

mod_exit:

    if (curIndex == 0 && NULL != *result)
    {
        if (rc != OK)
        {
            iemem_free(pThreadData, iemem_subsQuery, *result);
            *result = NULL;
            *nodeCount = 0;
        }
        // Remove any duplicates caused by wild card processing.
        else
        {
            uint32_t finalCount = *nodeCount;
            iettRemSrvNode_t **finalResult;

            // Ensure remoteServerNodes array is NULL terminated.
            if (*maxNodes == finalCount)
            {
                finalResult = iemem_realloc(pThreadData,
                                            IEMEM_PROBE(iemem_subsQuery, 8),
                                            *result,
                                            (finalCount+1)*sizeof(iettSubsNode_t *));

                if (NULL == finalResult)
                {
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    iemem_free(pThreadData, iemem_subsQuery, *result);
                    *result = NULL;
                    *nodeCount = 0;
                }
                else
                {
                    *result = finalResult;
                }
            }
            else
            {
                finalResult = *result;
            }

            if (NULL != finalResult)
            {
                finalResult[finalCount] = NULL;

                // If there are no remote servers in the topic tree which specify multiple multi-level
                // wildcards, then there will be no duplicates in the results - otherwise there may
                // be and we should remove them.
                if (ismEngine_serverGlobal.maintree->multiMultiRemSrvs > 0)
                {
                    uint32_t nodeIndex = 0;
                    iettRemSrvNode_t *possibleDuplicate;

                    finalCount = 0;
                    while(NULL != (node = finalResult[nodeIndex]))
                    {
                        if (node != iettINVALID_ADDRESS)
                        {
                            finalResult[nodeIndex] = iettINVALID_ADDRESS;
                            finalResult[finalCount++] = node;

                            while(NULL != (possibleDuplicate = finalResult[++nodeIndex]))
                            {
                                if (node == possibleDuplicate)
                                {
                                    finalResult[nodeIndex] = iettINVALID_ADDRESS;
                                }
                            }

                            nodeIndex = finalCount;
                        }
                        else
                        {
                            nodeIndex++;
                        }
                    }

                    finalResult[finalCount] = NULL;
                    *nodeCount = finalCount;
                }
            }
        }
    }

    if (curIndex == 0)
    {
        ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d, *nodeCount=%d\n", __func__, rc, *nodeCount);
    }

    return rc;
}

//****************************************************************************
/// @brief Add an array of remote servers with an interest in a given topic
///        from the the global engine topic tree.
///
/// @param[in]     topic           Pre-analysed topic string
/// @param[in,out] subscriberList  Pointer to a list to be added to.
///
/// @return OK if remote servers found, ISMRC_NotFound if not or another
///         ISMRC_ value on error.
///
/// @remark The subscriberList must be pre-filled with a topic string.
///
/// @remark The topic specified is assumed to contain no wildcards.
///
/// @remark Upon OK return the remote servers in the list have had their use
///         count incremented, meaning that they will not be removed while the
///         operation is being performed.
///
///         As a result once the caller no longer needs to refer to the list, it
///         must be released by either a call to to iett_releaseSubscriberList or
///         a pair of calls, one to iett_SLEReplayReleaseNodes and one to to
///         iett_freeSubscriberList.
///
/// @see iett_releaseSubscriberList iett_SLEReplayReleaseNodes iett_freeSubscriberList
//****************************************************************************
int32_t iett_addRemoteServersToSubscriberList(ieutThreadData_t *pThreadData,
                                              iettTopic_t *topic,
                                              iettSubscriberList_t *subscriberList)
{
    int32_t                    rc = OK;
    iettTopicTree_t           *tree;
    iettRemSrvNode_t         **remSrvNodes = NULL;

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_ENTRY "topicString='%s'\n", __func__,
               topic->topicString);

    tree = ismEngine_serverGlobal.maintree;

    assert(subscriberList->remoteServerCount == 0);

    // We do not accept wildcards on the topic specified for a
    // subscriber list at the moment.
    assert(topic->topicStringCopy != NULL);
    assert((topic->wildcardCount == 0) && (topic->multicardCount == 0));

    // Get the remote servers tree lock for read
    ismEngine_getRWLockForRead(&tree->remoteServersLock);

    uint32_t remSrvNodeCount = 0;
    uint32_t remSrvNodeCapacity = 0;

    // Use iett_findMatchingRemSrvNodes to walk the tree.
    rc = iett_findMatchingRemSrvNodes(pThreadData,
                                      tree->remoteServers,
                                      topic,
                                      0,
                                      &remSrvNodeCapacity,
                                      &remSrvNodeCount,
                                      &remSrvNodes);

    // If we have nodes we need to extract the remote servers from them
    if (remSrvNodeCount != 0)
    {
        assert(remSrvNodes != NULL);

        uint32_t maxCapacityNeeded = 0;
        iettRemSrvNode_t **remSrvNodePos = remSrvNodes;

        do
        {
            iettRemSrvNode_t *remSrvNode = *remSrvNodePos;
            maxCapacityNeeded += remSrvNode->activeServers.count;
        }
        while(*(++remSrvNodePos) != NULL);

        assert(maxCapacityNeeded != 0);

        if (maxCapacityNeeded != 0)
        {
            subscriberList->remoteServers = iemem_realloc(pThreadData,
                                                          IEMEM_PROBE(iemem_subsQuery, 9),
                                                          subscriberList->remoteServers,
                                                          (maxCapacityNeeded+1) * sizeof(ismEngine_RemoteServer_t *));

            if (NULL == subscriberList->remoteServers)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            subscriberList->remoteServerCapacity = maxCapacityNeeded;

            remSrvNodePos = remSrvNodes;
            ismEngine_RemoteServer_t **remoteServerPos = subscriberList->remoteServers;
            do
            {
                iettRemSrvNode_t *remSrvNode = *remSrvNodePos;
                if (remSrvNode->activeServers.count != 0)
                {
                    memcpy(remoteServerPos,
                           remSrvNode->activeServers.list,
                           remSrvNode->activeServers.count * sizeof(ismEngine_RemoteServer_t *));
                    remoteServerPos += remSrvNode->activeServers.count;
                }
            }
            while(*(++remSrvNodePos) != NULL);
            *remoteServerPos = NULL;

            // Deduplicate the remote servers list
            uint32_t nodeIndex = 0;
            uint32_t serverCount = 0;

            ismEngine_RemoteServer_t *remoteServer;
            ismEngine_RemoteServer_t *possibleDuplicate;

            while(NULL != (remoteServer = subscriberList->remoteServers[nodeIndex]))
            {
                if (remoteServer != iettINVALID_ADDRESS)
                {
                    subscriberList->remoteServers[nodeIndex] = iettINVALID_ADDRESS;
                    subscriberList->remoteServers[serverCount++] = remoteServer;

                    // Increment the use count on this server
                    __sync_fetch_and_add(&remoteServer->useCount, 1);

                    while(NULL != (possibleDuplicate = subscriberList->remoteServers[++nodeIndex]))
                    {
                        if (remoteServer == possibleDuplicate)
                        {
                            subscriberList->remoteServers[nodeIndex] = iettINVALID_ADDRESS;
                        }
                    }

                    nodeIndex = serverCount;
                }
                else
                {
                    nodeIndex++;
                }
            }
            subscriberList->remoteServers[serverCount] = NULL;
            subscriberList->remoteServerCount = serverCount;

            assert(subscriberList->remoteServerCount <= subscriberList->remoteServerCapacity);
        }
    }

mod_exit:

    // Free up the list of remote server nodes if we have one
    if (remSrvNodes != NULL) iemem_free(pThreadData, iemem_subsQuery, remSrvNodes);

    // Release the remote server tree lock
    ismEngine_unlockRWLock(&tree->remoteServersLock);

    if (rc == OK && subscriberList->remoteServerCount == 0)
    {
        rc = ISMRC_NotFound;  // None found - not an error
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, remoteServerCount=%d\n", __func__, rc, subscriberList->remoteServerCount);

    return rc;
}

//****************************************************************************
/// @brief Clean up / free the content of a remote server tree
///
/// @param[in]     key      Key of the hash table entry being passed
/// @param[in]     keyHash  Hash of the key
/// @param[in]     value    Value of the hash entry (Node at the top of the subtree)
/// @param[in]     context  The context passed to the callback routine
///
/// @remark We use this to avoid lots of additional levels when recursing,
///         when first called the key and keyHash are not valid.
//****************************************************************************
void iett_destroyRemoteServersTreeCallback(ieutThreadData_t *pThreadData,
                                           char *key,
                                           uint32_t keyHash,
                                           void *value,
                                           void *context)
{
    iettRemSrvNode_t *node = (iettRemSrvNode_t *)value;

    if (node->children)
    {
        ieut_traverseHashTable(pThreadData,
                               node->children,
                               &iett_destroyRemoteServersTreeCallback,
                               context);
        ieut_destroyHashTable(pThreadData, node->children);
    }

    // Free the list
    if (node->activeServers.list)
    {
        iemem_free(pThreadData, iemem_remoteServers, node->activeServers.list);
    }

    if (node->wildcardChild) iett_destroyRemoteServersTreeCallback(pThreadData,
                                                                   key,
                                                                   keyHash,
                                                                   node->wildcardChild,
                                                                   context);
    if (node->multicardChild) iett_destroyRemoteServersTreeCallback(pThreadData,
                                                                    key,
                                                                    keyHash,
                                                                    node->multicardChild,
                                                                    context);

    iemem_freeStruct(pThreadData, iemem_remoteServers, node, node->strucId);
}

//****************************************************************************
/// @brief Find an origin server record for the specified serverUID.
///        Optionally, if an existing record is not found, insert one and
///        return the newly inserted record.
///
/// @param[in]     serverUID      The UID of the server to find
/// @param[in]     operation      The operation being performed requiring
///                               this record.
/// @param[out]    originServer   A pointer to the record pointer being returned
///
/// @return OK on successful completion, ISMRC_NotFound or another
///         ISMRC_ value on error.
///
/// @remark The topics tree lock is expected to be held upon entry to this function.
///         When called to potentially insert a node, this must be held for write,
///         otherwise it may be held for read.
//****************************************************************************
int32_t iett_insertOrFindOriginServer(ieutThreadData_t *pThreadData,
                                      const char *serverUID,
                                      int32_t operation,
                                      iettOriginServer_t **originServer)
{
    iettOriginServer_t *localRecord = NULL;
    int32_t             rc = OK;

    assert(serverUID != NULL);

    ieutTRACEL(pThreadData, serverUID, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "serverUID='%s'\n", __func__, serverUID);

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    uint32_t serverUIDHash = iett_generateOriginServerHash(serverUID);

    // Get the list of named subscriptions for this client.
    rc = ieut_getHashEntry(tree->originServers,
                           serverUID,
                           serverUIDHash,
                           (void **)&localRecord);

    if (rc == ISMRC_NotFound)
    {
        // Not allowing creation, so return this error
        if (operation == iettOP_FIND)
        {
            ism_common_setError(rc);
            goto mod_exit;
        }

        // Allocate a new structure
        localRecord = iemem_calloc(pThreadData,
                                   IEMEM_PROBE(iemem_remoteServers, 17), 1,
                                   sizeof(iettOriginServer_t)+strlen(serverUID)+1);

        if (localRecord == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        // Initialize the new record
        memcpy(localRecord->strucId, iettORIGIN_SERVER_STRUCID, 4);
        localRecord->serverUID = (char *)(localRecord+1);
        localRecord->stats.version = iettORIGIN_SERVER_STATS_VERSION_1;
        strcpy(localRecord->serverUID, serverUID);
        assert(ism_common_getServerUID() != NULL);
        if (strcmp(serverUID, ism_common_getServerUID()) == 0)
        {
            localRecord->localServer = true;
        }

        // Add the new record to the hash table
        rc = ieut_putHashEntry(pThreadData,
                               tree->originServers,
                               ieutFLAG_DUPLICATE_NONE,
                               localRecord->serverUID,
                               serverUIDHash,
                               localRecord,
                               0);

        if (rc != OK)
        {
            ism_common_setError(rc);
            iemem_freeStruct(pThreadData, iemem_remoteServers, localRecord, localRecord->strucId);
            goto mod_exit;
        }
    }

    assert(rc == OK);

    // Return the record
    *originServer = localRecord;

mod_exit:

    ieutTRACEL(pThreadData, localRecord, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "originServer=%p, rc=%d\n", __func__, localRecord, rc);

    return rc;
}

//****************************************************************************
/// @brief Increment the statistics for the origin server.
///
/// @param[in]     originServer   The origin server to add the node to increment
/// @param[out]    topicNode      The node to add to the origin server
///
/// @return OK on successful completion, ISMRC_NotFound or another
///         ISMRC_ value on error.
///
/// @remark The topics tree lock is expected to be held for write.
//****************************************************************************
static inline void iett_incrementOriginServerStats(iettOriginServer_t *originServer, iettTopicNode_t *topicNode)
{
    // We don't include System topics in external stats
    bool updateExternalStats = (topicNode->nodeFlags & iettNODE_FLAG_LOCAL_ORIGIN_STATS_ONLY_MASK) == 0;

    if (updateExternalStats)
    {
        originServer->stats.count++;

        // TODO: Update more stats?

        if (topicNode->originNext == NULL)
        {
            originServer->stats.highestTimestampAvailable = topicNode->currRetTimestamp;

            if (topicNode->currRetTimestamp > originServer->stats.highestTimestampSeen)
            {
                originServer->stats.highestTimestampSeen = topicNode->currRetTimestamp;
            }
        }
    }

    originServer->stats.localCount++;
}

//****************************************************************************
/// @brief Add this topic node to the list of topic nodes which contain a
///        retained messages from the specified originating server.
///
/// @param[in]     topicNode      The node to add to the origin server
/// @param[out]    originServer   The origin server to add the node to
///
/// @return OK on successful completion, ISMRC_NotFound or another
///         ISMRC_ value on error.
///
/// @remark The topics tree lock is expected to be held for write.
//****************************************************************************
void iett_addTopicNodeToOriginServer(ieutThreadData_t *pThreadData,
                                     iettTopicNode_t *topicNode,
                                     iettOriginServer_t *originServer)
{
    ieutTRACEL(pThreadData, topicNode, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "topicNode=%p originServer=%p\n",
               __func__, topicNode, originServer);

    assert(originServer != NULL);
    assert(topicNode->currRetMessage != NULL);
    assert(topicNode->originNext == NULL);
    assert(topicNode->originPrev == NULL);

    iettTopicNode_t *curr = originServer->lastAdded;

    bool followPrev;

    if (curr == NULL)
    {
        curr = originServer->tail;
        followPrev = true;
    }
    else
    {
        followPrev = (curr->currRetTimestamp > topicNode->currRetTimestamp);
    }

    // Walking down the Prev pointers
    if (followPrev)
    {
        while((curr != NULL) && (curr->currRetTimestamp > topicNode->currRetTimestamp))
        {
            curr = curr->originPrev;
        }

        topicNode->originPrev = curr;

        if (curr == NULL)
        {
            topicNode->originNext = originServer->head;
            originServer->head = topicNode;
        }
        else
        {
            topicNode->originNext = curr->originNext;
            curr->originNext = topicNode;
        }

        if (originServer->tail == curr)
        {
            originServer->tail = topicNode;
        }

        if (topicNode->originNext != NULL)
        {
            topicNode->originNext->originPrev = topicNode;
        }
    }
    // Walking up the Next pointers
    else
    {
        while((curr != NULL) && (curr->currRetTimestamp < topicNode->currRetTimestamp))
        {
            curr = curr->originNext;
        }

        topicNode->originNext = curr;

        if (curr == NULL)
        {
            topicNode->originPrev = originServer->tail;
            originServer->tail = topicNode;
        }
        else
        {
            topicNode->originPrev = curr->originPrev;
            curr->originPrev = topicNode;
        }

        if (originServer->head == curr)
        {
            originServer->head = topicNode;
        }

        if (topicNode->originPrev != NULL)
        {
            topicNode->originPrev->originNext = topicNode;
        }
    }

    iett_incrementOriginServerStats(originServer, topicNode);

    topicNode->currOriginServer = originServer;
    originServer->lastAdded = topicNode;

    ieutTRACEL(pThreadData, originServer, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "topicNode=%p originServer=%p\n",
               __func__, topicNode, originServer);
}

//****************************************************************************
/// @brief Add this topic node to the array of topic nodes with retained messages
///        from the specified originating server that is built during recovery.
///
/// @param[in]     topicNode      The node to add to the origin server
/// @param[out]    originServer   The origin server to add the node to
///
/// @return OK on successful completion or an ISMRC_ value on error.
///
/// @remark The topics tree lock is expected to be held for write.
//****************************************************************************
int32_t iett_addTopicNodeToOriginServerRecovery(ieutThreadData_t *pThreadData,
                                                iettTopicNode_t *topicNode,
                                                iettOriginServer_t *originServer)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, topicNode, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "topicNode=%p originServer=%p\n",
               __func__, topicNode, originServer);

    assert(originServer != NULL);
    assert(topicNode->currRetMessage != NULL);
    assert(topicNode->originNext == NULL);
    assert(topicNode->originPrev == NULL);

    // During recovery the localCount stat is used to record the number of retained messages
    // we have added to the 'recovered' array for the originServer, and the count stat is used
    // to record the size of that array -- the stats get fixed up to the expcted values during
    // reconciliation, before we publish the values to the cluster.
    if (originServer->stats.localCount == originServer->stats.count)
    {
        uint32_t newCount = originServer->stats.count;

        if (newCount == 0)
        {
            newCount = 10000;
        }
        else
        {
            newCount = newCount * 2;
        }

        iettTopicNode_t **newRecovered = iemem_realloc(pThreadData,
                                                       IEMEM_PROBE(iemem_unneededRetainedMsgs, 2),
                                                       originServer->recovered, newCount * sizeof(iettTopicNode_t *));

        if (newRecovered == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        originServer->recovered = newRecovered;
        originServer->stats.count = newCount;
    }

    originServer->recovered[originServer->stats.localCount] = topicNode;
    originServer->stats.localCount += 1;

    topicNode->currOriginServer = originServer;

    // Squirrel away the array index in 'originNext' so that if we need to remove it during recovery
    // (because another replaces it) we can quickly locate it in the array (note it is actually
    // position+1 so we can assert it is not 0 when removing).
    topicNode->originNext = (iettTopicNode_t *)(uint64_t)(originServer->stats.localCount);

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "topicNode=%p originServer=%p rc=%d\n",
               __func__, topicNode, originServer, rc);

    return rc;
}

//****************************************************************************
/// @brief Remove this topic node from the list of topic nodes which
///        contain retained messages from the specified originating server.
///
/// @param[in]     topicNode      The node to remove from the origin server
/// @param[out]    originServer   The origin server to remove the node from
///
/// @return OK on successful completion, ISMRC_NotFound or another
///         ISMRC_ value on error.
///
/// @remark The topics tree lock is expected to be held for write.
//****************************************************************************
void iett_removeTopicNodeFromOriginServer(ieutThreadData_t *pThreadData,
                                          iettTopicNode_t *topicNode,
                                          iettOriginServer_t *originServer)
{
    ieutTRACEL(pThreadData, topicNode, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "topicNode=%p originServer=%p\n",
               __func__, topicNode, originServer);

    assert(originServer != NULL);

    // We actually only ever expect to be called for the one the topicnode is on
    assert(topicNode->currOriginServer == originServer);

    // We don't include System topics in external stats
    bool updateExternalStats = (topicNode->nodeFlags & iettNODE_FLAG_LOCAL_ORIGIN_STATS_ONLY_MASK) == 0;

    // Removing the First
    if (topicNode->originPrev == NULL)
    {
        originServer->head = topicNode->originNext;
    }
    else
    {
        topicNode->originPrev->originNext = topicNode->originNext;
    }

    // Removing the last
    if (topicNode->originNext == NULL)
    {
        originServer->tail = topicNode->originPrev;

        if (updateExternalStats)
        {
            // Update the external stats with the stats from the previous externalised one
            iettTopicNode_t *prevExternalNode = topicNode->originPrev;

            while((prevExternalNode != NULL) &&
                  ((prevExternalNode->nodeFlags & iettNODE_FLAG_LOCAL_ORIGIN_STATS_ONLY_MASK) != 0))
            {
                prevExternalNode = prevExternalNode->originPrev;
            }

            if (prevExternalNode != NULL)
            {
                originServer->stats.highestTimestampAvailable = prevExternalNode->currRetTimestamp;
            }
            else
            {
                originServer->stats.highestTimestampAvailable = 0;
            }
        }
    }
    else
    {
        topicNode->originNext->originPrev = topicNode->originPrev;
    }

    originServer->stats.localCount--;

    if (updateExternalStats)
    {
        // Update statistics
        originServer->stats.count--;

        // TODO: Update more stats?
    }

    if (originServer->lastAdded == topicNode) originServer->lastAdded = NULL;

    topicNode->currOriginServer = NULL;
    topicNode->originNext = NULL;
    topicNode->originPrev = NULL;

    ieutTRACEL(pThreadData, originServer, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "topicNode=%p originServer=%p\n",
               __func__, topicNode, originServer);
}

//****************************************************************************
/// @brief Remove this topic node from the array of topic nodes being built
/// during recovery.
///
/// @param[in]     topicNode      The node to remove from the origin server
/// @param[out]    originServer   The origin server to remove the node from
///
/// @remark The topics tree lock is expected to be held for write.
//****************************************************************************
void iett_removeTopicNodeFromOriginServerRecovery(ieutThreadData_t *pThreadData,
                                                  iettTopicNode_t *topicNode,
                                                  iettOriginServer_t *originServer)
{
    ieutTRACEL(pThreadData, topicNode, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "topicNode=%p originServer=%p\n",
               __func__, topicNode, originServer);

    assert(originServer != NULL);

    // We actually only ever expect to be called for the one the topicnode is on
    assert(topicNode->currOriginServer == originServer);

    // Get the index back that we stored away so we can swap it with the last one.
    uint64_t currEntry = (uint64_t)(topicNode->originNext);

    assert(currEntry != 0);

    // Reposition the last array entry to the position we want to remove and change the index
    // value that it was keeping... the entries aren't ordered anyway, so it really doesn't
    // matter that we moved the position of one to remove another.
    originServer->stats.localCount -= 1;

    iettTopicNode_t *repositionedNode = originServer->recovered[originServer->stats.localCount];

    repositionedNode->originNext = (iettTopicNode_t *)currEntry;

    originServer->recovered[currEntry-1] = repositionedNode;

    topicNode->currOriginServer = NULL;
    topicNode->originNext = 0;

    ieutTRACEL(pThreadData, originServer, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "topicNode=%p originServer=%p\n",
               __func__, topicNode, originServer);
}

//****************************************************************************
/// @brief Callback function used to report origin server statistics to the
///        cluster component.
///
/// @param[in]     key          serverUID
/// @param[in]     keyHash      hash value of the serverUID
/// @param[in]     value        The iettOriginServer_t to get values from
/// @param[in]     context      iettOriginServerCbContext_t
//****************************************************************************
void iett_clusterReportOriginServer(ieutThreadData_t *pThreadData,
                                    char *key,
                                    uint32_t keyHash,
                                    void *value,
                                    void *context)
{
    if (ismEngine_serverGlobal.clusterEnabled == true)
    {
        iettOriginServerCbContext_t *pContext = (iettOriginServerCbContext_t *)context;

        assert(value != NULL);

        iettOriginServer_t *originServer = (iettOriginServer_t *)value;

        ismEngine_CheckStructId(originServer->strucId, iettORIGIN_SERVER_STRUCID, ieutPROBE_001);

        pContext->rc = ism_cluster_updateRetainedStats(originServer->serverUID,
                                                       &originServer->stats,
                                                       sizeof(originServer->stats));

        ieutTRACEL(pThreadData, originServer->stats.highestTimestampSeen, ENGINE_HIGH_TRACE,
                   FUNCTION_IDENT "originServer='%s' (%p) highestSeen=%lu count=%u localCount=%u rc=%d\n", __func__,
                   originServer->serverUID, originServer,
                   originServer->stats.highestTimestampSeen,
                   originServer->stats.count,
                   originServer->stats.localCount,
                   pContext->rc);
    }
}

//****************************************************************************
/// @brief Callback function used to reconcile origin servers at the end of
/// recovery.
///
/// This will tidy up anything used only during restart, in particular it will
/// turn the array we built into an ordered linked list, we also report statistics
/// to the cluster component as required.
///
/// @param[in]     key          serverUID
/// @param[in]     keyHash      hash value of the serverUID
/// @param[in]     value        The iettOriginServer_t to get values from
/// @param[in]     context      iettOriginServerCbContext_t
//****************************************************************************
void iett_reconcileOriginServer(ieutThreadData_t *pThreadData,
                                char *key,
                                uint32_t keyHash,
                                void *value,
                                void *context)
{
    iettOriginServer_t *originServer = (iettOriginServer_t *)value;

    assert(originServer != NULL);

    ieutTRACEL(pThreadData, originServer->stats.localCount, ENGINE_FNC_TRACE, FUNCTION_IDENT "Reconciling originServer '%s' Retained msgs:%u (Array size: %u)\n",
               __func__, originServer->serverUID, originServer->stats.localCount, originServer->stats.count);

    assert(originServer->stats.highestTimestampSeen == 0);
    assert(originServer->stats.highestTimestampAvailable == 0);
    assert(originServer->stats.topicsIdentifier == 0);

    // During recovery, we built up an unsorted array of all topicNodes for this originServer. This must be
    // turned into a sorted linked list which is what the runtime code uses -- we do it this way because then
    // recovery is much quicker with only a single sort at the end.
    if (originServer->recovered != NULL)
    {
        uint32_t nodeCount = originServer->stats.localCount;

        // Reset statistics
        originServer->stats.localCount = originServer->stats.count = 0;

        if (nodeCount)
        {
            // Sort the currently unsorted array by timestamp
            iett_sortTopicNodesByTimestamp(pThreadData, originServer->recovered, nodeCount);

            iettTopicNode_t *topicNode = NULL;

            for(uint32_t i=0; i<nodeCount; i++)
            {
                iettTopicNode_t *prevTopicNode = topicNode;

                topicNode = originServer->recovered[i];

                assert(topicNode->currOriginServer == originServer);

                // The first entry in the list is this originServer's head
                if (i == 0)
                {
                    originServer->head = topicNode;
                    topicNode->originPrev = NULL;
                }
                else
                {
                    assert(prevTopicNode != NULL);
                    assert(prevTopicNode != topicNode);
                    assert(topicNode->currRetTimestamp >= prevTopicNode->currRetTimestamp);

                    prevTopicNode->originNext = topicNode;
                    topicNode->originPrev = prevTopicNode;
                }

                topicNode->originNext = NULL;

                iett_incrementOriginServerStats(originServer, topicNode);
            }

            assert(topicNode != NULL);

            // The last entry in the array is our tail (might be the same as the head)
            originServer->tail = topicNode;
            topicNode->originNext = NULL;
        }
        else
        {
            assert(originServer->head == NULL);
            assert(originServer->tail == NULL);
        }

        assert(originServer->stats.localCount == nodeCount);

        iemem_free(pThreadData, iemem_unneededRetainedMsgs, originServer->recovered);
        originServer->recovered = NULL;
    }
    else
    {
        assert(originServer->head == NULL);
        assert(originServer->tail == NULL);
        assert(originServer->stats.count == 0);
        assert(originServer->stats.localCount == 0);
    }

    // Verify the message order & stats recovered during restart
#ifndef NDEBUG
    ism_time_t lastSeenTimestamp = 0;
    uint32_t entryNumber = 0;

    // Go forward, from head to tail
    iettTopicNode_t *curr = originServer->head;
    while(curr != NULL)
    {
        assert(curr->currRetTimestamp >= lastSeenTimestamp);
        entryNumber++;
        lastSeenTimestamp = curr->currRetTimestamp;
        curr = curr->originNext;
    }
    assert(entryNumber == originServer->stats.localCount);

    if (originServer->tail != NULL)
    {
        // If the last timestamp in our linked list is bigger than either the highestTimestampSeen
        // or highestTimestampAvailable statistics, this is only expected if the last entries in
        // the linked list are ones we do not include in the statistics (ie. ones that are not
        // replicated around a cluster) - Let's check that.
        if (lastSeenTimestamp > originServer->stats.highestTimestampSeen ||
            lastSeenTimestamp > originServer->stats.highestTimestampAvailable)
        {
            bool updateExternalStats = (originServer->tail->nodeFlags & iettNODE_FLAG_LOCAL_ORIGIN_STATS_ONLY_MASK) == 0;
            assert(updateExternalStats == false);
        }

        // Go backward, from tail to head
        curr = originServer->tail;
        while(curr != NULL)
        {
            assert(curr->currRetTimestamp <= lastSeenTimestamp);
            entryNumber--;
            lastSeenTimestamp = curr->currRetTimestamp;
            curr = curr->originPrev;
        }
    }
    assert(entryNumber == 0);
#endif

    iett_clusterReportOriginServer(pThreadData, key, keyHash, value, context);
}

//****************************************************************************
/// @brief Callback used to build a list of OriginServers flagged localServer.
//****************************************************************************
typedef struct tag_iettListLocalOriginServersContext_t
{
    iettOriginServer_t **list;
    uint32_t count;
    uint32_t max;
    int32_t rc;
} iettListLocalOriginServersContext_t;

void iett_listLocalOriginServers(ieutThreadData_t *pThreadData,
                                 char *key,
                                 uint32_t keyHash,
                                 void *value,
                                 void *context)
{
    iettListLocalOriginServersContext_t *pContext = (iettListLocalOriginServersContext_t *)context;
    iettOriginServer_t *thisOriginServer = (iettOriginServer_t *)value;

    if (pContext->rc == OK)
    {
        if (thisOriginServer->localServer == true)
        {
            if (pContext->count == pContext->max)
            {
                uint32_t newMax = pContext->max + 10;
                iettOriginServer_t **newList = iemem_realloc(pThreadData,
                                                             IEMEM_PROBE(iemem_topicsTree, 11),
                                                             pContext->list,
                                                             sizeof(iettOriginServer_t *) * newMax);
                if (newList == NULL)
                {
                    iemem_free(pThreadData, iemem_topicsTree, pContext->list);
                    pContext->rc = ISMRC_AllocateError;
                    ism_common_setError(pContext->rc);
                    goto mod_exit;
                }

                pContext->list = newList;
                pContext->max = newMax;
            }

            pContext->list[pContext->count++] = value;
        }
    }

mod_exit:

    return;
}

//****************************************************************************
/// @brief Find all the retained messages that this server has from the
///        specified origin serverUID.
///
/// @param[in]     serverUID           The origin server being requested
/// @param[in]     options             ismENGINE_FORWARD_RETAINED_OPTION_* values
/// @param[in]     timestamp           Timestamp to use in selecting messages
/// @param[in]     ppFoundMessages     Array of messages
/// @param[in]     pFoundMessageCount  Count of the number of messages
///
/// @remark The messages returned in the array will have their use counts
///         incremented by two, persistent messages will also have had their
///         store RefCount incremented by one to ensure that they remain in
///         the store while being processed.
///
///         Once processing has completed, iett_releaseOriginServerRetained
///         should be called to release the array (this will decrement the use
///         count by ONE and the persistent message RefCount by one).
///
/// @return OK on successful completion, ISMRC_NotFound or another
///         ISMRC_ value on error.
//****************************************************************************
int32_t iett_findOriginServerRetainedMessages(ieutThreadData_t *pThreadData,
                                              const char *serverUID,
                                              uint32_t options,
                                              ism_time_t timestamp,
                                              ismEngine_MessageHandle_t **ppFoundMessages,
                                              uint32_t *pFoundMessageCount)
{
    int32_t rc = OK;
    bool treeLocked = false;
    ismEngine_MessageHandle_t *foundMessages = NULL;
    uint32_t foundMessageCount = 0;

    ieutTRACEL(pThreadData, serverUID, ENGINE_FNC_TRACE, FUNCTION_ENTRY "serverUID=%s options=0x%08x timestamp=%lu\n",
               __func__, serverUID ? serverUID : NULL, options, timestamp);

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    iettOriginServer_t *originServer;
    iettOriginServer_t **originServerList = &originServer;
    uint32_t originServerCount;

    ismEngine_getRWLockForRead(&tree->topicsLock);
    treeLocked = true;

    if (serverUID != NULL)
    {
        rc = iett_insertOrFindOriginServer(pThreadData,
                                           serverUID,
                                           iettOP_FIND,
                                           &originServer);

        if (rc != OK) goto mod_exit;

        assert(originServer != NULL);

        originServerCount = 1;
    }
    else
    {
        iettListLocalOriginServersContext_t context = {NULL, 0, 0, OK};

        ieut_traverseHashTable(pThreadData,
                               tree->originServers,
                               iett_listLocalOriginServers,
                               &context);

        if (context.rc != OK)
        {
            rc = context.rc;
            goto mod_exit;
        }

        originServerList = context.list;
        originServerCount = context.count;

        ieutTRACEL(pThreadData, originServerCount, ENGINE_HIGH_TRACE,
                   "Found %u OriginServers flagged localServer\n", originServerCount);
    }

    // For each of the servers we found, add messages to the array
    for(uint32_t i=0; i<originServerCount; i++)
    {
        iettOriginServer_t *thisOriginServer = originServerList[i];

        ieutTRACEL(pThreadData, serverUID, ENGINE_HIGH_TRACE, "Checking serverUID='%s'\n", thisOriginServer->serverUID);

        // TODO: If we ever have localOnly retained messages - these would need to
        //       be discounted from the subsequent set we actually deliver.
        if (thisOriginServer->stats.count != 0)
        {
            ismEngine_MessageHandle_t *newFoundMessages = iemem_realloc(pThreadData,
                                                                        IEMEM_PROBE(iemem_topicsTree, 9),
                                                                        foundMessages,
                                                                        sizeof(ismEngine_MessageHandle_t) *
                                                                        (foundMessageCount + thisOriginServer->stats.count + 1));

            if (newFoundMessages == NULL)
            {
                if (foundMessageCount != 0)
                {
                    assert(foundMessages != NULL);

                    for(uint32_t m=0; m<foundMessageCount; m++)
                    {
                        iem_releaseMessage(pThreadData, foundMessages[m]);
                    }

                    iett_releaseOriginServerRetainedMessages(pThreadData, foundMessages);
                }

                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            foundMessages = newFoundMessages;

            iettTopicNode_t *topicNode = thisOriginServer->head;

            // Add the retained messages from this origin server to the list
            while(topicNode != NULL)
            {
                bool skip = false;

                if (options != ismENGINE_FORWARD_RETAINED_OPTION_NONE)
                {
                    if (((options & ismENGINE_FORWARD_RETAINED_OPTION_NEWER) != 0) &&
                         (topicNode->currRetTimestamp <= timestamp))
                    {
                        skip = true;
                    }
                }

                // If it's a System topic, we don't include the message.
                if (skip == false && (topicNode->nodeFlags & iettNODE_FLAG_BRANCH_SYSTOPIC) == 0)
                {
                    ismEngine_Message_t *pMessage = (ismEngine_Message_t *)topicNode->currRetMessage;

                    assert(pMessage != NULL);

                    // Make sure that persistent messages will stay in the store while they are being
                    // put to the queue.
                    if (pMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
                    {
                        ismStore_Handle_t hStoreMessage;

                        assert(pMessage->StoreMsg.parts.RefCount != 0);
                        rc = iest_storeMessage(pThreadData,
                                               pMessage,
                                               1,
                                               iestStoreMessageOptions_ATOMIC_REFCOUNTING,
                                               &hStoreMessage);
                        assert(rc == OK);
                    }

                    iem_recordMessageMultipleUsage(topicNode->currRetMessage, 2);
                    foundMessages[foundMessageCount++] = topicNode->currRetMessage;
                }

                topicNode = topicNode->originNext;
            }
        }
    }

    // Clean up if we didn't find any messages, and NULL terminate the list if we did.
    if (foundMessageCount == 0)
    {
        iemem_free(pThreadData, iemem_topicsTree, foundMessages);
        foundMessages = NULL;
    }
    else
    {
        foundMessages[foundMessageCount] = NULL; // NULL terminate
    }

    *ppFoundMessages = foundMessages;
    *pFoundMessageCount = foundMessageCount;

mod_exit:

    if (treeLocked == true) ismEngine_unlockRWLock(&tree->topicsLock);
    if (originServerList != &originServer) iemem_free(pThreadData, iemem_topicsTree, originServerList);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "foundMessages=%p foundMessageCount=%u rc=%d\n",
               __func__, foundMessages, foundMessageCount, rc);

    return rc;
}

//****************************************************************************
/// @brief Release the array of retained messages from an origin server
///
/// @param[in]     pFoundMessages     Array of messages
//****************************************************************************
void iett_releaseOriginServerRetainedMessages(ieutThreadData_t *pThreadData,
                                              ismEngine_MessageHandle_t *foundMessages)
{
    assert(foundMessages != NULL);

    uint32_t storeOpCount = 0;

    ismEngine_Message_t *pMessage;

    for(uint32_t i=0; ((pMessage = foundMessages[i]) != NULL); i++)
    {
        // Remove our store use count on the persistent retained messages
        if (pMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
        {
            iest_unstoreMessage(pThreadData, pMessage, false, false, NULL,
                                &storeOpCount);
        }

        // Release our in-memory use count for this message
        iem_releaseMessage(pThreadData, pMessage);
    }

    if (storeOpCount != 0)
    {
        iest_store_commit(pThreadData, false);
    }

    ieutTRACEL(pThreadData, foundMessages, ENGINE_FNC_TRACE, FUNCTION_IDENT "storeOpCount=%u foundMessages=%p\n",
               __func__, storeOpCount, foundMessages);

    iemem_free(pThreadData, iemem_topicsTree, foundMessages);
}

//****************************************************************************
/// @brief Convert an incoming data blob into an iettOriginServerStats_t
///
/// @param[in]     pData              Data blob to be converted
/// @param[in]     dataLength         Length of the data blob
/// @param[out]    originServerStats  Stats structure to fill in
///
/// @return OK on successful completion, or an ISMRC_ value on error.
//****************************************************************************
int32_t iett_convertDataToOriginServerStats(ieutThreadData_t *pThreadData,
                                            void *pData,
                                            uint32_t dataLength,
                                            iettOriginServerStats_t *originServerStats)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, dataLength, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pData=%p dataLength=%u\n",
               __func__, pData, dataLength);

    // This is not even the size of a V1 structure, so must be invalid
    if (dataLength < sizeof(iettOriginServerStats_V1_t))
    {
        rc = ISMRC_InvalidParameter;
    }
    else
    {
        assert(offsetof(iettOriginServerStats_t, version) == 0);

        memcpy(&originServerStats->version, pData, sizeof(originServerStats->version));

        // Most often the version will be the same as ours - so it's just a copy
        if (LIKELY(originServerStats->version == iettORIGIN_SERVER_STATS_CURRENT_VERSION))
        {
            assert(dataLength == sizeof(iettOriginServerStats_t));
            memcpy(originServerStats, pData, sizeof(iettOriginServerStats_t));
        }
        else
        {
            if (dataLength > sizeof(iettOriginServerStats_t))
            {
                memcpy(originServerStats, pData, sizeof(iettOriginServerStats_t));
            }
            else
            {
                memset(originServerStats, 0, sizeof(iettOriginServerStats_t));
                memcpy(originServerStats, pData, dataLength);
            }

            ieutTRACEL(pThreadData, originServerStats->version, ENGINE_INTERESTING_TRACE,
                       "Processing version %u Origin Server stats\n", originServerStats->version);
        }
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Compare two origin server stats structures and decide which represents
///        the most up-to-date one
///
/// @param[in]     statsA             First set of stats to compare
/// @param[in]     statsB             Second set of stats to compare
///
/// @return Positive value if statsA is more up-to-date than statsB, Negative
///         if statsB is more up-to-date than statsA or 0 if they are equal.
//****************************************************************************
int32_t iett_compareOriginServerStats(iettOriginServerStats_t *statsA,
                                      iettOriginServerStats_t *statsB)
{
    int32_t returnVal;

    // TODO: This probably needs to be a more complex test

    if (statsA->highestTimestampSeen > statsB->highestTimestampSeen)
    {
        returnVal = 1;
    }
    else if (statsB->highestTimestampSeen > statsA->highestTimestampSeen)
    {
        returnVal = -1;
    }
    else
    {
        returnVal = 0;
    }

    return returnVal;
}

//****************************************************************************
/// @brief Configuration callback to handle cluster requested topics
///
/// @param[in]     changedProps      Properties qualified by string identifier
///                                  as a suffix
/// @param[in]     changeType        The type of change being made
///
/// @return OK on successful completion or another ISMRC_ value on error.
//****************************************************************************
int iett_clusterRequestedTopicsConfigCallback(ieutThreadData_t *pThreadData,
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
                    ismENGINE_ADMIN_PREFIX_CLUSTERREQUESTEDTOPICS_TOPICSTRING,
                    strlen(ismENGINE_ADMIN_PREFIX_CLUSTERREQUESTEDTOPICS_TOPICSTRING)) == 0)
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

    if (!iett_validateTopicString(pThreadData, topicString, iettVALIDATE_FOR_SUBSCRIBE))
    {
        rc = ISMRC_InvalidClusterRequestedTopic;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // The action taken varies depending on the requested changeType.
    switch(changeType)
    {
        case ISM_CONFIG_CHANGE_PROPS:
            rc = iett_activateClusterRequestedTopic(pThreadData, topicString, false);
            break;

        case ISM_CONFIG_CHANGE_DELETE:
            rc = iett_deactivateClusterRequestedTopic(pThreadData, topicString);
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
/// @brief Activate specified topic string as a cluster requested topic
///
/// @param[in]     topicString       Topic string to activate
/// @param[in]     inRecovery        Whether being called during recovery
///
/// @return OK on successful completion or another ISMRC_ value on error.
///
/// @remark During recovery we update the node, but don't report it to the
/// cluster because the cluster isn't open for business yet, and we will go
/// through and collectively update then.
///
/// @remark This function does NOT update the remote servers tree but rather
/// the subscription tree (because it is the subscription tree that drives
/// requests to other servers for topics to be delivered to this server, and we
/// only want to request the topic if this is the 1st of EITHER an active
/// subscription or the activation of the cluster requested topic)
///
/// @see iett_deactivateClusterRequestedTopic
//****************************************************************************
int32_t iett_activateClusterRequestedTopic(ieutThreadData_t *pThreadData,
                                           const char *topicString,
                                           bool inRecovery)
{
    int32_t rc;
    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *wildcards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *multicards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    iettSubsNode_t *removedSubtree = NULL;

    ieutTRACEL(pThreadData, topicString,  ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "topicString='%s' inRecovery=%d\n",
               __func__, topicString, (int32_t)inRecovery);

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
        // Not already flagged, so we should do so now.
        if ((subsNode->nodeFlags & iettNODE_FLAG_CLUSTER_REQUESTED_TOPIC) == 0)
        {
            subsNode->activeCluster += 1;

            // If we are not running during recovery, and this is the 1st indication that the
            // node should be of interest to the cluster...
            if (inRecovery == false && subsNode->activeCluster == 1)
            {
                ismCluster_SubscriptionInfo_t clusterInfo;

                clusterInfo.pSubscription = subsNode->topicString;
                clusterInfo.fWildcard = (subsNode->nodeFlags & iettNODE_FLAG_BRANCH_WILD_OR_MULTI) != 0;

                if (ismEngine_serverGlobal.clusterEnabled)
                {
                    rc = ism_cluster_addSubscriptions(&clusterInfo, 1);
                }

                // If there is a problem, we should roll this back.
                if (rc != OK)
                {
                    subsNode->activeCluster -= 1;
                    ism_common_setError(rc);

                    // If this node is inactive, clear up whatever we now can.
                    if (iettQUICK_INACTIVE_SUBSNODE_TEST(subsNode))
                    {
                        // Remove any nodes that are inactive from the subscription tree
                        iett_removeInactiveSubsNodesFromEngineTopicTree(pThreadData, subsNode, &removedSubtree);

                        // We removed some nodes, invalidate cached node lists
                        if (removedSubtree != NULL)
                        {
                            tree->cacheUpdates++;
                        }
                    }

                    goto mod_exit;
                }
            }

            // Everything has been updated, we can flag the node as one with a cluster requested topic.
            subsNode->nodeFlags |= iettNODE_FLAG_CLUSTER_REQUESTED_TOPIC;
        }
        else
        {
            rc = ISMRC_ExistingClusterRequestedTopic;
            goto mod_exit;
        }
    }

mod_exit:

    // Unlock if we locked the subscription tree
    ismEngine_unlockRWLock(&tree->subsLock);

    // If a subtree was removed from the tree, destroy it now.
    if (removedSubtree != NULL)
    {
        iett_destroySubsTreeCallback(pThreadData, NULL, 0, removedSubtree, NULL);
    }

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
/// @brief Deactivate the specified topic string as a cluster requested topic
///
/// @param[in]     topicString    Topic string to deactivate
///
/// @return OK on successful completion or another ISMRC_ value on error.
///
/// @remark This function does NOT update the remote servers tree but rather
/// the subscription tree (because it is the subscription tree that drives
/// requests to other servers for topics to be delivered to this server, and we
/// only want to cancel if there are BOTH no active subscriptions AND it is
/// not a requested topic).
///
/// @see iett_activateClusterRequestedTopic
//****************************************************************************
int32_t iett_deactivateClusterRequestedTopic(ieutThreadData_t *pThreadData,
                                             const char *topicString)
{
    int32_t rc;
    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *wildcards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *multicards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    iettSubsNode_t *removedSubtree = NULL;

    ieutTRACEL(pThreadData, topicString,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "topicString='%s'\n", __func__, topicString);

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = topicString;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

    rc = iett_analyseTopicString(pThreadData, &topic);

    // Not possible for cluster requested topic to be active - nothing to do
    if (rc != OK) goto mod_exit_no_unlock;

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    // Get the lock for Write
    ismEngine_getRWLockForWrite(&tree->subsLock);

    iettSubsNode_t *subsNode = NULL;

    rc = iett_insertOrFindSubsNode(pThreadData, tree->subs, &topic, iettOP_FIND, &subsNode);

    // Didn't find the node - nothing to do
    if (rc != OK) goto mod_exit;

    assert(subsNode != NULL);

    // If the node is found and has a cluster requested topic activated, we need to deactivate it.
    if ((subsNode->nodeFlags & iettNODE_FLAG_CLUSTER_REQUESTED_TOPIC) != 0)
    {
        subsNode->activeCluster -= 1;

        // Node not interesting to the cluster any more?
        if (subsNode->activeCluster == 0)
        {
            ismCluster_SubscriptionInfo_t clusterInfo;

            clusterInfo.pSubscription = subsNode->topicString;
            clusterInfo.fWildcard = (subsNode->nodeFlags & iettNODE_FLAG_BRANCH_WILD_OR_MULTI) != 0;

            if (ismEngine_serverGlobal.clusterEnabled)
            {
                rc = ism_cluster_removeSubscriptions(&clusterInfo, 1);
                if (rc == ISMRC_NotFound) rc = OK;
            }

            if (rc != OK)
            {
                subsNode->activeCluster += 1;
                ism_common_setError(rc);
                goto mod_exit;
            }

            // If this node is inactive, clear up whatever we now can.
            if (iettQUICK_INACTIVE_SUBSNODE_TEST(subsNode))
            {
                // Remove any nodes that are inactive from the subscription tree
                iett_removeInactiveSubsNodesFromEngineTopicTree(pThreadData, subsNode, &removedSubtree);

                // We removed some nodes, invalidate cached node lists
                if (removedSubtree != NULL)
                {
                    tree->cacheUpdates++;
                }
            }
        }

        subsNode->nodeFlags &= ~iettNODE_FLAG_CLUSTER_REQUESTED_TOPIC;
    }
    else
    {
        rc = ISMRC_NotFound;
    }

mod_exit:

    // Unlock if we locked the subscription tree
    ismEngine_unlockRWLock(&tree->subsLock);

    // If a subtree was removed from the tree, destroy it now.
    if (removedSubtree != NULL)
    {
        iett_destroySubsTreeCallback(pThreadData, NULL, 0, removedSubtree, NULL);
    }

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
