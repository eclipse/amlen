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

#ifndef MQTT_H_DEFINED
#define MQTT_H_DEFINED

#include <transport.h>
#include <protocol.h>
#include <engine.h>
#include <ismjson.h>
#include <stdbool.h>

#ifdef PROTOCOL_EXTRA_SRC
#include <protocol_extra.h>
#endif

/*
 * MQTTv3 commands
 */
#define MT_ZERO        0
#define MT_CONNECT     1
#define MT_CONNACK     2
#define MT_PUBLISH     3
#define MT_PUBACK      4
#define MT_PUBREC      5
#define MT_PUBREL      6
#define MT_PUBCOMP     7
#define MT_SUBSCRIBE   8
#define MT_SUBACK      9
#define MT_UNSUBSCRIBE 10
#define MT_UNSUBACK    11
#define MT_PINGREQ     12
#define MT_PINGRESP    13
#define MT_DISCONNECT  14
#define MT_AUTH        15
/* Extra proxy protocol commands */
#define MTX_PUBLISHX   17
#define MTX_SENDACL    18
#define MTX_SUBSCRIBEX 19
#define MTX_GETX       20
#define MTX_SENDSUBS   21

#define MT_CMD_PUBLISHX 0x3F
#define MT_PUBLISHX     0x3F
#define MT_GETX         0x5F
#define MT_SUBSCRIBEX   0x8F
#define MT_SENDACL      0xCF
#define MT_GW_PINGREG   0xCE
#define MT_SENDSUBS     0xDF
#define MT_GW_PINGRES   0xDE

/*
 * Enumerated values for return codes.
 * The first few of these are defined return code values on the CONACK
 * command.  The others are internal return code.  If an error is found
 * anywhere except on a connect (and then only some of the cases) the
 * action is to disconnect.
 */
enum conack_e {
    /* Return codes from the MQTT spec */
    CRC_MinConnectRc   = 0x0000,
    CRC_Accepted       = 0x0000,
    CRC_InvalidVersion = 0x0001,
    CRC_BadIdentifier  = 0x0002,
    CRC_NotAvailable   = 0x0003,
    CRC_BadUser        = 0x0004,
    CRC_NotAuthorized  = 0x0005,
    CRC_MaxConnectRc   = 0x0006
};


#ifdef MQTT_RC_STRINGS
/*
 * String expansions for the return codes
 */
static const char * getMQTTErrorString(int rc, char * buf, int len ) {
    const char * ret;
    switch (rc) {
    case CRC_Accepted :         return NULL;
    case CRC_InvalidVersion :   ret = "The MQTT client version is not supported.";    break;
    case CRC_BadIdentifier :    ret = "The client ID is not valid.";                  break;
    case CRC_NotAvailable :     ret = "The server is not available.";                 break;
    case CRC_BadUser :          ret = "The user name or password is not valid.";      break;
    case CRC_NotAuthorized :    ret = "The connection is not authorized.";            break;
    default:                    ret = ism_common_getErrorString(rc, buf, len);        break;
    }
    if (buf != ret)
        ism_common_strlcpy(buf, ret, len);
    return buf;
}


#endif

/*
 * Connection flags for the CONNECT command
 */
#define CFLAG_UserName   0x80
#define CFLAG_Password   0x40
#define CFLAG_WillRetain 0x20
#define CFLAG_WillQoS    0x18
#define CFLAG_Will       0x04
#define CFLAG_Clean      0x02

/* Proxy protocol flags */
#define XCFLAG_Domain       0x80
#define XCFLAG_MaxConn      0x40
#define XCFLAG_NoUserCheck  0x20
#define XCFLAG_ExtraFlags   0x10   /* was NotifyTopic */
#define XCFLAG_ClientAddr   0x08
#define XCFLAG_CertName     0x04
#define XCFLAG_Port         0x02
#define XCFLAG_NoLog        0x01


/*
 * Load a big endian 2 byte integer
 */
#define BIGINT16(p) (((int)(uint8_t)(p)[0]<<8) | (uint8_t)(p)[1])

/*
 * Consumer object for the MQTT protocol
 */
