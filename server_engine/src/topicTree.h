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
/// @file  topicTree.h
/// @brief Defines functions available to engine code outside of the topic tree.
//****************************************************************************
#ifndef __ISM_TOPICTREE_DEFINED
#define __ISM_TOPICTREE_DEFINED

#include <stdint.h>
#include "ismutil.h"
#include "engineInternal.h"
#include "transaction.h"
#include "engineSplitList.h"

#define iettMAX_TOPIC_DEPTH 32  ///< Maximum allowed depth of topic string

#define iettSUBATTR_NONE                 0x00000000  ///< No special attributes
#define iettSUBATTR_PERSISTENT           0x00000001  ///< Internal attribute for persistent subscriptions
#define iettSUBATTR_SYSTEM_TOPIC         0x00000004  ///< Subscription is on a system topic
#define iettSUBATTR_DOLLARSYS_DCN_TOPIC  0x00000008  ///< Subscription matches $SYS/DisconnectedClientNotification
#define iettSUBATTR_SHARE_WITH_CLUSTER   0x00000100  ///< Subscription should be shared with cluster [not persisted]
#define iettSUBATTR_REHYDRATED           0x00000200  ///< Subscription was rehydrated [not persisted, set on at restart]
#define iettSUBATTR_MAPPED_POLICY_NAME   0x00000400  ///< Subscription contained a policy UUID or name that we mapped to another name
#define iettSUBATTR_IMPORTING            0x00000800  ///< Subscription is in the middle of an import request [not persisted]
#define iettSUBATTR_GLOBALLY_SHARED      0x00001000  ///< Subscription is sharable between multiple clients
#define iettSUBATTR_DELETED              0x10000000  ///< Subscription is logically deleted

// Internal attributes to persist
#define iettSUBATTR_PERSISTENT_MASK      (iettSUBATTR_PERSISTENT |\
                                          iettSUBATTR_SYSTEM_TOPIC|\
                                          iettSUBATTR_DOLLARSYS_DCN_TOPIC|\
                                          iettSUBATTR_GLOBALLY_SHARED|\
                                          iettSUBATTR_DELETED)

// Exported internal attributes to honour on import
#define iettSUBATTR_IMPORT_MASK          (iettSUBATTR_PERSISTENT |\
                                          iettSUBATTR_GLOBALLY_SHARED)

//****************************************************************************
/// @brief A list of subscribers returned for a requested topic
//****************************************************************************
typedef struct tag_iettSubscriberList_t
{
    ismEngine_Subscription_t **subscribers;            ///< NULL terminated array of subscribers
    uint32_t                   subscriberCount;        ///< Count of the subscribers
    uint32_t                   subscriberCapacity;     ///< Capacity of the array of subscribers
    ismEngine_RemoteServer_t **remoteServers;          ///< NULL terminated array of remote servers
    uint32_t                   remoteServerCount;      ///< Count of remote servers
    uint32_t                   remoteServerCapacity;   ///< Capacity of the array of remote servers
    iettSubsNodeHandle_t      *subscriberNodes;        ///< Nodes from which subscribers found
    uint32_t                   subscriberNodeCount;    ///< Count of nodes
    uint32_t                   subscriberNodeCapacity; ///< Capacity of the array of nodes
    const char                *topicString;            ///< The topic string for which this is the subscriber list
    uint64_t                   publishSUV;             ///< The value of topic tree subsUpdates at start of publish
    bool                       usingCachedArrays;      ///< Whether the arrays are cached by the caller (and so should not be free'd)
    bool                       requestSelection;       ///< Subscribers are using some form of selection (NO_LOCAL or MESSAGE_SELECTION)
} iettSubscriberList_t;

//****************************************************************************
/// @brief Soft Log Entry for releasing nodes used in a transaction
//****************************************************************************
typedef struct tag_iettSLEReleaseNodes_t
{
    bool                       updateStats;      ///< Whether we should update stats (normally only publish loop)
    bool                       publishRejected;  ///< Publish was rejected by one or more subscriptions
    bool                       publishOK;        ///< The publish loop completed successfully
    iettSubsNodeHandle_t      *subscriberNodes;  ///< Subscriber to be released / updated
    ismEngine_RemoteServer_t **remoteServers;    ///< Remote servers to be released / updated
} iettSLEReleaseNodes_t;

//****************************************************************************
/// @brief Validation type for topic strings
//****************************************************************************
typedef enum tag_iettValidationType_t
{
    iettVALIDATE_FOR_PUBLISH = 1,
    iettVALIDATE_FOR_SUBSCRIBE,
    iettVALIDATE_FOR_TOPICMONITOR
} iettValidationType_t;

//****************************************************************************
/// @brief Additional subscription information for shared subscriptions
//****************************************************************************
typedef struct tag_iettSharingClient_t
{
    char             *clientId;
    uint32_t          clientIdHash;
    uint32_t          subOptions;
    ismEngine_SubId_t subId;
} iettSharingClient_t;

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iettSharingClient_t(__file)\
    iedm_descriptionStart(__file, iettSharingClient_t,,"");\
    iedm_describeMember(char *, clientId);\
    iedm_describeMember(uint32_t, clientIdHash);\
    iedm_describeMember(uint32_t, subOptions);\
    iedm_describeMember(ismEngine_SubId_t, subId);\
    iedm_descriptionEnd;

typedef struct tag_iettSharedSubData_t
{
    pthread_spinlock_t   lock;
    uint8_t              anonymousSharers;
    uint32_t             sharingClientCount;
    uint32_t             sharingClientMax;
    iettSharingClient_t *sharingClients;
} iettSharedSubData_t;

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iettSharedSubData_t(__file)\
    iedm_descriptionStart(__file, iettSharedSubData_t,,"");\
    iedm_describeMember(pthread_sponlock_t, lock);\
    iedm_describeMember(uint8_t, anonymousSharers);\
    iedm_describeMember(uint32_t, sharingClientCount);\
    iedm_describeMember(uint32_t, sharingClientMax);\
    iedm_describeMember(iettSharingClient_t *, sharingClients);\
    iedm_descriptionEnd;

