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
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/time.h>

#include <cmqc.h>
#include <cmqxc.h>
#include <cmqcfc.h>
#include <ismc.h>
#include <ismc_p.h>
#include <mqrc.h>
#include <xa.h>

#include <config.h>

#ifdef TRACE_COMP
#undef TRACE_COMP
#endif

#define TRACE_COMP MQConn

/* Constants */
#define MQC_UNLIMITED -1
#define MAX_API_LENGTH 32

/* When we read /proc/meminfo we keep the buffer on the stack. The    */
/* numbers we want are in the first few lines so the buffer needn't   */
/* be large.                                                          */
#define MQC_MEMINFO_BUFFERSIZE 256

#ifdef MQC_GLOBAL_INIT
  #define mqcXA_RET_CODES
#endif

typedef struct _XARetCode {
    int32_t xarc;
    char *pSymbol;
} XARetCode;

#define XA_ENTRY(ret) {(ret), #ret}

#ifdef mqcXA_RET_CODES
  const XARetCode XASymbols[] =
{
    {   XAER_ASYNC, "XAER_ASYNC"},
    {   XAER_RMERR, "XAER_RMERR"},
    {   XAER_NOTA, "XAER_NOTA"},
    {   XAER_INVAL, "XAER_INVAL"},
    {   XAER_PROTO, "XAER_PROTO"},
    {   XAER_RMFAIL, "XAER_RMFAIL"},
    {   XAER_DUPID, "XAER_DUPID"},
    {   XAER_OUTSIDE, "XAER_OUTSIDE"},
    {   XA_RBROLLBACK, "XA_RBROLLBACK"},
    {   XA_RBTIMEOUT, "XA_RBTIMEOUT"},
    {   XA_OK, "XA_OK"} /* Must be the last entry. */
};
#else
  extern const XARetCode XASymbols[];
#endif

/* Tuning Parameters */

/* These values should be supplied by the static configuration file. */
/* However, the constants defined here provide defaults in case the  */
/* config file doesn't supply what is needed.                        */
#define MQC_BATCH_SIZE          200
#define MQC_BATCH_DATA_LIMIT    5120000
#define MQC_LONG_BATCH_TIMEOUT  2000
#define MQC_SHORT_BATCH_TIMEOUT 1
#define MQC_PURGE_XIDS          0

#define MQC_IMMEDIATE_RETRY_COUNT    1
#define MQC_SHORT_RETRY_INTERVAL     10
#define MQC_SHORT_RETRY_COUNT        6
#define MQC_LONG_RETRY_INTERVAL      60
#define MQC_LONG_RETRY_COUNT         MQC_UNLIMITED
#define MQC_DEFAULT_CONNECTION_COUNT 1
#define MQC_RULE_LIMIT               64
#define MQC_MQ_TRACE_MAX_FILE_SIZE   512
#define MQC_MQ_TRACE_USER_DATA_SIZE  1024
#define MQC_ENABLE_READ_AHEAD        0
#define MQC_DISCARD_UNDELIVERABLE    1

#define MANAGER_RECORD_EYECATCHER 0x4D475252 /* "MGRR" */
#define MANAGER_RECORD_VERSION 0x00010000

/* The size of the product id field is defined by MQ in the */
/* rfpPRDIDSPIIN struct.                                    */
#define PRODUCT_ID_SIZE 12
#define CLIENT_PRODUCT_ID_ARRAY 'I','X','M','C',0,0,0,0,0,0,0,0
#define MQC_OBJECT_IDENTIFIER_LENGTH 12

#define ERROR_INSERT_SIZE 64

/* Trace Levels */
#define MQC_FNC_TRACE         7
#define MQC_MQAPI_TRACE       7
#define MQC_ISMC_TRACE        7
#define MQC_NORMAL_TRACE      7
#define MQC_HIGH_TRACE        8
#define MQC_INTERESTING_TRACE 5

#define FUNCTION_ENTRY ">>> %s "
#define FUNCTION_EXIT  "<<< %s "

#define MODEL_REPLY_QUEUE "SYSTEM.DEFAULT.MODEL.QUEUE"
#define COMMAND_QUEUE "SYSTEM.ADMIN.COMMAND.QUEUE"

/* Rule Types */
/* mqcRULE_Any is used only by the monitoring interface */

typedef enum _mqcRuleType_t {
    mqcRULE_Any = -1,
    mqcRULE_None = 0,
    mqcRULE_FROM_IMATOPIC_TO_MQQUEUE = 1,
    mqcRULE_FROM_IMATOPIC_TO_MQTOPIC = 2,
    mqcRULE_FROM_MQQUEUE_TO_IMATOPIC = 3,
    mqcRULE_FROM_MQTOPIC_TO_IMATOPIC = 4,
    mqcRULE_FROM_IMAWILDTOPIC_TO_MQQUEUE = 5,
    mqcRULE_FROM_IMAWILDTOPIC_TO_MQTOPIC = 6,
    mqcRULE_FROM_IMAWILDTOPIC_TO_MQWILDTOPIC = 7,
    mqcRULE_FROM_MQWILDTOPIC_TO_IMATOPIC = 8,
    mqcRULE_FROM_MQWILDTOPIC_TO_IMAWILDTOPIC = 9,
    mqcRULE_FROM_IMAQUEUE_TO_MQQUEUE = 10,
    mqcRULE_FROM_IMAQUEUE_TO_MQTOPIC = 11,
    mqcRULE_FROM_MQQUEUE_TO_IMAQUEUE = 12,
    mqcRULE_FROM_MQTOPIC_TO_IMAQUEUE = 13,
    mqcRULE_FROM_MQWILDTOPIC_TO_IMAQUEUE = 14
} mqcRuleType_t;