typedef struct mqtt_prodcons_t {
    ism_transport_t * transport;
    ismEngine_ConsumerHandle_t handle;
    const char * topic;
    uint32_t which;
    uint32_t subID;
    uint8_t closing;
    uint8_t closed;
    uint8_t qos;
    uint8_t flags;
    uint8_t publishX;
    uint8_t resv[3];
} mqtt_prodcons_t;

/* Subscription options */
#define MSUBOPT_QoS             0x03    /* QoS for subscription */
#define MSUBOPT_NoLocal         0x04    /* NoLocal (MQTTv5) */
#define MSUBOPT_AsPublished     0x08    /* Retain as published (MQTTv5) */
#define MSUBOPT_RetainHandling  0x30    /* Retain handling (MQTTv5) */
#define MSUBOPT_Reservedv3      0xFC    /* Reserved in V3.1 and V3.1.1 */
#define MSUBOPT_Reservedv5      0xC0    /* Reserved in V5 */

#define MSUBOPT_SendRetained    0       /* RetainHandling 0 = Send retained on subscribe */
#define MSUBOPT_SendRetainedNew 1       /* RetainHandling 1 = Send retained on new subscription */
#define MSUBOPT_SendNoRetained  2       /* RetainHandling 2 = Do not send retained on subscibe */
#define MSUBOPT_InvalidRetained 3       /* Reserved encoding */

#define CONFLAG_UseFilter       1
#define CONFLAG_RerunFilter     2

/*
 * Publisher object for the MQTT protocol
 */
typedef struct mqtt_pubobj_t {
    ismEngine_ProducerHandle_t producerh; /* Producer handle used for more efficient
                                           * publishing when many consecutive
                                           * messages are sent to the same topic.
                                           */
    char *lasttopic;    /* Last topic destination for this publisher */
    int lasttopic_len;  /* Number of bytes in topic name */
    int lasttopic_alloc;/* Number of bytes allocated to hold lasttopic - not to exceed 1024. */
    int count;          /* The number of consecutive messages sent to lastttopic */
    int rsrv;     		/* Set to 0 unless/until producer handle is ready for publishing */
} mqtt_pubobj_t;

/*
 * Parse the various mqtt message/action types.
 * Note that the pointers are into the original message buffer, and therefore
 * that cannot be freed until message processing is done.
 */
typedef struct mqttMsg_t {
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
    int32_t         keepalive;      /* Keepalive in seconds   */
    int             payload_len;
    int             topic_len;
    int             clientid_len;
    const char *    payload;        /* The message body       */
    const char *    topic;
    const char *    clientid;       /* The clientId - only on connect */
    int             username_len;
    int             password_len;
    const char *    username;       /* The username - only on connect */
    const char *    password;       /* The password - only on connect */
    uint32_t        msgExpire;
    int             willtopic_len;
    int             willpayload_len;
    int             willproplen;
    const char *    willtopic;      /* The will topic - only on connect */
    const char *    willpayload;    /* The will payload - only on connect */
    const char *    willprops;      /* The will properties - only on connect */
    ism_json_parse_t * jsonobj;
    int             prop_entnum;
    int             domain_len;
    const char *    domain;         /* Domain name */
    const char *    ext;            /* Proxy protocol extension */
    uint32_t        expireTTL;
    int             max_connect;    /* Max connect count */
    const char *    v5props;        /* MQTT properties */
    uint32_t        v5prop_len;     /* MQTT properties length */
    uint8_t         isMsgExpire;    /* Message expire set */
    uint8_t         isExpire;       /* Session expire set */
    uint8_t         resvx[2];
    char            locale [8];
    uint16_t        ext_len;        /* Proxy protocol extension length */
    uint8_t         more_flags;     /* Additional proxy flags */
    uint8_t         isMsgid;
    uint8_t         msgfmt;
    uint8_t         v5_shared;
    uint8_t         msgRate;        /* Expected message rate (0-9) */
    uint8_t         resvx2[1];
    uint16_t        maxActive;
    uint16_t        topic_alias;
    uint32_t        will_delay;
    uint8_t         request_reply_info;
    uint8_t         debug_lvl;
    uint8_t         mqtt_rc;
    uint8_t         has_props;
    uint32_t        subID;
    uint32_t        maxPacketSize;
    const char *    reason;
    int             reason_len;
    uint32_t        resvi;
} mqttMsg_t;

