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
/// @file  topicTreeInternal.h
/// @brief Defines structures and functions used internally by the topic tree
///        code - for 'externals' see topicTree.h.
/// @see topicTree.h
//****************************************************************************
#ifndef __ISM_ENGINE_TOPICTREE_INTERNAL_DEFINED
#define __ISM_ENGINE_TOPICTREE_INTERNAL_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "memHandler.h"
#include "transaction.h"
#include "engineHashTable.h"
#include "engineHashSet.h"
#include "engineInternal.h"
#include "engineNotifications.h"

/*********************************************************************/
/* Structures and constants from topicTree.c                         */
/*********************************************************************/
#define iettNODE_FLAG_NORMAL                   0x00000000  ///< A non-wildcard topic tree node
#define iettNODE_FLAG_WILDCARD                 0x00000001  ///< A single-level wildcard topic tree node
#define iettNODE_FLAG_MULTICARD                0x00000002  ///< A multi-level wildcard topic tree node
#define iettNODE_FLAG_TREE_ROOT                0x00000004  ///< The root topic tree node
#define iettNODE_FLAG_DELETED                  0x00000010  ///< A deleted node (only created at rehydration)
#define iettNODE_FLAG_TYPE_MASK                0x00000017  ///< Mask used to determine the type of this node
#define iettNODE_FLAG_NULLRETAINED             0x00000020  ///< Node contains a NullRetained message at the moment
#define iettNODE_FLAG_CLUSTER_REQUESTED_TOPIC  0x00000040  ///< Node is one of the set of cluster requested topics
#define iettNODE_FLAG_INACTIVE                 0x00000100  ///< A node that has been identified as inactive (should be removed when possible)
#define iettNODE_FLAG_BRANCH_WILD_OR_MULTI     0x10000000  ///< A wildcard or multicard appears somewhere on this branch
#define iettNODE_FLAG_BRANCH_MULTIMULTI        0x20000000  ///< Multiple multicards appear somewhere on this branch
#define iettNODE_FLAG_BRANCH_SYSTOPIC          0x40000000  ///< This is a branch under a SYSTEM topic (one that starts $)

///< Node flags indicating that this should not be included in the external origin server stats
#define iettNODE_FLAG_LOCAL_ORIGIN_STATS_ONLY_MASK (iettNODE_FLAG_BRANCH_SYSTOPIC)


#define iettINITIAL_SUBSCRIBER_ARRAY_CAPACITY       10     ///< Initial capacity of arrays at each node of subscription tree
#define iettINITIAL_SUBSCRIBER_NODE_CAPACITY        2      ///< Initial capacity of child hash table at each node in of subscription tree
#define iettINITIAL_REMOTE_SERVER_ARRAY_CAPACITY    10     ///< Initial capacity of array at each node of remote server tree
#define iettINITIAL_REMOTE_SERVER_NODE_CAPACITY     2      ///< Initial capacity of child hash table at each node of remote server tree
#define iettINITIAL_TOPICS_NODE_CAPACITY            2      ///< Initial capacity of child hash table at each node of topics tree
#define iettNODE_LOADINGFACTOR_HIGH_WATER           5      ///< (integral) Loading factor (count/capacity) at which to resize child hash table (all trees)
#define iettNODE_CAPACITY_INCREMENT_FACTOR          10     ///< Factor by which the capacity of the child table is increased upon resize (all trees)
#define iettINITIAL_NAMEDSUB_HASH_CAPACITY          1000   ///< Initial capacity of the named subscriptions hash table
#define iettFAN_IN_LISTCOUNT_BOUNDARY               100    ///< Number of requests at which to consider subscriber node to be on a 'fan-in' topic
#define iettSHARED_SUBCLIENT_INCREMENT              10     ///< Number of additional shared sub owner client slots to allocate when full
#define iettINITIAL_ORIGINSERVER_HASH_CAPACITY      100    ///< Initial capacity of the origin server UID hash table

#define iettFIND_SUBSCRIBER_RESULTS_INCREMENT       20     ///< How much to increment the array of matching subscriber nodes
#define iettFIND_REMOTE_SERVER_RESULTS_INCREMENT    20     ///< How much to increment the array of matching remote server nodes
#define iettFIND_TOPIC_RESULTS_INCREMENT            20     ///< How much to increment the array of matching topics nodes

#define iettFLAG_REMOVE_SUB_NONE            0x00000000  ///< No special flags to iett_removeSubFromEngineTopic
#define iettFLAG_REMOVE_SUB_ALREADY_LOCKED  0x00000001  ///< Subscriber lock already held
#define iettFLAG_REMOVE_SUB_ROLLBACK_ADD    0x00000002  ///< Being called as part of rolling back iett_addSubToEngineTopic
#define iettFLAG_REMOVE_SUB_INLINE_DESTROY  0x00000004  ///< Removal is being done inline for the a destroy sub

