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
/// @file  engineStore.h
/// @brief Engine component persistent store header file. The structures for
///        all data persisted by the Engine in the Store must be defined
///        in this header file.
//****************************************************************************
#ifndef __ISM_ENGINESTORE_DEFINED
#define __ISM_ENGINESTORE_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include <engine.h>         /* Engine external header file           */
#include <engineCommon.h>   /* Engine common internal header file    */
#include <store.h>          /* Store external header file            */
#include <stdint.h>         /* Standard integer defns header file    */
#include <transaction.h>    /* Engine transaction header file        */
#include <policyInfo.h>     /* Engine policy information header file */

/*********************************************************************/
/*                                                                   */
/* TYPE DEFINITIONS                                                  */
/*                                                                   */
/*********************************************************************/

/** @name Eyecatchers for Engine records in the Store
 * The eyecatchers match the names of the records.
 */

/**@{*/

#define iestSERVER_CONFIG_RECORD_STRUCID       "ESCR"            ///< Eyecatcher for Server Configuration Record (SCR)
#define iestSERVER_CONFIG_RECORD_STRUCID_ARRAY {'E','S','C','R'}
#define iestCLIENT_STATE_RECORD_STRUCID        "ECSR"            ///< Eyecatcher for Client State Record (CSR)
#define iestCLIENT_STATE_RECORD_STRUCID_ARRAY  {'E','C','S','R'}
#define iestCLIENT_PROPS_RECORD_STRUCID        "ECPR"            ///< Eyecatcher for Client Properties Record (CPR)
#define iestCLIENT_PROPS_RECORD_STRUCID_ARRAY  {'E','C','P','R'}
#define iestQUEUE_DEFN_RECORD_STRUCID          "EQDR"            ///< Eyecatcher for Queue Definition Record (QDR)
#define iestQUEUE_DEFN_RECORD_STRUCID_ARRAY    {'E','Q','D','R'}
#define iestQUEUE_PROPS_RECORD_STRUCID         "EQPR"            ///< Eyecatcher for Queue Properties Record (QPR)
#define iestQUEUE_PROPS_RECORD_STRUCID_ARRAY   {'E','Q','P','R'}
#define iestTOPIC_DEFN_RECORD_STRUCID          "ETDR"            ///< Eyecatcher for Topic Definition Record (TDR)
#define iestTOPIC_DEFN_RECORD_STRUCID_ARRAY    {'E','T','D','R'}
#define iestSUBSC_DEFN_RECORD_STRUCID          "ESDR"            ///< Eyecatcher for Subscription Definition Record (SDR)
#define iestSUBSC_DEFN_RECORD_STRUCID_ARRAY    {'E','S','D','R'}
#define iestSUBSC_PROPS_RECORD_STRUCID         "ESPR"            ///< Eyecatcher for Subscription Properties Record (SPR)
#define iestSUBSC_PROPS_RECORD_STRUCID_ARRAY   {'E','S','P','R'}
#define iestTRANSACTION_RECORD_STRUCID         "ETR "            ///< Eyecatcher for Transaction Record (TR)
#define iestTRANSACTION_RECORD_STRUCID_ARRAY   {'E','T','R',' '}
#define iestMESSAGE_HEADER_AREA_STRUCID        "EMHA"            ///< Eyecatcher for Message Header Area (MHA)
#define iestMESSAGE_HEADER_AREA_STRUCID_ARRAY  {'E','M','H','A'}
#define iestMESSAGE_RECORD_STRUCID             "EMR "            ///< Eyecatcher for Message Record (MR)
#define iestMESSAGE_RECORD_STRUCID_ARRAY       {'E','M','R',' '}
#define iestBRIDGE_QMGR_RECORD_STRUCID         "EBMR"            ///< Eyecatcher for Bridge Queue Manager Record (BMR)
#define iestBRIDGE_QMGR_RECORD_STRUCID_ARRAY   {'E','B','M','R'}
#define iestREMSRV_DEFN_RECORD_STRUCID         "ERDR"            ///< Eyecatcher for Remote Server Definition Record (RDR)
#define iestREMSRV_DEFN_RECORD_STRUCID_ARRAY   {'E','R','D','R'}
#define iestREMSRV_PROPS_RECORD_STRUCID        "ERPR"            ///< Eyecatcher for Remote Server Properties Record (RPR)
#define iestREMSRV_PROPS_RECORD_STRUCID_ARRAY  {'E','R','P','R'}

/**@}*/


//*********************************************************************
/// @brief  Softlog entry for Add Unreleased Message State
/// @remark
///    This structure contains the softlog entry for an Add Unreleased
///    Message State request.
//*********************************************************************
typedef struct iestSLEAddUMS_t
{
    ietrStoreTranRef_t TranRef;
    ismEngine_ClientState_t *pClient;
    ismEngine_UnreleasedState_t *pUnrelChunk;
    int16_t SlotNumber;
    ismStore_Handle_t hStoreUMS;
} iestSLEAddUMS_t;


//*********************************************************************
/// @brief  Softlog entry for Remove Unreleased Message State
/// @remark
///    This structure contains the softlog entry for a Remove Unreleased
///    Message State request.
//*********************************************************************
typedef struct iestSLERemoveUMS_t
{
    ietrStoreTranRef_t TranRef;
    ismEngine_ClientState_t *pClient;
    ismEngine_UnreleasedState_t *pUnrelChunk;
    int16_t SlotNumber;
    ismStore_Handle_t hStoreUMS;
} iestSLERemoveUMS_t;