typedef struct mqttSavedData_t {
    struct mqttSavedData_t *    next;
    int                         kind;
    int                         buflen;
    char *                      buf;
} mqttSavedData_t ;

/*
 * The MQTT protocol specific area of the transport object
 */
typedef struct ism_protobj_t {
    void *             handle;            /* Client handle            */
    void *             session_handle;    /* Session handle           */
    uint64_t           client_uid;
    volatile int       closed;            /* Connection is not is use */
    int                prot;              /* Which protocol           */
    mqtt_prodcons_t * * prodcons;         /* List of consumers        */
    int                prodcons_alloc;    /* Number of allocated consumers */
    int                consumer_count;    /* Number of consumers which exist */
    int                suspended;         /* Message delivery for this session is suspended */
    uint32_t           savedSize;
    mqttSavedData_t *  savedDataHead;
    mqttSavedData_t *  savedDataTail;
    uint8_t            clean;             /* Disconnect is clean      */
    uint8_t            cleansession;      /* Clean session indicator (0 - durable client, 1 - non-durable) */
    volatile uint8_t   startState;        /* Start state 0=not yet, 1=in progress, 2=stolen, 3=done */
    uint8_t            mqtt_version;      /* 3=3.1, 4=3.1.1           */
    uint8_t            mqtt_bridge;       /* Use bridge protocol      */
    uint8_t            mqtt_client;       /* This connection is a client */
    uint8_t            mqtt_proxy;        /* Proxy protocol */
    uint8_t            allow_persistent;  /* Allow persistent messages to be published */
    void *             authHandle;        /* Authentication handle */
    struct ism_msgid_list_t * msgids;
    struct ism_msgid_list_t * incompmsgids;	/* List of incomplete QoS 2 publications*/
    pthread_spinlock_t lock;
    pthread_spinlock_t msgids_spinlock;
    pthread_mutex_t    msgids_mutex;
    int64_t            keepAlive;         /* Keep Alive time */
    uint64_t           messageCount;      /* Current message count */
    volatile  uint64_t lastAccessTime;
    pthread_spinlock_t sessionlock;
    int32_t            inprogress;        /* Count of actions in progress */
    ismHashMap       * errors;            /* Errors that were reported already */
    struct ism_protobj_t *  prev;
    struct ism_protobj_t *	next;
    ism_transport_t *		transport;
    int32_t            morelen;           /* WebSockets extra buffer length */
    int32_t            morealloc;         /* WebSockets extra buffer allocated size */
    char *             morebuf;           /* WebSockets extra buffer pointer */
    struct ima_tenantInfo_t * tenantInfo;
    ism_timer_t        pingTimer;
    mqtt_pubobj_t      publisher[1];
    char               locale[8];
    uint8_t            publishNAK;
    uint8_t            qos0NAK;
    uint8_t            subscribeNAK;
    uint8_t            serverInfo;
    uint8_t            aclwait;
    uint8_t            stolen;
    uint8_t            session_existed;
    uint8_t            v5_shared;
    uint8_t            msgRate;
    uint8_t            send_disconnect;
    uint8_t            send_keepalive;
    uint8_t            getx_active;
    uint32_t           will_delay;
    uint16_t           maxActive;
    uint8_t            generated_cid;
    uint8_t            requestReason;
    uint8_t            requestReplyInfo;
    uint8_t            expire_set;
    uint16_t           monitor_opt;
    char *             monitor_topic;
    uint32_t           expireTTL;      /* Session expire time in seconds */
    uint32_t           maxExpire;
    uint32_t           modifiedExpire;
    uint32_t           timeout;
    uint32_t           maxPacketSize;
    uint32_t           flow_count;     /* Current outstanding QoS>0 messages*/
    uint32_t           flow_max;       /* Receive Maximum set to client */
    uint16_t           topicalias_count_in;
    uint16_t           topicalias_count_out;
    const char * *     topicalias_in;
    const char * *     topicalias_out;
    uint8_t            sendSubs;
    uint8_t            resvi2[7];
} ism_protobj_t;

typedef ism_protobj_t mqttProtoObj_t;
/*
 * Message ID states
 */
#define ISM_MQTT_PUBLISH    1000
#define ISM_MQTT_PUBREL     1001
#define ISM_MQTT_PUBREC     1002

/*
 * Protocol states
 */
