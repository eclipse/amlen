/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
/// @file  exportSubscription.c
/// @brief Export / Import functions for subscriptions
//*********************************************************************
#define TRACE_COMP Engine

#include <assert.h>

#include "topicTree.h"
#include "policyInfo.h"
#include "queueCommon.h"          // ieq_ functions
#include "topicTreeInternal.h"
#include "exportSubscription.h"
#include "clientState.h"
#include "exportClientState.h"

// Max subscriptions we will export in a single pass
#define ieieEXPORT_SUBSCRIPTIONS_MAX_EXPORT 1000
// Max subscriptions we will examine before releasing the subsTree lock
#define ieieEXPORT_SUBSCRIPTIONS_MAX_CHECKS 100000

typedef struct tag_ieieSubExport_t
{
    ismEngine_Subscription_t *subscription; ///< Subscription
    ieieDataType_t dataType;                ///< Either ieieDATATYPE_EXPORTEDSUBSCRIPTION or ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB
    bool partiallyShared;                   ///< Whether the subscription is only partially shared by the sharing clients
    uint8_t anonymousSharers;               ///< Anonymous sharers which have registered with this subscription (e.g. JMS)
    char *sharingClientIds;                 ///< Set of exported clients sharing this subscription
    uint32_t sharingClientIdsLength;        ///< Length of the sharing clients array
    uint32_t *sharingClientSubOptions;      ///< Set of subOptions for the exported clients sharing this subscription
    uint32_t sharingClientSubOptionsLength; ///< Length of the subOptions array
    ismEngine_SubId_t *sharingClientSubIds; ///< Set of subIds for the exported clients sharing this subscription
    uint32_t sharingClientSubIdsLength;     ///< Length of the subIds array
    uint32_t sharingClientCount;            ///< Count of the sharing client Ids
} ieieSubExport_t;

typedef enum tag_ieieImportSubscriptionStage_t
{
    ieieISS_Start = 0,
    ieieISS_CreateNew,
    ieieISS_RecordCreation,
    ieieISS_Finish
} ieieImportSubscriptionStage_t;

typedef struct tag_ieieImportSubscriptionCallbackContext_t
{
    ieieImportSubscriptionStage_t stage;
    bool wentAsync;
    bool destroyTried;
    ieieDataType_t dataType;
    uint64_t dataId;
    const char *owningClientId;
    const char *topicString;
    const char *subscriptionName;
    const char *subProperties;
    const char *policyName;
    const char **sharingClientIds;
    uint32_t *sharingClientSubOptions;
    ismEngine_SubId_t *sharingClientSubIds;
    ismEngine_Subscription_t *subscription;
    ieieImportResourceControl_t *control;
    ieieSubscriptionInfo_t info;
} ieieImportSubscriptionCallbackContext_t;

// Forward declaration of async Import function
void ieie_asyncDoImportSubscription(int32_t retcode, void *handle, void *pContext);

