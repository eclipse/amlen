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

//****************************************************************************
/// @file  exportSubscription.h
/// @brief Functions to export subscription information
//****************************************************************************
#ifndef __ISM_EXPORTSUBSCRIPTION_DEFINED
#define __ISM_EXPORTSUBSCRIPTION_DEFINED

#include "engineInternal.h"
#include "engineHashTable.h"
#include "exportResources.h"
#include "exportCrypto.h"
#include "topicTree.h"
#include "topicTreeInternal.h"

//****************************************************************************
/// @brief Information exported about a subscription.
///
/// @remark Following after this structure in an exported file will be, in order:
///
/// - char[]      ClientId                  - Client Identifier for this clientState
/// - char[]      TopicString               - The topic the subscription is on
/// - char[]      SubscriptionName          - Name of the subscription
/// - char[]      SubProperties             - Subscription properties
/// - char[]      PolicyName                - The topic/subscription policy name
/// - char[]      SharingClientIds          - Clients sharing a globally shared sub
/// - uint32_t[]  SharingClientSubOptions   - SubOptions specified by the sharing clients
///
/// Character arrays (and lengths) include a NULL terminator.
//****************************************************************************
typedef struct tag_ieieSubscriptionInfo_t
{
    uint32_t                    Version;                       ///< The version of subscription import/export information
    ismQueueType_t              QueueType;                     ///< Queue Type
    uint32_t                    SubOptions;                    ///< Subscription options
    uint32_t                    InternalAttrs;                 ///< Internal attributes
    uint32_t                    ClientIdLength;                ///< Length of the clientId owning the subscription
    uint32_t                    TopicStringLength;             ///< Length of the topic for the subscription
    uint32_t                    SubNameLength;                 ///< Length of the subscription name
    uint32_t                    SubPropertiesLength;           ///< Length of optional subscription properties
    uint64_t                    MaxMessageCount;               ///< Maximum count of messages for this subscription
    uint32_t                    PolicyNameLength;              ///< Length of the policy name
    bool                        DCNEnabled;                    ///< Disconnected Client Notification value
    uint8_t                     MaxMsgBehavior;                ///< Behavior used when Maximum message limit hit (renamed at v6)
    uint8_t                     AnonymousSharers;              ///< Anonymous sharers that have registered with this subscription (e.g. JMS)
    uint32_t                    SharingClientCount;            ///< Count of explicitly sharing clients
    uint64_t                    SharingClientIdsLength;        ///< Length of the sharing client IDs array (containing SharingClientCount null-terminated strings)
    uint64_t                    SharingClientSubOptionsLength; ///< Length of the sharing client subOptions array (containing subOption values)
    // Version 2+
    ismEngine_SubId_t           SubId;                         ///< Subscription identifier
    uint64_t                    SharingClientSubIdsLength;     ///< Length of the sharing client subIds array
} ieieSubscriptionInfo_t;

#define ieieSUBSCRIPTION_VERSION_1           1
#define ieieSUBSCRIPTION_VERSION_2           2
#define ieieSUBSCRIPTION_CURRENT_VERSION     ieieSUBSCRIPTION_VERSION_2

typedef struct tag_ieieSubscriptionInfo_V1_t
{
    uint32_t                    Version;                       ///< The version of subscription import/export information
    ismQueueType_t              QueueType;                     ///< Queue Type
    uint32_t                    SubOptions;                    ///< Subscription options
    uint32_t                    InternalAttrs;                 ///< Internal attributes
    uint32_t                    ClientIdLength;                ///< Length of the clientId owning the subscription
    uint32_t                    TopicStringLength;             ///< Length of the topic for the subscription
    uint32_t                    SubNameLength;                 ///< Length of the subscription name
    uint32_t                    SubPropertiesLength;           ///< Length of optional subscription properties
    uint64_t                    MaxMessageCount;               ///< Maximum count of messages for this subscription
    uint32_t                    PolicyNameLength;              ///< Length of the policy name
    bool                        DCNEnabled;                    ///< Disconnected Client Notification value
    uint8_t                     MaxMsgBehavior;                ///< Behavior used when Maximum message limit hit (renamed at v6)
    bool                        GenericallyShared;             ///< Whether the subscription is shared between generic consumers (like JMS)
    uint32_t                    SharingClientCount;            ///< Count of explicitly sharing clients
    uint64_t                    SharingClientIdsLength;        ///< Length of the sharing client IDs array (containing SharingClientCount null-terminated strings)
    uint64_t                    SharingClientSubOptionsLength; ///< Length of the sharing client subOptions array (containing subOption values)
} ieieSubscriptionInfo_V1_t;

//****************************************************************************
/// @brief Context structure used when releasing imported subscriptions
//****************************************************************************
typedef struct tag_ieieReleaseImportedSubContext_t
{
    uint32_t releasedCount;      ///< Count of the total released
    iettTopicTree_t *tree;       ///< Topic Tree pointer for convenience
} ieieReleaseImportedSubContext_t;

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
                                 ieieExportResourceControl_t *control);

//****************************************************************************
/// @internal
///
/// @brief  Release Subscriptions that were added during import
///
/// @param[in]     key            DataId
/// @param[in]     keyHash        Hash of the key value
/// @param[in]     value          pClient imported
/// @param[in]     context        unused
///
/// @remark Note if the value is NULL it means that this a subscription
/// which we chose not to import - in this case there is no work to do.
//****************************************************************************
int32_t ieie_releaseImportedSubscription(ieutThreadData_t *pThreadData,
                                         char *key,
                                         uint32_t keyHash,
                                         void *value,
                                         void *context);

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
                                     ismQHandle_t *pQHandle);

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
                                size_t dataLen);

#endif