#define MQTT_NEW            0
#define MQTT_IN_PROGRESS    1
#define MQTT_STOLEN         2
#define MQTT_CONNECTED      3
#define MQTT_DISCONNECTED   4

/* IoT Monitoring */
#define JM_Connect      0
#define JM_Disconnect   1
#define JM_Failed       2
#define JM_Active       3
#define JM_Info         4
#define JM_Clear        5

/*
 * Max number of subscriptions allowed in a single connection.
 * This can be overridden with the MqttMaxSubscriptions static config proeprty.
 */
#define MQTT_MAX_SUBS       500

/*
 * Defined in mqtt.c
 * These are not exported entry points, but are external so they show up in the symbol table.
 */
extern HOT int ism_mqtt_receive(ism_transport_t * transport, char * buf, int buflen, int kind);
extern void ism_mqtt_replyAction(int32_t rc, void * handle, void * vaction);
extern void ism_mqtt_replySubscribe(int32_t rc, void * handle, void * vaction);
extern void ism_mqtt_replyUnSubscribe(int32_t rc, void * handle, void * vaction);
extern HOT bool ism_mqtt_replyMessage(ismEngine_ConsumerHandle_t consumerh,
		ismEngine_DeliveryHandle_t deliveryh, ismEngine_MessageHandle_t  msgh,
        uint32_t seqnum, ismMessageState_t state,
        uint32_t options, ismMessageHeader_t * hdr, uint8_t areas, ismMessageAreaType_t areatype[areas],
        size_t areasize[areas], void * areaptr[areas], void * vaction, ismEngine_DelivererContext_t * delivererContext);
extern void ism_mqtt_replyClosing(int32_t rc, void * handle, void * vaction);
extern void ism_mqtt_replyPublish(int32_t rc, void * handle, void * vaction);
extern void ism_mqtt_replyPutMessage(int32_t rc, void * handle, void * vaction);
extern void ism_mqtt_replyPubAck(int32_t rc, void * handle, void * vaction);
extern void ism_mqtt_replySteal(int32_t reason, ismEngine_ClientStateHandle_t handle, uint32_t options, void * vaction);
extern void ism_mqtt_doConnect(ism_transport_t * transport, mqttMsg_t * mmsg);
extern void ism_mqtt_doDisconnect(ism_transport_t * transport, mqttMsg_t * mmsg);
extern void ism_mqtt_doPublish(ism_transport_t * transport, mqttMsg_t * mmsg);
extern void ism_mqtt_doPubAck(ism_transport_t * transport, uint16_t msgid, int mqttrc);
extern void ism_mqtt_doSubscribe(ism_transport_t * transport, mqttMsg_t * mmsg);
extern void ism_mqtt_doUnsubscribe(ism_transport_t * transport, mqttMsg_t * mmsg);
extern void ism_mqtt_expandJson(ism_json_parse_t * parseobj, int ent, concat_alloc_t * buf);
extern void ism_mqtt_replyCloseClient(ism_transport_t * transport);
extern void ism_mqtt_replyDisconnect(int32_t rc, void * handle, void * vaction);
extern void ism_mqtt_replyDoneConnection(int32_t rc, void * handle, void * vaction);
extern void ism_mqtt_putJsonPayload(ism_transport_t * transport, concat_alloc_t * buf, char * body,
        uint32_t bodylen, char inarray);
extern void ism_mqtt_putJsonPayloadContent(ism_transport_t * transport, concat_alloc_t * buf, char * body,
        uint32_t bodylen, char inarray);
extern int ism_mqtt_resume(ism_transport_t * transport, void * userdata);

/*
 * An action header used to pass a single pointer
 */
typedef struct mqtt_act_t {
	uint8_t           action;
	uint8_t           command;
	uint8_t           clean;
	uint8_t           qos;
	int               rc;
	int               consumer;
	uint16_t          msgid;
	uint8_t           unset_retained;
    uint8_t           resumed_session;
	ism_transport_t * transport;
	void *            handle; /* Message handle for Will message */
	char *            topic;  /* Will message topic */
	char              locale [8];
	uint8_t           will_qos; /* Will message QoS */
	uint8_t           isMsgid;
	uint8_t           inherit_durable;
	uint8_t           resv[1];
	uint32_t          msgExpire;
} mqtt_act_t;

/*
 * Possible values for job->subscriptionFound
 */
