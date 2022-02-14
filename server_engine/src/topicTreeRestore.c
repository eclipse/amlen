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
/// @file  topicTreeRestore.c
/// @brief Engine component topic tree transaction / restore functions
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
#include "queueCommon.h"       // ieq functions & constants
#include "msgCommon.h"         // iem functions & constants
#include "engineStore.h"       // iest functions & constants
#include "multiConsumerQ.h"    // Direct access to iemq_ functions
#include "messageExpiry.h"     // ieme functions & constants
#include "clientState.h"       // iecs functions & constants
#include "engineRestore.h"     // ierr functions & constants
#include "resourceSetStats.h"  // iere functions & constants
#include "engineUtils.h"       // ieut functions & constants

//****************************************************************************
/// @brief Information used to clean up any retained messages which should
///        be removed during restart.
//****************************************************************************
typedef struct tag_iettUnneededRetained_t
{
    ismStore_Handle_t                  retRefHandle;  ///< Retained message ref handle
    uint64_t                           retRefOrderId; ///< Retained message ref orderId
    ismEngine_Message_t               *message;       ///< Message pointer
    struct tag_iettUnneededRetained_t *next;          ///< Next one
} iettUnneededRetained_t;

static iettUnneededRetained_t *unneededRetainedMsgs = NULL;

//*********************************************************************
/// @brief  Context used when reconciling admin shared subscriptions
//*********************************************************************
typedef struct tag_iettReconcileAdminSharedSubsContext_t
{
    const char *subType;
    const char *namespace;
    volatile uint32_t remainingActions; // How many asynchronous actions we are waiting for
} iettReconcileAdminSharedSubsContext_t;

//****************************************************************************
/// @brief Names of the administrative subscriptions used to reinstate the admin
///        sharers flag during recovery (and create subscriptions at the end of
///        recovery).
//****************************************************************************
ieutHashTable_t *allPersistentAdminSubNames = NULL;

// Whether there are any subscriptions that should be updated in the store after recovery
static uint32_t subscriptionsNeedUpdating = 0;

//****************************************************************************
/// @brief  Initialise the topic tree structures used during recovery
///
/// @param[in]     pThreadData        Thread data
///
/// @remark This function initializes various things including the list of
///         expected AdminSubscription and DurableNamespaceAdminSub subscriptions
///         so that they can be identified during recovery.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_initializeRecovery(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    // Get AdminSubscription and DurableNamespaceAdminSub names from the configuration (in both cases, if
    // NULL is returned meaning none are defined, create an empty properties structure so that subsequent
    // code doesn't need to check for NULL.
    ism_prop_t *adminSubscriptionNames = ism_config_getObjectInstanceNames(ismEngine_serverGlobal.configCallbackHandle,
                                                                           ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION);
    if (adminSubscriptionNames == NULL) adminSubscriptionNames = ism_common_newProperties(0);

    ism_prop_t *durableNamespaceAdminSubNames = ism_config_getObjectInstanceNames(ismEngine_serverGlobal.configCallbackHandle,
                                                                                  ismENGINE_ADMIN_VALUE_DURABLENAMESPACEADMINSUB);
    if (durableNamespaceAdminSubNames == NULL) durableNamespaceAdminSubNames = ism_common_newProperties(0);

    uint32_t expectedCount = (uint32_t)(ism_common_getPropertyCount(adminSubscriptionNames)+
                                        ism_common_getPropertyCount(durableNamespaceAdminSubNames));

    ieutTRACEL(pThreadData, expectedCount,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "expectedCount=%u\n", __func__, expectedCount);

    // Create a hash table big enough to contain the names we found
    rc = ieut_createHashTable(pThreadData,
                              ieut_suggestCapacity(pThreadData, expectedCount, 0),
                              iemem_restoreTable, &allPersistentAdminSubNames);

    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Add both AdminSubscription and DurableNamespaceAdminSub names to the hash table.
    ism_prop_t *allNameLists[] = {adminSubscriptionNames, durableNamespaceAdminSubNames, NULL};
    for(int32_t list=0; allNameLists[list] != NULL; list++)
    {
        const char * thisSubName;

        for (int32_t i = 0; ism_common_getPropertyIndex(allNameLists[list], i, &thisSubName) == 0; i++)
        {
            assert(thisSubName != NULL);
            // Assert that the 2 namespaces will not clash (because of leading slash on AdminSubscription names)
            assert((allNameLists[list] == adminSubscriptionNames && thisSubName[0] == '/') ||
                   (allNameLists[list] == durableNamespaceAdminSubNames && thisSubName[0] != '/'));

            rc = ieut_putHashEntry(pThreadData,
                                   allPersistentAdminSubNames,
                                   ieutFLAG_DUPLICATE_KEY_STRING,
                                   thisSubName,
                                   iett_generateSubNameHash(thisSubName),
                                   NULL, 0);

            if (rc != OK)
            {
                ism_common_setError(rc);
                goto mod_exit;
            }
        }
    }

mod_exit:

    ism_common_freeProperties(adminSubscriptionNames);
    ism_common_freeProperties(durableNamespaceAdminSubNames);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief  Callback used if iett_reconcileAdminSharedSubscriptions goes async
//****************************************************************************
static void iett_reconcileAdminSharedSubCallback(int32_t rc, void *handle, void *context)
{
   iettReconcileAdminSharedSubsContext_t *pContext = *(iettReconcileAdminSharedSubsContext_t **)context;
   __sync_sub_and_fetch(&pContext->remainingActions, 1);
}