// No anonymous sharers
#define iettNO_ANONYMOUS_SHARER               0x00
// A JMS application has registered as an anonymous sharer of a subscription
#define iettANONYMOUS_SHARER_JMS_APPLICATION  0x01
// Administrative definition has registered as an anonymous sharer of a subscription
#define iettANONYMOUS_SHARER_ADMIN            0x02

// Mask of the sharer flags that must be persisted
#define iettANONYMOUS_SHARER_PERSISTENT_MASK ~(iettANONYMOUS_SHARER_ADMIN)

//********************************************************************************
/// @brief Function to access the shared subscription information for a subscription
//********************************************************************************
static inline iettSharedSubData_t * iett_getSharedSubData(ismEngine_Subscription_t *subscription)
{
    iettSharedSubData_t *sharedSubData;

    if (LIKELY((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) == 0))
    {
        sharedSubData = NULL;
    }
    else
    {
        size_t offset = 0;

        sharedSubData = (iettSharedSubData_t *)(((uint8_t *)(subscription+1))+offset);
    }

    return sharedSubData;
}

//********************************************************************************
/// @brief Function to determine if the subscription is an admin subscription and
/// if so, which type.
//********************************************************************************
static inline const char * iett_getAdminSubscriptionType(ismEngine_Subscription_t *subscription)
{
    const char *adminSubscriptionType;

    if ((subscription->internalAttrs & iettSUBATTR_GLOBALLY_SHARED) == 0)
    {
        adminSubscriptionType = NULL;
    }
    else
    {
        iettSharedSubData_t *sharedSubData = iett_getSharedSubData(subscription);

        if (sharedSubData == NULL || (sharedSubData->anonymousSharers & iettANONYMOUS_SHARER_ADMIN) == 0)
        {
            adminSubscriptionType = NULL;
        }
        // The appropriate type depends on the subscription...
        else if ((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY) != 0)
        {
            adminSubscriptionType = ismENGINE_ADMIN_VALUE_ADMINSUBSCRIPTION;
        }
        else if ((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE) != 0)
        {
            adminSubscriptionType = ismENGINE_ADMIN_VALUE_DURABLENAMESPACEADMINSUB;
        }
        else
        {
            adminSubscriptionType = ismENGINE_ADMIN_VALUE_NONPERSISTENTADMINSUB;
        }
    }

    return adminSubscriptionType;
}

//*********************************************************************
/// @brief Subscription
///
/// Subscribes to messages on a particular topic
//*********************************************************************
typedef struct tag_iettNewSubCreationData_t
{
    uint64_t  retUpdatesValue;            ///< The value of the topic tree retUpdates at subscribe time
    uint64_t  subsUpdatesValue;           ///< The value of the topic tree subsUpdates at subscribe time
} iettNewSubCreationData_t;

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iettNewSubCreationData_t(__file)\
    iedm_descriptionStart(__file, iettNewSubCreationData_t,,"");\
    iedm_describeMember(uint64_t, retUpdatesValue);\
    iedm_describeMember(uint64_t, subsUpdatesValue);\
    iedm_descriptionEnd;

//*********************************************************************************
/// @brief Macro to access creation information for subscriptions created since restart
//*********************************************************************************
#define iett_getNewSubCreationData_MACRO(__subs) \
    (((__subs)->internalAttrs & iettSUBATTR_REHYDRATED) ? NULL : \
     ((__subs)->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) ? (iettNewSubCreationData_t *)(((iettSharedSubData_t *)((__subs)+1))+1) : \
                                                                     (iettNewSubCreationData_t *)((__subs)+1))

//*********************************************************************************
/// @brief Function to access creation information for subscriptions created
///        after restart
//*********************************************************************************
static inline iettNewSubCreationData_t * iett_getNewSubCreationData(ismEngine_Subscription_t *subscription)
{
    iettNewSubCreationData_t *newSubCreationData;

    if ((subscription->internalAttrs & iettSUBATTR_REHYDRATED) != 0)
    {
        newSubCreationData = NULL;
    }
    else
    {
        size_t offset = 0;

        if (UNLIKELY((subscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) != 0))
        {
            offset += sizeof(iettSharedSubData_t);
        }

        newSubCreationData = (iettNewSubCreationData_t *)(((uint8_t *)(subscription+1))+offset);
    }

    return newSubCreationData;
}

//****************************************************************************
/// @brief A topic information node in a topic tree
///
/// @remark The topic substring at this node follows immediately after the
///         structure.
//****************************************************************************
typedef struct tag_iettTopicNode_t
{
    char                                  strucId[4];            ///< Structure identifier 'ETTI'
    uint32_t                              nodeFlags;             ///< Node flags (iettNODE_FLAG_* values)
    iereResourceSetHandle_t               resourceSet;           ///< The resourceSet of which this node is part (if any)
    ieutHashTableHandle_t                 children;              ///< Child nodes
    struct tag_iettTopicNode_t           *parent;                ///< Parent of this node
    ismEngine_MessageHandle_t             currRetMessage;        ///< Current retained message
    ismStore_Handle_t                     currRetRefHandle;      ///< Current retained reference handle
    uint64_t                              currRetOrderId;        ///< Current (confirmed) retained order Id
    ism_time_t                            currRetTimestamp;      ///< Current retained timestamp
    struct tag_iettSLEUpdateRetained_t   *inflightRetUpdates;    ///< Inflight update retained SLEs for this node
    uint64_t                              activeOrderIdVote;     ///< Minimum active retained order Id vote from this node
    ieutSplitListLink_t                   expiryLink;            ///< Linkage for topic nodes in expiry reaper list
    uint32_t                              expiryTime;            ///< Expiry time of the current retained message
    uint32_t                              pendingUpdates;        ///< Number of updates pending on this node
    struct tag_iettOriginServer_t        *currOriginServer;      ///< Origin server record for the current retained
    struct tag_iettTopicNode_t           *originPrev;            ///< Previous node (in timestamp order) for this origin server
    struct tag_iettTopicNode_t           *originNext;            ///< Next node (in timestamp order) for this origin server
} iettTopicNode_t;