#define iettRMR_STATE_NONE                  0x00 ///< Store reference state with no information (either remote, or pre-State setting)
#define iettRMR_STATE_ORIGINSERVER_LOCAL    0x01 ///< Store reference state indicating the retained message originated locally

#define iettREQUIRE_PERSISTENT_SHARED_MASK (ismENGINE_SUBSCRIPTION_OPTION_SHARED | \
                                            ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY)

#define iettTRAVERSE_SUBSCRIPTION_RETAKE_LOCK_AFTER 50000 ///< After examining this many subscriptions, release and retake the lock

//****************************************************************************
/// @brief A list of subscriptions associated with a subs topic node
//****************************************************************************
typedef struct tag_iettSubscriptionList_t
{
    uint32_t                   count;        ///< Count of current subscriptions in the list
    uint32_t                   max;          ///< Capacity of the subscription list
    ismEngine_Subscription_t **list;         ///< List of subscriptions
} iettSubscriptionList_t;

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iettSubscriptionList_t(__file)\
    iedm_descriptionStart(__file, iettSubscriptionList_t,,"");\
    iedm_describeMember(uint32_t,                    count);\
    iedm_describeMember(uint32_t,                    max);\
    iedm_describeMember(ismEngine_Subscription_t **, list);\
    iedm_descriptionEnd;

//****************************************************************************
/// @brief A link between a client and a subscription
//****************************************************************************
typedef struct tag_iettClientSubscription_t
{
    uint32_t subNameHash;
    uint32_t unused;
    ismEngine_Subscription_t *subscription;
} iettClientSubscription_t;

//****************************************************************************
/// @brief A list of subscriptions for a specific client
//****************************************************************************
typedef struct tag_iettClientSubscriptionList_t
{
    uint32_t                  count;        ///< Count of current subscriptions in the list
    uint32_t                  max;          ///< Capacity of the subscription list
    iettClientSubscription_t *list;         ///< List of subscriptions
} iettClientSubscriptionList_t;

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iettClientSubscriptionList_t(__file)\
    iedm_descriptionStart(__file, iettClientSubscriptionList_t,,"");\
    iedm_describeMember(uint32_t,                    count);\
    iedm_describeMember(uint32_t,                    max);\
    iedm_describeMember(ismEngine_Subscription_t **, list);\
    iedm_descriptionEnd;

//****************************************************************************
/// @brief Statistics to keep for a subscription node
//****************************************************************************
typedef struct tag_iettSubsNodeStats_t
{
    struct tag_iettSubsNode_t      *node;             ///< The node with which this stats structure is associated
    struct tag_iettSubsNodeStats_t *next;             ///< next node stats structure
    struct tag_iettSubsNodeStats_t *prev;             ///< prev node stats structure
    ismEngine_TopicStatistics_t     topicStats;       ///< Actual statistics
} iettSubsNodeStats_t;

//****************************************************************************
/// @brief A subscription information node in a topic tree
//****************************************************************************
typedef struct tag_iettSubsNode_t
{
    char                        strucId[4];      ///< Structure identifier 'ETSI'
    uint32_t                    nodeFlags;       ///< Node flags (iettNODE_FLAG* values)
    char                       *topicString;     ///< The full topic string for this node
    ieutHashTable_t            *children;        ///< Child nodes
    struct tag_iettSubsNode_t  *wildcardChild;   ///< Single-level wildcard child
    struct tag_iettSubsNode_t  *multicardChild;  ///< Multi-level wildcard child
    struct tag_iettSubsNode_t  *parent;          ///< Parent of this node
    iettSubsNodeStats_t        *stats;           ///< Statistics for structure for this node
    iettSubscriptionList_t      activeSubs;      ///< List of active subscriptions
    iettSubscriptionList_t      delPendSubs;     ///< List of subscriptions pending deletion
    uint32_t                    useCount;        ///< How many lists refer to this node
    uint32_t                    listCount;       ///< How many times this node was included in a list
    uint32_t                    totalSubsCount;  ///< Count of the total active subscriptions at OR BELOW this node
    uint32_t                    activeSelection; ///< Count of active subscriptions using ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION
                                                 ///  or ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL
    uint32_t                    activeCluster;   ///< Count of active subscriptions which should be shared
                                                 ///  with remote servers in a clustered environment (also incremented if
                                                 ///  nodeFlags contains iettNODE_FLAG_CLUSTER_REQUESTED_TOPIC)
} iettSubsNode_t;

