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

#ifndef __MQTTCLIENT_H
#define __MQTTCLIENT_H

#include <ismutil.h>

#include "mbconstants.h"
#include "mqttpacket.h"
#include "mqttbench.h"
#include "tcpclient.h"

/* MQTT Protocol Versions Enum */
enum mqttVersions {
	MQTT_V3		= 3,
	MQTT_V311	= 4,
	MQTT_V5		= 5
};

/* ------------------------------------------------------------------------------------
 * Structure for Source IP Addresses provided by environment variable:  SIPList   This
 * is used with the Destination IP Address provided via a client list.
 * ------------------------------------------------------------------------------------ */
typedef struct {
	uint16_t nextSIP;
	uint16_t numElements;
	char   **sipArrayList;
} srcIPList_t;

/* ------------------------------------------------------------------------------------
 * Structure to store a list of destination IPv4 addresses that are returned from a
 * DNS query. There may be one or more DNS records returned per DNS name.
 * ------------------------------------------------------------------------------------ */
typedef struct {
	uint16_t nextDIP;
	uint16_t numElements;
	char   **dipArrayList;
} dstIPList_t;

/* ------------------------------------------------------------------------------------
 * Structure for handling situation where the topic name contains the ${COUNT:x-y} variable
 * embedded in the name and needs to be expanded. All properties of the topic are cloned
 * and the topic name is incremented according the values and placement of the ${COUNT:x-y}
 * variable.  x is the start of the sequence and y is the end of the sequence.
 * ------------------------------------------------------------------------------------ */
typedef struct {
	int      startNum;
	int      endNum;

	char     topicName_Pfx[MQTT_MAX_TOPIC_NAME];
	char     topicName_Sfx[MQTT_MAX_TOPIC_NAME];
} topicInfo_t;

/* ------------------------------------------------------------------------------------
 * Structure for destination list
 *
 * NOTE:  There is a separate counter for QoS 1 and QoS 2 received PUBLISH messages.
 *        The Topic is subscribed to at a specific QoS but won't necessarily get the
 *        SUBACK back with the subscribed to QoS  ....OR.... the PUBLISHed message by
 *        the publisher may be at a lower level than subscribed to.
 * ------------------------------------------------------------------------------------ */
typedef struct {
	uint64_t currQoS0MsgCount;  /* number of QoS 0 messages sent (topic) or received (subscription) */
	uint64_t currQoS1MsgCount;  /* number of QoS 1 messages sent (topic) or received (subscription) */
	uint64_t currQoS2MsgCount;  /* number of QoS 2 messages sent (topic) or received (subscription) */

	uint64_t currPUBACKCount;
	uint64_t currPUBRECCount;
	uint64_t currPUBRELCount;
	uint64_t currPUBCOMPCount;

	uint64_t badRCCount;
	uint64_t rxRetainCount;		/* number of retained messages received for this subscription */
	uint64_t rxDupCount;		/* number of duplicate/republished messages received for this subscription */
	uint64_t txNoSubCount;		/* number of QoS > 0 messages published and accepted by the MQTT server, but there are no subscribers */
	int      topicNameLength;
	int      nextMsgIdIndex;

	uint64_t streamID;			/* For publish topics, this is a globally unique ID for this stream of messages */
    char *streamIDStr;          /* string representation of the stream ID */
    int streamIDStrLen;			/* length of the stream ID string */
    uint8_t  fromMD;            /* a flag to indicate that this destTuple was configured from a message file in a message directory */
    mqttclient *client;         /* reference to the MQTT client that owns this destTuple */

    uint8_t  mappedAlias;		/* a boolean flag to indicate that the mapping has been published for this connection */
    uint16_t topicAlias;		// PUBLISH						- the alias for this topic
	uint8_t  topicQoS;			// PUBLISH/SUBSCRIBE:	3.3.1.2 - the Quality of Service (QoS) governing message delivery and reliability on this topic. Default is 0.
	uint8_t  retain;			// PUBLISH:				3.3.1.3 - indicates whether the retained flag is set on messages published to the topic. Default is false.

	char    *topicName;			// PUBLISH/SUBSCRIBE:	3.3.2.1 - the topic string to publish or subscribe to (e.g. "/this/is/a/topic" or "/this/is/a/topic/with/a/wildcard/+", wildcards are not valid in PUBLISH, only SUBSCRIBE)

	/* MQTT v5 and later fields */
	int		 subId;				// PUBLISH/SUBSCRIBE:	3.3.2.3.8/3.8.2.1.2 - identifier of a SUBSCRIBE packet, also used in the PUBLISH packet to indicate which subscription resulted in the delivery of the PUBLISH packet to this client
	uint8_t	 noLocal;			// SUBSCRIBE:	3.8.3.1 - if set to true, the server must not forward this message to a connection with a client ID equal to the client ID of the publishing connection
	uint8_t  retainAsPublished; // SUBSCRIBE:	3.8.3.1 - if set to true, messages forwarded using this subscription keep the retain flag they were published with. if set to false, server should set retain flag to 0 on forwarded message.
	uint8_t  retainHandling;	// SUBSCRIBE:	3.8.3.1 - 0 = send retained message at time of subscribe, 1 = send only if subscription does not currently exist, 2 = do not send retained message at time of subscribe

	uint8_t            numUserProps;     // number of MQTT User-Defined properties
	mqtt_user_prop_t **userPropsArray;   // array of MQTT User-Defined properties provided in the message file
	uint8_t            numMQTTProps;     // number of MQTT Spec-Defined property IDs
	uint8_t           *propIDsArray;     // array of MQTT Spec-Defined property IDs provided in the message file (value is stored in propsBuf)
	concat_alloc_t    *propsBuf;         // buffer used to hold Spec-Defined AND User-Defined MQTT properties

} destTuple_t;  // IMPORTANT - IF YOU ADD POINTERS TO THIS STRUCT, THEN UPDATE deepCopyTuple() function