//****************************************************************************
/// @internal
///
/// @brief  Export an individual subscription
///
/// @param[in]     exportSub     An ieieSubExport_t describing what to export
/// @param[in]     control       The control structure for the export.
///
/// @remark The usage count on the subscription is expected to be incremented by
/// the caller and this function does not decrement it.
///
/// @remark The messages on the subscription are NOT exported by this function.
/// To export them use the additional function ieie_exportSubscriptionMessages.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
///
/// @see ieie_exportSubscriptionMessages
//****************************************************************************
static inline int32_t ieie_exportSubscription(ieutThreadData_t *pThreadData,
                                              ieieSubExport_t *exportSub,
                                              ieieExportResourceControl_t *control)
{
    int32_t rc = OK;
    ismEngine_Subscription_t *subscription = exportSub->subscription;

    assert(subscription != NULL);
    assert(control != NULL);

    uint64_t dataId = (uint64_t)subscription->queueHandle;
    assert(dataId != 0);
    iepiPolicyInfo_t *policyInfo = ieq_getPolicyInfo(subscription->queueHandle);
    assert(policyInfo != NULL);

    ieutTRACEL(pThreadData, dataId, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "subscription=%p dataId=0x%0lx\n",
               __func__, subscription, dataId);

    assert(subscription->clientId != NULL);
    assert(subscription->node != NULL);
    assert(subscription->node->topicString != NULL);

    // Initialize to zero to ensure unused fields are obvious (and usable in the future)
    ieieSubscriptionInfo_t subInfo = {0};

    // Subscription could contain up to 9 elements if all optional parts are supplied.
    void *data[9];
    uint32_t dataLen[9];

    data[0] = &subInfo;
    dataLen[0] = (uint32_t)sizeof(subInfo);
    subInfo.Version = ieieSUBSCRIPTION_CURRENT_VERSION;
    subInfo.QueueType = ieq_getQType(subscription->queueHandle);
    subInfo.SubOptions = subscription->subOptions;
    subInfo.SubId = subscription->subId;
    subInfo.InternalAttrs = subscription->internalAttrs; // Take ALL internal attrs (and filter on import)
    subInfo.MaxMessageCount = policyInfo->maxMessageCount;
    subInfo.MaxMsgBehavior = (uint8_t)policyInfo->maxMsgBehavior;
    subInfo.DCNEnabled = policyInfo->DCNEnabled;
    subInfo.ClientIdLength = strlen(subscription->clientId)+1;
    data[1] = subscription->clientId;
    dataLen[1] = subInfo.ClientIdLength;
    subInfo.TopicStringLength = strlen(subscription->node->topicString)+1;
    data[2] = subscription->node->topicString;
    dataLen[2] = subInfo.TopicStringLength;
    uint32_t index = 2;

    if (subscription->subName != NULL)
    {
        subInfo.SubNameLength = strlen(subscription->subName) + 1;
        data[++index] = subscription->subName;
        dataLen[index] = subInfo.SubNameLength;
    }
    else
    {
        assert(subInfo.SubNameLength == 0);
    }

    subInfo.SubPropertiesLength = subscription->flatSubPropertiesLength;
    if (subInfo.SubPropertiesLength != 0)
    {
        data[++index] = subscription->flatSubProperties;
        dataLen[index] = subInfo.SubPropertiesLength;
    }

    if (policyInfo->name != NULL)
    {
        subInfo.PolicyNameLength = strlen(policyInfo->name)+1;
        data[++index] = policyInfo->name;
        dataLen[index] = subInfo.PolicyNameLength;
    }
    else
    {
        assert(subInfo.PolicyNameLength == 0);
    }

    if (exportSub->dataType == ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB)
    {
        subInfo.SharingClientCount = exportSub->sharingClientCount;
        subInfo.SharingClientIdsLength = exportSub->sharingClientIdsLength;
        subInfo.SharingClientSubOptionsLength = exportSub->sharingClientSubOptionsLength;
        subInfo.SharingClientSubIdsLength = exportSub->sharingClientSubIdsLength;

        if (subInfo.SharingClientCount != 0)
        {
            assert(subInfo.SharingClientIdsLength != 0);
            assert(exportSub->sharingClientSubOptionsLength/sizeof(uint32_t) == exportSub->sharingClientCount);
            assert(exportSub->sharingClientSubIdsLength/sizeof(uint32_t) == exportSub->sharingClientCount);

            data[++index] = exportSub->sharingClientIds;
            dataLen[index] = exportSub->sharingClientIdsLength;
            data[++index] = exportSub->sharingClientSubOptions;
            dataLen[index] = exportSub->sharingClientSubOptionsLength;
            data[++index] = exportSub->sharingClientSubIds;
            dataLen[index] = exportSub->sharingClientSubIdsLength;
        }

        subInfo.AnonymousSharers = exportSub->anonymousSharers;
    }
    else
    {
        assert(exportSub->sharingClientIds == NULL);
        assert(exportSub->sharingClientIdsLength == 0);
        assert(subInfo.SharingClientIdsLength == 0);
        assert(exportSub->sharingClientSubOptionsLength == 0);
        assert(exportSub->sharingClientSubIdsLength == 0);
        assert(subInfo.SharingClientSubOptionsLength == 0);
        assert(exportSub->sharingClientCount == 0);
        assert(subInfo.SharingClientCount == 0);
        assert(subInfo.AnonymousSharers == iettNO_ANONYMOUS_SHARER);
    }

    assert(index < sizeof(data)/sizeof(data[0]));

    ieieFragmentedExportData_t fragsData = {index+1, data, dataLen};

    rc = ieie_writeExportRecordFrags(pThreadData,
                                     control,
                                     exportSub->dataType,
                                     dataId,
                                     &fragsData);

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Export the messages for a (non-partially shared) subscription
///
/// @param[in]     exportSub     An ieieSubExport_t describing what to export
/// @param[in]     control       The control structure for the export.
///
/// @remark The usage count on the subscription is expected to be incremented by
///         the caller and this function does not decrement it.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
static inline int32_t ieie_exportSubscriptionMessages(ieutThreadData_t *pThreadData,
                                                      ieieSubExport_t *exportSub,
                                                      ieieExportResourceControl_t *control)
{
    int32_t rc = OK;
    ismEngine_Subscription_t *subscription = exportSub->subscription;
    uint64_t dataId = (uint64_t)(subscription->queueHandle);

    ieutTRACEL(pThreadData, dataId, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "subscription=%p dataId=0x%0lx partiallyShared=%d\n",
               __func__, subscription, dataId, (int)exportSub->partiallyShared);

    // If this subscription is being exported in full, we call the queue interface function
    // to export all of its messages.
    //
    // For a partially shared subscription, we will add the message information later (by
    // processing message delivery info for the clients being exported).
    if (exportSub->partiallyShared == false)
    {
        rc = ieq_exportMessages(pThreadData, subscription->queueHandle, control);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Export the information for subscriptions identified as owned by
///         or in use by the clientIds in the specified control client Id table.
///
/// @param[in]     control        ieieExportResourceControl_t for this export
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_exportSubscriptions(ieutThreadData_t *pThreadData,
                                 ieieExportResourceControl_t *control)
{
    int32_t rc = OK;

    assert(control != NULL);
    assert(control->file != NULL);
    assert(control->clientId != NULL);

    ieutTRACEL(pThreadData, control->clientId, ENGINE_FNC_TRACE, FUNCTION_ENTRY "clientId='%s' outFile=%p\n", __func__,
               control->clientId, control->file);

    ieieSubExport_t exportSub[ieieEXPORT_SUBSCRIPTIONS_MAX_EXPORT] = {{0}};
    ismEngine_Subscription_t *heldSubscription = NULL;

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    uint32_t totalSubsFound = 0;

    // Lock the topic tree for read access
    ismEngine_getRWLockForRead(&tree->subsLock);

    // Start with the first subscription
    ismEngine_Subscription_t *curSubscription = tree->subscriptionHead;

    // There are no subscriptions unlock the tree and return
    if (curSubscription == NULL)
    {
        ismEngine_unlockRWLock(&tree->subsLock);
        goto mod_exit;
    }

    // There are subscriptions, go through them
    uint32_t subsCheckedSinceLastUnlock = 0;
    uint32_t subsFound = 0;
    while(curSubscription != NULL)
    {
        // Include only subscriptions not flagged as logically deleted
        if ((curSubscription->internalAttrs & iettSUBATTR_DELETED) == 0)
        {
            ieieSubExport_t *thisExportSub = &exportSub[subsFound];

            void *clientDataId; // This is the data Id of the client that matches

            assert(curSubscription->clientId != NULL);
            assert(curSubscription->queueHandle != NULL);
            assert(curSubscription->queueHandle->PolicyInfo != NULL);

            // Globally shared subscriptions will have an internal attribute set
            if ((curSubscription->internalAttrs & iettSUBATTR_GLOBALLY_SHARED) == iettSUBATTR_GLOBALLY_SHARED)
            {
                iettSharedSubData_t *sharedSubData = iett_getSharedSubData(curSubscription);

                assert(sharedSubData != NULL);

                DEBUG_ONLY int osrc = pthread_spin_lock(&sharedSubData->lock);
                assert(osrc == 0);

                // Initialize the entry
                memset(thisExportSub, 0, sizeof(*thisExportSub));

                thisExportSub->anonymousSharers = sharedSubData->anonymousSharers;

                for(uint32_t index = 0; index<sharedSubData->sharingClientCount; index++)
                {
                    // See if this clientId is one of the ones we're going to be exporting
                    if (ieut_getHashEntry(control->clientIdTable,
                                          sharedSubData->sharingClients[index].clientId,
                                          sharedSubData->sharingClients[index].clientIdHash,
                                          &clientDataId) == OK)
                    {
                        uint32_t newClientIdSize = (uint32_t)(strlen(sharedSubData->sharingClients[index].clientId) + 1);

                        // Allocate buffer space for the client Id string we need to add
                        uint32_t newBufferSize = thisExportSub->sharingClientIdsLength + newClientIdSize;

                        char *newClientIdBuffer = iemem_realloc(pThreadData,
                                                                IEMEM_PROBE(iemem_exportResources, 11),
                                                                thisExportSub->sharingClientIds,
                                                                newBufferSize);
                        if (newClientIdBuffer == NULL)
                        {
                            rc = ISMRC_AllocateError;
                            ism_common_setError(rc);
                            break;
                        }

                        memcpy(&newClientIdBuffer[thisExportSub->sharingClientIdsLength],
                               sharedSubData->sharingClients[index].clientId,
                               newClientIdSize);

                        thisExportSub->sharingClientIds = newClientIdBuffer;
                        thisExportSub->sharingClientIdsLength = newBufferSize;

                        // Allocate buffer space for the client SubOptions we need to add
                        newBufferSize = thisExportSub->sharingClientSubOptionsLength + sizeof(sharedSubData->sharingClients[index].subOptions);

                        uint32_t *newSubOptionsBuffer = iemem_realloc(pThreadData,
                                                                      IEMEM_PROBE(iemem_exportResources, 12),
                                                                      thisExportSub->sharingClientSubOptions,
                                                                      newBufferSize);

                        if (newSubOptionsBuffer == NULL)
                        {
                            rc = ISMRC_AllocateError;
                            ism_common_setError(rc);
                            break;
                        }

                        newSubOptionsBuffer[thisExportSub->sharingClientCount] = sharedSubData->sharingClients[index].subOptions;

                        thisExportSub->sharingClientSubOptions = newSubOptionsBuffer;
                        thisExportSub->sharingClientSubOptionsLength = newBufferSize;

                        // Allocate buffer space for the client SubIds we need to add
                        newBufferSize = thisExportSub->sharingClientSubIdsLength + sizeof(ismEngine_SubId_t);

                        ismEngine_SubId_t *newSubIdsBuffer = iemem_realloc(pThreadData,
                                                                           IEMEM_PROBE(iemem_exportResources, 26),
                                                                           thisExportSub->sharingClientSubIds,
                                                                           newBufferSize);

                        if (newSubIdsBuffer == NULL)
                        {
                            rc = ISMRC_AllocateError;
                            ism_common_setError(rc);
                            break;
                        }

                        newSubIdsBuffer[thisExportSub->sharingClientCount] = sharedSubData->sharingClients[index].subId;

                        thisExportSub->sharingClientSubIds = newSubIdsBuffer;
                        thisExportSub->sharingClientSubIdsLength = newBufferSize;

                        thisExportSub->sharingClientCount += 1;
                    }
                }

                if (rc == OK)
                {
                    bool exportThis;

                    thisExportSub->partiallyShared = false;

                    // The caller explicitly wants internal clients (which could include the shared sub
                    // namespace clientIds) so we ought to include this subscription fully if it did.
                    if ((control->options & ismENGINE_EXPORT_RESOURCES_OPTION_INCLUDE_INTERNAL_CLIENTIDS) != 0)
                    {
                        exportThis = (ieut_getHashEntry(control->clientIdTable,
                                                        curSubscription->clientId,
                                                        curSubscription->clientIdHash,
                                                        &clientDataId) == OK);
                    }
                    else
                    {
                        exportThis = false;
                    }

                    // Not already decided to export this one, so check if we should...
                    if (exportThis == false)
                    {
                        // If one of the clients we're exporting is in the sharing set, we need to export the
                        // subscription - but only fully if ALL sharing clients are being exported.
                        if (thisExportSub->sharingClientCount != 0)
                        {
                            exportThis = true;

                            if (thisExportSub->sharingClientCount != sharedSubData->sharingClientCount)
                            {
                                thisExportSub->partiallyShared = true;
                            }
                        }
                        // If no clients match, but it's anonymously shared we check if the topic matches
                        // the specified topic regular expression, if it does, we also export it fully.
                        else if ((thisExportSub->anonymousSharers != iettNO_ANONYMOUS_SHARER) &&
                                 (control->regexTopic != NULL))
                        {
                            exportThis = (ism_regex_match(control->regexTopic, curSubscription->node->topicString) == 0);
                        }
                    }

                    // We need to export this shared subscription
                    if (exportThis == true)
                    {
                        iett_acquireSubscriptionReference(curSubscription);

                        thisExportSub->dataType = ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB;
                        thisExportSub->subscription = curSubscription;

                        subsFound += 1;
                    }
                    else
                    {
                        iemem_free(pThreadData, iemem_exportResources, thisExportSub->sharingClientIds);
                        iemem_free(pThreadData, iemem_exportResources, thisExportSub->sharingClientSubOptions);
                    }
                }

                pthread_spin_unlock(&sharedSubData->lock);
            }
            // Not globally shared, can just check the owner matches one of our clients
            else
            {
                rc = ieut_getHashEntry(control->clientIdTable,
                                       curSubscription->clientId,
                                       curSubscription->clientIdHash,
                                       &clientDataId);

                if (rc == OK)
                {
                    iett_acquireSubscriptionReference(curSubscription);

                    // Initialize the entry
                    memset(thisExportSub, 0, sizeof(*thisExportSub));

                    thisExportSub->dataType = ieieDATATYPE_EXPORTEDSUBSCRIPTION;
                    thisExportSub->subscription = curSubscription;

                    subsFound += 1;
                }
                else
                {
                    assert(rc == ISMRC_NotFound);
                    rc = OK;
                }
            }

            subsCheckedSinceLastUnlock++;
        }

        ismEngine_Subscription_t *nextSubscription = curSubscription->next;

        // If we have found the maximum, or checked enough or we're at the end of
        // the list, we need to deal with the subscriptions we found so far.
        if ((subsFound == ieieEXPORT_SUBSCRIPTIONS_MAX_EXPORT) ||
            (subsCheckedSinceLastUnlock == ieieEXPORT_SUBSCRIPTIONS_MAX_CHECKS) ||
            (nextSubscription == NULL))
        {
            if (nextSubscription != NULL)
            {
                iett_acquireSubscriptionReference(nextSubscription);
            }

            // Unlock the topic tree
            ismEngine_unlockRWLock(&tree->subsLock);

            // We previously held a subscription, so now release it
            if (heldSubscription != NULL)
            {
                (void)iett_releaseSubscription(pThreadData, heldSubscription, false);
            }

            // If we have a nextSubscription it is now the held one, else held should be NULL
            heldSubscription = nextSubscription;

            assert(rc == OK);

            for(uint32_t i=0; i<subsFound; i++)
            {
                if (rc == OK)
                {
                    rc = ieie_exportSubscription(pThreadData, &exportSub[i], control);
                }

                // Free up the list of sharing clients if there is one.
                if (exportSub[i].sharingClientIds != NULL)
                {
                    assert(exportSub[i].sharingClientIdsLength != 0);
                    iemem_free(pThreadData, iemem_exportResources, exportSub[i].sharingClientIds);
                }

                // Free up the list of sharing client subOptions if there is one
                if (exportSub[i].sharingClientSubOptions != NULL)
                {
                    assert(exportSub[i].sharingClientSubOptionsLength != 0);
                    iemem_free(pThreadData, iemem_exportResources, exportSub[i].sharingClientSubOptions);
                }

                // Free up the list of sharing client subIds if there is one
                if (exportSub[i].sharingClientSubIds != NULL)
                {
                    assert(exportSub[i].sharingClientSubIdsLength != 0);
                    iemem_free(pThreadData, iemem_exportResources, exportSub[i].sharingClientSubIds);
                }
            }

            for(uint32_t i=0; i<subsFound; i++)
            {
                if (rc == OK)
                {
                    rc = ieie_exportSubscriptionMessages(pThreadData, &exportSub[i], control);
                }

                // Always need to release the use count on this subscription
                iett_releaseSubscription(pThreadData, exportSub[i].subscription, false);
            }

            totalSubsFound += subsFound;
            subsFound = 0;
            subsCheckedSinceLastUnlock = 0;

            // We have more subscriptions to analyse, so get the lock back
            if (nextSubscription != NULL)
            {
                if (rc == OK)
                {
                    ismEngine_getRWLockForRead(&tree->subsLock);
                }
                else
                {
                    assert(heldSubscription == nextSubscription);
                    (void)iett_releaseSubscription(pThreadData, heldSubscription, false);
                    nextSubscription = NULL;
                }
            }
        }

        curSubscription = nextSubscription;
    }

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d totalSubsFound=%u\n", __func__, rc, totalSubsFound);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Release Subscriptions that were added during import
///
/// @param[in]     key            DataId
/// @param[in]     keyHash        Hash of the key value
/// @param[in]     value          pClient imported
/// @param[in]     context        Context to keep track of work done
///
/// @remark Note if the value is NULL it means that this a subscription
/// which we chose not to import - in this case there is no work to do.
//****************************************************************************
int32_t ieie_releaseImportedSubscription(ieutThreadData_t *pThreadData,
                                         char *key,
                                         uint32_t keyHash,
                                         void *value,
                                         void *context)
{
    ismEngine_Subscription_t *subscription = (ismEngine_Subscription_t *)value;
    int32_t rc = OK;

    if (subscription != NULL)
    {
        ieieReleaseImportedSubContext_t *pContext = (ieieReleaseImportedSubContext_t *)context;

        assert(subscription->node != NULL);
        assert((subscription->internalAttrs & iettSUBATTR_IMPORTING) != 0);

        // Update the topic node & subscription to remove the importing flag
        ismEngine_getRWLockForWrite(&pContext->tree->subsLock);
        subscription->internalAttrs &= ~iettSUBATTR_IMPORTING;
        subscription->node->activeSelection -= 1;
        ismEngine_unlockRWLock(&pContext->tree->subsLock);

        rc = ieq_completeImport(pThreadData, subscription->queueHandle);

        iett_releaseSubscription(pThreadData, subscription, false);
        pContext->releasedCount += 1;
    }

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Find a queue handle for a given dataId
///
/// @param[in]     control              Import control structure
/// @param[in]     dataId               Identifier given to the message on export
/// @param[out]    pQHandle             Queue handle found
///
/// @remark It is possible for the function to return OK but for the queue handle
/// to be NULL. This means that, while the specified dataId did represent a queue
/// of some type (either subscription or named queue) the object was not imported,
/// for example because it was nondurable.
///
/// @remark The use count on the queue is *not* incremented on return.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_findImportedQueueHandle(ieutThreadData_t *pThreadData,
                                     ieieImportResourceControl_t *control,
                                     uint64_t dataId,
                                     ismQHandle_t *pQHandle)
{
    ismQHandle_t queueHandle = NULL;
    uint32_t dataIdHash = (uint32_t)(dataId>>4);

    ismEngine_Subscription_t *subscription = NULL;

    ismEngine_getRWLockForRead(&control->importedTablesLock);
    int32_t rc = ieut_getHashEntry(control->importedSubscriptions,
                                   (const char *)dataId,
                                   dataIdHash,
                                   (void **)&subscription);
    ismEngine_unlockRWLock(&control->importedTablesLock);

    if (rc == OK)
    {
        if (subscription != NULL)
        {
            assert(subscription->node != NULL);
            queueHandle = subscription->queueHandle;
        }

        *pQHandle = queueHandle;
    }

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "dataId=0x%0lx queueHandle=%p rc=%d\n",
               __func__, dataId, queueHandle, rc);

    return rc;
}

//****************************************************************************
/// @brief Get a policy info structure using name or creating a unique one
///
/// @param[in]     policyName        The name for this policy (NULL means unique)
/// @param[in]     policyType        Policy type expected (for named policy)
/// @param[in]     creationTemplate  Template to use when creation is required
/// @param[out]    ppPolicyInfo      Pointer to receive the policyInfo found or
///                                  created.
///
/// @remark Upon successful completion, the policyInfo returned will have its
/// use count incremented to release the useCount use iepi_releasePolicyInfo
///
/// @return OK on successful completion, ISMRC_AsyncCompletion if the operation
/// needed to go async or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_getPolicyInfo(ieutThreadData_t *pThreadData,
                           const char *policyName,
                           ismSecurityPolicyType_t policyType,
                           iepiPolicyInfo_t *creationTemplate,
                           iepiPolicyInfo_t **ppPolicyInfo)
{
    int32_t rc = OK;

    iepiPolicyInfo_t *policyInfo = NULL;

    // Get the policy to use
    if (policyName != NULL)
    {
        rc = iepi_getEngineKnownPolicyInfo(pThreadData,
                                           policyName,
                                           policyType,
                                           &policyInfo);
    }
    else
    {
        // This is going to be an unnamed, unique policy - so override the type
        policyType = ismSEC_POLICY_LAST;
    }

    // Didn't find (or didn't have) a named policy - so now we need to add one.
    if (policyInfo == NULL)
    {
        rc = iepi_createPolicyInfo(pThreadData,
                                   policyName,
                                   policyType,
                                   false,
                                   creationTemplate,
                                   &policyInfo);

        // Add it to the known policies if this is a named one
        if (rc == OK)
        {
            assert(policyInfo->policyType == policyType);

            if (policyType != ismSEC_POLICY_LAST)
            {
                rc = iepi_addEngineKnownPolicyInfo(pThreadData,
                                                   policyName,
                                                   policyType,
                                                   policyInfo);

                // Someone else got in ahead of us - let's share...
                if (rc == ISMRC_ExistingKey)
                {
                    iepiPolicyInfo_t *ourPolicyInfo = policyInfo;

                    rc = iepi_getEngineKnownPolicyInfo(pThreadData,
                                                       policyName,
                                                       policyType,
                                                       &policyInfo);

                    if (rc == OK)
                    {
                        assert(policyInfo != ourPolicyInfo);
                        iepi_freePolicyInfo(pThreadData, ourPolicyInfo, false);
                    }
                }
                // We just gave our useCount to the known policy table - but we need it for
                // the import
                else
                {
                    assert(rc == OK);
                    iepi_acquirePolicyInfoReference(policyInfo);
                }
            }
        }
    }

    if (rc == OK) *ppPolicyInfo = policyInfo;

    return rc;
}

//****************************************************************************
/// @brief Perform all of the actions required to import a subscription
/// handling async completion in any one of them.
///
/// @param[in]     rc             Return code of the previous stage
/// @param[in]     handle         Handle returned by previous stage
/// @param[in]     context        The callback context for this subscription
///
/// @return OK on successful completion, ISMRC_AsyncCompletion if the operation
/// needed to go async or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_doImportSubscription(ieutThreadData_t *pThreadData,
                                  int32_t rc,
                                  void *handle,
                                  ieieImportSubscriptionCallbackContext_t *context)
{
    ieieImportSubscriptionCallbackContext_t *pContext = context;
    ieieImportSubscriptionCallbackContext_t **ppContext = &pContext;
    ieieImportResourceControl_t *control = context->control;
    ieieDataType_t dataType = context->dataType;
    uint64_t dataId = context->dataId;
    size_t humanIdentifierLen = strlen(context->owningClientId) + strlen("ClientID:") +
                                   (context->subscriptionName == NULL ? 0 : strlen(context->subscriptionName) + strlen("Subscription:")) + 2;

    ieutTRACEL(pThreadData, dataId, ENGINE_FNC_TRACE, FUNCTION_ENTRY "dataType=%d dataId=0x%0lx, owningClientId=%s, subName=%s\n",
               __func__, dataType, dataId, context->owningClientId, context->subscriptionName ? context->subscriptionName : "<NULL>");

    while(rc != ISMRC_AsyncCompletion)
    {
        context->stage += 1;

        ieutTRACEL(pThreadData, context->stage, ENGINE_HIFREQ_FNC_TRACE, "dataId=0x%0lx, destroyTried=%d, stage=%u\n",
                   dataId, (int)context->destroyTried, (uint32_t)context->stage);

        switch(context->stage)
        {
            case ieieISS_Start:
                // Don't ever expect value to be Start at this stage...
                assert(false);
                break;
            case ieieISS_CreateNew:
                if (rc == OK)
                {
                    // At the moment we don't support the import of non-persistent subscriptions (the export
                    // files we produce shouldn't include them because the client set should have been
                    // disconnected at import)
                    if ((context->info.InternalAttrs & iettSUBATTR_PERSISTENT) == 0)
                    {
                        char humanIdentifier[humanIdentifierLen];

                        sprintf(humanIdentifier, "ClientID:%s", context->owningClientId);
                        if (context->subscriptionName != NULL)
                        {
                            strcat(humanIdentifier, ",Subscription:");
                            strcat(humanIdentifier, context->subscriptionName);
                        }

                        // This return code will be reported, but the import should continue
                        ieie_recordImportError(pThreadData,
                                               control,
                                               dataType,
                                               context->dataId,
                                               humanIdentifier,
                                               ISMRC_NonDurableImport);

                        assert(context->subscription == NULL);
                    }
                    else
                    {
                        assert(context->subscriptionName != NULL);

                        ismSecurityPolicyType_t policyType;
                        iettCreateSubscriptionClientInfo_t clientInfo;
                        iettCreateSubscriptionInfo_t createSubInfo;
                        iepiPolicyInfo_t template;

                        // Only a subset of the template fields are used if creation is required,
                        // and so only that subset (listed in the comment for iepi_createPolicyInfo)
                        // are updated here.
                        template.maxMessageCount = context->info.MaxMessageCount;
                        template.DCNEnabled = context->info.DCNEnabled;
                        template.maxMsgBehavior = (iepiMaxMsgBehavior_t)(context->info.MaxMsgBehavior);

                        if (dataType == ieieDATATYPE_EXPORTEDSUBSCRIPTION)
                        {
                            assert((context->info.InternalAttrs & iettSUBATTR_GLOBALLY_SHARED) == 0);
                            policyType = ismSEC_POLICY_TOPIC;

                            // Get the owning clientState
                            rc = ieie_findImportedClientStateByClientId(pThreadData,
                                                                        control,
                                                                        context->owningClientId,
                                                                        &clientInfo.owningClient);

                            if (rc != OK)
                            {
                                ism_common_setError(rc);
                                break;
                            }

                            assert(clientInfo.owningClient->OpState == iecsOpStateZombie);
                            assert(context->info.AnonymousSharers == iettNO_ANONYMOUS_SHARER);
                            assert(context->info.SharingClientCount == 0);

                            // One of our importing client states, so we don't want to release it.
                            clientInfo.releaseClientState = false;
                        }
                        else
                        {
                            assert((context->info.InternalAttrs & iettSUBATTR_GLOBALLY_SHARED) != 0);
                            assert(dataType == ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB);
                            policyType = ismSEC_POLICY_SUBSCRIPTION;

                            ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);
                            clientInfo.owningClient = iecs_getVictimizedClient(pThreadData,
                                                                               context->owningClientId,
                                                                               iecs_generateClientIdHash(context->owningClientId));
                            ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);

                            if (clientInfo.owningClient == NULL)
                            {
                                rc = ISMRC_BadClientID;
                                ism_common_setError(rc);
                                break;
                            }

                            // Not one of our importing clients, so we need to release it.
                            clientInfo.releaseClientState = true;
                        }

                        rc = ieie_getPolicyInfo(pThreadData,
                                                context->policyName,
                                                policyType,
                                                &template,
                                                &createSubInfo.policyInfo);

                        if (rc != OK)
                        {
                            ism_common_setError(rc);
                            break;
                        }

                        assert(createSubInfo.policyInfo != NULL);

                        if (clientInfo.releaseClientState == true)
                        {
                            iecs_acquireClientStateReference(clientInfo.owningClient);
                        }

                        clientInfo.requestingClient = clientInfo.owningClient;

                        // Pass on the list of sharers (if any)
                        createSubInfo.anonymousSharer = context->info.AnonymousSharers;
                        createSubInfo.sharingClientCount = context->info.SharingClientCount;
                        createSubInfo.sharingClientIds = context->sharingClientIds;
                        createSubInfo.sharingClientSubOpts = context->sharingClientSubOptions;
                        createSubInfo.sharingClientSubIds = context->sharingClientSubIds;

                        createSubInfo.subProps.flat = context->subProperties;
                        createSubInfo.flatLength = (size_t)(context->info.SubPropertiesLength);
                        createSubInfo.subName = context->subscriptionName;
                        createSubInfo.topicString = context->topicString;
                        createSubInfo.subId = context->info.SubId;

                        // Ensure we don't give the subscription retained messages - they would have
                        // been added at original subscription creation time.
                        createSubInfo.subOptions = context->info.SubOptions
                                                  | ismENGINE_SUBSCRIPTION_OPTION_NO_RETAINED_MSGS;


                        // We treat this imported subscription like one that was rehydrated - the IMPORTING flag
                        // is temporary (while the import is taking place) but the REHYDRATED one will stay on.
                        createSubInfo.internalAttrs = iettSUBATTR_IMPORTING | iettSUBATTR_REHYDRATED |
                                                      (context->info.InternalAttrs & iettSUBATTR_IMPORT_MASK);

                        ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
                                {ismENGINE_ASYNCDATAENTRY_STRUCID, TopicCreateSubscriptionClientInfo,
                                    &clientInfo, sizeof(clientInfo), NULL,
                                    {.internalFn = iett_asyncCreateSubscriptionReleaseClients}},
                                {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineCaller,
                                    ppContext, sizeof(ppContext), NULL,
                                    {.externalFn = ieie_asyncDoImportSubscription }}};

                        ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID,
                                                           clientInfo.owningClient,
                                                           IEAD_MAXARRAYENTRIES, 2, 0, true,  0, 0, asyncArray};

                        rc = iett_createSubscription(pThreadData,
                                                     &clientInfo,
                                                     &createSubInfo,
                                                     &context->subscription,
                                                     &asyncData);

                        if (rc != ISMRC_AsyncCompletion)
                        {
                            iett_createSubscriptionReleaseClients(pThreadData, &clientInfo);
                        }

                        iepi_releasePolicyInfo(pThreadData, createSubInfo.policyInfo);

                        // If there is a globally shared subscription that matches the one we're trying to import,
                        // we will make one attempt to destroy it and retry.
                        if (rc == ISMRC_ExistingSubscription && dataType == ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB)
                        {
                            if (context->destroyTried == false)
                            {
                                context->destroyTried = true;
                                context->stage = ieieISS_Start;

                                rc = iett_destroySubscriptionForClientId(pThreadData,
                                                                         context->owningClientId,
                                                                         NULL, // Making this an admin request
                                                                         context->subscriptionName,
                                                                         NULL,
                                                                         iettSUB_DESTROY_OPTION_NONE,
                                                                         ppContext,
                                                                         sizeof(ppContext),
                                                                         ieie_asyncDoImportSubscription);

                                // Okay - it got destroyed now! Continue with the retry...
                                if (rc == ISMRC_NotFound) rc = OK;
                            }
                        }
                    }
                }
                break;
            case ieieISS_RecordCreation:
                if (rc == OK)
                {
                    assert(handle == NULL || handle == context->subscription);

                    uint32_t dataIdHash = (uint32_t)(context->dataId>>4);

                    ismEngine_getRWLockForWrite(&control->importedTablesLock);
                    rc = ieut_putHashEntry(pThreadData,
                                           control->importedSubscriptions,
                                           ieutFLAG_NUMERIC_KEY,
                                           (const char *)context->dataId,
                                           dataIdHash,
                                           context->subscription,
                                           0);
                    ismEngine_unlockRWLock(&control->importedTablesLock);

                    // Failed to add it to the table - so release it now.
                    if (rc != OK && context->subscription != NULL)
                    {
                        ieieReleaseImportedSubContext_t releaseContext = {0, ismEngine_serverGlobal.maintree};

                        ieie_releaseImportedSubscription(pThreadData,
                                                         (char *)context->dataId,
                                                         dataIdHash,
                                                         context->subscription,
                                                         &releaseContext);

                        // NOTE: We don't update the tree if we released this one - it will happen later.
                    }
                }
                break;
            case ieieISS_Finish:
                {
                    bool wentAsync = context->wentAsync;

                    // We failed - Report this.
                    if (rc != OK)
                    {
                        assert(rc != ISMRC_AsyncCompletion);

                        char humanIdentifier[humanIdentifierLen];

                        sprintf(humanIdentifier, "ClientID:%s", context->owningClientId);
                        if (context->subscriptionName != NULL)
                        {
                            strcat(humanIdentifier, ",Subscription:");
                            strcat(humanIdentifier, context->subscriptionName);
                        }

                        ieie_recordImportError(pThreadData,
                                               control,
                                               dataType,
                                               context->dataId,
                                               humanIdentifier,
                                               rc);
                    }

                    iemem_free(pThreadData, iemem_exportResources, context->sharingClientIds);
                    iemem_free(pThreadData, iemem_exportResources, context->sharingClientSubOptions);
                    iemem_free(pThreadData, iemem_exportResources, context->sharingClientSubIds);
                    iemem_free(pThreadData, iemem_exportResources, context);

                    if (wentAsync)
                    {
                        ieie_finishImportRecord(pThreadData, control, dataType);
                        (void)ieie_importTaskFinish(pThreadData, control, true, NULL);
                    }
                }
                goto mod_exit;
        }
    }

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "dataId=0x%0lx, rc=%d\n",
               __func__, dataId, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Async callback used to re-invoke ieie_doImportSubscription
