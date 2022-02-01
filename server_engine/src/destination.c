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

//*********************************************************************
/// @file  destination.c
/// @brief Management of destinations
//*********************************************************************
#define TRACE_COMP Engine

#include <stdio.h>
#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "destination.h"
#include "topicTree.h"
#include "msgCommon.h"
#include "transaction.h"
#include "queueCommon.h"
#include "clientState.h"
#include "queueNamespace.h"    // ieqn functions & constants
#include "engineStore.h"
#include "memHandler.h"
#include "topicTreeInternal.h"
#include "selector.h"
#include "remoteServers.h"     // iers functions & constants
#include "resourceSetStats.h"  // iere functions

//****************************************************************************
/// @brief Decide whether the rejection of this message by this subscription
///        constitutes a failed publish
///
/// @param[in]     unreliableMsg  Whether the message is unreliable (QoS=0)
/// @param[in]     pSubscription  The subscription to which being published
///
/// @return true if the rejection is a failure, false otherwise.
///
/// @remark What to do when a message is rejected by a subscription queue is
///         dependent on the reliability of the message and the delivery
///         option on the subscription. If either is 'AT_MOST_ONCE', and the
///         subscription is not a shared subscription, we don't need to treat
///         the rejection as a failed publish, otherwise we do.
//****************************************************************************
static inline bool ieds_rejectionMeansFailure(const bool unreliableMsg,
                                              const ismEngine_Subscription_t *pSubscription)
{
    if (unreliableMsg ||
        (pSubscription->subOptions & (ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK |
                                      ismENGINE_SUBSCRIPTION_OPTION_SHARED)) ==
        ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE)
    {
        // ieutTRACEL(ENGINE_HIGH_TRACE, pSubscription->subOptions,
        //            "Ignoring error from put msg %p to %p (msg reliability=%u, sub options=%u, sub maxMessageCount=%lu)\n",
        //            pMessage, pSubscription, pMessage->Header.Reliability,
        //            pSubscription->subOptions, pSubscription->queueHandle->PolicyInfo->maxMessageCount);
        return false;
    }
    else
    {
        return true;
    }
}

//****************************************************************************
/// @brief Allocate a new destination
///
/// @param[in]     resourceSet       ResourceSet the entry should use
/// @param[in]     destinationType   The ismDESTINATION_* type
/// @param[in]     pDestinationName  The 'string' for this destination
/// @param[out]    ppDestination     Address of the allocated pDestination
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark Release the storage using iemem_free(iemem_externalObjs, pDestination)
//****************************************************************************
int32_t ieds_create_newDestination(ieutThreadData_t *pThreadData,
                                   iereResourceSetHandle_t resourceSet,
                                   uint8_t destinationType,
                                   const char *pDestinationName,
                                   ismEngine_Destination_t **ppDestination)
{
    ismEngine_Destination_t *pDestination = NULL;
    int32_t rc = OK;

    assert(destinationType == ismDESTINATION_TOPIC ||
           destinationType == ismDESTINATION_SUBSCRIPTION ||
           destinationType == ismDESTINATION_QUEUE ||
           destinationType == ismDESTINATION_REMOTESERVER_HIGH ||
           destinationType == ismDESTINATION_REMOTESERVER_LOW);
    assert(pDestinationName != NULL);
    assert(*pDestinationName != '\0');
    assert(strlen(pDestinationName) <= ismDESTINATION_NAME_LENGTH);

    pDestination = (ismEngine_Destination_t *)iere_malloc(pThreadData,
                                                          resourceSet,
                                                          IEMEM_PROBE(iemem_externalObjs, 1),
                                                          sizeof(ismEngine_Destination_t) + strlen(pDestinationName) + 1);
    if (pDestination != NULL)
    {
        memcpy(pDestination->StrucId, ismENGINE_DESTINATION_STRUCID, 4);
        pDestination->DestinationType = destinationType;
        pDestination->pDestinationName = (char *) (pDestination + 1);
        strcpy(pDestination->pDestinationName, pDestinationName);
    }
    else
    {
        rc = ISMRC_AllocateError;
    }

    *ppDestination = pDestination;
    return rc;
}