/* ***************************************************************************************
 * msgid_info_t structure
 * ***************************************************************************************/
typedef struct {
    destTuple_t *destTuple;            /* destTuple associated with this msg id object */
    uint8_t state;                     /* the current state of the msg ID object (e.g. see publishMsgIDStates or subscribeMsgIDStates) */
} msgid_info_t;

/* ***************************************************************************************
 * mqttclient_t structure
 * ***************************************************************************************/
typedef struct mqttclient_t {
	uint64_t currRxMsgCounts[MQTT_NUM_MSG_TYPES];	/* rx array of MQTT message types counters (per client) */
	uint64_t currTxMsgCounts[MQTT_NUM_MSG_TYPES];	/* tx array of MQTT message types counters (per client) */

	pthread_spinlock_t mqttclient_lock; /* client lock */

	int      line;					   /* line in the client list file where this client appears */
	int      connIdx;
	int      protocolState;

	uint8_t  mqttVersion;              /* The Version of MQTT used in MQTT Communications. */
	uint8_t  ioProcThreadIdx;		   /* The I/O processor thread assigned to service this client */
	uint8_t  ioListenerThreadIdx;	   /* The I/O listener thread assigned to service this client */
	uint8_t  traceEnabled;             /* Flag to indicate if low level trace (e.g. log every connect, disconnect, publish, subscribe, etc.) is enabled for this client.
	 	 	 	 	 	 	 	 	 	  Use command -clientTrace <regex> <enable/disable> command to enable/disable trace)*/

	mqttbench *mbinst;          	   /* The mqttbench thread this client is associated with */

	/* Transport related fields */
	uint16_t serverPort;               /* the destination port of the message broker endpoint to connect to */
	char    *server;                   /* the configured destination DNS name or IP address in the client list file */
	char    *serverIP;				   /* the resolved IP address of the message broker to connect to
										  (in case of multiple DNS records were returned, this will be selected in round robin order) */
	char    *srcIP;					   /* the source IPv4 address assigned to this client from the list of local IPv4 addresses which can
	 	 	 	 	 	 	 	 	 	  reach the server IP address */
	transport *trans;                  /* transport object used by the mqttclient */
	do_data_t  doData;				   /* application callback for processing messages */

	int      keepAliveInterval;
	int      retryInterval;
	int      numPubRecs;			   /* Keep track of how many PUBRELs have not been handled (submitted) */
	int      subackCount;
	int      unsubackCount;
	int      lastErrorNo;
	int      socketErrorCt;
	int      connectionTimeoutSecs;

	/* Topic/Subscription related fields */
	int      destRxListCount;
	int      destTxListCount;
	destTuple_t **destRxList;
	destTuple_t **destTxList;
	uint8_t  *destTxAliasMapped;	   /* a table which stores the state of alias mapping for publish topics (0=not yet mapped, 1=topic alias has been mapped) */
	int      topicIdxFromCL;           /* index into the publish topic array (i.e. destTxList) for the next topic (from CLient List) to publish a message to */
	int      topicIdxFromMD;           /* index into the publish topic array (i.e. destTxList) for the next topic (from Message Directory) to publish a message to, IF
                                          the message object */
	int     *destTxFromCL;             /* array of indexes in the destTxList array, for message objects which do NOT have a configured topic - from Client List */
	int     *destTxFromMD;             /* array of indexes in the destTxList array, for message objects which DO have a configured topic     - from Message Directory */
	int      destTxFromCLCount;        /* number of elements in destTxFromCL array */
	int      destTxFromMDCount;        /* number of elements in destTxFromMD array */

	ismHashMap *destRxSubMap;          /* For MQTT v3.1.1 and earlier - Hash table of subscriptions (topic name is the key). Used to lookup destTuple in onMessage, not permitted for wildcard subscriptions */
	ismHashMap *destRxSubIDMap;        /* For MQTT v5 and later       - Hash table of subscriptions (sub ID is the key). Used to lookup destTuple in onMessage which use subID */

	uint8_t  stopLogging;              /* Flag used to rate limit connection retry log messages from this client */
	uint8_t  usesWildCardSubs;         /* this client has at least 1 wildcard subscription, disables use of the destRxSubMap */
	uint8_t  allTopicsUnSubAck;        /* Indicates that all topics have been unsubscribed. */
	uint8_t  disconnectType;           /* Indicates the type of disconnect that the client will perform:
	                                    *       0 = disconnect WITHOUT unsubscribe (default)
	                                    *       1 = disconnect WITH unsubscribe */

	uint8_t  clientType;               /* Provides the type of client: Consumer, Producer or "dual" (producer and consumer) client */
	uint8_t  cleansession;             /* the clean session flag for MQTT v3.1.1 (or earlier) clients, i.e. delete state on connect */
	uint8_t  cleanStart;               /* the clean start flag is new for MQTT v5 client, clean start=1 + sessionExpiryInterval=0 is equivalent to cleansession=1 */
	uint8_t  isSecure;                 /* indicates whether the client is using a secure connection or not */

	uint8_t  issuedDisconnect;
	uint8_t  reconnectStatus;          /* Indication it is performing a reconnection */
	uint8_t  useWebSockets;            /* indicates whether the client is using a websocket connection or not. */
	uint8_t  errorMsgDisplayed;        /* Flag to indicate error message already displayed used when not reconnecting. */

	uint8_t  allSubscribesComplete;    /* Received all SUBACKs for this client. */
	uint8_t  txQoSBitMap;			   /* QoS bit is set if any QoS 0, 1, or 2 messages published by this client (e.g. 1 = QoS 0 only, 3 = QoS 0 and 1, and 7 = QoS 0, 1, and 2 */
	uint8_t  rxQoSBitMap;			   /* QoS bit is set if any QoS 0, 1, or 2 messages subscribed by this client */
	uint8_t  partialBufContainsWSFrame;/* Indicator that the partial buffer contains WebSocket Frames */

	ism_byte_buffer_t *partialMsg;     /* Partial message buffer that was read in. */
	int msgBytesNeeded;                /* Number of Bytes needed to complete the partial message. */

	int16_t  clientIDLen;
	int16_t  usernameLen;
	int16_t  passwordLen;

	/* MQTT v5 specific fields */
	uint8_t            numUserProps;   /* number of MQTT User-Defined properties */
	mqtt_user_prop_t **userPropsArray; /* array of MQTT User-Defined properties provided for this client in the client list file */
	uint8_t            numMQTTProps;   /* number of MQTT Spec-Defined property IDs */
	uint8_t           *propIDsArray;   /* array of MQTT Spec-Defined property IDs provided for this client in the client list file (value is stored in propsBuf) */
	concat_alloc_t    *propsBuf;       /* buffer used to hold Spec-Defined AND User-Defined MQTT properties */

	/* The following elements are for Predefined Message Directories */
	char    *msgDirPath;               /* directory path where to read message files from */
	messagedir_object_t *msgDirObj;	   /* the msg directory object containing the source message object to be transmitted by this client, the -s
	 	 	 	 	 	 	 	 	 	  command line parameter is a artificial case of "message dir" in which generated payloads are stored
	 	 	 	 	 	 	 	 	 	  in a messagedir_object_t and inserted into the msg dir map under the key PARM_MSG_SIZES */
	int16_t  msgObjIndex;			   /* the offset into the message object array of the current message to transmit by this client */
	uint8_t  disconnectRC;			   /* the reason code to be sent on a DISCONNECT message from the client - MQTT v5 ONLY */
	uint8_t  sessionPresent;		   /* a flag included in the CONNACK from the server to indicate that a session state for the client was found */

	uint32_t maxPacketSize;			   /* max MQTT packet size that the client is willing to accept from the server */
	int      currInflightMessages;     /* Current number of messages in flight. */
	int      maxInflightMsgs;          /* The maximum number of Inflight MsgIDs that can be used by this client for publishing messages.
	                                    * For v5 clients the server can decrease this limit based on data returned in the CONNACK, this is a mechanism
	                                    * of flow control that is new in MQTT v5.  This server announced limit is ignored for SUBSCRIBE and UNSUBCRIBE */
	int      allocatedInflightMsgs;    /* The number of Inflight MsgID objects allocated for this client */
	uint16_t availMsgId;               /* next available message ID */
	uint16_t pubMsgId;                 /* msg ID of the most recently transmitted PUBLISH message. */
	int      pubRecMsgId;			   /* msg ID of the most recently received PUBREC message */
	msgid_info_t *inflightMsgIds;      /* list of inflight message IDs for this client */

	char    *clientID;                 /* The unique clientID string needed for each client connection to server. */
	char    *username;
	char    *password;

	uint32_t connRetries;			   /* number of connection retry attempts (mqttbench will abort when MAX_CONN_RETRIES is exceeded */
	uint32_t tcpLatency;               /* TCP Latency for TCP-MQTT or TCP-Subscribe Latency */
	uint32_t mqttLatency;              /* MQTT Latency for TCP-Subscribe Latency */

	double   tcpConnReqSubmitTime;     /* The time at which CONNECT request message was submitted */
	double   mqttConnReqSubmitTime;    /* The time at which a MQTT Connect message was submitted. */
	double   mqttDisConnReqSubmitTime;    /* The time at which a MQTT DISCONNECT message was submitted. */
	double   resetConnTime;            /* The time at which a client was reset (e.g. beginning of a reconnect) */
	double   subscribeConnReqSubmitTime;/* The time at which a Subscribe message was submitted. */
	double   pubSubmitTime;            /* The time at which PUBLISH message was submitted, used to calculate Server PUBACK latency */
	double   nextPingReqTime;          /* The time when the next PINGREQ message must be sent. */
	double   lastMqttConnAckTime;      /* If performing RTT latency, then keep last MQTT CONNACK Time.  Used for deploys
	                                      and also for reconnects. */

	/* Client certificate and Key */
	char    *clientCertPath;           /* file path to client certificate file to send on TLS handshake */
	char    *clientKeyPath;            /* file path to client key file to send on TLS handshake */

	/* TLS cipher using PreShared Keys */
	char    *psk_id;                   /* id for PreSharedKey ; NULL terminated string*/
	unsigned char *psk_key;            /* PreSharedKey for the id ; byte array */
	unsigned int psk_id_len;
	unsigned int psk_key_len;

	time_t lastContact;
	willMessage_t *willMsg;			   /* the Last Will and Testament message to send when this client is not heard from */

    uint16_t     topicalias_count_in;  /* number of topic aliases that this client will accept */
    uint16_t     topicalias_count_out; /* number of topic aliases that this client will send (value will be determined from server, from CONNACK) */
    const char **topicalias_in;		   /* mapping table to inbound (RX PUBLISH) topic aliases and topics */

	/* These 3 settings are stored in NANO seconds due to the timer. */
	uint64_t lingerTime_ns;            /* Linger Time for this Client (overrides -l option). */
	uint32_t lingerMsgsCount;          /* Number of PUBLISH messages sent by this client before it closes the connection, if lingerTime_ns and
	                                      lingerMsgsCount are set, then whichever comes first will initiate the connection close */
	uint64_t initRetryDelayTime_ns;    /* Initial Retry Delay, which is used for resetting when 1st disconnected. */
	uint64_t currRetryDelayTime_ns;    /* Sleep amount between next connection - updated each time */

	/* Counters for assisting in connection issues */
	uint64_t tcpConnCtr;               /* TCP Connection Counter */
	uint64_t tlsConnCtr;               /* TLS Connection Counter */
	uint64_t tlsErrorCtr;              /* TLS Error Counter */
	uint64_t badRxRCCount;			   /* Number of BAD MQTT Return Codes received */
	uint64_t badTxRCCount;			   /* Number of BAD MQTT Return Codes transmitted */
	uint64_t failedSubscriptions;      /* Number of failed subscriptions */
} mqttclient_t;

