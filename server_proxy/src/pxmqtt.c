/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
#define TRACE_COMP Mqtt
#define ACTION_NAMES
#define MQTT_RC_STRINGS
#include <ismutil.h>
#include <selector.h>
#include <pxtransport.h>
#include <pxtcp.h>
#include <assert.h>
#include <pthread.h>
#include <tenant.h>
#include <throttle.h>
#include <openssl/rand.h>
#include <ismregex.h>
#include <pxmqtt.h>
#include <protoex.h>
#include <auth.h>
#include <pxrouting.h>
#ifndef NO_PXACT
#include <pxactivity.h>
#endif
#include <byteswap.h>
#define endian_int16(x)  bswap_16(x)
#define endian_int32(x)  bswap_32(x)
#define endian_int64(x)  bswap_64(x)

xUNUSED static int mqtt_unit_test = 0;
       int g_useMQTTpx = 1;
static int g_allowMQTTv5 = 0;
static mqtt_prop_ctx_t * g_ctx5;
int g_AAAEnabled = 0;
int g_discardMsgsEnabled = 1; //Enabled or Disabled the Discard Msgs Feature
int g_discardMsgsSoftLimit = 104857600; //Default SoftLimit: 100M
int g_discardMsgsHardLimit = 1073741824; //Default HardLimit: 1G
int g_discardMsgsLogIntervalInSec = 3600; //Default: 60 minutes
int g_sendGWPingREQ=1;


enum willPolicy g_willValidation = WillTopicPolicy_RemoveNotValid;

#ifndef NO_PROXY
static int g_noProxyLog = 0;
static int g_useMux     = 0;
extern int                 g_metering_interval;
extern uint64_t            g_metering_delta;
static int g_gatewayRegister = 0;
uint64_t g_gwCleanupTime = 0;
static int gwMaxActiveDevices = 500;
static int gwMaxPendingAuthRequests = 10;

static uint32_t g_maxSessionExpire = 0;

static int g_ExpectedDevMsgRate = EXPECTEDMSGRATE_LOW;
static int g_ExpectedAppMsgRate = EXPECTEDMSGRATE_DEFAULT;
static int g_ExpectedShAppMsgRate = EXPECTEDMSGRATE_HIGH;
static int g_ExpectedGWMsgRate = EXPECTEDMSGRATE_DEFAULT;
static int g_MaxTopicAliasIn = 8;
static int g_MaxTopicAliasOut = 8;
static int g_canRegister = 0;
static int g_maxKeepAlive = 0;        /* Max keepalive for other than A client class */
static int g_maxKeepAliveA = 3600;    /* Max keepalive default for A client class */
static int g_verifyReasonCode = 0;
static int g_verifyPayload = 1;
#endif

/* If in debug, always set DEBUG_INPROGRESS */
#ifndef DEBUG_INPROGRESS
#ifdef DEBUG
#define DEBUG_INPROGRESS 1
#endif
#endif

/*
 * Forward references
 */
extern int ism_log_removeConditionallyLoggedEntries(const char * clientID, const char * sourceIP);
int ism_transport_frameMqtt(ism_transport_t * transport, char * buffer, int pos, int avail, int * used);
int ism_transport_addMqttFrame(ism_transport_t * transport, char * buf, int len, int command);
int ism_transport_connectStream(ism_transport_t * transport, ism_transport_t * ctransport, ism_tenant_t * tenant);
ism_transport_t * ism_mux_createVirtualConnection(ism_server_t * server, int tid, int * pRC, char * errMsg);
int ism_transport_closeWS(ism_transport_t * transport, int rc);

#ifndef NO_PROXY
static int mpropCheck(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value);
static int sendACLs(ism_transport_t * transport);
int ism_proxy_setSessionExpire(int time);
static void removeWill(ism_transport_t * transport);
extern int ism_proxy_setMsgSizeStats(px_msgsize_stats_t * msgSizeStat, int size, int originated);
extern int ism_proxy_routeToKafka(ism_tenant_t * tenant, int qos);
extern int ism_route_routeMessage(ism_transport_t * transport, const char * buf, int buf_len, int kind, int *sendMQTT);
extern int ism_route_mhubMessage(ism_transport_t * transport, mqtt_pmsg_t * pmsg);
int ism_proxy_parseMQTTTopic(char * topic, const char * * devtype, const char * * devid, const char * * event, const char * * fmt);
static int routeMhubMessage(ism_transport_t * transport, int kind, const char * buf, int buflen);
static int matchTopicRule(ism_transport_t * transport, ism_pxrule_t * rule, const char * topic, int topic_len);
static int mpropCopyUserProps(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value);
static int sendGWPINGREQ(ism_transport_t * transport);
#endif

int mapToIsmRC(int mqttrc, int version);
extern int g_msgRoutingDefaultSendMQTT;

/*IM Event Direct Publishing*/
extern int g_useKafkaIMMessaging;
extern void ism_proxy_sqs_send_message(ism_transport_t * transport, const char * message, int message_len);
tenant_fairuse_t * ism_proxy_getFairUse(ism_transport_t * transport);

extern int sendSize;

/*
 * Default all trace to be based on the transport trace level.
 * This requires that there be a transport variable for all TRACE or to use TRACEL
 */
#undef  TRACE_DOMAIN
#define TRACE_DOMAIN  transport->trclevel

/*
 * MQTT extended fields (CSD02).
 * This must match the table in mqtt.c
 */
static mqtt_prop_field_t mqtt_propFields [] = {
    { MPI_PayloadFormat,     MPT_Int1,     5, CPOI_PUBLISH | CPOI_WILLPROP,       "PayloadFormat"    },
    { MPI_MsgExpire,         MPT_Int4,     5, CPOI_PUBLISH | CPOI_WILLPROP,       "MessageExpire"    },
    { MPI_ContentType,       MPT_String,   5, CPOI_PUBLISH | CPOI_WILLPROP,       "ContentType"      },
    { MPI_ReplyTopic,        MPT_String,   5, CPOI_PUBLISH | CPOI_WILLPROP,       "ResponseTopic"    },
    { MPI_Correlation,       MPT_Bytes,    5, CPOI_PUBLISH | CPOI_WILLPROP,       "CorrelationData"  },
    { MPI_SubID,             MPT_VarInt,   5, CPOI_S_PUBLISH | CPOI_SUBSCRIBE | CPOI_ALT_MULTI,    "SubscriptionID"   },
    { MPI_SessionExpire,     MPT_Int4,     5, CPOI_CONNECT | CPOI_CONNACK | CPOI_C_DISCONNECT,   "SessionExpire"    },
    { MPI_ClientID,          MPT_String,   5, CPOI_CONNACK,                       "AssignedClientID" },
    { MPI_KeepAlive,         MPT_Int2,     5, CPOI_CONNACK,                       "ServerKeepAlive"  },
    { MPI_AuthMethod,        MPT_String,   5, CPOI_AUTH_CONN_ACK,                 "AuthenticationMethod" },
    { MPI_AuthData,          MPT_Bytes,    5, CPOI_AUTH_CONN_ACK,                 "AuthenticationData"   },
    { MPI_RequestReason,     MPT_Int1,     5, CPOI_CONNECT,                       "RequestReason"    },
    { MPI_WillDelay,         MPT_Int4,     5, CPOI_WILLPROP,                      "WillDelay"        },
    { MPI_RequestReplyInfo,  MPT_Int1,     5, CPOI_CONNECT,                       "RequestReplyInfo" },
    { MPI_ReplyInfo,         MPT_String,   5, CPOI_CONNACK,                       "ReplyInfo"        },
    { MPI_ServerReference,   MPT_String,   5, CPOI_CONNACK | CPOI_S_DISCONNECT,   "ServerReference"  },
    { MPI_Reason,            MPT_String,   5, CPOI_ACK,                           "Reason"           },
    { MPI_MaxReceive,        MPT_Int2,     5, CPOI_CONNECT_CONNACK,               "ReceiveMaximum"   },
    { MPI_MaxTopicAlias,     MPT_Int2,     5, CPOI_CONNECT_CONNACK,               "TopicAliasMaximum"},
    { MPI_TopicAlias,        MPT_Int2,     5, CPOI_PUBLISH,                       "TopicAlias"       },
    { MPI_MaxQoS,            MPT_Int1,     5, CPOI_CONNECT_CONNACK,               "MaximumQoS"       },
    { MPI_RetainAvailable,   MPT_Boolean,  5, CPOI_CONNACK,                       "RetainAvailable"  },
    { MPI_UserProperty,      MPT_NamePair, 5, CPOI_USERPROPS | CPOI_MULTI,        "UserProperty"     },
    { MPI_MaxPacketSize,     MPT_Int4,     5, CPOI_CONNECT_CONNACK,               "MaxPacketSize"    },
    { MPI_WildcardAvailable, MPT_Int1,     5, CPOI_CONNACK,                       "WildcardAvailable"},
    { MPI_SubIDAvailable,    MPT_Int1,     5, CPOI_CONNACK,                       "SubIDAvailable"   },
    { MPI_SharedAvailable,   MPT_Int1,     5, CPOI_CONNACK,                       "SharedAvailable"  },
    { 0, 0, 0, 0, NULL },
};

/*
 * Return the allowed control packets for a return code
 */
int ism_mqtt_reasonCodeAllowed(int rc) {
    int allow;
    switch (rc) {
    case MQTTRC_OK                 :   /* 0x00   0 */
        allow = CPOI_CONNACK | CPOI_PUBACK | CPOI_PUBCOMP | CPOI_SUBACK | CPOI_UNSUBACK | CPOI_AUTH | CPOI_DISCONNECT | CPOI_AUTH;  break;
    case MQTTRC_QoS1               :   /* 0x01   1 QoS=1 return on SUBSCRIBE */
    case MQTTRC_QoS2               :   /* 0x02   2 QoS=2 return on SUBSCRIBE */
        allow = CPOI_SUBACK;
    case MQTTRC_KeepWill           :   /* 0x04   4 Normal close of connection but keep the will */
        allow = CPOI_DISCONNECT;                       break;
    case MQTTRC_NoSubscription     :   /* 0x10  16 There was no matching subscription on a PUBLISH */
        allow = CPOI_PUBACK | CPOI_PUBREC;            break;
    case MQTTRC_NoSubExisted       :   /* 0x11  17 No subscription existed on UNSUBSCRIBE */
        allow = CPOI_UNSUBACK;                         break;
    case MQTTRC_ContinueAuth       :   /* 0x18  24 Continue authentication */
    case MQTTRC_Reauthenticate     :   /* 0x19  25 Start a re-authentication */
        allow = CPOI_AUTH;                             break;
    case MQTTRC_UnspecifiedError   :   /* 0x80 128 Unspecified error */
        allow = CPOI_CONNACK | CPOI_PUBACK | CPOI_PUBCOMP | CPOI_UNSUBACK | CPOI_AUTH;  break;
    case MQTTRC_MalformedPacket    :   /* 0x81 129 The packet is malformed */
    case MQTTRC_ProtocolError      :   /* 0x82 130 Protocol error */
        allow = CPOI_CONNACK | CPOI_DISCONNECT;        break;
    case MQTTRC_ImplError          :   /* 0x83 131 Implementation specific error */
        allow = CPOI_CONNACK | CPOI_PUBACK | CPOI_PUBCOMP | CPOI_UNSUBACK | CPOI_AUTH;  break;
    case MQTTRC_UnknownVersion     :   /* 0x84 132 Unknown MQTT version */
    case MQTTRC_IdentifierNotValid :   /* 0x85 133 ClientID not valid */
    case MQTTRC_BadUserPassword    :   /* 0x86 134 Username or Password not valid */
        allow = CPOI_CONNACK;                          break;
    case MQTTRC_NotAuthorized      :   /* 0x87 135 Not authorized */
        allow = CPOI_CONNACK | CPOI_PUBACK | CPOI_SUBACK | CPOI_UNSUBACK | CPOI_DISCONNECT;   break;
    case MQTTRC_ServerUnavailable  :   /* 0x88 136 The server in not available */
        allow = CPOI_CONNACK;                          break;
    case MQTTRC_ServerBusy         :   /* 0x89 137 The server is busy */
        allow = CPOI_CONNACK | CPOI_DISCONNECT;        break;
    case MQTTRC_ServerBanned       :   /* 0x8A 138 The server is banned from connecting */
        allow = CPOI_CONNACK;                          break;
    case MQTTRC_ServerShutdown     :   /* 0x8B 139 The server is being shut down */
        allow = CPOI_DISCONNECT;                       break;
    case MQTTRC_BadAuthMethod      :   /* 0x8C 140 The authentication method is not valid */
        allow = CPOI_CONNACK | CPOI_DISCONNECT;        break;
    case MQTTRC_KeepAliveTimeout   :   /* 0x8D 141 The Keep Alive time has been exceeded */
    case MQTTRC_SessionTakenOver   :   /* 0x8E 142The ClientID has been taken over */
        allow = CPOI_DISCONNECT;                       break;
    case MQTTRC_TopicFilterInvalid :   /* 0x8F 143 The topic filter is not valid for this server */
        allow = CPOI_SUBACK | CPOI_UNSUBACK | CPOI_DISCONNECT;   break;
    case MQTTRC_TopicInvalid       :   /* 0x90 144 The topic name is not valid for this server */
        allow = CPOI_CONNACK | CPOI_PUBACK | CPOI_DISCONNECT;    break;
    case MQTTRC_PacketIDInUse      :   /* 0x91 145 The PacketID is in use */
        allow = CPOI_PUBACK | CPOI_SUBACK | CPOI_UNSUBACK;   break;
    case MQTTRC_PacketIDNotFound   :   /* 0x92 146 The PacketID is not found */
        allow = CPOI_PUBCOMP;                          break;
    case MQTTRC_ReceiveMaxExceeded :   /* 0x93 147 The Receive Maximum value is exceeded */
    case MQTTRC_TopicAliasInvalid  :   /* 0x94 148 The topic alias is not valid */
        allow = CPOI_DISCONNECT;                       break;
    case MQTTRC_PacketTooLarge     :   /* 0x95 149 The packet is too large */
        allow = CPOI_CONNACK | CPOI_DISCONNECT;        break;
    case MQTTRC_MsgRateTooHigh     :   /* 0x96 150 The message rate is too high */
        allow = CPOI_DISCONNECT;                       break;
    case MQTTRC_QuotaExceeded      :   /* 0x97 151 A user quota is exceeded */
        allow = CPOI_CONNACK | CPOI_PUBACK | CPOI_SUBACK | CPOI_DISCONNECT;    break;
    case MQTTRC_AdminAction        :   /* 0x98 152 The connection is closed due to an administrative action */
        allow = CPOI_DISCONNECT;                       break;
    case MQTTRC_PayloadInvalid     :   /* 0x99 153 The payload format is invalid */
        allow = CPOI_CONNACK | CPOI_PUBACK | CPOI_DISCONNECT;         break;
    case MQTTRC_RetainNotSupported :   /* 0x9A 154 Retain not supported */
    case MQTTRC_QoSNotSupported    :   /* 0x9B 155 The QoS is not supported */
    case MQTTRC_UseAnotherServer   :   /* 0x9C 156 Temporary use another server */
    case MQTTRC_ServerMoved        :   /* 0x9D 157 Permanent use another server */
        allow = CPOI_CONNACK | CPOI_DISCONNECT;        break;
    case MQTTRC_SharedNotSupported :   /* 0x9E 158 Shared subscriptions are not allowed */
        allow = CPOI_SUBACK | CPOI_DISCONNECT;         break;
    case MQTTRC_ConnectRate        :   /* 0x9F 159 Connection rate exceeded */
        allow = CPOI_CONNACK | CPOI_DISCONNECT;        break;
    case MQTTRC_MaxConnectTime     :   /* 0xA0 160 Shared subscriptions are not allowed */
        allow = CPOI_DISCONNECT;                       break;
    case MQTTRC_SubIDNotSupported  :   /* 0xA1 161 Subscription IDs are not supportd */
    case MQTTRC_WildcardNotSupported : /* 0xA2 163 Wildcard subscriptions are not supported */
        allow = CPOI_SUBACK | CPOI_DISCONNECT;         break;
    default :
        allow = 0;
    }
    return allow;
}

/*
 * String expansions for the return codes
 */
xUNUSED static const char * getMQTTErrorString(int rc, char * buf, int len ) {
    const char * ret;

    switch (rc) {
    case CRC_Accepted :         return NULL;
    case CRC_InvalidVersion :   ret = "The MQTT client version is not supported";     break;
    case CRC_BadIdentifier :    ret = "The client ID is not valid.";                  break;
    case CRC_BadUser :          ret = "The user name or password is not valid.";      break;
    case CRC_NotAuthorized :    ret = "The connection is not authorized.";            break;
    default:
        if (rc == ism_common_getLastError()) {
            ism_common_formatLastError(buf, len);
            ret = buf;
        } else {
            ret = ism_common_getErrorString(rc, buf, len);        break;
        }
    }
    if (buf != ret)
        ism_common_strlcpy(buf, ret, len);
    return buf;
}

/*
 * Fix problems with the topic for trace and log.
 * Replace invalid characters and all characters >= 0x80 with ?
 */
xUNUSED static const char * fixtopic(const char * topic, int len, char * out, int uni) {
    unsigned char * op;
    if (len > 0)
        memcpy(out, topic, len);
    out[len] = 0;
    op = (unsigned char *) out;
    while (*op) {
        if (*op < ' ' || (uni && *op >= 0x80))
            *op = '?';
        op++;
    }
    return out;
}
#define FIXTOPIC(t,l)   fixtopic((t), (l), alloca((l)+1), 0)
#define FIXUNICODE(t,l)   fixtopic((t), (l), alloca((l)+1), 1)

/*
 * Load a big endian 2 byte integer
 */
#define BIGINT16(p) (((int)(uint8_t)(p)[0]<<8) | (uint8_t)(p)[1])

/*
 * Parse the various mqtt message/action types.
 * Note that the pointers are into the original message buffer, and therefore
 * that cannot be freed until message processing is done.
 */
typedef struct mqttMsg_t {
    ism_transport_t * transport;
    uint8_t         action;         /* Current job action     */
    uint8_t         command;        /* Current MQTT command   */
    uint8_t         qos;            /* Quality of service     */
    uint8_t         retain;         /* Retain the message     */
    uint8_t         dup;            /* Dup flag               */
    uint8_t         flags;          /* Connect flag           */
    uint8_t         version;        /* API version            */
    uint8_t         msgtype;        /* Message type           */
    uint32_t        rc;             /* Return code            */
    uint32_t        msgid;          /* Message id             */
    uint32_t        keepalive;      /* Keepalive in seconds   */
    int             payload_len;
    int             topic_len;
    int             clientid_len;
    const char *    payload;        /* The message body       */
    const char *    topic;
    const char *    clientid;       /* The clientId - only on connect */
    const char *    props;
    int             prop_len;
    int             kind;
    int             username_len;
    int             password_len;
    const char *    username;       /* The username - only on connect */
    const char *    password;       /* The password - only on connect */
    int             willtopic_len;
    int             willpayload_len;
    int             willprop_len;
    int             resvi;
    const char *    willtopic;      /* The will topic - only on connect */
    const char *    willpayload;        /* The will message - only on connect */
    const char *    willprops;
    uint8_t         more_flags;     /* Additional proxy flags */
    uint8_t         msgfmt;
    uint8_t         isExpire;
    uint8_t         isMsgExpire;
    int             max_connect;    /* Max connect count */
    uint32_t        expireTTL;
    uint16_t        maxActive;      /* Max active packets */
    uint16_t        topic_alias;
    char *          topic_alias_loc;
    uint32_t        maxMsgSize;     /* Max message size */
    uint32_t        maxPacketSize;
    uint8_t         request_reply_info;
    uint8_t         request_reason;
    uint8_t         has_user_props;
    uint8_t         resv1[1];
    const char *    reason;
} mqttMsg_t;



/*
 * The MQTT protocol specific area of the proxy transport object
 */
typedef struct ism_protobj_t {
    ism_transport_t *  transport;
    volatile int       closed;            /* Connection is not is use */
    int                prot;              /* Which protocol           */
    uint8_t            clean;             /* Disconnect is clean      */
    uint8_t            cleansession;      /* Clean session indicator (0 - durable client, 1 - non-durable) */
    uint8_t            startState;        /* Start state 0=not yet, 1=in progress, 2=stolen, 3=done */
    uint8_t            mqtt_version;      /* 3=3.1, 4=3.1.1 5=5          */
    uint8_t            mqtt_bridge;       /* Use bridge protocol      */
    uint8_t            mqtt_client;       /* This connection is a client */
    uint8_t			   delay_closing;	  /* In the delay process to close */
    uint8_t            disconnect_pending;
    pthread_spinlock_t lock;
    int16_t            inprogress;        /* Count of actions in progress */
    volatile uint8_t   connectPending;
    uint8_t            mqttpx_version;
    uint64_t           messageCount;      /* Current message count */
    int32_t            morelen;           /* WebSockets extra buffer length */
    int32_t            morealloc;         /* WebSockets extra buffer allocated size */
    char *             morebuf;           /* WebSockets extra buffer pointer */
    ism_transport_t *  client_transport;
    ism_transport_t *  server_transport;
    char *             connectdata;
    char *             savedata;
    uint16_t           keepalive;
    uint8_t            send_keepalive;
    uint8_t            send_disconnect;
    uint8_t            pendingRC;
    uint8_t            requestReason;
    uint8_t            checkSessionUser;
    uint8_t            flag_pos;
    uint8_t            resvi[4];
    uint32_t           connectlen;
    uint32_t           savelen;
    uint32_t           savecount;
    uint32_t           savedataprocessing;
    uint32_t           end_ext_loc;
    uint32_t           knownDevsCount;
    uint32_t           pendingDevsCount;
    uint32_t           maxPacketSize;      /* client max packet size */
    uint16_t           topicalias_count_in;
    uint16_t           topicalias_count_out;
    const char * *     topicalias_in;
    const char * *     topicalias_out;
    uint32_t           expiryOriginal;
    uint32_t           expiryRequested;
    const char *       willTopic;    /* Original unconverted will topic */
} ism_protobj_t;

typedef ism_protobj_t mqttProtoObj_t;

#ifndef NO_PROXY
static ism_regex_t clientIDMatch;
static int         doClientIDMatch = 0;
#endif
ism_regex_t devIDMatch;
int         doDevIDMatch = 0;
ism_regex_t devTypeMatch;
int         doDevTypeMatch = 0;

/*
 * Forward references
 */
#ifndef NO_PROXY
extern HOT int ism_mqtt_receive(ism_transport_t * transport, char * buf, int buflen, int kind);
void ism_mqtt_doneConnection(ism_transport_t * transport);
const char * ism_mqtt_mqttCommand(int ix);
static void parseConnectPayload(mqttMsg_t * mmsg);
static void doConnect(ism_transport_t * transport, mqttMsg_t * mmsg, concat_alloc_t * buf);
int  ism_mqtt_doMoreConnect(ism_transport_t * transport, void * param1, uint64_t param2);
int ism_mqtt_connection(ism_transport_t * transport);

static int convertTopic(ism_transport_t * transport, concat_alloc_t * buf,
        const char * topic, int topic_len, int subscribe);
static int convertTopicOut(ism_transport_t * transport, concat_alloc_t * buf,
        const char * topic, int topic_len);
static void mqttConnectError(mqttMsg_t * msg);
ism_topicrule_t * ism_proxy_getTopicRuleClass(ism_tenant_t * tenant, uint8_t client_class);
static int matchClientClass(ism_transport_t * transport, ism_pxrule_t * rule);
ism_clientclass_t * ism_proxy_getClientClass(const char * name);
const char * ism_proxy_showRule(ism_pxrule_t * pxrule, char * buf, int len);
void ism_proxy_makeMonitorTopic(concat_alloc_t * buf, ism_transport_t * transport, const char * monitor_topic);
static int checkPublishAuth(ism_transport_t * transport, concat_alloc_t * buf, int kind, int rcCheckACL, int sendSavedData);
static int checkSubscribeAuth(ism_transport_t * transport, concat_alloc_t * buf, int kind, int subcnt, int sendSavedData);
ism_transport_t * ism_transport_newStream(ism_transport_t * transport);
void ism_transport_muxInit(int usemux);
static int parseTopicAuth(char * topic, const char * * devtype, const char * * devid);
int ism_proxy_getAuthMask(const char * name);
#endif
static int mapToMqttRc(int rc, int version, int cp);

#ifndef NO_PROXY
static int revalidateAndSendSavedData(ism_transport_t * transport, char * saveddata, int saveddatalen, int savecount);
#endif


extern  int  g_bigLog;

/*
 * Protocol states
 */
#define MQTT_NOT_CONNECTED  0
#define MQTT_IN_PROGRESS    1
#define MQTT_STOLEN         2
#define MQTT_CONNECTED      3

/*
 * The MQTT protocol handles two methods of framing MQTT:
 * - A native tcp binary frame
 * - A WebSockets frame
 *
 * The processing done is the same regardless of the frame
 *
 * There are several version of the MQTT spec supported by this code specified in the version byte of the CONNECT.
 * 3 = v3.1   - The Paho level of the specification
 * 4 = v3.1.1 - The OASIS standardization of the specification
 * 5 = v5     - The second OASIS standard version
 *
 * There are only minor differences between these versions, but v3.1.1 is a lot more strict about requirements
 * to check reserved bits.  It also removes a couple of limits imposed by v3.1.  The check for v3.1.1 is
 * normally version > 3.  The version is kept in both mmsg->version and pobj->mqtt_version as not all methods
 * have both.  When both exist they should be the same.
 *
 * The design of ISM allows work to be done as scheduled jobs or inline.
 * The result is that many calls to other components including the engine
 * do not return a result but call a callback function when they are
 * complete, which might not be in the same thread.
 */
#define PROT_MQTT_BIN    1  /* Binary with native framing         */
#define PROT_MQTT_WSBIN  2  /* Binary with WebSockets framing     */

#ifndef NO_PROXY
static px_mqtt_stats_t mqttStats = {0};

static px_msgsize_stats_t mqttMsgSizeStats = {0};
#endif

/*
 * Put one character to a concat buf
 */
xUNUSED static void bputchar(concat_alloc_t * buf, char ch) {
    if (buf->used + 1 < buf->len) {
        buf->buf[buf->used++] = ch;
    } else {
        char chx = ch;
        ism_common_allocBufferCopyLen(buf, &chx, 1);
    }
}

#ifndef NO_PROXY
/*
 * Send a PUBACK
 */
static inline void sendPubAck(ism_transport_t * transport, uint8_t qos, char id[2]) {
    char buf[32];
    int action = (qos == 1) ? MT_PUBACK : MT_PUBREC;
    buf[16] = id[0];
    buf[17] = id[1];
    transport->send(transport, buf+16, 2, action << 4, SFLAG_FRAMESPACE);
}


/*
 * Send a SUBACK directly in case of a bad request
 */
static void sendSubAck(ism_transport_t * transport, int count, char id[2], uint8_t * subrcp) {
    char xbuf[256];
    int  i;
    concat_alloc_t buf = {xbuf, sizeof(xbuf), 16};
    char value = (transport->pobj->mqtt_version == 3) ? 0 : 0x80;
    buf.buf[buf.used++] = id[0];
    buf.buf[buf.used++] = id[1];
    if (transport->pobj->mqtt_version >= 5)
        buf.buf[buf.used++] = 0;
    for (i=0; i<count; i++) {
        bputchar(&buf, subrcp ? subrcp[i] : value);
    }
    transport->send(transport, buf.buf+16, buf.used-16, MT_SUBACK << 4, SFLAG_FRAMESPACE);
    ism_common_freeAllocBuffer(&buf);
}

/*
 * Send a UNSUBACK directly in case of a bad request
 */
static void sendUnsubAck(ism_transport_t * transport, int count, char id[2], uint8_t * subrcp) {
    char xbuf[256];
    int  i;
    concat_alloc_t buf = {xbuf, sizeof(xbuf), 16};
    char value = (transport->pobj->mqtt_version == 3) ? 0 : 0x80;
    buf.buf[buf.used++] = id[0];
    buf.buf[buf.used++] = id[1];
    if (transport->pobj->mqtt_version >= 5) {
        buf.buf[buf.used++] = 0;
        for (i=0; i<count; i++) {
            bputchar(&buf, subrcp ? subrcp[i] : value);
        }
    }
    transport->send(transport, buf.buf+16, buf.used-16, MT_UNSUBACK << 4, SFLAG_FRAMESPACE);
    ism_common_freeAllocBuffer(&buf);
}


/*
 * Send a reply for cases where we are not sending the message to the server
 */
static void sendDirectReply(ism_transport_t * transport, int kind, char * buf, int buflen, int subcnt ) {
    uint8_t command = (uint8_t)((kind >> 4) & 15);
    uint8_t qos = (uint8_t)((kind >> 1) & 3);
    if (qos) {
        if (command == MT_PUBLISH) {
            int topiclen = (uint16_t)BIGINT16(buf);
            char *id = buf+ 2 + topiclen;
            sendPubAck(transport, qos, id);
        } else if (command == MT_UNSUBSCRIBE) {
            sendUnsubAck(transport, subcnt, buf, NULL);
        } else {
            sendSubAck(transport, subcnt, buf, NULL);
        }
    }
}
#endif


/*
 * Return the MQTT version
 * This is used to provide access to the MQTT pobj which is only defined in this source module
 */
uint8_t ism_mqtt_getMqttVersion(ism_transport_t * transport) {
    return transport->pobj->mqtt_version;
}

/*
 * Return the MQTT frame
 * Revert behavior for MQTT v3 (to fix customer issue). So for MQTT v3 duplicate message is silently dropped
 */
uint8_t ism_mqtt_getMqttFrame(ism_transport_t * transport) {
    return transport->pobj->prot;
}


/*
 * Base62 is used for the random part of a generated clientID.
 */
xUNUSED static char base62 [62] = {
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
    'W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l',
    'm','n','o','p','q','r','s','t','u','v','w','x','y','z',
};

#ifndef NO_PROXY
/*
 * Base52 is used to remove vowels which removes almost all objectionable words
 */
static char base52 [52] = {
    '0','1','2','3','4','5','6','7','8','9',
    'B','C','D','F','G','H','J','K','L','M','N','P','Q','R','S','T','V','W','X','Y','Z',
    'b','c','d','f','g','h','j','k','l','m','n','p','q','r','s','t','v','w','x','y','z',
};


/*
 * Generate a client ID.
 * This is only used in MQTT v3.1.1 and above
 */
static int generate_cid(ism_transport_t * transport, char * buf, int client) {
    uint64_t rval;
    uint8_t * randbuf = (uint8_t *)&rval;
    char * bp = buf + 1;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    RAND_pseudo_bytes(randbuf, 8);
#else
    RAND_bytes(randbuf, 8);
#endif
    buf[0] = '_';
    if (client) {
        if (transport->client_addr && *transport->client_addr)
            strcpy(bp, transport->client_addr);
        else
            strcpy(bp, "client");
        bp += strlen(bp);
        *bp++ = '_';
    }
    for (int i=0; i<8; i++) {
        *bp++ = base52[(int)(rval%52)];
        rval /= 52;
    }
    *bp++ = 0;
    return bp-buf;
}


/*
 * Make a clientID including a generated portion for shared subscriptions
 */
static int makeSharedSubClientID(ism_transport_t * transport, char * buf, const char * clientId, int clientId_len) {
	memcpy(buf, clientId, clientId_len);
	buf[clientId_len] = ':';
	generate_cid(transport, buf + clientId_len + 1, 0);
	return strlen(buf);
}





/*
 * Receive websockets binary
 *
 * This code handles the case where the WebSockets buffer does not align with the MQTT control packet.
 * This is required for MQTTv3.1.1, but it is not necessary to make it fast as our client always
 * aligns the two.
 */
int ism_mqtt_wsbReceive(ism_transport_t * transport, char * buf, int buflen, int command) {
    int    ret = 0;
    int    count;
    int    len;
    int    kind;
    char * bp;
    char * freebuf = NULL;

    /*
     * Check that this is a binary packet.
     * This is a V3.1.1 requirement, but we really need to test this to be conformant for WebSockets
     * in any case (as a text packet must be valid UTF-8).  Thus we impose this for all versions
     */
    if (command != 2) {
        transport->close(transport, ISMRC_NotBinary, 0, "The packet type must be binary.");
        return ISMRC_NotBinary;
    }

    /*
     * If we have a saved buffer combine the new buffer with it.
     * In general we hope this does not happen.
     */
    if (transport->pobj->morelen) {
        char * newbuf;

        if (transport->pobj->morelen + buflen > 32000) {
            freebuf = newbuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_mqtt_receive,1),transport->pobj->morelen + buflen);
        } else {
            newbuf = alloca(transport->pobj->morelen + buflen);
        }
        memcpy(newbuf, transport->pobj->morebuf, transport->pobj->morelen);
        memcpy(newbuf+transport->pobj->morelen, buf, buflen);
        buflen += transport->pobj->morelen;
        transport->pobj->morelen = 0;
        buf = newbuf;
    }
    while (buflen >= 2) {
        bp = buf;
        kind = bp[0];
        len = (uint8_t) bp[1];
        buflen -= 2;
        count = 2;
        bp += 2;
        /* Handle a multi-byte length */
        if (len & 0x80) {
            len &= 0x7f;
            int multshift = 7;
            do {
                buflen--;
                count++;
                if (count > 5) {
                    ism_common_setErrorData(ISMRC_BadLength, "%s", "MQTT fixed header");
                    transport->close(transport, ISMRC_BadLength, 0, "The data from the client is not valid");
                    if (freebuf)
                        ism_common_free(ism_memory_proxy_mqtt_receive,freebuf);
                    return -1;
                }
                if (buflen < 0) {
                    buflen += count;
                    goto savebuf;
                }
                len += (*bp++ & 0x7f) << multshift;
                multshift += 7;
            } while ((bp[-1] & 0x80));
            /* Check that this is the shorteest encoding */
            if (bp[-1] == 0) {
                ism_common_setErrorData(ISMRC_BadLength, "%s", "MQTT Remaining Length");
                transport->close(transport, ISMRC_BadLength, 0, "The data from the client is not valid");
                if (freebuf)
                    ism_common_free(ism_memory_proxy_mqtt_receive,freebuf);
                return -1;
            }
        }

        if (buflen < len) {
            buflen += count;
            break;
        }

        /* Call mqtt binary receive */
        int rrc = ism_mqtt_receive(transport, buf + count, len, kind);
        buf += count + len;
        buflen -= len;
        count = 0;
        if (rrc < 0) {
            // TODO: fix WebSockets fair use throttling
            // ret = rrc;
            // break;
        }
    }

    if (buflen > 0) {
    savebuf:
        if (buflen > transport->pobj->morealloc) {
            transport->pobj->morebuf = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_mqtt_receive,2),transport->pobj->morebuf, buflen);
            transport->pobj->morealloc = buflen;
        }
        memmove(transport->pobj->morebuf, buf, buflen);
        transport->pobj->morelen = buflen;
    }
    if (freebuf)
        ism_common_free(ism_memory_proxy_mqtt_receive,freebuf);
    return ret;
}


/*
 * Notification of the outgoing connection complete.
 * If there is a saved packet send it at this point.
 */
int ism_mqtt_connected(ism_transport_t * transport, int rc) {
    if (__sync_bool_compare_and_swap(&transport->pobj->connectPending, 1, 0)) {
        ism_throttle_setConnectReqInQ(transport->clientID, 0);
        ism_transport_t * stransport = transport->pobj->client_transport;


        if (!stransport || !stransport->pobj || stransport->pobj->server_transport != transport) {
            if (rc == 0)
                rc = ISMRC_ServerNotAvailable;
            TRACE(5, "Stale client connection: connect=%u client=%s rc=%d\n", transport->index, transport->name, rc);
            
            transport->close(transport, rc, 0, "Could not connect to server");
            return 1;
        }
        TRACE(7, "Outgoing connection complete: connect=%d ip=%s port=%u rc=%d connectlen=%d inprogress=%d client_connect=%u\n",
                transport->index, transport->server_addr, transport->serverport, rc, stransport->pobj->connectlen,
                stransport->pobj->inprogress, stransport->index);

        if (rc == 0) {
            mqttProtoObj_t * pobj = stransport->pobj;
            char * senddata = pobj->connectdata;
            int    sendlen = pobj->connectlen;
            transport->state = ISM_TRANST_Open;
            if (sendlen) {
#ifdef DEBUG
                if (SHOULD_TRACE(8)) {
                    char obuf[256];
                    sprintf(obuf, "ism_mqtt_connected:  connect=%u sendlen=%d senddata=%p",
                            transport->index, sendlen, senddata);
                    traceDataFunction(obuf, 0, __FILE__, __LINE__, senddata, sendlen, 256);
                }
#endif
                transport->send(transport, senddata, sendlen, 0, SFLAG_HASFRAME);
            }
            transport->ready = 5;   /* TCP and TLS connection, but no MQTT connection */
        }


        return 0;
    }
    return 1;
}

extern int ism_transport_addMqttFrame(ism_transport_t * transport, char * buf, int len, int command);

/*
 * Append new data to the saved data
 */
static void appendSavedData(ism_transport_t * transport, char * buf, int length, int kind) {
    int flen = ism_transport_addMqttFrame(transport, buf, length, kind|0x100);
    buf -= flen;
    length += flen;
    mqttProtoObj_t * pobj = transport->pobj;

    if (pobj->connectlen == 0) {
        pobj->connectdata = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_mqtt_connection,1),length);
        pobj->connectlen = length;
        if (pobj->end_ext_loc) {
            pobj->end_ext_loc += flen;
        }
        pobj->flag_pos += flen;
        memcpy(pobj->connectdata, buf, length);
#ifdef DEBUG
        if (SHOULD_TRACE(9)) {
            char obuf[256];
            sprintf(obuf, "appendSavedData:  connect=%u connectlen=%d connectdata=%p command=%d",
                    transport->index, pobj->connectlen, pobj->connectdata, kind & 0xff);
            traceDataFunction(obuf, 0, __FILE__, __LINE__, pobj->connectdata, pobj->connectlen, 256);
        }
#endif
    } else {
        int newLen = pobj->savelen + length;
        pobj->savedata = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_mqtt_savedata,1),pobj->savedata, newLen);
        memcpy(pobj->savedata+pobj->savelen, buf, length);
        pobj->savelen = newLen;
        pobj->savecount++;
#ifdef DEBUG
        if (SHOULD_TRACE(9)) {
            char obuf[256];
            sprintf(obuf, "appendSavedData:  connect=%u savelen=%d savedata=%p command=%d length=%d",
                    transport->index, pobj->savelen, pobj->savedata, kind & 0xff, length);
            traceDataFunction(obuf, 0, __FILE__, __LINE__, pobj->savedata, pobj->savelen, 256);
        }