enum sub_found_type_e {
    SUB_NotFound    = 0,
    SUB_Found       = 1,
    SUB_Error       = 2,
    SUB_Resubscribe = 3
};

/*
 * Defined in msgid.h
 * These are not exported entry points, but are used from another source module within protocol
 */
typedef struct ism_msgid_list_t ism_msgid_list_t;
/*
 * Message ID entry
 */
typedef struct ism_msgid_info_t {
    __uint128_t handle; /* Saved handle            */
    struct ism_msgid_info_t * next; /* Next entry or next free */
    uint16_t msgid; /* Message ID              */
    uint16_t state; /* Message state           */
    int      pending;
} ism_msgid_info_t;


/*
 * MQTT property support (wd10 level)
 */
enum mqtt_prop_id_e {
    MPI_PayloadFormat      = 0x01,
    MPI_MsgExpire          = 0x02,
    MPI_ContentType        = 0x03,
    MPI_ReplyTopic         = 0x08,
    MPI_Correlation        = 0x09,
    MPI_SubID              = 0x0B,
    MPI_SessionExpire      = 0x11,
    MPI_ClientID           = 0x12,
    MPI_KeepAlive          = 0x13,
    MPI_AuthMethod         = 0x15,
    MPI_AuthData           = 0x16,
    MPI_RequestReason      = 0x17,
    MPI_WillDelay          = 0x18,
    MPI_RequestReplyInfo   = 0x19,
    MPI_ReplyInfo          = 0x1A,
    MPI_ServerReference    = 0x1C,
    MPI_Reason             = 0x1F,
    MPI_MaxReceive         = 0x21,
    MPI_MaxTopicAlias      = 0x22,
    MPI_TopicAlias         = 0x23,
    MPI_MaxQoS             = 0x24,
    MPI_RetainAvailable    = 0x25,
    MPI_UserProperty       = 0x26,
    MPI_MaxPacketSize      = 0x27,
    MPI_WildcardAvailable  = 0x28,
    MPI_SubIDAvailable     = 0x29,
    MPI_SharedAvailable    = 0x2A
};

/*
 * Control packet masks
 */
enum mqtt_prop_packet_e {
    CPOI_CONNECT         = 0x0001,
    CPOI_CONNACK         = 0x0002,
    CPOI_CONNECT_CONNACK = 0x0003,
    CPOI_S_PUBLISH       = 0x10004,   /* Is ALT */
    CPOI_C_PUBLISH       = 0x0008,
    CPOI_PUBLISH         = 0x000C,
    CPOI_PUBACK          = 0x0010,
    CPOI_PUBREC          = 0x0010,
    CPOI_PUBCOMP         = 0x0020,
    CPOI_SUBSCRIBE       = 0x0040,
    CPOI_SUBACK          = 0x0080,
    CPOI_UNSUBSCRIBE     = 0x0100,
    CPOI_UNSUBACK        = 0x0200,
    CPOI_S_DISCONNECT    = 0x0400,
    CPOI_C_DISCONNECT    = 0x0800,
    CPOI_DISCONNECT      = 0x0C00,
    CPOI_AUTH            = 0x1000,
    CPOI_WILLPROP        = 0x2000,
    CPOI_AUTH_CONN_ACK   = 0x1003,
    CPOI_ACK             = 0x1EB2,
    CPOI_USERPROPS       = 0x3FFF
};

/*
 * MQTT return codes (CSD02)
 */
