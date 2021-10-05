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

#ifndef __MQTTPACKET_H
#define __MQTTPACKET_H

#include <byteswap.h>

#include "mbconstants.h"

/* ***************************************************************************************
 * Network macros
 * ***************************************************************************************/
#define INT_SIZE    4
#define SHORT_SIZE  sizeof(uint32_t)
#define LONG_SIZE   sizeof(uint64_t)

#define htonll(x) bswap_64(x)

#ifndef ntohll
	#define ntohll(x)  htonll(x)
#endif

/* TSC timestamp storage type */
typedef union
{
	double   d;
	uint32_t i[2];		/* i[0] is the low TSC register, i[1] is the high TSC register */
	uint64_t l;
} uli;

#if __BYTE_ORDER == __LITTLE_ENDIAN
 	#define F_LITTLE_ENDIAN
	#define endian_int16(x)  bswap_16(x)
	#define endian_int32(x)  bswap_32(x)
	#define endian_int64(x)  bswap_64(x)
	__inline__ static float  endian_float(float x)   {union{uint32_t i; float  f;}t;t.f=(x);t.i=bswap_32(t.i); return t.f;}
	__inline__ static double endian_double(double x) {union{uint64_t i; double f;}t;t.f=(x);t.i=bswap_64(t.i); return t.f;}
#else
	#define F_BIG_ENDIAN
#endif

/* ***************************************************************************************
 * packet operation macros
 * ***************************************************************************************/
#define NgetChar(ptr,val)     {val=ptr[0];ptr++;}
#define NgetShort(ptr,val)    {uint16_t netval;memcpy(&netval,ptr,SHORT_SIZE);val=ntohs(netval);ptr+=SHORT_SIZE;}
#define NgetInt(ptr,val)      {uint32_t netval;memcpy(&netval,ptr,INT_SIZE);val=ntohl(netval);ptr+=INT_SIZE;}
#define NgetLong(ptr,val)     {uli netval;memcpy(&netval.l,ptr,LONG_SIZE);val.l=ntohll(netval.l);ptr+=LONG_SIZE;}
#define NgetLongX(ptr,val)    {uint64_t netval;memcpy(&netval,ptr,LONG_SIZE);val=ntohll(netval);ptr+=LONG_SIZE;}
#define NgetDouble(ptr,val)   NgetLong(ptr,val)

#define NputChar(ptr,val)     {ptr[0]=val;ptr++;}
#define NputShort(ptr,val)    {uint16_t netval;netval=htons(val);memcpy(ptr,&netval,SHORT_SIZE);ptr+=SHORT_SIZE;}
#define NputInt(ptr,val)      {uint32_t netval;netval=htonl(val);memcpy(ptr,&netval,INT_SIZE);ptr+=INT_SIZE;}
#define NputLong(ptr,val)     {uli netval;netval.l=htonll(val.l);memcpy(ptr,&netval.l,LONG_SIZE);ptr+=LONG_SIZE;}
#define NputLongX(ptr,val)    {uint64_t netval;netval=htonll(val);memcpy(ptr,&netval,LONG_SIZE);ptr+=LONG_SIZE;}
#define NputDouble(ptr,val)   NputLong(ptr,val)

/* WRITE macros will increment the ptr by the size. */
#define WRITECHAR(ptr,val)    {ptr[0]=val;ptr++;}                                 /* Write 1 Byte Integer at ptr */
#define WRITEINT16(ptr,val)   {ptr[0]=(val>>8);ptr[1]=val;ptr+=2;}                /* Write 2 Byte Integer at ptr */
#define WRITEUTF(ptr,val,len) {memcpy(ptr,val,len);ptr+=len;}                     /* Write x Bytes at ptr */