#endif
    }
}


/*
 * Check for an ACL from a converted (full) topic name
 */
static int checkACL(ism_transport_t * transport, const char * mtopic) {
    int    rc;
    int    aclfound = 0;
    const char * devtype;
    const char * devid;
    int    topiclen = (uint16_t) BIGINT16(mtopic);
    char * topic = alloca(topiclen+1);
    const char * aclKey;

    memcpy(topic, mtopic+2, topiclen);
    topic[topiclen] = 0;
    rc = parseTopicAuth(topic, &devtype, &devid);
    if (rc==0 && devtype && devid && *devtype != '+' && *devid != '+') {
        int dtlen = strlen(devtype);
        int idlen = strlen(devid);
        char * key = alloca(dtlen+idlen+2);
        memcpy(key, devtype, dtlen);
        key[dtlen] = '/';
        memcpy(key+dtlen+1, devid, idlen+1);
        aclKey = ism_proxy_getACLKey(transport);
        aclfound = ism_protocol_checkACL(key, aclKey, NULL);
    }
    if (aclfound == 1) {
        return ISMRC_NotAuthorized;
    } else {
        return 0;
    }
}

/*
 * Replace a topic alias
 */
static void replaceTopicAlias(const char * * alias, int which, const char * topic, int topic_len) {
    char * newalias;
    if (alias[which]) {
        if (strlen(alias[which]) == topic_len && !memcmp(alias[which], topic, topic_len))
            return;
        ism_common_free(ism_memory_proxy_mqtt,(char *)alias[which]);
        alias[which] = NULL;
    }
    newalias = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_mqtt,1),topic_len + 1);
    memcpy(newalias, topic, topic_len);
    newalias[topic_len] = 0;
    alias[which] = newalias;
}


/*
 * Parse a publish packet and validate any properties
 */
static int parsePublish(ism_transport_t * transport, mqttMsg_t * mmsg, uint8_t * bp, int buflen, int server) {
    mqttProtoObj_t * pobj = transport->pobj;
    int topic_len = -1;

    mmsg->qos = (uint8_t)((mmsg->kind >> 1)  & 3);
    /* Check max QoS */
    if (mmsg->qos > 2) {
        ism_common_setErrorData(ISMRC_InvalidQoS, "%s", "PUBLISH");
        return ISMRC_InvalidQoS;
    }
    if (buflen > 1)
        topic_len = BIGINT16(bp);
    buflen -= 2;
    if ((topic_len < 0) || (buflen < topic_len)) {
        ism_common_setErrorData(ISMRC_BadLength, "%s", "PUBLISH");
        return ISMRC_BadLength;
    }

    bp += 2;
    mmsg->topic = (char *) bp;
    mmsg->topic_len = topic_len;
    bp += topic_len;
    buflen -= topic_len;

    /* Get the message ID */
    if (mmsg->qos > 0) {
        if (buflen > 1) {
            mmsg->msgid = (uint16_t)BIGINT16(bp);
            bp += 2;
            buflen -= 2;
        } else {
            ism_common_setErrorData(ISMRC_BadLength, "%s", "PUBLISH");
            return ISMRC_BadLength;
        }
    }

    /* Check for properties for version 5 and above */
    if (buflen > 0 && pobj->mqtt_version >= 5) {
        if (*bp != 0) {
            int vilen;
            mmsg->prop_len = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
            bp += vilen;
            buflen -= vilen;
            mmsg->props = (char *)bp;
            if (mmsg->prop_len < 0 || mmsg->prop_len > buflen) {
                ism_common_setErrorData(ISMRC_BadLength, "%s%s", "PUBLISH", "MQTTPropLen");
                return ISMRC_BadLength;
            } else {
                /* Check the properties.  If it is good we will just copy it as we got it */
                int xrc = ism_common_checkMqttPropFields((char *)bp, mmsg->prop_len,
                    g_ctx5, server ? CPOI_S_PUBLISH : CPOI_C_PUBLISH, mpropCheck, mmsg);
                if (xrc)
                    return xrc;   /* setError must already be done */
            }
            bp += mmsg->prop_len;
            buflen -= mmsg->prop_len;
        } else {
            bp++;
            buflen--;
        }
    }
    mmsg->payload = (char *) bp;
    mmsg->payload_len = buflen;
    return 0;
}


/*
 * Direct publish of a NAK rather than getting the NAK from the server
 */
static void publishNAK(mqttMsg_t * mmsg) {
    ism_transport_t * transport = mmsg->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    int sendnak = 0;
    int rc = mmsg->rc;

    switch (pobj->mqtt_version) {
    case 3:             /* MQTT v3.1  */
    case 4:             /* Leave the RC set bad causing a connection close */
        sendnak = 0;
        break;
    case 5:             /* Send the nak with rc and continue for QoS>0 */
    default:
        if (mmsg->qos > 0) {
            sendnak = 1;
            mmsg->rc = 0;
        }
    }

    if (sendnak) {
        int cmd = mmsg->qos == 2 ? (MT_PUBREC << 4) : (MT_PUBACK << 4);
        char xbuf[2092];
        char mbuf[2048];
        concat_alloc_t rbuf = {xbuf, sizeof xbuf};
        ism_common_formatLastError(mbuf, sizeof mbuf);
        xbuf[16] = (char) (mmsg->msgid >> 8);
        xbuf[17] = (char) mmsg->msgid;
        rbuf.used = 18;
        if (pobj->mqtt_version >= 5) {
            xbuf[rbuf.used++] = mapToMqttRc(rc, pobj->mqtt_version, CPOI_PUBACK);
            if (mmsg->rc && *mbuf && pobj->requestReason) {
                int reasonlen = strlen(mbuf);
                if (reasonlen && (!transport->pobj->maxPacketSize || (reasonlen+11 < transport->pobj->maxPacketSize))) {
                    ism_common_putMqttVarInt(&rbuf, reasonlen+3);
                    ism_common_putMqttPropString(&rbuf, MPI_Reason, g_ctx5, mbuf, reasonlen);
                } else {
                    bputchar(&rbuf, 0);         /* proplen = 0 */
                }
            } else {
                bputchar(&rbuf, 0);
            }
            TRACE(7, "Send %s: connect=%u rc=%d reason=%s\n", mmsg->qos==2 ? "PUBREL" : "PUBACK",
                    transport->index, (uint8_t)xbuf[18], mbuf);
        }
        transport->send(transport, rbuf.buf + 16, rbuf.used - 16, cmd, SFLAG_SYNC | SFLAG_FRAMESPACE);
        if (rbuf.inheap)
               ism_common_freeAllocBuffer(&rbuf);
    }
}

/*
 * Do an incoming (from client) publish
 */
static int doPublishC2P(ism_transport_t * transport, mqttMsg_t * mmsg, uint8_t * bp, int buflen, concat_alloc_t * buf) {
	mqttProtoObj_t * pobj = transport->pobj;
	int  rc;

	rc = parsePublish(transport, mmsg, bp, buflen, 0);
	if (rc)
        return rc;

    /* Check max message length for this endpoint, server, or organization */
    int maxMsgSize = transport->maxMsgSize;     /* Start with endpoint max message size */
    if (maxMsgSize) {
        if (pobj->mqtt_version >= 5) {
            /* Already checked */
        } else {
            int chklen = mmsg->payload_len;
            if (mmsg->topic_len > 200)                              /* Allow up to 200 bytes of overhead */
                chklen += mmsg->topic_len - 200;
            if (chklen > maxMsgSize) {
                ism_common_setErrorData(ISMRC_MsgTooBig, "%u%u", chklen, maxMsgSize);
                return ISMRC_MsgTooBig;
            }
        }
    }

    /* Implement topic alias */
    if (mmsg->topic_alias) {
        if (mmsg->topic_alias > pobj->topicalias_count_in) {
            ism_common_setError(ISMRC_TopicAliasNotValid);
            return ISMRC_TopicAliasNotValid;
        }
        /* Allocate the topic alias table on first use */
        if (!transport->pobj->topicalias_in) {
            transport->pobj->topicalias_in = (const char * *)
                    ism_transport_allocBytes(transport, pobj->topicalias_count_in * sizeof(char *), 1);
            memset(transport->pobj->topicalias_in, 0, pobj->topicalias_count_in * sizeof(char *));
        }
        /* Use the topic alias */
        if (mmsg->topic_len == 0) {
            if (pobj->topicalias_in[mmsg->topic_alias-1]) {
                mmsg->topic = (char *)pobj->topicalias_in[mmsg->topic_alias-1];
                mmsg->topic_len = strlen(mmsg->topic);
            }
        } else {
            replaceTopicAlias(pobj->topicalias_in, mmsg->topic_alias-1, mmsg->topic, mmsg->topic_len);
        }
    }

    /* Make sure we have a topic */
    if (mmsg->topic_len == 0) {
        ism_common_setErrorData(ISMRC_BadLength, "%s", "PUBLISH");
        return ISMRC_BadLength;
    }

    /* Ignore retain if it is not allowed */
    if ((mmsg->kind&1) && (transport->tenant->allow_retain != 1)) {
        mmsg->kind &= ~1;       /* Turn off retain */
    }

    /*
     * Check that the payload is valid UTF-8
     */
    if (g_verifyPayload && mmsg->msgfmt==1 && mmsg->payload_len) {
        int pcount = ism_common_validUTF8(mmsg->payload, mmsg->payload_len);
        if (pcount < 0) {
            ism_common_setError(ISMRC_InvalidPayload);
            mmsg->rc = ISMRC_InvalidPayload;
            publishNAK(mmsg);
            return mmsg->rc ? mmsg->rc : 2;
        }
    }

    __sync_add_and_fetch(&mqttStats.mqttC2PMsgsReceived[mmsg->qos], 1);

    if (mmsg->qos > transport->tenant->max_qos) {
        ism_common_setErrorData(ISMRC_MaxQoS, "%u", mmsg->qos);
        return ISMRC_MaxQoS;
    }
    int topic_pos = buf->used;
    rc = convertTopic(transport, buf, mmsg->topic, mmsg->topic_len, 0);
    if (rc) {
        TRACE(5, "doPublishC2P: Error in convert topic on publish: connect=%u client=%s rc=%u\n", transport->index, transport->name, rc);
        mmsg->rc = rc;
        publishNAK(mmsg);
        return mmsg->rc ? mmsg->rc : 2;
    }

    /* Put out the msgid */
    if (mmsg->qos > 0) {
        bputchar(buf, (char)(mmsg->msgid>>8));
        bputchar(buf, (char)mmsg->msgid);
    }

    /* Write out properties */
    if (pobj->mqtt_version >= 5) {
        if (!mmsg->prop_len) {
            bputchar(buf, 0);       /* No metadata */
        } else {
            /*
             * Remove the topic alias.
             * All of the other metadata fields are passed from client to server.
             */
            if (mmsg->topic_alias_loc) {
                int pos = mmsg->topic_alias_loc - mmsg->props;
                assert(pos >= 0 && pos+3 <= mmsg->prop_len);
                memmove((char *)mmsg->props+pos, mmsg->props+pos+3, mmsg->prop_len-pos-3);
                mmsg->prop_len -= 3;
            }
            ism_common_putMqttVarInt(buf, mmsg->prop_len);
            ism_common_allocBufferCopyLen(buf, mmsg->props, mmsg->prop_len);
        }
    }

    /* Write out the payload */
    if (mmsg->payload_len)
    	ism_common_allocBufferCopyLen(buf, mmsg->payload, mmsg->payload_len);

    if (transport->has_acl) {
        rc = checkACL(transport, buf->buf+topic_pos);
        if (rc) {
        	TRACE(7, "doPublishC2P: check ACL failed : connect=%u client=%s rc=%u\n", transport->index, transport->name, rc);
        	ism_common_setErrorData(rc, "%s%s", "Device not in group: ", buf->buf+topic_pos);
            mmsg->rc = rc;
            /* Do not send the NAK at this point, but we must do so later as required */
            return mmsg->rc;
        }
    }

    if (mmsg->qos==1) {
        transport->endpoint->stats->count[transport->tid].qos1_read_msg++;
    } else if (mmsg->qos==2) {
        transport->endpoint->stats->count[transport->tid].qos2_read_msg++;
    }

    /* Set MsgSize Stat */
    ism_proxy_setMsgSizeStats(&mqttMsgSizeStats, mmsg->payload_len, transport->originated);

    /* Check message rate throttling */
    if (transport->useMups && transport->fairuse) {
        int furc = transport->fairuse(transport, FUR_Message, 1, mmsg->payload_len);
        if (furc == -9) {
        	if (transport->tenant->fairuse.log != 2) {
        		TRACE(7, "throttle connect=%d length=%d\n", transport->index, mmsg->password_len);
        		return furc;
        	} else {
        		TRACE(7, "throttle log only connect=%d length=%d\n", transport->index, mmsg->password_len);
        	}
        }
    }
    return ISMRC_OK;
}


/*
 * Do an incoming (from server) publish
 */
static int doPublishS2P(ism_transport_t * transport, mqttMsg_t * mmsg, uint8_t * bp, int buflen, concat_alloc_t * buf) {
    mqttProtoObj_t * pobj = transport->pobj;
    mqttProtoObj_t * cpobj = pobj->client_transport->pobj;
    int  rc;
    int  topicalias = 0;
    int  topicpos;

    rc = parsePublish(transport, mmsg, bp, buflen, 1);
    if (rc)
        return rc;

    __sync_add_and_fetch(&mqttStats.mqttP2SMsgsReceived[mmsg->qos], 1);

    topicpos = buf->used;
    convertTopicOut(transport, buf, mmsg->topic, mmsg->topic_len);
    /* Implement outgoing topic alias */
    if (cpobj->topicalias_count_out) {
        int xx;
        const char * topicstr = (const char *)buf->buf+topicpos+2;
        for (xx=0; xx < cpobj->topicalias_count_out; xx++) {
            if (cpobj->topicalias_out[xx] == NULL) {
                cpobj->topicalias_out[xx] = ism_transport_putString(pobj->client_transport, topicstr);
                topicalias = xx + 1;
                break;
            }
            if (!strcmp(cpobj->topicalias_out[xx], topicstr)) {
                topicalias = xx + 1;
                buf->buf[topicpos] = 0;
                buf->buf[topicpos+1] = 0;
                buf->used = topicpos+2;
                break;
            }
        }
    }

    /* Put out the msgid */
    if (mmsg->qos > 0) {
        bputchar(buf, (char)(mmsg->msgid>>8));
        bputchar(buf, (char)mmsg->msgid);
    }

    /* Write out properties */
    if (cpobj->mqtt_version >= 5) {
        /*Add in a topic alias.   All of the other metadata fields are passed from client to server. */
        if (topicalias > 0) {
            ism_common_putMqttVarInt(buf, mmsg->prop_len+3);
            ism_common_putMqttPropField(buf, MPI_TopicAlias, g_ctx5, topicalias);
        } else {
            ism_common_putMqttVarInt(buf, mmsg->prop_len);
        }
        /* Copy the existing properties */
        if (mmsg->prop_len)
            ism_common_allocBufferCopyLen(buf, mmsg->props, mmsg->prop_len);
    }

    /* Write out the payload */
    if (mmsg->payload_len)
        ism_common_allocBufferCopyLen(buf, mmsg->payload, mmsg->payload_len);
    transport->endpoint->stats->count[transport->tid].write_msg++;
    ism_proxy_setMsgSizeStats(&mqttMsgSizeStats, mmsg->payload_len, 1);
    return 0;
}


/*
 * Implement an incoming (from client) subscribe
 */
static int doSubscribe(ism_transport_t * transport, uint8_t * bp, int buflen, concat_alloc_t * buf, int * outcount) {
    static const char * hexdigit = "0123456789abcdef";
    int count = 0;
    int goodcount = 0;
    int xrc;
    int rc = 0;
    const char * ttopic;
    char fixbuf[1024];
    char xbuf[1024];
    int  topic_pos;
    int  doreply = 0;
    uint8_t * inbp = bp;
    int  mproplen;
    uint8_t subrc = 0;
    uint8_t * subrcp = NULL;
    int     subrc_count = 1;
    char * mpropbytes;

    *outcount = 0;
    if (buflen < 6) {
        ism_common_setErrorData(ISMRC_BadLength, "%s", "SUBSCRIBE");
        return ISMRC_BadLength;
    }

    /* Copy msgid */
    int packetid = BIGINT16(bp);
    if (packetid == 0) {
        ism_common_setErrorData(ISMRC_InvalidID, "%u%s", packetid, transport->name);
        return ISMRC_InvalidID;
    }
    memcpy(buf->buf + buf->used, bp, 2);
    buf->used += 2;
    bp += 2;
    buflen -= 2;

    /* Check for properties for version 5 and above */
    if (buflen > 0 && transport->pobj->mqtt_version >= 5) {
        if (*bp != 0) {
            int vilen;
            mproplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
            bp += vilen;
            buflen -= vilen;
            mpropbytes = (char *)bp;
            if (mproplen < 0 || mproplen > buflen) {
                ism_common_setErrorData(ISMRC_BadLength, "%s%s", "SUBSCRIBE", "PropLen");
                return ISMRC_BadLength;
            } else {
                /* Check the properties.  If it is good we will just copy it as we got it */
                xrc = ism_common_checkMqttPropFields((char *)bp, mproplen,
                    g_ctx5, CPOI_SUBSCRIBE, mpropCheck, NULL);
                if (xrc)
                    return xrc;   /* setError must already be done */
            }
            bp += mproplen;
            buflen -= mproplen;
            /* Copy properties */
            ism_common_putMqttVarInt(buf, mproplen);
            if (mproplen)
                ism_common_allocBufferCopyLen(buf, mpropbytes, mproplen);
        } else {
            bp++;
            buflen--;
            bputchar(buf, 0);    /* Copy property length of 0 */
        }
    }

    while (buflen > 3) {
        uint8_t qos;
        int len = (uint16_t)BIGINT16(bp);
        bp += 2;
        buflen -= 2;
        if (buflen < (len+1))
        	break;
        /* Convert topic */
        topic_pos = buf->used;
        count++;
        xrc = convertTopic(transport, buf, (char *)bp, len, 1);
        if (xrc == 0 && transport->has_acl) {
            xrc = checkACL(transport, buf->buf+topic_pos);
        }
        if (!xrc) {
            goodcount++;
        } else {
            ttopic = fixtopic((const char *)bp, len>1023?1023:len, fixbuf, xrc==ISMRC_UnicodeNotValid);
            TRACE(6, "Error in convert topic on subscribe: topic=%s connect=%u client=%s rc=%u\n", ttopic, transport->index, transport->name, xrc);
            if (transport->pobj->mqtt_version >= 5) {
                int mqttrc = mapToMqttRc(xrc, transport->pobj->mqtt_version, CPOI_SUBACK);
                if (count == 1) {
                    subrc = mqttrc;
                } else {
                    if (count >= subrc_count) {
                        subrc_count = subrc_count == 1 ? 256 : subrc_count*4;
                        subrcp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_mqtt_subscribe,1),subrcp, subrc_count);
                        memset(subrcp+count-1, 0, subrc_count-count+1);
                        subrcp[0] = subrc;
                    }
                    subrcp[count-1] = mqttrc;
                }
                buf->used = topic_pos;
                ism_common_allocBufferCopyLen(buf, "\0\5$_$xx", 7);
                buf->buf[buf->used-2] = hexdigit[(mqttrc>>4) & 0xf];
                buf->buf[buf->used-1] = hexdigit[mqttrc & 0xf];
                if (transport->client_class == 'g' &&
                    (xrc==ISMRC_BadTopic || xrc==ISMRC_BadSysTopic || xrc==ISMRC_NotAuthorized)) {
                    const char * errStr = getMQTTErrorString(xrc, xbuf, sizeof xbuf);
                    ism_proxy_GWNotify(transport, "Subscribe", ttopic, NULL, NULL, xrc, errStr);
                }
            } else {
                if (transport->client_class == 'g' &&
                    (xrc==ISMRC_BadTopic || xrc==ISMRC_BadSysTopic || xrc==ISMRC_NotAuthorized)) {
                    const char * errStr = getMQTTErrorString(xrc, xbuf, sizeof xbuf);
                    ism_proxy_GWNotify(transport, "Subscribe", ttopic, NULL, NULL, xrc, errStr);
                    doreply = 1;
                } else {
                    rc = xrc;
                }
            }
        }
        bp += len;
        buflen -= len;
        qos = *bp&3;

        if (!rc) {
            int usefilter = 0;
            int badmask = transport->pobj->mqtt_version >=5 ? 0xC0 : 0xFC;
            if ((*bp&badmask) || (qos > 2)) {
                ttopic = fixtopic((char *)(bp-len), len>1023?1023:len, fixbuf, 0);
                ism_common_setErrorData(ISMRC_BadClientData, "%s%s", "SUBSCRIBE", ttopic);
                rc = ISMRC_InvalidQoS;
                break;
            }

            /* Downgrade QoS to max QoS for this tenant */
            if (qos > transport->tenant->max_qos)
                qos = transport->tenant->max_qos;
            qos |= (*bp&0xFC);

            /*
             * Check gateway group restriction
             */
            if (transport->has_acl) {
                const char * devtype;
                const char * devid;
                int cpylen = BIGINT16(buf->buf+topic_pos);
                memcpy(fixbuf, buf->buf+topic_pos+2, cpylen);
                fixbuf[cpylen] = 0;
                int ptarc = parseTopicAuth(fixbuf, &devtype, &devid);
                /* If there is a wildcard in the devtype or devid we use the filter */
                if (ptarc == 2) {
                    usefilter = 1;
                }
            }

            /*
             * Put out the QoS byte or subscriptions option extension
             */
            if (!transport->has_acl) {
                bputchar(buf, qos);   /* Copy the qos */
            } else {
                int extlen;
                int savepos = buf->used;
                buf->used += 2;    /* Skip over len, add later */

                /*
                 * Put out the ACL filter.
                 * This is of the form:  JMS_Topic aclcheck('aclname',3,5) is not false
                 */
                if (usefilter) {
                    bputchar(buf, EXIV_ACLFilter);
                    buf->used += 2;    /* Skip over len, add later */
                    ism_json_putBytes(buf, "JMS_Topic aclcheck('");
                    ism_json_putBytes(buf, transport->name);
                    ism_json_putBytes(buf, "',3,5) is not false");
                    extlen = buf->used - savepos - 5;
                    buf->buf[savepos+3] = (char)(extlen<<8);
                    buf->buf[savepos+4] = (char)extlen;
                }

                /* Put out the QoS byte */
                ism_common_putExtensionValue(buf, EXIV_SubscribeOpt, qos);

                extlen = buf->used - savepos - 2;
                buf->buf[savepos] = (char)(extlen<<8);
                buf->buf[savepos+1] = (char)extlen;
            }
        }
        buflen--;
        bp++;
    }
    /* Check that we are at the end of the payload and have at least one topic */
    if (buflen != 0 || count == 0) {
        if (rc == 0) {
            ism_common_setErrorData(ISMRC_BadLength, "%s", "SUBSCRIBE");
            count = 0;
            rc = ISMRC_BadLength;
        }
        *outcount = count;
        return rc;
    }
    if (rc == 0 && (doreply || goodcount==0)) {
        if (transport->pobj->mqtt_version >= 5) {
            sendSubAck(transport, count, (char *)inbp, subrcp ? subrcp : &subrc);
        } else {
            sendDirectReply(transport, (MT_SUBSCRIBE<<4) + 2, (char*)inbp, buflen, count);
        }
        *outcount = 0;
    } else {
        *outcount = count;
    }
    if (subrcp)
        ism_common_free(ism_memory_proxy_mqtt_subscribe,subrcp);
    return rc;
}

/*
 * Implement an incoming (from client) unsubscribe
 */
static int doUnsubscribe(ism_transport_t * transport, uint8_t * bp, int buflen, concat_alloc_t * buf, int * outcount) {
    static const char * hexdigit = "0123456789abcdef";
    int count = 0;
    int goodcount = 0;
    int doreply = 0;
    int mproplen;
    uint8_t subrc = 0;
    uint8_t * subrcp = NULL;
    int     subrc_count = 1;
    int vilen;
    int rc = ISMRC_OK;
    const char * ttopic;
    char fixbuf[1024];
    char xbuf[1024];
    uint8_t * inbp = bp;
    int  topic_pos;
    int xrc;
    char * mpropbytes;

    if (buflen < 5) {
        ism_common_setErrorData(ISMRC_BadLength, "%s", "UNSUBSCRIBE");
        return ISMRC_BadLength;
    }

    /* Copy msgid */
    memcpy(buf->buf + buf->used, bp, 2);
    buf->used += 2;
    bp += 2;
    buflen -= 2;

    /*
     * Parse the unsubscribe properties and copy them to the output buffer
     */
    if (buflen > 0 &&  transport->pobj->mqtt_version >= 5) {
        if (*bp != 0) {
            mproplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
            bp += vilen;
            buflen -= vilen;
            mpropbytes = (char *)bp;
            if (mproplen < 0 || mproplen > buflen) {
                ism_common_setErrorData(ISMRC_BadLength, "%s%s", "UNSUBSCRIBE", "MQTTPropLen");
                return ISMRC_BadLength;
            } else {
                /* Check the properties.  If it is good we will just copy it as we got it */
                xrc = ism_common_checkMqttPropFields((char *)bp, mproplen,
                    g_ctx5, CPOI_UNSUBSCRIBE, mpropCheck, NULL);
                if (xrc)
                    return xrc;   /* setError must already be done */
            }
            bp += mproplen;
            buflen -= mproplen;
            /* Copy properties */
            ism_common_putMqttVarInt(buf, mproplen);
            if (mproplen)
                ism_common_allocBufferCopyLen(buf, mpropbytes, mproplen);
        } else {
            /* TODO: remove special code to handle CSD01 to CSD02 incompat change */
            if (g_allowMQTTv5 > 1 || bp[1]==0) {
                bp++;
                buflen--;
                bputchar(buf, 0);
            }
        }
    }
    while (buflen > 2) {
        int len = (uint16_t)BIGINT16(bp);
        bp += 2;
        buflen -= 2;
        if (len > buflen)
            break;
        topic_pos = buf->used;
        xrc = convertTopic(transport, buf, (char *)bp, len, 1);
        if (!xrc) {
            goodcount++;
        } else {
            ttopic = fixtopic((const char *)bp, len>1023?1023:len, fixbuf, xrc==ISMRC_UnicodeNotValid);
            TRACE(6, "Error in convert topic on unsubscribe: topic=%s connect=%u client=%s rc=%u\n", ttopic, transport->index, transport->name, xrc);
            if (transport->pobj->mqtt_version >= 5) {
                int mqttrc = mapToMqttRc(xrc, transport->pobj->mqtt_version, CPOI_SUBACK);
                if (count == 0) {
                    subrc = mqttrc;
                } else {
                    if (count >= subrc_count) {
                        subrc_count = subrc_count == 1 ? 256 : subrc_count*4;
                        subrcp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_mqtt_subscribe,2),subrcp, subrc_count);
                        memset(subrcp+count, 0, subrc_count-count);
                        subrcp[0] = subrc;
                    }
                    subrcp[count] = mqttrc;
                }
                buf->used = topic_pos;
                ism_common_allocBufferCopyLen(buf, "\0\5$_$xx", 7);
                buf->buf[buf->used-2] = hexdigit[(mqttrc>>4) & 0xf];
                buf->buf[buf->used-1] = hexdigit[mqttrc & 0xf];
                if (transport->client_class == 'g' &&
                    (xrc==ISMRC_BadTopic || xrc==ISMRC_BadSysTopic || xrc==ISMRC_NotAuthorized)) {
                    const char * errStr = getMQTTErrorString(xrc, xbuf, sizeof xbuf);
                    ism_proxy_GWNotify(transport, "Unsubscribe", ttopic, NULL, NULL, xrc, errStr);
                }
            } else {
                if (transport->client_class == 'g' &&
                    (xrc==ISMRC_BadTopic || xrc==ISMRC_BadSysTopic || xrc==ISMRC_NotAuthorized)) {
                    const char * errStr = getMQTTErrorString(xrc, xbuf, sizeof xbuf);
                    ism_proxy_GWNotify(transport, "Unsubscribe", ttopic, NULL, NULL, xrc, errStr);
                    doreply = 1;
                } else {
                    rc = xrc;
                }
            }
        }
        buflen -= len;
        bp += len;
        count++;
    }
    /* Check that we are at the end of the payload and have at least one topic */
    if (buflen != 0 || count == 0) {
        if (rc == 0) {
            rc = ISMRC_BadLength;
            ism_common_setErrorData(ISMRC_BadLength, "%s", "UNSUBSCRIBE");
            count = 0;
        }
        *outcount = count;
        return rc;
    }
    if (rc == 0 && (doreply || goodcount==0)) {
        if (transport->pobj->mqtt_version >= 5) {
            sendUnsubAck(transport, count, (char *)inbp, subrcp ? subrcp : &subrc);
        } else {
            sendDirectReply(transport, (MT_UNSUBSCRIBE<<4) + 2, (char*)inbp, buflen, count);
        }
        *outcount = 0;
    } else {
        *outcount = count;
    }
    if (subrcp)
        ism_common_free(ism_memory_proxy_mqtt_subscribe,subrcp);
    return ISMRC_OK;
}

/*
 * Implement an incoming connect packet
 */
static int handleConnectRequest(ism_transport_t * transport, mqttMsg_t * mmsg, uint8_t * bp, int buflen, int kind, concat_alloc_t * buf) {
    mqttProtoObj_t * pobj = transport->pobj;
    char * inbuf = (char*)bp;
    int savebuflen = buflen;
    uint8_t * verpos = NULL;
    uint8_t inver;
    if (buflen < 2) {
        ism_common_setErrorData(ISMRC_BadLength, "%s", "CONNECT");
        return ISMRC_BadLength;
    }

    /* Check if connect is already happened. Do not allow twice */
    mmsg->command = MT_CONNECT;
    mmsg->version = pobj->mqtt_version;    /* Version comes from pobj except for CONNECT */
    mmsg->kind = kind;
    pobj->startState = MQTT_IN_PROGRESS;
    mmsg->topic_len = BIGINT16(bp);
    buflen -= 2;
    if (UNLIKELY(buflen <= mmsg->topic_len)) {
        ism_common_setErrorData(ISMRC_BadLength, "%s", "CONNECT");
        return ISMRC_BadLength;
    }
    bp += 2;
    mmsg->topic = (char *) bp;
    bp += mmsg->topic_len;
    buflen -= mmsg->topic_len;
    mmsg->version = *bp;
    inver = *bp;
    verpos = bp;
    *bp = 0;
    /* MQTT v3.1 */
    if (mmsg->topic_len == 6 && !strcmp(mmsg->topic, "MQIsdp") ) {
        if (mmsg->version != 3 )
            mmsg->version = 0;
    }
    /* MQTT v3.1.1 or MQTTv5 */
    else if (mmsg->topic_len == 4 && !strcmp(mmsg->topic, "MQTT") ) {
        if (mmsg->version != 4 &&
            (mmsg->version != 5 || !g_allowMQTTv5))
            mmsg->version = 0;
    }
    else {
        mmsg->version = 0;
    }
    if (!mmsg->version) {
        ism_common_setErrorData(ISMRC_BadClientData, "%s%s", "CONNECT", "version");
        mmsg->rc = ISMRC_BadClientData;
    }
    *bp = mmsg->version;

    if (mmsg->version > 3 && (kind&0x0f)) {
        ism_common_setErrorData(ISMRC_BadClientData, "%s", "CONNECT");
        mmsg->rc = ISMRC_BadClientData;
    }
    if (mmsg->version == 4 && !strcmp(transport->protocol, "mqtt-tcp")) {
        transport->protocol = "mqtt4-tcp";
    } else if (mmsg->version == 5 && !strcmp(transport->protocol, "mqtt-tcp")) {
        transport->protocol = "mqtt5-tcp";
    }

    /*
     * If this is an MQTT connect then parse it
     */
    if (buflen > 3 && (mmsg->version >= 3)) {
        pobj->mqtt_version = mmsg->version;
        mmsg->flags = bp[1];
        if (mmsg->flags & 0x02)
            pobj->cleansession = 1;
        if (mmsg->version>3 && (mmsg->flags&0x01)) {
            mmsg->rc = ISMRC_BadClientData;
            ism_common_setErrorData(mmsg->rc, "%s", "CONNECT");
        } else {
            bp += 2;
            mmsg->keepalive = (uint16_t) BIGINT16(bp);
            buflen -= 4;
            bp += 2;
            if (mmsg->version >= 5) {
                mmsg->request_reason = 2;
                if (*bp != 0) {
                    int vilen;
                    int mproplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
                    bp += vilen;
                    buflen -= vilen;
                    mmsg->props = (char *)bp;
                    mmsg->prop_len = mproplen;
                    if (mproplen < 0 || mproplen > buflen) {
                        mmsg->rc = ISMRC_BadLength;
                        ism_common_setErrorData(mmsg->rc, "%s%s", "CONNECT", "MQTTExtLen");
                    } else {
                        mmsg->rc = ism_common_checkMqttPropFields((char *)bp, mproplen,
                            g_ctx5, CPOI_CONNECT, mpropCheck, mmsg);
                    }
                    pobj->requestReason = mmsg->request_reason;
                    bp += mproplen;
                    buflen -= mproplen;
                } else {
                    bp++;
                    buflen--;
                }
            }
            mmsg->payload = (char *) bp;
            mmsg->payload_len = buflen;
            mmsg->transport = transport;
            parseConnectPayload(mmsg);
        }
    }

    /*
     * On a CONNECT we wait until after the parse to make sure we do not trace
     * a password.
     */
    if (UNLIKELY(SHOULD_TRACE(9))) {
        char obuf[64];
        int maxlen = ism_common_getTraceMsgData();
        if (mmsg->rc == 0) {
            if (mmsg->password)
                maxlen = mmsg->password - inbuf;
        }
        if (verpos) {
            *verpos = inver;
        }
        sprintf(obuf, "MQTT receive 10 CONNECT: connect=%u", transport->index);
        traceDataFunction(obuf, 0, __FILE__, __LINE__, inbuf, savebuflen, maxlen);
        if (verpos)
            *verpos = 0;
    }


    /*
     * Perform the connect
     */
    if (LIKELY(mmsg->rc == 0)) {
        doConnect(transport, mmsg, buf);
    } else {
        if (pobj->mqtt_version >= 5) {
            int reasonlen;
            char * erbuf = alloca(2048);
            buf->buf[16] = 0;
            buf->buf[17] = mapToMqttRc(mmsg->rc, pobj->mqtt_version, CPOI_CONNACK);
            buf->buf[16] = 0;
            buf->used = 18;
            ism_common_formatLastError(erbuf, 2048);
            reasonlen = strlen(erbuf);
            if (reasonlen && (!transport->pobj->maxPacketSize || (reasonlen+11 < transport->pobj->maxPacketSize))) {
                ism_common_putMqttVarInt(buf, reasonlen+3);
                ism_common_putMqttPropString(buf, MPI_Reason, g_ctx5, erbuf, reasonlen);
            } else {
                buf->used++;            /* proplen = 0 */
            }
            transport->send(transport, buf->buf + 16, buf->used-16, MT_CONNACK << 4, SFLAG_SYNC | SFLAG_FRAMESPACE);
        }
    }
    buf->used = 0;
    return mmsg->rc;
}

#ifndef NO_PXACT
// PXACT
static int ism_pxact_track_client(ism_transport_t * transport, PXACT_ACTIVITY_TYPE_t type) {
	if (!transport) {
		TRACE(3, "ism_pxact_track_client: transport is NULL.\n");
		return 0;
	}
	if (!transport->tenant) {
		TRACE(5, "ism_pxact_track_client: tenant is NULL.\n");
		return 0;
	}
	if (!transport->tenant->name) {
		TRACE(3, "ism_pxact_track_client: tenant name is NULL.\n");
		return 0;
	}
    if (transport->ready != 3) { //Avoid processing activity for clients which are not successfully connected (didn't receive CONACK)
        TRACE(7, "ism_pxact_track_client: Client is not connected successfully. activityType=%d connect=%d clientId=%s transport->ready=%d.\n", type, transport->index, transport->clientID, transport->ready);
        return 0;
    }

	if (!transport->tenant->pxactEnabled ||
			!ism_pxactivity_is_started() ||
			(transport->client_class == 'A' && transport->genClientID) ||
			!transport->server) {
		if (!transport->tenant->pxactEnabled) {
			TRACE(9, "ism_pxact_track_client: client activity monitoring is disabled for tenant=%s\n", transport->tenant->name);
		}
		return 0;
	} else {
		return 1;
	}
}

static void ism_pxact_alloc_pxclient(ism_transport_t * transport) {
	transport->aobj = (ismPXACT_Client_t *)ism_transport_allocBytes(transport, sizeof(ismPXACT_Client_t), 1);
	ismPXACT_Client_t *cl = (ismPXACT_Client_t *)transport->aobj;
	memset(cl,0,sizeof(ismPXACT_Client_t));
	cl->pClientID = transport->clientID;
    cl->pOrgName = transport->org;
	cl->pDeviceType = transport->typeID;
	cl->pDeviceID = transport->deviceID;
	switch (transport->client_class) {
		case 'd': cl->clientType = PXACT_CLIENT_TYPE_DEVICE ; break;
		case 'a': cl->clientType = PXACT_CLIENT_TYPE_APP ; break;
		case 'A': cl->clientType = PXACT_CLIENT_TYPE_APP_SCALE ; break;
		case 'g': cl->clientType = PXACT_CLIENT_TYPE_GATEWAY ; break;
		case 'c': cl->clientType = PXACT_CLIENT_TYPE_CONNECTOR ; break;
		default : cl->clientType = PXACT_CLIENT_TYPE_OTHER ; break;
	}
}