#define iettTOPIC_NODE_STRUCID "ETTI"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iettTopicNode_t(__file)\
    iedm_descriptionStart(__file, iettTopicNode_t, strucId, iettTOPIC_NODE_STRUCID);\
    iedm_describeMember(char [4],                  strucId);\
    iedm_describeMember(uint32_t,                  nodeFlags);\
    iedm_describeMember(iereResourceSet_t *,       resourceSet);\
    iedm_describeMember(ieutHashTable_t *,         children);\
    iedm_describeMember(iettTopicNode_t *,         parent);\
    iedm_describeMember(ismEngine_MessageHandle_t, currRetMessage);\
    iedm_describeMember(ismStore_Handle_t,         currRetRefHandle);\
    iedm_describeMember(uint64_t,                  currRetOrderId);\
    iedm_describeMember(ism_time_t,                currRetTimestamp);\
    iedm_describeMember(iettSLEUpdateRetained_t *, inflightRetUpdates);\
    iedm_describeMember(uint64_t,                  activeOrderIdVote);\
    iedm_describeMember(ieutSplitListLink_t *,     expiryLink);\
    iedm_describeMember(uint32_t,                  expiryTime);\
    iedm_describeMember(uint32_t,                  pendingUpdates);\
    iedm_describeMember(iettOriginServer_t *,      currOriginServer);\
    iedm_describeMember(iettTopicNode_t *,         originPrev);\
    iedm_describeMember(iettTopicNode_t *,         originNext);\
    iedm_descriptionEnd;

//****************************************************************************
/// @brief Stats about the originating server of retained messages
//****************************************************************************
typedef struct tag_iettOriginServerStats_t
{
    uint32_t           version;                    ///< The version of the statistics information being kept
    uint32_t           count;                      ///< Count of the retained messages originating from this server
                                                   ///< that would be sent to a remote server
    uint32_t           localCount;                 ///< Count of ALL retained messages originating from this server
    ism_time_t         highestTimestampSeen;       ///< The highest ID_ServerTime seen from this server
    ism_time_t         highestTimestampAvailable;  ///< The highest ID_ServerTime available now from this server
    uint64_t           topicsIdentifier;           ///< Numeric value identifying the topics represented by the list
} iettOriginServerStats_t;

#define iettORIGIN_SERVER_STATS_VERSION_1       1
#define iettORIGIN_SERVER_STATS_CURRENT_VERSION iettORIGIN_SERVER_STATS_VERSION_1

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iettOriginServerStats_t(__file)\
    iedm_descriptionStart(__file, iettOriginServerStats_t,,"");\
    iedm_describeMember(uint32_t,     version);\
    iedm_describeMember(uint32_t,     count);\
    iedm_describeMember(uint32_t,     localCount);\
    iedm_describeMember(ism_time_t,   highestTimestampSeen);\
    iedm_describeMember(ism_time_t,   highestTimestampAvailable);\
    iedm_describeMember(uint64_t,     topicsIdentifier);\
    iedm_descriptionEnd;

typedef struct tag_iettOriginServerStats_V1_t
{
    uint32_t           version;
    uint32_t           count;
    uint32_t           localCount;
    ism_time_t         highestTimestampSeen;
    ism_time_t         highestTimestampAvailable;
    uint64_t           topicsIdentifier;
} iettOriginServerStats_V1_t;

//****************************************************************************
/// @brief Information for an originating server of retained messages
//****************************************************************************
typedef struct tag_iettOriginServer_t
{
    char                      strucId[4];    ///< Structure identifier 'ETOS'
    bool                      localServer;   ///< This is either the local server's current serverUID or a previous one
    char                     *serverUID;     ///< The serverUID for this origin server
    iettTopicNode_t         **recovered;     ///< The set of recovered topic nodes from this originServer
    iettTopicNode_t          *head;          ///< Head of the linked list of nodes with retained messages from this server
    iettTopicNode_t          *tail;          ///< Tail of the linked list
    iettTopicNode_t          *lastAdded;     ///< Position in the linked list of most recently added retained message
    iettOriginServerStats_t   stats;         ///< Statistics for this origin server
} iettOriginServer_t;

#define iettORIGIN_SERVER_STRUCID "ETOS"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iettOriginServer_t(__file)\
    iedm_descriptionStart(__file, iettOriginServer_t, strucId, iettORIGIN_SERVER_STRUCID);\
    iedm_describeMember(char [4],                  strucId);\
    iedm_describeMember(bool,                      localServer);\
    iedm_describeMember(char *,                    serverUID);\
    iedm_describeMember(iettTopicNode_t **,        recovered);\
    iedm_describeMember(iettTopicNode_t *,         head);\
    iedm_describeMember(iettTopicNode_t *,         tail);\
    iedm_describeMember(iettTopicNode_t *,         lastAdded);\
    iedm_describeMember(iettOriginServerStats_t,   stats);\
    iedm_descriptionEnd;

//****************************************************************************
// @brief Information used in the migration of old retained messages
//****************************************************************************
typedef struct tag_iettTopicMigrationInfo_t
{
    char                         strucId[4];           ///< Structure identifier 'ETMI'
    ismStore_Handle_t            storeHandle;          ///< Store handle for this topic definition record
    iettTopicNode_t             *topicNode;            ///< Node in the topic tree for this migrated record
    ietrSLE_Header_t           **inflightSLEs;         ///< Inflight SLEs involving this topic
    ismEngine_Transaction_t    **inflightSLETrans;     ///< Transactions in which inflight SLEs exist
    uint64_t                     inflightSLECount;     ///< Current count of inflight SLEs
    uint64_t                     inflightSLEMax;       ///< Max count of inflight SLEs
} iettTopicMigrationInfo_t;

#define iettTOPIC_MIGRATION_INFO_STRUCID "ETMI"