#define iettSUBSCRIPTION_NODE_STRUCID "ETSI"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iettSubsNode_t(__file)\
    iedm_descriptionStart(__file, iettSubsNode_t, strucId, iettSUBSCRIPTION_NODE_STRUCID);\
    iedm_describeMember(char [4],               strucId);\
    iedm_describeMember(uint32_t,               nodeFlags);\
    iedm_describeMember(char *,                 topicString);\
    iedm_describeMember(ieutHashTable_t *,      children);\
    iedm_describeMember(iettSubsNode_t *,       wildcardChild);\
    iedm_describeMember(iettSubsNode_t *,       multicardChild);\
    iedm_describeMember(iettSubsNode_t *,       parent);\
    iedm_describeMember(iettSubsNodeStats_t *,  stats);\
    iedm_describeMember(iettSubscriptionList_t, activeSubs);\
    iedm_describeMember(iettSubscriptionList_t, delPendSubs);\
    iedm_describeMember(uint32_t,               useCount);\
    iedm_describeMember(uint32_t,               listCount);\
    iedm_describeMember(uint32_t,               totalSubsCount);\
    iedm_describeMember(uint32_t,               activeSelection);\
    iedm_describeMember(uint32_t,               activeCluster);\
    iedm_descriptionEnd;

// Macro to use to determine whether this subscription node might be inactive
#define iettQUICK_INACTIVE_SUBSNODE_TEST(node) (((node)->useCount == 0) && \
                                                ((node)->activeSubs.count == 0) && \
                                                ((node)->delPendSubs.count == 0) && \
                                                ((node)->activeCluster == 0) && \
                                                ((node)->stats == NULL || (node)->stats->topicStats.ResetTime == 0))

//****************************************************************************
/// @brief A list of remote servers
//****************************************************************************
typedef struct tag_iettRemoteServerList_t
{
    uint32_t                   count;        ///< Count of current remote servers in the list
    uint32_t                   max;          ///< Capacity of the remote server list
    ismEngine_RemoteServer_t **list;         ///< List of remote servers
} iettRemoteServerList_t;

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iettRemoteServerList_t(__file)\
    iedm_descriptionStart(__file, iettRemoteServerList_t,,"");\
    iedm_describeMember(uint32_t,                    count);\
    iedm_describeMember(uint32_t,                    max);\
    iedm_describeMember(ismEngine_RemoteServer_t **, list);\
    iedm_descriptionEnd;

//****************************************************************************
/// @brief A remote server information node in a topic tree
//****************************************************************************
typedef struct tag_iettRemSrvNode_t
{
    char                          strucId[4];      ///< Structure identifier 'ETSI'
    uint32_t                      nodeFlags;       ///< Node flags (iettNODE_FLAG* values)
    char                         *topicSubstring;  ///< The topic substring for this node
    ieutHashTable_t              *children;        ///< Child nodes
    struct tag_iettRemSrvNode_t  *wildcardChild;   ///< Single-level wildcard child
    struct tag_iettRemSrvNode_t  *multicardChild;  ///< Multi-level wildcard child
    struct tag_iettRemSrvNode_t  *parent;          ///< Parent of this node
    iettRemoteServerList_t        activeServers;   ///< List of active remote servers
} iettRemSrvNode_t;

#define iettREMOTE_SERVER_NODE_STRUCID "ETRI"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iettRemSrvNode_t(__file)\
    iedm_descriptionStart(__file, iettRemSrvNode_t, strucId, iettREMOTE_SERVER_NODE_STRUCID);\
    iedm_describeMember(char [4],               strucId);\
    iedm_describeMember(uint32_t,               nodeFlags);\
    iedm_describeMember(char *,                 topicSubstring);\
    iedm_describeMember(ieutHashTable_t *,      children);\
    iedm_describeMember(iettRemSrvNode_t *,     wildcardChild);\
    iedm_describeMember(iettRemSrvNode_t *,     multicardChild);\
    iedm_describeMember(iettRemSrvNode_t *,     parent);\
    iedm_describeMember(iettRemoteServerList_t, activeServers);\
    iedm_descriptionEnd;

