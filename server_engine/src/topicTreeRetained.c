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
/// @file  topicTreeRetained.c
/// @brief Engine component retained publication manipulation functions
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
#include "queueCommon.h"         // ieq functions & constants
#include "engineStore.h"         // iest functions & constants
#include "msgCommon.h"           // iem functions & constants
#include "messageExpiry.h"       // ieme functions & structures
#include "destination.h"         // ieds functions & constants
#include "resourceSetStats.h"    // iere functions & constants

//****************************************************************************
/// @brief Context used when repositioning messages after a scan determines
/// repositioning is required.
//****************************************************************************
typedef struct tag_iettScanRepositionContext_t
{
    uint64_t firstOrderId;                    ///< The first OrderId that is being repositioned
    uint64_t lastOrderId;                     ///< The last OrderId that is being repositioned
    uint32_t batchSize;                       ///< The number of messages being repositioned
    uint32_t scanNumber;                      ///< Which repositioning scan in this sequence of scans this is
    uint32_t maxScans;                        ///< Maximum number of scans to allow in this sequence
    ismEngine_Message_t **repositionMessages; ///< Array of messages that are being repositioned
} iettScanRepositionContext_t;

bool scanRepositionInProgress = false; ///< Whether or not a repositioning scan is in progress

//****************************************************************************
/// @brief Deliver all of the retained messages that match a given topicstring
///        to the queue associated with the specified subscription.
///
/// @param[in]     pThreadData      Current thread context
/// @param[in]     tree             The topic tree in which to find retained msgs
/// @param[in]     topic            Topic for this subscription
/// @param[in]     subscription     Subscription to which retained messages are
///                                 delivered.
/// @param[in]     ppTran           Pointer to either existing transaction or to
///                                 return a new transaction.
/// @param[in]     republish        Whether this is a republish request
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The subsLock must be held for write on entry to this function
//****************************************************************************
int32_t iett_putRetainedMessagesToSubscription(ieutThreadData_t *pThreadData,
                                               iettTopicTree_t *tree,
                                               iettTopic_t *topic,
                                               ismEngine_Subscription_t *subscription,
                                               ismEngine_Transaction_t **ppTran,
                                               bool republish)
{
    int32_t rc;
    uint32_t maxNodes = 0;
    uint32_t nodeCount = 0;
    iettTopicNode_t *topicNode = NULL;
    iettTopicNode_t **topicNodes = NULL;
    iettNewSubCreationData_t *creationData = NULL;
    ismEngine_MessageHandle_t *foundMessages = NULL;
    uint32_t foundMessageCount = 0;
    uint32_t foundMessageMax = 0;
    uint32_t foundMessageIncrement;

    assert(topic->destinationType == ismDESTINATION_TOPIC);

    ieutTRACEL(pThreadData, subscription, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (republish == false) creationData = iett_getNewSubCreationData(subscription);

    ismEngine_getRWLockForRead(&tree->topicsLock);

    // Remember the retUpdates value so we can retrospectively deliver in-flight retained
    // messages to this (newly created) subscriptions.
    if (creationData != NULL) creationData->retUpdatesValue = tree->retUpdates;

    // No wildcards, so we know which node to look for
    if (topic->wildcardCount == 0 && topic->multicardCount == 0)
    {
        rc = iett_insertOrFindTopicsNode(pThreadData, tree->topics, topic, iettOP_FIND, &topicNode);

        if (rc == OK)
        {
            topicNodes = &topicNode;
            nodeCount = 1;
            foundMessageIncrement = 1;
        }
    }
    else
    {
        rc = iett_findMatchingTopicsNodes(pThreadData,
                                          tree->topics, false,
                                          topic,
                                          0, 0, 0,
                                          NULL, &maxNodes, &nodeCount, &topicNodes);

        foundMessageIncrement = 100;
    }

    if (rc == OK)
    {
        const uint32_t nowExpiry = ism_common_nowExpire();

        // Go through all of the nodes returned building a list of retained messages
        for(uint32_t i=0; i<nodeCount; i++)
        {
            ismEngine_Message_t *pMessage = topicNodes[i]->currRetMessage;

            // If there is a message consider adding it to our list
            if (pMessage != NULL)
            {
                // Only add the message to the list if is not a NULL retained and has not expired.
                if ((pMessage->Header.MessageType != MTYPE_NullRetained) &&
                    ((pMessage->Header.Expiry == 0) || (pMessage->Header.Expiry > nowExpiry)))
                {
                    if (foundMessageCount == foundMessageMax)
                    {
                        uint32_t newFoundMessageMax = foundMessageMax + foundMessageIncrement;
                        ismEngine_MessageHandle_t *newFoundMessages;

                        newFoundMessages = iemem_realloc(pThreadData,
                                                         iemem_topicsTree,
                                                         foundMessages,
                                                         sizeof(ismEngine_MessageHandle_t)*newFoundMessageMax);

                        if (newFoundMessages == NULL)
                        {
                            rc = ISMRC_AllocateError;
                            ism_common_setError(rc);

                            // Don't process the list we built up already
                            foundMessageCount = 0;
                            break;
                        }

                        foundMessages = newFoundMessages;
                        foundMessageMax = newFoundMessageMax;
                    }

                    foundMessages[foundMessageCount++] = pMessage;
                }
            }
        }

        // Increment the useCount of found messages while we're holding the lock
        for(uint32_t i=0; i<foundMessageCount; i++)
        {
            iem_recordMessageUsage(foundMessages[i]);
        }
    }
    else if (rc == ISMRC_NotFound)
    {
        rc = OK; // caller doesn't care
    }

    // Release the lock
    ismEngine_unlockRWLock(&tree->topicsLock);

    if (foundMessageCount != 0)
    {
        // Need to honour selection if the subscription is using it or the policy it's using has
        // a default selection rule.
        ismRule_t *selectionRule = subscription->selectionRule;
        size_t selectionRuleLen = (size_t)(subscription->selectionRuleLen);
        const uint32_t subOptions = subscription->subOptions;

        // No explicit selection rule -- is there a default selection rule on the policy?
        if (selectionRule == NULL)
        {
            iepiPolicyInfo_t *subPolicy = ieq_getPolicyInfo(subscription->queueHandle);
            iepiSelectionInfo_t *defaultSelectionInfo = subPolicy->defaultSelectionInfo;

            if (defaultSelectionInfo != NULL && defaultSelectionInfo->selectionRule != NULL)
            {
                selectionRule = defaultSelectionInfo->selectionRule;
                selectionRuleLen = (size_t)defaultSelectionInfo->selectionRuleLen;
            }
        }

        // The transaction only needs to be persistent if the subscription is persistent
        bool persistent = (subscription->internalAttrs & iettSUBATTR_PERSISTENT) == iettSUBATTR_PERSISTENT;

        ismEngine_Transaction_t *pTran = *ppTran;

        // We need a transaction now - so let's make sure we have one
        if (pTran == NULL)
        {
            assert(republish == true);

            // Create an fAsStoreTran transaction and reserve store resources for it.
            rc = ietr_createLocal(pThreadData, NULL, persistent, true, NULL, ppTran);

            pTran = *ppTran;
        }
        else
        {
            assert(pTran->fAsStoreTran == true);
            assert((persistent == false && pThreadData->ReservationState == Inactive) ||
                   (persistent == true && pThreadData->ReservationState == Pending));
        }

        if (rc != OK)
        {
            // Release all the messages.
            for(uint32_t i=0; i<foundMessageCount; i++)
            {
                iem_releaseMessage(pThreadData, (ismEngine_Message_t *)foundMessages[i]);
            }

            goto mod_exit;
        }

        assert(pTran != NULL);

        if (persistent)
        {
            // Note: We reserve space for a single record update - this is used in the case where a subscription
            //       is being added to the topic tree to change it's store state from CREATING
            rc = ietr_reserve(pThreadData, pTran, 0, foundMessageCount);
            assert(rc == OK);
        }

        // If we found some messages, put them to the queue
        for(uint32_t i=0; i<foundMessageCount; i++)
        {
            int32_t selResult;
            ismEngine_Message_t *pMessage = (ismEngine_Message_t *)foundMessages[i];

            // Selection is enabled, check that this message matches it - note that
            // we want message selection to treat this as though it were already
            // retained so that, for example, JMS_IBM_Retain can be selected on...
            //
            // To achieve this, we pass a _copy_ of the message header into the
            // selection function, setting the retained flag on in that header.
            if (selectionRule != NULL)
            {
                ismMessageHeader_t retainedHeader = pMessage->Header;

                retainedHeader.Flags |= ismMESSAGE_FLAGS_RETAINED;

                selResult = ismEngine_serverGlobal.selectionFn(&retainedHeader,
                                                               pMessage->AreaCount,
                                                               pMessage->AreaTypes,
                                                               pMessage->AreaLengths,
                                                               pMessage->pAreaData,
                                                               NULL,
                                                               selectionRule,
                                                               selectionRuleLen,
                                                               NULL);
            }
            else
            {
                selResult = SELECT_TRUE;
            }

            // Honour selection based on message reliability
            if (selResult == SELECT_TRUE)
            {
                bool unreliableMsg = (pMessage->Header.Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE);

                if (unreliableMsg)
                {
                    if ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY) != 0)
                    {
                       selResult = SELECT_FALSE;
                    }
                }
                else
                {
                    if ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY) != 0)
                    {
                        selResult = SELECT_FALSE;
                    }
                }
            }

            // Put the message to the subscription queue
            if (selResult == SELECT_TRUE)
            {
                rc = ieq_put(pThreadData,
                             subscription->queueHandle,
                             ieqPutOptions_RETAINED,
                             pTran,
                             pMessage,
                             IEQ_MSGTYPE_INHERIT,
                             NULL ); // already incremented

                // Release the remaining messages if there was an error
                if (rc != OK)
                {
                    // This will end the outer loop because we're updating its iterator
                    for(;i<foundMessageCount; i++)
                    {
                        pMessage = foundMessages[i];
                        iem_releaseMessage(pThreadData, pMessage);
                    }
                }
            }
            // Not putting this message, need to release it's usage
            else
            {
                ieutTRACEL(pThreadData, pMessage, ENGINE_HIGH_TRACE, "Retained message %p does not match selector (result=%d).\n",
                           pMessage, selResult);
                iem_releaseMessage(pThreadData, pMessage);
            }
        }
    }

