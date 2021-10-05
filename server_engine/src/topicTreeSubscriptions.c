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
/// @file  topicTreeSubscriptions.c
/// @brief Engine component subscription manipulation functions
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
#include "clientState.h"         // iecs functions & constants
#include "resourceSetStats.h"    // iere functions & constants

//****************************************************************************
/// @brief Structure used to remember clients that need to relinquish messages
///        on a shared queue once its lock has been released.
//****************************************************************************
typedef struct tag_iettRelinquishInfo_t
{
    iecsMessageDeliveryInfoHandle_t hMsgDelInfo;
    ismEngine_RelinquishType_t relinquishType;
} iettRelinquishInfo_t;

//****************************************************************************
/// @brief Information passed between phases of subscription creation
//****************************************************************************
typedef struct tag_iettCreateSubscriptionPhaseInfo_t
{
    iettCreateSubscriptionClientInfo_t  clientInfo;
    ismEngine_Subscription_t           *subscription;
    iettSharedSubData_t                *sharedSubData;
    char                               *queueName;
    uint32_t                            subOptions;
    ismEngine_SubId_t                   subId;
    uint32_t                            internalAttrs;
    uint32_t                            phase;
    ismQueueType_t                      queueType;
    ieqOptions_t                        queueOptions;
    bool                                incrementedDOC;
    bool                                persistent;
    uint32_t                            asyncStackIndex;
    iepiPolicyInfo_t                   *policyInfo;
    ismStore_Handle_t                   hStoreDefn;
    ismStore_Handle_t                   hStoreProps;
    const char                         *topicString;
    size_t                              topicStringLength;
    ismEngine_SubscriptionHandle_t     *pSubHandle;
    iereResourceSetHandle_t             resourceSet;
    // The following are used in phase3 if the commit goes async
    void                               *externalContext;
    size_t                              externalContextLength;
    ismEngine_CompletionCallback_t      externalCallback;
} iettCreateSubscriptionPhaseInfo_t;

//****************************************************************************
/// @brief Allocate the storage for a subscription
///
/// Allocates the storage for an ismEngine_Subscription_t.
///
/// @param[in]     pClientId          Client Id of creator
/// @param[out]    pClientIdLength    Optional returned client Id length
/// @param[in]     pSubName           Subscription name (may be NULL)
/// @param[out]    pSubNameLength     Optional subscription name length
/// @param[in]     pSubContext        Subscription context data (may be NULL)
/// @param[in,out] pSubContextLength  Updated length of subscription context
/// @param[in]     pSubAttributes     Supplied subscription attributes
/// @param[in]     internalAttrs      iettSUBATTR_* values
/// @param[in]     resourceSet        Resource Set to which this subscription belongs
/// @param[out]    pSubscription      Returned subscription
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_allocateSubscription(ieutThreadData_t *pThreadData,
                                  const char *pClientId,
                                  size_t *pClientIdLength,
                                  const char *pSubName,
                                  size_t *pSubNameLength,
                                  void *pFlatSubProperties,
                                  size_t *pFlatSubPropertiesLength,
                                  const ismEngine_SubscriptionAttributes_t *pSubAttributes,
                                  uint32_t internalAttrs,
                                  iereResourceSetHandle_t resourceSet,
                                  ismEngine_Subscription_t **ppSubscription)
{
    int32_t rc = OK;

    size_t clientIdLength;
    size_t subNameLength;
    size_t flatSubPropertiesLength;
    size_t sharedSubDataLength;
    size_t newSubCreationDataLength;
    ismEngine_Subscription_t *subscription = NULL;
    ismRule_t *pSelectionRule = NULL;
    int32_t SelectionRuleLen = 0;
    int32_t SelectionRuleOffset = 0;
    bool fFreeSelectionRule = false;

    ieutTRACEL(pThreadData, pSubAttributes->subOptions, ENGINE_FNC_TRACE, FUNCTION_ENTRY "SubOptions=0x%08x SubId=%u\n",
               __func__, pSubAttributes->subOptions, pSubAttributes->subId);

    clientIdLength = strlen(pClientId)+1;

    if (NULL != pSubName)
    {
        subNameLength = strlen(pSubName)+1;
    }
    else
    {
        subNameLength = 0;
    }

    if (NULL == pFlatSubProperties)
    {
        flatSubPropertiesLength = 0;
    }
    else
    {
        flatSubPropertiesLength = *pFlatSubPropertiesLength;
    }

    *pFlatSubPropertiesLength = flatSubPropertiesLength;

    // If message selection was requested then cache pointers. At the end
    // of this section
    // SelectRuleLen will be > 0 if there is a selection string
    // SelectionString will be != NULL if a string selection string was passed
    // pSelectionRule will be != NULL if a string selection string was passed
    //    or a compiled selection rule was passed but it was not aligned on
    //    a 4 byte boundard
    // pSelectionRuleOffset will be non-zero if a compiled selection rule was
    //    passed and it is 4 byte aligned
    if ((pSubAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION) &&
        (flatSubPropertiesLength != 0))
    {
        concat_alloc_t flatProp = { pFlatSubProperties
                                  , flatSubPropertiesLength
                                  , flatSubPropertiesLength
                                  , 0
                                  , false };

        ism_field_t selectionProperty;

        rc = ism_common_findPropertyName( &flatProp
                                        , ismENGINE_PROPERTY_SELECTOR
                                        , &selectionProperty);

        if (rc != OK)
        {
            ieutTRACE_FFDC(ieutPROBE_004, false,
                           "Missing selection string", rc,
                           NULL);
            rc = ISMRC_InvalidParameter;
            ism_common_setError(rc);
            goto mod_exit;
        }
        else
        {
            if (selectionProperty.type == VT_ByteArray)
            {
                SelectionRuleOffset = (ptrdiff_t)selectionProperty.val.s -
                                      (ptrdiff_t)(pFlatSubProperties);
                SelectionRuleLen = selectionProperty.len;
                if ((SelectionRuleOffset % 4) != 0)
                {
                    // The selection rule passed is not 4byte aligned so we
                    // must allocate extra space and copy it out of the property.
                    pSelectionRule = (ismRule_t *)selectionProperty.val.s;
                    SelectionRuleOffset = 0;
                }
            }
            else
            {
                assert(selectionProperty.type == VT_String);

                // We have been provided a selection string in uncompiled
                // form. Let's assume it is an SQL92 selection string and
                // try and compile it.

                rc = ism_common_compileSelectRule(&pSelectionRule,
                                                  (int *)&SelectionRuleLen,
                                                  selectionProperty.val.s);
                if (rc == OK)
                {
                    fFreeSelectionRule = true;
                }
                else
                {
                    ieutTRACE_FFDC(ieutPROBE_005, false,
                                   "Selection string compilation failed.", rc,
                                   NULL);
                    rc = ISMRC_InvalidParameter;
                    ism_common_setError(rc);
                    goto mod_exit;
                }
            }
        }
    }
    else if (pSubAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION)
    {
        ieutTRACE_FFDC(ieutPROBE_006, false,
                       "Selection requested but not property provided.", rc,
                       NULL);
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Ensure that we allocate enough storage for the shared subscription data
    if (pSubAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED)
    {
        sharedSubDataLength = sizeof(iettSharedSubData_t);
    }
    else
    {
        sharedSubDataLength = 0;
    }

    // For non-rehydrated subscriptions allow for subscription creation data
    if (internalAttrs & iettSUBATTR_REHYDRATED)
    {
        newSubCreationDataLength = 0;
    }
    else
    {
        newSubCreationDataLength = sizeof(iettNewSubCreationData_t);
    }

    iere_primeThreadCache(pThreadData, resourceSet);

    // Note, the flattened properties and the selection rule must both
    // be stored on a 4-byte boundary so we allocate an extra 4 bytes
    // for the selection rule to allow padding to 4 bytes boundaries if
    // required.
    void *flatSubPropertiesCopy;
    if (flatSubPropertiesLength != 0)
    {
        size_t allocationLength = flatSubPropertiesLength;

        if (SelectionRuleOffset == 0 && SelectionRuleLen != 0)
        {
            allocationLength += (size_t)(SelectionRuleLen + 4); // 4-byte boundary
        }

        flatSubPropertiesCopy = iere_malloc(pThreadData,
                                            resourceSet,
                                            IEMEM_PROBE(iemem_subsTree, 9), allocationLength);
        if (NULL == flatSubPropertiesCopy)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        // make sure it's 4 byte aligned
        assert(((ptrdiff_t)flatSubPropertiesCopy & 0x3) == 0);

        memcpy(flatSubPropertiesCopy, pFlatSubProperties, flatSubPropertiesLength);
    }
    else
    {
        flatSubPropertiesCopy = NULL;
    }

    subscription = iere_calloc(pThreadData,
                               resourceSet,
                               IEMEM_PROBE(iemem_subsTree, 6), 1,
                               sizeof(ismEngine_Subscription_t)
                               + sharedSubDataLength + newSubCreationDataLength
                               + subNameLength
                               + clientIdLength);

    if (NULL == subscription)
    {
        iere_free(pThreadData, resourceSet, iemem_subsTree, flatSubPropertiesCopy);
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Decide whether this subscription is one that we should share with a cluster
    // or not.
    //
    // At the moment this can be achieved by establishing that the subscription is
    // not shared (ismENGINE_SUBSCRIPTION_OPTION_SHARED not set), that it is not
    // a JMS subscription (ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE not set)
    // and that it is not a subscription on a system topic (iettSUBATTR_SYSTEM_TOPIC
    // not set)
    //
    // This may be a different decision in the future - and may be more complicated
    // than checking the subOptions / internalAtts - for instance, we might need to
    // change the information stored for a subscription to include the protocolId from
    // the owning client when the subscription was created (which would mean changing
    // the subscription object in the store).
    if (((pSubAttributes->subOptions & (ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                        ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE)) == 0) &&
        ((internalAttrs & iettSUBATTR_SYSTEM_TOPIC) == 0))
    {
        internalAttrs |= iettSUBATTR_SHARE_WITH_CLUSTER;
    }

    /***********************************************************************/
    /* Initialize the structure.                                           */
    /***********************************************************************/
    memcpy(subscription->StrucId, ismENGINE_SUBSCRIPTION_STRUCID, 4);
    // Use only the persistent subOptions in the actual subscription
    subscription->subOptions = pSubAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK;
    subscription->subId = pSubAttributes->subId;
    subscription->internalAttrs = internalAttrs;
    subscription->nodeListIndex = 0xffffffff;
    subscription->resourceSet = resourceSet;

    // Initialize useCount to 1 for named subscriptions - this is decremented
    // when the request to destroy them is received.
    if (pSubName != NULL) subscription->useCount = 1;

    char *tmpPtr = (char *)(subscription+1);

    /***********************************************************************/
    /* Initialize shared subscription data if required                     */
    /***********************************************************************/
    if (sharedSubDataLength != 0)
    {
        iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);

        assert(sharedSubData != NULL);

        int osrc = pthread_spin_init(&sharedSubData->lock, PTHREAD_PROCESS_PRIVATE);

        if (osrc != 0)
        {
            rc = ISMRC_Error;
            ism_common_setError(rc);
            goto mod_exit;
        }

        tmpPtr += sharedSubDataLength;
    }

    /***********************************************************************/
    /* Move beyond any new subscription data                               */
    /***********************************************************************/
    if (newSubCreationDataLength != 0)
    {
        assert(iett_getNewSubCreationData(subscription) != NULL);

        tmpPtr += newSubCreationDataLength;
    }

    /***********************************************************************/
    /* Record the Subscription Name                                        */
    /***********************************************************************/
    if (subNameLength != 0)
    {
        memcpy(tmpPtr, pSubName, subNameLength);
        subscription->subName = tmpPtr;
        tmpPtr += subNameLength;
        subscription->subNameHash = iett_generateSubNameHash(subscription->subName);
    }
    if (NULL != pSubNameLength) *pSubNameLength = subNameLength;

    /***********************************************************************/
    /* Record the Client Id                                                */
    /***********************************************************************/
    if (clientIdLength != 0)
    {
        memcpy(tmpPtr, pClientId, clientIdLength);
        subscription->clientId = tmpPtr;
        tmpPtr += clientIdLength;
        subscription->clientIdHash = iett_generateClientIdHash(subscription->clientId);
    }
    if (NULL != pClientIdLength) *pClientIdLength = clientIdLength;

    /***********************************************************************/
    /* Store the subscription properties                                   */
    /***********************************************************************/
    if (flatSubPropertiesLength != 0)
    {
        subscription->flatSubPropertiesLength = flatSubPropertiesLength;
        subscription->flatSubProperties = flatSubPropertiesCopy;

        /*******************************************************************/
        /* Record the compiled selection string                            */
        /*******************************************************************/
        if (SelectionRuleOffset != 0)
        {
            // No copy of selection rule, use version direct from flattened
            //  properties
            subscription->selectionRule = (ismRule_t *)(flatSubPropertiesCopy + SelectionRuleOffset);
            subscription->selectionRuleLen = SelectionRuleLen;
        }
        else if (pSelectionRule != NULL)
        {
            char *selectionRulePtr = (char *)(RoundUp4((ptrdiff_t)(flatSubPropertiesCopy + flatSubPropertiesLength)));
            memcpy(selectionRulePtr, pSelectionRule, SelectionRuleLen);
            subscription->selectionRule = (ismRule_t *)selectionRulePtr;
            subscription->selectionRuleLen = SelectionRuleLen;

            if (fFreeSelectionRule)
            {
                ism_common_freeSelectRule(pSelectionRule);
            }
        }
     }

    *ppSubscription = subscription;

mod_exit:

    ieutTRACEL(pThreadData, subscription, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return rc;
}

//****************************************************************************
/// @brief  Release any client states being held while creating a subscription
///
/// @param[in]     pInfo                  Structure containing required client
///                                       information.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
void iett_createSubscriptionReleaseClients(ieutThreadData_t *pThreadData,
                                           iettCreateSubscriptionClientInfo_t *pInfo)
{
    if (pInfo->releaseClientState)
    {
        if (pInfo->requestingClient != NULL)
        {
            (void)iecs_releaseClientStateReference(pThreadData, pInfo->requestingClient, false, false);
        }

        if (pInfo->requestingClient != pInfo->owningClient)
        {
            (void)iecs_releaseClientStateReference(pThreadData, pInfo->owningClient, false, false);
        }
    }
}

//****************************************************************************
/// @brief  Async callback to release any client states held in create
///         subscription
///
/// @param[in]     rc                     Return code from the previous phase
/// @param[in]     asyncInfo              AsyncData for this call
/// @param[in]     context                AsyncData entry for this call.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_asyncCreateSubscriptionReleaseClients(ieutThreadData_t *pThreadData,
                                                   int32_t rc,
                                                   ismEngine_AsyncData_t *asyncInfo,
                                                   ismEngine_AsyncDataEntry_t *context)
{
    assert(context->Type == TopicCreateSubscriptionClientInfo);

    // Get the create subscription phase 2 info off the stack
    iettCreateSubscriptionClientInfo_t *pInfo = (iettCreateSubscriptionClientInfo_t *)(context->Data);

    iead_popAsyncCallback(asyncInfo, context->DataLen);

    iett_createSubscriptionReleaseClients(pThreadData, pInfo);

    return OK;
}

//****************************************************************************
/// @brief  Third phase of subscription creation (after async commit)
///
/// Invokes the remaining async actions.
///
/// @param[in]     rc        Return code from the previous phase
/// @param[in]     pContext  A pointer to the ismEngine_AsyncData_t
///                          containing remaining callbacks.
//****************************************************************************
static void iett_createSubscriptionPostAsyncCommit(ieutThreadData_t *pThreadData,
                                                   int32_t rc,
                                                   void *pContext)
{
    if (pContext != NULL) iead_completeAsyncData(rc, *(void**)pContext);
}

//****************************************************************************
/// @brief  Third phase of subscription creation (Adding to topic tree)
///
/// Adds the subscription that will have been written to the store (if needed)
/// to the topic tree.
///
/// @param[in]     rc                     Return code from the previous phase
/// @param[in]     asyncInfo              Pointer to the current asyncInfo
/// @param[in]     pInfo                  Structure containing required state
///                                       for the createSubscription phases.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static int32_t iett_createSubscriptionPhase3(ieutThreadData_t *pThreadData,
                                             int32_t rc,
                                             ismEngine_AsyncData_t *asyncInfo,
                                             iettCreateSubscriptionPhaseInfo_t *pInfo)
{
    ismEngine_Transaction_t  *pTran = NULL;
    bool addedToTree = false;
    assert(pInfo != NULL);
    ismEngine_Subscription_t *subscription = pInfo->subscription;
    iereResourceSetHandle_t resourceSet = pInfo->resourceSet;

    assert(pInfo->phase == 2);

    pInfo->phase = 3;

    ieutTRACEL(pThreadData, pInfo, ENGINE_FNC_TRACE, FUNCTION_ENTRY "rc=%d, pInfo=%p\n", __func__,
               rc, pInfo);

    if (rc != OK) goto mod_exit;

    // Create an fAsStoreTran transaction that will contain all of the operations associated
    // with the creation of this subscription:
    //   - Writing the subscription to the store (if durable)
    //   - Adding the subscription to the topic tree
    //   - Putting retained messages
    rc = ietr_createLocal(pThreadData,
                          NULL,
                          pInfo->persistent,
                          true, // fAsStoreTran
                          NULL,
                          &pTran);

    if (rc != OK) goto mod_exit;

    bool putRetained = (pInfo->subOptions & ismENGINE_SUBSCRIPTION_OPTION_NO_RETAINED_MSGS) == 0;

    // Add the subscription to the topic tree as part of the transaction
    rc = iett_addSubToEngineTopic(pThreadData,
                                  pInfo->topicString,
                                  pInfo->subscription,
                                  pTran,
                                  false,
                                  putRetained);

    // If the subscription was added to the tree (i.e. has a reference to a node
    // in the tree) then any error will cause the queue to be deleted and the
    // subscription to be free'd by the transaction when it is removed from the
    // tree - so we no longer need to clear up here.
    if (pInfo->subscription->node != NULL)
    {
        addedToTree = true;
    }

    if (rc != OK) goto mod_exit;

    // Add the subscription to each of the sharing client's list of subscriptions if required
    if (pInfo->sharedSubData != NULL)
    {
        for(uint32_t i=0; i<pInfo->sharedSubData->sharingClientCount; i++)
        {
            rc = iett_addSubscription(pThreadData,
                                      pInfo->subscription,
                                      pInfo->sharedSubData->sharingClients[i].clientId,
                                      pInfo->sharedSubData->sharingClients[i].clientIdHash);

            if (rc != OK) goto mod_exit;
        }
    }

    // SubHandle was requested, so return it
    if (NULL != pInfo->pSubHandle)
    {
        if (asyncInfo != NULL)
        {
            iead_setEngineCallerHandle(asyncInfo, pInfo->subscription);
        }

        *pInfo->pSubHandle = (ismEngine_SubscriptionHandle_t)pInfo->subscription;
    }

mod_exit:

    if (pInfo->queueName != NULL) iett_freeSubQueueName(pThreadData, pInfo->queueName);

    // We don't expect anything above here to have caused us to go async
    assert(rc != ISMRC_AsyncCompletion);

    ietrAsyncTransactionDataHandle_t asyncCommitTransactionData = NULL;
    ismEngine_AsyncData_t *heapAsyncInfo = NULL;

    // Get ready for the persistent commit to go async...
    if (rc == OK && pInfo->persistent == true)
    {
        // Include the remaining asyncInfo for an asynchronous commit
        assert(asyncInfo != NULL);

        // It needs to be on the heap for this to work.
        heapAsyncInfo = iead_ensureAsyncDataOnHeap(pThreadData, asyncInfo);

        asyncCommitTransactionData = ietr_allocateAsyncTransactionData( pThreadData
                                                                      , pTran
                                                                      , true
                                                                      , sizeof(heapAsyncInfo));

        if (asyncCommitTransactionData != NULL)
        {
            ismEngine_AsyncData_t **ppHeapAsyncInfo = ietr_getCustomDataPtr(asyncCommitTransactionData);
            *ppHeapAsyncInfo = heapAsyncInfo;
        }
        else
        {
            rc = ISMRC_AllocateError;
        }

        // If we didn't make a copy, we don't want to free it if the commit doesn't
        // go async - also, if we did make a copy but there was a subsequent failure
        // allocating the transaction data, we want to free the copy.
        if (heapAsyncInfo == asyncInfo)
        {
            heapAsyncInfo = NULL;
        }
        else if (rc != OK)
        {
            iead_freeAsyncData(pThreadData, heapAsyncInfo);
        }
    }

    // All OK, commit.
    if (rc == OK)
    {
        assert(NULL != pTran);

        rc = ietr_commit(pThreadData,
                         pTran,
                         ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT,
                         NULL,
                         asyncCommitTransactionData,
                         iett_createSubscriptionPostAsyncCommit);

        assert((pInfo->persistent == false && rc == OK) ||
               (pInfo->persistent == true && (rc == OK || rc == ISMRC_AsyncCompletion)));

        // If the commit didn't go async, and we allocated an ismEngine_AsyncData_t for it
        // heapAsyncInfo will be non-NULL, if that is the case, we need to free it.
        if (rc != ISMRC_AsyncCompletion && heapAsyncInfo != NULL)
        {
            assert(pInfo->persistent == true);
            iead_freeAsyncData(pThreadData, heapAsyncInfo);
        }
    }
    else
    {
        DEBUG_ONLY int32_t rc2;

        if (pInfo->incrementedDOC)
        {
            iecs_decrementDurableObjectCount(pThreadData, pInfo->clientInfo.owningClient);
        }

        // Roll back the transaction
        if (NULL != pTran)
        {
            rc2 = ietr_rollback(pThreadData, pTran, NULL, IETR_ROLLBACK_OPTIONS_NONE);
            assert(rc2 == OK);
        }

        // If this was never added to the topic tree, the transaction will not
        // perform deletion of the queue or releasing of the subscription storage,
        // so we need to do that manually.
        if (!addedToTree)
        {
            if (NULL != pInfo->subscription)
            {
                if (NULL != pInfo->subscription->queueHandle)
                {
                    pInfo->policyInfo = NULL; // PolicyInfo will be released by ieq_delete
                    rc2 = ieq_delete(pThreadData, &pInfo->subscription->queueHandle, false);
                    assert(rc2 == OK);
                }
                else
                {
                    uint32_t storeOpCount = 0;

                    rc2 = OK;

                    if (pInfo->hStoreDefn != ismSTORE_NULL_HANDLE)
                    {
                        rc2 = ism_store_deleteRecord(pThreadData->hStream, pInfo->hStoreDefn);
                        if (rc2 == OK) storeOpCount++;
                    }

                    if (pInfo->hStoreProps != ismSTORE_NULL_HANDLE)
                    {
                        rc2 = ism_store_deleteRecord(pThreadData->hStream, pInfo->hStoreProps);
                        if (rc2 == OK) storeOpCount++;
                    }

                    if (storeOpCount != 0) iest_store_commit(pThreadData, false);
                }

                assert(pInfo->subscription->resourceSet == pInfo->resourceSet);
                iere_primeThreadCache(pThreadData, resourceSet);
                iere_free(pThreadData, resourceSet, iemem_subsTree, pInfo->subscription->flatSubProperties);
                iere_freeStruct(pThreadData, resourceSet, iemem_subsTree, pInfo->subscription, pInfo->subscription->StrucId);
            }

            if (pInfo->policyInfo != NULL) iepi_releasePolicyInfo(pThreadData, pInfo->policyInfo);
        }

    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, subscription=%p\n", __func__,
               rc, subscription);

    return rc;
}