///
/// @param[in]     retcode        rc of the previous async operation
/// @param[in]     handle         handle from the previous async operation
/// @param[in]     pContext       ieieImportClientStateCallbackContext_t
//****************************************************************************
void ieie_asyncDoImportSubscription(int32_t retcode,
                                    void *handle,
                                    void *pContext)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ieieImportSubscriptionCallbackContext_t *context = *(ieieImportSubscriptionCallbackContext_t **)pContext;

    context->wentAsync = true;

    // Remember the handle for our subscription if it got created
    if (context->stage == ieieISS_CreateNew)
    {
        if (retcode == OK)
        {
            assert(handle != NULL);
            context->subscription = (ismEngine_Subscription_t *)handle;
        }
        else
        {
            assert(context->subscription == NULL);
        }
    }

    (void)ieie_doImportSubscription(pThreadData, retcode, handle, context);

    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @internal
///
/// @brief  Import a subscription
///
/// @param[in]     control        ieieImportResourceControl_t for this import
/// @param[in]     dataType       Which type of subscription this is
/// @param[in]     dataId         dataId (the dataId for this subscription)
/// @param[in]     data           ieieSubscriptionInfo_t
/// @param[in]     dataLen        sizeof(ieieSubscriptionInfo_t plus data)
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_importSubscription(ieutThreadData_t *pThreadData,
                                ieieImportResourceControl_t *control,
                                ieieDataType_t dataType,
                                uint64_t dataId,
                                uint8_t *data,
                                size_t dataLen)
{
    int32_t rc = OK;
    ieieSubscriptionInfo_t *SI = (ieieSubscriptionInfo_t *)data;
    size_t extraDataLen;
    size_t extraDataAllocLen;
    uint64_t defaultSubIdsLen;

    assert(dataType == ieieDATATYPE_EXPORTEDSUBSCRIPTION || dataType == ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB);

    ieutTRACEL(pThreadData, dataId, ENGINE_FNC_TRACE, FUNCTION_ENTRY "dataId=0x%0lx\n", __func__, dataId);

    // Cope with different versions of subscription
    if (SI->Version == ieieSUBSCRIPTION_CURRENT_VERSION)
    {
        defaultSubIdsLen = 0;
        extraDataAllocLen = extraDataLen = dataLen-sizeof(ieieSubscriptionInfo_t);
    }
    else
    {
        assert(SI->Version == ieieSUBSCRIPTION_VERSION_1);
        extraDataLen = dataLen-sizeof(ieieSubscriptionInfo_V1_t);

        // Allow extra space for a SubId array initialized to 0
        defaultSubIdsLen = (((ieieSubscriptionInfo_V1_t *)SI)->SharingClientCount * sizeof(uint32_t));
        extraDataAllocLen = extraDataLen + defaultSubIdsLen;
    }

    ieieImportSubscriptionCallbackContext_t *context = iemem_malloc(pThreadData,
                                                                    IEMEM_PROBE(iemem_exportResources, 18),
                                                                    sizeof(ieieImportSubscriptionCallbackContext_t)+
                                                                    extraDataAllocLen);

    if (context == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    context->stage = ieieISS_Start;
    context->dataType = dataType;
    context->dataId = dataId;
    context->wentAsync = false;
    context->destroyTried = false;
    context->subscription = NULL;

    const char *extraData = (const char *)(&(context->info)+1);

    // Same version can just copy everything across
    if (SI->Version == ieieSUBSCRIPTION_CURRENT_VERSION)
    {
        memcpy(&context->info, data, dataLen);
    }
    // Other versions need to copy pieces across and initialize new fields.
    else
    {
        assert(SI->Version == ieieSUBSCRIPTION_VERSION_1);
        assert(offsetof(ieieSubscriptionInfo_t, AnonymousSharers) == offsetof(ieieSubscriptionInfo_V1_t, GenericallyShared));
        assert(sizeof(context->info.AnonymousSharers) == sizeof(((ieieSubscriptionInfo_V1_t *)0)->GenericallyShared));

        uint8_t *copyPtr = (uint8_t *)extraData;

        memcpy(&context->info, data, sizeof(ieieSubscriptionInfo_V1_t));
        context->info.SubId = ismENGINE_NO_SUBID;
        context->info.SharingClientSubIdsLength = defaultSubIdsLen;
        memcpy(copyPtr, data+sizeof(ieieSubscriptionInfo_V1_t), extraDataLen);
        copyPtr += extraDataLen;

        if (defaultSubIdsLen != 0)
        {
            assert(ismENGINE_NO_SUBID == 0);
            memset(copyPtr, 0, (size_t)defaultSubIdsLen);
            copyPtr += defaultSubIdsLen;
        }
    }

    size_t SharingClientArraySize = context->info.SharingClientCount * sizeof(const char *);
    size_t SharingClientOptionsArraySize;
    size_t SharingClientSubIdArraySize;
    if (SharingClientArraySize != 0)
    {
        assert(context->dataType == ieieDATATYPE_EXPORTEDGLOBALLYSHAREDSUB);

        context->sharingClientIds = iemem_malloc(pThreadData,
                                                 IEMEM_PROBE(iemem_exportResources, 19),
                                                 SharingClientArraySize);
        if (context->sharingClientIds == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }

        if (rc == OK)
        {
            SharingClientOptionsArraySize = context->info.SharingClientCount * sizeof(uint32_t);
            context->sharingClientSubOptions = iemem_malloc(pThreadData,
                                                            IEMEM_PROBE(iemem_exportResources, 20),
                                                            SharingClientOptionsArraySize);
            if (context->sharingClientSubOptions == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            }
        }
        else
        {
            context->sharingClientSubOptions = NULL;
        }

        if (rc == OK)
        {
            SharingClientSubIdArraySize = context->info.SharingClientCount * sizeof(ismEngine_SubId_t);
            context->sharingClientSubIds = iemem_malloc(pThreadData,
                                                        IEMEM_PROBE(iemem_exportResources, 27),
                                                        SharingClientSubIdArraySize);
            if (context->sharingClientSubIds == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            }
        }
        else
        {
            context->sharingClientSubIds = NULL;
        }

        if (rc != OK)
        {
            assert(rc == ISMRC_AllocateError);

            iemem_free(pThreadData, iemem_exportResources, context->sharingClientSubIds);
            iemem_free(pThreadData, iemem_exportResources, context->sharingClientSubOptions);
            iemem_free(pThreadData, iemem_exportResources, context->sharingClientIds);
            iemem_free(pThreadData, iemem_exportResources, context);

            goto mod_exit;
        }

        assert(SharingClientArraySize != 0);
        assert(SharingClientOptionsArraySize != 0);
        assert(SharingClientSubIdArraySize != 0);
    }
    else
    {
        context->sharingClientIds = NULL;
        context->sharingClientSubOptions = NULL;
        context->sharingClientSubIds = NULL;

        SharingClientOptionsArraySize = SharingClientSubIdArraySize = 0;
    }

    assert(extraData == (const char *)(&(context->info)+1));

    // ClientId
    assert(context->info.ClientIdLength != 0);
    context->owningClientId = extraData;
    extraData += context->info.ClientIdLength;

    // TopicString
    assert(context->info.TopicStringLength != 0);
    context->topicString = extraData;
    extraData += context->info.TopicStringLength;

    // Subscription Name
    if (context->info.SubNameLength != 0)
    {
        context->subscriptionName = extraData;
        extraData += context->info.SubNameLength;
    }
    else
    {
        context->subscriptionName = NULL;
    }

    // Subscription Properties
    if (context->info.SubPropertiesLength != 0)
    {
        context->subProperties = extraData;
        extraData += context->info.SubPropertiesLength;
    }
    else
    {
        context->subProperties = NULL;
    }

    // Policy Name
    if (context->info.PolicyNameLength != 0)
    {
        context->policyName = extraData;
        extraData += context->info.PolicyNameLength;
    }
    else
    {
        context->policyName = NULL;
    }

    // Sharing ClientIds, Subscription options and subscription Ids
    if (SharingClientArraySize != 0)
    {
        assert(context->info.SharingClientCount != 0);
        assert(SharingClientOptionsArraySize == sizeof(uint32_t)*context->info.SharingClientCount);
        assert(SharingClientSubIdArraySize == sizeof(ismEngine_SubId_t)*context->info.SharingClientCount);

        for(uint32_t i=0; i<context->info.SharingClientCount; i++)
        {
            context->sharingClientIds[i] = extraData;
            while(*extraData != '\0')
            {
                extraData++;
            }
            extraData++;
            assert(strlen(context->sharingClientIds[i]) != 0);

            // Check that the sharingClient does exist (i.e. has been imported)
            ismEngine_ClientState_t *sharingClient;

            rc = ieie_findImportedClientStateByClientId(pThreadData,
                                                        control,
                                                        context->sharingClientIds[i],
                                                        &sharingClient);

            if (rc != OK)
            {
                iemem_free(pThreadData, iemem_exportResources, context->sharingClientIds);
                iemem_free(pThreadData, iemem_exportResources, context);
                ism_common_setError(rc);
                goto mod_exit;
            }

            assert(sharingClient != NULL);
        }
        memcpy(context->sharingClientSubOptions, extraData, SharingClientOptionsArraySize);
        extraData += SharingClientOptionsArraySize;
        memcpy(context->sharingClientSubIds, extraData, SharingClientSubIdArraySize);
        extraData += SharingClientSubIdArraySize;
    }

    context->control = control;

    rc = ieie_doImportSubscription(pThreadData, OK, NULL, context);

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of exportSubscription.c                                       */
/*                                                                   */
/*********************************************************************/