//****************************************************************************
/// @brief The root of a topic tree
//****************************************************************************
typedef struct tag_iettTopicTree_t
{
    char                      strucId[4];           ///< Structure identifier 'ETRE'
    uint32_t                  cacheUpdates;         ///< Count of updates that affect per-thread sublist caches
    iettSubsNode_t           *subs;                 ///< Top of the tree of subscription information nodes
    pthread_rwlock_t          subsLock;             ///< Lock on the subscriptions in the tree (protects subs)
    iettRemSrvNode_t         *remoteServers;        ///< Top of the tree of remote server information nodes
    pthread_rwlock_t          remoteServersLock;    ///< Lock on the remote servers in the tree (protects remoteServers)
    ieutHashTable_t          *namedSubs;            ///< Named subscriptions by clientId
    pthread_rwlock_t          namedSubsLock;        ///< Lock on the named subscriptions hash (protects namedSubs)
    iettTopicNode_t          *topics;               ///< Top of the tree of topic information nodes
    pthread_rwlock_t          topicsLock;           ///< Lock on the topics in the tree (protects topics & originServers)
    ieutHashTable_t          *originServers;        ///< Origin server entries listing topic nodes for each server
    void                     *retRefContext;        ///< Reference context for retained messages in the store
    uint64_t                  retMinActiveOrderId;  ///< The retained messages minimum active order Id last given to the store
    uint64_t                  retNextOrderId;       ///< Next retained order Id to use
    uint64_t                  retUpdates;           ///< Count of the retained messages published to the entire tree
    uint64_t                  subsUpdates;          ///< Count of updates to the subscription information nodes
    uint64_t                  multiMultiSubs;       ///< Count of subscriptions which have multiple multicards in their topic string
    uint64_t                  multiMultiRemSrvs;    ///< Count of remote servers which have multiple multicards in their topic string
    uint64_t                  topicsUpdates;        ///< Count of updates to the topics information nodes
    ismEngine_Subscription_t *subscriptionHead;     ///< Head of linked list of subscribers
    iettSubsNodeStats_t      *subNodeStatsHead;     ///< Head of linked list of subNodeStats structures
    uint32_t                  activeSubNodeStats;   ///< Count of _active_ subNodeStats structures (i.e. topic monitors)
    ismStore_Handle_t         retStoreHandle;       ///< Store handle for retained message topic definition record
} iettTopicTree_t;

#define iettTOPIC_TREE_STRUCID "ETRE"

// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_iettTopicTree_t(__file)\
    iedm_descriptionStart(__file, iettTopicTree_t, strucId, iettTOPIC_TREE_STRUCID);\
    iedm_describeMember(char [4],                   strucId);\
    iedm_describeMember(uint32_t,                   cacheUpdates);\
    iedm_describeMember(iettSubsNode_t *,           subs);\
    iedm_describeMember(pthread_rwlock_t,           subsLock);\
    iedm_describeMember(iettRemSrvNode_t *,         remoteServers);\
    iedm_describeMember(pthread_rwlock_t,           remoteServersLock);\
    iedm_describeMember(ieutHashTable_t *,          namedSubs);\
    iedm_describeMember(pthread_rwlock_t,           namedSubsLock);\
    iedm_describeMember(iettTopicNode_t *,          topics);\
    iedm_describeMember(pthread_rwlock_t,           topicsLock);\
    iedm_describeMember(ieutHashTable_t *,          originServers);\
    iedm_describeMember(void *,                     retRefContext);\
    iedm_describeMember(uint64_t,                   retMinActiveOrderId);\
    iedm_describeMember(uint64_t,                   retNextOrderId);\
    iedm_describeMember(uint64_t,                   retUpdates);\
    iedm_describeMember(uint64_t,                   subsUpdates);\
    iedm_describeMember(uint64_t,                   multiMultiSubs);\
    iedm_describeMember(uint64_t,                   multiMultiRemSrvs);\
    iedm_describeMember(uint64_t,                   topicsUpdates);\
    iedm_describeMember(ismEngine_Subscription_t *, subscriptionHead);\
    iedm_describeMember(iettSubsNodeStats_t *,      subNodeStatsHead);\
    iedm_describeMember(uint32_t,                   activeSubNodeStats);\
    iedm_describeMember(ismStore_Handle_t,          retStoreHandle);\
    iedm_describeMember(ieutHashTable_t *,          originServers);\
    iedm_descriptionEnd;

#define iettOP_ADD    0  ///< Add a node or record (implies exclusive lock)
#define iettOP_FIND   1  ///< Find a node or record (implies shared lock)

#define iettINVALID_ADDRESS (void *)-1  ///< Ignored address in a list

/* Default substring arrays large enough to hold a max topic depth string & additional values */
#define iettDEFAULT_SUBSTRING_ARRAY_SIZE (iettMAX_TOPIC_DEPTH + 2)

/// Useful topic string extensions
#define iettSLASH_HASH "/#"
#define iettSLASH_PLUS "/+"

// macro to check if a topic string is a system topic
#define iettTOPIC_IS_SYSTOPIC(topic) ((topic)[0] == ismENGINE_SYSTOPIC_PREFIX[0])

// macro to check if a topic string would match $SYS/DisconnectedClientNotification
#define iettTOPIC_MATCHES_DOLLARSYS_DCN(topic) (((topic)[0] == ismENGINE_SYSTOPIC_PREFIX[0]) && \
                                                ((strcmp((topic), ismENGINE_DOLLARSYS_PREFIX iettSLASH_HASH) == 0) || \
                                                 (strcmp((topic), ismENGINE_DOLLARSYS_PREFIX iettSLASH_PLUS) == 0) || \
                                                 (strcmp((topic), ienfDCN_TOPIC) == 0) || \
                                                 (strcmp((topic), ienfDCN_TOPIC iettSLASH_HASH) == 0)))