//****************************************************************************
/// @brief Client information passed to final phase of subscription creation
//****************************************************************************
typedef struct tag_iettCreateSubscriptionClientInfo_t
{
    ismEngine_ClientState_t        *requestingClient;
    ismEngine_ClientState_t        *owningClient;
    bool                            releaseClientState;
} iettCreateSubscriptionClientInfo_t;

//****************************************************************************
/// @brief Subscription properties information passed to subscription creation
//****************************************************************************
typedef union tag_iettSubProps_t
{
    const ism_prop_t *props;
    const char *flat;
} iettSubProps_t;

typedef struct tag_iettCreateSubscriptionInfo_t
{
    iettSubProps_t subProps;
    size_t flatLength;
    const char *subName;
    iepiPolicyInfoHandle_t policyInfo;
    const char *topicString;
    uint32_t subOptions;
    ismEngine_SubId_t subId;
    uint32_t internalAttrs;
    uint8_t anonymousSharer;
    uint32_t sharingClientCount;
    const char **sharingClientIds;
    uint32_t *sharingClientSubOpts;
    ismEngine_SubId_t *sharingClientSubIds;
} iettCreateSubscriptionInfo_t;

//****************************************************************************
/// @brief Callback prototype for the internal iett_listSubscriptions
//****************************************************************************
typedef void (*iett_listSubscriptionCallback_t)(
    ieutThreadData_t                         *pThreadData,
    ismEngine_SubscriptionHandle_t            subHandle,
    const char *                              pSubName,
    const char *                              pTopicString,
    void *                                    properties,
    size_t                                    propertiesLength,
    const ismEngine_SubscriptionAttributes_t *pSubAttributes,
    uint32_t                                  consumerCount,
    void *                                    pContext);

//****************************************************************************
/// @brief Callback prototype for the internal iett_listSubscriptions
//****************************************************************************
typedef bool (*iettTraverseSubscriptionsCallback_t)(ieutThreadData_t *pThreadData,
                                                    ismEngine_Subscription_t *pSubscription,
                                                    void *pContext);

//****************************************************************************
/// @brief Create and initialize the global engine topic tree
///
/// @remark Will replace the existing global topic tree without checking that
///         this has happened.
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @see iett_getEngineTopicTree
/// @see iett_destroyEngineTopicTree
//****************************************************************************
int32_t iett_initEngineTopicTree(ieutThreadData_t *pThreadData);

//****************************************************************************
/// @brief Destroy and remove the global engine topic tree
///
/// @remark This will result in all subscription information being discarded.
///
/// @see iett_getEngineTopicTree
/// @see iett_initEngineTopicTree
//****************************************************************************
void iett_destroyEngineTopicTree(ieutThreadData_t *pThreadData);

//****************************************************************************
// @brief  Initialize the objects that represent the globally shared subscription
//         namespaces.
//
// @param[in]     pThreadData          Current thread control data block
//
// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_initSharedSubNameSpaces(ieutThreadData_t *pThreadData);

//****************************************************************************
/// @brief  Release any client states being held while creating a subscription
///
/// @param[in]     pInfo                  Structure containing required client
///                                       information.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
void iett_createSubscriptionReleaseClients(ieutThreadData_t *pThreadData,
                                           iettCreateSubscriptionClientInfo_t *pInfo);

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
                                                   ismEngine_AsyncDataEntry_t *context);

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
/// @remark pValidatedPolicyInfo might be NULL here, if the request came from
/// __MQConnectivity or a unit test - if it is, then we want to create a
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
                                ismEngine_AsyncData_t *pAsyncInfo);

//****************************************************************************
/// @brief  Destroy Named Subscription owned by a specified clientId
///
/// Destroy a named subscription owned by a specified clientId.
///
/// @param[in]     pThreadData        Thread data structure
/// @param[in]     pClientId          Client Id of the subscription creator
/// @param[in]     pClient            ClientState of the subscription owner (NULL for admin)
/// @param[in]     pSubName           Subscription name
/// @param[in]     pRequestingClient  Client requesting this deletion
/// @param[in]     subDestroyOptions  Options for this destroy request (iettSUB_DESTROY_OPTION_*)
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark There must be no active consumers on the durable subscription
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
/// @see ism_engine_destroySubscription
//****************************************************************************
int32_t iett_destroySubscriptionForClientId(ieutThreadData_t *pThreadData,
                                            const char *pClientId,
                                            ismEngine_ClientState_t *pClient,
                                            const char *pSubName,
                                            ismEngine_ClientState_t *pRequestingClient,
                                            uint32_t subDestroyOptions,
                                            void *pContext,
                                            size_t contextLength,
                                            ismEngine_CompletionCallback_t pCallbackFn);

/** @brief
 * No options.
 */
#define iettSUB_DESTROY_OPTION_NONE                               0x00000000

/** @brief
 * This is a request to entirely destroy an admin subscription
 */
#define iettSUB_DESTROY_OPTION_DESTROY_ADMINSUBSCRIPTION             0x00000001

/** @brief
 * This is a request to only remove the anonymous admin sharer and allow the
 * subscription to be destroyed when clients disconnect.
 */
#define iettSUB_DESTROY_OPTION_ONLY_REMOVE_ANONYMOUS_SHARER_ADMIN    0x00000002

//****************************************************************************
/// @brief Get the topic string from a given subscription
///
/// @param[in]     subscription  Subscription from which to get topic string
///
/// @return pointer to the subscription's associated topic string
//****************************************************************************
char *iett_getSubscriptionTopic(ismEngine_Subscription_t *subscription);

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
                                void *context);

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
                                     ismEngine_SubscriptionHandle_t *phSubscription);

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
                                    const char * pClientId,
                                    const uint32_t clientIdHash,
                                    const char * pSubName,
                                    const uint32_t subNameHash,
                                    ismEngine_SubscriptionHandle_t *phSubscription);

