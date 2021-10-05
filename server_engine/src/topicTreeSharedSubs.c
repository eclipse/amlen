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
/// @file  topicTreeSharedSubs.c
/// @brief Engine component shared subscription specific functions
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
// @brief  Initialize the objects that represent the globally shared subscription
//         namespaces.
//
// @param[in]     pThreadData          Current thread control data block
//
// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_initSharedSubNameSpaces(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pThreadData, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    const char *nameSpaceClientIDs[] = { ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE,
                                         ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_NONDURABLE,
                                         ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED,
                                         NULL };
    const char **pNameSpaceClientID = nameSpaceClientIDs;
    while(*pNameSpaceClientID != NULL)
    {
        ismEngine_ClientState_t *pClient;
        iecsNewClientStateInfo_t clientInfo;

        clientInfo.pClientId = *pNameSpaceClientID;
        clientInfo.protocolId = PROTOCOL_ID_SHARED;
        clientInfo.durability = iecsNonDurable;
        clientInfo.resourceSet = iecs_getResourceSet(pThreadData,
                                                     clientInfo.pClientId,
                                                     clientInfo.protocolId,
                                                     iereOP_ADD);

        rc = iecs_newClientStateRecovery(pThreadData,
                                         &clientInfo,
                                         &pClient);

        if (rc == OK)
        {
            assert(pClient->OpState == iecsOpStateZombie);

            // Treat the client state like a zombie that was connected at last shutdown
            rc = iecs_addClientStateRecovery(pThreadData, pClient);

            // Set last connected time to *now*
            if (rc == OK)
            {
                iecs_updateLastConnectedTime(pThreadData, pClient, false, NULL);

                assert(pClient->ExpiryInterval == iecsEXPIRY_INTERVAL_INFINITE);
                assert(pClient->ExpiryTime == 0);
            }
        }

        if (rc == OK)
        {
            ieutTRACEL(pThreadData, pClient, ENGINE_INTERESTING_TRACE, FUNCTION_IDENT "NameSpace '%s' created.\n",
                       __func__, clientInfo.pClientId);        
            pNameSpaceClientID++;
        }
        else
        {
            ieutTRACEL(pThreadData, rc, ENGINE_WORRYING_TRACE, FUNCTION_IDENT "Failed to create NameSpace '%s'. rc=%d\n",
                       __func__, clientInfo.pClientId, rc);
            break;
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
// @brief  Create a shared subscription shared by Admin (or update an existing
//         one to be shared by Admin).
//
// @param[in]     pThreadData          Current thread control data block
// @param[in]     nameSpace            The NameSpace
// @param[in]     subscriptionName     Shared subscription name to create
// @param[in]     topicFilter          Topic filter for the subscription
// @param[in]     adminProps           Admin properties to use
// @param[in]     adminObjectType      The admin objectType used to find properties
// @param[in]     pContext             Optional context for completion callback
// @param[in]     contextLength        Length of data pointed to by pContext
// @param[in]     pCallbackFn          Operation-completion callback
//
// @remark If the requested subscription already exists, we turn on the flag to
//         say this is an admin subscription (which means it will survive when
//         no-one is connected).
//
// @remark The subscriptionName specified is _modified_ for subscriptions that
//         are being created in the __SharedM namespace so that the actual
//         subscription name which gets created / modified is the same as would
//         be accessed from a connecting client.
//
// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_createAdminSharedSubscription(ieutThreadData_t *pThreadData,
                                           const char *nameSpace,
                                           const char *subscriptionName,
                                           ism_prop_t *adminProps,
                                           const char *adminObjectType,
                                           void *pContext,
                                           size_t contextLength,
                                           ismEngine_CompletionCallback_t pCallbackFn)
{
    int32_t rc = OK;
    DEBUG_ONLY int osrc;

    ieutTRACEL(pThreadData, adminProps, ENGINE_FNC_TRACE, FUNCTION_ENTRY "nameSpace=%s, subscriptionName='%s'\n", __func__,
               nameSpace, subscriptionName);

    ismEngine_ClientState_t *pOwningClient = NULL;
    ismEngine_Subscription_t *subscription = NULL;
    iettSharedSubData_t *sharedSubData = NULL;
    const char *messagingPolicyName = NULL;
    const char *topicPolicyName = NULL;
    const char *subscriptionPolicyName = NULL;
    const char *topicFilter = NULL;

    ismEngine_SubscriptionAttributes_t subAttributes = { ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                                         iettALLADMINSUBS_DEFAULT_MAXQUALITYOFSERVICE_SUBOPTIONS |
                                                         iettALLADMINSUBS_DEFAULT_ADDRETAINEDMSGS_SUBOPTIONS |
                                                         iettALLADMINSUBS_DEFAULT_QUALITYOFSERVICEFILTER_SUBOPTIONS,
                                                         ismENGINE_NO_SUBID };

    size_t objectTypeLen = strlen(adminObjectType)+1;

    // For AdminSubscription objects, we get the topic filter from the subscriptionName
    if (strcmp(adminObjectType, ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION) == 0)
    {
        assert(subscriptionName[0] == '/'); // Caller should already have checked this

        topicFilter = strchr(&subscriptionName[1], '/');

        // If there is no topicFilter or the share name is empty, complain
        // [an empty topic filter is valid, you can subscribe on empty string].
        if (topicFilter == NULL || topicFilter == &subscriptionName[1])
        {
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", ismENGINE_ADMIN_SUBSCRIPTION_SUBSCRIPTIONNAME, subscriptionName);
            goto mod_exit;
        }

        topicFilter++;
    }

    // Whether (hidden) SubOptions specified (which means we ignore other options)
    const char *requestedSubOptions = NULL;
    // Whether MaxQoS specified
    int32_t requestedMaxQoS = -1;
    // Whether QoSFilter specified
    const char *requestedQoSFilter = NULL;

    const char *propertyName;
    for (int32_t i = 0; ism_common_getPropertyIndex(adminProps, i, &propertyName) == 0; i++)
    {
        // Ignore properties which don't have the object type in them
        if (strlen(propertyName) <= objectTypeLen) continue;

        const char *checkPropertyName = propertyName + objectTypeLen;

        // TopicFilter
        if (strncmp(checkPropertyName,
                    ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_TOPICFILTER,
                    strlen(ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_TOPICFILTER)) == 0)
        {
            if (topicFilter == NULL)
            {
                topicFilter = ism_common_getStringProperty(adminProps, propertyName);
            }
            else
            {
                rc = ISMRC_BadPropertyName;
                ism_common_setErrorData(rc, "%s", ismENGINE_ADMIN_ALLADMINSUBS_TOPICFILTER);
                goto mod_exit;
            }
        }
        // SubscriptionPolicy
        else if (strncmp(checkPropertyName,
                         ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_SUBSCRIPTIONPOLICY,
                         strlen(ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_SUBSCRIPTIONPOLICY)) == 0)
        {
            assert(strcmp(adminObjectType, ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB) != 0);
            subscriptionPolicyName = ism_common_getStringProperty(adminProps, propertyName);
        }
        // TopicPolicy
        else if (strncmp(checkPropertyName,
                         ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_TOPICPOLICY,
                         strlen(ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_TOPICPOLICY)) == 0)
        {
            assert(strcmp(adminObjectType, ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB) == 0);
            topicPolicyName = ism_common_getStringProperty(adminProps, propertyName);
        }
        // MaxQualityOfService
        else if (strncmp(checkPropertyName,
                         ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_MAXQUALITYOFSERVICE,
                         strlen(ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_MAXQUALITYOFSERVICE)) == 0)
        {
            requestedMaxQoS = ism_common_getIntProperty(adminProps, propertyName, -1);

            if (requestedSubOptions == NULL)
            {
                subAttributes.subOptions &= ~iettALLADMINSUBS_DEFAULT_MAXQUALITYOFSERVICE_SUBOPTIONS;

                switch(requestedMaxQoS)
                {
                    case 0:
                        subAttributes.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
                        break;
                    case 1:
                        subAttributes.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
                        break;
                    case 2:
                        subAttributes.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
                        break;
                    default:
                        rc = ISMRC_BadPropertyValue;
                        ism_common_setErrorData(rc, "%s%d", ismENGINE_ADMIN_ALLADMINSUBS_MAXQUALITYOFSERVICE, requestedMaxQoS);
                        goto mod_exit;
                }
            }
        }
        // AddRetainedMsgs
        else if (strncmp(checkPropertyName,
                         ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_ADDRETAINEDMSGS,
                         strlen(ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_ADDRETAINEDMSGS)) == 0)
        {
            if (requestedSubOptions == NULL)
            {
                subAttributes.subOptions &= ~iettALLADMINSUBS_DEFAULT_ADDRETAINEDMSGS_SUBOPTIONS;

                if (ism_common_getBooleanProperty(adminProps, propertyName, 0) != true)
                {
                    subAttributes.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_NO_RETAINED_MSGS;
                }
            }
        }
        // QualityOfServiceFilter
        else if (strncmp(checkPropertyName,
                         ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_QUALITYOFSERVICEFILTER,
                         strlen(ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_QUALITYOFSERVICEFILTER)) == 0)
        {
            requestedQoSFilter = ism_common_getStringProperty(adminProps, propertyName);

            if (requestedSubOptions == NULL)
            {
                subAttributes.subOptions &= ~iettALLADMINSUBS_DEFAULT_QUALITYOFSERVICEFILTER_SUBOPTIONS;

                if (requestedQoSFilter == NULL ||
                    strcmp(requestedQoSFilter, ismENGINE_ADMIN_VALUE_QUALITYOFSERVICEFILTER_NONE) == 0)
                {
                    subAttributes.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_NONE;
                }
                else if (strcmp(requestedQoSFilter, ismENGINE_ADMIN_VALUE_QUALITYOFSERVICEFILTER_UNRELIABLEONLY) == 0)
                {
                    subAttributes.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY;
                }
                else if (strcmp(requestedQoSFilter, ismENGINE_ADMIN_VALUE_QUALITYOFSERVICEFILTER_RELIABLEONLY) == 0)
                {
                    subAttributes.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY;
                }
                else
                {
                    rc = ISMRC_BadPropertyValue;
                    ism_common_setErrorData(rc, "%s%s", ismENGINE_ADMIN_ALLADMINSUBS_QUALITYOFSERVICEFILTER, requestedQoSFilter);
                    goto mod_exit;
                }
            }
        }
        // SubOptions
        else if (strncmp(checkPropertyName,
                         ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_SUBOPTIONS,
                         strlen(ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_SUBOPTIONS)) == 0)
        {
            requestedSubOptions = ism_common_getStringProperty(adminProps, propertyName);

            char *endPtr = NULL;
            uint32_t requestedSubOptionsValue = requestedSubOptions ? (uint32_t)strtoul(requestedSubOptions, &endPtr, 0) : ismENGINE_SUBSCRIPTION_OPTION_NONE;

            if (endPtr == NULL || *endPtr != '\0')
            {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(rc, "%s%s", ismENGINE_ADMIN_ALLADMINSUBS_SUBOPTIONS, requestedSubOptions);
                goto mod_exit;
            }

            // Must have at least SHARED and a non-zero delivery mask
            if (((requestedSubOptionsValue & ismENGINE_SUBSCRIPTION_OPTION_SHARED) == 0) ||
                ((requestedSubOptionsValue & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK) == 0))
            {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(rc, "%s%s", ismENGINE_ADMIN_ALLADMINSUBS_SUBOPTIONS, requestedSubOptions);
                goto mod_exit;
            }

            subAttributes.subOptions = requestedSubOptionsValue;
        }
        // We ignore the Name (it is passed in as subscriptionName)
        else if (strncmp(checkPropertyName,
                         ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_NAME,
                         strlen(ismENGINE_ADMIN_PREFIX_ALLADMINSUBS_NAME)) == 0)
        {
            assert(strcmp(subscriptionName, ism_common_getStringProperty(adminProps, propertyName)) == 0);
        }
        // Unexpected property... probably need to extend this parsing (we igno
        else
        {
            rc = ISMRC_BadPropertyName;
            ism_common_setErrorData(rc, "%s", propertyName);
            goto mod_exit;
        }
    }

    iecs_findClientState(pThreadData, nameSpace, false, &pOwningClient);

    if (pOwningClient == NULL)
    {
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(rc, "%s%s", ismENGINE_ADMIN_ALLADMINSUBS_NAMESPACE, nameSpace);
        goto mod_exit;
    }

    assert(pOwningClient->protocolId == PROTOCOL_ID_SHARED);

    iepiPolicyInfo_t *policyInfo;
    ismSecurityPolicyType_t policyType;
    const char *policyTypeName;

    // Subscriptions in the mixed shared namespace should have a name starting with '/'
    // followed by shareName and topic filter (which we currently have as 'subscriptionName'
    // and 'topicString'.
    if (strcmp(pOwningClient->pClientId, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_MIXED) == 0)
    {
        // Set mixed durability (potentially overriding subOptions specified)
        subAttributes.subOptions &= ~ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
        subAttributes.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY;

        messagingPolicyName = subscriptionPolicyName;
        policyType = ismSEC_POLICY_SUBSCRIPTION;
        policyTypeName = ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY;
    }
    // Subscriptions in durable namespace should be durable
    else if (strcmp(pOwningClient->pClientId, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE) == 0)
    {
        subAttributes.subOptions &= ~ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY;
        subAttributes.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;

        messagingPolicyName = subscriptionPolicyName;
        policyType= ismSEC_POLICY_SUBSCRIPTION;
        policyTypeName = ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY;
    }
    // And in the non-durable namespace should be non-durable.
    else
    {
        if (strcmp(pOwningClient->pClientId, ismENGINE_SHARED_SUBSCRIPTION_NAMESPACE_NONDURABLE) != 0)
        {
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", ismENGINE_ADMIN_ALLADMINSUBS_NAMESPACE, pOwningClient->pClientId);
            goto mod_exit;
        }

        subAttributes.subOptions &= ~(ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                      ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY);

        messagingPolicyName = topicPolicyName;
        policyType = ismSEC_POLICY_TOPIC;
        policyTypeName = ismENGINE_ADMIN_VALUE_TOPICPOLICY;
    }

    // Check whether the specified subscription exists and we should just updated it
    rc = iett_findClientSubscription(pThreadData,
                                     pOwningClient->pClientId,
                                     iett_generateClientIdHash(pOwningClient->pClientId),
                                     subscriptionName,
                                     iett_generateSubNameHash(subscriptionName),
                                     &subscription);

    // The subscription already exists, so we just need to check it matches what we want
    // and ensure that it's configured with Admin as a sharer.
    if (rc == OK)
    {
        uint32_t persistentAttrSubOpts = subAttributes.subOptions & ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK;

        policyInfo = ieq_getPolicyInfo(subscription->queueHandle);

        if (topicFilter != NULL && strcmp(subscription->node->topicString, topicFilter) != 0)
        {
            rc = ISMRC_PropertyValueMismatch;
            ism_common_setErrorData(rc, "%s%-s%-s", ismENGINE_ADMIN_ALLADMINSUBS_TOPICFILTER, topicFilter, subscription->node->topicString);
        }
        else if (messagingPolicyName != NULL &&
                 (policyInfo->name == NULL ||
                  strcmp(policyInfo->name, messagingPolicyName) != 0 ||
                  policyInfo->policyType != policyType))
        {
            rc = ISMRC_PropertyValueMismatch;
            ism_common_setErrorData(rc, "%s%-s%-s", policyTypeName, messagingPolicyName, policyInfo->name);
        }
        else if (strcmp(subscription->clientId, pOwningClient->pClientId) != 0)
        {
            rc = ISMRC_PropertyValueMismatch;
            ism_common_setErrorData(rc, "%s%-s%-s", ismENGINE_ADMIN_ALLADMINSUBS_NAMESPACE, pOwningClient->pClientId, subscription->clientId);
        }
        else if (subscription->subOptions != persistentAttrSubOpts)
        {
            if (requestedSubOptions != NULL)
            {
                char existingSubOptions[20];
                sprintf(existingSubOptions, "0x%08x", subscription->subOptions);

                rc = ISMRC_PropertyValueMismatch;
                ism_common_setErrorData(rc, "%s%s%s", ismENGINE_ADMIN_ALLADMINSUBS_SUBOPTIONS, requestedSubOptions, existingSubOptions);
                goto mod_exit;
            }
            else if (requestedMaxQoS != -1 &&
                     (subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK) !=
                     (persistentAttrSubOpts & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK))
            {
                int32_t existingMaxQoS = ((int32_t)subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK)-1;

                rc = ISMRC_PropertyValueMismatch;
                ism_common_setErrorData(rc, "%s%d%d", ismENGINE_ADMIN_ALLADMINSUBS_MAXQUALITYOFSERVICE, requestedMaxQoS, existingMaxQoS);
                goto mod_exit;
            }
            else if (requestedQoSFilter != NULL &&
                     (subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_QOSFILTER_MASK) !=
                     (persistentAttrSubOpts & ismENGINE_SUBSCRIPTION_OPTION_QOSFILTER_MASK))
            {
                const char *existingQoSFilter;

                if ((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY) != 0)
                {
                    existingQoSFilter = ismENGINE_ADMIN_VALUE_QUALITYOFSERVICEFILTER_UNRELIABLEONLY;
                }
                else if ((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY) != 0)
                {
                    existingQoSFilter = ismENGINE_ADMIN_VALUE_QUALITYOFSERVICEFILTER_RELIABLEONLY;
                }
                else
                {
                    existingQoSFilter = ismENGINE_ADMIN_VALUE_QUALITYOFSERVICEFILTER_NONE;
                }

                rc = ISMRC_PropertyValueMismatch;
                ism_common_setErrorData(rc, "%s%-s%-s", ismENGINE_ADMIN_ALLADMINSUBS_QUALITYOFSERVICEFILTER, requestedQoSFilter, existingQoSFilter);
                goto mod_exit;
            }
            // There is an existing subscription that doesn't match and we cannot indicate what specifically
            // doesn't seem to match -- so return a generic error.
            else
            {
                rc = ISMRC_ExistingSubscription;
                ism_common_setError(rc);
                goto mod_exit;
            }
        }

        // TODO: Selection... cannot change selection at the moment

        if (rc != OK) goto mod_exit;

        sharedSubData = iett_getSharedSubData(subscription);

        // Only able to do this for shared subscriptions.
        if (sharedSubData == NULL)
        {
            rc = ISMRC_ShareMismatch;
            ism_common_setError(rc);
            goto mod_exit;
        }

        bool needPersistentUpdate;

        // Get the lock on the shared subscription
        osrc = pthread_spin_lock(&sharedSubData->lock);
        assert(osrc == 0);

        // The subscription has been logically deleted, return as not found
        if ((subscription->internalAttrs & iettSUBATTR_DELETED) != 0)
        {
            rc = ISMRC_NotFound;
            ism_common_setError(rc);
            goto mod_exit;
        }

        assert((subAttributes.subOptions & ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT) == 0);

        rc = iett_shareSubscription(pThreadData,
                                    NULL,
                                    iettANONYMOUS_SHARER_ADMIN,
                                    subscription,
                                    &subAttributes,
                                    &needPersistentUpdate);

        if (rc != OK) goto mod_exit;

        // Need to update the subscription in the store if it had a persistent change and we have finished recovery
        if (needPersistentUpdate == true && ismEngine_serverGlobal.runPhase > EnginePhaseRecovery)
        {
            ismStore_Handle_t hNewSubscriptionProps = ismSTORE_NULL_HANDLE;

            // Only expect persistent subscriptions to require a persistent update
            assert((subscription->internalAttrs & iettSUBATTR_PERSISTENT) != 0);

            rc = iest_updateSubscription(pThreadData,
                                         policyInfo,
                                         subscription,
                                         ieq_getDefnHdl(subscription->queueHandle),
                                         ieq_getPropsHdl(subscription->queueHandle),
                                         &hNewSubscriptionProps,
                                         true);

            if (rc != OK)
            {
                ism_common_setError(rc);
                goto mod_exit;
            }

            assert(hNewSubscriptionProps != ismSTORE_NULL_HANDLE);

            ieq_setPropsHdl(subscription->queueHandle, hNewSubscriptionProps);
        }
    }
    // Need to create the subscription
    else if (rc == ISMRC_NotFound)
    {
        // Must have a topic and policy name if we're creating a new subscription.
        if (topicFilter == NULL)
        {
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", ismENGINE_ADMIN_ALLADMINSUBS_TOPICFILTER, topicFilter);
        }
        else if (messagingPolicyName == NULL)
        {
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", policyTypeName, messagingPolicyName);
        }
        else
        {
            rc = OK;
        }

        if (rc != OK) goto mod_exit;

        ism_prop_t *pSubProperties = NULL;

        assert(subscription == NULL);
        assert(sharedSubData == NULL);

        uint32_t internalAttrs = iettSUBATTR_GLOBALLY_SHARED;

        if ((subAttributes.subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE) != 0 ||
            (subAttributes.subOptions & iettREQUIRE_PERSISTENT_SHARED_MASK) == iettREQUIRE_PERSISTENT_SHARED_MASK)
        {
            internalAttrs |= iettSUBATTR_PERSISTENT;
        }

        rc = iepi_getEngineKnownPolicyInfo(pThreadData,
                                           messagingPolicyName,
                                           policyType,
                                           &policyInfo);

        if (rc != OK)
        {
            if (rc == ISMRC_NotFound)
            {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(rc, "%s%s", policyTypeName, messagingPolicyName);
            }
            else
            {
                ism_common_setError(rc);
            }
            goto mod_exit;
        }

        iettCreateSubscriptionInfo_t createSubInfo = {{pSubProperties}, 0,
                                                      subscriptionName,
                                                      policyInfo,
                                                      topicFilter,
                                                      subAttributes.subOptions,
                                                      subAttributes.subId,
                                                      internalAttrs,
                                                      iettANONYMOUS_SHARER_ADMIN,
                                                      0, NULL, NULL, NULL};

        iettCreateSubscriptionClientInfo_t clientInfo = {NULL, pOwningClient, true};

        pOwningClient = NULL; // The create function will release the clientState, we shouldn't

        ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
            {ismENGINE_ASYNCDATAENTRY_STRUCID, TopicCreateSubscriptionClientInfo,
                &clientInfo, sizeof(clientInfo), NULL,
                {.internalFn = iett_asyncCreateSubscriptionReleaseClients}},
           {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineCaller,
                pContext, contextLength, NULL,
                {.externalFn = pCallbackFn }}};

        ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID,
                                           NULL,
                                           IEAD_MAXARRAYENTRIES, 2, 0, true,  0, 0, asyncArray};

        rc = iett_createSubscription(pThreadData,
                                     &clientInfo,
                                     &createSubInfo,
                                     NULL,
                                     &asyncData);

        // Need to release our useCount on the policy (iett_createSubscription will have
        // acquired one for the subscription if it is being created)
        iepi_releasePolicyInfo(pThreadData, policyInfo);

        if (rc != ISMRC_AsyncCompletion)
        {
            iett_createSubscriptionReleaseClients(pThreadData, &clientInfo);
            ism_common_setError(rc);
        }
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

    if (pOwningClient != NULL)
    {
        iecs_releaseClientStateReference(pThreadData, pOwningClient, false, false);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
// @brief  Update list of subscription sharers
//
// @param[in]     pThreadData           Current thread control data block
// @param[in]     clientId              Client Id to add (might be NULL)
// @param[in]     anonymousSharers      Anonymous sharer types being added
// @param[in]     subscription          Shared subscription to update
// @param[in]     subAttributes         Subscription attributes for this client
// @param[out]    needpersistentUpdate  Whether a persistent update is required
//
// @remark The lock on the shared subscription must already have been taken on entry
//         to this function.
//
// @remark If no clientId is specified, this is treated as a request to ask that the
//         subscription be enabled for anonymous sharing, of type(s) specified by
//         anonymousSharers.
//
// @return OK on successful completion or an ISMRC_ value.
//
// @see ism_engine_reuseSubscription
//****************************************************************************
int32_t iett_shareSubscription(ieutThreadData_t *pThreadData,
                               const char *clientId,
                               uint8_t anonymousSharers,
                               ismEngine_Subscription_t *subscription,
                               const ismEngine_SubscriptionAttributes_t *subAttributes,
                               bool *needPersistentUpdate)
{
    int32_t rc = OK;
    iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);
    bool persistentChange = (subscription->internalAttrs & iettSUBATTR_PERSISTENT) != 0;
    iereResourceSetHandle_t resourceSet = subscription->resourceSet;

    ieutTRACEL(pThreadData, sharedSubData, ENGINE_FNC_TRACE, FUNCTION_ENTRY "clientId='%s', subscription=%p, sharedSubData=%p, subOptions=0x%08x subId=%u\n", __func__,
               clientId, subscription, sharedSubData, subAttributes->subOptions, subAttributes->subId);

    if (clientId != NULL)
    {
        uint32_t clientIdHash = iett_generateClientIdHash(clientId);
        uint32_t index = 0;

        for(; index<sharedSubData->sharingClientCount; index++)
        {
            if (clientIdHash == sharedSubData->sharingClients[index].clientIdHash &&
                !strcmp(sharedSubData->sharingClients[index].clientId, clientId))
            {
                // Found a match
                break;
            }
        }

        iettSharingClient_t *thisSharingClient;

        // If we didn't find the client in the list already, we need to add it
        if (index == sharedSubData->sharingClientCount)
        {
            iere_primeThreadCache(pThreadData, resourceSet);

            if (sharedSubData->sharingClientCount == sharedSubData->sharingClientMax)
            {
                // We use iemem_externalObjs for this memory because it is really just to
                // enable additional consumers on an existing subscription and we want to
                // be able to reallocate it in low memory situations, to help draining the
                // shared subscription.
                iettSharingClient_t *newSharingClients = iere_realloc(pThreadData,
                                                                      resourceSet,
                                                                      IEMEM_PROBE(iemem_externalObjs, 7),
                                                                      sharedSubData->sharingClients,
                                                                      sizeof(iettSharingClient_t) *
                                                                      (sharedSubData->sharingClientMax + iettSHARED_SUBCLIENT_INCREMENT));

                if (newSharingClients == NULL)
                {
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    goto mod_exit;
                }

                sharedSubData->sharingClients = newSharingClients;
                sharedSubData->sharingClientMax += iettSHARED_SUBCLIENT_INCREMENT;
            }

            char *newClientId = iere_malloc(pThreadData,
                                            resourceSet,
                                            IEMEM_PROBE(iemem_externalObjs, 8),
                                            strlen(clientId)+1);

            if (newClientId == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            strcpy(newClientId, clientId);

            // Don't need to persist if we're adding a non-persistent client
            if (persistentChange == true)
            {
                persistentChange = (subAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE) != 0;
            }

            thisSharingClient = &sharedSubData->sharingClients[sharedSubData->sharingClientCount];

            thisSharingClient->clientId = newClientId;
            thisSharingClient->clientIdHash = clientIdHash;
            thisSharingClient->subOptions = subAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARING_CLIENT_PERSISTENT_MASK;
            thisSharingClient->subId = subAttributes->subId;
            sharedSubData->sharingClientCount++;
        }
        else
        {
            uint32_t requestedPersistentSubOptions = subAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARING_CLIENT_PERSISTENT_MASK;
            thisSharingClient = &sharedSubData->sharingClients[index];

            if (thisSharingClient->subOptions != requestedPersistentSubOptions ||
                thisSharingClient->subId != subAttributes->subId)
            {
                // Check that the only options being altered are in the alterable subset
                if ((requestedPersistentSubOptions & ~ismENGINE_SUBSCRIPTION_OPTION_SHARING_CLIENT_ALTERABLE_MASK) !=
                    (thisSharingClient->subOptions & ~ismENGINE_SUBSCRIPTION_OPTION_SHARING_CLIENT_ALTERABLE_MASK))
                {
                    rc = ISMRC_InvalidParameter;
                    ism_common_setError(rc);
                    goto mod_exit;
                }

                // Don't need to persist if the share is not, and was not durable
                if (persistentChange == true)
                {
                    persistentChange = ((subAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE) != 0) ||
                                       ((thisSharingClient->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE) != 0);
                }

                thisSharingClient->subOptions = requestedPersistentSubOptions;
                thisSharingClient->subId = subAttributes->subId;
            }
            else
            {
                // Don't need to persist if nothing has changed
                persistentChange = false;
            }
        }

        // Add the subscription to the requesting client's list of subscriptions
        rc = iett_addSubscription(pThreadData,
                                  subscription,
                                  clientId,
                                  clientIdHash);

        if (rc != OK) goto mod_exit;
    }
    else
    {
        assert(anonymousSharers != iettNO_ANONYMOUS_SHARER); // Can only *add* sharers.

        // Calculate which (if any) new sharers are being set
        uint8_t newSharers = ((sharedSubData->anonymousSharers & anonymousSharers) ^ anonymousSharers);

        sharedSubData->anonymousSharers |= newSharers;

        if (persistentChange == true)
        {
            // Only need to persist if any of the new sharers are persisted values.
            persistentChange = ((newSharers & iettANONYMOUS_SHARER_PERSISTENT_MASK) != 0);
        }
    }

    if (needPersistentUpdate != NULL)
    {
        *needPersistentUpdate = persistentChange;
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d remainingSharers=%u persistentChange=%d\n",
               __func__, rc, sharedSubData->sharingClientCount + (sharedSubData->anonymousSharers == iettNO_ANONYMOUS_SHARER ? 1 : 0), (int)persistentChange);

    return rc;
}

//****************************************************************************
/// @brief  Get suboptions specific to a client id for an MQTT client on
///         a shared sub
///
///
/// @param[in]     pThreadData        Thread performing the request
/// @param[in]     subscription       The subscription to get options for
/// @param[in]     clientId           The client which is added to the subscription
/// @param[out]    pSubOptions        The suboptions for this client
///
/// @remark For a client that hasn't added themselves with add_client... no
///         options will be found.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_getSharedSubOptionsForClientId(ieutThreadData_t *pThreadData,
                                            ismEngine_Subscription_t *subscription,
                                            const char *pClientId,
                                            uint32_t *pSubOptions)
{
    int32_t rc = ISMRC_NotFound;
    iettSharedSubData_t *sharedSubData = NULL;

    sharedSubData = iett_getSharedSubData(subscription);

    if (sharedSubData != NULL)
    {
        uint32_t clientIdHash = iett_generateClientIdHash(pClientId);

        DEBUG_ONLY int osrc = pthread_spin_lock(&sharedSubData->lock);
        assert(osrc == 0);

        // If we have sharing client ids, remove this subscription tidy each one up
        if (sharedSubData->sharingClientCount != 0)
        {
            for(int32_t index=0; index<sharedSubData->sharingClientCount; index++)
            {
                if (sharedSubData->sharingClients[index].clientIdHash == clientIdHash &&
                        !strcmp(sharedSubData->sharingClients[index].clientId, pClientId))
                {
                    *pSubOptions = sharedSubData->sharingClients[index].subOptions;
                    rc = ISMRC_OK;
                    break;
                }
            }
        }

        DEBUG_ONLY int osrc2 = pthread_spin_unlock(&sharedSubData->lock);
        assert(osrc2 == 0);
    }

    return rc;
}