//*********************************************************************
/// @brief  Server Configuration Record (SCR)
///
/// A record in the Store of type ISM_STORE_RECTYPE_SERVER.
///
/// Note: Version 2 did not add any fields, it exists to determine if
///       we accept the existence of multiple SCRs (<2) or not (>=2).
//*********************************************************************
typedef struct
{
    char                            StrucId[4];                ///< Eyecatcher "ESCR"
    uint32_t                        Version;                   ///< Version number
} iestServerConfigRecord_t;

#define iestSCR_VERSION_1           1
#define iestSCR_VERSION_2           2
#define iestSCR_CURRENT_VERSION     iestSCR_VERSION_2


/** @name State values for the Server Configuration Record
 * The upper 32 bits are the time in seconds since 2000-01-01T00
 * (the same as message expiry in ISM). This is updated periodically
 * as the server is running to give an indication of when the server
 * was last running in the event that it stops suddenly.
 */

/**@{*/

#define iestSCR_STATE_NONE          0x0 ///< None

/**@}*/

typedef struct tag_iestServerConfigRecord_V1_t
{
    char                            StrucId[4];                ///< Eyecatcher "ESCR"
    uint32_t                        Version;                   ///< Version number
} iestServerConfigRecord_V1_t;

//*********************************************************************
/// @brief  Client State Record (CSR)
///
/// A record in the Store of type ISM_STORE_RECTYPE_CLIENT.
/// Following this structure is the client ID, as a null-terminated
/// string. If the length of either of this is zero, the corresponding
/// string will be omitted.
/// @par
/// If the client-state has a will message, the CSR's attribute will
/// be the handle of a CPR.
/// @par
/// The CSR's state contains flags.
/// @par
/// The CSR's attribute contains the handle of the CPR if the client-state
/// has an associated will-message.
//*********************************************************************
typedef struct tag_iestClientStateRecord_t
{
    char                            StrucId[4];                ///< Eyecatcher "ECSR"
    uint32_t                        Version;                   ///< Version number
    uint32_t                        Flags;                     ///< Flags
    uint32_t                        ClientIdLength;            ///< Length of optional client ID
    // Version 2+
    uint32_t                        protocolId;                ///< Protocol identifier
} iestClientStateRecord_t;

#define iestCSR_VERSION_1           1
#define iestCSR_VERSION_2           2
#define iestCSR_CURRENT_VERSION     iestCSR_VERSION_2


/** @name Flags for the Client State Record
 * These are used in the Flags of the iestClientStateRecord_t.
 */

/**@{*/

#define iestCSR_FLAGS_NONE          0x0 ///< No flags
#define iestCSR_FLAGS_DURABLE       0x1 ///< The client-state is durable

/**@}*/


/** @name State values for the Client State Record
 * The upper 32 bits are the last-connected time in seconds since 2000-01-01T00
 * (the same as message expiry in ISM) and are present if iestCSR_STATE_DISCONNECTED
 * is set.
 */

/**@{*/

#define iestCSR_STATE_NONE          0x0 ///< None
#define iestCSR_STATE_DELETED       0x1 ///< The client-state is being deleted
#define iestCSR_STATE_DISCONNECTED  0x2 ///< The client-state is disconnected

/**@}*/

typedef struct tag_iestClientStateRecord_V1_t
{
    char                            StrucId[4];                ///< Eyecatcher "ECSR"
    uint32_t                        Version;                   ///< Version number
    uint32_t                        Flags;                     ///< Flags
    uint32_t                        ClientIdLength;            ///< Length of optional client ID
} iestClientStateRecord_V1_t;


//*********************************************************************
/// @brief  Client Properties Record (CPR)
///
/// A record in the Store of type ISM_STORE_RECTYPE_CPROP.
/// Following this structure is the will-topic name, as a
/// null-terminated string.
/// @par
/// If the client-state has a will message, the CPR's attribute will
/// be the handle of the will-message's MR.
//*********************************************************************
typedef struct tag_iestClientPropertiesRecord_t
{
    char                            StrucId[4];                ///< Eyecatcher "ECPR"
    uint32_t                        Version;                   ///< Version number
    uint32_t                        Flags;                     ///< Flags
    uint32_t                        WillTopicNameLength;       ///< Length of will-topic name
    // Version 2+
    uint32_t                        UserIdLength;              ///< Length of associated user-id
    // Version 3+
    uint32_t                        WillMsgTimeToLive;         ///< Will message Time-to-Live in seconds
    // Version 4+
    uint32_t                        ExpiryInterval;            ///< ClientState expiry interval in seconds
    // Version 5+
    uint32_t                        WillDelay;                 ///< Delay in seconds before publishing will message
} iestClientPropertiesRecord_t;

#define iestCPR_VERSION_1           1
#define iestCPR_VERSION_2           2
#define iestCPR_VERSION_3           3
#define iestCPR_VERSION_4           4
#define iestCPR_VERSION_5           5
#define iestCPR_CURRENT_VERSION     iestCPR_VERSION_5

/** @name Flags for the Client Properties Record
 * These are used in the Flags of the iestClientPropertiesRecord_t.
 */

/**@{*/

#define iestCPR_FLAGS_NONE          0x0 ///< No flags
#define iestCPR_FLAGS_DURABLE       0x1 ///< The client-state is durable