typedef struct {
	uint64_t streamID;                 /* ID of the message stream */
    uint64_t q0;                       /* latest QoS 0 sequence number seen for this stream */
    uint64_t q1;                       /* latest QoS 1 sequence number seen for this stream */
    uint64_t q2;                       /* latest QoS 2 sequence number seen for this stream */

    uint64_t      outOfOrder;          /* Number of out of order msgs for all QoS' */
    uint64_t      reDelivered;         /* Number of redelivered msgs for all QoS' */
    uint16_t      txInstanceId;        /* the mqttbench instance ID that generated this message stream */
    char         *txTopicStr;          /* the published topic string for this message stream */
    mqttclient_t *rxClient;			   /* the consumer that received this stream */
} msgStreamObj;

typedef struct {
	int      isComplete;
	ism_common_list *clients;
	uint64_t currRXMsgCount;    /* Used in tallying RX count in doCommands tc command.*/
	uint64_t currTXMsgCount;    /* Used in tallying TX count in doCommands tc command.*/
} tmap_t;

/* ************************************************************************************
 * Function Prototypes
 * ************************************************************************************ */
int   allocateInFlightMsgIDs(mqttclient_t * client, mqttbenchInfo_t * mqttbenchInfo, int tunedMsgIdCount);
int   connectMQTTClient (mqttclient_t *, ism_byteBufferPool);
int   createMQTTClientID (mqttclient_t *, int);
int   createMQTTClient (mqttclient_t *, mqttbench_t *, mqttbenchInfo_t *, char *, int, int, int, int);
int   disconnectMQTTClient (mqttclient_t *, ism_byte_buffer_t *);
void  doDisconnectMQTTClient (mqttclient_t *);
int   setMQTTPreSharedKey (mqttclient_t *, int, int, pskArray_t *);
int   startSubmitterThreads (mqttbenchInfo_t *, ioProcThread **, latencyUnits_t *);
int   UTF8_validateString (char *);