//****************************************************************************
/// @brief  Call create subscription phase 3 when phase 2 goes asynchonous
///
/// Unpack the async callback structures and call phase3.
///
/// @param[in]     rc                     Return code
/// @param[in]     asyncInfo              Async information
/// @param[in]     context                Async data entry
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static int32_t iett_asyncCreateSubscriptionPhase3(ieutThreadData_t *pThreadData,
                                                  int32_t rc,
                                                  ismEngine_AsyncData_t *asyncInfo,
                                                  ismEngine_AsyncDataEntry_t *context)
{
    assert(context->Type == TopicCreateSubscriptionPhaseInfo);
    assert(rc == OK); //Store commit shouldn't fail (fatal fdc here?)

    // Get the create subscription phase 2 info off the stack
    iettCreateSubscriptionPhaseInfo_t *pInfo = (iettCreateSubscriptionPhaseInfo_t *)(context->Data);

    iead_popAsyncCallback(asyncInfo, context->DataLen);

    // Grab the topic string from the next entry down the stack (which is now the top)
    ismEngine_AsyncDataEntry_t *topicStringContext = &asyncInfo->entries[asyncInfo->numEntriesUsed-1];

    assert(topicStringContext->Type == TopicCreateSubscriptionTopicString);

    pInfo->topicString = topicStringContext->Data;

    iead_popAsyncCallback(asyncInfo, topicStringContext->DataLen);

    rc = iett_createSubscriptionPhase3(pThreadData, rc, asyncInfo, pInfo);

    return rc;
}

//****************************************************************************
/// @brief  Second phase of subscription creation (Creating the queue)
///
/// @param[in]     rc                     Return code from the previous phase
/// @param[in]     asyncInfo              Pointer to the current asyncInfo
/// @param[in]     pInfo                  Structure containing required state
///                                       for the createSubscription phases.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static int32_t iett_createSubscriptionPhase2(ieutThreadData_t *pThreadData,
                                             int32_t rc,
                                             ismEngine_AsyncData_t *asyncInfo,
                                             iettCreateSubscriptionPhaseInfo_t *pInfo)
{
    assert(pInfo != NULL);
    ismEngine_Subscription_t *subscription = pInfo->subscription;

    assert(pInfo->phase == 1);

    pInfo->phase = 2;

    ieutTRACEL(pThreadData, pInfo, ENGINE_FNC_TRACE, FUNCTION_ENTRY "rc=%d, pInfo=%p\n", __func__,
               rc, pInfo);

    uint32_t asyncStackIndex = pInfo->asyncStackIndex;

    // Check that we are set up ready to go asynchronous
    if (asyncStackIndex != 0)
    {
        assert(asyncInfo->entries[asyncStackIndex].Type == TopicCreateSubscriptionPhaseInfo);
        assert(asyncInfo->entries[asyncStackIndex].pCallbackFn.internalFn == iett_asyncCreateSubscriptionPhase3);
        assert(asyncInfo->entries[asyncStackIndex-1].Type == TopicCreateSubscriptionTopicString);
        assert(asyncInfo->entries[asyncStackIndex-1].pCallbackFn.internalFn == iett_asyncCreateSubscriptionPhase3);
    }

    if (rc != OK) goto mod_exit;

    assert(subscription != NULL);

    // Create the queue
    rc = ieq_createQ( pThreadData
                    , pInfo->queueType
                    , pInfo->queueName
                    , pInfo->queueOptions
                    , pInfo->policyInfo
                    , pInfo->hStoreDefn
                    , pInfo->hStoreProps
                    , pInfo->resourceSet
                    , &(subscription->queueHandle));

mod_exit:

    // The queue creation didn't go async, we need to proceed to the next phase
    if (rc != ISMRC_AsyncCompletion)
    {
        // Re-asses the structures in case they have been moved in the asyncInfo stack -- while the
        // data may have moved, we ought to be on the same index in the asyncInfo structure.
        //
        // We also use this opportunity to pop the entries off the stack since we do not want
        // phase3 to be called again.
        if (asyncStackIndex != 0)
        {
            assert(asyncInfo->entries[asyncStackIndex].DataLen == sizeof(iettCreateSubscriptionPhaseInfo_t));
            pInfo = asyncInfo->entries[asyncStackIndex].Data;
            iead_popAsyncCallback(asyncInfo, asyncInfo->entries[asyncStackIndex].DataLen);

            assert(asyncInfo->entries[asyncStackIndex-1].DataLen == (pInfo->topicStringLength + 1));
            pInfo->topicString = asyncInfo->entries[asyncStackIndex-1].Data;
            iead_popAsyncCallback(asyncInfo, asyncInfo->entries[asyncStackIndex-1].DataLen);
        }

        rc = iett_createSubscriptionPhase3(pThreadData, rc, asyncInfo, pInfo);
    }
    else
    {
        assert(pInfo->persistent == false);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, subscription=%p\n", __func__,
               rc, subscription);

    return rc;
}

//****************************************************************************
/// @brief  Call create subscription phase 2 when phase 1 goes asynchonous
///
/// Unpack the async callback structures and call phase2.
///
/// @param[in]     rc                     Return code
/// @param[in]     asyncInfo              Async information
/// @param[in]     context                Async data entry
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static int32_t iett_asyncCreateSubscriptionPhase2(ieutThreadData_t *pThreadData,
                                                  int32_t rc,
                                                  ismEngine_AsyncData_t *asyncInfo,
                                                  ismEngine_AsyncDataEntry_t *context)
{
    assert(context->Type == TopicCreateSubscriptionPhaseInfo);
    assert(rc == OK); //Store commit shouldn't fail (fatal fdc here?)

    uint32_t infoEntry = asyncInfo->numEntriesUsed-1;
    uint32_t topicStringEntry = infoEntry-1;

    // Get the create subscription phase info off the stack
    iettCreateSubscriptionPhaseInfo_t *pInfo = (iettCreateSubscriptionPhaseInfo_t *)(context->Data);

    asyncInfo->entries[infoEntry].pCallbackFn.internalFn = iett_asyncCreateSubscriptionPhase3;

    // Grab the topic string from the next entry down the stack (which is now the top)
    ismEngine_AsyncDataEntry_t *topicStringContext = &asyncInfo->entries[topicStringEntry];

    assert(topicStringContext->Type == TopicCreateSubscriptionTopicString);

    pInfo->topicString = topicStringContext->Data;

    asyncInfo->entries[topicStringEntry].pCallbackFn.internalFn = iett_asyncCreateSubscriptionPhase3;

    rc = iett_createSubscriptionPhase2(pThreadData, rc, asyncInfo, pInfo);

    return rc;
}