typedef enum _mqcXIDType_t {
    mqcXID_Alien = 0,
    mqcXID_Local = 1,
    mqcXID_Match = 2,
    mqcXID_UnknownRule = 3,
    mqcXID_UnknownQM = 4
} mqcXIDType_t;

/* Possible settings for the RetainedMessages property of a Destination   */
/* Mapping Rule.                                                          */
typedef enum _mqcRetainedType_t {
    mqcRetained_None = 0,
    mqcRetained_All = 1
} mqcRetainedType_t;

/* MQ Connectivity Flags */
#define MQC_DISABLE                  0x00000001
#define MQC_DISABLED                 0x00000002
#define MQC_RECONNECT                0x00000004
#define MQC_RECONNECTING             0x00000008
#define MQC_IMMEDIATE_RECONNECT      0x00000010

/* QM Flags (pQM) */
#define QM_COLDSTART                 0x00000001

/* Rule Flags (pRule) */
#define RULE_DISABLE                 0x00000001
#define RULE_USER_DISABLE            0x00000002
#define RULE_DISABLED                0x00000004
#define RULE_RECONNECT               0x00000008
#define RULE_RECONNECTING            0x00000010
#define RULE_IMA_CONSUMER_CREATED    0x00000020
#define RULE_IMA_XA_STARTED          0x00000040
#define RULE_IMA_XA_COMMIT           0x00000080
#define RULE_COUNTED                 0x00000100
#define RULE_PROTECTED_UPDATE        0x00000200
#define RULE_MAXMESSAGES_UPDATED     0x00000400
#define RULE_USE_NOLOCAL             0x00000800

/* RuleQM Flags (pRuleQM) */
#define RULEQM_DISABLE               0x00000001
#define RULEQM_USER_DISABLE          0x00000002
#define RULEQM_DISABLED              0x00000004
#define RULEQM_RECONNECT             0x00000008
#define RULEQM_RECONNECTING          0x00000010
#define RULEQM_QUIESCING             0x00000020
#define RULEQM_QUIESCED              0x00000040
#define RULEQM_MQ_XA_OPENED          0x00000080
#define RULEQM_MQ_XA_STARTED         0x00000100
#define RULEQM_MQ_XA_COMMIT          0x00000200
#define RULEQM_READ_AHEAD            0x00000400
#define RULEQM_IMPLICIT_XA_END       0x00000800
#define RULEQM_MQSTAT_REQUIRED       0x00001000
#define RULEQM_IMA_XA_STARTED        0x00002000
#define RULEQM_IMA_XA_COMMIT         0x00004000
#define RULEQM_NON_DURABLE           0x00008000
#define RULEQM_NO_SYNCPOINT          0x00010000
#define RULEQM_CONNECTED             0x00020000
#define RULEQM_STARTING              0x00040000
#define RULEQM_IDLE_CONN_BROKEN      0x00080000
#define RULEQM_MCAST_TOPIC_STATUS    0x00100000
#define RULEQM_MULTICAST             0x00200000
#define RULEQM_SHARING_CONVERSATIONS 0x00400000
#define RULEQM_USEDLQ                0x00800000

/* IMA Flags */
#define IMA_CONNECTED                0x00000001
#define IMA_PURGE_XIDS               0x00000002

/* Override behavior */
#define MQC_NO_OVERRIDE              -1
#define MQC_OVERRIDE_NOLOCAL_FALSE    0
#define MQC_OVERRIDE_NOLOCAL_TRUE     1

/* Return codes */
#define RC_RULE_DISABLE              1
#define RC_RULE_RECONNECT            2
#define RC_RULEQM_DISABLE            3
#define RC_RULEQM_RECONNECT          4
#define RC_MQC_DISABLE               5
#define RC_MQC_RECONNECT             6
#define RC_MQC_IMMEDIATE_RECONNECT   7
#define RC_NOT_DISABLED              8
#define RC_INVALID_STATE_UPDATE      9
#define RC_INVALID_CONFIG            10
#define RC_INVALID_TOPIC             11
#define RC_DISCARD_MESSAGE           12
#define RC_NOT_FOUND                 13
#define RC_TOO_MANY_RULES            14
#define RC_UNRESOLVED                15

/* Return codes with associated messages */
#define RC_ISMCError                 7005
#define RC_ISMCRuleError             7006
#define RC_ISMCRuleQMError           7007
#define RC_ISMCNetworkError          7008
#define RC_ISMCDestNotValidError     7009

#define RC_MQAPIError                7010
#define RC_MQCONNXError              7011
#define RC_MQOPENQueueError          7012
#define RC_MQOPENTopicError          7013
#define RC_MQSUBError                7014
#define RC_MQSUBTopicError           7015
#define RC_MQPUTQueueError           7016
#define RC_PCFMQGETError             7017
#define RC_MQCreateSyncQError        7018
#define RC_MQDeleteSyncQError        7019

#define RC_InvalidMessage            7020
#define RC_AllocateError             7021
#define RC_AllocateRuleQMError       7022
#define RC_SystemAPIError            7023
#define RC_MQConnected               7024

#define RC_MonitorParseError         7025
#define RC_MonitorActionUnsupported  7026

#define RC_Starting                  7027
#define RC_MQAPILengthError          7028
#define RC_MQCBError                 7029

/* Macros */
#define MQ_CONSUME_RULE(a)                                             \
    ((a -> RuleType == mqcRULE_FROM_MQQUEUE_TO_IMATOPIC)         ||    \
     (a -> RuleType == mqcRULE_FROM_MQTOPIC_TO_IMATOPIC)         ||    \
     (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMATOPIC)     ||    \
     (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMAWILDTOPIC) ||    \
     (a -> RuleType == mqcRULE_FROM_MQQUEUE_TO_IMAQUEUE)         ||    \
     (a -> RuleType == mqcRULE_FROM_MQTOPIC_TO_IMAQUEUE)         ||    \
     (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMAQUEUE))

