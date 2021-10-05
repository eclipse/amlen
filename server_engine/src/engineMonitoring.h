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
/// @file  engineMonitoring.h
/// @brief Data types for Engine monitoring
//****************************************************************************
#ifndef __ISM_ENGINEMONITORING_DEFINED
#define __ISM_ENGINEMONITORING_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include <engine.h>           /* Engine external header file         */
#include <engineCommon.h>     /* Engine common internal header file  */
#include <store.h>            /* Store external header file          */
#include <stdint.h>           /* Standard integer defns header file  */
#include <transaction.h>      /* Engine transaction header file      */

/*********************************************************************/
/*                                                                   */
/* DATA TYPES                                                        */
/*                                                                   */
/*********************************************************************/
typedef struct tag_iemnSubscriptionFilters_t
{
    const char *subName;
    const char *clientId;
    ism_regex_t regexClientId;
    const char *topicString;
    const char *messagingPolicyName;
    const char *resourceSetIdentifier;
    uint32_t subOptionsMask;
    uint32_t subOptionsValue;
    uint32_t internalAttrsMask;
    uint32_t internalAttrsValue;
    uint8_t anonymousSharersMask;
    uint8_t anonymousSharersValue;
} iemnSubscriptionFilters_t;

#define iemnSUBSCRIPTION_FILTERS_DEFAULT { NULL, NULL, NULL, NULL, NULL, ismENGINE_SUBSCRIPTION_OPTION_NONE, 0, iettSUBATTR_NONE, 0, iettNO_ANONYMOUS_SHARER, 0 }

typedef struct tag_iemnTopicFilters_t
{
    const char *topicString;
} iemnTopicFilters_t;

#define iemnTOPIC_FILTERS_DEFAULT { NULL }

typedef struct tag_iemnQueueFilters_t
{
    const char *queueName;
    ismQueueScope_t QueueScope;
} iemnQueueFilters_t;

#define iemnQUEUE_FILTERS_DEFAULT { NULL, ismQueueScope_All }

typedef struct tag_iemnClientStateFilters_t
{
    const char *clientId;
    ism_regex_t regexClientId;
    const char *resourceSetIdentifier;
} iemnClientStateFilters_t;

#define iemnCLIENT_STATE_FILTERS_DEFAULT { NULL, NULL }

typedef struct tag_iemnTransactionFilters_t
{
    bool includeState[ietrMAX_TRANSACTION_STATE_VALUE+1]; ///< Which transaction states to include
    bool excludeMQConnectivity;                           ///< Whether to exclude MQConnectivity transactions
    const char *xidString;                                ///< XID string to compare with
} iemnTransactionFilters_t;

typedef struct tag_iemnResourceSetFilters_t
{
    const char *resourceSetId;
} iemnResourceSetFilters_t;

#define iemnRESOURCESET_FILTERS_DEFAULT { NULL }

typedef struct tag_iemnMessagingStatistics_t
{
    ismEngine_MessagingStatistics_t externalStats;  ///< External statistics visible outside the engine
    uint64_t InternalRetainedMessages;              ///< Retained message count including NullRetained
    uint64_t BufferedMessagesWithExpirySet;         ///< Current buffered messages with an expiration set (excluding in-flight)
    uint64_t RetainedMessagesWithExpirySet;         ///< Current retained messages with an expiration set (excluding in-flight)
    uint64_t RemoteServerBufferedMessageBytes;      ///< Current buffered message bytes on remote server queues (including in-flight)
    uint64_t FromForwarderRetainedMessages;         ///< Retained messages published from a forwarder
    uint64_t FromForwarderMessages;                 ///< Non-retained messages published from a forwarder
    uint64_t FromForwarderNoRecipientMessages;      ///< Non-retained messages published from a forwarder with no recipients (potential cluster lookup false positives)
    uint64_t ResourceSetMemoryBytes;                ///< Bytes being tracked in ANY resource set (including default resource set)
} iemnMessagingStatistics_t;

typedef struct tag_iemnClientStateStatistics_t
{
    uint64_t ExpiredClientStates;                   ///< ClientStates that have begun the expiry process
    uint64_t ZombieClientStatesWithExpirySet;       ///< Zombie ClientStates that have a non-zero ExpiryTime set
} iemnClientStateStatistics_t;