//****************************************************************************
/// @brief Callback function used to create a missing admin subscription, either
/// called when traversing the hash table of durable subs, or for a non persistent
/// one.
///
/// @return OK if the subscription was created (sync or async) otherwise an error.
//****************************************************************************
static int32_t iett_reconcileAdminSharedSub(ieutThreadData_t *pThreadData,
                                            char *thisSubName,
                                            uint32_t thisSubNameHash,
                                            void *value,
                                            void *context)
{
    int32_t rc = OK;
    iettReconcileAdminSharedSubsContext_t *pContext = context;
    const char *thisSubType = pContext->subType;
    const char *thisNamespace = pContext->namespace;

    assert(thisSubName != NULL);

    if (thisSubType == NULL)
    {
        assert(thisNamespace == NULL);

        if (thisSubName[0] == '/')
        {
            thisSubType = ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION;
            thisNamespace = ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED;
        }
        else
        {
            thisSubType = ismENGINE_ADMIN_VALUE_DURABLENAMESPACEADMINSUB;
            thisNamespace = ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE;
        }
    }
    else
    {
        assert(thisNamespace != NULL);
    }

    ieutTRACEL(pThreadData, thisSubType,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "thisSubType=%s thisNamespace=%s thisSubName='%s'\n",
               __func__, thisSubType, thisNamespace, thisSubName);

    // Get configuration information for this subscriptions
    ism_prop_t *subProps = ism_config_getProperties(ismEngine_serverGlobal.configCallbackHandle,
                                                    thisSubType,
                                                    thisSubName);

    if (subProps != NULL)
    {
        __sync_add_and_fetch(&pContext->remainingActions, 1);
        rc = iett_createAdminSharedSubscription(pThreadData,
                                                thisNamespace,
                                                thisSubName,
                                                subProps,
                                                thisSubType,
                                                &pContext, sizeof(pContext), iett_reconcileAdminSharedSubCallback);
        ism_common_freeProperties(subProps);

        if (rc == ISMRC_AsyncCompletion)
        {
            rc = OK;
        }
        else
        {
            iett_reconcileAdminSharedSubCallback(rc, NULL, &pContext);
            if (rc != OK)
            {
                ism_common_setError(rc);
            }
        }
    }
    else
    {
        rc = ISMRC_NotFound;
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}


//****************************************************************************
/// @brief  Create and administrative subscriptions not recovered
///
/// @param[in]     pThreadData        Thread data
///
/// @remark Create AdminSubscription and DurableNamespaceAdminSubs not found during
///         recovery plus NonpersistentAdminSubs which wouldn't expect to be found.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_reconcileAdminSharedSubscriptions(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;
    int32_t waitRC;

    iettReconcileAdminSharedSubsContext_t context = {0};
    iettReconcileAdminSharedSubsContext_t *pContext = &context;

    ieutTRACEL(pThreadData, NULL,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Create any persistent subscriptions which appear in the config, but were not recovered.
    // This can happen if the store is cleared and so we need to make a fresh set of subscriptions.
    rc = ieut_traverseHashTableWithRC(pThreadData,
                                      allPersistentAdminSubNames,
                                      iett_reconcileAdminSharedSub,
                                      pContext);

    ieut_destroyHashTable(pThreadData, allPersistentAdminSubNames);

    if (rc != OK) goto mod_exit;

    // Get The NonpersistentAdminsubs and create each one
    pContext->subType = ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB;
    pContext->namespace = ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_NONDURABLE;
    ism_prop_t *nonpersistentAdminSubNames = ism_config_getObjectInstanceNames(ismEngine_serverGlobal.configCallbackHandle,
                                                                               pContext->subType);

    if (nonpersistentAdminSubNames != NULL)
    {
        const char *thisSubName;

        for (int32_t i = 0; ism_common_getPropertyIndex(nonpersistentAdminSubNames, i, &thisSubName) == 0; i++)
        {
            assert(thisSubName[0] != '/');

            rc = iett_reconcileAdminSharedSub(pThreadData,
                                              (char *)thisSubName,
                                              0,
                                              NULL,
                                              pContext);

            if (rc != OK) break;
        }

        ism_common_freeProperties(nonpersistentAdminSubNames);
    }

    if (rc != OK)
    {
        goto mod_exit;
    }

mod_exit:

    /* Wait for any asynchronous operations performed to complete */
    waitRC = ieut_waitForRemainingActions(pThreadData,
                                          &pContext->remainingActions,
                                          __func__, 1);

    if (rc == OK) rc = waitRC;

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief  Replay an UpdateRetained (iettSLEUpdateRetained_t) soft log entry
///
/// @param[in]     pTran    Transaction being committed/rolledback
/// @param[in]     Phase    Which of the commit/rollback phases this is
/// @param[in]     pThreadData  Engine Thread data
/// @param[in]     entry    Pointer to the SLE
/// @param[in]     pRecord  Transaction State Data
//****************************************************************************
void iett_SLEReplayUpdateRetained(ietrReplayPhase_t        Phase,
                                  ieutThreadData_t        *pThreadData,
                                  ismEngine_Transaction_t *pTran,
                                  void                    *entry,
                                  ietrReplayRecord_t      *pRecord,
                                  ismEngine_DelivererContext_t *delivererContext)
{
    int32_t rc;

    iettSLEUpdateRetained_t *pSLE = (iettSLEUpdateRetained_t *)entry;

    ieutTRACEL(pThreadData, pSLE,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Phase=%d\n", __func__, Phase);

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    iettTopicNode_t *topicNode = pSLE->topicNode;
    iereResourceSetHandle_t resourceSet = topicNode->resourceSet;
    ismEngine_Message_t *inflightMsg = (ismEngine_Message_t *)pSLE->message;

    bool newStoreTran;
    uint32_t preLockStoreOpCount;

    // If this is a savepoint rollback, it will have been called from the publish loop
    // when a non-function scope transaction was using a store transaction for this publish,
    // which is then rolled back... so we want to ignore the refHandle.
    if (Phase == SavepointRollback)
    {
        pSLE->savepointRollback = true;
        newStoreTran = false;
    }
    // In the Commit, Rollback, PostCommit and PostRollback cases, we may be about to
    // start a store transaction. If we do this while holding the topic lock then there
    // could be a deadlock with another thread which is accessing the lock in the middle
    // of a store transaction (because of the store mechanism for dealing with the creation
    // of new store transactions).
    //
    // So - if we are not doing Cleanup, we start a store transaction now to ensure that
    // we won't start one while we're holding an engine lock. If we didn't start the store
    // transaction (i.e. one was already active) we remember that so we can clear up later.
    else if (Phase == Cleanup)
    {
        newStoreTran = false;
    }
    else
    {
        assert(Phase == Commit ||
               Phase == Rollback ||
               Phase == PostCommit ||
               Phase == PostRollback);

        newStoreTran = iest_store_startTransaction(pThreadData);
        preLockStoreOpCount = pRecord->StoreOpCount;
    }

    if (Phase == Commit || Phase == Rollback)
    {
        bool deleteTranRef = false;

        // Initialize the fields used during the PostCommit / PostRollback phase
        pSLE->releaseMsgHdl = ismENGINE_NULL_MESSAGE_HANDLE;
        pSLE->needUnstore = false;
        pSLE->replacedOriginServer = NULL;
        pSLE->supersededAtCommit = false;

        ismEngine_getRWLockForRead(&tree->topicsLock);

        // If it looks like we will commit this one, we have some additional work to do if we
        // haven't already stored the message.
        if (((Phase == Commit) && (pSLE->savepointRollback == false) &&
             ((pSLE->repositioningRetained == true && pSLE->timestamp == topicNode->currRetTimestamp) ||
              (pSLE->repositioningRetained == false && ((pSLE->timestamp > topicNode->currRetTimestamp) ||
                                                        (topicNode->currRetTimestamp == 0))))))
        {
            // Actually store the message and create the reference if not already done
            if (pSLE->refHandle == ismSTORE_NULL_HANDLE &&
                inflightMsg->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
            {
                assert(pTran->fAsStoreTran);

                ismStore_Reference_t msgRef = {0};

                rc = iest_storeMessage(pThreadData,
                                       inflightMsg,
                                       2, // Additional reference for delivery to late subs
                                       iestStoreMessageOptions_EXISTING_TRANSACTION |
                                       iestStoreMessageOptions_ATOMIC_REFCOUNTING,
                                       &msgRef.hRefHandle);
                assert(rc == OK);
                pRecord->StoreOpCount++;

                msgRef.OrderId = pSLE->orderId;
                msgRef.State = pSLE->originServer->localServer ? iettRMR_STATE_ORIGINSERVER_LOCAL :
                                                                 iettRMR_STATE_NONE;

                assert(msgRef.hRefHandle != ismSTORE_NULL_HANDLE);
                assert(inflightMsg->StoreMsg.parts.hStoreMsg != ismSTORE_NULL_HANDLE);
                assert(tree->retRefContext != NULL);

                rc = ism_store_createReference(pThreadData->hStream,
                                               tree->retRefContext,
                                               &msgRef,
                                               tree->retMinActiveOrderId,
                                               &pSLE->refHandle);
                assert(rc == OK);
                assert(pSLE->refHandle != ismSTORE_NULL_HANDLE);
                pRecord->StoreOpCount++;
            }

            // Optimistically assume that this will get propagated as retained.
            inflightMsg->Header.Flags |= ismMESSAGE_FLAGS_PROPAGATE_RETAINED;
        }
        else
        {
            if (pSLE->savepointRollback == false && pSLE->refHandle != ismSTORE_NULL_HANDLE)
            {
                assert(pTran->fAsStoreTran == false);

                if (pTran->fIncremental) deleteTranRef = true;

                assert(tree->retRefContext != NULL);

                rc = ism_store_deleteReference(pThreadData->hStream,
                                               tree->retRefContext,
                                               pSLE->refHandle,
                                               pSLE->orderId,
                                               0); // minimumActiveOrderId unchanged

                if (rc != OK)
                {
                    ieutTRACE_FFDC(ieutPROBE_001, true, "ism_store_deleteReference failed.", rc,
                                   "hStream", pThreadData->hStream, sizeof(pThreadData->hStream),
                                   "topicNode", topicNode, sizeof(iettTopicNode_t),
                                   "pSLE->refHandle", &pSLE->refHandle, sizeof(ismStore_Handle_t),
                                   "pSLE->orderId", &pSLE->orderId, sizeof(uint64_t),
                                   "SLE", pSLE, sizeof(iettSLEUpdateRetained_t),
                                   NULL);
                }

                pRecord->StoreOpCount++;

                // Set up the unstore of the message in Cleanup
                pSLE->needUnstore = true;
            }

            // Indicate that we've already decided not to commit this
            pSLE->supersededAtCommit = true;
        }

        ismEngine_unlockRWLock(&tree->topicsLock);

        // Now that we have released the lock on the topics, remove anything from the store
        // that we no longer need.
        if (deleteTranRef)
        {
            ietr_deleteTranRef( pThreadData
                              , pTran
                              , &pSLE->TranRef );

            pRecord->StoreOpCount++;
        }
    }
    // During the PostCommit / PostRollback finally decide if this is going to be the
    // retained message and set up for that.
    else if (Phase == PostCommit || Phase == PostRollback)
    {
        ismStore_Handle_t deleteRefHandle = ismSTORE_NULL_HANDLE;
        uint64_t deleteRefOrderId = 0;
        bool needExpiry = false;

        ismEngine_getRWLockForWrite(&tree->topicsLock);

        tree->retUpdates++;

        // Decide whether we should definitely make this the retained because it's a
        // commit that we have not already decided is superseded, and the SLE still indicates
        // that it supersedes (or replaces) the current retained message by comparing the
        // timestamp in the SLE with the current retained timestamp for this topic.
        if (((Phase == PostCommit) &&
             (pSLE->supersededAtCommit == false) &&
             ((pSLE->repositioningRetained == true && pSLE->timestamp == topicNode->currRetTimestamp) ||
              (pSLE->repositioningRetained == false && ((pSLE->timestamp > topicNode->currRetTimestamp) ||
                                                        (topicNode->currRetTimestamp == 0))))))
        {
            // The message is no longer in-flight, so we need to update some stats.
            // The amount by which we change them will depend on this message and the current
            // retained - we start by considering the new message, and modify the deltas when
            // we get to look at the old message.

            // If this message has expiry we want to increase the expiry stat
            int64_t expiryStatDelta = (inflightMsg->Header.Expiry == 0) ? 0 : 1;
            // If this message is not an MTYPE_NullRetained, we want to increase the externalRetained stats
            int64_t externalStatDelta;
            int64_t externalStatBytesDelta;

            if (inflightMsg->Header.MessageType == MTYPE_NullRetained)
            {
                externalStatDelta = externalStatBytesDelta = 0;
            }
            else
            {
                externalStatDelta = 1;
                externalStatBytesDelta = inflightMsg->fullMemSize;
            }

            // We are going to commit this request - we ensure that late subscribers
            // are given the message if appropriate.
            pSLE->commitRUV = tree->retUpdates;

            // There is an existing retained message, which we need to release.
            if (NULL != topicNode->currRetMessage)
            {
                ismEngine_Message_t *pMessage = topicNode->currRetMessage;

                bool isNullRetained = (pMessage->Header.MessageType == MTYPE_NullRetained);

                // The previous retained message already had expiry - update our delta
                if (pMessage->Header.Expiry != 0) expiryStatDelta -= 1;

                // The previous retained message was not an MTYPE_NullRetained - update our deltas
                if (isNullRetained == false)
                {
                    externalStatDelta -= 1;
                    externalStatBytesDelta -= pMessage->fullMemSize;
                }

                // Stop propagating this as the retained message
                pMessage->Header.Flags &= ~ismMESSAGE_FLAGS_PROPAGATE_RETAINED;

                // We are no longer in-flight, but are taking over this topic
                pThreadData->stats.internalRetainedMsgCount--;

                pSLE->releaseMsgHdl = pMessage;

                // The existing message is persistent, need to remove it from the
                // reference context of the topic node in the store.
                if (topicNode->currRetRefHandle != ismSTORE_NULL_HANDLE)
                {
                    deleteRefHandle = topicNode->currRetRefHandle;
                    deleteRefOrderId = topicNode->currRetOrderId;

                    // Set up the unstore of the message in Cleanup
                    pSLE->needUnstore = true;
                }

                assert(topicNode->currOriginServer != NULL);

                pSLE->replacedOriginServer = topicNode->currOriginServer;

                if (pSLE->repositioningRetained == false)
                {
                    iett_removeTopicNodeFromOriginServer(pThreadData,
                                                         topicNode,
                                                         topicNode->currOriginServer);

                    assert(topicNode->currOriginServer == NULL);
                    assert(topicNode->originNext == NULL);
                    assert(topicNode->originPrev == NULL);
                }
                else
                {
                    assert(pSLE->originServer == topicNode->currOriginServer);
                }
            }

            if (inflightMsg->Header.MessageType == MTYPE_NullRetained)
            {
                topicNode->nodeFlags |= iettNODE_FLAG_NULLRETAINED;
            }
            else
            {
                topicNode->nodeFlags &= ~iettNODE_FLAG_NULLRETAINED;
            }

            topicNode->currRetOrderId = pSLE->orderId;
            topicNode->currRetRefHandle = pSLE->refHandle;
            topicNode->currRetMessage = inflightMsg;
            topicNode->currRetTimestamp = pSLE->timestamp;
            topicNode->expiryTime = inflightMsg->Header.Expiry;

            pThreadData->stats.retainedExpiryMsgCount += expiryStatDelta;
            pThreadData->stats.externalRetainedMsgCount += externalStatDelta;
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSGS, externalStatDelta);
            iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSG_BYTES, externalStatBytesDelta);

            // Consumers should propagate this as a retained message
            inflightMsg->Header.Flags |= ismMESSAGE_FLAGS_PROPAGATE_RETAINED;

            if (pSLE->repositioningRetained == false)
            {
                iett_addTopicNodeToOriginServer(pThreadData,
                                                topicNode,
                                                pSLE->originServer);
            }

            assert(topicNode->currOriginServer == pSLE->originServer);
        }
        else // rollback OR superseded retained message
        {
            // This retained message is no longer in-flight
            pThreadData->stats.internalRetainedMsgCount--;

            // We do not deliver this message to late subscribers.
            pSLE->commitRUV = 0;

            pSLE->releaseMsgHdl = inflightMsg;

            if ((pSLE->supersededAtCommit == false) && (pSLE->refHandle != ismSTORE_NULL_HANDLE))
            {
                assert(pSLE->needUnstore == false);

                deleteRefHandle = pSLE->refHandle;
                deleteRefOrderId = pSLE->orderId;

                // Set up the unstore of the message in Cleanup
                pSLE->needUnstore = true;
            }

            // Make it clear that this is not the retained
            inflightMsg->Header.Flags &= ~ismMESSAGE_FLAGS_PROPAGATE_RETAINED;
        }

        // No longer need this SLE in the inflight set
        iett_removeInflightRetUpdate(pThreadData, topicNode, pSLE);

        // Decide whether we want to put this node on the expiry list before releasing the lock
        if (topicNode->expiryTime != 0) needExpiry = true;

        // Delete a message reference from the topic, this will be either the existing
        // retained message, or the one we are about to publish that has been either rolled
        // back or superseded.
        if (deleteRefHandle != ismSTORE_NULL_HANDLE)
        {
            assert(tree->retRefContext != NULL);

            rc = ism_store_deleteReference(pThreadData->hStream,
                                           tree->retRefContext,
                                           deleteRefHandle,
                                           deleteRefOrderId,
                                           0); // minimumActiveOrderId unchanged

            if (rc != OK)
            {
                ieutTRACE_FFDC(ieutPROBE_002, true, "ism_store_deleteReference failed.", rc,
                               "hStream", pThreadData->hStream, sizeof(pThreadData->hStream),
                               "topicNode", topicNode, sizeof(iettTopicNode_t),
                               "deleteRefHandle", &deleteRefHandle, sizeof(ismStore_Handle_t),
                               "deleteRefOrderid", &deleteRefOrderId, sizeof(uint64_t),
                               "SLE", pSLE, sizeof(iettSLEUpdateRetained_t),
                               NULL);
            }

            pRecord->StoreOpCount++;
        }

        // This is the last pending update on this node, before we release the lock, work out
        // what our vote is for minimum active orderId.
        if (topicNode->pendingUpdates == 1)
        {
            if (topicNode->currRetRefHandle != ismSTORE_NULL_HANDLE)
            {
                topicNode->activeOrderIdVote = topicNode->currRetOrderId;
            }
            else
            {
                topicNode->activeOrderIdVote = 0;
            }
        }

        ismEngine_unlockRWLock(&tree->topicsLock);

        // Add this node to the expiry list (it may already be on it).
        if (needExpiry)
        {
            ieme_addTopicToExpiryReaperList(pThreadData, topicNode);
        }
    }
    // During Cleanup, we need to deliver to new subscribers who are owed this message.
    else if (Phase == Cleanup)
    {
        if (pSLE->commitRUV != 0)
        {
            if (pSLE->publishSUV != 0)
            {
                concat_alloc_t  props;
                ism_field_t field = {0};

                assert(pSLE->repositioningRetained == false);

                // Extract the full topic string from the message (the node only has a substring)
                iem_locateMessageProperties((ismEngine_Message_t *)inflightMsg, &props);

                // The properties contain a topic string and the node's substring matches the end of it
                ism_common_findPropertyID(&props, ID_Topic, &field);

                assert(field.val.s != NULL);

                (void)iett_putRetainedMessageToNewSubs(pThreadData,
                                                       field.val.s,
                                                       inflightMsg,
                                                       pSLE->publishSUV,
                                                       pSLE->commitRUV);
            }

            // Tell the cluster component about the stats for the origin servers
            iettOriginServerCbContext_t originServerCbContext = { OK };

            // Report on the origin server being replaced (if it's different to this one)
            if ((pSLE->replacedOriginServer != NULL) &&
                (pSLE->replacedOriginServer != pSLE->originServer))
            {
                iett_clusterReportOriginServer(pThreadData,
                                               NULL,
                                               0,
                                               pSLE->replacedOriginServer,
                                               &originServerCbContext);
            }

            // Report on the origin server of the new message
            iett_clusterReportOriginServer(pThreadData,
                                           NULL,
                                           0,
                                           pSLE->originServer,
                                           &originServerCbContext);
        }

        // If during Commit/PostCommit/Rollback/PostRollback we deleted a message
        // reference, now we need to unstore & release the message
        if (pSLE->releaseMsgHdl != ismENGINE_NULL_MESSAGE_HANDLE)
        {
            ismEngine_Message_t *message = pSLE->releaseMsgHdl;

            if (pSLE->needUnstore)
            {
                rc = iest_unstoreMessage(pThreadData,
                                         message,
                                         false,
                                         false,
                                         NULL,
                                         &pRecord->StoreOpCount);
                assert(rc == OK);
            }

            iem_releaseMessage(pThreadData, message);
        }

        // For a persistent message that was committed, we need to release the store
        // reference count we were keeping until we published to new subscribers.
        if (pSLE->savepointRollback == false && pSLE->refHandle != ismSTORE_NULL_HANDLE)
        {
            rc = iest_unstoreMessage(pThreadData,
                                     inflightMsg,
                                     false,
                                     false,
                                     NULL,
                                     &pRecord->StoreOpCount);
            assert(rc == OK);
        }

        // We can now release the message being retained
        iem_releaseMessage(pThreadData, inflightMsg);

        // Take the lock while we decrement the pendingUpdates count now that
        // we have finished updating
        ismEngine_getRWLockForWrite(&tree->topicsLock);

        topicNode->pendingUpdates--;

        iettTopicNode_t *removedTree = iett_removeUnusedTree(pThreadData, tree, topicNode);

        assert(removedTree != ismEngine_serverGlobal.maintree->topics);

        ismEngine_unlockRWLock(&tree->topicsLock);

        // We have a tree that got removed.
        if (removedTree != NULL)
        {
            iettDestroyTopicsTreeCbContext_t destroyCbContext;

            destroyCbContext.freeingEngineTree = false;

            iett_destroyTopicsTreeCallback(pThreadData, NULL, 0, removedTree, &destroyCbContext);
        }
    }

    // We created a new store transaction (to avoid locking problems) but didn't actually
    // add any store operations - we want to cancel the transaction in this case.
    if (newStoreTran == true && pRecord->StoreOpCount == preLockStoreOpCount)
    {
        iest_store_cancelTransaction(pThreadData);
    }

    ieutTRACEL(pThreadData, Phase, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief  Replay an AddSubscription (iettSLEAddSubscription_t) soft log entry
///
/// @param[in]     Phase    Which of the commit/rollback phases this is
/// @param[in]     pThreadData  Engine Thread data
/// @param[in]     entry    Pointer to the SLE
/// @param[in]     pRecord  Transaction State Data
//****************************************************************************
void iett_SLEReplayAddSubscription(ietrReplayPhase_t        Phase,
                                   ieutThreadData_t        *pThreadData,
                                   ismEngine_Transaction_t *pTran,
                                   void                    *entry,
                                   ietrReplayRecord_t      *pRecord,
                                   ismEngine_DelivererContext_t *delivererContext)
{
    iettSLEAddSubscription_t *pSLE = entry;

    ieutTRACEL(pThreadData, pSLE,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Phase=%d\n", __func__, Phase);

    assert ((Phase == Commit) || ( Phase == PostRollback));

    if (Phase == PostRollback)
    {
        assert(pSLE->lock != NULL);

        DEBUG_ONLY int32_t rc;
        rc = iett_removeSubFromEngineTopic(pThreadData,
                                           pSLE->subscription,
                                           iettFLAG_REMOVE_SUB_ROLLBACK_ADD | iettFLAG_REMOVE_SUB_ALREADY_LOCKED);

        // removal from the tree should not go asynchronous when we are either
        // in recovery, or rolling back a new addition.
        assert(rc == OK);
    }

    // If we have been holding onto the subs lock then release it now.
    // We do this to ensure that no messages are delivered to this subscription until
    // it's addition has been completed.
    if (pSLE->lock != NULL)
    {
        ismEngine_unlockRWLock(pSLE->lock);
    }

    ieutTRACEL(pThreadData, Phase, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief  Replay an old Store Subscription Definition (iettSLEStoreSubscDefn_t)
///         soft log entry
///
/// @param[in]     Phase    Which of the commit/rollback phases this is
/// @param[in]     pThreadData  Engine Thread data
/// @param[in]     entry    Pointer to the SLE
/// @param[in]     pRecord  Transaction State Data
///
/// @remark These SLEs have been deprecated, we only expect to deal with them
///         for rehydrated transactions.
//****************************************************************************
void iett_SLEReplayOldStoreSubscDefn(ietrReplayPhase_t        Phase,
                                     ieutThreadData_t        *pThreadData,
                                     ismEngine_Transaction_t *pTran,
                                     void                    *entry,
                                     ietrReplayRecord_t      *pRecord,
                                     ismEngine_DelivererContext_t * delivererContext)
{
    iettSLEOldStoreSubscDefn_t *pSLE = entry;

    ieutTRACEL(pThreadData, pSLE,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Phase=%d, pSLE=%p\n", __func__, Phase, pSLE);

    assert(Phase == Rollback);
    assert(pTran->fAsStoreTran == false);
    assert((pTran->TranFlags & ietrTRAN_FLAG_REHYDRATED) == ietrTRAN_FLAG_REHYDRATED);
    assert(pSLE->queueHandle != NULL);

    ieutTRACEL(pThreadData, Phase, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief  Replay a Store Subscription Properties (iettSLEStoreSubscProps_t)
///         soft log entry
///
/// @param[in]     Phase    Which of the commit/rollback phases this is
/// @param[in]     pThreadData  Engine Thread data
/// @param[in]     entry    Pointer to the SLE
/// @param[in]     pRecord  Transaction State Data
///
/// @remark These SLEs have been deprecated, we only expect to deal with them
///         for rehydrated transactions.
//****************************************************************************
void iett_SLEReplayOldStoreSubscProps(ietrReplayPhase_t        Phase,
                                      ieutThreadData_t        *pThreadData,
                                      ismEngine_Transaction_t *pTran,
                                      void                    *entry,
                                      ietrReplayRecord_t      *pRecord,
                                      ismEngine_DelivererContext_t * delivererContext)
{
    iettSLEOldStoreSubscProps_t *pSLE = entry;

    ieutTRACEL(pThreadData, pSLE,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Phase=%d, pSLE=%p\n", __func__, Phase, pSLE);

    assert(Phase == Rollback);
    assert(pTran->fAsStoreTran == false);
    assert((pTran->TranFlags & ietrTRAN_FLAG_REHYDRATED) == ietrTRAN_FLAG_REHYDRATED);
    assert(pSLE->queueHandle != NULL);

    ieutTRACEL(pThreadData, Phase, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief  Replay a Free Subscriber list (iettSLEFreeSubList_t) soft log entry
///
/// @param[in]     Phase    Which of the commit/rollback phases this is
/// @param[in]     pThreadData  Engine Thread data
/// @param[in]     entry    Pointer to the SLE
/// @param[in]     pRecord  Transaction State Data
//****************************************************************************
void iett_SLEReplayReleaseNodes(ietrReplayPhase_t        Phase,
                                ieutThreadData_t        *pThreadData,
                                ismEngine_Transaction_t *pTran,
                                void                    *entry,
                                ietrReplayRecord_t      *pRecord,
                                ismEngine_DelivererContext_t *delivererContext)
{
    iettSLEReleaseNodes_t *pSLE = entry;

    ieutTRACEL(pThreadData, pSLE,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Phase=%d\n", __func__, Phase);

    if (Phase == Rollback)
    {
        // If we are rolling back a successful publish it means tha application
        // explicitly rolled back the publish, so we don't want to update any
        // stats.
        if (pSLE->publishOK)
        {
            pSLE->updateStats = false;
        }
    }
    else
    {
        assert(Phase == Cleanup);

        // Somewhere during the commit of this transaction, a simple queue has
        // rejected this message - we need to make sure that any stats are
        // updated correctly.
        if (pRecord != NULL && pRecord->SkippedPutCount > 0)
        {
            pSLE->publishRejected = true;
        }

        // Update global statistics
        if (pSLE->updateStats && pSLE->publishRejected)
        {
            pThreadData->stats.droppedMsgCount++;
        }

        // If there are any susbcriber nodes, we need to update stats on them and
        // potentially remove any subscriptions that are waiting to be deleted.
        if (pSLE->subscriberNodes != NULL)
        {
            iettSubsNode_t **subsNodePos = pSLE->subscriberNodes;

            iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

            do
            {
                char *pendingDeletionTopic;

                iettSubsNode_t *subsNode = *subsNodePos;

                // Update statistics if required for this node
                if (pSLE->updateStats &&
                    (NULL != subsNode->stats && subsNode->stats->topicStats.ResetTime != 0))
                {
                    // If the publish was OK, we are updating successful stats
                    if (pSLE->publishOK)
                    {
                        (void)__sync_fetch_and_add(&subsNode->stats->topicStats.PublishedMsgs,1);

                        if (pSLE->publishRejected)
                        {
                            (void)__sync_fetch_and_add(&subsNode->stats->topicStats.RejectedMsgs, 1);
                        }
                    }
                    else
                    {
                        (void)__sync_fetch_and_add(&subsNode->stats->topicStats.FailedPublishes, 1);
                    }
                }

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
                if (oldCount == 1 && pendingDeletionTopic != NULL)
                {
                    iett_performPendingSubscriptionDeletions(pThreadData, tree, pendingDeletionTopic);
                }

                if (NULL != pendingDeletionTopic) ism_common_free(ism_memory_engine_misc,pendingDeletionTopic);
            }
            while(NULL != *(++subsNodePos));
        }

        // Release any remote servers
        if (pSLE->remoteServers != NULL)
        {
            ismEngine_RemoteServer_t **remoteServerPos = pSLE->remoteServers;

            do
            {
                ismEngine_RemoteServer_t *remoteServer = *remoteServerPos;
                (void)iers_releaseServer(pThreadData, remoteServer);
            }
            while(NULL != *(++remoteServerPos));
        }
    }

    ieutTRACEL(pThreadData, Phase, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief  Add an iettSLEOldStoreSubscDefn_t SLE to the specified transaction
///         for the SDR whose storeHandle is specified.
///
/// @param[in]     transData           The restart transaction data
/// @param[in]     hQueue              Handle of the queue
/// @param[in]     storeHandle         Store handle of the SDR
/// @param[in]     operationRefHandle  Handle of the transaction operation reference
/// @param[out]    operationRefOrderId OrderId of the transaction operation reference
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_rehydrateOldSubscriptionDefnSLE(ieutThreadData_t *pThreadData,
                                             ismEngine_RestartTransactionData_t *transData,
                                             ismQHandle_t hQueue,
                                             ismStore_Handle_t storeHandle,
                                             ismStore_Handle_t operationRefHandle,
                                             uint64_t operationRefOrderId)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, storeHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    iettSLEOldStoreSubscDefn_t SLED;

    SLED.storeHandle = storeHandle;
    SLED.queueHandle = hQueue;

    // Ensure we have details of the link to the transaction
    SLED.TranRef.hTranRef = operationRefHandle;
    SLED.TranRef.orderId = operationRefOrderId;

    rc = ietr_softLogRehydrate( pThreadData
                              , transData
                              , ietrSLE_TT_OLD_STORE_SUBSC_DEFN
                              , iett_SLEReplayOldStoreSubscDefn
                              , NULL
                              , Rollback
                              , &SLED
                              , sizeof(SLED)
                              , 0, 1
                              , NULL );

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief  Rehydrate a subscription definition which will result in the
///         creation of an in-memory queue for use with a subscription
///
/// Recreates a queue based on the information read from an SDR in the store.
///
/// @param[in]     recHandle         Store handle of the SDR
/// @param[in]     record            Pointer to the SDR
/// @param[in]     transData         Info about transaction (or NULL)
/// @param[out]    rehydratedRecord  The queue that is created for this definition
/// @param[in]     pContext          Context (unused)
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The record must be consistent upon return.
//****************************************************************************
int32_t iett_rehydrateSubscriptionDefn(ieutThreadData_t *pThreadData,
                                       ismStore_Handle_t recHandle,
                                       ismStore_Record_t *record,
                                       ismEngine_RestartTransactionData_t *transData,
                                       void **rehydratedRecord,
                                       void *pContext)
{
    int32_t rc = OK;
    ismQHandle_t hQueue;

    ieutTRACEL(pThreadData, recHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Verify this is a subscription definition record
    assert(record->Type == ISM_STORE_RECTYPE_SUBSC);
    assert(record->FragsCount == 1);

    iestSubscriptionDefinitionRecord_t *pSDR = (iestSubscriptionDefinitionRecord_t *)(record->pFrags[0]);
    ismEngine_CheckStructId(pSDR->StrucId, iestSUBSC_DEFN_RECORD_STRUCID, ieutPROBE_005);

    ismQueueType_t queueType;
    ieqOptions_t queueOptions;

    if (LIKELY(pSDR->Version == iestSDR_CURRENT_VERSION))
    {
        queueType = pSDR->Type;
        // Assume the subscription is not shared, if the SPR subsequently shows that it is,
        // we will change the options on the queue which we can do during recovery
        queueOptions = ieqOptions_SUBSCRIPTION_QUEUE  |
                       ieqOptions_SINGLE_CONSUMER_ONLY;
    }
    else
    {
        rc = ISMRC_InvalidValue;
        ism_common_setErrorData(rc, "%u", pSDR->Version);
        goto mod_exit;
    }

    ieutTRACEL(pThreadData, queueType, ENGINE_HIFREQ_FNC_TRACE, "Found SDR for queueType %d.\n", queueType);

    // Create the queue
    rc = ieq_createQ(pThreadData,
                     queueType,
                     NULL,
                     queueOptions | ieqOptions_IN_RECOVERY,
                     iepi_getDefaultPolicyInfo(true),   // PolicyInfo - will be updated when SPR rehydrated
                     recHandle,
                     ismSTORE_NULL_HANDLE, // Props handle - will be update when SPR rehydrated
                     iereNO_RESOURCE_SET,  // Resource set - will be updated when SPR rehydrated
                     &hQueue);

    if (rc != OK) goto mod_exit;

    // If we have a transaction, add an SLE for the subscription definition and mark
    // the queue as deleted
    if (transData != NULL)
    {
        assert((transData->pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) != ietrTRAN_FLAG_GLOBAL);

        rc = iett_rehydrateOldSubscriptionDefnSLE(pThreadData,
                                               transData,
                                               hQueue,
                                               recHandle,
                                               transData->operationRefHandle,
                                               transData->operationReference.OrderId);

        // Mark the queue as deleted unless we are going to commit this transaction
        if (rc == OK && transData->pTran->TranState != ismTRANSACTION_STATE_COMMIT_ONLY)
        {
            rc = ieq_markQDeleted(pThreadData, hQueue, false);
        }

        if (rc != OK) goto mod_exit;
    }
    // If this SDR is for a subscription queue that is deleted or hasn't finished creation,
    // mark it as deleted so that it can be deleted during cleanup / when outstanding transactions
    // complete.
    else if (record->State & (iestSDR_STATE_DELETED | iestSDR_STATE_CREATING))
    {
        ieutTRACEL(pThreadData, hQueue, ENGINE_FNC_TRACE, "Deleted / Creating SDR [state:0x%016lx] found for queue %p\n",
                   record->State, hQueue);

        rc = ieq_markQDeleted(pThreadData, hQueue, false);

        if (rc != OK) goto mod_exit;
    }

    // Pass the queue handle back to the caller. This will be passed back later
    // as the requestingRecord for the corresponding SPR.
    *rehydratedRecord = hQueue;

mod_exit:
    if (rc != OK)
    {
        ierr_recordBadStoreRecord( pThreadData
                                 , record->Type
                                 , recHandle
                                 , NULL
                                 , rc);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief  Callback used if the commit of some updated subscriptions goes
///         async
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
void iett_asyncUpdateMigratedSubscription(int rc, void *context)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    pThreadData->threadType = AsyncCallbackThreadType;
    uint32_t newValue = __sync_sub_and_fetch((uint32_t *)context, 1);
    ieutTRACEL(pThreadData, newValue, ENGINE_HIGH_TRACE, FUNCTION_IDENT "newValue=%u\n", __func__, newValue);
    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @brief  Update any subscriptions that have just been migrated
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_updateMigratedSubscriptions(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;
    bool updatesToDo = (subscriptionsNeedUpdating > 0);
    uint32_t pendingCommits = 0;

    ieutTRACEL(pThreadData, subscriptionsNeedUpdating, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (updatesToDo)
    {
        assert(ismEngine_serverGlobal.runPhase > EnginePhaseRecovery);

        iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

        // Lock the topic tree for write access
        ismEngine_getRWLockForWrite(&tree->subsLock);

        // Start with the first subscription
        ismEngine_Subscription_t *subscription = tree->subscriptionHead;

        // Reserve store stream resources
        uint32_t initialRecordReservation = iettUPDATE_MIGRATION_RESERVATION_RECORDS;
        uint64_t initialDataReservation = (uint64_t)initialRecordReservation * iettUPDATE_MIGRATION_RESERVATION_RECORD_SIZE;
        uint32_t remainingRecordReservation;
        uint64_t remainingDataReservation;

        iest_store_reserve(pThreadData,
                           initialDataReservation,
                           initialRecordReservation,
                           0);
        remainingDataReservation = initialDataReservation;
        remainingRecordReservation = initialRecordReservation;

        while(subscription != NULL)
        {
            if ((subscription->internalAttrs & iettSUBATTR_MAPPED_POLICY_NAME) != 0)
            {
                assert(subscription->node != NULL);
                assert(subscription->node->topicString != NULL);
                assert((subscription->internalAttrs & iettSUBATTR_PERSISTENT) != 0);

                ieutTRACEL(pThreadData, subscription, ENGINE_HIGH_TRACE, "Updating migrated subscription %p (internalAttrs:0x%08x NewPolicy:%s)\n",
                           subscription, subscription->internalAttrs,
                           subscription->queueHandle->PolicyInfo->name);

                ismStore_Handle_t hNewSubscriptionProps = ismSTORE_NULL_HANDLE;

                uint64_t thisUpdateDataSize = iest_getSPRSize(pThreadData,
                                                              ieq_getPolicyInfo(subscription->queueHandle),
                                                              subscription->node->topicString,
                                                              subscription);

                if ((remainingRecordReservation == 0) || (remainingDataReservation < thisUpdateDataSize))
                {
                    __sync_add_and_fetch(&pendingCommits, 1);
                    if (iest_store_asyncCommit(pThreadData, true, iett_asyncUpdateMigratedSubscription, &pendingCommits) == OK)
                    {
                        __sync_sub_and_fetch(&pendingCommits, 1);
                    }

                    // Reserve more store stream resources for the next set
                    iest_store_reserve(pThreadData,
                                       initialDataReservation,
                                       initialRecordReservation,
                                       0);
                    remainingDataReservation = initialDataReservation;
                    remainingRecordReservation = initialRecordReservation;
                }

                rc = iest_updateSubscription(pThreadData,
                                             ieq_getPolicyInfo(subscription->queueHandle),
                                             subscription,
                                             ieq_getDefnHdl(subscription->queueHandle),
                                             ieq_getPropsHdl(subscription->queueHandle),
                                             &hNewSubscriptionProps,
                                             false);

                // We want to stop - but ought to commit what we have outstanding
                if (rc != OK) break;

                remainingDataReservation -= thisUpdateDataSize;
                remainingRecordReservation -= 1;

                assert(hNewSubscriptionProps != ismSTORE_NULL_HANDLE);

                ieq_setPropsHdl(subscription->queueHandle, hNewSubscriptionProps);

                subscription->internalAttrs &= ~iettSUBATTR_MAPPED_POLICY_NAME;
            }

            subscription = subscription->next;
        }

        // If we have updates to commit, do so now, otherwise cancel the reservation
        if (remainingRecordReservation < initialRecordReservation)
        {
            __sync_add_and_fetch(&pendingCommits, 1);
            if (iest_store_asyncCommit(pThreadData, true, iett_asyncUpdateMigratedSubscription, &pendingCommits) == OK)
            {
                __sync_sub_and_fetch(&pendingCommits, 1);
            }
        }
        else
        {
            iest_store_cancelReservation(pThreadData);
        }

        ismEngine_unlockRWLock(&tree->subsLock);

        assert(pThreadData->ReservationState == Inactive);
    }

    // We can now destroy the UUID to Name mapping structures.
    // We choose to keep the file if there were any updates from it, so the first time
    // all UUIDs have been mapped to names we will delete the file.
    iepi_destroyPolicyNameMappings(pThreadData, updatesToDo);

    // No longer have any subscriptions in need of update
    subscriptionsNeedUpdating = 0;

    // Wait for any pending commits to complete
    while((volatile uint32_t)pendingCommits != 0)
    {
        sched_yield();
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief  Rehydrate a durable subscription read from the store
///
/// Recreates a durable subscription from the store.
///
/// @param[in]     recHandle         Store handle of the SPR
/// @param[in]     record            Pointer to the SPR
/// @param[in]     transData         info about transaction (or NULL)
/// @param[in]     requestingRecord  The SDR (queue) that requested this SPR
/// @param[out]    rehydratedRecord  Pointer to data created from this record if
///                                  the recovery code needs to track it
///                                  (unused for Subscriptions)
/// @param[in]     pContext          Context (unused)
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark if called with NULL requestingRecord, assumes that this subscription
///         is using a simple queue, and creates one for it - otherwise uses the
///         queue specified.
///
/// @remark The record must be consistent upon return.
//****************************************************************************
int32_t iett_rehydrateSubscriptionProps(ieutThreadData_t *pThreadData,
                                        ismStore_Handle_t recHandle,
                                        ismStore_Record_t *record,
                                        ismEngine_RestartTransactionData_t *transData,
                                        void *requestingRecord,
                                        void **rehydratedRecord,
                                        void *pContext)
{
    int32_t rc = OK;
    iestSubscriptionPropertiesRecord_t *pSPR;
    ismEngine_Subscription_t *subscription = NULL;
    iepiPolicyInfo_t *pPolicyInfo = NULL;
    char *queueName = NULL;
    char *subName = NULL;
    iereResourceSetHandle_t resourceSet;

    ieutTRACEL(pThreadData, recHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Verify this is a subscription properties record
    assert(record->Type == ISM_STORE_RECTYPE_SPROP);
    assert(record->FragsCount == 1);

    ismQHandle_t hQueue = (ismQHandle_t)requestingRecord;

    assert(hQueue != NULL);

    uint32_t subOptions, internalAttrs, sharingClientCount, sharingClientSubIdCount;
    size_t  clientIdLength, subNameLength, topicStringLength, subPropertiesLength;
    size_t  policyUUIDLength, policyNameLength;
    uint64_t sharingClientIdsLength;
    uint8_t anonymousSharers;
    ismEngine_SubId_t subId;
    char *tmpPtr;

    pSPR = (iestSubscriptionPropertiesRecord_t *)(record->pFrags[0]);
    ismEngine_CheckStructId(pSPR->StrucId, iestSUBSC_PROPS_RECORD_STRUCID, ieutPROBE_006);

    ieutTRACEL(pThreadData, pSPR->Version, ENGINE_HIFREQ_FNC_TRACE, "Found Version %u SPR.\n", pSPR->Version);

    // Create a template policy in case we need to create a policy for this subscription
    iepiPolicyInfo_t template;

    // Cope with different subscription properties versions
    if (LIKELY(pSPR->Version == iestSPR_CURRENT_VERSION))
    {
        subOptions = pSPR->SubOptions;
        internalAttrs = pSPR->InternalAttrs;
        clientIdLength = pSPR->ClientIdLength;
        subNameLength = pSPR->SubNameLength;
        topicStringLength = pSPR->TopicStringLength;
        subPropertiesLength = pSPR->SubPropertiesLength;
        template.maxMessageCount = pSPR->MaxMessageCount;
        template.DCNEnabled = pSPR->DCNEnabled;
        template.maxMsgBehavior = (iepiMaxMsgBehavior_t)pSPR->MaxMsgBehavior;
        anonymousSharers = pSPR->AnonymousSharers;
        sharingClientCount = pSPR->SharingClientCount;
        sharingClientSubIdCount = sharingClientCount;
        sharingClientIdsLength = pSPR->SharingClientIdsLength;
        policyNameLength = pSPR->PolicyNameLength;
        subId = pSPR->SubId;

        // deprecated fields default
        policyUUIDLength = 0;

        tmpPtr = (char*)(pSPR+1);
    }
    else
    {
        // Get the default policy info in order to default any new values
        iepiPolicyInfo_t *defaultPolicyInfo = iepi_getDefaultPolicyInfo(false);

        // Previous versions we understand...
        if (pSPR->Version == iestSPR_VERSION_6)
        {
            iestSubscriptionPropertiesRecord_V6_t *pSPR_V6 = (iestSubscriptionPropertiesRecord_V6_t *)pSPR;

            subOptions = pSPR_V6->SubOptions;
            internalAttrs = pSPR_V6->InternalAttrs;
            clientIdLength = pSPR_V6->ClientIdLength;
            subNameLength = pSPR_V6->SubNameLength;
            topicStringLength = pSPR_V6->TopicStringLength;
            subPropertiesLength = pSPR_V6->SubPropertiesLength;
            template.maxMessageCount = pSPR_V6->MaxMessageCount;
            template.DCNEnabled = pSPR_V6->DCNEnabled;
            template.maxMsgBehavior = (iepiMaxMsgBehavior_t)pSPR_V6->MaxMsgBehavior;
            anonymousSharers = pSPR_V6->AnonymousSharers;
            sharingClientCount = pSPR_V6->SharingClientCount;
            sharingClientIdsLength = pSPR_V6->SharingClientIdsLength;
            policyNameLength = pSPR_V6->PolicyNameLength;

            // newer fields default
            sharingClientSubIdCount = 0;
            subId = ismENGINE_NO_SUBID;

            // deprecated fields default
            policyUUIDLength = 0;

            tmpPtr = (char*)(pSPR_V6+1);
        }
        else if (pSPR->Version == iestSPR_VERSION_5)
        {
            iestSubscriptionPropertiesRecord_V5_t *pSPR_V5 = (iestSubscriptionPropertiesRecord_V5_t *)pSPR;

            subOptions = pSPR_V5->SubOptions;
            internalAttrs = pSPR_V5->InternalAttrs;
            clientIdLength = pSPR_V5->ClientIdLength;
            subNameLength = pSPR_V5->SubNameLength;
            topicStringLength = pSPR_V5->TopicStringLength;
            subPropertiesLength = pSPR_V5->SubPropertiesLength;
            template.maxMessageCount = pSPR_V5->MaxMessages;
            template.DCNEnabled = pSPR_V5->DCNEnabled;
            template.maxMsgBehavior = (iepiMaxMsgBehavior_t)pSPR_V5->MaxMsgsBehavior;
            anonymousSharers = pSPR_V5->GenericallyShared ?
                               iettANONYMOUS_SHARER_JMS_APPLICATION : iettNO_ANONYMOUS_SHARER;
            sharingClientCount = pSPR_V5->SharingClientCount;
            sharingClientIdsLength = pSPR_V5->SharingClientIdsLength;
            policyUUIDLength = pSPR_V5->PolicyUUIDLength;

            // Newer fields default
            sharingClientSubIdCount = 0;
            policyNameLength = 0;
            subId = ismENGINE_NO_SUBID;

            tmpPtr = (char*)(pSPR_V5+1);
        }
        else if (pSPR->Version == iestSPR_VERSION_4)
        {
            iestSubscriptionPropertiesRecord_V4_t *pSPR_V4 = (iestSubscriptionPropertiesRecord_V4_t *)pSPR;

            subOptions = pSPR_V4->SubOptions;
            internalAttrs = pSPR_V4->InternalAttrs;
            clientIdLength = pSPR_V4->ClientIdLength;
            subNameLength = pSPR_V4->SubNameLength;
            topicStringLength = pSPR_V4->TopicStringLength;
            subPropertiesLength = pSPR_V4->SubPropertiesLength;
            template.maxMessageCount = pSPR_V4->MaxMessages;
            template.DCNEnabled = pSPR_V4->DCNEnabled;
            template.maxMsgBehavior = (iepiMaxMsgBehavior_t)pSPR_V4->MaxMsgsBehavior;
            anonymousSharers = pSPR_V4->GenericallyShared ?
                               iettANONYMOUS_SHARER_JMS_APPLICATION : iettNO_ANONYMOUS_SHARER;
            sharingClientCount = pSPR_V4->SharingClientCount;
            sharingClientIdsLength = pSPR_V4->SharingClientIdsLength;

            // Newer fields default
            sharingClientSubIdCount = 0;
            policyUUIDLength = 0;
            policyNameLength = 0;
            subId = ismENGINE_NO_SUBID;

            tmpPtr = (char*)(pSPR_V4+1);
        }
        else if (pSPR->Version == iestSPR_VERSION_3)
        {
            iestSubscriptionPropertiesRecord_V3_t *pSPR_V3 = (iestSubscriptionPropertiesRecord_V3_t *)pSPR;

            subOptions = pSPR_V3->SubOptions;
            internalAttrs = pSPR_V3->InternalAttrs;
            clientIdLength = pSPR_V3->ClientIdLength;
            subNameLength = pSPR_V3->SubNameLength;
            topicStringLength = pSPR_V3->TopicStringLength;
            subPropertiesLength = pSPR_V3->SubPropertiesLength;
            template.maxMessageCount = pSPR_V3->MaxMessages;
            template.DCNEnabled = pSPR_V3->DCNEnabled;
            template.maxMsgBehavior = (iepiMaxMsgBehavior_t)pSPR_V3->MaxMsgsBehavior;

            // Newer fields default
            anonymousSharers = ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) != 0) ?
                                iettANONYMOUS_SHARER_JMS_APPLICATION : iettNO_ANONYMOUS_SHARER;
            sharingClientCount = 0;
            sharingClientSubIdCount = 0;
            sharingClientIdsLength = 0;
            policyUUIDLength = 0;
            policyNameLength = 0;
            subId = ismENGINE_NO_SUBID;

            tmpPtr = (char*)(pSPR_V3+1);
        }
        else if (pSPR->Version == iestSPR_VERSION_2)
        {
            iestSubscriptionPropertiesRecord_V2_t *pSPR_V2 = (iestSubscriptionPropertiesRecord_V2_t *)pSPR;

            subOptions = pSPR_V2->SubOptions;
            internalAttrs = pSPR_V2->InternalAttrs;
            clientIdLength = pSPR_V2->ClientIdLength;
            subNameLength = pSPR_V2->SubNameLength;
            topicStringLength = pSPR_V2->TopicStringLength;
            subPropertiesLength = pSPR_V2->SubPropertiesLength;
            template.maxMessageCount = pSPR_V2->MaxMessages;
            template.DCNEnabled = pSPR_V2->DCNEnabled;

            // Newer fields default
            template.maxMsgBehavior = defaultPolicyInfo->maxMsgBehavior;
            anonymousSharers = ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) != 0) ?
                                iettANONYMOUS_SHARER_JMS_APPLICATION : iettNO_ANONYMOUS_SHARER;
            sharingClientCount = 0;
            sharingClientSubIdCount = 0;
            sharingClientIdsLength = 0;
            policyUUIDLength = 0;
            policyNameLength = 0;
            subId = ismENGINE_NO_SUBID;

            tmpPtr = (char*)(pSPR_V2+1);
        }
        else if (pSPR->Version == iestSPR_VERSION_1)
        {
            iestSubscriptionPropertiesRecord_V1_t *pSPR_V1 = (iestSubscriptionPropertiesRecord_V1_t *)pSPR;

            // Durable subscriptions written to the store at V1 would only have
            // iettSUBATTR_DURABLE set (now re-purposed as iettSUBATTR_PERSISTENT), so we
            // add on the ismENGINE_SUBSCRIPTION_OPTION_DURABLE they would have had, had
            // it existed then.
            subOptions = pSPR_V1->SubOptions | ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
            internalAttrs = pSPR_V1->InternalAttrs;
            clientIdLength = pSPR_V1->ClientIdLength;
            subNameLength = pSPR_V1->SubNameLength;
            topicStringLength = pSPR_V1->TopicStringLength;
            subPropertiesLength = pSPR_V1->SubPropertiesLength;
            template.maxMessageCount = pSPR_V1->MaxMessages;

            // Newer fields default
            template.DCNEnabled = defaultPolicyInfo->DCNEnabled;
            template.maxMsgBehavior = defaultPolicyInfo->maxMsgBehavior;
            anonymousSharers = ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) != 0) ?
                                iettANONYMOUS_SHARER_JMS_APPLICATION : iettNO_ANONYMOUS_SHARER;
            sharingClientCount = 0;
            sharingClientSubIdCount = 0;
            sharingClientIdsLength = 0;
            policyUUIDLength = 0;
            policyNameLength = 0;
            subId = ismENGINE_NO_SUBID;

            tmpPtr = (char*)(pSPR_V1+1);
        }
        else
        {
            rc = ISMRC_InvalidValue;
            ism_common_setErrorData(rc, "%u", pSPR->Version);
        }
    }

    if (rc == OK)
    {
        assert(subNameLength != 0);
        assert(clientIdLength != 0);
        assert((internalAttrs & iettSUBATTR_PERSISTENT) != 0);
        assert((subOptions & ~ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK) == 0);

        // Pull the constituent parts of the subscription from the end of the SPR header
        char   *clientId = tmpPtr;
        tmpPtr += clientIdLength;

        ieqOptions_t queueOptions;
        ismSecurityPolicyType_t policyType;

        // Now that we know the subOptions we can set queue options (this can only be
        // done during recovery).
        //
        // We can also decide which type of auth object the subscription should be using
        // based on the whether it is shared globally or not using either the internal
        // attribute, or for older subscriptions checking if the clientID is __Shared.
        if (subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED)
        {
            assert(ieq_getQType(hQueue) == multiConsumer);
            queueOptions = ieqOptions_SUBSCRIPTION_QUEUE;

            if ((internalAttrs & iettSUBATTR_GLOBALLY_SHARED) == iettSUBATTR_GLOBALLY_SHARED)
            {
                policyType = ismSEC_POLICY_SUBSCRIPTION;
            }
            // Determine whether this is a globally shared subscription from the owning clientId
            // for older subscriptions
            else if (strncmp(clientId, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE, 8) == 0)
            {
                policyType = ismSEC_POLICY_SUBSCRIPTION;
                internalAttrs |= iettSUBATTR_GLOBALLY_SHARED;
            }
            else
            {
                policyType = ismSEC_POLICY_TOPIC;
            }
        }
        else
        {
            queueOptions = ieqOptions_SUBSCRIPTION_QUEUE  |
                           ieqOptions_SINGLE_CONSUMER_ONLY;
            policyType = ismSEC_POLICY_TOPIC;
        }

        subName = tmpPtr;
        tmpPtr += subNameLength;

        char *topicString;
        if (topicStringLength != 0)
        {
            topicString = tmpPtr;

            // Before version 6 of the SPR, subscriptions were considered system topics if
            // they started with "$SYS", at which point iettSUBATTR_DOLLARSYS_TOPIC would be
            // set in the internalAttrs.
            //
            // Now the definition of a system topic is simply that it starts with '$', so if
            // this is an older SPR we need to set a bit on in internalAttrs. As it happens,
            // iettSUBATTR_SYSTEM_TOPIC is the *same* value that iettSUBATTR_DOLLARSYS_TOPIC
            // was, so those starting "$SYS" will already have the bit on, but others which
            // only start with '$' will not - so we set that value on now.
            if ((pSPR->Version < iestSPR_VERSION_6) && (topicString[0] == ismENGINE_SYSTOPIC_PREFIX[0]))
            {
                internalAttrs |= iettSUBATTR_SYSTEM_TOPIC;
            }
        }
        else
        {
            topicString = "";
        }
        tmpPtr += topicStringLength;

        void *subProperties;
        if (subPropertiesLength != 0)
        {
            subProperties = tmpPtr;
        }
        else
        {
            subProperties = NULL;
        }
        tmpPtr += subPropertiesLength;

        char *policyUUID;
        char *policyName = NULL;
        if (policyUUIDLength != 0)
        {
            assert(policyNameLength == 0);

            // See if there is a mapping from this UUID to a policy name
            if ((policyName = iepi_findPolicyNameMapping(pThreadData, tmpPtr)) != NULL)
            {
                policyNameLength = strlen(policyName)+1;
                tmpPtr += policyUUIDLength;
                policyUUID = NULL;
                policyUUIDLength = 0;
                internalAttrs |= iettSUBATTR_MAPPED_POLICY_NAME;
                subscriptionsNeedUpdating++;
            }
            else
            {
                policyUUID = tmpPtr;
            }
        }
        else
        {
            policyUUID = NULL;
        }
        tmpPtr += policyUUIDLength;

        if (policyNameLength != 0)
        {
            // We may have already picked up a policy name (from a UUID mapping)
            if (policyName != NULL)
            {
                assert(policyUUIDLength == 0);
            }
            else
            {
                #if defined(ALLOW_POLICY_NAME_TO_NAME_MAPPING)
                if ((policyName = iepi_findPolicyNameMapping(pThreadData, tmpPtr)) != NULL)
                {
                    tmpPtr += policyNameLength;
                    policyNameLength = strlen(policyName)+1;
                    internalAttrs |= iettSUBATTR_MAPPED_POLICY_NAME;
                    subscriptionsNeedUpdating++;
                }
                else
                #endif
                {
                    policyName = tmpPtr;
                    tmpPtr += policyNameLength;
                }
            }
        }

        char internalPolicyName[policyNameLength+policyUUIDLength+20];
        if (policyNameLength != 0)
        {
            sprintf(internalPolicyName, iepiINTERNAL_POLICYNAME_FORMAT, policyType, policyName);
        }
        else
        {
            assert(policyName == NULL);

            if (policyUUIDLength != 0)
            {
                sprintf(internalPolicyName, iepiINTERNAL_POLICYNAME_FORMAT, policyType, policyUUID);
            }
        }

        char *sharingSubOptionBuffer;
        if (sharingClientCount != 0)
        {
            sharingSubOptionBuffer = tmpPtr;

            // We expect at client Id for each client
            assert(sharingClientIdsLength >= (uint64_t)sharingClientCount);
        }
        else
        {
            sharingSubOptionBuffer = NULL;
        }
        tmpPtr += (sharingClientCount * sizeof(uint32_t));

        char *sharingClientIdsBuffer;
        if (sharingClientIdsLength != 0)
        {
            sharingClientIdsBuffer = tmpPtr;

            assert(sharingClientCount != 0);
        }
        else
        {
            sharingClientIdsBuffer = NULL;
        }
        tmpPtr += sharingClientIdsLength;

        char *sharingSubIdBuffer;
        if (sharingClientSubIdCount != 0)
        {
            sharingSubIdBuffer = tmpPtr;

            assert(pSPR->Version >= iestSPR_VERSION_7);
            assert(sharingClientSubIdCount == sharingClientCount);
        }
        else
        {
            sharingSubIdBuffer = NULL;
        }
        tmpPtr += (sharingClientSubIdCount * sizeof(ismEngine_SubId_t));

        // We need a name for the queue associated with the subscription so that we can
        // print out something sensible in log messages.
        // Note that clientIdLength and subNameLength already include the null termination
        // character.
        rc = iett_allocateSubQueueName(pThreadData,
                                       clientId,
                                       clientIdLength,
                                       subName,
                                       subNameLength,
                                       topicString,
                                       topicStringLength,
                                       &queueName);

        if (rc != OK) goto mod_exit;

        // The resourceSet to which a subscription belongs is based on topicString for
        // globally shared subscriptions, and on clientId for all others.
        if ((internalAttrs & iettSUBATTR_GLOBALLY_SHARED) == iettSUBATTR_GLOBALLY_SHARED)
        {
            resourceSet = iere_getResourceSet(pThreadData, NULL, topicString, iereOP_ADD);
        }
        else
        {
            assert(strncmp(clientId,
                           ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_PREFIX,
                           strlen(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_PREFIX)) != 0);

            resourceSet = iere_getResourceSet(pThreadData, clientId, NULL, iereOP_ADD);
        }

        // Should not already have any properties handle associated with this queue
        assert(ieq_getPropsHdl(hQueue) == ismSTORE_NULL_HANDLE);

        ieq_updateProperties(pThreadData,
                             hQueue,
                             queueName,
                             queueOptions | ieqOptions_IN_RECOVERY,
                             recHandle,
                             resourceSet);

        // Add an SLE for the subscription properties if there is a transaction - We
        // don't want to add anything to the topic tree if this is going to be rolled back
        if (transData != NULL)
        {
            assert((transData->pTran->TranFlags & ietrTRAN_FLAG_GLOBAL) != ietrTRAN_FLAG_GLOBAL);

            iettSLEOldStoreSubscProps_t SLEP;

            SLEP.storeHandle = recHandle;
            SLEP.queueHandle = hQueue;

            // Ensure we have details of the link to the transaction
            SLEP.TranRef.hTranRef = transData->operationRefHandle;
            SLEP.TranRef.orderId = transData->operationReference.OrderId;

            // Note we include an extra rollback store operation for the
            rc = ietr_softLogRehydrate( pThreadData
                                      , transData
                                      , ietrSLE_TT_OLD_STORE_SUBSC_PROPS
                                      , iett_SLEReplayOldStoreSubscProps
                                      , NULL
                                      , Rollback
                                      , &SLEP
                                      , sizeof(SLEP)
                                      , 0, 2
                                      , NULL );

            // If we are not going to commit, we should not add to the tree
            if (transData->pTran->TranState != ismTRANSACTION_STATE_COMMIT_ONLY)
            {
                assert(ieq_isDeleted(hQueue));
                goto mod_exit;
            }
        }

        // If the queue isn't already marked as deleted, but is a globally shared subscription
        // then if it also has no sharing clients (and no anonymous sharers) we should mark the
        // queue as deleted so that it doesn't get added to the topic tree and is cleared up at
        // the end of recovery.
        if (!ieq_isDeleted(hQueue) &&
            (internalAttrs & iettSUBATTR_GLOBALLY_SHARED) == iettSUBATTR_GLOBALLY_SHARED)
        {
            void *value;

            assert(subName != NULL);

            uint32_t subNameHash = iett_generateSubNameHash(subName);

            // Flag this as an admin sub if its name is in the hash table
            if (ieut_getHashEntry(allPersistentAdminSubNames,
                                  subName,
                                  subNameHash,
                                  &value) == OK)
            {
                ieut_removeHashEntry(pThreadData, allPersistentAdminSubNames, subName, subNameHash);
                anonymousSharers |= iettANONYMOUS_SHARER_ADMIN;
            }
            // Mark this queue as deleted if there are no sharers
            else if (sharingClientCount == 0 && anonymousSharers == iettNO_ANONYMOUS_SHARER)
            {
                ieq_markQDeleted(pThreadData, hQueue, false);
            }
        }

        // See if the queue associated with this properties record is deleted. If it is
        // then it will be deleted at the end of restart or when a transaction referencing
        // it completes, so we do not want to put it in the topic tree.
        if (ieq_isDeleted(hQueue))
        {
            ieutTRACEL(pThreadData, hQueue, ENGINE_FNC_TRACE, "SPR found for %s SDR (queue %p), not adding to tree.\n",
                       (transData ? "transactional (assumed will be rolled back)": "deleted"), hQueue);

            goto mod_exit;
        }

        // Determine which policy we need to look for (could be none)
        const char *policyID;

        if (policyUUID != NULL)
        {
            assert(policyName == NULL);
            policyID = policyName = policyUUID;
        }
        else if (policyName != NULL)
        {
            // Use the created unique identifier
            policyID = internalPolicyName;
        }
        else
        {
            policyID = NULL;
        }

        // If we have an identifier for the policy, look it up from the known policies table or create it.
        if (policyID != NULL)
        {
            iepiPolicyInfoGlobal_t *policyInfoGlobal = ismEngine_serverGlobal.policyInfoGlobal;

            rc = iepi_getKnownPolicyInfo(pThreadData, policyID, policyInfoGlobal, &pPolicyInfo);

            // Didn't find it - assume it must have been deleted and add it to the restart table
            if (rc == ISMRC_NotFound)
            {
                rc = iepi_createPolicyInfo(pThreadData, policyName, policyType, false, &template, &pPolicyInfo);

                if (rc != OK) goto mod_exit;

                assert(strcmp(policyName, pPolicyInfo->name) == 0);
                assert(pPolicyInfo->creationState == CreatedByEngine); // admin unaware

                // Add this new policy info into the restart table
                do
                {
                    rc = iepi_addKnownPolicyInfo(pThreadData, policyID, policyInfoGlobal, pPolicyInfo);

                    // Already there, if we can get the one that's there then use
                    // that instead and free up the one we just created.
                    if (rc == ISMRC_ExistingKey)
                    {
                        iepiPolicyInfo_t *ourPolicyInfo = pPolicyInfo;

                        // This will return the policyInfo pointer in ppPolicyInfo if
                        // the get is successful.
                        rc = iepi_getKnownPolicyInfo(pThreadData, policyID, policyInfoGlobal, &pPolicyInfo);

                        if (rc == OK)
                        {
                            assert(pPolicyInfo != ourPolicyInfo);
                            iepi_freePolicyInfo(pThreadData, ourPolicyInfo, false);
                        }
                    }
                    // If we didn't use an internal policy name, add the same policy using the
                    // internal policy name too.
                    else if (policyID != internalPolicyName)
                    {
                        rc = iepi_addKnownPolicyInfo(pThreadData, internalPolicyName, policyInfoGlobal, pPolicyInfo);
                        assert(rc == OK);
                    }
                }
                while(rc == ISMRC_NotFound);
            }
        }
        // Create a unique policy
        else
        {
            rc = iepi_createPolicyInfo(pThreadData, NULL, ismSEC_POLICY_LAST, false, &template, &pPolicyInfo);

            assert((rc != OK) || ((pPolicyInfo->name == NULL) && (pPolicyInfo->creationState == CreatedByEngine)));
        }

        if (rc != OK) goto mod_exit;

        assert(pPolicyInfo != NULL);

        // Allocate the subscription structure for addition to the tree
        ieutTRACEL(pThreadData, hQueue, ENGINE_HIFREQ_FNC_TRACE, "Rehydrating SubName '%s' for client '%s' (queue=%p).\n",
                   subName, clientId, hQueue);

        ismEngine_SubscriptionAttributes_t subAttributes = {subOptions, subId};

        // Make it clear that this is a rehydrated subscription
        internalAttrs |= iettSUBATTR_REHYDRATED;

        rc = iett_allocateSubscription(pThreadData,
                                       clientId,
                                       &clientIdLength,
                                       subName,
                                       &subNameLength,
                                       subProperties,
                                       &subPropertiesLength,
                                       &subAttributes,
                                       internalAttrs,
                                       resourceSet,
                                       &subscription);

        if (rc != OK) goto mod_exit;

        DEBUG_ONLY bool changedPolicy = ieq_setPolicyInfo(pThreadData, hQueue, pPolicyInfo);
        assert(changedPolicy == true);

        subscription->queueHandle = hQueue;

        // Add to the tree indicating that this is recovery mode.
        rc = iett_addSubToEngineTopic(pThreadData,
                                      topicString,
                                      subscription,
                                      transData == NULL ? NULL : transData->pTran,
                                      true,
                                      false);

        if (rc != OK) goto mod_exit;

        // Make sure the durable object count is up-to-date for this client
        if ((internalAttrs & iettSUBATTR_PERSISTENT) == iettSUBATTR_PERSISTENT)
        {
            // Increment the owning client's durable object count, creating
            // the client state if it doesn't already exist -- note, the protocolID
            // to use if a client doesn't exist should only ever be SHARED for globally
            // shared subscriptions, or JMS (because a non-shared durable MQTT subscription
            // should have a durable MQTT client in the store too).
            rc = iecs_updateDurableObjectCountForClientId(pThreadData,
                                                          clientId,
                                                          policyType == ismSEC_POLICY_SUBSCRIPTION ?
                                                            PROTOCOL_ID_SHARED : PROTOCOL_ID_JMS,
                                                          true);

            if (rc != OK) goto mod_exit;
        }

        // Shared subscription, add to the sharers.
        if (subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED)
        {
            // NOTE: We are intentionally not locking the subscription since during
            //       rehydration we should be the only thread modifying it.

            if (anonymousSharers)
            {
                assert((subOptions & (ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                      ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY)) != 0);

                rc = iett_shareSubscription(pThreadData,
                                            NULL, // Anonymously shared, no clientId
                                            anonymousSharers,
                                            subscription,
                                            &subAttributes,
                                            NULL);

                if (rc != OK) goto mod_exit;
            }

            // Add the sharing clients to the list for this subscription
            if (sharingClientCount != 0)
            {
                assert(sharingSubOptionBuffer != NULL);
                assert(sharingClientIdsBuffer != NULL);

                char *sharingClientId = sharingClientIdsBuffer;
                for(uint32_t sharingClient=0; sharingClient < sharingClientCount; sharingClient++)
                {
                    memcpy(&subAttributes.subOptions,
                           &sharingSubOptionBuffer[sharingClient * sizeof(uint32_t)],
                           sizeof(uint32_t));

                    if (sharingSubIdBuffer != NULL)
                    {
                        memcpy(&subAttributes.subId,
                               &sharingSubIdBuffer[sharingClient * sizeof(ismEngine_SubId_t)],
                               sizeof(ismEngine_SubId_t));
                    }
                    else
                    {
                        subAttributes.subId = ismENGINE_NO_SUBID;
                    }

                    // Add this client to the sharers
                    rc = iett_shareSubscription(pThreadData,
                                                sharingClientId,
                                                iettNO_ANONYMOUS_SHARER,
                                                subscription,
                                                &subAttributes,
                                                NULL);

                    if (rc != OK) goto mod_exit;

                    // Move to the next client Id
                    sharingClientId += strlen(sharingClientId) + 1;
                }
            }
        }
    }

mod_exit:

    if (rc != OK)
    {
        //D'oh we didn't manage to rehydrate. 'Fess up:
        ierr_recordBadStoreRecord(pThreadData,
                                  ISM_STORE_RECTYPE_SPROP,
                                  recHandle,
                                  subName,
                                  rc);

        // If we decide to do partial store recovery, remove the problematic subscription.
        // To do that, we also need to make sure that the queue knows what the SPR record handle is.
        ieq_setPropsHdl(hQueue, recHandle);
        ieq_markQDeleted(pThreadData, hQueue, false);
    }

    if (queueName != NULL) iett_freeSubQueueName(pThreadData, queueName);

    ieutTRACEL(pThreadData, subscription, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, subscription=%p\n", __func__,
               rc, subscription);

    return rc;
}

//****************************************************************************
/// @brief  Rehydrate a stored topic definition
///
/// Recreates a topic based on the information read from a TDR in the store.
///
/// @param[in]     recHandle         Store handle of the TDR
/// @param[in]     record            Pointer to the TDR
/// @param[in]     transData         Info about transaction (or NULL)
/// @param[out]    rehydratedRecord  The topic tree node created for this definition
/// @param[in]     pContext          Context (unused)
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The record must be consistent upon return.
//****************************************************************************
int32_t iett_rehydrateTopicDefn(ieutThreadData_t *pThreadData,
                                ismStore_Handle_t recHandle,
                                ismStore_Record_t *record,
                                ismEngine_RestartTransactionData_t *transData,
                                void **rehydratedRecord,
                                void *pContext)
{
    int32_t rc = OK;
    iettTopicMigrationInfo_t *topicMigrationInfo = NULL;
    iestTopicDefinitionRecord_t *pTDR;

    ieutTRACEL(pThreadData, recHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Verify this is a subscription definition record
    assert(record->Type == ISM_STORE_RECTYPE_TOPIC);
    assert(record->FragsCount == 1);

    pTDR = (iestTopicDefinitionRecord_t *)(record->pFrags[0]);
    ismEngine_CheckStructId(pTDR->StrucId, iestTOPIC_DEFN_RECORD_STRUCID, ieutPROBE_007);

    if (LIKELY(pTDR->Version == iestTDR_CURRENT_VERSION))
    {
        assert(ismEngine_serverGlobal.maintree->retStoreHandle == ismSTORE_NULL_HANDLE);

        ismEngine_serverGlobal.maintree->retStoreHandle = recHandle;
    }
    else
    {
        if (pTDR->Version == iestTDR_VERSION_1)
        {
            iettTopicNode_t *topicNode;

            iestTopicDefinitionRecord_V1_t *pTDR_V1 = (iestTopicDefinitionRecord_V1_t *)pTDR;

            char *topicString = (char*)(pTDR_V1+1);

            ieutTRACEL(pThreadData, record->State, ENGINE_HIFREQ_FNC_TRACE, "Found version %d TDR for topic '%s' (State:0x%016lx)\n",
                       pTDR_V1->Version, topicString, record->State);

            topicMigrationInfo = (iettTopicMigrationInfo_t *)iemem_calloc(pThreadData,
                                                                          IEMEM_PROBE(iemem_topicsTree, 5), 1,
                                                                          sizeof(iettTopicMigrationInfo_t));

            if (NULL == topicMigrationInfo)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            assert(topicMigrationInfo != NULL);
            memcpy(topicMigrationInfo->strucId, iettTOPIC_MIGRATION_INFO_STRUCID, 4);
            topicMigrationInfo->storeHandle = recHandle;

            // This is a deleted record, we don't add it to the tree we delete it after
            // restart has completed.
            if (record->State & iestTDR_STATE_DELETED)
            {
                // NOTE: For deleted nodes, we store the entire topic string in the node's substring
                //       as a diagnostic aid.
                topicNode = (iettTopicNode_t*)iemem_calloc(pThreadData,
                                                           IEMEM_PROBE(iemem_topicsTree, 3), 1,
                                                           sizeof(iettTopicNode_t)+strlen(topicString)+1);

                if (NULL == topicNode)
                {
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    goto mod_exit;
                }

                memcpy(topicNode->strucId, iettTOPIC_NODE_STRUCID, 4);
                topicNode->nodeFlags = iettNODE_FLAG_DELETED;
                strcpy((char *)(topicNode+1), topicString);
                assert(topicNode->resourceSet == iereNO_RESOURCE_SET);
            }
            else
            {
                iettTopic_t topic = {0};
                const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
                uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];

                // Analyse the topic string for this node
                topic.destinationType = ismDESTINATION_TOPIC;
                topic.topicString = topicString;
                topic.substrings = substrings;
                topic.substringHashes = substringHashes;
                topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

                rc = iett_analyseTopicString(pThreadData, &topic);

                if (rc != OK) goto mod_exit;

                rc = iett_insertOrFindTopicsNode(pThreadData,
                                                 ismEngine_serverGlobal.maintree->topics,
                                                 &topic,
                                                 iettOP_ADD,
                                                 &topicNode);

                if (NULL != topic.topicStringCopy)
                {
                    iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

                    if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
                    if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
                }

                if (rc != OK) goto mod_exit;

                assert(topicNode != NULL);
            }

            assert(topicNode != NULL);

            topicMigrationInfo->topicNode = topicNode;
        }
        else
        {
            rc = ISMRC_InvalidValue;
            ism_common_setErrorData(rc, "%u", pTDR->Version);
            goto mod_exit;
        }
    }

    // Pass the migration info back to the caller. This will be passed back later as the
    // requestingRecord for the corresponding TPR and RMRs.
    *rehydratedRecord = topicMigrationInfo;

mod_exit:

    if (rc != OK)
    {
        ierr_recordBadStoreRecord( pThreadData
                                 , record->Type
                                 , recHandle
                                 , NULL
                                 , rc);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, topicMigrationInfo=%p\n", __func__, rc, topicMigrationInfo);

    return rc;
}

//****************************************************************************
/// @brief  Rehydrate retained message reference read from the store
///
/// Recreates the retained message
///
/// @param[in]     owner        Pointer to Topic structure
/// @param[in]     child        Pointer to Message structure
/// @param[in]     refHandle    Message Reference Handle
/// @param[in]     reference    Message reference
/// @param[in]     pContext     Context (unused)
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The record must be consistent upon return.
//****************************************************************************
int32_t iett_rehydrateRetainedMsgRef(ieutThreadData_t *pThreadData,
                                     void *owner,
                                     void *child,
                                     ismStore_Handle_t refHandle,
                                     ismStore_Reference_t *reference,
                                     ismEngine_RestartTransactionData_t *transData,
                                     void *pContext)
{
    int32_t rc = OK;
    iettTopicMigrationInfo_t *topicMigrationInfo = (iettTopicMigrationInfo_t *)owner;
    iettTopicNode_t *topicNode = NULL;
    ismEngine_Message_t *message = (ismEngine_Message_t *)child;
    ismEngine_TransactionHandle_t pTran;

    ieutTRACEL(pThreadData, refHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(message != NULL);

    // Verify we have been given a message reference
    ismEngine_CheckStructId(message->StrucId, ismENGINE_MESSAGE_STRUCID, ieutPROBE_009);

    // Is this happening as part of a transaction or not?
    if (transData != NULL)
    {
        pTran = transData->pTran;
    }
    else
    {
        pTran = NULL;
    }

    const char *serverUID;
    ism_time_t serverTimestamp;
    char *topicString = NULL;

    if (topicMigrationInfo != NULL)
    {
        topicNode = topicMigrationInfo->topicNode;
        serverUID = ism_common_getServerUID();
        serverTimestamp = (ism_time_t)(reference->OrderId);
    }
    else
    {
        // We expect this message to be loaded.
        assert(message->AreaCount != 0);

        // Extract the topic string from the message
        concat_alloc_t  props;
        ism_field_t field = {0};

        iem_locateMessageProperties(message, &props);

        // The properties contain a topic string
        ism_common_findPropertyID(&props, ID_Topic, &field);

        topicString = field.val.s;

        assert(topicString != NULL);

        // The properties contain an originating serverUID
        ism_common_findPropertyID(&props, ID_OriginServer, &field);
        serverUID = field.val.s;
        assert(serverUID != NULL);

        // The properties contain an originating server timestamp
        ism_common_findPropertyID(&props, ID_ServerTime, &field);
        serverTimestamp = field.val.l;
        assert(serverTimestamp != 0);

        iettTopic_t topic = {0};
        const char *substrings[iettDEFAULT_SUBSTRING_ARRAY_SIZE];
        uint32_t substringHashes[iettDEFAULT_SUBSTRING_ARRAY_SIZE];

        // Analyse the topic string for this node
        topic.destinationType = ismDESTINATION_TOPIC;
        topic.topicString = topicString;
        topic.substrings = substrings;
        topic.substringHashes = substringHashes;
        topic.initialArraySize = iettDEFAULT_SUBSTRING_ARRAY_SIZE;

        rc = iett_analyseTopicString(pThreadData, &topic);

        if (rc != OK) goto mod_exit;

        rc = iett_insertOrFindTopicsNode(pThreadData,
                                         ismEngine_serverGlobal.maintree->topics,
                                         &topic,
                                         iettOP_ADD,
                                         &topicNode);

        if (NULL != topic.topicStringCopy)
        {
            iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

            if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
            if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
        }

        if (rc != OK) goto mod_exit;
    }

    assert(topicNode != NULL);

    iettOriginServer_t *originServer;

    rc = iett_insertOrFindOriginServer(pThreadData,
                                       serverUID,
                                       iettOP_ADD,
                                       &originServer);

    if (rc != OK) goto mod_exit;

    assert(originServer != NULL);

    // The retained message originated on this server
    if ((reference->State & iettRMR_STATE_ORIGINSERVER_LOCAL) != 0)
    {
        // If the originServer isn't flagged as localServer, this must have
        // been published when the server had a previous serverUID.
        //
        // We'll flag this originServer as localServer aswell so that we
        // can correctly identify all retained messages that originated here.
        if (originServer->localServer == false)
        {
            ieutTRACEL(pThreadData, reference->State, ENGINE_INTERESTING_TRACE,
                       "RMR state 0x%02x for OriginServer %p (ServerUID:%s). Marking it localServer.\n",
                       reference->State, originServer, originServer->serverUID);

            originServer->localServer = true;
        }
    }

    // Verify this is a topic node
    ismEngine_CheckStructId(topicNode->strucId, iettTOPIC_NODE_STRUCID, ieutPROBE_008);

    char *topicSubstring = (char *)(topicNode+1);

    // Check that NullRetained messages have an expiry time
    assert((message->Header.MessageType != MTYPE_NullRetained) || (message->Header.Expiry > 0));

    // Ensure that the store reference count for the retained message is correct.
    iest_rehydrateMessageRef(pThreadData, message);

    // Record that this message is now being referenced - this makes good the memory
    // use count of the message.
    iem_recordMessageUsage(message);

    #if defined(DISPLAY_LOTS_OF_DETAIL_ABOUT_RMRS)
    static ism_ts_t *ts = NULL;
    char timeStampBuffer[80];

    if (ts == NULL) ts = ism_common_openTimestamp(ISM_TZF_UTC);

    ism_common_setTimestamp(ts, serverTimestamp);
    ism_common_formatTimestamp(ts, timeStampBuffer, 80, 7, ISM_TFF_SPACE|ISM_TFF_ISO8601);

    ieutTRACEL(pThreadData, reference->OrderId, ENGINE_INTERESTING_TRACE, "Found RMR with from server '%s' OrderId=%lu timeStamp=%ld (%s) %sTopic='%s' message=%p expiry=%u msgType=0x%X persistence=%u reliability=%u.\n",
               serverUID ? serverUID : "<NONE>", reference->OrderId, serverTimestamp, timeStampBuffer,
               topicString ? "" : "sub", topicString ? topicString : topicSubstring,
               message, message->Header.Expiry, (uint32_t)message->Header.MessageType,
               message->Header.Persistence, message->Header.Reliability);

    // Because this is for debug only, we are not doing what we should: ism_common_closeTimestamp(ts);
    #else
    ieutTRACEL(pThreadData, reference->OrderId, ENGINE_HIGH_TRACE, "Found RMR with from server '%s' OrderId=%lu timeStamp=%ld %sTopic='%s' msgType=0x%X.\n",
               serverUID ? serverUID : "<NONE>", reference->OrderId, serverTimestamp,
               topicString ? "" : "sub", topicString ? topicString : topicSubstring,
               (uint32_t)message->Header.MessageType);
    #endif

    iereResourceSetHandle_t resourceSet = topicNode->resourceSet;

    // If this was part of a transaction, we need to set the topic node and soft log up
    // as though the message had just been created, since the mechanism used to make the
    // node good is to either commit or rollback the transaction (most likely rollback)
    if (pTran != NULL)
    {
        ietrSLE_Header_t *pSLE;
        ietrSLE_Header_t **ppSLE;

        if (topicMigrationInfo != NULL)
        {
            if (topicMigrationInfo->inflightSLECount == topicMigrationInfo->inflightSLEMax)
            {
                uint64_t newInflightSLEMax = topicMigrationInfo->inflightSLEMax+10;

                ietrSLE_Header_t **newInflightSLEs = iemem_realloc(pThreadData,
                                                                   IEMEM_PROBE(iemem_topicsTree, 6),
                                                                   topicMigrationInfo->inflightSLEs,
                                                                   newInflightSLEMax * sizeof(ietrSLE_Header_t *));

                if (newInflightSLEs == NULL)
                {
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    goto mod_exit;
                }

                ismEngine_Transaction_t **newInflightSLETrans = iemem_realloc(pThreadData,
                                                                              IEMEM_PROBE(iemem_topicsTree, 7),
                                                                              topicMigrationInfo->inflightSLETrans,
                                                                              newInflightSLEMax * sizeof(ismEngine_Transaction_t *));

                if (newInflightSLETrans == NULL)
                {
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    goto mod_exit;
                }

                topicMigrationInfo->inflightSLEs = newInflightSLEs;
                topicMigrationInfo->inflightSLETrans = newInflightSLETrans;
                topicMigrationInfo->inflightSLEMax = newInflightSLEMax;
            }

            topicMigrationInfo->inflightSLETrans[topicMigrationInfo->inflightSLECount] = pTran;
            ppSLE = &topicMigrationInfo->inflightSLEs[topicMigrationInfo->inflightSLECount];
        }
        else
        {
            ppSLE = &pSLE;
        }

        // The message needs an additional store reference count and an additional
        // in-memory reference count both of which are released when late subscribers
        // have been dealt with in the transaction commit / rollback.
        iest_rehydrateMessageRef(pThreadData, message);
        iem_recordMessageUsage(message);

        topicNode->pendingUpdates++;

        iettSLEUpdateRetained_t SLE;

        SLE.topicNode = topicNode;
        SLE.message = message;
        SLE.refHandle = refHandle;
        SLE.orderId = reference->OrderId;
        SLE.originServer = originServer;
        SLE.timestamp = serverTimestamp;
        SLE.nextInflightRetUpdate = NULL;
        SLE.savepointRollback = false;
        SLE.repositioningRetained = false;

        // If this is committed, and becomes the new retained we need to deliver it
        // to new subscribers (whose subsUpdatesValue will be greater than 1) but not to
        // old subscribers (whose subsUpdatesValue will be 1). Setting the publishSUV
        // to 1 achieves this.
        SLE.publishSUV = 1;

        // Ensure we have details of the link to the transaction
        SLE.TranRef.hTranRef = transData->operationRefHandle;
        SLE.TranRef.orderId = transData->operationReference.OrderId;

        // NOTE: We are not registering for the 'SavepointRollback' phase in the rehydrated SLE.
        //
        // There will be no Savepoints because they're in-memory only, and the only reason for
        // registering for that phase in the 1st place is to ignore uncommitted store handles,
        // which, clearly (because we've rehydrated things) the store handles involved here were.
        rc = ietr_softLogRehydrate( pThreadData
                                  , transData
                                  , ietrSLE_TT_UPDATERETAINED
                                  , iett_SLEReplayUpdateRetained
                                  , NULL
                                  , Commit | Rollback | PostCommit | PostRollback | Cleanup
                                  , &SLE
                                  , sizeof(SLE)
                                  , 1, 1
                                  , ppSLE);

        if (rc != OK) goto mod_exit;

        // Retained messages count against the resourceSet matching the topic
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_updateMessageResourceSet(pThreadData, resourceSet, message, false, true);

        // Add to the list of inflight SLEs
        assert(ppSLE != NULL);
        assert(*ppSLE != NULL);

        iett_addInflightRetUpdate(pThreadData, topicNode, (iettSLEUpdateRetained_t *)((*ppSLE)+1));

        if (topicMigrationInfo != NULL)
        {
            topicMigrationInfo->inflightSLECount++;
        }
    }
    // Non transactional case
    else
    {
        bool addRetained = true;

        // Retained messages count against the resourceSet matching the topic
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_updateMessageResourceSet(pThreadData, resourceSet, message, false, true);

        if (topicNode->currRetMessage != NULL)
        {
            // One of these two retained messages is unneeded
            iettUnneededRetained_t *newUnneededMsg = iemem_malloc(pThreadData,
                                                                  IEMEM_PROBE(iemem_unneededRetainedMsgs, 1),
                                                                  sizeof(iettUnneededRetained_t));

            assert(newUnneededMsg != NULL);

            // Adjust the statistics - we're going to increment at the end of the function, so we
            // need to decrement now regardless of which one we choose to keep.
            pThreadData->stats.internalRetainedMsgCount--;

            // We don't need the one we've already recovered (we need this new one) if it has
            // a newer timestamp OR has the same timestamp but a newer orderId
            if (serverTimestamp > topicNode->currRetTimestamp ||
                ((serverTimestamp == topicNode->currRetTimestamp) &&
                 (reference->OrderId > topicNode->currRetOrderId)))
            {
                ismEngine_Message_t *currMessage = topicNode->currRetMessage;

                if ((topicNode->nodeFlags & iettNODE_FLAG_NULLRETAINED) == 0)
                {
                    pThreadData->stats.externalRetainedMsgCount--;
                    iere_primeThreadCache(pThreadData, resourceSet);
                    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSGS, -1);
                    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSG_BYTES, -(currMessage->fullMemSize));
                }

                if (topicNode->expiryTime != 0)
                {
                    ieme_removeTopicFromExpiryReaperList(pThreadData, topicNode);
                    pThreadData->stats.retainedExpiryMsgCount--;
                }

                // Always remove the node from the origin server even if it is unchanged
                // to ensure we go through all of the statistics updating correctly.
                assert(topicNode->currOriginServer != NULL);
                iett_removeTopicNodeFromOriginServerRecovery(pThreadData,
                                                             topicNode,
                                                             topicNode->currOriginServer);
                topicNode->currOriginServer = NULL;

                currMessage->Header.Flags &= ~ismMESSAGE_FLAGS_PROPAGATE_RETAINED;

                newUnneededMsg->message = currMessage;
                newUnneededMsg->retRefHandle = topicNode->currRetRefHandle;
                newUnneededMsg->retRefOrderId = topicNode->currRetOrderId;
            }
            // We don't need this new one...
            else
            {
                newUnneededMsg->message = message;
                newUnneededMsg->retRefHandle = refHandle;
                newUnneededMsg->retRefOrderId = reference->OrderId;
                addRetained = false;
            }

            assert(newUnneededMsg->retRefHandle != ismSTORE_NULL_HANDLE);

            ieutTRACEL(pThreadData, refHandle, ENGINE_INTERESTING_TRACE,
                       "Resolving RMR conflict with addRetained=%d. refHandle=0x%0lx, serverTimestamp=%lu, topicNode->currRetRefHandle=0x%0lx, topicNode->currRetTimestamp=%lu.\n",
                       addRetained, refHandle, serverTimestamp, topicNode->currRetRefHandle, topicNode->currRetTimestamp);

            newUnneededMsg->next = unneededRetainedMsgs;
            unneededRetainedMsgs = newUnneededMsg;
        }

        if (addRetained)
        {
            if (message->Header.MessageType == MTYPE_NullRetained)
            {
                topicNode->nodeFlags |= iettNODE_FLAG_NULLRETAINED;
            }
            else
            {
                topicNode->nodeFlags &= ~iettNODE_FLAG_NULLRETAINED;
                pThreadData->stats.externalRetainedMsgCount++;
                iere_primeThreadCache(pThreadData, resourceSet);
                iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSGS, 1);
                iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSG_BYTES, message->fullMemSize);
            }

            topicNode->currRetOrderId = reference->OrderId;
            topicNode->currRetRefHandle = refHandle;
            topicNode->currRetMessage = message;
            topicNode->currRetTimestamp = serverTimestamp;
            topicNode->expiryTime = message->Header.Expiry;

            if (topicNode->expiryTime != 0)
            {
                // Add node to the expiry list
                ieme_addTopicToExpiryReaperList(pThreadData, topicNode);

                pThreadData->stats.retainedExpiryMsgCount++;
            }

            message->Header.Flags |= ismMESSAGE_FLAGS_PROPAGATE_RETAINED;

            assert(topicNode->currOriginServer == NULL);
            rc = iett_addTopicNodeToOriginServerRecovery(pThreadData,
                                                         topicNode,
                                                         originServer);

            if (rc != OK) goto mod_exit;

            assert(topicNode->currOriginServer == originServer);
        }
    }

    // For non-migrated topics, make sure we keep minimum active orderId vote up-to-date
    if (topicMigrationInfo == NULL)
    {
        if ((topicNode->activeOrderIdVote == 0) ||
            (reference->OrderId < topicNode->activeOrderIdVote))
        {
            topicNode->activeOrderIdVote = reference->OrderId;
        }
    }

    // Whether the message is the current retained or is in-flight we increment the
    // retained count
    pThreadData->stats.internalRetainedMsgCount++;

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief  Add an iettSLEUpdateRetained_t to the inflight chain for the
///         specified node.
///
/// @param[in]     topicNode    The topic node whose chain is to be added to
/// @param[in]     pSLE         SLE to add
//****************************************************************************
void iett_addInflightRetUpdate(ieutThreadData_t *pThreadData,
                               iettTopicNode_t *topicNode,
                               iettSLEUpdateRetained_t *pSLE)
{
    assert(topicNode != NULL);
    assert(pSLE != NULL);
    assert(pSLE->nextInflightRetUpdate == NULL);
    assert(pSLE->topicNode == topicNode);

    pSLE->nextInflightRetUpdate = topicNode->inflightRetUpdates;
    topicNode->inflightRetUpdates = pSLE;
}

//****************************************************************************
/// @brief  Remove an iettSLEUpdateRetained_t from the inflight chain for the
///         specified node.
///
/// @param[in]     topicNode    The topic node whose chain is to be added to
/// @param[in]     pSLE         SLE to add
//****************************************************************************
void iett_removeInflightRetUpdate(ieutThreadData_t *pThreadData,
                                  iettTopicNode_t *topicNode,
                                  iettSLEUpdateRetained_t *pSLE)
{
    assert(topicNode != NULL);
    assert(pSLE != NULL);
    assert(pSLE->topicNode == topicNode);

    iettSLEUpdateRetained_t *prevSLE = NULL;
    iettSLEUpdateRetained_t *currSLE = topicNode->inflightRetUpdates;

    // Remove the specified SLE from the chain
    while(currSLE != NULL)
    {
        if (currSLE == pSLE)
        {
            if (prevSLE != NULL)
            {
                prevSLE->nextInflightRetUpdate = pSLE->nextInflightRetUpdate;
            }
            else
            {
                topicNode->inflightRetUpdates = pSLE->nextInflightRetUpdate;
            }

            pSLE->nextInflightRetUpdate = NULL;
            break;
        }

        prevSLE = currSLE;
        currSLE = currSLE->nextInflightRetUpdate;
    }
}

//****************************************************************************
/// @brief  Return the highest timestamp in the inflight retained updates
///
/// @param[in]     topicNode    The topic node whose chain is to be added to
///
/// @return highest ism_time_t found (0 for none).
//****************************************************************************
ism_time_t iett_findHighestInflightRetTimestamp(ieutThreadData_t *pThreadData,
                                                iettTopicNode_t *topicNode)
{
    ism_time_t highestTimestamp = 0;

    iettSLEUpdateRetained_t *currSLE = topicNode->inflightRetUpdates;

    while(currSLE != NULL)
    {
        if (currSLE->timestamp > highestTimestamp) highestTimestamp = currSLE->timestamp;

        currSLE = currSLE->nextInflightRetUpdate;
    }

    return highestTimestamp;
}

//****************************************************************************
/// @brief  Complete the initalization of the topic tree 
///
/// @remark This function opens the reference context for the store topic
///         object - if we want it to do more then we might need to consider
///         splitting this to it's own function because it's called from places
///         other than server startup.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_reconcileEngineTopicTree(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ieutTRACEL(pThreadData, tree, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (tree->retStoreHandle != ismSTORE_NULL_HANDLE)
    {
        assert(tree->retRefContext == NULL);

        // Get store reference statistics to initialize the tree
        ismStore_ReferenceStatistics_t refStats;

        rc = ism_store_openReferenceContext(tree->retStoreHandle, &refStats, &tree->retRefContext);

        if (rc != OK)
        {
            ism_common_setError(rc);
        }
        else
        {
            assert(tree->retRefContext != NULL);

            // Remove any unneeded rehydrated retained messages discovered during recovery
            // in 2 phases, the first being to delete the reference, then unstore the
            // message.
            uint32_t storeOpCount = 0;
            iettUnneededRetained_t *currentUnneededMsg = unneededRetainedMsgs;
            while(currentUnneededMsg != NULL)
            {
                rc = ism_store_deleteReference(pThreadData->hStream,
                                               tree->retRefContext,
                                               currentUnneededMsg->retRefHandle,
                                               currentUnneededMsg->retRefOrderId,
                                               0);

                assert(rc == OK);
                storeOpCount++;

                currentUnneededMsg = currentUnneededMsg->next;

                if (storeOpCount > 1000 || currentUnneededMsg == NULL)
                {
                    iest_store_commit(pThreadData, false);
                    storeOpCount = 0;
                }
            }

            // Phase 2 - unstore and release the messages (and free up the list)
            while(unneededRetainedMsgs != NULL)
            {
                currentUnneededMsg = unneededRetainedMsgs;
                unneededRetainedMsgs = currentUnneededMsg->next;

                // We take the risk of the deleteReference not being committed before
                // the unstore because this is happening during restart.
                rc = iest_unstoreMessage(pThreadData,
                                         currentUnneededMsg->message,
                                         false, false, NULL, &storeOpCount);

                assert(rc == OK);

                iem_releaseMessage(pThreadData, currentUnneededMsg->message);

                iemem_free(pThreadData, iemem_unneededRetainedMsgs, currentUnneededMsg);

                if (storeOpCount > 1000 || unneededRetainedMsgs == NULL)
                {
                    iest_store_commit(pThreadData, false);
                    storeOpCount = 0;
                }
            }

            assert(unneededRetainedMsgs == NULL);

            ieutTRACEL(pThreadData, refStats.MinimumActiveOrderId, ENGINE_HIGH_TRACE, "Reference Statistics: GenIds=%hu-%hu HighestOrderId=%lu MinimumActiveOrderId=%lu\n",
                       refStats.LowestGenId, refStats.HighestGenId,
                       refStats.HighestOrderId, refStats.MinimumActiveOrderId);
            ieutTRACEL(pThreadData, refStats.HighestOrderId, ENGINE_HIGH_TRACE, "Setting minActiveRetOrderId and nextRetOrderId to %lu\n",
                       refStats.HighestOrderId);

            // Use the minimum active order id returned in the stats
            tree->retMinActiveOrderId = refStats.MinimumActiveOrderId;

            // The nextRetOrderId is incremented when the next retained is published, so we can
            // start by setting it to the previous highest value now.
            tree->retNextOrderId = refStats.HighestOrderId;
        }
    }
    else
    {
        // This code once created the topic record if it hadn't already existed, however
        // that would mean that as soon as a server starts up, it has a topic record -
        // which then means it appears not to be empty...

        // Instead, we now create the topic record only when a retained message is being
        // published, or migrated. This happens in iett_ensureEngineStoreTopicRecord.

        // Once a retained message has been published, it is OK to treat the store as
        // 'not empty'.
    }


    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


//****************************************************************************
/// @brief Change the reference context (owning Topic object) of a retained msg
/// to the single tree reference context.
///
/// @param[in]     pThreadData       Thread data for the calling thread
/// @param[in]     oldMessage        The message handle of the message being
///                                  repositioned.
/// @param[in]     originServer      OriginServer of the message being repositioned
/// @param[in]     oldRefContext     The reference context in which the old message
///                                  was referenced.
/// @param[in]     oldRefHandle      The old reference handle.
/// @param[in]     oldRefOrderId     The old reference orderId
/// @param[in]     newRetTimestamp   For a retained message, the new timestamp to set
/// @param[in]     pTran             Transaction
/// @param[in,out] pSLE              Soft log entry for which this is being repositioned
/// @param[out]    pNewMessage       The new message which may have been created as
///                                  a result of repositioning
/// @param[out]    pNewRefHandle     The new reference handle.
/// @param[out]    pNewRefOrderId    The new reference orderid
///
/// @remark The new reference will be a reference created against the single
///         retained owner record's reference context (tree->retRefContext).
///
/// @remark The topic tree lock should be held for WRITE before calling this
///         function (but during recovery we can skip this while we are single threaded)
///
/// @remark If a transaction and an SLE are specified a new transaction reference is
///         created and the old one deleted.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static int32_t iett_changeRetainedRefContext(ieutThreadData_t *pThreadData,
                                             ismEngine_MessageHandle_t oldMessage,
                                             iettOriginServer_t *originServer,
                                             void *oldRefContext,
                                             ismStore_Handle_t oldRefHandle, uint64_t oldRefOrderId,
                                             ism_time_t newRetTimestamp,
                                             ismEngine_Transaction_t *pTran, iettSLEUpdateRetained_t *pSLE,
                                             ismEngine_MessageHandle_t *pNewMessage,
                                             ismStore_Handle_t *pNewRefHandle, uint64_t *pNewRefOrderId,
                                             ismStore_Handle_t *pNewTranRefHandle, uint64_t *pNewTranRefOrderId)
{
    int32_t rc;
    void *tranRefContext = NULL;
    ismEngine_Message_t *newMsg = NULL;
    ismStore_Handle_t newMsgRefHandle = ismSTORE_NULL_HANDLE;
    uint64_t newMsgRefOrderId = 0;
    ismStore_Handle_t tranRefHandle = ismSTORE_NULL_HANDLE;
    uint64_t tranRefOrderId = 0;

    ieutTRACEL(pThreadData, oldMessage,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "oldMessage=%p oldRefOrderId=%lu\n",
               __func__, oldMessage, oldRefOrderId);

    // We always move *TO* the single retained owner
    iettTopicTree_t *tree = iett_getEngineTopicTree(pThreadData);

    // Don't want to allow the use of this function to move retained messages on the same context
    if (oldRefContext == tree->retRefContext)
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
        goto mod_exit;
    }

    rc = iem_createMessageCopy(pThreadData,
                               oldMessage,
                               false,
                               newRetTimestamp,
                               oldMessage->Header.Expiry,
                               &newMsg);

    if (rc != OK) goto mod_exit;

    // Make sure that this repositioning will not change expiry processing
    assert(oldMessage->Header.Expiry == newMsg->Header.Expiry);

    // Make sure the new message doesn't get stored with ismMESSAGE_FLAGS_PROPAGATE_RETAINED set.
    assert(((pTran == NULL) && ((newMsg->Header.Flags & ismMESSAGE_FLAGS_PROPAGATE_RETAINED) != 0)) ||
           ((pTran != NULL) && ((newMsg->Header.Flags & ismMESSAGE_FLAGS_PROPAGATE_RETAINED) == 0)));

    newMsg->Header.Flags &= ~ismMESSAGE_FLAGS_PROPAGATE_RETAINED;

    if (pTran != NULL)
    {
        assert(pSLE != NULL);

        rc = ism_store_openReferenceContext(pTran->hTran, NULL, &tranRefContext);

        if (rc != OK) goto mod_exit;

        tranRefOrderId = pTran->nextOrderId + 1;
    }

    newMsgRefOrderId = tree->retNextOrderId + 1;

    do
    {
        uint32_t storeOperations = 0;

        rc = ism_store_deleteReference(pThreadData->hStream,
                                       oldRefContext,
                                       oldRefHandle,
                                       oldRefOrderId,
                                       0);

        ismStore_Handle_t newMsgStoreHandle = ismSTORE_NULL_HANDLE;

        if (rc == OK)
        {
            storeOperations++;

            rc = iest_storeMessage(pThreadData,
                                   newMsg,
                                   1,
                                   iestStoreMessageOptions_EXISTING_TRANSACTION,
                                   &newMsgStoreHandle);
        }

        if (rc == OK)
        {
            storeOperations++;

            assert(newMsg->StoreMsg.parts.hStoreMsg == newMsgStoreHandle);
            assert(tree->retRefContext != NULL);

            ismStore_Reference_t newMsgRef = {0};

            newMsgRef.OrderId = newMsgRefOrderId;
            newMsgRef.hRefHandle = newMsgStoreHandle;
            newMsgRef.State = originServer->localServer ? iettRMR_STATE_ORIGINSERVER_LOCAL :
                                                          iettRMR_STATE_NONE;

            rc = ism_store_createReference(pThreadData->hStream,
                                           tree->retRefContext,
                                           &newMsgRef,
                                           0,
                                           &newMsgRefHandle);
        }

        if (rc == OK)
        {
            storeOperations++;

            if (tranRefContext != NULL)
            {
                // Delete the old transaction reference
                rc = ism_store_deleteReference(pThreadData->hStream,
                                               tranRefContext,
                                               pSLE->TranRef.hTranRef,
                                               pSLE->TranRef.orderId,
                                               0);

                if (rc == OK)
                {
                    storeOperations++;

                    // Create a new transaction reference
                    ismStore_Reference_t tranRef = {0};

                    tranRef.OrderId = tranRefOrderId;
                    tranRef.hRefHandle = newMsgRefHandle;
                    tranRef.Value = iestTOR_VALUE_PUT_MESSAGE;
                    tranRef.State = 0;

                    rc = ism_store_createReference(pThreadData->hStream,
                                                   tranRefContext,
                                                   &tranRef,
                                                   0,
                                                   &tranRefHandle);
                }

                if (rc == OK)
                {
                    storeOperations++;
                }
            }
        }

        // Need to commit before unstoring the old message
        if (storeOperations != 0)
        {
            if (rc == OK)
            {
                tree->retNextOrderId += 1;
                iest_store_commit(pThreadData, false);

                // All good - We can now unstore the old message
                assert(oldMessage != newMsg);

                rc = iest_unstoreMessage(pThreadData,
                                         oldMessage,
                                         false,
                                         true,
                                         NULL,
                                         NULL);

                assert(rc == OK);
            }
            else
            {
                iest_store_rollback(pThreadData, false);
                newMsg->StoreMsg.parts.hStoreMsg = ismSTORE_NULL_HANDLE;
                newMsg->StoreMsg.parts.RefCount = 0;
            }

            storeOperations = 0;
        }
    }
    while(rc == ISMRC_StoreGenerationFull);

    if (rc != OK) goto mod_exit;

    // For the actual retained message, have the propagate retained flag on in-memory
    if (pTran == NULL) newMsg->Header.Flags |= ismMESSAGE_FLAGS_PROPAGATE_RETAINED;

    *pNewMessage = newMsg;
    *pNewRefHandle = newMsgRefHandle;
    *pNewRefOrderId = newMsgRefOrderId;
    if (pNewTranRefHandle != NULL) *pNewTranRefHandle = tranRefHandle;
    if (pNewTranRefOrderId != NULL) *pNewTranRefOrderId = tranRefOrderId;

mod_exit:

    if (tranRefContext != NULL)
    {
        if (rc == OK)
        {
            pTran->nextOrderId += 1;
        }

        rc = ism_store_closeReferenceContext(tranRefContext);

        if (rc != OK)
        {
            ism_common_setError(rc);
        }
    }

    if (rc != OK && newMsg != NULL)
    {
        iem_releaseMessage(pThreadData, newMsg);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, newMsgRefOrderId=%lu\n",
               __func__, rc, newMsgRefOrderId);

    return rc;
}

//****************************************************************************
/// @brief  Complete the rehydration of a topic
///
/// Perform final actions required to fully rehydrate a topic object
///
/// @param[in]     topicHandle     Store handle of the TDR
/// @param[in]     migrationInfo   Pointer to the migration info
/// @param[in]     pContext        Context (unused)
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_completeTopicRehydration( ieutThreadData_t *pThreadData
                                     , uint64_t topicHandle
                                     , void *migrationInfo
                                     , void *pContext)
{
    int32_t rc = OK;
    iettTopicMigrationInfo_t *topicMigrationInfo = (iettTopicMigrationInfo_t *)migrationInfo;

    ieutTRACEL(pThreadData, migrationInfo, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // We only expect to see a NULL for the single topic node for all retained msgs
    if (topicMigrationInfo == NULL)
    {
        assert(topicHandle == ismEngine_serverGlobal.maintree->retStoreHandle);
        goto mod_exit;
    }

    iettTopicNode_t *topicNode = topicMigrationInfo->topicNode;

    assert(topicMigrationInfo->storeHandle != ismSTORE_NULL_HANDLE);

    // Move messages to the single topic record...
    if ((topicNode->nodeFlags & iettNODE_FLAG_TYPE_MASK) != iettNODE_FLAG_DELETED)
    {
        // Make sure we've actually created the record for them to move to
        rc = iett_ensureEngineStoreTopicRecord(pThreadData);

        if (rc != OK) goto mod_exit;

        assert(ismEngine_serverGlobal.maintree->retStoreHandle != ismSTORE_NULL_HANDLE);
        assert(ismEngine_serverGlobal.maintree->retRefContext != NULL);

        void *oldTopicRefContext = NULL;

        assert(topicMigrationInfo->storeHandle != ismEngine_serverGlobal.maintree->retStoreHandle);

        rc = ism_store_openReferenceContext(topicMigrationInfo->storeHandle, NULL, &oldTopicRefContext);

        if (rc != OK) goto mod_exit;

        assert(oldTopicRefContext != NULL);

        ismEngine_Message_t *newMessage = NULL;
        ismStore_Handle_t newRetRefHandle = ismSTORE_NULL_HANDLE;
        uint64_t newRetOrderId = 0;
        ism_time_t newRetTimestamp;

        // Move the current retained to the single topic record...
        if (topicNode->currRetRefHandle != ismSTORE_NULL_HANDLE)
        {
            newRetTimestamp = (ism_time_t)(topicNode->currRetOrderId); // Use old orderId to maintain order

            rc = iett_changeRetainedRefContext(pThreadData,
                                               topicNode->currRetMessage,
                                               topicNode->currOriginServer,
                                               oldTopicRefContext,
                                               topicNode->currRetRefHandle, topicNode->currRetOrderId,
                                               newRetTimestamp,
                                               NULL, NULL,
                                               &newMessage,
                                               &newRetRefHandle, &newRetOrderId,
                                               NULL, NULL);

            if (rc != OK) goto mod_exit;

            assert(newMessage != NULL);
            assert(newRetRefHandle != ismSTORE_NULL_HANDLE);
            assert(newRetOrderId != 0);

            // Switch the message usage over.
            if (newMessage != topicNode->currRetMessage)
            {
                assert(newMessage->usageCount == 1); // Inheriting the use count of the copy
                iem_releaseMessage(pThreadData, topicNode->currRetMessage);
                topicNode->currRetMessage = newMessage;
            }

            topicNode->currRetRefHandle = newRetRefHandle;
            topicNode->currRetOrderId = newRetOrderId;
            topicNode->currRetTimestamp = newRetTimestamp;

            if ((topicNode->activeOrderIdVote == 0) ||
                (topicNode->currRetOrderId < topicNode->activeOrderIdVote))
            {
                topicNode->activeOrderIdVote = topicNode->currRetOrderId;
            }
        }

        // Deal with modifying any inflight SLEs
        if (topicMigrationInfo->inflightSLECount != 0)
        {
            // Move all of the SLEs and transaction references to the new single topic.
            for(uint64_t loop=0; loop<topicMigrationInfo->inflightSLECount; loop++)
            {
                ismEngine_Transaction_t *pTran = topicMigrationInfo->inflightSLETrans[loop];
                ietrSLE_Header_t *pSLEHeader = topicMigrationInfo->inflightSLEs[loop];
                iettSLEUpdateRetained_t *pSLE = (iettSLEUpdateRetained_t *)(pSLEHeader+1);

                assert(pSLEHeader->Type == ietrSLE_TT_UPDATERETAINED);

                ismStore_Handle_t newTranRefHandle = ismSTORE_NULL_HANDLE;
                uint64_t newTranOrderId = 0;
                newRetTimestamp = (ism_time_t)(pSLE->orderId); // Use old orderId to maintain order

                rc = iett_changeRetainedRefContext(pThreadData,
                                                   pSLE->message,
                                                   pSLE->originServer,
                                                   oldTopicRefContext,
                                                   pSLE->refHandle, pSLE->orderId,
                                                   newRetTimestamp,
                                                   pTran, pSLE,
                                                   &newMessage,
                                                   &newRetRefHandle, &newRetOrderId,
                                                   &newTranRefHandle, &newTranOrderId);

                if (rc != OK) goto mod_exit;

                // Switch the message usage over
                if (newMessage != pSLE->message)
                {
                    assert(newMessage->usageCount == 1); // Inheriting the use count of the clone
                    iem_releaseMessage(pThreadData, pSLE->message);
                    iest_rehydrateMessageRef(pThreadData, newMessage);
                    iest_unstoreMessageCommit(pThreadData, pSLE->message, 0);
                    iem_recordMessageUsage(newMessage); // Additional use count for being in a transaction
                    iem_releaseMessage(pThreadData, pSLE->message);

                    pSLE->message = newMessage;
                }

                pSLE->refHandle = newRetRefHandle;
                pSLE->orderId = newRetOrderId;
                pSLE->timestamp = newRetTimestamp;

                if ((topicNode->activeOrderIdVote == 0) ||
                    (pSLE->orderId < topicNode->activeOrderIdVote))
                {
                    topicNode->activeOrderIdVote = pSLE->orderId;
                }

                pSLE->TranRef.hTranRef = newTranRefHandle;
                pSLE->TranRef.orderId = newTranOrderId;
            }

            iemem_free(pThreadData, iemem_topicsTree, topicMigrationInfo->inflightSLETrans);
            iemem_free(pThreadData, iemem_topicsTree, topicMigrationInfo->inflightSLEs);
        }

        rc = ism_store_closeReferenceContext(oldTopicRefContext);

        if (rc != OK) goto mod_exit;
    }

    // Delete this topic node (and any references left behind)
    rc = ism_store_deleteRecord(pThreadData->hStream, topicMigrationInfo->storeHandle);

    if (rc != OK) goto mod_exit;

    iest_store_commit(pThreadData, false);

    iemem_freeStruct(pThreadData, iemem_topicsTree, topicMigrationInfo, topicMigrationInfo->strucId);

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief Initialize the topic monitors in the topic tree with admin topic
///        monitor objects.
//****************************************************************************
int32_t iett_reconcileEngineTopicMonitors(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Get Topic monitor configuration information for the engine
    ism_prop_t *props = ism_config_getProperties(ismEngine_serverGlobal.configCallbackHandle,
                                                 ismENGINE_ADMIN_VALUE_TOPICMONITOR,
                                                 NULL);

    const char *propertyName = NULL;

    // Loop through the topic monitors starting topic monitoring on the ones found
    for (int32_t i = 0; rc == OK && ism_common_getPropertyIndex(props, i, &propertyName) == 0; i++)
    {
        // We have discovered a topic monitor.
        if (strncmp(propertyName,
                    ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING,
                    strlen(ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING)) == 0)
        {
            // const char *propQualifier = propertyName + strlen(ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING);
            const char *topicString = ism_common_getStringProperty(props, propertyName);

            rc = ism_engine_startTopicMonitor(topicString, false);
        }
    }

    // We are finished with the properties now - so release them.
    if (NULL != props) ism_common_freeProperties(props);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Initialize the cluster requested topics from the admin defined
///        cluster requested topics.
//****************************************************************************
int32_t iett_reconcileClusterRequestedTopics(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Get Cluster Requested Topics configuration information for the engine
    ism_prop_t *props = ism_config_getProperties(ismEngine_serverGlobal.configCallbackHandle,
                                                 ismENGINE_ADMIN_VALUE_CLUSTERREQUESTEDTOPICS,
                                                 NULL);

    const char *propertyName = NULL;

    // Loop through the results activating cluster requested topic on the ones found
    for (int32_t i = 0; rc == OK && ism_common_getPropertyIndex(props, i, &propertyName) == 0; i++)
    {
        // We have discovered a cluster requested topic
        if (strncmp(propertyName,
                    ismENGINE_ADMIN_PREFIX_CLUSTERREQUESTEDTOPICS_TOPICSTRING,
                    strlen(ismENGINE_ADMIN_PREFIX_CLUSTERREQUESTEDTOPICS_TOPICSTRING)) == 0)
        {
            const char *topicString = ism_common_getStringProperty(props, propertyName);

            rc = iett_activateClusterRequestedTopic(pThreadData, topicString, true);
        }
    }

    // We are finished with the properties now - so release them.
    if (NULL != props) ism_common_freeProperties(props);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Context for iett_reconcileSubsTreeCallback
//****************************************************************************
typedef struct tag_iettReconcileSubsTreeCbContext_t
{
    ismCluster_SubscriptionInfo_t topicInfo[1000];
    uint32_t topicInfoCount;
    int32_t rc;
} iettReconcileSubsTreeCbContext_t;

//****************************************************************************
/// @brief Report interim results of cluster topics to cluster component
///
/// @param[in,out] context  Context information
//****************************************************************************
void iett_reconcileClusterReportTopics(ieutThreadData_t *pThreadData,
                                       iettReconcileSubsTreeCbContext_t *context)
{
    ieutTRACEL(pThreadData, context, ENGINE_FNC_TRACE, FUNCTION_ENTRY "topicInfoCount=%u\n",
               __func__, context->topicInfoCount);

    assert(context->topicInfoCount != 0);

    if (context->rc == OK)
    {
        if (ismEngine_serverGlobal.clusterEnabled)
        {
            // The cluster interface is called 'addSubscriptions' but it really is a list of topics
            // in which this server is interested (i.e. has subscriptions or cluster requested topics)
            context->rc = ism_cluster_addSubscriptions(context->topicInfo, context->topicInfoCount);
        }

        if (context->rc != OK) ism_common_setError(context->rc);
    }

    context->topicInfoCount = 0;

    ieutTRACEL(pThreadData, context->rc, ENGINE_FNC_TRACE, FUNCTION_ENTRY "rc=%d\n",
               __func__, context->rc);
}

//****************************************************************************
// Forward declaration of iett_reconcileClusterTopics
//****************************************************************************
void iett_reconcileClusterTopics(ieutThreadData_t *pThreadData,
                                 iettSubsNode_t *node,
                                 iettReconcileSubsTreeCbContext_t *context);

//****************************************************************************
/// @brief Callback used when reconciling a subscription tree with cluster
///
/// @param[in]     key      Key being processed (topic substring)
/// @param[in]     keyHash  Hash for the key being processed
/// @param[in]     value    Value from the hash table (iettSubsNode_t)
/// @param[in,out] context  Context information
//****************************************************************************
void iett_reconcileClusterSubsTreeCallback(ieutThreadData_t *pThreadData,
                                           char     *key,
                                           uint32_t  keyHash,
                                           void     *value,
                                           void     *context)
{
    iett_reconcileClusterTopics(pThreadData,
                                (iettSubsNode_t *)value,
                                (iettReconcileSubsTreeCbContext_t *)context);
}

//****************************************************************************
/// @brief Visit the nodes below this one in the tree and reconcile with the
///        cluster component
///
/// @param[in]     node     Node from which to begin printing.
/// @param[in,out] context  Context of the dump
///
/// @see iett_dumpTopicTree
//****************************************************************************
void iett_reconcileClusterTopics(ieutThreadData_t *pThreadData,
                                        iettSubsNode_t *node,
                                        iettReconcileSubsTreeCbContext_t *context)
{
    if (context->rc == OK)
    {
        if (node->activeCluster > 0)
        {
            assert((node->nodeFlags & iettNODE_FLAG_TREE_ROOT) == 0);
            assert(node->topicString != NULL);

            context->topicInfo[context->topicInfoCount].pSubscription = node->topicString;
            context->topicInfo[context->topicInfoCount].fWildcard = (node->nodeFlags & iettNODE_FLAG_BRANCH_WILD_OR_MULTI) != 0;

            context->topicInfoCount += 1;

            if (context->topicInfoCount == sizeof(context->topicInfo)/sizeof(context->topicInfo[0]))
            {
                iett_reconcileClusterReportTopics(pThreadData, context);

                assert(context->topicInfoCount == 0);
            }
        }

        if (node->children)
        {
            ieut_traverseHashTable(pThreadData,
                                   node->children,
                                   iett_reconcileClusterSubsTreeCallback,
                                   context);
        }

        if (node->wildcardChild)
        {
            iett_reconcileClusterTopics(pThreadData, node->wildcardChild, context);
        }

        if (node->multicardChild)
        {
            iett_reconcileClusterTopics(pThreadData, node->multicardChild, context);
        }
    }
}

//****************************************************************************
/// @brief  Reconcile the engine subscription topics with the cluster component
///
/// @remark At the end of recovery we will have rebuilt any durable subscriptions
///         for the server into the topic tree and identified cluster requested
///         topics. We need to inform the cluster component of topics that this
///         server is interested in.
///
/// @param[in]     pThreadData     Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_reconcileEngineClusterTopics(ieutThreadData_t *pThreadData)
{
    iettReconcileSubsTreeCbContext_t context;

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ieutTRACEL(pThreadData, tree, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    context.topicInfoCount = 0;
    context.rc = OK;

    ismEngine_getRWLockForRead(&tree->subsLock);

    // Walk the subscription nodes reporting any topics with non-zero activeCluster counts
    iett_reconcileClusterTopics(pThreadData, tree->subs, &context);

    // If there are any topics left in the final batch, report them now.
    if (context.topicInfoCount != 0)
    {
        iett_reconcileClusterReportTopics(pThreadData, &context);
    }

    ismEngine_unlockRWLock(&tree->subsLock);

    ieutTRACEL(pThreadData, context.rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, context.rc);

    return context.rc;
}

//****************************************************************************
/// @brief  Reconcile the engine retained origin server information with the
///         cluster component
///
/// @remark At the end of recovery we will have rebuilt persistent retained
///         messages, noting the server from which they originated we need to
///         clean up recovery and inform the cluster component so that the information
///         can be flowed to other servers in the cluster.
///
/// @param[in]     pThreadData     Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_reconcileEngineRetainedOriginServers(ieutThreadData_t *pThreadData)
{
    iettOriginServerCbContext_t originServerCbContext;

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ieutTRACEL(pThreadData, tree, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    originServerCbContext.rc = OK;

    ismEngine_getRWLockForRead(&tree->topicsLock);

    ieut_traverseHashTable(pThreadData,
                           tree->originServers,
                           iett_reconcileOriginServer,
                           &originServerCbContext);

    ismEngine_unlockRWLock(&tree->topicsLock);

    ieutTRACEL(pThreadData, originServerCbContext.rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, originServerCbContext.rc);

    return originServerCbContext.rc;
}