#define MQ_TOPIC_CONSUME_RULE(a)                                       \
    ((a -> RuleType == mqcRULE_FROM_MQTOPIC_TO_IMATOPIC)         ||    \
     (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMATOPIC)     ||    \
     (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMAWILDTOPIC) ||    \
     (a -> RuleType == mqcRULE_FROM_MQTOPIC_TO_IMAQUEUE)         ||    \
     (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMAQUEUE))

#define MQ_QUEUE_CONSUME_RULE(a)                                       \
    ((a -> RuleType == mqcRULE_FROM_MQQUEUE_TO_IMATOPIC)         ||    \
     (a -> RuleType == mqcRULE_FROM_MQQUEUE_TO_IMAQUEUE))

#define IMA_CONSUME_RULE(a)                                            \
    ((a -> RuleType == mqcRULE_FROM_IMATOPIC_TO_MQQUEUE)         ||    \
     (a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQQUEUE)     ||    \
     (a -> RuleType == mqcRULE_FROM_IMATOPIC_TO_MQTOPIC)         ||    \
     (a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQTOPIC)     ||    \
     (a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQWILDTOPIC) ||    \
     (a -> RuleType == mqcRULE_FROM_IMAQUEUE_TO_MQQUEUE)         ||    \
     (a -> RuleType == mqcRULE_FROM_IMAQUEUE_TO_MQTOPIC))

#define IMA_TOPIC_CONSUME_RULE(a)                                      \
    ((a -> RuleType == mqcRULE_FROM_IMATOPIC_TO_MQQUEUE)         ||    \
     (a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQQUEUE)     ||    \
     (a -> RuleType == mqcRULE_FROM_IMATOPIC_TO_MQTOPIC)         ||    \
     (a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQTOPIC)     ||    \
     (a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQWILDTOPIC))

#define NONWILD_MQ_PRODUCE_RULE(a)                                     \
    ((a -> RuleType == mqcRULE_FROM_IMATOPIC_TO_MQQUEUE)         ||    \
     (a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQQUEUE)     ||    \
     (a -> RuleType == mqcRULE_FROM_IMATOPIC_TO_MQTOPIC)         ||    \
     (a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQTOPIC)     ||    \
     (a -> RuleType == mqcRULE_FROM_IMAQUEUE_TO_MQQUEUE)         ||    \
     (a -> RuleType == mqcRULE_FROM_IMAQUEUE_TO_MQTOPIC))

#define NONWILD_IMA_PRODUCE_RULE(a)                                    \
    ((a -> RuleType == mqcRULE_FROM_MQQUEUE_TO_IMATOPIC)         ||    \
     (a -> RuleType == mqcRULE_FROM_MQTOPIC_TO_IMATOPIC)         ||    \
     (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMATOPIC)     ||    \
     (a -> RuleType == mqcRULE_FROM_MQQUEUE_TO_IMAQUEUE)         ||    \
     (a -> RuleType == mqcRULE_FROM_MQTOPIC_TO_IMAQUEUE)         ||    \
     (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMAQUEUE))

#define MQ_QUEUE_PRODUCE_RULE(a)                                       \
    ((a -> RuleType == mqcRULE_FROM_IMATOPIC_TO_MQQUEUE)         ||    \
     (a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQQUEUE)     ||    \
     (a -> RuleType == mqcRULE_FROM_IMAQUEUE_TO_MQQUEUE))

#define MQ_TOPIC_PRODUCE_RULE(a)                                       \
    ((a -> RuleType == mqcRULE_FROM_IMATOPIC_TO_MQTOPIC)         ||    \
     (a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQTOPIC)     ||    \
     (a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQWILDTOPIC) ||    \
     (a -> RuleType == mqcRULE_FROM_IMAQUEUE_TO_MQTOPIC))

#define IMA_QUEUE_PRODUCE_RULE(a)                                      \
    ((a -> RuleType == mqcRULE_FROM_MQQUEUE_TO_IMAQUEUE)         ||    \
     (a -> RuleType == mqcRULE_FROM_MQTOPIC_TO_IMAQUEUE))

#define IMA_TOPIC_PRODUCE_RULE(a)                                      \
    ((a -> RuleType == mqcRULE_FROM_MQQUEUE_TO_IMATOPIC)         ||    \
     (a -> RuleType == mqcRULE_FROM_MQTOPIC_TO_IMATOPIC)         ||    \
     (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMATOPIC)     ||    \
     (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMAWILDTOPIC))

#define WILD_IMA_PRODUCE_RULE(a)                                       \
    (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMAWILDTOPIC)

#define WILD_PRODUCE_RULE(a)                                           \
    ((a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQWILDTOPIC) ||    \
     (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMAWILDTOPIC))

#define WILD_CONSUME_RULE(a)                                           \
    ((a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQQUEUE)     ||    \
     (a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQTOPIC)     ||    \
     (a -> RuleType == mqcRULE_FROM_IMAWILDTOPIC_TO_MQWILDTOPIC) ||    \
     (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMATOPIC)     ||    \
     (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMAWILDTOPIC) ||    \
     (a -> RuleType == mqcRULE_FROM_MQWILDTOPIC_TO_IMAQUEUE))

/* Read Barrier: */
/* Reads (loads) from memory can not be reordered (by compiler or */
/* CPU) across this barrier */
#define imaEngine_ReadMemoryBarrier() asm volatile("lfence":::"memory")
//Write Barrier: Stores to memory can not be reordered (by compiler or CPU)
//across this barrier
#define imaEngine_WriteMemoryBarrier() asm volatile("sfence":::"memory")
//Full Barrier, read or writes to memory can not the reordered (by compiler or CPU)
//across this barrier
#define imaEngine_FullMemoryBarrier() asm volatile("mfence":::"memory")