//****************************************************************************
/// @brief Structure containing the analysis of a topic string
//****************************************************************************
typedef struct tag_iettTopic_t
{
    ismDestinationType_t  destinationType;
    const char           *topicString;
    char                 *topicStringCopy;
    const char          **substrings;
    uint32_t             *substringHashes;
    const char          **wildcards;
    const char          **multicards;
    ism_regex_t           regex;
    size_t                topicStringLength;
    int32_t               substringCount;
    int32_t               wildcardCount;
    int32_t               multicardCount;
    int32_t               initialArraySize;
    int32_t               sysTopicEndIndex;  /// Index of the end of a system ($*) topic string
} iettTopic_t;

//****************************************************************************
/// @brief Context used in iett_destroyTopicsTreeCallback
///
/// @see iett_destroyTopicsTreeCallback
//****************************************************************************
typedef struct tag_iettDestroyTopicsTreeCbContext_t
{
    bool freeingEngineTree;
} iettDestroyTopicsTreeCbContext_t;

//****************************************************************************
/// @brief Old soft Log Entry for storing of a durable subscription definition
//****************************************************************************
typedef struct tag_iettSLEOldStoreSubscDefn_t
{
    ietrStoreTranRef_t TranRef;
    ismStore_Handle_t storeHandle;
    ismQHandle_t queueHandle;
} iettSLEOldStoreSubscDefn_t;

//****************************************************************************
/// @brief Old soft Log Entry for storing of a durable subscription properties
//****************************************************************************
typedef struct tag_iettSLEOldStoreSubscProps_t
{
    ietrStoreTranRef_t TranRef;
    ismStore_Handle_t storeHandle;
    ismQHandle_t queueHandle;
} iettSLEOldStoreSubscProps_t;

//****************************************************************************
/// @brief Soft Log Entry for addition of subscription to topic tree
//****************************************************************************
typedef struct tag_iettSLEAddSubscription_t
{
    ismEngine_Subscription_t *subscription; ///< The subscription being added to the tree
    pthread_rwlock_t         *lock;         ///< Lock held as part of this SLE (or NULL)
} iettSLEAddSubscription_t;

//****************************************************************************
/// @brief Soft Log Entry for updating the retained message in the topic tree
//****************************************************************************
typedef struct tag_iettSLEUpdateRetained_t
{
    ietrStoreTranRef_t                   TranRef;               ///< Reference to transaction
    iettTopicNode_t                     *topicNode;             ///< Topic on which the retained message is being updated
    ismEngine_MessageHandle_t            message;               ///< New retained message
    ismEngine_MessageHandle_t            releaseMsgHdl;         ///< Message to be released during PostCommit/PostRollback
    bool                                 needUnstore;           ///< Whether the released message handle needs to be unstored
    bool                                 supersededAtCommit;    ///< Whether the commit was superseded during Commit phase
    bool                                 savepointRollback;     ///< Whether this SLE has already been called for SavepointRollback
    bool                                 repositioningRetained; ///< Whether this SLE is being added because we are repositioning a retained msg.
    uint64_t                             orderId;               ///< orderId assigned for this retained message
    ismStore_Handle_t                    refHandle;             ///< Handle to the reference created for this new message
    uint64_t                             publishSUV;            ///< The value of topic tree subsUpdates used to publish to
    uint64_t                             commitRUV;             ///< The value of topic tree retUpdates at end of commit
    iettOriginServer_t                  *replacedOriginServer;  ///< The originating server for the retained message being replaced
    ism_time_t                           timestamp;             ///< The server timestamp of this retained message
    iettOriginServer_t                  *originServer;          ///< The originating server for this retained message
    struct tag_iettSLEUpdateRetained_t  *nextInflightRetUpdate; ///< The next inflight iettSLEUpdateRetained_t in the chain for this node
} iettSLEUpdateRetained_t;

//****************************************************************************
/// @brief Context used when reporting retained message origin information
///        to the cluster
//****************************************************************************
typedef struct tag_iettOriginServerCbContext_t
{
    int32_t rc;
} iettOriginServerCbContext_t;

// Number of records to reserve space for when updating migrated subscriptions
#define iettUPDATE_MIGRATION_RESERVATION_RECORDS      2000
// Size of record to reserve space for during migration updating
#define iettUPDATE_MIGRATION_RESERVATION_RECORD_SIZE  4096