void  createMQTTMessageAck (ism_byte_buffer_t *, uint8_t, uint8_t, uint16_t, uint8_t);
void  createMQTTMessageConnect (ism_byte_buffer_t *, mqttclient_t *, uint8_t);
void  createMQTTMessageDisconnect (ism_byte_buffer_t *, mqttclient_t *, int);
void  createMQTTMessagePublish (ism_byte_buffer_t *, destTuple_t *, message_object_t *, mqttmessage_t *, concat_alloc_t *, uint8_t, uint8_t, uint8_t);
void  createMQTTMessageSubscribe (ism_byte_buffer_t *, destTuple_t *, mqttmessage_t *, int, uint8_t);
void  createMQTTMessageUnSubscribe (ism_byte_buffer_t *, destTuple_t *, mqttmessage_t *, int, uint8_t);
void  createMQTTMessagePingReq (ism_byte_buffer_t *, int);
int   createSIPArrayList (char *, int, srcIPList_t *, char **, int);
void  doDisplayTopicInfo (mqttclient_t *, int, destTuple_t *, int, int);
void  doDisplayClientInfo (mqttclient_t *, int ,int);
void  doDisplayClientDetail (mqttclient_t *, int);
void  doHandle_PubRec (mqttclient_t *, uint16_t);
void  submitPublish (mqttclient_t *, message_object_t *msgObj, mqttmessage_t *, int, ism_byte_buffer_t *);
void  submitACK (mqttclient_t *, ism_byte_buffer_t *);
int   submitSubscribe (mqttclient_t *);
void  submitUnSubscribe (mqttclient_t *);
int   doUnSubscribeTopicType (mqttclient_t *, int);
int   getTopicInfoFromString (char *, topicInfo_t *);
int   getTopicString (char *, int, char *);
int   getTopicVariableInfo (char *, topicInfo_t *);
void  performMQTTSubscribe (mqttclient_t *);
int   testClientListTopic (char *, int);

char * protocol_addressPort (char *, int *);

/* Used for resetting clients due to reconnecting. */
void resetMQTTClient (mqttclient_t *);


#endif /* __MQTTCLIENT_H */