/**@}*/


/** @name State values for the Client Properties Record
 */

/**@{*/

#define iestCPR_STATE_NONE          0x0 ///< None

/**@}*/

/// @brief Version 4 Client Properties Record (CPR)
typedef struct tag_iestClientPropertiesRecord_V4_t
{
    char                            StrucId[4];                ///< Eyecatcher "ECPR"
    uint32_t                        Version;                   ///< Version number
    uint32_t                        Flags;                     ///< Flags
    uint32_t                        WillTopicNameLength;       ///< Length of will-topic name
    // Version 2+
    uint32_t                        UserIdLength;              ///< Length of associated user-id
    // Version 3+
    uint32_t                        WillMsgTimeToLive;         ///< Will message Time-to-Live in seconds
    // Version 4+
    uint32_t                        ExpiryInterval;            ///< ClientState expiry interval in seconds
} iestClientPropertiesRecord_V4_t;

/// @brief Version 3 Client Properties Record (CPR)
typedef struct tag_iestClientPropertiesRecord_V3_t
{
    char                            StrucId[4];                ///< Eyecatcher "ECPR"
    uint32_t                        Version;                   ///< Version number
    uint32_t                        Flags;                     ///< Flags
    uint32_t                        WillTopicNameLength;       ///< Length of will-topic name
    // Version 2+
    uint32_t                        UserIdLength;              ///< Length of associated user-id
    // Version 3+
    uint32_t                        WillMsgTimeToLive;         ///< Expiry Time-to-Live in seconds for will msg
} iestClientPropertiesRecord_V3_t;

/// @brief Version 2 Client Properties Record (CPR)
typedef struct tag_iestClientPropertiesRecord_V2_t
{
    char                            StrucId[4];                ///< Eyecatcher "ECPR"
    uint32_t                        Version;                   ///< Version number
    uint32_t                        Flags;                     ///< Flags
    uint32_t                        WillTopicNameLength;       ///< Length of will-topic name
    // Version 2+
    uint32_t                        UserIdLength;              ///< Length of associated user-id
} iestClientPropertiesRecord_V2_t;

/// @brief Version 1 Client Properties Record (CPR)
typedef struct tag_iestClientPropertiesRecord_V1_t
{
    char                            StrucId[4];                ///< Eyecatcher "ECPR"
    uint32_t                        Version;                   ///< Version number
    uint32_t                        Flags;                     ///< Flags
    uint32_t                        WillTopicNameLength;       ///< Length of will-topic name
} iestClientPropertiesRecord_V1_t;

//*********************************************************************
/// @brief  Transaction Record (TR)
///
/// A record in the Store of type ISM_STORE_RECTYPE_TRANS.
/// Following this structure is the transaction ID, as an
/// array of bytes.
//*********************************************************************
typedef struct
{
    char                            StrucId[4];                ///< Eyecatcher "ETR "
    uint32_t                        Version;                   ///< Version number
    bool                            GlobalTran;                ///< Local/Global
    uint32_t                        TransactionIdLength;       ///< Transaction ID
} iestTransactionRecord_t;

#define iestTR_VERSION_1            1
#define iestTR_CURRENT_VERSION      iestTR_VERSION_1



//*********************************************************************
/// @brief  Message Record
///
/// A record in the Store of type ISM_STORE_RECTYPE_MSG.
/// Following this structure are the message buffers, the number 
/// of message buffers is defined in the AreaCount value.
/// Immediately following this structure is 
/// &li  An array of uint32_t values of size AreaCount containing the 
///      length of each area
/// &li  An array of uint32_t values of size AreaCount containing the
///      type of each Area
/// &li  Each message area. 
//*********************************************************************
typedef struct
{
    char                            StrucId[4];                ///< Eyecatcher "EMR "
    uint32_t                        Version;                   ///< Version number
    uint32_t                        AreaCount;                 ///< Number of areas
    uint32_t                        AreaTypes[ismMESSAGE_AREA_COUNT+1]; ///< Type of messages
    size_t                          AreaLen[ismMESSAGE_AREA_COUNT+1]; ///< Length of areas
} iestMessageRecord_t;

#define iestMR_VERSION_1            1
#define iestMR_CURRENT_VERSION      iestMR_VERSION_1


//*********************************************************************
/// @brief  Store Message Header 
///
/// Each message record in the store contains a set of data areas
/// which includes the message header as well as the message payload.
/// This structure defines the format of the message header area
//*********************************************************************
typedef struct
{
    char                            StrucId[4];                ///< Eyecatcher "EMHA"
    uint32_t                        Version;                   ///< Version number
    uint8_t                         Persistence;               ///< Message persistence
    uint8_t                         Reliability;               ///< Message reliability
    uint8_t                         Priority;                  ///< Message priority
    uint8_t                         Flags;                     ///< Message flags
    uint32_t                        Expiry;                    ///< Expiry
    uint8_t                         MessageType;               ///< Message type
} iestMessageHdrArea_t;

#define iestMHA_VERSION_1           1
#define iestMHA_CURRENT_VERSION     iestMHA_VERSION_1


//*********************************************************************
/// @brief  Queue Definition Record (QDR)
///
/// A record in the Store of type ISM_STORE_RECTYPE_QUEUE.
///
//*********************************************************************
typedef struct tag_iestQueueDefinitionRecord_t
{
    char                            StrucId[4];                ///< Eyecatcher "EQDR"
    uint32_t                        Version;                   ///< Version number
    ismQueueType_t                  Type;                      ///< Queue Type
} iestQueueDefinitionRecord_t;

