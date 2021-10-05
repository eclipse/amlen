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

#ifndef PXMQTT_H
#define PXMQTT_H

#include <pxtransport.h>
#ifdef __cplusplus
extern "C" {
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

#define MT_PUBLISHX    0x3F
#define MT_GETX        0x5F
#define MT_SUBSCRIBEX  0x8F
#define MT_SENDACL     0xCF
#define MT_GW_PINGREG  0xCE
#define MT_SENDSUBS    0xDF
#define MT_GW_PINGRES  0xDE

/*
 * Enumerated values for return codes.
 * The first few of these are defined return code values on the CONACK
 * command.  The others are internal return code.  If an error is found
 * anywhere except on a connect (and then only some of the cases) the
 * action is to disconnect.
 */
enum conack_e {
    /* Return codes from the MQTT spec */
    CRC_MinConnectRc   = 0x000,
    CRC_Accepted       = 0x000,
    CRC_InvalidVersion = 0x001,
    CRC_BadIdentifier  = 0x002,
    CRC_NotAvailable   = 0x003,
    CRC_BadUser        = 0x004,
    CRC_NotAuthorized  = 0x005,
    CRC_MaxConnectRc   = 0x006
};


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
 * MQTTv5 property defintions
 */
enum mqtt_ext_id_e {
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
 * MQTTv5 Control packet masks (used to define which packets properties are allowed in)
 */
enum mqtt_ext_packet_e {
    CPOI_CONNECT         = 0x0001,
    CPOI_CONNACK         = 0x0002,
    CPOI_CONNECT_CONNACK = 0x0003,
    CPOI_S_PUBLISH       = 0x10004,   /* Is ALT */
    CPOI_C_PUBLISH       = 0x0008,
    CPOI_PUBLISH         = 0x000C,
    CPOI_PUBACK          = 0x0010,
    CPOI_PUBREC          = 0x0010,    /* For now PUBREC is the same as PUBACK */
    CPOI_PUBCOMP         = 0x0020,    /* For now PUBREL is the same as PUBCOMP */
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

/*
 * Parsed MQTT PUBLISH packet
 * This is a temporary object for passing parsed information
 * Any pointers must be either constants or stack memory
 */
typedef struct mqtt_pmsg_t {
    const char * buf;         /* Input - buffer containing publish message */
    uint32_t     buflen;      /* Input - length of buffer */
    uint32_t     cmd;         /* Input - command byte (includes QoS */
    const char * topic;       /* Output - topic name */
    uint32_t     topic_len;   /* Output - topic length */
    uint32_t     payload_len; /* Output - payload length */
    const char * payload;     /* Output - payload */
    const char * props;       /* Output - properties */
    uint32_t     prop_len;    /* Output - property length */
    uint32_t     hdr_count;   /* The Kafka headers count */
    const char * headers;     /* The Kafka headers */
    uint32_t     hdr_len;     /* The kafka headers length */
    uint32_t     key_len;     /* The kafka key length */
    const char * key;         /* The kafka generated key */
    uint32_t     packetID;    /* The packet ID (message ID) of the publish */
    uint32_t     waitValue;   /* The integer wait value (commonly the packet ID) */
    uint64_t     waitID;      /* The ID used to send ACKs */
    concat_alloc_t * route_props;
    concat_alloc_t * user_props;
    const char * type;        /* Parsed type field from topic */
    const char * id;          /* Parsed id field from topic */
    const char * event;       /* Parsed event field from topic */
    const char * format;      /* Parsed format field from topic */
    ism_prop_t * msgprops;    /* Expanded props used locally by a mapper */
} mqtt_pmsg_t;

int ism_mqtt_getPublishPayload(ism_transport_t * transport, mqtt_pmsg_t * pmsg);
int ism_kafka_publishEvent(ism_transport_t * transport, mqtt_pmsg_t * pmsg, const char * kafkaTopic,
        const char * kafkaKey, int kafkaKey_Len, int hdrcount, const char * headers, int hdr_len);
int ism_mqtt_getPublishPayload(ism_transport_t * transport, mqtt_pmsg_t * pmsg);

/*
 * Load a big endian 2 byte integer
 */
#define BIGINT16(p) (((int)(uint8_t)(p)[0]<<8) | (uint8_t)(p)[1])


#ifndef MQTT_CONST_ONLY

typedef struct px_mqtt_stats_t {
    volatile uint32_t    mqttPendingAuthenticationRequests;
    volatile uint32_t    mqttPendingAuthorizationRequests;
    volatile uint32_t    mqttConnections;
    volatile uint32_t    mqttConnectedDev;
    volatile uint32_t    mqttConnectedApp;
    volatile uint32_t    mqttConnectedGWs;
    volatile uint64_t    mqttConnectionsTotal;
    volatile uint64_t    mqttC2PMsgsSent;
    volatile uint64_t    mqttC2PMsgsReceived[3];
    volatile uint64_t    mqttP2SMsgsSent;
    volatile uint64_t    mqttP2SMsgsReceived[3];
    volatile uint32_t    mqttVersion3_1;
    volatile uint32_t    mqttVersion3_1_1;
    volatile uint32_t    mqttVersion5_0;
    volatile uint32_t    mqttUseWebsocket;
} px_mqtt_stats_t;

XAPI int ism_proxy_getMQTTStats(px_mqtt_stats_t * stats);

enum willPolicy { WillTopicPolicy_RemoveNotValid = 0,       //Remove Will topic if it is not authorized
                  WillTopicPolicy_ReturnErrorNotValid = 1,  //Return connect error (Not authorized) if will topic is not authorized
                  WillTopicPolicy_RemoveAll = 2,             //Remove all Will topics
                  WillTopicPolicy_AllowAll = 3              //Allow all will topics (No Authorization consideration)
};
#endif

#ifdef __cplusplus
}
#endif
#endif