mod_exit:

    // Free the arrays we allocated one
    if (foundMessages != NULL) iemem_free(pThreadData, iemem_topicsTree, foundMessages);
    if (topicNodes != NULL && topicNodes != &topicNode) iemem_free(pThreadData, iemem_topicsQuery, topicNodes);

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @internal
/// @brief  Republish messages to the specified subscription
///
/// @param[in]     pThreadData      Current thread context
/// @param[in]     subscription     Subscription to deliver messages to
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_republishRetainedMessages(ieutThreadData_t *pThreadData,
                                       ismEngine_Subscription_t *subscription)
{
    int32_t rc = OK;
    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *wildcards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *multicards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    ismEngine_Transaction_t *transaction = NULL;

    ieutTRACEL(pThreadData, subscription,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "subscription=%p\n", __func__, subscription);

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = subscription->node->topicString;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

    rc = iett_analyseTopicString(pThreadData, &topic);

    if (rc != OK) goto mod_exit;

    rc = iett_putRetainedMessagesToSubscription(pThreadData,
                                                ismEngine_serverGlobal.maintree,
                                                &topic,
                                                subscription,
                                                &transaction,
                                                true);
mod_exit:

    // Commit or roll back the transaction if we got that far
    if (transaction != NULL)
    {
        if (rc == OK)
        {
            ietr_commit(pThreadData, transaction, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
        }
        else
        {
            ietr_rollback(pThreadData, transaction, NULL, IETR_ROLLBACK_OPTIONS_NONE);
        }
    }

    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Deliver the specified message (which was retained) to subscribers
///        that were added after the message was published, but before it was
///        committed (i.e. those owed the retained message).
///
/// @param[in]     pThreadData   Current thread context
/// @param[in]     topicString   The topic string on which message was published
/// @param[in]     message       The message in question
/// @param[in]     publishSUV    The value of topic tree subUpdates at publish
/// @param[in]     commitRUV     The value of topic tree retUpdates at commit
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_putRetainedMessageToNewSubs(ieutThreadData_t *pThreadData,
                                         const char *topicString,
                                         ismEngine_Message_t *message,
                                         uint64_t publishSUV,
                                         uint64_t commitRUV)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, message, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Don't bother processing for messages that have already expired
    if (   (message->Header.Expiry != 0)
        && (message->Header.Expiry < ism_common_nowExpire()))
    {
        goto mod_exit;
    }

    // Note we don't use the pThreadData cache, as we can be called while it is in use
    iettSubscriberList_t newSublist = {0};
    newSublist.topicString = topicString;

    // Get the list of subscribers now in-place for this topic in order to
    // identify any that came into existence since this retained was
    // published.
    rc = iett_getSubscriberList(pThreadData, &newSublist);

    if (rc == OK)
    {
        if (newSublist.subscriberCount != 0)
        {
            const bool policySelection = (ismEngine_serverGlobal.policiesWithDefaultSelection != 0);
            bool unreliableMsg = (message->Header.Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE);

            iepiSelectionInfo_t *cachedDefaultSelectionInfo = NULL;
            int32_t cachedSubPolicySelectionResult = SELECT_TRUE;

            assert(NULL != newSublist.subscribers);

            int count=0;
            ismEngine_Subscription_t *pSubscription = newSublist.subscribers[0];

            assert(rc == OK);

            while (pSubscription != NULL)
            {
                iettNewSubCreationData_t *pCreationInfo = iett_getNewSubCreationData(pSubscription);

                // This subscription was created after the original subscriber list
                // was produced (> publishSUV) but before the publish was
                // committed (< commitRUV), and so is owed this retained message,
                // but, they are owed it as a non-retained message (they should
                // have received it live).
                if (pCreationInfo != NULL &&
                    pCreationInfo->subsUpdatesValue > publishSUV &&
                    pCreationInfo->retUpdatesValue < commitRUV)
                {
                    int32_t selResult;

                    // Honour selection either explicitly from the subscription, or via a default
                    // selection rule.
                    ismRule_t *selectionRule = pSubscription->selectionRule;
                    bool cacheThisSelectionResult;
                    size_t selectionRuleLen;

                    // No explicit selection, and there is a possibility of policy default selection
                    if (policySelection == true && selectionRule == NULL)
                    {
                        iepiPolicyInfo_t *subPolicy = ieq_getPolicyInfo(pSubscription->queueHandle);
                        iepiSelectionInfo_t *defaultSelectionInfo = subPolicy->defaultSelectionInfo;

                        if (defaultSelectionInfo != NULL)
                        {
                            selectionRule = defaultSelectionInfo->selectionRule;

                            // See if this is using the same policy (or precisely matching selection
                            // rule) as the one we have cached
                            if (selectionRule != NULL)
                            {
                                selectionRuleLen = (size_t)defaultSelectionInfo->selectionRuleLen;

                                if (defaultSelectionInfo == cachedDefaultSelectionInfo ||
                                    (cachedDefaultSelectionInfo != NULL &&
                                     selectionRuleLen == cachedDefaultSelectionInfo->selectionRuleLen &&
                                     memcmp(selectionRule, cachedDefaultSelectionInfo->selectionRule, selectionRuleLen) == 0))
                                {
                                    selResult = cachedSubPolicySelectionResult;
                                    selectionRule = NULL;
                                }
                                else
                                {
                                    cachedDefaultSelectionInfo = defaultSelectionInfo;
                                    cacheThisSelectionResult = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        selectionRuleLen = (size_t)pSubscription->selectionRuleLen;
                        cacheThisSelectionResult = false;
                    }

                    // If there is a selection rule (either from subscription or policy) select on it.
                    if (selectionRule != NULL)
                    {
                        assert(cacheThisSelectionResult == true || cacheThisSelectionResult == false);

                        selResult = ismEngine_serverGlobal.selectionFn(&(message->Header),
                                                                       message->AreaCount,
                                                                       message->AreaTypes,
                                                                       message->AreaLengths,
                                                                       message->pAreaData,
                                                                       topicString,
                                                                       selectionRule,
                                                                       selectionRuleLen,
                                                                       NULL );

                        if (cacheThisSelectionResult == true) cachedSubPolicySelectionResult = selResult;
                    }
                    else
                    {
                        selResult = SELECT_TRUE;
                    }

                    // Honour selection based on messsage reliability
                    if (selResult == SELECT_TRUE)
                    {
                        const uint32_t subOptions = pSubscription->subOptions;

                        if (unreliableMsg)
                        {
                            if ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY) != 0)
                            {
                                selResult = SELECT_FALSE;
                            }
                        }
                        else
                        {
                            if ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY) != 0)
                            {
                                selResult = SELECT_FALSE;
                            }
                        }
                    }

                    // Attempt to put the message
                    if (selResult == SELECT_TRUE)
                    {
                        int32_t msg_rc = ieq_put(pThreadData,
                                                 pSubscription->queueHandle,
                                                 ieqPutOptions_NONE,
                                                 NULL, // no transaction
                                                 message,
                                                 IEQ_MSGTYPE_REFCOUNT,
                                                 NULL );

                        if (msg_rc != OK)
                        {
                            if (rc == OK) rc = msg_rc;
                        }
                    }
                }

                count++;
                pSubscription = newSublist.subscribers[count];
            }
        }

        // Release this 'inner' subscriber list
        iett_releaseSubscriberList(pThreadData, &newSublist);
    }
    else if (rc == ISMRC_NotFound)
    {
        rc = OK;
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}


//****************************************************************************
/// @brief  Create the engine's store topic record
///
/// @remark The engine's topics lock must be held for WRITE while this is
///         being called.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static int32_t iett_createEngineStoreTopicRecord(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;
    ismStore_Record_t storeRecord;
    iestTopicDefinitionRecord_t topicDefinitionRecord;
    char *Fragment;
    uint32_t FragmentLength;

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ieutTRACEL(pThreadData, tree, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (tree->retStoreHandle == ismSTORE_NULL_HANDLE)
    {
        // First fragment is always the topic definition record
        Fragment = (char *)&topicDefinitionRecord;
        FragmentLength = sizeof(topicDefinitionRecord);
        storeRecord.DataLength = FragmentLength;

        // Fill in the store record and contents of the topic record
        storeRecord.Type = ISM_STORE_RECTYPE_TOPIC;
        storeRecord.Attribute = 0;
        storeRecord.State = iestTDR_STATE_NONE;
        storeRecord.pFrags = &Fragment;
        storeRecord.pFragsLengths = &FragmentLength;
        storeRecord.FragsCount = 1;

        memcpy(topicDefinitionRecord.StrucId, iestTOPIC_DEFN_RECORD_STRUCID, 4);
        topicDefinitionRecord.Version = iestTDR_CURRENT_VERSION;

        // We are going to be creating a record, which might need to be retried, the
        // code assumes that there is no pending store operations.
        #ifndef NDEBUG
        uint32_t OpCount;
        rc = ism_store_getStreamOpsCount(pThreadData->hStream, &OpCount);
        assert(rc == OK);
        assert(OpCount == 0);
        #endif

        ismStore_Handle_t storeHandle;

        do
        {
            // Add to the store
            rc = ism_store_createRecord(pThreadData->hStream,
                                        &storeRecord,
                                        &storeHandle);
        }
        while(rc == ISMRC_StoreGenerationFull);

        if (rc != OK)
        {
            ism_common_setError(rc);
            goto mod_exit;
        }

        assert(storeHandle != ismSTORE_NULL_HANDLE);

        iest_store_commit(pThreadData, false);

        tree->retStoreHandle = storeHandle;

        // Now that we have a store handle, re-reconcile the engine topic tree
        // which should open the reference context and get store statistics
        rc = iett_reconcileEngineTopicTree(pThreadData);

        if (rc != OK) goto mod_exit;
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief Finish the unstoring of an array of previously retained messages
///        by releasing message references and freeing the array.
//****************************************************************************
static inline void iett_finishUnstoreRetainedMsgArray(ieutThreadData_t *pThreadData,
                                                      ismEngine_Message_t **removedMessages)
{
    ieutTRACEL(pThreadData, removedMessages,  ENGINE_CEI_TRACE, FUNCTION_IDENT "iettACId=0x%016lx\n",
               __func__, (long unsigned int)removedMessages);

    ismEngine_Message_t **currMessage = removedMessages;

    while(*currMessage != NULL)
    {
        iem_releaseMessage(pThreadData, *currMessage);
        currMessage++;
    }

    iemem_free(pThreadData, iemem_topicsTree, removedMessages);
}

void iett_asyncUnstoreRetainedMsgArray(int32_t rc, void *context)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    pThreadData->threadType = AsyncCallbackThreadType;
    iett_finishUnstoreRetainedMsgArray(pThreadData, (ismEngine_Message_t **)context);
    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @brief Finish the deletion of an array of retained message references
///        by unstoring the messages.
//****************************************************************************
static inline void iett_finishRetainedMsgArrayReferenceDeletion(ieutThreadData_t *pThreadData,
                                                                ismEngine_Message_t **removedMessages)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, removedMessages,  ENGINE_CEI_TRACE, FUNCTION_IDENT "iettACId=0x%016lx\n",
               __func__, (long unsigned int)removedMessages);

    uint32_t storeOpCount = 0;

    ismEngine_Message_t **currMessage = removedMessages;

    while(*currMessage != NULL)
    {
        rc = iest_unstoreMessage(pThreadData, *currMessage, false, false, NULL, &storeOpCount);
        assert(rc == OK);
        currMessage++;
    }

    if (storeOpCount != 0)
    {
        rc = iest_store_asyncCommit(pThreadData, false, iett_asyncUnstoreRetainedMsgArray, removedMessages);
    }

    if (rc != ISMRC_AsyncCompletion)
    {
        assert(rc == OK);
        iett_finishUnstoreRetainedMsgArray(pThreadData, removedMessages);
    }
}

void iett_asyncRetainedMsgArrayReferenceDeletion(int32_t rc, void *context)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    pThreadData->threadType = AsyncCallbackThreadType;
    iett_finishRetainedMsgArrayReferenceDeletion(pThreadData, (ismEngine_Message_t **)context);
    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @brief Commit the deletion of references for an array of messages removed
///        by iett_removeRetainedMessageFromNode calls.
///
/// @param[in]     pThreadData     Thread data to use
/// @param[in]     removedMessages A NULL terminated array of messages whose
///                                references have been deleted, but need to be
///                                committed and unstored.
//****************************************************************************
void iett_commitRetainedMsgArrayReferenceDeletion(ieutThreadData_t *pThreadData,
                                                  ismEngine_Message_t **removedMessages)
{
    int32_t rc;

    ieutTRACEL(pThreadData, removedMessages, ENGINE_FNC_TRACE, FUNCTION_ENTRY "removedMessages=%p iettACId=0x%016lx\n",
               __func__, removedMessages, (long unsigned int)removedMessages);

    // Commit the ism_store_deleteReferences that are waiting to be committed
    rc = iest_store_asyncCommit(pThreadData, false, iett_asyncRetainedMsgArrayReferenceDeletion, removedMessages);

    if (rc != ISMRC_AsyncCompletion)
    {
        assert(rc == OK);
        iett_finishRetainedMsgArrayReferenceDeletion(pThreadData, removedMessages);
    }

    ieutTRACEL(pThreadData, removedMessages,  ENGINE_FNC_TRACE, FUNCTION_EXIT "removedMessages=%p\n",
               __func__, removedMessages);
}
//****************************************************************************
/// @brief Remove retained message from the specified.
///
/// @param[in]     pThreadData    Thread data to use
/// @param[in]     topicNode      The node in the topic tree to work on
/// @param[out]    removedTree    A pointer to the node in the tree which was
///                               removed as a result of ther being no remaining
///                               retained nodes (this may be ABOVE the node
///                               requested)
/// @param[out]    originServer   The origin server that this message has been
///                               removed for.
/// @param[out]    removedMessage A persistent retained message that's been removed
///                               and the reference deleted.
/// @param[in]     expiryTime     Expiration time to check against, 0 for removal
///                               regardless of expiry.
///
/// @remark The topic tree must be locked for WRITE before calling.
///
/// @remark If the removal of the retained message results in a node which no
///         longer needs to exist the removedTree value will be returned pointing
///         to a branch which has been removed from the tree, but not yet destroyed.
///
///         When the caller is able to do so iett_destroyTopicsTreeCallback should
///         be used to remove this tree completely.
///
///         If a reference to a persistent retained message is removed, and
///         removedMessage is not NULL, the caller must complete the removal by
///         issuing a store commit / async commit. The caller can choose to batch
///         multiple such messages in a group.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_removeRetainedMessageFromNode(ieutThreadData_t *pThreadData,
                                           iettTopicNode_t *topicNode,
                                           iettTopicNode_t **removedTree,
                                           iettOriginServer_t **originServer,
                                           ismEngine_Message_t **removedMessage,
                                           uint32_t expiryTime)
{
    int32_t rc = OK;
    ismEngine_Message_t *pMessage = topicNode->currRetMessage;
    iereResourceSetHandle_t resourceSet = topicNode->resourceSet;

    ieutTRACEL(pThreadData, topicNode, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Only support the removedMessage if expiryTime is not zero or we are removing
    // a null retained.
    assert(removedMessage == NULL ||
           (expiryTime != 0 || (topicNode->nodeFlags & iettNODE_FLAG_NULLRETAINED) != 0));

    *removedTree = NULL;
    *originServer = NULL;

    // If we're only removing expired messages and there are pending updates,
    // there is no message, or it hasn't expired then we have nothing to do.
    // If we are unsetting we always go through the motions whether there is currently
    // a retained message or not to ensure that we ignore earlier in-flight retained
    // messages when they are committed.
    if (expiryTime != 0 && (topicNode->pendingUpdates > 0 ||
                            pMessage == NULL ||
                            pMessage->Header.Expiry == 0 ||
                            pMessage->Header.Expiry > expiryTime))
    {
        goto mod_exit;
    }
    // Need to remove any retained message from this node
    else
    {
        bool releaseMessage = true;

        iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

        if (topicNode->currRetRefHandle != ismSTORE_NULL_HANDLE)
        {
            assert(pMessage != NULL);
            assert(tree->retStoreHandle != ismSTORE_NULL_HANDLE);
            assert(tree->retRefContext != ismSTORE_NULL_HANDLE);

            rc = ism_store_deleteReference(pThreadData->hStream,
                                           tree->retRefContext,
                                           topicNode->currRetRefHandle,
                                           topicNode->currRetOrderId,
                                           0);  // minimumActiveOrderId unchanged

            if (rc != OK) goto mod_exit;

            // If our caller is using an array of messages being deleted, add this one
            // to it so that they can commit a batch at once, otherwise we'll need to
            // commit this one ourselves.
            if (removedMessage != NULL)
            {
                releaseMessage = false; // Keep hold of the message until after committing
                *removedMessage = pMessage;
            }
            else
            {
                iest_store_commit(pThreadData, false);

                // Decrement the store use count of the message
                rc = iest_unstoreMessage(pThreadData, pMessage, false, true, NULL, NULL);

                assert(rc == OK);
            }

            topicNode->currRetRefHandle = ismSTORE_NULL_HANDLE;
            topicNode->currRetOrderId = 0;

            // No pending updates, so can set the activeOrderIdVote to 0.
            if (topicNode->pendingUpdates == 0)
            {
                topicNode->activeOrderIdVote = 0;
            }
        }

        // There is a message, remove it
        if (pMessage)
        {
            bool isNullRetained = (pMessage->Header.MessageType == MTYPE_NullRetained);

            // We are removing this message BECAUSE it has expired, so increase the global
            // count of messages which have been expired
            if (expiryTime != 0)
            {
                assert(pMessage->Header.Expiry != 0);

                // We are removing an expiring NullRetained message from the topic tree,
                // this has expiry because that's the mechanism we chose to clear them from
                // the topic tree - we should not be counting this in the external statistics.
                if (isNullRetained == false)
                {
                    pThreadData->stats.expiredMsgCount++;
                }
            }

            // The message we are removing did have expiry, so decrease the global count
            // of retained messages that have an expiry set (we do this whether we expired
            // it now or cleared it for some other reason).
            if (pMessage->Header.Expiry != 0)
            {
                pThreadData->stats.retainedExpiryMsgCount--;
            }

            // The message we are removing is not an MTYPE_NullRetained, so decrease the
            // external counts of retained messages and retained message bytes
            if (isNullRetained == false)
            {
                pThreadData->stats.externalRetainedMsgCount--;
                iere_primeThreadCache(pThreadData, resourceSet);
                iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSGS, -1);
                iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSG_BYTES, -(pMessage->fullMemSize));
            }

            // Stop propagating this as the retained message.
            pMessage->Header.Flags &= ~ismMESSAGE_FLAGS_PROPAGATE_RETAINED;

            // Decrement the overall count of retained messages in the system
            pThreadData->stats.internalRetainedMsgCount--;

            assert(topicNode->currOriginServer != NULL);

            *originServer = topicNode->currOriginServer;

            iett_removeTopicNodeFromOriginServer(pThreadData,
                                                 topicNode,
                                                 topicNode->currOriginServer);

            assert(topicNode->currOriginServer == NULL);
            assert(topicNode->originNext == NULL);
            assert(topicNode->originPrev == NULL);

            topicNode->nodeFlags &= ~iettNODE_FLAG_NULLRETAINED;
            topicNode->currRetMessage = NULL;
            topicNode->expiryTime = 0;

            if (releaseMessage == true) iem_releaseMessage(pThreadData, pMessage);
        }

        // TODO: I don't think we want to do this if we're explicitly removing something from
        //       the reaper - we probably want a flag to indicate whether we're being called
        // from the reaper thread, or not and to only do this if NOT (not if expiryTime == 0)
        // ... I am only not making this change now because of where we are in the release
        // schedule and the potential for it to be disruptive.
        //
        // If we're not just removing expired messages, this effectively represents a new
        // 'put' with an empty message, so any messages put earlier that are still in-flight
        // should be ignored when they are committed and any new retained messages are really
        // new.
        if (expiryTime == 0)
        {
            topicNode->currRetTimestamp = ism_engine_retainedServerTime();
        }

        *removedTree = iett_removeUnusedTree(pThreadData, tree, topicNode);

        tree->topicsUpdates++;
    }