#define iestQDR_VERSION_1           1
#define iestQDR_CURRENT_VERSION     iestQDR_VERSION_1

/** @name State values for the Queue Definition Record
 */

/**@{*/

#define iestQDR_STATE_NONE          0x00000000  ///< None
#define iestQDR_STATE_DELETED       0x00000100  ///< This queue has been deleted

/**@}*/

//*********************************************************************
/// @brief Maximum number of store record fragments for either QDR or QPR
//*********************************************************************
#define iestQUEUE_MAX_FRAGMENTS 2

//*********************************************************************
/// @brief  Queue Properties Record (QPR)
///
/// A record in the Store of type ISM_STORE_RECTYPE_QPROP.
///
/// Following this structure is the queue name as null-terminated
/// strings as an array of bytes. If the length of any field is
/// zero, the corresponding data will be omitted.
//*********************************************************************
typedef struct tag_iestQueuePropertiesRecord_t
{
    char                        StrucId[4];                   ///< Eyecatcher "EQPR"
    uint32_t                    Version;                      ///< Version number
    uint32_t                    QueueNameLength;              ///< Length of the queue name
} iestQueuePropertiesRecord_t;

#define iestQPR_VERSION_1           1
#define iestQPR_CURRENT_VERSION     iestQPR_VERSION_1

/** @name State values for the Queue Properties Record
 */

/**@{*/

#define iestQPR_STATE_NONE          0 ///< None

/**@}*/

//*********************************************************************
/// @brief  Topic Definition Record (TDR)
///
/// A record in the Store of type ISM_STORE_RECTYPE_TOPIC.
///
/// Following this structure is the null-terminated topic string.
//*********************************************************************
typedef struct tag_iestTopicDefinitionRecord_t
{
    char                            StrucId[4];                ///< Eyecatcher "ETDR"
    uint32_t                        Version;                   ///< Version number
} iestTopicDefinitionRecord_t;

#define iestTDR_VERSION_1           1
#define iestTDR_VERSION_2           2
#define iestTDR_CURRENT_VERSION     iestTDR_VERSION_2

/** @name State values for the Topic Definition Record
 */

/**@{*/

#define iestTDR_STATE_NONE        0x00000000  ///< None
#define iestTDR_STATE_DELETED     0x00000100  ///< This topic node has been deleted

/**@}*/

typedef struct tag_iestTopicDefinitionRecord_V1_t
{
    char                            StrucId[4];                ///< Eyecatcher "ETDR"
    uint32_t                        Version;                   ///< Version number
    uint32_t                        TopicStringLength;         ///< Length of topic string
} iestTopicDefinitionRecord_V1_t;


//*********************************************************************
/// @brief  Subscription Definition Record (SDR)
///
/// A record in the Store of type ISM_STORE_RECTYPE_SUBSC.
//*********************************************************************
typedef struct tag_iestSubscriptionDefinitionRecord_t
{
    char                            StrucId[4];                ///< Eyecatcher "ESDR"
    uint32_t                        Version;                   ///< Version number
    ismQueueType_t                  Type;                      ///< Queue Type
} iestSubscriptionDefinitionRecord_t;

#define iestSDR_VERSION_1           1
#define iestSDR_CURRENT_VERSION     iestSDR_VERSION_1

/** @name State values for the Subscription Definition Record
 */

/**@{*/

#define iestSDR_STATE_NONE                0x00000000  ///< None
#define iestSDR_STATE_DELETED             0x00000100  ///< This subscription has been deleted
#define iestSDR_STATE_CREATING            0x00000200  ///< This subscription has not yet finished creation

/**@}*/

//*********************************************************************
/// @brief Maximum number of store record fragments for either SDR or SPR
//*********************************************************************
#define iestSUBSCRIPTION_MAX_FRAGMENTS 9

//*********************************************************************
/// @brief  Subscription Properties Record (SPR)
///
/// A record in the Store of type ISM_STORE_RECTYPE_SPROP.
///
/// Following this structure is the client ID, subscription name,
/// topic string, and policy uuid as null-terminated strings and the
/// optional subscription properties, as an array of bytes.
///
/// If the length of any of these is zero, the corresponding data will be omitted.
///
/// For shared subscriptions with a non-zero SharingClientCount an array
/// of subOptions (one for each sharing client), an array of client IDs (one
/// for each sharing client) and an array of subIds (one for each sharing client)
/// follow.
//*********************************************************************
typedef struct tag_iestSubscriptionPropertiesRecord_t
{
    char                        StrucId[4];                 ///< Eyecatcher "ESPR"
    uint32_t                    Version;                    ///< Version number
    uint32_t                    SubOptions;                 ///< Subscription options
    uint32_t                    InternalAttrs;              ///< Internal attributes
    uint32_t                    ClientIdLength;             ///< Length of client ID
    uint32_t                    SubNameLength;              ///< Length of subscription name
    uint32_t                    TopicStringLength;          ///< Length of topic string
    uint32_t                    SubPropertiesLength;        ///< Length of optional subscription properties
    uint64_t                    MaxMessageCount;            ///< Maximum count of messages for this subscription (renamed at v6)
    // Version 2+ ...
    bool                        DCNEnabled;                 ///< Disconnected Client Notification value
    // Version 3+ ...
    uint8_t                     MaxMsgBehavior;             ///< Behavior used when Maximum message limit hit (renamed at v6)
    // Version 4+ ...
    uint8_t                     AnonymousSharers;           ///< Anonymous sharers that have registered with the subscription (like JMS)
    uint32_t                    SharingClientCount;         ///< Count of explicitly sharing clients
    uint64_t                    SharingClientIdsLength;     ///< Length of the sharing client IDs array (containing SharingClientCount null-terminated strings)
    // Version 5+ ...
    uint32_t                    PolicyNameLength;           ///< Length of the policy name (renamed at v6)
    // Version 7+ ...
    ismEngine_SubId_t           SubId;                      ///< SubId of the subscription
} iestSubscriptionPropertiesRecord_t;