typedef enum _monitorStatistic_t {
    mqcMON_STAT_None = 0,
    mqcMON_STAT_DestinationMappingRule = 1
} monitorStatistic_t;

typedef enum _ruleStatistic_t {
    mqcRULE_STAT_None = 0,
    mqcRULE_STAT_CommitCount = 1,
    mqcRULE_STAT_RollbackCount = 2,
    mqcRULE_STAT_PersistentCount = 3,
    mqcRULE_STAT_NonpersistentCount = 4,
    mqcRULE_STAT_CommittedMessageCount = 5,
    mqcRULE_STAT_Status = 6
} ruleStatistic_t;

/* Structures */

typedef struct _mqcQueueManagerConnection_t {
    int index;
    int length;
    char name[1];
} mqcQueueManagerConnection_t;

typedef struct _mqcDestinationMappingRule_t {
    int index;
    int length;
    char name[1];
} mqcDestinationMappingRule_t;

typedef struct _mqcManagerRecord_t {
    int eyecatcher;
    int version;
    int size;
    int type;
    union {
        char objectIdentifier[MQC_OBJECT_IDENTIFIER_LENGTH];
        int lastindex;
        mqcQueueManagerConnection_t qmc;
        mqcDestinationMappingRule_t dmr;
    } object;
} mqcManagerRecord_t;

#define MRTYPE_OBJECTIDENTIFIER 0 /* Previously MRTYPE_MACADDRESS */
#define MRTYPE_QMC              1
#define MRTYPE_DMR              2
#define MRTYPE_QMC_LASTINDEX    3
#define MRTYPE_DMR_LASTINDEX    4

typedef struct _mqcXIDListElement_t {
    struct _mqcXIDListElement_t *pNext;
    ism_xid_t xid;
    mqcXIDType_t xidType;
} mqcXIDListElement_t;

/* IMA */
typedef struct _mqcIMA_t {
    char StrucId[4];
    /* config */
    char * pIMAProtocol;
    char * pIMAAddress;
    int    IMAPort;
    char * pIMAClientID;
    /* internal */
    int    flags;
    int    errorMsgId;
    char   errorInsert[3][ERROR_INSERT_SIZE + 1];
    ismc_connection_t * pConn;
    ism_time_t recoveryTimestamp;
    pthread_mutex_t xa_recoverMutex;
    mqcXIDListElement_t *pRecoveredXIDs_IMA;
} mqcIMA_t;

#define IMA_STRUC_ID       "IMA "
#define IMA_STRUC_ID_ARRAY 'I','M','A',' '

#define IMA_INIT                                                       \
    {IMA_STRUC_ID_ARRAY},              /* StrucId                   */ \
    NULL,                              /* pIMAProtocol              */ \
    NULL,                              /* pIMAAddress               */ \
    0,                                 /* IMAPort                   */ \
    NULL,                              /* pIMAClientID              */ \
    0,                                 /* flags                     */ \
    0,                                 /* errorMsgId                */ \
    {"", "", ""},                      /* errorInsert                 */ \
    NULL,                              /* pConn                     */ \
    0,                                 /* recoveryTimestamp         */ \
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,                            \
                                       /* xa_recoverMutex           */ \
    NULL                               /* pRecoveredXIDs_IMA        */

/* Queue manager */
typedef struct _mqcQM_t {
    char StrucId[4];
    /* config */
    char * pName;
    char * pDescription;
    char * pQMName;
    char * pChannelUserName;
    char * pChannelPassword;
    int lengthOfQMName;
    MQLONG platformType;
    MQCHAR QMgrIdentifier[MQ_Q_MGR_IDENTIFIER_LENGTH];
    MQCHAR dlqName[MQ_Q_NAME_LENGTH + 1];
    char * pConnectionName;
    char * pChannelName;
    int BatchSize;
    char * pSSLCipherSpec;
    /* internal */
    int flags;
    int index;
    struct ismc_manrec_t indexRecord;
    struct _mqcQM_t * pNext;
    ism_time_t recoveryTimestamp;
    pthread_mutex_t xa_recoverMutex;
} mqcQM_t;

#define QM_STRUC_ID       "QM  "
#define QM_STRUC_ID_ARRAY 'Q','M',' ',' '

#define QM_INIT                                                        \
    {QM_STRUC_ID_ARRAY},               /* StrucId                   */ \
    NULL,                              /* pName                     */ \
    NULL,                              /* pDescription              */ \
    NULL,                              /* pQMName                   */ \
    NULL,                              /* pChannelUserName          */ \
    NULL,                              /* pChannelPassword          */ \
    0,                                 /* lengthOfQMName            */ \
    0,                                 /* platformType              */ \
    "",                                /* QMgrIdentifier            */ \
    "",                                /* dlqName                   */ \
    NULL,                              /* pConnectionName           */ \
    NULL,                              /* pChannelName              */ \
    MQC_BATCH_SIZE,                    /* BatchSize                 */ \
    NULL,                              /* pSSLCipherSpec            */ \
    0,                                 /* flags                     */ \
    0,                                 /* index                     */ \
    {{0,0,0,0},0},                     /* indexRecord               */ \
    NULL,                              /* pNext                     */ \
    0,                                 /* recoveryTimestamp         */ \
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP, /* xa_recoverMutex      */

/* rule queue manager handles */

/* syncQName is the name of the queue that we create on the queue manager */
/* associated with this rule and use to ensure that only one instance of  */
/* this program is active on the connection via this rule at a time.      */
/* The queue name is of the form ...                                      */
/* SYSTEM.IMA.<12 character mac address>.<8 character rule index>.QUEUE   */
/* eg SYSTEM.IMA.001641EDA6BB.0000.QUEUE                                  */
/* The mac address is required so that two different IMAs don't try to    */
/* use the same queue. We frequently use C string operations (eg strcmp)  */
/* to manipulate fragments of this field. In principle, this is risky     */
/* because the null terminator might spill off the end of the MQCHAR48    */
/* field, however the queue name as defined is 38 characters long so in   */
/* practice this isn't an issue.                                          */