//****************************************************************************
/// @brief Publish a message to the specified topic
///
/// @param[in]     pClient           (optional) The client state of the publisher
/// @param[in]     topicString       The topic on which to publish
/// @param[in]     options           iedsPUBLISH_OPTION_ values
/// @param[in]     pTran             (optional) Transaction under which to publish
/// @param[in]     pMessage          Message to publish
/// @param[in]     unrelDeliveryId   Unreleased delivery ID (may be zero)
/// @param[out]    phUnrel           (optional) Returned unreleased-message handle
/// @param[in]     contextLength     If we go async how much memory do we need for external context
/// @param[out]    pAsyncDataHandle  Details of unfinished store tran requiring commit
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieds_publish(ieutThreadData_t *pThreadData,
                     ismEngine_ClientState_t *pClient,
                     const char *pTopicString,
                     uint32_t options,
                     ismEngine_Transaction_t *pTran,
                     ismEngine_Message_t *pMessage,
                     uint32_t unrelDeliveryId,
                     ismEngine_UnreleasedHandle_t *phUnrel,
                     size_t contextLength,
                     ietrAsyncTransactionDataHandle_t *pAsyncDataHandle)
{
    iettSLEReleaseNodes_t localReleaseNodesSLE = {0};
    iettSLEReleaseNodes_t *pReleaseNodesSLE = &localReleaseNodesSLE;
    iettSubscriberList_t sublist;
    int32_t rc = OK;
    int32_t callerRC = OK; // The return code for the engine caller
    DEBUG_ONLY int32_t rc2;
    bool retainMessage;
    bool persistentMessage;
    bool unreliableMsg;
    bool functionScopeTransaction = false; //Is this function creating a temporary transaction for our use
    size_t storeDataLength = 0;
    bool fAsStoreTran = false;
    bool fromForwarder = (pClient == NULL) ? false : (pClient->protocolId == PROTOCOL_ID_FWD);
    ismEngine_Message_t *pRemoteMsg = pMessage;
    ismEngine_Message_t *pRetainMsg = NULL;
    ietrSavepoint_t *savepoint = NULL;

    // Allow us to determine if this is a publish within a publish...
    pThreadData->publishDepth++;

    retainMessage = ((pMessage->Header.Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN) != 0);
    persistentMessage = (pMessage->Header.Persistence != ismMESSAGE_PERSISTENCE_NONPERSISTENT);
    unreliableMsg = (pMessage->Header.Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE);

    // Sanity check that the output RETAINED flag is not set on input
    assert((pMessage->Header.Flags & ismMESSAGE_FLAGS_RETAINED) == 0);
    // Check that the options related to retained messages are only supplied with retained messages
    assert(((options & (iedsPUBLISH_OPTION_ONLY_UPDATE_RETAINED |
                        iedsPUBLISH_OPTION_REPOSITIONING_RETAINED)) == 0) || (retainMessage == true));

    // If we have an unreleased delivery Id, we must also have a client
    assert((unrelDeliveryId == 0) || (pClient != NULL));

    // We will need 1 store ref if we need to add an unreleased delivery Id
    // cppcheck-suppress *
    uint32_t requiredStoreRefs = ((unrelDeliveryId != 0) && (pClient->Durability == iecsDurable)) ? 1 : 0;

    // Initialize these two counts so that stats will be accurate regardless of what fails...
    sublist.subscriberCount = 0;
    sublist.remoteServerCount = 0;

    // If we are publishing a QoS1 or QoS2 message, or the message is to be retained
    // we need to ensure that we have a transaction and that the message is stored if
    // it is persistent.
    if (!unreliableMsg || retainMessage)
    {
        // For a persistent message, which we are going to try and publish asynchronously, check
        // that the async callback queue can take another entry, waiting if we can afford to
        // do so.
        if (persistentMessage && pAsyncDataHandle != NULL)
        {
            bool failIfAlerted = (options & iedsPUBLISH_OPTION_FAIL_IF_ASYNC_CBQ_ALERTED) != 0;

            rc = iead_checkAsyncCallbackQueue(pThreadData, pTran, failIfAlerted);
        }

        // If we're about to write a retained message, make sure we have a retained topic object
        if (rc == OK && retainMessage)
        {
            rc = iett_ensureEngineStoreTopicRecord(pThreadData);
        }

        // If there is no external transaction, we need to create one now, and otherwise
        // create a savepoint so we can undo anything added to SLEs during this publish.
        if (rc == OK)
        {
            if (pTran == NULL)
            {
                functionScopeTransaction = true;

                if (persistentMessage)
                {
                    fAsStoreTran = true;

                    // Calculate the actual data length we will write to the store which
                    // needs to include the store structures as well as the message data
                    storeDataLength = iest_MessageStoreDataLength(pMessage);
                }

                rc = ietr_createLocal(pThreadData,
                                      NULL,
                                      persistentMessage,
                                      fAsStoreTran,
                                      NULL,
                                      &pTran);
            }
            else
            {
                // Start a savepoint in case we need to tell SLEs we've rolled back
                rc = ietr_beginSavepoint(pThreadData, pTran, &savepoint);
            }
        }
    }

    // Deal with messages required for retained message processing
    if (rc == OK && retainMessage)
    {
        assert(pTran != NULL);

        pRetainMsg = pMessage;

        uint32_t expiryForPublish = pMessage->Header.Expiry;

        // Need to use the real message expiry for messages from the forwarder
        if (fromForwarder == true)
        {
            concat_alloc_t  props;
            ism_field_t field = {0};

            // Get the real expiry from the message properties if there is one
            iem_locateMessageProperties(pMessage, &props);

            ism_common_findPropertyID(&props, ID_RealExpiry, &field);

            if (field.type == VT_UInt)
            {
                expiryForPublish = field.val.u;
            }

            // Retained messages from the forwarder could be being republished - add that option.
            options |= iedsPUBLISH_OPTION_POTENTIAL_REPUBLISH;
        }

        // We want to remember NullRetained messages for some period of time - the amount
        // of time depends on whether this server is a member of the cluster or not.
        if (pMessage->Header.MessageType == MTYPE_NullRetained)
        {
            rc = iem_createMessageCopy(pThreadData,
                                       pMessage,
                                       false,
                                       0, // Already set
                                       expiryForPublish,
                                       &pRemoteMsg);

            if (rc == OK)
            {
                pRetainMsg = pRemoteMsg;

                // If this message has come from a forwarder, we use the existing expiry
                // for the retained message so that it expires at the same time throughout
                // the cluster
                if (fromForwarder == true)
                {
                    assert(pRemoteMsg->Header.Expiry == pMessage->Header.Expiry);
                    assert(pRemoteMsg->Header.Expiry != 0);
                }
                else if (ismEngine_serverGlobal.clusterEnabled == true)
                {
                    pRemoteMsg->Header.Expiry = ism_common_nowExpire()+ismENGINE_CLUSTER_RETAINED_EXPIRY_INTERVAL;
                }
                else
                {
                    // Yes - in the past, we really want to expire it
                    pRemoteMsg->Header.Expiry = ism_common_nowExpire()-1;
                }
            }
            else
            {
                assert(pRemoteMsg == pMessage);
            }
        }
        // If this is a publish of a retained message from a locally connected application
        // we may need to create a copy of the message with a modified expiry to ensure
        // that the message on the forwarding queue doesn't expire.
        else if ((fromForwarder == false) &&
                 (ismEngine_serverGlobal.clusterEnabled == true) &&
                 (pMessage->Header.Expiry != 0))
        {
            uint32_t customExpiryTime = ism_common_nowExpire() + ismENGINE_CLUSTER_RETAINED_EXPIRY_INTERVAL;

            if (pMessage->Header.Expiry < customExpiryTime)
            {
                assert(expiryForPublish == pMessage->Header.Expiry);

                rc = iem_createMessageCopy(pThreadData,
                                           pMessage,
                                           false,
                                           0, // Already set
                                           expiryForPublish,
                                           &pRemoteMsg);

                if (rc == OK)
                {
                    pRemoteMsg->Header.Expiry = customExpiryTime;
                }
                else
                {
                    assert(pRemoteMsg == pMessage);
                }
            }

            assert(pRetainMsg == pMessage);
        }

        // Make sure the message we give to local subscribers has the real expiry
        pMessage->Header.Expiry = expiryForPublish;

        // Increase the store resources we'll need for this retained message
        if (rc == OK && (pTran->TranFlags & ietrTRAN_FLAG_PERSISTENT) == ietrTRAN_FLAG_PERSISTENT)
        {
            if (pMessage != pRemoteMsg)
            {
                storeDataLength += iest_MessageStoreDataLength(pRemoteMsg);
            }
            requiredStoreRefs += fAsStoreTran ? 2 : 3;
        }
    }

    // Go get a list of subscribers for this topic
    if (rc == OK)
    {
        // Don't find any subscribers if we are only performing retained update / reposition.
        if ((options & (iedsPUBLISH_OPTION_ONLY_UPDATE_RETAINED |
                        iedsPUBLISH_OPTION_REPOSITIONING_RETAINED)) != 0)
        {
            sublist.publishSUV = 0;

            assert(sublist.subscriberCount == 0);
            assert(sublist.remoteServerCount == 0);

            rc = ISMRC_NotFound;
        }
        else
        {
            sublist.topicString = pTopicString;

            // Initialise the arrays in the subscriber list from the thread cache.
            rc = iett_initSublistArrays(pThreadData, &sublist);

            // If we did not get the result directly from the cache, call iett_getSubscriberList
            if (rc == ISMRC_NotInThreadCache)
            {
                // Actually get the list of local subscribers
                rc = iett_getSubscriberList(pThreadData, &sublist);

                // Update the cache
                iett_updateCachedArrays(pThreadData, &sublist, rc);
            }

            // Whether we found any local subscribers or not we want to add in any
            // remote servers at this point too.
            if (rc == OK || rc == ISMRC_NotFound)
            {
                assert(pReleaseNodesSLE == &localReleaseNodesSLE);
                if (rc == OK) pReleaseNodesSLE->subscriberNodes = sublist.subscriberNodes;

                // Messages from the forwarder don't go to any remote servers (we
                // don't want loops!) but this is a convenient time to update stats
                // about messages from the forwarder and to specify that
                // we need to select only subscriptions shared in the cluster
                if (fromForwarder == true)
                {
                    if (retainMessage == true)
                    {
                        pThreadData->stats.fromForwarderRetainedMsgCount++;
                    }
                    else
                    {
                        pThreadData->stats.fromForwarderMsgCount++;

                        // If there are no subscribers we can consider this a false positive.
                        // This does not take into consideration the slim possibility that
                        // the only subscribers are not shared in the cluster (but the stat
                        // is intended as an indication only)
                        if (rc == ISMRC_NotFound || sublist.subscriberCount == 0)
                        {
                            pThreadData->stats.fromForwarderNoRecipientMsgCount++;
                        }
                    }

                    sublist.requestSelection = true;
                }
                // This is not from a forwarder, so look for remote servers...
                // But if this is a system topic, no remote servers should be returned - checking
                // this early avoids the overhead of calling the remoteServer code.
                else if (iettTOPIC_IS_SYSTOPIC(pTopicString) == false)
                {
                    int32_t remoteRc = iers_addRemoteServersToSubscriberList(pThreadData, &sublist, pRemoteMsg);

                    // We added some remote servers, so this is now our return code
                    if (remoteRc == OK)
                    {
                        pReleaseNodesSLE->remoteServers = sublist.remoteServers;
                        rc = OK;
                    }
                    // If the remoteRc is ISMRC_NotFound, we keep the existing rc, otherwise
                    // we might need to do some tidy up and we set rc to the remoteRc.
                    else if (remoteRc != ISMRC_NotFound)
                    {
                        // We have a subscriber list to clean up
                        if (rc == OK)
                        {
                            // Don't want to update stats so just call release
                            iett_releaseSubscriberList(pThreadData, &sublist);
                        }

                        // Need anything that happened to be reported.
                        rc = remoteRc;
                    }
                }
            }

            // Need the SLE to release nodes and update statistics at the end of the transaction
            if (rc == OK)
            {
                localReleaseNodesSLE.updateStats = true;

                if (pTran != NULL)
                {
                    size_t subscriberNodesSize = (sublist.subscriberNodeCount+1) * sizeof(iettSubsNodeHandle_t);
                    size_t remoteServersSize = (sublist.remoteServerCount+1) * sizeof(iersRemoteServersHandle_t);
                    ietrSLE_Header_t *pAllocatedSLEHdr;

                    rc = ietr_softLogAdd( pThreadData
                                        , pTran
                                        , ietrSLE_TT_RELEASENODES
                                        , iett_SLEReplayReleaseNodes
                                        , NULL
                                        , Rollback | Cleanup
                                        , NULL
                                        , sizeof(localReleaseNodesSLE) + subscriberNodesSize + remoteServersSize
                                        , 0, 0, &pAllocatedSLEHdr );

                    if (rc == OK)
                    {
                        pReleaseNodesSLE = (iettSLEReleaseNodes_t *)(pAllocatedSLEHdr+1);
                        memcpy(pReleaseNodesSLE, &localReleaseNodesSLE, sizeof(localReleaseNodesSLE));

                        // Copy over subscribers
                        if (subscriberNodesSize != sizeof(iettSubsNodeHandle_t))
                        {
                            pReleaseNodesSLE->subscriberNodes = (iettSubsNodeHandle_t *)(pReleaseNodesSLE+1);
                            memcpy(pReleaseNodesSLE->subscriberNodes, sublist.subscriberNodes, subscriberNodesSize);
                            assert(pReleaseNodesSLE->subscriberNodes[sublist.subscriberNodeCount] == NULL);
                        }
                        else
                        {
                            assert(pReleaseNodesSLE->subscriberNodes == NULL);
                        }

                        // Copy over remote servers
                        if (remoteServersSize != sizeof(iersRemoteServersHandle_t))
                        {
                            pReleaseNodesSLE->remoteServers = (ismEngine_RemoteServer_t **)((char *)(pReleaseNodesSLE+1)+subscriberNodesSize);
                            memcpy(pReleaseNodesSLE->remoteServers, sublist.remoteServers, remoteServersSize);
                            assert(pReleaseNodesSLE->remoteServers[sublist.remoteServerCount] == NULL);
                        }
                        else
                        {
                            assert(pReleaseNodesSLE->remoteServers == NULL);
                        }
                    }
                    else
                    {
                        // Don't want to update stats so just call release
                        iett_releaseSubscriberList(pThreadData, &sublist);
                    }
                }
            }
        }
    }

    // Note: We speculatively get these values, they won't actually be used unless rc == OK.
    const uint32_t subsCount = sublist.subscriberCount;
    const uint32_t remSrvCount = sublist.remoteServerCount;
    const uint32_t totalRecipientCount = subsCount + remSrvCount;

    // Reserve store space for a transaction
    if (pTran != NULL)
    {
        if (rc == OK)
        {
            if (fAsStoreTran)
            {
                requiredStoreRefs += totalRecipientCount;
            }
            else
            {
                requiredStoreRefs += 2 * totalRecipientCount;
            }
        }

        int32_t res_rc = OK;

        // If this publish is transactional, and we have established that
        // we can use a single store transaction then now is the time to
        // reserve space in the store for our transaction
        if (fAsStoreTran)
        {
#ifndef NDEBUG
            uint32_t OpCount;
            res_rc = ism_store_getStreamOpsCount(pThreadData->hStream, &OpCount);
            assert(res_rc == OK);
            assert(OpCount == 0);
#endif
            assert(storeDataLength != 0);

            res_rc = ietr_reserve(pThreadData,
                                  pTran,
                                  storeDataLength,
                                  requiredStoreRefs);
        }
        else if (pTran != NULL && (pTran->TranFlags & ietrTRAN_FLAG_PERSISTENT) == ietrTRAN_FLAG_PERSISTENT)
        {
            //We're doing a transactional publish but the whole engine transaction
            //isn't going to be done as a single transactional. We should be able to do the
            //fan out for the publish as a single store tran though...
            assert(pTran->fStoreTranPublish == false); //No fan out should already be in progress
            pTran->fStoreTranPublish = true;

#ifndef NDEBUG
            uint32_t OpCount;
            res_rc = ism_store_getStreamOpsCount(pThreadData->hStream, &OpCount);
            assert(res_rc == OK);
            assert(OpCount == 0);
#endif

            //For each recipient, we need a reference from queue->msg & one tran->msgref
            res_rc = ietr_reserve(pThreadData,
                                  pTran,
                                  storeDataLength,
                                  requiredStoreRefs);
        }

        if (res_rc != OK) rc = res_rc;
    }

    // Retained message update
    if (retainMessage && (rc == OK || rc == ISMRC_NotFound))
    {
        assert(pRetainMsg != NULL);

        uint32_t retRc = iett_updateRetainedMessage(pThreadData,
                                                    pTopicString,
                                                    pRetainMsg,
                                                    sublist.publishSUV,
                                                    pTran,
                                                    options);

        if (retRc != OK) rc = retRc;
    }

    uint32_t totalMissed = 0;
    uint32_t totalRejected = 0;

    iereResourceSetHandle_t defaultResourceSet = iere_getDefaultResourceSet();
    iereResourceSetHandle_t firstRecipientResourceSet = iereNO_RESOURCE_SET;

    // Publish to all of the subscribers & remote servers
    if (rc == OK)
    {
        uint32_t subsSkipped = 0;

        if (totalRecipientCount != 0)
        {
            uint32_t totalSkipped=0;
            uint32_t subsRejected;

            const ieqPutOptions_t putOptions = ieqPutOptions_THREAD_LOCAL_MESSAGE |
                    ((fromForwarder && !unreliableMsg) ? ieqPutOptions_IGNORE_REJECTNEWMSGS : ieqPutOptions_NONE);
            const ismEngine_Subscription_t *pSubscription;
            const ismEngine_Subscription_t **subscribers = (const ismEngine_Subscription_t **)sublist.subscribers;

            // Increment the usage counts on the messages based on the potential number of recipients.
            // Note: We do this non-atomically because the messages have not been shown to any
            //       of them yet, so cannot be being updated by anyone else.
            pMessage->usageCount += subsCount;
            pRemoteMsg->usageCount += remSrvCount;

            // Put message to any local subscribers
            if (subsCount != 0)
            {
                // What type of selection is required?
                const bool clusterSelection = fromForwarder;
                const bool subscriptionSelection = sublist.requestSelection;
                const bool policySelection = (ismEngine_serverGlobal.policiesWithDefaultSelection != 0);

                // No selection is required, so use a simple loop.
                if (clusterSelection == false &&
                    subscriptionSelection == false &&
                    policySelection == false)
                {
                    while ((pSubscription = *(subscribers++)) != NULL)
                    {
                        int32_t msg_rc = ieq_put(pThreadData,
                                                 pSubscription->queueHandle,
                                                 putOptions,
                                                 pTran,
                                                 pMessage,
                                                 IEQ_MSGTYPE_INHERIT, // no usageCount increment
                                                 NULL);

                        if (msg_rc != OK)
                        {
                            totalRejected++;

                            if ((rc == OK) && (ieds_rejectionMeansFailure(unreliableMsg, pSubscription) == true))
                            {
                                rc = msg_rc;
                            }
                        }
                        else if (firstRecipientResourceSet == iereNO_RESOURCE_SET ||
                                 firstRecipientResourceSet == defaultResourceSet)
                        {
                            firstRecipientResourceSet = pSubscription->resourceSet;
                        }
                    }
                }
                // Some subscriptions are requesting selection, longer loop.
                else
                {
                    const char *clientId = pClient ? pClient->pClientId : NULL;

                    iepiSelectionInfo_t *cachedDefaultSelectionInfo = NULL;
                    bool cacheThisSelectionResult;
                    int32_t cachedSubPolicySelectionResult = SELECT_TRUE;
                    ismEngine_DelivererContext_t delivererContext;
                    delivererContext.lockStrategy.rlac = LS_NO_LOCK_HELD;
                    delivererContext.lockStrategy.lock_persisted_counter = 0;
                    delivererContext.lockStrategy.lock_dropped_counter = 0;

                    while ((pSubscription = *(subscribers++)) != NULL)
                    {
                        ismRule_t *selectionRule = NULL;
                        size_t selectionRuleLen;

                        if (subscriptionSelection == true)
                        {
                            const uint32_t subOptions = pSubscription->subOptions;

                            if ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL) != 0 &&
                                clientId != NULL && strcmp(clientId, pSubscription->clientId) == 0)
                            {
                                totalSkipped++;
                                continue;
                            }

                            if (unreliableMsg)
                            {
                                if ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY) != 0)
                                {
                                    totalSkipped++;
                                    continue;
                                }
                            }
                            else
                            {
                                if ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY) != 0)
                                {
                                    totalSkipped++;
                                    continue;
                                }
                            }

                            // Don't publish to subscriptions in the middle of being imported
                            if ((pSubscription->internalAttrs & iettSUBATTR_IMPORTING) != 0)
                            {
                                totalSkipped++;
                                continue;
                            }

                            selectionRule = pSubscription->selectionRule;
                            selectionRuleLen = (size_t)pSubscription->selectionRuleLen;
                            cacheThisSelectionResult = false;
                        }

                        if (clusterSelection == true)
                        {
                            if ((pSubscription->internalAttrs & iettSUBATTR_SHARE_WITH_CLUSTER) == 0)
                            {
                                totalSkipped++;
                                continue;
                            }
                        }

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
                                        if (cachedSubPolicySelectionResult != SELECT_TRUE)
                                        {
                                            ieutTRACE_HISTORYBUF(pThreadData, selectionRule);
                                            totalSkipped++;
                                            continue;
                                        }

                                        // Don't need to redo selection
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

                        // If there is a selection rule (either from subscription or policy) select on it.
                        if (selectionRule != NULL)
                        {
                            int32_t selResult = ismEngine_serverGlobal.selectionFn( &(pMessage->Header)
                                                                                  , pMessage->AreaCount
                                                                                  , pMessage->AreaTypes
                                                                                  , pMessage->AreaLengths
                                                                                  , pMessage->pAreaData
                                                                                  , pTopicString
                                                                                  , selectionRule
                                                                                  , selectionRuleLen
                                                                                  , &delivererContext.lockStrategy );

                            if (cacheThisSelectionResult == true) cachedSubPolicySelectionResult = selResult;

                            if (selResult != SELECT_TRUE)
                            {
                                ieutTRACE_HISTORYBUF(pThreadData, selectionRule);
                                totalSkipped++;
                                continue;
                            }
                        }

                        int32_t msg_rc = ieq_put(pThreadData,
                                                 pSubscription->queueHandle,
                                                 putOptions,
                                                 pTran,
                                                 pMessage,
                                                 IEQ_MSGTYPE_INHERIT, // no usageCount increment
                                                 &delivererContext );

                        if (msg_rc != OK)
                        {
                            totalRejected++;

                            if ((rc == OK) && (ieds_rejectionMeansFailure(unreliableMsg, pSubscription) == true))
                            {
                                rc = msg_rc;
                            }
                        }
                        else if (firstRecipientResourceSet == iereNO_RESOURCE_SET ||
                                 firstRecipientResourceSet == defaultResourceSet)
                        {
                            firstRecipientResourceSet = pSubscription->resourceSet;
                        }
                    }
                    if ( delivererContext.lockStrategy.rlac == LS_READ_LOCK_HELD || delivererContext.lockStrategy.rlac == LS_WRITE_LOCK_HELD ) {
                        ieutTRACEL(pThreadData, 0, ENGINE_PERFDIAG_TRACE,
                            "RLAC Lock was held and has now been released, debug: %d,%d\n",
                            delivererContext.lockStrategy.lock_persisted_counter,delivererContext.lockStrategy.lock_dropped_counter);
                        ism_common_unlockACLList();
                    } else {
                        ieutTRACEL(pThreadData, 0, ENGINE_PERFDIAG_TRACE,
                            "RLAC Lock was not held, debug: %d,%d\n",
                            delivererContext.lockStrategy.lock_persisted_counter,delivererContext.lockStrategy.lock_dropped_counter);
                    } 

                    delivererContext.lockStrategy.rlac = LS_NO_LOCK_HELD;
                }

                subsSkipped = totalSkipped;
                subsRejected = totalRejected;
            }
            else
            {
                subsSkipped = 0;
                subsRejected = 0;
                assert((sublist.usingCachedArrays == true) || (sublist.subscribers == NULL));
            }

            // Now put the message to any remote servers
            if (remSrvCount != 0)
            {
                assert(fromForwarder == false); // We don't expect the forwarder to deliver to any remote servers
                assert((putOptions & ieqPutOptions_IGNORE_REJECTNEWMSGS) == 0);

                const ismEngine_RemoteServer_t *pRemoteServer;
                const ismEngine_RemoteServer_t **remoteServers = (const ismEngine_RemoteServer_t **)sublist.remoteServers;

                // Unreliable message put to the low QoS queue and ignore any problems
                if (unreliableMsg)
                {
                    while ((pRemoteServer = *(remoteServers++)) != NULL)
                    {
                        int32_t msg_rc = ieq_put(pThreadData,
                                                 pRemoteServer->lowQoSQueueHandle,
                                                 putOptions,
                                                 pTran,
                                                 pRemoteMsg,
                                                 IEQ_MSGTYPE_INHERIT, // no usageCount increment
                                                 NULL );

                        if (msg_rc != OK) totalRejected++;
                    }
                }
                // Reliable message, put to the high QoS queue and remember any problems
                else
                {
                    while ((pRemoteServer = *(remoteServers++)) != NULL)
                    {
                        int32_t msg_rc = ieq_put(pThreadData,
                                                 pRemoteServer->highQoSQueueHandle,
                                                 putOptions,
                                                 pTran,
                                                 pRemoteMsg,
                                                 IEQ_MSGTYPE_INHERIT, // no usageCount increment
                                                 NULL);

                        if (msg_rc != OK)
                        {
                            totalRejected++;

                            if (rc == OK) rc = msg_rc;
                        }
                    }
                }
            }
            else
            {
                assert((totalRejected - subsRejected) == 0);
            }

            totalMissed = totalSkipped+totalRejected;

            // The subscriptions that were skipped or rejected the message should no
            // longer count towards the usage count on the message.
            //
            // NOTE: We rely on the fact that the usageCount will never hit zero, so don't
            //       need to make a call to ism_engine_releaseMessage.
            if (totalMissed > 0)
            {
                // If we have a transaction, the message has not yet been made visible on any
                // queues, no-one will be adjusting the usage count of this message, so we
                // can decrement it non-atomically
                if (pTran != NULL)
                {
                    if (pMessage == pRemoteMsg)
                    {
                        pMessage->usageCount -= totalMissed;
                    }
                    else
                    {
                        pMessage->usageCount -= subsSkipped + subsRejected;
                        pRemoteMsg->usageCount -= (totalRejected-subsRejected);
                    }
                }
                // No transaction, need to decrement the usage count atomically
                else
                {
                    if (pMessage == pRemoteMsg)
                    {
                        __sync_sub_and_fetch(&(pMessage->usageCount), totalMissed);
                    }
                    else
                    {
                        __sync_sub_and_fetch(&(pMessage->usageCount), subsSkipped + subsRejected);
                        __sync_sub_and_fetch(&(pRemoteMsg->usageCount), (totalRejected-subsRejected));
                    }
                }
            }

            ieutTRACEL(pThreadData, totalRecipientCount-totalMissed, ENGINE_HIGH_TRACE,
                       "Published to %d of %d subscribers and %d of %d remote servers, skipped %d rejected %d. (rc=%d, same_message=%d)\n",
                       subsCount-(subsSkipped+subsRejected), subsCount,
                       remSrvCount-(totalRejected-subsRejected), remSrvCount,
                       totalSkipped, totalRejected, rc, (int)(pMessage == pRemoteMsg));
        }

        if (rc == OK)
        {
            pReleaseNodesSLE->publishOK = true;
            pReleaseNodesSLE->publishRejected = (totalRejected != 0);

            if ((options & iedsPUBLISH_OPTION_INFORMATIONAL_RETCODES) != 0)
            {
                if (totalRejected != 0)
                {
                    callerRC = ISMRC_SomeDestinationsFull;
                }
                else if (subsCount == subsSkipped)
                {
                    callerRC = (remSrvCount == 0) ? ISMRC_NoMatchingDestinations : ISMRC_NoMatchingLocalDestinations;
                }
                else
                {
                    assert(callerRC == OK);
                }
            }
            else
            {
                assert(callerRC == OK);
            }
        }
        else
        {
            pReleaseNodesSLE->publishOK = false;
        }

        // Update stats directly if there is no transaction to do so
        if (pTran == NULL)
        {
            iett_SLEReplayReleaseNodes(Cleanup,
                                       pThreadData,
                                       NULL,
                                       pReleaseNodesSLE,
                                       NULL);
        }
        else
        {
            assert((pReleaseNodesSLE->subscriberNodes == NULL) ||
                   (pReleaseNodesSLE->subscriberNodes != sublist.subscriberNodes));
            assert((pReleaseNodesSLE->remoteServers == NULL) ||
                   (pReleaseNodesSLE->remoteServers != sublist.remoteServers));
        }

        // Just free the subscriber list
        iett_freeSubscriberList(pThreadData, &sublist);
    }
    // No local subscribers and remote servers (ISMRC_NotFound) OR a retained message
    // reposition was not required (ISMRC_NotFound) or a retained message was
    // superseded (ISMRC_OldTimestamp).
    else if (rc == ISMRC_NotFound || rc == ISMRC_OldTimestamp)
    {
        assert(totalRecipientCount == 0);
        assert(totalMissed == 0);
        rc = OK;
        callerRC = (options & iedsPUBLISH_OPTION_INFORMATIONAL_RETCODES) == 0 ? OK : ISMRC_NoMatchingDestinations;
    }

    // If we have a different message for remote servers, release it now.
    if (pRemoteMsg != pMessage) iem_releaseMessage(pThreadData, pRemoteMsg);

    // Update resourceSet stats (these are based on the resourceSet of the publisher)
    if (pClient != NULL)
    {
        iereResourceSetHandle_t resourceSet = pClient->resourceSet;

        if (resourceSet != iereNO_RESOURCE_SET)
        {
            iere_primeThreadCache(pThreadData, resourceSet);

            int64_t fullMemSize = pMessage->fullMemSize;

            if (pMessage->Header.Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
            {
                iere_updateInt64Stat(pThreadData, resourceSet,
                                     ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSGS_PUBLISHED, 1);
                iere_updateInt64Stat(pThreadData, resourceSet,
                                     ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSG_BYTES_PUBLISHED, fullMemSize);
            }
            else if (pMessage->Header.Reliability == ismMESSAGE_RELIABILITY_AT_LEAST_ONCE)
            {
                iere_updateInt64Stat(pThreadData, resourceSet,
                                     ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSGS_PUBLISHED, 1);
                iere_updateInt64Stat(pThreadData, resourceSet,
                                     ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSG_BYTES_PUBLISHED, fullMemSize);
            }
            else
            {
                assert(pMessage->Header.Reliability == ismMESSAGE_RELIABILITY_EXACTLY_ONCE);
                iere_updateInt64Stat(pThreadData, resourceSet,
                                     ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSGS_PUBLISHED, 1);
                iere_updateInt64Stat(pThreadData, resourceSet,
                                     ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSG_BYTES_PUBLISHED, fullMemSize);
            }

            iere_updateInt64MaxStat(pThreadData, resourceSet,
                                    ISM_ENGINE_RESOURCESETSTATS_I64_MAX_PUBLISH_RECIPIENTS, (int64_t)(totalRecipientCount-totalMissed));
        }
    }

    // For non-retained msgs, if we gave this message to anyone in a resourceSet, we now
    // need to assign this message to it.
    if (!retainMessage) iere_updateMessageResourceSet(pThreadData, firstRecipientResourceSet, pMessage, false, false);

    // If we've been given an unreleased delivery ID to save as part of the
    // publication transaction, do it now
    if ((rc == OK) && (unrelDeliveryId != 0))
    {
        assert(pClient != NULL);
        rc = iecs_addUnreleasedDelivery(pThreadData, pClient, pTran, unrelDeliveryId);
    }

    if (pTran != NULL)
    {
        bool doRollback = false;

        if (rc == OK)
        {
            if (   (pTran->fAsStoreTran || pTran->fStoreTranPublish)
                && (pTran->StoreRefCount > 0 || retainMessage)
                && (pAsyncDataHandle != NULL))
            {
                //Let's go async!
                bool useMemReservedForCommit = pTran->fAsStoreTran; //in this case there won't be a separate commit later that needs the mem

                ietrAsyncTransactionDataHandle_t asyncTranHandle = ietr_allocateAsyncTransactionData(
                                                                         pThreadData
                                                                       , pTran
                                                                       , useMemReservedForCommit
                                                                       , sizeof(ismEngine_AsyncPut_t) + contextLength);

                if (asyncTranHandle != NULL)
                {
                    if (savepoint != NULL) ietr_endSavepoint(pThreadData, pTran, savepoint, None);

                    ismEngine_AsyncPut_t *putInfo = ietr_getCustomDataPtr(asyncTranHandle);
                    ismEngine_SetStructId(putInfo->StrucId, ismENGINE_ASYNCPUT_STRUCID );

                    putInfo->callerRC = callerRC;
                    putInfo->unrelDeliveryIdHandle = (ismEngine_UnreleasedHandle_t)(uintptr_t)unrelDeliveryId;
                    putInfo->hTran = (ismEngine_TransactionHandle_t)pTran;
                    putInfo->engineLocalTran = functionScopeTransaction;

                    *pAsyncDataHandle = asyncTranHandle;

                    rc = ISMRC_NeedStoreCommit;
                }
                else
                {
                    rc = ISMRC_AllocateError;
                    doRollback = true;
                }
            }
            else
            {
                if (savepoint != NULL) ietr_endSavepoint(pThreadData, pTran, savepoint, None);

                //Not going to go Async
                if (pTran->fStoreTranPublish)
                {
                    assert(functionScopeTransaction == false);

                    if (pTran->StoreRefCount > 0)
                    {
                        //We have ops with the store that we need to write and can't go async
                        iest_store_commit(pThreadData, true);
                    }
                    else
                    {
                        //We just need to get rid of the reservation we made.
                        iest_store_cancelReservation(pThreadData);
                    }
                }
                else if (functionScopeTransaction)
                {
                    rc = ietr_commit( pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL );
                }
            }
        }
        else
        {
            doRollback = true;
        }

        if (doRollback)
        {
            if (functionScopeTransaction)
            {
                assert(savepoint == NULL);
                rc2 = ietr_rollback( pThreadData, pTran, NULL, IETR_ROLLBACK_OPTIONS_NONE );
                assert(rc2 == OK);  // Rollback should never fail
            }
            else
            {
                if (savepoint != NULL) ietr_endSavepoint(pThreadData, pTran, savepoint, SavepointRollback);

                iest_store_rollback(pThreadData, true );

                // We mark the transaction as rollback only or we could rollback the operations we have done so
                // far either for the entire transaction, or just this publish (but that would need savepoint
                // type support. Whatever we do, the pTran itself must remain valid since our caller may be
                // addressing it.

                // At the moment, we mark the transaction rollback only.
                ietr_markRollbackOnly( pThreadData, pTran );
            }
        }

        // We've finished the fan-out... If this was an external transaction of any sort, we
        // need to unset the store tran fanout flag so that it is unset when the transaction is
        // used for a future operation
        if (!functionScopeTransaction)
        {
            pTran->fStoreTranPublish = false;
            assert(pTran->pActiveSavepoint == NULL);
        }
    }

    if (rc == OK || rc == ISMRC_NeedStoreCommit)
    {
        if ((unrelDeliveryId != 0) && (phUnrel != NULL))
        {
            *phUnrel = (ismEngine_UnreleasedHandle_t)(uintptr_t)unrelDeliveryId;
        }

        // If the publish worked, we should pass back the return code for the caller
        if (rc == OK) rc = callerRC;
    }

    pThreadData->publishDepth--;

    return rc;
}