//****************************************************************************
/// @brief  Internal function to create a Subscription
///
/// Creates a subscription for a topic. The subscription can be nondurable or
/// durable. The reliability of message delivery for the subscription is also
/// specified.
///
/// @param[in]     pClientInfo            Info about clients involved
/// @param[in]     pCreateSubInfo         Filled in subscription info structure
/// @param[out]    pSubHandle             Pointer to subscription handle to return
/// @param[in]     pAsyncInfo             Info needed to enable the call to go
///                                       asynchronous
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark pCreateSubInfo->policyInfo might be NULL here, if the request came
/// from __MQConnectivity or a unit test - if it is, then we want to create a
/// unique messaging policy for this subscription based on the defaults so that
/// it can be independently updated.
///
/// @remark The operation may complete synchronously or asynchronously. If it
/// completes synchronously, the return code indicates the completion
/// status.
///
/// If the operation completes asynchronously, the return code from
/// this function will be ISMRC_AsyncCompletion. The actual
/// completion of the operation will be signalled by a call to the
/// operation-completion callback, if one is provided. When the
/// operation becomes asynchronous, a copy of the context will be
/// made into memory owned by the Engine. This will be freed upon
/// return from the operation-completion callback. Because the
/// completion is asynchronous, the call to the operation-completion
/// callback might occur before this function returns.
//****************************************************************************
int32_t iett_createSubscription(ieutThreadData_t *pThreadData,
                                iettCreateSubscriptionClientInfo_t *pClientInfo,
                                const iettCreateSubscriptionInfo_t *pCreateSubInfo,
                                ismEngine_SubscriptionHandle_t *pSubHandle,
                                ismEngine_AsyncData_t *pAsyncInfo)
{
    int32_t rc = OK;
    iettCreateSubscriptionPhaseInfo_t info = {{0},0};
    size_t subNameLength;
    size_t clientIdLength;
    size_t flatSubPropertiesLength = 0;
    void *pflatSubProperties = NULL;
    concat_alloc_t FlatProperties = { NULL };

    assert(pCreateSubInfo != NULL);

    info.clientInfo = *pClientInfo;
    info.topicString = pCreateSubInfo->topicString;
    info.subOptions = pCreateSubInfo->subOptions;
    info.subId = pCreateSubInfo->subId;
    info.internalAttrs = pCreateSubInfo->internalAttrs;
    info.pSubHandle = pSubHandle;
    info.phase = 1;

    ieutTRACEL(pThreadData, info.clientInfo.requestingClient, ENGINE_FNC_TRACE, FUNCTION_ENTRY "requestingClient=%p, owningClient=%p, policyInfo=%p, topicString='%s', subOptions=0x%08x, internalAttrs=0x%08x\n", __func__,
               info.clientInfo.requestingClient, info.clientInfo.owningClient, pCreateSubInfo->policyInfo, info.topicString, info.subOptions, info.internalAttrs);

    // If this is a persistent subscription then we are going to need to
    // create records in the store. If the management generation is
    // almost full then prevent us making things worse.
    if ((info.internalAttrs & iettSUBATTR_PERSISTENT) == iettSUBATTR_PERSISTENT)
    {
        ismEngineComponentStatus_t storeStatus = ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_0];

        if (storeStatus != StatusOk)
        {
            ieutTRACEL(pThreadData, storeStatus, ENGINE_WORRYING_TRACE, "Rejecting createSubscription as store status[%d] is %d\n",
                       ismENGINE_STATUS_STORE_MEMORY_0, storeStatus);
            rc = ISMRC_ServerCapacity;
            ism_common_setError(rc);
            goto mod_exit;
        }

        info.persistent = true;
    }

    if (pCreateSubInfo->subName != NULL)
    {
        // Check that the specified subscription is not already in the table
        rc = iett_findClientSubscription(pThreadData,
                                         info.clientInfo.owningClient->pClientId,
                                         iett_generateClientIdHash(info.clientInfo.owningClient->pClientId),
                                         pCreateSubInfo->subName,
                                         iett_generateSubNameHash(pCreateSubInfo->subName),
                                         NULL);

        if (rc == OK)
        {
            ieutTRACEL(pThreadData, info.subscription, ENGINE_WORRYING_TRACE, "Existing subscription named '%s' for client '%s' found.\n",
                       pCreateSubInfo->subName, pClientInfo->owningClient->pClientId);
            rc = ISMRC_ExistingSubscription;
            ism_common_setError(rc);
            goto mod_exit;
        }
        // If subscription was not found, that's good.
        else
        {
            assert(rc == ISMRC_NotFound);
        }
    }

    // ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL requires a client Id - assert that this is the case.
    assert(((info.subOptions & ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL) == 0) ||
             info.clientInfo.owningClient->pClientId != NULL);

    // Check if the topic string is a system topic and whether it would match one of
    // the engine notification topics, adding additional internalAttrs flags
    // so that we can quickly identify subscriptions on those topics.
    if (iettTOPIC_IS_SYSTOPIC(info.topicString))
    {
        // Persistent subscriptions are not allowed on system topics
        if (info.persistent == true &&
            (info.clientInfo.owningClient->pClientId[0] != '_' ||
             info.clientInfo.owningClient->pClientId[1] != '_'))
        {
            rc = ISMRC_BadSysTopic;
            ism_common_setError(rc);
            goto mod_exit;
        }

        info.internalAttrs |= iettSUBATTR_SYSTEM_TOPIC;

        // DisconnectedClientNotification?
        if (iettTOPIC_MATCHES_DOLLARSYS_DCN(info.topicString))
        {
            info.internalAttrs |= iettSUBATTR_DOLLARSYS_DCN_TOPIC;
        }
    }

    // Deal with null policy info
    if (pCreateSubInfo->policyInfo == NULL)
    {
        // Create a unique policy info based on the defaults
        rc = iepi_createPolicyInfo(pThreadData, NULL, ismSEC_POLICY_LAST, false, NULL, &info.policyInfo);

        if (rc != OK) goto mod_exit;

        assert(info.policyInfo->creationState == CreatedByEngine); // admin unaware

        // The properties may contain some overrides to the validated policy
        if (pCreateSubInfo->flatLength == 0 && NULL != pCreateSubInfo->subProps.props)
        {
            rc = iepi_updatePolicyInfoFromProperties(pThreadData,
                                                     info.policyInfo,
                                                     NULL,
                                                     (struct ism_prop_t *)pCreateSubInfo->subProps.props,
                                                     NULL);

            if (rc != OK) goto mod_exit;
        }
    }
    // Make sure we've got a reference on the policy info we're using
    else
    {
        iepi_acquirePolicyInfoReference(pCreateSubInfo->policyInfo);
        info.policyInfo = pCreateSubInfo->policyInfo;
    }

    assert(info.policyInfo != NULL);

    char localPropBuffer[1024];
    // Create a serialized version of the properties
    if (pCreateSubInfo->flatLength != 0)
    {
        pflatSubProperties = (void *)(pCreateSubInfo->subProps.flat);
        flatSubPropertiesLength = pCreateSubInfo->flatLength;
    }
    else if (NULL != pCreateSubInfo->subProps.props)
    {
        FlatProperties.buf = localPropBuffer;
        FlatProperties.len = 1024;

        rc = ism_common_serializeProperties((ism_prop_t *)(pCreateSubInfo->subProps.props), &FlatProperties);

        if (rc != OK) goto mod_exit;

        flatSubPropertiesLength = FlatProperties.used;
        pflatSubProperties = FlatProperties.buf;
    }

    // The resourceSet to which a subscription belongs is based on topicString for
    // globally shared subscriptions, and on clientId for all others.
    if ((info.internalAttrs & iettSUBATTR_GLOBALLY_SHARED) == iettSUBATTR_GLOBALLY_SHARED)
    {
        info.resourceSet = iere_getResourceSet(pThreadData, NULL, info.topicString, iereOP_ADD);
    }
    else
    {
        assert(strncmp(info.clientInfo.owningClient->pClientId,
                       ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_PREFIX,
                       strlen(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_PREFIX)) != 0);
        info.resourceSet = info.clientInfo.owningClient->resourceSet;
        assert(info.resourceSet == iere_getResourceSet(pThreadData, info.clientInfo.owningClient->pClientId, NULL, iereOP_FIND));
    }

    // Create the structure and resources for this subscription - starting
    // with calculating the lengths of various strings and allocating the
    // storage to contain them.
    ismEngine_SubscriptionAttributes_t subAttributes = { info.subOptions, info.subId };
    rc = iett_allocateSubscription(pThreadData,
                                   info.clientInfo.owningClient->pClientId,
                                   &clientIdLength,
                                   pCreateSubInfo->subName,
                                   &subNameLength,
                                   pflatSubProperties,
                                   &flatSubPropertiesLength,
                                   &subAttributes,
                                   info.internalAttrs,
                                   info.resourceSet,
                                   &info.subscription);

    // Free resources used by the flattened properties
    if (FlatProperties.len != 0)
    {
        ism_common_freeAllocBuffer(&FlatProperties);
    }

    if (rc != OK) goto mod_exit;

    // Caller is going to expect a subscription back with useCount incremented.
    if (info.pSubHandle != NULL) info.subscription->useCount += 1;

    // We need a name for the queue associated with the subscription so that we can
    // print out something sensible in log messages.
    // Note that clientIdLength and subNameLength already include the null terminator
    info.topicStringLength = strlen(info.topicString);
    rc = iett_allocateSubQueueName(pThreadData,
                                   info.clientInfo.owningClient->pClientId,
                                   clientIdLength,
                                   pCreateSubInfo->subName,
                                   subNameLength,
                                   info.topicString,
                                   info.topicStringLength + 1,
                                   &info.queueName);

    if (rc != OK) goto mod_exit;

    // Transaction capable and shared subscriptions require a multiConsumer queue,
    // otherwise the type of queue to use is determined by the requested reliability.
    if ((info.subOptions & (ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                            ismENGINE_SUBSCRIPTION_OPTION_SHARED)) != 0)
    {
        info.queueType = multiConsumer;
    }
    else
    {
        // Calculate the Reliablity (QoS) for this subscription.
        uint32_t Reliability = info.subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK;

        if (Reliability == ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE)
        {
            info.queueType = simple;
        }
        else
        {
            info.queueType = intermediate;
        }
    }

    info.queueOptions = ieqOptions_SUBSCRIPTION_QUEUE;

    // Non shared subscriptions can be created with single Consumer only option
    // which improves performance
    if ((info.subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) == 0)
    {
        info.queueOptions |= ieqOptions_SINGLE_CONSUMER_ONLY;
    }
    // Shared subscriptions need to have additional information associated with them
    else
    {
        info.sharedSubData = iett_getSharedSubData(info.subscription);

        assert(info.sharedSubData != NULL);

        info.sharedSubData->anonymousSharers = pCreateSubInfo->anonymousSharer;

        if (pCreateSubInfo->sharingClientCount != 0)
        {
            assert(info.sharedSubData->sharingClientCount == 0);
            assert(info.sharedSubData->sharingClientMax == 0);

            uint32_t newSharingClientMax = pCreateSubInfo->sharingClientCount;

            if (newSharingClientMax < iettSHARED_SUBCLIENT_INCREMENT) newSharingClientMax = iettSHARED_SUBCLIENT_INCREMENT;

            // We use iemem_externalObjs for this memory because it is really just to
            // enable additional consumers on an existing subscription and we want to
            // be able to reallocate it in low memory situations, to help drain the
            // shared subscription.
            iere_primeThreadCache(pThreadData, info.resourceSet);
            iettSharingClient_t *newSharingClients = iere_calloc(pThreadData,
                                                                 info.resourceSet,
                                                                 IEMEM_PROBE(iemem_externalObjs, 5), 1,
                                                                 sizeof(iettSharingClient_t) * newSharingClientMax);

            if (newSharingClients == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            for(uint32_t i=0; i<pCreateSubInfo->sharingClientCount; i++)
            {
                char *newClientId = iere_malloc(pThreadData,
                                                info.resourceSet,
                                                IEMEM_PROBE(iemem_externalObjs, 6),
                                                strlen(pCreateSubInfo->sharingClientIds[i])+1);

                if (newClientId == NULL)
                {
                    for(uint32_t j=0; j<i; j++)
                    {
                        iere_free(pThreadData, info.resourceSet, iemem_externalObjs, newSharingClients[j].clientId);
                    }
                    iere_free(pThreadData, info.resourceSet, iemem_externalObjs, newSharingClients);
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    goto mod_exit;
                }

                strcpy(newClientId, pCreateSubInfo->sharingClientIds[i]);

                newSharingClients[i].clientId = newClientId;
                newSharingClients[i].clientIdHash = iett_generateClientIdHash(newClientId);
                newSharingClients[i].subOptions = pCreateSubInfo->sharingClientSubOpts[i] & ismENGINE_SUBSCRIPTION_OPTION_SHARING_CLIENT_PERSISTENT_MASK;
                newSharingClients[i].subId = pCreateSubInfo->sharingClientSubIds[i];
            }

            info.sharedSubData->sharingClients = newSharingClients;
            info.sharedSubData->sharingClientMax = newSharingClientMax;
            info.sharedSubData->sharingClientCount = pCreateSubInfo->sharingClientCount;
        }
    }

    // Tell the queue that it is part of an import request
    if ((info.internalAttrs & iettSUBATTR_IMPORTING) != 0)
    {
        info.queueOptions |= ieqOptions_IMPORTING;
    }

    // If this is a persistent subscription add it into the store.
    if (info.persistent == true)
    {
        rc = iecs_incrementDurableObjectCount(pThreadData, info.clientInfo.owningClient);

        if (rc != OK) goto mod_exit;

        info.incrementedDOC = true;

        assert(clientIdLength != 0);
        assert(subNameLength != 0);

        rc = iest_storeSubscription(pThreadData,
                                    info.topicString,
                                    info.topicStringLength,
                                    info.subscription,
                                    clientIdLength,
                                    subNameLength,
                                    flatSubPropertiesLength,
                                    info.queueType,
                                    info.policyInfo,
                                    &info.hStoreDefn,
                                    &info.hStoreProps);

        if (rc != OK) goto mod_exit;

        assert(pAsyncInfo != NULL);

        // Set up ready to go asynchronous
        ismEngine_AsyncDataEntry_t topicStringAsyncEntry = { ismENGINE_ASYNCDATAENTRY_STRUCID
                                                           , TopicCreateSubscriptionTopicString
                                                           , (char *)info.topicString, info.topicStringLength + 1
                                                           , NULL
                                                           , {.internalFn = iett_asyncCreateSubscriptionPhase2}};

        ismEngine_AsyncDataEntry_t callbackAsyncEntry = { ismENGINE_ASYNCDATAENTRY_STRUCID
                                                        , TopicCreateSubscriptionPhaseInfo
                                                        , &info, sizeof(info)
                                                        , NULL
                                                        , {.internalFn = iett_asyncCreateSubscriptionPhase2}};

        iead_pushAsyncCallback(pThreadData, pAsyncInfo, &topicStringAsyncEntry);
        info.asyncStackIndex = pAsyncInfo->numEntriesUsed;

        iead_pushAsyncCallback(pThreadData, pAsyncInfo, &callbackAsyncEntry);

        rc = iead_store_asyncCommit(pThreadData, false, pAsyncInfo);

        if (rc != ISMRC_AsyncCompletion)
        {
            // This phase didn't go async, but phase2 might and we want it to call phase3
            pAsyncInfo->entries[info.asyncStackIndex-1].pCallbackFn.internalFn = iett_asyncCreateSubscriptionPhase3;
            pAsyncInfo->entries[info.asyncStackIndex].pCallbackFn.internalFn = iett_asyncCreateSubscriptionPhase3;
        }
    }

mod_exit:

    if (rc != ISMRC_AsyncCompletion)
    {
        rc = iett_createSubscriptionPhase2(pThreadData, rc, pAsyncInfo, &info);
    }
    else
    {
        assert(pAsyncInfo != NULL);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, subscription=%p\n", __func__,
               rc, info.subscription);

    return rc;
}

//****************************************************************************
/// @brief Get the topic string from a given subscription
///
/// @param[in]     subscription  Subscription from which to get topic string
///
/// @return pointer to the subscription's associated topic string
//****************************************************************************
char *iett_getSubscriptionTopic(ismEngine_Subscription_t *subscription)
{
    assert(subscription != NULL);
    assert(subscription->node != NULL);
    assert(((iettSubsNode_t *)(subscription->node))->topicString != NULL);

    return(((iettSubsNode_t *)(subscription->node))->topicString);
}