//****************************************************************************
/// @brief Context for iett_scanTopicsTreeCallback and iett_scanTopicsTreeNode
//****************************************************************************
#define iettLOWEST_ORDERID_ARRAY_SIZE         500     // Number of lowest orderIds to track during scan
#define iettORDERID_SPREAD_REPOSITION_LWM     100000  // OrderId spread at which engine considers taking action
#define iettORDERID_SPREAD_INUSE_PERCENT_LWM  20.0L   // Percentage of orderId spread in-use below which engine
                                                      // takes action

typedef struct tag_iettScanTopicsTreeCbContext_t
{
    iettTopicTree_t *tree;
    uint32_t activeOrderIdVoteCount;
    uint64_t minActiveOrderIdVote;
    uint64_t maxActiveOrderIdVote;
    iettTopicNode_t *lowestOrderIdNode[iettLOWEST_ORDERID_ARRAY_SIZE];
} iettScanTopicsTreeCbContext_t;

//****************************************************************************
/// @brief Default suboptions for admin subscriptions for different admin sub
/// config properties
//****************************************************************************
#define iettALLADMINSUBS_DEFAULT_MAXQUALITYOFSERVICE_SUBOPTIONS      ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE
#define iettALLADMINSUBS_DEFAULT_ADDRETAINEDMSGS_SUBOPTIONS          ismENGINE_SUBSCRIPTION_OPTION_NONE
#define iettALLADMINSUBS_DEFAULT_QUALITYOFSERVICEFILTER_SUBOPTIONS   ismENGINE_SUBSCRIPTION_OPTION_NONE

/*********************************************************************/
/* FUNCTION PROTOTYPES                                               */
/*********************************************************************/
iettTopicTree_t *iett_getEngineTopicTree(ieutThreadData_t *pThreadData);

// topicTree.c
int32_t iett_addSubscriptionToSubsNode(ieutThreadData_t *pThreadData,
                                       ismEngine_Subscription_t *subscription,
                                       iettSubscriptionList_t *subList);
int32_t iett_removeSubscriptionFromSubsNode(ieutThreadData_t *pThreadData,
                                            ismEngine_Subscription_t *subscription,
                                            iettSubscriptionList_t *subList);
void iett_destroySubsTreeCallback(ieutThreadData_t *pThreadData,
                                  char *key,
                                  uint32_t keyHash,
                                  void *value,
                                  void *context);
void iett_destroyTopicsTreeCallback(ieutThreadData_t *pThreadData,
                                    char *key,
                                    uint32_t keyHash,
                                    void *value,
                                    void *context);
int32_t iett_insertOrFindSubsNode(ieutThreadData_t *pThreadData,
                                  iettSubsNode_t *pParent,
                                  iettTopic_t *topic,
                                  int32_t treeOperation,
                                  iettSubsNode_t **node);
void iett_removeInactiveSubsNodesFromEngineTopicTree(ieutThreadData_t *pThreadData,
                                                     iettSubsNode_t *node,
                                                     iettSubsNode_t **removedSubtree);
int32_t iett_insertOrFindTopicsNode(ieutThreadData_t *pThreadData,
                                    iettTopicNode_t  *parent,
                                    iettTopic_t      *topic,
                                    int32_t           operation,
                                    iettTopicNode_t **node);
int32_t iett_findMatchingTopicsNodes(ieutThreadData_t *pThreadData,
                                     const iettTopicNode_t *parent,
                                     const bool             multiMode,
                                     const iettTopic_t     *topic,
                                     const uint32_t         curIndex,
                                     const uint32_t         wildIndex,
                                     const uint32_t         multiIndex,
                                     ieutHashSet_t         *resultSet,
                                     uint32_t              *maxNodes,
                                     uint32_t              *nodeCount,
                                     iettTopicNode_t     ***result);
iettTopicTree_t *iett_createTopicTree(ieutThreadData_t *pThreadData);
void iett_destroyTopicTree(ieutThreadData_t *pThreadData,
                           iettTopicTree_t *tree);
int32_t iett_addSubToEngineTopic(ieutThreadData_t *pThreadData,
                                 const char *topicString,
                                 ismEngine_Subscription_t *subscription,
                                 ismEngine_Transaction_t *pTran,
                                 bool inRecovery,
                                 bool putRetained);
int32_t iett_removeSubFromEngineTopic(ieutThreadData_t *pThreadData,
                                      ismEngine_Subscription_t *subscription,
                                      uint32_t flags);
void iett_performPendingSubscriptionDeletions(ieutThreadData_t *pThreadData,
                                              iettTopicTree_t *tree,
                                              char *pendingDeletionTopic);

// topicTreeRemote.c
void iett_destroyRemoteServersTreeCallback(ieutThreadData_t *pThreadData,
                                           char *key,
                                           uint32_t keyHash,
                                           void *value,
                                           void *context);
int32_t iett_insertOrFindRemSrvNode(ieutThreadData_t *pThreadData,
                                    iettRemSrvNode_t *parent,
                                    iettTopic_t *topic,
                                    int32_t operation,
                                    iettRemSrvNode_t **node);