//****************************************************************************
/// @brief Find the clientIds owning or known to be using a subscription
///
/// @param[in]     subscription   Subscription being added
/// @param[out]    pFoundClients  Pointer to receive array of clientIds
///                               (terminated by a NULL entry)
///
/// @return OK on successful completion, ISMRC_NotFound if no clients were found
/// or an ISMRC_ error value.
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
/// @see iett_freeSubscriptionClientIds
//****************************************************************************
int32_t iett_findSubscriptionClientIds(ieutThreadData_t *pThreadData,
                                       ismEngine_SubscriptionHandle_t subscription,
                                       const char ***pFoundClients);

//****************************************************************************
/// @brief Free an array of found clients from iett_findSubscriptionClientIds
///
/// @param[in]     subscription  Subscription being added
/// @param[in]     foundClients  Array of clientIds to free
///
/// @see iett_findSubscriptionClientIds
//****************************************************************************
void iett_freeSubscriptionClientIds(ieutThreadData_t *pThreadData,
                                    const char **foundClients);

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
                               bool *needPersistentUpdate);

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
                             const uint32_t clientIdHash);

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
                             const uint32_t clientIdHash);

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
void iett_acquireSubscriptionReference(ismEngine_Subscription_t *subscription);

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
                               iett_listSubscriptionCallback_t pCallbackFn);

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
                                       iepiPolicyInfoHandle_t policy);

/** @name Flags for the iett_listSubscriptions function
 */

/**@{*/

#define iettFLAG_LISTSUBS_NONE               0x00000000 ///< No special matching or output requirements
#define iettFLAG_LISTSUBS_MATCH_SUBNAME      0x00000001 ///< Subscriptions must match the specified subname
#define iettFLAG_LISTSUBS_MATCH_DCNMSGNEEDED 0x00000002 ///< Subscriptions must have DCN enabled and have new msgs available
#define iettFLAG_LISTSUBS_MATCH_OPTIONS_MASK 0x00000003 ///< Subset of flags that represent match options
#define iettFLAG_LISTSUBS_OUTPUT_PROPERTIES  0x10000000 ///< Callback will be passed subscription properties (default == no)

/**@}*/

//****************************************************************************
/// @brief  Release the subscription
///
/// Decrement the use count for the subscription removing it from the engine
/// topic tree if it's use count drops to zero.
///
/// @param[in]     subscription  Subscription to be released
/// @param[in]     fInline       Whether being called inline for the destroy
///                              function.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_releaseSubscription(ieutThreadData_t *pThreadData,
                                 ismEngine_Subscription_t * subscription,
                                 bool fInline);

//****************************************************************************
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
                           ismEngine_Consumer_t *consumer);

//****************************************************************************
/// @brief Unregister a subscriber previously registered with
///        iett_registerConsumer from the global engine topic tree.
///
/// @param[in]     consumer  Subscriber being registered
///
/// @remark Assumes that the global engine topic tree has been initialised
///         by a call to iett_initEngineTopicTree.
///
/// @see iett_initEngineTopicTree
/// @see iett_registerConsumer
//****************************************************************************
void iett_unregisterConsumer(ieutThreadData_t *pThreadData,
                             ismEngine_Consumer_t *consumer);

//****************************************************************************
/// @brief Get an array of subscribers to a given topic from the global
///        engine topic tree.
///
/// @param[in,out] subscriberList  Pointer to a subscriber list to be filled in.
///
/// @return OK if subscribers found, ISMRC_NotFound if not or another
///         ISMRC_ value on error.
///
/// @remark The subscriberList must be pre-filled with a topic string.
///
/// @remark Upon OK return the subscribers in the list have had their use
///         count incremented, meaning that they will not be removed while the
///         operation is being performed, also, if the message passed in is a
///         retained message we will hold the subscribers lock.
///
///         As a result once the caller no longer needs to refer to the list, it
///         must be released by either a call to to iett_releaseSubscriberList or
///         a pair of calls, one to iett_SLEReplayReleaseNodes and one to to
///         iett_freeSubscriberList.
///
/// @see iett_releaseSubscriberList iett_SLEReplayReleaseNodes iett_freeSubscriberList
//****************************************************************************
int32_t iett_getSubscriberList(ieutThreadData_t *pThreadData,
                               iettSubscriberList_t *subscriberList);

//****************************************************************************
/// @brief Add an array of remote servers with an interest in a given topic
///        from the the global engine topic tree.
///
/// @param[in]     topic           Pre-analysed topic string
/// @param[in,out] subscriberList  Pointer to a list to be added to.
///
/// @return OK if remote servers found, ISMRC_NotFound if not or another
///         ISMRC_ value on error.
///
/// @remark The subscriberList must be pre-filled with a topic string.
///
/// @remark The topic specified is assumed to contain no wildcards.
///
/// @remark Upon OK return the remote servers in the list have had their use
///         count incremented, meaning that they will not be removed while the
///         operation is being performed.
///
///         As a result once the caller no longer needs to refer to the list, it
///         must be released by either a call to to iett_releaseSubscriberList or
///         a pair of calls, one to iett_SLEReplayReleaseNodes and one to to
///         iett_freeSubscriberList.
///
/// @see iett_releaseSubscriberList iett_SLEReplayReleaseNodes iett_freeSubscriberList
//****************************************************************************
int32_t iett_addRemoteServersToSubscriberList(ieutThreadData_t *pThreadData,
                                              iettTopicHandle_t topic,
                                              iettSubscriberList_t *subscriberList);

//****************************************************************************
/// @brief Find all the retained messages that this server has from the
///        specified origin serverUID.
///
/// @param[in]     serverUID           The origin server being requested
/// @param[in]     options             ismENGINE_FORWARD_RETAINED_OPTION_* values
/// @param[in]     timestamp           Timestamp to use in selecting messages
/// @param[in]     ppFoundMessages     Array of messages
/// @param[in]     pFoundMessageCount  Count of the number of messages
///
/// @remark The messages returned in the array will have their use counts
///         incremented by two, persistent messages will also have had their
///         store RefCount incremented by one to ensure that they remain in
///         the store while being processed.
///
///         Once processing has completed, iett_releaseOriginServerRetained
///         should be called to release the array (this will decrement the use
///         count by ONE and the persistent message RefCount by one).
///
/// @return OK on successful completion, ISMRC_NotFound or another
///         ISMRC_ value on error.
//****************************************************************************
int32_t iett_findOriginServerRetainedMessages(ieutThreadData_t *pThreadData,
                                              const char *serverUID,
                                              uint32_t options,
                                              ism_time_t timestamp,
                                              ismEngine_MessageHandle_t **ppFoundMessages,
                                              uint32_t *pFoundMessageCount);