enum mqtt_rc_e {
    MQTTRC_OK                 = 0x00,
    MQTTRC_QoS0               = 0x00,  /*   0 QoS=0 return on SUBSCRIBE */
    MQTTRC_QoS1               = 0x01,  /*   1 QoS=1 return on SUBSCRIBE */
    MQTTRC_QoS2               = 0x02,  /*   2 QoS=2 return on SUBSCRIBE */
    MQTTRC_KeepWill           = 0x04,  /*   4 Normal close of connection but keep the will */
    MQTTRC_NoSubscription     = 0x10,  /*  16 There was no matching subscription on a PUBLISH */
    MQTTRC_NoSubExisted       = 0x11,  /*  17 No subscription existed on UNSUBSCRIBE */
    MQTTRC_ContinueAuth       = 0x18,  /*  24 Continue authentication */
    MQTTRC_Reauthenticate     = 0x19,  /*  25 Start a re-authentication */
    MQTTRC_UnspecifiedError   = 0x80,  /* 128 Unspecified error */
    MQTTRC_MalformedPacket    = 0x81,  /* 129 The packet is malformed */
    MQTTRC_ProtocolError      = 0x82,  /* 130 Protocol error */
    MQTTRC_ImplError          = 0x83,  /* 131 Implementation specific error */
    MQTTRC_UnknownVersion     = 0x84,  /* 132 Unknown MQTT version */
    MQTTRC_IdentifierNotValid = 0x85,  /* 133 ClientID not valid */
    MQTTRC_BadUserPassword    = 0x86,  /* 134 Username or Password not valid */
    MQTTRC_NotAuthorized      = 0x87,  /* 135 Not authorized */
    MQTTRC_ServerUnavailable  = 0x88,  /* 136 The server in not available */
    MQTTRC_ServerBusy         = 0x89,  /* 137 The server is busy */
    MQTTRC_ServerBanned       = 0x8A,  /* 138 The server is banned from connecting */
    MQTTRC_ServerShutdown     = 0x8B,  /* 139 The server is being shut down */
    MQTTRC_BadAuthMethod      = 0x8C,  /* 140 The authentication method is not valid */
    MQTTRC_KeepAliveTimeout   = 0x8D,  /* 141 The Keep Alive time has been exceeded */
    MQTTRC_SessionTakenOver   = 0x8E,  /* 142 The ClientID has been taken over */
    MQTTRC_TopicFilterInvalid = 0x8F,  /* 143 The topic filter is not valid for this server */
    MQTTRC_TopicInvalid       = 0x90,  /* 144 The topic name is not valid for this server */
    MQTTRC_PacketIDInUse      = 0x91,  /* 145 The PacketID is in use */
    MQTTRC_PacketIDNotFound   = 0x92,  /* 146 The PacketID is not found */
    MQTTRC_ReceiveMaxExceeded = 0x93,  /* 147 The Receive Maximum value is exceeded */
    MQTTRC_TopicAliasInvalid  = 0x94,  /* 148 The topic alias is not valid */
    MQTTRC_PacketTooLarge     = 0x95,  /* 149 The packet is too large */
    MQTTRC_MsgRateTooHigh     = 0x96,  /* 150 The message rate is too high */
    MQTTRC_QuotaExceeded      = 0x97,  /* 151 A client quota is exceeded */
    MQTTRC_AdminAction        = 0x98,  /* 152 The connection is closed due to an administrative action */
    MQTTRC_PayloadInvalid     = 0x99,  /* 153 The payload format is invalid */
    MQTTRC_RetainNotSupported = 0x9A,  /* 154 Retain not supported */
    MQTTRC_QoSNotSupported    = 0x9B,  /* 155 The QoS is not supported */
    MQTTRC_UseAnotherServer   = 0x9C,  /* 156 Temporary use another server */
    MQTTRC_ServerMoved        = 0x9D,  /* 157 Permanent use another server */
    MQTTRC_SharedNotSupported = 0x9E,  /* 158 Shared subscriptions are not allowed */
    MQTTRC_ConnectRate        = 0x9F,  /* 159 Connection rate exceeded */
    MQTTRC_MaxConnectTime     = 0xA0,  /* 160 Shared subscriptions are not allowed */
    MQTTRC_SubIDNotSupported  = 0xA1,  /* 161 Subscription IDs are not supportd */
    MQTTRC_WildcardNotSupported = 0xA2 /* 162 Wildcard subscriptions are not supported */
};

extern void ism_msgid_init(void);
extern ism_msgid_list_t * ism_create_msgid_list(ism_transport_t *transport, int isRX, uint16_t range);
extern void ism_msgid_freelist(struct ism_msgid_list_t * list);
extern int ism_msgid_addMsgIdInfo(ism_msgid_list_t * mlist, __uint128_t handle, uint16_t msgid, uint16_t state);
extern ism_msgid_info_t * ism_msgid_getMsgIdInfo(ism_msgid_list_t * mlist, uint16_t msgid);
extern __uint128_t ism_msgid_delMsgIdInfo(ism_msgid_list_t * mlist, uint16_t msgid, int *pPending);
#endif