#define iestSPR_VERSION_1           1
#define iestSPR_VERSION_2           2
#define iestSPR_VERSION_3           3
#define iestSPR_VERSION_4           4
#define iestSPR_VERSION_5           5
#define iestSPR_VERSION_6           6
#define iestSPR_VERSION_7           7
#define iestSPR_CURRENT_VERSION     iestSPR_VERSION_7

/** @name State values for the Subscription Properties Record
 */

/**@{*/

#define iestSPR_STATE_NONE          0  ///< None

/**@}*/

/// @brief Version 6 Subscription Properties Record (SPR) used for migration
typedef struct tag_iestSubscriptionPropertiesRecord_V6_t
{
    char                        StrucId[4];                 ///< Eyecatcher "ESPR"
    uint32_t                    Version;                    ///< Version number
    uint32_t                    SubOptions;                 ///< Subscription options
    uint32_t                    InternalAttrs;              ///< Internal attributes
    uint32_t                    ClientIdLength;             ///< Length of client ID
    uint32_t                    SubNameLength;              ///< Length of subscription name
    uint32_t                    TopicStringLength;          ///< Length of topic string
    uint32_t                    SubPropertiesLength;        ///< Length of optional subscription properties
    uint64_t                    MaxMessageCount;            ///< Maximum count of messages for this subscription (renamed at v6)
    // Version 2+ ...
    bool                        DCNEnabled;                 ///< Disconnected Client Notification value
    // Version 3+ ...
    uint8_t                     MaxMsgBehavior;             ///< Behavior used when Maximum message limit hit (renamed at v6)
    // Version 4+ ...
    uint8_t                     AnonymousSharers;           ///< Anonymous sharers that have registered with the subscription (like JMS)
    uint32_t                    SharingClientCount;         ///< Count of explicitly sharing clients
    uint64_t                    SharingClientIdsLength;     ///< Length of the sharing client IDs array (containing SharingClientCount null-terminated strings)
    // Version 5+ ...
    uint32_t                    PolicyNameLength;           ///< Length of the policy name (renamed at v6)
} iestSubscriptionPropertiesRecord_V6_t;

/// @brief Version 5 Subscription Properties Record (SPR) used for migration
typedef struct tag_iestSubscriptionPropertiesRecord_V5_t
{
    char                        StrucId[4];                 ///< Eyecatcher "ESPR"
    uint32_t                    Version;                    ///< Version number
    uint32_t                    SubOptions;                 ///< Subscription options
    uint32_t                    InternalAttrs;              ///< Internal attributes
    uint32_t                    ClientIdLength;             ///< Length of client ID
    uint32_t                    SubNameLength;              ///< Length of subscription name
    uint32_t                    TopicStringLength;          ///< Length of topic string
    uint32_t                    SubPropertiesLength;        ///< Length of optional subscription properties
    uint64_t                    MaxMessages;                ///< Maximum message count for this subscription
    // Version 2+ ...
    bool                        DCNEnabled;                 ///< Disconnected Client Notification value
    // Version 3+ ...
    uint8_t                     MaxMsgsBehavior;            ///< Behavior used when Maximum messages hit
    // Version 4+ ...
    bool                        GenericallyShared;          ///< Whether the subscription is shared between generic consumers (like JMS)
    uint32_t                    SharingClientCount;         ///< Count of explicitly sharing clients
    uint64_t                    SharingClientIdsLength;     ///< Length of the sharing client IDs array (containing SharingClientCount null-terminated strings)
    // Version 5+
    uint32_t                    PolicyUUIDLength;           ///< Length of the policy uuid
} iestSubscriptionPropertiesRecord_V5_t;

/// @brief Version 4 Subscription Properties Record (SPR) used for migration
typedef struct tag_iestSubscriptionPropertiesRecord_V4_t
{
    char                        StrucId[4];                 ///< Eyecatcher "ESPR"
    uint32_t                    Version;                    ///< Version number
    uint32_t                    SubOptions;                 ///< Subscription options
    uint32_t                    InternalAttrs;              ///< Internal attributes
    uint32_t                    ClientIdLength;             ///< Length of client ID
    uint32_t                    SubNameLength;              ///< Length of subscription name
    uint32_t                    TopicStringLength;          ///< Length of topic string
    uint32_t                    SubPropertiesLength;        ///< Length of optional subscription properties
    uint64_t                    MaxMessages;                ///< Maximum message count for this subscription
    bool                        DCNEnabled;                 ///< Disconnected Client Notification value
    uint8_t                     MaxMsgsBehavior;            ///< Behavior used when Maximum messages hit
    bool                        GenericallyShared;          ///< Whether the subscription is shared between generic consumers (like JMS)
    uint32_t                    SharingClientCount;         ///< Count of explicitly sharing clients
    uint64_t                    SharingClientIdsLength;     ///< Length of the sharing client IDs array (containing SharingClientCount null-terminated strings)
} iestSubscriptionPropertiesRecord_V4_t;