#define iemnTRANSACTION_FILTERS_DEFAULT { { false, false, true, false, false, true, true }, true, NULL }

/// @brief  Function to compare two Subscription Monitor records
typedef int32_t (*iemnCompSubscriptionMonitorFunction_t)(const ismEngine_SubscriptionMonitor_t *,
                                                         const ismEngine_SubscriptionMonitor_t *);

/// @brief  Function to compare two Topic Monitor records
typedef int32_t (*iemnCompTopicMonitorFunction_t)(const ismEngine_TopicMonitor_t *,
                                                  const ismEngine_TopicMonitor_t *);

/// @brief  Function to compare two Queue Monitor records
typedef int32_t (*iemnCompQueueMonitorFunction_t)(const ismEngine_QueueMonitor_t *,
                                                  const ismEngine_QueueMonitor_t *);

/// @brief  Function to compare two Transaction Monitor records
typedef int32_t (*iemnCompTransactionMonitorFunction_t)(const ismEngine_TransactionMonitor_t *,
                                                        const ismEngine_TransactionMonitor_t *);

/// @brief  Function to compare two ResourceSet Monitor records
typedef int32_t (*iemnCompResourceSetMonitorFunction_t)(const ismEngine_ResourceSetMonitor_t *,
                                                        const ismEngine_ResourceSetMonitor_t *);

typedef struct tag_iemnGetQueueMonitorContext_t
{
    int32_t rc;
    ismEngineMonitorType_t type;
    bool applyFilters;
    iemnCompQueueMonitorFunction_t comparisonFunction;
    iemnQueueFilters_t *pFilters;
    uint32_t maxResults;
    uint32_t localResultCount;
    ismEngine_QueueMonitor_t *localResults;
} iemnGetQueueMonitorContext_t;

typedef struct tag_iemnGetClientStateMonitorContext_t
{
    int32_t rc;
    ismEngineMonitorType_t type;
    bool requestedOpState[iecsOpStateLAST];
    bool requestedProtocol[PROTOCOL_ID_PLUGIN + 1];
    bool applyFilters;
    bool resultsOverflow;
    iemnClientStateFilters_t *pFilters;
    uint32_t maxResults;
    uint32_t resultCount;
    ismEngine_ClientStateMonitor_t *pResults;
} iemnGetClientStateMonitorContext_t;

typedef struct tag_iemnGetTransactionMonitorContext_t
{
    int32_t rc;
    bool applyFilters;
    iemnCompTransactionMonitorFunction_t comparisonFunction;
    iemnTransactionFilters_t *pFilters;
    uint32_t maxResults;
    uint32_t localResultCount;
    ismEngine_TransactionMonitor_t *localResults;
    ieutThreadData_t *pThreadData;
} iemnGetTransactionMonitorContext_t;

typedef struct tag_iemnGetResourceSetMonitorContext_t
{
    int32_t rc;
    ismEngineMonitorType_t type;
    bool applyFilters;
    iemnResourceSetFilters_t *pFilters;
    iemnCompResourceSetMonitorFunction_t comparisonFunction;
    uint32_t maxResults;
    uint32_t localResultCount;
    ismEngine_ResourceSetMonitor_t *localResults;
    ismEngine_ResourceSetStatistics_t otherStats;
    ism_time_t reportTime;
} iemnGetResourceSetMonitorContext_t;

/*********************************************************************/
/*                                                                   */
/* FUNCTIONS                                                         */
/*                                                                   */
/*********************************************************************/

void iemn_getMessagingStatistics(ieutThreadData_t *pThreadData,
                                 iemnMessagingStatistics_t *pStatistics);
void iemn_getClientStateStatistics(ieutThreadData_t *pThreadData,
                                   iemnClientStateStatistics_t *pStatistics);
int32_t iemn_getResourceSetMonitor(ieutThreadData_t *pThreadData,
                                   ismEngine_ResourceSetMonitor_t **results,
                                   uint32_t *resultCount,
                                   ismEngineMonitorType_t type,
                                   uint32_t maxResults,
                                   ismEngine_ResourceSetMonitor_t *otherSets,
                                   ism_prop_t *filterProperties);

#endif /* __ISM_ENGINEMONITORING_DEFINED */

/*********************************************************************/
/* End of engineMonitoring.h                                         */
/*********************************************************************/
