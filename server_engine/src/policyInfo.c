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
/// @file  policyInfo.c
/// @brief Functions for access to and management of engine view of policies
///        (i.e. the properties defined by config that apply to a engine objects)
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
#include "policyInfo.h"
#include "memHandler.h"
#include "engineDump.h"
#include "engineDeferredFree.h"
#include "engineHashTable.h"

iepiPolicyInfo_t iepiPolicyInfo_DEFAULT =
{
    iepiPOLICY_INFO_STRUCID_ARRAY,
    1,                                       // useCount
    NULL,                                    // name *this must be NULL*
    ismMAXIMUM_MESSAGES_DEFAULT,             // maxMessageCount
    0,                                       // maxMessageBytes
    ismMAXIMUM_MESSAGE_TIME_TO_LIVE_DEFAULT, // maxMessageTimeToLive
    true,                                    // concurrentConsumers
    true,                                    // allowSend
    false,                                   // DCNEnabled
    RejectNewMessages,                       // maxMsgBehavior
    NULL,                                    // defaultSelectionInfo
    CreatedByEngine,                         // creationState
    ismSEC_POLICY_LAST,                      // policyType
};

//****************************************************************************
/// @brief Function to get the address of the default policy info structure
/// @return Address of the default policy info structure
//****************************************************************************
iepiPolicyInfo_t *iepi_getDefaultPolicyInfo(bool acquireReference)
{
    if (acquireReference) iepi_acquirePolicyInfoReference(&iepiPolicyInfo_DEFAULT);

    return &iepiPolicyInfo_DEFAULT;
}

//****************************************************************************
/// @brief Create and initialize the global policy info structures
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @see iepi_destroyEnginePolicyInfo
//****************************************************************************
int32_t iepi_initEnginePolicyInfoGlobal(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;
    iepiPolicyInfoGlobal_t *policyInfoGlobal = NULL;

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Create the structure
    policyInfoGlobal = iemem_calloc(pThreadData,
                                    IEMEM_PROBE(iemem_policyInfo, 4), 1,
                                    sizeof(iepiPolicyInfoGlobal_t));

    if (NULL == policyInfoGlobal)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(policyInfoGlobal->strucId, iepiPOLICY_GLOBAL_STRUCID, 4);

    // Initialize the hash table from the configured security policies
    rc = iepi_createKnownPoliciesTable(pThreadData, policyInfoGlobal);

    if (rc != OK) goto mod_exit;

    // Load any Name mappings that have been provided
    iepi_loadPolicyNameMappings(pThreadData);

    ismEngine_serverGlobal.policyInfoGlobal = policyInfoGlobal;

mod_exit:

    if (rc != OK)
    {
        iepi_destroyPolicyInfoGlobal(pThreadData, policyInfoGlobal);
    }

    ieutTRACEL(pThreadData, policyInfoGlobal,  ENGINE_FNC_TRACE, FUNCTION_EXIT "policyInfoGlobal=%p, rc=%d\n", __func__, policyInfoGlobal, rc);

    return rc;
}

//****************************************************************************
/// @brief Function to free the contents of a policy info structure either
///        immediately, or by deferring it.
//****************************************************************************
void iepi_freePolicyInfo(ieutThreadData_t *pThreadData,
                         iepiPolicyInfo_t *pPolicyInfo,
                         bool deferred)
{
    bool hadDefaultSelection = (pPolicyInfo->defaultSelectionInfo != NULL) &&
                               (pPolicyInfo->defaultSelectionInfo != iepiPolicyInfo_DEFAULT.defaultSelectionInfo);

    ieutTRACEL(pThreadData, deferred, ENGINE_FNC_TRACE,
               FUNCTION_IDENT "pPolicyInfo=%p hadDefaultSelection=%d deferred=%d\n", __func__,
               pPolicyInfo, hadDefaultSelection, (int)deferred);

    // Make it clear this is a policy that has been destroyed
    pPolicyInfo->creationState = Destroyed;

    if (hadDefaultSelection)
    {
        DEBUG_ONLY uint32_t oldValue = __sync_fetch_and_sub(&ismEngine_serverGlobal.policiesWithDefaultSelection, 1);
        assert(oldValue != 0);
    }

    if (!deferred)
    {
        if (hadDefaultSelection) iemem_free(pThreadData, iemem_policyInfo, pPolicyInfo->defaultSelectionInfo);

        iemem_freeStruct(pThreadData, iemem_policyInfo, pPolicyInfo, pPolicyInfo->strucId);
    }
    else
    {
        ieutDeferredFreeList_t *engineDeferredFrees = ismEngine_serverGlobal.deferredFrees;

        if (hadDefaultSelection)
        {
            ieut_addDeferredFree(pThreadData,
                                 engineDeferredFrees,
                                 pPolicyInfo->defaultSelectionInfo, NULL,
                                 iemem_policyInfo,
                                 iereNO_RESOURCE_SET);
        }

        // Invalidate the strucId now although the freeing is deferred
        ieut_addDeferredFree(pThreadData,
                             engineDeferredFrees,
                             pPolicyInfo, pPolicyInfo->strucId,
                             iemem_policyInfo,
                             iereNO_RESOURCE_SET);
    }
}

//****************************************************************************
/// @brief Release a known policy during shutdown
//****************************************************************************
void iepi_releaseKnownPolicy(ieutThreadData_t *pThreadData,
                             char *key,
                             uint32_t keyHash,
                             void *value,
                             void *context)
{
    // If an entry is in the table with a UUID and an internal id, it will only
    // have a single increase in the reference count, so we only release
    // references for ones with the internal id.
    if (value != NULL && strncmp(key,
                                 iepiINTERNAL_POLICYNAME_FORMAT,
                                 iepiINTERNAL_POLICYNAME_IDENT_LENGTH) == 0)
    {
        iepi_releasePolicyInfo(pThreadData, (iepiPolicyInfo_t *)value);
    }
}