/// @brief Version 3 Subscription Properties Record (SPR) used for migration
typedef struct tag_iestSubscriptionPropertiesRecord_V3_t
{
    char                        StrucId[4];                 ///< Eyecatcher "ESPR"
    uint32_t                    Version;                    ///< Version number
    uint32_t                    SubOptions;                 ///< Subscription options
    uint32_t                    InternalAttrs;              ///< Internal attributes
    uint32_t                    ClientIdLength;             ///< Length of client ID
    uint32_t                    SubNameLength;              ///< Length of subscription name
    uint32_t                    TopicStringLength;          ///< Length of topic string
    uint32_t                    SubPropertiesLength;        ///< Length of optional subscription properties
    uint64_t                    MaxMessages;                ///< Maximum message count for this subscription
    bool                        DCNEnabled;                 ///< Disconnected Client Notification value
    uint8_t                     MaxMsgsBehavior;            ///< Behavior used when Maximum messages hit
} iestSubscriptionPropertiesRecord_V3_t;

/// @brief Version 2 Subscription Properties Record (SPR) used for migration
typedef struct tag_iestSubscriptionPropertiesRecord_V2_t
{
    char                        StrucId[4];                 ///< Eyecatcher "ESPR"
    uint32_t                    Version;                    ///< Version number
    uint32_t                    SubOptions;                 ///< Subscription options
    uint32_t                    InternalAttrs;              ///< Internal attributes
    uint32_t                    ClientIdLength;             ///< Length of client ID
    uint32_t                    SubNameLength;              ///< Length of subscription name
    uint32_t                    TopicStringLength;          ///< Length of topic string
    uint32_t                    SubPropertiesLength;        ///< Length of optional subscription properties
    uint64_t                    MaxMessages;                ///< Maximum message count for this subscription
    bool                        DCNEnabled;                 ///< Disconnected Client Notification value
} iestSubscriptionPropertiesRecord_V2_t;

/// @brief Version 1 Subscription Properties Record (SPR) used for migration
typedef struct tag_iestSubscriptionPropertiesRecord_V1_t
{
    char                        StrucId[4];                 ///< Eyecatcher "ESPR"
    uint32_t                    Version;                    ///< Version number
    uint32_t                    SubOptions;                 ///< Subscription options
    uint32_t                    InternalAttrs;              ///< Internal attributes
    uint32_t                    ClientIdLength;             ///< Length of client ID
    uint32_t                    SubNameLength;              ///< Length of subscription name
    uint32_t                    TopicStringLength;          ///< Length of topic string
    uint32_t                    SubPropertiesLength;        ///< Length of optional subscription properties
    uint64_t                    MaxMessages;                ///< Maximum message count for this subscription
} iestSubscriptionPropertiesRecord_V1_t;

/** @name State flags for the Message Delivery Reference (MDR)
 */

/**@{*/

#define iestMDR_STATE_NONE                0x00  ///< None
#define iestMDR_STATE_OWNER_IS_QUEUE      0x01  ///< MDR is for a message on a point-to-point queue
#define iestMDR_STATE_OWNER_IS_SUBSC      0x02  ///< MDR is for a message on a subscription
#define iestMDR_STATE_HANDLE_IS_RECORD    0x04  ///< MDR referred-to handle is the destination record
#define iestMDR_STATE_HANDLE_IS_MESSAGE   0x08  ///< MDR referred-to handle is the message record

/**@}*/


/** @name Values for the Transaction Operation Reference (TOR)
 */

/**@{*/

#define iestTOR_VALUE_PUT_MESSAGE             1  ///< Transaction operation is put-message
#define iestTOR_VALUE_OLD_STORE_SUBSC_DEFN    2  ///< Transaction operation is store-subscription-defn (migrated refs only)
#define iestTOR_VALUE_OLD_STORE_SUBSC_PROPS   3  ///< Transaction operation is store-subscription-props (migrated refs only)
#define iestTOR_VALUE_ADD_UMS                 4  ///< Transaction operation is add-UMS
#define iestTOR_VALUE_REMOVE_UMS              5  ///< Transaction operation is remove-UMS
#define iestTOR_VALUE_CONSUME_MSG             6  ///< Transaction operation is to remove a message from a queue

/**@}*/

/** @name State values for the Transaction Operation Reference (TOR)
 */

/**@{*/

#define iestTOR_STATE_NONE          0  ///< None

/**@}*/


//*********************************************************************
/// @brief  Bridge Queue Manager Record (BMR)
///
/// A record in the Store of type ISM_STORE_RECTYPE_BMGR.
///
/// Following this structure is the queue-manager record data.
//*********************************************************************
typedef struct tag_iestBridgeQMgrRecord_t
{
    char                            StrucId[4];                ///< Eyecatcher "EBMR"
    uint32_t                        Version;                   ///< Version number
    uint32_t                        DataLength;                ///< Length of record data
} iestBridgeQMgrRecord_t;

#define iestBMR_VERSION_1           1
#define iestBMR_CURRENT_VERSION     iestBMR_VERSION_1