//****************************************************************************
/// @brief Release the array of retained messages from an origin server
///
/// @param[in]     pFoundMessages     Array of messages
//****************************************************************************
void iett_releaseOriginServerRetainedMessages(ieutThreadData_t *pThreadData,
                                              ismEngine_MessageHandle_t *pFoundMessages);

//****************************************************************************
/// @brief Convert an incoming data blob into an iettOriginServerStats_t
///
/// @param[in]     pData              Data blob to be converted
/// @param[in]     dataLength         Length of the data blob
/// @param[out]    originServerStats  Stats structure to fill in
///
/// @return OK on successful completion, or an ISMRC_ value on error.
//****************************************************************************
int32_t iett_convertDataToOriginServerStats(ieutThreadData_t *pThreadData,
                                            void *pData,
                                            uint32_t dataLength,
                                            iettOriginServerStats_t *originServerStats);

//****************************************************************************
/// @brief Compare two origin server stats structures and decide which represents
///        the most up-to-date one
///
/// @param[in]     statsA             First set of stats to compare
/// @param[in]     statsB             Second set of stats to compare
///
/// @return Positive value if statsA is more up-to-date than statsB, Negative
///         if statsB is more up-to-date than statsA or 0 if they are equal.
//****************************************************************************
int32_t iett_compareOriginServerStats(iettOriginServerStats_t *statsA,
                                      iettOriginServerStats_t *statsB);

//****************************************************************************
/// @brief Free any free-able memory pointed to from the subscriber list
///
/// @param[in]     subscriberList  List of subscribers to be cleaned up
///
/// @see iett_getSubscriberList
//****************************************************************************
void iett_freeSubscriberList(ieutThreadData_t *pThreadData,
                             iettSubscriberList_t *subscriberList);

//****************************************************************************
/// @brief Release a list of subscribers previously returned by
///        iett_getSubscriberList and free the list.
///
/// @param[in]     subscriberList  List of subscribers to be released.
///
/// @remark If the subscriber list was produced for a retained message, a call
///         to iett_releaseSubscriberList will be taken as a request to make
///         that message the new retained for the topic. In order to stop the
///         message being retained, the subscriberList message handle should
///         be set to NULL before calling iett_releaseSubscriberList.
///
/// @see iett_getSubscriberList
//****************************************************************************
void iett_releaseSubscriberList(ieutThreadData_t *pThreadData,
                                iettSubscriberList_t *subscriberList);

//****************************************************************************
/// @brief Remove local copy of all retained messages for a given REGEX
///        topic string
///
/// @param[in]     topicString       Topic string.
///
/// @remark This really removes the message / topic nodes from the local topic
///         tree without sending information to other cluster members.
///
/// @return OK if retained message removed successfully, or another
///         ISMRC_ value on error.
//****************************************************************************
int32_t iett_removeLocalRetainedMessages(ieutThreadData_t *pThreadData,
                                         const char *topicString);

//****************************************************************************
/// @brief Callback used when scanning the topic expiry reaper list
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
                                                              void *context);

//****************************************************************************
/// @brief Finish the topic expiry reaper list scan
///
/// @param[in]     pThreadData  Current thread context
/// @param[in,out] context      Context supplied to the callback(s)
//****************************************************************************
void iett_finishReapTopicExpiredMessages(ieutThreadData_t *pThreadData,
                                         void *context);

//****************************************************************************
/// @brief  Republish messages to the specified subscription
///
/// @param[in]     pThreadData      Current thread context
/// @param[in]     subscription     Subscription to deliver messages to
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_republishRetainedMessages(ieutThreadData_t *pThreadData,
                                       ismEngine_Subscription_t *subscription);

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
                                             uint64_t operationRefOrderId);

//****************************************************************************
/// @brief  Rehydrate the queue which will be associated with a durable sub
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
                                       void *pContext);

//****************************************************************************
/// @brief  Rehydrate a durable subscription read from the store
///
/// Recreates a durable subscription from the store.
///
/// @param[in]     recHandle          Store handle of the SPR
/// @param[in]     record             Pointer to the SPR
/// @param[in]     transData          Info about transaction (or NULL)
/// @param[in]     requestingRecord   The SR (queue) that requested this SPR
/// @param[out]    rehydratedRecord   Pointer to data created from this record if
///                                   the recovery code needs to track it
///                                   (unused for Subscriptions)
/// @param[in]     pContext           Context (unused)
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
                                        void *pContext);

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
                                void *pContext);

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
                                     void *pContext);

//****************************************************************************
/// @brief  Update any subscriptions which are flagged as requiring an update
///         due to migration.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_updateMigratedSubscriptions(ieutThreadData_t *pThreadData);

//****************************************************************************
/// @brief  Complete the initalization of the topic tree
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_reconcileEngineTopicTree(ieutThreadData_t *pThreadData);

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
                                     , void *pContext);

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
int32_t iett_reconcileEngineClusterTopics(ieutThreadData_t *pThreadData);

//****************************************************************************
/// @brief  Reconcile the engine retained origin server information with the
///         cluster component
///
/// @remark At the end of recovery we will have rebuilt persistent retained
///         messages, noting the server from which they originated we neeed to
///         inform the cluster component so that the information can be flowed
///         to other servers in the cluster.
///
/// @param[in]     pThreadData     Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_reconcileEngineRetainedOriginServers(ieutThreadData_t *pThreadData);

