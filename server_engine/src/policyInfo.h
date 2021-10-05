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
/// @file  policyInfo.h
/// @brief Engine component policy information header file.
//****************************************************************************
#ifndef __ISM_POLICYINFO_DEFINED
#define __ISM_POLICYINFO_DEFINED

#include "engineDeferredFree.h"

// We want this to fit into a uint8_t (so it fits in a gap)
typedef enum __attribute__ ((__packed__)) tag_iepiMaxMsgBehavior_t
{
    RejectNewMessages = 1,
    DiscardOldMessages = 2,
} iepiMaxMsgBehavior_t;

// We also want this to fit into a uint8_t (so it fits in a gap!)
typedef enum __attribute__ ((__packed__)) tag_iepiCreationState_t
{
    CreatedByConfig = 0,
    CreatedByEngine = 1,
    Destroyed = 2,
} iepiCreationState_t;

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Information on policies reported to the engine from the admin/config
///    components.
///////////////////////////////////////////////////////////////////////////////
typedef struct tag_iepiPolicyTypeAdminInfo_t
{
    const char *name;
    const char *namePrefix;
    const char *contextPrefix;
    const char *propertyFormat;
    ismSecurityPolicyType_t type;
} iepiPolicyTypeAdminInfo_t;

static const iepiPolicyTypeAdminInfo_t iepiPolicyTypeAdminInfo[] =
{
    // TopicPolicy
    {
        ismENGINE_ADMIN_VALUE_TOPICPOLICY,
        ismENGINE_ADMIN_PREFIX_TOPICPOLICY_NAME,
        ismENGINE_ADMIN_PREFIX_TOPICPOLICY_CONTEXT,
        ismENGINE_ADMIN_TOPICPOLICY_PROPERTY_FORMAT,
        ismSEC_POLICY_TOPIC,
    },
    // QueuePolicy
    {
        ismENGINE_ADMIN_VALUE_QUEUEPOLICY,
        ismENGINE_ADMIN_PREFIX_QUEUEPOLICY_NAME,
        ismENGINE_ADMIN_PREFIX_QUEUEPOLICY_CONTEXT,
        ismENGINE_ADMIN_QUEUEPOLICY_PROPERTY_FORMAT,
        ismSEC_POLICY_QUEUE,
    },
    // SubscriptionPolicy
    {
        ismENGINE_ADMIN_VALUE_SUBSCRIPTIONPOLICY,
        ismENGINE_ADMIN_PREFIX_SUBSCRIPTIONPOLICY_NAME,
        ismENGINE_ADMIN_PREFIX_SUBSCRIPTIONPOLICY_CONTEXT,
        ismENGINE_ADMIN_SUBSCRIPTIONPOLICY_PROPERTY_FORMAT,
        ismSEC_POLICY_SUBSCRIPTION,
    },
    // SENTINEL
    {
        NULL,
        NULL,
        NULL,
        NULL,
        ismSEC_POLICY_LAST,
    }
};

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Structure specifying message selection to apply with this policy (if
///    no selection exists on the subscription it is used by) -- the structure
///    is followed by the compiled rule and selection string in that order.
///////////////////////////////////////////////////////////////////////////////
typedef struct tag_iepiSelectionInfo_t
{
    ismRule_t    *selectionRule;      ///< Pointer to the rule (follows the structure)
    uint32_t      selectionRuleLen;   ///< Length of the rule
    char         *selectionString;    ///< String specified for selection
} iepiSelectionInfo_t;

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Structure specifying information from config policies referenced by
///    engine objects - this is a mixture of properties that can be specified
///    on a messaging policy, or on a queue definition that can be referred to
///    by different objects inside the engine.
///////////////////////////////////////////////////////////////////////////////
typedef struct tag_iepiPolicyInfo_t
{
    char                          strucId[4];           ///< Strucid 'EPPI'
    uint32_t                      useCount;             ///< Count of users of this policy
    char                         *name;                 ///< External name of the policy
    uint64_t                      maxMessageCount;      ///< Maximum count of messages (0 is unlimited)               (Subscription/Queue)
    uint64_t                      maxMessageBytes;      ///< Maximum total bytes of messages (0 is unlimited)         (RemoteServer)
    uint32_t                      maxMessageTimeToLive; ///< Maximum time to live allowed on a message                (Producer)
    bool                          concurrentConsumers;  ///< Whether to allow concurrent consumers on a queue         (Queue)
    bool                          allowSend;            ///< Whether send (put) is allowed to a queue                 (Queue)
    bool                          DCNEnabled;           ///< Whether disconnected client notification is enabled      (Subscription)
    iepiMaxMsgBehavior_t          maxMsgBehavior;       ///< What to do when maximum message count or bytes buffered  (Subscription/RemoteServer)
    iepiSelectionInfo_t          *defaultSelectionInfo; ///< Policy based default selection information                       (Subscription)
    volatile iepiCreationState_t  creationState;        ///< The creation / deletion state of this policy
    ismSecurityPolicyType_t       policyType;           ///< The policy type for this policy
} iepiPolicyInfo_t;

#define iepiPOLICY_INFO_STRUCID       "EPPI"
#define iepiPOLICY_INFO_STRUCID_ARRAY {'E', 'P', 'P', 'I'}

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iepiPolicyInfo_t(__file)\
    iedm_descriptionStart(__file, iepiPolicyInfo_t, strucId, iepiPOLICY_INFO_STRUCID);\
    iedm_describeMember(char [4],                strucId);\
    iedm_describeMember(uint32_t,                useCount);\
    iedm_describeMember(char *,                  name);\
    iedm_describeMember(uint64_t,                maxMessageCount);\
    iedm_describeMember(uint64_t,                maxMessageBytes);\
    iedm_describeMember(uint32_t,                maxMessageTimeToLive);\
    iedm_describeMember(bool,                    concurrentConsumers);\
    iedm_describeMember(bool,                    allowSend);\
    iedm_describeMember(bool,                    DCNEnabled);\
    iedm_describeMember(iepiMaxMsgBehavior_t,    maxMsgBehavior);\
    iedm_describeMember(iepiSelectionInfo_t *,   defaultSelectionInfo);\
    iedm_describeMember(iepiCreationState_t,     creationState);\
    iedm_describeMember(ismSecurityPolicyType_t, policyType);\
    iedm_descriptionEnd;

// Format string for an internal policy name known by the engine using policyType and name
#define iepiINTERNAL_POLICYNAME_FORMAT "NID-%03d-%s"
// The length of string at the beginning of the internal policy name that enables
// us to distinguish it from a UUID.
#define iepiINTERNAL_POLICYNAME_IDENT_LENGTH strlen("NID-")

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Global structure housing the hash table of policy info structures by
///    unique identifiers (uuids)
///////////////////////////////////////////////////////////////////////////////
typedef struct tag_iepiPolicyInfoGlobal_t
{
    char                    strucId[4];        ///< Structure identifier 'EPGL'
    ieutHashTableHandle_t   knownPolicies;     ///< Restored policy infos by unique identifier
    pthread_mutex_t         knownPoliciesLock; ///< Lock protecting access to the knownPolicies hash
} iepiPolicyInfoGlobal_t;

#define iepiPOLICY_GLOBAL_STRUCID       "EPGL"
#define iepiPOLICY_GLOBAL_STRUCID_ARRAY {'E','P','G','L'}

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iepiPolicyInfoGlobal_t(__file)\
    iedm_descriptionStart(__file, iepiPolicyInfoGlobal_t, strucId, iepiPOLICY_GLOBAL_STRUCID);\
    iedm_describeMember(char [4],               strucId);\
    iedm_describeMember(ieutHashTable_t *,      knownPolicies);\
    iedm_describeMember(pthread_mutex_t,        knownPoliciesLock);\
    iedm_descriptionEnd;

#define iepiINITIAL_POLICYINFO_CAPACITY  193  ///< Initial capacity of policy info hash

#define iepiPOLICY_NAME_MAPPING_FILE "policyNameMapping.txt" ///< Name of file that maps old UUIDs / names to policy names

/* policyInfoRestore.c functions */
int32_t iepi_createKnownPoliciesTable(ieutThreadData_t *pThreadData,
                                      iepiPolicyInfoGlobal_t *policyInfoGlobal);
int32_t iepi_getKnownPolicyInfo(ieutThreadData_t *pThreadData,
                                const char *policyID,
                                iepiPolicyInfoGlobal_t *policyInfoGlobal,
                                iepiPolicyInfo_t **ppPolicyInfo);
int32_t iepi_addKnownPolicyInfo(ieutThreadData_t *pThreadData,
                                const char *policyID,
                                iepiPolicyInfoGlobal_t *policyInfoGlobal,
                                iepiPolicyInfo_t *pPolicyInfo);
int32_t iepi_getEngineKnownPolicyInfo(ieutThreadData_t *pThreadData,
                                      const char *policyName,
                                      ismSecurityPolicyType_t  policyType,
                                      iepiPolicyInfo_t **ppPolicyInfo);
int32_t iepi_validateSubscriptionPolicy(ieutThreadData_t *pThreadData,
                                        ismSecurity_t *secContext,
                                        uint32_t subOptions,
                                        const char * fullSubName,
                                        ismSecurityAuthActionType_t actionType,
                                        void ** ppValidatedPolicy);
int32_t iepi_addEngineKnownPolicyInfo(ieutThreadData_t *pThreadData,
                                      const char *policyName,
                                      ismSecurityPolicyType_t policyType,
                                      iepiPolicyInfo_t *policyInfo);
void iepi_loadPolicyNameMappings(ieutThreadData_t *pThreadData);
char *iepi_findPolicyNameMapping(ieutThreadData_t *pThreadData,
                                 char *SourceString);
void iepi_destroyPolicyNameMappings(ieutThreadData_t *pThreadData,
                                    bool keepFile);

/* policyInfo.c functions */
iepiPolicyInfo_t *iepi_getDefaultPolicyInfo(bool acquireReference);

int32_t iepi_initEnginePolicyInfoGlobal(ieutThreadData_t *pThreadData);
void iepi_destroyPolicyInfoGlobal(ieutThreadData_t *pThreadData,
                                  iepiPolicyInfoGlobal_t *policyInfoGlobal);
void iepi_destroyEnginePolicyInfoGlobal(ieutThreadData_t *pThreadData);

int32_t iepi_createPolicyInfoFromProperties(ieutThreadData_t *pThreadData,
                                            const char *propertyNameFormat,
                                            ism_prop_t *pProperties,
                                            ismSecurityPolicyType_t policyType,
                                            bool configCreation,
                                            bool useNameProperty,
                                            iepiPolicyInfo_t **ppPolicyInfo);
int32_t iepi_createPolicyInfo(ieutThreadData_t *pThreadData,
                              const char *name,
                              ismSecurityPolicyType_t policyType,
                              bool configCreation,
                              iepiPolicyInfo_t *pTemplate,
                              iepiPolicyInfo_t **ppPolicyInfo);
int32_t iepi_copyPolicyInfo(ieutThreadData_t *pThreadData,
                            iepiPolicyInfo_t *pSourcePolicyInfo,
                            char *newName,
                            iepiPolicyInfo_t **ppPolicyInfo);
int32_t iepi_setDefaultSelectorRule(ieutThreadData_t *pThreadData,
                                    iepiPolicyInfo_t *pPolicyInfo,
                                    char *selectionString,
                                    bool *updated);
int32_t iepi_updatePolicyInfoFromProperties(ieutThreadData_t *pThreadData,
                                            iepiPolicyInfo_t *pPolicyInfo,
                                            const char *propertyNameFormat,
                                            ism_prop_t *pProperties,
                                            bool *updated);
void iepi_freePolicyInfo(ieutThreadData_t *pThreadData,
                         iepiPolicyInfo_t *pPolicyInfo,
                         bool deferred);
void iepi_acquirePolicyInfoReference(iepiPolicyInfo_t *pPolicyInfo);
void iepi_releasePolicyInfo(ieutThreadData_t *pThreadData,
                            iepiPolicyInfo_t *pPolicyInfo);

void iepi_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl);
void iepi_dumpPolicyInfo(iepiPolicyInfo_t *pPolicyInfo, iedmDumpHandle_t dumpHdl);
int iepi_policyInfoConfigCallback(ieutThreadData_t *pThreadData,
                                  char *objectType,
                                  int32_t adminInfoIndex,
                                  char *objectIdentifier,
                                  ism_prop_t *changedProps,
                                  ism_ConfigChangeType_t changeType);

int32_t iepi_DA_security_validate_policy(ismSecurity_t *secContext,
                                         ismSecurityAuthObjectType_t objectType,
                                         const char * objectName,
                                         ismSecurityAuthActionType_t actionType,
                                         ism_ConfigComponentType_t compType,
                                         void ** context);
int32_t iepi_DA_security_set_policyContext(const char *name,
                                           ismSecurityPolicyType_t policyType,
                                           ism_ConfigComponentType_t compType,
                                           void *newContext);

#endif /* __ISM_POLICYINFO_DEFINED */

/*********************************************************************/
/* End of policyInfo.h                                               */
/*********************************************************************/