typedef struct _mqcRuleQM_t {
    char StrucId[4];

    mqcQM_t * pQM;
    struct _mqcRule_t * pRule;
    volatile int flags;
    int      index;

    int32_t  errorMsgId;
    char     errorInsert[4][ERROR_INSERT_SIZE + 1];
    int32_t  MQConnectivityRC;
    int32_t  OtherRC; /* MQ or XA */
    char   * pOtherRCSymbol;
    char     errorQueueName[MQ_Q_NAME_LENGTH + 1];
    char     errorAPI[MAX_API_LENGTH];

    pthread_mutex_t controlMutex;
    /* MQPUT hConn */
    MQHCONN  hPubConn;
    /* MQSUB hConn */
    MQHCONN  hSubConn;
    /* MQSUB ConnectionId */
    MQBYTE24 subConnectionId;
    /* Name of synchronisation queue */
    MQCHAR   syncQName[MQ_Q_NAME_LENGTH + 1];
    /* Subscription name */
    MQCHAR   subName[MQ_SUB_NAME_LENGTH + 1];
    /* Managed queue name */
    MQCHAR   managedQName[MQ_Q_NAME_LENGTH + 1];
    /* MQ Version */
    int      vrmf[4];
    MQHOBJ   hObjCmdQ;
    MQHOBJ   hObjReplyQ;
    /* MQSUB handles */
    MQHOBJ   hObj;
    MQHOBJ   hSub;
    /* MQPUT topic / queue handle */
    MQHOBJ   hPutObj;
    /* Sync queue handle */
    MQHOBJ   hSyncObj;
    /* Message handle */
    MQHMSG   hMsg;
    /* MQGET buffer / length */
    void *pActualBuffer;
    MQLONG actualBufferLength;
    void   * buffer;
    MQLONG   bufferlen;
    /* Properties buffer */
    concat_alloc_t buf;
    /* XA thread handle */
    ism_threadh_t xa_thread_handle;
    bool xa_thread_started;
    ism_time_t xaInitialTime;
    uint64_t xaSequenceNumber;
    mqcXIDListElement_t *pRecoveredXIDs_MQ;
    mqcXIDListElement_t *pRecoveredXIDs_IMA;
    /* ismc publisher */
    ismc_session_t * pSess;
    ismc_session_t * pNonTranSess;
    ismc_message_t * pMessage;
    ismc_message_t * pNonTranMessage;
    ismc_destination_t * pDest;
    ismc_producer_t * pProd;
    ismc_producer_t * pNonTranProd;
    /* time ruleQM was disabled */
    time_t disabledTime;
    int immediateRetries;
    int shortRetries;
    int longRetries;
    char clientProductId[PRODUCT_ID_SIZE];
    /* pNext */
    struct _mqcRuleQM_t * pNext;
    /* Message counts */
    int batchMsgsSent;
    int batchDataSent;
    int batchMsgsToResend;
    /* Monitoring */
    uint64_t nonpersistentCount;
    uint64_t persistentCount;
    uint64_t commitCount;
    uint64_t rollbackCount;
    uint64_t committedMessageCount;
    ism_xid_t xid;
} mqcRuleQM_t;

#define RULEQM_STRUC_ID       "RQM "
#define RULEQM_STRUC_ID_ARRAY 'R','Q','M',' '

#define RULEQM_INIT                                                    \
    {RULEQM_STRUC_ID_ARRAY},           /* StrucId                   */ \
    NULL,                              /* pQM                       */ \
    NULL,                              /* pRule                     */ \
    RULEQM_DISABLED,                   /* flags                     */ \
    0,                                 /* index                     */ \
    0,                                 /* errorMsgId                */ \
    {"", "", "", ""},                  /* errorInsert                 */ \
    0,                                 /* MQConnectivityRC          */ \
    0,                                 /* OtherRC                   */ \
    NULL,                              /* pOtherRCSymbol            */ \
    "",                                /* errorQueueName            */ \
    "",                                /* errorAPI                  */ \
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,                            \
                                       /* controlMutex              */ \
    MQHC_UNUSABLE_HCONN,               /* hPubConn                  */ \
    MQHC_UNUSABLE_HCONN,               /* hSubConn                  */ \
    {MQCONNID_NONE_ARRAY},             /* subConnectionId           */ \
    "",                                /* syncQName                 */ \
    "",                                /* subName                   */ \
    "",                                /* managedQName              */ \
    {0,0,0,0},                         /* vrmf                      */ \
    MQHO_UNUSABLE_HOBJ,                /* hObjCmdQ                  */ \
    MQHO_UNUSABLE_HOBJ,                /* hObjReplyQ                */ \
    MQHO_UNUSABLE_HOBJ,                /* hObj                      */ \
    MQHO_UNUSABLE_HOBJ,                /* hSub                      */ \
    MQHO_UNUSABLE_HOBJ,                /* hPutObj                   */ \
    MQHO_UNUSABLE_HOBJ,                /* hSyncObj                  */ \
    MQHM_UNUSABLE_HMSG,                /* hMsg                      */ \
    NULL,                              /* pActualBuffer             */ \
    0,                                 /* actualBufferLength        */ \
    NULL,                              /* buffer                    */ \
    0,                                 /* bufferlen                 */ \
    { NULL, 0, 0, 0 },                 /* buf                       */ \
    0,                                 /* xa_thread_handle          */ \
    false,                             /* xa_thread_started         */ \
    0,                                 /* xaInitialTime             */ \
    0,                                 /* xaSequenceNumber          */ \
    NULL,                              /* pRecoveredXIDs_MQ         */ \
    NULL,                              /* pRecoveredXIDs_IMA        */ \
    NULL,                              /* pSess                     */ \
    NULL,                              /* pNonTranSess              */ \
    NULL,                              /* pMessage                  */ \
    NULL,                              /* pNonTranMessage           */ \
    NULL,                              /* pDest                     */ \
    NULL,                              /* pProd                     */ \
    NULL,                              /* pNonTranProd              */ \
    0,                                 /* disabledTime              */ \
    0,                                 /* immediateRetries          */ \
    0,                                 /* shortRetries              */ \
    0,                                 /* longRetries               */ \
    { CLIENT_PRODUCT_ID_ARRAY },       /* clientProductId           */ \
    NULL,                              /* pNext                     */ \
    0,                                 /* batchMsgsSent             */ \
    0,                                 /* batchDataSent             */ \
    0,                                 /* batchMsgsToResend         */ \
    0,                                 /* nonpersistentCount        */ \
    0,                                 /* persistentCount           */ \
    0,                                 /* commitCount               */ \
    0,                                 /* rollbackCount             */ \
    0,                                 /* committedMessageCount     */ \
    {0,0,0,{0}}                        /* xid                       */