//****************************************************************************
/// @brief Check whether the specified topic string is valid for either publish
///        or subscribe.
///
/// @param[in]     topicString     Null-terminated topic string to check.
/// @param[in]     validationType  Whether validating for publish, subscribe or
///                                for a topic monitor.
///
/// @remark The string is considered valid for publish unless it contains a
///         wildcard/multicard character, or has more than iettMAX_TOPIC_DEPTH
///         substrings.
///
///         The string is considered valid for a topic monitor if it contains
///         one and only one multicard, at the end of the string.
///
///         The string is considered valid for subscribe unless it has more than
///         iettMAX_TOPIC_DEPTH substrings.
///
/// @remark Note this is an engine specific routine which performs only the
///         validation required by the engine (assuming that maximum string
///         length and UTF-8 correctness have been carried out elsewhere)
///
/// @return true if the string is considered valid by the engine, otherwise false.
//****************************************************************************
bool iett_validateTopicString(ieutThreadData_t *pThreadData,
                              const char *topicString,
                              const iettValidationType_t validationType);

//****************************************************************************
/// @brief  Release subscriber nodes and remote servers used in a transaction
///         also updating any subscriber node stats
///
/// @param[in]     Phase    Which of the commit/rollback phases this is
/// @param[in]     entry    Pointer to the SLE
//****************************************************************************
void iett_SLEReplayReleaseNodes(ietrReplayPhase_t        Phase,
                                ieutThreadData_t        *pThreadData,
                                ismEngine_Transaction_t *pTran,
                                void                    *entry,
                                ietrReplayRecord_t      *pRecord);

//****************************************************************************
/// @brief Initialize the topic tree values in the per thread data structure
///
/// @param[in]     pThreadData  Pointer to the per-thread data structure
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iett_createThreadData(ieutThreadData_t *pThreadData);

//****************************************************************************
/// @brief Release the topic tree values from the per-thread data structure
///
/// @param[in]     pThreadData  Pointer to the per-thread data structure
//****************************************************************************
void iett_destroyThreadData(ieutThreadData_t *pThreadData);

//****************************************************************************
/// @brief Initialize a Subscriber list from the topic thread data arrays
///
/// @param[in]     pSublist     The subscriber list to initialize
///
/// @remark A return code of OK or ISMRC_NotFound indicates that there is no need
///         to call iett_getSubscriberList, the list was initialized from the threads
///         cache, and nothing has changed.
///
///         A return code of ISMRC_NotInThreadCache indicates that a call to
///         iett_getSubscriberList is required, as the topic string did not match
///         the cached value for this thread (or more subscriptions have been added).
///
/// @return OK or ISMRC_NotFound indicating that the cache satisfied the request,
///         or ISMRC_NotInThreadCache indicating that it was not.
//****************************************************************************
int32_t iett_initSublistArrays(ieutThreadData_t *pThreadData,
                               iettSubscriberList_t *pSublist);

//****************************************************************************
/// @brief Update the topic thread data arrays from a subscriber list
///
/// @param[in]     pSublist     The subscriber list to use
/// @param[in]     rc           Return code to decide whether to free
//****************************************************************************
void iett_updateCachedArrays(ieutThreadData_t *pThreadData,
                             iettSubscriberList_t *pSublist,
                             int32_t rc);

//****************************************************************************
/// @brief Activate statistics gathering on the specified topic string
///
/// @param[in]     topicString       Topic string on which to start collecting
///                                  stats (Note: This can be wildcarded)
/// @param[in]     resetActiveStats  Whether to reset already active statistics
///                                  gathering.
///
/// @remark If statistics gathering is already enabled on the specified node, the
///         stats for it are reset by this call only if resetActiveStats is true.
///
/// @return OK on successful completion or another ISMRC_ value on error.
///
/// @see iett_deactivateSubsNodeStats
//****************************************************************************
int32_t iett_activateSubsNodeStats(ieutThreadData_t *pThreadData,
                                   const char *topicString,
                                   bool resetActiveStats);

//****************************************************************************
/// @brief Deactivate statistics gathering on the specified topic node
///
/// @param[in]     topicString    Topic string on which to start collecting stats
///
/// @remark This function turns gathering off, by setting the reset time to zero,
///         but does not remove the stats structure from the node (or zero the stats).
///         When the node is free'd the stats are also free'd.
///
/// @return OK on successful completion or another ISMRC_ value on error.
///
/// @see iett_activateSubsNodeStats
//****************************************************************************
int32_t iett_deactivateSubsNodeStats(ieutThreadData_t *pThreadData,
                                     const char *topicString);

//****************************************************************************
/// @brief Configuration callback to handle topic monitor objects
///
/// @param[in]     changedProps      Properties qualified by string identifier
///                                  as a suffix
/// @param[in]     changeType        The type of change being made
///
/// @return OK on successful completion or another ISMRC_ value on error.
//****************************************************************************
int iett_topicMonitorConfigCallback(ieutThreadData_t *pThreadData,
                                    ism_prop_t *changedProps,
                                    ism_ConfigChangeType_t changeType);

//****************************************************************************
/// @brief Configuration callback to handle cluster requested topics
///
/// @param[in]     changedProps      Properties qualified by string identifier
///                                  as a suffix
/// @param[in]     changeType        The type of change being made
///
/// @return OK on successful completion or another ISMRC_ value on error.
//****************************************************************************
int iett_clusterRequestedTopicsConfigCallback(ieutThreadData_t *pThreadData,
                                              ism_prop_t *changedProps,
                                              ism_ConfigChangeType_t changeType);

//****************************************************************************
/// @brief Activate specified topic string as a cluster requested topic
///
/// @param[in]     topicString       Topic string to activate
/// @param[in]     inRecovery        Whether being called during recovery
///
/// @return OK on successful completion or another ISMRC_ value on error.
///
/// @see iett_deactivateClusterRequestedTopic
//****************************************************************************
int32_t iett_activateClusterRequestedTopic(ieutThreadData_t *pThreadData,
                                           const char *topicString,
                                           bool inRecovery);

