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
/// @file  topicTree.c
/// @brief Engine component topic tree access functions
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
#include "msgCommon.h"          // iem functions & constants
#include "engineStore.h"        // iest functions & constants
#include "messageExpiry.h"      // ieme functions & constants
#include "remoteServers.h"      // iers functions & constants
#include "resourceSetStats.h"   // iere functions & constants

//****************************************************************************
/// @brief Create and initialize the global engine topic tree
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark Will replace the existing global topic tree without checking that
///         this has happened.
///
/// @see iett_getEngineTopicTree
/// @see iett_destroyEngineTopicTree
//****************************************************************************
int32_t iett_initEngineTopicTree(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Assert some basic things we expect to be true...
    assert(ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK == 0x1D7F);
    assert(ismENGINE_SUBSCRIPTION_OPTION_SHARING_CLIENT_PERSISTENT_MASK == 0x1DFF);

    ismEngine_serverGlobal.maintree = iett_createTopicTree(pThreadData);

    if (NULL == ismEngine_serverGlobal.maintree)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Returns the global engine topic tree to the caller
///
/// @return Pointer to topic tree, NULL if not initialized.
///
/// @see iett_initEngineTopicTree
/// @see iett_destroyEngineTopicTree
//****************************************************************************
iettTopicTree_t *iett_getEngineTopicTree(ieutThreadData_t *pThreadData)
{
    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ieutTRACEL(pThreadData, tree, ENGINE_FNC_TRACE, FUNCTION_IDENT "tree=%p\n", __func__, tree);

    return tree;
}

//****************************************************************************
/// @brief Destroy and remove the global engine topic tree
///
/// @remark This will result in all subscription information being discarded.
///
/// @see iett_getEngineTopicTree
/// @see iett_initEngineTopicTree
//****************************************************************************
void iett_destroyEngineTopicTree(ieutThreadData_t *pThreadData)
{
    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ieutTRACEL(pThreadData, tree, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (NULL != tree)
    {
        iett_destroyTopicTree(pThreadData, tree);
        ismEngine_serverGlobal.maintree = NULL;
    }

    ieutTRACEL(pThreadData, tree, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief Find an existing node underneath a given parent node that matches a
///        given topic string in a subscription tree. Optionally, if an
///        existing node is not found, insert one and return the newly inserted
///        node.
///
/// @param[in]     parent         Parent node in subscription tree
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
int32_t iett_insertOrFindSubsNode(ieutThreadData_t *pThreadData,
                                  iettSubsNode_t *parent,
                                  iettTopic_t *topic,
                                  int32_t operation,
                                  iettSubsNode_t **node)
{
    iettSubsNode_t  *localNode = NULL;
    int32_t          rc = OK;
    uint32_t         multicardsSeen = 0;
    uint32_t         wildcardsSeen = 0;
    const char     **substring = topic->substrings;
    const uint32_t  *substringHash = topic->substringHashes;
    const char     **wildcard = topic->wildcards;
    const char     **multicard = topic->multicards;

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

            size_t topicStringLength;

            if (NULL != parent->topicString)
            {
                topicStringLength = strlen(parent->topicString)+1+strlen(*substring)+1;
            }
            else
            {
                topicStringLength = strlen(*substring)+1;
            }


            localNode = (iettSubsNode_t*)iemem_calloc(pThreadData,
                                                      IEMEM_PROBE(iemem_subsTree, 1),
                                                      1, sizeof(iettSubsNode_t)+topicStringLength);

            if (NULL == localNode)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            memcpy(localNode->strucId, iettSUBSCRIPTION_NODE_STRUCID, 4);

            // If this node is at or below multiple multi-cards, add to it's flags
            if (multicardsSeen > 1) nodeFlags |= iettNODE_FLAG_BRANCH_MULTIMULTI;
            // If this node is at or below ANY wild-cards or multi-cards, add to it's flags
            if (wildcardsSeen != 0 || multicardsSeen != 0) nodeFlags |= iettNODE_FLAG_BRANCH_WILD_OR_MULTI;
            // If this node is at or below a system topic, add to it's flags
            if (topic->sysTopicEndIndex != 0) nodeFlags |= iettNODE_FLAG_BRANCH_SYSTOPIC;

            localNode->nodeFlags = nodeFlags;
            localNode->parent = parent;
            localNode->topicString = (char *)(localNode+1);

            char *localNodeTopicSubstring = localNode->topicString;

            if (NULL != parent->topicString)
            {
                strcpy(localNode->topicString, parent->topicString);
                strcat(localNode->topicString, "/");

                localNodeTopicSubstring += strlen(localNode->topicString);
            }

            strcpy(localNodeTopicSubstring, *substring);

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
                                                  iettINITIAL_SUBSCRIBER_NODE_CAPACITY,
                                                  iemem_subsTree,
                                                  &parent->children);

                        if (rc != OK)
                        {
                            iemem_freeStruct(pThreadData, iemem_subsTree, localNode, localNode->strucId);
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
                                iemem_freeStruct(pThreadData, iemem_subsTree, localNode, localNode->strucId);
                                goto mod_exit;
                            }
                        }
                    }

                    rc = ieut_putHashEntry(pThreadData,
                                           parent->children,
                                           ieutFLAG_DUPLICATE_NONE,
                                           localNodeTopicSubstring,
                                           *substringHash,
                                           localNode,
                                           0);

                    if (rc != OK)
                    {
                        iemem_freeStruct(pThreadData, iemem_subsTree, localNode, localNode->strucId);
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

// Function to return the appropriate resourceSet 'total' type to use for a given subscription
static inline iereResourceSet_I64_StatType_t iett_getSubResSetTotalType(ismEngine_Subscription_t *subscription)
{
    iereResourceSet_I64_StatType_t statType;

    // Determine which resourceSet stats should be updated
    if ((subscription->internalAttrs & iettSUBATTR_PERSISTENT) != 0)
    {
        statType = ((subscription->internalAttrs & iettSUBATTR_GLOBALLY_SHARED) != 0) ?
                ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_SHARED_SUBSCRIPTIONS :
                ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_SUBSCRIPTIONS;
    }
    else
    {
        statType = ((subscription->internalAttrs & iettSUBATTR_GLOBALLY_SHARED) != 0) ?
                ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_NONPERSISTENT_SHARED_SUBSCRIPTIONS :
                ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_NONPERSISTENT_SUBSCRIPTIONS;
    }

    return statType;
}

//****************************************************************************
/// @brief From the specified node, traverse up and find the highest node in
///        the tree that is inactive, if there are any, remove that node (and
///        therefore all of it's children) from the tree.
///
/// @param[in]     node            The node on which to start processing
/// @param[out]    removedSubtree  The subtree that was removed
///
/// @remark The lock covering the tree is expected to be held for write.
///
/// @remark If a node is returned, it will have been removed from the tree
///         but will not have been free'd. Once the lock has been released,
///         the caller should call iett_destroySubsTreeCallback to destroy it.
///
/// @see iett_analyseTopicString
/// @see iett_destroySubsTreeCallback
//****************************************************************************
void iett_removeInactiveSubsNodesFromEngineTopicTree(ieutThreadData_t *pThreadData,
                                                     iettSubsNode_t *node,
                                                     iettSubsNode_t **removedSubtree)
{
    iettSubsNode_t *removeNode = NULL;
    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;
    uint64_t allowChildren = 0;
    bool allowWildcard = false;
    bool allowMulticard = false;

    ieutTRACEL(pThreadData, node,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "node=%p topic='%s'\n", __func__, node, node->topicString);

    // While we have a node in our hands and it is not the root of the tree (the root has
    // no parent) and the node looks inactive, move our starting point up the tree.
    while (NULL != node &&
           node->parent != NULL &&
           iettQUICK_INACTIVE_SUBSNODE_TEST(node))
    {
        // Check if no child nodes left and move the removeNode up to it.
        if ((NULL == node->children || node->children->totalCount == allowChildren) &&
             (allowWildcard || NULL == node->wildcardChild) &&
             (allowMulticard || NULL == node->multicardChild))
        {
            removeNode = node;

            // Unhook the stats from the list of stats structures
            if (node->stats != NULL)
            {
                if (node->stats->prev != NULL)
                {
                    node->stats->prev->next = node->stats->next;
                }
                else
                {
                    tree->subNodeStatsHead = node->stats->next;
                }

                if (node->stats->next != NULL)
                {
                    node->stats->next->prev = node->stats->prev;
                }
            }

            // From this node, decide what the parent can have and still
            // be considered inactive
            uint32_t nodeType = node->nodeFlags & iettNODE_FLAG_TYPE_MASK;

            allowChildren = (nodeType == iettNODE_FLAG_NORMAL) ? 1 : 0;
            allowWildcard = (nodeType == iettNODE_FLAG_WILDCARD);
            allowMulticard = (nodeType == iettNODE_FLAG_MULTICARD);

            node = node->parent;
        }
        else
        {
            // Cannot remove the node, can we free any storage?
            if (NULL != node->activeSubs.list)
            {
                iemem_free(pThreadData, iemem_subsTree, node->activeSubs.list);
                node->activeSubs.list = NULL;
                node->activeSubs.max = 0;
            }

            if (NULL != node->delPendSubs.list)
            {
                iemem_free(pThreadData, iemem_subsTree, node->delPendSubs.list);
                node->delPendSubs.list = NULL;
                node->delPendSubs.max = 0;
            }

            node = NULL;
        }
    }

    // If we found a subtree that is eligible for removal, remove it from the tree.
    // The subtree will be destroyed by the caller.
    if (removeNode != NULL)
    {
        uint32_t nodeType = removeNode->nodeFlags & iettNODE_FLAG_TYPE_MASK;

        if (iettNODE_FLAG_WILDCARD == nodeType)
        {
            removeNode->parent->wildcardChild = NULL;
        }
        else if (iettNODE_FLAG_MULTICARD == nodeType)
        {
            removeNode->parent->multicardChild = NULL;
        }
        else
        {
            char *substring = strrchr(removeNode->topicString, '/') + 1;

            if (substring == (char *)1) substring = removeNode->topicString;

            ieut_removeHashEntry(pThreadData,
                                 removeNode->parent->children,
                                 substring,
                                 iett_generateSubstringHash(substring));
        }
        removeNode->parent = NULL;

        // Tell the caller the node we removed (which is the top of the subtree)
        *removedSubtree = removeNode;
    }

    ieutTRACEL(pThreadData, removeNode,  ENGINE_FNC_TRACE, FUNCTION_EXIT "removeNode=%p\n", __func__, removeNode);
}

//****************************************************************************
/// @brief Find a node in the specified topic information tree that matches
///        the given topic string. If a matching node is not found, optionally
///        insert a new node and return that.
///
/// @param[in]     parent         Parent node in subscription tree
/// @param[in]     topic          The topic being added / found.
/// @param[in]     operation      The operation being performed requiring this node.
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
int32_t iett_insertOrFindTopicsNode(ieutThreadData_t *pThreadData,
                                    iettTopicNode_t  *parent,
                                    iettTopic_t      *topic,
                                    int32_t           operation,
                                    iettTopicNode_t **node)
{
    iettTopicNode_t   *localNode = NULL;
    int32_t            rc = OK;
    const char       **substring = topic->substrings;
    const uint32_t    *substringHash = topic->substringHashes;

    assert(topic->destinationType == ismDESTINATION_TOPIC);
    assert(*substring == topic->topicStringCopy);

    ieutTRACEL(pThreadData, topic->topicString,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "topicString='%s'\n", __func__, topic->topicString);

    while(1)
    {
        if (NULL == parent->children)
        {
            localNode = NULL;
        }
        else
        {
            rc = ieut_getHashEntry(parent->children,
                                   *substring,
                                   *substringHash,
                                   (void **)&localNode);

            if (rc == ISMRC_NotFound)
            {
                localNode = NULL;
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

            localNode = (iettTopicNode_t*)iemem_calloc(pThreadData,
                                                       IEMEM_PROBE(iemem_topicsTree, 1),
                                                       1, sizeof(iettTopicNode_t)+strlen(*substring)+1);

            if (NULL == localNode)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            memcpy(localNode->strucId, iettTOPIC_NODE_STRUCID, 4);
            localNode->nodeFlags = iettNODE_FLAG_NORMAL;
            localNode->parent = parent;
            localNode->resourceSet = iere_getResourceSet(pThreadData, NULL, topic->topicStringCopy, iereOP_ADD);

            // If this node is at or below a system topic, add to it's flags
            if (topic->sysTopicEndIndex != 0) localNode->nodeFlags |= iettNODE_FLAG_BRANCH_SYSTOPIC;

            char *topicSubstring = (char *)(localNode+1);

            strcpy(topicSubstring, *substring);

            if (NULL == parent->children)
            {
                rc = ieut_createHashTable(pThreadData,
                                          iettINITIAL_TOPICS_NODE_CAPACITY,
                                          iemem_topicsTree,
                                          &parent->children);

                if (rc != OK)
                {
                    iemem_freeStruct(pThreadData, iemem_topicsTree, localNode, localNode->strucId);
                    goto mod_exit;
                }
            }

            if (parent->children->totalCount > (parent->children->capacity * (iettNODE_LOADINGFACTOR_HIGH_WATER-1)))
            {
                rc = ieut_resizeHashTable(pThreadData,
                                          parent->children,
                                          parent->children->capacity * iettNODE_CAPACITY_INCREMENT_FACTOR);

                if (rc != OK)
                {
                    iemem_freeStruct(pThreadData, iemem_topicsTree, localNode, localNode->strucId);
                    goto mod_exit;
                }
            }

            rc = ieut_putHashEntry(pThreadData,
                                   parent->children,
                                   ieutFLAG_DUPLICATE_NONE,
                                   topicSubstring,
                                   *substringHash,
                                   localNode,
                                   0);

            if (rc != OK)
            {
                iemem_freeStruct(pThreadData, iemem_topicsTree, localNode, localNode->strucId);
                goto mod_exit;
            }
        }

        parent = localNode;
        substring++;
        substringHash++;

        if (*substring == NULL)
        {
            // Make sure the topicStringCopy now looks like the original
            assert(strcmp(topic->topicString, topic->topicStringCopy) == 0 || operation != iettOP_ADD);
            break;
        }

        if (operation == iettOP_ADD) *(char *)((*substring)-1) = '/';
    }

mod_exit:

    if (operation == iettOP_ADD)
    {
        substring = topic->substrings;
        while(*(++substring) != NULL) *(char *)((*substring)-1) = '\0';
        // Make sure the topicStringCopy is correct again...
        assert(strcmp(topic->topicString, topic->topicStringCopy) != 0 || topic->substringCount == 1);
    }

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
/// @brief Add a subscription to a list of subscriptions on a subsNode
///
/// @param[in]     subscription  Subscription being added
/// @param[in out] subList       List being updated
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_addSubscriptionToSubsNode(ieutThreadData_t *pThreadData,
                                       ismEngine_Subscription_t *subscription,
                                       iettSubscriptionList_t *subList)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, subscription,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "subscription=%p, subList=%p\n", __func__, subscription, subList);

    // Ensure there is space in the list for another subscription
    if (subList->count == subList->max)
    {
        uint32_t newMax = subList->max;

        if (newMax == 0)
            newMax = iettINITIAL_SUBSCRIBER_ARRAY_CAPACITY;
        else
            newMax *= 2;

        ismEngine_Subscription_t **newSubList;

        // Note: We allocate one extra entry to allow for a NULL sentinel
        newSubList = iemem_realloc(pThreadData,
                                   IEMEM_PROBE(iemem_subsTree, 2),
                                   subList->list,
                                   (newMax+1) * sizeof(ismEngine_Subscription_t *));

        if (NULL == newSubList)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        subList->max = newMax;
        subList->list = newSubList;
    }

    // Must not be on another SUBINDEX list
    assert(subscription->nodeListIndex == 0xffffffff);
    subscription->nodeListIndex = subList->count;
    subList->list[subList->count] = subscription;
    subList->count++;
    subList->list[subList->count] = NULL;

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Remove a subscription from a list of subscriptions on a subsNode
///
/// @param[in]     subscription  Subscription being removed
/// @param[in out] subList       List being updated
///
/// @return OK on successful completion, ISMRC_NotFound if the subscription
///         is not in the list or an ISMRC_ value.
//****************************************************************************
int32_t iett_removeSubscriptionFromSubsNode(ieutThreadData_t *pThreadData,
                                            ismEngine_Subscription_t *subscription,
                                            iettSubscriptionList_t *subList)
{
    int32_t   rc = ISMRC_NotFound;
    uint32_t  middle = subscription->nodeListIndex;
    uint32_t  end = subList->count;

    ieutTRACEL(pThreadData, subscription,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "subscription=%p, subList=%p\n", __func__, subscription, subList);

    if (middle < subList->count && subList->list[middle] == subscription)
    {
        rc = OK;
    }
    else
    {
        // If all else fails, linear search of the list
        for(middle=0; middle<end; middle++)
        {
            if (subList->list[middle] == subscription)
            {
                rc = OK;
                break;
            }
        }
    }

    // Move entries in the list, freeing it if no entries left, and
    // null terminating it if there are
    if (rc == OK)
    {
        // Give ourselves a local variable for the end of the list
        end = subList->count;

        if (end > 1)
        {
            // Move the end to this position, and update it's index
            subList->list[middle] = subList->list[end-1];
            subList->list[middle]->nodeListIndex = middle;
        }

        subscription->nodeListIndex = 0xffffffff;
        subList->count--;

        if (subList->count == 0)
        {
            if (NULL != subList->list)
            {
                iemem_free(pThreadData, iemem_subsTree, subList->list);
                subList->list = NULL;
            }
            subList->max = 0;
        }
        else
        {
            subList->list[subList->count] = NULL;
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Add a subscription to the active subscription list of a subsNode
///
/// @param[in]     subscription  Subscription being added
/// @param[in]     tree          Topic tree
/// @param[in]     subsNode      Subscription node
/// @param[in]     inRecovery    Whether we are currently in recovery
///
/// @remark The subscription tree must be locked for write
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static inline int32_t iett_addActiveSubscriptionToSubsNode(ieutThreadData_t *pThreadData,
                                                           ismEngine_Subscription_t *subscription,
                                                           iettTopicTree_t *tree,
                                                           iettSubsNode_t *subsNode,
                                                           bool inRecovery)
{
    int32_t   rc;

    ieutTRACEL(pThreadData, subsNode,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "subscription=%p, subsNode=%p\n", __func__, subscription, subsNode);

    rc = iett_addSubscriptionToSubsNode(pThreadData,
                                        subscription,
                                        &subsNode->activeSubs);

    if (rc != OK) goto mod_exit;

    bool clusterSub = (subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER) == iettSUBATTR_SHARE_WITH_CLUSTER;

    // This subscription is one that should be shared with the cluster
    if (clusterSub == true)
    {
        subsNode->activeCluster += 1;
    }

    // During normal running, we may need to do more work
    if (!inRecovery)
    {
        // If this is the 1st active subscription on this node invalidate any per-thread
        // caches.
        if (subsNode->activeSubs.count == 1)
        {
            tree->cacheUpdates++;
        }

        // If this is the 1st active clustered subscription tell the cluster component
        if (clusterSub == true && subsNode->activeCluster == 1)
        {
            ismCluster_SubscriptionInfo_t clusterInfo;

            clusterInfo.pSubscription = subsNode->topicString;
            clusterInfo.fWildcard = (subsNode->nodeFlags & iettNODE_FLAG_BRANCH_WILD_OR_MULTI) != 0;

            if (ismEngine_serverGlobal.clusterEnabled)
            {
                rc = ism_cluster_addSubscriptions(&clusterInfo, 1);
            }

            // If there is a problem, we need to remove this subscription to keep things
            // straight.
            if (rc != OK)
            {
                (void)iett_removeSubscriptionFromSubsNode(pThreadData,
                                                          subscription,
                                                          &subsNode->activeSubs);
                subsNode->activeCluster -= 1;
                ism_common_setError(rc);
                goto mod_exit;
            }
        }
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Remove a subscription from the active subscription list of a subsNode
///
/// @param[in]     subscription  Subscription being removed
/// @param[in]     tree          Topic tree
/// @param[in]     subsNode      Subscription node
/// @param[in]     inRecovery    Whether we are currently in recovery
///
/// @remark The subscription tree must be locked for write
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static inline int32_t iett_removeActiveSubscriptionFromSubsNode(ieutThreadData_t *pThreadData,
                                                                ismEngine_Subscription_t *subscription,
                                                                iettTopicTree_t *tree,
                                                                iettSubsNode_t *subsNode,
                                                                bool inRecovery)
{
    int32_t   rc;

    ieutTRACEL(pThreadData, subsNode,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "subscription=%p, subsNode=%p\n", __func__, subscription, subsNode);

    rc = iett_removeSubscriptionFromSubsNode(pThreadData,
                                             subscription,
                                             &subsNode->activeSubs);

    if (rc != OK) goto mod_exit;

    bool clusterSub = (subscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER) == iettSUBATTR_SHARE_WITH_CLUSTER;

    // This subscription is one that should have been shared with the cluster
    if (clusterSub == true)
    {
        subsNode->activeCluster -= 1;
    }

    // During normal running, we may need to do more work
    if (!inRecovery)
    {
        // If that was the last active clustered subscription tell the cluster component
        if (clusterSub == true && subsNode->activeCluster == 0)
        {
            ismCluster_SubscriptionInfo_t clusterInfo;

            clusterInfo.pSubscription = subsNode->topicString;
            clusterInfo.fWildcard = (subsNode->nodeFlags & iettNODE_FLAG_BRANCH_WILD_OR_MULTI) != 0;

            if (ismEngine_serverGlobal.clusterEnabled)
            {
                rc = ism_cluster_removeSubscriptions(&clusterInfo, 1);
                if (rc == ISMRC_NotFound) rc = OK;
            }

            // If there is a problem, we need to add the subscription back in to keep things
            // straight.
            if (rc != OK)
            {
                (void)iett_addSubscriptionToSubsNode(pThreadData,
                                                     subscription,
                                                     &subsNode->activeSubs);
                subsNode->activeCluster += 1;
                ism_common_setError(rc);
                goto mod_exit;
            }
        }
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Context for iett_findRegExTopicsNodeCallback and iett_findRegExTopicsNode
//****************************************************************************
typedef struct tag_iettFindRegExTopicsCbContext_t
{
    int32_t rc;
    int32_t topicLevel;
    char *topicString;
    size_t topicStringLength;
    size_t topicStringBufferSize;
    ism_regex_t regex;
    uint32_t *maxNodes;
    uint32_t *nodeCount;
    iettTopicNode_t ***result;
} iettFindRegExTopicsCbContext_t;

//****************************************************************************
// Forward declaration of iett_findRegExTopicsNode
//****************************************************************************
void iett_findRegExTopicsNode(ieutThreadData_t *pThreadData,
                              iettTopicNode_t *node,
                              char *subString,
                              iettFindRegExTopicsCbContext_t *context);

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
void iett_findRegExTopicsNodeCallback(ieutThreadData_t *pThreadData,
                                      char *key,
                                      uint32_t keyHash,
                                      void *value,
                                      void *context)
{
    iett_findRegExTopicsNode(pThreadData,
                             (iettTopicNode_t *)value,
                             key,
                             (iettFindRegExTopicsCbContext_t *)context);
}

//****************************************************************************
/// @brief Write all of the topics nodes below a given node to the dump.
///
/// @param[in]     node     Node from which to begin printing.
/// @param[in,out] context  Context of the dump
///
/// @see iett_dumpTopicTree
//****************************************************************************
void iett_findRegExTopicsNode(ieutThreadData_t *pThreadData,
                              iettTopicNode_t *node,
                              char *subString,
                              iettFindRegExTopicsCbContext_t *context)
{
    if (context->rc == OK)
    {
        size_t substringLength;

        if (subString)
        {
            substringLength = strlen(subString);

            if ((context->topicStringLength + substringLength + 2) > context->topicStringBufferSize)
            {
                char *newStringBuffer = iemem_realloc(pThreadData,
                                                      IEMEM_PROBE(iemem_topicsQuery, 3),
                                                      context->topicString,
                                                      context->topicStringBufferSize + 1024);

                if (newStringBuffer == NULL)
                {
                    context->rc = ISMRC_AllocateError;
                    ism_common_setError(context->rc);
                    goto mod_exit;
                }

                context->topicString = newStringBuffer;
            }

            strcpy(&context->topicString[context->topicStringLength], subString);

            if (ism_regex_match(context->regex, context->topicString) == 0)
            {
                if (*context->nodeCount == *context->maxNodes)
                {
                    uint32_t newMax = (*context->maxNodes) + iettFIND_TOPIC_RESULTS_INCREMENT;

                    iettTopicNode_t **newResult;

                    newResult = iemem_realloc(pThreadData,
                                              IEMEM_PROBE(iemem_topicsQuery, 4),
                                              *context->result, newMax * sizeof(iettTopicNode_t *));

                    if (NULL == newResult)
                    {
                        context->rc = ISMRC_AllocateError;
                        ism_common_setError(context->rc);
                        goto mod_exit;
                    }

                    *context->maxNodes = newMax;
                    *context->result = newResult;
                }

                (*context->result)[(*context->nodeCount)++] = (iettTopicNode_t *)node;
            }
        }
        else
        {
            substringLength = 0;
        }

        if (node->children)
        {
            context->topicStringLength += substringLength;
            context->topicLevel += 1;

            if (context->topicLevel > 1)
            {
                context->topicString[context->topicStringLength++] = '/';
                substringLength += 1;
            }

            ieut_traverseHashTable(pThreadData,
                                   node->children,
                                   iett_findRegExTopicsNodeCallback,
                                   context);

            context->topicLevel--;
            context->topicStringLength -= substringLength;
        }
    }

mod_exit:

    return;
}

//****************************************************************************
/// @brief Build the result array from a hash set of topic node results
///
/// @param[in]     value    Value of the hash entry (Node)
/// @param[in]     context  The context passed to the callback routine
//****************************************************************************
typedef struct tag_iettFindTopicNodeContext_t
{
    iettTopicNode_t **results;
    uint32_t nodeIndex;
} iettFindTopicNodeContext_t;

void iett_findTopicsNodesCallback(ieutThreadData_t *pThreadData,
                                  uint64_t value,
                                  void *context)
{
    iettFindTopicNodeContext_t *pContext = (iettFindTopicNodeContext_t *)context;

    pContext->results[pContext->nodeIndex] = (iettTopicNode_t *)value;
    (pContext->nodeIndex)++;
}

//****************************************************************************
/// @brief Find the nodes in a topic information tree which match a given
///        (possibly wildcarded) topic string.
///
/// @param[in]     parent      Parent node under which to start searching.
/// @param[in]     multiMode   Processing a multicard entry - implies that
///                            we must visit all child nodes.
/// @param[in]     topic       Topic analysis to use
/// @param[in]     curIndex    Current index into the substring arrays
/// @param[in]     wildIndex   Current index into the wildcard array
/// @param[in]     multiIndex  Current index into the multicard array
/// @param[in,out] resultSet   Set of results used when duplicates possible
/// @param[in,out] maxNodes    Space available in the array of topic nodes
/// @param[out]    nodeCount   Count of nodes containing subscribers found.
/// @param[in,out] result      Current array of nodes
///
/// @return OK on successful completion or an ISMRC_ value
///
/// @remark The lock covering the nodes being added must be held for read.
///
/// @remark This function calls itself recursively for child nodes. If there
///         is a possibility that this recursion could result in duplicate
///         nodes being found we use a hash set (resultSet) to build a set
///         of unique nodes, then traverse the set to build the list - if
///         there is no duplication possible, the list is built directly (thus
///         avoiding the overhead of the hash set).
//****************************************************************************
int32_t iett_findMatchingTopicsNodes(ieutThreadData_t *pThreadData,
                                     const iettTopicNode_t *parent,
                                     const bool multiMode,
                                     const iettTopic_t *topic,
                                     const uint32_t curIndex,
                                     const uint32_t wildIndex,
                                     const uint32_t multiIndex,
                                     ieutHashSet_t *resultSet,
                                     uint32_t *maxNodes,
                                     uint32_t *nodeCount,
                                     iettTopicNode_t ***result)
{
    int32_t          rc = OK;
    ieutHashTable_t *table = parent->children;
    iettTopicNode_t *node = NULL;
    int32_t          chain;
    int32_t          index;
    ieutHashEntry_t *entry;
    bool             childrenExist;

    // Decide whether we need to gather results in a hash table or straight into an array.
    //
    // We use the hash table when there is a possibility that there will be duplicates in the
    // results (i.e. when there are multiple multi-level wildcards in the subscription).
    if (curIndex == 0)
    {
        if (topic->multicardCount > 1)
        {
            assert(resultSet == NULL);

            rc = ieut_createHashSet(pThreadData, iemem_topicsQuery, &resultSet);

            if (rc != OK) goto mod_exit;
        }

        ieutTRACEL(pThreadData, topic, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "topicString='%s' resultSet=%p\n", __func__,
                   topic->topicString, resultSet);

        // For a regular expression topic string, we call a separate function
        if (topic->destinationType == ismDESTINATION_REGEX_TOPIC)
        {
            assert(topic->regex != NULL);

            iettFindRegExTopicsCbContext_t context;

            // Allocate an initial area to track the topic string - this is reallocated
            // if needed as the tree is traversed
            context.topicString = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_topicsQuery, 5), 1024);

            if (context.topicString == NULL)
            {
                rc = ISMRC_AllocateError;
                goto mod_exit;
            }

            context.rc = OK;
            context.topicLevel = 0;
            context.topicStringLength = 0;
            context.topicStringBufferSize = 1024;
            context.regex = topic->regex;
            context.maxNodes = maxNodes,
            context.nodeCount = nodeCount,
            context.result = result;

            iett_findRegExTopicsNode(pThreadData, (iettTopicNode_t *)parent, NULL, &context);

            iemem_free(pThreadData, iemem_topicsQuery, context.topicString);

            rc = context.rc;

            goto mod_exit;
        }

        assert(topic->destinationType == ismDESTINATION_TOPIC);
    }

    const char *curSubstring = topic->substrings[curIndex];

    // Remember whether the parent node has any children.
    if (NULL != table && table->totalCount != 0)
    {
        childrenExist = true;
    }
    else
    {
        childrenExist = false;
    }

    // If this substring is the last one in the topic being matched,
    // add the node to our list of matching nodes.
    if (curIndex == topic->substringCount)
    {
        if ((parent->nodeFlags & iettNODE_FLAG_TYPE_MASK) != iettNODE_FLAG_TREE_ROOT)
        {
            // If we are using a hash set to avoid duplicates, add this node to it now
            if (resultSet != NULL)
            {
                rc = ieut_addValueToHashSet(pThreadData, resultSet, (uint64_t)parent);

                if (rc != OK) goto mod_exit;
            }
            // Otherwise build the result string now
            else
            {
                if (*nodeCount == *maxNodes)
                {
                    uint32_t newMax = (*maxNodes) + iettFIND_TOPIC_RESULTS_INCREMENT;

                    iettTopicNode_t **newResult;

                    newResult = iemem_realloc(pThreadData, IEMEM_PROBE(iemem_topicsQuery, 1), *result, newMax * sizeof(iettTopicNode_t *));

                    if (NULL == newResult)
                    {
                        rc = ISMRC_AllocateError;
                        ism_common_setError(rc);
                        goto mod_exit;
                    }

                    *maxNodes = newMax;
                    *result = newResult;
                }

                (*result)[(*nodeCount)++] = (iettTopicNode_t *)parent;
            }
        }
    }
    // Not yet at the end of the topic being matched, need to recurse.
    else
    {
        uint32_t nextIndex = curIndex+1;

        // This substring is a multiple wildcard node (#) so need to revisit
        // this node and all of it's children with the remaining topic string.
        if (curSubstring == topic->multicards[multiIndex])
        {
            rc = iett_findMatchingTopicsNodes(pThreadData,
                                              parent, true,
                                              topic,
                                              nextIndex, wildIndex, multiIndex+1,
                                              resultSet, maxNodes, nodeCount, result);

            if (rc != OK) goto mod_exit;
        }
        // Consider recursion if this node has children.
        else if (childrenExist)
        {
            // This substring is a wildcard node, so visit it's children with
            // the remaining topic string (but not grand-children etc)
            if (curSubstring == topic->wildcards[wildIndex])
            {
                for(chain=0;chain<table->capacity;chain++) // enumerate chains
                {
                    if (table->chains[chain].count > 0)
                    {
                        index = table->chains[chain].count;
                        entry = table->chains[chain].entries;

                        while(index>0) // enumerate entries
                        {
                            rc = iett_findMatchingTopicsNodes(pThreadData,
                                                              entry->value, false,
                                                              topic,
                                                              nextIndex, wildIndex+1, multiIndex,
                                                              resultSet, maxNodes, nodeCount, result);

                            if (rc != OK) goto mod_exit;

                            entry++;
                            index--;
                        }
                    }
                }
            }
            // The substring must be a non-wildcard node, so just find this
            // substring in the children and, if it exists, recurse into it.
            else
            {
                (void)ieut_getHashEntry(table,
                                        curSubstring,
                                        topic->substringHashes[curIndex],
                                        (void **)&node);

                if (NULL != node)
                {
                    rc = iett_findMatchingTopicsNodes(pThreadData,
                                                      node, false,
                                                      topic,
                                                      nextIndex, wildIndex, multiIndex,
                                                      resultSet, maxNodes, nodeCount, result);

                    if (rc != OK) goto mod_exit;
                }
            }
        }
    }

    // If an ancestor was a multiple wildcard node, we need to visit all of the
    // children of this node with the remaining string from that ancestor -
    // duplicates are removed at the end.
    if (multiMode && childrenExist)
    {
        for(chain=0;chain<table->capacity;chain++) // enumerate chains
        {
            if (table->chains[chain].count > 0)
            {
                index = table->chains[chain].count;
                entry = table->chains[chain].entries;

                while(index > 0) // enumerate entries
                {
                    rc = iett_findMatchingTopicsNodes(pThreadData,
                                                      entry->value, true,
                                                      topic,
                                                      curIndex, wildIndex, multiIndex,
                                                      resultSet, maxNodes, nodeCount, result);

                    if (rc != OK) goto mod_exit;

                    entry++;
                    index--;
                }
            }
        }
    }

mod_exit:

    // If we are now at the top level again, having unwound recursion, we
    // need to deal with any results (for example, traverse the hash set
    // or remove any non-matching system topics).
    if (curIndex == 0)
    {
        // If we had any failure, free up the results.
        if (rc != OK)
        {
            if (NULL != *result)
            {
                assert(resultSet == NULL);
                iemem_free(pThreadData, iemem_topicsQuery, *result);
                *result = NULL;
            }

            *nodeCount = 0;
        }
        // Otherwise update our counts if using a resultSet
        else if (resultSet != NULL)
        {
            assert(*result == NULL);
            *maxNodes = *nodeCount = resultSet->totalCount;
        }

        // If we now have some results, build the final list from it
        if (*nodeCount != 0)
        {
            uint32_t finalCount = *nodeCount;

            if (*maxNodes == finalCount)
            {
                iettTopicNode_t **newResult = iemem_realloc(pThreadData,
                                                            IEMEM_PROBE(iemem_topicsQuery, 2),
                                                            *result,
                                                            (finalCount+1)*sizeof(iettTopicNode_t *));

                if (NULL == newResult)
                {
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    if (*result != NULL) iemem_free(pThreadData, iemem_topicsQuery, *result);
                }

                *result = newResult;
            }

            // Finalize the results
            if (NULL != *result)
            {
                if (topic->destinationType == ismDESTINATION_TOPIC)
                {
                    // Gather the entries from the hash set
                    if (resultSet != NULL)
                    {
                        iettFindTopicNodeContext_t CallbackContext = {*result, 0};

                        ieut_traverseHashSet(pThreadData, resultSet, iett_findTopicsNodesCallback, &CallbackContext);

                        assert(CallbackContext.nodeIndex == finalCount);
                    }

                    // Remove any non-matching system nodes from the list
                    for(int32_t nodeIndex=0; nodeIndex<(int32_t)finalCount; nodeIndex++)
                    {
                        node = (*result)[nodeIndex];

                        // Want to examine the root of this topic string - so traverse to it.
                        while(node->parent != NULL && node->parent->parent != NULL)
                        {
                            node = node->parent;
                        }

                        char *topicSubstring = (char *)(node+1);

                        // If this is a system node, and the topic is not a system
                        // topic, we do not want to include the node in the results so
                        // move the last node down to this position
                        if ((topicSubstring[0] == ismENGINE_SYSTOPIC_PREFIX[0]) &&
                            (topic->sysTopicEndIndex == 0))
                        {
                            if (finalCount != 0) finalCount -= 1;

                            (*result)[nodeIndex] = (*result)[finalCount];

                            nodeIndex -= 1; // Need to now check this node
                        }
                    }
                }

                if (finalCount != 0)
                {
                    (*result)[finalCount] = NULL;
                }
                else
                {
                    iemem_free(pThreadData, iemem_topicsQuery, *result);
                    *result = NULL;
                }

                *nodeCount = finalCount;
            }
        }

        // Don't need the result table any more
        if (resultSet != NULL)
        {
            ieut_destroyHashSet(pThreadData, resultSet);
        }

        ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d, *nodeCount=%d\n", __func__, rc, *nodeCount);
    }

    return rc;
}

//****************************************************************************
/// @brief Add a subscription to the global engine topic tree.
///
/// @param[in]     topicString   Topic for this subscription
/// @param[in]     subscription  Subscription being added
/// @param[in]     pTran         Transaction in place for this request
/// @param[in]     inRecovery    Whether in the recovery stage or not
/// @param[in]     putRetained   Whether the subscription should be populated
///                              with matching retained msgs.
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark Assumes that the global engine topic tree has been initialised
///         by a call to iett_initEngineTopicTree.
///
/// @remark During recovery, the inRecovery flag should be set to true, this
///         implies that we do not bother to get any locks (we should be single
///         threaded at this time) and that we don't try to deliver retained
///         messages.
///
/// @remark When called within a transaction, responsibility for releasing
///         the subs lock is passed to a soft-log entry, to ensure that no
///         messages are delivered to it until it's addition has been completed,
///         so it is important to commit / rollback the transaction.
///
/// @see iett_initEngineTopicTree
/// @see iett_removeSubFromEngineTopic
//****************************************************************************
int32_t iett_addSubToEngineTopic(ieutThreadData_t *pThreadData,
                                 const char *topicString,
                                 ismEngine_Subscription_t *subscription,
                                 ismEngine_Transaction_t *pTran,
                                 bool inRecovery,
                                 bool putRetained)
{
    int32_t rc;
    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *wildcards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *multicards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    bool subsLocked = false;
    ietrSLE_Header_t *pAddSubSLEHdr = NULL;
    iereResourceSetHandle_t resourceSet = subscription->resourceSet;
    iereResourceSet_I64_StatType_t totalStatType = iett_getSubResSetTotalType(subscription);

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ieutTRACEL(pThreadData, subscription, ENGINE_FNC_TRACE, FUNCTION_ENTRY "topicString='%s', subscription=%p\n", __func__,
               topicString, subscription);

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = topicString;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

    rc = iett_analyseTopicString(pThreadData, &topic);

    if (rc != OK) goto mod_exit;

    // If there is a subscription name, we are going to add it to a clientId list
    assert(subscription->subName == NULL || subscription->clientId != NULL);

    // Adding a subscription requires write access to the subscription tree
    // during normal running, during recovery however we avoid the overhead
    // of getting and releasing the lock as we know we are single threaded.
    if (!inRecovery)
    {
        ismEngine_getRWLockForWrite(&tree->subsLock);
        subsLocked = true;
    }
    else
    {
        assert(pTran == NULL || pTran->TranState == ismTRANSACTION_STATE_COMMIT_ONLY);
    }

    // Check that this is not a duplicate of an existing named subscription...
    //
    // In theory, we don't need to do this test during recovery since it will have been done
    // during earlier runtime and we should not get subscriptions that failed this test back
    // when the server restarts. However, this test is deemed a useful protection since if
    // we ever did see the same subname come back we can diagnose it because we'll cause the
    // server to stop.
    //
    // For now, the performance of this is improved by the use of a hash of the subname before
    // performing a strcmp - if we need to improve startup further, this whole test only be performed
    // if inRecovery is not set.
    if (subscription->subName != NULL)
    {
        rc = iett_findClientSubscription(pThreadData,
                                         subscription->clientId,
                                         subscription->clientIdHash,
                                         subscription->subName,
                                         subscription->subNameHash,
                                         NULL);

        if (rc == OK)
        {
            ieutTRACEL(pThreadData, subscription, ENGINE_WORRYING_TRACE,
                       "Existing subscription named '%s' found.\n",
                       subscription->subName);
            rc = ISMRC_ExistingSubscription;
            goto mod_exit;
        }

        assert(rc == ISMRC_NotFound);

        rc = OK;
    }

    // We don't add an 'add subscription' SLE to a transaction during recovery,
    // and skip delivery of retained messages (since this is an existing subscription
    // and thus not expecting their re-delivery)
    if (!inRecovery)
    {
        // Only make the update transactionally if we have a transaction,
        // we remember the SLE that was added in pAddSubSLEHdr, so that if
        // the subscription is not ultimately added to the topic tree, we
        // can remove it again.
        if (NULL != pTran)
        {
            iettSLEAddSubscription_t SLE;

            SLE.subscription = subscription;
            SLE.lock = &tree->subsLock;

            // Note that we call this during Commit to release the lock as soon
            // as possible, or in post-rollback to ensure that the queue is not
            // deleted until everyone has stopped referencing it.
            rc = ietr_softLogAdd( pThreadData
                                , pTran
                                , ietrSLE_TT_ADDSUBSCRIPTION
                                , iett_SLEReplayAddSubscription
                                , NULL
                                , Commit | PostRollback
                                , &SLE
                                , sizeof(SLE)
                                , 0, 0
                                , &pAddSubSLEHdr );

            // Pass responsibility for releasing the lock to the transaction.
            if (rc == OK)
            {
                assert(subsLocked == true);
                subsLocked = false;
            }
            else
            {
                goto mod_exit;
            }
        }

        bool persistent = ((subscription->internalAttrs & iettSUBATTR_PERSISTENT) == iettSUBATTR_PERSISTENT);

        assert(rc == OK);
        assert(pTran != NULL);
        assert(pTran->fAsStoreTran == true);
        assert((persistent == false && pThreadData->ReservationState == Inactive) ||
               (persistent == true && pThreadData->ReservationState == Pending));

        // If we are expecting to put retained msgs, check whether there are any for this subscription
        if (putRetained)
        {
            rc = iett_putRetainedMessagesToSubscription(pThreadData,
                                                        tree,
                                                        &topic,
                                                        subscription,
                                                        &pTran,
                                                        false);
        }

        // All OK, update the store record for this subscription (as part of the
        // store transaction) to make it no longer 'CREATING' - note, we don't need to
        // include the updateRecord in the store reservation, but we do need to wait
        // until after iett_putRetainedMessagesToSubscription which might reserve store
        // resources for retained message references.
        if (rc == OK && persistent == true)
        {
            // Didn't do a reserve when putting retained messages, we need one to ensure that
            // we do a commit.
            if (pThreadData->ReservationState == Pending)
            {
                rc = ietr_reserve(pThreadData, pTran, 0, 0);
            }

            if (rc == OK)
            {
                assert(pThreadData->ReservationState == Active);

                ismStore_Handle_t hStoreDefn = ieq_getDefnHdl(subscription->queueHandle);

                assert(hStoreDefn != ismSTORE_NULL_HANDLE);

                rc = ism_store_updateRecord(pThreadData->hStream,
                                            hStoreDefn,
                                            ismSTORE_NULL_HANDLE,
                                            iestSDR_STATE_NONE,
                                            ismSTORE_SET_STATE);
            }
        }

        if (rc != OK) goto mod_exit;
    }
    else
    {
        assert(subsLocked == false);
        assert(putRetained == false);
    }

    iettSubsNode_t *subsNode = NULL;

    rc = iett_insertOrFindSubsNode(pThreadData, tree->subs, &topic, iettOP_ADD, &subsNode);

    if (rc == OK)
    {
        // Add the new subscription to the list of active subscriptions at this node.
        rc = iett_addActiveSubscriptionToSubsNode(pThreadData, subscription, tree, subsNode, inRecovery);

        if (rc != OK) goto mod_exit;

        subscription->node = subsNode;
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_updateInt64Stat(pThreadData, resourceSet, totalStatType, 1);
        ismEngine_serverGlobal.totalSubsCount++;

        // Update any special topic counters
        if ((subscription->internalAttrs & iettSUBATTR_SYSTEM_TOPIC) != 0)
        {
            if ((subscription->internalAttrs & iettSUBATTR_DOLLARSYS_DCN_TOPIC) != 0)
            {
                ismEngine_serverGlobal.totalDCNTopicSubs++;
            }
        }

        // If this is a named subscription, add it to the list of named subs
        // for this Client Id.
        if (subscription->subName != NULL)
        {
            rc = iett_addSubscription(pThreadData,
                                      subscription,
                                      subscription->clientId,
                                      subscription->clientIdHash);
        }

        if (rc != OK)
        {
            // If we don't have a transaction, we also need to remove the subscription from the node
            if (pTran == NULL)
            {
                assert(subsLocked == true);
                (void)iett_removeActiveSubscriptionFromSubsNode(pThreadData, subscription, tree, subsNode, inRecovery);
                subscription->node = NULL;
                iere_primeThreadCache(pThreadData, resourceSet);
                iere_updateInt64Stat(pThreadData, resourceSet, totalStatType, -1);
                ismEngine_serverGlobal.totalSubsCount--;
                if ((subscription->internalAttrs & iettSUBATTR_SYSTEM_TOPIC) != 0)
                {
                    if ((subscription->internalAttrs & iettSUBATTR_DOLLARSYS_DCN_TOPIC) != 0)
                    {
                        ismEngine_serverGlobal.totalDCNTopicSubs--;
                    }
                }
            }

            goto mod_exit;
        }

        // Add this subscription into the list of subscriptions
        subscription->prev = NULL;
        subscription->next = tree->subscriptionHead;
        if (tree->subscriptionHead != NULL)
        {
            tree->subscriptionHead->prev = subscription;
        }
        tree->subscriptionHead = subscription;

        // Increment the count of subscriptions performing selection
        if (subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SELECTION_MASK)
        {
            subsNode->activeSelection++;
        }

        // Increment the count of subscriptions performing selection if this is an import so
        // that anyone publishing will avoid this subscription until the import is done.
        if (subscription->internalAttrs & iettSUBATTR_IMPORTING)
        {
            subsNode->activeSelection++;
        }

        // Increment the count of multiple multi-level wildcard subscriptions if appropriate
        if (subsNode->nodeFlags & iettNODE_FLAG_BRANCH_MULTIMULTI)
        {
            tree->multiMultiSubs++;
        }

        // Increment the counts of subscriptions up the tree
        do
        {
            subsNode->totalSubsCount++;
        }
        while((subsNode = subsNode->parent) != NULL);

        tree->subsUpdates++;

        iettNewSubCreationData_t *creationData = iett_getNewSubCreationData(subscription);
        if (creationData != NULL)
        {
            creationData->subsUpdatesValue = tree->subsUpdates;
        }
    }

mod_exit:

    // Something went wrong before we added the subscription to the topic tree, but
    // after we'd added the SLE to the transaction (which we have to do early, to
    // ensure the ordering of putting retained & deleting the subscription is correct
    // during rollback).
    //
    // We need to remove the SLE from the transaction, and take responsibility for
    // releasing the lock.
    if (rc != OK && pAddSubSLEHdr != NULL && subscription->node == NULL)
    {
        assert(pTran != NULL);
        assert(inRecovery == false);
        (void)ietr_softLogRemove(pThreadData, pTran, pAddSubSLEHdr);
        subsLocked = true; // take unlock responsibility back
    }

    // Release the subscription tree lock if we are holding it.
    if (subsLocked) ismEngine_unlockRWLock(&tree->subsLock);

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
/// @brief Force the deletion of pending subscriptions on a given topic.
///
/// @param[in]     topicString    The topic to force
///
/// @remark There is a window in which the deletion of a subscription from
///         a topic node needed to go asynchronous, but that there is no call
///         to iett_releaseSubscriberList to actually cause the deletion.
///
///         With no further action, a subscription that went into pending
///         deletion could end up constantly pending, because no call to
///         iett_releaseSubscriberList occurs to delete it - we can use this
///         function (which just builds a subscriber list for the requested
///         topic node, then releases it) to ensure that _someone_ will call
///         release for the specified node.
///
/// @see iett_removeSubFromEngineTopic
/// @see iett_releaseSubscriberList
//****************************************************************************
void iett_forcePendingDeletions(ieutThreadData_t *pThreadData,
                                char *topicString)
{
    iettSubsNode_t *subsNode[2];
    iettSubscriberList_t forceSublist = {0};

    ieutTRACEL(pThreadData, topicString,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "topic='%s'\n", __func__, topicString);

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *wildcards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *multicards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = topicString;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

    if (iett_analyseTopicString(pThreadData, &topic) != OK) goto mod_exit;

    // Create a partial subscriber list for the requested node.
    // Note: This subscriber list cannot be used to publish messages, it
    //       contains subscriber nodes only, no subscriptions.
    ismEngine_getRWLockForRead(&tree->subsLock);

    if (iett_insertOrFindSubsNode(pThreadData, tree->subs, &topic, iettOP_FIND, &(subsNode[0])) == OK)
    {
        forceSublist.subscriberNodes = (iettSubsNodeHandle_t *)&subsNode;
        __sync_fetch_and_add(&(subsNode[0]->useCount), 1);
    }

    ismEngine_unlockRWLock(&tree->subsLock);

mod_exit:

    // Free any topic string analysis data
    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
        if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
        if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);
    }

    // We found a node, finish off the pseudo subscriber list and issue release
    if (forceSublist.subscriberNodes != NULL)
    {
        subsNode[1] = NULL;
        forceSublist.subscriberNodeCount = 1;
        forceSublist.usingCachedArrays = true;

        iett_releaseSubscriberList(pThreadData, &forceSublist);
    }

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

//****************************************************************************
/// @brief Remove a subscription from the global engine topic tree and either
///        free the associated resources or add it to a list of resources to
///        be free'd when the node they are hung off is not in-use.
///
/// @param[in]     subscription   Subscription being removed
/// @param[in]     flags          Flags from iettFLAG_REMOVE_SUB_* set
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark Assumes that the global engine topic tree has been initialised
///         by a call to iett_initEngineTopicTree.
///
/// @remark During recovery, the inRecovery flag should be set to true, this
///         implies that we do not bother to get any locks.
///
/// @remark If we are being called as the result of a rollback of an add
///         Sub, alreadyLocked will be set to indicate that the lock is held
///         (it is released once the SLE is committed or rolled back).
///
/// @see iett_initEngineTopicTree
/// @see iett_addSubToEngineTopic
//****************************************************************************
int32_t iett_removeSubFromEngineTopic(ieutThreadData_t *pThreadData,
                                      ismEngine_Subscription_t *subscription,
                                      uint32_t flags)
{
    int32_t         rc = OK;
    iettSubsNode_t *removedSubtree = NULL;
    bool            subscriptionNotInUse = false;
    char           *pendingDeletionTopicString = NULL;
    bool            cleaningUp = ((flags & iettFLAG_REMOVE_SUB_ROLLBACK_ADD) != 0);
    iereResourceSetHandle_t resourceSet = subscription->resourceSet;
    iereResourceSet_I64_StatType_t totalStatType = iett_getSubResSetTotalType(subscription);

    ieutTRACEL(pThreadData, subscription, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "subscription=%p cleaningUp=%d\n", __func__,
               subscription, (int)cleaningUp);

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    // Removing a subscription requires write access to the subscription tree
    // but we only need to acquire the lock if the lock has not already been
    // acquired.
    if ((flags & iettFLAG_REMOVE_SUB_ALREADY_LOCKED) == 0)
    {
        ismEngine_getRWLockForWrite(&tree->subsLock);
    }

    // If this subscription now has a non-zero useCount, then it means that someone
    // has their finger on it - we need to let them release it, at which point it will
    // get removed - this call will effectively go asynchronous
    if (!cleaningUp && subscription->useCount != 0)
    {
        assert(subscriptionNotInUse == false);
        goto mod_exit;
    }

    iettSubsNode_t *node = subscription->node;

    if (NULL != node  && node->activeSubs.count != 0)
    {
        // Remove the subscription from the list of active subscriptions at this node.
        rc = iett_removeActiveSubscriptionFromSubsNode(pThreadData, subscription, tree, node, false);

        if (rc == OK)
        {
            // Whenever a list of subscribers is requested, the topic tree
            // nodes involved have their useCount values incremented. If
            // this node is currently involved, we cannot release the
            // the subscription reference yet since it will potentially be
            // accessed. We _can_ release the subscription reference though
            // if the caller is rolling back the addition of a subscription,
            // since the lock will have been held up to this point, so the subscription
            // cannot be a part of any subscriber lists - by taking this into account,
            // we can avoid the possibility that we will need to allocate storage for
            // the list of deleted subscriptions.
            //
            // In this case, we instead move the subscription to a list of
            // deleted subs, so that it won't be included in any further
            // lists (only active subs are included in lists).
            //
            // When this node's use count drops to zero, we run the list of
            // deleted subs, releasing the subscription reference.
            if (node->useCount > 0 && !cleaningUp)
            {
                rc = iett_addSubscriptionToSubsNode(pThreadData,
                                                    subscription,
                                                    &node->delPendSubs);

                if (rc != OK) goto mod_exit;

                // Remember the topic string so that we can force deletion later
                // cppcheck-suppress *
                if ((pendingDeletionTopicString = alloca(strlen(node->topicString)+1)) != NULL)
                {
                    strcpy(pendingDeletionTopicString, node->topicString);
                }

            }
            // The node is not in use, so we can free the subscription once the
            // lock has been released.
            else
            {
                // Sanity check, if we are removing a subscription that was just added that
                // we expect to have been holding the lock, so we know no-one can be referring
                // to this subscription.
                assert((flags & iettFLAG_REMOVE_SUB_ROLLBACK_ADD) == 0 ||
                       (flags & iettFLAG_REMOVE_SUB_ALREADY_LOCKED) != 0);

                subscriptionNotInUse = true;
            }

        }
        else if (rc == ISMRC_NotFound)
        {
            subscriptionNotInUse = true;
        }

        // If cleaning up a subscription addition, ensure that the subscription is
        // removed from the client list
        if (cleaningUp && subscription->subName != NULL)
        {
            iett_removeSubscription(pThreadData,
                                    subscription,
                                    subscription->clientId,
                                    subscription->clientIdHash);
        }

        // Remove this subscription from the linked list of subscriptions
        if (subscription->prev != NULL)
        {
            subscription->prev->next = subscription->next;
        }
        else
        {
            tree->subscriptionHead = subscription->next;
        }

        if (subscription->next != NULL)
        {
            subscription->next->prev = subscription->prev;
        }

        iettSubsNode_t *subsNode = node;

        // Decrement the count of subscriptions performing selection
        if (subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SELECTION_MASK)
        {
            subsNode->activeSelection--;
        }

        // If this subscription has the IMPORTING flag set, it will also be contributing
        // an _additional_ increment on the active selection count
        if (subscription->internalAttrs & iettSUBATTR_IMPORTING)
        {
            subsNode->activeSelection--;
        }

        // Decrement the count of multiple multi-level wildcard subscriptions if appropriate
        if (subsNode->nodeFlags & iettNODE_FLAG_BRANCH_MULTIMULTI)
        {
            assert(tree->multiMultiSubs > 0);
            tree->multiMultiSubs--;
        }

        // Decrement the counts of subscriptions up the tree
        do
        {
            subsNode->totalSubsCount--;
        }
        while((subsNode = subsNode->parent) != NULL);

        // Update the overall count(s) of subscriptions
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_updateInt64Stat(pThreadData, resourceSet, totalStatType, -1);
        assert(ismEngine_serverGlobal.totalSubsCount > 0);
        ismEngine_serverGlobal.totalSubsCount--;

        // Update any special topic counters
        if ((subscription->internalAttrs & iettSUBATTR_SYSTEM_TOPIC) != 0)
        {
            if ((subscription->internalAttrs & iettSUBATTR_DOLLARSYS_DCN_TOPIC) != 0)
            {
                assert(ismEngine_serverGlobal.totalDCNTopicSubs > 0);
                ismEngine_serverGlobal.totalDCNTopicSubs--;
            }
        }

        tree->subsUpdates++;

        // If this node is inactive, clear up whatever we now can.
        if (iettQUICK_INACTIVE_SUBSNODE_TEST(node))
        {
            // Remove any nodes that are inactive from the subscription tree
            iett_removeInactiveSubsNodesFromEngineTopicTree(pThreadData, node, &removedSubtree);

            // We removed some nodes, invalidate cached node lists
            if (removedSubtree != NULL)
            {
                tree->cacheUpdates++;
            }
        }
    }

mod_exit:

    if ((flags & iettFLAG_REMOVE_SUB_ALREADY_LOCKED) == 0)
    {
        ismEngine_unlockRWLock(&tree->subsLock);
    }

    // If a subtree was removed from the tree, destroy it now.
    if (removedSubtree != NULL)
    {
        iett_destroySubsTreeCallback(pThreadData, NULL, 0, removedSubtree, NULL);
    }

    // Now that we have released the lock, we can free the subscription if
    // it is not still in use - if it is still in use the last user will
    // free it when it cleans up.
    if (rc == OK)
    {
        if (subscriptionNotInUse)
        {
            iett_freeSubscription(pThreadData, subscription, false);
        }
        // Attempt to force the pending deletion of subscriptions
        else if (pendingDeletionTopicString != NULL)
        {
            iett_forcePendingDeletions(pThreadData, pendingDeletionTopicString);
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Append all nodes containing active subscriptions from the given node
///        including multi-level and wildcard child nodes.
///
/// @param[in]     node             Node
/// @param[in,out] maxNodes         Space available in the array of topic nodes
/// @param[in,out] nodeCount        Count of nodes containing subscribers found
/// @param[in]     result           Current array of nodes
///
/// @return Pointer to the array of nodes (re-allocated if required) containing
///         the newly appended nodes, NULL if a problem was encountered.
///
/// @remark The lock covering the nodes being added must be held for read.
//****************************************************************************
int32_t iett_addActiveSubsNodes(ieutThreadData_t *pThreadData,
                                iettSubsNode_t    *node,
                                uint32_t          *maxNodes,
                                uint32_t          *nodeCount,
                                iettSubsNode_t  ***result)
{
    int32_t          rc = OK;
    uint32_t         entryNodeType = node->nodeFlags & iettNODE_FLAG_TYPE_MASK;

    while(NULL != node)
    {
        // Include nodes that either have active subscribers or for which stats collection is active
        if (node->activeSubs.count != 0 ||
            (node->stats != NULL && node->stats->topicStats.ResetTime != 0))
        {
            if (*nodeCount == *maxNodes)
            {
                uint32_t newMax = (*maxNodes) + iettFIND_SUBSCRIBER_RESULTS_INCREMENT;

                iettSubsNode_t **newResult;

                newResult = iemem_realloc(pThreadData,
                                          IEMEM_PROBE(iemem_subsQuery, 1),
                                          *result,
                                          newMax * sizeof(iettSubsNode_t *));

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

        // For non-multicard nodes on entry, collect any multicard subscribers lower
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
/// @brief Find the nodes in a subscription tree which match a given topic
///        string appending them to an array of nodes.
///
/// @param[in]     parent            Parent node under which to start searching.
/// @param[in]     topic             Topic analysis to use
/// @param[in]     curIndex          Current index into the topic analysis arrays
/// @param[in]     usingCachedArray  Whether the array being used comes from a
///                                  cache (if it does we reallocate it but do not
///                                  free it).
/// @param[in,out] maxNodes          Space available in the array of topic nodes
/// @param[out]    nodeCount         Count of nodes containing subscribers found.
/// @param[in]     result            Current array of nodes
///
/// @return Pointer to the array of nodes containing subscribers, NULL if a
////        problem was encountered.
///
/// @remark The lock covering the nodes being added must be held for read.
///
/// @remark This function calls itself recursively for child nodes.
///
/// @see iett_getSubscriberList
//****************************************************************************
int32_t iett_findMatchingSubsNodes(ieutThreadData_t *pThreadData,
                                   const iettSubsNode_t *parent,
                                   const iettTopic_t    *topic,
                                   const uint32_t        curIndex,
                                   const bool            usingCachedArray,
                                   uint32_t             *maxNodes,
                                   uint32_t             *nodeCount,
                                   iettSubsNode_t     ***result)
{
    int32_t rc = OK;
    iettSubsNode_t *node = NULL;
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
                rc = iett_addActiveSubsNodes(pThreadData, node, maxNodes, nodeCount, result);
            }
            else
            {
                rc = iett_findMatchingSubsNodes(pThreadData,
                                                node,
                                                topic,
                                                index,
                                                usingCachedArray,
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
                rc = iett_addActiveSubsNodes(pThreadData, node, maxNodes, nodeCount, result);
            }
            else
            {
                rc = iett_findMatchingSubsNodes(pThreadData,
                                                node,
                                                topic,
                                                index,
                                                usingCachedArray,
                                                maxNodes, nodeCount, result);
            }

            if (rc != OK) goto mod_exit;
        }

        // Look for multi-level wildcards (multicards).
        if (NULL != parent->multicardChild)
        {
            index = curIndex;

            node = parent->multicardChild;

            rc = iett_addActiveSubsNodes(pThreadData, node, maxNodes, nodeCount, result);

            if (rc != OK) goto mod_exit;

            do
            {
                rc = iett_findMatchingSubsNodes(pThreadData,
                                                node,
                                                topic,
                                                index++,
                                                usingCachedArray,
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
            if (!usingCachedArray)
            {
                iemem_free(pThreadData, iemem_subsQuery, *result);
                *result = NULL;
            }

            *nodeCount = 0;
        }
        // Remove any duplicates caused by wild card processing.
        else
        {
            uint32_t finalCount = *nodeCount;
            iettSubsNode_t **finalResult;

            // Ensure subscriberNodes array is NULL terminated.
            if (*maxNodes == finalCount)
            {
                finalResult = iemem_realloc(pThreadData,
                                            IEMEM_PROBE(iemem_subsQuery, 2),
                                            *result,
                                            (finalCount+1)*sizeof(iettSubsNode_t *));

                if (NULL == finalResult)
                {
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);

                    if (!usingCachedArray)
                    {
                        iemem_free(pThreadData, iemem_subsQuery, *result);
                        *result = NULL;
                    }

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

                // If there are no subscriptions in the topic tree which specify multiple multi-level
                // wildcards, then there will be no duplicates in the results - otherwise there may
                // be and we should remove them.
                if (ismEngine_serverGlobal.maintree->multiMultiSubs > 0)
                {
                    uint32_t nodeIndex = 0;
                    iettSubsNode_t *possibleDuplicate;

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
/// @brief Get an array of subscribers to a given topic from the global
///        engine topic tree.
///
/// @param[in,out] subscriberList  Pointer to a subscriber list to be filled in.
///
/// @return OK if subscribers found, ISMRC_NotFound if not or another
///         ISMRC_ value on error.
///
/// @remark The subscriberList must be pre-filled with a topic string.
///
/// @remark The topic specified must be a literal, if a wildcard is specified
///         an error (ISMRC_DestNotValid) is returned.
///
/// @remark Upon OK return the subscribers in the list have had their use
///         count incremented, meaning that they will not be removed while the
///         operation is being performed, also, if the message passed in is a
///         retained message we will hold the subscribers lock.
///
///         As a result once the caller no longer needs to refer to the list, it
///         must be released by either a call to to iett_releaseSubscriberList or
///         a pair of calls, one to iett_SLEReplayReleaseNodes and one to to
///         iett_freeSubscriberList.
///
/// @see iett_releaseSubscriberList iett_SLEReplayReleaseNodes iett_freeSubscriberList
//****************************************************************************
int32_t iett_getSubscriberList(ieutThreadData_t *pThreadData, iettSubscriberList_t *subscriberList)
{
    int32_t                    rc = OK;
    iettTopicTree_t           *tree = ismEngine_serverGlobal.maintree;
    bool                       foundInCache = false;
    uint32_t                   topicStringHash = 0;
    iettTopic_t                topic;
    const char                *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t                   substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t                   localNodeCount;
    ieutHashTable_t           *sublistCache = pThreadData->sublistCache;
    bool                       cachesMatch = (pThreadData->cacheUpdates == tree->cacheUpdates);

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_ENTRY "topicString='%s'\n", __func__,
               subscriberList->topicString);

    topic.topicStringCopy = NULL;

    assert(subscriberList->subscriberNodeCount == 0 && subscriberList->subscriberCount == 0);

retry_cacheLookup:

    localNodeCount = 0;

    if (NULL != sublistCache)
    {
        if (cachesMatch)
        {
            topicStringHash = iett_generateTopicStringHash(subscriberList->topicString);

            iettSubsNode_t **subscriberNodes = NULL;

            rc = ieut_getHashEntry(sublistCache,
                                   subscriberList->topicString,
                                   topicStringHash,
                                   (void**)&subscriberNodes);

            if (rc == OK) foundInCache = true;

            if (NULL != subscriberNodes)
            {
                // Count the nodes
                while(NULL != subscriberNodes[localNodeCount])
                {
                    localNodeCount++;
                }

                // Enough space in the subscriberList for nodes?
                if (localNodeCount > subscriberList->subscriberNodeCapacity)
                {
                    iettSubsNode_t **newSubscriberNodes;

                    newSubscriberNodes = iemem_realloc(pThreadData,
                                                       IEMEM_PROBE(iemem_subsQuery, 3),
                                                       subscriberList->subscriberNodes,
                                                       (localNodeCount+1)*sizeof(iettSubsNode_t *));

                    if (NULL == newSubscriberNodes)
                    {
                        rc = ISMRC_AllocateError;
                        ism_common_setError(rc);
                        goto mod_exit_no_release;
                    }

                    subscriberList->subscriberNodes = newSubscriberNodes;
                    subscriberList->subscriberNodeCapacity = localNodeCount;
                }

                memcpy(subscriberList->subscriberNodes, subscriberNodes, (localNodeCount+1)*sizeof(iettSubsNode_t *));
            }
        }
        else
        {
            ieut_clearHashTable(pThreadData, sublistCache);
            foundInCache = false;
        }
    }

    // If we didn't find this topic in the cache we will need an analysis of the topic
    // string in order to call other functions - do this now, before taking any locks.
    if (!foundInCache)
    {
        // If we have not already analysed the topic string, we need to do so now - this
        // is indicated by the topicStringCopy being NULL.
        if (NULL == topic.topicStringCopy)
        {
            memset(&topic, 0, sizeof(topic));

            topic.destinationType = ismDESTINATION_TOPIC;
            topic.topicString = subscriberList->topicString;
            topic.substrings = substrings;
            topic.substringHashes = substringHashes;
            topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

            rc = iett_analyseTopicString(pThreadData, &topic);

            if (rc != OK) goto mod_exit_no_release;

            // We do not accept wildcards on the topic specified for a
            // subscriber list at the moment.
            assert((topic.wildcardCount == 0) && (topic.multicardCount == 0));
        }
    }

    // Get the subscription tree lock for read
    ismEngine_getRWLockForRead(&tree->subsLock);

    // Now that we are holding the lock on the tree, confirm that our cache is up to date,
    // if it is not, unlock and retry the lookup, which results in our cache being cleared.
    if (NULL != pThreadData && (pThreadData->cacheUpdates != tree->cacheUpdates))
    {
        pThreadData->cacheUpdates = tree->cacheUpdates;
        ismEngine_unlockRWLock(&tree->subsLock);
        cachesMatch = false;
        goto retry_cacheLookup;
    }

    // Not found in the cache, use iett_findMatchingSubsNodes to walk the tree.
    if (!foundInCache)
    {
        //assert(sameAsPrevious == false);
        rc = iett_findMatchingSubsNodes(pThreadData,
                                        tree->subs,
                                        &topic,
                                        0,
                                        subscriberList->usingCachedArrays,
                                        &(subscriberList->subscriberNodeCapacity),
                                        &localNodeCount,
                                        &(subscriberList->subscriberNodes));
    }

    // If we have subscriber nodes from either the cache or iett_findMatchinSubsNodes
    // we need to increment the use count and extract subscribers from them.
    if (localNodeCount != 0)
    {
        uint32_t activeSelection = 0;

        // Now need to make sure we remember these nodes
        subscriberList->subscriberNodeCount = localNodeCount;

        // Increment the useCount on each node, calculating subscriberCount
        iettSubsNode_t **subsNodePos = subscriberList->subscriberNodes;
        do
        {
            iettSubsNode_t *subsNode = *subsNodePos;
            subscriberList->subscriberCount += subsNode->activeSubs.count;
            subsNode->listCount++;
            activeSelection += subsNode->activeSelection;
            __sync_fetch_and_add(&subsNode->useCount, 1);
        }
        while(*(++subsNodePos) != NULL);

        // If there are subscribers, allocate space and copy from each node
        if (subscriberList->subscriberCount != 0)
        {
            subscriberList->requestSelection = (activeSelection != 0);

            // More than our list has capacity to contain, reallocate
            if (subscriberList->subscriberCount > subscriberList->subscriberCapacity)
            {
                ismEngine_Subscription_t **newSubscribers;

                newSubscribers = iemem_realloc(pThreadData,
                                               IEMEM_PROBE(iemem_subsQuery, 4),
                                               subscriberList->subscribers,
                                               ((subscriberList->subscriberCount)+1)*sizeof(ismEngine_Subscription_t *));

                // Handle an allocate failure
                if (NULL == newSubscribers)
                {
                    //***subscriberList->subscriberCount = 0;
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    goto mod_exit;
                }

                subscriberList->subscribers = newSubscribers;
                subscriberList->subscriberCapacity = subscriberList->subscriberCount;
            }

            // Copy the subscribers to the list
            subsNodePos = subscriberList->subscriberNodes;
            ismEngine_Subscription_t **subscriberPos = subscriberList->subscribers;
            do
            {
                iettSubsNode_t *subsNode = *subsNodePos;

                if (subsNode->activeSubs.count != 0)
                {
                    memcpy(subscriberPos,
                           subsNode->activeSubs.list,
                           subsNode->activeSubs.count * sizeof(ismEngine_Subscription_t *));
                    subscriberPos += subsNode->activeSubs.count;
                }
            }
            while(*(++subsNodePos) != NULL);
            *subscriberPos = NULL;
        }
    }

mod_exit:

    // Remember the tree's subsUpdates value for this publish
    if (rc == OK) subscriberList->publishSUV = tree->subsUpdates;

    // Release the subscription tree lock
    ismEngine_unlockRWLock(&tree->subsLock);

    // If this result did not come from the cache, update the cache
    if (!foundInCache && NULL != sublistCache)
    {
        // If this list has only one subscriber associated with it it suggests
        // this is either a 'point-to-point' subscription or a 'fan-in' subscription.
        // Ideally, we want to cache 'fan-in' subscriptions, and not 'point-to-point'
        // ones - but we are happy to cache both if we have capacity.

        if (// We have ample capacity...
            sublistCache->totalCount < sublistCache->capacity ||
            // There are >1 subscribers (suggesting fan-out)
            subscriberList->subscriberCount > 1 ||
            // There is a single node involved, and it looks like a non-wildcarded fan-in.
            (subscriberList->subscriberNodeCount == 1 &&
             (subscriberList->subscriberNodes[0]->nodeFlags & iettNODE_FLAG_TYPE_MASK) == iettNODE_FLAG_NORMAL &&
             subscriberList->subscriberNodes[0]->listCount > iettFAN_IN_LISTCOUNT_BOUNDARY))
        {
            if (topicStringHash == 0)
            {
                topicStringHash = iett_generateTopicStringHash(subscriberList->topicString);
            }

            (void)ieut_putHashEntry(pThreadData,
                                    sublistCache,
                                    ieutFLAG_DUPLICATE_ALL,
                                    subscriberList->topicString,
                                    topicStringHash,
                                    subscriberList->subscriberNodeCount ? subscriberList->subscriberNodes : NULL,
                                    ((subscriberList->subscriberNodeCount)+1) * sizeof(iettSubsNode_t *));
        }
    }

mod_exit_no_release:

    assert(rc != ISMRC_NotFound);

    if (rc == OK && subscriberList->subscriberCount == 0 && subscriberList->subscriberNodeCount == 0)
    {
        rc = ISMRC_NotFound;  // None found - not an error

        assert(subscriberList->publishSUV != 0); // Reliant on this for lateSubscribers
    }

    // If there was a failure, ensure we release resources, our caller will not
    if (rc != OK)
    {
        iett_releaseSubscriberList(pThreadData, subscriberList);
    }

    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, subscriberCount=%d\n", __func__, rc, subscriberList->subscriberCount);

    return rc;
}

//****************************************************************************
/// @brief Free any free-able memory pointed to from the subscriber list
///
/// @param[in]     subscriberList  List of subscribers to be cleaned up
///
/// @see iett_getSubscriberList
//****************************************************************************
void iett_freeSubscriberList(ieutThreadData_t *pThreadData,
                             iettSubscriberList_t *subscriberList)
{
    // Free up the subscriber nodes if they did not come from this requester's cache.
    if (!subscriberList->usingCachedArrays && subscriberList->subscriberNodeCount != 0)
    {
        if (NULL != subscriberList->subscriberNodes) iemem_free(pThreadData, iemem_subsQuery, subscriberList->subscriberNodes);
        if (NULL != subscriberList->subscribers) iemem_free(pThreadData, iemem_subsQuery, subscriberList->subscribers);
    }

    if (subscriberList->remoteServerCount != 0)
    {
        // TODO: Consider caching
        iemem_free(pThreadData, iemem_subsQuery, subscriberList->remoteServers);
    }
    else
    {
        assert(subscriberList->remoteServers == NULL);
    }
}

//****************************************************************************
/// @brief Release a list of subscribers previously returned by
///        iett_getSubscriberList and free the list.
///
/// @param[in]     subscriberList  List of subscribers to be released.
///
/// @remark As we release nodes, if we see deleted subscriptions we free them.
///
/// @see iett_getSubscriberList
//****************************************************************************
void iett_releaseSubscriberList(ieutThreadData_t *pThreadData, iettSubscriberList_t *subscriberList)
{
    if (NULL != subscriberList)
    {
        // We will not be changing the subscriberList yet - give the compiler the hint
        const iettSubscriberList_t *constSubList = (const iettSubscriberList_t *)subscriberList;

        iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

        // Tidy up subscriber nodes
        if (constSubList->subscriberNodeCount != 0)
        {
            iettSubsNode_t **subsNodePos = constSubList->subscriberNodes;

            do
            {
                char *pendingDeletionTopic;

                iettSubsNode_t *subsNode = *subsNodePos;

                if (subsNode->delPendSubs.count != 0)
                {
                    pendingDeletionTopic = ism_common_strdup(ISM_MEM_PROBE(ism_memory_engine_misc,1000),subsNode->topicString);
                }
                else
                {
                    pendingDeletionTopic = NULL;
                }

                uint32_t oldCount = __sync_fetch_and_sub(&subsNode->useCount, 1);

                assert(oldCount != 0); // useCount just went negative!

                // We were the last user of this node and we think there are pending
                // deletions to be considered
                if (pendingDeletionTopic != NULL)
                {
                    if (oldCount == 1)
                    {
                        iett_performPendingSubscriptionDeletions(pThreadData, tree, pendingDeletionTopic);
                    }

                    ism_common_free(ism_memory_engine_misc,pendingDeletionTopic);
                }
            }
            while(NULL != *(++subsNodePos));
        }

        // We can now release the use count on any remote servers
        if (constSubList->remoteServerCount != 0)
        {
            ismEngine_RemoteServer_t **remoteServerPos = constSubList->remoteServers;

            do
            {
                ismEngine_RemoteServer_t *remoteServer = *remoteServerPos;

                (void)iers_releaseServer(pThreadData, remoteServer);
            }
            while(NULL != *(++remoteServerPos));
        }

        iett_freeSubscriberList(pThreadData, subscriberList);
    }

    ieutTRACEL(pThreadData, subscriberList,  ENGINE_NORMAL_TRACE, "%s subscriberList %p\n", __func__, subscriberList);

    return;
}

//****************************************************************************
/// @brief Deal with any subscriptions that are pending deletion
//****************************************************************************
void iett_performPendingSubscriptionDeletions(ieutThreadData_t *pThreadData,
                                              iettTopicTree_t *tree,
                                              char *pendingDeletionTopic)
{
    ieutTRACEL(pThreadData, tree, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pendingDeletionTopic='%s'\n", __func__,
               pendingDeletionTopic);

    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *wildcards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *multicards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t deletedCount = 0;

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = pendingDeletionTopic;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

    if (iett_analyseTopicString(pThreadData, &topic) != OK) goto skip_cleanup;

    ismEngine_Subscription_t **deletedList = NULL;
    iettSubsNode_t *subsNode;

    // Get the subscription tree lock for write to collect deleted subs
    ismEngine_getRWLockForWrite(&tree->subsLock);

    // Now we have the lock, check that we still have cleanup to do by
    // first finding the node (it may have been removed by now) and then
    // checking that we still have work to do.
    if (iett_insertOrFindSubsNode(pThreadData, tree->subs, &topic, iettOP_FIND, &subsNode) == OK)
    {
        assert(subsNode != NULL);

        // Now we hold the lock and know the subscription node is valid,
        // check that we should still clear up
        if (subsNode->useCount == 0 && subsNode->delPendSubs.count > 0)
        {
            deletedCount = subsNode->delPendSubs.count;
            deletedList = subsNode->delPendSubs.list;

            subsNode->delPendSubs.count = 0;
            subsNode->delPendSubs.max = 0;
            subsNode->delPendSubs.list = NULL;
        }
    }

    ismEngine_unlockRWLock(&tree->subsLock);

    // We have some subscriptions which are pending deletion from this node
    // so let's free them up now.
    if (deletedCount > 0)
    {
        for(uint32_t i=0; i<deletedCount; i++)
        {
            iett_freeSubscription(pThreadData, deletedList[i], false);
        }

        iemem_free(pThreadData, iemem_subsTree, deletedList);
    }

skip_cleanup:

    // Free any topic string analysis data
    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
        if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
        if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);
    }

    ieutTRACEL(pThreadData, deletedCount,  ENGINE_FNC_TRACE, FUNCTION_EXIT "deletedCount=%d\n", __func__, deletedCount);
}

//****************************************************************************
/// @brief Create and initialize a new topic tree
///
/// @return Pointer to a newly created topic tree, NULL on failure.
///
/// @see iett_initEngineTopicTree
//****************************************************************************
iettTopicTree_t *iett_createTopicTree(ieutThreadData_t *pThreadData)
{
    int osrc = 0;
    int32_t rc = OK;
    iettTopicTree_t *tree = NULL;
    pthread_rwlockattr_t rwlockattr_init;

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    osrc = pthread_rwlockattr_init(&rwlockattr_init);

    if (osrc) goto mod_exit;

    osrc = pthread_rwlockattr_setkind_np(&rwlockattr_init, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);

    if (osrc) goto mod_exit;

    // Create the tree structure
    tree = iemem_calloc(pThreadData, IEMEM_PROBE(iemem_subsTree, 4), 1, sizeof(iettTopicTree_t));

    if (NULL == tree)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(tree->strucId, iettTOPIC_TREE_STRUCID, 4);

    // Create the subscription tree
    osrc = pthread_rwlock_init(&tree->subsLock, &rwlockattr_init);

    if (osrc) goto mod_exit;

    tree->subs = iemem_calloc(pThreadData, IEMEM_PROBE(iemem_subsTree, 5), 1, sizeof(iettSubsNode_t));

    if (NULL == tree->subs)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(tree->subs->strucId, iettSUBSCRIPTION_NODE_STRUCID, 4);
    tree->subs->nodeFlags = iettNODE_FLAG_TREE_ROOT;

    // Create the remote server tree
    osrc = pthread_rwlock_init(&tree->remoteServersLock, &rwlockattr_init);

    if (osrc) goto mod_exit;

    tree->remoteServers = iemem_calloc(pThreadData, IEMEM_PROBE(iemem_remoteServers, 8), 1, sizeof(iettRemSrvNode_t));

    if (NULL == tree->remoteServers)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(tree->remoteServers->strucId, iettREMOTE_SERVER_NODE_STRUCID, 4);
    tree->remoteServers->nodeFlags = iettNODE_FLAG_TREE_ROOT;

    // Create the hash of named subscriptions
    osrc = pthread_rwlock_init(&tree->namedSubsLock, &rwlockattr_init);

    if (osrc) goto mod_exit;

    rc = ieut_createHashTable(pThreadData,
                              iettINITIAL_NAMEDSUB_HASH_CAPACITY,
                              iemem_namedSubs,
                              &tree->namedSubs);

    if (rc != OK) goto mod_exit;

    // Create the topic information tree
    osrc = pthread_rwlock_init(&tree->topicsLock, &rwlockattr_init);

    if (osrc) goto mod_exit;

    // Initialize retUpdates and subsUpdates to 1, so we can identify recovered
    // subscriptions because they will have a subscription specific retUpdatesValue of 0
    // and subsUpdatesValue of 0.
    tree->retUpdates = 1;
    tree->subsUpdates = 1;

    tree->topics = iemem_calloc(pThreadData, IEMEM_PROBE(iemem_topicsTree, 2), 1, sizeof(iettTopicNode_t));

    if (NULL == tree->topics)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(tree->topics->strucId, iettTOPIC_NODE_STRUCID, 4);
    tree->topics->nodeFlags = iettNODE_FLAG_TREE_ROOT;
    assert(tree->topics->resourceSet == iereNO_RESOURCE_SET);

    // Create the hash of origin serverUIDs
    rc = ieut_createHashTable(pThreadData,
                              iettINITIAL_ORIGINSERVER_HASH_CAPACITY,
                              iemem_remoteServers,
                              &tree->originServers);

    if (rc != OK) goto mod_exit;

mod_exit:

    if (osrc || rc != OK)
    {
        iett_destroyTopicTree(pThreadData, tree);
        tree = NULL;
    }

    ieutTRACEL(pThreadData, tree,  ENGINE_FNC_TRACE, FUNCTION_EXIT "tree=%p\n", __func__, tree);

    return tree;
}

//****************************************************************************
/// @brief Clean up / free the content of a subscription tree
///
/// @param[in]     key      Key of the hash table entry being passed
/// @param[in]     keyHash  Hash of the key
/// @param[in]     value    Value of the hash entry (Node at the top of the subtree)
/// @param[in]     context  The context passed to the callback routine
///
/// @remark We use this to avoid lots of additional levels when recursing,
///         when first called the key and keyHash are not valid.
//****************************************************************************
void iett_destroySubsTreeCallback(ieutThreadData_t *pThreadData,
                                  char *key,
                                  uint32_t keyHash,
                                  void *value,
                                  void *context)
{
    iettSubsNode_t *node = (iettSubsNode_t *)value;

    if (node->children)
    {
        ieut_traverseHashTable(pThreadData,
                               node->children,
                               &iett_destroySubsTreeCallback,
                               context);
        ieut_destroyHashTable(pThreadData, node->children);
    }

    int32_t i;

    // Release the stats structure
    if (node->stats != NULL)
    {
        iemem_free(pThreadData, iemem_subsTree, node->stats);
    }

    // Had some subscriptions that were pending deletion, free them up now
    // but don't delete them - we never called their callback back.
    if (node->delPendSubs.list)
    {
        if (node->delPendSubs.count > 0)
        {
            for(i=0; i<node->delPendSubs.count; i++)
            {
                // Free only during destroy
                iett_freeSubscription(pThreadData, node->delPendSubs.list[i], true);
            }
        }

        iemem_free(pThreadData, iemem_subsTree, node->delPendSubs.list);
    }

    // Release any remaining subscriptions, and free the lists
    if (node->activeSubs.list)
    {
        if (node->activeSubs.count > 0)
        {
            for(i=0; i<node->activeSubs.count; i++)
            {
                // Free only during destroy
                iett_freeSubscription(pThreadData, node->activeSubs.list[i], true);
            }
        }

        iemem_free(pThreadData, iemem_subsTree, node->activeSubs.list);
    }

    if (node->wildcardChild) iett_destroySubsTreeCallback(pThreadData,
                                                          key,
                                                          keyHash,
                                                          node->wildcardChild,
                                                          context);
    if (node->multicardChild) iett_destroySubsTreeCallback(pThreadData,
                                                           key,
                                                           keyHash,
                                                           node->multicardChild,
                                                           context);

    iemem_freeStruct(pThreadData, iemem_subsTree, node, node->strucId);
}

//****************************************************************************
/// @brief Clean up / free the content of a topic information tree
///
/// @param[in]     key      Key of the hash table entry being passed
/// @param[in]     keyHash  Hash of the key
/// @param[in]     value    Value of the hash entry (Node at the top of the subtree)
/// @param[in]     context  The context passed to the callback routine
///
/// @remark We use this to avoid lots of additional levels when recursing,
///         when first called the key and keyHash are not valid.
//****************************************************************************
void iett_destroyTopicsTreeCallback(ieutThreadData_t *pThreadData,
                                    char *key,
                                    uint32_t keyHash,
                                    void *value,
                                    void *context)
{
    iettDestroyTopicsTreeCbContext_t *destroyCbContext = (iettDestroyTopicsTreeCbContext_t *)context;
    iettTopicNode_t *node = (iettTopicNode_t *)value;

    if (node->children)
    {
        ieut_traverseHashTable(pThreadData,
                               node->children,
                               &iett_destroyTopicsTreeCallback,
                               destroyCbContext);
        ieut_destroyHashTable(pThreadData, node->children);
    }

    // Remove node from expiry list (it may not be on it)
    ieme_removeTopicFromExpiryReaperList(pThreadData, node);

    // Decrement the in-memory usage count for the retained message if there is one
    if (node->currRetMessage != NULL)
    {
        ismEngine_Message_t *pMessage = node->currRetMessage;
        iereResourceSetHandle_t resourceSet = node->resourceSet;

        // If there is a message left in the store, that's okay if we're
        // just tidying up the engine tree, but there had better not be any
        // in other cases because the reference will come back at restart.
        assert(destroyCbContext->freeingEngineTree == true ||
               node->currRetRefHandle == ismSTORE_NULL_HANDLE);

        // Decrement appropriate statistics
        if (pMessage->Header.Expiry != 0)
        {
            pThreadData->stats.retainedExpiryMsgCount--;
        }
        if (pMessage->Header.MessageType != MTYPE_NullRetained)
        {
            pThreadData->stats.externalRetainedMsgCount--;
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSGS, -1);
            iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSG_BYTES, -(pMessage->fullMemSize));
        }

        pThreadData->stats.internalRetainedMsgCount--;

        assert(node->currOriginServer != NULL);

        iett_removeTopicNodeFromOriginServer(pThreadData,
                                             node,
                                             node->currOriginServer);

        assert(node->currOriginServer == NULL);
        assert(node->originNext == NULL);
        assert(node->originPrev == NULL);

        node->nodeFlags &= ~iettNODE_FLAG_NULLRETAINED;
        node->currRetMessage = NULL;
        node->expiryTime = 0;

        iem_releaseMessage(pThreadData, pMessage);
    }

    iemem_freeStruct(pThreadData, iemem_topicsTree, node, node->strucId);

    return;
}

//****************************************************************************
/// @brief Clean up / free a list of named subscriptions
///
/// @param[in]     key      Key of the hash table entry being passed
/// @param[in]     keyHash  Hash of the key
/// @param[in]     value    Value of the hash entry (Node at the top of the subtree)
/// @param[in]     context  The context passed to the callback routine
//****************************************************************************
void iett_destroyNamedSubsCallback(ieutThreadData_t *pThreadData,
                                   char *key,
                                   uint32_t keyHash,
                                   void *value,
                                   void *context)
{
    iettSubscriptionList_t *subList = (iettSubscriptionList_t *)value;

    if (NULL != subList->list) iemem_free(pThreadData, iemem_subsTree, subList->list);

    iemem_free(pThreadData, iemem_subsTree, subList);
}

//****************************************************************************
/// @brief Clean up oring server information
///
/// @param[in]     key      Key of the hash table entry being passed
/// @param[in]     keyHash  Hash of the key
/// @param[in]     value    Value of the hash entry (Node at the top of the subtree)
/// @param[in]     context  The context passed to the callback routine
//****************************************************************************
void iett_destroyOriginServerCallback(ieutThreadData_t *pThreadData,
                                      char *key,
                                      uint32_t keyHash,
                                      void *value,
                                      void *context)
{
    iettOriginServer_t *originServer = (iettOriginServer_t *)value;

    assert(originServer->head == NULL);
    assert(originServer->tail == NULL);

    iemem_freeStruct(pThreadData, iemem_remoteServers, originServer, originServer->strucId);
}

//****************************************************************************
/// @brief Clean up / free the content of a topic tree
///
/// @param[in]     tree  Topic tree to be cleaned up
///
/// @see iett_createTopicTree
//****************************************************************************
void iett_destroyTopicTree(ieutThreadData_t *pThreadData, iettTopicTree_t *tree)
{
    ieutTRACEL(pThreadData, tree,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "tree=%p\n", __func__, tree);
    
    if (NULL != tree)
    {
        // Clear up the hash of named subscriptions
        if (NULL != tree->namedSubs)
        {
            assert(pthread_rwlock_trywrlock(&tree->namedSubsLock) == 0);
            ieut_traverseHashTable(pThreadData,
                                   tree->namedSubs,
                                   &iett_destroyNamedSubsCallback,
                                   NULL);
            ieut_destroyHashTable(pThreadData, tree->namedSubs);
            (void)pthread_rwlock_destroy(&tree->namedSubsLock);
        }

        // Visit all of nodes in the subscription tree
        if (NULL != tree->subs)
        {
            assert(pthread_rwlock_trywrlock(&tree->subsLock) == 0);
            iett_destroySubsTreeCallback(pThreadData, NULL, 0, tree->subs, NULL);
            (void)pthread_rwlock_destroy(&tree->subsLock);
        }

        // Visit all of nodes in the remote server tree
        if (NULL != tree->remoteServers)
        {
            assert(pthread_rwlock_trywrlock(&tree->remoteServersLock) == 0);
            iett_destroyRemoteServersTreeCallback(pThreadData, NULL, 0, tree->remoteServers, NULL);
            (void)pthread_rwlock_destroy(&tree->remoteServersLock);
        }

        // Visit all of nodes in the topic information tree
        if (NULL != tree->topics)
        {
            assert(pthread_rwlock_trywrlock(&tree->topicsLock) == 0);

            // Don't want to remove from the store
            iettDestroyTopicsTreeCbContext_t destroyCbContext;

            destroyCbContext.freeingEngineTree = (tree == ismEngine_serverGlobal.maintree);

            iett_destroyTopicsTreeCallback(pThreadData, NULL, 0, tree->topics, &destroyCbContext);

            // Clear up the hash of origin serverUIDs
            if (NULL != tree->originServers)
            {
                ieut_traverseHashTable(pThreadData,
                                       tree->originServers,
                                       &iett_destroyOriginServerCallback,
                                       NULL);
                ieut_destroyHashTable(pThreadData, tree->originServers);
            }

            (void)pthread_rwlock_destroy(&tree->topicsLock);
        }

        iemem_freeStruct(pThreadData, iemem_subsTree, tree, tree->strucId);
    }

    ieutTRACEL(pThreadData, tree, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}