/** @name State values for the Bridge Queue Manager Record
 */

/**@{*/

#define iestBMR_STATE_NONE          0x0 ///< None

/**@}*/

//*********************************************************************
/// @brief  Remote server Definition Record (RDR)
///
/// A record in the Store of type ISM_STORE_RECTYPE_REMSRV.
//*********************************************************************
typedef struct tag_iestRemoteServerDefinitionRecord_t
{
    char                            StrucId[4];                ///< Eyecatcher "ERDR"
    uint32_t                        Version;                   ///< Version number
    bool                            Local;                     ///< Is this the RDR for the local server?
} iestRemoteServerDefinitionRecord_t;

#define iestRDR_VERSION_1           1
#define iestRDR_CURRENT_VERSION     iestRDR_VERSION_1

/** @name State values for the Remote server Definition Record
 */

/**@{*/

#define iestRDR_STATE_NONE                0x00000000  ///< None
#define iestRDR_STATE_CREATING            0x00000100  ///< This remote server has not yet completed the creation process
#define iestRDR_STATE_DELETED             0x00000200  ///< This remote server has been deleted

/**@}*/

//*********************************************************************
/// @brief Maximum number of store record fragments for either RDR or RPR
//*********************************************************************
#define iestREMOTESERVER_MAX_FRAMGENTS 4

//*********************************************************************
/// @brief  Remote server Properties Record (SPR)
///
/// A record in the Store of type ISM_STORE_RECTYPE_RPROP.
///
/// Following this structure is the UID then Name as null-terminated
/// strings and optional remote server cluster data as an array of bytes.
///
/// If the length of any of these is zero, the corresponding data will
/// be omitted.
//*********************************************************************
typedef struct tag_iestRemoteServerPropertiesRecord_t
{
    char         StrucId[4];         ///< Eyecatcher "ERPR"
    uint32_t     Version;            ///< Version number
    uint32_t     InternalAttrs;      ///< Internal attributs
    size_t       UIDLength;          ///< Length of remote server UID
    size_t       NameLength;         ///< Length of remote server Name
    size_t       ClusterDataLength;  ///< Length of cluster data
} iestRemoteServerPropertiesRecord_t;

#define iestRPR_VERSION_1           1
#define iestRPR_CURRENT_VERSION     iestRPR_VERSION_1

/** @name State values for the Remote server Properties Record
 */

/**@{*/

#define iestRPR_STATE_NONE          0  ///< None

/**@}*/

/// @brief Calculate the store data length of a message
#define iest_MessageStoreDataLength( _pMsg ) \
    (sizeof(iestMessageHdrArea_t) + sizeof(iestMessageRecord_t) + (_pMsg)->MsgLength)

/// @brief Calculate the store data length of a remote server
#define iest_RemoteServerStorePropertiesDataLength( _pRemoteServer ) \
    (sizeof(iestRemoteServerPropertiesRecord_t) + \
     strlen((_pRemoteServer)->serverUID) + 1 + \
     strlen((_pRemoteServer)->serverName) + 1 + \
     (_pRemoteServer)->clusterDataLength)

/// @brief Options specified on the iest_storeMessage function
typedef enum
{
    iestStoreMessageOptions_NONE                 = 0,
    iestStoreMessageOptions_EXISTING_TRANSACTION = 0x00000001, // This storeMessage is happening during an existing store transaction
    iestStoreMessageOptions_ATOMIC_REFCOUNTING   = 0x00000002, // Atomic reference counting is required for this storeMessage
} iestStoreMessageOptions_t;

/*********************************************************************/
/*                                                                   */
/* FUNCTION PROTOTYPES                                               */
/*                                                                   */
/*********************************************************************/

int32_t iest_storeMessage( ieutThreadData_t *pThreadData
                         , ismEngine_Message_t *pMsg
                         , uint64_t refCountIncrement
                         , iestStoreMessageOptions_t options
                         , ismStore_Handle_t *phStoreMsg );

int32_t iest_unstoreMessage( ieutThreadData_t *pThreadData
                           , ismEngine_Message_t *pMsg
                           , bool storeSameTran
                           , bool useLazyRemoval
                           , ismStore_Handle_t *phHandleToUnstore
                           , uint32_t *pStoreUpdated);

void iest_unstoreMessageCommit( ieutThreadData_t *pThreadData
                              , ismEngine_Message_t *pMsg
                              , uint32_t StoreOpCount);

int32_t iest_finishUnstoreMessages( ieutThreadData_t *pThreadData
                                  , ismEngine_AsyncData_t *asyncInfo
                                  , size_t numStoreHandles
                                  , ismStore_Handle_t hHandleToUnstore[]);

int32_t iest_rehydrateMessage( ieutThreadData_t *pThreadData
                             , ismStore_Handle_t recHandle
                             , ismStore_Record_t *record
                             , ismEngine_RestartTransactionData_t *transData
                             , void **outData
                             , void *pContext);

int32_t iest_rehydrateMessageRef( ieutThreadData_t *pThreadData
                                , ismEngine_Message_t *pMsg );

uint64_t iest_getSPRSize(ieutThreadData_t *pThreadData,
                         iepiPolicyInfo_t *pPolicyInfo,
                         char *pTopicString,
                         ismEngine_Subscription_t *pSubscription);