void ism_pxact_disconnect(ism_transport_t * transport, int rc) {
	int arc;
	ismPXACT_ConnectivityInfo_t ci[1];
	ism_transport_t *ctransport;

    if (!( transport->protocol_family && (strcmp(transport->protocol_family, "mqtt")==0) )) {
    	TRACE(9,"ism_pxact_disconnect: connect=%d protocol_family %s will not be tracked.\n",
    			transport->index, transport->protocol_family);
        return;
    }
    if (!transport->pobj) {
    	TRACE(5,"ism_pxact_disconnect: connect=%d protocol local object is NULL.\n", transport->index);
        return;
    }

    ctransport = transport->pobj->client_transport ? transport->pobj->client_transport : transport ;
	if (!ctransport->aobj) {
		TRACEL(8, ctransport->trclevel, "ism_pxact_disconnect: connect=%d activity client object is NULL.\n", ctransport->index);
		return;
	}

    if (!ism_pxact_track_client(ctransport, PXACT_ACTIVITY_TYPE_DISCONN)) {
		return;
	}

	ism_protobj_t *cpobj = ctransport->pobj ;
	uint8_t reasonCode;
	if (cpobj->pendingRC)
		reasonCode = cpobj->pendingRC;
	else
		reasonCode= (uint8_t)mapToMqttRc(rc, cpobj->mqtt_version, CPOI_DISCONNECT);

	if (reasonCode == MQTTRC_SessionTakenOver) {
		// Client ID steal, skip
		TRACEL(7, ctransport->trclevel, "ism_pxact_disconnect: connect=%d Client ID was reused.\n", ctransport->index);
		return;
	}

	ismPXACT_Client_t *cl = (ismPXACT_Client_t *)ctransport->aobj ;
	memset(ci,0,sizeof(ismPXACT_ConnectivityInfo_t));
	ci->connectionID = ism_transport_getConnId(ctransport);
	switch (cpobj->mqtt_version) {
		case 3 : ci->protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1 ; break ;
		case 4 : ci->protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1_1 ; break ;
		case 5 : ci->protocol = PXACT_CONN_PROTO_TYPE_MQTT_v5 ; break ;
		default: ci->protocol = PXACT_CONN_PROTO_TYPE_NONE ; break ;
	} 				
	ci->cleanS = cpobj->cleansession;
	ci->targetServerType = PXACT_TRG_SERVER_TYPE_MS;
	if ( cl->activityType != PXACT_ACTIVITY_TYPE_MQTT_DISCONN_SERVER && 
   		 cl->activityType != PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT ) {        		
		cl->activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_NETWORK;
	}

	ci->reasonCode = reasonCode;

	TRACEL(9, ctransport->trclevel, "ism_pxact_disconnect: connect=%d no_monitor=%d Calling ism_pxactivity_Client_Connectivity with actType %d for cid %s, org %s devType %s devId %s cType %d connId %lu\n",
			ctransport->index, ctransport->no_monitor, cl->activityType,cl->pClientID,cl->pOrgName,cl->pDeviceType,cl->pDeviceID,cl->clientType,ci->connectionID);
	arc = ism_pxactivity_Client_Connectivity(cl, ci);
	if ( arc != ISMRC_OK && arc != ISMRC_Closed ) {
		TRACEL(1, ism_defaultTrace, "ism_pxactivity_Client_Connectivity failed, rc= %d\n", arc);
	}    					
}            
#endif

/**
 * Set Discard Msgs Enabled or Disabled
 */
int ism_mqtt_setDiscardMsgsEnabled(int enabled){

	if(g_discardMsgsEnabled!=enabled){
		g_discardMsgsEnabled=enabled;
		TRACEL(5,ism_defaultTrace, "Set DiscardMsgsEnabled: enabled=%d\n", g_discardMsgsEnabled);
	}
	return 0;
}
/**
 * Set Soft Limit for Discard Msgs
 */
int ism_mqtt_setDiscardMsgsSoftLimit(int limit){
	if(g_discardMsgsSoftLimit!=limit){
		g_discardMsgsSoftLimit=limit;
		TRACEL(5,ism_defaultTrace, "Set DiscardMsgsSoftLimit: limit=%d\n", g_discardMsgsSoftLimit);
	}
	return 0;
}
/**
 * Set Hard Limit for Discard Msgs
 */
int ism_mqtt_setDiscardMsgsHardLimit(int limit){
	if(g_discardMsgsHardLimit!=limit){
		g_discardMsgsHardLimit=limit;
		TRACEL(5,ism_defaultTrace, "Set DiscardMsgsHardLimit: limit=%d\n", g_discardMsgsHardLimit);
	}
	return 0;
}

/**
 * Set Log Interval for Discard Msgs.
 * Value is in seconds
 * Default: 6 hours
 */
int ism_mqtt_setDiscardMsgsLogInterval(int interval){
	if(g_discardMsgsLogIntervalInSec!=interval){
		g_discardMsgsLogIntervalInSec=interval;
		TRACEL(5,ism_defaultTrace, "Set DiscardMsgsLogInterval: interval=%d\n", g_discardMsgsLogIntervalInSec);

	}
	return 0;
}

/**
 * Set SendGWPingREQ
 * Value is in seconds
 * Default: 6 hours
 */
int ism_mqtt_setSendGWPingREQ(int enabled){
	if(g_sendGWPingREQ!=enabled){
		g_sendGWPingREQ=enabled;
		TRACEL(5,ism_defaultTrace, "Set SendGWPingREQ: enabled=%d\n", g_sendGWPingREQ);

	}
	return 0;
}

/**
 * Check the Pending Send Bytes Limit.
 * 1. If the Soft Limit is reached, log it
 * 2. If the Hard Limit is reached, disconnect or else?
 */
int checkDiscardMsgsLimit(ism_transport_t * transport)
{
	int result = 0;
	const char * msg;
	if(!g_discardMsgsEnabled){
		return 0;
	}

	if(transport){
		int pendingSendBytes = transport->sendQueueSize * sendSize;

		if(pendingSendBytes >= g_discardMsgsHardLimit){
			msg = "Pending Send Bytes Hard Limit has been exceeded. Closing connection for slow consumer: ClientID={0} UserID={1} ConnectionID={2} From={3}:{4} PendingSendBytes={5} MaxAllowed={6}";
			LOG(WARN, Connection, 994, "%-s%s%d%s%u%d%d", msg,
					 transport->name, transport->userid, transport->index, transport->client_addr, transport->clientport, pendingSendBytes, g_discardMsgsHardLimit);
			result=1;//Hard Limit is reached

		}

		if(!result && pendingSendBytes >= g_discardMsgsSoftLimit){
			//Construct the Msg
			ism_time_t now = ism_common_readTSC();
			if((now - transport->lastDiscardMsgsLogTime) > g_discardMsgsLogIntervalInSec){
				msg = "Pending Send Bytes Soft Limit has been exceeded for slow consumer: ClientID={0} UserID={1} ConnectionID={2} From={3}:{4} PendingSendBytes={5} MaxAllowed={6}";
				LOG(WARN, Connection, 993, "%-s%s%d%s%u%d%d", msg,
						 transport->name, transport->userid,  transport->index, transport->client_addr, transport->clientport, pendingSendBytes, g_discardMsgsSoftLimit);
				transport->lastDiscardMsgsLogTime=ism_common_readTSC();
			}
			result=0;//Soft Limit is reached
		}

	}

	return result;
}

/*
 * Check the subscriptions are still valid based on the current authority and ACLs
 * We can run either a full match including doing topic rule matching, or an ACL only check.
 */
int checkSubs(const char * subs, int sublen, ism_transport_t * stransport, ism_transport_t * transport, int full, int dounsub) {
    const char * sp = subs;
    const char * topic;
    const char * fulltopic;
    uint16_t topiclen;
    uint16_t fulltopiclen;
    uint32_t subid;
    uint8_t  subopt;
    int count = 0;
    int cnt = 0;
    char xbuf [2048];
    char uxbuf [2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    concat_alloc_t ubuf = {uxbuf, sizeof uxbuf, 18};
    ism_topicrule_t * topicrule = NULL;
    ism_pxrule_t * * rules;
    int matched;
    int rc = 0;

    uxbuf[16] = 0;
    uxbuf[17] = 0;
    if (full) {
        topicrule = ism_proxy_getTopicRuleClass(transport->tenant, transport->client_class);
    }
    while (sublen > 2) {
        topiclen = fulltopiclen = BIGINT16(sp);
        if (sublen < topiclen + 7 || sp[topiclen+2])
            break;    /* error handling */
        cnt++;
        topic = fulltopic = sp+2;
        memcpy(&subid, sp+topiclen+2, 4);
        subopt = sp[topiclen+6];
        TRACE(9, "Subscription %u: connect=%u topic=%s subopt=%02x subid=%u\n",
                cnt, transport->index, topic, subopt, endian_int32(subid));
        sp += topiclen + 7;
        sublen -= topiclen + 7;

        if (topicrule) {
            int   i;
            const char * pos;
            fulltopic = topic;
            rules = topicrule->subscribe;
            count = topicrule->subscribe_count;

            /*
             * Ignore the shared prefix
             */
            if (*topic == '$') {
                if (topiclen > 20 && !memcmp(topic, "$SharedSubscription/", 20)) {
                    topic += 20;
                } else if (topiclen >= 7 && !memcmp(topic, "$share/", 7)) {
                    topic += 7;
                } else {
                    continue;
                }
                pos = strchr(topic, '/');
                if (pos) {
                    topic = pos+1;
                } else {
                    continue;
                }
                topiclen -= (topic-fulltopic);
            }

            /* Convert the topic to the external format */
            buf.used = 0;
            convertTopicOut(transport, &buf, topic, topiclen);

            /* Check if we have a matching mapping rule */
            matched = 0;
            for (i=0; i<count; i++) {
                if (matchTopicRule(transport, rules[i], buf.buf+2, buf.used-2)) {
                    matched = 1;
                    break;
                }
            }
        } else {
            matched = 1;
        }

        /* Check the ACL */
        if (matched && transport->has_acl) {
            /* We might not yet have checked if we have ACLs */
            if (transport->has_acl == 2) {
                const char * aclKey = ism_proxy_getACLKey(transport);
                ism_acl_t * acl = ism_protocol_findACL(aclKey, 0, NULL);
                if (acl) {
                    ism_protocol_unlockACL(acl);
                    transport->has_acl = 1;
                }
            }
            if (transport->has_acl == 1) {
                if (checkACL(transport, topic-2))
                    matched = 0;
            }
        }

        /* If subscription would not now be allowed, unsubscribe it */
        if (!matched) {
            if (dounsub) {
                TRACE(5, "Automatic unsubscribe: connect=%u client=%s topic=%s\n",
                        transport->index, transport->name, topic);
            } else {
                TRACE(5, "Subscription not authorized: connect=%u client=%s topic=%s\n",
                        transport->index, transport->name, topic);
            }
            /* LOG? */
            if (ubuf.used == 18) {
                if (transport->pobj->mqtt_version == 5) {
                    uxbuf[18] = 0;    /* Add properties for MQTTv5 */
                    ubuf.used++;
                }
            }
            /* add the topic to the unsub buffer */
            ism_common_allocBufferCopyLen(&ubuf, fulltopic-2, fulltopiclen+2);
        }
    }

    /*
     * If we found any to unsubscribe, send it to the server
     * This is sent with packetid=0 and does not expect a response.
     */
    if (ubuf.used != 18) {
        rc = 1;
        if (dounsub) {
            stransport->send(stransport, ubuf.buf+16, ubuf.used-16, (MT_UNSUBSCRIBE<<4) + 2, SFLAG_FRAMESPACE);
        }
    }

    /* Clean up buffers */
    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);
    if (ubuf.inheap)
        ism_common_freeAllocBuffer(&ubuf);
    return rc;
}

#define alloc_str(r, s,l) \
    (r) = alloca((l)+1);\
    memcpy((r), (s), (l));\
    (r)[l] = 0;

/*
 * Receive MQTT binary.
 *
 * This is an instance of ism_transport_receive_t invoked using transport->receive().
 *
 * This method is used when doing native binary MQTT.  The MQTT command byte is sent as
 * the kind parameter.
 *
 * In any case where we return a non-zero return code, the connection should have been
 * disconnected.
 *
 */