int32_t iett_insertOrFindOriginServer(ieutThreadData_t *pThreadData,
                                      const char *serverUID,
                                      int32_t operation,
                                      iettOriginServer_t **record);
void iett_removeTopicNodeFromOriginServer(ieutThreadData_t *pThreadData,
                                          iettTopicNode_t *topicNode,
                                          iettOriginServer_t *originServer);
void iett_addTopicNodeToOriginServer(ieutThreadData_t *pThreadData,
                                     iettTopicNode_t *topicNode,
                                     iettOriginServer_t *originServer);
int32_t iett_addTopicNodeToOriginServerRecovery(ieutThreadData_t *pThreadData,
                                                iettTopicNode_t *topicNode,
                                                iettOriginServer_t *originServer);
void iett_removeTopicNodeFromOriginServerRecovery(ieutThreadData_t *pThreadData,
                                                  iettTopicNode_t *topicNode,
                                                  iettOriginServer_t *originServer);
void iett_clusterReportOriginServer(ieutThreadData_t *pThreadData,
                                    char *key,
                                    uint32_t keyHash,
                                    void *value,
                                    void *context);
void iett_reconcileOriginServer(ieutThreadData_t *pThreadData,
                                char *key,
                                uint32_t keyHash,
                                void *value,
                                void *context);

// topicTreeSubscriptions.c
int32_t iett_allocateSubscription(ieutThreadData_t *pThreadData,
                                  const char *pClientId,
                                  size_t *pClientIdLength,
                                  const char *pSubName,
                                  size_t *pSubNameLength,
                                  void *pSubProperties,
                                  size_t *pSubPropertiesLength,
                                  const ismEngine_SubscriptionAttributes_t *pSubAttributes,
                                  uint32_t internalAttrs,
                                  iereResourceSetHandle_t resourceSet,
                                  ismEngine_Subscription_t **ppSubscription);
void    iett_freeSubscription(ieutThreadData_t *pThreadData,
                              ismEngine_Subscription_t *subscription,
                              bool freeOnly);
int32_t iett_allocateSubQueueName(ieutThreadData_t *pThreadData,
                                  const char *pClientId,
                                  size_t clientIdLength,
                                  const char *pSubName,
                                  size_t subNameLength,
                                  const char *pTopicString,
                                  size_t topicStringLength,
                                  char **ppQueueName);
void    iett_freeSubQueueName(ieutThreadData_t *pThreadData, char *pAllocName);

// topicTreeSharedSubs.c
int32_t iett_createAdminSharedSubscription(ieutThreadData_t *pThreadData,
                                           const char *nameSpace,
                                           const char *subscriptionName,
                                           ism_prop_t *adminProps,
                                           const char *adminObjectType,
                                           void *pContext,
                                           size_t contextLength,
                                           ismEngine_CompletionCallback_t pCallbackFn);

// topicTreeRetained.c
int32_t iett_putRetainedMessagesToSubscription(ieutThreadData_t *pThreadData,
                                               iettTopicTree_t *tree,
                                               iettTopic_t *topic,
                                               ismEngine_Subscription_t *subscription,
                                               ismEngine_Transaction_t **ppTran,
                                               bool republish);
int32_t iett_putRetainedMessageToNewSubs(ieutThreadData_t *pThreadData,
                                         const char *topicString,
                                         ismEngine_Message_t *message,
                                         uint64_t publishSUV,
                                         uint64_t commitRUV);
int32_t iett_removeRetainedMessages(ieutThreadData_t *pThreadData,
                                    iettTopicTree_t *tree,
                                    iettTopic_t *topic);
int32_t iett_ensureEngineStoreTopicRecord(ieutThreadData_t *pThreadData);
int32_t iett_updateRetainedMessage(ieutThreadData_t *pThreadData,
                                   const char *topicString,
                                   ismEngine_Message_t *message,
                                   uint64_t publishSUV,
                                   ismEngine_Transaction_t *pTran,
                                   uint32_t options);
void iett_preGetSubscriberListForRetainedPublish(ieutThreadData_t *pThreadData,
                                                 ietrSLE_Header_t *pSLEHdr);
void iett_postGetSubscriberListForRetainedPublish(ieutThreadData_t *pThreadData);
iettTopicNode_t *iett_removeUnusedTree(ieutThreadData_t *pThreadData,
                                       iettTopicTree_t *tree,
                                       iettTopicNode_t *topicNode);
void iett_sortTopicNodesByTimestamp(ieutThreadData_t *pThreadData,
                                    iettTopicNode_t **topicNodes,
                                    uint32_t nodeCount);