int32_t iest_storeSubscription( ieutThreadData_t         *pThreadData
                              , const char               *topicString
                              , size_t                    topicStringLength
                              , ismEngine_Subscription_t *subscription
                              , size_t                    clientIdLength
                              , size_t                    subNameLength
                              , size_t                    subPropertiesLength
                              , ismQueueType_t            queueType
                              , iepiPolicyInfoHandle_t    pPolicyInfo
                              , ismStore_Handle_t        *phStoreSubscDefn
                              , ismStore_Handle_t        *phStoreSubscProps );

int32_t iest_updateSubscription( ieutThreadData_t         *pThreadData
                               , iepiPolicyInfo_t         *pPolicyInfo
                               , ismEngine_Subscription_t *pSubscription
                               , ismStore_Handle_t         hStoreSubscriptionDefn
                               , ismStore_Handle_t         hOldStoreSubscriptionProps
                               , ismStore_Handle_t        *phNewStoreSubscriptionProps
                               , bool                      commitUpdate );

int32_t iest_storeQueue(ieutThreadData_t         *pThreadData,
                        ismQueueType_t            type,
                        const char               *name,
                        ismStore_Handle_t        *phStoreQueueDefn,
                        ismStore_Handle_t        *phStoreQueueProps);

int32_t iest_updateQueue(ieutThreadData_t    *pThreadData,
                         ismStore_Handle_t    hStoreQueueDefn,
                         ismStore_Handle_t    hStoreQueueProps,
                         const char          *name,
                         ismStore_Handle_t   *phNewStoreQueueProps);

int32_t iest_storeRemoteServer(ieutThreadData_t         *pThreadData,
                               ismEngine_RemoteServer_t *remoteServer,
                               ismStore_Handle_t        *phStoreRemoteServerDefn,
                               ismStore_Handle_t        *phStoreRemoteServerProps);

int32_t iest_updateRemoteServers(ieutThreadData_t *pThreadData,
                                ismEngine_RemoteServer_t **remoteServers,
                                int32_t remoteServerCount);

void iest_store_reserve( ieutThreadData_t *pThreadData
                       , uint64_t DataLength
                       , uint32_t RecordsCount
                       , uint32_t RefsCount );

int32_t iest_store_asyncCommit( ieutThreadData_t *pThreadData
                              , bool commitReservation
                              , ismStore_CompletionCallback_t pCallback
                              , void *pContext );

void iest_store_commit( ieutThreadData_t *pThreadData
                      , bool commitReservation );

void iest_store_rollback( ieutThreadData_t *pThreadData
                        , bool rollbackReservation );

void iest_store_cancelReservation( ieutThreadData_t *pThreadData );

bool iest_store_startTransaction( ieutThreadData_t *pThreadData );

void iest_store_cancelTransaction( ieutThreadData_t *pThreadData );

int32_t iest_store_deleteReferenceCommit(
                       ieutThreadData_t *pThreadData,
                       ismStore_StreamHandle_t hStream,
                       void *hRefCtxt,
                       ismStore_Handle_t handle,
                       uint64_t orderId,
                       uint64_t minimumActiveOrderId);

int32_t iest_store_updateReferenceCommit(ieutThreadData_t *pThreadData,
                                        ismStore_StreamHandle_t hStream,
                                        void *hRefCtxt,
                                        ismStore_Handle_t handle,
                                        uint64_t orderId,
                                        uint8_t state,
                                        uint64_t minimumActiveOrderId);

int32_t iest_store_createReferenceCommit(ieutThreadData_t *pThreadData,
                                        ismStore_StreamHandle_t hStream,
                                        void *hRefCtxt,
                                        const ismStore_Reference_t *pReference,
                                        uint64_t minimumActiveOrderId,
                                        ismStore_Handle_t *pHandle);

int32_t iest_storeEventHandler(ismStore_EventType_t eventType, void *pContext);

// NOTE: We only need ismMESSAGE_AREA_COUNT + 1 fragments (one for MsgRecord, one for the
//       ismMESSAGE_AREA_INTERNAL_HEADER and up to 1 each for the other area types)...
// HOWEVER, this has always been coded as ismMESSAGE_AREA_COUNT+2, so rather than potentially
// change the structure we write to the store, it is left as ismMESSAGE_AREA_COUNT+2.
void iest_setupPersistedMsgData(
                        ieutThreadData_t *pThreadData
                      , ismEngine_Message_t *pMsg
                      , iestMessageRecord_t *pMsgRecord
                      , iestMessageHdrArea_t *pMsgHdrArea
                      , uint32_t *pTotalDataLength
                      , char *Frags[ismMESSAGE_AREA_COUNT + 2]
                      , uint32_t FragLengths[ismMESSAGE_AREA_COUNT + 2]);

#ifndef ASYNCDATA_FORWARDDECLARATION
#define ASYNCDATA_FORWARDDECLARATION
typedef struct tag_ismEngine_AsyncData_t      ismEngine_AsyncData_t; //forward declaration...defined in engineAsync.h
#endif
//If we've run out of space to remember messages to delete at some point, do it here
int32_t iest_checkLazyMessages( ieutThreadData_t *pThreadData
                              , ismEngine_AsyncData_t *asyncInfo);

#define iest_AssertStoreCommitAllowed( _pThreadData ) \
    assert((_pThreadData)->ReservationState == Inactive)

#endif /* __ISM_ENGINESTORE_DEFINED */

/*********************************************************************/
/* End of engineStore.h                                              */
/*********************************************************************/