/* READ macros do NOT increment the ptr. */
#define READINT16(ptr)        (((int)(uint8_t)(ptr)[0]<<8) | (uint8_t)(ptr)[1])   /* Read 2 Byte Integer at ptr */
#define READCHAR(ptr)         ((int)(uint8_t)(ptr)[0])                            /* Read 1 Byte Integer at ptr */
#define READUTF(val,ptr,len)  {memcpy(val,ptr,len);}                              /* Read x Bytes at ptr */

/* ***************************************************************************************
 * MQTT property support (wd10 level)
 * ***************************************************************************************/
typedef enum mqtt_prop_id_e {
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
} mqtt_prop_id_e;

/*
 * Control packet masks
 */
typedef enum mqtt_prop_packet_e {
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
} mqtt_prop_packet_e;

/*
 * MQTT return codes (CSD02)
 */
enum mqtt_rc_e {
    MQTTRC_OK                 = 0x00,
    MQTTRC_QoS0               = 0x00,  /*   0 QoS=0 return on SUBSCRIBE */
    MQTTRC_QoS1               = 0x01,  /*   1 QoS=1 return on SUBSCRIBE */
    MQTTRC_QoS2               = 0x02,  /*   2 QoS=2 return on SUBSCRIBE */
    MQTTRC_KeepWill           = 0x04,  /*   4 Normal close of connection but keep the will */
	MQTTRC_V311NotAuthorized  = 0x05,  /*   5 MQTT 3.1.1 client not authorized to connect */
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

/* MQTT User property, a UTF-8 Name/Value Pair */
typedef struct mqtt_user_prop_t {
	uint16_t namelen;
	uint16_t valuelen;
	char    *name;
	char    *value;
} mqtt_user_prop_t;

/* MQTT V5 User Properties Lengths */
#define USRPROP_NAME_LEN                 2
#define USRPROP_VALUE_LEN                2
#define USRPROP_ID_LEN                   1

#define UINT64_HEXSTRING_LEN             16
#define UINT64_ASCIISTRING_LEN           64

/* MQTT V5 User Properties  - RESERVED for MQTTBENCH */
#define UPROP_TIMESTAMP           "$TS"
#define UPROP_TIMESTAMP_LEN       0x03
#define UPROP_SEQNUM              "$SN"
#define UPROP_SEQNUM_LEN          0x03
#define UPROP_STREAMID            "$SID"
#define UPROP_STREAMID_LEN        0x04


/* ***************************************************************************************
 * Packet Structures from MQTTPacket.h
 * ***************************************************************************************/

typedef void* (*pf)(unsigned char, char*, int);

/* ------------------------------------------------------------------------------------
 * Bit fields for the WebSocket header 2 bytes.
 * ------------------------------------------------------------------------------------ */
typedef union
{
	char flags[2];
	uint16_t  all;              /* Both bytes */
	struct
	{
		uint16_t len : 7;	    /* data length */
		uint16_t mask : 1;	    /* data is masked flag bit */
		uint16_t opCode : 4;    /* opCode nibble */
		uint16_t rsvrd : 3;     /* 3 reserved bits. */
		uint16_t final : 1;		/* final fragment indicator bit */
	} bits;
} WSFrame;

/* ------------------------------------------------------------------------------------
 * Bit fields for the MQTT header byte.
 * ------------------------------------------------------------------------------------ */
typedef union
{
	char     byte;	           /* the whole byte */
	struct
	{
		uint8_t retain : 1;	   /* retained flag bit */
		uint8_t qos : 2;       /* QoS value, 0, 1 or 2 */
		uint8_t dup : 1;	   /* DUP flag bit */
		uint8_t type : 4;      /* message type nibble */
	} bits;
} Header;

/* ------------------------------------------------------------------------------------
 * Data for a CONNECT packet.
 * ------------------------------------------------------------------------------------ */
typedef struct
{
	Header   header;	     /* MQTT header byte */
	union
	{
		uint8_t all;	 /* all CONNECT flags */
		struct
		{
			uint8_t : 1;	           /* unused */
			uint8_t cleanstart : 1;    /* cleansession flag */
			uint8_t will : 1;          /* will flag */
			uint8_t willQoS : 2;       /* will QoS value */
			uint8_t willRetain : 1;    /* will retain setting */
			uint8_t password : 1;      /* 3.1 password */
			uint8_t username : 1;      /* 3.1 user name */
		} bits;
	} flags;	             /* connect flags byte */

	char    *Protocol,       /* MQTT protocol name */
		    *clientID,       /* string client id */
            *willTopic,	     /* will topic */
            *willMsg;	     /* will payload */

	int      keepAliveTimer; /* keepalive timeout value in seconds */
	unsigned char  version;	 /* MQTT version number */
} Connect;

/* ------------------------------------------------------------------------------------
 * Data for a CONNACK packet.
 * ------------------------------------------------------------------------------------ */
typedef struct
{
	union {																		/* First byte */
		uint8_t all;	 					    /* all CONNACK flags */
		struct {
			uint8_t  sessPresent : 1;           /* Session Present Flag */
			uint8_t  rsvrd : 7;  			    /* Reserved bits */
		};
	} flags;	             					/* connack flags byte */

	uint8_t     rc;            					/* connack return code */		/* Second byte */
} Connack;

/* ------------------------------------------------------------------------------------
 * Data for a SUBSCRIBE Option Byte.
 * ------------------------------------------------------------------------------------ */
typedef union
{
	uint8_t     byte;              /* The whole Subscribe Option Byte */
	struct
	{
		uint8_t qos : 2;           /* QoS value, 0, 1 or 2 */
		uint8_t noLocal : 1;	   /* No Local Option bit */
		uint8_t retainPub : 1;     /* Retain Publish bit */
		uint8_t retainHand : 2;    /* Retain Handling bits */
		uint8_t rsrvd : 2;         /* Reserved */
	} bits;
} SubOption;

/* ------------------------------------------------------------------------------------
 * Data for a PUBLISH packet.
 * ------------------------------------------------------------------------------------ */
typedef struct
{
	Header   header;         /* MQTT header byte */
	char    *topic;          /* topic string */
	int      topiclen;
	uint16_t msgId;	         /* MQTT message id */
	char    *payload;	     /* binary payload, length delimited */
	int      payloadlen;     /* payload length */
} Publish;

/* ------------------------------------------------------------------------------------
 * Complete structure for an ACK Message
 * ------------------------------------------------------------------------------------ */
typedef struct
{
	Header   header;
	char     remainLen;
	char     resvd;
	uint16_t msgid;
} mqttAckMessage;

/* ------------------------------------------------------------------------------------
 * Data for one of the ACK packets.
 * ------------------------------------------------------------------------------------ */
typedef struct
{
	uint16_t msgId;		     /* MQTT message id */
} Ack;

typedef Ack Puback;
typedef Ack Pubrec;
typedef Ack Pubrel;
typedef Ack Pubcomp;
typedef Ack Unsuback;

/* ------------------------------------------------------------------------------------
 * Client WILL message data
 * ------------------------------------------------------------------------------------ */
typedef struct
{
	char    	*topic;
	int			 topicLen;
	char    	*payload;
	int			 payloadLen;
	uint8_t      retained;
	uint8_t      qos;

	/* MQTT v5 specific fields */
	uint8_t            numUserProps;   /* number of MQTT User-Defined properties */
	mqtt_user_prop_t **userPropsArray; /* array of MQTT User-Defined properties provided in the message file */
	uint8_t            numMQTTProps;   /* number of MQTT Spec-Defined property IDs */
	uint8_t           *propIDsArray;   /* array of MQTT Spec-Defined property IDs provided in the message file (value is stored in propsBuf) */
	concat_alloc_t    *propsBuf;       /* buffer used to hold Spec-Defined AND User-Defined MQTT properties */
} willMessage_t;

#endif /* __MQTTPACKET_H */