mod_exit:

    ieutTRACEL(pThreadData, *removedTree,  ENGINE_FNC_TRACE, FUNCTION_EXIT "removedTree=%p rc=%d\n", __func__, *removedTree, rc);

    return rc;
}

//****************************************************************************
/// @brief Find retained messages on the specified wildcarded topic string
///        and remove them and any nodes we can remove.
///
/// @param[in]     pThreadData   Current thread context
/// @param[in]     tree          The topic tree to use
/// @param[in]     topic         An analysed topic (must be REGEX_TOPIC)
///
/// @remark We are performing active retained message removal, meaning in-flight
///         retained publishes are ignored when they get committed.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_removeRetainedMessages(ieutThreadData_t *pThreadData,
                                    iettTopicTree_t *tree,
                                    iettTopic_t *topic)
{
    int32_t rc = OK;
    uint32_t maxNodes = 0;
    uint32_t nodeCount = 0;
    uint32_t originServerCount = 0;

    iettTopicNode_t *topicNode = NULL;
    iettTopicNode_t **topicNodes = NULL;

    ieutTRACEL(pThreadData, topic, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    ismEngine_getRWLockForWrite(&tree->topicsLock);

    // We should only be called for REGEX topics now (used to be different)
    assert(topic->destinationType == ismDESTINATION_REGEX_TOPIC);

    rc = iett_findMatchingTopicsNodes(pThreadData,
                                      tree->topics, false,
                                      topic,
                                      0, 0, 0,
                                      NULL, &maxNodes, &nodeCount, &topicNodes);

    uint32_t originServerCapacity = nodeCount;

    if (nodeCount > tree->originServers->totalCount) originServerCapacity = tree->originServers->totalCount;

    iettOriginServer_t *originServer[originServerCapacity+1]; // +1 ensures this is not a zero length array

    // Found some matching nodes
    if (rc == OK)
    {
        // Go through all of the nodes in reverse order looking for retained messages
        for(int32_t i=(int32_t)nodeCount-1; i>=0; i--)
        {
            rc = iett_removeRetainedMessageFromNode(pThreadData,
                                                    topicNodes[i],
                                                    &topicNodes[i],
                                                    &originServer[originServerCount],
                                                    NULL,
                                                    0);

            if (rc != OK) goto mod_exit;

            if (originServer[originServerCount] != NULL)
            {
                int32_t x;
                for(x=0; x<originServerCount; x++)
                {
                    if (originServer[x] == originServer[originServerCount]) break;
                }

                if (x == originServerCount) originServerCount++;
            }
        }
    }
    else
    {
        // Don't expect to see ISMRC_NotFound for REGEX. If that changes,
        // just set it to OK if rc == ISMRC_NotFound
        assert(rc != ISMRC_NotFound);
    }

mod_exit:

    // Release the lock
    ismEngine_unlockRWLock(&tree->topicsLock);

    // Finally we can destroy any subtrees identified earlier
    if (rc == OK && nodeCount != 0)
    {
        assert(topicNodes != NULL);

        iettDestroyTopicsTreeCbContext_t destroyCbContext;

        destroyCbContext.freeingEngineTree = false;

        uint32_t i;
        for(i=0; i<nodeCount; i++)
        {
            assert(topicNodes[i] != ismEngine_serverGlobal.maintree->topics);

            if (topicNodes[i] != NULL)
            {
                iett_destroyTopicsTreeCallback(pThreadData, NULL, 0, topicNodes[i], &destroyCbContext);
            }
        }

        // And report any origin servers
        iettOriginServerCbContext_t originServerCbContext;

        originServerCbContext.rc = OK;

        for(i=0; i<originServerCount; i++)
        {
            iett_clusterReportOriginServer(pThreadData, NULL, 0, originServer[i], &originServerCbContext);
        }
    }

    if (topicNodes != NULL && topicNodes != &topicNode) iemem_free(pThreadData, iemem_topicsQuery, topicNodes);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Callback used when scanning the expiry reaper list for topics
///
/// @param[in]     pThreadData  Current thread context
/// @param[in]     object       The object for which the callback is being called
/// @param[in,out] context      Context supplied to the callback
///
/// @return ieutSPLIT_LIST* value indicating how the traversal should proceed
///
/// @see ieutSplitListCallbackAction
//****************************************************************************
ieutSplitListCallbackAction_t iett_reapTopicExpiredMessagesCB(ieutThreadData_t *pThreadData,
                                                              void *object,
                                                              void *context)
{
    iettTopicNode_t *topicNode = (iettTopicNode_t *)object;
    iemeExpiryReaperTopicContext_t *pContext = context;

    ieutSplitListCallbackAction_t action = ieutSPLIT_LIST_CALLBACK_CONTINUE;

    // Check we've not been asked to end
    if (*(pContext->reaperEndRequested) == true)
    {
        action = ieutSPLIT_LIST_CALLBACK_STOP;
        goto mod_exit;
    }

    // Every 32 calls (and the 1st call) update the expiry time we use
    if ((pContext->callbackCount & 0x1F) == 0)
    {
        pContext->nowExpire = ism_common_nowExpire();
    }

    pContext->callbackCount += 1;

    bool explicitRemoval = false;

    // Whether this NullRetained has expired, we may perform an explicit removal
    if ((pContext->remoteServerMemLimit != NoLimit) && ((topicNode->nodeFlags & iettNODE_FLAG_NULLRETAINED) != 0))
    {
        // If we're at the level where we should discard local NullRetained messages,
        // or at the HighQoSLimit and this is not a local null retained, we should remove
        // it now.
        if ((pContext->remoteServerMemLimit == DiscardLocalNullRetained) ||
            ((pContext->remoteServerMemLimit >= HighQoSLimit) &&
             (topicNode->currOriginServer->localServer == false)))
        {
            explicitRemoval = true;
        }
    }

    if (explicitRemoval == false)
    {
        // Message not due to expire yet  - continue on the list
        if (LIKELY(topicNode->expiryTime > pContext->nowExpire))
        {
            // Remember the earliest expiry time we saw this scan
            if (topicNode->expiryTime < pContext->earliestObservedExpiry)
            {
                pContext->earliestObservedExpiry = topicNode->expiryTime;
            }
            pContext->statTopicNoWorkRequired += 1;
            topicNode = NULL;
            assert(action == ieutSPLIT_LIST_CALLBACK_CONTINUE);
        }
        // No longer has an expiry - remove it pro-actively from the list
        else if (topicNode->expiryTime == 0)
        {
            pContext->statTopicNoExpiry += 1;
            topicNode = NULL;
            action = ieutSPLIT_LIST_CALLBACK_REMOVE_OBJECT;
        }
    }

    // If we still have a topicNode in our hand, we need to take some action.
    if (topicNode != NULL)
    {
        // Reallocate the removed subtree array if required
        if (pContext->removedSubtreeCount == pContext->removedSubtreesCapacity)
        {
            uint32_t newRemovedCapacity = pContext->removedSubtreesCapacity + 1000;

            iettTopicNode_t **newRemovedTrees = iemem_realloc(pThreadData,
                                                              IEMEM_PROBE(iemem_topicsTree, 4),
                                                              pContext->removedSubtrees,
                                                              newRemovedCapacity * sizeof(iettTopicNode_t *));

            // No memory available to satisfy the allocation - stop the scan
            if (newRemovedTrees == NULL)
            {
                ieutTRACEL(pThreadData, newRemovedCapacity, ENGINE_INTERESTING_TRACE,
                           "Unable to allocate memory for %u removed trees, stopping scan early\n",
                           newRemovedCapacity);

                pContext->reapStoppedEarly = true;

                action = ieutSPLIT_LIST_CALLBACK_STOP;
                goto mod_exit;
            }
            else
            {
                pContext->removedSubtreesCapacity = newRemovedCapacity;
                pContext->removedSubtrees = newRemovedTrees;
            }
        }

        assert(pContext->removedSubtrees != NULL);
        pContext->removedSubtrees[pContext->removedSubtreeCount] = NULL;

        // Reallocate the origin server array if required
        if (pContext->originServerCount == pContext->originServerCapacity)
        {
            uint32_t newOriginServerCapacity = pContext->originServerCapacity + 100;

            iettOriginServer_t **newOriginServers = iemem_realloc(pThreadData,
                                                                  IEMEM_PROBE(iemem_topicsTree, 8),
                                                                  pContext->originServers,
                                                                  newOriginServerCapacity * sizeof(iettOriginServer_t *));

            // No memory available to satisfy the allocation - stop the scan
            if (newOriginServers == NULL)
            {
                ieutTRACEL(pThreadData, newOriginServerCapacity, ENGINE_INTERESTING_TRACE,
                           "Unable to allocate memory for %u origin servers, stopping scan early\n",
                           newOriginServerCapacity);

                pContext->reapStoppedEarly = true;

                action = ieutSPLIT_LIST_CALLBACK_STOP;
                goto mod_exit;
            }
            else
            {
                pContext->originServerCapacity = newOriginServerCapacity;
                pContext->originServers = newOriginServers;
            }
        }

        assert(pContext->originServers != NULL);
        pContext->originServers[pContext->originServerCount] = NULL;

        // Reallocate the removed message array if required
        if (pContext->removedMessagesCount == pContext->removedMessagesCapacity)
        {
            uint32_t newRemovedCapacity = pContext->removedMessagesCapacity + iettREAP_TOPIC_MESSAGE_BATCH_SIZE;

            ismEngine_Message_t **newRemovedMessages = iemem_realloc(pThreadData,
                                                                     IEMEM_PROBE(iemem_topicsTree, 10),
                                                                     pContext->removedMessages,
                                                                     (newRemovedCapacity + 1) * sizeof(ismEngine_Message_t *));

            // No memory available to satisfy the allocation - stop the scan
            if (newRemovedMessages == NULL)
            {
                ieutTRACEL(pThreadData, newRemovedMessages, ENGINE_INTERESTING_TRACE,
                           "Unable to allocate memory for %u removed messages, stopping scan early\n",
                           newRemovedCapacity);

                pContext->reapStoppedEarly = true;

                action = ieutSPLIT_LIST_CALLBACK_STOP;
                goto mod_exit;
            }
            else
            {
                pContext->removedMessagesCapacity = newRemovedCapacity;
                pContext->removedMessages = newRemovedMessages;
            }
        }

        assert(pContext->removedMessages != NULL);
        pContext->removedMessages[pContext->removedMessagesCount] = NULL;

        assert(action == ieutSPLIT_LIST_CALLBACK_CONTINUE);

        bool newStoreTran;

        // If we haven't removed any messages yet, start a store transaction to ensure that
        // we won't start one while we're holding an engine lock. If we did start the store
        // transaction (i.e. one was not already active) we remember that so we can cancel
        // it later if nothing was added to it.
        if (pContext->removedMessagesCount == 0)
        {
            newStoreTran = iest_store_startTransaction(pThreadData);
        }
        else
        {
            newStoreTran = false;
        }

        ismEngine_getRWLockForWrite(&ismEngine_serverGlobal.maintree->topicsLock);

        uint32_t expireTime;

        // If we're explicitly removing this message, there are no pending updates on this node, and
        // it's still a NullRetained message (which we can check cleanly now we've got the lock) then
        // don't pass an expiry time to the removal function - otherwise we pass the expiry time to use.
        if ((explicitRemoval == true) &&
            (topicNode->pendingUpdates == 0) &&
            ((topicNode->nodeFlags & iettNODE_FLAG_NULLRETAINED) != 0))
        {
            expireTime = 0;
        }
        else
        {
            expireTime = pContext->nowExpire;
        }

        int32_t rc = iett_removeRetainedMessageFromNode(pThreadData,
                                                        topicNode,
                                                        &pContext->removedSubtrees[pContext->removedSubtreeCount],
                                                        &pContext->originServers[pContext->originServerCount],
                                                        &pContext->removedMessages[pContext->removedMessagesCount],
                                                        expireTime);

        ismEngine_unlockRWLock(&ismEngine_serverGlobal.maintree->topicsLock);

        // If that call failed, give up the scan
        if (rc != OK)
        {
            // We started the store transaction so we had better cancel it.
            if (newStoreTran == true) iest_store_cancelTransaction(pThreadData);

            ieutTRACEL(pThreadData, topicNode, ENGINE_INTERESTING_TRACE,
                       "Failed to remove retained messages from node %p\n", topicNode);

            pContext->reapStoppedEarly = true;

            action = ieutSPLIT_LIST_CALLBACK_STOP;
            goto mod_exit;
        }
        else
        {
            pContext->statTopicReaped += 1;

            if (pContext->removedMessages[pContext->removedMessagesCount] != NULL)
            {
                pContext->removedMessagesCount += 1;

                if (pContext->removedMessagesCount == iettREAP_TOPIC_MESSAGE_BATCH_SIZE)
                {
                    // Add a NULL as a sentinel
                    pContext->removedMessages[iettREAP_TOPIC_MESSAGE_BATCH_SIZE] = NULL;

                    // Call the function to commit and unstore this array of messages - we have
                    // now passed control of the array to that function.
                    iett_commitRetainedMsgArrayReferenceDeletion(pThreadData, pContext->removedMessages);

                    pContext->removedMessages = NULL;
                    pContext->removedMessagesCount = pContext->removedMessagesCapacity = 0;
                }
            }
            // If we started a new store transaction but didn't put anything in it, cancel it.
            else if (newStoreTran == true)
            {
                assert(pContext->removedMessagesCount == 0);
                iest_store_cancelTransaction(pThreadData);
            }

            // The removal of the retained message removed a subtree of topic nodes from the tree,
            // this must have included this node, but may also include others - these are removed
            // as part of the finish processing later.
            if (pContext->removedSubtrees[pContext->removedSubtreeCount] != NULL)
            {
                action = ieutSPLIT_LIST_CALLBACK_REMOVE_OBJECT;
                pContext->removedSubtreeCount += 1;
            }

            // If there is an origin server, make sure we don't already know about it - these will
            // be reported as part of the finish processing later.
            if (pContext->originServers[pContext->originServerCount] != NULL)
            {
                uint32_t i;

                for(i=0; i<pContext->originServerCount; i++)
                {
                    if (pContext->originServers[i] == pContext->originServers[pContext->originServerCount]) break;
                }

                if (i == pContext->originServerCount) pContext->originServerCount += 1;
            }
        }
    }

mod_exit:

    return action;
}

//****************************************************************************
/// @brief Finish the topic reaper list scan
///
/// @param[in]     pThreadData  Current thread context
/// @param[in,out] context      Context supplied to the callback(s)
//****************************************************************************
void iett_finishReapTopicExpiredMessages(ieutThreadData_t *pThreadData,
                                         void *context)
{
    iemeExpiryReaperTopicContext_t *pContext = context;

    ieutTRACEL(pThreadData, pContext, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "callbackCount=%u removedSubtreeCount=%u\n", __func__,
               pContext->callbackCount, pContext->removedSubtreeCount);

    // Commit the removal of any retained message references
    if (pContext->removedMessages != NULL)
    {
        if (pContext->removedMessagesCount != 0)
        {
            // Add a NULL as a sentinel
            pContext->removedMessages[pContext->removedMessagesCount] = NULL;

            // Call the function to commit and unstore this array of messages - we have
            // now passed control of the array to that function.
            iett_commitRetainedMsgArrayReferenceDeletion(pThreadData, pContext->removedMessages);
        }
        else
        {
            iemem_free(pThreadData, iemem_topicsTree, pContext->removedMessages);
        }

        pContext->removedMessages = NULL;
        pContext->removedMessagesCount = pContext->removedMessagesCapacity = 0;
    }

    // Destroy any removed subtrees
    if (pContext->removedSubtrees != NULL)
    {
        iettDestroyTopicsTreeCbContext_t destroyCbContext;

        destroyCbContext.freeingEngineTree = false;

        for(uint32_t i=0; i<pContext->removedSubtreeCount; i++)
        {
            assert(pContext->removedSubtrees[i] != ismEngine_serverGlobal.maintree->topics);
            iett_destroyTopicsTreeCallback(pThreadData, NULL, 0, pContext->removedSubtrees[i], &destroyCbContext);
        }

        iemem_free(pThreadData, iemem_topicsTree, pContext->removedSubtrees);

        pContext->removedSubtrees = NULL;
        pContext->removedSubtreeCount = pContext->removedSubtreesCapacity = 0;
    }

    // Report on any origin servers reaped
    if (pContext->originServers != NULL)
    {
        iettOriginServerCbContext_t originServerCbContext;

        originServerCbContext.rc = OK;

        for(uint32_t i=0; i<pContext->originServerCount; i++)
        {
            iett_clusterReportOriginServer(pThreadData,
                                           NULL,
                                           0,
                                           pContext->originServers[i],
                                           &originServerCbContext);
        }

        iemem_free(pThreadData, iemem_topicsTree, pContext->originServers);

        pContext->originServers = NULL;
        pContext->originServerCount = pContext->originServerCapacity = 0;
    }

    ieutTRACEL(pThreadData, pContext, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

//****************************************************************************
/// @brief  Remove any unused nodes from the topic tree indicated by the
/// specified node.
///
/// @param[in]     pThreadData  Current thread context
/// @param[in]     tree         The root of the topic tree
/// @param[in]     topicNode    The topic node at which to start looking
///
/// @remark The topics tree lock must be held for Write on entry.
///
/// @remark The returned subtree is removed from the topic tree, but not
/// destroyed. After releasing the lock, a call to iett_destroyTopicsTreeCallback
/// should be made to actually free up storage.
///
/// @return The node at the start of the removed subtree
//****************************************************************************
iettTopicNode_t *iett_removeUnusedTree(ieutThreadData_t *pThreadData,
                                       iettTopicTree_t *tree,
                                       iettTopicNode_t *topicNode)
{
    iettTopicNode_t *removedTree = NULL;

    // Work out whether we should remove any nodes
    while(NULL != topicNode && topicNode != tree->topics &&
          topicNode->pendingUpdates == 0 &&
          NULL == topicNode->currRetMessage &&
          (NULL == topicNode->children ||
           (topicNode->children->totalCount == (removedTree == NULL ? 0 : 1))))
    {
        removedTree = topicNode;
        topicNode = topicNode->parent;
    }

    // We found nodes that can be destroyed. We remove them from the tree now
    // but don't destroy them until we've released the topic lock.
    if (NULL != removedTree)
    {
        char *topicSubstring = (char *)(removedTree+1);
        ieut_removeHashEntry(pThreadData,
                             removedTree->parent->children,
                             topicSubstring,
                             iett_generateSubstringHash(topicSubstring));
        removedTree->parent = NULL;
    }

    return removedTree;
}

//****************************************************************************
/// @brief Remove local copy of all retained messages for a given regex
///        topic string
///
/// @param[in]     pThreadData       Current thread context
/// @param[in]     topicString       Topic string.
///
/// @remark This really removes the message / topic nodes from the local topic
///         tree without sending information to other cluster members.
///
/// @return OK if retained message removed successfully, or another
///         ISMRC_ value on error.
//****************************************************************************
int32_t iett_removeLocalRetainedMessages(ieutThreadData_t *pThreadData,
                                         const char *topicString)
{
    int32_t rc = OK;
    iettTopic_t topic = {0};

    ieutTRACEL(pThreadData, topicString,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "topicString='%s'\n", __func__, topicString);

    topic.destinationType = ismDESTINATION_REGEX_TOPIC;
    topic.topicString = topicString;

    rc = iett_analyseTopicString(pThreadData, &topic);

    if (rc == OK)
    {
        assert(topic.substrings == NULL);
        assert(topic.substringHashes == NULL);
        assert(topic.wildcards == NULL);
        assert(topic.multicards == NULL);

        // Remove all messages for the specified topics
        rc = iett_removeRetainedMessages(pThreadData,
                                         ismEngine_serverGlobal.maintree,
                                         &topic);
    }

    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);
        ism_regex_free(topic.regex);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Ensure we have a topic record in the store.
///
/// @param[in]     pThreadData       Thread data for the calling thread
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_ensureEngineStoreTopicRecord(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    iettTopicTree_t  *tree = ismEngine_serverGlobal.maintree;

    if (tree->retStoreHandle == ismSTORE_NULL_HANDLE)
    {
        ismEngine_getRWLockForWrite(&tree->topicsLock);

        // Make sure we have a topic record in the store.
        if (tree->retStoreHandle == ismSTORE_NULL_HANDLE)
        {
            rc = iett_createEngineStoreTopicRecord(pThreadData);
            assert(rc != OK || tree->retStoreHandle != ismSTORE_NULL_HANDLE);
        }

        ismEngine_unlockRWLock(&tree->topicsLock);
    }

    return rc;
}

//****************************************************************************
/// @brief Update the retained message for a given topic as part of the publish
///        operation.
///
/// @param[in]     pThreadData         Thread data for the calling thread
/// @param[in]     topicString         The topic string to update.
/// @param[in]     message             The message being retained
/// @param[in]     publishSUV          The SUV for the subscriber list we're going to
///                                    publish this message to
/// @param[in]     pTran               The transaction under which this is happening
/// @param[in]     options             iedsPUBLISH_ options for this request.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_updateRetainedMessage(ieutThreadData_t *pThreadData,
                                   const char *topicString,
                                   ismEngine_Message_t *message,
                                   uint64_t publishSUV,
                                   ismEngine_Transaction_t *pTran,
                                   uint32_t options)
{
    int32_t rc;
    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    iettTopicNode_t *topicNode = NULL;
    ismStore_Handle_t storeRefHandle = ismSTORE_NULL_HANDLE;
    iettSLEUpdateRetained_t SLE;
    uint32_t storeOperations = 0;
    bool potentialRepublish = (options & iedsPUBLISH_OPTION_POTENTIAL_REPUBLISH) != 0;
    bool repositioningRetained = (options & iedsPUBLISH_OPTION_REPOSITIONING_RETAINED) != 0;

    assert(pTran != NULL);

    ieutTRACEL(pThreadData, options,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "topicString='%s' options=0x%08x\n",
               __func__, topicString, options);

    // Get values out of the message properties
    size_t proplen = 0;
    char *propp = NULL;
    for (uint32_t i = 0; i < message->AreaCount; i++)
    {
        if (message->AreaTypes[i] == ismMESSAGE_AREA_PROPERTIES)
        {
            proplen = message->AreaLengths[i];
            propp = (char *) message->pAreaData[i];
            break;
        }
    }

    // We have properties
    assert((proplen != 0) && (propp != NULL));

    concat_alloc_t  props = {propp, proplen, proplen};
    ism_field_t field;

    // The properties contain an originating serverUID
    field.val.s = NULL;
    ism_common_findPropertyID(&props, ID_OriginServer, &field);
    const char * serverUID = field.val.s;
    assert(serverUID != NULL);

    // The properties contain an originating server timestamp
    field.val.l = 0;
    ism_common_findPropertyID(&props, ID_ServerTime, &field);
    uint64_t serverTimestamp = (uint64_t)field.val.l;
    assert(serverTimestamp != 0);

    ieutTRACEL(pThreadData, serverTimestamp, ENGINE_HIGH_TRACE, "serverUID='%s' serverTimestamp=%lu\n", serverUID, serverTimestamp);

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = topicString;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

    rc = iett_analyseTopicString(pThreadData, &topic);

    if (rc != OK) goto mod_exit_no_release;

    iettTopicTree_t  *tree = ismEngine_serverGlobal.maintree;

    ismEngine_getRWLockForWrite(&tree->topicsLock);

    // Look for the topic node, if we're repositioning a retained msg, we are only interested in
    // the node if we don't have to add it.
    rc = iett_insertOrFindTopicsNode(pThreadData,
                                     tree->topics,
                                     &topic,
                                     repositioningRetained ? iettOP_FIND : iettOP_ADD,
                                     &topicNode);

    if (rc != OK) goto mod_exit;

    // We now have a pending update
    topicNode->pendingUpdates++;

    // This is now an in-flight retained message, increment the thread stats
    pThreadData->stats.internalRetainedMsgCount++;

    ism_time_t highestInflightRetTimestamp = 0;

    // If we're repositioning the retained message, we only want to proceed if current timestamp is still
    // the one we're sending in...
    if (repositioningRetained)
    {
        if (serverTimestamp != topicNode->currRetTimestamp)
        {
            ieutTRACEL(pThreadData, serverTimestamp, ENGINE_HIGH_TRACE,
                       "No Repositioning required (node timestamp %lu)\n", topicNode->currRetTimestamp);

            assert(storeOperations == 0);

            rc = ISMRC_NotFound;

            goto mod_exit;
        }
    }
    // If the timestamp on this message is less than or equal to the current
    // timestamp for this topic, or one in the list of inflight SLEs
    // then it is either older than or the same message as an existing retained
    // message - so we don't try retaining it and the caller might not publish it.
    else if ((serverTimestamp <= topicNode->currRetTimestamp) ||
             (serverTimestamp <= (highestInflightRetTimestamp = iett_findHighestInflightRetTimestamp(pThreadData, topicNode))))
    {
        ieutTRACEL(pThreadData, serverTimestamp, ENGINE_UNUSUAL_TRACE,
                   "New timestamp (%lu) superseded by either current (%lu) or highest inflight (%lu) [options=0x%08x, serverUID='%s']\n",
                   serverTimestamp, topicNode->currRetTimestamp, highestInflightRetTimestamp, options, serverUID);

        assert(storeOperations == 0);

        rc = ISMRC_OldTimestamp;

        goto mod_exit;
    }

    // Locate the origin server record for this serverUID
    iettOriginServer_t *originServer;

    rc = iett_insertOrFindOriginServer(pThreadData,
                                       serverUID,
                                       iettOP_ADD,
                                       &originServer);

    if (rc != OK) goto mod_exit;

    assert(originServer != NULL);

    // The minimum active orderId required for this retained message
    uint64_t activeOrderIdVote = 0;

    // If the message needs to be persistent we need to write things to the store
    if (message->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
    {
        // Update the retained orderid
        tree->retNextOrderId += 1;

        ismStore_Reference_t msgRef = {0};

        activeOrderIdVote = msgRef.OrderId = tree->retNextOrderId;

        msgRef.State = originServer->localServer ? iettRMR_STATE_ORIGINSERVER_LOCAL :
                                                   iettRMR_STATE_NONE;

        // For an fAsStoreTran transaction, we delay adding to the store until we know
        // this message is going to be committed as the retained message. For any other
        // transaction, we need something in the transaction
        if (pTran->fAsStoreTran == false)
        {
            // Remember the store usage of the message.
            __uint128_t oldWhole = message->StoreMsg.whole;

            do
            {
                assert(storeOperations == 0);

                // We want the message to be stored in the same generation as the reference
                // to it - so it must not already be in the store.
                //
                // This should only ever be the case for a WILL message.
                assert(message->StoreMsg.parts.hStoreMsg == ismSTORE_NULL_HANDLE);

                rc = iest_storeMessage(pThreadData,
                                       message,
                                       2,
                                       iestStoreMessageOptions_EXISTING_TRANSACTION,
                                       &msgRef.hRefHandle);

                if (rc == OK)
                {
                    storeOperations++;

                    assert(msgRef.hRefHandle != ismSTORE_NULL_HANDLE);
                    assert(message->StoreMsg.parts.hStoreMsg != ismSTORE_NULL_HANDLE);
                    assert(msgRef.hRefHandle == message->StoreMsg.parts.hStoreMsg);
                    assert(tree->retRefContext != NULL);

                    rc = ism_store_createReference(pThreadData->hStream,
                                                   tree->retRefContext,
                                                   &msgRef,
                                                   tree->retMinActiveOrderId,
                                                   &storeRefHandle);
                }

                if (rc == OK)
                {
                    storeOperations++;

                    // No code to deal with remembering this is a repositioningRetained request
                    // for a rehydrated transaction ref (probably use State of tranRef if needed)
                    assert(repositioningRetained == false);

                    // Record the creation of this reference as part of the transaction
                    // in case the server ends before this transaction has completed, and
                    // we need to remove the reference
                    rc = ietr_createTranRef( pThreadData
                                           , pTran
                                           , storeRefHandle
                                           , iestTOR_VALUE_PUT_MESSAGE
                                           , 0
                                           , &SLE.TranRef);
                }

                if (rc == OK)
                {
                    storeOperations++;
                }

                // One of the previous store operations went wrong, we need to roll back
                // any changes we made.
                if (rc != OK)
                {
                    if (storeOperations != 0)
                    {
                        iest_store_rollback(pThreadData, false);
                        storeOperations = 0;
                    }

                    // This message was never actually stored (or unstored and re-stored)
                    // make sure we leave it as we found it.
                    message->StoreMsg.whole = oldWhole;
                }
            }
            while(rc == ISMRC_StoreGenerationFull);

            if (rc != OK) goto mod_exit;
        }
    }

    // Update the tree stats
    tree->topicsUpdates++;
    tree->retUpdates++;

    SLE.topicNode = topicNode;
    SLE.message = message;
    SLE.refHandle = storeRefHandle;
    SLE.orderId = activeOrderIdVote;
    SLE.originServer = originServer;
    SLE.timestamp = serverTimestamp;
    SLE.nextInflightRetUpdate = NULL;
    SLE.publishSUV = publishSUV;
    SLE.savepointRollback = false;
    SLE.repositioningRetained = repositioningRetained;

    ietrSLE_Header_t *pSLE;

    rc = ietr_softLogAdd( pThreadData
                        , pTran
                        , ietrSLE_TT_UPDATERETAINED
                        , iett_SLEReplayUpdateRetained
                        , NULL
                        , Commit | Rollback | PostCommit | PostRollback | Cleanup | SavepointRollback
                        , &SLE
                        , sizeof(SLE)
                        , pTran->fAsStoreTran ? 2 : 1
                        , 1
                        , &pSLE );

    if (rc != OK) goto mod_exit;

    // Add this SLE to the inflight set for this topic
    iett_addInflightRetUpdate(pThreadData, topicNode, (iettSLEUpdateRetained_t *)(pSLE+1));

    // Make sure that we vote to keep at least this orderId
    if (topicNode->activeOrderIdVote == 0)
    {
        topicNode->activeOrderIdVote = activeOrderIdVote;
    }
    else
    {
        assert((activeOrderIdVote == 0) || (activeOrderIdVote > topicNode->activeOrderIdVote));
    }

    // Record that this message is now being referenced twice, one will be released
    // when the retained is committed, the other when we have delivered to late
    // subscribers
    iem_recordMessageMultipleUsage(message, 2);

mod_exit:

    // If this failed we need to clean up
    if (rc != OK)
    {
        // Roll back anything that we had done above
        if (storeOperations != 0)
        {
            iest_store_rollback(pThreadData, false);
            storeOperations = 0;
        }

        iettTopicNode_t *removedTree = NULL;

        if (topicNode != NULL)
        {
            topicNode->pendingUpdates--;
            pThreadData->stats.internalRetainedMsgCount--;

            removedTree = iett_removeUnusedTree(pThreadData, tree, topicNode);
        }

        assert(removedTree != ismEngine_serverGlobal.maintree->topics);

        ismEngine_unlockRWLock(&tree->topicsLock);

        // We have a tree that got removed because of a problem.
        if (removedTree != NULL)
        {
            iettDestroyTopicsTreeCbContext_t destroyCbContext;

            destroyCbContext.freeingEngineTree = false;

            iett_destroyTopicsTreeCallback(pThreadData, NULL, 0, removedTree, &destroyCbContext);
        }

        // If this is an old timestamp and it did not come from a source that is potentially
        // republishing a message, return OK to the caller so that the publish will continue.
        if (rc == ISMRC_OldTimestamp && potentialRepublish == false)
        {
            ieutTRACEL(pThreadData, rc, ENGINE_HIGH_TRACE, "Ignoring ISMRC_OldTimestamp for non-republish request\n");
            rc = OK;
        }
    }
    else
    {
        // Assign this retained message to the resourceSet of the topicNode.
        assert(topicNode != NULL);

        iereResourceSetHandle_t resourceSet = topicNode->resourceSet;

        iere_primeThreadCache(pThreadData, resourceSet);
        iere_updateMessageResourceSet(pThreadData, resourceSet, message, false, true);

        ismEngine_unlockRWLock(&tree->topicsLock);
    }

mod_exit_no_release:

    if (NULL != topic.topicStringCopy)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
// Forward declaration of iett_scanTopicsTreeNode
//****************************************************************************
void iett_scanTopicsTreeNode(ieutThreadData_t *pThreadData,
                             iettTopicNode_t *node,
                             iettScanTopicsTreeCbContext_t *context);

//****************************************************************************
/// @brief Callback used when scanning a topics tree
///
/// @param[in]     pThreadData     Current thread context
/// @param[in]     key             Key being processed (topic substring)
/// @param[in]     keyHash         Hash for the key being processed
/// @param[in]     value           Value from the hash table (iettTopicNode_t)
/// @param[in,out] context         Context information
///
/// @see iett_dumpTopicTree
//****************************************************************************
void iett_scanTopicsTreeCallback(ieutThreadData_t *pThreadData,
                                 char *key,
                                 uint32_t keyHash,
                                 void *value,
                                 void *context)
{
    iett_scanTopicsTreeNode(pThreadData,
                            (iettTopicNode_t *)value,
                            (iettScanTopicsTreeCbContext_t *)context);
}

//****************************************************************************
/// @brief Scan a node and traverse to all nodes below
///
/// @param[in]     pThreadData   Current thread context
/// @param[in]     node          Node from which to begin processing.
/// @param[in,out] context       Context of the scan
///
/// @see iett_dumpTopicTree
//****************************************************************************
void iett_scanTopicsTreeNode(ieutThreadData_t *pThreadData,
                             iettTopicNode_t *node,
                             iettScanTopicsTreeCbContext_t *context)
{
    if (node->activeOrderIdVote != 0)
    {
        // See if this is one of our lowest orderIds
        uint32_t pos;
        uint32_t voteCount = context->activeOrderIdVoteCount;
        if (voteCount < iettLOWEST_ORDERID_ARRAY_SIZE)
        {
            for(pos=0; pos<voteCount; pos++)
            {
                if (context->lowestOrderIdNode[pos]->activeOrderIdVote < node->activeOrderIdVote)
                {
                    memmove(&context->lowestOrderIdNode[pos+1],
                            &context->lowestOrderIdNode[pos],
                            (voteCount-pos)*sizeof(context->lowestOrderIdNode[0]));
                    break;
                }
            }

            context->lowestOrderIdNode[pos] = node;
        }
        else if (context->lowestOrderIdNode[0]->activeOrderIdVote > node->activeOrderIdVote)
        {
            int32_t start=0;
            int32_t end=iettLOWEST_ORDERID_ARRAY_SIZE;

            // The set is sorted, so we use a binary split search
            while(start != end)
            {
                int32_t mid = start+((end-start)/2);

                assert(node->activeOrderIdVote != context->lowestOrderIdNode[mid]->activeOrderIdVote);

                if (node->activeOrderIdVote > context->lowestOrderIdNode[mid]->activeOrderIdVote)
                {
                    end = mid;
                }
                else
                {
                    start = mid + 1;
                }
            }

            pos = start-1;

            memmove(&context->lowestOrderIdNode[0],
                    &context->lowestOrderIdNode[1],
                    pos*sizeof(context->lowestOrderIdNode[0]));

            context->lowestOrderIdNode[pos] = node;
        }

        context->activeOrderIdVoteCount += 1;

        // Quick confirmation that the array is ordered largest->smallest orderId as expected
        assert(context->activeOrderIdVoteCount < 2 ||
               context->lowestOrderIdNode[0]->activeOrderIdVote > context->lowestOrderIdNode[1]->activeOrderIdVote);

        if ((context->minActiveOrderIdVote == 0) || (node->activeOrderIdVote < context->minActiveOrderIdVote))
        {
            context->minActiveOrderIdVote = node->activeOrderIdVote;
        }

        if (node->activeOrderIdVote > context->maxActiveOrderIdVote)
        {
            context->maxActiveOrderIdVote = node->activeOrderIdVote;
        }
    }

    if (node->children)
    {
        ieut_traverseHashTable(pThreadData,
                               node->children,
                               iett_scanTopicsTreeCallback,
                               context);
    }
}

//****************************************************************************
/// @brief Reposition an existing retained msg (giving it a higher orderid)
///
/// @param[in]     pMessage         The retained message being repositioned.
/// @param[in]     pContext         Optional context for completion callback
/// @param[in]     contextLength    Length of data pointed to by pContext
/// @param[in]     pCallbackFn      Operation-completion callback
///
/// @remark The message being repositioned should have had its use count incremented,
/// a copy is made and the copy repositioned.
///
/// @return ISMRC indicating the outcome.
//****************************************************************************
static int32_t iett_repositionRetainedMsg(ieutThreadData_t *pThreadData,
                                          ismEngine_Message_t *pMessage,
                                          void *pContext,
                                          size_t contextLength,
                                          ismEngine_CompletionCallback_t pCallbackFn)
{
    int32_t rc;
    ismEngine_Message_t *pNewMessage = NULL;

    ieutTRACEL(pThreadData, pMessage, ENGINE_FNC_TRACE, FUNCTION_ENTRY "(pMessage %p)\n", __func__, pMessage);

    assert((pMessage->Header.Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN) != 0);

    rc = iem_createMessageCopy(pThreadData,
                               pMessage,
                               true, 0, 0, &pNewMessage);

    // We should release the original message now (whether or not we got a copy)
    iem_releaseMessage(pThreadData, pMessage);

    if (rc != OK) goto mod_exit;

    assert(pNewMessage != NULL);

    // Get the topic string out of the message
    concat_alloc_t  props;
    ism_field_t field = {0};

    iem_locateMessageProperties(pNewMessage, &props);

    ism_common_findPropertyID(&props, ID_Topic, &field);

    assert(field.type == VT_String);
    assert(field.val.s != NULL);

    const char * topicString = field.val.s;
    assert(iett_validateTopicString(pThreadData, topicString, iettVALIDATE_FOR_PUBLISH) == true);

    pNewMessage->Header.Flags &= ~ismMESSAGE_FLAGS_PROPAGATE_RETAINED;

    assert((pNewMessage->Header.Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN) != 0);
    assert(pNewMessage->Header.Persistence == pNewMessage->Header.Persistence);
    assert(pNewMessage->Header.Expiry == pNewMessage->Header.Expiry);

    ietrAsyncTransactionDataHandle_t hAsyncData = NULL;

    // We don't allow this publish to pause if the Async callback queue is alerted, because
    // we could be running on a timer thread, and we don't want to delay the timer.
    rc = ieds_publish(pThreadData,
                      NULL, // No client
                      topicString,
                      iedsPUBLISH_OPTION_REPOSITIONING_RETAINED |
                      iedsPUBLISH_OPTION_FAIL_IF_ASYNC_CBQ_ALERTED,
                      NULL, // No transaction
                      pNewMessage,
                      0,
                      NULL,
                      contextLength,
                      &hAsyncData);

    if (rc == ISMRC_NeedStoreCommit)
    {
        //The publish wants to go async.... get ready
        rc = setupAsyncPublish( pThreadData
                              , NULL
                              , NULL
                              , pContext
                              , contextLength
                              , pCallbackFn
                              , &hAsyncData);

        if (rc == ISMRC_AsyncCompletion)
        {
            goto mod_exit;
        }
    }

mod_exit:

    if (pNewMessage != NULL) iem_releaseMessage(pThreadData, pNewMessage);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief  Called to update the count of remaining messages in this repositioning scan
///
/// @param[in]     retcode                 Return code indicating success of the operation
/// @param[in]     scanRepositionContext   Reposition context of finishing request
/// @param[in]     allowAnotherReposition  Whether, if this is the last message to
///                                        allow another reposition scan to start.
///
//****************************************************************************
void iett_scanRepositionFinished(int32_t retcode,
                                 iettScanRepositionContext_t *scanRepositionContext,
                                 bool allowAnotherReposition)
{
    uint32_t remainingMsgs = __sync_sub_and_fetch(&scanRepositionContext->batchSize, 1);

    // None left, we're all done.
    if (remainingMsgs == 0)
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();

        uint32_t scansSoFar = scanRepositionContext->scanNumber;
        uint32_t maxScans = scanRepositionContext->maxScans;

        // Check that we haven't exhausted the maximum number of scans allowed
        if ((allowAnotherReposition == true) && (maxScans != 0) && (scansSoFar >= maxScans))
        {
            allowAnotherReposition = false;
        }

        assert(pThreadData != NULL);

        ieutTRACEL(pThreadData, scanRepositionContext, ENGINE_INTERESTING_TRACE,
                   FUNCTION_IDENT "scanRepositionContext=%p [%lu-%lu] Scans=%u/%u\n", __func__,
                   scanRepositionContext,
                   scanRepositionContext->firstOrderId, scanRepositionContext->lastOrderId,
                   scansSoFar, maxScans);

        iemem_free(pThreadData, iemem_callbackContext, scanRepositionContext);

        // This reposition is no longer in progress.
        DEBUG_ONLY bool swapped = __sync_bool_compare_and_swap(&scanRepositionInProgress, true, false);
        assert(swapped == true);

        assert(ismEngine_serverGlobal.retainedRepositioningEnabled == true);

        // Do another scan to update the minActiveOrderId and possibly do more work
        iett_scanForRetMinActiveOrderId(pThreadData, scansSoFar, allowAnotherReposition);
    }
}

//****************************************************************************
/// @brief  Callback to be called when the repositioning of a retained message
/// within a batch built from a scan request finishes
///
/// @param[in]     retcode          Return code indicating success of the operation
/// @param[in]     handle           For operations that create a resource, its handle
/// @param[in]     pContext         Optional context for completion callback
///
//****************************************************************************
void iett_scanRepositionFinishedAsync(int32_t retcode, void *handle, void *pContext)
{
    iett_scanRepositionFinished(retcode, *(iettScanRepositionContext_t **)pContext, true);
}

//*****************************************************************************
/// @brief Scans the topic tree to determine the lowest minimum active orderid
///        for retained messages and optionally to reposition messages with low
///        orderIds.
///
/// @param[in]     pThreadData         Current thread context
/// @param[in]     scansSoFar          How many scans we've already done this
///                                    sequence
/// @param[in]     allowRepositioning  Whether to allow repositioning or not.
///
/// @remark The scan takes the read lock, and calculates the new minimum
/// active orderId for retained messages in the tree.
///
/// At the end of a scan, a calculation is made to determine whether repositioning
/// of messages with lower orderids should be carried out, and if the caller
/// allows, it does so.
///
/// @remark At the end of a repositioning request, another call will be made to
/// see if more repositioning is required.
///
/// @return true if repositioning was initiated.
//******************************************************************************
bool iett_scanForRetMinActiveOrderId(ieutThreadData_t *pThreadData,
                                     uint32_t scansSoFar,
                                     bool allowRepositioning)
{
    iettScanRepositionContext_t *scanRepositionContext = NULL;

    iettTopicTree_t *tree = iett_getEngineTopicTree(pThreadData);

    iettScanTopicsTreeCbContext_t scanContext = {tree, 0, 0, 0, {0}};

    ieutTRACEL(pThreadData, tree, ENGINE_FNC_TRACE, FUNCTION_ENTRY "tree=%p, allowRepositioning=%d\n",
               __func__, tree, (int)allowRepositioning);

    if (tree != NULL)
    {
        ismEngine_getRWLockForRead(&tree->topicsLock);

        iett_scanTopicsTreeNode(pThreadData, tree->topics, &scanContext);

        // If we have a minimum active orderId, set it.
        if (scanContext.minActiveOrderIdVote != 0)
        {
            tree->retMinActiveOrderId = scanContext.minActiveOrderIdVote;
        }

        // Calculate the wasted orderIds to decide if we need to reposition messages.
        if (scanContext.minActiveOrderIdVote != 0)
        {
            uint64_t firstOrderId = 0;
            uint64_t lastOrderId = 0;

            bool attemptRepositioning;

            assert(scanContext.activeOrderIdVoteCount > 0);

            uint64_t activeOrderIdSpread = (scanContext.maxActiveOrderIdVote - scanContext.minActiveOrderIdVote)+1;
            double orderIdsInUsePercent = (((double)scanContext.activeOrderIdVoteCount)/((double)activeOrderIdSpread))*100.0L;

            // If this scan is allowed to attempt repositioning, see if it is needed.
            if (allowRepositioning == true)
            {
                // If either the store's small buffers are at a warning level, or we are using less than
                // 20% of the orderId spread, let's attempt to reposition messages (if one isn't already in progress).
                if (((ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_0] == StatusWarning) ||
                    ((activeOrderIdSpread > iettORDERID_SPREAD_REPOSITION_LWM) &&
                     (orderIdsInUsePercent < iettORDERID_SPREAD_INUSE_PERCENT_LWM))))
                {
                    // We only want one reposition in progress at a time.
                    attemptRepositioning = __sync_bool_compare_and_swap(&scanRepositionInProgress, false, true);
                }
                else
                {
                    attemptRepositioning = false;
                }
            }
            else
            {
                attemptRepositioning = false;
            }

            // If we want to attempt repositioning need to set up the batch
            if (attemptRepositioning == true)
            {
                uint32_t repositionBatchSize = iettLOWEST_ORDERID_ARRAY_SIZE;

                if (scanContext.activeOrderIdVoteCount < repositionBatchSize)
                {
                    repositionBatchSize = scanContext.activeOrderIdVoteCount;
                }

                scanRepositionContext = iemem_malloc(pThreadData,
                                                     IEMEM_PROBE(iemem_callbackContext, 18),
                                                     sizeof(iettScanRepositionContext_t) +
                                                     repositionBatchSize * sizeof(ismEngine_Message_t *));

                if (scanRepositionContext != NULL)
                {
                    scanRepositionContext->repositionMessages = (ismEngine_Message_t **)(scanRepositionContext+1);

                    // Make sure the messages we're going to reposition remain addressable, we put them into the
                    // array in reverse order so that the earliest one we reposition is the one with the lowest
                    // orderId.
                    int32_t pos = 0;
                    for(int32_t i=(int32_t)(repositionBatchSize-1); i>=0; i--)
                    {
                        iettTopicNode_t *topicNode = scanContext.lowestOrderIdNode[i];

                        assert(topicNode != NULL);

                        uint64_t thisOrderId = topicNode->activeOrderIdVote;
                        ismEngine_Message_t *pMessage = topicNode->currRetMessage;

                        if (pMessage != NULL)
                        {
                            assert(thisOrderId > lastOrderId);
                            if (pos == 0) firstOrderId = thisOrderId;
                            lastOrderId = thisOrderId;
                            iem_recordMessageUsage(pMessage);
                            scanRepositionContext->repositionMessages[pos++] = pMessage;
                        }
                        else
                        {
                            ieutTRACEL(pThreadData, thisOrderId, ENGINE_INTERESTING_TRACE,
                                       "Node %p with orderId %lu has no message (pendingUpdates=%u)\n",
                                       topicNode, thisOrderId, topicNode->pendingUpdates);
                        }
                    }

                    if (pos == 0)
                    {
                        iemem_free(pThreadData, iemem_callbackContext, scanRepositionContext);
                        scanRepositionContext = NULL;
                    }
                    else
                    {
                        scanRepositionContext->batchSize = pos;
                        scanRepositionContext->firstOrderId = firstOrderId;
                        scanRepositionContext->lastOrderId = lastOrderId;
                        scanRepositionContext->scanNumber = scansSoFar+1;
                        scanRepositionContext->maxScans = ismEngine_serverGlobal.RetMinActiveOrderIdMaxScans;
                    }
                }

                // For some reason, we are not going to actually do a reposition, reset the InProgress flag.
                if (scanRepositionContext == NULL)
                {
                    DEBUG_ONLY bool swapped = __sync_bool_compare_and_swap(&scanRepositionInProgress, true, false);
                    assert(swapped == true);
                }
            }

            ieutTRACEL(pThreadData, orderIdsInUsePercent, ENGINE_INTERESTING_TRACE, FUNCTION_IDENT
                       "scanRepositionContext=%p [%lu-%lu] allow=%d attempt=%d spread=%lu inUse=%.4f%% retMinActiveOrderId=%lu memStatus=%d\n",
                       __func__, scanRepositionContext, firstOrderId, lastOrderId, (int)allowRepositioning, (int)attemptRepositioning,
                       activeOrderIdSpread, orderIdsInUsePercent, tree->retMinActiveOrderId,
                       (int)(ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_0]));
        }

        ismEngine_unlockRWLock(&tree->topicsLock);

        // Reposition any identified messages
        if (scanRepositionContext != NULL)
        {
            uint32_t repositionCount = scanRepositionContext->batchSize;
            iettScanRepositionContext_t *pRepositionContext = scanRepositionContext;

            assert(repositionCount != 0);

            uint32_t msgNo;
            int32_t rc;
            for(msgNo=0; msgNo<repositionCount; msgNo++)
            {
                rc = iett_repositionRetainedMsg(pThreadData,
                                                scanRepositionContext->repositionMessages[msgNo],
                                                &pRepositionContext,
                                                sizeof(pRepositionContext),
                                                iett_scanRepositionFinishedAsync);

                if (rc != ISMRC_AsyncCompletion)
                {
                    // If we failed to publish because the async callback queue is alerted, ensure
                    // that we don't try and do another scan this time. We need to do this before
                    // calling iett_scanRepositionFinished so we know there is at least one remaining
                    // message, and thus pRepositionContext will be valid.
                    if (rc == ISMRC_AsyncCBQAlerted)
                    {
                        assert(pRepositionContext->scanNumber >= 1);
                        pRepositionContext->maxScans = 1;
                    }

                    iett_scanRepositionFinished(rc, pRepositionContext, false);

                    if (rc == ISMRC_AsyncCBQAlerted) break;
                }
            }

            // Tidy up if we stopped repositioning
            for(msgNo++; msgNo<repositionCount; msgNo++)
            {
                iem_releaseMessage(pThreadData, scanRepositionContext->repositionMessages[msgNo]);
                assert(rc != OK); // Report the failing rc for all
                iett_scanRepositionFinished(rc, pRepositionContext, false);
            }
        }
    }

    ieutTRACEL(pThreadData, scanContext.minActiveOrderIdVote, ENGINE_FNC_TRACE, FUNCTION_EXIT "context.minActiveOrderIdVote=%lu\n",
               __func__, scanContext.minActiveOrderIdVote);

    // If the pointer is not NULL, we initiated a reposition
    return (scanRepositionContext != NULL);
}

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
/// @remark The ismEngine_MessageCallback_t function must release the message when it is
///  finished
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
           ismEngine_CompletionCallback_t  pCallbackFn)
{
    int32_t rc = OK;
    iettTopic_t topic = {0};
    const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *wildcards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    const char *multicards[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ismEngine_ClientState_t *pClient = pSession->pClient;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pClient);

    ieutTRACEL(pThreadData,topicString,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "topicString=%s\n", __func__,topicString);

    // Check that the requester is authorized to subscribe on the specified topic
    rc = ismEngine_security_validate_policy_func(pClient->pSecContext,
                                                 ismSEC_AUTH_TOPIC,
                                                 topicString,
                                                 ismSEC_AUTH_ACTION_SUBSCRIBE,
                                                 ISM_CONFIG_COMP_ENGINE,
                                                 NULL);

    if (rc != OK) goto mod_exit;

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = topicString;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

    rc = iett_analyseTopicString(pThreadData, &topic);

    if (rc != OK) goto mod_exit;

    uint32_t maxNodes = 0;
    uint32_t nodeCount = 0;
    iettTopicNode_t *topicNode = NULL;
    iettTopicNode_t **topicNodes = NULL;
    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ismEngine_MessageHandle_t *foundMessages = NULL;
    uint32_t foundMessageCount = 0;
    uint32_t foundMessageMax = 0;
    uint32_t foundMessageIncrement;

    ismEngine_getRWLockForRead(&tree->topicsLock);

    // No wildcards, so we know which node to look for
    if (topic.wildcardCount == 0 && topic.multicardCount == 0)
    {
        rc = iett_insertOrFindTopicsNode(pThreadData, tree->topics, &topic, iettOP_FIND, &topicNode);

        if (rc == OK)
        {
            topicNodes = &topicNode;
            nodeCount = 1;
            foundMessageIncrement = 1;
        }
    }
    else
    {
        rc = iett_findMatchingTopicsNodes(pThreadData,
                                          tree->topics, false,
                                          &topic,
                                          0, 0, 0,
                                          NULL, &maxNodes, &nodeCount, &topicNodes);

        foundMessageIncrement = 10000;
    }

    ismEngine_Message_t *pMessage;

    if (rc == OK)
    {
        const uint32_t nowExpiry = ism_common_nowExpire();

        for(uint32_t i=0; i<nodeCount; i++)
        {
            pMessage = topicNodes[i]->currRetMessage;

            // If there is a message consider adding it to our list
            if (pMessage != NULL)
            {
                // Only add the message to the list if is not a NULL retained and has not expired.
                if ((pMessage->Header.MessageType != MTYPE_NullRetained) &&
                    ((pMessage->Header.Expiry == 0) || (pMessage->Header.Expiry > nowExpiry)))
                {
                    if (foundMessageCount == foundMessageMax)
                    {
                        uint32_t newFoundMessageMax = foundMessageMax + foundMessageIncrement;
                        ismEngine_MessageHandle_t *newFoundMessages;

                        newFoundMessages = iemem_realloc(pThreadData,
                                                         IEMEM_PROBE(iemem_callbackContext, 11),
                                                         foundMessages,
                                                         sizeof(ismEngine_MessageHandle_t)*newFoundMessageMax);

                        if (newFoundMessages == NULL)
                        {
                            rc = ISMRC_AllocateError;
                            ism_common_setError(rc);

                            // Don't process the list we built up already
                            foundMessageCount = 0;
                            break;
                        }

                        foundMessages = newFoundMessages;
                        foundMessageMax = newFoundMessageMax;
                    }

                    foundMessages[foundMessageCount++] = pMessage;
                }
            }
        }

        for(uint32_t i=0; i<foundMessageCount; i++)
        {
            iem_recordMessageUsage(foundMessages[i]);
        }
    }

    ismEngine_unlockRWLock(&tree->topicsLock);

    // Free the memory we're using for the list of topic nodes if it is allocated
    if (topicNodes != NULL && topicNodes != &topicNode) iemem_free(pThreadData, iemem_topicsQuery, topicNodes);

    // If we found some messages we need to call the callback with them all.
    if (foundMessageCount != 0)
    {
        for(uint32_t i=0; i<foundMessageCount; i++)
        {
            pMessage = foundMessages[i];

            //The callback function is responsible for releasing the message
            bool keepRunning = pMessageCallbackFn(NULL,
                                                  ismENGINE_NULL_DELIVERY_HANDLE,
                                                  (ismEngine_MessageHandle_t)pMessage,
                                                  0,
                                                  ismMESSAGE_STATE_CONSUMED,
                                                  ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                                  &(pMessage->Header),
                                                  pMessage->AreaCount,
                                                  pMessage->AreaTypes,
                                                  pMessage->AreaLengths,
                                                  pMessage->pAreaData,
                                                  pMessageContext,
                                                  NULL);

            if (keepRunning == false)
            {
                // Release any messages that the callback function has abandoned
                for(i++; i<foundMessageCount; i++)
                {
                    pMessage = foundMessages[i];
                    iem_releaseMessage(pThreadData, pMessage);
                }
            }
        }
    }
    else
    {
        rc = ISMRC_NotFound;
    }


    if (foundMessages != NULL) iemem_free(pThreadData, iemem_callbackContext, foundMessages);

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
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @brief Comparator for qsort to choose between topic nodes by timestamp.
///
/// @param[in]     data1        first Node to compare
/// @param[in]     data2        second node to compare
//****************************************************************************
static int32_t compareTopicNodeTimestamps(const void *data1, const void *data2)
{
    int32_t rc;

    iettTopicNode_t *node1 = *(iettTopicNode_t **)data1;
    iettTopicNode_t *node2 = *(iettTopicNode_t **)data2;

    if (node1->currRetTimestamp < node2->currRetTimestamp)
    {
        rc = -1;
    }
    else if (node1->currRetTimestamp > node2->currRetTimestamp)
    {
        rc = 1;
    }
    else
    {
        rc = 0;
    }

    return rc;
}

//****************************************************************************
/// @brief Sort an array of topicNodes into order based on the current retained
/// timestamp stored in the node.
///
/// @param[in]     pThreadData    Current thread context
/// @param[in,out] topicNodes     Array of nodes to be sorted
/// @param[in]     nodeCount      Count of nodes in the array
///
/// @remark This function is very useful in putting nodes in an order so that
/// when referenced, they will be in time order (which is often important for
/// retained messages).
//****************************************************************************
void iett_sortTopicNodesByTimestamp(ieutThreadData_t *pThreadData,
                                    iettTopicNode_t **topicNodes,
                                    uint32_t nodeCount)
{
    qsort(topicNodes, nodeCount, sizeof(iettTopicNode_t *), compareTopicNodeTimestamps);
}