//****************************************************************************
/// @brief Deactivate the specified topic string as a cluster requested topic
///
/// @param[in]     topicString    Topic string to deactivate
///
/// @return OK on successful completion or another ISMRC_ value on error.
///
/// @see iett_activateClusterRequestedTopic
//****************************************************************************
int32_t iett_deactivateClusterRequestedTopic(ieutThreadData_t *pThreadData,
                                             const char *topicString);

//****************************************************************************
/// @brief Configuration callback to handle deletion of subscriptions
///
/// @param[in]     objectIdentifier  The property string identifier
/// @param[in]     changedProps      Properties qualified by string identifier
///                                  as a suffix
/// @param[in]     changeType        The type of change being made
/// @param[in]     objectType        Which object type was specified on the call.
///
/// @return OK on successful completion, ISMRC_AsyncCompletion if the delete
///         needs to happen asynchronously or another ISMRC_ value on error.
//****************************************************************************
int iett_subscriptionConfigCallback(ieutThreadData_t *pThreadData,
                                    char *objectIdentifier,
                                    ism_prop_t *changedProps,
                                    ism_ConfigChangeType_t changeType,
                                    const char *objectType);

//****************************************************************************
/// @brief Initialize the topic monitors in the topic tree with admin topic
///        monitor objects.
//****************************************************************************
int32_t iett_reconcileEngineTopicMonitors(ieutThreadData_t *pThreadData);

//****************************************************************************
/// @brief Initialize the cluster requested topics in the topic tree with
/// admin defined cluster requested topics.
//****************************************************************************
int32_t iett_reconcileClusterRequestedTopics(ieutThreadData_t *pThreadData);

//*****************************************************************************
/// @brief Scans the topic tree to determine the lowest minimum active orderid
///        for retained messages and optionally to reposition messages with low
///        orderIds.
///
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
                                     bool allowRepositioning);

//****************************************************************************
/// @brief Write the descriptions of topic tree structures to the dump
///
/// @param[in]     dump          Pointer to a dump structure
//****************************************************************************
void iett_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl);

//****************************************************************************
/// @brief Dump the subscription specified to a dump
///
/// @param[in]     subscription  Subscription to dump
/// @param[in]     dumpHdl       Dump to write to
//****************************************************************************
void iett_dumpSubscription(ieutThreadData_t *pThreadData,
                           ismEngine_Subscription_t *subscription,
                           iedmDumpHandle_t dumpHdl);

//****************************************************************************
/// @brief Dump the subscription and topic node for a specified topic string
///
/// @param[in]     topicString  Topic to dump
/// @param[in]     dumpHdl      Dump to write to
///
/// @returns OK, ISMRC_NotFound if the topic not found or another ISMRC_ on
///          error.
//****************************************************************************
int32_t iett_dumpTopic(ieutThreadData_t *pThreadData,
                       const char *topicString,
                       iedmDumpHandle_t dumpHdl);

//****************************************************************************
/// @brief Dump the subscription & topic subtrees below the specified topic
///        string.
///
/// @param[in]     rootTopicString  Root of the topic subtree to dump (NULL
///                                 for entire tree)
/// @param[in]     dumpHdl          Dump to write to
///
/// @returns OK, ISMRC_NotFound if the root topic string is not found or another
///          ISMRC_ value on error.
//****************************************************************************
int32_t iett_dumpTopicTree(ieutThreadData_t *pThreadData,
                           const char *rootTopicString,
                           iedmDumpHandle_t dumpHdl);

//****************************************************************************
/// @brief Add a remote server to a topic in the global engine topic tree.
///
/// @param[in]     topicString   Topic to which to add remote server
/// @param[in]     remoteServer  Remote server being added
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark Assumes that the global engine topic tree has been initialised
///         by a call to iett_initEngineTopicTree.
///
/// @see iett_initEngineTopicTree
/// @see iett_removeRemoteServerFromEngineTopic
//****************************************************************************
int32_t iett_addRemoteServerToEngineTopic(ieutThreadData_t *pThreadData,
                                          const char *topicString,
                                          ismEngine_RemoteServer_t *remoteServer);

//****************************************************************************
/// @brief Remove a remote server from a topic in the global engine topic tree
///
/// @param[in]     topicString   Topic from which to remove remote server
/// @param[in]     remoteServer  Remote server being removed
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark Assumes that the global engine topic tree has been initialised
///         by a call to iett_initEngineTopicTree.
///
/// @see iett_initEngineTopicTree
/// @see iett_addRemoteServerToEngineTopic
//****************************************************************************
int32_t iett_removeRemoteServerFromEngineTopic(ieutThreadData_t *pThreadData,
                                               const char *topicString,
                                               ismEngine_RemoteServer_t *remoteServer);

//****************************************************************************
/// @brief Remove all references to a remote server from the global engine
///        topic tree
///
/// @param[in]     remoteServer  Remote server being removed
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark Assumes that the global engine topic tree has been initialised
///         by a call to iett_initEngineTopicTree.
///
/// @see iett_initEngineTopicTree
//****************************************************************************
int32_t iett_purgeRemoteServerFromEngineTopicTree(ieutThreadData_t *pThreadData,
                                                  ismEngine_RemoteServer_t *remoteServer);

//****************************************************************************
/// @brief Generate a hash value for a specified subscription name for use in
///        the topic tree.
///
/// @param[in]     key  The key for which to generate a hash value
///
/// @return The hash value
//****************************************************************************
uint32_t iett_generateSubNameHash(const char *subName);

//****************************************************************************
/// @brief Generate a hash value for a specified client Id for use in the
///        topic tree.
///
/// @param[in]     key  The key for which to generate a hash value
///
/// @return The hash value
//****************************************************************************
uint32_t iett_generateClientIdHash(const char *clientId);

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
                                            uint32_t *pSubOptions);

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
int32_t iett_initializeRecovery(ieutThreadData_t *pThreadData);

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
int32_t iett_reconcileAdminSharedSubscriptions(ieutThreadData_t *pThreadData);

#endif /* __ISM_TOPICTREE_DEFINED */

/*********************************************************************/
/* End of topicTree.h                                           */
/*********************************************************************/