/* Rule */

/* advanceToNextQM - a boolean, true when one of the committer threads    */
/* instructs the consumer thread to start sending messages to a different */
/* destination queue manager, if one is available. Represented as an int  */
/* so that it can be accessed with synchronised instructions.             */

/* sourceEndOffset - when making wildcard subscriptions we append a '#'   */
/* '/#' to the given source topic string. However, if the rule type ever  */
/* changes we will need to restore the original string so we keep a note  */
/* of the offset of the null terminator in the original string.           */
typedef struct _mqcRule_t {
    char StrucId[4];
    int receiveActive;
    /* config */
    char * pName;
    char * pDescription;
    mqcRuleType_t RuleType;
    char * pQueueManagerConnection;
    char * pSource;
    char * pDestination;
    int sourceEndOffset;
    int Enabled;
    int MaxMessages;
    mqcRetainedType_t RetainedMessages;
    int SubLevel;
    int errorMsgId;
    char errorInsert[2][ERROR_INSERT_SIZE + 1];
    /* internal */
    int flags;
    int index;
    struct ismc_manrec_t indexRecord;
    pthread_mutex_t ruleQMListMutex;
    pthread_mutex_t XIDTimestampMutex;
    ism_time_t mostRecentInitialTime;
    int ruleQMCount;
    mqcRuleQM_t * pRuleQMList;
    /* time rule was disabled */
    time_t disabledTime;
    int immediateRetries;
    int shortRetries;
    int longRetries;
    /* ismc consumer */
    ismc_session_t * pSess;
    ismc_destination_t * pDest;
    ismc_consumer_t * pConsumer;
    ism_xid_t xid;
    struct _mqcRule_t * pNext;
} mqcRule_t;

#define RULE_STRUC_ID       "RULE"
#define RULE_STRUC_ID_ARRAY 'R','U','L','E'

#define RULE_INIT                                                     \
    {RULE_STRUC_ID_ARRAY},             /* StrucId                   */ \
    0,                                 /* receiveActive             */ \
    NULL,                              /* pName                     */ \
    NULL,                              /* pDescription              */ \
    mqcRULE_None,                      /* RuleType                  */ \
    NULL,                              /* pRuleQM                   */ \
    NULL,                              /* pSource                   */ \
    NULL,                              /* pDestination              */ \
    0,                                 /* sourceEndOffset           */ \
    0,                                 /* Enabled                   */ \
    0,                                 /* MaxMessages               */ \
    mqcRetained_None,                  /* RetainedMessages          */ \
    1,                                 /* SubLevel                  */ \
    0,                                 /* errorMsgId                */ \
    {"", ""},                          /* errorInsert                 */ \
    RULE_DISABLED,                     /* flags                     */ \
    0,                                 /* index                     */ \
    {{0,0,0,0},0},                     /* indexRecord               */ \
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,                            \
                                       /* ruleQMListMutex           */ \
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,                            \
                                       /* XIDTimestampMutex         */ \
    0,                                 /* mostRecentInitialTime     */ \
    0,                                 /* ruleQMCount               */ \
    NULL,                              /* pRuleQMList               */ \
    0,                                 /* disabledTime              */ \
    0,                                 /* immediateRetries          */ \
    0,                                 /* shortRetries              */ \
    0,                                 /* longRetries               */ \
    NULL,                              /* pSess                     */ \
    NULL,                              /* pDest                     */ \
    NULL,                              /* pConsumer                 */ \
    {0,0,0,{0}},                       /* xid                       */ \
    NULL                               /* pNext                     */

/* MQ Connectivity itself */
typedef struct _mqcMQC_t {
    char StrucId[4];
    int flags;
    pthread_mutex_t controlMutex;
    ism_threadh_t reconnect_thread_handle;
    bool reconnect_thread_started;
    mqcIMA_t ima;
    pthread_mutex_t qmListMutex;
    mqcQM_t * pQMList;
    pthread_mutex_t ruleListMutex;
    mqcRule_t * pRuleList;
    uint32_t ruleLimit;
    uint32_t ruleCount;
    uint32_t discardUndeliverable;
    char objectIdentifier[MQC_OBJECT_IDENTIFIER_LENGTH + 1];
    ism_prop_t * props;
    /* time MQ Connectivity was disabled */
    time_t disabledTime;
    pthread_cond_t reconnectCond;
    pthread_mutex_t reconnectMutex;
    pthread_cond_t immediateCond;
    pthread_mutex_t immediateMutex;
    int immediateRetries;
    int shortRetries;
    int longRetries;
    int immediateRetryCount;
    int shortRetryInterval;
    int shortRetryCount;
    int longRetryInterval;
    int longRetryCount;
    int batchSize;
    int batchDataLimit;
    int longBatchTimeout;
    int shortBatchTimeout;
    int defaultConnectionCount;
    int MQTraceMaxFileSize;
    int MQTraceUserDataSize;
    int8_t overrideNoLocalAtMessageSight;
    bool traceAdmin;
    int traceAdminLevel;
    int enableReadAhead;
    int lastQMIndex;
    int lastRuleIndex;
    struct ismc_manrec_t lastQMIndexRecord;
    struct ismc_manrec_t lastRuleIndexRecord;
    pthread_mutex_t coldStartMutex;
    pthread_mutex_t adminCallbackMutex;
    int currentAllocatedMemory;
    char *MQInstallationPath;
} mqcMQC_t;