// topicTreeUtils.c
int32_t iett_analyseTopicString(ieutThreadData_t *pThreadData,
                                iettTopic_t *topic);
uint32_t iett_generateSubstringHash(const char *key);
uint32_t iett_generateTopicStringHash(const char *key);
uint32_t iett_generateSubNameHash(const char *subName);
uint32_t iett_generateOriginServerHash(const char *serverUID);

//****************************************************************************
/// @brief  Replay an UpdateRetained (iettSLEUpdateRetained_t) soft log entry
///
/// @param[in]     pTran    Transaction being committed/rolled back
/// @param[in]     Phase    Which of the commit/rollback phases this is
/// @param[in]     hStream  The store stream handle being used
/// @param[in]     entry    Pointer to the SLE
/// @param[in]     pRecord  Pointer to transaction state data
//****************************************************************************
void iett_SLEReplayUpdateRetained(ietrReplayPhase_t        Phase,
                                  ieutThreadData_t        *pThreadData,
                                  ismEngine_Transaction_t *pTran,
                                  void                    *entry,
                                  ietrReplayRecord_t      *pRecord,
                                  ismEngine_DelivererContext_t *delivererContext);

//****************************************************************************
/// @brief  Replay an AddSubscription (iettSLEAddSubscription_t) soft log entry
///
/// @param[in]     Phase    Which of the commit/rollback phases this is
/// @param[in]     hStream  The store stream handle being used
/// @param[in]     entry    Pointer to the SLE
/// @param[in]     pRecord  Pointer to transaction state data
//****************************************************************************
void iett_SLEReplayAddSubscription(ietrReplayPhase_t        Phase,
                                   ieutThreadData_t        *pThreadData,
                                   ismEngine_Transaction_t *pTran,
                                   void                    *entry,
                                   ietrReplayRecord_t      *pRecord,
                                   ismEngine_DelivererContext_t *delivererContext);

//****************************************************************************
/// @brief  Replay an old Store Subscription Definition (iettSLEOldStoreSubscDefn_t)
///         soft log entry
///
/// @param[in]     Phase    Which of the commit/rollback phases this is
/// @param[in]     hStream  The store stream handle being used
/// @param[in]     entry    Pointer to the SLE
/// @param[in]     pRecord  Pointer to transaction state data
//****************************************************************************
void iett_SLEReplayOldStoreSubscDefn(ietrReplayPhase_t        Phase,
                                     ieutThreadData_t        *pThreadData,
                                     ismEngine_Transaction_t *pTran,
                                     void                    *entry,
                                     ietrReplayRecord_t      *pRecord,
                                     ismEngine_DelivererContext_t * delivererContext);

//****************************************************************************
/// @brief  Replay an old Store Subscription Properties (iettSLEOldStoreSubscProps_t)
///         soft log entry
///
/// @param[in]     Phase    Which of the commit/rollback phases this is
/// @param[in]     hStream  The store stream handle being used
/// @param[in]     entry    Pointer to the SLE
/// @param[in]     pRecord  Pointer to transaction state data
//****************************************************************************
void iett_SLEReplayOldStoreSubscProps(ietrReplayPhase_t        Phase,
                                      ieutThreadData_t        *pThreadData,
                                      ismEngine_Transaction_t *pTran,
                                      void                    *entry,
                                      ietrReplayRecord_t      *pRecord,
                                      ismEngine_DelivererContext_t * delivererContext);

//****************************************************************************
/// @brief  Add an iettSLEUpdateRetained_t to the inflight chain for the
///         specified node.
///
/// @param[in]     topicNode    The topic node whose chain is to be added to
/// @param[in]     pSLE         SLE to add
//****************************************************************************
void iett_addInflightRetUpdate(ieutThreadData_t *pThreadData,
                               iettTopicNode_t *topicNode,
                               iettSLEUpdateRetained_t *pSLE);

//****************************************************************************
/// @brief  Remove an iettSLEUpdateRetained_t from the inflight chain for the
///         specified node.
///
/// @param[in]     topicNode    The topic node whose chain is to be added to
/// @param[in]     pSLE         SLE to add
//****************************************************************************
void iett_removeInflightRetUpdate(ieutThreadData_t *pThreadData,
                                  iettTopicNode_t *topicNode,
                                  iettSLEUpdateRetained_t *pSLE);

//****************************************************************************
/// @brief  Return the highest timestamp in the inflight retained updates
///
/// @param[in]     topicNode    The topic node whose chain is to be added to
///
/// @return highest ism_time_t found (0 for none).
//****************************************************************************
ism_time_t iett_findHighestInflightRetTimestamp(ieutThreadData_t *pThreadData,
                                                iettTopicNode_t *topicNode);

#ifdef __cplusplus
}
#endif

#endif /* __ISM_ENGINE_TOPICTREE_INTERNAL_DEFINED */