int ism_mqtt_receive(ism_transport_t * transport, char * inbuf, int buflen, int kind) {
    uint8_t * bp = (uint8_t *) inbuf;
    int rc = ISMRC_OK;
    char xbuf[4096];
    int  subcnt = 0;

    int  sendit = 1;      /* Resend this packet */
    int  decrement = 1;   /* Decrement inprogress */
    int  throttle = 0;
    concat_alloc_t buf = {xbuf, sizeof xbuf, 16};
    mqttProtoObj_t * pobj = transport->pobj;
    uint8_t command = (uint8_t)((kind >> 4) & 15);
    mqttMsg_t mmsg;



    // printf("recvmqtt: kind=%02x buflen=%d bp=%02x %02x %02x\n", kind&0xff, buflen, bp[0]&0xff, bp[1]&0xff, bp[2]&0xff);
    if (pobj == NULL) {
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }

    /* If protocol is closing the connection, reject the message */
    if (__builtin_expect((pobj->closed || pobj->delay_closing || pobj->disconnect_pending), 0)) {
        ism_common_setError(ISMRC_Closed);
        TRACE(5, "MQTT receive %02x %s on closed connection connect=%u", kind & 0xff, ism_mqtt_mqttCommand(command), transport->index);
        return ISMRC_Closed;
    }

    /* If we are in the process of closing, return closed */
#ifdef DEBUG_INPROGRESS
    TRACE(9, "Increment inprogress: connect=%u inprogress=%d\n", transport->index, pobj->inprogress+1);
#endif
    if (__builtin_expect((__sync_fetch_and_add(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
        __sync_fetch_and_sub(&pobj->inprogress, 1);
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }

    /* Do the receive trace */
    if (SHOULD_TRACE(9) && command != MT_CONNECT) {
        char obuf[64];
        sprintf(obuf, "MQTT receive %02x %s connect=%u inprogress=%d", kind & 0xff, ism_mqtt_mqttCommand(kind), transport->index, pobj->inprogress);
        traceDataFunction(obuf, 0, __FILE__, __LINE__, inbuf, buflen, ism_common_getTraceMsgData());
    }

    if ((transport->clientID == NULL) || (transport->clientID[0] == '\0')) {
        /*If command is NOT CONNECT and clientID is NOT already set. It is an error*/
        if (command != MT_CONNECT) {
            ism_common_setError(ISMRC_ConnectFirst);
            rc = ISMRC_ConnectFirst;
        }
    } else {
        /*If command is CONNECT and clientID is already set. Consider it is an error.*/
        if (command == MT_CONNECT) {
            ism_common_setError(ISMRC_ConnectFirst);
            rc = ISMRC_ConnectFirst;
        }
    }

    /*
     * For MQTTv5 it is a protocol error if the packet exceeds our specified max packet size
     */
    if (transport->maxMsgSize && (pobj->mqtt_version >= 5 || command == MT_CONNECT)) {
        int chklen = buflen + 2;
        if (buflen >= 128) {
            chklen++;
            if (buflen >= 16384) {
                chklen++;
                if (buflen >= 2097152)
                    chklen++;
            }
        }
        if (chklen > transport->maxMsgSize) {
            rc = ISMRC_MsgTooBig;
            ism_common_setErrorData(rc, "%u%u", chklen, transport->maxMsgSize);
        }
    }

    if (!rc) {
        /*
         * Process the commands
         */
        switch (command) {

        /*
         * Publish
         */
        case MT_PUBLISH:
            transport->read_msg++;
            memset(&mmsg, 0, sizeof mmsg);
            mmsg.command = command;
            mmsg.kind = (uint8_t)kind;
            mmsg.transport = transport;
            if (transport->originated)
                rc = doPublishS2P(transport, &mmsg, bp, buflen, &buf);
            else {
                rc = doPublishC2P(transport, &mmsg, bp, buflen, &buf);
#ifndef NO_PXACT
	            // PXACT
                if (!rc && ism_pxact_track_client(transport, PXACT_ACTIVITY_TYPE_MQTT_PUBLISH))
	            {
	            	int arc;
	            	if (!transport->aobj ) ism_pxact_alloc_pxclient(transport);
	            	ismPXACT_Client_t *cl = (ismPXACT_Client_t *)transport->aobj;
	            	cl->activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
					arc = ism_pxactivity_Client_Activity(cl);
					if ( arc != ISMRC_OK && arc != ISMRC_Closed ) {
						TRACEL(1, ism_defaultTrace, "ism_pxactivity_Client_Activity (%d) failed, rc= %d\n",cl->activityType,arc);
					}
				}
#endif
            }
            kind = mmsg.kind;
            if (rc) {
                if (rc == 2) {
                    sendit = 0;
                    decrement = 1;
                    break;
                }
                if (rc == -9) {
                    throttle = 1;
                    rc = 0;
                }
            }

            if (!transport->originated && transport->client_class == 'g') {
                int shouldNotify = 1;
                transport->endpoint->stats->count[transport->tid].read_msg++;
                //sendGWPINGREQ(transport);
                if (!rc || rc == ISMRC_NotAuthorized) {
                    if (!rc && transport->has_acl) {
                        //NOTE: SECURITY:
                        //During doConnect, we set transport->has_acl=2 initially. It will goes
                        //to this code first if CONNACK is not back yet.
                        rc = checkACL(transport, buf.buf+16);
                        if (rc) {
                            ism_common_setErrorData(rc, "%s%s", "Device not in group: ", buf.buf+16);
                        }
                    }
                    /* If we need to go async, do not send the action */
                    else if (g_gatewayRegister) {

                        //If CONNACK is NOT back yet, save the data for later auto registeration
                        if (LIKELY(pobj->connectdata == NULL && pobj->savedata == NULL && !pobj->savedataprocessing)) {
                            if (!rc) {
                                //For Priviledged GW: Always go thru checkPublishAuth
                                // This gateway does not have any resource group
                                rc = checkPublishAuth(transport, &buf, kind, rc, 0);
                                shouldNotify = 0;
                                TRACE(7, "MQTT receive MT_PUBLISH connect=%u client=%s gateway does not have any resource group has_acl=%d rc=%d shouldNotify=%d\n",
                                        transport->index, transport->name, transport->has_acl, rc, shouldNotify);

                            } else {
                               //For Non Priviledge Gateway: Still allow AutoRegister if there is permission

                                if ((rc == ISMRC_NotAuthorized) && (transport->auth_permissions & 0x200)) {
                                    TRACE(9, "Gateway CheckPublish: CONNACK is completed. checkPublishAuth continue. connect=%u client=%s has_acl=%d ready=%d\n",
                                                                                             transport->index, transport->name, transport->has_acl, transport->pobj->server_transport->ready);
                                    rc = checkPublishAuth(transport, &buf, kind, rc, 0);
                                    shouldNotify = 0;
                                    TRACE(7, "MQTT receive MT_PUBLISH connect=%u client=%s gateway has resource group has_acl=%d rc=%d shouldNotify=%d\n",
                                            transport->index, transport->name, transport->has_acl, rc, shouldNotify);
                                }

                            }
                        }else{
                            //Transport CONNACK is NOT completed. Save the data for later processing.
                            TRACE(9, "Append SavedData of Gateway for Publish and Device Auto Register: connect=%d len=%d command=%d\n", transport->index, buf.used-16, command);
                            appendSavedData(transport,buf.buf+16, buf.used-16, kind);
                            sendit = 0;
                            rc = 0;
                            decrement = 1;
                        }

                        if (rc == ISMRC_NotAuthorized && !shouldNotify) {
                            sendGWPINGREQ(transport);
                            if (pobj->mqtt_version >= 5)
                                publishNAK(&mmsg);
                            else
                                sendDirectReply(transport, kind, (char*)bp, buflen, 1);
                            sendit = 0;
                            rc = 0;
                            decrement = 1;
                            transport->endpoint->stats->count[transport->tid].lost_msg++;
                            if (pobj->server_transport)
                                pobj->server_transport->lost_msg++;
                        }
                    }
                }

                TRACE(7, "MQTT receive MT_PUBLISH connect=%u client=%s rc=%d shouldNotify=%d\n",
                        transport->index, transport->name, rc, shouldNotify);

                if (rc) {
                    sendit = 0;
                    if (rc == ISMRC_AsyncCompletion) {
                        rc = 0;
                        decrement = 0;
                    } else {
                        decrement = 1;
                        /* Do a notification but not for malformed and protocol errors */
                        if (shouldNotify && rc != ISMRC_BadLength && rc != ISMRC_InvalidQoS && rc != ISMRC_MsgTooBig) {
                            /* Error during doPublish. Send Notification */
                            const char * errStr = getMQTTErrorString(rc, xbuf, sizeof xbuf);
                            int    topiclen = BIGINT16(bp);
                            char * temp_topic = (char *)((rc == ISMRC_UnicodeNotValid) ? FIXUNICODE((const char *)(bp+2), topiclen) :
                                    FIXTOPIC((const char *)(bp+2), topiclen));
                            ism_proxy_GWNotify(transport, "Publish", temp_topic, NULL, NULL, rc, errStr);
                            sendDirectReply(transport, kind, (char*)bp, buflen, 1);
                            rc = 0;
                        }
                        transport->endpoint->stats->count[transport->tid].lost_msg++;
                        if (pobj->server_transport)
                            pobj->server_transport->lost_msg++;
                    }
                }
            } else if ((transport->client_class == 'a') || (transport->client_class == 'A')) {
                    if (!rc && transport->has_acl) {
                        rc = checkACL(transport, buf.buf+16);
                        if (rc) {
                            ism_common_setErrorData(rc, "%s%s", "Device not in group: ", buf.buf+16);
                        }
                    }
                    if (rc) {
                        // Also handle error such as invalid topic returned by doPublishC2P() above
                        sendit = 0;
                        decrement = 1;
                        break;
                    }
#ifndef NO_SQS
            } else if(transport->server != NULL && transport->server->serverKind==2){
					//AWS SQS Publish
					ism_proxy_sqs_send_message(transport, mmsg.payload, mmsg.payload_len);
					sendit = 0;
					decrement = 0;
#endif
            }

            break;

        /*
         * Publish acknowledge (QoS=1)
         */
        case MT_PUBACK: /* ACK publish from QoS=1 */
            ism_common_allocBufferCopyLen(&buf, (char *)bp, buflen);
            break;

        /*
         * Publish received (first response for QoS=2)
         */
        case MT_PUBREC: /* Message received for QoS=2 */
            ism_common_allocBufferCopyLen(&buf, (char *)bp, buflen);
            break;

        /*
         * Publish release (second response for QoS=2)
         */
        case MT_PUBREL: /* Release publish for QoS=2 */
            ism_common_allocBufferCopyLen(&buf, (char *)bp, buflen);
            break;

        /*
         * Publish complete (third response for publish)
         */
        case MT_PUBCOMP: /* Message is consumed for QoS=2 */
            ism_common_allocBufferCopyLen(&buf, (char *)bp, buflen);
            break;

        /*
         * Subscribe
         */
        case MT_SUBSCRIBE:
            if (!transport->server) {
                rc = ISMRC_NotAuthorized;
                ism_common_setErrorData(rc, "%s", "Subscribe");
                break;
            }
            rc = doSubscribe(transport, bp, buflen, &buf, &subcnt);
            if (transport->has_acl)
                kind = MT_SUBSCRIBEX;
            if (subcnt == 0 && rc == 0)    /* Notified in doSubscribe */
                sendit = 0;

            /*
             *  For legacy gateways (whithout ACLs) with auto registration enabled check for
             *  auto registration on subscribe.  As this code conflicts with the MQTTv5 error
             *  handling do not do it when using MQTTv5 and above.  It would be a bit better to
             *  remove it all together.
             */
            if (subcnt>0 && g_gatewayRegister && transport->client_class == 'g'
                    && !transport->has_acl && pobj->mqtt_version < 5) {
                if (LIKELY(pobj->connectdata == NULL && pobj->savedata == NULL && !pobj->savedataprocessing)) {
                    rc = checkSubscribeAuth(transport, &buf, kind, subcnt, 0);
                    if (rc) {
                        if (rc != ISMRC_AsyncCompletion) {
                            sendDirectReply(transport, kind, (char*)bp, buflen, subcnt);
                            decrement = 1;
                        } else {
                            decrement = 0;
                        }
                        sendit = 0;
                        rc = 0;
                    }
                }else{
                    //Transport CONNACK is completed. Save the data for later processing.
                   TRACE(9, "Append SavedData of Gateway for SUBSCRIBE and Device Auto Register: connect=%d len=%d command=%d\n", transport->index, buf.used-16, command);
                   appendSavedData(transport,buf.buf+16, buf.used-16, kind);
                   sendit = 0;
                   rc = 0;
                   decrement = 1;
                }
            }

#ifndef NO_PXACT
            /*
             * PXACT - Do activity monitoring
             */
            if (!rc && LIKELY(sendit != 0) && ism_pxact_track_client(transport, PXACT_ACTIVITY_TYPE_MQTT_SUB))
            {
            	int arc;
            	if (!transport->aobj ) ism_pxact_alloc_pxclient(transport);
            	ismPXACT_Client_t *cl = (ismPXACT_Client_t *)transport->aobj;
            	cl->activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
				arc = ism_pxactivity_Client_Activity(cl);
				if ( arc != ISMRC_OK && arc != ISMRC_Closed ) {
					TRACEL(1, ism_defaultTrace, "ism_pxactivity_Client_Activity (%d) failed, rc= %d\n",cl->activityType,arc);
				}
			}
#endif
            break;

        /*
         * Subscribe ACK
         */
        case MT_SUBACK:
            ism_common_allocBufferCopyLen(&buf, (char *)bp, buflen);
            break;

        /*
         * Unsubscribe
         */
        case MT_UNSUBSCRIBE:
            rc = doUnsubscribe(transport, bp, buflen, &buf, &subcnt);
            if (subcnt == 0 && rc == 0)
                sendit = 0;
#ifndef NO_PXACT
            // PXACT
            if (LIKELY(sendit != 0) && ism_pxact_track_client(transport, PXACT_ACTIVITY_TYPE_MQTT_UNSUB))
            {
            	int arc;
            	if (!transport->aobj ) ism_pxact_alloc_pxclient(transport);
            	ismPXACT_Client_t *cl = (ismPXACT_Client_t *)transport->aobj;
            	cl->activityType = PXACT_ACTIVITY_TYPE_MQTT_UNSUB;
				arc = ism_pxactivity_Client_Activity(cl);
				if ( arc != ISMRC_OK && arc != ISMRC_Closed ) {
					TRACE(1, "ism_pxactivity_Client_Activity (%d) failed, rc= %d\n",cl->activityType, arc);
				}
			}
#endif
            break;

        /*
         * Unsubscribe ACK
         */
        case MT_UNSUBACK:
            ism_common_allocBufferCopyLen(&buf, (char *)bp, buflen);
            break;

        /*
         * Ping request
         */
        case MT_PINGREQ:

        	/* Serverless client or client who talk with SQS */
        	if (!transport->server || transport->server->serverKind==2){
        		sendit = 0;
				char pbuf[16];
				rc = transport->send(transport, pbuf+16, 0, MT_PINGRESP << 4,
						SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
        		break;
        	}
            if (pobj->client_transport) {
                /* If this is a pingreq from the server, send it back to the server */
                sendit = 0;
                char pbuf[16];
                rc = transport->send(transport, pbuf+16, 0, MT_PINGRESP << 4,
                        SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
            } else {
                if (buflen != 0) {
                    rc = ISMRC_BadLength;
                    ism_common_setErrorData(rc, "%s", "PINGREQ");
                    break;
                }
                if (pobj->mqtt_version > 3 && (kind&0x0f)) {
                    /* In v3.1.1 check reserved bits */
                    rc = ISMRC_BadClientData;
                    ism_common_setErrorData(rc, "%s", "PINGREQ");
                    break;
                }
            }
            break;

        /*
         * Ping response.
         */
        case MT_PINGRESP:
            if (!pobj->client_transport) {
                rc = ISMRC_InvalidCommand; /* Invalid command */
                ism_common_setError(rc);
                break;
            }
            if (transport->server->mqttProxyProtocol && (uint8_t)kind == 0xDF) {
                int extlen = BIGINT16(bp);
                bp += extlen+2;
                buflen -= 2;
                if (buflen >= (extlen+2)) {
                    buflen -= extlen;
                } else {
                    buflen = 0;
                }
                checkSubs((const char *)bp, buflen, transport, pobj->client_transport, 0, 1);
                break;

            } else if (transport->server->mqttProxyProtocol && (uint8_t)kind == MT_GW_PINGRES) {
                TRACE(7, "MT_GW_PINGRES: connect=%u client=%s version=%u\n",
                        transport->index, transport->name, pobj->mqttpx_version);
                break;//No need to send back to client.

            }
            ism_common_allocBufferCopyLen(&buf, (const char *)bp, buflen);
            break;

        /*
         * Connect ACK.  There are extra return codes in proxy protocol
         */
        case MT_CONNACK:
            /*
             * We have now completed connection retry
             */
            if (!pobj->client_transport) {
                rc = ISMRC_InvalidCommand; /* Invalid command */
                ism_common_setError(rc);
                break;
            }
            if (!g_useMux) {
                /* Remember whether this connection is from the primary or backup  */
                ism_server_setLastGoodAddress(transport->server, transport->connect_order);
            }

            ism_transport_t * ctransport = pobj->client_transport;
            mqttProtoObj_t * cpobj = ctransport->pobj;

            /*
             * Handle proxy protocol or mqttv5 extra error info
             */
            memset(&mmsg, 0, sizeof mmsg);
            mmsg.transport = transport;
            int  xrc = 0;
            if (buflen >= 2) {
                xrc = bp[1];    /* MQTT RC */
                if (buflen > 2) {
                    char * ext;
                    int    extlen;
                    bp += 2;
                    buflen -= 2;
                    if (buflen > 0 && pobj->mqtt_version >= 5) {
                        if (*bp == 0) {
                            bp++;
                            buflen--;
                        } else {
                            int vilen;
                            int mproplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
                            bp += vilen;
                            buflen -= vilen;
                            if (mproplen < 0 || mproplen > buflen) {
                                xrc = ISMRC_BadLength;
                                ism_common_setErrorData(xrc, "%s%s", "CONNACK", "PropLen");
                            } else {
                                xrc = ism_common_checkMqttPropFields((char *)bp, mproplen,
                                    g_ctx5, CPOI_CONNACK, mpropCheck, &mmsg);
                            }
                            bp += mproplen;    /* Skip past extensions */
                            buflen -= mproplen;
                        }
                    }
                    if (buflen) {
                        int  sublen;
                        ext = (char *)bp;
                        extlen = buflen;
                        pobj->mqttpx_version = ism_common_getExtensionValue(ext, extlen, EXIV_ProxyProtLevel, 4);
                        TRACE(7, "Set MQTTpx version: connect=%u client=%s version=%u\n",
                                transport->index, transport->name, pobj->mqttpx_version);
                        rc = ism_common_getExtensionValue(ext, extlen, EXIV_ServerRC, rc);
                        if (!mmsg.reason) {
                            mmsg.reason = ism_common_getExtensionString(ext, extlen, EXIV_ReasonString, NULL);
                        }

                        const char * subs = ism_common_getExtensionByteArray(ext, extlen, EXIV_Subscriptions, &sublen);

                        if (subs) {
                            /* Unsubscribe as required */
                            checkSubs(subs, sublen, transport, pobj->client_transport, 1, 1);
                        }

                    }
                    if (mmsg.reason) {
                        TRACE(5, "Unable to connect to server: connect=%u client=%s, rc=%u reason=%s\n",
                                transport->index, transport->name, rc, mmsg.reason);
                    }
                }
                if (pobj->mqtt_version >= 5) {
                } else {
                    if (xrc >= CRC_MaxConnectRc) {
                        if (rc == 0)
                            rc = xrc;
                        ism_common_setError(rc);
                        sendit = 0;
                    } else {
                        if (xrc) {
                            rc = xrc;
                            ism_common_setError(rc);
                        }
                    }
                }
            } else {
                rc = ISMRC_BadLength;
                ism_common_setError(rc);
                sendit = 0;
            }

            /*
             * If we are sending the CONNACK with RC=0 we have a good connection
             */
            TRACE(7, "CONNACK connect=%u rc=%d clientid=%s inprogress=%d\n",
                    transport->index, rc, transport->name, transport->pobj->inprogress);
            if (LIKELY(rc == 0)) {
                transport->ready = 3;   /* Server connection ready in normal state */

                if (cpobj->connectlen) {
                    cpobj->connectlen = 0;
                    cpobj->end_ext_loc = 0;
                    ism_common_free(ism_memory_proxy_mqtt_connection,cpobj->connectdata);
                    cpobj->connectdata = NULL;
                }
                /* Send ACLs to the server */
                if (ctransport->has_acl) {
                    const char * aclKey = ism_proxy_getACLKey(ctransport);
                    TRACE(6, "Send ACL to the server connect=%u acl=%s\n", transport->index, aclKey);
                    ism_acl_t * acl = ism_protocol_findACL(aclKey, 0, NULL);
                    if (acl) {
                        TRACE(8, "Add transport to ACL: connect=%u name=%s was=%p\n",
                                transport->index, aclKey, acl->object);
                        acl->object = ctransport;
                        ism_protocol_unlockACL(acl);
                    }
                    sendACLs(ctransport);
                }
                //Process until savelen is zero.
                //savedata can be used while revalidate happen. Process until we done.
                while (cpobj->savelen) {
                    cpobj->savedataprocessing=1;
                    int sendlen = cpobj->savelen;
                    char * senddata = cpobj->savedata;
                    int savecount=cpobj->savecount;

                    cpobj->savelen = 0;
                    cpobj->savedata = NULL;
                    cpobj->savecount=0;

                    TRACE(8, "CONNACK: revalidate send saved data connect=%u clientid=%s senddata=%p sendlen=%d savecount=%d\n", transport->index,
                                               transport->name, senddata, sendlen, savecount);

                    rc = revalidateAndSendSavedData(ctransport, senddata, sendlen, savecount);

                    ism_common_free(ism_memory_proxy_mqtt_savedata, senddata);
                    //If error during the buffer validation.
                    if (rc!=ISMRC_OK){
                        break; //break from the while loop
                    }
                }
                cpobj->savedataprocessing=0;

                if (cpobj->pendingRC) {
                    ctransport->close(ctransport, mapToIsmRC(cpobj->pendingRC, 5), 0, ctransport->reason);
                    break;
                }
                if(rc != ISMRC_OK){
                    break; //break from switch statement due revalidate savedBuffer error.
                }

                pobj->client_transport->ready = 3;
                pobj->send_disconnect = 2;
                if (!ctransport->no_monitor)
                    ism_proxy_connectMsg(ctransport);
                if (g_metering_interval && ctransport->client_class) {
                    ctransport->metering_time = transport->connect_time + (g_metering_delta/2);
                }
#ifdef DEBUG_INPROGRESS
                TRACE(9, "Decrement inprogress on CONNACK send: connect=%u inprogress=%d\n", ctransport->index, ctransport->pobj->inprogress);
#endif
                if (UNLIKELY(__sync_sub_and_fetch(&ctransport->pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
                    TRACEL(7, ctransport->trclevel, "Complete close: connect=%u client=%s client_connect=%u\n",
                        transport->index, transport->name, ctransport->index);
                    ism_mqtt_doneConnection(ctransport);
                    break;
                }

#ifndef NO_PXACT
                // PXACT
                if (ism_pxact_track_client(ctransport, PXACT_ACTIVITY_TYPE_MQTT_CONN))
                {
                    int arc;
                    ismPXACT_ConnectivityInfo_t ci[1];
                    if (!ctransport->aobj ) ism_pxact_alloc_pxclient(ctransport);
                    ismPXACT_Client_t *cl = (ismPXACT_Client_t *)ctransport->aobj;
                    cl->activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
                    memset(ci,0,sizeof(ismPXACT_ConnectivityInfo_t));
                    ci->connectionID = ism_transport_getConnId(ctransport);
                    switch (cpobj->mqtt_version) {
                        case 3 : ci->protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1 ; break ;
                        case 4 : ci->protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1_1 ; break ;
                        case 5 : ci->protocol = PXACT_CONN_PROTO_TYPE_MQTT_v5 ; break ;
                        default: ci->protocol = PXACT_CONN_PROTO_TYPE_NONE ; break ;
                    }
                    ci->expiryIntervalSec = 0; /* TODO */
                    ci->cleanS = cpobj->cleansession;
                    ci->targetServerType = PXACT_TRG_SERVER_TYPE_MS;
                    ci->connectTime = ctransport->connect_time;
                    TRACEL(9, ism_defaultTrace, "Calling ism_pxactivity_Client_Connectivity with actType %d for cid %s, org %s devType %s devId %s cType %d connId %lu\n",
                          cl->activityType, cl->pClientID, cl->pOrgName, cl->pDeviceType, cl->pDeviceID, cl->clientType, ci->connectionID);
                    arc = ism_pxactivity_Client_Connectivity(cl, ci);
                    if ( arc != ISMRC_OK && arc != ISMRC_Closed ) {
                        TRACEL(1, ism_defaultTrace, "ism_pxactivity_Client_Connectivity failed, rc= %d\n",arc);
                    }
                }
#endif
            } else {
                transport->closestate[3] = rc;
#ifdef DEBUG_INPROGRESS
                TRACE(9, "Decrement inprogress on CONNACK send: connect=%u inprogress=%d rc=%d\n", ctransport->index, ctransport->pobj->inprogress, rc);
#endif
                if (UNLIKELY(__sync_sub_and_fetch(&ctransport->pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
                    TRACEL(7, ctransport->trclevel, "Complete close: connect=%u client=%s client_connect=%u rc=%d\n",
                        transport->index, transport->name, ctransport->index, rc);
                    ism_mqtt_doneConnection(ctransport);
                    break;
                }
            }


            ism_common_allocBufferCopyLen(&buf, (char *)inbuf, 2);   /* Copy flags and rc */
            if (pobj->mqtt_version >= 5) {
                if (xrc) {
                    if (mmsg.reason) {
                        int reason_len = strlen(mmsg.reason);
                        ism_common_putMqttVarInt(&buf, reason_len+3);
                        ism_common_putMqttPropString(&buf, MPI_Reason, g_ctx5, mmsg.reason, reason_len);
                    } else {
                        bputchar(&buf, 0);   /* properties */
                    }
                } else {
                    int pos;
                    ctransport = pobj->client_transport;
                    bputchar(&buf, 0);
                    pos = buf.used;

                    ism_common_putMqttPropField(&buf, MPI_MaxTopicAlias, g_ctx5, pobj->client_transport->pobj->topicalias_count_out);
                    if (mmsg.isExpire || (ctransport && ctransport->pobj->expiryOriginal != ctransport->pobj->expiryRequested))
                        ism_common_putMqttPropField(&buf, MPI_SessionExpire, g_ctx5, mmsg.isExpire ? mmsg.expireTTL : ctransport->pobj->expiryRequested);
                    if (mmsg.maxActive)
                        ism_common_putMqttPropField(&buf, MPI_MaxReceive, g_ctx5, mmsg.maxActive);
                    if (pobj->client_transport->maxMsgSize)
                        ism_common_putMqttPropField(&buf, MPI_MaxPacketSize, g_ctx5, pobj->client_transport->maxMsgSize);
                    if (pobj->send_keepalive || (mmsg.keepalive > 0))
                        ism_common_putMqttPropField(&buf, MPI_KeepAlive, g_ctx5, mmsg.keepalive);
                    buf.buf[pos-1] = buf.used-pos;    /* Always less than 128 */
                }
            }
            break;

        /*
         * Connect.  This must be the first control packet in the sequence
         */
        case MT_CONNECT:
            /* Parse the connect packet */
            sendit = 0;         /* We save the text for when the connection is established */
            decrement = 0;      /* We decrement when the outgoing connection is established */
            memset(&mmsg, 0, sizeof mmsg);
            mmsg.command = command;
            mmsg.kind = (uint8_t)kind;
            mmsg.transport = transport;
            rc = handleConnectRequest(transport, &mmsg, bp, buflen, kind, &buf);
            break;

        /*
         * Disconnect.
         * In MQTTv3.1.1 this is only client to server.
         * In MQTTv5 this can be sent server to client.
         * In proxy protocol at any level it can be send server to proxy.
         */
        case MT_DISCONNECT:
            /* Server to proxy direction */
            if (transport->pobj->client_transport) {
                char * closemsg = NULL;
                rc = ISMRC_ClosedByServer;

                if (transport->pobj->mqtt_version >= 5) {
                    if (buflen > 0)
                        rc = mapToIsmRC(*bp, pobj->mqtt_version);
                    if (buflen > 1) {
                        int rlen;
                        const char * reason;
                        concat_alloc_t propbuf = {(char *)bp, buflen, buflen, 1};
                        int proplen = ism_common_getMqttVarInt(&propbuf);
                        if (proplen > 0) {
                            bp += propbuf.pos;   /* Skip over property length */
                            reason = ism_common_getMqttPropString((char *)bp, proplen, MPI_Reason, g_ctx5, &rlen);
                            if (reason && rlen > 0) {
                                closemsg = alloca(rlen + 1);
                                memcpy(closemsg, reason, rlen);
                                closemsg[rlen] = 0;
                            }
                        }
                    }
                } else if (g_useMQTTpx) {
                    /* If using proxy protocol, use that rather than the MQTTv5 info */
                    if (buflen > 2) {
                        rc = ism_common_getExtensionValue((const char *)bp+2, buflen-2, EXIV_ServerRC, rc);
                        closemsg = (char *)ism_common_getExtensionString((const char *)bp+2, buflen-2, EXIV_ReasonString, NULL);
                    }
                }

                if (closemsg == NULL && rc) {
                    closemsg = alloca(256);
                    ism_common_getErrorString(rc, closemsg, 256);
                }
                TRACE(6, "Receive DISCONNECT from server: connect=%u client=%s rc=%u reason=%s\n",
                        transport->index, transport->name, rc, closemsg ? closemsg : "");
                transport->close(transport, rc, rc==0, closemsg);
                rc = 0;
                sendit = 0;
            }

            /* Client to proxy */
            else {
                ism_common_allocBufferCopyLen(&buf, (char *)bp, buflen);
                transport->closestate[3] = 1;           /* Protocol disconnected */
                if (pobj->server_transport && pobj->server_transport->pobj) {
                	pobj->server_transport->pobj->send_disconnect = 0;
                }
                pobj->send_disconnect = 0;
                rc = 0;
                if (buflen > 0 && pobj->mqtt_version >= 5) {
                    transport->closestate[3] = *bp;   /* record return code */
                    if (buflen > 2) {
                        int vilen;
                        int rlen;
                        char * closemsg;
                        int proplen;
                        pobj->pendingRC = *bp++;
                        proplen = ism_common_getMqttVarIntExp((char *)bp, buflen, &vilen);
                        bp += vilen;
                        const char * reason = ism_common_getMqttPropString((char *)bp, proplen, MPI_Reason, g_ctx5, &rlen);
                        if (reason && rlen > 0) {
                            closemsg = alloca(rlen + 1);
                            memcpy(closemsg, reason, rlen);
                            closemsg[rlen] = 0;
                        } else {
                            int myrc = mapToIsmRC(pobj->pendingRC, 5);
                            if (myrc == 0)
                                closemsg = "The connection has completed normally";
                            else
                                closemsg = (char *)ism_common_getErrorString(myrc, xbuf+2048, sizeof(xbuf)-2048);
                        }
                        transport->reason = ism_transport_putString(transport, closemsg);
                    }
                }
                /* If in async wait, do not send DISCONNECT but mark it to be sent later */
#ifdef DEBUG_INPROGRESS
                TRACE(9, "Decrement inprogress: connect=%u inprogress=%d\n", transport->index, pobj->inprogress);
#endif
                if (__sync_sub_and_fetch(&pobj->inprogress, 0) > 1) {
                    sendit = 0;
                    __sync_bool_compare_and_swap(&pobj->disconnect_pending, 0, 1);
                }
            }
#ifndef NO_PXACT
            // PXACT
            {
				ctransport = transport->pobj->client_transport ? transport->pobj->client_transport : transport ;
				PXACT_ACTIVITY_TYPE_t actType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
                if(transport->pobj->client_transport)
                    actType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_SERVER;
                if (ism_pxact_track_client(ctransport, actType)) {
					if (!ctransport->aobj ) ism_pxact_alloc_pxclient(ctransport);
					ismPXACT_Client_t *cl = (ismPXACT_Client_t *)ctransport->aobj ;
					cl->activityType = actType;
				}
			}
#endif
            break;

        case MT_AUTH:
            if (pobj->mqtt_version >= 5) {
                rc = ISMRC_NotAuthorized;
                ism_common_setErrorData(rc, "%s", "AUTH");
                break;
            }
            /* Fall thru for version < 5 */

        /*
         * All other packets are an error
         */
        default:
            rc = ISMRC_InvalidCommand; /* Invalid command */
            ism_common_setError(rc);
            break;
        }
    }

    /*
     * If we detect an error disconnect.
     * It this is a connection request, we have already handled the error.
     */
    if (UNLIKELY(rc >= ISMRC_Error)) {
        TRACE(5, "MQTT error: rc=%s (%d) command=%s connect=%u inprogress=%d\n",
                getMQTTErrorString(rc, xbuf, sizeof xbuf), rc, ism_mqtt_mqttCommand(command), transport->index, pobj->inprogress);
#ifdef DEBUG_INPROGRESS
        TRACE(9, "Decrement inprogress: connect=%u inprogress=%d\n", transport->index, pobj->inprogress);
#endif
        if (__sync_sub_and_fetch(&pobj->inprogress, 1) < 0) { /* BEAM suppression: constant condition */
            ism_mqtt_doneConnection(transport);
        } else {
            ism_common_formatLastError(xbuf, sizeof xbuf);
            transport->close(transport, rc, 0, xbuf);
        }
        if (command == MT_PUBLISH) {
            transport->endpoint->stats->count[transport->tid].lost_msg++;
        }
        ism_common_freeAllocBuffer(&buf);
        return rc;
    }

    /*
     * Update the last access time (except for a disconnect)
     */
    if (__builtin_expect((command != MT_DISCONNECT), 1)) {
        pobj->messageCount++;
        transport->lastAccessTime = ism_common_readTSC();
    }
    if (LIKELY(sendit != 0)) {
        if (transport->originated) {
        		ism_transport_t *client_transport=pobj->client_transport;
            if (client_transport){
            		int limitResult = checkDiscardMsgsLimit(client_transport);
            		if(limitResult==ISMRC_OK){
            			rc = client_transport->send(client_transport, buf.buf+16, buf.used-16, kind, SFLAG_FRAMESPACE);

            			if(command == MT_PUBLISH){
                        __sync_add_and_fetch(&mqttStats.mqttC2PMsgsSent, 1);
                        if (pobj->client_transport)
                                pobj->client_transport->write_msg++;
            			}

            		}else{
            			//Hard Limit is reached. Close the Connection
            			 transport->close(transport, ISMRC_SendQBytesLimitReached, 0, "Exceeded quota for buffered data (slow consumer)");
            		}
            }else{
                rc = 0;
            }
        } else {
            if (transport->tenant->serverless) {
                switch (command) {
                case MT_PUBLISH:
                    rc = routeMhubMessage(transport, kind, buf.buf+16, buf.used-16);
                    break;
                case MT_PUBREL:
                    memcpy(xbuf+16, bp, 2);
                    transport->send(transport, xbuf+16, 2, MT_PUBCOMP<<4, SFLAG_FRAMESPACE);
                    break;
                }
            } else {
                int sendMQTT=g_msgRoutingDefaultSendMQTT;
                if (command == MT_PUBLISH){
                    //Route to Kafka. Only if CONNACK is done.
                    //connectdata should NULL and savedata should be NULL
                    //when CONNACK completed successfully
                    if (LIKELY(pobj->connectdata == NULL && pobj->savedata == NULL && !pobj->savedataprocessing)) {
                        ism_route_routeMessage(transport, buf.buf+16, buf.used-16, kind,  &sendMQTT);
                    }
                } else {
                    sendMQTT=1;
                }
                if(sendMQTT){
                    if (LIKELY(pobj->connectdata == NULL && pobj->savedata == NULL && !pobj->savedataprocessing)) {
                        if (LIKELY(pobj->server_transport != NULL)) {

                            rc = pobj->server_transport->send(pobj->server_transport, buf.buf+16, buf.used-16, kind, SFLAG_FRAMESPACE);

                            if(command == MT_PUBLISH){
                                __sync_add_and_fetch(&mqttStats.mqttP2SMsgsSent, 1);
                                if (pobj->server_transport)
                                    pobj->server_transport->write_msg++;
                            }

                        }
                        if (pobj->pendingRC) {
                            transport->close(transport, mapToIsmRC(pobj->pendingRC, 5), 0, transport->reason);
                        }
                    } else {
                        rc = 0;
                        TRACE(9, "Append SavedData: connect=%d len=%d command=%d\n", transport->index, buf.used-16, command);
                        appendSavedData(transport,buf.buf+16, buf.used-16, kind);
                    }
                }
            }
        }
    }
    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);

    /*
     * If close is in progress, proceed with it.
     * For connect we wait to do this until the connection is established
     */
    if (LIKELY(decrement)) {
        int inprogress;
#ifdef DEBUG_INPROGRESS
        TRACE(9, "Decrement inprogress: command=%s connect=%u inprogress=%d\n",
                ism_mqtt_mqttCommand(kind), transport->index, pobj->inprogress);
#endif
        inprogress = __sync_sub_and_fetch(&pobj->inprogress, 1);
        if (inprogress <= 0) {
            if (inprogress < 0) {
                TRACE(7, "Continue close of connection: connect=%d client=%s\n", transport->index, transport->name);
                ism_mqtt_doneConnection(transport);
            } else {
                if (pobj->server_transport) {
                    if (__sync_bool_compare_and_swap(&pobj->disconnect_pending, 1, 0)) {
                        char dbuf[16];
                        if (LIKELY(pobj->server_transport != NULL)) {
                            pobj->server_transport->send(pobj->server_transport, dbuf+16, 0, MT_DISCONNECT << 4, SFLAG_FRAMESPACE);
                        }
                    }
                }
            }
        }
    }
    return throttle ? -9 : rc;
}


/*
 * Check if the wildcards are valid for an MQTT topic filter.
 */
static int validWild(const char * s, int len) {
    int  i;
    for (i = 0; i < len; i++) {
        if (s[i] == '#') {
            if ((i > 0 && s[i - 1] != '/') || (i + 1 != len)) {
                return 0;
            }
        } else if (s[i] == '+') {
            if ((i > 0 && s[i - 1] != '/') || (i + 1 != len && s[i + 1] != '/')) {
                return 0;
            }
        }
    }
    return 1;
}


/*
 * Send the ACLs to the server.
 * For now we just send it as a single packet.  We might later want to break this up.
 */
static int sendACLs(ism_transport_t * transport) {
    char xbuf [2048];
    ism_acl_t * acl;
    const char * aclKey;
    ism_transport_t * stransport = transport->pobj->server_transport;
    concat_alloc_t buf = {xbuf, sizeof xbuf, 19};

    xbuf[16] = 0;
    xbuf[17] = 1;
    xbuf[18] = EXIV_EndExtension;
    
    aclKey = ism_proxy_getACLKey(transport);
    acl = ism_protocol_findACL(aclKey, 0, NULL);
    if (acl) {
        ism_protocol_getACL(&buf, acl);
        ism_protocol_unlockACL(acl);
        transport->has_acl = 1;
    } else {
        bputchar(&buf, '!');
        ism_json_putBytes(&buf, aclKey);
        transport->has_acl = 0;
    }
    stransport->send(stransport, buf.buf+16, buf.used-16, MT_SENDACL, SFLAG_FRAMESPACE);
    if (buf.inheap)
            ism_common_freeAllocBuffer(&buf);
    return 0;
}

/**
 * Send PINGREQ for Gateway when PUBLISH failed
 */
static int sendGWPINGREQ(ism_transport_t * transport) {
    char xbuf [2048];
    if(g_sendGWPingREQ){

        ism_transport_t * stransport = transport->pobj->server_transport;
        concat_alloc_t buf = {xbuf, sizeof xbuf, 16};

        TRACE(7, "Sending PINGREG for Gateway: connect=%u client=%s\n",transport->index, transport->name);
        stransport->send(stransport, buf.buf+16, buf.used-16, MT_GW_PINGREG, SFLAG_FRAMESPACE);
        if (buf.inheap)
                ism_common_freeAllocBuffer(&buf);

    }else{
        TRACE(7, "Sending PINGREG for Gateway is DISABLED: connect=%u client=%s\n",transport->index, transport->name);
    }
    return 0;
}


/*
 * Match a client class rule.
 * @param transport  The transport object which must have the clientID set
 * @param rule       The client class rule
 * @return 1=matched, 0=not matched
 */
static int matchClientClass(ism_transport_t * transport, ism_pxrule_t * rule) {
    int       len;
    uint8_t * rp = rule->rule;
    uint8_t * tp = (uint8_t *)transport->clientID;
    int       left;
    const char * pos;
    char *    varp;

    if (!tp)
        return 0;

    // char xbuf [2048];
    // printf("matchClient rule=[%s]\n", ism_proxy_showRule(rule, xbuf, sizeof xbuf));

    left = (int)strlen((const char *) tp);
    while (*rp != RULEEND) {
       if (*rp < 0xf8) {
           len = *rp++;
           if (left < len || memcmp(tp, rp, len)) {
               return 0;
           }
           left -= len;
           tp += len;
           rp += len;
       } else if (*rp == RULEVAR) {
           rp++;
           int var = *rp++;
           if (*rp == RULEEND) {
               pos = (const char *)(tp + left);
           } else {
               pos = strchr((const char *)tp, rp[1]);
               if (!pos) {
                   // printf("failed at %d\n", __LINE__);
                   return 0;
               }
           }
           len = pos-(const char *)tp;
           /* Empty value for the variable. return false.*/
           if (len<1)
        	   return 0;
           varp = ism_transport_allocBytes(transport, len+1, 0);
           memcpy(varp, tp, len);
           varp[len] = 0;
           switch (var) {
           case RULEVAR_Org:  transport->org = varp;       break;
           case RULEVAR_Id:   transport->deviceID = varp;  break;
           case RULEVAR_Type: transport->typeID = varp;    break;
           }
           left -= len;
           tp += len;
       } else {
           rp += 2;
       }
    }

    if (rule->endrule == RULEEND_None && left ) {
        return 0;
    }
    return 1;
}


/*
 * Match a topic rule
 *
 * @return 0=not matched, 1=matched
 */
static int matchTopicRule(ism_transport_t * transport, ism_pxrule_t * rule, const char * topic, int topic_len) {
    int       len;
    uint8_t * rp = rule->rule;
    uint8_t * tp = (uint8_t *)topic;
    int       left = topic_len;
    const char * value;
    const char * pos;

    // char xbuf [2048];
    // printf("match topic: rule=[%s] topic=[%s]\n", ism_proxy_showRule(rule, xbuf, sizeof xbuf), topic);
    if (g_AAAEnabled) {
		if ((transport->auth_permissions & rule->auth) != rule->auth) {
			return 0;
		}
    }


    while (*rp != RULEEND) {
       if (*rp < 0xf8) {
           len = *rp++;
           if (left < len || memcmp(tp, rp, len)) {
               // printf("failed at: %d\n", __LINE__);
               return 0;
           }
           left -= len;
           tp += len;
           rp += len;
       } else if (*rp == RULEVAR) {
           value = "";
           switch (*++rp) {
           case RULEVAR_Org:  value = transport->tenant ? transport->tenant->name : NULL;   break;
           case RULEVAR_Id:   value = transport->deviceID;                                  break;
           case RULEVAR_Type: value = transport->typeID;                                    break;
           }
           if (value == NULL) {
               // printf("failed at: %d\n", __LINE__);
               return 0;
           }
           len = strlen(value);
           if (left < len || memcmp(tp, value, len)) {
               // printf("failed at: %d\n", __LINE__);
               return 0;
           }
           left -= len;
           tp += len;
           rp++;
       } else if (*rp == RULEWILD) {
           rp++;
           pos = memchr(tp, '/', left);
           if (pos)
               len = pos - (const char *)tp;
           else
               len = left;
           if (len == 0 && *rp != RULEWILD_Any) {
               // printf("failed at: %d\n", __LINE__);
               return 0;
           }
           if (*rp == RULEWILD_NoWild && (*tp == '+' || *tp == '#')) {
               // printf("failed at: %d\n", __LINE__);
               return 0;
           }
           if (*tp == '#' && *rp != RULEWILD_Any) {
				 // printf("failed at: %d\n", __LINE__);
				 return 0;
           }
           left -= len;
           tp += len;
           rp++;
       }
    }
    if (rule->endrule == RULEEND_None && left ) {
        // printf("failed at: %d\n", __LINE__);
        return 0;
    }

    return 1;
}


/*
 * Find the nth occurrence of a character in a memory region.
 * @param str   The memory region to look in
 * @param ch    The character to look for
 * @param len   The length of the memory region
 * @param cnt   The count of occurrences.  If 0 the start of the string is returned.
 * @return the position of the nth occurrence of the chracter, or NULL to say there
 *    are less then cnt occurrences of the character.
 */
static const char * memchrcnt(const char * str, char ch, int len, int cnt) {
    const char * endloc;
    if (cnt <= 0)
        return str;
    endloc = str+len;
    while (str < endloc) {
        if (*str == ch) {
            if (--cnt == 0)
                return str;
        }
        str++;
    }
    return NULL;
}


/*
 * Replace any instance of a character in the source string with another character;
 */
static char *char_replace (char *s, char c, char r)  {
     char * p1;
     for (p1=s; *p1; p1++) {
        if (*p1 == c) {
           *p1 = r;
        }
     }
     return s;
}


/*
 * Convert the topic to a MessageSight shared subscription topic name
 */
static int makeSharedSubcriptionTopic(ism_transport_t * transport, concat_alloc_t * buf,
        const char * otopic, int otopic_len, char * cvttopic, int cvttopic_len) {
    /*
     * New shared format: $share/org:appID/filter
     */
    if (transport->typeID[0]) {
        ism_common_allocBufferCopyLen(buf, "$share/", 7);
        ism_common_allocBufferCopyLen(buf, transport->org, strlen(transport->org));            /* orgID */
        bputchar(buf, ':');
        ism_common_allocBufferCopyLen(buf, transport->deviceID, strlen(transport->deviceID));  /* appID */
    }

    /*
     * Old shared format: $SharedSubscription/org:appID/modified_filter/filter
     */
    else {
        ism_common_allocBufferCopyLen(buf, "$SharedSubscription/", 20);
        ism_common_allocBufferCopyLen(buf, transport->org, strlen(transport->org));
        bputchar(buf, ':');
        ism_common_allocBufferCopyLen(buf, transport->deviceID, strlen(transport->deviceID));
        bputchar(buf, ':');
        char * tmp_otopic = alloca(otopic_len+1);
        memcpy(tmp_otopic, otopic, otopic_len);
        tmp_otopic[otopic_len]='\0';
        char_replace(tmp_otopic, '/', ':');
        char_replace(tmp_otopic, '+', '&');
        char_replace(tmp_otopic, '#', '%');
        ism_common_allocBufferCopyLen(buf,tmp_otopic, strlen(tmp_otopic));
    }
    bputchar(buf, '/');
    ism_common_allocBufferCopyLen(buf, cvttopic, cvttopic_len);
    bputchar(buf, 0);    /* Zero terminate for trace */
    buf->used--;
	return 0;
}


/*
 * Convert iotdm-1/# to full form of iotdm-1/type/+/id/+/#
 * This is needed to fix the issue for GW and still maintain
 * the compability that devices uses the topic out there. 
 * This can be accomplished also by changing the topic rule. 
 * However, it will break any GW devices that use the topic 
 * currently. 
 * 
 * 
 */
static int convertIOTDMHashTopic(ism_transport_t * transport, concat_alloc_t * buf,
        const char * otopic, int otopic_len) {

	int rc=0;
	int prefix_len=0;
	int i=0;
	int count=0;
	char *tTopic=NULL;
	char *  subStr =NULL;
	int     subStr_len=0;
	int len = otopic_len;

	char * topicStr = alloca(len+1);
	memcpy(topicStr, otopic, len);
	topicStr[len]='\0';

	//Found how many slashes
	for (i = 8; topicStr[i] != '\0'; i++) {
			if (topicStr[i] == '/'){
					count++;
					tTopic=topicStr+i;
			}
	}

	if(tTopic)
			prefix_len = (tTopic - topicStr)+1;
	else
			prefix_len = 8;


	switch(count){
		case 1:
				subStr = "type/+/id/+/";
				break;
		case 2:
				subStr = "+/id/+/";
				break;

		case 3:
				subStr = "id/+/";
					break;
		case 4:
			subStr = "+/";
		   break;
   }
   if(subStr){
		   subStr_len = strlen(subStr);
		   ism_common_allocBufferCopyLen(buf, topicStr, prefix_len);
		   ism_common_allocBufferCopyLen(buf, subStr, subStr_len);
		   ism_common_allocBufferCopyLen(buf, "#", 1);
		   rc =1;
   }
   return rc;
}


/*
 * Change and check a topic.
 *
 */
static int convertTopic(ism_transport_t * transport, concat_alloc_t * buf,
        const char * topic, int topic_len, int subscribe) {
    char xbuf [2048];
    ism_tenant_t * tenant = transport->tenant;
    int rc = 0;
    int len;
    ism_topicrule_t * topicrule = ism_proxy_getTopicRuleClass(transport->tenant, transport->client_class);
    int   check = 1;
    ism_pxrule_t * convert_rule = NULL;
    char * topic_copy;
    const char * tp = topic;
    int   tplen = topic_len;
    const char * value;
    char * cp;
    int savelen;
    int mode;
    const char * reason;
    int orig_topic_len=topic_len;

    savelen = buf->used;
    bputchar(buf, (char)(topic_len>>8));
    bputchar(buf, (char)topic_len);
    ism_common_allocBufferCopyLen(buf, topic, topic_len);
    bputchar(buf, 0);
    buf->used--;
    topic_copy = buf->buf + savelen  + 2;   /* Now known to be null terminated */

    /*
     * Sanity check
     */
    mode = (UR_NoControl | UR_NoNonchar);
    if (subscribe) {
        if (topicrule && topicrule->nohash)
            mode |= UR_NoHash;
    } else {
        mode |= UR_NoWildcard;
    }

    if (ism_common_validUTF8Restrict(topic, topic_len, mode) < 1) {
        /* Fix up the name so we can put it into the logs */
        if (ism_common_validUTF8(topic, topic_len) < 0) {
            cp = topic_copy;
            while (*cp) {
                if (((uint8_t)*cp) < 0x20 || ((uint8_t)*cp) >0x7e)
                    *cp = '?';
                cp++;
            }
            TRACE(5, "A topic is not valid UTF-8 or contains control characters: topic=\"%s\" tenant=\"%s\" clientID=\"%s\" From=%s:%u, connect=%u\n",
                    topic_copy, tenant->name, transport->name, transport->client_addr, transport->clientport, transport->index);
            reason = "The topic is not valid Unicode";
        } else {
            if (topic_len == 0) {
                TRACE(5, "A topic is empty which is not allowed: tenant=\"%s\" clientID=\"%s\" From=%s:%u, connect=%u\n",
                        tenant->name, transport->name, transport->client_addr, transport->clientport, transport->index);
                reason = "The topic string is empty";
            } else {
                cp = topic_copy;
                while (*cp) {
                    if (((uint8_t)*cp) < 0x20)
                        *cp = '?';
                    cp++;
                }
                TRACE(5, "A topic contains characters which are not allowed: topic=\"%s\" tenant=\"%s\" clientID=\"%s\" From=%s:%u, connect=%u\n",
                    topic_copy, tenant->name, transport->name, transport->client_addr, transport->clientport, transport->index);
                reason = "The topic contains characters which are not allowed";
            }
        }
        rc = ISMRC_BadTopic;
    } else {
        if (subscribe && !validWild(topic, topic_len)) {
            TRACE(5, "A topic contains a wildcard where is it not valid: topic=\"%s\" tenant=\"%s\" clientID=\"%s\" From=%s:%u, connect=%u\n",
                    topic_copy, tenant->name, transport->name, transport->client_addr, transport->clientport, transport->index);
            reason = "The topic contains a wildcard which is not valid";
            rc = ISMRC_BadTopic;
        }
    }

    /* Copy the length which will be overridden if we change it */
    if (rc == 0 && *topic == '$') {
        tp = topic;
        tplen = topic_len;
        if (topic_len > 21 && !memcmp(tp, "$SharedSubscription/", 20)) {
            if (tenant->allow_shared != 1) {
                rc = ISMRC_BadTopic;
                ((char *)tp)[tplen] = 0;
                TRACE(5, "A shared topic is not authorized for this tenant: topic=\"%s\" tenant=\"%s\" clientID=\"%s\" client_addr=%s, connect=%u\n",
                        topic_copy, tenant->name, transport->name, transport->client_addr, transport->index);
                reason = "The shared topic is not authorized";
                ism_common_setErrorData(rc, "%s", reason);
            } else {
                tp = memchr(tp+20, '/', topic_len-20);
                if (tp) {
                    tp++;
                    tplen -= (tp-topic);
                }
                if (!tp || (tplen > 0 && *tp == '$')) {
                    rc = ISMRC_BadTopic;
                    if (!tp)
                        tp = "";
                    else
                        ((char *)tp)[tplen] = 0;
                    TRACE(5, "The topic is an invalid shared topic: topic=\"%s\" org=\"%s\" clientID=\"%s\" From=%s:%u, connect=%u\n",
                            topic_copy, tenant->name, transport->name, transport->client_addr, transport->clientport, transport->index);
                    reason = "The shared topic is not valid";
                    ism_common_setErrorData(rc, "%s", reason);
                }
            }
        }

        /*
         * Check for $SYS topics
         */
        if (rc == 0 && tplen >= 4 && tp[0]=='$' && tp[1]=='S' && tp[2]=='Y' && tp[3]=='S' ) {
            if (tenant->allow_systopic != 1) {
                rc = ISMRC_BadSysTopic;
                ((char *)tp)[tplen] = 0;
                TRACE(5, "A system topic is not authorized for this tenant: topic=\"%s\" org=\"%s\" clientID=\"%s\" From=%s:%u, connect=%u\n",
                        topic_copy, tenant->name, transport->name, transport->client_addr, transport->clientport, transport->index);
                reason = "A system topic is not allowed";
            }
            check = 0;     /* Do not check syntax */
        }
    }

    /*
     * Run the rules
     */
    if (rc == 0 && check && topicrule) {
        int i;
        int count;
        int matched = 0;
        ism_pxrule_t * * rules;
        if (subscribe) {
            rules = topicrule->subscribe;
            count = topicrule->subscribe_count;
        } else {
            rules = topicrule->publish;
            count = topicrule->publish_count;
        }
        for (i=0; i<count; i++) {
            if (matchTopicRule(transport, rules[i], tp, tplen)) {
                matched = 1;
                convert_rule = topicrule->convert;
                break;
            }
        }
        if (!matched) {
            TRACE(5, "The topic does not match an authorized rule=\"%s\" clientID=\"%s\" From=%s:%u, connect=%u\n",
                    topic_copy, transport->name, transport->client_addr, transport->clientport, transport->index);
            reason = "The topic does not match an authorized rule";
            rc = ISMRC_BadTopic;
        }
    }
    if (rc == 0) {
        if (convert_rule) {
            char * pos = (char *) memchrcnt(tp, '/', tplen, convert_rule->after);
            if (pos) {
                // printf("before convert: [%s] [%s]\n", topic_copy, ism_proxy_showRule(convert_rule, xbuf, sizeof xbuf));
                uint8_t * rp = convert_rule->rule;
                buf->used = savelen + 3 + (pos-topic);   /* Start of topic is already in buffer */
                while (*rp != RULEEND) {
                   if (*rp < 0xf8) {
                       len = *rp++;
                       ism_common_allocBufferCopyLen(buf, (const char *)rp, len);
                       rp += len;
                   } else if (*rp == RULEVAR) {
                       rp++;
                       int var = *rp++;
                       value = NULL;
                       switch (var) {
                       case RULEVAR_Org:  value = transport->org;       break;
                       case RULEVAR_Id:   value = transport->deviceID;  break;
                       case RULEVAR_Type: value = transport->typeID;    break;
                       }
                       if (value) {
                           ism_common_allocBufferCopyLen(buf, value, strlen(value));
                       }
                   } else {
                       rp += 2;
                   }
                }
                ism_common_allocBufferCopyLen(buf, pos+1, topic_len - (pos-topic) - 1);
                bputchar(buf, 0);
                buf->used--;
                //printf("converted topic: [%s]\n", topic_copy);
                topic_len = buf->used - savelen - 2;
                buf->buf[savelen] = (char)(topic_len>>8);
                buf->buf[savelen+1] = (char)topic_len;
            }
        }

        /*
         * For 'A' client class, convert topic to shared subscription
         */
		if (subscribe && transport->clientID && transport->client_class=='A') {
			concat_alloc_t subBuf = {xbuf, sizeof xbuf};
			makeSharedSubcriptionTopic(transport, &subBuf,  topic, orig_topic_len , topic_copy, strlen(topic_copy)  );

			buf->used=savelen+2;
			ism_common_allocBufferCopyLen(buf, subBuf.buf, subBuf.used+1);
			buf->used--;
			topic_len = buf->used - savelen - 2;
			buf->buf[savelen] = (char)(topic_len>>8);
			buf->buf[savelen+1] = (char)topic_len;

			TRACE(8, "Converted SharedSubscription Topic: connect=%u topic=%s\n", transport->index, subBuf.buf);

			if (subBuf.inheap)
			    ism_common_freeAllocBuffer(&subBuf);

		}

		/*
		 * Handle iotdm-1/# topic for subscription for GW client
		 */
		if (subscribe && transport->client_class == 'g' && memcmp(topic,"iotdm-1/",8) == 0 &&
		        topic[orig_topic_len-1]=='#' && topic[orig_topic_len-2]=='/') {
			concat_alloc_t subBuf = {xbuf, sizeof xbuf};
			int xrc = convertIOTDMHashTopic(transport, &subBuf, topic_copy, strlen(topic_copy));

			if(xrc){
				buf->used=savelen+2;
				ism_common_allocBufferCopyLen(buf, subBuf.buf, subBuf.used+1);
				buf->used--;
				topic_len = buf->used - savelen - 2;
				buf->buf[savelen] = (char)(topic_len>>8);
				buf->buf[savelen+1] = (char)topic_len;

				TRACE(8, "iotdm-1/# Topic: connect=%u topic=%s\n", transport->index, subBuf.buf);
			}

			if (subBuf.inheap)
			    ism_common_freeAllocBuffer(&subBuf);
		}
    }

    /*
     * Report an error for a bad topic
     */
    else {
        const char * msg;
        if (g_bigLog) {
            msg = "The topic is not valid: Topic={0} ClientID={1} Connect={2} From={3}:{4} Reason={5}.";
        } else {
            msg = "The topic is not valid: T={0} C={1} I={2} F={3}:{4} R={5}.";
        }
        if (ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Connection), 920, TRACE_DOMAIN, transport->name, transport->client_addr, reason) == 0) {
        	LOG(WARN, Connection, 920, "%-s%-s%u%s%u%-s", msg,
                topic_copy, transport->name, transport->index, transport->client_addr, transport->clientport, reason);
        }
        ism_common_setErrorData(rc, "%s%s", topic_copy, reason);
    }
    return rc;
}



/*
 * Change and check a topic from the server.
 * There are two supported algorithms for change and check and they can be
 * set separately.
 * <br>None - Do nothing
 * <br>IoT1 - Convert according to the IoT1 style (remove tenant name from topic)
 */
static int convertTopicOut(ism_transport_t * transport, concat_alloc_t * buf,
        const char * topic, int topic_len) {
    ism_topicrule_t * topicrule = ism_proxy_getTopicRuleClass(transport->tenant, transport->client_class);
    const char * pos;
    const char * pos2;

    int savelen = buf->used;
    bputchar(buf, (char)(topic_len>>8));
    bputchar(buf, (char)topic_len);
    ism_common_allocBufferCopyLen(buf, topic, topic_len);

    if (topicrule && topicrule->convert && topicrule->remove_count && topic[0] != '$') {
        pos = memchrcnt(topic, '/', topic_len, topicrule->convert->after);
        if (pos) {
            pos++;
            pos2 = memchrcnt(pos, '/', topic_len - (pos-topic), topicrule->remove_count);
            if (pos2) {
                pos2++;
                buf->used = savelen + 2 + (pos-topic);
                ism_common_allocBufferCopyLen(buf, pos2, topic_len - (pos2-topic));
                topic_len = buf->used - savelen - 2;
                buf->buf[savelen] = (char)(topic_len>>8);
                buf->buf[savelen+1] = (char)topic_len;
            }
        }
    }
    bputchar(buf, 0);   /* null terminate */
    buf->used--;
    return 0;
}

/*
 * Parse the binary mqtt CONNECT payload in binary MQTT
 */
static void parseConnectPayload(mqttMsg_t * mmsg) {
    uint8_t * bp = (uint8_t *) mmsg->payload;
    int buflen = mmsg->payload_len;

    /* Parse the client id */
    if (buflen > 1)
        mmsg->clientid_len = BIGINT16(bp);
    bp += 2;
    mmsg->clientid = (char *) bp;
    buflen -= (2 + mmsg->clientid_len);
    bp += mmsg->clientid_len;
    if (buflen >=0 && mmsg->clientid_len > 0) {
        if (ism_common_validUTF8Restrict(mmsg->clientid, mmsg->clientid_len,
                UR_NoC0 | UR_NoWildcard | UR_NoNonchar  | UR_NoSlash) < 1) {
            char * mp = ism_transport_allocBytes(mmsg->transport, mmsg->clientid_len, 0);
            memcpy(mp, mmsg->clientid, mmsg->clientid_len);
            mp[mmsg->clientid_len] = 0;
            mmsg->transport->name = mp;
            mmsg->rc = CRC_BadIdentifier;
            while (*mp) {
                if (((uint8_t)*mp) < 0x20 || ((uint8_t)*mp) >0x7e)
                    *mp = '?';
                mp++;
            }
            TRACEL(5, mmsg->transport->trclevel, "The clientID must be valid UTF-8 and cannot contain slash or characters not allowed in a topic name: "
                    "clientID=\"%s\" From=%s:%u, connect=%u\n",
                    mmsg->transport->name, mmsg->transport->client_addr, mmsg->transport->clientport, mmsg->transport->index);
            ism_common_setErrorData(ISMRC_ClientIDNotValid, "%s", mmsg->transport->name);
            mqttConnectError(mmsg);
            return;
        }
    }

    /* Parse the will topic and message */
    if (mmsg->flags & CFLAG_Will) {
        /* Will Properties */
        if (mmsg->version >= 5) {
            if (buflen > 0) {
                int wproplen = *bp;
                if (*bp) {
                    if (wproplen & 0x80) {
                        int vilen;
                        wproplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
                        bp += vilen;
                        buflen -= vilen;
                    } else {
                        buflen--;
                        bp++;
                    }
                    if (wproplen) {
                        if (wproplen < 0 || wproplen > buflen) {
                            mmsg->rc = ISMRC_BadLength;
                            ism_common_setErrorData(mmsg->rc, "%s", "WillPropLen");
                        } else {
                            mmsg->willprop_len = wproplen;
                            mmsg->willprops = (char *)bp;
                            if (mmsg->rc == 0)
                                mmsg->rc = ism_common_checkMqttPropFields((char *)bp, wproplen,
                                    g_ctx5, CPOI_WILLPROP, mpropCheck, NULL);
                        }
                        bp += wproplen;
                        buflen -= wproplen;
                    }
                } else {
                    buflen--;
                    bp++;
                }
            }
        }
        /* Parse Will Topic */
        if (buflen > 1)
            mmsg->willtopic_len = BIGINT16(bp);
        bp += 2;
        mmsg->willtopic = (char *) bp;
        buflen -= (2 + mmsg->willtopic_len);
        bp += mmsg->willtopic_len;

        /* Parse Will Payload */
        if (buflen > 1)
            mmsg->willpayload_len = BIGINT16(bp);
        bp += 2;
        mmsg->willpayload = (char *) bp;
        buflen -= (2 + mmsg->willpayload_len);
        bp += mmsg->willpayload_len;
    } else {
        if (mmsg->version > 3 && (mmsg->flags & (CFLAG_WillRetain|CFLAG_WillQoS))) {
            mmsg->rc = ISMRC_BadClientData;
            ism_common_setErrorData(mmsg->rc, "%s", "will");
        }
    }

    /* Parse the user name */
    if (mmsg->flags & CFLAG_UserName) {
        if (buflen > 1) {
            mmsg->username_len = BIGINT16(bp);
            bp += 2;
            mmsg->username = (char *) bp;
            buflen -= (2 + mmsg->username_len);
            bp += mmsg->username_len;
        } else if (mmsg->version > 3) {   /* In v3.1 we ignore this */
            mmsg->rc = ISMRC_BadClientData;
            ism_common_setErrorData(mmsg->rc, "%s", "username");
        }
    } else {
        if ((mmsg->flags & CFLAG_Password) && mmsg->version < 5) {
            /* If password flag is set without username flag, request is not valid. */
            mmsg->rc = ISMRC_UsernameRequired;
            ism_common_setError(mmsg->rc);
        }
    }

    /* Parse the password */
    if (mmsg->flags & CFLAG_Password) {
        if (buflen > 1) {
            mmsg->password_len = BIGINT16(bp);
            bp += 2;
            mmsg->password = (char *) bp;
            buflen -= (2 + mmsg->password_len);
        }  else if (mmsg->version > 3) {   /* In v3.1 we ignore this */
            mmsg->rc = ISMRC_BadClientData;
            ism_common_setErrorData(mmsg->rc, "%s", "password");
        }
    }

    if (mmsg->rc == 0) {
        if ((buflen < 0)) {
            mmsg->rc = ISMRC_BadLength;
            ism_common_setErrorData(mmsg->rc, "%s%s", "CONNECT", "short");
        }
        else if (mmsg->version > 3 && buflen != 0) {
            mmsg->rc = ISMRC_BadLength;
            ism_common_setErrorData(mmsg->rc, "%s%s", "CONNECT", "extra");
        }
    }
}

/*
 * Put out the MQTTv5 CONNACK properties
 */
static void putConnackProperties(concat_alloc_t * rbuf, ism_transport_t * transport) {
    char xbuf [4096];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;

    int maxReceive = 0;
    switch (transport->client_class) {
    case 'd':     maxReceive = 10;    break;
    case 'a':     maxReceive = 256;
    case 'g':     maxReceive = 256;
    case 'A':     maxReceive = 2048;  break;
    }
    if (maxReceive)
        ism_common_putMqttPropField(&buf, MPI_MaxReceive,    g_ctx5, maxReceive);
    ism_common_putMqttPropField(&buf, MPI_MaxTopicAlias, g_ctx5, pobj->topicalias_count_in);
    ism_common_putMqttPropField(&buf, MPI_MaxPacketSize, g_ctx5, transport->endpoint->maxMsgSize);
    if (pobj->send_keepalive)
        ism_common_putMqttPropField(&buf, MPI_KeepAlive, g_ctx5, transport->keepalive);

    if (pobj->requestReason) {
        int nouserprop = buf.used;
        ism_common_putMqttPropNamePair(&buf, MPI_UserProperty, g_ctx5, "ServerName",    -1, ism_common_getServerName(), -1);
        ism_common_putMqttPropNamePair(&buf, MPI_UserProperty, g_ctx5, "ServerProduct", -1, IMA_PRODUCTNAME_FULL, -1);
        ism_common_putMqttPropNamePair(&buf, MPI_UserProperty, g_ctx5, "ServerVersion", -1, ism_common_getVersion(), -1);
        ism_common_putMqttPropNamePair(&buf, MPI_UserProperty, g_ctx5, "ServerDetails", -1, ism_common_getBuildLabel(), -1);
        if (pobj->maxPacketSize && buf.used-12 > pobj->maxPacketSize)
            buf.used = nouserprop;
    }

    /* Update the CONNACK buffer */
    ism_common_putMqttVarInt(rbuf, buf.used);
    if (buf.used)
        ism_common_allocBufferCopyLen(rbuf, buf.buf, buf.used);
    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);
}

/*
 * Continue from external authorization
 */
static void mqttContinueAuth(ism_transport_t * transport) {
    /*Remove any previous failed delay object if any*/
    if (ism_throttle_isEnabled())
        ism_throttle_removeThrottleObj(transport->name);
    ism_transport_submitAsyncJobRequest(transport, ism_mqtt_doMoreConnect, NULL, 0);
}


/*
 * MQTT failed authentication and auto registration
 */
static void mqttfailedAuth(ism_transport_t * transport) {
    mqttMsg_t  mmsg = {0};
    mmsg.transport = transport;
    mmsg.rc = CRC_NotAuthorized;
    ism_common_setError(ISMRC_NotAuthorized);
    mqttConnectError(&mmsg);
    ism_throttle_setConnectReqInQ(transport->clientID,0);
}

/*
 * Authentication service / server unavailable
 */
static void mqttUnavailable(ism_transport_t * transport, int rc) {
    mqttMsg_t  mmsg = {0};
    mmsg.transport = transport;
    mmsg.rc = CRC_NotAvailable;
    ism_common_setError(rc);
    mqttConnectError(&mmsg);
    ism_throttle_setConnectReqInQ(transport->clientID,0);
}

/*
 * Callback from authentication
 */
static void mqttAuthCallback(int rc, void * callbackParam) {
	ism_transport_t * transport = (ism_transport_t*) callbackParam;
	TRACE(8, "mqttAuthCallback: rc=%d connect=%u client=%s transport->auth_cb_called=%d\n", rc, transport->index, transport->name, transport->auth_cb_called);

	//Make sure Authentication Callback call only once per transport
	if(transport->auth_cb_called == 0){
	    TRACE(9, "mqttAuthCallback: connect=%d mqttPendingAuthenticationRequests=%d\n", transport->index,mqttStats.mqttPendingAuthenticationRequests);
	    __sync_sub_and_fetch(&mqttStats.mqttPendingAuthenticationRequests, 1);
	    transport->auth_cb_called=1;
	}
    if (!rc) {
        mqttContinueAuth(transport);
    } else if (rc == ISMRC_AuthUnavailable || rc == ISMRC_ServerNotAvailable) {
        mqttUnavailable(transport, rc);
    } else {
        mqttfailedAuth(transport);
    }
}

/*
 * Authentication callback from serverless connection
 */
static void mqttAuthServerless(int rc, void * callbackParam) {
    ism_transport_t * transport = (ism_transport_t*) callbackParam;
    TRACE(8, "mqttAuthServerless: rc=%d connect=%u client=%s\n", rc, transport->index, transport->name);
    if (rc) {
         mqttfailedAuth(transport);
    } else {
        /* Remove any previous failed delay object if any */
        if (ism_throttle_isEnabled())
            ism_throttle_removeThrottleObj(transport->name);
        __sync_sub_and_fetch(&mqttStats.mqttPendingAuthenticationRequests, 1);
        transport->ready = 4;
        /* Send the CONNACK */
        char xbuf [4096];
        concat_alloc_t buf = {xbuf, sizeof xbuf, 18};
        xbuf[16] = 0;   /* Session present = 0 */
        xbuf[17] = 0;   /* RC = 0 */
        if (transport->pobj->mqtt_version >= 5) {
            putConnackProperties(&buf, transport);
        }
        transport->send(transport, buf.buf+16, buf.used-16, MT_CONNACK << 4, SFLAG_FRAMESPACE);
        if (buf.inheap)
            ism_common_freeAllocBuffer(&buf);

        /* Decrement the inprogress */
#ifdef DEBUG_INPROGRESS
        TRACE(9, "Decrement inprogress for serverless: connect=%u inprogress=%d\n", transport->index, transport->pobj->inprogress);
#endif
        if (__builtin_expect((__sync_sub_and_fetch(&transport->pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
			TRACEL(7, transport->trclevel, "Complete close: connect=%u client=%s\n", transport->index, transport->name);
			ism_mqtt_doneConnection(transport);
		}

    }
}

/*
 * Make retain options from flags.
 * Store the retain options in a two byte integer
 */
int makeMonitorOpt(int connect, int secure, int websock, int qos, int retain) {
    return (((secure&0x7)<<12) | ((retain&0xf)<<8) | ((websock&3)<<6) | ((qos&0x3)<<4)) | (connect&0xf);
}

XAPI int ism_transport_setMaxSocketBufSize(ism_transport_t * transport, int maxSendSize, int maxRecvSize);
/*
 * Set the max buffer size based on the size in the securityh context.
 * This must be done after connection policy is complete.
 */
static inline void setSocketBuffer(ism_transport_t * transport, int msgRate) {
    /* Set max IO buffer size */
    int sendbuf;
    int recvbuf;
    switch (msgRate) {
        case EXPECTEDMSGRATE_LOW:
            sendbuf = 16*1024;
            recvbuf = 16*1024;
            break;
        case EXPECTEDMSGRATE_HIGH:
            sendbuf = 512*1024;
            recvbuf = 2048*1024;
            break;
        case EXPECTEDMSGRATE_MAX:
            sendbuf = 0;
            recvbuf = 0;
            break;
        case EXPECTEDMSGRATE_DEFAULT:
        default:
            sendbuf = 32*1024;
            recvbuf = 64*1024;
            break;
    }
    ism_transport_setMaxSocketBufSize(transport, sendbuf, recvbuf);
}


/*
 * Send the expected message rate based on
 */
static void setExpectedMsgRate(ism_transport_t * transport, concat_alloc_t * buf, mqttMsg_t * mmsg) {
    switch (transport->client_class) {
    case 'd':   /* device */
        ism_common_putExtensionValue(buf, EXIV_ExpectedMsgRate, g_ExpectedDevMsgRate);
        setSocketBuffer(transport, g_ExpectedDevMsgRate);
        break;
    case 'a':   /* application */
        ism_common_putExtensionValue(buf, EXIV_ExpectedMsgRate, g_ExpectedAppMsgRate);
        setSocketBuffer(transport, g_ExpectedAppMsgRate);
        if (!mmsg->maxActive)
            ism_common_putExtensionValue(buf, EXIV_MaxActive,  256);
        break;
    case 'A':   /* shared application */
        ism_common_putExtensionValue(buf, EXIV_ExpectedMsgRate, g_ExpectedShAppMsgRate);
        setSocketBuffer(transport, g_ExpectedShAppMsgRate);
        if (!mmsg->maxActive)
            ism_common_putExtensionValue(buf, EXIV_MaxActive,  1024);
        break;
    case 'g':   /* gateway */
        ism_common_putExtensionValue(buf, EXIV_ExpectedMsgRate, g_ExpectedGWMsgRate);
        setSocketBuffer(transport, g_ExpectedGWMsgRate);
        break;
    }
}

/*
 * Parse a certificate name to construct the ClientID
 */
static int parseCertName(const char * name, char * client_class, const char * * devtypeid) {
    if (name[1]==':' && (name[0]=='d' || name[0]=='g')) {
        char * pos = strchr(name+2, ':');
        if (pos && pos[1]) {
            pos = strchr(pos+1, ':');
            if (!pos) {
                *client_class = name[0];
                *devtypeid = name+2;
                return 0;
            }
        }
    }
    return 1;
}

/*
 * Generate the transport name field from the certificate
 */
static void generateNameFromCert(ism_transport_t * transport) {
    if (transport->cert_name && transport->sniName) {
        const char * devtypeid = NULL;
        char         client_class = 0;
        if (parseCertName(transport->cert_name, &client_class, &devtypeid)) {
            struct ssl_st * ssl = ism_transport_getSSL(transport);
            if (ssl) {
                char * xxbuf = alloca(512);
                int  ii;
                concat_alloc_t sanbuf = {xxbuf, 512};
#ifdef CUNIT
                int sancount = 0;
                if (transport->tlsReadBytes == 0) {
                    memcpy(sanbuf.buf, transport->client_host, transport->clientport);
                    sanbuf.used = transport->clientport;
                    sancount = transport->serverport;
                }
#else
                int  sancount = ism_ssl_getSubjectAltNames(ssl, &sanbuf);
#endif
                char * san = sanbuf.buf;
                for (ii=0; ii<sancount; ii++) {
                    if (!parseCertName(san, &client_class, &devtypeid))
                        break;
                   san += (strlen(san)+1);
                }
                ism_common_freeAllocBuffer(&sanbuf);
            }
        }
        if (client_class && devtypeid) {
            char * clientid = ism_transport_allocBytes(transport, 5+strlen(devtypeid)+strlen(transport->sniName), 0);
            sprintf(clientid, "%c:%s:%s", client_class, transport->sniName, devtypeid);
            transport->name = (const char *)clientid;
        }
    }
}


/*
 * Do the connection
 * Returns 1 if ism_mqtt_error() has been called.
 */
static void doConnect(ism_transport_t * transport, mqttMsg_t * mmsg, concat_alloc_t * buf) {
    int  flag_pos;
    int  pw_pos = 0;
    const char * userName = NULL;
    const char * passwd = NULL;
    const char * pos;
    char  sep;
    int   matched;
    char * clientId;
    int  clientIdLen;
    //dev_info_t * devInfo;

    transport->ready = 1;            /* No longer need the DoS timer */

    if (mmsg->clientid == NULL) {
        mmsg->clientid = "";
        mmsg->clientid_len = 0;
    }

    clientId = (char*)mmsg->clientid;
    clientIdLen = mmsg->clientid_len;
    mmsg->transport = transport;

    if (transport->cert_name && *transport->cert_name) {
        generateNameFromCert(transport);
    }

    if (!transport->name || !*transport->name) {
        /* Copy clientid into outgoing transport object */
        transport->name = ism_transport_allocBytes(transport, mmsg->clientid_len + 1, 0);
        memcpy((char *)transport->name, mmsg->clientid, mmsg->clientid_len);
        ((char *) transport->name)[mmsg->clientid_len] = 0;
    } else {
        clientId = (char *)transport->name;
        clientIdLen = strlen(transport->name);
    }
    transport->clientID = transport->name;

    /*
     * Check for restricted character set for ClientID
     */
    if (doClientIDMatch) {
        /* Make sure clientID matches regex */
        if (ism_regex_match(clientIDMatch, transport->name)) {
            mmsg->rc = CRC_BadIdentifier;
            ism_common_setErrorData(ISMRC_ClientIDNotValid, "%s", transport->name);
            mqttConnectError(mmsg);
            return;
        }
    }

    /* Set trace selected for clientID */
    if (ism_common_traceSelectClientID(transport->name))
        transport->trclevel = &ism_defaultDomain.selected;

    /* Extract tenant, type, and device */
    ism_clientclass_t * clientclass = ism_proxy_getClientClass(transport->endpoint->clientclass);
    if (clientclass) {
        ism_pxrule_t * rule = clientclass->classlist;
        matched = 0;
        while (rule) {
            if (matchClientClass(transport, rule)) {
                transport->client_class = rule->class;
                if (rule->after == 1) {
                    transport->alt_monitor = 1;
                    transport->use_userid = 1;
                }
                matched = 1;
                break;
            }
            rule = rule->next;
        }
        if (!matched && clientclass->deflt) {
            if (matchClientClass(transport, rule)) {
                transport->org = "";
                matched = 1;
            }
        }
        if (!matched) {
            TRACE(5, "Unable to parse the client ID: org=\"%s\" clientID=\"%s\" From=%s:%u connect=%u\n",
                    transport->org, transport->name, transport->client_addr, transport->clientport, transport->index);
            if (ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Server), 921, TRACE_DOMAIN, transport->name, transport->client_addr, NULL) ==0) {
            	LOG(WARN, Server, 921, "%-s%u%s%u", "The clientID is not valid: ClientID={0} Connect={1} From={2}:{3}.",
            			transport->name, transport->index, transport->client_addr, transport->clientport);
            }
            mmsg->rc = CRC_NotAuthorized;
            ism_common_setErrorData(ISMRC_ClientIDNotValid, "%s", transport->name);
            mqttConnectError(mmsg);
            return;
        }
    } else {
        /*
         * Use the simple separator rule
         */
        sep = transport->endpoint->separator;
        if (!sep)
            sep = ':';
        pos = memchr(mmsg->clientid, sep, mmsg->clientid_len);
        if (pos) {
            int tlen = pos-mmsg->clientid;
            char * varp = alloca(tlen+1);
            memcpy(varp, mmsg->clientid, tlen);
            varp[tlen] = 0;
            transport->org = varp;
            transport->deviceID = transport->name + (pos-mmsg->clientid) + 1;
            transport->client_class = 0;
        } else {
            transport->org = "";
            transport->deviceID = transport->name;
        }
    }

    /*
     * Do stats by client class
     */
    switch (transport->client_class) {
    case 'd':
        __sync_add_and_fetch(&mqttStats.mqttConnectedDev, 1);
        break;
    case 'a':
    case 'A':
        __sync_add_and_fetch(&mqttStats.mqttConnectedApp, 1);
        break;
    case 'c':
        __sync_add_and_fetch(&mqttStats.mqttConnectedGWs, 1);
        break;
    case 'g':
        __sync_add_and_fetch(&mqttStats.mqttConnectedGWs, 1);
        break;
    default:
        break;
    }

    switch (transport->pobj->mqtt_version) {
    case 3:
        __sync_add_and_fetch(&mqttStats.mqttVersion3_1, 1);
        break;
    case 4:
        __sync_add_and_fetch(&mqttStats.mqttVersion3_1_1, 1);
        break;
    case 5:
        __sync_add_and_fetch(&mqttStats.mqttVersion5_0, 1);
        break;
    default:
        break;
    }
    if (transport->pobj->prot == PROT_MQTT_WSBIN) {
        __sync_add_and_fetch(&mqttStats.mqttUseWebsocket, 1);
    }

    /* Make sure the tenent info in zero length and not null */
    if (!transport->org)
        transport->org = "";
    if (!transport->deviceID)
        transport->deviceID = "";
    if (!transport->typeID)
        transport->typeID = "";

    ism_tenant_t * tenant = ism_proxy_getValidTenant(transport);
    if (!tenant || tenant->enabled != 1) {
        mmsg->rc = CRC_NotAuthorized;
        ism_common_setErrorData(ISMRC_ClientIDNotValid, "%s", transport->name);
        mqttConnectError(mmsg);
        return;
    }

    /* Prevent spoofing of SNI */
    if (transport->usedSNI && strcmp(transport->sniName, transport->tenant->name)) {
        mmsg->rc = ISMRC_NotAuthorized;   /* Do not send CONNACK */
        ism_common_setErrorData(ISMRC_NotAuthorized, "%s%s%s", "SNI does not match org name: ",
                transport->sniName, transport->tenant->name);
        mqttConnectError(mmsg);
        return;
    }

    /* Setup the fairuse rules */
    tenant_fairuse_t * fairuse = ism_proxy_getFairUse(transport);

    if (fairuse && fairuse->mups_units) {
        float mups = 0;
        switch(transport->client_class) {
        case 'a': mups = fairuse->mups_a;   break;
        case 'A': mups = fairuse->mups_A;   break;
        case 'd': mups = fairuse->mups_d;   break;
        case 'g': mups = fairuse->mups_g;   break;
        }
        transport->fairuse(transport, FUR_SetUnitSize, fairuse->mups_units, (int)fairuse->log);
        transport->fairuse(transport, FUR_SetMaxMUPS, (int)(mups*10), 10);
    }

    /* Check client certificate name match */
    if (transport->cert_name && *transport->cert_name) {
        int name_match = ism_proxy_matchCertNames(transport);
        if (name_match)
            transport->secure = name_match == 3 ? 7 : 3;
    }

    /* Check message size */
    if (tenant->server && tenant->server->maxMsgSize)
        transport->maxMsgSize = tenant->server->maxMsgSize;       /* Override with server max message size */
    if (tenant->maxMsgSize)
        transport->maxMsgSize = transport->tenant->maxMsgSize;       /* Ovnerride with org max message size */

    /*
     * Allocate topic alias tables
     */
    if (mmsg->topic_alias > 0 && g_MaxTopicAliasOut > 0) {
        transport->pobj->topicalias_count_out = g_MaxTopicAliasOut;
        if (mmsg->topic_alias < g_MaxTopicAliasOut)
            transport->pobj->topicalias_count_out = mmsg->topic_alias;
        if (transport->pobj->topicalias_count_out) {
            transport->pobj->topicalias_out = (const char * *)
                ism_transport_allocBytes(transport, transport->pobj->topicalias_count_out * sizeof(char *), 1);
            memset(transport->pobj->topicalias_out, 0, transport->pobj->topicalias_count_out * sizeof(char *));
        }
    }
    if (g_MaxTopicAliasIn > 0) {
        transport->pobj->topicalias_count_in = g_MaxTopicAliasIn;
    }

    /*
     * If SecGuardian is not enabled we implement a very simple policy that
     * devices and gateways if requireCertificate and requireSecure are both
     * specified, we require a name matching certificate.
     *
     * If SecGuardian is enabled we allow it to enforce a policy about this.
     */
    if (tenant->sgEnabled != 1 && tenant->require_cert && tenant->require_secure==1) {
        if (transport->client_class == 'd' || transport->client_class == 'g') {
            if (tenant->require_user==1 && ((transport->secure&3) != 3)) {     /* Secure and name match */
                if (transport->cert_name && *transport->cert_name)
                    mmsg->rc = ISMRC_CertificateNotValid;
                else
                    mmsg->rc = ISMRC_NoCertificate;
                ism_common_setErrorData(mmsg->rc, "%s", transport->name);
                mqttConnectError(mmsg);
                return;
            }
        }
    }

    /*
     * Handle a shared subscription application ('A')
     */
    transport->pobj->keepalive = mmsg->keepalive;
    if (transport->client_class == 'A')  {
        /*
         * If there is no instance ID, generate one and add it to the clientID
         * This can only be done for non-durable sessions
         */
        if ((mmsg->flags & CFLAG_Clean) && transport->typeID[0] == 0) {
            clientId    = alloca(mmsg->clientid_len + 12);
            clientIdLen = makeSharedSubClientID(transport, clientId, mmsg->clientid, mmsg->clientid_len );

            /* Check keepalive if it is greater max for big A app. */
            if (g_maxKeepAliveA && (mmsg->keepalive == 0 || mmsg->keepalive > g_maxKeepAliveA)) {
                mmsg->keepalive = g_maxKeepAliveA;
                transport->pobj->keepalive = (uint16_t)mmsg->keepalive;
                transport->pobj->send_keepalive = 1;
            }
            transport->genClientID = ism_transport_putString(transport, clientId);
            TRACE(5, "ClientId \"%s\" was generated for shared subscription application: org=\"%s\" clientID=\"%s\" client_addr=%s, connect=%u\n",
                    transport->genClientID, transport->org, transport->name, transport->client_addr, transport->index);
        } else {
            /* If there is no instance and the session is durable, it is an error */
            if (!transport->typeID[0]) {
                mmsg->rc = CRC_NotAuthorized;
                ism_common_setErrorData(ISMRC_ClientIDNotValid, "%s", transport->name);
                mqttConnectError(mmsg);
                return;
            }
        }
    } else {
        /* Check keepalive if it is greater max for big A app. */
        if (g_maxKeepAlive && (mmsg->keepalive == 0 || mmsg->keepalive > g_maxKeepAlive)) {
            mmsg->keepalive = g_maxKeepAlive;
            transport->pobj->send_keepalive = 1;
            transport->pobj->keepalive = (uint16_t)mmsg->keepalive;
        }
    }

    /*
     * Check if a durable session is allowed
     */
    if (!(mmsg->flags & CFLAG_Clean) && tenant->allow_durable != 1) {
        mmsg->rc = CRC_NotAuthorized;
        TRACE(5, "Durable session is not allowed: org=\"%s\" clientID=\"%s\" From=%s:%u, connect=%u\n",
                transport->org, transport->name, transport->client_addr, transport->clientport, transport->index);
        if (ism_common_conditionallyLogged(NULL, ISM_LOGLEV(WARN), ISM_MSG_CAT(Server), 924, TRACE_DOMAIN,
                transport->name, transport->client_addr, NULL) ==0) {
        	LOG(WARN, Server, 924, "%-s%u%s%u", "A durable session is not allowed for this organization: ClientID={0} Connect={1} From={2}:{3}.",
        			transport->name, transport->index, transport->client_addr, transport->clientport);
        }
        ism_common_setErrorData(ISMRC_DurableNotAllowed, "%s", transport->name);
        mqttConnectError(mmsg);
        return;
    }

    transport->userid = ism_transport_allocBytes(transport, mmsg->username_len + 1, 0);
    memcpy((char *)transport->userid, mmsg->username, mmsg->username_len);
    memset((char *)transport->userid + mmsg->username_len, 0, 1);

    /*
     * If org is serverless, do authenticate now
     */
    if (tenant->serverless) {
        if (mmsg->flags & CFLAG_Will) {
            mmsg->rc = ISMRC_WillNotAllowed;
            ism_common_setErrorData(mmsg->rc, "%s", transport->name);
            mqttConnectError(mmsg);
            return;
        }
        if (mmsg->keepalive > 2400 || mmsg->keepalive == 0) {
            mmsg->keepalive = 2400;
            transport->keepalive = 2400;
            transport->pobj->send_keepalive = 1;
        } else {
            transport->keepalive = mmsg->keepalive;
        }

        pw_pos = buf->used;
        ism_common_allocBufferCopyLen(buf, mmsg->password, mmsg->password_len);
        passwd = buf->buf + pw_pos;
        bputchar(buf, 0);
        buf->used = pw_pos;
        __sync_add_and_fetch(&mqttStats.mqttPendingAuthenticationRequests, 1);
        ism_common_setError(0);
        transport->pwIsRequired = transport->pobj->mqtt_version >= 5;
        int arc = ism_proxy_doAuthenticate(transport, transport->userid, passwd, mqttAuthServerless, transport);
        if (arc != ISMRC_AsyncCompletion)
            mqttAuthServerless(arc, transport);
        return;
    }

    /* Downgrade QoS of Will message */
    if (mmsg->flags & CFLAG_Will) {
        if (((mmsg->flags & CFLAG_WillQoS) >> 3) > tenant->max_qos) {
            if (((mmsg->flags & CFLAG_WillQoS) >> 3) != 3) {
                mmsg->flags &= ~CFLAG_WillQoS;
                mmsg->flags |= tenant->max_qos << 3;
            }
        }
    }

    /* Ignore retain if it is not allowed */
    if ((mmsg->flags&CFLAG_WillRetain) && transport->tenant->allow_retain != 1) {
        mmsg->flags &= ~CFLAG_WillRetain;       /* Turn off retain */
    }

    /* Create outgoing CONNECT packet */
    if (g_useMQTTpx) {
        ism_common_allocBufferCopyLen(buf, "\0\6MQTTpx\4", 9);
        buf->buf[buf->used-1] = mmsg->version;
        flag_pos = buf->used;
        transport->pobj->flag_pos = 9;
        bputchar(buf, (char)mmsg->flags);
        mmsg->more_flags = XCFLAG_Domain | XCFLAG_MaxConn | XCFLAG_ExtraFlags | XCFLAG_ClientAddr | XCFLAG_Port;
        if (tenant->check_user)
            mmsg->more_flags |= XCFLAG_NoUserCheck;
        if (g_noProxyLog)
            mmsg->more_flags |= XCFLAG_NoLog;
        if (transport->cert_name)
            mmsg->more_flags |= XCFLAG_CertName;
        bputchar(buf, (char)mmsg->more_flags);   /* Additional flags */
    } else {
        if (mmsg->version == 3) {
            ism_common_allocBufferCopyLen(buf, "\0\6MQIsdp\3", 9);
            transport->pobj->flag_pos = 9;
        } else {
            ism_common_allocBufferCopyLen(buf, "\0\4MQTT", 6);
            transport->pobj->flag_pos = 6;
            bputchar(buf, (char)mmsg->version);
        }
        flag_pos = buf->used;
        bputchar(buf, (char)mmsg->flags);
    }
    bputchar(buf, (char)(mmsg->keepalive>>8));
    bputchar(buf, (char)mmsg->keepalive);
    if (transport->pobj->mqtt_version >= 5) {
        /* Assume all connect metadata is < 128 bytes */
        int metapos = buf->used++;

        transport->pobj->expiryOriginal = mmsg->expireTTL;
        uint32_t expire = mmsg->expireTTL;
        if (g_maxSessionExpire && g_maxSessionExpire < expire)
            expire = g_maxSessionExpire;
        if (tenant->maxSessionExpire && tenant->maxSessionExpire < expire)
            expire = tenant->maxSessionExpire;
        transport->pobj->expiryRequested = expire;

        /* Send basic connect properties */
        if (expire)
            ism_common_putMqttPropField(buf, MPI_SessionExpire, g_ctx5, expire);
        if (mmsg->maxActive > 0)
            ism_common_putMqttPropField(buf, MPI_MaxReceive, g_ctx5, mmsg->maxActive);
        ism_common_putMqttPropField(buf, MPI_MaxTopicAlias, g_ctx5, 0);     /* No topic alias server to proxy */
        if (mmsg->request_reason == 0)
            ism_common_putMqttPropField(buf, MPI_RequestReason, g_ctx5, mmsg->request_reason);
        if (mmsg->maxPacketSize)
            ism_common_putMqttPropField(buf, MPI_MaxPacketSize, g_ctx5, mmsg->maxPacketSize);
        /* Set a property to say this comes from imaproxy */
        ism_common_putMqttPropNamePair(buf, MPI_UserProperty, g_ctx5, "imaproxy", -1,  "5", -1);
        /* Copy connect user props but not if they are too large */
        if (mmsg->has_user_props) {
            buf->compact = metapos;   /* Overhead before props */
            ism_common_checkMqttPropFields(mmsg->props, mmsg->prop_len, g_ctx5, 0xFFFF, mpropCopyUserProps, buf);
            buf->compact = 0;
        }
        buf->buf[metapos] = (char)(buf->used-metapos-1);
    }
    bputchar(buf, (char)clientIdLen>>8);
    bputchar(buf, (char)clientIdLen);
    ism_common_allocBufferCopyLen(buf, transport->genClientID ? transport->genClientID : transport->clientID, clientIdLen);

    if (mmsg->flags & CFLAG_Will) {
        int rc;
        if (mmsg->version >= 5) {
            if (!mmsg->willprop_len) {
                bputchar(buf, 0);
            } else {
                ism_common_putMqttVarInt(buf, mmsg->willprop_len);
                ism_common_allocBufferCopyLen(buf, mmsg->willprops, mmsg->willprop_len);
            }
        }
        rc = convertTopic(transport, buf, mmsg->willtopic, mmsg->willtopic_len, 0);
        if (rc) {
            mmsg->rc = rc;
            TRACE(6, "Error found in will message convert topic: connect=%u client=%s rc=%u\n", transport->index, transport->name, rc);
            mqttConnectError(mmsg);
            return;
        }
        /* Copy the will topic to re-check authority */
        transport->pobj->willTopic = ism_transport_allocBytes(transport, mmsg->willtopic_len+1, 0);
        memcpy((char *)transport->pobj->willTopic, mmsg->willtopic, mmsg->willtopic_len);
        memset((char *)(transport->pobj->willTopic+mmsg->willtopic_len), 0, 1);

        bputchar(buf, (char)(mmsg->willpayload_len>>8));
        bputchar(buf, (char)mmsg->willpayload_len);
        ism_common_allocBufferCopyLen(buf, mmsg->willpayload, mmsg->willpayload_len);
    }


    /*
     * If we are authenticating the user in the proxy, we do not need to check it
     * in the server, so we remove it.  We still send it in proxy protocol for use
     * in debugging.
     */
    if (tenant->remove_user && tenant->check_user) {
        if (g_useMQTTpx && (tenant->server->monitor_connect&0x0f)) {
            buf->buf[flag_pos] &= ~CFLAG_Password;
            if (mmsg->flags & CFLAG_UserName) {
                bputchar(buf, (char)(mmsg->username_len>>8));
                bputchar(buf, (char)mmsg->username_len);
                ism_common_allocBufferCopyLen(buf, mmsg->username,mmsg->username_len);
            }
        } else {
            buf->buf[flag_pos] &= ~(CFLAG_Password | CFLAG_UserName);
        }
    } else {
        if (mmsg->flags & CFLAG_UserName) {
            bputchar(buf, (char)(mmsg->username_len>>8));
            bputchar(buf, (char)mmsg->username_len);
            ism_common_allocBufferCopyLen(buf, mmsg->username, mmsg->username_len);
        }
        if (mmsg->flags & CFLAG_Password) {
            bputchar(buf, (char)(mmsg->password_len>>8));
            bputchar(buf, (char)mmsg->password_len);
            pw_pos = buf->used;
            ism_common_allocBufferCopyLen(buf, mmsg->password, mmsg->password_len);
            bputchar(buf, 0);
            buf->used--;

        }
    }

    /*
     * Put out tenent (org) and connection count
     */
    if (g_useMQTTpx) {
        int xint;
        int xlen;

        /* Send org name */
        bputchar(buf, (char)(tenant->namelen>>8));
        bputchar(buf, (char)tenant->namelen);
        ism_common_allocBufferCopyLen(buf, tenant->name, tenant->namelen);
        /* Send max connection count */
        xint = endian_int32(tenant->max_connects);
        ism_common_allocBufferCopyLen(buf, (const char *)&xint, 4);

        /* Add in the extension */
        if (mmsg->more_flags & XCFLAG_ExtraFlags) {
            int extlen;
            int extloc = buf->used;
            buf->used += 2;
            /* If we are doing server monitoring, send the options to the server */
            if (g_useMQTTpx && transport->server && (transport->server->monitor_connect&0x0f)) {
                int doretain = transport->server->monitor_retain&0x0f;
                /* Force retain to 1 for applications */
                if (doretain && transport->alt_monitor && transport->server->monitor_topic_alt)
                    doretain = 1;
                /* Force retain to 0 for connectors */
                if (transport->client_class == 'c')
                    doretain = 0;
                int monitor_opt = makeMonitorOpt(transport->server->monitor_connect, transport->secure,
                      (transport->pobj->prot==PROT_MQTT_WSBIN), transport->server->monitor_qos, doretain);
                ism_common_putExtensionValue(buf, EXIV_MonitorOpt, monitor_opt);
                bputchar(buf, EXIV_MonitorTopic);
                const char * montopic = transport->server->monitor_topic;
                if (transport->alt_monitor && transport->server->monitor_topic_alt)
                    montopic = transport->server->monitor_topic_alt;
                ism_proxy_makeMonitorTopic(buf, transport, montopic);
                transport->no_monitor = 1;
            }
            /* Put out session expire */
            if (!(mmsg->flags & CFLAG_Clean) && (g_maxSessionExpire || tenant->maxSessionExpire)) {
                uint32_t expire = g_maxSessionExpire;
                if (tenant->maxSessionExpire)
                    expire = tenant->maxSessionExpire;
                ism_common_putExtensionValue(buf, EXIV_ExpireTTL, expire);
            }
            /* Check if we have ACLs */
            if (transport->client_class == 'g' || transport->client_class == 'a' || transport->client_class == 'A') {
                if(!(transport->tenant->allow_anon==1 && transport->tenant->check_user)){
                    ism_common_putExtensionValue(buf, EXIV_ACLWait, 1);
                    ism_common_putExtensionValue(buf, EXIV_SendSubs, 1);
                    transport->has_acl = 2;
                }
            } else {
                ism_common_putExtensionValue(buf, EXIV_SendSubs, 1);
            }
            setExpectedMsgRate(transport, buf, mmsg);

            /* This must be the last item in the extension, and may be replaced later by CheckSessionUser */
            transport->pobj->end_ext_loc = buf->used-16;
            ism_common_putExtensionValue(buf, EXIV_EndExtension, 1);

            extlen = buf->used-extloc-2;
            buf->buf[extloc] = (char)(extlen<<8);
            buf->buf[extloc+1] = (char)(extlen);
        }

        /* Send client_addr */
        if (mmsg->more_flags & XCFLAG_ClientAddr) {
            xlen = transport->client_addr ? (int)strlen(transport->client_addr) : 0;
            bputchar(buf, (char)(xlen >> 8));
            bputchar(buf, (char)xlen);
            if (xlen)
                ism_common_allocBufferCopyLen(buf, transport->client_addr, xlen);
        }

        /* Send cert name */
        if (mmsg->more_flags & XCFLAG_CertName) {
            xlen = (int)strlen(transport->cert_name);
            bputchar(buf, (char)(xlen >> 8));
            bputchar(buf, (char)xlen);
            ism_common_allocBufferCopyLen(buf, transport->cert_name, xlen);
        }
        /* Send the server port */
        if (mmsg->more_flags & XCFLAG_Port) {
            bputchar(buf, (char)(transport->serverport>>8));
            bputchar(buf, (char)transport->serverport);
        }
    }

    /* Create the saved data to be sent when the outgoing connection is established */
    appendSavedData(transport, buf->buf+16, buf->used-16, MT_CONNECT<<4);

    /*
     * Check auth now that we have a null terminated username and password
     */
    if (pw_pos == 0 && mmsg->password_len) {
        pw_pos = buf->used;
        ism_common_allocBufferCopyLen(buf, mmsg->password, mmsg->password_len);
        passwd = buf->buf + pw_pos;
        bputchar(buf, 0);
        buf->used = pw_pos;
    }
    if (pw_pos)
        passwd = buf->buf + pw_pos;
    userName = transport->userid;
    if (tenant->user_is_clientid==1) {
        if (!transport->use_userid)
            userName = transport->clientID;
    }
    __sync_add_and_fetch(&mqttStats.mqttPendingAuthenticationRequests, 1);
    ism_common_setError(0);
    transport->pwIsRequired = transport->pobj->mqtt_version >= 5;
    mmsg->rc = ism_proxy_doAuthenticate(transport, userName, passwd, mqttAuthCallback, transport);
    if (mmsg->rc) {

        /* If auth went async, we will reenter in ism_mqtt_doMoreConnect() */
        if (mmsg->rc == ISMRC_AsyncCompletion) {
            mmsg->rc = 0;
        } else {
            if (mmsg->rc == ism_common_getLastError()) {
                /* Store reason in transport object */
                char * ebuf = alloca(2048);
                ism_common_formatLastError(ebuf, 2048);
                if (*ebuf) {
                    int elen = (int)strlen(ebuf);
                    char * rp = ism_transport_allocBytes(mmsg->transport, elen+1, 0);
                    memcpy(rp, ebuf, elen);
                    rp[elen] = 0;
                    mmsg->reason = (const char *)rp;
                }
            }
        	/* This can be only ISMRC_NoCertificate or CRC_NotAuthorized */
            TRACE(7, "doAuthenticate error: connect=%u client=%s rc=%d reason=%s\n", transport->index,
                    transport->name, mmsg->rc, mmsg->reason ? mmsg->reason : "");
            mqttConnectError(mmsg);
            mmsg->rc = CRC_NotAuthorized;
            __sync_sub_and_fetch(&mqttStats.mqttPendingAuthenticationRequests, 1);
        }
        return;
    }


    /*
     * If we get a good synchronous result, complete the connection now
     */
    mmsg->rc = 0;
    if (transport->pobj->server_transport) {   /* CUnit testing */
        ism_mqtt_connected(transport->pobj->server_transport, 0);
    } else {
        __sync_sub_and_fetch(&mqttStats.mqttPendingAuthenticationRequests, 1);
        ism_mqtt_doMoreConnect(transport, NULL, 0);
    }
}

/**
 * Set Will
 */
int ism_mqtt_setWillValidation(int willValidation) {
    if(g_willValidation != willValidation){
        g_willValidation = willValidation;
        TRACEL(5,ism_defaultTrace, "Set Will Validation Policy: =%d\n", g_willValidation);
    }
    return 0;
}

bool ism_mqtt_isValidWillPolicy(int willPolicy) {
    if (WillTopicPolicy_RemoveNotValid <= willPolicy && willPolicy <= WillTopicPolicy_AllowAll)
        return true;
    return false;
}
/*
 * Return Error when will topic is not authorized
 */
int willTopicNotAuthorized(ism_transport_t * transport) {
    mqttMsg_t mmsg = { 0 };
    mmsg.transport = transport;
    mmsg.rc = ISMRC_NotAuthorized;
    ism_common_setErrorData(mmsg.rc, "%s%s%s", "Will topic not authorized",
            transport->pobj->willTopic, transport->name);
    TRACE(6,
            "Error found in will message convert topic or ACL check: connect=%u client=%s rc=%u topic=%s\n",
            transport->index, transport->name, mmsg.rc,
            transport->pobj->willTopic);
    mqttConnectError(&mmsg);
    return mmsg.rc;
}
/*
 * Continue with the connect by constructing the outgoing connection
 */
int ism_mqtt_doMoreConnect(ism_transport_t * transport, void * param1, uint64_t param2) {
    ism_transport_t * stransport;
    int rc = 0;
    char * errMsg;
 	TRACE(9, "ism_mqtt_doMoreConnect: connect=%u transport->ready=%d inprogress=%d client=%s\n",
 	        transport->index, transport->ready, transport->pobj->inprogress, transport->name);

	/* If AWS SQS, return CONNACK */
	if (transport->server && transport->server->serverKind==2) {
		char * response = alloca(1024);
		concat_alloc_t rbuf = {response, 1024};
		response[16] = 0;
		response[17] = (char) 0; //Authorized
		response[18] = 0;
		rbuf.used = 18;
		if (transport->pobj->mqtt_version >= 5) {
			char * erbuf = alloca(2048);
			ism_common_formatLastError(erbuf, 2048);
			int reasonlen = strlen(erbuf);
			if (reasonlen && (!transport->pobj->maxPacketSize || (reasonlen+11 < transport->pobj->maxPacketSize))) {
				ism_common_putMqttVarInt(&rbuf, reasonlen+3);
				ism_common_putMqttPropString(&rbuf, MPI_Reason, g_ctx5, erbuf, reasonlen);
			} else {
				rbuf.used++;         /* proplen = 0 */
			}
		}
		TRACE(5, "ism_mqtt_doMoreConnect: SQS CONNACK: connect=%u client=%s\n", transport->index, transport->name);
		transport->send(transport, rbuf.buf + 16, rbuf.used - 16, MT_CONNACK << 4, SFLAG_SYNC | SFLAG_FRAMESPACE);
		return 0;
	}

	/*
	 * If we have a will message it was originally converted before we had the role or ACLs
	 * for the connection, so re-check it as we now have those.
	 */
	if (transport->pobj->willTopic) {
	    char * xbuf = alloca(1024);
	    concat_alloc_t wbuf = {xbuf, 1024};
	    int willTopic_len = strlen(transport->pobj->willTopic);
	    int crc;

	    crc = convertTopic(transport, &wbuf, transport->pobj->willTopic, willTopic_len, 0);
        if (crc) {
            if (wbuf.inheap)
                ism_common_freeAllocBuffer(&wbuf);
            return willTopicNotAuthorized(transport);
        }
        //If WillValidation is not set the org, use the proxy value, 0xff means not set
        uint8_t willValidation = g_willValidation;
        if (transport->tenant->willTopicValidationPolicy != 0xff){
            willValidation = transport->tenant->willTopicValidationPolicy;
        }
        if (willValidation != WillTopicPolicy_AllowAll) {
            /*
             * Disable ACL checking on the will message temporarily because this breaks one of our
             * customers.  This check should be re-enabled after the customer code is fixed.
             */
            if (!crc && transport->has_acl) {
                crc = checkACL(transport, wbuf.buf);
                TRACE(9,
                        "Checking ACLs for WillTopic. connect=%u client=%s rc=%u topic=%s\n",
                        transport->index, transport->name, crc,
                        transport->pobj->willTopic);
            }

            if (wbuf.inheap)
                ism_common_freeAllocBuffer(&wbuf);

            if (crc && willValidation == WillTopicPolicy_ReturnErrorNotValid)
                return willTopicNotAuthorized(transport);

            if (crc || willValidation == WillTopicPolicy_RemoveAll) {
                //Will Topic is not authorized or not valid. Remove the Will Topic. The CONNECT continue
                //Issue:2982
                TRACE(5,
                        "Will topic is removed since it is not authorized. connect=%u client=%s rc=%u topic=%s\n",
                        transport->index, transport->name, crc,
                        transport->pobj->willTopic);
                removeWill(transport);
            }
        } else {
            if (wbuf.inheap)
                ism_common_freeAllocBuffer(&wbuf);
        }
	}

    /*
     * Create outgoing connection
     */
    if (g_useMux) {
        errMsg = alloca(128);
        stransport = ism_mux_createVirtualConnection(transport->server, transport->tid, &rc, errMsg);
    } else {
        rc = ISMRC_AllocateError;
        errMsg = "Memory allocation error.";
        stransport = ism_transport_newOutgoing(transport->endpoint, 1);
    }
    if (!stransport) {
    	ism_throttle_setConnectReqInQ(transport->clientID, 0);
#ifdef DEBUG_INPROGRESS
    	TRACE(9, "Decrement inprogress unable to create outgoing: connect=%u inprogress=%d\n", transport->index, transport->pobj->inprogress);
#endif
        if (__builtin_expect((__sync_sub_and_fetch(&transport->pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
            ism_mqtt_doneConnection(transport);
        } else {
            transport->close(transport, rc, 0, errMsg);
        }
    	return -1;
    }
    switch (transport->pobj->mqtt_version) {
    case 3:  stransport->protocol = "mqtt-tcp";   break;
    case 5:  stransport->protocol = "mqtt5-tcp";  break;
    default:
    case 4:  stransport->protocol = "mqtt4-tcp";  break;
    }
    stransport->protocol_family = "mqtt";
    ism_mqtt_connection(stransport);
    stransport->tid = transport->tid;
    stransport->connected = ism_mqtt_connected;
    stransport->pobj->client_transport = transport;
    stransport->pobj->transport = stransport;
    stransport->pobj->mqtt_version = transport->pobj->mqtt_version;
    stransport->tenant = transport->tenant;
    stransport->server = transport->server;
    stransport->client_class = transport->client_class;
    stransport->originated = 1;
    transport->pobj->server_transport = stransport;
    stransport->pobj->connectPending = 1;

    /* Copy clientid into outgoing transport object */
    if (transport->genClientID) {
    	stransport->name = ism_transport_putString(stransport, transport->genClientID);
    } else {
    	stransport->name = ism_transport_putString(stransport, transport->name);
    }
	stransport->clientID = stransport->name;

    /* Copy userid into outgoing transport object */
    if (transport->userid && *transport->userid) {
        stransport->userid = ism_transport_putString(stransport, transport->userid);
    } else {
        stransport->userid = "";
    }

    TRACE(7, "Make outgoing connection. connect=%u inprogress=%d client=%s org=%s server=%s sconnect=%d\n",
            transport->index, transport->pobj->inprogress, transport->name, transport->tenant->name, transport->server->name, stransport->index);
    /* Do the connect, this completes in ism_mqtt_connected()  */
    transport->ready = 2;    /* Client connection authorized */

    if (g_useMux) {
        stransport->frame = ism_transport_frameMqtt;
        stransport->addframe = ism_transport_addMqttFrame;
        ism_mqtt_connected(stransport, 0);
    } else {
        rc = ism_transport_connect(stransport, transport, transport->tenant, transport->server->tlsCTX);
        if (rc) {
            ism_throttle_setConnectReqInQ(transport->clientID,0);
        }
        return rc;
    }
    return 0;
}

/*
 * Parse a topic into segments.
 * This is done in place so the topic must be a copy.
 */
static int topicSegment(char * topic, char * * segments, int count) {
    int segs = 1;
    if (count > 0)
        segments[0] = topic;
    while ((topic = strchr(topic, '/'))) {
        *topic++ = 0;
        if (segs < count)
            segments[segs] = topic;
        segs++;
    }
    return segs;
}

/*
 * Parse type and id from a topic.
 * This should be externalized, but we just code it for now.
 * This must be a copy
 *
 * Return 0 if we find a type and id.  2=has wildcae, 1=otherwise.
 * Some productions such as monitoring do not contain the type and id.
 */
static int parseTopicAuth(char * topic, const char * * devtype, const char * * devid) {
    char * topicseg[8];
    int    count;

    if (*topic == '$') {
        int topiclen = strlen(topic);
        if (topiclen > 20 && !memcmp(topic, "$SharedSubscription/", 20)) {
            topic += 20;
        } else if (topiclen >= 7 && !memcmp(topic, "$share/", 7)) {
            topic += 7;
        } else {
            return 1;
        }
        char * pos = strchr(topic, '/');
        if (pos) {
            topic = pos+1;
        } else {
            return 1;
        }
    }
    count = topicSegment(topic, topicseg, 8);
    if (count > 5) {
        if (!strcmp(topicseg[2], "type") && !strcmp(topicseg[4], "id")) {
            if (*topicseg[3] && *topicseg[3] != '+' &&
                *topicseg[5] && *topicseg[5] != '+') {
                *devtype = topicseg[3];
                *devid = topicseg[5];
                return 0;
            }
            return 2;
        }
    }
    return 1;
}

/*
 * Complete an action send on return from an authorization request
 */
static int sendMqttData(ism_transport_t * transport, void * param1, uint64_t param2) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    asyncauth_t * async = (asyncauth_t *) param1;
    asyncauth_t * pendingAuthRequest = NULL;
    int count = 0;
    char * devtype=async->devtype;
    char * devid = async->devid;
    const char * clientID = transport->clientID;
    int rc = (int32_t) param2;
    const char * org = transport->tenant ? transport->tenant->name : NULL ;

    dev_info_t * devInfo = ism_proxy_getDeviceAuthInfo(org, async->devtype, async->devid);

    if (devInfo)
        pendingAuthRequest = ism_proxy_getDeviceAuthPendingRequest(devInfo, transport->name);

    if (devInfo && (pendingAuthRequest == async)) {
        uint64_t authtime = (uint64_t) (ism_common_readTSC() * 1000.0);
        int authRC, createRC;
		if (async->action == AUTH_AutoCreateOnly) {
			createRC = rc;
			authRC = devInfo->authRC;
		} else {
			authRC = rc;
			createRC = devInfo->createRC;
		}

		ism_proxy_setDeviceAuthComplete(devInfo, authRC, createRC, authtime, transport->name);

		TRACE(6, "sendMqttData: connect=%d async=%p action=%d devInfo=%p clientID=%s org=%s devID=%s devType=%s pendingAuthRequest=%p authRC=%d createRC=%d authTime=%llu reqTime=%llu authElapsedTime=%llu knownDevsCount=%u pendingDevsCount=%u inprogress=%d\n",
				transport->index, async, async->action, devInfo, clientID, org, devid, devtype, pendingAuthRequest,
				devInfo->authRC, devInfo->createRC, (ULL)devInfo->authTime, (ULL)async->reqTime, (ULL)(devInfo->authTime - async->reqTime), pobj->knownDevsCount, pobj->pendingDevsCount, pobj->inprogress);

		ism_proxy_unlockDeviceInfo(devInfo);

		pobj->pendingDevsCount--;
        if (!rc)
            pobj->knownDevsCount++;

        do {
            asyncauth_t * currAuthInfo = async;
            count++;
            if (!pobj->closed) {
                char * bufbuf = async->bufbuf;
                int bufused = async->bufused;
                int kind = async->kind;
                if (rc) {
                    sendGWPINGREQ(transport);
                    if (pobj->mqtt_version >= 5) {
                        mqttMsg_t mmsg = {0};
                        mmsg.transport = transport;
                        mmsg.qos = (kind>>1)&3;
                        mmsg.rc = ISMRC_NotAuthorized;
                        ism_common_setErrorData(mmsg.rc, "%s%s%s", "Device not in group:", async->devtype, async->devid);
                        publishNAK(&mmsg);
                    } else {
                        sendDirectReply(transport, kind, bufbuf+16, bufused-16, 1);
                    }
                    if (pobj->server_transport)
                        pobj->server_transport->lost_msg++;

                } else {
                    //Added async->sendSavedData to avoid the circular add into the savedData while
                    //processing the savedData buffer. this will force the send of MQTT data since
                    //we know for sure that CONNACK is back.
                    TRACE(9, "sendMqttData: connect=%d connectdata=%p saveddata=%p savelength=%d sendSavedData=%d savedataprocessing=%d\n",
                                    transport->index, pobj->connectdata, pobj->savedata, pobj->savelen, async->sendSavedData, pobj->savedataprocessing);
                    if (LIKELY((pobj->connectdata == NULL && pobj->savedata == NULL && !pobj->savedataprocessing) || async->sendSavedData)) {
                    		uint8_t command = (uint8_t)((kind >> 4) & 15);
						int sendMQTT=g_msgRoutingDefaultSendMQTT;
						if (command == MT_PUBLISH){
							ism_route_routeMessage(transport, bufbuf+16, bufused-16, kind,  &sendMQTT);
						}else{
							sendMQTT=1;
						}
						if(sendMQTT){
							if (LIKELY(pobj->server_transport != NULL)) {
								pobj->server_transport->send(pobj->server_transport, bufbuf+16, bufused-16, kind, SFLAG_FRAMESPACE);
							}
						}
                    } else {
                        appendSavedData(transport, bufbuf+16, bufused-16, kind);
                    }
                    if (pobj->server_transport)
                        pobj->server_transport->write_msg++;
                }
            }
            async = async->next;
            ism_common_free(ism_memory_proxy_mqtt_auth,currAuthInfo);
        } while (async);
    } else {
        TRACE(6, "sendMqttData - ignore old request: connect=%d async=%p devInfo=%p clientID=%s org=%s devID=%s devType=%s pendingAuthRequest=%p inprogress=%d\n",
                transport->index, async, devInfo, clientID, org, async->devid, async->devtype, pendingAuthRequest, pobj->inprogress );
        while (async) {
            asyncauth_t * currAuthInfo = async;
            count++;
            async = async->next;
            ism_common_free(ism_memory_proxy_mqtt_auth,currAuthInfo);
        }
        if (devInfo)
        	ism_proxy_unlockDeviceInfo(devInfo);
    }

    int inprogress = __sync_sub_and_fetch(&pobj->inprogress, count);
#ifdef DEBUG_INPROGRESS
    TRACE(9, "Decrement inprogress sending connect: connect=%u inprogress=%d\n", transport->index, pobj->inprogress);
#endif
    if(async!=NULL){
        TRACE(7, "sendMqttData - complete: connect=%d async=%p devInfo=%p clientID=%s org=%s devID=%s devType=%s pendingAuthRequest=%p inprogress=%d processed_async=%d\n",
                      transport->index, async, devInfo, clientID, org, devid, devtype, pendingAuthRequest, inprogress, count);
    }else{
        TRACE(7, "sendMqttData - complete: connect=%d inprogress=%d processed_async=%d\n",
                             transport->index,  inprogress, count);
    }

    if (UNLIKELY(inprogress <= 0)) {
        if (inprogress < 0) {
            ism_mqtt_doneConnection(transport);
        } else {
            if (__sync_bool_compare_and_swap(&pobj->disconnect_pending, 1, 0)) {
                if (!pobj->closed) {
                    char dbuf[16];
                    if (LIKELY(pobj->server_transport != NULL)) {
                        pobj->server_transport->send(pobj->server_transport, dbuf+16, 0, MT_DISCONNECT << 4, SFLAG_FRAMESPACE);
                    }
                }
            }
        }
    }
    return 0;
}


static void mqttAuthorizeCallback(int rc, const char * reason, void * callbackParam) {
	asyncauth_t * async = (asyncauth_t *) callbackParam;
	ism_transport_t * transport = async->transport;

    __sync_sub_and_fetch(&mqttStats.mqttPendingAuthorizationRequests, 1);

	if (rc) {
		/* Send notification  */
		uint8_t command = (uint8_t)((async->kind >> 4) & 15);
		char * request =NULL;
		char * topic=NULL;
		if (command == MT_PUBLISH) {
			request = "Publish";
			int topiclen = (uint16_t)BIGINT16(async->bufbuf+16);
			topic = alloca(topiclen+1);
			memcpy(topic, async->bufbuf+18, topiclen);
			topic[topiclen] = 0;
		} else if (command == MT_SUBSCRIBE) {
			request = "Subscribe";
			int topiclen = (uint16_t)BIGINT16(async->bufbuf+18);
			topic = alloca(topiclen+1);
			memcpy(topic, async->bufbuf+20, topiclen);
			topic[topiclen] = 0;
		}
		TRACE(7, "mqttAuthorizeCallback: connect=%u rc=%d topic=%s\n", transport->index, rc, topic);
		if (topic)
			ism_proxy_GWNotify(transport, (const char *)request, topic, async->devtype, async->devid, rc, reason);
	}

	/* Handle publish or single subscribe */
	if (async->count < 0) {
		/* Add a work item to the IOP to send the data */
		ism_transport_submitAsyncJobRequest(transport, sendMqttData, async, rc);
	} else {
		/* Add a work item to the IOP to send the data */
		ism_transport_submitAsyncJobRequest(transport, sendMqttData, async, rc);
	}

}

/*
 * Create an async authorization job
 */
static asyncauth_t * createAsyncAuthJob(ism_transport_t * transport, concat_alloc_t * buf, int kind,
        uint16_t action, const char * devtype, const char * devid, ism_proxy_AuthorizeCallback_t callback) {
    int devtypeLen = (devtype) ? (strlen(devtype) + 1) : 0;
    int devidLen = (devid) ? (strlen(devid) + 1) : 0;
    size_t length = sizeof(asyncauth_t)+buf->used + devtypeLen+devidLen;
    asyncauth_t * result = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_mqtt_auth,1),length);
    char * ptr = (char*)(result+1);
    memset(result,0,sizeof(asyncauth_t));
    result->transport = transport;
    result->kind = kind;
    result->action = action;
    if (transport->channel) {
        result->authtoken = transport->channel;
        result->authtokenLen = strlen(result->authtoken);
    }
    result->bufbuf = ptr;
    result->bufused = buf->used;
    memcpy(ptr, buf->buf, buf->used);
    ptr += buf->used;
    if (devtypeLen) {
        memcpy(ptr, devtype,devtypeLen);
        result->devtype = ptr;
        ptr += devtypeLen;
    }

    if (devidLen) {
        memcpy(ptr, devid, devidLen);
        result->devid = ptr;
        ptr += devidLen;
    }

    result->callback = callback;
    result->callbackParam = result;
    result->permissions = transport->auth_permissions;

    result->reqTime = (uint64_t) (ism_common_readTSC() * 1000);
    return result;
}


/* Check Authorization interface */
extern int ism_proxy_checkAuthorization(asyncauth_t *asyncJob);


/*
 * Check if we need to do an async auth check for publish
 */
static int checkPublishAuth(ism_transport_t * transport, concat_alloc_t * buf, int kind, int rcCheckACL, int sendSavedData) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    int rc;
    uint16_t asyncAction;
    const char * devtype;
    const char * devid;
    const char * clientID=transport->clientID;
    /* The topic string is at offset 16 in the buf */
    int topiclen = (uint16_t)BIGINT16(buf->buf+16);
    char * topic = alloca(topiclen+1);
    memcpy(topic, buf->buf+18, topiclen);
    topic[topiclen] = 0;
    rc = parseTopicAuth(topic, &devtype, &devid);

    /* Check device auth info */
    if (rc == 0) {
    	const char * org = transport->tenant ? transport->tenant->name : NULL;
    	dev_info_t * devInfo = ism_proxy_getDeviceAuthInfo(org, devtype, devid);
        if (devInfo) {

        	asyncauth_t * pendingAuthRequest = ism_proxy_getDeviceAuthPendingRequest(devInfo, transport->name);
			uint64_t currTime = (uint64_t) (ism_common_readTSC() * 1000.0);
			TRACE(7, "checkPublishAuth: connect=%d devInfo=%p clientID=%s org=%s devID=%s devType=%s pendingAuthRequest=%p authRC=%d createRC=%d authTime=%llu currTime=%llu gwCleanupTime=%llu\n",
					transport->index, devInfo, clientID, org, devid, devtype, pendingAuthRequest, devInfo->authRC, devInfo->createRC, (ULL)devInfo->authTime,
					(ULL)currTime, (ULL)g_gwCleanupTime);

			int drc = ism_proxy_checkDeviceAuth(devInfo, currTime, transport->name);

			switch (drc) {
			case -1: // cache timed out
				break;
			case 0: // already authorized according to the cache
				if (rcCheckACL) {
					// Check ACL failed before, so will authorize again
					TRACE(7, "checkPublishAuth: connect=%d devInfo=%p clientID=%s org=%s devID=%s devType=%s rcCheckACL=%d authRC=%d createRC=%d checkDevAuthRC=%d\n",
								transport->index, devInfo, clientID, org, devid, devtype, rcCheckACL, devInfo->authRC, devInfo->createRC, drc);
					break;
				} else {
					ism_proxy_unlockDeviceInfo(devInfo);
					return 0;
				}
			default:
				ism_proxy_unlockDeviceInfo(devInfo);
				return drc;
			}
        } else {
            TRACE(5, "checkPublishAuth: new device connect=%d clientID=%s org=%s devID=%s devType=%s\n",
                    transport->index, clientID, org, devid, devtype);
            ism_proxy_setDeviceAuthInfo(org, devtype, devid, &devInfo);
        }

        asyncAction = transport->has_acl ? AUTH_AutoCreateOnly : AUTH_AuthorizationAutoCreate;
        asyncauth_t * async = createAsyncAuthJob(transport, buf, kind, asyncAction, devtype, devid, mqttAuthorizeCallback);
        async->count = -1;
        async->sendSavedData = sendSavedData;

        if(devInfo){
			asyncauth_t * currInfo = ism_proxy_getDeviceAuthPendingRequest(devInfo, transport->name);

			if (currInfo) {
				//There is a pending auth request already. Add current request to the end of requests list
				while (currInfo->next)
					currInfo = currInfo->next;
				currInfo->next = async;
				ism_proxy_unlockDeviceInfo(devInfo);
				return ISMRC_AsyncCompletion;
			} else {
                ism_proxy_setDeviceAuthPendingRequest(devInfo, async, transport->name);

			}
        }

        pobj->pendingDevsCount++;
        if ((pobj->pendingDevsCount > gwMaxPendingAuthRequests ) || ((pobj->knownDevsCount+pobj->pendingDevsCount) > gwMaxActiveDevices)) {
            TRACE(4, "checkPublishAuth - Too many active devices: connect=%d devInfo=%p org=%s devID=%s devType=%s knownDevsCount=%u pendingDevsCount=%u\n",
                    transport->index, devInfo, org, devid, devtype, pobj->knownDevsCount, pobj->pendingDevsCount);
            __sync_add_and_fetch(&mqttStats.mqttPendingAuthorizationRequests, 1);
            ism_proxy_completeAuthorize(async, ISMRC_TooManyConnections, "Too many active devices");
            ism_proxy_unlockDeviceInfo(devInfo);
            return ISMRC_AsyncCompletion;
        }


        if (g_gatewayRegister == 3) {     //Unit test
            ism_proxy_setDeviceAuthPendingRequest(devInfo, async, transport->name);
            ism_proxy_unlockDeviceInfo(devInfo);
            pobj->pendingDevsCount++;
            __sync_add_and_fetch(&mqttStats.mqttPendingAuthorizationRequests, 1);
            ism_proxy_completeAuthorize(async, 0, NULL);
            return ISMRC_AsyncCompletion;
        }else{
            //This is the first auth request. Submit it to Java and set the info
            __sync_add_and_fetch(&mqttStats.mqttPendingAuthorizationRequests, 1);
            if (ism_proxy_checkAuthorization(async)) {
                ism_proxy_unlockDeviceInfo(devInfo);
                return ISMRC_AsyncCompletion;
            }
        }
        __sync_sub_and_fetch(&mqttStats.mqttPendingAuthorizationRequests, 1);
        pobj->pendingDevsCount--;
        pobj->knownDevsCount++;

        if(devInfo){
            ism_proxy_setDeviceAuthComplete(devInfo, 0, 0, (uint64_t)(ism_common_readTSC()*1000.0), transport->name);
            ism_proxy_unlockDeviceInfo(devInfo);
        }

        ism_common_free(ism_memory_proxy_mqtt_auth,async);
        return ISMRC_OK;
    }
    return ISMRC_OK;
}


/*
 * Check if we need to do an async auth check for subscribe
 * We only do auth-register on subscribe for single topic subscribes
 */
static int checkSubscribeAuth(ism_transport_t * transport, concat_alloc_t * buf, int kind, int subcnt, int sendSavedData) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    const char * devtype = NULL;
    const char * devid = NULL;
    int    rc;
    int topiclen = (uint16_t)BIGINT16(buf->buf+18);

    /* Handle the normal case of 1 topic */
    if (subcnt == 1) {
        char * topic = alloca(topiclen+1);
        memcpy(topic, buf->buf+20, topiclen);
        topic[topiclen] = 0;
        rc = parseTopicAuth(topic, &devtype, &devid);
        if (rc == 0) {
            const char * org = transport->tenant ? transport->tenant->name : NULL;
            dev_info_t * devInfo = ism_proxy_getDeviceAuthInfo(org, devtype, devid);
            uint64_t currTime = (uint64_t)(ism_common_readTSC() * 1000.0);

            if (devInfo) {
            	    asyncauth_t * pendingAuthRequest = ism_proxy_getDeviceAuthPendingRequest(devInfo, transport->name);
                TRACE(7, "checkSubscribeAuth: connect=%d devInfo=%p org=%s devID=%s devType=%s pendingAuthRequest=%p authRC=%d authTime=%llu currTime=%llu gwCleanupTime=%llu sendSavedData=%d\n",
                        transport->index, devInfo, org, devid, devtype, pendingAuthRequest, devInfo->authRC, (ULL)devInfo->authTime,
                        (ULL)currTime, (ULL)g_gwCleanupTime, sendSavedData);

                int drc = ism_proxy_checkDeviceAuth(devInfo, currTime, transport->name);
				if(drc!= -1){
					ism_proxy_unlockDeviceInfo(devInfo);
					return drc;
				}
            } else {
                TRACE(5, "checkSubscribeAuth: new device connect=%d devInfo=%p org=%s devID=%s devType=%s\n",
                        transport->index, devInfo, org, devid, devtype);
                ism_proxy_setDeviceAuthInfo(org, devtype, devid, &devInfo);

            }

            asyncauth_t * async = createAsyncAuthJob(transport, buf, kind, AUTH_AuthorizationAutoCreate, devtype, devid, mqttAuthorizeCallback);
            async->count = -2;
            async->sendSavedData = sendSavedData;


            if(devInfo){
				asyncauth_t * currInfo = ism_proxy_getDeviceAuthPendingRequest(devInfo, transport->name);

				if (currInfo) {
					/* There is a pending auth request already. Add current request to the end of requests list */
					while (currInfo->next)
						currInfo = currInfo->next;
					currInfo->next = async;
					ism_proxy_unlockDeviceInfo(devInfo);
					return ISMRC_AsyncCompletion;
				} else {
					//devInfo->pendingAuthRequest = async;
					ism_proxy_setDeviceAuthPendingRequest(devInfo, async, transport->name);
				}

            }
            pobj->pendingDevsCount++;
            if ((pobj->pendingDevsCount > gwMaxPendingAuthRequests ) || ((pobj->knownDevsCount+pobj->pendingDevsCount) > gwMaxActiveDevices)) {
                TRACE(4, "checkSubscribeAuth - Too many active devices: connect=%d devInfo=%p org=%s devID=%s devType=%s knownDevsCount=%u pendingDevsCount=%u\n",
                        transport->index, devInfo, org, devid, devtype, pobj->knownDevsCount, pobj->pendingDevsCount);
                __sync_add_and_fetch(&mqttStats.mqttPendingAuthorizationRequests, 1);
                ism_proxy_completeAuthorize(async, ISMRC_TooManyConnections, "Too many active devices");
                ism_proxy_unlockDeviceInfo(devInfo);
                return ISMRC_AsyncCompletion;
            }

            /* This is the first auth request. Submit it to Java and set the info */
            if (g_gatewayRegister == 3) {    /* Unit test */
                TRACE(4, "checkSubscribeAuth - submit for authorization for unit test: connect=%d devInfo=%p org=%s devID=%s devType=%s knownDevsCount=%u pendingDevsCount=%u\n",
                     transport->index, devInfo, org, devid, devtype, pobj->knownDevsCount, pobj->pendingDevsCount);
                ism_proxy_setDeviceAuthPendingRequest(devInfo, async, transport->name);
                ism_proxy_unlockDeviceInfo(devInfo);
                pobj->pendingDevsCount++;
                __sync_add_and_fetch(&mqttStats.mqttPendingAuthorizationRequests, 1);
                ism_proxy_completeAuthorize(async, 0, NULL);
                return ISMRC_AsyncCompletion;
            }else{
                __sync_add_and_fetch(&mqttStats.mqttPendingAuthorizationRequests, 1);
                if (ism_proxy_checkAuthorization(async)) {
                        ism_proxy_unlockDeviceInfo(devInfo);
                    return ISMRC_AsyncCompletion;
                }
            }
            __sync_sub_and_fetch(&mqttStats.mqttPendingAuthorizationRequests, 1);
            pobj->pendingDevsCount--;
            pobj->knownDevsCount++;

            if(devInfo){
                ism_proxy_setDeviceAuthComplete(devInfo, 0, 0, (uint64_t)(ism_common_readTSC()*1000.0), transport->name);
                ism_proxy_unlockDeviceInfo(devInfo);
            }

            ism_common_free(ism_memory_proxy_mqtt_auth,async);
            return ISMRC_OK;
        }
        return ISMRC_OK;
    }
    return ISMRC_OK;
}
#endif

/*
 * Return the name of a command given the value.
 * This is used for trace and debug.pxmqtt
 */
const char * ism_mqtt_mqttCommand(int ixin) {
    int ix;
    ixin &= 0xff;
    if (ixin < 0x20 && ixin != 0x10) {
        ix = ixin;
    } else {
        ix = (ixin>>4) & 0xf;
        /* Handle proxy protocol extras */
        if ((ixin&0x0F) == 0xF) {
            switch(ix) {
            case MT_PUBLISH:
                return "PUBLISHX";
            case MT_PINGREQ:
                return "SENDACL";
            case MT_SUBSCRIBE:
                return "SUBSCRIBEX";
            case MT_PUBREC:
                return "GETX";
            case MT_PINGRESP:
                return "SENDSUBS";
            }
        }
    }
    switch (ix) {
    case MT_CONNECT:
        return "CONNECT";
    case MT_CONNACK:
        return "CONNACK";
    case MT_PUBLISH:
        return "PUBLISH";
    case MT_PUBACK:
        return "PUBACK";
    case MT_PUBREC:
        return "PUBREC";
    case MT_PUBREL:
        return "PUBREL";
    case MT_PUBCOMP:
        return "PUBCOMP";
    case MT_SUBSCRIBE:
        return "SUBSCRIBE";
    case MT_SUBACK:
        return "SUBACK";
    case MT_UNSUBSCRIBE:
        return "UNSUBSCRIBE";
    case MT_UNSUBACK:
        return "UNSUBACK";
    case MT_PINGREQ:
        return "PINGREQ";
    case MT_PINGRESP:
        return "PINGRESP";
    case MT_DISCONNECT:
        return "DISCONNECT";
    case MT_AUTH:
        return "AUTH";
    case MTX_PUBLISHX:
        return "PUBLISHX";
    case MTX_SENDACL:
        return "SENDACL";
    case MTX_SUBSCRIBEX:
        return "SUBSCRIBEX";
    case MTX_GETX:
        return "GETX";
    case MTX_SENDSUBS:
        return "SENDSUBS";
    }
    return "UNKNOWN";
}

/*
 * Map from internal RC to MQTT RC
 * This varies by MQTT version
 */
static int mapToMqttRc(int rc, int version, int cp) {
    if (rc == 0)
        return 0;
    if (version >= 5) {
        int  ret;
        switch (rc) {
        default:
            ret = MQTTRC_UnspecifiedError;   break;
        case ISMRC_BadClientData:
        case ISMRC_BadLength:
        case ISMRC_InvalidValue:
        case ISMRC_WillRequired:
            ret = MQTTRC_MalformedPacket;    break;
        case ISMRC_InvalidPayload:
            ret = MQTTRC_PayloadInvalid;     break;
        case ISMRC_TooManyProdCons:
        case ISMRC_ShareMismatch:
        case ISMRC_Failure:
            ret = MQTTRC_ImplError;          break;
        case ISMRC_ClientIDReused:
            ret = MQTTRC_SessionTakenOver;   break;
        case ISMRC_ServerTerminating:
            ret = MQTTRC_ServerShutdown;     break;
        case ISMRC_EndpointDisabled:
            ret = MQTTRC_AdminAction;        break;

        case ISMRC_BadTopic:
        case ISMRC_BadSysTopic:
            if (cp & (CPOI_SUBACK | CPOI_UNSUBACK))
                ret = MQTTRC_TopicFilterInvalid;
            else
                ret = MQTTRC_TopicInvalid;
            break;

        case ISMRC_ProtocolError:
        case ISMRC_UnicodeNotValid:
        case ISMRC_InvalidCommand:
        case ISMRC_InvalidQoS:
            ret = MQTTRC_ProtocolError;      break;
        case ISMRC_ReceiveMaxExceeded:
            ret = MQTTRC_ReceiveMaxExceeded; break;

        case ISMRC_TopicAliasNotValid:
            ret = MQTTRC_TopicAliasInvalid;  break;
        case CRC_InvalidVersion:
            ret = MQTTRC_UnknownVersion;     break;
        case CRC_BadIdentifier:
            ret = MQTTRC_IdentifierNotValid; break;
        case CRC_NotAvailable:
            ret = MQTTRC_ServerUnavailable;  break;
        case CRC_BadUser:
            ret = MQTTRC_BadUserPassword;    break;
        case CRC_NotAuthorized:
        case ISMRC_NotAuthorized:
        case ISMRC_ClientIDInUse:
            ret = MQTTRC_NotAuthorized;     break;
        }
        if (!(ism_mqtt_reasonCodeAllowed(ret) & cp))
            ret = MQTTRC_UnspecifiedError;
        return ret;
    } else {
        if (rc > CRC_MinConnectRc && rc <= CRC_MaxConnectRc)
            return rc;
        if (rc == ISMRC_NotAuthorized || rc == ISMRC_ClientIDInUse)
            return CRC_NotAuthorized;
        return 0x80;   /* Only proxy protocol */
    }
}

/*
 * Map an MQTT Return code to an ISMRC
 */
int mapToIsmRC(int mqttrc, int version) {
    if (mqttrc == 0)
        return 0;
    if (version >= 5) {
        if (mqttrc < 128)
            return 0;
        switch(mqttrc) {
        case MQTTRC_UnspecifiedError:   /* 0x80 Unspecified error */
            return ISMRC_Error;
        case MQTTRC_MalformedPacket:    /* 0x81 The packet is malformed */
            return ISMRC_BadClientData;
        case MQTTRC_ProtocolError:      /* 0x82 Protocol error */
            return ISMRC_ProtocolError;
        case MQTTRC_ImplError:          /* 0x83 Implementation specific error */
            return ISMRC_Failure;
        case MQTTRC_UnknownVersion:     /* 0x84 Unknown MQTT version */
            return ISMRC_ProtocolVersion;
        case MQTTRC_IdentifierNotValid: /* 0x85 ClientID not valid */
            return ISMRC_BadClientID;
        case MQTTRC_BadUserPassword:    /* 0x86 Username or Password not valid */
            return ISMRC_NotAuthorized;
        case MQTTRC_NotAuthorized:      /* 0x87 Not authorized */
            return ISMRC_NotAuthorized;
        case MQTTRC_ServerUnavailable:  /* 0x88 The server in not available */
            return ISMRC_ServerNotAvailable;
        case MQTTRC_ServerBusy:         /* 0x89 The server is busy */
            return ISMRC_MessagingNotAvailable;
        case MQTTRC_ServerShutdown:     /* 0x8B The server is being shut down */
            return ISMRC_ServerTerminating;
        case MQTTRC_SessionTakenOver:   /* 0x8E The session has been taken over */
            return ISMRC_ClientIDReused;
        case MQTTRC_KeepAliveTimeout:   /* 0x8F The Keep Alive time has been exceeded */
            return ISMRC_ConnectTimedOut;
        case MQTTRC_TopicInvalid:       /* 0x90 The topic is not valid for this server */
            return ISMRC_BadTopic;
        case MQTTRC_PacketIDInUse:      /* 0x91 The PacketID is in use */
            return ISMRC_Error;
        case MQTTRC_PacketIDNotFound:   /* 0x92 The PacketID is not found */
            return ISMRC_Error;
        case MQTTRC_PacketTooLarge:     /* 0x95 The packet is too large */
            return ISMRC_MsgTooBig;
        case MQTTRC_AdminAction:        /* 0x98 The connection is closed due to an administrative action */
            return ISMRC_EndpointDisabled;
        case MQTTRC_ReceiveMaxExceeded:
            return ISMRC_ReceiveMaxExceeded;
        default:
            return ISMRC_Error;
        }
    } else {
        switch (mqttrc) {
        case CRC_InvalidVersion:
            return ISMRC_ProtocolVersion;
        case CRC_BadIdentifier:
            return ISMRC_BadClientID;
        case CRC_NotAvailable:
            return ISMRC_ServerNotAvailable;
        case CRC_BadUser:
        case CRC_NotAuthorized:
            return ISMRC_NotAuthorized;
        }
    }
    return ISMRC_Error;
}

/*
 * External version of map RC
 */
int ism_proxy_mapToIsmRC(int mqttrc, int version) {
    return mapToIsmRC(mqttrc, version);
}

int ism_proxy_mapToMqttRC(int ismrc, int version) {
    return mapToMqttRc(ismrc, version, 0);
}

#ifndef NO_PROXY
/*
 * Continue with error on Connect
 */
static void mqttConnectErrorContinue(mqttMsg_t * msg) {
	ism_transport_t * transport = msg->transport;
	mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;

	char response[1024];
	char xbuf[200];
	concat_alloc_t rbuf = {response, sizeof response};

	if (msg->rc) {
		TRACE(5, "mqttConnectError: rc=%s (%d) connect=%u clientID=%s\n",
				getMQTTErrorString(msg->rc, xbuf, sizeof xbuf), msg->rc, transport->index, transport->name);
	}

	if (LIKELY(msg->rc == 0 || transport->pobj->mqtt_version >= 5 ||
	        msg->rc == ISMRC_NotAuthorized || msg->rc == ISMRC_ClientIDInUse ||
	        (msg->rc >= CRC_MinConnectRc && msg->rc <= CRC_MaxConnectRc))) {
	    int reasonlen = 0;
		/* This can be only OK, CRC_BadIdentifier, or CRC_NotAuthorized */
		if (msg->rc != 0) {
			pobj->startState = MQTT_NOT_CONNECTED;
		}

		response[16] = 0;
		response[17] = (char) mapToMqttRc(msg->rc, transport->pobj->mqtt_version, CPOI_CONNACK);
		response[18] = 0;
		rbuf.used = 18;
		if (transport->pobj->mqtt_version >= 5) {
		    char * erbuf = alloca(2048);
            ism_common_formatLastError(erbuf, 2048);
            reasonlen = strlen(erbuf);
            if (reasonlen && (!transport->pobj->maxPacketSize || (reasonlen+11 < transport->pobj->maxPacketSize))) {
                ism_common_putMqttVarInt(&rbuf, reasonlen+3);
                ism_common_putMqttPropString(&rbuf, MPI_Reason, g_ctx5, erbuf, reasonlen);
            } else {
                rbuf.used++;         /* proplen = 0 */
            }
		}
		transport->send(transport, rbuf.buf + 16, rbuf.used - 16, MT_CONNACK << 4, SFLAG_SYNC | SFLAG_FRAMESPACE);
		ism_common_freeAllocBuffer(&rbuf);
		/*
		 * Close the connection in the case of invalid CONNECT
		 * If close is in progress, proceed with it instead of this close
		 */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress due to bad connect: connect=%u inprogress=%d\n", transport->index, pobj->inprogress);
#endif
		if (__builtin_expect((__sync_sub_and_fetch(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
			ism_mqtt_doneConnection(transport);
			return ;
		}

		if (msg->rc) {
			/*
			 * Close the connection for v3.1.1 and above
			 */
			if (msg->rc == CRC_InvalidVersion || msg->transport->pobj->mqtt_version >= 3) {
				__sync_add_and_fetch(&transport->endpoint->stats->bad_connect_count, 1);
				switch (msg->rc) {
				case CRC_InvalidVersion: msg->rc = ISMRC_ProtocolVersion;     break;
				case CRC_BadIdentifier:  msg->rc = ISMRC_BadClientID;         break;
				case CRC_BadUser:
				case CRC_NotAuthorized:  msg->rc = ISMRC_NotAuthorized;       break;
                case CRC_NotAvailable:   msg->rc = ISMRC_ServerNotAvailable;  break;
				}
				if (msg->reason) {
				    transport->close(transport, msg->rc, 0, msg->reason);
				} else {
                    if (msg->rc == ism_common_getLastError())
                        ism_common_formatLastError(xbuf, sizeof xbuf);
                    else
                        ism_common_getErrorString(msg->rc, xbuf, sizeof xbuf);
                    transport->close(transport, msg->rc, 0, xbuf);
				}
			} else {
			    /* Wait for the client to close the connection */
				__sync_add_and_fetch(&transport->endpoint->stats->bad_connect_count, 1);
				transport->closestate[3] = (uint8_t)msg->rc;
			}
		}
	}

	/*
	 * For pre MQTTv5 if the error is not one of the standard errors,
	 * just close the connection
	 */
	else {
        /* If close is in progress, proceed with it instead of this close */
#ifdef DEBUG_INPROGRESS
	    TRACE(9, "Decrement inprogress for an error: connect=%u inprogress=%d\n", transport->index, pobj->inprogress);
#endif
        if (__builtin_expect((__sync_sub_and_fetch(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
            ism_mqtt_doneConnection(transport);
        } else {
            __sync_add_and_fetch(&transport->endpoint->stats->bad_connect_count, 1);
            if (msg->reason) {
                transport->close(transport, msg->rc, 0, msg->reason);
            } else {
                if (msg->rc == ism_common_getLastError())
                    ism_common_formatLastError(xbuf, sizeof xbuf);
                else
                    ism_common_getErrorString(msg->rc, xbuf, sizeof xbuf);
                transport->close(transport, msg->rc, 0, xbuf);
            }
        }
	}
}


/*
 * Process an MQTT error.
 *
 * If this is one of the connection returns, send the CONNACK, otherwise
 * close the connection.  This is also used to sent the CONACK for a
 * good connection.
 */
static void mqttConnectError(mqttMsg_t * msg) {
    ism_transport_t * transport = msg->transport;
    if (ism_throttle_isEnabled() && msg->rc != CRC_NotAvailable)
        ism_throttle_incrementAuthFailedCount(transport->name);
    mqttConnectErrorContinue(msg);
    return ;
}


/*
 * Receive a connection closing notification for the MQTT protocol.
 * We start the closing and it is completed in replyClosed().
 */
int ism_mqtt_closing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    int32_t count;
    char response [1024];

    TRACE(8, "ism_mqtt_closing: connect=%u client=%s transport_ready=%d transport_originated=%d inprogress=%d rc=%d clean=%d reason=%s \n",
            transport->index, transport->name, transport->ready,  transport->originated,pobj->inprogress,  rc, clean, reason);

    if (pobj == NULL) /* Connection was broken during initConnection phase */
        return 0;

    //NOTE:
    //During CONNECT, we don't decrement the inprogress. Assume the return of CONNACK will decrement it or
    //if the connection was terminated by MessageGateway or by client before the CONNECT sent
    //
    //when client transport->ready = 2 means Auth is completed and possible CONNECT sent to MG. Connection
    //can still be terminated by client or MG. In that case, need to decremented.

    if(!transport->originated && !pobj->closed && transport->ready == 2 ){
#ifdef DEBUG_INPROGRESS
        TRACE(9, "Decrement inprogress at close after Auth and before CONNECT sent: connect=%u inprogress=%d\n", transport->index, pobj->inprogress);
#endif
        __sync_sub_and_fetch(&pobj->inprogress, 1);
    }

    /* Send a server DISCONNECT */
    if (pobj->client_transport && pobj->send_disconnect && pobj->mqtt_version >= 5) {
        int was_send_disconnect = pobj->send_disconnect;
        pobj->send_disconnect = 0;
        concat_alloc_t rbuf = {response, sizeof response};

        response[16] = mapToMqttRc(rc, pobj->mqtt_version, CPOI_DISCONNECT);
        response[17] = 0;
        rbuf.used = 17;
        if (rc && reason) {
            int reasonlen = strlen(reason);
            if (reasonlen && (!transport->pobj->maxPacketSize || (reasonlen+11 < transport->pobj->maxPacketSize))) {
                ism_common_putMqttVarInt(&rbuf, reasonlen+3);
                ism_common_putMqttPropString(&rbuf, MPI_Reason, g_ctx5, reason, reasonlen);
            } else {
                rbuf.used++;            /* proplen = 0 */
            }
        }
        if (pobj->client_transport)
            pobj->client_transport->send(pobj->client_transport, rbuf.buf + 16, rbuf.used-16, MT_DISCONNECT << 4, SFLAG_SYNC | SFLAG_FRAMESPACE);
        if (was_send_disconnect == 2)
            transport->send(transport, rbuf.buf + 16, rbuf.used-16, MT_DISCONNECT << 4, SFLAG_SYNC | SFLAG_FRAMESPACE);

        if (rbuf.inheap)
            ism_common_freeAllocBuffer(&rbuf);
    }

    /*
     * Set the indicator that close is in progress. If set failed,
     * then this has been done before and we don't need to proceed.
     */
    if (!__sync_bool_compare_and_swap(&pobj->closed, 0, 1)) {
        return 0;
    }
    transport->close_rc = rc;
    if (rc && reason && *reason && !transport->reason)
        transport->reason = ism_transport_putString(transport, reason);

    /*
     * Subtract the "in progress" indicator. If it becomes negative,
     * no actions are in progress, so it is safe to clean up protocol data
     * and close the connection. If it is non-negative, there are
     * actions in progress. The action that sets this value to 0
     * would re-invoke closing().
     */
#ifdef DEBUG_INPROGRESS
    TRACE(9, "Decrement inprogress at close: connect=%u inprogress=%d\n", transport->index, pobj->inprogress);
#endif
    count = __sync_sub_and_fetch(&pobj->inprogress, 1);
    if (count >= 0) {
        TRACE(8, "ism_mqtt_closing postponed as there are %d actions/messages in progress: connect=%u client=%s\n",
                count+1, transport->index, transport->name);
        return 0;
    }

    pobj->clean = clean;

    /* Stop message delivery, destroy session and client state */
    ism_mqtt_doneConnection(transport);

    return 0;
}


/*
 * Get the protocol in external format
 */
const char * ism_mqtt_externalProtocol(ism_transport_t * transport, char * buf) {
    *buf = 0;
    if (transport->protocol_family && !strcmp(transport->protocol_family, "mqtt")) {
        sprintf(buf, "mqtt%u", transport->pobj->mqtt_version);
        if (transport->pobj->prot == PROT_MQTT_WSBIN)
            strcat(buf, "-ws");
        return buf;
    }
    return transport->protocol_family;
}


/*
 * The engine connection is closed, close our connection, and tell the transport it can close its connection
 */
void ism_mqtt_doneConnection(ism_transport_t * transport) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    char xbuf[256];
    const char * reason;
    int  rc = transport->close_rc;
    /*
     * Set the indicator that close is complete. If set failed,
     * then this has been done before and we don't need to proceed.
     */
    if (!__sync_bool_compare_and_swap(&pobj->closed, 1, 2)) {
        return;
    }

    reason = transport->reason;
    if (reason == NULL) {
        if (rc == 0) {
            reason = "The connection has completed normally";
        } else {
            ism_common_getErrorString(rc, xbuf, sizeof xbuf);
            reason = xbuf;
        }
    }

    /* On Normal Termination, clean up the logged table. */
	if (!transport->originated) {
	    /* Remove transport from ACL */
		if(transport->has_acl){
			const char * aclKey = ism_proxy_getACLKey(transport);;
			ism_acl_t * acl = ism_protocol_findACL(aclKey, 0, NULL);
			if (acl) {
				TRACE(8, "Remove connection from ACL: connect=%u client=%s rc=%u was=%p\n",
						transport->index, aclKey, rc, acl->object);
				if (acl->object == (void *)transport)
					acl->object = NULL;
				ism_protocol_unlockACL(acl);
			}
		}

		if (rc==0) {
			ism_log_removeConditionallyLoggedEntries(transport->clientID, transport->client_addr);
			ism_throttle_setLastCloseRC(transport->clientID, rc);
		} else if (rc==ISMRC_ClientIDReused) {
			ism_throttle_incrementClienIDStealCount(transport->clientID);
		} else if (rc != ISMRC_ClosedByClient && rc != ISMRC_ClosedByServer && rc!= ISMRC_NotAuthorized && rc!= ISMRC_ServerNotAvailable) {
				ism_throttle_incrementConnCloseError(transport->clientID, rc);
		} else {
			ism_throttle_setLastCloseRC(transport->clientID, rc);
		}
	}


    if (transport->pobj->client_transport) {
        TRACE(8, "ism_mqtt_doneConnection: Closing Client Transport: connect=%u client=%s connectPending=%d\n",
                                transport->index, transport->clientID, transport->pobj->connectPending);
        if (ism_mqtt_connected(transport, rc)){
            transport->pobj->client_transport->close(transport->pobj->client_transport, rc, rc==0, reason);
            TRACE(8, "ism_mqtt_doneConnection: Close Client Transport completed: connect=%u client=%s\n",
                                           transport->index, transport->clientID);
        }
    }

    if (transport->pobj->server_transport){
        TRACE(8, "ism_mqtt_doneConnection: Closing Server Transport: connect=%u client=%s\n",
                                       transport->index, transport->clientID);
        transport->pobj->server_transport->close(transport->pobj->server_transport, rc, rc==0, reason);
        TRACE(8, "ism_mqtt_doneConnection: Closing Server Transport completed: connect=%u client=%s\n",
                                             transport->index, transport->clientID);

    }

    /* For testing only, close hout connection */
    if (mqtt_unit_test) {
        if (transport->hout)
            iot_doneConnection(transport, rc, reason);
    }

    /* Do the WebSockets close */
    if (rc==0 && !strcmp(transport->protocol, "mqttv3.1") && !strcmp(transport->protocol, "mqtt")) {
        ism_transport_closeWS(transport, 1000);
    }
    TRACE(7, "close MQTT connection: connect=%u client=%s msgCount=%llu\n", transport->index, transport->name, (ULL)transport->pobj->messageCount);

    if (pobj->savelen) {
        pobj->savelen = 0;
        ism_common_free(ism_memory_proxy_mqtt_savedata,pobj->savedata);
        pobj->savedata = NULL;
    }
    if (pobj->connectlen) {
        pobj->connectlen = 0;
        ism_common_free(ism_memory_proxy_mqtt_connection,pobj->connectdata);
        pobj->connectdata = NULL;
    }
    if (pobj->morealloc) {
        pobj->morealloc = pobj->morelen = 0;
        ism_common_free(ism_memory_proxy_mqtt_receive,pobj->morebuf);
        pobj->morebuf = NULL;
    }


    pthread_spin_destroy(&pobj->lock);

    if (!transport->originated) {
        int mqttConnections = __sync_sub_and_fetch(&mqttStats.mqttConnections, 1);
        TRACE(9, "ism_mqtt_doneConnection: Close mqtt connect=%u client=%s mqttConnections=%d\n",
                transport->index, transport->name, mqttConnections);
        switch (transport->client_class) {
        case'd':
            __sync_sub_and_fetch(&mqttStats.mqttConnectedDev, 1);
            break;
        case 'c':
        case 'g':
            __sync_sub_and_fetch(&mqttStats.mqttConnectedGWs, 1);
            break;
        case 'a':
        case 'A':
            __sync_sub_and_fetch(&mqttStats.mqttConnectedApp, 1);
            break;
        default:
            break;
        }
        transport->client_class = '\0';
    }

    /* Tell the transport we are done */
    transport->closed(transport);
}

/*
 * Copy user properties
 */
static int mpropCopyUserProps(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value) {
    concat_alloc_t * buf = (concat_alloc_t *)userdata;
    if (fld->type == MPT_NamePair && fld->id == MPI_UserProperty) {
        int namelen = BIGINT16(ptr);
        int valuelen = BIGINT16(ptr+2+namelen);
        if ((((uint8_t)buf->compact) + namelen + valuelen + 8) < 128)
            ism_common_putMqttPropNamePair(buf, MPI_UserProperty, g_ctx5, ptr+2, namelen, ptr+4+namelen, valuelen);
    }
    return 0;
}

/*
 * Check an MQTTv5 identifier/value pair string
 */
static int mpropCheckString(mqtt_prop_field_t * fld, const char * ptr, int len) {
    if (ism_common_validUTF8Restrict(ptr, len, UR_NoControl | UR_NoNonchar) < 0) {
        char * out;
        if (len > 256)
            len = 256;
        out = alloca(len+1);
        ism_common_validUTF8Replace(ptr, len, out, UR_NoControl | UR_NoNonchar, '?');
        ism_common_setErrorData(ISMRC_UnicodeNotValid, "%s%s", fld->name, out);
        return ISMRC_UnicodeNotValid;
    }
    return 0;
}


/*
 * Validate an MQTTv5 namepair (user property)
 * The lengths are already validated
 */
static int validateNamePair(const char * ptr, int len) {
    int namelen = BIGINT16(ptr);
    int valuelen = BIGINT16(ptr+2+namelen);
    char * outname;
    char * outval;
    if (ism_common_validUTF8Restrict(ptr+2, namelen, UR_NoNull)<0 ||
        ism_common_validUTF8(ptr+4+namelen, valuelen) < 0) {
        /* Make the error string valid */
        int shortname = namelen;
        if (shortname > 256)
            shortname = 256;
        outname = alloca(shortname+1);
        if (valuelen > 256)
            valuelen = 256;
        outval = alloca(valuelen+1);
        ism_common_validUTF8Replace(ptr+2, shortname, outname, UR_NoControl | UR_NoNonchar, '?');
        ism_common_validUTF8Replace(ptr+4+namelen, valuelen, outval, UR_NoControl | UR_NoNonchar, '?');
        ism_common_setErrorData(ISMRC_UnicodeNotValid, "%s%s", outname, outval);
        return ISMRC_UnicodeNotValid;
    }
    return 0;
}


/*
 * Check for MQTTv5 properties
 */
static int mpropCheck(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value) {
    mqttMsg_t * mmsg = (mqttMsg_t *) userdata;
    if (fld->type == MPT_String) {
        int rc = mpropCheckString(fld, ptr, len);
        if (rc)
            return rc;
        switch (fld->id) {
        case MPI_Reason:
            if (mmsg) {
                /* Store reason in transport object */
                char * rp = ism_transport_allocBytes(mmsg->transport, len+1, 0);
                memcpy(rp, ptr, len);
                rp[len] = 0;
                mmsg->reason = (const char *)rp;
            }
        }
    }
    if (fld->type == MPT_NamePair) {
        if (mmsg)
            mmsg->has_user_props = 1;
        return validateNamePair(ptr, len);
    }
    switch (fld->id) {
    case MPI_PayloadFormat:
        if (value>2) {
            ism_common_setErrorData(ISMRC_ProtocolError, "%s%u", fld->name, value);
            return ISMRC_ProtocolError;
        }
        if (mmsg)
            mmsg->msgfmt = (uint8_t)value;
        break;
    case MPI_MsgExpire:
    case MPI_SessionExpire:
        if (mmsg) {
            mmsg->isExpire = 1;
            mmsg->expireTTL = value;
        }
        break;
    case MPI_ReplyTopic:
        if (ism_common_validUTF8Restrict(ptr, len, UR_NoControl | UR_NoWildcard) < 0) {
            ism_common_setErrorData(ISMRC_ProtocolError, "%s", fld->name);
            return ISMRC_ProtocolError;
        }
        break;
    case MPI_MaxReceive:
         if (value == 0) {
            ism_common_setErrorData(ISMRC_ProtocolError, "%s%u", fld->name, value);
            return ISMRC_ProtocolError;
        }
        if (mmsg)
            mmsg->maxActive = value;
        break;
    case MPI_MaxTopicAlias:
        if (mmsg)
            mmsg->topic_alias = value;
        break;
    case MPI_TopicAlias:
        if (value == 0) {
            ism_common_setErrorData(ISMRC_ProtocolError, "%s%u", fld->name, value);
            return ISMRC_ProtocolError;
        }
        if (mmsg) {
            mmsg->topic_alias = value;
            mmsg->topic_alias_loc = (char *)(ptr-1);
        }
        break;
    case MPI_RequestReplyInfo:
        if (value>1) {
            ism_common_setErrorData(ISMRC_ProtocolError, "%s%u", fld->name, value);
            return ISMRC_ProtocolError;
        }
        if (mmsg)
            mmsg->request_reply_info = value;
        break;
    case MPI_RequestReason:
        if (value > 1) {
            ism_common_setErrorData(ISMRC_ProtocolError, "%s%u", fld->name, value);
            return ISMRC_ProtocolError;
        }
        if (mmsg)
            mmsg->request_reason = value;
        break;
    case MPI_MaxPacketSize:
        if (value < 64) {
            ism_common_setErrorData(ISMRC_Error, "%s", "The Maximum Packet Size is too small");
            return ISMRC_Error;
        }
        if (mmsg)
            mmsg->maxPacketSize = value;
        break;
    case MPI_KeepAlive:
        if (mmsg)
            mmsg->keepalive = (int32_t)value;
        break;

    case MPI_AuthMethod:    /* Extended auth not supported */
    case MPI_AuthData:
        ism_common_setErrorData(ISMRC_NotAuthorized, "%s", fld->name);
        return ISMRC_NotAuthorized;
    }
    return 0;
}



/*
 * Parse and get the MQTT Properties from the buffer containing an MQTT PUBLISH packet.
 *
 * The buffer does not contain the fixed header which is sent in the kind parameter.
 *
 * Note: This code assumes that the PUBLISH packet in the buffer has already been
 * valiadated and does not check for validity.
 *
 * The buffer Contains:
 * - The topic length (2 byte big endian)
 * - topic (topic_len bytes of UTF-8 data)
 * - If QoS>0
 *   - A packet ID (2 bytes big endian)
 * - If MQTT version >= 5 a properties section
 *   - THe properties length as a variable byte integer (not including itself)
 *   - The properties
 * - The payload
 */
int ism_mqtt_getPublishPayload(ism_transport_t * transport, mqtt_pmsg_t * pmsg) {
    char * bp = (char *)pmsg->buf;
    int  qos;
    int topiclen = (uint16_t)BIGINT16(bp);
    bp += 2;        /* Skip over the topic length */
    pmsg->topic_len = topiclen;
    pmsg->topic = bp;
    bp += topiclen;              /* Skip over the topic */

    qos = (uint8_t)((pmsg->cmd >> 1) & 3);
    if (qos > 0) {
        pmsg->packetID = (uint16_t)BIGINT16(bp);
        bp += 2;               /* Skip over the packet ID */
    }

    pmsg->props = NULL;
    pmsg->prop_len = 0;
    if (transport->pobj->mqtt_version >= 5) {
        if (*bp) {
            int vilen;
            int proplen = ism_common_getMqttVarIntExp(bp, 4, &vilen);
            bp += vilen;         /* Skip over property length */
            pmsg->props = bp;
            pmsg->prop_len = proplen;
            bp += proplen;       /* Skip over the properties */
        } else {
            bp++;                /* Skip over the zero property length */
        }
    }

    pmsg->payload = bp;
    pmsg->payload_len = (int)(pmsg->buflen - (bp-(char *)pmsg->buf));
    return 0;
}


/*
 * Parse PUBLISHX buffer.
 *
 * Note: This code assumes that the PUBLISH packet in the buffer has already been
 * valiaated and does not check for validity.
 *
 * The buffer Contains:
 * - command (1 byte)
 * - if command has extensions (0x20)
 *   - 2 byte length
 *   - extension string
 * - The topic length (2 byte big endian)
 * - topic (topic_len bytes of UTF-8 data)
 * - A packet/message ID (2 bytes big endian)
 * - The payload
 */
int ism_mqtt_getPublishXPayload(ism_transport_t * transport, mqtt_pmsg_t * pmsg) {
    char * bp = (char *)pmsg->buf;
    bp++;  /* skip the kind or cmd */

    /* Process extension */
    if (pmsg->cmd & 0x20) {
    	int ext_len = BIGINT16(bp);
    	bp +=2;          /* skip the length */
    	bp+= ext_len;    /* skip over the extension */
    }


    int topiclen = (uint16_t)BIGINT16(bp);
    bp += 2;                     /* Skip over the topic length */
    pmsg->topic_len = topiclen;
    pmsg->topic = bp;
    bp += topiclen;              /* Skip over the topic */
    pmsg->packetID = (uint16_t)BIGINT16(bp);
    bp += 2;                 /* Skip over the packet ID */

    pmsg->payload = bp;
    pmsg->payload_len = (int)(pmsg->buflen - (bp-(char *)pmsg->buf));
    return 0;
}


/*
 * Send an MQTT ack
 *
 * For now we only send good ACK
 */
int ism_mqtt_ack(ism_transport_t * transport, int waitValue, int rc, const char * reason) {
    char xbuf[32];
    xbuf[16] = (char)(waitValue>>8);
    xbuf[17] = (char)(waitValue);
    transport->send(transport, xbuf+16, 2, (waitValue&0x10000) ? MT_PUBREC<<4 : MT_PUBACK<<4, SFLAG_FRAMESPACE);
    return 0;
}


/*
 * Starting with a formatted MQTT message, parse it for routing
 */
static int routeMhubMessage(ism_transport_t * transport, int kind, const char * buf, int buflen) {
    int  pubCount = 0;
    int  rc;
    mqtt_pmsg_t pmsg = {buf, buflen, kind};

    if (!transport->tenant ||  !transport->tenant->mhublist)
        return 0;

    if (kind == 0x3f) {
        pmsg.cmd = *buf;
        ism_mqtt_getPublishXPayload(transport, &pmsg);
    } else {
        ism_mqtt_getPublishPayload(transport, &pmsg);
    }
    pmsg.waitValue = pmsg.packetID;   /* Return packetID as ACK */
    if (((pmsg.cmd>>1)&3) == 2)
        pmsg.waitValue |= 0x10000;    /* QoS 2 */

    /* Null terminate the topic */
    char * topic = alloca(pmsg.topic_len+1);
    memcpy(topic, pmsg.topic, pmsg.topic_len);
    topic[pmsg.topic_len] = 0;

    /* Parse the standard topic fields */
    char * ttopic = alloca(pmsg.topic_len+1);
    memcpy(ttopic, pmsg.topic, pmsg.topic_len);
    ttopic[pmsg.topic_len] = 0;
    rc = ism_proxy_parseMQTTTopic(ttopic, &pmsg.type, &pmsg.id, &pmsg.event, &pmsg.format);
    if (rc == 0 || rc == 3) {   /* Event or Command */
        pubCount += ism_route_mhubMessage(transport, &pmsg);
    }
    if (pubCount == 0 && pmsg.waitValue) {
        transport->ack(transport, pmsg.waitValue, 0, NULL);  /* ACK as non-selected */
    }
    return pubCount;
}
/**
 * Validate the Publish topic
 *
 *
 */
static int revalidatePublish(ism_transport_t * transport, int kind, char * pbuf, int pbuflen, int * in_sendit)
{

    ism_topicrule_t * topicrule = NULL;
    ism_pxrule_t * * rules;
    int topiclen=0;
    int rulecount=0;
    char * topic=NULL;
    int i=0;
    int rc=0;
    int sendit=1;
    char * reason=NULL;
    mqttProtoObj_t * pobj = transport->pobj;

    char * tbp =pbuf;
    //int tlen = pbuflen;

    topicrule = ism_proxy_getTopicRuleClass(transport->tenant, transport->client_class);

    rules = topicrule->publish;
    rulecount = topicrule->publish_count;
    topiclen = (uint16_t)BIGINT16(tbp);
    topic = alloca(topiclen+1);
    memcpy(topic, tbp+2, topiclen);
    topic[topiclen] = 0;

    //tbp+=2;
    //tlen-=2;

    TRACE(9, "revalidatePulish: connect=%d clientID=%s topic=%s\n", transport->index, transport->clientID, topic);

    /* Convert the topic to the external format */
    char xbuf [2048];
    concat_alloc_t topic_out_buf = {xbuf, sizeof xbuf};
    convertTopicOut(transport, &topic_out_buf, topic, topiclen);
    char * topic_out = alloca((topic_out_buf.used-2)+1);
    int topiclen_out = topic_out_buf.used-2;
    memcpy(topic_out, topic_out_buf.buf+2, topiclen_out);
    topic_out[topiclen_out]='\0';
    TRACE(9, "revalidatePulish: connect=%d clientID=%s command=%s topic=%s permissions=%d topic_len=%d topic_out=%s topic_out_len=%d inprogress=%d\n",
           transport->index, transport->clientID, ism_mqtt_mqttCommand(kind), topic, transport->auth_permissions, topiclen, topic_out, topiclen_out, pobj->inprogress);

    //Check topic validity and permission
    int matched = 0;
    for (i=0; i<rulecount; i++) {
       if (matchTopicRule(transport, rules[i], topic_out_buf.buf+2, topic_out_buf.used-2)) {
           matched = 1;
           break;
       }
    }

    TRACE(9, "revalidatePulish: match rule: connect=%d clientID=%s command=%s topic=%s matched=%d\n",
                  transport->index, transport->clientID, ism_mqtt_mqttCommand(kind), topic, matched);
    if(matched){
       /* Check the ACL for permission of device type and device id*/
      if (transport->has_acl) {
          rc = checkACL(transport, tbp);
          if (rc){
              TRACE(5, "revalidatePublish: checkACL failed for the topic: connect=%d clientID=%s topic=%s\n",
                               transport->index, transport->clientID, topic);
              if (rc) {
                  ism_common_setErrorData(rc, "%s%s", "Device not in group: ", topic_out);
              }
              matched = 0;
              sendit=0;
          }else{
              TRACE(9, "revalidatePublish: checkACL succeeded for the topic: connect=%d clientID=%s topic=%s\n",
                                               transport->index, transport->clientID, topic);
          }
      }else{
          TRACE(9, "revalidatePublish: transport has no ACL: connect=%d clientID=%s topic=%s\n",
                                                             transport->index, transport->clientID, topic);
      }

    }else{
        sendit=0;
        TRACE(5, "revalidatePublish The topic does not match an authorized rule=\"%s\" clientID=\"%s\" From=%s:%u, connect=%u\n",
                topic_out, transport->name, transport->client_addr, transport->clientport, transport->index);
        rc = ISMRC_BadTopic;
        reason = "The topic does not match an authorized rule";
        ism_common_setErrorData(rc, "%s%s", topic, reason);
    }

    if(transport->client_class == 'g'){
        int shouldNotify = 1;
        int isCheckPublish =0;
        /*Note:
         * 2 Cases to Handle
         *1. If Priviledged GW, Always do checkPublishAuth (which will do AutoRegister if it is not there)
         *2. If non-Priviledged GW, If not in ACLs and the GW has the permission (of 200), it will call checkPublishAuth (which will do AutoRegister)
         */
        if(!rc || rc == ISMRC_NotAuthorized ){
            TRACE(9, "revalidatePublish: Gateway authorization: connect=%d clientID=%s topic=%s rc=%d has_acl=%d inprogress=%d g_gatewayRegister=%d\n",
                             transport->index, transport->clientID, topic, rc, transport->has_acl, pobj->inprogress, g_gatewayRegister);

            if (!rc && transport->has_acl) {
                rc = checkACL(transport, tbp);
                if (rc) {
                    ism_common_setErrorData(rc, "%s%s", "Device not in group: ", topic_out);
                }
            }else if (g_gatewayRegister) {
                if(!rc){
                    /* If we are in the process of closing, return closed */
                    #ifdef DEBUG_INPROGRESS
                    TRACE(9, "Increment inprogress in revalidatePublish: connect=%u inprogress=%d\n", transport->index, pobj->inprogress+1);
                    #endif
                    if (__sync_fetch_and_add(&pobj->inprogress, 1) < 0) { /* BEAM suppression: constant condition */
                         __sync_fetch_and_sub(&pobj->inprogress, 1);
                         rc = ISMRC_Closed;
                         ism_common_setError(ISMRC_Closed);
                         return rc;  //No need to process further. Assume the main process of _receive will decrement more and closing out the connection
                    }
                    char * cpa_xbuf = alloca(pbuflen+256);
                    concat_alloc_t cpa_buf = {cpa_xbuf,  pbuflen+16, 16};
                    ism_common_allocBufferCopyLen(&cpa_buf, pbuf, pbuflen);

                    rc = checkPublishAuth(transport, &cpa_buf, kind, rc, 1);
                    isCheckPublish=1;
                    shouldNotify=0;
                    TRACE(9, "revalidatePublish: checkPublishAuth: connect=%d clientID=%s topic=%s rc=%d\n",
                                                                         transport->index, transport->clientID, topic, rc);
                    ism_common_freeAllocBuffer(&cpa_buf);

                }else{
                    if ((rc == ISMRC_NotAuthorized) && (transport->auth_permissions & 0x200)) {
                        TRACE(9, "revalidatePublish: Gateway CheckPublish: CONNACK is completed. checkPublishAuth continue. connect=%u client=%s has_acl=%d \n",
                                                                               transport->index, transport->name, transport->has_acl);
                    #ifdef DEBUG_INPROGRESS
                        TRACE(9, "Increment inprogress in revalidatePublish: connect=%u inprogress=%d\n", transport->index, pobj->inprogress+1);
                    #endif
                        if (__sync_fetch_and_add(&pobj->inprogress, 1) < 0) { /* BEAM suppression: constant condition */
                            __sync_fetch_and_sub(&pobj->inprogress, 1);
                            rc = ISMRC_Closed;
                            ism_common_setError(ISMRC_Closed);
                            return rc; //No need to process further. Assume the main process of _receive will decrement more and closing out the connection
                        }

                       char * cpa_xbuf = alloca(pbuflen+256);
                       concat_alloc_t cpa_buf = {cpa_xbuf,  pbuflen+16, 16};
                       ism_common_allocBufferCopyLen(&cpa_buf, pbuf, pbuflen);

                       rc = checkPublishAuth(transport, &cpa_buf, kind, rc, 1);
                       isCheckPublish=1;
                       shouldNotify=0;
                       TRACE(9, "revalidatePublish: MQTT receive MT_PUBLISH connect=%u client=%s gateway has resource group has_acl=%d rc=%d shouldNotify\n",
                              transport->index, transport->name, transport->has_acl, rc);

                       ism_common_freeAllocBuffer(&cpa_buf);

                    }
                }

                if(isCheckPublish){
                    if (rc != ISMRC_AsyncCompletion ){
                        //If not going async. Need to decrement the inprogress for GW Auto Register
                        TRACE(9, "revalidatePublish: checkPublishAuth is not ASYNC: connect=%d clientID=%s topic=%s rc=%d\n",
                                                             transport->index, transport->clientID, topic, rc);
            #ifdef DEBUG_INPROGRESS
                        TRACE(9, "Decrement inprogress in revalidatePublish: connect=%u inprogress=%d\n", transport->index, pobj->inprogress);
            #endif
                        if (__builtin_expect((__sync_sub_and_fetch(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
                            rc = ISMRC_Closed;
                            ism_common_setError(ISMRC_Closed);
                            return rc; //No need to process further. Assume the main process of _receive will decrement more and closing out the connection
                        }
                    }else{
                        //it is ASYNC
                        rc=0;
                        sendit=0;
                        TRACE(9, "revalidatePublish: checkPublishAuth is ASYNC: connect=%d clientID=%s topic=%s rc=%d\n",
                                                              transport->index, transport->clientID, topic, rc);
                    }
                }
            }

            //If not authorize, consider non protocol error.
            if (rc == ISMRC_NotAuthorized &&  !shouldNotify) {
                sendGWPINGREQ(transport);
                if (pobj->mqtt_version >= 5){
                     mqttMsg_t mmsg = {0};
                     mmsg.transport = transport;
                     mmsg.rc = rc;
                     mmsg.qos = (kind>>1)&3;
                     publishNAK(&mmsg);
                }else {
                    sendDirectReply(transport, kind, (char*)pbuf, pbuflen, 1);
                }
                sendit = 0;
                rc = 0;
                transport->endpoint->stats->count[transport->tid].lost_msg++;
                if (pobj->server_transport)
                    pobj->server_transport->lost_msg++;
            }
        }

        //check for other Errors of GW and notify appropriately
        if (rc) {
            sendit = 0;
            if (rc == ISMRC_AsyncCompletion) {
                rc = 0;
            } else {
                /* Do a notification but not for malformed and protocol errors */
                if (shouldNotify && rc != ISMRC_BadLength && rc != ISMRC_InvalidQoS && rc != ISMRC_MsgTooBig) {
                   /* Error during doPublish. Send Notification */
                   const char * errStr = getMQTTErrorString(rc, xbuf, sizeof xbuf);
                   topiclen = BIGINT16(pbuf);
                   char * temp_topic = (char *)((rc == ISMRC_UnicodeNotValid) ? FIXUNICODE((const char *)(pbuf+2), topiclen) :
                           FIXTOPIC((const char *)(pbuf+2), topiclen));
                   ism_proxy_GWNotify(transport, "Publish", temp_topic, NULL, NULL, rc, errStr);
                   sendDirectReply(transport, kind, (char*)pbuf, pbuflen, 1);
                   rc = 0;
                }
                transport->endpoint->stats->count[transport->tid].lost_msg++;
                if (pobj->server_transport)
                   pobj->server_transport->lost_msg++;
            }
        }

    }else {
        //Reply for Non GW
        if(rc){
            mqttMsg_t mmsg = {0};
            mmsg.transport = transport;
            mmsg.rc = rc;
            mmsg.qos = (kind>>1)&3;
            publishNAK(&mmsg);
        }
    }

    TRACE(9, "revalidatePublish: completed: connect=%d clientID=%s rc=%d sendit=%d\n",
                           transport->index, transport->clientID, rc, sendit);
    if(in_sendit!=NULL)
        *in_sendit=sendit;

    return rc;
}

/*
 * Implement an incoming (from client) subscribe
 * Note:
 * =MG expect to have Extension for SUBSCRIBE from proxy
 */
static int revalidateSubscribe(ism_transport_t * transport, char * bp, int buflen, concat_alloc_t * buf, int kind, int * outcount) {
    static const char * hexdigit = "0123456789abcdef";
    int count = 0;
    int goodcount = 0;
    int xrc;
    int rc = 0;
    const char * ttopic;
    char fixbuf[1024];
    int  topic_pos;
    int  doreply = 0;
    char * inbp = bp;
    int  mproplen;
    uint8_t subrc = 0;
    uint8_t * subrcp = NULL;
    int     subrc_count = 1;
    char * mpropbytes;
    int issubsX=0;


    //Determine if has extension which is SUBSCRIBEX
    if((kind&0xF)==0xF){
        issubsX=1;
    }

    *outcount = 0;
    if (buflen < 6) {
        ism_common_setErrorData(ISMRC_BadLength, "%s", "SUBSCRIBE");
        return ISMRC_BadLength;
    }

    /* Copy msgid */
    int packetid = BIGINT16(bp);
    if (packetid == 0) {
        ism_common_setErrorData(ISMRC_InvalidID, "%u%s", packetid, transport->name);
        return ISMRC_InvalidID;
    }
    memcpy(buf->buf + buf->used, bp, 2);
    buf->used += 2;
    bp += 2;
    buflen -= 2;

    /* Check for properties for version 5 and above */
    if (buflen > 0 && transport->pobj->mqtt_version >= 5) {
        if (*bp != 0) {
            int vilen;
            mproplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
            bp += vilen;
            buflen -= vilen;
            mpropbytes = (char *)bp;
            if (mproplen < 0 || mproplen > buflen) {
                ism_common_setErrorData(ISMRC_BadLength, "%s%s", "SUBSCRIBE", "PropLen");
                return ISMRC_BadLength;
            }
            //Note: no need to revalicate the properties because it is ready verified
            bp += mproplen;
            buflen -= mproplen;
            /* Copy properties */
            ism_common_putMqttVarInt(buf, mproplen);
            if (mproplen)
                ism_common_allocBufferCopyLen(buf, mpropbytes, mproplen);
        } else {
            bp++;
            buflen--;
            bputchar(buf, 0);    /* Copy property length of 0 */
        }
    }

    while (buflen > 3) {
        uint8_t qos;
        int len = (uint16_t)BIGINT16(bp);
        bp += 2;
        buflen -= 2;
        if (buflen < (len+1))
            break;
        /* Convert topic */
        topic_pos = buf->used;
        count++;
        //Coming from saveData. The topic is already converted.
        /* Convert the topic to the external format */
        char xbuf [2048];
        concat_alloc_t topic_out_buf = {xbuf, sizeof xbuf};

        /*
        * Ignore the shared prefix
        */
        char * btopic = alloca(len+1);
        int btopiclen = len;
        memcpy(btopic, bp, btopiclen);
        btopic[btopiclen]=0;
        char * bfulltopic=btopic;
        char * pos;


        TRACE (7, "Full Converted Topic: %s len:%d\n", btopic, btopiclen);
        if (*btopic == '$') {
           if (btopiclen > 20 && !memcmp(btopic, "$SharedSubscription/", 20)) {
               btopic += 20;
           } else if (btopiclen >= 7 && !memcmp(btopic, "$share/", 7)) {
               btopic += 7;
           } else {
               continue;
           }
           pos = strchr(btopic, '/');
           if (pos) {
               btopic = pos+1;
           } else {
               continue;
           }
           btopiclen -= (btopic-bfulltopic);
        }

        TRACE (7, "Converted Topic: %s len:%d\n", btopic, btopiclen);
        convertTopicOut(transport, &topic_out_buf, (char *)btopic, btopiclen);
        xrc = convertTopic(transport, buf, (char *)topic_out_buf.buf+2, topic_out_buf.used-2, 1);

        if (xrc == 0 && transport->has_acl) {
            xrc = checkACL(transport, buf->buf+topic_pos);
        }
        if (!xrc) {
            goodcount++;
        } else {
            ttopic = fixtopic((const char *)bp, len>1023?1023:len, fixbuf, xrc==ISMRC_UnicodeNotValid);
            TRACE(6, "Error in convert topic on subscribe: topic=%s connect=%u client=%s rc=%u\n", ttopic, transport->index, transport->name, xrc);
            if (transport->pobj->mqtt_version >= 5) {
                int mqttrc = mapToMqttRc(xrc, transport->pobj->mqtt_version, CPOI_SUBACK);
                if (count == 1) {
                    subrc = mqttrc;
                } else {
                    if (count >= subrc_count) {
                        subrc_count = subrc_count == 1 ? 256 : subrc_count*4;
                        subrcp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_mqtt_subscribe,1),subrcp, subrc_count);
                        memset(subrcp+count-1, 0, subrc_count-count+1);
                        subrcp[0] = subrc;
                    }
                    subrcp[count-1] = mqttrc;
                }
                buf->used = topic_pos;
                ism_common_allocBufferCopyLen(buf, "\0\5$_$xx", 7);
                buf->buf[buf->used-2] = hexdigit[(mqttrc>>4) & 0xf];
                buf->buf[buf->used-1] = hexdigit[mqttrc & 0xf];
                if (transport->client_class == 'g' &&
                    (xrc==ISMRC_BadTopic || xrc==ISMRC_BadSysTopic || xrc==ISMRC_NotAuthorized)) {
                    const char * errStr = getMQTTErrorString(xrc, xbuf, sizeof xbuf);
                    ism_proxy_GWNotify(transport, "Subscribe", ttopic, NULL, NULL, xrc, errStr);
                }
            } else {
                if (transport->client_class == 'g' &&
                    (xrc==ISMRC_BadTopic || xrc==ISMRC_BadSysTopic || xrc==ISMRC_NotAuthorized)) {
                    const char * errStr = getMQTTErrorString(xrc, xbuf, sizeof xbuf);
                    ism_proxy_GWNotify(transport, "Subscribe", ttopic, NULL, NULL, xrc, errStr);
                    doreply = 1;
                } else {
                    rc = xrc;
                }
            }
        }
        bp += len;
        buflen -= len;
        if(issubsX){
            //Get QoS from Extension
            int extlen = BIGINT16(bp);
            const char * ext  =(char *)bp+2;
            qos = ism_common_getExtensionValue(ext, extlen, EXIV_SubscribeOpt, 0);
        }else{
            qos = *bp&3;
        }

        if (!rc) {
            int usefilter = 0;
            int badmask = transport->pobj->mqtt_version >=5 ? 0xC0 : 0xFC;
            if ((*bp&badmask) || (qos > 2)) {
                ttopic = fixtopic((char *)(bp-len), len>1023?1023:len, fixbuf, 0);
                ism_common_setErrorData(ISMRC_BadClientData, "%s%s", "SUBSCRIBE", ttopic);
                rc = ISMRC_InvalidQoS;
                break;
            }

            /* Downgrade QoS to max QoS for this tenant */
            if (qos > transport->tenant->max_qos)
                qos = transport->tenant->max_qos;
            qos |= (*bp&0xFC);

            /*
             * Check gateway group restriction
             */
            if (transport->has_acl) {
                const char * devtype;
                const char * devid;
                int cpylen = BIGINT16(buf->buf+topic_pos);
                memcpy(fixbuf, buf->buf+topic_pos+2, cpylen);
                fixbuf[cpylen] = 0;
                int ptarc = parseTopicAuth(fixbuf, &devtype, &devid);
                /* If there is a wildcard in the devtype or devid we use the filter */
                if (ptarc == 2) {
                    usefilter = 1;
                }
            }

            /*
             * Put out the QoS byte or subscriptions option extension
             */
            if (!transport->has_acl) {
                bputchar(buf, qos);   /* Copy the qos */
            } else {
                int extlen;
                int savepos = buf->used;
                buf->used += 2;    /* Skip over len, add later */

                /*
                 * Put out the ACL filter.
                 * This is of the form:  JMS_Topic aclcheck('aclname',3,5) is not false
                 */
                if (usefilter) {
                    bputchar(buf, EXIV_ACLFilter);
                    buf->used += 2;    /* Skip over len, add later */
                    ism_json_putBytes(buf, "JMS_Topic aclcheck('");
                    ism_json_putBytes(buf, transport->name);
                    ism_json_putBytes(buf, "',3,5) is not false");
                    extlen = buf->used - savepos - 5;
                    buf->buf[savepos+3] = (char)(extlen<<8);
                    buf->buf[savepos+4] = (char)extlen;
                }

                /* Put out the QoS byte */
                ism_common_putExtensionValue(buf, EXIV_SubscribeOpt, qos);

                extlen = buf->used - savepos - 2;
                buf->buf[savepos] = (char)(extlen<<8);
                buf->buf[savepos+1] = (char)extlen;
            }
        }

        if(issubsX){
            //Skip over the Extension if this is revalidation. 2 Bytes
            int extlen = BIGINT16(bp);
            bp+=extlen+2; //Skip over the len + 2 bytes for the size
            buflen-=extlen+2;
        }else{
            buflen--;
            bp++;
        }
    }
    /* Check that we are at the end of the payload and have at least one topic */
    if (buflen != 0 || count == 0) {
        if (rc == 0) {
            ism_common_setErrorData(ISMRC_BadLength, "%s", "SUBSCRIBE");
            count = 0;
            rc = ISMRC_BadLength;
        }
        *outcount = count;
        return rc;
    }
    if (rc == 0 && (doreply || goodcount==0)) {
        if (transport->pobj->mqtt_version >= 5) {
            sendSubAck(transport, count, (char *)inbp, subrcp ? subrcp : &subrc);
        } else {
            sendDirectReply(transport, (MT_SUBSCRIBE<<4) + 2, (char*)inbp, buflen, count);
        }
        *outcount = 0;
    } else {
        *outcount = count;
    }



    if (subrcp)
        ism_common_free(ism_memory_proxy_mqtt_subscribe,subrcp);
    return rc;
}

/**
 * Revalidate the savedBuffer (each buffer from savedData) and send to MQ
 * Note: Make sure the transport is the client transport. Not the outgoing transport.
 */
static int revalidateAndSendSavedBuffer(ism_transport_t * transport, char * buffer, int pos, int avail, int * used, int * resultcode) {
   char * bp = buffer += pos;
   char * orig_bp = bp;
   int  kind;
   int  command;
   int  len;
   int  count = 2;
   int  buflen = avail-pos;
   int orig_len=0;
   int  multshift = 7;
   int rc = 0;
   int sendit=1;
   mqttProtoObj_t * pobj = transport->pobj;

   if (buflen < 2){
       return 2;
   }
   kind = bp[0];
   command = (uint8_t)((kind >> 4) & 15);
   len =  (uint8_t)bp[1];
   buflen -= 2;
   bp += 2;
   /* Handle a multi-byte length */
   if (len & 0x80) {
       len &= 0x7f;
       do {
           count++;
           if (count > 5) {
                //The MQTT variable length integer is at most 4 bytes in length
               TRACE(5, "revalidateAndSendSavedBuffer: frameMqtt: The MQTT length is too long: connect=%u from=%s:%u\n",
                       transport->index, transport->client_addr, transport->clientport);
               return -1;
           }
           if (buflen <= 0){
               //Since the buffer is from savedData. Assumed complete buffer.
               //Consider this invalid buffer if happend
               return -1;
           }
           len += (*bp++ & 0x7f) << multshift;
           multshift += 7;
           buflen--;
       } while ((bp[-1]&0x80));
   }

   orig_len=len+count; //Length of buffer + N bytes MQTTframe

   TRACE(9, "Revalidate the buffer: connect=%d clientID=%s len=%d orig_buf_len=%d buflen=%d count=%d\n",
                                  transport->index, transport->clientID, len, orig_len, buflen, count);


   if (len <= buflen) {
        if (command == MT_PUBLISH) {

            rc = revalidatePublish(transport, kind, bp, len, &sendit);

        } else if (command == MT_SUBSCRIBE) {

            int outcount=0;
            char xbuf[4096];
            concat_alloc_t buf = {xbuf, sizeof xbuf, 16};
            rc = revalidateSubscribe(transport, bp, len, &buf, kind, &outcount) ;
            if (transport->has_acl)
                kind = MT_SUBSCRIBEX;
            else
                kind = 0x82; //Note: 0x82 is Hex for the SUBSCRIBE fix header


            //Gateway Auto Registration
            //do checkSubscribeAuth if subscription count is 1
            if(outcount==1){
                TRACE(9, "revalidateSubscribe: complete and Continue: connect=%d clientID=%s inprogress=%d mqtt_version=%d rc=%d\n",
                                    transport->index, transport->clientID,  pobj->inprogress, pobj->mqtt_version, rc);

                if (transport->client_class == 'g' && !transport->has_acl && g_gatewayRegister && pobj->mqtt_version < 5) {
                    TRACE(9, "revalidateSubscribe: checkSubscribeAuth: connect=%d clientID=%s inprogress=%d\n",
                                     transport->index, transport->clientID, pobj->inprogress);
                    /* If we are in the process of closing, return closed */
                    #ifdef DEBUG_INPROGRESS
                    TRACE(9, "Increment inprogress in revalidateSubscribe: connect=%u inprogress=%d\n", transport->index, pobj->inprogress+1);
                    #endif
                    if (__builtin_expect((__sync_fetch_and_add(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
                         __sync_fetch_and_sub(&pobj->inprogress, 1);
                        rc = ISMRC_ServerNotAvailable;
                        ism_common_setError(ISMRC_ServerNotAvailable);
                        return rc;
                    }

                    //Note: For Subscribe, need the buffer with the msgID in it.
                    //checkSubscribeAuth will skip it.

                    rc = checkSubscribeAuth(transport, &buf, kind, outcount, 1);

                    if (rc) {
                      if (rc != ISMRC_AsyncCompletion) {
                          //If not going async. Need to decrement the inprogress for GW Auto Register
                          TRACE(5, "revalidateSubscribe: checkSubscribeAuth is not ASYNC: connect=%d clientID=%s  rc=%d\n",
                                               transport->index, transport->clientID,  rc);
                          #ifdef DEBUG_INPROGRESS
                                      TRACE(9, "Decrement inprogress in revalidateSubscribe: connect=%u inprogress=%d\n", transport->index, pobj->inprogress);
                          #endif
                          if (__builtin_expect((__sync_sub_and_fetch(&pobj->inprogress, 1) < 0), 0)) { /* BEAM suppression: constant condition */
                              rc = ISMRC_ServerNotAvailable;
                              ism_common_setError(ISMRC_ServerNotAvailable);
                              return rc;
                          }
                          sendDirectReply(transport, kind, (char*)buf.buf+18, buf.used-18, outcount);

                      }else{
                          TRACE(9, "revalidateSubscribe: checkSubscribeAuth is ASYNC: connect=%d clientID=%s rc=%d\n",
                                               transport->index, transport->clientID,  rc);
                      }
                      sendit = 0;
                      rc = 0;
                    }else{
                        TRACE(9, "revalidateSubscribe: checkSubscribeAuth is ASYNC: connect=%d clientID=%s  rc=%d\n",
                                  transport->index, transport->clientID,  rc);
                    }

                }
            }

            if(outcount > 0 && sendit && rc == ISMRC_OK){
                //Add frame
                int flen = ism_transport_addMqttFrame(transport, buf.buf+16, buf.used-16, kind|0x100);
                orig_bp = (buf.buf+16) - flen;
                orig_len = (buf.used - 16) + flen ;
            }else{
                //If subscription is zero. No need to send
                sendit=0;
            }
            TRACE(9, "revalidateSubscribe: connect=%d clientID=%s command=%s subcount=%d rc=%d\n",
                            transport->index, transport->clientID, ism_mqtt_mqttCommand(kind), outcount, rc );

         }else{
            //No need to valid other commands. Just send
            rc=0;
            sendit=1;
            TRACE(9, "Revalidate the buffer with: No need for validation: connect=%d clientID=%s command=%s  inprogress=%d\n",
                  transport->index, transport->clientID, ism_mqtt_mqttCommand(kind),  pobj->inprogress);
        }

        if(rc == ISMRC_OK && sendit==1){
            //Send to MG
            TRACE(9, "Revalidate the buffer: Sending Messages: connect=%d clientID=%s command=%s rc=%d\n",
                                                   transport->index, transport->clientID, ism_mqtt_mqttCommand(kind),  rc);
            int sendMQTT=1;
            if(command == MT_PUBLISH){
               TRACE(9, "Revalidate the buffer: Send to Kafka: connect=%d clientID=%s command=%s  rc=%d\n",
                              transport->index, transport->clientID, ism_mqtt_mqttCommand(kind),  rc);
               //For Kafka Publish, we don't need the first N bytes (count) for MQTTFrame
               ism_route_routeMessage(transport, orig_bp+count, orig_len-count, kind,  &sendMQTT);

            }
            if(sendMQTT){


               ism_transport_t * stransport = pobj->server_transport;
               if (LIKELY(stransport != NULL)) {
                   TRACE(9, "Revalidate the buffer: Send to MG: connect=%d clientID=%s command=%s  rc=%d transport->ready=%d\n",
                               stransport->index, stransport->clientID, ism_mqtt_mqttCommand(kind), rc, stransport->ready);
                   //Send buffer with MQTTFrame. orig_bp contained the MQTT Framed buffer.
                   stransport->send(stransport, orig_bp, orig_len, 0, SFLAG_HASFRAME);
                   if(command == MT_PUBLISH){
                       __sync_add_and_fetch(&mqttStats.mqttP2SMsgsSent, 1);
                       if (pobj->server_transport)
                           pobj->server_transport->write_msg++;
                   }

               }else{
                   TRACE(5, "Re-validate the buffer: Send to MG: Server Transport is NULL: connect=%d clientID=%s command=%s rc=%d\n",
                                          transport->index, transport->clientID, ism_mqtt_mqttCommand(kind), rc);
                   rc = ISMRC_ServerNotAvailable;
                   ism_common_setError(ISMRC_ServerNotAvailable);
                   return rc;
               }
            }

        }else if (rc != ISMRC_OK){
            TRACE(5, "Revalidate the buffer: Topic is NOT authorized: connect=%d clientID=%s  command=%s\n",
            transport->index, transport->clientID, ism_mqtt_mqttCommand(kind));

        }



   }else{
       //We shouldn't expect invalid buffer since msproxy construct it from saveData.
       return -1;
   }

   *resultcode= rc;
   *used = (len+count);
   TRACE(6, "Revalidate the buffer: connect=%d clientID=%s command=%s used=%d len=%d count=%d\n",
                transport->index, transport->clientID, ism_mqtt_mqttCommand(kind),  *used, len, count);
   return 0;
}
/**
 * Route savedData which saved during CONNECT and wait for CONNACK to be completed
 *
 * Note:
 * - Saved Data is already framed for MQTT
 * - All data is validated. So we will revalidate the folloing
 *   - Topic Permission
 *   - ACL
 *   - Gateway Auto Register
 */
static int revalidateAndSendSavedData(ism_transport_t * transport, char * saveddata, int saveddatalen, int savecount){
    int dataLen = saveddatalen;
    int offset = 0;
    int used=0;
    int rc = 0;
    int xrc = 0;
    int buffer_processed=0;
    while (dataLen > 0) {
        xrc = revalidateAndSendSavedBuffer(transport, saveddata, offset,saveddatalen, &used, &rc);
        if(xrc < 0){
            //Failed to parse buffer. Consider the whole savedData is invalid or incomplete.
            TRACE(5, "revalidateAndSendSavedData: Failed to parse the saved data buffer: connect=%u from=%s:%u\n",
                                  transport->index, transport->client_addr, transport->clientport);
            break;
        }else if(xrc == 2){
            //No buffer to parse.
            TRACE(7, "revalidateAndSendSavedData: no more buffer to parse: connect=%u from=%s:%u\n",
                                  transport->index, transport->client_addr, transport->clientport);
            break;
        }

        TRACE(9, "revalidateAndSendSavedData: processing: connect=%u from=%s:%u savedatalen=%d buffer_processed=%d offset=%d used=%d datalen=%d\n",
                              transport->index, transport->client_addr, transport->clientport, saveddatalen, buffer_processed, offset, used, dataLen);
        offset += used;
        dataLen -= used;
        used=0;
        buffer_processed++;

        //If one of the topic is not authorized. Return Error and Disconnect.
        if(rc!=ISMRC_OK){
           TRACE(9, "revalidateAndSendSavedData: topic from the buffer is not authorized. Exit savedata processing: connect=%u from=%s:%u\n",
                            transport->index, transport->client_addr, transport->clientport);
           break;
        }
    }
    TRACE(5, "revalidateAndSendSavedData: completed: connect=%u from=%s:%u savedatalen=%d savedatacount=%d buffer_processed=%d rc=%d\n",
                       transport->index, transport->client_addr, transport->clientport, saveddatalen, savecount, buffer_processed, rc);

    //Calculate if any messages are lost due to error and return
    if(rc!=ISMRC_OK){
        int lostmsgs = savecount - buffer_processed;
        transport->endpoint->stats->count[transport->tid].lost_msg += lostmsgs;
        if (transport->pobj !=NULL && transport->pobj->server_transport)
            transport->pobj->server_transport->lost_msg += lostmsgs;

    }

    return rc;
}

/*
 * Remove the will message from the connectdata in the transport object
 */
static void removeWill(ism_transport_t * transport) {
    char * bp;
    char * data;
    int len;
    int pktlen;
    int buflen;
    int vilen;
    int fixedlen = 1;
    int proxyprotocol = 0;
    int willlen;
    char * willpos;

    data = bp = transport->pobj->connectdata;
    buflen = transport->pobj->connectlen;
    if (bp) {
        bp++; /* Skip over command */
        pktlen = ism_common_getMqttVarIntExp((const char *) bp, buflen, &vilen);
        bp += vilen; /* Skip over packet len */
        fixedlen = vilen + 1;
        len = BIGINT16(bp); /* Protocol name length */
        bp += (len + 2); /* Skip over protocol name */
        if (bp[-1] == 'x') /* Check for proxy protocol */
            proxyprotocol = 1;
        bp++; /* skip version */
        if (!(*bp & 4))
            return; /* There is no will to remove */
        *bp &= 0xC2; /* Turn off will bits */
        bp += (3 + proxyprotocol); /* skip flags and keepalive) */
        if (transport->pobj->mqtt_version >= 5) { /* skip over connect properties */
            bp += ism_common_getMqttVarIntExp((const char *) bp, buflen,
                    &vilen);
            bp += vilen;
        }
        len = BIGINT16(bp); /* Skip over clientID */
        bp += (len + 2);
        willpos = bp;
        if (transport->pobj->mqtt_version >= 5) { /* skip over will properties */
            bp += ism_common_getMqttVarIntExp((const char *) bp, buflen,
                    &vilen);
            bp += vilen;
        }
        len = BIGINT16(bp); /* Skip over will topic */
        bp += (len + 2);
        len = BIGINT16(bp); /* Skip over will bytes */
        bp += (len + 2);
        willlen = bp - willpos;

        /* Rewrite the connectdata in place */
        pktlen -= willlen;
        concat_alloc_t rbuf = { (char *) data, 5, 1 };
        ism_common_putMqttVarInt(&rbuf, pktlen);
        /* If we changed the size of the packet len, move the variable header */
        if (rbuf.used != fixedlen) {
            memmove(data + rbuf.used, data + fixedlen,
                    (willpos - data) - fixedlen);
        }
        /* Move everything after the will message */
        memmove(willpos - (fixedlen - rbuf.used), bp, buflen - (bp - data));
        /* Recalculate the length to send */
        transport->pobj->connectlen -= (willlen + (fixedlen - rbuf.used));
    }
}

/*
 * The actual frame support is in frame.c in transport
 */
XAPI int ism_transport_addMqttFrame(ism_transport_t * transport, char * buf,int len, int command);
XAPI int ism_transport_addWSFrame(ism_transport_t * transport, char * buffer, int len, int kind);


/*
 * For a WebSockets Binary MQTT we need to add both the MQTT frame and the WebSockets frame
 */
int ism_mqtt_addwsbframe(ism_transport_t * transport, char * buf, int len, int command) {
    int outlen = ism_transport_addMqttFrame(transport, buf, len, command);
    if (outlen >= 0) {
        int wslen = ism_transport_addWSFrame(transport, buf - outlen,
                len + outlen, 2);
        if (wslen >= 0) {
            outlen += wslen;
        } else {
            outlen = -1;
        }
    }
    return outlen;
}


/*
 * Format the MQTT protocol specific object in the transport object
 */
static int mqttDumpPobj(ism_transport_t * transport, char * buf, int len) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t*) transport->pobj;
    int n;
    if (!buf || len<8)
        return pobj ? pobj->inprogress : 0;
    if (!pobj) {
        sprintf(buf, "null");
        return 0;
    }
    n = snprintf(buf, len - 1, "MQTT pobj: closed=%d durable=%d inprogress=%d\n",
                 pobj->closed, (int) !pobj->cleansession, pobj->inprogress);
    if (n >= len) {
        buf[len - 1] = '\0';
        return 0;
    }
    return 0;
}



/*
 * Set session expire interval global default
 */
int ism_proxy_setSessionExpire(int time) {
    g_maxSessionExpire = time;   /* Time in seconds */
    TRACEL(3, ism_defaultTrace, "MaxSessionExpiryInterval = %u (%g days)\n", g_maxSessionExpire,
            (double)g_maxSessionExpire / (3600*24));
    return 0;
}


/*
 * Set authorization  cache cleanup time
 */
int ism_proxy_setGWCleanupTime(int time) {
    g_gwCleanupTime = time * 1000L;
    TRACEL(3, ism_defaultTrace, "gwCleanupTime = %llu\n", (ULL)g_gwCleanupTime);
    return 0;
}


/*
 * Set authorization cache maximum active devices
 */
int ism_proxy_setGWMaxActiveDevices(int maxDevices) {
    gwMaxActiveDevices = maxDevices;
    TRACEL(3, ism_defaultTrace, "gwMaxActiveDevices = %d\n", gwMaxActiveDevices);
    return 0;
}


/*
 * Get MQTT statistics
 */
int ism_proxy_getMQTTStats(px_mqtt_stats_t * stats) {
    memcpy(stats, &mqttStats, sizeof(px_mqtt_stats_t));
    return 0;
}

/*
 * Get MQTT Messages with Size statistics
 */
int ism_proxy_getMQTTMsgSizeStats(px_msgsize_stats_t* stats) {
    memcpy(stats, &mqttMsgSizeStats, sizeof(px_msgsize_stats_t));
    return 0;
}

/*
 * Get the mqtt server transport
 */
ism_transport_t * ism_mqtt_getServerTransport(ism_transport_t * transport) {
    if (!strcmp(transport->protocol_family, "mqtt"))
        return transport->pobj->server_transport;
    return NULL;
}


/*
 * Process a new MQTT connection.
 *
 * MQTT is accepted with multiple encodings and framing.
 */
int ism_mqtt_connection(ism_transport_t * transport) {
    int rc = 1;
    mqttProtoObj_t * pobj = NULL;

    /*
     * Binary MQTT over WebSockets
     */
    if (!strcmpi(transport->protocol, "mqttv3.1") ||
        !strcmpi(transport->protocol, "mqtt") || !strcmpi(transport->protocol, "mqtt4")) {
        pobj = (mqttProtoObj_t *) ism_transport_allocBytes(transport, sizeof(mqttProtoObj_t), 1);
        memset(pobj, 0, sizeof(mqttProtoObj_t));
        transport->dumpPobj = mqttDumpPobj;
        transport->pobj = pobj;
        transport->receive = ism_mqtt_wsbReceive;
        pobj->prot = PROT_MQTT_WSBIN;
        transport->addframep = ism_mqtt_addwsbframe;
        transport->actionname = ism_mqtt_mqttCommand;
        if (!strcmpi(transport->protocol, "mqtt") || !strcmpi(transport->protocol, "mqtt4"))
            transport->protocol = "mqtt";
        else
            transport->protocol = "mqttv3.1"; /* Make the string constant */
        rc = 0;
    }

    /*
     * Binary MQTT over tcp
     */
    else if (!strcmp(transport->protocol, "mqtt-tcp") || !strcmp(transport->protocol, "mqtt4-tcp") ||
             !strcmp(transport->protocol, "mqtt5-tcp")) {
        pobj = (mqttProtoObj_t *) ism_transport_allocBytes(transport, sizeof(mqttProtoObj_t), 1);
        memset((char *) pobj, 0, sizeof(mqttProtoObj_t));
        transport->pobj = pobj;
        transport->dumpPobj = mqttDumpPobj;
        transport->receive = ism_mqtt_receive;
        pobj->prot = PROT_MQTT_BIN;
        transport->actionname = ism_mqtt_mqttCommand;
        transport->protocol = "mqtt-tcp"; /* Make the string constant */
        rc = 0;
    }

    /*
     * If we have a valid protocol, do common initialization
     */
    if (rc == 0) {
        if (!transport->originated) {
            int mqttConnections = __sync_add_and_fetch(&mqttStats.mqttConnections, 1);
            uint64_t mqttConnectionsTotal = __sync_add_and_fetch(&mqttStats.mqttConnectionsTotal, 1);
            TRACE(9, "ism_mqtt_connection: New mqtt connect=%u mqttConnections=%d mqttConnectionsTotal=%llu\n",
                    transport->index, mqttConnections, (ULL)mqttConnectionsTotal);
        }
        transport->protocol_family = "mqtt"; /* Constant string */
        transport->auth_permissions = 0xffffff;
        transport->closing = ism_mqtt_closing;
        transport->ack = ism_mqtt_ack;
        pthread_spin_init(&pobj->lock, 0);
    }
    return rc;
}
#endif

/*
 * Allow MQTTv5 to be enabled and disabled dynamically.
 * Disabling MQTTv5 does not stop any existing v5 connections, it only stops
 * new MQTTv5 connections.
 */
void ism_mqtt_setMQTTv5(int val) {
    g_allowMQTTv5 = val;
}

/*
 * Get the MQTT context from outside the MQTT environment
 */
mqtt_prop_ctx_t * ism_proxy_getMqttContext(int version) {
    if (version == 5) {
        if (!g_ctx5) {
             g_ctx5 = ism_common_makeMqttPropCtx(mqtt_propFields, 5);
        }
        return g_ctx5;
    }
    return NULL;
}


/*
 * Get the property context.
 */
mqtt_prop_ctx_t * ism_mqtt_getPropContext(ism_transport_t * transport) {
    return ism_proxy_getMqttContext(transport->pobj->mqtt_version);
}

/*
 * Add a property, zero terminating strings as required
 */
void addProp(ism_prop_t * props, const char * name, int namelen, const char * val, int vallen) {
    ism_field_t f;
    if (namelen >= 0) {
        char * tempname = alloca(namelen+1);
        memcpy(tempname, name, namelen);
        tempname[namelen] = 0;
        name = tempname;
    }
    if (vallen >= 0) {
        char * tempval = alloca(vallen+1);
        memcpy(tempval, val, vallen);
        tempval[vallen] = 0;
        val = tempval;
    }
    f.type = VT_String;
    f.val.s = (char *)val;
    ism_common_setProperty(props, name, &f);
}


/*
 * Set message properties into ism_prop_t
 */
int ism_mqtt_mpropSet(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value) {
    ism_prop_t * mqttprops = (ism_prop_t *) userdata;
    int namelen;
    int vallen;

    switch (fld->id) {
    case MPI_ContentType:
        addProp(mqttprops, "_ContentType", -1, ptr, len);
        break;
    case MPI_ReplyTopic:
        addProp(mqttprops, "_ReplyTo", -1, ptr, len);
        break;
    case MPI_Correlation:
        if (ism_common_validUTF8Restrict(ptr, len, UR_NoNull) >= 0)
            addProp(mqttprops, "_Correlation", -1, ptr, len);
        break;
    case MPI_PayloadFormat:
        addProp(mqttprops, "_Format", -1, (value ? "text" : "binary"), -1);
        break;
    case MPI_UserProperty:
        namelen = BIGINT16(ptr);
        vallen = BIGINT16(ptr+2+namelen);
        addProp(mqttprops, ptr+2, namelen, ptr+4+namelen, vallen);
        break;
    }
    return 0;
}


/*
 * Generate properties from the message
 */
int ism_mqtt_propgen(ism_prop_t * xprops, ism_emsg_t * emsg, const char * name, ism_field_t * f, void * extra, ismMessageSelectionLockStrategy_t * lockStrategy) {
    if (emsg->otherprop_len > 0) {
        ism_common_checkMqttPropFields((char *)emsg->otherprops, emsg->otherprop_len, g_ctx5, 0xFFFF, ism_mqtt_mpropSet, xprops);
        emsg->otherprop_len = 0;
    }
    return ism_common_getProperty(xprops, name, f);
}


/*
 * Initialize the MQTT protocol
 */
int ism_protocol_initMQTT(void) {
#ifndef NO_PROXY
    const char * cidmatch;
    const char * devidmatch;
    const char * devtypematch;

    ism_transport_registerProtocol(ism_mqtt_connection);
    g_useMQTTpx = ism_common_getIntConfig("MqttProxyProtocol", 6);
    g_noProxyLog = ism_common_getIntConfig("MqttNoProxyLog", 0);

    int maxSessionExpire = ism_common_getIntConfig("MaxSessionExpiryInterval", 0);
    if (maxSessionExpire > 0)
        ism_proxy_setSessionExpire(maxSessionExpire);

    g_allowMQTTv5 = ism_common_getIntConfig("MQTTv5", 1);       /* Default true */
    /* Make the MQTTv5 context regardless of whether v5 is enabled */
    g_ctx5 = ism_common_makeMqttPropCtx(mqtt_propFields, 5);

    g_useMux = ism_common_getIntConfig("MqttUseMux", 1);
    ism_transport_muxInit(g_useMux);
    TRACEL(3, ism_defaultTrace, "MqttUseMux=%u\n", g_useMux);

    cidmatch = ism_common_getStringConfig("ClientIDMatch");
    if (cidmatch) {
        int rc = ism_regex_compile(&clientIDMatch, cidmatch);
        if (rc) {
            char buf[256];
            ism_regex_getError(rc, buf, sizeof(buf), clientIDMatch);
            TRACEL(3, ism_defaultTrace, "The ClientIDMatch property is not valid: rc=%d error=%s\n", rc, buf);
        } else {
            doClientIDMatch = 1;
            TRACEL(3, ism_defaultTrace, "Set the ClientIDMatch to '%s'\n", cidmatch);
        }
    }
    //Rule: ^[A-Za-z0-9\\_\\.-]{1,36}$
     devidmatch = ism_common_getStringConfig("DevIDMatch");
   	if (devidmatch) {
   		int rc = ism_regex_compile(&devIDMatch, devidmatch);
   		if (rc) {
   			char buf[256];
   			ism_regex_getError(rc, buf, sizeof(buf), devIDMatch);
   			TRACEL(3, ism_defaultTrace, "The devIDMatch property is not valid: rc=%d error=%s\n", rc, buf);
   		} else {
   			doDevIDMatch = 1;
   			TRACEL(3, ism_defaultTrace, "Set the DevIDMatch to '%s'\n", devidmatch);
   		}
   	}
   	devtypematch = ism_common_getStringConfig("DevTypeMatch");
   	if (devtypematch) {
   		int rc = ism_regex_compile(&devTypeMatch, devtypematch);
   		if (rc) {
   			char buf[256];
   			ism_regex_getError(rc, buf, sizeof(buf), devTypeMatch);
   			TRACEL(3, ism_defaultTrace, "The devIDMatch property is not valid: rc=%d error=%s\n", rc, buf);
   		} else {
   			doDevTypeMatch = 1;
   			TRACEL(3, ism_defaultTrace, "Set the DevIDMatch to '%s'\n", devtypematch);
   		}
   	}

    g_gatewayRegister = ism_common_getIntConfig("GatewayRegister", 0);
    TRACEL(3, ism_defaultTrace, "GatewayRegistger = %u\n", g_gatewayRegister);
    g_gwCleanupTime = ism_common_getIntConfig("gwCleanupTime", 600) * 1000L;
    TRACEL(3, ism_defaultTrace, "gwCleanupTime = %llu\n", (ULL)g_gwCleanupTime);
    gwMaxActiveDevices = ism_common_getIntConfig("gwMaxActiveDevices", 500);
    TRACEL(3, ism_defaultTrace, "gwMaxActiveDevices = %d\n", gwMaxActiveDevices);
    gwMaxPendingAuthRequests = ism_common_getIntConfig("gwMaxPendingAuthRequests", 10);
    TRACEL(3, ism_defaultTrace, "gwMaxPendingAuthRequests = %d\n", gwMaxPendingAuthRequests);

    g_AAAEnabled = ism_common_getIntConfig("AAAEnabled", 1);
    TRACEL(3, ism_defaultTrace, "AAAEnabled = %u\n", g_AAAEnabled);
    g_ExpectedDevMsgRate = ism_common_getIntConfig("ExpectedDevMsgRate", EXPECTEDMSGRATE_LOW);
    g_ExpectedAppMsgRate = ism_common_getIntConfig("ExpectedAppMsgRate", EXPECTEDMSGRATE_DEFAULT);
    g_ExpectedShAppMsgRate = ism_common_getIntConfig("ExpectedShAppMsgRate", EXPECTEDMSGRATE_HIGH);
    g_ExpectedGWMsgRate = ism_common_getIntConfig("ExpectedGWMsgRate", EXPECTEDMSGRATE_HIGH);
    TRACEL(3, ism_defaultTrace, "ExpectedDevMsgRate=%d ExpectedAppMsgRate=%d ExpectedShAppMsgRate=%d ExpectedGWMsgRate=%d\n",
            g_ExpectedDevMsgRate, g_ExpectedAppMsgRate, g_ExpectedShAppMsgRate, g_ExpectedGWMsgRate);

    /*Initialize the Discard msgs properties*/
    g_discardMsgsEnabled = ism_common_getIntConfig("DiscardMsgsEnabled", g_discardMsgsEnabled);
    g_discardMsgsSoftLimit = ism_common_getIntConfig("DiscardMsgsSoftLimit", g_discardMsgsSoftLimit);
    g_discardMsgsHardLimit = ism_common_getIntConfig("DiscardMsgsHardLimit", g_discardMsgsHardLimit);
    g_discardMsgsLogIntervalInSec = ism_common_getIntConfig("DiscardMsgsLogInterval", g_discardMsgsLogIntervalInSec);
    TRACEL(3, ism_defaultTrace, "DiscardMsgs Properties: Enabled=%d LogInterval=%d SoftLimit=%d HardLimit=%d\n",
    						g_discardMsgsEnabled,g_discardMsgsLogIntervalInSec, g_discardMsgsSoftLimit, g_discardMsgsHardLimit );

    g_sendGWPingREQ = ism_common_getIntConfig("SendGWPingREQ", 1);

    /*
     * Initialize topic alias maximums for client to proxy communications.
     * Topic alias is not used on the proxy to server link or server to proxy link.
     * The MqttTopicAliasOut is sent to the client as the most we support.
     * The MqttTopicAliasIn is used with a min of this value and the one supplied by the client
     */
    g_MaxTopicAliasIn  = ism_common_getIntConfig("MqttTopicAliasIn", g_MaxTopicAliasIn);   /* client to proxy */
    if (g_MaxTopicAliasIn > 255)
        g_MaxTopicAliasIn = 255;
    g_MaxTopicAliasOut = ism_common_getIntConfig("MqttTopicAliasOut", g_MaxTopicAliasOut); /* proxy to client */
    if (g_MaxTopicAliasOut > 255)
        g_MaxTopicAliasOut = 255;
    TRACEL(3, ism_defaultTrace, "MqttTopicAliasIn=%d MqttTopicAliasOut=%d\n", g_MaxTopicAliasIn, g_MaxTopicAliasOut);

    g_verifyPayload = ism_common_getBooleanConfig("MqttVerifyPayload", g_verifyPayload);
    g_verifyReasonCode = ism_common_getBooleanConfig("MqttVerifyReasonCode", g_verifyReasonCode);

    /*
     * Configure the max keep alive
     */
    g_maxKeepAlive  = ism_common_getIntConfig("MqttMaxKeepAlive", g_maxKeepAlive);
    if (g_maxKeepAlive > 65535)
        g_maxKeepAlive = 0;
    g_maxKeepAliveA = ism_common_getIntConfig("MqttMaxKeepAliveA", g_maxKeepAliveA);
    if (g_maxKeepAliveA > 65535)
        g_maxKeepAliveA = 0;
    if ((g_maxKeepAliveA == 0 && g_maxKeepAlive > 0) || (g_maxKeepAliveA > g_maxKeepAlive))
        g_maxKeepAliveA = g_maxKeepAlive;

    g_canRegister = ism_proxy_getAuthMask("devices_create");
    if (g_canRegister < 0)
        g_canRegister = 0;
    TRACEL(3, ism_defaultTrace, "canRegister=%x", g_canRegister);
#endif
    return 0;
}

#undef TRACE_DOMAIN
#define TRACE_DOMAIN ism_defaultTrace