#define MQC_STRUC_ID       "MQC "
#define MQC_STRUC_ID_ARRAY 'M','Q','C',' '

#define MQC_INIT                                                             \
    {MQC_STRUC_ID_ARRAY},              /* StrucId                         */ \
    MQC_DISABLED,                      /* flags                           */ \
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,                                  \
                                       /* controlMutex                    */ \
    0,                                 /* reconnect_thread_handle         */ \
    false,                             /* reconnect_thread_started        */ \
    {IMA_INIT},                        /* ima                             */ \
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,                                  \
                                       /* qmListMutex                     */ \
    NULL,                              /* pQMList                         */ \
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,                                  \
                                       /* ruleListMutex                   */ \
    NULL,                              /* pRuleList                       */ \
    MQC_RULE_LIMIT,                    /* ruleLimit                       */ \
    0,                                 /* ruleCount                       */ \
    MQC_DISCARD_UNDELIVERABLE,         /* discardUndeliverable            */ \
    {0,0,0,0,0,0,0,0,0,0,0,0,0},       /* objectIdentifier                */ \
    NULL,                              /* props                           */ \
    0,                                 /* disabledTime                    */ \
    PTHREAD_COND_INITIALIZER,          /* reconnectCond                   */ \
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,                                  \
                                       /* reconnectMutex                  */ \
    PTHREAD_COND_INITIALIZER,          /* immediateCond                   */ \
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,                                  \
                                       /* immediateMutex                  */ \
    0,                                 /* immediateRetries                */ \
    0,                                 /* shortRetries                    */ \
    0,                                 /* longRetries                     */ \
    MQC_IMMEDIATE_RETRY_COUNT,         /* immediateRetryCount             */ \
    MQC_SHORT_RETRY_INTERVAL,          /* shortRetryInterval              */ \
    MQC_SHORT_RETRY_COUNT,             /* shortRetryCount                 */ \
    MQC_LONG_RETRY_INTERVAL,           /* longRetryInterval               */ \
    MQC_LONG_RETRY_COUNT,              /* longRetryCount                  */ \
    MQC_BATCH_SIZE,                    /* batchSize                       */ \
    MQC_BATCH_DATA_LIMIT,              /* batchDataLimit                  */ \
    MQC_LONG_BATCH_TIMEOUT,            /* longBatchTimeout                */ \
    MQC_SHORT_BATCH_TIMEOUT,           /* shortBatchTimeout               */ \
    MQC_DEFAULT_CONNECTION_COUNT,      /* defaultConnectionCount          */ \
    MQC_MQ_TRACE_MAX_FILE_SIZE,        /* MQTraceMaxFileSize              */ \
    MQC_MQ_TRACE_USER_DATA_SIZE,       /* MQTraceUserDataSize             */ \
    MQC_NO_OVERRIDE,                   /* overrideNoLocalAtMessageSight   */ \
    false,                             /* traceAdmin                      */ \
    MQC_NORMAL_TRACE,                  /* traceAdminLevel                 */ \
    MQC_ENABLE_READ_AHEAD,             /* enableReadAhead                 */ \
    0,                                 /* lastQMIndex                     */ \
    0,                                 /* lastRuleIndex                   */ \
    {{0,0,0,0},0},                     /* lastQMIndexRecord               */ \
    {{0,0,0,0},0},                     /* lastRuleIndexRecord             */ \
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,                                  \
                                       /* coldStartMutex                  */ \
    PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,                                  \
                                       /* adminCallbackMutex              */ \
    0                                  /* currentAllocatedMemory          */

#ifdef MQC_GLOBAL_INIT
mqcMQC_t mqcMQConnectivity = { MQC_INIT };
#else
extern mqcMQC_t mqcMQConnectivity;
#endif


#define mqcRoundUp4(val) (((val)+3)&~3L)
/* Function prototypes */

/* admin.c */
int mqc_loadIMA(mqcIMA_t * pIMA);
int mqc_loadTuning(void);
int mqc_loadQMs(mqcQM_t ** ppQMList);
int mqc_loadRules(mqcRule_t ** ppRuleList,
                  mqcQM_t * pQMList);
int mqc_loadRecords(void);
void mqc_printIMA(void);
void mqc_printQMs(void);
void mqc_printRules(void);

int mqc_adminCallback(char * object, char * name,
                      ism_prop_t * props, ism_ConfigChangeType_t type);

int mqc_serverCallback(char * object, char * name,
                       ism_prop_t * props, ism_ConfigChangeType_t type);

int mqc_testQueueManagerConnection(const char *pQMName,
                                   const char *pConnectionName,
                                   const char *pChannelName,
                                   const char *pSSLCipherSpec,
                                   const char *pUserName,
                                   const char *pUserPassword);

/* control.c */
int mqc_start(int dynamicTraceLevel);
int mqc_term(bool requested);

int mqc_startRule(mqcRule_t * pRule);
int mqc_termRule(mqcRule_t * pRule, bool requested);

int mqc_startRuleIndex(int ruleindex);
int mqc_termRuleIndex(int ruleindex);

int mqc_startRuleQM(mqcRuleQM_t * pRuleQM);
int mqc_termRuleQM(mqcRuleQM_t * pRuleQM, bool requested);