//****************************************************************************
/// @brief Clear the hash table of known policies
//****************************************************************************
static inline void iepi_destroyKnownPoliciesTable(ieutThreadData_t *pThreadData,
                                                  iepiPolicyInfoGlobal_t *policyInfoGlobal)
{
    ieutTRACEL(pThreadData, policyInfoGlobal, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(policyInfoGlobal);

    if (policyInfoGlobal->knownPolicies != NULL)
    {
        ismEngine_lockMutex(&policyInfoGlobal->knownPoliciesLock);

        ieutHashTableHandle_t localKnownPolicies = policyInfoGlobal->knownPolicies;

        // Set knownPolicies table to NULL so that when the policies are released,
        // they don't try and remove themselves from the known policies table
        policyInfoGlobal->knownPolicies = NULL;

        ieut_traverseHashTable(pThreadData,
                               localKnownPolicies,
                               iepi_releaseKnownPolicy,
                               NULL);

        ieut_destroyHashTable(pThreadData, localKnownPolicies);
        pthread_mutex_destroy(&policyInfoGlobal->knownPoliciesLock);
    }

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

//****************************************************************************
/// @brief Destroy and the specified policy info global
///
/// @param[in]     policyInfoGlobal  Pointer to the policy info global to destroy
//****************************************************************************
void iepi_destroyPolicyInfoGlobal(ieutThreadData_t *pThreadData,
                                  iepiPolicyInfoGlobal_t *policyInfoGlobal)
{
    if (NULL == policyInfoGlobal) goto mod_exit;

    // NOTE: We do not get the lock (which we are going to destroy) since
    //       no-one else should be using it at the time we destroy it.

    iepi_destroyKnownPoliciesTable(pThreadData, policyInfoGlobal);

    iemem_freeStruct(pThreadData, iemem_policyInfo, policyInfoGlobal, policyInfoGlobal->strucId);

mod_exit:

    return;
}

//****************************************************************************
/// @brief Destroy and remove the global engine policy info global structure
///
/// @remark This will result in all policy infos being discarded regardless
///         of current use counts.
///
/// @see iepi_initEnginePolicyInfoGlobal
//****************************************************************************
void iepi_destroyEnginePolicyInfoGlobal(ieutThreadData_t *pThreadData)
{
    iepiPolicyInfoGlobal_t *policyInfoGlobal = ismEngine_serverGlobal.policyInfoGlobal;

    ieutTRACEL(pThreadData, policyInfoGlobal, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    iepi_destroyPolicyInfoGlobal(pThreadData, policyInfoGlobal);

    if (iepiPolicyInfo_DEFAULT.defaultSelectionInfo != NULL)
    {
        iemem_free(pThreadData, iemem_policyInfo, iepiPolicyInfo_DEFAULT.defaultSelectionInfo);
        iepiPolicyInfo_DEFAULT.defaultSelectionInfo = NULL;
    }

    ismEngine_serverGlobal.policyInfoGlobal = NULL;

    ieutTRACEL(pThreadData, 0, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);
}

//****************************************************************************
/// @brief Set the default selector rule on an existing policy
///
/// @param[in]     pThreadData         Current thread data pointer
/// @param[in]     pPolicyInfo         Pointer to the policy info to update
/// @param[in]     selectionString     The string form of the selector.
/// @param[out]    updated             Whether any actual updates were made to the
///                                    policy info (NULL if not required)
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iepi_setDefaultSelectorRule(ieutThreadData_t *pThreadData,
                                    iepiPolicyInfo_t *pPolicyInfo,
                                    char *selectionString,
                                    bool *updated)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pPolicyInfo=%p selectionString='%s'\n",
               __func__, pPolicyInfo, selectionString);

    iepiSelectionInfo_t *oldSelectionInfo = pPolicyInfo->defaultSelectionInfo;
    iepiSelectionInfo_t *newSelectionInfo = NULL;

    if (oldSelectionInfo == NULL || strcmp(oldSelectionInfo->selectionString, selectionString) != 0)
    {
        if (selectionString[0] != '\0')
        {
            ismRule_t *pSelectionRule = NULL;
            int32_t SelectionRuleLen = 0;

            rc = ism_common_compileSelectRuleOpt(&pSelectionRule,
                                                 (int *)&SelectionRuleLen,
                                                 selectionString,
                                                 SELOPT_Internal);

            if (rc != OK)
            {
                ism_common_setErrorData(rc, "%s", selectionString);
                goto mod_exit;
            }

            if (SelectionRuleLen != 0)
            {
                size_t selectionStringLen = strlen(selectionString)+1;

                newSelectionInfo = iemem_malloc(pThreadData,
                                                IEMEM_PROBE(iemem_policyInfo, 6),
                                                sizeof(iepiSelectionInfo_t) + selectionStringLen + SelectionRuleLen);

                if (newSelectionInfo == NULL)
                {
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                    ism_common_freeSelectRule(pSelectionRule);
                    goto mod_exit;
                }

                char *tmpPtr = (char *)(newSelectionInfo+1);

                assert(((uint64_t)tmpPtr & 0x3UL) == 0);

                memcpy(tmpPtr, pSelectionRule, SelectionRuleLen);
                newSelectionInfo->selectionRule = (ismRule_t *)tmpPtr;
                newSelectionInfo->selectionRuleLen = SelectionRuleLen;
                tmpPtr += SelectionRuleLen;

                ism_common_freeSelectRule(pSelectionRule);

                memcpy(tmpPtr, selectionString, selectionStringLen);
                newSelectionInfo->selectionString = tmpPtr;
                tmpPtr += selectionStringLen;
            }
        }

        // Try and replace the existing defaultSelectionInfo with the new one
        if (oldSelectionInfo != newSelectionInfo)
        {
            bool replacedRule = __sync_bool_compare_and_swap(&pPolicyInfo->defaultSelectionInfo, oldSelectionInfo, newSelectionInfo);

            if (replacedRule)
            {
                ieutDeferredFreeList_t *engineDeferredFrees = ismEngine_serverGlobal.deferredFrees;

                if (oldSelectionInfo != NULL)
                {
                    if (newSelectionInfo == NULL)
                    {
                        DEBUG_ONLY uint32_t oldValue = __sync_fetch_and_sub(&ismEngine_serverGlobal.policiesWithDefaultSelection, 1);
                        assert(oldValue != 0);
                    }

                    if (oldSelectionInfo != iepiPolicyInfo_DEFAULT.defaultSelectionInfo)
                    {
                        ieut_addDeferredFree(pThreadData,
                                             engineDeferredFrees,
                                             oldSelectionInfo, NULL,
                                             iemem_policyInfo,
                                             iereNO_RESOURCE_SET);
                    }
                }
                else
                {
                    assert(newSelectionInfo != NULL);

                    __sync_add_and_fetch(&ismEngine_serverGlobal.policiesWithDefaultSelection, 1);
                }

                assert(rc == OK);

                if (updated != NULL) *updated = true;
            }
            else
            {
                iemem_free(pThreadData, iemem_policyInfo, newSelectionInfo);
                newSelectionInfo = NULL;
                rc = ISMRC_ExistingKey;
                ism_common_setError(rc);
                goto mod_exit;
            }
        }
        else
        {
            assert(oldSelectionInfo == NULL);
        }
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "pPolicyInfo=%p, newSelectionInfo=%p, rc=%d\n",
               __func__, pPolicyInfo, newSelectionInfo, rc);
    return rc;
}

//****************************************************************************
/// @brief Update the values in an iepiPolicyInfo_t with specified properties
///
/// @param[in]     pThreadData         Current thread data pointer
/// @param[in]     pPolicyInfo         Pointer to the policy info to update
/// @param[in]     propertyNameFormat  A sprintf format to use when searching for
///                                    named properties (NULL for simple names).
/// @param[in]     pProperties         Properties to use for update
/// @param[out]    updated             Whether any actual updates were made to the
///                                    policy info (NULL if not required)
///
/// @remark If the properties structure does not contain a value then for an
///         the value remains unchanged.
///
///         The logic here relies on ism_common_get*Property returning the
///         specified default value if either pProperties is NULL, or it does
///         not contain the requested property.
///
/// @remark This function does not apply any limits on overridden properties
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iepi_updatePolicyInfoFromProperties(ieutThreadData_t *pThreadData,
                                            iepiPolicyInfo_t *pPolicyInfo,
                                            const char *propertyNameFormat,
                                            ism_prop_t *pProperties,
                                            bool *updated)
{
    int32_t rc = OK;
    int64_t tempInt64Value;
    uint64_t tempUint64Value;
    int tempIntValue;
    uint32_t tempUint32Value;
    bool tempBoolValue;
    char *propertyName = NULL;
    const char *propertyString = NULL;
    bool policyInfoUpdated = false;

    assert(pPolicyInfo != NULL);
    assert(pPolicyInfo != &iepiPolicyInfo_DEFAULT);
    assert(sizeof(iepiMaxMsgBehavior_t) == sizeof(uint8_t));

    if (NULL == propertyNameFormat) propertyNameFormat = "%s";

    ieutTRACEL(pThreadData, pPolicyInfo, ENGINE_FNC_TRACE, FUNCTION_ENTRY "propertyNameFormat='%s', pPolicyInfo=%p\n",
               __func__, propertyNameFormat, pPolicyInfo);

    propertyName = iemem_malloc(pThreadData,
                                IEMEM_PROBE(iemem_policyInfo, 1),
                                strlen(propertyNameFormat) + ismENGINE_MAX_ADMIN_PROPERTY_LENGTH + 1);

    if (NULL == propertyName)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Get default selection rule string
    sprintf(propertyName, propertyNameFormat, ismENGINE_ADMIN_PROPERTY_DEFAULTSELECTIONRULE);

    propertyString = ism_common_getStringProperty(pProperties, propertyName);

    if (propertyString != NULL)
    {
        rc = iepi_setDefaultSelectorRule(pThreadData, pPolicyInfo, (char *)propertyString, &policyInfoUpdated);

        if (rc != OK) goto mod_exit;
    }

    // Set maxMesageCount
    sprintf(propertyName, propertyNameFormat, ismENGINE_ADMIN_PROPERTY_MAXMESSAGES);

    tempUint64Value = pPolicyInfo->maxMessageCount;

    // There is no ism_common_getUint64Property function call
    if ((tempIntValue = ism_common_getIntProperty(pProperties, propertyName, -1)) > -1)
    {
        pPolicyInfo->maxMessageCount = (uint64_t)tempIntValue;
    }

    if (tempUint64Value != pPolicyInfo->maxMessageCount)
    {
        ieutTRACEL(pThreadData, pPolicyInfo->maxMessageCount, ENGINE_HIGH_TRACE,
                   "maxMessageCount set to %lu\n", pPolicyInfo->maxMessageCount);
        policyInfoUpdated = true;
    }

    // Set maxMsgBehavior
    sprintf(propertyName, propertyNameFormat, ismENGINE_ADMIN_PROPERTY_MAXMESSAGESBEHAVIOR);

    propertyString = ism_common_getStringProperty(pProperties, propertyName);

    if (propertyString != NULL)
    {
        iepiMaxMsgBehavior_t newMaxMsgBehavior = 0;

        // Need to translate the string into an enumeration value
        if (strcmp(propertyString, ismENGINE_ADMIN_VALUE_REJECTNEWMESSAGES) == 0)
        {
            newMaxMsgBehavior = RejectNewMessages;
        }
        else if (strcmp(propertyString, ismENGINE_ADMIN_VALUE_DISCARDOLDMESSAGES) == 0)
        {
            newMaxMsgBehavior = DiscardOldMessages;
        }

        // Update the policy info if the maxMsgBehavior value has changed
        if (newMaxMsgBehavior != 0 && newMaxMsgBehavior != pPolicyInfo->maxMsgBehavior)
        {
            pPolicyInfo->maxMsgBehavior = newMaxMsgBehavior;

            ieutTRACEL(pThreadData, pPolicyInfo->maxMsgBehavior, ENGINE_HIGH_TRACE,
                       "maxMsgBehavior set to %u\n", (uint32_t)pPolicyInfo->maxMsgBehavior);
            policyInfoUpdated = true;
        }
    }

    // Set concurrentConsumers
    sprintf(propertyName, propertyNameFormat, ismENGINE_ADMIN_PROPERTY_CONCURRENTCONSUMERS);

    tempBoolValue = pPolicyInfo->concurrentConsumers;

    pPolicyInfo->concurrentConsumers = ism_common_getBooleanProperty(pProperties,
                                                                     propertyName,
                                                                     tempBoolValue);
    if (tempBoolValue != pPolicyInfo->concurrentConsumers)
    {
        ieutTRACEL(pThreadData, pPolicyInfo->concurrentConsumers, ENGINE_HIGH_TRACE,
                   "concurrentConsumers set to %s\n", pPolicyInfo->concurrentConsumers ? "true":"false");
        policyInfoUpdated = true;
    }

    // Set allowSend
    sprintf(propertyName, propertyNameFormat, ismENGINE_ADMIN_PROPERTY_ALLOWSEND);

    tempBoolValue = pPolicyInfo->allowSend;

    pPolicyInfo->allowSend = ism_common_getBooleanProperty(pProperties,
                                                           propertyName,
                                                           tempBoolValue);
    if (tempBoolValue != pPolicyInfo->allowSend)
    {
        ieutTRACEL(pThreadData, pPolicyInfo->allowSend, ENGINE_HIGH_TRACE,
                   "allowSend set to %s\n", pPolicyInfo->allowSend ? "true":"false");
        policyInfoUpdated = true;
    }

    // Set DCNEnabled attribute
    sprintf(propertyName, propertyNameFormat, ismENGINE_ADMIN_PROPERTY_DISCONNECTEDCLIENTNOTIFICATION);

    tempBoolValue = pPolicyInfo->DCNEnabled;

    pPolicyInfo->DCNEnabled = ism_common_getBooleanProperty(pProperties,
                                                        propertyName,
                                                        tempBoolValue);

    if (tempBoolValue != pPolicyInfo->DCNEnabled)
    {
        ieutTRACEL(pThreadData, pPolicyInfo->DCNEnabled, ENGINE_HIGH_TRACE,
                   "Disconnected Client Notification (DCNEnabled) set to %s\n", pPolicyInfo->DCNEnabled ? "true":"false");
        policyInfoUpdated = true;
    }

    // Get MaxMessageTimeToLive attribute.
    sprintf(propertyName, propertyNameFormat, ismENGINE_ADMIN_PROPERTY_MAXMESSAGETIMETOLIVE);

    tempUint32Value = pPolicyInfo->maxMessageTimeToLive;

    if ((tempInt64Value = ism_common_getLongProperty(pProperties, propertyName, -1)) > -1)
    {
        assert(tempInt64Value <= UINT32_MAX);

        pPolicyInfo->maxMessageTimeToLive = (uint32_t)tempInt64Value;
    }

    if (tempUint32Value != pPolicyInfo->maxMessageTimeToLive)
    {
        ieutTRACEL(pThreadData, pPolicyInfo->maxMessageTimeToLive, ENGINE_HIGH_TRACE,
                   "Max Message Time To Live set to %u\n", pPolicyInfo->maxMessageTimeToLive);
        policyInfoUpdated = true;
    }

mod_exit:

    if (NULL != updated) *updated = policyInfoUpdated;

    if (NULL != propertyName) iemem_free(pThreadData, iemem_policyInfo, propertyName);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return rc;
}

//****************************************************************************
/// @brief Function to create a policy info from an ism_prop_t
///
/// @param[in]     pThreadData         Thread data structure to use
/// @param[in]     propertyNameFormat  A sprintf format to use when searching for
///                                    named properties (NULL for simple names)
/// @param[in]     pProperties         Properties from which to create policy info
/// @param[in]     policyType          The type of policy being created
/// @param[in]     configCreation      Whether this is being created on behalf
///                                    of a configuration request / policy.
/// @param[in]     useNameProperty     Whether the Name property from the properties
///                                    represents the Name of a policy.
/// @param[in]     ppPolicyInfo        Pointer to receive the newly created policy info
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iepi_createPolicyInfoFromProperties(ieutThreadData_t *pThreadData,
                                            const char *propertyNameFormat,
                                            ism_prop_t *pProperties,
                                            ismSecurityPolicyType_t policyType,
                                            bool configCreation,
                                            bool useNameProperty,
                                            iepiPolicyInfo_t **ppPolicyInfo)
{
    int32_t rc = OK;
    iepiPolicyInfo_t *newPolicyInfo;

    assert(ppPolicyInfo != NULL);

    const char *name;

    // If the Name in the passed properties is the policy Name, we need to get it.
    if (useNameProperty)
    {
        if (NULL == propertyNameFormat) propertyNameFormat = "%s";

        char namePropertyName[strlen(propertyNameFormat) + ismENGINE_MAX_ADMIN_PROPERTY_LENGTH + 1];

        sprintf(namePropertyName, propertyNameFormat, ismENGINE_ADMIN_PROPERTY_NAME);

        name = ism_common_getStringProperty(pProperties, namePropertyName);
    }
    else
    {
        name = NULL;
    }

    ieutTRACEL(pThreadData, name, ENGINE_FNC_TRACE, FUNCTION_ENTRY "name=%s, pProperties=%p, ppPolicyInfo=%p\n", __func__,
               name ? name : "<NONE>", pProperties, ppPolicyInfo);

    size_t nameLength = name ? (strlen(name) + 1) : 0;

    newPolicyInfo = iemem_malloc(pThreadData,
                                 IEMEM_PROBE(iemem_policyInfo, 2),
                                 sizeof(iepiPolicyInfo_t) + nameLength);

    if (NULL == newPolicyInfo)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    memcpy(newPolicyInfo, &iepiPolicyInfo_DEFAULT, sizeof(iepiPolicyInfo_t));

    newPolicyInfo->policyType = policyType;

    // Set the creator of this policy
    if (configCreation == true)
    {
        newPolicyInfo->creationState = CreatedByConfig;
    }
    else
    {
        newPolicyInfo->creationState = CreatedByEngine;
    }

    // Fill in the name (which appears just after the policy info structure)
    if (nameLength != 0)
    {
        newPolicyInfo->name = (char *)(newPolicyInfo+1);
        memcpy(newPolicyInfo->name, name, nameLength);
    }
    else
    {
        newPolicyInfo->name = NULL;
    }

    newPolicyInfo->useCount = 1;

    if (pProperties != NULL)
    {
        rc = iepi_updatePolicyInfoFromProperties(pThreadData,
                                                 newPolicyInfo,
                                                 propertyNameFormat,
                                                 pProperties,
                                                 NULL);
    }

    if (rc == OK)
    {
        *ppPolicyInfo = newPolicyInfo;
    }
    else
    {
        iepi_freePolicyInfo(pThreadData, newPolicyInfo, false);
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, *ppPolicyInfo=%p\n", __func__, rc, *ppPolicyInfo);

    return rc;
}

//****************************************************************************
/// @brief Function to create a policy info from a template.
///
/// @param[in]     pThreadData       Thread data to use
/// @param[in]     name              The name for this policy
/// @param[in]     policyType        The type for this policy
/// @param[in]     configCreation    Whether this is being created on behalf
///                                  of a configuration request / policy.
/// @param[in]     pTemplate         A policy info to use as a template
/// @param[in]     ppPolicyInfo      Pointer to receive the newly created / found
//                                   policy info
///
/// @remark Only a subset of attributes are taken from the template specified:
///
///         maxMessageCount
///         maxMsgBehavior
///         DSNEnabled
///
///         Other values come from the default policy info - if all defaults are
///         required, pTemplate can be NULL.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iepi_createPolicyInfo(ieutThreadData_t *pThreadData,
                              const char *name,
                              ismSecurityPolicyType_t policyType,
                              bool configCreation,
                              iepiPolicyInfo_t *pTemplate,
                              iepiPolicyInfo_t **ppPolicyInfo)
{
    int32_t rc = OK;
    iepiPolicyInfo_t *newPolicyInfo;

    assert(ppPolicyInfo != NULL);

    ieutTRACEL(pThreadData, name, ENGINE_FNC_TRACE, FUNCTION_ENTRY "name=%s, pTemplate=%p, ppPolicyInfo=%p\n", __func__,
               name ? name : "<NONE>", pTemplate, ppPolicyInfo);

    size_t nameLength = name ? (strlen(name) + 1) : 0;

    // We need to create the policy from the specified template values / default values
    newPolicyInfo = iemem_malloc(pThreadData,
                                 IEMEM_PROBE(iemem_policyInfo, 3),
                                 sizeof(iepiPolicyInfo_t) + nameLength);

    if (NULL == newPolicyInfo)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Default values
    memcpy(newPolicyInfo, &iepiPolicyInfo_DEFAULT, sizeof(iepiPolicyInfo_t));

    newPolicyInfo->policyType = policyType;

    if (configCreation == true)
    {
        newPolicyInfo->creationState = CreatedByConfig;
    }
    else
    {
        newPolicyInfo->creationState = CreatedByEngine;
    }

    // Fill in the name (which appears just after the policy info structure)
    if (nameLength != 0)
    {
        newPolicyInfo->name = (char *)(newPolicyInfo+1);
        memcpy(newPolicyInfo->name, name, nameLength);
    }
    else
    {
        newPolicyInfo->name = NULL;
    }

    newPolicyInfo->useCount = 1;

    if (pTemplate != NULL)
    {
        newPolicyInfo->maxMessageCount = pTemplate->maxMessageCount;
        newPolicyInfo->maxMsgBehavior = pTemplate->maxMsgBehavior;
        newPolicyInfo->DCNEnabled = pTemplate->DCNEnabled;
    }

    assert(rc == OK);

    *ppPolicyInfo = newPolicyInfo;

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, *ppPolicyInfo=%p\n", __func__, rc, *ppPolicyInfo);

    return rc;
}

//****************************************************************************
/// @brief Function to copy a policy info to another
///
/// @param[in]     pThreadData       Thread data to use
/// @param[in]     pSourcePolicyInfo The source policy info structure
/// @param[in]     newName           The name to give to the new policy
/// @param[in]     ppPolicyInfo      Pointer to receive the newly created policy info
///
/// @remark If no name is specified, the original is copied
/// @remark The useCount of the newly created policy is set to 1
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iepi_copyPolicyInfo(ieutThreadData_t *pThreadData,
                            iepiPolicyInfo_t *pSourcePolicyInfo,
                            char *newName,
                            iepiPolicyInfo_t **ppPolicyInfo)
{
    int32_t rc = OK;
    iepiPolicyInfo_t *newPolicyInfo;

    assert(ppPolicyInfo != NULL);

    ieutTRACEL(pThreadData, pSourcePolicyInfo, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pSourcePolicyInfo=%p, ppPolicyInfo=%p\n", __func__,
               pSourcePolicyInfo, ppPolicyInfo);

    // If a new name value was not specified, use the ones from the source policy
    if (newName == NULL) newName = pSourcePolicyInfo->name;

    size_t nameLength = newName ? strlen(newName) + 1 : 0;

    // We need to create the policy from the source
    newPolicyInfo = iemem_malloc(pThreadData,
                                 IEMEM_PROBE(iemem_policyInfo, 8),
                                 sizeof(iepiPolicyInfo_t) + nameLength);

    if (NULL == newPolicyInfo)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Start by copying the source
    memcpy(newPolicyInfo, pSourcePolicyInfo, sizeof(iepiPolicyInfo_t));

    // Copy or add the name
    char *curChar = (char *)(newPolicyInfo+1);
    if (nameLength != 0)
    {
        newPolicyInfo->name = curChar;
        strcpy(curChar, newName);
        curChar += nameLength;
    }

    newPolicyInfo->useCount = 1;

    *ppPolicyInfo = newPolicyInfo;

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, *ppPolicyInfo=%p\n", __func__, rc, *ppPolicyInfo);

    return rc;
}

//****************************************************************************
/// @brief Configuration callback to handle update of policies
///
/// @param[in]     objectType        Object type specified by config component
/// @param[in]     adminInfoIndex    Which of the policyTypeAdminInfo entries
///                                  this object request is for.
/// @param[in]     objectIdentifier  The property string identifier
/// @param[in]     changedProps      Properties qualified by string identifier
///                                  as a suffix
/// @param[in]     changeType        The type of change being made
///
/// @return OK on successful completion or another ISMRC_ value on error.
//****************************************************************************
int iepi_policyInfoConfigCallback(ieutThreadData_t *pThreadData,
                                  char *objectType,
                                  int32_t adminInfoIndex,
                                  char *objectIdentifier,
                                  ism_prop_t *changedProps,
                                  ism_ConfigChangeType_t changeType)
{
    int rc = OK;

    ieutTRACEL(pThreadData, changeType, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    const char *contextPropertyPrefix = iepiPolicyTypeAdminInfo[adminInfoIndex].contextPrefix;
    const char *propFormatBase = iepiPolicyTypeAdminInfo[adminInfoIndex].propertyFormat;
    ismSecurityPolicyType_t policyType = iepiPolicyTypeAdminInfo[adminInfoIndex].type;

    assert((policyType != ismSEC_POLICY_MESSAGING) && (policyType != ismSEC_POLICY_LAST));

    // Get the policy's name and context (policyInfo) from the properties
    const char *name = objectIdentifier;
    iepiPolicyInfo_t *pPolicyInfo = NULL;
    bool contextIsSet = false;

    // Simplest way to find properties is to loop through looking for the prefix
    if (changedProps != NULL)
    {
        const char *propertyName;
        for (int32_t i = 0; ism_common_getPropertyIndex(changedProps, i, &propertyName) == 0; i++)
        {
            if (strncmp(propertyName, contextPropertyPrefix, strlen(contextPropertyPrefix)) == 0)
            {
                pPolicyInfo = (iepiPolicyInfo_t *)ism_common_getLongProperty(changedProps, propertyName, 0);
                ieutTRACEL(pThreadData, pPolicyInfo, ENGINE_NORMAL_TRACE, "pPolicyInfo=%p\n", pPolicyInfo);
            }

            if (pPolicyInfo != NULL) break;
        }
    }

    // Sanity checks of passed context and report the name passed in
    if (pPolicyInfo != NULL)
    {
        assert(pPolicyInfo->creationState == CreatedByConfig);
        assert(strcmp(pPolicyInfo->name, name) == 0);

        ieutTRACEL(pThreadData, pPolicyInfo, ENGINE_HIGH_TRACE,
                   "Processing '%s' with context %p referring to '%s'.\n",
                   name, pPolicyInfo, pPolicyInfo->name);

        contextIsSet = true;
    }

    // The action taken varies depending on the requested changeType.
    switch(changeType)
    {
        // Remove the policy if it is created by config.
        case ISM_CONFIG_CHANGE_DELETE:
            if (pPolicyInfo)
            {
                // Engine takes over responsibility for policies deleted by config
                pPolicyInfo->creationState = CreatedByEngine;
                iepi_releasePolicyInfo(pThreadData, pPolicyInfo);
                assert(rc == OK);
            }
            else
            {
                rc = ISMRC_InvalidParameter;
                ism_common_setError(rc);
                goto mod_exit;
            }
            break;
        case ISM_CONFIG_CHANGE_NAME:
        case ISM_CONFIG_CHANGE_PROPS:
        {
            char *propertyNameFormat = iemem_malloc(pThreadData,
                                                    IEMEM_PROBE(iemem_policyInfo, 7),
                                                    strlen(propFormatBase) +
                                                    strlen(objectIdentifier) + 1);

            if (propertyNameFormat == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            strcpy(propertyNameFormat, propFormatBase);
            strcat(propertyNameFormat, objectIdentifier);

            if (pPolicyInfo == NULL)
            {
                // Make a check on the existing known policies to see if this is a recreation
                // of one still in use - if it is, we want to turn this into an update.
                rc = iepi_getEngineKnownPolicyInfo(pThreadData, name, policyType, &pPolicyInfo);

                if (rc == OK)
                {
                    assert(pPolicyInfo != NULL);
                    assert(pPolicyInfo->creationState == CreatedByEngine);
                    assert(pPolicyInfo->policyType == policyType);

                    // Config are taking it back
                    pPolicyInfo->creationState = CreatedByConfig;
                }
            }

            // This could be an update, or it could be a new policy
            if (pPolicyInfo)
            {
                rc = iepi_updatePolicyInfoFromProperties(pThreadData,
                                                         pPolicyInfo,
                                                         propertyNameFormat,
                                                         changedProps,
                                                         NULL);
            }
            else
            {
                rc = iepi_createPolicyInfoFromProperties(pThreadData,
                                                         propertyNameFormat,
                                                         changedProps,
                                                         policyType,
                                                         true,
                                                         true,
                                                         &pPolicyInfo);

                if (rc == OK)
                {
                    rc = iepi_addEngineKnownPolicyInfo(pThreadData,
                                                       name,
                                                       policyType,
                                                       pPolicyInfo);

                    if (rc != OK)
                    {
                        iepi_releasePolicyInfo(pThreadData, pPolicyInfo);
                    }
                }
            }

            // Set the engine's context for this policy in the security component
            if (rc == OK && contextIsSet == false)
            {
                // useCount required by context
                assert((pPolicyInfo->useCount >= 1) &&
                       (pPolicyInfo->creationState == CreatedByConfig));

                rc = ismEngine_security_set_policyContext_func(name,
                                                               // TODO: The following should be policyType
                                                               iepiPolicyTypeAdminInfo[adminInfoIndex].type,
                                                               ISM_CONFIG_COMP_ENGINE,
                                                               pPolicyInfo);

                assert(rc == OK);
            }

            if (propertyNameFormat != NULL) iemem_free(pThreadData, iemem_policyInfo, propertyNameFormat);
        }
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
/// @brief Write the descriptions of policy info structures to the dump
///
/// @param[in]     dumpHdl  Pointer to a dump structure
//****************************************************************************
void iepi_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    iedm_describe_iepiPolicyInfoGlobal_t(dump->fp);
    iedm_describe_iepiPolicyInfo_t(dump->fp);
}

//****************************************************************************
/// @brief Function to add the contents of a policy info to a dump
///
/// @param[in]     pPolicyInfo  Pointer to the policy info to dump
/// @param[in]     dumpHdl      Handle to the dump
//****************************************************************************
void iepi_dumpPolicyInfo(iepiPolicyInfo_t *pPolicyInfo,
                         iedmDumpHandle_t dumpHdl)
{
    if (iedm_dumpStartObject((iedmDump_t *)dumpHdl, pPolicyInfo) == true)
    {
        // If this is the default policy, we must use a fixed size, otherwise
        // we can use a variable size which will pick up the UID.
        size_t dumpSize = (pPolicyInfo == &iepiPolicyInfo_DEFAULT) ?
                            sizeof(iepiPolicyInfo_t) :
                            iemem_usable_size(iemem_policyInfo, pPolicyInfo);

        iedm_dumpData((iedmDump_t *)dumpHdl, "iepiPolicyInfo_t", pPolicyInfo, dumpSize);

        iedm_dumpEndObject((iedmDump_t *)dumpHdl, pPolicyInfo);
    }
}

//****************************************************************************
/// @brief Generate a hash value for the specified policy ID key
///
/// @param[in]     key  Policy ID to generate the hash for
///
/// @remark The policy ID is likely to be NID-XXX-Name where XXX is the type
///         of messaging policy - but for the configuration from a previous
///         config file, it could be a UUID from an earlier release.
///
/// @return hash value
//****************************************************************************
static inline uint32_t iepi_generatePolicyIDHash(const char *key)
{
    uint32_t keyHash = 5381;
    char curChar;

    while((curChar = *key++))
    {
        keyHash = (keyHash * 33) ^ curChar;
    }

    return keyHash;
}

//****************************************************************************
/// @brief Function to explicitly acquire a reference on a policy info struct
///
/// @param[in]     pPolicyInfo  Pointer to the policy info to destroy
//****************************************************************************
void iepi_acquirePolicyInfoReference(iepiPolicyInfo_t *pPolicyInfo)
{
    assert(pPolicyInfo != NULL);

    __sync_add_and_fetch(&pPolicyInfo->useCount, 1);
}

//****************************************************************************
/// @brief Function to release a reference on a policy info structure
///
/// @param[in]     pThreadData  Thread data being used
/// @param[in]     pPolicyInfo  Pointer to the policy info to release
///
/// @remark Once released, the policy info structure may be freed at any time
//****************************************************************************
void iepi_releasePolicyInfo(ieutThreadData_t *pThreadData,
                            iepiPolicyInfo_t *pPolicyInfo)
{
    assert(pPolicyInfo != NULL);

    uint32_t oldCount = __sync_fetch_and_sub(&pPolicyInfo->useCount, 1);

    assert(oldCount != 0); // useCount just went negative

    // This was the last user, actually perform the removal
    if (oldCount == 1)
    {
        // Remove this entry from the known policies table before freeing it
        if (pPolicyInfo->policyType != ismSEC_POLICY_LAST)
        {
            assert(pPolicyInfo->name != NULL);

            iepiPolicyInfoGlobal_t *policyInfoGlobal = ismEngine_serverGlobal.policyInfoGlobal;

            // If we still have a knownPolicies table, ensure that this is removed from it.
            if (policyInfoGlobal->knownPolicies != NULL)
            {
                char internalPolicyName[strlen(pPolicyInfo->name)+20];

                // Create a unique identifier from the authority object type and name
                sprintf(internalPolicyName, iepiINTERNAL_POLICYNAME_FORMAT,
                        pPolicyInfo->policyType, pPolicyInfo->name);

                uint32_t internalPolicyNameHash = iepi_generatePolicyIDHash(internalPolicyName);

                ismEngine_lockMutex(&policyInfoGlobal->knownPoliciesLock);

                // It *WILL BE* possible for someone to find this policy from the knownPolicies
                // hash - if they have done, then we don't remove it (or free it)
                if (pPolicyInfo->useCount == 0 && pPolicyInfo->creationState != Destroyed)
                {
                    ieutTRACEL(pThreadData, pPolicyInfo, ENGINE_FNC_TRACE,
                               "%s policyID='%s' pPolicyInfo=%p\n", __func__,
                               internalPolicyName, pPolicyInfo);

                    ieut_removeHashEntry(pThreadData,
                                         policyInfoGlobal->knownPolicies,
                                         internalPolicyName,
                                         internalPolicyNameHash);

                    // always defer free in case someone else still looking at this policy in engine
                    iepi_freePolicyInfo(pThreadData, pPolicyInfo, true);
                }

                ismEngine_unlockMutex(&policyInfoGlobal->knownPoliciesLock);
            }
            // No table left - we must be clearing up - so go ahead and free the policy now.
            else
            {
                iepi_freePolicyInfo(pThreadData, pPolicyInfo, false);
            }
        }
        else
        {
            ieutTRACEL(pThreadData, pPolicyInfo, ENGINE_FNC_TRACE,
                       "%s Name=%s pPolicyInfo=%p\n", __func__,
                       pPolicyInfo->name ? pPolicyInfo->name : "<NONE>", pPolicyInfo);

            // No-one else should see this policy, so we don't defer the free
            iepi_freePolicyInfo(pThreadData, pPolicyInfo, false);
        }
    }

    return;
}

//****************************************************************************
/// @brief Add the policy info to the hash
///
/// @param[in]     pThreadData   Thread data structure to use
/// @param[in]     policyID          Unique identifier of the policy to add
/// @param[in]     policyInfoGlobal  Policy info global structure containing the
///                                  known policies hash.
/// @param[in]     pPolicyInfo   Pointer to the policy info
///
/// @remark The lock covering the known policies table must be held on entry
///         to the call *BUT WE AVOID THIS DURING RESTART WHICH IS SINGLE
///         THREADED*
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iepi_addKnownPolicyInfo(ieutThreadData_t *pThreadData,
                                const char *policyID,
                                iepiPolicyInfoGlobal_t *policyInfoGlobal,
                                iepiPolicyInfo_t *pPolicyInfo)
{
    int32_t rc;

    ieutTRACEL(pThreadData, pPolicyInfo, ENGINE_FNC_TRACE, FUNCTION_ENTRY "policyID='%s'\n",
               __func__, policyID);

    assert(pPolicyInfo->name != NULL && policyInfoGlobal->knownPolicies != NULL);

    rc = ieut_putHashEntry(pThreadData,
                           policyInfoGlobal->knownPolicies,
                           ieutFLAG_DUPLICATE_KEY_STRING,
                           policyID,
                           iepi_generatePolicyIDHash(policyID),
                           pPolicyInfo,
                           0);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Add the policy info to the engine's global table of known policies
///
/// @param[in]     pThreadData     Thread data structure to use
/// @param[in]     policyName      Policy name to add
/// @param[in]     policyType      ismSEC_POLICY_* value of policy type
/// @param[in]     policyInfo      Pointer to the policy info
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iepi_addEngineKnownPolicyInfo(ieutThreadData_t *pThreadData,
                                      const char *policyName,
                                      ismSecurityPolicyType_t policyType,
                                      iepiPolicyInfo_t *policyInfo)
{
    int32_t rc;
    iepiPolicyInfoGlobal_t *policyInfoGlobal = ismEngine_serverGlobal.policyInfoGlobal;
    char internalPolicyName[strlen(policyName)+20];

    // Create a unique identifier from the authority object type and name
    sprintf(internalPolicyName, iepiINTERNAL_POLICYNAME_FORMAT, policyType, policyName);

    ismEngine_lockMutex(&policyInfoGlobal->knownPoliciesLock);

    rc = iepi_addKnownPolicyInfo(pThreadData,
                                 internalPolicyName,
                                 policyInfoGlobal,
                                 policyInfo);

    ismEngine_unlockMutex(&policyInfoGlobal->knownPoliciesLock);

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "policyInfo=%p rc=%d\n",
               __func__, policyInfo, rc);

    return rc;
}

//****************************************************************************
/// @brief Get the policy info with specified unique identifier from the hash
///
/// @param[in]     pThreadData       Thread data structure to use
/// @param[in]     policyID          Unique identifier of policy to get
/// @param[in]     policyInfoGlobal  Policy info global structure containing the
///                                  known policies hash.
/// @param[in]     ppPolicyInfo      Pointer to receive the policy info
///
/// @remark The use count on the returned policy info will be incremented, use
///         iepi_releasePolicyInfo to reduce the use count.
///
/// @remark The lock covering the known policies table must be held on entry
///         to the call *BUT WE AVOID THIS DURING RESTART WHICH IS SINGLE
///         THREADED*
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iepi_getKnownPolicyInfo(ieutThreadData_t *pThreadData,
                                const char *policyID,
                                iepiPolicyInfoGlobal_t *policyInfoGlobal,
                                iepiPolicyInfo_t **ppPolicyInfo)
{
    uint32_t policyIDHash = iepi_generatePolicyIDHash(policyID);

    iepiPolicyInfo_t *pPolicyInfo = NULL;

    int32_t rc = ieut_getHashEntry(policyInfoGlobal->knownPolicies,
                                   policyID,
                                   policyIDHash,
                                   (void**)&pPolicyInfo);

    if (rc == OK)
    {
        __sync_add_and_fetch(&pPolicyInfo->useCount, 1);

        *ppPolicyInfo = pPolicyInfo;
    }

    return rc;
}

//****************************************************************************
/// @brief Get the policy of specified type and name from the engine global
/// policies.
///
/// @param[in]     pThreadData       Thread data structure to use
/// @param[in]     policyName        Name of the policy.
/// @param[in]     policyType        ismSEC_POLICY_* value of policy type
/// @param[in]     ppPolicyInfo      Pointer to receive the policy info
///
/// @remark The use count on the returned policy info will be incremented, use
///         iepi_releasePolicyInfo to release the use count.
///
/// @remark This function assumes that the global known policies table is to be
///         used.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iepi_getEngineKnownPolicyInfo(ieutThreadData_t *pThreadData,
                                      const char *policyName,
                                      ismSecurityPolicyType_t  policyType,
                                      iepiPolicyInfo_t **ppPolicyInfo)
{
    int32_t rc;
    iepiPolicyInfo_t *policyInfo = NULL;
    char internalPolicyName[strlen(policyName)+20];

    // Create a unique identifier from the authority object type and name
    sprintf(internalPolicyName, iepiINTERNAL_POLICYNAME_FORMAT, policyType, policyName);

    iepiPolicyInfoGlobal_t *policyInfoGlobal = ismEngine_serverGlobal.policyInfoGlobal;

    ismEngine_lockMutex(&policyInfoGlobal->knownPoliciesLock);

    rc = iepi_getKnownPolicyInfo(pThreadData,
                                 internalPolicyName,
                                 policyInfoGlobal,
                                 &policyInfo);

    ismEngine_unlockMutex(&policyInfoGlobal->knownPoliciesLock);

    if (rc == OK) *ppPolicyInfo = policyInfo;

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "policyInfo=%p rc=%d\n",
               __func__, policyInfo, rc);

    return rc;
}

//****************************************************************************
/// @brief Wrapper around ismEngine_security_validate_policy_func for
/// subscription policies (taking into consideration which portion of the full
/// subname to validate against)
///
/// @param[in]     pThreadData       Thread data structure to use
/// @param[in]     secContext        Security context to validate against
/// @param[in]     subOptions        Subscription options for the subscription
/// @param[in]     fullSubName       Subscription name in full
/// @param[in]     actionType        The action being validated
/// @param[out]    ppValidatedPolicy Pointer to receive validated policy
///                                  (can be NULL if not required)
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iepi_validateSubscriptionPolicy(ieutThreadData_t *pThreadData,
                                        ismSecurity_t *secContext,
                                        uint32_t subOptions,
                                        const char * fullSubName,
                                        ismSecurityAuthActionType_t actionType,
                                        void ** ppValidatedPolicy)
{
    int32_t rc;

    assert(fullSubName != NULL);
    assert(actionType == ismSEC_AUTH_ACTION_RECEIVE ||
           actionType == ismSEC_AUTH_ACTION_CONTROL);

    const char *checkSubName = fullSubName;

    // If the full subscription name starts with '/' and the subOptions indicate
    // this is a mixed durability subscription, choose a subset to match subscription
    // policies (Everything after the initial slash).
    if (*checkSubName == '/' &&
        (subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY) != 0)
    {
        checkSubName += 1;
        assert(*checkSubName != '\0');
    }

    rc = ismEngine_security_validate_policy_func(secContext,
                                                 ismSEC_AUTH_SUBSCRIPTION,
                                                 checkSubName,
                                                 actionType,
                                                 ISM_CONFIG_COMP_ENGINE,
                                                 ppValidatedPolicy);

    return rc;
}

//****************************************************************************
/// @brief Disabled Authorization version if ism_security_set_policyContext
//****************************************************************************
int32_t iepi_DA_security_set_policyContext(const char *name,
                                           ismSecurityPolicyType_t policyType,
                                           ism_ConfigComponentType_t compType,
                                           void *newContext)
{
    return ISMRC_OK;
}

//****************************************************************************
/// @brief Disabled Authorization version if ism_security_validate_policy
//****************************************************************************
int32_t iepi_DA_security_validate_policy(ismSecurity_t *secContext,
        ismSecurityAuthObjectType_t objectType,
        const char * objectName,
        ismSecurityAuthActionType_t actionType,
        ism_ConfigComponentType_t compType,
        void ** context)
{
    if (context != NULL) *context = NULL;
    return ISMRC_OK;
}