//****************************************************************************
/// @brief Put a message to a named queue
///
/// @param[in]     pClient           (optional) The client state of the publisher
/// @param[in]     pQueueName        The name of the queue on which to put the message
/// @param[in]     pTran             (optional) Transaction under which to put
/// @param[in]     pMessage          Message to put
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieds_putToQueueName(ieutThreadData_t *pThreadData,
                            ismEngine_ClientState_t *pClient,
                            const char *pQueueName,
                            ismEngine_Transaction_t *pTran,
                            ismEngine_Message_t *pMessage)
{
    int32_t rc = OK;

    ismEngine_Producer_t localProducer;

    localProducer.queueHandle = NULL;
    localProducer.engineObject = NULL;

    rc = ieqn_openQueue(pThreadData,
                        pClient,
                        pQueueName,
                        NULL,
                        &localProducer);

    if (rc != OK) goto mod_exit;

    rc = ieds_put(pThreadData,
                  pClient,
                  &localProducer,
                  pTran,
                  pMessage);

    (void)ieqn_unregisterProducer(pThreadData, &localProducer);

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief Put a message to the queue referred to by a producer
///
/// @param[in]     pClient           (optional) The client state of the publisher
/// @param[in]     hQueue            The queue on which to put the message
/// @param[in]     pTran             (optional) Transaction under which to put
/// @param[in]     pMessage          Message to put
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieds_put(ieutThreadData_t *pThreadData,
                 ismEngine_ClientState_t *pClient,
                 ismEngine_Producer_t *pProducer,
                 ismEngine_Transaction_t *pTran,
                 ismEngine_Message_t *pMessage)
{
    int32_t rc;

    assert(pProducer->engineObject != NULL);
    assert(pProducer->queueHandle != NULL);

    // Messages intended for retention on a topic should not be put
    // onto a queue.
    if (pMessage->Header.Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN)
    {
        rc = ISMRC_DestTypeNotValid;
    }
    else
    {
        rc = ieq_put(pThreadData,
                     pProducer->queueHandle,
                     ieqPutOptions_THREAD_LOCAL_MESSAGE,
                     pTran,
                     pMessage,
                     IEQ_MSGTYPE_REFCOUNT,
                     NULL);

        // If this is a QoS0 message, ignore the failure but ensure that this message
        // contributes to the dropped message count.
        if (rc != OK && pMessage->Header.Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
        {
            pThreadData->stats.droppedMsgCount++;
            rc = ISMRC_SomeDestinationsFull;
        }
    }

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of destination.c                                              */
/*                                                                   */
/*********************************************************************/