/* mqapi.c */
int mqc_connectPublishQM(mqcRuleQM_t * pRuleQM);
int mqc_connectSubscribeQM(mqcRuleQM_t * pRuleQM);

int mqc_disconnectPublishQM(mqcRuleQM_t * pRuleQM);
int mqc_disconnectSubscribeQM(mqcRuleQM_t * pRuleQM);

int mqc_subscribeQM(mqcRuleQM_t * pRuleQM);
int mqc_unsubscribeQM(mqcRuleQM_t * pRuleQM, bool requested);

int mqc_openProducerQM(mqcRuleQM_t * pRuleQM);
int mqc_closeProducerQM(mqcRuleQM_t * pRuleQM);

int mqc_processMQMessage(mqcRuleQM_t * pRuleQM,
                         PMQMD pMD, MQLONG messlen, PMQVOID buffer);

int mqc_openSyncQueueExclusive(mqcRuleQM_t * pRuleQM, MQHCONN hConn);
int mqc_closeAndDeleteSyncQueue(mqcRuleQM_t * pRuleQM, MQHCONN hConn);

int mqc_quiesceConsumeQM(mqcRuleQM_t * pRuleQM);

int mqc_cancelWaiterQM(mqcRuleQM_t * pRuleQM);

int mqc_createMessageHandleQM(mqcRuleQM_t * pRuleQM, MQHCONN hConn);
int mqc_deleteMessageHandleQM(mqcRuleQM_t * pRuleQM, MQHCONN hConn, bool report);

int mqc_putMessageQM(mqcRuleQM_t * pRuleQM,
                     ismc_message_t * pMessage);

int mqc_MQSTAT(mqcRuleQM_t * pRuleQM);

/* mqfix.c */
int mqc_SPINotify(mqcRuleQM_t * pRuleQM,
                  MQHCONN  hConn,
                  MQHOBJ   hObj,
                  MQBYTE24 ConnectionId,
                  PMQLONG  pCompCode,
                  PMQLONG  pReason);

void mqc_SPISetProdId(mqcRuleQM_t *pRuleQM,
                      PMQLONG pCompCode,
                      PMQLONG pReason);

/* ismcapi.c */
int mqc_connectIMA(void);
int mqc_disconnectIMA(void);

int mqc_createProducerIMA(mqcRuleQM_t * pRuleQM);
int mqc_deleteProducerIMA(mqcRuleQM_t * pRuleQM);

int mqc_createRuleConsumerIMA(mqcRule_t * pRule);
int mqc_deleteRuleConsumerIMA(mqcRule_t * pRule, bool requested);

int mqc_createRuleQMSessionIMA(mqcRuleQM_t * pRuleQM);
int mqc_deleteRuleQMSessionIMA(mqcRuleQM_t * pRuleQM);

/* xathread.c */
void * mqc_MQXAConsumerThread(void * parm, void * context, int value);
void *mqc_IMAXAConsumerThread(void *pArgs, void *pContext, int value);
int mqc_createXID(mqcRuleQM_t *pRuleQM, ism_xid_t *pXid);
int mqc_recoverMQXIDsForQM(mqcRuleQM_t *pRuleQM);
int mqc_recoverIMAXIDsForQM(mqcRuleQM_t *pRuleQM);
int mqc_checkIMAXIDsForQM(mqcQM_t * pQM, bool rollbackXIDs);
bool mqc_localXIDEqual(ism_xid_t *pXidLeft, ism_xid_t *pXidRight);

int mqc_rollbackOrphanXIDs(void);

/* reconnect.c */
void * mqc_reconnectThread(void * parm, void * context, int value);
int mqc_ruleReconnected(mqcRule_t * pRule);
int mqc_ruleQMReconnected(mqcRuleQM_t * pRuleQM);
int mqc_reconnected(void);
int mqc_wakeReconnectThread(void);
int mqc_waitReconnectAttempt(void);

/* error.c */
int mqc_MQAPIError(mqcRuleQM_t * pRuleQM,
                   int rc, char * api, char * obj, MQLONG Reason, ...);
int mqc_XAError(mqcRuleQM_t * pRuleQM,
                char * api, int rc);

int mqc_ISMCError(char * obj);
int mqc_ISMCRuleError(mqcRule_t * pRule, char * api, char * obj);
int mqc_ISMCRuleQMError(mqcRuleQM_t * pRuleQM, char * api, char * obj);

int mqc_allocError(char * api, int len, int syserrno);
int mqc_ruleQMAllocError(mqcRuleQM_t * pRuleQM,
                         char * api, int len, int syserrno);
int mqc_systemAPIError(char * api, int sysrc, int syserrno);

int mqc_messageError(mqcRuleQM_t * pRuleQM, char * type, int len, char * buffer);

void mqc_errorListener(int rc, const char * error,
                       ismc_connection_t * connect, ismc_session_t * session, void * userdata);

int mqc_setMQTrace(int level);

/* properties.c */
int mqc_setMQProperties(mqcRuleQM_t * pRuleQM, ismc_message_t * pMessage,
                        PMQMD pMD, PMQLONG pBufferLength, PPMQVOID ppBuffer);

int mqc_setIMAProperties(mqcRuleQM_t * pRuleQM, ismc_message_t * pMessage,
                         PMQMD pMD, MQLONG BufferLength, PMQVOID pBuffer, char * topic);

/* uid.c */
int mqc_setObjectIdentifier(void);
int mqc_displayRecoveredSubscriptions(void);

int32_t mqc_readProcessMemInfo(char buffer[MQC_MEMINFO_BUFFERSIZE],
                                      int *bytesRead);
void mqc_traceProcessMemoryInfo(char *pPrefix);

void *mqc_malloc(size_t size);
void mqc_free(void *ptr);