//****************************************************************************
// @brief  Create Subscription
//
// Creates a subscription for a topic. The durability, reliability of message
// delivery and whether the subscription should be shared or single consumer
// only are also specified in the subOptions.
//
// @param[in]     hRequestingClient    Requesting Client handle
// @param[in]     pSubName             Subscription name
// @param[in]     pSubProperties       Optional context to associate with this subscription
// @param[in]     destinationType      Destination type - must be a TOPIC
// @param[in]     pDestinationName     Destination name - must be a topic string
// @param[in]     pSubAttributes       Attributes to use for this subscription
// @param[in]     hOwningClient        Owning Client handle, may be NULL
// @param[in]     pContext             Optional context for completion callback
// @param[in]     contextLength        Length of data pointed to by pContext
// @param[in]     pCallbackFn          Operation-completion callback
//
// @return OK on successful completion or an ISMRC_ value.
//
// @remark Two client handles are passed into this call one is the requesting
// client, the other is the client that will own the subscription. For non-shared
// durable subscriptions, or for those sharable between sessions of an individual
// client, these are expected to be the same client. For globally shared
// subscriptions, these may differ.
//
// If an owning client is not specified (NULL is passed) the owner is assumed to
// be the same as the requester.
//
// @remark The operation may complete synchronously or asynchronously. If it
// completes synchronously, the return code indicates the completion
// status.
//
// If the operation completes asynchronously, the return code from
// this function will be ISMRC_AsyncCompletion. The actual
// completion of the operation will be signalled by a call to the
// operation-completion callback, if one is provided. When the
// operation becomes asynchronous, a copy of the context will be
// made into memory owned by the Engine. This will be freed upon
// return from the operation-completion callback. Because the
// completion is asynchronous, the call to the operation-completion
// callback might occur before this function returns.
//
// The resulting subscription is a destination which can be used to
// create a message consumer.
//
// To consume from a named subscription, use ism_engine_createConsumer
// specifying a destinationType of ismDESTINATION_SUBSCRIPTION,
// a destinationName of the subname and the owning client handle.
//
// @see ism_engine_destroySubscription
// @see ism_engine_createConsumer
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_createSubscription(
    ismEngine_ClientStateHandle_t             hRequestingClient,
    const char *                              pSubName,
    const ism_prop_t *                        pSubProperties,
    uint8_t                                   destinationType,
    const char *                              pDestinationName,
    const ismEngine_SubscriptionAttributes_t *pSubAttributes,
    ismEngine_ClientStateHandle_t             hOwningClient,
    void *                                    pContext,
    size_t                                    contextLength,
    ismEngine_CompletionCallback_t            pCallbackFn)
{
    int32_t rc = OK;
    ismEngine_ClientState_t *pRequestingClient = (ismEngine_ClientState_t *)hRequestingClient;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pRequestingClient);
    ismEngine_ClientState_t *pOwningClient = (ismEngine_ClientState_t *)hOwningClient;

    assert(pRequestingClient != NULL);
    assert(pRequestingClient->pClientId != NULL);
    assert(pSubAttributes != NULL);

    iettCreateSubscriptionInfo_t createSubInfo = {{pSubProperties}, 0,
                                                  pSubName,
                                                  NULL,
                                                  pDestinationName,
                                                  pSubAttributes->subOptions,
                                                  pSubAttributes->subId,
                                                  iettSUBATTR_NONE,
                                                  iettNO_ANONYMOUS_SHARER,
                                                  0, NULL, NULL, NULL};

    // A durable subscription or a shared subscription that can be shared between
    // durable and non durable consumers is expected to be persistent, anything else isn't.
    if ((createSubInfo.subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE) != 0 ||
        (createSubInfo.subOptions & iettREQUIRE_PERSISTENT_SHARED_MASK) == iettREQUIRE_PERSISTENT_SHARED_MASK)
    {
        createSubInfo.internalAttrs |= iettSUBATTR_PERSISTENT;
    }
    else
    {
        assert((createSubInfo.internalAttrs & iettSUBATTR_PERSISTENT) == 0);
    }

    // If only one client was specified use it as the owner too
    if (pOwningClient == NULL)
    {
        pOwningClient = pRequestingClient;
    }
    else
    {
        if (pRequestingClient != pOwningClient)
        {
            assert((createSubInfo.subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) != 0);
            createSubInfo.internalAttrs |= iettSUBATTR_GLOBALLY_SHARED;
        }

        assert(pOwningClient->pClientId != NULL);
    }

    assert(destinationType == ismDESTINATION_TOPIC);
    assert(createSubInfo.topicString != NULL);
    assert(createSubInfo.subName != NULL);

    ieutTRACEL(pThreadData, pOwningClient, ENGINE_CEI_TRACE, FUNCTION_ENTRY "hRequestingClient=%p, subName='%s', hOwningClient=%p\n", __func__,
               hRequestingClient, createSubInfo.subName, hOwningClient);

    // Check with security before we do anything else

    // Acquire a reference to the requesting client-state to ensure it doesn't get deallocated
    // until the subscription has been created.
    rc = iecs_acquireClientStateReference(pRequestingClient);

    if (rc != OK) goto mod_exit;

    rc = ismEngine_security_validate_policy_func(pRequestingClient->pSecContext,
                                                 ismSEC_AUTH_TOPIC,
                                                 createSubInfo.topicString,
                                                 ismSEC_AUTH_ACTION_SUBSCRIBE,
                                                 ISM_CONFIG_COMP_ENGINE,
                                                 (void **)&(createSubInfo.policyInfo));

    // For a persistent globally shared subscription check for Control authority on the
    // subscription name.
    // Note: This will override the validated policy used in creating the subscription
    if (rc == OK &&
        (createSubInfo.subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) != 0 &&
        (createSubInfo.internalAttrs & iettSUBATTR_PERSISTENT) != 0 &&
        (createSubInfo.internalAttrs & iettSUBATTR_GLOBALLY_SHARED) != 0)
    {
        rc = iepi_validateSubscriptionPolicy(pThreadData,
                                             pRequestingClient->pSecContext,
                                             createSubInfo.subOptions,
                                             createSubInfo.subName,
                                             ismSEC_AUTH_ACTION_CONTROL,
                                             (void **)&(createSubInfo.policyInfo));
    }

    // If createSubInfo.policyInfo is NULL at this point the default policy info will be used

    // Acquire a reference to the owning client-state for globally shared subscriptions
    if (rc == OK && (createSubInfo.internalAttrs & iettSUBATTR_GLOBALLY_SHARED) != 0)
    {
        rc = iecs_acquireClientStateReference(pOwningClient);
    }

    if (rc != OK)
    {
        iecs_releaseClientStateReference(pThreadData, pRequestingClient, false, false);
        goto mod_exit;
    }

    iettCreateSubscriptionClientInfo_t clientInfo = {pRequestingClient, pOwningClient, true};

    ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
        {ismENGINE_ASYNCDATAENTRY_STRUCID, TopicCreateSubscriptionClientInfo,
            &clientInfo, sizeof(clientInfo), NULL,
            {.internalFn = iett_asyncCreateSubscriptionReleaseClients}},
       {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineCaller,
            pContext, contextLength, NULL,
            {.externalFn = pCallbackFn }}};

    ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID,
                                       pRequestingClient,
                                       IEAD_MAXARRAYENTRIES, 2, 0, true,  0, 0, asyncArray};

    // Decide if we are creating an anonymously shared subscription or not
    if (createSubInfo.subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED)
    {
        if (createSubInfo.subOptions & ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT)
        {
            createSubInfo.sharingClientCount = 1;
            createSubInfo.sharingClientIds = (const char **)(&pRequestingClient->pClientId);
            createSubInfo.sharingClientSubOpts = &createSubInfo.subOptions;
            createSubInfo.sharingClientSubIds = &createSubInfo.subId;
            assert(createSubInfo.anonymousSharer == iettNO_ANONYMOUS_SHARER);
        }
        else
        {
            uint8_t anonymousSharer = iettANONYMOUS_SHARER_JMS_APPLICATION;

            assert(createSubInfo.sharingClientCount == 0);
            createSubInfo.anonymousSharer = anonymousSharer;
        }
    }

    rc = iett_createSubscription(pThreadData,
                                 &clientInfo,
                                 &createSubInfo,
                                 NULL,
                                 &asyncData);

    if (rc != ISMRC_AsyncCompletion)
    {
        iett_createSubscriptionReleaseClients(pThreadData, &clientInfo);
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
// @brief  Reuse an existing Subscription
//
// Indicates to the engine that an existing subscription is being reused by the
// requesting client, enabling the engine to perform any usage counting or authorization
// required, and to update any required stored state due to altered attributes.
//
// @param[in]     hRequestingClient    Requesting Client handle
// @param[in]     pSubName             Subscription name
// @param[in]     pSubAttributes       Subscription attributes
// @param[in]     hOwningClient        Owning Client handle
//
// @return OK on successful completion or an ISMRC_ value.
//
// @remark Two client handles are passed into this call one is the requesting
// client, the other is the client that owns the shared subscription
//
// Both must be specified.
//
// The SubId can be updated to any value, however, the SubOptions that can
// be altered for a sharing client is limited to the set defined in
// ismENGINE_SUBSCRIPTION_OPTION_SHARING_CLIENT_ALTERABLE_MASK. Any attempt
// to modify options outside of this mask will result in ISMRC_InvalidParameter
// being returned.
//
// @see ism_engine_createSubscription
// @see ism_engine_createConsumer
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_reuseSubscription(
    ismEngine_ClientStateHandle_t             hRequestingClient,
    const char *                              pSubName,
    const ismEngine_SubscriptionAttributes_t *pSubAttributes,
    ismEngine_ClientStateHandle_t             hOwningClient)
{
    int32_t rc = OK;
    ismEngine_ClientState_t *pRequestingClient = (ismEngine_ClientState_t *)hRequestingClient;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pRequestingClient);
    ismEngine_ClientState_t *pOwningClient = (ismEngine_ClientState_t *)hOwningClient;
    ismEngine_Subscription_t *subscription = NULL;
    iettSharedSubData_t *sharedSubData = NULL;
    bool fHaveOwningClientStateReference = false;
    DEBUG_ONLY int osrc;

    assert(pRequestingClient != NULL);
    assert(pOwningClient != NULL);
    assert(pSubName != NULL);
    assert(pSubAttributes != NULL);

    uint32_t subOptions = pSubAttributes->subOptions;

    ieutTRACEL(pThreadData, hRequestingClient, ENGINE_CEI_TRACE, FUNCTION_ENTRY "hRequestingClient=%p, pSubName='%s', hOwningClient=%p\n", __func__,
               hRequestingClient, pSubName, hOwningClient);

    // Acquire a reference to the owning client-state
    rc = iecs_acquireClientStateReference(pOwningClient);

    if (rc != OK) goto mod_exit;

    fHaveOwningClientStateReference = true;

    // Check that the specified subscription does exist
    rc = iett_findClientSubscription(pThreadData,
                                     pOwningClient->pClientId,
                                     iett_generateClientIdHash(pOwningClient->pClientId),
                                     pSubName,
                                     iett_generateSubNameHash(pSubName),
                                     &subscription);

    // There was a problem, exit without releasing the lock on the subscription
    if (rc != OK)
    {
        // We fully expected to find this subscription
        if (rc == ISMRC_NotFound)
        {
            ieutTRACEL(pThreadData, 0, ENGINE_WORRYING_TRACE, "No subscription named '%s' found\n", pSubName);
        }
        goto mod_exit;
    }

    // Ensure that the durability of the subOptions matches the durability of subscription (if it should)
    if ((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE) !=
        (subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE))
    {
        // If the subscription's subOptions state that it supports mixed durability, then
        // this doesn't constitute a failure, but it gives us a chance to assert some things.
        if ((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY) != 0)
        {
            assert((subscription->internalAttrs & iettSUBATTR_PERSISTENT) != 0);
            assert((subscription->internalAttrs & iettSUBATTR_GLOBALLY_SHARED) != 0);
            assert((subOptions & ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT) != 0);
            assert((subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY) != 0);
        }
        else
        {
            ieutTRACE_FFDC(ieutPROBE_001, true, "Subscription's durability does not match reused durability", ISMRC_Error,
                           "subscription", subscription, iere_usable_size(iemem_subsTree, subscription),
                           "subOptions", &subOptions, sizeof(subOptions),
                           "pRequestingClient", pRequestingClient, iere_usable_size(iemem_clientState, pRequestingClient),
                           NULL);
        }
    }

    // Check that the requesting client has required authority

    // Acquire a reference to the requesting client-state to ensure it doesn't
    // get deallocated under us while performing an authority check
    rc = iecs_acquireClientStateReference(pRequestingClient);

    if (rc != OK) goto mod_exit;

    rc = ismEngine_security_validate_policy_func(pRequestingClient->pSecContext,
                                                 ismSEC_AUTH_TOPIC,
                                                 subscription->node->topicString,
                                                 ismSEC_AUTH_ACTION_SUBSCRIBE,
                                                 ISM_CONFIG_COMP_ENGINE,
                                                 NULL);

    // For a global shared persistent subscription (requesting and owning clients differ)
    // check for Receive authority on the subscription name.
    if (rc == OK &&
        (subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) != 0 &&
        (subscription->internalAttrs & iettSUBATTR_PERSISTENT) != 0 &&
        strcmp(pRequestingClient->pClientId, subscription->clientId) != 0)
    {
        rc = iepi_validateSubscriptionPolicy(pThreadData,
                                             pRequestingClient->pSecContext,
                                             subscription->subOptions,
                                             subscription->subName,
                                             ismSEC_AUTH_ACTION_RECEIVE,
                                             NULL);
    }

    iecs_releaseClientStateReference(pThreadData, pRequestingClient, false, false);

    if (rc != OK) goto mod_exit;

    sharedSubData = iett_getSharedSubData(subscription);

    bool needPersistentUpdate;

    if (sharedSubData != NULL)
    {
        // Get the lock on the shared subscription
        osrc = pthread_spin_lock(&sharedSubData->lock);
        assert(osrc == 0);

        // The subscription has been logically deleted, return as not found
        if ((subscription->internalAttrs & iettSUBATTR_DELETED) != 0)
        {
            rc = ISMRC_NotFound;
            goto mod_exit;
        }

        // Actually share the subscription
        if ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT) != 0)
        {
            rc = iett_shareSubscription(pThreadData,
                                        pRequestingClient->pClientId,
                                        iettNO_ANONYMOUS_SHARER,
                                        subscription,
                                        pSubAttributes,
                                        &needPersistentUpdate);
        }
        else
        {
            uint8_t anonymousSharer = iettANONYMOUS_SHARER_JMS_APPLICATION;

            rc = iett_shareSubscription(pThreadData,
                                        NULL,
                                        anonymousSharer,
                                        subscription,
                                        pSubAttributes,
                                        &needPersistentUpdate);
        }

        if (rc != OK) goto mod_exit;
    }
    else
    {
        // This function should only be called for shared subscriptions
        rc = ISMRC_InvalidOperation;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Need to update the subscription in the store if it had a persistent change and we have finished recovery
    if (needPersistentUpdate == true &&
        ismEngine_serverGlobal.runPhase > EnginePhaseRecovery)
    {
        ismStore_Handle_t hNewSubscriptionProps = ismSTORE_NULL_HANDLE;

        // Only expect persistent subscriptions to require a persistent update
        assert((subscription->internalAttrs & iettSUBATTR_PERSISTENT) != 0);

        rc = iest_updateSubscription(pThreadData,
                                     ieq_getPolicyInfo(subscription->queueHandle),
                                     subscription,
                                     ieq_getDefnHdl(subscription->queueHandle),
                                     ieq_getPropsHdl(subscription->queueHandle),
                                     &hNewSubscriptionProps,
                                     true);

        if (rc != OK) goto mod_exit;

        assert(hNewSubscriptionProps != ismSTORE_NULL_HANDLE);

        ieq_setPropsHdl(subscription->queueHandle, hNewSubscriptionProps);
    }

mod_exit:

    if (subscription != NULL)
    {
        // If we have locked the shared subscription, unlock it.
        if (sharedSubData != NULL)
        {
            #ifndef NDEBUG
            osrc = pthread_spin_unlock(&sharedSubData->lock);
            assert(osrc == 0);
            #else
            (void)pthread_spin_unlock(&sharedSubData->lock);
            #endif
        }

        (void)iett_releaseSubscription(pThreadData, subscription, false); // needed for iett_findClientSubscription
    }

    if (fHaveOwningClientStateReference)
    {
        iecs_releaseClientStateReference(pThreadData, pOwningClient, false, false);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @brief  Internal list subscriptions functon
///
/// List all of the named subscriptions owned by a given client and with
/// matching required characteristics.
///
/// @param[in]     owningClientId Client Id of the owning client state
/// @param[in]     flags          Flags controlling matching and required output
/// @param[in]     pSubName       Optional subscription name to match
/// @param[in]     pContext       Optional context for subscription callback
/// @param[in]     pCallbackFn    Internal subscription callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The subscriptions owned by the client specified
/// are enumerated, and for each that matches the criteria the
/// specified callback function is called.
///
/// The callback receives the subscription properties that were associated
/// at create subscription time, as well as context and subscription name.
//****************************************************************************
int32_t iett_listSubscriptions(ieutThreadData_t *pThreadData,
                               const char * owningClientId,
                               const uint32_t flags,
                               const char * pSubName,
                               void * pContext,
                               iett_listSubscriptionCallback_t pCallbackFn)
{
    int32_t rc = OK;
    int32_t subscriptionCount = 0;
    ismEngine_Subscription_t *thisSub = NULL;
    ismEngine_Subscription_t **subscriptions = &thisSub;

    assert(owningClientId != NULL);
    assert(((flags & iettFLAG_LISTSUBS_MATCH_SUBNAME) == 0) || pSubName != NULL);

    ieutTRACEL(pThreadData, flags, ENGINE_FNC_TRACE, FUNCTION_ENTRY "owningClientId='%s', matchFlags=0x%08x, pSubName='%s'\n", __func__,
               owningClientId, flags, pSubName ? pSubName : "<NULL>");

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    const uint32_t clientIdHash = iett_generateClientIdHash(owningClientId);

    // If we're matching the SUBNAME then we don't need to iterate over all entries
    if ((flags & iettFLAG_LISTSUBS_MATCH_OPTIONS_MASK) == iettFLAG_LISTSUBS_MATCH_SUBNAME)
    {
        const uint32_t subNameHash = (pSubName == NULL) ? 0 : iett_generateSubNameHash(pSubName);

        rc = iett_findClientSubscription(pThreadData,
                                         owningClientId,
                                         clientIdHash,
                                         pSubName,
                                         subNameHash,
                                         &thisSub);

        if (rc == OK) subscriptionCount = 1;
    }
    else
    {
        // Current code deals with subname matches separately -- need more logic in this
        // loop if we want to include subname matches here too...
        assert((flags & iettFLAG_LISTSUBS_MATCH_SUBNAME) == 0);

        iettClientSubscriptionList_t *clientNamedSubs = NULL;

        // Take the read lock so that we can access the named subscription hash.
        ismEngine_getRWLockForRead(&tree->namedSubsLock);

        rc = ieut_getHashEntry(tree->namedSubs,
                               owningClientId,
                               clientIdHash,
                               (void **)&clientNamedSubs);

        if (rc == OK)
        {
            // Allocate enough memory for a list of subscription pointers
            subscriptions = iemem_malloc(pThreadData,
                                         IEMEM_PROBE(iemem_callbackContext, 8),
                                         clientNamedSubs->count * sizeof(ismEngine_Subscription_t *));

            if (NULL == subscriptions)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            }
        }

        if (rc == OK)
        {
            // Go through each of the subscriptions incrementing the use count and
            // adding it to our list
            for(int32_t i=0; i<clientNamedSubs->count; i++)
            {
                thisSub = clientNamedSubs->list[i].subscription;

                // Subscriptions where disconnected client notification is needed
                if (flags & iettFLAG_LISTSUBS_MATCH_DCNMSGNEEDED)
                {
                    // We don't give shared subscriptions disconnected client notifications.
                    //
                    // Note: DCNEnabled should never be set for a shared subscription because a
                    // subscription messaging policy should not allow it to be set, but we can save
                    // the lookup of the policy info by checking if this is a shared subscription
                    // up front.
                    if (thisSub->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED)
                    {
                        continue;
                    }

                    iepiPolicyInfo_t *policyInfo = ieq_getPolicyInfo(thisSub->queueHandle);

                    assert(policyInfo != NULL);

                    // We're only interested in subscriptions that have disconnected client notification
                    // enabled
                    if (policyInfo->DCNEnabled == false)
                    {
                        continue;
                    }

                    ismEngine_QueueStatistics_t stats;

                    // Get queue statistics, specifically requesting activity stats
                    ieq_getStats(pThreadData, thisSub->queueHandle, &stats);

                    // There have been no new puts attempted on this queue, we are not interested
                    if (stats.PutsAttemptedDelta == 0)
                    {
                        continue;
                    }
                }

                subscriptions[subscriptionCount] = thisSub;
                (void)__sync_fetch_and_add(&subscriptions[subscriptionCount]->useCount, 1);
                subscriptionCount++;
            }
        }

        ismEngine_unlockRWLock(&tree->namedSubsLock);
    }

    if (rc == ISMRC_NotFound)
    {
        assert(subscriptions == &thisSub);

        rc = OK; // Caller doesn't need to know this
    }
    else if (rc == OK)
    {
        ism_prop_t *subProperties = (flags & iettFLAG_LISTSUBS_OUTPUT_PROPERTIES) ? ism_common_newProperties(2) : NULL;

        for(int32_t i=0; i<subscriptionCount; i++)
        {
            ism_prop_t *useProperties = NULL;

            thisSub = subscriptions[i];

            // If we need to reconstitute properties for the callback do so now
            if (subProperties != NULL && thisSub->flatSubPropertiesLength != 0)
            {
                concat_alloc_t flatBuf = { thisSub->flatSubProperties
                                         , thisSub->flatSubPropertiesLength
                                         , thisSub->flatSubPropertiesLength
                                         , 0
                                         , false };

                if (ism_common_deserializeProperties(&flatBuf, subProperties) == OK)
                {
                    useProperties = subProperties;
                }
            }

            ismEngine_SubscriptionAttributes_t subAttributes = { thisSub->subOptions , thisSub->subId };

            // This must be a shared subscription on another client's list...
            // In this case, we want to return the sub attributes for that specific client
            if (thisSub->clientIdHash != clientIdHash || strcmp(thisSub->clientId, owningClientId) != 0)
            {
                iettSharedSubData_t *sharedSubData = iett_getSharedSubData(thisSub);

                assert(sharedSubData != NULL);

                DEBUG_ONLY int osrc = pthread_spin_lock(&sharedSubData->lock);
                assert(osrc == 0);

                uint32_t index = 0;

                for(; index<sharedSubData->sharingClientCount; index++)
                {
                    if (sharedSubData->sharingClients[index].clientIdHash == clientIdHash &&
                        !strcmp(sharedSubData->sharingClients[index].clientId, owningClientId))
                    {
                        // Found a match
                        break;
                    }
                }

                // Don't expect not to find information, just use the values from the subscription
                if (index == sharedSubData->sharingClientCount)
                {
                    ieutTRACEL(pThreadData, sharedSubData, ENGINE_WORRYING_TRACE,
                               "Shared sub found for client %s with no info in shared sub.", owningClientId);
                }
                // Pick the subOptions associated with this clientId.
                else
                {
                    subAttributes.subOptions = sharedSubData->sharingClients[index].subOptions;
                    subAttributes.subId = sharedSubData->sharingClients[index].subId;
                }

                osrc = pthread_spin_unlock(&sharedSubData->lock);
                assert(osrc == 0);
            }

            assert((thisSub->subOptions & ~ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK) == 0);

            // Call the callback function
            pCallbackFn(pThreadData,
                        thisSub,
                        thisSub->subName,
                        thisSub->node->topicString,
                        useProperties,
                        0,
                        &subAttributes,
                        thisSub->consumerCount,
                        pContext);

            if (useProperties != NULL)
            {
                ism_common_clearProperties(subProperties);
            }

            (void)iett_releaseSubscription(pThreadData, thisSub, false);
        }

        if (subProperties != NULL) ism_common_freeProperties(subProperties);

        if (subscriptions != &thisSub)
        {
            iemem_free(pThreadData, iemem_callbackContext, subscriptions);
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief Internal callback for external list subscriptions calls
///
/// @remark The external callback functions for ism_engine_listSubscription
/// take less parameters than the internal function (iett_listSubscriptions)
/// wants to provide. This function is an internal callback function which
/// will subsequently call the external function without passing on the
/// internal flags.
//****************************************************************************
typedef struct tag_iettEngineListSubscriptionsCallbackContext_t
{
    ismEngine_SubscriptionCallback_t pCallbackFn;
    void *pContext;
} iettEngineListSubscriptionsCallbackContext_t;

void iett_engine_listSubscriptionsCallback(ieutThreadData_t                         *pThreadData,
                                           ismEngine_SubscriptionHandle_t            subHandle,
                                           const char *                              pSubName,
                                           const char *                              pTopicString,
                                           void *                                    properties,
                                           size_t                                    propertiesLength,
                                           const ismEngine_SubscriptionAttributes_t *pSubAttributes,
                                           uint32_t                                  consumerCount,
                                           void *                                    pContext)
{
    iettEngineListSubscriptionsCallbackContext_t *internalContext = (iettEngineListSubscriptionsCallbackContext_t *)pContext;

    internalContext->pCallbackFn(subHandle,
                                 pSubName,
                                 pTopicString,
                                 properties,
                                 propertiesLength,
                                 pSubAttributes,
                                 consumerCount,
                                 internalContext->pContext);
}

//****************************************************************************
/// @internal
///
/// @brief  List Subscriptions
///
/// List all of the named subscriptions owned by a given client.
///
/// @param[in]     hOwningClient  Owning client handle
/// @param[in]     pSubName       Optional subscription name to match
/// @param[in]     pContext       Optional context for subscription callback
/// @param[in]     pCallbackFn    Subscription callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark The named subscriptions owned by the client specified
/// are enumerated, and for each the specified callback function is called.
///
/// @remark If a subscription name is specified, only subscriptions matching
///         that name (for the specified client id) will be returned.
///
/// The callback receives the subscription properties that were associated
/// at create subscription time, as well as context and subscription name.
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_listSubscriptions(
                                          ismEngine_ClientStateHandle_t    hOwningClient,
                                          const char *                     pSubName,
                                          void *                           pContext,
                                          ismEngine_SubscriptionCallback_t pCallbackFn)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hOwningClient,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "hOwningClient=%p\n", __func__, hOwningClient);

    // Acquire a reference to the client-state to ensure it doesn't get deallocated under us
    ismEngine_ClientState_t *pOwningClient = (ismEngine_ClientState_t *)hOwningClient;

    rc = iecs_acquireClientStateReference(pOwningClient);

    if (rc != OK) goto mod_exit;

    iettEngineListSubscriptionsCallbackContext_t internalContext = { pCallbackFn, pContext };

    // Call the internal routine to perform the listing
    rc = iett_listSubscriptions(pThreadData,
                                pOwningClient->pClientId,
                                iettFLAG_LISTSUBS_OUTPUT_PROPERTIES |
                                (pSubName ? iettFLAG_LISTSUBS_MATCH_SUBNAME : 0),
                                pSubName,
                                &internalContext,
                                iett_engine_listSubscriptionsCallback);

    iecs_releaseClientStateReference(pThreadData, pOwningClient, false, false);

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Destroy named Subscription
///
/// Destroy a named subscription previously created with
/// ism_engine_createSubscription.
///
/// @param[in]     hRequestingClient  Requesting client handle
/// @param[in]     pSubName           Subscription name
/// @param[in]     hOwningClient      Owning Client handle
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark There must be no active consumers on the subscription
/// in order for it to be successfully destroyed, if there are active
/// consumers, ISMRC_ActiveConsumers is returned.
///
/// The operation may complete synchronously or asynchronously. If it
/// completes synchronously, the return code indicates the completion
/// status.
///
/// If the operation completes asynchronously, the return code from
/// this function will be ISMRC_AsyncCompletion. The actual
/// completion of the operation will be signalled by a call to the
/// operation-completion callback, if one is provided. When the
/// operation becomes asynchronous, a copy of the context will be
/// made into memory owned by the Engine. This will be freed upon
/// return from the operation-completion callback. Because the
/// completion is asynchronous, the call to the operation-completion
/// callback might occur before this function returns.
///
/// @see ism_engine_createSubscription
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_destroySubscription(
                                            ismEngine_ClientStateHandle_t  hRequestingClient,
                                            const char *                   pSubName,
                                            ismEngine_ClientStateHandle_t  hOwningClient,
                                            void *                         pContext,
                                            size_t                         contextLength,
                                            ismEngine_CompletionCallback_t pCallbackFn)
{
    ismEngine_ClientState_t *pRequestingClient = (ismEngine_ClientState_t *)hRequestingClient;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pRequestingClient);
    int32_t rc = OK;
    ismEngine_ClientState_t  *pOwningClient = (ismEngine_ClientState_t *)hOwningClient;
    bool fHaveOwningClientStateReference = false;

    ieutTRACEL(pThreadData, hOwningClient, ENGINE_CEI_TRACE,
               FUNCTION_ENTRY "hRequestingClient=%p, pSubName='%s', hOwningClient=%p\n", __func__,
               hRequestingClient, pSubName, hOwningClient);

    // Acquire a reference to the client-state to ensure it doesn't get deallocated under us
    rc = iecs_acquireClientStateReference(pOwningClient);

    if (rc != OK)
    {
        goto mod_exit;
    }

    fHaveOwningClientStateReference = true;

    // Call the generic function using the clientId from our ismEngine_ClientState_t
    rc = iett_destroySubscriptionForClientId(pThreadData,
                                             pOwningClient->pClientId,
                                             pOwningClient,
                                             pSubName,
                                             hRequestingClient,
                                             iettSUB_DESTROY_OPTION_NONE,
                                             pContext,
                                             contextLength,
                                             pCallbackFn);

    // We fully expected to find this subscription
    if (rc == ISMRC_NotFound)
    {
        ieutTRACEL(pThreadData, hOwningClient, ENGINE_WORRYING_TRACE,
                   "No subscription named '%s' found\n", pSubName);
    }

mod_exit:

    if (fHaveOwningClientStateReference)
    {
        iecs_releaseClientStateReference(pThreadData, pOwningClient, false, false);
    }

    // The code to cope with async completion has been removed, make sure we don't go async!
    assert(rc != ISMRC_AsyncCompletion);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Update Named subscription
///
/// Update a named subscription previously created with
/// ism_engine_createSubscription.
///
/// @param[in]     hRequestingClient  Requesting client handle
/// @param[in]     pSubName           Subscription name
/// @param[in]     pPropertyUpdates   Policy properties to update
/// @param[in]     hOwningClient      Owning client handle
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark This function can be used to alter the policy based attributes
/// of any subscription using a unique policy (basically, those created
/// by MQConnectivity or a unit test). At the moment, the only policy
/// property that is expected to be altered is MaxMessages (which is the
/// maximum message count) which can be altered while messages are
/// flowing.
///
/// The pPropertyUpdate parameter can include other undocumented policy
/// properties, however, the only one that is currently supported is
/// MaxMessages and it is an error to include any others. If others
/// are specified, then this function will update them, however, the
/// subsequent behaviour of the engine is undefined -- again, these will
/// only be updated for subscriptions using a unique messaging policy.
///
/// In addition to the policy properties, there are other properties
/// that can be updated on any subscription (whether it has a unique
/// policy or not).
///
/// "SubId" (uint32_t) specifies an update to the subscription Id.
/// "SubOptions" (uint32_t) specifies replacement subOptions.
///
/// Only a subset of SubOptions is allowed to be altered, the subset is
/// defined by ismENGINE_SUBSCRIPTION_OPTION_ALTERABLE_MASK. Any attempt
/// to alter other options will result in this function returning
/// ISMRC_InvalidParameter.
///
/// The operation may complete synchronously or asynchronously. If it
/// completes synchronously, the return code indicates the completion
/// status.
///
/// If the operation completes asynchronously, the return code from
/// this function will be ISMRC_AsyncCompletion. The actual
/// completion of the operation will be signalled by a call to the
/// operation-completion callback, if one is provided. When the
/// operation becomes asynchronous, a copy of the context will be
/// made into memory owned by the Engine. This will be freed upon
/// return from the operation-completion callback. Because the
/// completion is asynchronous, the call to the operation-completion
/// callback might occur before this function returns.
///
/// @see ism_engine_createSubscription
/// @see ism_engine_destroySubscription
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_updateSubscription(
                                           ismEngine_ClientStateHandle_t  hRequestingClient,
                                           const char *                   pSubName,
                                           const ism_prop_t *             pPropertyUpdates,
                                           ismEngine_ClientStateHandle_t  hOwningClient,
                                           void *                         pContext,
                                           size_t                         contextLength,
                                           ismEngine_CompletionCallback_t pCallbackFn)
{
    ismEngine_ClientState_t *pRequestingClient = (ismEngine_ClientState_t *)hRequestingClient;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pRequestingClient);
    int32_t rc = OK;
    ismEngine_ClientState_t *pOwningClient = (ismEngine_ClientState_t *)hOwningClient;
    bool fHaveOwningClientStateReference = false;
    ismEngine_Subscription_t *subscription = NULL;
    iettSharedSubData_t *sharedSubData = NULL;
    ismStore_Handle_t hNewSubscriptionProps = ismSTORE_NULL_HANDLE;

    ieutTRACEL(pThreadData, hRequestingClient, ENGINE_CEI_TRACE,
               FUNCTION_ENTRY "hRequestingClient=%p, pSubName='%s', hOwningClient=%p\n", __func__,
               hRequestingClient, pSubName, hOwningClient);

    // Acquire a reference to the client-state to ensure it doesn't get deallocated under us
    rc = iecs_acquireClientStateReference(pOwningClient);

    if (rc != OK)
    {
        goto mod_exit;
    }

    fHaveOwningClientStateReference = true;

    // Find the named subscription in the hash table.
    rc = iett_findClientSubscription(pThreadData,
                                     pOwningClient->pClientId,
                                     iett_generateClientIdHash(pOwningClient->pClientId),
                                     pSubName,
                                     iett_generateSubNameHash(pSubName),
                                     &subscription);

    // There was a problem, exit without releasing the lock on the subscription
    if (rc != OK)
    {
        // We fully expected to find this subscription
        if (rc == ISMRC_NotFound)
        {
            ieutTRACEL(pThreadData, hOwningClient, ENGINE_WORRYING_TRACE,
                       "No subscription named '%s' found\n", pSubName);
        }
        goto mod_exit;
    }

    // Check with security before we do anything else
    rc = iecs_acquireClientStateReference(pRequestingClient);

    if (rc == OK)
    {
        // If this is a global shared persistent subscription check that the requesting client
        // has the authority to update the subscription
        if ((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) != 0 &&
            (subscription->internalAttrs & iettSUBATTR_PERSISTENT) != 0 &&
            strcmp(pRequestingClient->pClientId, subscription->clientId) != 0)
        {
            rc = iepi_validateSubscriptionPolicy(pThreadData,
                                                 pRequestingClient->pSecContext,
                                                 subscription->subOptions,
                                                 pSubName,
                                                 ismSEC_AUTH_ACTION_CONTROL,
                                                 NULL);
        }

        iecs_releaseClientStateReference(pThreadData, pRequestingClient, false, false);

        if (rc != OK) goto mod_exit;
    }

    sharedSubData = iett_getSharedSubData(subscription);

    // For a shared subscription, grab it's lock while we update it
    if (sharedSubData != NULL)
    {
        DEBUG_ONLY int osrc = pthread_spin_lock(&sharedSubData->lock);
        assert(osrc == 0);

        // The subscription has been logically deleted, return as not found
        if ((subscription->internalAttrs & iettSUBATTR_DELETED) != 0)
        {
            rc = ISMRC_NotFound;
            goto mod_exit;
        }
    }

    bool subscriptionUpdated = false;

    iepiPolicyInfo_t *policyInfo = ieq_getPolicyInfo(subscription->queueHandle);

    // Check for modification of SubId
    ismEngine_SubId_t oldSubId = subscription->subId;
    ismEngine_SubId_t newSubId = ism_common_getUintProperty((struct ism_prop_t *)pPropertyUpdates,
                                                            ismENGINE_UPDATE_SUBSCRIPTION_PROPERTY_SUBID,
                                                            (uint32_t)oldSubId);

    if (newSubId != oldSubId)
    {
        ieutTRACEL(pThreadData, newSubId, ENGINE_HIGH_TRACE, "subId set to %u\n", newSubId);
        subscriptionUpdated = true;
    }

    // Check for modiciation of SubOptions
    uint32_t oldSubOptions = subscription->subOptions;
    uint32_t newSubOptions = ism_common_getUintProperty((struct ism_prop_t *)pPropertyUpdates,
                                                        ismENGINE_UPDATE_SUBSCRIPTION_PROPERTY_SUBOPTIONS,
                                                        oldSubOptions) & ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK;

    if (newSubOptions != oldSubOptions)
    {
        // Check that the only options being altered are in the alterable subset
        if ((newSubOptions & ~ismENGINE_SUBSCRIPTION_OPTION_ALTERABLE_MASK) !=
            (oldSubOptions & ~ismENGINE_SUBSCRIPTION_OPTION_ALTERABLE_MASK))
        {
            rc = ISMRC_InvalidParameter;
            ism_common_setError(rc);
            goto mod_exit;
        }

        ieutTRACEL(pThreadData, newSubOptions, ENGINE_HIGH_TRACE, "subOptions set to 0x%08x\n", newSubOptions);
        subscriptionUpdated = true;
    }

    // The only policies we expect to update via a call to ism_engine_updateSubscription are
    // unique policies - i.e. ones not shared with others for dynamic policy updates,
    if (policyInfo->name == NULL)
    {
        // We expect the pPropertyUpdates parameter to come from protocol and thus the
        // property name will not have a prefix (hence the NULL). If the admin
        // interface ever calls this function then something more sophisticated
        // will be required.
        rc = iepi_updatePolicyInfoFromProperties(pThreadData,
                                                 policyInfo,
                                                 NULL,
                                                 (struct ism_prop_t *) pPropertyUpdates,
                                                 subscriptionUpdated ? NULL : &subscriptionUpdated);

        if (rc != OK) goto mod_exit;
    }

    // Need to update actually update the subscription after recovery has completed
    if (subscriptionUpdated)
    {
        // We don't update the subscription that's being used until after we've updated the store.
        ismEngine_Subscription_t newSubscription = *subscription;

        newSubscription.subId = newSubId;
        newSubscription.subOptions = newSubOptions;

        // Update the store with the proposed changed subscription if persistent
        if ((subscription->internalAttrs & iettSUBATTR_PERSISTENT) != 0 &&
            ismEngine_serverGlobal.runPhase > EnginePhaseRecovery)
        {
            rc = iest_updateSubscription(pThreadData,
                                         policyInfo,
                                         &newSubscription,
                                         ieq_getDefnHdl(newSubscription.queueHandle),
                                         ieq_getPropsHdl(newSubscription.queueHandle),
                                         &hNewSubscriptionProps,
                                         true);

            if (rc != OK) goto mod_exit;

            assert(hNewSubscriptionProps != ismSTORE_NULL_HANDLE);

            ieq_setPropsHdl(newSubscription.queueHandle, hNewSubscriptionProps);
        }

        // Now update the live subscription with our proposed changes
        iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

        ismEngine_getRWLockForWrite(&tree->subsLock);

        // One of the subscriptions is changing, so invalidate any per-thread caches
        tree->subsUpdates++;

        subscription->subId = newSubscription.subId;
        oldSubOptions = subscription->subOptions;

        // Update the activeSelection count on the node if selection has changed
        if (newSubOptions != oldSubOptions)
        {
            if ((newSubOptions & ismENGINE_SUBSCRIPTION_OPTION_SELECTION_MASK) != 0 &&
                (oldSubOptions & ismENGINE_SUBSCRIPTION_OPTION_SELECTION_MASK) == 0)
            {
                subscription->node->activeSelection++;
            }
            else if ((newSubOptions & ismENGINE_SUBSCRIPTION_OPTION_SELECTION_MASK) == 0 &&
                     (oldSubOptions & ismENGINE_SUBSCRIPTION_OPTION_SELECTION_MASK) != 0)
            {
                subscription->node->activeSelection--;
            }
        }
        subscription->subOptions = newSubscription.subOptions;

        ismEngine_unlockRWLock(&tree->subsLock);
    }

mod_exit:

    if (subscription != NULL)
    {
        // Release the lock for a shared subscription
        if (sharedSubData != NULL)
        {
            DEBUG_ONLY int osrc = pthread_spin_unlock(&sharedSubData->lock);
            assert(osrc == 0);
        }

        (void)iett_releaseSubscription(pThreadData, subscription, false);
    }

    if (fHaveOwningClientStateReference)
    {
        iecs_releaseClientStateReference(pThreadData, pOwningClient, false, false);
    }

    // The code to cope with async completion has been removed, make sure we don't go async!
    assert(rc != ISMRC_AsyncCompletion);

    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @brief  Set the policy info associated with a specific subscription
///
/// Set the iepiPolicyInfo_t used by a subscription, updating the store for
/// a durable subscription if the policy changes.
///
/// @param[in]     pThreadData        Thread performing the request
/// @param[in]     subscription       The subscription being updated
/// @param[in]     policy             The policy being set
///
/// @remark The policy is not expected to have had it's use count increased,
/// this function will increase it if it is successfully set as the policy
/// on the queue for this subscription.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_setSubscriptionPolicyInfo(ieutThreadData_t *pThreadData,
                                       ismEngine_SubscriptionHandle_t subscription,
                                       iepiPolicyInfoHandle_t policy)
{
    int32_t rc = OK;
    bool policyInfoUpdated = false;

    ieutTRACEL(pThreadData, subscription, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "subscription=%p, policy=%p\n", __func__,
               subscription, policy);

    if (policy != NULL)
    {
        policyInfoUpdated = ieq_setPolicyInfo(pThreadData,
                                              (ismEngine_Queue_t *)(subscription->queueHandle),
                                              policy);

        if (policyInfoUpdated)
        {
            iepi_acquirePolicyInfoReference(policy);

            // Need to update the store for an updated persistent subscription after recovery has completed
            if ((subscription->internalAttrs && iettSUBATTR_PERSISTENT) != 0 &&
                ismEngine_serverGlobal.runPhase > EnginePhaseRecovery)
            {
                ismStore_Handle_t hNewSubscriptionProps = ismSTORE_NULL_HANDLE;

                rc = iest_updateSubscription(pThreadData,
                                             policy,
                                             subscription,
                                             ieq_getDefnHdl(subscription->queueHandle),
                                             ieq_getPropsHdl(subscription->queueHandle),
                                             &hNewSubscriptionProps,
                                             true);

                if (rc != OK) goto mod_exit;

                assert(hNewSubscriptionProps != ismSTORE_NULL_HANDLE);

                ieq_setPropsHdl(subscription->queueHandle, hNewSubscriptionProps);
            }
        }
    }

mod_exit:
    ieutTRACEL(pThreadData, policyInfoUpdated,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief  Destroy Named Subscription owned by a specified clientId
///
/// Destroy a named subscription owned by a specified clientId.
///
/// @param[in]     pThreadData        Thread data structure
/// @param[in]     pClientId          Client Id of the subscription creator
/// @param[in]     pClient            ClientState of the subscription owner (may be NULL for admin requests)
/// @param[in]     pSubName           Subscription name
/// @param[in]     pRequestingClient  Client requesting this deletion
/// @param[in]     subDestroyOptions  Options for this destroy request (iettSUB_DESTROY_OPTION_*)
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark For a global shared subscription pRequestingClient should be non-null
///         unless this is a request from admin.
//****************************************************************************
int32_t iett_destroySubscriptionForClientId(ieutThreadData_t *pThreadData,
                                            const char *pClientId,
                                            ismEngine_ClientState_t *pClient,
                                            const char *pSubName,
                                            ismEngine_ClientState_t *pRequestingClient,
                                            uint32_t subDestroyOptions,
                                            void *pContext,
                                            size_t contextLength,
                                            ismEngine_CompletionCallback_t pCallbackFn)
{
    int32_t rc = OK;
    ismEngine_Subscription_t *subscription = NULL;
    iettSharedSubData_t *sharedSubData = NULL;
    iettRelinquishInfo_t *needRelinquish = NULL;
    uint32_t needRelinquishCount = 0;
    bool fAcquiredRequestingClientReference = false;
    bool adminRequest = (pClient == NULL);
    bool decrementDurableCountByClient = !adminRequest;
    bool persistentSub = false;
    iereResourceSetHandle_t resourceSet;

    ieutTRACEL(pThreadData, pClient, ENGINE_CEI_TRACE, FUNCTION_ENTRY "pClientId='%s' pSubName='%s'\n", __func__,
               pClientId, pSubName);

    // For a non-admin request, make sure we have a reference on the requesting client
    if (!adminRequest)
    {
        assert(pRequestingClient != NULL);

        rc = iecs_acquireClientStateReference(pRequestingClient);

        if (rc != OK) goto mod_exit_no_release;

        fAcquiredRequestingClientReference = true;
    }

    // Find the named subscription in the hash table.
    rc = iett_findClientSubscription(pThreadData,
                                     pClientId,
                                     iett_generateClientIdHash(pClientId),
                                     pSubName,
                                     iett_generateSubNameHash(pSubName),
                                     &subscription);

    // There was a problem, exit without releasing the lock on the subscription
    if (rc != OK) goto mod_exit_no_release;

    // For requests coming from a REST API...
    if (adminRequest == true)
    {
        const char *configType = iett_getAdminSubscriptionType(subscription);

        // For a non admin subscription, make sure none of the admin subscription configuration
        // APIs are being used.
        if (configType == NULL)
        {
            if ((subDestroyOptions & iettSUB_DESTROY_OPTION_DESTROY_ADMINSUBSCRIPTION) != 0)
            {
                rc = ISMRC_WrongSubscriptionAPI;
                ism_common_setErrorData(rc, "%s", ismENGINE_ADMIN_VALUE_SUBSCRIPTION);
                goto mod_exit;
            }
        }
        // For an admin subscription, make sure an admin subscription call is being used
        else if ((subDestroyOptions & iettSUB_DESTROY_OPTION_DESTROY_ADMINSUBSCRIPTION) == 0)
        {
            rc = ISMRC_WrongSubscriptionAPI;
            ism_common_setErrorData(rc, "%s", configType);
            goto mod_exit;
        }
    }

    resourceSet = subscription->resourceSet;

    // Remember if this is a persistent subscription for later checking...
    persistentSub = (subscription->internalAttrs & iettSUBATTR_PERSISTENT) != 0;

    // We need to defer the authority check for shared subscriptions until we know
    // if this is just a removal of this client from the subscription or a generic
    // destroy of the subscription - so we use a temporary variable.
    int32_t authorityRC = OK;

    // Other than for admin requests and where we are removing a disconnected client which
    // must implicitly be an admin request - check security
    if (!adminRequest && pRequestingClient->pSecContext != NULL)
    {
        // If the requesting client is not the owning client, and this is a persistent global
        // shared subscription check that we have the authority to destroy the subscription
        if (persistentSub == true &&
            (subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) != 0 &&
            strcmp(pRequestingClient->pClientId, subscription->clientId) != 0)
        {
            authorityRC = iepi_validateSubscriptionPolicy(pThreadData,
                                                          pRequestingClient->pSecContext,
                                                          subscription->subOptions,
                                                          pSubName,
                                                          ismSEC_AUTH_ACTION_CONTROL,
                                                          NULL);
        }
    }

    uint32_t remainingSharers;

    // If we are dealing with a shared subscription, there is some additional work to do.
    sharedSubData = iett_getSharedSubData(subscription);

    bool subscriptionUpdated = false;

    if (sharedSubData)
    {
        DEBUG_ONLY int osrc = pthread_spin_lock(&sharedSubData->lock);
        assert(osrc == 0);

        // For an admin request...
        if (adminRequest)
        {
            // ...we either just remove the admin sharer...
            if ((subDestroyOptions & iettSUB_DESTROY_OPTION_ONLY_REMOVE_ANONYMOUS_SHARER_ADMIN) != 0)
            {
                sharedSubData->anonymousSharers &= ~iettANONYMOUS_SHARER_ADMIN;
                subscriptionUpdated = true;
            }
            // ...or remove the subscription entirely
            else
            {
                // Check that there are no active consumers on this subscription.
                if (subscription->consumerCount > 0)
                {
                    ieutTRACEL(pThreadData, subscription->consumerCount, ENGINE_NORMAL_TRACE,
                               "Subscription in use, consumerCount=%u\n", subscription->consumerCount);
                    rc = ISMRC_DestinationInUse;
                    goto mod_exit;
                }

                if (sharedSubData->anonymousSharers != iettNO_ANONYMOUS_SHARER)
                {
                    sharedSubData->anonymousSharers = iettNO_ANONYMOUS_SHARER;
                    subscriptionUpdated = true;
                }

                // If we have sharing client ids, remove this subscription tidy each one up
                if (sharedSubData->sharingClientCount != 0)
                {
                    needRelinquish = iemem_malloc(pThreadData,
                                                  IEMEM_PROBE(iemem_callbackContext, 12),
                                                  sizeof(iettRelinquishInfo_t) * (sharedSubData->sharingClientCount));

                    if (needRelinquish == NULL)
                    {
                        rc = ISMRC_AllocateError;
                        ism_common_setError(rc);
                        goto mod_exit;
                    }

                    // Run through the list in reverse order, so that if there is a
                    // problem half way through the list is still consistent
                    for(int32_t index=(int32_t)(sharedSubData->sharingClientCount-1); index >= 0; index--)
                    {
                        rc = iecs_findClientMsgDelInfo(pThreadData,
                                                       sharedSubData->sharingClients[index].clientId,
                                                       &needRelinquish[needRelinquishCount].hMsgDelInfo);

                        if (rc == OK)
                        {
                            if ((sharedSubData->sharingClients[index].subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK) ==
                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE)
                            {
                                needRelinquish[needRelinquishCount].relinquishType =
                                    ismEngine_RelinquishType_ACK_HIGHRELIABLITY;
                            }
                            else
                            {
                                needRelinquish[needRelinquishCount].relinquishType =
                                    ismEngine_RelinquishType_NACK_ALL;
                            }

                            needRelinquishCount++;
                        }
                        else
                        {
                            rc = OK;
                        }

                        // Remove the subscription from this client's list of subscriptions
                        iett_removeSubscription(pThreadData,
                                                subscription,
                                                sharedSubData->sharingClients[index].clientId,
                                                sharedSubData->sharingClients[index].clientIdHash);

                        iere_primeThreadCache(pThreadData, resourceSet);
                        iere_free(pThreadData, resourceSet, iemem_externalObjs, sharedSubData->sharingClients[index].clientId);
                        sharedSubData->sharingClients[index].clientId = NULL;

                        sharedSubData->sharingClientCount -= 1;
                    }

                    assert(sharedSubData->sharingClientCount == 0);
                    subscriptionUpdated = true;
                }
            }
        }
        else
        {
            assert(pRequestingClient != NULL);

            char *requestingClientId = pRequestingClient->pClientId;
            uint32_t requestingClientIdHash = iett_generateClientIdHash(requestingClientId);
            uint32_t index = 0;

            for(; index<sharedSubData->sharingClientCount; index++)
            {
                if (sharedSubData->sharingClients[index].clientIdHash == requestingClientIdHash &&
                    !strcmp(sharedSubData->sharingClients[index].clientId, requestingClientId))
                {
                    // Found a match
                    break;
                }
            }

            // If we found the client in the list, remove it and remove the subscription from it's list
            if (index < sharedSubData->sharingClientCount)
            {
                uint32_t subOptions = sharedSubData->sharingClients[index].subOptions;
                iettSharingClient_t *thisSharingClient = &sharedSubData->sharingClients[index];

                // Need to relinquish all messages on this queue for this client
                if (pRequestingClient->hMsgDeliveryInfo != NULL)
                {
                    needRelinquish = iemem_malloc(pThreadData,
                                                  IEMEM_PROBE(iemem_callbackContext, 13),
                                                  sizeof(iettRelinquishInfo_t));

                    if (needRelinquish == NULL)
                    {
                        rc = ISMRC_AllocateError;
                        ism_common_setError(rc);
                        goto mod_exit;
                    }

                    iecs_acquireMessageDeliveryInfoReference(pThreadData,
                                                             pRequestingClient,
                                                             &needRelinquish->hMsgDelInfo);

                    if ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK) == ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE)
                    {
                        needRelinquish->relinquishType =
                            ismEngine_RelinquishType_ACK_HIGHRELIABLITY;
                    }
                    else
                    {
                        needRelinquish->relinquishType =
                            ismEngine_RelinquishType_NACK_ALL;
                    }

                    needRelinquishCount = 1;
                }

                iere_primeThreadCache(pThreadData, resourceSet);
                iere_free(pThreadData, resourceSet, iemem_externalObjs, thisSharingClient->clientId);
                thisSharingClient->clientId = NULL;

                // Only need to make any changes durably for a durable sharer
                subscriptionUpdated = (thisSharingClient->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE) != 0;

                sharedSubData->sharingClientCount -= 1;

                // Move last entry to this entry
                if (index < sharedSubData->sharingClientCount)
                {
                    sharedSubData->sharingClients[index] = sharedSubData->sharingClients[sharedSubData->sharingClientCount];
                }

                // Remove the subscription from the requesting client's list of subscriptions
                iett_removeSubscription(pThreadData,
                                        subscription,
                                        requestingClientId,
                                        requestingClientIdHash);

                // We have removed ourselves from the subscription, whether the subscription
                // now disappears should not be based on our authority to control it - but
                // let's trace this decision out.
                if (authorityRC != OK)
                {
                    ieutTRACEL(pThreadData, authorityRC, ENGINE_NORMAL_TRACE,
                               "%s Ignoring authorityRC %d for clientId %s\n",
                               __func__, authorityRC, requestingClientId);

                    authorityRC = OK;
                }

                decrementDurableCountByClient = false;
            }
            else
            {
                // Need to be authorized to destroy the subscription
                if (authorityRC != OK)
                {
                    rc = authorityRC;
                    goto mod_exit;
                }

                uint8_t anonymousSharer = iettANONYMOUS_SHARER_JMS_APPLICATION;

                // FUTURE: Might change the type of anonymous sharer based on... something

                if ((sharedSubData->anonymousSharers & anonymousSharer) != 0)
                {
                    sharedSubData->anonymousSharers &= ~anonymousSharer;
                    subscriptionUpdated = true;
                }
            }
        }

        remainingSharers = sharedSubData->sharingClientCount +
                           ((sharedSubData->anonymousSharers == iettNO_ANONYMOUS_SHARER) ? 0 : 1);
    }
    // Not a shared subscription
    else
    {
        assert(authorityRC == OK); // Should not have done an authority check
        remainingSharers = 0;
    }

    // If we still have remaining sharers then there is no further work to do
    if (remainingSharers != 0)
    {
        ieutTRACEL(pThreadData, remainingSharers, ENGINE_NORMAL_TRACE,
                   "Sharers still exist, remainingSharers=%u\n", remainingSharers);

        // Need to update the subscription in the store if it's persistent, updated and we have finished recovery
        if (persistentSub == true &&
            subscriptionUpdated == true &&
            ismEngine_serverGlobal.runPhase > EnginePhaseRecovery)
        {
            ismStore_Handle_t hNewSubscriptionProps = ismSTORE_NULL_HANDLE;

            rc = iest_updateSubscription(pThreadData,
                                         ieq_getPolicyInfo(subscription->queueHandle),
                                         subscription,
                                         ieq_getDefnHdl(subscription->queueHandle),
                                         ieq_getPropsHdl(subscription->queueHandle),
                                         &hNewSubscriptionProps,
                                         true);

            if (rc != OK) goto mod_exit;

            assert(hNewSubscriptionProps != ismSTORE_NULL_HANDLE);

            ieq_setPropsHdl(subscription->queueHandle, hNewSubscriptionProps);
        }

        // Nothing more to do here
        goto mod_exit;
    }

    // Check that there are no active consumers on this subscription.
    if (subscription->consumerCount > 0)
    {
        ieutTRACEL(pThreadData, subscription->consumerCount, ENGINE_NORMAL_TRACE,
                   "Subscription in use, consumerCount=%u\n", subscription->consumerCount);
        rc = ISMRC_DestinationInUse;
        goto mod_exit;
    }

    // Check that this is not a logically deleted subscription
    if ((subscription->internalAttrs & iettSUBATTR_DELETED) != 0)
    {
        // Attempt to delete an already deleted subscription
        ieutTRACEL(pThreadData, subscription->internalAttrs, ENGINE_NORMAL_TRACE,
                   "Subscription already deleted (0x%08X)\n", subscription->internalAttrs);
        goto mod_exit;
    }

    // For non-shared MQTT subscriptions make sure that any inflight messages are still tracked.
    bool fCheckWhetherClientConnected = false;
    if (sharedSubData == NULL &&
        (subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE) == 0)
    {
        if (pRequestingClient != NULL)
        {
            assert(strcmp(pRequestingClient->pClientId, pClientId) == 0);

            iecs_trackInflightMsgs(pThreadData, pRequestingClient, subscription->queueHandle);
        }
        else
        {
            //We're doing an admin request, if the client is connected, we need to
            //track any inflight acks from this subscription. Otherwise we can just
            //forget about inflight messages
            fCheckWhetherClientConnected=true;
        }
    }

    // Flag this subscription as logically deleted
    subscription->internalAttrs |= iettSUBATTR_DELETED;

    // Decrement the useCount on this subscription from it's creation, which should
    // never be the last count on this subscription (because of our iett_findClientSubscription)
    DEBUG_ONLY uint32_t oldCount = __sync_fetch_and_sub(&subscription->useCount, 1);

    assert(oldCount > 1);

    // For a persistent subscription, mark the store definition record as deleted, so that
    // if the server restarts before we called free, it will be deleted at the end of startup.
    if (persistentSub == true)
    {
        ismStore_Handle_t hStoreDefn = ieq_getDefnHdl(subscription->queueHandle);

        assert(hStoreDefn != ismSTORE_NULL_HANDLE);

        rc = ism_store_updateRecord(pThreadData->hStream,
                                    hStoreDefn,
                                    ismSTORE_NULL_HANDLE,
                                    iestSDR_STATE_DELETED,
                                    ismSTORE_SET_STATE);
        if (rc == OK)
        {
            iest_store_commit(pThreadData, false);
        }
        else
        {
            assert(rc != ISMRC_StoreGenerationFull);
            iest_store_rollback(pThreadData, false);
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    // Remove the subscription from the list for the owning clientId
    iett_removeSubscription(pThreadData,
                            subscription,
                            subscription->clientId,
                            subscription->clientIdHash);

    if (fCheckWhetherClientConnected)
    {
       //Now the sub has been removed from the tree and no new clients can connect to it,
       //see whether any inflight messages need to be tracked..

       ismEngine_ClientState_t *pConnectedClient =NULL;

       iecs_findClientState( pThreadData
                           , subscription->clientId
                           , true
                           , &pConnectedClient);


       if (pConnectedClient != NULL)
       {
           iecs_trackInflightMsgs(pThreadData, pConnectedClient, subscription->queueHandle);
           iecs_releaseClientStateReference(pThreadData, pConnectedClient,false, false);
       }
       else
       {
           ieq_forgetInflightMsgs(pThreadData, subscription->queueHandle);
       }
    }

    // Decrement the durable object count for the client if it is a persistent subscription
    if (persistentSub == true)
    {
        if (decrementDurableCountByClient)
        {
            assert(pClient != NULL);
            iecs_decrementDurableObjectCount(pThreadData, pClient);
        }
        else
        {
            (void)iecs_updateDurableObjectCountForClientId(pThreadData,
                                                           subscription->clientId,
                                                           subscription->internalAttrs & iettSUBATTR_GLOBALLY_SHARED ?
                                                             PROTOCOL_ID_SHARED : PROTOCOL_ID_JMS,
                                                           false);
        }
    }

mod_exit:

    // If it's a shared subscription, we still have it locked
    if (sharedSubData != NULL)
    {
        DEBUG_ONLY int osrc = pthread_spin_unlock(&sharedSubData->lock);
        assert(osrc == 0);

        // We have some relinquishes to perform now that we've unlocked the subscription
        if (needRelinquishCount != 0)
        {
            assert(subscription != NULL);
            assert(needRelinquish != NULL);

            for(uint32_t i=0; i<needRelinquishCount; i++)
            {
                // Need to relinquish all messages on this queue for this client
                ieq_relinquishAllMsgsForClient( pThreadData
                                              , subscription->queueHandle
                                              , needRelinquish[i].hMsgDelInfo
                                              , needRelinquish[i].relinquishType);

                iecs_releaseMessageDeliveryInfoReference(pThreadData, needRelinquish[i].hMsgDelInfo);
            }

            iemem_free(pThreadData, iemem_callbackContext, needRelinquish);
        }
    }
    else
    {
        assert(needRelinquish == NULL);
        assert(needRelinquishCount == 0);
    }

    // Release our lock on the subscription - if it's been flagged as deleted
    // it may be freed after this call
    (void)iett_releaseSubscription(pThreadData, subscription, true);

mod_exit_no_release:

    if (fAcquiredRequestingClientReference == true)
    {
        iecs_releaseClientStateReference(pThreadData, pRequestingClient, false, false);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d, subscription=%p\n", __func__,
               rc, subscription);

    return rc;
}

//****************************************************************************
/// @brief Configuration callback to handle deletion of subscriptions
///
/// @param[in]     objectIdentifier  The property string identifier
/// @param[in]     changedProps      Properties qualified by string identifier
///                                  as a suffix
/// @param[in]     changeType        The type of change being made
/// @param[in]     objectType        For an admin subscription request,
///                                  which namespace to use.
///
/// @return OK on successful completion, ISMRC_AsyncCompletion if the delete
///         needs to happen asynchronously or another ISMRC_ value on error.
//****************************************************************************
int iett_subscriptionConfigCallback(ieutThreadData_t *pThreadData,
                                    char *objectIdentifier,
                                    ism_prop_t *changedProps,
                                    ism_ConfigChangeType_t changeType,
                                    const char *objectType)
{
    int rc = OK;

    ieutTRACEL(pThreadData, changeType, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Get required values from the properties
    char *subscriptionName = NULL;
    const char *nameSpace = NULL;
    const char *clientId = NULL;
    uint32_t destroyOptions;

    // Mustn't try and create or delete subscriptions unless we are in running state
    ismEngineRunPhase_t runPhase = ismEngine_serverGlobal.runPhase;
    if (runPhase != EnginePhaseRunning)
    {
        ieutTRACEL(pThreadData, runPhase, ENGINE_INTERESTING_TRACE,
                   FUNCTION_IDENT "Called during Phase 0x%08X.", __func__, runPhase);
        rc = ISMRC_InvalidOperation;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // For an admin subscription request, modify the defaults
    if (objectType[0] != ismENGINE_ADMIN_VALUE_SUBSCRIPTION[0])
    {
        if (strcmp(objectType, ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION) == 0)
        {
            nameSpace = ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED;
        }
        else if (strcmp(objectType, ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB) == 0)
        {
            nameSpace = ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_NONDURABLE;
        }
        else
        {
            assert(strcmp(objectType, ismENGINE_ADMIN_VALUE_DURABLENAMESPACEADMINSUB) == 0);
            nameSpace = ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE;
        }

        // Indicate to the destroy function that this request *did* come via the AdminSubscription interface.
        destroyOptions = iettSUB_DESTROY_OPTION_DESTROY_ADMINSUBSCRIPTION;

        subscriptionName = objectIdentifier;

        bool discardSharers = ism_common_getBooleanProperty(changedProps,
                                                            ismENGINE_ADMIN_PROPERTY_DISCARDSHARERS,
                                                            ismENGINE_DEFAULT_PROPERTY_DISCARDSHARERS);

        // If not discarding sharers, and in the option to indicate we should only remove anonymous sharer
        if (!discardSharers)
        {
            destroyOptions |= iettSUB_DESTROY_OPTION_ONLY_REMOVE_ANONYMOUS_SHARER_ADMIN;
        }
    }
    else
    {
        destroyOptions = iettSUB_DESTROY_OPTION_NONE;
    }

    // Simplest way to find properties is to loop through looking for the prefix
    if (changedProps != NULL)
    {
        const char *propertyName;
        for (int32_t i = 0; ism_common_getPropertyIndex(changedProps, i, &propertyName) == 0; i++)
        {
            if (strncmp(propertyName,
                        ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_SUBSCRIPTIONNAME,
                        strlen(ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_SUBSCRIPTIONNAME)) == 0)
            {
                subscriptionName = (char *)ism_common_getStringProperty(changedProps, propertyName);
                ieutTRACEL(pThreadData, subscriptionName, ENGINE_NORMAL_TRACE, "SubscriptionName='%s'\n", subscriptionName);
            }
            else if (strncmp(propertyName,
                             ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_CLIENTID,
                             strlen(ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_CLIENTID)) == 0)
            {
                clientId = ism_common_getStringProperty(changedProps, propertyName);
                ieutTRACEL(pThreadData, clientId, ENGINE_NORMAL_TRACE, "clientId='%s'\n", clientId);
            }
        }
    }

    // No subscription name supplied
    if (subscriptionName == NULL)
    {
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(rc, "%s%s", ismENGINE_ADMIN_SUBSCRIPTION_SUBSCRIPTIONNAME, "");
        goto mod_exit;
    }

    // For a change request, we only expect to be called for shared subscriptions
    if (changeType == ISM_CONFIG_CHANGE_PROPS)
    {
        if (nameSpace == NULL ||
            strncmp(nameSpace,
                    ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_PREFIX,
                    strlen(ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_PREFIX)) != 0)
        {
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", ismENGINE_ADMIN_ALLADMINSUBS_NAMESPACE, nameSpace);
            goto mod_exit;
        }

        // Subscription names in the mixed shared namespace always start with '/'.
        // The protocol layer relies on it, so let's enforce it.
        if (strcmp(nameSpace, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED) == 0 &&
            subscriptionName[0] != '/')
        {
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", ismENGINE_ADMIN_SUBSCRIPTION_SUBSCRIPTIONNAME, subscriptionName);
            goto mod_exit;
        }
    }
    else if (clientId == NULL)
    {
        // clientID is synonymous with nameSpace -- so allow the caller to specify either.
        clientId = nameSpace;

        if (clientId == NULL)
        {
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", ismENGINE_ADMIN_SUBSCRIPTION_CLIENTID, clientId);
            goto mod_exit;
        }
    }

    // The action taken varies depending on the requested changeType.
    switch(changeType)
    {
        case ISM_CONFIG_CHANGE_DELETE:
            rc = iett_destroySubscriptionForClientId(pThreadData,
                                                     clientId,
                                                     NULL, // No auth check on admin
                                                     subscriptionName,
                                                     NULL,
                                                     destroyOptions,
                                                     NULL, 0, NULL);

            assert(rc != ISMRC_AsyncCompletion);

            if (rc != OK && rc != ISMRC_WrongSubscriptionAPI) ism_common_setError(rc);
            break;

        case ISM_CONFIG_CHANGE_PROPS:
            rc = iett_createAdminSharedSubscription(pThreadData,
                                                    nameSpace,
                                                    subscriptionName,
                                                    changedProps,
                                                    objectType,
                                                    NULL, 0, NULL);

            // This might go Async, but the caller doesn't handle async responses
            if (rc == ISMRC_AsyncCompletion) rc = OK;
            break;

        default:
            rc = ISMRC_InvalidOperation;
            ism_common_setError(rc);
            break;
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Traverse the list of all subscriptions calling a callback for each
///
/// @param[in]     callback         Callback function to call for each subscription
/// @param[in]     context          Context to pass to the function
///
/// @remark At various points during traversal, the lock on the subscriptions
/// is released and regained (to allow other work to take place) -- so it is
/// possible the subscriptions found at the start will no longer exist at the end.
//****************************************************************************
void iett_traverseSubscriptions(ieutThreadData_t *pThreadData,
                                iettTraverseSubscriptionsCallback_t callback,
                                void *context)
{
    ieutTRACEL(pThreadData, context, ENGINE_HIGH_TRACE, FUNCTION_ENTRY "callback=%p context=%p\n", __func__, callback, context);

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    // Lock the topic tree for read access
    ismEngine_getRWLockForRead(&tree->subsLock);

    // Start with the first subscription
    ismEngine_Subscription_t *curSubscription = tree->subscriptionHead;
    ismEngine_Subscription_t *heldSubscription = NULL;

    uint32_t checkedCount = 0;

    // There are no subscriptions unlock the tree and return
    if (curSubscription == NULL)
    {
        ismEngine_unlockRWLock(&tree->subsLock);
    }
    else
    {
        // There are subscriptions, go through them
        while(curSubscription != NULL)
        {
            // Call the requested callback...
            bool keepGoing = callback(pThreadData, curSubscription, context);

            ismEngine_Subscription_t *nextSubscription = keepGoing ? curSubscription->next : NULL;

            // Once we've checked some number of subscriptions, or there are none left, go through
            // the process of releasing (and re-acquiring) the lock to give others a chance.
            if ((++checkedCount % iettTRAVERSE_SUBSCRIPTION_RETAKE_LOCK_AFTER) == 0 || nextSubscription == NULL)
            {
                // We need to make sure the subscription we are about to move to does not disappear
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

                // We have more subscriptions to analyse, so get the lock back
                if (nextSubscription != NULL)
                {
                    ismEngine_getRWLockForRead(&tree->subsLock);
                }
            }

            curSubscription = nextSubscription;
        }

        assert(heldSubscription == NULL);
    }

    ieutTRACEL(pThreadData, checkedCount, ENGINE_HIGH_TRACE, FUNCTION_ENTRY "callback=%p context=%p\n", __func__, callback, context);
}

//****************************************************************************
/// @brief  Callback used when finding a single subscription from attributes
///
/// Look for a subscription that matches the requested attributes
//****************************************************************************
typedef struct tag_iettFindSingleSubContext_t
{
    ismQHandle_t matchQHandle;
    ismEngine_Subscription_t *subscription;
    bool includeDeleted;
} iettFindSingleSubContext_t;

static bool iett_findSingleSubCallback(ieutThreadData_t *pThreadData,
                                       ismEngine_Subscription_t *pSubscription,
                                       void *pContext)
{
    bool keepGoing = true;

    iettFindSingleSubContext_t *context = (iettFindSingleSubContext_t *)pContext;

    if (context->matchQHandle == pSubscription->queueHandle)
    {
        // Check whether the caller only wants active (not logically deleted) subscriptions
        if (context->includeDeleted ||
            ((pSubscription->internalAttrs & iettSUBATTR_DELETED) == 0))
        {
            iett_acquireSubscriptionReference(pSubscription);
            context->subscription = pSubscription;
        }

        keepGoing = false; // Whether returning it or not, we found the matching subscription
    }

    return keepGoing;
}

//****************************************************************************
/// @brief  Get the subscription with a specified queue handle
///
/// Locate and return the specified subscription.
///
/// @param[in]     queueHandle      Client Id which owns the subscription
/// @param[out]    phSubscription   Pointer to receive a handle to the subscription
///
/// @return OK on successful completion, ISMRC_NotFound if not found or an
///         ISMRC_ value.
///
/// @remark The queue handle has no pointer back to the subscription, so in
/// order to satisfy the request, we need to walk the linked list of subscriptions
/// finding the required one.
///
/// if phSubscription is non-NULL, the subscription returned to the caller
/// with its useCount incremented by one. Once it is no longer needed, release
/// the subscription using iett_releaseSubscription.
///
/// @see iett_releaseSubscription
//****************************************************************************
int32_t iett_findQHandleSubscription(ieutThreadData_t *pThreadData,
                                     ismQHandle_t queueHandle,
                                     ismEngine_SubscriptionHandle_t *phSubscription)
{
    int32_t rc = OK;

    iettFindSingleSubContext_t context = {0};

    ieutTRACEL(pThreadData, queueHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "queueHandle=%p\n", __func__,
               queueHandle);

    context.matchQHandle = queueHandle;
    context.includeDeleted = false;

    iett_traverseSubscriptions(pThreadData, iett_findSingleSubCallback, &context);

    if (context.subscription != NULL)
    {
        if (phSubscription != NULL)
        {
            *phSubscription = context.subscription;
        }
        else
        {
            iett_releaseSubscription(pThreadData, context.subscription, false);
        }
    }
    else
    {
        rc = ISMRC_NotFound;
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, subscription=%p\n", __func__,
               rc, context.subscription);

    return rc;
}

//****************************************************************************
/// @brief  Get the subscription with a specific subscription name
///
/// Locate and return the named subscription.
///
/// @param[in]     pClientId        Client Id which owns the subscription
/// @param[in]     clientIdHash     Hash of the clientId as returned by
///                                 iett_generateClientIdHash
/// @param[in]     pSubName         Subscription name
/// @param[in]     subNameHash      Hash of the subscription name as returned by
///                                 iett_generateSubNameHash
/// @param[out]    phSubscription   Pointer to receive a handle to the subscription
///
/// @return OK on successful completion, ISMRC_NotFound if not found or an
///         ISMRC_ value.
///
/// @remark The requested subscription must have been previously created with
/// a call to ism_engine_createSubscription specifying the name.
///
/// If phSubscription is non-NULL, the subscription is returned to the caller
/// with its useCount incremented by one. Once it is no longer needed, release
/// the subscription using iett_releaseSubscription.
///
/// @see ism_engine_createSubscription
/// @see iett_releaseSubscription
//****************************************************************************
int32_t iett_findClientSubscription(ieutThreadData_t *pThreadData,
                                    const char *pClientId,
                                    const uint32_t clientIdHash,
                                    const char *pSubName,
                                    const uint32_t subNameHash,
                                    ismEngine_SubscriptionHandle_t *phSubscription)
{
    int32_t rc = OK;
    ismEngine_Subscription_t *subscription = NULL;
    iettClientSubscriptionList_t *clientNamedSubs = NULL;

    ieutTRACEL(pThreadData, pSubName, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pSubName='%s'\n", __func__,
               pSubName);

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ismEngine_getRWLockForRead(&tree->namedSubsLock);

    rc = ieut_getHashEntry(tree->namedSubs,
                           pClientId,
                           clientIdHash,
                           (void **)&clientNamedSubs);

    if (rc == OK)
    {
        rc = ISMRC_NotFound;

        uint32_t start = 0;
        uint32_t middle = 0;
        uint32_t end = clientNamedSubs->count;

        while(start < end)
        {
            middle = start + ((end-start)/2);

            uint32_t thisSubNameHash = clientNamedSubs->list[middle].subNameHash;

            // subNameHash match, so we need to find matching string.
            if (thisSubNameHash == subNameHash)
            {
                uint32_t pos = middle;

                while(1)
                {
                    subscription = clientNamedSubs->list[pos].subscription;

                    if (strcmp(subscription->subName, pSubName) == 0)
                    {
                        rc = OK;
                        break;
                    }
                    else if (pos == start ||
                             subNameHash != clientNamedSubs->list[--pos].subNameHash)
                    {
                        break;
                    }
                }

                if (rc == ISMRC_NotFound)
                {
                    pos = middle;

                    while(1)
                    {
                        if (pos == end ||                                             // End of range
                            subNameHash != clientNamedSubs->list[++pos].subNameHash)  // Hash mismatch
                        {
                            break;
                        }
                        else
                        {
                            subscription = clientNamedSubs->list[pos].subscription;

                            if (subscription != NULL && strcmp(subscription->subName, pSubName) == 0)
                            {
                                rc = OK;
                                break;
                            }
                        }
                    }
                }

                if (rc == OK && phSubscription != NULL)
                {
                    assert(subscription != NULL);
                    (void)__sync_fetch_and_add(&subscription->useCount, 1);
                    *phSubscription = subscription;
                }
                break;
            }
            else if (thisSubNameHash > subNameHash)
            {
                end = middle;
            }
            else
            {
                start = middle + 1;
            }
        }
    }

    ismEngine_unlockRWLock(&tree->namedSubsLock);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, subscription=%p, subOptions=0x%08x\n", __func__,
               rc, subscription, rc == OK ? subscription->subOptions : 0);

    return rc;
}

//****************************************************************************
/// @brief Find the clientIds owning or known to be using a subscription
///
/// @param[in]     subscription   Subscription being added
/// @param[out]    pFoundClients  Pointer to receive array of clientIds
///                               (terminated by a NULL entry)
///
/// @remark The subscription knows about a subset of the clients that could be
/// consuming from it. For a non-globally shared subscription, this will be the
/// owning client, and for a shared subscription it will be the list of clients
/// that have registered as sharing it (using ism_engine_createSubscription or
/// ism_engine_reuseSubscription).
///
/// It is possible that a shared subscription will have no sharing clients (but
/// still exist because it has anonymous sharers) -- For this reason
/// it is possible that the return code ISMRC_NotFound will be returned.
///
/// The array returned contains a NULL indicating the end of the array (each of
/// the clientIds are null terminated strings). The array must be freed by a call
/// to iett_freeSubscriptionClientIds.
///
/// Note: The list of clientIds is a point-in-time statement, no lock is held on
/// the list, and the clientIds may be removed from the subscription at any time.
///
/// @return OK on successful completion, ISMRC_NotFound if no clients were found
/// or an ISMRC_ error value.
///
/// @see iett_freeSubscriptionClientIds
//****************************************************************************
int32_t iett_findSubscriptionClientIds(ieutThreadData_t *pThreadData,
                                       ismEngine_SubscriptionHandle_t subscription,
                                       const char ***pFoundClients)
{
    int32_t rc = OK;
    uint32_t foundClientCount = 0;
    const char **foundClients = NULL;

    ieutTRACEL(pThreadData, subscription, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "subscription=%p\n", __func__,
               subscription);

    if ((subscription->internalAttrs & iettSUBATTR_GLOBALLY_SHARED) == 0)
    {
        assert(strstr(subscription->clientId, "__Share") == NULL);

        size_t clientIdLen = strlen(subscription->clientId) + 1;

        foundClientCount = 1;

        foundClients = iemem_malloc(pThreadData,
                                    IEMEM_PROBE(iemem_callbackContext, 14),
                                    (sizeof(const char *) * (foundClientCount+1)) + clientIdLen);

        if (foundClients == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        char *tmp = (char *)(foundClients + (foundClientCount+1));
        memcpy(tmp, subscription->clientId, clientIdLen);
        foundClients[0] = (const char *)tmp;
    }
    else
    {
        size_t clientIdBufferLen = 0;
        iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);

        assert(sharedSubData != NULL);

        // Get the lock on the shared subscription
        DEBUG_ONLY int osrc = pthread_spin_lock(&sharedSubData->lock);
        assert(osrc == 0);

        foundClientCount = sharedSubData->sharingClientCount;

        if (foundClientCount == 0)
        {
            rc = ISMRC_NotFound;
        }
        else
        {
            for(uint32_t index=0; index<foundClientCount; index++)
            {
                clientIdBufferLen += strlen(sharedSubData->sharingClients[index].clientId) + 1;
            }

            foundClients = iemem_malloc(pThreadData,
                                        IEMEM_PROBE(iemem_callbackContext, 15),
                                        (sizeof(const char *) * (foundClientCount+1)) + clientIdBufferLen);

            if (foundClients == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            }
            else
            {
                char *tmp = (char *)(foundClients + (foundClientCount+1));

                // Actually copy the clientIds across
                for(uint32_t index=0; index<foundClientCount; index++)
                {
                    assert(strstr(sharedSubData->sharingClients[index].clientId, "__Share") == NULL);
                    size_t clientIdLen = strlen(sharedSubData->sharingClients[index].clientId) + 1;
                    memcpy(tmp, sharedSubData->sharingClients[index].clientId, clientIdLen);
                    foundClients[index] = (const char *)tmp;
                    tmp += clientIdLen;
                }

                assert((void *)(&foundClients[foundClientCount+1]) == (void *)(tmp-clientIdBufferLen));
            }
        }

        osrc = pthread_spin_unlock(&sharedSubData->lock);
        assert(osrc == 0);

        if (rc != OK) goto mod_exit;
    }

    foundClients[foundClientCount] = NULL;
    *pFoundClients = foundClients;

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d, array=%p\n", __func__,
               rc, foundClients);

    return rc;

}

//****************************************************************************
/// @brief Free an array of found clients from iett_findSubscriptionClientIds
///
/// @param[in]     subscription  Subscription being added
/// @param[in]     foundClients  Array of clientIds to free
///
/// @see iett_findSubscriptionClientIds
//****************************************************************************
void iett_freeSubscriptionClientIds(ieutThreadData_t *pThreadData,
                                    const char **foundClients)
{
    iemem_free(pThreadData, iemem_callbackContext, foundClients);
}

//****************************************************************************
/// @brief Add a subscription to a list of subscriptions for a client
///
/// @param[in]     subscription  Subscription being added
/// @param[in out] subList       List being updated
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static int32_t iett_addSubscriptionToClientList(ieutThreadData_t *pThreadData,
                                                ismEngine_Subscription_t *subscription,
                                                iettClientSubscriptionList_t *subList)
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

        iettClientSubscription_t *newSubList;

        // Note: We allocate one extra entry to allow for a NULL sentinel
        newSubList = iemem_realloc(pThreadData,
                                   IEMEM_PROBE(iemem_subsTree, 10),
                                   subList->list,
                                   (newMax+1) * sizeof(iettClientSubscription_t));

        if (NULL == newSubList)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        subList->max = newMax;
        subList->list = newSubList;
    }

    // The list is sorted, need to put the entry in the right position
    // (ordered by subName hash).
    uint32_t start = 0;
    uint32_t end = subList->count;
    uint32_t subNameHash = subscription->subNameHash;
    ismEngine_Subscription_t *foundSub = NULL;

    while(start < end)
    {
        uint32_t middle = start + ((end-start)/2);
        uint32_t thisSubNameHash = subList->list[middle].subNameHash;

        if (thisSubNameHash == subNameHash)
        {
            uint32_t pos = middle;

            while(1)
            {
                if (subscription == subList->list[pos].subscription)
                {
                    // Found this subscriber already in the list
                    foundSub = subscription;
                    break;
                }
                else if (pos == start ||
                         subNameHash != subList->list[--pos].subNameHash)
                {
                    break;
                }
            }

            if (foundSub == NULL)
            {
                pos = middle;

                while(1)
                {
                    if (pos == end ||                                      // End of range
                        subNameHash != subList->list[++pos].subNameHash)   // Hash mismatch
                    {
                        break;
                    }
                    else if (subscription == subList->list[pos].subscription)
                    {
                        // Found this subscriber already in the list
                        foundSub = subscription;
                        break;
                    }
                }
            }

            end = pos;
            break;
        }
        else if (thisSubNameHash > subNameHash)
        {
            end = middle;
        }
        else
        {
            start = middle + 1;
        }
    }

    if (foundSub != subscription)
    {
        int32_t moveSubscribers = subList->count-end;

        if (moveSubscribers != 0)
        {
            memmove(&subList->list[end+1],
                    &subList->list[end],
                    moveSubscribers*sizeof(iettClientSubscription_t));
        }

        subList->list[end].subNameHash = subscription->subNameHash;
        subList->list[end].subscription = subscription;
        subList->count++;
        subList->list[subList->count].subscription = NULL; // Sentinel
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief  Add the specified named subscription to the requested client
///         list.
///
/// @param[in]     subscription     subscription to be removed
/// @param[in]     clientId         client Id to which to add subscription
/// @param[in]     clientIdHash     Hash calculated for the specified client Id
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @see iett_removeSubscription
//****************************************************************************
int32_t iett_addSubscription(ieutThreadData_t *pThreadData,
                             ismEngine_Subscription_t *subscription,
                             const char * clientId,
                             const uint32_t clientIdHash)
{
    int32_t rc = OK;

    assert(clientId != NULL);
    assert(subscription != NULL);
    assert(subscription->subName != NULL);

    ieutTRACEL(pThreadData, subscription, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Subscription=%p (clientId='%s' subName='%s' subNameHash=%u)\n", __func__,
               subscription, clientId, subscription->subName, subscription->subNameHash);

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    ismEngine_getRWLockForWrite(&tree->namedSubsLock);

    iettClientSubscriptionList_t *clientNamedSubs = NULL, *newClientNamedSubs = NULL;

    // Get the existing list of named subscriptions for this client.
    rc = ieut_getHashEntry(tree->namedSubs,
                           clientId,
                           clientIdHash,
                           (void **)&clientNamedSubs);

    if (rc == ISMRC_NotFound)
    {
        newClientNamedSubs = iemem_calloc(pThreadData,
                                          IEMEM_PROBE(iemem_subsTree, 3), 1,
                                          sizeof(iettClientSubscriptionList_t));

        if (NULL == newClientNamedSubs)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto release_namedSubs_lock;
        }

        clientNamedSubs = newClientNamedSubs;
    }

    // Update the list.
    rc = iett_addSubscriptionToClientList(pThreadData,
                                          subscription,
                                          clientNamedSubs);

    if (rc != OK) goto release_namedSubs_lock;

    // Update the entry for this client Id.
    rc = ieut_putHashEntry(pThreadData,
                           tree->namedSubs,
                           ieutFLAG_DUPLICATE_KEY_STRING | ieutFLAG_REPLACE_EXISTING,
                           clientId,
                           clientIdHash,
                           clientNamedSubs,
                           0);

    if (rc != OK) goto release_namedSubs_lock;

    // Now it's in the hash table, we don't want the new list to be free'd.
    newClientNamedSubs = NULL;

release_namedSubs_lock:

    ismEngine_unlockRWLock(&tree->namedSubsLock);

    // If we are failing we need to clear some things up
    if (rc != OK)
    {
        // If we allocated clientNamedSubs, but did not add it to the hash we need to clear it up
        if (newClientNamedSubs != NULL)
        {
            if (newClientNamedSubs->list != NULL) iemem_free(pThreadData, iemem_subsTree, newClientNamedSubs->list);
            iemem_free(pThreadData, iemem_subsTree, newClientNamedSubs);
        }
    }


    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief Remove a subscription from a list of subscriptions for a client
///
/// @param[in]     subscription  Subscription being removed
/// @param[in out] subList       List being updated
///
/// @return OK on successful completion, ISMRC_NotFound if the subscription
///         is not in the list or an ISMRC_ value.
//****************************************************************************
static int32_t iett_removeSubscriptionFromClientList(ieutThreadData_t *pThreadData,
                                                     ismEngine_Subscription_t *subscription,
                                                     iettClientSubscriptionList_t *subList)
{
    int32_t   rc = ISMRC_NotFound;
    uint32_t  start = 0;
    uint32_t  middle = 0;
    uint32_t  end = subList->count;
    uint32_t subNameHash = subscription->subNameHash;

    ieutTRACEL(pThreadData, subscription,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "subscription=%p, subList=%p\n", __func__, subscription, subList);

    while(start < end)
    {
        middle = start + ((end-start)/2);

        uint32_t thisSubNameHash = subList->list[middle].subNameHash;

        if (thisSubNameHash == subNameHash)
        {
            uint32_t pos = middle;

            while(1)
            {
                if (subscription == subList->list[pos].subscription)
                {
                    rc = OK;
                    break;
                }
                else if (pos == start ||
                         subNameHash != subList->list[--pos].subNameHash)
                {
                    break;
                }
            }

            if (rc == ISMRC_NotFound)
            {
                pos = middle;

                while(1)
                {
                    if (pos == end ||                                     // End of range
                        subNameHash != subList->list[++pos].subNameHash)  // Hash mismatch
                    {
                        break;
                    }
                    else if (subscription == subList->list[pos].subscription)
                    {
                        rc = OK;
                        break;
                    }
                }
            }

            middle = pos;
            break;
        }
        else if (thisSubNameHash > subNameHash)
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
        end = subList->count;

        if (end > 1)
        {
            int32_t moveSubscribers = end-middle;

            if (moveSubscribers)
            {
                memmove(&subList->list[middle],
                        &subList->list[middle+1],
                        moveSubscribers*sizeof(iettClientSubscription_t));
            }
        }

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
            subList->list[subList->count].subscription = NULL; // Sentinel
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief  Remove the specified named subscription from the requested client
///         list.
///
/// @param[in]     subscription     subscription to be removed
/// @param[in]     clientId         client Id from which to remove subscription
/// @param[in]     clientIdHash     Hash calculated for the specified client Id
///
/// @see iett_addSubscription
//****************************************************************************
void iett_removeSubscription(ieutThreadData_t *pThreadData,
                             ismEngine_Subscription_t *subscription,
                             const char * clientId,
                             const uint32_t clientIdHash)
{
    assert(clientId != NULL);
    assert(subscription != NULL);
    assert(subscription->subName != NULL);

    ieutTRACEL(pThreadData, subscription, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Subscription=%p (clientId='%s' subName='%s' subNameHash=%u)\n", __func__,
               subscription, clientId, subscription->subName, subscription->subNameHash);

    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    bool removeClientNamedSubs = false;
    iettClientSubscriptionList_t *clientNamedSubs = NULL;

    ismEngine_getRWLockForWrite(&tree->namedSubsLock);

    int32_t rc = ieut_getHashEntry(tree->namedSubs,
                                   clientId,
                                   clientIdHash,
                                   (void **)&clientNamedSubs);

    if (rc == OK)
    {
        rc = iett_removeSubscriptionFromClientList(pThreadData,
                                                   subscription,
                                                   clientNamedSubs);

        if (rc == OK)
        {
            // No named subscriptions left for this client, remove the client from
            // the hash table altogether.
            removeClientNamedSubs = (clientNamedSubs->count == 0);

            if (removeClientNamedSubs)
            {
                ieut_removeHashEntry(pThreadData,
                                     tree->namedSubs,
                                     clientId,
                                     clientIdHash);
            }
        }
    }

    ismEngine_unlockRWLock(&tree->namedSubsLock);

    assert(rc == OK || rc == ISMRC_NotFound);

    if (removeClientNamedSubs)
    {
        assert(clientNamedSubs != NULL);
        iemem_free(pThreadData, iemem_subsTree, clientNamedSubs);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d (not returned)\n", __func__, rc);

    return;
}

//****************************************************************************
/// @brief  Acquire a reference to this subscription
///
/// Increment the useCount for this subscription
///
/// @param[in]    subscription    Subscription to acquire a reference to
///
/// @return OK on successful completion, ISMRC_NotFound if not found or an
///         ISMRC_ value.
///
/// @remark Once it is no longer needed, release the subscription using
///         ism_engine_releaseSubscription.
///
/// @see iett_releaseSubscription
//****************************************************************************
void iett_acquireSubscriptionReference(ismEngine_Subscription_t *subscription)
{
    assert(subscription != NULL);
    ismEngine_CheckStructId(subscription->StrucId, ismENGINE_SUBSCRIPTION_STRUCID, ieutPROBE_007);

    (void)__sync_fetch_and_add(&subscription->useCount, 1);
}

//****************************************************************************
/// @brief  Release the subscription
///
/// Decrement the use count for the subscription removing it from the engine
/// topic tree if it drops to zero.
///
/// @param[in]     subscription  Subscription to be released
/// @param[in]     fInline       Whether being called from the destroy function
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_releaseSubscription(ieutThreadData_t *pThreadData,
                                 ismEngine_Subscription_t * subscription,
                                 bool fInline)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, subscription, ENGINE_FNC_TRACE, FUNCTION_ENTRY "subscription=%p, internalAttrs=0x%08x, fInline=%s\n", __func__,
               subscription, subscription->internalAttrs, fInline ? "true":"false");

    uint32_t oldCount = __sync_fetch_and_sub(&subscription->useCount, 1);

    assert(oldCount != 0); // useCount just went negative

    // This was the last user, actually perform the removal
    if (oldCount == 1)
    {
        // Flag the subscription as logically deleted (in case it hasn't already been)
        subscription->internalAttrs |= iettSUBATTR_DELETED;

        uint32_t flags = fInline ? iettFLAG_REMOVE_SUB_INLINE_DESTROY : iettFLAG_REMOVE_SUB_NONE;

        rc = iett_removeSubFromEngineTopic(pThreadData, subscription, flags);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d (useCount=%u)\n", __func__, rc, oldCount-1);

    return rc;
}

//****************************************************************************
/// @brief  Free the memory and runtime state for a given subscription.
///
/// @param[in]     subscription   Subscription to be free'd
/// @param[in]     freeOnly       Whether we should only free the memory and not
///                               store (e.g. during shutdown or rollback of a transaction
///                               where the records will be deleted by other soft log entries).
///
/// @remarks If the subscription is being free'd because it has been deleted, i.e.
///          it has been removed from the engine topic tree, freeOnly should be
///          false.
//****************************************************************************
void iett_freeSubscription(ieutThreadData_t *pThreadData,
                           ismEngine_Subscription_t *subscription,
                           bool freeOnly)
{
    iereResourceSetHandle_t resourceSet = subscription->resourceSet;

    ieutTRACEL(pThreadData, subscription, ENGINE_FNC_TRACE, FUNCTION_ENTRY "subscription=%p freeOnly=%d\n", __func__,
               subscription, (int)freeOnly);

    // Deleting the queue will also delete the SDR / SPR from the store
    (void)ieq_delete(pThreadData, &(subscription->queueHandle), freeOnly);

    iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);

    iere_primeThreadCache(pThreadData, resourceSet);

    // Tidy up shared subscription data
    if (sharedSubData != NULL)
    {
        // Free entries remaining in the sharing clients list
        if (sharedSubData->sharingClients != NULL)
        {
            for(uint32_t index=0; index<sharedSubData->sharingClientCount; index++)
            {
                iere_free(pThreadData, resourceSet, iemem_externalObjs, sharedSubData->sharingClients[index].clientId);
            }

            iere_free(pThreadData, resourceSet, iemem_externalObjs, sharedSubData->sharingClients);
        }

        pthread_spin_destroy(&sharedSubData->lock);
    }

    // Tidy up flattened subscription properties
    iere_free(pThreadData, resourceSet, iemem_subsTree, subscription->flatSubProperties);
    iere_freeStruct(pThreadData, resourceSet, iemem_subsTree, subscription, subscription->StrucId);

    ieutTRACEL(pThreadData, freeOnly, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

//****************************************************************************
/// @internal
///
/// @brief Register a consumer as a subscriber on a subscription
///
/// @param[in]     subscription  Subscription upon which consumer is registered
/// @param[in]     consumer      Consumer to be registered as a subscriber
///
/// @remark Assumes that the global engine topic tree has been initialised
///         by a call to iett_initEngineTopicTree.
///
/// This function expects the subscription to have it's useCount incremented by
//  the caller - it then takes over the useCount.
///
/// @see iett_unregisterConsumer
//****************************************************************************
void iett_registerConsumer(ieutThreadData_t *pThreadData,
                           ismEngine_Subscription_t *subscription,
                           ismEngine_Consumer_t *consumer)
{
    ieutTRACEL(pThreadData, consumer, ENGINE_FNC_TRACE, FUNCTION_IDENT "subscription=%p, consumer=%p\n", __func__,
               subscription, consumer);

    // Increment the consumerCount, the useCount should already have an increment
    // which we are going to adopt.
    (void)__sync_fetch_and_add(&subscription->consumerCount, 1);

    consumer->engineObject = subscription;
    consumer->queueHandle = subscription->queueHandle;
}

//****************************************************************************
/// @brief Unregister a subscriber previously registered with
///        iett_registerConsumer from the global engine topic tree.
///
/// @param[in]     consumer  Subscriber being registered
///
/// @see iett_registerConsumer
//****************************************************************************
void iett_unregisterConsumer(ieutThreadData_t *pThreadData,
                             ismEngine_Consumer_t *consumer)
{
    ismEngine_Subscription_t *subscription = (ismEngine_Subscription_t *)consumer->engineObject;

    ieutTRACEL(pThreadData, consumer, ENGINE_FNC_TRACE, FUNCTION_IDENT "consumer=%p, subscription=%p\n", __func__,
               consumer, subscription);

    // For non-durable MQTT subscriptions, we want to ensure that any in-flight messages
    // are still tracked.
    if ((subscription->subOptions & (ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE |
                                     ismENGINE_SUBSCRIPTION_OPTION_DURABLE)) == 0)
    {
        assert(consumer->pSession != NULL);
        assert(consumer->pSession->pClient != NULL);

        iecs_trackInflightMsgs(pThreadData, consumer->pSession->pClient, subscription->queueHandle);
    }

    // Decrement the consumer count
    DEBUG_ONLY uint32_t oldConsumerCount = __sync_fetch_and_sub(&subscription->consumerCount, 1);
    assert(oldConsumerCount != 0);

    // Decrement the general useCount - potentially destroying the subscription
    (void)iett_releaseSubscription(pThreadData, subscription, false);

    consumer->engineObject = NULL;
}

//****************************************************************************
/// @brief Allocate the storage for a subscription queue name
///
/// Build the name of a subscription's queue, the format is ClientID(SubName) or
/// ClientID<TopicName>.
///
/// @param[in]     pClientId          Client Id of creator
/// @param[in]     clientIdLength     Length of client id + null terminator
/// @param[in]     pSubName           Subscription name (may be NULL)
/// @param[in]     subNameLength      Subscription name length + null terminator
/// @param[in]     pTopicString       Topic string (may be NULL)
/// @param[in]     topicStringLength  Topic string length + null terminator
/// @param[out]    ppQueueName        Returned pointer to allocated queue name (or NULL)
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_allocateSubQueueName(ieutThreadData_t *pThreadData,
                                  const char *pClientId,
                                  size_t clientIdLength,
                                  const char *pSubName,
                                  size_t subNameLength,
                                  const char *pTopicString,
                                  size_t topicStringLength,
                                  char **ppQueueName)
{
    char *queueName = NULL;
    int32_t      rc = OK;

    // We must have a valid clientId
    assert(pClientId != NULL && clientIdLength > 0);

    if (subNameLength > 0)
    {
        assert(pSubName != NULL);

        queueName = iemem_malloc(pThreadData,
                                 IEMEM_PROBE(iemem_subsTree, 7),
                                 clientIdLength + subNameLength + 1);
        if (queueName != NULL)
        {
            sprintf(queueName, "%s(%s)", pClientId, pSubName);
            *ppQueueName = queueName;
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }
    }
    else
    {
        assert(pTopicString != NULL && topicStringLength > 0);

        queueName = iemem_malloc(pThreadData,
                                 IEMEM_PROBE(iemem_subsTree, 8),
                                 clientIdLength + topicStringLength + 1);
        if (queueName != NULL)
        {
            sprintf(queueName, "%s<%s>", pClientId, pTopicString);
            *ppQueueName = queueName;
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }
    }

    return rc;
}

//****************************************************************************
/// @brief Free the storage for a subscription queue name
///
/// Free the storage allocated to build the name of a subscription's queue.
///
/// @param[in]     pAllocName         Ptr to buffer to free
//****************************************************************************
void iett_freeSubQueueName(ieutThreadData_t *pThreadData, char *pAllocName)
{
    iemem_free(pThreadData, iemem_subsTree, pAllocName);
}
